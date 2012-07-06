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
#include "FBXImporter.h"

namespace Assimp {
namespace FBX {

	using namespace Util;

	// XXX vc9's debugger won't step into anonymous namespaces
//namespace {

/** Dummy class to encapsulate the conversion process */
class Converter
{

public:

	Converter(aiScene* out, const Document& doc)
		: out(out) 
		, doc(doc)
	{
		ConvertRootNode();

		if(doc.Settings().readAllMaterials) {
			// unfortunately this means we have to evaluate all objects
			BOOST_FOREACH(const ObjectMap::value_type& v,doc.Objects()) {

				const Object* ob = v.second->Get();
				if(!ob) {
					continue;
				}

				const Material* mat = dynamic_cast<const Material*>(ob);
				if(mat) {

					if (materials_converted.find(mat) == materials_converted.end()) {
						ConvertMaterial(*mat);
					}
				}
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
		out->mRootNode = new aiNode();
		out->mRootNode->mName.Set("Model::RootNode");

		// root has ID 0
		ConvertNodes(0L, *out->mRootNode);
	}


	// ------------------------------------------------------------------------------------------------
	// collect and assign child nodes
	void ConvertNodes(uint64_t id, aiNode& parent)
	{
		const std::vector<const Connection*>& conns = doc.GetConnectionsByDestinationSequenced(id);

		std::vector<aiNode*> nodes;
		nodes.reserve(conns.size());

		BOOST_FOREACH(const Connection* con, conns) {

			// ignore object-property links
			if(con->PropertyName().length()) {
				continue;
			}

			const Object* const object = con->SourceObject();
			if(!object) {
				FBXImporter::LogWarn("failed to convert source object for node link");
				continue;
			}

			const Model* const model = dynamic_cast<const Model*>(object);

		
			if(model) {
				aiNode* nd = new aiNode();
				nd->mName.Set(model->Name());
				nd->mParent = &parent;

				ConvertTransformation(*model,*nd);

				ConvertModel(*model, *nd);
				ConvertNodes(model->ID(), *nd);
			}
		}

		if(nodes.size()) {
			parent.mChildren = new aiNode*[nodes.size()]();
			parent.mNumChildren = static_cast<unsigned int>(nodes.size());

			std::swap_ranges(nodes.begin(),nodes.end(),parent.mChildren);
		}
	}


	// ------------------------------------------------------------------------------------------------
	void ConvertTransformation(const Model& model, aiNode& nd)
	{
		const PropertyTable& props = model.Props();

		bool ok;
		
		aiVector3D Translation = PropertyGet<aiVector3D>(props,"Lcl Translation",ok);
		if(!ok) {
			Translation = aiVector3D(0.0f,0.0f,0.0f);
		}

		aiVector3D Scaling = PropertyGet<aiVector3D>(props,"Lcl Scaling",ok);
		if(!ok) {
			Scaling = aiVector3D(1.0f,1.0f,1.0f);
		}

		// XXX euler angles, radians, xyz order?
		aiVector3D Rotation = PropertyGet<aiVector3D>(props,"Lcl Rotation",ok);
		if(!ok) {
			Rotation = aiVector3D(0.0f,0.0f,0.0f);
		}

		aiMatrix4x4 temp;
		nd.mTransformation = aiMatrix4x4::Scaling(Scaling,temp);
		if(fabs(Rotation.x) > 1e-6f) {
			nd.mTransformation *= aiMatrix4x4::RotationX(Rotation.x,temp);
		}
		if(fabs(Rotation.y) > 1e-6f) {
			nd.mTransformation *= aiMatrix4x4::RotationY(Rotation.y,temp);
		}
		if(fabs(Rotation.z) > 1e-6f) {
			nd.mTransformation *= aiMatrix4x4::RotationZ(Rotation.z,temp);
		}
		nd.mTransformation.a4 = Translation.x;
		nd.mTransformation.b4 = Translation.y;
		nd.mTransformation.c4 = Translation.z;
	}


	// ------------------------------------------------------------------------------------------------
	void ConvertModel(const Model& model, aiNode& nd)
	{
		const std::vector<const Geometry*>& geos = model.GetGeometry();

		std::vector<unsigned int> meshes;
		meshes.reserve(geos.size());

		BOOST_FOREACH(const Geometry* geo, geos) {

			const MeshGeometry* const mesh = dynamic_cast<const MeshGeometry*>(geo);
			if(mesh) {
				std::vector<unsigned int>& indices = ConvertMesh(*mesh, model);

				// mesh indices are shifted by 1 and 0 entries are failed conversions -
				// XXX maybe log how many conversions went wrong?
				std::remove(indices.begin(),indices.end(),0);
				std::transform(indices.begin(),indices.end(),std::back_inserter(meshes), std::bind2nd(std::minus<unsigned int>(),1) );
			}
			else {
				FBXImporter::LogWarn("ignoring unrecognized geometry: " + geo->Name());
			}
		}

		if(meshes.size()) {
			nd.mMeshes = new unsigned int[meshes.size()]();
			nd.mNumMeshes = static_cast<unsigned int>(meshes.size());

			std::swap_ranges(meshes.begin(),meshes.end(),nd.mMeshes);
		}
	}


	// ------------------------------------------------------------------------------------------------
	// MeshGeometry -> aiMesh, return mesh index + 1 or 0 if the conversion failed
	std::vector<unsigned int> ConvertMesh(const MeshGeometry& mesh, const Model& model)
	{
		std::vector<unsigned int> temp; 

		MeshMap::const_iterator it = meshes_converted.find(&mesh);
		if (it != meshes_converted.end()) {
			temp.push_back((*it).second + 1);
			return temp;
		}

		const std::vector<aiVector3D>& vertices = mesh.GetVertices();
		const std::vector<unsigned int>& faces = mesh.GetFaceIndexCounts();
		if(vertices.empty() || faces.empty()) {
			FBXImporter::LogWarn("ignoring empty geometry: " + mesh.Name());
			return temp;
		}

		// one material per mesh maps easily to aiMesh. Multiple material 
		// meshes need to be split.
		const std::vector<unsigned int>& mindices = mesh.GetMaterialIndices();
		if (!mindices.empty()) {
			const unsigned int base = mindices[0];
			BOOST_FOREACH(unsigned int index, mindices) {
				if(index != base) {
					return ConvertMeshMultiMaterial(mesh, model);
				}
			}
		}

		// faster codepath, just copy the data
		temp.push_back(ConvertMeshSingleMaterial(mesh, model));
		return temp;
	}


	// ------------------------------------------------------------------------------------------------
	unsigned int ConvertMeshSingleMaterial(const MeshGeometry& mesh, const Model& model)	
	{
		aiMesh* const out_mesh = new aiMesh();
		meshes.push_back(out_mesh);
		meshes_converted[&mesh] = static_cast<unsigned int>(meshes.size()-1);

		const std::vector<aiVector3D>& vertices = mesh.GetVertices();
		const std::vector<unsigned int>& faces = mesh.GetFaceIndexCounts();

		// copy vertices
		out_mesh->mNumVertices = static_cast<unsigned int>(vertices.size());
		out_mesh->mVertices = new aiVector3D[vertices.size()];
		std::copy(vertices.begin(),vertices.end(),out_mesh->mVertices);

		// generate dummy faces
		out_mesh->mNumFaces = static_cast<unsigned int>(faces.size());
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
		const std::vector<aiVector3D>& normals = mesh.GetNormals();
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

		const std::vector<unsigned int>& mindices = mesh.GetMaterialIndices();
		if(mindices.empty()) {
			FBXImporter::LogError("no material assigned to mesh, setting default material");
			out_mesh->mMaterialIndex = GetDefaultMaterial();
		}
		else {
			ConvertMaterialForMesh(out_mesh,model,mesh,mindices[0]);
		}

		return static_cast<unsigned int>(meshes.size());
	}


	// ------------------------------------------------------------------------------------------------
	std::vector<unsigned int> ConvertMeshMultiMaterial(const MeshGeometry& mesh, const Model& model)	
	{
		const std::vector<unsigned int>& mindices = mesh.GetMaterialIndices();
		ai_assert(mindices.size());
	
		std::set<unsigned int> had;
		std::vector<unsigned int> indices;

		BOOST_FOREACH(unsigned int index, mindices) {
			if(had.find(index) != had.end()) {

				indices.push_back(ConvertMeshMultiMaterial(mesh, model, index));
				had.insert(index);
			}
		}

		return indices;
	}


	// ------------------------------------------------------------------------------------------------
	unsigned int ConvertMeshMultiMaterial(const MeshGeometry& mesh, const Model& model, unsigned int index)	
	{
		aiMesh* const out_mesh = new aiMesh();
		meshes.push_back(out_mesh);
		meshes_converted[&mesh] = static_cast<unsigned int>(meshes.size()-1);

		const std::vector<unsigned int>& mindices = mesh.GetMaterialIndices();
		ai_assert(mindices.size());

		const std::vector<aiVector3D>& vertices = mesh.GetVertices();
		const std::vector<unsigned int>& faces = mesh.GetFaceIndexCounts();

		unsigned int count_faces = 0;
		unsigned int count_vertices = 0;

		// count faces
		for(std::vector<unsigned int>::const_iterator it = mindices.begin(), 
			end = mindices.end(), itf = faces.begin(); it != end; ++it, ++itf) 
		{	
			if ((*it) != index) {
				continue;
			}
			++count_faces;
			count_vertices += *itf;
		}

		ai_assert(count_faces);


		// allocate output data arrays, but don't fill them yet
		out_mesh->mNumVertices = count_vertices;
		out_mesh->mVertices = new aiVector3D[count_vertices];

		out_mesh->mNumFaces = count_faces;
		aiFace* fac = out_mesh->mFaces = new aiFace[count_faces]();


		// allocate normals
		const std::vector<aiVector3D>& normals = mesh.GetNormals();
		if(normals.size()) {
			ai_assert(normals.size() == vertices.size());
			out_mesh->mNormals = new aiVector3D[vertices.size()];
		}

		// allocate tangents, binormals. 
		const std::vector<aiVector3D>& tangents = mesh.GetTangents();
		const std::vector<aiVector3D>* binormals = &mesh.GetBinormals();

		if(tangents.size()) {
			std::vector<aiVector3D> tempBinormals;
			if (!binormals->size()) {
				if (normals.size()) {
					// XXX this computes the binormals for the entire mesh, not only 
					// the part for which we need them.
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
				out_mesh->mBitangents = new aiVector3D[vertices.size()];
			}
		}

		// allocate texture coords
		unsigned int num_uvs = 0;
		for (unsigned int i = 0; i < AI_MAX_NUMBER_OF_TEXTURECOORDS; ++i, ++num_uvs) {
			const std::vector<aiVector2D>& uvs = mesh.GetTextureCoords(i);
			if(uvs.empty()) {
				break;
			}

			out_mesh->mTextureCoords[i] = new aiVector3D[vertices.size()];
			out_mesh->mNumUVComponents[i] = 2;
		}

		// allocate vertex colors
		unsigned int num_vcs = 0;
		for (unsigned int i = 0; i < AI_MAX_NUMBER_OF_COLOR_SETS; ++i, ++num_vcs) {
			const std::vector<aiColor4D>& colors = mesh.GetVertexColors(i);
			if(colors.empty()) {
				break;
			}

			out_mesh->mColors[i] = new aiColor4D[vertices.size()];
		}

		unsigned int cursor = 0, in_cursor = 0;

		for(std::vector<unsigned int>::const_iterator it = mindices.begin(), 
			end = mindices.end(), itf = faces.begin(); it != end; ++it, ++itf) 
		{	
			const unsigned int pcount = *itf;
			if ((*it) != index) {
				in_cursor += pcount;
				continue;
			}

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
			for (unsigned int i = 0; i < pcount; ++i, ++cursor, ++in_cursor) {
				f.mIndices[i] = cursor;

				out_mesh->mVertices[cursor] = vertices[in_cursor];

				if(out_mesh->mNormals) {
					out_mesh->mNormals[cursor] = normals[in_cursor];
				}

				if(out_mesh->mTangents) {
					out_mesh->mTangents[cursor] = tangents[in_cursor];
					out_mesh->mBitangents[cursor] = (*binormals)[in_cursor];
				}

				for (unsigned int i = 0; i < num_uvs; ++i) {
					const std::vector<aiVector2D>& uvs = mesh.GetTextureCoords(i);
					out_mesh->mTextureCoords[i][cursor] = aiVector3D(uvs[in_cursor].x,uvs[in_cursor].y, 0.0f);
				}

				for (unsigned int i = 0; i < num_vcs; ++i) {
					const std::vector<aiColor4D>& cols = mesh.GetVertexColors(i);
					out_mesh->mColors[i][cursor] = cols[in_cursor];
				}
			}
		}
	
		ConvertMaterialForMesh(out_mesh,model,mesh,index);
		return static_cast<unsigned int>(meshes.size());
	}


	// ------------------------------------------------------------------------------------------------
	void ConvertMaterialForMesh(aiMesh* out, const Model& model, const MeshGeometry& geo, unsigned int materialIndex)
	{
		// locate source materials for this mesh
		const std::vector<const Material*>& mats = model.GetMaterials();
		if (materialIndex >= mats.size()) {
			FBXImporter::LogError("material index out of bounds, setting default material");
			out->mMaterialIndex = GetDefaultMaterial();
			return;
		}

		const Material* const mat = mats[materialIndex];
		MaterialMap::const_iterator it = materials_converted.find(mat);
		if (it != materials_converted.end()) {
			out->mMaterialIndex = (*it).second;
			return;
		}

		out->mMaterialIndex = ConvertMaterial(*mat);	
		materials_converted[mat] = out->mMaterialIndex;
	}


	// ------------------------------------------------------------------------------------------------
	unsigned int GetDefaultMaterial()
	{
		if (defaultMaterialIndex) {
			return defaultMaterialIndex - 1; 
		}

		aiMaterial* out_mat = new aiMaterial();
		materials.push_back(out_mat);

		const aiColor3D diffuse = aiColor3D(0.8f,0.8f,0.8f);
		out_mat->AddProperty(&diffuse,1,AI_MATKEY_COLOR_DIFFUSE);

		aiString s;
		s.Set(AI_DEFAULT_MATERIAL_NAME);

		out_mat->AddProperty(&s,AI_MATKEY_NAME);

		defaultMaterialIndex = static_cast<unsigned int>(materials.size());
		return defaultMaterialIndex - 1;
	}


	// ------------------------------------------------------------------------------------------------
	// Material -> aiMaterial
	unsigned int ConvertMaterial(const Material& material)
	{
		const PropertyTable& props = material.Props();

		// generate empty output material
		aiMaterial* out_mat = new aiMaterial();
		materials_converted[&material] = static_cast<unsigned int>(materials.size());

		materials.push_back(out_mat);

		aiString str;

		// set material name
		str.Set(material.Name());
		out_mat->AddProperty(&str,AI_MATKEY_NAME);

		// shading stuff and colors
		SetShadingPropertiesCommon(out_mat,props);
	
		// texture assignments
		SetTextureProperties(out_mat,material.Textures());

		return static_cast<unsigned int>(materials.size() - 1);
	}


	// ------------------------------------------------------------------------------------------------
	void TrySetTextureProperties(aiMaterial* out_mat, const TextureMap& textures, const std::string& propName, aiTextureType target)
	{
		TextureMap::const_iterator it = textures.find(propName);
		if(it == textures.end()) {
			return;
		}

		const Texture* const tex = (*it).second;
		
		aiString path;
		path.Set(tex->RelativeFilename());

		out_mat->AddProperty(&path,_AI_MATKEY_TEXTURE_BASE,target,0);

		aiUVTransform uvTrafo;
		// XXX handle all kinds of UV transformations
		uvTrafo.mScaling = tex->UVScaling();
		uvTrafo.mTranslation = tex->UVTranslation();
		out_mat->AddProperty(&uvTrafo,1,_AI_MATKEY_UVTRANSFORM_BASE,target,0);

		const PropertyTable& props = tex->Props();

		int uvIndex = 0;

		bool ok;
		const std::string& uvSet = PropertyGet<std::string>(props,"UVSet",ok);
		if(ok) {
			// "default" is the name which usually appears in the FbxFileTexture template
			if(uvSet != "default" && uvSet.length()) {
				// this is a bit awkward - we need to find a mesh that uses this
				// material and scan its UV channels for the given UV name because
				// assimp references UV channels by index, not by name.

				// XXX: the case that UV channels may appear in different orders
				// in meshes is unhandled. A possible solution would be to sort
				// the UV channels alphabetically, but this would have the side
				// effect that the primary (first) UV channel would sometimes
				// be moved, causing trouble when users read only the first
				// UV channel and ignore UV channel assignments altogether.

				const unsigned int matIndex = std::distance(materials.begin(), 
					std::find(materials.begin(),materials.end(),out_mat)
				);

				uvIndex = -1;
				BOOST_FOREACH(const MeshMap::value_type& v,meshes_converted) {
					const MeshGeometry* const mesh = dynamic_cast<const MeshGeometry*> (v.first);
					if(!mesh) {
						continue;
					}

					const std::vector<unsigned int>& mats = mesh->GetMaterialIndices();
					if(std::find(mats.begin(),mats.end(),matIndex) == mats.end()) {
						continue;
					}

					int index = -1;
					for (unsigned int i = 0; i < AI_MAX_NUMBER_OF_TEXTURECOORDS; ++i) {
						if(mesh->GetTextureCoords(i).empty()) {
							break;
						}
						const std::string& name = mesh->GetTextureCoordChannelName(i);
						if(name == uvSet) {
							index = static_cast<int>(i);
							break;
						}
					}
					if(index == -1) {
						FBXImporter::LogWarn("did not find UV channel named " + uvSet + " in a mesh using this material");
						continue;
					}

					if(uvIndex == -1) {
						uvIndex = index;
					}
					else {
						FBXImporter::LogWarn("the UV channel named " + uvSet + 
							" appears at different positions in meshes, results will be wrong");
					}
				}

				if(uvIndex == -1) {
					FBXImporter::LogWarn("failed to resolve UV channel " + uvSet + ", using first UV channel");
					uvIndex = 0;
				}
			}
		}

		out_mat->AddProperty(&uvIndex,1,_AI_MATKEY_UVWSRC_BASE,target,0);
	}


	// ------------------------------------------------------------------------------------------------
	void SetTextureProperties(aiMaterial* out_mat, const TextureMap& textures)
	{
		TrySetTextureProperties(out_mat, textures, "DiffuseColor", aiTextureType_DIFFUSE);
		TrySetTextureProperties(out_mat, textures, "AmbientColor", aiTextureType_AMBIENT);
		TrySetTextureProperties(out_mat, textures, "EmissiveColor", aiTextureType_EMISSIVE);
		TrySetTextureProperties(out_mat, textures, "SpecularColor", aiTextureType_SPECULAR);
		TrySetTextureProperties(out_mat, textures, "TransparentColor", aiTextureType_OPACITY);
		TrySetTextureProperties(out_mat, textures, "ReflectionColor", aiTextureType_REFLECTION);
		TrySetTextureProperties(out_mat, textures, "DisplacementColor", aiTextureType_DISPLACEMENT);
		TrySetTextureProperties(out_mat, textures, "NormalMap", aiTextureType_NORMALS);
		TrySetTextureProperties(out_mat, textures, "Bump", aiTextureType_HEIGHT);
	}



	// ------------------------------------------------------------------------------------------------
	aiColor3D GetColorPropertyFromMaterial(const PropertyTable& props,const std::string& baseName, bool& result)
	{
		result = true;

		bool ok;
		const aiVector3D& Diffuse = PropertyGet<aiVector3D>(props,baseName,ok);
		if(ok) {
			return aiColor3D(Diffuse.x,Diffuse.y,Diffuse.z);
		}
		else {
			aiVector3D DiffuseColor = PropertyGet<aiVector3D>(props,baseName + "Color",ok);
			if(ok) {
				float DiffuseFactor = PropertyGet<float>(props,baseName + "Factor",ok);
				if(ok) {
					DiffuseColor *= DiffuseFactor;
				}

				return aiColor3D(DiffuseColor.x,DiffuseColor.y,DiffuseColor.z);
			}
		}
		result = false;
		return aiColor3D(0.0f,0.0f,0.0f);
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
		const aiColor3D& Diffuse = GetColorPropertyFromMaterial(props,"Diffuse",ok);
		if(ok) {
			out_mat->AddProperty(&Diffuse,1,AI_MATKEY_COLOR_DIFFUSE);
		}

		const aiColor3D& Emissive = GetColorPropertyFromMaterial(props,"Emissive",ok);
		if(ok) {
			out_mat->AddProperty(&Emissive,1,AI_MATKEY_COLOR_EMISSIVE);
		}

		const aiColor3D& Ambient = GetColorPropertyFromMaterial(props,"Ambient",ok);
		if(ok) {
			out_mat->AddProperty(&Ambient,1,AI_MATKEY_COLOR_AMBIENT);
		}

		const aiColor3D& Specular = GetColorPropertyFromMaterial(props,"Specular",ok);
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


		if(materials.size()) {
			out->mMaterials = new aiMaterial*[materials.size()]();
			out->mNumMaterials = static_cast<unsigned int>(materials.size());

			std::swap_ranges(materials.begin(),materials.end(),out->mMaterials);
		}
	}


private:

	// 0: not assigned yet, others: index is value - 1
	unsigned int defaultMaterialIndex;

	std::vector<aiMesh*> meshes;
	std::vector<aiMaterial*> materials;

	typedef std::map<const Material*, unsigned int> MaterialMap;
	MaterialMap materials_converted;

	typedef std::map<const Geometry*, unsigned int> MeshMap;
	MeshMap meshes_converted;

	aiScene* const out;
	const FBX::Document& doc;
};

//} // !anon

// ------------------------------------------------------------------------------------------------
void ConvertToAssimpScene(aiScene* out, const Document& doc)
{
	Converter converter(out,doc);
}

} // !FBX
} // !Assimp

#endif
