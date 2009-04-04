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

#ifndef __MPEG_H__
#define __MPEG_H__

/* macroblock modes */
#define MACROBLOCK_INTRA 1
#define MACROBLOCK_PATTERN 2
#define MACROBLOCK_MOTION_BACKWARD 4
#define MACROBLOCK_MOTION_FORWARD 8
#define MACROBLOCK_QUANT 16
#define DCT_TYPE_INTERLACED 32
/* motion_type */
#define MOTION_TYPE_SHIFT 6
#define MOTION_TYPE_MASK (3*64)
#define MOTION_TYPE_BASE 64
#define MC_FIELD (1*64)
#define MC_FRAME (2*64)
#define MC_16X8 (2*64)
#define MC_DMV (3*64)

/* picture structure */
#define TOP_FIELD 1
#define BOTTOM_FIELD 2
#define FRAME_PICTURE 3

/* picture coding type */
#define I_TYPE 1
#define P_TYPE 2
#define B_TYPE 3
#define D_TYPE 4

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

struct rgb16{
	unsigned short r:5, g:5, b:5, a:1;
};

struct macroblock_rgb16{
	struct rgb16	c[16][16];
};

struct decoder_t {
	/* first, state that carries information from one macroblock to the */
	/* next inside a slice, and is never used outside of mpeg2_slice() */

	/* DCT coefficients - should be kept aligned ! */
	s16 DCTblock[64];

	/* bit parsing stuff */
	u32 bitstream_buf;		/* current 32 bit working set */
	int bitstream_bits;			/* used bits in working set */
	u8 * bitstream_ptr;			/* buffer with stream data; 128 bits buffer */

	struct macroblock_8		*mb8;
	struct macroblock_16	*mb16;
	struct macroblock_rgb32	*rgb32;
	struct macroblock_rgb16	*rgb16;

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

void mpeg2sliceIDEC(void* pdone);
void mpeg2_slice(void* pdone);
int get_macroblock_address_increment(decoder_t * const decoder);
int get_macroblock_modes (decoder_t * const decoder);

extern int get_motion_delta (decoder_t * const decoder, const int f_code);
extern int get_dmv (decoder_t * const decoder);

extern int non_linear_quantizer_scale[];
extern decoder_t g_decoder;

void __fastcall ipu_csc(macroblock_8 *mb8, macroblock_rgb32 *rgb32, int sgn);
void __fastcall ipu_dither(macroblock_8 *mb8, macroblock_rgb16 *rgb16, int dte);
void __fastcall ipu_dither2(const macroblock_rgb32* rgb32, macroblock_rgb16 *rgb16, int dte);
void __fastcall ipu_vq(macroblock_rgb16 *rgb16, u8* indx4);
void __fastcall ipu_copy(const macroblock_8 *mb8, macroblock_16 *mb16);

int slice (decoder_t * const decoder, u8 * buffer);
/* idct.c */
void mpeg2_idct_init ();

#ifdef _MSC_VER
#define BigEndian(out, in) out = _byteswap_ulong(in)
#else
#define BigEndian(out, in) out = __builtin_bswap32(in) // or we could use the asm function bswap...
#endif
	
#endif//__MPEG_H__
