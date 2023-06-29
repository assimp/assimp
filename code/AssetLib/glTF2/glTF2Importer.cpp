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

#if !defined(ASSIMP_BUILD_NO_GLTF_IMPORTER) && !defined(ASSIMP_BUILD_NO_GLTF2_IMPORTER)

#include "AssetLib/glTF2/glTF2Importer.h"
#include "AssetLib/glTF2/glTF2Asset.h"
#include "PostProcessing/MakeVerboseFormat.h"

#if !defined(ASSIMP_BUILD_NO_EXPORT)
#include "AssetLib/glTF2/glTF2AssetWriter.h"
#endif

#include <assimp/CreateAnimMesh.h>
#include <assimp/DefaultIOSystem.h>
#include <assimp/StringComparison.h>
#include <assimp/StringUtils.h>
#include <assimp/ai_assert.h>
#include <assimp/commonMetaData.h>
#include <assimp/importerdesc.h>
#include <assimp/scene.h>
#include <assimp/DefaultLogger.hpp>
#include <assimp/Importer.hpp>

#include <memory>
#include <unordered_map>

#include <rapidjson/document.h>
#include <rapidjson/rapidjson.h>

using namespace Assimp;
using namespace glTF2;
using namespace glTFCommon;

namespace {
// generate bi-tangents from normals and tangents according to spec
struct Tangent {
    aiVector3D xyz;
    ai_real w;
};
} // namespace

//
// glTF2Importer
//

static const aiImporterDesc desc = {
    "glTF2 Importer",
    "",
    "",
    "",
    aiImporterFlags_SupportTextFlavour | aiImporterFlags_SupportBinaryFlavour | aiImporterFlags_LimitedSupport | aiImporterFlags_Experimental,
    0,
    0,
    0,
    0,
    "gltf glb"
};

glTF2Importer::glTF2Importer() :
        mScene(nullptr) {
    // empty
}

const aiImporterDesc *glTF2Importer::GetInfo() const {
    return &desc;
}

bool glTF2Importer::CanRead(const std::string &filename, IOSystem *pIOHandler, bool checkSig) const {
    const std::string extension = GetExtension(filename);
    if (!checkSig && (extension != "gltf") && (extension != "glb")) {
        return false;
    }

    if (pIOHandler) {
        glTF2::Asset asset(pIOHandler);
        return asset.CanRead(
            filename,
            CheckMagicToken(
                pIOHandler, filename, AI_GLB_MAGIC_NUMBER, 1, 0,
                static_cast<unsigned int>(strlen(AI_GLB_MAGIC_NUMBER))));
    }

    return false;
}

static inline aiTextureMapMode ConvertWrappingMode(SamplerWrap gltfWrapMode) {
    switch (gltfWrapMode) {
    case SamplerWrap::Mirrored_Repeat:
        return aiTextureMapMode_Mirror;

    case SamplerWrap::Clamp_To_Edge:
        return aiTextureMapMode_Clamp;

    case SamplerWrap::UNSET:
    case SamplerWrap::Repeat:
    default:
        return aiTextureMapMode_Wrap;
    }
}

static inline void SetMaterialColorProperty(Asset & /*r*/, vec4 &prop, aiMaterial *mat,
        const char *pKey, unsigned int type, unsigned int idx) {
    aiColor4D col;
    CopyValue(prop, col);
    mat->AddProperty(&col, 1, pKey, type, idx);
}

static inline void SetMaterialColorProperty(Asset & /*r*/, vec3 &prop, aiMaterial *mat,
        const char *pKey, unsigned int type, unsigned int idx) {
    aiColor4D col;
    glTFCommon::CopyValue(prop, col);
    mat->AddProperty(&col, 1, pKey, type, idx);
}

static void SetMaterialTextureProperty(std::vector<int> &embeddedTexIdxs, Asset & /*r*/,
        glTF2::TextureInfo prop, aiMaterial *mat, aiTextureType texType,
        unsigned int texSlot = 0) {
    if (prop.texture && prop.texture->source) {
        aiString uri(prop.texture->source->uri);

        int texIdx = embeddedTexIdxs[prop.texture->source.GetIndex()];
        if (texIdx != -1) { // embedded
            // setup texture reference string (copied from ColladaLoader::FindFilenameForEffectTexture)
            uri.data[0] = '*';
            uri.length = 1 + ASSIMP_itoa10(uri.data + 1, MAXLEN - 1, texIdx);
        }

        mat->AddProperty(&uri, AI_MATKEY_TEXTURE(texType, texSlot));
        const int uvIndex = static_cast<int>(prop.texCoord);
        mat->AddProperty(&uvIndex, 1, AI_MATKEY_UVWSRC(texType, texSlot));

        if (prop.textureTransformSupported) {
            aiUVTransform transform;
            transform.mScaling.x = prop.TextureTransformExt_t.scale[0];
            transform.mScaling.y = prop.TextureTransformExt_t.scale[1];
            transform.mRotation = -prop.TextureTransformExt_t.rotation; // must be negated

            // A change of coordinates is required to map glTF UV transformations into the space used by
            // Assimp. In glTF all UV origins are at 0,1 (top left of texture) in Assimp space. In Assimp
            // rotation occurs around the image center (0.5,0.5) where as in glTF rotation is around the
            // texture origin. All three can be corrected for solely by a change of the translation since
            // the transformations available are shape preserving. Note the importer already flips the V
            // coordinate of the actual meshes during import.
            const ai_real rcos(cos(-transform.mRotation));
            const ai_real rsin(sin(-transform.mRotation));
            transform.mTranslation.x = (static_cast<ai_real>(0.5) * transform.mScaling.x) * (-rcos + rsin + 1) + prop.TextureTransformExt_t.offset[0];
            transform.mTranslation.y = ((static_cast<ai_real>(0.5) * transform.mScaling.y) * (rsin + rcos - 1)) + 1 - transform.mScaling.y - prop.TextureTransformExt_t.offset[1];

            mat->AddProperty(&transform, 1, _AI_MATKEY_UVTRANSFORM_BASE, texType, texSlot);
        }

        if (prop.texture->sampler) {
            Ref<Sampler> sampler = prop.texture->sampler;

            aiString name(sampler->name);
            aiString id(sampler->id);

            mat->AddProperty(&name, AI_MATKEY_GLTF_MAPPINGNAME(texType, texSlot));
            mat->AddProperty(&id, AI_MATKEY_GLTF_MAPPINGID(texType, texSlot));

            aiTextureMapMode wrapS = ConvertWrappingMode(sampler->wrapS);
            aiTextureMapMode wrapT = ConvertWrappingMode(sampler->wrapT);
            mat->AddProperty(&wrapS, 1, AI_MATKEY_MAPPINGMODE_U(texType, texSlot));
            mat->AddProperty(&wrapT, 1, AI_MATKEY_MAPPINGMODE_V(texType, texSlot));

            if (sampler->magFilter != SamplerMagFilter::UNSET) {
                mat->AddProperty(&sampler->magFilter, 1, AI_MATKEY_GLTF_MAPPINGFILTER_MAG(texType, texSlot));
            }

            if (sampler->minFilter != SamplerMinFilter::UNSET) {
                mat->AddProperty(&sampler->minFilter, 1, AI_MATKEY_GLTF_MAPPINGFILTER_MIN(texType, texSlot));
            }
        } else {
            // Use glTFv2 default sampler
            const aiTextureMapMode default_wrap = aiTextureMapMode_Wrap;
            mat->AddProperty(&default_wrap, 1, AI_MATKEY_MAPPINGMODE_U(texType, texSlot));
            mat->AddProperty(&default_wrap, 1, AI_MATKEY_MAPPINGMODE_V(texType, texSlot));
        }
    }
}

inline void SetMaterialTextureProperty(std::vector<int> &embeddedTexIdxs, Asset &r,
        NormalTextureInfo &prop, aiMaterial *mat, aiTextureType texType,
        unsigned int texSlot = 0) {
    SetMaterialTextureProperty(embeddedTexIdxs, r, (glTF2::TextureInfo)prop, mat, texType, texSlot);

    if (prop.texture && prop.texture->source) {
        mat->AddProperty(&prop.scale, 1, AI_MATKEY_GLTF_TEXTURE_SCALE(texType, texSlot));
    }
}

inline void SetMaterialTextureProperty(std::vector<int> &embeddedTexIdxs, Asset &r,
        OcclusionTextureInfo &prop, aiMaterial *mat, aiTextureType texType,
        unsigned int texSlot = 0) {
    SetMaterialTextureProperty(embeddedTexIdxs, r, (glTF2::TextureInfo)prop, mat, texType, texSlot);

    if (prop.texture && prop.texture->source) {
        mat->AddProperty(&prop.strength, 1, AI_MATKEY_GLTF_TEXTURE_STRENGTH(texType, texSlot));
    }
}

