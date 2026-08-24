// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

// SPDX-FileCopyrightText: Copyright 2025 lsfg-vk
// SPDX-License-Identifier: GPL-3.0-or-later

#include "lsfg_generate.hpp"
#include "lsfg_dll.h"
#include "lsfg_shaders.hpp"

#include <vector>

namespace lsfg {

namespace {

constexpr uint32_t DISPATCH_TILE_SHIFT = 4;

[[nodiscard]] uint32_t GroupCount(uint32_t size) {
    return (size + (1u << DISPATCH_TILE_SHIFT) - 1) >> DISPATCH_TILE_SHIFT;
}

VkImageMemoryBarrier MakeTargetBarrier(VkImage image, VkAccessFlags src_access,
                                       VkAccessFlags dst_access, VkImageLayout old_layout) {
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.srcAccessMask = src_access;
    barrier.dstAccessMask = dst_access;
    barrier.oldLayout = old_layout;
    barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.layerCount = 1;
    return barrier;
}

}

LsfgGenerate::LsfgGenerate(const Device& device, const LsfgShaders& shaders,
                           LsfgResources& resources, VkDescriptorPool descriptor_pool,
                           LsfgImagePair& frames_, LsfgImage& motion_, LsfgImage& detail1_,
                           LsfgImage& detail2_)
    : frames{&frames_}, motion{&motion_}, detail1{&detail1_}, detail2{&detail2_} {
    pass = LsfgPass(device, shaders, LSFG_SHADER_GENERATE,
                    {{1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER},
                     {2, VK_DESCRIPTOR_TYPE_SAMPLER},
                     {5, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE},
                     {1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE}});
    if (!pass.Valid()) return;

    sampler = resources.GetSampler();
    edge_sampler =
        resources.GetSampler(VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, VK_COMPARE_OP_ALWAYS, false);

    const uint32_t total =
        static_cast<uint32_t>(LSFG_GENERATION_SLOTS * LSFG_MAX_TARGETS * 2);
    const std::vector<VkDescriptorSet> sets =
        AllocateLsfgDescriptorSets(device, descriptor_pool, pass.SetLayout(), total);
    if (sets.size() != total) return;

    size_t next = 0;
    for (size_t slot = 0; slot < LSFG_GENERATION_SLOTS; ++slot) {
        Generation& target = generations[slot];
        target.buffer = resources.GetBuffer(LsfgSlotTimestamp(slot));

        for (auto& entry : target.targets) {
            for (auto& set : entry.descriptor_sets) {
                set = sets[next++];
            }
        }
    }
    allocated = true;
}

void LsfgGenerate::SetTarget(const Device& device, size_t slot, uint32_t target,
                             VkImageView view) {
    Target& entry = generations[slot].targets[target];
    if (entry.view == view) return;
    entry.view = view;

    for (size_t i = 0; i < entry.descriptor_sets.size(); ++i) {
        LsfgDescriptorWriter(entry.descriptor_sets[i])
            .AddUniformBuffer(generations[slot].buffer, LsfgResources::BufferSize())
            .AddSampler(sampler)
            .AddSampler(edge_sampler)
            .AddSampledImage((*frames)[1 - i])
            .AddSampledImage((*frames)[i])
            .AddSampledImage(*motion)
            .AddSampledImage(*detail1)
            .AddSampledImage(*detail2)
            .AddStorageView(view)
            .Build(device);
    }
}

void LsfgGenerate::ForgetTargets() {
    for (auto& generation : generations) {
        for (auto& entry : generation.targets) {
            entry.view = VK_NULL_HANDLE;
        }
    }
}

void LsfgGenerate::Dispatch(VkCommandBuffer cmdbuf, uint64_t frame_count, size_t slot,
                            uint32_t target, VkImage image, VkExtent2D extent) {
    const Target& entry = generations[slot].targets[target];

    LsfgBarriers(cmdbuf)
        .WriteToReadAll(*frames)
        .WriteToRead(*motion)
        .WriteToRead(*detail1)
        .WriteToRead(*detail2)
        .DiscardToWrite(image)
        .Build();

    pass.Bind(cmdbuf, entry.descriptor_sets[frame_count % entry.descriptor_sets.size()]);
    VK_GET_SYMBOL(vkCmdDispatch)(cmdbuf, GroupCount(extent.width), GroupCount(extent.height), 1);

    const VkImageMemoryBarrier after = MakeTargetBarrier(
        image, VK_ACCESS_SHADER_WRITE_BIT,
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_TRANSFER_READ_BIT,
        VK_IMAGE_LAYOUT_GENERAL);
    VK_GET_SYMBOL(vkCmdPipelineBarrier)(cmdbuf, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                         VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                             VK_PIPELINE_STAGE_TRANSFER_BIT,
                         0, 0, nullptr, 0, nullptr, 1, &after);
}

}
