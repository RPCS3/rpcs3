// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

// SPDX-FileCopyrightText: Copyright 2025 lsfg-vk
// SPDX-License-Identifier: GPL-3.0-or-later

// Adapted for ReX/PS5 in 2026; origin Eden 5f142c7926d0c7fcbbd0ce30794d72f638a43b2a.

#include <vector>

#include "lossless_dll.h"
#include "lsfg_gamma.h"
#include "lsfg_shaders.h"
#include "util.h"
#include "vulkan_adapter.h"

namespace rex::ui::vulkan::framegen {

namespace {

constexpr u32 DISPATCH_TILE_SHIFT = 3;

[[nodiscard]] u32 GroupCount(u32 size) {
    return (size + (1u << DISPATCH_TILE_SHIFT) - 1) >> DISPATCH_TILE_SHIFT;
}

} // Anonymous namespace

LsfgGamma::LsfgGamma(const Device& device, MemoryAllocator& memory_allocator,
                     const LsfgShaders& shaders, LsfgResources& resources,
                     vk::DescriptorPool& descriptor_pool, LsfgImageHistory& inputs_,
                     LsfgImage& flow_input_, LsfgImage* previous_)
    : inputs{&inputs_}, flow_input{&flow_input_}, previous{previous_} {
    using namespace PerformanceShader;

    passes[0] = LsfgPass(device, shaders, GAMMA[0],
                         {{1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER},
                          {2, VK_DESCRIPTOR_TYPE_SAMPLER},
                          {5, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE},
                          {3, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE}});
    passes[1] = LsfgPass(device, shaders, GAMMA[1],
                         {{1, VK_DESCRIPTOR_TYPE_SAMPLER},
                          {3, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE},
                          {2, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE}});
    passes[2] = LsfgPass(device, shaders, GAMMA[2],
                         {{1, VK_DESCRIPTOR_TYPE_SAMPLER},
                          {2, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE},
                          {2, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE}});
    passes[3] = LsfgPass(device, shaders, GAMMA[3],
                         {{1, VK_DESCRIPTOR_TYPE_SAMPLER},
                          {2, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE},
                          {2, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE}});
    passes[4] = LsfgPass(device, shaders, GAMMA[4],
                         {{1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER},
                          {2, VK_DESCRIPTOR_TYPE_SAMPLER},
                          {4, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE},
                          {1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE}});

    const VkExtent2D extent = (*inputs)[0][0].Extent();
    for (auto& image : temp1) {
        image = LsfgImage(device, memory_allocator, extent);
    }
    for (auto& image : temp2) {
        image = LsfgImage(device, memory_allocator, extent);
    }
    out_image = LsfgImage(device, memory_allocator, extent, LSFG_MOTION_FORMAT);

    std::vector<VkDescriptorSetLayout> layouts;
    for (size_t slot = 0; slot < LSFG_GENERATION_SLOTS; ++slot) {
        for (size_t i = 0; i < LSFG_HISTORY_SLOTS; ++i) {
            layouts.push_back(passes[0].SetLayout());
        }
        for (size_t i = 1; i < LSFG_GAMMA_STAGES; ++i) {
            layouts.push_back(passes[i].SetLayout());
        }
    }
    owned_sets = CreateWrappedDescriptorSets(descriptor_pool, layouts);

    const VkSampler sampler = resources.GetSampler();
    const VkSampler border_sampler = resources.GetSampler(
        VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER, VK_COMPARE_OP_NEVER, true);
    const VkSampler edge_sampler =
        resources.GetSampler(VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, VK_COMPARE_OP_ALWAYS, false);

    size_t next = 0;
    for (size_t slot = 0; slot < LSFG_GENERATION_SLOTS; ++slot) {
        Generation& pass = generations[slot];
        const VkBuffer buffer =
            resources.GetBuffer(LsfgSlotTimestamp(slot), previous == nullptr);

        for (size_t i = 0; i < LSFG_HISTORY_SLOTS; ++i) {
            pass.first_descriptor_sets[i] = owned_sets[next++];
        }
        for (size_t i = 0; i < LSFG_GAMMA_STAGES - 1; ++i) {
            pass.descriptor_sets[i] = owned_sets[next++];
        }

        for (size_t i = 0; i < LSFG_HISTORY_SLOTS; ++i) {
            LsfgDescriptorWriter(pass.first_descriptor_sets[i])
                .AddUniformBuffer(buffer, LsfgResources::BufferSize())
                .AddSampler(border_sampler)
                .AddSampler(edge_sampler)
                .AddSampledImages((*inputs)[(i + 2) % LSFG_HISTORY_SLOTS])
                .AddSampledImages((*inputs)[i % LSFG_HISTORY_SLOTS])
                .AddSampledImage(previous)
                .AddStorageImages(temp1)
                .Build(device);
        }
        LsfgDescriptorWriter(pass.descriptor_sets[0])
            .AddSampler(sampler)
            .AddSampledImages(temp1)
            .AddStorageImages(temp2)
            .Build(device);
        LsfgDescriptorWriter(pass.descriptor_sets[1])
            .AddSampler(sampler)
            .AddSampledImages(temp2)
            .AddStorageImage(temp1[0])
            .AddStorageImage(temp1[1])
            .Build(device);
        LsfgDescriptorWriter(pass.descriptor_sets[2])
            .AddSampler(sampler)
            .AddSampledImage(temp1[0])
            .AddSampledImage(temp1[1])
            .AddStorageImages(temp2)
            .Build(device);
        LsfgDescriptorWriter(pass.descriptor_sets[3])
            .AddUniformBuffer(buffer, LsfgResources::BufferSize())
            .AddSampler(sampler)
            .AddSampler(edge_sampler)
            .AddSampledImages(temp2)
            .AddSampledImage(previous)
            .AddSampledImage(*flow_input)
            .AddStorageImage(out_image)
            .Build(device);
    }
}

void LsfgGamma::Dispatch(vk::CommandBuffer cmdbuf, u64 frame_count, size_t slot) {
    const Generation& pass = generations[slot];

    const VkExtent2D extent = temp1[0].Extent();
    const u32 groups_x = GroupCount(extent.width);
    const u32 groups_y = GroupCount(extent.height);

    const size_t history = frame_count % LSFG_HISTORY_SLOTS;
    const size_t previous_history = (frame_count + 2) % LSFG_HISTORY_SLOTS;

    LsfgBarriers(cmdbuf)
        .WriteToReadAll((*inputs)[previous_history])
        .WriteToReadAll((*inputs)[history])
        .WriteToRead(previous)
        .ReadToWriteAll(temp1)
        .Build();
    passes[0].Bind(cmdbuf, pass.first_descriptor_sets[history]);
    cmdbuf.Dispatch(groups_x, groups_y, 1);

    LsfgBarriers(cmdbuf).WriteToReadAll(temp1).ReadToWriteAll(temp2).Build();
    passes[1].Bind(cmdbuf, pass.descriptor_sets[0]);
    cmdbuf.Dispatch(groups_x, groups_y, 1);

    LsfgBarriers(cmdbuf)
        .WriteToReadAll(temp2)
        .ReadToWrite(temp1[0])
        .ReadToWrite(temp1[1])
        .Build();
    passes[2].Bind(cmdbuf, pass.descriptor_sets[1]);
    cmdbuf.Dispatch(groups_x, groups_y, 1);

    LsfgBarriers(cmdbuf)
        .WriteToRead(temp1[0])
        .WriteToRead(temp1[1])
        .ReadToWriteAll(temp2)
        .Build();
    passes[3].Bind(cmdbuf, pass.descriptor_sets[2]);
    cmdbuf.Dispatch(groups_x, groups_y, 1);

    LsfgBarriers(cmdbuf)
        .WriteToReadAll(temp2)
        .WriteToRead(previous)
        .WriteToRead(*flow_input)
        .ReadToWrite(out_image)
        .Build();
    passes[4].Bind(cmdbuf, pass.descriptor_sets[3]);
    cmdbuf.Dispatch(groups_x, groups_y, 1);
}

} // namespace rex::ui::vulkan::framegen