static aiMaterial *ImportMaterial(std::vector<int> &embeddedTexIdxs, Asset &r, Material &mat) {
    aiMaterial *aimat = new aiMaterial();

    try {
        if (!mat.name.empty()) {
            aiString str(mat.name);

            aimat->AddProperty(&str, AI_MATKEY_NAME);
        }

        // Set Assimp DIFFUSE and BASE COLOR to the pbrMetallicRoughness base color and texture for backwards compatibility
        // Technically should not load any pbrMetallicRoughness if extensionsRequired contains KHR_materials_pbrSpecularGlossiness
        SetMaterialColorProperty(r, mat.pbrMetallicRoughness.baseColorFactor, aimat, AI_MATKEY_COLOR_DIFFUSE);
        SetMaterialColorProperty(r, mat.pbrMetallicRoughness.baseColorFactor, aimat, AI_MATKEY_BASE_COLOR);

        SetMaterialTextureProperty(embeddedTexIdxs, r, mat.pbrMetallicRoughness.baseColorTexture, aimat, aiTextureType_DIFFUSE);
        SetMaterialTextureProperty(embeddedTexIdxs, r, mat.pbrMetallicRoughness.baseColorTexture, aimat, aiTextureType_BASE_COLOR);

        // Keep AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_METALLICROUGHNESS_TEXTURE for backwards compatibility
        SetMaterialTextureProperty(embeddedTexIdxs, r, mat.pbrMetallicRoughness.metallicRoughnessTexture, aimat, AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_METALLICROUGHNESS_TEXTURE);
        SetMaterialTextureProperty(embeddedTexIdxs, r, mat.pbrMetallicRoughness.metallicRoughnessTexture, aimat, aiTextureType_METALNESS);
        SetMaterialTextureProperty(embeddedTexIdxs, r, mat.pbrMetallicRoughness.metallicRoughnessTexture, aimat, aiTextureType_DIFFUSE_ROUGHNESS);

        aimat->AddProperty(&mat.pbrMetallicRoughness.metallicFactor, 1, AI_MATKEY_METALLIC_FACTOR);
        aimat->AddProperty(&mat.pbrMetallicRoughness.roughnessFactor, 1, AI_MATKEY_ROUGHNESS_FACTOR);

        float roughnessAsShininess = 1 - mat.pbrMetallicRoughness.roughnessFactor;
        roughnessAsShininess *= roughnessAsShininess * 1000;
        aimat->AddProperty(&roughnessAsShininess, 1, AI_MATKEY_SHININESS);

        SetMaterialTextureProperty(embeddedTexIdxs, r, mat.normalTexture, aimat, aiTextureType_NORMALS);
        SetMaterialTextureProperty(embeddedTexIdxs, r, mat.occlusionTexture, aimat, aiTextureType_LIGHTMAP);
        SetMaterialTextureProperty(embeddedTexIdxs, r, mat.emissiveTexture, aimat, aiTextureType_EMISSIVE);
        SetMaterialColorProperty(r, mat.emissiveFactor, aimat, AI_MATKEY_COLOR_EMISSIVE);

        aimat->AddProperty(&mat.doubleSided, 1, AI_MATKEY_TWOSIDED);
        aimat->AddProperty(&mat.pbrMetallicRoughness.baseColorFactor[3], 1, AI_MATKEY_OPACITY);

        aiString alphaMode(mat.alphaMode);
        aimat->AddProperty(&alphaMode, AI_MATKEY_GLTF_ALPHAMODE);
        aimat->AddProperty(&mat.alphaCutoff, 1, AI_MATKEY_GLTF_ALPHACUTOFF);

        // KHR_materials_specular
        if (mat.materialSpecular.isPresent) {
            MaterialSpecular &specular = mat.materialSpecular.value;
            // Default values of zero disables Specular
            if (std::memcmp(specular.specularColorFactor, defaultSpecularColorFactor, sizeof(glTFCommon::vec3)) != 0 || specular.specularFactor != 0.0f) {
                SetMaterialColorProperty(r, specular.specularColorFactor, aimat, AI_MATKEY_COLOR_SPECULAR);
                aimat->AddProperty(&specular.specularFactor, 1, AI_MATKEY_SPECULAR_FACTOR);
                SetMaterialTextureProperty(embeddedTexIdxs, r, specular.specularTexture, aimat, aiTextureType_SPECULAR);
                SetMaterialTextureProperty(embeddedTexIdxs, r, specular.specularColorTexture, aimat, aiTextureType_SPECULAR);
            }
        }
        // pbrSpecularGlossiness
        else if (mat.pbrSpecularGlossiness.isPresent) {
            PbrSpecularGlossiness &pbrSG = mat.pbrSpecularGlossiness.value;

            SetMaterialColorProperty(r, pbrSG.diffuseFactor, aimat, AI_MATKEY_COLOR_DIFFUSE);
            SetMaterialColorProperty(r, pbrSG.specularFactor, aimat, AI_MATKEY_COLOR_SPECULAR);

            float glossinessAsShininess = pbrSG.glossinessFactor * 1000.0f;
            aimat->AddProperty(&glossinessAsShininess, 1, AI_MATKEY_SHININESS);
            aimat->AddProperty(&pbrSG.glossinessFactor, 1, AI_MATKEY_GLOSSINESS_FACTOR);

            SetMaterialTextureProperty(embeddedTexIdxs, r, pbrSG.diffuseTexture, aimat, aiTextureType_DIFFUSE);

            SetMaterialTextureProperty(embeddedTexIdxs, r, pbrSG.specularGlossinessTexture, aimat, aiTextureType_SPECULAR);
        }

        // glTFv2 is either PBR or Unlit
        aiShadingMode shadingMode = aiShadingMode_PBR_BRDF;
        if (mat.unlit) {
            aimat->AddProperty(&mat.unlit, 1, "$mat.gltf.unlit", 0, 0); // TODO: Remove this property, it is kept for backwards compatibility with assimp 5.0.1
            shadingMode = aiShadingMode_Unlit;
        }

        aimat->AddProperty(&shadingMode, 1, AI_MATKEY_SHADING_MODEL);

        // KHR_materials_sheen
        if (mat.materialSheen.isPresent) {
            MaterialSheen &sheen = mat.materialSheen.value;
            // Default value {0,0,0} disables Sheen
            if (std::memcmp(sheen.sheenColorFactor, defaultSheenFactor, sizeof(glTFCommon::vec3)) != 0) {
                SetMaterialColorProperty(r, sheen.sheenColorFactor, aimat, AI_MATKEY_SHEEN_COLOR_FACTOR);
                aimat->AddProperty(&sheen.sheenRoughnessFactor, 1, AI_MATKEY_SHEEN_ROUGHNESS_FACTOR);
                SetMaterialTextureProperty(embeddedTexIdxs, r, sheen.sheenColorTexture, aimat, AI_MATKEY_SHEEN_COLOR_TEXTURE);
                SetMaterialTextureProperty(embeddedTexIdxs, r, sheen.sheenRoughnessTexture, aimat, AI_MATKEY_SHEEN_ROUGHNESS_TEXTURE);
            }
        }

        // KHR_materials_clearcoat
        if (mat.materialClearcoat.isPresent) {
            MaterialClearcoat &clearcoat = mat.materialClearcoat.value;
            // Default value 0.0 disables clearcoat
            if (clearcoat.clearcoatFactor != 0.0f) {
                aimat->AddProperty(&clearcoat.clearcoatFactor, 1, AI_MATKEY_CLEARCOAT_FACTOR);
                aimat->AddProperty(&clearcoat.clearcoatRoughnessFactor, 1, AI_MATKEY_CLEARCOAT_ROUGHNESS_FACTOR);
                SetMaterialTextureProperty(embeddedTexIdxs, r, clearcoat.clearcoatTexture, aimat, AI_MATKEY_CLEARCOAT_TEXTURE);
                SetMaterialTextureProperty(embeddedTexIdxs, r, clearcoat.clearcoatRoughnessTexture, aimat, AI_MATKEY_CLEARCOAT_ROUGHNESS_TEXTURE);
                SetMaterialTextureProperty(embeddedTexIdxs, r, clearcoat.clearcoatNormalTexture, aimat, AI_MATKEY_CLEARCOAT_NORMAL_TEXTURE);
            }
        }

        // KHR_materials_transmission
        if (mat.materialTransmission.isPresent) {
            MaterialTransmission &transmission = mat.materialTransmission.value;

            aimat->AddProperty(&transmission.transmissionFactor, 1, AI_MATKEY_TRANSMISSION_FACTOR);
            SetMaterialTextureProperty(embeddedTexIdxs, r, transmission.transmissionTexture, aimat, AI_MATKEY_TRANSMISSION_TEXTURE);
        }

        // KHR_materials_volume
        if (mat.materialVolume.isPresent) {
            MaterialVolume &volume = mat.materialVolume.value;

            aimat->AddProperty(&volume.thicknessFactor, 1, AI_MATKEY_VOLUME_THICKNESS_FACTOR);
            SetMaterialTextureProperty(embeddedTexIdxs, r, volume.thicknessTexture, aimat, AI_MATKEY_VOLUME_THICKNESS_TEXTURE);
            aimat->AddProperty(&volume.attenuationDistance, 1, AI_MATKEY_VOLUME_ATTENUATION_DISTANCE);
            SetMaterialColorProperty(r, volume.attenuationColor, aimat, AI_MATKEY_VOLUME_ATTENUATION_COLOR);
        }

        // KHR_materials_ior
        if (mat.materialIOR.isPresent) {
            MaterialIOR &ior = mat.materialIOR.value;

            aimat->AddProperty(&ior.ior, 1, AI_MATKEY_REFRACTI);
        }

        // KHR_materials_emissive_strength
        if (mat.materialEmissiveStrength.isPresent) {
            MaterialEmissiveStrength &emissiveStrength = mat.materialEmissiveStrength.value;

            aimat->AddProperty(&emissiveStrength.emissiveStrength, 1, AI_MATKEY_EMISSIVE_INTENSITY);
        }

        return aimat;
    } catch (...) {
        delete aimat;
        throw;
    }
}

