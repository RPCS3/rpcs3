// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

// SPDX-FileCopyrightText: Copyright 2025 lsfg-vk
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <array>
#include <cstdint>
#include <deque>
#include <initializer_list>
#include <map>
#include <utility>
#include <vector>

#include "../VulkanAPI.h"

namespace lsfg {

constexpr VkFormat LSFG_DEFAULT_FORMAT = VK_FORMAT_R8G8B8A8_UNORM;
constexpr VkFormat LSFG_FLOW_FORMAT = VK_FORMAT_R8_UNORM;
constexpr VkFormat LSFG_MOTION_FORMAT = VK_FORMAT_R16G16B16A16_SFLOAT;

constexpr size_t LSFG_HISTORY_SLOTS = 3;
constexpr size_t LSFG_MAX_TARGETS = 7;
constexpr size_t LSFG_MAX_GENERATIONS = 3;
constexpr size_t LSFG_MIP_LEVELS = 7;

constexpr size_t LSFG_GENERATION_SLOTS = LSFG_MAX_GENERATIONS * (LSFG_MAX_GENERATIONS + 1) / 2;

constexpr std::array<uint32_t, 4> LSFG_ALPHA_SHADERS{290, 291, 292, 293};
constexpr std::array<uint32_t, 5> LSFG_BETA_SHADERS{298, 299, 300, 301, 302};
constexpr std::array<uint32_t, 5> LSFG_GAMMA_SHADERS{280, 282, 283, 284, 285};
constexpr std::array<uint32_t, 10> LSFG_DELTA_SHADERS{280, 286, 287, 288, 289,
                                                     281, 294, 295, 296, 297};

[[nodiscard]] constexpr size_t LsfgGenerationSlot(size_t generation_count, size_t generation) {
    return (generation_count - 1) * generation_count / 2 + generation;
}

[[nodiscard]] constexpr float LsfgTimestamp(size_t generation, size_t generation_count) {
    return static_cast<float>(generation + 1) /
           static_cast<float>(generation_count + 1);
}

[[nodiscard]] constexpr size_t LsfgSlotCount(size_t slot) {
    size_t count = 1;
    while (LsfgGenerationSlot(count + 1, 0) <= slot) {
        ++count;
    }
    return count;
}

[[nodiscard]] constexpr float LsfgSlotTimestamp(size_t slot) {
    const size_t count = LsfgSlotCount(slot);
    return LsfgTimestamp(slot - LsfgGenerationSlot(count, 0), count);
}

class Device {
public:
    Device() = default;
    Device(VkDevice device_, VkPhysicalDevice physical_device_);

    [[nodiscard]] VkDevice Handle() const {
        return device;
    }

    [[nodiscard]] uint32_t FindMemoryType(uint32_t bits, VkMemoryPropertyFlags properties) const;

private:
    VkDevice device{VK_NULL_HANDLE};
    VkPhysicalDevice physical_device{VK_NULL_HANDLE};
    VkPhysicalDeviceMemoryProperties memory_properties{};
};

class Buffer {
public:
    Buffer() = default;
    Buffer(const Device& device, VkDeviceSize size);
    ~Buffer();

    Buffer(const Buffer&) = delete;
    Buffer& operator=(const Buffer&) = delete;
    Buffer(Buffer&& other) noexcept;
    Buffer& operator=(Buffer&& other) noexcept;

    [[nodiscard]] VkBuffer Handle() const {
        return buffer;
    }

    [[nodiscard]] bool Valid() const {
        return buffer != VK_NULL_HANDLE;
    }

    void Upload(const void* data, size_t size);

private:
    void Release();

    VkDevice device{VK_NULL_HANDLE};
    VkBuffer buffer{VK_NULL_HANDLE};
    VkDeviceMemory memory{VK_NULL_HANDLE};
    void* mapped{nullptr};
};

class LsfgImage {
public:
    LsfgImage() = default;
    LsfgImage(const Device& device, VkExtent2D extent, VkFormat format = LSFG_DEFAULT_FORMAT);
    ~LsfgImage();

    LsfgImage(const LsfgImage&) = delete;
    LsfgImage& operator=(const LsfgImage&) = delete;
    LsfgImage(LsfgImage&& other) noexcept;
    LsfgImage& operator=(LsfgImage&& other) noexcept;

    [[nodiscard]] VkImage Handle() const {
        return image;
    }

    [[nodiscard]] VkImageView View() const {
        return view;
    }

    [[nodiscard]] VkExtent2D Extent() const {
        return extent;
    }

    [[nodiscard]] VkFormat Format() const {
        return format;
    }

    [[nodiscard]] VkImageLayout Layout() const {
        return layout;
    }

    [[nodiscard]] bool Valid() const {
        return image != VK_NULL_HANDLE;
    }

    void SetLayout(VkImageLayout new_layout) {
        layout = new_layout;
    }

private:
    void Release();

    VkDevice device{VK_NULL_HANDLE};
    VkImage image{VK_NULL_HANDLE};
    VkImageView view{VK_NULL_HANDLE};
    VkDeviceMemory memory{VK_NULL_HANDLE};
    VkExtent2D extent{};
    VkFormat format{VK_FORMAT_UNDEFINED};
    VkImageLayout layout{VK_IMAGE_LAYOUT_UNDEFINED};
};

using LsfgImagePair = std::array<LsfgImage, 2>;
using LsfgImageHistory = std::array<LsfgImagePair, LSFG_HISTORY_SLOTS>;

class LsfgResources {
public:
    LsfgResources() = default;
    LsfgResources(const Device& device_, float flow_scale_)
        : device{&device_}, flow_scale{flow_scale_} {}

    ~LsfgResources();

