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

/** @file Implementation of the CPP-API class #Importer */

// STL/CSL heades
#include <fstream>
#include <string>

// public Assimp API
#include "../include/assimp.hpp"
#include "../include/aiAssert.h"
#include "../include/aiScene.h"
#include "../include/aiPostProcess.h"
#include "../include/DefaultLogger.h"

// internal headers
#include "BaseImporter.h"
#include "BaseProcess.h"
#include "DefaultIOStream.h"
#include "DefaultIOSystem.h"

// Importers
#if (!defined AI_BUILD_NO_X_IMPORTER)
#	include "XFileImporter.h"
#endif
#if (!defined AI_BUILD_NO_3DS_IMPORTER)
#	include "3DSLoader.h"
#endif
#if (!defined AI_BUILD_NO_MD3_IMPORTER)
#	include "MD3Loader.h"
#endif
#if (!defined AI_BUILD_NO_MDL_IMPORTER)
#	include "MDLLoader.h"
#endif
#if (!defined AI_BUILD_NO_MD2_IMPORTER)
#	include "MD2Loader.h"
#endif
#if (!defined AI_BUILD_NO_PLY_IMPORTER)
#	include "PlyLoader.h"
#endif
#if (!defined AI_BUILD_NO_ASE_IMPORTER)
#	include "ASELoader.h"
#endif
#if (!defined AI_BUILD_NO_OBJ_IMPORTER)
#	include "ObjFileImporter.h"
#endif
#if (!defined AI_BUILD_NO_HMP_IMPORTER)
#	include "HMPLoader.h"
#endif
#if (!defined AI_BUILD_NO_SMD_IMPORTER)
#	include "SMDLoader.h"
#endif
#if 0
#if (!defined AI_BUILD_NO_MDR_IMPORTER)
#	include "MDRLoader.h"
#endif
#endif
#if (!defined AI_BUILD_NO_MDC_IMPORTER)
#	include "MDCLoader.h"
#endif
#if (!defined AI_BUILD_NO_MD5_IMPORTER)
#	include "MD5Loader.h"
#endif
#if (!defined AI_BUILD_NO_STL_IMPORTER)
#	include "STLLoader.h"
#endif
#if (!defined AI_BUILD_NO_LWO_IMPORTER)
#	include "LWOLoader.h"
#endif


// PostProcess-Steps
#if (!defined AI_BUILD_NO_CALCTANGENTS_PROCESS)
#	include "CalcTangentsProcess.h"
#endif
#if (!defined AI_BUILD_NO_JOINVERTICES_PROCESS)
#	include "JoinVerticesProcess.h"
#endif
#if (!defined AI_BUILD_NO_CONVERTTOLH_PROCESS)
#	include "ConvertToLHProcess.h"
#endif
#if (!defined AI_BUILD_NO_TRIANGULATE_PROCESS)
#	include "TriangulateProcess.h"
#endif
#if (!defined AI_BUILD_NO_GENFACENORMALS_PROCESS)
#	include "GenFaceNormalsProcess.h"
#endif
#if (!defined AI_BUILD_NO_GENVERTEXNORMALS_PROCESS)
#	include "GenVertexNormalsProcess.h"
#endif
#if (!defined AI_BUILD_NO_KILLNORMALS_PROCESS)
#	include "KillNormalsProcess.h"
#endif
#if (!defined AI_BUILD_NO_SPLITLARGEMESHES_PROCESS)
#	include "SplitLargeMeshes.h"
#endif
#if (!defined AI_BUILD_NO_PRETRANSFORMVERTICES_PROCESS)
#	include "PretransformVertices.h"
#endif
#if (!defined AI_BUILD_NO_LIMITBONEWEIGHTS_PROCESS)
#	include "LimitBoneWeightsProcess.h"
#endif
#if (!defined AI_BUILD_NO_VALIDATEDS_PROCESS)
#	include "ValidateDataStructure.h"
#endif
#if (!defined AI_BUILD_NO_IMPROVECACHELOCALITY_PROCESS)
#	include "ImproveCacheLocality.h"
#endif
#if (!defined AI_BUILD_NO_FIXINFACINGNORMALS_PROCESS)
#	include "FixNormalsStep.h"
#endif
#if (!defined AI_BUILD_NO_REMOVE_REDUNDANTMATERIALS_PROCESS)
#	include "RemoveRedundantMaterials.h"
#endif


using namespace Assimp;

