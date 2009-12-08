#pragma once

//native includes
#include "aiTypes.h"

using namespace System;

namespace AssimpNET
{

	public ref class Color4D
	{
	public:
		Color4D(void);
		Color4D (Color4D% other);
 		Color4D (float _r, float _g, float _b, float _a);
		Color4D (aiColor4D* native);

		bool IsBlack ();
		bool operator!= (const Color4D^ other);
		bool operator== (const Color4D^ other);
		float operator[] (unsigned int i);


		property float Red
		{
			float get()
			{
				return this->p_native->r;
			}
			void set(float value)
			{
				this->p_native->r = value;
			}
		}

		property float Green
		{
			float get()
			{
				return this->p_native->g;
			}
			void set(float value)
			{
				this->p_native->g = value;
			}
		}

		property float Blue
		{
			float get()
			{
				return this->p_native->b;
			}
			void set(float value)
			{
				this->p_native->b = value;
			}
		}

		property float Alpha
		{
			float get()
			{
				return this->p_native->a;
			}
			void set(float value)
			{
				this->p_native->a = value;
			}
		}

		aiColor4D* getNative();	
	private:
		aiColor4D *p_native;

	};

}//namespace