#pragma once

#include "gcm_enums.h"

extern "C"
{
#include <libavutil/pixfmt.h>
}

namespace rsx
{
	//Holds information about a framebuffer
	struct gcm_framebuffer_info
	{
		u32 address = 0;
		u32 pitch = 0;

		bool is_depth_surface;

		rsx::surface_color_format color_format;
		rsx::surface_depth_format depth_format;

		u16 width;
		u16 height;

		gcm_framebuffer_info()
		{
			address = 0;
			pitch = 0;
		}

		gcm_framebuffer_info(const u32 address_, const u32 pitch_, bool is_depth_, const rsx::surface_color_format fmt_, const rsx::surface_depth_format dfmt_, const u16 w, const u16 h)
			:address(address_), pitch(pitch_), is_depth_surface(is_depth_), color_format(fmt_), depth_format(dfmt_), width(w), height(h)
		{}
	};

	template<typename T>
	void pad_texture(void* input_pixels, void* output_pixels, u16 input_width, u16 input_height, u16 output_width, u16 output_height)
	{
		T *src = static_cast<T*>(input_pixels);
		T *dst = static_cast<T*>(output_pixels);

		for (u16 h = 0; h < input_height; ++h)
		{
			const u32 padded_pos = h * output_width;
			const u32 pos = h * input_width;
			for (u16 w = 0; w < input_width; ++w)
			{
				dst[padded_pos + w] = src[pos + w];
			}
		}
	}

	//
	static inline u32 ceil_log2(u32 value)
	{
		return value <= 1 ? 0 : ::cntlz32((value - 1) << 1, true) ^ 31;
	}

	/*   Note: What the ps3 calls swizzling in this case is actually z-ordering / morton ordering of pixels
	*       - Input can be swizzled or linear, bool flag handles conversion to and from
	*       - It will handle any width and height that are a power of 2, square or non square
	*	 Restriction: It has mixed results if the height or width is not a power of 2
	*/
	template<typename T>
	void convert_linear_swizzle(void* input_pixels, void* output_pixels, u16 width, u16 height, bool input_is_swizzled)
	{
		u32 log2width = ceil_log2(width);
		u32 log2height = ceil_log2(height);

		// Max mask possible for square texture
		u32 x_mask = 0x55555555;
		u32 y_mask = 0xAAAAAAAA;

		// We have to limit the masks to the lower of the two dimensions to allow for non-square textures
		u32 limit_mask = (log2width < log2height) ? log2width : log2height;
		// double the limit mask to account for bits in both x and y
		limit_mask = 1 << (limit_mask << 1);

		//x_mask, bits above limit are 1's for x-carry
		x_mask = (x_mask | ~(limit_mask - 1));
		//y_mask. bits above limit are 0'd, as we use a different method for y-carry over
		y_mask = (y_mask & (limit_mask - 1));

		u32 offs_y = 0;
		u32 offs_x = 0;
		u32 offs_x0 = 0; //total y-carry offset for x
		u32 y_incr = limit_mask;

		if (!input_is_swizzled)
		{
			for (int y = 0; y < height; ++y)
			{
				T *src = static_cast<T*>(input_pixels) + y * width;
				T *dst = static_cast<T*>(output_pixels) + offs_y;
				offs_x = offs_x0;

				for (int x = 0; x < width; ++x)
				{
					dst[offs_x] = src[x];
					offs_x = (offs_x - x_mask) & x_mask;
				}

				offs_y = (offs_y - y_mask) & y_mask;

				if (offs_y == 0)
				{
					offs_x0 += y_incr;
				}
			}
		}
		else
		{
			for (int y = 0; y < height; ++y)
			{
				T *src = static_cast<T*>(input_pixels) + offs_y;
				T *dst = static_cast<T*>(output_pixels) + y * width;
				offs_x = offs_x0;

				for (int x = 0; x < width; ++x)
				{
					dst[x] = src[offs_x];
					offs_x = (offs_x - x_mask) & x_mask;
				}

				offs_y = (offs_y - y_mask) & y_mask;

				if (offs_y == 0)
				{
					offs_x0 += y_incr;
				}
			}
		}
	}

	void scale_image_nearest(void* dst, const void* src, u16 src_width, u16 src_height, u16 dst_pitch, u16 src_pitch, u8 pixel_size, u8 samples, bool swap_bytes = false);

	void convert_scale_image(u8 *dst, AVPixelFormat dst_format, int dst_width, int dst_height, int dst_pitch,
		const u8 *src, AVPixelFormat src_format, int src_width, int src_height, int src_pitch, int src_slice_h, bool bilinear);

	void convert_scale_image(std::unique_ptr<u8[]>& dst, AVPixelFormat dst_format, int dst_width, int dst_height, int dst_pitch,
		const u8 *src, AVPixelFormat src_format, int src_width, int src_height, int src_pitch, int src_slice_h, bool bilinear);

	void clip_image(u8 *dst, const u8 *src, int clip_x, int clip_y, int clip_w, int clip_h, int bpp, int src_pitch, int dst_pitch);
	void clip_image(std::unique_ptr<u8[]>& dst, const u8 *src, int clip_x, int clip_y, int clip_w, int clip_h, int bpp, int src_pitch, int dst_pitch);

	void fill_scale_offset_matrix(void *dest_, bool transpose,
		float offset_x, float offset_y, float offset_z,
		float scale_x, float scale_y, float scale_z);
	void fill_window_matrix(void *dest, bool transpose);
	void fill_viewport_matrix(void *buffer, bool transpose);

	std::array<float, 4> get_constant_blend_colors();

	/**
	 * Clips a rect so that it never falls outside the parent region
	 * attempt_fit: allows resizing of the requested region. If false, failure to fit will result in the child rect being pinned to (0, 0)
	 */
	template <typename T>
	std::tuple<T, T, T, T> clip_region(T parent_width, T parent_height, T clip_x, T clip_y, T clip_width, T clip_height, bool attempt_fit)
	{
		T x = clip_x;
		T y = clip_y;
		T width = clip_width;
		T height = clip_height;

		if ((clip_x + clip_width) > parent_width)
		{
			if (clip_x >= parent_width)
			{
				if (clip_width < parent_width)
					width = clip_width;
				else
					width = parent_width;

				x = (T)0;
			}
			else
			{
				if (attempt_fit)
					width = parent_width - clip_x;
				else
					width = std::min(clip_width, parent_width);
			}
		}

		if ((clip_y + clip_height) > parent_height)
		{
			if (clip_y >= parent_height)
			{
				if (clip_height < parent_height)
					height = clip_height;
				else
					height = parent_height;

				y = (T)0;
			}
			else
			{
				if (attempt_fit)
					height = parent_height - clip_y;
				else
					height = std::min(clip_height, parent_height);
			}
		}

		return std::make_tuple(x, y, width, height);
	}
}
