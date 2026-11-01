#include "application.hpp"

#include <cmath>
#include <iostream>
#include <memory>
#include <utility>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/trigonometric.hpp>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "models.hpp"
#include "shader.hpp"

namespace {
constexpr float SCR_WIDTH  = 800.0f;
constexpr float SCR_HEIGHT = 600.0f;

constexpr const char* CUBE_MODEL_PATH            = "models/cube.v";
constexpr const char* FRAGMENT_SHADER_PATH       = "shaders/f.txt";
constexpr const char* VERTEX_SHADER_PATH         = "shaders/v.txt";
constexpr const char* LIGHT_VERTEX_SHADER_PATH   = "shaders/light_v.txt";
constexpr const char* LIGHT_FRAGMENT_SHADER_PATH = "shaders/light_f.txt";
constexpr const char* IMGUI_FONT_PATH            = "external/imgui/misc/fonts/Roboto-Medium.ttf"; // есть кириллица
constexpr float IMGUI_FONT_SIZE                  = 16.0f;
} // namespace

Application::Application(std::size_t triangleCount, std::vector<float> triangleVertices)
    : triangleCount_(triangleCount), triangleVertices_(std::move(triangleVertices)) {}

Application::~Application() {
    Shutdown();
}

int Application::Run() {
    // --------------------------glfw: initialize and configure--------------------------------
    if (glfwInit() != GLFW_TRUE) {
        std::cerr << "ERROR: Failed to initialize GLFW" << std::endl;
        return -1;
    }
    glfwInitialized_ = true;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    window_ = glfwCreateWindow(static_cast<int>(SCR_WIDTH), static_cast<int>(SCR_HEIGHT),
                               "Triangles", nullptr, nullptr);
    if (window_ == nullptr) {
        std::cerr << "ERROR: Failed to create GLFW window" << std::endl;
        Shutdown();
        return -1;
    }
    glfwMakeContextCurrent(window_);
    glfwSetWindowUserPointer(window_, this);
    glfwSetFramebufferSizeCallback(window_, FramebufferSizeCallback);

    // -------------------------------glad: load all funcs-------------------------------------
    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
        std::cerr << "ERROR: Failed to initialize GLAD" << std::endl;
        Shutdown();
        return -1;
    }

    // -----------------------------VBO and VAO setup for triangle-----------------------------
    cubeModel_ = std::make_unique<Models>(CUBE_MODEL_PATH);

    glGenVertexArrays(1, &cubeVAO_);
    glGenBuffers(1, &cubeVBO_);
    glGenVertexArrays(1, &triangleVAO_);
    glGenBuffers(1, &triangleVBO_);

    // Настраиваем кубик источника света.
    glBindVertexArray(cubeVAO_);
    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO_);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(cubeModel_->data().size() * sizeof(float)),
                 cubeModel_->data().data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(0));                 // координаты
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));  // нормали
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));  // текстурные координаты
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);

    // Настраиваем треугольники.
    glBindVertexArray(triangleVAO_);
    glBindBuffer(GL_ARRAY_BUFFER, triangleVBO_);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(triangleVertices_.size() * sizeof(float)),
                 triangleVertices_.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(0));                // координаты
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float))); // нормали
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    // --------------------Mouse and Keyboard Callbacks Configure------------------------------
    glfwSetInputMode(window_, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(window_, MouseCallback);
    glfwSetScrollCallback(window_, ScrollCallback);
    glfwSetKeyCallback(window_, KeyCallback);
    glEnable(GL_DEPTH_TEST);

    // -------------------------------Shaders Compiling----------------------------------------
    normalShaders_ = std::make_unique<Shader>(VERTEX_SHADER_PATH, FRAGMENT_SHADER_PATH);
    lightShaders_ = std::make_unique<Shader>(LIGHT_VERTEX_SHADER_PATH, LIGHT_FRAGMENT_SHADER_PATH);
    normalShaders_->use();

    // ------------------------------------ImGui setup-----------------------------------------
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    imguiContextInitialized_ = true;
    ImGuiIO& io = ImGui::GetIO();
    (void)io;

    ImGui::StyleColorsDark();
    if (!ImGui_ImplGlfw_InitForOpenGL(window_, true)) {
        std::cerr << "ERROR: failed to initialize Dear ImGui GLFW backend\n";
        Shutdown();
        return -1;
    }
    imguiGlfwInitialized_ = true;
    if (!ImGui_ImplOpenGL3_Init("#version 330")) {
        std::cerr << "ERROR: failed to initialize Dear ImGui OpenGL backend\n";
        Shutdown();
        return -1;
    }
    imguiOpenGLInitialized_ = true;
    io.ConfigFlags |= ImGuiConfigFlags_NoMouse | ImGuiConfigFlags_NoMouseCursorChange; // меню закрыто при запуске
    if (io.Fonts->AddFontFromFileTTF(IMGUI_FONT_PATH, IMGUI_FONT_SIZE) == nullptr) {
        std::cerr << "ERROR: failed to add font <" << IMGUI_FONT_PATH
                  << "> please check if it is correct and exists\n";
        Shutdown();
        return -1;
    }

    // ---------------------------------Rendering loop-----------------------------------------
    while (!glfwWindowShouldClose(window_)) {
        // GLFW не выполняет работу, пока окно свёрнуто.
        if (glfwGetWindowAttrib(window_, GLFW_ICONIFIED) != 0) {
            ImGui_ImplGlfw_Sleep(20);
            continue;
        }

        // Начинаем кадр Dear ImGui.
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        if (menuOpen_)
            RenderMenu();
        ImGui::Render();

        // Логика времени кадра.
        const float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime_ = currentFrame - lastFrame_;
        lastFrame_ = currentFrame;

        if (!menuOpen_)
            ProcessInput();

        camera_.SetSpeed(speed_);
        UpdateLightRotation(deltaTime_);

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        RenderScene();

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window_);
        glfwPollEvents();
    }

    Shutdown();
    return 0;
}

