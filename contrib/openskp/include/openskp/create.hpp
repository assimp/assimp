#pragma once

/// \file create.hpp
/// Create new legacy-format (v17) `.skp` files from scratch.
///
/// This is a C++ port of `openskp.create` (packages/python/src/openskp/create.py) - a genuine,
/// from-scratch binary writer for the same MFC `CArchive` object-stream format \ref legacy.cpp
/// reads, built by inverting that reader's own, already-proven decoding logic (the class-ref/
/// back-ref protocol, entity preambles, drawbase records), then validated against the Python
/// port's own SketchUp-SDK-cross-checked output. No SketchUp SDK is called at runtime; this
/// module never links against or shells out to any proprietary library.
///
/// **Scope (matches the Python writer this ports):** faces built directly from vertex
/// coordinates, sharing vertices/edges automatically wherever coordinates coincide exactly;
/// solid-color and PNG/JPEG-textured materials; named layers; reusable component definitions
/// with multiple positioned instances; groups (self-placing, structurally almost identical to a
/// component instance); nested definitions and nested group instances (built inside another
/// definition's own body via `ComponentDefinitionBuilder::add_instance` /
/// `add_group_instance` - a nested group can't be declared inline the way `SkpBuilder::
/// add_group` is at the root level, since this format has no way to embed one definition's
/// declaration inside another's); per-instance rotation (axis+angle convenience) and hidden
/// state; explicit per-side texture positioning (`FaceOptions::front_uv`/`back_uv`) on a face of
/// any orientation; custom attribute dictionaries on definitions/instances/faces/groups (the same
/// mechanism SketchUp's own "dynamic component" attributes use - a group only pays for a real
/// attribute container when it actually has one, unlike a component instance, which always
/// carries a real, if often empty, one); real, editable-by-radius
/// `CArcCurve` circles/arcs (`add_circle`/`add_arc`) and freeform `CCurve` polylines
/// (`add_polyline`); faces with one or more holes; `auto_triangulate` fan-splitting a
/// non-coplanar polygon into real, always-planar triangles instead of raising (mirrors real
/// SketchUp's own UI behavior for a not-quite-flat quad).
///
/// Coordinates are in **inches** - SketchUp's own native internal unit for this era of the
/// format. Converting from another unit is the caller's responsibility.
///
/// Every file opens to the standard "Iso" view (parallel projection, looking at the origin from
/// the (1, -1, 1) octant) rather than the blank scaffold's own arbitrary default camera. Not
/// configurable yet.
///
/// This module only ever builds a brand-new file from its own blank scaffold - it has no notion
/// of an existing input file at all (real SketchUp re-serializes the whole document on every
/// save, so there is no stable "original bytes + appended bytes" structure to target the way
/// there is for the blank scaffold). \ref edit.hpp builds on top of this module and the legacy
/// reader to load an *existing* legacy file by fully parsing it and replaying its content back
/// through this module's own API - see that header's docstring for the exact scope and gaps.
///
/// **The blank scaffold, and why it's there.** Every legacy `.skp` file carries a
/// header/material-manager/style-and-font-manager region this project has not fully
/// reverse-engineered - only enough of it is understood to preserve it byte-for-byte and
/// correctly renumber the handful of internal references inside it that shift when new geometry
/// is inserted. Rather than guess at synthesizing that region from scratch, new files are built
/// by splicing genuinely-written geometry into a bundled minimal empty-document template
/// embedded as `openskp::detail::kScaffoldBlankV17` (see `src/scaffold_blank_v17.hpp`, generated
/// byte-for-byte from `packages/python/src/openskp/_scaffold/blank_v17.skp`).
///
/// That template's bytes came from Trimble's own official SketchUp SDK during the Python
/// writer's research phase (`SUModelCreate` + a bare `SUModelSaveToFileWithVersion` call,
/// nothing else) - disclosed here plainly rather than hidden. Its content is SketchUp's own
/// built-in empty-document boilerplate (default style, default "Layer0", references to system
/// fonts like Arial/Tahoma) - the same bytes any brand-new SketchUp document contains regardless
/// of who created it, not anyone's creative work or user/client data. The actual value in this
/// module - the entity byte-encoding, the object-graph protocol, the specific flag bytes real
/// SketchUp silently requires, the tail-reference renumbering - is 100% independently
/// reverse-engineered (by the Python port this mirrors) and written from scratch here in C++; no
/// SDK call happens at any point in this module.
///
/// This C++ port additionally bakes the handful of scaffold-derived *positions* the Python
/// writer's `SkpBuilder.__init__` computes at runtime (by walking the scaffold with the generic
/// archive reader) into compile-time constants instead - see the comment on
/// `detail::kMaterialInsertPos` and neighbors in create.cpp. Because the scaffold is a fixed,
/// version-controlled blob (byte-for-byte identical to the Python copy, sha256-checked), those
/// positions are themselves fixed for as long as the scaffold is; re-deriving them generically at
/// every `create()` call would require porting a large fraction of the generic MFC archive reader
/// for no behavioral difference. If the scaffold is ever swapped, re-run the Python builder
/// against the new file and update both the byte array and these constants together (see
/// create.cpp's top-of-file comment for the exact recipe).

