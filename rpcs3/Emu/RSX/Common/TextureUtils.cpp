#include "stdafx.h"
#include "Emu/Memory/vm.h"
#include "TextureUtils.h"
#include "../RSXThread.h"
#include "../rsx_utils.h"
#include "3rdparty/bcdec/bcdec.hpp"

#include "util/asm.hpp"

namespace utils
{
	template <typename T, typename U>
	[[nodiscard]] auto bless(const std::span<U>& span)
	{
		return std::span<T>(bless<T>(span.data()), sizeof(U) * span.size() / sizeof(T));
	}

	template <typename T> requires(std::is_integral_v<T> && std::is_unsigned_v<T>)
	bool is_power_of_2(T value)
	{
		return !(value & (value - 1));
	}
}

namespace
{

#ifndef __APPLE__
u16 convert_rgb655_to_rgb565(const u16 bits)
{
	// g6 = g5
	// r5 = (((bits & 0xFC00) >> 1) & 0xFC00) << 1 is equivalent to truncating the least significant bit
	return (bits & 0xF81F) | (bits & 0x3E0) << 1;
}
#else
u32 convert_rgb565_to_bgra8(const u16 bits)
{
	const u8 r5 = ((bits >> 11) & 0x1F);
	const u8 g6 = ((bits >> 5) & 0x3F);
	const u8 b5 = (bits & 0x1F);

	const u8 b8 = ((b5 * 527) + 23) >> 6;
	const u8 g8 = ((g6 * 259) + 33) >> 6;
	const u8 r8 = ((r5 * 527) + 23) >> 6;
	const u8 a8 = 255;

	return b8 | (g8 << 8) | (r8 << 16) | (a8 << 24);
}

u32 convert_argb4_to_bgra8(const u16 bits)
{
	const u8 b8 = (bits & 0xF0);
	const u8 g8 = ((bits >> 4) & 0xF0);
	const u8 r8 = ((bits >> 8) & 0xF0);
	const u8 a8 = ((bits << 4) & 0xF0);

	return b8 | (g8 << 8) | (r8 << 16) | (a8 << 24);
}

u32 convert_a1rgb5_to_bgra8(const u16 bits)
{
	const u8 a1 = ((bits >> 11) & 0x80);
	const u8 r5 = ((bits >> 10) & 0x1F);
	const u8 g5 = ((bits >> 5) & 0x1F);
	const u8 b5 = (bits & 0x1F);

	const u8 b8 = ((b5 * 527) + 23) >> 6;
	const u8 g8 = ((g5 * 527) + 23) >> 6;
	const u8 r8 = ((r5 * 527) + 23) >> 6;
	const u8 a8 = a1;

	return b8 | (g8 << 8) | (r8 << 16) | (a8 << 24);
}

u32 convert_rgb5a1_to_bgra8(const u16 bits)
{
	const u8 r5 = ((bits >> 11) & 0x1F);
	const u8 g5 = ((bits >> 6) & 0x1F);
	const u8 b5 = ((bits >> 1) & 0x1F);
	const u8 a1 = (bits & 0x80);

	const u8 b8 = ((b5 * 527) + 23) >> 6;
	const u8 g8 = ((g5 * 527) + 23) >> 6;
	const u8 r8 = ((r5 * 527) + 23) >> 6;
	const u8 a8 = a1;

	return b8 | (g8 << 8) | (r8 << 16) | (a8 << 24);
}

u32 convert_rgb655_to_bgra8(const u16 bits)
{
	const u8 r6 = ((bits >> 10) & 0x3F);
	const u8 g5 = ((bits >> 5) & 0x1F);
	const u8 b5 = ((bits) & 0x1F);

	const u8 b8 = ((b5 * 527) + 23) >> 6;
	const u8 g8 = ((g5 * 527) + 23) >> 6;
	const u8 r8 = ((r6 * 259) + 33) >> 6;
	const u8 a8 = 1;

	return b8 | (g8 << 8) | (r8 << 16) | (a8 << 24);
}

u32 convert_d1rgb5_to_bgra8(const u16 bits)
{
	const u8 r5 = ((bits >> 10) & 0x1F);
	const u8 g5 = ((bits >> 5) & 0x1F);
	const u8 b5 = (bits & 0x1F);

	const u8 b8 = ((b5 * 527) + 23) >> 6;
	const u8 g8 = ((g5 * 527) + 23) >> 6;
	const u8 r8 = ((r5 * 527) + 23) >> 6;
	const u8 a8 = 1;

	return b8 | (g8 << 8) | (r8 << 16) | (a8 << 24);
}

struct convert_16_block_32
{
	template<typename T>
	static void copy_mipmap_level(std::span<u32> dst, std::span<const T> src, u16 width_in_block, u16 row_count, u16 depth, u8 border, u32 dst_pitch_in_block, u32 src_pitch_in_block, u32 (*converter)(const u16))
	{
		static_assert(sizeof(T) == 2, "Type size doesn't match.");

		u32 src_offset = 0, dst_offset = 0;
		const u32 v_porch = src_pitch_in_block * border;

		for (int layer = 0; layer < depth; ++layer)
		{
			// Front
			src_offset += v_porch;

			for (u32 row = 0; row < row_count; ++row)
			{
				for (int col = 0; col < width_in_block; ++col)
				{
					dst[dst_offset + col] = converter(src[src_offset + col + border]);
				}

				src_offset += src_pitch_in_block;
				dst_offset += dst_pitch_in_block;
			}

			// Back
			src_offset += v_porch;
		}
	}
};

struct convert_16_block_32_swizzled
{
	template<typename T, typename U>
	static void copy_mipmap_level(std::span<T> dst, std::span<const U> src, u16 width_in_block, u16 row_count, u16 depth, u8 border, u32 dst_pitch_in_block, u32 (*converter)(const u16))
	{
		u32 padded_width, padded_height;
		if (border)
		{
			padded_width = rsx::next_pow2(width_in_block + border + border);
			padded_height = rsx::next_pow2(row_count + border + border);
		}
		else
		{
			padded_width = width_in_block;
			padded_height = row_count;
		}

		u32 size = padded_width * padded_height * depth * 2;
		rsx::simple_array<U> tmp(size);

		rsx::convert_linear_swizzle_3d<U>(src.data(), tmp.data(), padded_width, padded_height, depth);

		std::span<const U> src_span = tmp;
		convert_16_block_32::copy_mipmap_level(dst, src_span, width_in_block, row_count, depth, border, dst_pitch_in_block, padded_width, converter);
	}
};
#endif

struct copy_unmodified_block
{
	template<typename T, typename U>
	static void copy_mipmap_level(std::span<T> dst, std::span<const U> src, u16 words_per_block, u16 width_in_block, u16 row_count, u16 depth, u8 border, u32 dst_pitch_in_block, u32 src_pitch_in_block)
	{
		static_assert(sizeof(T) == sizeof(U), "Type size doesn't match.");

		if (src_pitch_in_block == dst_pitch_in_block && !border)
		{
			// Fast copy
			const auto data_length = src_pitch_in_block * words_per_block * row_count * depth;
			std::copy_n(src.begin(), std::min<usz>({data_length, src.size(), dst.size()}), dst.begin());
			return;
		}

		const u32 width_in_words = width_in_block * words_per_block;
		const u32 src_pitch_in_words = src_pitch_in_block * words_per_block;
		const u32 dst_pitch_in_words = dst_pitch_in_block * words_per_block;

		const u32 h_porch = border * words_per_block;
		const u32 v_porch = src_pitch_in_words * border;
		u32 src_offset = h_porch, dst_offset = 0;

		for (int layer = 0; layer < depth; ++layer)
		{
			// Front
			src_offset += v_porch;

			for (int row = 0; row < row_count; ++row)
			{
				// NOTE: src_offset is already shifted along the border at initialization
				std::copy_n(src.begin() + src_offset, width_in_words, dst.begin() + dst_offset);

				src_offset += src_pitch_in_words;
				dst_offset += dst_pitch_in_words;
			}

			// Back
			src_offset += v_porch;
		}
	}
};

struct copy_unmodified_block_swizzled
{
	// NOTE: Pixel channel types are T (out) and const U (in). V is the pixel block type that consumes one whole pixel.
	// e.g 4x16-bit format can use u16, be_t<u16>, u64 as arguments
	template<typename T, typename U>
	static void copy_mipmap_level(std::span<T> dst, std::span<const U> src, u16 words_per_block, u16 width_in_block, u16 row_count, u16 depth, u8 border, u32 dst_pitch_in_block)
	{
		if (std::is_same_v<T, U> && dst_pitch_in_block == width_in_block && words_per_block == 1 && !border)
		{
			rsx::convert_linear_swizzle_3d<T>(src.data(), dst.data(), width_in_block, row_count, depth);
		}
		else
		{
			u32 padded_width, padded_height;
			if (border)
			{
				padded_width = rsx::next_pow2(width_in_block + border + border);
				padded_height = rsx::next_pow2(row_count + border + border);
			}
			else
			{
				padded_width = width_in_block;
				padded_height = row_count;
			}

			const u32 size_in_block = padded_width * padded_height * depth * 2;
			rsx::simple_array<U> tmp(size_in_block * words_per_block);

			if (words_per_block == 1) [[likely]]
			{
				rsx::convert_linear_swizzle_3d<T>(src.data(), tmp.data(), padded_width, padded_height, depth);
			}
			else
			{
				switch (words_per_block * sizeof(T))
				{
				case 4:
					rsx::convert_linear_swizzle_3d<u32>(src.data(), tmp.data(), padded_width, padded_height, depth);
					break;
				case 8:
					rsx::convert_linear_swizzle_3d<u64>(src.data(), tmp.data(), padded_width, padded_height, depth);
					break;
				case 16:
					rsx::convert_linear_swizzle_3d<u128>(src.data(), tmp.data(), padded_width, padded_height, depth);
					break;
				default:
					fmt::throw_exception("Failed to decode swizzled format, words_per_block=%d, src_type_size=%d", words_per_block, sizeof(T));
				}
			}

			std::span<const U> src_span = tmp;
			copy_unmodified_block::copy_mipmap_level(dst, src_span, words_per_block, width_in_block, row_count, depth, border, dst_pitch_in_block, padded_width);
		}
	}
};

struct copy_unmodified_block_vtc
{
	template<typename T, typename U>
	static void copy_mipmap_level(std::span<T> dst, std::span<const U> src, u16 width_in_block, u16 row_count, u16 depth, u32 dst_pitch_in_block, u32 /*src_pitch_in_block*/)
	{
		static_assert(sizeof(T) == sizeof(U), "Type size doesn't match.");
		u32 plane_size = dst_pitch_in_block * row_count;
		u32 row_element_count = width_in_block * row_count;
		u32 dst_offset = 0;
		u32 src_offset = 0;
		const u16 depth_4 = (depth >> 2) * 4;	// multiple of 4

		// Undo Nvidia VTC tiling - place each 2D texture slice back to back in linear memory
		//
		// More info:
		// https://www.khronos.org/registry/OpenGL/extensions/NV/NV_texture_compression_vtc.txt
		//
		// Note that the memory is tiled 4 planes at a time in the depth direction.
		// e.g.  d0, d1, d2, d3 is tiled as a group then d4, d5, d6, d7
		//

		//  Tile as 4x4x4
		for (int d = 0; d < depth_4; d++)
		{
			// Copy one slice of the 3d texture
			for (u32 i = 0; i < row_element_count; i += 1)
			{
				// Copy one span (8 bytes for DXT1 or 16 bytes for DXT5)
				dst[dst_offset + i] = src[src_offset + i * 4];
			}

			dst_offset += plane_size;

			// Last plane in the group of 4?
			if ((d & 0x3) == 0x3)
			{
				// Move forward to next group of 4 planes
				src_offset += row_element_count * 4 - 3;
			}
			else
			{
				src_offset += 1;
			}
		}

		// End Case - tile as 4x4x3 or 4x4x2 or 4x4x1
		const int vtc_tile_count = depth - depth_4;
		for (int d = 0; d < vtc_tile_count; d++)
		{
			// Copy one slice of the 3d texture
			for (u32 i = 0; i < row_element_count; i += 1)
			{
				// Copy one span (8 bytes for DXT1 or 16 bytes for DXT5)
				dst[dst_offset + i] = src[src_offset + i * vtc_tile_count];
			}

			dst_offset += plane_size;
			src_offset += 1;
		}
	}
};

struct copy_linear_block_to_vtc
{
	template<typename T, typename U>
	static void copy_mipmap_level(std::span<T> dst, std::span<const U> src, u16 width_in_block, u16 row_count, u16 depth, u32 /*dst_pitch_in_block*/, u32 src_pitch_in_block)
	{
		static_assert(sizeof(T) == sizeof(U), "Type size doesn't match.");
		u32 plane_size = src_pitch_in_block * row_count;
		u32 row_element_count = width_in_block * row_count;
		u32 dst_offset = 0;
		u32 src_offset = 0;
		const u16 depth_4 = (depth >> 2) * 4;	// multiple of 4

		// Convert incoming linear texture to VTC compressed texture
		// https://www.khronos.org/registry/OpenGL/extensions/NV/NV_texture_compression_vtc.txt

		//  Tile as 4x4x4
		for (int d = 0; d < depth_4; d++)
		{
			// Copy one slice of the 3d texture
			for (u32 i = 0; i < row_element_count; i += 1)
			{
				// Copy one span (8 bytes for DXT1 or 16 bytes for DXT5)
				dst[dst_offset + i * 4] = src[src_offset + i];
			}

			src_offset += plane_size;

			// Last plane in the group of 4?
			if ((d & 0x3) == 0x3)
			{
				// Move forward to next group of 4 planes
				dst_offset += row_element_count * 4 - 3;
			}
			else
			{
				dst_offset ++;
			}
		}

		// End Case - tile as 4x4x3 or 4x4x2 or 4x4x1
		const int vtc_tile_count = depth - depth_4;
		for (int d = 0; d < vtc_tile_count; d++)
		{
			// Copy one slice of the 3d texture
			for (u32 i = 0; i < row_element_count; i += 1)
			{
				// Copy one span (8 bytes for DXT1 or 16 bytes for DXT5)
				dst[dst_offset + i * vtc_tile_count] = src[src_offset + i];
			}

			src_offset += row_element_count;
			dst_offset ++;
		}
	}
};

struct copy_decoded_rb_rg_block
{
	template <bool SwapWords = false, typename T>
	static void copy_mipmap_level(std::span<u32> dst, std::span<const T> src, u16 width_in_block, u16 row_count, u16 depth, u32 dst_pitch_in_block, u32 src_pitch_in_block)
	{
		static_assert(sizeof(T) == 4, "Type size doesn't match.");

		u32 src_offset = 0;
		u32 dst_offset = 0;

		// Temporaries
		u32 red0, red1, blue, green;

		for (int row = 0; row < row_count * depth; ++row)
		{
			for (int col = 0; col < width_in_block; ++col)
			{
				// Decompress one block to 2 pixels at a time and write output in BGRA format
				const auto data = src[src_offset + col];

				if constexpr (SwapWords)
				{
					// BR_GR
					blue = (data >> 0) & 0xFF;
					red0 = (data >> 8) & 0xFF;
					green = (data >> 16) & 0XFF;
					red1 = (data >> 24) & 0xFF;
				}
				else
				{
					// RB_RG
					red0 = (data >> 0) & 0xFF;
					blue = (data >> 8) & 0xFF;
					red1 = (data >> 16) & 0XFF;
					green = (data >> 24) & 0xFF;
				}

				dst[dst_offset + (col * 2)] = blue | (green << 8) | (red0 << 16) | (0xFF << 24);
				dst[dst_offset + (col * 2 + 1)] = blue | (green << 8) | (red1 << 16) | (0xFF << 24);
			}

			src_offset += src_pitch_in_block;
			dst_offset += dst_pitch_in_block;
		}
	}
};

struct copy_rgb655_block
{
	template<typename T>
	static void copy_mipmap_level(std::span<u16> dst, std::span<const T> src, u16 width_in_block, u16 row_count, u16 depth, u8 border, u32 dst_pitch_in_block, u32 src_pitch_in_block)
	{
		static_assert(sizeof(T) == 2, "Type size doesn't match.");

		u32 src_offset = 0, dst_offset = 0;
		const u32 v_porch = src_pitch_in_block * border;

		for (int layer = 0; layer < depth; ++layer)
		{
			// Front
			src_offset += v_porch;

			for (u32 row = 0; row < row_count; ++row)
			{
				for (int col = 0; col < width_in_block; ++col)
				{
					dst[dst_offset + col] = convert_rgb655_to_rgb565(src[src_offset + col + border]);
				}

				src_offset += src_pitch_in_block;
				dst_offset += dst_pitch_in_block;
			}

			// Back
			src_offset += v_porch;
		}
	}
};

struct copy_rgb655_block_swizzled
{
	template<typename T, typename U>
	static void copy_mipmap_level(std::span<T> dst, std::span<const U> src, u16 width_in_block, u16 row_count, u16 depth, u8 border, u32 dst_pitch_in_block)
	{
		u32 padded_width, padded_height;
		if (border)
		{
			padded_width = rsx::next_pow2(width_in_block + border + border);
			padded_height = rsx::next_pow2(row_count + border + border);
		}
		else
		{
			padded_width = width_in_block;
			padded_height = row_count;
		}

		u32 size = padded_width * padded_height * depth * 2;
		rsx::simple_array<U> tmp(size);

		rsx::convert_linear_swizzle_3d<U>(src.data(), tmp.data(), padded_width, padded_height, depth);

		std::span<const U> src_span = tmp;
		copy_rgb655_block::copy_mipmap_level(dst, src_span, width_in_block, row_count, depth, border, dst_pitch_in_block, padded_width);
	}
};

struct copy_decoded_bc1_block
{
	static void copy_mipmap_level(std::span<u32> dst, std::span<const u64> src, u16 width_in_block, u32 row_count, u16 depth, u32 dst_pitch_in_block, u32 src_pitch_in_block)
	{
		u32 src_offset = 0, dst_offset = 0, destinationPitch = dst_pitch_in_block * 4;
		for (u32 row = 0; row < row_count * depth; row++)
		{
			for (u32 col = 0; col < width_in_block; col++)
			{
				const u8* compressedBlock = reinterpret_cast<const u8*>(&src[src_offset + col]);
				u8* decompressedBlock = reinterpret_cast<u8*>(&dst[dst_offset + col * 4]);
				bcdec_bc1(compressedBlock, decompressedBlock, destinationPitch);
			}

			src_offset += src_pitch_in_block;
			dst_offset += destinationPitch;
		}
	}
};

struct copy_decoded_bc2_block
{
	static void copy_mipmap_level(std::span<u32> dst, std::span<const u128> src, u16 width_in_block, u32 row_count, u16 depth, u32 dst_pitch_in_block, u32 src_pitch_in_block)
	{
		u32 src_offset = 0, dst_offset = 0, destinationPitch = dst_pitch_in_block * 4;
		for (u32 row = 0; row < row_count * depth; row++)
		{
			for (u32 col = 0; col < width_in_block; col++)
			{
				const u8* compressedBlock = reinterpret_cast<const u8*>(&src[src_offset + col]);
				u8* decompressedBlock = reinterpret_cast<u8*>(&dst[dst_offset + col * 4]);
				bcdec_bc2(compressedBlock, decompressedBlock, destinationPitch);
			}

			src_offset += src_pitch_in_block;
			dst_offset += destinationPitch;
		}
	}
};

struct copy_decoded_bc3_block
{
	static void copy_mipmap_level(std::span<u32> dst, std::span<const u128> src, u16 width_in_block, u32 row_count, u16 depth, u32 dst_pitch_in_block, u32 src_pitch_in_block)
	{
		u32 src_offset = 0, dst_offset = 0, destinationPitch = dst_pitch_in_block * 4;
		for (u32 row = 0; row < row_count * depth; row++)
		{
			for (u32 col = 0; col < width_in_block; col++)
			{
				const u8* compressedBlock = reinterpret_cast<const u8*>(&src[src_offset + col]);
				u8* decompressedBlock = reinterpret_cast<u8*>(&dst[dst_offset + col * 4]);
				bcdec_bc3(compressedBlock, decompressedBlock, destinationPitch);
			}

			src_offset += src_pitch_in_block;
			dst_offset += destinationPitch;
		}
	}
};

namespace
{
	/**
	 * Generates copy instructions required to build the texture GPU side without actually copying anything.
	 * Returns a set of addresses and data lengths to use. This can be used to generate a GPU task to avoid CPU doing the heavy lifting.
	 */
	std::vector<rsx::memory_transfer_cmd>
	build_transfer_cmds(const void* src, u16 block_size_in_bytes, u16 width_in_block, u16 row_count, u16 depth, u8 border, u32 dst_pitch_in_block, u32 src_pitch_in_block)
	{
		std::vector<rsx::memory_transfer_cmd> result;

		if (src_pitch_in_block == dst_pitch_in_block && !border)
		{
			// Fast copy
			rsx::memory_transfer_cmd cmd;
			cmd.src = src;
			cmd.dst = nullptr;
			cmd.length = src_pitch_in_block * block_size_in_bytes * row_count * depth;
			return { cmd };
		}

		const u32 width_in_bytes = width_in_block * block_size_in_bytes;
		const u32 src_pitch_in_bytes = src_pitch_in_block * block_size_in_bytes;
		const u32 dst_pitch_in_bytes = dst_pitch_in_block * block_size_in_bytes;

		const u32 h_porch = border * block_size_in_bytes;
		const u32 v_porch = src_pitch_in_bytes * border;

		auto src_ = static_cast<const char*>(src) + h_porch;
		auto dst_ = static_cast<const char*>(nullptr);

		for (int layer = 0; layer < depth; ++layer)
		{
			// Front
			src_ += v_porch;

			for (int row = 0; row < row_count; ++row)
			{
				rsx::memory_transfer_cmd cmd{ dst_, src_, width_in_bytes };
				result.push_back(cmd);
				src_ += src_pitch_in_bytes;
				dst_ += dst_pitch_in_bytes;
			}

			// Back
			src_ += v_porch;
		}

		return result;
	}

