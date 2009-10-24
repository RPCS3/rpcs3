/* 
 *	Copyright (C) 2007-2009 Gabest
 *	http://www.gabest.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *   
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *   
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA. 
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#pragma once

#include "GSRendererHW.h"

template<class Vertex> 
class GSRendererDX : public GSRendererHW<Vertex>
{
	GSVector2 m_pixelcenter;
	bool m_logz;
	bool m_fba;
	int m_pixoff_x;
	int m_pixoff_y;

protected:
	int m_topology;

	virtual void SetupDATE(GSTexture* rt, GSTexture* ds) {}
	virtual void UpdateFBA(GSTexture* rt) {}

public:
	GSRendererDX(GSTextureCache* tc, const GSVector2& pixelcenter = GSVector2(0, 0))
		: GSRendererHW<Vertex>(tc)
		, m_pixelcenter(pixelcenter)
		, m_topology(-1)
	{
		m_logz = !!theApp.GetConfig("logz", 0);
		m_fba = !!theApp.GetConfig("fba", 1);
		m_pixoff_x = theApp.GetConfig("pixoff_x", 0);
		m_pixoff_y = theApp.GetConfig("pixoff_y", 0);
	}

	virtual ~GSRendererDX()
	{
	}

	bool CreateDevice(GSDevice* dev)
	{
		if(!__super::CreateDevice(dev))
			return false;

		return true;
	}

	__forceinline void Draw(GSTexture* rt, GSTexture* ds, GSTextureCache::Source* tex)
	{
		GSDrawingEnvironment& env = m_env;
		GSDrawingContext* context = m_context;

		GSDeviceDX& dev = (GSDeviceDX&)*m_dev;

		//

		SetupDATE(rt, ds);

		//

		dev.BeginScene();

		// om

		GSDeviceDX::OMDepthStencilSelector om_dssel;

		if(context->TEST.ZTE)
		{
			om_dssel.ztst = context->TEST.ZTST;
			om_dssel.zwe = !context->ZBUF.ZMSK;
		}
		else
		{
			om_dssel.ztst = ZTST_ALWAYS;
		}

		if(context->FRAME.PSM != PSM_PSMCT24)
		{
			om_dssel.date = context->TEST.DATE;
		}

		if(m_fba)
		{
			om_dssel.fba = context->FBA.FBA;
		}

		GSDeviceDX::OMBlendSelector om_bsel;

		if(!IsOpaque())
		{
			om_bsel.abe = PRIM->ABE || PRIM->AA1 && m_vt.m_primclass == GS_LINE_CLASS;

			om_bsel.a = context->ALPHA.A;
			om_bsel.b = context->ALPHA.B;
			om_bsel.c = context->ALPHA.C;
			om_bsel.d = context->ALPHA.D;

			if(env.PABE.PABE)
			{
				if(om_bsel.a == 0 && om_bsel.b == 1 && om_bsel.c == 0 && om_bsel.d == 1)
				{
					// this works because with PABE alpha blending is on when alpha >= 0x80, but since the pixel shader 
					// cannot output anything over 0x80 (== 1.0) blending with 0x80 or turning it off gives the same result

					om_bsel.abe = 0; 
				}
				else
				{
					//Breath of Fire Dragon Quarter triggers this in battles. Graphics are fine though.
					ASSERT(0);
				}
			}
		}

		om_bsel.wrgba = ~GSVector4i::load((int)context->FRAME.FBMSK).eq8(GSVector4i::xffffffff()).mask();

		// vs

		GSDeviceDX::VSSelector vs_sel;

		vs_sel.tme = PRIM->TME;
		vs_sel.fst = PRIM->FST;
		vs_sel.logz = m_logz ? 1 : 0;

		if(om_dssel.ztst >= ZTST_ALWAYS && om_dssel.zwe)
		{
			if(context->ZBUF.PSM == PSM_PSMZ24)
			{
				if(m_vt.m_max.p.z > 0xffffff)
				{
					//ASSERT(m_vt.m_min.p.z > 0xffffff);
					// Fixme :Following conditional fixes some dialog frame in Wild Arms 3, but may not be what was intended.
					if (m_vt.m_min.p.z > 0xffffff)
					{
						vs_sel.bppz = 1;
						om_dssel.ztst = ZTST_ALWAYS;
					}
					//else printf ("GSdx: Z issue, please report\n");
				}
			}
			else if(context->ZBUF.PSM == PSM_PSMZ16 || context->ZBUF.PSM == PSM_PSMZ16S)
			{
				if(m_vt.m_max.p.z > 0xffff)
				{
					//ASSERT(m_vt.m_min.p.z > 0xffff); // sfex capcom logo
					// Fixme : Same as above, I guess.
					if (m_vt.m_min.p.z > 0xffff)
					{	
						vs_sel.bppz = 2;
						om_dssel.ztst = ZTST_ALWAYS;
					}
					//else printf ("GSdx: Z issue, please report\n");
				}
			}
		}

		GSDeviceDX::VSConstantBuffer vs_cb;

		float sx = 2.0f * rt->GetScale().x / (rt->GetWidth() << 4);
		float sy = 2.0f * rt->GetScale().y / (rt->GetHeight() << 4);
		float ox = (float)(int)context->XYOFFSET.OFX + m_pixoff_x;
		float oy = (float)(int)context->XYOFFSET.OFY + m_pixoff_y;
		float ox2 = 2.0f * m_pixelcenter.x / rt->GetWidth();
		float oy2 = 2.0f * m_pixelcenter.y / rt->GetHeight();

		vs_cb.VertexScale = GSVector4(sx, -sy, 1.0f / UINT_MAX, 0.0f);
		vs_cb.VertexOffset = GSVector4(ox * sx + ox2 + 1, -(oy * sy + oy2 + 1), 0.0f, -1.0f);

		// gs

		GSDeviceDX::GSSelector gs_sel;

		gs_sel.iip = PRIM->IIP;
		gs_sel.prim = m_vt.m_primclass;

		// ps

		GSDeviceDX::PSSelector ps_sel;
		GSDeviceDX::PSSamplerSelector ps_ssel;
		GSDeviceDX::PSConstantBuffer ps_cb;

		ps_sel.clr1 = om_bsel.IsCLR1();
		ps_sel.fba = context->FBA.FBA;
		ps_sel.aout = context->FRAME.PSM == PSM_PSMCT16 || context->FRAME.PSM == PSM_PSMCT16S || (context->FRAME.FBMSK & 0xff000000) == 0x7f000000 ? 1 : 0;

		if(PRIM->FGE)
		{
			ps_sel.fog = 1;

			ps_cb.FogColor_AREF = GSVector4(env.FOGCOL.u32[0]) / 255;
		}

		if(context->TEST.ATE)
		{
			ps_sel.atst = context->TEST.ATST;

			switch(ps_sel.atst)
			{
			case ATST_LESS:
				ps_cb.FogColor_AREF.a = (float)((int)context->TEST.AREF - 1);
				break;
			case ATST_GREATER:
				ps_cb.FogColor_AREF.a = (float)((int)context->TEST.AREF + 1);
				break;
			default:
				ps_cb.FogColor_AREF.a = (float)(int)context->TEST.AREF;
				break;
			}
		}
		else
		{
			ps_sel.atst = ATST_ALWAYS;
		}

		if(tex)
		{
			ps_sel.wms = context->CLAMP.WMS;
			ps_sel.wmt = context->CLAMP.WMT;
			ps_sel.fmt = tex->m_fmt;
			ps_sel.aem = env.TEXA.AEM;
			ps_sel.tfx = context->TEX0.TFX;
			ps_sel.tcc = context->TEX0.TCC;
			ps_sel.ltf = m_filter == 2 ? IsLinear() : m_filter;
			ps_sel.rt = tex->m_target;

			int w = tex->m_texture->GetWidth();
			int h = tex->m_texture->GetHeight();

			int tw = (int)(1 << context->TEX0.TW);
			int th = (int)(1 << context->TEX0.TH);

			GSVector4 WH(tw, th, w, h);

			if(PRIM->FST)
			{
				vs_cb.TextureScale = GSVector4(1.0f / 16) / WH.xyxy();

				ps_sel.fst = 1;
			}

			ps_cb.WH = WH;
			ps_cb.HalfTexel = GSVector4(-0.5f, 0.5f).xxyy() / WH.zwzw();
			ps_cb.MskFix = GSVector4i(context->CLAMP.MINU, context->CLAMP.MINV, context->CLAMP.MAXU, context->CLAMP.MAXV);

			GSVector4 clamp(ps_cb.MskFix);
			GSVector4 ta(env.TEXA & GSVector4i::x000000ff());

			ps_cb.MinMax = clamp / WH.xyxy();
			ps_cb.MinF_TA = (clamp + 0.5f).xyxy(ta) / WH.xyxy(GSVector4(255, 255));

			ps_ssel.tau = (context->CLAMP.WMS + 3) >> 1;
			ps_ssel.tav = (context->CLAMP.WMT + 3) >> 1;
			ps_ssel.ltf = ps_sel.ltf;
		}
		else
		{
			ps_sel.tfx = 4;
		}

		// rs

		GSVector4i scissor = GSVector4i(GSVector4(rt->GetScale()).xyxy() * context->scissor.in).rintersect(GSVector4i(rt->GetSize()).zwxy());

		dev.OMSetRenderTargets(rt, ds, &scissor);
		dev.PSSetShaderResources(tex ? tex->m_texture : NULL, tex ? tex->m_palette : NULL);

		uint8 afix = context->ALPHA.FIX;

		dev.SetupOM(om_dssel, om_bsel, afix);
		dev.SetupIA(m_vertices, m_count, m_topology);
		dev.SetupVS(vs_sel, &vs_cb);
		dev.SetupGS(gs_sel);
		dev.SetupPS(ps_sel, &ps_cb, ps_ssel);

		// draw

		if(context->TEST.DoFirstPass())
		{
			dev.DrawPrimitive();
		}

		if(context->TEST.DoSecondPass())
		{
			ASSERT(!env.PABE.PABE);

			static const uint32 iatst[] = {1, 0, 5, 6, 7, 2, 3, 4};

			ps_sel.atst = iatst[ps_sel.atst];

			switch(ps_sel.atst)
			{
			case ATST_LESS:
				ps_cb.FogColor_AREF.a = (float)((int)context->TEST.AREF - 1);
				break;
			case ATST_GREATER:
				ps_cb.FogColor_AREF.a = (float)((int)context->TEST.AREF + 1);
				break;
			default:
				ps_cb.FogColor_AREF.a = (float)(int)context->TEST.AREF;
				break;
			}

			dev.SetupPS(ps_sel, &ps_cb, ps_ssel);

			bool z = om_dssel.zwe;
			bool r = om_bsel.wr;
			bool g = om_bsel.wg;
			bool b = om_bsel.wb;
			bool a = om_bsel.wa;

			switch(context->TEST.AFAIL)
			{
			case 0: z = r = g = b = a = false; break; // none
			case 1: z = false; break; // rgba
			case 2: r = g = b = a = false; break; // z
			case 3: z = a = false; break; // rgb
			default: __assume(0);
			}

			if(z || r || g || b || a)
			{
				om_dssel.zwe = z;
				om_bsel.wr = r;
				om_bsel.wg = g;
				om_bsel.wb = b;
				om_bsel.wa = a;

				dev.SetupOM(om_dssel, om_bsel, afix);

				dev.DrawPrimitive();
			}
		}

		dev.EndScene();

		if(om_dssel.fba) UpdateFBA(rt);
	}
};