#include "stdafx.h"
#include "common.h"

#include "Emu/RSX/Common/TextureUtils.h"
#include "Emu/RSX/RSXThread.h"

#define RSX(ctx) ctx->rsxthr
#define REGS(ctx) (&rsx::method_registers)

namespace rsx
{
	namespace util
	{
		bool is_volatile_TIU(rsx::context* ctx, u32 index)
		{
			if (!RSX(ctx)->fs_sampler_state[index])
			{
				return false;
			}

			return RSX(ctx)->fs_sampler_state[index]->upload_context != rsx::texture_upload_context::shader_read;
		}

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
			if (REGS(ctx)->latch == arg && !is_volatile_TIU(ctx, index))
			{
				return;
			}

			RSX(ctx)->m_textures_dirty[index] = true;

			if (RSX(ctx)->current_fp_metadata.referenced_textures_mask & (1 << index))
			{
				RSX(ctx)->m_graphics_state |= rsx::pipeline_state::fragment_program_state_dirty;
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
