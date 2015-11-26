#include "stdafx.h"
#include "Utilities/Log.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/state.h"
#include "Emu/RSX/GSManager.h"
#include "RSXThread.h"

#include "Emu/SysCalls/Callback.h"
#include "Emu/SysCalls/CB_FUNC.h"
#include "Emu/SysCalls/lv2/sys_time.h"

#include "Common/BufferUtils.h"

#include "Utilities/types.h"

extern "C"
{
#include "libswscale/swscale.h"
}

#define CMD_DEBUG 0

bool user_asked_for_frame_capture = false;
frame_capture_data frame_debug;

namespace rsx
{
	using rsx_method_t = void(*)(thread*, u32);

	u32 method_registers[0x10000 >> 2];
	rsx_method_t methods[0x10000 >> 2]{};

	template<typename Type> struct vertex_data_type_from_element_type;
	template<> struct vertex_data_type_from_element_type<float> { enum { type = CELL_GCM_VERTEX_F }; };
	template<> struct vertex_data_type_from_element_type<f16> { enum { type = CELL_GCM_VERTEX_SF }; };
	template<> struct vertex_data_type_from_element_type<u8> { enum { type = CELL_GCM_VERTEX_UB }; };
	template<> struct vertex_data_type_from_element_type<u16> { enum { type = CELL_GCM_VERTEX_S1 }; };

	namespace nv406e
	{
		force_inline void set_reference(thread* rsx, u32 arg)
		{
			rsx->ctrl->ref.exchange(arg);
		}

		force_inline void semaphore_acquire(thread* rsx, u32 arg)
		{
			//TODO: dma
			while (vm::read32(rsx->label_addr + method_registers[NV406E_SEMAPHORE_OFFSET]) != arg)
			{
				if (Emu.IsStopped())
					break;

				std::this_thread::sleep_for(std::chrono::milliseconds(1));
			}
		}

		force_inline void semaphore_release(thread* rsx, u32 arg)
		{
			//TODO: dma
			vm::write32(rsx->label_addr + method_registers[NV406E_SEMAPHORE_OFFSET], arg);
		}
	}

	namespace nv4097
	{
		force_inline void texture_read_semaphore_release(thread* rsx, u32 arg)
		{
			//TODO: dma
			vm::write32(rsx->label_addr + method_registers[NV4097_SET_SEMAPHORE_OFFSET], arg);
		}

		force_inline void back_end_write_semaphore_release(thread* rsx, u32 arg)
		{
			//TODO: dma
			vm::write32(rsx->label_addr + method_registers[NV4097_SET_SEMAPHORE_OFFSET],
				(arg & 0xff00ff00) | ((arg & 0xff) << 16) | ((arg >> 16) & 0xff));
		}

		//fire only when all data passed to rsx cmd buffer
		template<u32 id, u32 index, int count, typename type>
		force_inline void set_vertex_data_impl(thread* rsx, u32 arg)
		{
			static const size_t element_size = (count * sizeof(type));
			static const size_t element_size_in_words = element_size / sizeof(u32);

			auto& info = rsx->vertex_arrays_info[index];

			info.type = vertex_data_type_from_element_type<type>::type;
			info.size = count;
			info.frequency = 0;
			info.stride = 0;
			info.array = false;

			auto& entry = rsx->vertex_arrays[index];

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
				info.unpack(arg);
				info.array = info.size > 0;
			}
		};

		force_inline void draw_arrays(thread* rsx, u32 arg)
		{
			u32 first = arg & 0xffffff;
			u32 count = (arg >> 24) + 1;

			rsx->load_vertex_data(first, count);
		}

