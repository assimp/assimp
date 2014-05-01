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

#ifndef ASSIMP_BUILD_NO_OGRE_IMPORTER

#include "OgreImporter.h"
#include "TinyFormatter.h"

using namespace std;

namespace Assimp
{
namespace Ogre
{

void OgreImporter::ReadSubMesh(const unsigned int submeshIndex, SubMesh &submesh, XmlReader *reader)
{
	if (reader->getAttributeValue("material"))
		submesh.MaterialName = GetAttribute<string>(reader, "material");
	if (reader->getAttributeValue("use32bitindexes"))
		submesh.Use32bitIndexes = GetAttribute<bool>(reader, "use32bitindexes");
	if (reader->getAttributeValue("usesharedvertices"))
		submesh.UseSharedGeometry = GetAttribute<bool>(reader, "usesharedvertices");

	DefaultLogger::get()->debug(Formatter::format() << "Reading submesh " << submeshIndex);
	DefaultLogger::get()->debug(Formatter::format() << "  - Material '" << submesh.MaterialName << "'");
	DefaultLogger::get()->debug(Formatter::format() << "  - Shader geometry = " << (submesh.UseSharedGeometry ? "true" : "false") << 
													   ", 32bit indexes = " << (submesh.Use32bitIndexes ? "true" : "false"));

	//TODO: maybe we have alsways just 1 faces and 1 geometry and always in this order. this loop will only work correct, when the order
	//of faces and geometry changed, and not if we have more than one of one
	/// @todo Fix above comment with better read logic below

	NextNode(reader);
	string currentNodeName = reader->getNodeName();

	const string nnFaces           = "faces";
	const string nnFace            = "face";
	const string nnGeometry        = "geometry";
	const string nnBoneAssignments = "boneassignments";
	const string nnVertexBuffer    = "vertexbuffer";

	bool quadWarned = false;

	while(currentNodeName == nnFaces    ||
		  currentNodeName == nnGeometry ||
		  currentNodeName == nnBoneAssignments)
	{
		if (currentNodeName == nnFaces)
		{
			unsigned int numFaces = GetAttribute<unsigned int>(reader, "count");

			NextNode(reader);
			currentNodeName = reader->getNodeName();

			while(currentNodeName == nnFace)
			{
				Face NewFace;
				NewFace.VertexIndices[0] = GetAttribute<int>(reader, "v1");
				NewFace.VertexIndices[1] = GetAttribute<int>(reader, "v2");
				NewFace.VertexIndices[2] = GetAttribute<int>(reader, "v3");

				/// @todo Support quads
				if (!quadWarned && reader->getAttributeValue("v4"))
					DefaultLogger::get()->warn("Submesh has quads, only triangles are supported at the moment!");

				submesh.Faces.push_back(NewFace);

				// Advance
				NextNode(reader);
				currentNodeName = reader->getNodeName();
			}

			if (submesh.Faces.size() == numFaces)
				DefaultLogger::get()->debug(Formatter::format() << "  - Faces " << numFaces);
			else
				throw DeadlyImportError(Formatter::format() << "Read only " << submesh.Faces.size() << " faces when should have read " << numFaces);
		}
		else if (currentNodeName == nnGeometry)
		{	
			unsigned int numVertices = GetAttribute<int>(reader, "vertexcount");

			NextNode(reader);
			while(string(reader->getNodeName()) == nnVertexBuffer)
				ReadVertexBuffer(submesh, reader, numVertices);
		}
		else if (reader->getNodeName() == nnBoneAssignments)
			ReadBoneWeights(submesh, reader);
			
		currentNodeName = reader->getNodeName();
	}
}

void OgreImporter::ReadVertexBuffer(SubMesh &submesh, XmlReader *reader, const unsigned int numVertices)
{
	DefaultLogger::get()->debug(Formatter::format() << "Reading vertex buffer with " << numVertices << " vertices");
	
	submesh.HasGeometry = true;

	if (reader->getAttributeValue("positions") && GetAttribute<bool>(reader, "positions"))
	{
		submesh.HasPositions = true;
		submesh.Positions.reserve(numVertices);
		DefaultLogger::get()->debug("  - Has positions");
	}
	if (reader->getAttributeValue("normals") && GetAttribute<bool>(reader, "normals"))
	{
		submesh.HasNormals = true;
		submesh.Normals.reserve(numVertices);
		DefaultLogger::get()->debug("  - Has normals");
	}
	if (reader->getAttributeValue("tangents") && GetAttribute<bool>(reader, "tangents"))
	{
		submesh.HasTangents = true;
		submesh.Tangents.reserve(numVertices);
		DefaultLogger::get()->debug("  - Has tangents");
	}
	if (reader->getAttributeValue("texture_coords"))
	{
		submesh.Uvs.resize(GetAttribute<unsigned int>(reader, "texture_coords"));
		for(size_t i=0, len=submesh.Uvs.size(); i<len; ++i)
			submesh.Uvs[i].reserve(numVertices);
		DefaultLogger::get()->debug(Formatter::format() << "  - Has " << submesh.Uvs.size() << " texture coords");
	}

	if (!submesh.HasPositions)
		throw DeadlyImportError("Vertex buffer does not contain positions!");

	const string nnVertex        = "vertex";
	const string nnPosition      = "position";
	const string nnNormal        = "normal";
	const string nnTangent       = "tangent";
	const string nnBinormal      = "binormal";
	const string nnTexCoord      = "texcoord";
	const string nnColorDiffuse  = "colour_diffuse";
	const string nnColorSpecular = "colour_specular";
	
	bool warnBinormal = true;
	bool warnColorDiffuse = true;
	bool warnColorSpecular = true;
	
	NextNode(reader);
	string currentNodeName = reader->getNodeName();

	/// @todo Make this loop nicer.
	while(currentNodeName == nnVertex       ||
		  currentNodeName == nnPosition     ||
		  currentNodeName == nnNormal       ||
		  currentNodeName == nnTangent      ||
		  currentNodeName == nnBinormal     ||
		  currentNodeName == nnTexCoord     ||
		  currentNodeName == nnColorDiffuse ||
		  currentNodeName == nnColorSpecular)
	{
		if (currentNodeName == nnVertex)
		{
			NextNode(reader);
			currentNodeName = reader->getNodeName();
		}
		
		/// @todo Implement nnBinormal, nnColorDiffuse and nnColorSpecular

		if (submesh.HasPositions && currentNodeName == nnPosition)
		{
			aiVector3D NewPos;
			NewPos.x = GetAttribute<float>(reader, "x");
			NewPos.y = GetAttribute<float>(reader, "y");
			NewPos.z = GetAttribute<float>(reader, "z");
			submesh.Positions.push_back(NewPos);
		}
		else if (submesh.HasNormals && currentNodeName == nnNormal)
		{
			aiVector3D NewNormal;
			NewNormal.x = GetAttribute<float>(reader, "x");
			NewNormal.y = GetAttribute<float>(reader, "y");
			NewNormal.z = GetAttribute<float>(reader, "z");
			submesh.Normals.push_back(NewNormal);
		}
		else if (submesh.HasTangents && currentNodeName == nnTangent)
		{
			aiVector3D NewTangent;
			NewTangent.x = GetAttribute<float>(reader, "x");
			NewTangent.y = GetAttribute<float>(reader, "y");
			NewTangent.z = GetAttribute<float>(reader, "z");
			submesh.Tangents.push_back(NewTangent);
		}
		else if (submesh.Uvs.size() > 0 && currentNodeName == nnTexCoord)
		{
			for(size_t i=0, len=submesh.Uvs.size(); i<len; ++i)
			{
				if (currentNodeName != nnTexCoord)
					throw DeadlyImportError("Vertex buffer declared more UVs than can be found in a vertex");

				aiVector3D NewUv;
				NewUv.x = GetAttribute<float>(reader, "u");
				NewUv.y = GetAttribute<float>(reader, "v") * (-1)+1; //flip the uv vertikal, blender exports them so! (ahem... @todo ????)
				submesh.Uvs[i].push_back(NewUv);
				
				NextNode(reader);
				currentNodeName = reader->getNodeName();
			}
			// Continue main loop as above already read next node
			continue;
		}
		else
		{
			/// @todo Remove this stuff once implemented. We only want to log warnings once per element.
			bool warn = true;
			if (currentNodeName == nnBinormal)
			{
				if (warnBinormal)
					warnBinormal = false;
				else
					warn = false;
			}
			else if (currentNodeName == nnColorDiffuse)
			{
				if (warnColorDiffuse)
					warnColorDiffuse = false;
				else
					warn = false;
			}
			else if (currentNodeName == nnColorSpecular)
			{
				if (warnColorSpecular)
					warnColorSpecular = false;
				else
					warn = false;
			}
			if (warn)
				DefaultLogger::get()->warn(string("Vertex buffer attribute read not implemented for element: ") + currentNodeName);
		}

		// Advance
		NextNode(reader);
		currentNodeName = reader->getNodeName();
	}

	DefaultLogger::get()->debug(Formatter::format() <<
		"  - Positions " << submesh.Positions.size() <<
		" Normals "   << submesh.Normals.size() <<
		" TexCoords " << submesh.Uvs.size() <<
		" Tangents "  << submesh.Tangents.size());

	// Sanity checks
	if (submesh.HasNormals && submesh.Normals.size() != numVertices)
		throw DeadlyImportError(Formatter::format() << "Read only " << submesh.Normals.size() << " normals when should have read " << numVertices);
	if (submesh.HasTangents && submesh.Tangents.size() != numVertices)
		throw DeadlyImportError(Formatter::format() << "Read only " << submesh.Tangents.size() << " tangents when should have read " << numVertices);
	for(unsigned int i=0; i<submesh.Uvs.size(); ++i)
	{
		if (submesh.Uvs[i].size() != numVertices)
			throw DeadlyImportError(Formatter::format() << "Read only " << submesh.Uvs[i].size() 
				<< " uvs for uv index " << i << " when should have read " << numVertices);
	}
}

void OgreImporter::ReadBoneWeights(SubMesh &submesh, XmlReader *reader)
{
	submesh.Weights.resize(submesh.Positions.size());

	unsigned int numRead = 0;
	const string nnVertexBoneAssignment = "vertexboneassignment";

	NextNode(reader);
	while(CurrentNodeNameEquals(reader, nnVertexBoneAssignment))
	{
		numRead++;

		BoneWeight weight;
		weight.Id = GetAttribute<int>(reader, "boneindex");
		weight.Value = GetAttribute<float>(reader, "weight");
		
		//calculate the number of bones used (this is the highest id +1 becuase bone ids start at 0)
		/// @todo This can probably be refactored to something else.
		submesh.BonesUsed = max(submesh.BonesUsed, weight.Id+1);

		const unsigned int vertexId = GetAttribute<int>(reader, "vertexindex");
		submesh.Weights[vertexId].push_back(weight);
		
		NextNode(reader);
	}
	DefaultLogger::get()->debug(Formatter::format() << "  - Bone weights " << numRead);
}

void OgreImporter::ProcessSubMesh(SubMesh &submesh, SubMesh &sharedGeometry)
{
	//---------------Make all Vertexes unique: (this is required by assimp)-----------------------
	vector<Face> UniqueFaceList(submesh.Faces.size());
	unsigned int UniqueVertexCount=submesh.Faces.size()*3;//*3 because each face consists of 3 vertexes, because we only support triangles^^

	vector<aiVector3D> UniquePositions(UniqueVertexCount);

	vector<aiVector3D> UniqueNormals(UniqueVertexCount);

	vector<aiVector3D> UniqueTangents(UniqueVertexCount);

	vector< vector<BoneWeight> > UniqueWeights(UniqueVertexCount);

	vector< vector<aiVector3D> > UniqueUvs(submesh.Uvs.size());
	for(unsigned int i=0; i<UniqueUvs.size(); ++i)	UniqueUvs[i].resize(UniqueVertexCount);



	//Support for shared data:
	/*We can use this loop to copy vertex informations from the shared data pool. In order to do so
	  we just use a reference to a submodel instead of our submodel itself*/

	SubMesh& VertexSource= submesh.UseSharedGeometry ? sharedGeometry : submesh;
	if(submesh.UseSharedGeometry)//copy vertexinformations to our mesh:
	{
		submesh.HasPositions=sharedGeometry.HasPositions;
		submesh.HasNormals=sharedGeometry.HasNormals;
		submesh.HasTangents=sharedGeometry.HasTangents;

		submesh.BonesUsed=sharedGeometry.BonesUsed;

		UniqueUvs.resize(sharedGeometry.Uvs.size());
		for(unsigned int i=0; i<UniqueUvs.size(); ++i)	UniqueUvs[i].resize(UniqueVertexCount);
	}

	for(unsigned int i=0; i<submesh.Faces.size(); ++i)
	{
		//We precalculate the index vlaues her, because we need them in all vertex attributes
		unsigned int Vertex1=submesh.Faces[i].VertexIndices[0];
		unsigned int Vertex2=submesh.Faces[i].VertexIndices[1];
		unsigned int Vertex3=submesh.Faces[i].VertexIndices[2];

		UniquePositions[3*i+0]=VertexSource.Positions[Vertex1];
		UniquePositions[3*i+1]=VertexSource.Positions[Vertex2];
		UniquePositions[3*i+2]=VertexSource.Positions[Vertex3];

		if(VertexSource.HasNormals)
		{
			UniqueNormals[3*i+0]=VertexSource.Normals[Vertex1];
			UniqueNormals[3*i+1]=VertexSource.Normals[Vertex2];
			UniqueNormals[3*i+2]=VertexSource.Normals[Vertex3];
		}

		if(VertexSource.HasTangents)
		{
			UniqueTangents[3*i+0]=VertexSource.Tangents[Vertex1];
			UniqueTangents[3*i+1]=VertexSource.Tangents[Vertex2];
			UniqueTangents[3*i+2]=VertexSource.Tangents[Vertex3];
		}

		if(UniqueUvs.size()>0)
		{
			for(unsigned int j=0; j<UniqueUvs.size(); ++j)
			{
				UniqueUvs[j][3*i+0]=VertexSource.Uvs[j][Vertex1];
				UniqueUvs[j][3*i+1]=VertexSource.Uvs[j][Vertex2];
				UniqueUvs[j][3*i+2]=VertexSource.Uvs[j][Vertex3];
			}
		}

		if(VertexSource.Weights.size() > 0)
		{
			UniqueWeights[3*i+0]=VertexSource.Weights[Vertex1];
			UniqueWeights[3*i+1]=VertexSource.Weights[Vertex2];
			UniqueWeights[3*i+2]=VertexSource.Weights[Vertex3];
		}

		//The indexvalues a just continuous numbers (0, 1, 2, 3, 4, 5, 6...)
		UniqueFaceList[i].VertexIndices[0]=3*i+0;
		UniqueFaceList[i].VertexIndices[1]=3*i+1;
		UniqueFaceList[i].VertexIndices[2]=3*i+2;
	}
	//_________________________________________________________________________________________

	//now we have the unique datas, but want them in the SubMesh, so we swap all the containers:
	//if we don't have one of them, we just swap empty containers, so everything is ok
	submesh.Faces.swap(UniqueFaceList);
	submesh.Positions.swap(UniquePositions);
	submesh.Normals.swap(UniqueNormals);
	submesh.Tangents.swap(UniqueTangents);
	submesh.Uvs.swap(UniqueUvs);
	submesh.Weights.swap(UniqueWeights);



	//------------- normalize weights -----------------------------
	//The Blender exporter doesn't care about whether the sum of all boneweights for a single vertex equals 1 or not,
	//so we have to make this sure:
	for(unsigned int VertexId=0; VertexId<submesh.Weights.size(); ++VertexId)//iterate over all vertices
	{
		float WeightSum=0.0f;
		for(unsigned int BoneId=0; BoneId<submesh.Weights[VertexId].size(); ++BoneId)//iterate over all bones
		{
			WeightSum+=submesh.Weights[VertexId][BoneId].Value;
		}
		
		//check if the sum is too far away from 1
		if(WeightSum<1.0f-0.05f || WeightSum>1.0f+0.05f)
		{
			//normalize all weights:
			for(unsigned int BoneId=0; BoneId<submesh.Weights[VertexId].size(); ++BoneId)//iterate over all bones
			{
				submesh.Weights[VertexId][BoneId].Value/=WeightSum;
			}
		}
	}
	//_________________________________________________________
}

aiMesh *OgreImporter::CreateAssimpSubMesh(aiScene *pScene, const SubMesh& submesh, const vector<Bone>& bones) const
{
	const size_t sizeVector3D = sizeof(aiVector3D);

	aiMesh *dest = new aiMesh();

	// Material
	dest->mMaterialIndex = submesh.MaterialIndex;

	// Positions
	dest->mVertices = new aiVector3D[submesh.Positions.size()];
	dest->mNumVertices = submesh.Positions.size();
	memcpy(dest->mVertices, &submesh.Positions[0], submesh.Positions.size() * sizeVector3D);
	
	// Normals
	if (submesh.HasNormals)
	{
		dest->mNormals = new aiVector3D[submesh.Normals.size()];
		memcpy(dest->mNormals, &submesh.Normals[0], submesh.Normals.size() * sizeVector3D);
	}
	
	// Tangents
	// Until we have support for bitangents, no tangents will be written
	/// @todo Investigate why the above?
	if (submesh.HasTangents)
	{
		DefaultLogger::get()->warn("Tangents found from Ogre mesh but writing to Assimp mesh not yet supported!");
		//dest->mTangents = new aiVector3D[submesh.Tangents.size()];
		//memcpy(dest->mTangents, &submesh.Tangents[0], submesh.Tangents.size() * sizeVector3D);
	}

	// UVs
	for (size_t i=0, len=submesh.Uvs.size(); i<len; ++i)
	{
		dest->mNumUVComponents[i] = 2;
		dest->mTextureCoords[i] = new aiVector3D[submesh.Uvs[i].size()];
		memcpy(dest->mTextureCoords[i], &(submesh.Uvs[i][0]), submesh.Uvs[i].size() * sizeVector3D);
	}

	// Bone weights. Convert internal vertex-to-bone mapping to bone-to-vertex.
	vector<vector<aiVertexWeight> > assimpWeights(submesh.BonesUsed);
	for(size_t vertexId=0, len=submesh.Weights.size(); vertexId<len; ++vertexId)
	{
		const vector<BoneWeight> &vertexWeights = submesh.Weights[vertexId];
		for (size_t boneId=0, len=vertexWeights.size(); boneId<len; ++boneId)
		{
			const BoneWeight &ogreWeight = vertexWeights[boneId];
			assimpWeights[ogreWeight.Id].push_back(aiVertexWeight(vertexId, ogreWeight.Value));
		}
	}

	// Bones.
	vector<aiBone*> assimpBones;
	assimpBones.reserve(submesh.BonesUsed);

	for(size_t boneId=0, len=submesh.BonesUsed; boneId<len; ++boneId)
	{
		const vector<aiVertexWeight> &boneWeights = assimpWeights[boneId];
		if (boneWeights.size() == 0)
			continue;

		// @note The bones list is sorted by id's, this was done in LoadSkeleton.
		aiBone *assimpBone = new aiBone();
		assimpBone->mName = bones[boneId].Name;
		assimpBone->mOffsetMatrix = bones[boneId].BoneToWorldSpace;
		assimpBone->mNumWeights = boneWeights.size();
		assimpBone->mWeights = new aiVertexWeight[boneWeights.size()];
		memcpy(assimpBone->mWeights, &boneWeights[0], boneWeights.size() * sizeof(aiVertexWeight));

		assimpBones.push_back(assimpBone);
	}

	if (!assimpBones.empty())
	{
		dest->mBones = new aiBone*[assimpBones.size()];
		dest->mNumBones = assimpBones.size();

		for(size_t i=0, len=assimpBones.size(); i<len; ++i)
			dest->mBones[i] = assimpBones[i];
	}

	// Faces
	dest->mFaces = new aiFace[submesh.Faces.size()];
	dest->mNumFaces = submesh.Faces.size();

	for(size_t i=0, len=submesh.Faces.size(); i<len; ++i)
	{
		dest->mFaces[i].mNumIndices = 3;
		dest->mFaces[i].mIndices = new unsigned int[3];

		const Face &f = submesh.Faces[i];
		dest->mFaces[i].mIndices[0] = f.VertexIndices[0];
		dest->mFaces[i].mIndices[1] = f.VertexIndices[1];
		dest->mFaces[i].mIndices[2] = f.VertexIndices[2];
	}

	return dest;
}

} // Ogre
} // Assimp

#endif // ASSIMP_BUILD_NO_OGRE_IMPORTER
