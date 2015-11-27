/*
Open Asset Import Library (assimp)
----------------------------------------------------------------------

Copyright (c) 2006-2015, assimp team
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
#ifndef AI_GLTFFILEDATA_H_INC
#define AI_GLTFFILEDATA_H_INC

#include <stdint.h>

namespace Assimp {
namespace glTF {


//! Magic number for GLB files
#define AI_GLB_MAGIC_NUMBER "glTF"


#include "./../include/assimp/Compiler/pushpack1.h"

// KHR_binary_glTF (binary .glb file)
// 20-byte header (+ the JSON + a "body" data section)
struct GLB_Header
{
    //! Magic number: "glTF"
    unsigned char magic[4];  // "glTF"

    //! Version number (always 1 as of the last update)
    uint32_t version;

    //! Total length of the Binary glTF, including header, scene, and body, in bytes
    uint32_t length;

    //! Length, in bytes, of the glTF scene
    uint32_t sceneLength;

    //! Specifies the format of the glTF scene (see the SceneFormat enum)
    uint32_t sceneFormat;
} PACK_STRUCT;

#include "./../include/assimp/Compiler/poppack1.h"



//! Values for the GLB_Header::sceneFormat field
enum SceneFormat
{
    SceneFormat_JSON = 0
};


//! Values for the mesh primitive modes
enum PrimitiveMode
{
    PrimitiveMode_POINTS         = 0,
    PrimitiveMode_LINES          = 1,
    PrimitiveMode_LINE_LOOP      = 2,
    PrimitiveMode_LINE_STRIP     = 3,
    PrimitiveMode_TRIANGLES      = 4,
    PrimitiveMode_TRIANGLE_STRIP = 5,
    PrimitiveMode_TRIANGLE_FAN   = 6
};


//! Values for the accessors component type field
enum ComponentType
{
    ComponentType_BYTE           = 5120,
    ComponentType_UNSIGNED_BYTE  = 5121,
    ComponentType_SHORT          = 5122,
    ComponentType_UNSIGNED_SHORT = 5123,
    ComponentType_FLOAT          = 5126
};


//! Will hold the enabled extensions
struct Extensions
{
    bool KHR_binary_glTF;
};


} // end namespaces
}


#endif // AI_GLTFFILEDATA_H_INC

