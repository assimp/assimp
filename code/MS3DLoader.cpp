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

/** @file  MS3DLoader.cpp
 *  @brief Implementation of the Ms3D importer class.
 *  Written against http://chumbalum.swissquake.ch/ms3d/ms3dspec.txt
 */

#include "AssimpPCH.h"
#ifndef ASSIMP_BUILD_NO_MS3D_IMPORTER

// internal headers
#include "MS3DLoader.h"
#include "StreamReader.h"
using namespace Assimp;

// ------------------------------------------------------------------------------------------------
// Constructor to be privately used by Importer
MS3DImporter::MS3DImporter()
{}

// ------------------------------------------------------------------------------------------------
// Destructor, private as well 
MS3DImporter::~MS3DImporter()
{}

// ------------------------------------------------------------------------------------------------
// Returns whether the class can handle the format of the given file. 
bool MS3DImporter::CanRead( const std::string& pFile, IOSystem* pIOHandler, bool checkSig) const
{
	// first call - simple extension check
	const std::string extension = GetExtension(pFile);
	if (extension == "ms3d") {
		return true;
	}

	// second call - check for magic identifiers
	else if (!extension.length() || checkSig)	{
		if (!pIOHandler) {
			return true;
		}
		const char* tokens[] = {"MS3D000000"};
		return SearchFileHeaderForToken(pIOHandler,pFile,tokens,1);
	}
	return false;
}

// ------------------------------------------------------------------------------------------------
void MS3DImporter::GetExtensionList(std::string& append)
{
	append.append("*.ms3d");
}

// ------------------------------------------------------------------------------------------------
void ReadColor(StreamReaderLE& stream, aiColor4D& ambient)
{
	// aiColor4D is packed on gcc, implicit binding to float& fails therefore.
	// But I guess casting is fine (it could cause alignment faults on some
	// architectures in general, but we're not touched because aiColor4D
	// should be properly aligned & packed due to its uniform structure)
	stream >> (float&)ambient.r >> (float&)ambient.g >> (float&)ambient.b >> (float&)ambient.a;
}

// ------------------------------------------------------------------------------------------------
void ReadVector(StreamReaderLE& stream, aiVector3D& pos)
{
	// See note in ReadColor()
	stream >> (float&)pos.x >> (float&)pos.y >> (float&)pos.z;
}

