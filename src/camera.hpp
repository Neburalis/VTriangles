#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <GLFW/glfw3.h>

// Варианты движения камеры, не зависящие от конкретной оконной системы.
enum Camera_Movement {
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT
};

// Значения камеры по умолчанию.
const float YAW         = -90.0f;
const float PITCH       = 0.0f;
const float SPEED       = 5.0f;
const float SENSITIVITY = 0.1f;
const float ZOOM        = 90.0f;
const float SPACE_THRESHOLD = 0.3f;   // максимум секунд между двумя нажатиями Space
const float G = 9.8f;
const float JUMP_SPEED = 4.0f;
const float GROUND_Y = 0.0f;

// Камера обрабатывает ввод и вычисляет углы Эйлера, векторы и матрицы для OpenGL.
class Camera {
public:
    // Атрибуты камеры.
    glm::vec3 Position_;
    glm::vec3 Front_;
    glm::vec3 Up_;
    glm::vec3 Right_;
    glm::vec3 WorldUp_;
    glm::vec3 WorldRight_;

    // Углы Эйлера.
    float Yaw_;
    float Pitch_;

    // Настройки камеры.
    float MovementSpeed_;
    float MouseSensitivity_;
    float Zoom_;

    // Режим полёта.
    float TimeLastSpace_;
    bool Flying_;
    bool SpaceWasPressed_;
    float VerticalSpeed_;

    // Последние координаты мыши.
    float LastX_;
    float LastY_;

    Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 3.0f),
           glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f),
           glm::vec3 right = glm::vec3(1.0f, 0.0f, 0.0f),
           float yaw = YAW, float pitch = PITCH)
        : Front_(glm::vec3(0.0f, 0.0f, -1.0f)),
          MovementSpeed_(SPEED),
          MouseSensitivity_(SENSITIVITY),
          Zoom_(ZOOM) {
        Position_ = position;
        WorldUp_ = up;
        WorldRight_ = right;
        Yaw_ = yaw;
        Pitch_ = pitch;
        TimeLastSpace_ = -SPACE_THRESHOLD;
        Flying_ = true;
        SpaceWasPressed_ = false;
        VerticalSpeed_ = 0.0f;

        LastX_ = 0.0f;
        LastY_ = 0.0f;
        updateCameraVectors();
    }

    // Возвращает view-матрицу по углам Эйлера и положению камеры.
    glm::mat4 GetViewMatrix() {
        return glm::lookAt(Position_, Position_ + Front_, Up_);
    }

    float GetZoom() const noexcept { return Zoom_; }
    void SetSpeed(float Speed) noexcept { MovementSpeed_ = Speed; }

    // Обрабатывает ввод с клавиатуры, не завися от конкретной оконной системы.
    void ProcessKeyboard(GLFWwindow* window, float deltaTime) {
        float velocity = MovementSpeed_ * deltaTime;
        float time = glfwGetTime();
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            Position_ += Front_ * velocity;
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            Position_ -= Front_ * velocity;
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            Position_ -= glm::normalize(glm::cross(Front_, Up_)) * velocity;
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            Position_ += glm::normalize(glm::cross(Front_, Up_)) * velocity;

        if (IsSpaceDoubleTapped(window, time)) {
            Flying_ = !Flying_;
            VerticalSpeed_ = 0.0f;
        } else if (SpaceWasPressed_ & Flying_) {
            Position_ += WorldUp_ * velocity;
        }

        if (Flying_)
            ProcessFlying(window, velocity);
        else {
            VerticalSpeed_ += G * deltaTime;
            Position_.y -= VerticalSpeed_ * deltaTime;
        }
    }

    // Обрабатывает движение мыши. Ожидает новые координаты курсора.
    void ProcessMouseMovement(float xpos, float ypos) {
        float xoffset = xpos - LastX_;
        float yoffset = LastY_ - ypos;
        LastX_ = xpos;
        LastY_ = ypos;

        xoffset *= MouseSensitivity_;
        yoffset *= MouseSensitivity_;

        Yaw_   += xoffset;
        Pitch_ += yoffset;

        // Ограничиваем наклон, чтобы изображение не переворачивалось.
        if (Pitch_ > 89.0f)
            Pitch_ = 89.0f;
        if (Pitch_ < -89.0f)
            Pitch_ = -89.0f;

        updateCameraVectors();
    }

    // Обрабатывает прокрутку колеса мыши и меняет угол обзора.
    void ProcessMouseScroll(float yoffset) {
        Zoom_ -= yoffset;
        if (Zoom_ < 1.0f)
            Zoom_ = 1.0f;
        if (Zoom_ > 90.0f)
            Zoom_ = 90.0f;
    }

private:
    // true только в кадре второго нажатия Space в пределах SPACE_THRESHOLD секунд.
    bool IsSpaceDoubleTapped(GLFWwindow* window, float time) {
        bool SpacePressed = glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;
        bool JustPressed  = SpacePressed && !SpaceWasPressed_;
        SpaceWasPressed_  = SpacePressed;

        if (!JustPressed)
            return false;

        if (time - TimeLastSpace_ < SPACE_THRESHOLD) {
            TimeLastSpace_ = -SPACE_THRESHOLD;   // третье нажатие начинает новую пару
            return true;
        }
        TimeLastSpace_ = time;
        return false;
    }

    // Полёт: Space — вверх, Left Shift — вниз, гравитации нет.
    void ProcessFlying(GLFWwindow* window, float velocity) {
        if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
            Position_ -= WorldUp_ * velocity;
    }

    // Пересчитывает вектор направления по обновлённым углам Эйлера.
    void updateCameraVectors() {
        glm::vec3 front;
        front.x = cos(glm::radians(Yaw_)) * cos(glm::radians(Pitch_));
        front.y = sin(glm::radians(Pitch_));
        front.z = sin(glm::radians(Yaw_)) * cos(glm::radians(Pitch_));

        Front_ = glm::normalize(front);
        // Также пересчитываем Right и Up.
        Right_ = glm::normalize(glm::cross(Front_, WorldUp_));
        Up_    = glm::normalize(glm::cross(Right_, Front_));
    }
};
