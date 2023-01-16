/*
Open Asset Import Library (assimp)
----------------------------------------------------------------------

Copyright (c) 2006-2022, assimp team

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
#ifndef ASSIMP_BUILD_NO_GLTF_EXPORTER

#include "AssetLib/glTF2/glTF2Exporter.h"
#include "AssetLib/glTF2/glTF2AssetWriter.h"
#include "PostProcessing/SplitLargeMeshes.h"

#include <assimp/ByteSwapper.h>
#include <assimp/Exceptional.h>
#include <assimp/SceneCombiner.h>
#include <assimp/StringComparison.h>
#include <assimp/commonMetaData.h>
#include <assimp/material.h>
#include <assimp/scene.h>
#include <assimp/version.h>
#include <assimp/Exporter.hpp>
#include <assimp/IOSystem.hpp>

// Header files, standard library.
#include <cinttypes>
#include <limits>
#include <memory>

using namespace rapidjson;

using namespace Assimp;
using namespace glTF2;

namespace Assimp {

// ------------------------------------------------------------------------------------------------
// Worker function for exporting a scene to GLTF. Prototyped and registered in Exporter.cpp
void ExportSceneGLTF2(const char *pFile, IOSystem *pIOSystem, const aiScene *pScene, const ExportProperties *pProperties) {
    // invoke the exporter
    glTF2Exporter exporter(pFile, pIOSystem, pScene, pProperties, false);
}

// ------------------------------------------------------------------------------------------------
// Worker function for exporting a scene to GLB. Prototyped and registered in Exporter.cpp
void ExportSceneGLB2(const char *pFile, IOSystem *pIOSystem, const aiScene *pScene, const ExportProperties *pProperties) {
    // invoke the exporter
    glTF2Exporter exporter(pFile, pIOSystem, pScene, pProperties, true);
}

} // end of namespace Assimp

glTF2Exporter::glTF2Exporter(const char *filename, IOSystem *pIOSystem, const aiScene *pScene,
        const ExportProperties *pProperties, bool isBinary) :
        mFilename(filename), mIOSystem(pIOSystem), mScene(pScene), mProperties(pProperties), mAsset(new Asset(pIOSystem)) {
    // Always on as our triangulation process is aware of this type of encoding
    mAsset->extensionsUsed.FB_ngon_encoding = true;

    if (isBinary) {
        mAsset->SetAsBinary();
    }

    ExportMetadata();

    ExportMaterials();

    if (mScene->mRootNode) {
        ExportNodeHierarchy(mScene->mRootNode);
    }

    ExportMeshes();
    MergeMeshes();

    ExportScene();

    ExportAnimations();

    // export extras
    if (mProperties->HasPropertyCallback("extras")) {
        std::function<void *(void *)> ExportExtras = mProperties->GetPropertyCallback("extras");
        mAsset->extras = (rapidjson::Value *)ExportExtras(0);
    }

    AssetWriter writer(*mAsset);

    if (isBinary) {
        writer.WriteGLBFile(filename);
    } else {
        writer.WriteFile(filename);
    }
}

glTF2Exporter::~glTF2Exporter() = default;

/*
 * Copy a 4x4 matrix from struct aiMatrix to typedef mat4.
 * Also converts from row-major to column-major storage.
 */
static void CopyValue(const aiMatrix4x4 &v, mat4 &o) {
    o[0] = v.a1;
    o[1] = v.b1;
    o[2] = v.c1;
    o[3] = v.d1;
    o[4] = v.a2;
    o[5] = v.b2;
    o[6] = v.c2;
    o[7] = v.d2;
    o[8] = v.a3;
    o[9] = v.b3;
    o[10] = v.c3;
    o[11] = v.d3;
    o[12] = v.a4;
    o[13] = v.b4;
    o[14] = v.c4;
    o[15] = v.d4;
}

static void CopyValue(const aiMatrix4x4 &v, aiMatrix4x4 &o) {
    memcpy(&o, &v, sizeof(aiMatrix4x4));
}

static void IdentityMatrix4(mat4 &o) {
    o[0] = 1;
    o[1] = 0;
    o[2] = 0;
    o[3] = 0;
    o[4] = 0;
    o[5] = 1;
    o[6] = 0;
    o[7] = 0;
    o[8] = 0;
    o[9] = 0;
    o[10] = 1;
    o[11] = 0;
    o[12] = 0;
    o[13] = 0;
    o[14] = 0;
    o[15] = 1;
}

static bool IsBoneWeightFitted(vec4 &weight) {
    return weight[0] + weight[1] + weight[2] + weight[3] >= 1.f;
}

static int FitBoneWeight(vec4 &weight, float value) {
    int i = 0;
    for (; i < 4; ++i) {
        if (weight[i] < value) {
            weight[i] = value;
            return i;
        }
    }

    return -1;
}

template <typename T>
void SetAccessorRange(Ref<Accessor> acc, void *data, size_t count,
        unsigned int numCompsIn, unsigned int numCompsOut) {
    ai_assert(numCompsOut <= numCompsIn);

    // Allocate and initialize with large values.
    for (unsigned int i = 0; i < numCompsOut; i++) {
        acc->min.push_back(std::numeric_limits<double>::max());
        acc->max.push_back(-std::numeric_limits<double>::max());
    }

    size_t totalComps = count * numCompsIn;
    T *buffer_ptr = static_cast<T *>(data);
    T *buffer_end = buffer_ptr + totalComps;

    // Search and set extreme values.
    for (; buffer_ptr < buffer_end; buffer_ptr += numCompsIn) {
        for (unsigned int j = 0; j < numCompsOut; j++) {
            double valueTmp = buffer_ptr[j];

            // Gracefully tolerate rogue NaN's in buffer data
            // Any NaNs/Infs introduced in accessor bounds will end up in
            // document and prevent rapidjson from writing out valid JSON
            if (!std::isfinite(valueTmp)) {
                continue;
            }

            if (valueTmp < acc->min[j]) {
                acc->min[j] = valueTmp;
            }
            if (valueTmp > acc->max[j]) {
                acc->max[j] = valueTmp;
            }
        }
    }
}

inline void SetAccessorRange(ComponentType compType, Ref<Accessor> acc, void *data,
        size_t count, unsigned int numCompsIn, unsigned int numCompsOut) {
    switch (compType) {
    case ComponentType_SHORT:
        SetAccessorRange<short>(acc, data, count, numCompsIn, numCompsOut);
        return;
    case ComponentType_UNSIGNED_SHORT:
        SetAccessorRange<unsigned short>(acc, data, count, numCompsIn, numCompsOut);
        return;
    case ComponentType_UNSIGNED_INT:
        SetAccessorRange<unsigned int>(acc, data, count, numCompsIn, numCompsOut);
        return;
    case ComponentType_FLOAT:
        SetAccessorRange<float>(acc, data, count, numCompsIn, numCompsOut);
        return;
    case ComponentType_BYTE:
        SetAccessorRange<int8_t>(acc, data, count, numCompsIn, numCompsOut);
        return;
    case ComponentType_UNSIGNED_BYTE:
        SetAccessorRange<uint8_t>(acc, data, count, numCompsIn, numCompsOut);
        return;
    }
}

// compute the (data-dataBase), store the non-zero data items
template <typename T>
size_t NZDiff(void *data, void *dataBase, size_t count, unsigned int numCompsIn, unsigned int numCompsOut, void *&outputNZDiff, void *&outputNZIdx) {
    std::vector<T> vNZDiff;
    std::vector<unsigned short> vNZIdx;
    size_t totalComps = count * numCompsIn;
    T *bufferData_ptr = static_cast<T *>(data);
    T *bufferData_end = bufferData_ptr + totalComps;
    T *bufferBase_ptr = static_cast<T *>(dataBase);

    // Search and set extreme values.
    for (short idx = 0; bufferData_ptr < bufferData_end; idx += 1, bufferData_ptr += numCompsIn) {
        bool bNonZero = false;

        //for the data, check any component Non Zero
        for (unsigned int j = 0; j < numCompsOut; j++) {
            double valueData = bufferData_ptr[j];
            double valueBase = bufferBase_ptr ? bufferBase_ptr[j] : 0;
            if ((valueData - valueBase) != 0) {
                bNonZero = true;
                break;
            }
        }

        //all zeros, continue
        if (!bNonZero)
            continue;

        //non zero, store the data
        for (unsigned int j = 0; j < numCompsOut; j++) {
            T valueData = bufferData_ptr[j];
            T valueBase = bufferBase_ptr ? bufferBase_ptr[j] : 0;
            vNZDiff.push_back(valueData - valueBase);
        }
        vNZIdx.push_back(idx);
    }

    //avoid all-0, put 1 item
    if (vNZDiff.size() == 0) {
        for (unsigned int j = 0; j < numCompsOut; j++)
            vNZDiff.push_back(0);
        vNZIdx.push_back(0);
    }

    //process data
    outputNZDiff = new T[vNZDiff.size()];
    memcpy(outputNZDiff, vNZDiff.data(), vNZDiff.size() * sizeof(T));

    outputNZIdx = new unsigned short[vNZIdx.size()];
    memcpy(outputNZIdx, vNZIdx.data(), vNZIdx.size() * sizeof(unsigned short));
    return vNZIdx.size();
}

