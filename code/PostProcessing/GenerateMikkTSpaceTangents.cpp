#include "GenerateMikkTSpaceTangents.h"
#include <assimp/scene.h>
#include <assimp/postprocess.h>

namespace Assimp {

static int get_vertex_index(const SMikkTSpaceContext *context, int iFace, int iVert);
static int get_num_faces(const SMikkTSpaceContext *context);
static int get_num_vertices_of_face(const SMikkTSpaceContext *context, int iFace);
static void get_position(const SMikkTSpaceContext *context, float outpos[], int iFace, int iVert);
static void get_normal(const SMikkTSpaceContext *context, float outnormal[], int iFace, int iVert);
static void get_tex_coords(const SMikkTSpaceContext *context, float outuv[], int iFace, int iVert);
static void set_tspace_basic(const SMikkTSpaceContext *context, const float tangentu[], float fSign, int iFace, int iVert);

static int get_num_faces(const SMikkTSpaceContext *context) {
    aiMesh *currentMesh = static_cast<aiMesh*>(context->m_pUserData);
    const int numFaces = static_cast<int>(currentMesh->mNumFaces);

    return numFaces;
}

static int get_num_vertices_of_face(const SMikkTSpaceContext *context, int iFace) {
    aiMesh *currentMesh = static_cast<aiMesh*>(context->m_pUserData);
    const aiFace &face = currentMesh->mFaces[iFace];
    return face.mNumIndices;
}

static void get_position(const SMikkTSpaceContext *context, float outpos[], int iFace, int iVert) {
    aiMesh *currentMesh = static_cast<aiMesh*>(context->m_pUserData);
    const aiVector3D &v = currentMesh->mVertices[static_cast<size_t>(iVert)];
    outpos[0] = v.x;
    outpos[1] = v.y;
    outpos[2] = v.z;
}

static void get_normal(const SMikkTSpaceContext *context, float outnormal[], int iFace, int iVert) {
    aiMesh *currentMesh = static_cast<aiMesh*>(context->m_pUserData);
    const aiVector3D &n = currentMesh->mNormals[static_cast<size_t>(iVert)];
    outnormal[0] = n.x;
    outnormal[0] = n.y;
    outnormal[0] = n.z;
}

static void get_tex_coords(const SMikkTSpaceContext *context, float outuv[], int iFace, int iVert) {
    aiMesh *currentMesh = static_cast<aiMesh*>(context->m_pUserData);
    aiVector3D &t = currentMesh->mTextureCoords[static_cast<size_t>(iVert)][0];
    if (currentMesh->mNumUVComponents[iVert] == 2) {
        outuv[0] = t.x;
        outuv[1] = t.y;
    } else if (currentMesh->mNumUVComponents[iVert] == 3){
        outuv[0] = t.x;
        outuv[1] = t.y;
        outuv[2] = t.z;
    }
}

static void set_tspace_basic(const SMikkTSpaceContext *context, const float tangentu[], float fSign, int iFace, int iVert) {
    aiMesh *currentMesh = static_cast<aiMesh*>(context->m_pUserData);
    currentMesh->mTangents[iVert].x = tangentu[0];
    currentMesh->mTangents[iVert].y = tangentu[1];
    currentMesh->mTangents[iVert].z = tangentu[1];
}

bool GenerateMikkTSpaceTangents::IsActive(unsigned int pFlags) const {
    const bool active = ((pFlags & aiProcess_CalcTangentSpace) != 0);
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
    genTangSpaceDefault(&mContext);
}

} // namespace Assimp
