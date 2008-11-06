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

#undef ABS
#define ABS(a) (((a)<0) ? -(a) : (a))

__inline int shl10idiv(int x, int y) {
	s64 bi = x;
	bi <<= 10;
	return (int)(bi / y);
}

__inline s64 shl10idiv64(s64 x, s64 y) {
	s64 bi = x;
	bi <<= 10;
	return (s64)(bi / y);
}

//

void drawLineF(Vertex * v) {
	int dx = v[1].x - v[0].x;
	int dy = v[1].y - v[0].y;
	int ax = ABS(dx) << 1;
	int ay = ABS(dy) << 1;
	int sx = (dx >= 0) ? 1 : -1;
	int sy = (dy >= 0) ? 1 : -1;
	int x = v[0].x;
	int y = v[0].y;

#ifdef GS_LOG
	GS_LOG("drawLineF %dx%d - %dx%d rgba=%x\n",
		   v[0].x, v[0].y, v[1].x, v[1].y, gs.rgba);
#endif

	SETdrawPixel();

	if (ax > ay) {
		int d = ay - (ax >> 1);
		while (x != v[1].x) {
			drawPixel(x, y, gs.rgba);

			if (d > 0 || (d == 0 && sx == 1)) {
				y += sy;
				d -= ax;
			}
			x += sx;
			d += ay;
		}
	} else {
		int d = ax - (ay >> 1);
		while (y != v[1].y) {
			drawPixel(x, y, gs.rgba);

			if (d > 0 || (d == 0 && sy == 1)) {
				x += sx;
				d -= ay;
			}
			y += sy;
			d += ax;
		}
	}
}

void drawLineF_F(Vertex * v) {
	int dx = v[1].x - v[0].x;
	int dy = v[1].y - v[0].y;
	int ax = ABS(dx) << 1;
	int ay = ABS(dy) << 1;
	int sx = (dx >= 0) ? 1 : -1;
	int sy = (dy >= 0) ? 1 : -1;
	int x = v[0].x;
	int y = v[0].y;
	u32 f0, f1;
	int df;

#ifdef GS_LOG
	GS_LOG("drawLineF_F %dx%d - %dx%d rgba=%x\n",
		   v[0].x, v[0].y, v[1].x, v[1].y, gs.rgba);
#endif

	SETdrawPixelF();

	f0 = v[0].f << 16;
	f1 = v[1].f << 16;

	if (ax > ay) {
		int d = ay - (ax >> 1);
		df = (f1 - f0) / dx;
		while (x != v[1].x) {
			drawPixelF(x, y, gs.rgba, f0 >> 16);

			if (d > 0 || (d == 0 && sx == 1)) {
				y += sy; d -= ax;
			}
			x += sx; d += ay;
			f0+= df; 
		}
	} else {
		int d = ax - (ay >> 1);
		df = (f1 - f0) / dy;
		while (y != v[1].y) {
			drawPixelF(x, y, gs.rgba, f0 >> 16);

			if (d > 0 || (d == 0 && sy == 1)) {
				x += sx; d -= ay;
			}
			y += sy; d += ax;
			f0+= df; 
		}
	}
}

void drawLineF_Z(Vertex * v) {
	int dx = v[1].x - v[0].x;
	int dy = v[1].y - v[0].y;
	int ax = ABS(dx) << 1;
	int ay = ABS(dy) << 1;
	int sx = (dx >= 0) ? 1 : -1;
	int sy = (dy >= 0) ? 1 : -1;
	int x = v[0].x;
	int y = v[0].y;

#ifdef GS_LOG
	GS_LOG("drawLineF_Z %dx%d - %dx%d rgba=%x\n",
		   v[0].x, v[0].y, v[1].x, v[1].y, gs.rgba);
#endif

	SETdrawPixelZ();

	if (ax > ay) {
		int d = ay - (ax >> 1);
		while (x != v[1].x) {
			drawPixelZ(x, y, gs.rgba, v[1].z);

			if (d > 0 || (d == 0 && sx == 1)) {
				y += sy; d -= ax;
			}
			x += sx; d += ay;
		}
	} else {
		int d = ax - (ay >> 1);
		while (y != v[1].y) {
			drawPixelZ(x, y, gs.rgba, v[1].z);

			if (d > 0 || (d == 0 && sy == 1)) {
				x += sx; d -= ay;
			}
			y += sy; d += ax;
		}
	}
}

void drawLineG(Vertex * v) {
	int dx = v[1].x - v[0].x;
	int dy = v[1].y - v[0].y;
	int ax = ABS(dx) << 1;
	int ay = ABS(dy) << 1;
	int sx = (dx >= 0) ? 1 : -1;
	int sy = (dy >= 0) ? 1 : -1;
	int x = v[0].x;
	int y = v[0].y;
	u32 r0, g0, b0, a0;
	u32 r1, g1, b1, a1;
	int dr, dg, db, da;

#ifdef GS_LOG
	GS_LOG("drawLineG %dx%d %x - %dx%d %x\n",
		   v[0].x, v[0].y, v[0].rgba, v[1].x, v[1].y, v[1].rgba);
#endif

	SETdrawPixel();

	r0 = (v[0].rgba & 0x00ff0000);
	g0 = (v[0].rgba & 0x0000ff00) << 8;
	b0 = (v[0].rgba & 0x000000ff) << 16;
	a0 = (v[0].rgba & 0xff000000) >> 8;
	r1 = (v[1].rgba & 0x00ff0000);
	g1 = (v[1].rgba & 0x0000ff00) << 8;
	b1 = (v[1].rgba & 0x000000ff) << 16;
	a1 = (v[1].rgba & 0xff000000) >> 8;

	if (ax > ay) {
		int d = ay - (ax >> 1);

		if (dx == 0) return;
		dr = ((int)r1 - (int)r0) / dx;
		dg = ((int)g1 - (int)g0) / dx;
		db = ((int)b1 - (int)b0) / dx;
		da = ((int)a1 - (int)a0) / dx;

		while (x != v[1].x) {
			drawPixel(x, y, ( r0 & 0xff0000)        | 
							((g0 & 0xff0000) >> 8)  | 
							((b0 & 0xff0000) >> 16) | 
							((a0 & 0xff0000) << 8));

			if (d > 0 || (d == 0 && sx == 1)) {
				y += sy;
				d -= ax;
			}
			x += sx; d += ay;
			r0+= dr; g0+= dg; b0+= db; a0+= da;
		}
	} else {
		int d = ax - (ay >> 1);

		if (dy == 0) return;
		dr = ((int)r1 - (int)r0) / dy;
		dg = ((int)g1 - (int)g0) / dy;
		db = ((int)b1 - (int)b0) / dy;
		da = ((int)a1 - (int)a0) / dy;

		while (y != v[1].y) {
			drawPixel(x, y, ( r0 & 0xff0000)        | 
							((g0 & 0xff0000) >> 8)  | 
							((b0 & 0xff0000) >> 16) |
							((a0 & 0xff0000) << 8));

			if (d > 0 || (d == 0 && sy == 1)) {
				x += sx;
				d -= ay;
			}
			y += sy; d += ax;
			r0+= dr; g0+= dg; b0+= db; a0+= da;
		}
	}
}

