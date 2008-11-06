/*  GSsoft
 *  Copyright (C) 2002-2005  GSsoft Team
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

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "GS.h"
#include "Mem.h"
#include "Rec.h"
#include "Page.h"
#include "Cache.h"

#ifdef __MSCW32__
#pragma warning(disable:4244)

#endif

//#define _OPTIMIZE


///////////////////////////////
// TransferPixel from/to vRam
//

__inline void TransferPixel(u32 pixel) {
	writePixel32(gs.imageX, gs.imageY, pixel, gs.dstbuf.bp, gs.dstbuf.bw);

	if (++gs.imageX == (gs.imageW + gs.trxpos.dx)) {
		gs.imageX = gs.trxpos.dx; gs.imageY++;
	}
}

__inline void TransferPixelB(u32 pixel) {
	writePixel32(gs.imageX, gs.imageY, pixel, gs.dstbuf.bp, gs.dstbuf.bw);

	if (++gs.imageX == (gs.imageW + gs.trxpos.dx)) {
		gs.imageX = gs.trxpos.dx; gs.imageY++;
	}
}

__inline void TransferPixel32Z(u32 pixel) {
	writePixel32Z(gs.imageX, gs.imageY, pixel, gs.dstbuf.bp, gs.dstbuf.bw);

	if (++gs.imageX == (gs.imageW + gs.trxpos.dx)) {
		gs.imageX = gs.trxpos.dx; gs.imageY++;
	}
}

__inline void TransferPixel24(u8 *pixel) {
	u32 pix = pixel[0] | (pixel[1] << 8) | (pixel[2] << 16);
	writePixel24(gs.imageX, gs.imageY, pix, gs.dstbuf.bp, gs.dstbuf.bw);

	if (++gs.imageX == (gs.imageW + gs.trxpos.dx)) {
		gs.imageX = gs.trxpos.dx; gs.imageY++;
	}
}

__inline void TransferPixel24B(u8 *pixel) {
	u32 pix = pixel[0] | (pixel[1] << 8) | (pixel[2] << 16);
	writePixel24(gs.imageX, gs.imageY, pix, gs.dstbuf.bp, gs.dstbuf.bw);

	if (++gs.imageX == (gs.imageW + gs.trxpos.dx)) {
		gs.imageX = gs.trxpos.dx; gs.imageY++;
	}
}

__inline void TransferPixel24Z(u8 *pixel) {
	u32 pix = pixel[0] | (pixel[1] << 8) | (pixel[2] << 16);
	writePixel24Z(gs.imageX, gs.imageY, pix, gs.dstbuf.bp, gs.dstbuf.bw);

	if (++gs.imageX == (gs.imageW + gs.trxpos.dx)) {
		gs.imageX = gs.trxpos.dx; gs.imageY++;
	}
}

__inline void TransferPixel16(u16 pixel) {
	writePixel16(gs.imageX, gs.imageY, pixel, gs.dstbuf.bp, gs.dstbuf.bw);

	if (++gs.imageX == (gs.imageW + gs.trxpos.dx)) {
		gs.imageX = gs.trxpos.dx; gs.imageY++;
	}
}

__inline void TransferPixel16B(u16 pixel) {
	writePixel16(gs.imageX, gs.imageY, pixel, gs.dstbuf.bp, gs.dstbuf.bw);

	if (++gs.imageX == (gs.imageW + gs.trxpos.dx)) {
		gs.imageX = gs.trxpos.dx; gs.imageY++;
	}
}

__inline void TransferPixel16Z(u16 pixel) {
	writePixel16Z(gs.imageX, gs.imageY, pixel, gs.dstbuf.bp, gs.dstbuf.bw);

	if (++gs.imageX == (gs.imageW + gs.trxpos.dx)) {
		gs.imageX = gs.trxpos.dx; gs.imageY++;
	}
}

__inline void TransferPixel16S(u16 pixel) {
	writePixel16S(gs.imageX, gs.imageY, pixel, gs.dstbuf.bp, gs.dstbuf.bw);

	if (++gs.imageX == (gs.imageW + gs.trxpos.dx)) {
		gs.imageX = gs.trxpos.dx; gs.imageY++;
	}
}

__inline void TransferPixel16SB(u16 pixel) {
	writePixel16S(gs.imageX, gs.imageY, pixel, gs.dstbuf.bp, gs.dstbuf.bw);

	if (++gs.imageX == (gs.imageW + gs.trxpos.dx)) {
		gs.imageX = gs.trxpos.dx; gs.imageY++;
	}
}

__inline void TransferPixel16SZ(u16 pixel) {
	writePixel16SZ(gs.imageX, gs.imageY, pixel, gs.dstbuf.bp, gs.dstbuf.bw);

	if (++gs.imageX == (gs.imageW + gs.trxpos.dx)) {
		gs.imageX = gs.trxpos.dx; gs.imageY++;
	}
}

__inline void TransferPixel8(u8 pixel) {
	writePixel8(gs.imageX, gs.imageY, pixel, gs.dstbuf.bp, gs.dstbuf.bw);

	if (++gs.imageX == (gs.imageW + gs.trxpos.dx)) {
		gs.imageX = gs.trxpos.dx; gs.imageY++;
	}
}

__inline void TransferPixel8B(u8 pixel) {
	writePixel8(gs.imageX, gs.imageY, pixel, gs.dstbuf.bp, gs.dstbuf.bw);

	if (++gs.imageX == (gs.imageW + gs.trxpos.dx)) {
		gs.imageX = gs.trxpos.dx; gs.imageY++;
	}
}

__inline void TransferPixel8H(u8 pixel) {
	writePixel8H(gs.imageX, gs.imageY, pixel, gs.dstbuf.bp, gs.dstbuf.bw);

	if (++gs.imageX == (gs.imageW + gs.trxpos.dx)) {
		gs.imageX = gs.trxpos.dx; gs.imageY++;
	}
}

__inline void TransferPixel8HB(u8 pixel) {
	writePixel8H(gs.imageX, gs.imageY, pixel, gs.dstbuf.bp, gs.dstbuf.bw);

	if (++gs.imageX == (gs.imageW + gs.trxpos.dx)) {
		gs.imageX = gs.trxpos.dx; gs.imageY++;
	}
}

__inline void TransferPixel4(u8 pixel) {
	writePixel4(gs.imageX,   gs.imageY, pixel & 0xf, gs.dstbuf.bp, gs.dstbuf.bw);
	writePixel4(gs.imageX+1, gs.imageY, pixel >> 4,  gs.dstbuf.bp, gs.dstbuf.bw);

	gs.imageX+= 2;
	if (gs.imageX == (gs.imageW + gs.trxpos.dx)) {
		gs.imageX = gs.trxpos.dx; gs.imageY++;
	}
}

__inline void TransferPixel4B(u8 pixel) {
	writePixel4(gs.imageX,   gs.imageY, pixel & 0xf, gs.dstbuf.bp, gs.dstbuf.bw);
	writePixel4(gs.imageX+1, gs.imageY, pixel >> 4,  gs.dstbuf.bp, gs.dstbuf.bw);

	gs.imageX+= 2;
	if (gs.imageX == (gs.imageW + gs.trxpos.dx)) {
		gs.imageX = gs.trxpos.dx; gs.imageY++;
	}
}

__inline void TransferPixel4HL(u8 pixel) {
	writePixel4HL(gs.imageX, gs.imageY, pixel, gs.dstbuf.bp, gs.dstbuf.bw);

	if (++gs.imageX == (gs.imageW + gs.trxpos.dx)) {
		gs.imageX = gs.trxpos.dx; gs.imageY++;
	}
}

__inline void TransferPixel4HLB(u8 pixel) {
	writePixel4HL(gs.imageX, gs.imageY, pixel, gs.dstbuf.bp, gs.dstbuf.bw);

	if (++gs.imageX == (gs.imageW + gs.trxpos.dx)) {
		gs.imageX = gs.trxpos.dx; gs.imageY++;
	}
}

__inline void TransferPixel4HH(u8 pixel) {
	writePixel4HH(gs.imageX, gs.imageY, pixel, gs.dstbuf.bp, gs.dstbuf.bw);

	if (++gs.imageX == (gs.imageW + gs.trxpos.dx)) {
		gs.imageX = gs.trxpos.dx; gs.imageY++;
	}
}

__inline void TransferPixel4HHB(u8 pixel) {
	writePixel4HH(gs.imageX, gs.imageY, pixel, gs.dstbuf.bp, gs.dstbuf.bw);

	if (++gs.imageX == (gs.imageW + gs.trxpos.dx)) {
		gs.imageX = gs.trxpos.dx; gs.imageY++;
	}
}

__inline void TransferPixelSrc(u32 *pixel) {
	*pixel = readPixel32(gs.imageX, gs.imageY, gs.srcbuf.bp, gs.srcbuf.bw);

	if (++gs.imageX == (gs.imageW + gs.trxpos.sx)) {
		gs.imageX = gs.trxpos.sx; gs.imageY++;
	}
}

__inline void TransferPixelSrcB(u32 *pixel) {
	*pixel = readPixel32(gs.imageX, gs.imageY, gs.srcbuf.bp, gs.srcbuf.bw);

	if (++gs.imageX == (gs.imageW + gs.trxpos.sx)) {
		gs.imageX = gs.trxpos.sx; gs.imageY++;
	}
}

__inline void TransferPixelSrc32Z(u32 *pixel) {
	*pixel = readPixel32Z(gs.imageX, gs.imageY, gs.srcbuf.bp, gs.srcbuf.bw);

	if (++gs.imageX == (gs.imageW + gs.trxpos.sx)) {
		gs.imageX = gs.trxpos.sx; gs.imageY++;
	}
}

__inline void TransferPixelSrc24(u8 *pixel) {
	u32 pix = readPixel24(gs.imageX, gs.imageY, gs.srcbuf.bp, gs.srcbuf.bw);
	pixel[0] = ((u8*)pix)[0]; pixel[1] = ((u8*)pix)[1]; pixel[2] = ((u8*)pix)[2];

	if (++gs.imageX == (gs.imageW + gs.trxpos.sx)) {
		gs.imageX = gs.trxpos.sx; gs.imageY++;
	}
}

__inline void TransferPixelSrc24B(u8 *pixel) {
	u32 pix = readPixel24(gs.imageX, gs.imageY, gs.srcbuf.bp, gs.srcbuf.bw);
	pixel[0] = ((u8*)pix)[0]; pixel[1] = ((u8*)pix)[1]; pixel[2] = ((u8*)pix)[2];

	if (++gs.imageX == (gs.imageW + gs.trxpos.sx)) {
		gs.imageX = gs.trxpos.sx; gs.imageY++;
	}
}

__inline void TransferPixelSrc24Z(u8 *pixel) {
	u32 pix = readPixel24Z(gs.imageX, gs.imageY, gs.srcbuf.bp, gs.srcbuf.bw);
	pixel[0] = ((u8*)pix)[0]; pixel[1] = ((u8*)pix)[1]; pixel[2] = ((u8*)pix)[2];

	if (++gs.imageX == (gs.imageW + gs.trxpos.sx)) {
		gs.imageX = gs.trxpos.sx; gs.imageY++;
	}
}

__inline void TransferPixelSrc16(u16 *pixel) {
	*pixel = readPixel16(gs.imageX, gs.imageY, gs.srcbuf.bp, gs.srcbuf.bw);

	if (++gs.imageX == (gs.imageW + gs.trxpos.sx)) {
		gs.imageX = gs.trxpos.sx; gs.imageY++;
	}
}

__inline void TransferPixelSrc16B(u16 *pixel) {
	*pixel = readPixel16(gs.imageX, gs.imageY, gs.srcbuf.bp, gs.srcbuf.bw);

	if (++gs.imageX == (gs.imageW + gs.trxpos.sx)) {
		gs.imageX = gs.trxpos.sx; gs.imageY++;
	}
}

__inline void TransferPixelSrc16Z(u16 *pixel) {
	*pixel = readPixel16Z(gs.imageX, gs.imageY, gs.srcbuf.bp, gs.srcbuf.bw);

	if (++gs.imageX == (gs.imageW + gs.trxpos.sx)) {
		gs.imageX = gs.trxpos.sx; gs.imageY++;
	}
}

__inline void TransferPixelSrc16S(u16 *pixel) {
	*pixel = readPixel16S(gs.imageX, gs.imageY, gs.srcbuf.bp, gs.srcbuf.bw);

	if (++gs.imageX == (gs.imageW + gs.trxpos.sx)) {
		gs.imageX = gs.trxpos.sx; gs.imageY++;
	}
}

__inline void TransferPixelSrc16SB(u16 *pixel) {
	*pixel = readPixel16S(gs.imageX, gs.imageY, gs.srcbuf.bp, gs.srcbuf.bw);

	if (++gs.imageX == (gs.imageW + gs.trxpos.sx)) {
		gs.imageX = gs.trxpos.sx; gs.imageY++;
	}
}

__inline void TransferPixelSrc16SZ(u16 *pixel) {
	*pixel = readPixel16SZ(gs.imageX, gs.imageY, gs.srcbuf.bp, gs.srcbuf.bw);

	if (++gs.imageX == (gs.imageW + gs.trxpos.sx)) {
		gs.imageX = gs.trxpos.sx; gs.imageY++;
	}
}

__inline void TransferPixelSrc8(u8 *pixel) {
	*pixel = readPixel8(gs.imageX, gs.imageY, gs.srcbuf.bp, gs.srcbuf.bw);

	if (++gs.imageX == (gs.imageW + gs.trxpos.sx)) {
		gs.imageX = gs.trxpos.sx; gs.imageY++;
	}
}

__inline void TransferPixelSrc8B(u8 *pixel) {
	*pixel = readPixel8(gs.imageX, gs.imageY, gs.srcbuf.bp, gs.srcbuf.bw);

	if (++gs.imageX == (gs.imageW + gs.trxpos.sx)) {
		gs.imageX = gs.trxpos.sx; gs.imageY++;
	}
}

__inline void TransferPixelSrc8H(u8 *pixel) {
	*pixel = readPixel8H(gs.imageX, gs.imageY, gs.srcbuf.bp, gs.srcbuf.bw);

	if (++gs.imageX == (gs.imageW + gs.trxpos.sx)) {
		gs.imageX = gs.trxpos.sx; gs.imageY++;
	}
}

__inline void TransferPixelSrc8HB(u8 *pixel) {
	*pixel = readPixel8H(gs.imageX, gs.imageY, gs.srcbuf.bp, gs.srcbuf.bw);

	if (++gs.imageX == (gs.imageW + gs.trxpos.sx)) {
		gs.imageX = gs.trxpos.sx; gs.imageY++;
	}
}

__inline void TransferPixelSrc4(u8 *pixel) {
	*pixel = readPixel4(gs.imageX, gs.imageY, gs.srcbuf.bp, gs.srcbuf.bw);

	gs.imageX+= 2;
	if (gs.imageX == (gs.imageW + gs.trxpos.sx)) {
		gs.imageX = gs.trxpos.sx; gs.imageY++;
	}
}

__inline void TransferPixelSrc4B(u8 *pixel) {
	*pixel = readPixel4(gs.imageX, gs.imageY, gs.srcbuf.bp, gs.srcbuf.bw);

	gs.imageX+= 2;
	if (gs.imageX == (gs.imageW + gs.trxpos.sx)) {
		gs.imageX = gs.trxpos.sx; gs.imageY++;
	}
}

__inline void TransferPixelSrc4HL(u8 *pixel) {
	*pixel = readPixel4HL(gs.imageX, gs.imageY, gs.srcbuf.bp, gs.srcbuf.bw);

	if (++gs.imageX == (gs.imageW + gs.trxpos.sx)) {
		gs.imageX = gs.trxpos.sx; gs.imageY++;
	}
}

__inline void TransferPixelSrc4HLB(u8 *pixel) {
	*pixel = readPixel4HL(gs.imageX, gs.imageY, gs.srcbuf.bp, gs.srcbuf.bw);

	if (++gs.imageX == (gs.imageW + gs.trxpos.sx)) {
		gs.imageX = gs.trxpos.sx; gs.imageY++;
	}
}

__inline void TransferPixelSrc4HH(u8 *pixel) {
	*pixel = readPixel4HH(gs.imageX, gs.imageY, gs.srcbuf.bp, gs.srcbuf.bw);

	if (++gs.imageX == (gs.imageW + gs.trxpos.sx)) {
		gs.imageX = gs.trxpos.sx; gs.imageY++;
	}
}

__inline void TransferPixelSrc4HHB(u8 *pixel) {
	*pixel = readPixel4HH(gs.imageX, gs.imageY, gs.srcbuf.bp, gs.srcbuf.bw);

	if (++gs.imageX == (gs.imageW + gs.trxpos.sx)) {
		gs.imageX = gs.trxpos.sx; gs.imageY++;
	}
}

///////////////////////////////
// FrameBuffer writes/reads
//

void FBwrite(u32 *data) {
	TransferPixel(data[0]);
	TransferPixel(data[1]);
	TransferPixel(data[2]);
	TransferPixel(data[3]);
}

void FBwriteB(u32 *data) {
	TransferPixelB(data[0]);
	TransferPixelB(data[1]);
	TransferPixelB(data[2]);
	TransferPixelB(data[3]);
}

void FBwrite32Z(u32 *data) {
	TransferPixel32Z(data[0]);
	TransferPixel32Z(data[1]);
	TransferPixel32Z(data[2]);
	TransferPixel32Z(data[3]);
}

void FBwrite_8x8(u32 *data) {
	u32 *fb = (u32*)&vRamUL[getPixelAddress32(gs.imageX, gs.imageY, gs.dstbuf.bp, gs.dstbuf.bw)];
	u32 *data2 = data+gs.imageW;

#if (defined(__i386__) || defined(__x86_64__)) && defined(__GNUC__)
	__asm__(
		"movq  0(%1), %%xmm0\n"
		"movq  8(%1), %%xmm1\n"
		"movq 16(%1), %%xmm2\n"
		"movq 24(%1), %%xmm3\n"

		"movhps  0(%2), %%xmm0\n"
		"movhps  8(%2), %%xmm1\n"
		"movhps 16(%2), %%xmm2\n"
		"movhps 24(%2), %%xmm3\n"

		"movaps %%xmm0,  0(%0)\n"
		"movaps %%xmm1, 16(%0)\n"
		"movaps %%xmm2, 32(%0)\n"
		"movaps %%xmm3, 48(%0)\n"

		:: "r"(fb), "r"(data), "r"(data2)
	);
#else
	fb[0] = data[0]; fb[1] = data[1];
	fb[4] = data[2]; fb[5] = data[3];
	fb[8] = data[4]; fb[9] = data[5];
	fb[12] = data[6]; fb[13] = data[7];

	fb[2] = data2[0]; fb[3] = data2[1];
	fb[6] = data2[2]; fb[7] = data2[3];
	fb[10] = data2[4]; fb[11] = data2[5];
	fb[14] = data2[6]; fb[15] = data2[7];
#endif

	gs.imageX+= 8;
	if (gs.imageX == (gs.imageW + gs.trxpos.dx)) {
		gs.imageX = gs.trxpos.dx; gs.imageY+= 2;
	}
}

void FBwrite_8(u32 *data) {
	u32 *fb = (u32*)&vRamUL[getPixelAddress32(gs.imageX, gs.imageY, gs.dstbuf.bp, gs.dstbuf.bw)];

	fb[0] = data[0]; fb[1] = data[1];
	fb[4] = data[2]; fb[5] = data[3];
	fb[8] = data[4]; fb[9] = data[5];
	fb[12] = data[6]; fb[13] = data[7];

	gs.imageX+= 8;
	if (gs.imageX == (gs.imageW + gs.trxpos.dx)) {
		gs.imageX = gs.trxpos.dx; gs.imageY++;
	}
}

void FBwrite24_3(u32 *data) {
	u8 *data8 = (u8*)data;
	int i;

	for (i=0; i<16; i++) {
		TransferPixel24((u8*)(data8+i*3));
	}
}

void FBwrite24_3B(u32 *data) {
	u8 *data8 = (u8*)data;
	int i;

	for (i=0; i<16; i++) {
		TransferPixel24B((u8*)(data8+i*3));
	}
}

void FBwrite24Z_3(u32 *data) {
	u8 *data8 = (u8*)data;
	int i;

	for (i=0; i<16; i++) {
		TransferPixel24Z((u8*)(data8+i*3));
	}
}

void FBwrite16(u32 *data) {
	const u16 *data16 = (u16*)data;
	TransferPixel16(data16[0]);
	TransferPixel16(data16[1]);
	TransferPixel16(data16[2]);
	TransferPixel16(data16[3]);
}

void FBwrite16B(u32 *data) {
	const u16 *data16 = (u16*)data;
	TransferPixel16B(data16[0]);
	TransferPixel16B(data16[1]);
	TransferPixel16B(data16[2]);
	TransferPixel16B(data16[3]);
}

void FBwrite16Z(u32 *data) {
	const u16 *data16 = (u16*)data;
	TransferPixel16Z(data16[0]);
	TransferPixel16Z(data16[1]);
	TransferPixel16Z(data16[2]);
	TransferPixel16Z(data16[3]);
}

void FBwrite16S(u32 *data) {
	const u16 *data16 = (u16*)data;
	TransferPixel16S(data16[0]);
	TransferPixel16S(data16[1]);
	TransferPixel16S(data16[2]);
	TransferPixel16S(data16[3]);
}

void FBwrite16SB(u32 *data) {
	const u16 *data16 = (u16*)data;
	TransferPixel16SB(data16[0]);
	TransferPixel16SB(data16[1]);
	TransferPixel16SB(data16[2]);
	TransferPixel16SB(data16[3]);
}

void FBwrite16SZ(u32 *data) {
	const u16 *data16 = (u16*)data;
	TransferPixel16SZ(data16[0]);
	TransferPixel16SZ(data16[1]);
	TransferPixel16SZ(data16[2]);
	TransferPixel16SZ(data16[3]);
}

void FBwrite8(u32 *data) {
	const u8 *data8 = (u8*)data;
	TransferPixel8(data8[0]);
	TransferPixel8(data8[1]);
	TransferPixel8(data8[2]);
	TransferPixel8(data8[3]);
}

void FBwrite8B(u32 *data) {
	const u8 *data8 = (u8*)data;
	TransferPixel8B(data8[0]);
	TransferPixel8B(data8[1]);
	TransferPixel8B(data8[2]);
	TransferPixel8B(data8[3]);
}

void FBwrite8H(u32 *data) {
	const u8 *data8 = (u8*)data;
	TransferPixel8H(data8[0]);
	TransferPixel8H(data8[1]);
	TransferPixel8H(data8[2]);
	TransferPixel8H(data8[3]);
}

void FBwrite8HB(u32 *data) {
	const u8 *data8 = (u8*)data;
	TransferPixel8HB(data8[0]);
	TransferPixel8HB(data8[1]);
	TransferPixel8HB(data8[2]);
	TransferPixel8HB(data8[3]);
}

void FBwrite4(u32 *data) {
	const u8 *data8 = (u8*)data;
	TransferPixel4(data8[0]);
	TransferPixel4(data8[1]);
	TransferPixel4(data8[2]);
	TransferPixel4(data8[3]);
}

void FBwrite4B(u32 *data) {
	const u8 *data8 = (u8*)data;
	TransferPixel4B(data8[0]);
	TransferPixel4B(data8[1]);
	TransferPixel4B(data8[2]);
	TransferPixel4B(data8[3]);
}

void FBwrite4HL(u32 *data) {
	const u8 *data8 = (u8*)data;
	TransferPixel4HL(data8[0] & 0xf);
	TransferPixel4HL(data8[0] >>  4);
	TransferPixel4HL(data8[1] & 0xf);
	TransferPixel4HL(data8[1] >>  4);
	TransferPixel4HL(data8[2] & 0xf);
	TransferPixel4HL(data8[2] >>  4);
	TransferPixel4HL(data8[3] & 0xf);
	TransferPixel4HL(data8[3] >>  4);
}

void FBwrite4HLB(u32 *data) {
	const u8 *data8 = (u8*)data;
	TransferPixel4HLB(data8[0] & 0xf);
	TransferPixel4HLB(data8[0] >>  4);
	TransferPixel4HLB(data8[1] & 0xf);
	TransferPixel4HLB(data8[1] >>  4);
	TransferPixel4HLB(data8[2] & 0xf);
	TransferPixel4HLB(data8[2] >>  4);
	TransferPixel4HLB(data8[3] & 0xf);
	TransferPixel4HLB(data8[3] >>  4);
}

void FBwrite4HH(u32 *data) {
	const u8 *data8 = (u8*)data;
	TransferPixel4HH(data8[0] <<   4);
	TransferPixel4HH(data8[0] & 0xf0);
	TransferPixel4HH(data8[1] <<   4);
	TransferPixel4HH(data8[1] & 0xf0);
	TransferPixel4HH(data8[2] <<   4);
	TransferPixel4HH(data8[2] & 0xf0);
	TransferPixel4HH(data8[3] <<   4);
	TransferPixel4HH(data8[3] & 0xf0);
}

void FBwrite4HHB(u32 *data) {
	const u8 *data8 = (u8*)data;
	TransferPixel4HHB(data8[0] <<   4);
	TransferPixel4HHB(data8[0] & 0xf0);
	TransferPixel4HHB(data8[1] <<   4);
	TransferPixel4HHB(data8[1] & 0xf0);
	TransferPixel4HHB(data8[2] <<   4);
	TransferPixel4HHB(data8[2] & 0xf0);
	TransferPixel4HHB(data8[3] <<   4);
	TransferPixel4HHB(data8[3] & 0xf0);
}

void FBread(u32 *data) {
	TransferPixelSrc(&data[0]);
	TransferPixelSrc(&data[1]);
	TransferPixelSrc(&data[2]);
	TransferPixelSrc(&data[3]);
}

void FBreadB(u32 *data) {
	TransferPixelSrcB(&data[0]);
	TransferPixelSrcB(&data[1]);
	TransferPixelSrcB(&data[2]);
	TransferPixelSrcB(&data[3]);
}

void FBread32Z(u32 *data) {
	TransferPixelSrc32Z(&data[0]);
	TransferPixelSrc32Z(&data[1]);
	TransferPixelSrc32Z(&data[2]);
	TransferPixelSrc32Z(&data[3]);
}

void FBread24_3(u32 *data) {
	u8 *data8 = (u8*)data;
	int i;

	for (i=0; i<16; i++) {
		TransferPixelSrc24((u8*)(data8+i*3));
	}
}

void FBread24_3B(u32 *data) {
	u8 *data8 = (u8*)data;
	int i;

	for (i=0; i<16; i++) {
		TransferPixelSrc24B((u8*)(data8+i*3));
	}
}

void FBread24Z_3(u32 *data) {
	u8 *data8 = (u8*)data;
	int i;

	for (i=0; i<16; i++) {
		TransferPixelSrc24Z((u8*)(data8+i*3));
	}
}

void FBread16(u32 *data) {
	u16 *data16 = (u16*)data;
	TransferPixelSrc16(&data16[0]);
	TransferPixelSrc16(&data16[1]);
	TransferPixelSrc16(&data16[2]);
	TransferPixelSrc16(&data16[3]);
}

void FBread16B(u32 *data) {
	u16 *data16 = (u16*)data;
	TransferPixelSrc16B(&data16[0]);
	TransferPixelSrc16B(&data16[1]);
	TransferPixelSrc16B(&data16[2]);
	TransferPixelSrc16B(&data16[3]);
}

void FBread16Z(u32 *data) {
	u16 *data16 = (u16*)data;
	TransferPixelSrc16Z(&data16[0]);
	TransferPixelSrc16Z(&data16[1]);
	TransferPixelSrc16Z(&data16[2]);
	TransferPixelSrc16Z(&data16[3]);
}

void FBread16S(u32 *data) {
	u16 *data16 = (u16*)data;
	TransferPixelSrc16S(&data16[0]);
	TransferPixelSrc16S(&data16[1]);
	TransferPixelSrc16S(&data16[2]);
	TransferPixelSrc16S(&data16[3]);
}

void FBread16SB(u32 *data) {
	u16 *data16 = (u16*)data;
	TransferPixelSrc16SB(&data16[0]);
	TransferPixelSrc16SB(&data16[1]);
	TransferPixelSrc16SB(&data16[2]);
	TransferPixelSrc16SB(&data16[3]);
}

void FBread16SZ(u32 *data) {
	u16 *data16 = (u16*)data;
	TransferPixelSrc16SZ(&data16[0]);
	TransferPixelSrc16SZ(&data16[1]);
	TransferPixelSrc16SZ(&data16[2]);
	TransferPixelSrc16SZ(&data16[3]);
}

void FBread8(u32 *data) {
	u8 *data8 = (u8*)data;
	TransferPixelSrc8(&data8[0]);
	TransferPixelSrc8(&data8[1]);
	TransferPixelSrc8(&data8[2]);
	TransferPixelSrc8(&data8[3]);
}

void FBread8B(u32 *data) {
	u8 *data8 = (u8*)data;
	TransferPixelSrc8B(&data8[0]);
	TransferPixelSrc8B(&data8[1]);
	TransferPixelSrc8B(&data8[2]);
	TransferPixelSrc8B(&data8[3]);
}

void FBread8H(u32 *data) {
	u8 *data8 = (u8*)data;
	TransferPixelSrc8H(&data8[0]);
	TransferPixelSrc8H(&data8[1]);
	TransferPixelSrc8H(&data8[2]);
	TransferPixelSrc8H(&data8[3]);
}

void FBread8HB(u32 *data) {
	u8 *data8 = (u8*)data;
	TransferPixelSrc8HB(&data8[0]);
	TransferPixelSrc8HB(&data8[1]);
	TransferPixelSrc8HB(&data8[2]);
	TransferPixelSrc8HB(&data8[3]);
}

void FBread4(u32 *data) {
	u8 *data8 = (u8*)data;
	TransferPixelSrc4(&data8[0]);
	TransferPixelSrc4(&data8[1]);
	TransferPixelSrc4(&data8[2]);
	TransferPixelSrc4(&data8[3]);
}

void FBread4B(u32 *data) {
	u8 *data8 = (u8*)data;
	TransferPixelSrc4B(&data8[0]);
	TransferPixelSrc4B(&data8[1]);
	TransferPixelSrc4B(&data8[2]);
	TransferPixelSrc4B(&data8[3]);
}

void FBread4HL(u32 *data) {
	u8 *data8 = (u8*)data;
	TransferPixelSrc4HL(&data8[0]);
	TransferPixelSrc4HL(&data8[1]);
	TransferPixelSrc4HL(&data8[2]);
	TransferPixelSrc4HL(&data8[3]);
}

void FBread4HLB(u32 *data) {
	u8 *data8 = (u8*)data;
	TransferPixelSrc4HLB(&data8[0]);
	TransferPixelSrc4HLB(&data8[1]);
	TransferPixelSrc4HLB(&data8[2]);
	TransferPixelSrc4HLB(&data8[3]);
}

void FBread4HH(u32 *data) {
	u8 *data8 = (u8*)data;
	TransferPixelSrc4HH(&data8[0]);
	TransferPixelSrc4HH(&data8[1]);
	TransferPixelSrc4HH(&data8[2]);
	TransferPixelSrc4HH(&data8[3]);
}

void FBread4HHB(u32 *data) {
	u8 *data8 = (u8*)data;
	TransferPixelSrc4HHB(&data8[0]);
	TransferPixelSrc4HHB(&data8[1]);
	TransferPixelSrc4HHB(&data8[2]);
	TransferPixelSrc4HHB(&data8[3]);
}

void FBtransferImageB(u32 *pMem, int size) {
//	printf("GS IMAGE transferB %d, gs.dstbuf.psm=%x; %dx%d %dx%d (bp=%x)\n", size, gs.dstbuf.psm, gs.imageX, gs.imageY, gs.imageW, gs.imageH, gs.dstbuf.bp);

	switch (gs.dstbuf.psm) {
		case 0x01: // PSMCT24
			while (size >= 3) {
				FBwrite24_3B(pMem); pMem+= 12; size-=3;
			}
			break;

		case 0x02: // PSMCT16
			while (size > 0) {
				FBwrite16B(pMem); pMem+= 2;
				FBwrite16B(pMem); pMem+= 2; size--;
			}
			break;

		case 0x0A: // PSMCT16S
			while (size > 0) {
				FBwrite16SB(pMem); pMem+= 2;
				FBwrite16SB(pMem); pMem+= 2; size--;
			}
			break;

		case 0x13: // PSMT8
			while (size > 0) {
				FBwrite8B(pMem); pMem++;
				FBwrite8B(pMem); pMem++;
				FBwrite8B(pMem); pMem++;
				FBwrite8B(pMem); pMem++; size--;
			}
			break;

		case 0x14: // PSMT4
			while (size > 0) {
				FBwrite4B(pMem); pMem++;
				FBwrite4B(pMem); pMem++;
				FBwrite4B(pMem); pMem++;
				FBwrite4B(pMem); pMem++; size--;
			}
			break;

		case 0x1B: // PSMT8H
			while (size > 0) {
				FBwrite8HB(pMem); pMem++;
				FBwrite8HB(pMem); pMem++;
				FBwrite8HB(pMem); pMem++;
				FBwrite8HB(pMem); pMem++; size--;
			}
			break;

		case 0x24: // PSMT4HL
			while (size > 0) {
				FBwrite4HLB(pMem); pMem++;
				FBwrite4HLB(pMem); pMem++;
				FBwrite4HLB(pMem); pMem++;
				FBwrite4HLB(pMem); pMem++; size--;
			}
			break;

		case 0x2C: // PSMT4HH
			while (size > 0) {
				FBwrite4HHB(pMem); pMem++;
				FBwrite4HHB(pMem); pMem++;
				FBwrite4HHB(pMem); pMem++;
				FBwrite4HHB(pMem); pMem++; size--;
			}
			break;

		case 0x30: // PSMZ32
			while (size > 0) {
				FBwrite32Z(pMem); pMem+= 4; size--;
			}
			break;

		case 0x31: // PSMZ24
			while (size >= 3) {
				FBwrite24Z_3(pMem); pMem+= 12; size-=3;
			}
			break;

		case 0x32: // PSMZ16
			while (size > 0) {
				FBwrite16Z(pMem); pMem+= 2;
				FBwrite16Z(pMem); pMem+= 2; size--;
			}
			break;

		case 0x3A: // PSMZ16S
			while (size > 0) {
				FBwrite16SZ(pMem); pMem+= 2;
				FBwrite16SZ(pMem); pMem+= 2; size--;
			}
			break;

		default:
		case 0x00: // PSMCT32
			if (gs.dstbuf.psm != 0) printf("gs.dstbuf.psm == %x!!\n", gs.dstbuf.psm);
			while (size > 0) {
				FBwriteB(pMem); pMem+= 4; size--;
			}
			break;
	}
}

void _TransferImage8(u32 *pMem, int size) {
/*	if ((gs.imageX & 0x1f) == 0 && (gs.imageY & 0xf) == 0 &&
		((gs.imageW + gs.trxpos.dx) & 0x1f) == 0) {
		while (size > 2) {
			FBwrite8B_8(pMem); pMem+= 4; size--;
		}
#ifdef __i386__
		_sfence();
		_emms();
#endif
	}*/
	while (size > 0) {
		FBwrite8(pMem); pMem++;
		FBwrite8(pMem); pMem++;
		FBwrite8(pMem); pMem++;
		FBwrite8(pMem); pMem++; size--;
	}
}

