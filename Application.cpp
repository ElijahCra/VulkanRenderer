//
// Created by Elijah Crain on 11/30/24.
//

#include "Application.hpp"

void Application::run() {
  window = new Window(800, 600, "Vulkan Application");
  initVulkan();
  mainLoop();
  cleanup();
}

void Application::initVulkan() {
  instance = new Instance();
  device = new Device(instance, window);
  swapChain = new SwapChain(device, window);
  pipeline = new Pipeline(device, swapChain);
  commandPool = new CommandPool(device);
  synchronization = new Synchronization(device);
}

void Application::mainLoop() {
  while (!glfwWindowShouldClose(window->getGLFWwindow())) {
    glfwPollEvents();
    drawFrame();
  }

  vkDeviceWaitIdle(device->getLogical());
}

void Application::cleanup() {
  delete synchronization;
  delete commandPool;
  delete pipeline;
  delete swapChain;
  delete device;
  delete instance;
  delete window;
}

    void cleanup() {
        cleanupSwapChain();

      vkDestroyBuffer(device, edgeVertexBuffer, nullptr);
      vkFreeMemory(device, edgeVertexBufferMemory, nullptr);
      vkDestroyBuffer(device, edgeIndexBuffer, nullptr);
      vkFreeMemory(device, edgeIndexBufferMemory, nullptr);
      vkDestroyBuffer(device, internalVertexBuffer, nullptr);
      vkFreeMemory(device, internalVertexBufferMemory, nullptr);
      vkDestroyBuffer(device, internalIndexBuffer, nullptr);
      vkFreeMemory(device, internalIndexBufferMemory, nullptr);

      vkDestroyBuffer(device, edgeInstanceBuffer, nullptr);
      vkFreeMemory(device, edgeInstanceBufferMemory, nullptr);
      vkDestroyBuffer(device, internalInstanceBuffer, nullptr);
      vkFreeMemory(device, internalInstanceBufferMemory, nullptr);

        vkDestroyPipeline(device, graphicsPipeline, nullptr);
        vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
        vkDestroyRenderPass(device, renderPass, nullptr);

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            vkDestroyBuffer(device, uniformBuffers[i], nullptr);
            vkFreeMemory(device, uniformBuffersMemory[i], nullptr);
        }

        vkDestroyDescriptorPool(device, descriptorPool, nullptr);

        vkDestroySampler(device, textureSampler, nullptr);
        vkDestroyImageView(device, textureImageView, nullptr);

        vkDestroyImage(device, textureImage, nullptr);
        vkFreeMemory(device, textureImageMemory, nullptr);

        vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
            vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
            vkDestroyFence(device, inFlightFences[i], nullptr);
        }

        vkDestroyCommandPool(device, commandPool, nullptr);

        vkDestroyDevice(device, nullptr);

        if (enableValidationLayers) {
            DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
        }

        vkDestroySurfaceKHR(instance, surface, nullptr);
        vkDestroyInstance(instance, nullptr);

        glfwDestroyWindow(window);

        glfwTerminate();
    }
