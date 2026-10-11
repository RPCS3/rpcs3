#pragma once

#include <util/types.hpp>
#include <string>
#include <utility>
#include "Emu/RSX/Common/texture_cache_types.h"
#include "Emu/RSX/Utils/rsx_utils.h"

namespace rsx
{
	struct context;

	// Host renderer interface. Translates common state into work for the host accelerator.
	// Backends are just graphics accelerator plugins. A backend never calls back up into rsx::thread.
	class render_backend
	{
	public:
		virtual ~render_backend() = default;

		// Draw
		virtual void clear_surface(u32 /*arg*/) {}
		virtual void emit_geometry(u32 /*sub_index*/) {}

		// Memory
		virtual bool on_access_violation(u32 /*address*/, bool /*is_writing*/) { return false; }
		virtual void on_invalidate_memory_range(const utils::address_range32& /*range*/, invalidation_cause /*cause*/) {}
		virtual void notify_tile_unbound(u32 /*tile*/) {}
		virtual void write_barrier(u32 /*memory_address*/, u32 /*memory_range*/) {}

		// Load an image from memory with optional scaling and rotation.
		// Returns false to tell the HW decoder to perform the operation on the CPU as a fallback when the operation cannot be safely accelerated.
		virtual bool scaled_image_from_memory(const blit_src_info& /*src_info*/, const blit_dst_info& /*dst_info*/, bool /*interpolate*/) { return false; }

		// Sync
		virtual bool release_GCM_label(u32 /*type*/, u32 /*address*/, u32 /*value*/) { return false; }
		virtual void on_semaphore_acquire_wait() {}

		// GRAPH. Patch transform constants. Units are in 32x4 units
		virtual void patch_transform_constants(context* /*ctx*/, u32 /*index*/, u32 /*count*/) {}

		// Inspection
		virtual std::pair<std::string, std::string> get_programs() const { return std::make_pair("", ""); }
		virtual bool is_current_program_interpreted() const { return false; }
	};
}
