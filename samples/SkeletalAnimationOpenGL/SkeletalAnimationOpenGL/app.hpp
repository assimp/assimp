// ----------------------------------------------------------------------------
// Skeletal Animation Sample using Assimp, freeglut and OpenGL 3.3 core.
//
// License: Released into the public domain.
//
// Written by Aravinda Samarathunga (I-A-S)
// ----------------------------------------------------------------------------

#pragma once

#include "model.hpp"

class App
{
    static constexpr GLuint UBO_BINDING = 0;
    static constexpr GLuint BONE_UBO_BINDING = 1;
    static constexpr GLint DIFFUSE_TEXTURE_UNIT = 0;

    struct UBO
    {
      Mat4 world;
      Mat4 view;
      Mat4 projection;
    };

    struct BoneUBO
    {
        Mat4 bones[Model::MAX_JOINT_COUNT];
    };

public:
    static Result<Box<App>> create(
        int32_t width, int32_t height,
        const std::string& model_path,
        const std::string& vertex_shader_path,
        const std::string& fragment_shader_path);

    App(const App &) = delete;
    App &operator=(const App &) = delete;

    ~App();

    void on_render();

    void on_resize(int32_t width, int32_t height);

private:
    App(int32_t width, int32_t height): m_width(width), m_height(height) {}

private:
    Result<> init_pipeline(const std::string& vertex_shader_path, const std::string& fragment_shader_path);
    Result<> init_graphics(const std::string& model_path);

private:
    GLuint m_program{};
    GLuint m_ubo_buffer{};
    GLuint m_bone_ubo_buffer{};
    GLuint m_tex_sampler_state{};

    Mat4 m_world{};
    Mat4 m_view{};
    Mat4 m_projection{};

    int32_t m_width{1};
    int32_t m_height{1};
    int32_t m_time_prev_frame{-1};

    Box<Model> m_model;

    UBO m_ubo_data{};
    BoneUBO m_bone_ubo_data{};
};
