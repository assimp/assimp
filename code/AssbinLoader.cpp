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

template <typename T> void ReadBounds( IOStream * stream, T* p, unsigned int n )
{
  // not sure what to do here, the data isn't really useful.
	stream->Seek( sizeof(T) * n, aiOrigin_CUR );
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

// -----------------------------------------------------------------------------------
void AssbinImporter::ReadBinaryBone( IOStream * stream, aiBone* b )
{
	ai_assert( Read<uint32_t>(stream) == ASSBIN_CHUNK_AIBONE );
  uint32_t size = Read<uint32_t>(stream);

	b->mName = Read<aiString>(stream);
	b->mNumWeights = Read<unsigned int>(stream);
	b->mOffsetMatrix = Read<aiMatrix4x4>(stream);

	// for the moment we write dumb min/max values for the bones, too.
	// maybe I'll add a better, hash-like solution later
	if (shortened) 
  {
		ReadBounds(stream,b->mWeights,b->mNumWeights);
	} // else write as usual
	else 
  {
    b->mWeights = new aiVertexWeight[b->mNumWeights];
    stream->Read(b->mWeights,1,b->mNumWeights*sizeof(aiVertexWeight));
  }
}


void AssbinImporter::ReadBinaryMesh( IOStream * stream, aiMesh* mesh )
{
	ai_assert( Read<uint32_t>(stream) == ASSBIN_CHUNK_AIMESH);
  uint32_t size = Read<uint32_t>(stream);

	mesh->mPrimitiveTypes = Read<unsigned int>(stream);
	mesh->mNumVertices = Read<unsigned int>(stream);
	mesh->mNumFaces = Read<unsigned int>(stream);
	mesh->mNumBones = Read<unsigned int>(stream);
	mesh->mMaterialIndex = Read<unsigned int>(stream);

	// first of all, write bits for all existent vertex components
	unsigned int c = Read<unsigned int>(stream);

	if (c & ASSBIN_MESH_HAS_POSITIONS) 
  {
		if (shortened) {
			ReadBounds(stream,mesh->mVertices,mesh->mNumVertices);
		} // else write as usual
		else 
    {
      mesh->mVertices = new aiVector3D[mesh->mNumVertices];
      stream->Read(mesh->mVertices,1,12*mesh->mNumVertices);
    }
	}
	if (c & ASSBIN_MESH_HAS_NORMALS) 
  {
		if (shortened) {
			ReadBounds(stream,mesh->mNormals,mesh->mNumVertices);
		} // else write as usual
		else 
    {
      mesh->mNormals = new aiVector3D[mesh->mNumVertices];
      stream->Read(mesh->mNormals,1,12*mesh->mNumVertices);
    }
	}
	if (c & ASSBIN_MESH_HAS_TANGENTS_AND_BITANGENTS) 
  {
		if (shortened) {
			ReadBounds(stream,mesh->mTangents,mesh->mNumVertices);
			ReadBounds(stream,mesh->mBitangents,mesh->mNumVertices);
		} // else write as usual
		else {
      mesh->mTangents = new aiVector3D[mesh->mNumVertices];
			stream->Read(mesh->mTangents,1,12*mesh->mNumVertices);
      mesh->mBitangents = new aiVector3D[mesh->mNumVertices];
			stream->Read(mesh->mBitangents,1,12*mesh->mNumVertices);
		}
	}
	for (unsigned int n = 0; n < AI_MAX_NUMBER_OF_COLOR_SETS;++n) 
  {
		if (!(c & ASSBIN_MESH_HAS_COLOR(n)))
			break;

		if (shortened) 
    {
			ReadBounds(stream,mesh->mColors[n],mesh->mNumVertices);
		} // else write as usual
		else 
    {
      mesh->mColors[n] = new aiColor4D[mesh->mNumVertices];
      stream->Read(mesh->mColors[n],16*mesh->mNumVertices,1);
    }
	}
	for (unsigned int n = 0; n < AI_MAX_NUMBER_OF_TEXTURECOORDS;++n) 
  {
		if (!(c & ASSBIN_MESH_HAS_TEXCOORD(n)))
			break;

		// write number of UV components
		mesh->mNumUVComponents[n] = Read<unsigned int>(stream);

		if (shortened) {
			ReadBounds(stream,mesh->mTextureCoords[n],mesh->mNumVertices);
		} // else write as usual
		else 
    {
      mesh->mTextureCoords[n] = new aiVector3D[mesh->mNumVertices];
      stream->Read(mesh->mTextureCoords[n],12*mesh->mNumVertices,1);
    }
	}

	// write faces. There are no floating-point calculations involved
	// in these, so we can write a simple hash over the face data
	// to the dump file. We generate a single 32 Bit hash for 512 faces
	// using Assimp's standard hashing function.
	if (shortened) {
		Read<unsigned int>(stream);
	}
	else // else write as usual
	{
		// if there are less than 2^16 vertices, we can simply use 16 bit integers ...
    mesh->mFaces = new aiFace[mesh->mNumFaces];
		for (unsigned int i = 0; i < mesh->mNumFaces;++i) {
			aiFace& f = mesh->mFaces[i];

			BOOST_STATIC_ASSERT(AI_MAX_FACE_INDICES <= 0xffff);
			f.mNumIndices = Read<uint16_t>(stream);
      f.mIndices = new unsigned int[f.mNumIndices];

			for (unsigned int a = 0; a < f.mNumIndices;++a) {
				if (mesh->mNumVertices < (1u<<16)) 
        {
					f.mIndices[a] = Read<uint16_t>(stream);
				}
				else 
        {
          f.mIndices[a] = Read<unsigned int>(stream);
        }
			}
		}
	}

	// write bones
	if (mesh->mNumBones) {
    mesh->mBones = new C_STRUCT aiBone*[mesh->mNumBones];
		for (unsigned int a = 0; a < mesh->mNumBones;++a) {
			mesh->mBones[a] = new aiBone();
			ReadBinaryBone(stream,mesh->mBones[a]);
		}
	}
}

void AssbinImporter::ReadBinaryMaterialProperty(IOStream * stream, aiMaterialProperty* prop)
{
	ai_assert( Read<uint32_t>(stream) == ASSBIN_CHUNK_AIMATERIALPROPERTY);
  uint32_t size = Read<uint32_t>(stream);

	prop->mKey = Read<aiString>(stream);
	prop->mSemantic = Read<unsigned int>(stream);
	prop->mIndex = Read<unsigned int>(stream);

	prop->mDataLength = Read<unsigned int>(stream);
	prop->mType = (aiPropertyTypeInfo)Read<unsigned int>(stream);
  prop->mData = new char [ prop->mDataLength ];
	stream->Read(prop->mData,1,prop->mDataLength);
}

// -----------------------------------------------------------------------------------
void AssbinImporter::ReadBinaryMaterial(IOStream * stream, aiMaterial* mat)
{
	ai_assert( Read<uint32_t>(stream) == ASSBIN_CHUNK_AIMATERIAL);
  uint32_t size = Read<uint32_t>(stream);

	mat->mNumAllocated = mat->mNumProperties = Read<unsigned int>(stream);
  if (mat->mNumProperties)
  {
    if (mat->mProperties) 
    {
      delete[] mat->mProperties;
    }
    mat->mProperties = new aiMaterialProperty*[mat->mNumProperties];
	  for (unsigned int i = 0; i < mat->mNumProperties;++i) {
      mat->mProperties[i] = new aiMaterialProperty();
		  ReadBinaryMaterialProperty( stream, mat->mProperties[i]);
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

	// Read all meshes
  if (scene->mNumMeshes)
  {
    scene->mMeshes = new aiMesh*[scene->mNumMeshes];
	  for (unsigned int i = 0; i < scene->mNumMeshes;++i) {
		  scene->mMeshes[i] = new aiMesh();
		  ReadBinaryMesh( stream,scene->mMeshes[i]);
	  }
  }

	// Read materials
  if (scene->mNumMaterials)
  {
    scene->mMaterials = new aiMaterial*[scene->mNumMaterials];
	  for (unsigned int i = 0; i< scene->mNumMaterials; ++i) {
		  scene->mMaterials[i] = new aiMaterial();
		  ReadBinaryMaterial(stream,scene->mMaterials[i]);
	  }
  }
/*

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
