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

constexpr size_t LSFG_DELTA_STAGES = 10;
constexpr size_t LSFG_DELTA_TEMPS = 3;

class LsfgDelta {
public:
    LsfgDelta() = default;
    LsfgDelta(const Device& device, MemoryAllocator& memory_allocator, const LsfgShaders& shaders,
              LsfgResources& resources, vk::DescriptorPool& descriptor_pool,
              LsfgImageHistory& inputs, LsfgImage& flow_input, LsfgImage* previous_gamma,
              LsfgImage* previous1, LsfgImage* previous2);

    void Dispatch(vk::CommandBuffer cmdbuf, u64 frame_count, size_t slot);

    [[nodiscard]] LsfgImage& Output1() {
        return out_image1;
    }

    [[nodiscard]] LsfgImage& Output2() {
        return out_image2;
    }

private:
    struct Generation {
        std::array<VkDescriptorSet, LSFG_HISTORY_SLOTS> first_descriptor_sets{};
        std::array<VkDescriptorSet, LSFG_HISTORY_SLOTS> sixth_descriptor_sets{};
        std::array<VkDescriptorSet, LSFG_DELTA_STAGES - 2> descriptor_sets{};
    };

    LsfgImageHistory* inputs{};
    LsfgImage* flow_input{};
    LsfgImage* previous_gamma{};
    LsfgImage* previous1{};
    LsfgImage* previous2{};

    std::array<LsfgPass, LSFG_DELTA_STAGES> passes;
    std::array<Generation, LSFG_GENERATION_SLOTS> generations{};
    vk::DescriptorSets owned_sets;

    std::array<LsfgImage, LSFG_DELTA_TEMPS> temp1;
    LsfgImagePair temp2;
    LsfgImage out_image1;
    LsfgImage out_image2;
};

} // namespace rex::ui::vulkan::framegen