inline size_t NZDiff(ComponentType compType, void *data, void *dataBase, size_t count, unsigned int numCompsIn, unsigned int numCompsOut, void *&nzDiff, void *&nzIdx) {
    switch (compType) {
    case ComponentType_SHORT:
        return NZDiff<short>(data, dataBase, count, numCompsIn, numCompsOut, nzDiff, nzIdx);
    case ComponentType_UNSIGNED_SHORT:
        return NZDiff<unsigned short>(data, dataBase, count, numCompsIn, numCompsOut, nzDiff, nzIdx);
    case ComponentType_UNSIGNED_INT:
        return NZDiff<unsigned int>(data, dataBase, count, numCompsIn, numCompsOut, nzDiff, nzIdx);
    case ComponentType_FLOAT:
        return NZDiff<float>(data, dataBase, count, numCompsIn, numCompsOut, nzDiff, nzIdx);
    case ComponentType_BYTE:
        return NZDiff<int8_t>(data, dataBase, count, numCompsIn, numCompsOut, nzDiff, nzIdx);
    case ComponentType_UNSIGNED_BYTE:
        return NZDiff<uint8_t>(data, dataBase, count, numCompsIn, numCompsOut, nzDiff, nzIdx);
    }
    return 0;
}

inline Ref<Accessor> ExportDataSparse(Asset &a, std::string &meshName, Ref<Buffer> &buffer,
        size_t count, void *data, AttribType::Value typeIn, AttribType::Value typeOut, ComponentType compType, BufferViewTarget target = BufferViewTarget_NONE, void *dataBase = nullptr) {
    if (!count || !data) {
        return Ref<Accessor>();
    }

    unsigned int numCompsIn = AttribType::GetNumComponents(typeIn);
    unsigned int numCompsOut = AttribType::GetNumComponents(typeOut);
    unsigned int bytesPerComp = ComponentTypeSize(compType);

    // accessor
    Ref<Accessor> acc = a.accessors.Create(a.FindUniqueID(meshName, "accessor"));

    // if there is a basic data vector
    if (dataBase) {
        size_t base_offset = buffer->byteLength;
        size_t base_padding = base_offset % bytesPerComp;
        base_offset += base_padding;
        size_t base_length = count * numCompsOut * bytesPerComp;
        buffer->Grow(base_length + base_padding);

        Ref<BufferView> bv = a.bufferViews.Create(a.FindUniqueID(meshName, "view"));
        bv->buffer = buffer;
        bv->byteOffset = base_offset;
        bv->byteLength = base_length; //! The target that the WebGL buffer should be bound to.
        bv->byteStride = 0;
        bv->target = target;
        acc->bufferView = bv;
        acc->WriteData(count, dataBase, numCompsIn * bytesPerComp);
    }
    acc->byteOffset = 0;
    acc->componentType = compType;
    acc->count = count;
    acc->type = typeOut;

    if (data) {
        void *nzDiff = nullptr, *nzIdx = nullptr;
        size_t nzCount = NZDiff(compType, data, dataBase, count, numCompsIn, numCompsOut, nzDiff, nzIdx);
        acc->sparse.reset(new Accessor::Sparse);
        acc->sparse->count = nzCount;

        //indices
        unsigned int bytesPerIdx = sizeof(unsigned short);
        size_t indices_offset = buffer->byteLength;
        size_t indices_padding = indices_offset % bytesPerIdx;
        indices_offset += indices_padding;
        size_t indices_length = nzCount * 1 * bytesPerIdx;
        buffer->Grow(indices_length + indices_padding);

        Ref<BufferView> indicesBV = a.bufferViews.Create(a.FindUniqueID(meshName, "view"));
        indicesBV->buffer = buffer;
        indicesBV->byteOffset = indices_offset;
        indicesBV->byteLength = indices_length;
        indicesBV->byteStride = 0;
        acc->sparse->indices = indicesBV;
        acc->sparse->indicesType = ComponentType_UNSIGNED_SHORT;
        acc->sparse->indicesByteOffset = 0;
        acc->WriteSparseIndices(nzCount, nzIdx, 1 * bytesPerIdx);

        //values
        size_t values_offset = buffer->byteLength;
        size_t values_padding = values_offset % bytesPerComp;
        values_offset += values_padding;
        size_t values_length = nzCount * numCompsOut * bytesPerComp;
        buffer->Grow(values_length + values_padding);

        Ref<BufferView> valuesBV = a.bufferViews.Create(a.FindUniqueID(meshName, "view"));
        valuesBV->buffer = buffer;
        valuesBV->byteOffset = values_offset;
        valuesBV->byteLength = values_length;
        valuesBV->byteStride = 0;
        acc->sparse->values = valuesBV;
        acc->sparse->valuesByteOffset = 0;
        acc->WriteSparseValues(nzCount, nzDiff, numCompsIn * bytesPerComp);

        //clear
        delete[](char *) nzDiff;
        delete[](char *) nzIdx;
    }
    return acc;
}
inline Ref<Accessor> ExportData(Asset &a, std::string &meshName, Ref<Buffer> &buffer,
        size_t count, void *data, AttribType::Value typeIn, AttribType::Value typeOut, ComponentType compType, BufferViewTarget target = BufferViewTarget_NONE) {
    if (!count || !data) {
        return Ref<Accessor>();
    }

    unsigned int numCompsIn = AttribType::GetNumComponents(typeIn);
    unsigned int numCompsOut = AttribType::GetNumComponents(typeOut);
    unsigned int bytesPerComp = ComponentTypeSize(compType);

    size_t offset = buffer->byteLength;
    // make sure offset is correctly byte-aligned, as required by spec
    size_t padding = offset % bytesPerComp;
    offset += padding;
    size_t length = count * numCompsOut * bytesPerComp;
    buffer->Grow(length + padding);

    // bufferView
    Ref<BufferView> bv = a.bufferViews.Create(a.FindUniqueID(meshName, "view"));
    bv->buffer = buffer;
    bv->byteOffset = offset;
    bv->byteLength = length; //! The target that the WebGL buffer should be bound to.
    bv->byteStride = 0;
    bv->target = target;

    // accessor
    Ref<Accessor> acc = a.accessors.Create(a.FindUniqueID(meshName, "accessor"));
    acc->bufferView = bv;
    acc->byteOffset = 0;
    acc->componentType = compType;
    acc->count = count;
    acc->type = typeOut;

    // calculate min and max values
    SetAccessorRange(compType, acc, data, count, numCompsIn, numCompsOut);

    // copy the data
    acc->WriteData(count, data, numCompsIn * bytesPerComp);

    return acc;
}

inline void SetSamplerWrap(SamplerWrap &wrap, aiTextureMapMode map) {
    switch (map) {
    case aiTextureMapMode_Clamp:
        wrap = SamplerWrap::Clamp_To_Edge;
        break;
    case aiTextureMapMode_Mirror:
        wrap = SamplerWrap::Mirrored_Repeat;
        break;
    case aiTextureMapMode_Wrap:
    case aiTextureMapMode_Decal:
    default:
        wrap = SamplerWrap::Repeat;
        break;
    };
}

