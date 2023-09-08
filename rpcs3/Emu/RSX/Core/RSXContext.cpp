#include "stdafx.h"
#include "Emu/RSX/rsx_utils.h"
#include "RSXContext.h"

namespace rsx
{
	GCM_tile_reference GCM_context::get_tiled_memory_region(const utils::address_range& range) const
	{
		if (rsx::get_location(range.start) != CELL_GCM_LOCATION_MAIN)
		{
			// Local mem can be tiled but access is transparent from the memory controller
			return {};
		}

		for (const auto& tile : tiles)
		{
			if (!tile.bound || tile.location != CELL_GCM_LOCATION_MAIN)
			{
				continue;
			}

			const auto tile_base_address = iomap_table.get_addr(tile.offset);
			const auto tile_range = utils::address_range::start_length(tile_base_address, tile.size);

			if (range.inside(tile_range))
			{
				ensure(tile_base_address + 1);
				return { .base_address = tile_base_address, .tile = &tile };
			}
		}

		return {};
	}
}
