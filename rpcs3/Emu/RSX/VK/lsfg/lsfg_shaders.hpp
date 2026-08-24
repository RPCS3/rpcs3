// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstdint>
#include <map>
#include <string>
#include "../VulkanAPI.h"

namespace lsfg {

class Device;

class LsfgShaders {
public:
    LsfgShaders() = default;
    LsfgShaders(const Device& device, const std::string& cache_path);
    ~LsfgShaders();

    LsfgShaders(const LsfgShaders&) = delete;
    LsfgShaders& operator=(const LsfgShaders&) = delete;

    [[nodiscard]] bool IsValid() const {
        return valid;
    }

    [[nodiscard]] VkShaderModule Get(uint32_t shader_id) const;

private:
    void Release();

    VkDevice device{VK_NULL_HANDLE};
    std::map<uint32_t, VkShaderModule> modules;
    bool valid{};
};

}
