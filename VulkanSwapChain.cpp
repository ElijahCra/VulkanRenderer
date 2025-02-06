//
// Created by Elijah on 2/6/2025.
//
#pragma once

#include "VulkanDevice.cpp"
#include <vector>
#include <array>
#include <stdexcept>
#include <memory>

class VulkanSwapChain {
public:
    // We pass references to a VulkanDevice and the surface, plus the current window dimensions:
    VulkanSwapChain(std::shared_ptr<VulkanDevice> device,
                    VkSurfaceKHR surface,
                    uint32_t width,
                    uint32_t height)
        : devicePtr(device), surface(surface), width(width), height(height)
    {
        createSwapChain();
        createImageViews();
        createColorResources();
        createDepthResources();
        createFramebuffers();
    }

    ~VulkanSwapChain() {
        cleanup();
    }

    // We'll expose some getters:
    VkFormat getImageFormat() const { return swapChainImageFormat; }
    VkExtent2D getExtent()    const { return swapChainExtent; }
    VkSwapchainKHR getSwapChain() const { return swapChain; }

    const std::vector<VkImageView>& getImageViews() const { return swapChainImageViews; }
    const std::vector<VkFramebuffer>& getFramebuffers() const { return swapChainFramebuffers; }

    // If you need the depth or color attachments:
    VkImageView getDepthImageView() const { return depthImageView; }
    VkImage getDepthImage() const { return depthImage; }
    VkDeviceMemory getDepthImageMemory() const { return depthImageMemory; }

    VkImageView getColorImageView() const { return colorImageView; }
    VkImage getColorImage() const { return colorImage; }
    VkDeviceMemory getColorImageMemory() const { return colorImageMemory; }

    // The recreate function for window resizing
    void recreate(uint32_t newWidth, uint32_t newHeight) {
        width = newWidth;
        height = newHeight;
        cleanup();

        createSwapChain();
        createImageViews();
        createColorResources();
        createDepthResources();
        createFramebuffers();
    }

private:
    // Reference to the device (we assume you store device in a shared_ptr now):
    std::shared_ptr<VulkanDevice> devicePtr;
    VkSurfaceKHR surface;
    uint32_t width, height;

    VkSwapchainKHR swapChain = VK_NULL_HANDLE;
    std::vector<VkImage> swapChainImages;
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;
    std::vector<VkImageView> swapChainImageViews;

    // Possibly handle framebuffers here:
    std::vector<VkFramebuffer> swapChainFramebuffers;

    // Depth/Color resources
    VkImage depthImage          = VK_NULL_HANDLE;
    VkDeviceMemory depthImageMemory = VK_NULL_HANDLE;
    VkImageView depthImageView  = VK_NULL_HANDLE;

    VkImage colorImage          = VK_NULL_HANDLE;
    VkDeviceMemory colorImageMemory = VK_NULL_HANDLE;
    VkImageView colorImageView  = VK_NULL_HANDLE;

private:
    // Clean up the old swap chain & images
    void cleanup() {
        // Destroy framebuffers
        for (auto framebuffer : swapChainFramebuffers) {
            vkDestroyFramebuffer(device(), framebuffer, nullptr);
        }

        // Destroy image views
        for (auto imageView : swapChainImageViews) {
            vkDestroyImageView(device(), imageView, nullptr);
        }

        // Destroy depth resources
        vkDestroyImageView(device(), depthImageView, nullptr);
        vkDestroyImage(device(), depthImage, nullptr);
        vkFreeMemory(device(), depthImageMemory, nullptr);

        // Destroy color resources
        vkDestroyImageView(device(), colorImageView, nullptr);
        vkDestroyImage(device(), colorImage, nullptr);
        vkFreeMemory(device(), colorImageMemory, nullptr);

        // Destroy swap chain
        if (swapChain != VK_NULL_HANDLE) {
            vkDestroySwapchainKHR(device(), swapChain, nullptr);
        }
    }

