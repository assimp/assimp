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
#include "assbin_chunks.h"
#include "./../include/assimp/version.h"
#include "ProcessHelper.h"

#ifndef ASSIMP_BUILD_NO_EXPORT
#ifndef ASSIMP_BUILD_NO_ASSBIN_EXPORTER

using namespace Assimp;

namespace Assimp	{

	class AssbinChunkWriter : public IOStream
	{
	private:

		uint8_t* buffer;
		uint32_t magic;
		IOStream * container;
		uint32_t cur_size, cursor, initial;

	private:
		// -------------------------------------------------------------------
		void Grow(size_t need = 0) 
		{
			size_t new_size = std::max(initial, std::max( need, cur_size+(cur_size>>1) ));

			const uint8_t* const old = buffer;
			buffer = new uint8_t[new_size];

			if (old) {
				memcpy(buffer,old,cur_size);
				delete[] old;
			}

			cur_size = new_size;
		}

	public:

		AssbinChunkWriter( IOStream * container, uint32_t magic, size_t initial = 4096) 
			: initial(initial), buffer(NULL), cur_size(0), cursor(0), container(container), magic(magic)
		{
		}

		virtual ~AssbinChunkWriter() 
		{
			if (container) {
				container->Write( &magic, sizeof(uint32_t), 1 );
				container->Write( &cursor, sizeof(uint32_t), 1 );
				container->Write( buffer, 1, cursor );
			}
			if (buffer) delete[] buffer;
		}

		// -------------------------------------------------------------------
		virtual size_t Read(void* pvBuffer, 
			size_t pSize, 
			size_t pCount) { return 0; };
		virtual aiReturn Seek(size_t pOffset,
			aiOrigin pOrigin) { return aiReturn_FAILURE; };
		virtual size_t Tell() const { return 0; };
		virtual void Flush() { };

		virtual size_t FileSize() const
		{
			return cursor;
		}

		// -------------------------------------------------------------------
		virtual size_t Write(const void* pvBuffer, size_t pSize, size_t pCount) 
		{
			pSize *= pCount;
			if (cursor + pSize > cur_size) {
				Grow(cursor + pSize);
			}

			memcpy(buffer+cursor, pvBuffer, pSize);
			cursor += pSize;

			return pCount; 
		}

		template <typename T> 
		size_t Write(const T& v)
		{
			return Write( &v, sizeof(T), 1 );
		}

		// -----------------------------------------------------------------------------------
		// Serialize an aiString
		template <>
		inline uint32_t Write<aiString>(const aiString& s)
		{
			const uint32_t s2 = (uint32_t)s.length;
			Write(&s,4,1);
			Write(s.data,s2,1);
			return s2+4;
		}

		// -----------------------------------------------------------------------------------
		// Serialize an unsigned int as uint32_t
		template <>
		inline uint32_t Write<unsigned int>(const unsigned int& w)
		{
			const uint32_t t = (uint32_t)w;
			if (w > t) {
				// this shouldn't happen, integers in Assimp data structures never exceed 2^32
				printf("loss of data due to 64 -> 32 bit integer conversion");
			}

			Write(&t,4,1);
			return 4;
		}

		// -----------------------------------------------------------------------------------
		// Serialize an unsigned int as uint16_t
		template <>
		inline uint32_t Write<uint16_t>(const uint16_t& w)
		{
			Write(&w,2,1);
			return 2;
		}

		// -----------------------------------------------------------------------------------
		// Serialize a float
		template <>
		inline uint32_t Write<float>(const float& f)
		{
			BOOST_STATIC_ASSERT(sizeof(float)==4);
			Write(&f,4,1);
			return 4;
		}

		// -----------------------------------------------------------------------------------
		// Serialize a double
		template <>
		inline uint32_t Write<double>(const double& f)
		{
			BOOST_STATIC_ASSERT(sizeof(double)==8);
			Write(&f,8,1);
			return 8;
		}

		// -----------------------------------------------------------------------------------
		// Serialize a vec3
		template <>
		inline uint32_t Write<aiVector3D>(const aiVector3D& v)
		{
			uint32_t t = Write<float>(v.x);
			t += Write<float>(v.y);
			t += Write<float>(v.z);
			return t;
		}

