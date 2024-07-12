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
