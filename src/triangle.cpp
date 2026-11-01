#include "triangle.hpp"

#include <cstddef>
#include <stdexcept>
#include <string>

std::vector<Triangle> ReadTriangles(std::istream& input) {
    int N = 0;
    if (!(input >> N) || N < 0)
        throw std::runtime_error("не удалось прочитать количество треугольников");

    std::vector<Triangle> triangles(N);

    for (std::size_t i = 0; i < static_cast<std::size_t>(N); ++i) {
        for (glm::vec3& point : triangles[i]) {
            if (!(input >> point.x >> point.y >> point.z))
                throw std::runtime_error("не удалось прочитать вершины треугольника " + std::to_string(i + 1));
        }
    }
    return triangles;
}
