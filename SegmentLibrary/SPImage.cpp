#include "SPImage.h"

int SPImage::SpSegment(LPCTSTR lpszPathName, int left, int top, int right, int bottom)
{
	mSegWidth = right - left;
	mSegHeight = bottom - top;
	// load image
	Log("load image...");
	int loadResult = LoadOriginalImage(lpszPathName);
	if (SEG_RESULT_LOAD_SUCCESS != loadResult)
	{
		return loadResult;
	}
	mSegData = new float[mSegWidth*mSegHeight*MAX_COLOR_DIM];
	mSegMark = new int[mSegWidth*mSegHeight];

	mMeansBG = new float*[BG_GAUSS_COUNT];
	for (int i = 0; i < BG_GAUSS_COUNT; i++)
	{
		mMeansBG[i] = new float[MAX_COLOR_DIM];
	}
	mVarBG = new float *[BG_GAUSS_COUNT];
	for (int i = 0; i < BG_GAUSS_COUNT; i++)
	{
		mVarBG[i] = new float[MAX_COLOR_DIM];
	}
	mLogwBG = new float[BG_GAUSS_COUNT];

	mMeansFG = new float*[FG_GAUSS_COUNT];
	for (int i = 0; i < FG_GAUSS_COUNT; i++)
	{
		mMeansFG[i] = new float[MAX_COLOR_DIM];
	}
	mVarFG = new float *[FG_GAUSS_COUNT];
	for (int i = 0; i < FG_GAUSS_COUNT; i++)
	{
		mVarFG[i] = new float[MAX_COLOR_DIM];
	}
	mLogwFG = new float[FG_GAUSS_COUNT];
	// initialize segmentation
	Log("initialize segmentation...");
	for (int i = 0; i < mSegHeight; i++)
	{
		for (int j = 0; j < mSegWidth; j++)
		{
			mSegData[(i*mSegWidth + j) * MAX_COLOR_DIM + 0] = mImgData[((top + i)*mWidth + left + j) * MAX_COLOR_DIM + 0];
			mSegData[(i*mSegWidth + j) * MAX_COLOR_DIM + 1] = mImgData[((top + i)*mWidth + left + j) * MAX_COLOR_DIM + 1];
			mSegData[(i*mSegWidth + j) * MAX_COLOR_DIM + 2] = mImgData[((top + i)*mWidth + left + j) * MAX_COLOR_DIM + 2];
		}
	}
	for (int i = 0; i < mSegHeight; i++)
	{
		for (int j = 0; j < mSegWidth; j++)
		{
			if (isCenter(i,j, mSegHeight, mSegWidth))
			{
				mSegMark[i*mSegWidth + j] = MARK_FOREGROUND;
			}
			else
			{
				mSegMark[i*mSegWidth + j] = MARK_BACKGROUND;
			}
		}
	}
	// timing
	clock_t t1,t2,t10,t11,t12,t13,t14;
	t1 = clock();
	Log("-----Start of Segmentation-----\n");
	for (int i = 0; i < 1; i++)
	{
		t10 = clock();
		Modeling();
		t11 = clock();
		Log("modeling cost:{1,0:F4}s", i + 1, (t11 - t10) / 1000);
		Lattice();
		t12 = clock();
		Log("lattice cost:{1,0:F4}s", i + 1, (t12 - t11) / 1000);
		//Dijkstra();
		t13 = clock();
		//Log("dijkstra cost:{1,0:F4}s", i + 1, (t13 - t12) / 1000);
		Mark();
		t14 = clock();
		Log("mark cost:{1,0:F4}s", i + 1, (t14 - t13) / 1000);
		Log("No.{0,0:D} iteration total cost:{1,0:F4}s\n",i+1,(t14-t11)/1000);
	}
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
	Log("segmentation total cost:{0,0:F4}s\n", (t2 - t1) / 1000);
	Log("release space...");
	if (mImgData != NULL)
	{
		delete[] mImgData;
	}
	if (mImgMark != NULL)
	{
		delete[] mImgMark;
	}
	mImgOriginal.Destroy();
	return 0;
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
			B = bits[j * MAX_COLOR_DIM];
			G = bits[j * MAX_COLOR_DIM + 1];
			R = bits[j * MAX_COLOR_DIM + 2];
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
void SPImage::Modeling()
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
			bgpoint[bgCount] = new real[MAX_COLOR_DIM];
			memcpy(bgpoint[bgCount], &mSegData[i * MAX_COLOR_DIM], MAX_COLOR_DIM * sizeof(real));
			bgCount++;
		}
		else if (mSegMark[i] == MARK_FOREGROUND)
		{
			fgpoint[fgCount] = new real[MAX_COLOR_DIM];
			memcpy(fgpoint[fgCount], &mSegData[i * MAX_COLOR_DIM], MAX_COLOR_DIM * sizeof(real));
			fgCount++;
		}
	}

	GMM(bgpoint, MAX_COLOR_DIM, bgCount, mMeansBG, mVarBG, mLogwBG, BG_GAUSS_COUNT);
	GMM(fgpoint, MAX_COLOR_DIM, fgCount, mMeansFG, mVarFG, mLogwFG, FG_GAUSS_COUNT);

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
void SPImage::Lattice()
{

	
}

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
				binaryBits[j * MAX_COLOR_DIM + 0] = 255;
				binaryBits[j * MAX_COLOR_DIM + 1] = 255;
				binaryBits[j * MAX_COLOR_DIM + 2] = 255;

				overlayBits[j * MAX_COLOR_DIM + 0] = originalBits[j * MAX_COLOR_DIM + 0];
				overlayBits[j * MAX_COLOR_DIM + 1] = originalBits[j * MAX_COLOR_DIM + 1];
				overlayBits[j * MAX_COLOR_DIM + 2] = originalBits[j * MAX_COLOR_DIM + 2];
			}
			else
			{
				binaryBits[j * MAX_COLOR_DIM + 0] = 0;
				binaryBits[j * MAX_COLOR_DIM + 1] = 0;
				binaryBits[j * MAX_COLOR_DIM + 2] = 0;

				overlayBits[j * MAX_COLOR_DIM + 0] = originalBits[j * MAX_COLOR_DIM + 0] / 3 + 120;
				overlayBits[j * MAX_COLOR_DIM + 1] = originalBits[j * MAX_COLOR_DIM + 1] / 3 + 120;
				overlayBits[j * MAX_COLOR_DIM + 2] = originalBits[j * MAX_COLOR_DIM + 2] / 3 + 120;
			}
		}
		originalBits += pitch;
		binaryBits += pitch;
		overlayBits += pitch;
	}
	mImgOriginal.Save(_T("..\\result\\original.jpg"), Gdiplus::ImageFormatJPEG);
	mImgBinary.Save(_T("..\\result\\binary.jpg"), Gdiplus::ImageFormatJPEG);
	mImgOverlay.Save(_T("..\\result\\overlay.jpg"), Gdiplus::ImageFormatJPEG);
}