void glTF2Exporter::GetTexSampler(const aiMaterial &mat, Ref<Texture> texture, aiTextureType tt, unsigned int slot) {
    aiString aId;
    std::string id;
    if (aiGetMaterialString(&mat, AI_MATKEY_GLTF_MAPPINGID(tt, slot), &aId) == AI_SUCCESS) {
        id = aId.C_Str();
    }

    if (Ref<Sampler> ref = mAsset->samplers.Get(id.c_str())) {
        texture->sampler = ref;
    } else {
        id = mAsset->FindUniqueID(id, "sampler");

        texture->sampler = mAsset->samplers.Create(id.c_str());

        aiTextureMapMode mapU, mapV;
        SamplerMagFilter filterMag;
        SamplerMinFilter filterMin;

        if (aiGetMaterialInteger(&mat, AI_MATKEY_MAPPINGMODE_U(tt, slot), (int *)&mapU) == AI_SUCCESS) {
            SetSamplerWrap(texture->sampler->wrapS, mapU);
        }

        if (aiGetMaterialInteger(&mat, AI_MATKEY_MAPPINGMODE_V(tt, slot), (int *)&mapV) == AI_SUCCESS) {
            SetSamplerWrap(texture->sampler->wrapT, mapV);
        }

        if (aiGetMaterialInteger(&mat, AI_MATKEY_GLTF_MAPPINGFILTER_MAG(tt, slot), (int *)&filterMag) == AI_SUCCESS) {
            texture->sampler->magFilter = filterMag;
        }

        if (aiGetMaterialInteger(&mat, AI_MATKEY_GLTF_MAPPINGFILTER_MIN(tt, slot), (int *)&filterMin) == AI_SUCCESS) {
            texture->sampler->minFilter = filterMin;
        }

        aiString name;
        if (aiGetMaterialString(&mat, AI_MATKEY_GLTF_MAPPINGNAME(tt, slot), &name) == AI_SUCCESS) {
            texture->sampler->name = name.C_Str();
        }
    }
}

void glTF2Exporter::GetMatTexProp(const aiMaterial &mat, unsigned int &prop, const char *propName, aiTextureType tt, unsigned int slot) {
    std::string textureKey = std::string(_AI_MATKEY_TEXTURE_BASE) + "." + propName;

    mat.Get(textureKey.c_str(), tt, slot, prop);
}

void glTF2Exporter::GetMatTexProp(const aiMaterial &mat, float &prop, const char *propName, aiTextureType tt, unsigned int slot) {
    std::string textureKey = std::string(_AI_MATKEY_TEXTURE_BASE) + "." + propName;

    mat.Get(textureKey.c_str(), tt, slot, prop);
}

void glTF2Exporter::GetMatTex(const aiMaterial &mat, Ref<Texture> &texture, unsigned int &texCoord, aiTextureType tt, unsigned int slot = 0) {
    if (mat.GetTextureCount(tt) == 0) {
        return;
    }

    aiString tex;

    // Read texcoord (UV map index)
    mat.Get(AI_MATKEY_UVWSRC(tt, slot), texCoord);

    if (mat.Get(AI_MATKEY_TEXTURE(tt, slot), tex) == AI_SUCCESS) {
        std::string path = tex.C_Str();

        if (path.size() > 0) {
            std::map<std::string, unsigned int>::iterator it = mTexturesByPath.find(path);
            if (it != mTexturesByPath.end()) {
                texture = mAsset->textures.Get(it->second);
            }

            bool useBasisUniversal = false;
            if (!texture) {
                std::string texId = mAsset->FindUniqueID("", "texture");
                texture = mAsset->textures.Create(texId);
                mTexturesByPath[path] = texture.GetIndex();

                std::string imgId = mAsset->FindUniqueID("", "image");
                texture->source = mAsset->images.Create(imgId);

                const aiTexture *curTex = mScene->GetEmbeddedTexture(path.c_str());
                if (curTex != nullptr) { // embedded
                    texture->source->name = curTex->mFilename.C_Str();

                    //basisu: embedded ktx2, bu
                    if (curTex->achFormatHint[0]) {
                        std::string mimeType = "image/";
                        if (memcmp(curTex->achFormatHint, "jpg", 3) == 0)
                            mimeType += "jpeg";
                        else if (memcmp(curTex->achFormatHint, "ktx", 3) == 0) {
                            useBasisUniversal = true;
                            mimeType += "ktx";
                        } else if (memcmp(curTex->achFormatHint, "kx2", 3) == 0) {
                            useBasisUniversal = true;
                            mimeType += "ktx2";
                        } else if (memcmp(curTex->achFormatHint, "bu", 2) == 0) {
                            useBasisUniversal = true;
                            mimeType += "basis";
                        } else
                            mimeType += curTex->achFormatHint;
                        texture->source->mimeType = mimeType;
                    }

                    // The asset has its own buffer, see Image::SetData
                    //basisu: "image/ktx2", "image/basis" as is
                    texture->source->SetData(reinterpret_cast<uint8_t *>(curTex->pcData), curTex->mWidth, *mAsset);
                } else {
                    texture->source->uri = path;
                    if (texture->source->uri.find(".ktx") != std::string::npos ||
                            texture->source->uri.find(".basis") != std::string::npos) {
                        useBasisUniversal = true;
                    }
                }

                //basisu
                if (useBasisUniversal) {
                    mAsset->extensionsUsed.KHR_texture_basisu = true;
                    mAsset->extensionsRequired.KHR_texture_basisu = true;
                }

                GetTexSampler(mat, texture, tt, slot);
            }
        }
    }
}

void glTF2Exporter::GetMatTex(const aiMaterial &mat, TextureInfo &prop, aiTextureType tt, unsigned int slot = 0) {
    Ref<Texture> &texture = prop.texture;
    GetMatTex(mat, texture, prop.texCoord, tt, slot);
}

void glTF2Exporter::GetMatTex(const aiMaterial &mat, NormalTextureInfo &prop, aiTextureType tt, unsigned int slot = 0) {
    Ref<Texture> &texture = prop.texture;

    GetMatTex(mat, texture, prop.texCoord, tt, slot);

    if (texture) {
        //GetMatTexProp(mat, prop.texCoord, "texCoord", tt, slot);
        GetMatTexProp(mat, prop.scale, "scale", tt, slot);
    }
}

void glTF2Exporter::GetMatTex(const aiMaterial &mat, OcclusionTextureInfo &prop, aiTextureType tt, unsigned int slot = 0) {
    Ref<Texture> &texture = prop.texture;

    GetMatTex(mat, texture, prop.texCoord, tt, slot);

    if (texture) {
        //GetMatTexProp(mat, prop.texCoord, "texCoord", tt, slot);
        GetMatTexProp(mat, prop.strength, "strength", tt, slot);
    }
}

aiReturn glTF2Exporter::GetMatColor(const aiMaterial &mat, vec4 &prop, const char *propName, int type, int idx) const {
    aiColor4D col;
    aiReturn result = mat.Get(propName, type, idx, col);

    if (result == AI_SUCCESS) {
        prop[0] = col.r;
        prop[1] = col.g;
        prop[2] = col.b;
        prop[3] = col.a;
    }

    return result;
}

aiReturn glTF2Exporter::GetMatColor(const aiMaterial &mat, vec3 &prop, const char *propName, int type, int idx) const {
    aiColor3D col;
    aiReturn result = mat.Get(propName, type, idx, col);

    if (result == AI_SUCCESS) {
        prop[0] = col.r;
        prop[1] = col.g;
        prop[2] = col.b;
    }

    return result;
}

bool glTF2Exporter::GetMatSpecGloss(const aiMaterial &mat, glTF2::PbrSpecularGlossiness &pbrSG) {
    bool result = false;
    // If has Glossiness, a Specular Color or Specular Texture, use the KHR_materials_pbrSpecularGlossiness extension
    // NOTE: This extension is being considered for deprecation (Dec 2020), may be replaced by KHR_material_specular

    if (mat.Get(AI_MATKEY_GLOSSINESS_FACTOR, pbrSG.glossinessFactor) == AI_SUCCESS) {
        result = true;
    } else {
        // Don't have explicit glossiness, convert from pbr roughness or legacy shininess
        float shininess;
        if (mat.Get(AI_MATKEY_ROUGHNESS_FACTOR, shininess) == AI_SUCCESS) {
            pbrSG.glossinessFactor = 1.0f - shininess; // Extension defines this way
        } else if (mat.Get(AI_MATKEY_SHININESS, shininess) == AI_SUCCESS) {
            pbrSG.glossinessFactor = shininess / 1000;
        }
    }

    if (GetMatColor(mat, pbrSG.specularFactor, AI_MATKEY_COLOR_SPECULAR) == AI_SUCCESS) {
        result = true;
    }
    // Add any appropriate textures
    GetMatTex(mat, pbrSG.specularGlossinessTexture, aiTextureType_SPECULAR);

    result = result || pbrSG.specularGlossinessTexture.texture;

    if (result) {
        // Likely to always have diffuse
        GetMatTex(mat, pbrSG.diffuseTexture, aiTextureType_DIFFUSE);
        GetMatColor(mat, pbrSG.diffuseFactor, AI_MATKEY_COLOR_DIFFUSE);
    }

    return result;
}

