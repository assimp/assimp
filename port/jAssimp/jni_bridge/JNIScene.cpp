/*
---------------------------------------------------------------------------
Open Asset Import Library (ASSIMP)
---------------------------------------------------------------------------

Copyright (c) 2006-2008, ASSIMP Development Team

All rights reserved.
See the disclaimer in JNIEnvironment.h for licensing and distribution 
conditions.
---------------------------------------------------------------------------
*/

/** @file Implementation of the JNI API for jAssimp */

#include "JNIEnvironment.h"
#include "JNILogger.h"

using namespace Assimp;

namespace Assimp	{
namespace JNIBridge		{

// ------------------------------------------------------------------------------------------------
void JNIEnvironment::_assimp::_Scene::Initialize()
{
	JNIEnv* pc = JNIEnvironment::Get()->GetThread()->m_pcEnv;

	// load a handle to the class
	if(!(Class = pc->FindClass("assimp.Scene")))
		JNIEnvironment::Get()->ThrowNativeError("Unable to load class assimp.Scene");

	DefaultCtor = pc->GetMethodID(Class,"<init>","");

	// load all fields of the class
	m_rootNode		= pc->GetFieldID(Class,"m_rootNode","Lassimp.Node;");
	m_vAnimations	= pc->GetFieldID(Class,"m_vAnimations","[Lassimp.Animation;");
	m_vMaterials	= pc->GetFieldID(Class,"m_vMaterials","[Lassimp.Material;");
	m_vMeshes		= pc->GetFieldID(Class,"m_vMeshes","[Lassimp.Mesh;");
	m_vTextures		= pc->GetFieldID(Class,"m_vTextures","[Lassimp.Texture;");
	flags			= pc->GetFieldID(Class,"flags","I");


	// check whether all fields have been loaded properly
	if (!m_vAnimations || !m_rootNode || !m_vMaterials || !m_vMeshes || !m_vTextures || !flags)
		JNIEnvironment::Get()->ThrowNativeError("Unable to load all fields of class assimp.Scene");
}
// ------------------------------------------------------------------------------------------------
void JNIEnvironment::_assimp::_Scene::Fill(jobject obj,const aiScene* pcSrc)
{
	JNIEnv* pc = JNIEnvironment::Get()->GetThread()->m_pcEnv;
	jobjectArray ja;

	// copy meshes
	if (pcSrc->mNumMeshes)
	{
		JNU_CopyObjectArrayToVM(pc,(const void**)pcSrc->mMeshes,pcSrc->mNumMeshes,
			AIJ_GET_HANDLE(assimp.Mesh),ja);
		pc->SetObjectField(obj,m_vMeshes,ja);
	}

	// copy textures
	if (pcSrc->mNumTextures)
	{
		JNU_CopyObjectArrayToVM(pc,(const void**)pcSrc->mTextures,pcSrc->mNumTextures,
			AIJ_GET_HANDLE(assimp.Texture),ja);
		pc->SetObjectField(obj,m_vTextures,ja);
	}

	// copy materials
	if (pcSrc->mNumMeshes)
	{
		JNU_CopyObjectArrayToVM(pc,(const void**)pcSrc->mMaterials,pcSrc->mNumMaterials,
			AIJ_GET_HANDLE(assimp.Material),ja);
		pc->SetObjectField(obj,m_vMaterials,ja);
	}

	// copy animations
	if (pcSrc->mNumAnimations)
	{
		JNU_CopyObjectArrayToVM(pc,(const void**)pcSrc->mAnimations,pcSrc->mNumAnimations,
			AIJ_GET_HANDLE(assimp.Animation),ja);
		pc->SetObjectField(obj,m_vAnimations,ja);
	}

	// copy flags
	pc->SetIntField(obj,flags,(jint)pcSrc->mFlags);

	// copy the root node
	jobject root = pc->NewObject(AIJ_GET_CLASS_HANDLE(assimp.Node),
		AIJ_GET_DEFAULT_CTOR_HANDLE(assimp.Node));

	AIJ_GET_HANDLE(assimp.Node).Fill(root,pcSrc->mRootNode);
	pc->SetObjectField(obj,m_rootNode,root);
}
}}