#include <array>
#include <cstdint>
#include <filesystem>
#include <map>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include <openskp/export.hpp>
#include <openskp/model.hpp>

namespace openskp {

/// Raised when a `.skp` file cannot be constructed by `SkpBuilder`/`ComponentDefinitionBuilder`.
class OPENSKP_EXPORT SkpWriteError : public std::runtime_error {
 public:
  explicit SkpWriteError(const std::string& message) : std::runtime_error(message) {}
};

/// A 3D point (x, y, z) in inches - SketchUp's native unit for this format era.
using Point3 = std::array<double, 3>;

/// Row-major 3x3 transform matrix (rotation/scale) - same convention `InstanceOptions::
/// matrix3x3` and `GroupOptions::matrix3x3` use.
using Matrix3x3 = std::array<double, 9>;

/// A rotation given as (axis, angle_radians), right-hand rule - axis need not be a unit vector.
/// An alternative to hand-deriving `Matrix3x3` for the common case of a pure rotation.
using Rotation = std::pair<Point3, double>;

/// One (world point, (u, v)) correspondence for explicit texture positioning.
using UvPoint = std::pair<Point3, std::array<double, 2>>;

/// Exactly 3 `UvPoint` correspondences - the minimum that fully determines an affine UV mapping
/// (scale/rotation/shear/translation, no perspective) - see `FaceOptions::front_uv`/`back_uv`.
using UvCorrespondence = std::vector<UvPoint>;

/// A custom attribute value - the same 3 types SketchUp's own "dynamic component" attributes
/// support in this writer (see `legacy.cpp`'s `_read_attr_named` equivalent for the full set this
/// *format* supports; only these 3 are exposed here).
using AttributeValue = std::variant<std::string, std::int32_t, double>;
/// A named dictionary of custom key/value metadata, attached to a definition/instance/face via
/// each of their `attributes`/`attribute_dict_name` options - the same mechanism SketchUp's own
/// "dynamic component" attributes use.
using AttributeDict = std::map<std::string, AttributeValue>;

class ComponentDefinitionBuilder;

/// Options for `SkpBuilder::add_face` / `ComponentDefinitionBuilder::add_face`.
struct FaceOptions {
  /// Front-side material slot (from `add_material`/`add_texture_material`); unset = default.
  std::optional<int> material;
  /// Layer slot (from `add_layer`); unset = default (Layer0).
  std::optional<int> layer;
  /// Back-side material slot; unset = default.
  std::optional<int> back_material;
  /// Hides the face itself (SketchUp's "Hide" on this specific face).
  bool hidden = false;
  /// Applied to any edge NEWLY declared by this call (not one already shared with a previous
  /// face, which keeps whatever flags it was first declared with).
  bool soft_edges = false;
  bool smooth_edges = false;
  bool hidden_edges = false;
  /// Explicit front-side texture positioning - exactly 3 (point, (u, v)) correspondences -
  /// instead of the default planar projection. Works on a face of any orientation.
  std::optional<UvCorrespondence> front_uv;
  std::optional<UvCorrespondence> back_uv;
  /// Custom key/value metadata attached to this face, under a dictionary named
  /// `attribute_dict_name`.
  AttributeDict attributes;
  std::string attribute_dict_name = "attributes";
  /// If `points` is not coplanar, fan-triangulate from `points[0]` into real, always-planar
  /// triangles (2 for a quad) instead of raising - mirrors real SketchUp's own UI behavior for a
  /// not-quite-flat polygon. Each triangle gets its own copy of `attributes`. Not compatible with
  /// `front_uv`/`back_uv` or `holes`. Already-planar input is written as a single face either
  /// way - this only changes behavior for input that would otherwise be rejected.
  bool auto_triangulate = false;
  /// Independent closed polygons cut out of the face (e.g. a window in a wall). Winding
  /// direction doesn't matter. Every hole's points must lie on the same plane as the outer
  /// boundary. Not combined with `auto_triangulate`.
  std::vector<std::vector<Point3>> holes;
};

/// Options for `SkpBuilder::add_circle` / `ComponentDefinitionBuilder::add_circle`.
struct CircleOptions {
  /// Tessellation segment count (3-255); matches SketchUp's own circle tool default of 24.
  int num_segments = 24;
  std::optional<int> material;
  std::optional<int> layer;
  std::optional<int> back_material;
  bool hidden = false;
  std::optional<UvCorrespondence> front_uv;
  std::optional<UvCorrespondence> back_uv;
  AttributeDict attributes;
  std::string attribute_dict_name = "attributes";
};

/// Options for `SkpBuilder::add_arc` / `ComponentDefinitionBuilder::add_arc`.
struct ArcOptions {
  int num_segments = 24;
  bool hidden_edges = false;
  bool soft_edges = false;
  bool smooth_edges = false;
};

/// Options for `SkpBuilder::add_polyline` / `ComponentDefinitionBuilder::add_polyline`.
struct PolylineOptions {
  /// Also connects the last point back to the first.
  bool closed = false;
  bool hidden_edges = false;
  bool soft_edges = false;
  bool smooth_edges = false;
};

/// Options for `SkpBuilder::add_component_definition`.
struct DefinitionOptions {
  /// Custom key/value metadata attached to the definition itself, under a dictionary named
  /// `attribute_dict_name`.
  AttributeDict attributes;
  std::string attribute_dict_name = "attributes";
};

/// Options for `SkpBuilder::add_layer`.
struct LayerOptions {
  /// This layer's own display color; unset keeps the previous all-zero-byte default.
  std::optional<Color4> color;
  /// Sets the layer's own visibility (SketchUp's layer-panel checkbox).
  bool hidden = false;
};

/// Options for `SkpBuilder::add_instance` / `ComponentDefinitionBuilder::add_instance`.
struct InstanceOptions {
  std::optional<std::string> name;
  Point3 translation{0.0, 0.0, 0.0};
  /// Row-major 3x3 rotation/scale matrix (identity if unset). Pass at most one of `matrix3x3`/
  /// `rotation`.
  std::optional<Matrix3x3> matrix3x3;
  std::optional<Rotation> rotation;
  /// Applied to the instance itself (not its contents).
  std::optional<int> material;
  std::optional<int> layer;
  /// Custom key/value metadata attached to this instance specifically (as opposed to its
  /// definition), under a dictionary named `attribute_dict_name`.
  AttributeDict attributes;
  std::string attribute_dict_name = "attributes";
  /// Hides this specific placement (SketchUp's "Hide" on the instance) - its contents still
  /// exist in the file, just not shown by default.
  bool hidden = false;
};

/// Options for `SkpBuilder::add_group`.
struct GroupOptions {
  /// Defaults to "Group" if unset.
  std::optional<std::string> name;
  Point3 translation{0.0, 0.0, 0.0};
  std::optional<Matrix3x3> matrix3x3;
  std::optional<Rotation> rotation;
  std::optional<int> material;
  std::optional<int> layer;
  /// Custom key/value metadata attached to this group, under a dictionary named
  /// `attribute_dict_name`. A group only gets a real attribute container when this is
  /// non-empty - matching `FaceOptions::attributes`' own conditional pattern (openskp#261).
  AttributeDict attributes;
  std::string attribute_dict_name = "attributes";
  bool hidden = false;
};

/// Options for `ComponentDefinitionBuilder::add_group_instance`. Same shape as `GroupOptions`.
struct GroupInstanceOptions {
  std::optional<std::string> name;
  Point3 translation{0.0, 0.0, 0.0};
  std::optional<Matrix3x3> matrix3x3;
  std::optional<Rotation> rotation;
  std::optional<int> material;
  std::optional<int> layer;
  AttributeDict attributes;
  std::string attribute_dict_name = "attributes";
  bool hidden = false;
};

/// Options for `SkpBuilder::add_image`.
struct ImageOptions {
  Point3 translation{0.0, 0.0, 0.0};
  /// Row-major 3x3 rotation/scale matrix (identity if unset). Pass at most one of `matrix3x3`/
  /// `rotation`.
  std::optional<Matrix3x3> matrix3x3;
  std::optional<Rotation> rotation;
  std::optional<int> layer;
  bool hidden = false;
};

/// Accumulates one component/group definition's geometry. Construct via
/// `SkpBuilder::add_component_definition` or `SkpBuilder::add_group`, not directly.
///
/// The Python port this mirrors uses a `with` context manager: geometry is added between
/// `add_component_definition(...)` and the `with` block's exit, which both finalizes the
/// definition's entity count and (for a group) places it. C++ has no directly equivalent
/// language construct, so this class exposes that same two-phase lifecycle explicitly via
/// `close()`: add geometry/instances, then call `close()` exactly once before the definition can
/// be placed (`SkpBuilder::add_instance`) or the file finalized (`SkpBuilder::to_bytes`/`save`,
/// which close it implicitly if you forget - see their own docs). Every mutating method throws
/// `SkpWriteError` if called after `close()`.
///
/// \code
/// auto& chair = builder->add_component_definition("Chair");
/// chair.add_face({{0, 0, 0}, {20, 0, 0}, {20, 20, 0}, {0, 20, 0}});
/// chair.close();
/// builder->add_instance(chair, {.translation = {100, 0, 0}});
/// \endcode
class OPENSKP_EXPORT ComponentDefinitionBuilder {
 public:
  ~ComponentDefinitionBuilder();
  ComponentDefinitionBuilder(const ComponentDefinitionBuilder&) = delete;
  ComponentDefinitionBuilder& operator=(const ComponentDefinitionBuilder&) = delete;
  ComponentDefinitionBuilder(ComponentDefinitionBuilder&&) = delete;
  ComponentDefinitionBuilder& operator=(ComponentDefinitionBuilder&&) = delete;

