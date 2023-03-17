// Copyright 2022 The Draco Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
#ifndef DRACO_IO_GLTF_DECODER_TEST_HELPER_H_
#define DRACO_IO_GLTF_DECODER_TEST_HELPER_H_

#include "draco/draco_features.h"

#ifdef DRACO_TRANSCODER_SUPPORTED
#include "draco/scene/scene.h"

namespace draco {

// Helper class for testing Draco glTF encoder and decoder.
class GltfTestHelper {
 public:
  // Adds various mesh feature ID sets (via attributes and via textures) and
  // structural metadata property table and property table schema to the box
  // |scene| loaded from the test file testdata/Box/glTF/Box.gltf.
  static void AddBoxMetaMeshFeatures(Scene *scene);
  static void AddBoxMetaStructuralMetadata(Scene *scene);

  // Checks the box |geometry| (draco::Mesh or draco::Scene) with  mesh features
  // loaded from one of these test files, with or without Draco compression:
  // 1. testdata/BoxMeta/glTF/BoxMeta.gltf
  // 2. testdata/BoxMetaDraco/glTF/BoxMetaDraco.gltf
  template <typename GeometryT>
  static void CheckBoxMetaMeshFeatures(const GeometryT &geometry,
                                       bool has_draco_compression);

  // Checks the box |geometry| (draco::Mesh or draco::Scene) with structural
  // metadata that includes property table and property table schema loaded from
  // test file testdata/BoxMeta/glTF/BoxMeta.gltf.
  template <typename GeometryT>
  static void CheckBoxMetaStructuralMetadata(const GeometryT &geometry) {
    CheckBoxMetaStructuralMetadata(geometry.GetStructuralMetadata());
  }

 private:
  static void CheckBoxMetaMeshFeatures(const Mesh &mesh,
                                       const TextureLibrary &texture_lib,
                                       bool has_draco_compression);
  static void CheckBoxMetaStructuralMetadata(
      const StructuralMetadata &structural_metadata);
};

}  // namespace draco

#endif  // DRACO_TRANSCODER_SUPPORTED
#endif  // DRACO_IO_GLTF_DECODER_TEST_HELPER_H_
