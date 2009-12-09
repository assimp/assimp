#pragma once

//managend includes
#include "Color4D.h"

//native includes
#include "aiTexture.h"

using namespace System;

namespace AssimpNET
{

public ref class Texel
{
	public:
		Texel(void);
		Texel(aiTexel* native);
		~Texel(void);

		operator Color4D();
		bool operator != (const Texel^ t);
		bool operator == (const Texel^ t);

		property unsigned char a
		{
			unsigned char get()
			{
				return this->p_native->a;
			}
			void set(unsigned char value)
			{
				this->p_native->a = value;
			}
		}

		property unsigned char r
		{
			unsigned char get()
			{
				return this->p_native->r;
			}
			void set(unsigned char value)
			{
				this->p_native->r = value;
			}
		}

		property unsigned char g
		{
			unsigned char get()
			{
				return this->p_native->g;
			}
			void set(unsigned char value)
			{
				this->p_native->g = value;
			}
		}

		property unsigned char b
		{
			unsigned char get()
			{
				return this->p_native->b;
			}
			void set(unsigned char value)
			{
				this->p_native->b = value;
			}
		}

		aiTexel* getNative();	
	private:
		aiTexel *p_native;
};

}//namespace