	/**
	 * Texture upload template.
	 *
	 * Source textures are stored as following (for power of 2 textures):
	 * - For linear texture every mipmap level share rowpitch (which is the one of mipmap 0). This means that for non 0 mipmap there's padding between row.
	 * - For swizzled texture row pitch is texture width X pixel/block size. There's not padding between row.
	 * - There is no padding between 2 mipmap levels. This means that next mipmap level starts at offset rowpitch X row count
	 * - Cubemap images are 128 bytes aligned.
	 *
	 * The template iterates over all depth (including cubemap) and over all mipmaps.
	 * Sometimes texture provides a pitch even if texture is swizzled (and then packed) and in such case it's ignored. It's passed via suggested_pitch and is used only if padded_row is false.
	 */
	template <u8 block_edge_in_texel, typename SRC_TYPE>
	std::vector<rsx::subresource_layout> get_subresources_layout_impl(const std::byte *texture_data_pointer, u16 width_in_texel, u16 height_in_texel, u16 depth, u8 layer_count, u16 mipmap_count, u32 suggested_pitch_in_bytes, bool padded_row, bool border)
	{
		/**
		* Note about size type: RSX texture width is stored in a 16 bits int and pitch is stored in a 20 bits int.
		*/

		// <= 128 so fits in u8
		constexpr u8 block_size_in_bytes = sizeof(SRC_TYPE);

		std::vector<rsx::subresource_layout> result;
		usz offset_in_src = 0;

		const u8 border_size = border ? (padded_row ? 1 : 4) : 0;
		u32 src_pitch_in_block;
		u32 full_height_in_block;

		for (unsigned layer = 0; layer < layer_count; layer++)
		{
			u16 miplevel_width_in_texel = width_in_texel, miplevel_height_in_texel = height_in_texel, miplevel_depth = depth;
			for (unsigned mip_level = 0; mip_level < mipmap_count; mip_level++)
			{
				result.push_back({});
				rsx::subresource_layout& current_subresource_layout = result.back();

				current_subresource_layout.width_in_texel = miplevel_width_in_texel;
				current_subresource_layout.height_in_texel = miplevel_height_in_texel;
				current_subresource_layout.level = mip_level;
				current_subresource_layout.layer = layer;
				current_subresource_layout.depth = miplevel_depth;
				current_subresource_layout.border = border_size;

				if constexpr (block_edge_in_texel == 1)
				{
					current_subresource_layout.width_in_block = miplevel_width_in_texel;
					current_subresource_layout.height_in_block = miplevel_height_in_texel;
				}
				else if constexpr (block_edge_in_texel == 4)
				{
					current_subresource_layout.width_in_block = utils::aligned_div(miplevel_width_in_texel, block_edge_in_texel);
					current_subresource_layout.height_in_block = utils::aligned_div(miplevel_height_in_texel, block_edge_in_texel);
				}
				else
				{
					// Only the width is compressed
					current_subresource_layout.width_in_block = utils::aligned_div(miplevel_width_in_texel, block_edge_in_texel);
					current_subresource_layout.height_in_block = miplevel_height_in_texel;
				}

				if (padded_row)
				{
					src_pitch_in_block = suggested_pitch_in_bytes / block_size_in_bytes;
					full_height_in_block = current_subresource_layout.height_in_block + (border_size + border_size);
				}
				else if (!border)
				{
					src_pitch_in_block = current_subresource_layout.width_in_block;
					full_height_in_block = current_subresource_layout.height_in_block;
				}
				else
				{
					src_pitch_in_block = rsx::next_pow2(current_subresource_layout.width_in_block + border_size + border_size);
					full_height_in_block = rsx::next_pow2(current_subresource_layout.height_in_block + border_size + border_size);
				}

				const u32 slice_sz = src_pitch_in_block * block_size_in_bytes * full_height_in_block * miplevel_depth;
				current_subresource_layout.pitch_in_block = src_pitch_in_block;
				current_subresource_layout.data = { texture_data_pointer + offset_in_src, slice_sz };

				offset_in_src += slice_sz;
				miplevel_width_in_texel = std::max(miplevel_width_in_texel / 2, 1);
				miplevel_height_in_texel = std::max(miplevel_height_in_texel / 2, 1);
				miplevel_depth = std::max(miplevel_depth / 2, 1);
			}

			if (!padded_row) // Only swizzled textures obey this restriction
			{
				offset_in_src = utils::align(offset_in_src, 128);
			}
		}

		return result;
	}
}

template<typename T>
u32 get_row_pitch_in_block(u16 width_in_block, usz alignment)
{
	if (const usz pitch = width_in_block * sizeof(T);
		pitch == alignment)
	{
		return width_in_block;
	}
	else
	{
		usz divided = (pitch + alignment - 1) / alignment;
		return static_cast<u32>(divided * alignment / sizeof(T));
	}
}

u32 get_row_pitch_in_block(u16 block_size_in_bytes, u16 width_in_block, usz alignment)
{
	if (const usz pitch = width_in_block * block_size_in_bytes;
		pitch == alignment)
	{
		return width_in_block;
	}
	else
	{
		usz divided = (pitch + alignment - 1) / alignment;
		return static_cast<u32>(divided * alignment / block_size_in_bytes);
	}
}

/**
 * Since rsx ignore unused dimensionality some app set them to 0.
 * Use 1 value instead to be more general.
 */
template<typename RsxTextureType>
std::tuple<u16, u16, u8> get_height_depth_layer(const RsxTextureType &tex)
{
	switch (tex.get_extended_texture_dimension())
	{
	case rsx::texture_dimension_extended::texture_dimension_1d: return std::make_tuple(1, 1, 1);
	case rsx::texture_dimension_extended::texture_dimension_2d: return std::make_tuple(tex.height(), 1, 1);
	case rsx::texture_dimension_extended::texture_dimension_cubemap: return std::make_tuple(tex.height(), 1, 6);
	case rsx::texture_dimension_extended::texture_dimension_3d: return std::make_tuple(tex.height(), tex.depth(), 1);
	}
	fmt::throw_exception("Unsupported texture dimension");
}
}

