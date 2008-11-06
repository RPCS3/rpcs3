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

#include <string.h>

#include "GS.h"
#include "Soft.h"
#include "Mem.h"


__inline void stepGSvertex() {
	int i;

	gs.primC--;

	for (i=0; i<gs.primC; i++) {
		gs.gsvertex[i].x = gs.gsvertex[i+1].x;
		gs.gsvertex[i].y = gs.gsvertex[i+1].y;
		gs.gsvertex[i].z = gs.gsvertex[i+1].z;
		gs.gsvertex[i].f = gs.gsvertex[i+1].f;

		if (prim->tme) {
			if (prim->fst) {
				gs.gsvertex[i].u = gs.gsvertex[i+1].u;
				gs.gsvertex[i].v = gs.gsvertex[i+1].v;
			} else { 
				gs.gsvertex[i].s = gs.gsvertex[i+1].s;
				gs.gsvertex[i].t = gs.gsvertex[i+1].t;
				gs.gsvertex[i].q = gs.gsvertex[i+1].q;
			}
		}

		if (prim->iip) {
			gs.gsvertex[i].rgba = gs.gsvertex[i+1].rgba;
		}
	}
}

__inline void stepGSfanvertex() {
	gs.primC--;

	gs.gsvertex[1].x = gs.gsvertex[2].x;
	gs.gsvertex[1].y = gs.gsvertex[2].y;
	gs.gsvertex[1].z = gs.gsvertex[2].z;
	gs.gsvertex[1].f = gs.gsvertex[2].f;

	if (prim->tme) {
		if (prim->fst) {
			gs.gsvertex[1].u = gs.gsvertex[2].u;
			gs.gsvertex[1].v = gs.gsvertex[2].v;
		} else {
			gs.gsvertex[1].s = gs.gsvertex[2].s;
			gs.gsvertex[1].t = gs.gsvertex[2].t;
			gs.gsvertex[1].q = gs.gsvertex[2].q;
		}
	}

	if (prim->iip) {
		gs.gsvertex[1].rgba = gs.gsvertex[2].rgba;
	}
}

__inline void STQtoUV(Vertex *v, float q) {
#ifdef GS_LOG
	GS_LOG("STQtoUV %f, %f, %f (tw=%d, th=%d)\n", 
			*(float*)&v->s, *(float*)&v->t, q, tex0->tw, tex0->th);
#endif
	if (v->s == 0xffffffff) { printf("v->s == nan\n"); }
	if (v->t == 0xffffffff) { printf("v->t == nan\n"); }
	if (q == 0xffffffff) { printf("v->q == nan\n"); }
	if (q == 0) {
		v->u = (int)((float)(tex0->tw-1) * (*(float*)&v->s));
		v->v = (int)((float)(tex0->th-1) * (*(float*)&v->t));
	} else {
		v->u = (int)((float)(tex0->tw-1) * (*(float*)&v->s / q));
		v->v = (int)((float)(tex0->th-1) * (*(float*)&v->t / q));
	}
	if (v->u > tex0->tw) {
#ifdef GS_LOG
		GS_LOG("*warning*: U > TW\n");
#endif
	}
	if (v->v > tex0->th) {
#ifdef GS_LOG
		GS_LOG("*warning*: V > TH\n");
#endif
	}
}

void _primPoint(Vertex *vertex) {
	Vertex v;

	memcpy(&v, vertex, sizeof(Vertex));

#ifdef GS_LOG
	GS_LOG("primPoint %dx%d %lx\n", 
			v.x, v.y, gs.rgba);
#endif

	v.x -= offset->x; v.y -= offset->y;

	if (v.x < scissor->x0) return;
	if (v.y < scissor->y0) return;
	if (v.x > scissor->x1) return;
	if (v.y > scissor->y1) return;

	if (v.x < 0) return;
	if (v.x >= gsfb->fbw) return;
	if (v.y < 0) return;
	if (v.y >= gsfb->fbh) return;

	writePixel32(v.x, v.y, gs.rgba, gsfb->fbp, gsfb->fbw);
}

void primPoint(Vertex *vertex) {
	if (vertex && norender == 0) _primPoint(vertex);

	memset(gs.gsvertex, 0, sizeof(Vertex));
	gs.primC = 0;
}

void _primLine(Vertex *vertex) {
	Vertex v[2];

	memcpy(v, vertex, sizeof(Vertex) * 2);

	v[0].x -= offset->x; v[0].y -= offset->y;
	v[1].x -= offset->x; v[1].y -= offset->y;

#ifdef GS_LOG
	GS_LOG("primLine %dx%d %lx, %dx%d %lx\n", 
			v[0].x, v[0].y, v[0].rgba,
			v[1].x, v[1].y, v[1].rgba);
#endif
	if (prim->iip == 0) {
		if (test->zte)
			 drawLineF_Z(v);
		else drawLineF(v);
	} else {
		if (test->zte)
			 drawLineG_Z(v);
		else drawLineG(v);
	}
}

