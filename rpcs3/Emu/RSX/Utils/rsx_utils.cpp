#include "stdafx.h"
#include "rsx_utils.h"
#include "Emu/RSX/rsx_methods.h"
#include "Emu/RSX/RSXThread.h"
#include "Emu/RSX/NV47/HW/context_accessors.define.h"
#include "Emu/Cell/Modules/cellVideoOut.h"
#include "Emu/system_config.h"

#ifdef _MSC_VER
#pragma warning(push, 0)
#else
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wall"
#pragma GCC diagnostic ignored "-Wextra"
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif
extern "C"
{
#include "libswscale/swscale.h"
}
#ifdef _MSC_VER
#pragma warning(pop)
#else
#pragma GCC diagnostic pop
#endif

#include "util/sysinfo.hpp"

namespace rsx
{
	atomic_t<u64> g_rsx_shared_tag{ 0 };

	void convert_scale_image(u8 *dst, AVPixelFormat dst_format, int dst_width, int dst_height, int dst_pitch,
		const u8 *src, AVPixelFormat src_format, int src_width, int src_height, int src_pitch, int src_slice_h, bool bilinear)
	{
		std::unique_ptr<SwsContext, void(*)(SwsContext*)> sws(sws_getContext(src_width, src_height, src_format,
			dst_width, dst_height, dst_format, bilinear ? SWS_FAST_BILINEAR : SWS_POINT, nullptr, nullptr, nullptr), sws_freeContext);

		sws_scale(sws.get(), &src, &src_pitch, 0, src_slice_h, &dst, &dst_pitch);
	}

	void clip_image(u8 *dst, const u8 *src, int clip_x, int clip_y, int clip_w, int clip_h, int bpp, int src_pitch, int dst_pitch)
	{
		const u8* pixels_src = src + clip_y * src_pitch + clip_x * bpp;
		u8* pixels_dst = dst;
		const u32 row_length = clip_w * bpp;

		for (int y = 0; y < clip_h; ++y)
		{
			std::memcpy(pixels_dst, pixels_src, row_length);
			pixels_src += src_pitch;
			pixels_dst += dst_pitch;
		}
	}

	void clip_image_may_overlap(u8 *dst, const u8 *src, int clip_x, int clip_y, int clip_w, int clip_h, int bpp, int src_pitch, int dst_pitch, u8 *buffer)
	{
		src += clip_y * src_pitch + clip_x * bpp;

		const u32 buffer_pitch = bpp * clip_w;
		u8* buf = buffer;

		// Read the whole buffer from source
		for (int y = 0; y < clip_h; ++y)
		{
			std::memcpy(buf, src, buffer_pitch);
			src += src_pitch;
			buf += buffer_pitch;
		}

		buf = buffer;

		// Write to destination
		for (int y = 0; y < clip_h; ++y)
		{
			std::memcpy(dst, buf, buffer_pitch);
			dst += dst_pitch;
			buf += buffer_pitch;
		}
	}

	// Fit a aspect-correct rectangle within a frame of wxh dimensions
	template <typename T>
	area_base<T> convert_aspect_ratio_impl(const size2_base<T>& output_dimensions, double aspect)
	{
		const double output_aspect = 1. * output_dimensions.width / output_dimensions.height;
		const double convert_ratio = aspect / output_aspect;

		area_base<T> result;
		if (convert_ratio > 1.)
		{
			const auto height = static_cast<T>(output_dimensions.height / convert_ratio);
			result.y1 = (output_dimensions.height - height) / 2;
			result.y2 = result.y1 + height;
			result.x1 = 0;
			result.x2 = output_dimensions.width;
		}
		else if (convert_ratio < 1.)
		{
			const auto width = static_cast<T>(output_dimensions.width * convert_ratio);
			result.x1 = (output_dimensions.width - width) / 2;
			result.x2 = result.x1 + width;
			result.y1 = 0;
			result.y2 = output_dimensions.height;
		}
		else
		{
			result = { 0, 0, output_dimensions.width, output_dimensions.height };
		}

		return result;
	}

