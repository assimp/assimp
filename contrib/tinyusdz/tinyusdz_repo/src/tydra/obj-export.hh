// SPDX-License-Identifier: Apache 2.0
// Copyright 2024 - Present, Light Transport Entertainment Inc.
//
// Simple RenderMesh/RenderMaterial -> wavefront .obj exporter
//
#pragma once

#include "render-data.hh"

namespace tinyusdz {
namespace tydra {

///
/// Export RenderMesh/RenderMaterial to .obj.
/// Requires RenderScene instance to export Material/Texture correctly.
///
/// NOTE: No consideration of up-Axis. 3D coordinate is exported as-as.
/// Thus, if your USD scene is Z-up, 3D coordinate in exported .obj is Z-up.
///
/// (fortunately, you can import .obj by specifying Z-up axis in Blender)
///
/// @param[in] scene RenderScene
/// @param[in] mesh_id Mesh id in RenderScene
/// @param[out] obj_str .obj string
/// @param[out] warn warning message
/// @param[out] err error message
///
/// @return true upon success.
///
bool export_to_obj(const RenderScene &scene, const int mesh_id,
                   std::string &obj_str, std::string &mtl_str,
                   std::string *warn, std::string *err);

}  // namespace tydra
}  // namespace tinyusdz
