#include "stdafx.h"
#include "rsx_methods.h"
#include "RSXThread.h"
#include "rsx_utils.h"
#include "rsx_decode.h"
#include "Emu/Cell/PPUCallback.h"
#include "Emu/Cell/lv2/sys_rsx.h"
#include "Emu/RSX/Common/BufferUtils.h"

#include <thread>

namespace rsx
{
	rsx_state method_registers;

	std::array<rsx_method_t, 0x10000 / 4> methods{};

	void invalid_method(thread* rsx, u32 _reg, u32 arg)
	{
		//Don't throw, gather information and ignore broken/garbage commands
		//TODO: Investigate why these commands are executed at all. (Heap corruption? Alignment padding?)
		const u32 cmd = rsx->get_fifo_cmd();
		rsx_log.error("Invalid RSX method 0x%x (arg=0x%x, start=0x%x, count=0x%x, non-inc=%s)", _reg << 2, arg,
		cmd & 0xfffc, (cmd >> 18) & 0x7ff, !!(cmd & RSX_METHOD_NON_INCREMENT_CMD));
		rsx->recover_fifo();
	}

	static void trace_method(thread* rsx, u32 _reg, u32 arg)
	{
		// For unknown yet valid methods
		rsx_log.trace("RSX method 0x%x (arg=0x%x)", _reg << 2, arg);
	}

	template<typename Type> struct vertex_data_type_from_element_type;
	template<> struct vertex_data_type_from_element_type<float> { static const vertex_base_type type = vertex_base_type::f; };
	template<> struct vertex_data_type_from_element_type<f16> { static const vertex_base_type type = vertex_base_type::sf; };
	template<> struct vertex_data_type_from_element_type<u8> { static const vertex_base_type type = vertex_base_type::ub; };
	template<> struct vertex_data_type_from_element_type<u16> { static const vertex_base_type type = vertex_base_type::s32k; };
	template<> struct vertex_data_type_from_element_type<s16> { static const vertex_base_type type = vertex_base_type::s1; };

	namespace nv406e
	{
		void set_reference(thread* rsx, u32 _reg, u32 arg)
		{
			rsx->sync();

			// Write ref+get atomically (get will be written again with the same value at command end)
			vm::_ref<atomic_be_t<u64>>(rsx->dma_address + ::offset32(&RsxDmaControl::get)).store(u64{rsx->fifo_ctrl->get_pos()} << 32 | arg);
		}

		void semaphore_acquire(thread* rsx, u32 /*_reg*/, u32 arg)
		{
			rsx->sync_point_request.release(true);
			const u32 addr = get_address(method_registers.semaphore_offset_406e(), method_registers.semaphore_context_dma_406e());

			const auto& sema = vm::_ref<RsxSemaphore>(addr).val;

			// TODO: Remove vblank semaphore hack
			if (addr == rsx->device_addr + 0x30) return;

			if (sema == arg)
			{
				// Flip semaphore doesnt need wake-up delay
				if (addr != rsx->label_addr + 0x10)
				{
					rsx->flush_fifo();
					rsx->fifo_wake_delay(2);
				}

				return;
			}
			else
			{
				rsx->flush_fifo();
			}

			u64 start = get_system_time();
			while (sema != arg)
			{
				if (rsx->is_stopped())
				{
					return;
				}

				if (const auto tdr = static_cast<u64>(g_cfg.video.driver_recovery_timeout))
				{
					if (rsx->is_paused())
					{
						const u64 start0 = get_system_time();

						while (rsx->is_paused())
						{
							rsx->cpu_wait({});
						}

						// Reset
						start += get_system_time() - start0;
					}
					else
					{
						if ((get_system_time() - start) > tdr)
						{
							// If longer than driver timeout force exit
							rsx_log.error("nv406e::semaphore_acquire has timed out. semaphore_address=0x%X", addr);
							break;
						}
					}
				}

				rsx->cpu_wait({});
			}

			rsx->fifo_wake_delay();
			rsx->performance_counters.idle_time += (get_system_time() - start);
		}

		void semaphore_release(thread* rsx, u32 /*_reg*/, u32 arg)
		{
			rsx->sync();

			const u32 offset = method_registers.semaphore_offset_406e();

			if (offset % 4)
			{
				rsx_log.warning("NV406E semaphore release is using unaligned semaphore, ignoring. (offset=0x%x)", offset);
				return;
			}

			const u32 ctxt = method_registers.semaphore_context_dma_406e();

			// By avoiding doing this on flip's semaphore release
			// We allow last gcm's registers reset to occur in case of a crash
			if (const bool is_flip_sema = (offset == 0x10 && ctxt == CELL_GCM_CONTEXT_DMA_SEMAPHORE_R);
				!is_flip_sema)
			{
				rsx->sync_point_request.release(true);
			}

			const u32 addr = get_address(offset, ctxt);

			// TODO: Check if possible to write on reservations
			if (rsx->label_addr >> 28 != addr >> 28)
			{
				rsx_log.fatal("NV406E semaphore unexpected address. Please report to the developers. (offset=0x%x, addr=0x%x)", offset, addr);
			}

			vm::_ref<RsxSemaphore>(addr).val = arg;
		}
	}

	namespace nv4097
	{
		void clear(thread* rsx, u32 _reg, u32 arg)
		{
			rsx->clear_surface(arg);

			if (rsx->capture_current_frame)
			{
				rsx->capture_frame("clear");
			}
		}

		void clear_zcull(thread* rsx, u32 _reg, u32 arg)
		{
			if (rsx->capture_current_frame)
			{
				rsx->capture_frame("clear zcull memory");
			}
		}

		void set_cull_face(thread* rsx, u32 reg, u32 arg)
		{
			switch(arg)
			{
			case CELL_GCM_FRONT_AND_BACK:
			case CELL_GCM_FRONT:
			case CELL_GCM_BACK:
				return;
			default:
				// Ignore value if unknown
				method_registers.registers[reg] = method_registers.register_previous_value;
			}
		}

		void set_notify(thread* rsx, u32 _reg, u32 arg)
		{
			const u32 location = method_registers.context_dma_notify();
			const u32 index = (location & 0x7) ^ 0x7;

			if ((location & ~7) != (CELL_GCM_CONTEXT_DMA_NOTIFY_MAIN_0 & ~7))
			{
				rsx_log.trace("NV4097_NOTIFY: invalid context = 0x%x", method_registers.context_dma_notify());
				return;
			}

			const u32 addr = rsx->iomap_table.get_addr(0xf100000 + (index * 0x40));

			ensure(addr != umax);

			vm::_ref<atomic_t<RsxNotify>>(addr).store(
			{
				rsx->timestamp(),
				0
			});
		}

		void texture_read_semaphore_release(thread* rsx, u32 _reg, u32 arg)
		{
			// Pipeline barrier seems to be equivalent to a SHADER_READ stage barrier
			g_fxo->get<rsx::dma_manager>().sync();
			if (g_cfg.video.strict_rendering_mode)
			{
				rsx->sync();
			}

			// lle-gcm likes to inject system reserved semaphores, presumably for system/vsh usage
			// Avoid calling render to avoid any havoc(flickering) they may cause from invalid flush/write
			const u32 offset = method_registers.semaphore_offset_4097();

			if (offset % 16)
			{
				rsx_log.error("NV4097 semaphore using unaligned offset, recovering. (offset=0x%x)", offset);
				rsx->recover_fifo();
				return;
			}

			vm::_ref<RsxSemaphore>(get_address(offset, method_registers.semaphore_context_dma_4097())).val = arg;
		}

		void back_end_write_semaphore_release(thread* rsx, u32 _reg, u32 arg)
		{
			// Full pipeline barrier
			g_fxo->get<rsx::dma_manager>().sync();
			rsx->sync();

			const u32 offset = method_registers.semaphore_offset_4097();

			if (offset % 16)
			{
				rsx_log.error("NV4097 semaphore using unaligned offset, recovering. (offset=0x%x)", offset);
				rsx->recover_fifo();
				return;
			}

			const u32 val = (arg & 0xff00ff00) | ((arg & 0xff) << 16) | ((arg >> 16) & 0xff);
			vm::_ref<RsxSemaphore>(get_address(offset, method_registers.semaphore_context_dma_4097())).val = val;
		}

		/**
		 * id = base method register
		 * index = register index in method
		 * count = element count per attribute
		 * register_count = number of registers consumed per attribute. E.g 3-element methods have padding
		 */
		template<u32 id, u32 index, int count, int register_count, typename type>
		void set_vertex_data_impl(thread* rsx, u32 arg)
		{
			static const usz increment_per_array_index = (register_count * sizeof(type)) / sizeof(u32);

			static const usz attribute_index = index / increment_per_array_index;
			static const usz vertex_subreg = index % increment_per_array_index;

			const auto vtype = vertex_data_type_from_element_type<type>::type;
			ensure(vtype != rsx::vertex_base_type::cmp);

			switch (vtype)
			{
			case rsx::vertex_base_type::ub:
			case rsx::vertex_base_type::ub256:
				// Get BE data
				arg = std::bit_cast<u32, be_t<u32>>(arg);
				break;
			default:
				break;
			}

			if (rsx->in_begin_end)
			{
				// Update to immediate mode register/array
				rsx->append_to_push_buffer(attribute_index, count, vertex_subreg, vtype, arg);

				// NOTE: one can update the register to update constant across primitive. Needs verification.
				// Fall through
			}

			auto& info = rsx::method_registers.register_vertex_info[attribute_index];

			info.type = vtype;
			info.size = count;
			info.frequency = 0;
			info.stride = 0;
			rsx::method_registers.register_vertex_info[attribute_index].data[vertex_subreg] = arg;
		}

		template<u32 index>
		struct set_vertex_data4ub_m
		{
			static void impl(thread* rsx, u32 _reg, u32 arg)
			{
				set_vertex_data_impl<NV4097_SET_VERTEX_DATA4UB_M, index, 4, 4, u8>(rsx, arg);
			}
		};

		template<u32 index>
		struct set_vertex_data1f_m
		{
			static void impl(thread* rsx, u32 _reg, u32 arg)
			{
				set_vertex_data_impl<NV4097_SET_VERTEX_DATA1F_M, index, 1, 1, f32>(rsx, arg);
			}
		};

		template<u32 index>
		struct set_vertex_data2f_m
		{
			static void impl(thread* rsx, u32 _reg, u32 arg)
			{
				set_vertex_data_impl<NV4097_SET_VERTEX_DATA2F_M, index, 2, 2, f32>(rsx, arg);
			}
		};

		template<u32 index>
		struct set_vertex_data3f_m
		{
			static void impl(thread* rsx, u32 _reg, u32 arg)
			{
				//Register alignment is only 1, 2, or 4 (Rachet & Clank 2)
				set_vertex_data_impl<NV4097_SET_VERTEX_DATA3F_M, index, 3, 4, f32>(rsx, arg);
			}
		};

		template<u32 index>
		struct set_vertex_data4f_m
		{
			static void impl(thread* rsx, u32 _reg, u32 arg)
			{
				set_vertex_data_impl<NV4097_SET_VERTEX_DATA4F_M, index, 4, 4, f32>(rsx, arg);
			}
		};

		template<u32 index>
		struct set_vertex_data2s_m
		{
			static void impl(thread* rsx, u32 _reg, u32 arg)
			{
				set_vertex_data_impl<NV4097_SET_VERTEX_DATA2S_M, index, 2, 2, u16>(rsx, arg);
			}
		};

		template<u32 index>
		struct set_vertex_data4s_m
		{
			static void impl(thread* rsx, u32 _reg, u32 arg)
			{
				set_vertex_data_impl<NV4097_SET_VERTEX_DATA4S_M, index, 4, 4, u16>(rsx, arg);
			}
		};

		template<u32 index>
		struct set_vertex_data_scaled4s_m
		{
			static void impl(thread* rsx, u32 _reg, u32 arg)
			{
				set_vertex_data_impl<NV4097_SET_VERTEX_DATA_SCALED4S_M, index, 4, 4, s16>(rsx, arg);
			}
		};

		void set_array_element16(thread* rsx, u32, u32 arg)
		{
			if (rsx->in_begin_end)
			{
				rsx->append_array_element(arg & 0xFFFF);
				rsx->append_array_element(arg >> 16);
			}
		}

		void set_array_element32(thread* rsx, u32, u32 arg)
		{
			if (rsx->in_begin_end)
				rsx->append_array_element(arg);
		}

		void draw_arrays(thread* rsx, u32 _reg, u32 arg)
		{
			rsx::method_registers.current_draw_clause.command = rsx::draw_command::array;
			rsx::registers_decoder<NV4097_DRAW_ARRAYS>::decoded_type v(arg);

			rsx::method_registers.current_draw_clause.append(v.start(), v.count());
		}

		void draw_index_array(thread* rsx, u32 _reg, u32 arg)
		{
			rsx::method_registers.current_draw_clause.command = rsx::draw_command::indexed;
			rsx::registers_decoder<NV4097_DRAW_INDEX_ARRAY>::decoded_type v(arg);

			rsx::method_registers.current_draw_clause.append(v.start(), v.count());
		}

		void draw_inline_array(thread* rsx, u32 _reg, u32 arg)
		{
			rsx::method_registers.current_draw_clause.command = rsx::draw_command::inlined_array;
			rsx::method_registers.current_draw_clause.inline_vertex_array.push_back(arg);
		}

		template<u32 index>
		struct set_transform_constant
		{
			static void impl(thread* rsx, u32 /*_reg*/, u32 /*arg*/)
			{
				static constexpr u32 reg = index / 4;
				static constexpr u8 subreg = index % 4;

				// Get real args count
				const u32 count = std::min<u32>({rsx->fifo_ctrl->get_remaining_args_count() + 1,
					static_cast<u32>(((rsx->ctrl->put & ~3ull) - (rsx->fifo_ctrl->get_pos() - 4)) / 4), 32 - index});

				const u32 load = rsx::method_registers.transform_constant_load();

				u32 rcount = count;
				if (const u32 max = (load + reg) * 4 + count + subreg, limit = 468 * 4; max > limit)
				{
					// Ignore addresses outside the usable [0, 467] range
					rsx_log.warning("Invalid transform register index (load=%u, index=%u, count=%u)", load, index, count);

					if ((max - count) < limit)
						rcount -= max - limit;
					else
						rcount = 0;
				}

				const auto values = &rsx::method_registers.transform_constants[load + reg][subreg];

				if (rsx->m_graphics_state & rsx::pipeline_state::transform_constants_dirty)
				{
					// Minor optimization: don't compare values if we already know we need invalidation
					stream_data_to_memory_swapped_u32<true>(values, vm::base(rsx->fifo_ctrl->get_current_arg_ptr()), rcount, 4);
				}
				else
				{
					if (stream_data_to_memory_swapped_and_compare_u32<true>(values, vm::base(rsx->fifo_ctrl->get_current_arg_ptr()), rcount * 4))
					{
						// Transform constants invalidation is expensive (~8k bytes per update)
						rsx->m_graphics_state |= rsx::pipeline_state::transform_constants_dirty;
					}
				}

				rsx->fifo_ctrl->skip_methods(count - 1);
			}
		};

		template<u32 index>
		struct set_transform_program
		{
			static void impl(thread* rsx, u32 /*_reg*/, u32 /*arg*/)
			{
				// Get real args count
				const u32 count = std::min<u32>({rsx->fifo_ctrl->get_remaining_args_count() + 1,
					static_cast<u32>(((rsx->ctrl->put & ~3ull) - (rsx->fifo_ctrl->get_pos() - 4)) / 4), 32 - index});

				const u32 load_pos = rsx::method_registers.transform_program_load();

				u32 rcount = count;

				if (const u32 max = load_pos * 4 + rcount + (index % 4);
					max > 512 * 4)
				{
					// PS3 seems to allow exceeding the program buffer by upto 32 instructions before crashing
					// Discard the "excess" instructions to not overflow our transform program buffer
					// TODO: Check if the instructions in the overflow area are executed by PS3
					rsx_log.warning("Program buffer overflow!");
					rcount -= max - (512 * 4);
				}

				stream_data_to_memory_swapped_u32<true>(&rsx::method_registers.transform_program[load_pos * 4 + index % 4]
					, vm::base(rsx->fifo_ctrl->get_current_arg_ptr()), rcount, 4);

				rsx->m_graphics_state |= rsx::pipeline_state::vertex_program_ucode_dirty;
				rsx::method_registers.transform_program_load_set(load_pos + ((rcount + index % 4) / 4));
				rsx->fifo_ctrl->skip_methods(count - 1);
			}
		};

		void set_transform_program_start(thread* rsx, u32 reg, u32)
		{
			if (method_registers.registers[reg] != method_registers.register_previous_value)
			{
				rsx->m_graphics_state |= rsx::pipeline_state::vertex_program_ucode_dirty;
			}
		}

		void set_vertex_attribute_output_mask(thread* rsx, u32 reg, u32)
		{
			if (method_registers.registers[reg] != method_registers.register_previous_value)
			{
				rsx->m_graphics_state |= rsx::pipeline_state::vertex_program_state_dirty;
			}
		}

		void set_begin_end(thread* rsxthr, u32 _reg, u32 arg)
		{
			// Ignore upper bits
			if (const u8 prim = static_cast<u8>(arg))
			{
				rsx::method_registers.current_draw_clause.reset(to_primitive_type(prim));

				if (rsx::method_registers.current_draw_clause.primitive == rsx::primitive_type::invalid)
				{
					rsxthr->in_begin_end = true;

					rsx_log.warning("Invalid NV4097_SET_BEGIN_END value: 0x%x", arg);
					return;
				}

				rsxthr->begin();
				return;
			}

			//Check if we have immediate mode vertex data in a driver-local buffer
			if (rsx::method_registers.current_draw_clause.command == rsx::draw_command::none)
			{
				const u32 push_buffer_vertices_count = rsxthr->get_push_buffer_vertex_count();
				const u32 push_buffer_index_count = rsxthr->get_push_buffer_index_count();

				//Need to set this flag since it overrides some register contents
				rsx::method_registers.current_draw_clause.is_immediate_draw = true;

				if (push_buffer_index_count)
				{
					rsx::method_registers.current_draw_clause.command = rsx::draw_command::indexed;
					rsx::method_registers.current_draw_clause.append(0, push_buffer_index_count);
				}
				else if (push_buffer_vertices_count)
				{
					rsx::method_registers.current_draw_clause.command = rsx::draw_command::array;
					rsx::method_registers.current_draw_clause.append(0, push_buffer_vertices_count);
				}
			}
			else
				rsx::method_registers.current_draw_clause.is_immediate_draw = false;

			if (!rsx::method_registers.current_draw_clause.empty())
			{
				if (rsx::method_registers.current_draw_clause.primitive == rsx::primitive_type::invalid)
				{
					// Recover from invalid primitive only if draw clause is not empty
					rsxthr->recover_fifo();

					rsx_log.error("NV4097_SET_BEGIN_END aborted due to invalid primitive!");
					return;
				}

				rsx::method_registers.current_draw_clause.compile();
				rsxthr->end();
			}
			else
			{
				rsxthr->in_begin_end = false;
			}
		}

		vm::addr_t get_report_data_impl(u32 offset)
		{
			u32 location = 0;
			blit_engine::context_dma report_dma = method_registers.context_dma_report();

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

		void get_report(thread* rsx, u32 _reg, u32 arg)
		{
			u8 type = arg >> 24;
			u32 offset = arg & 0xffffff;

			auto address_ptr = get_report_data_impl(offset);
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
				rsx->get_zcull_stats(type, address_ptr);
				break;
			default:
				rsx_log.error("NV4097_GET_REPORT: Bad type %d", type);

				vm::_ref<atomic_t<CellGcmReportData>>(address_ptr).atomic_op([&](CellGcmReportData& data)
				{
					data.timer = rsx->timestamp();
					data.padding = 0;
				});
				break;
			}
		}

		void clear_report_value(thread* rsx, u32 _reg, u32 arg)
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

			rsx->clear_zcull_stats(arg);
		}

		void set_render_mode(thread* rsx, u32, u32 arg)
		{
			const u32 mode = arg >> 24;
			switch (mode)
			{
			case 1:
				rsx->disable_conditional_rendering();
				return;
			case 2:
				break;
			default:
				rsx_log.error("Unknown render mode %d", mode);
				return;
			}

			const u32 offset = arg & 0xffffff;
			auto address_ptr = get_report_data_impl(offset);

			if (!address_ptr)
			{
				rsx_log.error("Bad argument passed to NV4097_SET_RENDER_ENABLE, arg=0x%X", arg);
				return;
			}

			// Defer conditional render evaluation
			rsx->enable_conditional_rendering(address_ptr);
		}

		void set_zcull_render_enable(thread* rsx, u32, u32 arg)
		{
			rsx->zcull_rendering_enabled = !!arg;
			rsx->notify_zcull_info_changed();
		}

		void set_zcull_stats_enable(thread* rsx, u32, u32 arg)
		{
			rsx->zcull_stats_enabled = !!arg;
			rsx->notify_zcull_info_changed();
		}

		void set_zcull_pixel_count_enable(thread* rsx, u32, u32 arg)
		{
			rsx->zcull_pixel_cnt_enabled = !!arg;
			rsx->notify_zcull_info_changed();
		}

		void sync(thread* rsx, u32, u32)
		{
			rsx->sync();
		}

		void set_shader_program_dirty(thread* rsx, u32, u32)
		{
			rsx->m_graphics_state |= rsx::pipeline_state::fragment_program_ucode_dirty;
		}

		void set_surface_dirty_bit(thread* rsx, u32 reg, u32 arg)
		{
			if (reg == NV4097_SET_SURFACE_CLIP_VERTICAL ||
				reg == NV4097_SET_SURFACE_CLIP_HORIZONTAL)
			{
				if (arg != method_registers.register_previous_value)
				{
					rsx->m_graphics_state |= rsx::pipeline_state::vertex_state_dirty;
				}
			}

			rsx->m_rtts_dirty = true;
			rsx->m_framebuffer_state_contested = false;
		}

		void set_surface_format(thread* rsx, u32 reg, u32 arg)
		{
			// Special consideration - antialiasing control can affect ROP state
			const auto aa_mask = (0xF << 12);
			if ((arg & aa_mask) != (method_registers.register_previous_value & aa_mask))
			{
				// Antialias control has changed, update ROP parameters
				rsx->m_graphics_state |= rsx::pipeline_state::fragment_state_dirty;
			}

			set_surface_dirty_bit(rsx, reg, arg);
		}

