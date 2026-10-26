// system headers
#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <map>
#include <stdexcept>
#include <vector>

// glm library
#include "glm/common.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/trigonometric.hpp"
// stb_image library
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
// dear imgui library
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
// glm library
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
// glad and glfw libraries
#include <glad/glad.h>
#include <GLFW/glfw3.h>


// my helper includes
#include "shader.hpp"
#include "camera.hpp"
#include "texture.hpp"
#include "models.hpp"


#define CUBE_MODEL_PATH             "models/cube.v"
#define FRAGMENT_SHADER_PATH        "shaders/f.txt"
#define VERTEX_SHADER_PATH          "shaders/v.txt"
#define LIGHT_VERTEX_SHADER_PATH    "shaders/light_v.txt"
#define LIGHT_FRAGMENT_SHADER_PATH  "shaders/light_f.txt"
#define IMGUI_FONT_PATH             "external/imgui/misc/fonts/Roboto-Medium.ttf" // has Cyrillic
#define IMGUI_FONT_SIZE             16.0f

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow *window);
void MouseCallback(GLFWwindow* window, double xpos, double ypos);
void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
void ToggleMenu(GLFWwindow* window);


// TODO: заменить эти функции на отдельный hpp?
// functions working with triangles
using Triangle = std::array<glm::vec3, 3>;
std::vector<Triangle> ReadTriangles(std::istream& input);
std::vector<float> BuildTriangleVertices(const std::vector<Triangle>& triangles);
bool TrianglesIntersect(const Triangle& first, const Triangle& second);
std::vector<bool> FindIntersections(const std::vector<Triangle>& triangles);



// ========================================================================================
// ====================================SECTION DATA========================================
// ========================================================================================
// settings
// --------
const float SCR_WIDTH    = 800;
const float SCR_HEIGHT   = 600;
const float CREASE_LIMIT = glm::cos(glm::radians(45.0f));

// timing
// ------
float deltaTime = 0.0f;	// time between current frame and last frame
float lastFrame = 0.0f;

// mouse
// -----
bool FirstMouse = true;
float lastX = SCR_WIDTH  / 2;
float lastY = SCR_HEIGHT / 2;
Camera camera{};

// menu vars
// ---------
bool MenuOpen = false;     // ESC: show cursor and ImGui menu, stop processInput
double SavedCursorX = 0.0; // cursor position before menu was opened
double SavedCursorY = 0.0;
// movement settings
float speed         = 2.5f;                    // flying speed
float lightSpeed    = 2.0f;                    // change light rotating speed
bool  lightRotateX  = false;                   // toggle light rotating around X
bool  lightRotateY  = false;                   // toggle light rotating around Y
// object color and lighing
glm::vec3 ambient{ 0.2f, 0.2f, 0.2f};
glm::vec3 diffuse{ 0.5f, 0.5f, 0.5f};
glm::vec3 specular{1.0f, 1.0f, 1.0f};
glm::vec3 objectColor{1.0f};
// light settings
float lightPower    = 1.0f;                    // light power
glm::vec4 lightColor{1.0f};                   // change LightColor
glm::vec3 lightPos{1.2f, 1.0f, 2.0f};          // light position
// ========================================================================================
// ==================================SECTION DATA END======================================
// ========================================================================================



// ========================================================================================
// ====================================SECTION MAIN========================================
// ========================================================================================
int main() {
    std::vector<Triangle> triangles     = ReadTriangles(std::cin);

    const std::vector<bool> intersects  = FindIntersections(triangles);
    std::vector<float> triangleVertices = BuildTriangleVertices(triangles);

// --------------------------glfw: initialize and configure--------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    // --------------------
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Triangles", NULL, NULL);
    if (window == NULL)
    {
        std::cerr << "ERROR: Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
// ----------------------------------------------------------------------------------------

// -------------------------------glad: load all funcs-------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cerr << "ERROR: Failed to initialize GLAD" << std::endl;
        return -1;
    }
// ----------------------------------------------------------------------------------------

// -----------------------------VBO and VAO setup for triangle-----------------------------
    Models cubeModel(CUBE_MODEL_PATH);

    unsigned int triangleVBO, triangleVAO;
    unsigned int cubeVBO, cubeVAO;
    glGenVertexArrays(1, &cubeVAO);     glGenBuffers(1, &cubeVBO);
    glGenVertexArrays(1, &triangleVAO); glGenBuffers(1, &triangleVBO);

    // настраиваем кубик света
    glBindVertexArray(cubeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
    glBufferData(GL_ARRAY_BUFFER, cubeModel.data().size() * sizeof(float), cubeModel.data().data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(0));                 // coordinates
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(3 * sizeof(float))); // normals
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(6 * sizeof(float))); // texture coords
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);

    // настраиваем треугольники
    glBindVertexArray(triangleVAO);
    glBindBuffer(GL_ARRAY_BUFFER, triangleVBO);
    glBufferData(GL_ARRAY_BUFFER, triangleVertices.size() * sizeof(float), triangleVertices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)(0));                 // coordinates
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)(3 * sizeof(float))); // normals
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
// ----------------------------------------------------------------------------------------


