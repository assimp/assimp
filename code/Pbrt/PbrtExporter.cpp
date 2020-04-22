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

    // Write geometry
    WriteGeometry();

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

        aiMatrix4x4 w2c = matrixChain[0];
        for(int i = 1; i < matrixChain.size(); i++){
            w2c *= matrixChain[i];
        }
        
        if (!cameraActive)
            mOutput << "# ";

        mOutput << "Transform ["
            << w2c.a1 << " " << w2c.a2 << " " << w2c.a3 << " " << w2c.a4 << " "
            << w2c.b1 << " " << w2c.b2 << " " << w2c.b3 << " " << w2c.b4 << " "
            << w2c.c1 << " " << w2c.c2 << " " << w2c.c3 << " " << w2c.c4 << " "
            << w2c.d1 << " " << w2c.d2 << " " << w2c.d3 << " " << w2c.d4
            << "]" << std::endl;
    }
    
    // Print camera descriptor
    if(!cameraActive)
        mOutput << "# ";
    mOutput << "Camera \"perspective\" \"float fov\" " 
        << "[" << fov << "]" << std::endl;
}

void PbrtExporter::WriteGeometry() {
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
    
    // Print Textures
    WriteTextures();

    // Print materials
    WriteMaterials();

    // Print Lights (w/o geometry)
    WriteLights();
    
    // Print Shapes
    WriteShapes();
    
    // Print Object Instancing (no emissive)

    // Print Area Lights (w/ geometry)

    // Print WorldEnd
    mOutput << std::endl << "WorldEnd";
}


void PbrtExporter::WriteTextures() {
    mOutput << std::endl;
    mOutput << "###################" << std::endl;
    mOutput << "# Writing Textures:" << std::endl;
    mOutput << "###################" << std::endl;
    mOutput << "# - Number of Textures found in scene: ";
    mOutput << mScene->mNumTextures << std::endl;

    if (mScene->mNumTextures == 0)
        return;

    for (int i = 0 ; i < mScene->mNumTextures; i++) {
        // TODO
    }

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

void PbrtExporter::WriteMaterial(int i) {
    // TODO

    // Use MakeNamedMaterial to give variable names to materials
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
    mOutput << "LightSource \"infinite\" \"rgb L\" [.4 .45 .5]" << std::endl; 

}

void PbrtExporter::WriteShapes() {
    mOutput << std::endl;
    mOutput << "#################" << std::endl;
    mOutput << "# Writing Shapes:" << std::endl;
    mOutput << "#################" << std::endl;
    mOutput << "# - Number of Meshes found in scene: ";
    mOutput << mScene->mNumMeshes << std::endl;

    if (mScene->mNumMeshes == 0)
        return;

    for (int i = 0 ; i < mScene->mNumMeshes; i++) {
        WriteShape(i);
    }
}

void PbrtExporter::WriteShape(int i) {
    // TODO IMMEDIATELY
    
    
    // if aiPrimtiveType == Tri -> you're fine
    // if aiPrimitiveType == Poly -> use aiProcess_triangulate
    // if aiPrimitiveType == anything else -> throw error
}

#endif // ASSIMP_BUILD_NO_PBRT_EXPORTER
#endif // ASSIMP_BUILD_NO_EXPORT
