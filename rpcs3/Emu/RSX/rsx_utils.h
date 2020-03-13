#pragma once

#include "../system_config.h"
#include "Utilities/address_range.h"
#include "Utilities/geometry.h"
#include "Utilities/asm.h"
#include "gcm_enums.h"

#include <memory>
#include <bitset>
#include <chrono>

extern "C"
{
#include <libavutil/pixfmt.h>
}

namespace rsx
{
	// Import address_range utilities
	using utils::address_range;
	using utils::address_range_vector;
	using utils::page_for;
	using utils::page_start;
	using utils::page_end;
	using utils::next_page;

	using flags64_t = uint64_t;
	using flags32_t = uint32_t;
	using flags16_t = uint16_t;
	using flags8_t = uint8_t;

	extern atomic_t<u64> g_rsx_shared_tag;

	//Base for resources with reference counting
	class ref_counted
	{
		atomic_t<s32> ref_count{ 0 }; // References held
		atomic_t<u8> idle_time{ 0 };  // Number of times the resource has been tagged idle

	public:
		void add_ref()
		{
			ref_count++;
			idle_time = 0;
		}

		void release()
		{
			ref_count--;
		}

		bool has_refs()
		{
			return (ref_count > 0);
		}

		// Returns number of times the resource has been checked without being used in-between checks
		u8 unused_check_count()
		{
			if (ref_count)
			{
				return 0;
			}

			return idle_time++;
		}
	};

	namespace limits
	{
		enum
		{
			fragment_textures_count = 16,
			vertex_textures_count = 4,
			vertex_count = 16,
			fragment_count = 32,
			tiles_count = 15,
			zculls_count = 8,
			color_buffers_count = 4
		};
	}

	namespace constants
	{
		static std::array<const char*, 16> fragment_texture_names =
		{
			"tex0", "tex1", "tex2", "tex3", "tex4", "tex5", "tex6", "tex7",
			"tex8", "tex9", "tex10", "tex11", "tex12", "tex13", "tex14", "tex15",
		};

		static std::array<const char*, 4> vertex_texture_names =
		{
			"vtex0", "vtex1", "vtex2", "vtex3",
		};

		// Local RSX memory base (known as constant)
		static constexpr u32 local_mem_base = 0xC0000000;
	}

	/**
	* Holds information about a framebuffer
	*/
	struct gcm_framebuffer_info
	{
		u32 address = 0;
		u32 pitch = 0;

		rsx::surface_color_format color_format;
		rsx::surface_depth_format depth_format;
		bool depth_buffer_float;

		u16 width = 0;
		u16 height = 0;
		u8  bpp = 0;
		u8  samples = 0;

		address_range range{};

		gcm_framebuffer_info() = default;

		void calculate_memory_range(u32 aa_factor_u, u32 aa_factor_v)
		{
			// Account for the last line of the block not reaching the end
			const u32 block_size = pitch * (height - 1) * aa_factor_v;
			const u32 line_size = width * aa_factor_u * bpp;
			range = address_range::start_length(address, block_size + line_size);
		}

		address_range get_memory_range(const u32* aa_factors)
		{
			calculate_memory_range(aa_factors[0], aa_factors[1]);
			return range;
		}

		address_range get_memory_range() const
		{
			verify(HERE), range.start == address;
			return range;
		}
	};

	struct avconf
	{
		bool _3d  = false;         // Stereo 3D off
		u8 format = 0;             // XRGB
		u8 aspect = 0;             // AUTO
		u8 resolution_id = 2;      // 720p
		u32 scanline_pitch = 0;    // PACKED
		atomic_t<f32> gamma = 1.f; // NO GAMMA CORRECTION
		u32 resolution_x = 1280;   // X RES
		u32 resolution_y = 720;    // Y RES
		atomic_t<u32> state = 0;   // 1 after cellVideoOutConfigure was called

