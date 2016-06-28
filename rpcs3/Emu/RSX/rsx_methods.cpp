#include "stdafx.h"
#include "Utilities/Config.h"
#include "rsx_methods.h"
#include "RSXThread.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "rsx_utils.h"
#include "Emu/Cell/PPUCallback.h"

#include <thread>

cfg::map_entry<double> g_cfg_rsx_frame_limit(cfg::root.video, "Frame limit",
{
	{ "Off", 0. },
	{ "59.94", 59.94 },
	{ "50", 50. },
	{ "60", 60. },
	{ "30", 30. },
	{ "Auto", -1. },
});

namespace rsx
{
	u32 method_registers[0x10000 >> 2];
	rsx_method_t methods[0x10000 >> 2]{};

	template<typename Type> struct vertex_data_type_from_element_type;
	template<> struct vertex_data_type_from_element_type<float> { static const vertex_base_type type = vertex_base_type::f; };
	template<> struct vertex_data_type_from_element_type<f16> { static const vertex_base_type type = vertex_base_type::sf; };
	template<> struct vertex_data_type_from_element_type<u8> { static const vertex_base_type type = vertex_base_type::ub; };
	template<> struct vertex_data_type_from_element_type<u16> { static const vertex_base_type type = vertex_base_type::s1; };

	namespace nv406e
	{
		force_inline void set_reference(thread* rsx, u32 arg)
		{
			rsx->ctrl->ref.exchange(arg);
		}

		force_inline void semaphore_acquire(thread* rsx, u32 arg)
		{
			//TODO: dma
			while (vm::ps3::read32(rsx->label_addr + method_registers[NV406E_SEMAPHORE_OFFSET]) != arg)
			{
				if (Emu.IsStopped())
					break;

				std::this_thread::sleep_for(1ms);
			}
		}

		force_inline void semaphore_release(thread* rsx, u32 arg)
		{
			//TODO: dma
			vm::ps3::write32(rsx->label_addr + method_registers[NV406E_SEMAPHORE_OFFSET], arg);
		}
	}

	namespace nv4097
	{
		force_inline void texture_read_semaphore_release(thread* rsx, u32 arg)
		{
			//TODO: dma
			vm::ps3::write32(rsx->label_addr + method_registers[NV4097_SET_SEMAPHORE_OFFSET], arg);
		}

		force_inline void back_end_write_semaphore_release(thread* rsx, u32 arg)
		{
			//TODO: dma
			vm::ps3::write32(rsx->label_addr + method_registers[NV4097_SET_SEMAPHORE_OFFSET],
				(arg & 0xff00ff00) | ((arg & 0xff) << 16) | ((arg >> 16) & 0xff));
		}

		//fire only when all data passed to rsx cmd buffer
		template<u32 id, u32 index, int count, typename type>
		force_inline void set_vertex_data_impl(thread* rsx, u32 arg)
		{
			static const size_t element_size = (count * sizeof(type));
			static const size_t element_size_in_words = element_size / sizeof(u32);

			auto& info = rsx->register_vertex_info[index];

			info.type = vertex_data_type_from_element_type<type>::type;
			info.size = count;
			info.frequency = 0;
			info.stride = 0;

			auto& entry = rsx->register_vertex_data[index];

			//find begin of data
			size_t begin = id + index * element_size_in_words;

			size_t position = entry.size();
			entry.resize(position + element_size);

			memcpy(entry.data() + position, method_registers + begin, element_size);
		}

		template<u32 index>
		struct set_vertex_data4ub_m
		{
			force_inline static void impl(thread* rsx, u32 arg)
			{
				set_vertex_data_impl<NV4097_SET_VERTEX_DATA4UB_M, index, 4, u8>(rsx, arg);
			}
		};

		template<u32 index>
		struct set_vertex_data1f_m
		{
			force_inline static void impl(thread* rsx, u32 arg)
			{
				set_vertex_data_impl<NV4097_SET_VERTEX_DATA1F_M, index, 1, f32>(rsx, arg);
			}
		};