template<typename RsxTextureType>
std::vector<rsx::subresource_layout> get_subresources_layout_impl(const RsxTextureType &texture)
{
	u16 w = texture.width();
	u16 h;
	u16 depth;
	u8 layer;

	std::tie(h, depth, layer) = get_height_depth_layer(texture);

	const auto format = texture.format() & ~(CELL_GCM_TEXTURE_LN | CELL_GCM_TEXTURE_UN);
	auto pitch = texture.pitch();

	const u32 texaddr = rsx::get_address(texture.offset(), texture.location());
	auto pixels = vm::_ptr<const std::byte>(texaddr);

	const bool is_swizzled = !(texture.format() & CELL_GCM_TEXTURE_LN);
	const bool has_border = !texture.border_type();

	if (!is_swizzled)
	{
		if (const auto packed_pitch = rsx::get_format_packed_pitch(format, w, has_border, false); pitch < packed_pitch) [[unlikely]]
		{
			if (pitch)
			{
				const u32 real_width_in_block = pitch / rsx::get_format_block_size_in_bytes(format);
				w = std::max<u16>(real_width_in_block * rsx::get_format_block_size_in_texel(format), 1);
			}
			else
			{
				h = depth = 1;
				pitch = packed_pitch;
			}
		}
	}

	switch (format)
	{
	case CELL_GCM_TEXTURE_B8:
		return get_subresources_layout_impl<1, u8>(pixels, w, h, depth, layer, texture.get_exact_mipmap_count(), pitch, !is_swizzled, has_border);
	case CELL_GCM_TEXTURE_COMPRESSED_B8R8_G8R8:
	case CELL_GCM_TEXTURE_COMPRESSED_R8B8_R8G8:
		return get_subresources_layout_impl<2, u32>(pixels, w, h, depth, layer, texture.get_exact_mipmap_count(), pitch, !is_swizzled, has_border);
	case CELL_GCM_TEXTURE_COMPRESSED_HILO8:
	case CELL_GCM_TEXTURE_COMPRESSED_HILO_S8:
	case CELL_GCM_TEXTURE_DEPTH16:
	case CELL_GCM_TEXTURE_DEPTH16_FLOAT: // Untested
	case CELL_GCM_TEXTURE_D1R5G5B5:
	case CELL_GCM_TEXTURE_A1R5G5B5:
	case CELL_GCM_TEXTURE_A4R4G4B4:
	case CELL_GCM_TEXTURE_R5G5B5A1:
	case CELL_GCM_TEXTURE_R5G6B5:
	case CELL_GCM_TEXTURE_R6G5B5:
	case CELL_GCM_TEXTURE_G8B8:
	case CELL_GCM_TEXTURE_X16:
		return get_subresources_layout_impl<1, u16>(pixels, w, h, depth, layer, texture.get_exact_mipmap_count(), pitch, !is_swizzled, has_border);
	case CELL_GCM_TEXTURE_DEPTH24_D8: // Untested
	case CELL_GCM_TEXTURE_DEPTH24_D8_FLOAT: // Untested
	case CELL_GCM_TEXTURE_D8R8G8B8:
	case CELL_GCM_TEXTURE_A8R8G8B8:
	case CELL_GCM_TEXTURE_Y16_X16:
	case CELL_GCM_TEXTURE_Y16_X16_FLOAT:
	case CELL_GCM_TEXTURE_X32_FLOAT:
		return get_subresources_layout_impl<1, u32>(pixels, w, h, depth, layer, texture.get_exact_mipmap_count(), pitch, !is_swizzled, has_border);
	case CELL_GCM_TEXTURE_W16_Z16_Y16_X16_FLOAT:
		return get_subresources_layout_impl<1, u64>(pixels, w, h, depth, layer, texture.get_exact_mipmap_count(), pitch, !is_swizzled, has_border);
	case CELL_GCM_TEXTURE_W32_Z32_Y32_X32_FLOAT:
		return get_subresources_layout_impl<1, u128>(pixels, w, h, depth, layer, texture.get_exact_mipmap_count(), pitch, !is_swizzled, has_border);
	case CELL_GCM_TEXTURE_COMPRESSED_DXT1:
		return get_subresources_layout_impl<4, u64>(pixels, w, h, depth, layer, texture.get_exact_mipmap_count(), pitch, !is_swizzled, false);
	case CELL_GCM_TEXTURE_COMPRESSED_DXT23:
	case CELL_GCM_TEXTURE_COMPRESSED_DXT45:
		return get_subresources_layout_impl<4, u128>(pixels, w, h, depth, layer, texture.get_exact_mipmap_count(), pitch, !is_swizzled, false);
	}
	fmt::throw_exception("Wrong format 0x%x", format);
}