		// -----------------------------------------------------------------------------------
		// Serialize a color value
		template <>
		inline uint32_t Write<aiColor4D>(const aiColor4D& v)
		{
			uint32_t t = Write<float>(v.r);
			t += Write<float>(v.g);
			t += Write<float>(v.b);
			t += Write<float>(v.a);
			return t;
		}

		// -----------------------------------------------------------------------------------
		// Serialize a quaternion
		template <>
		inline uint32_t Write<aiQuaternion>(const aiQuaternion& v)
		{
			uint32_t t = Write<float>(v.w);
			t += Write<float>(v.x);
			t += Write<float>(v.y);
			t += Write<float>(v.z);
			return 16;
		}


		// -----------------------------------------------------------------------------------
		// Serialize a vertex weight
		template <>
		inline uint32_t Write<aiVertexWeight>(const aiVertexWeight& v)
		{
			uint32_t t = Write<unsigned int>(v.mVertexId);
			return t+Write<float>(v.mWeight);
		}

		// -----------------------------------------------------------------------------------
		// Serialize a mat4x4
		template <>
		inline uint32_t Write<aiMatrix4x4>(const aiMatrix4x4& m)
		{
			for (unsigned int i = 0; i < 4;++i) {
				for (unsigned int i2 = 0; i2 < 4;++i2) {
					Write<float>(m[i][i2]);
				}
			}
			return 64;
		}

		// -----------------------------------------------------------------------------------
		// Serialize an aiVectorKey
		template <>
		inline uint32_t Write<aiVectorKey>(const aiVectorKey& v)
		{
			const uint32_t t = Write<double>(v.mTime);
			return t + Write<aiVector3D>(v.mValue);
		}

		// -----------------------------------------------------------------------------------
		// Serialize an aiQuatKey
		template <>
		inline uint32_t Write<aiQuatKey>(const aiQuatKey& v)
		{
			const uint32_t t = Write<double>(v.mTime);
			return t + Write<aiQuaternion>(v.mValue);
		}

		template <typename T>
		inline uint32_t WriteBounds(const T* in, unsigned int size)
		{
			T minc,maxc;
			ArrayBounds(in,size,minc,maxc);

			const uint32_t t = Write<T>(minc);
			return t + Write<T>(maxc);
		}


	};

/*
	class AssbinChunkWriter
	{
		AssbinStream stream;
		uint32_t magic;
	public:
		AssbinChunkWriter( uint32_t _magic )
		{
			magic = _magic;
		}
		void AppendToStream( AssbinStream & _stream )
		{
			uint32_t s = stream.FileSize();
			_stream.Write( &magic, sizeof(uint32_t), 1 );
			_stream.Write( &s, sizeof(uint32_t), 1 );
			_stream.Write( stream.GetBuffer(), stream.FileSize(), 1 );
		}
		void AppendToStream( AssbinChunkWriter & _stream )
		{
			uint32_t s = stream.FileSize();
			_stream.WriteRaw( &magic, sizeof(uint32_t) );
			_stream.WriteRaw( &s, sizeof(uint32_t) );
			_stream.WriteRaw( stream.GetBuffer(), stream.FileSize() );
		}

	};
*/

	class AssbinExport
	{
	private:
		bool shortened;
		bool compressed;
		//AssbinStream stream;

	protected:
		template <typename T> 
		size_t Write( IOStream * container, const T& v)
		{
			return container->Write( &v, sizeof(T), 1 );
		}

		// -----------------------------------------------------------------------------------
		void WriteBinaryNode( IOStream * container, const aiNode* node)
		{
			AssbinChunkWriter chunk( container, ASSBIN_CHUNK_AINODE );

			chunk.Write<aiString>(node->mName);
			chunk.Write<aiMatrix4x4>(node->mTransformation);
			chunk.Write<unsigned int>(node->mNumChildren);
			chunk.Write<unsigned int>(node->mNumMeshes);

			for (unsigned int i = 0; i < node->mNumMeshes;++i) {
				chunk.Write<unsigned int>(node->mMeshes[i]);
			}

			for (unsigned int i = 0; i < node->mNumChildren;++i) {
				WriteBinaryNode( &chunk, node->mChildren[i] );
			}
		}

