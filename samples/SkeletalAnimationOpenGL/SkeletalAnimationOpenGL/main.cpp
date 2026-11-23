// ----------------------------------------------------------------------------
// Skeletal Animation Sample using Assimp, freeglut and OpenGL 3.3 core.
//
// License: Released into the public domain.
//
// Written by Aravinda Samarathunga (I-A-S)
// ----------------------------------------------------------------------------

#include "app.hpp"

#include <GL/freeglut.h>

#define SAMPLE_GL_LOADER_IMPLEMENTATION
#include "gl_loader.hpp"

#include <cstdio>

#include <filesystem>

Box<App> g_app;

int main(int argc, char **argv)
{
    static constexpr int32_t WINDOW_WIDTH = 800;
    static constexpr int32_t WINDOW_HEIGHT = 600;

    std::string res_dir_path = SAMPLE_RES_DIR;
    if (argc > 1) {
        res_dir_path = argv[1];
    }
    const std::string model_path = res_dir_path + "/models/character.glb";
    const std::string vertex_shader_path = res_dir_path + "/shaders/skinned.vert";
    const std::string fragment_shader_path = res_dir_path + "/shaders/skinned.frag";
    if (!std::filesystem::is_directory(res_dir_path) ||
        !std::filesystem::exists(model_path) ||
        !std::filesystem::exists(vertex_shader_path) ||
        !std::filesystem::exists(fragment_shader_path)
    ) {
        std::fprintf(stderr, "[ERROR]: failed to find the 'res' dir.. try running '%s <path_to_res>'\n", argv[0]);
        return 1;
    }

    glutInit(&argc, argv);

    glutInitContextVersion(3, 3);
    glutInitContextProfile(GLUT_CORE_PROFILE);
    glutInitContextFlags(GLUT_FORWARD_COMPATIBLE);

    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
    glutInitWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);
    glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_GLUTMAINLOOP_RETURNS);

    glutCreateWindow("Assimp - Skeletal Animation OpenGL sample");

    {
        const auto result = load_gl_functions();
        if (result.is_error())
        {
            std::fprintf(stderr, "[ERROR]: \"%s\".\n", result.get_error().c_str());
            return 1;
        }
    }

    while (glGetError() != GL_NO_ERROR) {}

    std::printf("OpenGL version: %s\n", reinterpret_cast<const char *>(glGetString(GL_VERSION)));
    std::printf("Renderer:       %s\n", reinterpret_cast<const char *>(glGetString(GL_RENDERER)));

    {
        auto result = App::create(
            glutGet(GLUT_WINDOW_WIDTH),
            glutGet(GLUT_WINDOW_HEIGHT),
            model_path,
            vertex_shader_path,
            fragment_shader_path
        );
        if (result.is_error())
        {
            std::fprintf(stderr, "[ERROR]: failed to initialize the app: \"%s\".\n", result.get_error().c_str());
            return 1;
        }
        g_app = std::move(result).take_value();
    }

    glutDisplayFunc([]()
    {
        g_app->on_render();
        glutSwapBuffers();
    });

    glutReshapeFunc([](int width, int height)
    {
        g_app->on_resize(width, height);
    });

    glutKeyboardFunc([](unsigned char key, int, int)
    {
        if (key == 27) // Escape 
        {
            glutLeaveMainLoop();
        }
    });

    glutIdleFunc([]()
    {
        glutPostRedisplay();
    });
    
    glutCloseFunc([]()
    {
        g_app.reset();
    });

    glutMainLoop();

    g_app.reset();

    return 0;
}
