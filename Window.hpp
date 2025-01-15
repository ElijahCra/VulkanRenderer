//
// Created by Elijah Crain on 11/30/24.
//

#ifndef WINDOW_HPP
#define WINDOW_HPP

#include <GLFW/glfw3.h>
#include <glm/detail/func_trigonometric.inl>

class Window {
  public:
  Window(int width, int height, const char* title);
  ~Window();

  GLFWwindow* getGLFWwindow() const;
  bool shouldClose() const;
  void pollEvents();

  static void framebufferResizeCallback(GLFWwindow* window, int width, int height);

  bool framebufferResized = false;

  private:
  GLFWwindow* window;
  int width, height;

  float cameraAngleX = 0.0f;
  float cameraAngleY = glm::radians(90.0f);
  float radius = 20.0f;
  float fov = 45.0f;

  void initWindow(const char* title);
  static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
  void handleScrollInput(double xoffset, double yoffset);
  static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
  void handleKeyInput(int key, int action);
};

#endif // WINDOW_HPP

