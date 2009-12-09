/*
---------------------------------------------------------------------------
Open Asset Import Library (ASSIMP)
---------------------------------------------------------------------------

Copyright (c) 2006-2008, ASSIMP Development Team

All rights reserved.

Redistribution and use of this software in source and binary forms,
with or without modification, are permitted provided that the following
conditions are met:

 * Redistributions of source code must retain the above
  copyright notice, this list of conditions and the
  following disclaimer.

 * Redistributions in binary form must reproduce the above
  copyright notice, this list of conditions and the
  following disclaimer in the documentation and/or other
  materials provided with the distribution.

 * Neither the name of the ASSIMP team, nor the names of its
  contributors may be used to endorse or promote products
  derived from this software without specific prior
  written permission of the ASSIMP Development Team.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
---------------------------------------------------------------------------
 */

#pragma once

//managend includes
#include "Texel.h"

//native includs
#include "aiTexture.h"

using namespace System;

namespace AssimpNET
{

	public enum TextureType
	{
		aiTextureType_NONE,
		aiTextureType_DIFFUSE,
		aiTextureType_SPECULAR,
		aiTextureType_AMBIENT,
		aiTextureType_EMISSIVE,
		aiTextureType_HEIGHT,
		aiTextureType_NORMALS,
		aiTextureType_SHININESS,
		aiTextureType_OPACITY,
		aiTextureType_DISPLACEMENT,
		aiTextureType_LIGHTMAP,
		aiTextureType_REFLECTION,
		aiTextureType_UNKNOWN
	};

	public enum TextureMapping
	{
		aiTextureMapping_UV,
		aiTextureMapping_SPHERE,
		aiTextureMapping_CYLINDER,
		aiTextureMapping_BOX,
		aiTextureMapping_PLANE,
		aiTextureMapping_OTHER
	};

	public enum TextureOP
	{
		aiTextureOp_Multiply,
		aiTextureOp_Add,
		aiTextureOp_Subtract,
		aiTextureOp_Divide,
		aiTextureOp_SmoothAdd,
		aiTextureOp_SignedAdd
	};

	public enum TextureMapMode
	{
		TextureMapMode_Wrap,
		TextureMapMode_Clamp,
		TextureMapMode_Decal,
		TextureMapMode_Mirror
	};

	public ref class Texture
	{
	public:
		Texture(void);
		Texture(aiTexture* native);
		~Texture(void);

		bool CheckFormat(array<char>^ s);

		property array<char, 4>^ achFormatHint
		{
			array<char, 4>^ get()
			{
				array<char, 4>^ tmp = array<char, 4>();
				System::Runtime::InteropServices::Marshal::Copy((System::IntPtr)this->p_native->achFormatHint,(array<unsigned char>^)tmp,0,4);
				return tmp;
			}
			void set(array<char, 4>^ value)
			{
				System::Runtime::InteropServices::Marshal::Copy((array<unsigned char>^)value,0,(System::IntPtr)this->p_native->achFormatHint,4);
			}
		}

		property unsigned int mHeight
		{
			unsigned int get()
			{
				return this->p_native->mHeight;
			}
			void set(unsigned int value)
			{
				this->p_native->mHeight = value;
			}
		}

		property unsigned int mWidth
		{
			unsigned int get()
			{
				return this->p_native->mWidth;
			}
			void set(unsigned int value)
			{
				this->p_native->mWidth = value;
			}
		}

		property Texel^ pcData
		{
			Texel^ get()
			{
				return gcnew Texel(this->p_native->pcData);
			}
			void set(Texel^ value)
			{
				this->p_native->pcData = new aiTexel(*value->getNative());
			}
		}

		aiTexture* getNative();	
	private:
		aiTexture *p_native;
	};
}//namespace