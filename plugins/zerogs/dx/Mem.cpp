/*  ZeroGS
 *  Copyright (C) 2005-2006 zerofrog@gmail.com
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <d3dx9.h>

#include "GS.h"
#include "Mem.h"
#include "zerogs.h"
#include "targets.h"
#include "x86.h"

u32 g_blockTable32[4][8] = {
	{  0,  1,  4,  5, 16, 17, 20, 21},
	{  2,  3,  6,  7, 18, 19, 22, 23},
	{  8,  9, 12, 13, 24, 25, 28, 29},
	{ 10, 11, 14, 15, 26, 27, 30, 31}
};

u32 g_blockTable32Z[4][8] = {
	{ 24, 25, 28, 29,  8,  9, 12, 13},
	{ 26, 27, 30, 31, 10, 11, 14, 15},
	{ 16, 17, 20, 21,  0,  1,  4,  5},
	{ 18, 19, 22, 23,  2,  3,  6,  7}
};

u32 g_blockTable16[8][4] = {
	{  0,  2,  8, 10 },
	{  1,  3,  9, 11 },
	{  4,  6, 12, 14 },
	{  5,  7, 13, 15 },
	{ 16, 18, 24, 26 },
	{ 17, 19, 25, 27 },
	{ 20, 22, 28, 30 },
	{ 21, 23, 29, 31 }
};

u32 g_blockTable16S[8][4] = {
	{  0,  2, 16, 18 },
	{  1,  3, 17, 19 },
	{  8, 10, 24, 26 },
	{  9, 11, 25, 27 },
	{  4,  6, 20, 22 },
	{  5,  7, 21, 23 },
	{ 12, 14, 28, 30 },
	{ 13, 15, 29, 31 }
};

u32 g_blockTable16Z[8][4] = {
	{ 24, 26, 16, 18 },
	{ 25, 27, 17, 19 },
	{ 28, 30, 20, 22 },
	{ 29, 31, 21, 23 },
	{  8, 10,  0,  2 },
	{  9, 11,  1,  3 },
	{ 12, 14,  4,  6 },
	{ 13, 15,  5,  7 }
};

u32 g_blockTable16SZ[8][4] = {
	{ 24, 26,  8, 10 },
	{ 25, 27,  9, 11 },
	{ 16, 18,  0,  2 },
	{ 17, 19,  1,  3 },
	{ 28, 30, 12, 14 },
	{ 29, 31, 13, 15 },
	{ 20, 22,  4,  6 },
	{ 21, 23,  5,  7 }
};

u32 g_blockTable8[4][8] = {
	{  0,  1,  4,  5, 16, 17, 20, 21},
	{  2,  3,  6,  7, 18, 19, 22, 23},
	{  8,  9, 12, 13, 24, 25, 28, 29},
	{ 10, 11, 14, 15, 26, 27, 30, 31}
};

u32 g_blockTable4[8][4] = {
	{  0,  2,  8, 10 },
	{  1,  3,  9, 11 },
	{  4,  6, 12, 14 },
	{  5,  7, 13, 15 },
	{ 16, 18, 24, 26 },
	{ 17, 19, 25, 27 },
	{ 20, 22, 28, 30 },
	{ 21, 23, 29, 31 }
};

u32 g_columnTable32[8][8] = {
	{  0,  1,  4,  5,  8,  9, 12, 13 },
	{  2,  3,  6,  7, 10, 11, 14, 15 },
	{ 16, 17, 20, 21, 24, 25, 28, 29 },
	{ 18, 19, 22, 23, 26, 27, 30, 31 },
	{ 32, 33, 36, 37, 40, 41, 44, 45 },
	{ 34, 35, 38, 39, 42, 43, 46, 47 },
	{ 48, 49, 52, 53, 56, 57, 60, 61 },
	{ 50, 51, 54, 55, 58, 59, 62, 63 },
};

u32 g_columnTable16[8][16] = {
	{   0,   2,   8,  10,  16,  18,  24,  26, 
		1,   3,   9,  11,  17,  19,  25,  27 },
	{   4,   6,  12,  14,  20,  22,  28,  30, 
		5,   7,  13,  15,  21,  23,  29,  31 },
	{  32,  34,  40,  42,  48,  50,  56,  58,
	   33,  35,  41,  43,  49,  51,  57,  59 },
	{  36,  38,  44,  46,  52,  54,  60,  62,
	   37,  39,  45,  47,  53,  55,  61,  63 },
	{  64,  66,  72,  74,  80,  82,  88,  90,
	   65,  67,  73,  75,  81,  83,  89,  91 },
	{  68,  70,  76,  78,  84,  86,  92,  94,
	   69,  71,  77,  79,  85,  87,  93,  95 },
	{  96,  98, 104, 106, 112, 114, 120, 122,
	   97,  99, 105, 107, 113, 115, 121, 123 },
	{ 100, 102, 108, 110, 116, 118, 124, 126,
	  101, 103, 109, 111, 117, 119, 125, 127 },
};

u32 g_columnTable8[16][16] = {
	{   0,   4,  16,  20,  32,  36,  48,  52,	// column 0
		2,   6,  18,  22,  34,  38,  50,  54 },
	{   8,  12,  24,  28,  40,  44,  56,  60,
	   10,  14,  26,  30,  42,  46,  58,  62 },
	{  33,  37,  49,  53,   1,   5,  17,  21,
	   35,  39,  51,  55,   3,   7,  19,  23 },
	{  41,  45,  57,  61,   9,  13,  25,  29,
	   43,  47,  59,  63,  11,  15,  27,  31 },
	{  96, 100, 112, 116,  64,  68,  80,  84, 	// column 1
	   98, 102, 114, 118,  66,  70,  82,  86 },
	{ 104, 108, 120, 124,  72,  76,  88,  92, 
	  106, 110, 122, 126,  74,  78,  90,  94 },
	{  65,  69,  81,  85,  97, 101, 113, 117,
	   67,  71,  83,  87,  99, 103, 115, 119 },
	{  73,  77,  89,  93, 105, 109, 121, 125,
	   75,  79,  91,  95, 107, 111, 123, 127 },
	{ 128, 132, 144, 148, 160, 164, 176, 180,	// column 2
	  130, 134, 146, 150, 162, 166, 178, 182 },
	{ 136, 140, 152, 156, 168, 172, 184, 188,
	  138, 142, 154, 158, 170, 174, 186, 190 },
	{ 161, 165, 177, 181, 129, 133, 145, 149,
	  163, 167, 179, 183, 131, 135, 147, 151 },
	{ 169, 173, 185, 189, 137, 141, 153, 157,
	  171, 175, 187, 191, 139, 143, 155, 159 },
	{ 224, 228, 240, 244, 192, 196, 208, 212,	// column 3
	  226, 230, 242, 246, 194, 198, 210, 214 },
	{ 232, 236, 248, 252, 200, 204, 216, 220,
	  234, 238, 250, 254, 202, 206, 218, 222 },
	{ 193, 197, 209, 213, 225, 229, 241, 245,
	  195, 199, 211, 215, 227, 231, 243, 247 },
	{ 201, 205, 217, 221, 233, 237, 249, 253,
	  203, 207, 219, 223, 235, 239, 251, 255 },
};

u32 g_columnTable4[16][32] = {
	{   0,   8,  32,  40,  64,  72,  96, 104,	// column 0
		2,  10,  34,  42,  66,  74,  98, 106,
		4,  12,  36,  44,  68,  76, 100, 108,
		6,  14,  38,  46,  70,  78, 102, 110 },
	{  16,  24,  48,  56,  80,  88, 112, 120,
	   18,  26,  50,  58,  82,  90, 114, 122,
	   20,  28,  52,  60,  84,  92, 116, 124,
	   22,  30,  54,  62,  86,  94, 118, 126 },
	{  65,  73,  97, 105,   1,   9,  33,  41,
	   67,  75,  99, 107,   3,  11,  35,  43,
	   69,  77, 101, 109,   5,  13,  37,  45, 
	   71,  79, 103, 111,   7,  15,  39,  47 },
	{  81,  89, 113, 121,  17,  25,  49,  57,
	   83,  91, 115, 123,  19,  27,  51,  59,
	   85,  93, 117, 125,  21,  29,  53,  61,
	   87,  95, 119, 127,  23,  31,  55,  63 },
	{ 192, 200, 224, 232, 128, 136, 160, 168,	// column 1
	  194, 202, 226, 234, 130, 138, 162, 170,
	  196, 204, 228, 236, 132, 140, 164, 172,
	  198, 206, 230, 238, 134, 142, 166, 174 },
	{ 208, 216, 240, 248, 144, 152, 176, 184,
	  210, 218, 242, 250, 146, 154, 178, 186,
	  212, 220, 244, 252, 148, 156, 180, 188,
	  214, 222, 246, 254, 150, 158, 182, 190 },
	{ 129, 137, 161, 169, 193, 201, 225, 233,
	  131, 139, 163, 171, 195, 203, 227, 235,
	  133, 141, 165, 173, 197, 205, 229, 237, 
	  135, 143, 167, 175, 199, 207, 231, 239 },
	{ 145, 153, 177, 185, 209, 217, 241, 249,
	  147, 155, 179, 187, 211, 219, 243, 251,
	  149, 157, 181, 189, 213, 221, 245, 253,
	  151, 159, 183, 191, 215, 223, 247, 255 },
	{ 256, 264, 288, 296, 320, 328, 352, 360,	// column 2
	  258, 266, 290, 298, 322, 330, 354, 362,
	  260, 268, 292, 300, 324, 332, 356, 364,
	  262, 270, 294, 302, 326, 334, 358, 366 },
	{ 272, 280, 304, 312, 336, 344, 368, 376,
	  274, 282, 306, 314, 338, 346, 370, 378,
	  276, 284, 308, 316, 340, 348, 372, 380,
	  278, 286, 310, 318, 342, 350, 374, 382 },
	{ 321, 329, 353, 361, 257, 265, 289, 297,
	  323, 331, 355, 363, 259, 267, 291, 299,
	  325, 333, 357, 365, 261, 269, 293, 301, 
	  327, 335, 359, 367, 263, 271, 295, 303 },
	{ 337, 345, 369, 377, 273, 281, 305, 313,
	  339, 347, 371, 379, 275, 283, 307, 315,
	  341, 349, 373, 381, 277, 285, 309, 317,
	  343, 351, 375, 383, 279, 287, 311, 319 },
	{ 448, 456, 480, 488, 384, 392, 416, 424,	// column 3
	  450, 458, 482, 490, 386, 394, 418, 426,
	  452, 460, 484, 492, 388, 396, 420, 428,
	  454, 462, 486, 494, 390, 398, 422, 430 },
	{ 464, 472, 496, 504, 400, 408, 432, 440,
	  466, 474, 498, 506, 402, 410, 434, 442,
	  468, 476, 500, 508, 404, 412, 436, 444,
	  470, 478, 502, 510, 406, 414, 438, 446 },
	{ 385, 393, 417, 425, 449, 457, 481, 489,
	  387, 395, 419, 427, 451, 459, 483, 491,
	  389, 397, 421, 429, 453, 461, 485, 493, 
	  391, 399, 423, 431, 455, 463, 487, 495 },
	{ 401, 409, 433, 441, 465, 473, 497, 505,
	  403, 411, 435, 443, 467, 475, 499, 507,
	  405, 413, 437, 445, 469, 477, 501, 509,
	  407, 415, 439, 447, 471, 479, 503, 511 },
};

u32 g_pageTable32[32][64];
u32 g_pageTable32Z[32][64];
u32 g_pageTable16[64][64];
u32 g_pageTable16S[64][64];
u32 g_pageTable16Z[64][64];
u32 g_pageTable16SZ[64][64];
u32 g_pageTable8[64][128];
u32 g_pageTable4[128][128];

BLOCK m_Blocks[0x40]; // do so blocks are indexable
static __aligned16 u32 tempblock[64];

#define DSTPSM gs.dstbuf.psm

#define START_HOSTLOCAL() \
	assert( gs.imageTransfer == 0 ); \
	u8* pstart = ZeroGS::g_pbyGSMemory + gs.dstbuf.bp*256; \
	\
	const u8* pendbuf = (const u8*)pbyMem + nQWordSize*4; \
	int i = gs.imageY, j = gs.imageX; \

extern BOOL g_bSaveTrans;

#define END_HOSTLOCAL() \
End: \
	if( i >= gs.imageEndY ) { \
		assert( gs.imageTransfer == -1 || i == gs.imageEndY ); \
		gs.imageTransfer = -1; \
		/*int start, end; \
		ZeroGS::GetRectMemAddress(start, end, gs.dstbuf.psm, gs.trxpos.dx, gs.trxpos.dy, gs.imageWnew, gs.imageHnew, gs.dstbuf.bp, gs.dstbuf.bw); \
		ZeroGS::g_MemTargs.ClearRange(start, end);*/ \
	} \
	else { \
		/* update new params */ \
		gs.imageY = i; \
		gs.imageX = j; \
	} \

