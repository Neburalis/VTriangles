#include "application.hpp"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "imgui.h"

Application* Application::FromWindow(GLFWwindow* window) {
    return static_cast<Application*>(glfwGetWindowUserPointer(window));
}

// Ввод камеры: клавиши движения опрашиваются каждый кадр.
void Application::ProcessInput() {
    camera_.ProcessKeyboard(window_, deltaTime_);
}

// Мышь меняет направление взгляда, пока меню закрыто.
void Application::MouseCallback(GLFWwindow* window, double xpos, double ypos) {
    Application* application = FromWindow(window);
    if (application == nullptr || application->menuOpen_)
        return;
    application->camera_.ProcessMouseMovement(static_cast<float>(xpos), static_cast<float>(ypos));
}

// Колесо мыши меняет угол обзора.
void Application::ScrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
    Application* application = FromWindow(window);
    if (application == nullptr)
        return;
    application->camera_.ProcessMouseScroll(static_cast<float>(yoffset));
}

// ESC открывает или закрывает меню; L переносит свет в позицию камеры.
void Application::KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    (void)scancode;
    (void)mods;

    Application* application = FromWindow(window);
    if (application == nullptr)
        return;

    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        application->ToggleMenu();
    if (key == GLFW_KEY_L && action == GLFW_PRESS)
        application->lightPos_ = application->camera_.Position_;
}

// Открытие меню показывает курсор и отдаёт мышь ImGui.
// Закрытие прячет курсор и возвращает его в сохранённую позицию, чтобы камера не прыгала.
void Application::ToggleMenu() {
    ImGuiIO& io = ImGui::GetIO();

    if (!menuOpen_) {
        glfwGetCursorPos(window_, &savedCursorX_, &savedCursorY_);
        glfwSetInputMode(window_, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        io.ConfigFlags &= ~ImGuiConfigFlags_NoMouse;
        menuOpen_ = true;
        return;
    }

    glfwSetCursor(window_, nullptr);
    glfwSetInputMode(window_, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPos(window_, savedCursorX_, savedCursorY_);
    io.ConfigFlags |= ImGuiConfigFlags_NoMouse;
    menuOpen_ = false;
}

// При изменении размера окна обновляем область просмотра.
void Application::FramebufferSizeCallback(GLFWwindow* window, int width, int height) {
    (void)window;
    glViewport(0, 0, width, height);
}
