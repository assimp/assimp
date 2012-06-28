/*
Open Asset Import Library (assimp)
----------------------------------------------------------------------

Copyright (c) 2006-2012, assimp team
All rights reserved.

Redistribution and use of this software in source and binary forms, 
with or without modification, are permitted provided that the 
following conditions are met:

* Redistributions of source code must retain the above
  copyright notice, this list of conditions and the
  following disclaimer.

* Redistributions in binary form must reproduce the above
  copyright notice, this list of conditions and the
  following disclaimer in the documentation and/or other
  materials provided with the distribution.

* Neither the name of the assimp team, nor the names of its
  contributors may be used to endorse or promote products
  derived from this software without specific prior
  written permission of the assimp team.

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

----------------------------------------------------------------------
*/

/** @file  FBXDocument.cpp
 *  @brief Implementation of the FBX DOM classes
 */
#include "AssimpPCH.h"

#ifndef ASSIMP_BUILD_NO_FBX_IMPORTER

#include "FBXParser.h"
#include "FBXConverter.h"
#include "FBXDocument.h"
#include "FBXUtil.h"

namespace Assimp {
namespace FBX {
namespace {

/** Dummy class to encapsulate the conversion process */
class Converter
{

public:

	Converter(aiScene* out, const Document& doc)
		: out(out) 
		, doc(doc)
	{
		ConvertRootNode();
		TransferDataToScene();
	}


	~Converter()
	{
		std::for_each(meshes.begin(),meshes.end(),Util::delete_fun<aiMesh>());
	}


private:

	// ------------------------------------------------------------------------------------------------
	// find scene root and trigger recursive scene conversion
	void ConvertRootNode() 
	{

	}


	// ------------------------------------------------------------------------------------------------
	// MeshGeometry -> aiMesh
	void ConvertMesh(const MeshGeometry& mesh)
	{
		
	}


	// ------------------------------------------------------------------------------------------------
	// copy generated meshes, animations, lights, cameras and textures to the output scene
	void TransferDataToScene()
	{
		ai_assert(!out->mMeshes && !out->mNumMeshes);

		// note: the trailing () ensures initialization with NULL - not
		// many C++ users seem to know this, so pointing it out to avoid
		// confusion why this code works.
		out->mMeshes = new aiMesh*[meshes.size()]();
		out->mNumMeshes = static_cast<unsigned int>(meshes.size());

		std::swap_ranges(meshes.begin(),meshes.end(),out->mMeshes);
	}


private:

	std::vector<aiMesh*> meshes;

	aiScene* const out;
	const FBX::Document& doc;
};

} // !anon

// ------------------------------------------------------------------------------------------------
void ConvertToAssimpScene(aiScene* out, const Document& doc)
{
	Converter converter(out,doc);
}

} // !FBX
} // !Assimp

#endif
