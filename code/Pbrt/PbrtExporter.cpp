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

/* TODO:

Material improvements:
- don't export embedded textures that we're not going to use
- diffuse roughness
- what is with the uv mapping, uv transform not coming through??
- metal? glass? mirror?  detect these better?
  - eta/k from RGB?
- emissive textures: warn at least

Other:
- use aiProcess_GenUVCoords if needed to handle spherical/planar uv mapping?
- don't build up a big string in memory but write directly to a file
- aiProcess_Triangulate meshes to get triangles only?
- animation (allow specifying a time)

 */

#ifndef ASSIMP_BUILD_NO_EXPORT
#ifndef ASSIMP_BUILD_NO_PBRT_EXPORTER

#include "PbrtExporter.h"

#include <assimp/version.h>
#include <assimp/DefaultIOSystem.h>
#include <assimp/IOSystem.hpp>
#include <assimp/Exporter.hpp>
#include <assimp/DefaultLogger.hpp>
#include <assimp/StreamWriter.h>
#include <assimp/Exceptional.h>
#include <assimp/material.h>
#include <assimp/scene.h>
#include <assimp/mesh.h>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <fstream>
#include <functional>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>

#include "Common/StbCommon.h"

using namespace Assimp;

namespace Assimp {

void ExportScenePbrt (
    const char* pFile,
    IOSystem* pIOSystem,
    const aiScene* pScene,
    const ExportProperties* /*pProperties*/
){
    std::string path = DefaultIOSystem::absolutePath(std::string(pFile));
    std::string file = DefaultIOSystem::completeBaseName(std::string(pFile));

    // initialize the exporter
    PbrtExporter exporter(pScene, pIOSystem, path, file);
}

} // end of namespace Assimp

// Constructor
PbrtExporter::PbrtExporter(
        const aiScene *pScene, IOSystem *pIOSystem,
        const std::string &path, const std::string &file) :
        mScene(pScene),
        mIOSystem(pIOSystem),
        mPath(path),
        mFile(file) {
    // Export embedded textures.
    if (mScene->mNumTextures > 0)
        if (!mIOSystem->CreateDirectory("textures"))
            throw DeadlyExportError("Could not create textures/ directory.");
    for (unsigned int i = 0; i < mScene->mNumTextures; ++i) {
        aiTexture* tex = mScene->mTextures[i];
        std::string fn = CleanTextureFilename(tex->mFilename, false);
        std::cerr << "Writing embedded texture: " << tex->mFilename.C_Str() << " -> "
                  << fn << "\n";

        std::unique_ptr<IOStream> outfile(mIOSystem->Open(fn, "wb"));
        if (!outfile) {
            throw DeadlyExportError("could not open output texture file: " + fn);
        }
        if (tex->mHeight == 0) {
            // It's binary data
            outfile->Write(tex->pcData, tex->mWidth, 1);
        } else {
            std::cerr << fn << ": TODO handle uncompressed embedded textures.\n";
        }
    }

#if 0
    // Debugging: print the full node hierarchy
    std::function<void(aiNode*, int)> visitNode;
    visitNode = [&](aiNode* node, int depth) {
        for (int i = 0; i < depth; ++i) std::cerr << "    ";
        std::cerr << node->mName.C_Str() << "\n";
        for (int i = 0; i < node->mNumChildren; ++i)
            visitNode(node->mChildren[i], depth + 1);
    };
    visitNode(mScene->mRootNode, 0);
#endif

    mOutput.precision(ASSIMP_AI_REAL_TEXT_PRECISION);

    // Write everything out
    WriteMetaData();
    WriteCameras();
    WriteWorldDefinition();

    // And write the file to disk...
    std::unique_ptr<IOStream> outfile(mIOSystem->Open(mPath,"wt"));
    if (!outfile) {
        throw DeadlyExportError("could not open output .pbrt file: " + std::string(mFile));
    }
    outfile->Write(mOutput.str().c_str(), mOutput.str().length(), 1);
}

// Destructor
PbrtExporter::~PbrtExporter() = default;