void _TransferImage32(u32 *pMem, int size) {
	int s2size;
	size*= 4;

	if ((gs.imageX & 0x7) != 0 || (gs.imageY & 0x7) != 0) {
		while (size > 0) {
			TransferPixel(*pMem++); size--;
		}
		return;
	}

	if ((gs.imageW & 0x7) == 0) {
		s2size = gs.imageW*2;
		while (size >= s2size) {
			int i;

			for (i=0; i<gs.imageW/8; i++) {
				FBwrite_8x8(pMem); pMem+= 8;
			}
			pMem+= gs.imageW; size-= s2size;
		}
	}

	while (size > 0) {
		if (gs.imageX < (gs.imageW-8)) {
			while (size >= 2) {
				FBwrite_8(pMem); pMem+= 8; size-= 8;
			}
#ifdef __i386__
//			_sfence();
//			_emms();
#endif
		}
		while (size > 0) {
			TransferPixel(*pMem++); size--;
		}
	}
}

void FBtransferImage(u32 *pMem, int size) {
#ifdef GS_LOG
	GS_LOG("GS IMAGE transfer %d, gs.dstbuf.psm=%x; %dx%d\n", size, gs.dstbuf.psm, gs.imageW, gs.imageH);
#endif
	bpf+= size*16;

	if (conf.cache) {
		CacheClear(gs.dstbuf.bp, size*4);
	}

#ifndef _OPTIMIZE
	FBtransferImageB(pMem, size);
	return;
#endif
	if (gs.dstbuf.bp & 0x1f) {
		FBtransferImageB(pMem, size);
		return;
	}

	switch (gs.dstbuf.psm) {
		case 0x01: // PSMCT24
			while (size >= 3) {
				FBwrite24_3(pMem); pMem+= 12; size-=3;
			}
			break;

		case 0x02: // PSMCT16
			while (size > 0) {
				FBwrite16(pMem); pMem+= 2;
				FBwrite16(pMem); pMem+= 2; size--;
			}
			break;

		case 0x0A: // PSMCT16S
			while (size > 0) {
				FBwrite16S(pMem); pMem+= 2;
				FBwrite16S(pMem); pMem+= 2; size--;
			}
			break;

		case 0x13: // PSMT8
			_TransferImage8(pMem, size);
			break;

		case 0x14: // PSMT4
			while (size > 0) {
				FBwrite4(pMem); pMem++;
				FBwrite4(pMem); pMem++;
				FBwrite4(pMem); pMem++;
				FBwrite4(pMem); pMem++; size--;
			}
			break;

		case 0x1B: // PSMT8H
			while (size > 0) {
				FBwrite8H(pMem); pMem++;
				FBwrite8H(pMem); pMem++;
				FBwrite8H(pMem); pMem++;
				FBwrite8H(pMem); pMem++; size--;
			}
			break;

		case 0x24: // PSMT4HL
			while (size > 0) {
				FBwrite4HL(pMem); pMem++;
				FBwrite4HL(pMem); pMem++;
				FBwrite4HL(pMem); pMem++;
				FBwrite4HL(pMem); pMem++; size--;
			}
			break;

		case 0x2C: // PSMT4HH
			while (size > 0) {
				FBwrite4HH(pMem); pMem++;
				FBwrite4HH(pMem); pMem++;
				FBwrite4HH(pMem); pMem++;
				FBwrite4HH(pMem); pMem++; size--;
			}
			break;

		case 0x30: // PSMZ32
			while (size > 0) {
				FBwrite32Z(pMem); pMem+= 4; size--;
			}
			break;

		case 0x31: // PSMZ24
			while (size >= 3) {
				FBwrite24Z_3(pMem); pMem+= 12; size-=3;
			}
			break;

		case 0x32: // PSMZ16
			while (size > 0) {
				FBwrite16Z(pMem); pMem+= 2;
				FBwrite16Z(pMem); pMem+= 2; size--;
			}
			break;

		case 0x3A: // PSMZ16S
			while (size > 0) {
				FBwrite16SZ(pMem); pMem+= 2;
				FBwrite16SZ(pMem); pMem+= 2; size--;
			}
			break;

		default:
		case 0x00: // PSMCT32
			if (gs.dstbuf.psm != 0) printf("gs.dstbuf.psm == %x!!\n", gs.dstbuf.psm);
			_TransferImage32(pMem, size);
			break;
	}
}

