/*
 *	Copyright (C) 2011-2011 Gregory hainaut
 *	Copyright (C) 2007-2009 Gabest
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
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "stdafx.h"
#include "GSRendererOGL.h"
#include "GSRenderer.h"


GSRendererOGL::GSRendererOGL()
	: GSRendererHW(new GSTextureCacheOGL(this))
{
	m_logz = !!theApp.GetConfig("logz", 1);
	m_fba = !!theApp.GetConfig("fba", 1);
	UserHacks_AlphaHack = !!theApp.GetConfig("UserHacks_AlphaHack", 0) && !!theApp.GetConfig("UserHacks", 0);
	UserHacks_AlphaStencil = !!theApp.GetConfig("UserHacks_AlphaStencil", 0) && !!theApp.GetConfig("UserHacks", 0);
	UserHacks_DateGL4 = !!theApp.GetConfig("UserHacks_DateGL4", 0);
	m_pixelcenter = GSVector2(-0.5f, -0.5f);

	UserHacks_TCOffset = !!theApp.GetConfig("UserHacks", 0) ? theApp.GetConfig("UserHacks_TCOffset", 0) : 0;
	UserHacks_TCO_x = (UserHacks_TCOffset & 0xFFFF) / -1000.0f;
	UserHacks_TCO_y = ((UserHacks_TCOffset >> 16) & 0xFFFF) / -1000.0f;
}

bool GSRendererOGL::CreateDevice(GSDevice* dev)
{
	if(!GSRenderer::CreateDevice(dev))
		return false;

#ifdef ENABLE_GLES
	fprintf(stderr, "FIXME Creation of FBA dss/bs state is not yet implemented\n");
#endif

	return true;
}

void GSRendererOGL::EmulateGS()
{
	if (m_vt.m_primclass != GS_SPRITE_CLASS) return;

	// each sprite converted to quad needs twice the space

	while(m_vertex.tail * 2 > m_vertex.maxcount)
	{
		GrowVertexBuffer();
	}

	// assume vertices are tightly packed and sequentially indexed (it should be the case)

	if(m_vertex.next >= 2)
	{
		size_t count = m_vertex.next;

		int i = (int)count * 2 - 4;
		GSVertex* s = &m_vertex.buff[count - 2];
		GSVertex* q = &m_vertex.buff[count * 2 - 4];
		uint32* RESTRICT index = &m_index.buff[count * 3 - 6];

		for(; i >= 0; i -= 4, s -= 2, q -= 4, index -= 6)
		{
			GSVertex v0 = s[0];
			GSVertex v1 = s[1];

			v0.RGBAQ = v1.RGBAQ;
			v0.XYZ.Z = v1.XYZ.Z;
			v0.FOG = v1.FOG;

			q[0] = v0;
			q[3] = v1;

			// swap x, s, u

			uint16 x = v0.XYZ.X;
			v0.XYZ.X = v1.XYZ.X;
			v1.XYZ.X = x;

			float s = v0.ST.S;
			v0.ST.S = v1.ST.S;
			v1.ST.S = s;

			uint16 u = v0.U;
			v0.U = v1.U;
			v1.U = u;

			q[1] = v0;
			q[2] = v1;

			index[0] = i + 0;
			index[1] = i + 1;
			index[2] = i + 2;
			index[3] = i + 1;
			index[4] = i + 2;
			index[5] = i + 3;
		}

		m_vertex.head = m_vertex.tail = m_vertex.next = count * 2;
		m_index.tail = count * 3;
	}
}

void GSRendererOGL::SetupIA()
{
	GSDeviceOGL* dev = (GSDeviceOGL*)m_dev;

	if (!GLLoader::found_geometry_shader)
		EmulateGS();

	void* ptr = NULL;

	dev->IASetVertexState();

	if(UserHacks_WildHack && !isPackedUV_HackFlag) {
		// FIXME: why not put it on the Vertex shader
		if(dev->IAMapVertexBuffer(&ptr, sizeof(GSVertex), m_vertex.next))
		{
			GSVector4i::storent(ptr, m_vertex.buff, sizeof(GSVertex) * m_vertex.next);

			GSVertex* RESTRICT d = (GSVertex*)ptr;

			for(unsigned int i = 0; i < m_vertex.next; i++)
			{
				if(PRIM->TME && PRIM->FST) d[i].UV &= 0x3FEF3FEF;
			}

			dev->IAUnmapVertexBuffer();
		}
	} else {
		// By default use the common path (in case it can be made faster)
		dev->IASetVertexBuffer(m_vertex.buff, m_vertex.next);
	}

	dev->IASetIndexBuffer(m_index.buff, m_index.tail);

	GLenum t = 0;

	switch(m_vt.m_primclass)
	{
	case GS_POINT_CLASS:
		t = GL_POINTS;
		break;
	case GS_LINE_CLASS:
		t = GL_LINES;
		break;
	case GS_SPRITE_CLASS:
		if (GLLoader::found_geometry_shader)
			t = GL_LINES;
		else
			t = GL_TRIANGLES;
		break;
	case GS_TRIANGLE_CLASS:
		t = GL_TRIANGLES;
		break;
	default:
		__assume(0);
	}

	dev->IASetPrimitiveTopology(t);
}

void GSRendererOGL::DrawPrims(GSTexture* rt, GSTexture* ds, GSTextureCache::Source* tex)
{
	GSDrawingEnvironment& env = m_env;
	GSDrawingContext* context = m_context;

	const GSVector2i& rtsize = rt->GetSize();
	const GSVector2& rtscale = rt->GetScale();

	bool DATE = m_context->TEST.DATE && context->FRAME.PSM != PSM_PSMCT24;
	bool advance_DATE = false;

	ASSERT(m_dev != NULL);

	GSDeviceOGL* dev = (GSDeviceOGL*)m_dev;

	if(DATE)
	{
		// Note at the moment OGL has always stencil. Rt can be disabled
		if(dev->HasStencil())
		{
			GSVector4 s = GSVector4(rtscale.x / rtsize.x, rtscale.y / rtsize.y);
			GSVector4 o = GSVector4(-1.0f, 1.0f);

			GSVector4 src = ((m_vt.m_min.p.xyxy(m_vt.m_max.p) + o.xxyy()) * s.xyxy()).sat(o.zzyy());
			GSVector4 dst = src * 2.0f + o.xxxx();

			GSVertexPT1 vertices[] =
			{
				{GSVector4(dst.x, dst.y, 0.5f, 1.0f), GSVector2(src.x, src.y)},
				{GSVector4(dst.z, dst.y, 0.5f, 1.0f), GSVector2(src.z, src.y)},
				{GSVector4(dst.x, dst.w, 0.5f, 1.0f), GSVector2(src.x, src.w)},
				{GSVector4(dst.z, dst.w, 0.5f, 1.0f), GSVector2(src.z, src.w)},
			};

			dev->SetupDATE(rt, ds, vertices, m_context->TEST.DATM);
		}
		// Create an r32ui image that will containt primitive ID
		dev->InitPrimDateTexture(rtsize.x, rtsize.y);
	}

	//

	dev->BeginScene();

	// om

	GSDeviceOGL::OMDepthStencilSelector om_dssel;

	if(context->TEST.ZTE)
	{
		om_dssel.ztst = context->TEST.ZTST;
		om_dssel.zwe = !context->ZBUF.ZMSK;
	}
	else
	{
		om_dssel.ztst = ZTST_ALWAYS;
	}

	if(m_fba)
	{
		om_dssel.fba = context->FBA.FBA;
	}

	GSDeviceOGL::OMBlendSelector om_bsel;

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
				//ASSERT(0);
			}
		}
	}

	om_bsel.wrgba = ~GSVector4i::load((int)context->FRAME.FBMSK).eq8(GSVector4i::xffffffff()).mask();

	// TODO
	if (UserHacks_DateGL4 && DATE && om_bsel.wa && (!context->TEST.ATE || context->TEST.ATST == ATST_ALWAYS)) {
		//if (!(context->FBA.FBA && context->TEST.DATM == 1))

		//advance_DATE = true;
		advance_DATE = GLLoader::found_GL_ARB_shader_image_load_store;
	}

	// vs

	GSDeviceOGL::VSSelector vs_sel;

	vs_sel.tme = PRIM->TME;
	vs_sel.fst = PRIM->FST;
	vs_sel.logz = m_logz ? 1 : 0;

	// The real GS appears to do no masking based on the Z buffer format and writing larger Z values
	// than the buffer supports seems to be an error condition on the real GS, causing it to crash.
	// We are probably receiving bad coordinates from VU1 in these cases.

	if(om_dssel.ztst >= ZTST_ALWAYS && om_dssel.zwe)
	{
		if(context->ZBUF.PSM == PSM_PSMZ24)
		{
			if(m_vt.m_max.p.z > 0xffffff)
			{
				ASSERT(m_vt.m_min.p.z > 0xffffff);
				// Fixme :Following conditional fixes some dialog frame in Wild Arms 3, but may not be what was intended.
				if (m_vt.m_min.p.z > 0xffffff)
				{
					vs_sel.bppz = 1;
					om_dssel.ztst = ZTST_ALWAYS;
				}
			}
		}
		else if(context->ZBUF.PSM == PSM_PSMZ16 || context->ZBUF.PSM == PSM_PSMZ16S)
		{
			if(m_vt.m_max.p.z > 0xffff)
			{
				ASSERT(m_vt.m_min.p.z > 0xffff); // sfex capcom logo
				// Fixme : Same as above, I guess.
				if (m_vt.m_min.p.z > 0xffff)
				{
					vs_sel.bppz = 2;
					om_dssel.ztst = ZTST_ALWAYS;
				}
			}
		}
	}

	// FIXME Opengl support half pixel center (as dx10). Code could be easier!!!
	GSDeviceOGL::VSConstantBuffer vs_cb;

	float sx = 2.0f * rtscale.x / (rtsize.x << 4);
	float sy = 2.0f * rtscale.y / (rtsize.y << 4);
	float ox = (float)(int)context->XYOFFSET.OFX;
	float oy = (float)(int)context->XYOFFSET.OFY;
	float ox2 = -1.0f / rtsize.x;
	float oy2 = -1.0f / rtsize.y;

	//This hack subtracts around half a pixel from OFX and OFY. (Cannot do this directly,
	//because DX10 and DX9 have a different pixel center.)
	//
	//The resulting shifted output aligns better with common blending / corona / blurring effects,
	//but introduces a few bad pixels on the edges.

	if(rt->LikelyOffset)
	{
		ox2 *= rt->OffsetHack_modx;
		oy2 *= rt->OffsetHack_mody;
	}

	// Note: DX does y *= -1.0
	vs_cb.Vertex_Scale_Offset = GSVector4(sx, sy, ox * sx + ox2 + 1, oy * sy + oy2 + 1);
	// END of FIXME

	// ps

	GSDeviceOGL::PSSelector ps_sel;
	GSDeviceOGL::PSSamplerSelector ps_ssel;
	GSDeviceOGL::PSConstantBuffer ps_cb;

	// GS_SPRITE_CLASS are already flat (either by CPU or the GS)
	ps_sel.iip = (m_vt.m_primclass == GS_SPRITE_CLASS) ? 1 : PRIM->IIP;

	if(DATE)
	{
		om_dssel.date = 1;
		if (advance_DATE)
			ps_sel.date = 1 + context->TEST.DATM;
	}

	if(env.COLCLAMP.CLAMP == 0 && /* hack */ !tex && PRIM->PRIM != GS_POINTLIST)
	{
		ps_sel.colclip = 1;
	}

	ps_sel.clr1 = om_bsel.IsCLR1();
	ps_sel.fba = context->FBA.FBA;
	ps_sel.aout = context->FRAME.PSM == PSM_PSMCT16 || context->FRAME.PSM == PSM_PSMCT16S || (context->FRAME.FBMSK & 0xff000000) == 0x7f000000 ? 1 : 0;
		
	if(UserHacks_AlphaHack) ps_sel.aout = 1;

	if(PRIM->FGE)
	{
		ps_sel.fog = 1;

		ps_cb.FogColor_AREF = GSVector4::rgba32(env.FOGCOL.u32[0]) / 255;
	}

	if(context->TEST.ATE)
		ps_sel.atst = context->TEST.ATST;
	else
		ps_sel.atst = ATST_ALWAYS;

	if (context->TEST.ATE && context->TEST.ATST > 1)
		ps_cb.FogColor_AREF.a = (float)context->TEST.AREF;

	// Destination alpha pseudo stencil hack: use a stencil operation combined with an alpha test
	// to only draw pixels which would cause the destination alpha test to fail in the future once.
	// Unfortunately this also means only drawing those pixels at all, which is why this is a hack.
	// The interaction with FBA in D3D9 is probably less than ideal.
	if (UserHacks_AlphaStencil && DATE && dev->HasStencil() && om_bsel.wa && (!context->TEST.ATE || context->TEST.ATST == ATST_ALWAYS))
	{
		if (!context->FBA.FBA)
		{
			if (context->TEST.DATM == 0)
				ps_sel.atst = ATST_GEQUAL; // >=
			else
				ps_sel.atst = ATST_LESS; // <
			ps_cb.FogColor_AREF.a = (float)0x80;
		}
		if (!(context->FBA.FBA && context->TEST.DATM == 1))
			om_dssel.alpha_stencil = 1;
	}

	// By default don't use texture
	ps_sel.tfx = 4;

	if(tex)
	{
		const GSLocalMemory::psm_t &psm = GSLocalMemory::m_psm[context->TEX0.PSM];
		const GSLocalMemory::psm_t &cpsm = psm.pal > 0 ? GSLocalMemory::m_psm[context->TEX0.CPSM] : psm;
		bool bilinear = m_filter == 2 ? m_vt.IsLinear() : m_filter != 0;
		bool simple_sample = !tex->m_palette && cpsm.fmt == 0 && context->CLAMP.WMS < 3 && context->CLAMP.WMT < 3;

		ps_sel.wms = context->CLAMP.WMS;
		ps_sel.wmt = context->CLAMP.WMT;
		ps_sel.fmt = tex->m_palette ? cpsm.fmt | 4 : cpsm.fmt;
		ps_sel.aem = env.TEXA.AEM;
		ps_sel.tfx = context->TEX0.TFX;
		ps_sel.tcc = context->TEX0.TCC;
		ps_sel.ltf = bilinear && !simple_sample;
		ps_sel.spritehack = tex->m_spritehack_t;
		// FIXME the ati is currently disabled on the shader. I need to find a .gs to test that we got same
		// bug on opengl
		// FIXME for the moment disable it on subroutine (it will kill my perf for nothings)
		ps_sel.point_sampler = !(bilinear && simple_sample) && !GLLoader::found_GL_ARB_shader_subroutine;

		int w = tex->m_texture->GetWidth();
		int h = tex->m_texture->GetHeight();

		int tw = (int)(1 << context->TEX0.TW);
		int th = (int)(1 << context->TEX0.TH);

		GSVector4 WH(tw, th, w, h);

		if(PRIM->FST)
		{
			vs_cb.TextureScale = GSVector4(1.0f / 16) / WH.xyxy();
			//Maybe better?
			//vs_cb.TextureScale = GSVector4(1.0f / 16) * GSVector4(tex->m_texture->GetScale()).xyxy() / WH.zwzw();
			ps_sel.fst = 1;
		}

		ps_cb.WH = WH;
		ps_cb.HalfTexel = GSVector4(-0.5f, 0.5f).xxyy() / WH.zwzw();
		ps_cb.MskFix = GSVector4i(context->CLAMP.MINU, context->CLAMP.MINV, context->CLAMP.MAXU, context->CLAMP.MAXV);

		// TC Offset Hack
		ps_sel.tcoffsethack = !!UserHacks_TCOffset;
		ps_cb.TC_OffsetHack = GSVector4(UserHacks_TCO_x, UserHacks_TCO_y).xyxy() / WH.xyxy();

		GSVector4 clamp(ps_cb.MskFix);
		GSVector4 ta(env.TEXA & GSVector4i::x000000ff());

		ps_cb.MinMax = clamp / WH.xyxy();
		ps_cb.MinF_TA = (clamp + 0.5f).xyxy(ta) / WH.xyxy(GSVector4(255, 255));

		ps_ssel.tau = (context->CLAMP.WMS + 3) >> 1;
		ps_ssel.tav = (context->CLAMP.WMT + 3) >> 1;
		ps_ssel.ltf = bilinear && simple_sample;

		// Setup Texture ressources
		if (GLLoader::found_GL_ARB_bindless_texture) {
			GLuint64 handle[2];
			handle[0] = tex->m_texture ? static_cast<GSTextureOGL*>(tex->m_texture)->GetHandle(dev->GetSamplerID(ps_ssel)): 0;
			handle[1] = tex->m_palette ? static_cast<GSTextureOGL*>(tex->m_palette)->GetHandle(dev->GetPaletteSamplerID()): 0;

			dev->m_shader->PS_ressources(handle);
		} else {
			dev->SetupSampler(ps_ssel);

			if (tex->m_palette) {
				GLuint textures[2] = {static_cast<GSTextureOGL*>(tex->m_texture)->GetID(), static_cast<GSTextureOGL*>(tex->m_palette)->GetID()};
				dev->PSSetShaderResources(textures);
			} else if (tex->m_texture) {
				// Only main texture
				dev->PSSetShaderResource(static_cast<GSTextureOGL*>(tex->m_texture)->GetID());
			}
		}
	}

	// WARNING: setup of the program must be done first. So you can setup
	// 1/ subroutine uniform
	// 2/ bindless texture uniform
	// 3/ others uniform?
	dev->SetupVS(vs_sel);
	dev->SetupGS(m_vt.m_primclass == GS_SPRITE_CLASS);
	dev->SetupPS(ps_sel);

	// rs

	GSVector4i scissor = GSVector4i(GSVector4(rtscale).xyxy() * context->scissor.in).rintersect(GSVector4i(rtsize).zwxy());

	dev->OMSetRenderTargets(rt, ds, &scissor);

	uint8 afix = context->ALPHA.FIX;

	SetupIA();

	dev->SetupOM(om_dssel, om_bsel, afix);
	dev->SetupCB(&vs_cb, &ps_cb);

	if (advance_DATE) {
		// Create an r32i image that will contain primitive ID
		// Note: do it at the beginning because the clean will dirty the FBO state
		//dev->InitPrimDateTexture(rtsize.x, rtsize.y);

		// Don't write anything on the color buffer
		dev->OMSetWriteBuffer(GL_NONE);
		// Compute primitiveID max that pass the date test
		dev->DrawIndexedPrimitive();

		// Ask PS to discard shader above the primitiveID max
		dev->OMSetWriteBuffer();

		ps_sel.date = 3;
		dev->SetupPS(ps_sel);

		// Be sure that first pass is finished !
		dev->Barrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
	}

	if(context->TEST.DoFirstPass())
	{
		dev->DrawIndexedPrimitive();

		if (env.COLCLAMP.CLAMP == 0 && /* hack */ !tex && PRIM->PRIM != GS_POINTLIST)
		{
			GSDeviceOGL::OMBlendSelector om_bselneg(om_bsel);
			GSDeviceOGL::PSSelector ps_selneg(ps_sel);

			om_bselneg.negative = 1;
			ps_selneg.colclip = 2;

			dev->SetupOM(om_dssel, om_bselneg, afix);
			dev->SetupPS(ps_selneg);

			dev->DrawIndexedPrimitive();
			dev->SetupOM(om_dssel, om_bsel, afix);
		}
	}

	if(context->TEST.DoSecondPass())
	{
		ASSERT(!env.PABE.PABE);

		static const uint32 iatst[] = {1, 0, 5, 6, 7, 2, 3, 4};

		ps_sel.atst = iatst[ps_sel.atst];

		dev->SetupPS(ps_sel);

		bool z = om_dssel.zwe;
		bool r = om_bsel.wr;
		bool g = om_bsel.wg;
		bool b = om_bsel.wb;
		bool a = om_bsel.wa;

		switch(context->TEST.AFAIL)
		{
			case AFAIL_KEEP: z = r = g = b = a = false; break; // none
			case AFAIL_FB_ONLY: z = false; break; // rgba
			case AFAIL_ZB_ONLY: r = g = b = a = false; break; // z
			case AFAIL_RGB_ONLY: z = a = false; break; // rgb
			default: __assume(0);
		}

		if(z || r || g || b || a)
		{
			om_dssel.zwe = z;
			om_bsel.wr = r;
			om_bsel.wg = g;
			om_bsel.wb = b;
			om_bsel.wa = a;

			dev->SetupOM(om_dssel, om_bsel, afix);

			dev->DrawIndexedPrimitive();

			if (env.COLCLAMP.CLAMP == 0 && /* hack */ !tex && PRIM->PRIM != GS_POINTLIST)
			{
				GSDeviceOGL::OMBlendSelector om_bselneg(om_bsel);
				GSDeviceOGL::PSSelector ps_selneg(ps_sel);

				om_bselneg.negative = 1;
				ps_selneg.colclip = 2;

				dev->SetupOM(om_dssel, om_bselneg, afix);
				dev->SetupPS(ps_selneg);

				dev->DrawIndexedPrimitive();
			}
		}
	}
	if (advance_DATE)
		dev->RecycleDateTexture();

	dev->EndScene();

	if(om_dssel.fba) UpdateFBA(rt);
}

void GSRendererOGL::UpdateFBA(GSTexture* rt)
{
#ifdef ENABLE_GLES
#ifdef _DEBUG
	fprintf(stderr, "FIXME UpdateFBA not yet implemented\n");
#endif
#endif
}
