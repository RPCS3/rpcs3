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

#include <stdlib.h>
#include <math.h>

#include "GS.h"
#include "Mem.h"
#include "x86.h"
#include "targets.h"
#include "ZZoglShaders.h"
#include "ZZClut.h"
#include "ZZoglVB.h"

#ifdef ZEROGS_SSE2
#include <immintrin.h>
#endif

extern int g_TransferredToGPU;
extern int s_nResolved;

void _Resolve(const void* psrc, int fbp, int fbw, int fbh, int psm, u32 fbm, bool mode);
void SetWriteDepth();
bool IsWriteDepth();
bool IsWriteDestAlphaTest();



// Draw 4 triangles from binded array using only stencil buffer
inline void FillOnlyStencilBuffer()
{
	if (IsWriteDestAlphaTest() && !(conf.settings().no_stencil))
	{
		glColorMask(0, 0, 0, 0);
		glEnable(GL_ALPHA_TEST);
		glAlphaFunc(GL_GEQUAL, 1.0f);

		glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
		glStencilFunc(GL_ALWAYS, 1, 0xff);

		DrawTriangleArray();
		glColorMask(1, 1, 1, 1);
	}
}

inline void BindToSample(u32 *p_ptr)
{
	glBindTexture(GL_TEXTURE_RECTANGLE_NV, *p_ptr);
	setRectFilters(GL_NEAREST);
}

