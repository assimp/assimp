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

/** @file  WriteTextDumb.cpp
 *  @brief Implementation of the 'assimp dump' utility
 */

#include "Main.h"
#include "../code/ProcessHelper.h"

const char* AICMD_MSG_DUMP_HELP = 
"todo assimp dumb help";

// -----------------------------------------------------------------------------------
// Compress a binary dump file (beginning at offset head_size)
void CompressBinaryDump(const char* file, unsigned int head_size)
{
	// for simplicity ... copy the file into memory again and compress it there
	FILE* p = ::fopen(file,"r");
	::fseek(p,0,SEEK_END);
	const unsigned int size = (unsigned int)::ftell(p);
	::fseek(p,0,SEEK_SET);

	if (size<head_size) {
		::fclose(p);
		return;
	}

	uint8_t* data = new uint8_t[size];
	::fread(data,1,size,p);

	uLongf out_size = (uLongf)((size-head_size) * 1.001 + 12.);
	uint8_t* out = new uint8_t[out_size];

	compress2(out,&out_size,data+head_size,size-head_size,9);
	::fclose(p);
	p = ::fopen(file,"w");

	::fwrite(data,head_size,1,p);
	::fwrite(&out_size,sizeof(uLongf),1,p); // write size of uncompressed data
	::fwrite(out,out_size,1,p);

	::fclose(p);
	delete[] data;
	delete[] out;
}

// -----------------------------------------------------------------------------------
// Write a magic start value for each serialized data structure
inline void WriteMagic(const char* msg, FILE* out)
{
	::fwrite(msg,3,1,out);
}

// -----------------------------------------------------------------------------------
// Serialize an aiString
inline void WriteAiString(const aiString& s, FILE* out)
{
	const uint32_t s2 = (uint32_t)s.length;
	::fwrite(&s,4,1,out);
	::fwrite(s.data,s2,1,out);
}

// -----------------------------------------------------------------------------------
// Serialize an unsigned int as uint32_t
inline void WriteInteger(unsigned int w, FILE* out)
{
	const uint32_t t = (uint32_t)w;
	::fwrite(&t,4,1,out);
}

// -----------------------------------------------------------------------------------
// Serialize an unsigned int as uint16_t
inline void WriteShort(unsigned int w, FILE* out)
{
	const uint16_t t = (uint16_t)w;
	::fwrite(&t,2,1,out);
}

// -----------------------------------------------------------------------------------
// Serialize a float
inline void WriteFloat(float f, FILE* out)
{
	::fwrite(&f,4,1,out);
}

// -----------------------------------------------------------------------------------
// Serialize a double
inline void WriteDouble(double f, FILE* out)
{
	::fwrite(&f,8,1,out);
}

// -----------------------------------------------------------------------------------
// Serialize a vec3
inline void WriteVec3(const aiVector3D& v, FILE* out)
{
	::fwrite(&v,12,1,out);
}

// -----------------------------------------------------------------------------------
// Serialize a mat4x4
inline void WriteMat4x4(const aiMatrix4x4& m, FILE* out)
{
	for (unsigned int i = 0; i < 4;++i)
		for (unsigned int i2 = 0; i2 < 4;++i2)
			WriteFloat(m[i][i2],out);
}

// -----------------------------------------------------------------------------------
// Write a single node in a binary dump
void WriteBinaryNode(const aiNode* node, FILE* out)
{
	WriteMagic("#ND",out);
	WriteAiString(node->mName,out);
	WriteMat4x4(node->mTransformation,out);

	WriteInteger(node->mNumMeshes,out);
	for (unsigned int i = 0; i < node->mNumMeshes;++i)
		WriteInteger(node->mMeshes[i],out);

	WriteInteger(node->mNumChildren,out);
	for (unsigned int i = 0; i < node->mNumChildren;++i)
		WriteBinaryNode(node->mChildren[i],out);
}

// -----------------------------------------------------------------------------------
// Write the min/max values of an array of Ts to the file
template <typename T>
inline void WriteBounds(const T* in, unsigned int size, FILE* out)
{
	T minc,maxc;
	ArrayBounds(in,size,minc,maxc);
	::fwrite(&minc,sizeof(T),1,out);
	::fwrite(&maxc,sizeof(T),1,out);
}