		void set_surface_options_dirty_bit(thread* rsx, u32 reg, u32 arg)
		{
			if (arg != method_registers.register_previous_value)
			{
				rsx->on_framebuffer_options_changed(reg);
			}
		}

		void set_stencil_op(thread* rsx, u32 reg, u32 arg)
		{
			if (arg != method_registers.register_previous_value)
			{
				switch (arg)
				{
				case CELL_GCM_INVERT:
				case CELL_GCM_KEEP:
				case CELL_GCM_REPLACE:
				case CELL_GCM_INCR:
				case CELL_GCM_DECR:
				case CELL_GCM_INCR_WRAP:
				case CELL_GCM_DECR_WRAP:
				case CELL_GCM_ZERO:
					set_surface_options_dirty_bit(rsx, reg, arg);
					break;

				default:
					// Ignored on RSX
					method_registers.decode(reg, method_registers.register_previous_value);
					break;
				}
			}
		}

		template <u32 RsxFlags>
		void notify_state_changed(thread* rsx, u32, u32 arg)
		{
			if (arg != method_registers.register_previous_value)
			{
				rsx->m_graphics_state |= RsxFlags;
			}
		}

		void set_vertex_base_offset(thread* rsx, u32 reg, u32 arg)
		{
			if (rsx->in_begin_end &&
				!rsx::method_registers.current_draw_clause.empty() &&
				reg != method_registers.register_previous_value)
			{
				// Revert change to queue later
				method_registers.decode(reg, method_registers.register_previous_value);

				// Insert base mofifier barrier
				method_registers.current_draw_clause.insert_command_barrier(vertex_base_modifier_barrier, arg);
			}
		}

		void set_index_base_offset(thread* rsx, u32 reg, u32 arg)
		{
			if (rsx->in_begin_end &&
				!rsx::method_registers.current_draw_clause.empty() &&
				reg != method_registers.register_previous_value)
			{
				// Revert change to queue later
				method_registers.decode(reg, method_registers.register_previous_value);

				// Insert base mofifier barrier
				method_registers.current_draw_clause.insert_command_barrier(index_base_modifier_barrier, arg);
			}
		}

		void check_index_array_dma(thread* rsx, u32 reg, u32 arg)
		{
			// Check if either location or index type are invalid
			if (arg & ~(CELL_GCM_LOCATION_MAIN | (CELL_GCM_DRAW_INDEX_ARRAY_TYPE_16 << 4)))
			{
				// Ignore invalid value, recover
				method_registers.registers[reg] = method_registers.register_previous_value;
				rsx->recover_fifo();

				rsx_log.error("Invalid NV4097_SET_INDEX_ARRAY_DMA value: 0x%x", arg);
			}
		}

		void set_blend_equation(thread* rsx, u32 reg, u32 arg)
		{
			for (u32 i = 0; i < 32u; i += 16)
			{
				switch ((arg >> i) & 0xffff)
				{
				case CELL_GCM_FUNC_ADD:
				case CELL_GCM_MIN:
				case CELL_GCM_MAX:
				case CELL_GCM_FUNC_SUBTRACT:
				case CELL_GCM_FUNC_REVERSE_SUBTRACT:
				case CELL_GCM_FUNC_REVERSE_SUBTRACT_SIGNED:
				case CELL_GCM_FUNC_ADD_SIGNED:
				case CELL_GCM_FUNC_REVERSE_ADD_SIGNED:
					break;

				default:
				{
					// Ignore invalid values as a whole
					method_registers.decode(reg, method_registers.register_previous_value);
					return;
				}
				}
			}
		}

		void set_blend_factor(thread* rsx, u32 reg, u32 arg)
		{
			for (u32 i = 0; i < 32u; i += 16)
			{
				switch ((arg >> i) & 0xffff)
				{
				case CELL_GCM_ZERO:
				case CELL_GCM_ONE:
				case CELL_GCM_SRC_COLOR:
				case CELL_GCM_ONE_MINUS_SRC_COLOR:
				case CELL_GCM_SRC_ALPHA:
				case CELL_GCM_ONE_MINUS_SRC_ALPHA:
				case CELL_GCM_DST_ALPHA:
				case CELL_GCM_ONE_MINUS_DST_ALPHA:
				case CELL_GCM_DST_COLOR:
				case CELL_GCM_ONE_MINUS_DST_COLOR:
				case CELL_GCM_SRC_ALPHA_SATURATE:
				case CELL_GCM_CONSTANT_COLOR:
				case CELL_GCM_ONE_MINUS_CONSTANT_COLOR:
				case CELL_GCM_CONSTANT_ALPHA:
				case CELL_GCM_ONE_MINUS_CONSTANT_ALPHA:
					break;

				default:
					// Ignore invalid values as a whole
					method_registers.decode(reg, method_registers.register_previous_value);
					return;
				}
			}
		}

		template<u32 index>
		struct set_texture_dirty_bit
		{
			static void impl(thread* rsx, u32 _reg, u32 arg)
			{
				rsx->m_textures_dirty[index] = true;

				if (rsx->current_fp_metadata.referenced_textures_mask & (1 << index))
				{
					rsx->m_graphics_state |= rsx::pipeline_state::fragment_program_state_dirty;
				}
			}
		};

