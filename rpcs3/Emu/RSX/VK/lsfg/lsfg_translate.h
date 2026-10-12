// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

// ReX/PS5 adaptation: bounded native SPIR-V adoption; full validation follows before driver use.

#pragma once

#include <span>
#include <vector>

#include "types.h"

namespace rex::ui::vulkan::framegen {

[[nodiscard]] bool IsSpirvModule(std::span<const u8> blob);

[[nodiscard]] std::vector<u32> AdoptSpirvModule(std::span<const u8> blob);

} // namespace rex::ui::vulkan::framegen
