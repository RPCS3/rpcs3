#pragma once

#include <util/types.hpp>
#include "algorithm.hpp"

extern "C"
{
#include <libavutil/pixfmt.h>
}

namespace rsx
{
	template <typename T>
	void pad_texture(const void* input_pixels, void* output_pixels, u16 input_width, u16 input_height, u16 output_width, u16 /*output_height*/)
	{
		const T *src = static_cast<const T*>(input_pixels);
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

	// Returns interleaved bits of X|Y|Z used as Z-order curve indices
	static inline u32 calculate_z_index(u32 x, u32 y, u32 z, u32 log2_width, u32 log2_height, u32 log2_depth)
	{
		AUDIT(x < (1u << log2_width) && y < (1u << log2_height) && z < (1u << log2_depth));

		// offset = X' | Y' | Z' which are x,y,z bits interleaved
		u32 offset = 0;
		u32 shift_count = 0;
		do
		{
			if (log2_width)
			{
				offset |= (x & 0x1) << shift_count++;
				x >>= 1;
				log2_width--;
			}

			if (log2_height)
			{
				offset |= (y & 0x1) << shift_count++;
				y >>= 1;
				log2_height--;
			}

			if (log2_depth)
			{
				offset |= (z & 0x1) << shift_count++;
				z >>= 1;
				log2_depth--;
			}
		}
		while (x | y | z);

		return offset;
	}

	/*   Note: What the ps3 calls swizzling in this case is actually z-ordering / morton ordering of pixels
	*       - Input can be swizzled or linear, bool flag handles conversion to and from
	*       - It will handle any width and height that are a power of 2, square or non square
	*    Restriction: It has mixed results if the height or width is not a power of 2
	*    Restriction: Only works with 2D surfaces
	*/
	template <typename T, bool input_is_swizzled>
	void convert_linear_swizzle(const void* input_pixels, void* output_pixels, u16 width, u16 height, u32 pitch)
	{
		const u32 log2width = ceil_log2(width);
		const u32 log2height = ceil_log2(height);

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
		const u32 y_incr = limit_mask;

		// NOTE: The swizzled area is always a POT region and we must scan all of it to fill in the linear.
		// It is assumed that there is no padding on the linear side for simplicity - backend upload/download will crop as needed.
		// Remember, in cases of swizzling (and also tiled addressing) it is possible for tiled pixels to fall outside of their linear memory region.
		const u32 pitch_in_blocks = pitch / sizeof(T);
		u32 row_offset = 0;

		if constexpr (!input_is_swizzled)
		{
			for (int y = 0; y < height; ++y, row_offset += pitch_in_blocks)
			{
				auto src = static_cast<const T*>(input_pixels) + row_offset;
				auto dst = static_cast<T*>(output_pixels) + offs_y;
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
			for (int y = 0; y < height; ++y, row_offset += pitch_in_blocks)
			{
				auto src = static_cast<const T*>(input_pixels) + offs_y;
				auto dst = static_cast<T*>(output_pixels) + row_offset;
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

	/**
	 * Write swizzled data to linear memory with support for 3 dimensions
	 * Z ordering is done in all 3 planes independently with a unit being a 2x2 block per-plane
	 * A unit in 3d textures is a group of 2x2x2 texels advancing towards depth in units of 2x2x1 blocks
	 * i.e 32 texels per "unit"
	 */
	template <typename T>
	void convert_linear_swizzle_3d(const void* input_pixels, void* output_pixels, u16 width, u16 height, u16 depth)
	{
		if (depth == 1)
		{
			convert_linear_swizzle<T, true>(input_pixels, output_pixels, width, height, width * sizeof(T));
			return;
		}

		auto src = static_cast<const T*>(input_pixels);
		auto dst = static_cast<T*>(output_pixels);

		const u32 log2_w = ceil_log2(width);
		const u32 log2_h = ceil_log2(height);
		const u32 log2_d = ceil_log2(depth);

		for (u32 z = 0; z < depth; ++z)
		{
			for (u32 y = 0; y < height; ++y)
			{
				for (u32 x = 0; x < width; ++x)
				{
					*dst++ = src[calculate_z_index(x, y, z, log2_w, log2_h, log2_d)];
				}
			}
		}
	}

	void convert_scale_image(u8 *dst, AVPixelFormat dst_format, int dst_width, int dst_height, int dst_pitch,
		const u8 *src, AVPixelFormat src_format, int src_width, int src_height, int src_pitch, int src_slice_h, bool bilinear);

	void clip_image(u8 *dst, const u8 *src, int clip_x, int clip_y, int clip_w, int clip_h, int bpp, int src_pitch, int dst_pitch);
	void clip_image_may_overlap(u8 *dst, const u8 *src, int clip_x, int clip_y, int clip_w, int clip_h, int bpp, int src_pitch, int dst_pitch, u8* buffer);
}
