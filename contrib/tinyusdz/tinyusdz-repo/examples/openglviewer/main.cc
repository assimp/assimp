#include <algorithm>
#include <atomic>
#include <cassert>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <map>
#include <mutex>
#include <thread>
#include <vector>

#if defined(_MSC_VER)
#include <direct.h>  // _getcwd
#endif

// GL
//
// glad must be included before glfw3.h
// TODO: Use imgui's imgui_impl_opengl3_loader.h
#if defined(_WIN32)
#if !defined(NOMINMAX)
#define NOMINMAX
#endif
#include <Windows.h>
#endif

#include "glad/glad.h"
//
#include <GLFW/glfw3.h>

// GUI common
#include "../common/imgui/imgui.h"
#include "../common/imgui/imgui_impl_glfw.h"
#include "../common/imgui/imgui_impl_opengl3.h"
#include "../common/trackball.h"
#include "../common/viewport_camera.hh"

// TinyUSDZ
// include relative to openglviewer example cmake top dir for clangd lsp.
#include "external/linalg.h"
#include "io-util.hh"
#include "linear-algebra.hh"
#include "pprinter.hh"  // import to_string(tinyusdz::value::***)
#include "tinyusdz.hh"
#include "tydra/render-data.hh"
#include "tydra/scene-access.hh"
#include "value-pprint.hh"  // import to_string(tinyusdz::value::***)

// local
#include "shader.hh"

// variable name must match in shaders/***.vert
constexpr auto kAttribPoints = "input_position";
constexpr auto kAttribNormals = "input_normal";
constexpr auto kAttribTexCoordBase = "input_uv";
constexpr auto kAttribTexCoord0 = "input_uv";
constexpr auto kMaxTexCoords = 1;  // TODO: multi texcoords

constexpr auto kUniformModelMatrix = "modelMatrix";
constexpr auto kUniformNormalMatrix = "normalMatrix";
constexpr auto kUniformMVPMatrix = "mvp";

constexpr auto kUniformDiffuseTex = "diffuseTex";
constexpr auto kUniformDiffuseTexTransform = "diffuseTexTransform";
constexpr auto kUniformDiffuseTexScaleAndBias =
    "diffuseTexScaleAndBias";  // (sx, sy, bx, by)

constexpr auto kUniformNormaTex = "normalTex";
constexpr auto kUniformNormaTexTransform = "normalTexTransform";
constexpr auto kUniformNormalTexScaleAndBias =
    "normalTexScaleAndBias";  // (sx, sy, bx, by)

constexpr auto kUniformOcclusionTex = "occlusionlTex";
constexpr auto kUniformOcclusionTexTransform = "occlusionlTexTransform";
constexpr auto kUniformOcclusionTexScaleAndBias =
    "occlusionTexScaleAndBias";  // (sx, sy, bx, by)

// Embedded shaders
#include "shaders/no_skinning.vert_inc.hh"
#include "shaders/normals.frag_inc.hh"
#include "shaders/usdpreviewsurface.frag_inc.hh"
#include "shaders/world_fragment.frag_inc.hh"

#define CHECK_GL(tag)                                                        \
  do {                                                                       \
    GLenum err = glGetError();                                               \
    if (err != GL_NO_ERROR) {                                                \
      std::cerr << "[" << tag << "] " << __FILE__ << ":" << __LINE__ << ":"  \
                << __func__ << " code " << std::to_string(int(err)) << "\n"; \
    }                                                                        \
  } while (0)

struct GLTexParams {
  // std::map<std::string, GLint> uniforms;
  GLenum wrapS{GL_REPEAT};
  GLenum wrapT{GL_REPEAT};
  std::array<float, 4> borderCol{0.0f, 0.0f, 0.0f, 0.0f};  // transparent black

  // Use 3x3 mat to support pivot transform.
  tinyusdz::tydra::mat3 uv_transform{tinyusdz::tydra::mat3::identity()};
};

struct GLTexState {
  GLTexParams texParams;
  std::string sampler_name;
  uint32_t slot_id{0};
  GLuint tex_id;    // glBindTexture id
  GLint u_tex{-1};  // sampler glUniform location

  GLint u_transform;  // texcoord transform
};

template <typename T>
struct GLTexOrFactor {
  GLTexOrFactor(const T &v) : factor(v) {}

  GLTexState tex;
  T factor;
  GLint u_factor{-1};
};

template <typename T>
struct GLUniformFactor {
  GLUniformFactor(const T &v) : factor(v) {}

  T factor;
  GLint u_factor{-1};
};

struct GLUsdPreviewSurfaceState {
  static constexpr auto kDiffuseColor = "diffuseColor";
  static constexpr auto kEmissiveColor = "emissiveColor";
  static constexpr auto kSpecularColor = "specularColor";
  static constexpr auto kUseSpecularWorkflow = "useSpecularWorkflow";
  static constexpr auto kMetallic = "metallic";
  static constexpr auto kRoughness = "roughness";
  static constexpr auto kClearcoat = "clearcoat";
  static constexpr auto kClearcoatRoughness = "clearcoatRoughness";
  static constexpr auto kOpacity = "opacity";
  static constexpr auto kOpacityThreshold = "opacityThreshold";
  static constexpr auto kIor = "ior";
  static constexpr auto kNormal = "normal";
  static constexpr auto kOcclusion = "occlusion";

  GLTexOrFactor<tinyusdz::tydra::vec3> diffuseColor{{0.18f, 0.18f, 0.18f}};
  GLTexOrFactor<tinyusdz::tydra::vec3> emissiveColor{{0.0f, 0.0f, 0.0f}};

  GLUniformFactor<int> useSpecularWorkflow{0};  // non-texturable

  GLTexOrFactor<tinyusdz::tydra::vec3> specularColor{
      {0.0f, 0.0f, 0.0f}};              // useSpecularWorkflow = 1
  GLTexOrFactor<float> metallic{0.0f};  // useSpecularWorkflow = 0

  GLTexOrFactor<float> roughness{0.5f};
  GLTexOrFactor<float> clearcoat{0.0f};
  GLTexOrFactor<float> clearcoatRoughness{0.01f};
  GLTexOrFactor<float> opacity{1.0f};
  GLTexOrFactor<float> opacityThreshold{0.0f};

  GLTexOrFactor<float> ior{1.5f};

  GLTexOrFactor<tinyusdz::tydra::vec3> normal{
      {0.0f, 0.0f, 1.0f}};  // normal map

  // No displacement mapping on OpenGL
  // GLTexOrFactor<float> displacement{0.0f};

  GLTexOrFactor<float> occlusion{1.0f};
};

template <typename T>
bool SetupGLUsdPreviewSurfaceParam(const GLuint prog_id,
                                   const tinyusdz::tydra::RenderScene &scene,
                                   const std::string &base_shadername,
                                   const tinyusdz::tydra::ShaderParam<T> &s,

                                   GLTexOrFactor<T> &dst) {
  if (s.is_texture()) {
    {
      std::string u_name = base_shadername + "Tex";
      GLint loc = glGetUniformLocation(prog_id, u_name.c_str());
      dst.tex.u_tex = loc;
    }

    {
      std::string u_name = base_shadername + "TexTransform";
      GLint loc = glGetUniformLocation(prog_id, u_name.c_str());
      dst.tex.u_transform = loc;
      if (s.textureId < 0 || s.textureId >= scene.textures.size()) {
        std::cerr << "Invalid txtureId for " << base_shadername + "\n";
      } else {
        const tinyusdz::tydra::UVTexture &uvtex =
            scene.textures[size_t(s.textureId)];
        dst.tex.texParams.uv_transform = uvtex.transform;
      }
    }

  } else {
    GLint loc = glGetUniformLocation(prog_id, base_shadername.c_str());
    if (loc < 0) {
      std::cerr << base_shadername << " uniform not found in the shader.\n";
    }
    dst.u_factor = loc;
    dst.factor = s.value;
  }

  return true;
}

