/*
Open Asset Import Library (assimp)
----------------------------------------------------------------------

Copyright (c) 2006-2024, assimp team
Copyright (c) 2019 bzt

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

#ifndef ASSIMP_BUILD_NO_M3D_IMPORTER

#define M3D_IMPLEMENTATION
#define M3D_NONORMALS /* leave the post-processing to Assimp */
#define M3D_NOWEIGHTS
#define M3D_NOANIMATION

#include <assimp/DefaultIOSystem.h>
#include <assimp/IOStreamBuffer.h>
#include <assimp/ai_assert.h>
#include <assimp/importerdesc.h>
#include <assimp/scene.h>
#include <assimp/DefaultLogger.hpp>
#include <assimp/Importer.hpp>
#include <memory>

#include "M3DImporter.h"
#include "M3DMaterials.h"
#include "M3DWrapper.h"

// RESOURCES:
// https://gitlab.com/bztsrc/model3d/blob/master/docs/m3d_format.md
// https://gitlab.com/bztsrc/model3d/blob/master/docs/a3d_format.md

/*
 Unfortunately aiNode has bone structures and meshes too, yet we can't assign
 the mesh to a bone aiNode as a skin may refer to several aiNodes. Therefore
 I've decided to import into this structure:

   aiScene->mRootNode
    |        |->mMeshes (all the meshes)
    |        \->children (empty if there's no skeleton imported, no meshes)
    |             \->skeleton root aiNode*
    |                   |->bone aiNode
    |                   |   \->subbone aiNode
    |                   |->bone aiNode
    |                   |   ...
    |                   \->bone aiNode
    \->mMeshes[]
        \->aiBone, referencing mesh-less aiNodes from above

  * - normally one, but if a model has several skeleton roots, then all of them
      are listed in aiScene->mRootNode->children, but all without meshes
*/

static constexpr aiImporterDesc desc = {
    "Model 3D Importer",
    "",
    "",
    "",
    aiImporterFlags_SupportTextFlavour | aiImporterFlags_SupportBinaryFlavour,
    0,
    0,
    0,
    0,
    "m3d a3d"
};