// -----------------------------------------------------------------------------------
// Write a binary model dump
void WriteBinaryDump(const aiScene* scene, FILE* out, const char* src, const char* cmd, 
	bool shortened, bool compressed, ImportData& imp)
{
	time_t tt = ::time(NULL);
	tm* p     = ::gmtime(&tt);

	// header
	::fprintf(out,"ASSIMP.binary-dump.%s.",::asctime(p));
	// == 45 bytes

	WriteInteger(aiGetVersionMajor(),out);
	WriteInteger(aiGetVersionMinor(),out);
	WriteInteger(aiGetVersionRevision(),out);
	WriteInteger(aiGetCompileFlags(),out);
	WriteShort(shortened,out);
	WriteShort(compressed,out);
	// ==  20 bytes

	char buff[256]; 
	::strncpy(buff,src,256);
	::fwrite(buff,256,1,out);

	::strncpy(buff,cmd,128);
	::fwrite(buff,128,1,out);

	// leave 41 bytes free for future extensions
	::memset(buff,0xcd,41);
	::fwrite(buff,32,1,out);
	// == 435 bytes

	// ==== total header size: 500 bytes
	// Up to here the data is uncompressed. For compressed files, the rest
	// is compressed using standard DEFLATE from zlib.
	
	// basic scene information
	WriteInteger(scene->mFlags,out);
	WriteInteger(scene->mNumAnimations,out);
	WriteInteger(scene->mNumTextures,out);
	WriteInteger(scene->mNumMaterials,out);
	WriteInteger(scene->mNumCameras,out);
	WriteInteger(scene->mNumLights,out);
	WriteInteger(scene->mNumMeshes,out);

	// write node graph
	WriteBinaryNode(scene->mRootNode,out);

	// write materials
	for (unsigned int i = 0; i< scene->mNumMaterials; ++i) {
		const aiMaterial* mat = scene->mMaterials[i];

		WriteMagic("#MA",out);
		WriteInteger(mat->mNumProperties,out);

		for (unsigned int a = 0; a < mat->mNumProperties;++a) {
			const aiMaterialProperty* prop = mat->mProperties[a];
			
			WriteMagic("#MP",out);
			WriteAiString(prop->mKey,out);
			WriteInteger(prop->mSemantic,out);
			WriteInteger(prop->mIndex,out);

			WriteInteger(prop->mDataLength,out);
			::fwrite(prop->mData,prop->mDataLength,1,out);
		}
	}

	// write cameras
	for (unsigned int i = 0; i < scene->mNumCameras;++i) {
		const aiCamera* cam = scene->mCameras[i];

		WriteMagic("#CA",out);
		WriteAiString(cam->mName,out);
		WriteVec3(cam->mPosition,out);
		WriteVec3(cam->mLookAt,out);
		WriteVec3(cam->mUp,out);
		WriteFloat(cam->mClipPlaneNear,out);
		WriteFloat(cam->mClipPlaneFar,out);
		WriteFloat(cam->mHorizontalFOV,out);
		WriteFloat(cam->mAspect,out);
	}

	// write lights
	for (unsigned int i = 0; i < scene->mNumLights;++i) {
		const aiLight* l = scene->mLights[i];

		WriteMagic("#LI",out);
		WriteAiString(l->mName,out);
		WriteInteger(l->mType,out);

		WriteVec3((const aiVector3D&)l->mColorDiffuse,out);
		WriteVec3((const aiVector3D&)l->mColorSpecular,out);
		WriteVec3((const aiVector3D&)l->mColorAmbient,out);

		if (l->mType != aiLightSource_DIRECTIONAL) { 
			WriteVec3(l->mPosition,out);
			WriteFloat(l->mAttenuationLinear,out);
			WriteFloat(l->mAttenuationConstant,out);
			WriteFloat(l->mAttenuationQuadratic,out);
		}

		if (l->mType != aiLightSource_POINT) {
			WriteVec3(l->mDirection,out);
		}

		if (l->mType == aiLightSource_SPOT) {
			WriteFloat(l->mAttenuationConstant,out);
			WriteFloat(l->mAttenuationQuadratic,out);
		}
	}

	// write all animations
	for (unsigned int i = 0; i < scene->mNumAnimations;++i) {
		const aiAnimation* anim = scene->mAnimations[i];
		
		WriteMagic("#AN",out);
		WriteAiString (anim->mName,out);
		WriteDouble (anim->mTicksPerSecond,out);
		WriteDouble (anim->mDuration,out);
		WriteInteger(anim->mNumChannels,out);

		for (unsigned int a = 0; a < anim->mNumChannels;++a) {
			const aiNodeAnim* nd = anim->mChannels[a];

			WriteMagic("#NA",out);
			WriteAiString(nd->mNodeName,out);
			WriteInteger(nd->mPreState,out);
			WriteInteger(nd->mPostState,out);
			WriteInteger(nd->mNumPositionKeys,out);
			WriteInteger(nd->mNumRotationKeys,out);
			WriteInteger(nd->mNumScalingKeys,out);

			if (nd->mPositionKeys) {
				if (shortened) {
					WriteBounds(nd->mPositionKeys,nd->mNumPositionKeys,out);

				} // else write as usual
				else ::fwrite(nd->mPositionKeys,sizeof(aiVectorKey),nd->mNumPositionKeys,out);
			}
			if (nd->mRotationKeys) {
				if (shortened) {
					WriteBounds(nd->mRotationKeys,nd->mNumRotationKeys,out);

				} // else write as usual
				else ::fwrite(nd->mRotationKeys,sizeof(aiQuatKey),nd->mNumRotationKeys,out);
			}
			if (nd->mScalingKeys) {
				if (shortened) {
					WriteBounds(nd->mScalingKeys,nd->mNumScalingKeys,out);

				} // else write as usual
				else ::fwrite(nd->mScalingKeys,sizeof(aiVectorKey),nd->mNumScalingKeys,out);
			}
		}
	}

	// write all meshes
	for (unsigned int i = 0; i < scene->mNumMeshes;++i) {
		const aiMesh* mesh = scene->mMeshes[i];

		WriteMagic("#ME",out);
		WriteInteger(mesh->mPrimitiveTypes,out);
		WriteInteger(mesh->mNumBones,out);
		WriteInteger(mesh->mNumFaces,out);
		WriteInteger(mesh->mNumVertices,out);

		// write bones
		if (mesh->mNumBones) {
			for (unsigned int a = 0; a < mesh->mNumBones;++a) {
				const aiBone* b = mesh->mBones[a];

				WriteMagic("#BN",out);
				WriteAiString(b->mName,out);
				WriteMat4x4(b->mOffsetMatrix,out);
				WriteInteger(b->mNumWeights,out);

				// for the moment we write dumb min/max values for the bones, too.
				// maybe I'll add a better, hash-like solution later
				if (shortened) {
					WriteBounds(b->mWeights,b->mNumWeights,out);
				} // else write as usual
				else ::fwrite(b->mWeights,sizeof(aiVertexWeight),b->mNumWeights,out);
			}
		}

		// write faces. There are no floating-point calculations involved
		// in these, so we can write a simple hash over the face data
		// to the dump file. We generate a single 32 Bit hash for 512 faces
		// using Assimp's standard hashing function.
		if (shortened) {
			unsigned int processed = 0;
			for (unsigned int job;job = std::min(mesh->mNumFaces-processed,512u);processed += job) {

				unsigned int hash = 0;
				for (unsigned int a = 0; a < job;++a) {

					const aiFace& f = mesh->mFaces[processed+a];
					hash = SuperFastHash((const char*)&f.mNumIndices,sizeof(unsigned int),hash);
					hash = SuperFastHash((const char*) f.mIndices,f.mNumIndices*sizeof(unsigned int),hash);
				}
				WriteInteger(hash,out);
			}
		}
		else // else write as usual
		{
			for (unsigned int i = 0; i < mesh->mNumFaces;++i) {
				const aiFace& f = mesh->mFaces[i];

				WriteInteger(f.mNumIndices,out);
				for (unsigned int a = 0; a < f.mNumIndices;++a)
					WriteInteger(f.mIndices[a],out);
			}
		}

		// first of all, write bits for all existent vertex components
		unsigned int c = 0;
		if (mesh->mVertices) 
			c |= 1;
		if (mesh->mNormals)
			c |= 2;
		if (mesh->mTangents && mesh->mBitangents) 
			c |= 4;
		for (unsigned int n = 0; n < AI_MAX_NUMBER_OF_TEXTURECOORDS;++n) {
			if (!mesh->mTextureCoords[n])break;
			c |= (8 << n);
		}
		for (unsigned int n = 0; n < AI_MAX_NUMBER_OF_COLOR_SETS;++n) {
			if (!mesh->mColors[n])break;
			c |= (16 << n);
		}
		WriteInteger(c,out);
		
		aiVector3D minVec, maxVec;
		if (mesh->mVertices) {
			if (shortened) {
				WriteBounds(mesh->mVertices,mesh->mNumVertices,out);
			} // else write as usual
			else ::fwrite(mesh->mVertices,12*mesh->mNumVertices,1,out);
		}
		if (mesh->mNormals) {
			if (shortened) {
				WriteBounds(mesh->mNormals,mesh->mNumVertices,out);
			} // else write as usual
			else ::fwrite(mesh->mNormals,12*mesh->mNumVertices,1,out);
		}
		if (mesh->mTangents && mesh->mBitangents) {
			if (shortened) {
				WriteBounds(mesh->mTangents,mesh->mNumVertices,out);
				WriteBounds(mesh->mBitangents,mesh->mNumVertices,out);
			} // else write as usual
			else {
				::fwrite(mesh->mTangents,12*mesh->mNumVertices,1,out);
				::fwrite(mesh->mBitangents,12*mesh->mNumVertices,1,out);
			}
		}
		for (unsigned int n = 0; n < AI_MAX_NUMBER_OF_TEXTURECOORDS;++n) {
			if (!mesh->mTextureCoords[n])break;

			// write number of UV components
			WriteInteger(mesh->mNumUVComponents[n],out);

			if (shortened) {
				WriteBounds(mesh->mTextureCoords[n],mesh->mNumVertices,out);
			} // else write as usual
			else ::fwrite(mesh->mTextureCoords[n],12*mesh->mNumVertices,1,out);
		}
		for (unsigned int n = 0; n < AI_MAX_NUMBER_OF_COLOR_SETS;++n) {
			if (!mesh->mColors[n])
				break;

			if (shortened) {
				WriteBounds(mesh->mColors[n],mesh->mNumVertices,out);
			} // else write as usual
			else ::fwrite(mesh->mColors[n],16*mesh->mNumVertices,1,out);
		}
	}
}

