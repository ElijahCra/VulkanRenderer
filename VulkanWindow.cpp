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
      fov += fovIncrement * yoffset;
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
        }
      }
    }

    float cameraAngleX = 0.0f;
    float cameraAngleY = glm::radians(90.0f);
    float radius = 20.0f;
    float fov = 5.0f;
    bool framebufferResized = false;

  private:
    uint32_t width;
    uint32_t height;
    const char *title;
    GLFWwindow *window = nullptr;

    std::function<void(int, int)> framebufferResizeCallbackFn;
    std::function<void(int, int)> keyCallbackFn;
    std::function<void(double, double)> scrollCallbackFn;

    static void framebufferResizeCallbackProxy(GLFWwindow *window, int newWidth, int newHeight) {
      auto self = reinterpret_cast<VulkanWindow *>(glfwGetWindowUserPointer(window));
      if (self->framebufferResizeCallbackFn) {
        self->framebufferResizeCallbackFn(newWidth, newHeight);
      }
    }
    static void scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
      auto app = reinterpret_cast<VulkanWindow *>(glfwGetWindowUserPointer(window));
      app->handleScrollInput(xoffset, yoffset);
    }
    static void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods) {
      auto app = reinterpret_cast<VulkanWindow *>(glfwGetWindowUserPointer(window));
      app->handleKeyInput(key, action);
    }
    static void framebufferResizeCallback(GLFWwindow *window, int width, int height) {
      auto app = reinterpret_cast<VulkanWindow *>(glfwGetWindowUserPointer(window));
      app->framebufferResized = true;
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

      glfwSetFramebufferSizeCallback(window, framebufferResizeCallbackProxy);
      glfwSetKeyCallback(window, keyCallback);
      glfwSetScrollCallback(window, scroll_callback);
    }

    void cleanup() {
      glfwDestroyWindow(window);
      glfwTerminate();
    }
};
