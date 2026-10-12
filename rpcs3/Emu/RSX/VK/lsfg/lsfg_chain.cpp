// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

// SPDX-FileCopyrightText: Copyright 2025 lsfg-vk
// SPDX-License-Identifier: GPL-3.0-or-later

// Adapted for ReX/PS5 in 2026; origin Eden 5f142c7926d0c7fcbbd0ce30794d72f638a43b2a.

#include <algorithm>

#include "lsfg_chain.h"
#include "lsfg_shaders.h"
#include "vulkan_adapter.h"

namespace rex::ui::vulkan::framegen {

namespace {

constexpr u32 FIXED_DESCRIPTOR_SETS = 64;
constexpr u32 DESCRIPTOR_SETS_PER_SLOT = 112;
constexpr size_t FIRST_DELTA_LEVEL = 4;

} // Anonymous namespace

LsfgChain::LsfgChain(const Device& device, MemoryAllocator& memory_allocator,
                     const LsfgShaders& shaders, VkExtent2D extent, VkFormat format,
                     f32 flow_scale)
    : resources{device, memory_allocator, flow_scale},
      descriptor_pool{CreateLsfgDescriptorPool(
          device, FIXED_DESCRIPTOR_SETS +
                      DESCRIPTOR_SETS_PER_SLOT * static_cast<u32>(LSFG_GENERATION_SLOTS))} {
    for (auto& image : frames) {
        image = LsfgImage(device, memory_allocator, extent, format);
    }

    mipmaps = LsfgMipmaps(device, memory_allocator, shaders, resources, descriptor_pool, frames,
                          flow_scale);

    alpha_passes = LsfgAlphaPasses(device, shaders);
    for (size_t i = 0; i < LSFG_MIP_LEVELS; ++i) {
        alpha[i] = LsfgAlpha(device, memory_allocator, alpha_passes, resources, descriptor_pool,
                             mipmaps.Output(i));
    }

    beta = LsfgBeta(device, memory_allocator, shaders, resources, descriptor_pool,
                    alpha[0].Outputs());

    for (size_t i = 0; i < LSFG_MIP_LEVELS; ++i) {
        const size_t level = LSFG_MIP_LEVELS - 1 - i;
        gamma[i] = LsfgGamma(device, memory_allocator, shaders, resources, descriptor_pool,
                             alpha[level].Outputs(),
                             beta.Output(std::min(level, LSFG_BETA_OUTPUTS - 1)),
                             i == 0 ? nullptr : &gamma[i - 1].Output());

        if (i < FIRST_DELTA_LEVEL) {
            continue;
        }

        const size_t index = i - FIRST_DELTA_LEVEL;
        delta[index] = LsfgDelta(
            device, memory_allocator, shaders, resources, descriptor_pool, alpha[level].Outputs(),
            beta.Output(level), i == FIRST_DELTA_LEVEL ? nullptr : &gamma[i - 1].Output(),
            i == FIRST_DELTA_LEVEL ? nullptr : &delta[index - 1].Output1(),
            i == FIRST_DELTA_LEVEL ? nullptr : &delta[index - 1].Output2());
    }

    generate = LsfgGenerate(device, shaders, resources, descriptor_pool, frames,
                            gamma[LSFG_MIP_LEVELS - 1].Output(),
                            delta[LSFG_DELTA_INSTANCES - 1].Output1(),
                            delta[LSFG_DELTA_INSTANCES - 1].Output2());
}

void LsfgChain::DispatchShared(vk::CommandBuffer cmdbuf, u64 frame_count) {
    mipmaps.Dispatch(cmdbuf, frame_count);

    for (size_t stage = 0; stage < LSFG_ALPHA_STAGES; ++stage) {
        LsfgBarriers barriers(cmdbuf);
        for (auto& level : alpha) {
            level.PushBarriers(barriers, frame_count, stage);
        }
        barriers.Build();

        alpha_passes.Get(stage).BindPipeline(cmdbuf);
        for (auto& level : alpha) {
            level.DispatchStage(cmdbuf, frame_count, stage);
        }
    }

    beta.Dispatch(cmdbuf, frame_count);
}

void LsfgChain::DispatchGeneration(vk::CommandBuffer cmdbuf, u64 frame_count,
                                   size_t generation_count, size_t generation, u32 target,
                                   VkImage image, VkExtent2D extent) {
    const size_t slot = LsfgGenerationSlot(generation_count, generation);
    for (size_t i = 0; i < LSFG_MIP_LEVELS; ++i) {
        gamma[i].Dispatch(cmdbuf, frame_count, slot);
        if (i >= FIRST_DELTA_LEVEL) {
            delta[i - FIRST_DELTA_LEVEL].Dispatch(cmdbuf, frame_count, slot);
        }
    }
    generate.Dispatch(cmdbuf, frame_count, slot, target, image, extent);
}

} // namespace rex::ui::vulkan::framegen
