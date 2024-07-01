// SPDX-License-Identifier: Apache 2.0
// Copyright 2023 - Present, Light Transport Entertainment, Inc.

#include <array>
#include <string>
#include <cstdint>

//
#include "facial.hh"

namespace tinyusdz {
namespace tydra {

constexpr std::array<const char *, 52> gARKitBlendShapeLocationKV = {
    "blowDownLeft",
    "browDownRight",
    "browInnerUp",
    "browOuterUpLeft",
    "browOuterUpRight",
    "cheekPuff",
    "cheekSquintLeft",
    "cheekSquintRight",
    "eyeBlinkLeft",
    "eyeBlinkRight",
    "eyeLookDownLeft",
    "eyeLookDownRight",
    "eyeLookInLeft",
    "eyeLookInRight",
    "eyeLookOutLeft",
    "eyeLookOutRight",
    "eyeLookUpLeft",
    "eyeLookUpRight",
    "eyeSquintLeft",
    "eyeSquintRight",
    "eyeWideLeft",
    "eyeWideRight",
    "jawForward",
    "jawLeft",
    "jawOpen",
    "jawRight",
    "mouthClose",
    "mouthDimpleLeft",
    "mouthDimpleRight",
    "mouthFrownLeft",
    "mouthFrownRight",
    "mouthFunnel",
    "mouthLeft",
    "mouthLowerDownLeft",
    "mouthLowerDownRight",
    "mouthPressLeft",
    "mouthPressRight",
    "mouthPucker",
    "mouthRight",
    "mouthRollLower",
    "mouthRollUpper",
    "mouthShrugLower",
    "mouthShrugUpper",
    "mouthSmileLeft",
    "mouthSmileRight",
    "mouthStretchLeft",
    "mouthStretchRight",
    "mouthUpperUpLeft",
    "mouthUpperUpRight",
    "noseSneerLeft",
    "noseSneerRight",
    "tongueOut"};

std::string GetARKitBlendShapeLocationString(
    const ARKitBlendShapeLocation loc) {
  uint32_t i = static_cast<uint32_t>(loc);
  if (loc > 51) {
    return "[[InvalidBlendShapeLocationName]]";
  }

  return gARKitBlendShapeLocationKV[i];
}

bool GetARKitBlendShapeLocationEnumFromString(const std::string &s,
                                              ARKitBlendShapeLocation *loc) {
  if (!loc) {
    return false;
  }

  // simple linear scan
  for (size_t i = 0; i < 52; i++) {
    if (s.compare(gARKitBlendShapeLocationKV[i]) == 0) {
      (*loc) = static_cast<ARKitBlendShapeLocation>(i);
      return true;
    }
  }

  return false;
}

}  // namespace tydra
}  // namespace tinyusdz

