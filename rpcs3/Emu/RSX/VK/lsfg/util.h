// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2026 The Simpsons Game PS5 contributors
// ReX adapter for the small helper interface used by Eden/lsfg-vk.
#pragma once
#include "vulkan_adapter.h"

namespace rex::ui::vulkan::framegen {
vk::ImageView CreateWrappedImageView(const Device& device, vk::Image& image, VkFormat format);
vk::ShaderModule CreateWrappedShaderModule(const Device& device, std::span<const u32> code);
vk::DescriptorPool CreateWrappedDescriptorPool(const Device& device, size_t max_descriptors,
                                               size_t max_sets,
                                               std::initializer_list<VkDescriptorType> types);
vk::DescriptorSetLayout CreateWrappedDescriptorSetLayout(const Device& device,
                                                         std::span<const VkDescriptorType> types,
                                                         VkShaderStageFlags stages);
vk::DescriptorSets CreateWrappedDescriptorSets(vk::DescriptorPool& pool,
                                               std::span<const VkDescriptorSetLayout> layouts);
vk::PipelineLayout CreateWrappedPipelineLayout(const Device& device,
                                               const vk::DescriptorSetLayout& layout);
vk::Pipeline CreateWrappedComputePipeline(const Device& device, const vk::PipelineLayout& layout,
                                          VkShaderModule shader);
}  // namespace rex::ui::vulkan::framegen
