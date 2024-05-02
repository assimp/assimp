#pragma once
#ifndef AI_USDLOADER_IMPL_TINYUSDZ_HELPER_H_INCLUDED
#define AI_USDLOADER_IMPL_TINYUSDZ_HELPER_H_INCLUDED

#include <assimp/BaseImporter.h>
#include <assimp/scene.h>
#include <assimp/types.h>
#include "tinyusdz.hh"
#include "tydra/render-data.hh"

namespace Assimp {

std::string tinyusdzAnimChannelTypeFor(
        tinyusdz::tydra::AnimationChannel::ChannelType animChannel);
std::string tinyusdzNodeTypeFor(tinyusdz::tydra::NodeType type);
aiMatrix4x4 tinyUsdzMat4ToAiMat4(const double matIn[4][4]);

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
