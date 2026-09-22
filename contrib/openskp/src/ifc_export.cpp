#include "openskp/ifc_export.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <fstream>
#include <functional>
#include <iomanip>
#include <map>
#include <optional>
#include <random>
#include <sstream>

namespace openskp {

namespace {

const std::string IFC_BASE64 = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz_$";

std::string sanitize_name(const std::string& name) {
  if (name.empty()) return "Unnamed";
  std::string clean;
  clean.reserve(name.size());
  for (char c : name) {
    if (c == '\'') {
      clean += "''";
    } else if (c == '\\') {
      clean += "\\\\";
    } else {
      clean += c;
    }
  }
  // trim
  size_t start = clean.find_first_not_of(" \t\r\n");
  if (start == std::string::npos) return "Unnamed";
  size_t end = clean.find_last_not_of(" \t\r\n");
  return clean.substr(start, end - start + 1);
}

std::tuple<double, double, double, double> get_prim_rgb(const Scene& scene, size_t prim_mat_idx) {
  double r = 0.8, g = 0.8, b = 0.8, a = 1.0;
  if (prim_mat_idx < scene.gltf_materials.size()) {
    const auto& mat = scene.gltf_materials[prim_mat_idx];
    const auto& col = mat.pbr_metallic_roughness.base_color_factor;
    r = std::max(0.0, std::min(1.0, col[0]));
    g = std::max(0.0, std::min(1.0, col[1]));
    b = std::max(0.0, std::min(1.0, col[2]));
    a = std::max(0.0, std::min(1.0, col[3]));
  }
  return {r, g, b, a};
}

}  // namespace

std::string generate_ifc_guid() {
  static std::mt19937 rng(std::random_device{}());
  std::uniform_int_distribution<int> dist(0, 63);
  std::string result;
  result.reserve(22);
  for (int i = 0; i < 22; ++i) {
    result += IFC_BASE64[dist(rng)];
  }
  return result;
}

namespace {

std::optional<std::pair<std::string, std::string>> classify_by_keyword(const std::string& name) {
  std::string l = name;
  std::transform(l.begin(), l.end(), l.begin(), [](unsigned char c) { return std::tolower(c); });
  if (l.find("wall") != std::string::npos) return std::pair{"IFCWALL", "IfcWall"};
  if (l.find("door") != std::string::npos) return std::pair{"IFCDOOR", "IfcDoor"};
  if (l.find("window") != std::string::npos) return std::pair{"IFCWINDOW", "IfcWindow"};
  if (l.find("slab") != std::string::npos || l.find("floor") != std::string::npos)
    return std::pair{"IFCSLAB", "IfcSlab"};
  if (l.find("column") != std::string::npos || l.find("pillar") != std::string::npos)
    return std::pair{"IFCCOLUMN", "IfcColumn"};
  if (l.find("beam") != std::string::npos || l.find("joist") != std::string::npos)
    return std::pair{"IFCBEAM", "IfcBeam"};
  if (l.find("roof") != std::string::npos) return std::pair{"IFCROOF", "IfcRoof"};
  return std::nullopt;
}

}  // namespace

std::pair<std::string, std::string> classify_element(const std::string& geom_name,
                                                     const std::string& layer_name,
                                                     const std::string& path_name,
                                                     bool classify_using_full_path) {
  if (auto by_name = classify_by_keyword(geom_name)) return *by_name;
  if (!layer_name.empty()) {
    if (auto by_layer = classify_by_keyword(layer_name)) return *by_layer;
  }
  if (classify_using_full_path && !path_name.empty()) {
    if (auto by_path = classify_by_keyword(path_name)) return *by_path;
  }
  return {"IFCBUILDINGELEMENTPROXY", "IfcBuildingElementProxy"};
}

namespace {

// One IFCPROPERTYSET (props) attached to product_id via
// IFCRELDEFINESBYPROPERTIES. Shared by both meta.properties
// (Pset_CustomProperties) and every other attribute dictionary an
// instance carries (Pset_<dict-name>) below.
void write_pset(std::ostringstream& ss, const std::function<int()>& next_id, int owner_hist_id,
                int product_id, const std::string& pset_name,
                const std::map<std::string, std::string>& props) {
  if (props.empty()) return;
  std::vector<int> prop_val_ids;
  for (const auto& [pk, pv] : props) {
    std::string clean_k = sanitize_name(pk);
    std::string clean_v = sanitize_name(pv);
    int prop_id = next_id();
    ss << "#" << prop_id << "=IFCPROPERTYSINGLEVALUE('" << clean_k << "',$,IFCTEXT('" << clean_v
       << "'),$);\r\n";
    prop_val_ids.push_back(prop_id);
  }

  int pset_id = next_id();
  std::ostringstream prop_ss;
  for (size_t i = 0; i < prop_val_ids.size(); ++i) {
    if (i > 0) prop_ss << ",";
    prop_ss << "#" << prop_val_ids[i];
  }
  ss << "#" << pset_id << "=IFCPROPERTYSET('" << generate_ifc_guid() << "',#" << owner_hist_id
     << ",'" << sanitize_name(pset_name) << "',$,(" << prop_ss.str() << "));\r\n";
  ss << "#" << next_id() << "=IFCRELDEFINESBYPROPERTIES('" << generate_ifc_guid() << "',#"
     << owner_hist_id << ",$,$,(#" << product_id << "),#" << pset_id << ");\r\n";
}

}  // namespace

std::string to_ifc(const Scene& scene, double scale, const std::string& schema,
                   const IfcClassifier& classifier, bool classify_using_full_path) {
  // classifier (if given) keeps its original 2-arg (name, layer) public
  // signature; only the built-in classify_element() gains the path/
  // full-path-matching parameters, matching openskp.export.ifc's own
  // to_ifc()/classify_element() split.
  std::function<std::pair<std::string, std::string>(const std::string&, const std::string&,
                                                    const std::string&)>
      classify;
  if (classifier) {
    classify = [&](const std::string& name, const std::string& layer, const std::string&) {
      return classifier(name, layer);
    };
  } else {
    classify = [&](const std::string& name, const std::string& layer, const std::string& path) {
      return classify_element(name, layer, path, classify_using_full_path);
    };
  }
  std::string schema_str = schema.empty() ? "IFC4" : schema;
  std::transform(schema_str.begin(), schema_str.end(), schema_str.begin(),
                 [](unsigned char c) { return std::toupper(c); });

  auto now = std::chrono::system_clock::now();
  auto timestamp_epoch =
      std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
  std::time_t now_time = std::chrono::system_clock::to_time_t(now);
  std::tm gmt{};
#if defined(_WIN32)
  gmtime_s(&gmt, &now_time);
#else
  gmtime_r(&now_time, &gmt);
#endif
  char time_buf[32];
  std::strftime(time_buf, sizeof(time_buf), "%Y-%m-%dT%H:%M:%S", &gmt);

  std::ostringstream ss;
  ss.imbue(std::locale::classic());
  ss << std::fixed << std::setprecision(6);

  ss << "ISO-10303-21;\r\n";
  ss << "HEADER;\r\n";
  ss << "FILE_DESCRIPTION(('ViewDefinition [CoordinationView]'),'2;1');\r\n";
  ss << "FILE_NAME('model.ifc','" << time_buf
     << "',('OpenSKP Author'),('OpenSKP "
        "Organization'),'OpenSKP IFC Exporter','OpenSKP','');\r\n";
  ss << "FILE_SCHEMA(('" << schema_str << "'));\r\n";
  ss << "ENDSEC;\r\n";
  ss << "DATA;\r\n";

  int entity_id = 1;
  auto next_id = [&entity_id]() { return entity_id++; };

  int person_id = next_id();
  ss << "#" << person_id << "=IFCPERSON($,$,'OpenSKP User',$,$,$,$,$);\r\n";

  int org_id = next_id();
  ss << "#" << org_id << "=IFCORGANIZATION($,'OpenSKP',$,$,$);\r\n";

  int person_org_id = next_id();
  ss << "#" << person_org_id << "=IFCPERSONANDORGANIZATION(#" << person_id << ",#" << org_id
     << ",$);\r\n";

  int app_id = next_id();
  ss << "#" << app_id << "=IFCAPPLICATION(#" << org_id
     << ",'0.3.1','OpenSKP Exporter','OpenSKP');\r\n";

  int owner_hist_id = next_id();
  ss << "#" << owner_hist_id << "=IFCOWNERHISTORY(#" << person_org_id << ",#" << app_id
     << ",$,.READWRITE.,$,$,$," << timestamp_epoch << ");\r\n";

  int length_unit_id = next_id();
  ss << "#" << length_unit_id << "=IFCSIUNIT(*,.LENGTHUNIT.,.MILLI.,.METRE.);\r\n";

  int angle_unit_id = next_id();
  ss << "#" << angle_unit_id << "=IFCSIUNIT(*,.PLANEANGLEUNIT.,$,.RADIAN.);\r\n";

  int solid_unit_id = next_id();
  ss << "#" << solid_unit_id << "=IFCSIUNIT(*,.STERADIANUNIT.,$,.STERADIAN.);\r\n";

  int unit_assign_id = next_id();
  ss << "#" << unit_assign_id << "=IFCUNITASSIGNMENT((#" << length_unit_id << ",#" << angle_unit_id
     << ",#" << solid_unit_id << "));\r\n";

  int pt_zero_id = next_id();
  ss << "#" << pt_zero_id << "=IFCCARTESIANPOINT((0.0,0.0,0.0));\r\n";

  int axis_placement_id = next_id();
  ss << "#" << axis_placement_id << "=IFCAXIS2PLACEMENT3D(#" << pt_zero_id << ",$,$);\r\n";

  int geom_ctx_id = next_id();
  ss << "#" << geom_ctx_id << "=IFCGEOMETRICREPRESENTATIONCONTEXT($,'Model',3,1.0E-5,# "
     << axis_placement_id << ",$);\r\n";

  int proj_id = next_id();
  ss << "#" << proj_id << "=IFCPROJECT('" << generate_ifc_guid() << "',#" << owner_hist_id
     << ",'OpenSKP Project',$,$,$,$,(#" << geom_ctx_id << "),#" << unit_assign_id << ");\r\n";

  int site_placement_id = next_id();
  ss << "#" << site_placement_id << "=IFCLOCALPLACEMENT($,#" << axis_placement_id << ");\r\n";

  int site_id = next_id();
  ss << "#" << site_id << "=IFCSITE('" << generate_ifc_guid() << "',#" << owner_hist_id
     << ",'Site',$,$,#" << site_placement_id << ",$,$,.ELEMENT.,$,$,$,$,$);\r\n";

  int bldg_placement_id = next_id();
  ss << "#" << bldg_placement_id << "=IFCLOCALPLACEMENT(#" << site_placement_id << ",#"
     << axis_placement_id << ");\r\n";

  int bldg_id = next_id();
  ss << "#" << bldg_id << "=IFCBUILDING('" << generate_ifc_guid() << "',#" << owner_hist_id
     << ",'Building',$,$,#" << bldg_placement_id << ",$,$,.ELEMENT.,$,$,$);\r\n";

  int storey_placement_id = next_id();
  ss << "#" << storey_placement_id << "=IFCLOCALPLACEMENT(#" << bldg_placement_id << ",#"
     << axis_placement_id << ");\r\n";

  int storey_id = next_id();
  ss << "#" << storey_id << "=IFCBUILDINGSTOREY('" << generate_ifc_guid() << "',#" << owner_hist_id
     << ",'Level 0',$,$,#" << storey_placement_id << ",$,$,.ELEMENT.,0.0);\r\n";

  ss << "#" << next_id() << "=IFCRELAGGREGATES('" << generate_ifc_guid() << "',#" << owner_hist_id
     << ",$,$,#" << proj_id << ",(#" << site_id << "));\r\n";
  ss << "#" << next_id() << "=IFCRELAGGREGATES('" << generate_ifc_guid() << "',#" << owner_hist_id
     << ",$,$,#" << site_id << ",(#" << bldg_id << "));\r\n";
  ss << "#" << next_id() << "=IFCRELAGGREGATES('" << generate_ifc_guid() << "',#" << owner_hist_id
     << ",$,$,#" << bldg_id << ",(#" << storey_id << "));\r\n";

  std::vector<int> product_ids;
  std::map<std::string, std::vector<int>> layer_items;
  std::map<std::string, int> mat_style_cache;

  for (const auto& prim : scene.glb_primitives) {
    size_t tri_count = prim.indices.size() / 3;
    size_t v_count = prim.positions.size() / 3;
    if (tri_count == 0 || v_count == 0) continue;

    auto meta_it = scene.mesh_index.find(prim.geom_name);
    // prim.geom_name is an internal lookup key (mesh index + hierarchy
    // path + layer, e.g. "mesh_3115_ROOT__Component_6205261_Layer0") -
    // never the element's real name. meta.name is the actual SketchUp
    // instance name (or SketchUp's own "Component_<id>" default when
    // nobody renamed it) - use that for both classification and the
    // element's IFC Name, falling back to the internal key only if a
    // primitive somehow has no mesh_index entry.
    std::string display_name = (meta_it != scene.mesh_index.end() && !meta_it->second.name.empty())
                                   ? sanitize_name(meta_it->second.name)
                                   : sanitize_name(prim.geom_name);
    std::string layer_name = "Layer0";
    if (meta_it != scene.mesh_index.end() && !meta_it->second.layer.empty()) {
      layer_name = sanitize_name(meta_it->second.layer);
    }
    std::string path_name = meta_it != scene.mesh_index.end() ? meta_it->second.path : "";

    auto [step_type, _] = classify(display_name, layer_name, path_name);

    // scene.glb_primitives positions are baked in glTF's Y-up convention
    // (glTF.y = SketchUp Z/height, glTF.z = -SketchUp Y/depth) - correct
    // for GLB export, but IFC (like SketchUp itself) is Z-up, so it has
    // to be converted back rather than passed through raw, or the
    // exported building comes out rotated ~90 degrees and mirrored.
    std::ostringstream pt_ss;
    pt_ss.imbue(std::locale::classic());
    pt_ss << std::fixed << std::setprecision(6);
    for (size_t i = 0; i < v_count; ++i) {
      if (i > 0) pt_ss << ",";
      double vx = prim.positions[i * 3] * scale;
      double vy = -prim.positions[i * 3 + 2] * scale;
      double vz = prim.positions[i * 3 + 1] * scale;
      pt_ss << "(" << vx << "," << vy << "," << vz << ")";
    }

    int pt_list_id = next_id();
    ss << "#" << pt_list_id << "=IFCCARTESIANPOINTLIST3D((" << pt_ss.str() << "));\r\n";

    std::ostringstream face_ss;
    for (size_t i = 0; i < tri_count; ++i) {
      if (i > 0) face_ss << ",";
      face_ss << "(" << (prim.indices[i * 3] + 1) << "," << (prim.indices[i * 3 + 1] + 1) << ","
              << (prim.indices[i * 3 + 2] + 1) << ")";
    }

    int face_set_id = next_id();
    ss << "#" << face_set_id << "=IFCTRIANGULATEDFACESET(#" << pt_list_id << ",$,.TRUE.,("
       << face_ss.str() << "),$);\r\n";

    layer_items[layer_name].push_back(face_set_id);

    auto [r, g, b, a] = get_prim_rgb(scene, prim.material_index);
    std::ostringstream key_ss;
    key_ss.imbue(std::locale::classic());
    key_ss << std::fixed << std::setprecision(4) << r << "," << g << "," << b << "," << a;
    std::string rgba_key = key_ss.str();

    int style_assign_id;
    auto style_it = mat_style_cache.find(rgba_key);
    if (style_it == mat_style_cache.end()) {
      int col_id = next_id();
      ss << std::fixed << std::setprecision(4);
      ss << "#" << col_id << "=IFCCOLOURRGB($, " << r << "," << g << "," << b << ");\r\n";

      double transparency = 1.0 - a;
      int rendering_id = next_id();
      ss << "#" << rendering_id << "=IFCSURFACESTYLERENDERING(#" << col_id << "," << transparency
         << ",$,$,$,$,$,$,.FLAT.);\r\n";

      int style_id = next_id();
      ss << "#" << style_id << "=IFCSURFACESTYLE('" << display_name << "_Material',.BOTH.,(#"
         << rendering_id << "));\r\n";

      style_assign_id = next_id();
      ss << "#" << style_assign_id << "=IFCPRESENTATIONSTYLEASSIGNMENT((#" << style_id << "));\r\n";
      mat_style_cache[rgba_key] = style_assign_id;
    } else {
      style_assign_id = style_it->second;
    }

    int styled_item_id = next_id();
    ss << "#" << styled_item_id << "=IFCSTYLEDITEM(#" << face_set_id << ",(#" << style_assign_id
       << "),$);\r\n";

    int shape_rep_id = next_id();
    ss << "#" << shape_rep_id << "=IFCSHAPEREPRESENTATION(#" << geom_ctx_id
       << ",'Body','Tessellation',(#" << face_set_id << "));\r\n";

    int prod_shape_id = next_id();
    ss << "#" << prod_shape_id << "=IFCPRODUCTDEFINITIONSHAPE($,$,(#" << shape_rep_id << "));\r\n";

    int prod_placement_id = next_id();
    ss << "#" << prod_placement_id << "=IFCLOCALPLACEMENT(#" << storey_placement_id << ",#"
       << axis_placement_id << ");\r\n";

    int product_id = next_id();
    std::string prod_guid = generate_ifc_guid();
    if (step_type == "IFCBUILDINGELEMENTPROXY") {
      ss << "#" << product_id << "=" << step_type << "('" << prod_guid << "',#" << owner_hist_id
         << ",'" << display_name << "',$,$,#" << prod_placement_id << ",#" << prod_shape_id
         << ",$,.NOTDEFINED.);\r\n";
    } else {
      ss << "#" << product_id << "=" << step_type << "('" << prod_guid << "',#" << owner_hist_id
         << ",'" << display_name << "',$,$,#" << prod_placement_id << ",#" << prod_shape_id
         << ",$,$);\r\n";
    }
    product_ids.push_back(product_id);

    if (meta_it != scene.mesh_index.end()) {
      write_pset(ss, next_id, owner_hist_id, product_id, "Pset_CustomProperties",
                 meta_it->second.properties);

      // Any OTHER attribute dictionaries the instance carries (third-party
      // BIM/steel-detailing plugins, etc. - see MeshMetadata::
      // attribute_dictionaries) - each becomes its own named property set
      // instead of being merged into Pset_CustomProperties, since these
      // come from a distinct source and commonly share key names with
      // each other (e.g. multiple plugins using "name").
      for (const auto& [dict_name, entries] : meta_it->second.attribute_dictionaries) {
        write_pset(ss, next_id, owner_hist_id, product_id, "Pset_" + dict_name, entries);
      }
    }
  }

  // Presentation Layer Assignments (preserve layers and their on/off
  // state). IFCPRESENTATIONLAYERWITHSTYLE - not the plain
  // IFCPRESENTATIONLAYERASSIGNMENT neither SketchUp's own IFC exporter
  // nor the IFC-manager SketchUp extension use - is the only IFC4 entity
  // that can carry a layer's visibility at all (LayerOn); every other
  // attribute here besides Name and LayerOn is left unset/false since
  // this project doesn't track them (freeze/block/layer-level styles).
  for (const auto& [l_name, item_ids] : layer_items) {
    if (!item_ids.empty()) {
      std::ostringstream item_ss;
      for (size_t i = 0; i < item_ids.size(); ++i) {
        if (i > 0) item_ss << ",";
        item_ss << "#" << item_ids[i];
      }
      auto hidden_it = scene.layer_hidden.find(l_name);
      const char* layer_on =
          (hidden_it != scene.layer_hidden.end() && hidden_it->second) ? ".F." : ".T.";
      ss << "#" << next_id() << "=IFCPRESENTATIONLAYERWITHSTYLE('" << l_name << "',$,("
         << item_ss.str() << "),$," << layer_on << ",.F.,.F.,());\r\n";
    }
  }

  if (!product_ids.empty()) {
    std::ostringstream prod_ss;
    for (size_t i = 0; i < product_ids.size(); ++i) {
      if (i > 0) prod_ss << ",";
      prod_ss << "#" << product_ids[i];
    }
    ss << "#" << next_id() << "=IFCRELCONTAINEDINSPATIALSTRUCTURE('" << generate_ifc_guid() << "',#"
       << owner_hist_id << ",$,$,(" << prod_ss.str() << "),#" << storey_id << ");\r\n";
  }

  ss << "ENDSEC;\r\n";
  ss << "END-ISO-10303-21;\r\n";
  return ss.str();
}

void export_ifc(const Scene& scene, const std::filesystem::path& path, double scale,
                const std::string& schema, const IfcClassifier& classifier,
                bool classify_using_full_path) {
  if (path.has_parent_path()) {
    std::filesystem::create_directories(path.parent_path());
  }
  std::ofstream file(path, std::ios::out | std::ios::binary);
  if (!file.is_open()) {
    throw std::runtime_error("Failed to open file for writing: " + path.string());
  }
  std::string text = to_ifc(scene, scale, schema, classifier, classify_using_full_path);
  file.write(text.data(), text.size());
}

}  // namespace openskp
