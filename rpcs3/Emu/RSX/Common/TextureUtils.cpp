#include "stdafx.h"
#include "Emu/Memory/vm.h"
#include "TextureUtils.h"
#include "../RSXThread.h"
#include "../rsx_utils.h"


#define MAX2(a, b) ((a) > (b)) ? (a) : (b)
namespace
{
	// FIXME: GSL as_span break build if template parameter is non const with current revision.
	// Replace with true as_span when fixed.
	template <typename T>
	gsl::span<T> as_span_workaround(gsl::span<gsl::byte> unformated_span)
	{
		return{ (T*)unformated_span.data(), gsl::narrow<int>(unformated_span.size_bytes() / sizeof(T)) };
	}

	// TODO: Make this function part of GSL
	// Note: Doesn't handle overlapping range detection.
	template<typename T1, typename T2>
	constexpr void copy(gsl::span<T1> dst, gsl::span<T2> src)
	{
		static_assert(std::is_convertible<T1, T2>::value, "Cannot convert source and destination span type.");
		Expects(dst.size() == src.size());
		std::copy(src.begin(), src.end(), dst.begin());
	}

struct copy_unmodified_block
{
	template<typename T, typename U>
	static void copy_mipmap_level(gsl::span<T> dst, const U *src_ptr, u16 row_count, u16 width_in_block, u16 depth, u32 dst_pitch_in_block, u32 src_pitch_in_block)
	{
		size_t row_element_count = dst_pitch_in_block;
		static_assert(sizeof(T) == sizeof(U), "Type size doesn't match.");
		gsl::span<const U> src{ src_ptr, row_count * src_pitch_in_block * depth };
		for (int row = 0; row < row_count * depth; ++row)
			copy(dst.subspan(row * dst_pitch_in_block, width_in_block), src.subspan(row * src_pitch_in_block, width_in_block));
	}
};

struct copy_unmodified_block_swizzled
{
	template<typename T, typename U>
	static void copy_mipmap_level(gsl::span<T> dst, const U *src_ptr, u16 row_count, u16 width_in_block, u16 depth, u32 dst_pitch_in_block, u32)
	{
		std::unique_ptr<U[]> temp_swizzled(new U[width_in_block * row_count]);
		gsl::span<const U> src{ src_ptr, gsl::narrow<int>(width_in_block * row_count * depth) };
		for (int d = 0; d < depth; ++d)
		{
			rsx::convert_linear_swizzle<U>((void*)src.subspan(d * width_in_block * row_count).data(), temp_swizzled.get(), width_in_block, row_count, true);
			gsl::span<const U> swizzled_src{ temp_swizzled.get(), gsl::narrow<int>(width_in_block * row_count) };
			for (int row = 0; row < row_count; ++row)
				copy(dst.subspan((row + d * row_count) * dst_pitch_in_block, width_in_block), swizzled_src.subspan(row * width_in_block, width_in_block));
		}
	}
};

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
 * The alignment is 256 for mipmap levels and 512 for depth (TODO: make this customisable for Vulkan ?)
 * The template takes a struct with a "copy_mipmap_level" static function that copy the given mipmap level and returns the offset to add to the src buffer for next
 * mipmap level (to allow same code for packed/non packed texels)
 * Sometimes texture provides a pitch even if texture is swizzled (and then packed) and in such case it's ignored. It's passed via suggested_pitch and is used only if padded_row is false.
 */
template <typename T, bool padded_row, u8 block_edge_in_texel, typename DST_TYPE, typename SRC_TYPE>
std::vector<MipmapLevelInfo> copy_texture_data(gsl::span<DST_TYPE> dst, const SRC_TYPE *src, u16 width_in_texel, u16 height_in_texel, u16 depth, u8 layer_count, u16 mipmap_count, u32 suggested_pitch_in_bytes)
{
	/**
	 * Note about size type: RSX texture width is stored in a 16 bits int and pitch is stored in a 20 bits int.
	 */

	// <= 128 so fits in u8
	u8 block_size_in_bytes = sizeof(DST_TYPE);

	std::vector<MipmapLevelInfo> Result;
	size_t offsetInDst = 0, offsetInSrc = 0;
	// Always lower than width/height so fits in u16
	u16 texture_height_in_block = (height_in_texel + block_edge_in_texel - 1) / block_edge_in_texel;
	u16 texture_width_in_block = (width_in_texel + block_edge_in_texel - 1) / block_edge_in_texel;
	for (unsigned layer = 0; layer < layer_count; layer++)
	{
		u16 miplevel_height_in_block = texture_height_in_block, miplevel_width_in_block = texture_width_in_block;
		for (unsigned mip_level = 0; mip_level < mipmap_count; mip_level++)
		{
			// since mip_level is up to 16 bits needs at least 17 bits.
			u32 dst_pitch = align(miplevel_width_in_block * block_size_in_bytes, 256) / block_size_in_bytes;

			MipmapLevelInfo currentMipmapLevelInfo = {};
			currentMipmapLevelInfo.offset = offsetInDst;
			// Since <= width/height, fits on 16 bits
			currentMipmapLevelInfo.height = static_cast<u16>(miplevel_height_in_block * block_edge_in_texel);
			currentMipmapLevelInfo.width = static_cast<u16>(miplevel_width_in_block * block_edge_in_texel);
			currentMipmapLevelInfo.depth = depth;
			currentMipmapLevelInfo.rowPitch = static_cast<u32>(dst_pitch * block_size_in_bytes);
			Result.push_back(currentMipmapLevelInfo);

			// TODO: uses src_pitch from texture
			// src_pitch in texture can uses 20 bits so fits on 32 bits int.
			u32 src_pitch_in_block = padded_row ? suggested_pitch_in_bytes / block_size_in_bytes : miplevel_width_in_block;
			const SRC_TYPE *src_with_offset = reinterpret_cast<const SRC_TYPE*>(reinterpret_cast<const char*>(src) + offsetInSrc);
			T::copy_mipmap_level(dst.subspan(offsetInDst / block_size_in_bytes, dst_pitch * depth * miplevel_height_in_block), src_with_offset, miplevel_height_in_block, miplevel_width_in_block, depth, dst_pitch, src_pitch_in_block);
			offsetInSrc += miplevel_height_in_block * src_pitch_in_block * block_size_in_bytes * depth;
			offsetInDst += align(miplevel_height_in_block * dst_pitch * block_size_in_bytes, 512);
			miplevel_height_in_block = MAX2(miplevel_height_in_block / 2, 1);
			miplevel_width_in_block = MAX2(miplevel_width_in_block / 2, 1);
		}
		offsetInSrc = align(offsetInSrc, 128);
	}
	return Result;
}

/**
 * A texture is stored as an array of blocks, where a block is a pixel for standard texture
 * but is a structure containing several pixels for compressed format
 */
size_t get_texture_block_size(u32 format)
{
	switch (format)
	{
	case CELL_GCM_TEXTURE_B8: return 1;
	case CELL_GCM_TEXTURE_A1R5G5B5:
	case CELL_GCM_TEXTURE_A4R4G4B4:
	case CELL_GCM_TEXTURE_R5G6B5: return 2;
	case CELL_GCM_TEXTURE_A8R8G8B8: return 4;
	case CELL_GCM_TEXTURE_COMPRESSED_DXT1: return 8;
	case CELL_GCM_TEXTURE_COMPRESSED_DXT23: return 16;
	case CELL_GCM_TEXTURE_COMPRESSED_DXT45: return 16;
	case CELL_GCM_TEXTURE_G8B8: return 2;
	case CELL_GCM_TEXTURE_R6G5B5:
	case CELL_GCM_TEXTURE_DEPTH24_D8:
	case CELL_GCM_TEXTURE_DEPTH24_D8_FLOAT: return 4;
	case CELL_GCM_TEXTURE_DEPTH16:
	case CELL_GCM_TEXTURE_DEPTH16_FLOAT:
	case CELL_GCM_TEXTURE_X16: return 2;
	case CELL_GCM_TEXTURE_Y16_X16: return 4;
	case CELL_GCM_TEXTURE_R5G5B5A1: return 2;
	case CELL_GCM_TEXTURE_W16_Z16_Y16_X16_FLOAT: return 8;
	case CELL_GCM_TEXTURE_W32_Z32_Y32_X32_FLOAT: return 16;
	case CELL_GCM_TEXTURE_X32_FLOAT: return 4;
	case CELL_GCM_TEXTURE_D1R5G5B5: return 2;
	case CELL_GCM_TEXTURE_Y16_X16_FLOAT:
	case CELL_GCM_TEXTURE_D8R8G8B8:
	case CELL_GCM_TEXTURE_COMPRESSED_B8R8_G8R8:
	case CELL_GCM_TEXTURE_COMPRESSED_R8B8_R8G8: return 4;
	case CELL_GCM_TEXTURE_COMPRESSED_HILO8:
	case CELL_GCM_TEXTURE_COMPRESSED_HILO_S8:
	case ~(CELL_GCM_TEXTURE_LN | CELL_GCM_TEXTURE_UN) & CELL_GCM_TEXTURE_COMPRESSED_B8R8_G8R8:
	case ~(CELL_GCM_TEXTURE_LN | CELL_GCM_TEXTURE_UN) & CELL_GCM_TEXTURE_COMPRESSED_R8B8_R8G8:
	default:
		LOG_ERROR(RSX, "Unimplemented Texture format : 0x%x", format);
		return 0;
	}
}

size_t get_texture_block_edge(u32 format)
{
	switch (format)
	{
	case CELL_GCM_TEXTURE_B8:
	case CELL_GCM_TEXTURE_A1R5G5B5:
	case CELL_GCM_TEXTURE_A4R4G4B4:
	case CELL_GCM_TEXTURE_R5G6B5:
	case CELL_GCM_TEXTURE_A8R8G8B8: return 1;
	case CELL_GCM_TEXTURE_COMPRESSED_DXT1:
	case CELL_GCM_TEXTURE_COMPRESSED_DXT23:
	case CELL_GCM_TEXTURE_COMPRESSED_DXT45: return 4;
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
	case CELL_GCM_TEXTURE_Y16_X16_FLOAT:
	case CELL_GCM_TEXTURE_D8R8G8B8: return 1;
	case CELL_GCM_TEXTURE_COMPRESSED_B8R8_G8R8:
	case CELL_GCM_TEXTURE_COMPRESSED_R8B8_R8G8: return 2;
	case CELL_GCM_TEXTURE_COMPRESSED_HILO8:
	case CELL_GCM_TEXTURE_COMPRESSED_HILO_S8:
	case ~(CELL_GCM_TEXTURE_LN | CELL_GCM_TEXTURE_UN) & CELL_GCM_TEXTURE_COMPRESSED_B8R8_G8R8:
	case ~(CELL_GCM_TEXTURE_LN | CELL_GCM_TEXTURE_UN) & CELL_GCM_TEXTURE_COMPRESSED_R8B8_R8G8:
	default:
		LOG_ERROR(RSX, "Unimplemented Texture format : 0x%x", format);
		return 0;
	}
}
}


