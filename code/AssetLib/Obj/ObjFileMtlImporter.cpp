/*
---------------------------------------------------------------------------
Open Asset Import Library (assimp)
---------------------------------------------------------------------------

Copyright (c) 2006-2020, assimp team

All rights reserved.

Redistribution and use of this software in source and binary forms,
with or without modification, are permitted provided that the following
conditions are met:

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
---------------------------------------------------------------------------
*/

#ifndef ASSIMP_BUILD_NO_OBJ_IMPORTER

#include "ObjFileMtlImporter.h"
#include "ObjFileData.h"
#include "ObjTools.h"
#include <assimp/ParsingUtils.h>
#include <assimp/fast_atof.h>
#include <assimp/material.h>
#include <stdlib.h>
#include <assimp/DefaultLogger.hpp>

namespace Assimp {

// Material specific token (case insensitive compare)
static constexpr char DiffuseTexture[] = "map_Kd";
static constexpr char AmbientTexture[] = "map_Ka";
static constexpr char SpecularTexture[] = "map_Ks";
static constexpr char OpacityTexture[] = "map_d";
static constexpr char EmissiveTexture1[] = "map_emissive";
static constexpr char EmissiveTexture2[] = "map_Ke";
static constexpr char BumpTexture1[] = "map_bump";
static constexpr char BumpTexture2[] = "bump";
static constexpr char NormalTextureV1[] = "map_Kn";
static constexpr char NormalTextureV2[] = "norm";
static constexpr char ReflectionTexture[] = "refl";
static constexpr char DisplacementTexture1[] = "map_disp";
static constexpr char DisplacementTexture2[] = "disp";
static constexpr char SpecularityTexture[] = "map_ns";
static constexpr char RoughnessTexture[] = "map_Pr";
static constexpr char MetallicTexture[] = "map_Pm";
static constexpr char SheenTexture[] = "map_Ps";
static constexpr char RMATexture[] = "map_Ps";

// texture option specific token
static constexpr char BlendUOption[] = "-blendu";
static constexpr char BlendVOption[] = "-blendv";
static constexpr char BoostOption[] = "-boost";
static constexpr char ModifyMapOption[] = "-mm";
static constexpr char OffsetOption[] = "-o";
static constexpr char ScaleOption[] = "-s";
static constexpr char TurbulenceOption[] = "-t";
static constexpr char ResolutionOption[] = "-texres";
static constexpr char ClampOption[] = "-clamp";
static constexpr char BumpOption[] = "-bm";
static constexpr char ChannelOption[] = "-imfchan";
static constexpr char TypeOption[] = "-type";

// -------------------------------------------------------------------
//  Constructor
ObjFileMtlImporter::ObjFileMtlImporter(std::vector<char> &buffer,
        const std::string &,
        ObjFile::Model *pModel) :
        m_DataIt(buffer.begin()),
        m_DataItEnd(buffer.end()),
        m_pModel(pModel),
        m_uiLine(0),
        m_buffer() {
    ai_assert(nullptr != m_pModel);
    m_buffer.resize(BUFFERSIZE);
    std::fill(m_buffer.begin(), m_buffer.end(), '\0');
    if (nullptr == m_pModel->mDefaultMaterial) {
        m_pModel->mDefaultMaterial = new ObjFile::Material;
        m_pModel->mDefaultMaterial->MaterialName.Set("default");
    }
    load();
}

// -------------------------------------------------------------------
//  Destructor
ObjFileMtlImporter::~ObjFileMtlImporter() = default;

// -------------------------------------------------------------------
//  Loads the material description
void ObjFileMtlImporter::load() {
    if (m_DataIt == m_DataItEnd)
        return;

    while (m_DataIt != m_DataItEnd) {
        switch (*m_DataIt) {
            case 'k':
            case 'K': {
                ++m_DataIt;
                if (*m_DataIt == 'a') // Ambient color
                {
                    ++m_DataIt;
                    if (m_pModel->mCurrentMaterial != nullptr)
                        getColorRGBA(&m_pModel->mCurrentMaterial->ambient);
                } else if (*m_DataIt == 'd') {
                    // Diffuse color
                    ++m_DataIt;
                    if (m_pModel->mCurrentMaterial != nullptr)
                        getColorRGBA(&m_pModel->mCurrentMaterial->diffuse);
                } else if (*m_DataIt == 's') {
                    ++m_DataIt;
                    if (m_pModel->mCurrentMaterial != nullptr)
                        getColorRGBA(&m_pModel->mCurrentMaterial->specular);
                } else if (*m_DataIt == 'e') {
                    ++m_DataIt;
                    if (m_pModel->mCurrentMaterial != nullptr)
                        getColorRGBA(&m_pModel->mCurrentMaterial->emissive);
                }
                m_DataIt = skipLine<DataArrayIt>(m_DataIt, m_DataItEnd, m_uiLine);
            } break;
            case 'T': {
                ++m_DataIt;
                // Material transmission color
                if (*m_DataIt == 'f')  {
                    ++m_DataIt;
                    if (m_pModel->mCurrentMaterial != nullptr)
                        getColorRGBA(&m_pModel->mCurrentMaterial->transparent);
                } else if (*m_DataIt == 'r')  {
                    // Material transmission alpha value
                    ++m_DataIt;
                    ai_real d;
                    getFloatValue(d);
                    if (m_pModel->mCurrentMaterial != nullptr)
                        m_pModel->mCurrentMaterial->alpha = static_cast<ai_real>(1.0) - d;
                }
                m_DataIt = skipLine<DataArrayIt>(m_DataIt, m_DataItEnd, m_uiLine);
            } break;
            case 'd': {
                if (*(m_DataIt + 1) == 'i' && *(m_DataIt + 2) == 's' && *(m_DataIt + 3) == 'p') {
                    // A displacement map
                    getTexture();
                } else {
                    // Alpha value
                    ++m_DataIt;
                    if (m_pModel->mCurrentMaterial != nullptr)
                        getFloatValue(m_pModel->mCurrentMaterial->alpha);
                    m_DataIt = skipLine<DataArrayIt>(m_DataIt, m_DataItEnd, m_uiLine);
                }
            } break;

            case 'N':
            case 'n': {
                ++m_DataIt;
                switch (*m_DataIt) {
                    case 's': // Specular exponent
                        ++m_DataIt;
                        if (m_pModel->mCurrentMaterial != nullptr)
                            getFloatValue(m_pModel->mCurrentMaterial->shineness);
                        break;
                    case 'i': // Index Of refraction
                        ++m_DataIt;
                        if (m_pModel->mCurrentMaterial != nullptr)
                            getFloatValue(m_pModel->mCurrentMaterial->ior);
                        break;
                    case 'e': // New material
                        createMaterial();
                        break;
                    case 'o': // Norm texture
                        --m_DataIt;
                        getTexture();
                        break;
                }
                m_DataIt = skipLine<DataArrayIt>(m_DataIt, m_DataItEnd, m_uiLine);
            } break;

            case 'P':
                {
                    ++m_DataIt;
                    switch(*m_DataIt)
                    {
                    case 'r':
                        ++m_DataIt;
                        if (m_pModel->mCurrentMaterial != nullptr)
                            getFloatValue(m_pModel->mCurrentMaterial->roughness);
                        break;
                    case 'm':
                        ++m_DataIt;
                        if (m_pModel->mCurrentMaterial != nullptr)
                            getFloatValue(m_pModel->mCurrentMaterial->metallic);
                        break;
                    case 's':
                        ++m_DataIt;
                        if (m_pModel->mCurrentMaterial != nullptr)
                            getColorRGBA(m_pModel->mCurrentMaterial->sheen);
                        break;
                    case 'c':
                        ++m_DataIt;
                        if (*m_DataIt == 'r') {
                            ++m_DataIt;
                            if (m_pModel->mCurrentMaterial != nullptr)
                                getFloatValue(m_pModel->mCurrentMaterial->clearcoat_roughness);
                        } else {
                            if (m_pModel->mCurrentMaterial != nullptr)
                                getFloatValue(m_pModel->mCurrentMaterial->clearcoat_thickness);
                        }
                        break;
                    }
                    m_DataIt = skipLine<DataArrayIt>(m_DataIt, m_DataItEnd, m_uiLine);
                }
                break;

            case 'm': // Texture
            case 'b': // quick'n'dirty - for 'bump' sections
            case 'r': // quick'n'dirty - for 'refl' sections
            {
                getTexture();
                m_DataIt = skipLine<DataArrayIt>(m_DataIt, m_DataItEnd, m_uiLine);
            } break;

            case 'i': // Illumination model
            {
                m_DataIt = getNextToken<DataArrayIt>(m_DataIt, m_DataItEnd);
                if (m_pModel->mCurrentMaterial != nullptr)
                    getIlluminationModel(m_pModel->mCurrentMaterial->illumination_model);
                m_DataIt = skipLine<DataArrayIt>(m_DataIt, m_DataItEnd, m_uiLine);
            } break;

            case 'a': // Anisotropy
            {
                ++m_DataIt;
                getFloatValue(m_pModel->mCurrentMaterial->anisotropy);
                if (m_pModel->mCurrentMaterial != nullptr)
                    m_DataIt = skipLine<DataArrayIt>(m_DataIt, m_DataItEnd, m_uiLine);
            } break;

            default: {
                m_DataIt = skipLine<DataArrayIt>(m_DataIt, m_DataItEnd, m_uiLine);
            } break;
        }
    }
}

// -------------------------------------------------------------------
//  Loads a color definition
void ObjFileMtlImporter::getColorRGBA(aiColor3D *pColor) {
    ai_assert(nullptr != pColor);

    ai_real r(0.0), g(0.0), b(0.0);
    m_DataIt = getFloat<DataArrayIt>(m_DataIt, m_DataItEnd, r);
    pColor->r = r;

    // we have to check if color is default 0 with only one token
    if (!IsLineEnd(*m_DataIt)) {
        m_DataIt = getFloat<DataArrayIt>(m_DataIt, m_DataItEnd, g);
        m_DataIt = getFloat<DataArrayIt>(m_DataIt, m_DataItEnd, b);
    }
    pColor->g = g;
    pColor->b = b;
}

// -------------------------------------------------------------------
void ObjFileMtlImporter::getColorRGBA(Maybe<aiColor3D> &value) {
    aiColor3D v;
    getColorRGBA(&v);
    value = Maybe<aiColor3D>(v);
}

// -------------------------------------------------------------------
//  Loads the kind of illumination model.
void ObjFileMtlImporter::getIlluminationModel(int &illum_model) {
    m_DataIt = CopyNextWord<DataArrayIt>(m_DataIt, m_DataItEnd, &m_buffer[0], BUFFERSIZE);
    illum_model = atoi(&m_buffer[0]);
}


// -------------------------------------------------------------------
//  Loads a single float value.
void ObjFileMtlImporter::getFloatValue(ai_real &value) {
    m_DataIt = CopyNextWord<DataArrayIt>(m_DataIt, m_DataItEnd, &m_buffer[0], BUFFERSIZE);
    size_t len = std::strlen(&m_buffer[0]);
    if (0 == len) {
        value = 0.0f;
        return;
    }

    value = (ai_real)fast_atof(&m_buffer[0]);
}

// -------------------------------------------------------------------
void ObjFileMtlImporter::getFloatValue(Maybe<ai_real> &value) {
    m_DataIt = CopyNextWord<DataArrayIt>(m_DataIt, m_DataItEnd, &m_buffer[0], BUFFERSIZE);
    size_t len = std::strlen(&m_buffer[0]);
    if (len)
        value = Maybe<ai_real>(fast_atof(&m_buffer[0]));
    else
        value = Maybe<ai_real>();
}

// -------------------------------------------------------------------
//  Creates a material from loaded data.
void ObjFileMtlImporter::createMaterial() {
    std::string line;
    while (!IsLineEnd(*m_DataIt)) {
        line += *m_DataIt;
        ++m_DataIt;
    }

    std::vector<std::string> token;
    const unsigned int numToken = tokenize<std::string>(line, token, " \t");
    std::string name;
    if (numToken == 1) {
        name = AI_DEFAULT_MATERIAL_NAME;
    } else {
        // skip newmtl and all following white spaces
        std::size_t first_ws_pos = line.find_first_of(" \t");
        std::size_t first_non_ws_pos = line.find_first_not_of(" \t", first_ws_pos);
        if (first_non_ws_pos != std::string::npos) {
            name = line.substr(first_non_ws_pos);
        }
    }

    name = trim_whitespaces(name);

    std::map<std::string, ObjFile::Material *>::iterator it = m_pModel->mMaterialMap.find(name);
    if (m_pModel->mMaterialMap.end() == it) {
        // New Material created
        m_pModel->mCurrentMaterial = new ObjFile::Material();
        m_pModel->mCurrentMaterial->MaterialName.Set(name);
        m_pModel->mMaterialLib.push_back(name);
        m_pModel->mMaterialMap[name] = m_pModel->mCurrentMaterial;

        if (m_pModel->mCurrentMesh) {
            m_pModel->mCurrentMesh->m_uiMaterialIndex = static_cast<unsigned int>(m_pModel->mMaterialLib.size() - 1);
        }
    } else {
        // Use older material
        m_pModel->mCurrentMaterial = it->second;
    }
}

// -------------------------------------------------------------------
//  Gets a texture name from data.
void ObjFileMtlImporter::getTexture() {
    aiString *out = nullptr;
    int clampIndex = -1;

    if (m_pModel->mCurrentMaterial == nullptr) {
        m_pModel->mCurrentMaterial = new ObjFile::Material();
        m_pModel->mCurrentMaterial->MaterialName.Set("Empty_Material");
    }

    const char *pPtr(&(*m_DataIt));
    if (!ASSIMP_strincmp(pPtr, DiffuseTexture, static_cast<unsigned int>(strlen(DiffuseTexture)))) {
        // Diffuse texture
        out = &m_pModel->mCurrentMaterial->texture;
        clampIndex = ObjFile::Material::TextureDiffuseType;
    } else if (!ASSIMP_strincmp(pPtr, AmbientTexture, static_cast<unsigned int>(strlen(AmbientTexture)))) {
        // Ambient texture
        out = &m_pModel->mCurrentMaterial->textureAmbient;
        clampIndex = ObjFile::Material::TextureAmbientType;
    } else if (!ASSIMP_strincmp(pPtr, SpecularTexture, static_cast<unsigned int>(strlen(SpecularTexture)))) {
        // Specular texture
        out = &m_pModel->mCurrentMaterial->textureSpecular;
        clampIndex = ObjFile::Material::TextureSpecularType;
    } else if (!ASSIMP_strincmp(pPtr, DisplacementTexture1, static_cast<unsigned int>(strlen(DisplacementTexture1))) ||
               !ASSIMP_strincmp(pPtr, DisplacementTexture2, static_cast<unsigned int>(strlen(DisplacementTexture2)))) {
        // Displacement texture
        out = &m_pModel->mCurrentMaterial->textureDisp;
        clampIndex = ObjFile::Material::TextureDispType;
    } else if (!ASSIMP_strincmp(pPtr, OpacityTexture, static_cast<unsigned int>(strlen(OpacityTexture)))) {
        // Opacity texture
        out = &m_pModel->mCurrentMaterial->textureOpacity;
        clampIndex = ObjFile::Material::TextureOpacityType;
    } else if (!ASSIMP_strincmp(pPtr, EmissiveTexture1, static_cast<unsigned int>(strlen(EmissiveTexture1))) ||
               !ASSIMP_strincmp(pPtr, EmissiveTexture2, static_cast<unsigned int>(strlen(EmissiveTexture2)))) {
        // Emissive texture
        out = &m_pModel->mCurrentMaterial->textureEmissive;
        clampIndex = ObjFile::Material::TextureEmissiveType;
    } else if (!ASSIMP_strincmp(pPtr, BumpTexture1, static_cast<unsigned int>(strlen(BumpTexture1))) ||
               !ASSIMP_strincmp(pPtr, BumpTexture2, static_cast<unsigned int>(strlen(BumpTexture2)))) {
        // Bump texture
        out = &m_pModel->mCurrentMaterial->textureBump;
        clampIndex = ObjFile::Material::TextureBumpType;
    } else if (!ASSIMP_strincmp(pPtr, NormalTextureV1, static_cast<unsigned int>(strlen(NormalTextureV1))) || !ASSIMP_strincmp(pPtr, NormalTextureV2, static_cast<unsigned int>(strlen(NormalTextureV2)))) {
        // Normal map
        out = &m_pModel->mCurrentMaterial->textureNormal;
        clampIndex = ObjFile::Material::TextureNormalType;
    } else if (!ASSIMP_strincmp(pPtr, ReflectionTexture, static_cast<unsigned int>(strlen(ReflectionTexture)))) {
        // Reflection texture(s)
        //Do nothing here
        return;
    } else if (!ASSIMP_strincmp(pPtr, SpecularityTexture, static_cast<unsigned int>(strlen(SpecularityTexture)))) {
        // Specularity scaling (glossiness)
        out = &m_pModel->mCurrentMaterial->textureSpecularity;
        clampIndex = ObjFile::Material::TextureSpecularityType;
    } else if ( !ASSIMP_strincmp( pPtr, RoughnessTexture, static_cast<unsigned int>(strlen(RoughnessTexture)))) {
        // PBR Roughness texture
        out = & m_pModel->mCurrentMaterial->textureRoughness;
        clampIndex = ObjFile::Material::TextureRoughnessType;
    } else if ( !ASSIMP_strincmp( pPtr, MetallicTexture, static_cast<unsigned int>(strlen(MetallicTexture)))) {
        // PBR Metallic texture
        out = & m_pModel->mCurrentMaterial->textureMetallic;
        clampIndex = ObjFile::Material::TextureMetallicType;
    } else if (!ASSIMP_strincmp( pPtr, SheenTexture, static_cast<unsigned int>(strlen(SheenTexture)))) {
        // PBR Sheen (reflectance) texture
        out = & m_pModel->mCurrentMaterial->textureSheen;
        clampIndex = ObjFile::Material::TextureSheenType;
    } else if (!ASSIMP_strincmp( pPtr, RMATexture, static_cast<unsigned int>(strlen(RMATexture)))) {
        // PBR Rough/Metal/AO texture
        out = & m_pModel->mCurrentMaterial->textureRMA;
        clampIndex = ObjFile::Material::TextureRMAType;
    } else {
        ASSIMP_LOG_ERROR("OBJ/MTL: Encountered unknown texture type");
        return;
    }

    bool clamp = false;
    getTextureOption(clamp, clampIndex, out);
    m_pModel->mCurrentMaterial->clamp[clampIndex] = clamp;

    std::string texture;
    m_DataIt = getName<DataArrayIt>(m_DataIt, m_DataItEnd, texture);
    if (nullptr != out) {
        out->Set(texture);
    }
}

/* /////////////////////////////////////////////////////////////////////////////
 * Texture Option
 * /////////////////////////////////////////////////////////////////////////////
 * According to http://en.wikipedia.org/wiki/Wavefront_.obj_file#Texture_options
 * Texture map statement can contains various texture option, for example:
 *
 *  map_Ka -o 1 1 1 some.png
 *  map_Kd -clamp on some.png
 *
 * So we need to parse and skip these options, and leave the last part which is
 * the url of image, otherwise we will get a wrong url like "-clamp on some.png".
 *
 * Because aiMaterial supports clamp option, so we also want to return it
 * /////////////////////////////////////////////////////////////////////////////
 */
void ObjFileMtlImporter::getTextureOption(bool &clamp, int &clampIndex, aiString *&out) {
    m_DataIt = getNextToken<DataArrayIt>(m_DataIt, m_DataItEnd);

    // If there is any more texture option
    while (!isEndOfBuffer(m_DataIt, m_DataItEnd) && *m_DataIt == '-') {
        const char *pPtr(&(*m_DataIt));
        //skip option key and value
        int skipToken = 1;

        if (!ASSIMP_strincmp(pPtr, ClampOption, static_cast<unsigned int>(strlen(ClampOption)))) {
            DataArrayIt it = getNextToken<DataArrayIt>(m_DataIt, m_DataItEnd);
            char value[3];
            CopyNextWord(it, m_DataItEnd, value, sizeof(value) / sizeof(*value));
            if (!ASSIMP_strincmp(value, "on", 2)) {
                clamp = true;
            }

            skipToken = 2;
        } else if (!ASSIMP_strincmp(pPtr, TypeOption, static_cast<unsigned int>(strlen(TypeOption)))) {
            DataArrayIt it = getNextToken<DataArrayIt>(m_DataIt, m_DataItEnd);
            char value[12];
            CopyNextWord(it, m_DataItEnd, value, sizeof(value) / sizeof(*value));
            if (!ASSIMP_strincmp(value, "cube_top", 8)) {
                clampIndex = ObjFile::Material::TextureReflectionCubeTopType;
                out = &m_pModel->mCurrentMaterial->textureReflection[0];
            } else if (!ASSIMP_strincmp(value, "cube_bottom", 11)) {
                clampIndex = ObjFile::Material::TextureReflectionCubeBottomType;
                out = &m_pModel->mCurrentMaterial->textureReflection[1];
            } else if (!ASSIMP_strincmp(value, "cube_front", 10)) {
                clampIndex = ObjFile::Material::TextureReflectionCubeFrontType;
                out = &m_pModel->mCurrentMaterial->textureReflection[2];
            } else if (!ASSIMP_strincmp(value, "cube_back", 9)) {
                clampIndex = ObjFile::Material::TextureReflectionCubeBackType;
                out = &m_pModel->mCurrentMaterial->textureReflection[3];
            } else if (!ASSIMP_strincmp(value, "cube_left", 9)) {
                clampIndex = ObjFile::Material::TextureReflectionCubeLeftType;
                out = &m_pModel->mCurrentMaterial->textureReflection[4];
            } else if (!ASSIMP_strincmp(value, "cube_right", 10)) {
                clampIndex = ObjFile::Material::TextureReflectionCubeRightType;
                out = &m_pModel->mCurrentMaterial->textureReflection[5];
            } else if (!ASSIMP_strincmp(value, "sphere", 6)) {
                clampIndex = ObjFile::Material::TextureReflectionSphereType;
                out = &m_pModel->mCurrentMaterial->textureReflection[0];
            }

            skipToken = 2;
        } else if (!ASSIMP_strincmp(pPtr, BumpOption, static_cast<unsigned int>(strlen(BumpOption)))) {
            DataArrayIt it = getNextToken<DataArrayIt>(m_DataIt, m_DataItEnd);
            getFloat(it, m_DataItEnd, m_pModel->mCurrentMaterial->bump_multiplier);
            skipToken = 2;
        } else if (!ASSIMP_strincmp(pPtr, BlendUOption, static_cast<unsigned int>(strlen(BlendUOption))) ||
                !ASSIMP_strincmp(pPtr, BlendVOption, static_cast<unsigned int>(strlen(BlendVOption))) ||
                !ASSIMP_strincmp(pPtr, BoostOption, static_cast<unsigned int>(strlen(BoostOption))) ||
                !ASSIMP_strincmp(pPtr, ResolutionOption, static_cast<unsigned int>(strlen(ResolutionOption))) ||
                !ASSIMP_strincmp(pPtr, ChannelOption, static_cast<unsigned int>(strlen(ChannelOption)))) {
            skipToken = 2;
        } else if (!ASSIMP_strincmp(pPtr, ModifyMapOption, static_cast<unsigned int>(strlen(ModifyMapOption)))) {
            skipToken = 3;
        } else if (!ASSIMP_strincmp(pPtr, OffsetOption, static_cast<unsigned int>(strlen(OffsetOption))) ||
                !ASSIMP_strincmp(pPtr, ScaleOption, static_cast<unsigned int>(strlen(ScaleOption))) ||
                !ASSIMP_strincmp(pPtr, TurbulenceOption, static_cast<unsigned int>(strlen(TurbulenceOption)))) {
            skipToken = 4;
        }

        for (int i = 0; i < skipToken; ++i) {
            m_DataIt = getNextToken<DataArrayIt>(m_DataIt, m_DataItEnd);
        }
    }
}

// -------------------------------------------------------------------

} // Namespace Assimp

#endif // !! ASSIMP_BUILD_NO_OBJ_IMPORTER
