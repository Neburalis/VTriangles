#ifndef CAMERA_H
#define CAMERA_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <GLFW/glfw3.h>

// Defines several possible options for camera movement. Used as abstraction to stay away from window-system specific input methods
enum Camera_Movement {
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT
};

// Default camera values
const float YAW         = -90.0f;
const float PITCH       = 0.0f;
const float SPEED       = 5.0f;
const float SENSITIVITY = 0.1f;
const float ZOOM        = 90.0f;
const float SPACE_THRESHOLD = 0.3f;   // max seconds between two space taps
const float G = 9.8f;
const float JUMP_SPEED = 4.0f;
const float GROUND_Y = 0.0f;


// An abstract camera class that processes input and calculates the corresponding Euler Angles, Vectors and Matrices for use in OpenGL
class Camera
{
public:
    // camera Attributes
    glm::vec3 Position_;
    glm::vec3 Front_;
    glm::vec3 Up_;
    glm::vec3 Right_;
    glm::vec3 WorldUp_;
    glm::vec3 WorldRight_;

    // euler Angles
    float Yaw_;
    float Pitch_;
    
    // camera options
    float MovementSpeed_;
    float MouseSensitivity_;
    float Zoom_;

    // flying mechanic
    float TimeLastSpace_;
    bool Flying_;
    bool SpaceWasPressed_;
    float VerticalSpeed_;

    // mouse last coordinates
    float LastX_;
    float LastY_;

    // constructor with vectors
    Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 3.0f), glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3 right = glm::vec3(1.0f, 0.0f, 0.0f), float yaw = YAW, float pitch = PITCH) : Front_(glm::vec3(0.0f, 0.0f, -1.0f)), MovementSpeed_(SPEED), MouseSensitivity_(SENSITIVITY), Zoom_(ZOOM)
    {
        Position_ = position;
        WorldUp_ = up;
        WorldRight_ = right;
        Yaw_ = yaw;
        Pitch_ = pitch;
        TimeLastSpace_ = -SPACE_THRESHOLD;
        Flying_  = true;
        SpaceWasPressed_ = false;
        VerticalSpeed_ = 0.0f;

        LastX_ = 0.0f;
        LastY_ = 0.0f;
        updateCameraVectors();
    }

    // returns the view matrix calculated using Euler Angles and the LookAt Matrix
    glm::mat4 GetViewMatrix()
    {
        return glm::lookAt(Position_, Position_ + Front_, Up_);
    }

    float GetZoom()      const noexcept {return Zoom_;}
    void SetSpeed(float Speed) noexcept {MovementSpeed_ = Speed;} 

    // processes input received from any keyboard-like input system. Accepts input parameter in the form of camera defined ENUM (to abstract it from windowing systems)
    void ProcessKeyboard(GLFWwindow *window, float deltaTime)
    {
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
            Flying_  = !Flying_;
            VerticalSpeed_ = 0.0f;
        } else if (SpaceWasPressed_ & Flying_)
            Position_ += WorldUp_ * velocity;


        if (Flying_)
            ProcessFlying(window, velocity);
        else {
            VerticalSpeed_ += G * deltaTime;
            Position_.y -= VerticalSpeed_ * deltaTime;
        }
    }

    // processes input received from a mouse input system. Expects the offset value in both the x and y Key.
    void ProcessMouseMovement(float xpos, float ypos)
    {
        float xoffset = xpos - LastX_;
        float yoffset = LastY_ - ypos; 
        LastX_ = xpos;
        LastY_ = ypos;

        xoffset *= MouseSensitivity_;
        yoffset *= MouseSensitivity_;

        Yaw_   += xoffset;
        Pitch_ += yoffset;

        // make sure that when pitch is out of bounds, screen doesn't get flipped
        if (Pitch_ > 89.0f)
            Pitch_ = 89.0f;
        if (Pitch_ < -89.0f)
            Pitch_ = -89.0f;

        // update Front, Right and Up Vectors using the updated Euler angles
        updateCameraVectors();
    }

    // processes input received from a mouse scroll-wheel event. Only requires input on the vertical wheel-axis
    void ProcessMouseScroll(float yoffset)
    {
        Zoom_ -= (float)yoffset;
        if (Zoom_ < 1.0f)
            Zoom_ = 1.0f;
        if (Zoom_ > 90.0f)
            Zoom_ = 90.0f;
    }

private:
    // true only on the frame when space is pressed the second time within SPACE_THRESHOLD seconds
    bool IsSpaceDoubleTapped(GLFWwindow *window, float time)
    {
        bool SpacePressed = glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;
        bool JustPressed  = SpacePressed && !SpaceWasPressed_;
        SpaceWasPressed_  = SpacePressed;

        if (!JustPressed)
            return false;

        if (time - TimeLastSpace_ < SPACE_THRESHOLD) {
            TimeLastSpace_ = -SPACE_THRESHOLD;   // so a third tap starts a new pair
            return true;
        }
        TimeLastSpace_ = time;
        return false;
    }

    // flight: space - up, shift - down, no gravity
    void ProcessFlying(GLFWwindow *window, float velocity)
    {
        if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
            Position_ -= WorldUp_ * velocity;
    }


    // calculates the front vector from the Camera's (updated) Euler Angles
    void updateCameraVectors()
    {
        // calculate the new Front vector
        glm::vec3 front;
        front.x = cos(glm::radians(Yaw_)) * cos(glm::radians(Pitch_));
        front.y = sin(glm::radians(Pitch_));
        front.z = sin(glm::radians(Yaw_)) * cos(glm::radians(Pitch_));

        Front_ = glm::normalize(front);
        // also re-calculate the Right and Up vector
        Right_ = glm::normalize(glm::cross(Front_, WorldUp_));
        Up_    = glm::normalize(glm::cross(Right_, Front_));
    }
};
#endif