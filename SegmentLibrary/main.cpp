#include <stdlib.h>
#include <vcclr.h>
#include "SPImage.h"
using namespace System::Runtime::InteropServices;

int main() 
{
	SPImage m_spImage;
	wchar_t * p_wch = TEXT("C:\\Users\\GaoYixuan\\Documents\\MLMR\\基于图论的交互式图像分割\\IIS_DataSet\\GC_data\\fullmoon.bmp");
	m_spImage.SpSegment(p_wch, 141,95,285,248);
	system("pause");
	return 0;
}