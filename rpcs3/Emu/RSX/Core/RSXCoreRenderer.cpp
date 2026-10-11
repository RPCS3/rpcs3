#include "stdafx.h"
#include "RSXCoreRenderer.h"

#include "Emu/RSX/Capture/rsx_capture.h"
#include "Emu/RSX/RSXThread.h"
#include "Emu/RSX/rsx_methods.h"
#include "Emu/system_config.h"

#include "Emu/RSX/NV47/HW/context_accessors.define.h"

namespace rsx
{
	void core_renderer::begin()
	{
		// Save shader state now before prefetch and loading happens
		m_pre_begin_pipeline_state = (m_ctx->graphics_state.load() & rsx::pipeline_state::invalidate_pipeline_bits);

		if (m_ctx->cond_render.hw_cond_active)
		{
			if (!m_ctx->cond_render.eval_pending())
			{
				// End conditional rendering if still active
				RSX(m_ctx)->end_conditional_rendering();
			}

			// If hw cond render is enabled and evalutation is still pending, do nothing
		}
		else if (m_ctx->cond_render.eval_pending())
		{
			// Evaluate conditional rendering test or enable hw cond render until results are available
			if (RSX(m_ctx)->renderer()->config().supports_hw_conditional_render)
			{
				// In this mode, it is possible to skip the cond render while the backend is still processing data.
				// The backend guarantees that any draw calls emitted during this time will NOT generate any ROP writes
				ensure(!m_ctx->cond_render.hw_cond_active);

				// Pending evaluation, use hardware test
				RSX(m_ctx)->begin_conditional_rendering(m_ctx->cond_render.eval_sources);
			}
			else
			{
				// NOTE: eval_sources list is reversed with newest query first
				RSX(m_ctx)->zcull_ctrl->read_barrier(RSX(m_ctx), m_ctx->cond_render.eval_address, m_ctx->cond_render.eval_sources.front());
				ensure(!m_ctx->cond_render.eval_pending());
			}
		}

		if (!RSX(m_ctx)->renderer()->config().supports_normalized_barycentrics)
		{
			// Check for mode change between rasterized polys vs lines and points
			// Luckily this almost never happens in real games
			const auto current_mode = REGS(m_ctx)->current_draw_clause.classify_mode();
			if (current_mode != m_ctx->program.draw_mode)
			{
				m_ctx->graphics_state |= (rsx::vertex_program_state_dirty | rsx::fragment_program_state_dirty);
				m_ctx->program.draw_mode = current_mode;
			}
		}

		m_ctx->in_begin_end = true;

		if (m_ctx->frame_skip.skip_current_frame || m_ctx->cond_render.disable_rendering())
		{
			return;
		}

		RSX(m_ctx)->renderer()->begin_draw();
	}

	void core_renderer::end()
	{
		if (should_skip_draw() || !RSX(m_ctx)->renderer()->end_draw(m_pre_begin_pipeline_state))
		{
			execute_nop_draw();
		}

		post_draw_hook();
	}

	void core_renderer::discard_draw()
	{
		execute_nop_draw();
		post_draw_hook();
	}

	void core_renderer::post_draw_hook()
	{
		if (RSX(m_ctx)->capture_current_frame)
		{
			capture::capture_draw_memory(RSX(m_ctx));
		}

		m_ctx->in_begin_end = false;
		m_ctx->stats.draw_calls++;

		REGS(m_ctx)->current_draw_clause.post_execute_cleanup(m_ctx);

		m_ctx->graphics_state |= rsx::pipeline_state::framebuffer_reads_dirty;
		m_ctx->eng_interrupt_mask |= rsx::backend_interrupt;
		RSX(m_ctx)->ROP_sync_timestamp = rsx::get_shared_tag();

		RSX(m_ctx)->m_draw_processor.clear_push_buffers();

		RSX(m_ctx)->zcull_ctrl->on_draw();

		if (RSX(m_ctx)->capture_current_frame)
		{
			const u32 element_count = REGS(m_ctx)->current_draw_clause.get_elements_count();
			RSX(m_ctx)->capture_frame(fmt::format("Draw %s %d", REGS(m_ctx)->current_draw_clause.primitive, element_count));
		}
	}

	void core_renderer::execute_nop_draw()
	{
		auto& draw_clause = REGS(m_ctx)->current_draw_clause;
		draw_clause.begin();
		do
		{
			draw_clause.execute_pipeline_dependencies(m_ctx);
		}
		while (draw_clause.next());
	}

	bool core_renderer::should_skip_draw() const
	{
		if (m_ctx->frame_skip.skip_current_frame ||
			!m_ctx->graphics_state.test(rsx::rtt_config_valid) ||
			m_ctx->cond_render.disable_rendering())
		{
			return true;
		}

		if (m_ctx->graphics_state.test(rsx::rtt_config_no_attachments) &&
			(g_cfg.video.disable_zcull_queries || !RSX(m_ctx)->zcull_ctrl->has_active_queries()))
		{
			return true;
		}

		return false;
	}
}