bool ReloadShader(GLuint prog_id, const std::string &vert_filepath,
                  const std::string &frag_filepath) {
  std::string vert_str;
  std::string frag_str;

  if (vert_filepath.size() && tinyusdz::io::FileExists(vert_filepath)) {
    std::vector<uint8_t> bytes;
    std::string err;
    if (!tinyusdz::io::ReadWholeFile(&bytes, &err, vert_filepath)) {
      std::cerr << "Read vertg shader failed: " << err << "\n";
      return false;
    }

    vert_str =
        std::string(reinterpret_cast<char *>(bytes.data()), bytes.size());

    std::cout << "VERT:\n" << vert_str << "\n";
  }

  if (frag_filepath.size() && tinyusdz::io::FileExists(frag_filepath)) {
    std::vector<uint8_t> bytes;
    std::string err;
    if (!tinyusdz::io::ReadWholeFile(&bytes, &err, frag_filepath)) {
      std::cerr << "Read frag shader failed: " << err << "\n";
      return false;
    }

    frag_str =
        std::string(reinterpret_cast<char *>(bytes.data()), bytes.size());

    std::cout << "FRAG:\n" << frag_str << "\n";
  }

  // TODO
  return true;
}

bool SetupGLUsdPreviewSurface(GLuint prog_id,
                              tinyusdz::tydra::RenderScene &scene,
                              tinyusdz::tydra::RenderMaterial &m,
                              GLUsdPreviewSurfaceState &dst) {
  const auto surfaceShader = m.surfaceShader;

  if (!SetupGLUsdPreviewSurfaceParam(
          prog_id, scene, GLUsdPreviewSurfaceState::kDiffuseColor,
          surfaceShader.diffuseColor, dst.diffuseColor)) {
    return false;
  }

  if (!SetupGLUsdPreviewSurfaceParam(
          prog_id, scene, GLUsdPreviewSurfaceState::kEmissiveColor,
          surfaceShader.emissiveColor, dst.emissiveColor)) {
    return false;
  }

  if (!SetupGLUsdPreviewSurfaceParam(
          prog_id, scene, GLUsdPreviewSurfaceState::kSpecularColor,
          surfaceShader.specularColor, dst.specularColor)) {
    return false;
  }

  if (!SetupGLUsdPreviewSurfaceParam(prog_id, scene,
                                     GLUsdPreviewSurfaceState::kMetallic,
                                     surfaceShader.metallic, dst.metallic)) {
    return false;
  }

  if (!SetupGLUsdPreviewSurfaceParam(prog_id, scene,
                                     GLUsdPreviewSurfaceState::kRoughness,
                                     surfaceShader.roughness, dst.roughness)) {
    return false;
  }

  if (!SetupGLUsdPreviewSurfaceParam(prog_id, scene,
                                     GLUsdPreviewSurfaceState::kClearcoat,
                                     surfaceShader.clearcoat, dst.clearcoat)) {
    return false;
  }

  if (!SetupGLUsdPreviewSurfaceParam(
          prog_id, scene, GLUsdPreviewSurfaceState::kClearcoatRoughness,
          surfaceShader.clearcoatRoughness, dst.clearcoatRoughness)) {
    return false;
  }

  if (!SetupGLUsdPreviewSurfaceParam(prog_id, scene,
                                     GLUsdPreviewSurfaceState::kOpacity,
                                     surfaceShader.opacity, dst.opacity)) {
    return false;
  }

  if (!SetupGLUsdPreviewSurfaceParam(
          prog_id, scene, GLUsdPreviewSurfaceState::kOpacityThreshold,
          surfaceShader.opacityThreshold, dst.opacityThreshold)) {
    return false;
  }

  if (!SetupGLUsdPreviewSurfaceParam(prog_id, scene,
                                     GLUsdPreviewSurfaceState::kIor,
                                     surfaceShader.ior, dst.ior)) {
    return false;
  }

  if (!SetupGLUsdPreviewSurfaceParam(prog_id, scene,
                                     GLUsdPreviewSurfaceState::kOcclusion,
                                     surfaceShader.occlusion, dst.occlusion)) {
    return false;
  }

  if (!SetupGLUsdPreviewSurfaceParam(prog_id, scene,
                                     GLUsdPreviewSurfaceState::kNormal,
                                     surfaceShader.normal, dst.normal)) {
    return false;
  }

  {
    GLint loc = glGetUniformLocation(
        prog_id, GLUsdPreviewSurfaceState::kUseSpecularWorkflow);
    if (loc < 0) {
      std::cerr << GLUsdPreviewSurfaceState::kUseSpecularWorkflow
                << " uniform not found in the shader.\n";
    }
    dst.useSpecularWorkflow.factor =
        surfaceShader.useSpecularWorkFlow ? 1.0f : 0.0f;
    dst.useSpecularWorkflow.u_factor = loc;
  }

  // TODO: `displacement` param

  return true;
}

struct GLVertexUniformState {
  GLint u_model{-1};
  GLint u_normal{-1};
  GLint u_mvp{-1};

  std::array<float, 16> modelMatrix[16];
  std::array<float, 9> normalMatrix[9];  // 3x3 transpose(inverse(model * view))
  std::array<float, 16> mvp[16];         // modeviewprojection
};

// TODO: Use handle_id for key
struct GLMeshState {
  std::map<std::string, GLint> attribs;
  std::vector<GLuint> diffuseTexHandles;
  GLuint vertex_array_object{0};  // vertex array object
  GLuint num_triangles{0};        // up to 4GB triangles
};

struct GLNodeState {
  GLVertexUniformState gl_v_uniform_state;
  GLMeshState gl_mesh_state;

  GLTexState gl_tex_state;
};

struct GLProgramState {
  // std::map<std::string, GLint> uniforms;

  std::map<std::string, example::shader> shaders;
};

struct GLScene {
  std::vector<GLNodeState> gl_nodes;

  // scene bounding box
  std::array<float, 3> bmin;
  std::array<float, 3> bmax;
};

struct GUIContext {
  enum AOV {
    AOV_COLOR = 0,
    AOV_NORMAL,
    AOV_POSITION,
    AOV_DEPTH,
    AOV_TEXCOORD,
    AOV_VARYCOORD,
    AOV_VERTEXCOLOR
  };
  AOV aov{AOV_COLOR};

  int width = 1024;
  int height = 768;

  int mouse_x = -1;
  int mouse_y = -1;

  bool mouse_left_down = false;
  bool shift_pressed = false;
  bool ctrl_pressed = false;
  bool tab_pressed = false;

  float curr_quat[4] = {0.0f, 0.0f, 0.0f, 1.0f};
  float prev_quat[4] = {0.0f, 0.0f, 0.0f, 1.0f};

  float xrotate = 0.0f;  // in degree
  float yrotate = 0.0f;  // in degree

  float fov = 45.0f;  // in degree
  float znear = 0.01f;
  float zfar = 1000.0f;

  std::array<float, 3> eye = {0.0f, 0.5f, -5.0f};
  std::array<float, 3> lookat = {0.0f, 0.0f, 0.0f};
  std::array<float, 3> up = {0.0f, 1.0f, 0.0f};

  example::Camera camera;

  GLUsdPreviewSurfaceState *selected_surfaceShader{nullptr};

  std::vector<GLUsdPreviewSurfaceState> surfaceShaders;

  // for ImGui
  std::vector<std::string> surfaceShaderNames;
  std::string selected_surfaceShaderName;

  std::string usd_filepath;

  std::string converter_info;
  std::string converter_warn;

  std::string vert_filename{"../shaders/no_skinning.vert"};
  std::string frag_filename{"../shaders/usdpreviewsurface.frag"};
};

GUIContext gCtx;

//
// Combo with std::vector<std::string>
//
static bool ImGuiComboUI(const std::string &caption, std::string &current_item,
                         const std::vector<std::string> &items) {
  bool changed = false;

  if (ImGui::BeginCombo(caption.c_str(), current_item.c_str())) {
    for (int n = 0; n < items.size(); n++) {
      bool is_selected = (current_item == items[n]);
      if (ImGui::Selectable(items[n].c_str(), is_selected)) {
        current_item = items[n];
        changed = true;
      }
      if (is_selected) {
        // Set the initial focus when opening the combo (scrolling + for
        // keyboard navigation support in the upcoming navigation branch)
        ImGui::SetItemDefaultFocus();
      }
    }
    ImGui::EndCombo();
  }

  return changed;
}

