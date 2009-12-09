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
#include "Matrix4x4.h"

//native inclueds
#include "aiScene.h"

using namespace System;

namespace AssimpNET
{
	public ref class Node
	{
	public:
		Node(void);
		Node(aiNode* native);
		~Node(void);

		Node^ findNode(array<char>^ name);
		Node^ findNode(const String^ name);

		property array<Node^>^ mChildren
		{
			array<Node^>^ get(){throw gcnew System::NotImplementedException();}
			void set(array<Node^>^ value){throw gcnew System::NotImplementedException();}
		}

		property array<unsigned int>^ mMeshes
		{
			array<unsigned int>^ get()
			{
				array<unsigned int>^ tmp = gcnew array<unsigned int>(this->p_native->mNumMeshes);
				System::Runtime::InteropServices::Marshal::Copy((System::IntPtr)this->p_native->mMeshes,(array<int>^)tmp,0,this->p_native->mNumMeshes);
				return tmp;
			}
			void set(array<unsigned int>^ value)
			{
				System::Runtime::InteropServices::Marshal::Copy((array<int>^)value,0,(System::IntPtr)this->p_native->mMeshes,this->p_native->mNumMeshes);
			}
		}

		property String^ mName
		{
			String^ get()
			{
				return gcnew String(this->p_native->mName.data);
			}
			void set(String^ value)
			{
				this->p_native->mName.Set((char*)System::Runtime::InteropServices::Marshal::StringToHGlobalAnsi(value).ToPointer());
			}
		}

		property unsigned int mNumChildren
		{
			unsigned int get()
			{
				return this->p_native->mNumChildren;
			}
			void set(unsigned int value)
			{
				this->p_native->mNumChildren = value;
			}
		}

		property unsigned int mNumMeshes
		{
			unsigned int get()
			{
				return this->p_native->mNumMeshes;
			}
			void set(unsigned int value)
			{
				this->p_native->mNumMeshes = value;
			}
		}

		property Matrix4x4^ mTransformation
		{
			Matrix4x4^ get()
			{
				return gcnew Matrix4x4(&this->p_native->mTransformation);
			}
			void set(Matrix4x4^ value)
			{
				this->p_native->mTransformation = aiMatrix4x4(*value->getNative());
			}
		}

		property Node^ mParent
		{
			Node^ get()
			{
				return gcnew Node(this->p_native->mParent);
			}
			void set(Node^ value)
			{
				this->p_native->mParent = new aiNode(*value->getNative());
			}
		}

		aiNode* getNative();	
	private:
		aiNode *p_native;

	};
}//namespace