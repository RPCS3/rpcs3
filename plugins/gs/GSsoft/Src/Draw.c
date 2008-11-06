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

#include "Draw.h"
#include "Mem.h"
#include "Page.h"

#ifdef __MSCW32__
#pragma warning(disable:4244)
#endif

void (*_ScrBlit)(u8 *, int, int, int, Rect *, u32, u32, u32);
void (*_ScrBlitMerge)(u8 *, int, int, int);


////////////////////
// 15bit

void _ScrBlit32to15(u8 *scr, int dw, int dh, int sp, Rect *gsScr, u32 bp, u32 bw) {
	u16 *pscr;
	int x, y;
	int xpos, xinc;
	int ypos, yinc;
	u32 c;

	xinc = (gsScr->w << 16) / dw;
	yinc = (gsScr->h << 16) / dh;

	for (y=0, ypos=gsScr->y<<16; y<dh; y++, ypos+=yinc) {
		pscr = (u16*)(scr + y * sp);
		for(x=0, xpos=gsScr->x<<16; x<dw; x++, xpos+= xinc) {
			c = readPixel32(xpos>>16, ypos>>16, bp, bw);
			*pscr++= ((c & 0x0000f8) <<  7) | /* 0x7c00 */
					 ((c & 0x00f800) >>  6) | /* 0x03e0 */
					 ((c & 0xf80000) >> 19);  /* 0x001f */
		}
	}
}

void _ScrBlit16to15(u8 *scr, int dw, int dh, int sp, Rect *gsScr, u32 bp, u32 bw) {
	u16 *pscr;
	int x, y;
	int xpos, xinc;
	int ypos, yinc;
	u16 c;

	xinc = (gsScr->w << 16) / dw;
	yinc = (gsScr->h << 16) / dh;

	for (y=0, ypos=gsScr->y<<16; y<dh; y++, ypos+=yinc) {
		pscr = (u16*)(scr + y * sp);
		for(x=0, xpos=gsScr->x<<16; x<dw; x++, xpos+= xinc) {
			c = readPixel16(xpos>>16, ypos>>16, bp, bw);
			*pscr++= ((c & 0x001f) << 10) | /* 0x7c00 */
					 ((c & 0x03e0)      ) | /* 0x03e0 */
					 ((c & 0x7c00) >> 10);  /* 0x001f */
		}
	}
}

void _ScrBlit16Sto15(u8 *scr, int dw, int dh, int sp, Rect *gsScr, u32 bp, u32 bw) {
	u16 *pscr;
	int x, y;
	int xpos, xinc;
	int ypos, yinc;
	u16 c;

	xinc = (gsScr->w << 16) / dw;
	yinc = (gsScr->h << 16) / dh;

	for (y=0, ypos=gsScr->y<<16; y<dh; y++, ypos+=yinc) {
		pscr = (u16*)(scr + y * sp);
		for(x=0, xpos=gsScr->x<<16; x<dw; x++, xpos+= xinc) {
			c = readPixel16S(xpos>>16, ypos>>16, bp, bw);
			*pscr++= ((c & 0x001f) << 10) | /* 0x7c00 */
					 ((c & 0x03e0)      ) | /* 0x03e0 */
					 ((c & 0x7c00) >> 10);  /* 0x001f */
		}
	}
}

void ScrBlit15(u8 *scr, int dw, int dh, int sp, Rect *gsScr, u32 psm, u32 bp, u32 bw) {
	switch (psm) {
		case 0x2: _ScrBlit16to15(scr, dw, dh, sp, gsScr, bp, bw); break;
		case 0xA: _ScrBlit16Sto15(scr, dw, dh, sp, gsScr, bp, bw); break;
		default:  _ScrBlit32to15(scr, dw, dh, sp, gsScr, bp, bw); break;
	}
}

////////////////////
// 16bit

void _ScrBlit32to16(u8 *scr, int dw, int dh, int sp, Rect *gsScr, u32 bp, u32 bw) {
	u16 *pscr;
	int x, y;
	int xpos, xinc;
	int ypos, yinc;
	u32 c;

	xinc = (gsScr->w << 16) / dw;
	yinc = (gsScr->h << 16) / dh;

	for (y=0, ypos=gsScr->y<<16; y<dh; y++, ypos+=yinc) {
		pscr = (u16*)(scr + y * sp);
		for(x=0, xpos=gsScr->x<<16; x<dw; x++, xpos+= xinc) {
			c = readPixel32(xpos>>16, ypos>>16, bp, bw);
			*pscr++= ((c & 0x0000f8) <<  8) | /* 0xf800 */
					 ((c & 0x00fc00) >>  5) | /* 0x07e0 */
					 ((c & 0xf80000) >> 19);  /* 0x001f */
		}
	}
}

