// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

// SPDX-FileCopyrightText: Copyright 2025 lsfg-vk
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <array>

#include "lsfg_common.hpp"

namespace lsfg {

class LsfgShaders;

constexpr size_t LSFG_ALPHA_STAGES = 4;

class LsfgAlphaPasses {
public:
    LsfgAlphaPasses() = default;
    LsfgAlphaPasses(const Device& device, const LsfgShaders& shaders);

    [[nodiscard]] const LsfgPass& Get(size_t stage) const {
        return passes[stage];
    }

    [[nodiscard]] bool Valid() const;

private:
    std::array<LsfgPass, LSFG_ALPHA_STAGES> passes;
};

class LsfgAlpha {
public:
    LsfgAlpha() = default;
    LsfgAlpha(const Device& device, const LsfgAlphaPasses& passes, LsfgResources& resources,
              VkDescriptorPool descriptor_pool, LsfgImage& input);

    void PushBarriers(LsfgBarriers& barriers, uint64_t frame_count, size_t stage);
    void DispatchStage(VkCommandBuffer cmdbuf, uint64_t frame_count, size_t stage);

    [[nodiscard]] LsfgImageHistory& Outputs() {
        return out_images;
    }

    [[nodiscard]] bool Valid() const {
        return allocated;
    }

private:
    const LsfgAlphaPasses* passes{};
    LsfgImage* input{};

    std::array<VkDescriptorSet, LSFG_ALPHA_STAGES - 1> descriptor_sets{};
    std::array<VkDescriptorSet, LSFG_HISTORY_SLOTS> last_descriptor_sets{};

    LsfgImage temp1;
    LsfgImage temp2;
    LsfgImagePair temp3;
    LsfgImageHistory out_images;
    bool allocated{};
};

}