void glTF2Importer::ImportMaterials(Asset &r) {
    const unsigned int numImportedMaterials = unsigned(r.materials.Size());
    ASSIMP_LOG_DEBUG("Importing ", numImportedMaterials, " materials");
    Material defaultMaterial;

    mScene->mNumMaterials = numImportedMaterials + 1;
    mScene->mMaterials = new aiMaterial *[mScene->mNumMaterials];
    std::fill(mScene->mMaterials, mScene->mMaterials + mScene->mNumMaterials, nullptr);
    mScene->mMaterials[numImportedMaterials] = ImportMaterial(mEmbeddedTexIdxs, r, defaultMaterial);

    for (unsigned int i = 0; i < numImportedMaterials; ++i) {
        mScene->mMaterials[i] = ImportMaterial(mEmbeddedTexIdxs, r, r.materials[i]);
    }
}

static inline void SetFaceAndAdvance1(aiFace *&face, unsigned int numVertices, unsigned int a) {
    if (a >= numVertices) {
        return;
    }
    face->mNumIndices = 1;
    face->mIndices = new unsigned int[1];
    face->mIndices[0] = a;
    ++face;
}

static inline void SetFaceAndAdvance2(aiFace *&face, unsigned int numVertices,
        unsigned int a, unsigned int b) {
    if ((a >= numVertices) || (b >= numVertices)) {
        return;
    }
    face->mNumIndices = 2;
    face->mIndices = new unsigned int[2];
    face->mIndices[0] = a;
    face->mIndices[1] = b;
    ++face;
}

static inline void SetFaceAndAdvance3(aiFace *&face, unsigned int numVertices, unsigned int a,
        unsigned int b, unsigned int c) {
    if ((a >= numVertices) || (b >= numVertices) || (c >= numVertices)) {
        return;
    }
    face->mNumIndices = 3;
    face->mIndices = new unsigned int[3];
    face->mIndices[0] = a;
    face->mIndices[1] = b;
    face->mIndices[2] = c;
    ++face;
}

#ifdef ASSIMP_BUILD_DEBUG
static inline bool CheckValidFacesIndices(aiFace *faces, unsigned nFaces, unsigned nVerts) {
    for (unsigned i = 0; i < nFaces; ++i) {
        for (unsigned j = 0; j < faces[i].mNumIndices; ++j) {
            unsigned idx = faces[i].mIndices[j];
            if (idx >= nVerts) {
                return false;
            }
        }
    }
    return true;
}
#endif // ASSIMP_BUILD_DEBUG

template <typename T>
aiColor4D *GetVertexColorsForType(Ref<Accessor> input, std::vector<unsigned int> *vertexRemappingTable) {
    constexpr float max = std::numeric_limits<T>::max();
    aiColor4t<T> *colors;
    input->ExtractData(colors, vertexRemappingTable);
    auto output = new aiColor4D[input->count];
    for (size_t i = 0; i < input->count; i++) {
        output[i] = aiColor4D(
                colors[i].r / max, colors[i].g / max,
                colors[i].b / max, colors[i].a / max);
    }
    delete[] colors;
    return output;
}

