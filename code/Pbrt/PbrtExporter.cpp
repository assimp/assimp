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

    // Write preamble
    WritePreamble();

    // Write World Description
    WriteWorldDefinition();

    // Write out to file
    outfile->Write(mOutput.str().c_str(), 
            mOutput.str().length(), 1);
    
    // explicitly release file pointer,
    // so we don't have to rely on class destruction.
    outfile.reset();
}

// Destructor
PbrtExporter::~PbrtExporter() {
    // Empty
}

void PbrtExporter::WriteHeader() {

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
                
                break;
        }
        mOutput << std::endl;
    }
}

void PbrtExporter::WritePreamble() {

}

void PbrtExporter::WriteWorldDefinition() {

}

#endif // ASSIMP_BUILD_NO_PBRT_EXPORTER
#endif // ASSIMP_BUILD_NO_EXPORT
