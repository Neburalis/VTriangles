#include <cstddef>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <vector>

#include "mesh.hpp"

int main() {
    try {
        const std::vector<Triangle> triangles{
            Triangle{glm::vec3{0.0f, 0.0f, 0.0f},
                     glm::vec3{1.0f, 0.0f, 0.0f},
                     glm::vec3{0.0f, 1.0f, 0.0f}},
            Triangle{glm::vec3{2.0f, 0.0f, 0.0f},
                     glm::vec3{3.0f, 0.0f, 0.0f},
                     glm::vec3{2.0f, 1.0f, 0.0f}},
        };
        const std::vector<bool> intersects{false, true};
        const std::vector<float> vertices = BuildTriangleVertices(triangles, intersects);

        constexpr std::size_t stride = 7;
        if (vertices.size() != 2 * 3 * stride)
            throw std::runtime_error("Ожидалось 7 float на каждую из 6 вершин");

        for (std::size_t face = 0; face < triangles.size(); ++face) {
            for (std::size_t vertex = 0; vertex < 3; ++vertex) {
                const std::size_t offset = (face * 3 + vertex) * stride;
                const glm::vec3& point = triangles[face][vertex];
                if (vertices[offset] != point.x
                    || vertices[offset + 1] != point.y
                    || vertices[offset + 2] != point.z)
                    throw std::runtime_error("Неверная позиция в формате вершины");
                if (vertices[offset + 3] != 0.0f
                    || vertices[offset + 4] != 0.0f
                    || vertices[offset + 5] != 1.0f)
                    throw std::runtime_error("Неверная нормаль в формате вершины");
                const float expectedFlag = face == 0 ? 0.0f : 1.0f;
                if (vertices[offset + 6] != expectedFlag)
                    throw std::runtime_error("Флаг должен повторяться у всех трёх вершин треугольника");
            }
        }

        // Проверяем как слишком короткую, так и слишком длинную маску.
        for (const std::vector<bool>& invalidMask : {std::vector<bool>{false},
                                                    std::vector<bool>{false, true, false}}) {
            bool rejected = false;
            try {
                (void)BuildTriangleVertices(triangles, invalidMask);
            } catch (const std::invalid_argument&) {
                rejected = true;
            }
            if (!rejected)
                throw std::runtime_error("Несовпадение размеров должно вызывать std::invalid_argument");
        }
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