		template<u32 index>
		struct set_vertex_data2f_m
		{
			force_inline static void impl(thread* rsx, u32 arg)
			{
				set_vertex_data_impl<NV4097_SET_VERTEX_DATA2F_M, index, 2, f32>(rsx, arg);
			}
		};

		template<u32 index>
		struct set_vertex_data3f_m
		{
			force_inline static void impl(thread* rsx, u32 arg)
			{
				set_vertex_data_impl<NV4097_SET_VERTEX_DATA3F_M, index, 3, f32>(rsx, arg);
			}
		};

		template<u32 index>
		struct set_vertex_data4f_m
		{
			force_inline static void impl(thread* rsx, u32 arg)
			{
				set_vertex_data_impl<NV4097_SET_VERTEX_DATA4F_M, index, 4, f32>(rsx, arg);
			}
		};

		template<u32 index>
		struct set_vertex_data2s_m
		{
			force_inline static void impl(thread* rsx, u32 arg)
			{
				set_vertex_data_impl<NV4097_SET_VERTEX_DATA2S_M, index, 2, u16>(rsx, arg);
			}
		};

		template<u32 index>
		struct set_vertex_data4s_m
		{
			force_inline static void impl(thread* rsx, u32 arg)
			{
				set_vertex_data_impl<NV4097_SET_VERTEX_DATA4S_M, index, 4, u16>(rsx, arg);
			}
		};

		template<u32 index>
		struct set_vertex_data_array_format
		{
			force_inline static void impl(thread* rsx, u32 arg)
			{
				auto& info = rsx->vertex_arrays_info[index];
				info.unpack_array(arg);
			}
		};

		force_inline void draw_arrays(thread* rsx, u32 arg)
		{
			rsx->draw_command = rsx::draw_command::array;
			u32 first = arg & 0xffffff;
			u32 count = (arg >> 24) + 1;

			rsx->first_count_commands.emplace_back(std::make_pair(first, count));
		}

		force_inline void draw_index_array(thread* rsx, u32 arg)
		{
			rsx->draw_command = rsx::draw_command::indexed;
			u32 first = arg & 0xffffff;
			u32 count = (arg >> 24) + 1;

			rsx->first_count_commands.emplace_back(std::make_pair(first, count));
		}

		force_inline void draw_inline_array(thread* rsx, u32 arg)
		{
			rsx->draw_command = rsx::draw_command::inlined_array;
			rsx->draw_inline_vertex_array = true;
			rsx->inline_vertex_array.push_back(arg);
		}

		template<u32 index>
		struct set_transform_constant
		{
			force_inline static void impl(thread* rsxthr, u32 arg)
			{
				u32 load = method_registers[NV4097_SET_TRANSFORM_CONSTANT_LOAD];

				static const size_t count = 4;
				static const size_t size = count * sizeof(f32);

				size_t reg = index / 4;
				size_t subreg = index % 4;

				memcpy(rsxthr->transform_constants[load + reg].rgba + subreg, method_registers + NV4097_SET_TRANSFORM_CONSTANT + reg * count + subreg, sizeof(f32));
				rsxthr->m_transform_constants_dirty = true;
			}
		};

		template<u32 index>
		struct set_transform_program
		{
			force_inline static void impl(thread* rsx, u32 arg)
			{
				u32& load = method_registers[NV4097_SET_TRANSFORM_PROGRAM_LOAD];

				static const size_t count = 4;
				static const size_t size = count * sizeof(u32);

				memcpy(rsx->transform_program + load++ * count, method_registers + NV4097_SET_TRANSFORM_PROGRAM + index * count, size);
			}
		};

