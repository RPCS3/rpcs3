// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2026 The Simpsons Game PS5 contributors
// Small ReX Vulkan boundary for the GPL Eden/lsfg-vk compute stages.
#pragma once

#include <algorithm>
#include <array>
#include <span>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "device_shim.h" // PS5_RPCS3: RPCS3's device, not ReXGlue's

#include "types.h"

namespace rex::ui::vulkan::framegen {

void Check(VkResult result, const char* operation);

struct AllocationBudget {
  VkDeviceSize limit = VkDeviceSize(512) << 20;
  VkDeviceSize used = 0;
  VkDeviceSize peak = 0;
  uint32_t count = 0;
  uint32_t maximum_count = 0;
};

namespace vk {

template <typename T, auto DestroyFunction>
class Object {
 public:
  Object() = default;
  Object(const VulkanDevice& device, T handle) : device_(&device), handle_(handle) {}
  ~Object() { Reset(); }
  Object(const Object&) = delete;
  Object& operator=(const Object&) = delete;
  Object(Object&& other) noexcept { Swap(other); }
  Object& operator=(Object&& other) noexcept {
    Object temporary(std::move(other));
    Swap(temporary);
    return *this;
  }
  void Swap(Object& other) noexcept {
    std::swap(device_, other.device_);
    std::swap(handle_, other.handle_);
  }
  void Reset() noexcept {
    if (handle_ != VK_NULL_HANDLE) {
      (device_->functions().*DestroyFunction)(device_->device(), handle_, nullptr);
      handle_ = VK_NULL_HANDLE;
    }
  }
  [[nodiscard]] T operator*() const { return handle_; }
  [[nodiscard]] const T* address() const { return &handle_; }
  [[nodiscard]] explicit operator bool() const { return handle_ != VK_NULL_HANDLE; }
  [[nodiscard]] const VulkanDevice& Native() const { return *device_; }

 private:
  const VulkanDevice* device_ = nullptr;
  T handle_ = VK_NULL_HANDLE;
};

using ImageHandle = Object<VkImage, &VulkanDevice::Functions::vkDestroyImage>;
using BufferHandle = Object<VkBuffer, &VulkanDevice::Functions::vkDestroyBuffer>;
using ImageView = Object<VkImageView, &VulkanDevice::Functions::vkDestroyImageView>;
using Sampler = Object<VkSampler, &VulkanDevice::Functions::vkDestroySampler>;
using DescriptorPool = Object<VkDescriptorPool, &VulkanDevice::Functions::vkDestroyDescriptorPool>;
using DescriptorSetLayout = Object<VkDescriptorSetLayout,
                                   &VulkanDevice::Functions::vkDestroyDescriptorSetLayout>;
using PipelineLayout = Object<VkPipelineLayout, &VulkanDevice::Functions::vkDestroyPipelineLayout>;
using Pipeline = Object<VkPipeline, &VulkanDevice::Functions::vkDestroyPipeline>;
using ShaderModule = Object<VkShaderModule, &VulkanDevice::Functions::vkDestroyShaderModule>;
// All sets live until their owning pool is destroyed; no individual free and no
// shared pool across a resize or a pending submission.
using DescriptorSets = std::vector<VkDescriptorSet>;

class MemoryAllocation {
 public:
  MemoryAllocation() = default;
  MemoryAllocation(const VulkanDevice& device, VkDeviceMemory memory, VkDeviceSize size,
                   AllocationBudget& budget, bool coherent);
  ~MemoryAllocation();
  MemoryAllocation(const MemoryAllocation&) = delete;
  MemoryAllocation& operator=(const MemoryAllocation&) = delete;
  MemoryAllocation(MemoryAllocation&& other) noexcept { Swap(other); }
  MemoryAllocation& operator=(MemoryAllocation&& other) noexcept {
    MemoryAllocation temporary(std::move(other));
    Swap(temporary);
    return *this;
  }
  void Swap(MemoryAllocation& other) noexcept;
  void Map();
  void Flush() const;
  [[nodiscard]] VkDeviceMemory Handle() const { return memory_; }
  [[nodiscard]] std::span<u8> Mapped() const { return {mapped_, size_t(size_)}; }

 private:
  const VulkanDevice* device_ = nullptr;
  VkDeviceMemory memory_ = VK_NULL_HANDLE;
  VkDeviceSize size_ = 0;
  AllocationBudget* budget_ = nullptr;
  u8* mapped_ = nullptr;
  bool coherent_ = false;
};

class Image {
 public:
  Image() = default;
  Image(MemoryAllocation memory, ImageHandle image)
      : memory_(std::move(memory)), image_(std::move(image)) {}
  Image(const Image&) = delete;
  Image& operator=(const Image&) = delete;
  Image(Image&&) = default;
  Image& operator=(Image&& other) noexcept {
    // Destruction of the old image must precede freeing its memory.
    Image temporary(std::move(other));
    memory_.Swap(temporary.memory_);
    image_.Swap(temporary.image_);
    return *this;
  }
  [[nodiscard]] VkImage operator*() const { return *image_; }
  [[nodiscard]] VkDeviceMemory Memory() const { return memory_.Handle(); }

