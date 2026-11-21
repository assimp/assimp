// ----------------------------------------------------------------------------
// Skeletal Animation Sample using Assimp, freeglut and OpenGL 3.3 core.
//
// License: Released into the public domain.
//
// Written by Aravinda Samarathunga (I-A-S)
// ----------------------------------------------------------------------------

#include "app.hpp"
#include "model_loader.hpp"

struct ShaderHandle
{
    GLuint handle{};

    ShaderHandle(GLuint h = 0): handle(h)
    {
    }

    ShaderHandle(const ShaderHandle &) = delete;
    ShaderHandle &operator=(const ShaderHandle &) = delete;

    ~ShaderHandle()
    {
        if (handle)
            glDeleteShader(handle);
    }
};

static Result<> compile_shader_from_file(const std::string& path, GLenum type, ShaderHandle& handle)
{
    std::string source;
    
    std::ifstream file(path, std::ios::in | std::ios::binary);
    
    if (!file)
      return "failed to read the shader from file \"" + path + "\"";
    
    {
        std::stringstream ss;
        ss << file.rdbuf();
        source = ss.str();
    }

    GLuint shader = glCreateShader(type);

    const char* src = source.c_str();
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    GLint ok = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (ok != GL_TRUE)
    {
      GLint log_length = 0;
      glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_length);
      std::vector<char> log(static_cast<size_t>(log_length > 1 ? log_length : 1), '\0');
      glGetShaderInfoLog(shader, static_cast<GLsizei>(log.size()), nullptr, log.data());
      glDeleteShader(shader);
      return "failed to compile the shader \"" + path + "\": " + std::string(log.data());
    }

    handle.handle = shader;
    
    return Ok();
}

Result<Box<App>> App::create(
    int32_t width, int32_t height,
    const std::string& model_path,
    const std::string& vertex_shader_path,
    const std::string& fragment_shader_path)
{
    Box<App> app(new App(width, height));

    CHECK_RESULT(app->init_pipeline(vertex_shader_path, fragment_shader_path));
    CHECK_RESULT(app->init_graphics(model_path));

    return app;
}

App::~App()
{
    if (m_tex_sampler_state)
        glDeleteSamplers(1, &m_tex_sampler_state);

    if (m_ubo_buffer)
        glDeleteBuffers(1, &m_ubo_buffer);

    if (m_program)
        glDeleteProgram(m_program);
}

Result<> App::init_pipeline(const std::string& vertex_shader_path,
    const std::string& fragment_shader_path)
{
    {
        ShaderHandle vs, fs;
        CHECK_RESULT(compile_shader_from_file(vertex_shader_path, GL_VERTEX_SHADER, vs));
        CHECK_RESULT(compile_shader_from_file(fragment_shader_path, GL_FRAGMENT_SHADER, fs));

        m_program = glCreateProgram();
        glAttachShader(m_program, vs.handle);
        glAttachShader(m_program, fs.handle);
        glLinkProgram(m_program);
    }

    GLint ok = GL_FALSE;
    glGetProgramiv(m_program, GL_LINK_STATUS, &ok);
    if (ok != GL_TRUE)
    {
      GLint log_length = 0;
      glGetProgramiv(m_program, GL_INFO_LOG_LENGTH, &log_length);
      std::vector<char> log(static_cast<size_t>(log_length > 1 ? log_length : 1), '\0');
      glGetProgramInfoLog(m_program, static_cast<GLsizei>(log.size()), nullptr, log.data());
      return "failed to link shader program: \"" + std::string(log.data()) + "\"";
    }

    const GLuint block_index = glGetUniformBlockIndex(m_program, "UBO");
    if (block_index == GL_INVALID_INDEX)
      return "'UBO' uniform block not found in the vertex shader";
    glUniformBlockBinding(m_program, block_index, UBO_BINDING);

    glUseProgram(m_program);
    glUniform1i(glGetUniformLocation(m_program, "diffuse_texture"), DIFFUSE_TEXTURE_UNIT);
    glUseProgram(0);

    return Ok();
}

Result<> App::init_graphics(const std::string& model_path)
{
    m_projection = perspective_fov_rh(PI / 4.0f, m_width / static_cast<float>(m_height), 0.01f, 1000.0f);

    glGenBuffers(1, &m_ubo_buffer);
    glBindBuffer(GL_UNIFORM_BUFFER, m_ubo_buffer);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(UBO), nullptr, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    glGenSamplers(1, &m_tex_sampler_state);
    glSamplerParameteri(m_tex_sampler_state, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glSamplerParameteri(m_tex_sampler_state, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glSamplerParameteri(m_tex_sampler_state, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glSamplerParameteri(m_tex_sampler_state, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glSamplerParameteri(m_tex_sampler_state, GL_TEXTURE_WRAP_R, GL_REPEAT);

    if (glGetError() != GL_NO_ERROR)
      return "failed to create ubo buffer or sampler state";

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    const Vec3 eye(0.0f, 2.5f, 8.0f);
    const Vec3 at(0.0f, 1.9f, 0.0f);
    const Vec3 up(0.0f, 1.0f, 0.0f);
    m_view = look_at_rh(eye, at, up);

    CHECK_ASSIGN_RESULT(
      ModelLoader::from_file(model_path),
      m_model
    );

    return Ok();
}

void App::on_render()
{
    const int32_t time_cur = glutGet(GLUT_ELAPSED_TIME);
    if (m_time_start < 0)
      m_time_start = time_cur;
    const float t = (time_cur - m_time_start) / 1000.0f;

    glClearColor(100.0f / 255.0f, 149.0f / 255.0f, 237.0f / 255.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    Mat4::RotationY(-t, m_world);

    UBO ubo;
    ubo.world = transposed(m_world);
    ubo.view = transposed(m_view);
    ubo.projection = transposed(m_projection);
    glBindBuffer(GL_UNIFORM_BUFFER, m_ubo_buffer);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(ubo), &ubo);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    glUseProgram(m_program);
    glBindBufferBase(GL_UNIFORM_BUFFER, UBO_BINDING, m_ubo_buffer);
    glBindSampler(DIFFUSE_TEXTURE_UNIT, m_tex_sampler_state);

    if (m_model)
      m_model->draw();
}

void App::on_resize(int32_t width, int32_t height)
{
    m_width = width > 0 ? width : 1;
    m_height = height > 0 ? height : 1;
    glViewport(0, 0, m_width, m_height);
    m_projection = perspective_fov_rh(PI / 4.0f, m_width / static_cast<float>(m_height), 0.01f, 1000.0f);
}
