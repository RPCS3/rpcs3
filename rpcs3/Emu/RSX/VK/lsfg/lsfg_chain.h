// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

// SPDX-FileCopyrightText: Copyright 2025 lsfg-vk
// SPDX-License-Identifier: GPL-3.0-or-later

// Adapted for ReX/PS5 in 2026; origin Eden 5f142c7926d0c7fcbbd0ce30794d72f638a43b2a.

#pragma once

#include <array>

#include "types.h"
#include "lsfg_alpha.h"
#include "lsfg_beta.h"
#include "lsfg_common.h"
#include "lsfg_delta.h"
#include "lsfg_gamma.h"
#include "lsfg_generate.h"
#include "lsfg_mipmaps.h"

namespace rex::ui::vulkan::framegen {

class Device;
class LsfgShaders;

constexpr size_t LSFG_DELTA_INSTANCES = 3;

class LsfgChain {
public:
    LsfgChain(const Device& device, MemoryAllocator& memory_allocator, const LsfgShaders& shaders,
              VkExtent2D extent, VkFormat format, f32 flow_scale);

    LsfgChain(const LsfgChain&) = delete;
    LsfgChain& operator=(const LsfgChain&) = delete;

    void DispatchShared(vk::CommandBuffer cmdbuf, u64 frame_count);

    void DispatchGeneration(vk::CommandBuffer cmdbuf, u64 frame_count, size_t generation_count,
                            size_t generation, u32 target, VkImage image, VkExtent2D extent);

    void SetTarget(const Device& device, size_t generation_count, size_t generation, u32 target,
                   VkImageView view) {
        generate.SetTarget(device, LsfgGenerationSlot(generation_count, generation), target, view);
    }

    [[nodiscard]] LsfgImage& Input(u64 frame_count) {
        return frames[frame_count % frames.size()];
    }

    [[nodiscard]] LsfgImage& FlowLevel(size_t level) {
        return mipmaps.Output(level);
    }

    [[nodiscard]] LsfgImage& AlphaOutput(size_t level, u64 frame_count, size_t index) {
        return alpha[level].Outputs()[frame_count % LSFG_HISTORY_SLOTS][index];
    }

    [[nodiscard]] LsfgImage& BetaOutput(size_t level) {
        return beta.Output(level);
    }

    [[nodiscard]] LsfgImage& GammaOutput(size_t index) {
        return gamma[index].Output();
    }

    [[nodiscard]] LsfgImage& DeltaOutput1(size_t index) {
        return delta[index].Output1();
    }

    [[nodiscard]] LsfgImage& DeltaOutput2(size_t index) {
        return delta[index].Output2();
    }

private:
    LsfgResources resources;
    vk::DescriptorPool descriptor_pool;

    LsfgImagePair frames;
    LsfgMipmaps mipmaps;
    LsfgAlphaPasses alpha_passes;
    std::array<LsfgAlpha, LSFG_MIP_LEVELS> alpha;
    LsfgBeta beta;
    std::array<LsfgGamma, LSFG_MIP_LEVELS> gamma;
    std::array<LsfgDelta, LSFG_DELTA_INSTANCES> delta;
    LsfgGenerate generate;
};

} // namespace rex::ui::vulkan::framegen
