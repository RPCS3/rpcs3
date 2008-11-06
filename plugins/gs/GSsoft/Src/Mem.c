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
#include "Page.h"


void writePixel32(int x, int y, u32 pixel, u32 bp, u32 bw) {
	vRamUL[getPixelAddress32(x, y, bp, bw)] = pixel;
}

void writePixel24(int x, int y, u32 pixel, u32 bp, u32 bw) {
	u8 *buf = (u8*)&vRamUL[getPixelAddress32(x, y, bp, bw)];
	u8 *pix = (u8*)&pixel;
	buf[0] = pix[0]; buf[1] = pix[1]; buf[2] = pix[2];
}

void writePixel16(int x, int y, u16 pixel, u32 bp, u32 bw) {
	vRamUS[getPixelAddress16(x, y, bp, bw)] = pixel;
}

void writePixel16S(int x, int y, u16 pixel, u32 bp, u32 bw) {
	vRamUS[getPixelAddress16S(x, y, bp, bw)] = pixel;
}

void writePixel8(int x, int y, u8  pixel, u32 bp, u32 bw) {
	vRam[getPixelAddress8(x, y, bp, bw)] = pixel;
}

void writePixel8H(int x, int y, u8  pixel, u32 bp, u32 bw) {
	((u8*)&vRamUL[getPixelAddress32(x, y, bp, bw)])[3] = pixel;
}

void writePixel4(int x, int y, u8  pixel, u32 bp, u32 bw) {
	u32 addr = getPixelAddress4(x, y, bp, bw);
	u8 pix = vRam[addr/2];
	if (addr & 0x1)
	     vRam[addr/2] = (pix & 0x0f) | (pixel << 4);
	else vRam[addr/2] = (pix & 0xf0) | (pixel);
}

void writePixel4HL(int x, int y, u8  pixel, u32 bp, u32 bw) {
	u8 *p = &((u8*)&vRamUL[getPixelAddress32(x, y, bp, bw)])[3];
	*p = (*p & 0xf0) | pixel;
}

void writePixel4HH(int x, int y, u8  pixel, u32 bp, u32 bw) {
	u8 *p = &((u8*)&vRamUL[getPixelAddress32(x, y, bp, bw)])[3];
	*p = (*p & 0x0f) | pixel;
}


void writePixel32Z(int x, int y, u32 pixel, u32 bp, u32 bw) {
	vRamUL[getPixelAddress32Z(x, y, bp, bw)] = pixel;
}

void writePixel24Z(int x, int y, u32 pixel, u32 bp, u32 bw) {
	u8 *buf = (u8*)&vRamUL[getPixelAddress32Z(x, y, bp, bw)];
	u8 *pix = (u8*)&pixel;
	buf[0] = pix[0]; buf[1] = pix[1]; buf[2] = pix[2];
}

void writePixel16Z(int x, int y, u16 pixel, u32 bp, u32 bw) {
	vRamUS[getPixelAddress16Z(x, y, bp, bw)] = pixel;
}

void writePixel16SZ(int x, int y, u16 pixel, u32 bp, u32 bw) {
	vRamUS[getPixelAddress16SZ(x, y, bp, bw)] = pixel;
}


///////////////

u32  readPixel32(int x, int y, u32 bp, u32 bw) {
	return vRamUL[getPixelAddress32(x, y, bp, bw)];
}

u32  readPixel24(int x, int y, u32 bp, u32 bw) {
	return vRamUL[getPixelAddress32(x, y, bp, bw)] & 0xffffff;
}

u16  readPixel16(int x, int y, u32 bp, u32 bw) {
	return vRamUS[getPixelAddress16(x, y, bp, bw)];
}

u16  readPixel16S(int x, int y, u32 bp, u32 bw) {
	return vRamUS[getPixelAddress16S(x, y, bp, bw)];
}

u8   readPixel8(int x, int y, u32 bp, u32 bw) {
	return vRam[getPixelAddress8(x, y, bp, bw)];
}

u8   readPixel8H(int x, int y, u32 bp, u32 bw) {
	return ((u8*)&vRamUL[getPixelAddress32(x, y, bp, bw)])[3];
}

u8   readPixel4(int x, int y, u32 bp, u32 bw) {
	u32 addr = getPixelAddress4(x, y, bp, bw);
	u8 pix = vRam[addr/2];
	if (addr & 0x1)
	     return pix >> 4;
	else return pix & 0xf;
}

u8   readPixel4HL(int x, int y, u32 bp, u32 bw) {
	u8 *p = &((u8*)&vRamUL[getPixelAddress32(x, y, bp, bw)])[3];
	return *p & 0x0f;
}

u8   readPixel4HH(int x, int y, u32 bp, u32 bw) {
	u8 *p = &((u8*)&vRamUL[getPixelAddress32(x, y, bp, bw)])[3];
	return *p >> 4;
}

///////////////

u32  readPixel32Z(int x, int y, u32 bp, u32 bw) {
	return vRamUL[getPixelAddress32Z(x, y, bp, bw)];
}

u32  readPixel24Z(int x, int y, u32 bp, u32 bw) {
	return vRamUL[getPixelAddress32Z(x, y, bp, bw)] & 0xffffff;
}

u16  readPixel16Z(int x, int y, u32 bp, u32 bw) {
	return vRamUS[getPixelAddress16Z(x, y, bp, bw)];
}

u16  readPixel16SZ(int x, int y, u32 bp, u32 bw) {
	return vRamUS[getPixelAddress16SZ(x, y, bp, bw)];
}

///////////////

u32  readPixel16to32(int x, int y, u32 bp, u32 bw) {
	return RGBA16to32(vRamUS[getPixelAddress16(x, y, bp, bw)]);
}

u32  readPixel16Sto32(int x, int y, u32 bp, u32 bw) {
	return RGBA16to32(vRamUS[getPixelAddress16S(x, y, bp, bw)]);
}

///////////////

u32  readPixel16Zto32(int x, int y, u32 bp, u32 bw) {
	return RGBA16to32(vRamUS[getPixelAddress16Z(x, y, bp, bw)]);
}

u32  readPixel16SZto32(int x, int y, u32 bp, u32 bw) {
	return RGBA16to32(vRamUS[getPixelAddress16SZ(x, y, bp, bw)]);
}

