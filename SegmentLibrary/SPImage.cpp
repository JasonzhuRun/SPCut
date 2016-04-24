#include "SPImage.h"

int SPImage::SpSegment(LPCTSTR lpszPathName, int left, int top, int right, int bottom)
{
	mSegWidth = right - left;
	mSegHeight = bottom - top;
	// 加载图像
	Log("Load image...");
	int loadResult = LoadOriginalImage(lpszPathName);
	if (SEG_RESULT_LOAD_SUCCESS != loadResult)
	{
		return loadResult;
	}
	// 初始化分割数据
	Log("Initialize segmentation...");
	InitSpace();
	for (int i = 0; i < mSegHeight; i++)
	{
		for (int j = 0; j < mSegWidth; j++)
		{
			for (int k = 0; k < 3; k++)
			{
				mSegData[(i*mSegWidth + j) * MAX_FEATURE_DIM + k] = mImgData[((top + i)*mWidth + left + j) * MAX_COLOR_DIM + k];
			}
			mSegData[(i*mSegWidth + j) * MAX_FEATURE_DIM + 3] = getPositionIndex(i, j, mSegHeight, mSegWidth);
		}
	}
	// 计算运行时间
	Log("-----Start of Segmentation-----\n");
	clock_t t1, t2, t10, t11, t12, t13, t14;
	t1 = clock();
	// 预标记
	Mark(0);
	// 建立图结构
	Lattice();
	// 建模 预分类种子点
	Modeling(MAX_COLOR_DIM);
	UpdateWeights(MAX_COLOR_DIM);
	Mark(1);
	// 开始迭代
	/*for (int i = 0; i < MAX_ITERATIONS; i++)
	{
		Modeling(MAX_FEATURE_DIM);
		UpdateWeights(MAX_FEATURE_DIM);
		Dijkstra(0);
		Dijkstra(1);
		Mark(2);
	}*/
	// 将兴趣区域的标记转移到全图上面
	for (int i = 0; i < mSegHeight; i++)
	{
		for (int j = 0; j < mSegWidth; j++)
		{
			mImgMark[(top + i)*mWidth + left + j] = mSegMark[i*mSegWidth + j];
		}
	}
	GenerateImage();
	t2 = clock();
	Log("-----End of Segmentation-----");
	Log("Segmentation total cost:{0,0:F4}s\n", (t2 - t1) / 1000.0);
	ReleaseSpace();
	return 0;
}

void SPImage::InitSpace()
{
	// mImgData = new float[mWidth * mHeight * MAX_COLOR_DIM];
	// mImgMark = new int[mWidth * mHeight];
	int pointCount = mSegWidth*mSegHeight;
	mSegData = new float[pointCount*MAX_FEATURE_DIM];
	mSegMark = new int[pointCount];

	mMeansBG = new float*[BG_GAUSS_COUNT];
	for (int i = 0; i < BG_GAUSS_COUNT; i++)
	{
		mMeansBG[i] = new float[MAX_FEATURE_DIM];
	}
	mVarBG = new float *[BG_GAUSS_COUNT];
	for (int i = 0; i < BG_GAUSS_COUNT; i++)
	{
		mVarBG[i] = new float[MAX_FEATURE_DIM];
	}
	mLogwBG = new float[BG_GAUSS_COUNT];

	mMeansFG = new float*[FG_GAUSS_COUNT];
	for (int i = 0; i < FG_GAUSS_COUNT; i++)
	{
		mMeansFG[i] = new float[MAX_FEATURE_DIM];
	}
	mVarFG = new float *[FG_GAUSS_COUNT];
	for (int i = 0; i < FG_GAUSS_COUNT; i++)
	{
		mVarFG[i] = new float[MAX_FEATURE_DIM];
	}
	mLogwFG = new float[FG_GAUSS_COUNT];

	weights = new float *[pointCount];
	for (int i = 0; i < pointCount; i++)
	{
		weights[i] = new float[10];
	}

	edge = new int *[pointCount];
	for (int i = 0; i < pointCount; i++)
	{
		edge[i] = new int[8];
	}

	distances = new float *[pointCount];
	for (int i = 0; i < pointCount; i++)
	{
		distances[i] = new float[2];
	}
}