bool glTF2Exporter::GetMatSheen(const aiMaterial &mat, glTF2::MaterialSheen &sheen) {
    // Return true if got any valid Sheen properties or textures
    if (GetMatColor(mat, sheen.sheenColorFactor, AI_MATKEY_SHEEN_COLOR_FACTOR) != aiReturn_SUCCESS) {
        return false;
    }

    // Default Sheen color factor {0,0,0} disables Sheen, so do not export
    if (sheen.sheenColorFactor[0] == defaultSheenFactor[0] && sheen.sheenColorFactor[1] == defaultSheenFactor[1] && sheen.sheenColorFactor[2] == defaultSheenFactor[2]) {
        return false;
    }

    mat.Get(AI_MATKEY_SHEEN_ROUGHNESS_FACTOR, sheen.sheenRoughnessFactor);

    GetMatTex(mat, sheen.sheenColorTexture, AI_MATKEY_SHEEN_COLOR_TEXTURE);
    GetMatTex(mat, sheen.sheenRoughnessTexture, AI_MATKEY_SHEEN_ROUGHNESS_TEXTURE);

    return true;
}

bool glTF2Exporter::GetMatClearcoat(const aiMaterial &mat, glTF2::MaterialClearcoat &clearcoat) {
    if (mat.Get(AI_MATKEY_CLEARCOAT_FACTOR, clearcoat.clearcoatFactor) != aiReturn_SUCCESS) {
        return false;
    }

    // Clearcoat factor of zero disables Clearcoat, so do not export
    if (clearcoat.clearcoatFactor == 0.0f)
        return false;

    mat.Get(AI_MATKEY_CLEARCOAT_ROUGHNESS_FACTOR, clearcoat.clearcoatRoughnessFactor);

    GetMatTex(mat, clearcoat.clearcoatTexture, AI_MATKEY_CLEARCOAT_TEXTURE);
    GetMatTex(mat, clearcoat.clearcoatRoughnessTexture, AI_MATKEY_CLEARCOAT_ROUGHNESS_TEXTURE);
    GetMatTex(mat, clearcoat.clearcoatNormalTexture, AI_MATKEY_CLEARCOAT_NORMAL_TEXTURE);

    return true;
}

bool glTF2Exporter::GetMatTransmission(const aiMaterial &mat, glTF2::MaterialTransmission &transmission) {
    bool result = mat.Get(AI_MATKEY_TRANSMISSION_FACTOR, transmission.transmissionFactor) == aiReturn_SUCCESS;
    GetMatTex(mat, transmission.transmissionTexture, AI_MATKEY_TRANSMISSION_TEXTURE);
    return result || transmission.transmissionTexture.texture;
}

bool glTF2Exporter::GetMatVolume(const aiMaterial &mat, glTF2::MaterialVolume &volume) {
    bool result = mat.Get(AI_MATKEY_VOLUME_THICKNESS_FACTOR, volume.thicknessFactor) != aiReturn_SUCCESS;

    GetMatTex(mat, volume.thicknessTexture, AI_MATKEY_VOLUME_THICKNESS_TEXTURE);

    result = result || mat.Get(AI_MATKEY_VOLUME_ATTENUATION_DISTANCE, volume.attenuationDistance);
    result = result || GetMatColor(mat, volume.attenuationColor, AI_MATKEY_VOLUME_ATTENUATION_COLOR) != aiReturn_SUCCESS;

    // Valid if any of these properties are available
    return result || volume.thicknessTexture.texture;
}

bool glTF2Exporter::GetMatIOR(const aiMaterial &mat, glTF2::MaterialIOR &ior) {
    return mat.Get(AI_MATKEY_REFRACTI, ior.ior) == aiReturn_SUCCESS;
}

bool glTF2Exporter::GetMatEmissiveStrength(const aiMaterial &mat, glTF2::MaterialEmissiveStrength &emissiveStrength) {
    return mat.Get(AI_MATKEY_EMISSIVE_INTENSITY, emissiveStrength.emissiveStrength) == aiReturn_SUCCESS;
}

void glTF2Exporter::ExportMaterials() {
    aiString aiName;
    for (unsigned int i = 0; i < mScene->mNumMaterials; ++i) {
        ai_assert(mScene->mMaterials[i] != nullptr);

        const aiMaterial &mat = *(mScene->mMaterials[i]);

        std::string id = "material_" + ai_to_string(i);

        Ref<Material> m = mAsset->materials.Create(id);

        std::string name;
        if (mat.Get(AI_MATKEY_NAME, aiName) == AI_SUCCESS) {
            name = aiName.C_Str();
        }
        name = mAsset->FindUniqueID(name, "material");

        m->name = name;

        GetMatTex(mat, m->pbrMetallicRoughness.baseColorTexture, aiTextureType_BASE_COLOR);

        if (!m->pbrMetallicRoughness.baseColorTexture.texture) {
            //if there wasn't a baseColorTexture defined in the source, fallback to any diffuse texture
            GetMatTex(mat, m->pbrMetallicRoughness.baseColorTexture, aiTextureType_DIFFUSE);
        }

        GetMatTex(mat, m->pbrMetallicRoughness.metallicRoughnessTexture, AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_METALLICROUGHNESS_TEXTURE);

        if (GetMatColor(mat, m->pbrMetallicRoughness.baseColorFactor, AI_MATKEY_BASE_COLOR) != AI_SUCCESS) {
            // if baseColorFactor wasn't defined, then the source is likely not a metallic roughness material.
            //a fallback to any diffuse color should be used instead
            GetMatColor(mat, m->pbrMetallicRoughness.baseColorFactor, AI_MATKEY_COLOR_DIFFUSE);
        }

        if (mat.Get(AI_MATKEY_METALLIC_FACTOR, m->pbrMetallicRoughness.metallicFactor) != AI_SUCCESS) {
            //if metallicFactor wasn't defined, then the source is likely not a PBR file, and the metallicFactor should be 0
            m->pbrMetallicRoughness.metallicFactor = 0;
        }

        // get roughness if source is gltf2 file
        if (mat.Get(AI_MATKEY_ROUGHNESS_FACTOR, m->pbrMetallicRoughness.roughnessFactor) != AI_SUCCESS) {
            // otherwise, try to derive and convert from specular + shininess values
            aiColor4D specularColor;
            ai_real shininess;

            if (mat.Get(AI_MATKEY_COLOR_SPECULAR, specularColor) == AI_SUCCESS && mat.Get(AI_MATKEY_SHININESS, shininess) == AI_SUCCESS) {
                // convert specular color to luminance
                float specularIntensity = specularColor[0] * 0.2125f + specularColor[1] * 0.7154f + specularColor[2] * 0.0721f;
                //normalize shininess (assuming max is 1000) with an inverse exponentional curve
                float normalizedShininess = std::sqrt(shininess / 1000);

                //clamp the shininess value between 0 and 1
                normalizedShininess = std::min(std::max(normalizedShininess, 0.0f), 1.0f);
                // low specular intensity values should produce a rough material even if shininess is high.
                normalizedShininess = normalizedShininess * specularIntensity;

                m->pbrMetallicRoughness.roughnessFactor = 1 - normalizedShininess;
            }
        }

        GetMatTex(mat, m->normalTexture, aiTextureType_NORMALS);
        GetMatTex(mat, m->occlusionTexture, aiTextureType_LIGHTMAP);
        GetMatTex(mat, m->emissiveTexture, aiTextureType_EMISSIVE);
        GetMatColor(mat, m->emissiveFactor, AI_MATKEY_COLOR_EMISSIVE);

        mat.Get(AI_MATKEY_TWOSIDED, m->doubleSided);
        mat.Get(AI_MATKEY_GLTF_ALPHACUTOFF, m->alphaCutoff);

        float opacity;
        aiString alphaMode;

        if (mat.Get(AI_MATKEY_OPACITY, opacity) == AI_SUCCESS) {
            if (opacity < 1) {
                m->alphaMode = "BLEND";
                m->pbrMetallicRoughness.baseColorFactor[3] *= opacity;
            }
        }
        if (mat.Get(AI_MATKEY_GLTF_ALPHAMODE, alphaMode) == AI_SUCCESS) {
            m->alphaMode = alphaMode.C_Str();
        }

        {
            // KHR_materials_pbrSpecularGlossiness extension
            // NOTE: This extension is being considered for deprecation (Dec 2020)
            PbrSpecularGlossiness pbrSG;
            if (GetMatSpecGloss(mat, pbrSG)) {
                mAsset->extensionsUsed.KHR_materials_pbrSpecularGlossiness = true;
                m->pbrSpecularGlossiness = Nullable<PbrSpecularGlossiness>(pbrSG);
            }
        }

        // glTFv2 is either PBR or Unlit
        aiShadingMode shadingMode = aiShadingMode_PBR_BRDF;
        mat.Get(AI_MATKEY_SHADING_MODEL, shadingMode);
        if (shadingMode == aiShadingMode_Unlit) {
            mAsset->extensionsUsed.KHR_materials_unlit = true;
            m->unlit = true;
        } else {
            // These extensions are not compatible with KHR_materials_unlit or KHR_materials_pbrSpecularGlossiness
            if (!m->pbrSpecularGlossiness.isPresent) {
                // Sheen
                MaterialSheen sheen;
                if (GetMatSheen(mat, sheen)) {
                    mAsset->extensionsUsed.KHR_materials_sheen = true;
                    m->materialSheen = Nullable<MaterialSheen>(sheen);
                }

                MaterialClearcoat clearcoat;
                if (GetMatClearcoat(mat, clearcoat)) {
                    mAsset->extensionsUsed.KHR_materials_clearcoat = true;
                    m->materialClearcoat = Nullable<MaterialClearcoat>(clearcoat);
                }

                MaterialTransmission transmission;
                if (GetMatTransmission(mat, transmission)) {
                    mAsset->extensionsUsed.KHR_materials_transmission = true;
                    m->materialTransmission = Nullable<MaterialTransmission>(transmission);
                }

                MaterialVolume volume;
                if (GetMatVolume(mat, volume)) {
                    mAsset->extensionsUsed.KHR_materials_volume = true;
                    m->materialVolume = Nullable<MaterialVolume>(volume);
                }

                MaterialIOR ior;
                if (GetMatIOR(mat, ior)) {
                    mAsset->extensionsUsed.KHR_materials_ior = true;
                    m->materialIOR = Nullable<MaterialIOR>(ior);
                }

                MaterialEmissiveStrength emissiveStrength;
                if (GetMatEmissiveStrength(mat, emissiveStrength)) {
                    mAsset->extensionsUsed.KHR_materials_emissive_strength = true;
                    m->materialEmissiveStrength = Nullable<MaterialEmissiveStrength>(emissiveStrength);
                }
            }
        }
    }
}