		// -----------------------------------------------------------------------------------
		void WriteBinaryTexture(IOStream * container, const aiTexture* tex)
		{
			AssbinChunkWriter chunk( container, ASSBIN_CHUNK_AITEXTURE );

			chunk.Write<unsigned int>(tex->mWidth);
			chunk.Write<unsigned int>(tex->mHeight);
			chunk.Write( tex->achFormatHint, sizeof(char), 4 );

			if(!shortened) {
				if (!tex->mHeight) {
					chunk.Write(tex->pcData,1,tex->mWidth);
				}
				else {
					chunk.Write(tex->pcData,1,tex->mWidth*tex->mHeight*4);
				}
			}

		}

		// -----------------------------------------------------------------------------------
		void WriteBinaryBone(IOStream * container, const aiBone* b)
		{
			AssbinChunkWriter chunk( container, ASSBIN_CHUNK_AIBONE );

			chunk.Write<aiString>(b->mName);
			chunk.Write<unsigned int>(b->mNumWeights);
			chunk.Write<aiMatrix4x4>(b->mOffsetMatrix);

			// for the moment we write dumb min/max values for the bones, too.
			// maybe I'll add a better, hash-like solution later
			if (shortened) {
				chunk.WriteBounds(b->mWeights,b->mNumWeights);
			} // else write as usual
			else chunk.Write(b->mWeights,1,b->mNumWeights*sizeof(aiVertexWeight));
		}

