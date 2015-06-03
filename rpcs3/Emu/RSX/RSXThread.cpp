#include "stdafx.h"
#include "rpcs3/Ini.h"
#include "Utilities/Log.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/RSX/GSManager.h"
#include "Emu/RSX/RSXDMA.h"
#include "RSXThread.h"

#include "Emu/SysCalls/Callback.h"
#include "Emu/SysCalls/CB_FUNC.h"
#include "Emu/SysCalls/lv2/sys_time.h"

extern "C"
{
#include "libswscale/swscale.h"
}

#define CMD_DEBUG 0

/*
namespace emu
{
	class initialize_context_t
	{
	public:
		std::vector<std::function<void()>> onload_funcs;
	} initialize_context_t;

	//fire on execution emulator
	class onload
	{
	public:
		onload(std::function<void()> func)
		{
			initialize_context_t.onload_funcs.push_back(func);
		}
	};
}*/

namespace rsx
{
	using rsx_method_t = void(*)(thread*, u32);

	u32 method_registers[0x10000 >> 2];
	rsx_method_t methods[0x10000 >> 2] = { nullptr };

	template<typename Type> struct vertex_data_type_from_element_type;
	template<> struct vertex_data_type_from_element_type<float> { enum { type = CELL_GCM_VERTEX_F }; };
	template<> struct vertex_data_type_from_element_type<f16> { enum { type = CELL_GCM_VERTEX_SF }; };
	template<> struct vertex_data_type_from_element_type<u8> { enum { type = CELL_GCM_VERTEX_UB }; };
	template<> struct vertex_data_type_from_element_type<u16> { enum { type = CELL_GCM_VERTEX_S1 }; };

	namespace nv406e
	{
		__forceinline void set_reference(thread* rsx, u32 arg)
		{
			rsx->ctrl->ref.exchange(be_t<u32>::make(arg));
		}

		__forceinline void semaphore_acquire(thread* rsx, u32 arg)
		{
			//TODO: dma
			while (vm::read32(rsx->label_addr + method_registers[NV406E_SEMAPHORE_OFFSET]) != arg)
			{
				if (Emu.IsStopped())
					break;

				std::this_thread::sleep_for(std::chrono::milliseconds(1));
			}
		}

		__forceinline void semaphore_release(thread* rsx, u32 arg)
		{
			//TODO: dma
			vm::write32(rsx->label_addr + method_registers[NV406E_SEMAPHORE_OFFSET], arg);
		}
	}

	namespace nv4097
	{
		__forceinline void texture_read_semaphore_release(thread* rsx, u32 arg)
		{
			//TODO: dma
			vm::write32(rsx->label_addr + method_registers[NV4097_SET_SEMAPHORE_OFFSET], arg);
		}

		__forceinline void back_end_write_semaphore_release(thread* rsx, u32 arg)
		{
			//TODO: dma
			vm::write32(rsx->label_addr + method_registers[NV4097_SET_SEMAPHORE_OFFSET],
				(arg & 0xff00ff00) | ((arg & 0xff) << 16) | ((arg >> 16) & 0xff));
		}

		//fire only when all data passed to rsx cmd buffer
		template<u32 id, u32 index, int count, typename type>
		__forceinline void set_vertex_data_impl(thread* rsx, u32 arg)
		{
			static const size_t element_size = (count * sizeof(type));
			static const size_t element_size_in_words = element_size / sizeof(u32);

			auto& entry = rsx->vertex_data_array[index];

			//find begin of data
			size_t begin = id + index * element_size_in_words;

			size_t position = entry.data.size();
			entry.data.resize(position + element_size);

			entry.size = count;
			entry.type = vertex_data_type_from_element_type<type>::type;

			memcpy(entry.data.data() + position, method_registers + begin, element_size);
		}

		template<u32 index>
		__forceinline void set_vertex_data4ub_m(thread* rsx, u32 arg)
		{
			set_vertex_data_impl<NV4097_SET_VERTEX_DATA4UB_M, index, 4, u8>(rsx, arg);
		}

