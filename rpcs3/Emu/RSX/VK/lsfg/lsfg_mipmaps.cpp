// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

// SPDX-FileCopyrightText: Copyright 2025 lsfg-vk
// SPDX-License-Identifier: GPL-3.0-or-later

#include "lsfg_mipmaps.hpp"
#include "lsfg_dll.h"
#include "lsfg_shaders.hpp"

#include <algorithm>
#include <vector>

namespace lsfg {

namespace {

constexpr uint32_t DISPATCH_TILE_SHIFT = 6;

[[nodiscard]] uint32_t GroupCount(uint32_t size) {
    return (size + (1u << DISPATCH_TILE_SHIFT) - 1) >> DISPATCH_TILE_SHIFT;
}

}

LsfgMipmaps::LsfgMipmaps(const Device& device, const LsfgShaders& shaders,
                         LsfgResources& resources, VkDescriptorPool descriptor_pool,
                         LsfgImagePair& frames_, float flow_scale)
    : frames{&frames_} {
    pass = LsfgPass(device, shaders, LSFG_SHADER_MIPMAPS,
                    {{1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER},
                     {1, VK_DESCRIPTOR_TYPE_SAMPLER},
                     {1, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE},
                     {LSFG_MIP_LEVELS, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE}});
    if (!pass.Valid()) return;

    const VkExtent2D input_extent = (*frames)[0].Extent();
    flow_extent = VkExtent2D{
        std::max(1u, static_cast<uint32_t>(static_cast<float>(input_extent.width) * flow_scale)),
        std::max(1u, static_cast<uint32_t>(static_cast<float>(input_extent.height) * flow_scale)),
    };

    for (size_t i = 0; i < LSFG_MIP_LEVELS; ++i) {
        const VkExtent2D level_extent{
            std::max(1u, flow_extent.width >> i),
            std::max(1u, flow_extent.height >> i),
        };
        out_images[i] = LsfgImage(device, level_extent, LSFG_FLOW_FORMAT);
        if (!out_images[i].Valid()) return;
    }

    const std::vector<VkDescriptorSet> sets = AllocateLsfgDescriptorSets(
        device, descriptor_pool, pass.SetLayout(),
        static_cast<uint32_t>(descriptor_sets.size()));
    if (sets.size() != descriptor_sets.size()) return;

    const VkSampler sampler = resources.GetSampler();
    const VkBuffer buffer = resources.GetBuffer();

    for (size_t i = 0; i < descriptor_sets.size(); ++i) {
        descriptor_sets[i] = sets[i];
        LsfgDescriptorWriter(descriptor_sets[i])
            .AddUniformBuffer(buffer, LsfgResources::BufferSize())
            .AddSampler(sampler)
            .AddSampledImage((*frames)[i])
            .AddStorageImages(out_images)
            .Build(device);
    }
}

void LsfgMipmaps::Dispatch(VkCommandBuffer cmdbuf, uint64_t frame_count) {
    const size_t slot = frame_count % descriptor_sets.size();

    LsfgBarriers(cmdbuf).WriteToRead((*frames)[slot]).ReadToWriteAll(out_images).Build();

    pass.Bind(cmdbuf, descriptor_sets[slot]);
    VK_GET_SYMBOL(vkCmdDispatch)(cmdbuf, GroupCount(flow_extent.width), GroupCount(flow_extent.height), 1);
}

}