void SPImage::Test()
{

	int pitch = mImgTest.GetPitch();
	BYTE *bits = (BYTE *)mImgTest.GetBits();

	for (int i = 0; i < mHeight; i++)
	{
		for (int j = 0; j < mWidth; j++)
		{
			if (mImgMark[i*mWidth + j] == MARK_FOREGROUND)
			{
				bits[j * MAX_COLOR_DIM + 0] = 255;
				bits[j * MAX_COLOR_DIM + 1] = 255;
				bits[j * MAX_COLOR_DIM + 2] = 255;
			}
			else
			{
				bits[j * MAX_COLOR_DIM + 0] = 255;
				bits[j * MAX_COLOR_DIM + 1] = 255;
				bits[j * MAX_COLOR_DIM + 2] = 255;
			}
		}
		bits += pitch;
	}
	mImgTest.Save(_T("..\\result\\test.jpg"), Gdiplus::ImageFormatJPEG);
}
/// 标记各个像素点的属性
void SPImage::Mark()
{
	int pointCount = mSegHeight*mSegWidth;
	for (int i = 0; i < pointCount; i++)
	{
		float distanceFG = 0;
		float diatanceBG = 0;
		for (int j = 0; j < BG_GAUSS_COUNT; j++)
		{
			diatanceBG += GaussDistance(0, &mSegData[i * MAX_COLOR_DIM], j);
		}
		
		for (int j = 0; j < FG_GAUSS_COUNT; j++)
		{
			distanceFG += GaussDistance(1, &mSegData[i * MAX_COLOR_DIM], j);
		}
		if (distanceFG > diatanceBG)
		{
			mSegMark[i] = MARK_FOREGROUND;
		}
		else
		{
			mSegMark[i] = MARK_BACKGROUND;
		}
	}
}
/// 计算到高斯混合模型的距离
/// type 0:背景 1:前景
float SPImage::GaussDistance(int type,float z[3],int k)
{
	float D;
	if (type == 0)
	{
		D = -mLogwBG[k] + 0.5*(log(mVarBG[k][0] * mVarBG[k][1] * mVarBG[k][2]) +
			(z[0] - mMeansBG[k][0])*(z[0] - mMeansBG[k][0]) / mVarBG[k][0] +
			(z[1] - mMeansBG[k][1])*(z[1] - mMeansBG[k][1]) / mVarBG[k][1] +
			(z[2] - mMeansBG[k][2])*(z[2] - mMeansBG[k][2]) / mVarBG[k][2]);
	}
	else
	{
		D = -mLogwFG[k] + 0.5*(log(mVarFG[k][0] * mVarFG[k][1] * mVarFG[k][2]) +
			(z[0] - mMeansFG[k][0])*(z[0] - mMeansFG[k][0]) / mVarFG[k][0] +
			(z[1] - mMeansFG[k][1])*(z[1] - mMeansFG[k][1]) / mVarFG[k][1] +
			(z[2] - mMeansFG[k][2])*(z[2] - mMeansFG[k][2]) / mVarFG[k][2]);
	}
	return  D;
}

void RGB2Lab(float R, float G, float B, float &L, float &a, float &b)
{
	L = 0.2126 * R + 0.7152 * G		+ 0.0722 * B;
	a = 1.4749 * (0.2213 * R - 0.3390 * G + 0.1177 * B) + 128;
	b = 0.6245 * (0.1949 * R + 0.6057 * G - 0.8006 * B) + 128;
}

bool isCenter(int h, int w, int height, int width)
{
	if (h < 5||w < 5)
	{
		return false;
	}
	if (height - h <= 5 || width - w <= 5)
	{
		return false;
	}
	return true;
}