void drawLineG_Z(Vertex * v) {
	int dx = v[1].x - v[0].x;
	int dy = v[1].y - v[0].y;
	int ax = ABS(dx) << 1;
	int ay = ABS(dy) << 1;
	int sx = (dx >= 0) ? 1 : -1;
	int sy = (dy >= 0) ? 1 : -1;
	int x = v[0].x;
	int y = v[0].y;
	u32 r0, g0, b0, a0;
	u32 r1, g1, b1, a1;
	int dr, dg, db, da;

#ifdef GS_LOG
	GS_LOG("drawLineG_Z %dx%d %x - %dx%d %x\n",
		   v[0].x, v[0].y, v[0].rgba, v[1].x, v[1].y, v[1].rgba);
#endif

	SETdrawPixelZ();

	r0 = (v[0].rgba & 0x00ff0000);
	g0 = (v[0].rgba & 0x0000ff00) << 8;
	b0 = (v[0].rgba & 0x000000ff) << 16;
	a0 = (v[0].rgba & 0xff000000) >> 8;
	r1 = (v[1].rgba & 0x00ff0000);
	g1 = (v[1].rgba & 0x0000ff00) << 8;
	b1 = (v[1].rgba & 0x000000ff) << 16;
	a1 = (v[1].rgba & 0xff000000) >> 8;

	if (ax > ay) {
		int d = ay - (ax >> 1);

		if (dx == 0) return;
		dr = ((int)r1 - (int)r0) / dx;
		dg = ((int)g1 - (int)g0) / dx;
		db = ((int)b1 - (int)b0) / dx;
		da = ((int)a1 - (int)a0) / dx;

		while (x != v[1].x) {
			drawPixelZ(x, y, ( r0 & 0xff0000)        | 
							 ((g0 & 0xff0000) >> 8)  | 
							 ((b0 & 0xff0000) >> 16) |
							 ((a0 & 0xff0000) << 8), v[1].z);

			if (d > 0 || (d == 0 && sx == 1)) {
				y += sy;
				d -= ax;
			}
			x += sx; d += ay;
			r0+= dr; g0+= dg; b0+= db;
		}
	} else {
		int d = ax - (ay >> 1);

		if (dy == 0) return;
		dr = ((int)r1 - (int)r0) / dy;
		dg = ((int)g1 - (int)g0) / dy;
		db = ((int)b1 - (int)b0) / dy;
		da = ((int)a1 - (int)a0) / dy;

		while (y != v[1].y) {
			drawPixelZ(x, y, ( r0 & 0xff0000)        | 
							 ((g0 & 0xff0000) >> 8)  | 
							 ((b0 & 0xff0000) >> 16) |
							 ((a0 & 0xff0000) << 8), v[1].z);

			if (d > 0 || (d == 0 && sy == 1)) {
				x += sx;
				d -= ay;
			}
			y += sy; d += ax;
			r0+= dr; g0+= dg; b0+= db;
		}
	}
}

////////////////////////////////////////////////////////////////////////

void drawSprite(Vertex * v) {
	int x, y;

#ifdef GS_LOG
	GS_LOG("drawSprite %dx%d - %dx%d %lx\n",
		   v[0].x, v[0].y, v[1].x, v[1].y, gs.rgba);
#endif

	SETdrawPixel();
	for (y = v[0].y; y < v[1].y; y++)
		for (x = v[0].x; x < v[1].x; x++)
			drawPixel(x, y, gs.rgba);
}

void drawSprite_F(Vertex * v) {
	int x, y;

#ifdef GS_LOG
	GS_LOG("drawSprite_F %dx%d - %dx%d %lx\n",
		   v[0].x, v[0].y, v[1].x, v[1].y, gs.rgba);
#endif

	SETdrawPixelF();
	for (y = v[0].y; y < v[1].y; y++)
		for (x = v[0].x; x < v[1].x; x++)
			drawPixelF(x, y, gs.rgba, v[1].f);
}

void drawSprite_Z(Vertex * v) {
	int x, y;

#ifdef GS_LOG
	GS_LOG("drawSprite_Z %dx%d %lx - %dx%d %lx\n",
		   v[0].x, v[0].y, v[0].rgba, v[1].x, v[1].y, v[1].rgba);
#endif

	SETdrawPixelZ();

	for (y = v[0].y; y < v[1].y; y++) {
		for (x = v[0].x; x < v[1].x; x++) {
			drawPixelZ(x, y, gs.rgba, v[1].z);
		}
	}
}

void drawSprite_ZF(Vertex * v) {
	int x, y;

#ifdef GS_LOG
	GS_LOG("drawSprite_ZF %dx%d %lx - %dx%d %lx\n",
		   v[0].x, v[0].y, v[0].rgba, v[1].x, v[1].y, v[1].rgba);
#endif

	SETdrawPixelZF();

	for (y = v[0].y; y < v[1].y; y++) {
		for (x = v[0].x; x < v[1].x; x++) {
			drawPixelZF(x, y, gs.rgba, v[1].z, v[1].f);
		}
	}
}

////////////////////////////////////////////////////////////////////////
// TEXTURED SPRITE
////////////////////////////////////////////////////////////////////////


#define _drawSpriteT(ftx) \
	SetTexture(); \
	SETdrawPixel(); \
 \
	if (vx[1].x == vx[0].x) uinc = 0; \
	else uinc = ((vx[1].u - vx[0].u) << 16) / (vx[1].x - vx[0].x); \
	if (vx[1].y == vx[0].y) vinc = 0; \
	else vinc = ((vx[1].v - vx[0].v) << 16) / (vx[1].y - vx[0].y); \
 \
	for (y = vx[0].y, vpos = vx[0].v << 16; y < vx[1].y; y++, vpos+= vinc) { \
		v = vpos >> 16; \
		for (x = vx[0].x, upos = vx[0].u << 16; x < vx[1].x; x++, upos+= uinc) { \
			u = upos >> 16; \
			drawPixel(x, y, ftx); \
		} \
	}

void drawSpriteTDecal(Vertex * vx) {
	int x, y;
	int u, v;
	int upos, uinc;
	int vpos, vinc;

#ifdef GS_LOG
	GS_LOG("drawSpriteTDecal %dx%d - %dx%d %lx; tex: %dx%d - %dx%d (psm=%x, cbp=%x, csm=%x, cpsm=%x)\n",
		   vx[0].x, vx[0].y, vx[1].x, vx[1].y, gs.rgba,
		   vx[0].u, vx[0].v, vx[1].u, vx[1].v,
		   tex0->psm, tex0->cbp, tex0->csm, tex0->cpsm);
#endif

	_drawSpriteT(
		GetTexturePixel32(u, v)
	);
}

void drawSpriteTModulate(Vertex * vx) {
	int x, y;
	int u, v;
	int upos, uinc;
	int vpos, vinc;

#ifdef GS_LOG
	GS_LOG("drawSpriteTModulate %dx%d - %dx%d %lx; tex: %dx%d - %dx%d (psm=%x, cbp=%x, csm=%x, cpsm=%x)\n",
		   vx[0].x, vx[0].y, vx[1].x, vx[1].y, gs.rgba,
		   vx[0].u, vx[0].v, vx[1].u, vx[1].v,
		   tex0->psm, tex0->cbp, tex0->csm, tex0->cpsm);
#endif

	colSetCol(gs.rgba);
	_drawSpriteT(
		colModulate(GetTexturePixel32(u, v))
	);
}

void drawSpriteTHighlight(Vertex * vx) {
	int x, y;
	int u, v;
	int upos, uinc;
	int vpos, vinc;

#ifdef GS_LOG
	GS_LOG("drawSpriteTHighlight %dx%d - %dx%d %lx; tex: %dx%d - %dx%d (psm=%x, cbp=%x, csm=%x, cpsm=%x)\n",
		   vx[0].x, vx[0].y, vx[1].x, vx[1].y, gs.rgba,
		   vx[0].u, vx[0].v, vx[1].u, vx[1].v,
		   tex0->psm, tex0->cbp, tex0->csm, tex0->cpsm);
#endif

	colSetCol(gs.rgba);
	_drawSpriteT(
		colHighlight(GetTexturePixel32(u, v))
	);
}

