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

#include <math.h>

#include "GS.h"
#include "Soft.h"
#include "Texts.h"
#include "Color.h"
#include "Mem.h"

/* blend 2 colours together and return the result */
u32 blender(const u8 *A, const u8 *B, const int C, const u8 *D) {
	int i;
	u32 res = 0;
	int temp;

	for (i = 0; i < 3; i++) {
		temp = ((((int) A[i] -  (int) B[i]) * C) >> 7) + (int)D[i];
		if (gs.colclamp) {
			if (temp < 0x00) temp = 0x00; else
			if (temp > 0xFF) temp = 0xFF;
		} else {
			temp&= 0xFF;
		}

		res |= temp << (i << 3);
	}

	return res;
}

int scissorTest(int x, int y) {
	if (x < scissor->x0 || x > scissor->x1) return -1;
	if (y < scissor->y0 || y > scissor->y1) return -1;

	return 0;
}

int dateTest32(int x, int y) {
	if (test->date) { // Destination Alpha Test
		u32 pixel = readPixel32(x, y, gsfb->fbp, gsfb->fbw);
		if (test->datm) {
			if ((pixel >> 31) == 0) return -1;
		} else {
			if ((pixel >> 31) == 1) return -1;
		}
	}

	return 0;
}

int dateTest16(int x, int y) {
	if (test->date) { // Destination Alpha Test
		u16 pixel = readPixel16(x, y, gsfb->fbp, gsfb->fbw);
		if (test->datm) {
			if ((pixel >> 15) == 0) return -1;
		} else {
			if ((pixel >> 15) == 1) return -1;
		}
	}

	return 0;
}

int dateTest16S(int x, int y) {
	if (test->date) { // Destination Alpha Test
		u16 pixel = readPixel16S(x, y, gsfb->fbp, gsfb->fbw);
		if (test->datm) {
			if ((pixel >> 15) == 0) return -1;
		} else {
			if ((pixel >> 15) == 1) return -1;
		}
	}

	return 0;
}

int dateTest32Z(int x, int y) {
	if (test->date) { // Destination Alpha Test
		u32 pixel = readPixel32Z(x, y, gsfb->fbp, gsfb->fbw);
		if (test->datm) {
			if ((pixel >> 31) == 0) return -1;
		} else {
			if ((pixel >> 31) == 1) return -1;
		}
	}

	return 0;
}

int dateTest16Z(int x, int y) {
	if (test->date) { // Destination Alpha Test
		u16 pixel = readPixel16Z(x, y, gsfb->fbp, gsfb->fbw);
		if (test->datm) {
			if ((pixel >> 15) == 0) return -1;
		} else {
			if ((pixel >> 15) == 1) return -1;
		}
	}

	return 0;
}

int dateTest16SZ(int x, int y) {
	if (test->date) { // Destination Alpha Test
		u16 pixel = readPixel16SZ(x, y, gsfb->fbp, gsfb->fbw);
		if (test->datm) {
			if ((pixel >> 15) == 0) return -1;
		} else {
			if ((pixel >> 15) == 1) return -1;
		}
	}

	return 0;
}

int depthTest(int x, int y, u32 z) {
//GS_LOG("depthTest %dx%d %x\n", x, y, z);
	switch (test->ztst) {
		case ZTST_NEVER:
			return -1;

		case ZTST_ALWAYS:
			break;

		case ZTST_GEQUAL:
			if (z <  readPixelZ(x, y)) return -1;
			break;

		case ZTST_GREATER: 
			if (z <= readPixelZ(x, y)) return -1;
			break;
	}

	return 0;
}

int _alphaTest(u32 p) {
	switch (test->atst) {
		case ATST_NEVER:
			return -1;
		case ATST_ALWAYS:
			break;
		case ATST_LESS:
			if ((p >> 24) <  test->aref) break;
			else return -1;
		case ATST_LEQUAL:
			if ((p >> 24) <= test->aref) break;
			else return -1;
		case ATST_EQUAL:
			if ((p >> 24) == test->aref) break;
			else return -1;
		case ATST_GEQUAL:
			if ((p >> 24) >= test->aref) break;
			else return -1;
		case ATST_GREATER:
			if ((p >> 24) >  test->aref) break;
			else return -1;
		case ATST_NOTEQUAL:
			if ((p >> 24) != test->aref) break;
			else return -1;
	}

	return 0;
}

int alphaTest(u32 p) {
	if (test->ate == 0) return 0;

	if (_alphaTest(p) == -1) {
		switch (test->afail) {
			case AFAIL_KEEP:
				return -1;
			case AFAIL_FB_ONLY:
				return 1;
			case AFAIL_ZB_ONLY:
				return 2;
			case AFAIL_RGB_ONLY:
				return 3;
		}
	}

	return 0;
}

