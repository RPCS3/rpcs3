// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-FileCopyrightText: Copyright 2025 lsfg-vk
// SPDX-License-Identifier: GPL-3.0-or-later
// ReX/PS5 adaptation: read-only, bounded native shader extraction; no DLL execution.
#pragma once

#include <array>
#include <map>
#include <span>
#include <string>
#include <vector>
#include "types.h"

namespace rex::ui::vulkan::framegen {
enum class LosslessStatus : u32 {
  Ok, NotInstalled, UnreadableFile, NotPortableExecutable, MissingShaders, InvalidShader, TooLarge
};
enum class ShaderVariant : u32 { NativeFp32 = 1, NativeFp16 = 2 };
using ShaderModules = std::map<u32, std::vector<u32>>;

struct ShaderLoadResult {
  LosslessStatus status = LosslessStatus::MissingShaders;
  ShaderVariant variant = ShaderVariant::NativeFp32;
  u64 source_bytes = 0;
  std::string detail;
};

namespace PerformanceShader {
constexpr u32 MIPMAPS = 255;
constexpr u32 GENERATE = 256;
constexpr std::array<u32, 4> ALPHA{290, 291, 292, 293};
constexpr std::array<u32, 5> BETA{298, 299, 300, 301, 302};
constexpr std::array<u32, 5> GAMMA{280, 282, 283, 284, 285};
constexpr std::array<u32, 10> DELTA{280, 286, 287, 288, 289, 281, 294, 295, 296, 297};
constexpr u32 NATIVE_FP16_OFFSET = 49;
constexpr u32 NATIVE_FP32_OFFSET = 98;
}  // namespace PerformanceShader

constexpr size_t kMaximumDllBytes = size_t(256) << 20;
constexpr size_t kMaximumShaderBytes = size_t(16) << 20;

// Pure parser for testing the actual PE resource traversal and native variant
// selection. This is an internal renderer check; file presence enables the UI.
ShaderLoadResult ParseShaderModules(std::span<const u8> image, ShaderModules& out_modules,
                                    bool allow_fp16, bool prefer_fp16);
ShaderLoadResult LoadShaderModules(const std::string& path, ShaderModules& out_modules,
                                   bool allow_fp16, bool prefer_fp16);
const char* LosslessStatusName(LosslessStatus status);
}  // namespace rex::ui::vulkan::framegen