void FBtransferImageSrcB(u32 *pMem, int size) {
#ifdef GS_LOG
	GS_LOG("GS IMAGE SRC transfer %d, gs.srcbuf.psm=%x\n", size, gs.srcbuf.psm);
#endif

	switch (gs.srcbuf.psm) {
		case 0x01: // PSMCT24
			while (size >= 3) {
				FBread24_3B(pMem); pMem+= 12; size-=3;
			}
			break;

		case 0x02: // PSMCT16
			while (size > 0) {
				FBread16B(pMem); pMem+= 2;
				FBread16B(pMem); pMem+= 2; size--;
			}
			break;

		case 0x0A: // PSMCT16S
			while (size > 0) {
				FBread16SB(pMem); pMem+= 2;
				FBread16SB(pMem); pMem+= 2; size--;
			}
			break;

		case 0x13: // PSMT8
			while (size > 0) {
				FBread8B(pMem); pMem++;
				FBread8B(pMem); pMem++;
				FBread8B(pMem); pMem++;
				FBread8B(pMem); pMem++; size--;
			}
			break;

		case 0x14: // PSMT4
			while (size > 0) {
				FBread4B(pMem); pMem++;
				FBread4B(pMem); pMem++;
				FBread4B(pMem); pMem++;
				FBread4B(pMem); pMem++; size--;
			}
			break;

		case 0x1B: // PSMT8H
			while (size > 0) {
				FBread8HB(pMem); pMem++;
				FBread8HB(pMem); pMem++;
				FBread8HB(pMem); pMem++;
				FBread8HB(pMem); pMem++; size--;
			}
			break;

		case 0x24: // PSMT4HL
			while (size > 0) {
				FBread4HLB(pMem); pMem++;
				FBread4HLB(pMem); pMem++;
				FBread4HLB(pMem); pMem++;
				FBread4HLB(pMem); pMem++; size--;
			}
			break;

		case 0x2C: // PSMT4HH
			while (size > 0) {
				FBread4HHB(pMem); pMem++;
				FBread4HHB(pMem); pMem++;
				FBread4HHB(pMem); pMem++;
				FBread4HHB(pMem); pMem++; size--;
			}
			break;

		case 0x30: // PSMZ32
			while (size > 0) {
				FBread32Z(pMem); pMem+= 4; size--;
			}
			break;

		case 0x31: // PSMZ24
			while (size >= 3) {
				FBread24Z_3(pMem); pMem+= 12; size-=3;
			}
			break;

		case 0x32: // PSMZ16
			while (size > 0) {
				FBread16Z(pMem); pMem+= 2;
				FBread16Z(pMem); pMem+= 2; size--;
			}
			break;

		case 0x3A: // PSMZ16S
			while (size > 0) {
				FBread16SZ(pMem); pMem+= 2;
				FBread16SZ(pMem); pMem+= 2; size--;
			}
			break;

		default:
		case 0x00: // PSMCT32
			if (gs.srcbuf.psm != 0) printf("gs.srcbuf.psm == %d!!\n", gs.srcbuf.psm);
			while (size > 0) {
				FBreadB(pMem); pMem+= 4; size--;
			}
			break;
	}
}

