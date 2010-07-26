/*
 * Mpeg.h
 * Copyright (C) 2000-2002 Michel Lespinasse <walken@zoy.org>
 * Copyright (C) 1999-2000 Aaron Holtzman <aholtzma@ess.engr.uvic.ca>
 * Modified by Florin for PCSX2 emu
 *
 * This file is part of mpeg2dec, a free MPEG-2 video stream decoder.
 * See http://libmpeg2.sourceforge.net/ for updates.
 *
 * mpeg2dec is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * mpeg2dec is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#pragma once

#include <xmmintrin.h>

template< typename T >
__noinline void memzero_sse_a( T& dest )
{
#define MZFqwc (sizeof(dest)/16)

	C_ASSERT( (sizeof(dest) & 0xf) == 0 );

	__m128 zeroreg = _mm_setzero_ps();

	float (*destxmm)[4] = (float(*)[4])&dest;

#define StoreDestIdx(idx) case idx: _mm_store_ps(&destxmm[idx][0], zeroreg)

	switch( MZFqwc & 0x07 )
	{
		StoreDestIdx(0x07);
		StoreDestIdx(0x06);
		StoreDestIdx(0x05);
		StoreDestIdx(0x04);
		StoreDestIdx(0x03);
		StoreDestIdx(0x02);
		StoreDestIdx(0x01);
	}

	destxmm += (MZFqwc & 0x07);
	for( uint i=0; i<MZFqwc / 8; ++i, destxmm+=8 )
	{
		_mm_store_ps(&destxmm[0][0], zeroreg);
		_mm_store_ps(&destxmm[1][0], zeroreg);
		_mm_store_ps(&destxmm[2][0], zeroreg);
		_mm_store_ps(&destxmm[3][0], zeroreg);
		_mm_store_ps(&destxmm[4][0], zeroreg);
		_mm_store_ps(&destxmm[5][0], zeroreg);
		_mm_store_ps(&destxmm[6][0], zeroreg);
		_mm_store_ps(&destxmm[7][0], zeroreg);
	}

#undef MZFqwc
};


enum macroblock_modes
{
	MACROBLOCK_INTRA = 1,
	MACROBLOCK_PATTERN = 2,
	MACROBLOCK_MOTION_BACKWARD = 4,
	MACROBLOCK_MOTION_FORWARD = 8,
	MACROBLOCK_QUANT = 16,
	DCT_TYPE_INTERLACED = 32
};

enum motion_type
{
	MOTION_TYPE_SHIFT = 6,
	MOTION_TYPE_MASK = (3*64),
	MOTION_TYPE_BASE = 64,
	MC_FIELD = (1*64),
	MC_FRAME = (2*64),
	MC_16X8 = (2*64),
	MC_DMV = (3*64)
};

/* picture structure */
enum picture_structure
{
	TOP_FIELD = 1,
	BOTTOM_FIELD = 2,
	FRAME_PICTURE = 3
};

/* picture coding type */
enum picture_coding_type
{
	I_TYPE  =1,
	P_TYPE = 2,
	B_TYPE = 3,
	D_TYPE = 4
};

struct macroblock_8{
	unsigned char Y[16][16];	//0
	unsigned char Cb[8][8];		//1
	unsigned char Cr[8][8];		//2
};

struct macroblock_16{
	short Y[16][16];			//0
	short Cb[8][8];				//1
	short Cr[8][8];				//2
};

struct macroblock_rgb32{
	struct {
		unsigned char r, g, b, a;
	} c[16][16];
};

struct rgb16_t{
	unsigned short r:5, g:5, b:5, a:1;
};

struct macroblock_rgb16{
	rgb16_t	c[16][16];
};

struct decoder_t {
	/* first, state that carries information from one macroblock to the */
	/* next inside a slice, and is never used outside of mpeg2_slice() */

	/* DCT coefficients - should be kept aligned ! */
	s16 DCTblock[64];

	/* bit parsing stuff */
	u32 bitstream_buf;		/* current 32 bit working set */
	int bitstream_bits;			/* used bits in working set */

	int stride;

	/* predictor for DC coefficients in intra blocks */
	s16 dc_dct_pred[3];

	int quantizer_scale;	/* remove */
	int dmv_offset;		/* remove */

	/* now non-slice-specific information */

	/* sequence header stuff */
	u8 *intra_quantizer_matrix;
	u8 *non_intra_quantizer_matrix;

	/* picture header stuff */

	/* what type of picture this is (I, P, B, D) */
	int coding_type;

	/* picture coding extension stuff */

	/* quantization factor for intra dc coefficients */
	int intra_dc_precision;
	/* top/bottom/both fields */
	int picture_structure;
	/* bool to indicate all predictions are frame based */
	int frame_pred_frame_dct;
	/* bool to indicate whether intra blocks have motion vectors */
	/* (for concealment) */
	int concealment_motion_vectors;
	/* bit to indicate which quantization table to use */
	int q_scale_type;
	/* bool to use different vlc tables */
	int intra_vlc_format;
	/* used for DMV MC */
	int top_field_first;
	// Pseudo Sign Offset
	int sgn;
	// Dither Enable
	int dte;
	// Output Format
	int ofm;
	// Macroblock count
	int mbc;
	// Macroblock type
	int macroblock_modes;
	// DC Reset
	int dcr;
	// Coded block pattern
	int coded_block_pattern;

	/* stuff derived from bitstream */

	/* pointer to the zigzag scan we're supposed to be using */
	const u8 * scan;

	int second_field;

	int mpeg1;
};

extern void (__fastcall *mpeg2_idct_copy) (s16 * block, u8* dest, int stride);
extern void (__fastcall *mpeg2_idct_add) (int last, s16 * block, s16* dest, int stride);

#define IDEC	0
#define BDEC	1

extern bool mpeg2sliceIDEC();
extern bool mpeg2_slice();
extern int get_macroblock_address_increment();
extern int get_macroblock_modes();

extern int get_motion_delta(const int f_code);
extern int get_dmv();

extern int non_linear_quantizer_scale[];

extern void ipu_csc(macroblock_8& mb8, macroblock_rgb32& rgb32, int sgn);
extern void ipu_dither(const macroblock_rgb32& rgb32, macroblock_rgb16& rgb16, int dte);
extern void ipu_vq(macroblock_rgb16& rgb16, u8* indx4);
extern void ipu_copy(const macroblock_8& mb8, macroblock_16& mb16);

extern int slice (u8 * buffer);
/* idct.c */
extern void mpeg2_idct_init ();

#ifdef _MSC_VER
#define BigEndian(out, in) out = _byteswap_ulong(in)
#else
#define BigEndian(out, in) out = __builtin_bswap32(in) // or we could use the asm function bswap...
#endif

#ifdef _MSC_VER
#define BigEndian64(out, in) out = _byteswap_uint64(in)
#else
#define BigEndian64(out, in) out = __builtin_bswap64(in) // or we could use the asm function bswap...
#endif

// The IPU can only do one task at once and never uses other buffers so all mpeg state variables
// are made available to mpeg/vlc modules as globals here:

extern __aligned16 tIPU_BP g_BP;
extern __aligned16 decoder_t decoder;
extern __aligned16 macroblock_8 mb8;
extern __aligned16 macroblock_16 mb16;
extern __aligned16 macroblock_rgb32 rgb32;
extern __aligned16 macroblock_rgb16 rgb16;

