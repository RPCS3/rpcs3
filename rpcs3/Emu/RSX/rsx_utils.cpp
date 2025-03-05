#include "stdafx.h"
#include "rsx_utils.h"
#include "rsx_methods.h"
#include "Emu/Cell/Modules/cellVideoOut.h"

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

	//Convert decoded integer values for CONSTANT_BLEND_FACTOR into f32 array in 0-1 range
	std::array<float, 4> get_constant_blend_colors()
	{
		//TODO: check another color formats (probably all integer formats with > 8-bits wide channels)
		if (rsx::method_registers.surface_color() == rsx::surface_color_format::w16z16y16x16)
		{
			u16 blend_color_r = rsx::method_registers.blend_color_16b_r();
			u16 blend_color_g = rsx::method_registers.blend_color_16b_g();
			u16 blend_color_b = rsx::method_registers.blend_color_16b_b();
			u16 blend_color_a = rsx::method_registers.blend_color_16b_a();

			return { blend_color_r / 65535.f, blend_color_g / 65535.f, blend_color_b / 65535.f, blend_color_a / 65535.f };
		}
		else
		{
			u8 blend_color_r = rsx::method_registers.blend_color_8b_r();
			u8 blend_color_g = rsx::method_registers.blend_color_8b_g();
			u8 blend_color_b = rsx::method_registers.blend_color_8b_b();
			u8 blend_color_a = rsx::method_registers.blend_color_8b_a();

			return { blend_color_r / 255.f, blend_color_g / 255.f, blend_color_b / 255.f, blend_color_a / 255.f };
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
		const auto virtual_output = size2u{ resolution_x, resolution_y };
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
}
