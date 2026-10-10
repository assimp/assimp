#pragma once

#include <array>
#include <cstdint>
#include <deque>
#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <openskp/export.hpp>
#include <openskp/observability.hpp>

namespace openskp {

using EntityId = std::int64_t;
using ByteBuffer = std::vector<std::uint8_t>;
using Vec3 = std::array<double, 3>;
using Color3 = std::array<std::uint8_t, 3>;
using Color4 = std::array<std::uint8_t, 4>;

// 1 metre = 39.37007874015748 inches (SketchUp native unit). Shared by
// every exporter that takes a coordinate scale factor (dxf_export.hpp,
// ifc_export.hpp, ...) - defined once here rather than duplicated per
// header, since two same-named constexprs in the same namespace collide
// as a redefinition error in any translation unit that includes both.
constexpr double METRES_TO_INCHES = 39.37007874015748;

/// 3D point coordinate in raw SketchUp space.
struct Vertex {
  /// Unique TLV entity ID.
  EntityId id{};
  /// X coordinate in raw model space (Z-up, inches).
  double x{};
  /// Y coordinate in raw model space (Z-up, inches).
  double y{};
  /// Z coordinate in raw model space (Z-up, inches).
  double z{};
};

/// Straight line segment connecting two vertices.
struct Edge {
  /// Unique TLV entity ID.
  EntityId id{};
  /// Start vertex ID.
  EntityId v1_id{};
  /// End vertex ID.
  EntityId v2_id{};
  /// Soft edge flag (suppresses edge rendering across coplanar faces).
  bool soft{};
  /// Smooth edge flag (enables vertex normal interpolation).
  bool smooth{};
  /// Hidden edge flag (explicit "Hide" bit on this edge).
  bool hidden{};
};

/// Oriented edge reference within a face loop.
struct CoEdge {
  /// Target edge ID.
  EntityId edge_id{};
  /// Orientation relative to edge (+1 = same direction, -1 = reversed).
  std::int64_t orientation{};
};

/// Planar polygon bounded by outer and inner (hole) edge loops.
struct Face {
  /// Unique TLV entity ID.
  EntityId id{};
  /// Polygonal loops (loops[0] = outer boundary, loops[1..N] = holes).
  std::vector<std::vector<CoEdge>> loops;
  /// Unit surface normal vector [nx, ny, nz].
  std::optional<Vec3> normal;
  /// Front-face material ID (references SkpModel::materials).
  std::optional<EntityId> material_id;
  /// Back-face material ID (references SkpModel::materials).
  std::optional<EntityId> back_material_id;
  /// Front-face 3x3 row-major UV transform matrix.
  std::optional<std::array<double, 9>> uv_transform;
  /// Back-face 3x3 row-major UV transform matrix.
  std::optional<std::array<double, 9>> uv_transform_back;
  /// The texture is PROJECTED (e.g. the Add Location terrain drape): its
  /// UVs run in the projection plane's frame, not the face frame.
  bool uv_projected{};
  /// Same for the face's back side.
  bool uv_projected_back{};
  /// Whether the face is hidden (SketchUp's "Hide" on this specific face,
  /// not a layer/tag visibility toggle).
  bool hidden{};
};

/// Organization layer (tag) for model elements.
struct Layer {
  /// Layer name (e.g. "Layer0", "Walls").
  std::string name;
  /// Display color RGB [0..255].
  Color3 color{200, 200, 200};
  /// Whether the layer's visibility is switched off. Only populated for
  /// legacy (pre-2021 MFC) files, where the byte is read directly from the
  /// layer record - modern (VFF) files derive layers from
  /// Layer_<name>-prefixed materials, which carry no visibility data, so
  /// this is always false there.
  bool hidden{};
};

/// Embedded texture image data and metadata.
struct Texture {
  /// Original image filename inside the SKP ZIP archive.
  std::string filename;
  /// Horizontal scale factor.
  double width{};
  /// Vertical scale factor.
  double height{};
  /// Raw image file bytes (PNG/JPG/BMP).
  std::optional<ByteBuffer> data;
  /// Save texture image bytes to disk.
  OPENSKP_EXPORT void save(const std::filesystem::path& path) const;
};

/// Surface material properties (color, transparency, texture).
struct Material {
  /// Material name (e.g. "FrontColor", "Layer_Layer0").
  std::string name;
  /// Color channels RGBA [0..255].
  Color4 color{200, 200, 200, 255};
  /// Opacity factor [0.0 = fully transparent, 1.0 = fully opaque].
  double transparency{1.0};
  /// Unique TLV entity ID.
  std::optional<EntityId> id;
  /// Embedded texture reference.
  std::optional<Texture> texture;
  /// Whether material uses colorization.
  bool colorized{};
  /// Colorize mode enum identifier.
  std::int32_t colorize_type{};
};

/// Bundled rendering style settings.
struct Style {
  /// Style name.
  std::string name;
  /// Default front face color.
  std::optional<Color3> front_color;
  /// Default back face color.
  std::optional<Color3> back_color;
};

/// Component or group placement instance within a definition.
struct Instance {
  /// Instance name.
  std::string name;
  /// Reference index to the target definition.
  std::optional<EntityId> ref_idx;
  /// GUID string of target definition.
  std::string guid;
  /// 4x4 column-major affine transformation matrix (16 floats).
  std::vector<double> matrix;
  /// This instance's own explicit layer override, or "" when it has
  /// none. An instance without an explicit override inherits its
  /// *placement's* layer, which can only be resolved once the scene
  /// graph is flattened - see build_scene()'s InstanceNode::layer for
  /// that resolved value.
  std::string layer;
  /// Arbitrary key/value dynamic attributes attached directly to this
  /// instance (SketchUp's Dynamic Components) - a backward-compatible
  /// view of attribute_dictionaries["dynamic_attributes"].
  std::map<std::string, std::string> properties;
  /// Every custom attribute dictionary attached to this instance, keyed
  /// by the dictionary's own declared name - not just the one
  /// SketchUp's own Dynamic Components extension uses. A real instance
  /// routinely carries several at once (a plugin's own dictionary
  /// alongside SketchUp's, or several of the plugin's own).
  std::map<std::string, std::map<std::string, std::string>> attribute_dictionaries;
  /// Instance material override ID.
  std::optional<EntityId> material_id;
  /// Whether the instance itself is hidden (SketchUp's "Hide" on this
  /// specific component/group placement, not a layer/tag visibility
  /// toggle).
  bool hidden{};
};

/// Section plane entity.
struct SectionPlane {
  std::array<double, 4> plane{0.0, 0.0, 1.0, 0.0};
  std::string name;
  std::string label;
  bool hidden{false};
};

/// Text annotation entity.
struct TextEntity {
  std::string text;
  bool hidden{false};
};

/// A linear dimension (SketchUp's Dimension tool).
///
/// The legacy (pre-2021) reader recovers only `text`/`hidden`. The VFF
/// reader (2021+) recovers the full geometry - see `SkpModel::dimensions`
/// for the model-level, world-space list.
struct Dimension {
  /// The displayed text. Empty when the dimension shows its auto-computed
  /// measured value (the caller formats |b - a|).
  std::string text;
  bool hidden{false};
  /// First measured point (x, y, z) in inches (world space), or unset when
  /// only the text was recovered.
  std::optional<Vec3> a;
  /// Second measured point.
  std::optional<Vec3> b;
  /// Offset distance (inches) - how far the dimension line sits from the
  /// a-b segment, along the in-plane perpendicular.
  double offset{};
  /// The dimension plane's x-axis, or unset.
  std::optional<Vec3> plane_x;
  /// The dimension plane's normal, or unset.
  std::optional<Vec3> normal;
};

/// A construction/guide line (SketchUp's Construction Line tool).
/// Legacy (pre-2021) files only - the VFF (2021+) reader does not
/// currently recognize this entity.
///
/// Stored internally (and here, unchanged) as a point + normalized
/// direction + two signed distance parameters along that direction
/// marking where the visible segment starts/ends - the same shape
/// `Sketchup::ConstructionLine`'s own `start`/`end`/`direction`
/// properties expose. A parameter magnitude of `1e30` means unbounded
/// in that direction (SketchUp draws this as an infinite guide line
/// through `point`) - `start`/`end` come back unset in that case,
/// matching the real API returning `nil`.
struct ConstructionLine {
  /// A point on the line, in inches (world space) - matches the bounded
  /// case's own `start`, or the anchor point given for an infinite line.
  Vec3 point{0.0, 0.0, 0.0};
  /// The line's normalized direction vector.
  Vec3 direction{1.0, 0.0, 0.0};
  /// The bounded segment's start point, or unset if unbounded in this
  /// direction.
  std::optional<Vec3> start;
  /// The bounded segment's end point, or unset if unbounded.
  std::optional<Vec3> end;
};

/// A construction/guide point (SketchUp's Construction Point tool).
/// Legacy (pre-2021) files only - the VFF (2021+) reader does not
/// currently recognize this entity.
struct ConstructionPoint {
  /// The point's position, in inches (world space).
  Vec3 position{0.0, 0.0, 0.0};
};

/// A saved scene (SketchUp's "Scenes" tabs; "pages" in the SDK).
struct Page {
  /// Scene name as shown on its tab.
  std::string name;
  /// Camera position (x, y, z) in inches, or unset.
  std::optional<Vec3> eye;
  /// Point the camera looks at, in inches.
  std::optional<Vec3> target;
  /// Camera up vector.
  std::optional<Vec3> up;
  /// Field of view in degrees (SketchUp default 35).
  double fov{35.0};
  /// True when the scene uses parallel (orthographic) projection; `fov`
  /// still holds the stored perspective angle.
  bool parallel{};
  /// Visible height in inches when `parallel`.
  double ortho_height{};
  /// Names of the layers this scene hides.
  std::vector<std::string> hidden_layers;
};

/// Reusable geometry container (component definition or group).
struct Definition {
  /// Unique TLV entity ID.
  EntityId id{};
  /// GUID string identifier.
  std::string guid;
  /// Definition name.
  std::string name;
  /// Vertices in local definition space.
  std::map<EntityId, Vertex> vertices;
  /// Edges in local definition space.
  std::map<EntityId, Edge> edges;
  /// Faces in local definition space.
  std::map<EntityId, Face> faces;
  /// Child component/group instances placed inside this definition.
  std::vector<Instance> instances;
  /// Section planes placed inside this definition.
  std::vector<SectionPlane> section_planes;
  /// Text annotations placed inside this definition.
  std::vector<TextEntity> texts;
  /// Dimensions placed inside this definition.
  std::vector<Dimension> dimensions;
  /// Construction/guide lines placed inside this definition.
  std::vector<ConstructionLine> construction_lines;
  /// Construction/guide points placed inside this definition.
  std::vector<ConstructionPoint> construction_points;
  /// Always faces camera behavior flag.
  bool always_faces_camera{};
  /// Shadows face sun behavior flag.
  bool shadows_face_sun{};
  /// Image entity flag.
  bool is_image{};
};

/// Top-level model container holding all definitions, layers, materials, and styles.
class OPENSKP_EXPORT SkpModel {
 public:
  /// SketchUp version string (e.g. "{25.0.575}").
  std::string version{"unknown"};
  /// The model's unit-system string (e.g. "Millimeter"), read from
  /// meta/meta.dat in modern (VFF) files. Unset for legacy (pre-2021 MFC)
  /// files, which carry no equivalent container, or when the tag isn't
  /// found.
  std::optional<std::string> units;
  /// Geometry definitions map keyed by numeric entity ID.
  std::map<EntityId, Definition> definitions;
  /// Model layers.
  std::vector<Layer> layers;
  /// The file's saved scenes (VFF files; classic pre-2021 files import
  /// with none).
  std::vector<Page> pages;
  /// Model-level linear dimensions with world-space endpoints (VFF
  /// files). Legacy files surface text-only dimensions per definition
  /// instead (`Definition::dimensions`).
  std::vector<Dimension> dimensions;
  /// Model materials sequence.
  std::deque<Material> materials;
  /// Model rendering styles.
  std::vector<Style> styles;

  /// Access the implicit root model definition.
  Definition& root() noexcept;
  /// Access the implicit root model definition (const).
  const Definition& root() const noexcept;

  /// Look up a material by its TLV entity ID.
  Material* material_by_id(EntityId id) noexcept;
  /// Look up a material by its TLV entity ID (const).
  const Material* material_by_id(EntityId id) const noexcept;

  /// Enumerate all materials keyed by their TLV entity ID.
  std::map<EntityId, Material*> materials_by_id();
  /// Enumerate all materials keyed by their TLV entity ID (const).
  std::map<EntityId, const Material*> materials_by_id() const;

 private:
  Definition root_{0, "ROOT", "ROOT_MODEL"};
  std::unordered_map<EntityId, std::size_t> material_indices_;
  friend SkpModel build_model(struct RawParsed&&, const ParseOptions&);
};
}  // namespace openskp