void PbrtExporter::WriteMetaData() {
    mOutput << "#############################\n";
    mOutput << "# Scene metadata:\n";

    aiMetadata* pMetaData = mScene->mMetaData;
    for (unsigned int i = 0; i < pMetaData->mNumProperties; i++) {
        mOutput << "# - ";
        mOutput << pMetaData->mKeys[i].C_Str() << " :";
        switch(pMetaData->mValues[i].mType) {
            case AI_BOOL : {
                mOutput << " ";
                if (*static_cast<bool*>(pMetaData->mValues[i].mData))
                    mOutput << "TRUE\n";
                else
                    mOutput << "FALSE\n";
                break;
            }
            case AI_INT32 : {
                mOutput << " " <<
                    *static_cast<int32_t*>(pMetaData->mValues[i].mData) <<
                    std::endl;
                break;
            }
            case AI_UINT64 :
                mOutput << " " <<
                    *static_cast<uint64_t*>(pMetaData->mValues[i].mData) <<
                    std::endl;
                break;
            case AI_FLOAT :
                mOutput << " " <<
                    *static_cast<float*>(pMetaData->mValues[i].mData) <<
                    std::endl;
                break;
            case AI_DOUBLE :
                mOutput << " " <<
                    *static_cast<double*>(pMetaData->mValues[i].mData) <<
                    std::endl;
                break;
            case AI_AISTRING : {
                aiString* value =
                    static_cast<aiString*>(pMetaData->mValues[i].mData);
                std::string svalue = value->C_Str();
                std::size_t found = svalue.find_first_of('\n');
                mOutput << "\n";
                while (found != std::string::npos) {
                    mOutput << "#     " << svalue.substr(0, found) << "\n";
                    svalue = svalue.substr(found + 1);
                    found = svalue.find_first_of('\n');
                }
                mOutput << "#     " << svalue << "\n";
                break;
            }
            case AI_AIVECTOR3D :
                // TODO
                mOutput << " Vector3D (unable to print)\n";
                break;
            default:
                // AI_META_MAX and FORCE_32BIT
                mOutput << " META_MAX or FORCE_32Bit (unable to print)\n";
                break;
        }
    }
}

void PbrtExporter::WriteCameras() {
    mOutput << "\n";
    mOutput << "###############################\n";
    mOutput << "# Cameras (" << mScene->mNumCameras << ") total\n\n";

    if (mScene->mNumCameras == 0) {
        std::cerr << "Warning: No cameras found in scene file.\n";
        return;
    }

    if (mScene->mNumCameras > 1) {
        std::cerr << "Multiple cameras found in scene file; defaulting to first one specified.\n";
    }

    for (unsigned int i = 0; i < mScene->mNumCameras; i++) {
        WriteCamera(i);
    }
}

aiMatrix4x4 PbrtExporter::GetNodeTransform(const aiString &name) const {
    aiMatrix4x4 m;
    auto node = mScene->mRootNode->FindNode(name);
    if (!node) {
        std::cerr << '"' << name.C_Str() << "\": node not found in scene tree.\n";
        throw DeadlyExportError("Could not find node");
    }
    else {
        while (node) {
            m = node->mTransformation * m;
            node = node->mParent;
        }
    }
    return m;
}

std::string PbrtExporter::TransformAsString(const aiMatrix4x4 &m) {
    // Transpose on the way out to match pbrt's expected layout (sanity
    // check: the translation component should be the last 3 entries
    // before a '1' as the final entry in the matrix, assuming it's
    // non-projective.)
    std::stringstream s;
    s << m.a1 << " " << m.b1 << " " << m.c1 << " " << m.d1 << " "
      << m.a2 << " " << m.b2 << " " << m.c2 << " " << m.d2 << " "
      << m.a3 << " " << m.b3 << " " << m.c3 << " " << m.d3 << " "
      << m.a4 << " " << m.b4 << " " << m.c4 << " " << m.d4;
    return s.str();
}