/*
 * Search through node hierarchy and find the node containing the given meshID.
 * Returns true on success, and false otherwise.
 */
bool FindMeshNode(Ref<Node> &nodeIn, Ref<Node> &meshNode, const std::string &meshID) {
    for (unsigned int i = 0; i < nodeIn->meshes.size(); ++i) {
        if (meshID.compare(nodeIn->meshes[i]->id) == 0) {
            meshNode = nodeIn;
            return true;
        }
    }

    for (unsigned int i = 0; i < nodeIn->children.size(); ++i) {
        if (FindMeshNode(nodeIn->children[i], meshNode, meshID)) {
            return true;
        }
    }

    return false;
}

/*
 * Find the root joint of the skeleton.
 * Starts will any joint node and traces up the tree,
 * until a parent is found that does not have a jointName.
 * Returns the first parent Ref<Node> found that does not have a jointName.
 */
Ref<Node> FindSkeletonRootJoint(Ref<Skin> &skinRef) {
    Ref<Node> startNodeRef;
    Ref<Node> parentNodeRef;

    // Arbitrarily use the first joint to start the search.
    startNodeRef = skinRef->jointNames[0];
    parentNodeRef = skinRef->jointNames[0];

    do {
        startNodeRef = parentNodeRef;
        parentNodeRef = startNodeRef->parent;
    } while (parentNodeRef && !parentNodeRef->jointName.empty());

    return parentNodeRef;
}

void ExportSkin(Asset &mAsset, const aiMesh *aimesh, Ref<Mesh> &meshRef, Ref<Buffer> &bufferRef, Ref<Skin> &skinRef,
        std::vector<aiMatrix4x4> &inverseBindMatricesData) {
    if (aimesh->mNumBones < 1) {
        return;
    }

    // Store the vertex joint and weight data.
    const size_t NumVerts(aimesh->mNumVertices);
    vec4 *vertexJointData = new vec4[NumVerts];
    vec4 *vertexWeightData = new vec4[NumVerts];
    int *jointsPerVertex = new int[NumVerts];
    for (size_t i = 0; i < NumVerts; ++i) {
        jointsPerVertex[i] = 0;
        for (size_t j = 0; j < 4; ++j) {
            vertexJointData[i][j] = 0;
            vertexWeightData[i][j] = 0;
        }
    }

    for (unsigned int idx_bone = 0; idx_bone < aimesh->mNumBones; ++idx_bone) {
        const aiBone *aib = aimesh->mBones[idx_bone];

        // aib->mName   =====>  skinRef->jointNames
        // Find the node with id = mName.
        Ref<Node> nodeRef = mAsset.nodes.Get(aib->mName.C_Str());
        nodeRef->jointName = nodeRef->name;

        unsigned int jointNamesIndex = 0;
        bool addJointToJointNames = true;
        for (unsigned int idx_joint = 0; idx_joint < skinRef->jointNames.size(); ++idx_joint) {
            if (skinRef->jointNames[idx_joint]->jointName.compare(nodeRef->jointName) == 0) {
                addJointToJointNames = false;
                jointNamesIndex = idx_joint;
            }
        }

        if (addJointToJointNames) {
            skinRef->jointNames.push_back(nodeRef);

            // aib->mOffsetMatrix   =====>  skinRef->inverseBindMatrices
            aiMatrix4x4 tmpMatrix4;
            CopyValue(aib->mOffsetMatrix, tmpMatrix4);
            inverseBindMatricesData.push_back(tmpMatrix4);
            jointNamesIndex = static_cast<unsigned int>(inverseBindMatricesData.size() - 1);
        }

        // aib->mWeights   =====>  vertexWeightData
        for (unsigned int idx_weights = 0; idx_weights < aib->mNumWeights; ++idx_weights) {
            unsigned int vertexId = aib->mWeights[idx_weights].mVertexId;
            float vertWeight = aib->mWeights[idx_weights].mWeight;

            // A vertex can only have at most four joint weights, which ideally sum up to 1
            if (IsBoneWeightFitted(vertexWeightData[vertexId])) {
                continue;
            }
            if (jointsPerVertex[vertexId] > 3) {
                int boneIndexFitted = FitBoneWeight(vertexWeightData[vertexId], vertWeight);
                if (boneIndexFitted != -1) {
                    vertexJointData[vertexId][boneIndexFitted] = static_cast<float>(jointNamesIndex);
                }
            }else {
                vertexJointData[vertexId][jointsPerVertex[vertexId]] = static_cast<float>(jointNamesIndex);
                vertexWeightData[vertexId][jointsPerVertex[vertexId]] = vertWeight;

                jointsPerVertex[vertexId] += 1;
            }
        }

    } // End: for-loop mNumMeshes

    Mesh::Primitive &p = meshRef->primitives.back();
    Ref<Accessor> vertexJointAccessor = ExportData(mAsset, skinRef->id, bufferRef, aimesh->mNumVertices,
        vertexJointData, AttribType::VEC4, AttribType::VEC4, ComponentType_FLOAT);
    if (vertexJointAccessor) {
        size_t offset = vertexJointAccessor->bufferView->byteOffset;
        size_t bytesLen = vertexJointAccessor->bufferView->byteLength;
        unsigned int s_bytesPerComp = ComponentTypeSize(ComponentType_UNSIGNED_SHORT);
        unsigned int bytesPerComp = ComponentTypeSize(vertexJointAccessor->componentType);
        size_t s_bytesLen = bytesLen * s_bytesPerComp / bytesPerComp;
        Ref<Buffer> buf = vertexJointAccessor->bufferView->buffer;
        uint8_t *arrys = new uint8_t[bytesLen];
        unsigned int i = 0;
        for (unsigned int j = 0; j < bytesLen; j += bytesPerComp) {
            size_t len_p = offset + j;
            float f_value = *(float *)&buf->GetPointer()[len_p];
            unsigned short c = static_cast<unsigned short>(f_value);
            memcpy(&arrys[i * s_bytesPerComp], &c, s_bytesPerComp);
            ++i;
        }
        buf->ReplaceData_joint(offset, bytesLen, arrys, bytesLen);
        vertexJointAccessor->componentType = ComponentType_UNSIGNED_SHORT;
        vertexJointAccessor->bufferView->byteLength = s_bytesLen;

        p.attributes.joint.push_back(vertexJointAccessor);
        delete[] arrys;
    }

    Ref<Accessor> vertexWeightAccessor = ExportData(mAsset, skinRef->id, bufferRef, aimesh->mNumVertices,
            vertexWeightData, AttribType::VEC4, AttribType::VEC4, ComponentType_FLOAT);
    if (vertexWeightAccessor) {
        p.attributes.weight.push_back(vertexWeightAccessor);
    }
    delete[] jointsPerVertex;
    delete[] vertexWeightData;
    delete[] vertexJointData;
}

