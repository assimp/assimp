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

/** @file  FBXConverter.cpp
 *  @brief Implementation of the FBX DOM -> aiScene converter
 */
#include "AssimpPCH.h"

#ifndef ASSIMP_BUILD_NO_FBX_IMPORTER

#include "FBXParser.h"
#include "FBXConverter.h"
#include "FBXDocument.h"
#include "FBXUtil.h"
#include "FBXProperties.h"

namespace Assimp {
namespace FBX {

	using namespace Util;

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
		std::for_each(materials.begin(),materials.end(),Util::delete_fun<aiMaterial>());
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
	// Material -> aiMaterial
	void ConvertMaterial(const Material& material)
	{
		const PropertyTable& props = material.Props();

		// generate empty output material
		aiMaterial* out_mat = new aiMaterial();
		materials.push_back(out_mat);

		aiString str;

		// set material name
		str.Set(material.Name());
		out_mat->AddProperty(&str,AI_MATKEY_NAME);

		SetShadingPropertiesCommon(out_mat,props);
	}

	// ------------------------------------------------------------------------------------------------
	void SetShadingPropertiesCommon(aiMaterial* out_mat, const PropertyTable& props)
	{
		// set shading properties. There are various, redundant ways in which FBX materials
		// specify their shading settings (depending on shading models, prop
		// template etc.). No idea which one is right in a particular context. 
		// Just try to make sense of it - there's no spec to verify this against, 
		// so why should we.
		bool ok;
		const aiVector3D& Diffuse = PropertyGet<aiVector3D>(props,"Diffuse",ok);
		if(ok) {
			out_mat->AddProperty(&Diffuse,1,AI_MATKEY_COLOR_DIFFUSE);
		}
		else {
			aiVector3D DiffuseColor = PropertyGet<aiVector3D>(props,"DiffuseColor",ok);
			if(ok) {
				float DiffuseFactor = PropertyGet<float>(props,"DiffuseFactor",ok);
				if(ok) {
					DiffuseColor *= DiffuseFactor;
				}

				out_mat->AddProperty(&DiffuseColor,1,AI_MATKEY_COLOR_DIFFUSE);
			}
		}

		const aiVector3D& Emissive = PropertyGet<aiVector3D>(props,"Emissive",ok);
		if(ok) {
			out_mat->AddProperty(&Emissive,1,AI_MATKEY_COLOR_EMISSIVE);
		}

		const aiVector3D& Ambient = PropertyGet<aiVector3D>(props,"Ambient",ok);
		if(ok) {
			out_mat->AddProperty(&Ambient,1,AI_MATKEY_COLOR_AMBIENT);
		}

		const aiVector3D& Specular = PropertyGet<aiVector3D>(props,"Specular",ok);
		if(ok) {
			out_mat->AddProperty(&Specular,1,AI_MATKEY_COLOR_SPECULAR);
		}

		const float Opacity = PropertyGet<float>(props,"Opacity",ok);
		if(ok) {
			out_mat->AddProperty(&Opacity,1,AI_MATKEY_OPACITY);
		}

		const float Reflectivity = PropertyGet<float>(props,"Reflectivity",ok);
		if(ok) {
			out_mat->AddProperty(&Reflectivity,1,AI_MATKEY_REFLECTIVITY);
		}

		const float Shininess = PropertyGet<float>(props,"Shininess",ok);
		if(ok) {
			out_mat->AddProperty(&Shininess,1,AI_MATKEY_SHININESS_STRENGTH);
		}

		const float ShininessExponent = PropertyGet<float>(props,"ShininessExponent",ok);
		if(ok) {
			out_mat->AddProperty(&ShininessExponent,1,AI_MATKEY_SHININESS);
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


		out->mMaterials = new aiMaterial*[meshes.size()]();
		out->mNumMaterials = static_cast<unsigned int>(materials.size());

		std::swap_ranges(materials.begin(),materials.end(),out->mMaterials);
	}


private:

	std::vector<aiMesh*> meshes;
	std::vector<aiMaterial*> materials;

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
