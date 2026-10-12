// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

// SPDX-FileCopyrightText: Copyright 2025 lsfg-vk
// SPDX-License-Identifier: GPL-3.0-or-later

// Adapted for ReX/PS5 in 2026; origin Eden 5f142c7926d0c7fcbbd0ce30794d72f638a43b2a.

#include <algorithm>
#include <cstring>

#include "lsfg_common.h"
#include "lsfg_shaders.h"
#include "util.h"
#include "vulkan_adapter.h"

namespace rex::ui::vulkan::framegen {

namespace {

constexpr u32 DESCRIPTORS_PER_TYPE = 4096;

struct LsfgConstants {
    std::array<u32, 2> input_offset;
    u32 first_iter;
    u32 first_iter_s;
    u32 advanced_color_kind;
    u32 hdr_support;
    f32 resolution_inv_scale;
    f32 timestamp;
    f32 ui_threshold;
    std::array<u32, 3> padding;
};
static_assert(sizeof(LsfgConstants) == 48);

vk::Image CreateChainImage(MemoryAllocator& memory_allocator, VkExtent2D extent, VkFormat format) {
    const VkImageCreateInfo image_ci{
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = format,
        .extent = {.width = extent.width, .height = extent.height, .depth = 1},
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                 VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = nullptr,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };
    return memory_allocator.CreateImage(image_ci);
}

vk::Buffer CreateUniformBuffer(MemoryAllocator& memory_allocator, VkDeviceSize size) {
    const VkBufferCreateInfo buffer_ci{
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .size = size,
        .usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = nullptr,
    };
    return memory_allocator.CreateBuffer(buffer_ci, MemoryUsage::Upload);
}

VkImageMemoryBarrier MakeBarrier(const LsfgImage& image, VkAccessFlags src_access,
                                 VkAccessFlags dst_access) {
    return VkImageMemoryBarrier{
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .pNext = nullptr,
        .srcAccessMask = src_access,
        .dstAccessMask = dst_access,
        .oldLayout = image.Layout(),
        .newLayout = VK_IMAGE_LAYOUT_GENERAL,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = image.Handle(),
        .subresourceRange{
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
        },
    };
}

} // Anonymous namespace

LsfgImage::LsfgImage(const Device& device, MemoryAllocator& memory_allocator, VkExtent2D extent_,
                     VkFormat format_)
    : extent{std::max(1u, extent_.width), std::max(1u, extent_.height)}, format{format_} {
    image = CreateChainImage(memory_allocator, extent, format);
    view = CreateWrappedImageView(device, image, format);
}

LsfgBarriers& LsfgBarriers::Push(LsfgImage& image, VkAccessFlags src_access,
                                 VkAccessFlags dst_access) {
    barriers.push_back(MakeBarrier(image, src_access, dst_access));
    image.SetLayout(VK_IMAGE_LAYOUT_GENERAL);
    return *this;
}

LsfgBarriers& LsfgBarriers::WriteToRead(LsfgImage& image) {
    return Push(image, VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT);
}

LsfgBarriers& LsfgBarriers::ReadToWrite(LsfgImage& image) {
    return Push(image, VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_SHADER_WRITE_BIT);
}

LsfgBarriers& LsfgBarriers::WriteToRead(LsfgImage* image) {
    return image == nullptr ? *this : WriteToRead(*image);
}

LsfgBarriers& LsfgBarriers::ReadToWrite(LsfgImage* image) {
    return image == nullptr ? *this : ReadToWrite(*image);
}

LsfgBarriers& LsfgBarriers::DiscardToWrite(VkImage image) {
    barriers.push_back(VkImageMemoryBarrier{
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .pNext = nullptr,
        // Discard contents, but wait for the previous presenter sample before
        // writing the same output image again on the graphics queue.
        .srcAccessMask = VK_ACCESS_SHADER_READ_BIT,
        .dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .newLayout = VK_IMAGE_LAYOUT_GENERAL,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = image,
        .subresourceRange{
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
        },
    });
    return *this;
}

VkDeviceSize LsfgResources::BufferSize() {
    return sizeof(LsfgConstants);
}

VkSampler LsfgResources::GetSampler(VkSamplerAddressMode address_mode, VkCompareOp compare_op,
                                    bool white_border) {
    const u64 key = static_cast<u64>(address_mode) | (static_cast<u64>(compare_op) << 8) |
                    (static_cast<u64>(white_border) << 16);

    const auto it = samplers.find(key);
    if (it != samplers.end()) {
        return *it->second;
    }

    const auto [entry, inserted] =
        samplers.emplace(key, CreateLsfgSampler(*device, address_mode, compare_op, white_border));
    return *entry->second;
}

VkBuffer LsfgResources::GetBuffer(f32 timestamp, bool first_iter, bool first_iter_s) {
    u32 timestamp_bits{};
    std::memcpy(&timestamp_bits, &timestamp, sizeof(timestamp_bits));
    const u64 key = static_cast<u64>(timestamp_bits) | (static_cast<u64>(first_iter) << 32) |
                    (static_cast<u64>(first_iter_s) << 33);

    const auto it = buffers.find(key);
    if (it != buffers.end()) {
        return *it->second;
    }

    vk::Buffer buffer = CreateUniformBuffer(*memory_allocator, sizeof(LsfgConstants));

    const LsfgConstants constants{
        .input_offset = {0, 0},
        .first_iter = first_iter ? 1u : 0u,
        .first_iter_s = first_iter_s ? 1u : 0u,
        .advanced_color_kind = 0,
        .hdr_support = 0,
        .resolution_inv_scale = 1.0f / flow_scale,
        .timestamp = timestamp,
        .ui_threshold = 0.5f,
        .padding = {0, 0, 0},
    };

    const std::span<u8> mapped = buffer.Mapped();
    std::memcpy(mapped.data(), &constants, sizeof(constants));
    buffer.Flush();

    const auto [entry, inserted] = buffers.emplace(key, std::move(buffer));
    return *entry->second;
}

void LsfgBarriers::Build() {
    if (barriers.empty()) {
        return;
    }
    cmdbuf.PipelineBarrier(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT |
                               VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                           VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, {}, {}, barriers);
    barriers.clear();
}

LsfgDescriptorWriter& LsfgDescriptorWriter::PushImage(VkDescriptorType type, VkSampler sampler,
                                                      VkImageView view) {
    image_infos.push_back(VkDescriptorImageInfo{
        .sampler = sampler,
        .imageView = view,
        .imageLayout = view == VK_NULL_HANDLE ? VK_IMAGE_LAYOUT_UNDEFINED
                                              : VK_IMAGE_LAYOUT_GENERAL,
    });
    writes.push_back(VkWriteDescriptorSet{
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .pNext = nullptr,
        .dstSet = set,
        .dstBinding = binding++,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = type,
        .pImageInfo = &image_infos.back(),
        .pBufferInfo = nullptr,
        .pTexelBufferView = nullptr,
    });
    return *this;
}

LsfgDescriptorWriter& LsfgDescriptorWriter::AddSampler(VkSampler sampler) {
    return PushImage(VK_DESCRIPTOR_TYPE_SAMPLER, sampler, VK_NULL_HANDLE);
}

LsfgDescriptorWriter& LsfgDescriptorWriter::AddSampledImage(const LsfgImage& image) {
    return PushImage(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_NULL_HANDLE, image.View());
}

LsfgDescriptorWriter& LsfgDescriptorWriter::AddSampledImage(const LsfgImage* image) {
    return PushImage(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_NULL_HANDLE,
                     image == nullptr ? VK_NULL_HANDLE : image->View());
}

LsfgDescriptorWriter& LsfgDescriptorWriter::AddStorageImage(const LsfgImage& image) {
    return PushImage(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_NULL_HANDLE, image.View());
}

LsfgDescriptorWriter& LsfgDescriptorWriter::AddStorageView(VkImageView view) {
    return PushImage(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_NULL_HANDLE, view);
}

LsfgDescriptorWriter& LsfgDescriptorWriter::AddUniformBuffer(VkBuffer buffer, VkDeviceSize size) {
    buffer_infos.push_back(VkDescriptorBufferInfo{
        .buffer = buffer,
        .offset = 0,
        .range = size,
    });
    writes.push_back(VkWriteDescriptorSet{
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .pNext = nullptr,
        .dstSet = set,
        .dstBinding = binding++,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .pImageInfo = nullptr,
        .pBufferInfo = &buffer_infos.back(),
        .pTexelBufferView = nullptr,
    });
    return *this;
}

void LsfgDescriptorWriter::Build(const Device& device) {
    if (writes.empty()) {
        return;
    }
    device.GetLogical().UpdateDescriptorSets(writes, {});
    writes.clear();
}

LsfgPass::LsfgPass(const Device& device, const LsfgShaders& shaders, u32 shader_id,
                   LsfgBindings bindings) {
    std::vector<VkDescriptorType> types;
    for (const auto& [count, type] : bindings) {
        types.insert(types.end(), count, type);
    }
    descriptor_count = static_cast<u32>(types.size());
    shaders.ValidateInterface(shader_id, types);

    descriptor_set_layout = CreateWrappedDescriptorSetLayout(
        device, std::span<const VkDescriptorType>{types}, VK_SHADER_STAGE_COMPUTE_BIT);
    pipeline_layout = CreateWrappedPipelineLayout(device, descriptor_set_layout);
    pipeline = CreateWrappedComputePipeline(device, pipeline_layout, shaders.Get(shader_id));
}

void LsfgPass::Bind(vk::CommandBuffer cmdbuf, VkDescriptorSet set) const {
    BindPipeline(cmdbuf);
    BindSet(cmdbuf, set);
}

void LsfgPass::BindPipeline(vk::CommandBuffer cmdbuf) const {
    cmdbuf.BindPipeline(VK_PIPELINE_BIND_POINT_COMPUTE, *pipeline);
}

void LsfgPass::BindSet(vk::CommandBuffer cmdbuf, VkDescriptorSet set) const {
    cmdbuf.BindDescriptorSets(VK_PIPELINE_BIND_POINT_COMPUTE, *pipeline_layout, 0, set, {});
}

vk::DescriptorPool CreateLsfgDescriptorPool(const Device& device, u32 max_sets) {
    return CreateWrappedDescriptorPool(
        device, DESCRIPTORS_PER_TYPE, max_sets,
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_DESCRIPTOR_TYPE_SAMPLER,
         VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE});
}

vk::Sampler CreateLsfgSampler(const Device& device, VkSamplerAddressMode address_mode,
                              VkCompareOp compare_op, bool white_border) {
    return device.GetLogical().CreateSampler(VkSamplerCreateInfo{
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .magFilter = VK_FILTER_LINEAR,
        .minFilter = VK_FILTER_LINEAR,
        .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
        .addressModeU = address_mode,
        .addressModeV = address_mode,
        .addressModeW = address_mode,
        .mipLodBias = 0.0f,
        .anisotropyEnable = VK_FALSE,
        .maxAnisotropy = 0.0f,
        .compareEnable = VK_FALSE,
        .compareOp = compare_op,
        .minLod = 0.0f,
        .maxLod = VK_LOD_CLAMP_NONE,
        .borderColor = white_border ? VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE
                                    : VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK,
        .unnormalizedCoordinates = VK_FALSE,
    });
}

} // namespace rex::ui::vulkan::framegen
