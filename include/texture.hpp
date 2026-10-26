#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <string>
#include <iostream>

class Textures {
    inline static int NextTextureUnit_ = 0;

    public:
        int NumOfTextures_ = 0;
        unsigned int id_ = 0;

        Textures() = default;

        void InitTexture(std::string TexturePath) {
            int width, height, nChannels;
            unsigned char *data = stbi_load(TexturePath.c_str(), &width, &height, &nChannels, STBI_rgb_alpha);  
            if (!data) {
                std::cerr << "ERROR: Failed to load image <" << TexturePath << ">\n";
                stbi_image_free(data);
                return;
            }

            glGenTextures(1, &id_);

            NumOfTextures_ = NextTextureUnit_++;
            glActiveTexture(GL_TEXTURE0 + NumOfTextures_);
            glBindTexture(GL_TEXTURE_2D, id_);
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

            // PARAMETERS:
            //        CurBound2DTexture, mipmap level, StoreFormat, width, height, always 0, LoadFormat, LoadSYmbolsType,  data
            glTexImage2D(GL_TEXTURE_2D,        0,      GL_RGBA8,    width, height,     0,     GL_RGBA,   GL_UNSIGNED_BYTE, data);
            glGenerateMipmap(GL_TEXTURE_2D);

            stbi_image_free(data);
        }
};
