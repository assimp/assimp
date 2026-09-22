#pragma once

/// \file codegen.hpp
/// Generate C++ source that rebuilds a parsed `SkpModel` from scratch via `create.hpp`'s public
/// API - a faithful, human-readable, re-runnable transcript of the model as writer API calls, not
/// a serialized dump.
///
/// Handles: materials (solid and textured, including default-projection and explicitly-pinned
/// UVs), layers, component/group definitions (built in dependency order), faces (front/back
/// material, holes), instances (transform, instance-level paint, instance-level name).
///
/// Found and fixed via diffing a real, large file (jeff.skp: 2713 definitions, 113643 faces)
/// against its own regenerated output - the TypeScript port this mirrors (`toTypeScriptCode`)
/// found that an earlier prototype silently dropped instance-level paint (95% of that file's
/// instances) and every instance's own name entirely, and never emitted textured materials at
/// all. Building this module reused `edit.cpp`'s own already-fixed replay logic (`replay_uv`,
/// `non_collinear_triple`) as the design reference, so both share the exact same, already
/// real-fixture-tested UV/hole math.
///
/// Only reproduces geometry reachable by walking faces (`Definition::faces`) - a real file's
/// standalone/construction edges and curves that don't bound any face are NOT reproduced (same
/// limitation as the TypeScript port - see its own doc for the concrete numbers this was measured
/// against). This does not affect materials, textures, instance paint, or any face/surface
/// geometry - only invisible construction/reference lines.
///
/// Also not yet handled (matching this project's established disclosure pattern for known gaps):
/// colorized material tint, per-face hidden/soft/smooth edge flags, section planes, text/
/// dimension entities. A model using any of these round-trips its geometry/materials/instances
/// correctly; those specific facts are silently dropped.
///
/// A face a few millionths of an inch off its own fitted plane (common in real files) is
/// auto-triangulated rather than rejected, mirroring real SketchUp's own tolerance.

#include <string>

#include <openskp/model.hpp>

namespace openskp {

/// Generate C++ source that, when its `build()` function is called, rebuilds `model` from
/// scratch via `create.hpp`. See this header's own docstring for exactly what is and isn't
/// reproduced.
OPENSKP_EXPORT std::string to_cpp_code(const SkpModel& model);

}  // namespace openskp