// -----------------------------------------------------------------------------------
// Convert a name to standard XML format
void ConvertName(aiString& out, const aiString& in)
{
	out.length = 0;
	for (unsigned int i = 0; i < in.length; ++i)  {
		switch (in.data[i]) {
			case '<':
				out.Append("&lt;");break;
			case '>':
				out.Append("&gt;");break;
			case '&':
				out.Append("&amp;");break;
			case '\"':
				out.Append("&quot;");break;
			case '\'':
				out.Append("&apos;");break;
			default:
				out.data[out.length++] = in.data[i];
		}
	}
	out.data[out.length] = 0;
}

// -----------------------------------------------------------------------------------
// Write a single node as text dump
void WriteNode(const aiNode* node, FILE* out, unsigned int depth)
{
	char prefix[512];
	for (unsigned int i = 0; i < depth;++i)
		prefix[i] = '\t';
	prefix[depth] = '\0';

	const aiMatrix4x4& m = node->mTransformation;

	aiString name;
	ConvertName(name,node->mName);
	::fprintf(out,"%s<Node name=\"%s\"> \n"
		"%s\t<Matrix4 name=\"trafo\" > \n"
		"%s\t\t%0 6f %0 6f %0 6f %0 6f\n"
		"%s\t\t%0 6f %0 6f %0 6f %0 6f\n"
		"%s\t\t%0 6f %0 6f %0 6f %0 6f\n"
		"%s\t\t%0 6f %0 6f %0 6f %0 6f\n"
		"%s\t</Matrix4> \n",
		prefix,name.data,prefix,
		prefix,m.a1,m.a2,m.a3,m.a4,
		prefix,m.b1,m.b2,m.b3,m.b4,
		prefix,m.c1,m.c2,m.c3,m.c4,
		prefix,m.d1,m.d2,m.d3,m.d4,prefix);

	if (node->mNumMeshes) {
		::fprintf(out, "%s\t<MeshRefs num=\"%i\">\n%s\t",
			prefix,node->mNumMeshes,prefix);

		for (unsigned int i = 0; i < node->mNumMeshes;++i) {
			::fprintf(out,"%i ",node->mMeshes[i]);
		}
		::fprintf(out,"\n%s\t</MeshRefs>\n",prefix);
	}

	::fprintf(out,"%s\t<Integer name=\"num_children\">%i</Integer>\n",
		prefix,node->mNumChildren);

	for (unsigned int i = 0; i < node->mNumChildren;++i)
		WriteNode(node->mChildren[i],out,depth+1);

	::fprintf(out,"%s</Node>\n",prefix);
}