u32 fog(u32 p) {
/*	if (prim->fge) { // Fogging
		u8 r, g, b, a;

		r = ((F * (p >> 16)) >> 8) + 
			(((0xff - F) * fogcolr) >> 8);
		p = (r << 16) | (g << 8) | b
	}*/

	return p;
}

u32 (*procPixelABE) (int, int, u32);

u32 _procPixelABE(u32 Cs, u32 Cd) {
	u32 A, B, C, D, c;

	switch (alpha->a) {
		case 0x00: A = Cs & 0xffffff; break;
		case 0x01: A = Cd & 0xffffff; break;
		default:   A = 0; break;
	}

	switch (alpha->b) {
		case 0x00: B = Cs & 0xffffff; break;
		case 0x01: B = Cd & 0xffffff; break;
		default:   B = 0; break;
	}

	switch (alpha->c) {
		case 0x00: C = Cs >> 24; break;
		case 0x01: C = Cd >> 24; break;
		case 0x02: C = alpha->fix; break;
		default:   C = 0; break;
	}

	switch (alpha->d) {
		case 0x00: D = Cs & 0xffffff; break;
		case 0x01: D = Cd & 0xffffff; break;
		default:   D = 0; break;
	}
	c = blender((u8 *) &A, (u8 *) &B, C, (u8 *) &D);
	c|= fba->fba << 31;
//	printf("_procPixelABE (%d): %dx%d: %x, %x, %x, %x: %x (%d)\n", gs.pabe, x, y, A, B, C, D, c, alpha->c);

	return c;
}

u32 _procPixelABE32(int x, int y, u32 p) {
	if (gs.pabe) {
		if ((p >> 31) == 0) { // MSB = 0*/
			return p;
		}
	}

	return _procPixelABE(p, readPixel32(x, y, gsfb->fbp, gsfb->fbw));
}

u32 _procPixelABE24(int x, int y, u32 p) {
	if (gs.pabe) {
		if ((p >> 31) == 0) { // MSB = 0*/
			return p;
		}
	}

	return _procPixelABE(p, readPixel24(x, y, gsfb->fbp, gsfb->fbw));
}

u32 _procPixelABE16(int x, int y, u32 p) {
	if (gs.pabe) {
		if ((p >> 31) == 0) { // MSB = 0*/
			return p;
		}
	}

	return _procPixelABE(p, readPixel16to32(x, y, gsfb->fbp, gsfb->fbw));
}

u32 _procPixelABE16S(int x, int y, u32 p) {
	if (gs.pabe) {
		if ((p >> 31) == 0) { // MSB = 0*/
			return p;
		}
	}

	return _procPixelABE(p, readPixel16Sto32(x, y, gsfb->fbp, gsfb->fbw));
}

u32 _procPixelABE32Z(int x, int y, u32 p) {
	if (gs.pabe) {
		if ((p >> 31) == 0) { // MSB = 0*/
			return p;
		}
	}

	return _procPixelABE(p, readPixel32Z(x, y, gsfb->fbp, gsfb->fbw));
}

u32 _procPixelABE24Z(int x, int y, u32 p) {
	if (gs.pabe) {
		if ((p >> 31) == 0) { // MSB = 0*/
			return p;
		}
	}

	return _procPixelABE(p, readPixel24Z(x, y, gsfb->fbp, gsfb->fbw));
}

u32 _procPixelABE16Z(int x, int y, u32 p) {
	if (gs.pabe) {
		if ((p >> 31) == 0) { // MSB = 0*/
			return p;
		}
	}

	return _procPixelABE(p, readPixel16Zto32(x, y, gsfb->fbp, gsfb->fbw));
}

u32 _procPixelABE16SZ(int x, int y, u32 p) {
	if (gs.pabe) {
		if ((p >> 31) == 0) { // MSB = 0*/
			return p;
		}
	}

	return _procPixelABE(p, readPixel16SZto32(x, y, gsfb->fbp, gsfb->fbw));
}


void _drawPixelAA1(int x, int y, u32 p) {
}

//////////////////////////////////
// drawPixel

void _drawPixel32(int x, int y, u32 p) {
	int ret;

	if (scissorTest(x, y)) return;
	if (dateTest32(x, y)) return;
	ret = alphaTest(p);
	if (ret == -1) return;

	if (prim->abe) { // alpha blending
		p = procPixelABE(x, y, p);
	}

	if (gsfb->fbm) {
		p = (p & (~gsfb->fbm)) | (readPixel32(x, y, gsfb->fbp, gsfb->fbw) & gsfb->fbm);
	}

	if (ret == 0 || ret == 1) {
		writePixel32(x, y, p, gsfb->fbp, gsfb->fbw);
	} else
	if (ret == 3) {
		writePixel24(x, y, p, gsfb->fbp, gsfb->fbw);
	}
}

