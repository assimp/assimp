/*
---------------------------------------------------------------------------
Open Asset Import Library (assimp)
---------------------------------------------------------------------------

Copyright (c) 2006-2012, assimp team

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
---------------------------------------------------------------------------
*/

/** @file  AssbinLoader.cpp
 *  @brief Implementation of the .assbin importer class
 *
 *  see assbin_chunks.h
 */

#include "AssimpPCH.h"
#ifndef ASSIMP_BUILD_NO_ASSBIN_IMPORTER

// internal headers
#include "AssbinLoader.h"
#include "assbin_chunks.h"

using namespace Assimp;

static const aiImporterDesc desc = {
  ".assbin Importer",
  "Gargaj / Conspiracy",
  "",
  "",
  aiImporterFlags_SupportBinaryFlavour | aiImporterFlags_SupportCompressedFlavour,
  0,
  0,
  0,
  0,
  "assbin" 
};

const aiImporterDesc* AssbinImporter::GetInfo() const
{
  return &desc;
}

bool AssbinImporter::CanRead( const std::string& pFile, IOSystem* pIOHandler, bool checkSig ) const
{
  IOStream * in = pIOHandler->Open(pFile);
  if (!in)
    return false;

  char s[32];
  in->Read( s, sizeof(char), 32 );

  pIOHandler->Close(in);

  return strncmp( s, "ASSIMP.binary-dump.", 19 ) == 0;
}

template <typename T>
T Read(IOStream * stream)
{
  T t;
  stream->Read( &t, sizeof(T), 1 );
  return t;
}

template <>
aiString Read<aiString>(IOStream * stream)
{
  aiString s;
	stream->Read(&s.length,4,1);
  stream->Read(s.data,s.length,1);
  return s;
}

template <>
aiMatrix4x4 Read<aiMatrix4x4>(IOStream * stream)
{
  aiMatrix4x4 m;
	for (unsigned int i = 0; i < 4;++i) {
		for (unsigned int i2 = 0; i2 < 4;++i2) {
			m[i][i2] = Read<float>(stream);
		}
	}
	return m;
}

void AssbinImporter::ReadBinaryNode( IOStream * stream, aiNode** node )
{
	ai_assert( Read<uint32_t>(stream) == ASSBIN_CHUNK_AINODE);
  uint32_t size = Read<uint32_t>(stream);

  *node = new aiNode();

	(*node)->mName = Read<aiString>(stream);
	(*node)->mTransformation = Read<aiMatrix4x4>(stream);
	(*node)->mNumChildren = Read<unsigned int>(stream);
	(*node)->mNumMeshes = Read<unsigned int>(stream);

  if ((*node)->mNumMeshes)
  {
    (*node)->mMeshes = new unsigned int[(*node)->mNumMeshes];
	  for (unsigned int i = 0; i < (*node)->mNumMeshes; ++i) {
		  (*node)->mMeshes[i] = Read<unsigned int>(stream);
	  }
  }

  if ((*node)->mNumChildren)
  {
    (*node)->mChildren = new aiNode*[(*node)->mNumChildren];
	  for (unsigned int i = 0; i < (*node)->mNumChildren; ++i) {
		  ReadBinaryNode( stream, &(*node)->mChildren[i] );
	  }
  }

}

void AssbinImporter::ReadBinaryScene( IOStream * stream, aiScene* scene )
{
	ai_assert( Read<uint32_t>(stream) == ASSBIN_CHUNK_AISCENE);
  uint32_t size = Read<uint32_t>(stream);

	scene->mFlags         = Read<unsigned int>(stream);
	scene->mNumMeshes     = Read<unsigned int>(stream);
	scene->mNumMaterials  = Read<unsigned int>(stream);
	scene->mNumAnimations = Read<unsigned int>(stream);
	scene->mNumTextures   = Read<unsigned int>(stream);
	scene->mNumLights     = Read<unsigned int>(stream);
	scene->mNumCameras    = Read<unsigned int>(stream);

	// Read node graph
  scene->mRootNode = new aiNode[1];
	ReadBinaryNode( stream, &scene->mRootNode );

/*
	// Read all meshes
	for (unsigned int i = 0; i < scene->mNumMeshes;++i) {
		const aiMesh* mesh = scene->mMeshes[i];
		ReadBinaryMesh( stream,mesh);
	}

	// Read materials
	for (unsigned int i = 0; i< scene->mNumMaterials; ++i) {
		const aiMaterial* mat = scene->mMaterials[i];
		ReadBinaryMaterial(stream,mat);
	}

	// Read all animations
	for (unsigned int i = 0; i < scene->mNumAnimations;++i) {
		const aiAnimation* anim = scene->mAnimations[i];
		ReadBinaryAnim(stream,anim);
	}


	// Read all textures
	for (unsigned int i = 0; i < scene->mNumTextures;++i) {
		const aiTexture* mesh = scene->mTextures[i];
		ReadBinaryTexture(stream,mesh);
	}

	// Read lights
	for (unsigned int i = 0; i < scene->mNumLights;++i) {
		const aiLight* l = scene->mLights[i];
		ReadBinaryLight(stream,l);
	}

	// Read cameras
	for (unsigned int i = 0; i < scene->mNumCameras;++i) {
		const aiCamera* cam = scene->mCameras[i];
		ReadBinaryCamera(stream,cam);
	}
*/

}

void AssbinImporter::InternReadFile( const std::string& pFile, aiScene* pScene, IOSystem* pIOHandler )
{
	IOStream * stream = pIOHandler->Open(pFile,"rb");
	if (!stream)
    return;

  stream->Seek( 44, aiOrigin_CUR ); // signature

	unsigned int versionMajor = Read<unsigned int>(stream);
	unsigned int versionMinor = Read<unsigned int>(stream);
	unsigned int versionRevision = Read<unsigned int>(stream);
	unsigned int compileFlags = Read<unsigned int>(stream);

	shortened = Read<uint16_t>(stream) > 0;
	compressed = Read<uint16_t>(stream) > 0;

  stream->Seek( 256, aiOrigin_CUR ); // original filename
  stream->Seek( 128, aiOrigin_CUR ); // options
  stream->Seek( 64, aiOrigin_CUR ); // padding

  if (compressed)
  {
    // TODO
  }
  else
  {
    ReadBinaryScene(stream,pScene);
  }
  
  pIOHandler->Close(stream);
}

#endif // !! ASSIMP_BUILD_NO_ASSBIN_IMPORTER
