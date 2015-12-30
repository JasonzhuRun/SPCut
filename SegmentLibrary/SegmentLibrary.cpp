// 这是主 DLL 文件
#include "SegmentLibrary.h"
using namespace System::Runtime::InteropServices;

int SegmentLibrary::SPSegment::Segment(String^ imagePath, int left, int top, int right, int bottom)
{
	SPImage m_spImage;
	wchar_t * p_wch = (wchar_t *)(void*)Marshal::StringToHGlobalUni(imagePath);
	m_spImage.SpSegment(p_wch, left, top, right, bottom);
	return 0;
}

