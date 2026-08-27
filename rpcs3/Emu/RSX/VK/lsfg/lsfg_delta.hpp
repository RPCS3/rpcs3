// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

// SPDX-FileCopyrightText: Copyright 2025 lsfg-vk
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <array>

#include "lsfg_common.hpp"

namespace lsfg {

class LsfgShaders;

constexpr size_t LSFG_DELTA_STAGES = 10;
constexpr size_t LSFG_DELTA_TEMPS = 3;

class LsfgDelta {
public:
    LsfgDelta() = default;
    LsfgDelta(const Device& device, const LsfgShaders& shaders, LsfgResources& resources,
              VkDescriptorPool descriptor_pool, LsfgImageHistory& inputs, LsfgImage& flow_input,
              LsfgImage* previous_gamma, LsfgImage* previous1, LsfgImage* previous2);

    void Dispatch(VkCommandBuffer cmdbuf, uint64_t frame_count, size_t slot);

    void PushStepBarriers(LsfgBarriers& barriers, uint64_t frame_count, size_t step);

    void DispatchStep(VkCommandBuffer cmdbuf, uint64_t frame_count, size_t slot,
                      size_t step);

    [[nodiscard]] LsfgImage& Output1() {
        return out_image1;
    }

    [[nodiscard]] LsfgImage& Output2() {
        return out_image2;
    }

    [[nodiscard]] bool Valid() const {
        return allocated;
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

    std::array<LsfgImage, LSFG_DELTA_TEMPS> temp1;
    LsfgImagePair temp2;
    LsfgImage out_image1;
    LsfgImage out_image2;
    bool allocated{};
};

}
