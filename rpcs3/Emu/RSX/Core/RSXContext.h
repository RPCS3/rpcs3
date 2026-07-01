#pragma once

#include <util/types.hpp>
#include "Emu/Cell/lv2/sys_rsx.h"
#include "Emu/RSX/GCM.h"
#include "Emu/RSX/rsx_utils.h"
#include "RSXIOMap.hpp"

namespace rsx
{
	namespace gcm
	{
		enum limits
		{
			tiles_count = 15,
			zculls_count = 8
		};
	}

	struct GCM_tile_reference
	{
		u32 base_address = 0;
		const GcmTileInfo* tile = nullptr;

		operator bool() const
		{
			return !!tile;
		}

		utils::address_range32 tile_align(const rsx::address_range32& range) const;
	};

	using GCM_context = struct ::lv2_rsx_context; 
}