  /// This definition's own name.
  const std::string& name() const noexcept;
  /// True once `close()` has been called - no further geometry/instances may be added.
  bool closed() const noexcept;
  /// This definition's own archive slot number. Not normally needed directly - `add_instance`/
  /// `add_group_instance` take the `ComponentDefinitionBuilder` itself - but exposed for parity
  /// with the Python port's public `.slot` attribute.
  int slot() const noexcept;

  /// Add one planar face to this definition - same behavior as `SkpBuilder::add_face`, except
  /// vertices/edges are shared only within this definition, never with the root model or other
  /// definitions.
  void add_face(const std::vector<Point3>& points, const FaceOptions& options = {});
  /// Add one circular face - same behavior as `SkpBuilder::add_circle`.
  void add_circle(Point3 center, Point3 normal, double radius, const CircleOptions& options = {});
  /// Add one partial (open) arc - same behavior as `SkpBuilder::add_arc`.
  void add_arc(Point3 center, Point3 normal, double radius, double start_angle, double end_angle,
               const ArcOptions& options = {});
  /// Add one freeform polyline curve - same behavior as `SkpBuilder::add_polyline`.
  void add_polyline(const std::vector<Point3>& points, const PolylineOptions& options = {});

  /// Place one instance of another, already-closed component definition inside this one - the
  /// same nesting real SketchUp supports (an assembly definition containing instances of its own
  /// sub-part definitions). `definition` must come from the same `SkpBuilder`, must already be
  /// closed, and must not be `*this` (this format's definitions are strictly ordered - a
  /// definition can only nest others fully built strictly before it was opened, which also rules
  /// out cycles).
  void add_instance(const ComponentDefinitionBuilder& definition,
                    const InstanceOptions& options = {});
  /// Place `definition` inside this one as a *group* (`CGroup`) rather than a component
  /// instance - otherwise identical to `add_instance`, including the same already-closed/
  /// same-builder/no-self-reference requirements. A nested group can't be declared inline (this
  /// format has no way to embed one definition's declaration inside another's) - build its
  /// geometry with a normal `add_component_definition` first, then place it here.
  void add_group_instance(const ComponentDefinitionBuilder& definition,
                          const GroupInstanceOptions& options = {});

