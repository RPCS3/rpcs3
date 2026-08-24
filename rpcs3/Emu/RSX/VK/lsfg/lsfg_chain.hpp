// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

// SPDX-FileCopyrightText: Copyright 2025 lsfg-vk
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <array>

#include "lsfg_alpha.hpp"
#include "lsfg_beta.hpp"
#include "lsfg_common.hpp"
#include "lsfg_delta.hpp"
#include "lsfg_gamma.hpp"
#include "lsfg_generate.hpp"
#include "lsfg_mipmaps.hpp"

namespace lsfg {

class LsfgShaders;

constexpr size_t LSFG_DELTA_INSTANCES = 3;

class LsfgChain {
public:
    LsfgChain(const Device& device, const LsfgShaders& shaders, VkExtent2D extent, VkFormat format,
              float flow_scale);
    ~LsfgChain();

    LsfgChain(const LsfgChain&) = delete;
    LsfgChain& operator=(const LsfgChain&) = delete;

    void DispatchShared(VkCommandBuffer cmdbuf, uint64_t frame_count);

    void DispatchGeneration(VkCommandBuffer cmdbuf, uint64_t frame_count, size_t generation_count,
                            size_t generation, uint32_t target, VkImage image, VkExtent2D extent);

    void SetTarget(const Device& device, size_t generation_count, size_t generation,
                   uint32_t target, VkImageView view) {
        generate.SetTarget(device, LsfgGenerationSlot(generation_count, generation), target, view);
    }

    void ForgetTargets() {
        generate.ForgetTargets();
    }

    [[nodiscard]] LsfgImage& Input(uint64_t frame_count) {
        return frames[frame_count % frames.size()];
    }

    [[nodiscard]] LsfgImage& FlowLevel(size_t level) {
        return mipmaps.Output(level);
    }

    [[nodiscard]] bool Valid() const {
        return valid;
    }

private:
    LsfgResources resources;
    VkDevice owner{VK_NULL_HANDLE};
    VkDescriptorPool descriptor_pool{VK_NULL_HANDLE};

    LsfgImagePair frames;
    LsfgMipmaps mipmaps;
    LsfgAlphaPasses alpha_passes;
    std::array<LsfgAlpha, LSFG_MIP_LEVELS> alpha;
    LsfgBeta beta;
    std::array<LsfgGamma, LSFG_MIP_LEVELS> gamma;
    std::array<LsfgDelta, LSFG_DELTA_INSTANCES> delta;
    LsfgGenerate generate;
    bool valid{};
};

}
