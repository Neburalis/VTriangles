#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <stdexcept>
#endif

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <string>
#include <iostream>
#include <sstream>
#include <fstream>


class Models {
    std::vector<float> vertices_;
    public:
        Models (const std::string &path) {
            std::ifstream ModelFile(path);
            if (!ModelFile.is_open())
                throw std::runtime_error("ERROR: не удалось открыть модель <" + path + ">\n");

            
            std::string line;
            while (std::getline(ModelFile, line)) {
                size_t commentPos = line.find('/');
                if (commentPos != std::string::npos) {
                    line.erase(commentPos);
                }
                
                std::stringstream lineStream(line);
                float Data;
                while (lineStream >> Data) {
                    vertices_.push_back(Data);
                }
            }

            ModelFile.close();
        }

        const std::vector<float> &data() const {
            return vertices_;
        }
};