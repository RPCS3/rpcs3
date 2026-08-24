// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

// SPDX-FileCopyrightText: Copyright 2025 lsfg-vk
// SPDX-License-Identifier: GPL-3.0-or-later

#include "lsfg_common.hpp"
#include "lsfg_shaders.hpp"

#include <algorithm>
#include <cstring>

namespace lsfg {

namespace {

constexpr uint32_t DESCRIPTORS_PER_TYPE = 4096;

struct LsfgConstants {
    std::array<uint32_t, 2> input_offset;
    uint32_t first_iter;
    uint32_t first_iter_s;
    uint32_t advanced_color_kind;
    uint32_t hdr_support;
    float resolution_inv_scale;
    float timestamp;
    float ui_threshold;
    std::array<uint32_t, 3> padding;
};
static_assert(sizeof(LsfgConstants) == 48);

VkImageMemoryBarrier MakeBarrier(const LsfgImage& image, VkAccessFlags src_access,
                                 VkAccessFlags dst_access) {
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.srcAccessMask = src_access;
    barrier.dstAccessMask = dst_access;
    barrier.oldLayout = image.Layout();
    barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image.Handle();
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.layerCount = 1;
    return barrier;
}

}

Device::Device(VkDevice device_, VkPhysicalDevice physical_device_)
    : device{device_}, physical_device{physical_device_} {
    VK_GET_SYMBOL(vkGetPhysicalDeviceMemoryProperties)(physical_device, &memory_properties);
}

uint32_t Device::FindMemoryType(uint32_t bits, VkMemoryPropertyFlags properties) const {
    for (uint32_t i = 0; i < memory_properties.memoryTypeCount; i++) {
        if ((bits & (1u << i)) == 0) continue;
        if ((memory_properties.memoryTypes[i].propertyFlags & properties) == properties) return i;
    }
    return UINT32_MAX;
}

Buffer::Buffer(const Device& device_, VkDeviceSize size) : device{device_.Handle()} {
    VkBufferCreateInfo buffer_ci{};
    buffer_ci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_ci.size = size;
    buffer_ci.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    buffer_ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    if (VK_GET_SYMBOL(vkCreateBuffer)(device, &buffer_ci, nullptr, &buffer) != VK_SUCCESS) {
        buffer = VK_NULL_HANDLE;
        return;
    }

    VkMemoryRequirements requirements;
    VK_GET_SYMBOL(vkGetBufferMemoryRequirements)(device, buffer, &requirements);

    VkMemoryAllocateInfo allocate_info{};
    allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocate_info.allocationSize = requirements.size;
    allocate_info.memoryTypeIndex = device_.FindMemoryType(
        requirements.memoryTypeBits,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    if (allocate_info.memoryTypeIndex == UINT32_MAX ||
        VK_GET_SYMBOL(vkAllocateMemory)(device, &allocate_info, nullptr, &memory) != VK_SUCCESS) {
        Release();
        return;
    }

    VK_GET_SYMBOL(vkBindBufferMemory)(device, buffer, memory, 0);
    if (VK_GET_SYMBOL(vkMapMemory)(device, memory, 0, VK_WHOLE_SIZE, 0, &mapped) != VK_SUCCESS) {
        mapped = nullptr;
        Release();
    }
}

Buffer::~Buffer() {
    Release();
}

Buffer::Buffer(Buffer&& other) noexcept
    : device{other.device}, buffer{other.buffer}, memory{other.memory}, mapped{other.mapped} {
    other.device = VK_NULL_HANDLE;
    other.buffer = VK_NULL_HANDLE;
    other.memory = VK_NULL_HANDLE;
    other.mapped = nullptr;
}

Buffer& Buffer::operator=(Buffer&& other) noexcept {
    if (this != &other) {
        Release();
        device = other.device;
        buffer = other.buffer;
        memory = other.memory;
        mapped = other.mapped;
        other.device = VK_NULL_HANDLE;
        other.buffer = VK_NULL_HANDLE;
        other.memory = VK_NULL_HANDLE;
        other.mapped = nullptr;
    }
    return *this;
}

void Buffer::Upload(const void* data, size_t size) {
    if (mapped) std::memcpy(mapped, data, size);
}

void Buffer::Release() {
    if (device == VK_NULL_HANDLE) return;
    if (mapped) {
        VK_GET_SYMBOL(vkUnmapMemory)(device, memory);
        mapped = nullptr;
    }
    if (buffer) VK_GET_SYMBOL(vkDestroyBuffer)(device, buffer, nullptr);
    if (memory) VK_GET_SYMBOL(vkFreeMemory)(device, memory, nullptr);
    buffer = VK_NULL_HANDLE;
    memory = VK_NULL_HANDLE;
}

LsfgImage::LsfgImage(const Device& device_, VkExtent2D extent_, VkFormat format_)
    : device{device_.Handle()},
      extent{std::max(1u, extent_.width), std::max(1u, extent_.height)}, format{format_} {
    VkImageCreateInfo image_ci{};
    image_ci.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_ci.imageType = VK_IMAGE_TYPE_2D;
    image_ci.format = format;
    image_ci.extent = {extent.width, extent.height, 1};
    image_ci.mipLevels = 1;
    image_ci.arrayLayers = 1;
    image_ci.samples = VK_SAMPLE_COUNT_1_BIT;
    image_ci.tiling = VK_IMAGE_TILING_OPTIMAL;
    image_ci.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                     VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    image_ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    image_ci.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    if (VK_GET_SYMBOL(vkCreateImage)(device, &image_ci, nullptr, &image) != VK_SUCCESS) {
        image = VK_NULL_HANDLE;
        return;
    }

    VkMemoryRequirements requirements;
    VK_GET_SYMBOL(vkGetImageMemoryRequirements)(device, image, &requirements);

    VkMemoryAllocateInfo allocate_info{};
    allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocate_info.allocationSize = requirements.size;
    allocate_info.memoryTypeIndex =
        device_.FindMemoryType(requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    if (allocate_info.memoryTypeIndex == UINT32_MAX ||
        VK_GET_SYMBOL(vkAllocateMemory)(device, &allocate_info, nullptr, &memory) != VK_SUCCESS) {
        Release();
        return;
    }
    VK_GET_SYMBOL(vkBindImageMemory)(device, image, memory, 0);

    VkImageViewCreateInfo view_ci{};
    view_ci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view_ci.image = image;
    view_ci.viewType = VK_IMAGE_VIEW_TYPE_2D;
    view_ci.format = format;
    view_ci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    view_ci.subresourceRange.levelCount = 1;
    view_ci.subresourceRange.layerCount = 1;
    if (VK_GET_SYMBOL(vkCreateImageView)(device, &view_ci, nullptr, &view) != VK_SUCCESS) {
        view = VK_NULL_HANDLE;
        Release();
    }
}

LsfgImage::~LsfgImage() {
    Release();
}

LsfgImage::LsfgImage(LsfgImage&& other) noexcept
    : device{other.device}, image{other.image}, view{other.view}, memory{other.memory},
      extent{other.extent}, format{other.format}, layout{other.layout} {
    other.device = VK_NULL_HANDLE;
    other.image = VK_NULL_HANDLE;
    other.view = VK_NULL_HANDLE;
    other.memory = VK_NULL_HANDLE;
}

LsfgImage& LsfgImage::operator=(LsfgImage&& other) noexcept {
    if (this != &other) {
        Release();
        device = other.device;
        image = other.image;
        view = other.view;
        memory = other.memory;
        extent = other.extent;
        format = other.format;
        layout = other.layout;
        other.device = VK_NULL_HANDLE;
        other.image = VK_NULL_HANDLE;
        other.view = VK_NULL_HANDLE;
        other.memory = VK_NULL_HANDLE;
    }
    return *this;
}

void LsfgImage::Release() {
    if (device == VK_NULL_HANDLE) return;
    if (view) VK_GET_SYMBOL(vkDestroyImageView)(device, view, nullptr);
    if (image) VK_GET_SYMBOL(vkDestroyImage)(device, image, nullptr);
    if (memory) VK_GET_SYMBOL(vkFreeMemory)(device, memory, nullptr);
    view = VK_NULL_HANDLE;
    image = VK_NULL_HANDLE;
    memory = VK_NULL_HANDLE;
}

LsfgResources::~LsfgResources() {
    if (!device) return;
    for (auto& [key, sampler] : samplers) {
        VK_GET_SYMBOL(vkDestroySampler)(device->Handle(), sampler, nullptr);
    }
    samplers.clear();
}

VkDeviceSize LsfgResources::BufferSize() {
    return sizeof(LsfgConstants);
}

VkSampler LsfgResources::GetSampler(VkSamplerAddressMode address_mode, VkCompareOp compare_op,
                                    bool white_border) {
    const uint64_t key = static_cast<uint64_t>(address_mode) |
                         (static_cast<uint64_t>(compare_op) << 8) |
                         (static_cast<uint64_t>(white_border) << 16);

    const auto it = samplers.find(key);
    if (it != samplers.end()) return it->second;

    VkSampler sampler = CreateLsfgSampler(*device, address_mode, compare_op, white_border);
    samplers.emplace(key, sampler);
    return sampler;
}

VkBuffer LsfgResources::GetBuffer(float timestamp, bool first_iter, bool first_iter_s) {
    uint32_t timestamp_bits{};
    std::memcpy(&timestamp_bits, &timestamp, sizeof(timestamp_bits));
    const uint64_t key = static_cast<uint64_t>(timestamp_bits) |
                         (static_cast<uint64_t>(first_iter) << 32) |
                         (static_cast<uint64_t>(first_iter_s) << 33);

    const auto it = buffers.find(key);
    if (it != buffers.end()) return it->second.Handle();

    Buffer buffer{*device, sizeof(LsfgConstants)};
    if (!buffer.Valid()) return VK_NULL_HANDLE;

    LsfgConstants constants{};
    constants.first_iter = first_iter ? 1u : 0u;
    constants.first_iter_s = first_iter_s ? 1u : 0u;
    constants.resolution_inv_scale = 1.0f / flow_scale;
    constants.timestamp = timestamp;
    constants.ui_threshold = 0.5f;
    buffer.Upload(&constants, sizeof(constants));

    const auto [entry, inserted] = buffers.emplace(key, std::move(buffer));
    return entry->second.Handle();
}

const LsfgImage& LsfgResources::GetDummy(VkFormat format) {
    const uint32_t key = static_cast<uint32_t>(format);

    const auto it = dummies.find(key);
    if (it != dummies.end()) return it->second;

    LsfgImage image{*device, VkExtent2D{1, 1}, format};
    const auto [entry, inserted] = dummies.emplace(key, std::move(image));
    dummies_ready = false;
    return entry->second;
}

void LsfgResources::PrepareDummies(VkCommandBuffer cmdbuf) {
    if (dummies_ready || dummies.empty()) return;

    std::vector<VkImageMemoryBarrier> barriers;
    for (auto& [key, image] : dummies) {
        if (!image.Valid()) continue;

        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = image.Handle();
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.layerCount = 1;
        barriers.push_back(barrier);
        image.SetLayout(VK_IMAGE_LAYOUT_GENERAL);
    }

    if (!barriers.empty()) {
        VK_GET_SYMBOL(vkCmdPipelineBarrier)(cmdbuf, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                             VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 0, nullptr,
                             static_cast<uint32_t>(barriers.size()), barriers.data());
    }
    dummies_ready = true;
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
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.layerCount = 1;
    barriers.push_back(barrier);
    return *this;
}

void LsfgBarriers::Build() {
    if (barriers.empty()) return;
    VK_GET_SYMBOL(vkCmdPipelineBarrier)(cmdbuf, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                         VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 0, nullptr,
                         static_cast<uint32_t>(barriers.size()), barriers.data());
    barriers.clear();
}

LsfgDescriptorWriter& LsfgDescriptorWriter::PushImage(VkDescriptorType type, VkSampler sampler,
                                                      VkImageView view) {
    VkDescriptorImageInfo info{};
    info.sampler = sampler;
    info.imageView = view;
    info.imageLayout =
        view == VK_NULL_HANDLE ? VK_IMAGE_LAYOUT_UNDEFINED : VK_IMAGE_LAYOUT_GENERAL;
    image_infos.push_back(info);

    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = set;
    write.dstBinding = binding++;
    write.descriptorCount = 1;
    write.descriptorType = type;
    write.pImageInfo = &image_infos.back();
    writes.push_back(write);
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
    VkDescriptorBufferInfo info{};
    info.buffer = buffer;
    info.offset = 0;
    info.range = size;
    buffer_infos.push_back(info);

    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = set;
    write.dstBinding = binding++;
    write.descriptorCount = 1;
    write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    write.pBufferInfo = &buffer_infos.back();
    writes.push_back(write);
    return *this;
}

void LsfgDescriptorWriter::Build(const Device& device) {
    if (writes.empty()) return;
    VK_GET_SYMBOL(vkUpdateDescriptorSets)(device.Handle(), static_cast<uint32_t>(writes.size()), writes.data(), 0,
                           nullptr);
    writes.clear();
}

LsfgPass::LsfgPass(const Device& device_, const LsfgShaders& shaders, uint32_t shader_id,
                   LsfgBindings bindings)
    : device{device_.Handle()} {
    std::vector<VkDescriptorSetLayoutBinding> layout_bindings;
    uint32_t index = 0;
    for (const auto& [count, type] : bindings) {
        for (uint32_t i = 0; i < count; i++) {
            VkDescriptorSetLayoutBinding entry{};
            entry.binding = index++;
            entry.descriptorType = type;
            entry.descriptorCount = 1;
            entry.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
            layout_bindings.push_back(entry);
        }
    }
    descriptor_count = index;

    VkDescriptorSetLayoutCreateInfo layout_ci{};
    layout_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layout_ci.bindingCount = static_cast<uint32_t>(layout_bindings.size());
    layout_ci.pBindings = layout_bindings.data();
    if (VK_GET_SYMBOL(vkCreateDescriptorSetLayout)(device, &layout_ci, nullptr, &descriptor_set_layout) !=
        VK_SUCCESS) {
        Release();
        return;
    }

    VkPipelineLayoutCreateInfo pipeline_layout_ci{};
    pipeline_layout_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_ci.setLayoutCount = 1;
    pipeline_layout_ci.pSetLayouts = &descriptor_set_layout;
    if (VK_GET_SYMBOL(vkCreatePipelineLayout)(device, &pipeline_layout_ci, nullptr, &pipeline_layout) !=
        VK_SUCCESS) {
        Release();
        return;
    }

    const VkShaderModule module = shaders.Get(shader_id);
    if (module == VK_NULL_HANDLE) {
        Release();
        return;
    }

    VkPipelineShaderStageCreateInfo stage{};
    stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    stage.module = module;
    stage.pName = "main";

    VkComputePipelineCreateInfo pipeline_ci{};
    pipeline_ci.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipeline_ci.stage = stage;
    pipeline_ci.layout = pipeline_layout;
    if (VK_GET_SYMBOL(vkCreateComputePipelines)(device, VK_NULL_HANDLE, 1, &pipeline_ci, nullptr, &pipeline) !=
        VK_SUCCESS) {
        pipeline = VK_NULL_HANDLE;
        Release();
    }
}

LsfgPass::~LsfgPass() {
    Release();
}

LsfgPass::LsfgPass(LsfgPass&& other) noexcept
    : device{other.device}, descriptor_set_layout{other.descriptor_set_layout},
      pipeline_layout{other.pipeline_layout}, pipeline{other.pipeline},
      descriptor_count{other.descriptor_count} {
    other.device = VK_NULL_HANDLE;
    other.descriptor_set_layout = VK_NULL_HANDLE;
    other.pipeline_layout = VK_NULL_HANDLE;
    other.pipeline = VK_NULL_HANDLE;
}

LsfgPass& LsfgPass::operator=(LsfgPass&& other) noexcept {
    if (this != &other) {
        Release();
        device = other.device;
        descriptor_set_layout = other.descriptor_set_layout;
        pipeline_layout = other.pipeline_layout;
        pipeline = other.pipeline;
        descriptor_count = other.descriptor_count;
        other.device = VK_NULL_HANDLE;
        other.descriptor_set_layout = VK_NULL_HANDLE;
        other.pipeline_layout = VK_NULL_HANDLE;
        other.pipeline = VK_NULL_HANDLE;
    }
    return *this;
}

void LsfgPass::Release() {
    if (device == VK_NULL_HANDLE) return;
    if (pipeline) VK_GET_SYMBOL(vkDestroyPipeline)(device, pipeline, nullptr);
    if (pipeline_layout) VK_GET_SYMBOL(vkDestroyPipelineLayout)(device, pipeline_layout, nullptr);
    if (descriptor_set_layout) VK_GET_SYMBOL(vkDestroyDescriptorSetLayout)(device, descriptor_set_layout, nullptr);
    pipeline = VK_NULL_HANDLE;
    pipeline_layout = VK_NULL_HANDLE;
    descriptor_set_layout = VK_NULL_HANDLE;
}

void LsfgPass::Bind(VkCommandBuffer cmdbuf, VkDescriptorSet set) const {
    BindPipeline(cmdbuf);
    BindSet(cmdbuf, set);
}

void LsfgPass::BindPipeline(VkCommandBuffer cmdbuf) const {
    VK_GET_SYMBOL(vkCmdBindPipeline)(cmdbuf, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
}

void LsfgPass::BindSet(VkCommandBuffer cmdbuf, VkDescriptorSet set) const {
    VK_GET_SYMBOL(vkCmdBindDescriptorSets)(cmdbuf, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline_layout, 0, 1, &set, 0,
                            nullptr);
}

VkDescriptorPool CreateLsfgDescriptorPool(const Device& device, uint32_t max_sets) {
    const VkDescriptorType types[] = {
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        VK_DESCRIPTOR_TYPE_SAMPLER,
        VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
        VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
    };

    std::vector<VkDescriptorPoolSize> sizes;
    for (const VkDescriptorType type : types) {
        VkDescriptorPoolSize size{};
        size.type = type;
        size.descriptorCount = DESCRIPTORS_PER_TYPE;
        sizes.push_back(size);
    }

    VkDescriptorPoolCreateInfo pool_ci{};
    pool_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_ci.maxSets = max_sets;
    pool_ci.poolSizeCount = static_cast<uint32_t>(sizes.size());
    pool_ci.pPoolSizes = sizes.data();

    VkDescriptorPool pool = VK_NULL_HANDLE;
    if (VK_GET_SYMBOL(vkCreateDescriptorPool)(device.Handle(), &pool_ci, nullptr, &pool) != VK_SUCCESS) {
        return VK_NULL_HANDLE;
    }
    return pool;
}

std::vector<VkDescriptorSet> AllocateLsfgDescriptorSets(const Device& device,
                                                        VkDescriptorPool pool,
                                                        VkDescriptorSetLayout layout,
                                                        uint32_t count) {
    if (pool == VK_NULL_HANDLE || layout == VK_NULL_HANDLE || count == 0) return {};

    const std::vector<VkDescriptorSetLayout> layouts(count, layout);
    VkDescriptorSetAllocateInfo allocate_info{};
    allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocate_info.descriptorPool = pool;
    allocate_info.descriptorSetCount = count;
    allocate_info.pSetLayouts = layouts.data();

    std::vector<VkDescriptorSet> sets(count, VK_NULL_HANDLE);
    if (VK_GET_SYMBOL(vkAllocateDescriptorSets)(device.Handle(), &allocate_info, sets.data()) != VK_SUCCESS) {
        return {};
    }
    return sets;
}

std::vector<VkDescriptorSet> AllocateLsfgDescriptorSets(
    const Device& device, VkDescriptorPool pool,
    const std::vector<VkDescriptorSetLayout>& layouts) {
    if (pool == VK_NULL_HANDLE || layouts.empty()) return {};
    for (const VkDescriptorSetLayout layout : layouts) {
        if (layout == VK_NULL_HANDLE) return {};
    }

    VkDescriptorSetAllocateInfo allocate_info{};
    allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocate_info.descriptorPool = pool;
    allocate_info.descriptorSetCount = static_cast<uint32_t>(layouts.size());
    allocate_info.pSetLayouts = layouts.data();

    std::vector<VkDescriptorSet> sets(layouts.size(), VK_NULL_HANDLE);
    if (VK_GET_SYMBOL(vkAllocateDescriptorSets)(device.Handle(), &allocate_info, sets.data()) != VK_SUCCESS) {
        return {};
    }
    return sets;
}

VkSampler CreateLsfgSampler(const Device& device, VkSamplerAddressMode address_mode,
                            VkCompareOp compare_op, bool white_border) {
    VkSamplerCreateInfo sampler_ci{};
    sampler_ci.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler_ci.magFilter = VK_FILTER_LINEAR;
    sampler_ci.minFilter = VK_FILTER_LINEAR;
    sampler_ci.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    sampler_ci.addressModeU = address_mode;
    sampler_ci.addressModeV = address_mode;
    sampler_ci.addressModeW = address_mode;
    sampler_ci.anisotropyEnable = VK_FALSE;
    sampler_ci.compareEnable = VK_FALSE;
    sampler_ci.compareOp = compare_op;
    sampler_ci.maxLod = VK_LOD_CLAMP_NONE;
    sampler_ci.borderColor = white_border ? VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE
                                          : VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
    sampler_ci.unnormalizedCoordinates = VK_FALSE;

    VkSampler sampler = VK_NULL_HANDLE;
    if (VK_GET_SYMBOL(vkCreateSampler)(device.Handle(), &sampler_ci, nullptr, &sampler) != VK_SUCCESS) {
        return VK_NULL_HANDLE;
    }
    return sampler;
}

}