void glTF2Exporter::ExportMeshes() {
    typedef decltype(aiFace::mNumIndices) IndicesType;

    std::string fname = std::string(mFilename);
    std::string bufferIdPrefix = fname.substr(0, fname.rfind(".gltf"));
    std::string bufferId = mAsset->FindUniqueID("", bufferIdPrefix.c_str());

    Ref<Buffer> b = mAsset->GetBodyBuffer();
    if (!b) {
        b = mAsset->buffers.Create(bufferId);
    }

    //----------------------------------------
    // Initialize variables for the skin
    bool createSkin = false;
    for (unsigned int idx_mesh = 0; idx_mesh < mScene->mNumMeshes; ++idx_mesh) {
        const aiMesh *aim = mScene->mMeshes[idx_mesh];
        if (aim->HasBones()) {
            createSkin = true;
            break;
        }
    }

    Ref<Skin> skinRef;
    std::string skinName = mAsset->FindUniqueID("skin", "skin");
    std::vector<aiMatrix4x4> inverseBindMatricesData;
    if (createSkin) {
        skinRef = mAsset->skins.Create(skinName);
        skinRef->name = skinName;
    }
    //----------------------------------------

    for (unsigned int idx_mesh = 0; idx_mesh < mScene->mNumMeshes; ++idx_mesh) {
        const aiMesh *aim = mScene->mMeshes[idx_mesh];

        std::string name = aim->mName.C_Str();

        std::string meshId = mAsset->FindUniqueID(name, "mesh");
        Ref<Mesh> m = mAsset->meshes.Create(meshId);
        m->primitives.resize(1);
        Mesh::Primitive &p = m->primitives.back();

        m->name = name;

        p.material = mAsset->materials.Get(aim->mMaterialIndex);
        p.ngonEncoded = (aim->mPrimitiveTypes & aiPrimitiveType_NGONEncodingFlag) != 0;

        /******************* Vertices ********************/
        Ref<Accessor> v = ExportData(*mAsset, meshId, b, aim->mNumVertices, aim->mVertices, AttribType::VEC3,
            AttribType::VEC3, ComponentType_FLOAT, BufferViewTarget_ARRAY_BUFFER);
        if (v) {
            p.attributes.position.push_back(v);
        }

        /******************** Normals ********************/
        // Normalize all normals as the validator can emit a warning otherwise
        if (nullptr != aim->mNormals) {
            for (auto i = 0u; i < aim->mNumVertices; ++i) {
                aim->mNormals[i].NormalizeSafe();
            }
        }

        Ref<Accessor> n = ExportData(*mAsset, meshId, b, aim->mNumVertices, aim->mNormals, AttribType::VEC3,
            AttribType::VEC3, ComponentType_FLOAT, BufferViewTarget_ARRAY_BUFFER);
        if (n) {
            p.attributes.normal.push_back(n);
        }

        /************** Texture coordinates **************/
        for (int i = 0; i < AI_MAX_NUMBER_OF_TEXTURECOORDS; ++i) {
            if (!aim->HasTextureCoords(i)) {
                continue;
            }

            // Flip UV y coords
            if (aim->mNumUVComponents[i] > 1) {
                for (unsigned int j = 0; j < aim->mNumVertices; ++j) {
                    aim->mTextureCoords[i][j].y = 1 - aim->mTextureCoords[i][j].y;
                }
            }

            if (aim->mNumUVComponents[i] > 0) {
                AttribType::Value type = (aim->mNumUVComponents[i] == 2) ? AttribType::VEC2 : AttribType::VEC3;

                Ref<Accessor> tc = ExportData(*mAsset, meshId, b, aim->mNumVertices, aim->mTextureCoords[i],
                    AttribType::VEC3, type, ComponentType_FLOAT, BufferViewTarget_ARRAY_BUFFER);
                if (tc) {
                    p.attributes.texcoord.push_back(tc);
                }
            }
        }

        /*************** Vertex colors ****************/
        for (unsigned int indexColorChannel = 0; indexColorChannel < aim->GetNumColorChannels(); ++indexColorChannel) {
            Ref<Accessor> c = ExportData(*mAsset, meshId, b, aim->mNumVertices, aim->mColors[indexColorChannel],
                AttribType::VEC4, AttribType::VEC4, ComponentType_FLOAT, BufferViewTarget_ARRAY_BUFFER);
            if (c) {
                p.attributes.color.push_back(c);
            }
        }

        /*************** Vertices indices ****************/
        if (aim->mNumFaces > 0) {
            std::vector<IndicesType> indices;
            unsigned int nIndicesPerFace = aim->mFaces[0].mNumIndices;
            indices.resize(aim->mNumFaces * nIndicesPerFace);
            for (size_t i = 0; i < aim->mNumFaces; ++i) {
                for (size_t j = 0; j < nIndicesPerFace; ++j) {
                    indices[i * nIndicesPerFace + j] = IndicesType(aim->mFaces[i].mIndices[j]);
                }
            }

            p.indices = ExportData(*mAsset, meshId, b, indices.size(), &indices[0], AttribType::SCALAR, AttribType::SCALAR,
                ComponentType_UNSIGNED_INT, BufferViewTarget_ELEMENT_ARRAY_BUFFER);
        }

        switch (aim->mPrimitiveTypes) {
        case aiPrimitiveType_POLYGON:
            p.mode = PrimitiveMode_TRIANGLES;
            break; // TODO implement this
        case aiPrimitiveType_LINE:
            p.mode = PrimitiveMode_LINES;
            break;
        case aiPrimitiveType_POINT:
            p.mode = PrimitiveMode_POINTS;
            break;
        default: // aiPrimitiveType_TRIANGLE
            p.mode = PrimitiveMode_TRIANGLES;
            break;
        }

        /*************** Skins ****************/
        if (aim->HasBones()) {
            ExportSkin(*mAsset, aim, m, b, skinRef, inverseBindMatricesData);
        }

        /*************** Targets for blendshapes ****************/
        if (aim->mNumAnimMeshes > 0) {
            bool bUseSparse = this->mProperties->HasPropertyBool("GLTF2_SPARSE_ACCESSOR_EXP") &&
                              this->mProperties->GetPropertyBool("GLTF2_SPARSE_ACCESSOR_EXP");
            bool bIncludeNormal = this->mProperties->HasPropertyBool("GLTF2_TARGET_NORMAL_EXP") &&
                                  this->mProperties->GetPropertyBool("GLTF2_TARGET_NORMAL_EXP");
            bool bExportTargetNames = this->mProperties->HasPropertyBool("GLTF2_TARGETNAMES_EXP") &&
                                      this->mProperties->GetPropertyBool("GLTF2_TARGETNAMES_EXP");

            p.targets.resize(aim->mNumAnimMeshes);
            for (unsigned int am = 0; am < aim->mNumAnimMeshes; ++am) {
                aiAnimMesh *pAnimMesh = aim->mAnimMeshes[am];
                if (bExportTargetNames) {
                    m->targetNames.emplace_back(pAnimMesh->mName.data);
                }
                // position
                if (pAnimMesh->HasPositions()) {
                    // NOTE: in gltf it is the diff stored
                    aiVector3D *pPositionDiff = new aiVector3D[pAnimMesh->mNumVertices];
                    for (unsigned int vt = 0; vt < pAnimMesh->mNumVertices; ++vt) {
                        pPositionDiff[vt] = pAnimMesh->mVertices[vt] - aim->mVertices[vt];
                    }
                    Ref<Accessor> vec;
                    if (bUseSparse) {
                        vec = ExportDataSparse(*mAsset, meshId, b,
                                pAnimMesh->mNumVertices, pPositionDiff,
                                AttribType::VEC3, AttribType::VEC3, ComponentType_FLOAT);
                    } else {
                        vec = ExportData(*mAsset, meshId, b,
                                pAnimMesh->mNumVertices, pPositionDiff,
                                AttribType::VEC3, AttribType::VEC3, ComponentType_FLOAT);
                    }
                    if (vec) {
                        p.targets[am].position.push_back(vec);
                    }
                    delete[] pPositionDiff;
                }

                // normal
                if (pAnimMesh->HasNormals() && bIncludeNormal) {
                    aiVector3D *pNormalDiff = new aiVector3D[pAnimMesh->mNumVertices];
                    for (unsigned int vt = 0; vt < pAnimMesh->mNumVertices; ++vt) {
                        pNormalDiff[vt] = pAnimMesh->mNormals[vt] - aim->mNormals[vt];
                    }
                    Ref<Accessor> vec;
                    if (bUseSparse) {
                        vec = ExportDataSparse(*mAsset, meshId, b,
                                pAnimMesh->mNumVertices, pNormalDiff,
                                AttribType::VEC3, AttribType::VEC3, ComponentType_FLOAT);
                    } else {
                        vec = ExportData(*mAsset, meshId, b,
                                pAnimMesh->mNumVertices, pNormalDiff,
                                AttribType::VEC3, AttribType::VEC3, ComponentType_FLOAT);
                    }
                    if (vec) {
                        p.targets[am].normal.push_back(vec);
                    }
                    delete[] pNormalDiff;
                }

                // tangent?
            }
        }
    }

    //----------------------------------------
    // Finish the skin
    // Create the Accessor for skinRef->inverseBindMatrices
    bool bAddCustomizedProperty = this->mProperties->HasPropertyBool("GLTF2_CUSTOMIZE_PROPERTY");
    if (createSkin) {
        mat4 *invBindMatrixData = new mat4[inverseBindMatricesData.size()];
        for (unsigned int idx_joint = 0; idx_joint < inverseBindMatricesData.size(); ++idx_joint) {
            CopyValue(inverseBindMatricesData[idx_joint], invBindMatrixData[idx_joint]);
        }

        Ref<Accessor> invBindMatrixAccessor = ExportData(*mAsset, skinName, b,
                static_cast<unsigned int>(inverseBindMatricesData.size()),
                invBindMatrixData, AttribType::MAT4, AttribType::MAT4, ComponentType_FLOAT);
        if (invBindMatrixAccessor) {
            skinRef->inverseBindMatrices = invBindMatrixAccessor;
        }

        // Identity Matrix   =====>  skinRef->bindShapeMatrix
        // Temporary. Hard-coded identity matrix here
        skinRef->bindShapeMatrix.isPresent = bAddCustomizedProperty;
        IdentityMatrix4(skinRef->bindShapeMatrix.value);

        // Find nodes that contain a mesh with bones and add "skeletons" and "skin" attributes to those nodes.
        Ref<Node> rootNode = mAsset->nodes.Get(unsigned(0));
        Ref<Node> meshNode;
        for (unsigned int meshIndex = 0; meshIndex < mAsset->meshes.Size(); ++meshIndex) {
            Ref<Mesh> mesh = mAsset->meshes.Get(meshIndex);
            bool hasBones = false;
            for (unsigned int i = 0; i < mesh->primitives.size(); ++i) {
                if (!mesh->primitives[i].attributes.weight.empty()) {
                    hasBones = true;
                    break;
                }
            }
            if (!hasBones) {
                continue;
            }
            std::string meshID = mesh->id;
            FindMeshNode(rootNode, meshNode, meshID);
            Ref<Node> rootJoint = FindSkeletonRootJoint(skinRef);
            if (bAddCustomizedProperty)
                meshNode->skeletons.push_back(rootJoint);
            meshNode->skin = skinRef;
        }
        delete[] invBindMatrixData;
    }
}

