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

#include "AssimpPCH.h"


// internal headers
#include "BaseImporter.h"
#include "BaseProcess.h"
#include "DefaultIOStream.h"
#include "DefaultIOSystem.h"
#include "GenericProperty.h"
#include "ProcessHelper.h"
#include "ScenePreprocessor.h"

// Importers
#ifndef AI_BUILD_NO_X_IMPORTER
#	include "XFileImporter.h"
#endif
#ifndef AI_BUILD_NO_3DS_IMPORTER
#	include "3DSLoader.h"
#endif
#ifndef AI_BUILD_NO_MD3_IMPORTER
#	include "MD3Loader.h"
#endif
#ifndef AI_BUILD_NO_MDL_IMPORTER
#	include "MDLLoader.h"
#endif
#ifndef AI_BUILD_NO_MD2_IMPORTER
#	include "MD2Loader.h"
#endif
#ifndef AI_BUILD_NO_PLY_IMPORTER
#	include "PlyLoader.h"
#endif
#ifndef AI_BUILD_NO_ASE_IMPORTER
#	include "ASELoader.h"
#endif
#ifndef AI_BUILD_NO_OBJ_IMPORTER
#	include "ObjFileImporter.h"
#endif
#ifndef AI_BUILD_NO_HMP_IMPORTER
#	include "HMPLoader.h"
#endif
#ifndef AI_BUILD_NO_SMD_IMPORTER
#	include "SMDLoader.h"
#endif
#ifndef AI_BUILD_NO_MDR_IMPORTER
#	include "MDRLoader.h"
#endif
#ifndef AI_BUILD_NO_MDC_IMPORTER
#	include "MDCLoader.h"
#endif
#ifndef AI_BUILD_NO_MD5_IMPORTER
#	include "MD5Loader.h"
#endif
#ifndef AI_BUILD_NO_STL_IMPORTER
#	include "STLLoader.h"
#endif
#ifndef AI_BUILD_NO_LWO_IMPORTER
#	include "LWOLoader.h"
#endif
#ifndef AI_BUILD_NO_DXF_IMPORTER
#	include "DXFLoader.h"
#endif
#ifndef AI_BUILD_NO_NFF_IMPORTER
#	include "NFFLoader.h"
#endif
#ifndef AI_BUILD_NO_RAW_IMPORTER
#	include "RawLoader.h"
#endif
#ifndef AI_BUILD_NO_OFF_IMPORTER
#	include "OFFLoader.h"
#endif
#ifndef AI_BUILD_NO_AC_IMPORTER
#	include "ACLoader.h"
#endif
#ifndef AI_BUILD_NO_BVH_IMPORTER
#	include "BVHLoader.h"
#endif
#ifndef AI_BUILD_NO_IRRMESH_IMPORTER
#	include "IRRMeshLoader.h"
#endif
#ifndef AI_BUILD_NO_IRR_IMPORTER
#	include "IRRLoader.h"
#endif
#ifndef AI_BUILD_NO_Q3D_IMPORTER
#	include "Q3DLoader.h"
#endif
#ifndef AI_BUILD_NO_B3D_IMPORTER
#	include "B3DImporter.h"
#endif
#ifndef AI_BUILD_NO_COLLADA_IMPORTER
#	include "ColladaLoader.h"
#endif