void drawSpriteTHighlight2(Vertex * vx) {
	int x, y;
	int u, v;
	int upos, uinc;
	int vpos, vinc;

#ifdef GS_LOG
	GS_LOG("drawSpriteTHighlight2 %dx%d - %dx%d %lx; tex: %dx%d - %dx%d (psm=%x, cbp=%x, csm=%x, cpsm=%x)\n",
		   vx[0].x, vx[0].y, vx[1].x, vx[1].y, gs.rgba,
		   vx[0].u, vx[0].v, vx[1].u, vx[1].v,
		   tex0->psm, tex0->cbp, tex0->csm, tex0->cpsm);
#endif

	colSetCol(gs.rgba);
	_drawSpriteT(
		colHighlight2(GetTexturePixel32(u, v))
	);
}

////////////////////////////////////////////////////////////////////////
// TEXTURED SPRITE WITH Z-MAPPING
////////////////////////////////////////////////////////////////////////

#define _drawSpriteT_Z(ftx) \
	SetTexture(); \
	SETdrawPixelZ(); \
 \
	if (vx[1].x == vx[0].x) uinc = 0; \
	else uinc = ((vx[1].u - vx[0].u) << 16) / (vx[1].x - vx[0].x); \
	if (vx[1].y == vx[0].y) vinc = 0; \
	else vinc = ((vx[1].v - vx[0].v) << 16) / (vx[1].y - vx[0].y); \
 \
	for (y = vx[0].y, vpos = vx[0].v << 16; y < vx[1].y; y++, vpos+= vinc) { \
		v = vpos >> 16; \
		for (x = vx[0].x, upos = vx[0].u << 16; x < vx[1].x; x++, upos+= uinc) { \
			u = upos >> 16; \
			drawPixelZ(x, y, ftx, vx[1].z); \
		} \
	}

void drawSpriteTDecal_Z(Vertex * vx) {
	int x, y;
	int u, v;
	int upos, uinc;
	int vpos, vinc;

#ifdef GS_LOG
	GS_LOG("drawSpriteTDecal_Z %dx%d - %dx%d %lx; tex: %dx%d - %dx%d (psm=%x, cbp=%x, csm=%x, cpsm=%x)\n",
		   vx[0].x, vx[0].y, vx[1].x, vx[1].y, gs.rgba,
		   vx[0].u, vx[0].v, vx[1].u, vx[1].v,
		   tex0->psm, tex0->cbp, tex0->csm, tex0->cpsm);
#endif

	_drawSpriteT_Z(
		GetTexturePixel32(u, v)
	);
}

void drawSpriteTModulate_Z(Vertex * vx) {
	int x, y;
	int u, v;
	int upos, uinc;
	int vpos, vinc;

#ifdef GS_LOG
	GS_LOG("drawSpriteTModulate_Z %dx%d - %dx%d %lx; tex: %dx%d - %dx%d (psm=%x, tbp0=%x, cbp=%x, csm=%x, cpsm=%x)\n",
		   vx[0].x, vx[0].y, vx[1].x, vx[1].y, gs.rgba,
		   vx[0].u, vx[0].v, vx[1].u, vx[1].v,
		   tex0->psm, tex0->tbp0, tex0->cbp, tex0->csm, tex0->cpsm);
#endif

	colSetCol(gs.rgba);
	_drawSpriteT_Z(
		colModulate(GetTexturePixel32(u, v))
	);
}

void drawSpriteTHighlight_Z(Vertex * vx) {
	int x, y;
	int u, v;
	int upos, uinc;
	int vpos, vinc;

#ifdef GS_LOG
	GS_LOG("drawSpriteTHighlight_Z %dx%d - %dx%d %lx; tex: %dx%d - %dx%d (psm=%x, cbp=%x, csm=%x, cpsm=%x)\n",
		   vx[0].x, vx[0].y, vx[1].x, vx[1].y, gs.rgba,
		   vx[0].u, vx[0].v, vx[1].u, vx[1].v,
		   tex0->psm, tex0->cbp, tex0->csm, tex0->cpsm);
#endif

	colSetCol(gs.rgba);
	_drawSpriteT_Z(
		colHighlight(GetTexturePixel32(u, v))
	);
}

void drawSpriteTHighlight2_Z(Vertex * vx) {
	int x, y;
	int u, v;
	int upos, uinc;
	int vpos, vinc;

#ifdef GS_LOG
	GS_LOG("drawSpriteTHighlight2_Z %dx%d - %dx%d %lx; tex: %dx%d - %dx%d (psm=%x, cbp=%x, csm=%x, cpsm=%x)\n",
		   vx[0].x, vx[0].y, vx[1].x, vx[1].y, gs.rgba,
		   vx[0].u, vx[0].v, vx[1].u, vx[1].v,
		   tex0->psm, tex0->cbp, tex0->csm, tex0->cpsm);
#endif

	colSetCol(gs.rgba);
	_drawSpriteT_Z(
		colHighlight2(GetTexturePixel32(u, v))
	);
}



typedef struct {
	s32 X, U, V;
	s32 R, G, B, A;
	s64 Z;
} tagPolyEdge;

tagPolyEdge ForwardEdges[1024*8];
tagPolyEdge BackwardEdges[1024*8];

#define drawTriangle(scanFunc, drawFunc) { \
	int i; \
	int nYMin, nYMax; \
	int nPosMin, nPosMax; \
	int nPos1, nPos2; \
	int nY; \
 \
	nYMin = v[0].y; \
	nYMax = v[0].y; \
	nPosMin = 0; \
	nPosMax = 0; \
 \
	for (i=1; i<3; i++) { \
		if (v[i].y > nYMax) { \
			nYMax = v[i].y; \
			nPosMax = i; \
		} \
 \
		if (v[i].y < nYMin) { \
			nYMin = v[i].y; \
			nPosMin = i; \
		} \
	} \
 \
	if (nYMax == nYMin) \
		return; \
 \
	nPos1 = nPosMin; \
	nPos2 = nPos1; \
	while (nPos1 != nPosMax) { \
		nPos2 ++; \
		if (nPos2 == 3) \
			nPos2 = 0; \
 \
		scanFunc(ForwardEdges, &v[nPos1], &v[nPos2]); \
		nPos1 = nPos2; \
   } \
 \
	nPos1 = nPosMin; \
	nPos2 = nPos1; \
	while (nPos1 != nPosMax) { \
		nPos2 --; \
		if (nPos2 == -1) \
			nPos2 = 2; \
 \
		scanFunc(BackwardEdges, &v[nPos1], &v[nPos2]); \
		nPos1 = nPos2; \
	} \
 \
 	if (nYMin < 0) nYMin = 0; \
	for (nY = nYMin; nY < nYMax; nY++) { \
		if (BackwardEdges[nY].X < ForwardEdges[nY].X) { \
			drawFunc(nY, &BackwardEdges[nY], &ForwardEdges[nY]); \
		} else { \
			drawFunc(nY, &ForwardEdges[nY], &BackwardEdges[nY]); \
		} \
	} \
}