size_t get_placed_texture_storage_size(const rsx::texture &texture, size_t rowPitchAlignement)
{
	size_t w = texture.width(), h = texture.height(), d = MAX2(texture.depth(), 1);

	int format = texture.format() & ~(CELL_GCM_TEXTURE_LN | CELL_GCM_TEXTURE_UN);
	size_t blockEdge = get_texture_block_edge(format);
	size_t blockSizeInByte = get_texture_block_size(format);

	size_t heightInBlocks = (h + blockEdge - 1) / blockEdge;
	size_t widthInBlocks = (w + blockEdge - 1) / blockEdge;


	size_t result = 0;
	for (unsigned mipmap = 0; mipmap < texture.mipmap(); ++mipmap)
	{
		size_t rowPitch = align(blockSizeInByte * widthInBlocks, rowPitchAlignement);
		result += align(rowPitch * heightInBlocks * d, 512);
		heightInBlocks = MAX2(heightInBlocks / 2, 1);
		widthInBlocks = MAX2(widthInBlocks / 2, 1);
	}

	return result * (texture.cubemap() ? 6 : 1);
}

std::vector<MipmapLevelInfo> upload_placed_texture(gsl::span<gsl::byte> mapped_buffer, const rsx::texture &texture, size_t rowPitchAlignement)
{
	u16 w = texture.width(), h = texture.height();
	u16 depth;
	u8 layer;

	if (texture.dimension() == 1)
	{
		depth = 1;
		layer = 1;
		h = 1;
	}
	else if (texture.dimension() == 2)
	{
		depth = 1;
		layer = texture.cubemap() ? 6 : 1;
	}
	else if (texture.dimension() == 3)
	{
		depth = texture.depth();
		layer = 1;
	}
	else
		throw EXCEPTION("Unsupported texture dimension %d", texture.dimension());

	int format = texture.format() & ~(CELL_GCM_TEXTURE_LN | CELL_GCM_TEXTURE_UN);

	std::vector<MipmapLevelInfo> mipInfos;

	const u32 texaddr = rsx::get_address(texture.offset(), texture.location());
	auto pixels = vm::ps3::_ptr<const u8>(texaddr);
	bool is_swizzled = !(texture.format() & CELL_GCM_TEXTURE_LN);
	switch (format)
	{
	case CELL_GCM_TEXTURE_A8R8G8B8:
		if (is_swizzled)
			return copy_texture_data<copy_unmodified_block_swizzled, false, 1>(as_span_workaround<u32>(mapped_buffer), reinterpret_cast<const u32*>(pixels), w, h, depth, layer, texture.mipmap(), texture.pitch());
		else
			return copy_texture_data<copy_unmodified_block, true, 1>(as_span_workaround<u32>(mapped_buffer), reinterpret_cast<const u32*>(pixels), w, h, depth, layer, texture.mipmap(), texture.pitch());
	case CELL_GCM_TEXTURE_DEPTH16:
	case CELL_GCM_TEXTURE_A1R5G5B5:
	case CELL_GCM_TEXTURE_A4R4G4B4:
	case CELL_GCM_TEXTURE_R5G6B5:
		if (is_swizzled)
			return copy_texture_data<copy_unmodified_block_swizzled, false, 1>(as_span_workaround<u16>(mapped_buffer), reinterpret_cast<const be_t<u16>*>(pixels), w, h, depth, layer, texture.mipmap(), texture.pitch());
		else
			return copy_texture_data<copy_unmodified_block, true, 1>(as_span_workaround<u16>(mapped_buffer), reinterpret_cast<const be_t<u16>*>(pixels), w, h, depth, layer, texture.mipmap(), texture.pitch());
	case CELL_GCM_TEXTURE_W16_Z16_Y16_X16_FLOAT:
		return copy_texture_data<copy_unmodified_block, true, 1>(as_span_workaround<u16>(mapped_buffer), reinterpret_cast<const be_t<u16>*>(pixels), 4 * w, h, depth, layer, texture.mipmap(), texture.pitch());
	case CELL_GCM_TEXTURE_COMPRESSED_DXT1:
		if (is_swizzled)
			return copy_texture_data<copy_unmodified_block, false, 4>(as_span_workaround<u64>(mapped_buffer), reinterpret_cast<const u64*>(pixels), w, h, depth, layer, texture.mipmap(), texture.pitch());
		else
			return copy_texture_data<copy_unmodified_block, true, 4>(as_span_workaround<u64>(mapped_buffer), reinterpret_cast<const u64*>(pixels), w, h, depth, layer, texture.mipmap(), texture.pitch());
	case CELL_GCM_TEXTURE_COMPRESSED_DXT23:
		if (is_swizzled)
			return copy_texture_data<copy_unmodified_block, false, 4>(as_span_workaround<u128>(mapped_buffer), reinterpret_cast<const u128*>(pixels), w, h, depth, layer, texture.mipmap(), texture.pitch());
		else
			return copy_texture_data<copy_unmodified_block, true, 4>(as_span_workaround<u128>(mapped_buffer), reinterpret_cast<const u128*>(pixels), w, h, depth, layer, texture.mipmap(), texture.pitch());
	case CELL_GCM_TEXTURE_COMPRESSED_DXT45:
		if (is_swizzled)
			return copy_texture_data<copy_unmodified_block, false, 4>(as_span_workaround<u128>(mapped_buffer), reinterpret_cast<const u128*>(pixels), w, h, depth, layer, texture.mipmap(), texture.pitch());
		else
			return copy_texture_data<copy_unmodified_block, true, 4>(as_span_workaround<u128>(mapped_buffer), reinterpret_cast<const u128*>(pixels), w, h, depth, layer, texture.mipmap(), texture.pitch());
	case CELL_GCM_TEXTURE_B8:
		return copy_texture_data<copy_unmodified_block, true, 1>(as_span_workaround<u8>(mapped_buffer), reinterpret_cast<const u8*>(pixels), w, h, depth, layer, texture.mipmap(), texture.pitch());
	}
	throw EXCEPTION("Wrong format %d", format);
}