void PbrtExporter::WriteCamera(int i) {
    auto camera = mScene->mCameras[i];
    bool cameraActive = i == 0;

    mOutput << "# - Camera " << i+1 <<  ": "
        << camera->mName.C_Str() << "\n";

    // Get camera aspect ratio
    float aspect = camera->mAspect;
    if (aspect == 0) {
        aspect = 4.f/3.f;
        mOutput << "#   - Aspect ratio : 1.33333 (no aspect found, defaulting to 4/3)\n";
    } else {
        mOutput << "#   - Aspect ratio : " << aspect << "\n";
    }

    // Get Film xres and yres
    int xres = 1920;
    int yres = (int)round(xres/aspect);

    // Print Film for this camera
    if (!cameraActive)
        mOutput << "# ";
    mOutput << "Film \"rgb\" \"string filename\" \"" << mFile << ".exr\"\n";
    if (!cameraActive)
        mOutput << "# ";
    mOutput << "    \"integer xresolution\" [" << xres << "]\n";
    if (!cameraActive)
        mOutput << "# ";
    mOutput << "    \"integer yresolution\" [" << yres << "]\n";

    // Get camera fov
    float hfov = AI_RAD_TO_DEG(camera->mHorizontalFOV);
    float fov = (aspect >= 1.0) ? hfov : (hfov * aspect);
    if (fov < 5) {
        std::cerr << fov << ": suspiciously low field of view specified by camera. Setting to 45 degrees.\n";
        fov = 45;
    }

    // Get camera transform
    aiMatrix4x4 worldFromCamera = GetNodeTransform(camera->mName);

    // Print Camera LookAt
    auto position = worldFromCamera * camera->mPosition;
    auto lookAt = worldFromCamera * (camera->mPosition + camera->mLookAt);
    aiMatrix3x3 worldFromCamera3(worldFromCamera);
    auto up = worldFromCamera3 * camera->mUp;
    up.Normalize();

    if (!cameraActive)
        mOutput << "# ";
    mOutput << "Scale -1 1 1\n";  // right handed -> left handed
    if (!cameraActive)
        mOutput << "# ";
    mOutput << "LookAt "
        << position.x << " " << position.y << " " << position.z << "\n";
    if (!cameraActive)
        mOutput << "# ";
    mOutput << "       "
        << lookAt.x << " " << lookAt.y << " " << lookAt.z << "\n";
    if (!cameraActive)
        mOutput << "# ";
    mOutput << "       "
        << up.x << " " << up.y << " " << up.z << "\n";

    // Print camera descriptor
    if (!cameraActive)
        mOutput << "# ";
    mOutput << "Camera \"perspective\" \"float fov\" " << "[" << fov << "]\n\n";
}

void PbrtExporter::WriteWorldDefinition() {
    // Figure out which meshes are referenced multiple times; those will be
    // emitted as object instances and the rest will be emitted directly.
    std::map<int, int> meshUses;
    std::function<void(aiNode*)> visitNode;
    visitNode = [&](aiNode* node) {
        for (unsigned int i = 0; i < node->mNumMeshes; ++i)
            ++meshUses[node->mMeshes[i]];
        for (unsigned int i = 0; i < node->mNumChildren; ++i)
            visitNode(node->mChildren[i]);
    };
    visitNode(mScene->mRootNode);
    int nInstanced = 0, nUnused = 0;
    for (const auto &u : meshUses) {
        if (u.second == 0) ++nUnused;
        else if (u.second > 1) ++nInstanced;
    }
    std::cerr << nInstanced << " / " << mScene->mNumMeshes << " meshes instanced.\n";
    if (nUnused)
        std::cerr << nUnused << " meshes defined but not used in scene.\n";

    mOutput << "WorldBegin\n";

    WriteLights();
    WriteTextures();
    WriteMaterials();

    // Object instance definitions
    mOutput << "# Object instance definitions\n\n";
    for (const auto &mu : meshUses) {
        if (mu.second > 1) {
            WriteInstanceDefinition(mu.first);
        }
    }

    mOutput << "# Geometry\n\n";
    aiMatrix4x4 worldFromObject;
    WriteGeometricObjects(mScene->mRootNode, worldFromObject, meshUses);
}