// transfers whole rows
#define TRANSMIT_HOSTLOCAL_Y_(psm, T, widthlimit, endY) { \
	assert( (nSize%widthlimit) == 0 && widthlimit <= 4 ); \
	if( (gs.imageEndX-gs.trxpos.dx)%widthlimit ) { \
		/*GS_LOG("Bad Transmission! %d %d, psm: %d\n", gs.trxpos.dx, gs.imageEndX, DSTPSM);*/ \
		for(; i < endY; ++i) { \
			for(; j < gs.imageEndX && nSize > 0; j += 1, nSize -= 1, pbuf += 1) { \
				/* write as many pixel at one time as possible */ \
				writePixel##psm##_0(pstart, j%2048, i%2048, pbuf[0], gs.dstbuf.bw); \
			} \
		} \
	} \
	for(; i < endY; ++i) { \
		for(; j < gs.imageEndX && nSize > 0; j += widthlimit, nSize -= widthlimit, pbuf += widthlimit) { \
			/* write as many pixel at one time as possible */ \
			if( nSize < widthlimit ) goto End; \
			writePixel##psm##_0(pstart, j%2048, i%2048, pbuf[0], gs.dstbuf.bw); \
			\
			if( widthlimit > 1 ) { \
				writePixel##psm##_0(pstart, (j+1)%2048, i%2048, pbuf[1], gs.dstbuf.bw); \
				\
				if( widthlimit > 2 ) { \
					writePixel##psm##_0(pstart, (j+2)%2048, i%2048, pbuf[2], gs.dstbuf.bw); \
					\
					if( widthlimit > 3 ) { \
						writePixel##psm##_0(pstart, (j+3)%2048, i%2048, pbuf[3], gs.dstbuf.bw); \
					} \
				} \
			} \
		} \
		\
		if( j >= gs.imageEndX ) { assert(j == gs.imageEndX); j = gs.trxpos.dx; } \
		else { assert( gs.imageTransfer == -1 || nSize*sizeof(T)/4 == 0 ); goto End; } \
	} \
} \

