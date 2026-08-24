// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

// SPDX-FileCopyrightText: Copyright 2025 lsfg-vk
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <array>

#include "lsfg_common.hpp"

namespace lsfg {

class LsfgShaders;

class LsfgMipmaps {
public:
    LsfgMipmaps() = default;
    LsfgMipmaps(const Device& device, const LsfgShaders& shaders, LsfgResources& resources,
                VkDescriptorPool descriptor_pool, LsfgImagePair& frames, float flow_scale);

    void Dispatch(VkCommandBuffer cmdbuf, uint64_t frame_count);

    [[nodiscard]] LsfgImage& Output(size_t level) {
        return out_images[level];
    }

    [[nodiscard]] VkExtent2D FlowExtent() const {
        return flow_extent;
    }

    [[nodiscard]] bool Valid() const {
        return pass.Valid() && descriptor_sets[0] != VK_NULL_HANDLE;
    }

private:
    LsfgImagePair* frames{};

    LsfgPass pass;
    std::array<VkDescriptorSet, 2> descriptor_sets{};

    VkExtent2D flow_extent{};
    std::array<LsfgImage, LSFG_MIP_LEVELS> out_images;
};

}
