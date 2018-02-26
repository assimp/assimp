/*
Open Asset Import Library (assimp)
----------------------------------------------------------------------

Copyright (c) 2006-2017, assimp team

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

#include "glTF2Exporter.h"

#include "Exceptional.h"
#include "StringComparison.h"
#include "ByteSwapper.h"

#include "SplitLargeMeshes.h"

#include <assimp/SceneCombiner.h>
#include <assimp/version.h>
#include <assimp/IOSystem.hpp>
#include <assimp/Exporter.hpp>
#include <assimp/material.h>
#include <assimp/scene.h>
#include <assimp/config.h>

// Header files, standard library.
#include <memory>
#include <inttypes.h>
#include <algorithm>

#include "glTF2AssetWriter.h"

using namespace rapidjson;

using namespace Assimp;
using namespace glTF2;

namespace Assimp {

    // ------------------------------------------------------------------------------------------------
    // Worker function for exporting a scene to GLTF. Prototyped and registered in Exporter.cpp
    void ExportSceneGLTF2(const char* pFile, IOSystem* pIOSystem, const aiScene* pScene, const ExportProperties* pProperties)
    {
        // invoke the exporter
        glTF2Exporter exporter(pFile, pIOSystem, pScene, pProperties, false);
    }

} // end of namespace Assimp

namespace {
    /*
     * Find min and max element component of a array component
     */
    class IDataReader{
    public:
        virtual float value() = 0;
        virtual void increment(size_t n=1) = 0;
    };
    template<typename DataType>
    class DataReader: public IDataReader{
    public:
        DataReader(void* data) {
            _data = static_cast<DataType*>(data);
        }
        float value(){
            return static_cast<float>(*_data);
        }
        void increment(size_t n=1){
            _data+=n;
        }
    private:
        DataType* _data;
    };
    
    static std::pair<std::vector<float>, std::vector<float>> minMaxElements(void* data, size_t elementCount, ComponentType compType, size_t componentCountIn, size_t componentCountOut){
        
        std::unique_ptr<IDataReader> dataReader;
        switch(compType){
            case ComponentType_BYTE:{
                dataReader.reset(new DataReader<int8_t>(data));
                break;
            }
            case ComponentType_UNSIGNED_BYTE:{
                dataReader.reset(new DataReader<uint8_t>(data));
                break;
            }
            case ComponentType_SHORT:{
                dataReader.reset(new DataReader<int16_t>(data));
                break;
            }
            case ComponentType_UNSIGNED_SHORT:{
                dataReader.reset(new DataReader<uint16_t>(data));
                break;
            }
            case ComponentType_UNSIGNED_INT:{
                dataReader.reset(new DataReader<uint32_t>(data));
                break;
            }
            case ComponentType_FLOAT:
            default: {
                dataReader.reset(new DataReader<float>(data));
                break;
            }
        }
        
        std::pair<std::vector<float>, std::vector<float>> minMaxValues;
        auto& minValues = minMaxValues.first;
        auto& maxValues = minMaxValues.second;
        minValues.reserve(componentCountOut);
        maxValues.reserve(componentCountOut);
        
        for (size_t i=0; i<componentCountOut; ++i){
            minValues.push_back(std::numeric_limits<float>::max());
            maxValues.push_back(-std::numeric_limits<float>::max());
        }
        
        float value;
        size_t offset = std::max(componentCountIn - componentCountOut, size_t(0));
        for (size_t i=0; i<elementCount; ++i) {
            for (size_t c=0; c<componentCountOut; ++c){
                value = dataReader->value();
                if (value<minValues[c]) {
                    minValues[c]=value;
                }
                if (value>maxValues[c]) {
                    maxValues[c]=value;
                }
                dataReader->increment();
            }
            dataReader->increment(offset);
        }
        
        return minMaxValues;
    }
    
    class AKeyExtraction {
    public:
        size_t keyCount;
        AttribType::Value attribInType;
        AttribType::Value attribOutType;
        ComponentType componentType;
        Ref<Accessor>& targetTimeAccessor;
        Ref<Accessor>& targetValueAccessor;
        
        AKeyExtraction(size_t iKeyCount,
                       AttribType::Value& iAttribInType,
                       AttribType::Value& iAttribOutType,
                       ComponentType& iComponentType,
                       Ref<Accessor>& iTargetTimeAccessor,
                       Ref<Accessor>& iTargetValueAccessor
                       ):
                        keyCount(iKeyCount),
                        attribInType(iAttribInType),
                        attribOutType(iAttribOutType),
                        componentType(iComponentType),
                        targetTimeAccessor(iTargetTimeAccessor),
                        targetValueAccessor(iTargetValueAccessor) {
            
        }
        
        virtual void* getTimeData() = 0;
        virtual void* getValueData() = 0;
        virtual void extract() = 0;
    };
    
    template <typename DataTypeIn, typename DataTypeOut>
    class KeyExtraction : public AKeyExtraction {
        
    public:
        KeyExtraction(DataTypeIn* iKeyData,
                      size_t iDataCount,
                      size_t iValueDataCount,
                      AttribType::Value iAttribInType,
                      AttribType::Value iAttribOutType,
                      ComponentType iComponentType,
                      Ref<Accessor>& iTargetTimeAccessor,
                      Ref<Accessor>& iTargetValueAccessor,
                      double iTimeCoef): AKeyExtraction(iDataCount, iAttribInType, iAttribOutType, iComponentType, iTargetTimeAccessor, iTargetValueAccessor),
                      _keyData(iKeyData),
                      _timeCoef(iTimeCoef) {
            timeData.resize(iDataCount);
            valueData.resize(iValueDataCount);
        }
        
        void* getTimeData() {
            return timeData.data();
        }
        
        void* getValueData() {
            return valueData.data();
        }
        
        inline void extract() {
            DataTypeIn* key = _keyData;
            for (size_t i = 0, iend = timeData.size(); i < iend; ++i, ++key){
                timeData[i] = static_cast<float>(key->mTime / _timeCoef);
                valueData[i] = key->mValue;
            }
        }
        
    private:
        DataTypeIn* _keyData;
        double _timeCoef;
        std::vector<float> timeData;
        std::vector<DataTypeOut> valueData;
    };
    
    template<>
    void KeyExtraction<aiQuatKey, float>::extract() {
        aiQuatKey* key = _keyData;
        float* value = valueData.data();
        for (size_t i = 0, iend = timeData.size(); i < iend; ++i, ++key, value+=4){
            timeData[i] = static_cast<float>(key->mTime / _timeCoef);
            *value = key->mValue.x;
            *(value+1) = key->mValue.y;
            *(value+2) = key->mValue.z;
            *(value+3) = key->mValue.w;
        }
    }
}