void glTF2Importer::ImportMeshes(glTF2::Asset &r) {
    ASSIMP_LOG_DEBUG("Importing ", r.meshes.Size(), " meshes");
    std::vector<std::unique_ptr<aiMesh>> meshes;

    meshOffsets.clear();
    meshOffsets.reserve(r.meshes.Size() + 1);
    mVertexRemappingTables.clear();

    // Count the number of aiMeshes
    unsigned int num_aiMeshes = 0;
    for (unsigned int m = 0; m < r.meshes.Size(); ++m) {
        meshOffsets.push_back(num_aiMeshes);
        num_aiMeshes += unsigned(r.meshes[m].primitives.size());
    }
    meshOffsets.push_back(num_aiMeshes); // add a last element so we can always do meshOffsets[n+1] - meshOffsets[n]

    std::vector<unsigned int> reverseMappingIndices;
    std::vector<unsigned int> indexBuffer;
    meshes.reserve(num_aiMeshes);
    mVertexRemappingTables.resize(num_aiMeshes);

    for (unsigned int m = 0; m < r.meshes.Size(); ++m) {
        Mesh &mesh = r.meshes[m];

        for (unsigned int p = 0; p < mesh.primitives.size(); ++p) {
            Mesh::Primitive &prim = mesh.primitives[p];

            Mesh::Primitive::Attributes &attr = prim.attributes;

            // Find out the maximum number of vertices:
            size_t numAllVertices = 0;
            if (!attr.position.empty() && attr.position[0]) {
                numAllVertices = attr.position[0]->count;
            }

            // Extract used vertices:
            bool useIndexBuffer = prim.indices;
            std::vector<unsigned int> *vertexRemappingTable = nullptr;
            
            if (useIndexBuffer) {
                size_t count = prim.indices->count;
                indexBuffer.resize(count);
                reverseMappingIndices.clear();
                vertexRemappingTable = &mVertexRemappingTables[meshes.size()];
                vertexRemappingTable->reserve(count / 3); // this is a very rough heuristic to reduce re-allocations
                Accessor::Indexer data = prim.indices->GetIndexer();
                if (!data.IsValid()) {
                    throw DeadlyImportError("GLTF: Invalid accessor without data in mesh ", getContextForErrorMessages(mesh.id, mesh.name));
                }

                // Build the vertex remapping table and the modified index buffer (used later instead of the original one)
                // In case no index buffer is used, the original vertex arrays are being used so no remapping is required in the first place.
                const unsigned int unusedIndex = ~0u;
                for (unsigned int i = 0; i < count; ++i) {
                    unsigned int index = data.GetUInt(i);
                    if (index >= numAllVertices) {
                        // Out-of-range indices will be filtered out when adding the faces and then lead to a warning. At this stage, we just keep them.
                        indexBuffer[i] = index;
                        continue; 
                    }
                    if (index >= reverseMappingIndices.size()) {
                        reverseMappingIndices.resize(index + 1, unusedIndex);
                    }
                    if (reverseMappingIndices[index] == unusedIndex) {
                        reverseMappingIndices[index] = static_cast<unsigned int>(vertexRemappingTable->size());
                        vertexRemappingTable->push_back(index);
                    }
                    indexBuffer[i] = reverseMappingIndices[index];
                }
            }

            aiMesh *aim = new aiMesh();
            meshes.push_back(std::unique_ptr<aiMesh>(aim));

            aim->mName = mesh.name.empty() ? mesh.id : mesh.name;

            if (mesh.primitives.size() > 1) {
                ai_uint32 &len = aim->mName.length;
                aim->mName.data[len] = '-';
                len += 1 + ASSIMP_itoa10(aim->mName.data + len + 1, unsigned(MAXLEN - len - 1), p);
            }

            switch (prim.mode) {
            case PrimitiveMode_POINTS:
                aim->mPrimitiveTypes |= aiPrimitiveType_POINT;
                break;

            case PrimitiveMode_LINES:
            case PrimitiveMode_LINE_LOOP:
            case PrimitiveMode_LINE_STRIP:
                aim->mPrimitiveTypes |= aiPrimitiveType_LINE;
                break;

            case PrimitiveMode_TRIANGLES:
            case PrimitiveMode_TRIANGLE_STRIP:
            case PrimitiveMode_TRIANGLE_FAN:
                aim->mPrimitiveTypes |= aiPrimitiveType_TRIANGLE;
                break;
            }

            if (!attr.position.empty() && attr.position[0]) {
                aim->mNumVertices = static_cast<unsigned int>(attr.position[0]->ExtractData(aim->mVertices, vertexRemappingTable));
            }

            if (!attr.normal.empty() && attr.normal[0]) {
                    if (attr.normal[0]->count != numAllVertices) {
                    DefaultLogger::get()->warn("Normal count in mesh \"", mesh.name, "\" does not match the vertex count, normals ignored.");
                } else {
                    attr.normal[0]->ExtractData(aim->mNormals, vertexRemappingTable);

                    // only extract tangents if normals are present
                    if (!attr.tangent.empty() && attr.tangent[0]) {
                        if (attr.tangent[0]->count != numAllVertices) {
                            DefaultLogger::get()->warn("Tangent count in mesh \"", mesh.name, "\" does not match the vertex count, tangents ignored.");
                        } else {
                            // generate bitangents from normals and tangents according to spec
                            Tangent *tangents = nullptr;

                            attr.tangent[0]->ExtractData(tangents, vertexRemappingTable);

                            aim->mTangents = new aiVector3D[aim->mNumVertices];
                            aim->mBitangents = new aiVector3D[aim->mNumVertices];

                            for (unsigned int i = 0; i < aim->mNumVertices; ++i) {
                                aim->mTangents[i] = tangents[i].xyz;
                                aim->mBitangents[i] = (aim->mNormals[i] ^ tangents[i].xyz) * tangents[i].w;
                            }

                            delete[] tangents;
                        }
                    }
                }
            }

            for (size_t c = 0; c < attr.color.size() && c < AI_MAX_NUMBER_OF_COLOR_SETS; ++c) {
                if (attr.color[c]->count != numAllVertices) {
                    DefaultLogger::get()->warn("Color stream size in mesh \"", mesh.name,
                            "\" does not match the vertex count");
                    continue;
                }

                auto componentType = attr.color[c]->componentType;
                if (componentType == glTF2::ComponentType_FLOAT) {
                    attr.color[c]->ExtractData(aim->mColors[c], vertexRemappingTable);
                } else {
                    if (componentType == glTF2::ComponentType_UNSIGNED_BYTE) {
                        aim->mColors[c] = GetVertexColorsForType<unsigned char>(attr.color[c], vertexRemappingTable);
                    } else if (componentType == glTF2::ComponentType_UNSIGNED_SHORT) {
                        aim->mColors[c] = GetVertexColorsForType<unsigned short>(attr.color[c], vertexRemappingTable);
                    }
                }
            }
            for (size_t tc = 0; tc < attr.texcoord.size() && tc < AI_MAX_NUMBER_OF_TEXTURECOORDS; ++tc) {
                if (!attr.texcoord[tc]) {
                    DefaultLogger::get()->warn("Texture coordinate accessor not found or non-contiguous texture coordinate sets.");
                    continue;
                }

                if (attr.texcoord[tc]->count != numAllVertices) {
                    DefaultLogger::get()->warn("Texcoord stream size in mesh \"", mesh.name,
                            "\" does not match the vertex count");
                    continue;
                }

                attr.texcoord[tc]->ExtractData(aim->mTextureCoords[tc], vertexRemappingTable);
                aim->mNumUVComponents[tc] = attr.texcoord[tc]->GetNumComponents();

                aiVector3D *values = aim->mTextureCoords[tc];
                for (unsigned int i = 0; i < aim->mNumVertices; ++i) {
                    values[i].y = 1 - values[i].y; // Flip Y coords
                }
            }

            std::vector<Mesh::Primitive::Target> &targets = prim.targets;
            if (!targets.empty()) {
                aim->mNumAnimMeshes = (unsigned int)targets.size();
                aim->mAnimMeshes = new aiAnimMesh *[aim->mNumAnimMeshes];
                std::fill(aim->mAnimMeshes, aim->mAnimMeshes + aim->mNumAnimMeshes, nullptr);
                for (size_t i = 0; i < targets.size(); i++) {
                    bool needPositions = targets[i].position.size() > 0;
                    bool needNormals = (targets[i].normal.size() > 0) && aim->HasNormals();
                    bool needTangents = (targets[i].tangent.size() > 0) && aim->HasTangentsAndBitangents();
                    // GLTF morph does not support colors and texCoords
                    aim->mAnimMeshes[i] = aiCreateAnimMesh(aim,
                            needPositions, needNormals, needTangents, false, false);
                    aiAnimMesh &aiAnimMesh = *(aim->mAnimMeshes[i]);
                    Mesh::Primitive::Target &target = targets[i];

                    if (needPositions) {
                        if (target.position[0]->count != numAllVertices) {
                            ASSIMP_LOG_WARN("Positions of target ", i, " in mesh \"", mesh.name, "\" does not match the vertex count");
                        } else {
                            aiVector3D *positionDiff = nullptr;
                            target.position[0]->ExtractData(positionDiff, vertexRemappingTable);
                            for (unsigned int vertexId = 0; vertexId < aim->mNumVertices; vertexId++) {
                                aiAnimMesh.mVertices[vertexId] += positionDiff[vertexId];
                            }
                            delete[] positionDiff;
                        }
                    }
                    if (needNormals) {
                        if (target.normal[0]->count != numAllVertices) {
                            ASSIMP_LOG_WARN("Normals of target ", i, " in mesh \"", mesh.name, "\" does not match the vertex count");
                        } else {
                            aiVector3D *normalDiff = nullptr;
                            target.normal[0]->ExtractData(normalDiff, vertexRemappingTable);
                            for (unsigned int vertexId = 0; vertexId < aim->mNumVertices; vertexId++) {
                                aiAnimMesh.mNormals[vertexId] += normalDiff[vertexId];
                            }
                            delete[] normalDiff;
                        }
                    }
                    if (needTangents) {
                        if (!aiAnimMesh.HasNormals()) {
                            // prevent nullptr access to aiAnimMesh.mNormals below when no normals are available
                            ASSIMP_LOG_WARN("Bitangents of target ", i, " in mesh \"", mesh.name, "\" can't be computed, because mesh has no normals.");
                        } else if (target.tangent[0]->count != numAllVertices) {
                            ASSIMP_LOG_WARN("Tangents of target ", i, " in mesh \"", mesh.name, "\" does not match the vertex count");
                        } else {
                            Tangent *tangent = nullptr;
                            attr.tangent[0]->ExtractData(tangent, vertexRemappingTable);

                            aiVector3D *tangentDiff = nullptr;
                            target.tangent[0]->ExtractData(tangentDiff, vertexRemappingTable);

                            for (unsigned int vertexId = 0; vertexId < aim->mNumVertices; ++vertexId) {
                                tangent[vertexId].xyz += tangentDiff[vertexId];
                                aiAnimMesh.mTangents[vertexId] = tangent[vertexId].xyz;
                                aiAnimMesh.mBitangents[vertexId] = (aiAnimMesh.mNormals[vertexId] ^ tangent[vertexId].xyz) * tangent[vertexId].w;
                            }
                            delete[] tangent;
                            delete[] tangentDiff;
                        }
                    }
                    if (mesh.weights.size() > i) {
                        aiAnimMesh.mWeight = mesh.weights[i];
                    }
                    if (mesh.targetNames.size() > i) {
                        aiAnimMesh.mName = mesh.targetNames[i];
                    }
                }
            }

            aiFace *faces = nullptr;
            aiFace *facePtr = nullptr;
            size_t nFaces = 0;

            if (useIndexBuffer) {
                size_t count = indexBuffer.size();

                switch (prim.mode) {
                case PrimitiveMode_POINTS: {
                    nFaces = count;
                    facePtr = faces = new aiFace[nFaces];
                    for (unsigned int i = 0; i < count; ++i) {
                        SetFaceAndAdvance1(facePtr, aim->mNumVertices, indexBuffer[i]);
                    }
                    break;
                }

                case PrimitiveMode_LINES: {
                    nFaces = count / 2;
                    if (nFaces * 2 != count) {
                        ASSIMP_LOG_WARN("The number of vertices was not compatible with the LINES mode. Some vertices were dropped.");
                        count = nFaces * 2;
                    }
                    facePtr = faces = new aiFace[nFaces];
                    for (unsigned int i = 0; i < count; i += 2) {
                        SetFaceAndAdvance2(facePtr, aim->mNumVertices, indexBuffer[i], indexBuffer[i + 1]);
                    }
                    break;
                }

                case PrimitiveMode_LINE_LOOP:
                case PrimitiveMode_LINE_STRIP: {
                    nFaces = count - ((prim.mode == PrimitiveMode_LINE_STRIP) ? 1 : 0);
                    facePtr = faces = new aiFace[nFaces];
                    SetFaceAndAdvance2(facePtr, aim->mNumVertices, indexBuffer[0], indexBuffer[1]);
                    for (unsigned int i = 2; i < count; ++i) {
                        SetFaceAndAdvance2(facePtr, aim->mNumVertices, indexBuffer[i - 1], indexBuffer[i]);
                    }
                    if (prim.mode == PrimitiveMode_LINE_LOOP) { // close the loop
                        SetFaceAndAdvance2(facePtr, aim->mNumVertices, indexBuffer[static_cast<int>(count) - 1], faces[0].mIndices[0]);
                    }
                    break;
                }

                case PrimitiveMode_TRIANGLES: {
                    nFaces = count / 3;
                    if (nFaces * 3 != count) {
                        ASSIMP_LOG_WARN("The number of vertices was not compatible with the TRIANGLES mode. Some vertices were dropped.");
                        count = nFaces * 3;
                    }
                    facePtr = faces = new aiFace[nFaces];
                    for (unsigned int i = 0; i < count; i += 3) {
                        SetFaceAndAdvance3(facePtr, aim->mNumVertices, indexBuffer[i], indexBuffer[i + 1], indexBuffer[i + 2]);
                    }
                    break;
                }
                case PrimitiveMode_TRIANGLE_STRIP: {
                    nFaces = count - 2;
                    facePtr = faces = new aiFace[nFaces];
                    for (unsigned int i = 0; i < nFaces; ++i) {
                        // The ordering is to ensure that the triangles are all drawn with the same orientation
                        if ((i + 1) % 2 == 0) {
                            // For even n, vertices n + 1, n, and n + 2 define triangle n
                            SetFaceAndAdvance3(facePtr, aim->mNumVertices, indexBuffer[i + 1], indexBuffer[i], indexBuffer[i + 2]);
                        } else {
                            // For odd n, vertices n, n+1, and n+2 define triangle n
                            SetFaceAndAdvance3(facePtr, aim->mNumVertices, indexBuffer[i], indexBuffer[i + 1], indexBuffer[i + 2]);
                        }
                    }
                    break;
                }
                case PrimitiveMode_TRIANGLE_FAN:
                    nFaces = count - 2;
                    facePtr = faces = new aiFace[nFaces];
                    SetFaceAndAdvance3(facePtr, aim->mNumVertices, indexBuffer[0], indexBuffer[1], indexBuffer[2]);
                    for (unsigned int i = 1; i < nFaces; ++i) {
                        SetFaceAndAdvance3(facePtr, aim->mNumVertices, indexBuffer[0], indexBuffer[i + 1], indexBuffer[i + 2]);
                    }
                    break;
                }
            } else { // no indices provided so directly generate from counts

                // use the already determined count as it includes checks
                unsigned int count = aim->mNumVertices;

                switch (prim.mode) {
                case PrimitiveMode_POINTS: {
                    nFaces = count;
                    facePtr = faces = new aiFace[nFaces];
                    for (unsigned int i = 0; i < count; ++i) {
                        SetFaceAndAdvance1(facePtr, aim->mNumVertices, i);
                    }
                    break;
                }

                case PrimitiveMode_LINES: {
                    nFaces = count / 2;
                    if (nFaces * 2 != count) {
                        ASSIMP_LOG_WARN("The number of vertices was not compatible with the LINES mode. Some vertices were dropped.");
                        count = (unsigned int)nFaces * 2;
                    }
                    facePtr = faces = new aiFace[nFaces];
                    for (unsigned int i = 0; i < count; i += 2) {
                        SetFaceAndAdvance2(facePtr, aim->mNumVertices, i, i + 1);
                    }
                    break;
                }

                case PrimitiveMode_LINE_LOOP:
                case PrimitiveMode_LINE_STRIP: {
                    nFaces = count - ((prim.mode == PrimitiveMode_LINE_STRIP) ? 1 : 0);
                    facePtr = faces = new aiFace[nFaces];
                    SetFaceAndAdvance2(facePtr, aim->mNumVertices, 0, 1);
                    for (unsigned int i = 2; i < count; ++i) {
                        SetFaceAndAdvance2(facePtr, aim->mNumVertices, i - 1, i);
                    }
                    if (prim.mode == PrimitiveMode_LINE_LOOP) { // close the loop
                        SetFaceAndAdvance2(facePtr, aim->mNumVertices, count - 1, 0);
                    }
                    break;
                }

                case PrimitiveMode_TRIANGLES: {
                    nFaces = count / 3;
                    if (nFaces * 3 != count) {
                        ASSIMP_LOG_WARN("The number of vertices was not compatible with the TRIANGLES mode. Some vertices were dropped.");
                        count = (unsigned int)nFaces * 3;
                    }
                    facePtr = faces = new aiFace[nFaces];
                    for (unsigned int i = 0; i < count; i += 3) {
                        SetFaceAndAdvance3(facePtr, aim->mNumVertices, i, i + 1, i + 2);
                    }
                    break;
                }
                case PrimitiveMode_TRIANGLE_STRIP: {
                    nFaces = count - 2;
                    facePtr = faces = new aiFace[nFaces];
                    for (unsigned int i = 0; i < nFaces; ++i) {
                        // The ordering is to ensure that the triangles are all drawn with the same orientation
                        if ((i + 1) % 2 == 0) {
                            // For even n, vertices n + 1, n, and n + 2 define triangle n
                            SetFaceAndAdvance3(facePtr, aim->mNumVertices, i + 1, i, i + 2);
                        } else {
                            // For odd n, vertices n, n+1, and n+2 define triangle n
                            SetFaceAndAdvance3(facePtr, aim->mNumVertices, i, i + 1, i + 2);
                        }
                    }
                    break;
                }
                case PrimitiveMode_TRIANGLE_FAN:
                    nFaces = count - 2;
                    facePtr = faces = new aiFace[nFaces];
                    SetFaceAndAdvance3(facePtr, aim->mNumVertices, 0, 1, 2);
                    for (unsigned int i = 1; i < nFaces; ++i) {
                        SetFaceAndAdvance3(facePtr, aim->mNumVertices, 0, i + 1, i + 2);
                    }
                    break;
                }
            }

            if (faces) {
                aim->mFaces = faces;
                const unsigned int actualNumFaces = static_cast<unsigned int>(facePtr - faces);
                if (actualNumFaces < nFaces) {
                    ASSIMP_LOG_WARN("Some faces had out-of-range indices. Those faces were dropped.");
                }
                if (actualNumFaces == 0) {
                    throw DeadlyImportError("Mesh \"", aim->mName.C_Str(), "\" has no faces");
                }
                aim->mNumFaces = actualNumFaces;
                ai_assert(CheckValidFacesIndices(faces, actualNumFaces, aim->mNumVertices));
            }

            if (prim.material) {
                aim->mMaterialIndex = prim.material.GetIndex();
            } else {
                aim->mMaterialIndex = mScene->mNumMaterials - 1;
            }
        }
    }

    CopyVector(meshes, mScene->mMeshes, mScene->mNumMeshes);
}