void _ScrBlit16to16(u8 *scr, int dw, int dh, int sp, Rect *gsScr, u32 bp, u32 bw) {
	u16 *pscr;
	int x, y;
	int xpos, xinc;
	int ypos, yinc;
	u16 c;

	xinc = (gsScr->w << 16) / dw;
	yinc = (gsScr->h << 16) / dh;

	for (y=0, ypos=gsScr->y<<16; y<dh; y++, ypos+=yinc) {
		pscr = (u16*)(scr + y * sp);
		for(x=0, xpos=gsScr->x<<16; x<dw; x++, xpos+= xinc) {
			c = readPixel16(xpos>>16, ypos>>16, bp, bw);
			*pscr++= ((c & 0x001f) << 11) | /* 0xf800 */
					 ((c & 0x03e0) <<  1) | /* 0x07e0 */
					 ((c & 0x7c00) >> 10);  /* 0x001f */
		}
	}
}

void _ScrBlit16Sto16(u8 *scr, int dw, int dh, int sp, Rect *gsScr, u32 bp, u32 bw) {
	u16 *pscr;
	int x, y;
	int xpos, xinc;
	int ypos, yinc;
	u16 c;

	xinc = (gsScr->w << 16) / dw;
	yinc = (gsScr->h << 16) / dh;

	for (y=0, ypos=gsScr->y<<16; y<dh; y++, ypos+=yinc) {
		pscr = (u16*)(scr + y * sp);
		for(x=0, xpos=gsScr->x<<16; x<dw; x++, xpos+= xinc) {
			c = readPixel16(xpos>>16, ypos>>16, bp, bw);
			*pscr++= ((c & 0x001f) << 11) | /* 0xf800 */
					 ((c & 0x03e0) <<  1) | /* 0x07e0 */
					 ((c & 0x7c00) >> 10);  /* 0x001f */
		}
	}
}

void ScrBlit16(u8 *scr, int dw, int dh, int sp, Rect *gsScr, u32 psm, u32 bp, u32 bw) {
	switch (psm) {
		case 0x2: _ScrBlit16to16(scr, dw, dh, sp, gsScr, bp, bw); break;
		case 0xA: _ScrBlit16Sto16(scr, dw, dh, sp, gsScr, bp, bw); break;
		default:  _ScrBlit32to16(scr, dw, dh, sp, gsScr, bp, bw); break;
	}
}

////////////////////
// 32bit

void _ScrBlit32to32(u8 *scr, int dw, int dh, int sp, Rect *gsScr, u32 bp, u32 bw) {
	u8 *pscr;
	int x, y;
	int xpos, xinc;
	int ypos, yinc;
	u32 c;

	xinc = (gsScr->w << 16) / dw;
	yinc = (gsScr->h << 16) / dh;

	for (y=0, ypos=gsScr->y<<16; y<dh; y++, ypos+=yinc) {
		pscr = (u8 *)(scr + y * sp);
		for(x=0, xpos=gsScr->x<<16; x<dw; x++, xpos+= xinc) {
			c = readPixel32(xpos>>16, ypos>>16, bp, bw);
			pscr[0] = (c >> 16) & 0xff;
			pscr[1] = (c >>  8) & 0xff;
			pscr[2] = c & 0xff;
			pscr+=4;
		}
	}
}

void _ScrBlit16to32(u8 *scr, int dw, int dh, int sp, Rect *gsScr, u32 bp, u32 bw) {
	u8  *pscr;
	int x, y;
	int xpos, xinc;
	int ypos, yinc;
	u16 c;

	xinc = (gsScr->w << 16) / dw;
	yinc = (gsScr->h << 16) / dh;

	for (y=0, ypos=gsScr->y<<16; y<dh; y++, ypos+=yinc) {
		pscr = (u8 *)(scr + y * sp);
		for(x=0, xpos=gsScr->x<<16; x<dw; x++, xpos+= xinc) {
			c = readPixel16(xpos>>16, ypos>>16, bp, bw);
			pscr[0] = (c >> 7) & 0xf8;
			pscr[1] = (c >> 2) & 0xf8;
			pscr[2] = (c << 3) & 0xf8;
			pscr+=4;
		}
	}
}

