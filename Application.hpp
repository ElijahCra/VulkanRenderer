//
// Created by Elijah Crain on 11/30/24.
//

#ifndef APPLICATION_HPP
#define APPLICATION_HPP

#include "Window.hpp"
#include "Instance.hpp"
#include "Device.hpp"
#include "SwapChain.hpp"
#include "Pipeline.hpp"
#include "Buffer.hpp"
#include "Image.hpp"
#include "CommandPool.hpp"
#include "Synchronization.hpp"

class Application {
  public:
  void run();

  private:
  Window* window;
  Instance* instance;
  Device* device;
  SwapChain* swapChain;
  Pipeline* pipeline;
  CommandPool* commandPool;
  Synchronization* synchronization;

  void initVulkan();
  void mainLoop();
  void cleanup();
};

#endif // APPLICATION_HPP

