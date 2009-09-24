#pragma once

//native includes
#include "aiTypes.h"

using namespace System;

namespace AssimpNET
{
	public ref class Color3D
	{
	public:
		Color3D(void);
		Color3D(Color3D% other);
		Color3D(float _r, float _g, float _b);
		Color3D(aiColor3D* native);

		bool IsBlack();
		bool operator != (const Color3D^ other);
		Color3D^	operator*(float f);
		Color3D^	operator*(const Color3D^ c);
		Color3D^	operator+ (const Color3D^ c);
		Color3D^	operator- (const Color3D^ c);
		bool 		operator== (const Color3D^ other);
		float^	 	operator[] (unsigned int i);


		property float Red
		{
			float get(){throw gcnew System::NotImplementedException();}
			void set(float value){throw gcnew System::NotImplementedException();}
		}

		property float Green
		{
			float get(){throw gcnew System::NotImplementedException();}
			void set(float value){throw gcnew System::NotImplementedException();}
		}

		property float Blue
		{
			float get(){throw gcnew System::NotImplementedException();}
			void set(float value){throw gcnew System::NotImplementedException();}
		}

		aiColor3D* getNative();
		private:
		aiColor3D *p_native;

	};

}