void _ScrBlit16Sto32(u8 *scr, int dw, int dh, int sp, Rect *gsScr, u32 bp, u32 bw) {
	u8  *pscr;
	int x, y;
	int xpos, xinc;
	int ypos, yinc;
	u16 c;

	xinc = (gsScr->w << 16) / dw;
	yinc = (gsScr->h << 16) / dh;

	for (y=0, ypos=gsScr->y<<16; y<dh; y++, ypos+=yinc) {
		pscr = (u8 *)(scr + y * sp);
		for(x=0, xpos=gsScr->x<<16; x<dw; x++, xpos+= xinc) {
			c = readPixel16S(xpos>>16, ypos>>16, bp, bw);
			pscr[0] = (c >> 7) & 0xf8;
			pscr[1] = (c >> 2) & 0xf8;
			pscr[2] = (c << 3) & 0xf8;
			pscr+=4;
		}
	}
}

void ScrBlit32(u8 *scr, int dw, int dh, int sp, Rect *gsScr, u32 psm, u32 bp, u32 bw) {
	switch (psm) {
		case 0x2: _ScrBlit16to32(scr, dw, dh, sp, gsScr, bp, bw); break;
		case 0xA: _ScrBlit16Sto32(scr, dw, dh, sp, gsScr, bp, bw); break;
		default:  _ScrBlit32to32(scr, dw, dh, sp, gsScr, bp, bw); break;
	}
}


void ScrBlitMerge15(u8 *scr, int dw, int dh, int sp) {
	u32 (*_readPixel1)(int x, int y, u32 bp, u32 bw);
	u32 (*_readPixel2)(int x, int y, u32 bp, u32 bw);
	u16 *pscr;
	int x, y;
	int xpos1, xinc1;
	int ypos1, yinc1;
	int xpos2, xinc2;
	int ypos2, yinc2;
	u32 alpha;
	u32 c1, c2;
	u32 r1,g1,b1;
	u32 r2,g2,b2;

	switch (gs.DISPFB[0].psm) {
		case 0x2: _readPixel1 = readPixel16to32; break;
		case 0xA: _readPixel1 = readPixel16Sto32; break;
		default:  _readPixel1 = readPixel32; break;
	}
	switch (gs.DISPFB[1].psm) {
		case 0x2: _readPixel2 = readPixel16to32; break;
		case 0xA: _readPixel2 = readPixel16Sto32; break;
		default:  _readPixel2 = readPixel32; break;
	}

	xinc1 = (gs.DISPLAY[0].w << 16) / dw;
	yinc1 = (gs.DISPLAY[0].h << 16) / dh;

	ypos1 = gs.DISPLAY[0].y<<16;

	xinc2 = (gs.DISPLAY[1].w << 16) / dw;
	yinc2 = (gs.DISPLAY[1].h << 16) / dh;

	ypos2 = gs.DISPLAY[1].y<<16;

//	if (gs.PMODE.alp) alpha = gs.PMODE.alp;
	alpha = 0x80;

	for (y=0; y<dh; y++) {
		pscr = (u16 *)(scr + y * sp);
		xpos1 = gs.DISPLAY[0].x<<16;
		xpos2 = gs.DISPLAY[1].x<<16;
		for(x=0; x<dw; x++) {
			c1 = _readPixel1(xpos1>>16, ypos1>>16, gs.DISPFB[0].fbp, gs.DISPFB[0].fbw);
			r1 = (c1 >> 16) & 0xff;
			g1 = (c1 >>  8) & 0xff;
			b1 = (c1      ) & 0xff;
			c2 = _readPixel2(xpos2>>16, ypos2>>16, gs.DISPFB[1].fbp, gs.DISPFB[1].fbw);
			r2 = (c2 >> 16) & 0xff;
			g2 = (c2 >>  8) & 0xff;
			b2 = (c2      ) & 0xff;

			r1 = ((r1 * alpha) + (r2 * (0xff - alpha))) / alpha;
			g1 = ((g1 * alpha) + (g2 * (0xff - alpha))) / alpha;
			b1 = ((b1 * alpha) + (b2 * (0xff - alpha))) / alpha;
			if (r1 > 0xff) r1 = 0xff;
			if (g1 > 0xff) g1 = 0xff;
			if (b1 > 0xff) b1 = 0xff;

			*pscr++= ((b1 >> 3) << 10) | /* 0x7c00 */
					 ((g1 >> 3) <<  5) | /* 0x03e0 */
					 ((r1 >> 3)      );  /* 0x001f */
			xpos1+= xinc1;
			xpos2+= xinc2;
		}
		ypos1+= yinc1;
		ypos2+= yinc2;
	}
}

