#include "stdafx.h"
#include "Emu/Memory/vm.h"
#include "TextureUtils.h"
#include "../RSXThread.h"
#include "Utilities/Log.h"


#define MAX2(a, b) ((a) > (b)) ? (a) : (b)
namespace
{
/**
* Write data, assume src pixels are packed but not mipmaplevel
*/
std::vector<MipmapLevelInfo>
writeTexelsGeneric(const char *src, char *dst, size_t widthInBlock, size_t heightInBlock, size_t blockSize, size_t mipmapCount)
{
	std::vector<MipmapLevelInfo> Result;
	size_t offsetInDst = 0, offsetInSrc = 0;
	size_t currentHeight = heightInBlock, currentWidth = widthInBlock;
	for (unsigned mipLevel = 0; mipLevel < mipmapCount; mipLevel++)
	{
		size_t rowPitch = align(currentWidth * blockSize, 256);

		MipmapLevelInfo currentMipmapLevelInfo = {};
		currentMipmapLevelInfo.offset = offsetInDst;
		currentMipmapLevelInfo.height = currentHeight;
		currentMipmapLevelInfo.width = currentWidth;
		currentMipmapLevelInfo.rowPitch = rowPitch;
		Result.push_back(currentMipmapLevelInfo);

		for (unsigned row = 0; row < currentHeight; row++)
			memcpy((char*)dst + offsetInDst + row * rowPitch, (char*)src + offsetInSrc + row * widthInBlock * blockSize, currentWidth * blockSize);

		offsetInDst += currentHeight * rowPitch;
		offsetInDst = align(offsetInDst, 512);
		offsetInSrc += currentHeight * widthInBlock * blockSize;
		currentHeight = MAX2(currentHeight / 2, 1);
		currentWidth = MAX2(currentWidth / 2, 1);
	}
	return Result;
}

/**
* Write data, assume src pixels are swizzled and but not mipmaplevel
*/
std::vector<MipmapLevelInfo>
writeTexelsSwizzled(const char *src, char *dst, size_t widthInBlock, size_t heightInBlock, size_t blockSize, size_t mipmapCount)
{
	std::vector<MipmapLevelInfo> Result;
	size_t offsetInDst = 0, offsetInSrc = 0;
	size_t currentHeight = heightInBlock, currentWidth = widthInBlock;
	for (unsigned mipLevel = 0; mipLevel < mipmapCount; mipLevel++)
	{
		size_t rowPitch = align(currentWidth * blockSize, 256);

		MipmapLevelInfo currentMipmapLevelInfo = {};
		currentMipmapLevelInfo.offset = offsetInDst;
		currentMipmapLevelInfo.height = currentHeight;
		currentMipmapLevelInfo.width = currentWidth;
		currentMipmapLevelInfo.rowPitch = rowPitch;
		Result.push_back(currentMipmapLevelInfo);

		u32 *castedSrc, *castedDst;

		castedSrc = (u32*)src + offsetInSrc;
		castedDst = (u32*)dst + offsetInDst;

		std::unique_ptr<u32[]> temp_swizzled(new u32[currentHeight * currentWidth]);
		rsx::convert_linear_swizzle<u32>(castedSrc, temp_swizzled.get(), currentWidth, currentHeight, true);
		for (unsigned row = 0; row < currentHeight; row++)
			memcpy((char*)dst + offsetInDst + row * rowPitch, (char*)temp_swizzled.get() + offsetInSrc + row * widthInBlock * blockSize, currentWidth * blockSize);

		offsetInDst += currentHeight * rowPitch;
		offsetInSrc += currentHeight * widthInBlock * blockSize;
		currentHeight = MAX2(currentHeight / 2, 1);
		currentWidth = MAX2(currentWidth / 2, 1);
	}
	return Result;
}


/**
* Write data, assume compressed (DXTCn) format
*/
std::vector<MipmapLevelInfo>
writeCompressedTexel(const char *src, char *dst, size_t widthInBlock, size_t blockWidth, size_t heightInBlock, size_t blockHeight, size_t blockSize, size_t mipmapCount)
{
	std::vector<MipmapLevelInfo> Result;
	size_t offsetInDst = 0, offsetInSrc = 0;
	size_t currentHeight = heightInBlock, currentWidth = widthInBlock;
	for (unsigned mipLevel = 0; mipLevel < mipmapCount; mipLevel++)
	{
		size_t rowPitch = align(currentWidth * blockSize, 256);

		MipmapLevelInfo currentMipmapLevelInfo = {};
		currentMipmapLevelInfo.offset = offsetInDst;
		currentMipmapLevelInfo.height = currentHeight * blockHeight;
		currentMipmapLevelInfo.width = currentWidth * blockWidth;
		currentMipmapLevelInfo.rowPitch = rowPitch;
		Result.push_back(currentMipmapLevelInfo);

		for (unsigned row = 0; row < currentHeight; row++)
			memcpy((char*)dst + offsetInDst + row * rowPitch, (char*)src + offsetInSrc + row * currentWidth * blockSize, currentWidth * blockSize);

		offsetInDst += currentHeight * rowPitch;
		offsetInDst = align(offsetInDst, 512);
		offsetInSrc += currentHeight * currentWidth * blockSize;
		currentHeight = MAX2(currentHeight / 2, 1);
		currentWidth = MAX2(currentWidth / 2, 1);
	}
	return Result;
}


/**
* Write 16 bytes pixel textures, assume src pixels are swizzled and but not mipmaplevel
*/
std::vector<MipmapLevelInfo>
write16bTexelsSwizzled(const char *src, char *dst, size_t widthInBlock, size_t heightInBlock, size_t blockSize, size_t mipmapCount)
{
	std::vector<MipmapLevelInfo> Result;
	size_t offsetInDst = 0, offsetInSrc = 0;
	size_t currentHeight = heightInBlock, currentWidth = widthInBlock;
	size_t srcPitch = widthInBlock * blockSize;
	for (unsigned mipLevel = 0; mipLevel < mipmapCount; mipLevel++)
	{
		size_t rowPitch = align(currentWidth * blockSize, 256);

		MipmapLevelInfo currentMipmapLevelInfo = {};
		currentMipmapLevelInfo.offset = offsetInDst;
		currentMipmapLevelInfo.height = currentHeight;
		currentMipmapLevelInfo.width = currentWidth;
		currentMipmapLevelInfo.rowPitch = rowPitch;
		Result.push_back(currentMipmapLevelInfo);

		u16 *castedSrc, *castedDst;

		castedSrc = (u16*)src + offsetInSrc;
		castedDst = (u16*)dst + offsetInDst;

		std::unique_ptr<u16[]> temp_swizzled(new u16[currentHeight * currentWidth]);
		rsx::convert_linear_swizzle<u16>(castedSrc, temp_swizzled.get(), currentWidth, currentHeight, true);
		for (unsigned row = 0; row < heightInBlock; row++)
			for (int j = 0; j < currentWidth; j++)
			{
				u16 tmp = temp_swizzled[offsetInSrc / 2 + row * srcPitch / 2 + j];
				castedDst[offsetInDst / 2 + row * rowPitch / 2 + j] = (tmp >> 8) | (tmp << 8);
			}

		offsetInDst += currentHeight * rowPitch;
		offsetInSrc += currentHeight * widthInBlock * blockSize;
		currentHeight = MAX2(currentHeight / 2, 1);
		currentWidth = MAX2(currentWidth / 2, 1);
	}
	return Result;
}

/**
* Write 16 bytes pixel textures, assume src pixels are packed but not mipmaplevel
*/
std::vector<MipmapLevelInfo>
write16bTexelsGeneric(const char *src, char *dst, size_t widthInBlock, size_t heightInBlock, size_t blockSize, size_t mipmapCount)
{
	std::vector<MipmapLevelInfo> Result;
	size_t offsetInDst = 0, offsetInSrc = 0;
	size_t currentHeight = heightInBlock, currentWidth = widthInBlock;
	size_t srcPitch = widthInBlock * blockSize;
	for (unsigned mipLevel = 0; mipLevel < mipmapCount; mipLevel++)
	{
		size_t rowPitch = align(currentWidth * blockSize, 256);

		MipmapLevelInfo currentMipmapLevelInfo = {};
		currentMipmapLevelInfo.offset = offsetInDst;
		currentMipmapLevelInfo.height = currentHeight;
		currentMipmapLevelInfo.width = currentWidth;
		currentMipmapLevelInfo.rowPitch = rowPitch;
		Result.push_back(currentMipmapLevelInfo);

		unsigned short *castedDst = (unsigned short *)dst, *castedSrc = (unsigned short *)src;

		for (unsigned row = 0; row < heightInBlock; row++)
			for (int j = 0; j < currentWidth; j++)
			{
				u16 tmp = castedSrc[offsetInSrc / 2 + row * srcPitch / 2 + j];
				castedDst[offsetInDst / 2 + row * rowPitch / 2 + j] = (tmp >> 8) | (tmp << 8);
			}

		offsetInDst += currentHeight * rowPitch;
		offsetInSrc += currentHeight * widthInBlock * blockSize;
		currentHeight = MAX2(currentHeight / 2, 1);
		currentWidth = MAX2(currentWidth / 2, 1);
	}
	return Result;
}

/**
* Write 16 bytes pixel textures, assume src pixels are packed but not mipmaplevel
*/
std::vector<MipmapLevelInfo>
write16bX4TexelsGeneric(const char *src, char *dst, size_t widthInBlock, size_t heightInBlock, size_t blockSize, size_t mipmapCount)
{
	std::vector<MipmapLevelInfo> Result;
	size_t offsetInDst = 0, offsetInSrc = 0;
	size_t currentHeight = heightInBlock, currentWidth = widthInBlock;
	size_t srcPitch = widthInBlock * blockSize;
	for (unsigned mipLevel = 0; mipLevel < mipmapCount; mipLevel++)
	{
		size_t rowPitch = align(currentWidth * blockSize, 256);

		MipmapLevelInfo currentMipmapLevelInfo = {};
		currentMipmapLevelInfo.offset = offsetInDst;
		currentMipmapLevelInfo.height = currentHeight;
		currentMipmapLevelInfo.width = currentWidth;
		currentMipmapLevelInfo.rowPitch = rowPitch;
		Result.push_back(currentMipmapLevelInfo);

		unsigned short *castedDst = (unsigned short *)dst, *castedSrc = (unsigned short *)src;

		for (unsigned row = 0; row < heightInBlock; row++)
			for (int j = 0; j < currentWidth * 4; j++)
			{
				u16 tmp = castedSrc[offsetInSrc / 2 + row * srcPitch / 2 + j];
				castedDst[offsetInDst / 2 + row * rowPitch / 2 + j] = (tmp >> 8) | (tmp << 8);
			}

		offsetInDst += currentHeight * rowPitch;
		offsetInSrc += currentHeight * widthInBlock * blockSize;
		currentHeight = MAX2(currentHeight / 2, 1);
		currentWidth = MAX2(currentWidth / 2, 1);
	}
	return Result;
}

/**
 * A texture is stored as an array of blocks, where a block is a pixel for standard texture
 * but is a structure containing several pixels for compressed format
 */
size_t get_texture_block_size(u32 format) noexcept
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
		LOG_ERROR(RSX, "Unimplemented Texture format : %x", format);
		return 0;
	}
}