static void MaterialUI()
{
  ImGui::Begin("Material");

  ImGuiComboUI("surfaceShader", gCtx.selected_surfaceShaderName, gCtx.surfaceShaderNames);

  ImGui::End();
}

static void UsdPreviewSurfaceParamUI(
  GLUsdPreviewSurfaceState &state)
{
  bool changed = false;

  ImGui::Begin("Shader param");

  changed |= ImGui::ColorEdit3("diffuseColor", &state.diffuseColor.factor[0]);
  changed |= ImGui::ColorEdit3("emissiveColor", &state.emissiveColor.factor[0]);
  bool specWorkflow = state.useSpecularWorkflow.factor > 0 ? true : false;
  changed |= ImGui::Checkbox("useSpecularWorkflow", &specWorkflow);
  state.useSpecularWorkflow.factor = specWorkflow;

  if (specWorkflow) {
    changed |= ImGui::ColorEdit3("specularColor", &state.specularColor.factor[0]);
  } else {
    changed |= ImGui::SliderFloat("metallic", &state.metallic.factor, 0.0f, 1.0f);
  }

  changed |= ImGui::SliderFloat("clearcoat", &state.clearcoat.factor, 0.0f, 1.0f);
  changed |= ImGui::SliderFloat("clearcoatRoughness", &state.clearcoatRoughness.factor, 0.0f, 1.0f);

  changed |= ImGui::SliderFloat("opacity", &state.opacity.factor, 0.0f, 1.0f);
  changed |= ImGui::SliderFloat("opacityThreshold", &state.opacityThreshold.factor, 0.0f, 1.0f);

  changed |= ImGui::SliderFloat("ior", &state.ior.factor, 0.0f, 6.0f);

  changed |= ImGui::SliderFloat("occlusion", &state.occlusion.factor, 0.0f, 1.0f);


  ImGui::End();
}

// --- glfw ----------------------------------------------------

static void error_callback(int error, const char *description) {
  std::cerr << "GLFW Error : " << error << ", " << description << std::endl;
}

static void key_callback(GLFWwindow *window, int key, int scancode, int action,
                         int mods) {
  (void)scancode;

  ImGuiIO &io = ImGui::GetIO();
  if (io.WantCaptureKeyboard) {
    return;
  }

  if ((key == GLFW_KEY_LEFT_SHIFT) || (key == GLFW_KEY_RIGHT_SHIFT)) {
    auto *param =
        reinterpret_cast<GUIContext *>(glfwGetWindowUserPointer(window));

    param->shift_pressed = (action == GLFW_PRESS);
  }

  if ((key == GLFW_KEY_LEFT_CONTROL) || (key == GLFW_KEY_RIGHT_CONTROL)) {
    auto *param =
        reinterpret_cast<GUIContext *>(glfwGetWindowUserPointer(window));

    param->ctrl_pressed = (action == GLFW_PRESS);
  }

  // ctrl-q
  if ((key == GLFW_KEY_Q) && (action == GLFW_PRESS) &&
      (mods & GLFW_MOD_CONTROL)) {
    glfwSetWindowShouldClose(window, GLFW_TRUE);
  }

  // esc
  if ((key == GLFW_KEY_ESCAPE) && (action == GLFW_PRESS)) {
    glfwSetWindowShouldClose(window, GLFW_TRUE);
  }
}

static void mouse_move_callback(GLFWwindow *window, double x, double y) {
  auto param = reinterpret_cast<GUIContext *>(glfwGetWindowUserPointer(window));
  assert(param);

  if (param->mouse_left_down) {
    float w = static_cast<float>(param->width);
    float h = static_cast<float>(param->height);

    float x_offset = param->width - w;
    float y_offset = param->height - h;

    if (param->ctrl_pressed) {
      const float dolly_scale = 0.1f;
      param->eye[2] += dolly_scale * (param->mouse_y - static_cast<float>(y));
      param->lookat[2] +=
          dolly_scale * (param->mouse_y - static_cast<float>(y));
    } else if (param->shift_pressed) {
      const float trans_scale = 0.02f;
      param->eye[0] += trans_scale * (param->mouse_x - static_cast<float>(x));
      param->eye[1] -= trans_scale * (param->mouse_y - static_cast<float>(y));
      param->lookat[0] +=
          trans_scale * (param->mouse_x - static_cast<float>(x));
      param->lookat[1] -=
          trans_scale * (param->mouse_y - static_cast<float>(y));

    } else {
#if 0
      // Adjust y.
      trackball(param->prev_quat,
                (2.f * (param->mouse_x - x_offset) - w) / static_cast<float>(w),
                (h - 2.f * (param->mouse_y - y_offset)) / static_cast<float>(h),
                (2.f * (static_cast<float>(x) - x_offset) - w) /
                    static_cast<float>(w),
                (h - 2.f * (static_cast<float>(y) - y_offset)) /
                    static_cast<float>(h));
      add_quats(param->prev_quat, param->curr_quat, param->curr_quat);
#else
      const float rotation_amp = 1.0f;
      param->xrotate += rotation_amp * (param->mouse_y - static_cast<float>(y));
      param->yrotate += rotation_amp * (param->mouse_x - static_cast<float>(x));

      // limit rotation around X axis.
      if (param->xrotate < -89) {
        param->xrotate = -89;
      }
      if (param->xrotate > 89) {
        param->xrotate = 89;
      }

#endif
    }
  }

  param->mouse_x = static_cast<int>(x);
  param->mouse_y = static_cast<int>(y);
}

static void mouse_button_callback(GLFWwindow *window, int button, int action,
                                  int mods) {
  ImGuiIO &io = ImGui::GetIO();
  if (io.WantCaptureMouse || io.WantCaptureKeyboard) {
    return;
  }

  auto param = reinterpret_cast<GUIContext *>(glfwGetWindowUserPointer(window));
  assert(param);

  // left button
  if (button == 0) {
    if (action) {
      param->mouse_left_down = true;
      trackball(param->prev_quat, 0.0f, 0.0f, 0.0f, 0.0f);
    } else {
      param->mouse_left_down = false;
    }
  }
}

static void resize_callback(GLFWwindow *window, int width, int height) {
  auto param = reinterpret_cast<GUIContext *>(glfwGetWindowUserPointer(window));
  assert(param);

  param->width = width;
  param->height = height;
}

// ------------------------------------------------