void primLine(Vertex *vertex) {
	if (vertex && norender == 0) _primLine(vertex);

	memset(gs.gsvertex, 0, sizeof(Vertex)*2);
	gs.primC = 0;
}

void primLineStrip(Vertex *vertex) {
	if (vertex && norender == 0) _primLine(vertex);

	stepGSvertex();
}

void _primTriangle(Vertex *vertex) {
	Vertex v[3];

	memcpy(v, vertex, sizeof(Vertex) * 3);

	v[0].x -= offset->x; v[0].y -= offset->y;
	v[1].x -= offset->x; v[1].y -= offset->y;
	v[2].x -= offset->x; v[2].y -= offset->y;

#ifdef GS_LOG
	GS_LOG("primTriangle%s%s %dx%dx%x %lx - %dx%dx%x %lx - %dx%dx%x %lx (offset %dx%d)\n",
			prim->iip == 0 ? "F" : "G",
			prim->tme == 0 ? "" : "T",
			v[0].x, v[0].y, v[0].z, v[0].rgba,
			v[1].x, v[1].y, v[1].z, v[1].rgba,
			v[2].x, v[2].y, v[2].z, v[2].rgba,
			offset->x, offset->y);
#endif

	if (wireframe) {
		drawLineF(&v[0]);
		drawLineF(&v[1]);
		memcpy(&v[1], &v[2], sizeof(Vertex));
		drawLineF(&v[0]);
		return;
	}

	if (prim->tme) {
		if (tex0->tw == 0 || tex0->th == 0) return;

		if (prim->fst == 0) {
#ifdef GS_LOG
			GS_LOG("stq: %fx%fx%f - %fx%fx%f - %fx%fx%f (tex0.tw=%d, tex0.th=%d)\n",
				   *(float*)&v[0].s, *(float*)&v[0].t, *(float*)&v[0].q,
				   *(float*)&v[1].s, *(float*)&v[1].t, *(float*)&v[1].q, 
				   *(float*)&v[2].s, *(float*)&v[2].t, *(float*)&v[2].q,
				   tex0->tw, tex0->th);
#endif

			if (prim->iip) {
				STQtoUV(&v[0], *(float*)&v[0].q);
				STQtoUV(&v[1], *(float*)&v[1].q);
				STQtoUV(&v[2], *(float*)&v[2].q);
			} else {
				STQtoUV(&v[0], *(float*)&gs.q);
				STQtoUV(&v[1], *(float*)&gs.q);
				STQtoUV(&v[2], *(float*)&gs.q);
			}
		}
#ifdef GS_LOG
		GS_LOG("uv: %dx%d - %dx%d - %dx%d (tex0.tw=%d, tex0.th=%d)\n",
			   v[0].u, v[0].v,
			   v[1].u, v[1].v, v[2].u, v[2].v,
			   tex0->tw, tex0->th);
#endif

#ifdef GS_LOG
		GS_LOG("uv: %dx%d - %dx%d - %dx%d (tex0.tw=%d, tex0.th=%d)\n",
			   v[0].u, v[0].v,
			   v[1].u, v[1].v, v[2].u, v[2].v,
			   tex0->tw, tex0->th);
#endif

		if (prim->iip) {
			if (test->zte) {
				switch (tex0->tfx) {
					case TEX_DECAL:      drawTriangleGTDecal_Z(v); break;
					case TEX_MODULATE:   drawTriangleGTModulate_Z(v); break;
					case TEX_HIGHLIGHT:  drawTriangleGTHighlight_Z(v); break;
					case TEX_HIGHLIGHT2: drawTriangleGTHighlight2_Z(v); break;
				}
			} else {
				switch (tex0->tfx) {
					case TEX_DECAL:      drawTriangleGTDecal(v); break;
					case TEX_MODULATE:   drawTriangleGTModulate(v); break;
					case TEX_HIGHLIGHT:  drawTriangleGTHighlight(v); break;
					case TEX_HIGHLIGHT2: drawTriangleGTHighlight2(v); break;
				}
			}
		} else {
			if(test->zte) {
				switch (tex0->tfx) {
					case TEX_DECAL:      drawTriangleFTDecal_Z(v); break;
					case TEX_MODULATE:   drawTriangleFTModulate_Z(v); break;
					case TEX_HIGHLIGHT:  drawTriangleFTHighlight_Z(v); break;
					case TEX_HIGHLIGHT2: drawTriangleFTHighlight2_Z(v); break;
				}
			} else {
				switch (tex0->tfx) {
					case TEX_DECAL:      drawTriangleFTDecal(v); break;
					case TEX_MODULATE:   drawTriangleFTModulate(v); break;
					case TEX_HIGHLIGHT:  drawTriangleFTHighlight(v); break;
					case TEX_HIGHLIGHT2: drawTriangleFTHighlight2(v); break;
				}
			}
		}
	} else {
		if (prim->iip) {
			if(test->zte)
				drawTriangleG_Z(v);
			else
				drawTriangleG(v);
		} else {
			if(test->zte)
				drawTriangleF_Z(v);
			else
				drawTriangleF(v);
		}
	}
}