// Made an empty texture and bind it to $ptr_p
// returns false if creating texture was unsuccessful
// fbh and fdb should be properly shifted before calling this!
// We should ignore framebuffer trouble here, as we put textures of different sizes to it.
inline bool CRenderTarget::InitialiseDefaultTexture(u32 *ptr_p, int fbw, int fbh)
{
	glGenTextures(1, ptr_p);
	glBindTexture(GL_TEXTURE_RECTANGLE_NV, *ptr_p);

	// initialize to default
	TextureRect(GL_RGBA, fbw, fbh, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

	setRectWrap(GL_CLAMP);
	setRectFilters(GL_LINEAR);

	GLenum Error = glGetError();
	return ((Error == GL_NO_ERROR) || (Error == GL_INVALID_FRAMEBUFFER_OPERATION_EXT));
}

// used for transformation from vertex position in GS window.coords (I hope)
// to view coordinates (in range 0, 1).
float4 CRenderTarget::DefaultBitBltPos()
{
	float4 v = float4(1, -1, 0.5f / (float)RW(fbw), 0.5f / (float)RH(fbh));
	v *= 1.0f / 32767.0f;
	ZZshSetParameter4fv(pvsBitBlt.prog, pvsBitBlt.sBitBltPos, v, "g_sBitBltPos");
	return v;
}

// Used to transform texture coordinates from GS (when 0,0 is upper left) to
// OpenGL (0,0 - lower left).
float4 CRenderTarget::DefaultBitBltTex()
{
	// I really sure that -0.5 is correct, because OpenGL have no half-offset
	// issue, DirectX known for.
	float4 v = float4(1, -1, 0.5f / (float)RW(fbw), -0.5f / (float)RH(fbh));
	ZZshSetParameter4fv(pvsBitBlt.prog, pvsBitBlt.sBitBltTex, v, "g_sBitBltTex");
	return v;
}


////////////////////
// Render Targets //
////////////////////
CRenderTarget::CRenderTarget() : ptex(0), ptexFeedback(0), psys(NULL)
{
	FUNCLOG
	nUpdateTarg = 0;
}

CRenderTarget::~CRenderTarget()
{
	FUNCLOG
	Destroy();
}

bool CRenderTarget::Create(const frameInfo& frame)
{
	FUNCLOG
	Resolve();
	Destroy();
	created = 123;

	lastused = timeGetTime();
	fbp = frame.fbp;
	fbw = frame.fbw;
	fbh = frame.fbh;
	psm = (u8)frame.psm;
	fbm = frame.fbm;

	vposxy.x = 2.0f * (1.0f / 8.0f) / (float)fbw;
	vposxy.y = 2.0f * (1.0f / 8.0f) / (float)fbh;
	vposxy.z = -1.0f - 0.5f / (float)fbw;
	vposxy.w = -1.0f + 0.5f / (float)fbh;
	status = 0;

	if (fbw > 0 && fbh > 0)
	{
		GetRectMemAddressZero(start, end, psm, fbw, fbh, fbp, fbw);
		psys = _aligned_malloc(Tex_Memory_Size(fbw, fbh), 16);

		GL_REPORT_ERRORD();

		if (!InitialiseDefaultTexture(&ptex, RW(fbw), RH(fbh)))
		{
			Destroy();
			return false;
		}

		status = TS_NeedUpdate;
	}
	else
	{
		start = end = 0;
	}

	return true;
}

void CRenderTarget::Destroy()
{
	FUNCLOG
	created = 1;
	_aligned_free(psys);
	psys = NULL;
	SAFE_RELEASE_TEX(ptex);
	SAFE_RELEASE_TEX(ptexFeedback);
}

void CRenderTarget::SetTarget(int fbplocal, const Rect2& scissor, int context)
{
	FUNCLOG
	int dy = 0;

	if (fbplocal != fbp)
	{
		float4 v;

		// will be rendering to a subregion
		u32 bpp = PSMT_ISHALF(psm) ? 2 : 4;
		assert(((256 / bpp)*(fbplocal - fbp)) % fbw == 0);
		assert(fbplocal >= fbp);

		dy = ((256 / bpp) * (fbplocal - fbp)) / fbw;

		v.x = vposxy.x;
		v.y = vposxy.y;
		v.z = vposxy.z;
		v.w = vposxy.w - dy * 2.0f / (float)fbh;
		ZZshSetParameter4fv(g_vparamPosXY[context], v, "g_fPosXY");
	}
	else
	{
		ZZshSetParameter4fv(g_vparamPosXY[context], vposxy, "g_fPosXY");
	}

	// set render states
	// Bleh. I *really* need to fix this. << 3 when setting the scissors, then >> 3 when using them... --Arcum42
	scissorrect.x = scissor.x0 >> 3;
	scissorrect.y = (scissor.y0 >> 3) + dy;
	scissorrect.w = (scissor.x1 >> 3) + 1;
	scissorrect.h = (scissor.y1 >> 3) + 1 + dy;

	scissorrect.w = min(scissorrect.w, fbw) - scissorrect.x;
	scissorrect.h = min(scissorrect.h, fbh) - scissorrect.y;

	scissorrect.x = RW(scissorrect.x);
	scissorrect.y = RH(scissorrect.y);
	scissorrect.w = RW(scissorrect.w);
	scissorrect.h = RH(scissorrect.h);
}

void CRenderTarget::SetViewport()
{
	FUNCLOG
	glViewport(0, 0, RW(fbw), RH(fbh));
}

inline bool NotResolveHelper()
{
	return ((s_nResolved > 8 && (2 * s_nResolved > fFPS - 10)) || (conf.settings().no_target_resolve));
}

void CRenderTarget::Resolve()
{
	FUNCLOG

	if (ptex != 0 && !(status&TS_Resolved) && !(status&TS_NeedUpdate))
	{
		// flush if necessary
		FlushIfNecesary(this) ;

		if ((IsDepth() && !IsWriteDepth()) || NotResolveHelper())
		{
			// don't resolve if depths aren't used
			status = TS_Resolved;
			return;
		}

		glBindTexture(GL_TEXTURE_RECTANGLE_NV, ptex);

		GL_REPORT_ERRORD();
		// This code extremely slow on DC1.
//		_aligned_free(psys);
//		psys = _aligned_malloc( Tex_Memory_Size ( fbw, fbh ), 16 );

		glGetTexImage(GL_TEXTURE_RECTANGLE_NV, 0, GL_RGBA, GL_UNSIGNED_BYTE, psys);

		GL_REPORT_ERRORD();

#if defined(ZEROGS_DEVBUILD)

		if (g_bSaveResolved)
		{
			SaveTexture("resolved.tga", GL_TEXTURE_RECTANGLE_NV, ptex, RW(fbw), RH(fbh));
			g_bSaveResolved = 0;
		}

#endif
		_Resolve(psys, fbp, fbw, fbh, psm, fbm, true);

		status = TS_Resolved;
	}
}

void CRenderTarget::Resolve(int startrange, int endrange)
{
	FUNCLOG

	assert(startrange < end && endrange > start);   // make sure it at least intersects

	if (ptex != 0 && !(status&TS_Resolved) && !(status&TS_NeedUpdate))
	{
		// flush if necessary
		FlushIfNecesary(this) ;

#if defined(ZEROGS_DEVBUILD)
		if (g_bSaveResolved)
		{
			SaveTexture("resolved.tga", GL_TEXTURE_RECTANGLE_NV, ptex, RW(fbw), RH(fbh));
			g_bSaveResolved = 0;
		}
#endif
		if (conf.settings().no_target_resolve)
		{
			status = TS_Resolved;
			return;
		}

		int blockheight = PSMT_ISHALF(psm) ? 64 : 32;
		int resolvefbp = fbp, resolveheight = fbh;
		int scanlinewidth = 0x2000 * (fbw >> 6);

		// in no way should data be overwritten!, instead resolve less

		if (endrange < end)
		{
			// round down to nearest block and scanline
			resolveheight = ((endrange - start) / (0x2000 * (fbw >> 6))) * blockheight;

			if (resolveheight <= 32)
			{
				status = TS_Resolved;
				return;
			}
		}
		else if (startrange > start)
		{
			// round up to nearest block and scanline
			resolvefbp = startrange + scanlinewidth - 1;
			resolvefbp -= resolvefbp % scanlinewidth;

			resolveheight = fbh - ((resolvefbp - fbp) * blockheight / scanlinewidth);

			if (resolveheight <= 64)    // this is a total hack, but kh doesn't resolve now
			{
				status = TS_Resolved;
				return;
			}

			resolvefbp >>= 8;
		}

		glBindTexture(GL_TEXTURE_RECTANGLE_NV, ptex);

		glGetTexImage(GL_TEXTURE_RECTANGLE_NV, 0, GL_RGBA, GL_UNSIGNED_BYTE, psys);
		GL_REPORT_ERRORD();

		u8* pbits = (u8*)psys;

		if (fbp != resolvefbp) pbits += ((resolvefbp - fbp) * 256 / scanlinewidth) * blockheight * Pitch(fbw);

		_Resolve(pbits, resolvefbp, fbw, resolveheight, psm, fbm, true);

		status = TS_Resolved;
	}
}

void CRenderTarget::Update(int context, CRenderTarget* pdepth)
{
	FUNCLOG

	DisableAllgl();

	glBindBuffer(GL_ARRAY_BUFFER, vboRect);
	SET_STREAM();

	// assume depth already set
	//pd3dDevice->SetDepthStencilSurface(psurfDepth);
	ResetRenderTarget(1);
	SetRenderTarget(0);
	assert(pdepth != NULL);
	((CDepthTarget*)pdepth)->SetDepthStencilSurface();

	SetShaderCaller("CRenderTarget::Update");
	float4 v = DefaultBitBltPos();

	CRenderTargetMngr::MAPTARGETS::iterator ittarg;

	if (nUpdateTarg)
	{
		ittarg = s_RTs.mapTargets.find(nUpdateTarg);

		if (ittarg == s_RTs.mapTargets.end())
		{
			ittarg = s_DepthRTs.mapTargets.find(nUpdateTarg);

			if (ittarg == s_DepthRTs.mapTargets.end())
				nUpdateTarg = 0;
			else if (ittarg->second == this)
			{
				ZZLog::Debug_Log("Updating self.");
				nUpdateTarg = 0;
			}
		}
		else if (ittarg->second == this)
		{
			ZZLog::Debug_Log("Updating self.");
			nUpdateTarg = 0;
		}
	}

	SetViewport();

	if (nUpdateTarg)
	{
		ZZshGLSetTextureParameter(ppsBaseTexture.prog, ppsBaseTexture.sFinal, ittarg->second->ptex, "BaseTexture.final");

		//assert( ittarg->second->fbw == fbw );
		int offset = (fbp - ittarg->second->fbp) * 64 / fbw;

		if (PSMT_ISHALF(psm)) // 16 bit
			offset *= 2;

		v.x = (float)RW(fbw);
		v.y = (float)RH(fbh);
		v.z = 0.25f;
		v.w = (float)RH(offset) + 0.25f;

		ZZshSetParameter4fv(pvsBitBlt.prog, pvsBitBlt.sBitBltTex, v, "g_fBitBltTex");

//		v = DefaultBitBltTex(); Maybe?
		ZZshDefaultOneColor ( ppsBaseTexture );

		ZZshSetPixelShader(ppsBaseTexture.prog);

		nUpdateTarg = 0;
	}
	else
	{
		u32 bit_idx = (AA.x == 0) ? 0 : 1;

		// align the rect to the nearest page
		// note that fbp is always aligned on page boundaries
		tex0Info texframe;
		texframe.tbp0 = fbp;
		texframe.tbw = fbw;
		texframe.tw = fbw;
		texframe.th = fbh;
		texframe.psm = psm;
        // FIXME some field are not initialized...
        // in particular the clut related one
        assert(!PSMT_ISCLUT(psm));

		// write color and zero out stencil buf, always 0 context!
		// force bilinear if using AA
		// Fix in r133 -- FFX movies and Gust backgrounds!
		//SetTexVariablesInt(0, 0*(AA.x || AA.y) ? 2 : 0, texframe, false, &ppsBitBlt[!!s_AAx], 1);
		SetTexVariablesInt(0, 0, texframe, false, &ppsBitBlt[bit_idx], 1);
		ZZshGLSetTextureParameter(ppsBitBlt[bit_idx].prog, ppsBitBlt[bit_idx].sMemory, vb[0].pmemtarg->ptex->tex, "BitBlt.memory");

		v = float4(1, 1, 0.0f, 0.0f);
		ZZshSetParameter4fv(pvsBitBlt.prog, pvsBitBlt.sBitBltTex, v, "g_fBitBltTex");

		v.x = 1;
		v.y = 2;
		ZZshSetParameter4fv(ppsBitBlt[bit_idx].prog, ppsBitBlt[bit_idx].sOneColor, v, "g_fOneColor");

		assert(ptex != 0);

		if (conf.wireframe()) glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		if (IsWriteDestAlphaTest())
		{
			glEnable(GL_STENCIL_TEST);
			glStencilFunc(GL_ALWAYS, 0, 0xff);
			glStencilMask(0xff);
			glStencilOp(GL_KEEP, GL_KEEP, GL_ZERO);
		}

		// render with an AA shader if possible (bilinearly interpolates data)
		//cgGLLoadProgram(ppsBitBlt[bit_idx].prog);
		ZZshSetPixelShader(ppsBitBlt[bit_idx].prog);
	}

	ZZshSetVertexShader(pvsBitBlt.prog);

	DrawTriangleArray();

	// fill stencil buf only
	FillOnlyStencilBuffer();
	glEnable(GL_SCISSOR_TEST);

	if (conf.wireframe()) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	if (conf.mrtdepth && pdepth != NULL && IsWriteDepth()) pdepth->SetRenderTarget(1);

	status = TS_Resolved;

	// reset since settings changed
	vb[0].bVarsTexSync = 0;

//	ResetAlphaVariables();
}

void CRenderTarget::ConvertTo32()
{
	FUNCLOG

	u32 ptexConv;
//	ZZLog::Error_Log("Convert to 32, report if something missing.");
	// create new target

	if (! InitialiseDefaultTexture(&ptexConv, RW(fbw), RH(fbh) / 2))
	{
		ZZLog::Error_Log("Failed to create target for ConvertTo32 %dx%d.", RW(fbw), RH(fbh) / 2);
		return;
	}

	DisableAllgl();

	SetShaderCaller("CRenderTarget::ConvertTo32");

	// tex coords, test ffx bikanel island when changing these
	float4 v = DefaultBitBltPos();
	v = DefaultBitBltTex();

	v.x = (float)RW(16);
	v.y = (float)RH(16);
	v.z = -(float)RW(fbw);
	v.w = (float)RH(8);
	ZZshSetParameter4fv(ppsConvert16to32.prog, ppsConvert16to32.fTexOffset, v, "g_fTexOffset");

	v.x = (float)RW(8);
	v.y = 0;
	v.z = 0;
	v.w = 0.25f;
	ZZshSetParameter4fv(ppsConvert16to32.prog, ppsConvert16to32.fPageOffset, v, "g_fPageOffset");

	v.x = (float)RW(2 * fbw);
	v.y = (float)RH(fbh);
	v.z = 0;
	v.w = 0.0001f * (float)RH(fbh);
	ZZshSetParameter4fv(ppsConvert16to32.prog, ppsConvert16to32.fTexDims, v, "g_fTexDims");

//	v.x = 0;
//	ZZshSetParameter4fv(ppsConvert16to32.fTexBlock, v, "g_fTexBlock");

	glBindBuffer(GL_ARRAY_BUFFER, vboRect);
	SET_STREAM();

	// assume depth already set !?
	FB::Attach2D(0, ptexConv);
	ResetRenderTarget(1);

	BindToSample(&ptex);
	ZZshGLSetTextureParameter(ppsConvert16to32.prog, ppsConvert16to32.sFinal, ptex, "Convert 16 to 32.Final");

	fbh /= 2; // have 16 bit surfaces are usually 2x higher
	SetViewport();

	if (conf.wireframe()) glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	// render with an AA shader if possible (bilinearly interpolates data)
	ZZshSetVertexShader(pvsBitBlt.prog);
	ZZshSetPixelShader(ppsConvert16to32.prog);
	DrawTriangleArray();

#ifdef _DEBUG
	if (g_bSaveZUpdate)
	{
		// buggy
		SaveTexture("tex1.tga", GL_TEXTURE_RECTANGLE_NV, ptex, RW(fbw), RH(fbh)*2);
		SaveTexture("tex3.tga", GL_TEXTURE_RECTANGLE_NV, ptexConv, RW(fbw), RH(fbh));
	}

#endif

	vposxy.y = -2.0f * (32767.0f / 8.0f) / (float)fbh;
	vposxy.w = 1 + 0.5f / fbh;

	// restore
	SAFE_RELEASE_TEX(ptex);
	SAFE_RELEASE_TEX(ptexFeedback);

	ptex = ptexConv;

	// no need to free psys since the render target is getting shrunk
	if (conf.wireframe()) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	// reset textures
	BindToSample(&ptex);

	glEnable(GL_SCISSOR_TEST);

	status = TS_Resolved;

	// TODO, reset depth?
	if (icurctx >= 0)
	{
		// reset since settings changed
		vb[icurctx].bVarsTexSync = 0;
		vb[icurctx].bVarsSetTarg = 0;
	}

	vb[0].bVarsTexSync = 0;
}

void CRenderTarget::ConvertTo16()
{
	FUNCLOG

	u32 ptexConv;

//	ZZLog::Error_Log("Convert to 16, report if something missing.");
	// create new target

	if (! InitialiseDefaultTexture(&ptexConv, RW(fbw), RH(fbh)*2))
	{
		ZZLog::Error_Log("Failed to create target for ConvertTo16 %dx%d.", RW(fbw), RH(fbh)*2);
		return;
	}

	DisableAllgl();

	SetShaderCaller("CRenderTarget::ConvertTo16");

	// tex coords, test ffx bikanel island when changing these
	float4 v = DefaultBitBltPos();
	v = DefaultBitBltTex();

	v.x = 16.0f / (float)fbw;
	v.y = 8.0f / (float)fbh;
	v.z = 0.5f * v.x;
	v.w = 0.5f * v.y;
	ZZshSetParameter4fv(ppsConvert32to16.prog, ppsConvert32to16.fTexOffset, v, "g_fTexOffset");

	v.x = 256.0f / 255.0f;
	v.y = 256.0f / 255.0f;
	v.z = 0.05f / 256.0f;
	v.w = -0.001f / 256.0f;
	ZZshSetParameter4fv(ppsConvert32to16.prog, ppsConvert32to16.fPageOffset, v, "g_fPageOffset");

	v.x = (float)RW(fbw);
	v.y = (float)RH(2 * fbh);
	v.z = 0;
	v.w = -0.1f / RH(fbh);
	ZZshSetParameter4fv(ppsConvert32to16.prog, ppsConvert32to16.fTexDims, v, "g_fTexDims");

	glBindBuffer(GL_ARRAY_BUFFER, vboRect);
	SET_STREAM();

	// assume depth already set !?
	FB::Attach2D(0, ptexConv);
	ResetRenderTarget(1);
	GL_REPORT_ERRORD();

	BindToSample(&ptex);

	ZZshGLSetTextureParameter(ppsConvert32to16.prog, ppsConvert32to16.sFinal, ptex, "Convert 32 to 16");

//	fbh *= 2; // have 16 bit surfaces are usually 2x higher

	SetViewport();

	if (conf.wireframe()) glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	// render with an AA shader if possible (bilinearly interpolates data)
	ZZshSetVertexShader(pvsBitBlt.prog);
	ZZshSetPixelShader(ppsConvert32to16.prog);
	DrawTriangleArray();

#ifdef _DEBUG
	//g_bSaveZUpdate = 1;
	if (g_bSaveZUpdate)
	{
		SaveTexture("tex1.tga", GL_TEXTURE_RECTANGLE_NV, ptexConv, RW(fbw), RH(fbh));
	}

#endif

	vposxy.y = -2.0f * (32767.0f / 8.0f) / (float)fbh;
	vposxy.w = 1 + 0.5f / fbh;

	// restore
	SAFE_RELEASE_TEX(ptex);
	SAFE_RELEASE_TEX(ptexFeedback);

	ptex = ptexConv;

	_aligned_free(psys);

	psys = _aligned_malloc(Tex_Memory_Size(fbw, fbh), 16);

	if (conf.wireframe()) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	// reset textures
	BindToSample(&ptex) ;

	glEnable(GL_SCISSOR_TEST);

	status = TS_Resolved;

	// TODO, reset depth?
	if (icurctx >= 0)
	{
		// reset since settings changed
		vb[icurctx].bVarsTexSync = 0;
		vb[icurctx].bVarsSetTarg = 0;
	}

	vb[0].bVarsTexSync = 0;
}

void CRenderTarget::_CreateFeedback()
{
	FUNCLOG

	if (ptexFeedback == 0)
	{
		// create
		if (! InitialiseDefaultTexture(&ptexFeedback, RW(fbw), RH(fbh)))
		{
			ZZLog::Error_Log("Failed to create feedback %dx%d.", RW(fbw), RH(fbh));
			return;
		}
	}

	DisableAllgl();

	SetShaderCaller("CRenderTarget::_CreateFeedback");

	// assume depth already set
	ResetRenderTarget(1);

	// tex coords, test ffx bikanel island when changing these
	/*	float4 v = DefaultBitBltPos();
		v = float4 ((float)(RW(fbw+4)), (float)(RH(fbh+4)), +0.25f, -0.25f);
		ZZshSetParameter4fv(pvsBitBlt.prog, pvsBitBlt.sBitBltTex, v, "BitBltTex");*/

	// tex coords, test ffx bikanel island when changing these

//	float4 v = float4(1, -1, 0.5f / (fbw << AA.x), 0.5f / (fbh << AA.y));
//	v *= 1/32767.0f;
//	cgGLSetParameter4fv(pvsBitBlt.sBitBltPos, v);
	float4 v = DefaultBitBltPos();

	v.x = (float)(RW(fbw));
	v.y = (float)(RH(fbh));
	v.z = 0.0f;
	v.w = 0.0f;
	ZZshSetParameter4fv(pvsBitBlt.prog, pvsBitBlt.sBitBltTex, v, "BitBlt.Feedback");
	ZZshDefaultOneColor(ppsBaseTexture);

	glBindBuffer(GL_ARRAY_BUFFER, vboRect);
	SET_STREAM();

	FB::Attach2D(0, ptexFeedback);
	glBindTexture(GL_TEXTURE_RECTANGLE_NV, ptex);
	GL_REPORT_ERRORD();

	ZZshGLSetTextureParameter(ppsBaseTexture.prog, ppsBaseTexture.sFinal, ptex, "BaseTexture.Feedback");

	SetViewport();

	if (conf.wireframe()) glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	// render with an AA shader if possible (bilinearly interpolates data)
	ZZshSetVertexShader(pvsBitBlt.prog);
	ZZshSetPixelShader(ppsBaseTexture.prog);
	DrawTriangleArray();

	// restore
	swap(ptex, ptexFeedback);

	if (conf.wireframe()) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	glEnable(GL_SCISSOR_TEST);

	status |= TS_FeedbackReady;

	// TODO, reset depth?
	if (icurctx >= 0)
	{
		// reset since settings changed
		vb[icurctx].bVarsTexSync = 0;
	}

	GL_REPORT_ERRORD();
}

void CRenderTarget::SetRenderTarget(int targ)
{
	FUNCLOG

	FB::Attach2D(targ, ptex);

	//GL_REPORT_ERRORD();
	//if (glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT) != GL_FRAMEBUFFER_COMPLETE_EXT)
		//ERROR_LOG_SPAM("The Framebuffer is not complete. Glitches could appear onscreen.\n");
}

void CRenderTargetMngr::Destroy()
{
	FUNCLOG

	for (MAPTARGETS::iterator it = mapTargets.begin(); it != mapTargets.end(); ++it)
	{
		delete it->second;
	}

	mapTargets.clear();

	for (MAPTARGETS::iterator it = mapDummyTargs.begin(); it != mapDummyTargs.end(); ++it)
	{
		delete it->second;
	}

	mapDummyTargs.clear();
}

// This block was repeated several times, so I inlined it.
void CRenderTargetMngr::DestroyAllTargetsHelper(void* ptr)
{
	for (int i = 0; i < 2; ++i)
	{
		if (ptr == vb[i].prndr) { vb[i].prndr = NULL; vb[i].bNeedFrameCheck = 1; }
		if (ptr == vb[i].pdepth) { vb[i].pdepth = NULL; vb[i].bNeedZCheck = 1; }
	}
}

void CRenderTargetMngr::DestroyAllTargs(int start, int end, int fbw)
{
	FUNCLOG

	for (MAPTARGETS::iterator it = mapTargets.begin(); it != mapTargets.end();)
	{
		if (it->second->start < end && start < it->second->end)
		{
			// if is depth, only resolve if fbw is the same
			if (!it->second->IsDepth())
			{
				// only resolve if the widths are the same or it->second has bit outside the range
				// shadow of colossus swaps between fbw=256,fbh=256 and fbw=512,fbh=448. This kills the game if doing || it->second->end > end

				// kh hack, sometimes kh movies do this to clear the target, so have a static count that periodically checks end
				static int count = 0;

				if (it->second->fbw == fbw || (it->second->fbw != fbw && (it->second->start < start || ((count++&0xf) ? 0 : it->second->end > end))))
				{
					it->second->Resolve();
				}
				else
				{
					FlushIfNecesary(it->second);
					it->second->status |= CRenderTarget::TS_Resolved;
				}
			}
			else
			{
				if (it->second->fbw == fbw)
				{
					it->second->Resolve();
				}
				else
				{
					FlushIfNecesary(it->second);
					it->second->status |= CRenderTarget::TS_Resolved;
				}
			}

			DestroyAllTargetsHelper(it->second);

			u32 dummykey = GetFrameKeyDummy(it->second);

			if (mapDummyTargs.find(dummykey) == mapDummyTargs.end())
			{
				mapDummyTargs[dummykey] = it->second;
			}
			else
			{
				delete it->second;
			}

			mapTargets.erase(it++);
		}
		else
		{
			++it;
		}
	}
}

void CRenderTargetMngr::DestroyTarg(CRenderTarget* ptarg)
{
	FUNCLOG
	DestroyAllTargetsHelper(ptarg);
	delete ptarg;
}

void CRenderTargetMngr::DestroyIntersecting(CRenderTarget* prndr)
{
	FUNCLOG
	assert(prndr != NULL);

	int start, end;
	GetRectMemAddressZero(start, end, prndr->psm, prndr->fbw, prndr->fbh, prndr->fbp, prndr->fbw);

	for (MAPTARGETS::iterator it = mapTargets.begin(); it != mapTargets.end();)
	{
		if ((it->second != prndr) && (it->second->start < end) && (start < it->second->end))
		{
			it->second->Resolve();
			DestroyAllTargetsHelper(it->second);
			u32 dummykey = GetFrameKeyDummy(it->second);

			if (mapDummyTargs.find(dummykey) == mapDummyTargs.end())
			{
				mapDummyTargs[dummykey] = it->second;
			}
			else
			{
				delete it->second;
			}

			mapTargets.erase(it++);
		}
		else
		{
			++it;
		}
	}
}


void CRenderTargetMngr::PrintTargets()
{
#ifdef _DEBUG
	for (MAPTARGETS::iterator it1 = mapDummyTargs.begin(); it1 != mapDummyTargs.end(); ++it1)
		ZZLog::Debug_Log("\t Dummy Targets(0x%x) fbw:0x%x fbh:0x%x psm:0x%x fbp:0x%x", GetFrameKey(it1->second), it1->second->fbw, it1->second->fbh, it1->second->psm, it1->second->fbp);

	for (MAPTARGETS::iterator it1 = mapTargets.begin(); it1 != mapTargets.end(); ++it1)
		ZZLog::Debug_Log("\t Targets(0x%x) fbw:0x%x fbh:0x%x psm:0x%x fbp:0x%x", GetFrameKey(it1->second), it1->second->fbw, it1->second->fbh, it1->second->psm, it1->second->fbp);
#endif
}

bool CRenderTargetMngr::isFound(const frameInfo& frame, MAPTARGETS::iterator& it, u32 opts, u32 key, int maxposheight)
{
	// only enforce height if frame.fbh <= 0x1c0
	bool bfound = it != mapTargets.end();
	
	if (bfound)
	{
		if (opts&TO_StrictHeight)
		{
			bfound = it->second->fbh == frame.fbh;

			if ((conf.settings().partial_depth) && !bfound)
			{
				MAPTARGETS::iterator itnew = mapTargets.find(key + 1);

				if (itnew != mapTargets.end() && itnew->second->fbh == frame.fbh)
				{
					// found! delete the previous and restore
					delete it->second;
					mapTargets.erase(it);

					it = mapTargets.insert(MAPTARGETS::value_type(key, itnew->second)).first; // readd
					mapTargets.erase(itnew); // delete old

					bfound = true;
				}
			}
		}
		else
		{
			if (PSMT_ISHALF(frame.psm) == PSMT_ISHALF(it->second->psm) && !(conf.settings().full_16_bit_res))
				bfound = ((frame.fbh > 0x1c0) || (it->second->fbh >= frame.fbh)) && (it->second->fbh <= maxposheight);
		}
	}

	if (!bfound)
	{
		// might be a virtual target
		it = mapTargets.find(key | TARGET_VIRTUAL_KEY);
		bfound = it != mapTargets.end() && ((opts & TO_StrictHeight) ? it->second->fbh == frame.fbh : it->second->fbh >= frame.fbh) && it->second->fbh <= maxposheight;
	}

	if (bfound && PSMT_ISHALF(frame.psm) && PSMT_ISHALF(it->second->psm) && (conf.settings().full_16_bit_res))
	{
		// mgs3
		if (frame.fbh > it->second->fbh)
		{
			bfound = false;
		}
	}

	return bfound;
}

CRenderTarget* CRenderTargetMngr::GetTarg(const frameInfo& frame, u32 opts, int maxposheight)
{
	FUNCLOG

	if (frame.fbw <= 0 || frame.fbh <= 0) 
	{
		//ZZLog::Dev_Log("frame fbw == %d; fbh == %d", frame.fbw, frame.fbh);
		return NULL;
	}

	GL_REPORT_ERRORD();

	u32 key = GetFrameKey(frame);

	MAPTARGETS::iterator it = mapTargets.find(key);
	
	if (isFound(frame, it, opts, key, maxposheight))
	{
		// can be both 16bit and 32bit
		if (PSMT_ISHALF(frame.psm) != PSMT_ISHALF(it->second->psm))
		{
			// a lot of games do this, actually...
			ZZLog::Debug_Log("Really bad formats! %d %d", frame.psm, it->second->psm);

//			This code SHOULD be commented, until I redo the _Resolve function

			if (!(opts & TO_StrictHeight))
			{
				if ((conf.settings().vss_hack_off))
				{
					if (PSMT_ISHALF(it->second->psm))
					{
						it->second->status |= CRenderTarget::TS_NeedConvert32;
						it->second->fbh /= 2;
					}
					else
					{
						it->second->status |= CRenderTarget::TS_NeedConvert16;
						it->second->fbh *= 2;
					}
				}
			}

			// recalc extents
			GetRectMemAddressZero(it->second->start, it->second->end, frame.psm, frame.fbw, it->second->fbh, it->second->fbp, frame.fbw);
		}
		else
		{
			// certain variables have to be reset every time
			if ((it->second->psm & ~1) != (frame.psm & ~1))
			{
				ZZLog::Dev_Log("Bad formats 2: %d %d", frame.psm, it->second->psm);
				
				it->second->psm = frame.psm;

				// recalc extents
				GetRectMemAddressZero(it->second->start, it->second->end, frame.psm, frame.fbw, it->second->fbh, it->second->fbp, frame.fbw);
			}
		}

		if (it->second->fbm != frame.fbm)
		{
			//ZZLog::Dev_Log("Bad fbm: 0x%8.8x 0x%8.8x, psm: %d", frame.fbm, it->second->fbm, frame.psm);
		}

		it->second->fbm &= frame.fbm;
		it->second->psm = frame.psm; // have to convert (ffx2)

		if ((it->first & TARGET_VIRTUAL_KEY) && !(opts&TO_Virtual))
		{
			// switch
			it->second->lastused = timeGetTime();
			return Promote(it->first&~TARGET_VIRTUAL_KEY);
		}

		// check if there exists a more recent target that this target could update from
		// only update if target isn't mirrored
		bool bCheckHalfCovering = (conf.settings().full_16_bit_res) && PSMT_ISHALF(it->second->psm) && it->second->fbh + 32 < frame.fbh;

		for (MAPTARGETS::iterator itnew = mapTargets.begin(); itnew != mapTargets.end(); ++itnew)
		{
			if (itnew->second != it->second && itnew->second->ptex != it->second->ptex && itnew->second->ptexFeedback != it->second->ptex &&
					itnew->second->lastused > it->second->lastused && !(itnew->second->status & CRenderTarget::TS_NeedUpdate))
			{

				// if new target totally encompasses the current one
				if (itnew->second->start <= it->second->start && itnew->second->end >= it->second->end)
				{
					it->second->status |= CRenderTarget::TS_NeedUpdate;
					it->second->nUpdateTarg = itnew->first;
					break;
				}

				// if 16bit, then check for half encompassing targets
				if (bCheckHalfCovering && itnew->second->start > it->second->start && itnew->second->start < it->second->end && itnew->second->end <= it->second->end + 0x2000)
				{
					it->second->status |= CRenderTarget::TS_NeedUpdate;
					it->second->nUpdateTarg = itnew->first;
					break;
				}
			}
		}

		it->second->lastused = timeGetTime();

		return it->second;
	}

	// NOTE: instead of resolving, if current render targ is completely outside of old, can transfer
	// the data like that.

	// first search for the target
	CRenderTarget* ptarg = NULL;

	// have to change, so recreate (find all intersecting targets and Resolve)
	u32 besttarg = 0;

	if (!(opts & CRenderTargetMngr::TO_Virtual))
	{

		int start, end;
		GetRectMemAddressZero(start, end, frame.psm, frame.fbw, frame.fbh, frame.fbp, frame.fbw);
		CRenderTarget* pbesttarg = NULL;

		if (besttarg == 0)
		{
			// if there is only one intersecting target and it encompasses the current one, update the new render target with
			// its data instead of resolving then updating (ffx2). Do not change the original target.
			for (MAPTARGETS::iterator it = mapTargets.begin(); it != mapTargets.end(); ++it)
			{
				if (it->second->start < end && start < it->second->end)
				{
					if ((conf.settings().fast_update) ||
							((frame.fbw == it->second->fbw) &&
							 // check depth targets only if partialdepth option
							 ((it->second->fbp != frame.fbp) || ((conf.settings().partial_depth) && (opts & CRenderTargetMngr::TO_DepthBuffer)))))
					{
						if (besttarg != 0)
						{
							besttarg = 0;
							break;
						}

						if (start >= it->second->start && end <= it->second->end)
						{
							besttarg = it->first;
							pbesttarg = it->second;
						}
					}
				}
			}
		}

		if (besttarg != 0 && pbesttarg->fbw != frame.fbw)
		{
			//ZZLog::Debug_Log("A %d %d %d %d\n", frame.psm, frame.fbw, pbesttarg->psm, pbesttarg->fbw);

			vb[0].frame.fbw = pbesttarg->fbw;
			// Something should be here, but what?
		}

		if (besttarg == 0)
		{
			// if none found, resolve all
			DestroyAllTargs(start, end, frame.fbw);
		}
		else if (key == besttarg && pbesttarg != NULL)
		{
			// add one and store in a different location until best targ is processed
			mapTargets.erase(besttarg);
			besttarg++;
			mapTargets[besttarg] = pbesttarg;
		}
	}

	if (mapTargets.size() > 8)
	{
		// release some resources
		it = GetOldestTarg(mapTargets);

		// if more than 5s passed since target used, destroy

		if ((it->second != vb[0].prndr) && (it->second != vb[1].prndr) &&
			(it->second != vb[0].pdepth) && (it->second != vb[1].pdepth) &&
				((timeGetTime() - it->second->lastused) > 5000))
		{
			delete it->second;
			mapTargets.erase(it);
		}
	}

	if (ptarg == NULL)
	{
		// not found yet, so create

		if (mapDummyTargs.size() > 8)
		{
			it = GetOldestTarg(mapDummyTargs);

			delete it->second;
			mapDummyTargs.erase(it);
		}

		it = mapDummyTargs.find(GetFrameKeyDummy(frame));

		if (it != mapDummyTargs.end())
		{
			ZZLog::Debug_Log("Dummy Frame fbw:0x%x fbh:0x%x psm:0x%x fbp:0x%x", frame.fbw, frame.fbh, frame.psm, frame.fbp);
			PrintTargets();
			ZZLog::Debug_Log("Dummy it->second fbw:0x%x fbh:0x%x psm:0x%x fbp:0x%x", it->second->fbw, it->second->fbh, it->second->psm, it->second->fbp);
			ptarg = it->second;

			mapDummyTargs.erase(it);

			// restore all setttings
			ptarg->psm = frame.psm;
			ptarg->fbm = frame.fbm;
			ptarg->fbp = frame.fbp;

			GetRectMemAddressZero(ptarg->start, ptarg->end, frame.psm, frame.fbw, frame.fbh, frame.fbp, frame.fbw);

			ptarg->status = CRenderTarget::TS_NeedUpdate;
		}
		else
		{
			ZZLog::Debug_Log("Frame fbw:0x%x fbh:0x%x psm:0x%x fbp:0x%x", frame.fbw, frame.fbh, frame.psm, frame.fbp);
			PrintTargets();
			// create anew
			ptarg = (opts & TO_DepthBuffer) ? new CDepthTarget : new CRenderTarget;
			CRenderTargetMngr* pmngrs[2] = { &s_DepthRTs, this == &s_RTs ? &s_RTs : NULL };
			int cur = 0;

			while (!ptarg->Create(frame))
			{
				// destroy unused targets
				if (mapDummyTargs.size() > 0)
				{
					it = mapDummyTargs.begin();
					delete it->second;
					mapDummyTargs.erase(it);
					continue;
				}

				if (g_MemTargs.listClearedTargets.size() > 0)
				{
					g_MemTargs.DestroyCleared();
					continue;
				}
				else if (g_MemTargs.listTargets.size() > 32)
					{
						g_MemTargs.DestroyOldest();
						continue;
					}

				if (pmngrs[cur] == NULL)
				{
					cur = !cur;

					if (pmngrs[cur] == NULL)
					{
						ZZLog::Warn_Log("Out of memory!");
						delete ptarg;
						return NULL;
					}
				}

				if (pmngrs[cur]->mapTargets.size() == 0)
				{
					pmngrs[cur] = NULL;
					cur = !cur;
					continue;
				}

				it = GetOldestTarg(pmngrs[cur]->mapTargets);

				DestroyTarg(it->second);
				pmngrs[cur]->mapTargets.erase(it);
				cur = !cur;
			}
		}
	}

	if ((opts & CRenderTargetMngr::TO_Virtual))
	{
		ptarg->status = CRenderTarget::TS_Virtual;
		key |= TARGET_VIRTUAL_KEY;

		if ((it = mapTargets.find(key)) != mapTargets.end())
		{

			DestroyTarg(it->second);
			it->second = ptarg;
			ptarg->nUpdateTarg = besttarg;
			return ptarg;
		}
	}
	else
	{
		assert(mapTargets.find(key) == mapTargets.end());
	}

	ptarg->nUpdateTarg = besttarg;

	mapTargets[key] = ptarg;

	return ptarg;
}

CRenderTargetMngr::MAPTARGETS::iterator CRenderTargetMngr::GetOldestTarg(MAPTARGETS& m)
{
	FUNCLOG

	if (m.size() == 0)
	{
		return m.end();
	}

	// release some resources
	MAPTARGETS::iterator itmaxtarg = m.begin();

	for (MAPTARGETS::iterator it = ++m.begin(); it != m.end(); ++it)
	{
		if (itmaxtarg->second->lastused < it->second->lastused) itmaxtarg = it;
	}

	return itmaxtarg;
}

void CRenderTargetMngr::GetTargs(int start, int end, list<CRenderTarget*>& listTargets) const
{
	FUNCLOG

	for (MAPTARGETS::const_iterator it = mapTargets.begin(); it != mapTargets.end(); ++it)
	{
		if ((it->second->start < end) && (start < it->second->end)) listTargets.push_back(it->second);
	}
}

void CRenderTargetMngr::Resolve(int start, int end)
{
	FUNCLOG

	for (MAPTARGETS::const_iterator it = mapTargets.begin(); it != mapTargets.end(); ++it)
	{
		if ((it->second->start < end) && (start < it->second->end))
			it->second->Resolve();
	}
}

