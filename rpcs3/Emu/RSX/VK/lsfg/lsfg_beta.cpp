// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

// SPDX-FileCopyrightText: Copyright 2025 lsfg-vk
// SPDX-License-Identifier: GPL-3.0-or-later

// Adapted for ReX/PS5 in 2026; origin Eden 5f142c7926d0c7fcbbd0ce30794d72f638a43b2a.

#include <vector>

#include "lossless_dll.h"
#include "lsfg_beta.h"
#include "lsfg_shaders.h"
#include "util.h"
#include "vulkan_adapter.h"

namespace rex::ui::vulkan::framegen {

namespace {

constexpr u32 DISPATCH_TILE_SHIFT = 3;
constexpr u32 OUTPUT_TILE_SHIFT = 5;

[[nodiscard]] u32 GroupCount(u32 size, u32 shift) {
    return (size + (1u << shift) - 1) >> shift;
}

} // Anonymous namespace

LsfgBeta::LsfgBeta(const Device& device, MemoryAllocator& memory_allocator,
                   const LsfgShaders& shaders, LsfgResources& resources,
                   vk::DescriptorPool& descriptor_pool, LsfgImageHistory& inputs_)
    : inputs{&inputs_} {
    using namespace PerformanceShader;

    passes[0] = LsfgPass(device, shaders, BETA[0],
                         {{1, VK_DESCRIPTOR_TYPE_SAMPLER},
                          {6, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE},
                          {2, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE}});
    for (size_t i = 1; i < LSFG_BETA_STAGES - 1; ++i) {
        passes[i] = LsfgPass(device, shaders, BETA[i],
                             {{1, VK_DESCRIPTOR_TYPE_SAMPLER},
                              {2, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE},
                              {2, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE}});
    }
    passes[4] = LsfgPass(device, shaders, BETA[4],
                         {{1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER},
                          {1, VK_DESCRIPTOR_TYPE_SAMPLER},
                          {2, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE},
                          {6, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE}});

    const VkExtent2D extent = (*inputs)[0][0].Extent();
    for (size_t i = 0; i < temp1.size(); ++i) {
        temp1[i] = LsfgImage(device, memory_allocator, extent);
        temp2[i] = LsfgImage(device, memory_allocator, extent);
    }
    for (size_t i = 0; i < LSFG_BETA_OUTPUTS; ++i) {
        const VkExtent2D level_extent{
            .width = extent.width >> i,
            .height = extent.height >> i,
        };
        out_images[i] = LsfgImage(device, memory_allocator, level_extent, LSFG_FLOW_FORMAT);
    }

    std::vector<VkDescriptorSetLayout> layouts;
    for (size_t i = 0; i < LSFG_HISTORY_SLOTS; ++i) {
        layouts.push_back(passes[0].SetLayout());
    }
    for (size_t i = 1; i < LSFG_BETA_STAGES; ++i) {
        layouts.push_back(passes[i].SetLayout());
    }
    owned_sets = CreateWrappedDescriptorSets(descriptor_pool, layouts);

    for (size_t i = 0; i < LSFG_HISTORY_SLOTS; ++i) {
        first_descriptor_sets[i] = owned_sets[i];
    }
    for (size_t i = 0; i < LSFG_BETA_STAGES - 1; ++i) {
        descriptor_sets[i] = owned_sets[LSFG_HISTORY_SLOTS + i];
    }

    const VkSampler sampler = resources.GetSampler();
    const VkSampler border_sampler = resources.GetSampler(
        VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER, VK_COMPARE_OP_NEVER, true);

    for (size_t i = 0; i < LSFG_HISTORY_SLOTS; ++i) {
        LsfgDescriptorWriter(first_descriptor_sets[i])
            .AddSampler(border_sampler)
            .AddSampledImages((*inputs)[(i + 1) % LSFG_HISTORY_SLOTS])
            .AddSampledImages((*inputs)[(i + 2) % LSFG_HISTORY_SLOTS])
            .AddSampledImages((*inputs)[i % LSFG_HISTORY_SLOTS])
            .AddStorageImages(temp1)
            .Build(device);
    }
    LsfgDescriptorWriter(descriptor_sets[0])
        .AddSampler(sampler)
        .AddSampledImages(temp1)
        .AddStorageImages(temp2)
        .Build(device);
    LsfgDescriptorWriter(descriptor_sets[1])
        .AddSampler(sampler)
        .AddSampledImages(temp2)
        .AddStorageImages(temp1)
        .Build(device);
    LsfgDescriptorWriter(descriptor_sets[2])
        .AddSampler(sampler)
        .AddSampledImages(temp1)
        .AddStorageImages(temp2)
        .Build(device);
    LsfgDescriptorWriter(descriptor_sets[3])
        .AddUniformBuffer(resources.GetBuffer(0.5f), LsfgResources::BufferSize())
        .AddSampler(sampler)
        .AddSampledImages(temp2)
        .AddStorageImages(out_images)
        .Build(device);
}

void LsfgBeta::Dispatch(vk::CommandBuffer cmdbuf, u64 frame_count) {
    const VkExtent2D extent = temp1[0].Extent();
    const u32 groups_x = GroupCount(extent.width, DISPATCH_TILE_SHIFT);
    const u32 groups_y = GroupCount(extent.height, DISPATCH_TILE_SHIFT);

    LsfgBarriers barriers(cmdbuf);
    for (auto& slot : *inputs) {
        barriers.WriteToReadAll(slot);
    }
    barriers.ReadToWriteAll(temp1).Build();

    passes[0].Bind(cmdbuf, first_descriptor_sets[frame_count % LSFG_HISTORY_SLOTS]);
    cmdbuf.Dispatch(groups_x, groups_y, 1);

    LsfgBarriers(cmdbuf).WriteToReadAll(temp1).ReadToWriteAll(temp2).Build();
    passes[1].Bind(cmdbuf, descriptor_sets[0]);
    cmdbuf.Dispatch(groups_x, groups_y, 1);

    LsfgBarriers(cmdbuf).WriteToReadAll(temp2).ReadToWriteAll(temp1).Build();
    passes[2].Bind(cmdbuf, descriptor_sets[1]);
    cmdbuf.Dispatch(groups_x, groups_y, 1);

    LsfgBarriers(cmdbuf).WriteToReadAll(temp1).ReadToWriteAll(temp2).Build();
    passes[3].Bind(cmdbuf, descriptor_sets[2]);
    cmdbuf.Dispatch(groups_x, groups_y, 1);

    LsfgBarriers(cmdbuf).WriteToReadAll(temp2).ReadToWriteAll(out_images).Build();
    passes[4].Bind(cmdbuf, descriptor_sets[3]);
    cmdbuf.Dispatch(GroupCount(extent.width, OUTPUT_TILE_SHIFT),
                    GroupCount(extent.height, OUTPUT_TILE_SHIFT), 1);
}

} // namespace rex::ui::vulkan::framegen
