#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "VulkanWindow.cpp"
#include "VulkanRenderer.cpp"

#include <iostream>
#include <cstdlib>
#include <cstdint>
#include <memory>
#include <glm/glm.hpp>

constexpr uint32_t WIDTH = 800;
constexpr uint32_t HEIGHT = 600;

class HelloTriangleApplication {
  public:
    void run() {
      vulkanWindow = std::make_unique<VulkanWindow>(WIDTH, HEIGHT, "Vulkan");
      renderer.init(vulkanWindow, WIDTH, HEIGHT);

      mainLoop();
    }

  private:
    std::shared_ptr<VulkanWindow> vulkanWindow;
    VulkanRenderer renderer{};

    //uint32_t mipLevels;
    //VkImage textureImage;

    void mainLoop() {
      while (!vulkanWindow->shouldClose()) {
        vulkanWindow->pollEvents();
        renderer.drawFrame();
      }

      vkDeviceWaitIdle(renderer.getDevice());
      renderer.cleanup();
    }
};

int main() {
  HelloTriangleApplication app;

  try {
    app.run();
  } catch (const std::exception &e) {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