// ------------------------------------------------------------------------------------------------
// Constructor. 
Importer::Importer() :
	mIOHandler(NULL),
	mScene(NULL),
	mErrorString("")	
{
	// allocate a default IO handler
	mIOHandler = new DefaultIOSystem;
	mIsDefaultHandler = true; 
	bExtraVerbose = false; // disable extra verbose mode by default

	// add an instance of each worker class here
#if (!defined AI_BUILD_NO_X_IMPORTER)
	mImporter.push_back( new XFileImporter());
#endif
#if (!defined AI_BUILD_NO_OBJ_IMPORTER)
	mImporter.push_back( new ObjFileImporter());
#endif
#if (!defined AI_BUILD_NO_3DS_IMPORTER)
	mImporter.push_back( new Dot3DSImporter());
#endif
#if (!defined AI_BUILD_NO_MD3_IMPORTER)
	mImporter.push_back( new MD3Importer());
#endif
#if (!defined AI_BUILD_NO_MD2_IMPORTER)
	mImporter.push_back( new MD2Importer());
#endif
#if (!defined AI_BUILD_NO_PLY_IMPORTER)
	mImporter.push_back( new PLYImporter());
#endif
#if (!defined AI_BUILD_NO_MDL_IMPORTER)
	mImporter.push_back( new MDLImporter());
#endif
#if (!defined AI_BUILD_NO_ASE_IMPORTER)
	mImporter.push_back( new ASEImporter());
#endif
#if (!defined AI_BUILD_NO_HMP_IMPORTER)
	mImporter.push_back( new HMPImporter());
#endif
#if (!defined AI_BUILD_NO_SMD_IMPORTER)
	mImporter.push_back( new SMDImporter());
#endif
#if 0
#if (!defined AI_BUILD_NO_MDR_IMPORTER)
	mImporter.push_back( new MDRImporter());
#endif
#endif
#if (!defined AI_BUILD_NO_MDC_IMPORTER)
	mImporter.push_back( new MDCImporter());
#endif
#if (!defined AI_BUILD_NO_MD5_IMPORTER)
	mImporter.push_back( new MD5Importer());
#endif
#if (!defined AI_BUILD_NO_STL_IMPORTER)
	mImporter.push_back( new STLImporter());
#endif
#if (!defined AI_BUILD_NO_LWO_IMPORTER)
	mImporter.push_back( new LWOImporter());
#endif

	// add an instance of each post processing step here in the order 
	// of sequence it is executed
#if (!defined AI_BUILD_NO_VALIDATEDS_PROCESS)
	mPostProcessingSteps.push_back( new ValidateDSProcess()); // must be first
#endif
#if (!defined AI_BUILD_NO_REMOVE_REDUNDANTMATERIALS_PROCESS)
	mPostProcessingSteps.push_back( new RemoveRedundantMatsProcess());
#endif
#if (!defined AI_BUILD_NO_TRIANGULATE_PROCESS)
	mPostProcessingSteps.push_back( new TriangulateProcess());
#endif
#if (!defined AI_BUILD_NO_PRETRANSFORMVERTICES_PROCESS)
	mPostProcessingSteps.push_back( new PretransformVertices());
#endif
#if (!defined AI_BUILD_NO_FIXINFACINGNORMALS_PROCESS)
	mPostProcessingSteps.push_back( new FixInfacingNormalsProcess());
#endif
#if (!defined AI_BUILD_NO_SPLITLARGEMESHES_PROCESS)
	mPostProcessingSteps.push_back( new SplitLargeMeshesProcess_Triangle());
#endif
#if (!defined AI_BUILD_NO_KILLNORMALS_PROCESS)
	mPostProcessingSteps.push_back( new KillNormalsProcess());
#endif
#if (!defined AI_BUILD_NO_GENFACENORMALS_PROCESS)
	mPostProcessingSteps.push_back( new GenFaceNormalsProcess());
#endif
#if (!defined AI_BUILD_NO_GENVERTEXNORMALS_PROCESS)
	mPostProcessingSteps.push_back( new GenVertexNormalsProcess());
#endif
#if (!defined AI_BUILD_NO_CALCTANGENTS_PROCESS)
	mPostProcessingSteps.push_back( new CalcTangentsProcess());
#endif
#if (!defined AI_BUILD_NO_JOINVERTICES_PROCESS)
	mPostProcessingSteps.push_back( new JoinVerticesProcess());
#endif
#if (!defined AI_BUILD_NO_SPLITLARGEMESHES_PROCESS)
	mPostProcessingSteps.push_back( new SplitLargeMeshesProcess_Vertex());
#endif
#if (!defined AI_BUILD_NO_CONVERTTOLH_PROCESS)
	mPostProcessingSteps.push_back( new ConvertToLHProcess());
#endif
#if (!defined AI_BUILD_NO_LIMITBONEWEIGHTS_PROCESS)
	mPostProcessingSteps.push_back( new LimitBoneWeightsProcess());
#endif
#if (!defined AI_BUILD_NO_IMPROVECACHELOCALITY_PROCESS)
	mPostProcessingSteps.push_back( new ImproveCacheLocalityProcess());
#endif
}

