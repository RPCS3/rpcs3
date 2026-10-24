#include "stdafx.h"
#include "nv4097.h"
#include "nv47_sync.hpp"

#include "Emu/RSX/RSXThread.h"
#include "Emu/RSX/Common/BufferUtils.h"
#include "Emu/system_config.h"

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

		set_transform_constant::write_range set_transform_constant::compute_write_range([[maybe_unused]] context* ctx, u32 reg, u32 count)
		{
			const u32 load = REGS(ctx)->transform_constant_load();
			if (load >= max_transform_constants)
			{
				return {};
			}

			const u32 first_word = load * 4 + (reg - NV4097_SET_TRANSFORM_CONSTANT);
			const u32 max_words = max_transform_constants * 4;
			if (first_word >= max_words)
			{
				return {};
			}

			return { first_word, std::min(count, max_words - first_word) };
		}

		u32* set_transform_constant::get_constants_ptr([[maybe_unused]] context* ctx, u32 word)
		{
			return &REGS(ctx)->transform_constants[word / 4][word % 4];
		}

		void set_transform_constant::decode_one(context* ctx, u32 reg, u32 arg)
		{
			const auto range = compute_write_range(ctx, reg, 1);
			if (!range.word_count)
			{
				return;
			}

			*get_constants_ptr(ctx, range.first_word) = arg;
		}

		void set_transform_constant::batch_decode(context* ctx, u32 reg, const std::span<const u32>& args, const std::function<bool(context*, u32, u32)>& notify)
		{
			const auto range = compute_write_range(ctx, reg, ::size32(args));
			if (!range.word_count)
			{
				return;
			}

			copy_data_swap_u32(get_constants_ptr(ctx, range.first_word), args.data(), range.word_count);

			// Notify using the range of vec4 constants touched by the write
			const u32 first_constant = range.first_word / 4;
			const u32 end_constant = (range.first_word + range.word_count + 3) / 4;
			const u32 constant_count = end_constant - first_constant;

			if (!notify || !notify(ctx, first_constant, constant_count))
			{
				RSX(ctx)->patch_transform_constants(ctx, first_constant, constant_count);
			}
		}

		void set_transform_constant::write_constants(context* ctx, u32 first_word, const u32* src, u32 count)
		{
			const auto dst = get_constants_ptr(ctx, first_word);

			if (RSX(ctx)->m_graphics_state & rsx::pipeline_state::transform_constants_dirty)
			{
				// Minor optimization: don't compare values if we already know we need invalidation
				copy_data_swap_u32(dst, src, count);
				return;
			}

			if (copy_data_swap_u32_cmp(dst, src, count))
			{
				// Transform constants invalidation is expensive (~8k bytes per update)
				RSX(ctx)->m_graphics_state |= rsx::pipeline_state::transform_constants_dirty;
			}
		}

		void set_transform_constant::impl(context* ctx, u32 reg, [[maybe_unused]] u32 arg)
		{
			const u32 index = reg - NV4097_SET_TRANSFORM_CONSTANT;
			const bool non_increment = (RSX(ctx)->fifo_ctrl->last_cmd() & RSX_METHOD_NON_INCREMENT_CMD_MASK) == RSX_METHOD_NON_INCREMENT_CMD;

			// FIFO args count including this one
			const u32 fifo_args_cnt = RSX(ctx)->fifo_ctrl->get_remaining_args_count() + 1;

			// The range of methods this function resposible to.
			// Incrementing commands advance the register per arg, so args beyond the 32-register window target other methods.
			// Non-incrementing commands send every arg to this same register.
			const u32 method_range = non_increment ? fifo_args_cnt : 32 - index;

			// Get limit imposed by FIFO PUT (if put is behind get it will result in a number ignored by min)
			const u32 fifo_read_limit = static_cast<u32>(((RSX(ctx)->ctrl->put & ~3ull) - (RSX(ctx)->fifo_ctrl->get_pos())) / 4);

			// Number of args owned by this method. Every one of these must be consumed, even if the write is trimmed or dropped entirely.
			const u32 count = std::min<u32>({ fifo_args_cnt, fifo_read_limit, method_range });

			// Non-incrementing writes all land on the same word, so only the last arg sticks
			const u32 src_offset = non_increment ? count - 1 : 0;
			const u32 write_count = count - src_offset;

			const auto range = compute_write_range(ctx, reg, write_count);
			if (range.word_count < write_count)
			{
				rsx_log.warning("Invalid transform register index (load=%u, index=%u, count=%u)", REGS(ctx)->transform_constant_load(), index, write_count);
			}

			if (range.word_count == 0)
			{
				// Out-of-bounds write is a NOP
				rsx_log.trace("Out of bounds write for transform constant block.");
				RSX(ctx)->fifo_ctrl->skip_methods(count - 1);
				return;
			}

			if (RSX(ctx)->in_begin_end && !REGS(ctx)->current_draw_clause.empty())
			{
				// Updating constants mid-draw is messy. Defer the writes
				REGS(ctx)->current_draw_clause.insert_command_barrier(
					rsx::transform_constant_update_barrier,
					RSX(ctx)->fifo_ctrl->get_pos() + src_offset * 4,
					range.word_count,
					index
				);

				RSX(ctx)->fifo_ctrl->skip_methods(count - 1);
				return;
			}

			const u32 read_count = src_offset + range.word_count;
			const auto fifo_span = RSX(ctx)->fifo_ctrl->get_current_arg_ptr(read_count);

			if (const u32 available = ::size32(fifo_span); available < read_count)
			{
				// FIFO data is not contiguous. Consume what we can see, the remaining args are dispatched again.
				if (non_increment)
				{
					write_constants(ctx, range.first_word, &fifo_span[available - 1], 1);
				}
				else
				{
					write_constants(ctx, range.first_word, fifo_span.data(), available);
				}

				RSX(ctx)->fifo_ctrl->skip_methods(available - 1);
				return;
			}

			write_constants(ctx, range.first_word, fifo_span.data() + src_offset, range.word_count);
			RSX(ctx)->fifo_ctrl->skip_methods(count - 1);
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

			// Number of args owned by this method. Every one of these must be consumed, even if the write is trimmed or dropped entirely.
			const u32 count = std::min<u32>({ fifo_args_cnt, fifo_read_limit, method_range });

			// Writes start at the current load position, which advances by one instruction per 4 words written
			const u32 load_pos = REGS(ctx)->transform_program_load();
			constexpr u32 max_words = max_vertex_program_instructions * 4;
			const u32 first_word = load_pos < max_vertex_program_instructions ? load_pos * 4 + index % 4 : max_words;
			const u32 write_count = std::min(count, max_words - first_word);

			if (write_count < count)
			{
				rsx_log.warning("Program buffer overflow! (load=%u, index=%u, count=%u)", load_pos, index, count);
			}

			if (write_count == 0 || first_word >= REGS(ctx)->transform_program.size())
			{
				// Out-of-bounds write is a NOP
				rsx_log.trace("Out of bounds write for transform program block.");
				RSX(ctx)->fifo_ctrl->skip_methods(count - 1);
				return;
			}

			const auto fifo_span = RSX(ctx)->fifo_ctrl->get_current_arg_ptr(write_count);
			const u32 rcount = std::min(write_count, ::size32(fifo_span));

			const auto out_ptr = &REGS(ctx)->transform_program[first_word];

			pipeline_state to_set_dirty = rsx::pipeline_state::vertex_program_ucode_dirty;

			if (rcount >= 4 && !RSX(ctx)->m_graphics_state.test(rsx::pipeline_state::vertex_program_ucode_dirty))
			{
				// Assume clean
				to_set_dirty = {};

				const usz first_index_off = 0;
				const usz second_index_off = (((rcount / 4) - 1) / 2) * 4;

				const u64 src_op1_2 = read_from_ptr<be_t<u64>>(fifo_span, first_index_off);
				const u64 src_op2_2 = read_from_ptr<be_t<u64>>(fifo_span, second_index_off);

				// Fast comparison
				if (src_op1_2 != read_from_ptr_unsafe<u64>(out_ptr, first_index_off) || src_op2_2 != read_from_ptr_unsafe<u64>(out_ptr, second_index_off))
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

			// If the FIFO span came up short, the remaining in-range args are dispatched again. Otherwise consume everything, including the trimmed tail.
			RSX(ctx)->fifo_ctrl->skip_methods((rcount < write_count ? rcount : count) - 1);
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

		void set_zmin_max_control(context* ctx, u32 /*reg*/, u32 arg)
		{
			if (arg == REGS(ctx)->latch)
			{
				return;
			}

			RSX(ctx)->m_graphics_state |= rsx::pipeline_state::pipeline_config_dirty;

			// Depth clip and clamp are read by the fragment epilogue when the depth range is emulated
			if (!RSX(ctx)->get_backend_config().supports_extended_depth_range && g_cfg.video.emulate_extended_depth_range)
			{
				RSX(ctx)->m_graphics_state |= rsx::pipeline_state::fragment_state_dirty;
			}
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

			// Check for different ROP emulation
			if (current.is_integer_color_format() != previous.is_integer_color_format())
			{
				RSX(ctx)->m_graphics_state |= rsx::pipeline_state::fragment_program_state_dirty;
			}

			// If swizzle remap changed, we have to flag both the shader and the ROP parameters
			if (current.is_remapped_format() != previous.is_remapped_format())
			{
				RSX(ctx)->m_graphics_state |=
					rsx::pipeline_state::fragment_program_state_dirty |
					rsx::pipeline_state::fragment_state_dirty;
			}
			// If we're still remapping outputs but the format changed, reload ROP params
			else if ((current.is_remapped_format() && *current.color_fmt() != *previous.color_fmt()))
			{
				RSX(ctx)->m_graphics_state |= rsx::pipeline_state::fragment_state_dirty;
			}
			// If antialias control has changed, also update ROP parameters
			else if (*current.antialias() != *previous.antialias())
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

		void set_aa_control(context* ctx, u32 /*reg*/, u32 arg)
		{
			const auto latch = REGS(ctx)->latch;
			if (arg == latch)
			{
				return;
			}

			// Reconfigure pipeline.
			RSX(ctx)->m_graphics_state |= rsx::pipeline_config_dirty;

			// If we support A2C in hardware, leave the rest upto the hardware. The pipeline config should take care of it.
			const auto& backend_config = RSX(ctx)->get_backend_config();
			if (backend_config.supports_hw_a2c &&
				backend_config.supports_hw_a2c_1spp)
			{
				return;
			}

			// No A2C hardware support or partial hardware support. Invalidate the current program if A2C state changed.
			const auto a2c_old = REGS(ctx)->decode<NV4097_SET_ANTI_ALIASING_CONTROL>(latch).msaa_alpha_to_coverage();
			const auto a2c_new = REGS(ctx)->decode<NV4097_SET_ANTI_ALIASING_CONTROL>(arg).msaa_alpha_to_coverage();
			if (a2c_old != a2c_new)
			{
				RSX(ctx)->m_graphics_state |= rsx::fragment_program_state_dirty;
			}
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
			{
				struct logged_t
				{
					atomic_t<u8> logged_cause[256]{};
				};

				const auto& is_error = ::at32(g_fxo->get<logged_t>().logged_cause, mode).try_inc(10);
				(is_error ? rsx_log.error : rsx_log.trace)("Unknown render mode %d", mode);
				return;
			}
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

		void set_shading_mode(context* ctx, u32 reg, u32 arg)
		{
			if (arg == REGS(ctx)->latch)
			{
				return;
			}

			if (to_shading_mode(arg))
			{
				RSX(ctx)->m_graphics_state |= rsx::vertex_program_state_dirty | rsx::fragment_program_state_dirty;
				return;
			}

			// Rollback
			REGS(ctx)->decode(reg, REGS(ctx)->latch);
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

			// Notify ticks are strongly ordered
			RSX(ctx)->sync();

			vm::_ptr<atomic_t<RsxNotify>>(addr)->store(
			{
				RSX(ctx)->timestamp(),
				0
			});
		}

		void texture_read_semaphore_release(context* ctx, u32 reg, u32 arg)
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
				util::write_gcm_label<true, true>(ctx, reg, addr, arg);
			}
			else
			{
				util::write_gcm_label<true, false>(ctx, reg, addr, arg);
			}
		}

		void back_end_write_semaphore_release(context* ctx, u32 reg, u32 arg)
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
			util::write_gcm_label<true, true>(ctx, reg, addr, val);
		}

		void sync(context* ctx, u32, u32)
		{
			RSX(ctx)->sync();
		}
	}
}
