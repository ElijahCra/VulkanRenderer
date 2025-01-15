//
// Created by Elijah Crain on 11/30/24.
//

#include "Window.hpp"
#include <stdexcept>
#include <glm/ext/scalar_constants.hpp>

#include "Application.hpp"

class HelloTriangleApplication;
Window::Window(int width, int height, const char* title) : width(width), height(height) {
  initWindow(title);
}

Window::~Window() {
  glfwDestroyWindow(window);
  glfwTerminate();
}

void Window::initWindow(const char* title) {
  if (!glfwInit()) {
    throw std::runtime_error("Failed to initialize GLFW");
  }

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  window = glfwCreateWindow(width, height, title, nullptr, nullptr);

  if (!window) {
    throw std::runtime_error("Failed to create GLFW window");
  }

  glfwSetWindowUserPointer(window, this);
  glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
  glfwSetKeyCallback(window, keyCallback);
  glfwSetScrollCallback(window, scroll_callback);
}

GLFWwindow* Window::getGLFWwindow() const {
  return window;
}

bool Window::shouldClose() const {
  return glfwWindowShouldClose(window);
}

void Window::pollEvents() {
  glfwPollEvents();
}

void Window::framebufferResizeCallback(GLFWwindow* window, int width, int height) {
  auto app = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
  app->framebufferResized = true;
}

void Window::scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
  auto app = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
  app->handleScrollInput(xoffset, yoffset);
}

void Window::handleScrollInput(double xoffset, double yoffset) {
  const float fovIncrement = 1.0f;
  fov += fovIncrement*yoffset;
  if (fov > 45.0f) fov = 45.0f;
  if (fov < 1) fov = 1.0f;
}

void Window::keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
  auto app = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
  app->handleKeyInput(key, action);
}

void Window::handleKeyInput(int key, int action) {
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
