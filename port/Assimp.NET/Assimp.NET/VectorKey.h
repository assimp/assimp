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
#include "Vector3D.h"

//native includes
#include "aiAnim.h"

using namespace System;

namespace AssimpNET
{

	public ref struct VectorKey
	{
	public:
		VectorKey();
		VectorKey(double time, Vector3D^ value);
		VectorKey(aiVectorKey* native);
		~VectorKey(void);

		bool operator != (const VectorKey^ o);
		bool operator < (const VectorKey^ o);
		bool operator == (const VectorKey^ o);
		bool operator > (const VectorKey^ o);

		property double mTime
		{
			double get()
			{
				return this->p_native->mTime;
			}
			void set(double value)
			{
				this->p_native->mTime = value;
			}
		}

		property Vector3D^ mValue
		{
			Vector3D^ get() 
			{
				return gcnew Vector3D(&this->p_native->mValue);
			}
			void set(Vector3D^ value)
			{
				this->p_native->mValue = aiVector3D(*value->getNative());
			}
		}

		aiVectorKey* getNative();	
	private:
		aiVectorKey *p_native;
	};
}