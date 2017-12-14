#include "stdafx.h"
#include "Emu/Memory/vm.h"
#include "TextureUtils.h"
#include "../RSXThread.h"
#include "../rsx_utils.h"


namespace
{
	// FIXME: GSL as_span break build if template parameter is non const with current revision.
	// Replace with true as_span when fixed.
	template <typename T>
	gsl::span<T> as_span_workaround(gsl::span<gsl::byte> unformated_span)
	{
		return{ (T*)unformated_span.data(), ::narrow<int>(unformated_span.size_bytes() / sizeof(T)) };
	}

	// TODO: Make this function part of GSL
	// Note: Doesn't handle overlapping range detection.
	template<typename T1, typename T2>
	constexpr void copy(gsl::span<T1> dst, gsl::span<T2> src)
	{
		static_assert(std::is_convertible<T1, T2>::value, "Cannot convert source and destination span type.");
		verify(HERE), (dst.size() == src.size());
		std::copy(src.begin(), src.end(), dst.begin());
	}

struct copy_unmodified_block
{
	template<typename T, typename U>
	static void copy_mipmap_level(gsl::span<T> dst, gsl::span<const U> src, u16 width_in_block, u16 row_count, u16 depth, u32 dst_pitch_in_block, u32 src_pitch_in_block)
	{
		size_t row_element_count = dst_pitch_in_block;
		static_assert(sizeof(T) == sizeof(U), "Type size doesn't match.");
		for (int row = 0; row < row_count * depth; ++row)
			copy(dst.subspan(row * dst_pitch_in_block, width_in_block), src.subspan(row * src_pitch_in_block, width_in_block));
	}
};

struct copy_unmodified_block_swizzled
{
	template<typename T, typename U>
	static void copy_mipmap_level(gsl::span<T> dst, gsl::span<const U> src, u16 width_in_block, u16 row_count, u16 depth, u32 dst_pitch_in_block)
	{
		std::unique_ptr<U[]> temp_swizzled(new U[width_in_block * row_count]);
		for (int d = 0; d < depth; ++d)
		{
			rsx::convert_linear_swizzle<U>((void*)src.subspan(d * width_in_block * row_count).data(), temp_swizzled.get(), width_in_block, row_count, true);
			gsl::span<const U> swizzled_src{ temp_swizzled.get(), ::narrow<int>(width_in_block * row_count) };
			for (int row = 0; row < row_count; ++row)
				copy(dst.subspan((row + d * row_count) * dst_pitch_in_block, width_in_block), swizzled_src.subspan(row * width_in_block, width_in_block));
		}
	}
};

namespace
{
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
	std::vector<rsx_subresource_layout> get_subresources_layout_impl(const gsl::byte *texture_data_pointer, u16 width_in_texel, u16 height_in_texel, u16 depth, u8 layer_count, u16 mipmap_count, u32 suggested_pitch_in_bytes, bool padded_row)
	{
		/**
		* Note about size type: RSX texture width is stored in a 16 bits int and pitch is stored in a 20 bits int.
		*/

		// <= 128 so fits in u8
		u8 block_size_in_bytes = sizeof(SRC_TYPE);

		std::vector<rsx_subresource_layout> result;
		size_t offset_in_src = 0;
		// Always lower than width/height so fits in u16
		u16 texture_height_in_block = (height_in_texel + block_edge_in_texel - 1) / block_edge_in_texel;
		u16 texture_width_in_block = (width_in_texel + block_edge_in_texel - 1) / block_edge_in_texel;
		for (unsigned layer = 0; layer < layer_count; layer++)
		{
			u16 miplevel_height_in_block = texture_height_in_block, miplevel_width_in_block = texture_width_in_block;
			for (unsigned mip_level = 0; mip_level < mipmap_count; mip_level++)
			{
				rsx_subresource_layout current_subresource_layout = {};
				// Since <= width/height, fits on 16 bits
				current_subresource_layout.height_in_block = miplevel_height_in_block;
				current_subresource_layout.width_in_block = miplevel_width_in_block;
				current_subresource_layout.depth = depth;
				// src_pitch in texture can uses 20 bits so fits on 32 bits int.
				u32 src_pitch_in_block = padded_row ? suggested_pitch_in_bytes / block_size_in_bytes : miplevel_width_in_block;
				current_subresource_layout.pitch_in_bytes = src_pitch_in_block;

				current_subresource_layout.data = gsl::span<const gsl::byte>(texture_data_pointer + offset_in_src, src_pitch_in_block * block_size_in_bytes * miplevel_height_in_block * depth);

				result.push_back(current_subresource_layout);
				offset_in_src += miplevel_height_in_block * src_pitch_in_block * block_size_in_bytes * depth;
				miplevel_height_in_block = std::max(miplevel_height_in_block / 2, 1);
				miplevel_width_in_block = std::max(miplevel_width_in_block / 2, 1);
			}
			offset_in_src = align(offset_in_src, 128);
		}
		return result;
	}
}

template<typename T>
u32 get_row_pitch_in_block(u16 width_in_block, size_t multiple_constraints_in_byte)
{
	size_t divided = (width_in_block * sizeof(T) + multiple_constraints_in_byte - 1) / multiple_constraints_in_byte;
	return static_cast<u32>(divided * multiple_constraints_in_byte / sizeof(T));
}

/**
 * Since rsx ignore unused dimensionnality some app set them to 0.
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
	fmt::throw_exception("Unsupported texture dimension" HERE);
}
}

template<typename RsxTextureType>
std::vector<rsx_subresource_layout> get_subresources_layout_impl(const RsxTextureType &texture)
{
	u16 w = texture.width();
	u16 h;
	u16 depth;
	u8 layer;

	std::tie(h, depth, layer) = get_height_depth_layer(texture);

	int format = texture.format() & ~(CELL_GCM_TEXTURE_LN | CELL_GCM_TEXTURE_UN);

	const u32 texaddr = rsx::get_address(texture.offset(), texture.location());
	auto pixels = reinterpret_cast<const gsl::byte*>(vm::ps3::_ptr<const u8>(texaddr));
	bool is_swizzled = !(texture.format() & CELL_GCM_TEXTURE_LN);
	switch (format)
	{
	case CELL_GCM_TEXTURE_B8:
		return get_subresources_layout_impl<1, u8>(pixels, w, h, depth, layer, texture.get_exact_mipmap_count(), texture.pitch(), !is_swizzled);
	case ~(CELL_GCM_TEXTURE_LN | CELL_GCM_TEXTURE_UN) & CELL_GCM_TEXTURE_COMPRESSED_B8R8_G8R8:
	case ~(CELL_GCM_TEXTURE_LN | CELL_GCM_TEXTURE_UN) & CELL_GCM_TEXTURE_COMPRESSED_R8B8_R8G8:
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
		return get_subresources_layout_impl<1, u16>(pixels, w, h, depth, layer, texture.get_exact_mipmap_count(), texture.pitch(), !is_swizzled);
	case CELL_GCM_TEXTURE_DEPTH24_D8: // Untested
	case CELL_GCM_TEXTURE_DEPTH24_D8_FLOAT: // Untested
	case CELL_GCM_TEXTURE_D8R8G8B8:
	case CELL_GCM_TEXTURE_A8R8G8B8:
	case CELL_GCM_TEXTURE_Y16_X16:
	case CELL_GCM_TEXTURE_Y16_X16_FLOAT:
	case CELL_GCM_TEXTURE_X32_FLOAT:
		return get_subresources_layout_impl<1, u32>(pixels, w, h, depth, layer, texture.get_exact_mipmap_count(), texture.pitch(), !is_swizzled);
	case CELL_GCM_TEXTURE_W16_Z16_Y16_X16_FLOAT:
		return get_subresources_layout_impl<1, u64>(pixels, w, h, depth, layer, texture.get_exact_mipmap_count(), texture.pitch(), !is_swizzled);
	case CELL_GCM_TEXTURE_W32_Z32_Y32_X32_FLOAT:
		return get_subresources_layout_impl<1, u128>(pixels, w, h, depth, layer, texture.get_exact_mipmap_count(), texture.pitch(), !is_swizzled);
	case CELL_GCM_TEXTURE_COMPRESSED_DXT1:
		return get_subresources_layout_impl<4, u64>(pixels, w, h, depth, layer, texture.get_exact_mipmap_count(), texture.pitch(), !is_swizzled);
	case CELL_GCM_TEXTURE_COMPRESSED_DXT23:
	case CELL_GCM_TEXTURE_COMPRESSED_DXT45:
		return get_subresources_layout_impl<4, u128>(pixels, w, h, depth, layer, texture.get_exact_mipmap_count(), texture.pitch(), !is_swizzled);
	}
	fmt::throw_exception("Wrong format 0x%x" HERE, format);
}

std::vector<rsx_subresource_layout> get_subresources_layout(const rsx::fragment_texture &texture)
{
	return get_subresources_layout_impl(texture);
}

std::vector<rsx_subresource_layout> get_subresources_layout(const rsx::vertex_texture &texture)
{
	return get_subresources_layout_impl(texture);
}

void upload_texture_subresource(gsl::span<gsl::byte> dst_buffer, const rsx_subresource_layout &src_layout, int format, bool is_swizzled, size_t dst_row_pitch_multiple_of)
{
	u16 w = src_layout.width_in_block;
	u16 h = src_layout.height_in_block;
	u16 depth = src_layout.depth;
	u32 pitch = src_layout.pitch_in_bytes;

	// Ignore when texture width > pitch
	if (w > pitch)
		return;
		
	switch (format)
	{
	case CELL_GCM_TEXTURE_B8:
	{
		if (is_swizzled)
			copy_unmodified_block_swizzled::copy_mipmap_level(as_span_workaround<u8>(dst_buffer), gsl::as_span<const u8>(src_layout.data), w, h, depth, get_row_pitch_in_block<u8>(w, dst_row_pitch_multiple_of));
		else
			copy_unmodified_block::copy_mipmap_level(as_span_workaround<u8>(dst_buffer), gsl::as_span<const u8>(src_layout.data), w, h, depth, get_row_pitch_in_block<u8>(w, dst_row_pitch_multiple_of), src_layout.pitch_in_bytes);
		break;
	}

	case ~(CELL_GCM_TEXTURE_LN | CELL_GCM_TEXTURE_UN) & CELL_GCM_TEXTURE_COMPRESSED_B8R8_G8R8:
	case ~(CELL_GCM_TEXTURE_LN | CELL_GCM_TEXTURE_UN) & CELL_GCM_TEXTURE_COMPRESSED_R8B8_R8G8:
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
	{
		if (is_swizzled)
			copy_unmodified_block_swizzled::copy_mipmap_level(as_span_workaround<u16>(dst_buffer), gsl::as_span<const be_t<u16>>(src_layout.data), w, h, depth, get_row_pitch_in_block<u16>(w, dst_row_pitch_multiple_of));
		else
			copy_unmodified_block::copy_mipmap_level(as_span_workaround<u16>(dst_buffer), gsl::as_span<const be_t<u16>>(src_layout.data), w, h, depth, get_row_pitch_in_block<u16>(w, dst_row_pitch_multiple_of), src_layout.pitch_in_bytes);
		break;
	}

	case CELL_GCM_TEXTURE_DEPTH24_D8: // Untested
	case CELL_GCM_TEXTURE_DEPTH24_D8_FLOAT: // Untested
	case CELL_GCM_TEXTURE_A8R8G8B8:
	case CELL_GCM_TEXTURE_D8R8G8B8:
	{
		if (is_swizzled)
			copy_unmodified_block_swizzled::copy_mipmap_level(as_span_workaround<u32>(dst_buffer), gsl::as_span<const u32>(src_layout.data), w, h, depth, get_row_pitch_in_block<u32>(w, dst_row_pitch_multiple_of));
		else
			copy_unmodified_block::copy_mipmap_level(as_span_workaround<u32>(dst_buffer), gsl::as_span<const u32>(src_layout.data), w, h, depth, get_row_pitch_in_block<u32>(w, dst_row_pitch_multiple_of), src_layout.pitch_in_bytes);
		break;
	}

	case CELL_GCM_TEXTURE_Y16_X16:
	case CELL_GCM_TEXTURE_Y16_X16_FLOAT:
	case CELL_GCM_TEXTURE_X32_FLOAT:
		copy_unmodified_block::copy_mipmap_level(as_span_workaround<u32>(dst_buffer), gsl::as_span<const be_t<u32>>(src_layout.data), w, h, depth, get_row_pitch_in_block<u32>(w, dst_row_pitch_multiple_of), src_layout.pitch_in_bytes);
		break;

	case CELL_GCM_TEXTURE_W16_Z16_Y16_X16_FLOAT:
		copy_unmodified_block::copy_mipmap_level(as_span_workaround<u64>(dst_buffer), gsl::as_span<const be_t<u64>>(src_layout.data), w, h, depth, get_row_pitch_in_block<u64>(w, dst_row_pitch_multiple_of), src_layout.pitch_in_bytes);
		break;

	case CELL_GCM_TEXTURE_W32_Z32_Y32_X32_FLOAT:
		copy_unmodified_block::copy_mipmap_level(as_span_workaround<u128>(dst_buffer), gsl::as_span<const be_t<u128>>(src_layout.data), w, h, depth, get_row_pitch_in_block<u128>(w, dst_row_pitch_multiple_of), src_layout.pitch_in_bytes);
		break;

	case CELL_GCM_TEXTURE_COMPRESSED_DXT1:
		copy_unmodified_block::copy_mipmap_level(as_span_workaround<u64>(dst_buffer), gsl::as_span<const u64>(src_layout.data), w, h, depth, get_row_pitch_in_block<u64>(w, dst_row_pitch_multiple_of), src_layout.pitch_in_bytes);
		break;

	case CELL_GCM_TEXTURE_COMPRESSED_DXT23:
	case CELL_GCM_TEXTURE_COMPRESSED_DXT45:
		copy_unmodified_block::copy_mipmap_level(as_span_workaround<u128>(dst_buffer), gsl::as_span<const u128>(src_layout.data), w, h, depth, get_row_pitch_in_block<u128>(w, dst_row_pitch_multiple_of), src_layout.pitch_in_bytes);
		break;

	default:
		fmt::throw_exception("Wrong format 0x%x" HERE, format);
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
	case CELL_GCM_TEXTURE_COMPRESSED_HILO_S8:
	case ~(CELL_GCM_TEXTURE_LN | CELL_GCM_TEXTURE_UN) & CELL_GCM_TEXTURE_COMPRESSED_B8R8_G8R8:
	case ~(CELL_GCM_TEXTURE_LN | CELL_GCM_TEXTURE_UN) & CELL_GCM_TEXTURE_COMPRESSED_R8B8_R8G8: return 2;
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
		LOG_ERROR(RSX, "Unimplemented block size in bytes for texture format: 0x%x", format);
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
	case CELL_GCM_TEXTURE_COMPRESSED_HILO_S8:
	case ~(CELL_GCM_TEXTURE_LN | CELL_GCM_TEXTURE_UN) & CELL_GCM_TEXTURE_COMPRESSED_B8R8_G8R8:
	case ~(CELL_GCM_TEXTURE_LN | CELL_GCM_TEXTURE_UN) & CELL_GCM_TEXTURE_COMPRESSED_R8B8_R8G8: return 1;
	case CELL_GCM_TEXTURE_COMPRESSED_B8R8_G8R8:
	case CELL_GCM_TEXTURE_COMPRESSED_R8B8_R8G8: return 2;
	case CELL_GCM_TEXTURE_COMPRESSED_DXT1:
	case CELL_GCM_TEXTURE_COMPRESSED_DXT23:
	case CELL_GCM_TEXTURE_COMPRESSED_DXT45: return 4;
	default:
		LOG_ERROR(RSX, "Unimplemented block size in texels for texture format: 0x%x", format);
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
		fmt::throw_exception("Invalid color format 0x%x" HERE, (u32)format);
	}
}

size_t get_placed_texture_storage_size(u16 width, u16 height, u32 depth, u8 format, u16 mipmap, bool cubemap, size_t row_pitch_alignement, size_t mipmap_alignment)
{
	size_t w = width;
	size_t h = std::max<u16>(height, 1);
	size_t d = std::max<u16>(depth, 1);

	format &= ~(CELL_GCM_TEXTURE_LN | CELL_GCM_TEXTURE_UN);
	size_t block_edge = get_format_block_size_in_texel(format);
	size_t block_size_in_byte = get_format_block_size_in_bytes(format);

	size_t height_in_blocks = (h + block_edge - 1) / block_edge;
	size_t width_in_blocks = (w + block_edge - 1) / block_edge;

	size_t result = 0;
	for (u16 i = 0; i < mipmap; ++i)
	{
		size_t rowPitch = align(block_size_in_byte * width_in_blocks, row_pitch_alignement);
		result += align(rowPitch * height_in_blocks * d, mipmap_alignment);
		height_in_blocks = std::max<size_t>(height_in_blocks / 2, 1);
		width_in_blocks = std::max<size_t>(width_in_blocks / 2, 1);
	}

	return result * (cubemap ? 6 : 1);
}

size_t get_placed_texture_storage_size(const rsx::fragment_texture &texture, size_t row_pitch_alignement, size_t mipmap_alignment)
{
	return get_placed_texture_storage_size(texture.width(), texture.height(), texture.depth(), texture.format(), texture.mipmap(), texture.cubemap(),
		row_pitch_alignement, mipmap_alignment);
}

size_t get_placed_texture_storage_size(const rsx::vertex_texture &texture, size_t row_pitch_alignement, size_t mipmap_alignment)
{
	return get_placed_texture_storage_size(texture.width(), texture.height(), texture.depth(), texture.format(), texture.mipmap(), texture.cubemap(),
		row_pitch_alignement, mipmap_alignment);
}


static size_t get_texture_size(u32 w, u32 h, u8 format)
{
	format &= ~(CELL_GCM_TEXTURE_LN | CELL_GCM_TEXTURE_UN);

	// TODO: Take mipmaps into account
	switch (format)
	{
	case CELL_GCM_TEXTURE_B8:
		return w * h;
	case CELL_GCM_TEXTURE_G8B8:
		return w * h * 2;
	case CELL_GCM_TEXTURE_R5G5B5A1:
		return w * h * 4;
	case CELL_GCM_TEXTURE_D8R8G8B8:
		return w * h * 4;
	case CELL_GCM_TEXTURE_A8R8G8B8:
		return w * h * 4;
	case CELL_GCM_TEXTURE_D1R5G5B5:
		return w * h * 2;
	case CELL_GCM_TEXTURE_A1R5G5B5:
		return w * h * 2;
	case CELL_GCM_TEXTURE_A4R4G4B4:
		return w * h * 2;
	case CELL_GCM_TEXTURE_R6G5B5:
		return w * h * 2;
	case CELL_GCM_TEXTURE_R5G6B5:
		return w * h * 2;
	case CELL_GCM_TEXTURE_COMPRESSED_DXT1:
		w = align(w, 4);
		h = align(h, 4);
		return w * h / 6;
	case CELL_GCM_TEXTURE_COMPRESSED_DXT23:
		w = align(w, 4);
		h = align(h, 4);
		return w * h / 4;
	case CELL_GCM_TEXTURE_COMPRESSED_DXT45:
		w = align(w, 4);
		h = align(h, 4);
		return w * h / 4;
	case CELL_GCM_TEXTURE_DEPTH16:
		return w * h * 2;
	case CELL_GCM_TEXTURE_DEPTH16_FLOAT:
		return w * h * 2;
	case CELL_GCM_TEXTURE_DEPTH24_D8:
		return w * h * 4;
	case CELL_GCM_TEXTURE_DEPTH24_D8_FLOAT:
		return w * h * 4;
	case CELL_GCM_TEXTURE_X16:
		return w * h * 2;
	case CELL_GCM_TEXTURE_Y16_X16:
		return w * h * 4;
	case CELL_GCM_TEXTURE_Y16_X16_FLOAT:
		return w * h * 4;
	case CELL_GCM_TEXTURE_X32_FLOAT:
		return w * h * 4;
	case CELL_GCM_TEXTURE_W16_Z16_Y16_X16_FLOAT:
		return w * h * 8;
	case CELL_GCM_TEXTURE_W32_Z32_Y32_X32_FLOAT:
		return w * h * 16;
	case CELL_GCM_TEXTURE_COMPRESSED_B8R8_G8R8:
		return w * h * 4;
	case CELL_GCM_TEXTURE_COMPRESSED_R8B8_R8G8:
		return w * h * 4;
	case CELL_GCM_TEXTURE_COMPRESSED_HILO8:
		return w * h;
	case CELL_GCM_TEXTURE_COMPRESSED_HILO_S8:
		return w * h;
	case ~(CELL_GCM_TEXTURE_LN | CELL_GCM_TEXTURE_UN) & CELL_GCM_TEXTURE_COMPRESSED_B8R8_G8R8:
	case ~(CELL_GCM_TEXTURE_LN | CELL_GCM_TEXTURE_UN) & CELL_GCM_TEXTURE_COMPRESSED_R8B8_R8G8:
		return w * h * 2;
	default:
		LOG_ERROR(RSX, "Unimplemented texture size for texture format: 0x%x", format);
		return 0;
	}
}

size_t get_texture_size(const rsx::fragment_texture &texture)
{
	return get_texture_size(texture.width(), texture.height(), texture.format());
}

size_t get_texture_size(const rsx::vertex_texture &texture)
{
	return get_texture_size(texture.width(), texture.height(), texture.format());
}
