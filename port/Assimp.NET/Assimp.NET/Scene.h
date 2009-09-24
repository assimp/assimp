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
#include "Animation.h"
#include "Camera.h"
#include "Light.h"
#include "Material.h"
#include "Mesh.h"
#include "Node.h"
#include "Texture.h"

//native includes
#include "aiScene.h"


using namespace System;

namespace AssimpNET
{
	public ref class Scene
	{
	public:
		Scene(void);
		Scene(aiScene* native);
		~Scene(void);

		bool HasAnimations();
		bool HasCameras();
		bool HasLights();
		bool HasMaterials();
		bool HasMeshes();
		bool HasTextures();
		
		property array<Animation^>^ mAnimations
		{
			array<Animation^>^ get(){throw gcnew System::NotImplementedException();}
			void set(array<Animation^>^ value){throw gcnew System::NotImplementedException();}
		}
		property array<Camera^>^ mCameras
		{
			array<Camera^>^ get(){throw gcnew System::NotImplementedException();}
			void set(array<Camera^>^ value){throw gcnew System::NotImplementedException();}
		}
		property unsigned int mFlags
		{
			unsigned int get(){throw gcnew System::NotImplementedException();}
			void set(unsigned int value){throw gcnew System::NotImplementedException();}
		}
		property array<Light^>^ mLights
		{
			array<Light^>^ get(){throw gcnew System::NotImplementedException();}
			void set(array<Light^>^ value){throw gcnew System::NotImplementedException();}
		}
		property array<Material^>^ mMaterials
		{
			array<Material^>^ get(){throw gcnew System::NotImplementedException();}
			void set(array<Material^>^ value){throw gcnew System::NotImplementedException();}
		}
		property array<Mesh^>^ mMeshes
		{
			array<Mesh^>^ get(){throw gcnew System::NotImplementedException();}
			void set(array<Mesh^>^ value){throw gcnew System::NotImplementedException();}
		}
		property unsigned int mNumAnimations
		{
			unsigned int get(){throw gcnew System::NotImplementedException();}
			void set(unsigned int value){throw gcnew System::NotImplementedException();}
		}
		property unsigned int mNumCameras
		{
			unsigned int get(){throw gcnew System::NotImplementedException();}
			void set(unsigned int value){throw gcnew System::NotImplementedException();}
		}
		property unsigned int mNumLights
		{
			unsigned int get(){throw gcnew System::NotImplementedException();}
			void set(unsigned int value){throw gcnew System::NotImplementedException();}
		}
		property unsigned int mNumMaterials
		{
			unsigned int get(){throw gcnew System::NotImplementedException();}
			void set(unsigned int value){throw gcnew System::NotImplementedException();}
		}
		property unsigned int mNumMeshes
		{
			unsigned int get(){throw gcnew System::NotImplementedException();}
			void set(unsigned int value){throw gcnew System::NotImplementedException();}
		}
		property unsigned int mNumTextures
		{
			unsigned int get(){throw gcnew System::NotImplementedException();}
			void set(unsigned int value){throw gcnew System::NotImplementedException();}
		}
		property Node^ mRootNode
		{
			Node^ get(){throw gcnew System::NotImplementedException();}
			void set(Node^ value){throw gcnew System::NotImplementedException();}
		}
		property array<Texture^>^ mTextures
		{
			array<Texture^>^ get(){throw gcnew System::NotImplementedException();}
			void set(array<Texture^>^ value){throw gcnew System::NotImplementedException();}
		}

		aiScene* getNative();	
	private:
		aiScene *p_native;

	};
}//namespace