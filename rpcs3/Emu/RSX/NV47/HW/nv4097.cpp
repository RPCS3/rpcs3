#include "stdafx.h"
#include "nv4097.h"
#include "nv47_sync.hpp"

#include "Emu/RSX/RSXThread.h"
#include "Emu/RSX/Common/BufferUtils.h"

#define RSX(ctx) ctx->rsxthr
#define REGS(ctx) (&rsx::method_registers)
#define RSX_CAPTURE_EVENT(name) if (RSX(ctx)->capture_current_frame) { RSX(ctx)->capture_frame(name); }

namespace rsx
{
	namespace nv4097
	{
		///// Program management

		void set_shader_program_dirty(context* ctx, u32, u32)
		{
			RSX(ctx)->m_graphics_state |= rsx::pipeline_state::fragment_program_ucode_dirty;
		}

		void set_transform_constant::decode_one([[maybe_unused]] context* ctx, u32 reg, u32 arg)
		{
			const u32 index = reg - NV4097_SET_TRANSFORM_CONSTANT;
			const u32 constant_id = index / 4;
			const u8 subreg = index % 4;
			const u32 load = REGS(ctx)->transform_constant_load();

			REGS(ctx)->transform_constants[load + constant_id][subreg] = arg;
		}

		void set_transform_constant::batch_decode(context* ctx, u32 reg, const std::span<const u32>& args, const std::function<bool(context*, u32, u32)>& notify)
		{
			const u32 index = reg - NV4097_SET_TRANSFORM_CONSTANT;
			const u32 constant_id = index / 4;
			const u8 subreg = index % 4;
			const u32 load = REGS(ctx)->transform_constant_load();

			auto dst = &REGS(ctx)->transform_constants[load + constant_id][subreg];
			copy_data_swap_u32(dst, args.data(), ::size32(args));

			// Notify
			const u32 last_constant_id = ((reg + ::size32(args) + 3) - NV4097_SET_TRANSFORM_CONSTANT) / 4; // Aligned div
			const u32 load_index = load + constant_id;
			const u32 load_count = last_constant_id - constant_id;

			if (!notify || !notify(ctx, load_index, load_count))
			{
				RSX(ctx)->patch_transform_constants(ctx, load_index, load_count);
			}
		}

		void set_transform_constant::impl(context* ctx, u32 reg, [[maybe_unused]] u32 arg)
		{
			const u32 index = reg - NV4097_SET_TRANSFORM_CONSTANT;
			const u32 constant_id = index / 4;
			const u8 subreg = index % 4;

			// FIFO args count including this one
			const u32 fifo_args_cnt = RSX(ctx)->fifo_ctrl->get_remaining_args_count() + 1;

			// The range of methods this function resposible to
			const u32 method_range = 32 - index;

			// Get limit imposed by FIFO PUT (if put is behind get it will result in a number ignored by min)
			const u32 fifo_read_limit = static_cast<u32>(((RSX(ctx)->ctrl->put & ~3ull) - (RSX(ctx)->fifo_ctrl->get_pos())) / 4);

			const u32 count = std::min<u32>({ fifo_args_cnt, fifo_read_limit, method_range });

			const u32 load = REGS(ctx)->transform_constant_load();

			u32 rcount = count;
			if (const u32 max = (load + constant_id) * 4 + count + subreg, limit = 468 * 4; max > limit)
			{
				// Ignore addresses outside the usable [0, 467] range
				rsx_log.warning("Invalid transform register index (load=%u, index=%u, count=%u)", load, index, count);

				if ((max - count) < limit)
					rcount -= max - limit;
				else
					rcount = 0;
			}

			if (rcount == 0)
			{
				// Out-of-bounds write is a NOP
				rsx_log.trace("Out of bounds write for transform constant block.");
				RSX(ctx)->fifo_ctrl->skip_methods(fifo_args_cnt - 1);
				return;
			}

			if (RSX(ctx)->in_begin_end && !REGS(ctx)->current_draw_clause.empty())
			{
				// Updating constants mid-draw is messy. Defer the writes
				REGS(ctx)->current_draw_clause.insert_command_barrier(
					rsx::transform_constant_update_barrier,
					RSX(ctx)->fifo_ctrl->get_pos(),
					rcount,
					reg - NV4097_SET_TRANSFORM_CONSTANT
				);

				RSX(ctx)->fifo_ctrl->skip_methods(rcount - 1);
				return;
			}

			const auto values = &REGS(ctx)->transform_constants[load + constant_id][subreg];

			const auto fifo_span = RSX(ctx)->fifo_ctrl->get_current_arg_ptr(rcount);

			if (fifo_span.size() < rcount)
			{
				rcount = ::size32(fifo_span);
			}

			if (RSX(ctx)->m_graphics_state & rsx::pipeline_state::transform_constants_dirty)
			{
				// Minor optimization: don't compare values if we already know we need invalidation
				copy_data_swap_u32(values, fifo_span.data(), rcount);
			}
			else
			{
				if (copy_data_swap_u32_cmp(values, fifo_span.data(), rcount))
				{
					// Transform constants invalidation is expensive (~8k bytes per update)
					RSX(ctx)->m_graphics_state |= rsx::pipeline_state::transform_constants_dirty;
				}
			}

			RSX(ctx)->fifo_ctrl->skip_methods(rcount - 1);
		}

