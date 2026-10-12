#pragma once

#include <util/types.hpp>
#include "Emu/RSX/Utils/rsx_utils.h"
#include "RSXGraphEngine.h"

namespace rsx
{
	struct context;

	// Backend-agnostic half of the renderer. Runs the firmware side of each draw and drives the host backend through hooks.
	// It is a per-context object.
	class core_renderer
	{
	public:
		void init(context* ctx)
		{
			m_ctx = ctx;
			m_graph.init(ctx);
		}

		graph_engine& graph() { return m_graph; }
		const graph_engine& graph() const { return m_graph; }

		// NV4097_SET_BEGIN_END
		void begin();
		void end();

		// Retire the current draw clause without drawing it
		void discard_draw();

		// NV4097_CLEAR_SURFACE
		void clear(rsx::flags32_t arg);

	private:
		context* m_ctx = nullptr;

		// GRAPH unit: framebuffer and program resolution
		graph_engine m_graph;

		bool should_skip_draw() const;
		void decode_draw_state();
		void update_program_state(rsx::flags32_t program_bits);
		void resolve_programs();
		void resolve_pipeline_state();
		void execute_nop_draw();
		void post_draw_hook();
	};
}
