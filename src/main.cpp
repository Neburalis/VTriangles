#include <iostream>
#include <utility>
#include <vector>

#include "application.hpp"
#include "geometry.hpp"
#include "mesh.hpp"
#include "triangle.hpp"

int main() {
    std::vector<Triangle> triangles = ReadTriangles(std::cin);

    // Результат пока не используется рендерером, но поиск выполняется всегда.
    const std::vector<bool> intersects = FindIntersections(triangles);
    (void)intersects;

    std::vector<float> triangleVertices = BuildTriangleVertices(triangles);

    Application application(triangles.size(), std::move(triangleVertices));
    return application.Run();
}
