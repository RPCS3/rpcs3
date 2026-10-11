#pragma once

#include <util/types.hpp>
#include "Emu/RSX/Utils/rsx_utils.h"

namespace rsx
{
	struct context;

	// Backend-agnostic half of the renderer. Runs the firmware side of each draw and drives the host backend through hooks.
	// It is a per-context object.
	class core_renderer
	{
	public:
		void init(context* ctx) { m_ctx = ctx; }

		// NV4097_SET_BEGIN_END
		void begin();
		void end();

		// Retire the current draw clause without drawing it
		void discard_draw();

	private:
		context* m_ctx = nullptr;

		// invalidate_pipeline_bits as they were before the current draw began
		rsx::flags32_t m_pre_begin_pipeline_state = 0;

		bool should_skip_draw() const;
		void execute_nop_draw();
		void post_draw_hook();
	};
}
