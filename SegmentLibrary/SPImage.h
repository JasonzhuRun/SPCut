#pragma once
#define Log System::Console::WriteLine
#include <atlimage.h>
#include <vector>
#include <time.h>
#include "Gmm.h"

using namespace System;
/// transfer the rgb space to lab space
void RGB2Lab(float R, float G, float B, float &L, float &a, float &b);
/// is in the center of rectangle
bool isCenter(int h,int w,int height,int width);
class SPImage
{
public:
	// parameter
	const int MAX_COLOR_DIM = 3;	// 颜色特征维度
	const int MAX_ITERATIONS = 3;	// 最大迭代次数
	const int BG_GAUSS_COUNT = 5;	// 背景高斯成分个数
	const int FG_GAUSS_COUNT = 5;	// 前景高斯成分个数
	// constant
	const int SEG_RESULT_LOAD_ERROR = 0x01;
	const int SEG_RESULT_NOT_24bit = 0x02;
	const int SEG_RESULT_LOAD_SUCCESS = 0x03;
	
	const int MARK_UNKNOW = 0;
	const int MARK_BACKGROUND = 1;
	const int MARK_FOREGROUND = 2;
public:
	// public interface of segmentation
	int SpSegment(LPCTSTR lpszPathName, int left, int top, int right, int bottom);
private:
	CImage mImgOriginal;
	CImage mImgBinary;
	CImage mImgOverlay;
	CImage mImgTest;
	int mWidth;
	int mHeight;
	float* mImgData;
	int* mImgMark;
	int mSegWidth;
	int mSegHeight;
	float* mSegData;
	int* mSegMark;

	float** mMeansFG;  //前景高斯模型的均值
	float** mVarFG;    //前景高斯模型的方差
	float* mLogwFG;    //前景高斯模型的权值的log值

	float** mMeansBG;  //背景高斯模型的均值
	float** mVarBG;    //背景高斯模型的方差
	float* mLogwBG;    //背景高斯模型权值的log值

private:
	int LoadOriginalImage(LPCTSTR lpszPathName);
	void Modeling();
	void Lattice();
	void Mark();
	void GenerateImage();
	void Test();
	float GaussDistance(int type, float z[3],int k);		// 计算到高斯模型的距离
};
