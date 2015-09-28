#pragma once
#include "../RSXTexture.h"
#include <vector>

struct MipmapLevelInfo
{
	size_t offset;
	size_t width;
	size_t height;
	size_t rowPitch;
};

unsigned LinearToSwizzleAddress(unsigned x, unsigned y, unsigned z, unsigned log2_width, unsigned log2_height, unsigned log2_depth);

/**
* Get size to store texture in a linear fashion.
* Storage is assumed to use a rowPitchAlignement boundary for every row of texture.
*/
size_t getPlacedTextureStorageSpace(const RSXTexture &texture, size_t rowPitchAlignement);

/**
* Write texture data to textureData.
* Data are not packed, they are stored per rows using rowPitchAlignement.
* Similarly, offset for every mipmaplevel is aligned to rowPitchAlignement boundary.
*/
std::vector<MipmapLevelInfo> uploadPlacedTexture(const RSXTexture &texture, size_t rowPitchAlignement, void* textureData);