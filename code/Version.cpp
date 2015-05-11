
// Actually just a dummy, used by the compiler to build the precompiled header.

#include "./../include/assimp/version.h"
#include "./../include/assimp/scene.h"
#include "ScenePrivate.h"

static const unsigned int MajorVersion = 3;
static const unsigned int MinorVersion = 1;

// --------------------------------------------------------------------------------
// Legal information string - dont't remove this.
static const char* LEGAL_INFORMATION =

"Open Asset Import Library (Assimp).\n"
"A free C/C++ library to import various 3D file formats into applications\n\n"

"(c) 2008-2010, assimp team\n"
"License under the terms and conditions of the 3-clause BSD license\n"
"http://assimp.sourceforge.net\n"
;

// ------------------------------------------------------------------------------------------------
// Get legal string
ASSIMP_API const char*  aiGetLegalString  ()	{
	return LEGAL_INFORMATION;
}

// ------------------------------------------------------------------------------------------------
// Get Assimp minor version
ASSIMP_API unsigned int aiGetVersionMinor ()	{
    return MinorVersion;
}

// ------------------------------------------------------------------------------------------------
// Get Assimp major version
ASSIMP_API unsigned int aiGetVersionMajor ()	{
    return MajorVersion;
}

// ------------------------------------------------------------------------------------------------
// Get flags used for compilation
ASSIMP_API unsigned int aiGetCompileFlags ()	{

	unsigned int flags = 0;

#ifdef ASSIMP_BUILD_BOOST_WORKAROUND
	flags |= ASSIMP_CFLAGS_NOBOOST;
#endif
#ifdef ASSIMP_BUILD_SINGLETHREADED
	flags |= ASSIMP_CFLAGS_SINGLETHREADED;
#endif
#ifdef ASSIMP_BUILD_DEBUG
	flags |= ASSIMP_CFLAGS_DEBUG;
#endif
#ifdef ASSIMP_BUILD_DLL_EXPORT
	flags |= ASSIMP_CFLAGS_SHARED;
#endif
#ifdef _STLPORT_VERSION
	flags |= ASSIMP_CFLAGS_STLPORT;
#endif

	return flags;
}

// include current build revision, which is even updated from time to time -- :-)
#include "revision.h"

// ------------------------------------------------------------------------------------------------
ASSIMP_API unsigned int aiGetVersionRevision ()
{
    return GitVersion;
}

// ------------------------------------------------------------------------------------------------
ASSIMP_API aiScene::aiScene()
	: mFlags(0)
	, mRootNode(NULL)
	, mNumMeshes(0)
	, mMeshes(NULL)
	, mNumMaterials(0)
	, mMaterials(NULL)
	, mNumAnimations(0)
	, mAnimations(NULL)
	, mNumTextures(0)
	, mTextures(NULL)
	, mNumLights(0)
	, mLights(NULL)
	, mNumCameras(0)
	, mCameras(NULL)
	, mPrivate(new Assimp::ScenePrivateData())
	{
	}

// ------------------------------------------------------------------------------------------------
ASSIMP_API aiScene::~aiScene()
{
	// delete all sub-objects recursively
	delete mRootNode;

	// To make sure we won't crash if the data is invalid it's
	// much better to check whether both mNumXXX and mXXX are
	// valid instead of relying on just one of them.
	if (mNumMeshes && mMeshes)
		for( unsigned int a = 0; a < mNumMeshes; a++)
			delete mMeshes[a];
	delete [] mMeshes;

	if (mNumMaterials && mMaterials)
		for( unsigned int a = 0; a < mNumMaterials; a++)
			delete mMaterials[a];
	delete [] mMaterials;

	if (mNumAnimations && mAnimations)
		for( unsigned int a = 0; a < mNumAnimations; a++)
			delete mAnimations[a];
	delete [] mAnimations;

	if (mNumTextures && mTextures)
		for( unsigned int a = 0; a < mNumTextures; a++)
			delete mTextures[a];
	delete [] mTextures;

	if (mNumLights && mLights)
		for( unsigned int a = 0; a < mNumLights; a++)
			delete mLights[a];
	delete [] mLights;

	if (mNumCameras && mCameras)
		for( unsigned int a = 0; a < mNumCameras; a++)
			delete mCameras[a];
	delete [] mCameras;

	delete static_cast<Assimp::ScenePrivateData*>( mPrivate );
}

