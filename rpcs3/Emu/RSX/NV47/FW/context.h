#pragma once

#include <util/types.hpp>
#include "Emu/Cell/lv2/sys_rsx.h"
#include "Emu/RSX/GCM.h"
#include "Emu/RSX/Utils/rsx_utils.h"
#include "RSXIOMap.hpp"

namespace rsx
{
	struct GCM_tile_reference
	{
		u32 base_address = 0;
		const GcmTileInfo* tile = nullptr;

		operator bool() const
		{
			return !!tile;
		}

		utils::address_range32 tile_align(const utils::address_range32& range) const;
	};

	// Firmware and low-level driver state: DMA control, IO mapping, tiling/zcull regions and device info.
	struct fw_device_state
	{
		RsxDmaControl* ctrl = nullptr;
		u32 dma_address{ 0 };
		rsx_iomap_table iomap_table;

		GcmTileInfo tiles[limits::tiles_count];
		GcmZcullInfo zculls[limits::zculls_count];

		shared_mutex sys_rsx_mtx;
		u32 device_addr{ 0 };
		u32 local_mem_size{ 0 };

		GCM_tile_reference get_tiled_memory_region(const utils::address_range32& range) const;
	};

	// Per-context lv2 driver state, set up by sys_rsx
	struct fw_context
	{
		u32 label_addr{ 0 };
		u32 main_mem_size{ 0 };
		u32 rsx_event_port{ 0 };
		u32 driver_info{ 0 };

		// Unsent event bits when aborting RSX/VBLANK thread (will be sent on savestate load)
		atomic_t<u64> unsent_gcm_events = 0;

		// GET/PUT update latch for lv2 update requests
		atomic_t<u64> new_get_put = u64{ umax };

		// sys_rsx_context_free requested this context be removed
		atomic_t<bool> free_requested{ false };
	};
}