		// -----------------------------------------------------------------------------------
		void WriteBinaryMesh(IOStream * container, const aiMesh* mesh)
		{
			AssbinChunkWriter chunk( container, ASSBIN_CHUNK_AIMESH );

			chunk.Write<unsigned int>(mesh->mPrimitiveTypes);
			chunk.Write<unsigned int>(mesh->mNumVertices);
			chunk.Write<unsigned int>(mesh->mNumFaces);
			chunk.Write<unsigned int>(mesh->mNumBones);
			chunk.Write<unsigned int>(mesh->mMaterialIndex);

			// first of all, write bits for all existent vertex components
			unsigned int c = 0;
			if (mesh->mVertices) {
				c |= ASSBIN_MESH_HAS_POSITIONS;
			}
			if (mesh->mNormals) {
				c |= ASSBIN_MESH_HAS_NORMALS;
			}
			if (mesh->mTangents && mesh->mBitangents) {
				c |= ASSBIN_MESH_HAS_TANGENTS_AND_BITANGENTS;
			}
			for (unsigned int n = 0; n < AI_MAX_NUMBER_OF_TEXTURECOORDS;++n) {
				if (!mesh->mTextureCoords[n]) {
					break;
				}
				c |= ASSBIN_MESH_HAS_TEXCOORD(n);
			}
			for (unsigned int n = 0; n < AI_MAX_NUMBER_OF_COLOR_SETS;++n) {
				if (!mesh->mColors[n]) {
					break;
				}
				c |= ASSBIN_MESH_HAS_COLOR(n);
			}
			chunk.Write<unsigned int>(c);

			aiVector3D minVec, maxVec;
			if (mesh->mVertices) {
				if (shortened) {
					chunk.WriteBounds(mesh->mVertices,mesh->mNumVertices);
				} // else write as usual
				else chunk.Write(mesh->mVertices,1,12*mesh->mNumVertices);
			}
			if (mesh->mNormals) {
				if (shortened) {
					chunk.WriteBounds(mesh->mNormals,mesh->mNumVertices);
				} // else write as usual
				else chunk.Write(mesh->mNormals,1,12*mesh->mNumVertices);
			}
			if (mesh->mTangents && mesh->mBitangents) {
				if (shortened) {
					chunk.WriteBounds(mesh->mTangents,mesh->mNumVertices);
					chunk.WriteBounds(mesh->mBitangents,mesh->mNumVertices);
				} // else write as usual
				else {
					chunk.Write(mesh->mTangents,1,12*mesh->mNumVertices);
					chunk.Write(mesh->mBitangents,1,12*mesh->mNumVertices);
				}
			}
			for (unsigned int n = 0; n < AI_MAX_NUMBER_OF_COLOR_SETS;++n) {
				if (!mesh->mColors[n])
					break;

				if (shortened) {
					chunk.WriteBounds(mesh->mColors[n],mesh->mNumVertices);
				} // else write as usual
				else chunk.Write(mesh->mColors[n],16*mesh->mNumVertices,1);
			}
			for (unsigned int n = 0; n < AI_MAX_NUMBER_OF_TEXTURECOORDS;++n) {
				if (!mesh->mTextureCoords[n])
					break;

				// write number of UV components
				chunk.Write<unsigned int>(mesh->mNumUVComponents[n]);

				if (shortened) {
					chunk.WriteBounds(mesh->mTextureCoords[n],mesh->mNumVertices);
				} // else write as usual
				else chunk.Write(mesh->mTextureCoords[n],12*mesh->mNumVertices,1);
			}

			// write faces. There are no floating-point calculations involved
			// in these, so we can write a simple hash over the face data
			// to the dump file. We generate a single 32 Bit hash for 512 faces
			// using Assimp's standard hashing function.
			if (shortened) {
				unsigned int processed = 0;
				for (unsigned int job;(job = std::min(mesh->mNumFaces-processed,512u));processed += job) {

					uint32_t hash = 0;
					for (unsigned int a = 0; a < job;++a) {

						const aiFace& f = mesh->mFaces[processed+a];
						uint32_t tmp = f.mNumIndices;
						hash = SuperFastHash(reinterpret_cast<const char*>(&tmp),sizeof tmp,hash);
						for (unsigned int i = 0; i < f.mNumIndices; ++i) {
							BOOST_STATIC_ASSERT(AI_MAX_VERTICES <= 0xffffffff);
							tmp = static_cast<uint32_t>( f.mIndices[i] );
							hash = SuperFastHash(reinterpret_cast<const char*>(&tmp),sizeof tmp,hash);
						}
					}
					chunk.Write<unsigned int>(hash);
				}
			}
			else // else write as usual
			{
				// if there are less than 2^16 vertices, we can simply use 16 bit integers ...
				for (unsigned int i = 0; i < mesh->mNumFaces;++i) {
					const aiFace& f = mesh->mFaces[i];

					BOOST_STATIC_ASSERT(AI_MAX_FACE_INDICES <= 0xffff);
					chunk.Write<uint16_t>(f.mNumIndices);

					for (unsigned int a = 0; a < f.mNumIndices;++a) {
						if (mesh->mNumVertices < (1u<<16)) {
							chunk.Write<uint16_t>(f.mIndices[a]);
						}
						else chunk.Write<unsigned int>(f.mIndices[a]);
					}
				}
			}

			// write bones
			if (mesh->mNumBones) {
				for (unsigned int a = 0; a < mesh->mNumBones;++a) {
					const aiBone* b = mesh->mBones[a];
					WriteBinaryBone(&chunk,b);
				}
			}
		}

		// -----------------------------------------------------------------------------------
		void WriteBinaryMaterialProperty(IOStream * container, const aiMaterialProperty* prop)
		{
			AssbinChunkWriter chunk( container, ASSBIN_CHUNK_AIMATERIALPROPERTY );

			chunk.Write<aiString>(prop->mKey);
			chunk.Write<unsigned int>(prop->mSemantic);
			chunk.Write<unsigned int>(prop->mIndex);

			chunk.Write<unsigned int>(prop->mDataLength);
			chunk.Write<unsigned int>((unsigned int)prop->mType);
			chunk.Write(prop->mData,1,prop->mDataLength);
		}

		// -----------------------------------------------------------------------------------
		void WriteBinaryMaterial(IOStream * container, const aiMaterial* mat)
		{
			AssbinChunkWriter chunk( container, ASSBIN_CHUNK_AIMATERIAL);

			chunk.Write<unsigned int>(mat->mNumProperties);
			for (unsigned int i = 0; i < mat->mNumProperties;++i) {
				WriteBinaryMaterialProperty( &chunk, mat->mProperties[i]);
			}
		}