void scanEdgeF(tagPolyEdge* pastEdge, Vertex *v1, Vertex *v2) {
	int nX, nY;
	int nXInc;

	if (v1->y == v2->y)
		return;

	nX = v1->x << 16;
	nXInc = ((v2->x - v1->x) << 16) / (v2->y - v1->y);

	for (nY = v1->y; nY < 0; nY++) {
		nX += nXInc;
	}
	for (; nY < v2->y; nY++) {
		pastEdge[nY].X = nX >> 16; nX += nXInc;
	}
}

void drawHLineF(int nY, tagPolyEdge *stEdgeIni, tagPolyEdge *stEdgeEnd) {
	int nX;

	if (stEdgeEnd->X <= stEdgeIni->X) {
		return;
	}

	for (nX = stEdgeIni->X; nX <= stEdgeEnd->X; nX++) {
		drawPixel(nX, nY, gs.rgba);
	}
}

void drawTriangleF(Vertex * v) {
#ifdef GS_LOG
	GS_LOG("drawTriangleF %dx%d - %dx%d - %dx%d rgba=%x\n",
		   v[0].x, v[0].y, v[1].x, v[1].y, v[2].x, v[2].y, gs.rgba);
#endif

	SETdrawPixel();

	drawTriangle(scanEdgeF, drawHLineF);
}

void scanEdgeFZ(tagPolyEdge* pastEdge, Vertex *v1, Vertex *v2) {
	s32 nX, nY;
	s32 nXInc;
	s64 nZ;
	s64 nZInc;

	if (v1->y == v2->y)
		return;

	nX = v1->x << 16;
	nZ = (u64)v1->z << 16;

	nXInc = ((v2->x - v1->x) << 16) / (v2->y - v1->y);
	nZInc = ((s64)((s64)v2->z - v1->z) << 16) / (v2->y - v1->y);

	for (nY = v1->y; nY < 0; nY++) {
		nX += nXInc;
		nZ += nZInc;
	}
	for (; nY < v2->y; nY++) {
		pastEdge[nY].X = nX >> 16; nX += nXInc;
		pastEdge[nY].Z = nZ; nZ += nZInc;
	}
}

void drawHLineFZ(int nY, tagPolyEdge *stEdgeIni, tagPolyEdge *stEdgeEnd) {
	s32 nX;
	s64 nZ;
	s64 nZInc;

	if (stEdgeEnd->X <= stEdgeIni->X) {
		return;
	}

	nZ = stEdgeIni->Z;
	nZInc = (s64)((s64)stEdgeEnd->Z - stEdgeIni->Z) / (stEdgeEnd->X - stEdgeIni->X);

	for (nX = stEdgeIni->X; nX <= stEdgeEnd->X; nX++) {
		drawPixelZ(nX, nY, gs.rgba, (u32)(nZ >> 16));
		nZ += nZInc;
	}
}

void drawTriangleF_Z(Vertex * v) {
#ifdef GS_LOG
	GS_LOG("drawTriangleF_Z %dx%dx%lx - %dx%dx%lx - %dx%dx%lx rgba=%x\n",
		   v[0].x, v[0].y, v[0].z, v[1].x, v[1].y, v[1].z, v[2].x, v[2].y, v[2].z, gs.rgba);
#endif

	SETdrawPixelZ();

	drawTriangle(scanEdgeFZ, drawHLineFZ);
}


void scanEdgeG(tagPolyEdge* pastEdge, Vertex *v1, Vertex *v2) {
	s32 nX, nY, nR, nG, nB, nA;
	s32 nXInc, nRInc, nGInc, nBInc, nAInc;
	s32 r1, g1, b1, a1;
	s32 r2, g2, b2, a2;

	if (v1->y == v2->y)
		return;

	b1 = (v1->rgba & 0x000000FF);
	g1 = (v1->rgba & 0x0000FF00) >> 8;
	r1 = (v1->rgba & 0x00FF0000) >> 16;
	a1 = (v1->rgba & 0xFF000000) >> 24;
	b2 = (v2->rgba & 0x000000FF);
	g2 = (v2->rgba & 0x0000FF00) >> 8;
	r2 = (v2->rgba & 0x00FF0000) >> 16;
	a2 = (v2->rgba & 0xFF000000) >> 24;

	nX = v1->x << 16;
	nR =    r1 << 16;
	nG =    g1 << 16;
	nB =    b1 << 16;
	nA =    a1 << 16;
	nXInc = ((v2->x - v1->x) << 16) / (v2->y - v1->y);
	nRInc = ((   r2 -    r1) << 16) / (v2->y - v1->y);
	nGInc = ((   g2 -    g1) << 16) / (v2->y - v1->y);
	nBInc = ((   b2 -    b1) << 16) / (v2->y - v1->y);
	nAInc = ((   a2 -    a1) << 16) / (v2->y - v1->y);

	for (nY = v1->y; nY < 0; nY++) {
		nX += nXInc;
		nR += nRInc;
		nG += nGInc;
		nB += nBInc;
		nA += nAInc;
	}
	for (; nY < v2->y; nY++) {
		pastEdge[nY].X = nX >> 16; nX += nXInc;
		pastEdge[nY].R = nR; nR += nRInc;
		pastEdge[nY].G = nG; nG += nGInc;
		pastEdge[nY].B = nB; nB += nBInc;
		pastEdge[nY].A = nA; nA += nAInc;
	}
}

void drawHLineG(int nY, tagPolyEdge *stEdgeIni, tagPolyEdge *stEdgeEnd) {
	s32 nX, nR, nG, nB, nA;
	s32 nRInc, nGInc, nBInc, nAInc;

	if (stEdgeEnd->X <= stEdgeIni->X) {
		return;
	}

	nR = stEdgeIni->R;
	nG = stEdgeIni->G;
	nB = stEdgeIni->B;
	nA = stEdgeIni->A;
	nRInc = (stEdgeEnd->R - stEdgeIni->R) / (stEdgeEnd->X - stEdgeIni->X);
	nGInc = (stEdgeEnd->G - stEdgeIni->G) / (stEdgeEnd->X - stEdgeIni->X);
	nBInc = (stEdgeEnd->B - stEdgeIni->B) / (stEdgeEnd->X - stEdgeIni->X);
	nAInc = (stEdgeEnd->A - stEdgeIni->A) / (stEdgeEnd->X - stEdgeIni->X);

	for (nX = stEdgeIni->X; nX <= stEdgeEnd->X; nX++) {
		drawPixel(nX, nY, ( nR & 0xff0000)        | 
						  ((nG & 0xff0000) >> 8)  | 
						  ((nB & 0xff0000) >> 16) |
						  ((nA & 0xff0000) << 8));
		nR += nRInc; nG += nGInc; nB += nBInc; nA += nAInc;
	}
}

void drawTriangleG(Vertex * v) {
#ifdef GS_LOG
	GS_LOG("drawTriangleG %dx%d - %dx%d - %dx%d rgba=%x\n",
		   v[0].x, v[0].y, v[1].x, v[1].y, v[2].x, v[2].y, v[0].rgba);
#endif

	SETdrawPixel();

	drawTriangle(scanEdgeG, drawHLineG);
}


