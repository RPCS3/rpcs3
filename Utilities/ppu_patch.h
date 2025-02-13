#pragma once

#include <util/types.hpp>
#include <unordered_set>
#include <string>

// Patch utilities specific to PPU code
struct ppu_patch_block_registry_t
{
    ppu_patch_block_registry_t() = default;
    ppu_patch_block_registry_t(const ppu_patch_block_registry_t&) = delete;
    ppu_patch_block_registry_t& operator=(const ppu_patch_block_registry_t&) = delete;

    std::unordered_set<u32> block_addresses{};
};

void ppu_register_range(u32 addr, u32 size);
bool ppu_form_branch_to_code(u32 entry, u32 target, bool link = false, bool with_toc = false, std::string module_name = {});
u32 ppu_generate_id(std::string_view name);
