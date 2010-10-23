/*  ZZ Open GL graphics plugin
 *  Copyright (c)2009-2010 zeydlitz@gmail.com, arcum42@gmail.com
 *  Based on Zerofrog's ZeroGS KOSMOS (c)2005-2008
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include "ZZKick.h"
#include "ZZoglVB.h"

Kick* ZZKick;

const u32 g_primmult[8] = { 1, 2, 2, 3, 3, 3, 2, 0xff };
const u32 g_primsub[8] = { 1, 2, 1, 3, 1, 1, 2, 0 };

const GLenum primtype[8] = { GL_POINTS, GL_LINES, GL_LINES, GL_TRIANGLES, GL_TRIANGLES, GL_TRIANGLES, GL_TRIANGLES, 0xffffffff };

extern float fiTexWidth[2], fiTexHeight[2];	// current tex width and height

// Still thinking about the best place to put this.
// called on a primitive switch
void Prim()
{
	FUNCLOG

	VB& curvb = vb[prim->ctxt];

	if (curvb.CheckPrim()) Flush(prim->ctxt);

	curvb.curprim._val = prim->_val;
	curvb.curprim.prim = prim->prim;
}

// return true if triangle SHOULD be painted.
// Hackish and should be replaced.
bool __forceinline NoHighlights(int i)
{
//	This is hack-code, I still in search of correct reason, why some triangles should not be drawn.

	int dummy = 0;
	
	u32 resultA = prim->iip + (2 * (prim->tme)) + (4 * (prim->fge)) + (8 * (prim->abe)) + (16 * (prim->aa1)) + (32 * (prim->fst)) + (64 * (prim->ctxt)) + (128 * (prim->fix));
	
	const pixTest curtest = vb[i].test;
	
	u32 result = curtest.ate + ((curtest.atst) << 1) +((curtest.afail) << 4) + ((curtest.date) << 6) + ((curtest.datm) << 7) + ((curtest.zte) << 8) + ((curtest.ztst)<< 9);
	
	if ((resultA == 0x310a) && (result == 0x0)) return false; // Radiata Stories
	
	//Old code
	return (!(conf.settings().xenosaga_spec) || !vb[i].zbuf.zmsk || prim->iip) ;
}

void __forceinline Kick::KickVERTEX2()
{
	FUNCLOG

	if (++gs.primC >= (int)g_primmult[prim->prim])
	{
		if (NoHighlights(prim->ctxt)) ZZKick->fn(prim->prim);

		gs.primC -= g_primsub[prim->prim];
	}
}

void __forceinline Kick::KickVERTEX3()
{
	FUNCLOG

	if (++gs.primC >= (int)g_primmult[prim->prim])
	{
		gs.primC -= g_primsub[prim->prim];

		if (prim->prim == 5)
		{
			/* tri fans need special processing */
			if (gs.nTriFanVert == gs.primIndex) gs.primIndex = gs.primNext();
		}
	}
}

void __forceinline Kick::KickVertex(bool adc)
{
	FUNCLOG
	if (++gs.primC >= (int)g_primmult[prim->prim])
	{
		if (!adc && NoHighlights(prim->ctxt)) fn(prim->prim);
		
		gs.primC -= g_primsub[prim->prim];

		if (adc && prim->prim == 5)
		{
			/* tri fans need special processing */
			if (gs.nTriFanVert == gs.primIndex) gs.primIndex = gs.primNext();
		}
	}
}

inline void Kick::SET_VERTEX(VertexGPU *p, int i)
{
	p->move_x(gs.gsvertex[i], vb[prim->ctxt].offset.x);
	p->move_y(gs.gsvertex[i], vb[prim->ctxt].offset.y);
	p->move_z(gs.gsvertex[i], vb[prim->ctxt].zprimmask);
	p->move_fog(gs.gsvertex[i]);

	p->rgba = prim->iip ? gs.gsvertex[i].rgba : gs.rgba;

	if (conf.settings().texa)
	{
		u32 B = ((p->rgba & 0xfe000000) >> 1) + (0x01000000 * vb[prim->ctxt].fba.fba);
		p->rgba = (p->rgba & 0xffffff) + B;
	}

	if (prim->tme)
	{
		if (prim->fst)
		{
			p->s = (float)gs.gsvertex[i].u * fiTexWidth[prim->ctxt];
			p->t = (float)gs.gsvertex[i].v * fiTexHeight[prim->ctxt];
			p->q = 1;
		}
		else
		{
			p->s = gs.gsvertex[i].s;
			p->t = gs.gsvertex[i].t;
			p->q = gs.gsvertex[i].q;
		}
	}
}

__forceinline void Kick::OUTPUT_VERT(VertexGPU vert, u32 id)
{
#ifdef WRITE_PRIM_LOGS
	ZZLog::Prim_Log("%c%d(%d): xyzf=(%4d,%4d,0x%x,%3d), rgba=0x%8.8x, stq = (%2.5f,%2.5f,%2.5f)\n",
					id == 0 ? '*' : ' ', id, prim->prim, vert.x / 8, vert.y / 8, vert.z, vert.f / 128,
					vert.rgba, Clamp(vert.s, -10, 10), Clamp(vert.t, -10, 10), Clamp(vert.q, -10, 10));
#endif
}

void Kick::fn(u32 i)
{
	switch (i)
	{
		case 0: Point(); break;
		case 1: Line(); break;
		case 2: Line(); break;
		case 3: Triangle(); break;
		case 4: Triangle(); break;
		case 5: TriangleFan(); break;
		case 6: Sprite(); break;
		default: Dummy(); break;
	}
}
	