void SPImage::ReleaseSpace()
{
	if (mImgData != NULL)
	{
		delete[] mImgData;
	}
	if (mImgMark != NULL)
	{
		delete[] mImgMark;
	}
	mImgOriginal.Destroy();
}

int SPImage::LoadOriginalImage(LPCTSTR lpszPathName)
{
	HRESULT hResult = mImgOriginal.Load(lpszPathName);
	mImgBinary.Load(lpszPathName);
	mImgTest.Load(lpszPathName);
	mImgOverlay.Load(lpszPathName);
	if (FAILED(hResult))
	{
		Log("Load image failed:0x{0,0:X}", hResult);
		return SEG_RESULT_LOAD_ERROR;
	}
	int nBitCount = mImgOriginal.GetBPP();
	if (nBitCount != 24)
	{
		Log("Warning! Can only process a 24bits color image");
		return SEG_RESULT_NOT_24bit;
	}
	mWidth = mImgOriginal.GetWidth();
	mHeight = mImgOriginal.GetHeight();
	Log("Image width:{0,0:D} height:{1,0:D}", mWidth, mHeight);
	mImgData = new float[mWidth * mHeight * MAX_COLOR_DIM];
	mImgMark = new int[mWidth * mHeight];

	int pitch = mImgOriginal.GetPitch();
	BYTE *bits = (BYTE *)mImgOriginal.GetBits();

	for (int i = 0; i < mHeight; i++)
	{
		for (int j = 0; j < mWidth; j++)
		{
			float R, G, B;
			float L = 0.0, a = 0.0, b = 0.0;
			B = bits[j * 3];
			G = bits[j * 3 + 1];
			R = bits[j * 3 + 2];
			RGB2Lab(R, G, B, L, a, b);

			mImgData[(i*mWidth + j) * MAX_COLOR_DIM + 0] = L;
			mImgData[(i*mWidth + j) * MAX_COLOR_DIM + 1] = a;
			mImgData[(i*mWidth + j) * MAX_COLOR_DIM + 2] = b;
		}
		bits += pitch;
	}

	for (int i = 0; i < mHeight; i++)
	{
		for (int j = 0; j < mWidth; j++)
		{
			mImgMark[i*mWidth + j] = MARK_UNKNOW;
		}
	}
	return SEG_RESULT_LOAD_SUCCESS;
}
/// 生成高斯混合模型
void SPImage::Modeling(int dim)
{
	int pointCount = mSegHeight*mSegWidth;
	int bgCount = 0;		// 背景建模像素点计数
	int fgCount = 0;		// 前景建模像素点计数
	real **bgpoint = new real*[pointCount];
	real **fgpoint = new real*[pointCount];
	for (int i = 0; i < pointCount; i++)
	{
		if (mSegMark[i] == MARK_BACKGROUND)
		{
			bgpoint[bgCount] = new real[dim];
			memcpy(bgpoint[bgCount], &mSegData[i * dim], dim * sizeof(real));
			bgCount++;
		}
		else if (mSegMark[i] == MARK_FOREGROUND)
		{
			fgpoint[fgCount] = new real[dim];
			memcpy(fgpoint[fgCount], &mSegData[i * dim], dim * sizeof(real));
			fgCount++;
		}
	}

	GMM(bgpoint, dim, bgCount, mMeansBG, mVarBG, mLogwBG, BG_GAUSS_COUNT);
	GMM(fgpoint, dim, fgCount, mMeansFG, mVarFG, mLogwFG, FG_GAUSS_COUNT);

	for (int i = 0; i < bgCount; i++)
	{
		delete[] bgpoint[i];
	}
	for (int i = 0; i < fgCount; i++)
	{
		delete[] fgpoint[i];

	}
	delete[] bgpoint;
	delete[] fgpoint;
}
/// 计算图结构
/*************
 * 7 0 1     
 * 6   2
 * 5 4 3
 * edge8:FG
 * edge9:BG
 *************/