// -----------------------------------------------------------------------------------
// Write a text model dump
void WriteDump(const aiScene* scene, FILE* out, const char* src, const char* cmd, bool shortened)
{
	time_t tt = ::time(NULL);
	tm* p     = ::gmtime(&tt);

	aiString name;

	// write header
	::fprintf(out,
		"<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
		"<ASSIMP >\n\n"

		"<!-- XML Model dump produced by assimp dump\n"
		"  Library version: %i.%i.%i\n"
		"  Source: %s\n"
		"  Command line: %s\n"
		"  %s\n"
		"-->"
		" \n\n"
		"<Scene NumberOfMeshes=\"%i\" NumberOfMaterials=\"%i\" NumberOfTextures=\"%i\" NumberOfCameras=\"%i\" NumberOfLights=\"%i\" NumberOfAnimations=\"%i\">\n",
		
		aiGetVersionMajor(),aiGetVersionMinor(),aiGetVersionRevision(),src,cmd,::asctime(p),
		scene->mNumMeshes, scene->mNumMaterials,scene->mNumTextures,
		scene->mNumCameras,scene->mNumLights,scene->mNumAnimations);

	// write the node graph
	WriteNode(scene->mRootNode, out, 1);

		// write cameras
	for (unsigned int i = 0; i < scene->mNumCameras;++i) {
		aiCamera* cam  = scene->mCameras[i];
		ConvertName(name,cam->mName);

		// camera header
		::fprintf(out,"\t<Camera parent=\"%s\">\n"
			"\t\t<Vector3 name=\"up\"        > %0 8f %0 8f %0 8f </Vector3>\n"
			"\t\t<Vector3 name=\"lookat\"    > %0 8f %0 8f %0 8f </Vector3>\n"
			"\t\t<Vector3 name=\"pos\"       > %0 8f %0 8f %0 8f </Vector3>\n"
			"\t\t<Float   name=\"fov\"       > %f </Float>\n"
			"\t\t<Float   name=\"aspect\"    > %f </Float>\n"
			"\t\t<Float   name=\"near_clip\" > %f </Float>\n"
			"\t\t<Float   name=\"far_clip\"  > %f </Float>\n"
			"\t</Camera>\n",
			name.data,
			cam->mUp.x,cam->mUp.y,cam->mUp.z,
			cam->mLookAt.x,cam->mLookAt.y,cam->mLookAt.z,
			cam->mPosition.x,cam->mPosition.y,cam->mPosition.z,
			cam->mHorizontalFOV,cam->mAspect,cam->mClipPlaneNear,cam->mClipPlaneFar,i);
	}

	// write lights
	for (unsigned int i = 0; i < scene->mNumLights;++i) {
		aiLight* l  = scene->mLights[i];
		ConvertName(name,l->mName);

		// light header
		::fprintf(out,"\t<Light parent=\"%s\"> type=\"%s\"\n"
			"\t\t<Vector3 name=\"diffuse\"   > %0 8f %0 8f %0 8f </Vector3>\n"
			"\t\t<Vector3 name=\"specular\"  > %0 8f %0 8f %0 8f </Vector3>\n"
			"\t\t<Vector3 name=\"ambient\"   > %0 8f %0 8f %0 8f </Vector3>\n",
			name.data,
			(l->mType == aiLightSource_DIRECTIONAL ? "directional" :
			(l->mType == aiLightSource_POINT ? "point" : "spot" )),
			l->mColorDiffuse.r, l->mColorDiffuse.g, l->mColorDiffuse.b,
			l->mColorSpecular.r,l->mColorSpecular.g,l->mColorSpecular.b,
			l->mColorAmbient.r, l->mColorAmbient.g, l->mColorAmbient.b);

		if (l->mType != aiLightSource_DIRECTIONAL) {
			::fprintf(out,
				"\t\t<Vector3 name=\"pos\"       > %0 8f %0 8f %0 8f </Vector3>\n"
				"\t\t<Float   name=\"atten_cst\" > %f </Float>\n"
				"\t\t<Float   name=\"atten_lin\" > %f </Float>\n"
				"\t\t<Float   name=\"atten_sqr\" > %f </Float>\n",
				l->mPosition.x,l->mPosition.y,l->mPosition.z,
				l->mAttenuationConstant,l->mAttenuationLinear,l->mAttenuationQuadratic);
		}

		if (l->mType != aiLightSource_POINT) {
			::fprintf(out,
				"\t\t<Vector3 name=\"lookat\"    > %0 8f %0 8f %0 8f </Vector3>\n",
				l->mDirection.x,l->mDirection.y,l->mDirection.z);
		}

		if (l->mType == aiLightSource_SPOT) {
			::fprintf(out,
				"\t\t<Float   name=\"cone_out\" > %f </Float>\n"
				"\t\t<Float   name=\"cone_inn\" > %f </Float>\n",
				l->mAngleOuterCone,l->mAngleInnerCone);
		}
		::fprintf(out,"\t</Light>\n");
	}

	// write textures
	for (unsigned int i = 0; i < scene->mNumTextures;++i) {
		aiTexture* tex  = scene->mTextures[i];
		bool compressed = (tex->mHeight == 0);

		// mesh header
		::fprintf(out,"\t<Texture> \n"
			"\t\t<Integer   name=\"width\"      > %i </Integer>\n",
			"\t\t<Integer   name=\"height\"     > %i </Integer>\n",
			"\t\t<Boolean   name=\"compressed\" > %s </Boolean>\n",
			(compressed ? -1 : tex->mWidth),(compressed ? -1 : tex->mHeight),
			(compressed ? "true" : "false"));

		if (compressed) {
			::fprintf(out,"\t\t<Data length=\"%i\"> %i \n",tex->mWidth);

			if (!shortened) {
				for (unsigned int n = 0; n < tex->mWidth;++n) {
					::fprintf(out,"\t\t\t%2x",tex->pcData[n]);
					if (n && !(n % 50))
						::fprintf(out,"\n");
				}
			}
		}
		else if (!shortened){
			::fprintf(out,"\t\t<Data length=\"%i\"> %i \n",tex->mWidth*tex->mHeight*4);

			const unsigned int width = (unsigned int)log10((double)std::max(tex->mHeight,tex->mWidth))+1;
			for (unsigned int y = 0; y < tex->mHeight;++y) {
				for (unsigned int x = 0; x < tex->mWidth;++x) {
					aiTexel* tx = tex->pcData + y*tex->mWidth+x;
					unsigned int r = tx->r,g=tx->g,b=tx->b,a=tx->a;
					::fprintf(out,"\t\t\t%2x %2x %2x %2x",r,g,b,a);

					// group by four for readibility
					if (0 == (x+y*tex->mWidth) % 4)
						::fprintf(out,"\n");
				}
			}
		}
		::fprintf(out,"\t\t</Data>\n\t</Texture>\n");
	}

	// write materials
	for (unsigned int i = 0; i< scene->mNumMaterials; ++i) {
		const aiMaterial* mat = scene->mMaterials[i];

		::fprintf(out,
			"\t<Material  NumberOfProperties=\"%i\">\n",mat->mNumProperties);

		for (unsigned int n = 0; n < mat->mNumProperties;++n) {
			const aiMaterialProperty* prop = mat->mProperties[n];

			const char* sz = "";
			if (prop->mType == aiPTI_Float) 
				sz = "float";
			else if (prop->mType == aiPTI_Integer) 
				sz = "integer";
			else if (prop->mType == aiPTI_String) 
				sz = "string";
			else if (prop->mType == aiPTI_Buffer) 
				sz = "binary_buffer";

			::fprintf(out,
				"\t\t<MatProperty key=\"%s\" \n\t\t\ttype=\"%s\" tex_usage=\"%s\" tex_index=\"%i\"",
				prop->mKey.data, sz,
				TextureTypeToString((aiTextureType)prop->mSemantic),prop->mIndex);

			if (prop->mType == aiPTI_Float) {
				::fprintf(out,
				" size=\"%i\">\n\t\t\t",
				prop->mDataLength/sizeof(float));
				
				for (unsigned int p = 0; p < prop->mDataLength/sizeof(float);++p)
					::fprintf(out,"%f ",*((float*)(prop->mData+p*sizeof(float))));
			}
			else if (prop->mType == aiPTI_Integer) {
				::fprintf(out,
				" size=\"%i\">\n\t\t\t",
				prop->mDataLength/sizeof(int));

				for (unsigned int p = 0; p < prop->mDataLength/sizeof(int);++p)
					::fprintf(out,"%i ",*((int*)(prop->mData+p*sizeof(int))));
			}
			else if (prop->mType == aiPTI_Buffer) {
				::fprintf(out,
				" size=\"%i\">\n\t\t\t",
				prop->mDataLength);
				
				for (unsigned int p = 0; p < prop->mDataLength;++p) {
					::fprintf(out,"%2x ",prop->mData[p]);
					if (p && 0 == p%30)
						::fprintf(out,"\n\t\t\t");
				}
			}
			else if (prop->mType == aiPTI_String) {
				::fprintf(out,">\n\t\t\t\"%s\"",prop->mData+4 /* skip length */);
			}
			::fprintf(out,"\n\t\t</MatProperty>\n");
		}
		::fprintf(out,"\t</Material>\n");
	}

	// write animations
	for (unsigned int i = 0; i < scene->mNumAnimations;++i) {
		aiAnimation* anim = scene->mAnimations[i];

		// anim header
		ConvertName(name,anim->mName);
		::fprintf(out,"\t<Animation name=\"%s\">\n"
			"\t\t<Integer name=\"num_chan\" > %i </Integer>\n"
			"\t\t<Float   name=\"duration\" > %e </Float>\n"
			"\t\t<Float   name=\"tick_cnt\" > %e </Float>\n",
			name.data, anim->mNumChannels,anim->mDuration, anim->mTicksPerSecond);

		// write bone animation channels
		for (unsigned int n = 0; n < anim->mNumChannels;++n) {
			aiNodeAnim* nd = anim->mChannels[n];

			// node anim header
			ConvertName(name,nd->mNodeName);
			::fprintf(out,"\t\t<Channel node=\"%s\">\n"
				"\t\t\t<Integer name=\"num_pos_keys\" > %i </Integer>\n"
				"\t\t\t<Integer name=\"num_scl_keys\" > %i </Integer>\n"
				"\t\t\t<Integer name=\"num_rot_keys\" > %i </Integer>\n",
				name.data,nd->mNumPositionKeys,nd->mNumScalingKeys,nd->mNumRotationKeys);

			if (!shortened) {
				// write position keys
				for (unsigned int a = 0; a < nd->mNumPositionKeys;++a) {
					aiVectorKey* vc = nd->mPositionKeys+a;
					::fprintf(out,"\t\t\t<PositionKey time=\"%e\">\n"
						"\t\t\t\t%0 8f %0 8f %0 8f\n\t\t\t</PositionKey>\n",
						vc->mTime,vc->mValue.x,vc->mValue.y,vc->mValue.z,a);
				}

				// write scaling keys
				for (unsigned int a = 0; a < nd->mNumScalingKeys;++a) {
					aiVectorKey* vc = nd->mScalingKeys+a;
					::fprintf(out,"\t\t\t<ScalingKey time=\"%e\">\n"
						"\t\t\t\t%0 8f %0 8f %0 8f\n\t\t\t</ScalingKey>\n",
						vc->mTime,vc->mValue.x,vc->mValue.y,vc->mValue.z,a);
				}

				// write rotation keys
				for (unsigned int a = 0; a < nd->mNumRotationKeys;++a) {
					aiQuatKey* vc = nd->mRotationKeys+a;
					::fprintf(out,"\t\t\t<RotationKey time=\"%e\">\n"
						"\t\t\t\t%0 8f %0 8f %0 8f %0 8f\n\t\t\t</RotationKey>\n",
						vc->mTime,vc->mValue.x,vc->mValue.y,vc->mValue.z,vc->mValue.w,a);
				}
			}
			::fprintf(out,"\t\t</Channel>\n",n);
		}
		::fprintf(out,"\t</Animation>\n",i);
	}

	// write meshes
	for (unsigned int i = 0; i < scene->mNumMeshes;++i) {
		aiMesh* mesh = scene->mMeshes[i];
		const unsigned int width = (unsigned int)log10((double)mesh->mNumVertices)+1;

		// mesh header
		::fprintf(out,"\t<Mesh types=\"%s %s %s %s\">\n"
			"\t\t<Integer name=\"num_verts\" > %i </Integer>\n"
			"\t\t<Integer name=\"num_faces\" > %i </Integer>\n",
			(mesh->mPrimitiveTypes & aiPrimitiveType_POINT    ? "points"    : ""),
			(mesh->mPrimitiveTypes & aiPrimitiveType_LINE     ? "lines"     : ""),
			(mesh->mPrimitiveTypes & aiPrimitiveType_TRIANGLE ? "triangles" : ""),
			(mesh->mPrimitiveTypes & aiPrimitiveType_POLYGON  ? "polygons"  : ""),
			mesh->mNumVertices,mesh->mNumFaces);

		// bones
		for (unsigned int n = 0; n < mesh->mNumBones;++n) {
			aiBone* bone = mesh->mBones[n];

			ConvertName(name,bone->mName);
			// bone header
			::fprintf(out,"\t\t<Bone name=\"%s\">\n"
				"\t\t\t<Matrix4 name=\"offset\" > \n"
				"\t\t\t\t%0 6f %0 6f %0 6f %0 6f\n"
				"\t\t\t\t%0 6f %0 6f %0 6f %0 6f\n"
				"\t\t\t\t%0 6f %0 6f %0 6f %0 6f\n"
				"\t\t\t\t%0 6f %0 6f %0 6f %0 6f\n"
				"\t\t\t</Matrix4> \n"
				"\t\t\t<Integer name=\"num_weights\" > %i </Integer>\n",
				name.data,
				bone->mOffsetMatrix.a1,bone->mOffsetMatrix.a2,bone->mOffsetMatrix.a3,bone->mOffsetMatrix.a4,
				bone->mOffsetMatrix.b1,bone->mOffsetMatrix.b2,bone->mOffsetMatrix.b3,bone->mOffsetMatrix.b4,
				bone->mOffsetMatrix.c1,bone->mOffsetMatrix.c2,bone->mOffsetMatrix.c3,bone->mOffsetMatrix.c4,
				bone->mOffsetMatrix.d1,bone->mOffsetMatrix.d2,bone->mOffsetMatrix.d3,bone->mOffsetMatrix.d4,
				bone->mNumWeights);

			if (!shortened) {
				// bone weights
				for (unsigned int a = 0; a < bone->mNumWeights;++a) {
					aiVertexWeight* wght = bone->mWeights+a;

					::fprintf(out,"\t\t\t<VertexWeight index=\"%i\">\n\t\t\t\t%f\n\t\t\t</VertexWeight>\n",
						wght->mVertexId,wght->mWeight);
				}
			}
			::fprintf(out,"\t\t</Bone>\n",n);
		}

		// faces
		if (!shortened) {
			for (unsigned int n = 0; n < mesh->mNumFaces; ++n) {
				aiFace& f = mesh->mFaces[n];
				::fprintf(out,"\t\t<Face num_indices=\"%i\">\n"
					"\t\t\t",f.mNumIndices);

				for (unsigned int j = 0; j < f.mNumIndices;++j)
					::fprintf(out,"%i ",f.mIndices[j]);

				::fprintf(out,"\n\t\t</Face>\n");
			}
		}

		// vertex positions
		if (mesh->HasPositions()) {
			::fprintf(out,"\t\t<Positions> \n");
			if (!shortened) {
				for (unsigned int n = 0; n < mesh->mNumVertices; ++n) {
					::fprintf(out,"\t\t%0 8f %0 8f %0 8f\n",
						mesh->mVertices[n].x,
						mesh->mVertices[n].y,
						mesh->mVertices[n].z);
				}
			}
			else {
			}
			::fprintf(out,"\t\t</Positions>\n");
		}

		// vertex normals
		if (mesh->HasNormals()) {
			::fprintf(out,"\t\t<Normals> \n");
			if (!shortened) {
				for (unsigned int n = 0; n < mesh->mNumVertices; ++n) {
					::fprintf(out,"\t\t%0 8f %0 8f %0 8f\n",
						mesh->mNormals[n].x,
						mesh->mNormals[n].y,
						mesh->mNormals[n].z);
				}
			}
			else {
			}
			::fprintf(out,"\t\t</Normals>\n");
		}

		// vertex tangents and bitangents
		if (mesh->HasTangentsAndBitangents()) {
			::fprintf(out,"\t\t<Tangents> \n");
			if (!shortened) {
				for (unsigned int n = 0; n < mesh->mNumVertices; ++n) {
					::fprintf(out,"\t\t%0 8f %0 8f %0 8f \t %0 8f %0 8f %0 8f\n",
						mesh->mTangents[n].x,
						mesh->mTangents[n].y,
						mesh->mTangents[n].z,
						mesh->mBitangents[n].x,
						mesh->mBitangents[n].y,
						mesh->mBitangents[n].z);
				}
			}
			else {
			}
			::fprintf(out,"\t\t</Tangents>\n");
		}

		// texture coordinates
		for (unsigned int a = 0; a < AI_MAX_NUMBER_OF_TEXTURECOORDS; ++a) {
			if (!mesh->mTextureCoords[a])
				break;

			::fprintf(out,"\t\t<TextureCoords set=\"%i\" num_components=\"%i\"> \n",a,mesh->mNumUVComponents[a]);
			if (!shortened) {
				for (unsigned int n = 0; n < mesh->mNumVertices; ++n) {
					::fprintf(out,"\t\t%0 8f %0 8f %0 8f\n",
						mesh->mTextureCoords[a][n].x,
						mesh->mTextureCoords[a][n].y,
						mesh->mTextureCoords[a][n].z);
				}
			}
			else {
			}
			::fprintf(out,"\t\t</TextureCoords>\n");
		}

		// vertex colors
		for (unsigned int a = 0; a < AI_MAX_NUMBER_OF_COLOR_SETS; ++a) {
			if (!mesh->mColors[a])
				break;
			//::fprintf(out,"\t\t<Colors set=\"%i\"> \n",a);
			if (!shortened) {
				for (unsigned int n = 0; n < mesh->mNumVertices; ++n) {
					::fprintf(out,"\t\t%0 8f %0 8f %0 8f %0 8f\n",
						mesh->mColors[a][n].r,
						mesh->mColors[a][n].g,
						mesh->mColors[a][n].b,
						mesh->mColors[a][n].a);
				}
			}
			else {
			}
			::fprintf(out,"\t\t</Color>\n");
		}
		::fprintf(out,"\t</Mesh>\n");
	}
	::fprintf(out,"</Scene>\n</ASSIMP>");
}