void Kick::Point()
{
	FUNCLOG
	assert(gs.primC >= 1);

	VB& curvb = vb[prim->ctxt];

	curvb.FlushTexData();

	if ((vb[!prim->ctxt].nCount > 0) && (vb[prim->ctxt].gsfb.fbp == vb[!prim->ctxt].gsfb.fbp))
	{
		assert(vb[prim->ctxt].nCount == 0);
		Flush(!prim->ctxt);
	}

	curvb.NotifyWrite(1);

	int last = gs.primNext(2);

	VertexGPU* p = curvb.pBufferData + curvb.nCount;
	SET_VERTEX(&p[0], last);
	curvb.nCount++;

	OUTPUT_VERT(p[0], 0);
}

void Kick::Line()
{
	FUNCLOG
	assert(gs.primC >= 2);
	VB& curvb = vb[prim->ctxt];

	curvb.FlushTexData();

	if ((vb[!prim->ctxt].nCount > 0) && (vb[prim->ctxt].gsfb.fbp == vb[!prim->ctxt].gsfb.fbp))
	{
		assert(vb[prim->ctxt].nCount == 0);
		Flush(!prim->ctxt);
	}

	curvb.NotifyWrite(2);

	int next = gs.primNext();
	int last = gs.primNext(2);

	VertexGPU* p = curvb.pBufferData + curvb.nCount;
	SET_VERTEX(&p[0], next);
	SET_VERTEX(&p[1], last);

	curvb.nCount += 2;

	OUTPUT_VERT(p[0], 0);
	OUTPUT_VERT(p[1], 1);
}

void Kick::Triangle()
{
	FUNCLOG
	assert(gs.primC >= 3);
	VB& curvb = vb[prim->ctxt];

	curvb.FlushTexData();

	if ((vb[!prim->ctxt].nCount > 0) && (vb[prim->ctxt].gsfb.fbp == vb[!prim->ctxt].gsfb.fbp))
	{
		assert(vb[prim->ctxt].nCount == 0);
		Flush(!prim->ctxt);
	}

	curvb.NotifyWrite(3);

	VertexGPU* p = curvb.pBufferData + curvb.nCount;
	SET_VERTEX(&p[0], 0);
	SET_VERTEX(&p[1], 1);
	SET_VERTEX(&p[2], 2);

	curvb.nCount += 3;

	OUTPUT_VERT(p[0], 0);
	OUTPUT_VERT(p[1], 1);
	OUTPUT_VERT(p[2], 2);
}

void Kick::TriangleFan()
{
	FUNCLOG
	assert(gs.primC >= 3);
	VB& curvb = vb[prim->ctxt];

	curvb.FlushTexData();

	if ((vb[!prim->ctxt].nCount > 0) && (vb[prim->ctxt].gsfb.fbp == vb[!prim->ctxt].gsfb.fbp))
	{
		assert(vb[prim->ctxt].nCount == 0);
		Flush(!prim->ctxt);
	}

	curvb.NotifyWrite(3);

	VertexGPU* p = curvb.pBufferData + curvb.nCount;
	SET_VERTEX(&p[0], 0);
	SET_VERTEX(&p[1], 1);
	SET_VERTEX(&p[2], 2);

	curvb.nCount += 3;

	// add 1 to skip the first vertex

	if (gs.primIndex == gs.nTriFanVert) gs.primIndex = gs.primNext();

	OUTPUT_VERT(p[0], 0);
	OUTPUT_VERT(p[1], 1);
	OUTPUT_VERT(p[2], 2);
}

void Kick::SetKickVertex(VertexGPU *p, Vertex v, int next)
{
	SET_VERTEX(p, next);
	p->move_z(v, vb[prim->ctxt].zprimmask);
	p->move_fog(v);
}

void Kick::Sprite()
{
	FUNCLOG
	assert(gs.primC >= 2);
	VB& curvb = vb[prim->ctxt];

	curvb.FlushTexData();

	if ((vb[!prim->ctxt].nCount > 0) && (vb[prim->ctxt].gsfb.fbp == vb[!prim->ctxt].gsfb.fbp))
	{
		assert(vb[prim->ctxt].nCount == 0);
		Flush(!prim->ctxt);
	}

	curvb.NotifyWrite(6);
	int next = gs.primNext();
	int last = gs.primNext(2);
	
	// sprite is too small and AA shows lines (tek4, Mana Khemia)
	gs.gsvertex[last].x += (4 * AA.x);
	gs.gsvertex[last].y += (4 * AA.y);

	// might be bad sprite (KH dialog text)
	//if( gs.gsvertex[next].x == gs.gsvertex[last].x || gs.gsvertex[next].y == gs.gsvertex[last].y )
	//return;

	VertexGPU* p = curvb.pBufferData + curvb.nCount;

	SetKickVertex(&p[0], gs.gsvertex[last], next);
	SetKickVertex(&p[3], gs.gsvertex[last], next);
	SetKickVertex(&p[1], gs.gsvertex[last], last);
	SetKickVertex(&p[4], gs.gsvertex[last], last);
	SetKickVertex(&p[2], gs.gsvertex[last], next);

	p[2].s = p[1].s;
	p[2].x = p[1].x;

	SetKickVertex(&p[5], gs.gsvertex[last], last);

	p[5].s = p[0].s;
	p[5].x = p[0].x;

	curvb.nCount += 6;

	OUTPUT_VERT(p[0], 0);
	OUTPUT_VERT(p[1], 1);
}

void Kick::Dummy()
{
	FUNCLOG
	//ZZLog::Greg_Log("Kicking bad primitive: %.8x\n", *(u32*)prim);
}