void _drawPixel24(int x, int y, u32 p) {
	int ret;

	if (scissorTest(x, y)) return;
	ret = alphaTest(p);
	if (ret == -1) return;

	if (prim->abe) { // alpha blending
		p = procPixelABE(x, y, p);
	}

	if (gsfb->fbm) {
		p = (p & (~gsfb->fbm)) | (readPixel24(x, y, gsfb->fbp, gsfb->fbw) & gsfb->fbm);
	}
	writePixel24(x, y, p, gsfb->fbp, gsfb->fbw);
}

void _drawPixel16(int x, int y, u32 p) {
	int ret;

	if (scissorTest(x, y)) return;
	if (dateTest16(x, y)) return;
	ret = alphaTest(p);
	if (ret == -1) return;

	if (prim->abe) { // alpha blending
		p = procPixelABE(x, y, p);
	}

	if (ret == 0 || ret == 1) {
		if (gsfb->fbm) {
			p = (RGBA32to16(p) & (~gsfb->fbm)) | (readPixel16(x, y, gsfb->fbp, gsfb->fbw) & gsfb->fbm);
		} else {
			p = RGBA32to16(p);
		}
		writePixel16(x, y, p, gsfb->fbp, gsfb->fbw);
	} else
	if (ret == 3) {
		if (gsfb->fbm) {
			u32 fbm = gsfb->fbm | 0x8000;
			p = (RGBA32to16(p) & (~fbm)) | (readPixel16(x, y, gsfb->fbp, gsfb->fbw) & fbm);
		} else {
			p = (RGBA32to16(p) & ~0x8000) | (readPixel16(x, y, gsfb->fbp, gsfb->fbw) & 0x8000);
		}
		writePixel16(x, y, RGBA32to16(p), gsfb->fbp, gsfb->fbw);
	}
}

void _drawPixel16S(int x, int y, u32 p) {
	int ret;

	if (scissorTest(x, y)) return;
	if (dateTest16S(x, y)) return;
	ret = alphaTest(p);
	if (ret == -1) return;

	if (prim->abe) { // alpha blending
		p = procPixelABE(x, y, p);
	}

	if (ret == 0 || ret == 1) {
		if (gsfb->fbm) {
			p = (RGBA32to16(p) & (~gsfb->fbm)) | (readPixel16S(x, y, gsfb->fbp, gsfb->fbw) & gsfb->fbm);
		} else {
			p = RGBA32to16(p);
		}
		writePixel16S(x, y, p, gsfb->fbp, gsfb->fbw);
	} else
	if (ret == 3) {
		if (gsfb->fbm) {
			u32 fbm = gsfb->fbm | 0x8000;
			p = (RGBA32to16(p) & (~fbm)) | (readPixel16S(x, y, gsfb->fbp, gsfb->fbw) & fbm);
		} else {
			p = (RGBA32to16(p) & ~0x8000) | (readPixel16S(x, y, gsfb->fbp, gsfb->fbw) & 0x8000);
		}
		writePixel16S(x, y, RGBA32to16(p), gsfb->fbp, gsfb->fbw);
	}
}

void _drawPixel32Z(int x, int y, u32 p) {
	int ret;

	if (scissorTest(x, y)) return;
	if (dateTest32Z(x, y)) return;
	ret = alphaTest(p);
	if (ret == -1) return;

	if (prim->abe) { // alpha blending
		p = procPixelABE(x, y, p);
	}

	if (gsfb->fbm) {
		p = (p & (~gsfb->fbm)) | (readPixel32Z(x, y, gsfb->fbp, gsfb->fbw) & gsfb->fbm);
	}

	if (ret == 0 || ret == 1) {
		writePixel32Z(x, y, p, gsfb->fbp, gsfb->fbw);
	} else
	if (ret == 3) {
		writePixel24Z(x, y, p, gsfb->fbp, gsfb->fbw);
	}
}

void _drawPixel24Z(int x, int y, u32 p) {
	int ret;

	if (scissorTest(x, y)) return;
	ret = alphaTest(p);
	if (ret == -1) return;

	if (prim->abe) { // alpha blending
		p = procPixelABE(x, y, p);
	}

	if (gsfb->fbm) {
		p = (p & (~gsfb->fbm)) | (readPixel24Z(x, y, gsfb->fbp, gsfb->fbw) & gsfb->fbm);
	}
	writePixel24Z(x, y, p, gsfb->fbp, gsfb->fbw);
}