void scanEdgeGZ(tagPolyEdge* pastEdge, Vertex *v1, Vertex *v2) {
	s32 nX, nY, nR, nG, nB, nA;
	s32 nXInc, nRInc, nGInc, nBInc, nAInc;
	s32 r1, g1, b1, a1;
	s32 r2, g2, b2, a2;
	s64 nZ;
	s64 nZInc;

	if (v1->y == v2->y)
		return;

	b1 = (v1->rgba & 0x000000FF);
	g1 = (v1->rgba & 0x0000FF00) >> 8;
	r1 = (v1->rgba & 0x00FF0000) >> 16;
	a1 = (v1->rgba & 0xFF000000) >> 24;
	b2 = (v2->rgba & 0x000000FF);
	g2 = (v2->rgba & 0x0000FF00) >> 8;
	r2 = (v2->rgba & 0x00FF0000) >> 16;
	a2 = (v2->rgba & 0xFF000000) >> 24;

	nX = v1->x << 16;
	nZ = (u64)v1->z << 16;
	nR =    r1 << 16;
	nG =    g1 << 16;
	nB =    b1 << 16;
	nA =    a1 << 16;
	nXInc = ((v2->x - v1->x) << 16) / (v2->y - v1->y);
	nZInc = ((s64)((s64)v2->z - v1->z) << 16) / (v2->y - v1->y);
	nRInc = ((   r2 -    r1) << 16) / (v2->y - v1->y);
	nGInc = ((   g2 -    g1) << 16) / (v2->y - v1->y);
	nBInc = ((   b2 -    b1) << 16) / (v2->y - v1->y);
	nAInc = ((   a2 -    a1) << 16) / (v2->y - v1->y);

	for (nY = v1->y; nY < 0; nY++) {
		nX += nXInc;
		nZ += nZInc;
		nR += nRInc;
		nG += nGInc;
		nB += nBInc;
		nA += nAInc;
	}
	for (;nY < v2->y; nY++) {
		pastEdge[nY].X = nX >> 16; nX += nXInc;
		pastEdge[nY].Z = nZ; nZ += nZInc;
		pastEdge[nY].R = nR; nR += nRInc;
		pastEdge[nY].G = nG; nG += nGInc;
		pastEdge[nY].B = nB; nB += nBInc;
		pastEdge[nY].A = nA; nA += nAInc;
	}
}

void drawHLineGZ(int nY, tagPolyEdge *stEdgeIni, tagPolyEdge *stEdgeEnd) {
	s32 nX, nR, nG, nB, nA;
	s32 nRInc, nGInc, nBInc, nAInc;
	s64 nZ;
	s64 nZInc;

	if (stEdgeEnd->X <= stEdgeIni->X) {
		return;
	}

	nZ = stEdgeIni->Z;
	nR = stEdgeIni->R;
	nG = stEdgeIni->G;
	nB = stEdgeIni->B;
	nA = stEdgeIni->A;
	nZInc = (s64)((s64)stEdgeEnd->Z - stEdgeIni->Z) / (stEdgeEnd->X - stEdgeIni->X);
	nRInc = (stEdgeEnd->R - stEdgeIni->R) / (stEdgeEnd->X - stEdgeIni->X);
	nGInc = (stEdgeEnd->G - stEdgeIni->G) / (stEdgeEnd->X - stEdgeIni->X);
	nBInc = (stEdgeEnd->B - stEdgeIni->B) / (stEdgeEnd->X - stEdgeIni->X);
	nAInc = (stEdgeEnd->A - stEdgeIni->A) / (stEdgeEnd->X - stEdgeIni->X);

	for (nX = stEdgeIni->X; nX <= stEdgeEnd->X; nX++) {
		drawPixelZ(nX, nY, ( nR & 0xff0000)        | 
						   ((nG & 0xff0000) >> 8)  | 
						   ((nB & 0xff0000) >> 16) |
						   ((nA & 0xff0000) << 8), (u32)(nZ >> 16));
		nZ += nZInc;
		nR += nRInc; nG += nGInc; nB += nBInc; nA += nAInc;
	}
}

void drawTriangleG_Z(Vertex * v) {
#ifdef GS_LOG
	GS_LOG("drawTriangleG_Z %dx%d - %dx%d - %dx%d rgba=%x\n",
		   v[0].x, v[0].y, v[1].x, v[1].y, v[2].x, v[2].y, v[0].rgba);
#endif

	SETdrawPixelZ();

	drawTriangle(scanEdgeGZ, drawHLineGZ);
}


void scanEdgeFT(tagPolyEdge* pastEdge, Vertex *v1, Vertex *v2) {
	int nX, nY, nU, nV;
	int nXInc, nUInc, nVInc;

	if (v1->y == v2->y)
		return;

	nX = v1->x << 16;
	nU = v1->u << 16;
	nV = v1->v << 16;

	nXInc = ((v2->x - v1->x) << 16) / (v2->y - v1->y);
	nUInc = ((v2->u - v1->u) << 16) / (v2->y - v1->y);
	nVInc = ((v2->v - v1->v) << 16) / (v2->y - v1->y);

	for (nY = v1->y; nY < 0; nY++) {
		nX += nXInc;
		nU += nUInc;
		nV += nVInc;
	}
	for (; nY < v2->y; nY++) {
		pastEdge[nY].X = nX >> 16; nX += nXInc;
		pastEdge[nY].U = nU; nU += nUInc;
		pastEdge[nY].V = nV; nV += nVInc;
	}
}


#define _drawHLineFT(name, ftx) \
void drawHLineFT##name(int nY, tagPolyEdge *stEdgeIni, tagPolyEdge *stEdgeEnd) { \
	int nX, nU, nV; \
	int nUInc, nVInc; \
 \
	if (stEdgeEnd->X <= stEdgeIni->X) { \
		return; \
	} \
 \
	nX = stEdgeIni->X; \
	nU = stEdgeIni->U; \
	nV = stEdgeIni->V; \
	nUInc = (stEdgeEnd->U - stEdgeIni->U) / (stEdgeEnd->X - stEdgeIni->X); \
	nVInc = (stEdgeEnd->V - stEdgeIni->V) / (stEdgeEnd->X - stEdgeIni->X); \
 \
	for (nX = stEdgeIni->X; nX <= stEdgeEnd->X; nX++) { \
		drawPixel(nX, nY, ftx); \
		nU += nUInc; nV += nVInc; \
	} \
}

_drawHLineFT(Decal,
	GetTexturePixel32(nU >> 16, nV >> 16)
);
_drawHLineFT(Modulate,
	colModulate(GetTexturePixel32(nU >> 16, nV >> 16))
);
_drawHLineFT(Highlight,
	colHighlight(GetTexturePixel32(nU >> 16, nV >> 16))
);
_drawHLineFT(Highlight2,
	colHighlight2(GetTexturePixel32(nU >> 16, nV >> 16))
);

void drawTriangleFTDecal(Vertex * v) {
#ifdef GS_LOG
	GS_LOG("drawTriangleFTDecal %dx%d - %dx%d - %dx%d rgba=%x\n",
		   v[0].x, v[0].y, v[1].x, v[1].y, v[2].x, v[2].y, gs.rgba);
	GS_LOG("uv: %xx%x - %xx%x - %xx%x\n",
		   v[0].u, v[0].v, v[1].u, v[1].v, v[2].u, v[2].v);
#endif

	SetTexture();
	SETdrawPixel();

	drawTriangle(scanEdgeFT, drawHLineFTDecal);
}

void drawTriangleFTModulate(Vertex * v) {
#ifdef GS_LOG
	GS_LOG("drawTriangleFTModulate %dx%d - %dx%d - %dx%d rgba=%x\n",
		   v[0].x, v[0].y, v[1].x, v[1].y, v[2].x, v[2].y, gs.rgba);
	GS_LOG("uv: %xx%x - %xx%x - %xx%x\n",
		   v[0].u, v[0].v, v[1].u, v[1].v, v[2].u, v[2].v);
#endif

	SetTexture();
	SETdrawPixel();
	colSetCol(gs.rgba);

	drawTriangle(scanEdgeFT, drawHLineFTModulate);
}