// --------------------Mouse and Keyboard Callbacks Configure------------------------------
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED); 
    glfwSetCursorPosCallback(window, MouseCallback); 

    glfwSetScrollCallback(window, ScrollCallback); 
    glfwSetKeyCallback(window, KeyCallback);
    glEnable(GL_DEPTH_TEST);
// ----------------------------------------------------------------------------------------


// ---------------------------------Camera stuff-------------------------------------------
    glm::vec3 CameraTarget(0, 0, 0);
    glm::vec3 up(0, 1, 0);
// ----------------------------------------------------------------------------------------


// -------------------------------Shaders Compiling----------------------------------------
    Shader NormalShaders(VERTEX_SHADER_PATH, FRAGMENT_SHADER_PATH);
    Shader LightShaders(LIGHT_VERTEX_SHADER_PATH, LIGHT_FRAGMENT_SHADER_PATH);
    NormalShaders.use();
// ----------------------------------------------------------------------------------------


// ------------------------------------ImGui setup-----------------------------------------
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");
    io.ConfigFlags |= ImGuiConfigFlags_NoMouse | ImGuiConfigFlags_NoMouseCursorChange; // menu is closed at start
    if (io.Fonts->AddFontFromFileTTF(IMGUI_FONT_PATH, IMGUI_FONT_SIZE) == nullptr)
    {
        std::cerr << "ERROR: failed to add font <" << IMGUI_FONT_PATH << "> please check if it is correct and exists\n";
        return -1;
    }
// ----------------------------------------------------------------------------------------


// ---------------------------------Rendering loop-----------------------------------------
    while (!glfwWindowShouldClose(window))
    {
        // glfw do not do work while iconified
        // -----------------------------------
        if (glfwGetWindowAttrib(window, GLFW_ICONIFIED) != 0) {
            ImGui_ImplGlfw_Sleep(20);
            continue;
        }

        // Start the Dear ImGui frame
        // --------------------------
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        if (MenuOpen) {
            ImGui::Begin("Change some variables!");
            ImGui::Text("Здесь вы можете поменять\n"
                        "Разные параметры модели");
            ImGui::ColorEdit3("цвет треугольников",(float*) &objectColor);
            ImGui::ColorEdit3("цвет света",        (float*) &lightColor);
            ImGui::DragFloat3("позиция света",     (float*) &(lightPos),     0.03f);
            ImGui::SliderFloat("мощность света",   (float*) &(lightPower),   0.5f, 200.0f);
            ImGui::SliderFloat("Скорость",                  &speed,          1.0f, 100.0f);
            ImGui::SliderFloat("Скорость вращения света",   &lightSpeed,     1.0f, 20.0f);

            ImGui::Text("Здесь вы можете поменять\n"
                        "различные параметры освещения");
            ImGui::ColorEdit3("ambient",   (float*) &ambient);
            ImGui::ColorEdit3("diffuse",   (float*) &diffuse);
            ImGui::ColorEdit3("specular",  (float*) &specular);


            ImGui::Checkbox("вращение вокруг x", &lightRotateX);
            ImGui::Checkbox("вращение вокруг y", &lightRotateY);
            ImGui::End();
        }

        ImGui::Render();

        // per-frame time logic
        // --------------------
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // input
        // -----
        if (!MenuOpen)
            processInput(window);

        // reconfigure camera
        // ------------------
        camera.SetSpeed(speed);

        // set light cube parameters
        // -------------------------
        if (lightRotateX || lightRotateY) {
            float angle = lightSpeed * deltaTime;
            float sinA  = sin(angle);
            float cosA  = cos(angle);
            if (lightRotateX) {
                float y = lightPos.y;
                float z = lightPos.z;
                lightPos.y = y * cosA - z * sinA;
                lightPos.z = y * sinA + z * cosA;
            }
            if (lightRotateY) {
                float x = lightPos.x;
                float z = lightPos.z;
                lightPos.x =  x * cosA + z * sinA;
                lightPos.z = -x * sinA + z * cosA;
            }
        }
        
        // Clear Everything
        // ----------------
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        // activate normal shader
        // ---------------------
        NormalShaders.use();
        NormalShaders.setVec3("light.position", lightPos);
        NormalShaders.setVec3("viewPos", camera.Position_);

        // send different light properties
        // -------------------------------
        NormalShaders.setVec3("light.ambient",       ambient);
        NormalShaders.setVec3("light.diffuse",       diffuse);
        NormalShaders.setVec3("light.specular",      specular); 
        NormalShaders.setFloat("lightPower",         lightPower);
        NormalShaders.setFloat("light.constant",     1.0f);
        NormalShaders.setFloat("light.linear",       0.09f);
        NormalShaders.setFloat("light.quadratic",    0.032f);
        NormalShaders.setFloat("material.shininess", 16.0f);

        // calculate view, and projection for normal shader
        // ------------------------------------------------
        glm::mat4 projection = glm::perspective(glm::radians(camera.GetZoom()), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 1000.0f);
        glm::mat4 view = camera.GetViewMatrix();
        NormalShaders.setMat4fv("projection", projection);
        NormalShaders.setMat4fv("view", view);

        // calculate transform for normal shader
        // -------------------------------------

        // render the triangles
        // --------------------
        glBindVertexArray(triangleVAO);
        glm::mat4 trans{1.0f};
        NormalShaders.setMat4fv("trans", trans);
        NormalShaders.setVec3("material.diffuse",  glm::vec3{1.0f});
        NormalShaders.setVec3("material.specular", glm::vec3{1.0f});
        glDrawArrays(GL_TRIANGLES, 0, triangles.size() * 3);

        // activate light shader
        // -------------------------
        LightShaders.use();
        LightShaders.setMat4fv("projection", projection);
        LightShaders.setMat4fv("view", view);
        LightShaders.setVec3("lightColor", glm::vec3{1.0f});

        // calculate transform for light shader and render all lamps
        // ---------------------------------------------------------
        trans = glm::translate(glm::mat4{1.0f}, lightPos);
        trans = glm::scale(trans, glm::vec3(0.2f));
        LightShaders.setMat4fv("trans", trans);

        glBindVertexArray(cubeVAO);
        glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // render imgui
        // ------------
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
// ----------------------------------------------------------------------------------------

    // optional: de-allocate all resources once they've outlived their purpose:
    // ------------------------------------------------------------------------
    glDeleteVertexArrays(1, &cubeVAO);
    glDeleteVertexArrays(1, &triangleVAO);
    glDeleteBuffers(1, &triangleVBO);
    glDeleteBuffers(1, &cubeVBO);

    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}