void _drawPixel16Z(int x, int y, u32 p) {
	int ret;

	if (scissorTest(x, y)) return;
	if (dateTest16Z(x, y)) return;
	ret = alphaTest(p);
	if (ret == -1) return;

	if (prim->abe) { // alpha blending
		p = procPixelABE(x, y, p);
	}

	if (ret == 0 || ret == 1) {
		if (gsfb->fbm) {
			p = (RGBA32to16(p) & (~gsfb->fbm)) | (readPixel16Z(x, y, gsfb->fbp, gsfb->fbw) & gsfb->fbm);
		} else {
			p = RGBA32to16(p);
		}
		writePixel16(x, y, p, gsfb->fbp, gsfb->fbw);
	} else
	if (ret == 3) {
		if (gsfb->fbm) {
			u32 fbm = gsfb->fbm | 0x8000;
			p = (RGBA32to16(p) & (~fbm)) | (readPixel16Z(x, y, gsfb->fbp, gsfb->fbw) & fbm);
		} else {
			p = (RGBA32to16(p) & ~0x8000) | (readPixel16Z(x, y, gsfb->fbp, gsfb->fbw) & 0x8000);
		}
		writePixel16Z(x, y, RGBA32to16(p), gsfb->fbp, gsfb->fbw);
	}
}

void _drawPixel16SZ(int x, int y, u32 p) {
	int ret;

	if (scissorTest(x, y)) return;
	if (dateTest16SZ(x, y)) return;
	ret = alphaTest(p);
	if (ret == -1) return;

	if (prim->abe) { // alpha blending
		p = procPixelABE(x, y, p);
	}

	if (ret == 0 || ret == 1) {
		if (gsfb->fbm) {
			p = (RGBA32to16(p) & (~gsfb->fbm)) | (readPixel16SZ(x, y, gsfb->fbp, gsfb->fbw) & gsfb->fbm);
		} else {
			p = RGBA32to16(p);
		}
		writePixel16SZ(x, y, p, gsfb->fbp, gsfb->fbw);
	} else
	if (ret == 3) {
		if (gsfb->fbm) {
			u32 fbm = gsfb->fbm | 0x8000;
			p = (RGBA32to16(p) & (~fbm)) | (readPixel16SZ(x, y, gsfb->fbp, gsfb->fbw) & fbm);
		} else {
			p = (RGBA32to16(p) & ~0x8000) | (readPixel16SZ(x, y, gsfb->fbp, gsfb->fbw) & 0x8000);
		}
		writePixel16SZ(x, y, RGBA32to16(p), gsfb->fbp, gsfb->fbw);
	}
}

//////////////////////////////////
// drawPixelF


void SETprocPixelABE() {
	procPixelABE = 0;
#if 0
	if (alpha->a == 0) {
		if (alpha->b == 0) {
		} else
		if (alpha->b == 1) {
		} else {
			if (alpha->c == 0) {
				if (alpha->d == 0) {
				} else
				if (alpha->d == 1) {
					switch (gsfb->psm) {
						case 0x0:
							if (gs.pabe) {
								if (gs.colclamp) {
									procPixelABE = _procPixelABE32_A0B2C0D1_PABE_CC; 
								} else {
									procPixelABE = _procPixelABE32_A0B2C0D1_PABE; 
								}
							} else {
								if (gs.colclamp) {
									procPixelABE = _procPixelABE32_A0B2C0D1_CC; 
								} else {
									procPixelABE = _procPixelABE32_A0B2C0D1; 
								}
							}
							break;
						case 0x2: procPixelABE = _procPixelABE16_A0B2C0D1; break;
					}
				}
			} else
			if (alpha->c == 1) {
			} else
			if (alpha->c == 2) {
			}
		}
	} else
	if (alpha->a == 1) {
		if (alpha->b == 0) {
		} else
		if (alpha->b == 1) {
		} else {
			if (alpha->c == 0) {
				if (alpha->d == 0) {
				} else
				if (alpha->d == 1) {
				}
			} else
			if (alpha->c == 1) {
			} else
			if (alpha->c == 2) {
				if (alpha->d == 0) {
/*					switch (gsfb->psm) {
						case 0x0:
							if (gs.pabe) {
								if (gs.colclamp) {
									procPixelABE = _procPixelABE32_A1B2C2D0_PABE_CC; 
								} else {
									procPixelABE = _procPixelABE32_A1B2C2D0_PABE; 
								}
							} else {
								if (gs.colclamp) {
									procPixelABE = _procPixelABE32_A1B2C2D0_CC; 
								} else {
									procPixelABE = _procPixelABE32_A1B2C2D0; 
								}
							}
							break;
					}*/
				} else
				if (alpha->d == 1) {
				}
			}
		}
	} else {
	}
#endif
	if (procPixelABE == 0) {
//		printf("ABE default: %d,%d,%d,%d\n", alpha->a, alpha->b, alpha->c, alpha->d);
		switch (gsfb->psm) {
			case 0x0: procPixelABE = _procPixelABE32; break;
			case 0x1: procPixelABE = _procPixelABE24; break;
			case 0x2: procPixelABE = _procPixelABE16; break;
			case 0xa: procPixelABE = _procPixelABE16S; break;
			case 0x30:procPixelABE = _procPixelABE32Z; break;
			case 0x31:procPixelABE = _procPixelABE24Z; break;
			case 0x32:procPixelABE = _procPixelABE16Z; break;
			case 0x3a:procPixelABE = _procPixelABE16SZ; break;
			default:
				if (gsfb->psm) printf("SETprocPixelABE psm == %d!!\n", gsfb->psm);
				drawPixel = _drawPixel32;
				break;
		}
	}
}

