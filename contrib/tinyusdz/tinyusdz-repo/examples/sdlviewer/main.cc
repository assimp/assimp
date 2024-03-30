
#include <algorithm>
#include <atomic>  // C++11
#include <cassert>
#include <chrono>  // C++11
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <mutex>   // C++11
#include <thread>  // C++11

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>
#endif

// ../common/SDL2
#include <SDL.h>

#if defined(SDL_VIDEO_DRIVER_X11)
#include <SDL_syswm.h>
#endif

#if defined(USDVIEW_USE_BULLET3)
#include <btBulletDynamicsCommon.h>
#endif

// common
#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_sdlrenderer.h"
#include "imnodes.h"
#include "roboto_mono_embed.inc.h"
#include "virtualGizmo3D/vGizmo.h"

//
#include "gui.hh"
#include "simple-render.hh"
#include "tinyusdz.hh"
#include "trackball.h"

#if defined(USDVIEW_USE_NATIVEFILEDIALOG)
#include "nfd.h"
#endif

//#define EMULATE_EMSCRIPTEN

#if defined(EMULATE_EMSCRIPTEN)
#define EM_BOOL int
#define EM_TRUE 1
#define EM_FALSE 0
#endif

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

  SDL_Renderer* renderer;
  SDL_Texture* texture;  // Texture for rendered image
  int render_width = 512;
  int render_height = 512;

  // scene reload
  tinyusdz::Stage stage;
  std::atomic<bool> request_reload{false};
  std::string filename;

#if __EMSCRIPTEN__ || defined(EMULATE_EMSCRIPTEN)
  bool render_finished{false};
  int current_render_line = 0;
  int render_line_size =
      32;  // render images with this lines per animation loop.
  // for emscripten environment
#endif
};

GUIContext g_gui_ctx;

