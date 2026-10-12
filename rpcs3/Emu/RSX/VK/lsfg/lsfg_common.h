// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

// SPDX-FileCopyrightText: Copyright 2025 lsfg-vk
// SPDX-License-Identifier: GPL-3.0-or-later

// Adapted for ReX/PS5 in 2026; origin Eden 5f142c7926d0c7fcbbd0ce30794d72f638a43b2a.

#pragma once

#include <array>
#include <deque>
#include <initializer_list>
#include <map>
#include <utility>
#include <vector>

#include "types.h"
#include "vulkan_adapter.h"
#include "vulkan_adapter.h"

namespace rex::ui::vulkan::framegen {

class Device;
class LsfgShaders;

constexpr VkFormat LSFG_DEFAULT_FORMAT = VK_FORMAT_R8G8B8A8_UNORM;
constexpr VkFormat LSFG_FLOW_FORMAT = VK_FORMAT_R8_UNORM;
constexpr VkFormat LSFG_MOTION_FORMAT = VK_FORMAT_R16G16B16A16_SFLOAT;

constexpr size_t LSFG_HISTORY_SLOTS = 3;
constexpr size_t LSFG_MAX_TARGETS = 7;
constexpr size_t LSFG_MAX_GENERATIONS = 3;

constexpr size_t LSFG_GENERATION_SLOTS = LSFG_MAX_GENERATIONS * (LSFG_MAX_GENERATIONS + 1) / 2;

[[nodiscard]] constexpr size_t LsfgGenerationSlot(size_t generation_count, size_t generation) {
    return (generation_count - 1) * generation_count / 2 + generation;
}

[[nodiscard]] constexpr f32 LsfgTimestamp(size_t generation, size_t generation_count) {
    return static_cast<f32>(generation + 1) / static_cast<f32>(generation_count + 1);
}

[[nodiscard]] constexpr size_t LsfgSlotCount(size_t slot) {
    size_t count = 1;
    while (LsfgGenerationSlot(count + 1, 0) <= slot) {
        ++count;
    }
    return count;
}

[[nodiscard]] constexpr f32 LsfgSlotTimestamp(size_t slot) {
    const size_t count = LsfgSlotCount(slot);
    return LsfgTimestamp(slot - LsfgGenerationSlot(count, 0), count);
}

class LsfgImage {
public:
    LsfgImage() = default;
    LsfgImage(const Device& device, MemoryAllocator& memory_allocator, VkExtent2D extent_,
              VkFormat format = LSFG_DEFAULT_FORMAT);
    LsfgImage(const LsfgImage&) = delete;
    LsfgImage& operator=(const LsfgImage&) = delete;
    LsfgImage(LsfgImage&&) = default;
    LsfgImage& operator=(LsfgImage&& other) noexcept {
        LsfgImage temporary(std::move(other));
        std::swap(image, temporary.image);
        view.Swap(temporary.view);
        std::swap(extent, temporary.extent);
        std::swap(format, temporary.format);
        std::swap(layout, temporary.layout);
        return *this;
    }

    [[nodiscard]] VkImage Handle() const {
        return *image;
    }

    [[nodiscard]] VkImageView View() const {
        return *view;
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

    void SetLayout(VkImageLayout new_layout) {
        layout = new_layout;
    }

private:
    vk::Image image;
    vk::ImageView view;
    VkExtent2D extent{};
    VkFormat format{VK_FORMAT_UNDEFINED};
    // The adapter clears every allocated image once before any LSFG dispatch.
    // That recording ends in GENERAL, including all temporal history slots.
    VkImageLayout layout{VK_IMAGE_LAYOUT_GENERAL};
};

using LsfgImagePair = std::array<LsfgImage, 2>;
using LsfgImageHistory = std::array<LsfgImagePair, LSFG_HISTORY_SLOTS>;

class LsfgResources {
public:
    LsfgResources() = default;
    LsfgResources(const Device& device_, MemoryAllocator& memory_allocator_, f32 flow_scale_)
        : device{&device_}, memory_allocator{&memory_allocator_}, flow_scale{flow_scale_} {}

    [[nodiscard]] VkSampler GetSampler(
        VkSamplerAddressMode address_mode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
        VkCompareOp compare_op = VK_COMPARE_OP_NEVER, bool white_border = false);

    [[nodiscard]] VkBuffer GetBuffer(f32 timestamp = 0.0f, bool first_iter = false,
                                     bool first_iter_s = false);

    [[nodiscard]] static VkDeviceSize BufferSize();

private:
    const Device* device{};
    MemoryAllocator* memory_allocator{};
    f32 flow_scale{1.0f};

    std::map<u64, vk::Sampler> samplers;
    std::map<u64, vk::Buffer> buffers;
};

class LsfgBarriers {
public:
    explicit LsfgBarriers(vk::CommandBuffer cmdbuf_) : cmdbuf{cmdbuf_} {}

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

    vk::CommandBuffer cmdbuf;
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
    u32 binding{};
    std::deque<VkDescriptorImageInfo> image_infos;
    std::deque<VkDescriptorBufferInfo> buffer_infos;
    std::vector<VkWriteDescriptorSet> writes;
};

using LsfgBindings = std::initializer_list<std::pair<u32, VkDescriptorType>>;

class LsfgPass {
public:
    LsfgPass() = default;
    LsfgPass(const Device& device, const LsfgShaders& shaders, u32 shader_id,
             LsfgBindings bindings);

    [[nodiscard]] VkDescriptorSetLayout SetLayout() const {
        return *descriptor_set_layout;
    }

    [[nodiscard]] u32 DescriptorCount() const {
        return descriptor_count;
    }

    void Bind(vk::CommandBuffer cmdbuf, VkDescriptorSet set) const;
    void BindPipeline(vk::CommandBuffer cmdbuf) const;
    void BindSet(vk::CommandBuffer cmdbuf, VkDescriptorSet set) const;

private:
    vk::DescriptorSetLayout descriptor_set_layout;
    vk::PipelineLayout pipeline_layout;
    vk::Pipeline pipeline;
    u32 descriptor_count{};
};

[[nodiscard]] vk::DescriptorPool CreateLsfgDescriptorPool(const Device& device, u32 max_sets);

[[nodiscard]] vk::Sampler CreateLsfgSampler(const Device& device, VkSamplerAddressMode address_mode,
                                            VkCompareOp compare_op, bool white_border);

} // namespace rex::ui::vulkan::framegen