void drawTriangleFTHighlight(Vertex * v) {
#ifdef GS_LOG
	GS_LOG("drawTriangleFTHighlight %dx%d - %dx%d - %dx%d rgba=%x\n",
		   v[0].x, v[0].y, v[1].x, v[1].y, v[2].x, v[2].y, gs.rgba);
	GS_LOG("uv: %xx%x - %xx%x - %xx%x\n",
		   v[0].u, v[0].v, v[1].u, v[1].v, v[2].u, v[2].v);
#endif

	SetTexture();
	SETdrawPixel();
	colSetCol(gs.rgba);

	drawTriangle(scanEdgeFT, drawHLineFTHighlight);
}

void drawTriangleFTHighlight2(Vertex * v) {
#ifdef GS_LOG
	GS_LOG("drawTriangleFTHighlight2 %dx%d - %dx%d - %dx%d rgba=%x\n",
		   v[0].x, v[0].y, v[1].x, v[1].y, v[2].x, v[2].y, gs.rgba);
	GS_LOG("uv: %xx%x - %xx%x - %xx%x\n",
		   v[0].u, v[0].v, v[1].u, v[1].v, v[2].u, v[2].v);
#endif

	SetTexture();
	SETdrawPixel();
	colSetCol(gs.rgba);

	drawTriangle(scanEdgeFT, drawHLineFTHighlight2);
}

void scanEdgeFTZ(tagPolyEdge* pastEdge, Vertex *v1, Vertex *v2) {
	int nX, nY, nU, nV;
	int nXInc, nUInc, nVInc;
	s64 nZ;
	s64 nZInc;

	if (v1->y == v2->y)
		return;

	nX = v1->x << 16;
	nZ = (u64)v1->z << 16;
	nU = v1->u << 16;
	nV = v1->v << 16;

	nXInc = ((v2->x - v1->x) << 16) / (v2->y - v1->y);
	nZInc = ((s64)((s64)v2->z - v1->z) << 16) / (v2->y - v1->y);
	nUInc = ((v2->u - v1->u) << 16) / (v2->y - v1->y);
	nVInc = ((v2->v - v1->v) << 16) / (v2->y - v1->y);

	for (nY = v1->y; nY < 0; nY++) {
		nX += nXInc;
		nZ += nZInc;
		nU += nUInc;
		nV += nVInc;
	}
	for (; nY < v2->y; nY++) {
		pastEdge[nY].X = nX >> 16; nX += nXInc;
		pastEdge[nY].Z = nZ; nZ += nZInc;
		pastEdge[nY].U = nU; nU += nUInc;
		pastEdge[nY].V = nV; nV += nVInc;
	}
}


#define _drawHLineFTZ(name, ftx) \
void drawHLineFTZ##name(int nY, tagPolyEdge *stEdgeIni, tagPolyEdge *stEdgeEnd) { \
	int nX, nU, nV; \
	int nUInc, nVInc; \
	s64 nZ; \
	s64 nZInc; \
 \
	if (stEdgeEnd->X <= stEdgeIni->X) { \
		return; \
	} \
 \
	nX = stEdgeIni->X; \
	nZ = stEdgeIni->Z; \
	nU = stEdgeIni->U; \
	nV = stEdgeIni->V; \
	nZInc = (s64)((s64)stEdgeEnd->Z - stEdgeIni->Z) / (stEdgeEnd->X - stEdgeIni->X); \
	nUInc = (stEdgeEnd->U - stEdgeIni->U) / (stEdgeEnd->X - stEdgeIni->X); \
	nVInc = (stEdgeEnd->V - stEdgeIni->V) / (stEdgeEnd->X - stEdgeIni->X); \
 \
	for (nX = stEdgeIni->X; nX <= stEdgeEnd->X; nX++) { \
		drawPixelZ(nX, nY, ftx, (u32)(nZ >> 16)); \
		nZ += nZInc; \
		nU += nUInc; nV += nVInc; \
	} \
}

_drawHLineFTZ(Decal,
	GetTexturePixel32(nU >> 16, nV >> 16)
);
_drawHLineFTZ(Modulate,
	colModulate(GetTexturePixel32(nU >> 16, nV >> 16))
);
_drawHLineFTZ(Highlight,
	colHighlight(GetTexturePixel32(nU >> 16, nV >> 16))
);
_drawHLineFTZ(Highlight2,
	colHighlight2(GetTexturePixel32(nU >> 16, nV >> 16))
);

void drawTriangleFTDecal_Z(Vertex * v) {
#ifdef GS_LOG
	GS_LOG("drawTriangleFTDecal_Z %dx%d - %dx%d - %dx%d rgba=%x\n",
		   v[0].x, v[0].y, v[1].x, v[1].y, v[2].x, v[2].y, gs.rgba);
	GS_LOG("uv: %xx%x - %xx%x - %xx%x\n",
		   v[0].u, v[0].v, v[1].u, v[1].v, v[2].u, v[2].v);
#endif

	SetTexture();
	SETdrawPixelZ();

	drawTriangle(scanEdgeFTZ, drawHLineFTZDecal);
}

void drawTriangleFTModulate_Z(Vertex * v) {
#ifdef GS_LOG
	GS_LOG("drawTriangleFTModulate_Z %dx%d - %dx%d - %dx%d rgba=%x\n",
		   v[0].x, v[0].y, v[1].x, v[1].y, v[2].x, v[2].y, gs.rgba);
	GS_LOG("uv: %xx%x - %xx%x - %xx%x\n",
		   v[0].u, v[0].v, v[1].u, v[1].v, v[2].u, v[2].v);
#endif

	SetTexture();
	SETdrawPixelZ();
	colSetCol(gs.rgba);

	drawTriangle(scanEdgeFTZ, drawHLineFTZModulate);
}

void drawTriangleFTHighlight_Z(Vertex * v) {
#ifdef GS_LOG
	GS_LOG("drawTriangleFTHighlight_Z %dx%d - %dx%d - %dx%d rgba=%x\n",
		   v[0].x, v[0].y, v[1].x, v[1].y, v[2].x, v[2].y, gs.rgba);
	GS_LOG("uv: %xx%x - %xx%x - %xx%x\n",
		   v[0].u, v[0].v, v[1].u, v[1].v, v[2].u, v[2].v);
#endif

	SetTexture();
	SETdrawPixelZ();
	colSetCol(gs.rgba);

	drawTriangle(scanEdgeFTZ, drawHLineFTZHighlight);
}

void drawTriangleFTHighlight2_Z(Vertex * v) {
#ifdef GS_LOG
	GS_LOG("drawTriangleFTHighlight2_Z %dx%d - %dx%d - %dx%d rgba=%x\n",
		   v[0].x, v[0].y, v[1].x, v[1].y, v[2].x, v[2].y, gs.rgba);
	GS_LOG("uv: %xx%x - %xx%x - %xx%x\n",
		   v[0].u, v[0].v, v[1].u, v[1].v, v[2].u, v[2].v);
#endif

	SetTexture();
	SETdrawPixelZ();
	colSetCol(gs.rgba);

	drawTriangle(scanEdgeFTZ, drawHLineFTZHighlight2);
}

/**************************/
/* Gouraud Shaded/Textued */

