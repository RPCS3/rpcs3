// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include "stdafx.h"
#include "lsfg_shaders.hpp"
#include "lsfg_common.hpp"
#include "lsfg_dll.h"

namespace lsfg {

LsfgShaders::LsfgShaders(const Device& device_, const std::string& cache_path)
    : device{device_.Handle()} {
    LsfgModuleSet set{};
    const LsfgStatus status = lsfg_load_modules(cache_path.c_str(), &set);
    if (status != LSFG_OK) {
        rsx_log.error("Frame generation: shader cache unusable (status %d)", static_cast<int>(status));
        return;
    }

    for (uint32_t i = 0; i < set.count; i++) {
        const LsfgModule& module = set.modules[i];

        VkShaderModuleCreateInfo module_ci{};
        module_ci.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        module_ci.codeSize = static_cast<size_t>(module.word_count) * sizeof(uint32_t);
        module_ci.pCode = module.words;

        VkShaderModule handle = VK_NULL_HANDLE;
        if (VK_GET_SYMBOL(vkCreateShaderModule)(device, &module_ci, nullptr, &handle) != VK_SUCCESS) {
            rsx_log.error("Frame generation: vkCreateShaderModule failed for shader %u", module.id);
            lsfg_release_modules(&set);
            Release();
            return;
        }
        modules.emplace(module.id, handle);
    }

    const LsfgVariant variant = set.variant;
    lsfg_release_modules(&set);
    valid = modules.size() == LSFG_SHADER_COUNT;
    if (valid) {
        rsx_log.notice("Frame generation: created %zu shader modules, variant=%s", modules.size(),
                       lsfg_variant_name(variant));
    } else {
        rsx_log.error("Frame generation: expected %u shader modules, got %zu", LSFG_SHADER_COUNT, modules.size());
        Release();
    }
}

LsfgShaders::~LsfgShaders() {
    Release();
}

void LsfgShaders::Release() {
    if (device != VK_NULL_HANDLE) {
        for (auto& [id, module] : modules) {
            VK_GET_SYMBOL(vkDestroyShaderModule)(device, module, nullptr);
        }
    }
    modules.clear();
    valid = false;
}

VkShaderModule LsfgShaders::Get(uint32_t shader_id) const {
    const auto it = modules.find(shader_id);
    return it == modules.end() ? VK_NULL_HANDLE : it->second;
}

}