namespace rsx
{
	void typeless_xfer::analyse()
	{
		// TODO: This method needs to be re-evaluated
		// Check if scaling hints match, which likely means internal formats match as well
		// Only possible when doing RTT->RTT transfer with non-base-type formats like WZYX16/32
		if (src_is_typeless && dst_is_typeless && src_gcm_format == dst_gcm_format)
		{
			if (fcmp(src_scaling_hint, dst_scaling_hint) && !fcmp(src_scaling_hint, 1.f))
			{
				src_is_typeless = dst_is_typeless = false;
				src_scaling_hint = dst_scaling_hint = 1.f;
			}
		}
	}

	std::vector<rsx::subresource_layout> get_subresources_layout(const rsx::fragment_texture& texture)
	{
		return get_subresources_layout_impl(texture);
	}

	std::vector<rsx::subresource_layout> get_subresources_layout(const rsx::vertex_texture& texture)
	{
		return get_subresources_layout_impl(texture);
	}

	texture_memory_info upload_texture_subresource(rsx::io_buffer& dst_buffer, const rsx::subresource_layout& src_layout, int format, bool is_swizzled, texture_uploader_capabilities& caps)
	{
		u16 w = src_layout.width_in_block;
		u16 h = src_layout.height_in_block;
		u16 depth = src_layout.depth;
		u32 pitch = src_layout.pitch_in_block;

		texture_memory_info result{};

		// Ignore when texture width > pitch
		if (w > pitch)
			return result;

		// Check if we can use a fast path
		int word_size = 0;
		int words_per_block;
		u32 dst_pitch_in_block;

		switch (format)
		{
		case CELL_GCM_TEXTURE_B8:
		{
			word_size = words_per_block = 1;
			dst_pitch_in_block = get_row_pitch_in_block<u8>(w, caps.alignment);
			break;
		}

		case CELL_GCM_TEXTURE_COMPRESSED_B8R8_G8R8:
		{
			copy_decoded_rb_rg_block::copy_mipmap_level<true>(dst_buffer.as_span<u32>(), src_layout.data.as_span<const u32>(), w, h, depth, get_row_pitch_in_block<u32>(src_layout.width_in_texel, caps.alignment), src_layout.pitch_in_block);
			break;
		}

		case CELL_GCM_TEXTURE_COMPRESSED_R8B8_R8G8:
		{
			copy_decoded_rb_rg_block::copy_mipmap_level(dst_buffer.as_span<u32>(), src_layout.data.as_span<const u32>(), w, h, depth, get_row_pitch_in_block<u32>(src_layout.width_in_texel, caps.alignment), src_layout.pitch_in_block);
			break;
		}

#ifndef __APPLE__
		case CELL_GCM_TEXTURE_R6G5B5:
		{
			if (is_swizzled)
				copy_rgb655_block_swizzled::copy_mipmap_level(dst_buffer.as_span<u16>(), src_layout.data.as_span<const be_t<u16>>(), w, h, depth, src_layout.border, get_row_pitch_in_block<u16>(w, caps.alignment));
			else
				copy_rgb655_block::copy_mipmap_level(dst_buffer.as_span<u16>(), src_layout.data.as_span<const be_t<u16>>(), w, h, depth, src_layout.border, get_row_pitch_in_block<u16>(w, caps.alignment), src_layout.pitch_in_block);
			break;
		}

		case CELL_GCM_TEXTURE_D1R5G5B5:
		case CELL_GCM_TEXTURE_A1R5G5B5:
		case CELL_GCM_TEXTURE_A4R4G4B4:
		case CELL_GCM_TEXTURE_R5G5B5A1:
		case CELL_GCM_TEXTURE_R5G6B5:
#else
		// convert the following formats to B8G8R8A8_UNORM, because they are not supported by Metal
		case CELL_GCM_TEXTURE_R6G5B5:
		{
			if (is_swizzled)
				convert_16_block_32_swizzled::copy_mipmap_level(dst_buffer.as_span<u32>(), src_layout.data.as_span<const be_t<u16>>(), w, h, depth, src_layout.border, get_row_pitch_in_block<u32>(w, caps.alignment), &convert_rgb655_to_bgra8);
			else
				convert_16_block_32::copy_mipmap_level(dst_buffer.as_span<u32>(), src_layout.data.as_span<const be_t<u16>>(), w, h, depth, src_layout.border, get_row_pitch_in_block<u32>(w, caps.alignment), src_layout.pitch_in_block, &convert_rgb655_to_bgra8);
			break;
		}
		case CELL_GCM_TEXTURE_D1R5G5B5:
		{
			if (is_swizzled)
				convert_16_block_32_swizzled::copy_mipmap_level(dst_buffer.as_span<u32>(), src_layout.data.as_span<const be_t<u16>>(), w, h, depth, src_layout.border, get_row_pitch_in_block<u32>(w, caps.alignment), &convert_d1rgb5_to_bgra8);
			else
				convert_16_block_32::copy_mipmap_level(dst_buffer.as_span<u32>(), src_layout.data.as_span<const be_t<u16>>(), w, h, depth, src_layout.border, get_row_pitch_in_block<u32>(w, caps.alignment), src_layout.pitch_in_block, &convert_d1rgb5_to_bgra8);
			break;
		}
		case CELL_GCM_TEXTURE_A1R5G5B5:
		{
			if (is_swizzled)
				convert_16_block_32_swizzled::copy_mipmap_level(dst_buffer.as_span<u32>(), src_layout.data.as_span<const be_t<u16>>(), w, h, depth, src_layout.border, get_row_pitch_in_block<u32>(w, caps.alignment), &convert_a1rgb5_to_bgra8);
			else
				convert_16_block_32::copy_mipmap_level(dst_buffer.as_span<u32>(), src_layout.data.as_span<const be_t<u16>>(), w, h, depth, src_layout.border, get_row_pitch_in_block<u32>(w, caps.alignment), src_layout.pitch_in_block, &convert_a1rgb5_to_bgra8);
			break;
		}
		case CELL_GCM_TEXTURE_A4R4G4B4:
		{
			if (is_swizzled)
				convert_16_block_32_swizzled::copy_mipmap_level(dst_buffer.as_span<u32>(), src_layout.data.as_span<const be_t<u16>>(), w, h, depth, src_layout.border, get_row_pitch_in_block<u32>(w, caps.alignment), &convert_argb4_to_bgra8);
			else
				convert_16_block_32::copy_mipmap_level(dst_buffer.as_span<u32>(), src_layout.data.as_span<const be_t<u16>>(), w, h, depth, src_layout.border, get_row_pitch_in_block<u32>(w, caps.alignment), src_layout.pitch_in_block, &convert_argb4_to_bgra8);
			break;
		}
		case CELL_GCM_TEXTURE_R5G5B5A1:
		{
			if (is_swizzled)
				convert_16_block_32_swizzled::copy_mipmap_level(dst_buffer.as_span<u32>(), src_layout.data.as_span<const be_t<u16>>(), w, h, depth, src_layout.border, get_row_pitch_in_block<u32>(w, caps.alignment), &convert_rgb5a1_to_bgra8);
			else
				convert_16_block_32::copy_mipmap_level(dst_buffer.as_span<u32>(), src_layout.data.as_span<const be_t<u16>>(), w, h, depth, src_layout.border, get_row_pitch_in_block<u32>(w, caps.alignment), src_layout.pitch_in_block, &convert_rgb5a1_to_bgra8);
			break;
		}
		case CELL_GCM_TEXTURE_R5G6B5:
		{
			if (is_swizzled)
				convert_16_block_32_swizzled::copy_mipmap_level(dst_buffer.as_span<u32>(), src_layout.data.as_span<const be_t<u16>>(), w, h, depth, src_layout.border, get_row_pitch_in_block<u32>(w, caps.alignment), &convert_rgb565_to_bgra8);
			else
				convert_16_block_32::copy_mipmap_level(dst_buffer.as_span<u32>(), src_layout.data.as_span<const be_t<u16>>(), w, h, depth, src_layout.border, get_row_pitch_in_block<u32>(w, caps.alignment), src_layout.pitch_in_block, &convert_rgb565_to_bgra8);
			break;
		}
#endif
		case CELL_GCM_TEXTURE_COMPRESSED_HILO8:
		case CELL_GCM_TEXTURE_COMPRESSED_HILO_S8:
			// TODO: Test if the HILO compressed formats support swizzling (other compressed_* formats ignore this option)
		case CELL_GCM_TEXTURE_DEPTH16:
		case CELL_GCM_TEXTURE_DEPTH16_FLOAT: // Untested
		case CELL_GCM_TEXTURE_G8B8:
		{
			word_size = 2;
			words_per_block = 1;
			dst_pitch_in_block = get_row_pitch_in_block<u16>(w, caps.alignment);
			break;
		}

		case CELL_GCM_TEXTURE_A8R8G8B8:
		case CELL_GCM_TEXTURE_D8R8G8B8:
		case CELL_GCM_TEXTURE_DEPTH24_D8:
		case CELL_GCM_TEXTURE_DEPTH24_D8_FLOAT: // Untested
		{
			word_size = 4;
			words_per_block = 1;
			dst_pitch_in_block = get_row_pitch_in_block<u32>(w, caps.alignment);
			break;
		}

		// NOTE: Textures with WZYX notations refer to arbitrary data and not color swizzles as in common GPU lang
		// WZYX actually maps directly as a RGBA16 format in Cell memory! R=W, not R=X

		case CELL_GCM_TEXTURE_X16:
		case CELL_GCM_TEXTURE_Y16_X16:
		case CELL_GCM_TEXTURE_Y16_X16_FLOAT:
		case CELL_GCM_TEXTURE_W16_Z16_Y16_X16_FLOAT:
		{
			const u16 block_size = get_format_block_size_in_bytes(format);
			word_size = 2;
			words_per_block = block_size / 2;
			dst_pitch_in_block = get_row_pitch_in_block(block_size, w, caps.alignment);
			break;
		}

		case CELL_GCM_TEXTURE_X32_FLOAT:
		case CELL_GCM_TEXTURE_W32_Z32_Y32_X32_FLOAT:
		{
			const u16 block_size = get_format_block_size_in_bytes(format);
			word_size = 4;
			words_per_block = block_size / 4;
			dst_pitch_in_block = get_row_pitch_in_block(block_size, w, caps.alignment);
			break;
		}

		case CELL_GCM_TEXTURE_COMPRESSED_DXT1:
		{
			if (!caps.supports_dxt)
			{
				copy_decoded_bc1_block::copy_mipmap_level(dst_buffer.as_span<u32>(), src_layout.data.as_span<const u64>(), w, h, depth, get_row_pitch_in_block<u32>(w, caps.alignment), src_layout.pitch_in_block);
				break;
			}

			const bool is_3d = depth > 1;
			const bool is_po2 = utils::is_power_of_2(src_layout.width_in_texel) && utils::is_power_of_2(src_layout.height_in_texel);

			if (is_3d && is_po2 && !caps.supports_vtc_decoding)
			{
				// PS3 uses the Nvidia VTC memory layout for compressed 3D textures.
				// This is only supported using Nvidia OpenGL.
				// Remove the VTC tiling to support ATI and Vulkan.
				copy_unmodified_block_vtc::copy_mipmap_level(dst_buffer.as_span<u64>(), src_layout.data.as_span<const u64>(), w, h, depth, get_row_pitch_in_block<u64>(w, caps.alignment), src_layout.pitch_in_block);
			}
			else if (is_3d && !is_po2 && caps.supports_vtc_decoding)
			{
				// In this case, hardware expects us to feed it a VTC input, but on PS3 we only have a linear one.
				// We need to compress the 2D-planar DXT input into a VTC output
				copy_linear_block_to_vtc::copy_mipmap_level(dst_buffer.as_span<u64>(), src_layout.data.as_span<const u64>(), w, h, depth, get_row_pitch_in_block<u64>(w, caps.alignment), src_layout.pitch_in_block);
			}
			else if (caps.supports_zero_copy)
			{
				result.require_upload = true;
				result.deferred_cmds = build_transfer_cmds(src_layout.data.data(), 8, w, h, depth, 0, get_row_pitch_in_block<u64>(w, caps.alignment), src_layout.pitch_in_block);
			}
			else
			{
				copy_unmodified_block::copy_mipmap_level(dst_buffer.as_span<u64>(), src_layout.data.as_span<const u64>(), 1, w, h, depth, 0, get_row_pitch_in_block<u64>(w, caps.alignment), src_layout.pitch_in_block);
			}
			break;
		}

		case CELL_GCM_TEXTURE_COMPRESSED_DXT23:
		{
			if (!caps.supports_dxt)
			{
				copy_decoded_bc2_block::copy_mipmap_level(dst_buffer.as_span<u32>(), src_layout.data.as_span<const u128>(), w, h, depth, get_row_pitch_in_block<u32>(w, caps.alignment), src_layout.pitch_in_block);
				break;
			}
			[[fallthrough]];
		}
		case CELL_GCM_TEXTURE_COMPRESSED_DXT45:
		{
			if (!caps.supports_dxt)
			{
				copy_decoded_bc3_block::copy_mipmap_level(dst_buffer.as_span<u32>(), src_layout.data.as_span<const u128>(), w, h, depth, get_row_pitch_in_block<u32>(w, caps.alignment), src_layout.pitch_in_block);
				break;
			}

			const bool is_3d = depth > 1;
			const bool is_po2 = utils::is_power_of_2(src_layout.width_in_texel) && utils::is_power_of_2(src_layout.height_in_texel);

			if (is_3d && is_po2 && !caps.supports_vtc_decoding)
			{
				// PS3 uses the Nvidia VTC memory layout for compressed 3D textures.
				// This is only supported using Nvidia OpenGL.
				// Remove the VTC tiling to support ATI and Vulkan.
				copy_unmodified_block_vtc::copy_mipmap_level(dst_buffer.as_span<u128>(), src_layout.data.as_span<const u128>(), w, h, depth, get_row_pitch_in_block<u128>(w, caps.alignment), src_layout.pitch_in_block);
			}
			else if (is_3d && !is_po2 && caps.supports_vtc_decoding)
			{
				// In this case, hardware expects us to feed it a VTC input, but on PS3 we only have a linear one.
				// We need to compress the 2D-planar DXT input into a VTC output
				copy_linear_block_to_vtc::copy_mipmap_level(dst_buffer.as_span<u128>(), src_layout.data.as_span<const u128>(), w, h, depth, get_row_pitch_in_block<u128>(w, caps.alignment), src_layout.pitch_in_block);
			}
			else if (caps.supports_zero_copy)
			{
				result.require_upload = true;
				result.deferred_cmds = build_transfer_cmds(src_layout.data.data(), 16, w, h, depth, 0, get_row_pitch_in_block<u128>(w, caps.alignment), src_layout.pitch_in_block);
			}
			else
			{
				copy_unmodified_block::copy_mipmap_level(dst_buffer.as_span<u128>(), src_layout.data.as_span<const u128>(), 1, w, h, depth, 0, get_row_pitch_in_block<u128>(w, caps.alignment), src_layout.pitch_in_block);
			}
			break;
		}

		default:
			fmt::throw_exception("Wrong format 0x%x", format);
		}

		if (!word_size)
		{
			return result;
		}

		result.element_size = word_size;
		result.block_length = words_per_block;

		bool require_cpu_swizzle = !caps.supports_hw_deswizzle && is_swizzled;
		bool require_cpu_byteswap = word_size > 1 && !caps.supports_byteswap;

		if (is_swizzled && caps.supports_hw_deswizzle)
		{
			result.require_deswizzle = true;
		}

		if (!require_cpu_byteswap && !require_cpu_swizzle)
		{
			result.require_swap = (word_size > 1);

			if (caps.supports_zero_copy)
			{
				result.require_upload = true;
				result.deferred_cmds = build_transfer_cmds(src_layout.data.data(), word_size * words_per_block, w, h, depth, src_layout.border, dst_pitch_in_block, src_layout.pitch_in_block);
			}
			else if (word_size == 1)
			{
				copy_unmodified_block::copy_mipmap_level(dst_buffer.as_span<u8>(), src_layout.data.as_span<const u8>(), words_per_block, w, h, depth, src_layout.border, dst_pitch_in_block, src_layout.pitch_in_block);
			}
			else if (word_size == 2)
			{
				copy_unmodified_block::copy_mipmap_level(dst_buffer.as_span<u16>(), src_layout.data.as_span<const u16>(), words_per_block, w, h, depth, src_layout.border, dst_pitch_in_block, src_layout.pitch_in_block);
			}
			else if (word_size == 4)
			{
				copy_unmodified_block::copy_mipmap_level(dst_buffer.as_span<u32>(), src_layout.data.as_span<const u32>(), words_per_block, w, h, depth, src_layout.border, dst_pitch_in_block, src_layout.pitch_in_block);
			}

			return result;
		}

		if (word_size == 1)
		{
			ensure(is_swizzled);
			copy_unmodified_block_swizzled::copy_mipmap_level(dst_buffer.as_span<u8>(), src_layout.data.as_span<const u8>(), words_per_block, w, h, depth, src_layout.border, dst_pitch_in_block);
		}
		else if (word_size == 2)
		{
			if (is_swizzled)
				copy_unmodified_block_swizzled::copy_mipmap_level(dst_buffer.as_span<u16>(), src_layout.data.as_span<const be_t<u16>>(), words_per_block, w, h, depth, src_layout.border, dst_pitch_in_block);
			else
				copy_unmodified_block::copy_mipmap_level(dst_buffer.as_span<u16>(), src_layout.data.as_span<const be_t<u16>>(), words_per_block, w, h, depth, src_layout.border, dst_pitch_in_block, src_layout.pitch_in_block);
		}
		else if (word_size == 4)
		{
			if (is_swizzled)
				copy_unmodified_block_swizzled::copy_mipmap_level(dst_buffer.as_span<u32>(), src_layout.data.as_span<const be_t<u32>>(), words_per_block, w, h, depth, src_layout.border, dst_pitch_in_block);
			else
				copy_unmodified_block::copy_mipmap_level(dst_buffer.as_span<u32>(), src_layout.data.as_span<const be_t<u32>>(), words_per_block, w, h, depth, src_layout.border, dst_pitch_in_block, src_layout.pitch_in_block);
		}

		return result;
	}

