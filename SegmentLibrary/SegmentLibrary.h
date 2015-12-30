// SegmentLibrary.h

#pragma once
#include <stdlib.h>
#include <vcclr.h>
#include "SPImage.h"

using namespace System;

namespace SegmentLibrary {

	public ref class SPSegment
	{
	public:
		static int Segment(String^ imagePath,int left,int top,int right,int bottom);
	};
}
