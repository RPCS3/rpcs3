#include "stdafx.h"
#include "rsx_methods.h"
#include "RSXThread.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "rsx_utils.h"
#include "rsx_decode.h"
#include "Emu/Cell/PPUCallback.h"
#include "Emu/Cell/lv2/sys_rsx.h"

#include <sstream>
#include <cereal/archives/binary.hpp>

#include <thread>

template <>
void fmt_class_string<frame_limit_type>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](frame_limit_type value)
	{
		switch (value)
		{
		case frame_limit_type::none: return "Off";
		case frame_limit_type::_59_94: return "59.94";
		case frame_limit_type::_50: return "50";
		case frame_limit_type::_60: return "60";
		case frame_limit_type::_30: return "30";
		case frame_limit_type::_auto: return "Auto";
		}

		return unknown;
	});
}

namespace rsx
{
	rsx_state method_registers;
	
	std::array<rsx_method_t, 0x10000 / 4> methods{};

	void invalid_method(thread* rsx, u32 _reg, u32 arg)
	{
		//Don't throw, gather information and ignore broken/garbage commands
		//TODO: Investigate why these commands are executed at all. (Heap corruption? Alignment padding?)
		LOG_ERROR(RSX, "Invalid RSX method 0x%x (arg=0x%x)" HERE, _reg << 2, arg);
		rsx->invalid_command_interrupt_raised = true;
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
			rsx->ctrl->ref.exchange(arg);
		}

		void semaphore_acquire(thread* rsx, u32 _reg, u32 arg)
		{
			const u32 addr = get_address(method_registers.semaphore_offset_406e(), method_registers.semaphore_context_dma_406e());
			while (vm::ps3::read32(addr) != arg)
			{
				// todo: LLE: why does this one keep hanging? is it vsh system semaphore? whats actually pushing this to the command buffer?!
				if (addr == 0x40000030)
					break;

				if (Emu.IsStopped())
					break;

				std::this_thread::yield();
			}
		}

