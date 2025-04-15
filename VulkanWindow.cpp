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
      fov += fovIncrement * yoffset;
      if (fov > 45.0f) fov = 45.0f;
      if (fov < 1) fov = 1.0f;
    }

  void handleKeyInput(int key, int action) {
      if (action == GLFW_PRESS || action == GLFW_REPEAT) {
        constexpr float angleIncrement = 0.1f;

        // Camera rotation with arrow keys
        if (key == GLFW_KEY_LEFT && !freeCameraMode) {
          cameraAngleX -= angleIncrement;
        } else if (key == GLFW_KEY_RIGHT && !freeCameraMode) {
          cameraAngleX += angleIncrement;
        } else if (key == GLFW_KEY_UP && !freeCameraMode) {
          cameraAngleY += angleIncrement;
          if (cameraAngleY > glm::pi<float>() - 0.01f) cameraAngleY = glm::pi<float>() - 0.01f;
        } else if (key == GLFW_KEY_DOWN && !freeCameraMode) {
          cameraAngleY -= angleIncrement;
          if (cameraAngleY < 0.01f) cameraAngleY = 0.01f;
        }

        // Free camera movement key tracking
        else if (key == GLFW_KEY_W) {
          keyStates[FORWARD] = true;
        } else if (key == GLFW_KEY_S) {
          keyStates[BACKWARD] = true;
        } else if (key == GLFW_KEY_A) {
          keyStates[LEFT] = true;
        } else if (key == GLFW_KEY_D) {
          keyStates[RIGHT] = true;
        } else if (key == GLFW_KEY_SPACE) {
          if (freeCameraMode) {
            keyStates[UP] = true;
          } else {
            followPath = !followPath;
          }
        } else if (key == GLFW_KEY_LEFT_SHIFT) {
          keyStates[DOWN] = true;
        } else if (key == GLFW_KEY_R) {
          resetCamera();
        } else if (key == GLFW_KEY_F) {
          // Toggle free camera mode with F key (in addition to toolbar button)
          freeCameraMode = !freeCameraMode;
          // Reset key states when toggling modes
          for (int i = 0; i < 6; i++) {
            keyStates[i] = false;
          }
        }
      } else if (action == GLFW_RELEASE) {
        // Handle key releases for continuous movement
        if (key == GLFW_KEY_W) {
          keyStates[FORWARD] = false;
        } else if (key == GLFW_KEY_S) {
          keyStates[BACKWARD] = false;
        } else if (key == GLFW_KEY_A) {
          keyStates[LEFT] = false;
        } else if (key == GLFW_KEY_D) {
          keyStates[RIGHT] = false;
        } else if (key == GLFW_KEY_SPACE) {
          keyStates[UP] = false;
        } else if (key == GLFW_KEY_LEFT_SHIFT) {
          keyStates[DOWN] = false;
        }
      }
    }

    void handleMouseButton(int button, int action, int mods) {
      if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
          double xpos, ypos;
          glfwGetCursorPos(window, &xpos, &ypos);

          // Check if click is in toolbar area
          if (ypos < TOOLBAR_HEIGHT) {
            // Check if click is on the toggle button
            if (xpos >= TOGGLE_BUTTON_X && xpos <= TOGGLE_BUTTON_X + TOGGLE_BUTTON_WIDTH &&
                ypos >= TOGGLE_BUTTON_Y && ypos <= TOGGLE_BUTTON_Y + TOGGLE_BUTTON_HEIGHT) {
              // Toggle free camera mode
              freeCameraMode = !freeCameraMode;
              // Reset key states when toggling modes
              for (int i = 0; i < 6; i++) {
                keyStates[i] = false;
              }
            }
          } else {
            // Regular camera dragging
            dragging = true;
            lastMouseX = xpos;
            lastMouseY = ypos;
          }
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

        // Adjust for Z-up system
        cameraRotationOffsetX -= dx * sensitivity;
        cameraRotationOffsetY -= dy * sensitivity;

        // Clamp vertical rotation to avoid gimbal lock
        cameraRotationOffsetY = glm::clamp(cameraRotationOffsetY,
                                          -glm::half_pi<float>() + 0.1f,
                                          glm::half_pi<float>() - 0.1f);

        lastMouseX = xpos;
        lastMouseY = ypos;
      }
    }

  void resetCamera() {
      cameraAngleX = 0.0f;
      cameraAngleY = glm::radians(90.0f);  // 90 degrees for looking horizontally in Z-up system
      cameraRotationOffsetX = 0.0f;
      cameraRotationOffsetY = 0.0f;
      radius = 20.0f;
      fov = 30.0f;
      freeCameraMode = false;
      for (int i = 0; i < 6; i++) {
        keyStates[i] = false;
      }
    }

    void updateTitle(float fps) {
      std::string modeStr = freeCameraMode ? "Free Camera" : (followPath ? "Path Camera" : "Static Camera");
      std::string newTitle = std::string(title) + " - " + modeStr + " - FPS: " + std::to_string(static_cast<int>(fps));
      glfwSetWindowTitle(window, newTitle.c_str());
    }

    // Toolbar rendering
    void renderToolbar() {
      // Set the viewport for the toolbar
      glViewport(0, height - TOOLBAR_HEIGHT, width, TOOLBAR_HEIGHT);

      // Draw toolbar background
      glBegin(GL_QUADS);
      glColor3f(0.2f, 0.2f, 0.2f); // Dark gray background
      glVertex2f(0, 0);
      glVertex2f(width, 0);
      glVertex2f(width, TOOLBAR_HEIGHT);
      glVertex2f(0, TOOLBAR_HEIGHT);
      glEnd();

      // Draw toggle button
      glBegin(GL_QUADS);
      if (freeCameraMode) {
        glColor3f(0.2f, 0.6f, 0.2f); // Green when free camera is on
      } else {
        glColor3f(0.6f, 0.2f, 0.2f); // Red when free camera is off
      }
      glVertex2f(TOGGLE_BUTTON_X, TOGGLE_BUTTON_Y);
      glVertex2f(TOGGLE_BUTTON_X + TOGGLE_BUTTON_WIDTH, TOGGLE_BUTTON_Y);
      glVertex2f(TOGGLE_BUTTON_X + TOGGLE_BUTTON_WIDTH, TOGGLE_BUTTON_Y + TOGGLE_BUTTON_HEIGHT);
      glVertex2f(TOGGLE_BUTTON_X, TOGGLE_BUTTON_Y + TOGGLE_BUTTON_HEIGHT);
      glEnd();

      // Reset the viewport
      glViewport(0, 0, width, height);
    }

    // Camera movement enum
    enum CameraMovement {
      FORWARD = 0,
      BACKWARD,
      LEFT,
      RIGHT,
      UP,
      DOWN
    };

    // Camera settings
    float cameraAngleX = 0.0f;
    float cameraAngleY = glm::radians(90.0f);
    float radius = 20.0f;
    float fov = 5.0f;
    bool framebufferResized = false;

    // Free camera mode
    bool freeCameraMode = false;
    bool keyStates[6] = { false }; // WASD + Space + Shift
    float cameraSpeed = 0.1f;      // Base camera movement speed

    // Toolbar constants
    static const int TOOLBAR_HEIGHT = 30;
    static const int TOGGLE_BUTTON_X = 10;
    static const int TOGGLE_BUTTON_Y = 5;
    static const int TOGGLE_BUTTON_WIDTH = 120;
    static const int TOGGLE_BUTTON_HEIGHT = 20;

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
