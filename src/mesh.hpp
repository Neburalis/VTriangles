#pragma once

#include <vector>

#include "triangle.hpp"

// Строит массив вершин: координаты (3), нормаль (3), флаг пересечения (1 float).
// Нормали сглаживаются только между гранями с точно совпадающими координатами
// общей вершины и углом между нормалями не больше 45 градусов.
std::vector<float> BuildTriangleVertices(const std::vector<Triangle>& triangles, const std::vector<bool> &intersects);