void PbrtExporter::WriteTextures() {
    mOutput << "###################\n";
    mOutput << "# Textures\n\n";

    C_STRUCT aiString path;
    aiTextureMapping mapping;
    unsigned int uvIndex;
    ai_real blend;
    aiTextureOp op;
    aiTextureMapMode mapMode[3];

    // For every material in the scene,
    for (unsigned int m = 0 ; m < mScene->mNumMaterials; m++) {
        auto material = mScene->mMaterials[m];
        // Parse through all texture types,
        for (int tt = 1; tt <= aiTextureType_UNKNOWN; tt++) {
            int ttCount = material->GetTextureCount(aiTextureType(tt));
            // ... and get every texture
            for (int t = 0; t < ttCount; t++) {
                // TODO write out texture specifics
                // TODO UV transforms may be material specific
                //        so those may need to be baked into unique tex name
                if (material->GetTexture(aiTextureType(tt), t, &path, &mapping,
                                         &uvIndex, &blend, &op, mapMode) != AI_SUCCESS) {
                    std::cerr << "Error getting texture! " << m << " " << tt << " " << t << "\n";
                    continue;
                }

                std::string filename = CleanTextureFilename(path);

                if (uvIndex != 0)
                    std::cerr << "Warning: texture \"" << filename << "\" uses uv set #" <<
                        uvIndex << " but the pbrt converter only exports uv set 0.\n";
#if 0
                if (op != aiTextureOp_Multiply)
                    std::cerr << "Warning: unexpected texture op " << (int)op <<
                        " encountered for texture \"" <<
                        filename << "\". The resulting scene may have issues...\n";
                if (blend != 1)
                    std::cerr << "Blend value of " << blend << " found for texture \"" << filename
                              << "\" but not handled in converter.\n";
#endif

                std::string mappingString;
#if 0
                if (mapMode[0] != mapMode[1])
                    std::cerr << "Different texture boundary mode for u and v for texture \"" <<
                        filename << "\". Using u for both.\n";
                switch (mapMode[0]) {
                case aiTextureMapMode_Wrap:
                    // pbrt's default
                    break;
                case aiTextureMapMode_Clamp:
                    mappingString = "\"string wrap\" \"clamp\"";
                    break;
                case aiTextureMapMode_Decal:
                    std::cerr << "Decal texture boundary mode not supported by pbrt for texture \"" <<
                        filename << "\"\n";
                    break;
                case aiTextureMapMode_Mirror:
                    std::cerr << "Mirror texture boundary mode not supported by pbrt for texture \"" <<
                        filename << "\"\n";
                    break;
                default:
                    std::cerr << "Unexpected map mode " << (int)mapMode[0] << " for texture \"" <<
                        filename << "\"\n";
                    //throw DeadlyExportError("Unexpected aiTextureMapMode");
                }
#endif

#if 0
                aiUVTransform uvTransform;
                if (material->Get(AI_MATKEY_TEXTURE(tt, t), uvTransform) == AI_SUCCESS) {
                    mOutput << "# UV transform " << uvTransform.mTranslation.x << " "
                            << uvTransform.mTranslation.y << " " << uvTransform.mScaling.x << " "
                            << uvTransform.mScaling.y << " " << uvTransform.mRotation << "\n";
                }
#endif

                std::string texName, texType, texOptions;
                if (aiTextureType(tt) == aiTextureType_SHININESS ||
                    aiTextureType(tt) == aiTextureType_OPACITY ||
                    aiTextureType(tt) == aiTextureType_HEIGHT ||
                    aiTextureType(tt) == aiTextureType_DISPLACEMENT ||
                    aiTextureType(tt) == aiTextureType_METALNESS ||
                    aiTextureType(tt) == aiTextureType_DIFFUSE_ROUGHNESS) {
                    texType = "float";
                    texName = std::string("float:") + RemoveSuffix(filename);

                    if (aiTextureType(tt) == aiTextureType_SHININESS) {
                        texOptions = "    \"bool invert\" true\n";
                        texName += "_Roughness";
                    }
                } else if (aiTextureType(tt) == aiTextureType_DIFFUSE ||
                           aiTextureType(tt) == aiTextureType_BASE_COLOR) {
                    texType = "spectrum";
                    texName = std::string("rgb:") + RemoveSuffix(filename);
                }

                // Don't export textures we're not actually going to use...
                if (texName.empty())
                    continue;

                if (mTextureSet.find(texName) == mTextureSet.end()) {
                    mOutput << "Texture \"" << texName << "\" \"" << texType << "\" \"imagemap\"\n"
                            << texOptions
                            << "    \"string filename\" \"" << filename << "\" " << mappingString << '\n';
                    mTextureSet.insert(texName);
                }

                // Also emit a float version for use with alpha testing...
                if ((aiTextureType(tt) == aiTextureType_DIFFUSE ||
                     aiTextureType(tt) == aiTextureType_BASE_COLOR) &&
                    TextureHasAlphaMask(filename)) {
                    texType = "float";
                    texName = std::string("alpha:") + filename;
                    if (mTextureSet.find(texName) == mTextureSet.end()) {
                        mOutput << "Texture \"" << texName << "\" \"" << texType << "\" \"imagemap\"\n"
                                << "    \"string filename\" \"" << filename << "\" " << mappingString << '\n';
                        mTextureSet.insert(texName);
                    }
                }
            }
        }
    }
}

bool PbrtExporter::TextureHasAlphaMask(const std::string &filename) {
    // TODO: STBIDEF int      stbi_info               (char const *filename,     int *x, int *y, int *comp);
    // quick return if it's 3

    int xSize, ySize, nComponents;
    unsigned char *data = stbi_load(filename.c_str(), &xSize, &ySize, &nComponents, 0);
    if (!data) {
        std::cerr << filename << ": unable to load texture and check for alpha mask in texture. "
        "Geometry will not be alpha masked with this texture.\n";
        return false;
    }

    bool hasMask = false;
    switch (nComponents) {
    case 1:
        for (int i = 0; i < xSize * ySize; ++i)
            if (data[i] != 255) {
                hasMask = true;
                break;
            }
        break;
    case 2:
          for (int y = 0; y < ySize; ++y)
              for (int x = 0; x < xSize; ++x)
                  if (data[2 * (x + y * xSize) + 1] != 255) {
                      hasMask = true;
                      break;
                  }
        break;
    case 3:
        break;
    case 4:
          for (int y = 0; y < ySize; ++y)
              for (int x = 0; x < xSize; ++x)
                  if (data[4 * (x + y * xSize) + 3] != 255) {
                      hasMask = true;
                      break;
                  }
          break;
    default:
        std::cerr << filename << ": unexpected number of image channels, " <<
            nComponents << ".\n";
    }

    stbi_image_free(data);
    return hasMask;
}