namespace {

void SetupVertexUniforms(GLVertexUniformState &gl_state,
                         const tinyusdz::value::matrix4d &worldmatd,
                         const tinyusdz::value::matrix4f &viewproj) {
  using namespace tinyusdz::value;

  // implicitly casts matrix4d to matrix4f;
  matrix4f worldmat = worldmatd;

  matrix4d invtransmatd =
      tinyusdz::inverse(tinyusdz::upper_left_3x3_only(worldmatd));

  matrix3d invtransmat33d = tinyusdz::to_matrix3x3(invtransmatd);
  matrix3f invtransmat33 = invtransmat33d;

  memcpy(gl_state.modelMatrix, &worldmat.m[0][0], sizeof(float) * 16);
  memcpy(gl_state.normalMatrix, &invtransmat33.m[0][0], sizeof(float) * 9);

  // NOTE: USD uses pre-multiply matmul
  matrix4f mvp = viewproj * worldmat;

  // FIXME:
  memcpy(gl_state.mvp, &mvp.m[0][0], sizeof(float) * 16);
}

void SetVertexUniforms(const GLVertexUniformState &gl_state) {
  if (gl_state.u_model > -1) {
    glUniformMatrix4fv(gl_state.u_model, 1, GL_FALSE,
                       gl_state.modelMatrix->data());
    CHECK_GL("UniformMatrix u_modelview");
  }

  if (gl_state.u_normal > -1) {
    glUniformMatrix3fv(gl_state.u_normal, 1, GL_FALSE,
                       gl_state.normalMatrix->data());
    CHECK_GL("UniformMatrix u_normal");
  }

  if (gl_state.u_mvp > -1) {
    glUniformMatrix4fv(gl_state.u_mvp, 1, GL_FALSE, gl_state.mvp->data());
    CHECK_GL("UniformMatrix u_mvp");
  }
}

void SetTexUniforms(const GLuint prog_id, const GLTexState &gl_tex) {
  glActiveTexture(GL_TEXTURE0 + gl_tex.slot_id);
  glBindTexture(GL_TEXTURE_2D, gl_tex.tex_id);

  GLint loc = glGetUniformLocation(prog_id, gl_tex.sampler_name.c_str());
  if (loc > -1) {
    glUniform1i(loc, gl_tex.slot_id);
  }
  CHECK_GL("glUniform1i u_modelview");
}

bool LoadShaders(GLProgramState &gl_state) {
  // default = show normal vector as color.
  std::string vert_str(reinterpret_cast<char *>(shaders_no_skinning_vert),
                       shaders_no_skinning_vert_len);

  std::string frag_str(reinterpret_cast<char *>(shaders_usdpreviewsurface_frag),
                       shaders_usdpreviewsurface_frag_len);

  example::shader default_shader("default", vert_str, frag_str);

  gl_state.shaders["default"] = std::move(default_shader);

  return true;
}

bool SetupGLUniforms(GLuint prog_id, GLVertexUniformState &gl_v_uniform_state) {
  GLint model_loc = glGetUniformLocation(prog_id, kUniformModelMatrix);
  if (model_loc < 0) {
    std::cerr << kUniformModelMatrix << " not found in the vertex shader.\n";
    // return false;
  } else {
    gl_v_uniform_state.u_model = model_loc;
  }

  GLint norm_loc = glGetUniformLocation(prog_id, kUniformNormalMatrix);
  if (norm_loc < 0) {
    std::cerr << kUniformNormalMatrix << " not found in the vertex shader.\n";
    // return false;
  } else {
    gl_v_uniform_state.u_normal = norm_loc;
  }

  GLint mvp_loc = glGetUniformLocation(prog_id, kUniformMVPMatrix);
  if (mvp_loc < 0) {
    std::cerr << kUniformMVPMatrix << " not found in the vertex shader.\n";
    // return false;
  } else {
    gl_v_uniform_state.u_mvp = mvp_loc;
  }

  return true;
}

bool SetupTexture(const tinyusdz::tydra::RenderScene &scene,
                  tinyusdz::tydra::UVTexture &tex) {
  auto glwrapmode = [](const tinyusdz::tydra::UVTexture::WrapMode mode) {
    if (mode == tinyusdz::tydra::UVTexture::WrapMode::CLAMP_TO_EDGE) {
      return GL_CLAMP_TO_EDGE;
    } else if (mode == tinyusdz::tydra::UVTexture::WrapMode::REPEAT) {
      return GL_REPEAT;
    } else if (mode == tinyusdz::tydra::UVTexture::WrapMode::MIRROR) {
      return GL_MIRRORED_REPEAT;
    } else if (mode == tinyusdz::tydra::UVTexture::WrapMode::CLAMP_TO_BORDER) {
      return GL_CLAMP_TO_BORDER;
    }
    // Just in case: Fallback to REPEAT
    return GL_REPEAT;
  };

  GLTexState texState;
  GLTexParams texParams;

  texParams.wrapS = glwrapmode(tex.wrapS);
  texParams.wrapT = glwrapmode(tex.wrapT);

  GLuint texid{0};
  glGenTextures(1, &texid);

  glBindTexture(GL_TEXTURE_2D, texid);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, texParams.wrapS);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, texParams.wrapT);

  // Transparent black For `black` wrap mode.
  // https://github.com/PixarAnimationStudios/OpenUSD/commit/2cf6612b2b1d5a1a1031bc153867116c5963e605
  texParams.borderCol = {0.0f, 0.0f, 0.0f, 0.0f};
  glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR,
                   &texParams.borderCol[0]);
  CHECK_GL("texture_id[" << std::to_string(tex.texture_image_id)
                         << "] glTexParameters");

  texState.texParams = std::move(texParams);

  int64_t image_id = tex.texture_image_id;

  if (image_id >= 0) {
    if (image_id < scene.images.size()) {
      const tinyusdz::tydra::TextureImage &image =
          scene.images[size_t(image_id)];

      if ((image.width < 1) || (image.height < 1) || (image.channels < 1)) {
        std::cerr << "Texture image is not loaded(texture file not found?).\n";
        glBindTexture(GL_TEXTURE_2D, 0);
        return false;
      }

      uint32_t bytesperpixel = 1;

      GLenum format{GL_LUMINANCE};
      if (image.channels == 1) {
        format = GL_LUMINANCE;
        bytesperpixel = 1;
      } else if (image.channels == 2) {
        format = GL_LUMINANCE_ALPHA;
        bytesperpixel = 2;
      } else if (image.channels == 3) {
        format = GL_RGB;
        bytesperpixel = 3;
      } else if (image.channels == 4) {
        format = GL_RGBA;
        bytesperpixel = 4;
      }

      GLenum type{GL_BYTE};
      if (image.texelComponentType == tinyusdz::tydra::ComponentType::UInt8) {
        type = GL_BYTE;
        bytesperpixel *= 1;
      } else if (image.texelComponentType ==
                 tinyusdz::tydra::ComponentType::Half) {
        type = GL_SHORT;
        bytesperpixel *= 2;
      } else if (image.texelComponentType ==
                 tinyusdz::tydra::ComponentType::UInt32) {
        type = GL_UNSIGNED_INT;
        bytesperpixel *= 4;
      } else if (image.texelComponentType ==
                 tinyusdz::tydra::ComponentType::Float) {
        type = GL_FLOAT;
        bytesperpixel *= 4;
      } else {
        std::cout << "Unsupported texelComponentType: "
                  << tinyusdz::tydra::to_string(image.texelComponentType)
                  << "\n";
      }

      int64_t buffer_id = image.buffer_id;
      if ((buffer_id >= 0) && (buffer_id < scene.buffers.size())) {
        const tinyusdz::tydra::BufferData &buffer =
            scene.buffers[size_t(buffer_id)];

        // byte length check.
        if (size_t(image.width) * size_t(image.height) * size_t(bytesperpixel) >
            buffer.data.size()) {
          std::cerr << "Insufficient texel data. : "
                    << "width: " << image.width << ", height " << image.height
                    << ", bytesperpixel " << bytesperpixel
                    << ", requested bytes: "
                    << size_t(image.width) * size_t(image.height) *
                           size_t(bytesperpixel)
                    << ", buffer bytes: " << std::to_string(buffer.data.size())
                    << "\n";
          // continue anyway
        } else {
          glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image.width, image.height, 0,
                       format, type, buffer.data.data());
          CHECK_GL("texture_id[" << std::to_string(image_id)
                                 << "] glTexImage2D");
        }
      }
    }
  }

  tex.handle = texid;

  glBindTexture(GL_TEXTURE_2D, 0);

  return true;
}

static void BuildFacevaryingGeometricNormals(
    const std::vector<tinyusdz::tydra::vec3> &points,
    std::vector<tinyusdz::tydra::vec3> &geom_facevarying_normals);