// PostProcess-Steps
#ifndef AI_BUILD_NO_CALCTANGENTS_PROCESS
#	include "CalcTangentsProcess.h"
#endif
#ifndef AI_BUILD_NO_JOINVERTICES_PROCESS
#	include "JoinVerticesProcess.h"
#endif
#ifndef AI_BUILD_NO_CONVERTTOLH_PROCESS
#	include "ConvertToLHProcess.h"
#endif
#ifndef AI_BUILD_NO_TRIANGULATE_PROCESS
#	include "TriangulateProcess.h"
#endif
#ifndef AI_BUILD_NO_GENFACENORMALS_PROCESS
#	include "GenFaceNormalsProcess.h"
#endif
#ifndef AI_BUILD_NO_GENVERTEXNORMALS_PROCESS
#	include "GenVertexNormalsProcess.h"
#endif
#ifndef AI_BUILD_NO_REMOVEVC_PROCESS
#	include "RemoveVCProcess.h"
#endif
#ifndef AI_BUILD_NO_SPLITLARGEMESHES_PROCESS
#	include "SplitLargeMeshes.h"
#endif
#ifndef AI_BUILD_NO_PRETRANSFORMVERTICES_PROCESS
#	include "PretransformVertices.h"
#endif
#ifndef AI_BUILD_NO_LIMITBONEWEIGHTS_PROCESS
#	include "LimitBoneWeightsProcess.h"
#endif
#ifndef AI_BUILD_NO_VALIDATEDS_PROCESS
#	include "ValidateDataStructure.h"
#endif
#ifndef AI_BUILD_NO_IMPROVECACHELOCALITY_PROCESS
#	include "ImproveCacheLocality.h"
#endif
#ifndef AI_BUILD_NO_FIXINFACINGNORMALS_PROCESS
#	include "FixNormalsStep.h"
#endif
#ifndef AI_BUILD_NO_REMOVE_REDUNDANTMATERIALS_PROCESS
#	include "RemoveRedundantMaterials.h"
#endif
#ifndef AI_BUILD_NO_OPTIMIZEGRAPH_PROCESS
#	include "OptimizeGraphProcess.h"
#endif
#ifndef AI_BUILD_NO_FINDINVALIDDATA_PROCESS
#	include "FindInvalidDataProcess.h"
#endif
#ifndef AI_BUILD_NO_FINDDEGENERATES_PROCESS
#	include "FindDegenerates.h"
#endif
#ifndef AI_BUILD_NO_SORTBYPTYPE_PROCESS
#	include "SortByPTypeProcess.h"
#endif
#ifndef AI_BUILD_NO_GENUVCOORDS_PROCESS
#	include "ComputeUVMappingProcess.h"
#endif
#ifndef AI_BUILD_NO_TRANSFORMTEXCOORDS_PROCESS
#	include "TextureTransform.h"
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
	bExtraVerbose     = false; // disable extra verbose mode by default

	// add an instance of each worker class here
	// the order doesn't really care, however file formats that are
	// used more frequently than others should be at the beginning.
	mImporter.reserve(25);

#if (!defined AI_BUILD_NO_X_IMPORTER)
	mImporter.push_back( new XFileImporter());
#endif
#if (!defined AI_BUILD_NO_OBJ_IMPORTER)
	mImporter.push_back( new ObjFileImporter());
#endif
#if (!defined AI_BUILD_NO_3DS_IMPORTER)
	mImporter.push_back( new Discreet3DSImporter());
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
#if (!defined AI_BUILD_NO_MDR_IMPORTER)
	mImporter.push_back( new MDRImporter());
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
#if (!defined AI_BUILD_NO_DXF_IMPORTER)
	mImporter.push_back( new DXFImporter());
#endif
#if (!defined AI_BUILD_NO_NFF_IMPORTER)
	mImporter.push_back( new NFFImporter());
#endif
#if (!defined AI_BUILD_NO_RAW_IMPORTER)
	mImporter.push_back( new RAWImporter());
#endif
#if (!defined AI_BUILD_NO_OFF_IMPORTER)
	mImporter.push_back( new OFFImporter());
#endif
#if (!defined AI_BUILD_NO_AC_IMPORTER)
	mImporter.push_back( new AC3DImporter());
#endif
#if (!defined AI_BUILD_NO_BVH_IMPORTER)
	mImporter.push_back( new BVHLoader());
#endif
#if (!defined AI_BUILD_NO_IRRMESH_IMPORTER)
	mImporter.push_back( new IRRMeshImporter());
#endif
#if (!defined AI_BUILD_NO_IRR_IMPORTER)
	mImporter.push_back( new IRRImporter());
#endif
#if (!defined AI_BUILD_NO_Q3D_IMPORTER)
	mImporter.push_back( new Q3DImporter());
#endif
#if (!defined AI_BUILD_NO_B3D_IMPORTER)
	mImporter.push_back( new B3DImporter());
#endif
#if (!defined AI_BUILD_NO_COLLADA_IMPORTER)
	mImporter.push_back( new ColladaLoader());
#endif

	// Add an instance of each post processing step here in the order 
	// of sequence it is executed. steps that are added here are not validated -
	// as RegisterPPStep() does - all dependencies must be there.
	mPostProcessingSteps.reserve(25);

#if (!defined AI_BUILD_NO_VALIDATEDS_PROCESS)
	mPostProcessingSteps.push_back( new ValidateDSProcess()); 
#endif


#if (!defined AI_BUILD_NO_FINDDEGENERATES_PROCESS)
	mPostProcessingSteps.push_back( new FindDegeneratesProcess());
#endif


#if (!defined AI_BUILD_NO_REMOVEVC_PROCESS)
	mPostProcessingSteps.push_back( new RemoveVCProcess());
#endif


	
#ifndef AI_BUILD_NO_GENUVCOORDS_PROCESS
	mPostProcessingSteps.push_back( new ComputeUVMappingProcess());
