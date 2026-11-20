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
