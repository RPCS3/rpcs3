#include "stdafx.h"
#include "Emu/Memory/vm.h"
#include "TextureUtils.h"
#include "../RSXThread.h"
#include "Utilities/Log.h"


#define MAX2(a, b) ((a) > (b)) ? (a) : (b)

unsigned LinearToSwizzleAddress(unsigned x, unsigned y, unsigned z, unsigned log2_width, unsigned log2_height, unsigned log2_depth)
{
	unsigned offset = 0;
	unsigned shift_count = 0;
	while (log2_width | log2_height | log2_depth) {
		if (log2_width)
		{
			offset |= (x & 0x01) << shift_count;
			x >>= 1;
			++shift_count;
			--log2_width;
		}
		if (log2_height)
		{
			offset |= (y & 0x01) << shift_count;
			y >>= 1;
			++shift_count;
			--log2_height;
		}
		if (log2_depth)
		{
			offset |= (z & 0x01) << shift_count;
			z >>= 1;
			++shift_count;
			--log2_depth;
		}
	}
	return offset;
}


/**
* Write data, assume src pixels are packed but not mipmaplevel
*/
inline std::vector<MipmapLevelInfo>
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
inline std::vector<MipmapLevelInfo>
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
		u32 log2width, log2height;

		castedSrc = (u32*)src + offsetInSrc;
		castedDst = (u32*)dst + offsetInDst;

		log2width = (u32)(logf((float)currentWidth) / logf(2.f));
		log2height = (u32)(logf((float)currentHeight) / logf(2.f));

		for (int row = 0; row < currentHeight; row++)
			for (int j = 0; j < currentWidth; j++)
				castedDst[(row * rowPitch / 4) + j] = castedSrc[LinearToSwizzleAddress(j, row, 0, log2width, log2height, 0)];

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
inline std::vector<MipmapLevelInfo>
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
inline std::vector<MipmapLevelInfo>
write16bTexelsSwizzled(const char *src, char *dst, size_t widthInBlock, size_t heightInBlock, size_t blockSize, size_t mipmapCount)
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

		u16 *castedSrc, *castedDst;
		u16 log2width, log2height;

		castedSrc = (u16*)src + offsetInSrc;
		castedDst = (u16*)dst + offsetInDst;

		log2width = (u32)(logf((float)currentWidth) / logf(2.f));
		log2height = (u32)(logf((float)currentHeight) / logf(2.f));

		for (int row = 0; row < currentHeight; row++)
			for (int j = 0; j < currentWidth; j++)
				castedDst[(row * rowPitch / 2) + j] = castedSrc[LinearToSwizzleAddress(j, row, 0, log2width, log2height, 0)];

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
inline std::vector<MipmapLevelInfo>
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
inline std::vector<MipmapLevelInfo>
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


