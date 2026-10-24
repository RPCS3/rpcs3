#include "stdafx.h"
#include "GRAPH_backend.h"
#include "Emu/RSX/RSXThread.h"

#include "Emu/RSX/NV47/HW/context_accessors.define.h"

namespace rsx
{
	static bool is_scissor_valid(const context* ctx)
	{
		auto scissor_x = REGS(ctx)->scissor_origin_x();
		auto scissor_y = REGS(ctx)->scissor_origin_y();
		auto scissor_w = REGS(ctx)->scissor_width();
		auto scissor_h = REGS(ctx)->scissor_height();

		if (!scissor_w || !scissor_h)
		{
			// Empty scissor, all fragments fail.
			return false;
		}

		// Load viewport
		const u16 raster_x = REGS(ctx)->viewport_origin_x();
		const u16 raster_w = REGS(ctx)->viewport_width();
		const u16 raster_y = REGS(ctx)->viewport_origin_y();
		const u16 raster_h = REGS(ctx)->viewport_height();

		if (!raster_w || !raster_h)
		{
			return false;
		}

		// Clip scissor against viewport
		const auto x1 = std::max(scissor_x, raster_x);
		const auto y1 = std::max(scissor_y, raster_y);
		const auto x2 = std::min(scissor_x + scissor_w, raster_x + raster_w);
		const auto y2 = std::min(scissor_y + scissor_h, raster_y + raster_h);

		if (x2 <= x1 ||
			y2 <= y1 ||
			x1 >= REGS(ctx)->window_clip_horizontal() ||
			y1 >= REGS(ctx)->window_clip_vertical())
		{
			// Out of bounds scissor. All fragments fail.
			return false;
		}

		return true;
	}

	void GRAPH_backend::evaluate_zcount_on_null_draw(context* ctx)
	{
		// This is a generic solution to the problem. It is not precise though.
		// Backends should extend this method to render to an offscreen renderbuffer.

		// First, check if we pass the scissor test
		if (!is_scissor_valid(ctx))
		{
			return;
		}

		// If the scissor is valid, proceed with tagging the queries
		auto zcull_ctrl = RSX(ctx)->get_zcull_ctrl();
		auto query = ensure(zcull_ctrl->get_current_query_task());

		const auto scale = RSX(ctx)->resolution_scaling_config.scale_percent;
		query->result += (16u * scale * scale) / 10000u;
	}
}
