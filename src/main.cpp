#include <iostream>
#include <utility>
#include <vector>

#include "application.hpp"
#include "geometry.hpp"
#include "mesh.hpp"
#include "triangle.hpp"

int main() {
    std::vector<Triangle> triangles = ReadTriangles(std::cin);

    const std::vector<bool> intersects = FindIntersections(triangles);

    std::vector<float> triangleVertices = BuildTriangleVertices(triangles, intersects);

    Application application(triangles.size(), std::move(triangleVertices));
    return application.Run();
}
