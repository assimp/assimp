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
#ifndef ASSIMP_BUILD_NO_COLLADA_EXPORTER

#include "ColladaExporter.h"

#include <assimp/Bitmap.h>
#include <assimp/ColladaMetaData.h>
#include <assimp/DefaultIOSystem.h>
#include <assimp/Exceptional.h>
#include <assimp/MathFunctions.h>
#include <assimp/SceneCombiner.h>
#include <assimp/StringUtils.h>
#include <assimp/XMLTools.h>
#include <assimp/commonMetaData.h>
#include <assimp/fast_atof.h>
#include <assimp/scene.h>
#include <assimp/Exporter.hpp>
#include <assimp/IOSystem.hpp>

#include <ctime>
#include <memory>

namespace Assimp {

// ------------------------------------------------------------------------------------------------
// Worker function for exporting a scene to Collada. Prototyped and registered in Exporter.cpp
void ExportSceneCollada(const char *pFile, IOSystem *pIOSystem, const aiScene *pScene, const ExportProperties * /*pProperties*/) {
    std::string path = DefaultIOSystem::absolutePath(std::string(pFile));
    std::string file = DefaultIOSystem::completeBaseName(std::string(pFile));

    // invoke the exporter
    ColladaExporter iDoTheExportThing(pScene, pIOSystem, path, file);

    if (iDoTheExportThing.mOutput.fail()) {
        throw DeadlyExportError("output data creation failed. Most likely the file became too large: " + std::string(pFile));
    }

    // we're still here - export successfully completed. Write result to the given IOSYstem
    std::unique_ptr<IOStream> outfile(pIOSystem->Open(pFile, "wt"));
    if (outfile == nullptr) {
        throw DeadlyExportError("could not open output .dae file: " + std::string(pFile));
    }

    // XXX maybe use a small wrapper around IOStream that behaves like std::stringstream in order to avoid the extra copy.
    outfile->Write(iDoTheExportThing.mOutput.str().c_str(), static_cast<size_t>(iDoTheExportThing.mOutput.tellp()), 1);
}

// ------------------------------------------------------------------------------------------------
// Encodes a string into a valid XML ID using the xsd:ID schema qualifications.
static const std::string XMLIDEncode(const std::string &name) {
    const char XML_ID_CHARS[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz_-.";
    const unsigned int XML_ID_CHARS_COUNT = sizeof(XML_ID_CHARS) / sizeof(char);

    if (name.length() == 0) {
        return name;
    }

    std::stringstream idEncoded;

    // xsd:ID must start with letter or underscore
    if (!((name[0] >= 'A' && name[0] <= 'z') || name[0] == '_')) {
        idEncoded << '_';
    }

    for (std::string::const_iterator it = name.begin(); it != name.end(); ++it) {
        // xsd:ID can only contain letters, digits, underscores, hyphens and periods
        if (strchr(XML_ID_CHARS, *it) != nullptr) {
            idEncoded << *it;
        } else {
            // Select placeholder character based on invalid character to reduce ID collisions
            idEncoded << XML_ID_CHARS[(*it) % XML_ID_CHARS_COUNT];
        }
    }

    return idEncoded.str();
}

// ------------------------------------------------------------------------------------------------
// Helper functions to create unique ids
inline bool IsUniqueId(const std::unordered_set<std::string> &idSet, const std::string &idStr) {
    return (idSet.find(idStr) == idSet.end());
}

inline std::string MakeUniqueId(const std::unordered_set<std::string> &idSet, const std::string &idPrefix, const std::string &postfix) {
    std::string result(idPrefix + postfix);
    if (!IsUniqueId(idSet, result)) {
        // Select a number to append
        size_t idnum = 1;
        do {
            result = idPrefix + '_' + ai_to_string(idnum) + postfix;
            ++idnum;
        } while (!IsUniqueId(idSet, result));
    }
    return result;
}

// ------------------------------------------------------------------------------------------------
// Constructor for a specific scene to export
ColladaExporter::ColladaExporter(const aiScene *pScene, IOSystem *pIOSystem, const std::string &path, const std::string &file) :
        mIOSystem(pIOSystem),
        mPath(path),
        mFile(file),
        mScene(pScene),
        endstr("\n") {
    // make sure that all formatting happens using the standard, C locale and not the user's current locale
    mOutput.imbue(std::locale("C"));
    mOutput.precision(ASSIMP_AI_REAL_TEXT_PRECISION);

    // start writing the file
    WriteFile();
}

// ------------------------------------------------------------------------------------------------
// Destructor
ColladaExporter::~ColladaExporter() = default;

// ------------------------------------------------------------------------------------------------
// Starts writing the contents
void ColladaExporter::WriteFile() {
    // write the DTD
    mOutput << "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\" ?>" << endstr;
    // COLLADA element start
    mOutput << "<COLLADA xmlns=\"http://www.collada.org/2005/11/COLLADASchema\" version=\"1.4.1\">" << endstr;
    PushTag();

    WriteTextures();
    WriteHeader();

    // Add node names to the unique id database first so they are most likely to use their names as unique ids
    CreateNodeIds(mScene->mRootNode);

    WriteCamerasLibrary();
    WriteLightsLibrary();
    WriteMaterials();
    WriteGeometryLibrary();
    WriteControllerLibrary();

    WriteSceneLibrary();

    // customized, Writes the animation library
    WriteAnimationsLibrary();

    // instantiate the scene(s)
    // For Assimp there will only ever be one
    mOutput << startstr << "<scene>" << endstr;
    PushTag();
    mOutput << startstr << "<instance_visual_scene url=\"#" + mSceneId + "\" />" << endstr;
    PopTag();
    mOutput << startstr << "</scene>" << endstr;
    PopTag();
    mOutput << "</COLLADA>" << endstr;
}

// ------------------------------------------------------------------------------------------------
// Writes the asset header
void ColladaExporter::WriteHeader() {
    static const ai_real epsilon = Math::getEpsilon<ai_real>();
    static const aiQuaternion x_rot(aiMatrix3x3(
            0, -1, 0,
            1, 0, 0,
            0, 0, 1));
    static const aiQuaternion y_rot(aiMatrix3x3(
            1, 0, 0,
            0, 1, 0,
            0, 0, 1));
    static const aiQuaternion z_rot(aiMatrix3x3(
            1, 0, 0,
            0, 0, 1,
            0, -1, 0));

    static const unsigned int date_nb_chars = 20;
    char date_str[date_nb_chars];
    std::time_t date = std::time(nullptr);
    std::strftime(date_str, date_nb_chars, "%Y-%m-%dT%H:%M:%S", std::localtime(&date));

    aiVector3D scaling;
    aiQuaternion rotation;
    aiVector3D position;
    mScene->mRootNode->mTransformation.Decompose(scaling, rotation, position);
    rotation.Normalize();

    mAdd_root_node = false;

    ai_real scale = 1.0;
    if (std::abs(scaling.x - scaling.y) <= epsilon && std::abs(scaling.x - scaling.z) <= epsilon && std::abs(scaling.y - scaling.z) <= epsilon) {
        scale = (ai_real)((((double)scaling.x) + ((double)scaling.y) + ((double)scaling.z)) / 3.0);
    } else {
        mAdd_root_node = true;
    }

    std::string up_axis = "Y_UP";
    if (rotation.Equal(x_rot, epsilon)) {
        up_axis = "X_UP";
    } else if (rotation.Equal(y_rot, epsilon)) {
        up_axis = "Y_UP";
    } else if (rotation.Equal(z_rot, epsilon)) {
        up_axis = "Z_UP";
    } else {
        mAdd_root_node = true;
    }

    if (!position.Equal(aiVector3D(0, 0, 0))) {
        mAdd_root_node = true;
    }

    // Assimp root nodes can have meshes, Collada Scenes cannot
    if (mScene->mRootNode->mNumChildren == 0 || mScene->mRootNode->mMeshes != 0) {
        mAdd_root_node = true;
    }

    if (mAdd_root_node) {
        up_axis = "Y_UP";
        scale = 1.0;
    }

    mOutput << startstr << "<asset>" << endstr;
    PushTag();
    mOutput << startstr << "<contributor>" << endstr;
    PushTag();

    // If no Scene metadata, use root node metadata
    aiMetadata *meta = mScene->mMetaData;
    if (nullptr == meta) {
        meta = mScene->mRootNode->mMetaData;
    }

    aiString value;
    if (!meta || !meta->Get("Author", value)) {
        mOutput << startstr << "<author>"
                << "Assimp"
                << "</author>" << endstr;
    } else {
        mOutput << startstr << "<author>" << XMLEscape(value.C_Str()) << "</author>" << endstr;
    }

    if (nullptr == meta || !meta->Get(AI_METADATA_SOURCE_GENERATOR, value)) {
        mOutput << startstr << "<authoring_tool>"
                << "Assimp Exporter"
                << "</authoring_tool>" << endstr;
    } else {
        mOutput << startstr << "<authoring_tool>" << XMLEscape(value.C_Str()) << "</authoring_tool>" << endstr;
    }

    if (meta) {
        if (meta->Get("Comments", value)) {
            mOutput << startstr << "<comments>" << XMLEscape(value.C_Str()) << "</comments>" << endstr;
        }
        if (meta->Get(AI_METADATA_SOURCE_COPYRIGHT, value)) {
            mOutput << startstr << "<copyright>" << XMLEscape(value.C_Str()) << "</copyright>" << endstr;
        }
        if (meta->Get("SourceData", value)) {
            mOutput << startstr << "<source_data>" << XMLEscape(value.C_Str()) << "</source_data>" << endstr;
        }
    }

    PopTag();
    mOutput << startstr << "</contributor>" << endstr;

    if (nullptr == meta || !meta->Get("Created", value)) {
        mOutput << startstr << "<created>" << date_str << "</created>" << endstr;
    } else {
        mOutput << startstr << "<created>" << XMLEscape(value.C_Str()) << "</created>" << endstr;
    }

    // Modified date is always the date saved
    mOutput << startstr << "<modified>" << date_str << "</modified>" << endstr;

    if (meta) {
        if (meta->Get("Keywords", value)) {
            mOutput << startstr << "<keywords>" << XMLEscape(value.C_Str()) << "</keywords>" << endstr;
        }
        if (meta->Get("Revision", value)) {
            mOutput << startstr << "<revision>" << XMLEscape(value.C_Str()) << "</revision>" << endstr;
        }
        if (meta->Get("Subject", value)) {
            mOutput << startstr << "<subject>" << XMLEscape(value.C_Str()) << "</subject>" << endstr;
        }
        if (meta->Get("Title", value)) {
            mOutput << startstr << "<title>" << XMLEscape(value.C_Str()) << "</title>" << endstr;
        }
    }

    mOutput << startstr << "<unit name=\"meter\" meter=\"" << scale << "\" />" << endstr;
    mOutput << startstr << "<up_axis>" << up_axis << "</up_axis>" << endstr;
    PopTag();
    mOutput << startstr << "</asset>" << endstr;
}

// ------------------------------------------------------------------------------------------------
// Write the embedded textures
void ColladaExporter::WriteTextures() {
    static const unsigned int buffer_size = 1024;
    char str[buffer_size];

    if (mScene->HasTextures()) {
        for (unsigned int i = 0; i < mScene->mNumTextures; i++) {
            // It would be great to be able to create a directory in portable standard C++, but it's not the case,
            // so we just write the textures in the current directory.

            aiTexture *texture = mScene->mTextures[i];
            if (nullptr == texture) {
                continue;
            }

            ASSIMP_itoa10(str, buffer_size, i + 1);

            std::string name = mFile + "_texture_" + (i < 1000 ? "0" : "") + (i < 100 ? "0" : "") + (i < 10 ? "0" : "") + str + "." + ((const char *)texture->achFormatHint);

            std::unique_ptr<IOStream> outfile(mIOSystem->Open(mPath + mIOSystem->getOsSeparator() + name, "wb"));
            if (outfile == nullptr) {
                throw DeadlyExportError("could not open output texture file: " + mPath + name);
            }

            if (texture->mHeight == 0) {
                outfile->Write((void *)texture->pcData, texture->mWidth, 1);
            } else {
                Bitmap::Save(texture, outfile.get());
            }

            outfile->Flush();

            textures.insert(std::make_pair(i, name));
        }
    }
}

// ------------------------------------------------------------------------------------------------
// Write the embedded textures
void ColladaExporter::WriteCamerasLibrary() {
    if (mScene->HasCameras()) {

        mOutput << startstr << "<library_cameras>" << endstr;
        PushTag();

        for (size_t a = 0; a < mScene->mNumCameras; ++a)
            WriteCamera(a);

        PopTag();
        mOutput << startstr << "</library_cameras>" << endstr;
    }
}

void ColladaExporter::WriteCamera(size_t pIndex) {

    const aiCamera *cam = mScene->mCameras[pIndex];
    const std::string cameraId = GetObjectUniqueId(AiObjectType::Camera, pIndex);
    const std::string cameraName = GetObjectName(AiObjectType::Camera, pIndex);

    mOutput << startstr << "<camera id=\"" << cameraId << "\" name=\"" << cameraName << "\" >" << endstr;
    PushTag();
    mOutput << startstr << "<optics>" << endstr;
    PushTag();
    mOutput << startstr << "<technique_common>" << endstr;
    PushTag();
    //assimp doesn't support the import of orthographic cameras! se we write
    //always perspective
    mOutput << startstr << "<perspective>" << endstr;
    PushTag();
    mOutput << startstr << "<xfov sid=\"xfov\">" << AI_RAD_TO_DEG(cam->mHorizontalFOV)
            << "</xfov>" << endstr;
    mOutput << startstr << "<aspect_ratio>"
            << cam->mAspect
            << "</aspect_ratio>" << endstr;
    mOutput << startstr << "<znear sid=\"znear\">"
            << cam->mClipPlaneNear
            << "</znear>" << endstr;
    mOutput << startstr << "<zfar sid=\"zfar\">"
            << cam->mClipPlaneFar
            << "</zfar>" << endstr;
    PopTag();
    mOutput << startstr << "</perspective>" << endstr;
    PopTag();
    mOutput << startstr << "</technique_common>" << endstr;
    PopTag();
    mOutput << startstr << "</optics>" << endstr;
    PopTag();
    mOutput << startstr << "</camera>" << endstr;
}

// ------------------------------------------------------------------------------------------------
// Write the embedded textures
void ColladaExporter::WriteLightsLibrary() {
    if (mScene->HasLights()) {

        mOutput << startstr << "<library_lights>" << endstr;
        PushTag();

        for (size_t a = 0; a < mScene->mNumLights; ++a)
            WriteLight(a);

        PopTag();
        mOutput << startstr << "</library_lights>" << endstr;
    }
}

void ColladaExporter::WriteLight(size_t pIndex) {

    const aiLight *light = mScene->mLights[pIndex];
    const std::string lightId = GetObjectUniqueId(AiObjectType::Light, pIndex);
    const std::string lightName = GetObjectName(AiObjectType::Light, pIndex);

    mOutput << startstr << "<light id=\"" << lightId << "\" name=\""
            << lightName << "\" >" << endstr;
    PushTag();
    mOutput << startstr << "<technique_common>" << endstr;
    PushTag();
    switch (light->mType) {
    case aiLightSource_AMBIENT:
        WriteAmbienttLight(light);
        break;
    case aiLightSource_DIRECTIONAL:
        WriteDirectionalLight(light);
        break;
    case aiLightSource_POINT:
        WritePointLight(light);
        break;
    case aiLightSource_SPOT:
        WriteSpotLight(light);
        break;
    case aiLightSource_AREA:
    case aiLightSource_UNDEFINED:
    case _aiLightSource_Force32Bit:
        break;
    }
    PopTag();
    mOutput << startstr << "</technique_common>" << endstr;

    PopTag();
    mOutput << startstr << "</light>" << endstr;
}

void ColladaExporter::WritePointLight(const aiLight *const light) {
    const aiColor3D &color = light->mColorDiffuse;
    mOutput << startstr << "<point>" << endstr;
    PushTag();
    mOutput << startstr << "<color sid=\"color\">"
            << color.r << " " << color.g << " " << color.b
            << "</color>" << endstr;
    mOutput << startstr << "<constant_attenuation>"
            << light->mAttenuationConstant
            << "</constant_attenuation>" << endstr;
    mOutput << startstr << "<linear_attenuation>"
            << light->mAttenuationLinear
            << "</linear_attenuation>" << endstr;
    mOutput << startstr << "<quadratic_attenuation>"
            << light->mAttenuationQuadratic
            << "</quadratic_attenuation>" << endstr;

    PopTag();
    mOutput << startstr << "</point>" << endstr;
}

void ColladaExporter::WriteDirectionalLight(const aiLight *const light) {
    const aiColor3D &color = light->mColorDiffuse;
    mOutput << startstr << "<directional>" << endstr;
    PushTag();
    mOutput << startstr << "<color sid=\"color\">"
            << color.r << " " << color.g << " " << color.b
            << "</color>" << endstr;

    PopTag();
    mOutput << startstr << "</directional>" << endstr;
}

void ColladaExporter::WriteSpotLight(const aiLight *const light) {

    const aiColor3D &color = light->mColorDiffuse;
    mOutput << startstr << "<spot>" << endstr;
    PushTag();
    mOutput << startstr << "<color sid=\"color\">"
            << color.r << " " << color.g << " " << color.b
            << "</color>" << endstr;
    mOutput << startstr << "<constant_attenuation>"
            << light->mAttenuationConstant
            << "</constant_attenuation>" << endstr;
    mOutput << startstr << "<linear_attenuation>"
            << light->mAttenuationLinear
            << "</linear_attenuation>" << endstr;
    mOutput << startstr << "<quadratic_attenuation>"
            << light->mAttenuationQuadratic
            << "</quadratic_attenuation>" << endstr;
    /*
    out->mAngleOuterCone = AI_DEG_TO_RAD (std::acos(std::pow(0.1f,1.f/srcLight->mFalloffExponent))+
                            srcLight->mFalloffAngle);
    */

    const ai_real fallOffAngle = AI_RAD_TO_DEG(light->mAngleInnerCone);
    mOutput << startstr << "<falloff_angle sid=\"fall_off_angle\">"
            << fallOffAngle
            << "</falloff_angle>" << endstr;
    double temp = light->mAngleOuterCone - light->mAngleInnerCone;

    temp = std::cos(temp);
    temp = std::log(temp) / std::log(0.1);
    temp = 1 / temp;
    mOutput << startstr << "<falloff_exponent sid=\"fall_off_exponent\">"
            << temp
            << "</falloff_exponent>" << endstr;

    PopTag();
    mOutput << startstr << "</spot>" << endstr;
}

void ColladaExporter::WriteAmbienttLight(const aiLight *const light) {

    const aiColor3D &color = light->mColorAmbient;
    mOutput << startstr << "<ambient>" << endstr;
    PushTag();
    mOutput << startstr << "<color sid=\"color\">"
            << color.r << " " << color.g << " " << color.b
            << "</color>" << endstr;

    PopTag();
    mOutput << startstr << "</ambient>" << endstr;
}

// ------------------------------------------------------------------------------------------------
// Reads a single surface entry from the given material keys
bool ColladaExporter::ReadMaterialSurface(Surface &poSurface, const aiMaterial &pSrcMat, aiTextureType pTexture, const char *pKey, size_t pType, size_t pIndex) {
    if (pSrcMat.GetTextureCount(pTexture) > 0) {
        aiString texfile;
        unsigned int uvChannel = 0;
        pSrcMat.GetTexture(pTexture, 0, &texfile, nullptr, &uvChannel);

        std::string index_str(texfile.C_Str());

        if (index_str.size() != 0 && index_str[0] == '*') {
            unsigned int index;

            index_str = index_str.substr(1, std::string::npos);

            try {
                index = (unsigned int)strtoul10_64<DeadlyExportError>(index_str.c_str());
            } catch (std::exception &error) {
                throw DeadlyExportError(error.what());
            }

            std::map<unsigned int, std::string>::const_iterator name = textures.find(index);

            if (name != textures.end()) {
                poSurface.texture = name->second;
            } else {
                throw DeadlyExportError("could not find embedded texture at index " + index_str);
            }
        } else {
            poSurface.texture = texfile.C_Str();
        }

        poSurface.channel = uvChannel;
        poSurface.exist = true;
    } else {
        if (pKey)
            poSurface.exist = pSrcMat.Get(pKey, static_cast<unsigned int>(pType), static_cast<unsigned int>(pIndex), poSurface.color) == aiReturn_SUCCESS;
    }
    return poSurface.exist;
}

// ------------------------------------------------------------------------------------------------
// Reimplementation of isalnum(,C locale), because AppVeyor does not see standard version.
static bool isalnum_C(char c) {
    return (nullptr != strchr("0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz", c));
}

// ------------------------------------------------------------------------------------------------
// Writes an image entry for the given surface
void ColladaExporter::WriteImageEntry(const Surface &pSurface, const std::string &imageId) {
    if (!pSurface.texture.empty()) {
        mOutput << startstr << "<image id=\"" << imageId << "\">" << endstr;
        PushTag();
        mOutput << startstr << "<init_from>";

        // URL encode image file name first, then XML encode on top
        std::stringstream imageUrlEncoded;
        for (std::string::const_iterator it = pSurface.texture.begin(); it != pSurface.texture.end(); ++it) {
            if (isalnum_C((unsigned char)*it) || *it == ':' || *it == '_' || *it == '-' || *it == '.' || *it == '/' || *it == '\\')
                imageUrlEncoded << *it;
            else
                imageUrlEncoded << '%' << std::hex << size_t((unsigned char)*it) << std::dec;
        }
        mOutput << XMLEscape(imageUrlEncoded.str());
        mOutput << "</init_from>" << endstr;
        PopTag();
        mOutput << startstr << "</image>" << endstr;
    }
}

// ------------------------------------------------------------------------------------------------
// Writes a color-or-texture entry into an effect definition
void ColladaExporter::WriteTextureColorEntry(const Surface &pSurface, const std::string &pTypeName, const std::string &imageId) {
    if (pSurface.exist) {
        mOutput << startstr << "<" << pTypeName << ">" << endstr;
        PushTag();
        if (pSurface.texture.empty()) {
            mOutput << startstr << "<color sid=\"" << pTypeName << "\">" << pSurface.color.r << "   " << pSurface.color.g << "   " << pSurface.color.b << "   " << pSurface.color.a << "</color>" << endstr;
        } else {
            mOutput << startstr << "<texture texture=\"" << imageId << "\" texcoord=\"CHANNEL" << pSurface.channel << "\" />" << endstr;
        }
        PopTag();
        mOutput << startstr << "</" << pTypeName << ">" << endstr;
    }
}

// ------------------------------------------------------------------------------------------------
// Writes the two parameters necessary for referencing a texture in an effect entry
void ColladaExporter::WriteTextureParamEntry(const Surface &pSurface, const std::string &pTypeName, const std::string &materialId) {
    // if surface is a texture, write out the sampler and the surface parameters necessary to reference the texture
    if (!pSurface.texture.empty()) {
        mOutput << startstr << "<newparam sid=\"" << materialId << "-" << pTypeName << "-surface\">" << endstr;
        PushTag();
        mOutput << startstr << "<surface type=\"2D\">" << endstr;
        PushTag();
        mOutput << startstr << "<init_from>" << materialId << "-" << pTypeName << "-image</init_from>" << endstr;
        PopTag();
        mOutput << startstr << "</surface>" << endstr;
        PopTag();
        mOutput << startstr << "</newparam>" << endstr;

        mOutput << startstr << "<newparam sid=\"" << materialId << "-" << pTypeName << "-sampler\">" << endstr;
        PushTag();
        mOutput << startstr << "<sampler2D>" << endstr;
        PushTag();
        mOutput << startstr << "<source>" << materialId << "-" << pTypeName << "-surface</source>" << endstr;
        PopTag();
        mOutput << startstr << "</sampler2D>" << endstr;
        PopTag();
        mOutput << startstr << "</newparam>" << endstr;
    }
}

// ------------------------------------------------------------------------------------------------
// Writes a scalar property
void ColladaExporter::WriteFloatEntry(const Property &pProperty, const std::string &pTypeName) {
    if (pProperty.exist) {
        mOutput << startstr << "<" << pTypeName << ">" << endstr;
        PushTag();
        mOutput << startstr << "<float sid=\"" << pTypeName << "\">" << pProperty.value << "</float>" << endstr;
        PopTag();
        mOutput << startstr << "</" << pTypeName << ">" << endstr;
    }
}

// ------------------------------------------------------------------------------------------------
// Writes the material setup
void ColladaExporter::WriteMaterials() {
    std::vector<Material> materials;
    materials.resize(mScene->mNumMaterials);

    /// collect all materials from the scene
    size_t numTextures = 0;
    for (size_t a = 0; a < mScene->mNumMaterials; ++a) {
        Material &material = materials[a];
        material.id = GetObjectUniqueId(AiObjectType::Material, a);
        material.name = GetObjectName(AiObjectType::Material, a);

        const aiMaterial &mat = *(mScene->mMaterials[a]);
        aiShadingMode shading = aiShadingMode_Flat;
        material.shading_model = "phong";
        if (mat.Get(AI_MATKEY_SHADING_MODEL, shading) == aiReturn_SUCCESS) {
            if (shading == aiShadingMode_Phong) {
                material.shading_model = "phong";
            } else if (shading == aiShadingMode_Blinn) {
                material.shading_model = "blinn";
            } else if (shading == aiShadingMode_NoShading) {
                material.shading_model = "constant";
            } else if (shading == aiShadingMode_Gouraud) {
                material.shading_model = "lambert";
            }
        }

        if (ReadMaterialSurface(material.ambient, mat, aiTextureType_AMBIENT, AI_MATKEY_COLOR_AMBIENT))
            ++numTextures;
        if (ReadMaterialSurface(material.diffuse, mat, aiTextureType_DIFFUSE, AI_MATKEY_COLOR_DIFFUSE))
            ++numTextures;
        if (ReadMaterialSurface(material.specular, mat, aiTextureType_SPECULAR, AI_MATKEY_COLOR_SPECULAR))
            ++numTextures;
        if (ReadMaterialSurface(material.emissive, mat, aiTextureType_EMISSIVE, AI_MATKEY_COLOR_EMISSIVE))
            ++numTextures;
        if (ReadMaterialSurface(material.reflective, mat, aiTextureType_REFLECTION, AI_MATKEY_COLOR_REFLECTIVE))
            ++numTextures;
        if (ReadMaterialSurface(material.transparent, mat, aiTextureType_OPACITY, AI_MATKEY_COLOR_TRANSPARENT))
            ++numTextures;
        if (ReadMaterialSurface(material.normal, mat, aiTextureType_NORMALS, nullptr, 0, 0))
            ++numTextures;

        material.shininess.exist = mat.Get(AI_MATKEY_SHININESS, material.shininess.value) == aiReturn_SUCCESS;
        material.transparency.exist = mat.Get(AI_MATKEY_OPACITY, material.transparency.value) == aiReturn_SUCCESS;
        material.index_refraction.exist = mat.Get(AI_MATKEY_REFRACTI, material.index_refraction.value) == aiReturn_SUCCESS;
    }

    // output textures if present
    if (numTextures > 0) {
        mOutput << startstr << "<library_images>" << endstr;
        PushTag();
        for (const Material &mat : materials) {
            WriteImageEntry(mat.ambient, mat.id + "-ambient-image");
            WriteImageEntry(mat.diffuse, mat.id + "-diffuse-image");
            WriteImageEntry(mat.specular, mat.id + "-specular-image");
            WriteImageEntry(mat.emissive, mat.id + "-emission-image");
            WriteImageEntry(mat.reflective, mat.id + "-reflective-image");
            WriteImageEntry(mat.transparent, mat.id + "-transparent-image");
            WriteImageEntry(mat.normal, mat.id + "-normal-image");
        }
        PopTag();
        mOutput << startstr << "</library_images>" << endstr;
    }

    // output effects - those are the actual carriers of information
    if (!materials.empty()) {
        mOutput << startstr << "<library_effects>" << endstr;
        PushTag();
        for (const Material &mat : materials) {
            // this is so ridiculous it must be right
            mOutput << startstr << "<effect id=\"" << mat.id << "-fx\" name=\"" << mat.name << "\">" << endstr;
            PushTag();
            mOutput << startstr << "<profile_COMMON>" << endstr;
            PushTag();

            // write sampler- and surface params for the texture entries
            WriteTextureParamEntry(mat.emissive, "emission", mat.id);
            WriteTextureParamEntry(mat.ambient, "ambient", mat.id);
            WriteTextureParamEntry(mat.diffuse, "diffuse", mat.id);
            WriteTextureParamEntry(mat.specular, "specular", mat.id);
            WriteTextureParamEntry(mat.reflective, "reflective", mat.id);
            WriteTextureParamEntry(mat.transparent, "transparent", mat.id);
            WriteTextureParamEntry(mat.normal, "normal", mat.id);

            mOutput << startstr << "<technique sid=\"standard\">" << endstr;
            PushTag();
            mOutput << startstr << "<" << mat.shading_model << ">" << endstr;
            PushTag();

            WriteTextureColorEntry(mat.emissive, "emission", mat.id + "-emission-sampler");
            WriteTextureColorEntry(mat.ambient, "ambient", mat.id + "-ambient-sampler");
            WriteTextureColorEntry(mat.diffuse, "diffuse", mat.id + "-diffuse-sampler");
            WriteTextureColorEntry(mat.specular, "specular", mat.id + "-specular-sampler");
            WriteFloatEntry(mat.shininess, "shininess");
            WriteTextureColorEntry(mat.reflective, "reflective", mat.id + "-reflective-sampler");
            WriteTextureColorEntry(mat.transparent, "transparent", mat.id + "-transparent-sampler");
            WriteFloatEntry(mat.transparency, "transparency");
            WriteFloatEntry(mat.index_refraction, "index_of_refraction");

            if (!mat.normal.texture.empty()) {
                WriteTextureColorEntry(mat.normal, "bump", mat.id + "-normal-sampler");
            }

            PopTag();
            mOutput << startstr << "</" << mat.shading_model << ">" << endstr;
            PopTag();
            mOutput << startstr << "</technique>" << endstr;
            PopTag();
            mOutput << startstr << "</profile_COMMON>" << endstr;
            PopTag();
            mOutput << startstr << "</effect>" << endstr;
        }
        PopTag();
        mOutput << startstr << "</library_effects>" << endstr;

        // write materials - they're just effect references
        mOutput << startstr << "<library_materials>" << endstr;
        PushTag();
        for (std::vector<Material>::const_iterator it = materials.begin(); it != materials.end(); ++it) {
            const Material &mat = *it;
            mOutput << startstr << "<material id=\"" << mat.id << "\" name=\"" << mat.name << "\">" << endstr;
            PushTag();
            mOutput << startstr << "<instance_effect url=\"#" << mat.id << "-fx\"/>" << endstr;
            PopTag();
            mOutput << startstr << "</material>" << endstr;
        }
        PopTag();
        mOutput << startstr << "</library_materials>" << endstr;
    }
}

// ------------------------------------------------------------------------------------------------
// Writes the controller library
void ColladaExporter::WriteControllerLibrary() {
    mOutput << startstr << "<library_controllers>" << endstr;
    PushTag();

    for (size_t a = 0; a < mScene->mNumMeshes; ++a) {
        WriteController(a);
    }

    PopTag();
    mOutput << startstr << "</library_controllers>" << endstr;
}

// ------------------------------------------------------------------------------------------------
// Writes a skin controller of the given mesh
void ColladaExporter::WriteController(size_t pIndex) {
    const aiMesh *mesh = mScene->mMeshes[pIndex];
    // Is there a skin controller?
    if (mesh->mNumBones == 0 || mesh->mNumFaces == 0 || mesh->mNumVertices == 0)
        return;

    const std::string idstr = GetObjectUniqueId(AiObjectType::Mesh, pIndex);
    const std::string namestr = GetObjectName(AiObjectType::Mesh, pIndex);

    mOutput << startstr << "<controller id=\"" << idstr << "-skin\" ";
    mOutput << "name=\"skinCluster" << pIndex << "\">" << endstr;
    PushTag();

    mOutput << startstr << "<skin source=\"#" << idstr << "\">" << endstr;
    PushTag();

    // bind pose matrix
    mOutput << startstr << "<bind_shape_matrix>" << endstr;
    PushTag();

    // I think it is identity in general cases.
    aiMatrix4x4 mat;
    mOutput << startstr << mat.a1 << " " << mat.a2 << " " << mat.a3 << " " << mat.a4 << endstr;
    mOutput << startstr << mat.b1 << " " << mat.b2 << " " << mat.b3 << " " << mat.b4 << endstr;
    mOutput << startstr << mat.c1 << " " << mat.c2 << " " << mat.c3 << " " << mat.c4 << endstr;
    mOutput << startstr << mat.d1 << " " << mat.d2 << " " << mat.d3 << " " << mat.d4 << endstr;

    PopTag();
    mOutput << startstr << "</bind_shape_matrix>" << endstr;

    mOutput << startstr << "<source id=\"" << idstr << "-skin-joints\" name=\"" << namestr << "-skin-joints\">" << endstr;
    PushTag();

    mOutput << startstr << "<Name_array id=\"" << idstr << "-skin-joints-array\" count=\"" << mesh->mNumBones << "\">";

    for (size_t i = 0; i < mesh->mNumBones; ++i)
        mOutput << GetBoneUniqueId(mesh->mBones[i]) << ' ';

    mOutput << "</Name_array>" << endstr;

    mOutput << startstr << "<technique_common>" << endstr;
    PushTag();

    mOutput << startstr << "<accessor source=\"#" << idstr << "-skin-joints-array\" count=\"" << mesh->mNumBones << "\" stride=\"" << 1 << "\">" << endstr;
    PushTag();

    mOutput << startstr << "<param name=\"JOINT\" type=\"Name\"></param>" << endstr;

    PopTag();
    mOutput << startstr << "</accessor>" << endstr;

    PopTag();
    mOutput << startstr << "</technique_common>" << endstr;

    PopTag();
    mOutput << startstr << "</source>" << endstr;

    std::vector<ai_real> bind_poses;
    bind_poses.reserve(mesh->mNumBones * 16);
    for (unsigned int i = 0; i < mesh->mNumBones; ++i)
        for (unsigned int j = 0; j < 4; ++j)
            bind_poses.insert(bind_poses.end(), mesh->mBones[i]->mOffsetMatrix[j], mesh->mBones[i]->mOffsetMatrix[j] + 4);

    WriteFloatArray(idstr + "-skin-bind_poses", FloatType_Mat4x4, (const ai_real *)bind_poses.data(), bind_poses.size() / 16);

    bind_poses.clear();

    std::vector<ai_real> skin_weights;
    skin_weights.reserve(mesh->mNumVertices * mesh->mNumBones);
    for (size_t i = 0; i < mesh->mNumBones; ++i)
        for (size_t j = 0; j < mesh->mBones[i]->mNumWeights; ++j)
            skin_weights.push_back(mesh->mBones[i]->mWeights[j].mWeight);

    WriteFloatArray(idstr + "-skin-weights", FloatType_Weight, (const ai_real *)skin_weights.data(), skin_weights.size());

    skin_weights.clear();

    mOutput << startstr << "<joints>" << endstr;
    PushTag();

    mOutput << startstr << "<input semantic=\"JOINT\" source=\"#" << idstr << "-skin-joints\"></input>" << endstr;
    mOutput << startstr << "<input semantic=\"INV_BIND_MATRIX\" source=\"#" << idstr << "-skin-bind_poses\"></input>" << endstr;

    PopTag();
    mOutput << startstr << "</joints>" << endstr;

    mOutput << startstr << "<vertex_weights count=\"" << mesh->mNumVertices << "\">" << endstr;
    PushTag();

    mOutput << startstr << "<input semantic=\"JOINT\" source=\"#" << idstr << "-skin-joints\" offset=\"0\"></input>" << endstr;
    mOutput << startstr << "<input semantic=\"WEIGHT\" source=\"#" << idstr << "-skin-weights\" offset=\"1\"></input>" << endstr;

    mOutput << startstr << "<vcount>";

    std::vector<ai_uint> num_influences(mesh->mNumVertices, (ai_uint)0);
    for (size_t i = 0; i < mesh->mNumBones; ++i)
        for (size_t j = 0; j < mesh->mBones[i]->mNumWeights; ++j)
            ++num_influences[mesh->mBones[i]->mWeights[j].mVertexId];

    for (size_t i = 0; i < mesh->mNumVertices; ++i)
        mOutput << num_influences[i] << " ";

    mOutput << "</vcount>" << endstr;

    mOutput << startstr << "<v>";

    ai_uint joint_weight_indices_length = 0;
    std::vector<ai_uint> accum_influences;
    accum_influences.reserve(num_influences.size());
    for (size_t i = 0; i < num_influences.size(); ++i) {
        accum_influences.push_back(joint_weight_indices_length);
        joint_weight_indices_length += num_influences[i];
    }

    ai_uint weight_index = 0;
    std::vector<ai_int> joint_weight_indices(2 * joint_weight_indices_length, (ai_int)-1);
    for (unsigned int i = 0; i < mesh->mNumBones; ++i)
        for (unsigned j = 0; j < mesh->mBones[i]->mNumWeights; ++j) {
            unsigned int vId = mesh->mBones[i]->mWeights[j].mVertexId;
            for (ai_uint k = 0; k < num_influences[vId]; ++k) {
                if (joint_weight_indices[2 * (accum_influences[vId] + k)] == -1) {
                    joint_weight_indices[2 * (accum_influences[vId] + k)] = i;
                    joint_weight_indices[2 * (accum_influences[vId] + k) + 1] = weight_index;
                    break;
                }
            }
            ++weight_index;
        }

    for (size_t i = 0; i < joint_weight_indices.size(); ++i)
        mOutput << joint_weight_indices[i] << " ";

    num_influences.clear();
    accum_influences.clear();
    joint_weight_indices.clear();

    mOutput << "</v>" << endstr;

    PopTag();
    mOutput << startstr << "</vertex_weights>" << endstr;

    PopTag();
    mOutput << startstr << "</skin>" << endstr;

    PopTag();
    mOutput << startstr << "</controller>" << endstr;
}

// ------------------------------------------------------------------------------------------------
// Writes the geometry library
void ColladaExporter::WriteGeometryLibrary() {
    mOutput << startstr << "<library_geometries>" << endstr;
    PushTag();

    for (size_t a = 0; a < mScene->mNumMeshes; ++a)
        WriteGeometry(a);

    PopTag();
    mOutput << startstr << "</library_geometries>" << endstr;
}

// ------------------------------------------------------------------------------------------------
// Writes the given mesh
void ColladaExporter::WriteGeometry(size_t pIndex) {
    const aiMesh *mesh = mScene->mMeshes[pIndex];
    const std::string geometryId = GetObjectUniqueId(AiObjectType::Mesh, pIndex);
    const std::string geometryName = GetObjectName(AiObjectType::Mesh, pIndex);

    if (mesh->mNumFaces == 0 || mesh->mNumVertices == 0)
        return;

    // opening tag
    mOutput << startstr << "<geometry id=\"" << geometryId << "\" name=\"" << geometryName << "\" >" << endstr;
    PushTag();

    mOutput << startstr << "<mesh>" << endstr;
    PushTag();

    // Positions
    WriteFloatArray(geometryId + "-positions", FloatType_Vector, (ai_real *)mesh->mVertices, mesh->mNumVertices);
    // Normals, if any
    if (mesh->HasNormals())
        WriteFloatArray(geometryId + "-normals", FloatType_Vector, (ai_real *)mesh->mNormals, mesh->mNumVertices);

    // texture coords
    for (size_t a = 0; a < AI_MAX_NUMBER_OF_TEXTURECOORDS; ++a) {
        if (mesh->HasTextureCoords(static_cast<unsigned int>(a))) {
            WriteFloatArray(geometryId + "-tex" + ai_to_string(a), mesh->mNumUVComponents[a] == 3 ? FloatType_TexCoord3 : FloatType_TexCoord2,
                    (ai_real *)mesh->mTextureCoords[a], mesh->mNumVertices);
        }
    }

    // vertex colors
    for (size_t a = 0; a < AI_MAX_NUMBER_OF_TEXTURECOORDS; ++a) {
        if (mesh->HasVertexColors(static_cast<unsigned int>(a)))
            WriteFloatArray(geometryId + "-color" + ai_to_string(a), FloatType_Color, (ai_real *)mesh->mColors[a], mesh->mNumVertices);
    }

    // assemble vertex structure
    // Only write input for POSITION since we will write other as shared inputs in polygon definition
    mOutput << startstr << "<vertices id=\"" << geometryId << "-vertices"
            << "\">" << endstr;
    PushTag();
    mOutput << startstr << "<input semantic=\"POSITION\" source=\"#" << geometryId << "-positions\" />" << endstr;
    PopTag();
    mOutput << startstr << "</vertices>" << endstr;

    // count the number of lines, triangles and polygon meshes
    int countLines = 0;
    int countPoly = 0;
    for (size_t a = 0; a < mesh->mNumFaces; ++a) {
        if (mesh->mFaces[a].mNumIndices == 2)
            countLines++;
        else if (mesh->mFaces[a].mNumIndices >= 3)
            countPoly++;
    }

    // lines
    if (countLines) {
        mOutput << startstr << "<lines count=\"" << countLines << "\" material=\"defaultMaterial\">" << endstr;
        PushTag();
        mOutput << startstr << "<input offset=\"0\" semantic=\"VERTEX\" source=\"#" << geometryId << "-vertices\" />" << endstr;
        if (mesh->HasNormals())
            mOutput << startstr << "<input semantic=\"NORMAL\" source=\"#" << geometryId << "-normals\" />" << endstr;
        for (size_t a = 0; a < AI_MAX_NUMBER_OF_TEXTURECOORDS; ++a) {
            if (mesh->HasTextureCoords(static_cast<unsigned int>(a)))
                mOutput << startstr << "<input semantic=\"TEXCOORD\" source=\"#" << geometryId << "-tex" << a << "\" "
                        << "set=\"" << a << "\""
                        << " />" << endstr;
        }
        for (size_t a = 0; a < AI_MAX_NUMBER_OF_COLOR_SETS; ++a) {
            if (mesh->HasVertexColors(static_cast<unsigned int>(a)))
                mOutput << startstr << "<input semantic=\"COLOR\" source=\"#" << geometryId << "-color" << a << "\" "
                        << "set=\"" << a << "\""
                        << " />" << endstr;
        }

        mOutput << startstr << "<p>";
        for (size_t a = 0; a < mesh->mNumFaces; ++a) {
            const aiFace &face = mesh->mFaces[a];
            if (face.mNumIndices != 2) continue;
            for (size_t b = 0; b < face.mNumIndices; ++b)
                mOutput << face.mIndices[b] << " ";
        }
        mOutput << "</p>" << endstr;
        PopTag();
        mOutput << startstr << "</lines>" << endstr;
    }

    // triangle - don't use it, because compatibility problems

    // polygons
    if (countPoly) {
        mOutput << startstr << "<polylist count=\"" << countPoly << "\" material=\"defaultMaterial\">" << endstr;
        PushTag();
        mOutput << startstr << "<input offset=\"0\" semantic=\"VERTEX\" source=\"#" << geometryId << "-vertices\" />" << endstr;
        if (mesh->HasNormals())
            mOutput << startstr << "<input offset=\"0\" semantic=\"NORMAL\" source=\"#" << geometryId << "-normals\" />" << endstr;
        for (size_t a = 0; a < AI_MAX_NUMBER_OF_TEXTURECOORDS; ++a) {
            if (mesh->HasTextureCoords(static_cast<unsigned int>(a)))
                mOutput << startstr << "<input offset=\"0\" semantic=\"TEXCOORD\" source=\"#" << geometryId << "-tex" << a << "\" "
                        << "set=\"" << a << "\""
                        << " />" << endstr;
        }
        for (size_t a = 0; a < AI_MAX_NUMBER_OF_COLOR_SETS; ++a) {
            if (mesh->HasVertexColors(static_cast<unsigned int>(a)))
                mOutput << startstr << "<input offset=\"0\" semantic=\"COLOR\" source=\"#" << geometryId << "-color" << a << "\" "
                        << "set=\"" << a << "\""
                        << " />" << endstr;
        }

        mOutput << startstr << "<vcount>";
        for (size_t a = 0; a < mesh->mNumFaces; ++a) {
            if (mesh->mFaces[a].mNumIndices < 3) continue;
            mOutput << mesh->mFaces[a].mNumIndices << " ";
        }
        mOutput << "</vcount>" << endstr;

        mOutput << startstr << "<p>";
        for (size_t a = 0; a < mesh->mNumFaces; ++a) {
            const aiFace &face = mesh->mFaces[a];
            if (face.mNumIndices < 3) continue;
            for (size_t b = 0; b < face.mNumIndices; ++b)
                mOutput << face.mIndices[b] << " ";
        }
        mOutput << "</p>" << endstr;
        PopTag();
        mOutput << startstr << "</polylist>" << endstr;
    }

    // closing tags
    PopTag();
    mOutput << startstr << "</mesh>" << endstr;
    PopTag();
    mOutput << startstr << "</geometry>" << endstr;
}

// ------------------------------------------------------------------------------------------------
// Writes a float array of the given type
void ColladaExporter::WriteFloatArray(const std::string &pIdString, FloatDataType pType, const ai_real *pData, size_t pElementCount) {
    size_t floatsPerElement = 0;
    switch (pType) {
    case FloatType_Vector: floatsPerElement = 3; break;
    case FloatType_TexCoord2: floatsPerElement = 2; break;
    case FloatType_TexCoord3: floatsPerElement = 3; break;
    case FloatType_Color: floatsPerElement = 3; break;
    case FloatType_Mat4x4: floatsPerElement = 16; break;
    case FloatType_Weight: floatsPerElement = 1; break;
    case FloatType_Time: floatsPerElement = 1; break;
    default:
        return;
    }

    std::string arrayId = XMLIDEncode(pIdString) + "-array";

    mOutput << startstr << "<source id=\"" << XMLIDEncode(pIdString) << "\" name=\"" << XMLEscape(pIdString) << "\">" << endstr;
    PushTag();

    // source array
    mOutput << startstr << "<float_array id=\"" << arrayId << "\" count=\"" << pElementCount * floatsPerElement << "\"> ";
    PushTag();

    if (pType == FloatType_TexCoord2) {
        for (size_t a = 0; a < pElementCount; ++a) {
            mOutput << pData[a * 3 + 0] << " ";
            mOutput << pData[a * 3 + 1] << " ";
        }
    } else if (pType == FloatType_Color) {
        for (size_t a = 0; a < pElementCount; ++a) {
            mOutput << pData[a * 4 + 0] << " ";
            mOutput << pData[a * 4 + 1] << " ";
            mOutput << pData[a * 4 + 2] << " ";
        }
    } else {
        for (size_t a = 0; a < pElementCount * floatsPerElement; ++a)
            mOutput << pData[a] << " ";
    }
    mOutput << "</float_array>" << endstr;
    PopTag();

    // the usual Collada fun. Let's bloat it even more!
    mOutput << startstr << "<technique_common>" << endstr;
    PushTag();
    mOutput << startstr << "<accessor count=\"" << pElementCount << "\" offset=\"0\" source=\"#" << arrayId << "\" stride=\"" << floatsPerElement << "\">" << endstr;
    PushTag();

    switch (pType) {
    case FloatType_Vector:
        mOutput << startstr << "<param name=\"X\" type=\"float\" />" << endstr;
        mOutput << startstr << "<param name=\"Y\" type=\"float\" />" << endstr;
        mOutput << startstr << "<param name=\"Z\" type=\"float\" />" << endstr;
        break;

    case FloatType_TexCoord2:
        mOutput << startstr << "<param name=\"S\" type=\"float\" />" << endstr;
        mOutput << startstr << "<param name=\"T\" type=\"float\" />" << endstr;
        break;

    case FloatType_TexCoord3:
        mOutput << startstr << "<param name=\"S\" type=\"float\" />" << endstr;
        mOutput << startstr << "<param name=\"T\" type=\"float\" />" << endstr;
        mOutput << startstr << "<param name=\"P\" type=\"float\" />" << endstr;
        break;

    case FloatType_Color:
        mOutput << startstr << "<param name=\"R\" type=\"float\" />" << endstr;
        mOutput << startstr << "<param name=\"G\" type=\"float\" />" << endstr;
        mOutput << startstr << "<param name=\"B\" type=\"float\" />" << endstr;
        break;

    case FloatType_Mat4x4:
        mOutput << startstr << "<param name=\"TRANSFORM\" type=\"float4x4\" />" << endstr;
        break;

    case FloatType_Weight:
        mOutput << startstr << "<param name=\"WEIGHT\" type=\"float\" />" << endstr;
        break;

    // customized, add animation related
    case FloatType_Time:
        mOutput << startstr << "<param name=\"TIME\" type=\"float\" />" << endstr;
        break;
    }

    PopTag();
    mOutput << startstr << "</accessor>" << endstr;
    PopTag();
    mOutput << startstr << "</technique_common>" << endstr;
    PopTag();
    mOutput << startstr << "</source>" << endstr;
}

// ------------------------------------------------------------------------------------------------
// Writes the scene library
void ColladaExporter::WriteSceneLibrary() {
    // Determine if we are using the aiScene root or our own
    std::string sceneName("Scene");
    if (mAdd_root_node) {
        mSceneId = MakeUniqueId(mUniqueIds, sceneName, std::string());
        mUniqueIds.insert(mSceneId);
    } else {
        mSceneId = GetNodeUniqueId(mScene->mRootNode);
        sceneName = GetNodeName(mScene->mRootNode);
    }

    mOutput << startstr << "<library_visual_scenes>" << endstr;
    PushTag();
    mOutput << startstr << "<visual_scene id=\"" + mSceneId + "\" name=\"" + sceneName + "\">" << endstr;
    PushTag();

    if (mAdd_root_node) {
        // Export the root node
        WriteNode(mScene->mRootNode);
    } else {
        // Have already exported the root node
        for (size_t a = 0; a < mScene->mRootNode->mNumChildren; ++a)
            WriteNode(mScene->mRootNode->mChildren[a]);
    }

    PopTag();
    mOutput << startstr << "</visual_scene>" << endstr;
    PopTag();
    mOutput << startstr << "</library_visual_scenes>" << endstr;
}
// ------------------------------------------------------------------------------------------------
void ColladaExporter::WriteAnimationLibrary(size_t pIndex) {
    const aiAnimation *anim = mScene->mAnimations[pIndex];

    if (anim->mNumChannels == 0 && anim->mNumMeshChannels == 0 && anim->mNumMorphMeshChannels == 0)
        return;

    const std::string animationNameEscaped = GetObjectName(AiObjectType::Animation, pIndex);
    const std::string idstrEscaped = GetObjectUniqueId(AiObjectType::Animation, pIndex);

    mOutput << startstr << "<animation id=\"" + idstrEscaped + "\" name=\"" + animationNameEscaped + "\">" << endstr;
    PushTag();

    std::string cur_node_idstr;
    for (size_t a = 0; a < anim->mNumChannels; ++a) {
        const aiNodeAnim *nodeAnim = anim->mChannels[a];

        // sanity check
        if (nodeAnim->mNumPositionKeys != nodeAnim->mNumScalingKeys || nodeAnim->mNumPositionKeys != nodeAnim->mNumRotationKeys) {
            continue;
        }

        {
            cur_node_idstr.clear();
            cur_node_idstr += nodeAnim->mNodeName.data;
            cur_node_idstr += std::string("_matrix-input");

            std::vector<ai_real> frames;
            for (size_t i = 0; i < nodeAnim->mNumPositionKeys; ++i) {
                frames.push_back(static_cast<ai_real>(nodeAnim->mPositionKeys[i].mTime));
            }

            WriteFloatArray(cur_node_idstr, FloatType_Time, (const ai_real *)frames.data(), frames.size());
            frames.clear();
        }

        {
            cur_node_idstr.clear();

            cur_node_idstr += nodeAnim->mNodeName.data;
            cur_node_idstr += std::string("_matrix-output");

            std::vector<ai_real> keyframes;
            keyframes.reserve(nodeAnim->mNumPositionKeys * 16);
            for (size_t i = 0; i < nodeAnim->mNumPositionKeys; ++i) {
                aiVector3D Scaling = nodeAnim->mScalingKeys[i].mValue;
                aiMatrix4x4 ScalingM; // identity
                ScalingM[0][0] = Scaling.x;
                ScalingM[1][1] = Scaling.y;
                ScalingM[2][2] = Scaling.z;

                aiQuaternion RotationQ = nodeAnim->mRotationKeys[i].mValue;
                aiMatrix4x4 s = aiMatrix4x4(RotationQ.GetMatrix());
                aiMatrix4x4 RotationM(s.a1, s.a2, s.a3, 0, s.b1, s.b2, s.b3, 0, s.c1, s.c2, s.c3, 0, 0, 0, 0, 1);

                aiVector3D Translation = nodeAnim->mPositionKeys[i].mValue;
                aiMatrix4x4 TranslationM; // identity
                TranslationM[0][3] = Translation.x;
                TranslationM[1][3] = Translation.y;
                TranslationM[2][3] = Translation.z;

                // Combine the above transformations
                aiMatrix4x4 mat = TranslationM * RotationM * ScalingM;

                for (unsigned int j = 0; j < 4; ++j) {
                    keyframes.insert(keyframes.end(), mat[j], mat[j] + 4);
                }
            }

            WriteFloatArray(cur_node_idstr, FloatType_Mat4x4, (const ai_real *)keyframes.data(), keyframes.size() / 16);
        }

        {
            std::vector<std::string> names;
            for (size_t i = 0; i < nodeAnim->mNumPositionKeys; ++i) {
                if (nodeAnim->mPreState == aiAnimBehaviour_DEFAULT || nodeAnim->mPreState == aiAnimBehaviour_LINEAR || nodeAnim->mPreState == aiAnimBehaviour_REPEAT) {
                    names.emplace_back("LINEAR");
                } else if (nodeAnim->mPostState == aiAnimBehaviour_CONSTANT) {
                    names.emplace_back("STEP");
                }
            }

            const std::string cur_node_idstr2 = nodeAnim->mNodeName.data + std::string("_matrix-interpolation");
            std::string arrayId = XMLIDEncode(cur_node_idstr2) + "-array";

            mOutput << startstr << "<source id=\"" << XMLIDEncode(cur_node_idstr2) << "\">" << endstr;
            PushTag();

            // source array
            mOutput << startstr << "<Name_array id=\"" << arrayId << "\" count=\"" << names.size() << "\"> ";
            for (size_t aa = 0; aa < names.size(); ++aa) {
                mOutput << names[aa] << " ";
            }
            mOutput << "</Name_array>" << endstr;

            mOutput << startstr << "<technique_common>" << endstr;
            PushTag();

            mOutput << startstr << "<accessor source=\"#" << arrayId << "\" count=\"" << names.size() << "\" stride=\"" << 1 << "\">" << endstr;
            PushTag();

            mOutput << startstr << "<param name=\"INTERPOLATION\" type=\"name\"></param>" << endstr;

            PopTag();
            mOutput << startstr << "</accessor>" << endstr;

            PopTag();
            mOutput << startstr << "</technique_common>" << endstr;

            PopTag();
            mOutput << startstr << "</source>" << endstr;
        }
    }

    for (size_t a = 0; a < anim->mNumChannels; ++a) {
        const aiNodeAnim *nodeAnim = anim->mChannels[a];

        {
            // samplers
            const std::string node_idstr = nodeAnim->mNodeName.data + std::string("_matrix-sampler");
            mOutput << startstr << "<sampler id=\"" << XMLIDEncode(node_idstr) << "\">" << endstr;
            PushTag();

            mOutput << startstr << "<input semantic=\"INPUT\" source=\"#" << XMLIDEncode(nodeAnim->mNodeName.data + std::string("_matrix-input")) << "\"/>" << endstr;
            mOutput << startstr << "<input semantic=\"OUTPUT\" source=\"#" << XMLIDEncode(nodeAnim->mNodeName.data + std::string("_matrix-output")) << "\"/>" << endstr;
            mOutput << startstr << "<input semantic=\"INTERPOLATION\" source=\"#" << XMLIDEncode(nodeAnim->mNodeName.data + std::string("_matrix-interpolation")) << "\"/>" << endstr;

            PopTag();
            mOutput << startstr << "</sampler>" << endstr;
        }
    }

    for (size_t a = 0; a < anim->mNumChannels; ++a) {
        const aiNodeAnim *nodeAnim = anim->mChannels[a];

        {
            // channels
            mOutput << startstr << "<channel source=\"#" << XMLIDEncode(nodeAnim->mNodeName.data + std::string("_matrix-sampler")) << "\" target=\"" << XMLIDEncode(nodeAnim->mNodeName.data) << "/matrix\"/>" << endstr;
        }
    }

    PopTag();
    mOutput << startstr << "</animation>" << endstr;
}
// ------------------------------------------------------------------------------------------------
void ColladaExporter::WriteAnimationsLibrary() {
    if (mScene->mNumAnimations > 0) {
        mOutput << startstr << "<library_animations>" << endstr;
        PushTag();

        // start recursive write at the root node
        for (size_t a = 0; a < mScene->mNumAnimations; ++a)
            WriteAnimationLibrary(a);

        PopTag();
        mOutput << startstr << "</library_animations>" << endstr;
    }
}
// ------------------------------------------------------------------------------------------------
// Helper to find a bone by name in the scene
aiBone *findBone(const aiScene *scene, const aiString &name) {
    for (size_t m = 0; m < scene->mNumMeshes; m++) {
        aiMesh *mesh = scene->mMeshes[m];
        for (size_t b = 0; b < mesh->mNumBones; b++) {
            aiBone *bone = mesh->mBones[b];
            if (name == bone->mName) {
                return bone;
            }
        }
    }
    return nullptr;
}

// ------------------------------------------------------------------------------------------------
// Helper to find the node associated with a bone in the scene
const aiNode *findBoneNode(const aiNode *aNode, const aiBone *bone) {
    if (aNode && bone && aNode->mName == bone->mName) {
        return aNode;
    }

    if (aNode && bone) {
        for (unsigned int i = 0; i < aNode->mNumChildren; ++i) {
            aiNode *aChild = aNode->mChildren[i];
            const aiNode *foundFromChild = nullptr;
            if (aChild) {
                foundFromChild = findBoneNode(aChild, bone);
                if (foundFromChild) {
                    return foundFromChild;
                }
            }
        }
    }

    return nullptr;
}

const aiNode *findSkeletonRootNode(const aiScene *scene, const aiMesh *mesh) {
    std::set<const aiNode *> topParentBoneNodes;
    if (mesh && mesh->mNumBones > 0) {
        for (unsigned int i = 0; i < mesh->mNumBones; ++i) {
            aiBone *bone = mesh->mBones[i];

            const aiNode *node = findBoneNode(scene->mRootNode, bone);
            if (node) {
                while (node->mParent && findBone(scene, node->mParent->mName) != nullptr) {
                    node = node->mParent;
                }
                topParentBoneNodes.insert(node);
            }
        }
    }

    if (!topParentBoneNodes.empty()) {
        const aiNode *parentBoneNode = *topParentBoneNodes.begin();
        if (topParentBoneNodes.size() == 1) {
            return parentBoneNode;
        } else {
            for (auto it : topParentBoneNodes) {
                if (it->mParent) return it->mParent;
            }
            return parentBoneNode;
        }
    }

    return nullptr;
}

// ------------------------------------------------------------------------------------------------
// Recursively writes the given node
void ColladaExporter::WriteNode(const aiNode *pNode) {
    // If the node is associated with a bone, it is a joint node (JOINT)
    // otherwise it is a normal node (NODE)
    // Assimp-specific: nodes with no name cannot be associated with bones
    const char *node_type;
    bool is_joint, is_skeleton_root = false;
    if (pNode->mName.length == 0 || nullptr == findBone(mScene, pNode->mName)) {
        node_type = "NODE";
        is_joint = false;
    } else {
        node_type = "JOINT";
        is_joint = true;
        if (!pNode->mParent || nullptr == findBone(mScene, pNode->mParent->mName)) {
            is_skeleton_root = true;
        }
    }

    const std::string node_id = GetNodeUniqueId(pNode);
    const std::string node_name = GetNodeName(pNode);
    mOutput << startstr << "<node ";
    if (is_skeleton_root) {
        mFoundSkeletonRootNodeID = node_id; // For now, only support one skeleton in a scene.
    }
    mOutput << "id=\"" << node_id << "\" " << (is_joint ? "sid=\"" + node_id + "\" " : "");
    mOutput << "name=\"" << node_name
            << "\" type=\"" << node_type
            << "\">" << endstr;
    PushTag();

    // write transformation - we can directly put the matrix there
    // TODO: (thom) decompose into scale - rot - quad to allow addressing it by animations afterwards
    aiMatrix4x4 mat = pNode->mTransformation;

    // If this node is a Camera node, the camera coordinate system needs to be multiplied in.
    // When importing from Collada, the mLookAt is set to 0, 0, -1, and the node transform is unchanged.
    // When importing from a different format, mLookAt is set to 0, 0, 1. Therefore, the local camera
    // coordinate system must be changed to matche the Collada specification.
    for (size_t i = 0; i < mScene->mNumCameras; i++) {
        if (mScene->mCameras[i]->mName == pNode->mName) {
            aiMatrix4x4 sourceView;
            mScene->mCameras[i]->GetCameraMatrix(sourceView);

            aiMatrix4x4 colladaView;
            colladaView.a1 = colladaView.c3 = -1; // move into -z space.
            mat *= (sourceView * colladaView);
            break;
        }
    }

    // customized, sid should be 'matrix' to match with loader code.
    //mOutput << startstr << "<matrix sid=\"transform\">";
    mOutput << startstr << "<matrix sid=\"matrix\">";

    mOutput << mat.a1 << " " << mat.a2 << " " << mat.a3 << " " << mat.a4 << " ";
    mOutput << mat.b1 << " " << mat.b2 << " " << mat.b3 << " " << mat.b4 << " ";
    mOutput << mat.c1 << " " << mat.c2 << " " << mat.c3 << " " << mat.c4 << " ";
    mOutput << mat.d1 << " " << mat.d2 << " " << mat.d3 << " " << mat.d4;
    mOutput << "</matrix>" << endstr;

    if (pNode->mNumMeshes == 0) {
        //check if it is a camera node
        for (size_t i = 0; i < mScene->mNumCameras; i++) {
            if (mScene->mCameras[i]->mName == pNode->mName) {
                mOutput << startstr << "<instance_camera url=\"#" << GetObjectUniqueId(AiObjectType::Camera, i) << "\"/>" << endstr;
                break;
            }
        }
        //check if it is a light node
        for (size_t i = 0; i < mScene->mNumLights; i++) {
            if (mScene->mLights[i]->mName == pNode->mName) {
                mOutput << startstr << "<instance_light url=\"#" << GetObjectUniqueId(AiObjectType::Light, i) << "\"/>" << endstr;
                break;
            }
        }

    } else
        // instance every geometry
        for (size_t a = 0; a < pNode->mNumMeshes; ++a) {
            const aiMesh *mesh = mScene->mMeshes[pNode->mMeshes[a]];
            // do not instantiate mesh if empty. I wonder how this could happen
            if (mesh->mNumFaces == 0 || mesh->mNumVertices == 0)
                continue;

            const std::string meshId = GetObjectUniqueId(AiObjectType::Mesh, pNode->mMeshes[a]);

            if (mesh->mNumBones == 0) {
                mOutput << startstr << "<instance_geometry url=\"#" << meshId << "\">" << endstr;
                PushTag();
            } else {
                mOutput << startstr
                        << "<instance_controller url=\"#" << meshId << "-skin\">"
                        << endstr;
                PushTag();

                // note! this mFoundSkeletonRootNodeID some how affects animation, it makes the mesh attaches to armature skeleton root node.
                // use the first bone to find skeleton root
                const aiNode *skeletonRootBoneNode = findSkeletonRootNode(mScene, mesh);
                if (skeletonRootBoneNode) {
                    mFoundSkeletonRootNodeID = GetNodeUniqueId(skeletonRootBoneNode);
                }
                mOutput << startstr << "<skeleton>#" << mFoundSkeletonRootNodeID << "</skeleton>" << endstr;
            }
            mOutput << startstr << "<bind_material>" << endstr;
            PushTag();
            mOutput << startstr << "<technique_common>" << endstr;
            PushTag();
            mOutput << startstr << "<instance_material symbol=\"defaultMaterial\" target=\"#" << GetObjectUniqueId(AiObjectType::Material, mesh->mMaterialIndex) << "\">" << endstr;
            PushTag();
            for (size_t aa = 0; aa < AI_MAX_NUMBER_OF_TEXTURECOORDS; ++aa) {
                if (mesh->HasTextureCoords(static_cast<unsigned int>(aa)))
                    // semantic       as in <texture texcoord=...>
                    // input_semantic as in <input semantic=...>
                    // input_set      as in <input set=...>
                    mOutput << startstr << "<bind_vertex_input semantic=\"CHANNEL" << aa << "\" input_semantic=\"TEXCOORD\" input_set=\"" << aa << "\"/>" << endstr;
            }
            PopTag();
            mOutput << startstr << "</instance_material>" << endstr;
            PopTag();
            mOutput << startstr << "</technique_common>" << endstr;
            PopTag();
            mOutput << startstr << "</bind_material>" << endstr;

            PopTag();
            if (mesh->mNumBones == 0)
                mOutput << startstr << "</instance_geometry>" << endstr;
            else
                mOutput << startstr << "</instance_controller>" << endstr;
        }

    // recurse into subnodes
    for (size_t a = 0; a < pNode->mNumChildren; ++a)
        WriteNode(pNode->mChildren[a]);

    PopTag();
    mOutput << startstr << "</node>" << endstr;
}

void ColladaExporter::CreateNodeIds(const aiNode *node) {
    GetNodeUniqueId(node);
    for (size_t a = 0; a < node->mNumChildren; ++a)
        CreateNodeIds(node->mChildren[a]);
}

std::string ColladaExporter::GetNodeUniqueId(const aiNode *node) {
    // Use the pointer as the key. This is safe because the scene is immutable.
    auto idIt = mNodeIdMap.find(node);
    if (idIt != mNodeIdMap.cend())
        return idIt->second;

    // Prefer the requested Collada Id if extant
    std::string idStr;
    aiString origId;
    if (node->mMetaData && node->mMetaData->Get(AI_METADATA_COLLADA_ID, origId)) {
        idStr = origId.C_Str();
    } else {
        idStr = node->mName.C_Str();
    }
    // Make sure the requested id is valid
    if (idStr.empty())
        idStr = "node";
    else
        idStr = XMLIDEncode(idStr);

    // Ensure it's unique
    idStr = MakeUniqueId(mUniqueIds, idStr, std::string());
    mUniqueIds.insert(idStr);
    mNodeIdMap.insert(std::make_pair(node, idStr));
    return idStr;
}

std::string ColladaExporter::GetNodeName(const aiNode *node) {

    return XMLEscape(node->mName.C_Str());
}

std::string ColladaExporter::GetBoneUniqueId(const aiBone *bone) {
    // Find the Node that is this Bone
    const aiNode *boneNode = findBoneNode(mScene->mRootNode, bone);
    if (boneNode == nullptr)
        return std::string();

    return GetNodeUniqueId(boneNode);
}

std::string ColladaExporter::GetObjectUniqueId(AiObjectType type, size_t pIndex) {
    auto idIt = GetObjectIdMap(type).find(pIndex);
    if (idIt != GetObjectIdMap(type).cend())
        return idIt->second;

    // Not seen this object before, create and add
    NameIdPair result = AddObjectIndexToMaps(type, pIndex);
    return result.second;
}

std::string ColladaExporter::GetObjectName(AiObjectType type, size_t pIndex) {
    auto objectName = GetObjectNameMap(type).find(pIndex);
    if (objectName != GetObjectNameMap(type).cend())
        return objectName->second;

    // Not seen this object before, create and add
    NameIdPair result = AddObjectIndexToMaps(type, pIndex);
    return result.first;
}

// Determine unique id and add the name and id to the maps
// @param type object type
// @param index object index
// @param name in/out. Caller to set the original name if known.
// @param idStr in/out. Caller to set the preferred id if known.
ColladaExporter::NameIdPair ColladaExporter::AddObjectIndexToMaps(AiObjectType type, size_t index) {

    std::string name;
    std::string idStr;
    std::string idPostfix;

    // Get the name and id postfix
    switch (type) {
    case AiObjectType::Mesh: name = mScene->mMeshes[index]->mName.C_Str(); break;
    case AiObjectType::Material: name = mScene->mMaterials[index]->GetName().C_Str(); break;
    case AiObjectType::Animation: name = mScene->mAnimations[index]->mName.C_Str(); break;
    case AiObjectType::Light:
        name = mScene->mLights[index]->mName.C_Str();
        idPostfix = "-light";
        break;
    case AiObjectType::Camera:
        name = mScene->mCameras[index]->mName.C_Str();
        idPostfix = "-camera";
        break;
    case AiObjectType::Count: throw std::logic_error("ColladaExporter::AiObjectType::Count is not an object type");
    }

    if (name.empty()) {
        // Default ids if empty name
        switch (type) {
        case AiObjectType::Mesh: idStr = std::string("mesh_"); break;
        case AiObjectType::Material: idStr = std::string("material_"); break; // This one should never happen
        case AiObjectType::Animation: idStr = std::string("animation_"); break;
        case AiObjectType::Light: idStr = std::string("light_"); break;
        case AiObjectType::Camera: idStr = std::string("camera_"); break;
        case AiObjectType::Count: throw std::logic_error("ColladaExporter::AiObjectType::Count is not an object type");
        }
        idStr.append(ai_to_string(index));
    } else {
        idStr = XMLIDEncode(name);
    }

    if (!name.empty())
        name = XMLEscape(name);

    idStr = MakeUniqueId(mUniqueIds, idStr, idPostfix);

    // Add to maps
    mUniqueIds.insert(idStr);
    GetObjectIdMap(type).insert(std::make_pair(index, idStr));
    GetObjectNameMap(type).insert(std::make_pair(index, name));

    return std::make_pair(name, idStr);
}

} // end of namespace Assimp

#endif
#endif
