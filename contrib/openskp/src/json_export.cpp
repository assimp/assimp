#include <cassert>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <stdexcept>

#include <openskp/json_export.hpp>

namespace openskp {

void JsonValue::set(std::string key, JsonValue val) {
  assert(is_object());
  std::get<Object>(value_).emplace_back(std::move(key), std::move(val));
}

void JsonValue::push(JsonValue val) {
  assert(is_array());
  std::get<Array>(value_).push_back(std::move(val));
}

const JsonValue* JsonValue::find(const std::string& key) const {
  if (!is_object()) return nullptr;
  for (const auto& [k, v] : std::get<Object>(value_)) {
    if (k == key) return &v;
  }
  return nullptr;
}

namespace {

void write_escaped_string(std::ostringstream& out, const std::string& s) {
  out << '"';
  for (char c : s) {
    switch (c) {
      case '"':
        out << "\\\"";
        break;
      case '\\':
        out << "\\\\";
        break;
      case '\n':
        out << "\\n";
        break;
      case '\r':
        out << "\\r";
        break;
      case '\t':
        out << "\\t";
        break;
      default:
        if (static_cast<unsigned char>(c) < 0x20) {
          char buf[8];
          std::snprintf(buf, sizeof(buf), "\\u%04x", static_cast<unsigned char>(c));
          out << buf;
        } else {
          out << c;
        }
    }
  }
  out << '"';
}

void write_newline_indent(std::ostringstream& out, int indent, int depth) {
  if (indent <= 0) return;
  out << '\n';
  out << std::string(static_cast<std::size_t>(indent) * static_cast<std::size_t>(depth), ' ');
}

void write_value(std::ostringstream& out, const JsonValue& value, int indent, int depth) {
  switch (value.kind()) {
    case JsonValue::Kind::Null:
      out << "null";
      return;
    case JsonValue::Kind::Bool:
      out << (value.as_bool() ? "true" : "false");
      return;
    case JsonValue::Kind::Int:
      out << value.as_int();
      return;
    case JsonValue::Kind::Double: {
      // %.17g round-trips a double exactly, matching the other ports'
      // choice of always-safe float serialization precision.
      char buf[32];
      std::snprintf(buf, sizeof(buf), "%.17g", value.as_double());
      out << buf;
      return;
    }
    case JsonValue::Kind::String:
      write_escaped_string(out, value.as_string());
      return;
    case JsonValue::Kind::Array: {
      const auto& arr = value.as_array();
      out << '[';
      bool first = true;
      for (const auto& elem : arr) {
        if (!first) out << ',';
        first = false;
        write_newline_indent(out, indent, depth + 1);
        write_value(out, elem, indent, depth + 1);
      }
      if (!arr.empty()) write_newline_indent(out, indent, depth);
      out << ']';
      return;
    }
    case JsonValue::Kind::Object: {
      const auto& obj = value.as_object();
      out << '{';
      bool first = true;
      for (const auto& [key, val] : obj) {
        if (!first) out << ',';
        first = false;
        write_newline_indent(out, indent, depth + 1);
        write_escaped_string(out, key);
        out << ':';
        if (indent > 0) out << ' ';
        write_value(out, val, indent, depth + 1);
      }
      if (!obj.empty()) write_newline_indent(out, indent, depth);
      out << '}';
      return;
    }
  }
  throw std::logic_error("unreachable JsonValue::Kind");
}

JsonValue vertex_to_json(const Vertex& v) {
  auto obj = JsonValue::make_object();
  obj.set("id", static_cast<std::int64_t>(v.id));
  obj.set("x", v.x);
  obj.set("y", v.y);
  obj.set("z", v.z);
  return obj;
}

JsonValue edge_to_json(const Edge& e) {
  auto obj = JsonValue::make_object();
  obj.set("id", static_cast<std::int64_t>(e.id));
  obj.set("v1_id", static_cast<std::int64_t>(e.v1_id));
  obj.set("v2_id", static_cast<std::int64_t>(e.v2_id));
  return obj;
}

JsonValue face_to_json(const Face& f) {
  auto obj = JsonValue::make_object();
  obj.set("id", static_cast<std::int64_t>(f.id));

  auto loops = JsonValue::make_array();
  for (const auto& loop : f.loops) {
    auto loop_json = JsonValue::make_array();
    for (const auto& ce : loop) {
      auto ce_json = JsonValue::make_object();
      ce_json.set("edge_id", static_cast<std::int64_t>(ce.edge_id));
      ce_json.set("orientation", static_cast<std::int64_t>(ce.orientation));
      loop_json.push(std::move(ce_json));
    }
    loops.push(std::move(loop_json));
  }
  obj.set("loops", std::move(loops));

  if (f.normal.has_value()) {
    auto n = JsonValue::make_array();
    n.push((*f.normal)[0]);
    n.push((*f.normal)[1]);
    n.push((*f.normal)[2]);
    obj.set("normal", std::move(n));
  } else {
    obj.set("normal", nullptr);
  }
  return obj;
}

JsonValue instance_to_json(const Instance& inst) {
  auto obj = JsonValue::make_object();
  obj.set("name", inst.name);
  if (inst.ref_idx.has_value()) {
    obj.set("ref_idx", static_cast<std::int64_t>(*inst.ref_idx));
  } else {
    obj.set("ref_idx", nullptr);
  }
  obj.set("guid", inst.guid);
  auto matrix = JsonValue::make_array();
  for (double m : inst.matrix) matrix.push(m);
  obj.set("matrix", std::move(matrix));
  return obj;
}

JsonValue definition_to_json(const Definition& defn) {
  auto obj = JsonValue::make_object();
  obj.set("id", static_cast<std::int64_t>(defn.id));
  obj.set("guid", defn.guid);
  obj.set("name", defn.name);
  obj.set("vertex_count", static_cast<std::int64_t>(defn.vertices.size()));
  obj.set("edge_count", static_cast<std::int64_t>(defn.edges.size()));
  obj.set("face_count", static_cast<std::int64_t>(defn.faces.size()));

  auto vertices = JsonValue::make_array();
  for (const auto& [id, v] : defn.vertices) vertices.push(vertex_to_json(v));
  obj.set("vertices", std::move(vertices));

  auto edges = JsonValue::make_array();
  for (const auto& [id, e] : defn.edges) edges.push(edge_to_json(e));
  obj.set("edges", std::move(edges));

  auto faces = JsonValue::make_array();
  for (const auto& [id, f] : defn.faces) faces.push(face_to_json(f));
  obj.set("faces", std::move(faces));

  auto instances = JsonValue::make_array();
  for (const auto& inst : defn.instances) instances.push(instance_to_json(inst));
  obj.set("instances", std::move(instances));

  return obj;
}

// Every attribute dictionary an instance/mesh carries, keyed by the
// dictionary's own declared name - not just SketchUp's own
// dynamic_attributes (which `properties` above already covers). This is
// where a third-party plugin's own per-instance data (e.g. a
// steel-detailing tool's own named dictionary) reaches JSON output; it
// was already correctly resolved by build_scene()/build_instanced_scene()
// but previously never serialized here at all.
JsonValue attribute_dictionaries_to_json(
    const std::map<std::string, std::map<std::string, std::string>>& dicts) {
  auto obj = JsonValue::make_object();
  for (const auto& [dict_name, entries] : dicts) {
    auto entries_obj = JsonValue::make_object();
    for (const auto& [k, v] : entries) entries_obj.set(k, v);
    obj.set(dict_name, std::move(entries_obj));
  }
  return obj;
}

JsonValue instance_node_to_json(const InstanceNode& node) {
  auto obj = JsonValue::make_object();
  obj.set("name", node.name);
  obj.set("definition_name", node.definition_name);
  obj.set("layer", node.layer);
  auto pos = JsonValue::make_array();
  pos.push(node.position_mm[0]);
  pos.push(node.position_mm[1]);
  pos.push(node.position_mm[2]);
  obj.set("position_mm", std::move(pos));
  auto props = JsonValue::make_object();
  for (const auto& [k, v] : node.properties) props.set(k, v);
  obj.set("properties", std::move(props));
  obj.set("attribute_dictionaries", attribute_dictionaries_to_json(node.attribute_dictionaries));
  auto children = JsonValue::make_array();
  for (const auto& child : node.children) children.push(instance_node_to_json(child));
  obj.set("children", std::move(children));
  return obj;
}

JsonValue mesh_metadata_to_json(const MeshMetadata& m) {
  auto obj = JsonValue::make_object();
  obj.set("name", m.name);
  obj.set("definition_name", m.definition_name);
  obj.set("layer", m.layer);
  auto pos = JsonValue::make_array();
  pos.push(m.position_mm[0]);
  pos.push(m.position_mm[1]);
  pos.push(m.position_mm[2]);
  obj.set("position_mm", std::move(pos));
  auto props = JsonValue::make_object();
  for (const auto& [k, v] : m.properties) props.set(k, v);
  obj.set("properties", std::move(props));
  obj.set("attribute_dictionaries", attribute_dictionaries_to_json(m.attribute_dictionaries));
  obj.set("path", m.path);
  return obj;
}

}  // namespace

std::string to_json_string(const JsonValue& value, int indent) {
  std::ostringstream out;
  write_value(out, value, indent, 0);
  return out.str();
}

JsonValue to_json(const SkpModel& model, const Scene* scene) {
  auto definitions = JsonValue::make_object();
  for (const auto& [id, defn] : model.definitions) {
    definitions.set(std::to_string(id), definition_to_json(defn));
  }

  auto layers = JsonValue::make_array();
  for (const auto& l : model.layers) {
    auto layer_obj = JsonValue::make_object();
    layer_obj.set("name", l.name);
    auto color = JsonValue::make_object();
    color.set("r", static_cast<std::int64_t>(l.color[0]));
    color.set("g", static_cast<std::int64_t>(l.color[1]));
    color.set("b", static_cast<std::int64_t>(l.color[2]));
    layer_obj.set("color", std::move(color));
    layer_obj.set("hidden", l.hidden);
    layers.push(std::move(layer_obj));
  }

  auto materials = JsonValue::make_array();
  for (const auto& m : model.materials) {
    auto mat_obj = JsonValue::make_object();
    mat_obj.set("name", m.name);
    auto color = JsonValue::make_object();
    color.set("r", static_cast<std::int64_t>(m.color[0]));
    color.set("g", static_cast<std::int64_t>(m.color[1]));
    color.set("b", static_cast<std::int64_t>(m.color[2]));
    color.set("a", static_cast<std::int64_t>(m.color[3]));
    mat_obj.set("color", std::move(color));
    mat_obj.set("transparency", m.transparency);
    materials.push(std::move(mat_obj));
  }

  auto mesh_index = JsonValue::make_object();
  if (scene != nullptr) {
    for (const auto& [name, meta] : scene->mesh_index) {
      mesh_index.set(name, mesh_metadata_to_json(meta));
    }
  }

  auto result = JsonValue::make_object();
  result.set("format_version", "1.0");
  result.set("sketchup_version", model.version);
  if (model.units.has_value()) {
    result.set("units", *model.units);
  } else {
    result.set("units", nullptr);
  }
  result.set("total_definitions", static_cast<std::int64_t>(model.definitions.size()));
  result.set("total_layers", static_cast<std::int64_t>(model.layers.size()));
  result.set("total_meshes",
             static_cast<std::int64_t>(scene != nullptr ? scene->mesh_index.size() : 0));
  result.set("root", definition_to_json(model.root()));
  result.set("definitions", std::move(definitions));
  result.set("layers", std::move(layers));
  result.set("materials", std::move(materials));
  result.set("mesh_index", std::move(mesh_index));
  if (scene != nullptr) {
    result.set("scene_hierarchy", instance_node_to_json(scene->scene_hierarchy));
  } else {
    result.set("scene_hierarchy", nullptr);
  }

  return result;
}

void export_json(const SkpModel& model, const std::filesystem::path& output_path,
                 const Scene* scene, int indent) {
  auto json = to_json(model, scene);
  auto text = to_json_string(json, indent);
  std::ofstream out(output_path, std::ios::binary);
  if (!out) {
    throw std::runtime_error("failed to open output path for JSON export: " + output_path.string());
  }
  out << text;
}

}  // namespace openskp
