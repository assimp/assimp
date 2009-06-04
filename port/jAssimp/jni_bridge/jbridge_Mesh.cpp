
/** @file Implementation of the JNI API for jAssimp */

/* --------------------------------------------------------------------------------
 *
 * Open Asset Import Library (ASSIMP) (http://assimp.sourceforge.net)
 * Assimp2Java bridge 
 *
 * Copyright (c) 2006-2009, ASSIMP Development Team
 * All rights reserved. See the LICENSE file for more information.
 *
 * --------------------------------------------------------------------------------
 */

#include "jbridge_pch.h"
using namespace Assimp;


// ------------------------------------------------------------------------------------------------
void JNIEnvironment::_assimp::_Mesh::Initialize()
{
	AIJ_LOAD_CLASS();

	AIJ_LOAD_FIELD(m_iPrimitiveTypes);
	AIJ_LOAD_FIELD(m_vVertices);
	AIJ_LOAD_FIELD(m_vBitangents);
	AIJ_LOAD_FIELD(m_vTangents);
	AIJ_LOAD_FIELD(m_vNormals);
	AIJ_LOAD_FIELD(m_avUVs);
	AIJ_LOAD_FIELD(m_vFaces);
	AIJ_LOAD_FIELD(m_avColors);
	AIJ_LOAD_FIELD(m_aiNumUVComponents);
	AIJ_LOAD_FIELD(m_vBones);
	AIJ_LOAD_FIELD(m_iMaterialIndex);
}
// ------------------------------------------------------------------------------------------------
void JNIEnvironment::_assimp::_Mesh::Fill(jobject obj,const aiMesh* pcSrc)
{
	jobjectArray ja;
	JNIEnv* pc = JNIEnvironment::Get()->GetThread()->m_pcEnv;

	pc->SetIntField(obj,m_iMaterialIndex,pcSrc->mMaterialIndex);
	const unsigned int vsize = pcSrc->mNumVertices*12, nsize = pcSrc->mNumVertices*3;

	// copy vertex positions
	if (pcSrc->HasPositions())	{
		jfloatArray jfl = pc->NewFloatArray(nsize);	
		JNU_CopyDataToArray(pc,jfl,pcSrc->mVertices,vsize);

		pc->SetObjectField(obj,m_vVertices,jfl);
	}

	// copy vertex normals
	if (pcSrc->HasNormals())	{
		jfloatArray jfl = pc->NewFloatArray(nsize);	
		JNU_CopyDataToArray(pc,jfl,pcSrc->mNormals,vsize);

		pc->SetObjectField(obj,m_vNormals,jfl);
	}

	// copy tangents and bitangents
	if (pcSrc->HasTangentsAndBitangents())	{
		jfloatArray jfl = pc->NewFloatArray(nsize);	
		JNU_CopyDataToArray(pc,jfl,pcSrc->mTangents,vsize);
		pc->SetObjectField(obj,m_vTangents,jfl);


		jfl = pc->NewFloatArray(nsize);	
		JNU_CopyDataToArray(pc,jfl,pcSrc->mBitangents,vsize);
		pc->SetObjectField(obj,m_vBitangents,jfl);
	}


	// copy texture coordinates
	if (pcSrc->HasTextureCoords(0))	{
		jobjectArray jobjarr = pc->NewObjectArray(AI_MAX_NUMBER_OF_TEXTURECOORDS,
			AIJ_GET_HANDLE(java.lang.Array.FloatArray_Class),0);

		unsigned int channel = 0;
		while (pcSrc->HasTextureCoords(channel))	{
			jfloatArray jfl = pc->NewFloatArray(nsize);	
			JNU_CopyDataToArray(pc,jfl,pcSrc->mTextureCoords[channel],vsize);

			pc->SetObjectArrayElement(jobjarr,channel,jfl);
			++channel;
		}

		// set the corresponding field in the java object
		pc->SetObjectField(obj,m_avUVs,jobjarr);

		jobjarr = (jobjectArray)  pc->NewIntArray(AI_MAX_NUMBER_OF_TEXTURECOORDS);
		pc->SetIntArrayRegion((jintArray)jobjarr,0,channel,(const jint*)&pcSrc->mNumUVComponents);
		pc->SetObjectField(obj,m_aiNumUVComponents,jobjarr);
	}
	// copy vertex colors
	if (pcSrc->HasVertexColors(0))	{
		jobjectArray jobjarr = pc->NewObjectArray(AI_MAX_NUMBER_OF_COLOR_SETS,
			AIJ_GET_HANDLE(java.lang.Array.FloatArray_Class),0);

		unsigned int channel = 0;
		while (pcSrc->HasVertexColors(channel))	{

			jfloatArray jfl = pc->NewFloatArray(pcSrc->mNumVertices*4);	
			JNU_CopyDataToArray(pc,jfl,pcSrc->mColors[channel],pcSrc->mNumVertices*16);

			pc->SetObjectArrayElement(jobjarr,channel,jfl);
			++channel;
		}

		// set the corresponding field in the java object
		pc->SetObjectField(obj,m_avColors,jobjarr);
	}

	// copy faces
	if (pcSrc->mNumFaces) {
		JNU_CopyObjectArrayToVM(pc,(const void**)pcSrc->mFaces,pcSrc->mNumFaces,
			AIJ_GET_HANDLE(assimp.Face),ja);
		pc->SetObjectField(obj,m_vFaces,ja);
	}

	// copy bones
	if (pcSrc->mNumBones)	{

		JNU_CopyObjectArrayToVM(pc,(const void**)pcSrc->mBones,pcSrc->mNumBones,
			AIJ_GET_HANDLE(assimp.Bone),ja);
		pc->SetObjectField(obj,m_vBones,ja);
	}
}