size_t get_texture_size(const rsx::texture &texture)
{
	size_t w = texture.width(), h = texture.height();

	int format = texture.format() & ~(CELL_GCM_TEXTURE_LN | CELL_GCM_TEXTURE_UN);
	// TODO: Take mipmaps into account
	switch (format)
	{
	case CELL_GCM_TEXTURE_COMPRESSED_HILO8:
	case CELL_GCM_TEXTURE_COMPRESSED_HILO_S8:
	case ~(CELL_GCM_TEXTURE_LN | CELL_GCM_TEXTURE_UN) & CELL_GCM_TEXTURE_COMPRESSED_B8R8_G8R8:
	case ~(CELL_GCM_TEXTURE_LN | CELL_GCM_TEXTURE_UN) & CELL_GCM_TEXTURE_COMPRESSED_R8B8_R8G8:
	default:
		LOG_ERROR(RSX, "Unimplemented Texture format : 0x%x", format);
		return 0;
	case CELL_GCM_TEXTURE_B8:
		return w * h;
	case CELL_GCM_TEXTURE_A1R5G5B5:
		return w * h * 2;
	case CELL_GCM_TEXTURE_A4R4G4B4:
		return w * h * 2;
	case CELL_GCM_TEXTURE_R5G6B5:
		return w * h * 2;
	case CELL_GCM_TEXTURE_A8R8G8B8:
		return w * h * 4;
	case CELL_GCM_TEXTURE_COMPRESSED_DXT1:
		return w * h / 6;
	case CELL_GCM_TEXTURE_COMPRESSED_DXT23:
		return w * h / 4;
	case CELL_GCM_TEXTURE_COMPRESSED_DXT45:
		return w * h / 4;
	case CELL_GCM_TEXTURE_G8B8:
		return w * h * 2;
	case CELL_GCM_TEXTURE_R6G5B5:
		return w * h * 2;
	case CELL_GCM_TEXTURE_DEPTH24_D8:
		return w * h * 4;
	case CELL_GCM_TEXTURE_DEPTH24_D8_FLOAT:
		return w * h * 4;
	case CELL_GCM_TEXTURE_DEPTH16:
		return w * h * 2;
	case CELL_GCM_TEXTURE_DEPTH16_FLOAT:
		return w * h * 2;
	case CELL_GCM_TEXTURE_X16:
		return w * h * 2;
	case CELL_GCM_TEXTURE_Y16_X16:
		return w * h * 4;
	case CELL_GCM_TEXTURE_R5G5B5A1:
		return w * h * 2;
	case CELL_GCM_TEXTURE_W16_Z16_Y16_X16_FLOAT:
		return w * h * 8;
	case CELL_GCM_TEXTURE_W32_Z32_Y32_X32_FLOAT:
		return w * h * 16;
	case CELL_GCM_TEXTURE_X32_FLOAT:
		return w * h * 4;
	case CELL_GCM_TEXTURE_D1R5G5B5:
		return w * h * 2;
	case CELL_GCM_TEXTURE_Y16_X16_FLOAT:
		return w * h * 4;
	case CELL_GCM_TEXTURE_D8R8G8B8:
		return w * h * 4;
	case CELL_GCM_TEXTURE_COMPRESSED_B8R8_G8R8:
		return w * h * 4;
	case CELL_GCM_TEXTURE_COMPRESSED_R8B8_R8G8:
		return w * h * 4;
	}
}
