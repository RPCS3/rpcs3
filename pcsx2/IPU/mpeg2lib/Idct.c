/*
 * idct.c
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

//#include <stdlib.h>
#include "Common.h"

#define W1 2841 /* 2048*sqrt (2)*cos (1*pi/16) */
#define W2 2676 /* 2048*sqrt (2)*cos (2*pi/16) */
#define W3 2408 /* 2048*sqrt (2)*cos (3*pi/16) */
#define W5 1609 /* 2048*sqrt (2)*cos (5*pi/16) */ 
#define W6 1108 /* 2048*sqrt (2)*cos (6*pi/16) */
#define W7 565  /* 2048*sqrt (2)*cos (7*pi/16) */
#define clp(val,res)	res = (val < 0) ? 0 : ((val > 255) ? 255 : val);
#define clp2(val,res)	res = (val < -255) ? -255 : ((val > 255) ? 255 : val);
/* idct main entry point  */
void (* mpeg2_idct_copy) (s16 * block, u8 * dest, int stride);
/* JayteeMaster: changed dest to 16 bit signed */
void (* mpeg2_idct_add) (int last, s16 * block,
			 /*u8*/s16 * dest, int stride);

/*
 * In legal streams, the IDCT output should be between -384 and +384.
 * In corrupted streams, it is possible to force the IDCT output to go
 * to +-3826 - this is the worst case for a column IDCT where the
 * column inputs are 16-bit values.
 */
static u8 clip_lut[1024];
#define CLIP(i) ((clip_lut+384)[(i)])

#if 0
#define BUTTERFLY(t0,t1,W0,W1,d0,d1)	\
do {					\
    t0 = W0*d0 + W1*d1;			\
    t1 = W0*d1 - W1*d0;			\
} while (0)
#else
#define BUTTERFLY(t0,t1,W0,W1,d0,d1)	\
do {					\
    int tmp = W0 * (d0 + d1);		\
    t0 = tmp + (W1 - W0) * d1;		\
    t1 = tmp - (W1 + W0) * d0;		\
} while (0)
#endif

static void idct_row (s16 * const block)
{
    int d0, d1, d2, d3;
    int a0, a1, a2, a3, b0, b1, b2, b3;
    int t0, t1, t2, t3;

    /* shortcut */
    if (!(block[1] | ((s32 *)block)[1] | ((s32 *)block)[2] |
		  ((s32 *)block)[3])) {
	u32 tmp = (u16) (block[0] << 3);
	tmp |= tmp << 16;
	((s32 *)block)[0] = tmp;
	((s32 *)block)[1] = tmp;
	((s32 *)block)[2] = tmp;
	((s32 *)block)[3] = tmp;
	return;
    }

    d0 = (block[0] << 11) + 128;
    d1 = block[1];
    d2 = block[2] << 11;
    d3 = block[3];
    t0 = d0 + d2;
    t1 = d0 - d2;
    BUTTERFLY (t2, t3, W6, W2, d3, d1);
    a0 = t0 + t2;
    a1 = t1 + t3;
    a2 = t1 - t3;
    a3 = t0 - t2;

    d0 = block[4];
    d1 = block[5];
    d2 = block[6];
    d3 = block[7];
    BUTTERFLY (t0, t1, W7, W1, d3, d0);
    BUTTERFLY (t2, t3, W3, W5, d1, d2);
    b0 = t0 + t2;
    b3 = t1 + t3;
    t0 -= t2;
    t1 -= t3;
    b1 = ((t0 + t1) * 181) >> 8;
    b2 = ((t0 - t1) * 181) >> 8;

    block[0] = (a0 + b0) >> 8;
    block[1] = (a1 + b1) >> 8;
    block[2] = (a2 + b2) >> 8;
    block[3] = (a3 + b3) >> 8;
    block[4] = (a3 - b3) >> 8;
    block[5] = (a2 - b2) >> 8;
    block[6] = (a1 - b1) >> 8;
    block[7] = (a0 - b0) >> 8;
}

static void idct_col (s16 * const block)
{
    int d0, d1, d2, d3;
    int a0, a1, a2, a3, b0, b1, b2, b3;
    int t0, t1, t2, t3;

    d0 = (block[8*0] << 11) + 65536;
    d1 = block[8*1];
    d2 = block[8*2] << 11;
    d3 = block[8*3];
    t0 = d0 + d2;
    t1 = d0 - d2;
    BUTTERFLY (t2, t3, W6, W2, d3, d1);
    a0 = t0 + t2;
    a1 = t1 + t3;
    a2 = t1 - t3;
    a3 = t0 - t2;

    d0 = block[8*4];
    d1 = block[8*5];
    d2 = block[8*6];
    d3 = block[8*7];
    BUTTERFLY (t0, t1, W7, W1, d3, d0);
    BUTTERFLY (t2, t3, W3, W5, d1, d2);
    b0 = t0 + t2;
    b3 = t1 + t3;
    t0 = (t0 - t2) >> 8;
    t1 = (t1 - t3) >> 8;
    b1 = (t0 + t1) * 181;
    b2 = (t0 - t1) * 181;

    block[8*0] = (a0 + b0) >> 17;
    block[8*1] = (a1 + b1) >> 17;
    block[8*2] = (a2 + b2) >> 17;
    block[8*3] = (a3 + b3) >> 17;
    block[8*4] = (a3 - b3) >> 17;
    block[8*5] = (a2 - b2) >> 17;
    block[8*6] = (a1 - b1) >> 17;
    block[8*7] = (a0 - b0) >> 17;
}