		u32 get_compatible_gcm_format()
		{
			switch (format)
			{
			default:
				rsx_log.error("Invalid AV format 0x%x", format);
			case 0: // CELL_VIDEO_OUT_BUFFER_COLOR_FORMAT_X8R8G8B8:
			case 1: // CELL_VIDEO_OUT_BUFFER_COLOR_FORMAT_X8B8G8R8:
				return CELL_GCM_TEXTURE_A8R8G8B8;
			case 2: // CELL_VIDEO_OUT_BUFFER_COLOR_FORMAT_R16G16B16X16_FLOAT:
				return CELL_GCM_TEXTURE_W16_Z16_Y16_X16_FLOAT;
			}
		}

		u8 get_bpp()
		{
			switch (format)
			{
			default:
				rsx_log.error("Invalid AV format 0x%x", format);
			case 0: // CELL_VIDEO_OUT_BUFFER_COLOR_FORMAT_X8R8G8B8:
			case 1: // CELL_VIDEO_OUT_BUFFER_COLOR_FORMAT_X8B8G8R8:
				return 4;
			case 2: // CELL_VIDEO_OUT_BUFFER_COLOR_FORMAT_R16G16B16X16_FLOAT:
				return 8;
			}
		}
	};

	struct blit_src_info
	{
		blit_engine::transfer_source_format format;
		blit_engine::transfer_origin origin;
		u16 offset_x;
		u16 offset_y;
		u16 width;
		u16 height;
		u16 pitch;
		u32 rsx_address;
		void *pixels;
	};

	struct blit_dst_info
	{
		blit_engine::transfer_destination_format format;
		u16 offset_x;
		u16 offset_y;
		u16 width;
		u16 height;
		u16 pitch;
		u16 clip_x;
		u16 clip_y;
		u16 clip_width;
		u16 clip_height;
		f32 scale_x;
		f32 scale_y;
		u32  rsx_address;
		void *pixels;
		bool swizzled;
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
	static inline u32 floor_log2(u32 value)
	{
		return value <= 1 ? 0 : utils::cntlz32(value, true) ^ 31;
	}

	static inline u32 ceil_log2(u32 value)
	{
		return value <= 1 ? 0 : utils::cntlz32((value - 1) << 1, true) ^ 31;
	}

	static inline u32 next_pow2(u32 x)
	{
		if (x <= 2) return x;

		return static_cast<u32>((1ULL << 32) >> utils::cntlz32(x - 1, true));
	}

	static inline bool fcmp(float a, float b, float epsilon = 0.000001f)
	{
		return fabsf(a - b) < epsilon;
	}

	// Returns an ever-increasing tag value
	static inline u64 get_shared_tag()
	{
		return g_rsx_shared_tag++;
	}

	static inline u32 get_location(u32 addr)
	{
		return (addr >= rsx::constants::local_mem_base) ?
			CELL_GCM_LOCATION_LOCAL :
			CELL_GCM_LOCATION_MAIN;
	}

	// General purpose alignment without power-of-2 constraint
	template <typename T, typename U>
	static inline T align2(T value, U alignment)
	{
		return ((value + alignment - 1) / alignment) * alignment;
	}

