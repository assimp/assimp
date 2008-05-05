/** @file Implementation of the CPP-API class #Importer */
#include <fstream>
#include <string>

#include "../include/assimp.hpp"
#include "../include/aiScene.h"
#include "BaseImporter.h"
#include "BaseProcess.h"
#include "DefaultIOStream.h"
#include "DefaultIOSystem.h"
#include "XFileImporter.h"
#include "3DSLoader.h"
#include "MD3Loader.h"
#include "MD2Loader.h"
#include "PlyLoader.h"
#include "ObjFileImporter.h"
#include "CalcTangentsProcess.h"
#include "JoinVerticesProcess.h"
#include "ConvertToLHProcess.h"
#include "TriangulateProcess.h"
#include "GenFaceNormalsProcess.h"
#include "GenVertexNormalsProcess.h"
#include "KillNormalsProcess.h"
#include "SplitLargeMeshes.h"

using namespace Assimp;

// ------------------------------------------------------------------------------------------------
// Constructor. 
Importer::Importer() :
	mIOHandler(NULL),
	mScene(NULL),
	mErrorString("")	
{
	// default IO handler
	mIOHandler = new DefaultIOSystem;

	// add an instance of each worker class here
	mImporter.push_back( new XFileImporter());
	mImporter.push_back( new ObjFileImporter());
	mImporter.push_back( new Dot3DSImporter());
	mImporter.push_back( new MD3Importer());
	mImporter.push_back( new MD2Importer());
	mImporter.push_back( new PLYImporter());

	// add an instance of each post processing step here in the order of sequence it is executed
	mPostProcessingSteps.push_back( new TriangulateProcess());
	mPostProcessingSteps.push_back( new SplitLargeMeshesProcess());
	mPostProcessingSteps.push_back( new KillNormalsProcess());
	mPostProcessingSteps.push_back( new GenFaceNormalsProcess());
	mPostProcessingSteps.push_back( new GenVertexNormalsProcess());
	mPostProcessingSteps.push_back( new CalcTangentsProcess());
	mPostProcessingSteps.push_back( new JoinVerticesProcess());
	mPostProcessingSteps.push_back( new ConvertToLHProcess());
}

// ------------------------------------------------------------------------------------------------
// Destructor. 
Importer::~Importer()
{
	for( unsigned int a = 0; a < mImporter.size(); a++)
		delete mImporter[a];
	for( unsigned int a = 0; a < mPostProcessingSteps.size(); a++)
		delete mPostProcessingSteps[a];

	delete mIOHandler;

	// kill imported scene. Destructors should do that recursivly
	delete mScene;
}

// ------------------------------------------------------------------------------------------------
// Supplies a custom IO handler to the importer to open and access files.
void Importer::SetIOHandler( IOSystem* pIOHandler)
{
	delete mIOHandler;
	mIOHandler = pIOHandler;
}

// ------------------------------------------------------------------------------------------------
// Reads the given file and returns its contents if successful. 
const aiScene* Importer::ReadFile( const std::string& pFile, unsigned int pFlags)
{
	// first check if the file is accessable at all
	if( !mIOHandler->Exists( pFile))
	{
		mErrorString = "Unable to open file \"" + pFile + "\".";
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
		return NULL;
	}

	// dispatch the reading to the worker class for this format
	mScene = imp->ReadFile( pFile, mIOHandler);
	// if failed, extract the error string
	if( !mScene)
		mErrorString = imp->GetErrorText();

	// if successful, apply all active post processing steps to the imported data
	if( mScene)
	{
		for( unsigned int a = 0; a < mPostProcessingSteps.size(); a++)
		{
			BaseProcess* process = mPostProcessingSteps[a];
			if( process->IsActive( pFlags))
				process->Execute( mScene);
		}
	}

	// either successful or failure - the pointer expresses it anyways
	return mScene;
}

// ------------------------------------------------------------------------------------------------
//	Empty and rpivate copy constructor
Importer::Importer(const Importer &other)
{
	// empty
}