	avconf::avconf() noexcept
	{
		switch (g_cfg.video.aspect_ratio)
		{
		default:
		case video_aspect::_16_9:
			aspect = CELL_VIDEO_OUT_ASPECT_16_9;
			break;
		case video_aspect::_4_3:
			aspect = CELL_VIDEO_OUT_ASPECT_4_3;
			break;
		}
	}

	u32 avconf::get_compatible_gcm_format() const
	{
		switch (format)
		{
		default:
			rsx_log.error("Invalid AV format 0x%x", format);
			[[fallthrough]];
		case CELL_VIDEO_OUT_BUFFER_COLOR_FORMAT_X8R8G8B8:
		case CELL_VIDEO_OUT_BUFFER_COLOR_FORMAT_X8B8G8R8:
			return CELL_GCM_TEXTURE_A8R8G8B8;
		case CELL_VIDEO_OUT_BUFFER_COLOR_FORMAT_R16G16B16X16_FLOAT:
			return CELL_GCM_TEXTURE_W16_Z16_Y16_X16_FLOAT;
		}
	}

	u8 avconf::get_bpp() const
	{
		switch (format)
		{
		default:
			rsx_log.error("Invalid AV format 0x%x", format);
			[[fallthrough]];
		case CELL_VIDEO_OUT_BUFFER_COLOR_FORMAT_X8R8G8B8:
		case CELL_VIDEO_OUT_BUFFER_COLOR_FORMAT_X8B8G8R8:
			return 4;
		case CELL_VIDEO_OUT_BUFFER_COLOR_FORMAT_R16G16B16X16_FLOAT:
			return 8;
		}
	}

	double avconf::get_aspect_ratio() const
	{
		switch (aspect)
		{
		case CELL_VIDEO_OUT_ASPECT_16_9: return 16. / 9.;
		case CELL_VIDEO_OUT_ASPECT_4_3: return 4. / 3.;
		default: fmt::throw_exception("Invalid aspect ratio %d", aspect);
		}
	}

	size2u avconf::video_frame_size() const
	{
		if (state && stereo_enabled)
		{
			return size2u{ resolution_x, (resolution_y - 30) / 2 };
		}

		return size2u{ resolution_x, resolution_y };
	}

	size2u avconf::aspect_convert_dimensions(const size2u& image_dimensions) const
	{
		if (image_dimensions.width == 0 || image_dimensions.height == 0)
		{
			rsx_log.trace("Empty region passed to aspect-correct conversion routine [size]. This should never happen.");
			return {};
		}

		// Unconstrained aspect ratio conversion
		return size2u{ static_cast<u32>(image_dimensions.height * get_aspect_ratio()), image_dimensions.height };
	}

	areau avconf::aspect_convert_region(const size2u& image_dimensions, const size2u& output_dimensions) const
	{
		if (const auto test = image_dimensions * output_dimensions;
			test.width == 0 || test.height == 0)
		{
			rsx_log.trace("Empty region passed to aspect-correct conversion routine [region]. This should never happen.");
			return {};
		}

		// Fit the input image into the virtual display 'window'
		const auto source_aspect = 1. * image_dimensions.width / image_dimensions.height;
		const auto virtual_output = video_frame_size();
		const auto area1 = convert_aspect_ratio_impl(virtual_output, source_aspect);

		// Fit the virtual display into the physical display
		const auto area2 = convert_aspect_ratio_impl(output_dimensions, get_aspect_ratio());

		// Merge the two regions. Since aspect ratio was conserved between both transforms, a simple scale can be used
		const double stretch_x = 1. * area2.width() / virtual_output.width;
		const double stretch_y = 1. * area2.height() / virtual_output.height;

		return static_cast<areau>(static_cast<aread>(area1) * size2d { stretch_x, stretch_y }) + size2u{ area2.x1, area2.y1 };
	}

#ifdef TEXTURE_CACHE_DEBUG
	tex_cache_checker_t tex_cache_checker = {};
#endif

