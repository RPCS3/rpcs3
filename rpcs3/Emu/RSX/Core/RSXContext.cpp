#include "stdafx.h"
#include "Emu/RSX/rsx_utils.h"
#include "RSXContext.h"

namespace rsx
{
	GCM_tile_reference GCM_context::get_tiled_memory_region(const utils::address_range32& range) const
	{
		if (rsx::get_location(range.start) != CELL_GCM_LOCATION_MAIN)
		{
			// Local mem can be tiled but access is transparent from the memory controller
			return {};
		}

		if (!g_cfg.video.handle_tiled_memory)
		{
			// Tiled memory sections will simply be ignored if the option is disabled.
			// TODO: This option should never be needed.
			return {};
		}

		for (const auto& tile : tiles)
		{
			if (!tile.bound || tile.location != CELL_GCM_LOCATION_MAIN)
			{
				continue;
			}

			const auto tile_base_address = iomap_table.get_addr(tile.offset);
			const auto tile_range = utils::address_range32::start_length(tile_base_address, tile.size);

			if (range.inside(tile_range))
			{
				ensure(tile_base_address + 1);
				return { .base_address = tile_base_address, .tile = &tile };
			}
		}

		return {};
	}

	utils::address_range32 GCM_tile_reference::tile_align(const utils::address_range32& range) const
	{
		const auto alignment = 64 * tile->pitch;
		const u32 start_offset = rsx::align_down2(range.start - base_address, alignment);
		const u32 end_offset = rsx::align2(range.end - base_address + 1, alignment);

		return utils::address_range32::start_length(start_offset + base_address, end_offset - start_offset);
	}
}
