/*
 * ScaleToUnitBoxProcess.cpp
 *
 *  Created on: Jan 20, 2015
 *      Author: Alov Maxim <alovmax@yandex.ru>
 */

/// @file ScaleToUnitBoxProcess.cpp
/// Implementation of the ScaleToUnitBox postprocessing step

#include "AssimpPCH.h"

// internal headers of the post-processing framework
#include "ScaleToUnitBoxProcess.h"

using namespace Assimp;

// ------------------------------------------------------------------------------------------------
// Constructor
ScaleToUnitBoxProcess::ScaleToUnitBoxProcess()
{
  // nothing to do here
}

// ------------------------------------------------------------------------------------------------
// Destructor
ScaleToUnitBoxProcess::~ScaleToUnitBoxProcess()
{
  // nothing to do here
}

// ------------------------------------------------------------------------------------------------
// Returns whether the processing step is present in the given flag.
bool ScaleToUnitBoxProcess::IsActive( unsigned int pFlags ) const
{
  return !!(pFlags & aiProcess_ScaleToUnitBox);
}

// ------------------------------------------------------------------------------------------------
// Updates internal properties
void ScaleToUnitBoxProcess::SetupProperties( const Importer* pImp )
{
  // nothing to do here
}

// ------------------------------------------------------------------------------------------------
// Executes the post processing step on the given imported data.
void ScaleToUnitBoxProcess::Execute( aiScene* pScene )
{
  DefaultLogger::get()->debug("ScaleToUnitBoxProcess begin");

  if (pScene->mFlags & AI_SCENE_FLAGS_NON_VERBOSE_FORMAT) {
    throw DeadlyImportError("Post-processing order mismatch: expecting pseudo-indexed (\"verbose\") vertices here");
  }
  ScaleScene(pScene);

  DefaultLogger::get()->info("ScaleToUnitBoxProcess finished. "
        "Mesh vertices have been scaled to unit box");
}

// ------------------------------------------------------------------------------------------------
// Scales all vertices of the given mesh to conform unit box.
void ScaleToUnitBoxProcess::ScaleMesh( const aiMesh* pMesh ) const
{
  aiVector3D center = findCenter(pMesh);
  float radius = findRadius(pMesh, center);
  for (unsigned int i = 0; i < pMesh->mNumVertices; ++i) {
    aiVector3D segment = pMesh->mVertices[i];
    segment -= center;
    segment /= radius;
    segment += center / radius;
    pMesh->mVertices[i].Set(segment.x, segment.y, segment.z);
  }
}

void ScaleToUnitBoxProcess::ScaleScene( aiScene* pScene ) const
{
  aiVector3D center = findCenter(pScene);
  float radius = findRadius(pScene);
  // TODO: adjust root transform, do not change meshes
  for (unsigned int a = 0; a < pScene->mNumMeshes; ++a) {
    aiMesh* pMesh = pScene->mMeshes[a];
    for (unsigned int i = 0; i < pMesh->mNumVertices; ++i) {
      aiVector3D segment = pMesh->mVertices[i];
      segment -= center;
      segment /= radius;
      segment += center / radius;
      pMesh->mVertices[i].Set(segment.x, segment.y, segment.z);
    }
  }
}

aiVector3D ScaleToUnitBoxProcess::findCenter( const aiMesh* pMesh ) const
{
  aiVector3D center = aiVector3D(0.0f, 0.0f, 0.0f);
  for (unsigned int i = 0; i < pMesh->mNumVertices; ++i) {
    center += pMesh->mVertices[i];
  }
  center /= float(pMesh->mNumVertices);
  return center;
}

aiVector3D ScaleToUnitBoxProcess::findCenter( const aiScene* pScene ) const
{
  aiVector3D scene_center = aiVector3D(0.0f, 0.0f, 0.0f);
  for (unsigned int a = 0; a < pScene->mNumMeshes; ++a) {
    aiVector3D center = this->findCenter(pScene->mMeshes[a]);
    scene_center += center;
  }
  scene_center /= float(pScene->mNumMeshes);
  return scene_center;
}

float ScaleToUnitBoxProcess::findRadius( const aiMesh* pMesh, const aiVector3D& center ) const
{
  float radius = 0.0f;
  for (unsigned int i = 0; i < pMesh->mNumVertices; ++i) {
    aiVector3D distanceValues = center;
    distanceValues -= pMesh->mVertices[i];
    float vDistance = distanceValues.Length();
    if (vDistance > radius) {
      radius = vDistance;
    }
  }
  return radius;
}

float ScaleToUnitBoxProcess::findRadius( const aiScene* pScene ) const
{
  aiVector3D center = findCenter(pScene);
  float radius = 0.0f;
  for (unsigned int a = 0; a < pScene->mNumMeshes; ++a) {
    const aiMesh* pMesh = pScene->mMeshes[a];
    for (unsigned int i = 0; i < pMesh->mNumVertices; ++i) {
      aiVector3D distanceValues = center;
      distanceValues -= pMesh->mVertices[i];
      float vDistance = distanceValues.Length();
      if (vDistance > radius) {
        radius = vDistance;
      }
    }
  }
  return radius;
}