// ------------------------------------------------------------------------------------------------
// Destructor. 
Importer::~Importer()
{
	for( unsigned int a = 0; a < mImporter.size(); a++)
		delete mImporter[a];
	for( unsigned int a = 0; a < mPostProcessingSteps.size(); a++)
		delete mPostProcessingSteps[a];

	// delete the assigned IO handler
	delete mIOHandler;

	// kill imported scene. Destructors should do that recursivly
	delete mScene;
}

// ------------------------------------------------------------------------------------------------
//	Empty and private copy constructor
Importer::Importer(const Importer &other)
{
	// empty
}

// ------------------------------------------------------------------------------------------------
aiReturn Importer::RegisterLoader(BaseImporter* pImp)
{
	ai_assert(NULL != pImp);

	// check whether we would have two loaders for the same file extension now

	std::string st;
	pImp->GetExtensionList(st);

#ifdef _DEBUG
	const char* sz = ::strtok(st.c_str(),";");
	while (sz)
	{
		if (IsExtensionSupported(std::string(sz)))
		{
			DefaultLogger::get()->error(std::string( "The file extension " ) + sz + " is already in use");
			return AI_FAILURE;
		}
		sz = ::strtok(NULL,";");
	}
#endif

	// add the loader
	this->mImporter.push_back(pImp);
	DefaultLogger::get()->info("Registering custom importer: " + st);
	return AI_SUCCESS;
}

// ------------------------------------------------------------------------------------------------
aiReturn Importer::UnregisterLoader(BaseImporter* pImp)
{
	ai_assert(NULL != pImp);

	for (std::vector<BaseImporter*>::iterator
		it = mImporter.begin(),end = mImporter.end();
		it != end;++it)
	{
		if (pImp == (*it))
		{
			mImporter.erase(it);

			std::string st;
			pImp->GetExtensionList(st);
			DefaultLogger::get()->info("Unregistering custom importer: " + st);
			return AI_SUCCESS;
		}
	}
	DefaultLogger::get()->warn("Unable to remove importer: importer not found");
	return AI_FAILURE;
}

// ------------------------------------------------------------------------------------------------
// Supplies a custom IO handler to the importer to open and access files.
void Importer::SetIOHandler( IOSystem* pIOHandler)
{
	if (!pIOHandler)
	{
		delete mIOHandler;
		mIOHandler = new DefaultIOSystem();
		mIsDefaultHandler = true;
	}
	else if (mIOHandler != pIOHandler)
	{
		delete mIOHandler;
		mIOHandler = pIOHandler;
		mIsDefaultHandler = false;
	}
	return;
}

// ------------------------------------------------------------------------------------------------
IOSystem* Importer::GetIOHandler()
{
	return mIOHandler;
}

// ------------------------------------------------------------------------------------------------
bool Importer::IsDefaultIOHandler()
{
	return mIsDefaultHandler;
}

#ifdef _DEBUG
// ------------------------------------------------------------------------------------------------
// Validate post process step flags 
bool ValidateFlags(unsigned int pFlags)
{
	if (pFlags & aiProcess_GenSmoothNormals &&
		pFlags & aiProcess_GenNormals)
	{
		DefaultLogger::get()->error("aiProcess_GenSmoothNormals and aiProcess_GenNormals "
			"may not be specified together");
		return false;
	}

	return true;
}
#endif // ! DEBUG

