// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2026 The Simpsons Game PS5 contributors

#include "vulkan_adapter.h"
#include "util.h"

#include <limits>

namespace rex::ui::vulkan::framegen {

void Check(VkResult result, const char* operation) {
  if (result != VK_SUCCESS) {
    throw std::runtime_error(std::string(operation) + " VkResult=" +
                             std::to_string(static_cast<int>(result)));
  }
}

namespace vk {
MemoryAllocation::MemoryAllocation(const VulkanDevice& device, VkDeviceMemory memory,
                                   VkDeviceSize size, AllocationBudget& budget, bool coherent)
    : device_(&device), memory_(memory), size_(size), budget_(&budget), coherent_(coherent) {
  budget_->used += size_;
  budget_->peak = std::max(budget_->peak, budget_->used);
  ++budget_->count;
}

MemoryAllocation::~MemoryAllocation() {
  if (memory_ == VK_NULL_HANDLE) return;
  if (mapped_) device_->functions().vkUnmapMemory(device_->device(), memory_);
  device_->functions().vkFreeMemory(device_->device(), memory_, nullptr);
  budget_->used -= size_;
  --budget_->count;
}

void MemoryAllocation::Swap(MemoryAllocation& other) noexcept {
  std::swap(device_, other.device_);
  std::swap(memory_, other.memory_);
  std::swap(size_, other.size_);
  std::swap(budget_, other.budget_);
  std::swap(mapped_, other.mapped_);
  std::swap(coherent_, other.coherent_);
}

void MemoryAllocation::Map() {
  void* mapped = nullptr;
  Check(device_->functions().vkMapMemory(device_->device(), memory_, 0, VK_WHOLE_SIZE, 0, &mapped),
        "vkMapMemory");
  mapped_ = static_cast<u8*>(mapped);
  if (!mapped_) throw std::runtime_error("vkMapMemory returned no mapping");
}

void MemoryAllocation::Flush() const {
  if (coherent_) return;
  const VkMappedMemoryRange range{
      .sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
      .pNext = nullptr,
      .memory = memory_,
      .offset = 0,
      .size = VK_WHOLE_SIZE,
  };
  Check(device_->functions().vkFlushMappedMemoryRanges(device_->device(), 1, &range),
        "vkFlushMappedMemoryRanges");
}
}  // namespace vk

Device::Device(const VulkanDevice& device) : device_(&device) {
  const auto& functions = device.vulkan_instance()->functions();
  functions.vkGetPhysicalDeviceProperties(device.physical_device(), &properties_);
  functions.vkGetPhysicalDeviceMemoryProperties(device.physical_device(), &memory_properties_);
  if (device.properties().apiVersion >= VK_API_VERSION_1_1 &&
      functions.vkGetPhysicalDeviceProperties2) {
    VkPhysicalDeviceProperties2 extended{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};
    extended.pNext = &subgroup_properties_;
    if (device.properties().apiVersion >= VK_API_VERSION_1_2) {
      subgroup_properties_.pNext = &float_controls_;
    }
    functions.vkGetPhysicalDeviceProperties2(device.physical_device(), &extended);
    subgroup_properties_.pNext = nullptr;
  }
}

void Device::UpdateDescriptorSets(std::span<const VkWriteDescriptorSet> writes,
                                  std::span<const VkCopyDescriptorSet> copies) const {
  device_->functions().vkUpdateDescriptorSets(device_->device(), uint32_t(writes.size()),
                                             writes.data(), uint32_t(copies.size()), copies.data());
}

vk::Sampler Device::CreateSampler(const VkSamplerCreateInfo& create_info) const {
  VkSampler sampler = VK_NULL_HANDLE;
  Check(device_->functions().vkCreateSampler(device_->device(), &create_info, nullptr, &sampler),
        "vkCreateSampler");
  return vk::Sampler(*device_, sampler);
}

MemoryAllocator::MemoryAllocator(const Device& device, VkDeviceSize budget) : device_(&device) {
  budget_.limit = budget;
  // Leave the majority of the device's allocation count for the native game.
  budget_.maximum_count = std::max(1u, device.PhysicalProperties().limits.maxMemoryAllocationCount / 4);
}

vk::MemoryAllocation MemoryAllocator::Allocate(const VkMemoryRequirements& requirements,
                                                VkMemoryPropertyFlags required,
                                                VkMemoryPropertyFlags preferred) {
  if (requirements.size > budget_.limit - budget_.used ||
      budget_.count >= budget_.maximum_count) {
    throw std::runtime_error("framegen memory budget exceeded (512 MiB / allocation limit)");
  }
  const auto& properties = device_->MemoryProperties();
  uint32_t memory_type = UINT32_MAX;
  bool preferred_found = false;
  for (uint32_t i = 0; i < properties.memoryTypeCount; ++i) {
    if (!(requirements.memoryTypeBits & (uint32_t(1) << i))) continue;
    const auto flags = properties.memoryTypes[i].propertyFlags;
    if ((flags & required) != required) continue;
    const bool is_preferred = (flags & preferred) == preferred;
    if (memory_type == UINT32_MAX || (is_preferred && !preferred_found)) {
      memory_type = i;
      preferred_found = is_preferred;
    }
  }
  if (memory_type == UINT32_MAX) throw std::runtime_error("no compatible framegen memory type");
  const auto& native = device_->Native();
  const VkMemoryAllocateInfo allocate_info{
      .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
      .pNext = nullptr,
      .allocationSize = requirements.size,
      .memoryTypeIndex = memory_type,
  };
  VkDeviceMemory memory = VK_NULL_HANDLE;
  Check(native.functions().vkAllocateMemory(native.device(), &allocate_info, nullptr, &memory),
        "vkAllocateMemory");
  return vk::MemoryAllocation(native, memory, requirements.size, budget_,
                              (properties.memoryTypes[memory_type].propertyFlags &
                               VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) != 0);
}

vk::Image MemoryAllocator::CreateImage(const VkImageCreateInfo& create_info) {
  const auto& native = device_->Native();
  VkImage image = VK_NULL_HANDLE;
  Check(native.functions().vkCreateImage(native.device(), &create_info, nullptr, &image),
        "vkCreateImage");
  vk::ImageHandle owned_image(native, image);
  VkMemoryRequirements requirements{};
  native.functions().vkGetImageMemoryRequirements(native.device(), image, &requirements);
  auto memory = Allocate(requirements, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 0);
  vk::Image result(std::move(memory), std::move(owned_image));
  Check(native.functions().vkBindImageMemory(native.device(), *result, result.Memory(), 0),
        "vkBindImageMemory");
  images_.push_back(*result);
  return result;
}

vk::Buffer MemoryAllocator::CreateBuffer(const VkBufferCreateInfo& create_info, MemoryUsage) {
  const auto& native = device_->Native();
  VkBuffer buffer = VK_NULL_HANDLE;
  Check(native.functions().vkCreateBuffer(native.device(), &create_info, nullptr, &buffer),
        "vkCreateBuffer");
  vk::BufferHandle owned_buffer(native, buffer);
  VkMemoryRequirements requirements{};
  native.functions().vkGetBufferMemoryRequirements(native.device(), buffer, &requirements);
  auto memory = Allocate(requirements, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                         VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
  memory.Map();
  vk::Buffer result(std::move(memory), std::move(owned_buffer));
  Check(native.functions().vkBindBufferMemory(native.device(), *result, result.Memory(), 0),
        "vkBindBufferMemory");
  return result;
}

void MemoryAllocator::InitializeImages(vk::CommandBuffer command_buffer) const {
  const auto& native = device_->Native();
  const VkImageSubresourceRange range{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
  const VkClearColorValue zero{};
  for (VkImage image : images_) {
    VkImageMemoryBarrier barrier{
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .pNext = nullptr,
        .srcAccessMask = 0,
        .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = image,
        .subresourceRange = range,
    };
    command_buffer.PipelineBarrier(VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                                    0, {}, {}, std::span(&barrier, 1));
    native.functions().vkCmdClearColorImage(command_buffer.Handle(), image,
                                            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &zero, 1, &range);
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
    command_buffer.PipelineBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                                    0, {}, {}, std::span(&barrier, 1));
  }
}

vk::ImageView CreateWrappedImageView(const Device& device, vk::Image& image, VkFormat format) {
  const VkImageViewCreateInfo create_info{
      .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .image = *image,
      .viewType = VK_IMAGE_VIEW_TYPE_2D,
      .format = format,
      .components = {},
      .subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1},
  };
  VkImageView view = VK_NULL_HANDLE;
  Check(device.Native().functions().vkCreateImageView(device.Native().device(), &create_info,
                                                      nullptr, &view), "vkCreateImageView");
  return vk::ImageView(device.Native(), view);
}

vk::ShaderModule CreateWrappedShaderModule(const Device& device, std::span<const u32> code) {
  const VkShaderModuleCreateInfo create_info{
      .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .codeSize = code.size_bytes(),
      .pCode = code.data(),
  };
  VkShaderModule module = VK_NULL_HANDLE;
  Check(device.Native().functions().vkCreateShaderModule(device.Native().device(), &create_info,
                                                         nullptr, &module), "vkCreateShaderModule");
  return vk::ShaderModule(device.Native(), module);
}

vk::DescriptorPool CreateWrappedDescriptorPool(const Device& device, size_t max_descriptors,
                                               size_t max_sets,
                                               std::initializer_list<VkDescriptorType> types) {
  std::vector<VkDescriptorPoolSize> sizes;
  for (auto type : types) sizes.push_back({type, uint32_t(max_descriptors)});
  const VkDescriptorPoolCreateInfo create_info{
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .maxSets = uint32_t(max_sets),
      .poolSizeCount = uint32_t(sizes.size()),
      .pPoolSizes = sizes.data(),
  };
  VkDescriptorPool pool = VK_NULL_HANDLE;
  Check(device.Native().functions().vkCreateDescriptorPool(device.Native().device(), &create_info,
                                                           nullptr, &pool), "vkCreateDescriptorPool");
  return vk::DescriptorPool(device.Native(), pool);
}

vk::DescriptorSetLayout CreateWrappedDescriptorSetLayout(const Device& device,
                                                         std::span<const VkDescriptorType> types,
                                                         VkShaderStageFlags stages) {
  std::vector<VkDescriptorSetLayoutBinding> bindings;
  for (uint32_t i = 0; i < types.size(); ++i) {
    bindings.push_back({i, types[i], 1, stages, nullptr});
  }
  const VkDescriptorSetLayoutCreateInfo create_info{
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .bindingCount = uint32_t(bindings.size()),
      .pBindings = bindings.data(),
  };
  VkDescriptorSetLayout layout = VK_NULL_HANDLE;
  Check(device.Native().functions().vkCreateDescriptorSetLayout(
            device.Native().device(), &create_info, nullptr, &layout), "vkCreateDescriptorSetLayout");
  return vk::DescriptorSetLayout(device.Native(), layout);
}

vk::DescriptorSets CreateWrappedDescriptorSets(vk::DescriptorPool& pool,
                                               std::span<const VkDescriptorSetLayout> layouts) {
  vk::DescriptorSets sets(layouts.size(), VK_NULL_HANDLE);
  const VkDescriptorSetAllocateInfo allocate_info{
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
      .pNext = nullptr,
      .descriptorPool = *pool,
      .descriptorSetCount = uint32_t(layouts.size()),
      .pSetLayouts = layouts.data(),
  };
  Check(pool.Native().functions().vkAllocateDescriptorSets(pool.Native().device(), &allocate_info,
                                                            sets.data()), "vkAllocateDescriptorSets");
  return sets;
}

vk::PipelineLayout CreateWrappedPipelineLayout(const Device& device,
                                               const vk::DescriptorSetLayout& set_layout) {
  const VkPipelineLayoutCreateInfo create_info{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .setLayoutCount = 1,
      .pSetLayouts = set_layout.address(),
      .pushConstantRangeCount = 0,
      .pPushConstantRanges = nullptr,
  };
  VkPipelineLayout layout = VK_NULL_HANDLE;
  Check(device.Native().functions().vkCreatePipelineLayout(device.Native().device(), &create_info,
                                                           nullptr, &layout), "vkCreatePipelineLayout");
  return vk::PipelineLayout(device.Native(), layout);
}

vk::Pipeline CreateWrappedComputePipeline(const Device& device, const vk::PipelineLayout& layout,
                                          VkShaderModule shader) {
  if (!shader) throw std::runtime_error("missing LSFG shader module");
  const VkComputePipelineCreateInfo create_info{
      .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .stage = {
          .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
          .pNext = nullptr,
          .flags = 0,
          .stage = VK_SHADER_STAGE_COMPUTE_BIT,
          .module = shader,
          .pName = "main",
          .pSpecializationInfo = nullptr,
      },
      .layout = *layout,
      .basePipelineHandle = VK_NULL_HANDLE,
      .basePipelineIndex = -1,
  };
  VkPipeline pipeline = VK_NULL_HANDLE;
  const VkResult result = device.Native().functions().vkCreateComputePipelines(
      device.Native().device(), VK_NULL_HANDLE, 1, &create_info, nullptr, &pipeline);
  // Vulkan permits partial pipeline creation on failure: adopt before checking.
  vk::Pipeline owned(device.Native(), pipeline);
  Check(result, "vkCreateComputePipelines");
  return owned;
}
}  // namespace rex::ui::vulkan::framegen