namespace {

inline double radians(double degree) { return 3.141592653589 * degree / 180.0; }

// https://en.wikipedia.org/wiki/Conversion_between_quaternions_and_Euler_angles
std::array<double, 4> ToQuaternion(double yaw, double pitch,
                                   double roll)  // yaw (Z), pitch (Y), roll (X)
{
  // Abbreviations for the various angular functions
  double cy = std::cos(yaw * 0.5);
  double sy = std::sin(yaw * 0.5);
  double cp = std::cos(pitch * 0.5);
  double sp = std::sin(pitch * 0.5);
  double cr = std::cos(roll * 0.5);
  double sr = std::sin(roll * 0.5);

  std::array<double, 4> q;
  q[0] = cr * cp * cy + sr * sp * sy;
  q[1] = sr * cp * cy - cr * sp * sy;
  q[2] = cr * sp * cy + sr * cp * sy;
  q[3] = cr * cp * sy - sr * sp * cy;

  return q;
}

#if 0
static void DrawGeomMesh(tinyusdz::GeomMesh& mesh) {}

static void DrawNode(const tinyusdz::Scene& scene, const tinyusdz::Node& node) {
  if (node.type == tinyusdz::NODE_TYPE_XFORM) {
    const tinyusdz::Xform& xform = scene.xforms.at(node.index);
  }

  for (const auto& child : node.children) {
    // DrawNode(scene, scene.nodes.at(child));
  }

  if (node.type == tinyusdz::NODE_TYPE_XFORM) {
    // glPopMatrix();
  }
}
#endif

static void Proc(const tinyusdz::Stage& stage) {
  (void)stage;

  //std::cout << "num geom_meshes = " << scene.geom_meshes.size() << "\n";
  //for (auto& mesh : scene.geom_meshes) {
  //}
}

static std::string GetFileExtension(const std::string& filename) {
  if (filename.find_last_of(".") != std::string::npos)
    return filename.substr(filename.find_last_of(".") + 1);
  return "";
}

static std::string str_tolower(std::string s) {
  std::transform(s.begin(), s.end(), s.begin(),
                 // static_cast<int(*)(int)>(std::tolower)         // wrong
                 // [](int c){ return std::tolower(c); }           // wrong
                 // [](char c){ return std::tolower(c); }          // wrong
                 [](unsigned char c) { return std::tolower(c); }  // correct
  );
  return s;
}

// TODO: Use pow table for faster conversion.
inline float linearToSrgb(float x) {
  if (x <= 0.0f)
    return 0.0f;
  else if (x >= 1.0f)
    return 1.0f;
  else if (x < 0.0031308f)
    return x * 12.92f;
  else
    return std::pow(x, 1.0f / 2.4f) * 1.055f - 0.055f;
}

inline uint8_t ftouc(float f) {
  int val = int(f * 255.0f);
  val = (std::max)(0, (std::min)(255, val));

  return static_cast<uint8_t>(val);
}

void UpdateTexutre(SDL_Texture* tex, const GUIContext& ctx,
                   const example::AOV& aov) {
  int w, h;
  SDL_QueryTexture(tex, nullptr, nullptr, &w, &h);

  if ((aov.width != w) || (aov.height != h)) {
    std::cerr << "texture size and AOV sized mismatch\n";
    return;
  }

  std::vector<uint8_t> buf;
  buf.resize(aov.width * aov.height * 4);

  if (ctx.aov_mode == GUIContext::AOV_COLOR) {
    for (size_t i = 0; i < aov.width * aov.height; i++) {
      uint8_t r = ftouc(linearToSrgb(aov.rgb[3 * i + 0]));
      uint8_t g = ftouc(linearToSrgb(aov.rgb[3 * i + 1]));
      uint8_t b = ftouc(linearToSrgb(aov.rgb[3 * i + 2]));

      buf[4 * i + 0] = r;
      buf[4 * i + 1] = g;
      buf[4 * i + 2] = b;
      buf[4 * i + 3] = 255;
    }
  } else if (ctx.aov_mode == GUIContext::AOV_SHADING_NORMAL) {
    for (size_t i = 0; i < aov.width * aov.height; i++) {
      uint8_t r = ftouc(aov.shading_normal[3 * i + 0]);
      uint8_t g = ftouc(aov.shading_normal[3 * i + 1]);
      uint8_t b = ftouc(aov.shading_normal[3 * i + 2]);

      buf[4 * i + 0] = r;
      buf[4 * i + 1] = g;
      buf[4 * i + 2] = b;
      buf[4 * i + 3] = 255;
    }
  } else if (ctx.aov_mode == GUIContext::AOV_GEOMETRIC_NORMAL) {
    for (size_t i = 0; i < aov.width * aov.height; i++) {
      uint8_t r = ftouc(aov.geometric_normal[3 * i + 0]);
      uint8_t g = ftouc(aov.geometric_normal[3 * i + 1]);
      uint8_t b = ftouc(aov.geometric_normal[3 * i + 2]);

      buf[4 * i + 0] = r;
      buf[4 * i + 1] = g;
      buf[4 * i + 2] = b;
      buf[4 * i + 3] = 255;
    }
  } else if (ctx.aov_mode == GUIContext::AOV_TEXCOORD) {
    for (size_t i = 0; i < aov.width * aov.height; i++) {
      uint8_t r = ftouc(aov.texcoords[2 * i + 0]);
      uint8_t g = ftouc(aov.texcoords[2 * i + 1]);
      uint8_t b = 255;

      buf[4 * i + 0] = r;
      buf[4 * i + 1] = g;
      buf[4 * i + 2] = b;
      buf[4 * i + 3] = 255;
    }
  }

  SDL_UpdateTexture(tex, nullptr, reinterpret_cast<const void*>(buf.data()),
                    aov.width * 4);
}

// https://discourse.libsdl.org/t/sdl-and-xserver/12610/4
static void ScreenActivate(SDL_Window* window) {
#if defined(SDL_VIDEO_DRIVER_X11)
  SDL_SysWMinfo wm;

  // Get window info.
  SDL_VERSION(&wm.version);
  SDL_GetWindowWMInfo(window, &wm);

  // Lock to display access.
  // wm.info.x11.lock_func();

  // Show the window on top.
  XMapRaised(wm.info.x11.display, wm.info.x11.window);

  // Set the focus on it.
  XSetInputFocus(wm.info.x11.display, wm.info.x11.window, RevertToParent,
                 CurrentTime);
#else
  (void)window;
#endif
}

bool LoadModel(const std::string& filename, tinyusdz::Stage* stage) {
  std::string ext = str_tolower(GetFileExtension(filename));

  std::string warn;
  std::string err;

  if (ext.compare("usdz") == 0) {
    std::cout << "usdz\n";
    bool ret = tinyusdz::LoadUSDZFromFile(filename, stage, &warn, &err);
    if (!warn.empty()) {
      std::cerr << "WARN : " << warn << "\n";
    }
    if (!err.empty()) {
      std::cerr << "ERR : " << err << "\n";
    }

    if (!ret) {
      std::cerr << "Failed to load USDZ file: " << filename << "\n";
      return false;
    }
  } else if (ext.compare("usda") == 0) {
    std::cout << "usda\n";
    bool ret = tinyusdz::LoadUSDAFromFile(filename, stage, &warn, &err);
    if (!warn.empty()) {
      std::cerr << "WARN : " << warn << "\n";
    }
    if (!err.empty()) {
      std::cerr << "ERR : " << err << "\n";
    }

    if (!ret) {
      std::cerr << "Failed to load USDA file: " << filename << "\n";
      return false;
    }
  } else {  // assume usdc
    bool ret = tinyusdz::LoadUSDCFromFile(filename, stage, &warn, &err);
    if (!warn.empty()) {
      std::cerr << "WARN : " << warn << "\n";
    }
    if (!err.empty()) {
      std::cerr << "ERR : " << err << "\n";
    }

    if (!ret) {
      std::cerr << "Failed to load USDC file: " << filename << "\n";
      return false;
    }
  }

  return true;
}

void RenderThread(GUIContext* ctx) {
  bool done = false;

  while (!done) {
    if (ctx->quit) {
      return;
    }

    if (ctx->request_reload) {
      ctx->stage = tinyusdz::Stage();  // reset

      if (LoadModel(ctx->filename, &ctx->stage)) {
        Proc(ctx->stage);
#if 0
        if (ctx->scene.geom_meshes.empty()) {
          std::cerr << "The scene contains no GeomMesh\n";
        } else {
          ctx->render_scene.draw_meshes.clear();

          for (size_t i = 0; i < ctx->scene.geom_meshes.size(); i++) {
            example::DrawGeomMesh draw_mesh(&ctx->scene.geom_meshes[i]);
            ctx->render_scene.draw_meshes.push_back(draw_mesh);
          }

          // Setup render mesh
          if (!ctx->render_scene.Setup()) {
            std::cerr << "Failed to setup render mesh.\n";
            ctx->render_scene.draw_meshes.clear();
          }
          std::cout << "Setup render mesh\n";
        }
#endif
      }

      ctx->request_reload = false;

      ctx->redraw = true;
    }

    if (!ctx->redraw) {
      // Give CPU some cycles.
      std::this_thread::sleep_for(std::chrono::milliseconds(33));
      continue;
    }

    example::Render(ctx->render_scene, ctx->camera, &ctx->aov);

    ctx->update_texture = true;

    ctx->redraw = false;
  }
};

#if defined(USDVIEW_USE_NATIVEFILEDIALOG)
// TODO: widechar(UTF-16) support for Windows
std::string OpenFileDialog() {
  std::string path;

  nfdchar_t* outPath;
  nfdfilteritem_t filterItem[1] = {{"USD file", "usda,usdc,usdz"}};

  nfdresult_t result = NFD_OpenDialog(&outPath, filterItem, 1, NULL);
  if (result == NFD_OKAY) {
    puts("Success!");
    path = outPath;
    NFD_FreePath(outPath);
  } else if (result == NFD_CANCEL) {
    puts("User pressed cancel.");
  } else {
    printf("Error: %s\n", NFD_GetError());
  }

  return path;
}
#endif

// Helper to display a little (?) mark which shows a tooltip when hovered.
// In your own code you may want to display an actual icon if you are using a
// merged icon fonts (see docs/FONTS.md)
static void HelpMarker(const char* desc) {
  ImGui::TextDisabled("(?)");
  if (ImGui::IsItemHovered()) {
    ImGui::BeginTooltip();
    ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
    ImGui::TextUnformatted(desc);
    ImGui::PopTextWrapPos();
    ImGui::EndTooltip();
  }
}

#if defined(__EMSCRIPTEN__) || defined(EMULATE_EMSCRIPTEN)

EM_BOOL em_main_loop_frame(double tm, void* user) {
#if 1
  // Render image fragment
  if (g_gui_ctx.redraw) {
    g_gui_ctx.render_finished = false;
    g_gui_ctx.current_render_line = 0;

    g_gui_ctx.redraw = false;
  }

  if (!g_gui_ctx.render_finished) {
    std::cout << "RenderLines: " << g_gui_ctx.current_render_line << "\n";
    RenderLines(g_gui_ctx.current_render_line,
                g_gui_ctx.current_render_line + g_gui_ctx.render_line_size,
                g_gui_ctx.render_scene, g_gui_ctx.camera, &g_gui_ctx.aov);

    g_gui_ctx.current_render_line += g_gui_ctx.render_line_size;
    if (g_gui_ctx.current_render_line >= g_gui_ctx.render_height) {
      g_gui_ctx.current_render_line = 0;
      g_gui_ctx.render_finished = true;

      g_gui_ctx.update_texture = true;
    }
  }
#endif

  ImGuiIO& io = ImGui::GetIO();

#if 1
  int wheel = 0;

  SDL_Event e;
  if (SDL_PollEvent(&e)) {
    if (e.type == SDL_QUIT) {
      return EM_FALSE;
    } else if (e.type == SDL_DROPFILE) {
      char* filepath = e.drop.file;

      printf("File dropped: %s\n", filepath);

      SDL_free(filepath);

    } else if (e.type == SDL_WINDOWEVENT) {
      if (e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
        io.DisplaySize.x = static_cast<float>(e.window.data1);
        io.DisplaySize.y = static_cast<float>(e.window.data2);
      }
    } else if (e.type == SDL_KEYDOWN) {
      if (e.key.keysym.sym == SDLK_ESCAPE) {
        return EM_FALSE;
      }
    } else if (e.type == SDL_KEYUP) {
    } else if (e.type == SDL_MOUSEWHEEL) {
      wheel = e.wheel.y;
    }
  }

  int mouseX, mouseY;
  const int buttons = SDL_GetMouseState(&mouseX, &mouseY);

  // Setup low-level inputs (e.g. on Win32, GetKeyboardState(), or
  // write to those fields from your Windows message loop handlers,
  // etc.)

  io.DeltaTime = 1.0f / 60.0f;
  io.MousePos = ImVec2(static_cast<float>(mouseX), static_cast<float>(mouseY));
  io.MouseDown[0] = buttons & SDL_BUTTON(SDL_BUTTON_LEFT);
  io.MouseDown[1] = buttons & SDL_BUTTON(SDL_BUTTON_RIGHT);
  io.MouseWheel = static_cast<float>(wheel);
#endif

#if 1
  ImGui::NewFrame();

  bool update = false;
  bool update_display = false;
  ImGui::Begin("Scene");

  update |=
      ImGui::SliderFloat("eye.z", &g_gui_ctx.camera.eye[2], -1000.0, 1000.0f);
  update |= ImGui::SliderFloat("fov", &g_gui_ctx.camera.fov, 0.01f, 140.0f);

  // TODO: Validate coordinate definition.
  if (ImGui::SliderFloat("yaw", &g_gui_ctx.yaw, -360.0f, 360.0f)) {
    auto q = ToQuaternion(radians(g_gui_ctx.yaw), radians(g_gui_ctx.pitch),
                          radians(g_gui_ctx.roll));
    g_gui_ctx.camera.quat[0] = q[0];
    g_gui_ctx.camera.quat[1] = q[1];
    g_gui_ctx.camera.quat[2] = q[2];
    g_gui_ctx.camera.quat[3] = q[3];
    update = true;
  }
  if (ImGui::SliderFloat("pitch", &g_gui_ctx.pitch, -360.0f, 360.0f)) {
    auto q = ToQuaternion(radians(g_gui_ctx.yaw), radians(g_gui_ctx.pitch),
                          radians(g_gui_ctx.roll));
    g_gui_ctx.camera.quat[0] = q[0];
    g_gui_ctx.camera.quat[1] = q[1];
    g_gui_ctx.camera.quat[2] = q[2];
    g_gui_ctx.camera.quat[3] = q[3];
    update = true;
  }
  if (ImGui::SliderFloat("roll", &g_gui_ctx.roll, -360.0f, 360.0f)) {
    auto q = ToQuaternion(radians(g_gui_ctx.yaw), radians(g_gui_ctx.pitch),
                          radians(g_gui_ctx.roll));
    g_gui_ctx.camera.quat[0] = q[0];
    g_gui_ctx.camera.quat[1] = q[1];
    g_gui_ctx.camera.quat[2] = q[2];
    g_gui_ctx.camera.quat[3] = q[3];
    update = true;
  }

  ImGui::End();

  ImGui::Begin("Image");
  ImGui::Image(g_gui_ctx.texture,
               ImVec2(g_gui_ctx.render_width, g_gui_ctx.render_height));
  ImGui::End();

  if (update) {
    g_gui_ctx.redraw = true;
  }

  // Update texture
  if (g_gui_ctx.update_texture || update_display) {
    // Update texture for display
    UpdateTexutre(g_gui_ctx.texture, g_gui_ctx, g_gui_ctx.aov);

    g_gui_ctx.update_texture = false;
  }

  SDL_SetRenderDrawColor(g_gui_ctx.renderer, 114, 144, 154, 255);
  SDL_RenderClear(g_gui_ctx.renderer);

  // Imgui

  ImGui::Render();
  ImGuiSDL::Render(ImGui::GetDrawData());

  // static int texUpdateCount = 0;

  SDL_RenderPresent(g_gui_ctx.renderer);

#endif

  return EM_TRUE;
}

#endif

#if 0
void NodeTreeSubWindow(const tinyusdz::Node& node, uint32_t indent) {
  if (node.name.empty()) {
    // Do not traverse children of the node without name
    return;
  }

  if (ImGui::TreeNode(node.name.c_str())) {
    for (const auto& c : node.children) {
      NodeTreeSubWindow(c, indent + 1);
    }

    ImGui::TreePop();
  }
}

void NodeTreeWindow(const tinyusdz::HighLevelScene& scene) {
  ImGui::Begin("Node");

  if (scene.nodes.size()) {
    size_t root_node_id =
        (scene.default_root_node >= 0) ? size_t(scene.default_root_node) : 0;
    NodeTreeSubWindow(scene.nodes[root_node_id], /* indent */ 0);
  }

  ImGui::End();
}
#endif

#if 0
void ShaderParamWindow(const tinyusdz::HighLevelScene& scene) {
  ImGui::Begin("Shaders");

  for (const auto& item : scene.shaders) {
    if (item.name.empty()) {
      continue;
    }

    if (ImGui::TreeNode(item.name.c_str())) {
      if (item.value.is<tinyusdz::PreviewSurface>()) {
        ImGui::Text("type:id UsdPreviewSurface");

      } else if (item.value.is<tinyusdz::UVTexture>()) {
        ImGui::Text("type:id UVTexture");
      } else if (item.value.is<tinyusdz::PrimvarReader_float2>()) {
        ImGui::Text("type:id PrimvarReader_float2");
      } else if (item.value.is<tinyusdz::PrimvarReader_float3>()) {
        ImGui::Text("type:id PrimvarReader_float3");
      } else {
        ImGui::Text("Unsupported Shader type");
      }
      ImGui::TreePop();
    }
  }
  ImGui::End();
}

void MaterialsParamWindow(const tinyusdz::HighLevelScene& scene) {
  ImGui::Begin("Materials");

  for (const auto& item : scene.materials) {
    if (!item.name.empty()) {
      if (ImGui::TreeNode(item.name.c_str())) {
        int nrow = 1;

        ImGui::BeginTable("props", /* columns*/ 2);

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("output.surface");
        ImGui::TableSetColumnIndex(1);

        ImGui::Text("%s", item.outputs_surface.c_str());

        ImGui::EndTable();
        ImGui::TreePop();
      }
    }
  }

  ImGui::End();
}

void UVTextureNode(int node_id, const tinyusdz::UVTexture& texture) {
  constexpr int kMaxSlots = 100;
  constexpr int kOutputOffset = kMaxSlots / 2;

  ImNodes::BeginNode(kMaxSlots * node_id);

  ImNodes::BeginNodeTitleBar();
  ImGui::TextUnformatted("UsdUVTexture");
  ImNodes::EndNodeTitleBar();

  ImNodes::BeginInputAttribute(kMaxSlots * node_id + 1);
  ImGui::Text("inputs:file");
  ImNodes::EndInputAttribute();

  ImNodes::BeginInputAttribute(kMaxSlots * node_id + 2);
  ImGui::Text("inputs:sourceColorSpace");
  ImNodes::EndInputAttribute();

  ImNodes::BeginInputAttribute(kMaxSlots * node_id + 3);
  ImGui::Text("inputs:st");
  ImNodes::EndInputAttribute();

  ImNodes::BeginOutputAttribute(kMaxSlots * node_id + kOutputOffset + 1);
  ImGui::Indent(40);
  ImGui::Text("outputs:rgb");
  ImNodes::EndOutputAttribute();

  ImNodes::EndNode();
}

void ShaderGraphWindow(const tinyusdz::HighLevelScene& scene) {
  ImGui::Begin("Shader graph");

  ImNodes::BeginNodeEditor();

  ImNodes::BeginNode(1);

  ImNodes::BeginNodeTitleBar();
  ImGui::TextUnformatted("File");
  ImNodes::EndNodeTitleBar();

  // ImNodes::BeginInputAttribute(2);
  // ImGui::Text("file");
  // ImNodes::EndInputAttribute();

  ImNodes::BeginOutputAttribute(3);
  ImGui::Indent(40);
  ImGui::Text("checkerboard.png");
  ImNodes::EndOutputAttribute();

  ImNodes::EndNode();

  tinyusdz::UVTexture texture;
  int texture_node_id = 1;
  UVTextureNode(texture_node_id, texture);

  ImNodes::MiniMap();

  ImNodes::EndNodeEditor();

  ImGui::End();
}
#endif

}  // namespace

