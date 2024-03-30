// SPDX-License-Identifier: MIT
#include <android/log.h>

#include "render-ctx.hh"

namespace example {

GUIContext g_gui_ctx;


namespace {

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
    val = std::max(0, std::min(255, val));

    return static_cast<uint8_t>(val);
}

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

} // namespace

bool SetupScene(GUIContext &ctx) {

    __android_log_print(ANDROID_LOG_INFO, "tinyusdz", "SetupScene");
#if 0 // TODO
    if (ctx.stage.root_nodes.empty()) {
        __android_log_print(ANDROID_LOG_ERROR, "tinyusdz", "No GeomMesh");
        // No GeomMesh in the scene
        return false;
    }

    // Convert USD geom_mesh to renderable mesh.
    ctx.render_scene.draw_meshes.clear();
#if 0 // TODO
    __android_log_print(ANDROID_LOG_INFO, "tinyusdz", "# of geom_meshes %d", (int)ctx.scene.geom_meshes.size());
    for (size_t i = 0; i < ctx.scene.geom_meshes.size(); i++) {
        example::DrawGeomMesh draw_mesh(&ctx.scene.geom_meshes[i]);
        ctx.render_scene.draw_meshes.push_back(draw_mesh);
    }
#endif

    // Setup render mesh
    if (!ctx.render_scene.Setup()) {
        __android_log_print(ANDROID_LOG_ERROR, "tinyusdz", "Scene::Setup failed");
       return false;
    }
#endif

    // init camera matrix
    {
        auto q = ToQuaternion(radians(ctx.yaw), radians(ctx.pitch),
                              radians(ctx.roll));
        ctx.camera.quat[0] = q[0];
        ctx.camera.quat[1] = q[1];
        ctx.camera.quat[2] = q[2];
        ctx.camera.quat[3] = q[3];
    }

    // HACK. camera position adjusted for `suzanne.usdc`
    ctx.camera.eye[2] = 3.5f;

    return true;
}

bool RenderScene(GUIContext &ctx)
{

    // init camera matrix
    {
        auto q = ToQuaternion(radians(ctx.yaw), radians(ctx.pitch),
                              radians(ctx.roll));
        ctx.camera.quat[0] = q[0];
        ctx.camera.quat[1] = q[1];
        ctx.camera.quat[2] = q[2];
        ctx.camera.quat[3] = q[3];
    }

    // Init AOV image
    ctx.aov.Resize(ctx.render_width, ctx.render_height);

    example::Render(ctx.render_scene, ctx.camera, &ctx.aov);

  return true;
}

void GetRenderedImage(const GUIContext &ctx, std::vector<uint32_t> *argb) {
    if (!argb) {
        return;
    }

    std::vector<uint32_t> buf;
    buf.resize(ctx.aov.width * ctx.aov.height);

    for (size_t i = 0; i < ctx.aov.width * ctx.aov.height; i++) {
        uint8_t r = ftouc(linearToSrgb(ctx.aov.rgb[3 * i + 0]));
        uint8_t g = ftouc(linearToSrgb(ctx.aov.rgb[3 * i + 1]));
        uint8_t b = ftouc(linearToSrgb(ctx.aov.rgb[3 * i + 2]));
        uint8_t a = 255;

        buf[i] = (a << 24) | (r << 16) | (g << 8) | b;
    }

    (*argb) = buf;
}
} // example