	bool is_compressed_host_format(const texture_uploader_capabilities& caps, u32 texture_format)
	{
		switch (texture_format)
		{
		case CELL_GCM_TEXTURE_B8:
		case CELL_GCM_TEXTURE_A1R5G5B5:
		case CELL_GCM_TEXTURE_A4R4G4B4:
		case CELL_GCM_TEXTURE_R5G6B5:
		case CELL_GCM_TEXTURE_A8R8G8B8:
		case CELL_GCM_TEXTURE_G8B8:
		case CELL_GCM_TEXTURE_R6G5B5:
		case CELL_GCM_TEXTURE_DEPTH24_D8:
		case CELL_GCM_TEXTURE_DEPTH24_D8_FLOAT:
		case CELL_GCM_TEXTURE_DEPTH16:
		case CELL_GCM_TEXTURE_DEPTH16_FLOAT:
		case CELL_GCM_TEXTURE_X16:
		case CELL_GCM_TEXTURE_Y16_X16:
		case CELL_GCM_TEXTURE_R5G5B5A1:
		case CELL_GCM_TEXTURE_W16_Z16_Y16_X16_FLOAT:
		case CELL_GCM_TEXTURE_W32_Z32_Y32_X32_FLOAT:
		case CELL_GCM_TEXTURE_X32_FLOAT:
		case CELL_GCM_TEXTURE_D1R5G5B5:
		case CELL_GCM_TEXTURE_D8R8G8B8:
		case CELL_GCM_TEXTURE_Y16_X16_FLOAT:
		// The following formats are compressed in RSX/GCM but not on the host device.
		// They are decompressed in sw before uploading
		case CELL_GCM_TEXTURE_COMPRESSED_HILO8:
		case CELL_GCM_TEXTURE_COMPRESSED_HILO_S8:
		case CELL_GCM_TEXTURE_COMPRESSED_B8R8_G8R8:
		case CELL_GCM_TEXTURE_COMPRESSED_R8B8_R8G8:
			return false;
		// True compressed formats on the host device
		case CELL_GCM_TEXTURE_COMPRESSED_DXT1:
		case CELL_GCM_TEXTURE_COMPRESSED_DXT23:
		case CELL_GCM_TEXTURE_COMPRESSED_DXT45:
			return caps.supports_dxt;
		}
		fmt::throw_exception("Unknown format 0x%x", texture_format);
	}