// Merges a node's multiple meshes (with one primitive each) into one mesh with multiple primitives
void glTF2Exporter::MergeMeshes() {
    for (unsigned int n = 0; n < mAsset->nodes.Size(); ++n) {
        Ref<Node> node = mAsset->nodes.Get(n);

        unsigned int nMeshes = static_cast<unsigned int>(node->meshes.size());

        //skip if it's 1 or less meshes per node
        if (nMeshes > 1) {
            Ref<Mesh> firstMesh = node->meshes.at(0);

            //loop backwards to allow easy removal of a mesh from a node once it's merged
            for (unsigned int m = nMeshes - 1; m >= 1; --m) {
                Ref<Mesh> mesh = node->meshes.at(m);

                //append this mesh's primitives to the first mesh's primitives
                firstMesh->primitives.insert(
                        firstMesh->primitives.end(),
                        mesh->primitives.begin(),
                        mesh->primitives.end());

                //remove the mesh from the list of meshes
                unsigned int removedIndex = mAsset->meshes.Remove(mesh->id.c_str());

                //find the presence of the removed mesh in other nodes
                for (unsigned int nn = 0; nn < mAsset->nodes.Size(); ++nn) {
                    Ref<Node> curNode = mAsset->nodes.Get(nn);

                    for (unsigned int mm = 0; mm < curNode->meshes.size(); ++mm) {
                        Ref<Mesh> &meshRef = curNode->meshes.at(mm);
                        unsigned int meshIndex = meshRef.GetIndex();

                        if (meshIndex == removedIndex) {
                            curNode->meshes.erase(curNode->meshes.begin() + mm);
                        } else if (meshIndex > removedIndex) {
                            Ref<Mesh> newMeshRef = mAsset->meshes.Get(meshIndex - 1);

                            meshRef = newMeshRef;
                        }
                    }
                }
            }

            //since we were looping backwards, reverse the order of merged primitives to their original order
            std::reverse(firstMesh->primitives.begin() + 1, firstMesh->primitives.end());
        }
    }
}

/*
 * Export the root node of the node hierarchy.
 * Calls ExportNode for all children.
 */
unsigned int glTF2Exporter::ExportNodeHierarchy(const aiNode *n) {
    Ref<Node> node = mAsset->nodes.Create(mAsset->FindUniqueID(n->mName.C_Str(), "node"));

    node->name = n->mName.C_Str();

    if (!n->mTransformation.IsIdentity()) {
        node->matrix.isPresent = true;
        CopyValue(n->mTransformation, node->matrix.value);
    }

    for (unsigned int i = 0; i < n->mNumMeshes; ++i) {
        node->meshes.emplace_back(mAsset->meshes.Get(n->mMeshes[i]));
    }

    for (unsigned int i = 0; i < n->mNumChildren; ++i) {
        unsigned int idx = ExportNode(n->mChildren[i], node);
        node->children.emplace_back(mAsset->nodes.Get(idx));
    }

    return node.GetIndex();
}

/*
 * Export node and recursively calls ExportNode for all children.
 * Since these nodes are not the root node, we also export the parent Ref<Node>
 */
unsigned int glTF2Exporter::ExportNode(const aiNode *n, Ref<Node> &parent) {
    std::string name = mAsset->FindUniqueID(n->mName.C_Str(), "node");
    Ref<Node> node = mAsset->nodes.Create(name);

    node->parent = parent;
    node->name = name;

    if (!n->mTransformation.IsIdentity()) {
        if (mScene->mNumAnimations > 0 || (mProperties && mProperties->HasPropertyBool("GLTF2_NODE_IN_TRS"))) {
            aiQuaternion quaternion;
            n->mTransformation.Decompose(*reinterpret_cast<aiVector3D *>(&node->scale.value), quaternion, *reinterpret_cast<aiVector3D *>(&node->translation.value));

            aiVector3D vector(static_cast<ai_real>(1.0f), static_cast<ai_real>(1.0f), static_cast<ai_real>(1.0f));
            if (!reinterpret_cast<aiVector3D *>(&node->scale.value)->Equal(vector)) {
                node->scale.isPresent = true;
            }
            if (!reinterpret_cast<aiVector3D *>(&node->translation.value)->Equal(vector)) {
                node->translation.isPresent = true;
            }
            node->rotation.isPresent = true;
            node->rotation.value[0] = quaternion.x;
            node->rotation.value[1] = quaternion.y;
            node->rotation.value[2] = quaternion.z;
            node->rotation.value[3] = quaternion.w;
            node->matrix.isPresent = false;
        } else {
            node->matrix.isPresent = true;
            CopyValue(n->mTransformation, node->matrix.value);
        }
    }

    for (unsigned int i = 0; i < n->mNumMeshes; ++i) {
        node->meshes.emplace_back(mAsset->meshes.Get(n->mMeshes[i]));
    }

    for (unsigned int i = 0; i < n->mNumChildren; ++i) {
        unsigned int idx = ExportNode(n->mChildren[i], node);
        node->children.emplace_back(mAsset->nodes.Get(idx));
    }

    return node.GetIndex();
}

