// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

// SPDX-FileCopyrightText: Copyright 2025 lsfg-vk
// SPDX-License-Identifier: GPL-3.0-or-later

// Adapted for ReX/PS5 in 2026; origin Eden 5f142c7926d0c7fcbbd0ce30794d72f638a43b2a.

#include <vector>

#include "lossless_dll.h"
#include "lsfg_alpha.h"
#include "lsfg_shaders.h"
#include "util.h"
#include "vulkan_adapter.h"

namespace rex::ui::vulkan::framegen {

namespace {

constexpr u32 DISPATCH_TILE_SHIFT = 3;

[[nodiscard]] u32 GroupCount(u32 size) {
    return (size + (1u << DISPATCH_TILE_SHIFT) - 1) >> DISPATCH_TILE_SHIFT;
}

[[nodiscard]] VkExtent2D HalveExtent(VkExtent2D extent) {
    return VkExtent2D{
        .width = (extent.width + 1) >> 1,
        .height = (extent.height + 1) >> 1,
    };
}

} // Anonymous namespace

LsfgAlphaPasses::LsfgAlphaPasses(const Device& device, const LsfgShaders& shaders) {
    using namespace PerformanceShader;

    passes[0] = LsfgPass(device, shaders, ALPHA[0],
                         {{1, VK_DESCRIPTOR_TYPE_SAMPLER},
                          {1, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE},
                          {1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE}});
    passes[1] = LsfgPass(device, shaders, ALPHA[1],
                         {{1, VK_DESCRIPTOR_TYPE_SAMPLER},
                          {1, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE},
                          {1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE}});
    passes[2] = LsfgPass(device, shaders, ALPHA[2],
                         {{1, VK_DESCRIPTOR_TYPE_SAMPLER},
                          {1, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE},
                          {2, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE}});
    passes[3] = LsfgPass(device, shaders, ALPHA[3],
                         {{1, VK_DESCRIPTOR_TYPE_SAMPLER},
                          {2, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE},
                          {2, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE}});
}

LsfgAlpha::LsfgAlpha(const Device& device, MemoryAllocator& memory_allocator,
                     const LsfgAlphaPasses& passes_, LsfgResources& resources,
                     vk::DescriptorPool& descriptor_pool, LsfgImage& input_)
    : passes{&passes_}, input{&input_} {
    const VkExtent2D half_extent = HalveExtent(input->Extent());
    const VkExtent2D quarter_extent = HalveExtent(half_extent);

    temp1 = LsfgImage(device, memory_allocator, half_extent);
    temp2 = LsfgImage(device, memory_allocator, half_extent);
    for (size_t i = 0; i < temp3.size(); ++i) {
        temp3[i] = LsfgImage(device, memory_allocator, quarter_extent);
        for (size_t j = 0; j < LSFG_HISTORY_SLOTS; ++j) {
            out_images[j][i] = LsfgImage(device, memory_allocator, quarter_extent);
        }
    }

    std::vector<VkDescriptorSetLayout> layouts;
    for (size_t i = 0; i < LSFG_ALPHA_STAGES - 1; ++i) {
        layouts.push_back(passes->Get(i).SetLayout());
    }
    for (size_t i = 0; i < LSFG_HISTORY_SLOTS; ++i) {
        layouts.push_back(passes->Get(3).SetLayout());
    }
    owned_sets = CreateWrappedDescriptorSets(descriptor_pool, layouts);

    for (size_t i = 0; i < LSFG_ALPHA_STAGES - 1; ++i) {
        descriptor_sets[i] = owned_sets[i];
    }
    for (size_t i = 0; i < LSFG_HISTORY_SLOTS; ++i) {
        last_descriptor_sets[i] = owned_sets[LSFG_ALPHA_STAGES - 1 + i];
    }

    const VkSampler sampler = resources.GetSampler();

    LsfgDescriptorWriter(descriptor_sets[0])
        .AddSampler(sampler)
        .AddSampledImage(*input)
        .AddStorageImage(temp1)
        .Build(device);
    LsfgDescriptorWriter(descriptor_sets[1])
        .AddSampler(sampler)
        .AddSampledImage(temp1)
        .AddStorageImage(temp2)
        .Build(device);
    LsfgDescriptorWriter(descriptor_sets[2])
        .AddSampler(sampler)
        .AddSampledImage(temp2)
        .AddStorageImages(temp3)
        .Build(device);
    for (size_t i = 0; i < LSFG_HISTORY_SLOTS; ++i) {
        LsfgDescriptorWriter(last_descriptor_sets[i])
            .AddSampler(sampler)
            .AddSampledImages(temp3)
            .AddStorageImages(out_images[i])
            .Build(device);
    }
}

void LsfgAlpha::PushBarriers(LsfgBarriers& barriers, u64 frame_count, size_t stage) {
    switch (stage) {
    case 0:
        barriers.WriteToRead(*input).ReadToWrite(temp1);
        break;
    case 1:
        barriers.WriteToRead(temp1).ReadToWrite(temp2);
        break;
    case 2:
        barriers.WriteToRead(temp2).ReadToWriteAll(temp3);
        break;
    default:
        barriers.WriteToReadAll(temp3).ReadToWriteAll(out_images[frame_count % LSFG_HISTORY_SLOTS]);
        break;
    }
}

void LsfgAlpha::DispatchStage(vk::CommandBuffer cmdbuf, u64 frame_count, size_t stage) {
    const VkExtent2D extent = stage < 2 ? temp1.Extent() : temp3[0].Extent();
    const VkDescriptorSet set = stage < LSFG_ALPHA_STAGES - 1
                                    ? descriptor_sets[stage]
                                    : last_descriptor_sets[frame_count % LSFG_HISTORY_SLOTS];

    passes->Get(stage).BindSet(cmdbuf, set);
    cmdbuf.Dispatch(GroupCount(extent.width), GroupCount(extent.height), 1);
}

} // namespace rex::ui::vulkan::framegen
