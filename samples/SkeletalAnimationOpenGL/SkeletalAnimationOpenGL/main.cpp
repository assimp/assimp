// ----------------------------------------------------------------------------
// Skeletal Animation Sample using Assimp, freeglut and OpenGL 3.3 core.
//
// License: Released into the public domain.
//
// Written by Aravinda Samarathunga (I-A-S)
// ----------------------------------------------------------------------------

#include <glad/glad.h>

#include <GL/freeglut.h>

#include <cstdio>
#include <stdint.h>

#include <string>
#include <filesystem>

static constexpr int32_t WINDOW_WIDTH = 800;
static constexpr int32_t WINDOW_HEIGHT = 600;

void on_display()
{
    glClearColor(100.0f / 255.0f, 149.0f / 255.0f, 237.0f / 255.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glutSwapBuffers();
}

void on_reshape(int width, int height)
{
    glViewport(0, 0, width, height);
}

void on_keyboard(unsigned char key, int, int)
{
    if (key == 27) { // Escape
        glutLeaveMainLoop();
    }
}

int main(int argc, char **argv)
{
    std::string res_dir_path = "./res";
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
        std::printf("[ERROR]: failed to find the 'res' dir.. try running '%s <path_to_res>'\n", argv[0]);
        return -1;
    }

    glutInit(&argc, argv);

    glutInitContextVersion(3, 3);
    glutInitContextProfile(GLUT_CORE_PROFILE);
    glutInitContextFlags(GLUT_FORWARD_COMPATIBLE);

    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
    glutInitWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);
    glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_GLUTMAINLOOP_RETURNS);

    glutCreateWindow("Assimp - Skeletal Animation OpenGL sample");

    if (!gladLoadGL()) {
        std::fprintf(stderr, "Failed to load OpenGL functions with glad.\n");
        return 1;
    }

    std::printf("OpenGL version: %s\n", reinterpret_cast<const char *>(glGetString(GL_VERSION)));
    std::printf("Renderer:       %s\n", reinterpret_cast<const char *>(glGetString(GL_RENDERER)));

    glutDisplayFunc(on_display);
    glutReshapeFunc(on_reshape);
    glutKeyboardFunc(on_keyboard);

    glutMainLoop();

    return 0;
}
