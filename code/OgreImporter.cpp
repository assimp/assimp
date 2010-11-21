/*
Open Asset Import Library (ASSIMP)
----------------------------------------------------------------------

Copyright (c) 2006-2010, ASSIMP Development Team
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

----------------------------------------------------------------------
*/

/** @file  OgreImporter.cpp
 *  @brief Implementation of the Ogre XML (.mesh.xml) loader.
 */
#include "AssimpPCH.h"
#ifndef ASSIMP_BUILD_NO_OGRE_IMPORTER

#include <vector>
#include <sstream>
using namespace std;

//#include "boost/format.hpp"
//#include "boost/foreach.hpp"
//using namespace boost;

#include "TinyFormatter.h"

#include "OgreImporter.h"
#include "irrXMLWrapper.h"


namespace Assimp
{
namespace Ogre
{


bool OgreImporter::CanRead(const std::string &pFile, Assimp::IOSystem *pIOHandler, bool checkSig) const
{
	if(!checkSig)//Check File Extension
	{
		std::string extension("mesh.xml");
		int l=extension.length();
		return pFile.substr(pFile.length()-l, l)==extension;
	}
	else//Check file Header
	{
		const char* tokens[] = {"<mesh>"};
		return BaseImporter::SearchFileHeaderForToken(pIOHandler, pFile, tokens, 1);
	}
}



void OgreImporter::InternReadFile(const std::string &pFile, aiScene *pScene, Assimp::IOSystem *pIOHandler)
{
	m_CurrentFilename=pFile;
	m_CurrentIOHandler=pIOHandler;
	m_CurrentScene=pScene;

	//Open the File:
	boost::scoped_ptr<IOStream> file(pIOHandler->Open(pFile));
	if( file.get() == NULL)
		throw DeadlyImportError("Failed to open file "+pFile+".");

	//Read the Mesh File:
	boost::scoped_ptr<CIrrXML_IOStreamReader> mIOWrapper( new CIrrXML_IOStreamReader( file.get()));
	XmlReader* MeshFile = irr::io::createIrrXMLReader(mIOWrapper.get());
	if(!MeshFile)//parse the xml file
		throw DeadlyImportError("Failed to create XML Reader for "+pFile);


	DefaultLogger::get()->debug("Mesh File opened");
	
	//Read root Node:
	if(!(XmlRead(MeshFile) && string(MeshFile->getNodeName())=="mesh"))
	{
		throw DeadlyImportError("Root Node is not <mesh>! "+pFile+"  "+MeshFile->getNodeName());
	}

	//Go to the submeshs:
	if(!(XmlRead(MeshFile) && string(MeshFile->getNodeName())=="submeshes"))
	{
		throw DeadlyImportError("No <submeshes> node in <mesh> node! "+pFile);
	}


	//-------------------Read the submeshs and materials:-----------------------
	std::list<boost::shared_ptr<SubMesh> > SubMeshes;
	vector<aiMaterial*> Materials;
	XmlRead(MeshFile);
	while(MeshFile->getNodeName()==string("submesh"))
	{
		SubMesh* theSubMesh=new SubMesh();
		theSubMesh->MaterialName=GetAttribute<string>(MeshFile, "material");
		DefaultLogger::get()->debug("Loading Submehs with Material: "+theSubMesh->MaterialName);
		ReadSubMesh(*theSubMesh, MeshFile);

		//just a index in a array, we add a mesh in each loop cycle, so we get indicies like 0, 1, 2 ... n;
		//so it is important to do this before pushing the mesh in the vector!
		theSubMesh->MaterialIndex=SubMeshes.size();

		SubMeshes.push_back(boost::shared_ptr<SubMesh>(theSubMesh));

		//Load the Material:
		aiMaterial* MeshMat=LoadMaterial(theSubMesh->MaterialName);
		
		//Set the Material:
		Materials.push_back(MeshMat);
	}

	if(SubMeshes.empty())
		throw DeadlyImportError("no submesh loaded!");
	if(SubMeshes.size()!=Materials.size())
		throw DeadlyImportError("materialcount doesn't match mesh count!");

	//____________________________________________________________


	//----------------Load the skeleton: -------------------------------
	vector<Bone> Bones;
	vector<Animation> Animations;
	if(MeshFile->getNodeName()==string("skeletonlink"))
	{
		string SkeletonFile=GetAttribute<string>(MeshFile, "name");
		LoadSkeleton(SkeletonFile, Bones, Animations);
	}
	else
	{
		DefaultLogger::get()->warn("No skeleton file will be loaded");
		DefaultLogger::get()->warn(MeshFile->getNodeName());
	}
	//__________________________________________________________________

	
	//----------------- Now fill the Assimp scene ---------------------------
	
	//put the aiMaterials in the scene:
	m_CurrentScene->mMaterials=new aiMaterial*[Materials.size()];
	m_CurrentScene->mNumMaterials=Materials.size();
	for(unsigned int i=0; i<Materials.size(); ++i)
		m_CurrentScene->mMaterials[i]=Materials[i];

	//create the aiMehs... 
	vector<aiMesh*> aiMeshes;
	BOOST_FOREACH(boost::shared_ptr<SubMesh> theSubMesh, SubMeshes)
	{
		aiMeshes.push_back(CreateAssimpSubMesh(*theSubMesh, Bones));
	}
	//... and put them in the scene:
	m_CurrentScene->mNumMeshes=aiMeshes.size();
	m_CurrentScene->mMeshes=new aiMesh*[aiMeshes.size()];
	memcpy(m_CurrentScene->mMeshes, &(aiMeshes[0]), sizeof(aiMeshes[0])*aiMeshes.size());

	//Create the root node
	m_CurrentScene->mRootNode=new aiNode("root");

	//link the meshs with the root node:
	m_CurrentScene->mRootNode->mMeshes=new unsigned int[SubMeshes.size()];
	m_CurrentScene->mRootNode->mNumMeshes=SubMeshes.size();
	for(unsigned int i=0; i<SubMeshes.size(); ++i)
		m_CurrentScene->mRootNode->mMeshes[i]=i;

	

	CreateAssimpSkeleton(Bones, Animations);
	PutAnimationsInScene(Bones, Animations);
	//___________________________________________________________
}



void OgreImporter::GetExtensionList(std::set<std::string>& extensions)
{
	extensions.insert("mesh.xml");
}


void OgreImporter::SetupProperties(const Importer* pImp)
{
	m_MaterialLibFilename=pImp->GetPropertyString(AI_CONFIG_IMPORT_OGRE_MATERIAL_FILE, "Scene.material");
}

void OgreImporter::ReadSubMesh(SubMesh &theSubMesh, XmlReader *Reader)
{
	XmlRead(Reader);
	//TODO: maybe we have alsways just 1 faces and 1 geometry and always in this order. this loop will only work correct, when the order
	//of faces and geometry changed, and not if we have more than one of one
	while(Reader->getNodeName()==string("faces") || string(Reader->getNodeName())=="geometry" || Reader->getNodeName()==string("boneassignments"))
	{
		if(string(Reader->getNodeName())=="faces")//Read the face list
		{
			//some info logging:
			unsigned int NumFaces=GetAttribute<int>(Reader, "count");
			stringstream ss; ss <<"Submesh has " << NumFaces << " Faces.";
			DefaultLogger::get()->debug(ss.str());

			while(XmlRead(Reader) && Reader->getNodeName()==string("face"))
			{
				Face NewFace;
				NewFace.VertexIndices[0]=GetAttribute<int>(Reader, "v1");
				NewFace.VertexIndices[1]=GetAttribute<int>(Reader, "v2");
				NewFace.VertexIndices[2]=GetAttribute<int>(Reader, "v3");
				if(Reader->getAttributeValue("v4"))//this should be supported in the future
				{
					throw DeadlyImportError("Submesh has quads, only traingles are supported!");
				}
				theSubMesh.FaceList.push_back(NewFace);
			}

		}//end of faces
		else if(string(Reader->getNodeName())=="geometry")//Read the vertexdata
		{	
			//some info logging:
			unsigned int NumVertices=GetAttribute<int>(Reader, "vertexcount");
			stringstream ss; ss<<"VertexCount: "<<NumVertices;
			DefaultLogger::get()->debug(ss.str());
			
			//General Informations about vertices
			XmlRead(Reader);
			if(!(Reader->getNodeName()==string("vertexbuffer")))
			{
				throw DeadlyImportError("vertexbuffer node is not first in geometry node!");
			}
			theSubMesh.HasPositions=GetAttribute<bool>(Reader, "positions");
			theSubMesh.HasNormals=GetAttribute<bool>(Reader, "normals");
			if(!Reader->getAttributeValue("texture_coords"))//we can have 1 or 0 uv channels, and if the mesh has no uvs, it also doesn't have the attribute
				theSubMesh.NumUvs=0;
			else
				theSubMesh.NumUvs=GetAttribute<int>(Reader, "texture_coords");
			if(theSubMesh.NumUvs>1)
				throw DeadlyImportError("too many texcoords (just 1 supported!)");

			//read all the vertices:
			XmlRead(Reader);
			while(Reader->getNodeName()==string("vertex"))
			{
				//read all vertex attributes:

				//Position
				if(theSubMesh.HasPositions)
				{
					XmlRead(Reader);
					aiVector3D NewPos;
					NewPos.x=GetAttribute<float>(Reader, "x");
					NewPos.y=GetAttribute<float>(Reader, "y");
					NewPos.z=GetAttribute<float>(Reader, "z");
					theSubMesh.Positions.push_back(NewPos);
				}
				
				//Normal
				if(theSubMesh.HasNormals)
				{
					XmlRead(Reader);
					aiVector3D NewNormal;
					NewNormal.x=GetAttribute<float>(Reader, "x");
					NewNormal.y=GetAttribute<float>(Reader, "y");
					NewNormal.z=GetAttribute<float>(Reader, "z");
					theSubMesh.Normals.push_back(NewNormal);
				}

				//Uv:
				if(1==theSubMesh.NumUvs)
				{
					XmlRead(Reader);
					aiVector3D NewUv;
					NewUv.x=GetAttribute<float>(Reader, "u");
					NewUv.y=GetAttribute<float>(Reader, "v")*(-1)+1;//flip the uv vertikal, blender exports them so!
					theSubMesh.Uvs.push_back(NewUv);
				}
				XmlRead(Reader);
			}

		}//end of "geometry


		else if(string(Reader->getNodeName())=="boneassignments")
		{
			theSubMesh.Weights.resize(theSubMesh.Positions.size());
			while(XmlRead(Reader) && Reader->getNodeName()==string("vertexboneassignment"))
			{
				Weight NewWeight;
				unsigned int VertexId=GetAttribute<int>(Reader, "vertexindex");
				NewWeight.BoneId=GetAttribute<int>(Reader, "boneindex");
				NewWeight.Value=GetAttribute<float>(Reader, "weight");
				theSubMesh.BonesUsed=max(theSubMesh.BonesUsed, NewWeight.BoneId+1);//calculate the number of bones used (this is the highest id +1 becuase bone ids start at 0)
				
				theSubMesh.Weights[VertexId].push_back(NewWeight);

				//XmlRead(Reader);//Once i had this line, and than i got only every second boneassignment, but my first test models had even boneassignment counts, so i thougt, everything would work. And yes, i HATE irrXML!!!
			}

		}//end of boneassignments
	}
	DefaultLogger::get()->debug((Formatter::format(),
		"Positionen: ",theSubMesh.Positions.size(),
		" Normale: ",theSubMesh.Normals.size(),
		" TexCoords: ",theSubMesh.Uvs.size()
	));							
	DefaultLogger::get()->warn(Reader->getNodeName());



	//---------------Make all Vertexes unique: (this is required by assimp)-----------------------
	vector<Face> UniqueFaceList(theSubMesh.FaceList.size());
	unsigned int UniqueVertexCount=theSubMesh.FaceList.size()*3;//*3 because each face consists of 3 vertexes, because we only support triangles^^
	vector<aiVector3D> UniquePositions(UniqueVertexCount);
	vector<aiVector3D> UniqueNormals(UniqueVertexCount);
	vector<aiVector3D> UniqueUvs(UniqueVertexCount);
	vector< vector<Weight> > UniqueWeights((theSubMesh.Weights.size() ? UniqueVertexCount : 0));

	for(unsigned int i=0; i<theSubMesh.FaceList.size(); ++i)
	{
		//We precalculate the index vlaues her, because we need them in all vertex attributes
		unsigned int Vertex1=theSubMesh.FaceList[i].VertexIndices[0];
		unsigned int Vertex2=theSubMesh.FaceList[i].VertexIndices[1];
		unsigned int Vertex3=theSubMesh.FaceList[i].VertexIndices[2];

		UniquePositions[3*i+0]=theSubMesh.Positions[Vertex1];
		UniquePositions[3*i+1]=theSubMesh.Positions[Vertex2];
		UniquePositions[3*i+2]=theSubMesh.Positions[Vertex3];

		UniqueNormals[3*i+0]=theSubMesh.Normals[Vertex1];
		UniqueNormals[3*i+1]=theSubMesh.Normals[Vertex2];
		UniqueNormals[3*i+2]=theSubMesh.Normals[Vertex3];

		if(1==theSubMesh.NumUvs)
		{
			UniqueUvs[3*i+0]=theSubMesh.Uvs[Vertex1];
			UniqueUvs[3*i+1]=theSubMesh.Uvs[Vertex2];
			UniqueUvs[3*i+2]=theSubMesh.Uvs[Vertex3];
		}

		if (theSubMesh.Weights.size()) {
			UniqueWeights[3*i+0]=theSubMesh.Weights[Vertex1];
			UniqueWeights[3*i+1]=theSubMesh.Weights[Vertex2];
			UniqueWeights[3*i+2]=theSubMesh.Weights[Vertex3];
		}

		//The indexvalues a just continuous numbers (0, 1, 2, 3, 4, 5, 6...)
		UniqueFaceList[i].VertexIndices[0]=3*i+0;
		UniqueFaceList[i].VertexIndices[1]=3*i+1;
		UniqueFaceList[i].VertexIndices[2]=3*i+2;
	}
	//_________________________________________________________________________________________

	//now we have the unique datas, but want them in the SubMesh, so we swap all the containers:
	theSubMesh.FaceList.swap(UniqueFaceList);
	theSubMesh.Positions.swap(UniquePositions);
	theSubMesh.Normals.swap(UniqueNormals);
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

	aiMesh* NewAiMesh=new aiMesh();
		
	//Positions
	NewAiMesh->mVertices=new aiVector3D[theSubMesh.Positions.size()];
	memcpy(NewAiMesh->mVertices, &theSubMesh.Positions[0], theSubMesh.Positions.size()*sizeof(aiVector3D));
	NewAiMesh->mNumVertices=theSubMesh.Positions.size();

	//Normals
	NewAiMesh->mNormals=new aiVector3D[theSubMesh.Normals.size()];
	memcpy(NewAiMesh->mNormals, &theSubMesh.Normals[0], theSubMesh.Normals.size()*sizeof(aiVector3D));

	//Uvs
	if(0!=theSubMesh.NumUvs)
	{
		NewAiMesh->mNumUVComponents[0]=2;
		NewAiMesh->mTextureCoords[0]= new aiVector3D[theSubMesh.Uvs.size()];
		memcpy(NewAiMesh->mTextureCoords[0], &theSubMesh.Uvs[0], theSubMesh.Uvs.size()*sizeof(aiVector3D));
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


void OgreImporter::LoadSkeleton(std::string FileName, vector<Bone> &Bones, vector<Animation> &Animations) const
{
	const aiScene* const m_CurrentScene=this->m_CurrentScene;//make sure, that we can access but not change the scene


	//most likely the skeleton file will only end with .skeleton
	//But this is a xml reader, so we need: .skeleton.xml
	FileName+=".xml";

	DefaultLogger::get()->debug(string("Loading Skeleton: ")+FileName);

	//Open the File:
	boost::scoped_ptr<IOStream> File(m_CurrentIOHandler->Open(FileName));
	if(NULL==File.get())
		throw DeadlyImportError("Failed to open skeleton file "+FileName+".");

	//Read the Mesh File:
	boost::scoped_ptr<CIrrXML_IOStreamReader> mIOWrapper(new CIrrXML_IOStreamReader(File.get()));
	XmlReader* SkeletonFile = irr::io::createIrrXMLReader(mIOWrapper.get());
	if(!SkeletonFile)
		throw DeadlyImportError(string("Failed to create XML Reader for ")+FileName);

	//Quick note: Whoever read this should know this one thing: irrXml fucking sucks!!!

	XmlRead(SkeletonFile);
	if(string("skeleton")!=SkeletonFile->getNodeName())
		throw DeadlyImportError("No <skeleton> node in SkeletonFile: "+FileName);



	//------------------------------------load bones-----------------------------------------
	XmlRead(SkeletonFile);
	if(string("bones")!=SkeletonFile->getNodeName())
		throw DeadlyImportError("No bones node in skeleton "+FileName);

	XmlRead(SkeletonFile);

	while(string("bone")==SkeletonFile->getNodeName())
	{
		//TODO: Maybe we can have bone ids for the errrors, but normaly, they should never appear, so what....

		//read a new bone:
		Bone NewBone;
		NewBone.Id=GetAttribute<int>(SkeletonFile, "id");
		NewBone.Name=GetAttribute<string>(SkeletonFile, "name");

		//load the position:
		XmlRead(SkeletonFile);
		if(string("position")!=SkeletonFile->getNodeName())
			throw DeadlyImportError("Position is not first node in Bone!");
		NewBone.Position.x=GetAttribute<float>(SkeletonFile, "x");
		NewBone.Position.y=GetAttribute<float>(SkeletonFile, "y");
		NewBone.Position.z=GetAttribute<float>(SkeletonFile, "z");

		//Rotation:
		XmlRead(SkeletonFile);
		if(string("rotation")!=SkeletonFile->getNodeName())
			throw DeadlyImportError("Rotation is not the second node in Bone!");
		NewBone.RotationAngle=GetAttribute<float>(SkeletonFile, "angle");
		XmlRead(SkeletonFile);
		if(string("axis")!=SkeletonFile->getNodeName())
			throw DeadlyImportError("No axis specified for bone rotation!");
		NewBone.RotationAxis.x=GetAttribute<float>(SkeletonFile, "x");
		NewBone.RotationAxis.y=GetAttribute<float>(SkeletonFile, "y");
		NewBone.RotationAxis.z=GetAttribute<float>(SkeletonFile, "z");

		//append the newly loaded bone to the bone list
		Bones.push_back(NewBone);

		//Proceed to the next bone:
		XmlRead(SkeletonFile);
	}
	//The bones in the file a not neccesarly ordered by there id's so we do it now:
	std::sort(Bones.begin(), Bones.end());

	//now the id of each bone should be equal to its position in the vector:
	//so we do a simple check:
	{
		bool IdsOk=true;
		for(int i=0; i<static_cast<signed int>(Bones.size()); ++i)//i is signed, because all Id's are also signed!
		{
			if(Bones[i].Id!=i)
				IdsOk=false;
		}
		if(!IdsOk)
			throw DeadlyImportError("Bone Ids are not valid!"+FileName);
	}
	DefaultLogger::get()->debug((Formatter::format(),"Number of bones: ",Bones.size()));
	//________________________________________________________________________________






	//----------------------------load bonehierarchy--------------------------------
	if(string("bonehierarchy")!=SkeletonFile->getNodeName())
		throw DeadlyImportError("no bonehierarchy node in "+FileName);

	DefaultLogger::get()->debug("loading bonehierarchy...");
	XmlRead(SkeletonFile);
	while(string("boneparent")==SkeletonFile->getNodeName())
	{
		string Child, Parent;
		Child=GetAttribute<string>(SkeletonFile, "bone");
		Parent=GetAttribute<string>(SkeletonFile, "parent");

		unsigned int ChildId, ParentId;
		ChildId=find(Bones.begin(), Bones.end(), Child)->Id;
		ParentId=find(Bones.begin(), Bones.end(), Parent)->Id;

		Bones[ChildId].ParentId=ParentId;
		Bones[ParentId].Children.push_back(ChildId);

		XmlRead(SkeletonFile);//i once forget this line, which led to an endless loop, did i mentioned, that irrxml sucks??
	}
	//_____________________________________________________________________________


	//--------- Calculate the WorldToBoneSpace Matrix recursivly for all bones: ------------------
	BOOST_FOREACH(Bone theBone, Bones)
	{
		if(-1==theBone.ParentId) //the bone is a root bone
		{
			theBone.CalculateBoneToWorldSpaceMatrix(Bones);
		}
	}
	//_______________________________________________________________________
	

	//---------------------------load animations-----------------------------
	if(string("animations")==SkeletonFile->getNodeName())//animations are optional values
	{
		DefaultLogger::get()->debug("Loading Animations");
		XmlRead(SkeletonFile);
		while(string("animation")==SkeletonFile->getNodeName())
		{
			Animation NewAnimation;
			NewAnimation.Name=GetAttribute<string>(SkeletonFile, "name");
			NewAnimation.Length=GetAttribute<float>(SkeletonFile, "length");
			
			//Load all Tracks
			XmlRead(SkeletonFile);
			if(string("tracks")!=SkeletonFile->getNodeName())
				throw DeadlyImportError("no tracks node in animation");
			XmlRead(SkeletonFile);
			while(string("track")==SkeletonFile->getNodeName())
			{
				Track NewTrack;
				NewTrack.BoneName=GetAttribute<string>(SkeletonFile, "bone");

				//Load all keyframes;
				XmlRead(SkeletonFile);
				if(string("keyframes")!=SkeletonFile->getNodeName())
					throw DeadlyImportError("no keyframes node!");
				XmlRead(SkeletonFile);
				while(string("keyframe")==SkeletonFile->getNodeName())
				{
					Keyframe NewKeyframe;
					NewKeyframe.Time=GetAttribute<float>(SkeletonFile, "time");

					//Position:
					XmlRead(SkeletonFile);
					if(string("translate")!=SkeletonFile->getNodeName())
						throw DeadlyImportError("translate node not first in keyframe");
					NewKeyframe.Position.x=GetAttribute<float>(SkeletonFile, "x");
					NewKeyframe.Position.y=GetAttribute<float>(SkeletonFile, "y");
					NewKeyframe.Position.z=GetAttribute<float>(SkeletonFile, "z");

					//Rotation:
					XmlRead(SkeletonFile);
					if(string("rotate")!=SkeletonFile->getNodeName())
						throw DeadlyImportError("rotate is not second node in keyframe");
					float RotationAngle=GetAttribute<float>(SkeletonFile, "angle");
					aiVector3D RotationAxis;
					XmlRead(SkeletonFile);
					if(string("axis")!=SkeletonFile->getNodeName())
						throw DeadlyImportError("No axis for keyframe rotation!");
					RotationAxis.x=GetAttribute<float>(SkeletonFile, "x");
					RotationAxis.y=GetAttribute<float>(SkeletonFile, "y");
					RotationAxis.z=GetAttribute<float>(SkeletonFile, "z");
					NewKeyframe.Rotation=aiQuaternion(RotationAxis, RotationAngle);

					//Scaling:
					XmlRead(SkeletonFile);
					if(string("scale")!=SkeletonFile->getNodeName())
						throw DeadlyImportError("no scalling key in keyframe!");
					NewKeyframe.Scaling.x=GetAttribute<float>(SkeletonFile, "x");
					NewKeyframe.Scaling.y=GetAttribute<float>(SkeletonFile, "y");
					NewKeyframe.Scaling.z=GetAttribute<float>(SkeletonFile, "z");


					NewTrack.Keyframes.push_back(NewKeyframe);
					XmlRead(SkeletonFile);
				}


				NewAnimation.Tracks.push_back(NewTrack);
			}

			Animations.push_back(NewAnimation);
		}
	}
	//_____________________________________________________________________________

}


void OgreImporter::CreateAssimpSkeleton(const std::vector<Bone> &Bones, const std::vector<Animation> &Animations)
{
	if(!m_CurrentScene->mRootNode)
		throw DeadlyImportError("No root node exists!!");
	if(0!=m_CurrentScene->mRootNode->mNumChildren)
		throw DeadlyImportError("Root Node already has childnodes!");


	//Createt the assimp bone hierarchy
	DefaultLogger::get()->debug("Root Bones");
	vector<aiNode*> RootBoneNodes;
	BOOST_FOREACH(Bone theBone, Bones)
	{
		if(-1==theBone.ParentId) //the bone is a root bone
		{
			DefaultLogger::get()->debug(theBone.Name);
			RootBoneNodes.push_back(CreateAiNodeFromBone(theBone.Id, Bones, m_CurrentScene->mRootNode));//which will recursily add all other nodes
		}
	}
	
	if (RootBoneNodes.size()) {
		m_CurrentScene->mRootNode->mNumChildren=RootBoneNodes.size();	
		m_CurrentScene->mRootNode->mChildren=new aiNode*[RootBoneNodes.size()];
		memcpy(m_CurrentScene->mRootNode->mChildren, &RootBoneNodes[0], sizeof(aiNode*)*RootBoneNodes.size());
	}
}


void OgreImporter::PutAnimationsInScene(const std::vector<Bone> &Bones, const std::vector<Animation> &Animations)
{
	//-----------------Create the Assimp Animations --------------------
	if(Animations.size()>0)//Maybe the model had only a skeleton and no animations. (If it also has no skeleton, this function would'nt have been called
	{
		m_CurrentScene->mNumAnimations=Animations.size();
		m_CurrentScene->mAnimations=new aiAnimation*[Animations.size()];
		for(unsigned int i=0; i<Animations.size(); ++i)//create all animations
		{
			aiAnimation* NewAnimation=new aiAnimation();
			NewAnimation->mName=Animations[i].Name;
			NewAnimation->mDuration=Animations[i].Length;
			NewAnimation->mTicksPerSecond=1.0f;

			//Create all tracks in this animation
			NewAnimation->mNumChannels=Animations[i].Tracks.size();
			NewAnimation->mChannels=new aiNodeAnim*[Animations[i].Tracks.size()];
			for(unsigned int j=0; j<Animations[i].Tracks.size(); ++j)
			{
				aiNodeAnim* NewNodeAnim=new aiNodeAnim();
				NewNodeAnim->mNodeName=Animations[i].Tracks[j].BoneName;

				//we need this, to acces the bones default pose, which we need to make keys absolute
				vector<Bone>::const_iterator CurBone=find(Bones.begin(), Bones.end(), NewNodeAnim->mNodeName);
				aiMatrix4x4 t0, t1;
				aiMatrix4x4 DefBonePose=//The default bone pose doesnt have a scaling value
								  aiMatrix4x4::Rotation(CurBone->RotationAngle, CurBone->RotationAxis, t0)
								* aiMatrix4x4::Translation(CurBone->Position, t1);

				//Create the keyframe arrays...
				unsigned int KeyframeCount=Animations[i].Tracks[j].Keyframes.size();
				NewNodeAnim->mNumPositionKeys=KeyframeCount;
				NewNodeAnim->mPositionKeys=new aiVectorKey[KeyframeCount];
				NewNodeAnim->mNumRotationKeys=KeyframeCount;
				NewNodeAnim->mRotationKeys=new aiQuatKey[KeyframeCount];
				NewNodeAnim->mNumScalingKeys=KeyframeCount;
				NewNodeAnim->mScalingKeys=new aiVectorKey[KeyframeCount];
				
				//...and fill them
				for(unsigned int k=0; k<KeyframeCount; ++k)
				{
					aiMatrix4x4 t2, t3;

				//Create a matrix to transfrom a vector from the bones default pose to the bone bones in this animation key
				aiMatrix4x4 PoseToKey=aiMatrix4x4::Scaling(Animations[i].Tracks[j].Keyframes[k].Scaling, t2)	//scale
								* aiMatrix4x4(Animations[i].Tracks[j].Keyframes[k].Rotation.GetMatrix())		//rot
								* aiMatrix4x4::Translation(Animations[i].Tracks[j].Keyframes[k].Position, t3);	//pos
									

					//calculate the complete transformation from world space to bone space
					aiMatrix4x4 CompleteTransform=DefBonePose * PoseToKey;
					
					aiVector3D Pos;
					aiQuaternion Rot;
					aiVector3D Scale;

					CompleteTransform.Decompose(Scale, Rot, Pos);


					NewNodeAnim->mPositionKeys[k].mTime=Animations[i].Tracks[j].Keyframes[k].Time;
					NewNodeAnim->mPositionKeys[k].mValue=Pos;
					
					NewNodeAnim->mRotationKeys[k].mTime=Animations[i].Tracks[j].Keyframes[k].Time;
					NewNodeAnim->mRotationKeys[k].mValue=Rot;

					NewNodeAnim->mScalingKeys[k].mTime=Animations[i].Tracks[j].Keyframes[k].Time;
					NewNodeAnim->mScalingKeys[k].mValue=Scale;
				}
				
				NewAnimation->mChannels[j]=NewNodeAnim;
			}

			m_CurrentScene->mAnimations[i]=NewAnimation;
		}
	}
//TODO: Auf nicht vorhandene Animationskeys achten!
//#pragma warning (s.o.)
	//__________________________________________________________________
}



aiNode* OgreImporter::CreateAiNodeFromBone(int BoneId, const std::vector<Bone> &Bones, aiNode* ParentNode) const
{
	const aiScene* const m_CurrentScene=this->m_CurrentScene;//make sure, that we can access but not change the scene

	//----Create the node for this bone and set its values-----
	aiNode* NewNode=new aiNode(Bones[BoneId].Name);
	NewNode->mParent=ParentNode;

	aiMatrix4x4 t0,t1;
	//create a matrix from the transformation values of the ogre bone
	NewNode->mTransformation=aiMatrix4x4::Rotation(Bones[BoneId].RotationAngle, Bones[BoneId].RotationAxis, t1)
							*	aiMatrix4x4::Translation(Bones[BoneId].Position, t0)

							;
	//__________________________________________________________


	//---------- recursivly create all children Nodes: ----------
	NewNode->mNumChildren=Bones[BoneId].Children.size();
	NewNode->mChildren=new aiNode*[Bones[BoneId].Children.size()];
	for(unsigned int i=0; i<Bones[BoneId].Children.size(); ++i)
	{
		NewNode->mChildren[i]=CreateAiNodeFromBone(Bones[BoneId].Children[i], Bones, NewNode);
	}
	//____________________________________________________


	return NewNode;
}


void Bone::CalculateBoneToWorldSpaceMatrix(vector<Bone> &Bones)
{
	//Calculate the matrix for this bone:

	aiMatrix4x4 t0,t1;
	aiMatrix4x4 Transf=aiMatrix4x4::Translation(-Position, t0)
						* aiMatrix4x4::Rotation(-RotationAngle, RotationAxis, t1)
						;
	if(-1==ParentId)
	{
		BoneToWorldSpace=Transf;
	}
	else
	{
		BoneToWorldSpace=Transf*Bones[ParentId].BoneToWorldSpace;
	}

	//and recursivly for all children:
	BOOST_FOREACH(int theChildren, Children)
	{
		Bones[theChildren].CalculateBoneToWorldSpaceMatrix(Bones);
	}
}

}//namespace Ogre
}//namespace Assimp

#endif  // !! ASSIMP_BUILD_NO_OGRE_IMPORTER