		// -----------------------------------------------------------------------------------
		void WriteBinaryNodeAnim(IOStream * container, const aiNodeAnim* nd)
		{
			AssbinChunkWriter chunk( container, ASSBIN_CHUNK_AINODEANIM );

			chunk.Write<aiString>(nd->mNodeName);
			chunk.Write<unsigned int>(nd->mNumPositionKeys);
			chunk.Write<unsigned int>(nd->mNumRotationKeys);
			chunk.Write<unsigned int>(nd->mNumScalingKeys);
			chunk.Write<unsigned int>(nd->mPreState);
			chunk.Write<unsigned int>(nd->mPostState);

			if (nd->mPositionKeys) {
				if (shortened) {
					chunk.WriteBounds(nd->mPositionKeys,nd->mNumPositionKeys);

				} // else write as usual
				else chunk.Write(nd->mPositionKeys,1,nd->mNumPositionKeys*sizeof(aiVectorKey));
			}
			if (nd->mRotationKeys) {
				if (shortened) {
					chunk.WriteBounds(nd->mRotationKeys,nd->mNumRotationKeys);

				} // else write as usual
				else chunk.Write(nd->mRotationKeys,1,nd->mNumRotationKeys*sizeof(aiQuatKey));
			}
			if (nd->mScalingKeys) {
				if (shortened) {
					chunk.WriteBounds(nd->mScalingKeys,nd->mNumScalingKeys);

				} // else write as usual
				else chunk.Write(nd->mScalingKeys,1,nd->mNumScalingKeys*sizeof(aiVectorKey));
			}
		}


		// -----------------------------------------------------------------------------------
		void WriteBinaryAnim( IOStream * container, const aiAnimation* anim )
		{
			AssbinChunkWriter chunk( container, ASSBIN_CHUNK_AIANIMATION );

			chunk.Write<aiString> (anim->mName);
			chunk.Write<double> (anim->mDuration);
			chunk.Write<double> (anim->mTicksPerSecond);
			chunk.Write<unsigned int>(anim->mNumChannels);

			for (unsigned int a = 0; a < anim->mNumChannels;++a) {
				const aiNodeAnim* nd = anim->mChannels[a];
				WriteBinaryNodeAnim(&chunk,nd);	
			}
		}

		// -----------------------------------------------------------------------------------
		void WriteBinaryLight( IOStream * container, const aiLight* l )
		{
			AssbinChunkWriter chunk( container, ASSBIN_CHUNK_AILIGHT );

			chunk.Write<aiString>(l->mName);
			chunk.Write<unsigned int>(l->mType);

			if (l->mType != aiLightSource_DIRECTIONAL) { 
				chunk.Write<float>(l->mAttenuationConstant);
				chunk.Write<float>(l->mAttenuationLinear);
				chunk.Write<float>(l->mAttenuationQuadratic);
			}

			chunk.Write<aiVector3D>((const aiVector3D&)l->mColorDiffuse);
			chunk.Write<aiVector3D>((const aiVector3D&)l->mColorSpecular);
			chunk.Write<aiVector3D>((const aiVector3D&)l->mColorAmbient);

			if (l->mType == aiLightSource_SPOT) {
				chunk.Write<float>(l->mAngleInnerCone);
				chunk.Write<float>(l->mAngleOuterCone);
			}

		}

		// -----------------------------------------------------------------------------------
		void WriteBinaryCamera( IOStream * container, const aiCamera* cam )
		{
			AssbinChunkWriter chunk( container, ASSBIN_CHUNK_AICAMERA );

			chunk.Write<aiString>(cam->mName);
			chunk.Write<aiVector3D>(cam->mPosition);
			chunk.Write<aiVector3D>(cam->mLookAt);
			chunk.Write<aiVector3D>(cam->mUp);
			chunk.Write<float>(cam->mHorizontalFOV);
			chunk.Write<float>(cam->mClipPlaneNear);
			chunk.Write<float>(cam->mClipPlaneFar);
			chunk.Write<float>(cam->mAspect);
		}