int main(int argc, char** argv) {
#if defined(USDVIEW_USE_BULLET3)
  btDefaultCollisionConfiguration* collisionConfiguration =
      new btDefaultCollisionConfiguration();
#endif

  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
    std::cerr << "Failed to initialize SDL2\n";
    exit(-1);
  }

  std::cout << "SDL2 init OK\n";

#ifdef _WIN32
  std::string filename = "../../models/suzanne.usdc";
#elif __EMSCRIPTEN__
  // assume filename is embeded with --embed-file in emcc compile flag.
  std::string filename = "suzanne.usdc";
#else
  std::string filename = "../../../models/suzanne.usdc";
#endif

#if defined(USDVIEW_USE_NATIVEFILEDIALOG)
  NFD_Init();

#endif

  if (argc > 1) {
    filename = std::string(argv[1]);
  }

  std::cout << "Loading file " << filename << "\n";

  bool init_with_empty = false;

  if (!LoadModel(filename, &g_gui_ctx.stage)) {
    init_with_empty = true;
  }

  if (!init_with_empty) {
    std::cout << "Loaded USDC file\n";

    Proc(g_gui_ctx.stage);
    //if (g_gui_ctx.scene.geom_meshes.empty()) {
    //  std::cerr << "The scene contains no GeomMesh\n";
    //  exit(-1);
    //}
  }

  // Assume single monitor
  SDL_DisplayMode DM;
  SDL_GetCurrentDisplayMode(0, &DM);
  std::cout << "Current monitor: " << DM.w << " x " << DM.h << "\n";

  int default_win_x = 1600;
  int default_win_y = 800;

  if (DM.w > 2560) {
    default_win_x = 2560;
  }

  if (DM.h > 1600) {
    default_win_y = 1600;
  }

