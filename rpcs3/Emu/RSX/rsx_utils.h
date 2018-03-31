#pragma once

#include "../System.h"
#include "gcm_enums.h"
#include <atomic>

// TODO: replace the code below by #include <optional> when C++17 or newer will be used
#include <optional.hpp>
namespace std
{
	template<class T>
	using optional = experimental::optional<T>;
}

extern "C"
{
#include <libavutil/pixfmt.h>
}

namespace rsx
{
	//Base for resources with reference counting
	struct ref_counted
	{
		u8 deref_count = 0;

		void reset_refs() { deref_count = 0; }
	};

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

	struct avconf
	{
		u8 format = 0; //XRGB
		u8 aspect = 0; //AUTO
		u32 scanline_pitch = 0; //PACKED
		f32 gamma = 1.f; //NO GAMMA CORRECTION
	};

	static const std::pair<std::array<u8, 4>, std::array<u8, 4>> default_remap_vector =
	{
		{ CELL_GCM_TEXTURE_REMAP_FROM_A, CELL_GCM_TEXTURE_REMAP_FROM_R, CELL_GCM_TEXTURE_REMAP_FROM_G, CELL_GCM_TEXTURE_REMAP_FROM_B },
		{ CELL_GCM_TEXTURE_REMAP_REMAP, CELL_GCM_TEXTURE_REMAP_REMAP, CELL_GCM_TEXTURE_REMAP_REMAP, CELL_GCM_TEXTURE_REMAP_REMAP }
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

	static inline u32 next_pow2(u32 x)
	{
		if (x <= 2) return x;

		return static_cast<u32>((1ULL << 32) >> ::cntlz32(x - 1, true));
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

	void scale_image_nearest(void* dst, const void* src, u16 src_width, u16 src_height, u16 dst_pitch, u16 src_pitch, u8 pixel_size, u8 samples_u, u8 samples_v, bool swap_bytes = false);

	void convert_scale_image(u8 *dst, AVPixelFormat dst_format, int dst_width, int dst_height, int dst_pitch,
		const u8 *src, AVPixelFormat src_format, int src_width, int src_height, int src_pitch, int src_slice_h, bool bilinear);

	void convert_scale_image(std::unique_ptr<u8[]>& dst, AVPixelFormat dst_format, int dst_width, int dst_height, int dst_pitch,
		const u8 *src, AVPixelFormat src_format, int src_width, int src_height, int src_pitch, int src_slice_h, bool bilinear);

	void clip_image(u8 *dst, const u8 *src, int clip_x, int clip_y, int clip_w, int clip_h, int bpp, int src_pitch, int dst_pitch);
	void clip_image(std::unique_ptr<u8[]>& dst, const u8 *src, int clip_x, int clip_y, int clip_w, int clip_h, int bpp, int src_pitch, int dst_pitch);

	void convert_le_f32_to_be_d24(void *dst, void *src, u32 row_length_in_texels, u32 num_rows);
	void convert_le_d24x8_to_be_d24x8(void *dst, void *src, u32 row_length_in_texels, u32 num_rows);

	void fill_scale_offset_matrix(void *dest_, bool transpose,
		float offset_x, float offset_y, float offset_z,
		float scale_x, float scale_y, float scale_z);
	void fill_window_matrix(void *dest, bool transpose);
	void fill_viewport_matrix(void *buffer, bool transpose);

	std::array<float, 4> get_constant_blend_colors();

	/**
	 * Shuffle texel layout from xyzw to wzyx
	 * TODO: Variable src/dst and optional se conversion
	 */
	template <typename T>
	void shuffle_texel_data_wzyx(void *data, u16 row_pitch_in_bytes, u16 row_length_in_texels, u16 num_rows)
	{
		char *raw_src = (char*)data;
		T tmp[4];

		for (u16 n = 0; n < num_rows; ++n)
		{
			T* src = (T*)raw_src;
			raw_src += row_pitch_in_bytes;

			for (u16 m = 0; m < row_length_in_texels; ++m)
			{
				tmp[0] = src[3];
				tmp[1] = src[2];
				tmp[2] = src[1];
				tmp[3] = src[0];

				src[0] = tmp[0];
				src[1] = tmp[1];
				src[2] = tmp[2];
				src[3] = tmp[3];

				src += 4;
			}
		}
	}

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

	static inline const f32 get_resolution_scale()
	{
		return g_cfg.video.strict_rendering_mode? 1.f : ((f32)g_cfg.video.resolution_scale_percent / 100.f);
	}

	static inline const int get_resolution_scale_percent()
	{
		return g_cfg.video.strict_rendering_mode ? 100 : g_cfg.video.resolution_scale_percent;
	}

	static inline const u16 apply_resolution_scale(u16 value, bool clamp)
	{
		if (value <= g_cfg.video.min_scalable_dimension)
			return value;
		else if (clamp)
			return (u16)std::max((get_resolution_scale_percent() * value) / 100, 1);
		else
			return (get_resolution_scale_percent() * value) / 100;
	}

	static inline const u16 apply_inverse_resolution_scale(u16 value, bool clamp)
	{
		u16 result = value;

		if (clamp)
			result = (u16)std::max((value * 100) / get_resolution_scale_percent(), 1);
		else
			result = (value * 100) / get_resolution_scale_percent();

		if (result <= g_cfg.video.min_scalable_dimension)
			return value;

		return result;
	}

	template <typename T>
	void split_index_list(T* indices, int index_count, T restart_index, std::vector<std::pair<u32, u32>>& out)
	{
		int last_valid_index = -1;
		int last_start = -1;

		for (int i = 0; i < index_count; ++i)
		{
			if (indices[i] == restart_index)
			{
				if (last_start >= 0)
				{
					out.push_back(std::make_pair(last_start, i - last_start));
					last_start = -1;
				}

				continue;
			}

			if (last_start < 0)
				last_start = i;

			last_valid_index = i;
		}

		if (last_start >= 0)
			out.push_back(std::make_pair(last_start, last_valid_index - last_start + 1));
	}

	// The rsx internally adds the 'data_base_offset' and the 'vert_offset' and masks it 
	// before actually attempting to translate to the internal address. Seen happening heavily in R&C games
	static inline u32 get_vertex_offset_from_base(u32 vert_data_base_offset, u32 vert_base_offset)
	{
		return ((u64)vert_data_base_offset + vert_base_offset) & 0xFFFFFFF;
	}

	// Similar to vertex_offset_base calculation, the rsx internally adds and masks index
	// before using
	static inline u32 get_index_from_base(u32 index, u32 index_base)
	{
		return ((u64)index + index_base) & 0x000FFFFF;
	}

}
