// SegmentLibrary.h

#pragma once

using namespace System;

namespace SegmentLibrary {

	public ref class Calculator
	{
	public:
		static int Add(int a, int b);

		static int Minus(int a, int b);


		static String^ SayError();

	};
}