#endif
#ifndef AI_BUILD_NO_TRANSFORMTEXCOORDS_PROCESS
	mPostProcessingSteps.push_back( new TextureTransformStep());
#endif




#if (!defined AI_BUILD_NO_REMOVE_REDUNDANTMATERIALS_PROCESS)
	mPostProcessingSteps.push_back( new RemoveRedundantMatsProcess());
#endif
#if (!defined AI_BUILD_NO_PRETRANSFORMVERTICES_PROCESS)
	mPostProcessingSteps.push_back( new PretransformVertices());
#endif
#if (!defined AI_BUILD_NO_TRIANGULATE_PROCESS)
	mPostProcessingSteps.push_back( new TriangulateProcess());
#endif
#if (!defined AI_BUILD_NO_SORTBYPTYPE_PROCESS)
	mPostProcessingSteps.push_back( new SortByPTypeProcess());
#endif

#if (!defined AI_BUILD_NO_FINDINVALIDDATA_PROCESS)
	mPostProcessingSteps.push_back( new FindInvalidDataProcess());
#endif


#if (!defined AI_BUILD_NO_OPTIMIZEGRAPH_PROCESS)
	mPostProcessingSteps.push_back( new OptimizeGraphProcess());
#endif
#if (!defined AI_BUILD_NO_FIXINFACINGNORMALS_PROCESS)
	mPostProcessingSteps.push_back( new FixInfacingNormalsProcess());
#endif
#if (!defined AI_BUILD_NO_SPLITLARGEMESHES_PROCESS)
	mPostProcessingSteps.push_back( new SplitLargeMeshesProcess_Triangle());
#endif
#if (!defined AI_BUILD_NO_GENFACENORMALS_PROCESS)
	mPostProcessingSteps.push_back( new GenFaceNormalsProcess());
#endif


	// DON'T change the order of these five!
	mPostProcessingSteps.push_back( new ComputeSpatialSortProcess());

#if (!defined AI_BUILD_NO_GENVERTEXNORMALS_PROCESS)
	mPostProcessingSteps.push_back( new GenVertexNormalsProcess());
#endif
#if (!defined AI_BUILD_NO_CALCTANGENTS_PROCESS)
	mPostProcessingSteps.push_back( new CalcTangentsProcess());
#endif
#if (!defined AI_BUILD_NO_JOINVERTICES_PROCESS)
	mPostProcessingSteps.push_back( new JoinVerticesProcess());
#endif

	mPostProcessingSteps.push_back( new DestroySpatialSortProcess());


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




	// allocate a SharedPostProcessInfo object and store pointers to it
	// in all post-process steps in the list.
	mPPShared = new SharedPostProcessInfo();
	for (std::vector<BaseProcess*>::iterator it = mPostProcessingSteps.begin(),
		 end =  mPostProcessingSteps.end(); it != end; ++it)
	{
		(*it)->SetSharedData(mPPShared);
	}
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

	// delete shared post-processing data
	delete mPPShared;
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

	// Check whether we would have two loaders for the same file extension now
	// This is absolutely OK, but we should warn the developer of the new
	// loader that his code will propably never be called.

	std::string st;
	pImp->GetExtensionList(st);

#ifdef _DEBUG
	const char* sz = ::strtok(const_cast<char*>(st.c_str()),";");
	while (sz)
	{
		if (IsExtensionSupported(std::string(sz)))
		{
			DefaultLogger::get()->warn(std::string( "The file extension " ) + sz + " is already in use");
		}
		sz = ::strtok(NULL,";");
	}