static bool SetupMesh(const tinyusdz::Axis &stageUpAxis,
                      const tinyusdz::tydra::RenderMesh &mesh,
                      GLuint program_id,
                      GLMeshState &gl_state)  // [out]
{
  std::cout << "program_id " << program_id << "\n";
  std::vector<uint32_t> indices;

  if (mesh.faceVertexCounts.empty()) {
    // assume all triangulaged faces.
    if ((mesh.faceVertexIndices.size() % 3) != 0) {
      std::cerr << "mesh <" << mesh.abs_name << ">  faceVertexIndices.size "
                << std::to_string(mesh.faceVertexIndices.size())
                << " must be multiple of 3\n";
    }

    for (size_t f = 0; f < mesh.faceVertexIndices.size() / 3; f++) {
      for (size_t c = 0; c < 3; c++) {
        indices.push_back(mesh.faceVertexIndices[3 * f + c]);
      }
    }
  } else {
    size_t faceVertexIndexOffset = 0;

    // Currently all faces must be triangle.
    for (size_t f = 0; f < mesh.faceVertexCounts.size(); f++) {
      if (mesh.faceVertexCounts[f] != 3) {
        std::cerr << "mesh <" << mesh.abs_name
                  << ">  Non triangle face found at faceVertexCounts[" << f
                  << "] (" << mesh.faceVertexCounts[f] << ")\n";
        return false;
      }

      size_t fvCounts = mesh.faceVertexCounts[f];
      for (size_t c = 0; c < fvCounts; c++) {
        indices.push_back(mesh.faceVertexIndices[faceVertexIndexOffset + c]);
      }

      faceVertexIndexOffset += fvCounts;
    }
  }

  glGenVertexArrays(1, &gl_state.vertex_array_object);
  CHECK_GL(mesh.abs_name << "GenVertexArrays");

  glBindVertexArray(gl_state.vertex_array_object);
  CHECK_GL(mesh.abs_name << "BindVertexArray");

  //
  // Current settings
  // - position
  // - normals
  // - texcoords0
  //
  // all vertex attribs are represented as facevarying data.
  //
  // - Static mesh(STATIC_DRAW) only

  std::vector<tinyusdz::tydra::vec3> facevaryingVertices;
  {  // position

    // expand position to facevarying data.
    // assume faces are all triangle.
    gl_state.num_triangles = indices.size() / 3;

    for (size_t i = 0; i < indices.size() / 3; i++) {
      size_t vi0 = indices[3 * i + 0];
      size_t vi1 = indices[3 * i + 1];
      size_t vi2 = indices[3 * i + 2];

      if (vi0 >= mesh.points.size()) {
        std::cerr << "indices[" << (3 * i + 0) << "(" << vi0
                  << ") exceeds mesh.points.size()(" << mesh.points.size()
                  << ")\n";
        return false;
      }

      if (vi1 >= mesh.points.size()) {
        std::cerr << "indices[" << (3 * i + 1) << "(" << vi1
                  << ") exceeds mesh.points.size()(" << mesh.points.size()
                  << ")\n";
        return false;
      }

      if (vi2 >= mesh.points.size()) {
        std::cerr << "indices[" << (3 * i + 2) << "(" << vi2
                  << ") exceeds mesh.points.size()(" << mesh.points.size()
                  << ")\n";
        return false;
      }

      if (stageUpAxis == tinyusdz::Axis::Z) {
        tinyusdz::tydra::vec3 p0;
        p0[0] = mesh.points[vi0][0];
        p0[1] = mesh.points[vi0][2];
        p0[2] = mesh.points[vi0][1];

        tinyusdz::tydra::vec3 p1;
        p1[0] = mesh.points[vi1][0];
        p1[1] = mesh.points[vi1][2];
        p1[2] = mesh.points[vi1][1];

        tinyusdz::tydra::vec3 p2;
        p2[0] = mesh.points[vi2][0];
        p2[1] = mesh.points[vi2][2];
        p2[2] = mesh.points[vi2][1];

        facevaryingVertices.push_back(p0);
        facevaryingVertices.push_back(p1);
        facevaryingVertices.push_back(p2);
      } else {
        // TODO: upAxis X
        facevaryingVertices.push_back(mesh.points[vi0]);
        facevaryingVertices.push_back(mesh.points[vi1]);
        facevaryingVertices.push_back(mesh.points[vi2]);
      }
    }

    GLuint vb;
    glGenBuffers(1, &vb);
    glBindBuffer(GL_ARRAY_BUFFER, vb);
    glBufferData(GL_ARRAY_BUFFER,
                 facevaryingVertices.size() * sizeof(tinyusdz::tydra::vec3),
                 facevaryingVertices.data(), GL_STATIC_DRAW);
    CHECK_GL("Set facevaryingVertices buffer data");

    GLint loc = glGetAttribLocation(program_id, kAttribPoints);

    if (loc > -1) {
      glEnableVertexAttribArray(loc);
      glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE,
                            /* stride */ sizeof(GLfloat) * 3, 0);
      CHECK_GL("VertexAttribPointer");
    } else {
      std::cerr << "vertex positions: " << kAttribPoints
                << " attribute not found in vertex shader.\n";
      return false;
    }
  }

  std::vector<tinyusdz::tydra::vec3> facevaryingNormals;
  if (mesh.facevaryingNormals.size()) {
    facevaryingNormals = mesh.facevaryingNormals;
  } else {
    BuildFacevaryingGeometricNormals(facevaryingVertices, facevaryingNormals);
  }

  if (facevaryingNormals.size()) {  // normals
    GLuint vb;
    glGenBuffers(1, &vb);
    glBindBuffer(GL_ARRAY_BUFFER, vb);
    glBufferData(GL_ARRAY_BUFFER,
                 mesh.facevaryingNormals.size() * sizeof(tinyusdz::tydra::vec3),
                 mesh.facevaryingNormals.data(), GL_STATIC_DRAW);
    CHECK_GL("Set facevaryingNormals buffer data");

    GLint loc = glGetAttribLocation(program_id, kAttribNormals);

    if (loc > -1) {
      glEnableVertexAttribArray(loc);
      glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE,
                            /* stride */ sizeof(GLfloat) * 3, 0);
      CHECK_GL("VertexAttribPointer");
    } else {
      std::cerr
          << "vertex normals: " << kAttribNormals
          << " attribute not found in vertex shader. Shader does not use it?\n";
      // may ok
    }
  }

  // texcoords0 only
  // TODO: multi texcoords
  if (mesh.facevaryingTexcoords.size() == 1) {
    for (const auto it : mesh.facevaryingTexcoords) {
      uint32_t slot_id = it.first;
      if (slot_id >= kMaxTexCoords) {
        std::cerr << "Texcoord slot id " << slot_id
                  << " must be less than kMaxTexCoords " << kMaxTexCoords
                  << "\n";
        return false;
      }

      GLuint vb;
      glGenBuffers(1, &vb);
      glBindBuffer(GL_ARRAY_BUFFER, vb);
      glBufferData(GL_ARRAY_BUFFER,
                   it.second.size() * sizeof(tinyusdz::tydra::vec2),
                   it.second.data(), GL_STATIC_DRAW);
      CHECK_GL("Set facevaryingTexcoord0 buffer data");

      std::string texattr = kAttribTexCoordBase + std::to_string(slot_id);
      GLint loc = glGetAttribLocation(program_id, texattr.c_str());

      if (loc > -1) {
        glEnableVertexAttribArray(loc);
        glVertexAttribPointer(loc, 2, GL_FLOAT, GL_FALSE,
                              /* stride */ sizeof(GLfloat) * 2, 0);
        CHECK_GL("VertexAttribPointer");
      } else {
        std::cerr << "Texture UV0: " << texattr
                  << " attribute not found in vertex shader.\n";
        // may OK
      }
    }
  }

  // We build facevarying vertex data, so no index buffers.
#if 0
  // Build index buffer.
  GLuint elementbuffer;
  glGenBuffers(1, &elementbuffer);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementbuffer);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);
  CHECK_GL(mesh.abs_name << "ElementArrayBuffer");
#endif

  glBindVertexArray(0);
  CHECK_GL(mesh.abs_name << "UnBind VAO");

  return true;
}

static void DrawMesh(const GLMeshState &gl_state) {
  // Simply bind vertex array object and call glDrawArrays.
  glBindVertexArray(gl_state.vertex_array_object);
  glDrawArrays(GL_TRIANGLES, 0, gl_state.num_triangles * 3);
  CHECK_GL("DrawArrays");
  glBindVertexArray(0);
}

static void DrawNode(GLNodeState &gl_node,
                     const tinyusdz::value::matrix4f &viewproj) {
  // FIXME
  tinyusdz::value::matrix4d identm = tinyusdz::value::matrix4d::identity();
  tinyusdz::value::double3 trans = {0.0, 0.0, 0.0};
  tinyusdz::value::double3 rotate = {0.0, 0.0, 0.0};
  tinyusdz::value::double3 scale = {1.0, 1.0, -1.0};
  tinyusdz::value::matrix4d rotm =
      tinyusdz::trs_angle_xyz(trans, rotate, scale);

  tinyusdz::value::matrix4d modelm = rotm * identm;

  SetupVertexUniforms(gl_node.gl_v_uniform_state, modelm, viewproj);

  SetVertexUniforms(gl_node.gl_v_uniform_state);
  // SetTexUniforms(gl_node.gl_tex_state);

  DrawMesh(gl_node.gl_mesh_state);
}

