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
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "stdafx.h"
#include "GSDeviceOGL.h"
#include "GSTables.h"

void GSDeviceOGL::CreateTextureFX()
{
	m_vs_cb = new GSUniformBufferOGL(4, sizeof(VSConstantBuffer));
	m_ps_cb = new GSUniformBufferOGL(5, sizeof(PSConstantBuffer));

	glGenSamplers(1, &m_rt_ss);
	// FIXME, seem to have no difference between sampler !!!
	m_palette_ss = m_rt_ss;

	glSamplerParameteri(m_rt_ss, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glSamplerParameteri(m_rt_ss, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glSamplerParameteri(m_rt_ss, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glSamplerParameteri(m_rt_ss, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glSamplerParameteri(m_rt_ss, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	// FIXME which value for GL_TEXTURE_MIN_LOD
	glSamplerParameteri(m_rt_ss, GL_TEXTURE_MAX_LOD, FLT_MAX);
	// FIXME: seems there is 2 possibility in opengl
	// DX: sd.ComparisonFunc = D3D11_COMPARISON_NEVER;
	// glSamplerParameteri(m_rt_ss, GL_TEXTURE_COMPARE_MODE, GL_NONE);
	glSamplerParameteri(m_rt_ss, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
	glSamplerParameteri(m_rt_ss, GL_TEXTURE_COMPARE_FUNC, GL_NEVER);
	// FIXME: need ogl extension sd.MaxAnisotropy = 16;

	//{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
	//{"TEXCOORD", 1, DXGI_FORMAT_R32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
	//float2 t : TEXCOORD0;
	//float q : TEXCOORD1;
	//
	//{"POSITION", 0, DXGI_FORMAT_R16G16_UINT, 0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0},
	//{"POSITION", 1, DXGI_FORMAT_R32_UINT, 0, 20, D3D11_INPUT_PER_VERTEX_DATA, 0},
	//uint2 p : POSITION0;
	//uint z : POSITION1;
	//
	//{"COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 8, D3D11_INPUT_PER_VERTEX_DATA, 0},
	//{"COLOR", 1, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 28, D3D11_INPUT_PER_VERTEX_DATA, 0},
	//float4 c : COLOR0;
	//float4 f : COLOR1;

	GSInputLayoutOGL vert_format[] =
	{
		// FIXME
		{0 , 2 , GL_FLOAT          , GL_FALSE , sizeof(GSVertex) , (const GLvoid*)(0) }  ,
		{1 , 4 , GL_UNSIGNED_BYTE  , GL_TRUE  , sizeof(GSVertex) , (const GLvoid*)(8) }  ,
		{2 , 1 , GL_FLOAT          , GL_FALSE , sizeof(GSVertex) , (const GLvoid*)(12) } ,
		{3 , 2 , GL_UNSIGNED_SHORT , GL_FALSE , sizeof(GSVertex) , (const GLvoid*)(16) } ,
		{4 , 1 , GL_UNSIGNED_INT   , GL_FALSE , sizeof(GSVertex) , (const GLvoid*)(20) } ,
		// note: there is a 32 bits pad
		{5 , 2 , GL_UNSIGNED_SHORT , GL_FALSE , sizeof(GSVertex) , (const GLvoid*)(24) } ,
		{6 , 4 , GL_UNSIGNED_BYTE  , GL_TRUE  , sizeof(GSVertex) , (const GLvoid*)(28) } ,
	};
	m_vb = new GSVertexBufferStateOGL(sizeof(GSVertex), vert_format, countof(vert_format));
}

void GSDeviceOGL::SetupVS(VSSelector sel, const VSConstantBuffer* cb)
{
	// *************************************************************
	// Static
	// *************************************************************
	auto i = m_vs.find(sel);

	if(i == m_vs.end())
	{
		std::string macro = format("#define VS_BPPZ %d\n", sel.bppz)
			+ format("#define VS_TME %d\n", sel.tme)
			+ format("#define VS_FST %d\n", sel.fst)
			+ format("#define VS_RTCOPY %d\n", sel.rtcopy);

		GLuint vs;
		CompileShaderFromSource("tfx.glsl", "vs_main", GL_VERTEX_SHADER, &vs, macro);

		m_vs[sel] = vs;
		i = m_vs.find(sel);
	}

	// *************************************************************
	// Dynamic
	// *************************************************************
	if(m_vs_cb_cache.Update(cb)) {
		SetUniformBuffer(m_vs_cb);
		m_vs_cb->upload(cb);
	}

	VSSetShader(i->second);
}

void GSDeviceOGL::SetupGS(GSSelector sel)
{
	// *************************************************************
	// Static
	// *************************************************************
	GLuint gs = 0;
#ifdef AMD_DRIVER_WORKAROUND
	if (true)
#else
	if(sel.prim > 0 && (sel.iip == 0 || sel.prim == 3))
#endif
	{
		auto i = m_gs.find(sel);

		if(i == m_gs.end()) {
			std::string macro = format("#define GS_IIP %d\n", sel.iip)
				+ format("#define GS_PRIM %d\n", sel.prim);

			CompileShaderFromSource("tfx.glsl", "gs_main", GL_GEOMETRY_SHADER, &gs, macro);

			m_gs[sel] = gs;
		} else {
			gs = i->second;
		}
	}
	// *************************************************************
	// Dynamic
	// *************************************************************
	GSSetShader(gs);
}

void GSDeviceOGL::SetupPS(PSSelector sel, const PSConstantBuffer* cb, PSSamplerSelector ssel)
{
	// *************************************************************
	// Static
	// *************************************************************
	GLuint ps;
	auto i = m_ps.find(sel);

	if (i == m_ps.end())
	{
		std::string macro = format("#define PS_FST %d\n", sel.fst)
			+ format("#define PS_WMS %d\n", sel.wms)
			+ format("#define PS_WMT %d\n", sel.wmt)
			+ format("#define PS_FMT %d\n", sel.fmt)
			+ format("#define PS_AEM %d\n", sel.aem)
			+ format("#define PS_TFX %d\n", sel.tfx)
			+ format("#define PS_TCC %d\n", sel.tcc)
			+ format("#define PS_ATST %d\n", sel.atst)
			+ format("#define PS_FOG %d\n", sel.fog)
			+ format("#define PS_CLR1 %d\n", sel.clr1)
			+ format("#define PS_FBA %d\n", sel.fba)
			+ format("#define PS_AOUT %d\n", sel.aout)
			+ format("#define PS_LTF %d\n", sel.ltf)
			+ format("#define PS_COLCLIP %d\n", sel.colclip)
			+ format("#define PS_DATE %d\n", sel.date)
			+ format("#define PS_SPRITEHACK %d\n", sel.spritehack);

		CompileShaderFromSource("tfx.glsl", "ps_main", GL_FRAGMENT_SHADER, &ps, macro);

		m_ps[sel] = ps;
		i = m_ps.find(sel);
	} else {
		ps = i->second;
	}

	// *************************************************************
	// Dynamic
	// *************************************************************
	if(m_ps_cb_cache.Update(cb)) {
		SetUniformBuffer(m_ps_cb);
		m_ps_cb->upload(cb);
	}

	GLuint ss0, ss1;
	ss0 = ss1 = 0;
	if(sel.tfx != 4)
	{
		if(!(sel.fmt < 3 && sel.wms < 3 && sel.wmt < 3))
		{
			ssel.ltf = 0;
		}

		auto i = m_ps_ss.find(ssel);

		if(i != m_ps_ss.end())
		{
			ss0 = i->second;
		}
		else
		{
			// *************************************************************
			// Static
			// *************************************************************
			glGenSamplers(1, &ss0);
			if (ssel.ltf) {
				glSamplerParameteri(ss0, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glSamplerParameteri(ss0, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			} else {
				glSamplerParameteri(ss0, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
				glSamplerParameteri(ss0, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			}

			// FIXME ensure U -> S,  V -> T and W->R
			if (ssel.tau)
				glSamplerParameteri(ss0, GL_TEXTURE_WRAP_S, GL_REPEAT);
			else
				glSamplerParameteri(ss0, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);

			if (ssel.tav)
				glSamplerParameteri(ss0, GL_TEXTURE_WRAP_T, GL_REPEAT);
			else
				glSamplerParameteri(ss0, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

			glSamplerParameteri(ss0, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

			// FIXME which value for GL_TEXTURE_MIN_LOD
			glSamplerParameteri(m_rt_ss, GL_TEXTURE_MAX_LOD, FLT_MAX);
			// FIXME: seems there is 2 possibility in opengl
			// DX: sd.ComparisonFunc = D3D11_COMPARISON_NEVER;
			// glSamplerParameteri(m_rt_ss, GL_TEXTURE_COMPARE_MODE, GL_NONE);
			glSamplerParameteri(m_rt_ss, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
			glSamplerParameteri(m_rt_ss, GL_TEXTURE_COMPARE_FUNC, GL_NEVER);
			// FIXME: need ogl extension sd.MaxAnisotropy = 16;

			m_ps_ss[ssel] = ss0;
		}

		if(sel.fmt >= 3)
		{
			ss1 = m_palette_ss;
		}
	}

	PSSetSamplerState(ss0, ss1, sel.date ? m_rt_ss : 0);

	PSSetShader(ps);
}

void GSDeviceOGL::SetupOM(OMDepthStencilSelector dssel, OMBlendSelector bsel, uint8 afix)
{
	auto i = m_om_dss.find(dssel);

	// *************************************************************
	// Static
	// *************************************************************
	if (i == m_om_dss.end())
	{
		GSDepthStencilOGL* dss = new GSDepthStencilOGL();

		if (dssel.date)
		{
			dss->EnableStencil();
			dss->SetStencil(GL_EQUAL, GL_KEEP);
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

		m_om_dss[dssel] = dss;
		i = m_om_dss.find(dssel);
	}

	// *************************************************************
	// Dynamic
	// *************************************************************
	OMSetDepthStencilState(i->second, 1);

	// *************************************************************
	// Static
	// *************************************************************
	auto j = m_om_bs.find(bsel);

	if(j == m_om_bs.end())
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

		m_om_bs[bsel] = bs;
		j = m_om_bs.find(bsel);
	}

	// *************************************************************
	// Dynamic
	// *************************************************************
	OMSetBlendState(j->second, (float)(int)afix / 0x80);
}
