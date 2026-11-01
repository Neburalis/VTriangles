#pragma once

#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

// Загружает простую текстовую модель: числа из файла складываются в один массив.
class Models {
    std::vector<float> vertices_;

public:
    explicit Models(const std::string& path) {
        std::ifstream ModelFile(path);
        if (!ModelFile.is_open())
            throw std::runtime_error("ERROR: не удалось открыть модель <" + path + ">\n");

        std::string line;
        while (std::getline(ModelFile, line)) {
            size_t commentPos = line.find('/');
            if (commentPos != std::string::npos)
                line.erase(commentPos);

            std::stringstream lineStream(line);
            float Data;
            while (lineStream >> Data)
                vertices_.push_back(Data);
        }

        ModelFile.close();
    }

    const std::vector<float>& data() const {
        return vertices_;
    }
};
