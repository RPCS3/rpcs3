// SPDX-License-Identifier: GPL-3.0-or-later
// PS5_RPCS3: the little of ReXGlue's VulkanDevice that the Eden/lsfg-vk compute
// chain (vulkan_adapter.h) uses, over RPCS3's device: its handles, what it was
// created with (vkutils/device.cpp, VKFrameGenLSFG.h), and the commands volk
// has loaded (every RPCS3 unit is compiled with volk.h and VK_NO_PROTOTYPES)
#pragma once

#include <cstdint>

namespace rex::ui::vulkan {

// Of the device's enabled features, those the shader loader checks
struct VulkanDeviceProperties {
  uint32_t apiVersion = 0;
  bool vulkanMemoryModel = false;
  bool vulkanMemoryModelDeviceScope = false;
  bool nullDescriptor = false;
  bool shaderFloat16 = false;
  bool shaderStorageImageExtendedFormats = false;
  bool shaderStorageImageReadWithoutFormat = false;
  bool shaderStorageImageWriteWithoutFormat = false;
};

class VulkanInstance {
 public:
  struct Functions {
    PFN_vkGetPhysicalDeviceProperties vkGetPhysicalDeviceProperties = ::vkGetPhysicalDeviceProperties;
    PFN_vkGetPhysicalDeviceMemoryProperties vkGetPhysicalDeviceMemoryProperties = ::vkGetPhysicalDeviceMemoryProperties;
    PFN_vkGetPhysicalDeviceProperties2 vkGetPhysicalDeviceProperties2 = ::vkGetPhysicalDeviceProperties2;
    PFN_vkGetPhysicalDeviceFormatProperties vkGetPhysicalDeviceFormatProperties = ::vkGetPhysicalDeviceFormatProperties;
  };

  explicit VulkanInstance(uint32_t api_version) : api_version_(api_version) {}
  [[nodiscard]] const Functions& functions() const { return functions_; }
  [[nodiscard]] uint32_t api_version() const { return api_version_; }

 private:
  Functions functions_;
  uint32_t api_version_;
};

class VulkanDevice {
 public:
  struct Functions {
    PFN_vkAllocateDescriptorSets vkAllocateDescriptorSets = ::vkAllocateDescriptorSets;
    PFN_vkAllocateMemory vkAllocateMemory = ::vkAllocateMemory;
    PFN_vkBindBufferMemory vkBindBufferMemory = ::vkBindBufferMemory;
    PFN_vkBindImageMemory vkBindImageMemory = ::vkBindImageMemory;
    PFN_vkCmdBindDescriptorSets vkCmdBindDescriptorSets = ::vkCmdBindDescriptorSets;
    PFN_vkCmdBindPipeline vkCmdBindPipeline = ::vkCmdBindPipeline;
    PFN_vkCmdBlitImage vkCmdBlitImage = ::vkCmdBlitImage;
    PFN_vkCmdClearColorImage vkCmdClearColorImage = ::vkCmdClearColorImage;
    PFN_vkCmdDispatch vkCmdDispatch = ::vkCmdDispatch;
    PFN_vkCmdPipelineBarrier vkCmdPipelineBarrier = ::vkCmdPipelineBarrier;
    PFN_vkCreateBuffer vkCreateBuffer = ::vkCreateBuffer;
    PFN_vkCreateComputePipelines vkCreateComputePipelines = ::vkCreateComputePipelines;
    PFN_vkCreateDescriptorPool vkCreateDescriptorPool = ::vkCreateDescriptorPool;
    PFN_vkCreateDescriptorSetLayout vkCreateDescriptorSetLayout = ::vkCreateDescriptorSetLayout;
    PFN_vkCreateImage vkCreateImage = ::vkCreateImage;
    PFN_vkCreateImageView vkCreateImageView = ::vkCreateImageView;
    PFN_vkCreatePipelineLayout vkCreatePipelineLayout = ::vkCreatePipelineLayout;
    PFN_vkCreateSampler vkCreateSampler = ::vkCreateSampler;
    PFN_vkCreateShaderModule vkCreateShaderModule = ::vkCreateShaderModule;
    PFN_vkDestroyBuffer vkDestroyBuffer = ::vkDestroyBuffer;
    PFN_vkDestroyDescriptorPool vkDestroyDescriptorPool = ::vkDestroyDescriptorPool;
    PFN_vkDestroyDescriptorSetLayout vkDestroyDescriptorSetLayout = ::vkDestroyDescriptorSetLayout;
    PFN_vkDestroyImage vkDestroyImage = ::vkDestroyImage;
    PFN_vkDestroyImageView vkDestroyImageView = ::vkDestroyImageView;
    PFN_vkDestroyPipeline vkDestroyPipeline = ::vkDestroyPipeline;
    PFN_vkDestroyPipelineLayout vkDestroyPipelineLayout = ::vkDestroyPipelineLayout;
    PFN_vkDestroySampler vkDestroySampler = ::vkDestroySampler;
    PFN_vkDestroyShaderModule vkDestroyShaderModule = ::vkDestroyShaderModule;
    PFN_vkFlushMappedMemoryRanges vkFlushMappedMemoryRanges = ::vkFlushMappedMemoryRanges;
    PFN_vkFreeMemory vkFreeMemory = ::vkFreeMemory;
    PFN_vkGetBufferMemoryRequirements vkGetBufferMemoryRequirements = ::vkGetBufferMemoryRequirements;
    PFN_vkGetImageMemoryRequirements vkGetImageMemoryRequirements = ::vkGetImageMemoryRequirements;
    PFN_vkMapMemory vkMapMemory = ::vkMapMemory;
    PFN_vkUnmapMemory vkUnmapMemory = ::vkUnmapMemory;
    PFN_vkUpdateDescriptorSets vkUpdateDescriptorSets = ::vkUpdateDescriptorSets;
  };

  VulkanDevice(VkDevice device, VkPhysicalDevice physical_device,
               const VulkanDeviceProperties& properties)
      : device_(device), physical_device_(physical_device), properties_(properties),
        instance_(properties.apiVersion) {}

  [[nodiscard]] const Functions& functions() const { return functions_; }
  [[nodiscard]] VkDevice device() const { return device_; }
  [[nodiscard]] VkPhysicalDevice physical_device() const { return physical_device_; }
  [[nodiscard]] const VulkanDeviceProperties& properties() const { return properties_; }
  [[nodiscard]] const VulkanInstance* vulkan_instance() const { return &instance_; }

 private:
  Functions functions_;
  VkDevice device_;
  VkPhysicalDevice physical_device_;
  VulkanDeviceProperties properties_;
  VulkanInstance instance_;
};

}  // namespace rex::ui::vulkan