void Application::Shutdown() noexcept {
    if (window_ != nullptr)
        glfwMakeContextCurrent(window_);

    if (imguiOpenGLInitialized_) {
        ImGui_ImplOpenGL3_Shutdown();
        imguiOpenGLInitialized_ = false;
    }
    if (imguiGlfwInitialized_) {
        ImGui_ImplGlfw_Shutdown();
        imguiGlfwInitialized_ = false;
    }
    if (imguiContextInitialized_) {
        ImGui::DestroyContext();
        imguiContextInitialized_ = false;
    }

    // OpenGL-объекты должны уничтожаться, пока контекст ещё существует.
    lightShaders_.reset();
    normalShaders_.reset();
    cubeModel_.reset();

    if (cubeVAO_ != 0) {
        glDeleteVertexArrays(1, &cubeVAO_);
        cubeVAO_ = 0;
    }
    if (triangleVAO_ != 0) {
        glDeleteVertexArrays(1, &triangleVAO_);
        triangleVAO_ = 0;
    }
    if (triangleVBO_ != 0) {
        glDeleteBuffers(1, &triangleVBO_);
        triangleVBO_ = 0;
    }
    if (cubeVBO_ != 0) {
        glDeleteBuffers(1, &cubeVBO_);
        cubeVBO_ = 0;
    }

    if (window_ != nullptr) {
        glfwDestroyWindow(window_);
        window_ = nullptr;
    }
    if (glfwInitialized_) {
        glfwTerminate();
        glfwInitialized_ = false;
    }
}