	bool is_int8_remapped_format(u32 format)
	{
		switch (format)
		{
		case CELL_GCM_TEXTURE_DEPTH24_D8:
		case CELL_GCM_TEXTURE_DEPTH24_D8_FLOAT:
		case CELL_GCM_TEXTURE_DEPTH16:
		case CELL_GCM_TEXTURE_DEPTH16_FLOAT:
		case CELL_GCM_TEXTURE_X16:
		case CELL_GCM_TEXTURE_Y16_X16:
		case CELL_GCM_TEXTURE_COMPRESSED_HILO8:
		case CELL_GCM_TEXTURE_COMPRESSED_HILO_S8:
		case CELL_GCM_TEXTURE_W16_Z16_Y16_X16_FLOAT:
		case CELL_GCM_TEXTURE_W32_Z32_Y32_X32_FLOAT:
		case CELL_GCM_TEXTURE_X32_FLOAT:
		case CELL_GCM_TEXTURE_Y16_X16_FLOAT:
			// NOTE: Special data formats (XY, HILO, DEPTH) are not RGB formats
			return false;
		default:
			return true;
		}
	}

	/**
	 * A texture is stored as an array of blocks, where a block is a pixel for standard texture
	 * but is a structure containing several pixels for compressed format
	 */
	u8 get_format_block_size_in_bytes(int format)
	{
		switch (format)
		{
		case CELL_GCM_TEXTURE_B8: return 1;
		case CELL_GCM_TEXTURE_X16:
		case CELL_GCM_TEXTURE_G8B8:
		case CELL_GCM_TEXTURE_R6G5B5:
		case CELL_GCM_TEXTURE_R5G6B5:
		case CELL_GCM_TEXTURE_D1R5G5B5:
		case CELL_GCM_TEXTURE_R5G5B5A1:
		case CELL_GCM_TEXTURE_A1R5G5B5:
		case CELL_GCM_TEXTURE_A4R4G4B4:
		case CELL_GCM_TEXTURE_DEPTH16:
		case CELL_GCM_TEXTURE_DEPTH16_FLOAT:
		case CELL_GCM_TEXTURE_COMPRESSED_HILO8:
		case CELL_GCM_TEXTURE_COMPRESSED_HILO_S8: return 2;
		case CELL_GCM_TEXTURE_A8R8G8B8:
		case CELL_GCM_TEXTURE_D8R8G8B8:
		case CELL_GCM_TEXTURE_DEPTH24_D8:
		case CELL_GCM_TEXTURE_DEPTH24_D8_FLOAT:
		case CELL_GCM_TEXTURE_X32_FLOAT:
		case CELL_GCM_TEXTURE_Y16_X16:
		case CELL_GCM_TEXTURE_Y16_X16_FLOAT:
		case CELL_GCM_TEXTURE_COMPRESSED_B8R8_G8R8:
		case CELL_GCM_TEXTURE_COMPRESSED_R8B8_R8G8: return 4;
		case CELL_GCM_TEXTURE_COMPRESSED_DXT1:
		case CELL_GCM_TEXTURE_W16_Z16_Y16_X16_FLOAT: return 8;
		case CELL_GCM_TEXTURE_COMPRESSED_DXT23:
		case CELL_GCM_TEXTURE_COMPRESSED_DXT45:
		case CELL_GCM_TEXTURE_W32_Z32_Y32_X32_FLOAT: return 16;
		default:
			rsx_log.error("Unimplemented block size in bytes for texture format: 0x%x", format);
			return 1;
		}
	}

