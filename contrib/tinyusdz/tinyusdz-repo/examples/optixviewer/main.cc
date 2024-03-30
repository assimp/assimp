// glad must be included before glfw3.h
#include <glad/glad.h>
//
#include <GLFW/glfw3.h>

#include <atomic>  // C++11
#include <cassert>
#include <chrono>  // C++11
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <mutex>   // C++11
#include <thread>  // C++11
#include <algorithm>

// common
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl2.h"
#include "tinyusdz.hh"
#include "trackball.h"
#include "cuew/cuew.h"

#define OPTIX_DONT_INCLUDE_CUDA
#include <optix.h>

#define CU_CHECK(cond)                                                 \
  do {                                                                 \
    CUresult ret = cond;                                               \
    if (ret != CUDA_SUCCESS) {                                         \
      std::cerr << __FILE__ << ":" << __LINE__                         \
                << " CUDA Device API failed. retcode " << ret << "\n"; \
      exit(-1);                                                        \
    }                                                                  \
  } while (0)

#define CU_SYNC_CHECK()                                                   \
  do {                                                                    \
    /* assume cuCtxSynchronize() ~= cudaDeviceSynchronize() */            \
    CUresult ret = cuCtxSynchronize();                                    \
    if (ret != CUDA_SUCCESS) {                                            \
      std::cerr << __FILE__ << ":" << __LINE__                            \
                << " cuCtxSynchronize() failed. retcode " << ret << "\n"; \
      exit(-1);                                                           \
    }                                                                     \
  } while (0)

#define OPTIX_CHECK(callfun)                                                \
  do {                                                                      \
    OptixResult ret = callfun;                                              \
    if (ret != OPTIX_SUCCESS) {                                             \
      std::cerr << __FILE__ << ":" << __LINE__ << " Optix call" << #callfun \
                << " failed. retcode " << ret << "\n";                      \
      exit(-1);                                                             \
    }                                                                       \
  } while (0)


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

  std::array<float, 3> eye = {0.0f, 0.0f, 5.0f};
  std::array<float, 3> lookat = {0.0f, 0.0f, 0.0f};
  std::array<float, 3> up = {0.0f, 1.0f, 0.0f};
};

GUIContext gCtx;

// --- glfw ----------------------------------------------------

static void error_callback(int error, const char* description) {
  std::cerr << "GLFW Error : " << error << ", " << description << std::endl;
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action,
                         int mods) {
  (void)scancode;

  ImGuiIO& io = ImGui::GetIO();
  if (io.WantCaptureKeyboard) {
    return;
  }

  if ((key == GLFW_KEY_LEFT_SHIFT) || (key == GLFW_KEY_RIGHT_SHIFT)) {
    auto* param =
        reinterpret_cast<GUIContext*>(glfwGetWindowUserPointer(window));

    param->shift_pressed = (action == GLFW_PRESS);
  }

  if ((key == GLFW_KEY_LEFT_CONTROL) || (key == GLFW_KEY_RIGHT_CONTROL)) {
    auto* param =
        reinterpret_cast<GUIContext*>(glfwGetWindowUserPointer(window));

    param->ctrl_pressed = (action == GLFW_PRESS);
  }

  if (key == GLFW_KEY_Q && action == GLFW_PRESS && (mods & GLFW_MOD_CONTROL)) {
    glfwSetWindowShouldClose(window, GLFW_TRUE);
  }
}

static void mouse_move_callback(GLFWwindow* window, double x, double y) {
  auto param = reinterpret_cast<GUIContext*>(glfwGetWindowUserPointer(window));
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
      // Adjust y.
      trackball(param->prev_quat,
                (2.f * (param->mouse_x - x_offset) - w) / static_cast<float>(w),
                (h - 2.f * (param->mouse_y - y_offset)) / static_cast<float>(h),
                (2.f * (static_cast<float>(x) - x_offset) - w) /
                    static_cast<float>(w),
                (h - 2.f * (static_cast<float>(y) - y_offset)) /
                    static_cast<float>(h));
      add_quats(param->prev_quat, param->curr_quat, param->curr_quat);
    }
  }

  param->mouse_x = static_cast<int>(x);
  param->mouse_y = static_cast<int>(y);
}

