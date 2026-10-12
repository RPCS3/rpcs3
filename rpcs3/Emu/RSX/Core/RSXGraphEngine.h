#pragma once

#include <util/types.hpp>
#include "Emu/RSX/Core/RSXFrameBuffer.h"
#include "Emu/RSX/Utils/rsx_utils.h"
#include "Utilities/geometry.h"

namespace rsx
{
	struct context;

	// The GRAPH functional unit. It is a per-context component of the renderer core.
	class graph_engine
	{
	public:
		void init(context* ctx) { m_ctx = ctx; }

		std::array<u32, 4> get_color_surface_addresses() const;
		u32 get_zeta_surface_address() const;

		void get_framebuffer_layout(rsx::framebuffer_creation_context context, framebuffer_layout& layout);
		bool get_scissor(areau& region, bool clip_viewport, const surface_scaling_config_t& resolution_scaling_config);

		// Re-evaluates the framebuffer write state after a surface option register write
		void on_framebuffer_options_changed(u32 opt);

		// Notify framebuffer layout has been committed.
		// FIXME: This should not be here
		void on_framebuffer_layout_updated();

	private:
		context* m_ctx = nullptr;
	};
}
