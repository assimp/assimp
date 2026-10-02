#pragma once

#include <chrono>
#include <cstring>
#include <limits>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

#include <openskp/openskp.hpp>

namespace openskp {
struct TlvNode {
  std::uint64_t offset{};
  std::uint64_t size{};
  std::string tag;
  std::vector<TlvNode> children;
  ByteBuffer payload;
};

struct RawFace {
  std::vector<std::vector<CoEdge>> loops;
  Vec3 normal{0, 0, 1};
  std::optional<EntityId> material_id;
  std::optional<EntityId> back_material_id;
  std::optional<std::array<double, 9>> uv_transform;
  std::optional<std::array<double, 9>> uv_transform_back;
  bool uv_projected{};
  bool uv_projected_back{};
  bool hidden{};
};

struct RawInstance {
  std::uint64_t offset{};
  std::string ref_guid;
  std::string name;
  std::optional<EntityId> ref_idx;
  std::optional<EntityId> material_id;
  std::vector<double> matrix;
  std::vector<TlvNode> children;
  std::string layer;
  // The "dynamic_attributes" dictionary only (SketchUp's own Dynamic
  // Components data) - the same backward-compatible view Python's
  // extract_dynamic_properties() exposes as `properties`.
  std::map<std::string, std::string> properties;
  // Every OTHER attribute dictionary this instance carries, keyed by the
  // dictionary's own declared name (VFF tag B436) - a third-party plugin
  // (steel-detailing tool, etc.) commonly attaches its own richer
  // per-instance data under its own dictionary name instead of
  // dynamic_attributes. SU_InstanceSet (SketchUp's own always-present,
  // always-empty boilerplate) is excluded, same as dynamic_attributes.
  std::map<std::string, std::map<std::string, std::string>> attribute_dicts;
  bool hidden{};
};

struct GeometryBuilder {
  std::map<EntityId, Vec3> vertices;
  std::map<EntityId, std::pair<std::optional<EntityId>, std::optional<EntityId>>> edges;
  std::map<EntityId, int> edge_flags;
  std::map<EntityId, RawFace> faces;
  std::vector<RawInstance> instances;
  std::vector<SectionPlane> section_planes;
  std::vector<TextEntity> texts;
  std::vector<Dimension> dimensions;
  std::vector<ConstructionLine> construction_lines;
  std::vector<ConstructionPoint> construction_points;
};

struct RawDefinition {
  std::string guid;
  std::string name;
  bool always_faces_camera{};
  bool shadows_face_sun{};
  bool is_image{};
  GeometryBuilder builder;
};

struct RawTexture {
  std::string filename;
  double x_scale{};
  double y_scale{};
  std::optional<ByteBuffer> data;
};

struct RawMaterial {
  std::string name;
  int r{128};
  int g{128};
  int b{128};
  int a{255};
  double transparency{1};
  bool colorized{};
  int colorize_type{};
  std::optional<RawTexture> texture;
};

struct RawStyle {
  std::string name;
  std::optional<Color3> front_color;
  std::optional<Color3> back_color;
};

struct RawPage {
  std::string name;
  std::optional<Vec3> eye;
  std::optional<Vec3> target;
  std::optional<Vec3> up;
  double fov{35.0};
  bool parallel{};
  double ortho_height{};
  std::vector<EntityId> hidden_layer_ids;
};

struct RawDimension {
  Vec3 a{};
  Vec3 b{};
  double offset{};
  std::optional<Vec3> plane_x;
  std::optional<Vec3> normal;
  std::string text;
};

struct RawParsed {
  std::string version{"unknown"};
  // The model's unit-system string (e.g. "Millimeter"), read from
  // meta/meta.dat. Unset for legacy files or when the tag isn't found.
  std::optional<std::string> units;
  std::map<std::string, Color3> layer_colors;
  // Layer names in the order they were first encountered in the source
  // file (material.xml archive-entry order for VFF, slot-scan order for
  // legacy) - layer_colors above is a std::map (sorted by name), so this
  // is the only place file order survives; model.cpp's layer-building
  // loop iterates this instead of layer_colors directly. Mirrors
  // Python's plain dict for the same field, which preserves insertion
  // order natively.
  std::vector<std::string> layer_order;
  // Modern (VFF) files derive layer COLOR from Layer_<name>-prefixed
  // materials, which carry no visibility flag of their own - real
  // visibility comes from the model.dat layer manager's own 8E3C byte
  // (see geometry.cpp's collect_layers), read here into this same map.
  std::map<std::string, bool> layer_hidden;
  std::map<EntityId, std::string> layer_id_to_name;
  std::vector<RawPage> pages;
  std::vector<RawDimension> dimensions;
  std::map<EntityId, std::string> material_id_to_name;
  std::map<std::string, std::shared_ptr<RawMaterial>> materials;
  std::map<std::string, std::shared_ptr<RawMaterial>> materials_by_folder;
  std::vector<RawStyle> styles;
  std::map<EntityId, RawDefinition> definitions;
  RawDefinition root{"ROOT", "ROOT_MODEL"};
};

std::uint16_t read_u16(const ByteBuffer&, std::size_t);
std::uint32_t read_u32(const ByteBuffer&, std::size_t);
std::int32_t read_i32(const ByteBuffer&, std::size_t);
double read_f64(const ByteBuffer&, std::size_t);
std::uint64_t parse_varint(const ByteBuffer&, std::size_t, std::size_t);
std::string tag_at(const ByteBuffer&, std::size_t);
std::vector<TlvNode> parse_tlv_recursive(const ByteBuffer&, std::size_t, std::size_t);
std::vector<std::pair<std::string, ByteBuffer>> parse_flat(const ByteBuffer&);
std::optional<std::string> read_meta_units(const ByteBuffer&);
std::string extract_version(const ByteBuffer&);
bool valid_header(const ByteBuffer&);
bool is_legacy(const ByteBuffer&);
bool legacy_instance_has_guid(const std::string&, std::optional<int>);

/// Result of `find_count_after_v20_filler`: the recovered count and the
/// offset just past it.
struct V20FillerHit {
  std::uint32_t count;
  std::size_t next;
};

std::optional<V20FillerHit> find_count_after_v20_filler(const ByteBuffer&, std::size_t,
                                                        std::uint32_t);
RawParsed full_parse(const ByteBuffer&, const ParseOptions&);
std::string decode_xml_entities(const std::string&);
RawParsed parse_legacy(const ByteBuffer&, const ParseOptions&);

// legacy.cpp's own MFC object-graph value type - one decoded entity
// (material/layer/component/edge/face/... - `k` says which). Moved here
// (out of legacy.cpp's own anonymous namespace, alongside LegacySlotEntry
// below) rather than forward-declared only: a slot-table entry needs to
// hold a real, complete shared_ptr<V>, and legacy.cpp's own code (which
// sees this same type via ordinary unqualified lookup, exactly as before)
// must keep resolving to this one definition, not a second, incomplete
// one - two same-named types in nested scopes are different types in
// C++, so this can't be split into "forward-declared here, defined
// there" without every V-typed field silently binding to the wrong one.
struct V {
  std::string k;
  std::string name;
  std::string label;
  std::string text;
  std::string guid;
  Vec3 xyz{};
  // Only populated for k == "constructionline": the line's normalized
  // direction, and its bounded segment's start/end (unset when unbounded
  // in that direction) - see ConstructionLine's own doc comment.
  Vec3 direction{};
  std::optional<Vec3> start;
  std::optional<Vec3> end;
  std::vector<double> plane;
  std::vector<double> xf;
  std::vector<double> uvf;
  std::vector<double> uvb;
  bool front_projected{};
  bool back_projected{};
  std::uint64_t v1{};
  std::uint64_t v2{};
  std::uint64_t edge{};
  std::uint64_t def{};
  // Slot of this entity's CAttributeContainer (resolved through
  // Archive::slots), or nullopt when it has none. Not a bare slot number:
  // slot 0 is a legitimate real slot (the first object allocated in the
  // archive), so 0 can't double as a sentinel for "absent."
  std::optional<std::uint64_t> attrs;
  // Only populated for "dict" (CAttributeNamed) entities: this
  // dictionary's own key/value pairs, already stringified (see
  // Archive::typed()).
  std::map<std::string, std::string> entries;
  std::uint64_t tex_dib{};
  bool sense{};
  bool faces_camera{};
  bool shadows_face_sun{};
  bool colorized{};
  int mat{};
  int back_mat{};
  int layer{};
  int hidden{};
  int soft{};
  int smooth{};
  int r{128};
  int g{128};
  int b{128};
  int a{255};
  double opacity{};
  double tw{};
  double th{};
  std::string tex_file;
  ByteBuffer blob;
  std::vector<std::shared_ptr<V>> loops;
  std::vector<std::shared_ptr<V>> uses;
  std::vector<std::tuple<uint64_t, std::string, std::shared_ptr<V>>> ents;
};

/// One legacy MFC archive slot-table entry: either a class declaration
/// (cls=true, name/schema identify the class, v unused) or a read
/// object (cls=false, v holds its decoded value, possibly null for
/// object kinds this port doesn't decode into V). Exposed here (moved
/// out of legacy.cpp's own anonymous namespace) purely so
/// scan_pages_for_layers below is unit-testable without needing
/// legacy.cpp's full ~750-line Archive type - matches
/// find_count_after_v20_filler's precedent just above.
struct LegacySlotEntry {
  bool cls{};
  std::string name;
  int schema{};
  std::shared_ptr<V> v;
};

/// A minimal view into an Archive's slot-table bookkeeping - the only
/// pieces legacy.cpp's page (scene) scanner needs, exposed narrowly
/// (rather than the full Archive type) so that feature is unit-testable.
/// A real caller (legacy.cpp's parse_legacy) wraps its own Archive's
/// slots/class_slot/next members; a test can hand-build just the
/// entries a specific case needs.
struct LegacySlotTable {
  std::unordered_map<std::uint64_t, LegacySlotEntry>& slots;
  std::unordered_map<std::string, std::uint64_t>& class_slot;
  std::uint64_t& next;