// ------------------------------------------------------------------------------------------------
// Reads the given file and returns its contents if successful. 
const aiScene* Importer::ReadFile( const std::string& pFile, unsigned int pFlags)
{
	// validate the flags
	ai_assert(ValidateFlags(pFlags));

	// check whether this Importer instance has already loaded
	// a scene. In this case we need to delete the old one
	if (this->mScene)
	{
		delete mScene;
		this->mScene = NULL;
	}

	// first check if the file is accessable at all
	if( !mIOHandler->Exists( pFile))
	{
		mErrorString = "Unable to open file \"" + pFile + "\".";
		DefaultLogger::get()->error(mErrorString);
		return NULL;
	}

	// find an worker class which can handle the file
	BaseImporter* imp = NULL;
	for( unsigned int a = 0; a < mImporter.size(); a++)
	{
		if( mImporter[a]->CanRead( pFile, mIOHandler))
		{
			imp = mImporter[a];
			break;
		}
	}

	// put a proper error message if no suitable importer was found
	if( !imp)
	{
		mErrorString = "No suitable reader found for the file format of file \"" + pFile + "\".";
		DefaultLogger::get()->error(mErrorString);
		return NULL;
	}

	// dispatch the reading to the worker class for this format
	imp->SetupProperties( this );
	mScene = imp->ReadFile( pFile, mIOHandler);
	
	// if successful, apply all active post processing steps to the imported data
	if( mScene)
	{
#ifdef _DEBUG
		if (bExtraVerbose)
		{
			pFlags |= aiProcess_ValidateDataStructure;

			// use the MSB to tell the ValidateDS-Step that e're in extra verbose mode
			// TODO: temporary solution, clean up later
			mScene->mFlags |= 0x80000000; 
		}
#else
		if (bExtraVerbose)DefaultLogger::get()->warn("Not a debug build, ignoring extra verbose setting");
#endif // ! DEBUG
		for( unsigned int a = 0; a < mPostProcessingSteps.size(); a++)
		{
			BaseProcess* process = mPostProcessingSteps[a];
			if( process->IsActive( pFlags))
			{
				process->SetupProperties( this );
				process->ExecuteOnScene	( this );
			}
			if( !mScene)break; 
#ifdef _DEBUG
			// if the extra verbose mode is active execute the
			// VaidateDataStructureStep again after each step
			if (bExtraVerbose && a)
			{
				DefaultLogger::get()->debug("Extra verbose: revalidating data structures");
				((ValidateDSProcess*)mPostProcessingSteps[0])->ExecuteOnScene (this);
				if( !mScene)
				{
					DefaultLogger::get()->error("Extra verbose: failed to revalidate data structures");
					break; 
				}
			}
#endif // ! DEBUG
		}
#ifdef _DEBUG
		if (bExtraVerbose)mScene->mFlags &= ~0x80000000; 
#endif // ! DEBUG
	}

	// if failed, extract the error string
	else if( !mScene)mErrorString = imp->GetErrorText();

	// either successful or failure - the pointer expresses it anyways
	return mScene;
}

// ------------------------------------------------------------------------------------------------
// Helper function to check whether an extension is supported by ASSIMP
bool Importer::IsExtensionSupported(const std::string& szExtension)
{
	for (std::vector<BaseImporter*>::const_iterator
		i =  this->mImporter.begin();
		i != this->mImporter.end();++i)
	{
		// pass the file extension to the CanRead(..,NULL)-method
		if ((*i)->CanRead(szExtension,NULL))return true;
	}
	return false;
}

// ------------------------------------------------------------------------------------------------
// Helper function to build a list of all file extensions supported by ASSIMP
void Importer::GetExtensionList(std::string& szOut)
{
	unsigned int iNum = 0;
	for (std::vector<BaseImporter*>::const_iterator
		i =  this->mImporter.begin();
		i != this->mImporter.end();++i,++iNum)
	{
		// insert a comma as delimiter character
		if (0 != iNum)
			szOut.append(";");

		(*i)->GetExtensionList(szOut);
	}
	return;
}

// ------------------------------------------------------------------------------------------------
// Set a configuration property
int Importer::SetProperty(const char* szName, int iValue)
{
	ai_assert(NULL != szName);

	// search in the list ...
	for (std::vector<IntPropertyInfo>::iterator
		i =  this->mIntProperties.begin();
		i != this->mIntProperties.end();++i)
	{
		if (0 == ::strcmp( (*i).name.c_str(), szName ))
		{
			int iOld = (*i).value;
			(*i).value = iValue;
			return iOld;
		}
	}
	// the property is not yet in the list ...
	this->mIntProperties.push_back( IntPropertyInfo() );
	IntPropertyInfo& me = this->mIntProperties.back();
	me.name = std::string(szName);
	me.value = iValue;
	return AI_PROPERTY_WAS_NOT_EXISTING;
}

