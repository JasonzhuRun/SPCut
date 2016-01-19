#pragma once
#define Log System::Console::WriteLine
#include <atlimage.h>
#include <vector>
#include <time.h>
#include "Gmm.h"
#include "Fibonacci/fib.h"
#include "Fibonacci/fibpriv.h"

using namespace System;
/// transfer the rgb space to lab space
void RGB2Lab(float R, float G, float B, float &L, float &a, float &b);
/// is in the center of rectangle
bool isCenter(int h,int w,int height,int width);
class SPImage
{
public:
	// parameter
	const int MAX_COLOR_DIM = 5;	// ��ɫ����ά��
	const int MAX_ITERATIONS = 3;	// ����������
	const int BG_GAUSS_COUNT = 5;	// ������˹�ɷָ���
	const int FG_GAUSS_COUNT = 5;	// ǰ����˹�ɷָ���
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

	float** mMeansFG;  //ǰ����˹ģ�͵ľ�ֵ
	float** mVarFG;    //ǰ����˹ģ�͵ķ���
	float* mLogwFG;    //ǰ����˹ģ�͵�Ȩֵ��logֵ

	float** mMeansBG;  //������˹ģ�͵ľ�ֵ
	float** mVarBG;    //������˹ģ�͵ķ���
	float* mLogwBG;    //������˹ģ��Ȩֵ��logֵ
				   
	std::vector < std::vector <int> > edge;			// edge������ͼ�е����б�
	std::vector < std::vector <float> > weights;	// ���ͼ�����бߵ�Ȩֵ

	std::vector < std::vector <float> > distances;	// ���ͼ�����е㵽ǰ��Դ��ͱ������ľ���

private:
	int LoadOriginalImage(LPCTSTR lpszPathName);
	bool InitSpace();
	void ReleaseSpace();
	void Modeling();
	void Lattice();
	void UpdateWeights();
	int Edge(int h,int w);
	void Dijkstra(const int label);
	void Mark();
	void GenerateImage();
	void Test();
	float GaussDistance(int label, float z[5]);					// ���㵽��˹ģ�͵ľ���
	float GaussDistance(int label, float z[5],int k);			// ���㵽��˹ģ�͵ľ���
	float EulerDistance(float z1[3],float z2[3]);				// ��������֮���ŷʽ����
};