namespace Assimp {

using namespace std;

// ------------------------------------------------------------------------------------------------
//  Default constructor
M3DImporter::M3DImporter() :
        mScene(nullptr) {
    // empty
}

// ------------------------------------------------------------------------------------------------
//  Returns true, if file is a binary or ASCII Model 3D file.
bool M3DImporter::CanRead(const std::string &pFile, IOSystem *pIOHandler, bool /*checkSig*/) const {
    // don't use CheckMagicToken because that checks with swapped bytes too, leading to false
    // positives. This magic is not uint32_t, but char[4], so memcmp is the best way
    std::unique_ptr<IOStream> pStream(pIOHandler->Open(pFile, "rb"));
    unsigned char data[4];
    if (4 != pStream->Read(data, 1, 4)) {
        return false;
    }
    return !memcmp(data, "3DMO", 4) /* bin */
#ifdef M3D_ASCII
        || !memcmp(data, "3dmo", 4) /* ASCII */
#endif
            ;
}

// ------------------------------------------------------------------------------------------------
const aiImporterDesc *M3DImporter::GetInfo() const {
    return &desc;
}

// ------------------------------------------------------------------------------------------------
//  Model 3D import implementation
void M3DImporter::InternReadFile(const std::string &file, aiScene *pScene, IOSystem *pIOHandler) {
    // Read file into memory
    std::unique_ptr<IOStream> pStream(pIOHandler->Open(file, "rb"));
    if (!pStream.get()) {
        throw DeadlyImportError("Failed to open file ", file, ".");
    }

    // Get the file-size and validate it, throwing an exception when fails
    size_t fileSize = pStream->FileSize();
    if (fileSize < 8) {
        throw DeadlyImportError("M3D-file ", file, " is too small.");
    }
    std::vector<unsigned char> buffer(fileSize);
    if (fileSize != pStream->Read(buffer.data(), 1, fileSize)) {
        throw DeadlyImportError("Failed to read the file ", file, ".");
    }
    // extra check for binary format's first 8 bytes. Not done for the ASCII variant
    if (!memcmp(buffer.data(), "3DMO", 4) && memcmp(buffer.data() + 4, &fileSize, 4)) {
        throw DeadlyImportError("Bad binary header in file ", file, ".");
    }
    // make sure there's a terminator zero character, as input must be ASCIIZ
    if (!memcmp(buffer.data(), "3dmo", 4)) {
        buffer.push_back(0);
    }

    // Get the path for external assets
    std::string folderName("./");
    std::string::size_type pos = file.find_last_of("\\/");
    if (pos != std::string::npos) {
        folderName = file.substr(0, pos);
        if (!folderName.empty()) {
            pIOHandler->PushDirectory(folderName);
        }
    }

    //DefaultLogger::create("/dev/stderr", Logger::VERBOSE);
    ASSIMP_LOG_DEBUG("M3D: loading ", file);

    // let the C SDK do the hard work for us
    M3DWrapper m3d(pIOHandler, buffer);

    if (!m3d) {
        throw DeadlyImportError("Unable to parse ", file, " as M3D.");
    }

    // create the root node
    pScene->mRootNode = new aiNode;
    pScene->mRootNode->mName = aiString(m3d.Name());
    pScene->mRootNode->mTransformation = aiMatrix4x4();
    pScene->mRootNode->mNumChildren = 0;
    mScene = pScene;

    ASSIMP_LOG_DEBUG("M3D: root node ", m3d.Name());

    // now we just have to fill up the Assimp structures in pScene
    importMaterials(m3d);
    importTextures(m3d);
    importBones(m3d, M3D_NOTDEFINED, pScene->mRootNode);
    importMeshes(m3d);
    importAnimations(m3d);

    // Pop directory stack
    if (pIOHandler->StackSize() > 0) {
        pIOHandler->PopDirectory();
    }
}

// ------------------------------------------------------------------------------------------------
// convert materials. properties are converted using a static table in M3DMaterials.h
void M3DImporter::importMaterials(const M3DWrapper &m3d) {
    unsigned int i, j, k, l, n;
    m3dm_t *m;
    aiString name = aiString(AI_DEFAULT_MATERIAL_NAME);
    aiColor4D c;
    ai_real f;

    ai_assert(mScene != nullptr);
    ai_assert(m3d);

    mScene->mNumMaterials = m3d->nummaterial + 1;
    mScene->mMaterials = new aiMaterial *[mScene->mNumMaterials];

    ASSIMP_LOG_DEBUG("M3D: importMaterials ", mScene->mNumMaterials);

    // add a default material as first
    aiMaterial *defaultMat = new aiMaterial;
    defaultMat->AddProperty(&name, AI_MATKEY_NAME);
    c.a = 1.0f;
    c.b = c.g = c.r = 0.6f;
    defaultMat->AddProperty(&c, 1, AI_MATKEY_COLOR_DIFFUSE);
    mScene->mMaterials[0] = defaultMat;

    if (!m3d->nummaterial || !m3d->material) {
        return;
    }

    for (i = 0; i < m3d->nummaterial; i++) {
        m = &m3d->material[i];
        aiMaterial *newMat = new aiMaterial;
        name.Set(std::string(m->name));
        newMat->AddProperty(&name, AI_MATKEY_NAME);
        for (j = 0; j < m->numprop; j++) {
            // look up property type
            // 0 - 127 scalar values,
            // 128 - 255 the same properties but for texture maps
            k = 256;
            for (l = 0; l < sizeof(m3d_propertytypes) / sizeof(m3d_propertytypes[0]); l++)
                if (m->prop[j].type == m3d_propertytypes[l].id ||
                        m->prop[j].type == m3d_propertytypes[l].id + 128) {
                    k = l;
                    break;
                }
            // should never happen, but be safe than sorry
            if (k == 256)
                continue;

            // scalar properties
            if (m->prop[j].type < 128 && aiProps[k].pKey) {
                switch (m3d_propertytypes[k].format) {
                    case m3dpf_color:
                        c = mkColor(m->prop[j].value.color);
                        newMat->AddProperty(&c, 1, aiProps[k].pKey, aiProps[k].type, aiProps[k].index);
                        break;
                    case m3dpf_float:
                        f = m->prop[j].value.fnum;
                        newMat->AddProperty(&f, 1, aiProps[k].pKey, aiProps[k].type, aiProps[k].index);
                        break;
                    default:
                        n = m->prop[j].value.num;
                        if (m->prop[j].type == m3dp_il) {
                            switch (n) {
                                case 0:
                                    n = aiShadingMode_NoShading;
                                    break;
                                case 2:
                                    n = aiShadingMode_Phong;
                                    break;
                                default:
                                    n = aiShadingMode_Gouraud;
                                    break;
                            }
                        }
                        newMat->AddProperty(&n, 1, aiProps[k].pKey, aiProps[k].type, aiProps[k].index);
                        break;
                }
            }
            // texture map properties
            if (m->prop[j].type >= 128 && aiTxProps[k].pKey &&
                    // extra check, should never happen, do we have the referred texture?
                    m->prop[j].value.textureid < m3d->numtexture &&
                    m3d->texture[m->prop[j].value.textureid].name) {
                name.Set(std::string(std::string(m3d->texture[m->prop[j].value.textureid].name) + ".png"));
                newMat->AddProperty(&name, aiTxProps[k].pKey, aiTxProps[k].type, aiTxProps[k].index);
                n = 0;
                newMat->AddProperty(&n, 1, _AI_MATKEY_UVWSRC_BASE, aiProps[k].type, aiProps[k].index);
            }
        }
        mScene->mMaterials[i + 1] = newMat;
    }
}

// ------------------------------------------------------------------------------------------------
// import textures, this is the simplest of all
void M3DImporter::importTextures(const M3DWrapper &m3d) {
    unsigned int i;
    const char *formatHint[] = {
        "rgba0800",
        "rgba0808",
        "rgba8880",
        "rgba8888"
    };
    m3dtx_t *t;

    ai_assert(mScene != nullptr);
    ai_assert(m3d);

    mScene->mNumTextures = m3d->numtexture;
    ASSIMP_LOG_DEBUG("M3D: importTextures ", mScene->mNumTextures);

    if (!m3d->numtexture || !m3d->texture) {
        return;
    }

    mScene->mTextures = new aiTexture *[m3d->numtexture];
    for (i = 0; i < m3d->numtexture; i++) {
        unsigned int j, k;
        t = &m3d->texture[i];
        aiTexture *tx = new aiTexture;
        tx->mFilename = aiString(std::string(t->name) + ".png");
        if (!t->w || !t->h || !t->f || !t->d) {
            /* without ASSIMP_USE_M3D_READFILECB, we only have the filename, but no texture data ever */
            tx->mWidth = 0;
            tx->mHeight = 0;
            memcpy(tx->achFormatHint, "png\000", 4);
            tx->pcData = nullptr;
        } else {
            /* if we have the texture loaded, set format hint and pcData too */
            tx->mWidth = t->w;
            tx->mHeight = t->h;
            strcpy(tx->achFormatHint, formatHint[t->f - 1]);
            tx->pcData = new aiTexel[tx->mWidth * tx->mHeight];
            for (j = k = 0; j < tx->mWidth * tx->mHeight; j++) {
                switch (t->f) {
                    case 1: tx->pcData[j].g = t->d[k++]; break;
                    case 2:
                        tx->pcData[j].g = t->d[k++];
                        tx->pcData[j].a = t->d[k++];
                        break;
                    case 3:
                        tx->pcData[j].r = t->d[k++];
                        tx->pcData[j].g = t->d[k++];
                        tx->pcData[j].b = t->d[k++];
                        tx->pcData[j].a = 255;
                        break;
                    case 4:
                        tx->pcData[j].r = t->d[k++];
                        tx->pcData[j].g = t->d[k++];
                        tx->pcData[j].b = t->d[k++];
                        tx->pcData[j].a = t->d[k++];
                        break;
                }
            }
        }
        mScene->mTextures[i] = tx;
    }
}

// ------------------------------------------------------------------------------------------------
// this is tricky. M3D has a global vertex and UV list, and faces are indexing them
// individually. In assimp there're per mesh vertex and UV lists, and they must be
// indexed simultaneously.
void M3DImporter::importMeshes(const M3DWrapper &m3d) {
    ASSIMP_LOG_DEBUG("M3D: importMeshes ", m3d->numface);

    if (!m3d->numface || !m3d->face || !m3d->numvertex || !m3d->vertex) {
        return;
    }

    unsigned int i, j, k, l, numpoly = 3, lastMat = M3D_INDEXMAX;
    std::vector<aiMesh *> *meshes = new std::vector<aiMesh *>();
    std::vector<aiFace> *faces = nullptr;
    std::vector<aiVector3D> *vertices = nullptr;
    std::vector<aiVector3D> *normals = nullptr;
    std::vector<aiVector3D> *texcoords = nullptr;
    std::vector<aiColor4D> *colors = nullptr;
    std::vector<unsigned int> *vertexids = nullptr;
    aiMesh *pMesh = nullptr;

    ai_assert(mScene != nullptr);
    ai_assert(m3d);
    ai_assert(mScene->mRootNode != nullptr);

    for (i = 0; i < m3d->numface; i++) {
        // we must switch mesh if material changes
        if (lastMat != m3d->face[i].materialid) {
            lastMat = m3d->face[i].materialid;
            if (pMesh && vertices && vertices->size() && faces && faces->size()) {
                populateMesh(m3d, pMesh, faces, vertices, normals, texcoords, colors, vertexids);
                meshes->push_back(pMesh);
                delete faces;
                delete vertices;
                delete normals;
                delete texcoords;
                delete colors;
                delete vertexids; // this is not stored in pMesh, just to collect bone vertices
            }
            pMesh = new aiMesh;
            pMesh->mPrimitiveTypes = aiPrimitiveType_TRIANGLE;
            pMesh->mMaterialIndex = lastMat + 1;
            faces = new std::vector<aiFace>();
            vertices = new std::vector<aiVector3D>();
            normals = new std::vector<aiVector3D>();
            texcoords = new std::vector<aiVector3D>();
            colors = new std::vector<aiColor4D>();
            vertexids = new std::vector<unsigned int>();
        }
        // add a face to temporary vector
        aiFace *pFace = new aiFace;
        pFace->mNumIndices = numpoly;
        pFace->mIndices = new unsigned int[numpoly];
        for (j = 0; j < numpoly; j++) {
            aiVector3D pos, uv, norm;
            k = static_cast<unsigned int>(vertices->size());
            pFace->mIndices[j] = k;
            l = m3d->face[i].vertex[j];
            if (l >= m3d->numvertex) continue;
            pos.x = m3d->vertex[l].x;
            pos.y = m3d->vertex[l].y;
            pos.z = m3d->vertex[l].z;
            vertices->push_back(pos);
            colors->push_back(mkColor(m3d->vertex[l].color));
            // add a bone to temporary vector
            if (m3d->vertex[l].skinid != M3D_UNDEF && m3d->vertex[l].skinid != M3D_INDEXMAX && m3d->skin && m3d->bone) {
                // this is complicated, because M3D stores a list of bone id / weight pairs per
                // vertex but assimp uses lists of local vertex id/weight pairs per local bone list
                vertexids->push_back(l);
            }
            l = m3d->face[i].texcoord[j];
            if (l != M3D_UNDEF && l < m3d->numtmap) {
                uv.x = m3d->tmap[l].u;
                uv.y = m3d->tmap[l].v;
                uv.z = 0.0;
                texcoords->push_back(uv);
            }
            l = m3d->face[i].normal[j];
            if (l != M3D_UNDEF && l < m3d->numvertex) {
                norm.x = m3d->vertex[l].x;
                norm.y = m3d->vertex[l].y;
                norm.z = m3d->vertex[l].z;
                normals->push_back(norm);
            }
        }
        faces->push_back(*pFace);
        delete pFace;
    }
    // if there's data left in the temporary vectors, flush them
    if (pMesh && vertices->size() && faces->size()) {
        populateMesh(m3d, pMesh, faces, vertices, normals, texcoords, colors, vertexids);
        meshes->push_back(pMesh);
    }

    // create global mesh list in scene
    mScene->mNumMeshes = static_cast<unsigned int>(meshes->size());
    mScene->mMeshes = new aiMesh *[mScene->mNumMeshes];
    std::copy(meshes->begin(), meshes->end(), mScene->mMeshes);

    // create mesh indices in root node
    mScene->mRootNode->mNumMeshes = static_cast<unsigned int>(meshes->size());
    mScene->mRootNode->mMeshes = new unsigned int[meshes->size()];
    for (i = 0; i < meshes->size(); i++) {
        mScene->mRootNode->mMeshes[i] = i;
    }

    delete meshes;
    if (faces) delete faces;
    if (vertices) delete vertices;
    if (normals) delete normals;
    if (texcoords) delete texcoords;
    if (colors) delete colors;
    if (vertexids) delete vertexids;
}

// ------------------------------------------------------------------------------------------------
// a reentrant node parser. Otherwise this is simple
void M3DImporter::importBones(const M3DWrapper &m3d, unsigned int parentid, aiNode *pParent) {
    unsigned int i, n;

    ai_assert(pParent != nullptr);
    ai_assert(mScene != nullptr);
    ai_assert(m3d);

    ASSIMP_LOG_DEBUG("M3D: importBones ", m3d->numbone, " parentid ", (int)parentid);

    if (!m3d->numbone || !m3d->bone) {
        return;
    }

    for (n = 0, i = parentid + 1; i < m3d->numbone; i++) {
        if (m3d->bone[i].parent == parentid) {
            n++;
        }
    }
    pParent->mChildren = new aiNode *[n];

    for (i = parentid + 1; i < m3d->numbone; i++) {
        if (m3d->bone[i].parent == parentid) {
            aiNode *pChild = new aiNode;
            pChild->mParent = pParent;
            pChild->mName = aiString(std::string(m3d->bone[i].name));
            convertPose(m3d, &pChild->mTransformation, m3d->bone[i].pos, m3d->bone[i].ori);
            pChild->mNumChildren = 0;
            pParent->mChildren[pParent->mNumChildren] = pChild;
            pParent->mNumChildren++;
            importBones(m3d, i, pChild);
        }
    }
}

// ------------------------------------------------------------------------------------------------
// this is another headache. M3D stores list of changed bone id/position/orientation triplets and
// a timestamp per frame, but assimp needs timestamp and lists of position, orientation lists per
// bone, so we have to convert between the two conceptually different representation forms
void M3DImporter::importAnimations(const M3DWrapper &m3d) {
    unsigned int i, j, k, l, pos, ori;
    double t;
    m3da_t *a;

    ai_assert(mScene != nullptr);
    ai_assert(m3d);

    mScene->mNumAnimations = m3d->numaction;

    ASSIMP_LOG_DEBUG("M3D: importAnimations ", mScene->mNumAnimations);

    if (!m3d->numaction || !m3d->action || !m3d->numbone || !m3d->bone || !m3d->vertex) {
        return;
    }

    mScene->mAnimations = new aiAnimation *[m3d->numaction];
    for (i = 0; i < m3d->numaction; i++) {
        a = &m3d->action[i];
        aiAnimation *pAnim = new aiAnimation;
        pAnim->mName = aiString(std::string(a->name));
        pAnim->mDuration = ((double)a->durationmsec) / 10;
        pAnim->mTicksPerSecond = 100;
        // now we know how many bones are referenced in this animation
        pAnim->mNumChannels = m3d->numbone;
        pAnim->mChannels = new aiNodeAnim *[pAnim->mNumChannels];
        for (l = 0; l < m3d->numbone; l++) {
            unsigned int n;
            pAnim->mChannels[l] = new aiNodeAnim;
            pAnim->mChannels[l]->mNodeName = aiString(std::string(m3d->bone[l].name));
            // now n is the size of positions / orientations arrays
            pAnim->mChannels[l]->mNumPositionKeys = pAnim->mChannels[l]->mNumRotationKeys = a->numframe;
            pAnim->mChannels[l]->mPositionKeys = new aiVectorKey[a->numframe];
            pAnim->mChannels[l]->mRotationKeys = new aiQuatKey[a->numframe];
            pos = m3d->bone[l].pos;
            ori = m3d->bone[l].ori;
            for (j = n = 0; j < a->numframe; j++) {
                t = ((double)a->frame[j].msec) / 10;
                for (k = 0; k < a->frame[j].numtransform; k++) {
                    if (a->frame[j].transform[k].boneid == l) {
                        pos = a->frame[j].transform[k].pos;
                        ori = a->frame[j].transform[k].ori;
                    }
                }
                if (pos >= m3d->numvertex || ori >= m3d->numvertex) continue;
                m3dv_t *v = &m3d->vertex[pos];
                m3dv_t *q = &m3d->vertex[ori];
                pAnim->mChannels[l]->mPositionKeys[j].mTime = t;
                pAnim->mChannels[l]->mPositionKeys[j].mValue.x = v->x;
                pAnim->mChannels[l]->mPositionKeys[j].mValue.y = v->y;
                pAnim->mChannels[l]->mPositionKeys[j].mValue.z = v->z;
                pAnim->mChannels[l]->mRotationKeys[j].mTime = t;
                pAnim->mChannels[l]->mRotationKeys[j].mValue.w = q->w;
                pAnim->mChannels[l]->mRotationKeys[j].mValue.x = q->x;
                pAnim->mChannels[l]->mRotationKeys[j].mValue.y = q->y;
                pAnim->mChannels[l]->mRotationKeys[j].mValue.z = q->z;
            } // foreach frame
        } // foreach bones
        mScene->mAnimations[i] = pAnim;
    }
}

// ------------------------------------------------------------------------------------------------
// convert uint32_t into aiColor4D
aiColor4D M3DImporter::mkColor(uint32_t c) {
    aiColor4D color;
    color.a = ((float)((c >> 24) & 0xff)) / 255;
    color.b = ((float)((c >> 16) & 0xff)) / 255;
    color.g = ((float)((c >> 8) & 0xff)) / 255;
    color.r = ((float)((c >> 0) & 0xff)) / 255;
    return color;
}

// ------------------------------------------------------------------------------------------------
// convert a position id and orientation id into a 4 x 4 transformation matrix
void M3DImporter::convertPose(const M3DWrapper &m3d, aiMatrix4x4 *m, unsigned int posid, unsigned int orientid) {
    ai_assert(m != nullptr);
    ai_assert(m3d);
    ai_assert(posid != M3D_UNDEF);
    ai_assert(posid < m3d->numvertex);
    ai_assert(orientid != M3D_UNDEF);
    ai_assert(orientid < m3d->numvertex);
    if (!m3d->numvertex || !m3d->vertex)
        return;
    m3dv_t *p = &m3d->vertex[posid];
    m3dv_t *q = &m3d->vertex[orientid];

    /* quaternion to matrix. Do NOT use aiQuaternion to aiMatrix3x3, gives bad results */
    if (q->x == 0.0 && q->y == 0.0 && q->z >= 0.7071065 && q->z <= 0.7071075 && q->w == 0.0) {
        m->a2 = m->a3 = m->b1 = m->b3 = m->c1 = m->c2 = 0.0;
        m->a1 = m->b2 = m->c3 = -1.0;
    } else {
        m->a1 = 1 - 2 * (q->y * q->y + q->z * q->z);
        if (m->a1 > -M3D_EPSILON && m->a1 < M3D_EPSILON) m->a1 = 0.0;
        m->a2 = 2 * (q->x * q->y - q->z * q->w);
        if (m->a2 > -M3D_EPSILON && m->a2 < M3D_EPSILON) m->a2 = 0.0;
        m->a3 = 2 * (q->x * q->z + q->y * q->w);
        if (m->a3 > -M3D_EPSILON && m->a3 < M3D_EPSILON) m->a3 = 0.0;
        m->b1 = 2 * (q->x * q->y + q->z * q->w);
        if (m->b1 > -M3D_EPSILON && m->b1 < M3D_EPSILON) m->b1 = 0.0;
        m->b2 = 1 - 2 * (q->x * q->x + q->z * q->z);
        if (m->b2 > -M3D_EPSILON && m->b2 < M3D_EPSILON) m->b2 = 0.0;
        m->b3 = 2 * (q->y * q->z - q->x * q->w);
        if (m->b3 > -M3D_EPSILON && m->b3 < M3D_EPSILON) m->b3 = 0.0;
        m->c1 = 2 * (q->x * q->z - q->y * q->w);
        if (m->c1 > -M3D_EPSILON && m->c1 < M3D_EPSILON) m->c1 = 0.0;
        m->c2 = 2 * (q->y * q->z + q->x * q->w);
        if (m->c2 > -M3D_EPSILON && m->c2 < M3D_EPSILON) m->c2 = 0.0;
        m->c3 = 1 - 2 * (q->x * q->x + q->y * q->y);
        if (m->c3 > -M3D_EPSILON && m->c3 < M3D_EPSILON) m->c3 = 0.0;
    }

    /* set translation */
    m->a4 = p->x;
    m->b4 = p->y;
    m->c4 = p->z;

    m->d1 = 0;
    m->d2 = 0;
    m->d3 = 0;
    m->d4 = 1;
}

// ------------------------------------------------------------------------------------------------
// find a node by name
aiNode *M3DImporter::findNode(aiNode *pNode, const aiString &name) {
    ai_assert(pNode != nullptr);
    ai_assert(mScene != nullptr);

    if (pNode->mName == name) {
        return pNode;
    }

    for (unsigned int i = 0; i < pNode->mNumChildren; i++) {
        aiNode *pChild = findNode(pNode->mChildren[i], name);
        if (pChild) {
            return pChild;
        }
    }
    return nullptr;
}

// ------------------------------------------------------------------------------------------------
// fills up offsetmatrix in mBones
void M3DImporter::calculateOffsetMatrix(aiNode *pNode, aiMatrix4x4 *m) {
    ai_assert(pNode != nullptr);
    ai_assert(mScene != nullptr);

    if (pNode->mParent) {
        calculateOffsetMatrix(pNode->mParent, m);
        *m *= pNode->mTransformation;
    } else {
        *m = pNode->mTransformation;
    }
}

// ------------------------------------------------------------------------------------------------
// because M3D has a global mesh, global vertex ids and stores materialid on the face, we need
// temporary lists to collect data for an aiMesh, which requires local arrays and local indices
// this function fills up an aiMesh with those temporary lists
void M3DImporter::populateMesh(const M3DWrapper &m3d, aiMesh *pMesh, std::vector<aiFace> *faces, std::vector<aiVector3D> *vertices,
        std::vector<aiVector3D> *normals, std::vector<aiVector3D> *texcoords, std::vector<aiColor4D> *colors,
        std::vector<unsigned int> *vertexids) {

    ai_assert(pMesh != nullptr);
    ai_assert(faces != nullptr);
    ai_assert(vertices != nullptr);
    ai_assert(normals != nullptr);
    ai_assert(texcoords != nullptr);
    ai_assert(colors != nullptr);
    ai_assert(vertexids != nullptr);
    ai_assert(m3d);

    ASSIMP_LOG_DEBUG("M3D: populateMesh numvertices ", vertices->size(), " numfaces ", faces->size(),
            " numnormals ", normals->size(), " numtexcoord ", texcoords->size(), " numbones ", m3d->numbone);

    if (vertices->size() && faces->size()) {
        pMesh->mNumFaces = static_cast<unsigned int>(faces->size());
        pMesh->mFaces = new aiFace[pMesh->mNumFaces];
        std::copy(faces->begin(), faces->end(), pMesh->mFaces);
        pMesh->mNumVertices = static_cast<unsigned int>(vertices->size());
        pMesh->mVertices = new aiVector3D[pMesh->mNumVertices];
        std::copy(vertices->begin(), vertices->end(), pMesh->mVertices);
        if (normals->size() == vertices->size()) {
            pMesh->mNormals = new aiVector3D[pMesh->mNumVertices];
            std::copy(normals->begin(), normals->end(), pMesh->mNormals);
        }
        if (texcoords->size() == vertices->size()) {
            pMesh->mTextureCoords[0] = new aiVector3D[pMesh->mNumVertices];
            std::copy(texcoords->begin(), texcoords->end(), pMesh->mTextureCoords[0]);
            pMesh->mNumUVComponents[0] = 2;
        }
        if (colors->size() == vertices->size()) {
            pMesh->mColors[0] = new aiColor4D[pMesh->mNumVertices];
            std::copy(colors->begin(), colors->end(), pMesh->mColors[0]);
        }
        // this is complicated, because M3D stores a list of bone id / weight pairs per
        // vertex but assimp uses lists of local vertex id/weight pairs per local bone list
        pMesh->mNumBones = m3d->numbone;
        // we need aiBone with mOffsetMatrix for bones without weights as well
        if (pMesh->mNumBones && m3d->numbone && m3d->bone) {
            pMesh->mBones = new aiBone *[pMesh->mNumBones];
            for (unsigned int i = 0; i < m3d->numbone; i++) {
                aiNode *pNode;
                pMesh->mBones[i] = new aiBone;
                pMesh->mBones[i]->mName = aiString(std::string(m3d->bone[i].name));
                pMesh->mBones[i]->mNumWeights = 0;
                pNode = findNode(mScene->mRootNode, pMesh->mBones[i]->mName);
                if (pNode) {
                    calculateOffsetMatrix(pNode, &pMesh->mBones[i]->mOffsetMatrix);
                    pMesh->mBones[i]->mOffsetMatrix.Inverse();
                } else
                    pMesh->mBones[i]->mOffsetMatrix = aiMatrix4x4();
            }
            if (vertexids->size() && m3d->numvertex && m3d->vertex && m3d->numskin && m3d->skin) {
                unsigned int i, j;
                // first count how many vertices we have per bone
                for (i = 0; i < vertexids->size(); i++) {
                    if (vertexids->at(i) >= m3d->numvertex) {
                        continue;
                    }
                    unsigned int s = m3d->vertex[vertexids->at(i)].skinid;
                    if (s != M3D_UNDEF && s != M3D_INDEXMAX) {
                        for (unsigned int k = 0; k < M3D_NUMBONE && m3d->skin[s].weight[k] > 0.0; k++) {
                            aiString name = aiString(std::string(m3d->bone[m3d->skin[s].boneid[k]].name));
                            for (j = 0; j < pMesh->mNumBones; j++) {
                                if (pMesh->mBones[j]->mName == name) {
                                    pMesh->mBones[j]->mNumWeights++;
                                    break;
                                }
                            }
                        }
                    }
                }
                // allocate mWeights
                for (j = 0; j < pMesh->mNumBones; j++) {
                    aiBone *pBone = pMesh->mBones[j];
                    if (pBone->mNumWeights) {
                        pBone->mWeights = new aiVertexWeight[pBone->mNumWeights];
                        pBone->mNumWeights = 0;
                    }
                }
                // fill up with data
                for (i = 0; i < vertexids->size(); i++) {
                    if (vertexids->at(i) >= m3d->numvertex) continue;
                    unsigned int s = m3d->vertex[vertexids->at(i)].skinid;
                    if (s != M3D_UNDEF && s != M3D_INDEXMAX && s < m3d->numskin) {
                        for (unsigned int k = 0; k < M3D_NUMBONE && m3d->skin[s].weight[k] > 0.0; k++) {
                            if (m3d->skin[s].boneid[k] >= m3d->numbone) continue;
                            aiString name = aiString(std::string(m3d->bone[m3d->skin[s].boneid[k]].name));
                            for (j = 0; j < pMesh->mNumBones; j++) {
                                if (pMesh->mBones[j]->mName == name) {
                                    aiBone *pBone = pMesh->mBones[j];
                                    pBone->mWeights[pBone->mNumWeights].mVertexId = i;
                                    pBone->mWeights[pBone->mNumWeights].mWeight = m3d->skin[s].weight[k];
                                    pBone->mNumWeights++;
                                    break;
                                }
                            }
                        } // foreach skin
                    }
                } // foreach vertexids
            }
        }
    }
}

// ------------------------------------------------------------------------------------------------

} // Namespace Assimp

#endif // !! ASSIMP_BUILD_NO_M3D_IMPORTER