void SPImage::Lattice()
{
	for (int i = 0; i < mSegHeight; i++)
	{
		for (int j = 0; j < mSegWidth; j++)
		{
			int index = i*mSegWidth + j;
			edge[index][0] = Edge(i - 1, j);
			edge[index][1] = Edge(i - 1, j + 1);
			edge[index][2] = Edge(i, j + 1);
			edge[index][3] = Edge(i + 1, j + 1);
			edge[index][4] = Edge(i + 1, j);
			edge[index][5] = Edge(i + 1, j - 1);
			edge[index][6] = Edge(i, j - 1);
			edge[index][7] = Edge(i - 1, j - 1);
			
			for (int k = 0; k < 8; k++)
			{
				// 有效边则计算权值
				if (edge[index][k] != -1)
				{
					weights[index][k] = EulerDistance(&mSegData[index * MAX_FEATURE_DIM], &mSegData[edge[index][k] * MAX_FEATURE_DIM]);
				}
				// 标记无效边
				else
				{
					weights[index][k] = 1e30;
				}
			}
		}
	}
}
// 更新到高斯模型的距离
void SPImage::UpdateWeights(int dim)
{
	
	for (int i = 0; i < mSegHeight; i++)
	{
		for (int j = 0; j < mSegWidth; j++)
		{
			int index = i*mSegWidth + j;
			weights[index][8] =	GaussDistance(0,&mSegData[index * MAX_FEATURE_DIM], dim);
			weights[index][9] = GaussDistance(1,&mSegData[index * MAX_FEATURE_DIM], dim);
		}
	}

}
// 计算最短路径
// label = 0 到前景源点的最短路径
// label = 1 到背景汇点的最短路径
void SPImage::Dijkstra(const int label)
{
	int targetIndex;
	if (label == 0)
	{
		targetIndex = 8;
	}
	else
	{
		targetIndex = 9;
	}
	int pointCount = mSegHeight*mSegWidth;

	struct fibheap *tmpHeap = fh_makekeyheap();
	fibheap_el ** GraphMinDistances = new fibheap_el*[pointCount];

	bool* isVisited = new bool[pointCount];

	for (int i = 0; i < pointCount; i++)
	{
		GraphMinDistances[i] = fh_insertkey(tmpHeap, weights[i][targetIndex], (void *)i);
		isVisited[i] = false;
	}
	int position;
	float minDistance;
	for (int h = 1; h < pointCount; h++)
	{
		minDistance = fh_minkey(tmpHeap);
		position = (int)fh_extractmin(tmpHeap);

		isVisited[position] = true;
		distances[position][label] = minDistance;

		long int tmpPoint;
		float tmpDis;
		for (int i = 0; i < 8; i++)
		{
			tmpPoint = edge[position][i];
			if ((tmpPoint != -1) && !isVisited[tmpPoint])
			{
				tmpDis = weights[position][i];
				tmpDis += minDistance;
				fh_replacekey(tmpHeap, GraphMinDistances[tmpPoint], tmpDis);
			}
		}
	}
	minDistance = fh_minkey(tmpHeap);
	position = (int)fh_extractmin(tmpHeap);
	distances[position][label] = minDistance;

	// release
	tmpHeap->fh_cmp_fnct = NULL;
	tmpHeap->fh_neginf = NULL;
	if (tmpHeap->fh_cons != NULL)
		free(tmpHeap->fh_cons);
	tmpHeap->fh_cons = NULL;
	delete tmpHeap;

	if (isVisited != NULL)
	{
		delete[] isVisited;
	}
//	for (int i = 0; i < pointCount; i++)
//	{
//		delete[] GraphMinDistances[i];
//	}
//	delete[] GraphMinDistances;
}
// 生成结果
void SPImage::GenerateImage()
{
	int pitch = mImgOriginal.GetPitch();
	BYTE *originalBits = (BYTE *)mImgOriginal.GetBits();
	BYTE *binaryBits = (BYTE *)mImgBinary.GetBits();
	BYTE *overlayBits = (BYTE *)mImgOverlay.GetBits();

	for (int i = 0; i < mHeight; i++)
	{
		for (int j = 0; j < mWidth; j++)
		{
			if (mImgMark[i*mWidth+j] == MARK_FOREGROUND)
			{
				binaryBits[j * 3 + 0] = 255;
				binaryBits[j * 3 + 1] = 255;
				binaryBits[j * 3 + 2] = 255;

				overlayBits[j * 3 + 0] = originalBits[j * 3 + 0];
				overlayBits[j * 3 + 1] = originalBits[j * 3 + 1];
				overlayBits[j * 3 + 2] = originalBits[j * 3 + 2];
			}
			else if(mImgMark[i*mWidth + j] == MARK_BACKGROUND)
			{
				binaryBits[j * 3 + 0] = 0;
				binaryBits[j * 3 + 1] = 0;
				binaryBits[j * 3 + 2] = 0;

				overlayBits[j * 3 + 0] = originalBits[j * 3 + 0] / 3 + 120;
				overlayBits[j * 3 + 1] = originalBits[j * 3 + 1] / 3 + 120;
				overlayBits[j * 3 + 2] = originalBits[j * 3 + 2] / 3 + 120;
			}
			else
			{
				binaryBits[j * 3 + 0] = 127;
				binaryBits[j * 3 + 1] = 127;
				binaryBits[j * 3 + 2] = 127;

				overlayBits[j * 3 + 0] = originalBits[j * 3 + 0] / 3 + 120;
				overlayBits[j * 3 + 1] = originalBits[j * 3 + 1] / 3 + 120;
				overlayBits[j * 3 + 2] = originalBits[j * 3 + 2] / 3 + 120;
			}
		}
		originalBits += pitch;
		binaryBits += pitch;
		overlayBits += pitch;
	}
	mImgOriginal.Save(_T("C:\\Users\\GaoYixuan\\Workspaces\\SPCut\\result\\original.jpg"), Gdiplus::ImageFormatJPEG);
	mImgBinary.Save(_T("C:\\Users\\GaoYixuan\\Workspaces\\SPCut\\result\\binary.jpg"), Gdiplus::ImageFormatJPEG);
	mImgOverlay.Save(_T("C:\\Users\\GaoYixuan\\Workspaces\\SPCut\\result\\overlay.jpg"), Gdiplus::ImageFormatJPEG);
}
// 测试图像
void SPImage::Test()
{

	int pitch = mImgTest.GetPitch();
	BYTE *bits = (BYTE *)mImgTest.GetBits();

	for (int i = 0; i < mSegHeight; i++)
	{
		for (int j = 0; j < mSegWidth; j++)
		{
			bits[j * 3 + 0] = mSegData[(i*mSegWidth + j) * MAX_COLOR_DIM + 3] * 25;
			bits[j * 3 + 1] = mSegData[(i*mSegWidth + j) * MAX_COLOR_DIM + 3] * 25;
			bits[j * 3 + 2] = mSegData[(i*mSegWidth + j) * MAX_COLOR_DIM + 3] * 25;
		}
		bits += pitch;
	}
	mImgTest.Save(_T("..\\result\\test.jpg"), Gdiplus::ImageFormatJPEG);
	/*std::ofstream out("C:\\result\\ModelDistance.txt",std::ios::app);
	out <<"----------------------------------------------------------"<< std::endl;
	out << std::endl;
	out << std::endl;
	out.flags(std::ios::right); //右对齐
	for (int i = 0; i < mSegHeight; i++)
	{
	for (int j = 0; j < mSegWidth; j++)
	{
	int index = i*mSegWidth + j;
	for (int k = 0; k < 8; k++)
	{
	out << std::setw(10) << weights[index][k] << " ";
	}
	out << "| ";
	out << weights[index][8] << "  " << weights[index][9] << std::endl;
	}
	}
	out.close();*/
}
// 标记各个像素点的属性
void SPImage::Mark(int type)
{
	int pointCount = mSegHeight*mSegWidth;
	if (type == 0)		// 根据像素点原始位置标记
	{
		for (int i = 0; i < mSegHeight; i++)
		{
			for (int j = 0; j < mSegWidth; j++)
			{
				if (mSegData[(i*mSegWidth + j) * MAX_FEATURE_DIM + 3] < 4)
				{
					mSegMark[i*mSegWidth + j] = MARK_FOREGROUND;
				}
				else if (mSegData[(i*mSegWidth + j) * MAX_FEATURE_DIM + 3] >  8)
				{
					mSegMark[i*mSegWidth + j] = MARK_BACKGROUND;
				}
				else
				{
					mSegMark[i*mSegWidth + j] = MARK_UNKNOW;
				}
			}
		}
	}
	else if (type == 1)	// 根据高斯混合模型标记
	{
		for (int i = 0; i < pointCount; i++)
		{
			if (weights[i][8] > weights[i][9])
			{
				mSegMark[i] = MARK_BACKGROUND;
			}
			else
			{
				mSegMark[i] = MARK_FOREGROUND;
			}
		}
	}
	else if (type == 2)	// 根据最短路径标记
	{
		for (int i = 0; i < pointCount; i++)
		{
			if (distances[i][0] > distances[i][1])
			{
				mSegMark[i] = MARK_BACKGROUND;
			}
			else
			{
				mSegMark[i] = MARK_FOREGROUND;
			}
		}
	}
}
// 计算到高斯混合模型的距离
// label 0:前景 1:背景
float SPImage::GaussDistance(int label, float* z,int dim)
{
	if (label == 0)
	{
		float diatanceFG = 1e30;
		for (int j = 0; j < FG_GAUSS_COUNT; j++)
		{
			float D = GaussDistance(0, z, dim, j);
			if (D < diatanceFG)
			{
				diatanceFG = D;
			}
		}
		return diatanceFG;
	}
	else
	{
		float diatanceBG = 1e30;
		for (int j = 0; j < BG_GAUSS_COUNT; j++)
		{
			float D = GaussDistance(1, z, dim, j);
			if (D < diatanceBG)
			{
				diatanceBG = D;
			}
		}
		return diatanceBG;
	}
}
// 计算到高斯混合模型的距离
// label 0:前景 1:背景
float SPImage::GaussDistance(int label,float* z,int dim,int k)
{
	float D;
	if (label == 0)
	{
		float D1 = 1;
		float D2 = 0;
		for (int i = 0; i < dim; i++)
		{
			D1 *= mVarFG[k][i];
			D2 += (z[i] - mMeansFG[k][i])*(z[i] - mMeansFG[k][i]) / mVarFG[k][i];
		}
		D = -mLogwFG[k] + 0.5*(log(D1) + D2) + dim / 2 * log(3.1415926);
	}
	else
	{
		float D1 = 1;
		float D2 = 0;
		for (int i = 0; i < dim; i++)
		{
			D1 *= mVarBG[k][i];
			D2 += (z[i] - mMeansBG[k][i])*(z[i] - mMeansBG[k][i]) / mVarBG[k][i];
		}
		D = -mLogwBG[k] + 0.5*(log(D1) + D2) + dim / 2 * log(3.1415926);
	}
	return  D;
}
// 计算对应坐标的邻接边的index
int SPImage::Edge(int h, int w)
{
	// 点在上边界或左边界
	if (h < 0 || w < 0)
	{
		return -1;
	}
	// 点在右边界或下边界
	else if (h == mSegHeight||w == mSegWidth)
	{
		return -1;
	}
	return h * mSegWidth + w;
}
// 计算两点之间的欧式距离
float SPImage::EulerDistance(float z1[3], float z2[3])
{
	float D = 0;
	for (int i = 0; i < 3; i++)
	{
		D += (z1[i] - z2[i])*(z1[i] - z2[i]);
	}
	return D;
}

// RGB 到 Lab颜色空间的转换
void RGB2Lab(float R, float G, float B, float &L, float &a, float &b)
{
	L = 0.2126 * R + 0.7152 * G		+ 0.0722 * B;
	a = 1.4749 * (0.2213 * R - 0.3390 * G + 0.1177 * B) + 128;
	b = 0.6245 * (0.1949 * R + 0.6057 * G - 0.8006 * B) + 128;
}
// 获取位置坐标参数，越靠近中心，数字越小
int getPositionIndex(int h, int w, int height, int width)
{
	double pro_h = 1.0 * h / height;
	double pro_w = 1.0 * w / width;
	pro_h = abs(pro_h - 0.5);
	pro_w = abs(pro_w - 0.5);
	double pro = pro_h > pro_w ? pro_h : pro_w;
	return pro * 20;
}