void SETdrawPixel() {
	drawPixel = 0;
	if (drawPixel == 0) {
		switch (gsfb->psm) {
			case 0x1: drawPixel = _drawPixel24;  break;
			case 0x2: drawPixel = _drawPixel16;  break;
			case 0xa: drawPixel = _drawPixel16S; break;
			case 0x30:drawPixel = _drawPixel32Z; break;
			case 0x31:drawPixel = _drawPixel24Z; break;
			case 0x32:drawPixel = _drawPixel16Z; break;
			case 0x3a:drawPixel = _drawPixel16SZ; break;
			default: // PSMT32
				if (gsfb->psm) printf("SETdrawPixel psm == %d!!\n", gsfb->psm);
				drawPixel = _drawPixel32;
				break;
		}
	}

	if (prim->abe) { // alpha blending
		SETprocPixelABE();
	}
}

void SETdrawPixelF() {
/*	drawPixelF = 0;
	if (drawPixelF == 0) {
		switch (gsfb->psm) {
			case 0x1: drawPixelF = _drawPixel24F;  break;
			case 0x2: drawPixelF = _drawPixel16F;  break;
			case 0xa: drawPixelF = _drawPixel16SF; break;
			default: // PSMT32
				if (gsfb->psm) printf("SETdrawPixel psm == %d!!\n", gsfb->psm);
				drawPixelF = _drawPixel32F;
				break;
		}
	}

	if (prim->abe) { // alpha blending
		SETprocPixelABE();
	}*/
}

//////////////////////////////////
// drawPixelZ


// zmsk = 0
void _drawPixel32_Z(int x, int y, u32 p, u32 z) {
	int ret;

	if (scissorTest(x, y)) return;
	if (depthTest(x, y, z)) return;

	ret = alphaTest(p);
	switch (ret) {
		case 0: // write p and z
			writePixel32Z(x, y, z, zbuf->zbp, gsfb->fbw);
			drawPixel(x, y, p);
			break;

		case 1: // write p only
			drawPixel(x, y, p);
			break;

		case 2: // write z only
			writePixel32Z(x, y, z, zbuf->zbp, gsfb->fbw);
			break;

		case 3: // write rgb only
			drawPixel(x, y, p);
			break;
	}
}

// zmsk = 1
void _drawPixel32_Zmsk(int x, int y, u32 p, u32 z) {
	int ret;

	if (scissorTest(x, y)) return;
	if (depthTest(x, y, z)) return;

	ret = alphaTest(p);
	switch (ret) {
		case 0: // write p and z
			drawPixel(x, y, p);
			break;

		case 1: // write p only
			drawPixel(x, y, p);
			break;

		case 3: // write rgb only
			drawPixel(x, y, p);
			break;
	}
}

// zmsk = 0
void _drawPixel24_Z(int x, int y, u32 p, u32 z) {
	int ret;

	if (scissorTest(x, y)) return;
	if (depthTest(x, y, z)) return;

	ret = alphaTest(p);
	switch (ret) {
		case 0: // write p and z
			writePixel24Z(x, y, z, zbuf->zbp, gsfb->fbw);
			drawPixel(x, y, p);
			break;

		case 1: // write p only
			drawPixel(x, y, p);
			break;

		case 2: // write z only
			writePixel24Z(x, y, z, zbuf->zbp, gsfb->fbw);
			break;

		case 3: // write rgb only
			drawPixel(x, y, p);
			break;
	}
}

