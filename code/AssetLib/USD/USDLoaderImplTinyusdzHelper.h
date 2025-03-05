/*
Open Asset Import Library (assimp)
----------------------------------------------------------------------

Copyright (c) 2006-2025, assimp team

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

#pragma once
#ifndef AI_USDLOADER_IMPL_TINYUSDZ_HELPER_H_INCLUDED
#define AI_USDLOADER_IMPL_TINYUSDZ_HELPER_H_INCLUDED

#include <assimp/BaseImporter.h>
#include <assimp/scene.h>
#include <assimp/types.h>
#include "tinyusdz.hh"
#include "tydra/render-data.hh"
#include <type_traits>

namespace Assimp {

std::string tinyusdzAnimChannelTypeFor(
        tinyusdz::tydra::AnimationChannel::ChannelType animChannel);
std::string tinyusdzNodeTypeFor(tinyusdz::tydra::NodeType type);

template <typename T>
aiMatrix4x4 tinyUsdzMat4ToAiMat4(const T matIn[4][4]) {
    static_assert(std::is_floating_point_v<T>, "Only floating-point types are allowed.");
    aiMatrix4x4 matOut;
    matOut.a1 = ai_real(matIn[0][0]);
    matOut.a2 = ai_real(matIn[1][0]);
    matOut.a3 = ai_real(matIn[2][0]);
    matOut.a4 = ai_real(matIn[3][0]);
    matOut.b1 = ai_real(matIn[0][1]);
    matOut.b2 = ai_real(matIn[1][1]);
    matOut.b3 = ai_real(matIn[2][1]);
    matOut.b4 = ai_real(matIn[3][1]);
    matOut.c1 = ai_real(matIn[0][2]);
    matOut.c2 = ai_real(matIn[1][2]);
    matOut.c3 = ai_real(matIn[2][2]);
    matOut.c4 = ai_real(matIn[3][2]);
    matOut.d1 = ai_real(matIn[0][3]);
    matOut.d2 = ai_real(matIn[1][3]);
    matOut.d3 = ai_real(matIn[2][3]);
    matOut.d4 = ai_real(matIn[3][3]);
    return matOut;
}
aiVector3D tinyUsdzScaleOrPosToAssimp(const std::array<float, 3> &scaleOrPosIn);

/**
 * Convert quaternion from tinyusdz "quat" to assimp "aiQuaternion" type
 *
 * @param quatIn tinyusdz float[4] in x,y,z,w order
 * @return assimp aiQuaternion converted from input
 */
aiQuaternion tinyUsdzQuatToAiQuat(const std::array<float, 4> &quatIn);

} // namespace Assimp
#endif // AI_USDLOADER_IMPL_TINYUSDZ_HELPER_H_INCLUDED