		force_inline void draw_index_array(thread* rsx, u32 arg)
		{
			u32 first = arg & 0xffffff;
			u32 count = (arg >> 24) + 1;

			rsx->load_vertex_data(first, count);
			rsx->load_vertex_index_data(first, count);
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

		force_inline void set_begin_end(thread* rsx, u32 arg)
		{
			if (arg)
			{
				rsx->begin();
				return;
			}

			if (!rsx->vertex_draw_count)
			{
				bool has_array = false;

				for (int i = 0; i < rsx::limits::vertex_count; ++i)
				{
					if (rsx->vertex_arrays_info[i].array)
					{
						has_array = true;
						break;
					}
				}

				if (!has_array)
				{
					u32 min_count = ~0;

					for (int i = 0; i < rsx::limits::vertex_count; ++i)
					{
						if (!rsx->vertex_arrays_info[i].size)
							continue;

						u32 count = u32(rsx->vertex_arrays[i].size()) /
							rsx::get_vertex_type_size(rsx->vertex_arrays_info[i].type) * rsx->vertex_arrays_info[i].size;

						if (count < min_count)
							min_count = count;
					}

					if (min_count && min_count < ~0)
					{
						rsx->vertex_draw_count = min_count;
					}
				}
			}

			rsx->end();
			rsx->vertex_draw_count = 0;
		}

		force_inline void get_report(thread* rsx, u32 arg)
		{
			u8 type = arg >> 24;
			u32 offset = arg & 0xffffff;

			//TODO: use DMA
			vm::ptr<CellGcmReportData> result = { rsx->local_mem_addr + offset, vm::addr };

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
				vm::write32(address, arg);
			}
		};
	}

	namespace nv3089
	{
		never_inline void image_in(u32 arg)
		{
			const u16 src_height = method_registers[NV3089_IMAGE_IN_SIZE] >> 16;
			const u16 src_pitch = method_registers[NV3089_IMAGE_IN_FORMAT];
			const u8 src_origin = method_registers[NV3089_IMAGE_IN_FORMAT] >> 16;
			const u8 src_inter = method_registers[NV3089_IMAGE_IN_FORMAT] >> 24;
			const u32 src_color_format = method_registers[NV3089_SET_COLOR_FORMAT];
			const u32 operation = method_registers[NV3089_SET_OPERATION];

			const u16 out_w = method_registers[NV3089_IMAGE_OUT_SIZE];
			const u16 out_h = method_registers[NV3089_IMAGE_OUT_SIZE] >> 16;

			// handle weird RSX quirk, doesn't report less than 16 pixels width in some cases 
			u16 src_width = method_registers[NV3089_IMAGE_IN_SIZE];
			if (src_width == 16 && out_w < 16 && method_registers[NV3089_DS_DX] == (1 << 20)) 
			{
				src_width = out_w;
			}

			const u16 u = method_registers[NV3089_IMAGE_IN]; // inX (currently ignored)
			const u16 v = method_registers[NV3089_IMAGE_IN] >> 16; // inY (currently ignored)

			if (src_origin != CELL_GCM_TRANSFER_ORIGIN_CORNER)
			{
				LOG_ERROR(RSX, "NV3089_IMAGE_IN_SIZE: unknown origin (%d)", src_origin);
			}

			if (src_inter != CELL_GCM_TRANSFER_INTERPOLATOR_ZOH && src_inter != CELL_GCM_TRANSFER_INTERPOLATOR_FOH)
			{
				LOG_ERROR(RSX, "NV3089_IMAGE_IN_SIZE: unknown inter (%d)", src_inter);
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

			switch (method_registers[NV3089_SET_CONTEXT_SURFACE])
			{
			case CELL_GCM_CONTEXT_SURFACE2D:
				dst_dma = method_registers[NV3062_SET_CONTEXT_DMA_IMAGE_DESTIN];
				dst_offset = method_registers[NV3062_SET_OFFSET_DESTIN];
				dst_color_format = method_registers[NV3062_SET_COLOR_FORMAT];
				break;

			case CELL_GCM_CONTEXT_SWIZZLE2D:
				dst_dma = method_registers[NV309E_SET_CONTEXT_DMA_IMAGE];
				dst_offset = method_registers[NV309E_SET_OFFSET];
				dst_color_format = method_registers[NV309E_SET_FORMAT];
				break;

			default:
				LOG_ERROR(RSX, "NV3089_IMAGE_IN_SIZE: unknown m_context_surface (0x%x)", method_registers[NV3089_SET_CONTEXT_SURFACE]);
				break;
			}

			if (!dst_dma)
			{
				LOG_ERROR(RSX, "dst_dma not set");
				return;
			}

			LOG_ERROR(RSX, "NV3089_IMAGE_IN_SIZE: src = 0x%x, dst = 0x%x", src_offset, dst_offset);

			u8* pixels_src = vm::_ptr<u8>(get_address(src_offset, src_dma));
			u8* pixels_dst = vm::_ptr<u8>(get_address(dst_offset, dst_dma));

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

			LOG_WARNING(RSX, "NV3089_IMAGE_IN_SIZE: SIZE=0x%08x, pitch=0x%x, offset=0x%x, scaleX=%f, scaleY=%f, CLIP_SIZE=0x%08x, OUT_SIZE=0x%08x",
				method_registers[NV3089_IMAGE_IN_SIZE], src_pitch, src_offset, double(1 << 20) / (method_registers[NV3089_DS_DX]), double(1 << 20) / (method_registers[NV3089_DT_DY]),
				method_registers[NV3089_CLIP_SIZE], method_registers[NV3089_IMAGE_OUT_SIZE]);

			const u32 in_bpp = src_color_format == CELL_GCM_TRANSFER_SCALE_FORMAT_R5G6B5 ? 2 : 4; // bytes per pixel
			const u32 out_bpp = dst_color_format == CELL_GCM_TRANSFER_SURFACE_FORMAT_R5G6B5 ? 2 : 4;

			std::unique_ptr<u8[]> temp1, temp2;

			// resize/convert if necessary
			if (in_bpp != out_bpp && src_width != out_w && src_height != out_h)
			{
				temp1.reset(new u8[out_bpp * out_w * out_h]);

				AVPixelFormat in_format = src_color_format == CELL_GCM_TRANSFER_SCALE_FORMAT_R5G6B5 ? AV_PIX_FMT_RGB565BE : AV_PIX_FMT_ARGB;
				AVPixelFormat out_format = dst_color_format == CELL_GCM_TRANSFER_SURFACE_FORMAT_R5G6B5 ? AV_PIX_FMT_RGB565BE : AV_PIX_FMT_ARGB;

				std::unique_ptr<SwsContext, void(*)(SwsContext*)> sws(sws_getContext(src_width, src_height, in_format, out_w, out_h, out_format,
					src_inter ? SWS_FAST_BILINEAR : SWS_POINT, NULL, NULL, NULL), sws_freeContext);

				int in_line = in_bpp * src_width;
				u8* out_ptr = temp1.get();
				int out_line = out_bpp * out_w;

				sws_scale(sws.get(), &pixels_src, &in_line, 0, src_height, &out_ptr, &out_line);

				pixels_src = out_ptr; // use resized image as a source
			}

			// clip if necessary
			if (method_registers[NV3089_CLIP_SIZE] != method_registers[NV3089_IMAGE_OUT_SIZE] ||
				method_registers[NV3089_CLIP_POINT] != method_registers[NV3089_IMAGE_OUT_POINT])
			{
				temp2.reset(new u8[out_bpp * out_w * out_h]);
				for (s32 y = (method_registers[NV3089_CLIP_POINT] >> 16), dst_y = (method_registers[NV3089_IMAGE_OUT_POINT] >> 16); y < out_h; y++, dst_y++)
				{
					if (dst_y >= 0 && dst_y < method_registers[NV3089_IMAGE_OUT_SIZE] >> 16)
					{
						// destination line
						u8* dst_line = temp2.get() + dst_y * out_bpp * (method_registers[NV3089_IMAGE_OUT_SIZE] & 0xffff)
							+ std::min<s32>(std::max<s32>(method_registers[NV3089_IMAGE_OUT_POINT] & 0xffff, 0), method_registers[NV3089_IMAGE_OUT_SIZE] & 0xffff);

						size_t dst_max = std::min<s32>(
							std::max<s32>((s32)(method_registers[NV3089_IMAGE_OUT_SIZE] & 0xffff) - (method_registers[NV3089_IMAGE_OUT_POINT] & 0xffff), 0),
							method_registers[NV3089_IMAGE_OUT_SIZE] & 0xffff) * out_bpp;

						if (y >= 0 && y < std::min<s32>(method_registers[NV3089_CLIP_SIZE] >> 16, out_h))
						{
							// source line
							u8* src_line = pixels_src + y * out_bpp * out_w +
								std::min<s32>(std::max<s32>(method_registers[NV3089_CLIP_POINT] & 0xffff, 0), method_registers[NV3089_CLIP_SIZE] & 0xffff);
							size_t src_max = std::min<s32>(
								std::max<s32>((s32)(method_registers[NV3089_CLIP_SIZE] & 0xffff) - (method_registers[NV3089_CLIP_POINT] & 0xffff), 0),
								method_registers[NV3089_CLIP_SIZE] & 0xffff) * out_bpp;

							std::pair<u8*, size_t>
								z0 = { src_line + 0, std::min<size_t>(dst_max, std::max<s64>(0, method_registers[NV3089_CLIP_POINT] & 0xffff)) },
								d0 = { src_line + z0.second, std::min<size_t>(dst_max - z0.second, src_max) },
								z1 = { src_line + d0.second, dst_max - z0.second - d0.second };

							std::memset(z0.first, 0, z0.second);
							std::memcpy(d0.first, src_line, d0.second);
							std::memset(z1.first, 0, z1.second);
						}
						else
						{
							std::memset(dst_line, 0, dst_max);
						}
					}
				}
				pixels_src = temp2.get();
			}

			// Swizzle texture last after scaling is done
			if (method_registers[NV3089_SET_CONTEXT_SURFACE] == CELL_GCM_CONTEXT_SWIZZLE2D)
			{
				u8 sw_width_log2 = method_registers[NV309E_SET_FORMAT] >> 16;
				u8 sw_height_log2 = method_registers[NV309E_SET_FORMAT] >> 24;

				// 0 indicates height of 1 pixel
				sw_height_log2 = sw_height_log2 == 0 ? 1 : sw_height_log2;

				// swizzle based on destination size
				u16 sw_width = 1 << sw_width_log2;
				u16 sw_height =  1 << sw_height_log2;

				std::unique_ptr<u8[]> sw_temp, sw_temp2;

				sw_temp.reset(new u8[out_bpp * sw_width * sw_height]);

				u8* linear_pixels = pixels_src;
				u8* swizzled_pixels = sw_temp.get();

				// Check and pad texture out if we are given non square texture for swizzle to be correct
				if (sw_width != out_w || sw_height != out_h) 
				{
					sw_temp2.reset(new u8[out_bpp * sw_width * sw_height]());

					switch (out_bpp) 
					{
					case 1:
						pad_texture<u8>(linear_pixels, sw_temp2.get(), out_w, out_h, sw_width, sw_height);
						break;
					case 2:
						pad_texture<u16>(linear_pixels, sw_temp2.get(), out_w, out_h, sw_width, sw_height);
						break;
					case 4:
						pad_texture<u32>(linear_pixels, sw_temp2.get(), out_w, out_h, sw_width, sw_height);
						break;
					}
					
					linear_pixels = sw_temp2.get();
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
			else 
			{
				std::memcpy(pixels_dst, pixels_src, out_w * out_h * out_bpp);
			}
		}
	}

	namespace nv0039
	{
		force_inline void buffer_notify(u32 arg)
		{
			const u32 inPitch = method_registers[NV0039_PITCH_IN];
			const u32 outPitch = method_registers[NV0039_PITCH_OUT];
			const u32 lineLength = method_registers[NV0039_LINE_LENGTH_IN];
			const u32 lineCount = method_registers[NV0039_LINE_COUNT];
			const u8 outFormat = method_registers[NV0039_FORMAT] >> 8;
			const u8 inFormat = method_registers[NV0039_FORMAT];
			const u32 notify = arg;

			// The existing GCM commands use only the value 0x1 for inFormat and outFormat
			if (inFormat != 0x01 || outFormat != 0x01)
			{
				LOG_ERROR(RSX, "NV0039_OFFSET_IN: Unsupported format: inFormat=%d, outFormat=%d", inFormat, outFormat);
			}

			if (lineCount == 1 && !inPitch && !outPitch && !notify)
			{
				std::memcpy(
					vm::base(get_address(method_registers[NV0039_OFFSET_OUT], method_registers[NV0039_SET_CONTEXT_DMA_BUFFER_OUT])),
					vm::base(get_address(method_registers[NV0039_OFFSET_IN], method_registers[NV0039_SET_CONTEXT_DMA_BUFFER_IN])),
					lineLength);
			}
			else
			{
				LOG_ERROR(RSX, "NV0039_OFFSET_IN: bad offset(in=0x%x, out=0x%x), pitch(in=0x%x, out=0x%x), line(len=0x%x, cnt=0x%x), fmt(in=0x%x, out=0x%x), notify=0x%x",
					method_registers[NV0039_OFFSET_IN], method_registers[NV0039_OFFSET_OUT], inPitch, outPitch, lineLength, lineCount, inFormat, outFormat, notify);
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

		rsx->sem_flip.post_and_wait();

		//sync
		double limit;
		switch (rpcs3::state.config.rsx.frame_limit.value())
		{
		case rsx_frame_limit::_50: limit = 50.; break;
		case rsx_frame_limit::_59_94: limit = 59.94; break;
		case rsx_frame_limit::_30: limit = 30.; break;
		case rsx_frame_limit::_60: limit = 60.; break;
		case rsx_frame_limit::Auto: limit = rsx->fps_limit; break; //TODO

		case rsx_frame_limit::Off:
		default:
			return;
		}

		std::this_thread::sleep_for(std::chrono::milliseconds((s64)(1000.0 / limit - rsx->timer_sync.GetElapsedTimeInMilliSec())));
		rsx->timer_sync.Start();
		rsx->local_transform_constants.clear();
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

		template<int id, typename T, T impl_func>
		static void bind_impl()
		{
			if (methods[id])
			{
				throw std::logic_error(fmt::format("redefinition rsx method implementation (0x%04x)", id));
			}

			methods[id] = wrapper<id, T, impl_func>;
		}

		template<int id, typename T, T impl_func>
		static void bind_cpu_only_impl()
		{
			if (methods[id])
			{
				throw std::logic_error(fmt::format("redefinition rsx method implementation(cpu only) (0x%04x)", id));
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

			// NV4097
			bind<NV4097_TEXTURE_READ_SEMAPHORE_RELEASE, nv4097::texture_read_semaphore_release>();
			bind<NV4097_BACK_END_WRITE_SEMAPHORE_RELEASE, nv4097::back_end_write_semaphore_release>();
			bind<NV4097_SET_BEGIN_END, nv4097::set_begin_end>();
			bind<NV4097_CLEAR_SURFACE>();
			bind<NV4097_DRAW_ARRAYS, nv4097::draw_arrays>();
			bind<NV4097_DRAW_INDEX_ARRAY, nv4097::draw_index_array>();
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

	u32 get_address(u32 offset, u32 location)
	{
		u32 res = 0;

		switch (location)
		{
		case CELL_GCM_CONTEXT_DMA_MEMORY_FRAME_BUFFER:
		case CELL_GCM_LOCATION_LOCAL:
		{
			//TODO: don't use not named constants like 0xC0000000
			res = 0xC0000000 + offset;
			break;
		}

		case CELL_GCM_CONTEXT_DMA_MEMORY_HOST_BUFFER:
		case CELL_GCM_LOCATION_MAIN:
		{
			res = (u32)RSXIOMem.RealAddr(offset); // TODO: Error Check?
			if (res == 0)
			{
				throw EXCEPTION("GetAddress(offset=0x%x, location=0x%x): RSXIO memory not mapped", offset, location);
			}

			//if (Emu.GetGSManager().GetRender().strict_ordering[offset >> 20])
			//{
			//	_mm_mfence(); // probably doesn't have any effect on current implementation
			//}

			break;
		}
		default:
		{
			throw EXCEPTION("Invalid location (offset=0x%x, location=0x%x)", offset, location);
		}
		}

		return res;
	}

	u32 get_vertex_type_size(u32 type)
	{
		switch (type)
		{
		case CELL_GCM_VERTEX_S1:    return sizeof(u16);
		case CELL_GCM_VERTEX_F:     return sizeof(f32);
		case CELL_GCM_VERTEX_SF:    return sizeof(f16);
		case CELL_GCM_VERTEX_UB:    return sizeof(u8);
		case CELL_GCM_VERTEX_S32K:  return sizeof(u32);
		case CELL_GCM_VERTEX_CMP:   return sizeof(u32);
		case CELL_GCM_VERTEX_UB256: return sizeof(u8) * 4;

		default:
			LOG_ERROR(RSX, "RSXVertexData::GetTypeSize: Bad vertex data type (%d)!", type);
			assert(0);
			return 1;
		}
	}

	void thread::load_vertex_data(u32 first, u32 count)
	{
		vertex_draw_count += count;

		for (int index = 0; index < limits::vertex_count; ++index)
		{
			const auto &info = vertex_arrays_info[index];

			if (!info.array) // disabled or not a vertex array
				continue;

			auto &data = vertex_arrays[index];

			u32 type_size = get_vertex_type_size(info.type);
			u32 element_size = type_size * info.size;

			u32 dst_position = (u32)data.size();
			data.resize(dst_position + count * element_size);
			write_vertex_array_data_to_buffer(data.data() + dst_position, first, count, index, info);
		}
	}

	void thread::load_vertex_index_data(u32 first, u32 count)
	{
		u32 address = get_address(method_registers[NV4097_SET_INDEX_ARRAY_ADDRESS], method_registers[NV4097_SET_INDEX_ARRAY_DMA] & 0xf);
		u32 type = method_registers[NV4097_SET_INDEX_ARRAY_DMA] >> 4;

		u32 type_size = type == CELL_GCM_DRAW_INDEX_ARRAY_TYPE_32 ? sizeof(u32) : sizeof(u16);
		u32 dst_offset = (u32)vertex_index_array.size();
		vertex_index_array.resize(dst_offset + count * type_size);

		u32 base_offset = method_registers[NV4097_SET_VERTEX_DATA_BASE_OFFSET];
		u32 base_index = method_registers[NV4097_SET_VERTEX_DATA_BASE_INDEX];

		switch (type)
		{
		case CELL_GCM_DRAW_INDEX_ARRAY_TYPE_32:
			for (u32 i = 0; i < count; ++i)
			{
				(u32&)vertex_index_array[dst_offset + i * sizeof(u32)] = vm::read32(address + (first + i) * sizeof(u32));
			}
			break;

		case CELL_GCM_DRAW_INDEX_ARRAY_TYPE_16:
			for (u32 i = 0; i < count; ++i)
			{
				(u16&)vertex_index_array[dst_offset + i * sizeof(u16)] = vm::read16(address + (first + i) * sizeof(u16));
			}
			break;
		}
	}

	void thread::capture_frame(const std::string &name)
	{
		frame_capture_data::draw_state draw_state = {};

		int clip_w = rsx::method_registers[NV4097_SET_SURFACE_CLIP_HORIZONTAL] >> 16;
		int clip_h = rsx::method_registers[NV4097_SET_SURFACE_CLIP_VERTICAL] >> 16;
		size_t pitch = clip_w * 4;
		std::vector<size_t> color_index_to_record;
		switch (method_registers[NV4097_SET_SURFACE_COLOR_TARGET])
		{
		case CELL_GCM_SURFACE_TARGET_0:
			color_index_to_record = { 0 };
			break;
		case CELL_GCM_SURFACE_TARGET_1:
			color_index_to_record = { 1 };
			break;
		case CELL_GCM_SURFACE_TARGET_MRT1:
			color_index_to_record = { 0, 1 };
			break;
		case CELL_GCM_SURFACE_TARGET_MRT2:
			color_index_to_record = { 0, 1, 2 };
			break;
		case CELL_GCM_SURFACE_TARGET_MRT3:
			color_index_to_record = { 0, 1, 2, 3 };
			break;
		}
		for (size_t i : color_index_to_record)
		{
			draw_state.color_buffer[i].width = clip_w;
			draw_state.color_buffer[i].height = clip_h;
			draw_state.color_buffer[i].data.resize(pitch * clip_h);
			copy_render_targets_to_memory(draw_state.color_buffer[i].data.data(), i);
		}
		if (get_address(method_registers[NV4097_SET_SURFACE_ZETA_OFFSET], method_registers[NV4097_SET_CONTEXT_DMA_ZETA]))
		{
			draw_state.depth.width = clip_w;
			draw_state.depth.height = clip_h;
			draw_state.depth.data.resize(clip_w * clip_h * 4);
			copy_depth_buffer_to_memory(draw_state.depth.data.data());
			draw_state.stencil.width = clip_w;
			draw_state.stencil.height = clip_h;
			draw_state.stencil.data.resize(clip_w * clip_h * 4);
			copy_stencil_buffer_to_memory(draw_state.stencil.data.data());
		}
		draw_state.programs = get_programs();
		draw_state.name = name;
		frame_debug.draw_calls.push_back(draw_state);
	}

	void thread::begin()
	{
		draw_mode = method_registers[NV4097_SET_BEGIN_END];
	}

	void thread::end()
	{
		vertex_index_array.clear();
		for (auto &vertex_array : vertex_arrays)
			vertex_array.clear();

		transform_constants.clear();

		if (capture_current_frame)
			capture_frame("Draw " + std::to_string(vertex_draw_count));
	}

	void thread::on_task()
	{
		on_init_thread();

		reset();

		last_flip_time = get_system_time() - 1000000;

		scope_thread_t vblank(PURE_EXPR("VBlank Thread"s), [this]()
		{
			const u64 start_time = get_system_time();

			vblank_count = 0;

			// TODO: exit condition
			while (!Emu.IsStopped())
			{
				if (get_system_time() - start_time > vblank_count * 1000000 / 60)
				{
					vblank_count++;

					if (vblank_handler)
					{
						Emu.GetCallbackManager().Async([func = vblank_handler](PPUThread& ppu)
						{
							func(ppu, 1);
						});
					}

					continue;
				}

				std::this_thread::sleep_for(1ms); // hack
			}
		});

		// TODO: exit condition
		while (true)
		{
			CHECK_EMU_STATUS;

			be_t<u32> get = ctrl->get;
			be_t<u32> put = ctrl->put;

			if (put == get || !Emu.IsRunning())
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(1)); // hack
				continue;
			}

			const u32 cmd = ReadIO32(get);
			const u32 count = (cmd >> 18) & 0x7ff;

			if (cmd & CELL_GCM_METHOD_FLAG_JUMP)
			{
				u32 offs = cmd & 0x1fffffff;
				//LOG_WARNING(RSX, "rsx jump(0x%x) #addr=0x%x, cmd=0x%x, get=0x%x, put=0x%x", offs, m_ioAddress + get, cmd, get, put);
				ctrl->get = offs;
				continue;
			}
			if (cmd & CELL_GCM_METHOD_FLAG_CALL)
			{
				m_call_stack.push(get + 4);
				u32 offs = cmd & ~3;
				//LOG_WARNING(RSX, "rsx call(0x%x) #0x%x - 0x%x", offs, cmd, get);
				ctrl->get = offs;
				continue;
			}
			if (cmd == CELL_GCM_METHOD_FLAG_RETURN)
			{
				u32 get = m_call_stack.top();
				m_call_stack.pop();
				//LOG_WARNING(RSX, "rsx return(0x%x)", get);
				ctrl->get = get;
				continue;
			}

			if (cmd == 0) //nop
			{
				ctrl->get = get + 4;
				continue;
			}

			auto args = vm::ptr<u32>::make((u32)RSXIOMem.RealAddr(get + 4));

			u32 first_cmd = (cmd & 0xffff) >> 2;

			if (cmd & 0x3)
			{
				LOG_WARNING(Log::RSX, "unaligned command: %s (0x%x from 0x%x)", get_method_name(first_cmd).c_str(), first_cmd, cmd & 0xffff);
			}

			for (u32 i = 0; i < count; i++)
			{
				u32 reg = cmd & CELL_GCM_METHOD_FLAG_NON_INCREMENT ? first_cmd : first_cmd + i;
				u32 value = args[i];

				if (rpcs3::config.misc.log.rsx_logging.value())
				{
					LOG_NOTICE(Log::RSX, "%s(0x%x) = 0x%x", get_method_name(reg).c_str(), reg, value);
				}

				method_registers[reg] = value;
				if (capture_current_frame)
					frame_debug.command_queue.push_back(std::make_pair(reg, value));

				if (auto method = methods[reg])
					method(this, value);
			}

			ctrl->get = get + (count + 1) * 4;
		}
	}

	std::string thread::get_name() const
	{
		return "rsx::thread"s;
	}

	void thread::fill_scale_offset_data(void *buffer, bool is_d3d) const noexcept
	{
		int clip_w = rsx::method_registers[NV4097_SET_SURFACE_CLIP_HORIZONTAL] >> 16;
		int clip_h = rsx::method_registers[NV4097_SET_SURFACE_CLIP_VERTICAL] >> 16;

		float scale_x = (float&)rsx::method_registers[NV4097_SET_VIEWPORT_SCALE] / (clip_w / 2.f);
		float offset_x = (float&)rsx::method_registers[NV4097_SET_VIEWPORT_OFFSET] - (clip_w / 2.f);
		offset_x /= clip_w / 2.f;

		float scale_y = (float&)rsx::method_registers[NV4097_SET_VIEWPORT_SCALE + 1] / (clip_h / 2.f);
		float offset_y = ((float&)rsx::method_registers[NV4097_SET_VIEWPORT_OFFSET + 1] - (clip_h / 2.f));
		offset_y /= clip_h / 2.f;
		if (is_d3d) scale_y *= -1;
		if (is_d3d) offset_y *= -1;

		float scale_z = (float&)rsx::method_registers[NV4097_SET_VIEWPORT_SCALE + 2];
		float offset_z = (float&)rsx::method_registers[NV4097_SET_VIEWPORT_OFFSET + 2];
		if (!is_d3d) offset_z -= .5;

		float one = 1.f;

		stream_vector(buffer, (u32&)scale_x, 0, 0, (u32&)offset_x);
		stream_vector((char*)buffer + 16, 0, (u32&)scale_y, 0, (u32&)offset_y);
		stream_vector((char*)buffer + 32, 0, 0, (u32&)scale_z, (u32&)offset_z);
		stream_vector((char*)buffer + 48, 0, 0, 0, (u32&)one);
	}

	/**
	* Fill buffer with vertex program constants.
	* Buffer must be at least 512 float4 wide.
	*/
	void thread::fill_vertex_program_constants_data(void *buffer) noexcept
	{
		for (const auto &entry : transform_constants)
			local_transform_constants[entry.first] = entry.second;
		for (const auto &entry : local_transform_constants)
			stream_vector_from_memory((char*)buffer + entry.first * 4 * sizeof(float), (void*)entry.second.rgba);
	}

	u64 thread::timestamp() const
	{
		// Get timestamp, and convert it from microseconds to nanoseconds
		return get_system_time() * 1000;
	}

	void thread::reset()
	{
		//setup method registers
		std::memset(method_registers, 0, sizeof(method_registers));

		method_registers[NV4097_SET_COLOR_MASK] = CELL_GCM_COLOR_MASK_R | CELL_GCM_COLOR_MASK_G | CELL_GCM_COLOR_MASK_B | CELL_GCM_COLOR_MASK_A;
		method_registers[NV4097_SET_SCISSOR_HORIZONTAL] = (4096 << 16) | 0;
		method_registers[NV4097_SET_SCISSOR_VERTICAL] = (4096 << 16) | 0;

		method_registers[NV4097_SET_ALPHA_FUNC] = CELL_GCM_ALWAYS;
		method_registers[NV4097_SET_ALPHA_REF] = 0;

		method_registers[NV4097_SET_BLEND_FUNC_SFACTOR] = (CELL_GCM_ONE << 16) | CELL_GCM_ONE;
		method_registers[NV4097_SET_BLEND_FUNC_DFACTOR] = (CELL_GCM_ZERO << 16) | CELL_GCM_ZERO;
		method_registers[NV4097_SET_BLEND_COLOR] = 0;
		method_registers[NV4097_SET_BLEND_COLOR2] = 0;
		method_registers[NV4097_SET_BLEND_EQUATION] = (CELL_GCM_FUNC_ADD << 16) | CELL_GCM_FUNC_ADD;

		method_registers[NV4097_SET_STENCIL_MASK] = 0xff;
		method_registers[NV4097_SET_STENCIL_FUNC] = CELL_GCM_ALWAYS;
		method_registers[NV4097_SET_STENCIL_FUNC_REF] = 0x00;
		method_registers[NV4097_SET_STENCIL_FUNC_MASK] = 0xff;
		method_registers[NV4097_SET_STENCIL_OP_FAIL] = CELL_GCM_KEEP;
		method_registers[NV4097_SET_STENCIL_OP_ZFAIL] = CELL_GCM_KEEP;
		method_registers[NV4097_SET_STENCIL_OP_ZPASS] = CELL_GCM_KEEP;

		method_registers[NV4097_SET_BACK_STENCIL_MASK] = 0xff;
		method_registers[NV4097_SET_BACK_STENCIL_FUNC] = CELL_GCM_ALWAYS;
		method_registers[NV4097_SET_BACK_STENCIL_FUNC_REF] = 0x00;
		method_registers[NV4097_SET_BACK_STENCIL_FUNC_MASK] = 0xff;
		method_registers[NV4097_SET_BACK_STENCIL_OP_FAIL] = CELL_GCM_KEEP;
		method_registers[NV4097_SET_BACK_STENCIL_OP_ZFAIL] = CELL_GCM_KEEP;
		method_registers[NV4097_SET_BACK_STENCIL_OP_ZPASS] = CELL_GCM_KEEP;

		method_registers[NV4097_SET_SHADE_MODE] = CELL_GCM_SMOOTH;

		method_registers[NV4097_SET_LOGIC_OP] = CELL_GCM_COPY;

		(f32&)method_registers[NV4097_SET_DEPTH_BOUNDS_MIN] = 0.f;
		(f32&)method_registers[NV4097_SET_DEPTH_BOUNDS_MAX] = 1.f;

		(f32&)method_registers[NV4097_SET_CLIP_MIN] = 0.f;
		(f32&)method_registers[NV4097_SET_CLIP_MAX] = 1.f;

		method_registers[NV4097_SET_LINE_WIDTH] = 1 << 3;

		method_registers[NV4097_SET_FOG_MODE] = CELL_GCM_FOG_MODE_EXP;

		method_registers[NV4097_SET_DEPTH_FUNC] = CELL_GCM_LESS;
		method_registers[NV4097_SET_DEPTH_MASK] = CELL_GCM_TRUE;
		(f32&)method_registers[NV4097_SET_POLYGON_OFFSET_SCALE_FACTOR] = 0.f;
		(f32&)method_registers[NV4097_SET_POLYGON_OFFSET_BIAS] = 0.f;
		method_registers[NV4097_SET_FRONT_POLYGON_MODE] = CELL_GCM_POLYGON_MODE_FILL;
		method_registers[NV4097_SET_BACK_POLYGON_MODE] = CELL_GCM_POLYGON_MODE_FILL;
		method_registers[NV4097_SET_CULL_FACE] = CELL_GCM_BACK;
		method_registers[NV4097_SET_FRONT_FACE] = CELL_GCM_CCW;
		method_registers[NV4097_SET_RESTART_INDEX] = -1;

		method_registers[NV4097_SET_CLEAR_RECT_HORIZONTAL] = (4096 << 16) | 0;
		method_registers[NV4097_SET_CLEAR_RECT_VERTICAL] = (4096 << 16) | 0;

		method_registers[NV4097_SET_ZSTENCIL_CLEAR_VALUE] = 0xffffffff;

		// Construct Textures
		for (int i = 0; i < limits::textures_count; i++)
		{
			textures[i].init(i);
		}
	}

	void thread::init(const u32 ioAddress, const u32 ioSize, const u32 ctrlAddress, const u32 localAddress)
	{
		ctrl = vm::_ptr<CellGcmControl>(ctrlAddress);
		this->ioAddress = ioAddress;
		this->ioSize = ioSize;
		local_mem_addr = localAddress;
		flip_status = 0;

		m_used_gcm_commands.clear();

		on_init();
		start();
	}

	u32 thread::ReadIO32(u32 addr)
	{
		u32 value;
	
		if (!RSXIOMem.Read32(addr, &value))
		{
			throw EXCEPTION("%s(addr=0x%x): RSXIO memory not mapped", __FUNCTION__, addr);
		}

		return value;
	}

	void thread::WriteIO32(u32 addr, u32 value)
	{
		if (!RSXIOMem.Write32(addr, value))
		{
			throw EXCEPTION("%s(addr=0x%x): RSXIO memory not mapped", __FUNCTION__, addr);
		}
	}
}