  std::uint64_t alloc(LegacySlotEntry e) {
    auto s = next++;
    slots[s] = std::move(e);
    return s;
  }
};

/// Best-effort scan for CViewPage (scene) records in a legacy (pre-2021
/// MFC) file: name + hidden-layer slot ids only, for the narrow
/// CViewPage capture-flag combinations this reader understands. See
/// legacy.cpp's own scan_pages for the full rationale and byte layout -
/// mirrors Python's legacy._scan_pages exactly. `start_pos` is the byte
/// offset to begin scanning from (the main entity walk's own stopping
/// point in a real parse).
std::vector<RawPage> scan_pages_for_layers(const ByteBuffer& data, std::size_t start_pos,
                                           LegacySlotTable slots);
void collect_geometry(const std::vector<TlvNode>&, GeometryBuilder&);
void collect_layers(const std::vector<TlvNode>&, std::map<EntityId, std::string>&,
                    std::map<std::string, bool>&);
void collect_material_ids(const std::vector<TlvNode>&, std::map<EntityId, std::string>&);
void collect_definitions(const std::vector<TlvNode>&, std::map<EntityId, RawDefinition>&);
SkpModel build_model(RawParsed&&, const ParseOptions& = {});
Scene build_scene_raw(RawParsed&&, const ParseOptions&);
InstancedScene build_instanced_scene_raw(RawParsed&&, const ParseOptions&);
std::array<double, 3> transform_point(const std::vector<double>&, const std::array<double, 3>&);
std::array<double, 3> transform_normal(const std::vector<double>&, const std::array<double, 3>&);
double transform_determinant(const std::vector<double>&);
std::vector<double> multiply_matrices(const std::vector<double>&, const std::vector<double>&);
void scan_vertex_positions(const TlvNode&, std::map<std::string, Vec3>&);
void scan_instance_transforms(const TlvNode&, std::map<std::string, std::vector<double>>&);
std::vector<RawDimension> parse_dimensions(const ByteBuffer&, const std::map<std::string, Vec3>&,
                                           const std::map<std::string, std::vector<double>>&);
const TlvNode* find_page_node(const TlvNode&);
std::vector<RawPage> parse_pages(const TlvNode*);

struct EarPoint {
  double x{};
  double y{};
  EntityId id{};
};

std::vector<std::array<EntityId, 3>> earcut_2d(std::vector<std::vector<EarPoint>> loops);
void emit_log(const ParseOptions&, LogLevel, const std::string&);
void emit_progress(const ParseOptions&, ParseStage, std::uint64_t, std::uint64_t);
}  // namespace openskp
