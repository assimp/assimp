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

//native includes
#include "aiTypes.h"

using namespace System;

namespace AssimpNET
{
	ref class Matrix3x3;
	ref class Matrix4x4;
	public ref class Vector3D
	{
	public:
		Vector3D(void);
		Vector3D(Vector3D% o);
		Vector3D(float _xyz);
		Vector3D(float _x, float _y, float _z);
		Vector3D(aiVector3D* native);
		~Vector3D(void);
		float Length();
		Vector3D^ Normalize();
		bool operator!= (const Vector3D^ other);
		Vector3D^ operator*= (const Matrix4x4^ mat);
		Vector3D^ operator*= (const Matrix3x3^ mat);
		Vector3D^ operator*= (float f);
		Vector3D^ operator+= (const Vector3D^ o);
		Vector3D^ operator-= (const Vector3D^ o);
		Vector3D^ operator/= (float f);
		bool operator== (const Vector3D^ other);
		float operator[] (unsigned int i);
		void Set (float pX, float pY, float pZ);
		float SquareLength();
		Vector3D^ SymMul(const Vector3D^ o);

		property float x
		{
			float get(){throw gcnew System::NotImplementedException();}
			void set(float value){throw gcnew System::NotImplementedException();}
		}

		property float y
		{
			float get(){throw gcnew System::NotImplementedException();}
			void set(float value){throw gcnew System::NotImplementedException();}
		}

		property float z
		{
			float get(){throw gcnew System::NotImplementedException();}
			void set(float value){throw gcnew System::NotImplementedException();}
		}

		aiVector3D* getNative();	
	private:
		aiVector3D *p_native;

	};
}//namespace