static void ConvertMatrix(example::mat4 &m, tinyusdz::value::matrix4f &dst) {
  for (size_t i = 0; i < 4; i++) {
    for (size_t j = 0; j < 4; j++) {
      dst.m[i][j] = m[i][j];
    }
  }
}

static void DrawScene(const example::shader &shader, GLScene &scene) {
  //
  // Use single shader for the scene
  //

  tinyusdz::value::matrix4f view;
  ConvertMatrix(gCtx.camera.matrices.view, view);

  tinyusdz::value::matrix4f proj;
  ConvertMatrix(gCtx.camera.matrices.perspective, proj);

  tinyusdz::value::matrix4f viewproj = view * proj;

  // bind program
  shader.use();
  CHECK_GL("shader.use");

  for (size_t i = 0; i < scene.gl_nodes.size(); i++) {
    auto &gl_node = scene.gl_nodes[i];
    DrawNode(gl_node, viewproj);
  }

  glUseProgram(0);
  CHECK_GL("glUseProgram(0)");
}

static void ComputeBoundingBox(const tinyusdz::tydra::RenderMesh &mesh,
                               std::array<float, 3> &bmin,
                               std::array<float, 3> &bmax) {
  bmin = {std::numeric_limits<float>::infinity(),
          std::numeric_limits<float>::infinity(),
          std::numeric_limits<float>::infinity()};

  bmax = {-std::numeric_limits<float>::infinity(),
          -std::numeric_limits<float>::infinity(),
          -std::numeric_limits<float>::infinity()};

  for (const auto &p : mesh.points) {
    bmin[0] = (std::min)(bmin[0], p[0]);
    bmin[1] = (std::min)(bmin[1], p[1]);
    bmin[2] = (std::min)(bmin[2], p[2]);

    bmax[0] = (std::max)(bmax[0], p[0]);
    bmax[1] = (std::max)(bmax[1], p[1]);
    bmax[2] = (std::max)(bmax[2], p[2]);
  }
}

static bool ProcScene(const example::shader &gl_shader,
                      const tinyusdz::Stage &stage,
                      std::string &asset_search_path, GLScene *scene) {
  tinyusdz::Axis upAxis{tinyusdz::Axis::Y};
  if (stage.metas().upAxis.authored()) {
    upAxis = stage.metas().upAxis.get_value();
  }
  std::cout << "upAxis " << tinyusdz::to_string(upAxis) << "\n";

  //
  // Stage to Renderable Scene
  //
  tinyusdz::tydra::RenderSceneConverter converter;

  converter.set_search_paths({asset_search_path});

  tinyusdz::tydra::RenderScene renderScene;
  bool ret = converter.ConvertToRenderScene(stage, &renderScene);
  if (converter.GetWarning().size()) {
    std::cout << "ConvertToRenderScene WARN: " << converter.GetWarning()
              << "\n";

    gCtx.converter_warn = converter.GetWarning();
  }

  if (!ret) {
    std::cerr << "Failed to convert USD Stage to OpenGL-like data structure: "
              << converter.GetError() << "\n";
    exit(-1);
  }

  std::cout << "# of meshes: " << renderScene.meshes.size() << "\n";
  std::cout << "# of textures: " << renderScene.textures.size() << "\n";

  gCtx.converter_info = converter.GetInfo();

  std::array<float, 3> scene_bmin;
  std::array<float, 3> scene_bmax;

  scene_bmin = {std::numeric_limits<float>::infinity(),
                std::numeric_limits<float>::infinity(),
                std::numeric_limits<float>::infinity()};

  scene_bmax = {-std::numeric_limits<float>::infinity(),
                -std::numeric_limits<float>::infinity(),
                -std::numeric_limits<float>::infinity()};

  tinyusdz::value::matrix4f view;
  ConvertMatrix(gCtx.camera.matrices.view, view);

  tinyusdz::value::matrix4f proj;
  ConvertMatrix(gCtx.camera.matrices.perspective, proj);

  tinyusdz::value::matrix4f viewproj = proj * view;

  std::map<std::string, GLTexState> gl_tex_state_map;

  for (size_t i = 0; i < renderScene.textures.size(); i++) {
    SetupTexture(renderScene, renderScene.textures[i]);
  }

  // TODO: Material

  for (size_t i = 0; i < renderScene.meshes.size(); i++) {
    std::array<float, 3> bmin;
    std::array<float, 3> bmax;

    ComputeBoundingBox(renderScene.meshes[i], bmin, bmax);

    std::cout << "mesh[" << i << "].bmin " << bmin[0] << ", " << bmin[1] << ", "
              << bmin[2] << "\n";
    std::cout << "mesh[" << i << "].bmax " << bmax[0] << ", " << bmax[1] << ", "
              << bmax[2] << "\n";

    // TODO: accounnt for xform
    scene_bmin[0] = (std::min)(bmin[0], scene_bmin[0]);
    scene_bmin[1] = (std::min)(bmin[1], scene_bmin[1]);
    scene_bmin[2] = (std::min)(bmin[2], scene_bmin[2]);

    scene_bmax[0] = (std::max)(bmax[0], scene_bmax[0]);
    scene_bmax[1] = (std::max)(bmax[1], scene_bmax[1]);
    scene_bmax[2] = (std::max)(bmax[2], scene_bmax[2]);

    GLMeshState gl_mesh;
    if (!SetupMesh(upAxis, renderScene.meshes[i], gl_shader.get_program(),
                   gl_mesh)) {
      std::cerr << "SetupMesh for mesh[" << i << "] failed.\n";
      exit(-1);
    }

    GLNodeState gl_node;
    gl_node.gl_mesh_state = gl_mesh;

    // FIXME:
    tinyusdz::value::double3 scene_center;
    scene_center[0] = scene_bmin[0] + 0.5 * (scene_bmax[0] - scene_bmin[0]);
    scene_center[1] = scene_bmin[1] + 0.5 * (scene_bmax[1] - scene_bmin[1]);
    scene_center[2] = scene_bmin[2] + 0.5 * (scene_bmax[2] - scene_bmin[2]);

    // FIXME
    tinyusdz::value::matrix4d identm = tinyusdz::value::matrix4d::identity();
    tinyusdz::value::double3 trans = {scene_center[0], scene_center[1],
                                      scene_center[2]};
    tinyusdz::value::double3 rotate = {0.0, 0.0, 0.0};
    tinyusdz::value::double3 scale = {1.0, 1.0, -1.0};
    tinyusdz::value::matrix4d rotm =
        tinyusdz::trs_angle_xyz(trans, rotate, scale);

    tinyusdz::value::matrix4d modelm = rotm * identm;

    std::cout << "global matrix: " << identm << "\n";
    SetupVertexUniforms(gl_node.gl_v_uniform_state, modelm, viewproj);
    SetupGLUniforms(gl_shader.get_program(), gl_node.gl_v_uniform_state);

    scene->gl_nodes.emplace_back(gl_node);
  }

  scene->bmin = scene_bmin;
  scene->bmax = scene_bmax;

  // TODO
  return true;
}

static void vnormalize(float v[3]) {
  float r;

  r = std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
  if (r == 0.0) return;

  v[0] /= r;
  v[1] /= r;
  v[2] /= r;
}

static void vcross(float v1[3], float v2[3], float result[3]) {
  result[0] = v1[1] * v2[2] - v1[2] * v2[1];
  result[1] = v1[2] * v2[0] - v1[0] * v2[2];
  result[2] = v1[0] * v2[1] - v1[1] * v2[0];
}