// transmit until endX, don't check size since it has already been prevalidated
#define TRANSMIT_HOSTLOCAL_X_(psm, T, widthlimit, blockheight, startX) { \
	for(int tempi = 0; tempi < blockheight; ++tempi) { \
		for(j = startX; j < gs.imageEndX; j++, pbuf++) { \
			writePixel##psm##_0(pstart, j%2048, (i+tempi)%2048, pbuf[0], gs.dstbuf.bw); \
		} \
		pbuf += pitch-fracX; \
	} \
} \

// transfers whole rows
#define TRANSMIT_HOSTLOCAL_Y_24(psm, T, widthlimit, endY) { \
	if( widthlimit != 8 || ((gs.imageEndX-gs.trxpos.dx)%widthlimit) ) { \
		/*GS_LOG("Bad Transmission! %d %d, psm: %d\n", gs.trxpos.dx, gs.imageEndX, DSTPSM);*/ \
		for(; i < endY; ++i) { \
			for(; j < gs.imageEndX && nSize > 0; j += 1, nSize -= 1, pbuf += 3) { \
				writePixel##psm##_0(pstart, j%2048, i%2048, *(u32*)(pbuf), gs.dstbuf.bw); \
			} \
			\
			if( j >= gs.imageEndX ) { assert(gs.imageTransfer == -1 || j == gs.imageEndX); j = gs.trxpos.dx; } \
			else { assert( gs.imageTransfer == -1 || nSize == 0 ); goto End; } \
		} \
	} \
	else { \
		assert( /*(nSize%widthlimit) == 0 &&*/ widthlimit == 8 ); \
		for(; i < endY; ++i) { \
			for(; j < gs.imageEndX && nSize > 0; j += widthlimit, nSize -= widthlimit, pbuf += 3*widthlimit) { \
				if( nSize < widthlimit ) goto End; \
				/* write as many pixel at one time as possible */ \
				writePixel##psm##_0(pstart, j%2048, i%2048, *(u32*)(pbuf+0), gs.dstbuf.bw); \
				writePixel##psm##_0(pstart, (j+1)%2048, i%2048, *(u32*)(pbuf+3), gs.dstbuf.bw); \
				writePixel##psm##_0(pstart, (j+2)%2048, i%2048, *(u32*)(pbuf+6), gs.dstbuf.bw); \
				writePixel##psm##_0(pstart, (j+3)%2048, i%2048, *(u32*)(pbuf+9), gs.dstbuf.bw); \
				writePixel##psm##_0(pstart, (j+4)%2048, i%2048, *(u32*)(pbuf+12), gs.dstbuf.bw); \
				writePixel##psm##_0(pstart, (j+5)%2048, i%2048, *(u32*)(pbuf+15), gs.dstbuf.bw); \
				writePixel##psm##_0(pstart, (j+6)%2048, i%2048, *(u32*)(pbuf+18), gs.dstbuf.bw); \
				writePixel##psm##_0(pstart, (j+7)%2048, i%2048, *(u32*)(pbuf+21), gs.dstbuf.bw); \
			} \
			\
			if( j >= gs.imageEndX ) { assert(gs.imageTransfer == -1 || j == gs.imageEndX); j = gs.trxpos.dx; } \
			else { \
				if( nSize < 0 ) { \
					/* extracted too much */ \
					assert( (nSize%3)==0 && nSize > -24 ); \
					j += nSize/3; \
					nSize = 0; \
				} \
				assert( gs.imageTransfer == -1 || nSize == 0 ); \
				goto End; \
			} \
		} \
	} \
} \

