#pragma once

#include "../System.h"
#include "Utilities/address_range.h"
#include "Utilities/geometry.h"
#include "Utilities/asm.h"
#include "Utilities/VirtualMemory.h"
#include "Emu/Memory/vm.h"
#include "gcm_enums.h"
#include <atomic>
#include <memory>
#include <bitset>
#include <optional>

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

	// Definitions
	class thread;
	extern thread* g_current_renderer;

	//Base for resources with reference counting
	struct ref_counted
	{
		u8 deref_count = 0;

		void reset_refs() { deref_count = 0; }
	};

	/**
     * Holds information about a framebuffer
     */
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

		address_range get_memory_range(u32 aa_factor = 1) const
		{
			return address_range::start_length(address, pitch * height * aa_factor);
		}
	};

	struct avconf
	{
		u8 format = 0; //XRGB
		u8 aspect = 0; //AUTO
		u32 scanline_pitch = 0; //PACKED
		f32 gamma = 1.f; //NO GAMMA CORRECTION
	};

	struct blit_src_info
	{
		blit_engine::transfer_source_format format;
		blit_engine::transfer_origin origin;
		u16 offset_x;
		u16 offset_y;
		u16 width;
		u16 height;
		u16 slice_h;
		u16 pitch;
		void *pixels;

		bool compressed_x;
		bool compressed_y;
		u32 rsx_address;
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
		u16 max_tile_h;
		f32 scale_x;
		f32 scale_y;

		bool swizzled;
		void *pixels;

		bool compressed_x;
		bool compressed_y;
		u32  rsx_address;
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

	// Returns interleaved bits of X|Y|Z used as Z-order curve indices
	static inline u32 calculate_z_index(u32 x, u32 y, u32 z)
	{
		//Result = X' | Y' | Z' which are x,y,z bits interleaved
		u32 shift_size = 0;
		u32 result = 0;

		while (x | y | z)
		{
			result |= (x & 0x1) << shift_size++;
			result |= (y & 0x1) << shift_size++;
			result |= (z & 0x1) << shift_size++;

			x >>= 1;
			y >>= 1;
			z >>= 1;
		}

		return result;
	}

	/*   Note: What the ps3 calls swizzling in this case is actually z-ordering / morton ordering of pixels
	*       - Input can be swizzled or linear, bool flag handles conversion to and from
	*       - It will handle any width and height that are a power of 2, square or non square
	*    Restriction: It has mixed results if the height or width is not a power of 2
	*    Restriction: Only works with 2D surfaces
	*/
	template<typename T>
	void convert_linear_swizzle(void* input_pixels, void* output_pixels, u16 width, u16 height, u32 pitch, bool input_is_swizzled)
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

		if (!input_is_swizzled)
		{
			for (int y = 0; y < height; ++y)
			{
				T* src = static_cast<T*>(input_pixels) + y * adv;
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
				T* dst = static_cast<T*>(output_pixels) + y * adv;
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
	void convert_linear_swizzle_3d(void *input_pixels, void *output_pixels, u16 width, u16 height, u16 depth)
	{
		if (depth == 1)
		{
			convert_linear_swizzle<T>(input_pixels, output_pixels, width, height, width * sizeof(T), true);
			return;
		}

		T *src = static_cast<T*>(input_pixels);
		T *dst = static_cast<T*>(output_pixels);

		for (u32 z = 0; z < depth; ++z)
		{
			for (u32 y = 0; y < height; ++y)
			{
				for (u32 x = 0; x < width; ++x)
				{
					*dst++ = src[calculate_z_index(x, y, z)];
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
	void convert_le_d24x8_to_le_f32(void *dst, void *src, u32 row_length_in_texels, u32 num_rows);

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

	/**
	 * Calculates the regions used for memory transfer between rendertargets on succession events
	 */
	template <typename SurfaceType>
	std::tuple<u16, u16, u16, u16> get_transferable_region(SurfaceType* surface)
	{
		const u16 src_w = surface->old_contents->width();
		const u16 src_h = surface->old_contents->height();
		u16 dst_w = src_w;
		u16 dst_h = src_h;

		switch (static_cast<SurfaceType*>(surface->old_contents)->read_aa_mode)
		{
		case rsx::surface_antialiasing::center_1_sample:
			break;
		case rsx::surface_antialiasing::diagonal_centered_2_samples:
			dst_w *= 2;
			break;
		case rsx::surface_antialiasing::square_centered_4_samples:
		case rsx::surface_antialiasing::square_rotated_4_samples:
			dst_w *= 2;
			dst_h *= 2;
			break;
		}

		switch (surface->write_aa_mode)
		{
		case rsx::surface_antialiasing::center_1_sample:
			break;
		case rsx::surface_antialiasing::diagonal_centered_2_samples:
			dst_w /= 2;
			break;
		case rsx::surface_antialiasing::square_centered_4_samples:
		case rsx::surface_antialiasing::square_rotated_4_samples:
			dst_w /= 2;
			dst_h /= 2;
			break;
		}

		const f32 scale_x = (f32)dst_w / src_w;
		const f32 scale_y = (f32)dst_h / src_h;

		std::tie(std::ignore, std::ignore, dst_w, dst_h) = clip_region<u16>(dst_w, dst_h, 0, 0, surface->width(), surface->height(), true);
		return std::make_tuple(u16(dst_w / scale_x), u16(dst_h / scale_y), dst_w, dst_h);
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
		return ((u64)vert_data_base_offset + vert_base_offset) & 0xFFFFFFF;
	}

	// Similar to vertex_offset_base calculation, the rsx internally adds and masks index
	// before using
	static inline u32 get_index_from_base(u32 index, u32 index_base)
	{
		return ((u64)index + index_base) & 0x000FFFFF;
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

	static inline thread* get_current_renderer()
	{
		return g_current_renderer;
	}

	template <int N>
	void unpack_bitset(std::bitset<N>& block, u64* values)
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
		atomic_bitmask_t() { m_data.store(0); };
		~atomic_bitmask_t() {}

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

		inline u32 offset(const_iterator pos)
		{
			return (_data) ? (pos - _data) : 0;
		}

	public:
		simple_array() {}

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
			reserve(args.size());

			for (const auto& arg : args)
			{
				push_back(arg);
			}
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
			if (_capacity > size)
				return;

			auto old_data = _data;
			auto old_size = _size;

			_data = (Ty*)malloc(sizeof(Ty) * size);
			_capacity = size;

			if (old_data)
			{
				memcpy(_data, old_data, sizeof(Ty) * old_size);
				free(old_data);
			}
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

			const u32 remaining = (_size - _loc);
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
}