// zmsk = 1
void _drawPixel24_Zmsk(int x, int y, u32 p, u32 z) {
	int ret;

	if (scissorTest(x, y)) return;
	if (depthTest(x, y, z)) return;

	ret = alphaTest(p);
	switch (ret) {
		case 0: // write p and z
			drawPixel(x, y, p);
			break;

		case 1: // write p only
			drawPixel(x, y, p);
			break;

		case 3: // write rgb only
			drawPixel(x, y, p);
			break;
	}
}

// zmsk = 0
void _drawPixel16_Z(int x, int y, u32 p, u32 z) {
	int ret;

	if (scissorTest(x, y)) return;
	if (depthTest(x, y, z)) return;

	ret = alphaTest(p);
	switch (ret) {
		case 0: // write p and z
			writePixel16Z(x, y, (u16)z, zbuf->zbp, gsfb->fbw);
			drawPixel(x, y, p);
			break;

		case 1: // write p only
			drawPixel(x, y, p);
			break;

		case 2: // write z only
			writePixel16Z(x, y, (u16)z, zbuf->zbp, gsfb->fbw);
			break;

		case 3: // write rgb only
			drawPixel(x, y, p);
			break;
	}
}

// zmsk = 1
void _drawPixel16_Zmsk(int x, int y, u32 p, u32 z) {
	int ret;

	if (scissorTest(x, y)) return;
	if (depthTest(x, y, z)) return;

	ret = alphaTest(p);
	switch (ret) {
		case 0: // write p and z
			drawPixel(x, y, p);
			break;

		case 1: // write p only
			drawPixel(x, y, p);
			break;

		case 3: // write rgb only
			drawPixel(x, y, p);
			break;
	}
}

// zmsk = 0
void _drawPixel16S_Z(int x, int y, u32 p, u32 z) {
	int ret;

	if (scissorTest(x, y)) return;
	if (depthTest(x, y, z)) return;

	ret = alphaTest(p);
	switch (ret) {
		case 0: // write p and z
			writePixel16SZ(x, y, (u16)z, zbuf->zbp, gsfb->fbw);
			drawPixel(x, y, p);
			break;

		case 1: // write p only
			drawPixel(x, y, p);
			break;

		case 2: // write z only
			writePixel16SZ(x, y, (u16)z, zbuf->zbp, gsfb->fbw);
			break;

		case 3: // write rgb only
			drawPixel(x, y, p);
			break;
	}
}

// zmsk = 1
void _drawPixel16S_Zmsk(int x, int y, u32 p, u32 z) {
	int ret;

	if (scissorTest(x, y)) return;
	if (depthTest(x, y, z)) return;

	ret = alphaTest(p);
	switch (ret) {
		case 0: // write p and z
			drawPixel(x, y, p);
			break;

		case 1: // write p only
			drawPixel(x, y, p);
			break;

		case 3: // write rgb only
			drawPixel(x, y, p);
			break;
	}
}


//////////////////////////////////
// drawPixelZF


// zmsk = 0
void _drawPixel32_ZF(int x, int y, u32 p, u32 z, u32 f) {
	int ret;

	if (scissorTest(x, y)) return;
	if (depthTest(x, y, z)) return;

	ret = alphaTest(p);
	switch (ret) {
		case 0: // write p and z
			writePixel32Z(x, y, z, zbuf->zbp, gsfb->fbw);
			drawPixelF(x, y, p, f);
			break;

		case 1: // write p only
			drawPixelF(x, y, p, f);
			break;

		case 2: // write z only
			writePixel32Z(x, y, z, zbuf->zbp, gsfb->fbw);
			break;

		case 3: // write rgb only
			drawPixelF(x, y, p, f);
			break;
	}
}

// zmsk = 1
void _drawPixel32_ZFmsk(int x, int y, u32 p, u32 z, u32 f) {
	int ret;

	if (scissorTest(x, y)) return;
	if (depthTest(x, y, z)) return;

	ret = alphaTest(p);
	switch (ret) {
		case 0: // write p and z
			drawPixelF(x, y, p, f);
			break;

		case 1: // write p only
			drawPixelF(x, y, p, f);
			break;

		case 3: // write rgb only
			drawPixelF(x, y, p, f);
			break;
	}
}

// zmsk = 0
void _drawPixel24_ZF(int x, int y, u32 p, u32 z, u32 f) {
	int ret;

	if (scissorTest(x, y)) return;
	if (depthTest(x, y, z)) return;

	ret = alphaTest(p);
	switch (ret) {
		case 0: // write p and z
			writePixel24Z(x, y, z, zbuf->zbp, gsfb->fbw);
			drawPixelF(x, y, p, f);
			break;

		case 1: // write p only
			drawPixelF(x, y, p, f);
			break;

		case 2: // write z only
			writePixel24Z(x, y, z, zbuf->zbp, gsfb->fbw);
			break;

		case 3: // write rgb only
			drawPixelF(x, y, p, f);
			break;
	}
}