void glTF2Importer::ImportCameras(glTF2::Asset &r) {
    if (!r.cameras.Size()) {
        return;
    }

    const unsigned int numCameras = r.cameras.Size();
    ASSIMP_LOG_DEBUG("Importing ", numCameras, " cameras");
    mScene->mNumCameras = numCameras;
    mScene->mCameras = new aiCamera *[numCameras];
    std::fill(mScene->mCameras, mScene->mCameras + numCameras, nullptr);

    for (size_t i = 0; i < numCameras; ++i) {
        Camera &cam = r.cameras[i];

        aiCamera *aicam = mScene->mCameras[i] = new aiCamera();

        // cameras point in -Z by default, rest is specified in node transform
        aicam->mLookAt = aiVector3D(0.f, 0.f, -1.f);

        if (cam.type == Camera::Perspective) {
            aicam->mAspect = cam.cameraProperties.perspective.aspectRatio;
            aicam->mHorizontalFOV = 2.0f * std::atan(std::tan(cam.cameraProperties.perspective.yfov * 0.5f) * ((aicam->mAspect == 0.f) ? 1.f : aicam->mAspect));
            aicam->mClipPlaneFar = cam.cameraProperties.perspective.zfar;
            aicam->mClipPlaneNear = cam.cameraProperties.perspective.znear;
        } else {
            aicam->mClipPlaneFar = cam.cameraProperties.ortographic.zfar;
            aicam->mClipPlaneNear = cam.cameraProperties.ortographic.znear;
            aicam->mHorizontalFOV = 0.0;
            aicam->mOrthographicWidth = cam.cameraProperties.ortographic.xmag;
            aicam->mAspect = 1.0f;
            if (0.f != cam.cameraProperties.ortographic.ymag) {
                aicam->mAspect = cam.cameraProperties.ortographic.xmag / cam.cameraProperties.ortographic.ymag;
            }
        }
    }
}

void glTF2Importer::ImportLights(glTF2::Asset &r) {
    if (!r.lights.Size()) {
        return;
    }

    const unsigned int numLights = r.lights.Size();
    ASSIMP_LOG_DEBUG("Importing ", numLights, " lights");
    mScene->mNumLights = numLights;
    mScene->mLights = new aiLight *[numLights];
    std::fill(mScene->mLights, mScene->mLights + numLights, nullptr);

    for (size_t i = 0; i < numLights; ++i) {
        Light &light = r.lights[i];

        aiLight *ail = mScene->mLights[i] = new aiLight();

        switch (light.type) {
        case Light::Directional:
            ail->mType = aiLightSource_DIRECTIONAL;
            break;
        case Light::Point:
            ail->mType = aiLightSource_POINT;
            break;
        case Light::Spot:
            ail->mType = aiLightSource_SPOT;
            break;
        }

        if (ail->mType != aiLightSource_POINT) {
            ail->mDirection = aiVector3D(0.0f, 0.0f, -1.0f);
            ail->mUp = aiVector3D(0.0f, 1.0f, 0.0f);
        }

        vec3 colorWithIntensity = { light.color[0] * light.intensity, light.color[1] * light.intensity, light.color[2] * light.intensity };
        CopyValue(colorWithIntensity, ail->mColorAmbient);
        CopyValue(colorWithIntensity, ail->mColorDiffuse);
        CopyValue(colorWithIntensity, ail->mColorSpecular);

        if (ail->mType == aiLightSource_DIRECTIONAL) {
            ail->mAttenuationConstant = 1.0;
            ail->mAttenuationLinear = 0.0;
            ail->mAttenuationQuadratic = 0.0;
        } else {
            // in PBR attenuation is calculated using inverse square law which can be expressed
            // using assimps equation: 1/(att0 + att1 * d + att2 * d*d) with the following parameters
            // this is correct equation for the case when range (see
            // https://github.com/KhronosGroup/glTF/tree/master/extensions/2.0/Khronos/KHR_lights_punctual)
            // is not present. When range is not present it is assumed that it is infinite and so numerator is 1.
            // When range is present then numerator might be any value in range [0,1] and then assimps equation
            // will not suffice. In this case range is added into metadata in ImportNode function
            // and its up to implementation to read it when it wants to
            ail->mAttenuationConstant = 0.0;
            ail->mAttenuationLinear = 0.0;
            ail->mAttenuationQuadratic = 1.0;
        }

        if (ail->mType == aiLightSource_SPOT) {
            ail->mAngleInnerCone = light.innerConeAngle;
            ail->mAngleOuterCone = light.outerConeAngle;
        }
    }
}

static void GetNodeTransform(aiMatrix4x4 &matrix, const glTF2::Node &node) {
    if (node.matrix.isPresent) {
        CopyValue(node.matrix.value, matrix);
        return;
    }

    if (node.translation.isPresent) {
        aiVector3D trans;
        CopyValue(node.translation.value, trans);
        aiMatrix4x4 t;
        aiMatrix4x4::Translation(trans, t);
        matrix = matrix * t;
    }

    if (node.rotation.isPresent) {
        aiQuaternion rot;
        CopyValue(node.rotation.value, rot);
        matrix = matrix * aiMatrix4x4(rot.GetMatrix());
    }

    if (node.scale.isPresent) {
        aiVector3D scal(1.f);
        CopyValue(node.scale.value, scal);
        aiMatrix4x4 s;
        aiMatrix4x4::Scaling(scal, s);
        matrix = matrix * s;
    }
}