size_t getPlacedTextureStorageSpace(const RSXTexture &texture, size_t rowPitchAlignement)
{
	size_t w = texture.GetWidth(), h = texture.GetHeight();

	size_t blockSizeInByte, blockWidthInPixel, blockHeightInPixel;
	int format = texture.GetFormat() & ~(CELL_GCM_TEXTURE_LN | CELL_GCM_TEXTURE_UN);

	switch (format)
	{
	case CELL_GCM_TEXTURE_COMPRESSED_HILO8:
	case CELL_GCM_TEXTURE_COMPRESSED_HILO_S8:
	case ~(CELL_GCM_TEXTURE_LN | CELL_GCM_TEXTURE_UN) & CELL_GCM_TEXTURE_COMPRESSED_B8R8_G8R8:
	case ~(CELL_GCM_TEXTURE_LN | CELL_GCM_TEXTURE_UN) & CELL_GCM_TEXTURE_COMPRESSED_R8B8_R8G8:
	default:
		LOG_ERROR(RSX, "Unimplemented Texture format : %x", format);
		break;
	case CELL_GCM_TEXTURE_B8:
		blockSizeInByte = 1;
		blockWidthInPixel = 1, blockHeightInPixel = 1;
		break;
	case CELL_GCM_TEXTURE_A1R5G5B5:
		blockSizeInByte = 2;
		blockHeightInPixel = 1, blockWidthInPixel = 1;
		break;
	case CELL_GCM_TEXTURE_A4R4G4B4:
		blockSizeInByte = 2;
		blockWidthInPixel = 1, blockHeightInPixel = 1;
		break;
	case CELL_GCM_TEXTURE_R5G6B5:
		blockSizeInByte = 2;
		blockWidthInPixel = 1, blockHeightInPixel = 1;
		break;
	case CELL_GCM_TEXTURE_A8R8G8B8:
		blockSizeInByte = 4;
		blockWidthInPixel = 1, blockHeightInPixel = 1;
		break;
	case CELL_GCM_TEXTURE_COMPRESSED_DXT1:
		blockSizeInByte = 8;
		blockWidthInPixel = 4, blockHeightInPixel = 4;
		break;
	case CELL_GCM_TEXTURE_COMPRESSED_DXT23:
		blockSizeInByte = 16;
		blockWidthInPixel = 4, blockHeightInPixel = 4;
		break;
	case CELL_GCM_TEXTURE_COMPRESSED_DXT45:
		blockSizeInByte = 16;
		blockWidthInPixel = 4, blockHeightInPixel = 4;
		break;
	case CELL_GCM_TEXTURE_G8B8:
		blockSizeInByte = 2;
		blockWidthInPixel = 1, blockHeightInPixel = 1;
		break;
	case CELL_GCM_TEXTURE_R6G5B5:
		// Not native
		blockSizeInByte = 4;
		blockWidthInPixel = 1, blockHeightInPixel = 1;
		break;
	case CELL_GCM_TEXTURE_DEPTH24_D8:
		blockSizeInByte = 4;
		blockWidthInPixel = 1, blockHeightInPixel = 1;
		break;
	case CELL_GCM_TEXTURE_DEPTH24_D8_FLOAT:
		blockSizeInByte = 4;
		blockWidthInPixel = 1, blockHeightInPixel = 1;
		break;
	case CELL_GCM_TEXTURE_DEPTH16:
		blockSizeInByte = 2;
		blockWidthInPixel = 1, blockHeightInPixel = 1;
		break;
	case CELL_GCM_TEXTURE_DEPTH16_FLOAT:
		blockSizeInByte = 2;
		blockWidthInPixel = 1, blockHeightInPixel = 1;
		break;
	case CELL_GCM_TEXTURE_X16:
		blockSizeInByte = 2;
		blockWidthInPixel = 1, blockHeightInPixel = 1;
		break;
	case CELL_GCM_TEXTURE_Y16_X16:
		blockSizeInByte = 4;
		blockWidthInPixel = 1, blockHeightInPixel = 1;
		break;
	case CELL_GCM_TEXTURE_R5G5B5A1:
		blockSizeInByte = 2;
		blockWidthInPixel = 1, blockHeightInPixel = 1;
		break;
	case CELL_GCM_TEXTURE_W16_Z16_Y16_X16_FLOAT:
		blockSizeInByte = 8;
		blockWidthInPixel = 1, blockHeightInPixel = 1;
		break;
	case CELL_GCM_TEXTURE_W32_Z32_Y32_X32_FLOAT:
		blockSizeInByte = 16;
		blockWidthInPixel = 1, blockHeightInPixel = 1;
		break;
	case CELL_GCM_TEXTURE_X32_FLOAT:
		blockSizeInByte = 4;
		blockWidthInPixel = 1, blockHeightInPixel = 1;
		break;
	case CELL_GCM_TEXTURE_D1R5G5B5:
		blockSizeInByte = 2;
		blockWidthInPixel = 1, blockHeightInPixel = 1;
		break;
	case CELL_GCM_TEXTURE_Y16_X16_FLOAT:
		blockSizeInByte = 4;
		blockWidthInPixel = 1, blockHeightInPixel = 1;
		break;
	case CELL_GCM_TEXTURE_D8R8G8B8:
		blockSizeInByte = 4;
		blockWidthInPixel = 1, blockHeightInPixel = 1;
		break;
	case CELL_GCM_TEXTURE_COMPRESSED_B8R8_G8R8:
		blockSizeInByte = 4;
		blockWidthInPixel = 2, blockHeightInPixel = 2;
		break;
	case CELL_GCM_TEXTURE_COMPRESSED_R8B8_R8G8:
		blockSizeInByte = 4;
		blockWidthInPixel = 2, blockHeightInPixel = 2;
		break;
	}

	size_t heightInBlocks = (h + blockHeightInPixel - 1) / blockHeightInPixel;
	size_t widthInBlocks = (w + blockWidthInPixel - 1) / blockWidthInPixel;

	size_t rowPitch = align(blockSizeInByte * widthInBlocks, rowPitchAlignement);

	return rowPitch * heightInBlocks * 2; // * 2 for mipmap levels
}