void ScrBlitMerge16(u8 *scr, int dw, int dh, int sp) {
	u32 (*_readPixel1)(int x, int y, u32 bp, u32 bw);
	u32 (*_readPixel2)(int x, int y, u32 bp, u32 bw);
	u16 *pscr;
	int x, y;
	int xpos1, xinc1;
	int ypos1, yinc1;
	int xpos2, xinc2;
	int ypos2, yinc2;
	u32 alpha;
	u32 c1, c2;
	u32 r1,g1,b1;
	u32 r2,g2,b2;

	switch (gs.DISPFB[0].psm) {
		case 0x2: _readPixel1 = readPixel16to32; break;
		case 0xA: _readPixel1 = readPixel16Sto32; break;
		default:  _readPixel1 = readPixel32; break;
	}
	switch (gs.DISPFB[1].psm) {
		case 0x2: _readPixel2 = readPixel16to32; break;
		case 0xA: _readPixel2 = readPixel16Sto32; break;
		default:  _readPixel2 = readPixel32; break;
	}

	xinc1 = (gs.DISPLAY[0].w << 16) / dw;
	yinc1 = (gs.DISPLAY[0].h << 16) / dh;

	ypos1 = gs.DISPLAY[0].y<<16;

	xinc2 = (gs.DISPLAY[1].w << 16) / dw;
	yinc2 = (gs.DISPLAY[1].h << 16) / dh;

	ypos2 = gs.DISPLAY[1].y<<16;

//	if (gs.PMODE.alp) alpha = gs.PMODE.alp;
	alpha = 0x80;

	for (y=0; y<dh; y++) {
		pscr = (u16 *)(scr + y * sp);
		xpos1 = gs.DISPLAY[0].x<<16;
		xpos2 = gs.DISPLAY[1].x<<16;
		for(x=0; x<dw; x++) {
			c1 = _readPixel1(xpos1>>16, ypos1>>16, gs.DISPFB[0].fbp, gs.DISPFB[0].fbw);
			r1 = (c1 >> 16) & 0xff;
			g1 = (c1 >>  8) & 0xff;
			b1 = (c1      ) & 0xff;
			c2 = _readPixel2(xpos2>>16, ypos2>>16, gs.DISPFB[1].fbp, gs.DISPFB[1].fbw);
			r2 = (c2 >> 16) & 0xff;
			g2 = (c2 >>  8) & 0xff;
			b2 = (c2      ) & 0xff;

			r1 = ((r1 * alpha) + (r2 * (0xff - alpha))) / alpha;
			g1 = ((g1 * alpha) + (g2 * (0xff - alpha))) / alpha;
			b1 = ((b1 * alpha) + (b2 * (0xff - alpha))) / alpha;
			if (r1 > 0xff) r1 = 0xff;
			if (g1 > 0xff) g1 = 0xff;
			if (b1 > 0xff) b1 = 0xff;

			*pscr++= ((b1 >> 3) << 11) | /* 0xf800 */
					 ((g1 >> 2) <<  5) | /* 0x07e0 */
					 ((r1 >> 3)      );  /* 0x001f */
			xpos1+= xinc1;
			xpos2+= xinc2;
		}
		ypos1+= yinc1;
		ypos2+= yinc2;
	}
}