static void BuildVertexWeightMapping(Mesh::Primitive &primitive, std::vector<std::vector<aiVertexWeight>> &map, std::vector<unsigned int>* vertexRemappingTablePtr) {

    Mesh::Primitive::Attributes &attr = primitive.attributes;
    if (attr.weight.empty() || attr.joint.empty()) {
        return;
    }
    if (attr.weight[0]->count != attr.joint[0]->count) {
        return;
    }

    size_t num_vertices = 0;

    struct Weights {
        float values[4];
    };
    Weights **weights = new Weights*[attr.weight.size()];
    for (size_t w = 0; w < attr.weight.size(); ++w) {
        num_vertices = attr.weight[w]->ExtractData(weights[w], vertexRemappingTablePtr);
    }

    struct Indices8 {
        uint8_t values[4];
    };
    struct Indices16 {
        uint16_t values[4];
    };
    Indices8 **indices8 = nullptr;
    Indices16 **indices16 = nullptr;
    if (attr.joint[0]->GetElementSize() == 4) {
        indices8 = new Indices8*[attr.joint.size()];
        for (size_t j = 0; j < attr.joint.size(); ++j) {
            attr.joint[j]->ExtractData(indices8[j], vertexRemappingTablePtr);
        }
    } else {
        indices16 = new Indices16 *[attr.joint.size()];
        for (size_t j = 0; j < attr.joint.size(); ++j) {
            attr.joint[j]->ExtractData(indices16[j], vertexRemappingTablePtr);
        }
    }
    //
    if (nullptr == indices8 && nullptr == indices16) {
        // Something went completely wrong!
        ai_assert(false);
        return;
    }

    for (size_t w = 0; w < attr.weight.size(); ++w) {
        for (size_t i = 0; i < num_vertices; ++i) {
            for (int j = 0; j < 4; ++j) {
                const unsigned int bone = (indices8 != nullptr) ? indices8[w][i].values[j] : indices16[w][i].values[j];
                const float weight = weights[w][i].values[j];
                if (weight > 0 && bone < map.size()) {
                    map[bone].reserve(8);
                    map[bone].emplace_back(static_cast<unsigned int>(i), weight);
                }
            }
        }
    }

    for (size_t w = 0; w < attr.weight.size(); ++w) {
        delete[] weights[w];
        if(indices8)
            delete[] indices8[w];
        if (indices16)
            delete[] indices16[w];
    }
    delete[] weights;
    delete[] indices8;
    delete[] indices16;
}

static std::string GetNodeName(const Node &node) {
    return node.name.empty() ? node.id : node.name;
}

void ParseExtensions(aiMetadata *metadata, const CustomExtension &extension) {
    if (extension.mStringValue.isPresent) {
        metadata->Add(extension.name, aiString(extension.mStringValue.value));
    } else if (extension.mDoubleValue.isPresent) {
        metadata->Add(extension.name, extension.mDoubleValue.value);
    } else if (extension.mUint64Value.isPresent) {
        metadata->Add(extension.name, extension.mUint64Value.value);
    } else if (extension.mInt64Value.isPresent) {
        metadata->Add(extension.name, static_cast<int32_t>(extension.mInt64Value.value));
    } else if (extension.mBoolValue.isPresent) {
        metadata->Add(extension.name, extension.mBoolValue.value);
    } else if (extension.mValues.isPresent) {
        aiMetadata val;
        for (auto const &subExtension : extension.mValues.value) {
            ParseExtensions(&val, subExtension);
        }
        metadata->Add(extension.name, val);
    }
}

void ParseExtras(aiMetadata* metadata, const Extras& extras) {
    for (auto const &value : extras.mValues) {
        ParseExtensions(metadata, value);
    }
}

aiNode *glTF2Importer::ImportNode(glTF2::Asset &r, glTF2::Ref<glTF2::Node> &ptr) {
    Node &node = *ptr;

    aiNode *ainode = new aiNode(GetNodeName(node));

    try {
        if (!node.children.empty()) {
            ainode->mNumChildren = unsigned(node.children.size());
            ainode->mChildren = new aiNode *[ainode->mNumChildren];
            std::fill(ainode->mChildren, ainode->mChildren + ainode->mNumChildren, nullptr);

            for (unsigned int i = 0; i < ainode->mNumChildren; ++i) {
                aiNode *child = ImportNode(r, node.children[i]);
                child->mParent = ainode;
                ainode->mChildren[i] = child;
            }
        }

        if (node.customExtensions || node.extras.HasExtras()) {
            ainode->mMetaData = new aiMetadata;
            if (node.customExtensions) {
                ParseExtensions(ainode->mMetaData, node.customExtensions);
            }
            if (node.extras.HasExtras()) {
                ParseExtras(ainode->mMetaData, node.extras);
            }
        }

        GetNodeTransform(ainode->mTransformation, node);

        if (!node.meshes.empty()) {
            // GLTF files contain at most 1 mesh per node.
            if (node.meshes.size() > 1) {
                throw DeadlyImportError("GLTF: Invalid input, found ", node.meshes.size(),
                        " meshes in ", getContextForErrorMessages(node.id, node.name),
                        ", but only 1 mesh per node allowed.");
            }
            int mesh_idx = node.meshes[0].GetIndex();
            int count = meshOffsets[mesh_idx + 1] - meshOffsets[mesh_idx];

            ainode->mNumMeshes = count;
            ainode->mMeshes = new unsigned int[count];

            if (node.skin) {
                for (int primitiveNo = 0; primitiveNo < count; ++primitiveNo) {
                    unsigned int aiMeshIdx = meshOffsets[mesh_idx] + primitiveNo;
                    aiMesh *mesh = mScene->mMeshes[aiMeshIdx];
                    unsigned int numBones = static_cast<unsigned int>(node.skin->jointNames.size());
                    std::vector<unsigned int> *vertexRemappingTablePtr = mVertexRemappingTables[aiMeshIdx].empty() ? nullptr : &mVertexRemappingTables[aiMeshIdx];

                    std::vector<std::vector<aiVertexWeight>> weighting(numBones);
                    BuildVertexWeightMapping(node.meshes[0]->primitives[primitiveNo], weighting, vertexRemappingTablePtr);

                    mesh->mNumBones = static_cast<unsigned int>(numBones);
                    mesh->mBones = new aiBone *[mesh->mNumBones];
                    std::fill(mesh->mBones, mesh->mBones + mesh->mNumBones, nullptr);

                    // GLTF and Assimp choose to store bone weights differently.
                    // GLTF has each vertex specify which bones influence the vertex.
                    // Assimp has each bone specify which vertices it has influence over.
                    // To convert this data, we first read over the vertex data and pull
                    // out the bone-to-vertex mapping.  Then, when creating the aiBones,
                    // we copy the bone-to-vertex mapping into the bone.  This is unfortunate
                    // both because it's somewhat slow and because, for many applications,
                    // we then need to reconvert the data back into the vertex-to-bone
                    // mapping which makes things doubly-slow.

                    mat4 *pbindMatrices = nullptr;
                    node.skin->inverseBindMatrices->ExtractData(pbindMatrices, nullptr);

                    for (uint32_t i = 0; i < numBones; ++i) {
                        const std::vector<aiVertexWeight> &weights = weighting[i];
                        aiBone *bone = new aiBone();

                        Ref<Node> joint = node.skin->jointNames[i];
                        if (!joint->name.empty()) {
                            bone->mName = joint->name;
                        } else {
                            // Assimp expects each bone to have a unique name.
                            static const std::string kDefaultName = "bone_";
                            char postfix[10] = { 0 };
                            ASSIMP_itoa10(postfix, i);
                            bone->mName = (kDefaultName + postfix);
                        }
                        GetNodeTransform(bone->mOffsetMatrix, *joint);
                        CopyValue(pbindMatrices[i], bone->mOffsetMatrix);
                        bone->mNumWeights = static_cast<uint32_t>(weights.size());

                        if (bone->mNumWeights > 0) {
                            bone->mWeights = new aiVertexWeight[bone->mNumWeights];
                            memcpy(bone->mWeights, weights.data(), bone->mNumWeights * sizeof(aiVertexWeight));
                        } else {
                            // Assimp expects all bones to have at least 1 weight.
                            bone->mWeights = new aiVertexWeight[1];
                            bone->mNumWeights = 1;
                            bone->mWeights->mVertexId = 0;
                            bone->mWeights->mWeight = 0.f;
                        }
                        mesh->mBones[i] = bone;
                    }

                    if (pbindMatrices) {
                        delete[] pbindMatrices;
                    }
                }
            }

            int k = 0;
            for (unsigned int j = meshOffsets[mesh_idx]; j < meshOffsets[mesh_idx + 1]; ++j, ++k) {
                ainode->mMeshes[k] = j;
            }
        }

        if (node.camera) {
            mScene->mCameras[node.camera.GetIndex()]->mName = ainode->mName;
        }

        if (node.light) {
            mScene->mLights[node.light.GetIndex()]->mName = ainode->mName;

            // range is optional - see https://github.com/KhronosGroup/glTF/tree/master/extensions/2.0/Khronos/KHR_lights_punctual
            // it is added to meta data of parent node, because there is no other place to put it
            if (node.light->range.isPresent) {
                if (!ainode->mMetaData) {
                    ainode->mMetaData = aiMetadata::Alloc(1);
                    ainode->mMetaData->Set(0, "PBR_LightRange", node.light->range.value);
                } else {
                    ainode->mMetaData->Add("PBR_LightRange", node.light->range.value);
                }
            }
        }

        return ainode;
    } catch (...) {
        delete ainode;
        throw;
    }
}

