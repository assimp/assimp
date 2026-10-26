/*
---------------------------------------------------------------------------
Open Asset Import Library (assimp)
---------------------------------------------------------------------------

Copyright (c) 2006-2026, assimp team

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
#include <assimp/types.h>
#include <assimp/DefaultIOSystem.h>
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
ObjFileMtlImporter::ObjFileMtlImporter(std::vector<char> &buffer, const std::string &strAbsPath, ObjFile::Model *pModel) :
        mAbsPath(strAbsPath),
        mDataIt(buffer.begin()),
        mDataItEnd(buffer.end()),
        mModel(pModel) {
    ai_assert(nullptr != mModel);
    mBuffer.resize(BUFFERSIZE);
    std::fill(mBuffer.begin(), mBuffer.end(), '\0');
    if (nullptr == mModel->mDefaultMaterial) {
        mModel->mDefaultMaterial = new ObjFile::Material;
        mModel->mDefaultMaterial->MaterialName.Set("default");
    }

    // Try with OS folder separator first
    char folderSeparator = DefaultIOSystem().getOsSeparator();
    std::size_t found = mAbsPath.find_last_of(folderSeparator);
    if (found == std::string::npos) {
        // Not found, try alternative folder separator
        folderSeparator = (folderSeparator == '/' ? '\\' : '/');
        found = mAbsPath.find_last_of(folderSeparator);
    }
    if (found != std::string::npos) {
        mAbsPath = mAbsPath.substr(0, found + 1);
    } else {
        mAbsPath = "";
    }
    load();
}

// -------------------------------------------------------------------
//  Loads the material description
void ObjFileMtlImporter::load() {
    if (mDataIt == mDataItEnd) {
        return;
    }

    while (mDataIt != mDataItEnd) {
        switch (*mDataIt) {
            case 'k':
            case 'K': {
                ++mDataIt;
                if (*mDataIt == 'a') { // Ambient color
                    ++mDataIt;
                    if (mModel->mCurrentMaterial != nullptr) {
                        getColorRGBA(&mModel->mCurrentMaterial->ambient);
                    }
                } else if (*mDataIt == 'd') {
                    // Diffuse color
                    ++mDataIt;
                    if (mModel->mCurrentMaterial != nullptr) {
                        getColorRGBA(&mModel->mCurrentMaterial->diffuse);
                    }
                } else if (*mDataIt == 's') {
                    ++mDataIt;
                    if (mModel->mCurrentMaterial != nullptr) {
                        getColorRGBA(&mModel->mCurrentMaterial->specular);
                    }
                } else if (*mDataIt == 'e') {
                    ++mDataIt;
                    if (mModel->mCurrentMaterial != nullptr) {
                        getColorRGBA(&mModel->mCurrentMaterial->emissive);
                    }
                }
                mDataIt = skipLine<DataArrayIt>(mDataIt, mDataItEnd, mLine);
            } break;
            case 'T': {
                ++mDataIt;
                // Material transmission color
                if (*mDataIt == 'f')  {
                    ++mDataIt;
                    if (mModel->mCurrentMaterial != nullptr) {
                        getColorRGBA(&mModel->mCurrentMaterial->transparent);
                    }
                } else if (*mDataIt == 'r')  {
                    // Material transmission alpha value
                    ++mDataIt;
                    ai_real d;
                    getFloatValue(d);
                    if (mModel->mCurrentMaterial != nullptr) {
                        mModel->mCurrentMaterial->alpha = static_cast<ai_real>(1.0) - d;
                    }
                }
                mDataIt = skipLine<DataArrayIt>(mDataIt, mDataItEnd, mLine);
            } break;
            case 'd': {
                if (*(mDataIt + 1) == 'i' && *(mDataIt + 2) == 's' && *(mDataIt + 3) == 'p') {
                    // A displacement map
                    getTexture();
                } else {
                    // Alpha value
                    ++mDataIt;
                    if (mModel->mCurrentMaterial != nullptr) {
                        getFloatValue(mModel->mCurrentMaterial->alpha);
                    }
                    mDataIt = skipLine<DataArrayIt>(mDataIt, mDataItEnd, mLine);
                }
            } break;

            case 'N':
            case 'n': {
                ++mDataIt;
                switch (*mDataIt) {
                    case 's': // Specular exponent
                        ++mDataIt;
                        if (mModel->mCurrentMaterial != nullptr) {
                            getFloatValue(mModel->mCurrentMaterial->shineness);
                        }
                        break;
                    case 'i': // Index Of refraction
                        ++mDataIt;
                        if (mModel->mCurrentMaterial != nullptr) {
                            getFloatValue(mModel->mCurrentMaterial->ior);
                        }
                        break;
                    case 'e': // New material
                        createMaterial();
                        break;
                    case 'o': // Norm texture
                        --mDataIt;
                        getTexture();
                        break;
                }
                mDataIt = skipLine<DataArrayIt>(mDataIt, mDataItEnd, mLine);
            } break;

            case 'P':
                {
                    ++mDataIt;
                    switch(*mDataIt) {
                    case 'r':
                        ++mDataIt;
                        if (mModel->mCurrentMaterial != nullptr) {
                            getFloatValue(mModel->mCurrentMaterial->roughness);
                        }
                        break;
                    case 'm':
                        ++mDataIt;
                        if (mModel->mCurrentMaterial != nullptr) {
                            getFloatValue(mModel->mCurrentMaterial->metallic);
                        }
                        break;
                    case 's':
                        ++mDataIt;
                        if (mModel->mCurrentMaterial != nullptr) {
                            getColorRGBA(mModel->mCurrentMaterial->sheen);
                        }
                        break;
                    case 'c':
                        ++mDataIt;
                        if (*mDataIt == 'r') {
                            ++mDataIt;
                            if (mModel->mCurrentMaterial != nullptr) {
                                getFloatValue(mModel->mCurrentMaterial->clearcoat_roughness);
                            }
                        } else if (*mDataIt == 't') {
                            ++mDataIt;
                            if (mModel->mCurrentMaterial != nullptr) {
                                getFloatValue(mModel->mCurrentMaterial->clearcoat_thickness);
                            }
                        } else {
                            if (mModel->mCurrentMaterial != nullptr) {
                                getFloatValue(mModel->mCurrentMaterial->clearcoat);
                            }
                        }
                        break;
                    }
                    mDataIt = skipLine<DataArrayIt>(mDataIt, mDataItEnd, mLine);
                }
                break;
            
            case 'm': // Texture or metallic
            {
                // Save start of token (after 'm')
                auto tokenStart = mDataIt;  // points to 'm'
                auto tokenEnd = getNextDelimiter(mDataIt, mDataItEnd); // move iterator to end of token

                std::string_view svKeyword(&(*tokenStart), static_cast<size_t>(&(*tokenEnd) - &(*tokenStart)));
                mDataIt = getNextWord(tokenEnd, mDataItEnd); // advance iterator

                if (svKeyword.size() >= 3 && svKeyword.compare(0, 3, "map") == 0) {
                    // starts with "map", treat as texture map
                    mDataIt = tokenStart;
                    getTexture();
                } else if (svKeyword == "metallic" || svKeyword == "metal" || svKeyword == "metalness") {
                    // parse metallic float value instead of texture
                    getFloatIfMaterialValid(&ObjFile::Material::metallic);
                }

                mDataIt = skipLine<DataArrayIt>(mDataIt, mDataItEnd, mLine);
            } break;

            case 'b': // quick'n'dirty - for 'bump' sections
            {
                getTexture();
                mDataIt = skipLine<DataArrayIt>(mDataIt, mDataItEnd, mLine);
            } break;

            case 'r': // refl (map) or roughness (float)
            {
                auto tokenStart = mDataIt;  // points to 'r'
                auto tokenEnd = getNextDelimiter(mDataIt, mDataItEnd);
                std::string_view svKeyword(&(*tokenStart), static_cast<size_t>(&(*tokenEnd) - &(*tokenStart)));
                mDataIt = getNextWord(tokenEnd, mDataItEnd);

                if (svKeyword == "roughness" || svKeyword == "rough") {
                    getFloatIfMaterialValid(&ObjFile::Material::roughness);
                } else if (svKeyword == "refl" || svKeyword == "reflection") {
                    mDataIt = tokenStart;
                    getTexture();
                }

                mDataIt = skipLine<DataArrayIt>(mDataIt, mDataItEnd, mLine);
            } break;

            case 'i': // Illumination model
            {
                mDataIt = getNextToken<DataArrayIt>(mDataIt, mDataItEnd);
                if (mModel->mCurrentMaterial != nullptr) {
                    getIlluminationModel(mModel->mCurrentMaterial->illumination_model);
                }
                mDataIt = skipLine<DataArrayIt>(mDataIt, mDataItEnd, mLine);
            } break;

            case 'a': {
                auto tokenStart = mDataIt;
                auto tokenEnd = getNextDelimiter(mDataIt, mDataItEnd);
                std::string_view svKeyword(&(*tokenStart), static_cast<size_t>(&(*tokenEnd) - &(*tokenStart)));
                mDataIt = getNextWord(tokenEnd, mDataItEnd);

                if (svKeyword == "aniso" || svKeyword == "anisotropy") {
                    getFloatIfMaterialValid(&ObjFile::Material::anisotropy);
                } else if (svKeyword == "ao") {
                    getFloatIfMaterialValid(&ObjFile::Material::ambient_occlusion);
                } else if (svKeyword == "anisor" || svKeyword == "anisotropicrotation") {
                    getFloatIfMaterialValid(&ObjFile::Material::anisotropy_rotation);
                } else {
                    ASSIMP_LOG_WARN("Unhandled keyword: ", svKeyword);
                }

                mDataIt = skipLine<DataArrayIt>(mDataIt, mDataItEnd, mLine);
            } break;

            case 's': {
                auto tokenStart = mDataIt;
                auto tokenEnd = getNextDelimiter(mDataIt, mDataItEnd);
                std::string_view svKeyword(&(*tokenStart), static_cast<size_t>(&(*tokenEnd) - &(*tokenStart)));
                mDataIt = getNextWord(tokenEnd, mDataItEnd);

                if (svKeyword == "subsurface" || svKeyword == "scattering") {
                    getFloatIfMaterialValid(&ObjFile::Material::subsurface_scattering);
                } else if (svKeyword == "speculartint") {
                    getFloatIfMaterialValid(&ObjFile::Material::specular_tint);
                } else if (svKeyword == "sheen") {
                    getFloatIfMaterialValid(&ObjFile::Material::sheen_grazing);
                } else if (svKeyword == "sheentint") {
                    getFloatIfMaterialValid(&ObjFile::Material::sheen_tint);
                } else {
                    ASSIMP_LOG_WARN("Unhandled keyword: ", svKeyword);
                }

                mDataIt = skipLine<DataArrayIt>(mDataIt, mDataItEnd, mLine);
            } break;

            case 'c': {
                auto tokenStart = mDataIt;
                auto tokenEnd = getNextDelimiter(mDataIt, mDataItEnd);
                std::string_view svKeyword(&(*tokenStart), static_cast<size_t>(&(*tokenEnd) - &(*tokenStart)));
                mDataIt = getNextWord(tokenEnd, mDataItEnd);

                if (svKeyword == "clearcoat") {
                    getFloatIfMaterialValid(&ObjFile::Material::clearcoat);
                } else if (svKeyword == "clearcoatgloss") {
                    getFloatIfMaterialValid(&ObjFile::Material::clearcoat_gloss);
                } else {
                    ASSIMP_LOG_WARN("Unhandled keyword: ", svKeyword);
                }

                mDataIt = skipLine<DataArrayIt>(mDataIt, mDataItEnd, mLine);
            } break;

            default: {
                mDataIt = skipLine<DataArrayIt>(mDataIt, mDataItEnd, mLine);
            } break;
        }
    }
}

// -------------------------------------------------------------------
//  Loads a color definition
void ObjFileMtlImporter::getColorRGBA(aiColor3D *pColor) {
    ai_assert(nullptr != pColor);

    ai_real r(0.0), g(0.0), b(0.0);
    mDataIt = getFloat<DataArrayIt>(mDataIt, mDataItEnd, r);
    pColor->r = r;

    // we have to check if color is default 0 with only one token
    if (!IsLineEnd(*mDataIt)) {
        mDataIt = getFloat<DataArrayIt>(mDataIt, mDataItEnd, g);
        mDataIt = getFloat<DataArrayIt>(mDataIt, mDataItEnd, b);
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
    mDataIt = CopyNextWord<DataArrayIt>(mDataIt, mDataItEnd, &mBuffer[0], BUFFERSIZE);
    illum_model = atoi(&mBuffer[0]);
}

// -------------------------------------------------------------------
//  Loads a single float value.
void ObjFileMtlImporter::getFloatValue(ai_real &value) {
    mDataIt = CopyNextWord<DataArrayIt>(mDataIt, mDataItEnd, &mBuffer[0], BUFFERSIZE);
    const size_t len = std::strlen(&mBuffer[0]);
    if (0 == len) {
        value = 0.0f;
        return;
    }

    value = fast_atof(&mBuffer[0]);
}

// -------------------------------------------------------------------
void ObjFileMtlImporter::getFloatValue(Maybe<ai_real> &value) {
    mDataIt = CopyNextWord<DataArrayIt>(mDataIt, mDataItEnd, &mBuffer[0], BUFFERSIZE);
    const size_t len = std::strlen(&mBuffer[0]);
    if (len) {
        value = Maybe<ai_real>(fast_atof(&mBuffer[0]));
    } else {
        value = Maybe<ai_real>();
    }
}

// -------------------------------------------------------------------
//  Writes a loaded single float value if material not null
void ObjFileMtlImporter::getFloatIfMaterialValid(ai_real ObjFile::Material::*member) {
    if (mModel != nullptr && mModel->mCurrentMaterial != nullptr) {
        // This will call getFloatValue(ai_real&)
        getFloatValue(mModel->mCurrentMaterial->*member);
    }
}

// -------------------------------------------------------------------
void ObjFileMtlImporter::getFloatIfMaterialValid(Maybe<ai_real> ObjFile::Material::*member) {
    // It can directly access `m_pModel` because it's part of the class
    if (mModel != nullptr && mModel->mCurrentMaterial != nullptr) {
        getFloatValue(mModel->mCurrentMaterial->*member);
    }
}

// -------------------------------------------------------------------
//  Creates a material from loaded data.
void ObjFileMtlImporter::createMaterial() {
    // Parse the line directly without creating a temporary string
    auto lineStart = mDataIt;
    while (!IsLineEnd(*mDataIt)) {
        ++mDataIt;
    }
    
    // Get the line as string_view
    std::string_view svLine(&(*lineStart), static_cast<size_t>(&*mDataIt - &*lineStart));
    svLine = trimStringView(svLine);
    
    std::string name;
    if (svLine.empty() || svLine == "newmtl") {
        name = AI_DEFAULT_MATERIAL_NAME;
    } else {
        // skip newmtl and all following white spaces
        size_t first_ws_pos = svLine.find_first_of(" \t");
        size_t first_non_ws_pos = std::string_view::npos;
        if (first_ws_pos != std::string_view::npos) {
            first_non_ws_pos = svLine.find_first_not_of(" \t", first_ws_pos);
        }
        if (first_non_ws_pos != std::string_view::npos) {
            name = std::string(svLine.substr(first_non_ws_pos));
        } else {
            name = AI_DEFAULT_MATERIAL_NAME;
        }
    }

    name = ai_trim(name);

    std::map<std::string, ObjFile::Material *>::iterator it = mModel->mMaterialMap.find(name);
    if (mModel->mMaterialMap.end() == it) {
        // New Material created
        mModel->mCurrentMaterial = new ObjFile::Material();
        mModel->mCurrentMaterial->MaterialName.Set(name);
        mModel->mMaterialLib.push_back(name);
        mModel->mMaterialMap[name] = mModel->mCurrentMaterial;

        if (mModel->mCurrentMesh) {
            mModel->mCurrentMesh->mMaterialIndex = static_cast<unsigned int>(mModel->mMaterialLib.size() - 1);
        }
    } else {
        // Use older material
        mModel->mCurrentMaterial = it->second;
    }
}

// -------------------------------------------------------------------
//  Gets a texture name from data.
void ObjFileMtlImporter::getTexture() {
    aiString *out = nullptr;
    int clampIndex = -1;

    if (mModel->mCurrentMaterial == nullptr) {
        mModel->mCurrentMaterial = new ObjFile::Material();
        mModel->mCurrentMaterial->MaterialName.Set("Empty_Material");
        mModel->mMaterialMap["Empty_Material"] = mModel->mCurrentMaterial;
    }

    if (const char *pPtr(&*mDataIt); !ASSIMP_strincmp(pPtr, DiffuseTexture, static_cast<unsigned int>(strlen(DiffuseTexture)))) {
        // Diffuse texture
        out = &mModel->mCurrentMaterial->texture;
        clampIndex = ObjFile::Material::TextureDiffuseType;
    } else if (!ASSIMP_strincmp(pPtr, AmbientTexture, static_cast<unsigned int>(strlen(AmbientTexture)))) {
        // Ambient texture
        out = &mModel->mCurrentMaterial->textureAmbient;
        clampIndex = ObjFile::Material::TextureAmbientType;
    } else if (!ASSIMP_strincmp(pPtr, SpecularTexture, static_cast<unsigned int>(strlen(SpecularTexture)))) {
        // Specular texture
        out = &mModel->mCurrentMaterial->textureSpecular;
        clampIndex = ObjFile::Material::TextureSpecularType;
    } else if (!ASSIMP_strincmp(pPtr, DisplacementTexture1, static_cast<unsigned int>(strlen(DisplacementTexture1))) ||
               !ASSIMP_strincmp(pPtr, DisplacementTexture2, static_cast<unsigned int>(strlen(DisplacementTexture2)))) {
        // Displacement texture
        out = &mModel->mCurrentMaterial->textureDisp;
        clampIndex = ObjFile::Material::TextureDispType;
    } else if (!ASSIMP_strincmp(pPtr, OpacityTexture, static_cast<unsigned int>(strlen(OpacityTexture)))) {
        // Opacity texture
        out = &mModel->mCurrentMaterial->textureOpacity;
        clampIndex = ObjFile::Material::TextureOpacityType;
    } else if (!ASSIMP_strincmp(pPtr, EmissiveTexture1, static_cast<unsigned int>(strlen(EmissiveTexture1))) ||
               !ASSIMP_strincmp(pPtr, EmissiveTexture2, static_cast<unsigned int>(strlen(EmissiveTexture2)))) {
        // Emissive texture
        out = &mModel->mCurrentMaterial->textureEmissive;
        clampIndex = ObjFile::Material::TextureEmissiveType;
    } else if (!ASSIMP_strincmp(pPtr, BumpTexture1, static_cast<unsigned int>(strlen(BumpTexture1))) ||
               !ASSIMP_strincmp(pPtr, BumpTexture2, static_cast<unsigned int>(strlen(BumpTexture2)))) {
        // Bump texture
        out = &mModel->mCurrentMaterial->textureBump;
        clampIndex = ObjFile::Material::TextureBumpType;
    } else if (!ASSIMP_strincmp(pPtr, NormalTextureV1, static_cast<unsigned int>(strlen(NormalTextureV1))) || !ASSIMP_strincmp(pPtr, NormalTextureV2, static_cast<unsigned int>(strlen(NormalTextureV2)))) {
        // Normal map
        out = &mModel->mCurrentMaterial->textureNormal;
        clampIndex = ObjFile::Material::TextureNormalType;
    } else if (!ASSIMP_strincmp(pPtr, ReflectionTexture, static_cast<unsigned int>(strlen(ReflectionTexture)))) {
        // Reflection texture(s)
        //Do nothing here
        return;
    } else if (!ASSIMP_strincmp(pPtr, SpecularityTexture, static_cast<unsigned int>(strlen(SpecularityTexture)))) {
        // Specularity scaling (glossiness)
        out = &mModel->mCurrentMaterial->textureSpecularity;
        clampIndex = ObjFile::Material::TextureSpecularityType;
    } else if ( !ASSIMP_strincmp( pPtr, RoughnessTexture, static_cast<unsigned int>(strlen(RoughnessTexture)))) {
        // PBR Roughness texture
        out = & mModel->mCurrentMaterial->textureRoughness;
        clampIndex = ObjFile::Material::TextureRoughnessType;
    } else if ( !ASSIMP_strincmp( pPtr, MetallicTexture, static_cast<unsigned int>(strlen(MetallicTexture)))) {
        // PBR Metallic texture
        out = & mModel->mCurrentMaterial->textureMetallic;
        clampIndex = ObjFile::Material::TextureMetallicType;
    } else if (!ASSIMP_strincmp( pPtr, SheenTexture, static_cast<unsigned int>(strlen(SheenTexture)))) {
        // PBR Sheen (reflectance) texture
        out = & mModel->mCurrentMaterial->textureSheen;
        clampIndex = ObjFile::Material::TextureSheenType;
    } else if (!ASSIMP_strincmp( pPtr, RMATexture, static_cast<unsigned int>(strlen(RMATexture)))) {
        // PBR Rough/Metal/AO texture
        out = & mModel->mCurrentMaterial->textureRMA;
        clampIndex = ObjFile::Material::TextureRMAType;
    } else {
        ASSIMP_LOG_ERROR("OBJ/MTL: Encountered unknown texture type");
        return;
    }

    bool clamp = false;
    getTextureOption(clamp, clampIndex, out);
    mModel->mCurrentMaterial->clamp[clampIndex] = clamp;

    std::string texture;
    mDataIt = getName<DataArrayIt>(mDataIt, mDataItEnd, texture);
    if (nullptr != out) {
        out->Set(mAbsPath + texture);
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
    mDataIt = getNextToken<DataArrayIt>(mDataIt, mDataItEnd);

    // If there is any more texture option
    while (!isEndOfBuffer(mDataIt, mDataItEnd) && *mDataIt == '-') {
        const char *pPtr(&*mDataIt);
        //skip option key and value
        int skipToken = 1;

        if (!ASSIMP_strincmp(pPtr, ClampOption, static_cast<unsigned int>(strlen(ClampOption)))) {
            const DataArrayIt it = getNextToken<DataArrayIt>(mDataIt, mDataItEnd);
            char value[3];
            CopyNextWord(it, mDataItEnd, value, sizeof(value) / sizeof(*value));
            if (!ASSIMP_strincmp(value, "on", 2)) {
                clamp = true;
            }

            skipToken = 2;
        } else if (!ASSIMP_strincmp(pPtr, TypeOption, static_cast<unsigned int>(strlen(TypeOption)))) {
            DataArrayIt it = getNextToken<DataArrayIt>(mDataIt, mDataItEnd);
            char value[12];
            CopyNextWord(it, mDataItEnd, value, sizeof(value) / sizeof(*value));
            if (!ASSIMP_strincmp(value, "cube_top", 8)) {
                clampIndex = ObjFile::Material::TextureReflectionCubeTopType;
                out = &mModel->mCurrentMaterial->textureReflection[0];
            } else if (!ASSIMP_strincmp(value, "cube_bottom", 11)) {
                clampIndex = ObjFile::Material::TextureReflectionCubeBottomType;
                out = &mModel->mCurrentMaterial->textureReflection[1];
            } else if (!ASSIMP_strincmp(value, "cube_front", 10)) {
                clampIndex = ObjFile::Material::TextureReflectionCubeFrontType;
                out = &mModel->mCurrentMaterial->textureReflection[2];
            } else if (!ASSIMP_strincmp(value, "cube_back", 9)) {
                clampIndex = ObjFile::Material::TextureReflectionCubeBackType;
                out = &mModel->mCurrentMaterial->textureReflection[3];
            } else if (!ASSIMP_strincmp(value, "cube_left", 9)) {
                clampIndex = ObjFile::Material::TextureReflectionCubeLeftType;
                out = &mModel->mCurrentMaterial->textureReflection[4];
            } else if (!ASSIMP_strincmp(value, "cube_right", 10)) {
                clampIndex = ObjFile::Material::TextureReflectionCubeRightType;
                out = &mModel->mCurrentMaterial->textureReflection[5];
            } else if (!ASSIMP_strincmp(value, "sphere", 6)) {
                clampIndex = ObjFile::Material::TextureReflectionSphereType;
                out = &mModel->mCurrentMaterial->textureReflection[0];
            }

            skipToken = 2;
        } else if (!ASSIMP_strincmp(pPtr, BumpOption, static_cast<unsigned int>(strlen(BumpOption)))) {
            DataArrayIt it = getNextToken<DataArrayIt>(mDataIt, mDataItEnd);
            getFloat(it, mDataItEnd, mModel->mCurrentMaterial->bump_multiplier);
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
            mDataIt = getNextToken<DataArrayIt>(mDataIt, mDataItEnd);
        }
    }
}

} // Namespace Assimp

#endif // !! ASSIMP_BUILD_NO_OBJ_IMPORTER
