// SPDX-License-Identifier: MIT
#pragma once

#include "simple-render.hh"

namespace example {

struct GUIContext {
  enum AOVMode {
    AOV_COLOR = 0,
    AOV_SHADING_NORMAL,
    AOV_GEOMETRIC_NORMAL,
    AOV_POSITION,
    AOV_DEPTH,
    AOV_TEXCOORD,
    AOV_VARYCOORD,
    AOV_VERTEXCOLOR
  };
  int aov_mode{AOV_COLOR};


  example::AOV aov;  // framebuffer

  int width = 1024;
  int height = 768;

  int mouse_x = -1;
  int mouse_y = -1;

  bool mouse_left_down = false;
  bool shift_pressed = false;
  bool ctrl_pressed = false;
  bool tab_pressed = false;

  float yaw = 90.0f;  // for Z up scene
  float pitch = 0.0f;
  float roll = 0.0f;

  // float curr_quat[4] = {0.0f, 0.0f, 0.0f, 1.0f};
  // std::array<float, 4> prev_quat[4] = {0.0f, 0.0f, 0.0f, 1.0f};

  // std::array<float, 3> eye = {0.0f, 0.0f, 5.0f};
  // std::array<float, 3> lookat = {0.0f, 0.0f, 0.0f};
  // std::array<float, 3> up = {0.0f, 1.0f, 0.0f};

  example::RenderScene render_scene;

  example::Camera camera;

  std::atomic<bool> update_texture{false};
  std::atomic<bool> redraw{true};  // require redraw
  std::atomic<bool> quit{false};

  int render_width = 512;
  int render_height = 512;

  // scene reload
  tinyusdz::Stage stage;
  std::atomic<bool> request_reload{false};
  std::string filename;

#if __EMSCRIPTEN__ || defined(EMULATE_EMSCRIPTEN)
  bool render_finished{false};
  int current_render_line = 0;
  int render_line_size = 32; // render images with this lines per animation loop.
  // for emscripten environment
#endif
};

extern GUIContext g_gui_ctx;


// Setup scene. Only need to call once USD scene(ctx.scene) is loaded.
bool SetupScene(GUIContext &ctx);

// Render the scene(and store rendered image to ctx.aov)
bool RenderScene(GUIContext &ctx);

// Get rendered image from ctx.aov as Android ARGB_8888 IntArray.
void GetRenderedImage(const GUIContext &ctx, std::vector<uint32_t> *argb);

} // example
