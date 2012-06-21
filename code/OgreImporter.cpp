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

/** @file  OgreImporter.cpp
 *  @brief Implementation of the Ogre XML (.mesh.xml) loader.
 */
#include "AssimpPCH.h"
#ifndef ASSIMP_BUILD_NO_OGRE_IMPORTER

#include <vector>
#include <sstream>
using namespace std;

#include "OgreImporter.hpp"
#include "TinyFormatter.h"
#include "irrXMLWrapper.h"

static const aiImporterDesc desc = {
	"Ogre XML Mesh Importer",
	"",
	"",
	"",
	aiImporterFlags_SupportTextFlavour,
	0,
	0,
	0,
	0,
	"mesh.xml"
};

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

	//eventually load shared geometry
	XmlRead(MeshFile);//shared geometry is optional, so we need a reed for the next two if's
	if(MeshFile->getNodeName()==string("sharedgeometry"))
	{
		unsigned int NumVertices=GetAttribute<int>(MeshFile, "vertexcount");;

		XmlRead(MeshFile);
		while(MeshFile->getNodeName()==string("vertexbuffer"))
		{
			ReadVertexBuffer(m_SharedGeometry, MeshFile, NumVertices);
		}
	}

	//Go to the submeshs:
	if(MeshFile->getNodeName()!=string("submeshes"))
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
		XmlRead(MeshFile);
	}
	else
	{
		DefaultLogger::get()->warn("No skeleton file will be loaded");
		DefaultLogger::get()->warn(MeshFile->getNodeName());
	}
	//__________________________________________________________________


	//now there might be boneassignments for the shared geometry:
	if(MeshFile->getNodeName()==string("boneassignments"))
	{
		ReadBoneWeights(m_SharedGeometry, MeshFile);
	}


	//----------------- Process Meshs -----------------------
	BOOST_FOREACH(boost::shared_ptr<SubMesh> theSubMesh, SubMeshes)
	{
		ProcessSubMesh(*theSubMesh, m_SharedGeometry);
	}
	//_______________________________________________________



	
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


const aiImporterDesc* OgreImporter::GetInfo () const
{
	return &desc;
}


void OgreImporter::SetupProperties(const Importer* pImp)
{
	m_MaterialLibFilename=pImp->GetPropertyString(AI_CONFIG_IMPORT_OGRE_MATERIAL_FILE, "Scene.material");
	m_TextureTypeFromFilename=pImp->GetPropertyBool(AI_CONFIG_IMPORT_OGRE_TEXTURETYPE_FROM_FILENAME, false);
}


}//namespace Ogre
}//namespace Assimp

#endif  // !! ASSIMP_BUILD_NO_OGRE_IMPORTER