// based on mesa.
static void MygluLookAt(float eye[3], float center[3], float up[3]) {
  float forward[3], side[3];
  GLfloat m[4][4];

  forward[0] = center[0] - eye[0];
  forward[1] = center[1] - eye[1];
  forward[2] = center[2] - eye[2];

  vnormalize(forward);

  /* Side = forward x up */
  vcross(forward, up, side);
  vnormalize(side);

  /* Recompute up as: up = side x forward */
  vcross(side, forward, up);

  memset(m, 0, sizeof(GLfloat) * 16);
  m[0][0] = 1.0f;
  m[1][1] = 1.0f;
  m[2][2] = 1.0f;
  m[3][3] = 1.0f;

  m[0][0] = side[0];
  m[1][0] = side[1];
  m[2][0] = side[2];

  m[0][1] = up[0];
  m[1][1] = up[1];
  m[2][1] = up[2];

  m[0][2] = -forward[0];
  m[1][2] = -forward[1];
  m[2][2] = -forward[2];

  glMultMatrixf(&m[0][0]);
  // TODO: Use glTranslated?
  glTranslatef(-eye[0], -eye[1], -eye[2]);
}

std::string find_file(const std::string basefile, int max_parents = 8) {
  if (max_parents > 16) {
    return std::string();
  }

  std::string filepath = basefile;

  for (size_t i = 0; i < max_parents; i++) {
    if (tinyusdz::io::FileExists(filepath)) {
      return filepath;
    }

    filepath = "../" + filepath;
  }

  return std::string();
}

static void BuildFacevaryingGeometricNormals(
    const std::vector<tinyusdz::tydra::vec3> &points,
    std::vector<tinyusdz::tydra::vec3> &geom_facevarying_normals) {
  if ((points.size() % 3) != 0) {
    return;
  }

  size_t npoints = points.size() / 3;

  for (size_t i = 0; i < npoints; i++) {
    tinyusdz::value::point3f p0;
    p0.x = points[3 * i + 0][0];
    p0.y = points[3 * i + 0][1];
    p0.z = points[3 * i + 0][2];
    tinyusdz::value::point3f p1;
    p1.x = points[3 * i + 1][0];
    p1.y = points[3 * i + 1][1];
    p1.z = points[3 * i + 1][2];
    tinyusdz::value::point3f p2;
    p2.x = points[3 * i + 2][0];
    p2.y = points[3 * i + 2][1];
    p2.z = points[3 * i + 2][2];

    tinyusdz::value::point3f p10 = p1 - p0;
    tinyusdz::value::point3f p20 = p2 - p0;

    // CCW
    tinyusdz::value::point3f Ng = tinyusdz::vcross(p10, p20);
    Ng = tinyusdz::vnormalize(Ng);

    tinyusdz::tydra::vec3 nf;
    nf[0] = Ng.x;
    nf[1] = Ng.y;
    nf[2] = Ng.z;

    geom_facevarying_normals.push_back(nf);
    geom_facevarying_normals.push_back(nf);
    geom_facevarying_normals.push_back(nf);
  }
}

static void ImMatrix4Display(const char *label, const float *m) {
  static std::string labels[4];

  labels[0] = std::string(label) + " m0";
  labels[1] = std::string(label) + " m1";
  labels[2] = std::string(label) + " m2";
  labels[3] = std::string(label) + " m3";

  ImGui::InputFloat4(labels[0].c_str(), const_cast<float *>(m), "%.3f",
                     ImGuiInputTextFlags_ReadOnly);
  ImGui::InputFloat4(labels[1].c_str(), const_cast<float *>(m + 4), "%.3f",
                     ImGuiInputTextFlags_ReadOnly);
  ImGui::InputFloat4(labels[2].c_str(), const_cast<float *>(m + 8), "%.3f",
                     ImGuiInputTextFlags_ReadOnly);
  ImGui::InputFloat4(labels[3].c_str(), const_cast<float *>(m + 12), "%.3f",
                     ImGuiInputTextFlags_ReadOnly);
}

static void ImMatrix3Display(const char *label, const float *m) {
  static std::string labels[3];

  labels[0] = std::string(label) + " m0";
  labels[1] = std::string(label) + " m1";
  labels[2] = std::string(label) + " m2";

  ImGui::InputFloat3(labels[0].c_str(), const_cast<float *>(m), "%.3f",
                     ImGuiInputTextFlags_ReadOnly);
  ImGui::InputFloat3(labels[1].c_str(), const_cast<float *>(m + 3), "%.3f",
                     ImGuiInputTextFlags_ReadOnly);
  ImGui::InputFloat3(labels[2].c_str(), const_cast<float *>(m + 6), "%.3f",
                     ImGuiInputTextFlags_ReadOnly);
}

}  // namespace

int main(int argc, char **argv) {
  // Setup window
  glfwSetErrorCallback(error_callback);
  if (!glfwInit()) {
    exit(EXIT_FAILURE);
  }

// Decide GL+GLSL versions
#if defined(IMGUI_IMPL_OPENGL_ES2)
  // GL ES 2.0 + GLSL 100
  const char *glsl_version = "#version 100";
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
  glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
#elif defined(__APPLE__)
  // GL 3.2 + GLSL 150
  const char *glsl_version = "#version 150";
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);  // Required on Mac
#else
  // GL 3.0 + GLSL 130
  const char *glsl_version = "#version 130";
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
  // glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+
  // only glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // 3.0+ only
#endif

  float highDPIscaleFactor = 1.0f;

  float xscale=1.0f;
  float yscale=1.0f;
#if defined(_WIN32) || defined(__linux__)
  // if it's a HighDPI monitor, try to scale everything
  GLFWmonitor *monitor = glfwGetPrimaryMonitor();
  glfwGetMonitorContentScale(monitor, &xscale, &yscale);
  std::cout << "monitor xscale, yscale = " << xscale << ", " << yscale << "\n";
  if (xscale > 1 || yscale > 1) {
    highDPIscaleFactor = xscale;
    glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_TRUE);
  }
#elif __APPLE__
  glfwWindowHint(GLFW_COCOA_RETINA_FRAMEBUFFER, GLFW_FALSE);
#endif

#ifdef _DEBUG_OPENGL
  glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
#endif

  std::string filename = "models/suzanne.usdc";
  // std::string filename = "models/texture-cat-plane.usdc";
  // std::string filename = "models/texturedcube.usdc";
  // std::string filename = "models/simple-plane.usdz";
#if defined(_MSC_VER)
  std::cout << "cwd: " << _getcwd(nullptr, 0) << "\n";
#endif

  if (argc > 1) {
    filename = std::string(argv[1]);
  }

  std::string full_filepath = find_file(filename);
  if (full_filepath.empty()) {
    std::cerr << "cannot find or file not exists: " << filename << "\n";
  }

  std::cout << "Loading USD file " << full_filepath << "\n";

  std::string warn;
  std::string err;
  tinyusdz::Stage stage;

  bool ret = tinyusdz::LoadUSDFromFile(full_filepath, &stage, &warn, &err);
  if (!warn.empty()) {
    std::cerr << "WARN : " << warn << "\n";
    return EXIT_FAILURE;
  }
  if (!err.empty()) {
    std::cerr << "ERR : " << err << "\n";
    return EXIT_FAILURE;
  }

  std::string basedir = tinyusdz::io::GetBaseDir(full_filepath);
  std::cout << "basedir = " << basedir << "\n";

  gCtx.usd_filepath = full_filepath;

  GLFWwindow *window{nullptr};
  window = glfwCreateWindow(gCtx.width, gCtx.height, "Simple USDZ GL viewer",
                            nullptr, nullptr);
  glfwMakeContextCurrent(window);
  glfwSwapInterval(1);  // vsync on

  if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
    std::cerr << "Failed to load OpenGL functions with gladLoadGL\n";
    exit(EXIT_FAILURE);
  }

  std::cout << "OpenGL " << GLVersion.major << '.' << GLVersion.minor << '\n';

  if (GLVersion.major < 2) {
    std::cerr << "OpenGL 2. or later should be available." << std::endl;
    exit(EXIT_FAILURE);
  }

#ifdef _DEBUG_OPENGL
  glEnable(GL_DEBUG_OUTPUT);
  glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
  glDebugMessageCallback(reinterpret_cast<GLDEBUGPROC>(glDebugOutput), nullptr);
  glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr,
                        GL_TRUE);