	rsx_state& get_method_registers()
	{
		return *REGS(ensure(ensure(get_current_renderer())->get_context()));
	}

	u32 get_address(const context* ctx, u32 offset, u32 location, u32 size_to_check, std::source_location src_loc)
	{
		const auto render = ctx ? ctx->rsxthr : get_current_renderer();
		std::string_view msg;

		switch (location)
		{
		case CELL_GCM_CONTEXT_DMA_MEMORY_FRAME_BUFFER:
		case CELL_GCM_LOCATION_LOCAL:
		{
			if (offset < render->local_mem_size && render->local_mem_size - offset >= size_to_check)
			{
				return rsx::constants::local_mem_base + offset;
			}

			msg = "Local RSX offset out of range!"sv;
			break;
		}

		case CELL_GCM_CONTEXT_DMA_MEMORY_HOST_BUFFER:
		case CELL_GCM_LOCATION_MAIN:
		{
			if (const u32 ea = render->iomap_table.get_addr(offset); ea != umax)
			{
				if (const u32 main_mem_size = ensure(ctx)->fw.main_mem_size;
					size_to_check <= 1 || (offset < main_mem_size && main_mem_size - offset >= size_to_check))
				{
					bool ok = true;

					for (u32 offs_index = 0x100000;
						 offs_index < size_to_check + (offset & 0xfffff);
						 offs_index += 0x100000)
					{
						// This check does not check continuity but rather that it's mapped at all
						if (render->iomap_table.get_addr(offset + offs_index) == umax)
						{
							ok = false;
						}
					}

					if (ok)
					{
						if (!size_to_check || vm::check_addr(ea, 0, size_to_check))
						{
							return ea;
						}
					}
				}
			}

			msg = "RSXIO memory not mapped!"sv;
			break;
		}

		case CELL_GCM_CONTEXT_DMA_REPORT_LOCATION_LOCAL:
		{
			if (offset < sizeof(RsxReports::report) /*&& (offset % 0x10) == 0*/)
			{
				return ensure(ctx)->fw.label_addr + ::offset32(&RsxReports::report) + offset;
			}

			msg = "Local RSX REPORT offset out of range!"sv;
			break;
		}

		case CELL_GCM_CONTEXT_DMA_REPORT_LOCATION_MAIN:
		{
			if (const u32 ea = offset < 0x1000000 ? render->iomap_table.get_addr(0x0e000000 + offset) : -1; ea != umax)
			{
				if (!size_to_check || vm::check_addr(ea, 0, size_to_check))
				{
					return ea;
				}
			}

			msg = "RSXIO REPORT memory not mapped!"sv;
			break;
		}

		// They are handled elsewhere for targeted methods, so it's unexpected for them to be passed here
		case CELL_GCM_CONTEXT_DMA_TO_MEMORY_GET_NOTIFY0:
		case CELL_GCM_CONTEXT_DMA_TO_MEMORY_GET_NOTIFY1:
		case CELL_GCM_CONTEXT_DMA_TO_MEMORY_GET_NOTIFY2:
		case CELL_GCM_CONTEXT_DMA_TO_MEMORY_GET_NOTIFY3:
		case CELL_GCM_CONTEXT_DMA_TO_MEMORY_GET_NOTIFY4:
		case CELL_GCM_CONTEXT_DMA_TO_MEMORY_GET_NOTIFY5:
		case CELL_GCM_CONTEXT_DMA_TO_MEMORY_GET_NOTIFY6:
		case CELL_GCM_CONTEXT_DMA_TO_MEMORY_GET_NOTIFY7:
			msg = "CELL_GCM_CONTEXT_DMA_TO_MEMORY_GET_NOTIFYx"sv; break;

		case CELL_GCM_CONTEXT_DMA_NOTIFY_MAIN_0:
		case CELL_GCM_CONTEXT_DMA_NOTIFY_MAIN_1:
		case CELL_GCM_CONTEXT_DMA_NOTIFY_MAIN_2:
		case CELL_GCM_CONTEXT_DMA_NOTIFY_MAIN_3:
		case CELL_GCM_CONTEXT_DMA_NOTIFY_MAIN_4:
		case CELL_GCM_CONTEXT_DMA_NOTIFY_MAIN_5:
		case CELL_GCM_CONTEXT_DMA_NOTIFY_MAIN_6:
		case CELL_GCM_CONTEXT_DMA_NOTIFY_MAIN_7:
			msg = "CELL_GCM_CONTEXT_DMA_NOTIFY_MAIN_x"sv; break;

		case CELL_GCM_CONTEXT_DMA_SEMAPHORE_RW:
		case CELL_GCM_CONTEXT_DMA_SEMAPHORE_R:
		{
			if (offset < sizeof(RsxReports::semaphore) /*&& (offset % 0x10) == 0*/)
			{
				return ensure(ctx)->fw.label_addr + offset;
			}

			msg = "DMA SEMAPHORE offset out of range!"sv;
			break;
		}

		case CELL_GCM_CONTEXT_DMA_DEVICE_RW:
		case CELL_GCM_CONTEXT_DMA_DEVICE_R:
		{
			if (offset < 0x100000 /*&& (offset % 0x10) == 0*/)
			{
				return render->device_addr + offset;
			}

			// TODO: What happens here? It could wrap around or access other segments of rsx internal memory etc
			// Or can simply throw access violation error
			msg = "DMA DEVICE offset out of range!"sv;
			break;
		}

		default:
		{
			msg = "Invalid location!"sv;
			break;
		}
		}

		if (size_to_check)
		{
			// Allow failure if specified size
			// This is to allow accurate recovery for failures
			rsx_log.warning("rsx::get_address(offset=0x%x, location=0x%x, size=0x%x): %s%s", offset, location, size_to_check, msg, src_loc);
			return 0;
		}

		fmt::throw_exception("rsx::get_address(offset=0x%x, location=0x%x): %s%s", offset, location, msg, src_loc);
	}