void glTF2Exporter::ExportScene() {
    // Use the name of the scene if specified
    const std::string sceneName = (mScene->mName.length > 0) ? mScene->mName.C_Str() : "defaultScene";

    // Ensure unique
    Ref<Scene> scene = mAsset->scenes.Create(mAsset->FindUniqueID(sceneName, ""));

    // root node will be the first one exported (idx 0)
    if (mAsset->nodes.Size() > 0) {
        scene->nodes.emplace_back(mAsset->nodes.Get(0u));
    }

    // set as the default scene
    mAsset->scene = scene;
}

void glTF2Exporter::ExportMetadata() {
    AssetMetadata &asset = mAsset->asset;
    asset.version = "2.0";

    char buffer[256];
    ai_snprintf(buffer, 256, "Open Asset Import Library (assimp v%d.%d.%x)",
            aiGetVersionMajor(), aiGetVersionMinor(), aiGetVersionRevision());

    asset.generator = buffer;

    // Copyright
    aiString copyright_str;
    if (mScene->mMetaData != nullptr && mScene->mMetaData->Get(AI_METADATA_SOURCE_COPYRIGHT, copyright_str)) {
        asset.copyright = copyright_str.C_Str();
    }
}

inline Ref<Accessor> GetSamplerInputRef(Asset &asset, std::string &animId, Ref<Buffer> &buffer, std::vector<ai_real> &times) {
    return ExportData(asset, animId, buffer, (unsigned int)times.size(), &times[0], AttribType::SCALAR, AttribType::SCALAR, ComponentType_FLOAT);
}

inline void ExtractTranslationSampler(Asset &asset, std::string &animId, Ref<Buffer> &buffer, const aiNodeAnim *nodeChannel, float ticksPerSecond, Animation::Sampler &sampler) {
    const unsigned int numKeyframes = nodeChannel->mNumPositionKeys;

    std::vector<ai_real> times(numKeyframes);
    std::vector<ai_real> values(numKeyframes * 3);
    for (unsigned int i = 0; i < numKeyframes; ++i) {
        const aiVectorKey &key = nodeChannel->mPositionKeys[i];
        // mTime is measured in ticks, but GLTF time is measured in seconds, so convert.
        times[i] = static_cast<float>(key.mTime / ticksPerSecond);
        values[(i * 3) + 0] = (ai_real) key.mValue.x;
        values[(i * 3) + 1] = (ai_real) key.mValue.y;
        values[(i * 3) + 2] = (ai_real) key.mValue.z;
    }

    sampler.input = GetSamplerInputRef(asset, animId, buffer, times);
    sampler.output = ExportData(asset, animId, buffer, numKeyframes, &values[0], AttribType::VEC3, AttribType::VEC3, ComponentType_FLOAT);
    sampler.interpolation = Interpolation_LINEAR;
}

inline void ExtractScaleSampler(Asset &asset, std::string &animId, Ref<Buffer> &buffer, const aiNodeAnim *nodeChannel, float ticksPerSecond, Animation::Sampler &sampler) {
    const unsigned int numKeyframes = nodeChannel->mNumScalingKeys;

    std::vector<ai_real> times(numKeyframes);
    std::vector<ai_real> values(numKeyframes * 3);
    for (unsigned int i = 0; i < numKeyframes; ++i) {
        const aiVectorKey &key = nodeChannel->mScalingKeys[i];
        // mTime is measured in ticks, but GLTF time is measured in seconds, so convert.
        times[i] = static_cast<float>(key.mTime / ticksPerSecond);
        values[(i * 3) + 0] = (ai_real) key.mValue.x;
        values[(i * 3) + 1] = (ai_real) key.mValue.y;
        values[(i * 3) + 2] = (ai_real) key.mValue.z;
    }

    sampler.input = GetSamplerInputRef(asset, animId, buffer, times);
    sampler.output = ExportData(asset, animId, buffer, numKeyframes, &values[0], AttribType::VEC3, AttribType::VEC3, ComponentType_FLOAT);
    sampler.interpolation = Interpolation_LINEAR;
}

inline void ExtractRotationSampler(Asset &asset, std::string &animId, Ref<Buffer> &buffer, const aiNodeAnim *nodeChannel, float ticksPerSecond, Animation::Sampler &sampler) {
    const unsigned int numKeyframes = nodeChannel->mNumRotationKeys;

    std::vector<ai_real> times(numKeyframes);
    std::vector<ai_real> values(numKeyframes * 4);
    for (unsigned int i = 0; i < numKeyframes; ++i) {
        const aiQuatKey &key = nodeChannel->mRotationKeys[i];
        // mTime is measured in ticks, but GLTF time is measured in seconds, so convert.
        times[i] = static_cast<float>(key.mTime / ticksPerSecond);
        values[(i * 4) + 0] = (ai_real) key.mValue.x;
        values[(i * 4) + 1] = (ai_real) key.mValue.y;
        values[(i * 4) + 2] = (ai_real) key.mValue.z;
        values[(i * 4) + 3] = (ai_real) key.mValue.w;
    }

    sampler.input = GetSamplerInputRef(asset, animId, buffer, times);
    sampler.output = ExportData(asset, animId, buffer, numKeyframes, &values[0], AttribType::VEC4, AttribType::VEC4, ComponentType_FLOAT);
    sampler.interpolation = Interpolation_LINEAR;
}

static void AddSampler(Ref<Animation> &animRef, Ref<Node> &nodeRef, Animation::Sampler &sampler, AnimationPath path) {
    Animation::Channel channel;
    channel.sampler = static_cast<int>(animRef->samplers.size());
    channel.target.path = path;
    channel.target.node = nodeRef;
    animRef->channels.push_back(channel);
    animRef->samplers.push_back(sampler);
}

void glTF2Exporter::ExportAnimations() {
    Ref<Buffer> bufferRef = mAsset->buffers.Get(unsigned(0));

    for (unsigned int i = 0; i < mScene->mNumAnimations; ++i) {
        const aiAnimation *anim = mScene->mAnimations[i];
        const float ticksPerSecond = static_cast<float>(anim->mTicksPerSecond);

        std::string nameAnim = "anim";
        if (anim->mName.length > 0) {
            nameAnim = anim->mName.C_Str();
        }
        Ref<Animation> animRef = mAsset->animations.Create(nameAnim);
        animRef->name = nameAnim;

        for (unsigned int channelIndex = 0; channelIndex < anim->mNumChannels; ++channelIndex) {
            const aiNodeAnim *nodeChannel = anim->mChannels[channelIndex];

            std::string name = nameAnim + "_" + ai_to_string(channelIndex);
            name = mAsset->FindUniqueID(name, "animation");

            Ref<Node> animNode = mAsset->nodes.Get(nodeChannel->mNodeName.C_Str());

            if (nodeChannel->mNumPositionKeys > 0) {
                Animation::Sampler translationSampler;
                ExtractTranslationSampler(*mAsset, name, bufferRef, nodeChannel, ticksPerSecond, translationSampler);
                AddSampler(animRef, animNode, translationSampler, AnimationPath_TRANSLATION);
            }

            if (nodeChannel->mNumRotationKeys > 0) {
                Animation::Sampler rotationSampler;
                ExtractRotationSampler(*mAsset, name, bufferRef, nodeChannel, ticksPerSecond, rotationSampler);
                AddSampler(animRef, animNode, rotationSampler, AnimationPath_ROTATION);
            }

            if (nodeChannel->mNumScalingKeys > 0) {
                Animation::Sampler scaleSampler;
                ExtractScaleSampler(*mAsset, name, bufferRef, nodeChannel, ticksPerSecond, scaleSampler);
                AddSampler(animRef, animNode, scaleSampler, AnimationPath_SCALE);
            }
        }
    } // End: for-loop mNumAnimations
}

#endif // ASSIMP_BUILD_NO_GLTF_EXPORTER
#endif // ASSIMP_BUILD_NO_EXPORT
