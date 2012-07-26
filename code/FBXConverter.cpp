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

#include <boost/tuple/tuple.hpp>

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
		ConvertAnimations();

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

		TransferDataToScene();
	}


	~Converter()
	{
		std::for_each(meshes.begin(),meshes.end(),Util::delete_fun<aiMesh>());
		std::for_each(materials.begin(),materials.end(),Util::delete_fun<aiMaterial>());
		std::for_each(animations.begin(),animations.end(),Util::delete_fun<aiAnimation>());
	}


private:

	// ------------------------------------------------------------------------------------------------
	// find scene root and trigger recursive scene conversion
	void ConvertRootNode() 
	{
		out->mRootNode = new aiNode();
		out->mRootNode->mName.Set("RootNode");

		// root has ID 0
		ConvertNodes(0L, *out->mRootNode);
	}


	// ------------------------------------------------------------------------------------------------
	// collect and assign child nodes
	void ConvertNodes(uint64_t id, aiNode& parent)
	{
		const std::vector<const Connection*>& conns = doc.GetConnectionsByDestinationSequenced(id, "Model");

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
				nodes.push_back(nd);

				nd->mName.Set(FixNodeName(model->Name()));
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
				const std::vector<unsigned int>& indices = ConvertMesh(*mesh, model);
				std::copy(indices.begin(),indices.end(),std::back_inserter(meshes) );
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
			std::copy((*it).second.begin(),(*it).second.end(),std::back_inserter(temp));
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
		if (doc.Settings().readMaterials && !mindices.empty()) {
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
	aiMesh* SetupEmptyMesh(const MeshGeometry& mesh, unsigned int material_index)
	{
		aiMesh* const out_mesh = new aiMesh();
		meshes.push_back(out_mesh);
		meshes_converted[&mesh].push_back(static_cast<unsigned int>(meshes.size()-1));

		// set name
		std::string name = mesh.Name();
		if (name.substr(0,10) == "Geometry::") {
			name = name.substr(10);
		}

		if(name.length()) {
			out_mesh->mName.Set(name);
		}

		return out_mesh;
	}


	// ------------------------------------------------------------------------------------------------
	unsigned int ConvertMeshSingleMaterial(const MeshGeometry& mesh, const Model& model)	
	{
		const std::vector<unsigned int>& mindices = mesh.GetMaterialIndices();
		aiMesh* const out_mesh = SetupEmptyMesh(mesh,mindices.size() ? mindices[0] : static_cast<unsigned int>(-1)); 

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

		if(!doc.Settings().readMaterials || mindices.empty()) {
			FBXImporter::LogError("no material assigned to mesh, setting default material");
			out_mesh->mMaterialIndex = GetDefaultMaterial();
		}
		else {
			ConvertMaterialForMesh(out_mesh,model,mesh,mindices[0]);
		}

		if(doc.Settings().readWeights && mesh.DeformerSkin() != NULL) {
			ConvertWeights(out_mesh, model, mesh, std::numeric_limits<unsigned int>::max());
		}

		return static_cast<unsigned int>(meshes.size() - 1);
	}


	// ------------------------------------------------------------------------------------------------
	std::vector<unsigned int> ConvertMeshMultiMaterial(const MeshGeometry& mesh, const Model& model)	
	{
		const std::vector<unsigned int>& mindices = mesh.GetMaterialIndices();
		ai_assert(mindices.size());
	
		std::set<unsigned int> had;
		std::vector<unsigned int> indices;

		BOOST_FOREACH(unsigned int index, mindices) {
			if(had.find(index) == had.end()) {

				indices.push_back(ConvertMeshMultiMaterial(mesh, model, index));
				had.insert(index);
			}
		}

		return indices;
	}


	// ------------------------------------------------------------------------------------------------
	unsigned int ConvertMeshMultiMaterial(const MeshGeometry& mesh, const Model& model, unsigned int index)	
	{
		aiMesh* const out_mesh = SetupEmptyMesh(mesh, index);

		const std::vector<unsigned int>& mindices = mesh.GetMaterialIndices();
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
		return static_cast<unsigned int>(meshes.size() - 1);
	}


	// ------------------------------------------------------------------------------------------------
	void ConvertWeights(aiMesh* out, const Model& model, const MeshGeometry& geo, unsigned int materialIndex)
	{
		ai_assert(geo.DeformerSkin());

		std::vector<size_t> out_indices;
		std::vector<size_t> index_out_indices;
		std::vector<size_t> count_out_indices;

		const Skin& sk = *geo.DeformerSkin();

		std::vector<aiBone*> bones;
		bones.reserve(sk.Clusters().size());

		const bool no_mat_check = materialIndex == std::numeric_limits<unsigned int>::max();

		try {

			BOOST_FOREACH(const Cluster* cluster, sk.Clusters()) {
				ai_assert(cluster);

				const WeightIndexArray& indices = cluster->GetIndices();
				const WeightArray& weights = cluster->GetWeights();

				if(indices.empty()) {
					continue;
				}

				const MatIndexArray& mats = geo.GetMaterialIndices();

				bool ok = false;		

				const size_t no_index_sentinel = std::numeric_limits<size_t>::max();

				count_out_indices.clear();
				index_out_indices.clear();
				out_indices.clear();

				// now check if *any* of these weights is contained in the output mesh,
				// taking notes so we don't need to do it twice.
				BOOST_FOREACH(WeightIndexArray::value_type index, indices) {

					unsigned int count;
					const unsigned int* const out_idx = geo.ToOutputVertexIndex(index, count);

					index_out_indices.push_back(no_index_sentinel);

					for(unsigned int i = 0; i < count; ++i) {
						const unsigned int out_face_idx = geo.FaceForVertexIndex(out_idx[i]);
						ai_assert(out_face_idx <= mats.size());

						if (no_mat_check || mats[out_face_idx] == materialIndex) {
							

							if (index_out_indices.back() == no_index_sentinel) {
								index_out_indices.back() = out_indices.size();
							}

							out_indices.push_back(out_idx[i]);

							++count_out_indices.back();
							ok = true;
						}
					}		
				}

				// if we found at least one, generate the output bones
				// XXX this could be heavily simplified by collecting the bone
				// data in a single step.
				if (ok) {
					ConvertCluster(bones, *cluster, out_indices, index_out_indices, count_out_indices);
				}
			}
		}
		catch (std::exception&) {
			std::for_each(bones.begin(),bones.end(),Util::delete_fun<aiBone>());
			throw;
		}

		if(bones.empty()) {
			return;
		}

		out->mBones = new aiBone*[bones.size()]();
		out->mNumBones = static_cast<unsigned int>(bones.size());

		std::swap_ranges(bones.begin(),bones.end(),out->mBones);
	}


	// ------------------------------------------------------------------------------------------------
	void ConvertCluster(std::vector<aiBone*>& bones, const Cluster& cl, 		
		std::vector<size_t>& out_indices,
		std::vector<size_t>& index_out_indices,
		std::vector<size_t>& count_out_indices
		)
	{
		aiBone* const bone = new aiBone();
		bones.push_back(bone);

		bone->mName = FixNodeName(cl.TargetNode()->Name());

		bone->mNumWeights = static_cast<unsigned int>(out_indices.size());
		aiVertexWeight* cursor = bone->mWeights = new aiVertexWeight[out_indices.size()];

		const size_t no_index_sentinel = std::numeric_limits<size_t>::max();
		const WeightArray& weights = cl.GetWeights();

		const size_t c = index_out_indices.size();
		for (size_t i = 0; i < c; ++i) {
			const size_t index_index =  index_out_indices[i];

			if (index_index == no_index_sentinel) {
				continue;
			}

			const size_t cc = count_out_indices[i];
			for (size_t j = 0; j < cc; ++j) {
				aiVertexWeight& out_weight = *cursor++;

				out_weight.mVertexId = static_cast<unsigned int>(out_indices[index_index + j]);
				out_weight.mWeight = weights[i];
			}			
		}
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

		// stip Material:: prefix
		std::string name = material.Name();
		if(name.substr(0,10) == "Material::") {
			name = name.substr(10);
		}

		// set material name if not empty - this could happen
		// and there should be no key for it in this case.
		if(name.length()) {
			str.Set(name);
			out_mat->AddProperty(&str,AI_MATKEY_NAME);
		}

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

				const unsigned int matIndex = static_cast<unsigned int>(std::distance(materials.begin(), 
					std::find(materials.begin(),materials.end(),out_mat)
				));

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
	// convert animation data to aiAnimation et al
	void ConvertAnimations() 
	{
		const std::vector<const AnimationStack*>& animations = doc.AnimationStacks();
		BOOST_FOREACH(const AnimationStack* stack, animations) {
			ConvertAnimationStack(*stack);
		}
	}


	// name -> prefix_stripped?
	typedef std::map<std::string, bool> NodeNameMap;
	NodeNameMap node_names;

	// ------------------------------------------------------------------------------------------------
	std::string FixNodeName(const std::string& name)
	{
		// strip Model:: prefix, avoiding ambiguities (i.e. don't strip if 
		// this causes ambiguities, well possible between empty identifiers,
		// such as "Model::" and ""). Make sure the behaviour is consistent
		// across multiple calls to FixNodeName().
		if(name.substr(0,7) == "Model::") {
			std::string temp = name.substr(7);

			const NodeNameMap::const_iterator it = node_names.find(temp);
			if (it != node_names.end()) {
				if (!(*it).second) {
					return FixNodeName(name + "_");
				}
			}
			node_names[temp] = true;
			return temp;
		}

		const NodeNameMap::const_iterator it = node_names.find(name);
		if (it != node_names.end()) {
			if ((*it).second) {
				return FixNodeName(name + "_");
			}
		}
		node_names[name] = false;
		return name;
	}


	typedef std::map<const AnimationCurveNode*, const AnimationLayer*> LayerMap;


	// ------------------------------------------------------------------------------------------------
	void ConvertAnimationStack(const AnimationStack& st)
	{				
		const AnimationLayerList& layers = st.Layers();
		if(layers.empty()) {
			return;
		}

		aiAnimation* const anim = new aiAnimation();
		animations.push_back(anim);

		// strip AnimationStack:: prefix
		std::string name = st.Name();
		if(name.substr(0,16) == "AnimationStack::") {
			name = name.substr(16);
		}

		anim->mName.Set(name);
		
		// need to find all nodes for which we need to generate node animations -
		// it may happen that we need to merge multiple layers, though.
		// XXX: better use multi_map ..
		typedef std::map<std::string, std::vector<const AnimationCurveNode*> > NodeMap;
		NodeMap node_map;

		// reverse mapping from curves to layers, much faster than querying 
		// the FBX DOM for it.
		LayerMap layer_map;
		
		BOOST_FOREACH(const AnimationLayer* layer, layers) {
			ai_assert(layer);

			const AnimationCurveNodeList& nodes = layer->Nodes();
			BOOST_FOREACH(const AnimationCurveNode* node, nodes) {
				ai_assert(node);

				const Model* const model = dynamic_cast<const Model*>(node->Target());
				// this can happen - it could also be a NodeAttribute (i.e. for camera animations)
				if(!model) {
					continue;
				}

				const std::string& name = FixNodeName(model->Name());
				node_map[name].push_back(node);

				layer_map[node] = layer;
			}
		}

		// generate node animations
		std::vector<aiNodeAnim*> node_anims;

		double min_time = 1e10;
		double max_time = -1e10;

		try {

			NodeMap node_property_map;
			BOOST_FOREACH(const NodeMap::value_type& kv, node_map) {
				node_property_map.clear();

				ai_assert(kv.second.size());

				const AnimationCurveNode* curve_node;
				BOOST_FOREACH(const AnimationCurveNode* node, kv.second) {
					ai_assert(node);

					if (node->TargetProperty().empty()) {
						FBXImporter::LogWarn("target property for animation curve not set");
						continue;
					}

					curve_node = node;
					if (node->Curves().empty()) {
						FBXImporter::LogWarn("no animation curves assigned to AnimationCurveNode");
						continue;
					}

					node_property_map[node->TargetProperty()].push_back(node);
				}

				ai_assert(curve_node);

				const NodeMap::const_iterator itScale = node_property_map.find("Lcl Scaling");
				const NodeMap::const_iterator itRotation = node_property_map.find("Lcl Rotation");
				const NodeMap::const_iterator itTranslation = node_property_map.find("Lcl Translation");

				const bool hasScale = itScale != node_property_map.end();
				const bool hasRotation = itRotation != node_property_map.end();
				const bool hasTranslation = itTranslation != node_property_map.end();

				if (!hasScale && !hasRotation && !hasTranslation) {
					FBXImporter::LogWarn("ignoring node animation, did not find transformation key frames");
					continue;
				}

				aiNodeAnim* const na = new aiNodeAnim();
				node_anims.push_back(na);

				na->mNodeName.Set(kv.first);

				ai_assert(curve_node->TargetAsModel());
				const PropertyTable& props = curve_node->TargetAsModel()->Props();

				// if a particular transformation is not given, grab it from
				// the corresponding node to meet the semantics of aiNodeAnim,
				// which requires all of rotation, scaling and translation
				// to be set.
				if(hasScale) {
					ConvertScaleKeys(na, (*itScale).second, layer_map, 
						max_time, min_time);
				}
				else {
					na->mScalingKeys = new aiVectorKey[1];
					na->mNumScalingKeys = 1;

					na->mScalingKeys[0].mTime = 0.;
					na->mScalingKeys[0].mValue = PropertyGet(props,"Lcl Scaling",aiVector3D(1.f,1.f,1.f));
				}

				if(hasRotation) {
					ConvertRotationKeys(na, (*itRotation).second, layer_map, 
						max_time, min_time);
				}
				else {
					na->mRotationKeys = new aiQuatKey[1];
					na->mNumRotationKeys = 1;

					na->mRotationKeys[0].mTime = 0.;
					na->mRotationKeys[0].mValue = EulerToQuaternion(
						PropertyGet(props,"Lcl Rotation",aiVector3D(0.f,0.f,0.f))
					);
				}

				if(hasTranslation) {
					ConvertTranslationKeys(na, (*itTranslation).second, layer_map, 
						max_time, min_time);
				}
				else {
					na->mPositionKeys = new aiVectorKey[1];
					na->mNumPositionKeys = 1;

					na->mPositionKeys[0].mTime = 0.;
					na->mPositionKeys[0].mValue = PropertyGet(props,"Lcl Translation",aiVector3D(0.f,0.f,0.f));
				}
			}
		}
		catch(std::exception&) {
			std::for_each(node_anims.begin(), node_anims.end(), Util::delete_fun<aiNodeAnim>());
			throw;
		}

		if(node_anims.size()) {
			anim->mChannels = new aiNodeAnim*[node_anims.size()]();
			anim->mNumChannels = static_cast<unsigned int>(node_anims.size());

			std::swap_ranges(node_anims.begin(),node_anims.end(),anim->mChannels);
		}
		else {
			// empty animations would fail validation, so drop them
			delete anim;
			animations.pop_back();
			FBXImporter::LogInfo("ignoring empty AnimationStack: " + name);
			return;
		}

		// for some mysterious reason, mDuration is simply the maximum key -- the
		// validator always assumes animations to start at zero.
		anim->mDuration = max_time /*- min_time */;
		anim->mTicksPerSecond = 1000.0;
	}

	// key (time), value, mapto (component index)
	typedef boost::tuple< const KeyTimeList*, const KeyValueList*, unsigned int > KeyFrameList;
	typedef std::vector<KeyFrameList> KeyFrameListList;

	

	// ------------------------------------------------------------------------------------------------
	KeyFrameListList GetKeyframeList(const std::vector<const AnimationCurveNode*>& nodes)
	{
		KeyFrameListList inputs;
		inputs.reserve(nodes.size()*3);

		BOOST_FOREACH(const AnimationCurveNode* node, nodes) {
			ai_assert(node);

			const AnimationCurveMap& curves = node->Curves();
			BOOST_FOREACH(const AnimationCurveMap::value_type& kv, curves) {

				unsigned int mapto;
				if (kv.first == "d|X") {
					mapto = 0;
				}
				else if (kv.first == "d|Y") {
					mapto = 1;
				}
				else if (kv.first == "d|Z") {
					mapto = 2;
				}
				else {
					FBXImporter::LogWarn("ignoring scale animation curve, did not recognize target component");
					continue;
				}

				const AnimationCurve* const curve = kv.second;
				ai_assert(curve->GetKeys().size() == curve->GetValues().size() && curve->GetKeys().size());

				inputs.push_back(boost::make_tuple(&curve->GetKeys(), &curve->GetValues(), mapto));
			}
		}
		return inputs; // pray for NRVO :-)
	}


	// ------------------------------------------------------------------------------------------------
	KeyTimeList GetKeyTimeList(const KeyFrameListList& inputs)
	{
		ai_assert(inputs.size());

		// reserve some space upfront - it is likely that the keyframe lists
		// have matching time values, so max(of all keyframe lists) should 
		// be a good estimate.
		KeyTimeList keys;
		
		size_t estimate = 0;
		BOOST_FOREACH(const KeyFrameList& kfl, inputs) {
			estimate = std::max(estimate, kfl.get<0>()->size());
		}

		keys.reserve(estimate);

		std::vector<unsigned int> next_pos;
		next_pos.resize(inputs.size(),0);

		const size_t count = inputs.size();
		while(true) {

			uint64_t min_tick = std::numeric_limits<uint64_t>::max();
			for (size_t i = 0; i < count; ++i) {
				const KeyFrameList& kfl = inputs[i];

				if (kfl.get<0>()->size() > next_pos[i] && kfl.get<0>()->at(next_pos[i]) < min_tick) {
					min_tick = kfl.get<0>()->at(next_pos[i]);
				}
			}

			if (min_tick == std::numeric_limits<uint64_t>::max()) {
				break;
			}
			keys.push_back(min_tick);

			for (size_t i = 0; i < count; ++i) {
				const KeyFrameList& kfl = inputs[i];


				while(kfl.get<0>()->size() > next_pos[i] && kfl.get<0>()->at(next_pos[i]) == min_tick) {
					++next_pos[i];
				}
			}
		}	

		return keys;
	}


	// ------------------------------------------------------------------------------------------------
	void InterpolateKeys(aiVectorKey* valOut,const KeyTimeList& keys, const KeyFrameListList& inputs, const bool geom, 
		double& maxTime,
		double& minTime)

	{
		ai_assert(keys.size());
		ai_assert(valOut);

		std::vector<unsigned int> next_pos;
		const size_t count = inputs.size();

		next_pos.resize(inputs.size(),0);

		BOOST_FOREACH(KeyTimeList::value_type time, keys) {
			float result[3] = {0.0f, 0.0f, 0.0f};
			if(geom) {
				result[0] = result[1] = result[2] = 1.0f;
			}

			for (size_t i = 0; i < count; ++i) {
				const KeyFrameList& kfl = inputs[i];

				const size_t ksize = kfl.get<0>()->size();
				if (ksize > next_pos[i] && kfl.get<0>()->at(next_pos[i]) == time) {
					++next_pos[i]; 
				}

				const size_t id0 = next_pos[i]>0 ? next_pos[i]-1 : 0;
				const size_t id1 = next_pos[i]==ksize ? ksize-1 : next_pos[i];

				// use lerp for interpolation
				const KeyValueList::value_type valueA = kfl.get<1>()->at(id0);
				const KeyValueList::value_type valueB = kfl.get<1>()->at(id1);

				const KeyTimeList::value_type timeA = kfl.get<0>()->at(id0);
				const KeyTimeList::value_type timeB = kfl.get<0>()->at(id1);

				// do the actual interpolation in double-precision arithmetics
				// because it is a bit sensitive to rounding errors.
				const double factor = timeB == timeA ? 0. : static_cast<double>((time - timeA) / (timeB - timeA));
				const float interpValue = static_cast<float>(valueA + (valueB - valueA) * factor);

				if(geom) {
					result[kfl.get<2>()] *= interpValue;
				}
				else {
					result[kfl.get<2>()] += interpValue;
				}
			}

			// magic value to convert fbx times to milliseconds
			valOut->mTime = static_cast<double>(time) / 46186158;

			minTime = std::min(minTime, valOut->mTime);
			maxTime = std::max(maxTime, valOut->mTime);

			valOut->mValue.x = result[0];
			valOut->mValue.y = result[1];
			valOut->mValue.z = result[2];
			
			++valOut;
		}
	}


	// ------------------------------------------------------------------------------------------------
	void InterpolateKeys(aiQuatKey* valOut,const KeyTimeList& keys, const KeyFrameListList& inputs, const bool geom,
		double& maxTime,
		double& minTime)
	{
		ai_assert(keys.size());
		ai_assert(valOut);

		boost::scoped_array<aiVectorKey> temp(new aiVectorKey[keys.size()]);
		InterpolateKeys(temp.get(),keys,inputs,geom,maxTime, minTime);

		for (size_t i = 0, c = keys.size(); i < c; ++i) {

			valOut[i].mTime = temp[i].mTime;
			valOut[i].mValue = EulerToQuaternion(temp[i].mValue); 
		}
	}


	// ------------------------------------------------------------------------------------------------
	// euler xyz -> quat
	aiQuaternion EulerToQuaternion(const aiVector3D& rot) 
	{
		aiMatrix4x4 m, mtemp;
		if(fabs(rot.x) > 1e-6f) {
			m *= aiMatrix4x4::RotationX(rot.x,mtemp);
		}
		if(fabs(rot.y) > 1e-6f) {
			m *= aiMatrix4x4::RotationY(rot.y,mtemp);
		}
		if(fabs(rot.z) > 1e-6f) {
			m *= aiMatrix4x4::RotationZ(rot.z,mtemp);
		}

		return aiQuaternion(aiMatrix3x3(m));
	}


	// ------------------------------------------------------------------------------------------------
	void ConvertScaleKeys(aiNodeAnim* na, const std::vector<const AnimationCurveNode*>& nodes, const LayerMap& layers,
		double& maxTime,
		double& minTime)
	{
		ai_assert(nodes.size());

		// XXX for now, assume scale should be blended geometrically (i.e. two
		// layers should be multiplied with each other). There is a FBX 
		// property in the layer to specify the behaviour, though.

		const KeyFrameListList& inputs = GetKeyframeList(nodes);
		const KeyTimeList& keys = GetKeyTimeList(inputs);

		na->mNumScalingKeys = static_cast<unsigned int>(keys.size());
		na->mScalingKeys = new aiVectorKey[keys.size()];
		InterpolateKeys(na->mScalingKeys, keys, inputs, true, maxTime, minTime);
	}


	// ------------------------------------------------------------------------------------------------
	void ConvertTranslationKeys(aiNodeAnim* na, const std::vector<const AnimationCurveNode*>& nodes, const LayerMap& layers,
		double& maxTime,
		double& minTime)
	{
		ai_assert(nodes.size());

		// XXX see notes in ConvertScaleKeys()
		const KeyFrameListList& inputs = GetKeyframeList(nodes);
		const KeyTimeList& keys = GetKeyTimeList(inputs);

		na->mNumPositionKeys = static_cast<unsigned int>(keys.size());
		na->mPositionKeys = new aiVectorKey[keys.size()];
		InterpolateKeys(na->mPositionKeys, keys, inputs, false, maxTime, minTime);
	}


	// ------------------------------------------------------------------------------------------------
	void ConvertRotationKeys(aiNodeAnim* na, const std::vector<const AnimationCurveNode*>& nodes, const LayerMap& layers, 
		double& maxTime,
		double& minTime)
	{
		ai_assert(nodes.size());

		// XXX see notes in ConvertScaleKeys()
		const std::vector< KeyFrameList >& inputs = GetKeyframeList(nodes);
		const KeyTimeList& keys = GetKeyTimeList(inputs);

		na->mNumRotationKeys = static_cast<unsigned int>(keys.size());
		na->mRotationKeys = new aiQuatKey[keys.size()];
		InterpolateKeys(na->mRotationKeys, keys, inputs, false, maxTime, minTime);
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

		if(animations.size()) {
			out->mAnimations = new aiAnimation*[animations.size()]();
			out->mNumAnimations = static_cast<unsigned int>(animations.size());

			std::swap_ranges(animations.begin(),animations.end(),out->mAnimations);
		}
	}


private:

	// 0: not assigned yet, others: index is value - 1
	unsigned int defaultMaterialIndex;

	std::vector<aiMesh*> meshes;
	std::vector<aiMaterial*> materials;
	std::vector<aiAnimation*> animations;

	typedef std::map<const Material*, unsigned int> MaterialMap;
	MaterialMap materials_converted;


	typedef std::map<const Geometry*, std::vector<unsigned int> > MeshMap;
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