		// -----------------------------------------------------------------------------------
		void WriteBinaryScene( IOStream * container, const aiScene* scene)
		{
			AssbinChunkWriter chunk( container, ASSBIN_CHUNK_AISCENE );

			// basic scene information
			chunk.Write<unsigned int>(scene->mFlags);
			chunk.Write<unsigned int>(scene->mNumMeshes);
			chunk.Write<unsigned int>(scene->mNumMaterials);
			chunk.Write<unsigned int>(scene->mNumAnimations);
			chunk.Write<unsigned int>(scene->mNumTextures);
			chunk.Write<unsigned int>(scene->mNumLights);
			chunk.Write<unsigned int>(scene->mNumCameras);

			// write node graph
			WriteBinaryNode( &chunk, scene->mRootNode );

			// write all meshes
			for (unsigned int i = 0; i < scene->mNumMeshes;++i) {
				const aiMesh* mesh = scene->mMeshes[i];
				WriteBinaryMesh( &chunk,mesh);
			}

			// write materials
			for (unsigned int i = 0; i< scene->mNumMaterials; ++i) {
				const aiMaterial* mat = scene->mMaterials[i];
				WriteBinaryMaterial(&chunk,mat);
			}

			// write all animations
			for (unsigned int i = 0; i < scene->mNumAnimations;++i) {
				const aiAnimation* anim = scene->mAnimations[i];
				WriteBinaryAnim(&chunk,anim);
			}


			// write all textures
			for (unsigned int i = 0; i < scene->mNumTextures;++i) {
				const aiTexture* mesh = scene->mTextures[i];
				WriteBinaryTexture(&chunk,mesh);
			}

			// write lights
			for (unsigned int i = 0; i < scene->mNumLights;++i) {
				const aiLight* l = scene->mLights[i];
				WriteBinaryLight(&chunk,l);
			}

			// write cameras
			for (unsigned int i = 0; i < scene->mNumCameras;++i) {
				const aiCamera* cam = scene->mCameras[i];
				WriteBinaryCamera(&chunk,cam);
			}

		}

	public:
		AssbinExport()
		{
			shortened = false;
			compressed = false;
		}

		// -----------------------------------------------------------------------------------
		// Write a binary model dump
		void WriteBinaryDump(const char* pFile, IOSystem* pIOSystem, const aiScene* pScene)
		{
			IOStream * out = pIOSystem->Open( pFile, "wb" );
			if (!out) return;

			time_t tt = time(NULL);
			tm* p     = gmtime(&tt);

			// header
			char s[64];
			memset( s, 0, 64 );
#if _MSC_VER >= 1400
			sprintf_s(s,"ASSIMP.binary-dump.%s",asctime(p));
#else
			snprintf(s,64,"ASSIMP.binary-dump.%s",asctime(p));
#endif
			out->Write( s, 44, 1 );
			// == 44 bytes

			Write<unsigned int>( out, ASSBIN_VERSION_MAJOR );
			Write<unsigned int>( out, ASSBIN_VERSION_MINOR );
			Write<unsigned int>( out, aiGetVersionRevision() );
			Write<unsigned int>( out, aiGetCompileFlags() );
			Write<uint16_t>( out, shortened );
			Write<uint16_t>( out, compressed );
			// ==  20 bytes

			//todo 

			char buff[256]; 
			strncpy(buff,pFile,256);
			out->Write(buff,sizeof(char),256);

			char cmd[] = "\0";
			strncpy(buff,cmd,128);
			out->Write(buff,sizeof(char),128);

			// leave 64 bytes free for future extensions
			memset(buff,0xcd,64);
			out->Write(buff,sizeof(char),64);
			// == 435 bytes

			// ==== total header size: 512 bytes
			ai_assert( out->Tell() == ASSBIN_HEADER_LENGTH );

			// Up to here the data is uncompressed. For compressed files, the rest
			// is compressed using standard DEFLATE from zlib.
			WriteBinaryScene( out, pScene );

			pIOSystem->Close( out );
		}
	};

void ExportSceneAssbin(const char* pFile, IOSystem* pIOSystem, const aiScene* pScene)
{
	AssbinExport exporter;
	exporter.WriteBinaryDump( pFile, pIOSystem, pScene );
}
} // end of namespace Assimp

#endif // ASSIMP_BUILD_NO_ASSBIN_EXPORTER
#endif // ASSIMP_BUILD_NO_EXPORT