		void set_transform_program::impl(context* ctx, u32 reg, u32 /*arg*/)
		{
			const u32 index = reg - NV4097_SET_TRANSFORM_PROGRAM;

			// FIFO args count including this one
			const u32 fifo_args_cnt = RSX(ctx)->fifo_ctrl->get_remaining_args_count() + 1;

			// The range of methods this function resposible to
			const u32 method_range = 32 - index;

			// Get limit imposed by FIFO PUT (if put is behind get it will result in a number ignored by min)
			const u32 fifo_read_limit = static_cast<u32>(((RSX(ctx)->ctrl->put & ~3ull) - (RSX(ctx)->fifo_ctrl->get_pos())) / 4);

			const u32 count = std::min<u32>({ fifo_args_cnt, fifo_read_limit, method_range });

			const u32 load_pos = REGS(ctx)->transform_program_load();

			u32 rcount = count;

			if (const u32 max = load_pos * 4 + rcount + (index % 4);
				max > max_vertex_program_instructions * 4)
			{
				rsx_log.warning("Program buffer overflow! Attempted to write %u VP instructions.", max / 4);
				rcount -= max - (max_vertex_program_instructions * 4);
			}

			if (!rcount)
			{
				// Out-of-bounds write is a NOP
				rsx_log.trace("Out of bounds write for transform program block.");
				RSX(ctx)->fifo_ctrl->skip_methods(fifo_args_cnt - 1);
				return;
			}

			const auto fifo_span = RSX(ctx)->fifo_ctrl->get_current_arg_ptr(rcount);

			if (fifo_span.size() < rcount)
			{
				rcount = ::size32(fifo_span);
			}

			const auto out_ptr = &REGS(ctx)->transform_program[load_pos * 4 + index % 4];

			pipeline_state to_set_dirty = rsx::pipeline_state::vertex_program_ucode_dirty;

			if (rcount >= 4 && !RSX(ctx)->m_graphics_state.test(rsx::pipeline_state::vertex_program_ucode_dirty))
			{
				// Assume clean
				to_set_dirty = {};

				const usz first_index_off = 0;
				const usz second_index_off = (((rcount / 4) - 1) / 2) * 4;

				const u64 src_op1_2 = read_from_ptr<be_t<u64>>(fifo_span.data() + first_index_off);
				const u64 src_op2_2 = read_from_ptr<be_t<u64>>(fifo_span.data() + second_index_off);

				// Fast comparison
				if (src_op1_2 != read_from_ptr<u64>(out_ptr + first_index_off) || src_op2_2 != read_from_ptr<u64>(out_ptr + second_index_off))
				{
					to_set_dirty = rsx::pipeline_state::vertex_program_ucode_dirty;
				}
			}

			if (to_set_dirty)
			{
				copy_data_swap_u32(out_ptr, fifo_span.data(), rcount);
			}
			else if (copy_data_swap_u32_cmp(out_ptr, fifo_span.data(), rcount))
			{
				to_set_dirty = rsx::pipeline_state::vertex_program_ucode_dirty;
			}

			RSX(ctx)->m_graphics_state |= to_set_dirty;
			REGS(ctx)->transform_program_load_set(load_pos + ((rcount + index % 4) / 4));
			RSX(ctx)->fifo_ctrl->skip_methods(rcount - 1);
		}

		///// Texture management

		///// Surface management