 private:
  MemoryAllocation memory_;
  ImageHandle image_;
};

class Buffer {
 public:
  Buffer() = default;
  Buffer(MemoryAllocation memory, BufferHandle buffer)
      : memory_(std::move(memory)), buffer_(std::move(buffer)) {}
  Buffer(const Buffer&) = delete;
  Buffer& operator=(const Buffer&) = delete;
  Buffer(Buffer&&) = default;
  Buffer& operator=(Buffer&& other) noexcept {
    Buffer temporary(std::move(other));
    memory_.Swap(temporary.memory_);
    buffer_.Swap(temporary.buffer_);
    return *this;
  }
  [[nodiscard]] VkBuffer operator*() const { return *buffer_; }
  [[nodiscard]] VkDeviceMemory Memory() const { return memory_.Handle(); }
  [[nodiscard]] std::span<u8> Mapped() const { return memory_.Mapped(); }
  void Flush() const { memory_.Flush(); }

 private:
  MemoryAllocation memory_;
  BufferHandle buffer_;
};

// Commands are recorded onto the presenter's existing graphics queue. No queue
// ownership transfers, hidden submissions, scheduler or auxiliary thread.
class CommandBuffer {
 public:
  CommandBuffer(const VulkanDevice& device, VkCommandBuffer command_buffer)
      : device_(&device), command_buffer_(command_buffer) {}
  void PipelineBarrier(VkPipelineStageFlags source, VkPipelineStageFlags destination,
                       VkDependencyFlags dependency,
                       std::span<const VkMemoryBarrier> memory,
                       std::span<const VkBufferMemoryBarrier> buffers,
                       std::span<const VkImageMemoryBarrier> images) const {
    device_->functions().vkCmdPipelineBarrier(
        command_buffer_, source, destination, dependency, uint32_t(memory.size()), memory.data(),
        uint32_t(buffers.size()), buffers.data(), uint32_t(images.size()), images.data());
  }
  void Dispatch(u32 x, u32 y, u32 z) const {
    device_->functions().vkCmdDispatch(command_buffer_, x, y, z);
  }
  void BindPipeline(VkPipelineBindPoint bind_point, VkPipeline pipeline) const {
    device_->functions().vkCmdBindPipeline(command_buffer_, bind_point, pipeline);
  }
  void BindDescriptorSets(VkPipelineBindPoint bind_point, VkPipelineLayout layout,
                          uint32_t first_set, VkDescriptorSet set,
                          std::span<const uint32_t> dynamic_offsets) const {
    device_->functions().vkCmdBindDescriptorSets(
        command_buffer_, bind_point, layout, first_set, 1, &set,
        uint32_t(dynamic_offsets.size()), dynamic_offsets.data());
  }
  [[nodiscard]] VkCommandBuffer Handle() const { return command_buffer_; }
  [[nodiscard]] const VulkanDevice& Native() const { return *device_; }

 private:
  const VulkanDevice* device_;
  VkCommandBuffer command_buffer_;
};
}  // namespace vk

class Device {
 public:
  explicit Device(const VulkanDevice& device);
  [[nodiscard]] const VulkanDevice& Native() const { return *device_; }
  [[nodiscard]] const Device& GetLogical() const { return *this; }
  [[nodiscard]] const VkPhysicalDeviceProperties& PhysicalProperties() const { return properties_; }
  [[nodiscard]] const VkPhysicalDeviceMemoryProperties& MemoryProperties() const {
    return memory_properties_;
  }
  [[nodiscard]] const VkPhysicalDeviceSubgroupProperties& SubgroupProperties() const {
    return subgroup_properties_;
  }
  [[nodiscard]] const VkPhysicalDeviceFloatControlsProperties& FloatControls() const {
    return float_controls_;
  }
  void UpdateDescriptorSets(std::span<const VkWriteDescriptorSet> writes,
                            std::span<const VkCopyDescriptorSet> copies) const;
  [[nodiscard]] vk::Sampler CreateSampler(const VkSamplerCreateInfo& create_info) const;

 private:
  const VulkanDevice* device_;
  VkPhysicalDeviceProperties properties_{};
  VkPhysicalDeviceMemoryProperties memory_properties_{};
  VkPhysicalDeviceSubgroupProperties subgroup_properties_{
      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_PROPERTIES};
  VkPhysicalDeviceFloatControlsProperties float_controls_{
      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FLOAT_CONTROLS_PROPERTIES};
};

enum class MemoryUsage { Upload };

class MemoryAllocator {
 public:
  explicit MemoryAllocator(const Device& device, VkDeviceSize budget = VkDeviceSize(512) << 20);
  MemoryAllocator(const MemoryAllocator&) = delete;
  MemoryAllocator& operator=(const MemoryAllocator&) = delete;
  [[nodiscard]] vk::Image CreateImage(const VkImageCreateInfo& create_info);
  [[nodiscard]] vk::Buffer CreateBuffer(const VkBufferCreateInfo& create_info, MemoryUsage usage);
  // One-time clear before any shader reads history. LsfgImage's initial tracked
  // layout is GENERAL to match the end of this initialization command sequence.
  void InitializeImages(vk::CommandBuffer command_buffer) const;
  [[nodiscard]] const AllocationBudget& Budget() const { return budget_; }

 private:
  [[nodiscard]] vk::MemoryAllocation Allocate(const VkMemoryRequirements& requirements,
                                               VkMemoryPropertyFlags required,
                                               VkMemoryPropertyFlags preferred);
  const Device* device_;
  AllocationBudget budget_;
  std::vector<VkImage> images_;
};
}  // namespace rex::ui::vulkan::framegen
