// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

// SPDX-FileCopyrightText: Copyright 2025 lsfg-vk
// SPDX-License-Identifier: GPL-3.0-or-later

#include "lsfg_chain.hpp"
#include "lsfg_shaders.hpp"

#include <algorithm>

namespace lsfg {

namespace {

constexpr uint32_t FIXED_DESCRIPTOR_SETS = 64;
constexpr uint32_t DESCRIPTOR_SETS_PER_SLOT = 112;
[[nodiscard]] constexpr bool HasDelta(size_t i) {
    return i >= LSFG_FIRST_DELTA_LEVEL && i <= LSFG_LAST_DELTA_LEVEL;
}

}

LsfgChain::LsfgChain(const Device& device, const LsfgShaders& shaders, VkExtent2D extent,
                     VkFormat format, float flow_scale)
    : resources{device, flow_scale}, owner{device.Handle()} {
    descriptor_pool = CreateLsfgDescriptorPool(
        device, FIXED_DESCRIPTOR_SETS +
                    DESCRIPTOR_SETS_PER_SLOT * static_cast<uint32_t>(LSFG_GENERATION_SLOTS));
    if (descriptor_pool == VK_NULL_HANDLE) return;

    for (auto& image : frames) {
        image = LsfgImage(device, extent, format);
        if (!image.Valid()) return;
    }

    mipmaps = LsfgMipmaps(device, shaders, resources, descriptor_pool, frames, flow_scale);
    if (!mipmaps.Valid()) return;

    alpha_passes = LsfgAlphaPasses(device, shaders);
    if (!alpha_passes.Valid()) return;

    for (size_t i = 0; i < LSFG_MIP_LEVELS; ++i) {
        alpha[i] = LsfgAlpha(device, alpha_passes, resources, descriptor_pool, mipmaps.Output(i));
        if (!alpha[i].Valid()) return;
    }

    beta = LsfgBeta(device, shaders, resources, descriptor_pool, alpha[0].Outputs());
    if (!beta.Valid()) return;

    for (size_t i = 0; i < LSFG_MIP_LEVELS; ++i) {
        const size_t level = LSFG_MIP_LEVELS - 1 - i;
        gamma[i] = LsfgGamma(device, shaders, resources, descriptor_pool, alpha[level].Outputs(),
                             beta.Output(std::min(level, LSFG_BETA_OUTPUTS - 1)),
                             i == 0 ? nullptr : &gamma[i - 1].Output());
        if (!gamma[i].Valid()) return;

        if (!HasDelta(i)) {
            continue;
        }

        const size_t index = i - LSFG_FIRST_DELTA_LEVEL;
        const bool first = i == LSFG_FIRST_DELTA_LEVEL;
        delta[index] = LsfgDelta(device, shaders, resources, descriptor_pool,
                                 alpha[level].Outputs(), beta.Output(level),
                                 first ? nullptr : &gamma[i - 1].Output(),
                                 first ? nullptr : &delta[index - 1].Output1(),
                                 first ? nullptr : &delta[index - 1].Output2());
        if (!delta[index].Valid()) return;
    }

    generate = LsfgGenerate(device, shaders, resources, descriptor_pool, frames,
                            gamma[LSFG_MIP_LEVELS - 1].Output(),
                            delta[LSFG_DELTA_INSTANCES - 1].Output1(),
                            delta[LSFG_DELTA_INSTANCES - 1].Output2());
    if (!generate.Valid()) return;

    valid = true;
}

LsfgChain::~LsfgChain() {
    if (descriptor_pool != VK_NULL_HANDLE) {
        VK_GET_SYMBOL(vkDestroyDescriptorPool)(owner, descriptor_pool, nullptr);
        descriptor_pool = VK_NULL_HANDLE;
    }
}

void LsfgChain::DispatchShared(VkCommandBuffer cmdbuf, uint64_t frame_count) {
    resources.PrepareDummies(cmdbuf);

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

void LsfgChain::DispatchGeneration(VkCommandBuffer cmdbuf, uint64_t frame_count,
                                   size_t generation_count, size_t generation, uint32_t target,
                                   VkImage image, VkExtent2D extent) {
    const size_t slot = LsfgGenerationSlot(generation_count, generation);
    constexpr size_t PAIRED_STEPS =
        LSFG_GAMMA_STAGES > LSFG_DELTA_STAGES ? LSFG_GAMMA_STAGES : LSFG_DELTA_STAGES;

    for (size_t i = 0; i < LSFG_MIP_LEVELS; ++i) {
        if (!HasDelta(i)) {
            gamma[i].Dispatch(cmdbuf, frame_count, slot);
            continue;
        }

        LsfgDelta& paired = delta[i - LSFG_FIRST_DELTA_LEVEL];
        for (size_t step = 0; step < PAIRED_STEPS; ++step) {
            LsfgBarriers barriers(cmdbuf);
            if (step < LSFG_GAMMA_STAGES) {
                gamma[i].PushStepBarriers(barriers, frame_count, step);
            }
            if (step < LSFG_DELTA_STAGES) {
                paired.PushStepBarriers(barriers, frame_count, step);
            }
            barriers.Build();

            if (step < LSFG_GAMMA_STAGES) {
                gamma[i].DispatchStep(cmdbuf, frame_count, slot, step);
            }
            if (step < LSFG_DELTA_STAGES) {
                paired.DispatchStep(cmdbuf, frame_count, slot, step);
            }
        }
    }
    generate.Dispatch(cmdbuf, frame_count, slot, target, image, extent);
}

}