  /// Finalize this definition: patches its entity count, writes its tail (GUID/name/thumbnail),
  /// and - if this definition was started via `SkpBuilder::add_group` - places it immediately at
  /// its group translation/matrix. Throws `SkpWriteError` if this definition has no geometry, or
  /// if it is already closed.
  void close();

 private:
  friend class SkpBuilder;
  struct Impl;
  explicit ComponentDefinitionBuilder(std::unique_ptr<Impl> impl);
  /// Throws if this definition is already closed - shared by every add_*
  /// method below. A member function (not a free function) specifically so
  /// it can name the private nested `Impl` type in its own signature -
  /// only SkpBuilder and ComponentDefinitionBuilder itself have that
  /// access, and a free function doesn't inherit either friendship even
  /// when every one of its callers already has it.
  void check_writable(const std::string& action) const;
  std::unique_ptr<Impl> impl_;
};

/// Accumulates geometry and writes it into a new legacy-format (v17) `.skp` file. Construct via
/// `create()`, not directly.
class OPENSKP_EXPORT SkpBuilder {
 public:
  SkpBuilder();
  ~SkpBuilder();
  SkpBuilder(const SkpBuilder&) = delete;
  SkpBuilder& operator=(const SkpBuilder&) = delete;
  SkpBuilder(SkpBuilder&&) = delete;
  SkpBuilder& operator=(SkpBuilder&&) = delete;

