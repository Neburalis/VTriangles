#include "mesh.hpp"

#include <array>
#include <cstddef>
#include <map>
#include <vector>

#include <glm/glm.hpp>
#include <glm/trigonometric.hpp>

namespace {
// Порог косинуса угла между нормалями соседних граней (45 градусов).
const float CREASE_LIMIT = glm::cos(glm::radians(45.0f));
} // namespace

std::vector<float> BuildTriangleVertices(const std::vector<Triangle>& triangles, const std::vector<bool> &intersects) {
    std::vector<float> vertices;
    vertices.reserve(triangles.size() * 3 * 6);

    std::map<std::array<float, 3>, std::vector<std::size_t>> adjacentFaces;
    std::vector<glm::vec3> areaNormals(triangles.size());
    std::vector<glm::vec3> faceNormals(triangles.size(), glm::vec3(0.0f));

    for (std::size_t face = 0; face < triangles.size(); ++face) {
        areaNormals[face] = glm::cross(triangles[face][1] - triangles[face][0],
                                       triangles[face][2] - triangles[face][0]);
        faceNormals[face] = glm::normalize(areaNormals[face]);
        for (const glm::vec3& point : triangles[face])
            adjacentFaces[{point.x, point.y, point.z}].push_back(face);
    }

    for (std::size_t face = 0; face < triangles.size(); ++face) {
        for (const glm::vec3& point : triangles[face]) {
            glm::vec3 normalSum(0.0f);

            const auto key_value = adjacentFaces.find({point.x, point.y, point.z});
            const auto value     = key_value->second;
            for (std::size_t neighbour : value) {
                if (glm::dot(faceNormals[face], faceNormals[neighbour]) >= CREASE_LIMIT)
                    normalSum += areaNormals[neighbour];
            }
            const glm::vec3 normal = glm::normalize(normalSum);
            vertices.insert(vertices.end(), {
                point.x,      point.y,      point.z,
                normal.x,     normal.y,      normal.z,
                static_cast<float>(intersects[face]),
            });
        }
    }
    return vertices;
}
