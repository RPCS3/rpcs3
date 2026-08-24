// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

// SPDX-FileCopyrightText: Copyright 2025 lsfg-vk
// SPDX-License-Identifier: GPL-3.0-or-later

#include "lsfg_alpha.hpp"
#include "lsfg_shaders.hpp"

#include <vector>

namespace lsfg {

namespace {

constexpr uint32_t DISPATCH_TILE_SHIFT = 3;

[[nodiscard]] uint32_t GroupCount(uint32_t size) {
    return (size + (1u << DISPATCH_TILE_SHIFT) - 1) >> DISPATCH_TILE_SHIFT;
}

[[nodiscard]] VkExtent2D HalveExtent(VkExtent2D extent) {
    return VkExtent2D{
        (extent.width + 1) >> 1,
        (extent.height + 1) >> 1,
    };
}

}

LsfgAlphaPasses::LsfgAlphaPasses(const Device& device, const LsfgShaders& shaders) {
    passes[0] = LsfgPass(device, shaders, LSFG_ALPHA_SHADERS[0],
                         {{1, VK_DESCRIPTOR_TYPE_SAMPLER},
                          {1, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE},
                          {1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE}});
    passes[1] = LsfgPass(device, shaders, LSFG_ALPHA_SHADERS[1],
                         {{1, VK_DESCRIPTOR_TYPE_SAMPLER},
                          {1, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE},
                          {1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE}});
    passes[2] = LsfgPass(device, shaders, LSFG_ALPHA_SHADERS[2],
                         {{1, VK_DESCRIPTOR_TYPE_SAMPLER},
                          {1, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE},
                          {2, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE}});
    passes[3] = LsfgPass(device, shaders, LSFG_ALPHA_SHADERS[3],
                         {{1, VK_DESCRIPTOR_TYPE_SAMPLER},
                          {2, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE},
                          {2, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE}});
}

bool LsfgAlphaPasses::Valid() const {
    for (const auto& pass : passes) {
        if (!pass.Valid()) return false;
    }
    return true;
}

LsfgAlpha::LsfgAlpha(const Device& device, const LsfgAlphaPasses& passes_,
                     LsfgResources& resources, VkDescriptorPool descriptor_pool, LsfgImage& input_)
    : passes{&passes_}, input{&input_} {
    if (!passes->Valid()) return;

    const VkExtent2D half_extent = HalveExtent(input->Extent());
    const VkExtent2D quarter_extent = HalveExtent(half_extent);

    temp1 = LsfgImage(device, half_extent);
    temp2 = LsfgImage(device, half_extent);
    if (!temp1.Valid() || !temp2.Valid()) return;

    for (size_t i = 0; i < temp3.size(); ++i) {
        temp3[i] = LsfgImage(device, quarter_extent);
        if (!temp3[i].Valid()) return;

        for (size_t j = 0; j < LSFG_HISTORY_SLOTS; ++j) {
            out_images[j][i] = LsfgImage(device, quarter_extent);
            if (!out_images[j][i].Valid()) return;
        }
    }

    std::vector<VkDescriptorSetLayout> layouts;
    for (size_t i = 0; i < LSFG_ALPHA_STAGES - 1; ++i) {
        layouts.push_back(passes->Get(i).SetLayout());
    }
    for (size_t i = 0; i < LSFG_HISTORY_SLOTS; ++i) {
        layouts.push_back(passes->Get(3).SetLayout());
    }

    const std::vector<VkDescriptorSet> sets =
        AllocateLsfgDescriptorSets(device, descriptor_pool, layouts);
    if (sets.size() != layouts.size()) return;

    for (size_t i = 0; i < LSFG_ALPHA_STAGES - 1; ++i) {
        descriptor_sets[i] = sets[i];
    }
    for (size_t i = 0; i < LSFG_HISTORY_SLOTS; ++i) {
        last_descriptor_sets[i] = sets[LSFG_ALPHA_STAGES - 1 + i];
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
    allocated = true;
}

void LsfgAlpha::PushBarriers(LsfgBarriers& barriers, uint64_t frame_count, size_t stage) {
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

void LsfgAlpha::DispatchStage(VkCommandBuffer cmdbuf, uint64_t frame_count, size_t stage) {
    const VkExtent2D extent = stage < 2 ? temp1.Extent() : temp3[0].Extent();
    const VkDescriptorSet set = stage < LSFG_ALPHA_STAGES - 1
                                    ? descriptor_sets[stage]
                                    : last_descriptor_sets[frame_count % LSFG_HISTORY_SLOTS];

    passes->Get(stage).BindSet(cmdbuf, set);
    VK_GET_SYMBOL(vkCmdDispatch)(cmdbuf, GroupCount(extent.width), GroupCount(extent.height), 1);
}

}