static void mpeg2_idct_copy_c (s16 * block, u8 * dest,
			       const int stride)
{
    int i;

    for (i = 0; i < 8; i++)
	idct_row (block + 8 * i);
    for (i = 0; i < 8; i++)
	idct_col (block + i);
    do {
	dest[0] = CLIP (block[0]);
	dest[1] = CLIP (block[1]);
	dest[2] = CLIP (block[2]);
	dest[3] = CLIP (block[3]);
	dest[4] = CLIP (block[4]);
	dest[5] = CLIP (block[5]);
	dest[6] = CLIP (block[6]);
	dest[7] = CLIP (block[7]);

	block[0] = 0;	block[1] = 0;	block[2] = 0;	block[3] = 0;
	block[4] = 0;	block[5] = 0;	block[6] = 0;	block[7] = 0;

	dest += stride;
	block += 8;
    } while (--i);
}

/* JayteeMaster: changed dest to 16 bit signed */
static void mpeg2_idct_add_c (const int last, s16 * block,
			      /*u8*/s16 * dest, const int stride)
{
    int i;

    if (last != 129 || (block[0] & 7) == 4) {
	for (i = 0; i < 8; i++)
	    idct_row (block + 8 * i);
	for (i = 0; i < 8; i++)
	    idct_col (block + i);
	do {
	    dest[0] = block[0];
	    dest[1] = block[1];
	    dest[2] = block[2];
	    dest[3] = block[3];
	    dest[4] = block[4];
	    dest[5] = block[5];
	    dest[6] = block[6];
	    dest[7] = block[7];

	    block[0] = 0;	block[1] = 0;	block[2] = 0;	block[3] = 0;
	    block[4] = 0;	block[5] = 0;	block[6] = 0;	block[7] = 0;

	    dest += stride;
	    block += 8;
	} while (--i);
    } else {
	int DC;

	DC = (block[0] + 4) >> 3;
	block[0] = block[63] = 0;
	i = 8;
	do {
	    dest[0] = DC;
	    dest[1] = DC;
	    dest[2] = DC;
	    dest[3] = DC;
	    dest[4] = DC;
	    dest[5] = DC;
	    dest[6] = DC;
	    dest[7] = DC;
	    dest += stride;
	} while (--i);
    }
}

u8 mpeg2_scan_norm[64] = {
    /* Zig-Zag scan pattern */
     0,  1,  8, 16,  9,  2,  3, 10, 17, 24, 32, 25, 18, 11,  4,  5,
    12, 19, 26, 33, 40, 48, 41, 34, 27, 20, 13,  6,  7, 14, 21, 28,
    35, 42, 49, 56, 57, 50, 43, 36, 29, 22, 15, 23, 30, 37, 44, 51,
    58, 59, 52, 45, 38, 31, 39, 46, 53, 60, 61, 54, 47, 55, 62, 63
};

u8 mpeg2_scan_alt[64] = {
    /* Alternate scan pattern */
     0, 8,  16, 24,  1,  9,  2, 10, 17, 25, 32, 40, 48, 56, 57, 49,
    41, 33, 26, 18,  3, 11,  4, 12, 19, 27, 34, 42, 50, 58, 35, 43,
    51, 59, 20, 28,  5, 13,  6, 14, 21, 29, 36, 44, 52, 60, 37, 45,
    53, 61, 22, 30,  7, 15, 23, 31, 38, 46, 54, 62, 39, 47, 55, 63
};

/* idct_mmx.c */
void mpeg2_idct_copy_mmxext (s16 * block, u8 * dest, int stride);
void mpeg2_idct_add_mmxext (int last, s16 * block,
			   s16 * dest, int stride);
void mpeg2_idct_copy_mmx (s16 * block, u8 * dest, int stride);
void mpeg2_idct_add_mmx (int last, s16 * block,
			   s16 * dest, int stride);
void mpeg2_idct_mmx_init (void);

void mpeg2_idct_init()
{
#if !defined(_MSC_VER) || _MSC_VER < 1400 // ignore vc2005 and beyond
	int i, j;
	
/*	if(hasMultimediaExtensions == 1)
	{
		mpeg2_idct_copy = mpeg2_idct_copy_mmx;
		mpeg2_idct_add = mpeg2_idct_add_mmx;
		mpeg2_idct_mmx_init ();
	}else if(hasMultimediaExtensionsExt == 1)
	{
		mpeg2_idct_copy = mpeg2_idct_copy_mmxext;
		mpeg2_idct_add = mpeg2_idct_add_mmxext;
		mpeg2_idct_mmx_init ();
	}else*/
	{
		mpeg2_idct_copy = mpeg2_idct_copy_c;
		mpeg2_idct_add = mpeg2_idct_add_c;
		for (i = -384; i < 640; i++)
			clip_lut[i+384] = (i < 0) ? 0 : ((i > 255) ? 255 : i);
		for (i = 0; i < 64; i++) {
			j = mpeg2_scan_norm[i];
			mpeg2_scan_norm[i] = ((j & 0x36) >> 1) | ((j & 0x09) << 2);
			j = mpeg2_scan_alt[i];
			mpeg2_scan_alt[i] = ((j & 0x36) >> 1) | ((j & 0x09) << 2);
		}
	}

#else //blah vcnet2005 idiocity :D
	   int i,j;
  		mpeg2_idct_copy = mpeg2_idct_copy_c;
		mpeg2_idct_add = mpeg2_idct_add_c;
		for (i = -384; i < 640; i++)
			clip_lut[i+384] = (i < 0) ? 0 : ((i > 255) ? 255 : i);
		for (i = 0; i < 64; i++) {
			j = mpeg2_scan_norm[i];
			mpeg2_scan_norm[i] = ((j & 0x36) >> 1) | ((j & 0x09) << 2);
			j = mpeg2_scan_alt[i];
			mpeg2_scan_alt[i] = ((j & 0x36) >> 1) | ((j & 0x09) << 2);
		}
#endif
}
