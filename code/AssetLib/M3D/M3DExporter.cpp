/*
Open Asset Import Library (assimp)
----------------------------------------------------------------------

Copyright (c) 2006-2025, assimp team
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
#ifndef ASSIMP_BUILD_NO_EXPORT
#ifndef ASSIMP_BUILD_NO_M3D_EXPORTER

#define M3D_IMPLEMENTATION
#define M3D_NOIMPORTER
#define M3D_EXPORTER
#ifndef ASSIMP_BUILD_NO_M3D_IMPORTER
#define M3D_NODUP


// Header files, standard library.
#include <memory> // shared_ptr
#include <string>
#include <vector>

#include <assimp/Exceptional.h> // DeadlyExportError
#include <assimp/StreamWriter.h> // StreamWriterLE
#include <assimp/material.h> // aiTextureType
#include <assimp/mesh.h>
#include <assimp/scene.h>
#include <assimp/version.h> // aiGetVersion
#include <assimp/DefaultLogger.hpp>
#include <assimp/Exporter.hpp>
#include <assimp/IOSystem.hpp>

#include "M3DExporter.h"
#include "M3DMaterials.h"
#include "M3DWrapper.h"

// RESOURCES:
// https://gitlab.com/bztsrc/model3d/blob/master/docs/m3d_format.md
// https://gitlab.com/bztsrc/model3d/blob/master/docs/a3d_format.md

/*
 * Currently supports static meshes, vertex colors, materials, textures
 *
 * For animation, it would require the following conversions:
 *  - aiNode (bones) -> m3d_t.bone (with parent id, position vector and orientation quaternion)
 *  - aiMesh.aiBone -> m3d_t.skin (per vertex, with bone id, weight pairs)
 *  - aiAnimation -> m3d_action (frame with timestamp and list of bone id, position, orientation
 *      triplets, instead of per bone timestamp + lists)
 */

// ------------------------------------------------------------------------------------------------
// Conversion functions
// ------------------------------------------------------------------------------------------------
// helper to add a vertex (private to NodeWalk)
m3dv_t *AddVrtx(m3dv_t *vrtx, uint32_t *numvrtx, m3dv_t *v, uint32_t *idx) {
    if (v->x == (M3D_FLOAT)-0.0) v->x = (M3D_FLOAT)0.0;
    if (v->y == (M3D_FLOAT)-0.0) v->y = (M3D_FLOAT)0.0;
    if (v->z == (M3D_FLOAT)-0.0) v->z = (M3D_FLOAT)0.0;
    if (v->w == (M3D_FLOAT)-0.0) v->w = (M3D_FLOAT)0.0;
    vrtx = (m3dv_t *)M3D_REALLOC(vrtx, ((*numvrtx) + 1) * sizeof(m3dv_t));
    memcpy(&vrtx[*numvrtx], v, sizeof(m3dv_t));
    *idx = *numvrtx;
    (*numvrtx)++;
    return vrtx;
}

// ------------------------------------------------------------------------------------------------
// helper to add a tmap (private to NodeWalk)
m3dti_t *AddTmap(m3dti_t *tmap, uint32_t *numtmap, m3dti_t *ti, uint32_t *idx) {
    tmap = (m3dti_t *)M3D_REALLOC(tmap, ((*numtmap) + 1) * sizeof(m3dti_t));
    memcpy(&tmap[*numtmap], ti, sizeof(m3dti_t));
    *idx = *numtmap;
    (*numtmap)++;
    return tmap;
}

// ------------------------------------------------------------------------------------------------
// convert aiColor4D into uint32_t
uint32_t mkColor(aiColor4D *c) {
    return ((uint8_t)(c->a * 255) << 24L) |
           ((uint8_t)(c->b * 255) << 16L) |
           ((uint8_t)(c->g * 255) << 8L) |
           ((uint8_t)(c->r * 255) << 0L);
}

// ------------------------------------------------------------------------------------------------
// add a material property to the output
void addProp(m3dm_t *m, uint8_t type, uint32_t value) {
    unsigned int i;
    i = m->numprop++;
    m->prop = (m3dp_t *)M3D_REALLOC(m->prop, m->numprop * sizeof(m3dp_t));
    if (!m->prop) {
        throw DeadlyExportError("memory allocation error");
    }
    m->prop[i].type = type;
    m->prop[i].value.num = value;
}