		void semaphore_release(thread* rsx, u32 _reg, u32 arg)
		{
			const u32 addr = get_address(method_registers.semaphore_offset_406e(), method_registers.semaphore_context_dma_406e());

			if (addr >> 28 == 0x4)
			{
				// TODO: check no reservation area instead
				vm::ps3::write32(addr, arg);
				return;
			}

			vm::reader_lock lock;
			vm::ps3::write32(addr, arg);
			vm::notify(addr, 4);
		}
	}

	namespace nv4097
	{
		void clear(thread* rsx, u32 _reg, u32 arg)
		{
			// TODO: every backend must override method table to insert its own handlers
			if (!rsx->do_method(NV4097_CLEAR_SURFACE, arg))
			{
				//
			}

			if (rsx->capture_current_frame)
			{
				rsx->capture_frame("clear");
			}
		}

		void clear_zcull(thread* rsx, u32 _reg, u32 arg)
		{
			rsx->do_method(NV4097_CLEAR_ZCULL_SURFACE, arg);

			if (rsx->capture_current_frame)
			{
				rsx->capture_frame("clear zcull memory");
			}
		}

		void texture_read_semaphore_release(thread* rsx, u32 _reg, u32 arg)
		{
			const u32 index = method_registers.semaphore_offset_4097() >> 4;
			// lle-gcm likes to inject system reserved semaphores, presumably for system/vsh usage
			// Avoid calling render to avoid any havoc(flickering) they may cause from invalid flush/write

			if (index > 63 && !rsx->do_method(NV4097_TEXTURE_READ_SEMAPHORE_RELEASE, arg))
			{
				//
			}
			auto& sema = vm::ps3::_ref<RsxReports>(rsx->label_addr);
			sema.semaphore[index].val = arg;
			sema.semaphore[index].pad = 0;
			sema.semaphore[index].timestamp = rsx->timestamp();
		}

		void back_end_write_semaphore_release(thread* rsx, u32 _reg, u32 arg)
		{
			const u32 index = method_registers.semaphore_offset_4097() >> 4;
			if (index > 63 && !rsx->do_method(NV4097_BACK_END_WRITE_SEMAPHORE_RELEASE, arg))
			{
				//
			}
			u32 val = (arg & 0xff00ff00) | ((arg & 0xff) << 16) | ((arg >> 16) & 0xff);

			auto& sema = vm::ps3::_ref<RsxReports>(rsx->label_addr);
			sema.semaphore[index].val = val;
			sema.semaphore[index].pad = 0;
			sema.semaphore[index].timestamp = rsx->timestamp();
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
			static const size_t increment_per_array_index = (register_count * sizeof(type)) / sizeof(u32);

			static const size_t attribute_index = index / increment_per_array_index;
			static const size_t vertex_subreg = index % increment_per_array_index;

			const auto vtype = vertex_data_type_from_element_type<type>::type;

			if (rsx->in_begin_end)
				rsx->append_to_push_buffer(attribute_index, count, vertex_subreg, vtype, arg);

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

			rsx::method_registers.current_draw_clause.first_count_commands.emplace_back(
			    std::make_pair(v.start(), v.count()));
		}

		void draw_index_array(thread* rsx, u32 _reg, u32 arg)
		{
			rsx::method_registers.current_draw_clause.command = rsx::draw_command::indexed;
			rsx::registers_decoder<NV4097_DRAW_INDEX_ARRAY>::decoded_type v(arg);

			rsx::method_registers.current_draw_clause.first_count_commands.emplace_back(
			    std::make_pair(v.start(), v.count()));
		}

		void draw_inline_array(thread* rsx, u32 _reg, u32 arg)
		{
			rsx::method_registers.current_draw_clause.command = rsx::draw_command::inlined_array;
			rsx::method_registers.current_draw_clause.inline_vertex_array.push_back(arg);
		}

		template<u32 index>
		struct set_transform_constant
		{
			static void impl(thread* rsxthr, u32 _reg, u32 arg)
			{
				static constexpr u32 reg = index / 4;
				static constexpr u8 subreg = index % 4;

				u32 load = rsx::method_registers.transform_constant_load();
				rsx::method_registers.transform_constants[load + reg].rgba[subreg] = (f32&)arg;
				rsxthr->m_transform_constants_dirty = true;
			}
		};

		template<u32 index>
		struct set_transform_program
		{
			static void impl(thread* rsx, u32 _reg, u32 arg)
			{
				method_registers.commit_4_transform_program_instructions(index);
			}
		};

		void set_begin_end(thread* rsxthr, u32 _reg, u32 arg)
		{
			if (arg)
			{
				rsx::method_registers.current_draw_clause.first_count_commands.resize(0);
				rsx::method_registers.current_draw_clause.command = draw_command::none;
				rsx::method_registers.current_draw_clause.primitive = rsx::method_registers.primitive_mode();
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
					rsx::method_registers.current_draw_clause.first_count_commands.push_back(std::make_pair(0, push_buffer_index_count));
				}
				else if (push_buffer_vertices_count)
				{
					rsx::method_registers.current_draw_clause.command = rsx::draw_command::array;
					rsx::method_registers.current_draw_clause.first_count_commands.push_back(std::make_pair(0, push_buffer_vertices_count));
				}
			}
			else
				rsx::method_registers.current_draw_clause.is_immediate_draw = false;

			if (!(rsx::method_registers.current_draw_clause.first_count_commands.empty() &&
			        rsx::method_registers.current_draw_clause.inline_vertex_array.empty()))
			{
				rsxthr->end();
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
				LOG_ERROR(RSX, "Bad argument passed to NV4097_GET_REPORT, arg=0x%X", arg);
				return;
			}

			vm::ps3::ptr<CellGcmReportData> result = address_ptr;

			switch (type)
			{
			case CELL_GCM_ZPASS_PIXEL_CNT:
			case CELL_GCM_ZCULL_STATS:
			case CELL_GCM_ZCULL_STATS1:
			case CELL_GCM_ZCULL_STATS2:
			case CELL_GCM_ZCULL_STATS3:
				result->value = rsx->get_zcull_stats(type);
				LOG_WARNING(RSX, "NV4097_GET_REPORT: Unimplemented type %d", type);
				break;
			default:
				LOG_ERROR(RSX, "NV4097_GET_REPORT: Bad type %d", type);
				break;
			}

			result->timer = rsx->timestamp();
			result->padding = 0;
		}

		void clear_report_value(thread* rsx, u32 _reg, u32 arg)
		{
			switch (arg)
			{
			case CELL_GCM_ZPASS_PIXEL_CNT:
				LOG_WARNING(RSX, "TODO: NV4097_CLEAR_REPORT_VALUE: ZPASS_PIXEL_CNT");
				break;
			case CELL_GCM_ZCULL_STATS:
				LOG_WARNING(RSX, "TODO: NV4097_CLEAR_REPORT_VALUE: ZCULL_STATS");
				break;
			default:
				LOG_ERROR(RSX, "NV4097_CLEAR_REPORT_VALUE: Bad type: %d", arg);
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
				rsx->conditional_render_enabled = false;
				rsx->conditional_render_test_failed = false;
				return;
			case 2:
				rsx->conditional_render_enabled = true;
				LOG_WARNING(RSX, "Conditional rendering mode enabled (mode 2)");
				break;
			default:
				rsx->conditional_render_enabled = false;
				LOG_ERROR(RSX, "Unknown render mode %d", mode);
				return;
			}

			const u32 offset = arg & 0xffffff;
			auto address_ptr = get_report_data_impl(offset);

			if (!address_ptr)
			{
				rsx->conditional_render_test_failed = false;
				LOG_ERROR(RSX, "Bad argument passed to NV4097_SET_RENDER_ENABLE, arg=0x%X", arg);
				return;
			}

			vm::ps3::ptr<CellGcmReportData> result = address_ptr;
			rsx->conditional_render_test_failed = (result->value == 0);
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

		void set_surface_dirty_bit(thread* rsx, u32 _reg, u32)
		{
			rsx->m_rtts_dirty = true;
		}

		void set_surface_options_dirty_bit(thread* rsx, u32, u32)
		{
			if (rsx->m_framebuffer_state_contested)
				rsx->m_rtts_dirty = true;
		}

		template<u32 index>
		struct set_texture_dirty_bit
		{
			static void impl(thread* rsx, u32 _reg, u32 arg)
			{
				rsx->m_textures_dirty[index] = true;
			}
		};

		template<u32 index>
		struct set_vertex_texture_dirty_bit
		{
			static void impl(thread* rsx, u32 _reg, u32 arg)
			{
				rsx->m_vertex_textures_dirty[index] = true;
			}
		};
	}

	namespace nv308a
	{
		template<u32 index>
		struct color
		{
			static void impl(thread* rsx, u32 _reg, u32 arg)
			{
				u16 x = method_registers.nv308a_x();
				u16 y = method_registers.nv308a_y();

				if (y)
				{
					LOG_ERROR(RSX, "%s: y is not null (0x%x)", __FUNCTION__, y);
				}

				const u32 pixel_offset = (method_registers.blit_engine_output_pitch_nv3062() * y) + (x << 2);
				u32 address = get_address(method_registers.blit_engine_output_offset_nv3062() + pixel_offset + index * 4, method_registers.blit_engine_output_location_nv3062());
				vm::ps3::write32(address, arg);
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
			const rsx::blit_engine::transfer_source_format src_color_format = method_registers.blit_engine_src_color_format();

			const f32 in_x = method_registers.blit_engine_in_x();
			const f32 in_y = method_registers.blit_engine_in_y();

			//Clipping
			//Validate that clipping rect will fit onto both src and dst regions
			u16 clip_w = std::min(method_registers.blit_engine_clip_width(), out_w);
			u16 clip_h = std::min(method_registers.blit_engine_clip_height(), out_h);

			u16 clip_x = method_registers.blit_engine_clip_x();
			u16 clip_y = method_registers.blit_engine_clip_y();

			if (clip_w == 0)
			{
				clip_x = 0;
				clip_w = out_w;
			}

			if (clip_h == 0)
			{
				clip_y = 0;
				clip_h = out_h;
			}

			//Fit onto dst
			if (clip_x && (out_x + clip_x + clip_w) > out_w) clip_x = 0;
			if (clip_y && (out_y + clip_y + clip_h) > out_h) clip_y = 0;

			u16 in_pitch = method_registers.blit_engine_input_pitch();

			if (in_w == 0 || in_h == 0 || out_w == 0 || out_h == 0)
			{
				LOG_ERROR(RSX, "NV3089_IMAGE_IN_SIZE: Invalid blit dimensions passed");
				return;
			}

			if (in_origin != blit_engine::transfer_origin::corner)
			{
				// Probably refers to texel geometry which would affect clipping algorithm slightly when rounding texel addresses
				LOG_WARNING(RSX, "NV3089_IMAGE_IN_SIZE: unknown origin (%d)", (u8)in_origin);
			}

			if (operation != rsx::blit_engine::transfer_operation::srccopy)
			{
				LOG_ERROR(RSX, "NV3089_IMAGE_IN_SIZE: unknown operation (%d)", (u8)operation);
			}

			const u32 src_offset = method_registers.blit_engine_input_offset();
			const u32 src_dma = method_registers.blit_engine_input_location();

			u32 dst_offset;
			u32 dst_dma = 0;
			rsx::blit_engine::transfer_destination_format dst_color_format;
			u32 out_pitch = 0;
			u32 out_aligment = 64;

			switch (method_registers.blit_engine_context_surface())
			{
			case blit_engine::context_surface::surface2d:
				dst_dma = method_registers.blit_engine_output_location_nv3062();
				dst_offset = method_registers.blit_engine_output_offset_nv3062();
				dst_color_format = method_registers.blit_engine_nv3062_color_format();
				out_pitch = method_registers.blit_engine_output_pitch_nv3062();
				out_aligment = method_registers.blit_engine_output_alignment_nv3062();
				break;

			case blit_engine::context_surface::swizzle2d:
				dst_dma = method_registers.blit_engine_nv309E_location();
				dst_offset = method_registers.blit_engine_nv309E_offset();
				dst_color_format = method_registers.blit_engine_output_format_nv309E();
				break;

			default:
				LOG_ERROR(RSX, "NV3089_IMAGE_IN_SIZE: unknown m_context_surface (0x%x)", (u8)method_registers.blit_engine_context_surface());
				return;
			}

			const u32 in_bpp = (src_color_format == rsx::blit_engine::transfer_source_format::r5g6b5) ? 2 : 4; // bytes per pixel
			const u32 out_bpp = (dst_color_format == rsx::blit_engine::transfer_destination_format::r5g6b5) ? 2 : 4;

			const u32 in_offset = u32(in_x * in_bpp + in_pitch * in_y);
			const s32 out_offset = out_x * out_bpp + out_pitch * out_y;

			const tiled_region src_region = rsx->get_tiled_address(src_offset + in_offset, src_dma & 0xf);
			const tiled_region dst_region = rsx->get_tiled_address(dst_offset + out_offset, dst_dma & 0xf);

			u8* pixels_src = src_region.tile ? src_region.ptr + src_region.base : src_region.ptr;
			u8* pixels_dst = vm::ps3::_ptr<u8>(get_address(dst_offset + out_offset, dst_dma));

			if (out_pitch == 0)
			{
				out_pitch = out_bpp * out_w;
			}

			if (in_pitch == 0)
			{
				in_pitch = in_bpp * in_w;
			}

			if (dst_color_format != rsx::blit_engine::transfer_destination_format::r5g6b5 &&
				dst_color_format != rsx::blit_engine::transfer_destination_format::a8r8g8b8)
			{
				LOG_ERROR(RSX, "NV3089_IMAGE_IN_SIZE: unknown dst_color_format (%d)", (u8)dst_color_format);
			}

			if (src_color_format != rsx::blit_engine::transfer_source_format::r5g6b5 &&
				src_color_format != rsx::blit_engine::transfer_source_format::a8r8g8b8)
			{
				LOG_ERROR(RSX, "NV3089_IMAGE_IN_SIZE: unknown src_color_format (%d)", (u8)src_color_format);
			}

			f32 scale_x = 1048576.f / method_registers.blit_engine_ds_dx();
			f32 scale_y = 1048576.f / method_registers.blit_engine_dt_dy();

			u32 convert_w = (u32)(scale_x * in_w);
			u32 convert_h = (u32)(scale_y * in_h);

			if (convert_w == 0 || convert_h == 0)
			{
				LOG_ERROR(RSX, "NV3089_IMAGE_IN: Invalid dimensions or scaling factor. Request ignored");
				return;
			}

			u32 slice_h = clip_h;
			blit_src_info src_info = {};
			blit_dst_info dst_info = {};

			if (src_region.tile)
			{
				switch(src_region.tile->comp)
				{
				case CELL_GCM_COMPMODE_C32_2X2:
					slice_h *= 2;
					src_info.compressed_y = true;
				case CELL_GCM_COMPMODE_C32_2X1:
					src_info.compressed_x = true;
					break;
				}

				u32 size = slice_h * in_pitch;

				if (size > src_region.tile->size - src_region.base)
				{
					u32 diff = size - (src_region.tile->size - src_region.base);
					slice_h -= diff / in_pitch + (diff % in_pitch ? 1 : 0);
				}
			}

			if (dst_region.tile)
			{
				switch (dst_region.tile->comp)
				{
				case CELL_GCM_COMPMODE_C32_2X2:
					dst_info.compressed_y = true;
				case CELL_GCM_COMPMODE_C32_2X1:
					dst_info.compressed_x = true;
					break;
				}

				dst_info.max_tile_h = static_cast<u16>((dst_region.tile->size - dst_region.base) / out_pitch);
			}

			if (!g_cfg.video.force_cpu_blit_processing && (dst_dma == CELL_GCM_CONTEXT_DMA_MEMORY_FRAME_BUFFER || src_dma == CELL_GCM_CONTEXT_DMA_MEMORY_FRAME_BUFFER))
			{
				//For now, only use this for actual scaled images, there are use cases that should not go through 3d engine, e.g program ucode transfer
				//TODO: Figure out more instances where we can use this without problems
				//NOTE: In cases where slice_h is modified due to compression (read from tiled memory), the new value (clip_h * 2) does not matter if memory is on the GPU
				src_info.format = src_color_format;
				src_info.origin = in_origin;
				src_info.width = in_w;
				src_info.height = in_h;
				src_info.pitch = in_pitch;
				src_info.slice_h = slice_h;
				src_info.offset_x = (u16)in_x;
				src_info.offset_y = (u16)in_y;
				src_info.pixels = pixels_src;
				src_info.rsx_address = get_address(src_offset, src_dma);

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
				dst_info.pixels = pixels_dst;
				dst_info.rsx_address = get_address(dst_offset, dst_dma);
				dst_info.swizzled = (method_registers.blit_engine_context_surface() == blit_engine::context_surface::swizzle2d);

				if (rsx->scaled_image_from_memory(src_info, dst_info, in_inter == blit_engine::transfer_interpolator::foh))
					return;
			}

			std::unique_ptr<u8[]> temp1, temp2, sw_temp;

			const AVPixelFormat in_format = (src_color_format == rsx::blit_engine::transfer_source_format::r5g6b5) ? AV_PIX_FMT_RGB565BE : AV_PIX_FMT_ARGB;
			const AVPixelFormat out_format = (dst_color_format == rsx::blit_engine::transfer_destination_format::r5g6b5) ? AV_PIX_FMT_RGB565BE : AV_PIX_FMT_ARGB;

			const bool need_clip =
				clip_w != in_w ||
				clip_h != in_h ||
				clip_x > 0 || clip_y > 0 ||
				convert_w != out_w || convert_h != out_h;

			const bool need_convert = out_format != in_format || scale_x != 1.0 || scale_y != 1.0;

			if (method_registers.blit_engine_context_surface() != blit_engine::context_surface::swizzle2d)
			{
				if (need_convert || need_clip)
				{					
					if (need_clip)
					{
						if (need_convert)
						{
							convert_scale_image(temp1, out_format, convert_w, convert_h, out_pitch,
								pixels_src, in_format, in_w, in_h, in_pitch, slice_h, in_inter == blit_engine::transfer_interpolator::foh);

							clip_image(pixels_dst, temp1.get(), clip_x, clip_y, clip_w, clip_h, out_bpp, out_pitch, out_pitch);
						}
						else
						{
							clip_image(pixels_dst, pixels_src, clip_x, clip_y, clip_w, clip_h, out_bpp, in_pitch, out_pitch);
						}
					}
					else
					{
						convert_scale_image(pixels_dst, out_format, out_w, out_h, out_pitch,
							pixels_src, in_format, in_w, in_h, in_pitch, slice_h, in_inter == blit_engine::transfer_interpolator::foh);
					}
				}
				else
				{
					if (out_pitch != in_pitch || out_pitch != out_bpp * out_w)
					{
						for (u32 y = 0; y < out_h; ++y)
						{
							u8 *dst = pixels_dst + out_pitch * y;
							u8 *src = pixels_src + in_pitch * y;

							std::memmove(dst, src, out_w * out_bpp);
						}
					}
					else
					{
						std::memmove(pixels_dst, pixels_src, out_pitch * out_h);
					}
				}
			}
			else
			{
				if (need_convert || need_clip)
				{
					if (need_clip)
					{
						if (need_convert)
						{
							convert_scale_image(temp1, out_format, convert_w, convert_h, out_pitch,
								pixels_src, in_format, in_w, in_h, in_pitch, slice_h, in_inter == blit_engine::transfer_interpolator::foh);

							clip_image(temp2, temp1.get(), clip_x, clip_y, clip_w, clip_h, out_bpp, out_pitch, out_pitch);
						}
						else
						{
							clip_image(temp2, pixels_src, clip_x, clip_y, clip_w, clip_h, out_bpp, in_pitch, out_pitch);
						}
					}
					else
					{
						convert_scale_image(temp2, out_format, out_w, out_h, out_pitch,
							pixels_src, in_format, in_w, in_h, in_pitch, clip_h, in_inter == blit_engine::transfer_interpolator::foh);
					}

					pixels_src = temp2.get();
				}

				u8 sw_width_log2 = method_registers.nv309e_sw_width_log2();
				u8 sw_height_log2 = method_registers.nv309e_sw_height_log2();

				// 0 indicates height of 1 pixel
				sw_height_log2 = sw_height_log2 == 0 ? 1 : sw_height_log2;

				// swizzle based on destination size
				u16 sw_width = 1 << sw_width_log2;
				u16 sw_height = 1 << sw_height_log2;

				temp2.reset(new u8[out_bpp * sw_width * sw_height]);

				u8* linear_pixels = pixels_src;
				u8* swizzled_pixels = temp2.get();

				// restrict output to size of swizzle
				const u16 sw_in_w = std::min(out_w, sw_width);
				const u16 sw_in_h = std::min(out_h, sw_height);

				// Check and pad texture out if we are given non square texture for swizzle to be correct
				if (sw_width != sw_in_w || sw_height != sw_in_h)
				{
					sw_temp.reset(new u8[out_bpp * sw_width * sw_height]);

					switch (out_bpp)
					{
					case 1:
						pad_texture<u8>(linear_pixels, sw_temp.get(), sw_in_w, sw_in_h, sw_width, sw_height);
						break;
					case 2:
						pad_texture<u16>(linear_pixels, sw_temp.get(), sw_in_w, sw_in_h, sw_width, sw_height);
						break;
					case 4:
						pad_texture<u32>(linear_pixels, sw_temp.get(), sw_in_w, sw_in_h, sw_width, sw_height);
						break;
					}

					linear_pixels = sw_temp.get();
				}

				switch (out_bpp)
				{
				case 1:
					convert_linear_swizzle<u8>(linear_pixels, swizzled_pixels, sw_width, sw_height, false);
					break;
				case 2:
					convert_linear_swizzle<u16>(linear_pixels, swizzled_pixels, sw_width, sw_height, false);
					break;
				case 4:
					convert_linear_swizzle<u32>(linear_pixels, swizzled_pixels, sw_width, sw_height, false);
					break;
				}

				std::memcpy(pixels_dst, swizzled_pixels, out_bpp * sw_width * sw_height);
			}
		}
	}

	namespace nv0039
	{
		void buffer_notify(thread*, u32, u32 arg)
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
				LOG_ERROR(RSX, "NV0039_OFFSET_IN: Unsupported format: inFormat=%d, outFormat=%d", in_format, out_format);
			}

			LOG_NOTICE(RSX, "NV0039_OFFSET_IN: pitch(in=0x%x, out=0x%x), line(len=0x%x, cnt=0x%x), fmt(in=0x%x, out=0x%x), notify=0x%x",
				in_pitch, out_pitch, line_length, line_count, in_format, out_format, notify);

			if (!in_pitch)
			{
				in_pitch = line_length;
			}

			if (!out_pitch)
			{
				out_pitch = line_length;
			}

			u32 src_offset = method_registers.nv0039_input_offset();
			u32 src_dma = method_registers.nv0039_input_location();

			u32 dst_offset = method_registers.nv0039_output_offset();
			u32 dst_dma = method_registers.nv0039_output_location();

			u8 *dst = (u8*)vm::base(get_address(dst_offset, dst_dma));
			const u8 *src = (u8*)vm::base(get_address(src_offset, src_dma));

			if (in_pitch == out_pitch && out_pitch == line_length)
			{
				std::memcpy(dst, src, line_length * line_count);
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
	}

	void flip_command(thread* rsx, u32, u32 arg)
	{
		if (user_asked_for_frame_capture)
		{
			rsx->capture_current_frame = true;
			user_asked_for_frame_capture = false;
			frame_debug.reset();
		}
		else if (rsx->capture_current_frame)
		{
			rsx->capture_current_frame = false;
			std::stringstream os;
			cereal::BinaryOutputArchive archive(os);
			archive(frame_debug);
			{
				fs::file f(fs::get_config_dir() + "capture.txt", fs::rewrite);
				f.write(os.str());
			}
			Emu.Pause();
		}

		double limit = 0.;
		switch (g_cfg.video.frame_limit)
		{
		case frame_limit_type::none: limit = 0.; break;
		case frame_limit_type::_59_94: limit = 59.94; break;
		case frame_limit_type::_50: limit = 50.; break;
		case frame_limit_type::_60: limit = 60.; break;
		case frame_limit_type::_30: limit = 30.; break;
		case frame_limit_type::_auto: limit = rsx->fps_limit; break; // TODO
		}

		if (limit)
		{
			const u64 time = get_system_time() - Emu.GetPauseTime() - rsx->start_rsx_time;

			if (rsx->int_flip_index == 0)
			{
				rsx->start_rsx_time = time;
			}
			else
			{
				// Convert limit to expected time value
				double expected = rsx->int_flip_index * 1000000. / limit;

				while (time >= expected + 1000000. / limit)
				{
					expected = rsx->int_flip_index++ * 1000000. / limit;
				}

				if (expected > time + 1000)
				{
					std::this_thread::sleep_for(std::chrono::milliseconds{static_cast<s64>(expected - time) / 1000});
				}
			}
		}
		
		rsx->int_flip_index++;
		rsx->current_display_buffer = arg;
		rsx->flip(arg);
		// After each flip PS3 system is executing a routine that changes registers value to some default.
		// Some game use this default state (SH3).
		if (rsx->isHLE)
			rsx->reset();

		rsx->last_flip_time = get_system_time() - 1000000;
		rsx->flip_status = CELL_GCM_DISPLAY_FLIP_STATUS_DONE;

		if (rsx->flip_handler)
		{
			rsx->intr_thread->cmd_list
			({
				{ ppu_cmd::set_args, 1 }, u64{1},
				{ ppu_cmd::lle_call, rsx->flip_handler },
				{ ppu_cmd::sleep, 0 }
			});

			rsx->intr_thread->notify();
		}
	}

	void user_command(thread* rsx, u32, u32 arg)
	{
		sys_rsx_context_attribute(0x55555555, 0xFEF, 0, arg, 0, 0);
		if (rsx->user_handler)
		{
			rsx->intr_thread->cmd_list
			({
				{ ppu_cmd::set_args, 1 }, u64{arg},
				{ ppu_cmd::lle_call, rsx->user_handler },
				{ ppu_cmd::sleep, 0 }
			});

			rsx->intr_thread->notify();
		}
	}

	namespace gcm
	{
		// not entirely sure which one should actually do the flip, or if these should be handled seperately,
		// so for now lets flip in queue and just let the driver deal with it
		template<u32 index>
		struct driver_flip
		{
			static void impl(thread* rsx, u32 _reg, u32 arg)
			{
				rsx->reset();
				sys_rsx_context_attribute(0x55555555, 0x102, index, arg, 0, 0);
			}
		};

		template<u32 index>
		struct queue_flip
		{
			static void impl(thread* rsx, u32 _reg, u32 arg)
			{
				flip_command(rsx, _reg, arg);
				sys_rsx_context_attribute(0x55555555, 0x103, index, arg, 0, 0);
			}
		};
	}

	void rsx_state::reset()
	{
		//setup method registers
		std::memset(registers.data(), 0, registers.size() * sizeof(u32));

		registers[NV4097_SET_COLOR_MASK] = CELL_GCM_COLOR_MASK_R | CELL_GCM_COLOR_MASK_G | CELL_GCM_COLOR_MASK_B | CELL_GCM_COLOR_MASK_A;
		registers[NV4097_SET_SCISSOR_HORIZONTAL] = (4096 << 16) | 0;
		registers[NV4097_SET_SCISSOR_VERTICAL] = (4096 << 16) | 0;

		registers[NV4097_SET_ALPHA_FUNC] = CELL_GCM_ALWAYS;
		registers[NV4097_SET_ALPHA_REF] = 0;

		registers[NV4097_SET_BLEND_FUNC_SFACTOR] = (CELL_GCM_ONE << 16) | CELL_GCM_ONE;
		registers[NV4097_SET_BLEND_FUNC_DFACTOR] = (CELL_GCM_ZERO << 16) | CELL_GCM_ZERO;
		registers[NV4097_SET_BLEND_COLOR] = 0;
		registers[NV4097_SET_BLEND_COLOR2] = 0;
		registers[NV4097_SET_BLEND_EQUATION] = (0x8006 << 16) | 0x8006; // (add)

		registers[NV4097_SET_STENCIL_MASK] = 0xff;
		registers[NV4097_SET_STENCIL_FUNC] = CELL_GCM_ALWAYS;
		registers[NV4097_SET_STENCIL_FUNC_REF] = 0x00;
		registers[NV4097_SET_STENCIL_FUNC_MASK] = 0xff;
		registers[NV4097_SET_STENCIL_OP_FAIL] = CELL_GCM_KEEP;
		registers[NV4097_SET_STENCIL_OP_ZFAIL] = CELL_GCM_KEEP;
		registers[NV4097_SET_STENCIL_OP_ZPASS] = CELL_GCM_KEEP;

		registers[NV4097_SET_BACK_STENCIL_MASK] = 0xff;
		registers[NV4097_SET_BACK_STENCIL_FUNC] = CELL_GCM_ALWAYS;
		registers[NV4097_SET_BACK_STENCIL_FUNC_REF] = 0x00;
		registers[NV4097_SET_BACK_STENCIL_FUNC_MASK] = 0xff;
		registers[NV4097_SET_BACK_STENCIL_OP_FAIL] = CELL_GCM_KEEP;
		registers[NV4097_SET_BACK_STENCIL_OP_ZFAIL] = CELL_GCM_KEEP;
		registers[NV4097_SET_BACK_STENCIL_OP_ZPASS] = CELL_GCM_KEEP;

		//registers[NV4097_SET_SHADE_MODE] = CELL_GCM_SMOOTH;

		//registers[NV4097_SET_LOGIC_OP] = CELL_GCM_COPY;

		(f32&)registers[NV4097_SET_DEPTH_BOUNDS_MIN] = 0.f;
		(f32&)registers[NV4097_SET_DEPTH_BOUNDS_MAX] = 1.f;

		(f32&)registers[NV4097_SET_CLIP_MIN] = 0.f;
		(f32&)registers[NV4097_SET_CLIP_MAX] = 1.f;

		registers[NV4097_SET_LINE_WIDTH] = 1 << 3;

		// These defaults were found using After Burner Climax (which never set fog mode despite using fog input)
		registers[NV4097_SET_FOG_MODE] = CELL_GCM_FOG_MODE_LINEAR;
		(f32&)registers[NV4097_SET_FOG_PARAMS] = 1.;
		(f32&)registers[NV4097_SET_FOG_PARAMS + 1] = 1.;

		registers[NV4097_SET_DEPTH_FUNC] = CELL_GCM_LESS;
		registers[NV4097_SET_DEPTH_MASK] = CELL_GCM_TRUE;
		(f32&)registers[NV4097_SET_POLYGON_OFFSET_SCALE_FACTOR] = 0.f;
		(f32&)registers[NV4097_SET_POLYGON_OFFSET_BIAS] = 0.f;
		//registers[NV4097_SET_FRONT_POLYGON_MODE] = CELL_GCM_POLYGON_MODE_FILL;
		//registers[NV4097_SET_BACK_POLYGON_MODE] = CELL_GCM_POLYGON_MODE_FILL;
		registers[NV4097_SET_CULL_FACE] = CELL_GCM_BACK;
		registers[NV4097_SET_FRONT_FACE] = CELL_GCM_CCW;
		registers[NV4097_SET_RESTART_INDEX] = -1;

		registers[NV4097_SET_CLEAR_RECT_HORIZONTAL] = (4096 << 16) | 0;
		registers[NV4097_SET_CLEAR_RECT_VERTICAL] = (4096 << 16) | 0;

		// Stencil bits init to 00 - Tested with NPEB90184 (never sets the depth_stencil clear values but uses stencil test)
		registers[NV4097_SET_ZSTENCIL_CLEAR_VALUE] = 0xffffff00;

		// CELL_GCM_SURFACE_A8R8G8B8, CELL_GCM_SURFACE_Z24S8 and CELL_GCM_SURFACE_CENTER_1
		registers[NV4097_SET_SURFACE_FORMAT] = (8 << 0) | (2 << 5) | (0 << 12) | (1 << 16) | (1 << 24);

		// rsx dma initial values
		registers[NV4097_SET_CONTEXT_DMA_REPORT] = CELL_GCM_CONTEXT_DMA_REPORT_LOCATION_LOCAL;
		registers[NV406E_SET_CONTEXT_DMA_SEMAPHORE] = CELL_GCM_CONTEXT_DMA_SEMAPHORE_RW;
		registers[NV3062_SET_CONTEXT_DMA_IMAGE_DESTIN] = CELL_GCM_CONTEXT_DMA_MEMORY_FRAME_BUFFER;
		registers[NV309E_SET_CONTEXT_DMA_IMAGE] = CELL_GCM_CONTEXT_DMA_MEMORY_FRAME_BUFFER;
		registers[NV0039_SET_CONTEXT_DMA_BUFFER_IN] = CELL_GCM_CONTEXT_DMA_MEMORY_FRAME_BUFFER;
		registers[NV0039_SET_CONTEXT_DMA_BUFFER_OUT] = CELL_GCM_CONTEXT_DMA_MEMORY_FRAME_BUFFER;
		registers[NV4097_SET_CONTEXT_DMA_COLOR_A] = CELL_GCM_CONTEXT_DMA_MEMORY_FRAME_BUFFER;
		registers[NV4097_SET_CONTEXT_DMA_COLOR_B] = CELL_GCM_CONTEXT_DMA_MEMORY_FRAME_BUFFER;
		registers[NV4097_SET_CONTEXT_DMA_COLOR_C] = CELL_GCM_CONTEXT_DMA_MEMORY_FRAME_BUFFER;
		registers[NV4097_SET_CONTEXT_DMA_COLOR_D] = CELL_GCM_CONTEXT_DMA_MEMORY_FRAME_BUFFER;
		registers[NV4097_SET_CONTEXT_DMA_ZETA] = CELL_GCM_CONTEXT_DMA_MEMORY_FRAME_BUFFER;

		registers[NV3089_SET_CONTEXT_SURFACE] = 0x313371C3; // CELL_GCM_CONTEXT_SURFACE2D

		for (auto& tex : fragment_textures) tex.init();
		for (auto& tex : vertex_textures) tex.init();
		for (auto& vtx : vertex_arrays_info) vtx.reset();
	}

	void rsx_state::decode(u32 reg, u32 value)
	{
		registers[reg] = value;
	}

	namespace method_detail
	{
		template<int Id, int Step, int Count, template<u32> class T, int Index = 0>
		struct bind_range_impl_t
		{
			static inline void impl()
			{
				methods[Id] = &T<Index>::impl;
				bind_range_impl_t<Id + Step, Step, Count, T, Index + 1>::impl();
			}
		};

		template<int Id, int Step, int Count, template<u32> class T>
		struct bind_range_impl_t<Id, Step, Count, T, Count>
		{
			static inline void impl()
			{
			}
		};

		template<int Id, int Step, int Count, template<u32> class T, int Index = 0>
		static inline void bind_range()
		{
			bind_range_impl_t<Id, Step, Count, T, Index>::impl();
		}

		template<int Id, rsx_method_t Func>
		static void bind()
		{
			methods[Id] = Func;
		}

		template<int Id, int Step, int Count, rsx_method_t Func>
		static void bind_array()
		{
			for (int i = Id; i < Id + Count * Step; i += Step)
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
		bind_array<(0xac00 >> 2), 1, 16, nullptr>();  // Unknown texture control register

		// NV406E
		bind<NV406E_SET_REFERENCE, nv406e::set_reference>();
		bind<NV406E_SEMAPHORE_ACQUIRE, nv406e::semaphore_acquire>();
		bind<NV406E_SEMAPHORE_RELEASE, nv406e::semaphore_release>();

		// NV4097
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
		bind_range<NV4097_SET_TRANSFORM_PROGRAM + 3, 4, 128, nv4097::set_transform_program>();
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
		bind<NV4097_SET_SURFACE_FORMAT, nv4097::set_surface_dirty_bit>();
		bind<NV4097_SET_SURFACE_PITCH_A, nv4097::set_surface_dirty_bit>();
		bind<NV4097_SET_SURFACE_PITCH_B, nv4097::set_surface_dirty_bit>();
		bind<NV4097_SET_SURFACE_PITCH_C, nv4097::set_surface_dirty_bit>();
		bind<NV4097_SET_SURFACE_PITCH_D, nv4097::set_surface_dirty_bit>();
		bind<NV4097_SET_SURFACE_PITCH_Z, nv4097::set_surface_dirty_bit>();
		bind_range<NV4097_SET_TEXTURE_OFFSET, 8, 16, nv4097::set_texture_dirty_bit>();
		bind_range<NV4097_SET_TEXTURE_FORMAT, 8, 16, nv4097::set_texture_dirty_bit>();
		bind_range<NV4097_SET_TEXTURE_ADDRESS, 8, 16, nv4097::set_texture_dirty_bit>();
		bind_range<NV4097_SET_TEXTURE_CONTROL0, 8, 16, nv4097::set_texture_dirty_bit>();
		bind_range<NV4097_SET_TEXTURE_CONTROL1, 8, 16, nv4097::set_texture_dirty_bit>();
		bind_range<NV4097_SET_TEXTURE_CONTROL2, 8, 16, nv4097::set_texture_dirty_bit>();
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
		bind<NV4097_SET_DEPTH_MASK, nv4097::set_surface_options_dirty_bit>();
		bind<NV4097_SET_COLOR_MASK, nv4097::set_surface_options_dirty_bit>();

		//NV308A
		bind_range<NV308A_COLOR, 1, 256, nv308a::color>();
		bind_range<NV308A_COLOR + 256, 1, 512, nv308a::color, 256>();

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
		

		return true;	
	}();
}
