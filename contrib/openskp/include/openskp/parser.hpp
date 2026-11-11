#pragma once

#include <filesystem>
#include <utility>

#include <openskp/instanced_scene.hpp>
#include <openskp/model.hpp>
#include <openskp/observability.hpp>
#include <openskp/scene.hpp>

namespace openskp {

/// Parses an SKP buffer into its definitions, geometry, materials, layers, and styles.
///
/// The buffer is passed by value. Use `std::move(buffer)` to transfer an existing buffer without
/// copying it. Parsing is synchronous, and any configured progress or log callbacks are invoked on
/// the calling thread.
///
/// @param buffer Complete contents of an SKP file.
/// @param options Optional progress and logging callbacks.
/// @return A parsed model that owns all data exposed by its public API.
/// @throws SkpParseError if the buffer is not a supported or well-formed SKP file.
OPENSKP_EXPORT SkpModel parse_skp(ByteBuffer buffer, const ParseOptions& options = {});

/// Parses an SKP buffer and bakes it into world-space, GLB-ready scene data.
///
/// Scene construction reparses the supplied buffer; it does not require or reuse a `SkpModel`.
/// Component transforms and material inheritance are resolved during baking.
///
/// @param buffer Complete contents of an SKP file. Use `std::move(buffer)` to avoid a copy.
/// @param options Optional progress and logging callbacks for parsing and scene construction.
/// @return Baked scene hierarchy, mesh metadata, primitives, and glTF material data.
/// @throws SkpParseError if parsing or scene construction fails, including recursive definitions.
OPENSKP_EXPORT Scene build_scene(ByteBuffer buffer, const ParseOptions& options = {});

/// Parses an SKP buffer and builds the placed scene graph with SketchUp's
/// component/group instancing PRESERVED, instead of baked into
/// world-space vertex data.
///
/// Use this instead of build_scene() when the model reuses components:
/// build_scene() bakes each placement into its own world-space vertex
/// buffers, so its output grows with `definition geometry x placement
/// count`, while this grows with `unique geometry + instance transforms`.
/// A component placed 1,000 times costs one copy of its geometry here.
///
/// @param buffer Complete contents of an SKP file. Use `std::move(buffer)` to avoid a copy.
/// @param options Optional progress and logging callbacks for parsing and scene construction.
/// @return Placed instance tree, mesh resources, and glTF material data.
/// @throws SkpParseError if parsing or scene construction fails, including recursive definitions.
OPENSKP_EXPORT InstancedScene build_instanced_scene(ByteBuffer buffer,
                                                    const ParseOptions& options = {});

/// In-memory handle for repeatedly parsing or baking one SKP source.
///
/// A `SkpFile` owns the source bytes. Calling `parse()` or `build_scene()` does not mutate the
/// handle, so either operation may be called multiple times. Each call starts from the original
/// bytes and performs an independent parse.
class OPENSKP_EXPORT SkpFile {
 public:
  /// Reads an SKP file into memory.
  ///
  /// This performs file I/O and validates the `.skp` extension, but defers format validation and
  /// parsing until `parse()` or `build_scene()` is called. Extension matching is case-insensitive.
  ///
  /// @param path Path to an existing SKP file.
  /// @return A handle owning the file contents.
  /// @throws std::filesystem::filesystem_error if the path does not exist.
  /// @throws std::invalid_argument if the path does not have an `.skp` extension.
  /// @throws std::runtime_error if the file cannot be opened, sized, or read.
  static SkpFile open(const std::filesystem::path& path);

  /// Creates an SKP handle from bytes already held in memory.
  ///
  /// The buffer is passed by value and retained by the returned handle. Use `std::move(buffer)` to
  /// transfer ownership without copying. No format validation occurs until parsing begins.
  ///
  /// @param buffer Complete contents of an SKP file.
  /// @return A handle owning the supplied bytes.
  static SkpFile from_buffer(ByteBuffer buffer);

  /// Parses the stored source bytes into a new model.
  ///
  /// @param options Optional progress and logging callbacks.
  /// @return A newly parsed model that owns its exposed data.
  /// @throws SkpParseError if the source is not a supported or well-formed SKP file.
  SkpModel parse(const ParseOptions& options = {}) const;

  /// Parses the stored source and bakes a new world-space scene.
  ///
  /// This operation reparses the original source independently of any previous `parse()` call.
  ///
  /// @param options Optional progress and logging callbacks for parsing and scene construction.
  /// @return Baked scene hierarchy, mesh metadata, primitives, and glTF material data.
  /// @throws SkpParseError if parsing or scene construction fails, including recursive definitions.
  Scene build_scene(const ParseOptions& options = {}) const;

  /// Parses the stored source and builds a new instanced scene, with
  /// SketchUp's component/group instancing PRESERVED. See the free
  /// function build_instanced_scene() for the full explanation.
  ///
  /// This operation reparses the original source independently of any
  /// previous `parse()`/`build_scene()` call.
  ///
  /// @param options Optional progress and logging callbacks for parsing and scene construction.
  /// @return Placed instance tree, mesh resources, and glTF material data.
  /// @throws SkpParseError if parsing or scene construction fails, including recursive definitions.
  InstancedScene build_instanced_scene(const ParseOptions& options = {}) const;

 private:
  explicit SkpFile(ByteBuffer data) : data_(std::move(data)) {}

  ByteBuffer data_;
};
}  // namespace openskp
