#include <stdlib.h>
#include <vcclr.h>
#include "SPImage.h"

int main() 
{
	SPImage m_spImage;
	wchar_t * p_wch = TEXT("C:\\Users\\GaoYixuan\\Documents\\MLMR\\基于图论的交互式图像分割\\IIS_DataSet\\GC_data\\124084.jpg");
	m_spImage.SpSegment(p_wch, 15.5,11,445.5,312);
	system("pause");
	return 0;
}