		void set_surface_dirty_bit(context* ctx, u32 reg, u32 arg)
		{
			if (arg == REGS(ctx)->latch)
			{
				return;
			}

			switch (reg)
			{
			case NV4097_SET_SURFACE_COLOR_TARGET:
				RSX(ctx)->m_graphics_state |= rsx::pipeline_state::pipeline_config_dirty;
				break;
			case NV4097_SET_SURFACE_CLIP_VERTICAL:
			case NV4097_SET_SURFACE_CLIP_HORIZONTAL:
				RSX(ctx)->m_graphics_state |= rsx::pipeline_state::vertex_state_dirty;
				break;
			default:
				break;
			}

			RSX(ctx)->m_graphics_state.set(rtt_config_dirty);
			RSX(ctx)->m_graphics_state.clear(rtt_config_contested);
		}

		void set_surface_format(context* ctx, u32 reg, u32 arg)
		{
			// The high bits of this register are just log2(dimension), ignore them
			if ((arg & 0xFFFF) == (REGS(ctx)->latch & 0xFFFF))
			{
				return;
			}

			// The important parameters have changed (format, type, antialias)
			RSX(ctx)->m_graphics_state |= rsx::pipeline_state::pipeline_config_dirty;

			// Check if we need to also update fragment state
			const auto current = REGS(ctx)->decode<NV4097_SET_SURFACE_FORMAT>(arg);
			const auto previous = REGS(ctx)->decode<NV4097_SET_SURFACE_FORMAT>(REGS(ctx)->latch);

			if (*current.antialias() != *previous.antialias() ||                         // Antialias control has changed, update ROP parameters
				current.is_integer_color_format() != previous.is_integer_color_format()) // The type of color format also requires ROP control update
			{
				RSX(ctx)->m_graphics_state |= rsx::pipeline_state::fragment_state_dirty;
			}

			set_surface_dirty_bit(ctx, reg, arg);
		}

		void set_surface_options_dirty_bit(context* ctx, u32 reg, u32 arg)
		{
			if (arg != REGS(ctx)->latch)
			{
				RSX(ctx)->on_framebuffer_options_changed(reg);
				RSX(ctx)->m_graphics_state |= rsx::pipeline_config_dirty;
			}
		}

		void set_color_mask(context* ctx, u32 reg, u32 arg)
		{
			if (arg == REGS(ctx)->latch)
			{
				return;
			}

			if (REGS(ctx)->decode<NV4097_SET_COLOR_MASK>(arg).is_invalid()) [[ unlikely ]]
			{
				// Rollback
				REGS(ctx)->decode(reg, REGS(ctx)->latch);
				return;
			}

			set_surface_options_dirty_bit(ctx, reg, arg);
		}

		void set_stencil_op(context* ctx, u32 reg, u32 arg)
		{
			if (arg == REGS(ctx)->latch)
			{
				return;
			}

			if (to_stencil_op(arg)) [[ likely ]]
			{
				set_surface_options_dirty_bit(ctx, reg, arg);
				return;
			}

			// Rollback
			REGS(ctx)->decode(reg, REGS(ctx)->latch);
		}

		///// Draw call setup (vertex, etc)

		void set_array_element16(context* ctx, u32, u32 arg)
		{
			if (RSX(ctx)->in_begin_end)
			{
				RSX(ctx)->GRAPH_frontend().append_array_element(arg & 0xFFFF);
				RSX(ctx)->GRAPH_frontend().append_array_element(arg >> 16);
			}
		}

		void set_array_element32(context* ctx, u32, u32 arg)
		{
			if (RSX(ctx)->in_begin_end)
				RSX(ctx)->GRAPH_frontend().append_array_element(arg);
		}

		void draw_arrays(context* /*rsx*/, u32 /*reg*/, u32 arg)
		{
			REGS(ctx)->current_draw_clause.command = rsx::draw_command::array;
			rsx::registers_decoder<NV4097_DRAW_ARRAYS>::decoded_type v(arg);

			REGS(ctx)->current_draw_clause.append(v.start(), v.count());
		}

		void draw_index_array(context* /*rsx*/, u32 /*reg*/, u32 arg)
		{
			REGS(ctx)->current_draw_clause.command = rsx::draw_command::indexed;
			rsx::registers_decoder<NV4097_DRAW_INDEX_ARRAY>::decoded_type v(arg);

			REGS(ctx)->current_draw_clause.append(v.start(), v.count());
		}

