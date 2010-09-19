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

// the IPU is fixed to 16 byte strides (128-bit / QWC resolution):
static const uint decoder_stride = 16;

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
	u8 Y[16][16];		//0
	u8 Cb[8][8];		//1
	u8 Cr[8][8];		//2
};

struct macroblock_16{
	s16 Y[16][16];			//0
	s16 Cb[8][8];			//1
	s16 Cr[8][8];			//2
};

struct macroblock_rgb32{
	struct {
		u8 r, g, b, a;
	} c[16][16];
};

struct rgb16_t{
	u16 r:5, g:5, b:5, a:1;
};

struct macroblock_rgb16{
	rgb16_t	c[16][16];
};

struct decoder_t {
	/* first, state that carries information from one macroblock to the */
	/* next inside a slice, and is never used outside of mpeg2_slice() */

	/* DCT coefficients - should be kept aligned ! */
	s16 DCTblock[64];

	u8 niq[64];			//non-intraquant matrix (sequence header)
	u8 iq[64];			//intraquant matrix (sequence header)

	macroblock_8 mb8;
	macroblock_16 mb16;
	macroblock_rgb32 rgb32;
	macroblock_rgb16 rgb16;

	uint ipu0_data;		// amount of data in the output macroblock (in QWC)
	uint ipu0_idx;

	/* bit parsing stuff */
	//u32 bitstream_buf;		/* current 32 bit working set */
	//int bitstream_bits;			/* used bits in working set */

	int quantizer_scale;	/* remove */
	int dmv_offset;		/* remove */

	/* now non-slice-specific information */

	/* picture header stuff */

	/* what type of picture this is (I, P, B, D) */
	int coding_type;

	/* picture coding extension stuff */

	/* predictor for DC coefficients in intra blocks */
	s16 dc_dct_pred[3];

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

	/* the zigzag scan we're supposed to be using, true for alt, false for normal */
	bool scantype;

	int second_field;

	int mpeg1;

	template< typename T >
	void SetOutputTo( T& obj )
	{
		uint mb_offset = ((uptr)&obj - (uptr)&mb8);
		pxAssume( (mb_offset & 15) == 0 );
		ipu0_idx	= mb_offset / 16;
		ipu0_data	= sizeof(obj)/16;
	}

	u128* GetIpuDataPtr()
	{
		return ((u128*)&mb8) + ipu0_idx;
	}
	
	void AdvanceIpuDataBy(uint amt)
	{
		pxAssumeDev(ipu0_data>=amt, "IPU FIFO Overflow on advance!" );
		ipu0_idx += amt;
		ipu0_data -= amt;
	}
	
	__fi bool ReadIpuData(u128* out);
};

struct mpeg2_scan_pack
{
	u8 norm[64];
	u8 alt[64];

	mpeg2_scan_pack();
};

extern int bitstream_init ();
extern u32 UBITS(uint bits);
extern s32 SBITS(uint bits);

extern void mpeg2_idct_copy(s16 * block, u8* dest, int stride);
extern void mpeg2_idct_add(int last, s16 * block, s16* dest, int stride);

#define IDEC	0
#define BDEC	1

extern bool mpeg2sliceIDEC();
extern bool mpeg2_slice();
extern int get_macroblock_address_increment();
extern int get_macroblock_modes();

extern int get_motion_delta(const int f_code);
extern int get_dmv();

extern void ipu_csc(macroblock_8& mb8, macroblock_rgb32& rgb32, int sgn);
extern void ipu_dither(const macroblock_rgb32& rgb32, macroblock_rgb16& rgb16, int dte);
extern void ipu_vq(macroblock_rgb16& rgb16, u8* indx4);

extern int slice (u8 * buffer);

#ifdef _MSC_VER
#define BigEndian(in) _byteswap_ulong(in)
#else
#define BigEndian(in) __builtin_bswap32(in) // or we could use the asm function bswap...
#endif

#ifdef _MSC_VER
#define BigEndian64(in) _byteswap_uint64(in)
#else
#define BigEndian64(in) __builtin_bswap64(in) // or we could use the asm function bswap...
#endif

extern __aligned16 const mpeg2_scan_pack mpeg2_scan;
extern const int non_linear_quantizer_scale[];

// The IPU can only do one task at once and never uses other buffers so all mpeg state variables
// are made available to mpeg/vlc modules as globals here:

extern __aligned16 tIPU_BP g_BP;
extern __aligned16 decoder_t decoder;

