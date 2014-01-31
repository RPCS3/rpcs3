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
#include "GSDeviceOGL.h"
#include "GSTables.h"

static const uint32 g_vs_cb_index = 20;
static const uint32 g_ps_cb_index = 21;

void GSDeviceOGL::CreateTextureFX()
{
	m_vs_cb = new GSUniformBufferOGL(g_vs_cb_index, sizeof(VSConstantBuffer));
	m_ps_cb = new GSUniformBufferOGL(g_ps_cb_index, sizeof(PSConstantBuffer));

	// warning 1 sampler by image unit. So you cannot reuse m_ps_ss...
	m_palette_ss = CreateSampler(false, false, false);
	gl_BindSampler(1, m_palette_ss);

	GSInputLayoutOGL vert_format[] =
	{
		{0 , 2 , GL_FLOAT          , GL_FALSE , sizeof(GSVertex) , (const GLvoid*)(0) }  ,
		{1 , 4 , GL_UNSIGNED_BYTE  , GL_TRUE  , sizeof(GSVertex) , (const GLvoid*)(8) }  ,
		{2 , 1 , GL_FLOAT          , GL_FALSE , sizeof(GSVertex) , (const GLvoid*)(12) } ,
		{3 , 2 , GL_UNSIGNED_SHORT , GL_FALSE , sizeof(GSVertex) , (const GLvoid*)(16) } ,
		{4 , 1 , GL_UNSIGNED_INT   , GL_FALSE , sizeof(GSVertex) , (const GLvoid*)(20) } ,
		{5 , 2 , GL_UNSIGNED_SHORT , GL_FALSE , sizeof(GSVertex) , (const GLvoid*)(24) } ,
		{6 , 4 , GL_UNSIGNED_BYTE  , GL_TRUE  , sizeof(GSVertex) , (const GLvoid*)(28) } ,
	};
	m_vb = new GSVertexBufferStateOGL(sizeof(GSVertex), vert_format, countof(vert_format));

	// Pre compile all Geometry & Vertex Shader
	// It might cost a seconds at startup but it would reduce benchmark pollution
	m_gs = CompileGS();

	for (uint32 key = 0; key < VSSelector::size(); key++)
		m_vs[key] = CompileVS(VSSelector(key));

	for (uint32 key = 0; key < OMDepthStencilSelector::size(); key++)
		m_om_dss[key] = CreateDepthStencil(OMDepthStencilSelector(key));

	// Help to debug FS in apitrace
	m_apitrace = CompilePS(PSSelector());

	if (!!theApp.GetConfig("GL_NV_Depth", 0)) {
		gl_DepthRangedNV(-1.0f, 1.0f);
	}
}

GSDepthStencilOGL* GSDeviceOGL::CreateDepthStencil(OMDepthStencilSelector dssel)
{
	GSDepthStencilOGL* dss = new GSDepthStencilOGL();

	if (dssel.date)
	{
		dss->EnableStencil();
		dss->SetStencil(GL_EQUAL, dssel.alpha_stencil ? GL_ZERO : GL_KEEP);
	}

	if(dssel.ztst != ZTST_ALWAYS || dssel.zwe)
	{
		static const GLenum ztst[] =
		{
			GL_NEVER,
			GL_ALWAYS,
			GL_GEQUAL,
			GL_GREATER
		};
		dss->EnableDepth();
		dss->SetDepth(ztst[dssel.ztst], dssel.zwe);
	}

	return dss;
}

GSBlendStateOGL* GSDeviceOGL::CreateBlend(OMBlendSelector bsel, uint8 afix)
{
	GSBlendStateOGL* bs = new GSBlendStateOGL();

	if(bsel.abe)
	{
		int i = ((bsel.a * 3 + bsel.b) * 3 + bsel.c) * 3 + bsel.d;

		bs->EnableBlend();
		bs->SetRGB(m_blendMapD3D9[i].op, m_blendMapD3D9[i].src, m_blendMapD3D9[i].dst);

		if(m_blendMapD3D9[i].bogus == 1)
		{
			if (bsel.a == 0)
				bs->SetRGB(m_blendMapD3D9[i].op, GL_ONE, m_blendMapD3D9[i].dst);
			else
				bs->SetRGB(m_blendMapD3D9[i].op, m_blendMapD3D9[i].src, GL_ONE);

			const string afixstr = format("%d >> 7", afix);
			const char *col[3] = {"Cs", "Cd", "0"};
			const char *alpha[3] = {"As", "Ad", afixstr.c_str()};

			// FIXME, need to investigate OGL capabilities. Maybe for OGL5 ;)
			fprintf(stderr, "Impossible blend for D3D: (%s - %s) * %s + %s\n", col[bsel.a], col[bsel.b], alpha[bsel.c], col[bsel.d]);
		}

		// Not very good but I don't wanna write another 81 row table
		if(bsel.negative) bs->RevertOp();
	}

	bs->SetMask(bsel.wr, bsel.wg, bsel.wb, bsel.wa);

	return bs;
}