		void draw_inline_array(context* /*rsx*/, u32 /*reg*/, u32 arg)
		{
			arg = std::bit_cast<u32, be_t<u32>>(arg);
			REGS(ctx)->current_draw_clause.command = rsx::draw_command::inlined_array;
			REGS(ctx)->current_draw_clause.inline_vertex_array.push_back(arg);
		}

		void set_transform_program_start(context* ctx, u32 reg, u32)
		{
			if (REGS(ctx)->registers[reg] != REGS(ctx)->latch)
			{
				RSX(ctx)->m_graphics_state |= rsx::pipeline_state::vertex_program_ucode_dirty;
			}
		}

		void set_vertex_attribute_output_mask(context* ctx, u32 reg, u32)
		{
			if (REGS(ctx)->registers[reg] != REGS(ctx)->latch)
			{
				RSX(ctx)->m_graphics_state |= rsx::pipeline_state::vertex_program_state_dirty;
			}
		}

		void set_vertex_base_offset(context* ctx, u32 reg, u32 arg)
		{
			util::push_draw_parameter_change(ctx, vertex_base_modifier_barrier, reg, arg);
		}

		void set_index_base_offset(context* ctx, u32 reg, u32 arg)
		{
			util::push_draw_parameter_change(ctx, index_base_modifier_barrier, reg, arg);
		}

		void check_index_array_dma(context* ctx, u32 reg, u32 arg)
		{
			// Check if either location or index type are invalid
			if (arg & ~(CELL_GCM_LOCATION_MAIN | (CELL_GCM_DRAW_INDEX_ARRAY_TYPE_16 << 4)))
			{
				// Ignore invalid value, recover
				REGS(ctx)->registers[reg] = REGS(ctx)->latch;
				RSX(ctx)->recover_fifo();

				rsx_log.error("Invalid NV4097_SET_INDEX_ARRAY_DMA value: 0x%x", arg);
			}
		}

		///// Drawing

		void set_begin_end(context* ctx, u32 /*reg*/, u32 arg)
		{
			// Ignore upper bits
			if (const u8 prim = static_cast<u8>(arg))
			{
				const auto primitive_type = to_primitive_type(prim);
				if (!primitive_type)
				{
					RSX(ctx)->in_begin_end = true;

					rsx_log.warning("Invalid NV4097_SET_BEGIN_END value: 0x%x", arg);
					return;
				}

				REGS(ctx)->current_draw_clause.reset(primitive_type);
				RSX(ctx)->begin();
				return;
			}

			// Check if we have immediate mode vertex data in a driver-local buffer
			if (REGS(ctx)->current_draw_clause.command == rsx::draw_command::none)
			{
				const u32 push_buffer_vertices_count = RSX(ctx)->GRAPH_frontend().get_push_buffer_vertex_count();
				const u32 push_buffer_index_count = RSX(ctx)->GRAPH_frontend().get_push_buffer_index_count();

				// Need to set this flag since it overrides some register contents
				REGS(ctx)->current_draw_clause.is_immediate_draw = true;

				if (push_buffer_index_count)
				{
					REGS(ctx)->current_draw_clause.command = rsx::draw_command::indexed;
					REGS(ctx)->current_draw_clause.append(0, push_buffer_index_count);
				}
				else if (push_buffer_vertices_count)
				{
					REGS(ctx)->current_draw_clause.command = rsx::draw_command::array;
					REGS(ctx)->current_draw_clause.append(0, push_buffer_vertices_count);
				}
			}
			else
			{
				REGS(ctx)->current_draw_clause.is_immediate_draw = false;
			}

			if (!REGS(ctx)->current_draw_clause.empty())
			{
				REGS(ctx)->current_draw_clause.compile();

				if (g_cfg.video.disable_video_output)
				{
					RSX(ctx)->execute_nop_draw();
					RSX(ctx)->rsx::thread::end();
					return;
				}

				// Notify the backend if the drawing style changes (instanced vs non-instanced)
				if (REGS(ctx)->current_draw_clause.is_trivial_instanced_draw != RSX(ctx)->is_current_vertex_program_instanced())
				{
					RSX(ctx)->m_graphics_state |= rsx::pipeline_state::xform_instancing_state_dirty;
				}

				RSX(ctx)->end();
			}
			else
			{
				RSX(ctx)->in_begin_end = false;
			}

			if (RSX(ctx)->pause_on_draw && RSX(ctx)->pause_on_draw.exchange(false))
			{
				RSX(ctx)->state -= cpu_flag::dbg_step;
				RSX(ctx)->state += cpu_flag::dbg_pause;
				RSX(ctx)->check_state();
			}
		}