// zmsk = 1
void _drawPixel24_ZFmsk(int x, int y, u32 p, u32 z, u32 f) {
	int ret;

	if (scissorTest(x, y)) return;
	if (depthTest(x, y, z)) return;

	ret = alphaTest(p);
	switch (ret) {
		case 0: // write p and z
			drawPixelF(x, y, p, f);
			break;

		case 1: // write p only
			drawPixelF(x, y, p, f);
			break;

		case 3: // write rgb only
			drawPixelF(x, y, p, f);
			break;
	}
}

// zmsk = 0
void _drawPixel16_ZF(int x, int y, u32 p, u32 z, u32 f) {
	int ret;

	if (scissorTest(x, y)) return;
	if (depthTest(x, y, z)) return;

	ret = alphaTest(p);
	switch (ret) {
		case 0: // write p and z
			writePixel16Z(x, y, (u16)z, zbuf->zbp, gsfb->fbw);
			drawPixelF(x, y, p, f);
			break;

		case 1: // write p only
			drawPixelF(x, y, p, f);
			break;

		case 2: // write z only
			writePixel16Z(x, y, (u16)z, zbuf->zbp, gsfb->fbw);
			break;

		case 3: // write rgb only
			drawPixelF(x, y, p, f);
			break;
	}
}

// zmsk = 1
void _drawPixel16_ZFmsk(int x, int y, u32 p, u32 z, u32 f) {
	int ret;

	if (scissorTest(x, y)) return;
	if (depthTest(x, y, z)) return;

	ret = alphaTest(p);
	switch (ret) {
		case 0: // write p and z
			drawPixelF(x, y, p, f);
			break;

		case 1: // write p only
			drawPixelF(x, y, p, f);
			break;

		case 3: // write rgb only
			drawPixelF(x, y, p, f);
			break;
	}
}

// zmsk = 0
void _drawPixel16S_FZ(int x, int y, u32 p, u32 z, u32 f) {
	int ret;

	if (scissorTest(x, y)) return;
	if (depthTest(x, y, z)) return;

	ret = alphaTest(p);
	switch (ret) {
		case 0: // write p and z
			writePixel16SZ(x, y, (u16)z, zbuf->zbp, gsfb->fbw);
			drawPixelF(x, y, p, f);
			break;

		case 1: // write p only
			drawPixelF(x, y, p, f);
			break;

		case 2: // write z only
			writePixel16SZ(x, y, (u16)z, zbuf->zbp, gsfb->fbw);
			break;

		case 3: // write rgb only
			drawPixelF(x, y, p, f);
			break;
	}
}

// zmsk = 1
void _drawPixel16S_FZmsk(int x, int y, u32 p, u32 z, u32 f) {
	int ret;

	if (scissorTest(x, y)) return;
	if (depthTest(x, y, z)) return;

	ret = alphaTest(p);
	switch (ret) {
		case 0: // write p and z
			drawPixelF(x, y, p, f);
			break;

		case 1: // write p only
			drawPixelF(x, y, p, f);
			break;

		case 3: // write rgb only
			drawPixelF(x, y, p, f);
			break;
	}
}


u32  _readPixel32Z(int x, int y) {
	return readPixel32Z(x, y, zbuf->zbp, gsfb->fbw);
}

u32  _readPixel24Z(int x, int y) {
	return readPixel24Z(x, y, zbuf->zbp, gsfb->fbw);
}

u32  _readPixel16Z(int x, int y) {
	return readPixel16Z(x, y, zbuf->zbp, gsfb->fbw);
}

u32  _readPixel16SZ(int x, int y) {
	return readPixel16SZ(x, y, zbuf->zbp, gsfb->fbw);
}

void SETdrawPixelZ() {
	SETdrawPixel();
	switch (zbuf->psm) {
		case 0x1: // PSMZ24
			if (zbuf->zmsk == 0) {
				drawPixelZ = _drawPixel24_Z;
			} else {
				drawPixelZ = _drawPixel24_Zmsk;
			}
			readPixelZ = _readPixel24Z; break;
		case 0x2: // PSMZ16
			if (zbuf->zmsk == 0) {
				drawPixelZ = _drawPixel16_Z;
			} else {
				drawPixelZ = _drawPixel16_Zmsk;
			}
			readPixelZ = _readPixel16Z; break;
		case 0xa: // PSMZ16S
			if (zbuf->zmsk == 0) {
				drawPixelZ = _drawPixel16S_Z;
			} else {
				drawPixelZ = _drawPixel16S_Zmsk;
			}
			readPixelZ = _readPixel16SZ; break;
		default:
			if (zbuf->psm) printf("unknown PSMZ psm %x\n", zbuf->psm);
			if (zbuf->zmsk == 0) {
				drawPixelZ = _drawPixel32_Z;
			} else {
				drawPixelZ = _drawPixel32_Zmsk;
			}
			readPixelZ = _readPixel32Z; break;
	}
}

