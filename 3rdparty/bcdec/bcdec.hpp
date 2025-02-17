// Based on https://github.com/iOrange/bcdec/blob/963c5e56b7a335e066cff7d16a3de75f4e8ad366/bcdec.h
// provides functions to decompress blocks of BC compressed images
//
// ------------------------------------------------------------------------------
//
// MIT LICENSE
// ===========
// Copyright (c) 2022 Sergii Kudlai
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy of
// this software and associated documentation files (the "Software"), to deal in
// the Software without restriction, including without limitation the rights to
// use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
// of the Software, and to permit persons to whom the Software is furnished to do
// so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
// ------------------------------------------------------------------------------

#pragma once

#include <util/types.hpp>

static void bcdec__color_block(const u8* compressedBlock, u8* dstColors, int destinationPitch, bool onlyOpaqueMode) {
	u16 c0, c1;
	u32 refColors[4]; /* 0xAABBGGRR */
	u32 colorIndices;
	u32 r0, g0, b0, r1, g1, b1, r, g, b;

	c0 = *reinterpret_cast<const u16*>(compressedBlock);
	c1 = *(reinterpret_cast<const u16*>(compressedBlock) + 1);

	/* Unpack 565 ref colors */
	r0 = (c0 >> 11) & 0x1F;
	g0 = (c0 >> 5)  & 0x3F;
	b0 =  c0        & 0x1F;

	r1 = (c1 >> 11) & 0x1F;
	g1 = (c1 >> 5)  & 0x3F;
	b1 =  c1        & 0x1F;

	/* Expand 565 ref colors to 888 */
	r = (r0 * 527 + 23) >> 6;
	g = (g0 * 259 + 33) >> 6;
	b = (b0 * 527 + 23) >> 6;
	refColors[0] = 0xFF000000 | (r << 16) | (g << 8) | b;

	r = (r1 * 527 + 23) >> 6;
	g = (g1 * 259 + 33) >> 6;
	b = (b1 * 527 + 23) >> 6;
	refColors[1] = 0xFF000000 | (r << 16) | (g << 8) | b;

	if (c0 > c1 || onlyOpaqueMode)
	{   /* Standard BC1 mode (also BC3 color block uses ONLY this mode) */
		/* color_2 = 2/3*color_0 + 1/3*color_1
		color_3 = 1/3*color_0 + 2/3*color_1 */
		r = ((2 * r0 + r1) *  351 +   61) >>  7;
		g = ((2 * g0 + g1) * 2763 + 1039) >> 11;
		b = ((2 * b0 + b1) *  351 +   61) >>  7;
		refColors[2] = 0xFF000000 | (r << 16) | (g << 8) | b;

		r = ((r0 + r1 * 2) *  351 +   61) >>  7;
		g = ((g0 + g1 * 2) * 2763 + 1039) >> 11;
		b = ((b0 + b1 * 2) *  351 +   61) >>  7;
		refColors[3] = 0xFF000000 | (r << 16) | (g << 8) | b;
	}
	else
	{   /* Quite rare BC1A mode */
		/* color_2 = 1/2*color_0 + 1/2*color_1;
		color_3 = 0;                         */
		r = ((r0 + r1) * 1053 +  125) >>  8;
		g = ((g0 + g1) * 4145 + 1019) >> 11;
		b = ((b0 + b1) * 1053 +  125) >>  8;
		refColors[2] = 0xFF000000 | (r << 16) | (g << 8) | b;

		refColors[3] = 0x00000000;
	}

	colorIndices = *reinterpret_cast<const u32*>(compressedBlock + 4);

	/* Fill out the decompressed color block */
	for (int i = 0; i < 4; ++i)
	{
		for (int j = 0; j < 4; ++j)
		{
			int idx = colorIndices & 0x03;
			*reinterpret_cast<u32*>(dstColors + j * 4) = refColors[idx];
			colorIndices >>= 2;
		}

		dstColors += destinationPitch;
	}
}

static void bcdec__sharp_alpha_block(const u16* alpha, u8* decompressed, int destinationPitch) {
	for (int i = 0; i < 4; ++i)
	{
		for (int j = 0; j < 4; ++j)
		{
			decompressed[j * 4] = ((alpha[i] >> (4 * j)) & 0x0F) * 17;
		}
		decompressed += destinationPitch;
	}
}

static void bcdec__smooth_alpha_block(const u8* compressedBlock, u8* decompressed, int destinationPitch) {
	u8 alpha[8];
	u64 block = *reinterpret_cast<const u64*>(compressedBlock);
	u64 indices;

	alpha[0] = block & 0xFF;
	alpha[1] = (block >> 8) & 0xFF;

	if (alpha[0] > alpha[1])
	{
		/* 6 interpolated alpha values. */
		alpha[2] = (6 * alpha[0] +     alpha[1]) / 7;   /* 6/7*alpha_0 + 1/7*alpha_1 */
		alpha[3] = (5 * alpha[0] + 2 * alpha[1]) / 7;   /* 5/7*alpha_0 + 2/7*alpha_1 */
		alpha[4] = (4 * alpha[0] + 3 * alpha[1]) / 7;   /* 4/7*alpha_0 + 3/7*alpha_1 */
		alpha[5] = (3 * alpha[0] + 4 * alpha[1]) / 7;   /* 3/7*alpha_0 + 4/7*alpha_1 */
		alpha[6] = (2 * alpha[0] + 5 * alpha[1]) / 7;   /* 2/7*alpha_0 + 5/7*alpha_1 */
		alpha[7] = (    alpha[0] + 6 * alpha[1]) / 7;   /* 1/7*alpha_0 + 6/7*alpha_1 */
	}
	else
	{
		/* 4 interpolated alpha values. */
		alpha[2] = (4 * alpha[0] +     alpha[1]) / 5;   /* 4/5*alpha_0 + 1/5*alpha_1 */
		alpha[3] = (3 * alpha[0] + 2 * alpha[1]) / 5;   /* 3/5*alpha_0 + 2/5*alpha_1 */
		alpha[4] = (2 * alpha[0] + 3 * alpha[1]) / 5;   /* 2/5*alpha_0 + 3/5*alpha_1 */
		alpha[5] = (    alpha[0] + 4 * alpha[1]) / 5;   /* 1/5*alpha_0 + 4/5*alpha_1 */
		alpha[6] = 0x00;
		alpha[7] = 0xFF;
	}

	indices = block >> 16;
	for (int i = 0; i < 4; ++i)
	{
		for (int j = 0; j < 4; ++j)
		{
			decompressed[j * 4] = alpha[indices & 0x07];
			indices >>= 3;
		}
		decompressed += destinationPitch;
	}
}

static inline void bcdec_bc1(const u8* compressedBlock, u8* decompressedBlock, int destinationPitch) {
	bcdec__color_block(compressedBlock, decompressedBlock, destinationPitch, false);
}

static inline void bcdec_bc2(const u8* compressedBlock, u8* decompressedBlock, int destinationPitch) {
	bcdec__color_block(compressedBlock + 8, decompressedBlock, destinationPitch, true);
	bcdec__sharp_alpha_block(reinterpret_cast<const u16*>(compressedBlock), decompressedBlock + 3, destinationPitch);
}

static inline void bcdec_bc3(const u8* compressedBlock, u8* decompressedBlock, int destinationPitch) {
	bcdec__color_block(compressedBlock + 8, decompressedBlock, destinationPitch, true);
	bcdec__smooth_alpha_block(compressedBlock, decompressedBlock + 3, destinationPitch);
}

