
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
void JNIEnvironment::_assimp::_Scene::Initialize()
{
	AIJ_LOAD_CLASS();

	AIJ_LOAD_FIELD(m_rootNode);
	AIJ_LOAD_FIELD(m_vAnimations);
	AIJ_LOAD_FIELD(m_vMaterials);
	AIJ_LOAD_FIELD(m_vMeshes);
	AIJ_LOAD_FIELD(m_vTextures);
	AIJ_LOAD_FIELD(m_vLights);
	AIJ_LOAD_FIELD(m_vCameras);
	AIJ_LOAD_FIELD(flags);
}

// ------------------------------------------------------------------------------------------------
void JNIEnvironment::_assimp::_Scene::Fill(jobject obj,const aiScene* pcSrc)
{
	JNIEnv* pc = JNIEnvironment::Get()->GetThread()->m_pcEnv;
	jobjectArray ja;

	// copy meshes
	if (pcSrc->mNumMeshes)	{
		JNU_CopyObjectArrayToVM(pc,(const void**)pcSrc->mMeshes,pcSrc->mNumMeshes,
			AIJ_GET_HANDLE(assimp.Mesh),ja);
		pc->SetObjectField(obj,m_vMeshes,ja);
	}

	// copy textures
	if (pcSrc->mNumTextures)	{
		JNU_CopyObjectArrayToVM(pc,(const void**)pcSrc->mTextures,pcSrc->mNumTextures,
			AIJ_GET_HANDLE(assimp.Texture),ja);
		pc->SetObjectField(obj,m_vTextures,ja);
	}

	// copy materials
	if (pcSrc->mNumMeshes)	{
		JNU_CopyObjectArrayToVM(pc,(const void**)pcSrc->mMaterials,pcSrc->mNumMaterials,
			AIJ_GET_HANDLE(assimp.Material),ja);
		pc->SetObjectField(obj,m_vMaterials,ja);
	}

	// copy animations
	if (pcSrc->mNumAnimations)	{
		JNU_CopyObjectArrayToVM(pc,(const void**)pcSrc->mAnimations,pcSrc->mNumAnimations,
			AIJ_GET_HANDLE(assimp.Animation),ja);
		pc->SetObjectField(obj,m_vAnimations,ja);
	}

	// copy lights
	if (pcSrc->mNumLights)	{
		JNU_CopyObjectArrayToVM(pc,(const void**)pcSrc->mLights,pcSrc->mNumLights,
			AIJ_GET_HANDLE(assimp.Light),ja);
		pc->SetObjectField(obj,m_vLights,ja);
	}

	// copy cameras
	if (pcSrc->mNumCameras)	{
		JNU_CopyObjectArrayToVM(pc,(const void**)pcSrc->mCameras,pcSrc->mNumCameras,
			AIJ_GET_HANDLE(assimp.Camera),ja);
		pc->SetObjectField(obj,m_vCameras,ja);
	}

	// copy scene flags
	pc->SetIntField(obj,flags,(jint)pcSrc->mFlags);

	// copy the root node
	jobject root = pc->NewObject(AIJ_GET_CLASS_HANDLE(assimp.Node),
		AIJ_GET_DEFAULT_CTOR_HANDLE(assimp.Node));

	AIJ_GET_HANDLE(assimp.Node).Fill(root,pcSrc->mRootNode);
	pc->SetObjectField(obj,m_rootNode,root);
}