  /// Every material registered so far, by name - populated by `add_material`/
  /// `add_texture_material` as a side effect (they already de-dupe by name through this same
  /// map), not something a caller needs to maintain separately.
  std::map<std::string, int> materials_by_name;
  /// Every layer registered so far, by name - same pattern as `materials_by_name`.
  std::map<std::string, int> layers_by_name;

  /// Register a solid-color material and return a handle to pass as `FaceOptions::material`.
  /// `rgba` channels are 0-255. Calling this again with a name already registered returns the
  /// same handle rather than creating a duplicate material.
  ///
  /// All materials must be added before the first `add_face` call - the geometry section's slot
  /// numbering is fixed once writing begins, and depends on the final material count. They must
  /// also come before any `add_layer` or `add_component_definition` call - materials are
  /// spliced in earlier in the file, so both of those sections' own slot numbering depends on
  /// the final material count too.
  int add_material(const std::string& name, Color4 rgba,
                   std::optional<double> opacity = std::nullopt);
  /// Overload taking an opaque (alpha = 255) RGB color.
  int add_material(const std::string& name, Color3 rgb,
                   std::optional<double> opacity = std::nullopt);

  /// Register an image-textured material from a local PNG or JPEG file and return a handle to
  /// pass as `FaceOptions::material`. The format is detected from the file's own magic bytes,
  /// not its extension. Same ordering rules as `add_material`.
  ///
  /// `applied_height`/`applied_width`, if given, are the applied size in INCHES - how much model
  /// space one tile of the image covers. Both default to 1.0. A texture applied without
  /// positioning carries no per-face UV record, so this pair IS its mapping - see
  /// `write_textured_material`'s own note in create.cpp for why it matters even for
  /// `FaceOptions::front_uv`/`back_uv` pinning (a positioned mapping still divides by it).
  /// `applied_width` and `opacity` sit after `applied_height` (not alongside it) so an existing
  /// positional call passing `applied_height` as the 3rd argument keeps meaning what it always
  /// meant.
  int add_texture_material(const std::string& name, const std::filesystem::path& image_path,
                           std::optional<double> applied_height = std::nullopt,
                           std::optional<double> applied_width = std::nullopt,
                           std::optional<double> opacity = std::nullopt);