glTF2Exporter::glTF2Exporter(const char* filename, IOSystem* pIOSystem, const aiScene* pScene,
                           const ExportProperties* pProperties, bool /*isBinary*/)
    : mFilename(filename)
    , mIOSystem(pIOSystem)
    , mProperties(pProperties)
{
    aiScene* sceneCopy_tmp;
    SceneCombiner::CopyScene(&sceneCopy_tmp, pScene);
    std::unique_ptr<aiScene> sceneCopy(sceneCopy_tmp);

    SplitLargeMeshesProcess_Triangle tri_splitter;
    tri_splitter.SetLimit(0xffff);
    tri_splitter.Execute(sceneCopy.get());

    SplitLargeMeshesProcess_Vertex vert_splitter;
    vert_splitter.SetLimit(0xffff);
    vert_splitter.Execute(sceneCopy.get());

    mScene = sceneCopy.get();

    mAsset.reset( new Asset( pIOSystem ) );

    ExportMetadata();

    ExportMaterials();

    if (mScene->mRootNode) {
        ExportNodeHierarchy(mScene->mRootNode);
    }

    ExportMeshes();
    MergeMeshes();

    ExportScene();

    ExportAnimations();

    AssetWriter writer(*mAsset);

    writer.WriteFile(filename);
}

/*
 * Copy a 4x4 matrix from struct aiMatrix to typedef mat4.
 * Also converts from row-major to column-major storage.
 */
static void CopyValue(const aiMatrix4x4& v, mat4& o)
{
    o[ 0] = v.a1; o[ 1] = v.b1; o[ 2] = v.c1; o[ 3] = v.d1;
    o[ 4] = v.a2; o[ 5] = v.b2; o[ 6] = v.c2; o[ 7] = v.d2;
    o[ 8] = v.a3; o[ 9] = v.b3; o[10] = v.c3; o[11] = v.d3;
    o[12] = v.a4; o[13] = v.b4; o[14] = v.c4; o[15] = v.d4;
}

static void CopyValue(const aiMatrix4x4& v, aiMatrix4x4& o)
{
    o.a1 = v.a1; o.a2 = v.a2; o.a3 = v.a3; o.a4 = v.a4;
    o.b1 = v.b1; o.b2 = v.b2; o.b3 = v.b3; o.b4 = v.b4;
    o.c1 = v.c1; o.c2 = v.c2; o.c3 = v.c3; o.c4 = v.c4;
    o.d1 = v.d1; o.d2 = v.d2; o.d3 = v.d3; o.d4 = v.d4;
}

static void IdentityMatrix4(mat4& o)
{
    o[ 0] = 1; o[ 1] = 0; o[ 2] = 0; o[ 3] = 0;
    o[ 4] = 0; o[ 5] = 1; o[ 6] = 0; o[ 7] = 0;
    o[ 8] = 0; o[ 9] = 0; o[10] = 1; o[11] = 0;
    o[12] = 0; o[13] = 0; o[14] = 0; o[15] = 1;
}

inline Ref<Accessor> ExportData(Asset& a, std::string& meshName, Ref<Buffer>& buffer,
    unsigned int count, void* data, AttribType::Value typeIn, AttribType::Value typeOut, ComponentType compType, bool isIndices = false)
{
    if (!count || !data) return Ref<Accessor>();

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
    bv->byteOffset = unsigned(offset);
    bv->byteLength = length; //! The target that the WebGL buffer should be bound to.
    bv->byteStride = 0;
    bv->target = isIndices ? BufferViewTarget_ELEMENT_ARRAY_BUFFER : BufferViewTarget_ARRAY_BUFFER;

    // accessor
    Ref<Accessor> acc = a.accessors.Create(a.FindUniqueID(meshName, "accessor"));
    acc->bufferView = bv;
    acc->byteOffset = 0;
    acc->componentType = compType;
    acc->count = count;
    acc->type = typeOut;

    // calculate min and max values
    auto minmaxValues = minMaxElements(data, count, compType, numCompsIn, numCompsOut);
    acc->min = minmaxValues.first;
    acc->max = minmaxValues.second;

    // copy the data
    acc->WriteData(count, data, numCompsIn*bytesPerComp);

    return acc;
}