		template<u32 index>
		__forceinline void set_vertex_data1f_m(thread* rsx, u32 arg)
		{
			set_vertex_data_impl<NV4097_SET_VERTEX_DATA1F_M, index, 1, f32>(rsx, arg);
		}

		template<u32 index>
		__forceinline void set_vertex_data2f_m(thread* rsx, u32 arg)
		{
			set_vertex_data_impl<NV4097_SET_VERTEX_DATA1F_M, index, 2, f32>(rsx, arg);
		}

		template<u32 index>
		__forceinline void set_vertex_data3f_m(thread* rsx, u32 arg)
		{
			set_vertex_data_impl<NV4097_SET_VERTEX_DATA1F_M, index, 3, f32>(rsx, arg);
		}

		template<u32 index>
		__forceinline void set_vertex_data4f_m(thread* rsx, u32 arg)
		{
			set_vertex_data_impl<NV4097_SET_VERTEX_DATA1F_M, index, 4, f32>(rsx, arg);
		}

		template<u32 index>
		__forceinline void set_vertex_data2s_m(thread* rsx, u32 arg)
		{
			set_vertex_data_impl<NV4097_SET_VERTEX_DATA2S_M, index, 2, u16>(rsx, arg);
		}

		template<u32 index>
		__forceinline void set_vertex_data4s_m(thread* rsx, u32 arg)
		{
			set_vertex_data_impl<NV4097_SET_VERTEX_DATA4S_M, index, 4, u16>(rsx, arg);
		}

		__forceinline void draw_arrays(thread* rsx, u32 arg)
		{
			const u32 first = arg & 0xffffff;
			const u32 count = (arg >> 24) + 1;

			//rsx->vertex_data_array.load(first, count);
		}

		__forceinline void draw_index_array(thread* rsx, u32 arg)
		{
			u32 first = arg & 0xffffff;
			u32 count = (arg >> 24) + 1;

			u32 address = get_address(method_registers[NV4097_SET_INDEX_ARRAY_ADDRESS], method_registers[NV4097_SET_INDEX_ARRAY_DMA] & 0xf);
			u32 type = method_registers[NV4097_SET_INDEX_ARRAY_DMA] >> 4;

			u32 type_size = type == 0 ? 4 : 2;
			u32 packet_size = (first + count) * type_size;

			if (rsx->index_array.data.size() < packet_size)
			{
				rsx->index_array.data.resize(packet_size);
			}

			if (type == 0)
			{
				for (u32 i = first; i < count; ++i)
				{
					(u32&)rsx->index_array.data[i * 4] = vm::read32(address + i * 4);
				}
			}
			else
			{
				for (u32 i = first; i < count; ++i)
				{
					(u16&)rsx->index_array.data[i * 2] = vm::read16(address + i * 2);
				}
			}

			rsx->index_array.entries.push_back({ first, count });
		}

		template<u32 index>
		__forceinline void set_transform_constant(thread* rsx, u32 arg)
		{
			u32& load = method_registers[NV4097_SET_TRANSFORM_CONSTANT_LOAD];

			static const size_t count = 4;
			static const size_t size = count * sizeof(f32);

			memcpy(rsx->transform_constants[load].rgba, method_registers + NV4097_SET_TRANSFORM_CONSTANT + index * count, size);

			load += count;
		}

		template<u32 index>
		__forceinline void set_transform_program(thread* rsx, u32 arg)
		{
			u32& load = method_registers[NV4097_SET_TRANSFORM_PROGRAM_LOAD];

			static const size_t count = 4;
			static const size_t size = count * sizeof(u32);

			memcpy(rsx->transform_program + load, method_registers + NV4097_SET_TRANSFORM_PROGRAM + index * count, size);

			load += count;
		}

		__forceinline void set_begin_end(thread* rsx, u32 arg)
		{
			if (arg)
			{
				rsx->begin();
				return;
			}

			rsx->end();
		}
	}