// transmit until endX, don't check size since it has already been prevalidated
#define TRANSMIT_HOSTLOCAL_X_24(psm, T, widthlimit, blockheight, startX) { \
	for(int tempi = 0; tempi < blockheight; ++tempi) { \
		for(j = startX; j < gs.imageEndX; j++, pbuf += 3) { \
			writePixel##psm##_0(pstart, j%2048, (i+tempi)%2048, *(u32*)pbuf, gs.dstbuf.bw); \
		} \
		pbuf += 3*(pitch-fracX); \
	} \
} \

// meant for 4bit transfers
#define TRANSMIT_HOSTLOCAL_Y_4(psm, T, widthlimit, endY) { \
	for(; i < endY; ++i) { \
		for(; j < gs.imageEndX && nSize > 0; j += widthlimit, nSize -= widthlimit) { \
			/* write as many pixel at one time as possible */ \
			writePixel##psm##_0(pstart, j%2048, i%2048, *pbuf&0x0f, gs.dstbuf.bw); \
			writePixel##psm##_0(pstart, (j+1)%2048, i%2048, *pbuf>>4, gs.dstbuf.bw); \
			pbuf++; \
			if( widthlimit > 2 ) { \
				writePixel##psm##_0(pstart, (j+2)%2048, i%2048, *pbuf&0x0f, gs.dstbuf.bw); \
				writePixel##psm##_0(pstart, (j+3)%2048, i%2048, *pbuf>>4, gs.dstbuf.bw); \
				pbuf++; \
				\
				if( widthlimit > 4 ) { \
					writePixel##psm##_0(pstart, (j+4)%2048, i%2048, *pbuf&0x0f, gs.dstbuf.bw); \
					writePixel##psm##_0(pstart, (j+5)%2048, i%2048, *pbuf>>4, gs.dstbuf.bw); \
					pbuf++; \
					\
					if( widthlimit > 6 ) { \
						writePixel##psm##_0(pstart, (j+6)%2048, i%2048, *pbuf&0x0f, gs.dstbuf.bw); \
						writePixel##psm##_0(pstart, (j+7)%2048, i%2048, *pbuf>>4, gs.dstbuf.bw); \
						pbuf++; \
					} \
				} \
			} \
		} \
		\
		if( j >= gs.imageEndX ) { j = gs.trxpos.dx; } \
		else { assert( gs.imageTransfer == -1 || (nSize/32) == 0 ); goto End; } \
	} \
} \

// transmit until endX, don't check size since it has already been prevalidated
#define TRANSMIT_HOSTLOCAL_X_4(psm, T, widthlimit, blockheight, startX) { \
	for(int tempi = 0; tempi < blockheight; ++tempi) { \
		for(j = startX; j < gs.imageEndX; j+=2, pbuf++) { \
			writePixel##psm##_0(pstart, j%2048, (i+tempi)%2048, pbuf[0]&0x0f, gs.dstbuf.bw); \
			writePixel##psm##_0(pstart, (j+1)%2048, (i+tempi)%2048, pbuf[0]>>4, gs.dstbuf.bw); \
		} \
		pbuf += (pitch-fracX)/2; \
	} \
} \

// calculate pitch in source buffer
#define TRANSMIT_PITCH_(pitch, T) (pitch*sizeof(T))
#define TRANSMIT_PITCH_24(pitch, T) (pitch*3)
#define TRANSMIT_PITCH_4(pitch, T) (pitch/2)

// special swizzle macros
#define SwizzleBlock24(dst, src, pitch) { \
	u8* pnewsrc = src; \
	u32* pblock = tempblock; \
	\
	for(int by = 0; by < 7; ++by, pblock += 8, pnewsrc += pitch-24) { \
		for(int bx = 0; bx < 8; ++bx, pnewsrc += 3) { \
			pblock[bx] = *(u32*)pnewsrc; \
		} \
	} \
	for(int bx = 0; bx < 7; ++bx, pnewsrc += 3) { \
		/* might be 1 byte out of bounds of GS memory */ \
		pblock[bx] = *(u32*)pnewsrc; \
	} \
	/* do 3 bytes for the last copy */ \
	*((u8*)pblock+28) = pnewsrc[0]; \
	*((u8*)pblock+29) = pnewsrc[1]; \
	*((u8*)pblock+30) = pnewsrc[2]; \
	SwizzleBlock32((u8*)dst, (u8*)tempblock, 32, 0x00ffffff); \
} \

#define SwizzleBlock24u SwizzleBlock24

#define SwizzleBlock8H(dst, src, pitch) { \
	u8* pnewsrc = src; \
	u32* pblock = tempblock; \
	\
	for(int by = 0; by < 8; ++by, pblock += 8, pnewsrc += pitch) { \
		u32 u = *(u32*)pnewsrc; \
		pblock[0] = u<<24; \
		pblock[1] = u<<16; \
		pblock[2] = u<<8; \
		pblock[3] = u; \
		u = *(u32*)(pnewsrc+4); \
		pblock[4] = u<<24; \
		pblock[5] = u<<16; \
		pblock[6] = u<<8; \
		pblock[7] = u; \
	} \
	SwizzleBlock32((u8*)dst, (u8*)tempblock, 32, 0xff000000); \
} \