	void yield_rsx_ctx_thread() noexcept
	{
		if (auto rsxthr = get_current_renderer())
		{
			if (g_cfg.core.allow_rsx_cpu_preempt)
			{
				rsxthr->state += cpu_flag::yield;
			}
		}
	}

	void set_native_ui_flip()
	{
		if (auto rsxthr = rsx::get_current_renderer())
		{
			rsxthr->display_ctrl().request_native_ui_flip();
		}
	}

	u32 get_vertex_type_size_on_host(vertex_base_type type, u32 size)
	{
		switch (type)
		{
		case vertex_base_type::s1:
		case vertex_base_type::s32k:
			switch (size)
			{
			case 1:
			case 2:
			case 4:
				return sizeof(u16) * size;
			case 3:
				return sizeof(u16) * 4;
			default:
				break;
			}
			fmt::throw_exception("Wrong vector size");
		case vertex_base_type::f: return sizeof(f32) * size;
		case vertex_base_type::sf:
			switch (size)
			{
			case 1:
			case 2:
			case 4:
				return sizeof(f16) * size;
			case 3:
				return sizeof(f16) * 4;
			default:
				break;
			}
			fmt::throw_exception("Wrong vector size");
		case vertex_base_type::ub:
			switch (size)
			{
			case 1:
			case 2:
			case 4:
				return sizeof(u8) * size;
			case 3:
				return sizeof(u8) * 4;
			default:
				break;
			}
			fmt::throw_exception("Wrong vector size");
		case vertex_base_type::cmp: return 4;
		case vertex_base_type::ub256: ensure(size == 4); return sizeof(u8) * 4;
		default:
			break;
		}

		fmt::throw_exception("Bad vertex data type (%d)!", static_cast<u8>(type));
	}
}
