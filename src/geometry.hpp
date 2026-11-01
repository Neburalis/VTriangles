#pragma once

#include <vector>

#include "triangle.hpp"

// Точная проверка пересечения двух невырожденных треугольников методом
// разделяющих осей (SAT). Вырожденные треугольники считаются непересекающимися.
bool TrianglesIntersect(const Triangle& first, const Triangle& second);

// Помечает true треугольники, участвующие хотя бы в одном пересечении.
// Broad phase выполняется развёрткой по одной координатной оси (sweep-and-prune).
std::vector<bool> FindIntersections(const std::vector<Triangle>& triangles);