void PbrtExporter::WriteMaterials() {
    mOutput << "\n";
    mOutput << "####################\n";
    mOutput << "# Materials (" << mScene->mNumMaterials << ") total\n\n";

    for (unsigned int i = 0; i < mScene->mNumMaterials; i++) {
        WriteMaterial(i);
    }
    mOutput << "\n\n";
}

void PbrtExporter::WriteMaterial(int m) {
    aiMaterial* material = mScene->mMaterials[m];

    // get material name
    auto materialName = material->GetName();
    mOutput << std::endl << "# - Material " << m+1 <<  ": " << materialName.C_Str() << "\n";

    // Print out number of properties
    mOutput << "#   - Number of Material Properties: " << material->mNumProperties << "\n";

    // Print out texture type counts
    mOutput << "#   - Non-Zero Texture Type Counts: ";
    for (int i = 1; i <= aiTextureType_UNKNOWN; i++) {
        int count = material->GetTextureCount(aiTextureType(i));
        if (count > 0)
            mOutput << aiTextureTypeToString(aiTextureType(i)) << ": " <<  count << " ";
    }
    mOutput << "\n";

    auto White = [](const aiColor3D &c) { return c.r == 1 && c.g == 1 && c.b == 1; };
    auto Black = [](const aiColor3D &c) { return c.r == 0 && c.g == 0 && c.b == 0; };

    aiColor3D diffuse, specular, transparency;
    bool constantDiffuse = (material->Get(AI_MATKEY_COLOR_DIFFUSE, diffuse) == AI_SUCCESS &&
                            !White(diffuse));
    bool constantSpecular = (material->Get(AI_MATKEY_COLOR_SPECULAR, specular) == AI_SUCCESS &&
                             !White(specular));
    bool constantTransparency = (material->Get(AI_MATKEY_COLOR_TRANSPARENT, transparency) == AI_SUCCESS &&
                                 !Black(transparency));

    float opacity, shininess, shininessStrength, eta;
    bool constantOpacity = (material->Get(AI_MATKEY_OPACITY, opacity) == AI_SUCCESS &&
                            opacity != 0);
    bool constantShininess = material->Get(AI_MATKEY_SHININESS, shininess) == AI_SUCCESS;
    bool constantShininessStrength = material->Get(AI_MATKEY_SHININESS_STRENGTH, shininessStrength) == AI_SUCCESS;
    bool constantEta = (material->Get(AI_MATKEY_REFRACTI, eta) == AI_SUCCESS &&
                        eta != 1);

    mOutput << "#    - Constants: diffuse " << constantDiffuse << " specular " << constantSpecular <<
        " transparency " << constantTransparency << " opacity " << constantOpacity <<
        " shininess " << constantShininess << " shininess strength " << constantShininessStrength <<
        " eta " << constantEta << "\n";

    aiString roughnessMap;
    if (material->Get(AI_MATKEY_TEXTURE_SHININESS(0), roughnessMap) == AI_SUCCESS) {
        std::string roughnessTexture = std::string("float:") +
            RemoveSuffix(CleanTextureFilename(roughnessMap)) + "_Roughness";
        mOutput << "MakeNamedMaterial \"" << materialName.C_Str() << "\""
                << " \"string type\" \"coateddiffuse\"\n"
                << "    \"texture roughness\" \"" << roughnessTexture << "\"\n";
    } else if (constantShininess) {
        // Assume plastic for now at least
        float roughness = std::max(0.f, 1.f - shininess);
        mOutput << "MakeNamedMaterial \"" << materialName.C_Str() << "\""
                << " \"string type\" \"coateddiffuse\"\n"
                << "    \"float roughness\" " << roughness << "\n";
    } else
        // Diffuse
        mOutput << "MakeNamedMaterial \"" << materialName.C_Str() << "\""
                << " \"string type\" \"diffuse\"\n";

    aiString diffuseTexture;
    if (material->Get(AI_MATKEY_TEXTURE_DIFFUSE(0), diffuseTexture) == AI_SUCCESS)
        mOutput << "    \"texture reflectance\" \"rgb:" << RemoveSuffix(CleanTextureFilename(diffuseTexture)) << "\"\n";
    else
        mOutput << "    \"rgb reflectance\" [ " << diffuse.r << " " << diffuse.g <<
            " " << diffuse.b << " ]\n";

    aiString displacementTexture, normalMap;
    if (material->Get(AI_MATKEY_TEXTURE_NORMALS(0), displacementTexture) == AI_SUCCESS)
        mOutput << "    \"string normalmap\" \"" << CleanTextureFilename(displacementTexture) << "\"\n";
    else if (material->Get(AI_MATKEY_TEXTURE_HEIGHT(0), displacementTexture) == AI_SUCCESS)
        mOutput << "    \"texture displacement\" \"float:" <<
            RemoveSuffix(CleanTextureFilename(displacementTexture)) << "\"\n";
    else if (material->Get(AI_MATKEY_TEXTURE_DISPLACEMENT(0), displacementTexture) == AI_SUCCESS)
        mOutput << "    \"texture displacement\" \"float:" <<
            RemoveSuffix(CleanTextureFilename(displacementTexture)) << "\"\n";
}

