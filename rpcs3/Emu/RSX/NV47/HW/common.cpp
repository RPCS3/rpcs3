#include "stdafx.h"
#include "common.h"

#include "Emu/RSX/RSXThread.h"

#define RSX(ctx) ctx->rsxthr
#define REGS(ctx) (&rsx::method_registers)

namespace rsx
{
	namespace util
	{
		void push_vertex_data(rsx::context* ctx, u32 attrib_index, u32 channel_select, int count, rsx::vertex_base_type vtype, u32 value)
		{
			if (RSX(ctx)->in_begin_end)
			{
				// Update to immediate mode register/array
				// NOTE: Push buffers still behave like register writes.
				// You do not need to specify each attribute for each vertex, the register is referenced instead.
				// This is classic OpenGL 1.x behavior as I remember.
				RSX(ctx)->GRAPH_frontend().append_to_push_buffer(attrib_index, count, channel_select, vtype, value);
			}

			auto& info = REGS(ctx)->register_vertex_info[attrib_index];

			info.type = vtype;
			info.size = count;
			info.frequency = 0;
			info.stride = 0;
			REGS(ctx)->register_vertex_info[attrib_index].data[channel_select] = value;
		}

		void push_draw_parameter_change(rsx::context* ctx, rsx::command_barrier_type type, u32 reg, u32 arg0, u32 arg1, u32 index)
		{
			// NOTE: We can't test against latch here, since a previous change may be buffered in a pending barrier.
			if (!RSX(ctx)->in_begin_end || REGS(ctx)->current_draw_clause.empty())
			{
				return;
			}

			// Defer the change. Rollback...
			REGS(ctx)->decode(reg, REGS(ctx)->latch);

			// Insert barrier to reinsert the value later
			REGS(ctx)->current_draw_clause.insert_command_barrier(type, arg0, arg1, index);
		}

		u32 get_report_data_impl([[maybe_unused]] rsx::context* ctx, u32 offset)
		{
			u32 location = 0;
			blit_engine::context_dma report_dma = REGS(ctx)->context_dma_report();

			switch (report_dma)
			{
			case blit_engine::context_dma::to_memory_get_report: location = CELL_GCM_CONTEXT_DMA_REPORT_LOCATION_LOCAL; break;
			case blit_engine::context_dma::report_location_main: location = CELL_GCM_CONTEXT_DMA_REPORT_LOCATION_MAIN; break;
			case blit_engine::context_dma::memory_host_buffer: location = CELL_GCM_CONTEXT_DMA_MEMORY_HOST_BUFFER; break;
			default:
				return vm::addr_t(0);
			}

			return vm::cast(get_address(offset, location));
		}

		void set_fragment_texture_dirty_bit(rsx::context* ctx, u32 arg, u32 index)
		{
			if (REGS(ctx)->latch == arg)
			{
				return;
			}

			RSX(ctx)->m_textures_dirty[index] = true;

			if (RSX(ctx)->current_fp_metadata.referenced_textures_mask & (1 << index))
			{
				RSX(ctx)->m_graphics_state |= rsx::pipeline_state::fragment_program_state_dirty;
			}
		}

		void set_texture_configuration_command(rsx::context* ctx, u32 reg)
		{
			const u32 reg_index = reg - NV4097_SET_TEXTURE_OFFSET;
			ensure(reg_index % 8 == 0 && reg_index < 8 * 16); // Only NV4097_SET_TEXTURE_OFFSET is expected

			const u32 texture_index = reg_index / 8;

			// FIFO args count including this one
			const u32 fifo_args_cnt = RSX(ctx)->fifo_ctrl->get_remaining_args_count() + 1;

			// The range of methods this function resposible to
			constexpr u32 method_range = 8;

			// Get limit imposed by FIFO PUT (if put is behind get it will result in a number ignored by min)
			const u32 fifo_read_limit = static_cast<u32>(((RSX(ctx)->ctrl->put & ~3ull) - (RSX(ctx)->fifo_ctrl->get_pos())) / 4);

			const u32 count = std::min<u32>({ fifo_args_cnt, fifo_read_limit, method_range });

			// Clamp by the count of methods this function is responsible to
			std::span<const u32> command_span = RSX(ctx)->fifo_ctrl->get_current_arg_ptr(count);
			ensure(!command_span.empty() && command_span.size() <= count);

			u32* const dst_regs = &REGS(ctx)->registers[reg];
			bool set_dirty = (dst_regs[0] != REGS(ctx)->latch);

			for (usz i = 1; i < command_span.size(); i++)
			{
				const u32 command_data = std::bit_cast<be_t<u32>>(command_span[i]);

				set_dirty = set_dirty || (command_data != dst_regs[i]);
				dst_regs[i] = command_data;
			}

			// Skip handled methods
			RSX(ctx)->fifo_ctrl->skip_methods(static_cast<u32>(command_span.size()) - 1);

			if (set_dirty)
			{
				RSX(ctx)->m_textures_dirty[texture_index] = true;

				if (RSX(ctx)->current_fp_metadata.referenced_textures_mask & (1 << texture_index))
				{
					RSX(ctx)->m_graphics_state |= rsx::pipeline_state::fragment_program_state_dirty;
				}
			}
		}

		void set_vertex_texture_dirty_bit(rsx::context* ctx, u32 index)
		{
			RSX(ctx)->m_vertex_textures_dirty[index] = true;

			if (RSX(ctx)->current_vp_metadata.referenced_textures_mask & (1 << index))
			{
				RSX(ctx)->m_graphics_state |= rsx::pipeline_state::vertex_program_state_dirty;
			}
		}
	}
}
