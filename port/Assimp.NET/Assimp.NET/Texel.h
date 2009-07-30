#pragma once

#include "Color4D.h"

using namespace System;

namespace AssimpNET
{

public ref class Texel
{
	public:
		Texel(void);
		~Texel(void);

		operator Color4D();
		bool operator != (const Texel^ t);
		bool operator == (const Texel^ t);

		property unsigned char a
		{
			unsigned char get(){throw gcnew System::NotImplementedException();}
			void set(unsigned char value){throw gcnew System::NotImplementedException();}
		}

		property unsigned char r
		{
			unsigned char get(){throw gcnew System::NotImplementedException();}
			void set(unsigned char value){throw gcnew System::NotImplementedException();}
		}

		property unsigned char g
		{
			unsigned char get(){throw gcnew System::NotImplementedException();}
			void set(unsigned char value){throw gcnew System::NotImplementedException();}
		}

		property unsigned char b
		{
			unsigned char get(){throw gcnew System::NotImplementedException();}
			void set(unsigned char value){throw gcnew System::NotImplementedException();}
		}
};

}//namespace
