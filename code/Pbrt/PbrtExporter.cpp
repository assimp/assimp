/*
Open Asset Import Library (assimp)
----------------------------------------------------------------------

Copyright (c) 2006-2020, assimp team

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
#ifndef ASSIMP_BUILD_NO_PBRT_EXPORTER

#include "PbrtExporter.h"

#include <assimp/version.h> // aiGetVersion
#include <assimp/DefaultIOSystem.h>
#include <assimp/IOSystem.hpp>
#include <assimp/Exporter.hpp>
#include <assimp/DefaultLogger.hpp>
#include <assimp/StreamWriter.h> // StreamWriterLE
#include <assimp/Exceptional.h> // DeadlyExportError
#include <assimp/material.h> // aiTextureType
#include <assimp/scene.h>
#include <assimp/mesh.h>

// Header files, standard library.
#include <memory> // shared_ptr
#include <string>
#include <sstream> // stringstream
#include <ctime> // localtime, tm_*
#include <map>
#include <set>
#include <vector>
#include <array>
#include <unordered_set>
#include <numeric>
#include <iostream>

using namespace Assimp;

// some constants that we'll use for writing metadata
namespace Assimp {

// ---------------------------------------------------------------------
// Worker function for exporting a scene to ascii pbrt.
// Prototyped and registered in Exporter.cpp
void ExportScenePbrt (
    const char* pFile,
    IOSystem* pIOSystem,
    const aiScene* pScene,
    const ExportProperties* pProperties
){
    std::string path = DefaultIOSystem::absolutePath(std::string(pFile));
    std::string file = DefaultIOSystem::completeBaseName(std::string(pFile));
        
    // initialize the exporter
    PbrtExporter exporter(pScene, pIOSystem, path, file);
}

} // end of namespace Assimp

// Constructor
PbrtExporter::PbrtExporter (
    const aiScene* pScene, IOSystem* pIOSystem,
    const std::string path, const std::string file)
: mScene(pScene),
  mIOSystem(pIOSystem),
  mPath(path),
  mFile(file)
{
    std::unique_ptr<IOStream> outfile;

    // Open the indicated file for writing
    outfile.reset(mIOSystem->Open(mPath,"wt"));
    if (!outfile) {
        throw DeadlyExportError(
            "could not open output .pbrt file: " + std::string(mFile)
        );
    }

    // Write Header
    WriteHeader();

    // Write metadata to file
    WriteMetaData();

    // Write scene-wide rendering options
    WriteSceneWide();

    // Write the Shapes
    WriteShapes();

    // Write World Description
    WriteWorldDefinition();

    // Write out to file
    outfile->Write(mOutput.str().c_str(), 
            mOutput.str().length(), 1);
    
    // explicitly release file pointer,
    // so we don't have to rely on class destruction.
    outfile.reset();

    // TODO Do Animation
}

// Destructor
PbrtExporter::~PbrtExporter() {
    // Empty
}

void PbrtExporter::WriteHeader() {
    // TODO

    // TODO warn user if scene has animations
    // TODO warn user if mScene->mFlags is nonzero
    // TODO warn if a metadata defines the ambient term
}

void PbrtExporter::WriteMetaData() {
    mOutput << "#############################" << std::endl;
    mOutput << "# Writing out scene metadata:" << std::endl;
    mOutput << "#############################" << std::endl;
    aiMetadata* pMetaData = mScene->mMetaData;
    for (int i = 0; i < pMetaData->mNumProperties; i++) {
        mOutput << "# - ";
        mOutput << pMetaData->mKeys[i].C_Str() << " :";
        switch(pMetaData->mValues[i].mType) {
            case AI_BOOL : {
                mOutput << " ";
                if (*static_cast<bool*>(pMetaData->mValues[i].mData))
                    mOutput << "TRUE" << std::endl;
                else
                    mOutput << "FALSE" << std::endl;
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
                std::size_t found = svalue.find_first_of("\n");
                mOutput << std::endl;
                while (found != std::string::npos) {
                    mOutput << "#     " << svalue.substr(0, found) << std::endl;
                    svalue = svalue.substr(found + 1);
                    found = svalue.find_first_of("\n");
                }
                mOutput << "#     " << svalue << std::endl;
                break;
            }
            case AI_AIVECTOR3D :
                // TODO
                mOutput << " Vector3D (unable to print)" << std::endl;
                break;
            default:
                // AI_META_MAX and FORCE_32BIT
                mOutput << " META_MAX or FORCE_32Bit (unable to print)" << std::endl;
                break;
        }
    }
}

void PbrtExporter::WriteSceneWide() {
    // If there are 0 cameras in the scene, it is purely geometric
    //   Don't write any scene wide description
    if (mScene->mNumCameras == 0)
        return;
    
    // Cameras & Film
    WriteCameras();

    mOutput << std::endl;
    mOutput << "#####################################################################" << std::endl;
    mOutput << "# Assimp does not support explicit Sampler, Filter, Integrator, Accel" << std::endl;
    mOutput << "#####################################################################" << std::endl;
    mOutput << "# Setting to reasonable default values" << std::endl;

    // Samplers
    mOutput << "Sampler \"halton\" \"integer pixelsamples\" [16]" << std::endl;
   
    // Filters
    mOutput << "PixelFilter \"box\"" << std::endl;

    // Integrators
    mOutput << "Integrator \"path\" \"integer maxdepth\" [5]" << std::endl;
   
    // Accelerators
    mOutput << "Accelerator \"bvh\"" << std::endl;
   
    // Participating Media
    // Assimp does not support participating media
}

void PbrtExporter::WriteCameras() {
    mOutput << std::endl;
    mOutput << "###############################" << std::endl;
    mOutput << "# Writing Camera and Film data:" << std::endl;
    mOutput << "###############################" << std::endl;
    mOutput << "# - Number of Cameras found in scene: ";
    mOutput << mScene->mNumCameras << std::endl;
    
    if (mScene->mNumCameras == 0){
        mOutput << "# - No Cameras found in the scene" << std::endl;
        return;
    }

    if (mScene->mNumCameras > 1) {
        mOutput << "# - Multiple Cameras found in scene" << std::endl;
        mOutput << "#   - Defaulting to first Camera specified" << std::endl;
    }
    
    for(int i = 0; i < mScene->mNumCameras; i++){
        WriteCamera(i);
    }
}

void PbrtExporter::WriteCamera(int i) {
    auto camera = mScene->mCameras[i]; 
    bool cameraActive = i == 0;

    mOutput << "# - Camera " << i+1  <<  ": "
        << camera->mName.C_Str() << std::endl;

    // Get camera aspect ratio
    float aspect = camera->mAspect;
    if(aspect == 0){
        aspect = 4.0/3.0;
        mOutput << "#   - Aspect ratio : 1.33333 (no aspect found, defaulting to 4/3)" << std::endl;
    } else {
        mOutput << "#   - Aspect ratio : " << aspect << std::endl;
    }

    // Get camera fov
    float hfov = AI_RAD_TO_DEG(camera->mHorizontalFOV);
    float fov;
    mOutput << "#   - Horizontal fov : " << hfov << std::endl;
    if (aspect >= 1.0)
        fov = hfov;
    else
        fov = hfov * aspect;

    // Get Film xres and yres
    int xres = 640;
    int yres = (int)round(640/aspect);

    // Print Film for this camera
    if (!cameraActive)
        mOutput << "# ";
    mOutput << "Film \"image\" \"string filename\" \""
        << mFile << "_pbrt.exr\"" << std::endl;
    if (!cameraActive)
        mOutput << "# ";
    mOutput << "    \"integer xresolution\" ["
        << xres << "]" << std::endl;
    if (!cameraActive)
        mOutput << "# ";
    mOutput << "    \"integer yresolution\" ["
        << yres << "]" << std::endl;

    // Get camera transform
    // Isn't optimally efficient, but is the simplest implementation
    //   Get camera node
    aiMatrix4x4 w2c;
    auto cameraNode = mScene->mRootNode->FindNode(camera->mName);
    if (!cameraNode) {
        mOutput << "# ERROR: Camera declared but not found in scene tree" << std::endl;
    }
    else {
        std::vector<aiMatrix4x4> matrixChain;
        auto tempNode = cameraNode;
        while(tempNode) {
            matrixChain.insert(matrixChain.begin(), tempNode->mTransformation);
            tempNode = tempNode->mParent;
        }

        w2c = matrixChain[0];
        for(int i = 1; i < matrixChain.size(); i++){
            w2c *= matrixChain[i];
        }
    }
    
    // Print Camera LookAt
    auto position = w2c * camera->mPosition;
    auto lookAt = w2c * camera->mLookAt;
    auto up = w2c * camera->mUp;
    if  (!cameraActive)
        mOutput << "# ";
    mOutput << "LookAt "
        << position.x << " " << position.y << " " << position.z << std::endl;
    if  (!cameraActive)
        mOutput << "# ";
    mOutput << "       "
        << lookAt.x << " " << lookAt.y << " " << lookAt.z << std::endl;
    if  (!cameraActive)
        mOutput << "# ";
    mOutput << "       "
        << up.x << " " << up.y << " " << up.z << std::endl;
    
    // Print camera descriptor
    if(!cameraActive)
        mOutput << "# ";
    mOutput << "Camera \"perspective\" \"float fov\" " 
        << "[" << fov << "]" << std::endl;
}

void PbrtExporter::WriteShapes() {
    // - figure out if should all be in 1 file (no camera?)
    // - if more than 1 file, place each geo in separate directory
    // - NEED to figure out how meshes are/should be split up

    // create geometry_<filename> folder
    // bool mIOSystem->CreateDirectory(path)
    
    // TODO worry about sequestering geo later, after giant print
}

void PbrtExporter::WriteWorldDefinition() {
    mOutput << std::endl;
    mOutput << "############################" << std::endl;
    mOutput << "# Writing World Definitiion:" << std::endl;
    mOutput << "############################" << std::endl;
    
    // Print WorldBegin
    mOutput << "WorldBegin" << std::endl;
    
    // Check to see if the scene has Embedded Textures
    CheckEmbeddedTextures();
    
    // Print Textures
    WriteTextures();

    // Print materials
    WriteMaterials();

    // Print Objects
    // Both PBRT's `Shape` and `Object` are in Assimp's `aiMesh` class
    WriteObjects();

    // Print Object Instancing (nodes)
    WriteObjectInstances();

    // Print Lights (w/o geometry)
    WriteLights();

    // Print Area Lights (w/ geometry)

    // Print WorldEnd
    mOutput << std::endl << "WorldEnd";
}


void PbrtExporter::CheckEmbeddedTextures() {
    mOutput << std::endl;
    mOutput << "###################" << std::endl;
    mOutput << "# Checking Embededded Textures:" << std::endl;
    mOutput << "###################" << std::endl;
    mOutput << "# - Number of Embedded Textures found in scene: ";
    mOutput << mScene->mNumTextures << std::endl;

    if (mScene->mNumTextures == 0)
        return;

    mOutput << "# ERROR: PBRT does not support Embedded Textures" << std::endl;
}

void PbrtExporter::WriteTextures() {
    mOutput << std::endl;
    mOutput << "###################" << std::endl;
    mOutput << "# Writing Textures:" << std::endl;
    mOutput << "####################" << std::endl;

    std::stringstream texStream;
    int numTotalTextures = 0;
        
    C_STRUCT aiString path;
    aiTextureMapping mapping;
    unsigned int uvindex;
    ai_real blend;
    aiTextureOp op;
    aiTextureMapMode mapmode[3];

    // For every material in the scene,
    for (int m = 0 ; m < mScene->mNumMaterials; m++) {
        auto material = mScene->mMaterials[m];
        // Parse through all texture types,
        for (int tt = 1; tt <= aiTextureType_UNKNOWN; tt++) {
            int ttCount = material->GetTextureCount(aiTextureType(tt));
            // ... and get every texture
            for (int t = 0; t < ttCount; t++) {
                // TODO write out texture specifics
                // TODO UV transforms may be material specific
                //        so those may need to be baked into unique tex name  
                material->GetTexture(
                    aiTextureType(tt),
                    t,
                    &path,
                    &mapping,
                    &uvindex,
                    &blend,
                    &op,
                    mapmode);

                std::string spath = std::string(path.C_Str());
                std::replace(spath.begin(), spath.end(), '\\', '/');
                std::string name = GetTextureName(spath, tt);

                if (mTextureSet.find(name) == mTextureSet.end()) {
                    numTotalTextures++;
                    texStream << "Texture \"" << name << "\" \"spectrum\" "
                        << "\"string filename\" \"" << spath << "\"" << std::endl;
                }
            }
        }
    }

    // Write out to file
    mOutput << "# - Number of Textures found in scene: ";
    mOutput << numTotalTextures << std::endl;
    mOutput << texStream.str();
}

void PbrtExporter::WriteMaterials() {
    mOutput << std::endl;
    mOutput << "####################" << std::endl;
    mOutput << "# Writing Materials:" << std::endl;
    mOutput << "####################" << std::endl;
    mOutput << "# - Number of Materials found in scene: ";
    mOutput << mScene->mNumMaterials << std::endl;

    if (mScene->mNumMaterials == 0)
        return;

    // TODO remove default when numCameras == 0
    //      For now, only on debug
    mOutput << "# - Creating a default grey matte material" << std::endl;
    mOutput << "Material \"matte\" \"rgb Kd\" [.8 .8 .8]" << std::endl; 

    for (int i = 0 ; i < mScene->mNumMaterials; i++) {
        WriteMaterial(i);
    }

}

void PbrtExporter::WriteMaterial(int m) {
    auto material = mScene->mMaterials[m]; 
    
    // get material name
    auto materialName = material->GetName();
    mOutput << std::endl << "# - Material " << m+1 <<  ": "
        << materialName.C_Str() << std::endl;

    // Print out number of properties
    mOutput << "#   - Number of Material Properties: "
        << material->mNumProperties << std::endl;

    // Print out texture type counts
    int textureCounts[aiTextureType_UNKNOWN];
    for (int i = 1; i <= aiTextureType_UNKNOWN; i++) {
        textureCounts[i-1] = material->GetTextureCount(aiTextureType(i));
    }
    mOutput << "#   - Texture Type Counts:" << std::endl;
    for (int tt = 1; tt <= aiTextureType_UNKNOWN; tt++) {
        mOutput << "#     - " << TextureTypeToString(aiTextureType(tt)); 
        mOutput << ": " <<  textureCounts[tt - 1] << std::endl;
    }

    // Decide which material PBRT should set this to
    //   if contains any PBR textures, then Disney
    if(    textureCounts[aiTextureType_BASE_COLOR -1] > 0
        || textureCounts[aiTextureType_NORMAL_CAMERA -1] > 0
        || textureCounts[aiTextureType_EMISSION_COLOR -1] > 0
        || textureCounts[aiTextureType_METALNESS -1] > 0
        || textureCounts[aiTextureType_DIFFUSE_ROUGHNESS -1] > 0
        || textureCounts[aiTextureType_AMBIENT_OCCLUSION -1] > 0) {
        WriteDisneyMaterial(material);
        return;
    }
    else {
        WriteUberMaterial(material);
        return;
    }

    // TODO
    //      - matte   (has only kd?)
    //      - metal   (never? has only ks?)
    //      - mirror  (has reflection)
    //      - plastic (never?)
}


void PbrtExporter::WriteLights() {
    mOutput << std::endl;
    mOutput << "#################" << std::endl;
    mOutput << "# Writing Lights:" << std::endl;
    mOutput << "#################" << std::endl;
    mOutput << "# - Number of Lights found in scene: ";
    mOutput << mScene->mNumLights << std::endl;
    
    // TODO remove default ambient term when numCameras == 0
    //      For now, ambient may only be necessary for debug
    mOutput << "# - Creating a default blueish ambient light source" << std::endl;
    mOutput << "LightSource \"infinite\" \"rgb L\" [0.4 0.45 0.5]" << std::endl;
    mOutput << "    \"integer samples\" [8]" << std::endl;
}

void PbrtExporter::WriteObjects() {
    mOutput << std::endl;
    mOutput << "#############################" << std::endl;
    mOutput << "# Writing Object Definitions:" << std::endl;
    mOutput << "#############################" << std::endl;
    mOutput << "# - Number of Meshes found in scene: ";
    mOutput << mScene->mNumMeshes << std::endl;

    for (int i = 0 ; i < mScene->mNumMeshes; i++) {
        WriteObject(i);
    }
}

void PbrtExporter::WriteObject(int i) {
    auto mesh = mScene->mMeshes[i]; 
    mOutput << "# - Mesh " << i+1  <<  ": ";
    if (mesh->mName == aiString(""))
        mOutput << "<No Name>" << std::endl;
    else
        mOutput << mesh->mName.C_Str() << std::endl;
   
    // Print out primitive types found
    mOutput << "#   - Primitive Type(s):" << std::endl;
    if (mesh->mPrimitiveTypes & aiPrimitiveType_POINT)
        mOutput << "#     - POINT" << std::endl;
    if (mesh->mPrimitiveTypes & aiPrimitiveType_LINE)
        mOutput << "#     - LINE" << std::endl;
    if (mesh->mPrimitiveTypes & aiPrimitiveType_TRIANGLE)
        mOutput << "#     - TRIANGLE" << std::endl;
    if (mesh->mPrimitiveTypes & aiPrimitiveType_POLYGON)
        mOutput << "#     - POLYGON" << std::endl;
    
    // Check if any types other than tri
    if (   (mesh->mPrimitiveTypes & aiPrimitiveType_POINT) 
        || (mesh->mPrimitiveTypes & aiPrimitiveType_LINE)
        || (mesh->mPrimitiveTypes & aiPrimitiveType_POLYGON)) {
        mOutput << "# ERROR: PBRT Does not support POINT, LINE, POLY meshes" << std::endl;
    }

    // Check for Normals
    mOutput << "#   - Normals: ";
    if (mesh->mNormals)
        mOutput << "TRUE" << std::endl;
    else
        mOutput << "FALSE" << std::endl;
    
    // Check for Tangents
    mOutput << "#   - Tangents: ";
    if (mesh->mTangents)
        mOutput << "TRUE" << std::endl;
    else
        mOutput << "FALSE" << std::endl;

    // Count number of texture coordinates
    int numTextureCoords = 0;
    for (int i = 0; i < AI_MAX_NUMBER_OF_TEXTURECOORDS; i++) {
        if (mesh->mTextureCoords[i])
            numTextureCoords++;
    }
    mOutput << "#   - Number of Texture Coordinates: "
        << numTextureCoords << std::endl;
    if (numTextureCoords > 1) {
        mOutput << "# - Multiple Texture Coordinates found in scene" << std::endl;
        mOutput << "#   - Defaulting to first Texture Coordinate specified" << std::endl;
    }

    // TODO Check for Alpha texture
    mOutput << "#   - Alpha texture: " << std::endl;

    // Create ObjectBegin
    mOutput << "ObjectBegin \"";
    if (mesh->mName == aiString(""))
        mOutput << "mesh_" << i+1 << "\"" << std::endl;
    else
        mOutput << mesh->mName.C_Str() << "_" << i+1 << "\"" << std::endl;

    // Write Shapes
    mOutput << "Shape \"trianglemesh\"" << std::endl
        << "    \"integer indices\" [";
    //   Start with faces (which hold indices)
    for(int i = 0; i < mesh->mNumFaces; i++) {
        auto face = mesh->mFaces[i];
        for(int j = 0; j < face.mNumIndices; j++) {
            mOutput << face.mIndices[j] << " ";
        }
    }
    mOutput << "]" << std::endl;
    //   Then go to vertices
    mOutput << "    \"point P\" [";
    for(int i = 0; i < mesh->mNumVertices; i++) {
        auto vector = mesh->mVertices[i];
        mOutput << vector.x << " " << vector.y << " " << vector.z << "  ";
    }
    mOutput << "]" << std::endl;
    //   Normals (if present)
    if (mesh->mNormals) {
        mOutput << "    \"normal N\" [";
        for(int i = 0; i < mesh->mNumVertices; i++) {
            auto normal = mesh->mNormals[i];
            mOutput << normal.x << " " << normal.y << " " << normal.z << "  ";
        }
        mOutput << "]" << std::endl;
    }
    //   Tangents (if present)
    if (mesh->mTangents) {
        mOutput << "    \"vector S\" [";
        for(int i = 0; i < mesh->mNumVertices; i++) {
            auto tangent = mesh->mTangents[i];
            mOutput << tangent.x << " " << tangent.y << " " << tangent.z << "  ";
        }
        mOutput << "]" << std::endl;
    }
    //   Texture Coords (if present)
    //   TODO comment out wrong ones, only choose 1st 2d texture coord


    // Close ObjectBegin
    mOutput << "ObjectEnd" << std::endl;
}

void PbrtExporter::WriteObjectInstances() {
    mOutput << std::endl;
    mOutput << "###########################" << std::endl;
    mOutput << "# Writing Object Instances:" << std::endl;
    mOutput << "###########################" << std::endl;

    // Get root node of the scene
    auto rootNode = mScene->mRootNode;

    // Set base transform to identity
    aiMatrix4x4 parentTransform;

    // Recurse into root node
    WriteObjectInstance(rootNode, parentTransform);
}

void PbrtExporter::WriteObjectInstance(aiNode* node, aiMatrix4x4 parent) {
    auto w2o = parent * node->mTransformation;

    // Print transformation for this node
    if(node->mNumMeshes > 0) {
        mOutput << "Transform ["
            << w2o.a1 << " " << w2o.a2 << " " << w2o.a3 << " " << w2o.a4 << " "
            << w2o.b1 << " " << w2o.b2 << " " << w2o.b3 << " " << w2o.b4 << " "
            << w2o.c1 << " " << w2o.c2 << " " << w2o.c3 << " " << w2o.c4 << " "
            << w2o.d1 << " " << w2o.d2 << " " << w2o.d3 << " " << w2o.d4
            << "]" << std::endl;
    }

    // Loop over number of meshes in node
    for(int i = 0; i < node->mNumMeshes; i++) {
        // Print ObjectInstance
        mOutput << "ObjectInstance \"";
        auto mesh = mScene->mMeshes[node->mMeshes[i]];
        if (mesh->mName == aiString(""))
            mOutput << "mesh_" << node->mMeshes[i] + 1 << "\"" << std::endl;
        else
            mOutput << mesh->mName.C_Str() << "_" << node->mMeshes[i] + 1 << "\"" << std::endl;
    }

    // Recurse through children
    for (int i = 0; i < node->mNumChildren; i++) {
        WriteObjectInstance(node->mChildren[i], w2o);
    }
}

std::string PbrtExporter::GetTextureName(
    std::string spath, unsigned int textureType) {

    std::stringstream name;
    name << TextureTypeToString(aiTextureType(textureType));
    name << ":" << spath;
    return name.str();
}

void PbrtExporter::WriteDisneyMaterial(aiMaterial* material) {
    auto materialName = material->GetName();

    // Get texture type counts
    int textureCounts[aiTextureType_UNKNOWN];
    for (int i = 1; i <= aiTextureType_UNKNOWN; i++) {
        textureCounts[i-1] = material->GetTextureCount(aiTextureType(i));
    }

    // Get diffuse
    std::stringstream diffuse;

    // Use MakeNamedMaterial to give variable names to materials
    mOutput << "MakeNamedMaterial \"" << materialName.C_Str() << "\""
        << " \"string type\" \"disney\"" << std::endl;

}

void PbrtExporter::WriteUberMaterial(aiMaterial* material) {
    auto materialName = material->GetName();

    // Use MakeNamedMaterial to give variable names to materials
    mOutput << "MakeNamedMaterial \"" << materialName.C_Str() << "\""
        << " \"string type\" \"uber\"" << std::endl;

}

#endif // ASSIMP_BUILD_NO_PBRT_EXPORTER
#endif // ASSIMP_BUILD_NO_EXPORT
