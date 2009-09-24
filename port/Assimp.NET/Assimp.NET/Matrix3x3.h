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

//managed includes
#include "Vector3D.h"
#include "Vector2D.h"
#include "Matrix4x4.h"

//native includes
#include "aiVector3D.h"
#include "aiMatrix3x3.h"

namespace AssimpNET
{
	public ref class Matrix3x3
	{
	public:
		Matrix3x3(void);
		Matrix3x3(Matrix4x4^ matrix);
		Matrix3x3(	float _a1, float _a2, float _a3,
					float _b1, float _b2, float _b3,
					float _c1, float _c2, float _c3);
		Matrix3x3(aiMatrix3x3* native);
		~Matrix3x3(void);

		float Determinant();

		Matrix3x3^ Inverse();
		Matrix3x3^ Transpose();
		bool operator != (const Matrix3x3^ m);
		bool operator == (const Matrix3x3^ m);
		Matrix3x3^ operator* (const Matrix3x3^ m);
		Matrix3x3^ operator*= (const Matrix3x3^ m);
		float operator[](unsigned int i);

		static Matrix3x3^ FromToMatrix(Vector3D^ from, Vector3D^ to, Matrix3x3^ out);
		static Matrix3x3^ Rotation(float a, const Vector3D^ axis, Matrix3x3^ out);
		static Matrix3x3^ RotationZ(float a, Matrix3x3^ out);
		static Matrix3x3^ Translation(const Vector2D^ v, Matrix3x3^ out);

		property float a1
		{
			float get(){throw gcnew System::NotImplementedException();}
			void set(float value){throw gcnew System::NotImplementedException();}
		}

		property float a2
		{
			float get(){throw gcnew System::NotImplementedException();}
			void set(float value){throw gcnew System::NotImplementedException();}
		}

		property float a3
		{
			float get(){throw gcnew System::NotImplementedException();}
			void set(float value){throw gcnew System::NotImplementedException();}
		}

		property float b1
		{
			float get(){throw gcnew System::NotImplementedException();}
			void set(float value){throw gcnew System::NotImplementedException();}
		}

		property float b2
		{
			float get(){throw gcnew System::NotImplementedException();}
			void set(float value){throw gcnew System::NotImplementedException();}
		}

		property float b3
		{
			float get(){throw gcnew System::NotImplementedException();}
			void set(float value){throw gcnew System::NotImplementedException();}
		}

		property float c1
		{
			float get(){throw gcnew System::NotImplementedException();}
			void set(float value){throw gcnew System::NotImplementedException();}
		}

		property float c2
		{
			float get(){throw gcnew System::NotImplementedException();}
			void set(float value){throw gcnew System::NotImplementedException();}
		}
		property float c3
		{
			float get(){throw gcnew System::NotImplementedException();}
			void set(float value){throw gcnew System::NotImplementedException();}
		}

		aiMatrix3x3* getNative();	
	private:
		aiMatrix3x3 *p_native;

	};
}//namespace