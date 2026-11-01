#include "geometry.hpp"

#include <algorithm>
#include <array>
#include <cstddef>

#include <glm/glm.hpp>

namespace {
// Допуск в координатах сцены: касание считаем пересечением.
constexpr double IntersectionEpsilon = 1.0e-5;
} // namespace

// SAT: если проекции на хотя бы одну ось разошлись, треугольники не пересекаются.
// Порядок обхода вершин не важен; проверяем и треугольники в одной плоскости.
// Проекция — скалярное произведение вершины и единичного направления оси.
// Три проекции задают интервал [min, max]; зазор между интервалами разделяет фигуры.
bool TrianglesIntersect(const Triangle& first, const Triangle& second) {
    std::array<glm::dvec3, 3> a, b, edgesA, edgesB;
    for (int i = 0; i < 3; ++i) {
        // Общий локальный origin уменьшает потерю точности при проецировании.
        a[i] = glm::dvec3(first[i]) - glm::dvec3(first[0]);
        b[i] = glm::dvec3(second[i]) - glm::dvec3(first[0]);
        edgesA[i] = glm::dvec3(first[(i + 1) % 3]) - glm::dvec3(first[i]);
        edgesB[i] = glm::dvec3(second[(i + 1) % 3]) - glm::dvec3(second[i]);
    }
    const glm::dvec3 normalA = glm::cross(edgesA[0], edgesA[1]);
    const glm::dvec3 normalB = glm::cross(edgesB[0], edgesB[1]);
    // Как и раньше, вырожденные треугольники (точки/отрезки) не учитываются.
    if (glm::length(normalA) <= IntersectionEpsilon
        || glm::length(normalB) <= IntersectionEpsilon)
        return false;

    // Нормализация оси позволяет сравнивать зазор с допуском в единицах сцены.
    const auto separated = [&](glm::dvec3 axis) {
        const double length = glm::length(axis);
        if (length == 0.0) return false; // параллельные рёбра не задают ось
        axis /= length;
        double minA = glm::dot(a[0], axis), maxA = minA;
        double minB = glm::dot(b[0], axis), maxB = minB;
        for (int i = 1; i < 3; ++i) {
            const double pa = glm::dot(a[i], axis), pb = glm::dot(b[i], axis);
            minA = std::min(minA, pa); maxA = std::max(maxA, pa);
            minB = std::min(minB, pb); maxB = std::max(maxB, pb);
        }
        return maxA < minB - IntersectionEpsilon || maxB < minA - IntersectionEpsilon;
    };

    // Сначала проверяем направления, перпендикулярные плоскостям треугольников.
    if (separated(normalA) || separated(normalB)) return false;
    for (int i = 0; i < 3; ++i) {
        // Оси поперёк рёбер в плоскости нужны для копланарных треугольников.
        if (separated(glm::cross(normalA, edgesA[i]))
            || separated(glm::cross(normalB, edgesB[i])))
            return false;
        // Для непараллельных плоскостей нужны также оси поперёк каждой пары рёбер.
        for (int j = 0; j < 3; ++j)
            if (separated(glm::cross(edgesA[i], edgesB[j]))) return false;
    }
    return true;
}

// Sweep-and-prune: сортируем границы, точную геометрию проверяем лишь у кандидатов.
// O(N log N + K), где K — число пар с перекрытием интервалов вдоль выбранной оси.
std::vector<bool> FindIntersections(const std::vector<Triangle>& triangles) {
    struct Bounds {
        glm::dvec3 min, max;
        std::size_t index; // исходный номер, чтобы сохранить порядок и цвета
    };
    std::vector<bool> intersects(triangles.size(), false);
    if (triangles.empty()) return intersects;
    std::vector<Bounds> bounds;
    bounds.reserve(triangles.size());
    glm::dvec3 centersMin(0.0), centersMax(0.0);
    // min/max по X, Y, Z задают коробку, внутри которой лежит весь треугольник.
    // Раздельные коробки гарантируют отсутствие пересечения; общая коробка — ещё нет.
    for (std::size_t i = 0; i < triangles.size(); ++i) {
        Bounds box{glm::dvec3(triangles[i][0]), glm::dvec3(triangles[i][0]), i};
        for (int j = 1; j < 3; ++j) {
            box.min = glm::min(box.min, glm::dvec3(triangles[i][j]));
            box.max = glm::max(box.max, glm::dvec3(triangles[i][j]));
        }
        bounds.push_back(box);
        const glm::dvec3 center = (box.min + box.max) * 0.5;
        centersMin = i == 0 ? center : glm::min(centersMin, center);
        centersMax = i == 0 ? center : glm::max(centersMax, center);
    }
    // Берём ось с наибольшим разбросом центров: подходит и для плоскостей YZ/XZ.
    const glm::dvec3 spread = centersMax - centersMin;
    int axis = spread.y > spread.x ? 1 : 0;
    if (spread.z > spread[axis]) axis = 2;
    // Сортируем только коробки; исходный массив треугольников остаётся на месте.
    std::sort(bounds.begin(), bounds.end(), [axis](const Bounds& a, const Bounds& b) {
        return a.min[axis] < b.min[axis];
    });
    for (std::size_t i = 0; i < bounds.size(); ++i) {
        const Bounds& a = bounds[i];
        for (std::size_t j = i + 1; j < bounds.size(); ++j) {
            const Bounds& b = bounds[j];
            // Все следующие границы ещё дальше — оставшиеся пары можно пропустить.
            if (b.min[axis] > a.max[axis] + IntersectionEpsilon) break;
            // Для окраски нужен только факт пересечения, полный список пар не нужен.
            if (intersects[a.index] && intersects[b.index]) continue;
            // Дешёвая проверка ограничивающих параллелепипедов по всем трём осям.
            if (glm::any(glm::greaterThan(a.min, b.max + IntersectionEpsilon))
                || glm::any(glm::greaterThan(b.min, a.max + IntersectionEpsilon)))
                continue;
            if (TrianglesIntersect(triangles[a.index], triangles[b.index]))
                intersects[a.index] = intersects[b.index] = true;
        }
    }
    return intersects;
}
