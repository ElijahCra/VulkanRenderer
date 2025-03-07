//
// Created by Elijah Crain on 2/10/25.
//
#pragma once
#define TINYOBJLOADER_IMPLEMENTATION
#include "VulkanWindow.cpp"
#include "VulkanInstance.cpp"
#include "VulkanDevice.cpp"
#include "VulkanSwapChain.cpp"
#include "VulkanRenderPass.cpp"
#include "VulkanPipeline.cpp"
#include "VulkanDescriptor.cpp"
#include "VulkanCommands.cpp"
#include "VulkanSync.cpp"

#include "Util.cpp"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <stdexcept>
#include <array>
#include <vector>
#include "tiny_object_loader.h" // Add tinyobjloader include

class VulkanRenderer {
  public:
    VulkanRenderer() = default;
    ~VulkanRenderer() {
    }

  void init(std::shared_ptr<VulkanWindow> window, uint32_t width, uint32_t height) {
      vulkanWindow = window;
      vulkanInstance = std::make_unique<VulkanInstance>(window->getGLFWwindow());

      vulkanDevice = std::make_shared<VulkanDevice>(
        vulkanInstance->getVkInstance(),
        vulkanInstance->getSurface()
      );

      vulkanSwapChain = std::make_shared<VulkanSwapChain>(
        vulkanDevice,
        vulkanInstance->getSurface(),
        width,
        height
      );

      vulkanRenderPass = std::make_unique<VulkanRenderPass>(
        vulkanDevice,
        vulkanSwapChain->getImageFormat(),
        vulkanDevice->getMsaaSamples(),
        vulkanSwapChain->findDepthFormat()
      );

      vulkanSwapChain->createFramebuffers(vulkanRenderPass->getHandle());

      vulkanDescriptors = std::make_unique<VulkanDescriptors>(vulkanDevice,
                                                              vulkanSwapChain,
                                                              window,
                                                              MAX_FRAMES_IN_FLIGHT);

      vulkanPipeline = std::make_unique<VulkanPipeline>(
        vulkanDevice->getDevice(),
        vulkanRenderPass->getHandle(),
        vulkanDescriptors->getDescriptorSetLayout(),
        vulkanDevice->getMsaaSamples(),
        "../shaders/vert.spv",
        "../shaders/frag.spv"
      );

      vulkanCommands = std::make_unique<VulkanCommands>(
        vulkanDevice,
        MAX_FRAMES_IN_FLIGHT
      );

      // Load the STL model
      loadModel("../models/terrain.obj");
      createVertexBuffer(vertices, vertexBuffer, vertexBufferMemory);
      createIndexBuffer(indices, indexBuffer, indexBufferMemory);

      // We're intentionally NOT calling prepareInstanceData() here
      // Instead, we'll set instanceBuffer to NULL and handle it in recordCommandBuffer

      vulkanSync = std::make_unique<VulkanSync>(
        vulkanDevice,
        MAX_FRAMES_IN_FLIGHT
      );

      this->width = width;
      this->height = height;
    }

void cleanup() {
  auto vkDev = vulkanDevice->getDevice();

  vkDestroyBuffer(vkDev, vertexBuffer, nullptr);
  vkFreeMemory(vkDev, vertexBufferMemory, nullptr);

  vkDestroyBuffer(vkDev, indexBuffer, nullptr);
  vkFreeMemory(vkDev, indexBufferMemory, nullptr);

  // We're not creating the instance buffer, so we don't need to clean it up
  // If this causes issues, ensure instanceBuffer is initially set to VK_NULL_HANDLE
  // and only destroy if it's not null

  vulkanSync.reset();
  vulkanCommands.reset();
  vulkanPipeline.reset();
  vulkanDescriptors.reset();
  vulkanRenderPass.reset();
  vulkanSwapChain.reset();

  vulkanDevice.reset();
  vulkanInstance.reset();
}

void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex, uint32_t currentFrame) {
  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

  if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
    throw std::runtime_error("failed to begin recording command buffer!");
  }

  VkRenderPassBeginInfo renderPassInfo{};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  renderPassInfo.renderPass = vulkanRenderPass->getHandle();
  renderPassInfo.framebuffer = vulkanSwapChain->getFramebuffers()[imageIndex];
  renderPassInfo.renderArea.offset = {0, 0};
  renderPassInfo.renderArea.extent = vulkanSwapChain->getExtent();

  std::array<VkClearValue, 2> clearValues{};
  clearValues[0].color = {{0.1f, 0.1f, 0.1f, 1.0f}};
  clearValues[1].depthStencil = {1.0f, 0};

  renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
  renderPassInfo.pClearValues = clearValues.data();

  vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vulkanPipeline->getPipeline());

  vkCmdBindDescriptorSets(commandBuffer,
                          VK_PIPELINE_BIND_POINT_GRAPHICS,
                          vulkanPipeline->getLayout(),
                          0,
                          1,
                          &vulkanDescriptors->getDescriptorSets()[currentFrame],
                          0,
                          nullptr);

  VkViewport viewport{};
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.width = (float) vulkanSwapChain->getExtent().width;
  viewport.height = (float) vulkanSwapChain->getExtent().height;
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;
  vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

  VkRect2D scissor{};
  scissor.offset = {0, 0};
  scissor.extent = vulkanSwapChain->getExtent();
  vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

  VkDeviceSize offsets[] = {0};

  // Only bind the vertex buffer
  vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer, offsets);

  // Bind a dummy buffer for instance data (needed by our pipeline)
  // This is necessary because our pipeline still expects this binding
  static VkBuffer dummyBuffer = VK_NULL_HANDLE;
  if (dummyBuffer == VK_NULL_HANDLE) {
    // Create a minimal dummy buffer if needed
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = sizeof(InstanceData); // Just one instance
    bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkBuffer tempBuffer;
    VkDeviceMemory tempMemory;
    vulkanDevice->createBuffer(
      bufferInfo.size,
      bufferInfo.usage,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
      tempBuffer,
      tempMemory
    );

    // Initialize with a zero offset
    void* data;
    vkMapMemory(vulkanDevice->getDevice(), tempMemory, 0, bufferInfo.size, 0, &data);
    InstanceData inst{};
    inst.offset = glm::vec2(0.0f, 0.0f);
    memcpy(data, &inst, sizeof(InstanceData));
    vkUnmapMemory(vulkanDevice->getDevice(), tempMemory);

    dummyBuffer = tempBuffer;
  }

  vkCmdBindVertexBuffers(commandBuffer, 1, 1, &dummyBuffer, offsets);

  // Bind index buffer
  vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT16);

  // Draw with exactly 1 instance
  vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);

  vkCmdEndRenderPass(commandBuffer);

  if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
    throw std::runtime_error("failed to record command buffer!");
  }
}

    void drawFrame() {
      vkWaitForFences(vulkanDevice->getDevice(), 1, vulkanSync->getInFlightFence(currentFrame), VK_TRUE, UINT64_MAX);

      uint32_t imageIndex;
      VkResult result =
        vkAcquireNextImageKHR(vulkanDevice->getDevice(),
                              vulkanSwapChain->getSwapChain(),
                              UINT64_MAX,
                              vulkanSync->getImageAvailableSemaphore(currentFrame),
                              VK_NULL_HANDLE,
                              &imageIndex);

      if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        recreateSwapChain();
        return;
      } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("failed to acquire swap chain image!");
      }
      vulkanDescriptors->updateUniformBuffer(currentFrame);
      vkResetFences(vulkanDevice->getDevice(), 1, vulkanSync->getInFlightFence(currentFrame));

      vkResetCommandBuffer(vulkanCommands->getCommandBuffers()[currentFrame], /*VkCommandBufferResetFlagBits*/ 0);
      recordCommandBuffer(vulkanCommands->getCommandBuffers()[currentFrame], imageIndex, currentFrame);

      VkSubmitInfo submitInfo{};
      submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

      VkSemaphore waitSemaphores[] = {vulkanSync->getImageAvailableSemaphore(currentFrame)};
      VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
      submitInfo.waitSemaphoreCount = 1;
      submitInfo.pWaitSemaphores = waitSemaphores;
      submitInfo.pWaitDstStageMask = waitStages;

      submitInfo.commandBufferCount = 1;
      submitInfo.pCommandBuffers = &vulkanCommands->getCommandBuffers()[currentFrame];

      VkSemaphore signalSemaphores[] = {vulkanSync->getRenderFinishedSemaphore(currentFrame)};
      submitInfo.signalSemaphoreCount = 1;
      submitInfo.pSignalSemaphores = signalSemaphores;

      if (vkQueueSubmit(vulkanDevice->getGraphicsQueue(),
                        1,
                        &submitInfo,
                        *vulkanSync->getInFlightFence(currentFrame)) != VK_SUCCESS) {
        throw std::runtime_error("failed to submit draw command buffer!");
      }

      VkPresentInfoKHR presentInfo{};
      presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

      presentInfo.waitSemaphoreCount = 1;
      presentInfo.pWaitSemaphores = signalSemaphores;

      VkSwapchainKHR swapChains[] = {vulkanSwapChain->getSwapChain()};
      presentInfo.swapchainCount = 1;
      presentInfo.pSwapchains = swapChains;

      presentInfo.pImageIndices = &imageIndex;

      result = vkQueuePresentKHR(vulkanDevice->getPresentQueue(), &presentInfo);

      if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || vulkanWindow->framebufferResized) {
        vulkanWindow->framebufferResized = false;
        recreateSwapChain();
      } else if (result != VK_SUCCESS) {
        throw std::runtime_error("failed to present swap chain image!");
      }

      currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    }

    void recreateSwapChain() {
      int width = 0, height = 0;
      glfwGetFramebufferSize(vulkanWindow->getGLFWwindow(), &width, &height);
      while (width == 0 || height == 0) {
        glfwGetFramebufferSize(vulkanWindow->getGLFWwindow(), &width, &height);
        glfwWaitEvents();
      }
      vkDeviceWaitIdle(vulkanDevice->getDevice());

      vulkanSwapChain->recreate(width, height);

      vulkanRenderPass->createRenderPass(vulkanSwapChain->getImageFormat(),
                                         vulkanDevice->getMsaaSamples(),
                                         vulkanSwapChain->findDepthFormat());
      vulkanSwapChain->createFramebuffers(vulkanRenderPass->getHandle());
      vulkanPipeline->createGraphicsPipeline(vulkanRenderPass->getHandle(),
                                             vulkanDevice->getMsaaSamples(),
                                             "../shaders/vert.spv",
                                             "../shaders/frag.spv");
    }

    void setFramebufferResized(bool resized) { framebufferResized = resized; }
    void setNewExtent(uint32_t w, uint32_t h) {
      width = w;
      height = h;
    }

    VkDevice getDevice() const { return vulkanDevice->getDevice(); }

  private:
    std::unique_ptr<VulkanInstance> vulkanInstance;
    std::shared_ptr<VulkanDevice> vulkanDevice;
    std::shared_ptr<VulkanSwapChain> vulkanSwapChain;
    std::unique_ptr<VulkanRenderPass> vulkanRenderPass;
    std::unique_ptr<VulkanPipeline> vulkanPipeline;
    std::unique_ptr<VulkanDescriptors> vulkanDescriptors;
    std::unique_ptr<VulkanCommands> vulkanCommands;
    std::unique_ptr<VulkanSync> vulkanSync;
    std::shared_ptr<VulkanWindow> vulkanWindow;

    static constexpr int MAX_FRAMES_IN_FLIGHT = 2;
    uint32_t currentFrame = 0;
    bool framebufferResized = false;

    uint32_t width = 800;
    uint32_t height = 600;

    // Model data
    std::vector<Vertex> vertices;
    std::vector<uint16_t> indices;
    VkBuffer vertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory vertexBufferMemory = VK_NULL_HANDLE;
    VkBuffer indexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory indexBufferMemory = VK_NULL_HANDLE;

    // Instance data
    std::vector<InstanceData> instanceData;
    VkBuffer instanceBuffer = VK_NULL_HANDLE;
    VkDeviceMemory instanceBufferMemory = VK_NULL_HANDLE;

    void loadModel(const std::string& modelPath) {
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    if (!LoadObj(&attrib, &shapes, &materials, &warn, &err, modelPath.c_str())) {
        throw std::runtime_error(warn + err);
    }

    // Generate vertex and index data from the loaded model
    std::unordered_map<Vertex, uint32_t> uniqueVertices{};

    for (const auto& shape : shapes) {
        for (const auto& index : shape.mesh.indices) {
            Vertex vertex{};

            // Position
            vertex.pos = {
                attrib.vertices[3 * index.vertex_index + 0],
                attrib.vertices[3 * index.vertex_index + 1],
                attrib.vertices[3 * index.vertex_index + 2]
            };

            // Set a default color if there are no colors in the model
            // You can use normals to generate colors
            if (index.normal_index >= 0) {
                float nx = attrib.normals[3 * index.normal_index + 0];
                float ny = attrib.normals[3 * index.normal_index + 1];
                float nz = attrib.normals[3 * index.normal_index + 2];

                // Convert normal to color (normalize to 0-1 range)
                vertex.color = {
                    (nx + 1.0f) * 0.5f,
                    (ny + 1.0f) * 0.5f,
                    (nz + 1.0f) * 0.5f
                };
            } else {
                // Default color if no normal
                vertex.color = {0.8f, 0.8f, 0.8f};
            }

            // Add to vertices and indices
            if (uniqueVertices.count(vertex) == 0) {
                uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
                vertices.push_back(vertex);
            }

            indices.push_back(static_cast<uint16_t>(uniqueVertices[vertex]));
        }
    }

    // Check if we need to truncate indices if they exceed uint16_t limit
    if (vertices.size() > UINT16_MAX) {
        throw std::runtime_error("Model has too many vertices for uint16_t indices!");
    }

    std::cout << "Loaded model with " << vertices.size() << " vertices and "
              << indices.size() << " indices" << std::endl;
}

    void createVertexBuffer(const std::vector<Vertex>& vertices,
                            VkBuffer& vertexBuffer,
                            VkDeviceMemory& vertexBufferMemory) {
      VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

      VkBuffer stagingBuffer;
      VkDeviceMemory stagingBufferMemory;
      vulkanDevice->createBuffer(
        bufferSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        stagingBuffer,
        stagingBufferMemory
      );

      void* data;
      vkMapMemory(vulkanDevice->getDevice(), stagingBufferMemory, 0, bufferSize, 0, &data);
      memcpy(data, vertices.data(), (size_t)bufferSize);
      vkUnmapMemory(vulkanDevice->getDevice(), stagingBufferMemory);

      vulkanDevice->createBuffer(
        bufferSize,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        vertexBuffer,
        vertexBufferMemory
      );

      vulkanDevice->copyBuffer(stagingBuffer, vertexBuffer, bufferSize);

      vkDestroyBuffer(vulkanDevice->getDevice(), stagingBuffer, nullptr);
      vkFreeMemory(vulkanDevice->getDevice(), stagingBufferMemory, nullptr);
    }

    void createIndexBuffer(const std::vector<uint16_t>& indices,
                           VkBuffer& indexBuffer,
                           VkDeviceMemory& indexBufferMemory) {
      VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

      VkBuffer stagingBuffer;
      VkDeviceMemory stagingBufferMemory;
      vulkanDevice->createBuffer(
        bufferSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        stagingBuffer,
        stagingBufferMemory
      );

      void* data;
      vkMapMemory(vulkanDevice->getDevice(), stagingBufferMemory, 0, bufferSize, 0, &data);
      memcpy(data, indices.data(), (size_t)bufferSize);
      vkUnmapMemory(vulkanDevice->getDevice(), stagingBufferMemory);

      vulkanDevice->createBuffer(
        bufferSize,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        indexBuffer,
        indexBufferMemory
      );

      vulkanDevice->copyBuffer(stagingBuffer, indexBuffer, bufferSize);

      vkDestroyBuffer(vulkanDevice->getDevice(), stagingBuffer, nullptr);
      vkFreeMemory(vulkanDevice->getDevice(), stagingBufferMemory, nullptr);
    }

    void createInstanceBuffer(const std::vector<InstanceData>& instanceData,
                              VkBuffer& buffer,
                              VkDeviceMemory& bufferMemory) {
      VkDeviceSize bufferSize = sizeof(InstanceData) * instanceData.size();
      vulkanDevice->createBuffer(bufferSize,
                                 VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                 buffer,
                                 bufferMemory);

      void* data;
      vkMapMemory(vulkanDevice->getDevice(), bufferMemory, 0, bufferSize, 0, &data);
      memcpy(data, instanceData.data(), (size_t)bufferSize);
      vkUnmapMemory(vulkanDevice->getDevice(), bufferMemory);
    }

    void prepareInstanceData() {
      // Define a grid layout for instances of the model
      const int GRID_SIZE = 3; // 3x3 grid
      const float SPACING = 3.0f; // Space between instances

      for (int z = 0; z < GRID_SIZE; z++) {
        for (int x = 0; x < GRID_SIZE; x++) {
          InstanceData inst{};

          // Calculate offset in grid
          float xOffset = (x - (GRID_SIZE - 1) / 2.0f) * SPACING;
          float zOffset = (z - (GRID_SIZE - 1) / 2.0f) * SPACING;

          inst.offset = glm::vec2(xOffset, zOffset);
          instanceData.push_back(inst);
        }
      }

      createInstanceBuffer(instanceData, instanceBuffer, instanceBufferMemory);
    }
};