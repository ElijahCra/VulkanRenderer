//
// Created by Elijah Crain on 11/30/24.
//

#ifndef PIPELINE_HPP
#define PIPELINE_HPP

#include <vector>
#include <vulkan/vulkan.h>

#include "RenderPass.hpp"

class Device;
class SwapChain;

class Pipeline {
  public:
  Pipeline(Device* device, SwapChain* swapChain, RenderPass* renderPass);
  ~Pipeline();

  VkPipeline getGraphicsPipeline() const;
  VkPipelineLayout getPipelineLayout() const;
  VkShaderModule createShaderModule(const std::vector<char>& code);

  private:
  Device* device;
  SwapChain* swapChain;
  RenderPass* renderPass;

  VkPipeline graphicsPipeline;
  VkPipelineLayout pipelineLayout;

  void createGraphicsPipeline();
};

#endif // PIPELINE_HPP

