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
	//Old code
	return (!(conf.settings().xenosaga_spec) || !vb[i].zbuf.zmsk || prim->iip) ;
}

void __forceinline Kick::KickVertex(bool adc)
{
	FUNCLOG
	if (++gs.primC >= (int)g_primmult[prim->prim])
	{
		if (!adc && NoHighlights(prim->ctxt)) DrawPrim(prim->prim);
        else DirtyValidPrevPrim();
		
		gs.primC -= g_primsub[prim->prim];
		}
    gs.primIndex = gs.primNext();
}

template<bool DO_Z_FOG>
void Kick::Set_Vertex(VertexGPU *p, Vertex & gsvertex)
{
    VB& curvb = vb[prim->ctxt];
    
	p->move_x(gsvertex, curvb.offset.x);
	p->move_y(gsvertex, curvb.offset.y);
    if(DO_Z_FOG) {
        p->move_z(gsvertex, curvb.zprimmask);
        p->move_fog(gsvertex);
    }

	p->rgba = prim->iip ? gsvertex.rgba : gs.rgba;

	if (conf.settings().texa)
	{
		u32 B = ((p->rgba & 0xfe000000) >> 1) + (0x01000000 * vb[prim->ctxt].fba.fba);
		p->rgba = (p->rgba & 0xffffff) + B;
	}

	if (prim->tme)
	{
		if (prim->fst)
		{
			p->s = (float)gsvertex.u * fiTexWidth[prim->ctxt];
			p->t = (float)gsvertex.v * fiTexHeight[prim->ctxt];
			p->q = 1;
		}
		else
		{
			p->s = gsvertex.s;
			p->t = gsvertex.t;
			p->q = gsvertex.q;
		}
	}
}

__forceinline void Kick::Output_Vertex(VertexGPU vert, u32 id)
{
#ifdef WRITE_PRIM_LOGS
	ZZLog::Prim_Log("%c%d(%d): xyzf=(%4d,%4d,0x%x,%3d), rgba=0x%8.8x, stq = (%2.5f,%2.5f,%2.5f)",
					id == 0 ? '*' : ' ', id, prim->prim, vert.x / 8, vert.y / 8, vert.z, vert.f / 128,
					vert.rgba, Clamp(vert.s, -10, 10), Clamp(vert.t, -10, 10), Clamp(vert.q, -10, 10));
#endif
}

void Kick::DrawPrim(u32 prim_type)
{
    VB& curvb = vb[prim->ctxt];

    curvb.FlushTexData();

    if ((vb[!prim->ctxt].nCount > 0) && (vb[prim->ctxt].gsfb.fbp == vb[!prim->ctxt].gsfb.fbp))
    {
        assert(vb[prim->ctxt].nCount == 0);
        Flush(!prim->ctxt);
    }

    // check enough place is left for the biggest primitive (sprite)
    // This function is unlikely to be called so do not inline it.
    if (unlikely(curvb.nCount + 6 > curvb.nNumVertices))
        curvb.IncreaseVertexBuffer();

    VertexGPU* p = curvb.pBufferData + curvb.nCount;

    u32 prev;
    u32 last;
    switch(prim_type) {
        case PRIM_POINT:
            Set_Vertex<true>(&p[0], gs.gsvertex[gs.primIndex]);
            curvb.nCount ++;
            break;

        case PRIM_LINE:
            Set_Vertex<true>(&p[0], gs.gsvertex[gs.primPrev()]);
            Set_Vertex<true>(&p[1], gs.gsvertex[gs.primIndex]);
            curvb.nCount += 2;
            break;

        case PRIM_LINE_STRIP:
            if (likely(ValidPrevPrim)) {
                assert(curvb.nCount >= 1);
                p[0] = p[-1];
            } else {
                Set_Vertex<true>(&p[0], gs.gsvertex[gs.primPrev()]);
                ValidPrevPrim = true;
            }

            Set_Vertex<true>(&p[1], gs.gsvertex[gs.primIndex]);
            curvb.nCount += 2;
            break;

        case PRIM_TRIANGLE:
            Set_Vertex<true>(&p[0], gs.gsvertex[gs.primPrev(2)]);
            Set_Vertex<true>(&p[1], gs.gsvertex[gs.primPrev()]);
            Set_Vertex<true>(&p[2], gs.gsvertex[gs.primIndex]);
            curvb.nCount += 3;
            break;

        case PRIM_TRIANGLE_STRIP:
            if (likely(ValidPrevPrim)) {
                assert(curvb.nCount >= 2);
                p[0] = p[-2];
                p[1] = p[-1];
            } else {
                Set_Vertex<true>(&p[0], gs.gsvertex[gs.primPrev(2)]);
                Set_Vertex<true>(&p[1], gs.gsvertex[gs.primPrev()]);
                ValidPrevPrim = true;
            }

            Set_Vertex<true>(&p[2], gs.gsvertex[gs.primIndex]);
            curvb.nCount += 3;
            break;

        case PRIM_TRIANGLE_FAN:
            if (likely(ValidPrevPrim)) {
                assert(curvb.nCount >= 2);
                VertexGPU* TriFanVert = curvb.pBufferData + gs.nTriFanVert;
                p[0] = TriFanVert[0];
                p[1] = p[-1];
            } else {
                Set_Vertex<true>(&p[0], gs.gsTriFanVertex);
                Set_Vertex<true>(&p[1], gs.gsvertex[gs.primPrev(1)]);
                ValidPrevPrim = true;
                // Remenber the base for future processing
                gs.nTriFanVert = curvb.nCount;
            }

            Set_Vertex<true>(&p[2], gs.gsvertex[gs.primIndex]);
            curvb.nCount += 3;
            break;

        case PRIM_SPRITE:
            prev = gs.primPrev();
            last = gs.primIndex;

            // sprite is too small and AA shows lines (tek4, Mana Khemia)
            gs.gsvertex[last].x += (4 * AA.x);
            gs.gsvertex[last].y += (4 * AA.y);

            // might be bad sprite (KH dialog text)
            //if( gs.gsvertex[prev].x == gs.gsvertex[last].x || gs.gsvertex[prev].y == gs.gsvertex[last].y )
            //return;

            // process sprite as 2 triangles. The common diagonal is 0,1 and 3,4
            Set_Vertex<false>(&p[0], gs.gsvertex[prev]);
            Set_Vertex<true>(&p[1], gs.gsvertex[last]);

            // Only fog and Z of last vertex is valid
            p[0].z = p[1].z;
            p[0].f = p[1].f;

            // Duplicate the vertex
            p[3] = p[0];
            p[2] = p[0];
            p[4] = p[1];
            p[5] = p[1];

            // Move some vertex x coord to create the others corners of the sprite
            p[2].s = p[1].s;
            p[2].x = p[1].x;
            p[5].s = p[0].s;
            p[5].x = p[0].x;

            curvb.nCount += 6;
            break;

        default: break;
    }

    // Print DEBUG info and code assertion
    switch(prim_type) {
        case PRIM_TRIANGLE:
        case PRIM_TRIANGLE_STRIP:
        case PRIM_TRIANGLE_FAN:
            assert(gs.primC >= 3);
            Output_Vertex(p[2],2);
        case PRIM_LINE:
        case PRIM_LINE_STRIP:
        case PRIM_SPRITE:
            assert(gs.primC >= 2);
            Output_Vertex(p[1],1);
        case PRIM_POINT:
            assert(gs.primC >= 1);
            Output_Vertex(p[0],0);
        default: break;
    }
}