void SETdrawPixelZF() {
	SETdrawPixelF();
	switch (zbuf->psm) {
		case 0x1: // PSMZ24
			if (zbuf->zmsk == 0) {
				drawPixelZF = _drawPixel24_ZF;
			} else {
				drawPixelZF = _drawPixel24_ZFmsk;
			}
			readPixelZ = _readPixel24Z; break;
		case 0x2: // PSMZ16
			if (zbuf->zmsk == 0) {
				drawPixelZF = _drawPixel16_ZF;
			} else {
				drawPixelZF = _drawPixel16_ZFmsk;
			}
			readPixelZ = _readPixel16Z; break;
		case 0xa: // PSMZ16S
			if (zbuf->zmsk == 0) {
				drawPixelZF = _drawPixel16S_FZ;
			} else {
				drawPixelZF = _drawPixel16S_FZmsk;
			}
			readPixelZ = _readPixel16SZ; break;
		default:
			if (zbuf->psm) printf("unknown PSMZ psm %x\n", zbuf->psm);
			if (zbuf->zmsk == 0) {
				drawPixelZF = _drawPixel32_ZF;
			} else {
				drawPixelZF = _drawPixel32_ZFmsk;
			}
			readPixelZ = _readPixel32Z; break;
	}
}

int af, rf, gf, bf; /* Colour of vertex */

void colSetCol(u32 crgba) {
	af = (crgba >> 24) & 0xFF;
	rf = (crgba >> 16) & 0xFF;
	gf = (crgba >>  8) & 0xFF;
	bf = (crgba      ) & 0xFF;
}

u32 colModulate(u32 texcol) {
	u32 at, rt, gt, bt; /* Colour of texture */

	rt = (texcol >> 16) & 0xFF;
	gt = (texcol >> 8) & 0xFF;
	bt = texcol & 0xFF; /* Extract colours from texture */

	rt = (rt * rf) >> 7;
	gt = (gt * gf) >> 7;
	bt = (bt * bf) >> 7;
	if(rt > 0xFF) rt = 0xFF;
	if(gt > 0xFF) gt = 0xFF;
	if(bt > 0xFF) bt = 0xFF;

	if (tex0->tcc) { // RGBA
		at = (texcol >> 24) & 0xFF;
		at = (at * af) >> 7; /* Multiply by colour value */
		if(at > 0xFF) at = 0xFF;
	} else { // RGB
		at = af;
	}

	return (at << 24) | (rt << 16) | (gt << 8) | bt;
}

u32 colHighlight(u32 texcol) {
	u32 at, rt, gt, bt; /* Colour of texture */

	rt = (texcol >> 16) & 0xFF;
	gt = (texcol >> 8) & 0xFF;
	bt = texcol & 0xFF; /* Extract colours from texture */

	rt = ((rt * rf) >> 7) + af;
	gt = ((gt * gf) >> 7) + af;
	bt = ((bt * bf) >> 7) + af;
	if(rt > 0xFF) rt = 0xFF;
	if(gt > 0xFF) gt = 0xFF;
	if(bt > 0xFF) bt = 0xFF;

	if (tex0->tcc) { // RGBA
		at = (texcol >> 24) & 0xFF;
		at = (at * af) >> 7; /* Multiply by colour value */
		if(at > 0xFF) at = 0xFF;
	} else { // RGB
		at = af;
	}

	return (at << 24) | (rt << 16) | (gt << 8) | bt;
}

u32 colHighlight2(u32 texcol) {
	u32 at, rt, gt, bt; /* Colour of texture */

	rt = (texcol >> 16) & 0xFF;
	gt = (texcol >> 8) & 0xFF;
	bt = texcol & 0xFF; /* Extract colours from texture */

	rt = ((rt * rf) >> 7) + af;
	gt = ((gt * gf) >> 7) + af;
	bt = ((bt * bf) >> 7) + af;
	if(rt > 0xFF) rt = 0xFF;
	if(gt > 0xFF) gt = 0xFF;
	if(bt > 0xFF) bt = 0xFF;

	if (tex0->tcc) { // RGBA
		at = (texcol >> 24) & 0xFF;
	} else { // RGB
		at = af;
	}

	return (at << 24) | (rt << 16) | (gt << 8) | bt;
}

