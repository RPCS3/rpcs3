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
#include "GSTextureFX.h"

template<class Vertex> 
class GSRendererDX : public GSRendererHW<Vertex>
{
	GSTextureFX* m_tfx;
	bool m_logz;
	bool m_fba;

protected:
	int m_topology;
	GSVector2 m_pixelcenter;

	virtual void SetupDATE(GSTexture* rt, GSTexture* ds) {}
	virtual void UpdateFBA(GSTexture* rt) {}

public:
	GSRendererDX(uint8* base, bool mt, void (*irq)(), GSDevice* dev, GSTextureCache* tc, GSTextureFX* tfx)
		: GSRendererHW<Vertex>(base, mt, irq, dev, tc)
		, m_tfx(tfx)
		, m_topology(-1)
		, m_pixelcenter(0, 0)
	{
		m_logz = !!theApp.GetConfig("logz", 0);
		m_fba = !!theApp.GetConfig("fba", 1);
	}

	virtual ~GSRendererDX()
	{
		delete m_tfx;
	}

	bool Create(const string& title)
	{
		if(!__super::Create(title))
			return false;

		if(!m_tfx->Create(m_dev))
			return false;

		return true;
	}

	void Draw(GS_PRIM_CLASS primclass, GSTexture* rt, GSTexture* ds, GSTextureCache::Source* tex)
	{
		GSDrawingEnvironment& env = m_env;
		GSDrawingContext* context = m_context;

		//

		SetupDATE(rt, ds);

		//

		m_dev->BeginScene();

		// om

		GSTextureFX::OMDepthStencilSelector om_dssel;

		om_dssel.zte = context->TEST.ZTE;
		om_dssel.ztst = context->TEST.ZTST;
		om_dssel.zwe = !context->ZBUF.ZMSK;
		om_dssel.date = context->FRAME.PSM != PSM_PSMCT24 ? context->TEST.DATE : 0;
		om_dssel.fba = m_fba ? context->FBA.FBA : 0;

		GSTextureFX::OMBlendSelector om_bsel;

		om_bsel.abe = !IsOpaque();

		if(om_bsel.abe)
		{
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
					ASSERT(0);
				}
			}
		}

		om_bsel.wr = (context->FRAME.FBMSK & 0x000000ff) != 0x000000ff;
		om_bsel.wg = (context->FRAME.FBMSK & 0x0000ff00) != 0x0000ff00;
		om_bsel.wb = (context->FRAME.FBMSK & 0x00ff0000) != 0x00ff0000;
		om_bsel.wa = (context->FRAME.FBMSK & 0xff000000) != 0xff000000;

		// vs

		GSTextureFX::VSSelector vs_sel;

		vs_sel.bppz = 0;
		vs_sel.tme = PRIM->TME;
		vs_sel.fst = PRIM->FST;
		vs_sel.logz = m_logz ? 1 : 0;
		vs_sel.prim = primclass;

		if(om_dssel.zte && om_dssel.ztst > 0 && om_dssel.zwe)
		{
			if(context->ZBUF.PSM == PSM_PSMZ24)
			{
				if(m_vt.m_max.p.z > 0xffffff)
				{
					ASSERT(m_vt.m_min.p.z > 0xffffff);

					vs_sel.bppz = 1;
					om_dssel.ztst = 1;
				}
			}
			else if(context->ZBUF.PSM == PSM_PSMZ16 || context->ZBUF.PSM == PSM_PSMZ16S)
			{
				if(m_vt.m_max.p.z > 0xffff)
				{
					ASSERT(m_vt.m_min.p.z > 0xffff); // sfex capcom logo

					vs_sel.bppz = 2;
					om_dssel.ztst = 1;
				}
			}
		}

		GSTextureFX::VSConstantBuffer vs_cb;

		float sx = 2.0f * rt->m_scale.x / (rt->GetWidth() * 16);
		float sy = 2.0f * rt->m_scale.y / (rt->GetHeight() * 16);
		float ox = (float)(int)context->XYOFFSET.OFX;
		float oy = (float)(int)context->XYOFFSET.OFY;
		float ox2 = 2.0f * m_pixelcenter.x / rt->GetWidth();
		float oy2 = 2.0f * m_pixelcenter.y / rt->GetHeight();

		vs_cb.VertexScale = GSVector4(sx, -sy, 1.0f / UINT_MAX, 0.0f);
		vs_cb.VertexOffset = GSVector4(ox * sx + ox2 + 1, -(oy * sy + oy2 + 1), 0.0f, -1.0f);

		if(PRIM->TME)
		{
			if(PRIM->FST)
			{
				vs_cb.TextureScale.x = 1.0f / (16 << context->TEX0.TW);
				vs_cb.TextureScale.y = 1.0f / (16 << context->TEX0.TH);
			}
			else
			{
				vs_cb.TextureScale = GSVector2(1.0f, 1.0f);
			}
		}

		// gs

		GSTextureFX::GSSelector gs_sel;

		gs_sel.iip = PRIM->IIP;
		gs_sel.prim = primclass;

		// ps

		GSTextureFX::PSSelector ps_sel;

		ps_sel.fst = PRIM->FST;
		ps_sel.wms = context->CLAMP.WMS;
		ps_sel.wmt = context->CLAMP.WMT;
		ps_sel.aem = env.TEXA.AEM;
		ps_sel.tfx = context->TEX0.TFX;
		ps_sel.tcc = context->TEX0.TCC;
		ps_sel.ate = context->TEST.ATE;
		ps_sel.atst = context->TEST.ATST;
		ps_sel.fog = PRIM->FGE;
		ps_sel.clr1 = om_bsel.abe && om_bsel.a == 1 && om_bsel.b == 2 && om_bsel.d == 1;
		ps_sel.fba = context->FBA.FBA;
		ps_sel.aout = context->FRAME.PSM == PSM_PSMCT16 || context->FRAME.PSM == PSM_PSMCT16S || (context->FRAME.FBMSK & 0xff000000) == 0x7f000000 ? 1 : 0;
		ps_sel.ltf = m_filter == 2 ? IsLinear() : m_filter;

		GSTextureFX::PSSamplerSelector ps_ssel;

		ps_ssel.tau = 0;
		ps_ssel.tav = 0;
		ps_ssel.ltf = ps_sel.ltf;

		GSTextureFX::PSConstantBuffer ps_cb;

		if(ps_sel.fog)
		{
			ps_cb.FogColor_AREF = GSVector4((int)env.FOGCOL.FCR, (int)env.FOGCOL.FCG, (int)env.FOGCOL.FCB, 0) / 255;
		}

		if(ps_sel.ate)
		{
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

		if(tex)
		{
			ps_sel.fmt = tex->m_fmt;
			ps_sel.rt = tex->m_target;

			int w = tex->m_texture->GetWidth();
			int h = tex->m_texture->GetHeight();

			ps_cb.WH = GSVector4((int)(1 << context->TEX0.TW), (int)(1 << context->TEX0.TH), w, h);
			ps_cb.HalfTexel = GSVector4(-0.5f, 0.5f).xxyy() / GSVector4(w, h).xyxy();
			ps_cb.MinF_TA.z = (float)(int)env.TEXA.TA0 / 255;
			ps_cb.MinF_TA.w = (float)(int)env.TEXA.TA1 / 255;

			switch(context->CLAMP.WMS)
			{
			case CLAMP_REPEAT: 
				ps_ssel.tau = 1; 
				break;
			case CLAMP_CLAMP: 
				ps_ssel.tau = 0; 
				break;
			case CLAMP_REGION_CLAMP: 
				ps_cb.MinMax.x = ((float)(int)context->CLAMP.MINU) / (1 << context->TEX0.TW);
				ps_cb.MinMax.z = ((float)(int)context->CLAMP.MAXU) / (1 << context->TEX0.TW);
				ps_cb.MinF_TA.x = ((float)(int)context->CLAMP.MINU + 0.5f) / (1 << context->TEX0.TW);
				ps_ssel.tau = 0; 
				break;
			case CLAMP_REGION_REPEAT: 
				ps_cb.MskFix.x = context->CLAMP.MINU;
				ps_cb.MskFix.z = context->CLAMP.MAXU;
				ps_ssel.tau = 1; 
				break;
			default: 
				__assume(0);
			}

			switch(context->CLAMP.WMT)
			{
			case CLAMP_REPEAT: 
				ps_ssel.tav = 1; 
				break;
			case CLAMP_CLAMP: 
				ps_ssel.tav = 0; 
				break;
			case CLAMP_REGION_CLAMP: 
				ps_cb.MinMax.y = ((float)(int)context->CLAMP.MINV) / (1 << context->TEX0.TH);
				ps_cb.MinMax.w = ((float)(int)context->CLAMP.MAXV) / (1 << context->TEX0.TH);
				ps_cb.MinF_TA.y = ((float)(int)context->CLAMP.MINV + 0.5f) / (1 << context->TEX0.TH);
				ps_ssel.tav = 0; 
				break;
			case CLAMP_REGION_REPEAT: 
				ps_cb.MskFix.y = context->CLAMP.MINV;
				ps_cb.MskFix.w = context->CLAMP.MAXV;
				ps_ssel.tav = 1; 
				break;
			default: 
				__assume(0);
			}
		}
		else
		{
			ps_sel.tfx = 4;
		}

		// rs

		int w = rt->GetWidth();
		int h = rt->GetHeight();

		GSVector4i scissor = GSVector4i(GSVector4(rt->m_scale).xyxy() * context->scissor.in).rintersect(GSVector4i(0, 0, w, h));

		//

		uint8 afix = context->ALPHA.FIX;

		m_tfx->SetupOM(om_dssel, om_bsel, afix, rt, ds);
		m_tfx->SetupIA(m_vertices, m_count, m_topology);
		m_tfx->SetupVS(vs_sel, &vs_cb);
		m_tfx->SetupGS(gs_sel);
		m_tfx->SetupPS(ps_sel, &ps_cb, ps_ssel, tex ? tex->m_texture : NULL, tex ? tex->m_palette : NULL);
		m_tfx->SetupRS(w, h, scissor);

		// draw

		if(context->TEST.DoFirstPass())
		{
			m_dev->DrawPrimitive();
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

			m_tfx->UpdatePS(ps_sel, &ps_cb, ps_ssel);

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

				m_tfx->UpdateOM(om_dssel, om_bsel, afix);

				m_dev->DrawPrimitive();
			}
		}

		m_dev->EndScene();

		if(om_dssel.fba) UpdateFBA(rt);
	}
};