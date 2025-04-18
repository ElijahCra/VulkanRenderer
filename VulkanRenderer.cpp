//
// Created by Elijah Crain on 2/10/25.
//
#pragma once

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

      generateHexagonData();
      createVertexBuffer(edgeVertices, edgeVertexBuffer, edgeVertexBufferMemory);
      createIndexBuffer(edgeIndices, edgeIndexBuffer, edgeIndexBufferMemory);
      createVertexBuffer(internalVertices, internalVertexBuffer, internalVertexBufferMemory);
      createIndexBuffer(internalIndices, internalIndexBuffer, internalIndexBufferMemory);
      prepareInstanceData();

      vulkanSync = std::make_unique<VulkanSync>(
        vulkanDevice,
        MAX_FRAMES_IN_FLIGHT
      );

      this->width = width;
      this->height = height;
    }

    void cleanup() {
      auto vkDev = vulkanDevice->getDevice();

      vkDestroyBuffer(vkDev, edgeVertexBuffer, nullptr);
      vkFreeMemory(vkDev, edgeVertexBufferMemory, nullptr);

      vkDestroyBuffer(vkDev, edgeIndexBuffer, nullptr);
      vkFreeMemory(vkDev, edgeIndexBufferMemory, nullptr);

      vkDestroyBuffer(vkDev, internalVertexBuffer, nullptr);
      vkFreeMemory(vkDev, internalVertexBufferMemory, nullptr);

      vkDestroyBuffer(vkDev, internalIndexBuffer, nullptr);
      vkFreeMemory(vkDev, internalIndexBufferMemory, nullptr);

      vkDestroyBuffer(vkDev, edgeInstanceBuffer, nullptr);
      vkFreeMemory(vkDev, edgeInstanceBufferMemory, nullptr);

      vkDestroyBuffer(vkDev, internalInstanceBuffer, nullptr);
      vkFreeMemory(vkDev, internalInstanceBufferMemory, nullptr);

      vulkanSync.reset();
      vulkanCommands.reset();
      vulkanPipeline.reset();
      vulkanDescriptors.reset();
      vulkanRenderPass.reset();
      vulkanSwapChain.reset();

      vulkanDevice.reset();
      vulkanInstance.reset();
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

    std::vector<Vertex> edgeVertices;
    std::vector<uint16_t> edgeIndices;
    std::vector<Vertex> internalVertices;
    std::vector<uint16_t> internalIndices;

    VkBuffer edgeVertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory edgeVertexBufferMemory = VK_NULL_HANDLE;
    VkBuffer edgeIndexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory edgeIndexBufferMemory = VK_NULL_HANDLE;

    VkBuffer internalVertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory internalVertexBufferMemory = VK_NULL_HANDLE;
    VkBuffer internalIndexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory internalIndexBufferMemory = VK_NULL_HANDLE;

    std::vector<InstanceData> edgeInstanceData;
    std::vector<InstanceData> internalInstanceData;

    VkBuffer edgeInstanceBuffer = VK_NULL_HANDLE;
    VkDeviceMemory edgeInstanceBufferMemory = VK_NULL_HANDLE;
    VkBuffer internalInstanceBuffer = VK_NULL_HANDLE;
    VkDeviceMemory internalInstanceBufferMemory = VK_NULL_HANDLE;

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

      vkCmdBindVertexBuffers(commandBuffer, 0, 1, &edgeVertexBuffer, offsets);

      vkCmdBindVertexBuffers(commandBuffer, 1, 1, &edgeInstanceBuffer, offsets);

      vkCmdBindIndexBuffer(commandBuffer, edgeIndexBuffer, 0, VK_INDEX_TYPE_UINT16);

      vkCmdDrawIndexed(commandBuffer,
                       static_cast<uint32_t>(edgeIndices.size()),
                       static_cast<uint32_t>(edgeInstanceData.size()),
                       0,
                       0,
                       0);

      vkCmdBindVertexBuffers(commandBuffer, 0, 1, &internalVertexBuffer, offsets);

      vkCmdBindVertexBuffers(commandBuffer, 1, 1, &internalInstanceBuffer, offsets);

      vkCmdBindIndexBuffer(commandBuffer, internalIndexBuffer, 0, VK_INDEX_TYPE_UINT16);

      vkCmdDrawIndexed(commandBuffer,
                       static_cast<uint32_t>(internalIndices.size()),
                       static_cast<uint32_t>(internalInstanceData.size()),
                       0,
                       0,
                       0);

      vkCmdEndRenderPass(commandBuffer);

      if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to record command buffer!");
      }
    }

    [[nodiscard]] bool isEdgeHexagon(int x, int y) const {
      return (x == 0 || x == GRID_WIDTH - 1 || y == 0 || y == GRID_HEIGHT - 1);
    }

    [[nodiscard]] glm::vec2 calculatePositionOffset(const int gridX, const int gridY) const {
      float xOffset = (static_cast<float>(gridX) - static_cast<float>(GRID_WIDTH - 1) / 2.0f) * 1.5f;
      float yOffset = (static_cast<float>(gridY) - static_cast<float>(GRID_HEIGHT - 1) / 2.0f) * sqrt(3.0f);

      if (gridY % 2 == 1) {
        xOffset += 0.75f;
      }

      xOffset *= 0.1167f;
      yOffset *= 0.0875f;

      return {xOffset, yOffset};
    }

    void createInstanceBuffer(const std::vector<InstanceData> &instanceData,
                              VkBuffer &buffer,
                              VkDeviceMemory &bufferMemory) {
      VkDeviceSize bufferSize = sizeof(InstanceData) * instanceData.size();
      vulkanDevice->createBuffer(bufferSize,
                                 VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                 buffer,
                                 bufferMemory);

      void *data;
      vkMapMemory(vulkanDevice->getDevice(), bufferMemory, 0, bufferSize, 0, &data);
      memcpy(data, instanceData.data(), (size_t) bufferSize);
      vkUnmapMemory(vulkanDevice->getDevice(), bufferMemory);
    }

    void generateHexagonData() {
      generateHexagonMesh(true, edgeVertices, edgeIndices);

      generateHexagonMesh(false, internalVertices, internalIndices);
    }

    void generateHexagonMesh(bool includeSides, std::vector<Vertex> &vertices, std::vector<uint16_t> &indices) {
      constexpr float radius_outer = 1.0f;
      constexpr float radius_inner = 0.9f;
      constexpr float rotationAngle = glm::radians(30.0f);
      constexpr float height = 1.0f;
      constexpr auto blackColor = glm::vec3(0.0f, 0.0f, 0.0f);
      constexpr auto greenColor = glm::vec3(0.293f, 0.711f, 0.129f);
      constexpr auto brownColor = glm::vec3(0.367f, 0.172f, 0.039f);
      constexpr auto whiteColor = glm::vec3(1.0f, 1.0f, 1.0f);

      const auto baseIndexInnerGreen = static_cast<uint16_t>(vertices.size());
      generateNSidedShapeWithCenterVertices(6, radius_inner, rotationAngle, height, greenColor, vertices);
      auto centerIndex = static_cast<uint16_t>(vertices.size() - 1);

      for (uint16_t i = 0; i < 6; ++i) {
        indices.push_back(baseIndexInnerGreen + i);
        indices.push_back(baseIndexInnerGreen + ((i + 1) % 6));
        indices.push_back(centerIndex);
      }

      const auto baseIndexInnerBlack = static_cast<uint16_t>(vertices.size());
      offsetNVertSurface(vertices.end() - 1, vertices, 0.0f, blackColor, 6);

      const auto baseIndexOuter = static_cast<uint16_t>(vertices.size());
      offsetNVertSurface(vertices.end(), vertices, 0.111f, blackColor, 6);

      for (uint16_t i = 0; i < 6; ++i) {
        indices.push_back(baseIndexInnerBlack + i);
        indices.push_back(baseIndexOuter + i);
        indices.push_back(baseIndexOuter + ((i + 1) % 6));

        indices.push_back(baseIndexInnerBlack + i);
        indices.push_back(baseIndexOuter + ((i + 1) % 6));
        indices.push_back(baseIndexInnerBlack + ((i + 1) % 6));
      }

      const auto baseIndexInnerGreenBot = static_cast<uint16_t>(vertices.size());
      generateNSidedShapeWithCenterVertices(6, radius_inner, rotationAngle, -height, whiteColor, vertices);
      auto centerIndexBot = static_cast<uint16_t>(vertices.size() - 1);

      for (uint16_t i = 0; i < 6; ++i) {
        indices.push_back(baseIndexInnerGreenBot + i);
        indices.push_back(centerIndexBot);
        indices.push_back(baseIndexInnerGreenBot + ((i + 1) % 6));
      }

      const auto baseIndexInnerBlackBot = static_cast<uint16_t>(vertices.size());
      offsetNVertSurface(vertices.end() - 1, vertices, 0.0f, blackColor, 6);

      const auto baseIndexOuterBot = static_cast<uint16_t>(vertices.size());
      offsetNVertSurface(vertices.end(), vertices, 0.11f, blackColor, 6);

      for (uint16_t i = 0; i < 6; ++i) {
        indices.push_back(baseIndexOuterBot + i);
        indices.push_back(baseIndexInnerBlackBot + i);
        indices.push_back(baseIndexOuterBot + ((i + 1) % 6));

        indices.push_back(baseIndexOuterBot + ((i + 1) % 6));
        indices.push_back(baseIndexInnerBlackBot + i);
        indices.push_back(baseIndexInnerBlackBot + ((i + 1) % 6));
      }

      if (includeSides) {
        for (uint16_t i = 0; i < 6; ++i) {
          uint16_t topOuterCurr = baseIndexOuter + i;
          uint16_t topOuterNext = baseIndexOuter + ((i + 1) % 6);
          uint16_t bottomOuterCurr = baseIndexOuterBot + i;
          uint16_t bottomOuterNext = baseIndexOuterBot + ((i + 1) % 6);

          Vertex v0 = vertices[topOuterCurr];
          v0.color = blackColor;
          Vertex v1 = vertices[bottomOuterCurr];
          v1.color = blackColor;
          Vertex v2 = vertices[bottomOuterNext];
          v2.color = blackColor;
          Vertex v3 = vertices[topOuterNext];
          v3.color = blackColor;

          std::vector<Vertex> originalSquare{v0, v1, v2, v3};

          uint16_t idx_v0 = static_cast<uint16_t>(vertices.size());
          vertices.push_back(v0);
          uint16_t idx_v1 = static_cast<uint16_t>(vertices.size());
          vertices.push_back(v1);
          uint16_t idx_v2 = static_cast<uint16_t>(vertices.size());
          vertices.push_back(v2);
          uint16_t idx_v3 = static_cast<uint16_t>(vertices.size());
          vertices.push_back(v3);

          std::vector<Vertex> offsetVerticesBlack;
          Vertex centerVertex;
          float offsetInwards = 0.1f;

          offsetNVertSurfaceWithCenter(originalSquare, offsetVerticesBlack, offsetInwards, blackColor, centerVertex);

          uint16_t idx_offset_v0 = static_cast<uint16_t>(vertices.size());
          vertices.push_back(offsetVerticesBlack[0]);
          uint16_t idx_offset_v1 = static_cast<uint16_t>(vertices.size());
          vertices.push_back(offsetVerticesBlack[1]);
          uint16_t idx_offset_v2 = static_cast<uint16_t>(vertices.size());
          vertices.push_back(offsetVerticesBlack[2]);
          uint16_t idx_offset_v3 = static_cast<uint16_t>(vertices.size());
          vertices.push_back(offsetVerticesBlack[3]);

          std::vector<Vertex> offsetVerticesBrown = offsetVerticesBlack;
          for (auto &v : offsetVerticesBrown) {
            v.color = brownColor;
          }

          uint16_t idx_brown_v0 = static_cast<uint16_t>(vertices.size());
          vertices.push_back(offsetVerticesBrown[0]);
          uint16_t idx_brown_v1 = static_cast<uint16_t>(vertices.size());
          vertices.push_back(offsetVerticesBrown[1]);
          uint16_t idx_brown_v2 = static_cast<uint16_t>(vertices.size());
          vertices.push_back(offsetVerticesBrown[2]);
          uint16_t idx_brown_v3 = static_cast<uint16_t>(vertices.size());
          vertices.push_back(offsetVerticesBrown[3]);

          centerVertex.color = brownColor;
          uint16_t idx_center = static_cast<uint16_t>(vertices.size());
          vertices.push_back(centerVertex);

          indices.push_back(idx_v0);
          indices.push_back(idx_v1);
          indices.push_back(idx_offset_v1);

          indices.push_back(idx_v0);
          indices.push_back(idx_offset_v1);
          indices.push_back(idx_offset_v0);

          indices.push_back(idx_v1);
          indices.push_back(idx_v2);
          indices.push_back(idx_offset_v2);

          indices.push_back(idx_v1);
          indices.push_back(idx_offset_v2);
          indices.push_back(idx_offset_v1);

          indices.push_back(idx_v2);
          indices.push_back(idx_v3);
          indices.push_back(idx_offset_v3);

          indices.push_back(idx_v2);
          indices.push_back(idx_offset_v3);
          indices.push_back(idx_offset_v2);

          indices.push_back(idx_v3);
          indices.push_back(idx_v0);
          indices.push_back(idx_offset_v0);

          indices.push_back(idx_v3);
          indices.push_back(idx_offset_v0);
          indices.push_back(idx_offset_v3);

          indices.push_back(idx_brown_v0);
          indices.push_back(idx_brown_v1);
          indices.push_back(idx_center);

          indices.push_back(idx_brown_v1);
          indices.push_back(idx_brown_v2);
          indices.push_back(idx_center);

          indices.push_back(idx_brown_v2);
          indices.push_back(idx_brown_v3);
          indices.push_back(idx_center);

          indices.push_back(idx_brown_v3);
          indices.push_back(idx_brown_v0);
          indices.push_back(idx_center);
        }
      }
    }

    static void offsetNVertSurfaceWithCenter(
      const std::vector<Vertex> &surfaceVerts,
      std::vector<Vertex> &newVertices,
      const float offset,
      const glm::vec3 &color,
      Vertex &centerVertex) {
      glm::vec3 center{0.0f};
      for (const auto &v : surfaceVerts) {
        center += v.pos;
      }
      center /= surfaceVerts.size();

      for (const auto &v : surfaceVerts) {
        glm::vec3 dir = glm::normalize(center - v.pos);
        Vertex vert = v;
        vert.pos += dir * offset;
        vert.color = color;
        newVertices.push_back(vert);
      }

      centerVertex = {center, color};
    }

    static void offsetNVertSurface(const std::vector<Vertex> &surfaceVerts,
                                   std::vector<Vertex> &verticesBuffer,
                                   const float offset,
                                   const glm::vec3 &color) {
      glm::vec3 center{0.0f};
      for (const auto &v : surfaceVerts) {
        center += v.pos;
      }
      center /= surfaceVerts.size();

      for (auto v : surfaceVerts) {
        glm::vec3 dir = glm::normalize(v.pos - center);
        v.pos += dir * offset;
        v.color = color;

        verticesBuffer.push_back(v);
      }
    }

    static void offsetNVertSurface(
      const std::vector<Vertex>::iterator &endIt,
      std::vector<Vertex> &verticesBuffer,
      const float offset,
      const glm::vec3 &color,
      int n) {
      glm::vec3 center{0.0f};
      auto startIt = endIt - n;

      for (auto it = startIt; it != endIt; ++it) {
        center += it->pos;
      }
      center /= static_cast<float>(n);

      std::vector<Vertex> newVertices;

      for (auto it = startIt; it != endIt; ++it) {
        glm::vec3 dir = glm::normalize(it->pos - center);
        Vertex vert = {it->pos, it->color};
        vert.pos += dir * offset;
        vert.color = color;

        newVertices.push_back(vert);
      }

      verticesBuffer.insert(verticesBuffer.end(), newVertices.begin(), newVertices.end());
    }

    void generateNSidedShapeWithCenterVertices(int n,
                                               float radius,
                                               float rotationAngle,
                                               float height,
                                               glm::vec3 color,
                                               std::vector<Vertex> &vertexVec) {
      float angleIncrement = glm::radians(360.0f / static_cast<float>(n));
      for (int i = 0; i < n; ++i) {
        float angle = i * angleIncrement + rotationAngle;

        float x = radius * cos(angle);
        float y = radius * sin(angle);
        float z = height;
        vertexVec.push_back({{x, y, z}, color});
      }
      vertexVec.push_back({{0.0f, 0.0f, height}, color});
    }
    void generateNSidedShapeVertices(int n,
                                     float radius,
                                     float rotationAngle,
                                     float height,
                                     glm::vec3 color,
                                     std::vector<Vertex> &vertexVec) {
      float angleIncrement = glm::radians(360.0f / static_cast<float>(n));
      for (int i = 0; i < n; ++i) {
        float angle = i * angleIncrement + rotationAngle;

        float x = radius * cos(angle);
        float y = radius * sin(angle);
        float z = height;
        vertexVec.push_back({{x, y, z}, color});
      }
    }

    void createVertexBuffer(const std::vector<Vertex> &vertices,
                            VkBuffer &vertexBuffer,
                            VkDeviceMemory &vertexBufferMemory) {
      VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

      VkBufferCreateInfo bufferInfo{};
      bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
      bufferInfo.size = bufferSize;
      bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
      bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

      if (vkCreateBuffer(vulkanDevice->getDevice(), &bufferInfo, nullptr, &vertexBuffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to create vertex buffer!");
      }

      VkMemoryRequirements memRequirements;
      vkGetBufferMemoryRequirements(vulkanDevice->getDevice(), vertexBuffer, &memRequirements);

      VkMemoryAllocateInfo allocInfo{};
      allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
      allocInfo.allocationSize = memRequirements.size;
      allocInfo.memoryTypeIndex =
        vulkanDevice->findMemoryType(memRequirements.memoryTypeBits,
                                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

      if (vkAllocateMemory(vulkanDevice->getDevice(), &allocInfo, nullptr, &vertexBufferMemory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate vertex buffer memory!");
      }

      vkBindBufferMemory(vulkanDevice->getDevice(), vertexBuffer, vertexBufferMemory, 0);

      void *data;
      vkMapMemory(vulkanDevice->getDevice(), vertexBufferMemory, 0, bufferSize, 0, &data);
      memcpy(data, vertices.data(), (size_t) bufferSize);
      vkUnmapMemory(vulkanDevice->getDevice(), vertexBufferMemory);
    }

    void createIndexBuffer(const std::vector<uint16_t> &indices,
                           VkBuffer &indexBuffer,
                           VkDeviceMemory &indexBufferMemory) {
      VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

      VkBufferCreateInfo bufferInfo{};
      bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
      bufferInfo.size = bufferSize;
      bufferInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
      bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

      if (vkCreateBuffer(vulkanDevice->getDevice(), &bufferInfo, nullptr, &indexBuffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to create index buffer!");
      }

      VkMemoryRequirements memRequirements;
      vkGetBufferMemoryRequirements(vulkanDevice->getDevice(), indexBuffer, &memRequirements);

      VkMemoryAllocateInfo allocInfo{};
      allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
      allocInfo.allocationSize = memRequirements.size;
      allocInfo.memoryTypeIndex =
        vulkanDevice->findMemoryType(memRequirements.memoryTypeBits,
                                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

      if (vkAllocateMemory(vulkanDevice->getDevice(), &allocInfo, nullptr, &indexBufferMemory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate index buffer memory!");
      }

      vkBindBufferMemory(vulkanDevice->getDevice(), indexBuffer, indexBufferMemory, 0);

      void *data;
      vkMapMemory(vulkanDevice->getDevice(), indexBufferMemory, 0, bufferSize, 0, &data);
      memcpy(data, indices.data(), (size_t) bufferSize);
      vkUnmapMemory(vulkanDevice->getDevice(), indexBufferMemory);
    }
    void prepareInstanceData() {
      for (int y = 0; y < GRID_HEIGHT; ++y) {
        for (int x = 0; x < GRID_WIDTH; ++x) {
          InstanceData inst{};
          inst.offset = calculatePositionOffset(x, y);
          if (isEdgeHexagon(x, y)) {
            edgeInstanceData.push_back(inst);
          } else {
            internalInstanceData.push_back(inst);
          }
        }
      }

      createInstanceBuffer(edgeInstanceData, edgeInstanceBuffer, edgeInstanceBufferMemory);
      createInstanceBuffer(internalInstanceData, internalInstanceBuffer, internalInstanceBufferMemory);
    }

    static constexpr int GRID_WIDTH = 10;
    static constexpr int GRID_HEIGHT = 10;
};
