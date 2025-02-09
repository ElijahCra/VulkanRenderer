#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "VulkanWindow.cpp"
#include "VulkanInstance.cpp"
#include "VulkanDevice.cpp"
#include "VulkanSwapChain.cpp"
#include "VulkanRenderPass.cpp"
#include "VulkanPipeline.cpp"
#include "VulkanDescriptor.cpp"
#include "Util.cpp"

#include <iostream>
#include <fstream>
#include <stdexcept>
#include <algorithm>
#include <vector>
#include <array>
#include <cstring>
#include <cstdlib>
#include <cstdint>
#include <memory>
#include <optional>
#include <set>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#if defined(__APPLE__)
const bool macOS = true;
#else
const bool macOS = false;
#endif

constexpr uint32_t WIDTH = 800;
constexpr uint32_t HEIGHT = 600;

constexpr int MAX_FRAMES_IN_FLIGHT = 2;

const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif



class HelloTriangleApplication {
 public:
  [[nodiscard]] uint32_t findMemoryType(const uint32_t typeFilter, VkMemoryPropertyFlags properties) const {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(vulkanDevice->getPhysicalDevice(), &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
      if ((typeFilter & (1 << i)) &&
          (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
        return i;
          }
    }

    throw std::runtime_error("failed to find suitable memory type!");
  }

  void run() {
    vulkanWindow = std::make_unique<VulkanWindow>(WIDTH, HEIGHT, "Vulkan");

    initVulkan();
    mainLoop();
    cleanup();
  }

 private:
  std::shared_ptr<VulkanWindow> vulkanWindow;
  std::unique_ptr<VulkanInstance> vulkanInstance;
  std::shared_ptr<VulkanDevice> vulkanDevice;
  std::shared_ptr<VulkanSwapChain> vulkanSwapChain;
  std::unique_ptr<VulkanRenderPass> vulkanRenderPass;
  std::unique_ptr<VulkanPipeline> vulkanPipeline;
  std::unique_ptr<VulkanDescriptors> vulkanDescriptors;


  VkCommandPool commandPool = nullptr;
  std::vector<VkCommandBuffer> commandBuffers;

  std::vector<VkSemaphore> imageAvailableSemaphores;
  std::vector<VkSemaphore> renderFinishedSemaphores;
  std::vector<VkFence> inFlightFences;
  uint32_t currentFrame = 0;

  bool framebufferResized = false;

  std::vector<Vertex> edgeVertices;
  std::vector<uint16_t> edgeIndices;
  std::vector<Vertex> internalVertices;
  std::vector<uint16_t> internalIndices;

  VkBuffer edgeVertexBuffer = nullptr;
  VkDeviceMemory edgeVertexBufferMemory = nullptr;
  VkBuffer edgeIndexBuffer = nullptr;
  VkDeviceMemory edgeIndexBufferMemory = nullptr;

  VkBuffer internalVertexBuffer = nullptr;
  VkDeviceMemory internalVertexBufferMemory = nullptr;
  VkBuffer internalIndexBuffer = nullptr;
  VkDeviceMemory internalIndexBufferMemory = nullptr;



  std::vector<InstanceData> edgeInstanceData;
  std::vector<InstanceData> internalInstanceData;

  VkBuffer edgeInstanceBuffer;
  VkDeviceMemory edgeInstanceBufferMemory;
  VkBuffer internalInstanceBuffer;
  VkDeviceMemory internalInstanceBufferMemory;

  uint32_t mipLevels;
  VkImage textureImage;

  const int GRID_WIDTH = 10;
  const int GRID_HEIGHT = 10;
  const int instanceCount = GRID_WIDTH * GRID_HEIGHT;

  void initVulkan() {
    vulkanInstance = std::make_unique<VulkanInstance>(vulkanWindow->getGLFWwindow());
    vulkanDevice = std::make_shared<VulkanDevice>(vulkanInstance->getVkInstance(), vulkanInstance->getSurface());
    vulkanSwapChain = std::make_unique<VulkanSwapChain>(vulkanDevice,vulkanInstance->getSurface(),WIDTH,HEIGHT);
    vulkanRenderPass = std::make_unique<VulkanRenderPass>(vulkanDevice,vulkanSwapChain->getImageFormat(),vulkanDevice->getMsaaSamples(),vulkanSwapChain->findDepthFormat());
    vulkanSwapChain->createFramebuffers(vulkanRenderPass->getHandle());
    vulkanDescriptors = std::make_unique<VulkanDescriptors>(vulkanDevice,vulkanSwapChain,vulkanWindow, MAX_FRAMES_IN_FLIGHT);
    vulkanPipeline = std::make_unique<VulkanPipeline>(
        vulkanDevice->getDevice(),
        vulkanRenderPass->getHandle(),
        vulkanDescriptors->getDescriptorSetLayout(),
        vulkanDevice->getMsaaSamples(),
        "../shaders/vert.spv",
        "../shaders/frag.spv"
    );
    createCommandPool();
    generateHexagonData();
    createVertexBuffer(edgeVertices, edgeVertexBuffer, edgeVertexBufferMemory);
    createIndexBuffer(edgeIndices, edgeIndexBuffer, edgeIndexBufferMemory);
    createVertexBuffer(internalVertices, internalVertexBuffer, internalVertexBufferMemory);
    createIndexBuffer(internalIndices, internalIndexBuffer, internalIndexBufferMemory);
    prepareInstanceData();
    createCommandBuffers();
    createSyncObjects();
  }

  void mainLoop() {
    while (!vulkanWindow->shouldClose()) {
      vulkanWindow->pollEvents();
      drawFrame();
    }

    vkDeviceWaitIdle(vulkanDevice->getDevice());
  }



  void cleanup() {

    vkDestroyBuffer(vulkanDevice->getDevice(), edgeVertexBuffer, nullptr);
    vkFreeMemory(vulkanDevice->getDevice(), edgeVertexBufferMemory, nullptr);
    vkDestroyBuffer(vulkanDevice->getDevice(), edgeIndexBuffer, nullptr);
    vkFreeMemory(vulkanDevice->getDevice(), edgeIndexBufferMemory, nullptr);
    vkDestroyBuffer(vulkanDevice->getDevice(), internalVertexBuffer, nullptr);
    vkFreeMemory(vulkanDevice->getDevice(), internalVertexBufferMemory, nullptr);
    vkDestroyBuffer(vulkanDevice->getDevice(), internalIndexBuffer, nullptr);
    vkFreeMemory(vulkanDevice->getDevice(), internalIndexBufferMemory, nullptr);

    vkDestroyBuffer(vulkanDevice->getDevice(), edgeInstanceBuffer, nullptr);
    vkFreeMemory(vulkanDevice->getDevice(), edgeInstanceBufferMemory, nullptr);
    vkDestroyBuffer(vulkanDevice->getDevice(), internalInstanceBuffer, nullptr);
    vkFreeMemory(vulkanDevice->getDevice(), internalInstanceBufferMemory, nullptr);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
      vkDestroySemaphore(vulkanDevice->getDevice(), renderFinishedSemaphores[i], nullptr);
      vkDestroySemaphore(vulkanDevice->getDevice(), imageAvailableSemaphores[i], nullptr);
      vkDestroyFence(vulkanDevice->getDevice(), inFlightFences[i], nullptr);
    }

    vkDestroyCommandPool(vulkanDevice->getDevice(), commandPool, nullptr);

    vkDestroyDevice(vulkanDevice->getDevice(), nullptr);

    glfwTerminate();
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


    vulkanRenderPass->createRenderPass(vulkanSwapChain->getImageFormat(),vulkanDevice->getMsaaSamples(),vulkanSwapChain->findDepthFormat());
    vulkanSwapChain->createFramebuffers(vulkanRenderPass->getHandle());
    vulkanPipeline->createGraphicsPipeline(vulkanRenderPass->getHandle(),vulkanDevice->getMsaaSamples(),"../shaders/vert.spv","../shaders/frag.spv");
  }

