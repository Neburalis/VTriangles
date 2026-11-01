#pragma once

#include <iostream>
#include <string>

#include <glad/glad.h>

#include "stb_image.h"

// Обёртка над stb_image. Сейчас в сцене не используется, но оставлена как готовая утилита.
class Textures {
    inline static int NextTextureUnit_ = 0;

public:
    int NumOfTextures_ = 0;
    unsigned int id_ = 0;

    Textures() = default;

    void InitTexture(std::string TexturePath) {
        int width = 0, height = 0, nChannels = 0;
        unsigned char* data = stbi_load(TexturePath.c_str(), &width, &height, &nChannels, STBI_rgb_alpha);
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

        // Параметры: текущая 2D-текстура, уровень mipmap, внутренний формат,
        // ширина, высота, граница, формат данных, тип данных, данные.
        glTexImage2D(GL_TEXTURE_2D,        0,      GL_RGBA8,    width, height,     0,     GL_RGBA,   GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        stbi_image_free(data);
    }
};