void scanEdgeGT(tagPolyEdge* pastEdge, Vertex *v1, Vertex *v2) {
	s32 nX, nU, nV, nY, nR, nG, nB, nA;
	s32 nXInc, nUInc, nVInc, nRInc, nGInc, nBInc, nAInc;
	s32 r1, g1, b1, a1;
	s32 r2, g2, b2, a2;

	if (v1->y == v2->y)
		return;

	b1 = (v1->rgba & 0x000000FF);
	g1 = (v1->rgba & 0x0000FF00) >> 8;
	r1 = (v1->rgba & 0x00FF0000) >> 16;
	a1 = (v1->rgba & 0xFF000000) >> 24;
	b2 = (v2->rgba & 0x000000FF);
	g2 = (v2->rgba & 0x0000FF00) >> 8;
	r2 = (v2->rgba & 0x00FF0000) >> 16;
	a2 = (v2->rgba & 0xFF000000) >> 24;

	nX = v1->x << 16;
	nR =    r1 << 16;
	nG =    g1 << 16;
	nB =    b1 << 16;
	nA =    a1 << 16;
	nU = v1->u << 16;
	nV = v1->v << 16;

	nXInc = ((v2->x - v1->x) << 16) / (v2->y - v1->y);
	nRInc = ((   r2 -    r1) << 16) / (v2->y - v1->y);
	nGInc = ((   g2 -    g1) << 16) / (v2->y - v1->y);
	nBInc = ((   b2 -    b1) << 16) / (v2->y - v1->y);
	nAInc = ((   a2 -    a1) << 16) / (v2->y - v1->y);
	nUInc = ((v2->u - v1->u) << 16) / (v2->y - v1->y);
	nVInc = ((v2->v - v1->v) << 16) / (v2->y - v1->y);

	for (nY = v1->y; nY < 0; nY++) {
		nX += nXInc;
		nU += nUInc;
		nV += nVInc;
		nR += nRInc;
		nG += nGInc;
		nB += nBInc;
		nA += nAInc;
	}
	for (; nY < v2->y; nY++) {
		pastEdge[nY].X = nX >> 16; nX += nXInc;
		pastEdge[nY].U = nU; nU += nUInc;
		pastEdge[nY].V = nV; nV += nVInc;
		pastEdge[nY].R = nR; nR += nRInc;
		pastEdge[nY].G = nG; nG += nGInc;
		pastEdge[nY].B = nB; nB += nBInc;
		pastEdge[nY].A = nA; nA += nAInc;
	}
}

#define _drawHLineGT(name, ftx) \
void drawHLineGT##name(int nY, tagPolyEdge *stEdgeIni, tagPolyEdge *stEdgeEnd) { \
	s32 nX, nU, nV, nR, nG, nB, nA; \
	s32 nUInc, nVInc, nRInc, nGInc, nBInc, nAInc; \
 \
	if (stEdgeEnd->X <= stEdgeIni->X) { \
		return; \
	} \
 \
	nR = stEdgeIni->R; \
	nG = stEdgeIni->G; \
	nB = stEdgeIni->B; \
	nA = stEdgeIni->A; \
	nU = stEdgeIni->U; \
	nV = stEdgeIni->V; \
	nRInc = (stEdgeEnd->R - stEdgeIni->R) / (stEdgeEnd->X - stEdgeIni->X); \
	nGInc = (stEdgeEnd->G - stEdgeIni->G) / (stEdgeEnd->X - stEdgeIni->X); \
	nBInc = (stEdgeEnd->B - stEdgeIni->B) / (stEdgeEnd->X - stEdgeIni->X); \
	nAInc = (stEdgeEnd->A - stEdgeIni->A) / (stEdgeEnd->X - stEdgeIni->X); \
	nUInc = (stEdgeEnd->U - stEdgeIni->U) / (stEdgeEnd->X - stEdgeIni->X); \
	nVInc = (stEdgeEnd->V - stEdgeIni->V) / (stEdgeEnd->X - stEdgeIni->X); \
 \
	for (nX = stEdgeIni->X; nX <= stEdgeEnd->X; nX++) { \
		colSetCol(( nR & 0xff0000)        | \
				  ((nG & 0xff0000) >> 8)  | \
				  ((nB & 0xff0000) >> 16) | \
				  ((nA & 0xff0000) << 8)); \
		drawPixel(nX, nY, ftx); \
		nU += nUInc; nV += nVInc; \
		nR += nRInc; nG += nGInc; nB += nBInc; nA += nAInc; \
	} \
}

_drawHLineGT(Modulate,
	colModulate(GetTexturePixel32(nU >> 16, nV >> 16))
);
_drawHLineGT(Highlight,
	colHighlight(GetTexturePixel32(nU >> 16, nV >> 16))
);
_drawHLineGT(Highlight2,
	colHighlight2(GetTexturePixel32(nU >> 16, nV >> 16))
);


void drawTriangleGTDecal(Vertex * v) {
#ifdef GS_LOG
	GS_LOG("drawTriangleGTDecal %dx%d - %dx%d - %dx%d rgba=%x\n",
		   v[0].x, v[0].y, v[1].x, v[1].y, v[2].x, v[2].y, gs.rgba);
	GS_LOG("uv: %xx%x - %xx%x - %xx%x\n",
		   v[0].u, v[0].v, v[1].u, v[1].v, v[2].u, v[2].v);
#endif

	drawTriangleFTDecal(v);
}

void drawTriangleGTModulate(Vertex * v) {
#ifdef GS_LOG
	GS_LOG("drawTriangleGTModulate %dx%d - %dx%d - %dx%d rgba=%x\n",
		   v[0].x, v[0].y, v[1].x, v[1].y, v[2].x, v[2].y, gs.rgba);
	GS_LOG("uv: %xx%x - %xx%x - %xx%x\n",
		   v[0].u, v[0].v, v[1].u, v[1].v, v[2].u, v[2].v);
#endif

	SetTexture();
	SETdrawPixel();

	drawTriangle(scanEdgeGT, drawHLineGTModulate);
}

void drawTriangleGTHighlight(Vertex * v) {
#ifdef GS_LOG
	GS_LOG("drawTriangleGTHighlight %dx%d - %dx%d - %dx%d rgba=%x\n",
		   v[0].x, v[0].y, v[1].x, v[1].y, v[2].x, v[2].y, gs.rgba);
	GS_LOG("uv: %xx%x - %xx%x - %xx%x\n",
		   v[0].u, v[0].v, v[1].u, v[1].v, v[2].u, v[2].v);
#endif

	SetTexture();
	SETdrawPixel();

	drawTriangle(scanEdgeGT, drawHLineGTHighlight);
}

void drawTriangleGTHighlight2(Vertex * v) {
#ifdef GS_LOG
	GS_LOG("drawTriangleGTHighlight2 %dx%d - %dx%d - %dx%d rgba=%x\n",
		   v[0].x, v[0].y, v[1].x, v[1].y, v[2].x, v[2].y, gs.rgba);
	GS_LOG("uv: %xx%x - %xx%x - %xx%x\n",
		   v[0].u, v[0].v, v[1].u, v[1].v, v[2].u, v[2].v);
#endif

	SetTexture();
	SETdrawPixel();

	drawTriangle(scanEdgeGT, drawHLineGTHighlight2);
}

