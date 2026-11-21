// ----------------------------------------------------------------------------
// Skeletal Animation Sample using Assimp, freeglut and OpenGL 3.3 core.
//
// License: Released into the public domain.
//
// Written by Aravinda Samarathunga (I-A-S)
// ----------------------------------------------------------------------------

#include "texture_loader.hpp"

#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION
#include "contrib/stb/stb_image.h"

namespace TextureLoader
{
    Result<GLuint> from_pixels(const void *pixels, GLsizei width, GLsizei height, GLenum format)
    {
        GLuint texture = 0;

        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);

        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, format, GL_UNSIGNED_BYTE, pixels);
        glGenerateMipmap(GL_TEXTURE_2D);

        glBindTexture(GL_TEXTURE_2D, 0);

        if (glGetError() != GL_NO_ERROR) {
            glDeleteTextures(1, &texture);
            return "failed to create the texture from pixels";
        }

        return texture;
    }

    Result<GLuint> from_memory(const uint8_t *data, size_t size)
    {
        if (data == nullptr || size == 0 || size > static_cast<size_t>(INT_MAX))
            return "invalid arguments";

        stbi_set_flip_vertically_on_load(1);

        int w = 0, h = 0, n = 0;
        const auto pixels = stbi_load_from_memory(data, static_cast<int>(size), &w, &h, &n, STBI_rgb_alpha);
        if (pixels == nullptr)
            return "failed to load the image";

        GLuint texture{};
        CHECK_ASSIGN_RESULT(
            from_pixels(pixels, w, h, GL_RGBA),
            texture
        );

        stbi_image_free(pixels);

        return texture;
    }

    Result<GLuint> from_file(const std::string& file_name)
    {
        stbi_set_flip_vertically_on_load(1);
        
        int w = 0, h = 0, n = 0;
        const auto pixels = stbi_load(file_name.c_str(), &w, &h, &n, STBI_rgb_alpha);
        if (pixels == nullptr)
            return "failed to load the image";

        GLuint texture;
        CHECK_ASSIGN_RESULT(
            from_pixels(pixels, w, h, GL_RGBA),
            texture
        );

        stbi_image_free(pixels);

        return texture;
    }
}