  /// Register a layer and return a handle to pass as `FaceOptions::layer`. Calling this again
  /// with a name already registered returns the same handle (`options` are ignored on a repeat
  /// call - only the first registration sets them). Must be added before the first `add_face`
  /// call and before any `add_component_definition` call, for the same reasons as `add_material`.
  int add_layer(const std::string& name, const LayerOptions& options = {});

  /// Start a new reusable component definition. Add its geometry via the returned builder's
  /// `add_face`/etc., then call `.close()` on it, then pass it to `add_instance` to place copies
  /// of it in the model. Must be called before any `add_face`/`add_instance` call on this
  /// builder itself - component definitions splice in after materials and layers, before
  /// root-level geometry, so their slot numbering depends on the final material and layer
  /// counts. The returned reference remains valid for the lifetime of this `SkpBuilder`.
  ComponentDefinitionBuilder& add_component_definition(const std::string& name,
                                                       const DefinitionOptions& options = {});

  /// Start a new group. Add its geometry via the returned builder's `add_face`/etc.; the group
  /// is placed at `options.translation`/`matrix3x3` automatically when `.close()` is called on
  /// it (unlike `add_component_definition`, there is no separate placement call). Same ordering
  /// rule as `add_component_definition`.
  ComponentDefinitionBuilder& add_group(const GroupOptions& options = {});

  /// Place one instance of `definition` (already closed) in the model. `options.matrix3x3` is a
  /// row-major 3x3 rotation/scale matrix (identity if unset); `options.translation` is applied
  /// after it, in inches.
  void add_instance(const ComponentDefinitionBuilder& definition,
                    const InstanceOptions& options = {});

  /// Place a SketchUp Image entity (File > Import > Image) - a picture placed as its own object,
  /// distinct from painting a texture material onto an ordinary face (an Image gets its own
  /// Outliner classification and explode behavior a plain textured face doesn't).
  ///
  /// `width`/`height` size the image's quad in inches; the image covers it edge to edge,
  /// undistorted regardless of the source file's own pixel aspect ratio (get the ratio right
  /// yourself if that matters - this does not auto-derive it). `options.translation`/
  /// `matrix3x3`/`rotation`/`hidden` place it exactly like `add_instance` - the quad starts in
  /// the XY plane; rotate it to stand upright (e.g. on a wall) the same way you would any other
  /// placement. `options.layer`, if given, is a handle from `add_layer`.
  ///
  /// \code
  /// builder->add_image("photo.jpg", 48, 36,
  ///                    {.translation = {0, 0, 40}, .rotation = Rotation{{1, 0, 0}, M_PI / 2}});
  /// \endcode
  ///
  /// Must be called before any `add_layer`/`add_component_definition`/`add_group`/`add_face`/
  /// `add_instance` call - like `add_texture_material` (which this calls internally to register
  /// the image itself), it needs a material, and this writer's file format requires every
  /// material to be registered before any geometry section begins.
  ///
  /// The image's quad and UV mapping are pinned explicitly (`FaceOptions::front_uv`), not left
  /// to the default per-material tile-size projection - the read-side UV formula divides by the
  /// material's applied height even for a pinned mapping, and `add_texture_material`'s default
  /// height (1.0) makes that division a no-op against this method's own 0..1 pins.
  ///
  /// Unlike every other entity this writer produces, CImage's exact binary schema version (see
  /// `kImageSchema` in create.cpp) is a best-effort guess, not calibrated against a real
  /// SketchUp-authored Image entity - none was available. This project's own reader round-trips
  /// the result correctly, but real SketchUp's acceptance of the file is unverified beyond the
  /// Python port's own real-SketchUp test (placement/orientation/texture all confirmed correct
  /// there - see CHECKLIST.md).
  void add_image(const std::filesystem::path& image_path, double width, double height,
                 const ImageOptions& options = {});