void FBtransferImageSrc(u32 *pMem, int size) {
#ifdef GS_LOG
	GS_LOG("GS IMAGE SRC transfer %d, gs.srcbuf.psm=%x\n", size, gs.srcbuf.psm);
#endif

	if (gs.srcbuf.bp & 0x1f) {
		FBtransferImageSrcB(pMem, size);
		return;
	}

	switch (gs.srcbuf.psm) {
		case 0x01: // PSMCT24
			while (size >= 3) {
				FBread24_3(pMem); pMem+= 12; size-=3;
			}
			break;

		case 0x02: // PSMCT16
			while (size > 0) {
				FBread16(pMem); pMem+= 2;
				FBread16(pMem); pMem+= 2; size--;
			}
			break;

		case 0x0A: // PSMCT16S
			while (size > 0) {
				FBread16S(pMem); pMem+= 2;
				FBread16S(pMem); pMem+= 2; size--;
			}
			break;

		case 0x13: // PSMT8
			while (size> 0) {
				FBread8(pMem); pMem++;
				FBread8(pMem); pMem++;
				FBread8(pMem); pMem++;
				FBread8(pMem); pMem++; size--;
			}
			break;

		case 0x14: // PSMT4
			while (size > 0) {
				FBread4(pMem); pMem++;
				FBread4(pMem); pMem++;
				FBread4(pMem); pMem++;
				FBread4(pMem); pMem++; size--;
			}
			break;

		case 0x1B: // PSMT8H
			while (size > 0) {
				FBread8H(pMem); pMem++;
				FBread8H(pMem); pMem++;
				FBread8H(pMem); pMem++;
				FBread8H(pMem); pMem++; size--;
			}
			break;

		case 0x24: // PSMT4HL
			while (size > 0) {
				FBread4HL(pMem); pMem++;
				FBread4HL(pMem); pMem++;
				FBread4HL(pMem); pMem++;
				FBread4HL(pMem); pMem++; size--;
			}
			break;

		case 0x2C: // PSMT4HH
			while (size > 0) {
				FBread4HH(pMem); pMem++;
				FBread4HH(pMem); pMem++;
				FBread4HH(pMem); pMem++;
				FBread4HH(pMem); pMem++; size--;
			}
			break;

		case 0x30: // PSMZ32
			while (size > 0) {
				FBread32Z(pMem); pMem+= 4; size--;
			}
			break;

		case 0x31: // PSMZ24
			while (size >= 3) {
				FBread24Z_3(pMem); pMem+= 12; size-=3;
			}
			break;

		case 0x32: // PSMZ16
			while (size > 0) {
				FBread16Z(pMem); pMem+= 2;
				FBread16Z(pMem); pMem+= 2; size--;
			}
			break;

		case 0x3A: // PSMZ16S
			while (size > 0) {
				FBread16SZ(pMem); pMem+= 2;
				FBread16SZ(pMem); pMem+= 2; size--;
			}
			break;

		default:
		case 0x00: // PSMCT32
			if (gs.srcbuf.psm != 0) printf("gs.srcbuf.psm == %d!!\n", gs.srcbuf.psm);
			while (size > 0) {
				FBread(pMem); pMem+= 4; size--;
			}
			break;
	}
}

