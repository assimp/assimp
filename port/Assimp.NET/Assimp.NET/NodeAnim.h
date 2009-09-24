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
#include "VectorKey.h"
#include "QuatKey.h"

//native includes
#include "aiAnim.h"

using namespace System;

namespace AssimpNET
{
	public enum AnimBehaviour
	{
		aiAnimBehaviour_DEFAULT = 0x0,
		aiAnimBehaviour_CONSTANT = 0x1,
		aiAnimBehaviour_LINEAR = 0x2,
		aiAnimBehaviour_REPEAT = 0x3,
		_aiAnimBehaviour_Force32Bit = 0x8fffffff
	};

	public ref class NodeAnim
	{
	public:
		NodeAnim(void);
		NodeAnim(aiNodeAnim* native);
		~NodeAnim(void);

		property String^ mNodeName
		{
			String^ get(){throw gcnew System::NotImplementedException();}
			void set(String^ value){throw gcnew System::NotImplementedException();}
		}

		property unsigned int mNumPositionKeys
		{
			unsigned int get(){throw gcnew System::NotImplementedException();}
			void set(unsigned int value){throw gcnew System::NotImplementedException();}
		}

		property unsigned int mNumRotationKeys
		{
			unsigned int get(){throw gcnew System::NotImplementedException();}
			void set(unsigned int value){throw gcnew System::NotImplementedException();}
		}

		property unsigned int mNumScalingKeys
		{
			unsigned int get(){throw gcnew System::NotImplementedException();}
			void set(unsigned int value){throw gcnew System::NotImplementedException();}
		}

		property array<VectorKey^>^ mPositionKeys
		{
			array<VectorKey^>^ get(){throw gcnew System::NotImplementedException();}
			void set(array<VectorKey^>^ value){throw gcnew System::NotImplementedException();}
		}

		property AnimBehaviour mPosState
		{
			AnimBehaviour get(){throw gcnew System::NotImplementedException();}
			void set(AnimBehaviour value){throw gcnew System::NotImplementedException();}
		}

		property AnimBehaviour mPreState
		{
			AnimBehaviour get(){throw gcnew System::NotImplementedException();}
			void set(AnimBehaviour value){throw gcnew System::NotImplementedException();}
		}

		property array<QuatKey^>^ mRotationKeys
		{
			array<QuatKey^>^ get(){throw gcnew System::NotImplementedException();}
			void set(array<QuatKey^>^ value){throw gcnew System::NotImplementedException();}
		}

		property array<VectorKey^>^ mScalingKeys
		{
			array<VectorKey^>^ get(){throw gcnew System::NotImplementedException();}
			void set(array<VectorKey^>^ value){throw gcnew System::NotImplementedException();}
		}

		aiNodeAnim* getNative();	
	private:
		aiNodeAnim *p_native;

	};
}//namespace