    LsfgResources(const LsfgResources&) = delete;
    LsfgResources& operator=(const LsfgResources&) = delete;

    [[nodiscard]] VkSampler GetSampler(
        VkSamplerAddressMode address_mode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
        VkCompareOp compare_op = VK_COMPARE_OP_NEVER, bool white_border = false);

    [[nodiscard]] VkBuffer GetBuffer(float timestamp = 0.0f, bool first_iter = false,
                                     bool first_iter_s = false);

    [[nodiscard]] const LsfgImage& GetDummy(VkFormat format = LSFG_MOTION_FORMAT);

    void PrepareDummies(VkCommandBuffer cmdbuf);

    [[nodiscard]] static VkDeviceSize BufferSize();

private:
    const Device* device{};
    float flow_scale{1.0f};

    std::map<uint64_t, VkSampler> samplers;
    std::map<uint64_t, Buffer> buffers;
    std::map<uint32_t, LsfgImage> dummies;
    bool dummies_ready{};
};

class LsfgBarriers {
public:
    explicit LsfgBarriers(VkCommandBuffer cmdbuf_) : cmdbuf{cmdbuf_} {}

    LsfgBarriers& WriteToRead(LsfgImage& image);
    LsfgBarriers& ReadToWrite(LsfgImage& image);
    LsfgBarriers& WriteToRead(LsfgImage* image);
    LsfgBarriers& ReadToWrite(LsfgImage* image);
    LsfgBarriers& DiscardToWrite(VkImage image);

    template <typename Range>
    LsfgBarriers& WriteToReadAll(Range& images) {
        for (auto& image : images) {
            WriteToRead(image);
        }
        return *this;
    }

    template <typename Range>
    LsfgBarriers& ReadToWriteAll(Range& images) {
        for (auto& image : images) {
            ReadToWrite(image);
        }
        return *this;
    }

    void Build();

private:
    LsfgBarriers& Push(LsfgImage& image, VkAccessFlags src_access, VkAccessFlags dst_access);

    VkCommandBuffer cmdbuf;
    std::vector<VkImageMemoryBarrier> barriers;
};

class LsfgDescriptorWriter {
public:
    explicit LsfgDescriptorWriter(VkDescriptorSet set_) : set{set_} {}

    LsfgDescriptorWriter& AddSampler(VkSampler sampler);
    LsfgDescriptorWriter& AddSampledImage(const LsfgImage& image);
    LsfgDescriptorWriter& AddSampledImage(const LsfgImage* image);
    LsfgDescriptorWriter& AddStorageImage(const LsfgImage& image);
    LsfgDescriptorWriter& AddStorageView(VkImageView view);
    LsfgDescriptorWriter& AddUniformBuffer(VkBuffer buffer, VkDeviceSize size);

    template <typename Range>
    LsfgDescriptorWriter& AddSampledImages(const Range& images) {
        for (const auto& image : images) {
            AddSampledImage(image);
        }
        return *this;
    }

    template <typename Range>
    LsfgDescriptorWriter& AddStorageImages(const Range& images) {
        for (const auto& image : images) {
            AddStorageImage(image);
        }
        return *this;
    }

    void Build(const Device& device);

private:
    LsfgDescriptorWriter& PushImage(VkDescriptorType type, VkSampler sampler, VkImageView view);

    VkDescriptorSet set;
    uint32_t binding{};
    std::deque<VkDescriptorImageInfo> image_infos;
    std::deque<VkDescriptorBufferInfo> buffer_infos;
    std::vector<VkWriteDescriptorSet> writes;
};

using LsfgBindings = std::initializer_list<std::pair<uint32_t, VkDescriptorType>>;

class LsfgShaders;

class LsfgPass {
public:
    LsfgPass() = default;
    LsfgPass(const Device& device, const LsfgShaders& shaders, uint32_t shader_id,
             LsfgBindings bindings);
    ~LsfgPass();

    LsfgPass(const LsfgPass&) = delete;
    LsfgPass& operator=(const LsfgPass&) = delete;
    LsfgPass(LsfgPass&& other) noexcept;
    LsfgPass& operator=(LsfgPass&& other) noexcept;

    [[nodiscard]] VkDescriptorSetLayout SetLayout() const {
        return descriptor_set_layout;
    }

    [[nodiscard]] uint32_t DescriptorCount() const {
        return descriptor_count;
    }

    [[nodiscard]] bool Valid() const {
        return pipeline != VK_NULL_HANDLE;
    }

    void Bind(VkCommandBuffer cmdbuf, VkDescriptorSet set) const;
    void BindPipeline(VkCommandBuffer cmdbuf) const;
    void BindSet(VkCommandBuffer cmdbuf, VkDescriptorSet set) const;

private:
    void Release();

    VkDevice device{VK_NULL_HANDLE};
    VkDescriptorSetLayout descriptor_set_layout{VK_NULL_HANDLE};
    VkPipelineLayout pipeline_layout{VK_NULL_HANDLE};
    VkPipeline pipeline{VK_NULL_HANDLE};
    uint32_t descriptor_count{};
};

[[nodiscard]] VkDescriptorPool CreateLsfgDescriptorPool(const Device& device, uint32_t max_sets);

[[nodiscard]] std::vector<VkDescriptorSet> AllocateLsfgDescriptorSets(
    const Device& device, VkDescriptorPool pool, VkDescriptorSetLayout layout, uint32_t count);

[[nodiscard]] std::vector<VkDescriptorSet> AllocateLsfgDescriptorSets(
    const Device& device, VkDescriptorPool pool,
    const std::vector<VkDescriptorSetLayout>& layouts);

[[nodiscard]] VkSampler CreateLsfgSampler(const Device& device, VkSamplerAddressMode address_mode,
                                          VkCompareOp compare_op, bool white_border);

}
