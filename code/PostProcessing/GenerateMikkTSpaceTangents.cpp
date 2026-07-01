/*
Open Asset Import Library (assimp)
----------------------------------------------------------------------

Copyright (c) 2006-2026, assimp team

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
#include "GenerateMikkTSpaceTangents.h"
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/Exceptional.h>

namespace Assimp {

static int get_num_faces(const SMikkTSpaceContext *context);
static int get_num_vertices_of_face(const SMikkTSpaceContext *context, int iFace);
static void get_position(const SMikkTSpaceContext *context, float outpos[], int iFace, int iVert);
static void get_normal(const SMikkTSpaceContext *context, float outnormal[], int iFace, int iVert);
static void get_tex_coords(const SMikkTSpaceContext *context, float outuv[], int iFace, int iVert);
static void set_tspace_basic(const SMikkTSpaceContext *context, const float tangentu[], float fSign, int iFace, int iVert);

static int get_num_faces(const SMikkTSpaceContext *context) {
    const auto *currentMesh = static_cast<aiMesh*>(context->m_pUserData);
    const auto numFaces = static_cast<int>(currentMesh->mNumFaces);

    return numFaces;
}

static int get_num_vertices_of_face(const SMikkTSpaceContext *context, int iFace) {
    const auto *currentMesh = static_cast<aiMesh*>(context->m_pUserData);
    const aiFace &face = currentMesh->mFaces[iFace];

    return face.mNumIndices;
}

static void get_position(const SMikkTSpaceContext *context, float outpos[], int /*iFace*/, int iVert) {
    const auto *currentMesh = static_cast<aiMesh*>(context->m_pUserData);
    const aiVector3D &v = currentMesh->mVertices[static_cast<size_t>(iVert)];
    outpos[0] = v.x;
    outpos[1] = v.y;
    outpos[2] = v.z;
}

static void get_normal(const SMikkTSpaceContext *context, float outnormal[], int /*iFace*/, int iVert) {
    const auto *currentMesh = static_cast<aiMesh*>(context->m_pUserData);
    const aiVector3D &n = currentMesh->mNormals[static_cast<size_t>(iVert)];
    outnormal[0] = n.x;
    outnormal[1] = n.y;
    outnormal[2] = n.z;
}

static void get_tex_coords(const SMikkTSpaceContext *context, float outuv[], int /*iFace*/, int iVert) {
    const auto *currentMesh = static_cast<aiMesh*>(context->m_pUserData);
    const aiVector3D &t = currentMesh->mTextureCoords[static_cast<size_t>(iVert)][0];
    if (currentMesh->mNumUVComponents[iVert] == 2) {
        outuv[0] = t.x;
        outuv[1] = t.y;
    } else if (currentMesh->mNumUVComponents[iVert] == 3){
        outuv[0] = t.x;
        outuv[1] = t.y;
        outuv[2] = 0.0;
    }
}

static void set_tspace_basic(const SMikkTSpaceContext *context, const float tangentu[], float /*fSign*/, int /*iFace*/, int iVert) {
    const auto *currentMesh = static_cast<aiMesh*>(context->m_pUserData);
    currentMesh->mTangents[iVert].x = tangentu[0];
    currentMesh->mTangents[iVert].y = tangentu[1];
    currentMesh->mTangents[iVert].z = tangentu[2];
}

bool GenerateMikkTSpaceTangents::IsActive(unsigned int pFlags) const {
    const bool active = (pFlags & aiProcess_CalcTangentSpace) != 0;
    if (mActive) {
        return active;
    }
    return false;
}

void GenerateMikkTSpaceTangents::Execute(aiScene* pScene) {
    mIface.m_getNumFaces = get_num_faces;
    mIface.m_getNumVerticesOfFace = get_num_vertices_of_face;
    mIface.m_getNormal = get_normal;
    mIface.m_getPosition = get_position;
    mIface.m_getTexCoord = get_tex_coords;
    mIface.m_setTSpaceBasic = set_tspace_basic;
    mContext.m_pInterface = &mIface;

    for (size_t i=0; i<pScene->mNumMeshes; ++i) {
        aiMesh *mesh = pScene->mMeshes[i];
        ExecutePerMesh(mesh);
    }
}

void GenerateMikkTSpaceTangents::SetupProperties(const Importer *pImp) {
    mActive = pImp->GetPropertyBool(AI_CONFIG_POSTPROCESS_USE_MIKKTSPACE_TANGENTS, false);
}

void GenerateMikkTSpaceTangents::ExecutePerMesh(aiMesh *mesh) {
    if (mesh == nullptr) {
        return;
    }

    mContext.m_pUserData = mesh;
    const tbool result = genTangSpaceDefault(&mContext);
    if (!result) {
        throw DeadlyImportError("MikkTSpace tangent space generation failed");
    }
}

} // namespace Assimp
