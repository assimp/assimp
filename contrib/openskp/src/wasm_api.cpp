#ifdef __EMSCRIPTEN__

#include <chrono>
#include <cmath>
#include <cstdint>
#include <emscripten/bind.h>
#include <emscripten/val.h>
#include <string>
#include <utility>
#include <vector>

#include <openskp/fragments_export.hpp>
#include <openskp/instanced_glb.hpp>
#include <openskp/openskp.hpp>

#include "internal.hpp"

using namespace emscripten;
using namespace openskp;

namespace {

val parse_skp_to_fragments(const val& uint8_array) {
  auto start_time = std::chrono::high_resolution_clock::now();
  val result_obj = val::object();

  if (uint8_array.isNull() || uint8_array.isUndefined()) {
    result_obj.set("error", val("Invalid input: buffer is null or undefined"));
    return result_obj;
  }

  std::string stage = "init";
  try {
    // 1. Read buffer from JavaScript Uint8Array directly into native ByteBuffer
    std::size_t length = uint8_array["byteLength"].as<std::size_t>();
    if (length == 0) {
      result_obj.set("error", val("Empty buffer"));
      return result_obj;
    }

    stage = "copy_input";
    ByteBuffer buffer(length);
    val js_buf_view(typed_memory_view(length, buffer.data()));
    js_buf_view.call<void>("set", uint8_array);

    // 2. Parse instanced scene in native C++
    stage = "full_parse";
    ParseOptions parse_opts;
    RawParsed parsed = full_parse(buffer, parse_opts);
    buffer.clear();
    buffer.shrink_to_fit();

    stage = "build_instanced_scene_raw";
    InstancedScene scene = build_instanced_scene_raw(std::move(parsed), parse_opts);

    // Calculate face count
    std::size_t total_faces = 0;
    for (const auto& res : scene.mesh_resources) {
      for (const auto& prim : res.primitives) {
        total_faces += prim.indices.size() / 3;
      }
    }

    std::size_t material_count = scene.gltf_materials.size();
    std::size_t component_count = scene.mesh_resources.size();

    // 3. Convert to ThatOpen Fragments (.frag) FlatBuffers in native C++
    stage = "to_fragments";
    std::vector<std::uint8_t> frag_bytes = to_fragments(scene, /*raw=*/true);

    // Release scene data before allocating output JavaScript array
    scene = InstancedScene{};

    // 4. Copy to a new JavaScript Uint8Array
    stage = "copy_output";
    std::size_t frag_len = frag_bytes.size();
    val js_frag = val::global("Uint8Array").new_(frag_len);
    val wasm_frag_view(typed_memory_view(frag_len, frag_bytes.data()));
    js_frag.call<void>("set", wasm_frag_view);

    auto end_time = std::chrono::high_resolution_clock::now();
    double elapsed_ms = std::chrono::duration<double, std::milli>(end_time - start_time).count();

    result_obj.set("fragBytes", js_frag);
    result_obj.set("faceCount", static_cast<double>(total_faces));
    result_obj.set("materialCount", static_cast<double>(material_count));
    result_obj.set("componentCount", static_cast<double>(component_count));
    result_obj.set("parseTimeMs", elapsed_ms);
  } catch (const std::exception& e) {
    result_obj.set("error", val(std::string(e.what()) + " [stage=" + stage + "]"));
  } catch (...) {
    result_obj.set("error", val(std::string("Unknown native exception [stage=") + stage + "]"));
  }

  return result_obj;
}

val parse_skp_to_glb(const val& uint8_array) {
  auto start_time = std::chrono::high_resolution_clock::now();
  val result_obj = val::object();

  if (uint8_array.isNull() || uint8_array.isUndefined()) {
    result_obj.set("error", val("Invalid input: buffer is null or undefined"));
    return result_obj;
  }

  try {
    std::size_t length = uint8_array["byteLength"].as<std::size_t>();
    if (length == 0) {
      result_obj.set("error", val("Empty buffer"));
      return result_obj;
    }

    ByteBuffer buffer(length);
    val js_buf_view(typed_memory_view(length, buffer.data()));
    js_buf_view.call<void>("set", uint8_array);

    ParseOptions parse_opts;
    InstancedScene scene = build_instanced_scene(std::move(buffer), parse_opts);

    std::size_t total_faces = 0;
    for (const auto& res : scene.mesh_resources) {
      for (const auto& prim : res.primitives) {
        total_faces += prim.indices.size() / 3;
      }
    }

    std::size_t material_count = scene.gltf_materials.size();
    std::size_t component_count = scene.mesh_resources.size();

    InstancedGlbOptions glb_opts;
    glb_opts.textures = false;
    ByteBuffer glb_bytes = to_instanced_glb(scene, glb_opts);

    std::size_t glb_len = glb_bytes.size();
    val js_glb = val::global("Uint8Array").new_(glb_len);
    val wasm_glb_view(typed_memory_view(glb_len, glb_bytes.data()));
    js_glb.call<void>("set", wasm_glb_view);

    auto end_time = std::chrono::high_resolution_clock::now();
    double elapsed_ms = std::chrono::duration<double, std::milli>(end_time - start_time).count();

    result_obj.set("glbBytes", js_glb);
    result_obj.set("faceCount", static_cast<double>(total_faces));
    result_obj.set("materialCount", static_cast<double>(material_count));
    result_obj.set("componentCount", static_cast<double>(component_count));
    result_obj.set("parseTimeMs", elapsed_ms);
  } catch (const std::exception& e) {
    result_obj.set("error", val(e.what()));
  } catch (...) {
    result_obj.set("error", val("Unknown native exception in OpenSKP WASM"));
  }

  return result_obj;
}

}  // namespace

EMSCRIPTEN_BINDINGS(openskp_wasm_module) {
  function("parseSkpToFragments", &parse_skp_to_fragments);
  function("parseSkpToGLB", &parse_skp_to_glb);
}

#endif
