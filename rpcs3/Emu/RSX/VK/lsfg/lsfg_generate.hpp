// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

// SPDX-FileCopyrightText: Copyright 2025 lsfg-vk
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <array>

#include "lsfg_common.hpp"

namespace lsfg {

class LsfgShaders;

class LsfgGenerate {
public:
    LsfgGenerate() = default;
    LsfgGenerate(const Device& device, const LsfgShaders& shaders, LsfgResources& resources,
                 VkDescriptorPool descriptor_pool, LsfgImagePair& frames, LsfgImage& motion,
                 LsfgImage& detail1, LsfgImage& detail2);

    void SetTarget(const Device& device, size_t slot, uint32_t target, VkImageView view);

    void ForgetTargets();

    void Dispatch(VkCommandBuffer cmdbuf, uint64_t frame_count, size_t slot, uint32_t target,
                  VkImage image, VkExtent2D extent);

    [[nodiscard]] bool Valid() const {
        return pass.Valid() && allocated;
    }

private:
    struct Target {
        std::array<VkDescriptorSet, 2> descriptor_sets{};
        VkImageView view{VK_NULL_HANDLE};
    };

    struct Generation {
        std::array<Target, LSFG_MAX_TARGETS> targets{};
        VkBuffer buffer{VK_NULL_HANDLE};
    };

    LsfgImagePair* frames{};
    LsfgImage* motion{};
    LsfgImage* detail1{};
    LsfgImage* detail2{};
    VkSampler sampler{VK_NULL_HANDLE};
    VkSampler edge_sampler{VK_NULL_HANDLE};

    LsfgPass pass;
    std::array<Generation, LSFG_GENERATION_SLOTS> generations{};
    bool allocated{};
};

}