	// Copy memory in inverse direction from source
	// Used to scale negatively x axis while transfering image data
	template <typename Ts = u8, typename Td = Ts>
	static void memcpy_r(void* dst, void* src, std::size_t size)
	{
		for (u32 i = 0; i < size; i++)
		{
			*(static_cast<Td*>(dst) + i) = *(static_cast<Ts*>(src) - i);
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

		u32 adv = pitch / sizeof(T);

		if constexpr (!input_is_swizzled)
		{
			for (int y = 0; y < height; ++y)
			{
				auto src = static_cast<const T*>(input_pixels) + y * adv;
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
			for (int y = 0; y < height; ++y)
			{
				auto src = static_cast<const T*>(input_pixels) + offs_y;
				auto dst = static_cast<T*>(output_pixels) + y * adv;
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

	void scale_image_nearest(void* dst, const void* src, u16 src_width, u16 src_height, u16 dst_pitch, u16 src_pitch, u8 element_size, u8 samples_u, u8 samples_v, bool swap_bytes = false);

	void convert_scale_image(u8 *dst, AVPixelFormat dst_format, int dst_width, int dst_height, int dst_pitch,
		const u8 *src, AVPixelFormat src_format, int src_width, int src_height, int src_pitch, int src_slice_h, bool bilinear);

	void clip_image(u8 *dst, const u8 *src, int clip_x, int clip_y, int clip_w, int clip_h, int bpp, int src_pitch, int dst_pitch);
	void clip_image_may_overlap(u8 *dst, const u8 *src, int clip_x, int clip_y, int clip_w, int clip_h, int bpp, int src_pitch, int dst_pitch, u8* buffer);

	void convert_le_f32_to_be_d24(void *dst, void *src, u32 row_length_in_texels, u32 num_rows);
	void convert_le_d24x8_to_be_d24x8(void *dst, void *src, u32 row_length_in_texels, u32 num_rows);
	void convert_le_d24x8_to_le_f32(void *dst, void *src, u32 row_length_in_texels, u32 num_rows);

	std::array<float, 4> get_constant_blend_colors();

	/**
	 * Shuffle texel layout from xyzw to wzyx
	 * TODO: Variable src/dst and optional se conversion
	 */
	template <typename T>
	void shuffle_texel_data_wzyx(void* data, u16 row_pitch_in_bytes, u16 row_length_in_texels, u16 num_rows)
	{
		char* raw_src = static_cast<char*>(data);
		T tmp[4];

		for (u16 n = 0; n < num_rows; ++n)
		{
			T* src = reinterpret_cast<T*>(raw_src);
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

				x = static_cast<T>(0);
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

				y = static_cast<T>(0);
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

	/**
	 * Extracts from 'parent' a region that fits in 'child'
	 */
	static inline std::tuple<position2u, position2u, size2u> intersect_region(
		u32 parent_address, u16 parent_w, u16 parent_h, u16 parent_bpp,
		u32 child_address, u16 child_w, u16 child_h, u32 child_bpp,
		u32 pitch)
	{
		if (child_address < parent_address)
		{
			const auto offset = parent_address - child_address;
			const auto src_x = 0u;
			const auto src_y = 0u;
			const auto dst_y = (offset / pitch);
			const auto dst_x = (offset % pitch) / child_bpp;
			const auto w = std::min<u32>(parent_w, child_w - dst_x);
			const auto h = std::min<u32>(parent_h, child_h - dst_y);

			return std::make_tuple<position2u, position2u, size2u>({ src_x, src_y }, { dst_x, dst_y }, { w, h });
		}
		else
		{
			const auto offset = child_address - parent_address;
			const auto src_y = (offset / pitch);
			const auto src_x = (offset % pitch) / parent_bpp;
			const auto dst_x = 0u;
			const auto dst_y = 0u;
			const auto w = std::min<u32>(child_w, parent_w - src_x);
			const auto h = std::min<u32>(child_h, parent_h - src_y);

			return std::make_tuple<position2u, position2u, size2u>({ src_x, src_y }, { dst_x, dst_y }, { w, h });
		}
	}

	static inline const f32 get_resolution_scale()
	{
		return g_cfg.video.strict_rendering_mode? 1.f : (g_cfg.video.resolution_scale_percent / 100.f);
	}

	static inline const int get_resolution_scale_percent()
	{
		return g_cfg.video.strict_rendering_mode ? 100 : g_cfg.video.resolution_scale_percent;
	}

	static inline const u16 apply_resolution_scale(u16 value, bool clamp, u16 ref = 0)
	{
		if (ref == 0)
			ref = value;

		if (ref <= g_cfg.video.min_scalable_dimension)
			return value;

		else if (clamp)
			return static_cast<u16>(std::max((get_resolution_scale_percent() * value) / 100, 1));
		else
			return static_cast<u16>((get_resolution_scale_percent() * value) / 100);
	}

	static inline const u16 apply_inverse_resolution_scale(u16 value, bool clamp)
	{
		u16 result = value;

		if (clamp)
			result = static_cast<u16>(std::max((value * 100) / get_resolution_scale_percent(), 1));
		else
			result = static_cast<u16>((value * 100) / get_resolution_scale_percent());

		if (result <= g_cfg.video.min_scalable_dimension)
			return value;

		return result;
	}

	/**
	 * Calculates the regions used for memory transfer between rendertargets on succession events
	 * Returns <src_w, src_h, dst_w, dst_h>
	 */
	template <typename SurfaceType>
	std::tuple<u16, u16, u16, u16> get_transferable_region(const SurfaceType* surface)
	{
		auto src = static_cast<const SurfaceType*>(surface->old_contents[0].source);
		auto area1 = src->get_normalized_memory_area();
		auto area2 = surface->get_normalized_memory_area();

		auto w = std::min(area1.x2, area2.x2);
		auto h = std::min(area1.y2, area2.y2);

		const auto src_scale_x = src->get_bpp() * src->samples_x;
		const auto src_scale_y = src->samples_y;
		const auto dst_scale_x = surface->get_bpp() * surface->samples_x;
		const auto dst_scale_y = surface->samples_y;

		const u16 src_w = u16(w / src_scale_x);
		const u16 src_h = u16(h / src_scale_y);
		const u16 dst_w = u16(w / dst_scale_x);
		const u16 dst_h = u16(h / dst_scale_y);

		return std::make_tuple(src_w, src_h, dst_w, dst_h);
	}

	template <typename SurfaceType>
	inline bool pitch_compatible(const SurfaceType* a, const SurfaceType* b)
	{
		if (a->get_surface_height() == 1 || b->get_surface_height() == 1)
			return true;

		return (a->get_rsx_pitch() == b->get_rsx_pitch());
	}

	template <bool __is_surface = true, typename SurfaceType>
	inline bool pitch_compatible(const SurfaceType* surface, u16 pitch_required, u16 height_required)
	{
		if constexpr (__is_surface)
		{
			if (height_required == 1 || surface->get_surface_height() == 1)
				return true;
		}
		else
		{
			if (height_required == 1 || surface->get_height() == 1)
				return true;
		}

		return (surface->get_rsx_pitch() == pitch_required);
	}

	/**
	 * Remove restart index and emulate using degenerate triangles
	 * Can be used as a workaround when restart_index doesnt work too well
	 * dst should be able to hold at least 2xcount entries
	 */
	template <typename T>
	u32 remove_restart_index(T* dst, T* src, int count, T restart_index)
	{
		// Converts a stream e.g [1, 2, 3, -1, 4, 5, 6] to a stream with degenerate splits
		// Output is e.g [1, 2, 3, 3, 3, 4, 4, 5, 6] (5 bogus triangles)
		T last_index, index;
		u32 dst_index = 0;
		for (int n = 0; n < count;)
		{
			index = src[n];
			if (index == restart_index)
			{
				for (; n < count; ++n)
				{
					if (src[n] != restart_index)
						break;
				}

				if (n == count)
					return dst_index;

				dst[dst_index++] = last_index; //Duplicate last

				if ((dst_index & 1) == 0)
					//Duplicate last again to fix face winding
					dst[dst_index++] = last_index;

				last_index = src[n];
				dst[dst_index++] = last_index; //Duplicate next
			}
			else
			{
				dst[dst_index++] = index;
				last_index = index;
				++n;
			}
		}

		return dst_index;
	}

	// The rsx internally adds the 'data_base_offset' and the 'vert_offset' and masks it
	// before actually attempting to translate to the internal address. Seen happening heavily in R&C games
	static inline u32 get_vertex_offset_from_base(u32 vert_data_base_offset, u32 vert_base_offset)
	{
		return (vert_data_base_offset + vert_base_offset) & 0xFFFFFFF;
	}

	// Similar to vertex_offset_base calculation, the rsx internally adds and masks index
	// before using
	static inline u32 get_index_from_base(u32 index, u32 index_base)
	{
		return (index + index_base) & 0x000FFFFF;
	}

	// Convert color write mask for G8B8 to R8G8
	static inline u32 get_g8b8_r8g8_colormask(u32 mask)
	{
		u32 result = 0;
		if (mask & 0x20) result |= 0x20;
		if (mask & 0x40) result |= 0x10;

		return result;
	}

	static inline void get_g8b8_r8g8_colormask(bool &red, bool &green, bool &blue, bool &alpha)
	{
		red = blue;
		green = green;
		blue = false;
		alpha = false;
	}

	static inline color4f decode_border_color(u32 colorref)
	{
		color4f result;
		result.b = (colorref & 0xFF) / 255.f;
		result.g = ((colorref >> 8) & 0xFF) / 255.f;
		result.r = ((colorref >> 16) & 0xFF) / 255.f;
		result.a = ((colorref >> 24) & 0xFF) / 255.f;
		return result;
	}

	template <uint integer, uint frac, bool sign = true, typename To = f32>
	static inline To decode_fxp(u32 bits)
	{
		static_assert(u64{sign} + integer + frac <= 32, "Invalid decode_fxp range");

		// Classic fixed point, see PGRAPH section of nouveau docs for TEX_FILTER (lod_bias) and TEX_CONTROL (min_lod, max_lod)
		// Technically min/max lod are fixed 4.8 but a 5.8 decoder should work just as well since sign bit is 0

		if constexpr (sign) if (bits & (1 << (integer + frac)))
		{
			bits = (0 - bits) & (~0u >> (31 - (integer + frac)));
			return bits / (-To(1u << frac));
		}

		return bits / To(1u << frac);
	}

	template <int N>
	void unpack_bitset(const std::bitset<N>& block, u64* values)
	{
		constexpr int count = N / 64;
		for (int n = 0; n < count; ++n)
		{
			int i = (n << 6);
			values[n] = 0;

			for (int bit = 0; bit < 64; ++bit, ++i)
			{
				if (block[i])
				{
					values[n] |= (1ull << bit);
				}
			}
		}
	}

	template <int N>
	void pack_bitset(std::bitset<N>& block, u64* values)
	{
		constexpr int count = N / 64;
		for (int n = (count - 1); n >= 0; --n)
		{
			if ((n + 1) < count)
			{
				block <<= 64;
			}

			if (values[n])
			{
				block |= values[n];
			}
		}
	}

	template <typename T, typename bitmask_type = u32>
	class atomic_bitmask_t
	{
	private:
		atomic_t<bitmask_type> m_data;

	public:
		atomic_bitmask_t() { m_data.store(0); }
		~atomic_bitmask_t() = default;

		T load() const
		{
			return static_cast<T>(m_data.load());
		}

		void store(T value)
		{
			m_data.store(static_cast<bitmask_type>(value));
		}

		bool operator & (T mask) const
		{
			return ((m_data.load() & static_cast<bitmask_type>(mask)) != 0);
		}

		T operator | (T mask) const
		{
			return static_cast<T>(m_data.load() | static_cast<bitmask_type>(mask));
		}

		void operator &= (T mask)
		{
			m_data.fetch_and(static_cast<bitmask_type>(mask));
		}

		void operator |= (T mask)
		{
			m_data.fetch_or(static_cast<bitmask_type>(mask));
		}

		bool test_and_set(T mask)
		{
			const auto old = m_data.fetch_or(static_cast<bitmask_type>(mask));
			return (old & static_cast<bitmask_type>(mask)) != 0;
		}

		auto clear(T mask)
		{
			bitmask_type clear_mask = ~(static_cast<bitmask_type>(mask));
			return m_data.and_fetch(clear_mask);
		}

		void clear()
		{
			m_data.store(0);
		}
	};

	template <typename Ty>
	struct simple_array
	{
	public:
		using iterator = Ty * ;
		using const_iterator = Ty * const;

	private:
		u32 _capacity = 0;
		u32 _size = 0;
		Ty* _data = nullptr;

		inline u64 offset(const_iterator pos)
		{
			return (_data) ? u64(pos - _data) : 0ull;
		}

	public:
		simple_array() = default;

		simple_array(u32 initial_size, const Ty val = {})
		{
			reserve(initial_size);
			_size = initial_size;

			for (int n = 0; n < initial_size; ++n)
			{
				_data[n] = val;
			}
		}

		simple_array(const std::initializer_list<Ty>& args)
		{
			reserve(::size32(args));

			for (const auto& arg : args)
			{
				push_back(arg);
			}
		}

		simple_array(const simple_array<Ty>& other)
		{
			_capacity = other._capacity;
			_size = other._size;

			const auto size_bytes = sizeof(Ty) * _capacity;
			_data = static_cast<Ty*>(malloc(size_bytes));
			std::memcpy(_data, other._data, size_bytes);
		}

		simple_array(simple_array<Ty>&& other) noexcept
		{
			swap(other);
		}

		~simple_array()
		{
			if (_data)
			{
				free(_data);
				_data = nullptr;
				_size = _capacity = 0;
			}
		}

		void swap(simple_array<Ty>& other) noexcept
		{
			std::swap(_capacity, other._capacity);
			std::swap(_size, other._size);
			std::swap(_data, other._data);
		}

		void reserve(u32 size)
		{
			if (_capacity >= size)
				return;

			verify("realloc() failed!" HERE), _data = static_cast<Ty*>(std::realloc(_data, sizeof(Ty) * size));
			_capacity = size;
		}

		void resize(u32 size)
		{
			reserve(size);
			_size = size;
		}

		void push_back(const Ty& val)
		{
			if (_size >= _capacity)
			{
				reserve(_capacity + 16);
			}

			_data[_size++] = val;
		}

		void push_back(Ty&& val)
		{
			if (_size >= _capacity)
			{
				reserve(_capacity + 16);
			}

			_data[_size++] = val;
		}

		iterator insert(iterator pos, const Ty& val)
		{
			verify(HERE), pos >= _data;
			const auto _loc = offset(pos);

			if (_size >= _capacity)
			{
				reserve(_capacity + 16);
				pos = _data + _loc;
			}

			if (_loc >= _size)
			{
				_data[_size++] = val;
				return pos;
			}

			verify(HERE), _loc < _size;

			const auto remaining = (_size - _loc);
			memmove(pos + 1, pos, remaining * sizeof(Ty));

			*pos = val;
			_size++;

			return pos;
		}

		iterator insert(iterator pos, Ty&& val)
		{
			verify(HERE), pos >= _data;
			const auto _loc = offset(pos);

			if (_size >= _capacity)
			{
				reserve(_capacity + 16);
				pos = _data + _loc;
			}

			if (_loc >= _size)
			{
				_data[_size++] = val;
				return pos;
			}

			verify(HERE), _loc < _size;

			const u32 remaining = (_size - _loc);
			memmove(pos + 1, pos, remaining * sizeof(Ty));

			*pos = val;
			_size++;

			return pos;
		}

		void clear()
		{
			_size = 0;
		}

		bool empty() const
		{
			return _size == 0;
		}

		u32 size() const
		{
			return _size;
		}

		u32 capacity() const
		{
			return _capacity;
		}

		Ty& operator[] (u32 index)
		{
			return _data[index];
		}

		const Ty& operator[] (u32 index) const
		{
			return _data[index];
		}

		Ty* data()
		{
			return _data;
		}

		const Ty* data() const
		{
			return _data;
		}

		Ty& back()
		{
			return _data[_size - 1];
		}

		const Ty& back() const
		{
			return _data[_size - 1];
		}

		Ty& front()
		{
			return _data[0];
		}

		const Ty& front() const
		{
			return _data[0];
		}

		iterator begin()
		{
			return _data;
		}

		iterator end()
		{
			return _data ? _data + _size : nullptr;
		}

		const_iterator begin() const
		{
			return _data;
		}

		const_iterator end() const
		{
			return _data ? _data + _size : nullptr;
		}
	};

	struct profiling_timer
	{
		bool enabled = false;
		std::chrono::time_point<steady_clock> last;

		profiling_timer() = default;

		void start()
		{
			if (enabled) [[unlikely]]
			{
				last = steady_clock::now();
			}
		}

		s64 duration()
		{
			if (!enabled) [[likely]]
			{
				return 0ll;
			}

			auto old = last;
			last = steady_clock::now();
			return std::chrono::duration_cast<std::chrono::microseconds>(last - old).count();
		}
	};
}