	u8 get_format_block_size_in_texel(int format)
	{
		switch (format)
		{
		case CELL_GCM_TEXTURE_B8:
		case CELL_GCM_TEXTURE_G8B8:
		case CELL_GCM_TEXTURE_D8R8G8B8:
		case CELL_GCM_TEXTURE_D1R5G5B5:
		case CELL_GCM_TEXTURE_A1R5G5B5:
		case CELL_GCM_TEXTURE_A4R4G4B4:
		case CELL_GCM_TEXTURE_A8R8G8B8:
		case CELL_GCM_TEXTURE_R5G5B5A1:
		case CELL_GCM_TEXTURE_R6G5B5:
		case CELL_GCM_TEXTURE_R5G6B5:
		case CELL_GCM_TEXTURE_DEPTH24_D8:
		case CELL_GCM_TEXTURE_DEPTH24_D8_FLOAT:
		case CELL_GCM_TEXTURE_DEPTH16:
		case CELL_GCM_TEXTURE_DEPTH16_FLOAT:
		case CELL_GCM_TEXTURE_X16:
		case CELL_GCM_TEXTURE_Y16_X16:
		case CELL_GCM_TEXTURE_Y16_X16_FLOAT:
		case CELL_GCM_TEXTURE_W16_Z16_Y16_X16_FLOAT:
		case CELL_GCM_TEXTURE_W32_Z32_Y32_X32_FLOAT:
		case CELL_GCM_TEXTURE_X32_FLOAT:
		case CELL_GCM_TEXTURE_COMPRESSED_HILO8:
		case CELL_GCM_TEXTURE_COMPRESSED_HILO_S8: return 1;
		case CELL_GCM_TEXTURE_COMPRESSED_B8R8_G8R8:
		case CELL_GCM_TEXTURE_COMPRESSED_R8B8_R8G8: return 2;
		case CELL_GCM_TEXTURE_COMPRESSED_DXT1:
		case CELL_GCM_TEXTURE_COMPRESSED_DXT23:
		case CELL_GCM_TEXTURE_COMPRESSED_DXT45: return 4;
		default:
			rsx_log.error("Unimplemented block size in texels for texture format: 0x%x", format);
			return 1;
		}
	}

	u8 get_format_block_size_in_bytes(rsx::surface_color_format format)
	{
		switch (format)
		{
		case rsx::surface_color_format::b8:
			return 1;
		case rsx::surface_color_format::g8b8:
		case rsx::surface_color_format::r5g6b5:
		case rsx::surface_color_format::x1r5g5b5_o1r5g5b5:
		case rsx::surface_color_format::x1r5g5b5_z1r5g5b5:
			return 2;
		case rsx::surface_color_format::a8b8g8r8:
		case rsx::surface_color_format::a8r8g8b8:
		case rsx::surface_color_format::x8b8g8r8_o8b8g8r8:
		case rsx::surface_color_format::x8b8g8r8_z8b8g8r8:
		case rsx::surface_color_format::x8r8g8b8_o8r8g8b8:
		case rsx::surface_color_format::x8r8g8b8_z8r8g8b8:
		case rsx::surface_color_format::x32:
			return 4;
		case rsx::surface_color_format::w16z16y16x16:
			return 8;
		case rsx::surface_color_format::w32z32y32x32:
			return 16;
		default:
			fmt::throw_exception("Invalid color format 0x%x", static_cast<u32>(format));
		}
	}

	u8 get_format_block_size_in_bytes(rsx::surface_depth_format2 format)
	{
		switch (format)
		{
		case rsx::surface_depth_format2::z24s8_uint:
		case rsx::surface_depth_format2::z24s8_float:
			return 4;
		default:
			return 2;
		}
	}

	u8 get_format_sample_count(rsx::surface_antialiasing antialias)
	{
		switch (antialias)
		{
		case rsx::surface_antialiasing::center_1_sample:
			return 1;
		case rsx::surface_antialiasing::diagonal_centered_2_samples:
			return 2;
		case rsx::surface_antialiasing::square_centered_4_samples:
		case rsx::surface_antialiasing::square_rotated_4_samples:
			return 4;
		default:
			fmt::throw_exception("Unreachable");
		}
	}

	bool is_depth_stencil_format(rsx::surface_depth_format2 format)
	{
		switch (format)
		{
		case rsx::surface_depth_format2::z24s8_uint:
		case rsx::surface_depth_format2::z24s8_float:
			return true;
		default:
			return false;
		}
	}

	/**
	 * Returns number of texel lines decoded in one pitch-length number of bytes
	 */
	u8 get_format_texel_rows_per_line(u32 format)
	{
		switch (format)
		{
		case CELL_GCM_TEXTURE_COMPRESSED_DXT1:
		case CELL_GCM_TEXTURE_COMPRESSED_DXT23:
		case CELL_GCM_TEXTURE_COMPRESSED_DXT45:
			// Layout is 4x4 blocks, i.e one row of pitch bytes in length actually encodes 4 texel rows
			return 4;
		default:
			return 1;
		}
	}

	u32 get_format_packed_pitch(u32 format, u16 width, bool border, bool swizzled)
	{
		const auto texels_per_block = get_format_block_size_in_texel(format);
		const auto bytes_per_block = get_format_block_size_in_bytes(format);

		auto width_in_block = ((width + texels_per_block - 1) / texels_per_block);
		if (border)
		{
			width_in_block = swizzled ? rsx::next_pow2(width_in_block + 8) :
				width_in_block + 2;
		}

		return width_in_block * bytes_per_block;
	}

	usz get_placed_texture_storage_size(u16 width, u16 height, u32 depth, u8 format, u16 mipmap, bool cubemap, usz row_pitch_alignment, usz mipmap_alignment)
	{
		format &= ~(CELL_GCM_TEXTURE_LN | CELL_GCM_TEXTURE_UN);
		usz block_edge = get_format_block_size_in_texel(format);
		usz block_size_in_byte = get_format_block_size_in_bytes(format);

		usz height_in_blocks = (height + block_edge - 1) / block_edge;
		usz width_in_blocks = (width + block_edge - 1) / block_edge;

		usz result = 0;
		for (u16 i = 0; i < mipmap; ++i)
		{
			usz rowPitch = utils::align(block_size_in_byte * width_in_blocks, row_pitch_alignment);
			result += utils::align(rowPitch * height_in_blocks * depth, mipmap_alignment);
			height_in_blocks = std::max<usz>(height_in_blocks / 2, 1);
			width_in_blocks = std::max<usz>(width_in_blocks / 2, 1);
		}

		// Mipmap, height and width aren't allowed to be zero
		return (ensure(result) * (cubemap ? 6 : 1));
	}

	usz get_placed_texture_storage_size(const rsx::fragment_texture& texture, usz row_pitch_alignment, usz mipmap_alignment)
	{
		return get_placed_texture_storage_size(texture.width(), texture.height(), texture.depth(), texture.format(), texture.mipmap(), texture.cubemap(),
			row_pitch_alignment, mipmap_alignment);
	}

	usz get_placed_texture_storage_size(const rsx::vertex_texture& texture, usz row_pitch_alignment, usz mipmap_alignment)
	{
		return get_placed_texture_storage_size(texture.width(), texture.height(), texture.depth(), texture.format(), texture.mipmap(), texture.cubemap(),
			row_pitch_alignment, mipmap_alignment);
	}