#define SwizzleBlock8Hu SwizzleBlock8H

#define SwizzleBlock4HH(dst, src, pitch) { \
	u8* pnewsrc = src; \
	u32* pblock = tempblock; \
	\
	for(int by = 0; by < 8; ++by, pblock += 8, pnewsrc += pitch) { \
		u32 u = *(u32*)pnewsrc; \
		pblock[0] = u<<28; \
		pblock[1] = u<<24; \
		pblock[2] = u<<20; \
		pblock[3] = u<<16; \
		pblock[4] = u<<12; \
		pblock[5] = u<<8; \
		pblock[6] = u<<4; \
		pblock[7] = u; \
	} \
	SwizzleBlock32((u8*)dst, (u8*)tempblock, 32, 0xf0000000); \
} \

#define SwizzleBlock4HHu SwizzleBlock4HH

#define SwizzleBlock4HL(dst, src, pitch) { \
	u8* pnewsrc = src; \
	u32* pblock = tempblock; \
	\
	for(int by = 0; by < 8; ++by, pblock += 8, pnewsrc += pitch) { \
		u32 u = *(u32*)pnewsrc; \
		pblock[0] = u<<24; \
		pblock[1] = u<<20; \
		pblock[2] = u<<16; \
		pblock[3] = u<<12; \
		pblock[4] = u<<8; \
		pblock[5] = u<<4; \
		pblock[6] = u; \
		pblock[7] = u>>4; \
	} \
	SwizzleBlock32((u8*)dst, (u8*)tempblock, 32, 0x0f000000); \
} \

#define SwizzleBlock4HLu SwizzleBlock4HL

