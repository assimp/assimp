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

#if !defined(ASSIMP_BUILD_NO_EXPORT) && !defined(ASSIMP_BUILD_NO_PLY_EXPORTER)

#include "PlyExporter.h"
#include "../include/assimp/version.h"

using namespace Assimp;
namespace Assimp	{

// ------------------------------------------------------------------------------------------------
// Worker function for exporting a scene to PLY. Prototyped and registered in Exporter.cpp
void ExportScenePly(const char* pFile,IOSystem* pIOSystem, const aiScene* pScene)
{
	// invoke the exporter 
	PlyExporter exporter(pFile, pScene);

	// we're still here - export successfully completed. Write the file.
	boost::scoped_ptr<IOStream> outfile (pIOSystem->Open(pFile,"wt"));
	if(outfile == NULL) {
		throw DeadlyExportError("could not open output .ply file: " + std::string(pFile));
	}

	outfile->Write( exporter.mOutput.str().c_str(), static_cast<size_t>(exporter.mOutput.tellp()),1);
}

} // end of namespace Assimp

#define PLY_EXPORT_HAS_NORMALS 0x1
#define PLY_EXPORT_HAS_TANGENTS_BITANGENTS 0x2
#define PLY_EXPORT_HAS_TEXCOORDS 0x4
#define PLY_EXPORT_HAS_COLORS (PLY_EXPORT_HAS_TEXCOORDS << AI_MAX_NUMBER_OF_TEXTURECOORDS)

// ------------------------------------------------------------------------------------------------
PlyExporter :: PlyExporter(const char* _filename, const aiScene* pScene)
: filename(_filename)
, pScene(pScene)
, endl("\n") 
{
	// make sure that all formatting happens using the standard, C locale and not the user's current locale
	const std::locale& l = std::locale("C");
	mOutput.imbue(l);

	unsigned int faces = 0u, vertices = 0u, components = 0u;
	for (unsigned int i = 0; i < pScene->mNumMeshes; ++i) {
		const aiMesh& m = *pScene->mMeshes[i];
		faces += m.mNumFaces;
		vertices += m.mNumVertices;

		if (m.HasNormals()) {
			components |= PLY_EXPORT_HAS_NORMALS;
		}
		if (m.HasTangentsAndBitangents()) {
			components |= PLY_EXPORT_HAS_TANGENTS_BITANGENTS;
		}
		for (unsigned int t = 0; m.HasTextureCoords(t); ++t) {
			components |= PLY_EXPORT_HAS_TEXCOORDS << t;
		}
		for (unsigned int t = 0; m.HasVertexColors(t); ++t) {
			components |= PLY_EXPORT_HAS_COLORS << t;
		}
	}

	mOutput << "ply" << endl;
	mOutput << "format ascii 1.0" << endl;
	mOutput << "comment Created by Open Asset Import Library - http://assimp.sf.net (v"
		<< aiGetVersionMajor() << '.' << aiGetVersionMinor() << '.' 
		<< aiGetVersionRevision() << ")" << endl;

	mOutput << "element vertex " << vertices << endl;
	mOutput << "property float x" << endl;
	mOutput << "property float y" << endl;
	mOutput << "property float z" << endl;

	if(components & PLY_EXPORT_HAS_NORMALS) {
		mOutput << "property float nx" << endl;
		mOutput << "property float ny" << endl;
		mOutput << "property float nz" << endl;
	}

	// write texcoords first, just in case an importer does not support tangents
	// bitangents and just skips over the rest of the line upon encountering
	// unknown fields (Ply leaves pretty much every vertex component open,
	// but in reality most importers only know about vertex positions, normals
	// and texture coordinates).
	for (unsigned int n = PLY_EXPORT_HAS_TEXCOORDS, c = 0; (components & n) && c != AI_MAX_NUMBER_OF_TEXTURECOORDS; n <<= 1, ++c) {
		if (!c) {
			mOutput << "property float s" << endl;
			mOutput << "property float t" << endl;
		}
		else {
			mOutput << "property float s" << c << endl;
			mOutput << "property float t" << c << endl;
		}
	}

	for (unsigned int n = PLY_EXPORT_HAS_COLORS, c = 0; (components & n) && c != AI_MAX_NUMBER_OF_COLOR_SETS; n <<= 1, ++c) {
		if (!c) {
			mOutput << "property float r" << endl;
			mOutput << "property float g" << endl;
			mOutput << "property float b" << endl;
			mOutput << "property float a" << endl;
		}
		else {
			mOutput << "property float r" << c << endl;
			mOutput << "property float g" << c << endl;
			mOutput << "property float b" << c << endl;
			mOutput << "property float a" << c << endl;
		}
	}

	if(components & PLY_EXPORT_HAS_TANGENTS_BITANGENTS) {
		mOutput << "property float tx" << endl;
		mOutput << "property float ty" << endl;
		mOutput << "property float tz" << endl;
		mOutput << "property float bx" << endl;
		mOutput << "property float by" << endl;
		mOutput << "property float bz" << endl;
	}

	mOutput << "element face " << faces << endl;
	mOutput << "property list uint uint vertex_index" << endl;
	mOutput << "end_header" << endl;

	for (unsigned int i = 0; i < pScene->mNumMeshes; ++i) {
		WriteMeshVerts(pScene->mMeshes[i],components);
	}
	for (unsigned int i = 0, ofs = 0; i < pScene->mNumMeshes; ++i) {
		WriteMeshIndices(pScene->mMeshes[i],ofs);
		ofs += pScene->mMeshes[i]->mNumVertices;
	}
}

