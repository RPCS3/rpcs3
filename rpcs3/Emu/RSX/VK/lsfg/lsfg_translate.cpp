// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

// ReX/PS5 adaptation: bounded native SPIR-V adoption; full validation follows before driver use.

#include <algorithm>
#include <cstring>
#include <map>
#include <tuple>

#include "lsfg_translate.h"

namespace rex::ui::vulkan::framegen {

namespace {

constexpr u32 SPIRV_MAGIC = 0x07230203;
constexpr u32 SPIRV_WORD_COUNT_SHIFT = 16;
constexpr u32 SPIRV_OPCODE_MASK = 0xffff;
constexpr u32 SPIRV_OP_FUNCTION = 54;
constexpr u32 SPIRV_OP_DECORATE = 71;
constexpr u32 SPIRV_DECORATION_BINDING = 33;
constexpr u32 SPIRV_DECORATION_DESCRIPTOR_SET = 34;

constexpr u32 DECORATION_LITERAL_WORD = 3;
constexpr size_t SPIRV_HEADER_WORDS = 5;

bool RenumberBindingsInOrder(std::vector<u32>& words) {
    struct Slot {
        u32 set;
        u32 binding;
        size_t literal_offset;
    };

    std::map<u32, u32> sets;
    std::vector<Slot> slots;

    size_t offset = SPIRV_HEADER_WORDS;
    while (offset + 1 <= words.size()) {
        const u32 length = words[offset] >> SPIRV_WORD_COUNT_SHIFT;
        const u32 opcode = words[offset] & SPIRV_OPCODE_MASK;
        if (length == 0 || offset + length > words.size()) {
            return false;
        }
        if (opcode == SPIRV_OP_FUNCTION) {
            break;
        }
        if (opcode == SPIRV_OP_DECORATE && length >= 4) {
            if (words[offset + 2] == SPIRV_DECORATION_DESCRIPTOR_SET) {
                sets[words[offset + 1]] = words[offset + 3];
            } else if (words[offset + 2] == SPIRV_DECORATION_BINDING) {
                slots.push_back(Slot{0, words[offset + 3], offset + DECORATION_LITERAL_WORD});
            }
        }
        offset += length;
    }

    for (Slot& slot : slots) {
        const auto hit = sets.find(words[slot.literal_offset - 2]);
        slot.set = hit == sets.end() ? 0 : hit->second;
    }

    std::ranges::stable_sort(slots, [](const Slot& lhs, const Slot& rhs) {
        return std::tie(lhs.set, lhs.binding) < std::tie(rhs.set, rhs.binding);
    });

    for (size_t i = 0; i < slots.size(); ++i) {
        words[slots[i].literal_offset] = static_cast<u32>(i);
    }
    return true;
}

} // Anonymous namespace

bool IsSpirvModule(std::span<const u8> blob) {
    if (blob.size() > 16u * 1024u * 1024u || blob.size() < SPIRV_HEADER_WORDS * sizeof(u32) || blob.size() % sizeof(u32) != 0) {
        return false;
    }
    u32 magic{};
    std::memcpy(&magic, blob.data(), sizeof(magic));
    return magic == SPIRV_MAGIC;
}

std::vector<u32> AdoptSpirvModule(std::span<const u8> blob) {
    if (!IsSpirvModule(blob)) {
        return {};
    }

    std::vector<u32> words(blob.size() / sizeof(u32));
    std::memcpy(words.data(), blob.data(), blob.size());

    if (!RenumberBindingsInOrder(words)) return {};
    return words;
}

} // namespace rex::ui::vulkan::framegen
