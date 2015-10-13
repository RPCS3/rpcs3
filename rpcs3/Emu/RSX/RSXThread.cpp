#include "stdafx.h"
#include "rpcs3/Ini.h"
#include "Utilities/Log.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/RSX/GSManager.h"
#include "RSXThread.h"

#include "Emu/SysCalls/Callback.h"
#include "Emu/SysCalls/CB_FUNC.h"
#include "Emu/SysCalls/lv2/sys_time.h"

#include "Utilities/types.h"
#include <chrono>

using namespace std::chrono_literals;

extern "C"
{
#include "libswscale/swscale.h"
}

#define CMD_DEBUG 0

extern u64 get_system_time();

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
			vm::write32(rsx->label_addr + rsx::method_registers[NV4097_SET_SEMAPHORE_OFFSET], arg);
		}

		force_inline void back_end_write_semaphore_release(thread* rsx, u32 arg)
		{
			//TODO: dma
			vm::ps3::write32(rsx->label_addr + rsx::method_registers[NV4097_SET_SEMAPHORE_OFFSET],
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
		};		template<u32 index>
		struct set_vertex_data3f_m
		{
			force_inline static void impl(thread* rsx, u32 arg)
			{
				set_vertex_data_impl<NV4097_SET_VERTEX_DATA3F_M, index, 3, f32>(rsx, arg);
			}
		};		template<u32 index>
		struct set_vertex_data4f_m
		{
			force_inline static void impl(thread* rsx, u32 arg)
			{
				set_vertex_data_impl<NV4097_SET_VERTEX_DATA4F_M, index, 4, f32>(rsx, arg);
			}		template<u32 index>
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
				u32& load = method_registers[NV4097_SET_TRANSFORM_CONSTANT_LOAD];

				static const size_t count = 4;
				static const size_t size = count * sizeof(f32);

				memcpy(rsxthr->transform_constants[load++].rgba, method_registers + NV4097_SET_TRANSFORM_CONSTANT + index * count, size);
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
			vm::ptr<CellGcmReportData> result = { rsx->local_mem_addr + offset };

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
		force_inline void image_in(u32 arg)
		{
			const u16 width = method_registers[NV3089_IMAGE_IN_SIZE];
			const u16 height = method_registers[NV3089_IMAGE_IN_SIZE] >> 16;
			const u16 pitch = method_registers[NV3089_IMAGE_IN_FORMAT];
			const u8 origin = method_registers[NV3089_IMAGE_IN_FORMAT] >> 16;
			const u8 inter = method_registers[NV3089_IMAGE_IN_FORMAT] >> 24;

			if (origin != 2 /* CELL_GCM_TRANSFER_ORIGIN_CORNER */)
			{
				LOG_ERROR(RSX, "NV3089_IMAGE_IN_SIZE: unknown origin (%d)", origin);
			}

			if (inter != 0 /* CELL_GCM_TRANSFER_INTERPOLATOR_ZOH */ && inter != 1 /* CELL_GCM_TRANSFER_INTERPOLATOR_FOH */)
			{
				LOG_ERROR(RSX, "NV3089_IMAGE_IN_SIZE: unknown inter (%d)", inter);
			}

			const u32 src_offset = method_registers[NV3089_IMAGE_IN_OFFSET];
			const u32 src_dma = method_registers[NV3089_SET_CONTEXT_DMA_IMAGE];

			u32 dst_offset;
			u32 dst_dma = 0;

			switch (method_registers[NV3089_SET_CONTEXT_SURFACE])
			{
			case CELL_GCM_CONTEXT_SURFACE2D:
				dst_dma = method_registers[NV3062_SET_CONTEXT_DMA_IMAGE_DESTIN];
				dst_offset = method_registers[NV3062_SET_OFFSET_DESTIN];
				break;

			case CELL_GCM_CONTEXT_SWIZZLE2D:
				dst_dma = method_registers[NV309E_SET_CONTEXT_DMA_IMAGE];
				dst_offset = method_registers[NV309E_SET_OFFSET];
				break;

			default:
				LOG_ERROR(RSX, "NV3089_IMAGE_IN_SIZE: unknown m_context_surface (0x%x)", method_registers[NV3089_SET_CONTEXT_SURFACE]);
				break;
			}

			if (!dst_dma)
				return;

			LOG_ERROR(RSX, "NV3089_IMAGE_IN_SIZE: src = 0x%x, dst = 0x%x", src_offset, dst_offset);

			const u16 u = arg; // inX (currently ignored)
			const u16 v = arg >> 16; // inY (currently ignored)

			u8* pixels_src = vm::get_ptr<u8>(get_address(src_offset, src_dma));
			u8* pixels_dst = vm::get_ptr<u8>(get_address(dst_offset, dst_dma));

			if (method_registers[NV3062_SET_COLOR_FORMAT] != 4 /* CELL_GCM_TRANSFER_SURFACE_FORMAT_R5G6B5 */ &&
				method_registers[NV3062_SET_COLOR_FORMAT] != 10 /* CELL_GCM_TRANSFER_SURFACE_FORMAT_A8R8G8B8 */)
			{
				LOG_ERROR(RSX, "NV3089_IMAGE_IN_SIZE: unknown m_color_format (%d)", method_registers[NV3062_SET_COLOR_FORMAT]);
			}

			const u32 in_bpp = method_registers[NV3062_SET_COLOR_FORMAT] == 4 ? 2 : 4; // bytes per pixel
			const u32 out_bpp = method_registers[NV3089_SET_COLOR_FORMAT] == 7 ? 2 : 4;

			const s32 out_w = (s32)(u64(width) * (1 << 20) / method_registers[NV3089_DS_DX]);
			const s32 out_h = (s32)(u64(height) * (1 << 20) / method_registers[NV3089_DT_DY]);

			if (method_registers[NV3089_SET_CONTEXT_SURFACE] == CELL_GCM_CONTEXT_SWIZZLE2D)
			{
				u8* linear_pixels = pixels_src;
				u8* swizzled_pixels = new u8[in_bpp * width * height];

				int sw_width = 1 << (int)log2(width);
				int sw_height = 1 << (int)log2(height);

				for (int y = 0; y < sw_height; y++)
				{
					for (int x = 0; x < sw_width; x++)
					{
						switch (in_bpp)
						{
						case 1:
							swizzled_pixels[linear_to_swizzle(x, y, 0, sw_width, sw_height, 0)] = linear_pixels[y * sw_height + x];
							break;
						case 2:
							((u16*)swizzled_pixels)[linear_to_swizzle(x, y, 0, sw_width, sw_height, 0)] = ((u16*)linear_pixels)[y * sw_height + x];
							break;
						case 4:
							((u32*)swizzled_pixels)[linear_to_swizzle(x, y, 0, sw_width, sw_height, 0)] = ((u32*)linear_pixels)[y * sw_height + x];
							break;
						}
					}
				}

				pixels_src = swizzled_pixels;
			}

			LOG_WARNING(RSX, "NV3089_IMAGE_IN_SIZE: w=%d, h=%d, pitch=%d, offset=0x%x, inX=%f, inY=%f, scaleX=%f, scaleY=%f",
				width, height, pitch, src_offset, double(u) / 16, double(v) / 16, double(1 << 20) / (method_registers[NV3089_DS_DX]),
				double(1 << 20) / (method_registers[NV3089_DT_DY]));

			std::unique_ptr<u8[]> temp;

			if (in_bpp != out_bpp && width != out_w && height != out_h)
			{
				// resize/convert if necessary

				temp.reset(new u8[out_bpp * out_w * out_h]);

				AVPixelFormat in_format = method_registers[NV3062_SET_COLOR_FORMAT] == 4 ? AV_PIX_FMT_RGB565BE : AV_PIX_FMT_ARGB; // ???
				AVPixelFormat out_format = method_registers[NV3089_SET_COLOR_FORMAT] == 7 ? AV_PIX_FMT_RGB565BE : AV_PIX_FMT_ARGB; // ???

				std::unique_ptr<SwsContext, void(*)(SwsContext*)> sws(sws_getContext(width, height, in_format, out_w, out_h, out_format,
					inter ? SWS_FAST_BILINEAR : SWS_POINT, NULL, NULL, NULL), sws_freeContext);

				int in_line = in_bpp * width;
				u8* out_ptr = temp.get();
				int out_line = out_bpp * out_w;

				sws_scale(sws.get(), &pixels_src, &in_line, 0, height, &out_ptr, &out_line);

				pixels_src = temp.get(); // use resized image as a source
			}

			if (method_registers[NV3089_CLIP_SIZE] != method_registers[NV3089_IMAGE_OUT_SIZE] ||
				method_registers[NV3089_IMAGE_OUT_SIZE] != (out_w | (out_h << 16)) ||
				method_registers[NV3089_IMAGE_OUT_POINT] || method_registers[NV3089_CLIP_POINT])
			{
				// clip if necessary

				for (s32 y = method_registers[NV3089_CLIP_POINT] >> 16, dst_y = method_registers[NV3089_IMAGE_OUT_POINT] >> 16; y < out_h; y++, dst_y++)
				{
					if (dst_y >= 0 && dst_y < method_registers[NV3089_IMAGE_OUT_SIZE] >> 16)
					{
						// destination line
						u8* dst_line = pixels_dst + dst_y * out_bpp * (method_registers[NV3089_IMAGE_OUT_SIZE] & 0xffff)
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

							memset(z0.first, 0, z0.second);
							memcpy(d0.first, src_line, d0.second);
							memset(z1.first, 0, z1.second);
						}
						else
						{
							memset(dst_line, 0, dst_max);
						}
					}
				}
			}
			else
			{
				memcpy(pixels_dst, pixels_src, out_w * out_h * out_bpp);
			}

			if (method_registers[NV3089_SET_CONTEXT_SURFACE] == CELL_GCM_CONTEXT_SWIZZLE2D)
			{
				delete[] pixels_src;
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
				memcpy(
					vm::get_ptr(get_address(method_registers[NV0039_OFFSET_OUT], method_registers[NV0039_SET_CONTEXT_DMA_BUFFER_OUT])),
					vm::get_ptr(get_address(method_registers[NV0039_OFFSET_IN], method_registers[NV0039_SET_CONTEXT_DMA_BUFFER_IN])),
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
		rsx->gcm_current_buffer = arg;
		rsx->flip(arg);

		rsx->last_flip_time = get_system_time() - 1000000;
		rsx->gcm_current_buffer = arg;
		rsx->flip_status = 0;

		if (auto cb = rsx->flip_handler)
		{
			Emu.GetCallbackManager().Async([=](CPUThread& cpu)
			{
				cb(static_cast<PPUThread&>(cpu), 1);
			});
		}

		rsx->sem_flip.post_and_wait();

		//sync
		double limit;
		switch (Ini.GSFrameLimit.GetValue())
		{
		case 1: limit = 50.; break;
		case 2: limit = 59.94; break;
		case 3: limit = 30.; break;
		case 4: limit = 60.; break;
		case 5: limit = rsx->fps_limit; break; //TODO

		case 0:
		default:
			return;
		}

		std::this_thread::sleep_for(std::chrono::milliseconds((s64)(1000.0 / limit - rsx->timer_sync.GetElapsedTimeInMilliSec())));
		rsx->timer_sync.Start();
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
			if (rsx->domethod(id, arg))
				return;

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
				bind<id, T<id>::impl>();
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
			bind_range<NV4097_SET_TRANSFORM_CONSTANT + 3, 4, 8, nv4097::set_transform_constant>();
			bind_range<NV4097_SET_TRANSFORM_PROGRAM + 3, 4, 128, nv4097::set_transform_program>();
			bind_cpu_only<NV4097_GET_REPORT, nv4097::get_report>();
			bind_cpu_only<NV4097_CLEAR_REPORT_VALUE, nv4097::clear_report_value>();

			//NV308A
			bind_range<NV308A_COLOR, 1, 512, nv308a::color>();

			//NV3089
			bind<NV3089_IMAGE_IN, nv3089::image_in>();

			//NV0039
			bind<NV0039_BUFFER_NOTIFY, nv0039::buffer_notify>();

			// custom methods
			bind_cpu_only<GCM_FLIP_COMMAND, flip_command>();
		}
	} __rsx_methods;

	u32 linear_to_swizzle(u32 x, u32 y, u32 z, u32 log2_width, u32 log2_height, u32 log2_depth)
	{
		u32 offset = 0;
		u32 shift_count = 0;
		while (log2_width | log2_height | log2_depth)
		{
			if (log2_width)
			{
				offset |= (x & 0x01) << shift_count;
				x >>= 1;
				++shift_count;
				--log2_width;
			}

			if (log2_height)
			{
				offset |= (y & 0x01) << shift_count;
				y >>= 1;
				++shift_count;
				--log2_height;
			}

			if (log2_depth)
			{
				offset |= (z & 0x01) << shift_count;
				z >>= 1;
				++shift_count;
				--log2_depth;
			}
		}
		return offset;
	}

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
				throw fmt::format("GetAddress(offset=0x%x, location=0x%x): RSXIO memory not mapped", offset, location);
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
			auto &info = vertex_arrays_info[index];

			if (!info.array) // disabled or not a vertex array
			{
				continue;
			}

			auto &data = vertex_arrays[index];

			if (info.frequency)
			{
				LOG_ERROR(RSX, "%s: frequency is not null (%d, index=%d)", __FUNCTION__, info.frequency, index);
			}

			u32 offset = method_registers[NV4097_SET_VERTEX_DATA_ARRAY_OFFSET + index];
			u32 address = get_address(offset & 0x7fffffff, offset >> 31);

			u32 type_size = get_vertex_type_size(info.type);
			u32 element_size = type_size * info.size;

			u32 dst_position = (u32)data.size();
			data.resize(dst_position + count * element_size);

			u32 base_offset = method_registers[NV4097_SET_VERTEX_DATA_BASE_OFFSET];
			u32 base_index = method_registers[NV4097_SET_VERTEX_DATA_BASE_INDEX];

			for (u32 i = 0; i < count; ++i)
			{
				auto src = vm::get_ptr<const u8>(address + base_offset + info.stride * (first + i + base_index));
				u8* dst = data.data() + dst_position + i * element_size;

				switch (type_size)
				{
				case 1:
					memcpy(dst, src, info.size);
					break;

				case 2:
				{
					auto* c_src = (const be_t<u16>*)src;
					u16* c_dst = (u16*)dst;

					for (u32 j = 0; j < info.size; ++j)
					{
						*c_dst++ = *c_src++;
					}
					break;
				}

				case 4:
				{
					auto* c_src = (const be_t<u32>*)src;
					u32* c_dst = (u32*)dst;

					for (u32 j = 0; j < info.size; ++j)
					{
						*c_dst++ = *c_src++;
					}
					break;
				}
				}
			}
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
	}

	void thread::task()
	{
		u8 inc;
		LOG_NOTICE(RSX, "RSX thread started");

		oninit_thread();

		last_flip_time = get_system_time() - 1000000;

		autojoin_thread_t  vblank(WRAP_EXPR("VBlank Thread"), [this]()
		{
			const u64 start_time = get_system_time();

			vblank_count = 0;

			while (joinable())
			{
				CHECK_EMU_STATUS;

				if (get_system_time() - start_time > vblank_count * 1000000 / 60)
				{
					vblank_count++;

					if (auto cb = vblank_handler)
					{
						Emu.GetCallbackManager().Async([=](CPUThread& cpu)
						{
							cb(static_cast<PPUThread&>(cpu), 1);
						});
					}

					continue;
				}

				std::this_thread::sleep_for(1ms); // hack
			}
		});

		reset();

		try
		{
			while (joinable())
			{
				//TODO: async mode
				if (Emu.IsStopped())
				{
					LOG_WARNING(RSX, "RSX thread aborted");
					break;
				}
				std::lock_guard<std::mutex> lock(cs_main);

				inc = 1;

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
					ctrl->get.exchange(offs);
					continue;
				}
				if (cmd & CELL_GCM_METHOD_FLAG_CALL)
				{
					m_call_stack.push(get + 4);
					u32 offs = cmd & ~3;
					//LOG_WARNING(RSX, "rsx call(0x%x) #0x%x - 0x%x", offs, cmd, get);
					ctrl->get.exchange(offs);
					continue;
				}
				if (cmd == CELL_GCM_METHOD_FLAG_RETURN)
				{
					u32 get = m_call_stack.top();
					m_call_stack.pop();
					//LOG_WARNING(RSX, "rsx return(0x%x)", get);
					ctrl->get.exchange(get);
					continue;
				}
				if (cmd & CELL_GCM_METHOD_FLAG_NON_INCREMENT)
				{
					//LOG_WARNING(RSX, "rsx non increment cmd! 0x%x", cmd);
					inc = 0;
				}

				if (cmd == 0) //nop
				{
					ctrl->get.atomic_op([](be_t<u32>& value)
					{
						value += 4;
					});

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
					u32 reg = first_cmd + (i * inc);
					u32 value = args[i];

					if (Ini.RSXLogging.GetValue())
					{
						LOG_NOTICE(Log::RSX, "%s(0x%x) = 0x%x", get_method_name(reg).c_str(), reg, value);
					}

					method_registers[reg] = value;

					if (auto method = methods[reg])
						method(this, value);
				}

				ctrl->get.atomic_op([count](be_t<u32>& value)
				{
					value += (count + 1) * 4;
				});
			}
		}
		catch (const std::exception& ex)
		{
			LOG_ERROR(Log::RSX, ex.what());

			std::rethrow_exception(std::current_exception());
		}

		LOG_NOTICE(RSX, "RSX thread ended");

		onexit_thread();
	}

	u64 thread::timestamp() const
	{
		// Get timestamp, and convert it from microseconds to nanoseconds
		return get_system_time() * 1000;
	}

	void thread::reset()
	{
		//setup method registers
		memset(method_registers, 0, sizeof(method_registers));

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
		ctrl = vm::get_ptr<CellGcmControl>(ctrlAddress);
		this->ioAddress = ioAddress;
		this->ioSize = ioSize;
		local_mem_addr = localAddress;
		flip_status = 0;

		m_used_gcm_commands.clear();

		oninit();
		named_thread_t::start(WRAP_EXPR("rsx::thread"), WRAP_EXPR(task()));
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
