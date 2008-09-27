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

/** @file Implementation of the JNI API for jAssimp */

#include "JNIEnvironment.h"
#include "JNILogger.h"

using namespace Assimp;

namespace Assimp	{
namespace JNIBridge		{

// ------------------------------------------------------------------------------------------------
void JNIEnvironment::_assimp::_Mesh::Initialize()
{
	JNIEnv* pc = JNIEnvironment::Get()->GetThread()->m_pcEnv;

	// load a handle to the class
	if(!(this->Class = pc->FindClass("assimp.Mesh")))
		JNIEnvironment::Get()->ThrowNativeError("Unable to load class assimp.mesh");

	DefaultCtor = pc->GetMethodID(Class,"<init>","");

	// load all fields of the class
	this->m_vVertices	= pc->GetFieldID(Class,"m_vVertices","[F");
	this->m_vNormals	= pc->GetFieldID(Class,"m_vNormals","[F");
	this->m_vTangents	= pc->GetFieldID(Class,"m_vTangents","[F");
	this->m_vBitangents = pc->GetFieldID(Class,"m_vBitangents","[F");
	this->m_avColors	= pc->GetFieldID(Class,"m_avColors","[[F");
	this->m_avUVs		= pc->GetFieldID(Class,"m_avColors","[[F");
	this->m_vFaces		= pc->GetFieldID(Class,"m_vFaces","[I");
	this->m_vBones		= pc->GetFieldID(Class,"m_vBones","[Lassimp.Bone;");

	this->m_aiNumUVComponents	= pc->GetFieldID(Class,"m_aiNumUVComponents","[I");
	this->m_iMaterialIndex		= pc->GetFieldID(Class,"m_iMaterialIndex","I");

	// check whether all fields have been loaded properly
	if (!this->m_vVertices		|| !this->m_vNormals	|| !this->m_vTangents ||
		!this->m_vBitangents	|| !this->m_avColors	|| !this->m_avUVs ||
		!this->m_vFaces			|| !this->m_vBones		|| !this->m_aiNumUVComponents ||
		!this->m_iMaterialIndex)
	{
		JNIEnvironment::Get()->ThrowNativeError("Unable to load all fields of class assimp.mesh");
	}
}
// ------------------------------------------------------------------------------------------------
void JNIEnvironment::_assimp::_Mesh::Fill(jobject obj,const aiMesh* pcSrc)
{
	JNIEnv* pc = JNIEnvironment::Get()->GetThread()->m_pcEnv;
	
	// set the material index
	pc->SetIntField(obj,this->m_iMaterialIndex,pcSrc->mMaterialIndex);


	// allocate the arrays and fill them
	const unsigned int size = pcSrc->mNumVertices*12;
	const unsigned int size2 = pcSrc->mNumVertices*3;

	// copy vertex positions
	if (pcSrc->HasPositions())
	{
		jfloatArray jfl = pc->NewFloatArray(size2);	
		JNU_CopyDataToArray(pc,jfl,pcSrc->mVertices,size);

		pc->SetObjectField(obj,this->m_vVertices,jfl);
	}
	// copy vertex normals
	if (pcSrc->HasNormals())
	{
		// allocate and copy data
		jfloatArray jfl = pc->NewFloatArray(size2);	
		JNU_CopyDataToArray(pc,jfl,pcSrc->mNormals,size);

		pc->SetObjectField(obj,this->m_vNormals,jfl);	
	}
	// copy tangents and bitangents
	if (pcSrc->HasTangentsAndBitangents())
	{
		jfloatArray jfl = pc->NewFloatArray(size2);	
		JNU_CopyDataToArray(pc,jfl,pcSrc->mTangents,size);

		pc->SetObjectField(obj,this->m_vTangents,jfl);

		jfl = pc->NewFloatArray(size2);	
		JNU_CopyDataToArray(pc,jfl,pcSrc->mBitangents,size);
		pc->SetObjectField(obj,this->m_vBitangents,jfl);
	}
	// copy texture coordinates
	if (pcSrc->HasTextureCoords(0))
	{
		jobjectArray jobjarr = pc->NewObjectArray(AI_MAX_NUMBER_OF_TEXTURECOORDS,
			AIJ_GET_HANDLE(java.lang.Array.FloatArray_Class),0);

		unsigned int channel = 0;
		while (pcSrc->HasTextureCoords(channel))
		{
			jfloatArray jfl = pc->NewFloatArray(size2);	
			JNU_CopyDataToArray(pc,jfl,pcSrc->mTextureCoords[channel],size);

			pc->SetObjectArrayElement(jobjarr,channel,jfl);
			++channel;
		}

		// set the corresponding field in the java object
		pc->SetObjectField(obj,this->m_avUVs,jobjarr);

		jobjarr = (jobjectArray)  pc->NewIntArray(AI_MAX_NUMBER_OF_TEXTURECOORDS);
		pc->SetIntArrayRegion((jintArray)jobjarr,0,channel,(const jint*)&pcSrc->mNumUVComponents);
		pc->SetObjectField(obj,this->m_aiNumUVComponents,jobjarr);
	}
	// copy vertex colors
	if (pcSrc->HasVertexColors(0))
	{
		const unsigned int size = pcSrc->mNumVertices<<4;
		const unsigned int size2 = pcSrc->mNumVertices<<2;

		jobjectArray jobjarr = pc->NewObjectArray(AI_MAX_NUMBER_OF_COLOR_SETS,
			AIJ_GET_HANDLE(java.lang.Array.FloatArray_Class),0);

		unsigned int channel = 0;
		while (pcSrc->HasVertexColors(channel))
		{
			jfloatArray jfl = pc->NewFloatArray(size2);	
			JNU_CopyDataToArray(pc,jfl,pcSrc->mColors[channel],size);

			pc->SetObjectArrayElement(jobjarr,channel,jfl);
			++channel;
		}

		// set the corresponding field in the java object
		pc->SetObjectField(obj,this->m_avColors,jobjarr);
	}


	// copy faces
	if (0 < pcSrc->mNumFaces) // just for safety
	{
		const unsigned int size = pcSrc->mNumFaces*12;
		const unsigned int size2 = pcSrc->mNumFaces*3;

		// allocate and copy data
		jintArray jil = pc->NewIntArray(size2);	

		uint32_t* pf;
		jboolean iscopy = FALSE;

		if(!(pf = (uint32_t*)pc->GetPrimitiveArrayCritical(jil,&iscopy)))
			JNIEnvironment::Get()->ThrowNativeError("Unable to lock face array");

		const aiFace* const pcEnd = pcSrc->mFaces+pcSrc->mNumFaces;
		for (const aiFace* pcCur = pcSrc->mFaces;pcCur != pcEnd;++pcCur)
		{
			ai_assert(3 == pcCur->mNumIndices);
			*pf++ = pcCur->mIndices[0];
			*pf++ = pcCur->mIndices[1];
			*pf++ = pcCur->mIndices[2];
		}

		pc->ReleasePrimitiveArrayCritical(jil,pf,0);
		pc->SetObjectField(obj,this->m_vFaces,jil);
	}

	// copy bones
	if (pcSrc->mNumBones)
	{
		jobjectArray ja;
		JNU_CopyObjectArrayToVM(pc,(const void**)pcSrc->mBones,pcSrc->mNumBones,
			AIJ_GET_HANDLE(assimp.Bone),ja);
		pc->SetObjectField(obj,m_vBones,ja);
	}
}

}}