  void prepareInstanceData() {
    for (int y = 0; y < GRID_HEIGHT; ++y) {
      for (int x = 0; x < GRID_WIDTH; ++x) {
        InstanceData instance_data{};
        instance_data.offset = calculatePositionOffset(x, y);

        if (isEdgeHexagon(x, y)) {
          edgeInstanceData.push_back(instance_data);
        } else {
          internalInstanceData.push_back(instance_data);
        }
      }
    }

    std::cout << "Edge instances: " << edgeInstanceData.size() << std::endl;
    std::cout << "Internal instances: " << internalInstanceData.size() << std::endl;

    createInstanceBuffer(edgeInstanceData, edgeInstanceBuffer, edgeInstanceBufferMemory);
    createInstanceBuffer(internalInstanceData, internalInstanceBuffer, internalInstanceBufferMemory);
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

  void createInstanceBuffer(const std::vector<InstanceData>& instanceData, VkBuffer& buffer, VkDeviceMemory& bufferMemory) {
    VkDeviceSize bufferSize = sizeof(InstanceData) * instanceData.size();
    vulkanDevice->createBuffer(bufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, buffer, bufferMemory);

    void* data;
    vkMapMemory(vulkanDevice->getDevice(), bufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, instanceData.data(), (size_t)bufferSize);
    vkUnmapMemory(vulkanDevice->getDevice(), bufferMemory);
  }


  void createImage(uint32_t width, uint32_t height, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory) {
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;  // For depth image
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;  // 2D image, depth is 1
    imageInfo.mipLevels = 1;     // No mipmapping
    imageInfo.arrayLayers = 1;   // Single-layered image
    imageInfo.format = format;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usage;  // Should include VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT
    imageInfo.samples = numSamples;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateImage(vulkanDevice->getDevice(), &imageInfo, nullptr, &image) != VK_SUCCESS) {
      throw std::runtime_error("failed to create image!");
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(vulkanDevice->getDevice(), image, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(vulkanDevice->getDevice(), &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
      throw std::runtime_error("failed to allocate image memory!");
    }

    vkBindImageMemory(vulkanDevice->getDevice(), image, imageMemory, 0);
  }

  VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags) const {
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;  // The image to create a view for
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;

    viewInfo.subresourceRange.aspectMask = aspectFlags;  // e.g., VK_IMAGE_ASPECT_DEPTH_BIT
    viewInfo.subresourceRange.baseMipLevel = 0;  // No mipmapping
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;  // Single-layered image
    viewInfo.subresourceRange.layerCount = 1;

    VkImageView imageView;
    if (vkCreateImageView(vulkanDevice->getDevice(), &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
      throw std::runtime_error("failed to create texture image view!");
    }

    return imageView;
  }

  VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling,
                             VkFormatFeatureFlags features) {
    for (VkFormat format : candidates) {
      VkFormatProperties props;
      vkGetPhysicalDeviceFormatProperties(vulkanDevice->getPhysicalDevice(), format, &props);

      if (tiling == VK_IMAGE_TILING_LINEAR &&
          (props.linearTilingFeatures & features) == features) {
        return format;
          } else if (tiling == VK_IMAGE_TILING_OPTIMAL &&
                     (props.optimalTilingFeatures & features) == features) {
            return format;
                     }
    }

    throw std::runtime_error("failed to find supported format!");
  }

  void createCommandPool() {
    QueueFamilyIndices queueFamilyIndices = vulkanDevice->findQueueFamilies(vulkanDevice->getPhysicalDevice());

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

    if (vkCreateCommandPool(vulkanDevice->getDevice(), &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
      throw std::runtime_error("failed to create command pool!");
    }
  }


void generateHexagonData() {
    // Generate mesh for edge hexagons (with sides)
    generateHexagonMesh(true, edgeVertices, edgeIndices);

    // Generate mesh for internal hexagons (without sides)
    generateHexagonMesh(false, internalVertices, internalIndices);
}

// ======== Helper Functions ========
  void generateHexagonMesh(bool includeSides, std::vector<Vertex>& vertices, std::vector<uint16_t>& indices) {

    constexpr float radius_outer = 1.0f;
    constexpr float radius_inner = 0.9f;
    constexpr float rotationAngle = glm::radians(30.0f);
    constexpr float height = 1.0f;
    constexpr auto blackColor = glm::vec3(0.0f, 0.0f, 0.0f);
    constexpr auto greenColor = glm::vec3(0.293f, 0.711f, 0.129f);
    constexpr auto brownColor = glm::vec3(0.367f, 0.172f, 0.039f);
    constexpr auto whiteColor = glm::vec3(1.0f, 1.0f, 1.0f);

    // ======== Top Hexagon ========

    // Inner hexagon vertices (green color)
    const auto baseIndexInnerGreen = static_cast<uint16_t>(vertices.size());
    generateNSidedShapeWithCenterVertices(6, radius_inner, rotationAngle, height, greenColor, vertices);
    auto centerIndex = static_cast<uint16_t>(vertices.size() - 1);

    // Inner hexagon indices (green color)
    for (uint16_t i = 0; i < 6; ++i) {
        indices.push_back(baseIndexInnerGreen + i);
        indices.push_back(baseIndexInnerGreen + ((i + 1) % 6));
        indices.push_back(centerIndex);
    }

    // Inner hexagon vertices for border (black color)
    const auto baseIndexInnerBlack = static_cast<uint16_t>(vertices.size());
    offsetNVertSurface(vertices.end()-1,vertices,0.0f,blackColor,6);
    //generateNSidedShapeVertices(6, radius_inner, rotationAngle, height, blackColor, vertices);

    // Outer hexagon vertices (black color)
    const auto baseIndexOuter = static_cast<uint16_t>(vertices.size());
    offsetNVertSurface(vertices.end(),vertices,0.111f,blackColor,6);
    //generateNSidedShapeVertices(6, radius_outer, rotationAngle, height, blackColor, vertices);

    // Outer hexagon indices (border)
    for (uint16_t i = 0; i < 6; ++i) {
        // First triangle of the border quad
        indices.push_back(baseIndexInnerBlack + i);
        indices.push_back(baseIndexOuter + i);
        indices.push_back(baseIndexOuter + ((i + 1) % 6));

        // Second triangle of the border quad
        indices.push_back(baseIndexInnerBlack + i);
        indices.push_back(baseIndexOuter + ((i + 1) % 6));
        indices.push_back(baseIndexInnerBlack + ((i + 1) % 6));
    }

    // ======== Bottom Hexagon ========

    // Inner hexagon vertices (green color)
    const auto baseIndexInnerGreenBot = static_cast<uint16_t>(vertices.size());
    generateNSidedShapeWithCenterVertices(6, radius_inner, rotationAngle, -height, whiteColor, vertices);
    auto centerIndexBot = static_cast<uint16_t>(vertices.size() - 1);

    // Inner hexagon indices (green color)
    for (uint16_t i = 0; i < 6; ++i) {
        indices.push_back(baseIndexInnerGreenBot + i);
        indices.push_back(centerIndexBot);
        indices.push_back(baseIndexInnerGreenBot + ((i + 1) % 6));
    }

    // Inner hexagon vertices for border (black color)
    const auto baseIndexInnerBlackBot = static_cast<uint16_t>(vertices.size());
    offsetNVertSurface(vertices.end()-1,vertices,0.0f,blackColor,6);

    // Outer hexagon vertices (black color)
    const auto baseIndexOuterBot = static_cast<uint16_t>(vertices.size());
    offsetNVertSurface(vertices.end(),vertices,0.11f,blackColor,6);

    // Outer hexagon indices (border)
    for (uint16_t i = 0; i < 6; ++i) {
        // First triangle of the border quad

        indices.push_back(baseIndexOuterBot + i);
      indices.push_back(baseIndexInnerBlackBot + i);
        indices.push_back(baseIndexOuterBot + ((i + 1) % 6));

        // Second triangle of the border quad

        indices.push_back(baseIndexOuterBot + ((i + 1) % 6));
      indices.push_back(baseIndexInnerBlackBot + i);
        indices.push_back(baseIndexInnerBlackBot + ((i + 1) % 6));
    }




    if (includeSides) {
    // ======== Side Faces ========

    // Generate side faces between top and bottom outer hexagons
    for (uint16_t i = 0; i < 6; ++i) {
        // Indices of the top and bottom outer vertices
        uint16_t topOuterCurr = baseIndexOuter + i;
        uint16_t topOuterNext = baseIndexOuter + ((i + 1) % 6);
        uint16_t bottomOuterCurr = baseIndexOuterBot + i;
        uint16_t bottomOuterNext = baseIndexOuterBot + ((i + 1) % 6);

        // ======== First Set: Original Square (Black Color) ========

        // Create original vertices
        Vertex v0 = vertices[topOuterCurr]; v0.color = blackColor;
        Vertex v1 = vertices[bottomOuterCurr]; v1.color = blackColor;
        Vertex v2 = vertices[bottomOuterNext]; v2.color = blackColor;
        Vertex v3 = vertices[topOuterNext]; v3.color = blackColor;

        std::vector<Vertex> originalSquare { v0, v1, v2, v3 };

        // Add original vertices to vertex buffer
        uint16_t idx_v0 = static_cast<uint16_t>(vertices.size()); vertices.push_back(v0);
        uint16_t idx_v1 = static_cast<uint16_t>(vertices.size()); vertices.push_back(v1);
        uint16_t idx_v2 = static_cast<uint16_t>(vertices.size()); vertices.push_back(v2);
        uint16_t idx_v3 = static_cast<uint16_t>(vertices.size()); vertices.push_back(v3);

        // ======== Second Set: Offset Square (Black Color) ========

        std::vector<Vertex> offsetVerticesBlack;
        Vertex centerVertex;
        float offsetInwards = 0.1f; // Adjust as needed

        offsetNVertSurfaceWithCenter(originalSquare, offsetVerticesBlack, offsetInwards, blackColor, centerVertex);

        // Add offset vertices to vertex buffer
        uint16_t idx_offset_v0 = static_cast<uint16_t>(vertices.size()); vertices.push_back(offsetVerticesBlack[0]);
        uint16_t idx_offset_v1 = static_cast<uint16_t>(vertices.size()); vertices.push_back(offsetVerticesBlack[1]);
        uint16_t idx_offset_v2 = static_cast<uint16_t>(vertices.size()); vertices.push_back(offsetVerticesBlack[2]);
        uint16_t idx_offset_v3 = static_cast<uint16_t>(vertices.size()); vertices.push_back(offsetVerticesBlack[3]);

        // ======== Third Set: Offset Square (Brown Color) ========

        std::vector<Vertex> offsetVerticesBrown = offsetVerticesBlack;
        for (auto& v : offsetVerticesBrown) {
            v.color = brownColor;
        }

        // Add brown vertices to vertex buffer
        uint16_t idx_brown_v0 = static_cast<uint16_t>(vertices.size()); vertices.push_back(offsetVerticesBrown[0]);
        uint16_t idx_brown_v1 = static_cast<uint16_t>(vertices.size()); vertices.push_back(offsetVerticesBrown[1]);
        uint16_t idx_brown_v2 = static_cast<uint16_t>(vertices.size()); vertices.push_back(offsetVerticesBrown[2]);
        uint16_t idx_brown_v3 = static_cast<uint16_t>(vertices.size()); vertices.push_back(offsetVerticesBrown[3]);

        // ======== Add Center Point (Brown Color) ========

        centerVertex.color = brownColor;
        uint16_t idx_center = static_cast<uint16_t>(vertices.size()); vertices.push_back(centerVertex);

        // ======== Fill Area Between First and Second Squares (Black Border) ========

        // Edge 0-1
        indices.push_back(idx_v0);
        indices.push_back(idx_v1);
        indices.push_back(idx_offset_v1);

        indices.push_back(idx_v0);
        indices.push_back(idx_offset_v1);
        indices.push_back(idx_offset_v0);

        // Edge 1-2
        indices.push_back(idx_v1);
        indices.push_back(idx_v2);
        indices.push_back(idx_offset_v2);

        indices.push_back(idx_v1);
        indices.push_back(idx_offset_v2);
        indices.push_back(idx_offset_v1);

        // Edge 2-3
        indices.push_back(idx_v2);
        indices.push_back(idx_v3);
        indices.push_back(idx_offset_v3);

        indices.push_back(idx_v2);
        indices.push_back(idx_offset_v3);
        indices.push_back(idx_offset_v2);

        // Edge 3-0
        indices.push_back(idx_v3);
        indices.push_back(idx_v0);
        indices.push_back(idx_offset_v0);

        indices.push_back(idx_v3);
        indices.push_back(idx_offset_v0);
        indices.push_back(idx_offset_v3);

        // ======== Fill Third Square with Brown Using Center Point ========

        // Triangle 1
        indices.push_back(idx_brown_v0);
        indices.push_back(idx_brown_v1);
        indices.push_back(idx_center);

        // Triangle 2
        indices.push_back(idx_brown_v1);
        indices.push_back(idx_brown_v2);
        indices.push_back(idx_center);

        // Triangle 3
        indices.push_back(idx_brown_v2);
        indices.push_back(idx_brown_v3);
        indices.push_back(idx_center);

        // Triangle 4
        indices.push_back(idx_brown_v3);
        indices.push_back(idx_brown_v0);
        indices.push_back(idx_center);
      }
    }
  }

static void offsetNVertSurfaceWithCenter(
    const std::vector<Vertex>& surfaceVerts,
    std::vector<Vertex>& newVertices,
    const float offset,
    const glm::vec3& color,
    Vertex& centerVertex) {
    glm::vec3 center { 0.0f };
    for (const auto& v : surfaceVerts) {
      center += v.pos;
    }
    center /= surfaceVerts.size();

    for (const auto& v : surfaceVerts) {
      glm::vec3 dir = glm::normalize(center - v.pos); // Direction towards center
      Vertex vert = v;
      vert.pos += dir * offset;
      vert.color = color;
      newVertices.push_back(vert);
    }

    centerVertex = { center, color }; // Center point
  }

  static void offsetNVertSurface(const std::vector<Vertex> & surfaceVerts, std::vector<Vertex>& verticesBuffer, const float offset, const glm::vec3& color)
  {
    glm::vec3 center {0.0f};
    for (const auto& v : surfaceVerts) {
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
                               const std::vector<Vertex>::iterator& endIt,
                               std::vector<Vertex>& verticesBuffer,
                               const float offset,
                               const glm::vec3& color, int n)
  {
    glm::vec3 center{ 0.0f };
    auto startIt = endIt-n;
    // Compute the center
    for (auto it = startIt; it != endIt; ++it) {
      center += it->pos;
    }
    center /= static_cast<float>(n);

    std::vector<Vertex> newVertices;

    for (auto it = startIt; it != endIt; ++it) {
      glm::vec3 dir = glm::normalize(it->pos - center);
      Vertex vert = { it->pos, it->color };
      vert.pos += dir * offset;
      vert.color = color;

      newVertices.push_back(vert);
    }

    verticesBuffer.insert(verticesBuffer.end(), newVertices.begin(), newVertices.end());
  }





  void generateNSidedShapeWithCenterVertices(int n, float radius, float rotationAngle, float height, glm::vec3 color, std::vector<Vertex>& vertexVec) {
    float angleIncrement = glm::radians(360.0f/static_cast<float>(n));
    for (int i = 0; i < n; ++i) {
      float angle = i * angleIncrement+rotationAngle;

      float x = radius * cos(angle);
      float y = radius * sin(angle);
      float z = height;
      vertexVec.push_back({{x, y,z}, color});  // Border color (black)
    }
    vertexVec.push_back({{0.0f, 0.0f,height}, color});  // Center vertex for outer hexagon
  }
  void generateNSidedShapeVertices(int n, float radius, float rotationAngle, float height, glm::vec3 color, std::vector<Vertex>& vertexVec) {
    float angleIncrement = glm::radians(360.0f/static_cast<float>(n));
    for (int i = 0; i < n; ++i) {
      float angle = i * angleIncrement+rotationAngle;

      float x = radius * cos(angle);
      float y = radius * sin(angle);
      float z = height;
      vertexVec.push_back({{x, y,z}, color});  // Border color (black)
    }
  }

  void createVertexBuffer(const std::vector<Vertex>& vertices, VkBuffer& vertexBuffer, VkDeviceMemory& vertexBufferMemory) {
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
        findMemoryType(memRequirements.memoryTypeBits,
                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    if (vkAllocateMemory(vulkanDevice->getDevice(), &allocInfo, nullptr, &vertexBufferMemory) != VK_SUCCESS) {
      throw std::runtime_error("failed to allocate vertex buffer memory!");
    }

    vkBindBufferMemory(vulkanDevice->getDevice(), vertexBuffer, vertexBufferMemory, 0);

    void* data;
    vkMapMemory(vulkanDevice->getDevice(), vertexBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, vertices.data(), (size_t) bufferSize);
    vkUnmapMemory(vulkanDevice->getDevice(), vertexBufferMemory);
  }

  void createIndexBuffer(const std::vector<uint16_t>& indices, VkBuffer& indexBuffer, VkDeviceMemory& indexBufferMemory) {
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
        findMemoryType(memRequirements.memoryTypeBits,
                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    if (vkAllocateMemory(vulkanDevice->getDevice(), &allocInfo, nullptr, &indexBufferMemory) != VK_SUCCESS) {
      throw std::runtime_error("failed to allocate index buffer memory!");
    }

    vkBindBufferMemory(vulkanDevice->getDevice(), indexBuffer, indexBufferMemory, 0);

    void* data;
    vkMapMemory(vulkanDevice->getDevice(), indexBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, indices.data(), (size_t) bufferSize);
    vkUnmapMemory(vulkanDevice->getDevice(), indexBufferMemory);
  }

  void createCommandBuffers() {
    commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = (uint32_t) commandBuffers.size();

    if (vkAllocateCommandBuffers(vulkanDevice->getDevice(), &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
      throw std::runtime_error("failed to allocate command buffers!");
    }
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
    clearValues[0].color = { {0.1f, 0.1f, 0.1f, 1.0f} }; // Clear color
    clearValues[1].depthStencil = { 1.0f, 0 };           // Clear depth

    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();


    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vulkanPipeline->getPipeline());

    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vulkanPipeline->getLayout(),
                            0, 1, &vulkanDescriptors->getDescriptorSets()[currentFrame], 0, nullptr);

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

    // Draw edge hexagons
    VkDeviceSize offsets[] = {0};

    // Bind vertex buffer at binding 0
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, &edgeVertexBuffer, offsets);
    // Bind instance buffer at binding 1
    vkCmdBindVertexBuffers(commandBuffer, 1, 1, &edgeInstanceBuffer, offsets);

    vkCmdBindIndexBuffer(commandBuffer, edgeIndexBuffer, 0, VK_INDEX_TYPE_UINT16);

    vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(edgeIndices.size()),
                     static_cast<uint32_t>(edgeInstanceData.size()), 0, 0, 0);

    // Draw internal hexagons
    // Bind vertex buffer at binding 0
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, &internalVertexBuffer, offsets);
    // Bind instance buffer at binding 1
    vkCmdBindVertexBuffers(commandBuffer, 1, 1, &internalInstanceBuffer, offsets);

    vkCmdBindIndexBuffer(commandBuffer, internalIndexBuffer, 0, VK_INDEX_TYPE_UINT16);

    vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(internalIndices.size()),
                     static_cast<uint32_t>(internalInstanceData.size()), 0, 0, 0);

    vkCmdEndRenderPass(commandBuffer);

    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
      throw std::runtime_error("failed to record command buffer!");
    }
  }

  void createSyncObjects() {
    imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
      if (vkCreateSemaphore(vulkanDevice->getDevice(), &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
          vkCreateSemaphore(vulkanDevice->getDevice(), &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
          vkCreateFence(vulkanDevice->getDevice(), &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {
        throw std::runtime_error("failed to create synchronization objects for a frame!");
      }
    }
  }

  void drawFrame() {
    vkWaitForFences(vulkanDevice->getDevice(), 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

    uint32_t imageIndex;
    VkResult result =
        vkAcquireNextImageKHR(vulkanDevice->getDevice(), vulkanSwapChain->getSwapChain(), UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE,
                              &imageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
      recreateSwapChain();
      return;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
      throw std::runtime_error("failed to acquire swap chain image!");
    }
    vulkanDescriptors->updateUniformBuffer(currentFrame);
    vkResetFences(vulkanDevice->getDevice(), 1, &inFlightFences[currentFrame]);

    vkResetCommandBuffer(commandBuffers[currentFrame], /*VkCommandBufferResetFlagBits*/ 0);
    recordCommandBuffer(commandBuffers[currentFrame], imageIndex,currentFrame);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = {imageAvailableSemaphores[currentFrame]};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffers[currentFrame];

    VkSemaphore signalSemaphores[] = {renderFinishedSemaphores[currentFrame]};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (vkQueueSubmit(vulkanDevice->getGraphicsQueue(), 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) {
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

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized) {
      framebufferResized = false;
      recreateSwapChain();
    } else if (result != VK_SUCCESS) {
      throw std::runtime_error("failed to present swap chain image!");
    }


    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
  }

  VkShaderModule createShaderModule(const std::vector<char>& code) {
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(vulkanDevice->getDevice(), &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
      throw std::runtime_error("failed to create shader module!");
    }

    return shaderModule;
  }

  static std::vector<char> readFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
      throw std::runtime_error("failed to open file!");
    }

    size_t fileSize = (size_t) file.tellg();
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);

    file.close();

    return buffer;
  }

  static VKAPI_ATTR VkBool32 VKAPI_CALL
  debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType,
                const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) {
    std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

    return VK_FALSE;
  }
};

int main() {
  HelloTriangleApplication app;

  try {
    app.run();
  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