void scanEdgeGTZ(tagPolyEdge* pastEdge, Vertex *v1, Vertex *v2) {
	s32 nX, nU, nV, nY, nR, nG, nB, nA;
	s32 nXInc, nUInc, nVInc, nRInc, nGInc, nBInc, nAInc;
	s64 nZ;
	s64 nZInc;
	s32 r1, g1, b1, a1;
	s32 r2, g2, b2, a2;

	if (v1->y == v2->y)
		return;

	b1 = (v1->rgba & 0x000000FF);
	g1 = (v1->rgba & 0x0000FF00) >> 8;
	r1 = (v1->rgba & 0x00FF0000) >> 16;
	a1 = (v1->rgba & 0xFF000000) >> 24;
	b2 = (v2->rgba & 0x000000FF);
	g2 = (v2->rgba & 0x0000FF00) >> 8;
	r2 = (v2->rgba & 0x00FF0000) >> 16;
	a2 = (v2->rgba & 0xFF000000) >> 24;

	nX = v1->x << 16;
	nZ = (u64)v1->z << 16;
	nR =    r1 << 16;
	nG =    g1 << 16;
	nB =    b1 << 16;
	nA =    a1 << 16;
	nU = v1->u << 16;
	nV = v1->v << 16;

	nXInc = ((v2->x - v1->x) << 16) / (v2->y - v1->y);
	nZInc = ((s64)((s64)v2->z - v1->z) << 16) / (v2->y - v1->y);
	nRInc = ((   r2 -    r1) << 16) / (v2->y - v1->y);
	nGInc = ((   g2 -    g1) << 16) / (v2->y - v1->y);
	nBInc = ((   b2 -    b1) << 16) / (v2->y - v1->y);
	nAInc = ((   a2 -    a1) << 16) / (v2->y - v1->y);
	nUInc = ((v2->u - v1->u) << 16) / (v2->y - v1->y);
	nVInc = ((v2->v - v1->v) << 16) / (v2->y - v1->y);

	for (nY = v1->y; nY < 0; nY++) {
		nX += nXInc;
		nZ += nZInc;
		nU += nUInc;
		nV += nVInc;
		nR += nRInc;
		nG += nGInc;
		nB += nBInc;
		nA += nAInc;
	}
	for (; nY < v2->y; nY++) {
		pastEdge[nY].X = nX >> 16; nX += nXInc;
		pastEdge[nY].Z = nZ; nZ += nZInc;
		pastEdge[nY].U = nU; nU += nUInc;
		pastEdge[nY].V = nV; nV += nVInc;
		pastEdge[nY].R = nR; nR += nRInc;
		pastEdge[nY].G = nG; nG += nGInc;
		pastEdge[nY].B = nB; nB += nBInc;
		pastEdge[nY].A = nA; nA += nAInc;
	}
}

#define _drawHLineGTZ(name, ftx) \
void drawHLineGTZ##name(int nY, tagPolyEdge *stEdgeIni, tagPolyEdge *stEdgeEnd) { \
	s32 nX, nU, nV, nR, nG, nB, nA; \
	s32 nUInc, nVInc, nRInc, nGInc, nBInc, nAInc; \
	s64 nZ; \
	s64 nZInc; \
 \
	if (stEdgeEnd->X <= stEdgeIni->X) { \
		return; \
	} \
 \
	nZ = stEdgeIni->Z; \
	nR = stEdgeIni->R; \
	nG = stEdgeIni->G; \
	nB = stEdgeIni->B; \
	nA = stEdgeIni->A; \
	nU = stEdgeIni->U; \
	nV = stEdgeIni->V; \
	nZInc = (s64)((s64)stEdgeEnd->Z - stEdgeIni->Z) / (stEdgeEnd->X - stEdgeIni->X); \
	nRInc = (stEdgeEnd->R - stEdgeIni->R) / (stEdgeEnd->X - stEdgeIni->X); \
	nGInc = (stEdgeEnd->G - stEdgeIni->G) / (stEdgeEnd->X - stEdgeIni->X); \
	nBInc = (stEdgeEnd->B - stEdgeIni->B) / (stEdgeEnd->X - stEdgeIni->X); \
	nAInc = (stEdgeEnd->A - stEdgeIni->A) / (stEdgeEnd->X - stEdgeIni->X); \
	nUInc = (stEdgeEnd->U - stEdgeIni->U) / (stEdgeEnd->X - stEdgeIni->X); \
	nVInc = (stEdgeEnd->V - stEdgeIni->V) / (stEdgeEnd->X - stEdgeIni->X); \
 \
	for (nX = stEdgeIni->X; nX <= stEdgeEnd->X; nX++) { \
		colSetCol(( nR & 0xff0000)        | \
				  ((nG & 0xff0000) >> 8)  | \
				  ((nB & 0xff0000) >> 16) | \
				  ((nA & 0xff0000) << 8)); \
		drawPixelZ(nX, nY, ftx, (u32)(nZ >> 16)); \
		nZ += nZInc; \
		nU += nUInc; nV += nVInc; \
		nR += nRInc; nG += nGInc; nB += nBInc; nA += nAInc; \
	} \
}

_drawHLineGTZ(Modulate,
	colModulate(GetTexturePixel32(nU >> 16, nV >> 16))
);
_drawHLineGTZ(Highlight,
	colHighlight(GetTexturePixel32(nU >> 16, nV >> 16))
);
_drawHLineGTZ(Highlight2,
	colHighlight2(GetTexturePixel32(nU >> 16, nV >> 16))
);

void drawTriangleGTDecal_Z(Vertex * v) {
#ifdef GS_LOG
	GS_LOG("drawTriangleGTDecal_Z %dx%d - %dx%d - %dx%d rgba=%x\n",
		   v[0].x, v[0].y, v[1].x, v[1].y, v[2].x, v[2].y, gs.rgba);
	GS_LOG("uv: %xx%x - %xx%x - %xx%x\n",
		   v[0].u, v[0].v, v[1].u, v[1].v, v[2].u, v[2].v);
#endif

	drawTriangleFTDecal_Z(v);
}

void drawTriangleGTModulate_Z(Vertex * v) {
#ifdef GS_LOG
	GS_LOG("drawTriangleGTModulate_Z %dx%d - %dx%d - %dx%d rgba=%x\n",
		   v[0].x, v[0].y, v[1].x, v[1].y, v[2].x, v[2].y, gs.rgba);
	GS_LOG("uv: %xx%x - %xx%x - %xx%x\n",
		   v[0].u, v[0].v, v[1].u, v[1].v, v[2].u, v[2].v);
#endif

	SetTexture();
	SETdrawPixelZ();

	drawTriangle(scanEdgeGTZ, drawHLineGTZModulate);
}

void drawTriangleGTHighlight_Z(Vertex * v) {
#ifdef GS_LOG
	GS_LOG("drawTriangleGTHighlight_Z %dx%d - %dx%d - %dx%d rgba=%x\n",
		   v[0].x, v[0].y, v[1].x, v[1].y, v[2].x, v[2].y, gs.rgba);
	GS_LOG("uv: %xx%x - %xx%x - %xx%x\n",
		   v[0].u, v[0].v, v[1].u, v[1].v, v[2].u, v[2].v);
#endif

	SetTexture();
	SETdrawPixelZ();

	drawTriangle(scanEdgeGTZ, drawHLineGTZHighlight);
}

void drawTriangleGTHighlight2_Z(Vertex * v) {
#ifdef GS_LOG
	GS_LOG("drawTriangleGTHighlight2_Z %dx%d - %dx%d - %dx%d rgba=%x\n",
		   v[0].x, v[0].y, v[1].x, v[1].y, v[2].x, v[2].y, gs.rgba);
	GS_LOG("uv: %xx%x - %xx%x - %xx%x\n",
		   v[0].u, v[0].v, v[1].u, v[1].v, v[2].u, v[2].v);
#endif

	SetTexture();
	SETdrawPixelZ();

	drawTriangle(scanEdgeGTZ, drawHLineGTZHighlight2);
}


/*
 3DFC
 http://www.geocities.com/SiliconValley/Bay/1704
 
*/

