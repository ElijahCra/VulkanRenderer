//
// Created by Elijah Crain on 11/30/24.
//

#ifndef BUFFER_HPP
#define BUFFER_HPP

#include <vulkan/vulkan.h>
#include <vector>

class Device;

class Buffer {
  public:
  Buffer(Device* device, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);
  ~Buffer();

  VkBuffer getBuffer() const;
  VkDeviceMemory getBufferMemory() const;

  void copyFrom(const Buffer& srcBuffer, VkDeviceSize size);

  private:
  Device* device;
  VkBuffer buffer;
  VkDeviceMemory bufferMemory;

  void allocateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);
  uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
};

#endif // BUFFER_HPP

