#pragma once

#include <cstddef>
#include <memory>
#include <vector>

#include <glm/glm.hpp>

#include "camera.hpp"

struct GLFWwindow;
class Models;
class Shader;

// Окно, контекст OpenGL, отрисовка сцены и меню Dear ImGui.
class Application {
public:
    Application(std::size_t triangleCount, std::vector<float> triangleVertices);
    ~Application();

    // Создаёт окно и выполняет цикл отрисовки. Возвращает код завершения main.
    int Run();

private:
    void Shutdown() noexcept;
    void ProcessInput();
    void UpdateLightRotation(float deltaTime);
    void ToggleMenu();
    void RenderMenu();
    void RenderScene();

    static Application* FromWindow(GLFWwindow* window);
    static void FramebufferSizeCallback(GLFWwindow* window, int width, int height);
    static void MouseCallback(GLFWwindow* window, double xpos, double ypos);
    static void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
    static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

    GLFWwindow* window_ = nullptr;
    bool glfwInitialized_ = false;
    bool imguiContextInitialized_ = false;
    bool imguiGlfwInitialized_ = false;
    bool imguiOpenGLInitialized_ = false;

    std::size_t triangleCount_ = 0;
    std::vector<float> triangleVertices_;

    unsigned int triangleVBO_ = 0;
    unsigned int triangleVAO_ = 0;
    unsigned int cubeVBO_ = 0;
    unsigned int cubeVAO_ = 0;

    std::unique_ptr<Models> cubeModel_;
    std::unique_ptr<Shader> normalShaders_;
    std::unique_ptr<Shader> lightShaders_;

    Camera camera_{};

    float deltaTime_ = 0.0f;
    float lastFrame_ = 0.0f;

    bool menuOpen_ = false;
    double savedCursorX_ = 0.0;
    double savedCursorY_ = 0.0;

    float speed_ = 2.5f;
    float lightSpeed_ = 2.0f;
    bool lightRotateX_ = false;
    bool lightRotateY_ = false;

    glm::vec3 ambient_{0.2f, 0.2f, 0.2f};
    glm::vec3 diffuse_{0.5f, 0.5f, 0.5f};
    glm::vec3 specular_{1.0f, 1.0f, 1.0f};
    glm::vec3 objectColor_{1.0f};

    float lightPower_ = 1.0f;
    glm::vec4 lightColor_{1.0f};
    glm::vec3 lightPos_{1.2f, 1.0f, 2.0f};
};