// ------------------------
// |              Y       |
// ------------------------
// |        block     |   |
// |   aligned area   | X |
// |                  |   |
// ------------------------
// |              Y       |
// ------------------------
#define DEFINE_TRANSFERLOCAL(psm, T, widthlimit, blockbits, blockwidth, blockheight, TransSfx, SwizzleBlock) \
int TransferHostLocal##psm(const void* pbyMem, u32 nQWordSize) \
{ \
	START_HOSTLOCAL(); \
	\
	const T* pbuf = (const T*)pbyMem; \
	int nLeftOver = (nQWordSize*4*2)%(TRANSMIT_PITCH##TransSfx(2, T)); \
	int nSize = nQWordSize*4*2/TRANSMIT_PITCH##TransSfx(2, T); \
	nSize = min(nSize, gs.imageWnew * gs.imageHnew); \
	\
	int pitch, area, fracX; \
	int endY = ROUND_UPPOW2(i, blockheight); \
	int alignedY = ROUND_DOWNPOW2(gs.imageEndY, blockheight); \
	int alignedX = ROUND_DOWNPOW2(gs.imageEndX, blockwidth); \
	bool bAligned, bCanAlign = MOD_POW2(gs.trxpos.dx, blockwidth) == 0 && (j == gs.trxpos.dx) && (alignedY > endY) && alignedX > gs.trxpos.dx; \
	\
	if( (gs.imageEndX-gs.trxpos.dx)%widthlimit ) { \
		/* hack */ \
        int testwidth = (int)nSize - (gs.imageEndY-i)*(gs.imageEndX-gs.trxpos.dx)+(j-gs.trxpos.dx); \
		if ((testwidth <= widthlimit) && (testwidth >= -widthlimit)) { \
		/* Don't transfer */ \
		/*DEBUG_LOG("bad texture %s: %d %d %d\n", #psm, gs.trxpos.dx, gs.imageEndX, nQWordSize);*/ \
			gs.imageTransfer = -1; \
		} \
		bCanAlign = false; \
	} \
	\
	/* first align on block boundary */ \
	if( MOD_POW2(i, blockheight) || !bCanAlign ) { \
		\
		if( !bCanAlign ) \
			endY = gs.imageEndY; /* transfer the whole image */ \
		else \
			assert( endY < gs.imageEndY); /* part of alignment condition */ \
		\
		if( ((gs.imageEndX-gs.trxpos.dx)%widthlimit) || ((gs.imageEndX-j)%widthlimit) ) { \
			/* transmit with a width of 1 */ \
			TRANSMIT_HOSTLOCAL_Y##TransSfx(psm, T, (1+(DSTPSM == 0x14)), endY); \
		} \
		else { \
			TRANSMIT_HOSTLOCAL_Y##TransSfx(psm, T, widthlimit, endY); \
		} \
		\
		if( nSize == 0 || i == gs.imageEndY ) \
			goto End; \
	} \
	\
	assert( MOD_POW2(i, blockheight) == 0 && j == gs.trxpos.dx); \
	\
	/* can align! */ \
	pitch = gs.imageEndX-gs.trxpos.dx; \
	area = pitch*blockheight; \
	fracX = gs.imageEndX-alignedX; \
	\
	/* on top of checking whether pbuf is alinged, make sure that the width is at least aligned to its limits (due to bugs in pcsx2) */ \
	bAligned = !((uptr)pbuf & 0xf) && (TRANSMIT_PITCH##TransSfx(pitch, T)&0xf) == 0; \
	\
	/* transfer aligning to blocks */ \
	for(; i < alignedY && nSize >= area; i += blockheight, nSize -= area) { \
		\
		if( bAligned || ((DSTPSM==PSMCT24) || (DSTPSM==PSMT8H) || (DSTPSM==PSMT4HH) || (DSTPSM==PSMT4HL)) ) { \
			for(int tempj = gs.trxpos.dx; tempj < alignedX; tempj += blockwidth, pbuf += TRANSMIT_PITCH##TransSfx(blockwidth, T)/sizeof(T)) { \
				SwizzleBlock(pstart + getPixelAddress##psm##_0(tempj, i, gs.dstbuf.bw)*blockbits/8, \
					(u8*)pbuf, TRANSMIT_PITCH##TransSfx(pitch, T)); \
			} \
		} \
		else { \
			for(int tempj = gs.trxpos.dx; tempj < alignedX; tempj += blockwidth, pbuf += TRANSMIT_PITCH##TransSfx(blockwidth, T)/sizeof(T)) { \
				SwizzleBlock##u(pstart + getPixelAddress##psm##_0(tempj, i, gs.dstbuf.bw)*blockbits/8, \
					(u8*)pbuf, TRANSMIT_PITCH##TransSfx(pitch, T)); \
			} \
		} \
		\
		/* transfer the rest */ \
		if( alignedX < gs.imageEndX ) { \
			TRANSMIT_HOSTLOCAL_X##TransSfx(psm, T, widthlimit, blockheight, alignedX); \
			pbuf -= TRANSMIT_PITCH##TransSfx((alignedX-gs.trxpos.dx), T)/sizeof(T); \
		} \
		else pbuf += (blockheight-1)*TRANSMIT_PITCH##TransSfx(pitch, T)/sizeof(T); \
		j = gs.trxpos.dx; \
	} \
	\
	if( TRANSMIT_PITCH##TransSfx(nSize, T)/4 > 0 ) { \
		TRANSMIT_HOSTLOCAL_Y##TransSfx(psm, T, widthlimit, gs.imageEndY); \
		/* sometimes wrong sizes are sent (tekken tag) */ \
		assert( gs.imageTransfer == -1 || TRANSMIT_PITCH##TransSfx(nSize,T)/4 <= 2 ); \
	} \
	\
	END_HOSTLOCAL(); \
	return (nSize * TRANSMIT_PITCH##TransSfx(2, T) + nLeftOver)/2; \
} \

DEFINE_TRANSFERLOCAL(32, u32, 2, 32, 8, 8, _, SwizzleBlock32);
DEFINE_TRANSFERLOCAL(32Z, u32, 2, 32, 8, 8, _, SwizzleBlock32);
DEFINE_TRANSFERLOCAL(24, u8, 8, 32, 8, 8, _24, SwizzleBlock24);
DEFINE_TRANSFERLOCAL(24Z, u8, 8, 32, 8, 8, _24, SwizzleBlock24);
DEFINE_TRANSFERLOCAL(16, u16, 4, 16, 16, 8, _, SwizzleBlock16);
DEFINE_TRANSFERLOCAL(16S, u16, 4, 16, 16, 8, _, SwizzleBlock16);
DEFINE_TRANSFERLOCAL(16Z, u16, 4, 16, 16, 8, _, SwizzleBlock16);
DEFINE_TRANSFERLOCAL(16SZ, u16, 4, 16, 16, 8, _, SwizzleBlock16);
DEFINE_TRANSFERLOCAL(8, u8, 4, 8, 16, 16, _, SwizzleBlock8);
DEFINE_TRANSFERLOCAL(4, u8, 8, 4, 32, 16, _4, SwizzleBlock4);
DEFINE_TRANSFERLOCAL(8H, u8, 4, 32, 8, 8, _, SwizzleBlock8H);
DEFINE_TRANSFERLOCAL(4HL, u8, 8, 32, 8, 8, _4, SwizzleBlock4HL);
DEFINE_TRANSFERLOCAL(4HH, u8, 8, 32, 8, 8, _4, SwizzleBlock4HH);

//#define T u8
//#define widthlimit 8
//#define blockbits 4
//#define blockwidth 32
//#define blockheight 16
//
//void TransferHostLocal4(const void* pbyMem, u32 nQWordSize)
//{
//	START_HOSTLOCAL();
//	
//	const T* pbuf = (const T*)pbyMem;
//	u32 nSize = nQWordSize*16*2/TRANSMIT_PITCH_4(2, T);
//	nSize = min(nSize, gs.imageWnew * gs.imageHnew);
//	
//	int endY = ROUND_UPPOW2(i, blockheight);
//	int alignedY = ROUND_DOWNPOW2(gs.imageEndY, blockheight);
//	int alignedX = ROUND_DOWNPOW2(gs.imageEndX, blockwidth);
//	bool bCanAlign = MOD_POW2(gs.trxpos.dx, blockwidth) == 0 && (j == gs.trxpos.dx) && (alignedY > endY) && alignedX > gs.trxpos.dx;
//	
//	if( (gs.imageEndX-gs.trxpos.dx)%widthlimit ) {
//		/* hack */
//		if( abs((int)nSize - (gs.imageEndY-i)*(gs.imageEndX-gs.trxpos.dx)+(j-gs.trxpos.dx)) <= widthlimit ) {
//			/* don't transfer */
//			/*DEBUG_LOG("bad texture %s: %d %d %d\n", #psm, gs.trxpos.dx, gs.imageEndX, nQWordSize);*/
//			gs.imageTransfer = -1;
//		}
//		bCanAlign = false;
//	}
//	
//	/* first align on block boundary */
//	if( MOD_POW2(i, blockheight) || !bCanAlign ) {
//		
//		if( !bCanAlign )
//			endY = gs.imageEndY; /* transfer the whole image */
//		else
//			assert( endY < gs.imageEndY); /* part of alignment condition */
//		
//		if( (DSTPSM == 0x13 || DSTPSM == 0x14) && ((gs.imageEndX-gs.trxpos.dx)%widthlimit) ) {
//			/* transmit with a width of 1 */
//			TRANSMIT_HOSTLOCAL_Y_4(4, T, (1+(DSTPSM == 0x14)), endY);
//		}
//		else {
//			TRANSMIT_HOSTLOCAL_Y_4(4, T, widthlimit, endY);
//		}
//		
//		if( nSize == 0 || i == gs.imageEndY )
//			goto End;
//	}
//	
//	assert( MOD_POW2(i, blockheight) == 0 && j == gs.trxpos.dx);
//	
//	/* can align! */
//	int pitch = gs.imageEndX-gs.trxpos.dx;
//	u32 area = pitch*blockheight;
//	int fracX = gs.imageEndX-alignedX;
//	
//	/* on top of checking whether pbuf is alinged, make sure that the width is at least aligned to its limits (due to bugs in pcsx2) */
//	bool bAligned = !((u32)pbuf & 0xf) && (TRANSMIT_PITCH_4(pitch, T)&0xf) == 0;
//	
//	/* transfer aligning to blocks */
//	for(; i < alignedY && nSize >= area; i += blockheight, nSize -= area) {
//		
//		if( bAligned || ((DSTPSM==PSMCT24) || (DSTPSM==PSMT8H) || (DSTPSM==PSMT4HH) || (DSTPSM==PSMT4HL)) ) {
//			for(int tempj = gs.trxpos.dx; tempj < alignedX; tempj += blockwidth, pbuf += TRANSMIT_PITCH_4(blockwidth, T)/sizeof(T)) {
//				SwizzleBlock4(pstart + getPixelAddress4_0(tempj, i, gs.dstbuf.bw)*blockbits/8,
//					(u8*)pbuf, TRANSMIT_PITCH_4(pitch, T));
//			}
//		}
//		else {
//			for(int tempj = gs.trxpos.dx; tempj < alignedX; tempj += blockwidth, pbuf += TRANSMIT_PITCH_4(blockwidth, T)/sizeof(T)) {
//				SwizzleBlock4u(pstart + getPixelAddress4_0(tempj, i, gs.dstbuf.bw)*blockbits/8,
//					(u8*)pbuf, TRANSMIT_PITCH_4(pitch, T));
//			}
//		}
//		
//		/* transfer the rest */
//		if( alignedX < gs.imageEndX ) {
//			TRANSMIT_HOSTLOCAL_X_4(4, T, widthlimit, blockheight, alignedX);
//			pbuf -= TRANSMIT_PITCH_4((alignedX-gs.trxpos.dx), T)/sizeof(T);
//		}
//		else pbuf += (blockheight-1)*TRANSMIT_PITCH_4(pitch, T)/sizeof(T);
//		j = 0;
//	}
//	
//	if( TRANSMIT_PITCH_4(nSize, T)/4 > 0 ) {
//		TRANSMIT_HOSTLOCAL_Y_4(4, T, widthlimit, gs.imageEndY);
//		/* sometimes wrong sizes are sent (tekken tag) */
//		assert( gs.imageTransfer == -1 || TRANSMIT_PITCH_4(nSize,T)/4 <= 2 );
//	}
//	
//	END_HOSTLOCAL();
//}

void TransferLocalHost32(void* pbyMem, u32 nQWordSize)
{
}

void TransferLocalHost24(void* pbyMem, u32 nQWordSize)
{
}

void TransferLocalHost16(void* pbyMem, u32 nQWordSize)
{
}

void TransferLocalHost16S(void* pbyMem, u32 nQWordSize)
{
}

void TransferLocalHost8(void* pbyMem, u32 nQWordSize)
{
}

void TransferLocalHost4(void* pbyMem, u32 nQWordSize)
{
}

void TransferLocalHost8H(void* pbyMem, u32 nQWordSize)
{
}

void TransferLocalHost4HL(void* pbyMem, u32 nQWordSize)
{
}

void TransferLocalHost4HH(void* pbyMem, u32 nQWordSize)
{
}

void TransferLocalHost32Z(void* pbyMem, u32 nQWordSize)
{
}

void TransferLocalHost24Z(void* pbyMem, u32 nQWordSize)
{
}

void TransferLocalHost16Z(void* pbyMem, u32 nQWordSize)
{
}

void TransferLocalHost16SZ(void* pbyMem, u32 nQWordSize)
{
}

#define FILL_BLOCK(bw, bh, ox, oy, mult, psm, psmcol) { \
	b.vTexDims = D3DXVECTOR2(BLOCK_TEXWIDTH/(float)(bw), BLOCK_TEXHEIGHT/(float)bh); \
	b.vTexBlock = D3DXVECTOR4((float)bw/BLOCK_TEXWIDTH, (float)bh/BLOCK_TEXHEIGHT, ((float)ox+0.2f)/BLOCK_TEXWIDTH, ((float)oy+0.05f)/BLOCK_TEXHEIGHT); \
	b.width = bw; \
	b.height = bh; \
	b.colwidth = bh / 4; \
	b.colheight = bw / 8; \
	b.bpp = 32/mult; \
	\
	b.pageTable = &g_pageTable##psm[0][0]; \
	b.blockTable = &g_blockTable##psm[0][0]; \
	b.columnTable = &g_columnTable##psmcol[0][0]; \
	assert( sizeof(g_pageTable##psm) == bw*bh*sizeof(g_pageTable##psm[0][0]) ); \
	psrcf = (float*)pBlockTexture->pBits + ox + oy * pBlockTexture->Pitch/sizeof(float); \
	psrcw = (WORD*)pBlockTexture->pBits + ox + oy * pBlockTexture->Pitch/sizeof(WORD); \
	for(i = 0; i < bh; ++i) { \
		for(j = 0; j < bw; ++j) { \
			/* fill the table */ \
			u32 u = g_blockTable##psm[(i / b.colheight)][(j / b.colwidth)] * 64 * mult + g_columnTable##psmcol[i%b.colheight][j%b.colwidth]; \
			b.pageTable[i*bw+j] = u; \
			if( format == D3DFMT_R32F ) { \
				psrcf[i*pBlockTexture->Pitch/sizeof(float)+j] = (float)(u) / (float)(GPU_TEXWIDTH*mult); \
			} \
			else { \
				psrcw[i*pBlockTexture->Pitch/sizeof(WORD)+j] = u; \
			} \
		} \
	} \
	\
	if( pBilinearTexture != NULL ) { \
		assert( format == D3DFMT_R32F ); \
		psrcv = (DXVEC4*)pBilinearTexture->pBits + ox + oy * pBilinearTexture->Pitch/sizeof(DXVEC4); \
		for(i = 0; i < bh; ++i) { \
			for(j = 0; j < bw; ++j) { \
				DXVEC4* pv = &psrcv[i*pBilinearTexture->Pitch/sizeof(DXVEC4)+j]; \
				pv->x = psrcf[i*pBlockTexture->Pitch/sizeof(float)+j]; \
				pv->y = psrcf[i*pBlockTexture->Pitch/sizeof(float)+((j+1)%bw)]; \
				pv->z = psrcf[((i+1)%bh)*pBlockTexture->Pitch/sizeof(float)+j]; \
				pv->w = psrcf[((i+1)%bh)*pBlockTexture->Pitch/sizeof(float)+((j+1)%bw)]; \
			} \
		} \
	} \
	b.getPixelAddress = getPixelAddress##psm; \
	b.getPixelAddress_0 = getPixelAddress##psm##_0; \
	b.writePixel = writePixel##psm; \
	b.writePixel_0 = writePixel##psm##_0; \
	b.readPixel = readPixel##psm; \
	b.readPixel_0 = readPixel##psm##_0; \
	b.TransferHostLocal = TransferHostLocal##psm; \
	b.TransferLocalHost = TransferLocalHost##psm; \
} \

void BLOCK::FillBlocks(const D3DLOCKED_RECT* pBlockTexture, const D3DLOCKED_RECT* pBilinearTexture, D3DFORMAT format)
{
	assert( pBlockTexture != NULL );
	assert( format == D3DFMT_R32F || format == D3DFMT_G16R16 );

	int i, j;
	BLOCK b;
	float* psrcf = NULL;
	WORD* psrcw = NULL;
	DXVEC4* psrcv = NULL;

	memset(m_Blocks, 0, sizeof(m_Blocks));

	// 32
	FILL_BLOCK(64, 32, 0, 0, 1, 32, 32);
	m_Blocks[PSMCT32] = b;

	// 24 (same as 32 except write/readPixel are different)
	m_Blocks[PSMCT24] = b;
	m_Blocks[PSMCT24].writePixel = writePixel24;
	m_Blocks[PSMCT24].writePixel_0 = writePixel24_0;
	m_Blocks[PSMCT24].readPixel = readPixel24;
	m_Blocks[PSMCT24].readPixel_0 = readPixel24_0;
	m_Blocks[PSMCT24].TransferHostLocal = TransferHostLocal24;
	m_Blocks[PSMCT24].TransferLocalHost = TransferLocalHost24;

	// 8H (same as 32 except write/readPixel are different)
	m_Blocks[PSMT8H] = b;
	m_Blocks[PSMT8H].writePixel = writePixel8H;
	m_Blocks[PSMT8H].writePixel_0 = writePixel8H_0;
	m_Blocks[PSMT8H].readPixel = readPixel8H;
	m_Blocks[PSMT8H].readPixel_0 = readPixel8H_0;
	m_Blocks[PSMT8H].TransferHostLocal = TransferHostLocal8H;
	m_Blocks[PSMT8H].TransferLocalHost = TransferLocalHost8H;

	m_Blocks[PSMT4HL] = b;
	m_Blocks[PSMT4HL].writePixel = writePixel4HL;
	m_Blocks[PSMT4HL].writePixel_0 = writePixel4HL_0;
	m_Blocks[PSMT4HL].readPixel = readPixel4HL;
	m_Blocks[PSMT4HL].readPixel_0 = readPixel4HL_0;
	m_Blocks[PSMT4HL].TransferHostLocal = TransferHostLocal4HL;
	m_Blocks[PSMT4HL].TransferLocalHost = TransferLocalHost4HL;

	m_Blocks[PSMT4HH] = b;
	m_Blocks[PSMT4HH].writePixel = writePixel4HH;
	m_Blocks[PSMT4HH].writePixel_0 = writePixel4HH_0;
	m_Blocks[PSMT4HH].readPixel = readPixel4HH;
	m_Blocks[PSMT4HH].readPixel_0 = readPixel4HH_0;
	m_Blocks[PSMT4HH].TransferHostLocal = TransferHostLocal4HH;
	m_Blocks[PSMT4HH].TransferLocalHost = TransferLocalHost4HH;

	// 32z
	FILL_BLOCK(64, 32, 64, 0, 1, 32Z, 32);
	m_Blocks[PSMT32Z] = b;

	// 24Z (same as 32Z except write/readPixel are different)
	m_Blocks[PSMT24Z] = b;
	m_Blocks[PSMT24Z].writePixel = writePixel24Z;
	m_Blocks[PSMT24Z].writePixel_0 = writePixel24Z_0;
	m_Blocks[PSMT24Z].readPixel = readPixel24Z;
	m_Blocks[PSMT24Z].readPixel_0 = readPixel24Z_0;
	m_Blocks[PSMT24Z].TransferHostLocal = TransferHostLocal24Z;
	m_Blocks[PSMT24Z].TransferLocalHost = TransferLocalHost24Z;

	// 16
	FILL_BLOCK(64, 64, 0, 32, 2, 16, 16);
	m_Blocks[PSMCT16] = b;

	// 16s
	FILL_BLOCK(64, 64, 64, 32, 2, 16S, 16);
	m_Blocks[PSMCT16S] = b;

	// 16z
	FILL_BLOCK(64, 64, 0, 96, 2, 16Z, 16);
	m_Blocks[PSMT16Z] = b;

	// 16sz
	FILL_BLOCK(64, 64, 64, 96, 2, 16SZ, 16);
	m_Blocks[PSMT16SZ] = b;

	// 8
	FILL_BLOCK(128, 64, 0, 160, 4, 8, 8);
	m_Blocks[PSMT8] = b;

	// 4
	FILL_BLOCK(128, 128, 0, 224, 8, 4, 4);
	m_Blocks[PSMT4] = b;
}
