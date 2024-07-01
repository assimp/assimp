// SPDX-License-Identifier: Apache 2.0
// Copyright 2023 - Present, Light Transport Entertainment, Inc.
#pragma once

#include <string>

namespace tinyusdz {
namespace tydra {

//
// ARKit compatible BlendShape target names(52 shape targets).
//

enum ARKitBlendShapeLocation {
  BrowDownLeft = 0,
  BrowDownRight,
  BrowInnerUp,
  BrowOuterUpLeft,
  BrowOuterUpRight,
  CheekPuff,
  CheekSquintLeft,
  CheekSquintRight,
  EyeBlinkLeft,
  EyeBlinkRight,
  EyeLookDownLeft,
  EyeLookDownRight,
  EyeLookInLeft,
  EyeLookInRight,
  EyeLookOutLeft,
  EyeLookOutRight,
  EyeLookUpLeft,
  EyeLookUpRight,
  EyeSquintLeft,
  EyeSquintRight,
  EyeWideLeft,
  EyeWideRight,
  JawForward,
  JawLeft,
  JawOpen,
  JawRight,
  MouthClose,
  MouthDimpleLeft,
  MouthDimpleRight,
  MouthFrownLeft,
  MouthFrownRight,
  MouthFunnel,
  MouthLeft,
  MouthLowerDownLeft,
  MouthLowerDownRight,
  MouthPressLeft,
  MouthPressRight,
  MouthPucker,
  MouthRight,
  MouthRollLower,
  MouthRollUpper,
  MouthShrugLower,
  MouthShrugUpper,
  MouthSmileLeft,
  MouthSmileRight,
  MouthStretchLeft,
  MouthStretchRight,
  MouthUpperUpLeft,
  MouthUpperUpRight,
  NoseSneerLeft,
  NoseSneerRight,
  TongueOut  // = 52
};

std::string GetARKitBlendShapeLocationString(
    const ARKitBlendShapeLocation loc);
bool GetARKitBlendShapeLocationEnumFromString(const std::string &name,
                                              ARKitBlendShapeLocation *loc);

}  // namespace tydra
}  // namespace tinyusdz

