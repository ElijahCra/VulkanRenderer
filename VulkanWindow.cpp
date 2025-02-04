//
// Created by Elijah Crain on 1/28/25.
//
#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <stdexcept>
#include <functional>

class VulkanWindow {
public:
    VulkanWindow(uint32_t width, uint32_t height, const char* title)
        : width(width), height(height), title(title)
    {
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

    GLFWwindow* getGLFWwindow() const {
        return window;
    }

    void setFramebufferResizeCallback(std::function<void(int, int)> callback) {
        framebufferResizeCallbackFn = callback;
    }

    void setKeyCallback(std::function<void(int, int)> callback) {
        keyCallbackFn = callback;
    }

    void setScrollCallback(std::function<void(double, double)> callback) {
        scrollCallbackFn = callback;
    }



private:
    uint32_t width;
    uint32_t height;
    const char* title;
    GLFWwindow* window = nullptr;

    std::function<void(int, int)> framebufferResizeCallbackFn;
    std::function<void(int, int)> keyCallbackFn;
    std::function<void(double, double)> scrollCallbackFn;

    static void framebufferResizeCallbackProxy(GLFWwindow* window, int newWidth, int newHeight) {
        auto self = reinterpret_cast<VulkanWindow*>(glfwGetWindowUserPointer(window));
        if (self->framebufferResizeCallbackFn) {
            self->framebufferResizeCallbackFn(newWidth, newHeight);
        }
    }

    static void keyCallbackProxy(GLFWwindow* window, int key, int scancode, int action, int mods) {
        auto self = reinterpret_cast<VulkanWindow*>(glfwGetWindowUserPointer(window));
        if (self->keyCallbackFn) {
            self->keyCallbackFn(key, action);
        }
    }

    static void scrollCallbackProxy(GLFWwindow* window, double xoffset, double yoffset) {
        auto self = reinterpret_cast<VulkanWindow*>(glfwGetWindowUserPointer(window));
        if (self->scrollCallbackFn) {
            self->scrollCallbackFn(xoffset, yoffset);
        }
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
        glfwSetKeyCallback(window, keyCallbackProxy);
        glfwSetScrollCallback(window, scrollCallbackProxy);
    }

    void cleanup() {
        glfwDestroyWindow(window);
        glfwTerminate();
    }
};
