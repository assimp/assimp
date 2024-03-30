// SPDX-License-Identifier: Apache 2.0
// Copyright 2022 - 2023, Syoyo Fujita.
// Copyright 2023 - Present, Light Transport Entertainment Inc.
//
// UsdSkel API implementations

#include "usdSkel.hh"

#include <sstream>

#include "common-macros.inc"
#include "tiny-format.hh"
#include "prim-types.hh"

namespace tinyusdz {
namespace {}  // namespace

constexpr auto kInbetweensNamespace = "inbetweens";

bool BlendShape::add_inbetweenBlendShape(const double weight, Attribute &&attr) {

  if (attr.name().empty()) {
    return false;
  }

  if (attr.is_uniform()) {
    return false;
  }

  if (!attr.is_value()) {
    return false;
  }

  std::string attr_name = fmt::format("{}:{}", kInbetweensNamespace, attr.name());
  attr.set_name(attr_name);

  attr.metas().weight = weight;

  props[attr_name] = Property(attr, /* custom */false);

  return true;
}

bool SkelAnimation::get_blendShapes(std::vector<value::token> *toks) {
  return blendShapes.get_value(toks);
}

bool SkelAnimation::get_joints(std::vector<value::token> *dst) {
  return joints.get_value(dst);
}

bool SkelAnimation::get_blendShapeWeights(
    std::vector<float> *vals, const double t,
    const value::TimeSampleInterpolationType tinterp) {
  Animatable<std::vector<float>> v;
  if (blendShapeWeights.get_value(&v)) {
    // Evaluate at time `t` with `tinterp` interpolation
    return v.get(t, vals, tinterp);
  }

  return false;
}

bool SkelAnimation::get_rotations(std::vector<value::quatf> *vals,
                                  const double t,
                                  const value::TimeSampleInterpolationType tinterp) {
  Animatable<std::vector<value::quatf>> v;
  if (rotations.get_value(&v)) {
    // Evaluate at time `t` with `tinterp` interpolation
    return v.get(t, vals, tinterp);
  }

  return false;
}

bool SkelAnimation::get_scales(std::vector<value::half3> *vals, const double t,
                               const value::TimeSampleInterpolationType tinterp) {
  Animatable<std::vector<value::half3>> v;
  if (scales.get_value(&v)) {
    // Evaluate at time `t` with `tinterp` interpolation
    return v.get(t, vals, tinterp);
  }

  return false;
}

bool SkelAnimation::get_translations(
    std::vector<value::float3> *vals, const double t,
    const value::TimeSampleInterpolationType tinterp) {
  Animatable<std::vector<value::float3>> v;
  if (translations.get_value(&v)) {
    // Evaluate at time `t` with `tinterp` interpolation
    return v.get(t, vals, tinterp);
  }

  return false;
}

}  // namespace tinyusdz