// -----------------------------------------------------------------------------------
int Assimp_Dump (const char** params, unsigned int num)
{
	if (num < 1) {
		::printf("assimp dump: Invalid number of arguments. See \'assimp extract --help\'\r\n");
		return 1;
	}

	// --help
	if (!::strcmp( params[0], "-h") || !::strcmp( params[0], "--help") || !::strcmp( params[0], "-?") ) {
		printf(AICMD_MSG_DUMP_HELP);
		return 0;
	}

	// asssimp dump in out [options]
	if (num < 1) {
		::printf("assimp dump: Invalid number of arguments. See \'assimp dump --help\'\r\n");
		return 1;
	}

	std::string in  = std::string(params[0]);
	std::string out = (num > 1 ? std::string(params[1]) : std::string("-"));

	// store full command line
	std::string cmd;
	for (unsigned int i = (out[0] == '-' ? 1 : 2); i < num;++i)	{
		if (!params[i])continue;
		cmd.append(params[i]);
		cmd.append(" ");
	}

	// get import flags
	ImportData import;
	ProcessStandardArguments(import,params+1,num-1);

	bool binary = false, shortened = false,compressed=false;
	
	// process other flags
	for (unsigned int i = 1; i < num;++i)		{
		if (!params[i])continue;
		if (!::strcmp( params[i], "-b") || !::strcmp( params[i], "--binary")) {
			binary = true;
		}
		else if (!::strcmp( params[i], "-s") || !::strcmp( params[i], "--short")) {
			shortened = true;
		}
		else if (!::strcmp( params[i], "-z") || !::strcmp( params[i], "--compressed")) {
			compressed = true;
		}
		else if (i > 2 || params[i][0] == '-') {
			::printf("Unknown parameter: %s\n",params[i]);
			return 10;
		}
	}

	if (out[0] == '-') {
	
		// take file name from input file
		std::string::size_type s = in.find_last_of('.');
		if (s == std::string::npos)
			s = in.length();

		out = in.substr(0,s);
		out.append((binary ? ".assfile" : ".xml"));
		if (shortened && binary)
			out.append(".regress");
	}

	// import the main model
	const aiScene* scene = ImportModel(import,in);
	if (!scene) {
		::printf("assimp dump: Unable to load input file %s\n",in.c_str());
		return 5;
	}

	// open the output file and build the dump
	FILE* o = ::fopen(out.c_str(),(binary ? "wb" : "wt"));
	if (!o) {
		::printf("assimp dump: Unable to open output file %s\n",out.c_str());
		return 12;
	}

	if (binary)
		WriteBinaryDump (scene,o,in.c_str(),cmd.c_str(),shortened,compressed,import);
	else WriteDump (scene,o,in.c_str(),cmd.c_str(),shortened);
	::fclose(o);

	if (compressed && binary)
		CompressBinaryDump(out.c_str(),500);

	::printf("assimp dump: Wrote output dump %s\n",out.c_str());
	return 0;
}

