//
// Created by Elijah Crain on 11/30/24.
//

#ifndef IMAGE_HPP
#define IMAGE_HPP

#include <vulkan/vulkan.h>

class Device;

class Image {
  public:
  Image(Device* device, uint32_t width, uint32_t height, VkFormat format,
        VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties);
  ~Image();

  VkImage getImage() const;
  VkDeviceMemory getImageMemory() const;
  VkImageView createImageView(VkFormat format, VkImageAspectFlags aspectFlags) const;

  private:
  Device* device;
  VkImage image;
  VkDeviceMemory imageMemory;

  void allocateImage(uint32_t width, uint32_t height, VkFormat format,
                     VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties);
};

#endif // IMAGE_HPP