std::string PbrtExporter::CleanTextureFilename(const aiString &f, bool rewriteExtension) const {
    std::string fn = f.C_Str();
    // Remove directory name
    size_t offset = fn.find_last_of("/\\");
    if (offset != std::string::npos) {
        fn.erase(0, offset + 1);
    }

    // Expect all textures in textures
    fn = std::string("textures") + mIOSystem->getOsSeparator() + fn;

    // Rewrite extension for unsupported file formats.
    if (rewriteExtension) {
        offset = fn.rfind('.');
        if (offset != std::string::npos) {
            std::string extension = fn;
            extension.erase(0, offset + 1);
            std::transform(extension.begin(), extension.end(), extension.begin(),
                           [](unsigned char c) { return (char)std::tolower(c); });

            if (extension != "tga" && extension != "exr" && extension != "png" &&
                extension != "pfm" && extension != "hdr") {
                std::string orig = fn;
                fn.erase(offset + 1);
                fn += "png";

                // Does it already exist? Warn if not.
                std::ifstream filestream(fn);
                if (!filestream.good())
                    std::cerr << orig << ": must convert this texture to PNG.\n";
            }
        }
    }

    return fn;
}

std::string PbrtExporter::RemoveSuffix(std::string filename) {
    size_t offset = filename.rfind('.');
    if (offset != std::string::npos)
        filename.erase(offset);
    return filename;
}