void GSDeviceOGL::SetupCB(const VSConstantBuffer* vs_cb, const PSConstantBuffer* ps_cb)
{
	if(m_vs_cb_cache.Update(vs_cb)) {
		m_vs_cb->upload(vs_cb);
	}

	if(m_ps_cb_cache.Update(ps_cb)) {
		m_ps_cb->upload(ps_cb);
	}

}

void GSDeviceOGL::SetupVS(VSSelector sel)
{
	if (GLLoader::found_GL_ARB_shader_subroutine) {
		GLuint sub[1];
		sub[0] = sel.tme ? 1 + (uint32)sel.fst : 0;
		m_shader->VS_subroutine(sub);
		// Handle by subroutine useless now
		sel.tme = 0;
		sel.fst = 0;
	}

	m_shader->VS(m_vs[sel], 1);
}

void GSDeviceOGL::SetupGS(bool enable)
{
	if (enable)
		m_shader->GS(m_gs);
	else
		m_shader->GS(0);
}

void GSDeviceOGL::SetupPS(PSSelector sel)
{
	if (GLLoader::found_GL_ARB_shader_subroutine) {
		GLuint tfx = sel.tfx > 3 ? 19 : 11 + (uint32)sel.tfx + (uint32)sel.tcc*4;

		GLuint colclip = 8 + (uint32)sel.colclip;

		GLuint clamp = 
			(sel.wms == 2 && sel.wmt == 2) ? 20 :
			(sel.wms == 2)                 ? 21 :
			(sel.wmt == 2)                 ? 22 : 23;

		GLuint wrap = 
			(sel.wms == 2 && sel.wmt == 2) ? 24 :
			(sel.wms == 3 && sel.wmt == 3) ? 25 :
			(sel.wms == 2 && sel.wmt == 3) ? 26 :
			(sel.wms == 3 && sel.wmt == 2) ? 27 :
			(sel.wms == 2)                 ? 28 :
			(sel.wmt == 3)                 ? 29 :
			(sel.wms == 3)                 ? 30 :
			(sel.wmt == 2)                 ? 31 : 32;

		GLuint sub[5] = {sel.atst, colclip, tfx, clamp, wrap};

		m_shader->PS_subroutine(sub);
		// Handle by subroutine useless now
		sel.atst = 0;
		sel.colclip = 0;
		sel.tfx = 0;
		sel.tcc = 0;
		// sel.wms = 0;
		// sel.wmt = 0;
	}

	// *************************************************************
	// Static
	// *************************************************************
	GLuint ps;
	auto i = m_ps.find(sel);

	if (i == m_ps.end()) {
		ps = CompilePS(sel);
		m_ps[sel] = ps;
	} else {
		ps = i->second;
	}

	// *************************************************************
	// Dynamic
	// *************************************************************
	m_shader->PS(ps, 3);
}

void GSDeviceOGL::SetupSampler(PSSamplerSelector ssel)
{
	PSSetSamplerState(m_ps_ss[ssel]);
}

GLuint GSDeviceOGL::GetSamplerID(PSSamplerSelector ssel)
{
	return m_ps_ss[ssel];
}

GLuint GSDeviceOGL::GetPaletteSamplerID()
{
	return m_palette_ss;
}

void GSDeviceOGL::SetupOM(OMDepthStencilSelector dssel, OMBlendSelector bsel, uint8 afix)
{
	GSDepthStencilOGL* dss = m_om_dss[dssel];

	OMSetDepthStencilState(dss, 1);

	// *************************************************************
	// Static
	// *************************************************************
	auto j = m_om_bs.find(bsel);
	GSBlendStateOGL* bs;

	if(j == m_om_bs.end())
	{
		bs = CreateBlend(bsel, afix);
		m_om_bs[bsel] = bs;
	} else {
		bs = j->second;
	}

	// *************************************************************
	// Dynamic
	// *************************************************************
	OMSetBlendState(bs, (float)(int)afix / 0x80);
}