#endif

	// add the loader
	mImporter.push_back(pImp);
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
		DefaultLogger::get()->error("aiProcess_GenSmoothNormals and "
			"aiProcess_GenNormals may not be specified together");
		return false;
	}

	if (pFlags & aiProcess_PreTransformVertices &&
		pFlags & aiProcess_OptimizeGraph)
	{
		DefaultLogger::get()->error("aiProcess_PreTransformVertives and "
			"aiProcess_OptimizeGraph may not be specified together");
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

	// put a large try block around everything to catch all std::exception's
	// that might be thrown by STL containers or by new(). 
	// ImportErrorException's are throw by ourselves and caught elsewhere.
#ifndef _DEBUG
	try
	{
#endif 
		// check whether this Importer instance has already loaded
		// a scene. In this case we need to delete the old one
		if (this->mScene)
		{
			DefaultLogger::get()->debug("The previous scene has been deleted");
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
		DefaultLogger::get()->info("Found a matching importer for this file format");
		imp->SetupProperties( this );
		mScene = imp->ReadFile( pFile, mIOHandler);

		// if successful, apply all active post processing steps to the imported data
		if( mScene)
		{
			// FIRST of all - preprocess the scene 
			ScenePreprocessor pre;
			pre.ProcessScene(mScene);

			DefaultLogger::get()->info("Import successful, entering postprocessing-steps");
#ifdef _DEBUG
			if (bExtraVerbose)
			{
#if (!defined AI_BUILD_NO_VALIDATEDS_PROCESS)

				DefaultLogger::get()->error("Extra verbose mode not available, library"
					" wasn't build with the ValidateDS-Step");
#endif


				pFlags |= aiProcess_ValidateDataStructure;
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

#ifndef AI_BUILD_NO_VALIDATEDS_PROCESS
				continue;
#endif

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
		}
		// if failed, extract the error string
		else if( !mScene)mErrorString = imp->GetErrorText();

		// clear any data allocated by post-process steps
		mPPShared->Clean();
#ifndef _DEBUG
	}
	catch (std::exception &e)
	{
#if (defined _MSC_VER) &&	(defined _CPPRTTI) 

		// if we have RTTI get the full name of the exception that occured
		mErrorString = std::string(typeid( e ).name()) + ": " + e.what();
#else
		mErrorString = std::string("std::exception: ") + e.what();
#endif

		DefaultLogger::get()->error(mErrorString);
		delete mScene;mScene = NULL;
	}
#endif

	// either successful or failure - the pointer expresses it anyways
	return mScene;
}

// ------------------------------------------------------------------------------------------------
// Helper function to check whether an extension is supported by ASSIMP
bool Importer::IsExtensionSupported(const std::string& szExtension)
{
	return NULL != FindLoader(szExtension);
}

// ------------------------------------------------------------------------------------------------
BaseImporter* Importer::FindLoader (const std::string& szExtension)
{
	for (std::vector<BaseImporter*>::const_iterator
		i =  this->mImporter.begin();
		i != this->mImporter.end();++i)
	{
		// pass the file extension to the CanRead(..,NULL)-method
		if ((*i)->CanRead(szExtension,NULL))return *i;
	}
	return NULL;
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
void Importer::SetPropertyInteger(const char* szName, int iValue, 
	bool* bWasExisting /*= NULL*/)
{
	SetGenericProperty<int>(mIntProperties, szName,iValue,bWasExisting);	
}

// ------------------------------------------------------------------------------------------------
void Importer::SetPropertyFloat(const char* szName, float iValue, 
	bool* bWasExisting /*= NULL*/)
{
	SetGenericProperty<float>(mFloatProperties, szName,iValue,bWasExisting);	
}

// ------------------------------------------------------------------------------------------------
void Importer::SetPropertyString(const char* szName, const std::string& value, 
	bool* bWasExisting /*= NULL*/)
{
	SetGenericProperty<std::string>(mStringProperties, szName,value,bWasExisting);	
}

// ------------------------------------------------------------------------------------------------
// Get a configuration property
int Importer::GetPropertyInteger(const char* szName, 
	int iErrorReturn /*= 0xffffffff*/) const
{
	return GetGenericProperty<int>(mIntProperties,szName,iErrorReturn);
}

// ------------------------------------------------------------------------------------------------
float Importer::GetPropertyFloat(const char* szName, 
	float iErrorReturn /*= 10e10*/) const
{
	return GetGenericProperty<float>(mFloatProperties,szName,iErrorReturn);
}

// ------------------------------------------------------------------------------------------------
const std::string& Importer::GetPropertyString(const char* szName, 
	const std::string& iErrorReturn /*= ""*/) const
{
	return GetGenericProperty<std::string>(mStringProperties,szName,iErrorReturn);
}

// ------------------------------------------------------------------------------------------------
inline void AddNodeWeight(unsigned int& iScene,const aiNode* pcNode)
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
	in = aiMemoryInfo();
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
		for (unsigned int a = 0; a < pc->mNumChannels; ++a)
		{
			const aiNodeAnim* pc2 = pc->mChannels[i];
			in.animations += sizeof(aiNodeAnim);
			in.animations += pc2->mNumPositionKeys * sizeof(aiVectorKey);
			in.animations += pc2->mNumScalingKeys * sizeof(aiVectorKey);
			in.animations += pc2->mNumRotationKeys * sizeof(aiQuatKey);
		}
	}
	in.total += in.animations;

	// add all cameras and all lights
	in.total += in.cameras = sizeof(aiCamera) *  mScene->mNumCameras;
	in.total += in.lights  = sizeof(aiLight)  *  mScene->mNumLights;

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
}