std::vector<MipmapLevelInfo> uploadPlacedTexture(const RSXTexture &texture, size_t rowPitchAlignement, void* textureData)
{
	size_t w = texture.GetWidth(), h = texture.GetHeight();

	int format = texture.GetFormat() & ~(CELL_GCM_TEXTURE_LN | CELL_GCM_TEXTURE_UN);

	size_t blockSizeInByte, blockWidthInPixel, blockHeightInPixel;
	switch (format)
	{
	case CELL_GCM_TEXTURE_COMPRESSED_HILO8:
	case CELL_GCM_TEXTURE_COMPRESSED_HILO_S8:
	case ~(CELL_GCM_TEXTURE_LN | CELL_GCM_TEXTURE_UN) & CELL_GCM_TEXTURE_COMPRESSED_B8R8_G8R8:
	case ~(CELL_GCM_TEXTURE_LN | CELL_GCM_TEXTURE_UN) & CELL_GCM_TEXTURE_COMPRESSED_R8B8_R8G8:
	default:
		LOG_ERROR(RSX, "Unimplemented Texture format : %x", format);
		break;
	case CELL_GCM_TEXTURE_B8:
		blockSizeInByte = 1;
		blockWidthInPixel = 1, blockHeightInPixel = 1;
		break;
	case CELL_GCM_TEXTURE_A1R5G5B5:
		blockSizeInByte = 2;
		blockHeightInPixel = 1, blockWidthInPixel = 1;
		break;
	case CELL_GCM_TEXTURE_A4R4G4B4:
		blockSizeInByte = 2;
		blockWidthInPixel = 1, blockHeightInPixel = 1;
		break;
	case CELL_GCM_TEXTURE_R5G6B5:
		blockSizeInByte = 2;
		blockWidthInPixel = 1, blockHeightInPixel = 1;
		break;
	case CELL_GCM_TEXTURE_A8R8G8B8:
		blockSizeInByte = 4;
		blockWidthInPixel = 1, blockHeightInPixel = 1;
		break;
	case CELL_GCM_TEXTURE_COMPRESSED_DXT1:
		blockSizeInByte = 8;
		blockWidthInPixel = 4, blockHeightInPixel = 4;
		break;
	case CELL_GCM_TEXTURE_COMPRESSED_DXT23:
		blockSizeInByte = 16;
		blockWidthInPixel = 4, blockHeightInPixel = 4;
		break;
	case CELL_GCM_TEXTURE_COMPRESSED_DXT45:
		blockSizeInByte = 16;
		blockWidthInPixel = 4, blockHeightInPixel = 4;
		break;
	case CELL_GCM_TEXTURE_G8B8:
		blockSizeInByte = 2;
		blockWidthInPixel = 1, blockHeightInPixel = 1;
		break;
	case CELL_GCM_TEXTURE_R6G5B5:
		// Not native
		blockSizeInByte = 4;
		blockWidthInPixel = 1, blockHeightInPixel = 1;
		break;
	case CELL_GCM_TEXTURE_DEPTH24_D8:
		blockSizeInByte = 4;
		blockWidthInPixel = 1, blockHeightInPixel = 1;
		break;
	case CELL_GCM_TEXTURE_DEPTH24_D8_FLOAT:
		blockSizeInByte = 4;
		blockWidthInPixel = 1, blockHeightInPixel = 1;
		break;
	case CELL_GCM_TEXTURE_DEPTH16:
		blockSizeInByte = 2;
		blockWidthInPixel = 1, blockHeightInPixel = 1;
		break;
	case CELL_GCM_TEXTURE_DEPTH16_FLOAT:
		blockSizeInByte = 2;
		blockWidthInPixel = 1, blockHeightInPixel = 1;
		break;
	case CELL_GCM_TEXTURE_X16:
		blockSizeInByte = 2;
		blockWidthInPixel = 1, blockHeightInPixel = 1;
		break;
	case CELL_GCM_TEXTURE_Y16_X16:
		blockSizeInByte = 4;
		blockWidthInPixel = 1, blockHeightInPixel = 1;
		break;
	case CELL_GCM_TEXTURE_R5G5B5A1:
		blockSizeInByte = 2;
		blockWidthInPixel = 1, blockHeightInPixel = 1;
		break;
	case CELL_GCM_TEXTURE_W16_Z16_Y16_X16_FLOAT:
		blockSizeInByte = 8;
		blockWidthInPixel = 1, blockHeightInPixel = 1;
		break;
	case CELL_GCM_TEXTURE_W32_Z32_Y32_X32_FLOAT:
		blockSizeInByte = 16;
		blockWidthInPixel = 1, blockHeightInPixel = 1;
		break;
	case CELL_GCM_TEXTURE_X32_FLOAT:
		blockSizeInByte = 4;
		blockWidthInPixel = 1, blockHeightInPixel = 1;
		break;
	case CELL_GCM_TEXTURE_D1R5G5B5:
		blockSizeInByte = 2;
		blockWidthInPixel = 1, blockHeightInPixel = 1;
		break;
	case CELL_GCM_TEXTURE_Y16_X16_FLOAT:
		blockSizeInByte = 4;
		blockWidthInPixel = 1, blockHeightInPixel = 1;
		break;
	case CELL_GCM_TEXTURE_D8R8G8B8:
		blockSizeInByte = 4;
		blockWidthInPixel = 1, blockHeightInPixel = 1;
		break;
	case CELL_GCM_TEXTURE_COMPRESSED_B8R8_G8R8:
		blockSizeInByte = 4;
		blockWidthInPixel = 2, blockHeightInPixel = 2;
		break;
	case CELL_GCM_TEXTURE_COMPRESSED_R8B8_R8G8:
		blockSizeInByte = 4;
		blockWidthInPixel = 2, blockHeightInPixel = 2;
		break;
	}

	size_t heightInBlocks = (h + blockHeightInPixel - 1) / blockHeightInPixel;
	size_t widthInBlocks = (w + blockWidthInPixel - 1) / blockWidthInPixel;

	std::vector<MipmapLevelInfo> mipInfos;

	const u32 texaddr = GetAddress(texture.GetOffset(), texture.GetLocation());
	auto pixels = vm::get_ptr<const u8>(texaddr);
	bool is_swizzled = !(texture.GetFormat() & CELL_GCM_TEXTURE_LN);
	switch (format)
	{
	case CELL_GCM_TEXTURE_A8R8G8B8:
		if (is_swizzled)
			return writeTexelsSwizzled((char*)pixels, (char*)textureData, w, h, 4, texture.GetMipmap());
		else
			return writeTexelsGeneric((char*)pixels, (char*)textureData, w, h, 4, texture.GetMipmap());
	case CELL_GCM_TEXTURE_A1R5G5B5:
	case CELL_GCM_TEXTURE_A4R4G4B4:
	case CELL_GCM_TEXTURE_R5G6B5:
		if (is_swizzled)
			return write16bTexelsSwizzled((char*)pixels, (char*)textureData, w, h, 2, texture.GetMipmap());
		else
			return write16bTexelsGeneric((char*)pixels, (char*)textureData, w, h, 2, texture.GetMipmap());
	case CELL_GCM_TEXTURE_W16_Z16_Y16_X16_FLOAT:
		return write16bX4TexelsGeneric((char*)pixels, (char*)textureData, w, h, 8, texture.GetMipmap());
	case CELL_GCM_TEXTURE_COMPRESSED_DXT1:
	case CELL_GCM_TEXTURE_COMPRESSED_DXT23:
	case CELL_GCM_TEXTURE_COMPRESSED_DXT45:
		return writeCompressedTexel((char*)pixels, (char*)textureData, widthInBlocks, blockWidthInPixel, heightInBlocks, blockHeightInPixel, blockSizeInByte, texture.GetMipmap());
	default:
		return writeTexelsGeneric((char*)pixels, (char*)textureData, w, h, blockSizeInByte, texture.GetMipmap());
	}

}