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

#include "AssimpPCH.h"

#if !defined(ASSIMP_BUILD_NO_EXPORT) && !defined(ASSIMP_BUILD_NO_STL_EXPORTER)

#include "STLExporter.h"
#include "../include/assimp/version.h"

using namespace Assimp;
namespace Assimp	{

// ------------------------------------------------------------------------------------------------
// Worker function for exporting a scene to Stereolithograpy. Prototyped and registered in Exporter.cpp
void ExportSceneSTL(const char* pFile,IOSystem* pIOSystem, const aiScene* pScene)
{
	// invoke the exporter 
	STLExporter exporter(pFile, pScene);

	// we're still here - export successfully completed. Write the file.
	boost::scoped_ptr<IOStream> outfile (pIOSystem->Open(pFile,"wt"));
	outfile->Write( exporter.mOutput.str().c_str(), static_cast<size_t>(exporter.mOutput.tellp()),1);
}

} // end of namespace Assimp


// ------------------------------------------------------------------------------------------------
STLExporter :: STLExporter(const char* _filename, const aiScene* pScene)
: filename(_filename)
, pScene(pScene)
, endl("\n") 
{
	// make sure that all formatting happens using the standard, C locale and not the user's current locale
	const std::locale& l = std::locale("C");
	mOutput.imbue(l);

	const std::string& name = "AssimpScene";
	
	mOutput << "solid " << name << endl;
	for(unsigned int i = 0; i < pScene->mNumMeshes; ++i) {
		WriteMesh(pScene->mMeshes[i]);
	}
	mOutput << "endsolid " << name << endl;
}

// ------------------------------------------------------------------------------------------------
void STLExporter :: WriteMesh(const aiMesh* m)
{
	for (unsigned int i = 0; i < m->mNumFaces; ++i) {
		const aiFace& f = m->mFaces[i];

		// we need per-face normals. We specified aiProcess_GenNormals as pre-requisite for this exporter,
		// but nonetheless we have to expect per-vertex normals.
		aiVector3D nor;
		if (m->mNormals) {
			for(unsigned int a = 0; a < f.mNumIndices; ++a) {
				nor += m->mNormals[f.mIndices[a]];
			}
			nor.Normalize();
		}
		mOutput << " facet normal " << nor.x << " " << nor.y << " " << nor.z << endl;
		mOutput << "  outer loop" << endl; 
		for(unsigned int a = 0; a < f.mNumIndices; ++a) {
			const aiVector3D& v  = m->mVertices[f.mIndices[a]];
			mOutput << "  vertex " << v.x << " " << v.y << " " << v.z << endl;
		}

		mOutput << "  endloop" << endl; 
		mOutput << " endfacet" << endl << endl; 
	}
}

#endif
