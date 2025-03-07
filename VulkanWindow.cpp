//
// Created by Elijah Crain on 1/28/25.
//
#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <stdexcept>
#include <functional>
#include <glm/glm.hpp>
#include <glm/ext/scalar_constants.hpp>
#include <string>
#include <glm/gtc/constants.hpp>

#include "FpsCounter.cpp"

class VulkanWindow {
  public:
    VulkanWindow(uint32_t width, uint32_t height, const char *title)
      : width(width), height(height), title(title) {
      initWindow();
    }

    ~VulkanWindow() {
      cleanup();
    }

    void pollEvents() {
      glfwPollEvents();
    }

    bool shouldClose() const {
      return glfwWindowShouldClose(window);
    }

    GLFWwindow *getGLFWwindow() const {
      return window;
    }

    void handleScrollInput(double xoffset, double yoffset) {
      const float fovIncrement = 1.0f;
      fov += fovIncrement * -yoffset; // Inverted for more intuitive zoom
      if (fov > 45.0f) fov = 45.0f;
      if (fov < 1) fov = 1.0f;
    }

    void handleKeyInput(int key, int action) {
      if (action == GLFW_PRESS || action == GLFW_REPEAT) {
        constexpr float angleIncrement = 0.1f;

        if (key == GLFW_KEY_LEFT) {
          cameraAngleX -= angleIncrement;
        } else if (key == GLFW_KEY_RIGHT) {
          cameraAngleX += angleIncrement;
        } else if (key == GLFW_KEY_UP) {
          cameraAngleY += angleIncrement;
          if (cameraAngleY > glm::pi<float>() - 0.01f) cameraAngleY = glm::pi<float>() - 0.01f;
        } else if (key == GLFW_KEY_DOWN) {
          cameraAngleY -= angleIncrement;
          if (cameraAngleY < 0.01f) cameraAngleY = 0.01f;
        } else if (key == GLFW_KEY_SPACE) {
          followPath = !followPath; // Toggle path following
        } else if (key == GLFW_KEY_R) {
          resetCamera(); // Reset camera to default
        }
      }
    }

    void handleMouseButton(int button, int action, int mods) {
      if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
          dragging = true;
          glfwGetCursorPos(window, &lastMouseX, &lastMouseY);
        } else if (action == GLFW_RELEASE) {
          dragging = false;
        }
      }
    }

  void handleMouseMove(double xpos, double ypos) {
      if (dragging) {
        // Calculate mouse delta
        double dx = xpos - lastMouseX;
        double dy = ypos - lastMouseY;

        // Apply rotation offset (adjust sensitivity as needed)
        const float sensitivity = 0.005f;

        // Changed the direction to make mouse movement more intuitive
        cameraRotationOffsetX -= dx * sensitivity;
        cameraRotationOffsetY -= dy * sensitivity;

        // Clamp vertical rotation to avoid gimbal lock
        // Extended the range slightly to allow more freedom of movement
        cameraRotationOffsetY = glm::clamp(cameraRotationOffsetY,
                                           -glm::half_pi<float>() + 0.1f,
                                           glm::half_pi<float>() - 0.1f);

        lastMouseX = xpos;
        lastMouseY = ypos;

        // Debug output (uncomment if needed)
        // std::cout << "Rotation Offsets: (" << cameraRotationOffsetX << ", " << cameraRotationOffsetY << ")\n";
      }
    }

  void resetCamera() {
      cameraAngleX = 0.0f;
      cameraAngleY = glm::radians(90.0f);
      cameraRotationOffsetX = 0.0f;
      cameraRotationOffsetY = 0.0f;
      radius = 20.0f;
      fov = 30.0f;
    }

    void updateTitle(float fps) {
      std::string newTitle = std::string(title) + " - FPS: " + std::to_string(static_cast<int>(fps));
      glfwSetWindowTitle(window, newTitle.c_str());
    }

    float cameraAngleX = 0.0f;
    float cameraAngleY = glm::radians(90.0f);
    float cameraRotationOffsetX = 0.0f;
    float cameraRotationOffsetY = 0.0f;
    float radius = 20.0f;
    float fov = 30.0f;
    bool framebufferResized = false;
    bool followPath = true;
    bool dragging = false;
    double lastMouseX = 0.0, lastMouseY = 0.0;

  private:
    uint32_t width;
    uint32_t height;
    const char *title;
    GLFWwindow *window = nullptr;

    std::function<void(int, int)> framebufferResizeCallbackFn;
    std::function<void(int, int, int)> mouseButtonCallbackFn;
    std::function<void(double, double)> cursorPosCallbackFn;

    static void framebufferResizeCallbackProxy(GLFWwindow *window, int newWidth, int newHeight) {
      auto self = reinterpret_cast<VulkanWindow *>(glfwGetWindowUserPointer(window));
      self->framebufferResized = true;
    }

    static void scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
      auto app = reinterpret_cast<VulkanWindow *>(glfwGetWindowUserPointer(window));
      app->handleScrollInput(xoffset, yoffset);
    }

    static void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods) {
      auto app = reinterpret_cast<VulkanWindow *>(glfwGetWindowUserPointer(window));
      app->handleKeyInput(key, action);
    }

    static void mouseButtonCallback(GLFWwindow *window, int button, int action, int mods) {
      auto app = reinterpret_cast<VulkanWindow *>(glfwGetWindowUserPointer(window));
      app->handleMouseButton(button, action, mods);
    }

    static void cursorPosCallback(GLFWwindow *window, double xpos, double ypos) {
      auto app = reinterpret_cast<VulkanWindow *>(glfwGetWindowUserPointer(window));
      app->handleMouseMove(xpos, ypos);
    }

    void initWindow() {
      if (!glfwInit()) {
        throw std::runtime_error("Failed to initialize GLFW!");
      }
      glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

      window = glfwCreateWindow(width, height, title, nullptr, nullptr);
      if (!window) {
        throw std::runtime_error("Failed to create GLFW window!");
      }

      glfwSetWindowUserPointer(window, this);

      // Set up callbacks
      glfwSetFramebufferSizeCallback(window, framebufferResizeCallbackProxy);
      glfwSetKeyCallback(window, keyCallback);
      glfwSetScrollCallback(window, scroll_callback);
      glfwSetMouseButtonCallback(window, mouseButtonCallback);
      glfwSetCursorPosCallback(window, cursorPosCallback);
    }

    void cleanup() {
      glfwDestroyWindow(window);
      glfwTerminate();
    }
};