		force_inline void set_begin_end(thread* rsxthr, u32 arg)
		{
			if (arg)
			{
				rsxthr->begin();
				return;
			}

			u32 max_vertex_count = 0;

			for (u8 index = 0; index < rsx::limits::vertex_count; ++index)
			{
				auto &vertex_info = rsxthr->register_vertex_info[index];

				if (vertex_info.size > 0)
				{
					auto &vertex_data = rsxthr->register_vertex_data[index];

					u32 element_size = rsx::get_vertex_type_size_on_host(vertex_info.type, vertex_info.size);
					u32 element_count = vertex_data.size() / element_size;

					vertex_info.frequency = element_count;
					rsx::method_registers[NV4097_SET_FREQUENCY_DIVIDER_OPERATION] |= 1 << index;

					if (rsxthr->draw_command == rsx::draw_command::none)
					{
						max_vertex_count = std::max<u32>(max_vertex_count, element_count);
					}
				}
			}

			if (rsxthr->draw_command == rsx::draw_command::none && max_vertex_count)
			{
				rsxthr->draw_command = rsx::draw_command::array;
				rsxthr->first_count_commands.push_back(std::make_pair(0, max_vertex_count));
			}

			if (!rsxthr->first_count_commands.empty())
			{
				rsxthr->end();
			}
		}

		force_inline void get_report(thread* rsx, u32 arg)
		{
			u8 type = arg >> 24;
			u32 offset = arg & 0xffffff;
			u32 report_dma = method_registers[NV4097_SET_CONTEXT_DMA_REPORT];
			u32 location;

			switch (report_dma)
			{
			case CELL_GCM_CONTEXT_DMA_TO_MEMORY_GET_REPORT: location = CELL_GCM_LOCATION_LOCAL; break;
			case CELL_GCM_CONTEXT_DMA_REPORT_LOCATION_MAIN: location = CELL_GCM_LOCATION_MAIN; break;
			default:
				LOG_WARNING(RSX, "nv4097::get_report: bad report dma: 0x%x", report_dma);
				return;
			}

			vm::ps3::ptr<CellGcmReportData> result = vm::cast(get_address(offset, location));

			result->timer = rsx->timestamp();

			switch (type)
			{
			case CELL_GCM_ZPASS_PIXEL_CNT:
			case CELL_GCM_ZCULL_STATS:
			case CELL_GCM_ZCULL_STATS1:
			case CELL_GCM_ZCULL_STATS2:
			case CELL_GCM_ZCULL_STATS3:
				result->value = 0;
				LOG_WARNING(RSX, "NV4097_GET_REPORT: Unimplemented type %d", type);
				break;

			default:
				result->value = 0;
				LOG_ERROR(RSX, "NV4097_GET_REPORT: Bad type %d", type);
				break;
			}

			//result->padding = 0;
		}

		force_inline void clear_report_value(thread* rsx, u32 arg)
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
		}

		force_inline void set_surface_dirty_bit(thread* rsx, u32)
		{
			rsx->m_rtts_dirty = true;
		}