    // Helper to avoid writing devicePtr->getDevice() repeatedly
    VkDevice device() const { return devicePtr->getDevice(); }

    // We replicate your original createSwapChain() logic here:
    void createSwapChain() {
        // Query swap chain support from your VulkanDevice, or re-use the same approach:
        SwapChainSupportDetails swapChainSupport = devicePtr->querySwapChainSupport(devicePtr->getPhysicalDevice());

        VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
        VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
        VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

        // see if newWidth/newHeight are stored in "extent"
        uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
        if (swapChainSupport.capabilities.maxImageCount > 0 &&
            imageCount > swapChainSupport.capabilities.maxImageCount) {
            imageCount = swapChainSupport.capabilities.maxImageCount;
        }

        VkSwapchainCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = surface;
        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = surfaceFormat.format;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        createInfo.imageExtent = extent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        QueueFamilyIndices indices = devicePtr->getQueueFamilyIndices();
        uint32_t queueFamilyIndices[] = {
            indices.graphicsFamily.value(),
            indices.presentFamily.value()
        };

        if (indices.graphicsFamily != indices.presentFamily) {
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices = queueFamilyIndices;
        } else {
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        }

        createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo.presentMode = presentMode;
        createInfo.clipped = VK_TRUE;
        createInfo.oldSwapchain = VK_NULL_HANDLE;

        if (vkCreateSwapchainKHR(device(), &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
            throw std::runtime_error("failed to create swap chain!");
        }

        vkGetSwapchainImagesKHR(device(), swapChain, &imageCount, nullptr);
        swapChainImages.resize(imageCount);
        vkGetSwapchainImagesKHR(device(), swapChain, &imageCount, swapChainImages.data());

        swapChainImageFormat = surfaceFormat.format;
        swapChainExtent = extent;
    }

    void createImageViews() {
        swapChainImageViews.resize(swapChainImages.size());

        for (size_t i = 0; i < swapChainImages.size(); i++) {
            VkImageViewCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            createInfo.image = swapChainImages[i];
            createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            createInfo.format = swapChainImageFormat;
            createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            createInfo.subresourceRange.baseMipLevel = 0;
            createInfo.subresourceRange.levelCount = 1;
            createInfo.subresourceRange.baseArrayLayer = 0;
            createInfo.subresourceRange.layerCount = 1;

            if (vkCreateImageView(device(), &createInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS) {
                throw std::runtime_error("failed to create image views!");
            }
        }
    }

    // If you want to create a color resource for MSAA
    void createColorResources() {
        if (devicePtr->getMsaaSamples() == VK_SAMPLE_COUNT_1_BIT) {
            // No MSAA needed, skip
            return;
        }
        VkFormat colorFormat = swapChainImageFormat;

        createImage(swapChainExtent.width,
                    swapChainExtent.height,
                    devicePtr->getMsaaSamples(),
                    colorFormat,
                    VK_IMAGE_TILING_OPTIMAL,
                    VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                    colorImage,
                    colorImageMemory);

        colorImageView = createImageView(colorImage,
                                         colorFormat,
                                         VK_IMAGE_ASPECT_COLOR_BIT,
                                         1 /*mipLevels*/);
    }

    void createDepthResources() {
        VkFormat depthFormat = findDepthFormat();
        createImage(swapChainExtent.width,
                    swapChainExtent.height,
                    devicePtr->getMsaaSamples(),
                    depthFormat,
                    VK_IMAGE_TILING_OPTIMAL,
                    VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                    depthImage,
                    depthImageMemory);

        depthImageView = createImageView(depthImage,
                                         depthFormat,
                                         VK_IMAGE_ASPECT_DEPTH_BIT,
                                         1 /*mipLevels*/);
    }

    void createFramebuffers() {
        // Example for when you have color+depth+swapchain attachments
        swapChainFramebuffers.resize(swapChainImageViews.size());

        for (size_t i = 0; i < swapChainImageViews.size(); i++) {
            std::array<VkImageView, 3> attachments = {
                // 0: color
                (devicePtr->getMsaaSamples() == VK_SAMPLE_COUNT_1_BIT)
                    ? swapChainImageViews[i]
                    : colorImageView,
                // 1: depth
                depthImageView,
                // 2: if MSAA, the resolved color is the swapchain image:
                (devicePtr->getMsaaSamples() == VK_SAMPLE_COUNT_1_BIT)
                    ? VK_NULL_HANDLE /* not used if no msaa? */
                    : swapChainImageViews[i]
            };

            // If not using MSAA, you'll have a simpler attachments array
            // Adjust accordingly

            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = /* pass in or store a reference to your render pass */;
            framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
            framebufferInfo.pAttachments = attachments.data();
            framebufferInfo.width = swapChainExtent.width;
            framebufferInfo.height = swapChainExtent.height;
            framebufferInfo.layers = 1;

            if (vkCreateFramebuffer(device(), &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS) {
                throw std::runtime_error("failed to create framebuffer!");
            }
        }
    }

    // ========== Helper methods for images, formats, etc. ==========

    VkFormat findDepthFormat() {
        return findSupportedFormat(
            {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
            VK_IMAGE_TILING_OPTIMAL,
            VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
        );
    }

    VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates,
                                VkImageTiling tiling,
                                VkFormatFeatureFlags features) {
        for (VkFormat format : candidates) {
            VkFormatProperties props;
            vkGetPhysicalDeviceFormatProperties(devicePtr->getPhysicalDevice(), format, &props);

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

    // Create an image and allocate memory
    void createImage(uint32_t w, uint32_t h,
                     VkSampleCountFlagBits samples,
                     VkFormat format,
                     VkImageTiling tiling,
                     VkImageUsageFlags usage,
                     VkMemoryPropertyFlags properties,
                     VkImage& image,
                     VkDeviceMemory& imageMemory) {
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = w;
        imageInfo.extent.height = h;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = format;
        imageInfo.tiling = tiling;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = usage;
        imageInfo.samples = samples;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateImage(device(), &imageInfo, nullptr, &image) != VK_SUCCESS) {
            throw std::runtime_error("failed to create image!");
        }

        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(device(), image, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = devicePtr->findMemoryType(memRequirements.memoryTypeBits, properties);

        if (vkAllocateMemory(device(), &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate image memory!");
        }

        vkBindImageMemory(device(), image, imageMemory, 0);
    }

    // Create an image view
    VkImageView createImageView(VkImage image,
                                VkFormat format,
                                VkImageAspectFlags aspectFlags,
                                uint32_t mipLevels) {
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = format;
        viewInfo.subresourceRange.aspectMask = aspectFlags;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = mipLevels;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        VkImageView imageView;
        if (vkCreateImageView(device(), &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
            throw std::runtime_error("failed to create texture image view!");
        }
        return imageView;
    }

    // The "choose" helpers:
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
        for (const auto& availableFormat : availableFormats) {
            if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
                availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                return availableFormat;
            }
        }
        return availableFormats[0];
    }

    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
        for (const auto& mode : availablePresentModes) {
            if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
                return mode;
            }
        }
        return VK_PRESENT_MODE_FIFO_KHR; // fallback
    }

    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
        if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
            return capabilities.currentExtent;
        } else {
            // Use the app's width/height from the constructor
            VkExtent2D actualExtent = { width, height };
            actualExtent.width = std::clamp(actualExtent.width,
                                            capabilities.minImageExtent.width,
                                            capabilities.maxImageExtent.width);
            actualExtent.height = std::clamp(actualExtent.height,
                                             capabilities.minImageExtent.height,
                                             capabilities.maxImageExtent.height);
            return actualExtent;
        }
    }
};
