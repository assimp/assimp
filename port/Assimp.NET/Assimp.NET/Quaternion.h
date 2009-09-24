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

//native includes
#include "aiTypes.h"
#include "aiQuaternion.h"


using namespace System;

namespace AssimpNET
{
	ref class Matrix3x3;

	public ref class Quaternion
	{
	public:
		Quaternion(void);
		Quaternion(Vector3D^ normalized);
		Quaternion(Vector3D^ axis, float angle);
		Quaternion(float rotx, float roty, float rotz);
		Quaternion(Matrix3x3^ rotMatrix);
		Quaternion(float _w, float _x, float _y, float _z);
		Quaternion(aiQuaternion* native);
		~Quaternion(void);

		Quaternion^ Conjugate();
		Matrix3x3^ GetMatrix();
		Quaternion^ Nomalize();
		bool operator != (const Quaternion^ q);
		bool operator == (const Quaternion^ q);
		Quaternion^ operator* (const Quaternion^ q);
		Vector3D^ Rotate(const Vector3D^ in);

		static void Interpolate(Quaternion^ pOut, const Quaternion^ pStart, const Quaternion^ pEnd, float factor);

		property float x
		{
			float get() { throw gcnew System::NotImplementedException();}
			void set(float value) { throw gcnew System::NotImplementedException();}
		}

		property float y
		{
			float get() { throw gcnew System::NotImplementedException();}
			void set(float value) { throw gcnew System::NotImplementedException();}
		}

		property float z
		{
			float get() { throw gcnew System::NotImplementedException();}
			void set(float value) { throw gcnew System::NotImplementedException();}
		}

		property float w
		{
			float get() { throw gcnew System::NotImplementedException();}
			void set(float value) { throw gcnew System::NotImplementedException();}
		}

		aiQuaternion* getNative();	
	private:
		aiQuaternion *p_native;

	};
}//namespace