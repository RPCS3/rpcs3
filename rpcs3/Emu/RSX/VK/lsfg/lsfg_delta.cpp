// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

// SPDX-FileCopyrightText: Copyright 2025 lsfg-vk
// SPDX-License-Identifier: GPL-3.0-or-later

// Adapted for ReX/PS5 in 2026; origin Eden 5f142c7926d0c7fcbbd0ce30794d72f638a43b2a.

#include <vector>

#include "lossless_dll.h"
#include "lsfg_delta.h"
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

LsfgDelta::LsfgDelta(const Device& device, MemoryAllocator& memory_allocator,
                     const LsfgShaders& shaders, LsfgResources& resources,
                     vk::DescriptorPool& descriptor_pool, LsfgImageHistory& inputs_,
                     LsfgImage& flow_input_, LsfgImage* previous_gamma_, LsfgImage* previous1_,
                     LsfgImage* previous2_)
    : inputs{&inputs_}, flow_input{&flow_input_}, previous_gamma{previous_gamma_},
      previous1{previous1_}, previous2{previous2_} {
    using namespace PerformanceShader;

    passes[0] = LsfgPass(device, shaders, DELTA[0],
                         {{1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER},
                          {2, VK_DESCRIPTOR_TYPE_SAMPLER},
                          {5, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE},
                          {3, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE}});
    passes[1] = LsfgPass(device, shaders, DELTA[1],
                         {{1, VK_DESCRIPTOR_TYPE_SAMPLER},
                          {3, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE},
                          {2, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE}});
    passes[2] = LsfgPass(device, shaders, DELTA[2],
                         {{1, VK_DESCRIPTOR_TYPE_SAMPLER},
                          {2, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE},
                          {2, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE}});
    passes[3] = LsfgPass(device, shaders, DELTA[3],
                         {{1, VK_DESCRIPTOR_TYPE_SAMPLER},
                          {2, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE},
                          {2, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE}});
    passes[4] = LsfgPass(device, shaders, DELTA[4],
                         {{1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER},
                          {2, VK_DESCRIPTOR_TYPE_SAMPLER},
                          {4, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE},
                          {1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE}});
    passes[5] = LsfgPass(device, shaders, DELTA[5],
                         {{1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER},
                          {2, VK_DESCRIPTOR_TYPE_SAMPLER},
                          {6, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE},
                          {1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE}});
    for (size_t i = 6; i < LSFG_DELTA_STAGES - 1; ++i) {
        passes[i] = LsfgPass(device, shaders, DELTA[i],
                             {{1, VK_DESCRIPTOR_TYPE_SAMPLER},
                              {1, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE},
                              {1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE}});
    }
    passes[9] = LsfgPass(device, shaders, DELTA[9],
                         {{1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER},
                          {2, VK_DESCRIPTOR_TYPE_SAMPLER},
                          {2, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE},
                          {1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE}});

    const VkExtent2D extent = (*inputs)[0][0].Extent();
    for (auto& image : temp1) {
        image = LsfgImage(device, memory_allocator, extent);
    }
    for (auto& image : temp2) {
        image = LsfgImage(device, memory_allocator, extent);
    }
    out_image1 = LsfgImage(device, memory_allocator, extent, LSFG_MOTION_FORMAT);
    out_image2 = LsfgImage(device, memory_allocator, extent, LSFG_MOTION_FORMAT);

    std::vector<VkDescriptorSetLayout> layouts;
    for (size_t slot = 0; slot < LSFG_GENERATION_SLOTS; ++slot) {
        for (size_t i = 0; i < LSFG_HISTORY_SLOTS; ++i) {
            layouts.push_back(passes[0].SetLayout());
        }
        for (size_t i = 1; i <= 4; ++i) {
            layouts.push_back(passes[i].SetLayout());
        }
        for (size_t i = 0; i < LSFG_HISTORY_SLOTS; ++i) {
            layouts.push_back(passes[5].SetLayout());
        }
        for (size_t i = 6; i < LSFG_DELTA_STAGES; ++i) {
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
            resources.GetBuffer(LsfgSlotTimestamp(slot), false, previous_gamma == nullptr);

        for (size_t i = 0; i < LSFG_HISTORY_SLOTS; ++i) {
            pass.first_descriptor_sets[i] = owned_sets[next++];
        }
        for (size_t i = 0; i < 4; ++i) {
            pass.descriptor_sets[i] = owned_sets[next++];
        }
        for (size_t i = 0; i < LSFG_HISTORY_SLOTS; ++i) {
            pass.sixth_descriptor_sets[i] = owned_sets[next++];
        }
        for (size_t i = 4; i < LSFG_DELTA_STAGES - 2; ++i) {
            pass.descriptor_sets[i] = owned_sets[next++];
        }

        for (size_t i = 0; i < LSFG_HISTORY_SLOTS; ++i) {
            LsfgDescriptorWriter(pass.first_descriptor_sets[i])
                .AddUniformBuffer(buffer, LsfgResources::BufferSize())
                .AddSampler(border_sampler)
                .AddSampler(edge_sampler)
                .AddSampledImages((*inputs)[(i + 2) % LSFG_HISTORY_SLOTS])
                .AddSampledImages((*inputs)[i % LSFG_HISTORY_SLOTS])
                .AddSampledImage(previous_gamma)
                .AddStorageImages(temp1)
                .Build(device);
            LsfgDescriptorWriter(pass.sixth_descriptor_sets[i])
                .AddUniformBuffer(buffer, LsfgResources::BufferSize())
                .AddSampler(border_sampler)
                .AddSampler(edge_sampler)
                .AddSampledImages((*inputs)[(i + 2) % LSFG_HISTORY_SLOTS])
                .AddSampledImages((*inputs)[i % LSFG_HISTORY_SLOTS])
                .AddSampledImage(previous_gamma)
                .AddSampledImage(previous1)
                .AddStorageImage(temp2[0])
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
            .AddSampledImage(previous_gamma)
            .AddSampledImage(*flow_input)
            .AddStorageImage(out_image1)
            .Build(device);
        LsfgDescriptorWriter(pass.descriptor_sets[4])
            .AddSampler(sampler)
            .AddSampledImage(temp2[0])
            .AddStorageImage(temp1[0])
            .Build(device);
        LsfgDescriptorWriter(pass.descriptor_sets[5])
            .AddSampler(sampler)
            .AddSampledImage(temp1[0])
            .AddStorageImage(temp2[0])
            .Build(device);
        LsfgDescriptorWriter(pass.descriptor_sets[6])
            .AddSampler(sampler)
            .AddSampledImage(temp2[0])
            .AddStorageImage(temp1[0])
            .Build(device);
        LsfgDescriptorWriter(pass.descriptor_sets[7])
            .AddUniformBuffer(buffer, LsfgResources::BufferSize())
            .AddSampler(sampler)
            .AddSampler(edge_sampler)
            .AddSampledImage(temp1[0])
            .AddSampledImage(previous2)
            .AddStorageImage(out_image2)
            .Build(device);
    }
}

void LsfgDelta::Dispatch(vk::CommandBuffer cmdbuf, u64 frame_count, size_t slot) {
    const Generation& pass = generations[slot];

    const VkExtent2D extent = temp1[0].Extent();
    const u32 groups_x = GroupCount(extent.width);
    const u32 groups_y = GroupCount(extent.height);

    const size_t history = frame_count % LSFG_HISTORY_SLOTS;
    const size_t previous_history = (frame_count + 2) % LSFG_HISTORY_SLOTS;

    LsfgBarriers(cmdbuf)
        .WriteToReadAll((*inputs)[previous_history])
        .WriteToReadAll((*inputs)[history])
        .WriteToRead(previous_gamma)
        .ReadToWriteAll(temp1)
        .Build();
    passes[0].Bind(cmdbuf, pass.first_descriptor_sets[history]);
    cmdbuf.Dispatch(groups_x, groups_y, 1);

    LsfgBarriers(cmdbuf).WriteToReadAll(temp1).ReadToWriteAll(temp2).Build();
    passes[1].Bind(cmdbuf, pass.descriptor_sets[0]);
    cmdbuf.Dispatch(groups_x, groups_y, 1);

    LsfgBarriers(cmdbuf).WriteToReadAll(temp2).ReadToWriteAll(temp1).Build();
    passes[2].Bind(cmdbuf, pass.descriptor_sets[1]);
    cmdbuf.Dispatch(groups_x, groups_y, 1);

    LsfgBarriers(cmdbuf).WriteToReadAll(temp1).ReadToWriteAll(temp2).Build();
    passes[3].Bind(cmdbuf, pass.descriptor_sets[2]);
    cmdbuf.Dispatch(groups_x, groups_y, 1);

    LsfgBarriers(cmdbuf)
        .WriteToReadAll(temp2)
        .WriteToRead(previous_gamma)
        .WriteToRead(*flow_input)
        .ReadToWrite(out_image1)
        .Build();
    passes[4].Bind(cmdbuf, pass.descriptor_sets[3]);
    cmdbuf.Dispatch(groups_x, groups_y, 1);

    LsfgBarriers(cmdbuf)
        .WriteToReadAll((*inputs)[previous_history])
        .WriteToReadAll((*inputs)[history])
        .WriteToRead(previous_gamma)
        .WriteToRead(previous1)
        .ReadToWriteAll(temp2)
        .Build();
    passes[5].Bind(cmdbuf, pass.sixth_descriptor_sets[history]);
    cmdbuf.Dispatch(groups_x, groups_y, 1);

    LsfgBarriers(cmdbuf)
        .WriteToReadAll(temp2)
        .ReadToWrite(temp1[0])
        .ReadToWrite(temp1[1])
        .Build();
    passes[6].Bind(cmdbuf, pass.descriptor_sets[4]);
    cmdbuf.Dispatch(groups_x, groups_y, 1);

    LsfgBarriers(cmdbuf)
        .WriteToRead(temp1[0])
        .WriteToRead(temp1[1])
        .ReadToWriteAll(temp2)
        .Build();
    passes[7].Bind(cmdbuf, pass.descriptor_sets[5]);
    cmdbuf.Dispatch(groups_x, groups_y, 1);

    LsfgBarriers(cmdbuf)
        .WriteToReadAll(temp2)
        .ReadToWrite(temp1[0])
        .ReadToWrite(temp1[1])
        .Build();
    passes[8].Bind(cmdbuf, pass.descriptor_sets[6]);
    cmdbuf.Dispatch(groups_x, groups_y, 1);

    LsfgBarriers(cmdbuf)
        .WriteToRead(temp1[0])
        .WriteToRead(temp1[1])
        .WriteToRead(previous2)
        .ReadToWrite(out_image2)
        .Build();
    passes[9].Bind(cmdbuf, pass.descriptor_sets[7]);
    cmdbuf.Dispatch(groups_x, groups_y, 1);
}

} // namespace rex::ui::vulkan::framegen
