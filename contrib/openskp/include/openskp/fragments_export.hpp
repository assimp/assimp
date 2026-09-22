#pragma once

#include <cstdint>
#include <filesystem>
#include <vector>

#include <openskp/export.hpp>
#include <openskp/instanced_scene.hpp>

namespace openskp {

/// Builds a `.frag` (FlatBuffers) buffer from an InstancedScene - a direct
/// port of Python's openskp.export.fragments.to_fragments(), writing
/// against the same vendored ThatOpen Fragments schema
/// (include/openskp/_fragments_fb/index.fbs). See that module's own
/// docstring for the full rationale and the real-loader verification
/// status this port inherits.
///
/// `raw = false` (the default, matching ThatOpen's own IfcImporter
/// convention) deflate-compresses the output (zlib/RFC 1950) - the real
/// Fragments loader auto-detects either form. Pass `raw = true` for the
/// uncompressed FlatBuffers bytes directly.
OPENSKP_EXPORT std::vector<std::uint8_t> to_fragments(const InstancedScene& scene,
                                                      bool raw = false);

/// Exports an instanced scene directly to a `.frag` file.
OPENSKP_EXPORT void export_fragments(const InstancedScene& scene,
                                     const std::filesystem::path& output_path, bool raw = false);

/// Parses a real `.frag` file's bytes into an InstancedScene - the mirror
/// of to_fragments(). Accepts either the zlib-compressed wire format (the
/// default to_fragments()/real loader convention) or raw, uncompressed
/// FlatBuffers bytes; detected automatically the same way the real
/// @thatopen/fragments loader does, by attempting zlib inflation first.
///
/// Known gaps, matching Python's/TypeScript's/.NET's/Dart's own read side
/// exactly (not independently improved on here - see openskp#285):
/// InstancedNode::position_mm/properties/attribute_dictionaries are left
/// at their defaults, since the Fragments format doesn't carry them
/// separately from the `Name` attribute this function does extract.
OPENSKP_EXPORT InstancedScene from_fragments(const std::vector<std::uint8_t>& data);

/// Reads a `.frag` file from disk into an InstancedScene. See
/// from_fragments() for the full contract and known gaps.
OPENSKP_EXPORT InstancedScene read_fragments(const std::filesystem::path& path);

}  // namespace openskp
