// ������ DLL �ļ���

#include "stdafx.h"

#include "SegmentLibrary.h"

int SegmentLibrary::Calculator::Add(int a, int b)
{
	return a + b;
}

int SegmentLibrary::Calculator::Minus(int a, int b)
{
	return a - b;
}

String ^ SegmentLibrary::Calculator::SayError()
{
	return gcnew String("����������");

}