void glTF2Importer::ImportNodes(glTF2::Asset &r) {
    if (!r.scene) {
        throw DeadlyImportError("GLTF: No scene");
    }
    ASSIMP_LOG_DEBUG("Importing nodes");

    std::vector<Ref<Node>> rootNodes = r.scene->nodes;

    // The root nodes
    unsigned int numRootNodes = unsigned(rootNodes.size());
    if (numRootNodes == 1) { // a single root node: use it
        mScene->mRootNode = ImportNode(r, rootNodes[0]);
    } else if (numRootNodes > 1) { // more than one root node: create a fake root
        aiNode *root = mScene->mRootNode = new aiNode("ROOT");

        root->mChildren = new aiNode *[numRootNodes];
        std::fill(root->mChildren, root->mChildren + numRootNodes, nullptr);

        for (unsigned int i = 0; i < numRootNodes; ++i) {
            aiNode *node = ImportNode(r, rootNodes[i]);
            node->mParent = root;
            root->mChildren[root->mNumChildren++] = node;
        }
    } else {
        mScene->mRootNode = new aiNode("ROOT");
    }
}

struct AnimationSamplers {
    AnimationSamplers() :
            translation(nullptr),
            rotation(nullptr),
            scale(nullptr),
            weight(nullptr) {
        // empty
    }

    Animation::Sampler *translation;
    Animation::Sampler *rotation;
    Animation::Sampler *scale;
    Animation::Sampler *weight;
};

aiNodeAnim *CreateNodeAnim(glTF2::Asset &, Node &node, AnimationSamplers &samplers) {
    aiNodeAnim *anim = new aiNodeAnim();

    try {
        anim->mNodeName = GetNodeName(node);

        static const float kMillisecondsFromSeconds = 1000.f;

        if (samplers.translation && samplers.translation->input && samplers.translation->output) {
            float *times = nullptr;
            samplers.translation->input->ExtractData(times);
            aiVector3D *values = nullptr;
            samplers.translation->output->ExtractData(values);
            anim->mNumPositionKeys = static_cast<uint32_t>(samplers.translation->input->count);
            anim->mPositionKeys = new aiVectorKey[anim->mNumPositionKeys];
            unsigned int ii = (samplers.translation->interpolation == Interpolation_CUBICSPLINE) ? 1 : 0;
            for (unsigned int i = 0; i < anim->mNumPositionKeys; ++i) {
                anim->mPositionKeys[i].mTime = times[i] * kMillisecondsFromSeconds;
                anim->mPositionKeys[i].mValue = values[ii];
                ii += (samplers.translation->interpolation == Interpolation_CUBICSPLINE) ? 3 : 1;
            }
            delete[] times;
            delete[] values;
        } else if (node.translation.isPresent) {
            anim->mNumPositionKeys = 1;
            anim->mPositionKeys = new aiVectorKey[anim->mNumPositionKeys];
            anim->mPositionKeys->mTime = 0.f;
            anim->mPositionKeys->mValue.x = node.translation.value[0];
            anim->mPositionKeys->mValue.y = node.translation.value[1];
            anim->mPositionKeys->mValue.z = node.translation.value[2];
        }

        if (samplers.rotation && samplers.rotation->input && samplers.rotation->output) {
            float *times = nullptr;
            samplers.rotation->input->ExtractData(times);
            aiQuaternion *values = nullptr;
            samplers.rotation->output->ExtractData(values);
            anim->mNumRotationKeys = static_cast<uint32_t>(samplers.rotation->input->count);
            anim->mRotationKeys = new aiQuatKey[anim->mNumRotationKeys];
            unsigned int ii = (samplers.rotation->interpolation == Interpolation_CUBICSPLINE) ? 1 : 0;
            for (unsigned int i = 0; i < anim->mNumRotationKeys; ++i) {
                anim->mRotationKeys[i].mTime = times[i] * kMillisecondsFromSeconds;
                anim->mRotationKeys[i].mValue.x = values[ii].w;
                anim->mRotationKeys[i].mValue.y = values[ii].x;
                anim->mRotationKeys[i].mValue.z = values[ii].y;
                anim->mRotationKeys[i].mValue.w = values[ii].z;
                ii += (samplers.rotation->interpolation == Interpolation_CUBICSPLINE) ? 3 : 1;
            }
            delete[] times;
            delete[] values;
        } else if (node.rotation.isPresent) {
            anim->mNumRotationKeys = 1;
            anim->mRotationKeys = new aiQuatKey[anim->mNumRotationKeys];
            anim->mRotationKeys->mTime = 0.f;
            anim->mRotationKeys->mValue.x = node.rotation.value[0];
            anim->mRotationKeys->mValue.y = node.rotation.value[1];
            anim->mRotationKeys->mValue.z = node.rotation.value[2];
            anim->mRotationKeys->mValue.w = node.rotation.value[3];
        }

        if (samplers.scale && samplers.scale->input && samplers.scale->output) {
            float *times = nullptr;
            samplers.scale->input->ExtractData(times);
            aiVector3D *values = nullptr;
            samplers.scale->output->ExtractData(values);
            anim->mNumScalingKeys = static_cast<uint32_t>(samplers.scale->input->count);
            anim->mScalingKeys = new aiVectorKey[anim->mNumScalingKeys];
            unsigned int ii = (samplers.scale->interpolation == Interpolation_CUBICSPLINE) ? 1 : 0;
            for (unsigned int i = 0; i < anim->mNumScalingKeys; ++i) {
                anim->mScalingKeys[i].mTime = times[i] * kMillisecondsFromSeconds;
                anim->mScalingKeys[i].mValue = values[ii];
                ii += (samplers.scale->interpolation == Interpolation_CUBICSPLINE) ? 3 : 1;
            }
            delete[] times;
            delete[] values;
        } else if (node.scale.isPresent) {
            anim->mNumScalingKeys = 1;
            anim->mScalingKeys = new aiVectorKey[anim->mNumScalingKeys];
            anim->mScalingKeys->mTime = 0.f;
            anim->mScalingKeys->mValue.x = node.scale.value[0];
            anim->mScalingKeys->mValue.y = node.scale.value[1];
            anim->mScalingKeys->mValue.z = node.scale.value[2];
        }

        return anim;
    } catch (...) {
        delete anim;
        throw;
    }
}

aiMeshMorphAnim *CreateMeshMorphAnim(glTF2::Asset &, Node &node, AnimationSamplers &samplers) {
    auto *anim = new aiMeshMorphAnim();

    try {
        anim->mName = GetNodeName(node);

        static const float kMillisecondsFromSeconds = 1000.f;

        if (samplers.weight && samplers.weight->input && samplers.weight->output) {
            float *times = nullptr;
            samplers.weight->input->ExtractData(times);
            float *values = nullptr;
            samplers.weight->output->ExtractData(values);
            anim->mNumKeys = static_cast<uint32_t>(samplers.weight->input->count);

            // for Interpolation_CUBICSPLINE can have more outputs
            const unsigned int weightStride = (unsigned int)samplers.weight->output->count / anim->mNumKeys;
            const unsigned int numMorphs = (samplers.weight->interpolation == Interpolation_CUBICSPLINE) ? weightStride - 2 : weightStride;

            anim->mKeys = new aiMeshMorphKey[anim->mNumKeys];
            unsigned int ii = (samplers.weight->interpolation == Interpolation_CUBICSPLINE) ? 1 : 0;
            for (unsigned int i = 0u; i < anim->mNumKeys; ++i) {
                unsigned int k = weightStride * i + ii;
                anim->mKeys[i].mTime = times[i] * kMillisecondsFromSeconds;
                anim->mKeys[i].mNumValuesAndWeights = numMorphs;
                anim->mKeys[i].mValues = new unsigned int[numMorphs];
                anim->mKeys[i].mWeights = new double[numMorphs];

                for (unsigned int j = 0u; j < numMorphs; ++j, ++k) {
                    anim->mKeys[i].mValues[j] = j;
                    anim->mKeys[i].mWeights[j] = (0.f > values[k]) ? 0.f : values[k];
                }
            }

            delete[] times;
            delete[] values;
        }

        return anim;
    } catch (...) {
        delete anim;
        throw;
    }
}

std::unordered_map<unsigned int, AnimationSamplers> GatherSamplers(Animation &anim) {
    std::unordered_map<unsigned int, AnimationSamplers> samplers;
    for (unsigned int c = 0; c < anim.channels.size(); ++c) {
        Animation::Channel &channel = anim.channels[c];
        if (channel.sampler < 0 || channel.sampler >= static_cast<int>(anim.samplers.size())) {
            continue;
        }

        auto &animsampler = anim.samplers[channel.sampler];

        if (!animsampler.input) {
            ASSIMP_LOG_WARN("Animation ", anim.name, ": Missing sampler input. Skipping.");
            continue;
        }

        if (!animsampler.output) {
            ASSIMP_LOG_WARN("Animation ", anim.name, ": Missing sampler output. Skipping.");
            continue;
        }

        if (animsampler.input->count > animsampler.output->count) {
            ASSIMP_LOG_WARN("Animation ", anim.name, ": Number of keyframes in sampler input ", animsampler.input->count, " exceeds number of keyframes in sampler output ", animsampler.output->count);
            continue;
        }

        const unsigned int node_index = channel.target.node.GetIndex();

        AnimationSamplers &sampler = samplers[node_index];
        if (channel.target.path == AnimationPath_TRANSLATION) {
            sampler.translation = &anim.samplers[channel.sampler];
        } else if (channel.target.path == AnimationPath_ROTATION) {
            sampler.rotation = &anim.samplers[channel.sampler];
        } else if (channel.target.path == AnimationPath_SCALE) {
            sampler.scale = &anim.samplers[channel.sampler];
        } else if (channel.target.path == AnimationPath_WEIGHTS) {
            sampler.weight = &anim.samplers[channel.sampler];
        }
    }

    return samplers;
}

