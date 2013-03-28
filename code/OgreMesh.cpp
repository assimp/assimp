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

#include "OgreImporter.hpp"
#include "TinyFormatter.h"

using namespace std;

namespace Assimp
{
namespace Ogre
{


void OgreImporter::ReadSubMesh(SubMesh &theSubMesh, XmlReader *Reader)
{
	if(Reader->getAttributeValue("usesharedvertices"))
		theSubMesh.SharedData=GetAttribute<bool>(Reader, "usesharedvertices");

	XmlRead(Reader);
	//TODO: maybe we have alsways just 1 faces and 1 geometry and always in this order. this loop will only work correct, when the order
	//of faces and geometry changed, and not if we have more than one of one
	while(	Reader->getNodeName()==string("faces")
		||	Reader->getNodeName()==string("geometry")
		||	Reader->getNodeName()==string("boneassignments"))
	{
		if(string(Reader->getNodeName())=="faces")//Read the face list
		{
			//some info logging:
			unsigned int NumFaces=GetAttribute<int>(Reader, "count");
			ostringstream ss; ss <<"Submesh has " << NumFaces << " Faces.";
			DefaultLogger::get()->debug(ss.str());

			while(XmlRead(Reader) && Reader->getNodeName()==string("face"))
			{
				Face NewFace;
				NewFace.VertexIndices[0]=GetAttribute<int>(Reader, "v1");
				NewFace.VertexIndices[1]=GetAttribute<int>(Reader, "v2");
				NewFace.VertexIndices[2]=GetAttribute<int>(Reader, "v3");
				if(Reader->getAttributeValue("v4"))//this should be supported in the future
				{
					DefaultLogger::get()->warn("Submesh has quads, only traingles are supported!");
					//throw DeadlyImportError("Submesh has quads, only traingles are supported!");
				}
				theSubMesh.FaceList.push_back(NewFace);
			}

		}//end of faces
		else if(string(Reader->getNodeName())=="geometry")//Read the vertexdata
		{	
			//some info logging:
			unsigned int NumVertices=GetAttribute<int>(Reader, "vertexcount");
			ostringstream ss; ss<<"VertexCount: " << NumVertices;
			DefaultLogger::get()->debug(ss.str());
			
			//General Informations about vertices
			XmlRead(Reader);
			while(Reader->getNodeName()==string("vertexbuffer"))
			{
				ReadVertexBuffer(theSubMesh, Reader, NumVertices);
			}

			//some error checking on the loaded data
			if(!theSubMesh.HasPositions)
				throw DeadlyImportError("No positions could be loaded!");

			if(theSubMesh.HasNormals && theSubMesh.Normals.size() != NumVertices)
				throw DeadlyImportError("Wrong Number of Normals loaded!");

			if(theSubMesh.HasTangents && theSubMesh.Tangents.size() != NumVertices)
				throw DeadlyImportError("Wrong Number of Tangents loaded!");

			for(unsigned int i=0; i<theSubMesh.Uvs.size(); ++i)
			{
				if(theSubMesh.Uvs[i].size() != NumVertices)
					throw DeadlyImportError("Wrong Number of Uvs loaded!");
			}

		}//end of "geometry


		else if(Reader->getNodeName()==string("boneassignments"))
		{
			ReadBoneWeights(theSubMesh, Reader);
		}
	}
	DefaultLogger::get()->debug((Formatter::format(),
		"Positionen: ",theSubMesh.Positions.size(),
		" Normale: ",theSubMesh.Normals.size(),
		" TexCoords: ",theSubMesh.Uvs.size(),
		" Tantents: ",theSubMesh.Tangents.size()
	));
}


void OgreImporter::ReadVertexBuffer(SubMesh &theSubMesh, XmlReader *Reader, unsigned int NumVertices)
{
	DefaultLogger::get()->debug("new Vertex Buffer");

	bool ReadPositions=false;
	bool ReadNormals=false;
	bool ReadTangents=false;
	unsigned int NumUvs=0;

	//-------------------- check, what we need to read: --------------------------------
	if(Reader->getAttributeValue("positions") && GetAttribute<bool>(Reader, "positions"))
	{
		ReadPositions=theSubMesh.HasPositions=true;
		theSubMesh.Positions.reserve(NumVertices);
		DefaultLogger::get()->debug("reading positions");
	}
	if(Reader->getAttributeValue("normals") && GetAttribute<bool>(Reader, "normals"))
	{
		ReadNormals=theSubMesh.HasNormals=true;
		theSubMesh.Normals.reserve(NumVertices);
		DefaultLogger::get()->debug("reading normals");
	}
	if(Reader->getAttributeValue("tangents") && GetAttribute<bool>(Reader, "tangents"))
	{
		ReadTangents=theSubMesh.HasTangents=true;
		theSubMesh.Tangents.reserve(NumVertices);
		DefaultLogger::get()->debug("reading tangents");
	}

	if(Reader->getAttributeValue("texture_coords"))
	{
		NumUvs=GetAttribute<unsigned int>(Reader, "texture_coords");
		theSubMesh.Uvs.resize(NumUvs);
		for(unsigned int i=0; i<theSubMesh.Uvs.size(); ++i) theSubMesh.Uvs[i].reserve(NumVertices);
		DefaultLogger::get()->debug("reading texture coords");
	}
	//___________________________________________________________________


	//check if we will load anything
	if(!( ReadPositions || ReadNormals || ReadTangents || (NumUvs>0) ))
		DefaultLogger::get()->warn("vertexbuffer seams to be empty!");
	

	//read all the vertices:
	XmlRead(Reader);

	/*it might happen, that we have more than one attribute per vertex (they are not splitted to different buffers)
	so the break condition is a bit tricky */
	while(Reader->getNodeName()==string("vertex")
		||Reader->getNodeName()==string("position")
		||Reader->getNodeName()==string("normal")
		||Reader->getNodeName()==string("tangent")
		||Reader->getNodeName()==string("texcoord")
		||Reader->getNodeName()==string("colour_diffuse"))
	{
		if(Reader->getNodeName()==string("vertex"))
			XmlRead(Reader);//Read an attribute tag

		//Position
		if(ReadPositions && Reader->getNodeName()==string("position"))
		{
			aiVector3D NewPos;
			NewPos.x=GetAttribute<float>(Reader, "x");
			NewPos.y=GetAttribute<float>(Reader, "y");
			NewPos.z=GetAttribute<float>(Reader, "z");
			theSubMesh.Positions.push_back(NewPos);
		}
				
		//Normal
		else if(ReadNormals && Reader->getNodeName()==string("normal"))
		{
			aiVector3D NewNormal;
			NewNormal.x=GetAttribute<float>(Reader, "x");
			NewNormal.y=GetAttribute<float>(Reader, "y");
			NewNormal.z=GetAttribute<float>(Reader, "z");
			theSubMesh.Normals.push_back(NewNormal);
		}
				
		//Tangent
		else if(ReadTangents && Reader->getNodeName()==string("tangent"))
		{
			aiVector3D NewTangent;
			NewTangent.x=GetAttribute<float>(Reader, "x");
			NewTangent.y=GetAttribute<float>(Reader, "y");
			NewTangent.z=GetAttribute<float>(Reader, "z");
			theSubMesh.Tangents.push_back(NewTangent);
		}

		//Uv:
		else if(NumUvs>0 && Reader->getNodeName()==string("texcoord"))
		{
			for(unsigned int i=0; i<NumUvs; ++i)
			{
				if(Reader->getNodeName()!=string("texcoord"))
				{
					DefaultLogger::get()->warn(string("Not enough UVs in Vertex: ")+Reader->getNodeName());
				}
				aiVector3D NewUv;
				NewUv.x=GetAttribute<float>(Reader, "u");
				NewUv.y=GetAttribute<float>(Reader, "v")*(-1)+1;//flip the uv vertikal, blender exports them so!
				theSubMesh.Uvs[i].push_back(NewUv);
				XmlRead(Reader);
			}
			continue;//because we already read the next node...
		}

		//Color:
		//TODO: actually save this data!
		else if(Reader->getNodeName()==string("colour_diffuse"))
		{
			//do nothing, because we not yet support them
		}

		//Attribute could not be read
		else
		{
			DefaultLogger::get()->warn(string("Attribute was not read: ")+Reader->getNodeName());
		}

		XmlRead(Reader);//Read the Vertex tag
	}
}


void OgreImporter::ReadBoneWeights(SubMesh &theSubMesh, XmlReader *Reader)
{
	theSubMesh.Weights.resize(theSubMesh.Positions.size());
	while(XmlRead(Reader) && Reader->getNodeName()==string("vertexboneassignment"))
	{
		Weight NewWeight;
		unsigned int VertexId=GetAttribute<int>(Reader, "vertexindex");
		NewWeight.BoneId=GetAttribute<int>(Reader, "boneindex");
		NewWeight.Value=GetAttribute<float>(Reader, "weight");
		//calculate the number of bones used (this is the highest id +1 becuase bone ids start at 0)
		theSubMesh.BonesUsed=max(theSubMesh.BonesUsed, NewWeight.BoneId+1);

		theSubMesh.Weights[VertexId].push_back(NewWeight);
	}
}



void OgreImporter::ProcessSubMesh(SubMesh &theSubMesh, SubMesh &theSharedGeometry)
{
	//---------------Make all Vertexes unique: (this is required by assimp)-----------------------
	vector<Face> UniqueFaceList(theSubMesh.FaceList.size());
	unsigned int UniqueVertexCount=theSubMesh.FaceList.size()*3;//*3 because each face consists of 3 vertexes, because we only support triangles^^

	vector<aiVector3D> UniquePositions(UniqueVertexCount);

	vector<aiVector3D> UniqueNormals(UniqueVertexCount);

	vector<aiVector3D> UniqueTangents(UniqueVertexCount);

	vector< vector<Weight> > UniqueWeights(UniqueVertexCount);

	vector< vector<aiVector3D> > UniqueUvs(theSubMesh.Uvs.size());
	for(unsigned int i=0; i<UniqueUvs.size(); ++i)	UniqueUvs[i].resize(UniqueVertexCount);



	//Support for shared data:
	/*We can use this loop to copy vertex informations from the shared data pool. In order to do so
	  we just use a reference to a submodel instead of our submodel itself*/

	SubMesh& VertexSource= theSubMesh.SharedData ? theSharedGeometry : theSubMesh;
	if(theSubMesh.SharedData)//copy vertexinformations to our mesh:
	{
		theSubMesh.HasPositions=theSharedGeometry.HasPositions;
		theSubMesh.HasNormals=theSharedGeometry.HasNormals;
		theSubMesh.HasTangents=theSharedGeometry.HasTangents;

		theSubMesh.BonesUsed=theSharedGeometry.BonesUsed;

		UniqueUvs.resize(theSharedGeometry.Uvs.size());
		for(unsigned int i=0; i<UniqueUvs.size(); ++i)	UniqueUvs[i].resize(UniqueVertexCount);
	}

	for(unsigned int i=0; i<theSubMesh.FaceList.size(); ++i)
	{
		//We precalculate the index vlaues her, because we need them in all vertex attributes
		unsigned int Vertex1=theSubMesh.FaceList[i].VertexIndices[0];
		unsigned int Vertex2=theSubMesh.FaceList[i].VertexIndices[1];
		unsigned int Vertex3=theSubMesh.FaceList[i].VertexIndices[2];

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
	theSubMesh.FaceList.swap(UniqueFaceList);
	theSubMesh.Positions.swap(UniquePositions);
	theSubMesh.Normals.swap(UniqueNormals);
	theSubMesh.Tangents.swap(UniqueTangents);
	theSubMesh.Uvs.swap(UniqueUvs);
	theSubMesh.Weights.swap(UniqueWeights);



	//------------- normalize weights -----------------------------
	//The Blender exporter doesn't care about whether the sum of all boneweights for a single vertex equals 1 or not,
	//so we have to make this sure:
	for(unsigned int VertexId=0; VertexId<theSubMesh.Weights.size(); ++VertexId)//iterate over all vertices
	{
		float WeightSum=0.0f;
		for(unsigned int BoneId=0; BoneId<theSubMesh.Weights[VertexId].size(); ++BoneId)//iterate over all bones
		{
			WeightSum+=theSubMesh.Weights[VertexId][BoneId].Value;
		}
		
		//check if the sum is too far away from 1
		if(WeightSum<1.0f-0.05f || WeightSum>1.0f+0.05f)
		{
			//normalize all weights:
			for(unsigned int BoneId=0; BoneId<theSubMesh.Weights[VertexId].size(); ++BoneId)//iterate over all bones
			{
				theSubMesh.Weights[VertexId][BoneId].Value/=WeightSum;
			}
		}
	}
	//_________________________________________________________
}




aiMesh* OgreImporter::CreateAssimpSubMesh(const SubMesh& theSubMesh, const vector<Bone>& Bones) const
{
	const aiScene* const m_CurrentScene=this->m_CurrentScene;//make sure, that we can access but not change the scene
	(void)m_CurrentScene;

	aiMesh* NewAiMesh=new aiMesh();
		
	//Positions
	NewAiMesh->mVertices=new aiVector3D[theSubMesh.Positions.size()];
	memcpy(NewAiMesh->mVertices, &theSubMesh.Positions[0], theSubMesh.Positions.size()*sizeof(aiVector3D));
	NewAiMesh->mNumVertices=theSubMesh.Positions.size();

	//Normals
	if(theSubMesh.HasNormals)
	{
		NewAiMesh->mNormals=new aiVector3D[theSubMesh.Normals.size()];
		memcpy(NewAiMesh->mNormals, &theSubMesh.Normals[0], theSubMesh.Normals.size()*sizeof(aiVector3D));
	}


	//until we have support for bitangents, no tangents will be written
	/*
	//Tangents
	if(theSubMesh.HasTangents)
	{
		NewAiMesh->mTangents=new aiVector3D[theSubMesh.Tangents.size()];
		memcpy(NewAiMesh->mTangents, &theSubMesh.Tangents[0], theSubMesh.Tangents.size()*sizeof(aiVector3D));
	}
	*/

	//Uvs
	if(theSubMesh.Uvs.size()>0)
	{
		for(unsigned int i=0; i<theSubMesh.Uvs.size(); ++i)
		{
			NewAiMesh->mNumUVComponents[i]=2;
			NewAiMesh->mTextureCoords[i]=new aiVector3D[theSubMesh.Uvs[i].size()];
			memcpy(NewAiMesh->mTextureCoords[i], &(theSubMesh.Uvs[i][0]), theSubMesh.Uvs[i].size()*sizeof(aiVector3D));
		}
	}


	//---------------------------------------- Bones --------------------------------------------

	//Copy the weights in in Bone-Vertices Struktur
	//(we have them in a Vertex-Bones Structur, this is much easier for making them unique, which is required by assimp
	vector< vector<aiVertexWeight> > aiWeights(theSubMesh.BonesUsed);//now the outer list are the bones, and the inner vector the vertices
	for(unsigned int VertexId=0; VertexId<theSubMesh.Weights.size(); ++VertexId)//iterate over all vertices
	{
		for(unsigned int BoneId=0; BoneId<theSubMesh.Weights[VertexId].size(); ++BoneId)//iterate over all bones
		{
			aiVertexWeight NewWeight;
			NewWeight.mVertexId=VertexId;//the current Vertex, we can't use the Id form the submehs weights, because they are bone id's
			NewWeight.mWeight=theSubMesh.Weights[VertexId][BoneId].Value;
			aiWeights[theSubMesh.Weights[VertexId][BoneId].BoneId].push_back(NewWeight);
		}
	}

	

	vector<aiBone*> aiBones;
	aiBones.reserve(theSubMesh.BonesUsed);//the vector might be smaller, because there might be empty bones (bones that are not attached to any vertex)
	
	//create all the bones and fill them with informations
	for(unsigned int i=0; i<theSubMesh.BonesUsed; ++i)
	{
		if(aiWeights[i].size()>0)
		{
			aiBone* NewBone=new aiBone();
			NewBone->mNumWeights=aiWeights[i].size();
			NewBone->mWeights=new aiVertexWeight[aiWeights[i].size()];
			memcpy(NewBone->mWeights, &(aiWeights[i][0]), sizeof(aiVertexWeight)*aiWeights[i].size());
			NewBone->mName=Bones[i].Name;//The bone list should be sorted after its id's, this was done in LoadSkeleton
			NewBone->mOffsetMatrix=Bones[i].BoneToWorldSpace;
				
			aiBones.push_back(NewBone);
		}
	}
	NewAiMesh->mNumBones=aiBones.size();
	
	// mBones must be NULL if mNumBones is non 0 or the validation fails.
	if (aiBones.size()) {
		NewAiMesh->mBones=new aiBone* [aiBones.size()];
		memcpy(NewAiMesh->mBones, &(aiBones[0]), aiBones.size()*sizeof(aiBone*));
	}

	//______________________________________________________________________________________________________



	//Faces
	NewAiMesh->mFaces=new aiFace[theSubMesh.FaceList.size()];
	for(unsigned int i=0; i<theSubMesh.FaceList.size(); ++i)
	{
		NewAiMesh->mFaces[i].mNumIndices=3;
		NewAiMesh->mFaces[i].mIndices=new unsigned int[3];

		NewAiMesh->mFaces[i].mIndices[0]=theSubMesh.FaceList[i].VertexIndices[0];
		NewAiMesh->mFaces[i].mIndices[1]=theSubMesh.FaceList[i].VertexIndices[1];
		NewAiMesh->mFaces[i].mIndices[2]=theSubMesh.FaceList[i].VertexIndices[2];
	}
	NewAiMesh->mNumFaces=theSubMesh.FaceList.size();

	//Link the material:
	NewAiMesh->mMaterialIndex=theSubMesh.MaterialIndex;//the index is set by the function who called ReadSubMesh

	return NewAiMesh;
}


}//namespace Ogre
}//namespace Assimp

#endif  // !! ASSIMP_BUILD_NO_OGRE_IMPORTER
