// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later
// ReX/PS5 adaptation: validate native code and device/layout contracts before Vulkan consumes it.
#pragma once

#include <array>
#include <map>
#include <string>
#include <vector>
#include "lossless_dll.h"
#include "vulkan_adapter.h"

namespace rex::ui::vulkan::framegen {
struct ShaderCapabilities {
  u32 api_version = VK_API_VERSION_1_2;
  bool memory_model = false;
  bool device_scope = false;
  bool float16 = false;
  bool extended_storage_formats = false;
  bool storage_read_without_format = false;
  bool storage_write_without_format = false;
  VkSubgroupFeatureFlags subgroup_operations = 0;
  VkPhysicalDeviceLimits limits{};
  VkPhysicalDeviceFloatControlsProperties float_controls{
      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FLOAT_CONTROLS_PROPERTIES};
};

struct ShaderInterface {
  std::vector<VkDescriptorType> bindings;
  std::array<u32, 3> local_size{};
};

// Actual SPIRV-Tools Vulkan validation, then enabled-feature and LSFG interface
// checks. Kept independent of VkDevice calls for adversarial host fixtures.
bool ValidateShaderModule(std::span<const u32> words, const ShaderCapabilities& capabilities,
                           ShaderInterface& interface, std::string& error);

class LsfgShaders {
 public:
  LsfgShaders(const Device& device, const std::string& path);
  [[nodiscard]] VkShaderModule Get(u32 shader_id) const;
  void ValidateInterface(u32 shader_id, std::span<const VkDescriptorType> expected) const;
  [[nodiscard]] ShaderVariant Variant() const { return variant_; }
  [[nodiscard]] u64 SourceBytes() const { return source_bytes_; }

 private:
  std::map<u32, vk::ShaderModule> modules_;
  std::map<u32, ShaderInterface> interfaces_;
  ShaderVariant variant_ = ShaderVariant::NativeFp32;
  u64 source_bytes_ = 0;
};
}  // namespace rex::ui::vulkan::framegen
