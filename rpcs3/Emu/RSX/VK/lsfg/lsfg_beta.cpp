// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

// SPDX-FileCopyrightText: Copyright 2025 lsfg-vk
// SPDX-License-Identifier: GPL-3.0-or-later

#include "lsfg_beta.hpp"
#include "lsfg_shaders.hpp"

#include <vector>

namespace lsfg {

namespace {

constexpr uint32_t DISPATCH_TILE_SHIFT = 3;
constexpr uint32_t OUTPUT_TILE_SHIFT = 5;

[[nodiscard]] uint32_t GroupCount(uint32_t size, uint32_t shift) {
    return (size + (1u << shift) - 1) >> shift;
}

}

LsfgBeta::LsfgBeta(const Device& device, const LsfgShaders& shaders, LsfgResources& resources,
                   VkDescriptorPool descriptor_pool, LsfgImageHistory& inputs_)
    : inputs{&inputs_} {
    passes[0] = LsfgPass(device, shaders, LSFG_BETA_SHADERS[0],
                         {{1, VK_DESCRIPTOR_TYPE_SAMPLER},
                          {6, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE},
                          {2, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE}});
    for (size_t i = 1; i < LSFG_BETA_STAGES - 1; ++i) {
        passes[i] = LsfgPass(device, shaders, LSFG_BETA_SHADERS[i],
                             {{1, VK_DESCRIPTOR_TYPE_SAMPLER},
                              {2, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE},
                              {2, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE}});
    }
    passes[4] = LsfgPass(device, shaders, LSFG_BETA_SHADERS[4],
                         {{1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER},
                          {1, VK_DESCRIPTOR_TYPE_SAMPLER},
                          {2, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE},
                          {6, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE}});
    for (const auto& pass : passes) {
        if (!pass.Valid()) return;
    }

    const VkExtent2D extent = (*inputs)[0][0].Extent();
    for (size_t i = 0; i < temp1.size(); ++i) {
        temp1[i] = LsfgImage(device, extent);
        temp2[i] = LsfgImage(device, extent);
        if (!temp1[i].Valid() || !temp2[i].Valid()) return;
    }
    for (size_t i = 0; i < LSFG_BETA_OUTPUTS; ++i) {
        const VkExtent2D level_extent{
            extent.width >> i,
            extent.height >> i,
        };
        out_images[i] = LsfgImage(device, level_extent, LSFG_FLOW_FORMAT);
        if (!out_images[i].Valid()) return;
    }

    std::vector<VkDescriptorSetLayout> layouts;
    for (size_t i = 0; i < LSFG_HISTORY_SLOTS; ++i) {
        layouts.push_back(passes[0].SetLayout());
    }
    for (size_t i = 1; i < LSFG_BETA_STAGES; ++i) {
        layouts.push_back(passes[i].SetLayout());
    }

    const std::vector<VkDescriptorSet> sets =
        AllocateLsfgDescriptorSets(device, descriptor_pool, layouts);
    if (sets.size() != layouts.size()) return;

    for (size_t i = 0; i < LSFG_HISTORY_SLOTS; ++i) {
        first_descriptor_sets[i] = sets[i];
    }
    for (size_t i = 0; i < LSFG_BETA_STAGES - 1; ++i) {
        descriptor_sets[i] = sets[LSFG_HISTORY_SLOTS + i];
    }

    const VkSampler sampler = resources.GetSampler();
    const VkSampler border_sampler =
        resources.GetSampler(VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER, VK_COMPARE_OP_NEVER, true);

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
    allocated = true;
}

void LsfgBeta::Dispatch(VkCommandBuffer cmdbuf, uint64_t frame_count) {
    const VkExtent2D extent = temp1[0].Extent();
    const uint32_t groups_x = GroupCount(extent.width, DISPATCH_TILE_SHIFT);
    const uint32_t groups_y = GroupCount(extent.height, DISPATCH_TILE_SHIFT);

    LsfgBarriers barriers(cmdbuf);
    for (auto& slot : *inputs) {
        barriers.WriteToReadAll(slot);
    }
    barriers.ReadToWriteAll(temp1).Build();

    passes[0].Bind(cmdbuf, first_descriptor_sets[frame_count % LSFG_HISTORY_SLOTS]);
    VK_GET_SYMBOL(vkCmdDispatch)(cmdbuf, groups_x, groups_y, 1);

    LsfgBarriers(cmdbuf).WriteToReadAll(temp1).ReadToWriteAll(temp2).Build();
    passes[1].Bind(cmdbuf, descriptor_sets[0]);
    VK_GET_SYMBOL(vkCmdDispatch)(cmdbuf, groups_x, groups_y, 1);

    LsfgBarriers(cmdbuf).WriteToReadAll(temp2).ReadToWriteAll(temp1).Build();
    passes[2].Bind(cmdbuf, descriptor_sets[1]);
    VK_GET_SYMBOL(vkCmdDispatch)(cmdbuf, groups_x, groups_y, 1);

    LsfgBarriers(cmdbuf).WriteToReadAll(temp1).ReadToWriteAll(temp2).Build();
    passes[3].Bind(cmdbuf, descriptor_sets[2]);
    VK_GET_SYMBOL(vkCmdDispatch)(cmdbuf, groups_x, groups_y, 1);

    LsfgBarriers(cmdbuf).WriteToReadAll(temp2).ReadToWriteAll(out_images).Build();
    passes[4].Bind(cmdbuf, descriptor_sets[3]);
    VK_GET_SYMBOL(vkCmdDispatch)(cmdbuf, GroupCount(extent.width, OUTPUT_TILE_SHIFT),
                  GroupCount(extent.height, OUTPUT_TILE_SHIFT), 1);
}

}
