// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

// SPDX-FileCopyrightText: Copyright 2025 lsfg-vk
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <array>

#include "lsfg_common.hpp"

namespace lsfg {

class LsfgShaders;

constexpr size_t LSFG_BETA_STAGES = 5;
constexpr size_t LSFG_BETA_OUTPUTS = 6;

class LsfgBeta {
public:
    LsfgBeta() = default;
    LsfgBeta(const Device& device, const LsfgShaders& shaders, LsfgResources& resources,
             VkDescriptorPool descriptor_pool, LsfgImageHistory& inputs);

    void Dispatch(VkCommandBuffer cmdbuf, uint64_t frame_count);

    [[nodiscard]] LsfgImage& Output(size_t level) {
        return out_images[level];
    }

    [[nodiscard]] bool Valid() const {
        return allocated;
    }

private:
    LsfgImageHistory* inputs{};

    std::array<LsfgPass, LSFG_BETA_STAGES> passes;
    std::array<VkDescriptorSet, LSFG_HISTORY_SLOTS> first_descriptor_sets{};
    std::array<VkDescriptorSet, LSFG_BETA_STAGES - 1> descriptor_sets{};

    LsfgImagePair temp1;
    LsfgImagePair temp2;
    std::array<LsfgImage, LSFG_BETA_OUTPUTS> out_images;
    bool allocated{};
};

}
