#pragma once

#include <util/types.hpp>
#include "../gcm_enums.h"
#include "../GCM.h"
#include "../Utils/rsx_utils.h"

namespace rsx
{
	enum framebuffer_creation_context : u8
	{
		context_draw = 0,
		context_clear_color = 1,
		context_clear_depth = 2,
		context_clear_all = context_clear_color | context_clear_depth
	};

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

	// Per-context framebuffer configuration as decoded from the surface registers
	struct framebuffer_state
	{
		gcm_framebuffer_info surface_info[limits::color_buffers_count];
		gcm_framebuffer_info depth_surface_info;
		framebuffer_layout layout{};
		framebuffer_creation_context current_context = context_draw;
		bool zcull_surface_active = false;
	};
}