  /// Add one planar face, defined by 3+ coplanar points (inches) forming a closed polygon in
  /// order - do not repeat the first point at the end. Vertices/edges are automatically shared
  /// with previously-added faces wherever a point's (x, y, z) coordinates match exactly (same
  /// float values).
  void add_face(const std::vector<Point3>& points, const FaceOptions& options = {});

  /// Add one circular face - a true SketchUp circle (editable by radius, re-tessellatable,
  /// selectable as a single "Curve" entity), not `num_segments` disconnected straight edges that
  /// merely happen to trace that shape. `normal` need not be a unit vector.
  void add_circle(Point3 center, Point3 normal, double radius, const CircleOptions& options = {});

  /// Add one partial (open) arc - a genuine SketchUp arc entity, not disconnected straight edges
  /// that merely trace that shape. Unlike `add_circle`, this creates edges only, no face.
  /// `start_angle`/`end_angle` (radians) measure the sweep from an arbitrary but fixed 0-angle
  /// reference direction in the arc's plane (chosen automatically, consistently for a given
  /// `normal`) - sweeps in either direction, and sweeps beyond a full turn, are both valid.
  void add_arc(Point3 center, Point3 normal, double radius, double start_angle, double end_angle,
               const ArcOptions& options = {});

  /// Add one freeform polyline curve - a chain of straight edges (`points` in order, at least 2)
  /// grouped into one genuine SketchUp "Curve" entity, not disconnected individual edges that
  /// merely happen to connect end-to-end. No face, unlike `add_face`.
  void add_polyline(const std::vector<Point3>& points, const PolylineOptions& options = {});

  /// Add a FREE linear dimension between two explicit points (inches, world space). `offset` is
  /// the dimension line's offset from the measured segment, in inches (signed).
  void add_dimension(Point3 p1, Point3 p2, double offset = 10.0);

  /// Add a leader text (SketchUp's Text tool) anchored at `point` (inches, world space), with the
  /// label floating at `point + leader` and a leader line joining them.
  void add_text(const std::string& text, Point3 point, Point3 leader = {15.0, 15.0, 15.0});

  /// Add a construction/guide line (SketchUp's Construction Line tool). Pass exactly one of
  /// `point2` (a bounded segment between `point` and `point2`, matching
  /// `Entities#add_cline(p1, p2)`) or `direction` (an unbounded guide line through `point`,
  /// matching `Entities#add_cline(point, vector)`).
  void add_construction_line(Point3 point, std::optional<Point3> point2 = std::nullopt,
                             std::optional<Point3> direction = std::nullopt);

  /// Add a construction/guide point (SketchUp's Construction Point tool) at `position` (inches,
  /// world space).
  void add_construction_point(Point3 position);

  /// Add a section plane (SketchUp's Section Plane tool) through `point` with the given `normal`
  /// (need not be unit length), matching `Entities#add_section_plane([point, normal])`.
  void add_section_plane(Point3 point, Point3 normal);

  /// Return the finished file's bytes.
  ByteBuffer to_bytes();
  /// Write the finished file to `path`.
  void save(const std::filesystem::path& path);

 private:
  friend class ComponentDefinitionBuilder;
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

/// Start building a new legacy-format (v17) `.skp` file from scratch.
///
/// \code
/// auto builder = create();
/// int red = builder->add_material("Red", Color3{255, 0, 0});
/// int roof = builder->add_layer("Roof");
/// FaceOptions opts;
/// opts.material = red;
/// opts.layer = roof;
/// builder->add_face({{0, 0, 0}, {100, 0, 0}, {100, 100, 0}, {0, 100, 0}}, opts);
/// builder->save("output.skp");
/// \endcode
OPENSKP_EXPORT std::unique_ptr<SkpBuilder> create();

}  // namespace openskp
