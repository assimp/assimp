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
		//ConvertRootNode();

		// hack to process all meshes
		BOOST_FOREACH(const ObjectMap::value_type& v,doc.Objects()) {

			const Object* ob = v.second->Get();
			if(!ob) {
				continue;
			}
			const MeshGeometry* geo = dynamic_cast<const MeshGeometry*>(ob);
			if(geo) {
				ConvertMesh(*geo);
			}
		}

		// dummy root node
		out->mRootNode = new aiNode();
		out->mRootNode->mNumMeshes = static_cast<unsigned int>(meshes.size());
		out->mRootNode->mMeshes = new unsigned int[meshes.size()];
		for(unsigned int i = 0; i < out->mRootNode->mNumMeshes; ++i) {
			out->mRootNode->mMeshes[i] = i;
		}

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
		const std::vector<aiVector3D>& vertices = mesh.GetVertices();
		const std::vector<unsigned int>& faces = mesh.GetFaceIndexCounts();
		if(vertices.empty() || faces.empty()) {
			return;
		}

		aiMesh* out_mesh = new aiMesh();
		meshes.push_back(out_mesh);

		// copy vertices
		out_mesh->mNumVertices = static_cast<size_t>(vertices.size());
		out_mesh->mVertices = new aiVector3D[vertices.size()];
		std::copy(vertices.begin(),vertices.end(),out_mesh->mVertices);

		// generate dummy faces
		out_mesh->mNumFaces = static_cast<size_t>(faces.size());
		aiFace* fac = out_mesh->mFaces = new aiFace[faces.size()]();

		unsigned int cursor = 0;
		BOOST_FOREACH(unsigned int pcount, faces) {
			aiFace& f = *fac++;
			f.mNumIndices = pcount;
			f.mIndices = new unsigned int[pcount];
			switch(pcount) 
			{
				case 1:
					out_mesh->mPrimitiveTypes |= aiPrimitiveType_POINT;
					break;
				case 2:
					out_mesh->mPrimitiveTypes |= aiPrimitiveType_LINE;
					break;
				case 3:
					out_mesh->mPrimitiveTypes |= aiPrimitiveType_TRIANGLE;
					break;
				default:
					out_mesh->mPrimitiveTypes |= aiPrimitiveType_POLYGON;
					break;
			}
			for (unsigned int i = 0; i < pcount; ++i) {
				f.mIndices[i] = cursor++;
			}
		}

		// copy normals
		const std::vector<aiVector3D>& normals = mesh.GetVertices();
		if(normals.size()) {
			ai_assert(normals.size() == vertices.size());

			out_mesh->mNormals = new aiVector3D[vertices.size()];
			std::copy(normals.begin(),normals.end(),out_mesh->mNormals);
		}

		// copy tangents - assimp requires both tangents and bitangents (binormals)
		// to be present, or neither of them. Compute binormals from normals
		// and tangents if needed.
		const std::vector<aiVector3D>& tangents = mesh.GetTangents();
		const std::vector<aiVector3D>* binormals = &mesh.GetBinormals();

		if(tangents.size()) {
			std::vector<aiVector3D> tempBinormals;
			if (!binormals->size()) {
				if (normals.size()) {
					tempBinormals.resize(normals.size());
					for (unsigned int i = 0; i < tangents.size(); ++i) {
						tempBinormals[i] = normals[i] ^ tangents[i];
					}

					binormals = &tempBinormals;
				}
				else {
					binormals = NULL;	
				}
			}

			if(binormals) {
				ai_assert(tangents.size() == vertices.size() && binormals->size() == vertices.size());

				out_mesh->mTangents = new aiVector3D[vertices.size()];
				std::copy(tangents.begin(),tangents.end(),out_mesh->mTangents);

				out_mesh->mBitangents = new aiVector3D[vertices.size()];
				std::copy(binormals->begin(),binormals->end(),out_mesh->mBitangents);
			}
		}

		// copy texture coords
		for (unsigned int i = 0; i < AI_MAX_NUMBER_OF_TEXTURECOORDS; ++i) {
			const std::vector<aiVector2D>& uvs = mesh.GetTextureCoords(i);
			if(uvs.empty()) {
				break;
			}

			aiVector3D* out_uv = out_mesh->mTextureCoords[i] = new aiVector3D[vertices.size()];
			BOOST_FOREACH(const aiVector2D& v, uvs) {
				*out_uv++ = aiVector3D(v.x,v.y,0.0f);
			}

			out_mesh->mNumUVComponents[i] = 2;
		}

		// copy vertex colors
		for (unsigned int i = 0; i < AI_MAX_NUMBER_OF_COLOR_SETS; ++i) {
			const std::vector<aiColor4D>& colors = mesh.GetVertexColors(i);
			if(colors.empty()) {
				break;
			}

			out_mesh->mColors[i] = new aiColor4D[vertices.size()];
			std::copy(colors.begin(),colors.end(),out_mesh->mColors[i]);
		}
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