void ScrBlitMerge32(u8 *scr, int dw, int dh, int sp) {
	u32 (*_readPixel1)(int x, int y, u32 bp, u32 bw);
	u32 (*_readPixel2)(int x, int y, u32 bp, u32 bw);
	u8 *pscr;
	int x, y;
	int xpos1, xinc1;
	int ypos1, yinc1;
	int xpos2, xinc2;
	int ypos2, yinc2;
	u32 alpha;
	u32 c1, c2;
	u32 r1,g1,b1;
	u32 r2,g2,b2;

	switch (gs.DISPFB[0].psm) {
		case 0x2: _readPixel1 = readPixel16to32; break;
		case 0xA: _readPixel1 = readPixel16Sto32; break;
		default:  _readPixel1 = readPixel32; break;
	}
	switch (gs.DISPFB[1].psm) {
		case 0x2: _readPixel2 = readPixel16to32; break;
		case 0xA: _readPixel2 = readPixel16Sto32; break;
		default:  _readPixel2 = readPixel32; break;
	}

	xinc1 = (gs.DISPLAY[0].w << 16) / dw;
	yinc1 = (gs.DISPLAY[0].h << 16) / dh;

	ypos1 = gs.DISPLAY[0].y<<16;

	xinc2 = (gs.DISPLAY[1].w << 16) / dw;
	yinc2 = (gs.DISPLAY[1].h << 16) / dh;

	ypos2 = gs.DISPLAY[1].y<<16;

//	if (gs.PMODE.alp) alpha = gs.PMODE.alp;
	alpha = 0x80;

	for (y=0; y<dh; y++) {
		pscr = (u8 *)(scr + y * sp);
		xpos1 = gs.DISPLAY[0].x<<16;
		xpos2 = gs.DISPLAY[1].x<<16;
		for(x=0; x<dw; x++) {
			c1 = _readPixel1(xpos1>>16, ypos1>>16, gs.DISPFB[0].fbp, gs.DISPFB[0].fbw);
			r1 = (c1 >> 16) & 0xff;
			g1 = (c1 >>  8) & 0xff;
			b1 = (c1      ) & 0xff;
			c2 = _readPixel2(xpos2>>16, ypos2>>16, gs.DISPFB[1].fbp, gs.DISPFB[1].fbw);
			r2 = (c2 >> 16) & 0xff;
			g2 = (c2 >>  8) & 0xff;
			b2 = (c2      ) & 0xff;

			r1 = ((r1 * alpha) + (r2 * (0xff - alpha))) / alpha;
			g1 = ((g1 * alpha) + (g2 * (0xff - alpha))) / alpha;
			b1 = ((b1 * alpha) + (b2 * (0xff - alpha))) / alpha;
			if (r1 > 0xff) r1 = 0xff;
			if (g1 > 0xff) g1 = 0xff;
			if (b1 > 0xff) b1 = 0xff;

			pscr[0] = r1;
			pscr[1] = g1;
			pscr[2] = b1;
			pscr+=4;
			xpos1+= xinc1;
			xpos2+= xinc2;
		}
		ypos1+= yinc1;
		ypos2+= yinc2;
	}
}


void ScrBlit(SDL_Surface *dst, int y) {
	Rect gsScr;

	if (showfullvram) {
		if (gs.PMODE.en[0]) {
			gsScr.x = 0;
			gsScr.y = 0;
			gsScr.w = gs.DISPFB[0].fbw;
			gsScr.h = gs.DISPFB[0].fbh;

			_ScrBlit((u8*)dst->pixels + y * dst->pitch, dst->w, dst->h-y, dst->pitch, &gsScr, gs.DISPFB[0].psm, 0, gs.DISPFB[0].fbw);
		} else
		if (gs.PMODE.en[1]) {
			gsScr.x = 0;
			gsScr.y = 0;
			gsScr.w = gs.DISPFB[1].fbw;
			gsScr.h = gs.DISPFB[1].fbh;

			_ScrBlit((u8*)dst->pixels + y * dst->pitch, dst->w, dst->h-y, dst->pitch, &gsScr, gs.DISPFB[1].psm, 0, gs.DISPFB[1].fbw);
		}
		return;
	}

	if (gs.PMODE.en[0] && gs.PMODE.en[1]) {
		_ScrBlitMerge((u8*)dst->pixels + y * dst->pitch, dst->w, dst->h-y, dst->pitch);
		return;
	}

	if (gs.PMODE.en[0]) {
		gsScr.x = gs.DISPLAY[0].x;
		gsScr.y = gs.DISPLAY[0].y;
		gsScr.w = gs.DISPLAY[0].w;
		gsScr.h = gs.DISPLAY[0].h;
		_ScrBlit((u8*)dst->pixels + y * dst->pitch, dst->w, dst->h-y, dst->pitch, &gsScr, gs.DISPFB[0].psm, gs.DISPFB[0].fbp, gs.DISPFB[0].fbw);
	}

	if (gs.PMODE.en[1]) {
		gsScr.x = gs.DISPLAY[1].x;
		gsScr.y = gs.DISPLAY[1].y;
		gsScr.w = gs.DISPLAY[1].w;
		gsScr.h = gs.DISPLAY[1].h;
		_ScrBlit((u8*)dst->pixels + y * dst->pitch, dst->w, dst->h-y, dst->pitch, &gsScr, gs.DISPFB[1].psm, gs.DISPFB[1].fbp, gs.DISPFB[1].fbw);
	}
}


void SETScrBlit(int bpp) {
	switch (bpp) {
		case 15:
			_ScrBlit = ScrBlit15;
			_ScrBlitMerge = ScrBlitMerge15;
			break;
		case 16:
			_ScrBlit = ScrBlit16;
			_ScrBlitMerge = ScrBlitMerge16;
			break;
		case 32:
			_ScrBlit = ScrBlit32;
			_ScrBlitMerge = ScrBlitMerge32;
			break;
	}
}