		template<u32 index>
		struct set_vertex_texture_dirty_bit
		{
			static void impl(thread* rsx, u32 _reg, u32 arg)
			{
				rsx->m_vertex_textures_dirty[index] = true;

				if (rsx->current_vp_metadata.referenced_textures_mask & (1 << index))
				{
					rsx->m_graphics_state |= rsx::pipeline_state::vertex_program_state_dirty;
				}
			}
		};
	}

	namespace nv308a
	{
		template<u32 index>
		struct color
		{
			static void impl(thread* rsx, u32 /*_reg*/, u32 /*arg*/)
			{
				const u32 out_x_max = method_registers.nv308a_size_out_x();

				if (index >= out_x_max)
				{
					// Skip
					return;
				}

				// Get position of the current command arg
				const u32 src_offset = rsx->fifo_ctrl->get_pos() - 4;

				// Get real args count (starting from NV3089_COLOR)
				const u32 count = std::min<u32>({rsx->fifo_ctrl->get_remaining_args_count() + 1,
					static_cast<u32>(((rsx->ctrl->put & ~3ull) - src_offset) / 4), 0x700 - index, out_x_max - index});

				const u32 dst_dma = method_registers.blit_engine_output_location_nv3062();
				const u32 dst_offset = method_registers.blit_engine_output_offset_nv3062();
				const u32 out_pitch = method_registers.blit_engine_output_pitch_nv3062();

				const u32 x = method_registers.nv308a_x() + index;
				const u32 y = method_registers.nv308a_y();

				// TODO
				//auto res = vm::passive_lock(address, address + write_len);

				switch (method_registers.blit_engine_nv3062_color_format())
				{
				case blit_engine::transfer_destination_format::a8r8g8b8:
				case blit_engine::transfer_destination_format::y32:
				{
					// Bit cast - optimize to mem copy

					const auto dst_address = get_address(dst_offset + (x * 4) + (out_pitch * y), dst_dma);
					const auto src_address = get_address(src_offset, CELL_GCM_LOCATION_MAIN);
					const auto dst = vm::_ptr<u8>(dst_address);
					const auto src = vm::_ptr<const u8>(src_address);

					const u32 data_length = count * 4;
					auto res = rsx::reservation_lock<true>(dst_address, data_length, src_address, data_length);

					if (rsx->fifo_ctrl->last_cmd() & RSX_METHOD_NON_INCREMENT_CMD_MASK) [[unlikely]]
					{
						// Move last 32 bits
						reinterpret_cast<u32*>(dst)[0] = reinterpret_cast<const u32*>(src)[count - 1];
						rsx->invalidate_fragment_program(dst_dma, dst_offset, 4);
					}
					else
					{
						if (dst_dma & CELL_GCM_LOCATION_MAIN)
						{
							// May overlap
							std::memmove(dst, src, data_length);
						}
						else
						{
							// Never overlaps
							std::memcpy(dst, src, data_length);
						}

						rsx->invalidate_fragment_program(dst_dma, dst_offset, count * 4);
					}

					break;
				}
				case blit_engine::transfer_destination_format::r5g6b5:
				{
					const auto dst_address = get_address(dst_offset + (x * 2) + (y * out_pitch), dst_dma);
					const auto src_address = get_address(src_offset, CELL_GCM_LOCATION_MAIN);
					const auto dst = vm::_ptr<u16>(dst_address);
					const auto src = vm::_ptr<const u32>(src_address);

					const auto data_length = count * 2;
					auto res = rsx::reservation_lock<true>(dst_address, data_length, src_address, data_length);

					auto convert = [](u32 input) -> u16
					{
						// Input is considered to be ARGB8
						u32 r = (input >> 16) & 0xFF;
						u32 g = (input >> 8) & 0xFF;
						u32 b = input & 0xFF;

						r = (r * 32) / 255;
						g = (g * 64) / 255;
						b = (b * 32) / 255;
						return static_cast<u16>((r << 11) | (g << 5) | b);
					};

					if (rsx->fifo_ctrl->last_cmd() & RSX_METHOD_NON_INCREMENT_CMD_MASK) [[unlikely]]
					{
						// Move last 16 bits
						dst[0] = convert(src[count - 1]);
						rsx->invalidate_fragment_program(dst_dma, dst_offset, 2);
						break;
					}

					for (u32 i = 0; i < count; i++)
					{
						dst[i] = convert(src[i]);
					}

					rsx->invalidate_fragment_program(dst_dma, dst_offset, count * 2);
					break;
				}
				default:
				{
					fmt::throw_exception("Unreachable");
				}
				}

				//res->release(0);

				// Skip "handled methods"
				rsx->fifo_ctrl->skip_methods(count - 1);
			}
		};
	}

	namespace nv3089
	{
		void image_in(thread *rsx, u32 _reg, u32 arg)
		{
			const rsx::blit_engine::transfer_operation operation = method_registers.blit_engine_operation();

			const u16 out_x = method_registers.blit_engine_output_x();
			const u16 out_y = method_registers.blit_engine_output_y();
			const u16 out_w = method_registers.blit_engine_output_width();
			const u16 out_h = method_registers.blit_engine_output_height();

			const u16 in_w = method_registers.blit_engine_input_width();
			const u16 in_h = method_registers.blit_engine_input_height();

			const blit_engine::transfer_origin in_origin = method_registers.blit_engine_input_origin();
			const blit_engine::transfer_interpolator in_inter = method_registers.blit_engine_input_inter();
			rsx::blit_engine::transfer_source_format src_color_format = method_registers.blit_engine_src_color_format();

			const f32 scale_x = method_registers.blit_engine_ds_dx();
			const f32 scale_y = method_registers.blit_engine_dt_dy();

			// Clipping
			// Validate that clipping rect will fit onto both src and dst regions
			const u16 clip_w = std::min(method_registers.blit_engine_clip_width(), out_w);
			const u16 clip_h = std::min(method_registers.blit_engine_clip_height(), out_h);

			// Check both clip dimensions and dst dimensions
			if (clip_w == 0 || clip_h == 0)
			{
				rsx_log.warning("NV3089_IMAGE_IN: Operation NOPed out due to empty regions");
				return;
			}

			if (in_w == 0 || in_h == 0)
			{
				// Input cant be an empty region
				fmt::throw_exception("NV3089_IMAGE_IN_SIZE: Invalid blit dimensions passed (in_w=%d, in_h=%d)", in_w, in_h);
			}

			u16 clip_x = method_registers.blit_engine_clip_x();
			u16 clip_y = method_registers.blit_engine_clip_y();

			//Fit onto dst
			if (clip_x && (out_x + clip_x + clip_w) > out_w) clip_x = 0;
			if (clip_y && (out_y + clip_y + clip_h) > out_h) clip_y = 0;

			u16 in_pitch = method_registers.blit_engine_input_pitch();

			switch (in_origin)
			{
			case blit_engine::transfer_origin::corner:
			case blit_engine::transfer_origin::center:
				break;
			default:
				rsx_log.warning("NV3089_IMAGE_IN_SIZE: unknown origin (%d)", static_cast<u8>(in_origin));
			}

			if (operation != rsx::blit_engine::transfer_operation::srccopy)
			{
				fmt::throw_exception("NV3089_IMAGE_IN_SIZE: unknown operation (%d)", static_cast<u8>(operation));
			}

			const u32 src_offset = method_registers.blit_engine_input_offset();
			const u32 src_dma = method_registers.blit_engine_input_location();

			u32 dst_offset;
			u32 dst_dma = 0;
			rsx::blit_engine::transfer_destination_format dst_color_format;
			u32 out_pitch = 0;
			[[maybe_unused]] u32 out_alignment = 64;
			bool is_block_transfer = false;

			switch (method_registers.blit_engine_context_surface())
			{
			case blit_engine::context_surface::surface2d:
			{
				dst_dma = method_registers.blit_engine_output_location_nv3062();
				dst_offset = method_registers.blit_engine_output_offset_nv3062();
				dst_color_format = method_registers.blit_engine_nv3062_color_format();
				out_pitch = method_registers.blit_engine_output_pitch_nv3062();
				out_alignment = method_registers.blit_engine_output_alignment_nv3062();
				is_block_transfer = fcmp(scale_x, 1.f) && fcmp(scale_y, 1.f);
				break;
			}
			case blit_engine::context_surface::swizzle2d:
			{
				dst_dma = method_registers.blit_engine_nv309E_location();
				dst_offset = method_registers.blit_engine_nv309E_offset();
				dst_color_format = method_registers.blit_engine_output_format_nv309E();
				break;
			}
			default:
				rsx_log.error("NV3089_IMAGE_IN_SIZE: unknown m_context_surface (0x%x)", static_cast<u8>(method_registers.blit_engine_context_surface()));
				return;
			}

			const u32 in_bpp = (src_color_format == rsx::blit_engine::transfer_source_format::r5g6b5) ? 2 : 4; // bytes per pixel
			const u32 out_bpp = (dst_color_format == rsx::blit_engine::transfer_destination_format::r5g6b5) ? 2 : 4;

			if (out_pitch == 0)
			{
				out_pitch = out_bpp * out_w;
			}

			if (in_bpp != out_bpp)
			{
				is_block_transfer = false;
			}

			u16 in_x, in_y;
			if (in_origin == blit_engine::transfer_origin::center)
			{
				// Convert to normal u,v addressing. Under this scheme offset of 1 is actually half-way inside pixel 0
				const float x = std::max(method_registers.blit_engine_in_x(), 0.5f);
				const float y = std::max(method_registers.blit_engine_in_y(), 0.5f);
				in_x = static_cast<u16>(std::floor(x - 0.5f));
				in_y = static_cast<u16>(std::floor(y - 0.5f));
			}
			else
			{
				in_x = static_cast<u16>(std::floor(method_registers.blit_engine_in_x()));
				in_y = static_cast<u16>(std::floor(method_registers.blit_engine_in_y()));
			}

			// Check for subpixel addressing
			if (scale_x < 1.f)
			{
				float dst_x = in_x * scale_x;
				in_x = static_cast<u16>(std::floor(dst_x) / scale_x);
			}

			if (scale_y < 1.f)
			{
				float dst_y = in_y * scale_y;
				in_y = static_cast<u16>(std::floor(dst_y) / scale_y);
			}

			const u32 in_offset = in_x * in_bpp + in_pitch * in_y;
			const u32 out_offset = out_x * out_bpp + out_pitch * out_y;

			const u32 src_address = get_address(src_offset, src_dma);
			const u32 dst_address = get_address(dst_offset, dst_dma);

			const u32 src_line_length = (in_w * in_bpp);

			if (is_block_transfer && (clip_h == 1 || (in_pitch == out_pitch && src_line_length == in_pitch)))
			{
				const u32 nb_lines = std::min(clip_h, in_h);
				const u32 data_length = nb_lines * src_line_length;

				rsx->invalidate_fragment_program(dst_dma, dst_offset, data_length);

				if (const auto result = rsx->read_barrier(src_address, data_length, false);
					result == rsx::result_zcull_intr)
				{
					if (rsx->copy_zcull_stats(src_address, data_length, dst_address) == data_length)
					{
						// All writes deferred
						return;
					}
				}
			}
			else
			{
				const u32 data_length = in_pitch * (in_h - 1) + src_line_length;

				rsx->invalidate_fragment_program(dst_dma, dst_offset, data_length);
				rsx->read_barrier(src_address, data_length, true);
			}

			u8* pixels_src = vm::_ptr<u8>(src_address + in_offset);
			u8* pixels_dst = vm::_ptr<u8>(dst_address + out_offset);

			if (dst_color_format != rsx::blit_engine::transfer_destination_format::r5g6b5 &&
				dst_color_format != rsx::blit_engine::transfer_destination_format::a8r8g8b8)
			{
				fmt::throw_exception("NV3089_IMAGE_IN_SIZE: unknown dst_color_format (%d)", static_cast<u8>(dst_color_format));
			}

			if (src_color_format != rsx::blit_engine::transfer_source_format::r5g6b5 &&
				src_color_format != rsx::blit_engine::transfer_source_format::a8r8g8b8)
			{
				// Alpha has no meaning in both formats
				if (src_color_format == rsx::blit_engine::transfer_source_format::x8r8g8b8)
				{
					src_color_format = rsx::blit_engine::transfer_source_format::a8r8g8b8;
				}
				else
				{
					// TODO: Support more formats
					fmt::throw_exception("NV3089_IMAGE_IN_SIZE: unknown src_color_format (%d)", static_cast<u8>(src_color_format));
				}
			}

			u32 convert_w = static_cast<u32>(std::abs(scale_x) * in_w);
			u32 convert_h = static_cast<u32>(std::abs(scale_y) * in_h);

			if (convert_w == 0 || convert_h == 0)
			{
				rsx_log.error("NV3089_IMAGE_IN: Invalid dimensions or scaling factor. Request ignored (ds_dx=%f, dt_dy=%f)",
					method_registers.blit_engine_ds_dx(), method_registers.blit_engine_dt_dy());
				return;
			}

			// Lock here. RSX cannot execute any locking operations from this point, including ZCULL read barriers
			auto res = ::rsx::reservation_lock<true>(dst_address, out_pitch * out_h, src_address, in_pitch * in_h);

			if (!g_cfg.video.force_cpu_blit_processing && (dst_dma == CELL_GCM_CONTEXT_DMA_MEMORY_FRAME_BUFFER || src_dma == CELL_GCM_CONTEXT_DMA_MEMORY_FRAME_BUFFER))
			{
				blit_src_info src_info = {};
				blit_dst_info dst_info = {};

				src_info.format = src_color_format;
				src_info.origin = in_origin;
				src_info.width = in_w;
				src_info.height = in_h;
				src_info.pitch = in_pitch;
				src_info.offset_x = in_x;
				src_info.offset_y = in_y;
				src_info.rsx_address = src_address;
				src_info.pixels = pixels_src;

				dst_info.format = dst_color_format;
				dst_info.width = convert_w;
				dst_info.height = convert_h;
				dst_info.clip_x = clip_x;
				dst_info.clip_y = clip_y;
				dst_info.clip_width = clip_w;
				dst_info.clip_height = clip_h;
				dst_info.offset_x = out_x;
				dst_info.offset_y = out_y;
				dst_info.pitch = out_pitch;
				dst_info.scale_x = scale_x;
				dst_info.scale_y = scale_y;
				dst_info.rsx_address = dst_address;
				dst_info.pixels = pixels_dst;
				dst_info.swizzled = (method_registers.blit_engine_context_surface() == blit_engine::context_surface::swizzle2d);

				if (rsx->scaled_image_from_memory(src_info, dst_info, in_inter == blit_engine::transfer_interpolator::foh))
					return;
			}

			std::vector<u8> temp1, temp2, temp3, sw_temp;

			if (scale_y < 0 || scale_x < 0)
			{
				const u32 packed_pitch = in_w * in_bpp;
				temp1.resize(packed_pitch * in_h);

				const s32 stride_y = (scale_y < 0 ? -1 : 1) * s32{in_pitch};

				for (u32 y = 0; y < in_h; ++y)
				{
					u8 *dst = temp1.data() + (packed_pitch * y);
					u8 *src = pixels_src + (static_cast<s32>(y) * stride_y);

					if (scale_x < 0)
					{
						if (in_bpp == 2)
						{
							rsx::memcpy_r<u16>(dst, src, in_w);
						}
						else
						{
							rsx::memcpy_r<u32>(dst, src, in_w);
						}
					}
					else
					{
						std::memcpy(dst, src, packed_pitch);
					}
				}

				pixels_src = temp1.data();
				in_pitch = packed_pitch;
			}

			const AVPixelFormat in_format = (src_color_format == rsx::blit_engine::transfer_source_format::r5g6b5) ? AV_PIX_FMT_RGB565BE : AV_PIX_FMT_ARGB;
			const AVPixelFormat out_format = (dst_color_format == rsx::blit_engine::transfer_destination_format::r5g6b5) ? AV_PIX_FMT_RGB565BE : AV_PIX_FMT_ARGB;

			const bool need_clip =
				clip_w != in_w ||
				clip_h != in_h ||
				clip_x > 0 || clip_y > 0 ||
				convert_w != out_w || convert_h != out_h;

			const bool need_convert = out_format != in_format || !rsx::fcmp(fabsf(scale_x), 1.f) || !rsx::fcmp(fabsf(scale_y), 1.f);
			const u32 slice_h = static_cast<u32>(std::ceil(static_cast<f32>(clip_h + clip_y) / scale_y));

			if (method_registers.blit_engine_context_surface() != blit_engine::context_surface::swizzle2d)
			{
				if (!need_convert)
				{
					const bool is_overlapping = scale_x > 0 && scale_y > 0 && dst_dma == src_dma && [&]() -> bool
					{
						const u32 src_max = src_offset + in_pitch * (in_h - 1) + (in_bpp * in_w);
						const u32 dst_max = dst_offset + out_pitch * (out_h - 1) + (out_bpp * out_w);
						return (src_offset >= dst_offset && src_offset < dst_max) ||
						 (dst_offset >= src_offset && dst_offset < src_max);
					}();

					if (is_overlapping)
					{
						if (need_clip)
						{
							temp2.resize(out_pitch * clip_h);

							clip_image_may_overlap(pixels_dst, pixels_src, clip_x, clip_y, clip_w, clip_h, out_bpp, in_pitch, out_pitch, temp2.data());
						}
						else if (out_pitch != in_pitch || out_pitch != out_bpp * out_w)
						{
							const u32 buffer_pitch = out_bpp * out_w;
							temp2.resize(buffer_pitch * out_h);
							std::add_pointer_t<u8> buf = temp2.data(), pixels = pixels_src;

							// Read the whole buffer from source
							for (u32 y = 0; y < out_h; ++y)
							{
								std::memcpy(buf, pixels, buffer_pitch);
								pixels += in_pitch;
								buf += buffer_pitch;
							}

							buf = temp2.data(), pixels = pixels_dst;

							// Write to destination
							for (u32 y = 0; y < out_h; ++y)
							{
								std::memcpy(pixels, buf, buffer_pitch);
								pixels += out_pitch;
								buf += buffer_pitch;
							}
						}
						else
						{
							std::memmove(pixels_dst, pixels_src, out_pitch * out_h);
						}
					}
					else
					{
						if (need_clip)
						{
							clip_image(pixels_dst, pixels_src, clip_x, clip_y, clip_w, clip_h, out_bpp, in_pitch, out_pitch);
						}
						else if (out_pitch != in_pitch || out_pitch != out_bpp * out_w)
						{
							u8 *dst = pixels_dst, *src = pixels_src;

							for (u32 y = 0; y < out_h; ++y)
							{
								std::memcpy(dst, src, out_w * out_bpp);
								dst += out_pitch;
								src += in_pitch;
							}
						}
						else
						{
							std::memcpy(pixels_dst, pixels_src, out_pitch * out_h);
						}
					}
				}
				else
				{
					if (need_clip)
					{
						temp2.resize(out_pitch * std::max<u32>(convert_h, clip_h));

						convert_scale_image(temp2.data(), out_format, convert_w, convert_h, out_pitch,
							pixels_src, in_format, in_w, in_h, in_pitch, slice_h, in_inter == blit_engine::transfer_interpolator::foh);

						clip_image(pixels_dst, temp2.data(), clip_x, clip_y, clip_w, clip_h, out_bpp, out_pitch, out_pitch);
					}
					else
					{
						convert_scale_image(pixels_dst, out_format, out_w, out_h, out_pitch,
							pixels_src, in_format, in_w, in_h, in_pitch, slice_h, in_inter == blit_engine::transfer_interpolator::foh);
					}
				}
			}
			else
			{
				if (need_convert || need_clip)
				{
					if (need_clip)
					{
						temp3.resize(out_pitch * clip_h);

						if (need_convert)
						{
							temp2.resize(out_pitch * std::max<u32>(convert_h, clip_h));

							convert_scale_image(temp2.data(), out_format, convert_w, convert_h, out_pitch,
								pixels_src, in_format, in_w, in_h, in_pitch, slice_h, in_inter == blit_engine::transfer_interpolator::foh);

							clip_image(temp3.data(), temp2.data(), clip_x, clip_y, clip_w, clip_h, out_bpp, out_pitch, out_pitch);
						}
						else
						{
							clip_image(temp3.data(), pixels_src, clip_x, clip_y, clip_w, clip_h, out_bpp, in_pitch, out_pitch);
						}
					}
					else
					{
						temp3.resize(out_pitch * out_h);

						convert_scale_image(temp3.data(), out_format, out_w, out_h, out_pitch,
							pixels_src, in_format, in_w, in_h, in_pitch, slice_h, in_inter == blit_engine::transfer_interpolator::foh);
					}

					pixels_src = temp3.data();
					in_pitch = out_pitch;
				}

				// It looks like rsx may ignore the requested swizzle size and just always
				// round up to nearest power of 2
				/*u8 sw_width_log2 = method_registers.nv309e_sw_width_log2();
				u8 sw_height_log2 = method_registers.nv309e_sw_height_log2();

				// 0 indicates height of 1 pixel
				sw_height_log2 = sw_height_log2 == 0 ? 1 : sw_height_log2;

				// swizzle based on destination size
				u16 sw_width = 1 << sw_width_log2;
				u16 sw_height = 1 << sw_height_log2;
				*/

				u32 sw_width = next_pow2(out_w);
				u32 sw_height = next_pow2(out_h);

				u8* linear_pixels = pixels_src;
				u8* swizzled_pixels = pixels_dst;

				// Check and pad texture out if we are given non power of 2 output
				if (sw_width != out_w || sw_height != out_h)
				{
					sw_temp.resize(out_bpp * sw_width * sw_height);

					switch (out_bpp)
					{
					case 1:
						pad_texture<u8>(linear_pixels, sw_temp.data(), out_w, out_h, sw_width, sw_height);
						break;
					case 2:
						pad_texture<u16>(linear_pixels, sw_temp.data(), out_w, out_h, sw_width, sw_height);
						break;
					case 4:
						pad_texture<u32>(linear_pixels, sw_temp.data(), out_w, out_h, sw_width, sw_height);
						break;
					}

					linear_pixels = sw_temp.data();
				}

				switch (out_bpp)
				{
				case 1:
					convert_linear_swizzle<u8, false>(linear_pixels, swizzled_pixels, sw_width, sw_height, in_pitch);
					break;
				case 2:
					convert_linear_swizzle<u16, false>(linear_pixels, swizzled_pixels, sw_width, sw_height, in_pitch);
					break;
				case 4:
					convert_linear_swizzle<u32, false>(linear_pixels, swizzled_pixels, sw_width, sw_height, in_pitch);
					break;
				}
			}
		}
	}

	namespace nv0039
	{
		void buffer_notify(thread *rsx, u32, u32 arg)
		{
			s32 in_pitch = method_registers.nv0039_input_pitch();
			s32 out_pitch = method_registers.nv0039_output_pitch();
			const u32 line_length = method_registers.nv0039_line_length();
			const u32 line_count = method_registers.nv0039_line_count();
			const u8 out_format = method_registers.nv0039_output_format();
			const u8 in_format = method_registers.nv0039_input_format();
			const u32 notify = arg;

			// The existing GCM commands use only the value 0x1 for inFormat and outFormat
			if (in_format != 0x01 || out_format != 0x01)
			{
				rsx_log.error("NV0039_BUFFER_NOTIFY: Unsupported format: inFormat=%d, outFormat=%d", in_format, out_format);
			}

			if (!line_count || !line_length)
			{
				rsx_log.warning("NV0039_BUFFER_NOTIFY NOPed out: pitch(in=0x%x, out=0x%x), line(len=0x%x, cnt=0x%x), fmt(in=0x%x, out=0x%x), notify=0x%x",
					in_pitch, out_pitch, line_length, line_count, in_format, out_format, notify);
				return;
			}

			rsx_log.trace("NV0039_BUFFER_NOTIFY: pitch(in=0x%x, out=0x%x), line(len=0x%x, cnt=0x%x), fmt(in=0x%x, out=0x%x), notify=0x%x",
				in_pitch, out_pitch, line_length, line_count, in_format, out_format, notify);

			u32 src_offset = method_registers.nv0039_input_offset();
			u32 src_dma = method_registers.nv0039_input_location();

			u32 dst_offset = method_registers.nv0039_output_offset();
			u32 dst_dma = method_registers.nv0039_output_location();

			const bool is_block_transfer = (in_pitch == out_pitch && out_pitch + 0u == line_length);
			const auto read_address = get_address(src_offset, src_dma);
			const auto write_address = get_address(dst_offset, dst_dma);
			const auto read_length = in_pitch * (line_count - 1) + line_length;
			const auto write_length = out_pitch * (line_count - 1) + line_length;

			rsx->invalidate_fragment_program(dst_dma, dst_offset, write_length);

			if (const auto result = rsx->read_barrier(read_address, read_length, !is_block_transfer);
				result == rsx::result_zcull_intr)
			{
				// This transfer overlaps will zcull data pool
				if (rsx->copy_zcull_stats(read_address, read_length, write_address) == write_length)
				{
					// All writes deferred
					return;
				}
			}

			auto res = ::rsx::reservation_lock<true>(write_address, write_length, read_address, read_length);

			u8 *dst = vm::_ptr<u8>(write_address);
			const u8 *src = vm::_ptr<u8>(read_address);

			const bool is_overlapping = dst_dma == src_dma && [&]() -> bool
			{
				const u32 src_max = src_offset + read_length;
				const u32 dst_max = dst_offset + (out_pitch * (line_count - 1) + line_length);
				return (src_offset >= dst_offset && src_offset < dst_max) ||
				 (dst_offset >= src_offset && dst_offset < src_max);
			}();

			if (is_overlapping)
			{
				if (is_block_transfer)
				{
					std::memmove(dst, src, read_length);
				}
				else
				{
					std::vector<u8> temp(line_length * line_count);
					u8* buf = temp.data();

					for (u32 y = 0; y < line_count; ++y)
					{
						std::memcpy(buf, src, line_length);
						buf += line_length;
						src += in_pitch;
					}

					buf = temp.data();

					for (u32 y = 0; y < line_count; ++y)
					{
						std::memcpy(dst, buf, line_length);
						buf += line_length;
						dst += out_pitch;
					}
				}
			}
			else
			{
				if (is_block_transfer)
				{
					std::memcpy(dst, src, read_length);
				}
				else
				{
					for (u32 i = 0; i < line_count; ++i)
					{
						std::memcpy(dst, src, line_length);
						dst += out_pitch;
						src += in_pitch;
					}
				}
			}

			//res->release(0);
		}
	}

	void flip_command(thread* rsx, u32, u32 arg)
	{
		ensure(rsx->isHLE);
		rsx->reset();
		rsx->request_emu_flip(arg);
	}

	void user_command(thread* rsx, u32, u32 arg)
	{
		if (!rsx->isHLE)
		{
			sys_rsx_context_attribute(0x55555555, 0xFEF, 0, arg, 0, 0);
			return;
		}

		if (rsx->user_handler)
		{
			rsx->intr_thread->cmd_list
			({
				{ ppu_cmd::set_args, 1 }, u64{arg},
				{ ppu_cmd::lle_call, rsx->user_handler },
				{ ppu_cmd::sleep, 0 }
			});

			rsx->intr_thread->cmd_notify++;
			rsx->intr_thread->cmd_notify.notify_one();
		}
	}

	namespace gcm
	{
		template<u32 index>
		struct driver_flip
		{
			static void impl(thread* rsx, u32 _reg, u32 arg)
			{
				sys_rsx_context_attribute(0x55555555, 0x102, index, arg, 0, 0);
			}
		};

		template<u32 index>
		struct queue_flip
		{
			static void impl(thread* rsx, u32 _reg, u32 arg)
			{
				sys_rsx_context_attribute(0x55555555, 0x103, index, arg, 0, 0);
			}
		};
	}

	namespace fifo
	{
		void draw_barrier(thread* rsx, u32, u32)
		{
			if (rsx->in_begin_end)
			{
				if (!method_registers.current_draw_clause.is_disjoint_primitive)
				{
					// Enable primitive barrier request
					method_registers.current_draw_clause.primitive_barrier_enable = true;
				}
			}
		}
	}

	void rsx_state::init()
	{
		// Reset all regsiters
		registers.fill(0);
		transform_program.fill(0);
		transform_constants = {};

		// Special values set at initialization, these are not set by a context reset
		registers[NV4097_SET_SHADER_PROGRAM] = (0 << 2) | (CELL_GCM_LOCATION_LOCAL + 1);

		for (u32 i = 0; i < 16; i++)
		{
			registers[NV4097_SET_TEXTURE_FORMAT + (i * 8)] = (1 << 16 /* mipmap */) | ((CELL_GCM_TEXTURE_R5G6B5 | CELL_GCM_TEXTURE_SZ | CELL_GCM_TEXTURE_NR) << 8) | (2 << 4 /* 2D */) | (CELL_GCM_LOCATION_LOCAL + 1);
		}

		for (u32 i = 0; i < 4; i++)
		{
			registers[NV4097_SET_VERTEX_TEXTURE_FORMAT + (i * 8)] = (1 << 16 /* mipmap */) | ((CELL_GCM_TEXTURE_X32_FLOAT | CELL_GCM_TEXTURE_LN | CELL_GCM_TEXTURE_NR) << 8) | (2 << 4 /* 2D */) | (CELL_GCM_LOCATION_LOCAL + 1);
		}

		registers[NV406E_SET_CONTEXT_DMA_SEMAPHORE] = CELL_GCM_CONTEXT_DMA_SEMAPHORE_R;
		registers[NV4097_SET_CONTEXT_DMA_SEMAPHORE] = CELL_GCM_CONTEXT_DMA_SEMAPHORE_RW;

		if (get_current_renderer()->isHLE)
		{
			// Commands injected by cellGcmInit
			registers[NV406E_SEMAPHORE_OFFSET] = 0x30;
			registers[NV406E_SEMAPHORE_ACQUIRE] = 0x1;
			registers[NV406E_SET_CONTEXT_DMA_SEMAPHORE] = 0x66616661;
			registers[0x0] = 0x31337000;
			registers[NV4097_SET_CONTEXT_DMA_NOTIFIES] = 0x66604200;
			registers[NV4097_SET_CONTEXT_DMA_A] = 0xfeed0000;
			registers[NV4097_SET_CONTEXT_DMA_B] = 0xfeed0001;
			registers[NV4097_SET_CONTEXT_DMA_COLOR_B] = 0xfeed0000;
			registers[NV4097_SET_CONTEXT_DMA_STATE] = 0x0;
			registers[NV4097_SET_CONTEXT_DMA_COLOR_A] = 0xfeed0000;
			registers[NV4097_SET_CONTEXT_DMA_ZETA] = 0xfeed0000;
			registers[NV4097_SET_CONTEXT_DMA_VERTEX_A] = 0xfeed0000;
			registers[NV4097_SET_CONTEXT_DMA_VERTEX_B] = 0xfeed0001;
			registers[NV4097_SET_CONTEXT_DMA_SEMAPHORE] = 0x66606660;
			registers[NV4097_SET_CONTEXT_DMA_REPORT] = 0x66626660;
			registers[NV4097_SET_CONTEXT_DMA_CLIP_ID] = 0x0;
			registers[NV4097_SET_CONTEXT_DMA_CULL_DATA] = 0x0;
			registers[NV4097_SET_CONTEXT_DMA_COLOR_C] = 0xfeed0000;
			registers[NV4097_SET_CONTEXT_DMA_COLOR_D] = 0xfeed0000;
			registers[NV406E_SET_CONTEXT_DMA_SEMAPHORE] = 0x66616661;
			registers[NV4097_SET_SURFACE_CLIP_HORIZONTAL] = 0x0;
			registers[NV4097_SET_SURFACE_CLIP_VERTICAL] = 0x0;
			registers[NV4097_SET_SURFACE_FORMAT] = 0x121;
			registers[NV4097_SET_SURFACE_PITCH_A] = 0x40;
			registers[NV4097_SET_SURFACE_COLOR_AOFFSET] = 0x0;
			registers[NV4097_SET_SURFACE_ZETA_OFFSET] = 0x0;
			registers[NV4097_SET_SURFACE_COLOR_BOFFSET] = 0x0;
			registers[NV4097_SET_SURFACE_PITCH_B] = 0x40;
			registers[NV4097_SET_SURFACE_COLOR_TARGET] = 0x1;
			registers[0x224 / 4] = 0x80;
			registers[0x228 / 4] = 0x100;
			registers[NV4097_SET_SURFACE_PITCH_Z] = 0x40;
			registers[0x230 / 4] = 0x0;
			registers[NV4097_SET_SURFACE_PITCH_C] = 0x40;
			registers[NV4097_SET_SURFACE_PITCH_D] = 0x40;
			registers[NV4097_SET_SURFACE_COLOR_COFFSET] = 0x0;
			registers[NV4097_SET_SURFACE_COLOR_DOFFSET] = 0x0;
			registers[0x1d80 / 4] = 0x3;
			registers[NV4097_SET_WINDOW_OFFSET] = 0x0;
			registers[0x2bc / 4] = 0x0;
			registers[0x2c0 / 4] = 0xfff0000;
			registers[0x2c4 / 4] = 0xfff0000;
			registers[0x2c8 / 4] = 0xfff0000;
			registers[0x2cc / 4] = 0xfff0000;
			registers[0x2d0 / 4] = 0xfff0000;
			registers[0x2d4 / 4] = 0xfff0000;
			registers[0x2d8 / 4] = 0xfff0000;
			registers[0x2dc / 4] = 0xfff0000;
			registers[0x2e0 / 4] = 0xfff0000;
			registers[0x2e4 / 4] = 0xfff0000;
			registers[0x2e8 / 4] = 0xfff0000;
			registers[0x2ec / 4] = 0xfff0000;
			registers[0x2f0 / 4] = 0xfff0000;
			registers[0x2f4 / 4] = 0xfff0000;
			registers[0x2f8 / 4] = 0xfff0000;
			registers[0x2fc / 4] = 0xfff0000;
			registers[0x1d98 / 4] = 0xfff0000;
			registers[0x1d9c / 4] = 0xfff0000;
			registers[0x1da4 / 4] = 0x0;
			registers[NV4097_SET_CONTROL0] = 0x100000;
			registers[0x1454 / 4] = 0x0;
			registers[NV4097_SET_VERTEX_ATTRIB_OUTPUT_MASK] = 0x3fffff;
			registers[NV4097_SET_FREQUENCY_DIVIDER_OPERATION] = 0x0;
			registers[NV4097_SET_ATTRIB_COLOR] = 0x6144321;
			registers[NV4097_SET_ATTRIB_TEX_COORD] = 0xedcba987;
			registers[NV4097_SET_ATTRIB_TEX_COORD_EX] = 0x6f;
			registers[NV4097_SET_ATTRIB_UCLIP0] = 0x171615;
			registers[NV4097_SET_ATTRIB_UCLIP1] = 0x1b1a19;
			registers[NV4097_SET_TEX_COORD_CONTROL] = 0x0;
			registers[NV4097_SET_TEX_COORD_CONTROL + 1] = 0x0;
			registers[NV4097_SET_TEX_COORD_CONTROL + 2] = 0x0;
			registers[NV4097_SET_TEX_COORD_CONTROL + 3] = 0x0;
			registers[NV4097_SET_TEX_COORD_CONTROL + 4] = 0x0;
			registers[NV4097_SET_TEX_COORD_CONTROL + 5] = 0x0;
			registers[NV4097_SET_TEX_COORD_CONTROL + 6] = 0x0;
			registers[NV4097_SET_TEX_COORD_CONTROL + 7] = 0x0;
			registers[NV4097_SET_TEX_COORD_CONTROL + 8] = 0x0;
			registers[NV4097_SET_TEX_COORD_CONTROL + 9] = 0x0;
			registers[0xa0c / 4] = 0x0;
			registers[0xa60 / 4] = 0x0;
			registers[NV4097_SET_POLY_OFFSET_LINE_ENABLE] = 0x0;
			registers[NV4097_SET_POLY_OFFSET_FILL_ENABLE] = 0x0;
			registers[NV4097_SET_POLYGON_OFFSET_SCALE_FACTOR] = 0x0;
			registers[NV4097_SET_POLYGON_OFFSET_BIAS] = 0x0;
			registers[0x1428 / 4] = 0x1;
			registers[NV4097_SET_SHADER_WINDOW] = 0x1000;
			registers[0x1e94 / 4] = 0x11;
			registers[0x1450 / 4] = 0x80003;
			registers[0x1d64 / 4] = 0x2000000;
			registers[0x145c / 4] = 0x1;
			registers[NV4097_SET_REDUCE_DST_COLOR] = 0x1;
			registers[NV4097_SET_TEXTURE_CONTROL2] = 0x2dc8;
			registers[NV4097_SET_TEXTURE_CONTROL2 + 1] = 0x2dc8;
			registers[NV4097_SET_TEXTURE_CONTROL2 + 2] = 0x2dc8;
			registers[NV4097_SET_TEXTURE_CONTROL2 + 3] = 0x2dc8;
			registers[NV4097_SET_TEXTURE_CONTROL2 + 4] = 0x2dc8;
			registers[NV4097_SET_TEXTURE_CONTROL2 + 5] = 0x2dc8;
			registers[NV4097_SET_TEXTURE_CONTROL2 + 6] = 0x2dc8;
			registers[NV4097_SET_TEXTURE_CONTROL2 + 7] = 0x2dc8;
			registers[NV4097_SET_TEXTURE_CONTROL2 + 8] = 0x2dc8;
			registers[NV4097_SET_TEXTURE_CONTROL2 + 9] = 0x2dc8;
			registers[NV4097_SET_TEXTURE_CONTROL2 + 10] = 0x2dc8;
			registers[NV4097_SET_TEXTURE_CONTROL2 + 11] = 0x2dc8;
			registers[NV4097_SET_TEXTURE_CONTROL2 + 12] = 0x2dc8;
			registers[NV4097_SET_TEXTURE_CONTROL2 + 13] = 0x2dc8;
			registers[NV4097_SET_TEXTURE_CONTROL2 + 14] = 0x2dc8;
			registers[NV4097_SET_TEXTURE_CONTROL2 + 15] = 0x2dc8;
			registers[NV4097_SET_FOG_MODE] = 0x800;
			registers[NV4097_SET_FOG_PARAMS] = 0x0;
			registers[NV4097_SET_FOG_PARAMS + 1] = 0x0;
			registers[NV4097_SET_FOG_PARAMS + 2] = 0x0;
			registers[0x240 / 4] = 0xffff;
			registers[0x244 / 4] = 0x0;
			registers[0x248 / 4] = 0x0;
			registers[0x24c / 4] = 0x0;
			registers[NV4097_SET_ANISO_SPREAD] = 0x10101;
			registers[NV4097_SET_ANISO_SPREAD + 1] = 0x10101;
			registers[NV4097_SET_ANISO_SPREAD + 2] = 0x10101;
			registers[NV4097_SET_ANISO_SPREAD + 3] = 0x10101;
			registers[NV4097_SET_ANISO_SPREAD + 4] = 0x10101;
			registers[NV4097_SET_ANISO_SPREAD + 5] = 0x10101;
			registers[NV4097_SET_ANISO_SPREAD + 6] = 0x10101;
			registers[NV4097_SET_ANISO_SPREAD + 7] = 0x10101;
			registers[NV4097_SET_ANISO_SPREAD + 8] = 0x10101;
			registers[NV4097_SET_ANISO_SPREAD + 9] = 0x10101;
			registers[NV4097_SET_ANISO_SPREAD + 10] = 0x10101;
			registers[NV4097_SET_ANISO_SPREAD + 11] = 0x10101;
			registers[NV4097_SET_ANISO_SPREAD + 12] = 0x10101;
			registers[NV4097_SET_ANISO_SPREAD + 13] = 0x10101;
			registers[NV4097_SET_ANISO_SPREAD + 14] = 0x10101;
			registers[NV4097_SET_ANISO_SPREAD + 15] = 0x10101;
			registers[0x400 / 4] = 0x7421;
			registers[0x404 / 4] = 0x7421;
			registers[0x408 / 4] = 0x7421;
			registers[0x40c / 4] = 0x7421;
			registers[0x410 / 4] = 0x7421;
			registers[0x414 / 4] = 0x7421;
			registers[0x418 / 4] = 0x7421;
			registers[0x41c / 4] = 0x7421;
			registers[0x420 / 4] = 0x7421;
			registers[0x424 / 4] = 0x7421;
			registers[0x428 / 4] = 0x7421;
			registers[0x42c / 4] = 0x7421;
			registers[0x430 / 4] = 0x7421;
			registers[0x434 / 4] = 0x7421;
			registers[0x438 / 4] = 0x7421;
			registers[0x43c / 4] = 0x7421;
			registers[0x440 / 4] = 0x9aabaa98;
			registers[0x444 / 4] = 0x66666789;
			registers[0x448 / 4] = 0x98766666;
			registers[0x44c / 4] = 0x89aabaa9;
			registers[0x450 / 4] = 0x99999999;
			registers[0x454 / 4] = 0x88888889;
			registers[0x458 / 4] = 0x98888888;
			registers[0x45c / 4] = 0x99999999;
			registers[0x460 / 4] = 0x56676654;
			registers[0x464 / 4] = 0x33333345;
			registers[0x468 / 4] = 0x54333333;
			registers[0x46c / 4] = 0x45667665;
			registers[0x470 / 4] = 0xaabbba99;
			registers[0x474 / 4] = 0x66667899;
			registers[0x478 / 4] = 0x99876666;
			registers[0x47c / 4] = 0x99abbbaa;
			registers[NV4097_SET_VERTEX_DATA_BASE_OFFSET] = 0x0;
			registers[NV4097_SET_VERTEX_DATA_BASE_INDEX] = 0x0;
			registers[0xe000 / 4] = 0xcafebabe;
			registers[NV4097_SET_ALPHA_FUNC] = 0x207;
			registers[NV4097_SET_ALPHA_REF] = 0x0;
			registers[NV4097_SET_ALPHA_TEST_ENABLE] = 0x0;
			registers[NV4097_SET_BACK_STENCIL_FUNC] = 0x207;
			registers[NV4097_SET_BACK_STENCIL_FUNC_REF] = 0x0;
			registers[NV4097_SET_BACK_STENCIL_FUNC_MASK] = 0xff;
			registers[NV4097_SET_BACK_STENCIL_MASK] = 0xff;
			registers[NV4097_SET_BACK_STENCIL_OP_FAIL] = 0x1e00;
			registers[NV4097_SET_BACK_STENCIL_OP_ZFAIL] = 0x1e00;
			registers[NV4097_SET_BACK_STENCIL_OP_ZPASS] = 0x1e00;
			registers[NV4097_SET_BLEND_COLOR] = 0x0;
			registers[NV4097_SET_BLEND_COLOR2] = 0x0;
			registers[NV4097_SET_BLEND_ENABLE] = 0x0;
			registers[NV4097_SET_BLEND_ENABLE_MRT] = 0x0;
			registers[NV4097_SET_BLEND_EQUATION] = 0x80068006;
			registers[NV4097_SET_BLEND_FUNC_SFACTOR] = 0x10001;
			registers[NV4097_SET_BLEND_FUNC_DFACTOR] = 0x0;
			registers[NV4097_SET_ZSTENCIL_CLEAR_VALUE] = 0xffffff00;
			registers[NV4097_CLEAR_SURFACE] = 0x0;
			registers[NV4097_NO_OPERATION] = 0x0;
			registers[NV4097_SET_COLOR_MASK] = 0x1010101;
			registers[NV4097_SET_CULL_FACE_ENABLE] = 0x0;
			registers[NV4097_SET_CULL_FACE] = 0x405;
			registers[NV4097_SET_DEPTH_BOUNDS_MIN] = 0x0;
			registers[NV4097_SET_DEPTH_BOUNDS_MAX] = 0x3f800000;
			registers[NV4097_SET_DEPTH_BOUNDS_TEST_ENABLE] = 0x0;
			registers[NV4097_SET_DEPTH_FUNC] = 0x201;
			registers[NV4097_SET_DEPTH_MASK] = 0x1;
			registers[NV4097_SET_DEPTH_TEST_ENABLE] = 0x0;
			registers[NV4097_SET_DITHER_ENABLE] = 0x1;
			registers[NV4097_SET_SHADER_PACKER] = 0x0;
			registers[NV4097_SET_FREQUENCY_DIVIDER_OPERATION] = 0x0;
			registers[NV4097_SET_FRONT_FACE] = 0x901;
			registers[NV4097_SET_LINE_WIDTH] = 0x8;
			registers[NV4097_SET_LOGIC_OP_ENABLE] = 0x0;
			registers[NV4097_SET_LOGIC_OP] = 0x1503;
			registers[NV4097_SET_POINT_SIZE] = 0x3f800000;
			registers[NV4097_SET_POLY_OFFSET_FILL_ENABLE] = 0x0;
			registers[NV4097_SET_POLYGON_OFFSET_SCALE_FACTOR] = 0x0;
			registers[NV4097_SET_POLYGON_OFFSET_BIAS] = 0x0;
			registers[NV4097_SET_RESTART_INDEX_ENABLE] = 0x0;
			registers[NV4097_SET_RESTART_INDEX] = 0xffffffff;
			registers[NV4097_SET_SCISSOR_HORIZONTAL] = 0x10000000;
			registers[NV4097_SET_SCISSOR_VERTICAL] = 0x10000000;
			registers[NV4097_SET_SHADE_MODE] = 0x1d01;
			registers[NV4097_SET_STENCIL_FUNC] = 0x207;
			registers[NV4097_SET_STENCIL_FUNC_REF] = 0x0;
			registers[NV4097_SET_STENCIL_FUNC_MASK] = 0xff;
			registers[NV4097_SET_STENCIL_MASK] = 0xff;
			registers[NV4097_SET_STENCIL_OP_FAIL] = 0x1e00;
			registers[NV4097_SET_STENCIL_OP_ZFAIL] = 0x1e00;
			registers[NV4097_SET_STENCIL_OP_ZPASS] = 0x1e00;
			registers[NV4097_SET_STENCIL_TEST_ENABLE] = 0x0;
			registers[NV4097_SET_TEXTURE_ADDRESS] = 0x30101;
			registers[NV4097_SET_TEXTURE_BORDER_COLOR] = 0x0;
			registers[NV4097_SET_TEXTURE_CONTROL0] = 0x60000;
			registers[NV4097_SET_TEXTURE_FILTER] = 0x2052000;
			registers[NV4097_SET_TEXTURE_ADDRESS + 8] = 0x30101;
			registers[NV4097_SET_TEXTURE_BORDER_COLOR + 8] = 0x0;
			registers[NV4097_SET_TEXTURE_CONTROL0 + 8] = 0x60000;
			registers[NV4097_SET_TEXTURE_FILTER + 8] = 0x2052000;
			registers[NV4097_SET_TEXTURE_ADDRESS + 16] = 0x30101;
			registers[NV4097_SET_TEXTURE_BORDER_COLOR + 16] = 0x0;
			registers[NV4097_SET_TEXTURE_CONTROL0 + 16] = 0x60000;
			registers[NV4097_SET_TEXTURE_FILTER + 16] = 0x2052000;
			registers[NV4097_SET_TEXTURE_ADDRESS + 24] = 0x30101;
			registers[NV4097_SET_TEXTURE_BORDER_COLOR + 24] = 0x0;
			registers[NV4097_SET_TEXTURE_CONTROL0 + 24] = 0x60000;
			registers[NV4097_SET_TEXTURE_FILTER + 24] = 0x2052000;
			registers[NV4097_SET_TEXTURE_ADDRESS + 32] = 0x30101;
			registers[NV4097_SET_TEXTURE_BORDER_COLOR + 32] = 0x0;
			registers[NV4097_SET_TEXTURE_CONTROL0 + 32] = 0x60000;
			registers[NV4097_SET_TEXTURE_FILTER + 32] = 0x2052000;
			registers[NV4097_SET_TEXTURE_ADDRESS + 40] = 0x30101;
			registers[NV4097_SET_TEXTURE_BORDER_COLOR + 40] = 0x0;
			registers[NV4097_SET_TEXTURE_CONTROL0 + 40] = 0x60000;
			registers[NV4097_SET_TEXTURE_FILTER + 40] = 0x2052000;
			registers[NV4097_SET_TEXTURE_ADDRESS + 48] = 0x30101;
			registers[NV4097_SET_TEXTURE_BORDER_COLOR + 48] = 0x0;
			registers[NV4097_SET_TEXTURE_CONTROL0 + 48] = 0x60000;
			registers[NV4097_SET_TEXTURE_FILTER + 48] = 0x2052000;
			registers[NV4097_SET_TEXTURE_ADDRESS + 56] = 0x30101;
			registers[NV4097_SET_TEXTURE_BORDER_COLOR + 56] = 0x0;
			registers[NV4097_SET_TEXTURE_CONTROL0 + 56] = 0x60000;
			registers[NV4097_SET_TEXTURE_FILTER + 56] = 0x2052000;
			registers[NV4097_SET_TEXTURE_ADDRESS + 64] = 0x30101;
			registers[NV4097_SET_TEXTURE_BORDER_COLOR + 64] = 0x0;
			registers[NV4097_SET_TEXTURE_CONTROL0 + 64] = 0x60000;
			registers[NV4097_SET_TEXTURE_FILTER + 64] = 0x2052000;
			registers[NV4097_SET_TEXTURE_ADDRESS + 72] = 0x30101;
			registers[NV4097_SET_TEXTURE_BORDER_COLOR + 72] = 0x0;
			registers[NV4097_SET_TEXTURE_CONTROL0 + 72] = 0x60000;
			registers[NV4097_SET_TEXTURE_FILTER + 72] = 0x2052000;
			registers[NV4097_SET_TEXTURE_ADDRESS + 80] = 0x30101;
			registers[NV4097_SET_TEXTURE_BORDER_COLOR + 80] = 0x0;
			registers[NV4097_SET_TEXTURE_CONTROL0 + 80] = 0x60000;
			registers[NV4097_SET_TEXTURE_FILTER + 80] = 0x2052000;
			registers[NV4097_SET_TEXTURE_ADDRESS + 88] = 0x30101;
			registers[NV4097_SET_TEXTURE_BORDER_COLOR + 88] = 0x0;
			registers[NV4097_SET_TEXTURE_CONTROL0 + 88] = 0x60000;
			registers[NV4097_SET_TEXTURE_FILTER + 88] = 0x2052000;
			registers[NV4097_SET_TEXTURE_ADDRESS + 96] = 0x30101;
			registers[NV4097_SET_TEXTURE_BORDER_COLOR + 96] = 0x0;
			registers[NV4097_SET_TEXTURE_CONTROL0 + 96] = 0x60000;
			registers[NV4097_SET_TEXTURE_FILTER + 96] = 0x2052000;
			registers[NV4097_SET_TEXTURE_ADDRESS + 104] = 0x30101;
			registers[NV4097_SET_TEXTURE_BORDER_COLOR + 104] = 0x0;
			registers[NV4097_SET_TEXTURE_CONTROL0 + 104] = 0x60000;
			registers[NV4097_SET_TEXTURE_FILTER + 104] = 0x2052000;
			registers[NV4097_SET_TEXTURE_ADDRESS + 112] = 0x30101;
			registers[NV4097_SET_TEXTURE_BORDER_COLOR + 112] = 0x0;
			registers[NV4097_SET_TEXTURE_CONTROL0 + 112] = 0x60000;
			registers[NV4097_SET_TEXTURE_FILTER + 112] = 0x2052000;
			registers[NV4097_SET_TEXTURE_ADDRESS + 120] = 0x30101;
			registers[NV4097_SET_TEXTURE_BORDER_COLOR + 120] = 0x0;
			registers[NV4097_SET_TEXTURE_CONTROL0 + 120] = 0x60000;
			registers[NV4097_SET_TEXTURE_FILTER + 120] = 0x2052000;
			registers[NV4097_SET_TWO_SIDED_STENCIL_TEST_ENABLE] = 0x0;
			registers[NV4097_SET_VERTEX_DATA_ARRAY_FORMAT] = 0x2;
			registers[NV4097_SET_VERTEX_DATA_ARRAY_OFFSET] = 0x0;
			registers[NV4097_SET_VERTEX_DATA_ARRAY_FORMAT + 1] = 0x2;
			registers[NV4097_SET_VERTEX_DATA_ARRAY_OFFSET + 1] = 0x0;
			registers[NV4097_SET_VERTEX_DATA_ARRAY_FORMAT + 2] = 0x2;
			registers[NV4097_SET_VERTEX_DATA_ARRAY_OFFSET + 2] = 0x0;
			registers[NV4097_SET_VERTEX_DATA_ARRAY_FORMAT + 3] = 0x2;
			registers[NV4097_SET_VERTEX_DATA_ARRAY_OFFSET + 3] = 0x0;
			registers[NV4097_SET_VERTEX_DATA_ARRAY_FORMAT + 4] = 0x2;
			registers[NV4097_SET_VERTEX_DATA_ARRAY_OFFSET + 4] = 0x0;
			registers[NV4097_SET_VERTEX_DATA_ARRAY_FORMAT + 5] = 0x2;
			registers[NV4097_SET_VERTEX_DATA_ARRAY_OFFSET + 5] = 0x0;
			registers[NV4097_SET_VERTEX_DATA_ARRAY_FORMAT + 6] = 0x2;
			registers[NV4097_SET_VERTEX_DATA_ARRAY_OFFSET + 6] = 0x0;
			registers[NV4097_SET_VERTEX_DATA_ARRAY_FORMAT + 7] = 0x2;
			registers[NV4097_SET_VERTEX_DATA_ARRAY_OFFSET + 7] = 0x0;
			registers[NV4097_SET_VERTEX_DATA_ARRAY_FORMAT + 8] = 0x2;
			registers[NV4097_SET_VERTEX_DATA_ARRAY_OFFSET + 8] = 0x0;
			registers[NV4097_SET_VERTEX_DATA_ARRAY_FORMAT + 9] = 0x2;
			registers[NV4097_SET_VERTEX_DATA_ARRAY_OFFSET + 9] = 0x0;
			registers[NV4097_SET_VERTEX_DATA_ARRAY_FORMAT + 10] = 0x2;
			registers[NV4097_SET_VERTEX_DATA_ARRAY_OFFSET + 10] = 0x0;
			registers[NV4097_SET_VERTEX_DATA_ARRAY_FORMAT + 11] = 0x2;
			registers[NV4097_SET_VERTEX_DATA_ARRAY_OFFSET + 11] = 0x0;
			registers[NV4097_SET_VERTEX_DATA_ARRAY_FORMAT + 12] = 0x2;
			registers[NV4097_SET_VERTEX_DATA_ARRAY_OFFSET + 12] = 0x0;
			registers[NV4097_SET_VERTEX_DATA_ARRAY_FORMAT + 13] = 0x2;
			registers[NV4097_SET_VERTEX_DATA_ARRAY_OFFSET + 13] = 0x0;
			registers[NV4097_SET_VERTEX_DATA_ARRAY_FORMAT + 14] = 0x2;
			registers[NV4097_SET_VERTEX_DATA_ARRAY_OFFSET + 14] = 0x0;
			registers[NV4097_SET_VERTEX_DATA_ARRAY_FORMAT + 15] = 0x2;
			registers[NV4097_SET_VERTEX_DATA_ARRAY_OFFSET + 15] = 0x0;
			registers[NV4097_SET_VIEWPORT_HORIZONTAL] = 0x10000000;
			registers[NV4097_SET_VIEWPORT_VERTICAL] = 0x10000000;
			registers[NV4097_SET_CLIP_MIN] = 0x0;
			registers[NV4097_SET_CLIP_MAX] = 0x3f800000;
			registers[NV4097_SET_VIEWPORT_OFFSET + 0] = 0x45000000;
			registers[NV4097_SET_VIEWPORT_OFFSET + 1] = 0x45000000;
			registers[NV4097_SET_VIEWPORT_OFFSET + 2] = 0x3f000000;
			registers[NV4097_SET_VIEWPORT_OFFSET + 3] = 0x0;
			registers[NV4097_SET_VIEWPORT_SCALE + 0] = 0x45000000;
			registers[NV4097_SET_VIEWPORT_SCALE + 1] = 0x45000000;
			registers[NV4097_SET_VIEWPORT_SCALE + 2] = 0x3f000000;
			registers[NV4097_SET_VIEWPORT_SCALE + 3] = 0x0;
			// NOTE: Realhw emits this sequence twice, likely to work around a hardware bug. Similar behavior can be seen in other buggy register blocks
			//registers[NV4097_SET_VIEWPORT_OFFSET + 0] = 0x45000000;
			//registers[NV4097_SET_VIEWPORT_OFFSET + 1] = 0x45000000;
			//registers[NV4097_SET_VIEWPORT_OFFSET + 2] = 0x3f000000;
			//registers[NV4097_SET_VIEWPORT_OFFSET + 3] = 0x0;
			//registers[NV4097_SET_VIEWPORT_SCALE + 0] = 0x45000000;
			//registers[NV4097_SET_VIEWPORT_SCALE + 1] = 0x45000000;
			//registers[NV4097_SET_VIEWPORT_SCALE + 2] = 0x3f000000;
			//registers[NV4097_SET_VIEWPORT_SCALE + 3] = 0x0;
			registers[NV4097_SET_ANTI_ALIASING_CONTROL] = 0xffff0000;
			registers[NV4097_SET_BACK_POLYGON_MODE] = 0x1b02;
			registers[NV4097_SET_COLOR_CLEAR_VALUE] = 0x0;
			registers[NV4097_SET_COLOR_MASK_MRT] = 0x0;
			registers[NV4097_SET_FRONT_POLYGON_MODE] = 0x1b02;
			registers[NV4097_SET_LINE_SMOOTH_ENABLE] = 0x0;
			registers[NV4097_SET_LINE_STIPPLE] = 0x0;
			registers[NV4097_SET_POINT_PARAMS_ENABLE] = 0x0;
			registers[NV4097_SET_POINT_SPRITE_CONTROL] = 0x0;
			registers[NV4097_SET_POLY_SMOOTH_ENABLE] = 0x0;
			registers[NV4097_SET_POLYGON_STIPPLE] = 0x0;
			registers[NV4097_SET_RENDER_ENABLE] = 0x1000000;
			registers[NV4097_SET_USER_CLIP_PLANE_CONTROL] = 0x0;
			registers[NV4097_SET_VERTEX_ATTRIB_INPUT_MASK] = 0xffff;
			registers[NV4097_SET_ZPASS_PIXEL_COUNT_ENABLE] = 0x0;
			registers[NV4097_SET_VERTEX_TEXTURE_ADDRESS] = 0x101;
			registers[NV4097_SET_VERTEX_TEXTURE_BORDER_COLOR] = 0x0;
			registers[NV4097_SET_VERTEX_TEXTURE_CONTROL0] = 0x60000;
			registers[NV4097_SET_VERTEX_TEXTURE_FILTER] = 0x0;
			registers[NV4097_SET_VERTEX_TEXTURE_ADDRESS + 8] = 0x101;
			registers[NV4097_SET_VERTEX_TEXTURE_BORDER_COLOR + 8] = 0x0;
			registers[NV4097_SET_VERTEX_TEXTURE_CONTROL0 + 8] = 0x60000;
			registers[NV4097_SET_VERTEX_TEXTURE_FILTER + 8] = 0x0;
			registers[NV4097_SET_VERTEX_TEXTURE_ADDRESS + 16] = 0x101;
			registers[NV4097_SET_VERTEX_TEXTURE_BORDER_COLOR + 16] = 0x0;
			registers[NV4097_SET_VERTEX_TEXTURE_CONTROL0 + 16] = 0x60000;
			registers[NV4097_SET_VERTEX_TEXTURE_FILTER + 16] = 0x0;
			registers[NV4097_SET_VERTEX_TEXTURE_ADDRESS + 24] = 0x101;
			registers[NV4097_SET_VERTEX_TEXTURE_BORDER_COLOR + 24] = 0x0;
			registers[NV4097_SET_VERTEX_TEXTURE_CONTROL0 + 24] = 0x60000;
			registers[NV4097_SET_VERTEX_TEXTURE_FILTER + 24] = 0x0;
			registers[NV4097_SET_CYLINDRICAL_WRAP] = 0x0;
			registers[NV4097_SET_ZMIN_MAX_CONTROL] = 0x1;
			registers[NV4097_SET_TWO_SIDE_LIGHT_EN] = 0x0;
			registers[NV4097_SET_TRANSFORM_BRANCH_BITS] = 0x0;
			registers[NV4097_SET_NO_PARANOID_TEXTURE_FETCHES] = 0x0;
			registers[0x2000 / 4] = 0x31337303;
			registers[0x2180 / 4] = 0x66604200;
			registers[0x2184 / 4] = 0xfeed0001;
			registers[0x2188 / 4] = 0xfeed0000;
			registers[NV3062_SET_OBJECT] = 0x313371c3;
			registers[NV3062_SET_CONTEXT_DMA_NOTIFIES] = 0x66604200;
			registers[NV3062_SET_CONTEXT_DMA_IMAGE_SOURCE] = 0xfeed0000;
			registers[NV3062_SET_CONTEXT_DMA_IMAGE_DESTIN] = 0xfeed0000;
			registers[0xa000 / 4] = 0x31337808;
			registers[0xa180 / 4] = 0x66604200;
			registers[0xa184 / 4] = 0x0;
			registers[0xa188 / 4] = 0x0;
			registers[0xa18c / 4] = 0x0;
			registers[0xa190 / 4] = 0x0;
			registers[0xa194 / 4] = 0x0;
			registers[0xa198 / 4] = 0x0;
			registers[0xa19c / 4] = 0x313371c3;
			registers[0xa2fc / 4] = 0x3;
			registers[0xa300 / 4] = 0x4;
			registers[0x8000 / 4] = 0x31337a73;
			registers[0x8180 / 4] = 0x66604200;
			registers[0x8184 / 4] = 0xfeed0000;
			registers[0xc000 / 4] = 0x3137af00;
			registers[0xc180 / 4] = 0x66604200;
			registers[NV4097_SET_ZCULL_EN] = 0x3;
			registers[NV4097_SET_ZCULL_STATS_ENABLE] = 0x0;
			registers[NV4097_SET_ZCULL_CONTROL0] = 0x10;
			registers[NV4097_SET_ZCULL_CONTROL1] = 0x1000100;
			registers[NV4097_SET_SCULL_CONTROL] = 0xff000002;
			registers[NV4097_SET_TEXTURE_OFFSET] = 0x0;
			registers[NV4097_SET_TEXTURE_FORMAT] = 0x18429;
			registers[NV4097_SET_TEXTURE_IMAGE_RECT] = 0x80008;
			registers[NV4097_SET_TEXTURE_CONTROL3] = 0x100008;
			registers[NV4097_SET_TEXTURE_CONTROL1] = 0xaae4;
			registers[NV4097_SET_TEXTURE_OFFSET + 8] = 0x0;
			registers[NV4097_SET_TEXTURE_FORMAT + 8] = 0x18429;
			registers[NV4097_SET_TEXTURE_IMAGE_RECT + 8] = 0x80008;
			registers[NV4097_SET_TEXTURE_CONTROL3 + 1] = 0x100008;
			registers[NV4097_SET_TEXTURE_CONTROL1 + 8] = 0xaae4;
			registers[NV4097_SET_TEXTURE_OFFSET + 16] = 0x0;
			registers[NV4097_SET_TEXTURE_FORMAT + 16] = 0x18429;
			registers[NV4097_SET_TEXTURE_IMAGE_RECT + 16] = 0x80008;
			registers[NV4097_SET_TEXTURE_CONTROL3 + 2] = 0x100008;
			registers[NV4097_SET_TEXTURE_CONTROL1 + 16] = 0xaae4;
			registers[NV4097_SET_TEXTURE_OFFSET + 24] = 0x0;
			registers[NV4097_SET_TEXTURE_FORMAT + 24] = 0x18429;
			registers[NV4097_SET_TEXTURE_IMAGE_RECT + 24] = 0x80008;
			registers[NV4097_SET_TEXTURE_CONTROL3 + 3] = 0x100008;
			registers[NV4097_SET_TEXTURE_CONTROL1 + 24] = 0xaae4;
			registers[NV4097_SET_TEXTURE_OFFSET + 32] = 0x0;
			registers[NV4097_SET_TEXTURE_FORMAT + 32] = 0x18429;
			registers[NV4097_SET_TEXTURE_IMAGE_RECT + 32] = 0x80008;
			registers[NV4097_SET_TEXTURE_CONTROL3 + 4] = 0x100008;
			registers[NV4097_SET_TEXTURE_CONTROL1 + 32] = 0xaae4;
			registers[NV4097_SET_TEXTURE_OFFSET + 40] = 0x0;
			registers[NV4097_SET_TEXTURE_FORMAT + 40] = 0x18429;
			registers[NV4097_SET_TEXTURE_IMAGE_RECT + 40] = 0x80008;
			registers[NV4097_SET_TEXTURE_CONTROL3 + 5] = 0x100008;
			registers[NV4097_SET_TEXTURE_CONTROL1 + 40] = 0xaae4;
			registers[NV4097_SET_TEXTURE_OFFSET + 48] = 0x0;
			registers[NV4097_SET_TEXTURE_FORMAT + 48] = 0x18429;
			registers[NV4097_SET_TEXTURE_IMAGE_RECT + 48] = 0x80008;
			registers[NV4097_SET_TEXTURE_CONTROL3 + 6] = 0x100008;
			registers[NV4097_SET_TEXTURE_CONTROL1 + 48] = 0xaae4;
			registers[NV4097_SET_TEXTURE_OFFSET + 56] = 0x0;
			registers[NV4097_SET_TEXTURE_FORMAT + 56] = 0x18429;
			registers[NV4097_SET_TEXTURE_IMAGE_RECT + 56] = 0x80008;
			registers[NV4097_SET_TEXTURE_CONTROL3 + 7] = 0x100008;
			registers[NV4097_SET_TEXTURE_CONTROL1 + 56] = 0xaae4;
			registers[NV4097_SET_TEXTURE_OFFSET + 64] = 0x0;
			registers[NV4097_SET_TEXTURE_FORMAT + 64] = 0x18429;
			registers[NV4097_SET_TEXTURE_IMAGE_RECT + 64] = 0x80008;
			registers[NV4097_SET_TEXTURE_CONTROL3 + 8] = 0x100008;
			registers[NV4097_SET_TEXTURE_CONTROL1 + 64] = 0xaae4;
			registers[NV4097_SET_TEXTURE_OFFSET + 72] = 0x0;
			registers[NV4097_SET_TEXTURE_FORMAT + 72] = 0x18429;
			registers[NV4097_SET_TEXTURE_IMAGE_RECT + 72] = 0x80008;
			registers[NV4097_SET_TEXTURE_CONTROL3 + 9] = 0x100008;
			registers[NV4097_SET_TEXTURE_CONTROL1 + 72] = 0xaae4;
			registers[NV4097_SET_TEXTURE_OFFSET + 80] = 0x0;
			registers[NV4097_SET_TEXTURE_FORMAT + 80] = 0x18429;
			registers[NV4097_SET_TEXTURE_IMAGE_RECT + 80] = 0x80008;
			registers[NV4097_SET_TEXTURE_CONTROL3 + 10] = 0x100008;
			registers[NV4097_SET_TEXTURE_CONTROL1 + 80] = 0xaae4;
			registers[NV4097_SET_TEXTURE_OFFSET + 88] = 0x0;
			registers[NV4097_SET_TEXTURE_FORMAT + 88] = 0x18429;
			registers[NV4097_SET_TEXTURE_IMAGE_RECT + 88] = 0x80008;
			registers[NV4097_SET_TEXTURE_CONTROL3 + 11] = 0x100008;
			registers[NV4097_SET_TEXTURE_CONTROL1 + 88] = 0xaae4;
			registers[NV4097_SET_TEXTURE_OFFSET + 96] = 0x0;
			registers[NV4097_SET_TEXTURE_FORMAT + 96] = 0x18429;
			registers[NV4097_SET_TEXTURE_IMAGE_RECT + 96] = 0x80008;
			registers[NV4097_SET_TEXTURE_CONTROL3 + 12] = 0x100008;
			registers[NV4097_SET_TEXTURE_CONTROL1 + 96] = 0xaae4;
			registers[NV4097_SET_TEXTURE_OFFSET + 104] = 0x0;
			registers[NV4097_SET_TEXTURE_FORMAT + 104] = 0x18429;
			registers[NV4097_SET_TEXTURE_IMAGE_RECT + 104] = 0x80008;
			registers[NV4097_SET_TEXTURE_CONTROL3 + 13] = 0x100008;
			registers[NV4097_SET_TEXTURE_CONTROL1 + 104] = 0xaae4;
			registers[NV4097_SET_TEXTURE_OFFSET + 112] = 0x0;
			registers[NV4097_SET_TEXTURE_FORMAT + 112] = 0x18429;
			registers[NV4097_SET_TEXTURE_IMAGE_RECT + 112] = 0x80008;
			registers[NV4097_SET_TEXTURE_CONTROL3 + 14] = 0x100008;
			registers[NV4097_SET_TEXTURE_CONTROL1 + 112] = 0xaae4;
			registers[NV4097_SET_TEXTURE_OFFSET + 120] = 0x0;
			registers[NV4097_SET_TEXTURE_FORMAT + 120] = 0x18429;
			registers[NV4097_SET_TEXTURE_IMAGE_RECT + 120] = 0x80008;
			registers[NV4097_SET_TEXTURE_CONTROL3 + 15] = 0x100008;
			registers[NV4097_SET_TEXTURE_CONTROL1 + 120] = 0xaae4;
			registers[NV4097_SET_VERTEX_TEXTURE_OFFSET] = 0x0;
			registers[NV4097_SET_VERTEX_TEXTURE_FORMAT] = 0x1bc21;
			registers[NV4097_SET_VERTEX_TEXTURE_CONTROL3] = 0x8;
			registers[NV4097_SET_VERTEX_TEXTURE_IMAGE_RECT] = 0x80008;
			registers[NV4097_SET_VERTEX_TEXTURE_OFFSET + 8] = 0x0;
			registers[NV4097_SET_VERTEX_TEXTURE_FORMAT + 8] = 0x1bc21;
			registers[NV4097_SET_VERTEX_TEXTURE_CONTROL3 + 8] = 0x8;
			registers[NV4097_SET_VERTEX_TEXTURE_IMAGE_RECT + 8] = 0x80008;
			registers[NV4097_SET_VERTEX_TEXTURE_OFFSET + 16] = 0x0;
			registers[NV4097_SET_VERTEX_TEXTURE_FORMAT + 16] = 0x1bc21;
			registers[NV4097_SET_VERTEX_TEXTURE_CONTROL3 + 16] = 0x8;
			registers[NV4097_SET_VERTEX_TEXTURE_IMAGE_RECT + 16] = 0x80008;
			registers[NV4097_SET_VERTEX_TEXTURE_OFFSET + 24] = 0x0;
			registers[NV4097_SET_VERTEX_TEXTURE_FORMAT + 24] = 0x1bc21;
			registers[NV4097_SET_VERTEX_TEXTURE_CONTROL3 + 24] = 0x8;
			registers[NV4097_SET_VERTEX_TEXTURE_IMAGE_RECT + 24] = 0x80008;
			registers[0x230c / 4] = 0x0;
			registers[0x2310 / 4] = 0x0;
			registers[0x2314 / 4] = 0x0;
			registers[0x2318 / 4] = 0x0;
			registers[0x231c / 4] = 0x0;
			registers[0x2320 / 4] = 0x0;
			registers[0x2324 / 4] = 0x101;
			registers[NV3062_SET_COLOR_FORMAT] = 0xa;
			registers[NV3062_SET_PITCH] = 0x400040;
			registers[NV3062_SET_OFFSET_SOURCE] = 0x0;
			registers[NV3062_SET_OFFSET_DESTIN] = 0x0;
			registers[0x8300 / 4] = 0x1000a;
			registers[0x8304 / 4] = 0x0;
			registers[0xc184 / 4] = 0xfeed0000;
			registers[0xc198 / 4] = 0x313371c3;
			registers[0xc2fc / 4] = 0x1;
			registers[0xc300 / 4] = 0x3;
			registers[0xc304 / 4] = 0x3;
			registers[0xc308 / 4] = 0x0;
			registers[0xc30c / 4] = 0x0;
			registers[0xc310 / 4] = 0x0;
			registers[0xc314 / 4] = 0x0;
			registers[0xc318 / 4] = 0x0;
			registers[0xc31c / 4] = 0x0;
			registers[0xc400 / 4] = 0x10002;
			registers[0xc404 / 4] = 0x10000;
			registers[0xc408 / 4] = 0x0;
			registers[0xc40c / 4] = 0x0;
			registers[NV308A_POINT] = 0x0;
			registers[NV308A_SIZE_OUT] = 0x0;
			registers[NV308A_SIZE_IN] = 0x0;
			registers[NV406E_SET_REFERENCE] = get_current_renderer()->ctrl->ref = 0xffffffff;
		}
	}

	void rsx_state::reset()
	{
		// TODO: Name unnamed registers and constants, better group methods
		registers[NV406E_SET_CONTEXT_DMA_SEMAPHORE] = 0x56616661;

		registers[NV4097_SET_OBJECT] = 0x31337000;
		registers[NV4097_SET_CONTEXT_DMA_NOTIFIES] = 0x66604200;
		registers[NV4097_SET_CONTEXT_DMA_A] = 0xfeed0000;
		registers[NV4097_SET_CONTEXT_DMA_B] = 0xfeed0001;
		registers[NV4097_SET_CONTEXT_DMA_COLOR_B] = 0xfeed0000;
		registers[NV4097_SET_CONTEXT_DMA_STATE] = 0x0;
		registers[NV4097_SET_CONTEXT_DMA_COLOR_A] = 0xfeed0000;
		registers[NV4097_SET_CONTEXT_DMA_ZETA] = 0xfeed0000;
		registers[NV4097_SET_CONTEXT_DMA_VERTEX_A] = 0xfeed0000;
		registers[NV4097_SET_CONTEXT_DMA_VERTEX_B] = 0xfeed0001;
		registers[NV4097_SET_CONTEXT_DMA_SEMAPHORE] = 0x66606660;
		registers[NV4097_SET_CONTEXT_DMA_REPORT] = 0x66626660;
		registers[NV4097_SET_CONTEXT_DMA_CLIP_ID] = 0x0;
		registers[NV4097_SET_CONTEXT_DMA_CULL_DATA] = 0x0;
		registers[NV4097_SET_CONTEXT_DMA_COLOR_C] = 0xfeed0000;
		registers[NV4097_SET_CONTEXT_DMA_COLOR_D] = 0xfeed0000;
		registers[NV406E_SET_CONTEXT_DMA_SEMAPHORE] = 0x66616661;
		registers[NV4097_SET_SURFACE_CLIP_HORIZONTAL] = 0x0;
		registers[NV4097_SET_SURFACE_CLIP_VERTICAL] = 0x0;
		registers[NV4097_SET_SURFACE_FORMAT] = 0x121;
		registers[NV4097_SET_SURFACE_PITCH_A] = 0x40;
		registers[NV4097_SET_SURFACE_COLOR_AOFFSET] = 0x0;
		registers[NV4097_SET_SURFACE_ZETA_OFFSET] = 0x0;
		registers[NV4097_SET_SURFACE_COLOR_BOFFSET] = 0x0;
		registers[NV4097_SET_SURFACE_PITCH_B] = 0x40;
		registers[NV4097_SET_SURFACE_COLOR_TARGET] = 0x1;
		registers[0x224 / 4] = 0x80;
		registers[0x228 / 4] = 0x100;
		registers[NV4097_SET_SURFACE_PITCH_Z] = 0x40;
		registers[0x230 / 4] = 0x0;
		registers[NV4097_SET_SURFACE_PITCH_C] = 0x40;
		registers[NV4097_SET_SURFACE_PITCH_D] = 0x40;
		registers[NV4097_SET_SURFACE_COLOR_COFFSET] = 0x0;
		registers[NV4097_SET_SURFACE_COLOR_DOFFSET] = 0x0;
		registers[0x1d80 / 4] = 0x3;
		registers[NV4097_SET_WINDOW_OFFSET] = 0x0;
		registers[0x02bc / 4] = 0x0;
		registers[0x02c0 / 4] = 0xfff0000;
		registers[0x02c4 / 4] = 0xfff0000;
		registers[0x02c8 / 4] = 0xfff0000;
		registers[0x02cc / 4] = 0xfff0000;
		registers[0x02d0 / 4] = 0xfff0000;
		registers[0x02d4 / 4] = 0xfff0000;
		registers[0x02d8 / 4] = 0xfff0000;
		registers[0x02dc / 4] = 0xfff0000;
		registers[0x02e0 / 4] = 0xfff0000;
		registers[0x02e4 / 4] = 0xfff0000;
		registers[0x02e8 / 4] = 0xfff0000;
		registers[0x02ec / 4] = 0xfff0000;
		registers[0x02f0 / 4] = 0xfff0000;
		registers[0x02f4 / 4] = 0xfff0000;
		registers[0x02f8 / 4] = 0xfff0000;
		registers[0x02fc / 4] = 0xfff0000;
		registers[0x1d98 / 4] = 0xfff0000;
		registers[0x1d9c / 4] = 0xfff0000;
		registers[0x1da4 / 4] = 0x0;
		registers[NV4097_SET_CONTROL0] = 0x100000;
		registers[0x1454 / 4] = 0x0;
		registers[NV4097_SET_VERTEX_ATTRIB_OUTPUT_MASK] = 0x3fffff;
		registers[NV4097_SET_FREQUENCY_DIVIDER_OPERATION] = 0x0;
		registers[NV4097_SET_ATTRIB_COLOR] = 0x6144321;
		registers[NV4097_SET_ATTRIB_TEX_COORD] = 0xedcba987;
		registers[NV4097_SET_ATTRIB_TEX_COORD_EX] = 0x6f;
		registers[NV4097_SET_ATTRIB_UCLIP0] = 0x171615;
		registers[NV4097_SET_ATTRIB_UCLIP1] = 0x1b1a19;
		registers[NV4097_SET_TEX_COORD_CONTROL] = 0x0;
		registers[NV4097_SET_TEX_COORD_CONTROL + 1] = 0x0;
		registers[NV4097_SET_TEX_COORD_CONTROL + 2] = 0x0;
		registers[NV4097_SET_TEX_COORD_CONTROL + 3] = 0x0;
		registers[NV4097_SET_TEX_COORD_CONTROL + 4] = 0x0;
		registers[NV4097_SET_TEX_COORD_CONTROL + 5] = 0x0;
		registers[NV4097_SET_TEX_COORD_CONTROL + 6] = 0x0;
		registers[NV4097_SET_TEX_COORD_CONTROL + 7] = 0x0;
		registers[NV4097_SET_TEX_COORD_CONTROL + 8] = 0x0;
		registers[NV4097_SET_TEX_COORD_CONTROL + 9] = 0x0;
		registers[0xa0c / 4] = 0x0;
		registers[0xa60 / 4] = 0x0;
		registers[NV4097_SET_POLY_OFFSET_LINE_ENABLE] = 0x0;
		registers[NV4097_SET_POLY_OFFSET_FILL_ENABLE] = 0x0;
		registers[NV4097_SET_POLYGON_OFFSET_SCALE_FACTOR] = 0x0;
		registers[NV4097_SET_POLYGON_OFFSET_BIAS] = 0x0;
		registers[0x1428 / 4] = 0x1;
		registers[NV4097_SET_SHADER_WINDOW] = 0x1000;
		registers[0x1e94 / 4] = 0x11;
		registers[0x1450 / 4] = 0x80003;
		registers[0x1d64 / 4] = 0x2000000;
		registers[0x145c / 4] = 0x1;
		registers[NV4097_SET_REDUCE_DST_COLOR] = 0x1;
		registers[NV4097_SET_TEXTURE_CONTROL2] = 0x2dc8;
		registers[NV4097_SET_TEXTURE_CONTROL2 + 1] = 0x2dc8;
		registers[NV4097_SET_TEXTURE_CONTROL2 + 2] = 0x2dc8;
		registers[NV4097_SET_TEXTURE_CONTROL2 + 3] = 0x2dc8;
		registers[NV4097_SET_TEXTURE_CONTROL2 + 4] = 0x2dc8;
		registers[NV4097_SET_TEXTURE_CONTROL2 + 5] = 0x2dc8;
		registers[NV4097_SET_TEXTURE_CONTROL2 + 6] = 0x2dc8;
		registers[NV4097_SET_TEXTURE_CONTROL2 + 7] = 0x2dc8;
		registers[NV4097_SET_TEXTURE_CONTROL2 + 8] = 0x2dc8;
		registers[NV4097_SET_TEXTURE_CONTROL2 + 9] = 0x2dc8;
		registers[NV4097_SET_TEXTURE_CONTROL2 + 10] = 0x2dc8;
		registers[NV4097_SET_TEXTURE_CONTROL2 + 11] = 0x2dc8;
		registers[NV4097_SET_TEXTURE_CONTROL2 + 12] = 0x2dc8;
		registers[NV4097_SET_TEXTURE_CONTROL2 + 13] = 0x2dc8;
		registers[NV4097_SET_TEXTURE_CONTROL2 + 14] = 0x2dc8;
		registers[NV4097_SET_TEXTURE_CONTROL2 + 15] = 0x2dc8;
		registers[NV4097_SET_FOG_MODE] = 0x800;
		registers[NV4097_SET_FOG_PARAMS] = 0x0;
		registers[NV4097_SET_FOG_PARAMS + 1] = 0x0;
		registers[NV4097_SET_FOG_PARAMS + 2] = 0x0;
		registers[0x240 / 4] = 0xffff;
		registers[0x244 / 4] = 0x0;
		registers[0x248 / 4] = 0x0;
		registers[0x24c / 4] = 0x0;
		registers[NV4097_SET_ANISO_SPREAD] = 0x10101;
		registers[NV4097_SET_ANISO_SPREAD + 1] = 0x10101;
		registers[NV4097_SET_ANISO_SPREAD + 2] = 0x10101;
		registers[NV4097_SET_ANISO_SPREAD + 3] = 0x10101;
		registers[NV4097_SET_ANISO_SPREAD + 4] = 0x10101;
		registers[NV4097_SET_ANISO_SPREAD + 5] = 0x10101;
		registers[NV4097_SET_ANISO_SPREAD + 6] = 0x10101;
		registers[NV4097_SET_ANISO_SPREAD + 7] = 0x10101;
		registers[NV4097_SET_ANISO_SPREAD + 8] = 0x10101;
		registers[NV4097_SET_ANISO_SPREAD + 9] = 0x10101;
		registers[NV4097_SET_ANISO_SPREAD + 10] = 0x10101;
		registers[NV4097_SET_ANISO_SPREAD + 11] = 0x10101;
		registers[NV4097_SET_ANISO_SPREAD + 12] = 0x10101;
		registers[NV4097_SET_ANISO_SPREAD + 13] = 0x10101;
		registers[NV4097_SET_ANISO_SPREAD + 14] = 0x10101;
		registers[NV4097_SET_ANISO_SPREAD + 15] = 0x10101;
		registers[0x400 / 4] = 0x7421;
		registers[0x404 / 4] = 0x7421;
		registers[0x408 / 4] = 0x7421;
		registers[0x40c / 4] = 0x7421;
		registers[0x410 / 4] = 0x7421;
		registers[0x414 / 4] = 0x7421;
		registers[0x418 / 4] = 0x7421;
		registers[0x41c / 4] = 0x7421;
		registers[0x420 / 4] = 0x7421;
		registers[0x424 / 4] = 0x7421;
		registers[0x428 / 4] = 0x7421;
		registers[0x42c / 4] = 0x7421;
		registers[0x430 / 4] = 0x7421;
		registers[0x434 / 4] = 0x7421;
		registers[0x438 / 4] = 0x7421;
		registers[0x43c / 4] = 0x7421;
		registers[0x440 / 4] = 0x9aabaa98;
		registers[0x444 / 4] = 0x66666789;
		registers[0x448 / 4] = 0x98766666;
		registers[0x44c / 4] = 0x89aabaa9;
		registers[0x450 / 4] = 0x99999999;
		registers[0x454 / 4] = 0x88888889;
		registers[0x458 / 4] = 0x98888888;
		registers[0x45c / 4] = 0x99999999;
		registers[0x460 / 4] = 0x56676654;
		registers[0x464 / 4] = 0x33333345;
		registers[0x468 / 4] = 0x54333333;
		registers[0x46c / 4] = 0x45667665;
		registers[0x470 / 4] = 0xaabbba99;
		registers[0x474 / 4] = 0x66667899;
		registers[0x478 / 4] = 0x99876666;
		registers[0x47c / 4] = 0x99abbbaa;
		registers[NV4097_SET_VERTEX_DATA_BASE_OFFSET] = 0x0;
		registers[NV4097_SET_VERTEX_DATA_BASE_INDEX] = 0x0;
		registers[GCM_SET_DRIVER_OBJECT] = 0xcafebabe;
		registers[NV4097_SET_ALPHA_FUNC] = 0x207;
		registers[NV4097_SET_ALPHA_REF] = 0x0;
		registers[NV4097_SET_ALPHA_TEST_ENABLE] = 0x0;
		registers[NV4097_SET_BACK_STENCIL_FUNC] = 0x207;
		registers[NV4097_SET_BACK_STENCIL_FUNC_REF] = 0x0;
		registers[NV4097_SET_BACK_STENCIL_FUNC_MASK] = 0xff;
		registers[NV4097_SET_BACK_STENCIL_MASK] = 0xff;
		registers[NV4097_SET_BACK_STENCIL_OP_FAIL] = 0x1e00;
		registers[NV4097_SET_BACK_STENCIL_OP_ZFAIL] = 0x1e00;
		registers[NV4097_SET_BACK_STENCIL_OP_ZPASS] = 0x1e00;
		registers[NV4097_SET_BLEND_COLOR] = 0x0;
		registers[NV4097_SET_BLEND_COLOR2] = 0x0;
		registers[NV4097_SET_BLEND_ENABLE] = 0x0;
		registers[NV4097_SET_BLEND_ENABLE_MRT] = 0x0;
		registers[NV4097_SET_BLEND_EQUATION] = 0x80068006;
		registers[NV4097_SET_BLEND_FUNC_SFACTOR] = 0x10001;
		registers[NV4097_SET_BLEND_FUNC_DFACTOR] = 0x0;
		registers[NV4097_SET_ZSTENCIL_CLEAR_VALUE] = 0xffffff00;
		registers[NV4097_CLEAR_SURFACE] = 0x0;
		registers[NV4097_NO_OPERATION] = 0x0;
		registers[NV4097_SET_COLOR_MASK] = 0x1010101;
		registers[NV4097_SET_CULL_FACE_ENABLE] = 0x0;
		registers[NV4097_SET_CULL_FACE] = 0x405;
		registers[NV4097_SET_DEPTH_BOUNDS_MIN] = 0x0;
		registers[NV4097_SET_DEPTH_BOUNDS_MAX] = 0x3f800000;
		registers[NV4097_SET_DEPTH_BOUNDS_TEST_ENABLE] = 0x0;
		registers[NV4097_SET_DEPTH_FUNC] = 0x201;
		registers[NV4097_SET_DEPTH_MASK] = 0x1;
		registers[NV4097_SET_DEPTH_TEST_ENABLE] = 0x0;
		registers[NV4097_SET_DITHER_ENABLE] = 0x1;
		registers[NV4097_SET_SHADER_PACKER] = 0x0;
		registers[NV4097_SET_FREQUENCY_DIVIDER_OPERATION] = 0x0;
		registers[NV4097_SET_FRONT_FACE] = 0x901;
		registers[NV4097_SET_LINE_WIDTH] = 0x8;
		registers[NV4097_SET_LOGIC_OP_ENABLE] = 0x0;
		registers[NV4097_SET_LOGIC_OP] = 0x1503;
		registers[NV4097_SET_POINT_SIZE] = 0x3f800000;
		registers[NV4097_SET_POLY_OFFSET_FILL_ENABLE] = 0x0;
		registers[NV4097_SET_POLYGON_OFFSET_SCALE_FACTOR] = 0x0;
		registers[NV4097_SET_POLYGON_OFFSET_BIAS] = 0x0;
		registers[NV4097_SET_RESTART_INDEX_ENABLE] = 0x0;
		registers[NV4097_SET_RESTART_INDEX] = 0xffffffff;
		registers[NV4097_SET_SCISSOR_HORIZONTAL] = 0x10000000;
		registers[NV4097_SET_SCISSOR_VERTICAL] = 0x10000000;
		registers[NV4097_SET_SHADE_MODE] = 0x1d01;
		registers[NV4097_SET_STENCIL_FUNC] = 0x207;
		registers[NV4097_SET_STENCIL_FUNC_REF] = 0x0;
		registers[NV4097_SET_STENCIL_FUNC_MASK] = 0xff;
		registers[NV4097_SET_STENCIL_MASK] = 0xff;
		registers[NV4097_SET_STENCIL_OP_FAIL] = 0x1e00;
		registers[NV4097_SET_STENCIL_OP_ZFAIL] = 0x1e00;
		registers[NV4097_SET_STENCIL_OP_ZPASS] = 0x1e00;
		registers[NV4097_SET_STENCIL_TEST_ENABLE] = 0x0;
		registers[NV4097_SET_TEXTURE_ADDRESS] = 0x30101;
		registers[NV4097_SET_TEXTURE_BORDER_COLOR] = 0x0;
		registers[NV4097_SET_TEXTURE_CONTROL0] = 0x60000;
		registers[NV4097_SET_TEXTURE_FILTER] = 0x2052000;
		registers[NV4097_SET_TEXTURE_ADDRESS + 8] = 0x30101;
		registers[NV4097_SET_TEXTURE_BORDER_COLOR + 8] = 0x0;
		registers[NV4097_SET_TEXTURE_CONTROL0 + 8] = 0x60000;
		registers[NV4097_SET_TEXTURE_FILTER + 8] = 0x2052000;
		registers[NV4097_SET_TEXTURE_ADDRESS + 8] = 0x30101;
		registers[NV4097_SET_TEXTURE_BORDER_COLOR + 16] = 0x0;
		registers[NV4097_SET_TEXTURE_CONTROL0 + 16] = 0x60000;
		registers[NV4097_SET_TEXTURE_FILTER + 16] = 0x2052000;
		registers[NV4097_SET_TEXTURE_ADDRESS + 24] = 0x30101;
		registers[NV4097_SET_TEXTURE_BORDER_COLOR + 24] = 0x0;
		registers[NV4097_SET_TEXTURE_CONTROL0 + 24] = 0x60000;
		registers[NV4097_SET_TEXTURE_FILTER + 24] = 0x2052000;
		registers[NV4097_SET_TEXTURE_ADDRESS + 32] = 0x30101;
		registers[NV4097_SET_TEXTURE_BORDER_COLOR + 32] = 0x0;
		registers[NV4097_SET_TEXTURE_CONTROL0 + 32] = 0x60000;
		registers[NV4097_SET_TEXTURE_FILTER + 32] = 0x2052000;
		registers[NV4097_SET_TEXTURE_ADDRESS + 40] = 0x30101;
		registers[NV4097_SET_TEXTURE_BORDER_COLOR + 40] = 0x0;
		registers[NV4097_SET_TEXTURE_CONTROL0 + 40] = 0x60000;
		registers[NV4097_SET_TEXTURE_FILTER + 40] = 0x2052000;
		registers[NV4097_SET_TEXTURE_ADDRESS + 48] = 0x30101;
		registers[NV4097_SET_TEXTURE_BORDER_COLOR + 48] = 0x0;
		registers[NV4097_SET_TEXTURE_CONTROL0 + 48] = 0x60000;
		registers[NV4097_SET_TEXTURE_FILTER + 48] = 0x2052000;
		registers[NV4097_SET_TEXTURE_ADDRESS + 56] = 0x30101;
		registers[NV4097_SET_TEXTURE_BORDER_COLOR + 56] = 0x0;
		registers[NV4097_SET_TEXTURE_CONTROL0 + 56] = 0x60000;
		registers[NV4097_SET_TEXTURE_FILTER + 56] = 0x2052000;
		registers[NV4097_SET_TEXTURE_ADDRESS + 64] = 0x30101;
		registers[NV4097_SET_TEXTURE_BORDER_COLOR + 64] = 0x0;
		registers[NV4097_SET_TEXTURE_CONTROL0 + 64] = 0x60000;
		registers[NV4097_SET_TEXTURE_FILTER + 64] = 0x2052000;
		registers[NV4097_SET_TEXTURE_ADDRESS + 72] = 0x30101;
		registers[NV4097_SET_TEXTURE_BORDER_COLOR + 72] = 0x0;
		registers[NV4097_SET_TEXTURE_CONTROL0 + 72] = 0x60000;
		registers[NV4097_SET_TEXTURE_FILTER + 72] = 0x2052000;
		registers[NV4097_SET_TEXTURE_ADDRESS + 80] = 0x30101;
		registers[NV4097_SET_TEXTURE_BORDER_COLOR + 80] = 0x0;
		registers[NV4097_SET_TEXTURE_CONTROL0 + 80] = 0x60000;
		registers[NV4097_SET_TEXTURE_FILTER + 80] = 0x2052000;
		registers[NV4097_SET_TEXTURE_ADDRESS + 88] = 0x30101;
		registers[NV4097_SET_TEXTURE_BORDER_COLOR + 88] = 0x0;
		registers[NV4097_SET_TEXTURE_CONTROL0 + 88] = 0x60000;
		registers[NV4097_SET_TEXTURE_FILTER + 88] = 0x2052000;
		registers[NV4097_SET_TEXTURE_ADDRESS + 96] = 0x30101;
		registers[NV4097_SET_TEXTURE_BORDER_COLOR + 96] = 0x0;
		registers[NV4097_SET_TEXTURE_CONTROL0 + 96] = 0x60000;
		registers[NV4097_SET_TEXTURE_FILTER + 96] = 0x2052000;
		registers[NV4097_SET_TEXTURE_ADDRESS + 104] = 0x30101;
		registers[NV4097_SET_TEXTURE_BORDER_COLOR + 104] = 0x0;
		registers[NV4097_SET_TEXTURE_CONTROL0 + 104] = 0x60000;
		registers[NV4097_SET_TEXTURE_FILTER + 104] = 0x2052000;
		registers[NV4097_SET_TEXTURE_ADDRESS + 112] = 0x30101;
		registers[NV4097_SET_TEXTURE_BORDER_COLOR + 112] = 0x0;
		registers[NV4097_SET_TEXTURE_CONTROL0 + 112] = 0x60000;
		registers[NV4097_SET_TEXTURE_FILTER + 112] = 0x2052000;
		registers[NV4097_SET_TEXTURE_ADDRESS + 120] = 0x30101;
		registers[NV4097_SET_TEXTURE_BORDER_COLOR + 120] = 0x0;
		registers[NV4097_SET_TEXTURE_CONTROL0 + 120] = 0x60000;
		registers[NV4097_SET_TEXTURE_FILTER + 120] = 0x2052000;
		registers[NV4097_SET_TWO_SIDED_STENCIL_TEST_ENABLE] = 0x0;
		registers[NV4097_SET_VERTEX_DATA_ARRAY_FORMAT] = 0x2;
		registers[NV4097_SET_VERTEX_DATA_ARRAY_OFFSET] = 0x0;
		registers[NV4097_SET_VERTEX_DATA_ARRAY_FORMAT + 1] = 0x2;
		registers[NV4097_SET_VERTEX_DATA_ARRAY_OFFSET + 1] = 0x0;
		registers[NV4097_SET_VERTEX_DATA_ARRAY_FORMAT + 2] = 0x2;
		registers[NV4097_SET_VERTEX_DATA_ARRAY_OFFSET + 2] = 0x0;
		registers[NV4097_SET_VERTEX_DATA_ARRAY_FORMAT + 3] = 0x2;
		registers[NV4097_SET_VERTEX_DATA_ARRAY_OFFSET + 3] = 0x0;
		registers[NV4097_SET_VERTEX_DATA_ARRAY_FORMAT + 4] = 0x2;
		registers[NV4097_SET_VERTEX_DATA_ARRAY_OFFSET + 4] = 0x0;
		registers[NV4097_SET_VERTEX_DATA_ARRAY_FORMAT + 5] = 0x2;
		registers[NV4097_SET_VERTEX_DATA_ARRAY_OFFSET + 5] = 0x0;
		registers[NV4097_SET_VERTEX_DATA_ARRAY_FORMAT + 6] = 0x2;
		registers[NV4097_SET_VERTEX_DATA_ARRAY_OFFSET + 6] = 0x0;
		registers[NV4097_SET_VERTEX_DATA_ARRAY_FORMAT + 7] = 0x2;
		registers[NV4097_SET_VERTEX_DATA_ARRAY_OFFSET + 7] = 0x0;
		registers[NV4097_SET_VERTEX_DATA_ARRAY_FORMAT + 8] = 0x2;
		registers[NV4097_SET_VERTEX_DATA_ARRAY_OFFSET + 8] = 0x0;
		registers[NV4097_SET_VERTEX_DATA_ARRAY_FORMAT + 9] = 0x2;
		registers[NV4097_SET_VERTEX_DATA_ARRAY_OFFSET + 9] = 0x0;
		registers[NV4097_SET_VERTEX_DATA_ARRAY_FORMAT + 10] = 0x2;
		registers[NV4097_SET_VERTEX_DATA_ARRAY_OFFSET + 10] = 0x0;
		registers[NV4097_SET_VERTEX_DATA_ARRAY_FORMAT + 11] = 0x2;
		registers[NV4097_SET_VERTEX_DATA_ARRAY_OFFSET + 11] = 0x0;
		registers[NV4097_SET_VERTEX_DATA_ARRAY_FORMAT + 12] = 0x2;
		registers[NV4097_SET_VERTEX_DATA_ARRAY_OFFSET + 12] = 0x0;
		registers[NV4097_SET_VERTEX_DATA_ARRAY_FORMAT + 13] = 0x2;
		registers[NV4097_SET_VERTEX_DATA_ARRAY_OFFSET + 13] = 0x0;
		registers[NV4097_SET_VERTEX_DATA_ARRAY_FORMAT + 14] = 0x2;
		registers[NV4097_SET_VERTEX_DATA_ARRAY_OFFSET + 14] = 0x0;
		registers[NV4097_SET_VERTEX_DATA_ARRAY_FORMAT + 15] = 0x2;
		registers[NV4097_SET_VERTEX_DATA_ARRAY_OFFSET + 15] = 0x0;
		registers[NV4097_SET_VIEWPORT_HORIZONTAL] = 0x10000000;
		registers[NV4097_SET_VIEWPORT_VERTICAL] = 0x10000000;
		registers[NV4097_SET_CLIP_MIN] = 0x0;
		registers[NV4097_SET_CLIP_MAX] = 0x3f800000;
		registers[NV4097_SET_VIEWPORT_OFFSET + 0] = 0x45000000;
		registers[NV4097_SET_VIEWPORT_OFFSET + 1] = 0x45000000;
		registers[NV4097_SET_VIEWPORT_OFFSET + 2] = 0x3f000000;
		registers[NV4097_SET_VIEWPORT_OFFSET + 3] = 0x0;
		registers[NV4097_SET_VIEWPORT_SCALE + 0] = 0x45000000;
		registers[NV4097_SET_VIEWPORT_SCALE + 1] = 0x45000000;
		registers[NV4097_SET_VIEWPORT_SCALE + 2] = 0x3f000000;
		registers[NV4097_SET_VIEWPORT_SCALE + 3] = 0x0;
		// NOTE: Realhw emits this sequence twice, likely to work around a hardware bug. Similar behavior can be seen in other buggy register blocks
		//registers[NV4097_SET_VIEWPORT_OFFSET + 0] = 0x45000000;
		//registers[NV4097_SET_VIEWPORT_OFFSET + 1] = 0x45000000;
		//registers[NV4097_SET_VIEWPORT_OFFSET + 2] = 0x3f000000;
		//registers[NV4097_SET_VIEWPORT_OFFSET + 3] = 0x0;
		//registers[NV4097_SET_VIEWPORT_SCALE + 0] = 0x45000000;
		//registers[NV4097_SET_VIEWPORT_SCALE + 1] = 0x45000000;
		//registers[NV4097_SET_VIEWPORT_SCALE + 2] = 0x3f000000;
		//registers[NV4097_SET_VIEWPORT_SCALE + 3] = 0x0;
		registers[NV4097_SET_ANTI_ALIASING_CONTROL] = 0xffff0000;
		registers[NV4097_SET_BACK_POLYGON_MODE] = 0x1b02;
		registers[NV4097_SET_COLOR_CLEAR_VALUE] = 0x0;
		registers[NV4097_SET_COLOR_MASK_MRT] = 0x0;
		registers[NV4097_SET_FRONT_POLYGON_MODE] = 0x1b02;
		registers[NV4097_SET_LINE_SMOOTH_ENABLE] = 0x0;
		registers[NV4097_SET_LINE_STIPPLE] = 0x0;
		registers[NV4097_SET_POINT_PARAMS_ENABLE] = 0x0;
		registers[NV4097_SET_POINT_SPRITE_CONTROL] = 0x0;
		registers[NV4097_SET_POLY_SMOOTH_ENABLE] = 0x0;
		registers[NV4097_SET_POLYGON_STIPPLE] = 0x0;
		registers[NV4097_SET_RENDER_ENABLE] = 0x1000000;
		registers[NV4097_SET_USER_CLIP_PLANE_CONTROL] = 0x0;
		registers[NV4097_SET_VERTEX_ATTRIB_INPUT_MASK] = 0xffff;
		registers[NV4097_SET_ZPASS_PIXEL_COUNT_ENABLE] = 0x0;
		registers[NV4097_SET_VERTEX_TEXTURE_ADDRESS] = 0x101;
		registers[NV4097_SET_VERTEX_TEXTURE_BORDER_COLOR] = 0x0;
		registers[NV4097_SET_VERTEX_TEXTURE_CONTROL0] = 0x60000;
		registers[NV4097_SET_VERTEX_TEXTURE_FILTER] = 0x0;
		registers[NV4097_SET_VERTEX_TEXTURE_ADDRESS + 8] = 0x101;
		registers[NV4097_SET_VERTEX_TEXTURE_BORDER_COLOR + 8] = 0x0;
		registers[NV4097_SET_VERTEX_TEXTURE_CONTROL0 + 8] = 0x60000;
		registers[NV4097_SET_VERTEX_TEXTURE_FILTER + 8] = 0x0;
		registers[NV4097_SET_VERTEX_TEXTURE_ADDRESS + 16] = 0x101;
		registers[NV4097_SET_VERTEX_TEXTURE_BORDER_COLOR + 16] = 0x0;
		registers[NV4097_SET_VERTEX_TEXTURE_CONTROL0 + 16] = 0x60000;
		registers[NV4097_SET_VERTEX_TEXTURE_FILTER + 16] = 0x0;
		registers[NV4097_SET_VERTEX_TEXTURE_ADDRESS + 24] = 0x101;
		registers[NV4097_SET_VERTEX_TEXTURE_BORDER_COLOR + 24] = 0x0;
		registers[NV4097_SET_VERTEX_TEXTURE_CONTROL0 + 24] = 0x60000;
		registers[NV4097_SET_VERTEX_TEXTURE_FILTER + 24] = 0x0;
		registers[NV4097_SET_CYLINDRICAL_WRAP] = 0x0;
		registers[NV4097_SET_ZMIN_MAX_CONTROL] = 0x1;
		registers[NV4097_SET_TWO_SIDE_LIGHT_EN] = 0x0;
		registers[NV4097_SET_TRANSFORM_BRANCH_BITS] = 0x0;
		registers[NV4097_SET_NO_PARANOID_TEXTURE_FETCHES] = 0x0;

		registers[NV0039_SET_OBJECT] = 0x31337303;
		registers[NV0039_SET_CONTEXT_DMA_NOTIFIES] = 0x66604200;
		registers[NV0039_SET_CONTEXT_DMA_BUFFER_IN] = 0xfeed0001;
		registers[NV0039_SET_CONTEXT_DMA_BUFFER_OUT] = 0xfeed0000;

		registers[NV3062_SET_OBJECT] = 0x313371c3;
		registers[NV3062_SET_CONTEXT_DMA_NOTIFIES] = 0x66604200;
		registers[NV3062_SET_CONTEXT_DMA_IMAGE_SOURCE] = 0xfeed0000;
		registers[NV3062_SET_CONTEXT_DMA_IMAGE_DESTIN] = 0xfeed0000;

		registers[0xa000 / 4] = 0x31337808;
		registers[0xa180 / 4] = 0x66604200;
		registers[0xa184 / 4] = 0x0;
		registers[0xa188 / 4] = 0x0;
		registers[0xa18c / 4] = 0x0;
		registers[0xa190 / 4] = 0x0;
		registers[0xa194 / 4] = 0x0;
		registers[0xa198 / 4] = 0x0;
		registers[0xa19c / 4] = 0x313371c3;
		registers[0xa2fc / 4] = 0x3;
		registers[0xa300 / 4] = 0x4;
		registers[0x8000 / 4] = 0x31337a73;
		registers[0x8180 / 4] = 0x66604200;
		registers[0x8184 / 4] = 0xfeed0000;
		registers[0xc000 / 4] = 0x3137af00;
		registers[0xc180 / 4] = 0x66604200;

		registers[NV406E_SEMAPHORE_OFFSET] = 0x10;
	}

	void rsx_state::decode(u32 reg, u32 value)
	{
		// Store new value and save previous
		register_previous_value = std::exchange(registers[reg], value);
	}

	bool rsx_state::test(u32 reg, u32 value) const
	{
		return registers[reg] == value;
	}

	u32 draw_clause::execute_pipeline_dependencies() const
	{
		u32 result = 0;

		for (const auto &barrier : draw_command_barriers)
		{
			if (barrier.draw_id != current_range_index)
				continue;

			switch (barrier.type)
			{
			case primitive_restart_barrier:
				break;
			case index_base_modifier_barrier:
				// Change index base offset
				method_registers.decode(NV4097_SET_VERTEX_DATA_BASE_INDEX, barrier.arg);
				result |= index_base_changed;
				break;
			case vertex_base_modifier_barrier:
				// Change vertex base offset
				method_registers.decode(NV4097_SET_VERTEX_DATA_BASE_OFFSET, barrier.arg);
				result |= vertex_base_changed;
				break;
			default:
				fmt::throw_exception("Unreachable");
			}
		}

		return result;
	}

	namespace method_detail
	{
		template <u32 Id, u32 Step, u32 Count, template<u32> class T, u32 Index = 0>
		struct bind_range_impl_t
		{
			static inline void impl()
			{
				methods[Id] = &T<Index>::impl;

				if constexpr (Count > 1)
				{
					bind_range_impl_t<Id + Step, Step, Count - 1, T, Index + 1>::impl();
				}
			}
		};

		template <u32 Id, u32 Step, u32 Count, template<u32> class T, u32 Index = 0>
		static inline void bind_range()
		{
			static_assert(Step && Count && Id + u64{Step} * (Count - 1) < 0x10000 / 4);

			bind_range_impl_t<Id, Step, Count, T, Index>::impl();
		}

		template<u32 Id, rsx_method_t Func>
		static void bind()
		{
			static_assert(Id < 0x10000 / 4);

			methods[Id] = Func;
		}

		template <u32 Id, u32 Step, u32 Count, rsx_method_t Func>
		static void bind_array()
		{
			static_assert(Step && Count && Id + u64{Step} * (Count - 1) < 0x10000 / 4);

			for (u32 i = Id; i < Id + Count * Step; i += Step)
			{
				methods[i] = Func;
			}
		}
	}

	// TODO: implement this as virtual function: rsx::thread::init_methods() or something
	static const bool s_methods_init = []() -> bool
	{
		using namespace method_detail;

		methods.fill(&invalid_method);

		// NV40_CHANNEL_DMA (NV406E)
		methods[NV406E_SET_REFERENCE]                     = nullptr;
		methods[NV406E_SET_CONTEXT_DMA_SEMAPHORE]         = nullptr;
		methods[NV406E_SEMAPHORE_OFFSET]                  = nullptr;
		methods[NV406E_SEMAPHORE_ACQUIRE]                 = nullptr;
		methods[NV406E_SEMAPHORE_RELEASE]                 = nullptr;

		// NV40_CURIE_PRIMITIVE	(NV4097)
		methods[NV4097_SET_OBJECT]                        = nullptr;
		methods[NV4097_NO_OPERATION]                      = nullptr;
		methods[NV4097_NOTIFY]                            = nullptr;
		methods[NV4097_WAIT_FOR_IDLE]                     = nullptr;
		methods[NV4097_PM_TRIGGER]                        = nullptr;
		methods[NV4097_SET_CONTEXT_DMA_NOTIFIES]          = nullptr;
		methods[NV4097_SET_CONTEXT_DMA_A]                 = nullptr;
		methods[NV4097_SET_CONTEXT_DMA_B]                 = nullptr;
		methods[NV4097_SET_CONTEXT_DMA_COLOR_B]           = nullptr;
		methods[NV4097_SET_CONTEXT_DMA_STATE]             = nullptr;
		methods[NV4097_SET_CONTEXT_DMA_COLOR_A]           = nullptr;
		methods[NV4097_SET_CONTEXT_DMA_ZETA]              = nullptr;
		methods[NV4097_SET_CONTEXT_DMA_VERTEX_A]          = nullptr;
		methods[NV4097_SET_CONTEXT_DMA_VERTEX_B]          = nullptr;
		methods[NV4097_SET_CONTEXT_DMA_SEMAPHORE]         = nullptr;
		methods[NV4097_SET_CONTEXT_DMA_REPORT]            = nullptr;
		methods[NV4097_SET_CONTEXT_DMA_CLIP_ID]           = nullptr;
		methods[NV4097_SET_CONTEXT_DMA_CULL_DATA]         = nullptr;
		methods[NV4097_SET_CONTEXT_DMA_COLOR_C]           = nullptr;
		methods[NV4097_SET_CONTEXT_DMA_COLOR_D]           = nullptr;
		methods[NV4097_SET_SURFACE_CLIP_HORIZONTAL]       = nullptr;
		methods[NV4097_SET_SURFACE_CLIP_VERTICAL]         = nullptr;
		methods[NV4097_SET_SURFACE_FORMAT]                = nullptr;
		methods[NV4097_SET_SURFACE_PITCH_A]               = nullptr;
		methods[NV4097_SET_SURFACE_COLOR_AOFFSET]         = nullptr;
		methods[NV4097_SET_SURFACE_ZETA_OFFSET]           = nullptr;
		methods[NV4097_SET_SURFACE_COLOR_BOFFSET]         = nullptr;
		methods[NV4097_SET_SURFACE_PITCH_B]               = nullptr;
		methods[NV4097_SET_SURFACE_COLOR_TARGET]          = nullptr;
		methods[0x224 >> 2]                               = nullptr;
		methods[0x228 >> 2]                               = nullptr;
		methods[0x230 >> 2]                               = nullptr;
		methods[NV4097_SET_SURFACE_PITCH_Z]               = nullptr;
		methods[NV4097_INVALIDATE_ZCULL]                  = nullptr;
		methods[NV4097_SET_CYLINDRICAL_WRAP]              = nullptr;
		methods[NV4097_SET_CYLINDRICAL_WRAP1]             = nullptr;
		methods[0x240 >> 2]                               = nullptr;
		methods[0x244 >> 2]                               = nullptr;
		methods[0x248 >> 2]                               = nullptr;
		methods[0x24C >> 2]                               = nullptr;
		methods[NV4097_SET_SURFACE_PITCH_C]               = nullptr;
		methods[NV4097_SET_SURFACE_PITCH_D]               = nullptr;
		methods[NV4097_SET_SURFACE_COLOR_COFFSET]         = nullptr;
		methods[NV4097_SET_SURFACE_COLOR_DOFFSET]         = nullptr;
		methods[NV4097_SET_WINDOW_OFFSET]                 = nullptr;
		methods[NV4097_SET_WINDOW_CLIP_TYPE]              = nullptr;
		methods[NV4097_SET_WINDOW_CLIP_HORIZONTAL]        = nullptr;
		methods[NV4097_SET_WINDOW_CLIP_VERTICAL]          = nullptr;
		methods[0x2c8 >> 2]                               = nullptr;
		methods[0x2cc >> 2]                               = nullptr;
		methods[0x2d0 >> 2]                               = nullptr;
		methods[0x2d4 >> 2]                               = nullptr;
		methods[0x2d8 >> 2]                               = nullptr;
		methods[0x2dc >> 2]                               = nullptr;
		methods[0x2e0 >> 2]                               = nullptr;
		methods[0x2e4 >> 2]                               = nullptr;
		methods[0x2e8 >> 2]                               = nullptr;
		methods[0x2ec >> 2]                               = nullptr;
		methods[0x2f0 >> 2]                               = nullptr;
		methods[0x2f4 >> 2]                               = nullptr;
		methods[0x2f8 >> 2]                               = nullptr;
		methods[0x2fc >> 2]                               = nullptr;
		methods[NV4097_SET_DITHER_ENABLE]                 = nullptr;
		methods[NV4097_SET_ALPHA_TEST_ENABLE]             = nullptr;
		methods[NV4097_SET_ALPHA_FUNC]                    = nullptr;
		methods[NV4097_SET_ALPHA_REF]                     = nullptr;
		methods[NV4097_SET_BLEND_ENABLE]                  = nullptr;
		methods[NV4097_SET_BLEND_FUNC_SFACTOR]            = nullptr;
		methods[NV4097_SET_BLEND_FUNC_DFACTOR]            = nullptr;
		methods[NV4097_SET_BLEND_COLOR]                   = nullptr;
		methods[NV4097_SET_BLEND_EQUATION]                = nullptr;
		methods[NV4097_SET_COLOR_MASK]                    = nullptr;
		methods[NV4097_SET_STENCIL_TEST_ENABLE]           = nullptr;
		methods[NV4097_SET_STENCIL_MASK]                  = nullptr;
		methods[NV4097_SET_STENCIL_FUNC]                  = nullptr;
		methods[NV4097_SET_STENCIL_FUNC_REF]              = nullptr;
		methods[NV4097_SET_STENCIL_FUNC_MASK]             = nullptr;
		methods[NV4097_SET_STENCIL_OP_FAIL]               = nullptr;
		methods[NV4097_SET_STENCIL_OP_ZFAIL]              = nullptr;
		methods[NV4097_SET_STENCIL_OP_ZPASS]              = nullptr;
		methods[NV4097_SET_TWO_SIDED_STENCIL_TEST_ENABLE] = nullptr;
		methods[NV4097_SET_BACK_STENCIL_MASK]             = nullptr;
		methods[NV4097_SET_BACK_STENCIL_FUNC]             = nullptr;
		methods[NV4097_SET_BACK_STENCIL_FUNC_REF]         = nullptr;
		methods[NV4097_SET_BACK_STENCIL_FUNC_MASK]        = nullptr;
		methods[NV4097_SET_BACK_STENCIL_OP_FAIL]          = nullptr;
		methods[NV4097_SET_BACK_STENCIL_OP_ZFAIL]         = nullptr;
		methods[NV4097_SET_BACK_STENCIL_OP_ZPASS]         = nullptr;
		methods[NV4097_SET_SHADE_MODE]                    = nullptr;
		methods[NV4097_SET_BLEND_ENABLE_MRT]              = nullptr;
		methods[NV4097_SET_COLOR_MASK_MRT]                = nullptr;
		methods[NV4097_SET_LOGIC_OP_ENABLE]               = nullptr;
		methods[NV4097_SET_LOGIC_OP]                      = nullptr;
		methods[NV4097_SET_BLEND_COLOR2]                  = nullptr;
		methods[NV4097_SET_DEPTH_BOUNDS_TEST_ENABLE]      = nullptr;
		methods[NV4097_SET_DEPTH_BOUNDS_MIN]              = nullptr;
		methods[NV4097_SET_DEPTH_BOUNDS_MAX]              = nullptr;
		methods[NV4097_SET_CLIP_MIN]                      = nullptr;
		methods[NV4097_SET_CLIP_MAX]                      = nullptr;
		methods[NV4097_SET_CONTROL0]                      = nullptr;
		methods[NV4097_SET_LINE_WIDTH]                    = nullptr;
		methods[NV4097_SET_LINE_SMOOTH_ENABLE]            = nullptr;
		methods[NV4097_SET_ANISO_SPREAD]                  = nullptr;
		methods[NV4097_SET_SCISSOR_HORIZONTAL]            = nullptr;
		methods[NV4097_SET_SCISSOR_VERTICAL]              = nullptr;
		methods[NV4097_SET_FOG_MODE]                      = nullptr;
		methods[NV4097_SET_FOG_PARAMS]                    = nullptr;
		methods[NV4097_SET_FOG_PARAMS + 1]                = nullptr;
		methods[0x8d8 >> 2]                               = nullptr;
		methods[NV4097_SET_SHADER_PROGRAM]                = nullptr;
		methods[NV4097_SET_VERTEX_TEXTURE_OFFSET]         = nullptr;
		methods[NV4097_SET_VERTEX_TEXTURE_FORMAT]         = nullptr;
		methods[NV4097_SET_VERTEX_TEXTURE_ADDRESS]        = nullptr;
		methods[NV4097_SET_VERTEX_TEXTURE_CONTROL0]       = nullptr;
		methods[NV4097_SET_VERTEX_TEXTURE_CONTROL3]       = nullptr;
		methods[NV4097_SET_VERTEX_TEXTURE_FILTER]         = nullptr;
		methods[NV4097_SET_VERTEX_TEXTURE_IMAGE_RECT]     = nullptr;
		methods[NV4097_SET_VERTEX_TEXTURE_BORDER_COLOR]   = nullptr;
		methods[NV4097_SET_VIEWPORT_HORIZONTAL]           = nullptr;
		methods[NV4097_SET_VIEWPORT_VERTICAL]             = nullptr;
		methods[NV4097_SET_POINT_CENTER_MODE]             = nullptr;
		methods[NV4097_ZCULL_SYNC]                        = nullptr;
		methods[NV4097_SET_VIEWPORT_OFFSET]               = nullptr;
		methods[NV4097_SET_VIEWPORT_OFFSET + 1]           = nullptr;
		methods[NV4097_SET_VIEWPORT_OFFSET + 2]           = nullptr;
		methods[NV4097_SET_VIEWPORT_OFFSET + 3]           = nullptr;
		methods[NV4097_SET_VIEWPORT_SCALE]                = nullptr;
		methods[NV4097_SET_VIEWPORT_SCALE + 1]            = nullptr;
		methods[NV4097_SET_VIEWPORT_SCALE + 2]            = nullptr;
		methods[NV4097_SET_VIEWPORT_SCALE + 3]            = nullptr;
		methods[NV4097_SET_POLY_OFFSET_POINT_ENABLE]      = nullptr;
		methods[NV4097_SET_POLY_OFFSET_LINE_ENABLE]       = nullptr;
		methods[NV4097_SET_POLY_OFFSET_FILL_ENABLE]       = nullptr;
		methods[NV4097_SET_DEPTH_FUNC]                    = nullptr;
		methods[NV4097_SET_DEPTH_MASK]                    = nullptr;
		methods[NV4097_SET_DEPTH_TEST_ENABLE]             = nullptr;
		methods[NV4097_SET_POLYGON_OFFSET_SCALE_FACTOR]   = nullptr;
		methods[NV4097_SET_POLYGON_OFFSET_BIAS]           = nullptr;
		methods[NV4097_SET_VERTEX_DATA_SCALED4S_M]        = nullptr;
		methods[NV4097_SET_TEXTURE_CONTROL2]              = nullptr;
		methods[NV4097_SET_TEX_COORD_CONTROL]             = nullptr;
		methods[NV4097_SET_TRANSFORM_PROGRAM]             = nullptr;
		methods[NV4097_SET_SPECULAR_ENABLE]               = nullptr;
		methods[NV4097_SET_TWO_SIDE_LIGHT_EN]             = nullptr;
		methods[NV4097_CLEAR_ZCULL_SURFACE]               = nullptr;
		methods[NV4097_SET_PERFORMANCE_PARAMS]            = nullptr;
		methods[NV4097_SET_FLAT_SHADE_OP]                 = nullptr;
		methods[NV4097_SET_EDGE_FLAG]                     = nullptr;
		methods[NV4097_SET_USER_CLIP_PLANE_CONTROL]       = nullptr;
		methods[NV4097_SET_POLYGON_STIPPLE]               = nullptr;
		methods[NV4097_SET_POLYGON_STIPPLE_PATTERN]       = nullptr;
		methods[NV4097_SET_VERTEX_DATA3F_M]               = nullptr;
		methods[NV4097_SET_VERTEX_DATA_ARRAY_OFFSET]      = nullptr;
		methods[NV4097_INVALIDATE_VERTEX_CACHE_FILE]      = nullptr;
		methods[NV4097_INVALIDATE_VERTEX_FILE]            = nullptr;
		methods[NV4097_PIPE_NOP]                          = nullptr;
		methods[NV4097_SET_VERTEX_DATA_BASE_OFFSET]       = nullptr;
		methods[NV4097_SET_VERTEX_DATA_BASE_INDEX]        = nullptr;
		methods[NV4097_SET_VERTEX_DATA_ARRAY_FORMAT]      = nullptr;
		methods[NV4097_CLEAR_REPORT_VALUE]                = nullptr;
		methods[NV4097_SET_ZPASS_PIXEL_COUNT_ENABLE]      = nullptr;
		methods[NV4097_GET_REPORT]                        = nullptr;
		methods[NV4097_SET_ZCULL_STATS_ENABLE]            = nullptr;
		methods[NV4097_SET_BEGIN_END]                     = nullptr;
		methods[NV4097_ARRAY_ELEMENT16]                   = nullptr;
		methods[NV4097_ARRAY_ELEMENT32]                   = nullptr;
		methods[NV4097_DRAW_ARRAYS]                       = nullptr;
		methods[NV4097_INLINE_ARRAY]                      = nullptr;
		methods[NV4097_SET_INDEX_ARRAY_ADDRESS]           = nullptr;
		methods[NV4097_SET_INDEX_ARRAY_DMA]               = nullptr;
		methods[NV4097_DRAW_INDEX_ARRAY]                  = nullptr;
		methods[NV4097_SET_FRONT_POLYGON_MODE]            = nullptr;
		methods[NV4097_SET_BACK_POLYGON_MODE]             = nullptr;
		methods[NV4097_SET_CULL_FACE]                     = nullptr;
		methods[NV4097_SET_FRONT_FACE]                    = nullptr;
		methods[NV4097_SET_POLY_SMOOTH_ENABLE]            = nullptr;
		methods[NV4097_SET_CULL_FACE_ENABLE]              = nullptr;
		methods[NV4097_SET_TEXTURE_CONTROL3]              = nullptr;
		methods[NV4097_SET_VERTEX_DATA2F_M]               = nullptr;
		methods[NV4097_SET_VERTEX_DATA2S_M]               = nullptr;
		methods[NV4097_SET_VERTEX_DATA4UB_M]              = nullptr;
		methods[NV4097_SET_VERTEX_DATA4S_M]               = nullptr;
		methods[NV4097_SET_TEXTURE_OFFSET]                = nullptr;
		methods[NV4097_SET_TEXTURE_FORMAT]                = nullptr;
		methods[NV4097_SET_TEXTURE_ADDRESS]               = nullptr;
		methods[NV4097_SET_TEXTURE_CONTROL0]              = nullptr;
		methods[NV4097_SET_TEXTURE_CONTROL1]              = nullptr;
		methods[NV4097_SET_TEXTURE_FILTER]                = nullptr;
		methods[NV4097_SET_TEXTURE_IMAGE_RECT]            = nullptr;
		methods[NV4097_SET_TEXTURE_BORDER_COLOR]          = nullptr;
		methods[NV4097_SET_VERTEX_DATA4F_M]               = nullptr;
		methods[NV4097_SET_COLOR_KEY_COLOR]               = nullptr;
		methods[0x1d04 >> 2]                              = nullptr;
		methods[NV4097_SET_SHADER_CONTROL]                = nullptr;
		methods[NV4097_SET_INDEXED_CONSTANT_READ_LIMITS]  = nullptr;
		methods[NV4097_SET_SEMAPHORE_OFFSET]              = nullptr;
		methods[NV4097_BACK_END_WRITE_SEMAPHORE_RELEASE]  = nullptr;
		methods[NV4097_TEXTURE_READ_SEMAPHORE_RELEASE]    = nullptr;
		methods[NV4097_SET_ZMIN_MAX_CONTROL]              = nullptr;
		methods[NV4097_SET_ANTI_ALIASING_CONTROL]         = nullptr;
		methods[NV4097_SET_SURFACE_COMPRESSION]           = nullptr;
		methods[NV4097_SET_ZCULL_EN]                      = nullptr;
		methods[NV4097_SET_SHADER_WINDOW]                 = nullptr;
		methods[NV4097_SET_ZSTENCIL_CLEAR_VALUE]          = nullptr;
		methods[NV4097_SET_COLOR_CLEAR_VALUE]             = nullptr;
		methods[NV4097_CLEAR_SURFACE]                     = nullptr;
		methods[NV4097_SET_CLEAR_RECT_HORIZONTAL]         = nullptr;
		methods[NV4097_SET_CLEAR_RECT_VERTICAL]           = nullptr;
		methods[NV4097_SET_CLIP_ID_TEST_ENABLE]           = nullptr;
		methods[NV4097_SET_RESTART_INDEX_ENABLE]          = nullptr;
		methods[NV4097_SET_RESTART_INDEX]                 = nullptr;
		methods[NV4097_SET_LINE_STIPPLE]                  = nullptr;
		methods[NV4097_SET_LINE_STIPPLE_PATTERN]          = nullptr;
		methods[NV4097_SET_VERTEX_DATA1F_M]               = nullptr;
		methods[NV4097_SET_TRANSFORM_EXECUTION_MODE]      = nullptr;
		methods[NV4097_SET_RENDER_ENABLE]                 = nullptr;
		methods[NV4097_SET_TRANSFORM_PROGRAM_LOAD]        = nullptr;
		methods[NV4097_SET_TRANSFORM_PROGRAM_START]       = nullptr;
		methods[NV4097_SET_ZCULL_CONTROL0]                = nullptr;
		methods[NV4097_SET_ZCULL_CONTROL1]                = nullptr;
		methods[NV4097_SET_SCULL_CONTROL]                 = nullptr;
		methods[NV4097_SET_POINT_SIZE]                    = nullptr;
		methods[NV4097_SET_POINT_PARAMS_ENABLE]           = nullptr;
		methods[NV4097_SET_POINT_SPRITE_CONTROL]          = nullptr;
		methods[NV4097_SET_TRANSFORM_TIMEOUT]             = nullptr;
		methods[NV4097_SET_TRANSFORM_CONSTANT_LOAD]       = nullptr;
		methods[NV4097_SET_TRANSFORM_CONSTANT]            = nullptr;
		methods[NV4097_SET_FREQUENCY_DIVIDER_OPERATION]   = nullptr;
		methods[NV4097_SET_ATTRIB_COLOR]                  = nullptr;
		methods[NV4097_SET_ATTRIB_TEX_COORD]              = nullptr;
		methods[NV4097_SET_ATTRIB_TEX_COORD_EX]           = nullptr;
		methods[NV4097_SET_ATTRIB_UCLIP0]                 = nullptr;
		methods[NV4097_SET_ATTRIB_UCLIP1]                 = nullptr;
		methods[NV4097_INVALIDATE_L2]                     = nullptr;
		methods[NV4097_SET_REDUCE_DST_COLOR]              = nullptr;
		methods[NV4097_SET_NO_PARANOID_TEXTURE_FETCHES]   = nullptr;
		methods[NV4097_SET_SHADER_PACKER]                 = nullptr;
		methods[NV4097_SET_VERTEX_ATTRIB_INPUT_MASK]      = nullptr;
		methods[NV4097_SET_VERTEX_ATTRIB_OUTPUT_MASK]     = nullptr;
		methods[NV4097_SET_TRANSFORM_BRANCH_BITS]         = nullptr;

		// NV03_MEMORY_TO_MEMORY_FORMAT	(NV0039)
		methods[NV0039_SET_OBJECT]                        = nullptr;
		methods[NV0039_SET_CONTEXT_DMA_NOTIFIES]          = nullptr;
		methods[NV0039_SET_CONTEXT_DMA_BUFFER_IN]         = nullptr;
		methods[NV0039_SET_CONTEXT_DMA_BUFFER_OUT]        = nullptr;
		methods[NV0039_OFFSET_IN]                         = nullptr;
		methods[NV0039_OFFSET_OUT]                        = nullptr;
		methods[NV0039_PITCH_IN]                          = nullptr;
		methods[NV0039_PITCH_OUT]                         = nullptr;
		methods[NV0039_LINE_LENGTH_IN]                    = nullptr;
		methods[NV0039_LINE_COUNT]                        = nullptr;
		methods[NV0039_FORMAT]                            = nullptr;
		methods[NV0039_BUFFER_NOTIFY]                     = nullptr;

		// NV30_CONTEXT_SURFACES_2D	(NV3062)
		methods[NV3062_SET_OBJECT]                        = nullptr;
		methods[NV3062_SET_CONTEXT_DMA_NOTIFIES]          = nullptr;
		methods[NV3062_SET_CONTEXT_DMA_IMAGE_SOURCE]      = nullptr;
		methods[NV3062_SET_CONTEXT_DMA_IMAGE_DESTIN]      = nullptr;
		methods[NV3062_SET_COLOR_FORMAT]                  = nullptr;
		methods[NV3062_SET_PITCH]                         = nullptr;
		methods[NV3062_SET_OFFSET_SOURCE]                 = nullptr;
		methods[NV3062_SET_OFFSET_DESTIN]                 = nullptr;

		// NV30_CONTEXT_SURFACE_SWIZZLED (NV309E)
		methods[NV309E_SET_OBJECT]                        = nullptr;
		methods[NV309E_SET_CONTEXT_DMA_NOTIFIES]          = nullptr;
		methods[NV309E_SET_CONTEXT_DMA_IMAGE]             = nullptr;
		methods[NV309E_SET_FORMAT]                        = nullptr;
		methods[NV309E_SET_OFFSET]                        = nullptr;

		// NV30_IMAGE_FROM_CPU (NV308A)
		methods[NV308A_SET_OBJECT]                        = nullptr;
		methods[NV308A_SET_CONTEXT_DMA_NOTIFIES]          = nullptr;
		methods[NV308A_SET_CONTEXT_COLOR_KEY]             = nullptr;
		methods[NV308A_SET_CONTEXT_CLIP_RECTANGLE]        = nullptr;
		methods[NV308A_SET_CONTEXT_PATTERN]               = nullptr;
		methods[NV308A_SET_CONTEXT_ROP]                   = nullptr;
		methods[NV308A_SET_CONTEXT_BETA1]                 = nullptr;
		methods[NV308A_SET_CONTEXT_BETA4]                 = nullptr;
		methods[NV308A_SET_CONTEXT_SURFACE]               = nullptr;
		methods[NV308A_SET_COLOR_CONVERSION]              = nullptr;
		methods[NV308A_SET_OPERATION]                     = nullptr;
		methods[NV308A_SET_COLOR_FORMAT]                  = nullptr;
		methods[NV308A_POINT]                             = nullptr;
		methods[NV308A_SIZE_OUT]                          = nullptr;
		methods[NV308A_SIZE_IN]                           = nullptr;
		methods[NV308A_COLOR]                             = nullptr;

		// NV30_SCALED_IMAGE_FROM_MEMORY (NV3089)
		methods[NV3089_SET_OBJECT]                        = nullptr;
		methods[NV3089_SET_CONTEXT_DMA_NOTIFIES]          = nullptr;
		methods[NV3089_SET_CONTEXT_DMA_IMAGE]             = nullptr;
		methods[NV3089_SET_CONTEXT_PATTERN]               = nullptr;
		methods[NV3089_SET_CONTEXT_ROP]                   = nullptr;
		methods[NV3089_SET_CONTEXT_BETA1]                 = nullptr;
		methods[NV3089_SET_CONTEXT_BETA4]                 = nullptr;
		methods[NV3089_SET_CONTEXT_SURFACE]               = nullptr;
		methods[NV3089_SET_COLOR_CONVERSION]              = nullptr;
		methods[NV3089_SET_COLOR_FORMAT]                  = nullptr;
		methods[NV3089_SET_OPERATION]                     = nullptr;
		methods[NV3089_CLIP_POINT]                        = nullptr;
		methods[NV3089_CLIP_SIZE]                         = nullptr;
		methods[NV3089_IMAGE_OUT_POINT]                   = nullptr;
		methods[NV3089_IMAGE_OUT_SIZE]                    = nullptr;
		methods[NV3089_DS_DX]                             = nullptr;
		methods[NV3089_DT_DY]                             = nullptr;
		methods[NV3089_IMAGE_IN_SIZE]                     = nullptr;
		methods[NV3089_IMAGE_IN_FORMAT]                   = nullptr;
		methods[NV3089_IMAGE_IN_OFFSET]                   = nullptr;
		methods[NV3089_IMAGE_IN]                          = nullptr;

		//Some custom GCM methods
		methods[GCM_SET_DRIVER_OBJECT]                    = nullptr;
		methods[FIFO::FIFO_DRAW_BARRIER >> 2]             = nullptr;

		bind_array<GCM_FLIP_HEAD, 1, 2, nullptr>();
		bind_array<GCM_DRIVER_QUEUE, 1, 8, nullptr>();

		bind_array<(0x400 >> 2), 1, 0x10, nullptr>();
		bind_array<(0x440 >> 2), 1, 0x20, nullptr>();
		bind_array<NV4097_SET_ANISO_SPREAD, 1, 16, nullptr>();
		bind_array<NV4097_SET_VERTEX_TEXTURE_OFFSET, 1, 8 * 4, nullptr>();
		bind_array<NV4097_SET_VERTEX_DATA_SCALED4S_M, 1, 32, nullptr>();
		bind_array<NV4097_SET_TEXTURE_CONTROL2, 1, 16, nullptr>();
		bind_array<NV4097_SET_TEX_COORD_CONTROL, 1, 10, nullptr>();
		bind_array<NV4097_SET_TRANSFORM_PROGRAM, 1, 32, nullptr>();
		bind_array<NV4097_SET_POLYGON_STIPPLE_PATTERN, 1, 32, nullptr>();
		bind_array<NV4097_SET_VERTEX_DATA3F_M, 1, 64, nullptr>();
		bind_array<NV4097_SET_VERTEX_DATA_ARRAY_OFFSET, 1, 16, nullptr>();
		bind_array<NV4097_SET_VERTEX_DATA_ARRAY_FORMAT, 1, 16, nullptr>();
		bind_array<NV4097_SET_TEXTURE_CONTROL3, 1, 16, nullptr>();
		bind_array<NV4097_SET_VERTEX_DATA2F_M, 1, 32, nullptr>();
		bind_array<NV4097_SET_VERTEX_DATA2S_M, 1, 16, nullptr>();
		bind_array<NV4097_SET_VERTEX_DATA4UB_M, 1, 16, nullptr>();
		bind_array<NV4097_SET_VERTEX_DATA4S_M, 1, 32, nullptr>();
		bind_array<NV4097_SET_TEXTURE_OFFSET, 1, 8 * 16, nullptr>();
		bind_array<NV4097_SET_VERTEX_DATA4F_M, 1, 64, nullptr>();
		bind_array<NV4097_SET_VERTEX_DATA1F_M, 1, 16, nullptr>();
		bind_array<NV4097_SET_COLOR_KEY_COLOR, 1, 16, nullptr>();

		// Unknown (NV4097?)
		bind<(0x171c >> 2), trace_method>();

		// NV406E
		bind<NV406E_SET_REFERENCE, nv406e::set_reference>();
		bind<NV406E_SEMAPHORE_ACQUIRE, nv406e::semaphore_acquire>();
		bind<NV406E_SEMAPHORE_RELEASE, nv406e::semaphore_release>();

		// NV4097
		bind<NV4097_SET_CULL_FACE, nv4097::set_cull_face>();
		bind<NV4097_TEXTURE_READ_SEMAPHORE_RELEASE, nv4097::texture_read_semaphore_release>();
		bind<NV4097_BACK_END_WRITE_SEMAPHORE_RELEASE, nv4097::back_end_write_semaphore_release>();
		bind<NV4097_SET_BEGIN_END, nv4097::set_begin_end>();
		bind<NV4097_CLEAR_SURFACE, nv4097::clear>();
		bind<NV4097_DRAW_ARRAYS, nv4097::draw_arrays>();
		bind<NV4097_DRAW_INDEX_ARRAY, nv4097::draw_index_array>();
		bind<NV4097_INLINE_ARRAY, nv4097::draw_inline_array>();
		bind<NV4097_ARRAY_ELEMENT16, nv4097::set_array_element16>();
		bind<NV4097_ARRAY_ELEMENT32, nv4097::set_array_element32>();
		bind_range<NV4097_SET_VERTEX_DATA_SCALED4S_M, 1, 32, nv4097::set_vertex_data_scaled4s_m>();
		bind_range<NV4097_SET_VERTEX_DATA4UB_M, 1, 16, nv4097::set_vertex_data4ub_m>();
		bind_range<NV4097_SET_VERTEX_DATA1F_M, 1, 16, nv4097::set_vertex_data1f_m>();
		bind_range<NV4097_SET_VERTEX_DATA2F_M, 1, 32, nv4097::set_vertex_data2f_m>();
		bind_range<NV4097_SET_VERTEX_DATA3F_M, 1, 64, nv4097::set_vertex_data3f_m>();
		bind_range<NV4097_SET_VERTEX_DATA4F_M, 1, 64, nv4097::set_vertex_data4f_m>();
		bind_range<NV4097_SET_VERTEX_DATA2S_M, 1, 16, nv4097::set_vertex_data2s_m>();
		bind_range<NV4097_SET_VERTEX_DATA4S_M, 1, 32, nv4097::set_vertex_data4s_m>();
		bind_range<NV4097_SET_TRANSFORM_CONSTANT, 1, 32, nv4097::set_transform_constant>();
		bind_range<NV4097_SET_TRANSFORM_PROGRAM, 1, 32, nv4097::set_transform_program>();
		bind<NV4097_GET_REPORT, nv4097::get_report>();
		bind<NV4097_CLEAR_REPORT_VALUE, nv4097::clear_report_value>();
		bind<NV4097_SET_SURFACE_CLIP_HORIZONTAL, nv4097::set_surface_dirty_bit>();
		bind<NV4097_SET_SURFACE_CLIP_VERTICAL, nv4097::set_surface_dirty_bit>();
		bind<NV4097_SET_SURFACE_COLOR_AOFFSET, nv4097::set_surface_dirty_bit>();
		bind<NV4097_SET_SURFACE_COLOR_BOFFSET, nv4097::set_surface_dirty_bit>();
		bind<NV4097_SET_SURFACE_COLOR_COFFSET, nv4097::set_surface_dirty_bit>();
		bind<NV4097_SET_SURFACE_COLOR_DOFFSET, nv4097::set_surface_dirty_bit>();
		bind<NV4097_SET_SURFACE_ZETA_OFFSET, nv4097::set_surface_dirty_bit>();
		bind<NV4097_SET_CONTEXT_DMA_COLOR_A, nv4097::set_surface_dirty_bit>();
		bind<NV4097_SET_CONTEXT_DMA_COLOR_B, nv4097::set_surface_dirty_bit>();
		bind<NV4097_SET_CONTEXT_DMA_COLOR_C, nv4097::set_surface_dirty_bit>();
		bind<NV4097_SET_CONTEXT_DMA_COLOR_D, nv4097::set_surface_dirty_bit>();
		bind<NV4097_SET_CONTEXT_DMA_ZETA, nv4097::set_surface_dirty_bit>();
		bind<NV4097_NOTIFY, nv4097::set_notify>();
		bind<NV4097_SET_SURFACE_FORMAT, nv4097::set_surface_format>();
		bind<NV4097_SET_SURFACE_PITCH_A, nv4097::set_surface_dirty_bit>();
		bind<NV4097_SET_SURFACE_PITCH_B, nv4097::set_surface_dirty_bit>();
		bind<NV4097_SET_SURFACE_PITCH_C, nv4097::set_surface_dirty_bit>();
		bind<NV4097_SET_SURFACE_PITCH_D, nv4097::set_surface_dirty_bit>();
		bind<NV4097_SET_SURFACE_PITCH_Z, nv4097::set_surface_dirty_bit>();
		bind<NV4097_SET_WINDOW_OFFSET, nv4097::set_surface_dirty_bit>();
		bind_range<NV4097_SET_TEXTURE_OFFSET, 8, 16, nv4097::set_texture_dirty_bit>();
		bind_range<NV4097_SET_TEXTURE_FORMAT, 8, 16, nv4097::set_texture_dirty_bit>();
		bind_range<NV4097_SET_TEXTURE_ADDRESS, 8, 16, nv4097::set_texture_dirty_bit>();
		bind_range<NV4097_SET_TEXTURE_CONTROL0, 8, 16, nv4097::set_texture_dirty_bit>();
		bind_range<NV4097_SET_TEXTURE_CONTROL1, 8, 16, nv4097::set_texture_dirty_bit>();
		bind_range<NV4097_SET_TEXTURE_CONTROL2, 1, 16, nv4097::set_texture_dirty_bit>();
		bind_range<NV4097_SET_TEXTURE_CONTROL3, 1, 16, nv4097::set_texture_dirty_bit>();
		bind_range<NV4097_SET_TEXTURE_FILTER, 8, 16, nv4097::set_texture_dirty_bit>();
		bind_range<NV4097_SET_TEXTURE_IMAGE_RECT, 8, 16, nv4097::set_texture_dirty_bit>();
		bind_range<NV4097_SET_TEXTURE_BORDER_COLOR, 8, 16, nv4097::set_texture_dirty_bit>();
		bind_range<NV4097_SET_VERTEX_TEXTURE_OFFSET, 8, 4, nv4097::set_vertex_texture_dirty_bit>();
		bind_range<NV4097_SET_VERTEX_TEXTURE_FORMAT, 8, 4, nv4097::set_vertex_texture_dirty_bit>();
		bind_range<NV4097_SET_VERTEX_TEXTURE_ADDRESS, 8, 4, nv4097::set_vertex_texture_dirty_bit>();
		bind_range<NV4097_SET_VERTEX_TEXTURE_CONTROL0, 8, 4, nv4097::set_vertex_texture_dirty_bit>();
		bind_range<NV4097_SET_VERTEX_TEXTURE_CONTROL3, 8, 4, nv4097::set_vertex_texture_dirty_bit>();
		bind_range<NV4097_SET_VERTEX_TEXTURE_FILTER, 8, 4, nv4097::set_vertex_texture_dirty_bit>();
		bind_range<NV4097_SET_VERTEX_TEXTURE_IMAGE_RECT, 8, 4, nv4097::set_vertex_texture_dirty_bit>();
		bind_range<NV4097_SET_VERTEX_TEXTURE_BORDER_COLOR, 8, 4, nv4097::set_vertex_texture_dirty_bit>();
		bind<NV4097_SET_RENDER_ENABLE, nv4097::set_render_mode>();
		bind<NV4097_SET_ZCULL_EN, nv4097::set_zcull_render_enable>();
		bind<NV4097_SET_ZCULL_STATS_ENABLE, nv4097::set_zcull_stats_enable>();
		bind<NV4097_SET_ZPASS_PIXEL_COUNT_ENABLE, nv4097::set_zcull_pixel_count_enable>();
		bind<NV4097_CLEAR_ZCULL_SURFACE, nv4097::clear_zcull>();
		bind<NV4097_SET_DEPTH_TEST_ENABLE, nv4097::set_surface_options_dirty_bit>();
		bind<NV4097_SET_DEPTH_FUNC, nv4097::set_surface_options_dirty_bit>();
		bind<NV4097_SET_STENCIL_TEST_ENABLE, nv4097::set_surface_options_dirty_bit>();
		bind<NV4097_SET_DEPTH_MASK, nv4097::set_surface_options_dirty_bit>();
		bind<NV4097_SET_COLOR_MASK, nv4097::set_surface_options_dirty_bit>();
		bind<NV4097_SET_COLOR_MASK_MRT, nv4097::set_surface_options_dirty_bit>();
		bind<NV4097_SET_TWO_SIDED_STENCIL_TEST_ENABLE, nv4097::set_surface_options_dirty_bit>();
		bind<NV4097_SET_STENCIL_TEST_ENABLE, nv4097::set_surface_options_dirty_bit>();
		bind<NV4097_SET_STENCIL_MASK, nv4097::set_surface_options_dirty_bit>();
		bind<NV4097_SET_STENCIL_OP_ZPASS, nv4097::set_stencil_op>();
		bind<NV4097_SET_STENCIL_OP_FAIL, nv4097::set_stencil_op>();
		bind<NV4097_SET_STENCIL_OP_ZFAIL, nv4097::set_stencil_op>();
		bind<NV4097_SET_BACK_STENCIL_MASK, nv4097::set_surface_options_dirty_bit>();
		bind<NV4097_SET_BACK_STENCIL_OP_ZPASS, nv4097::set_stencil_op>();
		bind<NV4097_SET_BACK_STENCIL_OP_FAIL, nv4097::set_stencil_op>();
		bind<NV4097_SET_BACK_STENCIL_OP_ZFAIL, nv4097::set_stencil_op>();
		bind<NV4097_WAIT_FOR_IDLE, nv4097::sync>();
		bind<NV4097_INVALIDATE_L2, nv4097::set_shader_program_dirty>();
		bind<NV4097_SET_SHADER_PROGRAM, nv4097::set_shader_program_dirty>();
		bind<NV4097_SET_SHADER_CONTROL, nv4097::notify_state_changed<fragment_program_state_dirty>>();
		bind_array<NV4097_SET_TEX_COORD_CONTROL, 1, 10, nv4097::notify_state_changed<fragment_program_state_dirty>>();
		bind<NV4097_SET_TWO_SIDE_LIGHT_EN, nv4097::notify_state_changed<fragment_program_state_dirty>>();
		bind<NV4097_SET_POINT_SPRITE_CONTROL, nv4097::notify_state_changed<fragment_program_state_dirty>>();
		bind<NV4097_SET_TRANSFORM_PROGRAM_START, nv4097::set_transform_program_start>();
		bind<NV4097_SET_VERTEX_ATTRIB_OUTPUT_MASK, nv4097::set_vertex_attribute_output_mask>();
		bind<NV4097_SET_VERTEX_DATA_BASE_OFFSET, nv4097::set_vertex_base_offset>();
		bind<NV4097_SET_VERTEX_DATA_BASE_INDEX, nv4097::set_index_base_offset>();
		bind<NV4097_SET_USER_CLIP_PLANE_CONTROL, nv4097::notify_state_changed<vertex_state_dirty>>();
		bind<NV4097_SET_TRANSFORM_BRANCH_BITS, nv4097::notify_state_changed<vertex_state_dirty>>();
		bind<NV4097_SET_CLIP_MIN, nv4097::notify_state_changed<invalidate_zclip_bits>>();
		bind<NV4097_SET_CLIP_MAX, nv4097::notify_state_changed<invalidate_zclip_bits>>();
		bind<NV4097_SET_POINT_SIZE, nv4097::notify_state_changed<vertex_state_dirty>>();
		bind<NV4097_SET_ALPHA_FUNC, nv4097::notify_state_changed<fragment_state_dirty>>();
		bind<NV4097_SET_ALPHA_REF, nv4097::notify_state_changed<fragment_state_dirty>>();
		bind<NV4097_SET_ALPHA_TEST_ENABLE, nv4097::notify_state_changed<fragment_state_dirty>>();
		bind<NV4097_SET_ANTI_ALIASING_CONTROL, nv4097::notify_state_changed<fragment_state_dirty>>();
		bind<NV4097_SET_SHADER_PACKER, nv4097::notify_state_changed<fragment_state_dirty>>();
		bind<NV4097_SET_SHADER_WINDOW, nv4097::notify_state_changed<fragment_state_dirty>>();
		bind<NV4097_SET_FOG_MODE, nv4097::notify_state_changed<fragment_state_dirty>>();
		bind<NV4097_SET_SCISSOR_HORIZONTAL, nv4097::notify_state_changed<scissor_config_state_dirty>>();
		bind<NV4097_SET_SCISSOR_VERTICAL, nv4097::notify_state_changed<scissor_config_state_dirty>>();
		bind<NV4097_SET_VIEWPORT_HORIZONTAL, nv4097::notify_state_changed<scissor_config_state_dirty>>();
		bind<NV4097_SET_VIEWPORT_VERTICAL, nv4097::notify_state_changed<scissor_config_state_dirty>>();
		bind_array<NV4097_SET_FOG_PARAMS, 1, 2, nv4097::notify_state_changed<fragment_state_dirty>>();
		bind_array<NV4097_SET_VIEWPORT_SCALE, 1, 3, nv4097::notify_state_changed<vertex_state_dirty>>();
		bind_array<NV4097_SET_VIEWPORT_OFFSET, 1, 3, nv4097::notify_state_changed<vertex_state_dirty>>();
		bind<NV4097_SET_INDEX_ARRAY_DMA, nv4097::check_index_array_dma>();
		bind<NV4097_SET_BLEND_EQUATION, nv4097::set_blend_equation>();
		bind<NV4097_SET_BLEND_FUNC_SFACTOR, nv4097::set_blend_factor>();
		bind<NV4097_SET_BLEND_FUNC_DFACTOR, nv4097::set_blend_factor>();
		bind<NV4097_SET_POLYGON_STIPPLE, nv4097::notify_state_changed<fragment_state_dirty>>();
		bind_array<NV4097_SET_POLYGON_STIPPLE_PATTERN, 1, 32, nv4097::notify_state_changed<polygon_stipple_pattern_dirty>>();

		//NV308A (0xa400..0xbffc!)
		bind_range<NV308A_COLOR + (256 * 0), 1, 256, nv308a::color, 256 * 0>();
		bind_range<NV308A_COLOR + (256 * 1), 1, 256, nv308a::color, 256 * 1>();
		bind_range<NV308A_COLOR + (256 * 2), 1, 256, nv308a::color, 256 * 2>();
		bind_range<NV308A_COLOR + (256 * 3), 1, 256, nv308a::color, 256 * 3>();
		bind_range<NV308A_COLOR + (256 * 4), 1, 256, nv308a::color, 256 * 4>();
		bind_range<NV308A_COLOR + (256 * 5), 1, 256, nv308a::color, 256 * 5>();
		bind_range<NV308A_COLOR + (256 * 6), 1, 256, nv308a::color, 256 * 6>();

		//NV3089
		bind<NV3089_IMAGE_IN, nv3089::image_in>();

		//NV0039
		bind<NV0039_BUFFER_NOTIFY, nv0039::buffer_notify>();

		// lv1 hypervisor
		bind_array<GCM_SET_USER_COMMAND, 1, 2, user_command>();
		bind_range<GCM_FLIP_HEAD, 1, 2, gcm::driver_flip>();
		bind_range<GCM_DRIVER_QUEUE, 1, 8, gcm::queue_flip>();

		// custom methods
		bind<GCM_FLIP_COMMAND, flip_command>();

		// FIFO
		bind<(FIFO::FIFO_DRAW_BARRIER >> 2), fifo::draw_barrier>();

		return true;
	}();
}
