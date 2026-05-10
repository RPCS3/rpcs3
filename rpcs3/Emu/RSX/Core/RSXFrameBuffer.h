#pragma once

#include <util/types.hpp>
#include "../gcm_enums.h"
#include "../GCM.h"

namespace rsx
{
	struct tiled_region
	{
		u32 address;
		u32 base;
		GcmTileInfo* tile;
		u8* ptr;

		void write(const void* src, u32 width, u32 height, u32 pitch);
		void read(void* dst, u32 width, u32 height, u32 pitch);
	};

	struct framebuffer_layout
	{
		ENABLE_BITWISE_SERIALIZATION;

		u16 width;
		u16 height;
		std::array<u32, 4> color_addresses;
		std::array<u32, 4> color_pitch;
		std::array<u32, 4> actual_color_pitch;
		std::array<bool, 4> color_write_enabled;
		u32 zeta_address;
		u32 zeta_pitch;
		u32 actual_zeta_pitch;
		bool zeta_write_enabled;
		rsx::surface_target target;
		rsx::surface_color_format color_format;
		rsx::surface_depth_format2 depth_format;
		rsx::surface_antialiasing aa_mode;
		rsx::surface_raster_type raster_type;
		u32 aa_factors[2];
		bool ignore_change;
	};
}
