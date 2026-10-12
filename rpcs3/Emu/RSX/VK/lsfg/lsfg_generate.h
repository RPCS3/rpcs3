// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

// SPDX-FileCopyrightText: Copyright 2025 lsfg-vk
// SPDX-License-Identifier: GPL-3.0-or-later

// Adapted for ReX/PS5 in 2026; origin Eden 5f142c7926d0c7fcbbd0ce30794d72f638a43b2a.

#pragma once

#include <array>

#include "types.h"
#include "lsfg_common.h"

namespace rex::ui::vulkan::framegen {

class Device;
class LsfgShaders;

class LsfgGenerate {
public:
    LsfgGenerate() = default;
    LsfgGenerate(const Device& device, const LsfgShaders& shaders, LsfgResources& resources,
                 vk::DescriptorPool& descriptor_pool, LsfgImagePair& frames, LsfgImage& motion,
                 LsfgImage& detail1, LsfgImage& detail2);

    void SetTarget(const Device& device, size_t slot, u32 target, VkImageView view);

    void Dispatch(vk::CommandBuffer cmdbuf, u64 frame_count, size_t slot, u32 target,
                  VkImage image, VkExtent2D extent);

private:
    struct Target {
        std::array<VkDescriptorSet, 2> descriptor_sets{};
        VkImageView view{};
    };

    struct Generation {
        std::array<Target, LSFG_MAX_TARGETS> targets{};
        VkBuffer buffer{};
    };

    LsfgImagePair* frames{};
    LsfgImage* motion{};
    LsfgImage* detail1{};
    LsfgImage* detail2{};
    VkSampler sampler{};
    VkSampler edge_sampler{};

    LsfgPass pass;
    std::array<Generation, LSFG_GENERATION_SLOTS> generations{};
    vk::DescriptorSets owned_sets;
};

} // namespace rex::ui::vulkan::framegen