// ------------------------------------------------------------------------------------------------
// convert aiString to identifier safe C string. This is a duplication of _m3d_safestr
char *SafeStr(aiString str, bool isStrict) {
    char *s = (char *)&str.data;
    char *d, *ret;
    int i, len;

    for (len = str.length + 1; *s && (*s == ' ' || *s == '\t'); s++, len--)
        ;
    if (len > 255) len = 255;
    ret = (char *)M3D_MALLOC(len + 1);
    if (!ret) {
        throw DeadlyExportError("memory allocation error");
    }
    for (i = 0, d = ret; i < len && *s && *s != '\r' && *s != '\n'; s++, d++, i++) {
        *d = isStrict && (*s == ' ' || *s == '\t' || *s == '/' || *s == '\\') ? '_' : (*s == '\t' ? ' ' : *s);
    }
    for (; d > ret && (*(d - 1) == ' ' || *(d - 1) == '\t'); d--)
        ;
    *d = 0;
    return ret;
}

// ------------------------------------------------------------------------------------------------
// add a material to the output
M3D_INDEX addMaterial(const Assimp::M3DWrapper &m3d, const aiMaterial *mat) {
    unsigned int mi = M3D_NOTDEFINED;
    aiColor4D c;
    aiString name;
    ai_real f;
    char *fn;

    if (mat && mat->Get(AI_MATKEY_NAME, name) == AI_SUCCESS && name.length &&
            strcmp((char *)&name.data, AI_DEFAULT_MATERIAL_NAME)) {
        // check if we have saved a material by this name. This has to be done
        // because only the referenced materials should be added to the output
        for (unsigned int i = 0; i < m3d->nummaterial; i++)
            if (!strcmp((char *)&name.data, m3d->material[i].name)) {
                mi = i;
                break;
            }
        // if not found, add the material to the output
        if (mi == M3D_NOTDEFINED) {
            unsigned int k;
            mi = m3d->nummaterial++;
            m3d->material = (m3dm_t *)M3D_REALLOC(m3d->material, m3d->nummaterial * sizeof(m3dm_t));
            if (!m3d->material) {
                throw DeadlyExportError("memory allocation error");
            }
            m3d->material[mi].name = SafeStr(name, true);
            m3d->material[mi].numprop = 0;
            m3d->material[mi].prop = nullptr;
            // iterate through the material property table and see what we got
            for (k = 0; k < 15; k++) {
                unsigned int j;
                if (m3d_propertytypes[k].format == m3dpf_map)
                    continue;
                if (aiProps[k].pKey) {
                    switch (m3d_propertytypes[k].format) {
                    case m3dpf_color:
                        if (mat->Get(aiProps[k].pKey, aiProps[k].type,
                                    aiProps[k].index, c) == AI_SUCCESS)
                            addProp(&m3d->material[mi],
                                    m3d_propertytypes[k].id, mkColor(&c));
                        break;
                    case m3dpf_float:
                        if (mat->Get(aiProps[k].pKey, aiProps[k].type,
                                    aiProps[k].index, f) == AI_SUCCESS) {
                            uint32_t f_uint32;
                            memcpy(&f_uint32, &f, sizeof(uint32_t));
                            addProp(&m3d->material[mi],
                                    m3d_propertytypes[k].id,
                                    /* not (uint32_t)f, because we don't want to convert
                                         * it, we want to see it as 32 bits of memory */
                                    f_uint32);
                        }
                        break;
                    case m3dpf_uint8:
                        if (mat->Get(aiProps[k].pKey, aiProps[k].type,
                                    aiProps[k].index, j) == AI_SUCCESS) {
                            // special conversion for illumination model property
                            if (m3d_propertytypes[k].id == m3dp_il) {
                                switch (j) {
                                case aiShadingMode_NoShading: j = 0; break;
                                case aiShadingMode_Phong: j = 2; break;
                                default: j = 1; break;
                                }
                            }
                            addProp(&m3d->material[mi],
                                    m3d_propertytypes[k].id, j);
                        }
                        break;
                    default:
                        if (mat->Get(aiProps[k].pKey, aiProps[k].type,
                                    aiProps[k].index, j) == AI_SUCCESS)
                            addProp(&m3d->material[mi],
                                    m3d_propertytypes[k].id, j);
                        break;
                    }
                }
                if (aiTxProps[k].pKey &&
                        mat->GetTexture((aiTextureType)aiTxProps[k].type,
                                aiTxProps[k].index, &name, nullptr, nullptr, nullptr,
                                nullptr, nullptr) == AI_SUCCESS) {
                    unsigned int i;
                    for (j = name.length - 1; j > 0 && name.data[j] != '.'; j++)
                        ;
                    if (j && name.data[j] == '.' &&
                            (name.data[j + 1] == 'p' || name.data[j + 1] == 'P') &&
                            (name.data[j + 1] == 'n' || name.data[j + 1] == 'N') &&
                            (name.data[j + 1] == 'g' || name.data[j + 1] == 'G'))
                        name.data[j] = 0;
                    // do we have this texture saved already?
                    fn = SafeStr(name, true);
                    for (j = 0, i = M3D_NOTDEFINED; j < m3d->numtexture; j++)
                        if (!strcmp(fn, m3d->texture[j].name)) {
                            i = j;
                            free(fn);
                            break;
                        }
                    if (i == M3D_NOTDEFINED) {
                        i = m3d->numtexture++;
                        m3d->texture = (m3dtx_t *)M3D_REALLOC(
                                m3d->texture,
                                m3d->numtexture * sizeof(m3dtx_t));
                        if (!m3d->texture) {
                            throw DeadlyExportError("memory allocation error");
                        }
                        // we don't need the texture itself, only its name
                        m3d->texture[i].name = fn;
                        m3d->texture[i].w = 0;
                        m3d->texture[i].h = 0;
                        m3d->texture[i].d = nullptr;
                    }
                    addProp(&m3d->material[mi],
                            m3d_propertytypes[k].id + 128, i);
                }
            }
        }
    }
    return mi;
}