#if defined(__APPLE__)
  // For some reason, HIGHDPI does not work well on Retina Display for SDLRenderer backend.
  // Disable it for a while.
  SDL_WindowFlags window_flags = static_cast<SDL_WindowFlags>(SDL_WINDOW_RESIZABLE);
#else
  SDL_WindowFlags window_flags = static_cast<SDL_WindowFlags>(SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
#endif

  std::cout << "default window size: " << default_win_x << " x " << default_win_y << "\n";
  SDL_Window* window = SDL_CreateWindow(
      "Simple USDZ viewer", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
      default_win_x, default_win_y, window_flags);
  if (!window) {
    std::cerr << "Failed to create SDL2 window. If you are running on Linux, "
                 "probably X11 Display is not setup correctly. Check your "
                 "DISPLAY environment.\n";
    exit(-1);
  }

  std::cout << "SDL2 Window creation OK\n";

  SDL_Renderer* renderer = SDL_CreateRenderer(
      window, -1, SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED);

  if (!renderer) {
    std::cerr << "Failed to create SDL2 renderer. If you are running on "
                 "Linux, "
                 "probably X11 Display is not setup correctly. Check your "
                 "DISPLAY environment.\n";
    exit(-1);
  }

  std::cout << "SDL2 Renderer creation OK\n";


  GUIContext& gui_ctx = g_gui_ctx;
  gui_ctx.renderer = renderer;

  if (!init_with_empty) {
    //for (size_t i = 0; i < g_gui_ctx.scene.geom_meshes.size(); i++) {
    //  example::DrawGeomMesh draw_mesh(&g_gui_ctx.scene.geom_meshes[i]);
    //  gui_ctx.render_scene.draw_meshes.push_back(draw_mesh);
    //}

    // Setup render mesh
    if (!gui_ctx.render_scene.Setup()) {
      std::cerr << "Failed to setup render mesh.\n";
      exit(-1);
    }
    std::cout << "Setup render mesh\n";
  }

  bool done = false;

  ImGui::CreateContext();
  ImNodes::CreateContext();

  {

    float ddpi, hdpi, vdpi;
    if (SDL_GetDisplayDPI(0, &ddpi, &hdpi, &vdpi) != 0) {
        fprintf(stderr, "Failed to obtain DPI information for display 0: %s\n", SDL_GetError());
        exit(1);
    }
    std::cout << "ddpi " << ddpi << ", hdpi " << hdpi << ", vdpi " << vdpi << "\n";

    float dpi_scaling = ddpi / 72.0f;

    ImGuiIO& io = ImGui::GetIO();

    if (dpi_scaling >= 144.0f) {
      // https://github.com/ocornut/imgui/issues/1786
      // nx DisplayFrameBufferScale + nx font_size + FontGlobalScale 0.5 may give nicer visual on High DPI monitor
      io.FontGlobalScale = 0.5f;
    }

    io.DisplayFramebufferScale = {dpi_scaling , dpi_scaling}; // HACK

    ImFontConfig roboto_config;
    strcpy(roboto_config.Name, "Roboto");

    float font_size = 18.0 * dpi_scaling;

    //ImFontConfig font_config;
    //font_config.SizePixels = 18.0f * xscale;
    //io.Fonts->AddFontDefault(&font_config);
    io.Fonts->AddFontFromMemoryCompressedTTF(roboto_mono_compressed_data,
                                             roboto_mono_compressed_size,
                                             font_size, &roboto_config);
  }

  ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
  ImGui_ImplSDLRenderer_Init(renderer);

  std::cout << "Imgui initialized\n";

  gui_ctx.texture = SDL_CreateTexture(
      renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_TARGET,
      gui_ctx.render_width, gui_ctx.render_height);
  {
    SDL_SetRenderTarget(renderer, gui_ctx.texture);
    SDL_SetRenderDrawColor(renderer, 255, 0, 255, 255);
    SDL_RenderClear(renderer);
    SDL_SetRenderTarget(renderer, nullptr);

    // UpdateTexutre(texture, 0);
  }

  ScreenActivate(window);

  gui_ctx.aov.Resize(gui_ctx.render_width, gui_ctx.render_height);
  UpdateTexutre(gui_ctx.texture, gui_ctx, gui_ctx.aov);

  int display_w, display_h;
  ImVec4 clear_color = {0.1f, 0.18f, 0.3f, 1.0f};

  // init camera matrix
  {
    auto q = ToQuaternion(radians(gui_ctx.yaw), radians(gui_ctx.pitch),
                          radians(gui_ctx.roll));
    gui_ctx.camera.quat[0] = q[0];
    gui_ctx.camera.quat[1] = q[1];
    gui_ctx.camera.quat[2] = q[2];
    gui_ctx.camera.quat[3] = q[3];
  }

#if __EMSCRIPTEN__ || defined(EMULATE_EMSCRIPTEN)
  // no thread
#else
  std::thread render_thread(RenderThread, &gui_ctx);
#endif

  // Initial rendering requiest
  gui_ctx.redraw = true;

  std::map<std::string, int> aov_list = {
      {"color", GUIContext::AOV_COLOR},
      {"shading normal", GUIContext::AOV_SHADING_NORMAL},
      {"geometric normal", GUIContext::AOV_GEOMETRIC_NORMAL},
      {"texcoord", GUIContext::AOV_TEXCOORD}};

  std::string aov_name = "color";

#if __EMSCRIPTEN__ || defined(EMULATE_EMSCRIPTEN)

#if __EMSCRIPTEN__
  std::cout << "enter loop\n";
  emscripten_request_animation_frame_loop(em_main_loop_frame, /* fps */ 0);

  // render_thread.join();
  std::cout << "quit\n";
#else

  while (!done) {
    auto ret = em_main_loop_frame(0, nullptr);
    if (ret == EM_FALSE) {
      break;
    }
  }

#endif

#else
  // Enable drop file
  SDL_EventState(SDL_DROPFILE, SDL_ENABLE);

#if 0
  // Enable Docking;
  {
    ImGuiIO& io& io = ImGui::GetIO();

    // Enable docking(available in imgui `docking` branch at the moment)
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
  }
#endif

  while (!done) {
    ImGuiIO& io = ImGui::GetIO();

    int wheel = 0;

    SDL_Event e;
    while (SDL_PollEvent(&e)) {
      if (e.type == SDL_QUIT) {
        done = true;
      } else if (e.type == SDL_DROPFILE) {
        char* filepath = e.drop.file;

        printf("File dropped: %s\n", filepath);

        std::string fname = filepath;

        // Scene reloading is done in render thread.
        g_gui_ctx.filename = fname;
        g_gui_ctx.request_reload = true;

        SDL_free(filepath);

      } else if (e.type == SDL_WINDOWEVENT) {
        if (e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
          io.DisplaySize.x = static_cast<float>(e.window.data1);
          io.DisplaySize.y = static_cast<float>(e.window.data2);
        }
      } else if (e.type == SDL_KEYDOWN) {
        if (e.key.keysym.sym == SDLK_ESCAPE) {
          done = true;
        }
      } else if (e.type == SDL_KEYUP) {
      } else if (e.type == SDL_MOUSEWHEEL) {
        wheel = e.wheel.y;
      }
    }

    int mouseX, mouseY;
    const int buttons = SDL_GetMouseState(&mouseX, &mouseY);

    // Setup low-level inputs (e.g. on Win32, GetKeyboardState(), or
    // write to those fields from your Windows message loop handlers,
    // etc.)

    io.DeltaTime = 1.0f / 60.0f;
    io.MousePos =
        ImVec2(static_cast<float>(mouseX), static_cast<float>(mouseY));
    io.MouseDown[0] = buttons & SDL_BUTTON(SDL_BUTTON_LEFT);
    io.MouseDown[1] = buttons & SDL_BUTTON(SDL_BUTTON_RIGHT);
    io.MouseWheel = static_cast<float>(wheel);

    ImGui_ImplSDLRenderer_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    ImGui::Begin("Scene");
    bool update = false;

    bool update_display = false;

#if defined(USDVIEW_USE_NATIVEFILEDIALOG)
    if (ImGui::Button("Open file ...")) {
      std::string filename = OpenFileDialog();
      std::cout << "TODO: Open file"
                << "\n";
    }

    ImGui::SameLine();
    HelpMarker("You can also drop USDZ file to the window to open a file.");
#endif

    if (example::ImGuiComboUI("aov", aov_name, aov_list)) {
      gui_ctx.aov_mode = aov_list[aov_name];
      update_display = true;
    }

    // update |= ImGui::InputFloat3("eye", gui_ctx.camera.eye);
    // update |= ImGui::InputFloat3("look_at", gui_ctx.camera.look_at);
    // update |= ImGui::InputFloat3("up", gui_ctx.camera.up);
    update |=
        ImGui::SliderFloat("eye.z", &gui_ctx.camera.eye[2], -1000.0, 1000.0f);
    update |= ImGui::SliderFloat("fov", &gui_ctx.camera.fov, 0.01f, 140.0f);

    // TODO: Validate coordinate definition.
    if (ImGui::SliderFloat("yaw", &gui_ctx.yaw, -360.0f, 360.0f)) {
      auto q = ToQuaternion(radians(gui_ctx.yaw), radians(gui_ctx.pitch),
                            radians(gui_ctx.roll));
      gui_ctx.camera.quat[0] = q[0];
      gui_ctx.camera.quat[1] = q[1];
      gui_ctx.camera.quat[2] = q[2];
      gui_ctx.camera.quat[3] = q[3];
      update = true;
    }
    if (ImGui::SliderFloat("pitch", &gui_ctx.pitch, -360.0f, 360.0f)) {
      auto q = ToQuaternion(radians(gui_ctx.yaw), radians(gui_ctx.pitch),
                            radians(gui_ctx.roll));
      gui_ctx.camera.quat[0] = q[0];
      gui_ctx.camera.quat[1] = q[1];
      gui_ctx.camera.quat[2] = q[2];
      gui_ctx.camera.quat[3] = q[3];
      update = true;
    }
    if (ImGui::SliderFloat("roll", &gui_ctx.roll, -360.0f, 360.0f)) {
      auto q = ToQuaternion(radians(gui_ctx.yaw), radians(gui_ctx.pitch),
                            radians(gui_ctx.roll));
      gui_ctx.camera.quat[0] = q[0];
      gui_ctx.camera.quat[1] = q[1];
      gui_ctx.camera.quat[2] = q[2];
      gui_ctx.camera.quat[3] = q[3];
      update = true;
    }
    ImGui::End();

    ImGui::Begin("Image");
    ImGui::Image(gui_ctx.texture,
                 ImVec2(gui_ctx.render_width, gui_ctx.render_height));
    ImGui::End();

    //NodeTreeWindow(gui_ctx.scene);
    //MaterialsParamWindow(gui_ctx.scene);
    //ShaderParamWindow(gui_ctx.scene);
    //ShaderGraphWindow(gui_ctx.scene);

    if (update) {
      gui_ctx.redraw = true;
    }

#if 0
    // Draw scene
    if ((scene.root_node >= 0) && (scene.root_node < scene.nodes.size())) {
      DrawNode(scene, scene.nodes[scene.root_node]);
    }
#endif

    // Update texture
    if (gui_ctx.update_texture || update_display) {
      // Update texture for display
      UpdateTexutre(gui_ctx.texture, gui_ctx, gui_ctx.aov);

      gui_ctx.update_texture = false;
    }

    SDL_SetRenderDrawColor(renderer, 114, 144, 154, 255);
    SDL_RenderClear(renderer);

    // Imgui

    ImGui::Render();

    // static int texUpdateCount = 0;
    static int frameCount = 0;
    static double currentTime =
        SDL_GetTicks() / 1000.0;  // GetTicks returns milliseconds
    static double previousTime = currentTime;
    static char title[256];

    frameCount++;
    currentTime = SDL_GetTicks() / 1000.0;
    const auto deltaTime = currentTime - previousTime;
    if (deltaTime >= 1.0) {
      sprintf(title, "Simple USDZ viewer [%dFPS]", frameCount);
      SDL_SetWindowTitle(window, title);
      frameCount = 0;
      previousTime = currentTime;
    }

    SDL_RenderClear(renderer);
    ImGui_ImplSDLRenderer_RenderDrawData(ImGui::GetDrawData());
    SDL_RenderPresent(renderer);
  };

  // Notify render thread to exit app
  gui_ctx.quit = true;

  render_thread.join();

  ImGui_ImplSDLRenderer_Shutdown();
  ImGui_ImplSDL2_Shutdown();

  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);

  SDL_Quit();

  ImNodes::DestroyContext();
  ImGui::DestroyContext();

#if defined(USDVIEW_USE_NATIVEFILEDIALOG)
  NFD_Quit();
#endif

#endif

  return EXIT_SUCCESS;
}