void glTF2Importer::ImportAnimations(glTF2::Asset &r) {
    if (!r.scene) return;

    const unsigned numAnimations = r.animations.Size();
    ASSIMP_LOG_DEBUG("Importing ", numAnimations, " animations");
    mScene->mNumAnimations = numAnimations;
    if (mScene->mNumAnimations == 0) {
        return;
    }

    mScene->mAnimations = new aiAnimation *[numAnimations];
    std::fill(mScene->mAnimations, mScene->mAnimations + numAnimations, nullptr);

    for (unsigned int i = 0; i < numAnimations; ++i) {
        aiAnimation *ai_anim = mScene->mAnimations[i] = new aiAnimation();

        Animation &anim = r.animations[i];

        ai_anim->mName = anim.name;
        ai_anim->mDuration = 0;
        ai_anim->mTicksPerSecond = 0;

        std::unordered_map<unsigned int, AnimationSamplers> samplers = GatherSamplers(anim);

        uint32_t numChannels = 0u;
        uint32_t numMorphMeshChannels = 0u;

        for (auto &iter : samplers) {
            if ((nullptr != iter.second.rotation) || (nullptr != iter.second.scale) || (nullptr != iter.second.translation)) {
                ++numChannels;
            }
            if (nullptr != iter.second.weight) {
                ++numMorphMeshChannels;
            }
        }

        ai_anim->mNumChannels = numChannels;
        if (ai_anim->mNumChannels > 0) {
            ai_anim->mChannels = new aiNodeAnim *[ai_anim->mNumChannels];
            std::fill(ai_anim->mChannels, ai_anim->mChannels + ai_anim->mNumChannels, nullptr);
            int j = 0;
            for (auto &iter : samplers) {
                if ((nullptr != iter.second.rotation) || (nullptr != iter.second.scale) || (nullptr != iter.second.translation)) {
                    ai_anim->mChannels[j] = CreateNodeAnim(r, r.nodes[iter.first], iter.second);
                    ++j;
                }
            }
        }

        ai_anim->mNumMorphMeshChannels = numMorphMeshChannels;
        if (ai_anim->mNumMorphMeshChannels > 0) {
            ai_anim->mMorphMeshChannels = new aiMeshMorphAnim *[ai_anim->mNumMorphMeshChannels];
            std::fill(ai_anim->mMorphMeshChannels, ai_anim->mMorphMeshChannels + ai_anim->mNumMorphMeshChannels, nullptr);
            int j = 0;
            for (auto &iter : samplers) {
                if (nullptr != iter.second.weight) {
                    ai_anim->mMorphMeshChannels[j] = CreateMeshMorphAnim(r, r.nodes[iter.first], iter.second);
                    ++j;
                }
            }
        }

        // Use the latest key-frame for the duration of the animation
        double maxDuration = 0;
        unsigned int maxNumberOfKeys = 0;
        for (unsigned int j = 0; j < ai_anim->mNumChannels; ++j) {
            auto chan = ai_anim->mChannels[j];
            if (chan->mNumPositionKeys) {
                auto lastPosKey = chan->mPositionKeys[chan->mNumPositionKeys - 1];
                if (lastPosKey.mTime > maxDuration) {
                    maxDuration = lastPosKey.mTime;
                }
                maxNumberOfKeys = std::max(maxNumberOfKeys, chan->mNumPositionKeys);
            }
            if (chan->mNumRotationKeys) {
                auto lastRotKey = chan->mRotationKeys[chan->mNumRotationKeys - 1];
                if (lastRotKey.mTime > maxDuration) {
                    maxDuration = lastRotKey.mTime;
                }
                maxNumberOfKeys = std::max(maxNumberOfKeys, chan->mNumRotationKeys);
            }
            if (chan->mNumScalingKeys) {
                auto lastScaleKey = chan->mScalingKeys[chan->mNumScalingKeys - 1];
                if (lastScaleKey.mTime > maxDuration) {
                    maxDuration = lastScaleKey.mTime;
                }
                maxNumberOfKeys = std::max(maxNumberOfKeys, chan->mNumScalingKeys);
            }
        }

        for (unsigned int j = 0; j < ai_anim->mNumMorphMeshChannels; ++j) {
            const auto *const chan = ai_anim->mMorphMeshChannels[j];

            if (0u != chan->mNumKeys) {
                const auto &lastKey = chan->mKeys[chan->mNumKeys - 1u];
                if (lastKey.mTime > maxDuration) {
                    maxDuration = lastKey.mTime;
                }
                maxNumberOfKeys = std::max(maxNumberOfKeys, chan->mNumKeys);
            }
        }

        ai_anim->mDuration = maxDuration;
        ai_anim->mTicksPerSecond = 1000.0;
    }
}

static unsigned int countEmbeddedTextures(glTF2::Asset &r) {
    unsigned int numEmbeddedTexs = 0;
    for (size_t i = 0; i < r.images.Size(); ++i) {
        if (r.images[i].HasData()) {
            numEmbeddedTexs += 1;
        }
    }

    return numEmbeddedTexs;
}

void glTF2Importer::ImportEmbeddedTextures(glTF2::Asset &r) {
    mEmbeddedTexIdxs.resize(r.images.Size(), -1);
    const unsigned int numEmbeddedTexs = countEmbeddedTextures(r);
    if (numEmbeddedTexs == 0) {
        return;
    }

    ASSIMP_LOG_DEBUG("Importing ", numEmbeddedTexs, " embedded textures");

    mScene->mTextures = new aiTexture *[numEmbeddedTexs];
    std::fill(mScene->mTextures, mScene->mTextures + numEmbeddedTexs, nullptr);

    // Add the embedded textures
    for (size_t i = 0; i < r.images.Size(); ++i) {
        Image &img = r.images[i];
        if (!img.HasData()) {
            continue;
        }

        int idx = mScene->mNumTextures++;
        mEmbeddedTexIdxs[i] = idx;

        aiTexture *tex = mScene->mTextures[idx] = new aiTexture();

        size_t length = img.GetDataLength();
        void *data = img.StealData();

        tex->mFilename = img.name;
        tex->mWidth = static_cast<unsigned int>(length);
        tex->mHeight = 0;
        tex->pcData = reinterpret_cast<aiTexel *>(data);

        if (!img.mimeType.empty()) {
            const char *ext = strchr(img.mimeType.c_str(), '/') + 1;
            if (ext) {
                if (strcmp(ext, "jpeg") == 0) {
                    ext = "jpg";
                } else if (strcmp(ext, "ktx2") == 0) { // basisu: ktx remains
                    ext = "kx2";
                } else if (strcmp(ext, "basis") == 0) { // basisu
                    ext = "bu";
                }

                size_t len = strlen(ext);
                if (len <= 3) {
                    strcpy(tex->achFormatHint, ext);
                }
            }
        }
    }
}

void glTF2Importer::ImportCommonMetadata(glTF2::Asset &a) {
    ASSIMP_LOG_DEBUG("Importing metadata");
    ai_assert(mScene->mMetaData == nullptr);
    const bool hasVersion = !a.asset.version.empty();
    const bool hasGenerator = !a.asset.generator.empty();
    const bool hasCopyright = !a.asset.copyright.empty();
    const bool hasSceneMetadata = a.scene->customExtensions;
    if (hasVersion || hasGenerator || hasCopyright || hasSceneMetadata) {
        mScene->mMetaData = new aiMetadata;
        if (hasVersion) {
            mScene->mMetaData->Add(AI_METADATA_SOURCE_FORMAT_VERSION, aiString(a.asset.version));
        }
        if (hasGenerator) {
            mScene->mMetaData->Add(AI_METADATA_SOURCE_GENERATOR, aiString(a.asset.generator));
        }
        if (hasCopyright) {
            mScene->mMetaData->Add(AI_METADATA_SOURCE_COPYRIGHT, aiString(a.asset.copyright));
        }
        if (hasSceneMetadata) {
            ParseExtensions(mScene->mMetaData, a.scene->customExtensions);
        }
    }
}

void glTF2Importer::InternReadFile(const std::string &pFile, aiScene *pScene, IOSystem *pIOHandler) {
    ASSIMP_LOG_DEBUG("Reading GLTF2 file");

    // clean all member arrays
    meshOffsets.clear();
    mVertexRemappingTables.clear();
    mEmbeddedTexIdxs.clear();

    this->mScene = pScene;

    // read the asset file
    glTF2::Asset asset(pIOHandler, static_cast<rapidjson::IRemoteSchemaDocumentProvider *>(mSchemaDocumentProvider));
    asset.Load(pFile,
               CheckMagicToken(
                   pIOHandler, pFile, AI_GLB_MAGIC_NUMBER, 1, 0,
                   static_cast<unsigned int>(strlen(AI_GLB_MAGIC_NUMBER))));
    if (asset.scene) {
        pScene->mName = asset.scene->name;
    }

    // Copy the data out
    ImportEmbeddedTextures(asset);
    ImportMaterials(asset);

    ImportMeshes(asset);

    ImportCameras(asset);
    ImportLights(asset);

    ImportNodes(asset);

    ImportAnimations(asset);

    ImportCommonMetadata(asset);

    if (pScene->mNumMeshes == 0) {
        pScene->mFlags |= AI_SCENE_FLAGS_INCOMPLETE;
    }
}

void glTF2Importer::SetupProperties(const Importer *pImp) {
    mSchemaDocumentProvider = static_cast<rapidjson::IRemoteSchemaDocumentProvider *>(pImp->GetPropertyPointer(AI_CONFIG_IMPORT_SCHEMA_DOCUMENT_PROVIDER));
}

#endif // ASSIMP_BUILD_NO_GLTF_IMPORTER