static void mouse_button_callback(GLFWwindow* window, int button, int action,
                                  int mods) {
  ImGuiIO& io = ImGui::GetIO();
  if (io.WantCaptureMouse || io.WantCaptureKeyboard) {
    return;
  }

  auto param = reinterpret_cast<GUIContext*>(glfwGetWindowUserPointer(window));
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

static void resize_callback(GLFWwindow* window, int width, int height) {
  auto param = reinterpret_cast<GUIContext*>(glfwGetWindowUserPointer(window));
  assert(param);

  param->width = width;
  param->height = height;
}

// ------------------------------------------------

namespace {

#if 0
static void DrawGeomMesh(tinyusdz::GeomMesh& mesh) {}

static void DrawNode(const tinyusdz::Scene& scene, const tinyusdz::Node& node) {
  if (node.type == tinyusdz::NODE_TYPE_XFORM) {
    const tinyusdz::Xform& xform = scene.xforms.at(node.index);
    glPushMatrix();

    tinyusdz::Matrix4d matrix = xform.GetMatrix();
    glMultMatrixd(reinterpret_cast<const double*>(&(matrix.m)));
  }

  for (const auto& child : node.children) {
    if ((child.index >= 0) && (child.index < scene.nodes.size())) {
      DrawNode(scene, scene.nodes.at(size_t(child.index)));
    }
  }

  if (node.type == tinyusdz::NODE_TYPE_XFORM) {
    glPopMatrix();
  }
}

static void Proc(const tinyusdz::Scene &scene)
{
  std::cout << "num geom_meshes = " << scene.geom_meshes.size();

  for (auto &mesh : scene.geom_meshes) {
  }
}
#endif


} // namespace

int main(int argc, char** argv) {

  if (cuewInit(CUEW_INIT_CUDA) != CUEW_SUCCESS) {
    std::cerr << "Failed to initialize CUDA\n";
    return -1;
  }

  printf("CUDA compiler path: %s, compiler version: %d\n", cuewCompilerPath(),
         cuewCompilerVersion());

  // Currently we require NVRTC to be available for runtime .cu compilation.
  if (cuewInit(CUEW_INIT_NVRTC) != CUEW_SUCCESS) {
    std::cerr << "Failed to initialize NVRTC. NVRTC library is not available "
                 "or not found in the system search path\n";
    return -1;
  } else {
    int major, minor;
    nvrtcVersion(&major, &minor);
    std::cout << "Found NVRTC runtime compilation library version " << major
              << "." << minor << "\n";
  }

  //
  // Initialize CUDA and create OptiX context
  //

  if (cuInit(0) != CUDA_SUCCESS) {
    std::cerr << "Failed to init CUDA\n";
    return -1;
  }

  OptixDeviceContext context = nullptr;
  CUcontext cuCtx{};
  {
    int counts{0};
    CU_CHECK(cuDeviceGetCount(&counts));

    std::cout << "# of CUDA devices: " << counts << "\n";
    if (counts < 1) {
      std::cerr << "No CUDA device found\n";
      return -1;
    }

    CUdevice device{};
    if (CUDA_SUCCESS != cuDeviceGet(&device, /* devid */ 0)) {
      std::cerr << "Failed to get CUDA device.\n";
      return -1;
    }

    {
      int major, minor;
      CU_CHECK(cuDeviceComputeCapability(&major, &minor, device));
      std::cerr << "compute capability: " << major << "." << minor << "\n";
    }

    if (CUDA_SUCCESS != cuCtxCreate(&cuCtx, /* flags */ 0, device)) {
      std::cerr << "Failed to get Create CUDA context.\n";
      return -1;
    }
  }


  // ======================================


  // Setup window
  glfwSetErrorCallback(error_callback);
  if (!glfwInit()) {
    exit(EXIT_FAILURE);
  }

  std::string filename = "../../../models/suzanne.usdc";

  if (argc > 1) {
    filename = std::string(argv[1]);
  }

  std::cout << "Loading file " << filename << "\n";

  std::string warn;
  std::string err;
  tinyusdz::Stage stage;

  bool ret = tinyusdz::LoadUSDFromFile(filename, &stage, &warn, &err);

  if (!warn.empty()) {
    std::cerr << "WARN : " << warn << "\n";
    return EXIT_FAILURE;
  }
  if (!err.empty()) {
    std::cerr << "ERR : " << err << "\n";
    return EXIT_FAILURE;
  }

  if (!ret) {
    std::cerr << "Failed to load USD file: " << filename << "\n";
    return EXIT_FAILURE;
  }


  //Proc(scene);

#ifdef _DEBUG_OPENGL
  glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
#endif

  //// Create a GLES 3.0 context
  // glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
  // glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  // glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  // glfwWindowHint(GLFW_SAMPLES, 16);
  // glfwWindowHint(GLFW_DOUBLEBUFFER, GL_TRUE);

#ifdef __APPLE__
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT,
                 GL_TRUE);  // It looks this is important on macOS.
#endif

  GLFWwindow* window{nullptr};
  window = glfwCreateWindow(gCtx.width, gCtx.height, "Simple USDZ GL viewer",
                            nullptr, nullptr);
  glfwMakeContextCurrent(window);

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

  glfwSwapInterval(1);  // Enable vsync

  GUIContext gui_ctx;

  glfwSetWindowUserPointer(window, &gui_ctx);
  glfwSetKeyCallback(window, key_callback);
  glfwSetCursorPosCallback(window, mouse_move_callback);
  glfwSetMouseButtonCallback(window, mouse_button_callback);
  glfwSetFramebufferSizeCallback(window, resize_callback);

  bool done = false;

  ImGui::CreateContext();

  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL2_Init();

  int display_w, display_h;
  ImVec4 clear_color = {0.1f, 0.18f, 0.3f, 1.0f};

  while (!done) {
    glfwPollEvents();
    ImGui_ImplOpenGL2_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGui::Begin("Bora");
    ImGui::Button("muda");
    ImGui::End();

    glfwGetFramebufferSize(window, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
    glEnable(GL_DEPTH_TEST);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

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
    ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());

    glfwSwapBuffers(window);
    glFlush();

    static int frameCount = 0;
    static double currentTime = glfwGetTime();
    static double previousTime = currentTime;
    static char title[256];

    frameCount++;
    currentTime = glfwGetTime();
    const auto deltaTime = currentTime - previousTime;
    if (deltaTime >= 1.0) {
      sprintf(title, "Simple GL USDZ viewer [%dFPS]", frameCount);
      glfwSetWindowTitle(window, title);
      frameCount = 0;
      previousTime = currentTime;
    }

    glfwSwapBuffers(window);

    done = glfwWindowShouldClose(window);
  };

  ImGui_ImplOpenGL2_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();

  glfwDestroyWindow(window);
  glfwTerminate();

  return EXIT_SUCCESS;
}
