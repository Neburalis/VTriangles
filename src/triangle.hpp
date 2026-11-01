#pragma once

#include <array>
#include <istream>
#include <vector>

#include <glm/glm.hpp>

// Треугольник задаётся тремя вершинами; порядок обхода важен для нормали.
using Triangle = std::array<glm::vec3, 3>;

// Читает количество треугольников и координаты их вершин из потока.
// Бросает std::runtime_error, если ввод не соответствует формату.
std::vector<Triangle> ReadTriangles(std::istream& input);
