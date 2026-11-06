#include <fstream>

#include "internal.hpp"

namespace openskp {

Definition& SkpModel::root() noexcept { return root_; }

const Definition& SkpModel::root() const noexcept { return root_; }

void Texture::save(const std::filesystem::path& p) const {
  if (!data) throw std::logic_error("Texture '" + filename + "' has no image data");
  std::ofstream f(p, std::ios::binary);
  if (!f) throw std::runtime_error("Cannot open texture output: " + p.string());
  f.write(reinterpret_cast<const char*>(data->data()), static_cast<std::streamsize>(data->size()));
  if (!f) throw std::runtime_error("Failed to write texture: " + p.string());
}

Material* SkpModel::material_by_id(EntityId id) noexcept {
  auto i = material_indices_.find(id);
  return i == material_indices_.end() ? nullptr : &materials[i->second];
}

const Material* SkpModel::material_by_id(EntityId id) const noexcept {
  auto i = material_indices_.find(id);
  return i == material_indices_.end() ? nullptr : &materials[i->second];
}

std::map<EntityId, Material*> SkpModel::materials_by_id() {
  std::map<EntityId, Material*> result;
  for (auto& entry : material_indices_) {
    result[entry.first] = &materials[entry.second];
  }
  return result;
}

std::map<EntityId, const Material*> SkpModel::materials_by_id() const {
  std::map<EntityId, const Material*> result;
  for (const auto& entry : material_indices_) {
    result[entry.first] = &materials[entry.second];
  }
  return result;
}

static Definition definition(EntityId id, RawDefinition&& r) {
  Definition d;
  d.id = id;
  d.guid = std::move(r.guid);
  d.name = std::move(r.name);
  d.always_faces_camera = r.always_faces_camera;
  d.shadows_face_sun = r.shadows_face_sun;
  d.is_image = r.is_image;
  for (auto& v : r.builder.vertices)
    d.vertices.emplace(v.first, Vertex{v.first, v.second[0], v.second[1], v.second[2]});
  for (auto& e : r.builder.edges) {
    auto flags = r.builder.edge_flags[e.first];
    d.edges.emplace(e.first, Edge{e.first, e.second.first.value_or(0), e.second.second.value_or(0),
                                  bool(flags & 8), bool(flags & 16), bool(flags & 1)});
  }
  for (auto& f : r.builder.faces) {
    Face x;
    x.id = f.first;
    x.loops = std::move(f.second.loops);
    x.normal = f.second.normal;
    x.material_id = f.second.material_id;
    x.back_material_id = f.second.back_material_id;
    x.uv_transform = f.second.uv_transform;
    x.uv_transform_back = f.second.uv_transform_back;
    x.uv_projected = f.second.uv_projected;
    x.uv_projected_back = f.second.uv_projected_back;
    x.hidden = f.second.hidden;
    d.faces.emplace(f.first, std::move(x));
  }
  for (auto& i : r.builder.instances)
    d.instances.push_back({std::move(i.name), i.ref_idx, std::move(i.ref_guid), std::move(i.matrix),
                           std::move(i.layer), std::move(i.properties),
                           std::move(i.attribute_dicts), i.material_id, i.hidden});
  d.section_planes = std::move(r.builder.section_planes);
  d.texts = std::move(r.builder.texts);
  d.dimensions = std::move(r.builder.dimensions);
  d.construction_lines = std::move(r.builder.construction_lines);
  d.construction_points = std::move(r.builder.construction_points);
  return d;
}

SkpModel build_model(RawParsed&& p, const ParseOptions& o) {
  SkpModel m;
  m.version = std::move(p.version);
  m.units = std::move(p.units);
  auto resolve_layers = [&](RawDefinition& d) {
    for (auto& i : d.builder.instances)
      if (!i.layer.empty()) try {
          auto l = p.layer_id_to_name.find(std::stoll(i.layer));
          if (l != p.layer_id_to_name.end()) i.layer = l->second;
        } catch (...) {
          emit_log(o, LogLevel::debug, "Failed to resolve layer id '" + i.layer + "' to a name");
        };
  };
  for (auto& d : p.definitions) {
    resolve_layers(d.second);
    m.definitions.emplace(d.first, definition(d.first, std::move(d.second)));
  }
  resolve_layers(p.root);
  m.root_ = definition(0, std::move(p.root));
  for (auto& name : p.layer_order) {
    auto color_it = p.layer_colors.find(name);
    if (color_it == p.layer_colors.end()) continue;
    auto hidden_it = p.layer_hidden.find(name);
    bool hidden = hidden_it != p.layer_hidden.end() && hidden_it->second;
    m.layers.push_back({name, color_it->second, hidden});
  }
  // Convert pages (saved scenes) - hidden layer ids resolve to names;
  // unknown ids (stale refs) are dropped.
  for (auto& pg : p.pages) {
    Page page;
    page.name = pg.name;
    page.eye = pg.eye;
    page.target = pg.target;
    page.up = pg.up;
    page.fov = pg.fov;
    page.parallel = pg.parallel;
    page.ortho_height = pg.ortho_height;
    for (auto id : pg.hidden_layer_ids) {
      auto it = p.layer_id_to_name.find(id);
      if (it != p.layer_id_to_name.end()) page.hidden_layers.push_back(it->second);
    }
    m.pages.push_back(std::move(page));
  }
  // Convert model-level linear dimensions (VFF; world space).
  for (auto& dm : p.dimensions) {
    Dimension dim;
    dim.a = dm.a;
    dim.b = dm.b;
    dim.offset = dm.offset;
    dim.plane_x = dm.plane_x;
    dim.normal = dm.normal;
    dim.text = dm.text;
    m.dimensions.push_back(std::move(dim));
  }
  std::map<const RawMaterial*, std::size_t> raw_index;
  for (auto& v : p.materials) {
    auto& r = *v.second;
    Material x;
    x.name = r.name;
    x.color = {std::uint8_t(r.r), std::uint8_t(r.g), std::uint8_t(r.b), std::uint8_t(r.a)};
    x.transparency = r.transparency;
    x.colorized = r.colorized;
    x.colorize_type = r.colorize_type;
    if (r.texture)
      x.texture = Texture{r.texture->filename, r.texture->x_scale, r.texture->y_scale,
                          std::move(r.texture->data)};
    raw_index[&r] = m.materials.size();
    m.materials.push_back(std::move(x));
  }
  for (auto& v : p.material_id_to_name) {
    std::shared_ptr<RawMaterial> r;
    auto a = p.materials.find(v.second);
    if (a != p.materials.end())
      r = a->second;
    else {
      auto b = p.materials_by_folder.find(v.second);
      if (b != p.materials_by_folder.end()) r = b->second;
    }
    if (r && raw_index.count(r.get())) {
      auto i = raw_index[r.get()];
      if (!m.materials[i].id) m.materials[i].id = v.first;
      m.material_indices_[v.first] = i;
    }
  }
  for (auto& s : p.styles) m.styles.push_back({std::move(s.name), s.front_color, s.back_color});
  return m;
}
}  // namespace openskp