void Application::RenderMenu() {
    ImGui::Begin("Change some variables!");
    ImGui::Text("Здесь вы можете поменять\n"
                "Разные параметры модели");
    ImGui::ColorEdit3("цвет треугольников", reinterpret_cast<float*>(&objectColor_));
    ImGui::ColorEdit3("цвет света",         reinterpret_cast<float*>(&lightColor_));
    ImGui::DragFloat3("позиция света",      reinterpret_cast<float*>(&lightPos_), 0.03f);
    ImGui::SliderFloat("мощность света",    &lightPower_, 0.5f, 200.0f);
    ImGui::SliderFloat("Скорость",          &speed_, 1.0f, 100.0f);
    ImGui::SliderFloat("Скорость вращения света", &lightSpeed_, 1.0f, 20.0f);

    ImGui::Text("Здесь вы можете поменять\n"
                "различные параметры освещения");
    ImGui::ColorEdit3("ambient",  reinterpret_cast<float*>(&ambient_));
    ImGui::ColorEdit3("diffuse",  reinterpret_cast<float*>(&diffuse_));
    ImGui::ColorEdit3("specular", reinterpret_cast<float*>(&specular_));

    ImGui::Checkbox("вращение вокруг x", &lightRotateX_);
    ImGui::Checkbox("вращение вокруг y", &lightRotateY_);
    ImGui::End();
}

void Application::UpdateLightRotation(float deltaTime) {
    if (!lightRotateX_ && !lightRotateY_)
        return;

    const float angle = lightSpeed_ * deltaTime;
    const float sinA  = std::sin(angle);
    const float cosA  = std::cos(angle);
    if (lightRotateX_) {
        const float y = lightPos_.y;
        const float z = lightPos_.z;
        lightPos_.y = y * cosA - z * sinA;
        lightPos_.z = y * sinA + z * cosA;
    }
    if (lightRotateY_) {
        const float x = lightPos_.x;
        const float z = lightPos_.z;
        lightPos_.x =  x * cosA + z * sinA;
        lightPos_.z = -x * sinA + z * cosA;
    }
}

void Application::RenderScene() {
    // Активируем шейдер обычных треугольников.
    normalShaders_->use();
    normalShaders_->setVec3("light.position", lightPos_);
    normalShaders_->setVec3("viewPos", camera_.Position_);

    // Передаём параметры источника света.
    const glm::vec3 lightColor{lightColor_};
    normalShaders_->setVec3("light.ambient",       ambient_ * lightColor);
    normalShaders_->setVec3("light.diffuse",       diffuse_ * lightColor);
    normalShaders_->setVec3("light.specular",      specular_ * lightColor);
    normalShaders_->setFloat("lightPower",         lightPower_);
    normalShaders_->setFloat("light.constant",     1.0f);
    normalShaders_->setFloat("light.linear",       0.09f);
    normalShaders_->setFloat("light.quadratic",    0.032f);
    normalShaders_->setFloat("material.shininess", 16.0f);

    const glm::mat4 projection = glm::perspective(
        glm::radians(camera_.GetZoom()),
        SCR_WIDTH / SCR_HEIGHT, 0.1f, 1000.0f);
    const glm::mat4 view = camera_.GetViewMatrix();
    normalShaders_->setMat4fv("projection", projection);
    normalShaders_->setMat4fv("view", view);

    // Отрисовываем треугольники.
    glBindVertexArray(triangleVAO_);
    const glm::mat4 identity{1.0f};
    normalShaders_->setMat4fv("trans", identity);
    normalShaders_->setVec3("material.diffuse",  objectColor_);
    normalShaders_->setVec3("material.specular", glm::vec3{1.0f});
    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(triangleCount_ * 3));

    // Активируем шейдер источника света.
    lightShaders_->use();
    lightShaders_->setMat4fv("projection", projection);
    lightShaders_->setMat4fv("view", view);
    lightShaders_->setVec3("lightColor", lightColor);

    glm::mat4 trans = glm::translate(glm::mat4{1.0f}, lightPos_);
    trans = glm::scale(trans, glm::vec3(0.2f));
    lightShaders_->setMat4fv("trans", trans);

    glBindVertexArray(cubeVAO_);
    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO_);
    glDrawArrays(GL_TRIANGLES, 0, 36);
}