		void clear(context* ctx, u32 /*reg*/, u32 arg)
		{
			RSX(ctx)->clear_surface(arg);

			RSX_CAPTURE_EVENT("clear");
		}

		void clear_zcull(context* ctx, u32 /*reg*/, u32 /*arg*/)
		{
			RSX_CAPTURE_EVENT("clear zcull memory");
		}

		void set_face_property(context* ctx, u32 reg, u32 arg)
		{
			if (arg == REGS(ctx)->latch)
			{
				return;
			}

			bool valid;
			switch (reg)
			{
			case NV4097_SET_CULL_FACE:
				valid = !!to_cull_face(arg); break;
			case NV4097_SET_FRONT_FACE:
				valid = !!to_front_face(arg); break;
			default:
				valid = false; break;
			}

			if (valid) [[ likely ]]
			{
				RSX(ctx)->m_graphics_state |= rsx::pipeline_config_dirty;
			}
			else
			{
				REGS(ctx)->registers[reg] = REGS(ctx)->latch;
			}
		}

		void set_blend_equation(context* ctx, u32 reg, u32 arg)
		{
			if (arg == REGS(ctx)->latch)
			{
				return;
			}

			if (to_blend_equation(arg & 0xFFFF) &&
				to_blend_equation((arg >> 16) & 0xFFFF)) [[ likely ]]
			{
				RSX(ctx)->m_graphics_state |= rsx::pipeline_config_dirty;
				return;
			}

			// Rollback
			REGS(ctx)->decode(reg, REGS(ctx)->latch);
		}

		void set_blend_factor(context* ctx, u32 reg, u32 arg)
		{
			if (arg == REGS(ctx)->latch)
			{
				return;
			}

			if (to_blend_factor(arg & 0xFFFF) &&
				to_blend_factor((arg >> 16) & 0xFFFF)) [[ likely ]]
			{
				RSX(ctx)->m_graphics_state |= rsx::pipeline_config_dirty;
				return;
			}

			// Rollback
			REGS(ctx)->decode(reg, REGS(ctx)->latch);
		}

		void set_transform_constant_load(context* ctx, u32 reg, u32 arg)
		{
			util::push_draw_parameter_change(ctx, rsx::transform_constant_load_modifier_barrier, reg, arg);
		}

		///// Reports

		void get_report(context* ctx, u32 /*reg*/, u32 arg)
		{
			u8 type = arg >> 24;
			u32 offset = arg & 0xffffff;

			auto address_ptr = util::get_report_data_impl(ctx, offset);
			if (!address_ptr)
			{
				rsx_log.error("Bad argument passed to NV4097_GET_REPORT, arg=0x%X", arg);
				return;
			}

			switch (type)
			{
			case CELL_GCM_ZPASS_PIXEL_CNT:
			case CELL_GCM_ZCULL_STATS:
			case CELL_GCM_ZCULL_STATS1:
			case CELL_GCM_ZCULL_STATS2:
			case CELL_GCM_ZCULL_STATS3:
				RSX(ctx)->get_zcull_stats(type, vm::cast(address_ptr));
				break;
			default:
				rsx_log.error("NV4097_GET_REPORT: Bad type %d", type);

				vm::_ptr<atomic_t<CellGcmReportData>>(address_ptr)->atomic_op([&](CellGcmReportData& data)
				{
					data.timer = RSX(ctx)->timestamp();
					data.padding = 0;
				});
				break;
			}
		}

		void clear_report_value(context* ctx, u32 /*reg*/, u32 arg)
		{
			switch (arg)
			{
			case CELL_GCM_ZPASS_PIXEL_CNT:
			case CELL_GCM_ZCULL_STATS:
				break;
			default:
				rsx_log.error("NV4097_CLEAR_REPORT_VALUE: Bad type: %d", arg);
				break;
			}

			RSX(ctx)->clear_zcull_stats(arg);
		}