namespace Assimp {

// ---------------------------------------------------------------------
// Worker function for exporting a scene to binary M3D.
// Prototyped and registered in Exporter.cpp
void ExportSceneM3D(
        const char *pFile,
        IOSystem *pIOSystem,
        const aiScene *pScene,
        const ExportProperties *pProperties) {
    // initialize the exporter
    M3DExporter exporter(pScene, pProperties);

    // perform binary export
    exporter.doExport(pFile, pIOSystem, false);
}

// ---------------------------------------------------------------------
// Worker function for exporting a scene to ASCII A3D.
// Prototyped and registered in Exporter.cpp
void ExportSceneM3DA(
        const char *pFile,
        IOSystem *pIOSystem,
        const aiScene *pScene,
        const ExportProperties *pProperties

) {
    // initialize the exporter
    M3DExporter exporter(pScene, pProperties);

    // perform ascii export
    exporter.doExport(pFile, pIOSystem, true);
}

// ------------------------------------------------------------------------------------------------
M3DExporter::M3DExporter(const aiScene *pScene, const ExportProperties *pProperties) :
        mScene(pScene),
        mProperties(pProperties),
        outfile() {
    // empty
}

// ------------------------------------------------------------------------------------------------
void M3DExporter::doExport(
        const char *pFile,
        IOSystem *pIOSystem,
        bool toAscii) {
    // TODO: convert mProperties into M3D_EXP_* flags
    (void)mProperties;

    // open the indicated file for writing (in binary / ASCII mode)
    outfile.reset(pIOSystem->Open(pFile, toAscii ? "wt" : "wb"));
    if (!outfile) {
        throw DeadlyExportError("could not open output .m3d file: " + std::string(pFile));
    }

    M3DWrapper m3d;
    if (!m3d) {
        throw DeadlyExportError("memory allocation error");
    }
    m3d->name = SafeStr(mScene->mRootNode->mName, false);

    // Create a model from assimp structures
    aiMatrix4x4 m;
    NodeWalk(m3d, mScene->mRootNode, m);

    // serialize the structures
    unsigned int size;
    unsigned char *output = m3d.Save(M3D_EXP_FLOAT, M3D_EXP_EXTRA | (toAscii ? M3D_EXP_ASCII : 0), size);

    if (!output || size < 8) {
        throw DeadlyExportError("unable to serialize into Model 3D");
    }

    // Write out serialized model
    outfile->Write(output, size, 1);

    // explicitly release file pointer,
    // so we don't have to rely on class destruction.
    outfile.reset();

    M3D_FREE(m3d->name);
    m3d->name = nullptr;
}

// ------------------------------------------------------------------------------------------------
// recursive node walker
void M3DExporter::NodeWalk(const M3DWrapper &m3d, const aiNode *pNode, aiMatrix4x4 m) {
    aiMatrix4x4 nm = m * pNode->mTransformation;

    for (unsigned int i = 0; i < pNode->mNumMeshes; i++) {
        const aiMesh *mesh = mScene->mMeshes[pNode->mMeshes[i]];
        unsigned int mi = M3D_NOTDEFINED;
        if (mScene->mMaterials) {
            // get the material for this mesh
            mi = addMaterial(m3d, mScene->mMaterials[mesh->mMaterialIndex]);
        }
        // iterate through the mesh faces
        for (unsigned int j = 0; j < mesh->mNumFaces; j++) {
            unsigned int n;
            const aiFace *face = &(mesh->mFaces[j]);
            // only triangle meshes supported for now
            if (face->mNumIndices != 3) {
                throw DeadlyExportError("use aiProcess_Triangulate before export");
            }
            // add triangle to the output
            n = m3d->numface++;
            m3d->face = (m3df_t *)M3D_REALLOC(m3d->face,
                    m3d->numface * sizeof(m3df_t));
            if (!m3d->face) {
                throw DeadlyExportError("memory allocation error");
            }
            /* set all index to -1 by default */
            m3d->face[n].vertex[0] = m3d->face[n].vertex[1] = m3d->face[n].vertex[2] =
                    m3d->face[n].normal[0] = m3d->face[n].normal[1] = m3d->face[n].normal[2] =
                            m3d->face[n].texcoord[0] = m3d->face[n].texcoord[1] = m3d->face[n].texcoord[2] = M3D_UNDEF;
            m3d->face[n].materialid = mi;
            for (unsigned int k = 0; k < face->mNumIndices; k++) {
                // get the vertex's index
                unsigned int l = face->mIndices[k];
                unsigned int idx;
                m3dv_t vertex;
                m3dti_t ti;
                // multiply the position vector by the transformation matrix
                aiVector3D v = mesh->mVertices[l];
                v *= nm;
                vertex.x = v.x;
                vertex.y = v.y;
                vertex.z = v.z;
                vertex.w = 1.0;
                vertex.color = 0;
                vertex.skinid = M3D_UNDEF;
                // add color if defined
                if (mesh->HasVertexColors(0))
                    vertex.color = mkColor(&mesh->mColors[0][l]);
                // save the vertex to the output
                m3d->vertex = AddVrtx(m3d->vertex, &m3d->numvertex,
                        &vertex, &idx);
                m3d->face[n].vertex[k] = (M3D_INDEX)idx;
                // do we have texture coordinates?
                if (mesh->HasTextureCoords(0)) {
                    ti.u = mesh->mTextureCoords[0][l].x;
                    ti.v = mesh->mTextureCoords[0][l].y;
                    m3d->tmap = AddTmap(m3d->tmap, &m3d->numtmap, &ti, &idx);
                    m3d->face[n].texcoord[k] = (M3D_INDEX)idx;
                }
                // do we have normal vectors?
                if (mesh->HasNormals()) {
                    vertex.x = mesh->mNormals[l].x;
                    vertex.y = mesh->mNormals[l].y;
                    vertex.z = mesh->mNormals[l].z;
                    vertex.color = 0;
                    m3d->vertex = AddVrtx(m3d->vertex, &m3d->numvertex, &vertex, &idx);
                    m3d->face[n].normal[k] = (M3D_INDEX)idx;
                }
            }
        }
    }
    // repeat for the children nodes
    for (unsigned int i = 0; i < pNode->mNumChildren; i++) {
        NodeWalk(m3d, pNode->mChildren[i], nm);
    }
}
} // namespace Assimp
#endif
#endif // ASSIMP_BUILD_NO_M3D_EXPORTER
#endif // ASSIMP_BUILD_NO_EXPORT