	namespace nv308a
	{
		template<u32 index>
		__forceinline void color(u32 arg)
		{
			u32 point = method_registers[NV308A_POINT];
			u16 x = point;
			u16 y = point >> 16;

			if (y)
			{
				LOG_ERROR(RSX, "%s: y is not null (0x%x)", __FUNCTION__, y);
			}

			u32 address = get_address(method_registers[NV3062_SET_OFFSET_DESTIN] + (x << 2) + index, method_registers[NV3062_SET_CONTEXT_DMA_IMAGE_DESTIN]);
			vm::write32(address, arg);
		}
	}

	namespace nv3089
	{
		__forceinline void image_in(u32 arg)
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
		__forceinline void buffer_notify(u32 arg)
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

		rsx->last_flip_time = get_system_time();
		rsx->gcm_current_buffer = arg;
		rsx->flip_status = 0;

		if (rsx->flip_handler)
		{
			auto cb = rsx->flip_handler;
			Emu.GetCallbackManager().Async([cb](PPUThread& CPU)
			{
				cb(CPU, 1);
			});
		}

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
		__forceinline static void call_impl_func(thread *rsx, u32 arg)
		{
			impl_func(rsx, arg);
		}

		template<rsx_impl_method_t impl_func>
		__forceinline static void call_impl_func(thread *rsx, u32 arg)
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

		/*
		template<int id, int step, typename T, template<u32> T impl_func = nullptr, int limit = 0>
		static void bind_impl()
		{
		bind<id, impl_func<id>>();

		if (id + step < limit)
		bind_impl<id + step, step, T, impl_func, limit>();
		}

		template<int id, int step, int count, template<u32> rsx_impl_method_t impl_func = nullptr>
		static void bind() { bind_impl<id, step, rsx_impl_method_t, impl_func, id + step * count>(); }

		template<int id, int step, int count, template<u32> rsx_method_t impl_func = nullptr>
		static void bind() { bind_impl<id, step, rsx_method_t, impl_func, id + step * count>(); }
		*/

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

#define bind_2(index, offset, step, func) \
		bind<offset, func<index>>(); \
		bind<offset + step, func<index + 1>>()

#define bind_4(index, offset, step, func) \
		bind_2(index, offset, step, func); \
		bind_2(index + 2, offset + 2*step, step, func)

#define bind_8(index, offset, step, func) \
		bind_4(index, offset, step, func); \
		bind_4(index + 4, offset + 4*step, step, func)

#define bind_16(index, offset, step, func) \
		bind_8(index, offset, step, func); \
		bind_8(index + 8, offset + 8*step, step, func)

#define bind_32(index, offset, step, func) \
		bind_16(index, offset, step, func); \
		bind_16(index + 16, offset + 16*step, step, func)

#define bind_64(index, offset, step, func) \
		bind_32(index, offset, step, func); \
		bind_32(index + 32, offset + 32*step, step, func)

#define bind_128(index, offset, step, func) \
		bind_64(index, offset, step, func); \
		bind_64(index + 64, offset + 64*step, step, func)

#define bind_256(index, offset, step, func) \
		bind_128(index, offset, step, func); \
		bind_128(index + 128, offset + 128*step, step, func)

#define bind_512(index, offset, step, func) \
		bind_256(index, offset, step, func); \
		bind_256(index + 256, offset + 256*step, step, func)

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
			//bind<NV4097_SET_VERTEX_DATA4UB_M, 1, 16, nv4097::set_vertex_data4ub_m>();
			bind_16(0, NV4097_SET_VERTEX_DATA4UB_M, 1, nv4097::set_vertex_data4ub_m);
			bind_16(0, NV4097_SET_VERTEX_DATA1F_M, 1, nv4097::set_vertex_data1f_m);
			bind_16(0, NV4097_SET_VERTEX_DATA2F_M + 1, 2, nv4097::set_vertex_data2f_m);
			bind_16(0, NV4097_SET_VERTEX_DATA3F_M + 2, 3, nv4097::set_vertex_data3f_m);
			bind_16(0, NV4097_SET_VERTEX_DATA4F_M + 3, 4, nv4097::set_vertex_data4f_m);
			bind_16(0, NV4097_SET_VERTEX_DATA2S_M, 1, nv4097::set_vertex_data2s_m);
			bind_16(0, NV4097_SET_VERTEX_DATA4S_M + 1, 2, nv4097::set_vertex_data4s_m);
			bind_8(0, NV4097_SET_TRANSFORM_CONSTANT + 3, 4, nv4097::set_transform_constant);
			bind_128(0, NV4097_SET_TRANSFORM_PROGRAM + 3, 4, nv4097::set_transform_program);