size_t get_texture_block_edge(u32 format) noexcept
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
		LOG_ERROR(RSX, "Unimplemented Texture format : %x", format);
		return 0;
	}
}
}


size_t get_placed_texture_storage_size(const rsx::texture &texture, size_t rowPitchAlignement) noexcept
{
	size_t w = texture.width(), h = texture.height();

	int format = texture.format() & ~(CELL_GCM_TEXTURE_LN | CELL_GCM_TEXTURE_UN);
	size_t blockEdge = get_texture_block_edge(format);
	size_t blockSizeInByte = get_texture_block_size(format);

	size_t heightInBlocks = (h + blockEdge - 1) / blockEdge;
	size_t widthInBlocks = (w + blockEdge - 1) / blockEdge;

	size_t rowPitch = align(blockSizeInByte * widthInBlocks, rowPitchAlignement);

	return rowPitch * heightInBlocks * 2; // * 2 for mipmap levels
}

std::vector<MipmapLevelInfo> upload_placed_texture(const rsx::texture &texture, size_t rowPitchAlignement, void* textureData) noexcept
{
	size_t w = texture.width(), h = texture.height();

	int format = texture.format() & ~(CELL_GCM_TEXTURE_LN | CELL_GCM_TEXTURE_UN);

	size_t blockSizeInByte = get_texture_block_size(format);
	size_t blockEdge = get_texture_block_edge(format);

	size_t heightInBlocks = (h + blockEdge - 1) / blockEdge;
	size_t widthInBlocks = (w + blockEdge - 1) / blockEdge;

	std::vector<MipmapLevelInfo> mipInfos;

	const u32 texaddr = rsx::get_address(texture.offset(), texture.location());
	auto pixels = vm::ps3::_ptr<const u8>(texaddr);
	bool is_swizzled = !(texture.format() & CELL_GCM_TEXTURE_LN);
	switch (format)
	{
	case CELL_GCM_TEXTURE_A8R8G8B8:
		if (is_swizzled)
			return writeTexelsSwizzled((char*)pixels, (char*)textureData, w, h, 4, texture.mipmap());
		else
			return writeTexelsGeneric((char*)pixels, (char*)textureData, w, h, 4, texture.mipmap());
	case CELL_GCM_TEXTURE_A1R5G5B5:
	case CELL_GCM_TEXTURE_A4R4G4B4:
	case CELL_GCM_TEXTURE_R5G6B5:
		if (is_swizzled)
			return write16bTexelsSwizzled((char*)pixels, (char*)textureData, w, h, 2, texture.mipmap());
		else
			return write16bTexelsGeneric((char*)pixels, (char*)textureData, w, h, 2, texture.mipmap());
	case CELL_GCM_TEXTURE_W16_Z16_Y16_X16_FLOAT:
		return write16bX4TexelsGeneric((char*)pixels, (char*)textureData, w, h, 8, texture.mipmap());
	case CELL_GCM_TEXTURE_COMPRESSED_DXT1:
	case CELL_GCM_TEXTURE_COMPRESSED_DXT23:
	case CELL_GCM_TEXTURE_COMPRESSED_DXT45:
		return writeCompressedTexel((char*)pixels, (char*)textureData, widthInBlocks, blockEdge, heightInBlocks, blockEdge, blockSizeInByte, texture.mipmap());
	default:
		return writeTexelsGeneric((char*)pixels, (char*)textureData, w, h, blockSizeInByte, texture.mipmap());
	}
}

size_t get_texture_size(const rsx::texture &texture) noexcept
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
		LOG_ERROR(RSX, "Unimplemented Texture format : %x", format);
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