	static usz get_texture_size(u32 format, u16 width, u16 height, u16 depth, u32 pitch, u16 mipmaps, u16 layers, u8 border)
	{
		const auto gcm_format = format & ~(CELL_GCM_TEXTURE_LN | CELL_GCM_TEXTURE_UN);
		const bool packed = !(format & CELL_GCM_TEXTURE_LN);
		const auto texel_rows_per_line = get_format_texel_rows_per_line(gcm_format);

		if (!pitch && !packed)
		{
			if (width > 1 || height > 1)
			{
				// If width == 1, the scanning just returns texel 0, so it is a valid setup
				rsx_log.warning("Invalid texture pitch setup, width=%d, height=%d, format=0x%x(0x%x)",
					width, height, format, gcm_format);
			}

			pitch = get_format_packed_pitch(gcm_format, width, !!border, packed);
		}

		u32 size = 0;
		if (!packed)
		{
			// Constant pitch layout, simple scanning
			const u32 internal_height = (height + texel_rows_per_line - 1) / texel_rows_per_line;  // Convert texels to blocks
			for (u32 layer = 0; layer < layers; ++layer)
			{
				u32 mip_height = internal_height;
				for (u32 mipmap = 0; mipmap < mipmaps && mip_height > 0; ++mipmap)
				{
					size += pitch * mip_height * depth;
					mip_height = std::max(mip_height / 2u, 1u);
				}
			}
		}
		else
		{
			// Variable pitch per mipmap level
			const auto texels_per_block = get_format_block_size_in_texel(gcm_format);
			const auto bytes_per_block = get_format_block_size_in_bytes(gcm_format);

			const u32 internal_height = (height + texel_rows_per_line - 1) / texel_rows_per_line;  // Convert texels to blocks
			const u32 internal_width = (width + texels_per_block - 1) / texels_per_block;          // Convert texels to blocks
			for (u32 layer = 0; layer < layers; ++layer)
			{
				u32 mip_height = internal_height;
				u32 mip_width = internal_width;
				for (u32 mipmap = 0; mipmap < mipmaps && mip_height > 0; ++mipmap)
				{
					size += (mip_width * bytes_per_block * mip_height * depth);
					mip_height = std::max(mip_height / 2u, 1u);
					mip_width = std::max(mip_width / 2u, 1u);
				}
			}
		}

		return size;
	}

	usz get_texture_size(const rsx::fragment_texture& texture)
	{
		return get_texture_size(texture.format(), texture.width(), texture.height(), texture.depth(),
			texture.pitch(), texture.get_exact_mipmap_count(), texture.cubemap() ? 6 : 1,
			texture.border_type() ^ 1);
	}

	usz get_texture_size(const rsx::vertex_texture& texture)
	{
		return get_texture_size(texture.format(), texture.width(), texture.height(), texture.depth(),
			texture.pitch(), texture.get_exact_mipmap_count(), texture.cubemap() ? 6 : 1,
			texture.border_type() ^ 1);
	}

	u32 get_remap_encoding(const texture_channel_remap_t& remap)
	{
		u32 encode = 0;
		encode |= (remap.channel_map[0] << 0);
		encode |= (remap.channel_map[1] << 2);
		encode |= (remap.channel_map[2] << 4);
		encode |= (remap.channel_map[3] << 6);

		encode |= (remap.control_map[0] << 8);
		encode |= (remap.control_map[1] << 10);
		encode |= (remap.control_map[2] << 12);
		encode |= (remap.control_map[3] << 14);
		return encode;
	}

	std::pair<u32, bool> get_compatible_gcm_format(rsx::surface_color_format format)
	{
		switch (format)
		{
		case rsx::surface_color_format::r5g6b5:
			return{ CELL_GCM_TEXTURE_R5G6B5, false };

		case rsx::surface_color_format::x8r8g8b8_z8r8g8b8:
		case rsx::surface_color_format::x8r8g8b8_o8r8g8b8:
		case rsx::surface_color_format::a8r8g8b8:
			return{ CELL_GCM_TEXTURE_A8R8G8B8, true }; //verified

		case rsx::surface_color_format::x8b8g8r8_o8b8g8r8:
		case rsx::surface_color_format::x8b8g8r8_z8b8g8r8:
		case rsx::surface_color_format::a8b8g8r8:
			return{ CELL_GCM_TEXTURE_A8R8G8B8, true };

		case rsx::surface_color_format::w16z16y16x16:
			return{ CELL_GCM_TEXTURE_W16_Z16_Y16_X16_FLOAT, true };

		case rsx::surface_color_format::w32z32y32x32:
			return{ CELL_GCM_TEXTURE_W32_Z32_Y32_X32_FLOAT, true };

		case rsx::surface_color_format::x1r5g5b5_o1r5g5b5:
		case rsx::surface_color_format::x1r5g5b5_z1r5g5b5:
			return{ CELL_GCM_TEXTURE_A1R5G5B5, false };

		case rsx::surface_color_format::b8:
			return{ CELL_GCM_TEXTURE_B8, false };

		case rsx::surface_color_format::g8b8:
			return{ CELL_GCM_TEXTURE_G8B8, true };

		case rsx::surface_color_format::x32:
			return{ CELL_GCM_TEXTURE_X32_FLOAT, true }; //verified
		default:
			fmt::throw_exception("Unhandled surface format 0x%x", static_cast<u32>(format));
		}
	}

	std::pair<u32, bool> get_compatible_gcm_format(rsx::surface_depth_format2 format)
	{
		switch (format)
		{
		case rsx::surface_depth_format2::z16_uint:
			return{ CELL_GCM_TEXTURE_DEPTH16, true };
		case rsx::surface_depth_format2::z24s8_uint:
			return{ CELL_GCM_TEXTURE_DEPTH24_D8, true };
		case rsx::surface_depth_format2::z16_float:
			return{ CELL_GCM_TEXTURE_DEPTH16_FLOAT, true };
		case rsx::surface_depth_format2::z24s8_float:
			return{ CELL_GCM_TEXTURE_DEPTH24_D8_FLOAT, true };
		default:
			fmt::throw_exception("Unreachable");
		}
	}

	rsx::format_class classify_format(rsx::surface_depth_format2 format)
	{
		switch (format)
		{
		case rsx::surface_depth_format2::z16_uint:
			return RSX_FORMAT_CLASS_DEPTH16_UNORM;
		case rsx::surface_depth_format2::z24s8_uint:
			return RSX_FORMAT_CLASS_DEPTH24_UNORM_X8_PACK32;
		case rsx::surface_depth_format2::z16_float:
			return RSX_FORMAT_CLASS_DEPTH16_FLOAT;
		case rsx::surface_depth_format2::z24s8_float:
			return RSX_FORMAT_CLASS_DEPTH24_FLOAT_X8_PACK32;
		default:
			return RSX_FORMAT_CLASS_COLOR;
		}
	}

	rsx::format_class classify_format(u32 gcm_format)
	{
		switch (gcm_format)
		{
		case CELL_GCM_TEXTURE_DEPTH16:
			return RSX_FORMAT_CLASS_DEPTH16_UNORM;
		case CELL_GCM_TEXTURE_DEPTH16_FLOAT:
			return RSX_FORMAT_CLASS_DEPTH16_FLOAT;
		case CELL_GCM_TEXTURE_DEPTH24_D8:
			return RSX_FORMAT_CLASS_DEPTH24_UNORM_X8_PACK32;
		case CELL_GCM_TEXTURE_DEPTH24_D8_FLOAT:
			return RSX_FORMAT_CLASS_DEPTH24_FLOAT_X8_PACK32;
		default:
			return RSX_FORMAT_CLASS_COLOR;
		}
	}

	u32 get_max_depth_value(rsx::surface_depth_format2 format)
	{
		return get_format_block_size_in_bytes(format) == 2 ? 0xFFFF : 0xFFFFFF;
	}

	bool is_texcoord_wrapping_mode(rsx::texture_wrap_mode mode)
	{
		switch (mode)
		{
			// Clamping modes
			default:
				rsx_log.error("Unknown texture wrap mode: %d", static_cast<int>(mode));
				[[ fallthrough ]];
			case rsx::texture_wrap_mode::border:
			case rsx::texture_wrap_mode::clamp:
			case rsx::texture_wrap_mode::clamp_to_edge:
			case rsx::texture_wrap_mode::mirror_once_clamp_to_edge:
			case rsx::texture_wrap_mode::mirror_once_border:
			case rsx::texture_wrap_mode::mirror_once_clamp:
				return false;
			// Wrapping modes
			case rsx::texture_wrap_mode::wrap:
			case rsx::texture_wrap_mode::mirror:
				return true;
		}
	}

	bool is_border_clamped_texture(
		rsx::texture_wrap_mode wrap_s,
		rsx::texture_wrap_mode wrap_t,
		rsx::texture_wrap_mode wrap_r,
		rsx::texture_dimension dimension)
	{
		// Technically we should check border and mirror_once_border
		// However, the latter is not implemented in any modern API, so we can just ignore it (emulated with mirror_once_clamp).
		switch (dimension)
		{
		case rsx::texture_dimension::dimension1d:
			return wrap_s == rsx::texture_wrap_mode::border;
		case rsx::texture_dimension::dimension2d:
			return wrap_s == rsx::texture_wrap_mode::border || wrap_t == rsx::texture_wrap_mode::border;
		case rsx::texture_dimension::dimension3d:
			return wrap_s == rsx::texture_wrap_mode::border || wrap_t == rsx::texture_wrap_mode::border || wrap_r == rsx::texture_wrap_mode::border;
		default:
			return false;
		}
	}
}