void primTriangle(Vertex *vertex) {
	if (vertex && norender == 0) _primTriangle(vertex);

	memset(gs.gsvertex, 0, sizeof(Vertex)*3);
	gs.primC = 0;
//	if (conf.log) { DXupdate(); Sleep(250); }
}

void primTriangleStrip(Vertex *vertex) {
	if (vertex && norender == 0) _primTriangle(vertex);

	stepGSvertex();
//	if (conf.log) { DXupdate(); Sleep(250); }
}

void primTriangleFan(Vertex *vertex) {
	if (vertex && norender == 0) _primTriangle(vertex);

	stepGSfanvertex();
//	if (conf.log) { DXupdate(); Sleep(250); }
}

void _primSprite(Vertex *vertex) {
	Vertex v[2];

	memcpy(v, vertex, sizeof(Vertex) * 2);

#ifdef GS_LOG
	GS_LOG("primSprite%s %dx%d %lx - %dx%d %lx (offset %dx%d) (tex %dx%d %dx%d)\n", 
			prim->tme == 0 ? "" : "T",
			v[0].x, v[0].y, v[0].rgba,
			v[1].x, v[1].y, v[1].rgba,
			offset->x, offset->y,
			v[0].u, v[0].v,
			v[1].u, v[1].v);
#endif

	v[0].x -= offset->x; v[0].y -= offset->y;
	v[1].x -= offset->x; v[1].y -= offset->y;

	if (v[0].x > v[1].x) {
		int x;
		x = v[0].x; v[0].x = v[1].x; v[1].x = x;
		if (prim->tme) {
			float f;
			x = v[0].u; v[0].u = v[1].u; v[1].u = x;
			f = v[0].s; v[0].s = v[1].s; v[1].s = f;
		}
	}
	if (v[0].y > v[1].y) {
		int y;
		y = v[0].y; v[0].y = v[1].y; v[1].y = y;
		if (prim->tme) {
			float f;
			y = v[0].v; v[0].v = v[1].v; v[1].v = y;
			f = v[0].t; v[0].t = v[1].t; v[1].t = f;
		}
	}

	if (prim->tme) {
		if (tex0->tw == 0 || tex0->th == 0) return;

		if (prim->fst == 0) {
			STQtoUV(&v[0], *(float*)&gs.q);
			STQtoUV(&v[1], *(float*)&gs.q);
		}

		if(test->zte) {		// If z buffer enabled
			switch (tex0->tfx) {
				case TEX_DECAL:      drawSpriteTDecal_Z(v); break;
				case TEX_MODULATE:   drawSpriteTModulate_Z(v); break;
				case TEX_HIGHLIGHT:  drawSpriteTHighlight_Z(v); break;
				case TEX_HIGHLIGHT2: drawSpriteTHighlight2_Z(v); break;
			}
		} else {
			switch (tex0->tfx) {
				case TEX_DECAL:      drawSpriteTDecal(v); break;
				case TEX_MODULATE:   drawSpriteTModulate(v); break;
				case TEX_HIGHLIGHT:  drawSpriteTHighlight(v); break;
				case TEX_HIGHLIGHT2: drawSpriteTHighlight2(v); break;
			}
		}
	} else {
		if(test->zte)		// If z buffer enabled
			drawSprite_Z(v);
		else
			drawSprite(v);
	}
}

void primSprite(Vertex *vertex) {
	if (vertex && norender == 0) _primSprite(vertex);

	memset(gs.gsvertex, 0, sizeof(Vertex)*2);
	gs.primC = 0;
//	if (conf.log) { DXupdate(); Sleep(250); }
}

void primNull(Vertex *v) {
}

void (*primTable[8])(Vertex *v) = {
	primPoint, primLine, primLineStrip,
	primTriangle, primTriangleStrip, primTriangleFan,
	primSprite, primNull
};
