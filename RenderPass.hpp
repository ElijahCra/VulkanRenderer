//
// Created by Elijah Crain on 11/30/24.
//

#ifndef RENDERPASS_HPP
#define RENDERPASS_HPP
#include "Device.hpp"
#include "SwapChain.hpp"

class RenderPass {
  public:
  RenderPass(Device* device, SwapChain* swapchain);
  const VkRenderPass& getRenderPass() {
    return renderPass;
  }
  const VkDescriptorSetLayout* getDescriptorSetLayout() {
    return &descriptorSetLayout;
  }

  void createRenderPass();

  private:
  Device* device;
  SwapChain* swapchain;
  VkRenderPass renderPass;
  VkDescriptorSetLayout descriptorSetLayout;

  VkImage colorImage;
  VkDeviceMemory colorImageMemory;
  VkImageView colorImageView;

  VkImage depthImage;
  VkDeviceMemory depthImageMemory;
  VkImageView depthImageView;


  void createDescriptorSetLayout();
  VkFormat findDepthFormat();
  VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
  void createDepthResources();
};


#endif //RENDERPASS_HPP