// ------------------------------------------------------------------------------------------------
void PlyExporter :: WriteMeshVerts(const aiMesh* m, unsigned int components)
{
	for (unsigned int i = 0; i < m->mNumVertices; ++i) {
		mOutput << 
			m->mVertices[i].x << " " << 
			m->mVertices[i].y << " " << 
			m->mVertices[i].z
		;
		if(components & PLY_EXPORT_HAS_NORMALS) {
			if (m->HasNormals()) {
				mOutput << 
				" " << m->mNormals[i].x << 
				" " << m->mNormals[i].y << 
				" " << m->mNormals[i].z;
			}
			else {
				mOutput << " 0.0 0.0 0.0"; 
			}
		}

		for (unsigned int n = PLY_EXPORT_HAS_TEXCOORDS, c = 0; (components & n) && c != AI_MAX_NUMBER_OF_TEXTURECOORDS; n <<= 1, ++c) {
			if (m->HasTextureCoords(c)) {
				mOutput << 
					" " << m->mTextureCoords[c][i].x << 
					" " << m->mTextureCoords[c][i].y;
			}
			else {
				mOutput << " -1.0 -1.0"; 
			}
		}

		for (unsigned int n = PLY_EXPORT_HAS_COLORS, c = 0; (components & n) && c != AI_MAX_NUMBER_OF_COLOR_SETS; n <<= 1, ++c) {
			if (m->HasVertexColors(c)) {
				mOutput << 
					" " << m->mColors[c][i].r << 
					" " << m->mColors[c][i].g <<
					" " << m->mColors[c][i].b <<
					" " << m->mColors[c][i].a;
			}
			else {
				mOutput << " -1.0 -1.0 -1.0 -1.0"; 
			}
		}

		if(components & PLY_EXPORT_HAS_TANGENTS_BITANGENTS) {
			if (m->HasTangentsAndBitangents()) {
				mOutput << 
				" " << m->mTangents[i].x << 
				" " << m->mTangents[i].y << 
				" " << m->mTangents[i].z << 
				" " << m->mBitangents[i].x << 
				" " << m->mBitangents[i].y << 
				" " << m->mBitangents[i].z
				;
			}
			else {
				mOutput << " 0.0 0.0 0.0 0.0 0.0 0.0"; 
			}
		}

		mOutput << endl;
	}
}

// ------------------------------------------------------------------------------------------------
void PlyExporter :: WriteMeshIndices(const aiMesh* m, unsigned int offset)
{
	for (unsigned int i = 0; i < m->mNumFaces; ++i) {
		const aiFace& f = m->mFaces[i];
		mOutput << f.mNumIndices << " ";
		for(unsigned int c = 0; c < f.mNumIndices; ++c) {
			mOutput << (f.mIndices[c] + offset) << (c == f.mNumIndices-1 ? endl : " ");
		}
	}
}

#endif
