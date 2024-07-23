/*
Open Asset Import Library (assimp)
----------------------------------------------------------------------

Copyright (c) 2006-2024, assimp team

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

#ifndef ASSIMP_BUILD_NO_USD_IMPORTER
#include "USDLoaderImplTinyusdzHelper.h"

#include "../../../contrib/tinyusdz/assimp_tinyusdz_logging.inc"

namespace {
//const char *const TAG = "tinyusdz helper";
}

using ChannelType = tinyusdz::tydra::AnimationChannel::ChannelType;
std::string Assimp::tinyusdzAnimChannelTypeFor(ChannelType animChannel) {
    switch (animChannel) {
    case ChannelType::Transform: {
        return "Transform";
    }
    case ChannelType::Translation: {
        return "Translation";
    }
    case ChannelType::Rotation: {
        return "Rotation";
    }
    case ChannelType::Scale: {
        return "Scale";
    }
    case ChannelType::Weight: {
        return "Weight";
    }
    default:
        return "Invalid";
    }
}

using tinyusdz::tydra::NodeType;
std::string Assimp::tinyusdzNodeTypeFor(NodeType type) {
    switch (type) {
    case NodeType::Xform: {
        return "Xform";
    }
    case NodeType::Mesh: {
        return "Mesh";
    }
    case NodeType::Camera: {
        return "Camera";
    }
    case NodeType::Skeleton: {
        return "Skeleton";
    }
    case NodeType::PointLight: {
        return "PointLight";
    }
    case NodeType::DirectionalLight: {
        return "DirectionalLight";
    }
    case NodeType::EnvmapLight: {
        return "EnvmapLight";
    }
    default:
        return "Invalid";
    }
}

aiMatrix4x4 Assimp::tinyUsdzMat4ToAiMat4(const double matIn[4][4]) {
    aiMatrix4x4 matOut;
    matOut.a1 = matIn[0][0];
    matOut.a2 = matIn[0][1];
    matOut.a3 = matIn[0][2];
    matOut.a4 = matIn[0][3];
    matOut.b1 = matIn[1][0];
    matOut.b2 = matIn[1][1];
    matOut.b3 = matIn[1][2];
    matOut.b4 = matIn[1][3];
    matOut.c1 = matIn[2][0];
    matOut.c2 = matIn[2][1];
    matOut.c3 = matIn[2][2];
    matOut.c4 = matIn[2][3];
    matOut.d1 = matIn[3][0];
    matOut.d2 = matIn[3][1];
    matOut.d3 = matIn[3][2];
    matOut.d4 = matIn[3][3];
//    matOut.a1 = matIn[0][0];
//    matOut.a2 = matIn[1][0];
//    matOut.a3 = matIn[2][0];
//    matOut.a4 = matIn[3][0];
//    matOut.b1 = matIn[0][1];
//    matOut.b2 = matIn[1][1];
//    matOut.b3 = matIn[2][1];
//    matOut.b4 = matIn[3][1];
//    matOut.c1 = matIn[0][2];
//    matOut.c2 = matIn[1][2];
//    matOut.c3 = matIn[2][2];
//    matOut.c4 = matIn[3][2];
//    matOut.d1 = matIn[0][3];
//    matOut.d2 = matIn[1][3];
//    matOut.d3 = matIn[2][3];
//    matOut.d4 = matIn[3][3];
    return matOut;
}

aiVector3D Assimp::tinyUsdzScaleOrPosToAssimp(const std::array<float, 3> &scaleOrPosIn) {
    return aiVector3D(scaleOrPosIn[0], scaleOrPosIn[1], scaleOrPosIn[2]);
}

aiQuaternion Assimp::tinyUsdzQuatToAiQuat(const std::array<float, 4> &quatIn) {
    // tinyusdz "quat" is x,y,z,w
    // aiQuaternion is w,x,y,z
    return aiQuaternion(
            quatIn[3], quatIn[0], quatIn[1], quatIn[2]);
}

#endif // !! ASSIMP_BUILD_NO_USD_IMPORTER