		template<u32 index>
		struct set_texture_dirty_bit
		{
			force_inline static void impl(thread* rsx, u32 arg)
			{
				rsx->m_textures_dirty[index] = true;
			}
		};
	}

	namespace nv308a
	{
		template<u32 index>
		struct color
		{
			force_inline static void impl(u32 arg)
			{
				u32 point = method_registers[NV308A_POINT];
				u16 x = point;
				u16 y = point >> 16;

				if (y)
				{
					LOG_ERROR(RSX, "%s: y is not null (0x%x)", __FUNCTION__, y);
				}

				u32 address = get_address(method_registers[NV3062_SET_OFFSET_DESTIN] + (x << 2) + index * 4, method_registers[NV3062_SET_CONTEXT_DMA_IMAGE_DESTIN]);
				vm::ps3::write32(address, arg);
			}
		};
	}

	namespace nv3089
	{
		never_inline void image_in(thread *rsx, u32 arg)
		{
			u32 operation = method_registers[NV3089_SET_OPERATION];

			u32 clip_x = method_registers[NV3089_CLIP_POINT] & 0xffff;
			u32 clip_y = method_registers[NV3089_CLIP_POINT] >> 16;
			u32 clip_w = method_registers[NV3089_CLIP_SIZE] & 0xffff;
			u32 clip_h = method_registers[NV3089_CLIP_SIZE] >> 16;

			u32 out_x = method_registers[NV3089_IMAGE_OUT_POINT] & 0xffff;
			u32 out_y = method_registers[NV3089_IMAGE_OUT_POINT] >> 16;
			u32 out_w = method_registers[NV3089_IMAGE_OUT_SIZE] & 0xffff;
			u32 out_h = method_registers[NV3089_IMAGE_OUT_SIZE] >> 16;

			u16 in_w = method_registers[NV3089_IMAGE_IN_SIZE];
			u16 in_h = method_registers[NV3089_IMAGE_IN_SIZE] >> 16;
			u16 in_pitch = method_registers[NV3089_IMAGE_IN_FORMAT];
			u8 in_origin = method_registers[NV3089_IMAGE_IN_FORMAT] >> 16;
			u8 in_inter = method_registers[NV3089_IMAGE_IN_FORMAT] >> 24;
			u32 src_color_format = method_registers[NV3089_SET_COLOR_FORMAT];

			f32 in_x = (method_registers[NV3089_IMAGE_IN] & 0xffff) / 16.f;
			f32 in_y = (method_registers[NV3089_IMAGE_IN] >> 16) / 16.f;

			if (in_origin != CELL_GCM_TRANSFER_ORIGIN_CORNER)
			{
				LOG_ERROR(RSX, "NV3089_IMAGE_IN_SIZE: unknown origin (%d)", in_origin);
			}

			if (in_inter != CELL_GCM_TRANSFER_INTERPOLATOR_ZOH && in_inter != CELL_GCM_TRANSFER_INTERPOLATOR_FOH)
			{
				LOG_ERROR(RSX, "NV3089_IMAGE_IN_SIZE: unknown inter (%d)", in_inter);
			}

			if (operation != CELL_GCM_TRANSFER_OPERATION_SRCCOPY)
			{
				LOG_ERROR(RSX, "NV3089_IMAGE_IN_SIZE: unknown operation (%d)", operation);
			}

			const u32 src_offset = method_registers[NV3089_IMAGE_IN_OFFSET];
			const u32 src_dma = method_registers[NV3089_SET_CONTEXT_DMA_IMAGE];

			u32 dst_offset;
			u32 dst_dma = 0;
			u16 dst_color_format;
			u32 out_pitch = 0;
			u32 out_aligment = 64;

			switch (method_registers[NV3089_SET_CONTEXT_SURFACE])
			{
			case CELL_GCM_CONTEXT_SURFACE2D:
				dst_dma = method_registers[NV3062_SET_CONTEXT_DMA_IMAGE_DESTIN];
				dst_offset = method_registers[NV3062_SET_OFFSET_DESTIN];
				dst_color_format = method_registers[NV3062_SET_COLOR_FORMAT];
				out_pitch = method_registers[NV3062_SET_PITCH] >> 16;
				out_aligment = method_registers[NV3062_SET_PITCH] & 0xffff;
				break;

			case CELL_GCM_CONTEXT_SWIZZLE2D:
				dst_dma = method_registers[NV309E_SET_CONTEXT_DMA_IMAGE];
				dst_offset = method_registers[NV309E_SET_OFFSET];
				dst_color_format = method_registers[NV309E_SET_FORMAT];
				break;

			default:
				LOG_ERROR(RSX, "NV3089_IMAGE_IN_SIZE: unknown m_context_surface (0x%x)", method_registers[NV3089_SET_CONTEXT_SURFACE]);
				return;
			}

			if (dst_dma == CELL_GCM_CONTEXT_DMA_MEMORY_FRAME_BUFFER)
			{
				//HACK: it's extension of the flip-hack. remove this when textures cache would be properly implemented
				for (int i = 0; i < rsx::limits::color_buffers_count; ++i)
				{
					u32 begin = rsx->gcm_buffers[i].offset;

					if (dst_offset < begin || !begin)
					{
						continue;
					}

					if (rsx->gcm_buffers[i].width < 720 || rsx->gcm_buffers[i].height < 480)
					{
						continue;
					}

					if (begin == dst_offset)
					{
						return;
					}

					u32 end = begin + rsx->gcm_buffers[i].height * rsx->gcm_buffers[i].pitch;

					if (dst_offset < end)
					{
						return;
					}
				}
			}

			u32 in_bpp = src_color_format == CELL_GCM_TRANSFER_SCALE_FORMAT_R5G6B5 ? 2 : 4; // bytes per pixel
			u32 out_bpp = dst_color_format == CELL_GCM_TRANSFER_SURFACE_FORMAT_R5G6B5 ? 2 : 4;

			u32 in_offset = u32(in_x * in_bpp + in_pitch * in_y);
			u32 out_offset = out_x * out_bpp + out_pitch * out_y;

			tiled_region src_region = rsx->get_tiled_address(src_offset + in_offset, src_dma & 0xf);//get_address(src_offset, src_dma);
			u32 dst_address = get_address(dst_offset + out_offset, dst_dma);

			if (out_pitch == 0)
			{
				out_pitch = out_bpp * out_w;
			}

			if (in_pitch == 0)
			{
				in_pitch = in_bpp * in_w;
			}

			if (clip_w > out_w)
			{
				clip_w = out_w;
			}

			if (clip_h > out_h)
			{
				clip_h = out_h;
			}

			//LOG_ERROR(RSX, "NV3089_IMAGE_IN_SIZE: src = 0x%x, dst = 0x%x", src_address, dst_address);

			u8* pixels_src = src_region.tile ? src_region.ptr + src_region.base : src_region.ptr;
			u8* pixels_dst = vm::ps3::_ptr<u8>(dst_address + out_offset);

			if (dst_color_format != CELL_GCM_TRANSFER_SURFACE_FORMAT_R5G6B5 &&
				dst_color_format != CELL_GCM_TRANSFER_SURFACE_FORMAT_A8R8G8B8)
			{
				LOG_ERROR(RSX, "NV3089_IMAGE_IN_SIZE: unknown dst_color_format (%d)", dst_color_format);
			}

			if (src_color_format != CELL_GCM_TRANSFER_SCALE_FORMAT_R5G6B5 &&
				src_color_format != CELL_GCM_TRANSFER_SCALE_FORMAT_A8R8G8B8)
			{
				LOG_ERROR(RSX, "NV3089_IMAGE_IN_SIZE: unknown src_color_format (%d)", src_color_format);
			}

			//LOG_WARNING(RSX, "NV3089_IMAGE_IN_SIZE: SIZE=0x%08x, pitch=0x%x, offset=0x%x, scaleX=%f, scaleY=%f, CLIP_SIZE=0x%08x, OUT_SIZE=0x%08x",
			//	method_registers[NV3089_IMAGE_IN_SIZE], in_pitch, src_offset, double(1 << 20) / (method_registers[NV3089_DS_DX]), double(1 << 20) / (method_registers[NV3089_DT_DY]),
			//	method_registers[NV3089_CLIP_SIZE], method_registers[NV3089_IMAGE_OUT_SIZE]);

			std::unique_ptr<u8[]> temp1, temp2, sw_temp;

			AVPixelFormat in_format = src_color_format == CELL_GCM_TRANSFER_SCALE_FORMAT_R5G6B5 ? AV_PIX_FMT_RGB565BE : AV_PIX_FMT_ARGB;
			AVPixelFormat out_format = dst_color_format == CELL_GCM_TRANSFER_SURFACE_FORMAT_R5G6B5 ? AV_PIX_FMT_RGB565BE : AV_PIX_FMT_ARGB;

			f32 scale_x = 1048576.f / method_registers[NV3089_DS_DX];
			f32 scale_y = 1048576.f / method_registers[NV3089_DT_DY];

			u32 convert_w = (u32)(scale_x * in_w);
			u32 convert_h = (u32)(scale_y * in_h);

			bool need_clip =
				method_registers[NV3089_CLIP_SIZE] != method_registers[NV3089_IMAGE_IN_SIZE] ||
				method_registers[NV3089_CLIP_POINT] || convert_w != out_w || convert_h != out_h;

			bool need_convert = out_format != in_format || scale_x != 1.0 || scale_y != 1.0;

			u32 slice_h = clip_h;

			if (src_region.tile)
			{
				if (src_region.tile->comp == CELL_GCM_COMPMODE_C32_2X2)
				{
					slice_h *= 2;
				}

				u32 size = slice_h * in_pitch;

				if (size > src_region.tile->size - src_region.base)
				{
					u32 diff = size - (src_region.tile->size - src_region.base);
					slice_h -= diff / in_pitch + (diff % in_pitch ? 1 : 0);
				}
			}

			if (method_registers[NV3089_SET_CONTEXT_SURFACE] != CELL_GCM_CONTEXT_SWIZZLE2D)
			{
				if (need_convert || need_clip)
				{
					if (need_clip)
					{
						if (need_convert)
						{
							convert_scale_image(temp1, out_format, convert_w, convert_h, out_pitch,
								pixels_src, in_format, in_w, in_h, in_pitch, slice_h, in_inter ? true : false);

							clip_image(pixels_dst + out_offset, temp1.get(), clip_x, clip_y, clip_w, clip_h, out_bpp, out_pitch, out_pitch);
						}
						else
						{
							clip_image(pixels_dst + out_offset, pixels_src, clip_x, clip_y, clip_w, clip_h, out_bpp, in_pitch, out_pitch);
						}
					}
					else
					{
						convert_scale_image(pixels_dst + out_offset, out_format, out_w, out_h, out_pitch,
							pixels_src, in_format, in_w, in_h, in_pitch, slice_h, in_inter ? true : false);
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
						std::memmove(pixels_dst + out_offset, pixels_src, out_pitch * out_h);
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
								pixels_src, in_format, in_w, in_h, in_pitch, slice_h, in_inter ? true : false);

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
							pixels_src, in_format, in_w, in_h, in_pitch, clip_h, in_inter ? true : false);
					}

					pixels_src = temp2.get();
				}

				u8 sw_width_log2 = method_registers[NV309E_SET_FORMAT] >> 16;
				u8 sw_height_log2 = method_registers[NV309E_SET_FORMAT] >> 24;

				// 0 indicates height of 1 pixel
				sw_height_log2 = sw_height_log2 == 0 ? 1 : sw_height_log2;

				// swizzle based on destination size
				u16 sw_width = 1 << sw_width_log2;
				u16 sw_height = 1 << sw_height_log2;

				temp2.reset(new u8[out_bpp * sw_width * sw_height]);

				u8* linear_pixels = pixels_src;
				u8* swizzled_pixels = temp2.get();

				// Check and pad texture out if we are given non square texture for swizzle to be correct
				if (sw_width != out_w || sw_height != out_h)
				{
					sw_temp.reset(new u8[out_bpp * sw_width * sw_height]);

					switch (out_bpp)
					{
					case 1:
						pad_texture<u8>(linear_pixels, sw_temp.get(), out_w, out_h, sw_width, sw_height);
						break;
					case 2:
						pad_texture<u16>(linear_pixels, sw_temp.get(), out_w, out_h, sw_width, sw_height);
						break;
					case 4:
						pad_texture<u32>(linear_pixels, sw_temp.get(), out_w, out_h, sw_width, sw_height);
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
		force_inline void buffer_notify(u32 arg)
		{
			u32 in_pitch = method_registers[NV0039_PITCH_IN];
			u32 out_pitch = method_registers[NV0039_PITCH_OUT];
			const u32 line_length = method_registers[NV0039_LINE_LENGTH_IN];
			const u32 line_count = method_registers[NV0039_LINE_COUNT];
			const u8 out_format = method_registers[NV0039_FORMAT] >> 8;
			const u8 in_format = method_registers[NV0039_FORMAT];
			const u32 notify = arg;

			// The existing GCM commands use only the value 0x1 for inFormat and outFormat
			if (in_format != 0x01 || out_format != 0x01)
			{
				LOG_ERROR(RSX, "NV0039_OFFSET_IN: Unsupported format: inFormat=%d, outFormat=%d", in_format, out_format);
			}

			if (!in_pitch)
			{
				in_pitch = line_length;
			}

			if (!out_pitch)
			{
				out_pitch = line_length;
			}

			u32 src_offset = method_registers[NV0039_OFFSET_IN];
			u32 src_dma = method_registers[NV0039_SET_CONTEXT_DMA_BUFFER_IN];

			u32 dst_offset = method_registers[NV0039_OFFSET_OUT];
			u32 dst_dma = method_registers[NV0039_SET_CONTEXT_DMA_BUFFER_OUT];

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

	void flip_command(thread* rsx, u32 arg)
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
			Emu.Pause();
		}

		rsx->gcm_current_buffer = arg;
		rsx->flip(arg);
		// After each flip PS3 system is executing a routine that changes registers value to some default.
		// Some game use this default state (SH3).
		rsx->reset();

		rsx->last_flip_time = get_system_time() - 1000000;
		rsx->gcm_current_buffer = arg;
		rsx->flip_status = 0;

		if (rsx->flip_handler)
		{
			Emu.GetCallbackManager().Async([func = rsx->flip_handler](PPUThread& ppu)
			{
				func(ppu, 1);
			});
		}

		if (double limit = g_cfg_rsx_frame_limit.get())
		{
			if (limit < 0) limit = rsx->fps_limit; // TODO

			std::this_thread::sleep_for(std::chrono::milliseconds((s64)(1000.0 / limit - rsx->timer_sync.GetElapsedTimeInMilliSec())));
			rsx->timer_sync.Start();
			rsx->local_transform_constants.clear();
		}
	}

	void user_command(thread* rsx, u32 arg)
	{
		if (rsx->user_handler)
		{
			Emu.GetCallbackManager().Async([func = rsx->user_handler, arg](PPUThread& ppu)
			{
				func(ppu, arg);
			});
		}
		else
		{
			throw EXCEPTION("User handler not set");
		}
	}

	struct __rsx_methods_t
	{
		using rsx_impl_method_t = void(*)(u32);

		template<rsx_method_t impl_func>
		force_inline static void call_impl_func(thread *rsx, u32 arg)
		{
			impl_func(rsx, arg);
		}

		template<rsx_impl_method_t impl_func>
		force_inline static void call_impl_func(thread *rsx, u32 arg)
		{
			impl_func(arg);
		}

		template<int id, typename T, T impl_func>
		static void wrapper(thread *rsx, u32 arg)
		{
			// try process using gpu
			if (rsx->do_method(id, arg))
			{
				if (rsx->capture_current_frame && id == NV4097_CLEAR_SURFACE)
					rsx->capture_frame("clear");
				return;
			}

			// not handled by renderer
			// try process using cpu
			if (impl_func != nullptr)
				call_impl_func<impl_func>(rsx, arg);
		}

		template<int id, int step, int count, template<u32> class T, int index = 0>
		struct bind_range_impl_t
		{
			force_inline static void impl()
			{
				bind_range_impl_t<id + step, step, count, T, index + 1>::impl();
				bind<id, T<index>::impl>();
			}
		};

		template<int id, int step, int count, template<u32> class T>
		struct bind_range_impl_t<id, step, count, T, count>
		{
			force_inline static void impl()
			{
			}
		};

		template<int id, int step, int count, template<u32> class T, int index = 0>
		force_inline static void bind_range()
		{
			bind_range_impl_t<id, step, count, T, index>::impl();
		}

		[[noreturn]] never_inline static void bind_redefinition_error(int id)
		{
			throw EXCEPTION("RSX method implementation redefinition (0x%04x)", id);
		}

		template<int id, typename T, T impl_func>
		static void bind_impl()
		{
			if (methods[id])
			{
				bind_redefinition_error(id);
			}

			methods[id] = wrapper<id, T, impl_func>;
		}

		template<int id, typename T, T impl_func>
		static void bind_cpu_only_impl()
		{
			if (methods[id])
			{
				bind_redefinition_error(id);
			}

			methods[id] = call_impl_func<impl_func>;
		}

		template<int id, rsx_impl_method_t impl_func>       static void bind() { bind_impl<id, rsx_impl_method_t, impl_func>(); }
		template<int id, rsx_method_t impl_func = nullptr>  static void bind() { bind_impl<id, rsx_method_t, impl_func>(); }

		//do not try process on gpu
		template<int id, rsx_impl_method_t impl_func>      static void bind_cpu_only() { bind_cpu_only_impl<id, rsx_impl_method_t, impl_func>(); }
		//do not try process on gpu
		template<int id, rsx_method_t impl_func = nullptr> static void bind_cpu_only() { bind_cpu_only_impl<id, rsx_method_t, impl_func>(); }

		__rsx_methods_t()
		{
			// NV406E
			bind_cpu_only<NV406E_SET_REFERENCE, nv406e::set_reference>();
			bind<NV406E_SEMAPHORE_ACQUIRE, nv406e::semaphore_acquire>();
			bind<NV406E_SEMAPHORE_RELEASE, nv406e::semaphore_release>();

			/*

			// Store previous fbo addresses to detect RTT config changes.
			std::array<u32, 4> m_previous_color_address = {};
			u32 m_previous_address_z = 0;
			u32 m_previous_target = 0;
			u32 m_previous_clip_horizontal = 0;
			u32 m_previous_clip_vertical = 0;
			*/

			// NV4097
			bind<NV4097_TEXTURE_READ_SEMAPHORE_RELEASE, nv4097::texture_read_semaphore_release>();
			bind<NV4097_BACK_END_WRITE_SEMAPHORE_RELEASE, nv4097::back_end_write_semaphore_release>();
			bind<NV4097_SET_BEGIN_END, nv4097::set_begin_end>();
			bind<NV4097_CLEAR_SURFACE>();
			bind<NV4097_DRAW_ARRAYS, nv4097::draw_arrays>();
			bind<NV4097_DRAW_INDEX_ARRAY, nv4097::draw_index_array>();
			bind<NV4097_INLINE_ARRAY, nv4097::draw_inline_array>();
			bind_range<NV4097_SET_VERTEX_DATA_ARRAY_FORMAT, 1, 16, nv4097::set_vertex_data_array_format>();
			bind_range<NV4097_SET_VERTEX_DATA4UB_M, 1, 16, nv4097::set_vertex_data4ub_m>();
			bind_range<NV4097_SET_VERTEX_DATA1F_M, 1, 16, nv4097::set_vertex_data1f_m>();
			bind_range<NV4097_SET_VERTEX_DATA2F_M + 1, 2, 16, nv4097::set_vertex_data2f_m>();
			bind_range<NV4097_SET_VERTEX_DATA3F_M + 2, 3, 16, nv4097::set_vertex_data3f_m>();
			bind_range<NV4097_SET_VERTEX_DATA4F_M + 3, 4, 16, nv4097::set_vertex_data4f_m>();
			bind_range<NV4097_SET_VERTEX_DATA2S_M, 1, 16, nv4097::set_vertex_data2s_m>();
			bind_range<NV4097_SET_VERTEX_DATA4S_M + 1, 2, 16, nv4097::set_vertex_data4s_m>();
			bind_range<NV4097_SET_TRANSFORM_CONSTANT, 1, 32, nv4097::set_transform_constant>();
			bind_range<NV4097_SET_TRANSFORM_PROGRAM + 3, 4, 128, nv4097::set_transform_program>();
			bind_cpu_only<NV4097_GET_REPORT, nv4097::get_report>();
			bind_cpu_only<NV4097_CLEAR_REPORT_VALUE, nv4097::clear_report_value>();
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

			//NV308A
			bind_range<NV308A_COLOR, 1, 256, nv308a::color>();
			bind_range<NV308A_COLOR + 256, 1, 512, nv308a::color, 256>();

			//NV3089
			bind<NV3089_IMAGE_IN, nv3089::image_in>();

			//NV0039
			bind<NV0039_BUFFER_NOTIFY, nv0039::buffer_notify>();

			// custom methods
			bind_cpu_only<GCM_FLIP_COMMAND, flip_command>();
			bind_cpu_only<GCM_SET_USER_COMMAND, user_command>();
		}
	} __rsx_methods;
}
