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

    // TODO Prettify the output
    // TODO Do Animation
}

// Destructor
PbrtExporter::~PbrtExporter() {
    // Empty
}

void PbrtExporter::WriteHeader() {
    // TODO
}

void PbrtExporter::WriteMetaData() {
    mOutput << "# Writing out scene metadata:" << std::endl;
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
    // Cameras
    WriteCameras();

    // Samplers
   
    // Film
   
    // Filters
   
    // Integrators
   
    // Accelerators
   
    // Participating Media
}

void PbrtExporter::WriteWorldDefinition() {

}

void PbrtExporter::WriteCameras() {
    mOutput << std::endl;
    mOutput << "# Writing Camera data:" << std::endl;
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
    // IMMEDIATELY

    // Get camera hfov
    if (!cameraActive)
        mOutput << "# ";
    mOutput << "\"float hfov_" << camera->mName.C_Str() << "\" ["
        << AI_RAD_TO_DEG(camera->mHorizontalFOV)
        << "]" << std::endl;

    // Get Camera clipping planes?
    // TODO

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

        mOutput << "Transform "
            << w2c.a1 << " " << w2c.a2 << " " << w2c.a3 << " " << w2c.a4 << " "
            << w2c.b1 << " " << w2c.b2 << " " << w2c.b3 << " " << w2c.b4 << " "
            << w2c.c1 << " " << w2c.c2 << " " << w2c.c3 << " " << w2c.c4 << " "
            << w2c.d1 << " " << w2c.d2 << " " << w2c.d3 << " " << w2c.d4
            << std::endl;
    }
    
    // Print camera descriptor

}


void PbrtExporter::WriteGeometry() {

}

#endif // ASSIMP_BUILD_NO_PBRT_EXPORTER
#endif // ASSIMP_BUILD_NO_EXPORT