// ------------------------------------------------------------------------------------------------
// Get a configuration property
int Importer::GetProperty(const char* szName, 
	int iErrorReturn /*= 0xffffffff*/) const
{
	ai_assert(NULL != szName);

	// search in the list ...
	for (std::vector<IntPropertyInfo>::const_iterator
		i =  this->mIntProperties.begin();
		i != this->mIntProperties.end();++i)
	{
		if (0 == ::strcmp( (*i).name.c_str(), szName ))
		{
			return (*i).value;
		}
	}
	return iErrorReturn;
}
// ------------------------------------------------------------------------------------------------
void AddNodeWeight(unsigned int& iScene,const aiNode* pcNode)
{
	iScene += sizeof(aiNode);
	iScene += sizeof(unsigned int) * pcNode->mNumMeshes;
	iScene += sizeof(void*) * pcNode->mNumChildren;
	for (unsigned int i = 0; i < pcNode->mNumChildren;++i)
		AddNodeWeight(iScene,pcNode->mChildren[i]);
}

// ------------------------------------------------------------------------------------------------
// Get the memory requirements of the scene
void Importer::GetMemoryRequirements(aiMemoryInfo& in) const
{
	in.aiMemoryInfo::aiMemoryInfo();
	if (!this->mScene)return;

	in.total = sizeof(aiScene);

	// add all meshes
	for (unsigned int i = 0; i < mScene->mNumMeshes;++i)
	{
		in.meshes += sizeof(aiMesh);
		if (mScene->mMeshes[i]->HasPositions())
			in.meshes += sizeof(aiVector3D) * mScene->mMeshes[i]->mNumVertices;

		if (mScene->mMeshes[i]->HasNormals())
			in.meshes += sizeof(aiVector3D) * mScene->mMeshes[i]->mNumVertices;

		if (mScene->mMeshes[i]->HasTangentsAndBitangents())
			in.meshes += sizeof(aiVector3D) * mScene->mMeshes[i]->mNumVertices * 2;

		for (unsigned int a = 0; a < AI_MAX_NUMBER_OF_COLOR_SETS;++a)
		{
			if (mScene->mMeshes[i]->HasVertexColors(a))
				in.meshes += sizeof(aiColor4D) * mScene->mMeshes[i]->mNumVertices;
			else break;
		}
		for (unsigned int a = 0; a < AI_MAX_NUMBER_OF_TEXTURECOORDS;++a)
		{
			if (mScene->mMeshes[i]->HasTextureCoords(a))
				in.meshes += sizeof(aiVector3D) * mScene->mMeshes[i]->mNumVertices;
			else break;
		}
		if (mScene->mMeshes[i]->HasBones())
		{
			in.meshes += sizeof(void*) * mScene->mMeshes[i]->mNumBones;
			for (unsigned int p = 0; p < mScene->mMeshes[i]->mNumBones;++p)
			{
				in.meshes += sizeof(aiBone);
				in.meshes += mScene->mMeshes[i]->mBones[p]->mNumWeights * sizeof(aiVertexWeight);
			}
		}
		in.meshes += (sizeof(aiFace) + 3 * sizeof(unsigned int))*mScene->mMeshes[i]->mNumFaces;
	}
    in.total += in.meshes;

	// add all embedded textures
	for (unsigned int i = 0; i < mScene->mNumTextures;++i)
	{
		const aiTexture* pc = mScene->mTextures[i];
		in.textures += sizeof(aiTexture);
		if (pc->mHeight)
		{
			in.textures += 4 * pc->mHeight * pc->mWidth;
		}
		else in.textures += pc->mWidth;
	}
	in.total += in.textures;

	// add all animations
	for (unsigned int i = 0; i < mScene->mNumAnimations;++i)
	{
		const aiAnimation* pc = mScene->mAnimations[i];
		in.animations += sizeof(aiAnimation);

		// add all bone anims
		for (unsigned int a = 0; a < pc->mNumBones;++a)
		{
			const aiBoneAnim* pc2 = pc->mBones[i];
			in.animations += sizeof(aiBoneAnim);
			in.animations += pc2->mNumPositionKeys * sizeof(aiVectorKey);
			in.animations += pc2->mNumScalingKeys * sizeof(aiVectorKey);
			in.animations += pc2->mNumRotationKeys * sizeof(aiQuatKey);
		}
	}
	in.total += in.animations;

	// add all nodes
	AddNodeWeight(in.nodes,mScene->mRootNode);
	in.total += in.nodes;

	// add all materials
	for (unsigned int i = 0; i < mScene->mNumMaterials;++i)
	{
		const aiMaterial* pc = mScene->mMaterials[i];
		in.materials += sizeof(aiMaterial);
		in.materials += pc->mNumAllocated * sizeof(void*);
		for (unsigned int a = 0; a < pc->mNumProperties;++a)
		{
			in.materials += pc->mProperties[a]->mDataLength;
		}
	}
	in.total += in.materials;
	return;
}