			//NV308A
			bind_512(0, NV308A_COLOR, 1, nv308a::color);

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
			res = (u32)Memory.RSXFBMem.GetStartAddr() + offset;
			break;
		}

		case CELL_GCM_CONTEXT_DMA_MEMORY_HOST_BUFFER:
		case CELL_GCM_LOCATION_MAIN:
		{
			res = (u32)Memory.RSXIOMem.RealAddr(offset); // TODO: Error Check?
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
			throw fmt::format("GetAddress(offset=0x%x, location=0x%x): invalid location", offset, location);
		}
		}

		return res;
	}

	u32 get_vertex_type_size(u32 type)
	{
		switch (type)
		{
		case CELL_GCM_VERTEX_S1:    return 2;
		case CELL_GCM_VERTEX_F:     return 4;
		case CELL_GCM_VERTEX_SF:    return 2;
		case CELL_GCM_VERTEX_UB:    return 1;
		case CELL_GCM_VERTEX_S32K:  return 2;
		case CELL_GCM_VERTEX_CMP:   return 4;
		case CELL_GCM_VERTEX_UB256: return 1;

		default:
			LOG_ERROR(RSX, "RSXVertexData::GetTypeSize: Bad vertex data type (%d)!", type);
			assert(0);
			return 1;
		}
	}

	void vertex_data_t::load(u32 first, u32 count)
	{
		u32 format = method_registers[NV4097_SET_VERTEX_DATA_ARRAY_FORMAT];

		if (!format)
			return;

		u32 offset = method_registers[NV4097_SET_VERTEX_DATA_ARRAY_OFFSET];
		u32 base_offset = method_registers[NV4097_SET_VERTEX_DATA_BASE_OFFSET];
		u32 base_index = method_registers[NV4097_SET_VERTEX_DATA_BASE_INDEX];

		u32 addr = get_address(offset & 0x7fffffff, offset >> 31);
		u8 size = (format >> 4) & 0xf;
		u8 type = format & 0xf;
		u8 stride = (format >> 8) & 0xff;
		u16 frequency = format >> 16;

		u32 type_size = get_vertex_type_size(type);
		u32 full_size = (first + count) * type_size * size;

		if (frequency)
		{
			LOG_ERROR(RSX, "%s: frequency is not null (%d)", __FUNCTION__, frequency);
		}

		data.resize(full_size);

		for (u32 i = first; i < first + count; ++i)
		{
			auto src = vm::get_ptr<const u8>(addr + base_offset + stride * (i + base_index));
			u8* dst = &data[i * type_size * size];

			switch (type_size)
			{
			case 1:
				memcpy(dst, src, size);
				break;

			case 2:
			{
				auto* c_src = (const be_t<u16>*)src;
				u16* c_dst = (u16*)dst;
				for (u32 j = 0; j < size; ++j)
				{
					*c_dst++ = *c_src++;
				}
				break;
			}

			case 4:
			{
				auto* c_src = (const be_t<u32>*)src;
				u32* c_dst = (u32*)dst;
				for (u32 j = 0; j < size; ++j)
				{
					*c_dst++ = *c_src++;
				}
				break;
			}
			}
		}
	}

	void thread::begin()
	{
		//?
	}

	void thread::end()
	{
		index_array.clear();
		fragment_constants.clear();
		transform_constants.clear();

		onreset();
	}

	void thread::Task()
	{
		u8 inc;
		LOG_NOTICE(RSX, "RSX thread started");

		oninit_thread();

		last_flip_time = get_system_time() - 1000000;

		thread_t vblank("VBlank thread", [this]()
		{
			const u64 start_time = get_system_time();

			vblank_count = 0;

			while (!TestDestroy() && !Emu.IsStopped())
			{
				if (get_system_time() - start_time > vblank_count * 1000000 / 60)
				{
					vblank_count++;
					if (vblank_handler)
					{
						auto cb = vblank_handler;
						Emu.GetCallbackManager().Async([cb](PPUThread& CPU)
						{
							cb(CPU, 1);
						});
					}
					continue;
				}

				std::this_thread::sleep_for(std::chrono::milliseconds(1)); // hack
			}
		});

		while (!TestDestroy()) try
		{
			//TODO: async mode
			if (Emu.IsStopped())
			{
				LOG_WARNING(RSX, "RSX thread aborted");
				break;
			}
			std::lock_guard<std::mutex> lock(cs_main);

			inc = 1;

			u32 get = ctrl->get.read_sync();
			u32 put = ctrl->put.read_sync();

			if (put == get || !Emu.IsRunning())
			{
				if (put == get)
				{
					if (flip_status == 0)
						sem_flip.post_and_wait();

					sem_flush.post_and_wait();
				}

				std::this_thread::sleep_for(std::chrono::milliseconds(1)); // hack
				continue;
			}

			const u32 cmd = ReadIO32(get);
			const u32 count = (cmd >> 18) & 0x7ff;
	
			if (cmd & CELL_GCM_METHOD_FLAG_JUMP)
			{
				u32 offs = cmd & 0x1fffffff;
				//LOG_WARNING(RSX, "rsx jump(0x%x) #addr=0x%x, cmd=0x%x, get=0x%x, put=0x%x", offs, m_ioAddress + get, cmd, get, put);
				ctrl->get.exchange(be_t<u32>::make(offs));
				continue;
			}
			if (cmd & CELL_GCM_METHOD_FLAG_CALL)
			{
				m_call_stack.push(get + 4);
				u32 offs = cmd & ~3;
				//LOG_WARNING(RSX, "rsx call(0x%x) #0x%x - 0x%x", offs, cmd, get);
				ctrl->get.exchange(be_t<u32>::make(offs));
				continue;
			}
			if (cmd == CELL_GCM_METHOD_FLAG_RETURN)
			{
				u32 get = m_call_stack.top();
				m_call_stack.pop();
				//LOG_WARNING(RSX, "rsx return(0x%x)", get);
				ctrl->get.exchange(be_t<u32>::make(get));
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

			auto args = vm::ptr<u32>::make((u32)Memory.RSXIOMem.RealAddr(get + 4));

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

				if (auto method = methods[reg])
					method(this, value);

				method_registers[reg] = value;
			}

			ctrl->get.atomic_op([count](be_t<u32>& value)
			{
				value += (count + 1) * 4;
			});
		}
		catch (const std::exception& e)
		{
			LOG_ERROR(RSX, "Exception: %s", e.what());
			Emu.Pause();
		}
		catch (const std::string& e)
		{
			LOG_ERROR(RSX, "Exception: %s", e.c_str());
			Emu.Pause();
		}
		catch (const char* e)
		{
			LOG_ERROR(RSX, "Exception: %s", e);
			Emu.Pause();
		}

		LOG_NOTICE(RSX, "RSX thread ended");

		onexit_thread();
	}

	void thread::init(const u32 ioAddress, const u32 ioSize, const u32 ctrlAddress, const u32 localAddress)
	{
		ctrl = vm::get_ptr<CellGcmControl>(ctrlAddress);
		this->ioAddress = ioAddress;
		this->ioSize = ioSize;
		local_mem_addr = localAddress;

		m_used_gcm_commands.clear();

		oninit();
		ThreadBase::Start();
	}

	u32 thread::ReadIO32(u32 addr)
	{
		u32 value;
	
		if (!Memory.RSXIOMem.Read32(addr, &value))
		{
			throw fmt::Format("%s(addr=0x%x): RSXIO memory not mapped", __FUNCTION__, addr);
		}

		return value;
	}

	void thread::WriteIO32(u32 addr, u32 value)
	{
		if (!Memory.RSXIOMem.Write32(addr, value))
		{
			throw fmt::Format("%s(addr=0x%x): RSXIO memory not mapped", __FUNCTION__, addr);
		}
	}
}
