#include "stdafx.h"
#if defined(DX12_SUPPORT)
#include "D3D12GSRender.h"
// For clarity this code deals with texture but belongs to D3D12GSRender class


static
u32 LinearToSwizzleAddress(u32 x, u32 y, u32 z, u32 log2_width, u32 log2_height, u32 log2_depth)
{
	u32 offset = 0;
	u32 shift_count = 0;
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

static
D3D12_COMPARISON_FUNC getSamplerCompFunc[] =
{
	D3D12_COMPARISON_FUNC_NEVER,
	D3D12_COMPARISON_FUNC_LESS,
	D3D12_COMPARISON_FUNC_EQUAL,
	D3D12_COMPARISON_FUNC_LESS_EQUAL,
	D3D12_COMPARISON_FUNC_GREATER,
	D3D12_COMPARISON_FUNC_NOT_EQUAL,
	D3D12_COMPARISON_FUNC_GREATER_EQUAL,
	D3D12_COMPARISON_FUNC_ALWAYS
};

static
size_t getSamplerMaxAniso(size_t aniso)
{
	switch (aniso)
	{
	case CELL_GCM_TEXTURE_MAX_ANISO_1: return 1;
	case CELL_GCM_TEXTURE_MAX_ANISO_2: return 2;
	case CELL_GCM_TEXTURE_MAX_ANISO_4: return 4;
	case CELL_GCM_TEXTURE_MAX_ANISO_6: return 6;
	case CELL_GCM_TEXTURE_MAX_ANISO_8: return 8;
	case CELL_GCM_TEXTURE_MAX_ANISO_10: return 10;
	case CELL_GCM_TEXTURE_MAX_ANISO_12: return 12;
	case CELL_GCM_TEXTURE_MAX_ANISO_16: return 16;
	}

	return 1;
}

static
D3D12_TEXTURE_ADDRESS_MODE getSamplerWrap(size_t wrap)
{
	switch (wrap)
	{
	case CELL_GCM_TEXTURE_WRAP: return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	case CELL_GCM_TEXTURE_MIRROR: return D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
	case CELL_GCM_TEXTURE_CLAMP_TO_EDGE: return D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	case CELL_GCM_TEXTURE_BORDER: return D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	case CELL_GCM_TEXTURE_CLAMP: return D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	case CELL_GCM_TEXTURE_MIRROR_ONCE_CLAMP_TO_EDGE: return D3D12_TEXTURE_ADDRESS_MODE_MIRROR_ONCE;
	case CELL_GCM_TEXTURE_MIRROR_ONCE_BORDER: return D3D12_TEXTURE_ADDRESS_MODE_MIRROR_ONCE;
	case CELL_GCM_TEXTURE_MIRROR_ONCE_CLAMP: return D3D12_TEXTURE_ADDRESS_MODE_MIRROR_ONCE;
	}

	return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
}



static
D3D12_FILTER getSamplerFilter(u32 minFilter, u32 magFilter)
{
	D3D12_FILTER_TYPE min, mag, mip;
	switch (minFilter)
	{
	case CELL_GCM_TEXTURE_NEAREST:
		min = D3D12_FILTER_TYPE_POINT;
		mip = D3D12_FILTER_TYPE_POINT;
		break;
	case CELL_GCM_TEXTURE_LINEAR:
		min = D3D12_FILTER_TYPE_LINEAR;
		mip = D3D12_FILTER_TYPE_POINT;
		break;
	case CELL_GCM_TEXTURE_NEAREST_NEAREST:
		min = D3D12_FILTER_TYPE_POINT;
		mip = D3D12_FILTER_TYPE_POINT;
		break;
	case CELL_GCM_TEXTURE_LINEAR_NEAREST:
		min = D3D12_FILTER_TYPE_LINEAR;
		mip = D3D12_FILTER_TYPE_POINT;
		break;
	case CELL_GCM_TEXTURE_NEAREST_LINEAR:
		min = D3D12_FILTER_TYPE_POINT;
		mip = D3D12_FILTER_TYPE_LINEAR;
		break;
	case CELL_GCM_TEXTURE_LINEAR_LINEAR:
		min = D3D12_FILTER_TYPE_LINEAR;
		mip = D3D12_FILTER_TYPE_LINEAR;
		break;
	case CELL_GCM_TEXTURE_CONVOLUTION_MIN:
	default:
		LOG_ERROR(RSX, "Unknow min filter %x", minFilter);
	}

	switch (magFilter)
	{
	case CELL_GCM_TEXTURE_NEAREST:
		mag = D3D12_FILTER_TYPE_POINT;
		break;
	case CELL_GCM_TEXTURE_LINEAR:
		mag = D3D12_FILTER_TYPE_LINEAR;
		break;
	default:
		LOG_ERROR(RSX, "Unknow mag filter %x", magFilter);
	}

	return D3D12_ENCODE_BASIC_FILTER(min, mag, mip, D3D12_FILTER_REDUCTION_TYPE_STANDARD);
}

struct MipmapLevelInfo
{
	size_t offset;
	size_t width;
	size_t height;
	size_t rowPitch;
};

#define MAX2(a, b) ((a) > (b)) ? (a) : (b)

/**
 * Write data, assume src pixels are packed but not mipmaplevel
 */
static std::vector<MipmapLevelInfo>
writeTexelsGeneric(const char *src, char *dst, size_t widthInBlock, size_t heightInBlock, size_t blockSize, size_t mipmapCount)
{
	std::vector<MipmapLevelInfo> Result;
	size_t offsetInDst = 0, offsetInSrc = 0;
	size_t currentHeight = heightInBlock, currentWidth = widthInBlock;
	for (unsigned mipLevel = 0; mipLevel < mipmapCount; mipLevel++)
	{
		size_t rowPitch = powerOf2Align(currentWidth * blockSize, 256);

		MipmapLevelInfo currentMipmapLevelInfo = {};
		currentMipmapLevelInfo.offset = offsetInDst;
		currentMipmapLevelInfo.height = currentHeight;
		currentMipmapLevelInfo.width = currentWidth;
		currentMipmapLevelInfo.rowPitch = rowPitch;
		Result.push_back(currentMipmapLevelInfo);

		for (unsigned row = 0; row < currentHeight; row++)
			memcpy((char*)dst + offsetInDst + row * rowPitch, (char*)src + offsetInSrc + row * widthInBlock * blockSize, currentWidth * blockSize);

		offsetInDst += currentHeight * rowPitch;
		offsetInSrc += currentHeight * widthInBlock * blockSize;
		currentHeight = MAX2(currentHeight / 2, 1);
		currentWidth = MAX2(currentWidth / 2, 1);
	}
	return Result;
}

/**
* Write data, assume src pixels are swizzled and but not mipmaplevel
*/
static std::vector<MipmapLevelInfo>
writeTexelsSwizzled(const char *src, char *dst, size_t widthInBlock, size_t heightInBlock, size_t blockSize, size_t mipmapCount)
{
	std::vector<MipmapLevelInfo> Result;
	size_t offsetInDst = 0, offsetInSrc = 0;
	size_t currentHeight = heightInBlock, currentWidth = widthInBlock;
	for (unsigned mipLevel = 0; mipLevel < mipmapCount; mipLevel++)
	{
		size_t rowPitch = powerOf2Align(currentWidth * blockSize, 256);

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

#pragma omp parallel for
		for (unsigned row = 0; row < currentHeight; row++)
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
static std::vector<MipmapLevelInfo>
writeCompressedTexel(const char *src, char *dst, size_t widthInBlock, size_t blockWidth, size_t heightInBlock, size_t blockHeight, size_t blockSize, size_t mipmapCount)
{
	std::vector<MipmapLevelInfo> Result;
	size_t offsetInDst = 0, offsetInSrc = 0;
	size_t currentHeight = heightInBlock, currentWidth = widthInBlock;
	for (unsigned mipLevel = 0; mipLevel < mipmapCount; mipLevel++)
	{
		size_t rowPitch = powerOf2Align(currentWidth * blockSize, 256);

		MipmapLevelInfo currentMipmapLevelInfo = {};
		currentMipmapLevelInfo.offset = offsetInDst;
		currentMipmapLevelInfo.height = currentHeight * blockHeight;
		currentMipmapLevelInfo.width = currentWidth * blockWidth;
		currentMipmapLevelInfo.rowPitch = rowPitch;
		Result.push_back(currentMipmapLevelInfo);

		for (unsigned row = 0; row < currentHeight; row++)
			memcpy((char*)dst + offsetInDst + row * rowPitch, (char*)src + offsetInSrc + row * currentWidth * blockSize, currentWidth * blockSize);

		offsetInDst += currentHeight * rowPitch;
		offsetInDst = powerOf2Align(offsetInDst, 512);
		offsetInSrc += currentHeight * currentWidth * blockSize;
		currentHeight = MAX2(currentHeight / 2, 1);
		currentWidth = MAX2(currentWidth / 2, 1);
	}
	return Result;
}

/**
* Write 16 bytes pixel textures, assume src pixels are packed but not mipmaplevel
*/
static std::vector<MipmapLevelInfo>
write16bTexelsGeneric(const char *src, char *dst, size_t widthInBlock, size_t heightInBlock, size_t blockSize, size_t mipmapCount)
{
	std::vector<MipmapLevelInfo> Result;
	size_t offsetInDst = 0, offsetInSrc = 0;
	size_t currentHeight = heightInBlock, currentWidth = widthInBlock;
	size_t srcPitch = widthInBlock * blockSize;
	for (unsigned mipLevel = 0; mipLevel < mipmapCount; mipLevel++)
	{
		size_t rowPitch = powerOf2Align(currentWidth * blockSize, 256);

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
static std::vector<MipmapLevelInfo>
write16bX4TexelsGeneric(const char *src, char *dst, size_t widthInBlock, size_t heightInBlock, size_t blockSize, size_t mipmapCount)
{
	std::vector<MipmapLevelInfo> Result;
	size_t offsetInDst = 0, offsetInSrc = 0;
	size_t currentHeight = heightInBlock, currentWidth = widthInBlock;
	size_t srcPitch = widthInBlock * blockSize;
	for (unsigned mipLevel = 0; mipLevel < mipmapCount; mipLevel++)
	{
		size_t rowPitch = powerOf2Align(currentWidth * blockSize, 256);

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
 * Create a texture residing in default heap and generate uploads commands in commandList,
 * using a temporary texture buffer.
 */
static
ID3D12Resource *uploadSingleTexture(
	const RSXTexture &texture,
	ID3D12Device *device,
	ID3D12GraphicsCommandList *commandList,
	DataHeap<ID3D12Heap, 65536> &textureBuffersHeap,
	DataHeap<ID3D12Heap, 65536> &textureHeap)
{
	ID3D12Resource *vramTexture;
	size_t w = texture.GetWidth(), h = texture.GetHeight();

	size_t blockSizeInByte, blockWidthInPixel, blockHeightInPixel;
	int format = texture.GetFormat() & ~(CELL_GCM_TEXTURE_LN | CELL_GCM_TEXTURE_UN);
	DXGI_FORMAT dxgiFormat = getTextureDXGIFormat(format);

	const u32 texaddr = GetAddress(texture.GetOffset(), texture.GetLocation());

	bool is_swizzled = !(texture.GetFormat() & CELL_GCM_TEXTURE_LN);
	size_t srcPitch;
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
		srcPitch = w;
		break;
	case CELL_GCM_TEXTURE_A1R5G5B5:
		blockSizeInByte = 2;
		blockHeightInPixel = 1, blockWidthInPixel = 1;
		srcPitch = w * 2;
		break;
	case CELL_GCM_TEXTURE_A4R4G4B4:
		blockSizeInByte = 2;
		blockWidthInPixel = 1, blockHeightInPixel = 1;
		srcPitch = w * 2;
		break;
	case CELL_GCM_TEXTURE_R5G6B5:
		blockSizeInByte = 2;
		blockWidthInPixel = 1, blockHeightInPixel = 1;
		srcPitch = w * 2;
		break;
	case CELL_GCM_TEXTURE_A8R8G8B8:
		blockSizeInByte = 4;
		blockWidthInPixel = 1, blockHeightInPixel = 1;
		srcPitch = w * 4;
		break;
	case CELL_GCM_TEXTURE_COMPRESSED_DXT1:
		blockSizeInByte = 8;
		blockWidthInPixel = 4, blockHeightInPixel = 4;
		srcPitch = w * 2;
		break;
	case CELL_GCM_TEXTURE_COMPRESSED_DXT23:
		blockSizeInByte = 16;
		blockWidthInPixel = 4, blockHeightInPixel = 4;
		srcPitch = w * 4;
		break;
	case CELL_GCM_TEXTURE_COMPRESSED_DXT45:
		blockSizeInByte = 16;
		blockWidthInPixel = 4, blockHeightInPixel = 4;
		srcPitch = w * 4;
		break;
	case CELL_GCM_TEXTURE_G8B8:
		blockSizeInByte = 2;
		blockWidthInPixel = 1, blockHeightInPixel = 1;
		srcPitch = w * 2;
		break;
	case CELL_GCM_TEXTURE_R6G5B5:
		// Not native
		blockSizeInByte = 4;
		blockWidthInPixel = 1, blockHeightInPixel = 1;
		srcPitch = w * 2;
		break;
	case CELL_GCM_TEXTURE_DEPTH24_D8:
		blockSizeInByte = 4;
		blockWidthInPixel = 1, blockHeightInPixel = 1;
		srcPitch = w * 4;
		break;
	case CELL_GCM_TEXTURE_DEPTH24_D8_FLOAT:
		blockSizeInByte = 4;
		blockWidthInPixel = 1, blockHeightInPixel = 1;
		srcPitch = w * 4;
		break;
	case CELL_GCM_TEXTURE_DEPTH16:
		blockSizeInByte = 2;
		blockWidthInPixel = 1, blockHeightInPixel = 1;
		srcPitch = w * 2;
		break;
	case CELL_GCM_TEXTURE_DEPTH16_FLOAT:
		blockSizeInByte = 2;
		blockWidthInPixel = 1, blockHeightInPixel = 1;
		srcPitch = w * 2;
		break;
	case CELL_GCM_TEXTURE_X16:
		blockSizeInByte = 2;
		blockWidthInPixel = 1, blockHeightInPixel = 1;
		srcPitch = w * 2;
		break;
	case CELL_GCM_TEXTURE_Y16_X16:
		blockSizeInByte = 4;
		blockWidthInPixel = 1, blockHeightInPixel = 1;
		srcPitch = w * 4;
		break;
	case CELL_GCM_TEXTURE_R5G5B5A1:
		blockSizeInByte = 2;
		blockWidthInPixel = 1, blockHeightInPixel = 1;
		srcPitch = w * 2;
		break;
	case CELL_GCM_TEXTURE_W16_Z16_Y16_X16_FLOAT:
		blockSizeInByte = 8;
		blockWidthInPixel = 1, blockHeightInPixel = 1;
		srcPitch = w * 8;
		break;
	case CELL_GCM_TEXTURE_W32_Z32_Y32_X32_FLOAT:
		blockSizeInByte = 16;
		blockWidthInPixel = 1, blockHeightInPixel = 1;
		srcPitch = w * 16;
		break;
	case CELL_GCM_TEXTURE_X32_FLOAT:
		blockSizeInByte = 4;
		blockWidthInPixel = 1, blockHeightInPixel = 1;
		srcPitch = w * 4;
		break;
	case CELL_GCM_TEXTURE_D1R5G5B5:
		blockSizeInByte = 2;
		blockWidthInPixel = 1, blockHeightInPixel = 1;
		srcPitch = w * 2;
		break;
	case CELL_GCM_TEXTURE_Y16_X16_FLOAT:
		blockSizeInByte = 4;
		blockWidthInPixel = 1, blockHeightInPixel = 1;
		srcPitch = w * 4;
		break;
	case CELL_GCM_TEXTURE_D8R8G8B8:
		blockSizeInByte = 4;
		blockWidthInPixel = 1, blockHeightInPixel = 1;
		srcPitch = w * 4;
		break;
	case CELL_GCM_TEXTURE_COMPRESSED_B8R8_G8R8:
		blockSizeInByte = 4;
		blockWidthInPixel = 2, blockHeightInPixel = 2;
		srcPitch = w * 4;
		break;
	case CELL_GCM_TEXTURE_COMPRESSED_R8B8_R8G8:
		blockSizeInByte = 4;
		blockWidthInPixel = 2, blockHeightInPixel = 2;
		srcPitch = w * 4;
		break;
	}

	size_t heightInBlocks = (h + blockHeightInPixel - 1) / blockHeightInPixel;
	size_t widthInBlocks = (w + blockWidthInPixel - 1) / blockWidthInPixel;
	// Multiple of 256
	size_t rowPitch = powerOf2Align(blockSizeInByte * widthInBlocks, 256);

	ID3D12Resource *Texture;
	size_t textureSize = rowPitch * heightInBlocks * 2; // * 4 for mipmap levels
	assert(textureBuffersHeap.canAlloc(textureSize));
	size_t heapOffset = textureBuffersHeap.alloc(textureSize);

	check(device->CreatePlacedResource(
		textureBuffersHeap.m_heap,
		heapOffset,
		&getBufferResourceDesc(textureSize),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&Texture)
		));
	textureBuffersHeap.m_resourceStoredSinceLastSync.push_back(std::make_tuple(heapOffset, textureSize, Texture));

	auto pixels = vm::get_ptr<const u8>(texaddr);
	void *textureData;
	check(Texture->Map(0, nullptr, (void**)&textureData));
	std::vector<MipmapLevelInfo> mipInfos;

	switch (format)
	{
	case CELL_GCM_TEXTURE_A8R8G8B8:
	{
		if (is_swizzled)
			mipInfos = writeTexelsSwizzled((char*)pixels, (char*)textureData, w, h, 4, texture.GetMipmap());
		else
			mipInfos = writeTexelsGeneric((char*)pixels, (char*)textureData, w, h, 4, texture.GetMipmap());
		break;
	}
	case CELL_GCM_TEXTURE_A4R4G4B4:
	case CELL_GCM_TEXTURE_R5G6B5:
	{
		mipInfos = write16bTexelsGeneric((char*)pixels, (char*)textureData, w, h, 2, texture.GetMipmap());
		break;
	}
	case CELL_GCM_TEXTURE_W16_Z16_Y16_X16_FLOAT:
	{
		mipInfos = write16bX4TexelsGeneric((char*)pixels, (char*)textureData, w, h, 8, texture.GetMipmap());
		break;
	}
	case CELL_GCM_TEXTURE_COMPRESSED_DXT1:
	case CELL_GCM_TEXTURE_COMPRESSED_DXT23:
	case CELL_GCM_TEXTURE_COMPRESSED_DXT45:
	{
		mipInfos = writeCompressedTexel((char*)pixels, (char*)textureData, widthInBlocks, blockWidthInPixel, heightInBlocks, blockHeightInPixel, blockSizeInByte, texture.GetMipmap());
		break;
	}
	default:
	{
		mipInfos = writeTexelsGeneric((char*)pixels, (char*)textureData, w, h, blockSizeInByte, texture.GetMipmap());
		break;
	}
	}
	Texture->Unmap(0, nullptr);

	D3D12_RESOURCE_DESC texturedesc = getTexture2DResourceDesc(w, h, dxgiFormat, texture.GetMipmap());
	textureSize = device->GetResourceAllocationInfo(0, 1, &texturedesc).SizeInBytes;

	assert(textureHeap.canAlloc(textureSize));
	size_t heapOffset2 = textureHeap.alloc(textureSize);

	check(device->CreatePlacedResource(
		textureHeap.m_heap,
		heapOffset2,
		&texturedesc,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&vramTexture)
		));
	textureHeap.m_resourceStoredSinceLastSync.push_back(std::make_tuple(heapOffset2, textureSize, vramTexture));


	size_t miplevel = 0;
	for (const MipmapLevelInfo mli : mipInfos)
	{
		D3D12_TEXTURE_COPY_LOCATION dst = {}, src = {};
		dst.pResource = vramTexture;
		dst.SubresourceIndex = (UINT)miplevel;
		dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
		src.PlacedFootprint.Offset = mli.offset;
		src.pResource = Texture;
		src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
		src.PlacedFootprint.Footprint.Depth = 1;
		src.PlacedFootprint.Footprint.Width = (UINT)mli.width;
		src.PlacedFootprint.Footprint.Height = (UINT)mli.height;
		src.PlacedFootprint.Footprint.RowPitch = (UINT)mli.rowPitch;
		src.PlacedFootprint.Footprint.Format = dxgiFormat;

		commandList->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);
		miplevel++;
	}

	D3D12_RESOURCE_BARRIER barrier = {};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Transition.pResource = vramTexture;
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_GENERIC_READ;
	commandList->ResourceBarrier(1, &barrier);
	return vramTexture;
}

size_t D3D12GSRender::UploadTextures()
{
	size_t usedTexture = 0;

	for (u32 i = 0; i < m_textures_count; ++i)
	{
		if (!m_textures[i].IsEnabled()) continue;
		size_t w = m_textures[i].GetWidth(), h = m_textures[i].GetHeight();
		if (!w || !h) continue;

		const u32 texaddr = GetAddress(m_textures[i].GetOffset(), m_textures[i].GetLocation());

		int format = m_textures[i].GetFormat() & ~(CELL_GCM_TEXTURE_LN | CELL_GCM_TEXTURE_UN);
		DXGI_FORMAT dxgiFormat = getTextureDXGIFormat(format);
		bool is_swizzled = !(m_textures[i].GetFormat() & CELL_GCM_TEXTURE_LN);

		ID3D12Resource *vramTexture;
		std::unordered_map<u32, ID3D12Resource* >::const_iterator ItRTT = m_rtts.m_renderTargets.find(texaddr);
		std::unordered_map<u32, ID3D12Resource* >::const_iterator ItCache = m_texturesCache.find(texaddr);
		bool isRenderTarget = false;
		if (ItRTT != m_rtts.m_renderTargets.end())
		{
			vramTexture = ItRTT->second;
			isRenderTarget = true;
		}
		else if (ItCache != m_texturesCache.end())
		{
			vramTexture = ItCache->second;
		}
		else
		{
			// Upload at each iteration to take advantage of overlapping transfer
			ID3D12GraphicsCommandList *commandList;
			check(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, getCurrentResourceStorage().m_textureUploadCommandAllocator, nullptr, IID_PPV_ARGS(&commandList)));

			vramTexture = uploadSingleTexture(m_textures[i], m_device, commandList, m_textureUploadData, m_textureData);

			commandList->Close();
			m_commandQueueGraphic->ExecuteCommandLists(1, (ID3D12CommandList**)&commandList);
			getCurrentResourceStorage().m_inflightCommandList.push_back(commandList);
			m_texturesCache[texaddr] = vramTexture;

			size_t s = powerOf2Align(w * h * 4, 4096);
			LOG_ERROR(RSX, "PROTECTING %x of size %d", powerOf2Align(texaddr, 4096), s);
			texaddrs.push_back(std::make_pair(texaddr & ~0xfff, s));
			vm::page_protect(texaddr & ~0xfff, s, 0, 0, vm::page_writable);
		}

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Format = dxgiFormat;
		srvDesc.Texture2D.MipLevels = m_textures[i].GetMipmap();

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
			srvDesc.Shader4ComponentMapping = D3D12_ENCODE_SHADER_4_COMPONENT_MAPPING(
				D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_0,
				D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_0,
				D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_0,
				D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_0);
			break;
		case CELL_GCM_TEXTURE_A1R5G5B5:
		case CELL_GCM_TEXTURE_A4R4G4B4:
		case CELL_GCM_TEXTURE_R5G6B5:
			srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			break;
		case CELL_GCM_TEXTURE_A8R8G8B8:
		{
			const int RemapValue[4] =
			{
				D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_1,
				D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_2,
				D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_3,
				D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_0
			};

			u8 remap_a = m_textures[i].GetRemap() & 0x3;
			u8 remap_r = (m_textures[i].GetRemap() >> 2) & 0x3;
			u8 remap_g = (m_textures[i].GetRemap() >> 4) & 0x3;
			u8 remap_b = (m_textures[i].GetRemap() >> 6) & 0x3;
			if (isRenderTarget)
				srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			else
				srvDesc.Shader4ComponentMapping = D3D12_ENCODE_SHADER_4_COMPONENT_MAPPING(
					RemapValue[remap_a],
					RemapValue[remap_r],
					RemapValue[remap_g],
					RemapValue[remap_b]);

			break;
		}
		case CELL_GCM_TEXTURE_COMPRESSED_DXT1:
		case CELL_GCM_TEXTURE_COMPRESSED_DXT23:
		case CELL_GCM_TEXTURE_COMPRESSED_DXT45:
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
			srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			break;
		case CELL_GCM_TEXTURE_D8R8G8B8:
		{
			const int RemapValue[4] =
			{
				D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_1,
				D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_2,
				D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_3,
				D3D12_SHADER_COMPONENT_MAPPING_FORCE_VALUE_1
			};

			u8 remap_a = m_textures[i].GetRemap() & 0x3;
			u8 remap_r = (m_textures[i].GetRemap() >> 2) & 0x3;
			u8 remap_g = (m_textures[i].GetRemap() >> 4) & 0x3;
			u8 remap_b = (m_textures[i].GetRemap() >> 6) & 0x3;

			srvDesc.Shader4ComponentMapping = D3D12_ENCODE_SHADER_4_COMPONENT_MAPPING(
				RemapValue[remap_a],
				RemapValue[remap_r],
				RemapValue[remap_g],
				RemapValue[remap_b]);
			break;
		}
		case CELL_GCM_TEXTURE_Y16_X16_FLOAT:
			srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			break;
		case CELL_GCM_TEXTURE_COMPRESSED_B8R8_G8R8:
			srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			break;
		case CELL_GCM_TEXTURE_COMPRESSED_R8B8_R8G8:
			srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			break;
		}
		D3D12_CPU_DESCRIPTOR_HANDLE Handle = getCurrentResourceStorage().m_textureDescriptorsHeap->GetCPUDescriptorHandleForHeapStart();
		Handle.ptr += (getCurrentResourceStorage().m_currentTextureIndex + usedTexture) * m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_device->CreateShaderResourceView(vramTexture, &srvDesc, Handle);

		D3D12_SAMPLER_DESC samplerDesc = {};
		samplerDesc.Filter = getSamplerFilter(m_textures[i].GetMinFilter(), m_textures[i].GetMagFilter());
		samplerDesc.AddressU = getSamplerWrap(m_textures[i].GetWrapS());
		samplerDesc.AddressV = getSamplerWrap(m_textures[i].GetWrapT());
		samplerDesc.AddressW = getSamplerWrap(m_textures[i].GetWrapR());
		samplerDesc.ComparisonFunc = getSamplerCompFunc[m_textures[i].GetZfunc()];
		samplerDesc.MaxAnisotropy = (UINT)getSamplerMaxAniso(m_textures[i].GetMaxAniso());
		samplerDesc.MipLODBias = m_textures[i].GetBias();
		samplerDesc.BorderColor[4] = (FLOAT)m_textures[i].GetBorderColor();
		samplerDesc.MinLOD = (FLOAT)(m_textures[i].GetMinLOD() >> 8);
		samplerDesc.MaxLOD = (FLOAT)(m_textures[i].GetMaxLOD() >> 8);
		Handle = getCurrentResourceStorage().m_samplerDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
		Handle.ptr += (getCurrentResourceStorage().m_currentTextureIndex + usedTexture) * m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
		m_device->CreateSampler(&samplerDesc, Handle);

		usedTexture++;
	}

	return usedTexture;
}

#endif