// ========================================================================================
// =================================END SECTION MAIN=======================================
// ========================================================================================




// ========================================================================================
// ==================================SECTION FUNCS=========================================
// ========================================================================================

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

std::vector<Triangle> ReadTriangles(std::istream& input) {
    int N = 0;
    if (!(input >> N) || N < 0)
        throw std::runtime_error("не удалось прочитать количество треугольников");

    std::vector<Triangle> triangles(N);

    for (std::size_t i = 0; i < static_cast<size_t>(N); ++i) {
        for (glm::vec3& point : triangles[i]) {
            if (!(input >> point.x >> point.y >> point.z))
                throw std::runtime_error("не удалось прочитать вершины треугольника " + std::to_string(i + 1));
        }
    }
    return triangles;
}

std::vector<float> BuildTriangleVertices(const std::vector<Triangle>& triangles) {
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
                normal.x,      normal.y,      normal.z,
            });
        }
    }
    return vertices;
}



// mouse callback function - change yaw and pitch, Recalculate CameraViewDirVec
// ------------------------------------------------------------------------
void MouseCallback(GLFWwindow* window, double xpos, double ypos) {
    if (MenuOpen)
        return;
    camera.ProcessMouseMovement(xpos, ypos);
}  

// scroll callback function - on scroll change fov
// -----------------------------------------------
void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
    camera.ProcessMouseScroll(yoffset);
}

// key callback function - ESC toggles menu (once per press, not every frame)
// -------------------------------------------------------------------------
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        ToggleMenu(window);
    if (key == GLFW_KEY_L && action == GLFW_PRESS)
        lightPos = camera.Position_;
}

// open menu: show cursor, give mouse to ImGui
// close menu: hide cursor, return it to saved position so camera doesn't jump
// ---------------------------------------------------------------------------
void ToggleMenu(GLFWwindow* window) {
    ImGuiIO& io = ImGui::GetIO();

    if (!MenuOpen) {
        glfwGetCursorPos(window, &SavedCursorX, &SavedCursorY);
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        io.ConfigFlags &= ~ImGuiConfigFlags_NoMouse;
        MenuOpen = true;
        return;
    }

    glfwSetCursor(window, nullptr);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPos(window, SavedCursorX, SavedCursorY);
    io.ConfigFlags |= ImGuiConfigFlags_NoMouse;
    MenuOpen = false;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------

void processInput(GLFWwindow *window)
{
    camera.ProcessKeyboard(window, deltaTime);
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

// ========================================================================================
// ================================END SECTION FUNCS=======================================
// ========================================================================================