		void set_render_mode(context* ctx, u32, u32 arg)
		{
			const u32 mode = arg >> 24;
			switch (mode)
			{
			case 1:
				RSX(ctx)->disable_conditional_rendering();
				return;
			case 2:
				break;
			default:
				rsx_log.error("Unknown render mode %d", mode);
				return;
			}

			const u32 offset = arg & 0xffffff;
			auto address_ptr = util::get_report_data_impl(ctx, offset);

			if (!address_ptr)
			{
				rsx_log.error("Bad argument passed to NV4097_SET_RENDER_ENABLE, arg=0x%X", arg);
				return;
			}

			// Defer conditional render evaluation
			RSX(ctx)->enable_conditional_rendering(vm::cast(address_ptr));
		}

		void set_zcull_render_enable(context* ctx, u32, u32)
		{
			RSX(ctx)->notify_zcull_info_changed();
		}

		void set_zcull_stats_enable(context* ctx, u32, u32)
		{
			RSX(ctx)->notify_zcull_info_changed();
		}

		void set_zcull_pixel_count_enable(context* ctx, u32, u32)
		{
			RSX(ctx)->notify_zcull_info_changed();
		}

		///// Misc (sync objects, etc)

		void set_notify(context* ctx, u32 /*reg*/, u32 /*arg*/)
		{
			const u32 location = REGS(ctx)->context_dma_notify();
			const u32 index = (location & 0x7) ^ 0x7;

			if ((location & ~7) != (CELL_GCM_CONTEXT_DMA_NOTIFY_MAIN_0 & ~7))
			{
				if (rsx_log.trace)
					rsx_log.trace("NV4097_NOTIFY: invalid context = 0x%x", REGS(ctx)->context_dma_notify());
				return;
			}

			const u32 addr = RSX(ctx)->iomap_table.get_addr(0xf100000 + (index * 0x40));

			ensure(addr != umax);

			vm::_ptr<atomic_t<RsxNotify>>(addr)->store(
			{
				RSX(ctx)->timestamp(),
				0
			});
		}

		void texture_read_semaphore_release(context* ctx, u32 /*reg*/, u32 arg)
		{
			// Pipeline barrier seems to be equivalent to a SHADER_READ stage barrier.
			// Ideally the GPU only needs to have cached all textures declared up to this point before writing the label.

			// lle-gcm likes to inject system reserved semaphores, presumably for system/vsh usage
			// Avoid calling render to avoid any havoc(flickering) they may cause from invalid flush/write
			const u32 offset = REGS(ctx)->semaphore_offset_4097();

			if (offset % 16)
			{
				rsx_log.error("NV4097 semaphore using unaligned offset, recovering. (offset=0x%x)", offset);
				RSX(ctx)->recover_fifo();
				return;
			}

			const u32 addr = get_address(offset, REGS(ctx)->semaphore_context_dma_4097());

			if (RSX(ctx)->label_addr >> 28 != addr >> 28)
			{
				rsx_log.error("NV4097 semaphore unexpected address. Please report to the developers. (offset=0x%x, addr=0x%x)", offset, addr);
			}

			if (g_cfg.video.strict_rendering_mode) [[ unlikely ]]
			{
				util::write_gcm_label<true, true>(ctx, addr, arg);
			}
			else
			{
				util::write_gcm_label<true, false>(ctx, addr, arg);
			}
		}

		void back_end_write_semaphore_release(context* ctx, u32 /*reg*/, u32 arg)
		{
			// Full pipeline barrier. GPU must flush pipeline before writing the label

			const u32 offset = REGS(ctx)->semaphore_offset_4097();

			if (offset % 16)
			{
				rsx_log.error("NV4097 semaphore using unaligned offset, recovering. (offset=0x%x)", offset);
				RSX(ctx)->recover_fifo();
				return;
			}

			const u32 addr = get_address(offset, REGS(ctx)->semaphore_context_dma_4097());

			if (RSX(ctx)->label_addr >> 28 != addr >> 28)
			{
				rsx_log.error("NV4097 semaphore unexpected address. Please report to the developers. (offset=0x%x, addr=0x%x)", offset, addr);
			}

			const u32 val = (arg & 0xff00ff00) | ((arg & 0xff) << 16) | ((arg >> 16) & 0xff);
			util::write_gcm_label<true, true>(ctx, addr, val);
		}

		void sync(context* ctx, u32, u32)
		{
			RSX(ctx)->sync();
		}
	}
}
