#pragma once

/// \file edit.hpp
/// Load an existing legacy-format `.skp` file and rebuild it as a new, independent `SkpBuilder`.
///
/// C++ port of `openskp.edit` (packages/python/src/openskp/edit.py).
///
/// `create.hpp` only ever builds a brand-new file by splicing new geometry into its own bundled
/// blank scaffold (see that header's own docstring) - there is no way to append to or patch an
/// arbitrary existing file's bytes in place, because real SketchUp itself doesn't do that either:
/// it fully re-serializes the whole document on every save, so there is no stable "original
/// bytes + appended bytes" structure to target for a file this project didn't create.
///
/// This header takes the other viable approach instead: fully parse the existing file with this
/// project's own reader (`parser.hpp`/`legacy.cpp`, already comprehensive), then *replay*
/// everything it understood back through `create.hpp`'s own public API (materials, layers, every
/// component definition, every face/instance) to produce a brand-new file - not a byte-patched
/// copy of the original, but a freshly-built one with equivalent content, to which the caller can
/// add more geometry before saving.
///
/// **Adding more geometry after the fact.** The returned builder can take more `add_face`/
/// `add_circle`/`add_instance`/etc. calls, and every material/layer the source had is already
/// reachable via `builder->materials_by_name`/`builder->layers_by_name` (no separate lookup
/// needed - `open_existing` also returns a `definitions` map from each component definition's
/// name to its builder, for placing more instances of something the source already defined).
/// What the returned builder can no longer do is register a genuinely NEW material, layer, or
/// component definition/group - `create.hpp`'s own file-format ordering requirement
/// (materials/layers/definitions must all be finalized before any geometry is written) is
/// already satisfied by the time replay finishes writing the source's own root-level geometry
/// (which happens for any source file with root-level content - in practice, almost always), so
/// `add_material`/`add_layer`/`add_component_definition`/`add_group` all throw `SkpWriteError` on
/// the returned builder in that case. Build anything new into a separate `create()` call instead.
///
/// **Scope and known fidelity gaps** (every gap here is a genuine, deliberately-scoped
/// limitation carried over from the Python port, not an oversight):
///
/// * Only a **legacy-format** (SketchUp 2013-2020) source file is accepted - `create.hpp` never
///   writes any other format, so a modern VFF (2021+) source can't be faithfully round-tripped
///   through it.
/// * Per-edge `hidden`/`soft`/`smooth` flags are applied per-FACE, not per-edge (an "any edge in
///   this boundary has the flag" approximation) - `add_face` can only set these uniformly for
///   every edge it newly declares in one call, the same limitation any user of that API has.
/// * A positioned texture is replayed via 3 sample-point correspondences fitted to an affine map
///   (see `FaceOptions::front_uv`/`back_uv`) - exact at those 3 points, but a genuinely
///   projective (4-pin/distorted) source mapping won't interpolate identically between them. A
///   *projected* (draped) texture has no equivalent at all and falls back to the default
///   projection.
/// * A colorized (tinted) material variant is replayed as its plain source texture, losing the
///   tint.
/// * Per-face material/layer painting: only a face's front/back *material* is replayed - this
///   project's reader doesn't expose a per-face layer assignment at all (only instances carry an
///   explicit layer).
/// * Every placed thing (originally a group or a component instance alike) is replayed as a
///   plain component instance - structurally simpler, and visually identical, but no longer
///   shows as a "Group" in SketchUp's Outliner afterward.
/// * Section planes, text entities, and dimensions aren't carried over at all - the writer has no
///   support for any of these entity types.
/// * A circle/arc/polyline's original `CArcCurve`/`CCurve` grouping is lost - this project's
///   reader doesn't preserve that grouping in its public `Face`/`Edge` model, so a round-tripped
///   circle becomes an ordinary straight-edged face.
/// * Definition-level and face-level custom attributes aren't reproduced - the reader's public
///   model doesn't expose either (only an instance's own `properties` are).
///
/// Every one of these gaps is surfaced in the returned `warnings` list wherever it actually
/// affected the specific source file being replayed, not just documented here.

#include <filesystem>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include <openskp/create.hpp>
#include <openskp/export.hpp>
#include <openskp/model.hpp>

namespace openskp {

/// Result of `open_existing` - see that function's docstring for the full contract.
struct OPENSKP_EXPORT OpenExistingResult {
  /// Ready for more add_face/add_circle/add_instance/etc. calls before `SkpBuilder::save`. Every
  /// material and layer the source file had is already reachable via `builder->materials_by_name`
  /// / `builder->layers_by_name`.
  std::unique_ptr<SkpBuilder> builder;
  /// Anything from the source file that couldn't be faithfully reproduced - see this header's own
  /// docstring for the exact, deliberately-scoped gaps this draws from.
  std::vector<std::string> warnings;
  /// Maps each replayed component definition's own name to its (already-closed)
  /// `ComponentDefinitionBuilder`, owned by `builder` - so the caller can place additional
  /// instances of something the source file already defined via `builder->add_instance
  /// (*definitions["Wheel"], {.translation = ...})`. If two source definitions share a name, the
  /// later one wins - real SketchUp allows duplicate component names, this project's writer
  /// doesn't need them to be unique, only this convenience lookup does. Definitions with an empty
  /// name are not reachable through this map (but are still replayed and instantiable through
  /// their instances).
  std::map<std::string, ComponentDefinitionBuilder*> definitions;
};

/// Parse `path` (a legacy-format `.skp` file) and rebuild it as a new `SkpBuilder`, replaying
/// materials, layers, every component definition, and all root-level geometry/instances.
///
/// \throws SkpWriteError if `path` isn't a legacy-format file, or can't be read.
OPENSKP_EXPORT OpenExistingResult open_existing(const std::filesystem::path& path);

}  // namespace openskp