// ------------------------------------------------------------------------------------------------
// Imports the given file into the given scene structure. 
void MS3DImporter::InternReadFile( const std::string& pFile, 
	aiScene* pScene, IOSystem* pIOHandler)
{
	StreamReaderLE stream(pIOHandler->Open(pFile,"rb"));

	// CanRead() should have done this already
	char head[10];
	int32_t version;


	// 1 ------------ read into temporary data structures mirroring the original file

	stream.CopyAndAdvance(head,10);
	stream >> version;
	if (strncmp(head,"MS3D000000",10)) {
		throw new ImportErrorException("Not a MS3D file, magic string MS3D000000 not found: "+pFile);
	}

	if (version != 4) {
		throw new ImportErrorException("MS3D: Unsupported file format version, 4 was expected");
	}

	uint16_t verts;
	stream >> verts;

	std::vector<TempVertex> vertices(verts);
	for (unsigned int i = 0; i < verts; ++i) {
		TempVertex& v = vertices[i];

		stream.IncPtr(1);
		ReadVector(stream,v.pos);
		v.bone_id = static_cast<unsigned int>(stream.GetI1()); // signed in original spec !! intentional bit hack.
		v.ref_cnt = static_cast<unsigned int>(stream.GetI1());
	}

	uint16_t tris;
	stream >> tris;

	std::vector<TempTriangle> triangles(tris);
	for (unsigned int i = 0;i < tris; ++i) {
		TempTriangle& t = triangles[i];

		stream.IncPtr(2);
		for (unsigned int i = 0; i < 3; ++i) {
			t.indices[i] = static_cast<unsigned int>(stream.GetI2());
		}

		for (unsigned int i = 0; i < 3; ++i) {
			ReadVector(stream,t.normals[i]);
		}

		for (unsigned int i = 0; i < 3; ++i) {
			stream >> (float&)(t.uv[i].x); // see note in ReadColor()
		}
		for (unsigned int i = 0; i < 3; ++i) {
			stream >> (float&)(t.uv[i].y);
		}

		t.sg    = static_cast<unsigned int>(stream.GetI1()); 
		t.group = static_cast<unsigned int>(stream.GetI1()); 
	}

	uint16_t grp;
	stream >> grp;

	bool need_default = false;
	std::vector<TempGroup> groups(grp);
	for (unsigned int i = 0;i < grp; ++i) {
		TempGroup& t = groups[i];

		stream.IncPtr(1);
		stream.CopyAndAdvance(t.name,32);

		t.name[32] = '\0';
		uint16_t num;
		stream >> num;

		t.triangles.resize(num);
		for (unsigned int i = 0; i < num; ++i) {
			t.triangles[i] = static_cast<unsigned int>(stream.GetI2()); 
		}
		t.mat = static_cast<unsigned int>(stream.GetI1()); 
		if (t.mat == 0xff) {
			need_default = true;
		}
	}

	uint16_t mat;
	stream >> mat;

	std::vector<TempMaterial> materials(mat);
	for (unsigned int i = 0;i < mat; ++i) {
		TempMaterial& t = materials[i];

		stream.CopyAndAdvance(t.name,32);
		t.name[32] = '\0';

		ReadColor(stream,t.ambient);
		ReadColor(stream,t.diffuse);
		ReadColor(stream,t.specular);
		ReadColor(stream,t.emissive);
		stream >> t.shininess  >> t.transparency;

		stream.IncPtr(1);

		stream.CopyAndAdvance(t.texture,128);
		t.texture[128] = '\0';

		stream.CopyAndAdvance(t.alphamap,128);
		t.alphamap[128] = '\0';
	}


	// 2 ------------ convert to proper aiXX data structures

	if (need_default && materials.size()) {
		DefaultLogger::get()->warn("MS3D: Found group with no material assigned, spawning default material");
		// if one of the groups has no material assigned, but there are other 
		// groups with materials, a default material needs to be added (
		// scenepreprocessor adds a default material only if nummat==0).
		materials.push_back(TempMaterial());
		TempMaterial& m = materials.back();

		strcpy(m.name,"<MS3D_DefaultMat>");
		m.diffuse = aiColor4D(0.6f,0.6f,0.6f,1.0);
		m.transparency = 1.f;
		m.shininess = 0.f;

		for (unsigned int i = 0; i < groups.size(); ++i) {
			TempGroup& g = groups[i];
			if (g.mat == 0xff) {
				g.mat = materials.size()-1;
			}
		}
	}

	// convert materials to our generic key-value dict-alike
	if (materials.size()) {
		pScene->mMaterials = new aiMaterial*[pScene->mNumMaterials=static_cast<unsigned int>(materials.size())];
		for (unsigned int i = 0; i < pScene->mNumMaterials; ++i) {

			MaterialHelper* mo = new MaterialHelper();
			pScene->mMaterials[i] = mo;

			const TempMaterial& mi = materials[i];

			aiString tmp;
			if (0[mi.alphamap]) {
				tmp = aiString(mi.alphamap);
				mo->AddProperty(&tmp,AI_MATKEY_TEXTURE_OPACITY(0));
			}
			if (0[mi.texture]) {
				tmp = aiString(mi.texture);
				mo->AddProperty(&tmp,AI_MATKEY_TEXTURE_DIFFUSE(0));
			}
			if (0[mi.name]) {
				tmp = aiString(mi.name);
				mo->AddProperty(&tmp,AI_MATKEY_NAME);
			}

			mo->AddProperty(&mi.ambient,1,AI_MATKEY_COLOR_AMBIENT);
			mo->AddProperty(&mi.diffuse,1,AI_MATKEY_COLOR_DIFFUSE);
			mo->AddProperty(&mi.specular,1,AI_MATKEY_COLOR_SPECULAR);
			mo->AddProperty(&mi.emissive,1,AI_MATKEY_COLOR_EMISSIVE);

			mo->AddProperty(&mi.shininess,1,AI_MATKEY_SHININESS);
			mo->AddProperty(&mi.transparency,1,AI_MATKEY_OPACITY);

			const int sm = mi.shininess>0.f?aiShadingMode_Phong:aiShadingMode_Gouraud;
			mo->AddProperty(&sm,1,AI_MATKEY_SHADING_MODEL);
		}
	}

	// convert groups to meshes
	if (groups.empty()) {
		throw new ImportErrorException("MS3D: Didn't get any group records, file is malformed");
	}

	pScene->mMeshes = new aiMesh*[pScene->mNumMeshes=static_cast<unsigned int>(groups.size())];
	for (unsigned int i = 0; i < pScene->mNumMeshes; ++i) {
	
		aiMesh* m = pScene->mMeshes[i] = new aiMesh();
		const TempGroup& g = groups[i];

		if (pScene->mNumMaterials && g.mat > pScene->mNumMaterials) {
			throw new ImportErrorException("MS3D: Encountered invalid material index, file is malformed");
		} // no error if no materials at all - scenepreprocessor adds one then

		m->mMaterialIndex  = g.mat;
		m->mPrimitiveTypes = aiPrimitiveType_TRIANGLE; 

		m->mFaces = new aiFace[m->mNumFaces = g.triangles.size()];
		m->mNumVertices = m->mNumFaces*3;

		// storage for vertices - verbose format, as requested by the postprocessing pipeline
		m->mVertices = new aiVector3D[m->mNumVertices];
		m->mNormals  = new aiVector3D[m->mNumVertices];
		m->mTextureCoords[0] = new aiVector3D[m->mNumVertices];
		m->mNumUVComponents[0] = 2;

		for (unsigned int i = 0,n = 0; i < m->mNumFaces; ++i) {
			aiFace& f = m->mFaces[i];
			if (g.triangles[i]>triangles.size()) {
				throw new ImportErrorException("MS3D: Encountered invalid triangle index, file is malformed");
			}

			TempTriangle& t = triangles[g.triangles[i]];
			f.mIndices = new unsigned int[f.mNumIndices=3];
			
			for (unsigned int i = 0; i < 3; ++i,++n) {
				if (t.indices[i]>vertices.size()) {
					throw new ImportErrorException("MS3D: Encountered invalid vertex index, file is malformed");
				}

				// collect vertex components
				m->mVertices[n] = vertices[t.indices[i]].pos;

				m->mNormals[n] = t.normals[i];
				m->mTextureCoords[0][n] = aiVector3D(t.uv[i].x,t.uv[i].y,0.0);
				f.mIndices[i] = n;
			}
		}
	}

	// ... add dummy nodes under a single root, each holding a reference to one
	// mesh. If we didn't do this, we'd loose the group name.
	aiNode* rt = pScene->mRootNode = new aiNode("<MS3DRoot>");
	rt->mChildren = new aiNode*[rt->mNumChildren=pScene->mNumMeshes];

	for (unsigned int i = 0; i < rt->mNumChildren; ++i) {
		aiNode* nd = rt->mChildren[i] = new aiNode();

		const TempGroup& g = groups[i];
		nd->mName = aiString(g.name);
		nd->mParent = rt;

		nd->mMeshes = new unsigned int[nd->mNumMeshes = 1];
		nd->mMeshes[0] = i;
	}
}

#endif
