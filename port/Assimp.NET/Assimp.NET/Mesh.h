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
#include "Bone.h"
#include "Color4D.h"
#include "Face.h"

//native includes
#include "aiMesh.h"

using namespace System;

namespace AssimpNET
{
	public ref class Mesh
	{
	public:
		Mesh(void);
		Mesh(aiMesh* native);
		~Mesh(void);

		unsigned int GetNumColorChannels();
		unsigned int GetNumUVChannels();
		bool HasBones();
		bool HasFaces();
		bool HasNormals();
		bool HasPositions();
		bool HasTangentsAndBitangents();
		bool HasTextureCoords();
		bool HasVertexColors();
		
		property array<Vector3D^>^ mBitangents
		{
			array<Vector3D^>^ get(){throw gcnew System::NotImplementedException();}
			void set(array<Vector3D^>^ value){throw gcnew System::NotImplementedException();}
		}

		property array<Bone^>^ mBones
		{
			array<Bone^>^ get(){throw gcnew System::NotImplementedException();}
			void set(array<Bone^>^ value){throw gcnew System::NotImplementedException();}
		}

		property array<Color4D^>^ mColors
		{
			array<Color4D^>^ get(){throw gcnew System::NotImplementedException();}
			void set(array<Color4D^>^ value){throw gcnew System::NotImplementedException();}
		}

		property array<Face^>^ mFaces
		{
			array<Face^>^ get(){throw gcnew System::NotImplementedException();}
			void set(array<Face^>^ value){throw gcnew System::NotImplementedException();}
		}

		property unsigned int mMaterialIndex
		{
			unsigned int get(){throw gcnew System::NotImplementedException();}
			void set(unsigned int value){throw gcnew System::NotImplementedException();}
		}

		property array<Vector3D^>^ mNormals
		{
			array<Vector3D^>^ get(){throw gcnew System::NotImplementedException();}
			void set(array<Vector3D^>^ value){throw gcnew System::NotImplementedException();}
		}
		
		property unsigned int mNumBones
		{
			unsigned int get(){throw gcnew System::NotImplementedException();}
			void set(unsigned int value){throw gcnew System::NotImplementedException();}
		}

		property unsigned int mNumFaces
		{
			unsigned int get(){throw gcnew System::NotImplementedException();}
			void set(unsigned int value){throw gcnew System::NotImplementedException();}
		}

		property unsigned int mNumUVComponents
		{
			unsigned int get(){throw gcnew System::NotImplementedException();}
			void set(unsigned int value){throw gcnew System::NotImplementedException();}
		}

		property unsigned int mNumVertices
		{
			unsigned int get(){throw gcnew System::NotImplementedException();}
			void set(unsigned int value){throw gcnew System::NotImplementedException();}
		}

		property unsigned int mPrimitveTypes
		{
			unsigned int get(){throw gcnew System::NotImplementedException();}
			void set(unsigned int value){throw gcnew System::NotImplementedException();}
		}

		property array<Vector3D^>^ mTangents
		{
			array<Vector3D^>^ get(){throw gcnew System::NotImplementedException();}
			void set(array<Vector3D^>^ value){throw gcnew System::NotImplementedException();}
		}

		property array<Vector3D^>^ mTextureCoords
		{
			array<Vector3D^>^ get(){throw gcnew System::NotImplementedException();}
			void set(array<Vector3D^>^ value){throw gcnew System::NotImplementedException();}
		}

		property array<Vector3D^>^ mVertices
		{
			array<Vector3D^>^ get(){throw gcnew System::NotImplementedException();}
			void set(array<Vector3D^>^ value){throw gcnew System::NotImplementedException();}
		}

		aiMesh* getNative();
		private:
		aiMesh *p_native;

	};
}//namespace