#endif

  GUIContext gui_ctx;

  glfwSetWindowUserPointer(window, &gui_ctx);
  glfwSetKeyCallback(window, key_callback);
  glfwSetCursorPosCallback(window, mouse_move_callback);
  glfwSetMouseButtonCallback(window, mouse_button_callback);
  glfwSetFramebufferSizeCallback(window, resize_callback);

  bool done = false;

  ImGui::CreateContext();

  ImGui::StyleColorsDark();

  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init(glsl_version);

  GLProgramState gl_progs;
  if (!LoadShaders(gl_progs)) {
    exit(-1);
  }

  GLScene gl_scene;
  if (!ProcScene(gl_progs.shaders["default"], stage,
                 /* asset search path */ basedir, &gl_scene)) {
    exit(-1);
  }

  std::cout << "scene bmin: " << gl_scene.bmin[0] << ", " << gl_scene.bmin[1]
            << ", " << gl_scene.bmin[2] << "\n";
  std::cout << "scene bmax: " << gl_scene.bmax[0] << ", " << gl_scene.bmax[1]
            << ", " << gl_scene.bmax[2] << "\n";

  int display_w, display_h;
  ImVec4 clear_color = {0.1f, 0.18f, 0.3f, 1.0f};

  {
    float cxscale, cyscale;
    glfwGetWindowContentScale(window, &cxscale, &cyscale);
    std::cout << "xscale, yscale = " << cxscale << ", " << cyscale << "\n";

    ImGuiIO &io = ImGui::GetIO();

    io.DisplayFramebufferScale = {2.0f, 2.0f};  // HACK

    ImFontConfig font_config;
    font_config.SizePixels = 16.0f * xscale;
    io.Fonts->AddFontDefault(&font_config);
  }

  std::array<float, 3> bmin = {-100.0f, -100.0f, -100.0f};
  std::array<float, 3> bmax = {100.0f, 100.0f, 100.0f};

  float maxExtent = 0.5f * (bmax[0] - bmin[0]);
  if (maxExtent < 0.5f * (bmax[1] - bmin[1])) {
    maxExtent = 0.5f * (bmax[1] - bmin[1]);
  }
  if (maxExtent < 0.5f * (bmax[2] - bmin[2])) {
    maxExtent = 0.5f * (bmax[2] - bmin[2]);
  }

  float eye[3], lookat[3], up[3];

  up[0] = 0.0f;
  up[1] = 1.0f;
  up[2] = 0.0f;

  const example::shader &curr_shader = gl_progs.shaders["default"];

  while (!done) {
    glfwPollEvents();
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGui::Begin("Info");
    ImGui::Text("View control");
    ImGui::Text("ctrl + left mouse");
    ImGui::Text("shift + left mouse");
    ImGui::Text("left mouse");
    ImGui::End();

    ImGui::Begin("Scene");
    ImGui::InputText("filename", const_cast<char *>(gCtx.usd_filepath.c_str()),
                     gCtx.usd_filepath.size(), ImGuiInputTextFlags_ReadOnly);
    ImGui::InputFloat3("scene bmin", &gl_scene.bmin[0], "%.3f",
                       ImGuiInputTextFlags_ReadOnly);
    ImGui::InputFloat3("scene bmax", &gl_scene.bmax[0], "%.3f",
                       ImGuiInputTextFlags_ReadOnly);
    ImGui::End();

    ImGui::Begin("Material");
    
    ImGui::End();

    if (gCtx.selected_surfaceShader) {
      UsdPreviewSurfaceParamUI(*gCtx.selected_surfaceShader);
    }

    ImGui::Begin("RenderScene converter log");
    ImGui::InputTextMultiline("info",
                              const_cast<char *>(gCtx.converter_info.c_str()),
                              gCtx.converter_info.size(), ImVec2(800, 300),
                              ImGuiInputTextFlags_ReadOnly);
    ImGui::InputTextMultiline("warn",
                              const_cast<char *>(gCtx.converter_warn.c_str()),
                              gCtx.converter_warn.size(), ImVec2(800, 300),
                              ImGuiInputTextFlags_ReadOnly);
    ImGui::End();

    // For developers only
    ImGui::Begin("dev");
    {
      static bool compile_ok{true};

      if (ImGui::Button("Reload shader")) {
        compile_ok = ReloadShader(curr_shader.get_program(), gCtx.vert_filename,
                                  gCtx.frag_filename);
      }

      if (compile_ok) {
        ImGui::TextColored(ImVec4(0.3, 1.0, 0.4, 1.0), "Shader Compile OK");
      } else {
        ImGui::TextColored(ImVec4(1.0, 0.2, 0.1, 1.0), "Shader Compile Failed");
      }
      ImGui::End();
    }

    ImGui::Begin("Camera");
    ImGui::SliderFloat("fov", &gCtx.fov, 0.0f, 178.0f);
    ImGui::InputFloat("znear", &gCtx.znear);
    ImGui::InputFloat("zfar", &gCtx.zfar);
    ImGui::InputFloat3("eye", &gCtx.eye[0]);
    ImGui::Separator();
    ImGui::SliderFloat("xrot", &gCtx.xrotate, -89.9f, 89.9f);
    ImGui::SliderFloat("yrot", &gCtx.yrotate, -180.0f, 180.0f);
    ImGui::Separator();
    if (ImGui::Button("Reset rotation")) {
      gCtx.xrotate = 0.0f;
      gCtx.yrotate = 0.0f;
    }
    ImGui::End();

    glfwGetFramebufferSize(window, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
    glEnable(GL_DEPTH_TEST);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    float aspect = float(display_w) / float(display_h);
    // view
    gCtx.camera.setPosition(gCtx.eye);
    gCtx.camera.setRotation({gCtx.xrotate, gCtx.yrotate, 0.0f});
    gCtx.camera.setPerspective(gCtx.fov, aspect, gCtx.znear, gCtx.zfar);

    ImGui::Begin("View matrix");
    {
      tinyusdz::value::matrix4f view;
      ConvertMatrix(gCtx.camera.matrices.view, view);
      tinyusdz::value::matrix4f proj;
      ConvertMatrix(gCtx.camera.matrices.perspective, proj);

      // example::mat4 viewproj = linalg::mul(gCtx.camera.matrices.perspective,
      // gCtx.camera.matrices.view);
      tinyusdz::value::matrix4f viewproj = view * proj;

      ImMatrix4Display("view", &gCtx.camera.matrices.view[0][0]);
      ImGui::Separator();
      ImMatrix4Display("perspective", &gCtx.camera.matrices.perspective[0][0]);
      ImGui::Separator();
      // ImMatrix4Display("viewproj", &viewproj.x[0]);
      ImMatrix4Display("viewproj", &viewproj.m[0][0]);

      ImGui::End();
    }

    DrawScene(curr_shader, gl_scene);

#if 0
    // Draw scene
    if ((scene.default_root_node >= 0) && (scene.default_root_node < scene.nodes.size())) {
      DrawNode(scene, scene.nodes[scene.default_root_node]);
    } else {
      static bool printed = false;
      if (!printed) {
        std::cerr << "USD scene does not contain root node. or has invalid root node ID\n";
        std::cerr << "  # of nodes in the scene: " << scene.nodes.size() << "\n";
        std::cerr << "  scene.default_root_node: " << scene.default_root_node << "\n";
        printed = true;
      }
    }
#endif

    // Imgui

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    // glFlush();
    glfwSwapBuffers(window);

    static int frameCount = 0;
    static double currentTime = glfwGetTime();
    static double previousTime = currentTime;
    static char title[256];

    frameCount++;
    currentTime = glfwGetTime();
    const auto deltaTime = currentTime - previousTime;
    if (deltaTime >= 1.0) {
      sprintf(title, "Simple GL USDC/USDA/USDZ viewer [%dFPS]", frameCount);
      glfwSetWindowTitle(window, title);
      frameCount = 0;
      previousTime = currentTime;
    }

    done = glfwWindowShouldClose(window);
  };

  std::cout << "Close window\n";

  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();

  glfwDestroyWindow(window);
  glfwTerminate();

  return EXIT_SUCCESS;
}