inline void SetSamplerWrap(SamplerWrap& wrap, aiTextureMapMode map)
{
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

void glTF2Exporter::GetTexSampler(const aiMaterial* mat, Ref<Texture> texture, aiTextureType tt, unsigned int slot)
{
    aiString aId;
    std::string id;
    if (aiGetMaterialString(mat, AI_MATKEY_GLTF_MAPPINGID(tt, slot), &aId) == AI_SUCCESS) {
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

        if (aiGetMaterialInteger(mat, AI_MATKEY_MAPPINGMODE_U(tt, slot), (int*)&mapU) == AI_SUCCESS) {
            SetSamplerWrap(texture->sampler->wrapS, mapU);
        }

        if (aiGetMaterialInteger(mat, AI_MATKEY_MAPPINGMODE_V(tt, slot), (int*)&mapV) == AI_SUCCESS) {
            SetSamplerWrap(texture->sampler->wrapT, mapV);
        }

        if (aiGetMaterialInteger(mat, AI_MATKEY_GLTF_MAPPINGFILTER_MAG(tt, slot), (int*)&filterMag) == AI_SUCCESS) {
            texture->sampler->magFilter = filterMag;
        }

        if (aiGetMaterialInteger(mat, AI_MATKEY_GLTF_MAPPINGFILTER_MIN(tt, slot), (int*)&filterMin) == AI_SUCCESS) {
            texture->sampler->minFilter = filterMin;
        }

        aiString name;
        if (aiGetMaterialString(mat, AI_MATKEY_GLTF_MAPPINGNAME(tt, slot), &name) == AI_SUCCESS) {
            texture->sampler->name = name.C_Str();
        }
    }
}

void glTF2Exporter::GetMatTexProp(const aiMaterial* mat, unsigned int& prop, const char* propName, aiTextureType tt, unsigned int slot)
{
    std::string textureKey = std::string(_AI_MATKEY_TEXTURE_BASE) + "." + propName;

    mat->Get(textureKey.c_str(), tt, slot, prop);
}

void glTF2Exporter::GetMatTexProp(const aiMaterial* mat, float& prop, const char* propName, aiTextureType tt, unsigned int slot)
{
    std::string textureKey = std::string(_AI_MATKEY_TEXTURE_BASE) + "." + propName;

    mat->Get(textureKey.c_str(), tt, slot, prop);
}

void glTF2Exporter::GetMatTex(const aiMaterial* mat, Ref<Texture>& texture, aiTextureType tt, unsigned int slot = 0)
{

    if (mat->GetTextureCount(tt) > 0) {
        aiString tex;

        if (mat->Get(AI_MATKEY_TEXTURE(tt, slot), tex) == AI_SUCCESS) {
            std::string path = tex.C_Str();

            if (path.size() > 0) {
                if (path[0] != '*') {
                    std::map<std::string, unsigned int>::iterator it = mTexturesByPath.find(path);
                    if (it != mTexturesByPath.end()) {
                        texture = mAsset->textures.Get(it->second);
                    }
                }

                if (!texture) {
                    std::string texId = mAsset->FindUniqueID("", "texture");
                    texture = mAsset->textures.Create(texId);
                    mTexturesByPath[path] = texture.GetIndex();

                    std::string imgId = mAsset->FindUniqueID("", "image");
                    texture->source = mAsset->images.Create(imgId);

                    if (path[0] == '*') { // embedded
                        aiTexture* tex = mScene->mTextures[atoi(&path[1])];

                        uint8_t* data = reinterpret_cast<uint8_t*>(tex->pcData);
                        texture->source->SetData(data, tex->mWidth, *mAsset);

                        if (tex->achFormatHint[0]) {
                            std::string mimeType = "image/";
                            mimeType += (memcmp(tex->achFormatHint, "jpg", 3) == 0) ? "jpeg" : tex->achFormatHint;
                            texture->source->mimeType = mimeType;
                        }
                    }
                    else {
                        texture->source->uri = path;
                    }

                    GetTexSampler(mat, texture, tt, slot);
                }
            }
        }
    }
}

void glTF2Exporter::GetMatTex(const aiMaterial* mat, TextureInfo& prop, aiTextureType tt, unsigned int slot = 0)
{
    Ref<Texture>& texture = prop.texture;

    GetMatTex(mat, texture, tt, slot);

    if (texture) {
        GetMatTexProp(mat, prop.texCoord, "texCoord", tt, slot);
    }
}

void glTF2Exporter::GetMatTex(const aiMaterial* mat, NormalTextureInfo& prop, aiTextureType tt, unsigned int slot = 0)
{
    Ref<Texture>& texture = prop.texture;

    GetMatTex(mat, texture, tt, slot);

    if (texture) {
        GetMatTexProp(mat, prop.texCoord, "texCoord", tt, slot);
        GetMatTexProp(mat, prop.scale, "scale", tt, slot);
    }
}

void glTF2Exporter::GetMatTex(const aiMaterial* mat, OcclusionTextureInfo& prop, aiTextureType tt, unsigned int slot = 0)
{
    Ref<Texture>& texture = prop.texture;

    GetMatTex(mat, texture, tt, slot);

    if (texture) {
        GetMatTexProp(mat, prop.texCoord, "texCoord", tt, slot);
        GetMatTexProp(mat, prop.strength, "strength", tt, slot);
    }
}

aiReturn glTF2Exporter::GetMatColor(const aiMaterial* mat, vec4& prop, const char* propName, int type, int idx)
{
    aiColor4D col;
    aiReturn result = mat->Get(propName, type, idx, col);

    if (result == AI_SUCCESS) {
        prop[0] = col.r; prop[1] = col.g; prop[2] = col.b; prop[3] = col.a;
    }

    return result;
}

aiReturn glTF2Exporter::GetMatColor(const aiMaterial* mat, vec3& prop, const char* propName, int type, int idx)
{
    aiColor3D col;
    aiReturn result = mat->Get(propName, type, idx, col);

    if (result == AI_SUCCESS) {
        prop[0] = col.r; prop[1] = col.g; prop[2] = col.b;
    }

    return result;
}

void glTF2Exporter::ExportMaterials()
{
    aiString aiName;
    for (unsigned int i = 0; i < mScene->mNumMaterials; ++i) {
        const aiMaterial* mat = mScene->mMaterials[i];

        std::string id = "material_" + to_string(i);

        Ref<Material> m = mAsset->materials.Create(id);

        std::string name;
        if (mat->Get(AI_MATKEY_NAME, aiName) == AI_SUCCESS) {
            name = aiName.C_Str();
        }
        name = mAsset->FindUniqueID(name, "material");

        m->name = name;
        
        // The value is true if the material is from common material source or the export option is true
        bool hasCommonSource = false;
        mat->Get(AI_MATKEY_GLTF_COMMON, hasCommonSource);
        bool exportAsCommonMaterial = hasCommonSource || (mProperties->HasPropertyBool(AI_CONFIG_EXPORT_GLTF2_MATERIAL_COMMON) && mProperties->GetPropertyBool(AI_CONFIG_EXPORT_GLTF2_MATERIAL_COMMON));
        // The value is false if the export as a common material is active and the export option is false
        bool exportAsPBRMaterial = !(exportAsCommonMaterial && mProperties->HasPropertyBool(AI_CONFIG_EXPORT_GLTF2_MATERIAL_PBR) && !mProperties->GetPropertyBool(AI_CONFIG_EXPORT_GLTF2_MATERIAL_PBR));
        
        
        if (exportAsPBRMaterial) {
            
            GetMatTex(mat, m->pbrMetallicRoughness.baseColorTexture, AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_BASE_COLOR_TEXTURE);
            
            if (!m->pbrMetallicRoughness.baseColorTexture.texture) {
                //if there wasn't a baseColorTexture defined in the source, fallback to any diffuse texture
                GetMatTex(mat, m->pbrMetallicRoughness.baseColorTexture, aiTextureType_DIFFUSE);
                //exportAsCommonMaterial = exportAsCommonMaterial || !hasPbrSpecularGlossiness;
            }
            
            GetMatTex(mat, m->pbrMetallicRoughness.metallicRoughnessTexture, AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_METALLICROUGHNESS_TEXTURE);
            
            if (GetMatColor(mat, m->pbrMetallicRoughness.baseColorFactor, AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_BASE_COLOR_FACTOR) != AI_SUCCESS) {
                // if baseColorFactor wasn't defined, then the source is likely not a metallic roughness material.
                //a fallback to any diffuse color should be used instead
                GetMatColor(mat, m->pbrMetallicRoughness.baseColorFactor, AI_MATKEY_COLOR_DIFFUSE);
                //exportAsCommonMaterial = exportAsCommonMaterial || !hasPbrSpecularGlossiness;
            }
            
            if (mat->Get(AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_METALLIC_FACTOR, m->pbrMetallicRoughness.metallicFactor) != AI_SUCCESS) {
                //if metallicFactor wasn't defined, then the source is likely not a PBR file, and the metallicFactor should be 0
                m->pbrMetallicRoughness.metallicFactor = 0;
                //exportAsCommonMaterial = exportAsCommonMaterial || !hasPbrSpecularGlossiness;
            }
            
            // get roughness if source is gltf2 file
            if (mat->Get(AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_ROUGHNESS_FACTOR, m->pbrMetallicRoughness.roughnessFactor) != AI_SUCCESS) {
                // otherwise, try to derive and convert from specular + shininess values
                aiColor4D specularColor;
                ai_real shininess;
                
                if (
                    mat->Get(AI_MATKEY_COLOR_SPECULAR, specularColor) == AI_SUCCESS &&
                    mat->Get(AI_MATKEY_SHININESS, shininess) == AI_SUCCESS
                    ) {
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
        }

        GetMatTex(mat, m->normalTexture, aiTextureType_NORMALS);
        GetMatTex(mat, m->occlusionTexture, aiTextureType_LIGHTMAP);
        GetMatTex(mat, m->emissiveTexture, aiTextureType_EMISSIVE);
        GetMatColor(mat, m->emissiveFactor, AI_MATKEY_COLOR_EMISSIVE);

        mat->Get(AI_MATKEY_TWOSIDED, m->doubleSided);
        mat->Get(AI_MATKEY_GLTF_ALPHACUTOFF, m->alphaCutoff);

        aiString alphaMode;

        if (mat->Get(AI_MATKEY_GLTF_ALPHAMODE, alphaMode) == AI_SUCCESS) {
            m->alphaMode = alphaMode.C_Str();
        } else {
            float opacity;

            if (mat->Get(AI_MATKEY_OPACITY, opacity) == AI_SUCCESS) {
                if (opacity < 1) {
                    m->alphaMode = "BLEND";
                    m->pbrMetallicRoughness.baseColorFactor[3] *= opacity;
                }
            }
        }
        
        if (exportAsPBRMaterial) {
            // Check if the source is a specular glossiness material instead of a metallic roughness material
            bool hasPbrSpecularGlossiness = false;
            mat->Get(AI_MATKEY_GLTF_PBRSPECULARGLOSSINESS, hasPbrSpecularGlossiness);
            
            if (hasPbrSpecularGlossiness) {
                
                if (!mAsset->extensionsUsed.at("KHR_materials_pbrSpecularGlossiness")) {
                    mAsset->extensionsUsed["KHR_materials_pbrSpecularGlossiness"] = true;
                }
                
                PbrSpecularGlossiness pbrSG;
                
                GetMatColor(mat, pbrSG.diffuseFactor, AI_MATKEY_COLOR_DIFFUSE);
                GetMatColor(mat, pbrSG.specularFactor, AI_MATKEY_COLOR_SPECULAR);
                
                if (mat->Get(AI_MATKEY_GLTF_PBRSPECULARGLOSSINESS_GLOSSINESS_FACTOR, pbrSG.glossinessFactor) != AI_SUCCESS) {
                    float shininess;
                    
                    if (mat->Get(AI_MATKEY_SHININESS, shininess)) {
                        pbrSG.glossinessFactor = shininess / 1000;
                    }
                }
                
                GetMatTex(mat, pbrSG.diffuseTexture, aiTextureType_DIFFUSE);
                GetMatTex(mat, pbrSG.specularGlossinessTexture, aiTextureType_SPECULAR);
                
                m->pbrSpecularGlossiness = Nullable<PbrSpecularGlossiness>(pbrSG);
            }
        }
        
        if (exportAsCommonMaterial) {
            
            if (!mAsset->extensionsUsed.at("KHR_materials_common")) {
                mAsset->extensionsUsed["KHR_materials_common"] = true;
            }
            
            Common common;
            
            GetMatColor(mat, common.ambientFactor, AI_MATKEY_COLOR_AMBIENT);
            GetMatColor(mat, common.diffuseFactor, AI_MATKEY_COLOR_DIFFUSE);
            GetMatColor(mat, common.emissiveFactor, AI_MATKEY_COLOR_EMISSIVE);
            GetMatColor(mat, common.specularFactor, AI_MATKEY_COLOR_SPECULAR);
            
            GetMatTex(mat, common.ambientTexture, aiTextureType_AMBIENT);
            GetMatTex(mat, common.diffuseTexture, aiTextureType_DIFFUSE);
            GetMatTex(mat, common.emissiveTexture, aiTextureType_EMISSIVE);
            GetMatTex(mat, common.specularTexture, aiTextureType_SPECULAR);
            
            mat->Get(AI_MATKEY_TWOSIDED, common.doubleSided);
            mat->Get(AI_MATKEY_SHININESS, common.shininess);
            mat->Get(AI_MATKEY_OPACITY, common.transparency);
            
            int shadingMode;
            if (AI_SUCCESS == mat->Get(AI_MATKEY_SHADING_MODEL, shadingMode)) {
                std::string technique;
                
                switch (shadingMode) {
                    case aiShadingMode_Blinn: {
                        technique = "BLINN";
                    }
                    break;
                    case aiShadingMode_Phong: {
                        technique = "PHONG";
                    }
                    break;
                    case aiShadingMode_Lambert: {
                        technique = "LAMBERT";
                    }
                    break;
                    case aiShadingMode_Constant: {
                        technique = "CONSTANT";
                    }
                    break;
                    default: {
                        technique = "PHONG";
                    }
                    break;
                }
                
                common.technique = technique;
                
                if (!exportAsPBRMaterial) {
                    mAsset->extensionsRequired.at("KHR_materials_common") = true;
                }
            }
            
            aiString alphaMode;
            if (common.transparency!=1.0f && AI_SUCCESS == mat->Get(AI_MATKEY_GLTF_ALPHAMODE, alphaMode)) {
                if (aiString("BLEND")==alphaMode) {
                    common.transparent = true;
                }
            }
            
            m->common = Nullable<Common>(common);
        }
    }
}

/*
 * Search through node hierarchy and find the node containing the given meshID.
 * Returns true on success, and false otherwise.
 */
bool FindMeshNode(Ref<Node>& nodeIn, Ref<Node>& meshNode, std::string meshID)
{
    for (unsigned int i = 0; i < nodeIn->meshes.size(); ++i) {
        if (meshID.compare(nodeIn->meshes[i]->id) == 0) {
            meshNode = nodeIn;
            return true;
        }
    }

    for (unsigned int i = 0; i < nodeIn->children.size(); ++i) {
        if(FindMeshNode(nodeIn->children[i], meshNode, meshID)) {
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
Ref<Node> FindSkeletonRootJoint(Ref<Skin>& skinRef)
{
    Ref<Node> startNodeRef;
    Ref<Node> parentNodeRef;

    // Arbitrarily use the first joint to start the search.
    startNodeRef = skinRef->jointNames[0];
    parentNodeRef = skinRef->jointNames[0];

    do {
        startNodeRef = parentNodeRef;
        parentNodeRef = startNodeRef->parent;
    } while (!parentNodeRef->jointName.empty());

    return parentNodeRef;
}

void ExportSkin(Asset& mAsset, const aiMesh* aimesh, Ref<Mesh>& meshRef, Ref<Buffer>& bufferRef, Ref<Skin>& skinRef, std::vector<aiMatrix4x4>& inverseBindMatricesData)
{
    if (aimesh->mNumBones < 1) {
        return;
    }

    // Store the vertex joint and weight data.
    const size_t NumVerts( aimesh->mNumVertices );
    vec4* vertexJointData = new vec4[ NumVerts ];
    vec4* vertexWeightData = new vec4[ NumVerts ];
    int* jointsPerVertex = new int[ NumVerts ];
    for (size_t i = 0; i < NumVerts; ++i) {
        jointsPerVertex[i] = 0;
        for (size_t j = 0; j < 4; ++j) {
            vertexJointData[i][j] = 0;
            vertexWeightData[i][j] = 0;
        }
    }

    for (unsigned int idx_bone = 0; idx_bone < aimesh->mNumBones; ++idx_bone) {
        const aiBone* aib = aimesh->mBones[idx_bone];

        // aib->mName   =====>  skinRef->jointNames
        // Find the node with id = mName.
        Ref<Node> nodeRef = mAsset.nodes.Get(aib->mName.C_Str());
        nodeRef->jointName = nodeRef->name;

        unsigned int jointNamesIndex = 0;
        bool addJointToJointNames = true;
        for ( unsigned int idx_joint = 0; idx_joint < skinRef->jointNames.size(); ++idx_joint) {
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
            float vertWeight      = aib->mWeights[idx_weights].mWeight;

            // A vertex can only have at most four joint weights. Ignore all others.
            if (jointsPerVertex[vertexId] > 3) {
                continue;
            }

            vertexJointData[vertexId][jointsPerVertex[vertexId]] = static_cast<float>(jointNamesIndex);
            vertexWeightData[vertexId][jointsPerVertex[vertexId]] = vertWeight;

            jointsPerVertex[vertexId] += 1;
        }

    } // End: for-loop mNumMeshes

    Mesh::Primitive& p = meshRef->primitives.back();
    Ref<Accessor> vertexJointAccessor = ExportData(mAsset, skinRef->id, bufferRef, aimesh->mNumVertices, vertexJointData, AttribType::VEC4, AttribType::VEC4, ComponentType_FLOAT);
    if ( vertexJointAccessor ) {
        p.attributes.joint.push_back( vertexJointAccessor );
    }

    Ref<Accessor> vertexWeightAccessor = ExportData(mAsset, skinRef->id, bufferRef, aimesh->mNumVertices, vertexWeightData, AttribType::VEC4, AttribType::VEC4, ComponentType_FLOAT);
    if ( vertexWeightAccessor ) {
        p.attributes.weight.push_back( vertexWeightAccessor );
    }
    delete[] jointsPerVertex;
    delete[] vertexWeightData;
    delete[] vertexJointData;
}

void glTF2Exporter::ExportMeshes()
{
    // Not for
    //     using IndicesType = decltype(aiFace::mNumIndices);
    // But yes for
    //     using IndicesType = unsigned short;
    // because "ComponentType_UNSIGNED_SHORT" used for indices. And it's a maximal type according to glTF specification.
    typedef unsigned short IndicesType;

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
        const aiMesh* aim = mScene->mMeshes[idx_mesh];
        if(aim->HasBones()) {
            createSkin = true;
            break;
        }
    }

    Ref<Skin> skinRef;
    std::string skinName = mAsset->FindUniqueID("skin", "skin");
    std::vector<aiMatrix4x4> inverseBindMatricesData;
    if(createSkin) {
        skinRef = mAsset->skins.Create(skinName);
        skinRef->name = skinName;
    }
    //----------------------------------------

	for (unsigned int idx_mesh = 0; idx_mesh < mScene->mNumMeshes; ++idx_mesh) {
		const aiMesh* aim = mScene->mMeshes[idx_mesh];

        std::string name = aim->mName.C_Str();

        std::string meshId = mAsset->FindUniqueID(name, "mesh");
        Ref<Mesh> m = mAsset->meshes.Create(meshId);
        m->primitives.resize(1);
        Mesh::Primitive& p = m->primitives.back();

        m->name = name;

        p.material = mAsset->materials.Get(aim->mMaterialIndex);

		/******************* Vertices ********************/
        Ref<Accessor> v = ExportData(*mAsset, meshId, b, aim->mNumVertices, aim->mVertices, AttribType::VEC3, AttribType::VEC3, ComponentType_FLOAT);
		if (v) p.attributes.position.push_back(v);

		/******************** Normals ********************/
		Ref<Accessor> n = ExportData(*mAsset, meshId, b, aim->mNumVertices, aim->mNormals, AttribType::VEC3, AttribType::VEC3, ComponentType_FLOAT);
		if (n) p.attributes.normal.push_back(n);

		/************** Texture coordinates **************/
        for (int i = 0; i < AI_MAX_NUMBER_OF_TEXTURECOORDS; ++i) {
            // Flip UV y coords
            if (aim -> mNumUVComponents[i] > 1) {
                for (unsigned int j = 0; j < aim->mNumVertices; ++j) {
                    aim->mTextureCoords[i][j].y = 1 - aim->mTextureCoords[i][j].y;
                }
            }

            if (aim->mNumUVComponents[i] > 0) {
                AttribType::Value type = (aim->mNumUVComponents[i] == 2) ? AttribType::VEC2 : AttribType::VEC3;

				Ref<Accessor> tc = ExportData(*mAsset, meshId, b, aim->mNumVertices, aim->mTextureCoords[i], AttribType::VEC3, type, ComponentType_FLOAT, false);
				if (tc) p.attributes.texcoord.push_back(tc);
			}
		}

		/*************** Vertices indices ****************/
		if (aim->mNumFaces > 0) {
			std::vector<IndicesType> indices;
			unsigned int nIndicesPerFace = aim->mFaces[0].mNumIndices;
            indices.resize(aim->mNumFaces * nIndicesPerFace);
            for (size_t i = 0; i < aim->mNumFaces; ++i) {
                for (size_t j = 0; j < nIndicesPerFace; ++j) {
                    indices[i*nIndicesPerFace + j] = uint16_t(aim->mFaces[i].mIndices[j]);
                }
            }

			p.indices = ExportData(*mAsset, meshId, b, unsigned(indices.size()), &indices[0], AttribType::SCALAR, AttribType::SCALAR, ComponentType_UNSIGNED_SHORT, true);
		}

        switch (aim->mPrimitiveTypes) {
            case aiPrimitiveType_POLYGON:
                p.mode = PrimitiveMode_TRIANGLES; break; // TODO implement this
            case aiPrimitiveType_LINE:
                p.mode = PrimitiveMode_LINES; break;
            case aiPrimitiveType_POINT:
                p.mode = PrimitiveMode_POINTS; break;
            default: // aiPrimitiveType_TRIANGLE
                p.mode = PrimitiveMode_TRIANGLES;
        }

        /*************** Skins ****************/
        if(aim->HasBones()) {
            ExportSkin(*mAsset, aim, m, b, skinRef, inverseBindMatricesData);
        }
    }

    //----------------------------------------
    // Finish the skin
    // Create the Accessor for skinRef->inverseBindMatrices
    if (createSkin) {
        mat4* invBindMatrixData = new mat4[inverseBindMatricesData.size()];
        for ( unsigned int idx_joint = 0; idx_joint < inverseBindMatricesData.size(); ++idx_joint) {
            CopyValue(inverseBindMatricesData[idx_joint], invBindMatrixData[idx_joint]);
        }

        Ref<Accessor> invBindMatrixAccessor = ExportData(*mAsset, skinName, b, static_cast<unsigned int>(inverseBindMatricesData.size()), invBindMatrixData, AttribType::MAT4, AttribType::MAT4, ComponentType_FLOAT);
        if (invBindMatrixAccessor) skinRef->inverseBindMatrices = invBindMatrixAccessor;

        // Identity Matrix   =====>  skinRef->bindShapeMatrix
        // Temporary. Hard-coded identity matrix here
        skinRef->bindShapeMatrix.isPresent = true;
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
            meshNode->skeletons.push_back(rootJoint);
            meshNode->skin = skinRef;
        }
    }
}

//merges a node's multiple meshes (with one primitive each) into one mesh with multiple primitives
void glTF2Exporter::MergeMeshes()
{
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
                    mesh->primitives.end()
                );

                //remove the mesh from the list of meshes
                unsigned int removedIndex = mAsset->meshes.Remove(mesh->id.c_str());

                //find the presence of the removed mesh in other nodes
                for (unsigned int nn = 0; nn < mAsset->nodes.Size(); ++nn) {
                    Ref<Node> node = mAsset->nodes.Get(nn);

                    for (unsigned int mm = 0; mm < node->meshes.size(); ++mm) {
                        Ref<Mesh>& meshRef = node->meshes.at(mm);
                        unsigned int meshIndex = meshRef.GetIndex();

                        if (meshIndex == removedIndex) {
                            node->meshes.erase(node->meshes.begin() + mm);
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
unsigned int glTF2Exporter::ExportNodeHierarchy(const aiNode* n)
{
    Ref<Node> node = mAsset->nodes.Create(mAsset->FindUniqueID(n->mName.C_Str(), "node"));

    if (!n->mTransformation.IsIdentity()) {
        node->matrix.isPresent = true;
        CopyValue(n->mTransformation, node->matrix.value);
    }

    for (unsigned int i = 0; i < n->mNumMeshes; ++i) {
        node->meshes.push_back(mAsset->meshes.Get(n->mMeshes[i]));
    }

    for (unsigned int i = 0; i < n->mNumChildren; ++i) {
        unsigned int idx = ExportNode(n->mChildren[i], node);
        node->children.push_back(mAsset->nodes.Get(idx));
    }

    return node.GetIndex();
}

/*
 * Export node and recursively calls ExportNode for all children.
 * Since these nodes are not the root node, we also export the parent Ref<Node>
 */
unsigned int glTF2Exporter::ExportNode(const aiNode* n, Ref<Node>& parent)
{
    std::string name = mAsset->FindUniqueID(n->mName.C_Str(), "node");
    Ref<Node> node = mAsset->nodes.Create(name);

    node->parent = parent;
    node->name = name;

    if (!n->mTransformation.IsIdentity()) {
        node->matrix.isPresent = true;
        CopyValue(n->mTransformation, node->matrix.value);
    }

    for (unsigned int i = 0; i < n->mNumMeshes; ++i) {
        node->meshes.push_back(mAsset->meshes.Get(n->mMeshes[i]));
    }

    for (unsigned int i = 0; i < n->mNumChildren; ++i) {
        unsigned int idx = ExportNode(n->mChildren[i], node);
        node->children.push_back(mAsset->nodes.Get(idx));
    }

    return node.GetIndex();
}


void glTF2Exporter::ExportScene()
{
    const char* sceneName = "defaultScene";
    Ref<Scene> scene = mAsset->scenes.Create(sceneName);

    // root node will be the first one exported (idx 0)
    if (mAsset->nodes.Size() > 0) {
        scene->nodes.push_back(mAsset->nodes.Get(0u));
    }

    // set as the default scene
    mAsset->scene = scene;
}

void glTF2Exporter::ExportMetadata()
{
    AssetMetadata& asset = mAsset->asset;
    asset.version = "2.0";

    char buffer[256];
    ai_snprintf(buffer, 256, "Open Asset Import Library (assimp v%d.%d.%d)",
        aiGetVersionMajor(), aiGetVersionMinor(), aiGetVersionRevision());

    asset.generator = buffer;
}

inline void ExtractAnimationData(Asset& mAsset, std::string& animId, Ref<Animation>& animRef, Ref<Buffer>& buffer, const aiNodeAnim* nodeChannel, float ticksPerSecond)
{
    // Loop over the data and check to see if it exactly matches an existing buffer.
    //    If yes, then reference the existing corresponding accessor.
    //    Otherwise, add to the buffer and create a new accessor.

    struct {
        size_t position = 0;
        size_t rotation = 1;
        size_t scaling = 2;
        
        Ref<Accessor>& getTimeAccessor(glTF2::Animation::AnimParameters& parameters, size_t timeChannel) {
            return timeChannel==0 ? parameters.TIME : (timeChannel==1 ? parameters.TIME2 : parameters.TIME3);
        }
    } timeChannels;
    
    std::vector<std::unique_ptr<AKeyExtraction>> keyExtractions;
    keyExtractions.push_back(std::unique_ptr<AKeyExtraction>(new KeyExtraction<aiVectorKey, aiVector3D>(nodeChannel->mPositionKeys, nodeChannel->mNumPositionKeys, nodeChannel->mNumPositionKeys, AttribType::VEC3, AttribType::VEC3, ComponentType_FLOAT, timeChannels.getTimeAccessor(animRef->Parameters, timeChannels.position), animRef->Parameters.translation, ticksPerSecond
                                                                                                        )));
    keyExtractions.push_back(std::unique_ptr<AKeyExtraction>(new KeyExtraction<aiVectorKey, aiVector3D>(nodeChannel->mScalingKeys, nodeChannel->mNumScalingKeys, nodeChannel->mNumScalingKeys, AttribType::VEC3, AttribType::VEC3, ComponentType_FLOAT, timeChannels.getTimeAccessor(animRef->Parameters, timeChannels.scaling), animRef->Parameters.scale, ticksPerSecond
                                                                                                        )));
    keyExtractions.push_back(std::unique_ptr<AKeyExtraction>(new KeyExtraction<aiQuatKey, float>(nodeChannel->mRotationKeys, nodeChannel->mNumRotationKeys, 4*nodeChannel->mNumRotationKeys, AttribType::VEC4, AttribType::VEC4, ComponentType_FLOAT, timeChannels.getTimeAccessor(animRef->Parameters, timeChannels.rotation), animRef->Parameters.rotation, ticksPerSecond
                                                                                                        )));
    
    //-------------------------------------------------------
    // Extract the parameter data
    for (auto& keyExtraction : keyExtractions) {
        keyExtraction->extract();
        
        Ref<Accessor> timeAccessor = ExportData(mAsset, animId, buffer, static_cast<unsigned int>(keyExtraction->keyCount), keyExtraction->getTimeData(), AttribType::SCALAR, AttribType::SCALAR, ComponentType_FLOAT);
        Ref<Accessor> valueAccessor = ExportData(mAsset, animId, buffer, static_cast<unsigned int>(keyExtraction->keyCount), keyExtraction->getValueData(), keyExtraction->attribInType, keyExtraction->attribOutType, keyExtraction->componentType);
        
        if (timeAccessor) {
            keyExtraction->targetTimeAccessor = timeAccessor;
        }
        if (valueAccessor) {
            keyExtraction->targetValueAccessor = valueAccessor;
        }
    }
}

void glTF2Exporter::ExportAnimations()
{
    Ref<Buffer> bufferRef = mAsset->buffers.Get(unsigned (0));

    for (unsigned int i = 0; i < mScene->mNumAnimations; ++i) {
        const aiAnimation* anim = mScene->mAnimations[i];

        std::string nameAnim = "anim";
        if (anim->mName.length > 0) {
            nameAnim = anim->mName.C_Str();
        }

        for (unsigned int channelIndex = 0; channelIndex < anim->mNumChannels; ++channelIndex) {
            const aiNodeAnim* nodeChannel = anim->mChannels[channelIndex];

            // It appears that assimp stores this type of animation as multiple animations.
            // where each aiNodeAnim in mChannels animates a specific node.
            std::string name = nameAnim + "_" + to_string(channelIndex);
            name = mAsset->FindUniqueID(name, "animation");
            Ref<Animation> animRef = mAsset->animations.Create(name);

            // Parameters
            ExtractAnimationData(*mAsset, name, animRef, bufferRef, nodeChannel, static_cast<float>(anim->mTicksPerSecond));

            for (unsigned int j = 0; j < 3; ++j) {
                std::string channelType;
                std::string channelTime;
                int channelSize;
                switch (j) {
                    case 0:
                        channelType = "rotation";
                        channelTime = "TIME2";
                        channelSize = nodeChannel->mNumRotationKeys;
                        break;
                    case 1:
                        channelType = "scale";
                        channelTime = "TIME3";
                        channelSize = nodeChannel->mNumScalingKeys;
                        break;
                    case 2:
                        channelType = "translation";
                        channelTime = "TIME";
                        channelSize = nodeChannel->mNumPositionKeys;
                        break;
                }

                if (channelSize < 1) { continue; }

                Animation::AnimChannel tmpAnimChannel;
                Animation::AnimSampler tmpAnimSampler;

                tmpAnimChannel.sampler = static_cast<int>(animRef->Samplers.size());
                tmpAnimChannel.target.path = channelType;
                tmpAnimSampler.output = channelType;
                tmpAnimSampler.id = name + "_" + channelType;

                tmpAnimChannel.target.node = mAsset->nodes.Get(nodeChannel->mNodeName.C_Str());

                tmpAnimSampler.input = channelTime;
                tmpAnimSampler.interpolation = "LINEAR";

                animRef->Channels.push_back(tmpAnimChannel);
                animRef->Samplers.push_back(tmpAnimSampler);
            }

        }

        // Assimp documentation staes this is not used (not implemented)
        // for (unsigned int channelIndex = 0; channelIndex < anim->mNumMeshChannels; ++channelIndex) {
        //     const aiMeshAnim* meshChannel = anim->mMeshChannels[channelIndex];
        // }

    } // End: for-loop mNumAnimations
}


#endif // ASSIMP_BUILD_NO_GLTF_EXPORTER
#endif // ASSIMP_BUILD_NO_EXPORT