void PbrtExporter::WriteLights() {
    mOutput << "\n";
    mOutput << "#################\n";
    mOutput << "# Lights\n\n";
    if (mScene->mNumLights == 0) {
        // Skip the default light if no cameras and this is flat up geometry
        if (mScene->mNumCameras > 0) {
            std::cerr << "No lights specified. Using default infinite light.\n";

            mOutput << "AttributeBegin\n";
            mOutput << "    # default light\n";
            mOutput << "    LightSource \"infinite\" \"blackbody L\" [6000 1]\n";

            mOutput << "AttributeEnd\n\n";
        }
    } else {
        for (unsigned int i = 0; i < mScene->mNumLights; ++i) {
            const aiLight *light = mScene->mLights[i];

            mOutput << "# Light " << light->mName.C_Str() << "\n";
            mOutput << "AttributeBegin\n";

            aiMatrix4x4 worldFromLight = GetNodeTransform(light->mName);
            mOutput << "    Transform [ " << TransformAsString(worldFromLight) << " ]\n";

            aiColor3D color = light->mColorDiffuse + light->mColorSpecular;
            if (light->mAttenuationConstant != 0)
                color = color * (ai_real)(1. / light->mAttenuationConstant);

            switch (light->mType) {
            case aiLightSource_DIRECTIONAL: {
                mOutput << "    LightSource \"distant\"\n";
                mOutput << "        \"point3 from\" [ " << light->mPosition.x << " " <<
                    light->mPosition.y << " " << light->mPosition.z << " ]\n";
                aiVector3D to = light->mPosition + light->mDirection;
                mOutput << "        \"point3 to\" [ " << to.x << " " << to.y << " " << to.z << " ]\n";
                mOutput << "        \"rgb L\" [ " << color.r << " " << color.g << " " << color.b << " ]\n";
                break;
            } case aiLightSource_POINT:
                mOutput << "    LightSource \"distant\"\n";
                mOutput << "        \"point3 from\" [ " << light->mPosition.x << " " <<
                    light->mPosition.y << " " << light->mPosition.z << " ]\n";
                mOutput << "        \"rgb L\" [ " << color.r << " " << color.g << " " << color.b << " ]\n";
                break;
            case aiLightSource_SPOT: {
                mOutput << "    LightSource \"spot\"\n";
                mOutput << "        \"point3 from\" [ " << light->mPosition.x << " " <<
                    light->mPosition.y << " " << light->mPosition.z << " ]\n";
                aiVector3D to = light->mPosition + light->mDirection;
                mOutput << "        \"point3 to\" [ " << to.x << " " << to.y << " " << to.z << " ]\n";
                mOutput << "        \"rgb L\" [ " << color.r << " " << color.g << " " << color.b << " ]\n";
                mOutput << "        \"float coneangle\" [ " << AI_RAD_TO_DEG(light->mAngleOuterCone) << " ]\n";
                mOutput << "        \"float conedeltaangle\" [ " << AI_RAD_TO_DEG(light->mAngleOuterCone -
                                                                                  light->mAngleInnerCone) << " ]\n";
                break;
            } case aiLightSource_AMBIENT:
                mOutput << "# ignored ambient light source\n";
                break;
            case aiLightSource_AREA: {
                aiVector3D left = light->mDirection ^ light->mUp;
                // rectangle. center at position, direction is normal vector
                ai_real dLeft = light->mSize.x / 2, dUp = light->mSize.y / 2;
                aiVector3D vertices[4] = {
                     light->mPosition - dLeft * left - dUp * light->mUp,
                     light->mPosition + dLeft * left - dUp * light->mUp,
                     light->mPosition - dLeft * left + dUp * light->mUp,
                     light->mPosition + dLeft * left + dUp * light->mUp };
                mOutput << "    AreaLightSource \"diffuse\"\n";
                mOutput << "        \"rgb L\" [ " << color.r << " " << color.g << " " << color.b << " ]\n";
                mOutput << "    Shape \"bilinearmesh\"\n";
                mOutput << "        \"point3 p\" [ ";
                for (int j = 0; j < 4; ++j)
                    mOutput << vertices[j].x << " " << vertices[j].y << " " << vertices[j].z;
                mOutput << " ]\n";
                mOutput << "        \"integer indices\" [ 0 1 2 3 ]\n";
                break;
            } default:
                mOutput << "# ignored undefined light source type\n";
                break;
            }
            mOutput << "AttributeEnd\n\n";
        }
    }
}

void PbrtExporter::WriteMesh(aiMesh* mesh) {
    mOutput << "# - Mesh: ";
    if (mesh->mName == aiString(""))
        mOutput << "<No Name>\n";
    else
        mOutput << mesh->mName.C_Str() << "\n";

    mOutput << "AttributeBegin\n";
    aiMaterial* material = mScene->mMaterials[mesh->mMaterialIndex];
    mOutput << "    NamedMaterial \"" << material->GetName().C_Str() << "\"\n";

    // Handle area lights
    aiColor3D emission;
    if (material->Get(AI_MATKEY_COLOR_EMISSIVE, emission) == AI_SUCCESS &&
        (emission.r > 0 || emission.g > 0 || emission.b > 0))
        mOutput << "    AreaLightSource \"diffuse\" \"rgb L\" [ " << emission.r <<
            " " << emission.g << " " << emission.b << " ]\n";

    // Check if any types other than tri
    if (   (mesh->mPrimitiveTypes & aiPrimitiveType_POINT)
        || (mesh->mPrimitiveTypes & aiPrimitiveType_LINE)
        || (mesh->mPrimitiveTypes & aiPrimitiveType_POLYGON)) {
        std::cerr << "Error: ignoring point / line / polygon mesh " << mesh->mName.C_Str() << ".\n";
        return;
    }

    // Alpha mask
    std::string alpha;
    aiString opacityTexture;
    if (material->Get(AI_MATKEY_TEXTURE_OPACITY(0), opacityTexture) == AI_SUCCESS ||
        material->Get(AI_MATKEY_TEXTURE_DIFFUSE(0), opacityTexture) == AI_SUCCESS) {
        // material->Get(AI_MATKEY_TEXTURE_BASE_COLOR(0), opacityTexture) == AI_SUCCESS)
        std::string texName = std::string("alpha:") + CleanTextureFilename(opacityTexture);
        if (mTextureSet.find(texName) != mTextureSet.end())
            alpha = std::string("    \"texture alpha\" \"") + texName + "\"\n";
    } else {
        float opacity = 1;
        if (material->Get(AI_MATKEY_OPACITY, opacity) == AI_SUCCESS && opacity < 1)
            alpha = std::string("    \"float alpha\" [ ") + std::to_string(opacity) + " ]\n";
    }

    // Output the shape specification
    mOutput << "Shape \"trianglemesh\"\n" <<
        alpha <<
        "    \"integer indices\" [";

    // Start with faces (which hold indices)
    for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
        auto face = mesh->mFaces[i];
        if (face.mNumIndices != 3) throw DeadlyExportError("oh no not a tri!");

        for (unsigned int j = 0; j < face.mNumIndices; j++) {
            mOutput << face.mIndices[j] << " ";
        }
        if ((i % 7) == 6) mOutput << "\n    ";
    }
    mOutput << "]\n";

    // Then go to vertices
    mOutput << "    \"point3 P\" [";
    for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
        auto vector = mesh->mVertices[i];
        mOutput << vector.x << " " << vector.y << " " << vector.z << "  ";
        if ((i % 4) == 3) mOutput << "\n    ";
    }
    mOutput << "]\n";

    // Normals (if present)
    if (mesh->mNormals) {
        mOutput << "    \"normal N\" [";
        for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
            auto normal = mesh->mNormals[i];
            mOutput << normal.x << " " << normal.y << " " << normal.z << "  ";
            if ((i % 4) == 3) mOutput << "\n    ";
        }
        mOutput << "]\n";
    }

    // Tangents (if present)
    if (mesh->mTangents) {
        mOutput << "    \"vector3 S\" [";
        for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
            auto tangent = mesh->mTangents[i];
            mOutput << tangent.x << " " << tangent.y << " " << tangent.z << "  ";
            if ((i % 4) == 3) mOutput << "\n    ";
        }
        mOutput << "]\n";
    }

    // Texture Coords (if present)
    // Find the first set of 2D texture coordinates..
    for (int i = 0; i < AI_MAX_NUMBER_OF_TEXTURECOORDS; ++i) {
        if (mesh->mNumUVComponents[i] == 2) {
            // assert(mesh->mTextureCoords[i] != nullptr);
            aiVector3D* uv = mesh->mTextureCoords[i];
            mOutput << "    \"point2 uv\" [";
            for (unsigned int j = 0; j < mesh->mNumVertices; ++j) {
                mOutput << uv[j].x << " " << uv[j].y << " ";
                if ((j % 6) == 5) mOutput << "\n    ";
            }
            mOutput << "]\n";
            break;
        }
    }
    // TODO: issue warning if there are additional UV sets?

    mOutput << "AttributeEnd\n";
}

void PbrtExporter::WriteInstanceDefinition(int i) {
    aiMesh* mesh = mScene->mMeshes[i];

    mOutput << "ObjectBegin \"";
    if (mesh->mName == aiString(""))
        mOutput << "mesh_" << i+1 << "\"\n";
    else
        mOutput << mesh->mName.C_Str() << "_" << i+1 << "\"\n";

    WriteMesh(mesh);

    mOutput << "ObjectEnd\n";
}

void PbrtExporter::WriteGeometricObjects(aiNode* node, aiMatrix4x4 worldFromObject,
                                         std::map<int, int> &meshUses) {
    // Sometimes interior nodes have degenerate matrices??
    if (node->mTransformation.Determinant() != 0)
        worldFromObject = worldFromObject * node->mTransformation;

    if (node->mNumMeshes > 0) {
        mOutput << "AttributeBegin\n";

        mOutput << "  Transform [ " << TransformAsString(worldFromObject) << "]\n";

        for (unsigned int i = 0; i < node->mNumMeshes; i++) {
            aiMesh* mesh = mScene->mMeshes[node->mMeshes[i]];
            if (meshUses[node->mMeshes[i]] == 1) {
                // If it's only used once in the scene, emit it directly as
                // a triangle mesh.
                mOutput << "  # " << mesh->mName.C_Str();
                WriteMesh(mesh);
            } else {
                // If it's used multiple times, there will be an object
                // instance for it, so emit a reference to that.
                mOutput << "  ObjectInstance \"";
                if (mesh->mName == aiString(""))
                    mOutput << "mesh_" << node->mMeshes[i] + 1 << "\"\n";
                else
                    mOutput << mesh->mName.C_Str() << "_" << node->mMeshes[i] + 1 << "\"\n";
            }
        }
        mOutput << "AttributeEnd\n\n";
    }

    // Recurse through children
    for (unsigned int i = 0; i < node->mNumChildren; i++) {
        WriteGeometricObjects(node->mChildren[i], worldFromObject, meshUses);
    }
}

#endif // ASSIMP_BUILD_NO_PBRT_EXPORTER
#endif // ASSIMP_BUILD_NO_EXPORT
