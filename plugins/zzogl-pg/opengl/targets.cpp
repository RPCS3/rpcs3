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

#include "GS.h"

#include <stdlib.h>

#include "Mem.h"
#include "x86.h"
#include "targets.h"
#include "ZZoglShaders.h"
#include "ZZClut.h"
#include <math.h>

#ifdef ZEROGS_SSE2
#include <emmintrin.h>
#endif

const float g_filog32 = 0.999f / (32.0f * logf(2.0f));
#define RHA
//#define RW

extern int g_TransferredToGPU;
extern bool g_bUpdateStencil;

#if !defined(ZEROGS_DEVBUILD)
#	define INC_RESOLVE()
#else
#	define INC_RESOLVE() ++g_nResolve
#endif

extern int s_nResolved;
extern u32 g_nResolve;
extern bool g_bSaveTrans;

CRenderTargetMngr s_RTs, s_DepthRTs;
CBitwiseTextureMngr s_BitwiseTextures;
CMemoryTargetMngr g_MemTargs;

//extern u32 s_ptexCurSet[2];
bool g_bSaveZUpdate = 0;

int VALIDATE_THRESH = 8;
u32 TEXDESTROY_THRESH = 16;

void _Resolve(const void* psrc, int fbp, int fbw, int fbh, int psm, u32 fbm, bool mode);
void SetWriteDepth();
bool IsWriteDepth();
bool IsWriteDestAlphaTest();

// ------------------------- Useful inlines ------------------------------------

// memory size for one row of texture. It depends on width of texture and number of bytes
// per pixel
inline u32 Pitch(int fbw) { return (RW(fbw) * 4) ; }

// memory size of whole texture. It is number of rows multiplied by memory size of row
inline u32 Tex_Memory_Size(int fbw, int fbh) { return (RH(fbh) * Pitch(fbw)); }

// Often called for several reasons
// Call flush if renderer or depth target is equal to ptr
inline void FlushIfNecesary(void* ptr)
{
	if (vb[0].prndr == ptr || vb[0].pdepth == ptr) Flush(0);
	if (vb[1].prndr == ptr || vb[1].pdepth == ptr) Flush(1);
}

// This block was repeated several times, so I inlined it.
inline void DestroyAllTargetsHelper(void* ptr)
{
	for (int i = 0; i < 2; ++i)
	{
		if (ptr == vb[i].prndr) { vb[i].prndr = NULL; vb[i].bNeedFrameCheck = 1; }
		if (ptr == vb[i].pdepth) { vb[i].pdepth = NULL; vb[i].bNeedZCheck = 1; }
	}
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

// used for transformation from vertex position in GS window.coords (I hope)
// to view coordinates (in range 0, 1).
inline float4 CRenderTarget::DefaultBitBltPos()
{
	float4 v = float4(1, -1, 0.5f / (float)RW(fbw), 0.5f / (float)RH(fbh));
	v *= 1.0f / 32767.0f;
	ZZshSetParameter4fv(pvsBitBlt.prog, pvsBitBlt.sBitBltPos, v, "g_sBitBltPos");
	return v;
}

// Used to transform texture coordinates from GS (when 0,0 is upper left) to
// OpenGL (0,0 - lower left).
inline float4 CRenderTarget::DefaultBitBltTex()
{
	// I really sure that -0.5 is correct, because OpenGL have no half-offset
	// issue, DirectX known for.
	float4 v = float4(1, -1, 0.5f / (float)RW(fbw), -0.5f / (float)RH(fbh));
	ZZshSetParameter4fv(pvsBitBlt.prog, pvsBitBlt.sBitBltTex, v, "g_sBitBltTex");
	return v;
}

inline void BindToSample(u32 *p_ptr)
{
	glBindTexture(GL_TEXTURE_RECTANGLE_NV, *p_ptr);
	setRectFilters(GL_NEAREST);
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
	vposxy.z = -1 - 0.5f / (float)fbw;
	vposxy.w = -1 + 0.5f / (float)fbh;
	status = 0;

	if (fbw > 0 && fbh > 0)
	{
		GetRectMemAddress(start, end, psm, 0, 0, fbw, fbh, fbp, fbw);
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
	FBTexture(0, ptexConv);
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
	FBTexture(0, ptexConv);
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

	FBTexture(0, ptexFeedback);
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

	FBTexture(targ, ptex);

	//GL_REPORT_ERRORD();
	//if (glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT) != GL_FRAMEBUFFER_COMPLETE_EXT)
		//ERROR_LOG_SPAM("The Framebuffer is not complete. Glitches could appear onscreen.\n");
}

CDepthTarget::CDepthTarget() : CRenderTarget(), pdepth(0), pstencil(0), icount(0) {}

CDepthTarget::~CDepthTarget()
{
	FUNCLOG

	Destroy();
}

bool CDepthTarget::Create(const frameInfo& frame)
{
	FUNCLOG

	if (!CRenderTarget::Create(frame)) return false;

	GL_REPORT_ERROR();

	glGenRenderbuffersEXT(1, &pdepth);
	glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, pdepth);
	glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_DEPTH24_STENCIL8_EXT, RW(fbw), RH(fbh));

	if (glGetError() != GL_NO_ERROR)
	{
		// try a separate depth and stencil buffer
		glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, pdepth);
		glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT24, RW(fbw), RH(fbh));

		if (g_bUpdateStencil)
		{
			glGenRenderbuffersEXT(1, &pstencil);
			glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, pstencil);
			glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_STENCIL_INDEX8_EXT, RW(fbw), RH(fbh));

			if (glGetError() != GL_NO_ERROR)
			{
				ZZLog::Error_Log("Failed to create depth buffer %dx%d.", RW(fbw), RH(fbh));
				return false;
			}
		}
		else
		{
			pstencil = 0;
		}
	}
	else
	{
		pstencil = pdepth;
	}

	status = TS_NeedUpdate;

	return true;
}

void CDepthTarget::Destroy()
{
	FUNCLOG

	if (status)     // In this case Framebuffer extension is off-use and lead to segfault
	{
		ResetRenderTarget(1);
		TextureRect(GL_DEPTH_ATTACHMENT_EXT);
		TextureRect(GL_STENCIL_ATTACHMENT_EXT);
		GL_REPORT_ERRORD();

		if (pstencil != 0)
		{
			if (pstencil != pdepth) glDeleteRenderbuffersEXT(1, &pstencil);
			pstencil = 0;
		}

		if (pdepth != 0)
		{
			glDeleteRenderbuffersEXT(1, &pdepth);
			pdepth = 0;
		}

		GL_REPORT_ERRORD();
	}

	CRenderTarget::Destroy();
}


extern int g_nDepthUsed; // > 0 if depth is used

void CDepthTarget::Resolve()
{
	FUNCLOG

	if (g_nDepthUsed > 0 && conf.mrtdepth && !(status&TS_Virtual) && IsWriteDepth() && !(conf.settings().no_depth_resolve))
		CRenderTarget::Resolve();
	else
	{
		// flush if necessary
		FlushIfNecesary(this) ;

		if (!(status & TS_Virtual)) status |= TS_Resolved;
	}

	if (!(status&TS_Virtual))
	{
		SetWriteDepth();
	}
}

void CDepthTarget::Resolve(int startrange, int endrange)
{
	FUNCLOG

	if (g_nDepthUsed > 0 && conf.mrtdepth && !(status&TS_Virtual) && IsWriteDepth())
	{
		CRenderTarget::Resolve(startrange, endrange);
	}
	else
	{
		// flush if necessary
		FlushIfNecesary(this) ;

		if (!(status & TS_Virtual))
			status |= TS_Resolved;
	}

	if (!(status&TS_Virtual))
	{
		SetWriteDepth();
	}
}

void CDepthTarget::Update(int context, CRenderTarget* prndr)
{
	FUNCLOG

	assert(!(status & TS_Virtual));

	// align the rect to the nearest page
	// note that fbp is always aligned on page boundaries
	tex0Info texframe;
	texframe.tbp0 = fbp;
	texframe.tbw = fbw;
	texframe.tw = fbw;
	texframe.th = fbh;
	texframe.psm = psm;

	DisableAllgl();

	VB& curvb = vb[context];

	if (curvb.test.zte == 0) return;

	SetShaderCaller("CDepthTarget::Update");

	glEnable(GL_DEPTH_TEST);

	glDepthMask(!curvb.zbuf.zmsk);

	static const u32 g_dwZCmp[] = { GL_NEVER, GL_ALWAYS, GL_GEQUAL, GL_GREATER };

	glDepthFunc(g_dwZCmp[curvb.test.ztst]);

	// write color and zero out stencil buf, always 0 context!
	SetTexVariablesInt(0, 0, texframe, false, &ppsBitBltDepth, 1);
	ZZshGLSetTextureParameter(ppsBitBltDepth.prog, ppsBitBltDepth.sMemory, vb[0].pmemtarg->ptex->tex, "BitBltDepth");

	float4 v = DefaultBitBltPos();

	v = DefaultBitBltTex();

	v.x = 1;
	v.y = 2;
	v.z = PSMT_IS16Z(psm) ? 1.0f : 0.0f;
	v.w = g_filog32;
	ZZshSetParameter4fv(ppsBitBltDepth.prog, ppsBitBltDepth.sOneColor, v, "g_fOneColor");

	float4 vdepth = g_vdepth;

	if (psm == PSMT24Z)
	{
		vdepth.w = 0;
	}
	else if (psm != PSMT32Z)
	{
		vdepth.z = vdepth.w = 0;
	}

	assert(ppsBitBltDepth.sBitBltZ != 0);

	ZZshSetParameter4fv(ppsBitBltDepth.prog, ppsBitBltDepth.sBitBltZ, (vdepth*(255.0f / 256.0f)), "g_fBitBltZ");

	assert(pdepth != 0);
	//GLint w1 = 0;
	//GLint h1 = 0;

	glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_RECTANGLE_NV, ptex, 0);
	//glGetRenderbufferParameterivEXT(GL_RENDERBUFFER_EXT, GL_RENDERBUFFER_WIDTH_EXT, &w1);
	//glGetRenderbufferParameterivEXT(GL_RENDERBUFFER_EXT, GL_RENDERBUFFER_HEIGHT_EXT, &h1);
	SetDepthStencilSurface();

	FBTexture(1);

	GLenum buffer = GL_COLOR_ATTACHMENT0_EXT;

	//ZZLog::Error_Log("CDepthTarget::Update: w1 = 0x%x; h1 = 0x%x", w1, h1);
	DrawBuffers(&buffer);

	SetViewport();

	if (conf.wireframe()) glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	glBindBuffer(GL_ARRAY_BUFFER, vboRect);

	SET_STREAM();
	ZZshSetVertexShader(pvsBitBlt.prog);
	ZZshSetPixelShader(ppsBitBltDepth.prog);

	DrawTriangleArray();

	status = TS_Resolved;

	if (!IsWriteDepth())
	{
		ResetRenderTarget(1);
	}

	if (conf.wireframe()) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	glEnable(GL_SCISSOR_TEST);

#ifdef _DEBUG
	if (g_bSaveZUpdate)
	{
		SaveTex(&texframe, 1);
		SaveTexture("frame1.tga", GL_TEXTURE_RECTANGLE_NV, ptex, RW(fbw), RH(fbh));
	}
#endif
}

void CDepthTarget::SetDepthStencilSurface()
{
	FUNCLOG
	TextureRect(GL_DEPTH_ATTACHMENT_EXT, pdepth);

	if (pstencil)
	{
		// there's a bug with attaching stencil and depth buffers
		TextureRect(GL_STENCIL_ATTACHMENT_EXT, pstencil);

		if (icount++ < 8)    // not going to fail if succeeded 4 times
		{
			GL_REPORT_ERRORD();

			if (glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT) != GL_FRAMEBUFFER_COMPLETE_EXT)
			{
				TextureRect(GL_STENCIL_ATTACHMENT_EXT);

				if (pstencil != pdepth) glDeleteRenderbuffersEXT(1, &pstencil);

				pstencil = 0;
				g_bUpdateStencil = 0;
			}
		}
	}
	else
	{
		TextureRect(GL_STENCIL_ATTACHMENT_EXT);
	}
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
					FlushIfNecesary(it->second) ;
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
					FlushIfNecesary(it->second) ;
					it->second->status |= CRenderTarget::TS_Resolved;
				}
			}

			DestroyAllTargetsHelper(it->second) ;

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
	DestroyAllTargetsHelper(ptarg) ;
	delete ptarg;
}

void CRenderTargetMngr::DestroyIntersecting(CRenderTarget* prndr)
{
	FUNCLOG
	assert(prndr != NULL);

	int start, end;
	GetRectMemAddress(start, end, prndr->psm, 0, 0, prndr->fbw, prndr->fbh, prndr->fbp, prndr->fbw);

	for (MAPTARGETS::iterator it = mapTargets.begin(); it != mapTargets.end();)
	{
		if ((it->second != prndr) && (it->second->start < end) && (start < it->second->end))
		{
			it->second->Resolve();
			DestroyAllTargetsHelper(it->second) ;
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

//--------------------------------------------------


inline bool CheckWidthIsSame(const frameInfo& frame, CRenderTarget* ptarg)
{
	if (PSMT_ISHALF(frame.psm) == PSMT_ISHALF(ptarg->psm))
		return (frame.fbw == ptarg->fbw);

	if (PSMT_ISHALF(frame.psm))
		return (frame.fbw == 2 * ptarg->fbw);
	else
		return (2 * frame.fbw == ptarg->fbw);
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
			GetRectMemAddress(it->second->start, it->second->end, frame.psm, 0, 0, frame.fbw, it->second->fbh, it->second->fbp, frame.fbw);
		}
		else
		{
			// certain variables have to be reset every time
			if ((it->second->psm & ~1) != (frame.psm & ~1))
			{
				ZZLog::Dev_Log("Bad formats 2: %d %d", frame.psm, it->second->psm);
				
				it->second->psm = frame.psm;

				// recalc extents
				GetRectMemAddress(it->second->start, it->second->end, frame.psm, 0, 0, frame.fbw, it->second->fbh, it->second->fbp, frame.fbw);
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
		GetRectMemAddress(start, end, frame.psm, 0, 0, frame.fbw, frame.fbh, frame.fbp, frame.fbw);
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

			GetRectMemAddress(ptarg->start, ptarg->end, frame.psm, 0, 0, frame.fbw, frame.fbh, frame.fbp, frame.fbw);

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

void CMemoryTargetMngr::Destroy()
{
	FUNCLOG
	listTargets.clear();
	listClearedTargets.clear();
}

bool CMemoryTarget::ValidateTex(const tex0Info& tex0, int starttex, int endtex, bool bDeleteBadTex)
{
	FUNCLOG

	if (clearmaxy == 0) return true;

	int checkstarty = max(starttex, clearminy);
	int checkendy = min(endtex, clearmaxy);

	if (checkstarty >= checkendy) return true;

	if (validatecount++ > VALIDATE_THRESH)
	{
		height = 0;
		return false;
	}

	// lock and compare
	assert(ptex != NULL && ptex->memptr != NULL);

	int result = memcmp_mmx(ptex->memptr + MemorySize(checkstarty-realy), MemoryAddress(checkstarty), MemorySize(checkendy-checkstarty));
	
	if (result == 0)
	{
		clearmaxy = 0;
		return true;
	}

	if (!bDeleteBadTex) return false;

	// delete clearminy, clearmaxy range (not the checkstarty, checkendy range)
	//int newstarty = 0;
	if (clearminy <= starty)
	{
		if (clearmaxy < starty + height)
		{
			// preserve end
			height = starty + height - clearmaxy;
			starty = clearmaxy;
			assert(height > 0);
		}
		else
		{
			// destroy
			height = 0;
		}
	}
	else
	{
		// beginning can be preserved
		height = clearminy - starty;
	}

	clearmaxy = 0;

	assert((starty >= realy) && ((starty + height) <= (realy + realheight)));

	return false;
}

#define TARGET_THRESH 0x500

extern int g_MaxTexWidth, g_MaxTexHeight; // Maximum height & width of supported texture.

//#define SORT_TARGETS
inline list<CMemoryTarget>::iterator CMemoryTargetMngr::DestroyTargetIter(list<CMemoryTarget>::iterator& it)
{
	// find the target and destroy
	list<CMemoryTarget>::iterator itprev = it;
	++it;
	listClearedTargets.splice(listClearedTargets.end(), listTargets, itprev);

	if (listClearedTargets.size() > TEXDESTROY_THRESH)
	{
		listClearedTargets.pop_front();
	}

	return it;
}

// Compare target to current texture info
// Not same format -> 1
// Same format, not same data (clut only) -> 2
// identical -> 0
int CMemoryTargetMngr::CompareTarget(list<CMemoryTarget>::iterator& it, const tex0Info& tex0, int clutsize)
{
	if (PSMT_ISCLUT(it->psm) != PSMT_ISCLUT(tex0.psm))
		return 1;

	if (PSMT_ISCLUT(tex0.psm)) {
		if (it->psm != tex0.psm || it->cpsm != tex0.cpsm || it->clutsize != clutsize)
			return 1;

		if	(PSMT_IS32BIT(tex0.cpsm)) {
			if (Cmp_ClutBuffer_SavedClut<u32>((u32*)&it->clut[0], tex0.csa, clutsize))
				return 2;
		} else {
			if (Cmp_ClutBuffer_SavedClut<u16>((u16*)&it->clut[0], tex0.csa, clutsize))
				return 2;
		}

	} else {
		if (PSMT_IS16BIT(tex0.psm) != PSMT_IS16BIT(it->psm))
			return 1;
    }

	return 0;
}

void CMemoryTargetMngr::GetClutVariables(int& clutsize, const tex0Info& tex0)
{
	clutsize = 0;

	if (PSMT_ISCLUT(tex0.psm))
	{
		int entries = PSMT_IS8CLUT(tex0.psm) ? 256 : 16;

		if (PSMT_IS32BIT(tex0.cpsm))
			clutsize = min(entries, 256 - tex0.csa * 16) * 4;
		else
			clutsize = min(entries, 512 - tex0.csa * 16) * 2;
	}
}

void CMemoryTargetMngr::GetMemAddress(int& start, int& end,  const tex0Info& tex0)
{
	int nbStart, nbEnd;
	GetRectMemAddress(nbStart, nbEnd, tex0.psm, 0, 0, tex0.tw, tex0.th, tex0.tbp0, tex0.tbw);
	assert(nbStart < nbEnd);
	nbEnd = min(nbEnd, MEMORY_END);

	start = nbStart / (4 * GPU_TEXWIDTH);
	end = (nbEnd + GPU_TEXWIDTH * 4 - 1) / (4 * GPU_TEXWIDTH);
	assert(start < end);

}

CMemoryTarget* CMemoryTargetMngr::SearchExistTarget(int start, int end, int clutsize, const tex0Info& tex0, int forcevalidate)
{
	for (list<CMemoryTarget>::iterator it = listTargets.begin(); it != listTargets.end();)
	{

		if (it->starty <= start && it->starty + it->height >= end)
		{

			int res = CompareTarget(it, tex0, clutsize);

			if (res == 1)
			{
				if (it->validatecount++ > VALIDATE_THRESH)
				{
					it = DestroyTargetIter(it);

					if (listTargets.size() == 0) break;
				}
				else
					++it;

				continue;
			}
			else if (res == 2)
			{
				++it;
				continue;
			}

			if (forcevalidate)   //&& listTargets.size() < TARGET_THRESH ) {
			{
				// do more validation checking. delete if not been used for a while

				if (!it->ValidateTex(tex0, start, end, curstamp > it->usedstamp + 3))
				{

					if (it->height <= 0)
					{
						it = DestroyTargetIter(it);

						if (listTargets.size() == 0)
							break;
					}
					else
						++it;

					continue;
				}
			}

			it->usedstamp = curstamp;

			it->validatecount = 0;

			return &(*it);
		}

#ifdef SORT_TARGETS
		else if (it->starty >= end) break;

#endif

		++it;
	}

	return NULL;
}

CMemoryTarget* CMemoryTargetMngr::ClearedTargetsSearch(int fmt, int widthmult, int channels, int height)
{
	CMemoryTarget* targ = NULL;

	if (listClearedTargets.size() > 0)
	{
		list<CMemoryTarget>::iterator itbest = listClearedTargets.begin();

		while (itbest != listClearedTargets.end())
		{
			if ((height == itbest->realheight) && (itbest->fmt == fmt) && (itbest->widthmult == widthmult) && (itbest->channels == channels))
			{
				// check channels
				if (PIXELS_PER_WORD(itbest->psm) == channels) break;
			}

			++itbest;
		}

		if (itbest != listClearedTargets.end())
		{
			listTargets.splice(listTargets.end(), listClearedTargets, itbest);
			targ = &listTargets.back();
			targ->validatecount = 0;
		}
		else
		{
			// create a new
			listTargets.push_back(CMemoryTarget());
			targ = &listTargets.back();
		}
	}
	else
	{
		listTargets.push_back(CMemoryTarget());
		targ = &listTargets.back();
	}

	return targ;
}

CMemoryTarget* CMemoryTargetMngr::GetMemoryTarget(const tex0Info& tex0, int forcevalidate)
{
	FUNCLOG
	int start, end, clutsize;

	GetClutVariables(clutsize, tex0);
	GetMemAddress(start, end, tex0);

	CMemoryTarget* it = SearchExistTarget(start, end, clutsize, tex0, forcevalidate);

	if (it != NULL) return it;

	// couldn't find so create
	CMemoryTarget* targ;

	u32 fmt;
    u32 internal_fmt;
	if (PSMT_ISHALF_STORAGE(tex0)) {
        // RGBA_5551 storage format
        fmt = GL_UNSIGNED_SHORT_1_5_5_5_REV;
        internal_fmt = GL_RGB5_A1;
    } else {
        // RGBA_8888 storage format
        fmt = GL_UNSIGNED_BYTE;
        internal_fmt = GL_RGBA;
    }

	int widthmult = 1, channels = 1;

	// If our texture is too big and could not be placed in 1 GPU texture. Pretty rare in modern cards.
	if ((g_MaxTexHeight < 4096) && (end - start > g_MaxTexHeight)) 
	{
		// In this rare case we made a texture of half height and place it on the screen.
		ZZLog::Debug_Log("Making a half height texture (start - end == 0x%x)", (end-start));
		widthmult = 2;
	}
	
	channels = PIXELS_PER_WORD(tex0.psm);

	targ = ClearedTargetsSearch(fmt, widthmult, channels, end - start);

	if (targ->ptex != NULL)
	{
		assert(end - start <= targ->realheight && targ->fmt == fmt && targ->widthmult == widthmult);

		// good enough, so init
		targ->realy = targ->starty = start;
		targ->usedstamp = curstamp;
		targ->psm = tex0.psm;
		targ->cpsm = tex0.cpsm;
		targ->height = end - start;
	} else {
		// not initialized yet
		targ->fmt = fmt;
		targ->realy = targ->starty = start;
		targ->realheight = targ->height = end - start;
		targ->usedstamp = curstamp;
		targ->psm = tex0.psm;
		targ->cpsm = tex0.cpsm;
		targ->widthmult = widthmult;
		targ->channels = channels;
		targ->texH = (targ->realheight + widthmult - 1)/widthmult;
		targ->texW = GPU_TEXWIDTH *  widthmult * channels;

		// alloc the mem
		targ->ptex = new CMemoryTarget::TEXTURE();
		targ->ptex->ref = 1;
	}

#if defined(ZEROGS_DEVBUILD)
	g_TransferredToGPU += MemorySize(channels * targ->height);
#endif

	// fill with data
	if (targ->ptex->memptr == NULL)
	{
		targ->ptex->memptr = (u8*)_aligned_malloc(MemorySize(targ->realheight), 16);
		assert(targ->ptex->ref > 0);
	}

	memcpy_amd(targ->ptex->memptr, MemoryAddress(targ->realy), MemorySize(targ->height));

	__aligned16 u8* ptexdata = NULL;
	bool has_data = false;

	if (PSMT_ISCLUT(tex0.psm))
	{
		assert(clutsize > 0);

        // Local clut parameter
		targ->cpsm = tex0.cpsm;

        // Allocate a local clut array
        targ->clutsize = clutsize;
        if(targ->clut == NULL)
            targ->clut = (u8*)_aligned_malloc(clutsize, 16);
        else {
            // In case it could occured
            // realloc would be better but you need to get it from libutilies first
            _aligned_free(targ->clut);
            targ->clut = (u8*)_aligned_malloc(clutsize, 16);
        }

        // texture parameter
		ptexdata = (u8*)_aligned_malloc(CLUT_PIXEL_SIZE(tex0.cpsm) * targ->texH * targ->texW, 16);
		has_data = true;

		u8* psrc = (u8*)(MemoryAddress(targ->realy));

        // Fill a local clut then build the real texture
		if (PSMT_IS32BIT(tex0.cpsm))
		{
            ClutBuffer_to_Array<u32>((u32*)targ->clut, tex0.csa, clutsize);
			Build_Clut_Texture<u32>(tex0.psm, targ->height, (u32*)targ->clut, psrc, (u32*)ptexdata);
		}
		else
		{
            ClutBuffer_to_Array<u16>((u16*)targ->clut, tex0.csa, clutsize);
			Build_Clut_Texture<u16>(tex0.psm, targ->height, (u16*)targ->clut, psrc, (u16*)ptexdata);
		}

        assert(targ->clutsize > 0);
	}
	else
	{
		if (tex0.psm == PSMT16Z || tex0.psm == PSMT16SZ)
		{
			ptexdata = (u8*)_aligned_malloc(4 * targ->texH * targ->texW, 16);
			has_data = true;

			// needs to be 8 bit, use xmm for unpacking
			u16* dst = (u16*)ptexdata;
			u16* src = (u16*)(MemoryAddress(targ->realy));

#ifdef ZEROGS_SSE2
			assert(((u32)(uptr)dst) % 16 == 0);
            // FIXME Uncomment to test intrinsic versions (instead of asm)
            // perf improvement vs asm:
            // 1/ gcc updates both pointer with 1 addition
            // 2/ Bypass the cache for the store
#define NEW_INTRINSIC_VERSION
#ifdef NEW_INTRINSIC_VERSION

            __m128i zero_128 = _mm_setzero_si128();
            // NOTE: future performance improvement
            // SSE4.1 support uncacheable load 128bits. Maybe it can
            // avoid some cache pollution
            // NOTE2: I create multiple _n variable to mimic the previous ASM behavior
            // but I'm not sure there are real gains.
			for (int i = targ->height * GPU_TEXWIDTH/16 ; i > 0 ; --i)
            {
                // Convert 16 bits pixels to 32bits (zero extended)
                // Batch 64 bytes (32 pixels) at once.
                __m128i pixels_1 = _mm_load_si128((__m128i*)src);
                __m128i pixels_2 = _mm_load_si128((__m128i*)(src+8));
                __m128i pixels_3 = _mm_load_si128((__m128i*)(src+16));
                __m128i pixels_4 = _mm_load_si128((__m128i*)(src+24));

                __m128i pix_low_1 = _mm_unpacklo_epi16(pixels_1, zero_128);
                __m128i pix_high_1 = _mm_unpackhi_epi16(pixels_1, zero_128);
                __m128i pix_low_2 = _mm_unpacklo_epi16(pixels_2, zero_128);
                __m128i pix_high_2 = _mm_unpackhi_epi16(pixels_2, zero_128);

                // Note: bypass cache
                _mm_stream_si128((__m128i*)dst, pix_low_1);
                _mm_stream_si128((__m128i*)(dst+8), pix_high_1);
                _mm_stream_si128((__m128i*)(dst+16), pix_low_2);
                _mm_stream_si128((__m128i*)(dst+24), pix_high_2);

                __m128i pix_low_3 = _mm_unpacklo_epi16(pixels_3, zero_128);
                __m128i pix_high_3 = _mm_unpackhi_epi16(pixels_3, zero_128);
                __m128i pix_low_4 = _mm_unpacklo_epi16(pixels_4, zero_128);
                __m128i pix_high_4 = _mm_unpackhi_epi16(pixels_4, zero_128);

                // Note: bypass cache
                _mm_stream_si128((__m128i*)(dst+32), pix_low_3);
                _mm_stream_si128((__m128i*)(dst+40), pix_high_3);
                _mm_stream_si128((__m128i*)(dst+48), pix_low_4);
                _mm_stream_si128((__m128i*)(dst+56), pix_high_4);

                src += 32;
                dst += 64;
            }
            // It is advise to use a fence instruction after non temporal move (mm_stream) instruction...
            // store fence insures that previous store are finish before execute new one.
            _mm_sfence();
#else
			SSE2_UnswizzleZ16Target(dst, src, targ->height * GPU_TEXWIDTH / 16);
#endif
#else // ZEROGS_SSE2

			for (int i = 0; i < targ->height; ++i)
			{
				for (int j = 0; j < GPU_TEXWIDTH; ++j)
				{
					dst[0] = src[0];
					dst[1] = 0;
					dst[2] = src[1];
					dst[3] = 0;
					dst += 4;
					src += 2;
				}
			}

#endif // ZEROGS_SSE2
		}
		else
		{
			ptexdata = targ->ptex->memptr;
			// We really don't want to deallocate memptr. As a reminder...
			has_data = false;
		}
	}

	// create the texture
	GL_REPORT_ERRORD();

	assert(ptexdata != NULL);

	if (targ->ptex->tex == 0) glGenTextures(1, &targ->ptex->tex);

	glBindTexture(GL_TEXTURE_RECTANGLE_NV, targ->ptex->tex);

    TextureRect(internal_fmt, targ->texW, targ->texH, GL_RGBA, fmt, ptexdata);

	while (glGetError() != GL_NO_ERROR)
	{
		// release resources until can create
		if (listClearedTargets.size() > 0)
		{
			listClearedTargets.pop_front();
		}
		else
		{
			if (listTargets.size() == 0)
			{
				ZZLog::Error_Log("Failed to create %dx%x texture.", targ->texW, targ->texH);
				channels = 1;
				if (has_data) _aligned_free(ptexdata);
				return NULL;
			}

			DestroyOldest();
		}

        TextureRect(internal_fmt, targ->texW, targ->texH, GL_RGBA, fmt, ptexdata);
	}

	setRectWrap(GL_CLAMP);
	if (has_data) _aligned_free(ptexdata);

	assert(tex0.psm != 0xd);

	return targ;
}

void CMemoryTargetMngr::ClearRange(int nbStartY, int nbEndY)
{
	FUNCLOG
	int starty = nbStartY / (4 * GPU_TEXWIDTH);
	int endy = (nbEndY + 4 * GPU_TEXWIDTH - 1) / (4 * GPU_TEXWIDTH);
	//int endy = (nbEndY+4096-1) / 4096;

	//if( listTargets.size() < TARGET_THRESH ) {

	for (list<CMemoryTarget>::iterator it = listTargets.begin(); it != listTargets.end();)
	{

		if (it->starty < endy && (it->starty + it->height) > starty)
		{

			// intersects, reduce valid texture mem (or totally delete texture)
			// there are 4 cases
			int miny = max(it->starty, starty);
			int maxy = min(it->starty + it->height, endy);
			assert(miny < maxy);

			if (it->clearmaxy == 0)
			{
				it->clearminy = miny;
				it->clearmaxy = maxy;
			}
			else
			{
				if (it->clearminy > miny) it->clearminy = miny;
				if (it->clearmaxy < maxy) it->clearmaxy = maxy;
			}
		}

		++it;
	}

//  }
//  else {
//	  for(list<CMemoryTarget>::iterator it = listTargets.begin(); it != listTargets.end(); ) {
//
//		  if( it->starty < endy && (it->starty+it->height) > starty ) {
//			  int newstarty = 0;
//			  if( starty <= it->starty ) {
//				  if( endy < it->starty + it->height) {
//					  // preserve end
//					  it->height = it->starty+it->height-endy;
//					  it->starty = endy;
//					  assert(it->height > 0);
//				  }
//				  else {
//					  // destroy
//					  it->height = 0;
//				  }
//			  }
//			  else {
//				  // beginning can be preserved
//				  it->height = starty-it->starty;
//			  }
//
//			  assert( it->starty >= it->realy && it->starty+it->height<=it->realy+it->realheight );
//			  if( it->height <= 0 ) {
//				  list<CMemoryTarget>::iterator itprev = it; ++it;
//				  listClearedTargets.splice(listClearedTargets.end(), listTargets, itprev);
//				  continue;
//			  }
//		  }
//
//		  ++it;
//	  }
//  }
}

void CMemoryTargetMngr::DestroyCleared()
{
	FUNCLOG

	for (list<CMemoryTarget>::iterator it = listClearedTargets.begin(); it != listClearedTargets.end();)
	{
		if (it->usedstamp < curstamp - 2)
		{
			it = listClearedTargets.erase(it);
			continue;
		}

		++it;
	}

	if ((curstamp % 3) == 0)
	{
		// purge old targets every 3 frames
		for (list<CMemoryTarget>::iterator it = listTargets.begin(); it != listTargets.end();)
		{
			if (it->usedstamp < curstamp - 3)
			{
				it = listTargets.erase(it);
				continue;
			}

			++it;
		}
	}

	++curstamp;
}

void CMemoryTargetMngr::DestroyOldest()
{
	FUNCLOG

	if (listTargets.size() == 0)
		return;

	list<CMemoryTarget>::iterator it, itbest;

	it = itbest = listTargets.begin();

	while (it != listTargets.end())
	{
		if (it->usedstamp < itbest->usedstamp) itbest = it;
		++it;
	}

	listTargets.erase(itbest);
}

//////////////////////////////////////
// Texture Mngr For Bitwise AND Ops //
//////////////////////////////////////
void CBitwiseTextureMngr::Destroy()
{
	FUNCLOG

	for (map<u32, u32>::iterator it = mapTextures.begin(); it != mapTextures.end(); ++it)
	{
			glDeleteTextures(1, &it->second);
	}

	mapTextures.clear();
}

u32 CBitwiseTextureMngr::GetTexInt(u32 bitvalue, u32 ptexDoNotDelete)
{
	FUNCLOG

	if (mapTextures.size() > 32)
	{
		// randomly delete 8
		for (map<u32, u32>::iterator it = mapTextures.begin(); it != mapTextures.end();)
		{
			if (!(rand()&3) && it->second != ptexDoNotDelete)
			{
				glDeleteTextures(1, &it->second);
				mapTextures.erase(it++);
			}
			else
			{
				++it;
			}
		}
	}

	if (glGetError() != GL_NO_ERROR) ZZLog::Error_Log("Error before creation of bitmask texture.");

	// create a new tex
	u32 ptex;

	glGenTextures(1, &ptex);

	if (glGetError() != GL_NO_ERROR) ZZLog::Error_Log("Error on generation of bitmask texture.");

	vector<u16> data(GPU_TEXMASKWIDTH);

	for (u32 i = 0; i < GPU_TEXMASKWIDTH; ++i)
	{
		data[i] = (((i << MASKDIVISOR) & bitvalue) << 6); // add the 1/2 offset so that
	}

	//	data[GPU_TEXMASKWIDTH] = 0;	// I remove GPU_TEXMASKWIDTH+1 element of this texture, because it was a reason of FFC crush
									// Probably, some sort of PoT incompability in drivers.

	glBindTexture(GL_TEXTURE_RECTANGLE, ptex);
	if (glGetError() != GL_NO_ERROR) ZZLog::Error_Log("Error on binding bitmask texture.");

	TextureRect2(GL_LUMINANCE16, GPU_TEXMASKWIDTH, 1, GL_LUMINANCE, GL_UNSIGNED_SHORT, &data[0]);
	if (glGetError() != GL_NO_ERROR) ZZLog::Error_Log("Error on applying bitmask texture.");

//	Removing clamping, as it seems lead to numerous troubles at some drivers
//	Need to observe, may be clamping is not really needed.
	/* setRectWrap2(GL_REPEAT);

	GLint Error = glGetError();
	if( Error != GL_NO_ERROR ) {
		ERROR_LOG_SPAM_TEST("Failed to create bitmask texture; \t");
		if (SPAM_PASS) {
			ZZLog::Log("bitmask cache %d; \t",  mapTextures.size());
			switch (Error) {
				case GL_INVALID_ENUM: ZZLog::Error_Log("Invalid enumerator.") ; break;
				case GL_INVALID_VALUE: ZZLog::Error_Log("Invalid value."); break;
				case GL_INVALID_OPERATION: ZZLog::Error_Log("Invalid operation."); break;
				default: ZZLog::Error_Log("Error number: %d.", Error);
			}
		}
		return 0;
	}*/

	mapTextures[bitvalue] = ptex;

	return ptex;
}

void CRangeManager::RangeSanityCheck()
{
#ifdef _DEBUG
	// sanity check

	for (int i = 0; i < (int)ranges.size() - 1; ++i)
	{
		assert(ranges[i].end < ranges[i+1].start);
	}

#endif
}

void CRangeManager::Insert(int start, int end)
{
	FUNCLOG
	int imin = 0, imax = (int)ranges.size(), imid;

	RangeSanityCheck();

	switch (ranges.size())
	{

		case 0:
			ranges.push_back(RANGE(start, end));
			return;

		case 1:
			if (end < ranges.front().start)
			{
				ranges.insert(ranges.begin(), RANGE(start, end));
			}
			else if (start > ranges.front().end)
			{
				ranges.push_back(RANGE(start, end));
			}
			else
			{
				if (start < ranges.front().start) ranges.front().start = start;
				if (end > ranges.front().end) ranges.front().end = end;
			}

			return;
	}

	// find where start is
	while (imin < imax)
	{
		imid = (imin + imax) >> 1;

		assert(imid < (int)ranges.size());

		if ((ranges[imid].end >= start) && ((imid == 0) || (ranges[imid-1].end < start)))
		{
			imin = imid;
			break;
		}
		else if (ranges[imid].start > start)
		{
			imax = imid;
		}
		else
		{
			imin = imid + 1;
		}
	}

	int startindex = imin;

	if (startindex >= (int)ranges.size())
	{
		// non intersecting
		assert(start > ranges.back().end);
		ranges.push_back(RANGE(start, end));
		return;
	}

	if (startindex == 0 && end < ranges.front().start)
	{
		ranges.insert(ranges.begin(), RANGE(start, end));
		RangeSanityCheck();
		return;
	}

	imin = 0;
	imax = (int)ranges.size();

	// find where end is

	while (imin < imax)
	{
		imid = (imin + imax) >> 1;

		assert(imid < (int)ranges.size());

		if ((ranges[imid].end <= end) && ((imid == ranges.size() - 1) || (ranges[imid+1].start > end)))
		{
			imin = imid;
			break;
		}
		else if (ranges[imid].start >= end)
		{
			imax = imid;
		}
		else
		{
			imin = imid + 1;
		}
	}

	int endindex = imin;

	if (startindex > endindex)
	{
		// create a new range
		ranges.insert(ranges.begin() + startindex, RANGE(start, end));
		RangeSanityCheck();
		return;
	}

	if (endindex >= (int)ranges.size() - 1)
	{
		// pop until startindex is reached
		int lastend = ranges.back().end;
		int numpop = (int)ranges.size() - startindex - 1;

		while (numpop-- > 0)
		{
			ranges.pop_back();
		}

		assert(start <= ranges.back().end);

		if (start < ranges.back().start) ranges.back().start = start;
		if (lastend > ranges.back().end) ranges.back().end = lastend;
		if (end > ranges.back().end) ranges.back().end = end;

		RangeSanityCheck();

		return;
	}

	if (endindex == 0)
	{
		assert(end >= ranges.front().start);

		if (start < ranges.front().start) ranges.front().start = start;
		if (end > ranges.front().end) ranges.front().end = end;

		RangeSanityCheck();
	}

	// somewhere in the middle
	if (ranges[startindex].start < start) start = ranges[startindex].start;

	if (startindex < endindex)
	{
		ranges.erase(ranges.begin() + startindex, ranges.begin() + endindex);
	}

	if (start < ranges[startindex].start) ranges[startindex].start = start;
	if (end > ranges[startindex].end) ranges[startindex].end = end;

	RangeSanityCheck();
}

CRangeManager s_RangeMngr; // manages overwritten memory

void ResolveInRange(int start, int end)
{
	FUNCLOG
	list<CRenderTarget*> listTargs = CreateTargetsList(start, end);
	/*	s_DepthRTs.GetTargs(start, end, listTargs);
		s_RTs.GetTargs(start, end, listTargs);*/

	if (listTargs.size() > 0)
	{
		FlushBoth();

		// We need another list, because old one could be brocken by Flush().
		listTargs.clear();
		listTargs = CreateTargetsList(start, end);
		/*		s_DepthRTs.GetTargs(start, end, listTargs_1);
				s_RTs.GetTargs(start, end, listTargs_1);*/

		for (list<CRenderTarget*>::iterator it = listTargs.begin(); it != listTargs.end(); ++it)
		{
			// only resolve if not completely covered
			if ((*it)->created == 123)
				(*it)->Resolve();
			else
				ZZLog::Debug_Log("Resolving non-existing object! Destroy code %d.", (*it)->created);
		}
	}
}

//////////////////
// Transferring //
//////////////////
void FlushTransferRanges(const tex0Info* ptex)
{
	FUNCLOG
	assert(s_RangeMngr.ranges.size() > 0);
	//bool bHasFlushed = false;
	list<CRenderTarget*> listTransmissionUpdateTargs;

	int texstart = -1, texend = -1;

	if (ptex != NULL)
	{
		GetRectMemAddress(texstart, texend, ptex->psm, 0, 0, ptex->tw, ptex->th, ptex->tbp0, ptex->tbw);
	}

	for (vector<CRangeManager::RANGE>::iterator itrange = s_RangeMngr.ranges.begin(); itrange != s_RangeMngr.ranges.end(); ++itrange)
	{

		int start = itrange->start;
		int end = itrange->end;

		listTransmissionUpdateTargs.clear();
		listTransmissionUpdateTargs = CreateTargetsList(start, end);

		/*		s_DepthRTs.GetTargs(start, end, listTransmissionUpdateTargs);
				s_RTs.GetTargs(start, end, listTransmissionUpdateTargs);*/

//	  if( !bHasFlushed && listTransmissionUpdateTargs.size() > 0 ) {
//		  FlushBoth();
//
//#ifdef _DEBUG
//		  // make sure targets are still the same
//		  list<CRenderTarget*>::iterator it;
//		  FORIT(it, listTransmissionUpdateTargs) {
//			  CRenderTargetMngr::MAPTARGETS::iterator itmap;
//			  for(itmap = s_RTs.mapTargets.begin(); itmap != s_RTs.mapTargets.end(); ++itmap) {
//				  if( itmap->second == *it )
//					  break;
//			  }
//
//			  if( itmap == s_RTs.mapTargets.end() ) {
//
//				  for(itmap = s_DepthRTs.mapTargets.begin(); itmap != s_DepthRTs.mapTargets.end(); ++itmap) {
//					  if( itmap->second == *it )
//						  break;
//				  }
//
//				  assert( itmap != s_DepthRTs.mapTargets.end() );
//			  }
//		  }
//#endif
//	  }

		for (list<CRenderTarget*>::iterator it = listTransmissionUpdateTargs.begin(); it != listTransmissionUpdateTargs.end(); ++it)
		{

			CRenderTarget* ptarg = *it;

			if ((ptarg->status & CRenderTarget::TS_Virtual)) continue;

			if (!(ptarg->start < texend && ptarg->end > texstart))
			{
				// check if target is currently being used

				if (!(conf.settings().no_quick_resolve))
				{
					if (ptarg->fbp != vb[0].gsfb.fbp)   //&& (vb[0].prndr == NULL || ptarg->fbp != vb[0].prndr->fbp) ) {
					{
						if (ptarg->fbp != vb[1].gsfb.fbp)    //&& (vb[1].prndr == NULL || ptarg->fbp != vb[1].prndr->fbp) ) {
						{
							// this render target currently isn't used and is not in the texture's way, so can safely ignore
							// resolving it. Also the range has to be big enough compared to the target to really call it resolved
							// (ffx changing screens, shadowhearts)
							// start == ptarg->start, used for kh to transfer text

							if (ptarg->IsDepth() || end - start > 0x50000 || ((conf.settings().quick_resolve_1) && start == ptarg->start))
								ptarg->status |= CRenderTarget::TS_NeedUpdate | CRenderTarget::TS_Resolved;

							continue;
						}
					}
				}
			}
			else
			{
//			  if( start <= texstart && end >= texend ) {
//				  // texture taken care of so can skip!?
//				  continue;
//			  }
			}

			// the first range check was very rough; some games (dragonball z) have the zbuf in the same page as textures (but not overlapping)
			// so detect that condition
			if (ptarg->fbh % m_Blocks[ptarg->psm].height)
			{

				// get start of left-most boundry page
				int targstart, targend;
				GetRectMemAddress(targstart, targend, ptarg->psm, 0, 0, ptarg->fbw, ptarg->fbh & ~(m_Blocks[ptarg->psm].height - 1), ptarg->fbp, ptarg->fbw);

				if (start >= targend)
				{
					// don't bother
					if ((ptarg->fbh % m_Blocks[ptarg->psm].height) <= 2) continue;

					// calc how many bytes of the block that the page spans
				}
			}

			if (!(ptarg->status & CRenderTarget::TS_Virtual))
			{

				if (start < ptarg->end && end > ptarg->start)
				{

					// suikoden5 is faster with check, but too big of a value and kh screens mess up
					/* Zeydlitz remove this check, it does not do anything good
					if ((end - start > 0x8000) && (!(conf.settings() & GAME_GUSTHACK) || (end-start > 0x40000))) {
						// intersects, do only one sided resolves
						if( end-start > 4*ptarg->fbw ) { // at least it be greater than one scanline (spiro is faster)
							if( start > ptarg->start ) {
								ptarg->Resolve(ptarg->start, start);

							}
							else if( end < ptarg->end ) {
								ptarg->Resolve(end, ptarg->end);
							}
						}
					}*/

					ptarg->status |= CRenderTarget::TS_Resolved;

					if ((!ptarg->IsDepth() || (!(conf.settings().no_depth_update) || end - start > 0x1000)) && ((end - start > 0x40000) || !(conf.settings().gust)))
						ptarg->status |= CRenderTarget::TS_NeedUpdate;
				}
			}
		}

		g_MemTargs.ClearRange(start, end);
	}

	s_RangeMngr.Clear();
}


#if 0
// I removed some code here that wasn't getting called. The old versions #if'ed out below this.
#define RESOLVE_32_BIT(PSM, T, Tsrc, convfn) \
	{ \
		u32 mask, imask; \
		\
		if (PSMT_ISHALF(psm)) /* 16 bit */ \
		{\
			/* mask is shifted*/ \
			imask = RGBA32to16(fbm);\
			mask = (~imask)&0xffff;\
		}\
		else \
		{\
			mask = ~fbm;\
			imask = fbm;\
		}\
		\
		Tsrc* src = (Tsrc*)(psrc); \
		T* pPageOffset = (T*)g_pbyGSMemory + fbp*(256/sizeof(T)), *dst; \
		int maxfbh = (MEMORY_END-fbp*256) / (sizeof(T) * fbw); \
		if( maxfbh > fbh ) maxfbh = fbh; \
		\
		for(int i = 0; i < maxfbh; ++i) { \
			for(int j = 0; j < fbw; ++j) { \
				T dsrc = convfn(src[RW(j)]); \
				dst = pPageOffset + getPixelAddress##PSM##_0(j, i, fbw); \
				*dst = (dsrc & mask) | (*dst & imask); \
			} \
			src += RH(Pitch(fbw))/sizeof(Tsrc); \
		} \
	} \

#endif

//#define LOG_RESOLVE_PROFILE

template <typename Tdst, bool do_conversion>
inline void Resolve_32_Bit(const void* psrc, int fbp, int fbw, int fbh, const int psm, u32 fbm)
{
    u32 mask, imask;
#ifdef LOG_RESOLVE_PROFILE
#ifdef __LINUX__
     u32 startime = timeGetPreciseTime();
#endif
#endif

    if (PSMT_ISHALF(psm)) /* 16 bit */
    {
        /* mask is shifted*/
        imask = RGBA32to16(fbm);
        mask = (~imask)&0xffff;
    }
    else
    {
        mask = ~fbm;
        imask = fbm;
    }

    Tdst* pPageOffset = (Tdst*)g_pbyGSMemory + fbp*(256/sizeof(Tdst));
    Tdst* dst;
    Tdst  dsrc;

    int maxfbh = (MEMORY_END-fbp*256) / (sizeof(Tdst) * fbw);
    if( maxfbh > fbh ) maxfbh = fbh;
    
#ifdef LOG_RESOLVE_PROFILE
    ZZLog::Dev_Log("*** Resolve 32 bits: %dx%d in %x", maxfbh, fbw, psm);
#endif

    // Start the src array at the end to reduce testing in loop
    u32 raw_size = RH(Pitch(fbw))/sizeof(u32);
    u32* src = (u32*)(psrc) + (maxfbh-1)*raw_size;

    for(int i = maxfbh-1; i >= 0; --i) {
        for(int j = fbw-1; j >= 0; --j) {
            if (do_conversion) {
                dsrc = RGBA32to16(src[RW(j)]);
            } else {
                dsrc = (Tdst)src[RW(j)];
            }
            // They are 3 methods to call the functions
            // macro (compact, inline) but need a nice psm ; swich (inline) ; function pointer (compact)
            // Use a switch to allow inlining of the getPixel function.
            // Note: psm is const so the switch is completely optimized
            // Function method example:
            // dst = pPageOffset + getPixelFun_0[psm](j, i, fbw);
            switch (psm)
            {
                case PSMCT32:
                case PSMCT24:
                    dst = pPageOffset + getPixelAddress32_0(j, i, fbw);
                    break;

                case PSMCT16:
                    dst = pPageOffset + getPixelAddress16_0(j, i, fbw);
                    break;

                case PSMCT16S:
                    dst = pPageOffset + getPixelAddress16S_0(j, i, fbw);
                    break;

                case PSMT32Z:
                case PSMT24Z:
                    dst = pPageOffset + getPixelAddress32Z_0(j, i, fbw);
                    break;

                case PSMT16Z:
                    dst = pPageOffset + getPixelAddress16Z_0(j, i, fbw);
                    break;

                case PSMT16SZ:
                    dst = pPageOffset + getPixelAddress16SZ_0(j, i, fbw);
                    break;
            }
            *dst = (dsrc & mask) | (*dst & imask);
        }
        src -= raw_size;
    }
#ifdef LOG_RESOLVE_PROFILE
#ifdef __LINUX__
    ZZLog::Dev_Log("*** 32 bits: execution time %d", timeGetPreciseTime()-startime);
#endif
#endif
}

static const __aligned16 unsigned int pixel_5b_mask[4] = {0x0000001F, 0x0000001F, 0x0000001F, 0x0000001F};

#ifdef ZEROGS_SSE2
// The function process 2*2 pixels in 32bits. And 2*4 pixels in 16bits
template <u32 psm, u32 size, u32 pageTable[size][64], bool null_second_line, u32 INDEX>
__forceinline void update_8pixels_sse2(u32* src, u32* basepage, u32 i_msk, u32 j, u32 pix_mask, u32 src_pitch)
{
    u32* base_ptr;
    __m128i pixels_0;
    __m128i pixel_0_low;
    __m128i pixel_0_high;

    __m128i pixels_1;
    __m128i pixel_1_low;
    __m128i pixel_1_high;

    assert((i_msk&0x1) == 0); // Failure => wrong line selected

    // Note: pixels have a special arrangement in column. Here a short description when AA.x = 0
    //
    // 32 bits format: 8x2 pixels: the idea is to read pixels 0-3
    // It is easier to process 4 bits (we can not cross column bondary)
    //      0 1 4 5 8  9  12 13
    //      2 3 6 7 10 11 14 15
    //
    // 16 bits format: 16x2 pixels, each pixels have a lower and higher part.
    // Here the idea to read 0L-3L & 0H-3H to combine lower and higher part this avoid
    // data interleaving and useless read/write
    //      0L 1L 4L 5L 8L  9L  12L 13L  0H 1H 4H 5H 8H  9H  12H 13H
    //      2L 3L 6L 7L 10L 11L 14L 15L  2H 3H 6H 7H 10H 11H 14H 15H
    //
    if (AA.x == 2) {
        // Note: pixels (32bits) are stored like that:
        // p0 p0 p0 p0  p1 p1 p1 p1  p4 p4 p4 p4  p5 p5 p5 p5
        // ...
        // p2 p2 p2 p2  p3 p3 p3 p3  p6 p6 p6 p6  p7 p7 p7 p7
        base_ptr = &src[((j+INDEX)<<2)];
        pixel_0_low = _mm_loadl_epi64((__m128i*)(base_ptr + 3));
        if (!null_second_line) pixel_0_high = _mm_loadl_epi64((__m128i*)(base_ptr + 3 + src_pitch));

        if (PSMT_ISHALF(psm)) {
            pixel_1_low = _mm_loadl_epi64((__m128i*)(base_ptr + 3 + 32));
            if (!null_second_line) pixel_1_high = _mm_loadl_epi64((__m128i*)(base_ptr + 3 + 32 + src_pitch));
        }
    } else if(AA.x ==1) {
        // Note: pixels (32bits) are stored like that:
        // p0 p0 p1 p1  p4 p4 p5 p5
        // ...
        // p2 p2 p3 p3  p6 p6 p7 p7
        base_ptr = &src[((j+INDEX)<<1)];
        pixel_0_low = _mm_loadl_epi64((__m128i*)(base_ptr + 1));
        if (!null_second_line) pixel_0_high = _mm_loadl_epi64((__m128i*)(base_ptr + 1 + src_pitch));

        if (PSMT_ISHALF(psm)) {
            pixel_1_low = _mm_loadl_epi64((__m128i*)(base_ptr + 1 + 16));
            if (!null_second_line) pixel_1_high = _mm_loadl_epi64((__m128i*)(base_ptr + 1 + 16 + src_pitch));
        }
    } else {
        // Note: pixels (32bits) are stored like that:
        // p0 p1  p4 p5
        // p2 p3  p6 p7
        base_ptr = &src[(j+INDEX)];
        pixel_0_low = _mm_loadl_epi64((__m128i*)base_ptr);
        if (!null_second_line) pixel_0_high = _mm_loadl_epi64((__m128i*)(base_ptr + src_pitch));

        if (PSMT_ISHALF(psm)) {
            pixel_1_low = _mm_loadl_epi64((__m128i*)(base_ptr + 8));
            if (!null_second_line) pixel_1_high = _mm_loadl_epi64((__m128i*)(base_ptr + 8 + src_pitch));
        }
    }

    // 2nd line does not exist... Just duplicate the pixel value
    if(null_second_line) {
        pixel_0_high = pixel_0_low;
        if (PSMT_ISHALF(psm)) pixel_1_high = pixel_1_low;
    }

    // Merge the 2 dword
    pixels_0 = _mm_unpacklo_epi64(pixel_0_low, pixel_0_high);
    if (PSMT_ISHALF(psm)) pixels_1 = _mm_unpacklo_epi64(pixel_1_low, pixel_1_high);

    // transform pixel from ARGB:8888 to ARGB:1555
    if (psm == PSMCT16 || psm == PSMCT16S) {
        // shift pixel instead of the mask. It allow to keep 1 mask into a register
        // instead of 4 (not enough room on x86...).
        __m128i pixel_mask = _mm_load_si128((__m128i*)pixel_5b_mask);

        __m128i pixel_0_B = _mm_srli_epi32(pixels_0, 3);
        pixel_0_B = _mm_and_si128(pixel_0_B, pixel_mask);

        __m128i pixel_0_G = _mm_srli_epi32(pixels_0, 11);
        pixel_0_G = _mm_and_si128(pixel_0_G, pixel_mask);

        __m128i pixel_0_R = _mm_srli_epi32(pixels_0, 19);
        pixel_0_R = _mm_and_si128(pixel_0_R, pixel_mask);

        // Note: because of the logical shift we do not need to mask the value
        __m128i pixel_0_A = _mm_srli_epi32(pixels_0, 31);

        // Realignment of pixels
        pixel_0_A = _mm_slli_epi32(pixel_0_A, 15);
        pixel_0_R = _mm_slli_epi32(pixel_0_R, 10);
        pixel_0_G = _mm_slli_epi32(pixel_0_G, 5);

        // rebuild a complete pixel
        pixels_0 = _mm_or_si128(pixel_0_A, pixel_0_B);
        pixels_0 = _mm_or_si128(pixels_0, pixel_0_G);
        pixels_0 = _mm_or_si128(pixels_0, pixel_0_R);

        // do the same for pixel_1
        __m128i pixel_1_B = _mm_srli_epi32(pixels_1, 3);
        pixel_1_B = _mm_and_si128(pixel_1_B, pixel_mask);

        __m128i pixel_1_G = _mm_srli_epi32(pixels_1, 11);
        pixel_1_G = _mm_and_si128(pixel_1_G, pixel_mask);

        __m128i pixel_1_R = _mm_srli_epi32(pixels_1, 19);
        pixel_1_R = _mm_and_si128(pixel_1_R, pixel_mask);

        __m128i pixel_1_A = _mm_srli_epi32(pixels_1, 31);

        // Realignment of pixels
        pixel_1_A = _mm_slli_epi32(pixel_1_A, 15);
        pixel_1_R = _mm_slli_epi32(pixel_1_R, 10);
        pixel_1_G = _mm_slli_epi32(pixel_1_G, 5);

        // rebuild a complete pixel
        pixels_1 = _mm_or_si128(pixel_1_A, pixel_1_B);
        pixels_1 = _mm_or_si128(pixels_1, pixel_1_G);
        pixels_1 = _mm_or_si128(pixels_1, pixel_1_R);
    }

    // Move the pixels to higher parts and merge it with pixels_0
    if (PSMT_ISHALF(psm)) {
        pixels_1 = _mm_slli_epi32(pixels_1, 16);
        pixels_0 = _mm_or_si128(pixels_0, pixels_1);
    }

    // Status 16 bits
    // pixels_0 = p3H p3L p2H p2L  p1H p1L p0H p0L
    // Status 32 bits
    // pixels_0 = p3 p2 p1 p0

    // load the destination add
    u32* dst_add;
    if (PSMT_ISHALF(psm))
        dst_add = basepage + (pageTable[i_msk][(INDEX)] >> 1);
    else
        dst_add = basepage + pageTable[i_msk][(INDEX)];

    // Save some memory access when pix_mask is 0.
    if (pix_mask) {
        // Build fbm mask (tranform a u32 to a 4 packets u32)
        // In 16 bits texture one packet is "0000 DATA"
        __m128i imask = _mm_cvtsi32_si128(pix_mask);
        imask = _mm_shuffle_epi32(imask, 0);

        // apply the mask on new values
        pixels_0 = _mm_andnot_si128(imask, pixels_0);

        __m128i old_pixels_0;
        __m128i final_pixels_0;

        old_pixels_0 = _mm_and_si128(imask, _mm_load_si128((__m128i*)dst_add));
        final_pixels_0 = _mm_or_si128(old_pixels_0, pixels_0);

        _mm_store_si128((__m128i*)dst_add, final_pixels_0);
    } else {
        // Note: because we did not read the previous value of add. We could bypass the cache.
        // We gains a few percents
        _mm_stream_si128((__m128i*)dst_add, pixels_0);
    }

}

// Update 2 lines of a page (2*64 pixels)
template <u32 psm, u32 size, u32 pageTable[size][64], bool null_second_line>
__forceinline void update_pixels_row_sse2(u32* src, u32* basepage, u32 i_msk, u32 j, u32 pix_mask, u32 raw_size)
{
    update_8pixels_sse2<psm, size, pageTable, null_second_line, 0>(src, basepage, i_msk, j, pix_mask, raw_size);
    update_8pixels_sse2<psm, size, pageTable, null_second_line, 2>(src, basepage, i_msk, j, pix_mask, raw_size);
    update_8pixels_sse2<psm, size, pageTable, null_second_line, 4>(src, basepage, i_msk, j, pix_mask, raw_size);
    update_8pixels_sse2<psm, size, pageTable, null_second_line, 6>(src, basepage, i_msk, j, pix_mask, raw_size);

    if(!PSMT_ISHALF(psm)) {
        update_8pixels_sse2<psm, size, pageTable, null_second_line, 8>(src, basepage, i_msk, j, pix_mask, raw_size);
        update_8pixels_sse2<psm, size, pageTable, null_second_line, 10>(src, basepage, i_msk, j, pix_mask, raw_size);
        update_8pixels_sse2<psm, size, pageTable, null_second_line, 12>(src, basepage, i_msk, j, pix_mask, raw_size);
        update_8pixels_sse2<psm, size, pageTable, null_second_line, 14>(src, basepage, i_msk, j, pix_mask, raw_size);
    }

    update_8pixels_sse2<psm, size, pageTable, null_second_line, 16>(src, basepage, i_msk, j, pix_mask, raw_size);
    update_8pixels_sse2<psm, size, pageTable, null_second_line, 18>(src, basepage, i_msk, j, pix_mask, raw_size);
    update_8pixels_sse2<psm, size, pageTable, null_second_line, 20>(src, basepage, i_msk, j, pix_mask, raw_size);
    update_8pixels_sse2<psm, size, pageTable, null_second_line, 22>(src, basepage, i_msk, j, pix_mask, raw_size);

    if(!PSMT_ISHALF(psm)) {
        update_8pixels_sse2<psm, size, pageTable, null_second_line, 24>(src, basepage, i_msk, j, pix_mask, raw_size);
        update_8pixels_sse2<psm, size, pageTable, null_second_line, 26>(src, basepage, i_msk, j, pix_mask, raw_size);
        update_8pixels_sse2<psm, size, pageTable, null_second_line, 28>(src, basepage, i_msk, j, pix_mask, raw_size);
        update_8pixels_sse2<psm, size, pageTable, null_second_line, 30>(src, basepage, i_msk, j, pix_mask, raw_size);
    }

    update_8pixels_sse2<psm, size, pageTable, null_second_line, 32>(src, basepage, i_msk, j, pix_mask, raw_size);
    update_8pixels_sse2<psm, size, pageTable, null_second_line, 34>(src, basepage, i_msk, j, pix_mask, raw_size);
    update_8pixels_sse2<psm, size, pageTable, null_second_line, 36>(src, basepage, i_msk, j, pix_mask, raw_size);
    update_8pixels_sse2<psm, size, pageTable, null_second_line, 38>(src, basepage, i_msk, j, pix_mask, raw_size);

    if(!PSMT_ISHALF(psm)) {
        update_8pixels_sse2<psm, size, pageTable, null_second_line, 40>(src, basepage, i_msk, j, pix_mask, raw_size);
        update_8pixels_sse2<psm, size, pageTable, null_second_line, 42>(src, basepage, i_msk, j, pix_mask, raw_size);
        update_8pixels_sse2<psm, size, pageTable, null_second_line, 44>(src, basepage, i_msk, j, pix_mask, raw_size);
        update_8pixels_sse2<psm, size, pageTable, null_second_line, 46>(src, basepage, i_msk, j, pix_mask, raw_size);
    }

    update_8pixels_sse2<psm, size, pageTable, null_second_line, 48>(src, basepage, i_msk, j, pix_mask, raw_size);
    update_8pixels_sse2<psm, size, pageTable, null_second_line, 50>(src, basepage, i_msk, j, pix_mask, raw_size);
    update_8pixels_sse2<psm, size, pageTable, null_second_line, 52>(src, basepage, i_msk, j, pix_mask, raw_size);
    update_8pixels_sse2<psm, size, pageTable, null_second_line, 54>(src, basepage, i_msk, j, pix_mask, raw_size);

    if(!PSMT_ISHALF(psm)) {
        update_8pixels_sse2<psm, size, pageTable, null_second_line, 56>(src, basepage, i_msk, j, pix_mask, raw_size);
        update_8pixels_sse2<psm, size, pageTable, null_second_line, 58>(src, basepage, i_msk, j, pix_mask, raw_size);
        update_8pixels_sse2<psm, size, pageTable, null_second_line, 60>(src, basepage, i_msk, j, pix_mask, raw_size);
        update_8pixels_sse2<psm, size, pageTable, null_second_line, 62>(src, basepage, i_msk, j, pix_mask, raw_size);
    }
}

template <u32 psm, u32 size, u32 pageTable[size][64]>
void Resolve_32_Bit_sse2(const void* psrc, int fbp, int fbw, int fbh, u32 fbm)
{
    // Note a basic implementation was done in Resolve_32_Bit function
#ifdef LOG_RESOLVE_PROFILE
#ifdef __LINUX__
    u32 startime = timeGetPreciseTime();
#endif
#endif
    u32 pix_mask;
    if (PSMT_ISHALF(psm)) /* 16 bit format */
    {
        /* Use 2 16bits mask */
        u32 pix16_mask = RGBA32to16(fbm);
        pix_mask = (pix16_mask<<16) | pix16_mask;
    }
    else
        pix_mask = fbm;

    // Note GS register: frame_register__fbp is specified in units of the 32 bits address divided by 2048
    // fbp is stored as 32*frame_register__fbp
    u32* pPageOffset = (u32*)g_pbyGSMemory + (fbp/32)*2048;

    int maxfbh;
    int memory_space = MEMORY_END-(fbp/32)*2048*4;
    if (PSMT_ISHALF(psm))
        maxfbh = memory_space / (2*fbw);
    else
        maxfbh = memory_space / (4*fbw);

    if( maxfbh > fbh ) maxfbh = fbh;
    
#ifdef LOG_RESOLVE_PROFILE
    ZZLog::Dev_Log("*** Resolve 32 to 32 bits: %dx%d. Frame Mask %x. Format %x", maxfbh, fbw, pix_mask, psm);
#endif

    // Start the src array at the end to reduce testing in loop
    // If maxfbh is odd, proces maxfbh -1 alone and then go back to maxfbh -3
    u32 raw_size = RH(Pitch(fbw))/sizeof(u32);
    u32* src;
    if (maxfbh&0x1) {
        ZZLog::Dev_Log("*** Warning resolve 32bits have an odd number of lines");

        // decrease maxfbh to process the bottom line (maxfbh-1)
        maxfbh--;

        src = (u32*)(psrc) + maxfbh*raw_size;
        u32 i_msk = maxfbh & (size-1);
        // Note fbw is a multiple of 64. So you can unroll the loop 64 times
        for(int j = (fbw - 64); j >= 0; j -= 64) {
            u32* basepage = pPageOffset + ((maxfbh/size) * (fbw/64) + (j/64)) * 2048;
            update_pixels_row_sse2<psm, size, pageTable, true>(src, basepage, i_msk, j, pix_mask, raw_size);
        }
        // realign the src pointer to process others lines
        src -= 2*raw_size;
    } else {
        // Because we process 2 lines at once go back to maxfbh-2.
        src = (u32*)(psrc) + (maxfbh-2)*raw_size;
    }

    // Note i must be even for the update_8pixels functions
    assert((maxfbh&0x1) == 0);
    for(int i = (maxfbh-2); i >= 0; i -= 2) {
        u32 i_msk = i & (size-1);
        // Note fbw is a multiple of 64. So you can unroll the loop 64 times
        for(int j = (fbw - 64); j >= 0; j -= 64) {
            u32* basepage = pPageOffset + ((i/size) * (fbw/64) + (j/64)) * 2048;
            update_pixels_row_sse2<psm, size, pageTable, false>(src, basepage, i_msk, j, pix_mask, raw_size);
        }

        // Note update_8pixels process 2 lines at onces hence the factor 2
        src -= 2*raw_size;
    }

    if(!pix_mask) {
        // Ensure that previous (out of order) write are done. It must be done after non temporal instruction
        // (or *_stream_* intrinsic)
        _mm_sfence();
    }

#ifdef LOG_RESOLVE_PROFILE
#ifdef __LINUX__
    ZZLog::Dev_Log("*** 32 bits: execution time %d", timeGetPreciseTime()-startime);
#endif
#endif
}
#endif

void _Resolve(const void* psrc, int fbp, int fbw, int fbh, int psm, u32 fbm, bool mode = true)
{
	FUNCLOG

	int start, end;

	s_nResolved += 2;

	// align the rect to the nearest page
	// note that fbp is always aligned on page boundaries
	GetRectMemAddress(start, end, psm, 0, 0, fbw, fbh, fbp, fbw);

    // Comment this to restore the previous resolve_32 version
#define OPTI_RESOLVE_32
    // start the conversion process A8R8G8B8 -> psm
    switch (psm)
    {

        // NOTE pass psm as a constant value otherwise gcc does not do its job. It keep
        // the psm switch in Resolve_32_Bit
        case PSMCT32:
        case PSMCT24:
#if defined(ZEROGS_SSE2) && defined(OPTI_RESOLVE_32)
            Resolve_32_Bit_sse2<PSMCT32, 32, g_pageTable32 >(psrc, fbp, fbw, fbh, fbm);
#else
            Resolve_32_Bit<u32, false >(psrc, fbp, fbw, fbh, PSMCT32, fbm);
#endif
            break;

        case PSMCT16:
#if defined(ZEROGS_SSE2) && defined(OPTI_RESOLVE_32)
            Resolve_32_Bit_sse2<PSMCT16, 64, g_pageTable16 >(psrc, fbp, fbw, fbh, fbm);
#else
            Resolve_32_Bit<u16, true >(psrc, fbp, fbw, fbh, PSMCT16, fbm);
#endif
            break;

        case PSMCT16S:
#if defined(ZEROGS_SSE2) && defined(OPTI_RESOLVE_32)
            Resolve_32_Bit_sse2<PSMCT16S, 64, g_pageTable16S >(psrc, fbp, fbw, fbh, fbm);
#else
            Resolve_32_Bit<u16, true >(psrc, fbp, fbw, fbh, PSMCT16S, fbm);
#endif
            break;

        case PSMT32Z:
        case PSMT24Z:
#if defined(ZEROGS_SSE2) && defined(OPTI_RESOLVE_32)
            Resolve_32_Bit_sse2<PSMT32Z, 32, g_pageTable32Z >(psrc, fbp, fbw, fbh, fbm);
#else
            Resolve_32_Bit<u32, false >(psrc, fbp, fbw, fbh, PSMT32Z, fbm);
#endif
            break;

        case PSMT16Z:
#if defined(ZEROGS_SSE2) && defined(OPTI_RESOLVE_32)
            Resolve_32_Bit_sse2<PSMT16Z, 64, g_pageTable16Z >(psrc, fbp, fbw, fbh, fbm);
#else
            Resolve_32_Bit<u16, false >(psrc, fbp, fbw, fbh, PSMT16Z, fbm);
#endif
            break;

        case PSMT16SZ:
#if defined(ZEROGS_SSE2) && defined(OPTI_RESOLVE_32)
            Resolve_32_Bit_sse2<PSMT16SZ, 64, g_pageTable16SZ >(psrc, fbp, fbw, fbh, fbm);
#else
            Resolve_32_Bit<u16, false >(psrc, fbp, fbw, fbh, PSMT16SZ, fbm);
#endif
            break;
    }

	g_MemTargs.ClearRange(start, end);

	INC_RESOLVE();
}

// Leaving this code in for reference for the moment.
#if 0
void _Resolve(const void* psrc, int fbp, int fbw, int fbh, int psm, u32 fbm, bool mode)
{
	FUNCLOG
	//GL_REPORT_ERRORD();
	s_nResolved += 2;

	// align the rect to the nearest page
	// note that fbp is always aligned on page boundaries
	int start, end;
	GetRectMemAddress(start, end, psm, 0, 0, fbw, fbh, fbp, fbw);

	int i, j;
	//short smask1 = gs.smask&1;
	//short smask2 = gs.smask&2;
	u32 mask, imask;

	if (PSMT_ISHALF(psm))   // 16 bit
	{
		// mask is shifted
		imask = RGBA32to16(fbm);
		mask = (~imask) & 0xffff;
	}
	else
	{
		mask = ~fbm;
		imask = fbm;

		if ((psm&0xf) > 0  && 0)
		{
			// preserve the alpha?
			mask &= 0x00ffffff;
			imask |= 0xff000000;
		}
	}

	// Targets over 2000 should be shuffle. FFX and KH2 (0x2100)
	int X = (psm == 0) ? 0 : 0;

//if (X == 1)
//ZZLog::Error_Log("resolve: %x %x %x %x (%x-%x).", psm, fbp, fbw, fbh, start, end);


#define RESOLVE_32BIT(psm, T, Tsrc, blockbits, blockwidth, blockheight, convfn, frame, aax, aay) \
	{ \
		Tsrc* src = (Tsrc*)(psrc); \
		T* pPageOffset = (T*)g_pbyGSMemory + fbp*(256/sizeof(T)), *dst; \
		int srcpitch = Pitch(fbw) * blockheight/sizeof(Tsrc); \
		int maxfbh = (MEMORY_END-fbp*256) / (sizeof(T) * fbw); \
		if( maxfbh > fbh ) maxfbh = fbh; \
		for(i = 0; i < (maxfbh&~(blockheight-1))*X; i += blockheight) { \
			/*if( smask2 && (i&1) == smask1 ) continue; */ \
			for(j = 0; j < fbw; j += blockwidth) { \
				/* have to write in the tiled format*/ \
				frame##SwizzleBlock##blockbits(pPageOffset + getPixelAddress##psm##_0(j, i, fbw), \
					src+RW(j), Pitch(fbw)/sizeof(Tsrc), mask); \
			} \
			src += RH(srcpitch); \
		} \
		for(; i < maxfbh; ++i) { \
			for(j = 0; j < fbw; ++j) { \
				T dsrc = convfn(src[RW(j)]); \
				dst = pPageOffset + getPixelAddress##psm##_0(j, i, fbw); \
				*dst = (dsrc & mask) | (*dst & imask); \
			} \
			src += RH(Pitch(fbw))/sizeof(Tsrc); \
		} \
	} \

	if( GetRenderFormat() == RFT_byte8 ) {
		// start the conversion process A8R8G8B8 -> psm
		switch (psm)
		{

			case PSMCT32:

			case PSMCT24:

				if (AA.y)
				{
					RESOLVE_32BIT(32, u32, u32, 32A4, 8, 8, (u32), Frame, AA.x, AA.y);
				}
				else if (AA.x)
				{
					RESOLVE_32BIT(32, u32, u32, 32A2, 8, 8, (u32), Frame, 1, 0);
				}
				else
				{
					RESOLVE_32BIT(32, u32, u32, 32, 8, 8, (u32), Frame, 0, 0);
				}

				break;

			case PSMCT16:

				if (AA.y)
				{
					RESOLVE_32BIT(16, u16, u32, 16A4, 16, 8, RGBA32to16, Frame, AA.x, AA.y);
				}
				else if (AA.x)
				{
					RESOLVE_32BIT(16, u16, u32, 16A2, 16, 8, RGBA32to16, Frame, 1, 0);
				}
				else
				{
					RESOLVE_32BIT(16, u16, u32, 16, 16, 8, RGBA32to16, Frame, 0, 0);
				}

				break;

			case PSMCT16S:

				if (AA.y)
				{
					RESOLVE_32BIT(16S, u16, u32, 16A4, 16, 8, RGBA32to16, Frame, AA.x, AA.y);
				}
				else if (AA.x)
				{
					RESOLVE_32BIT(16S, u16, u32, 16A2, 16, 8, RGBA32to16, Frame, 1, 0);
				}
				else
				{
					RESOLVE_32BIT(16S, u16, u32, 16, 16, 8, RGBA32to16, Frame, 0, 0);
				}

				break;

			case PSMT32Z:

			case PSMT24Z:

				if (AA.y)
				{
					RESOLVE_32BIT(32Z, u32, u32, 32A4, 8, 8, (u32), Frame, AA.x, AA.y);
				}
				else if (AA.x)
				{
					RESOLVE_32BIT(32Z, u32, u32, 32A2, 8, 8, (u32), Frame, 1, 0);
				}
				else
				{
					RESOLVE_32BIT(32Z, u32, u32, 32, 8, 8, (u32), Frame, 0, 0);
				}

				break;

			case PSMT16Z:

				if (AA.y)
				{
					RESOLVE_32BIT(16Z, u16, u32, 16A4, 16, 8, (u16), Frame, AA.x, AA.y);
				}
				else if (AA.x)
				{
					RESOLVE_32BIT(16Z, u16, u32, 16A2, 16, 8, (u16), Frame, 1, 0);
				}
				else
				{
					RESOLVE_32BIT(16Z, u16, u32, 16, 16, 8, (u16), Frame, 0, 0);
				}

				break;

			case PSMT16SZ:

				if (AA.y)
				{
					RESOLVE_32BIT(16SZ, u16, u32, 16A4, 16, 8, (u16), Frame, AA.x, AA.y);
				}
				else if (AA.x)
				{
					RESOLVE_32BIT(16SZ, u16, u32, 16A2, 16, 8, (u16), Frame, 1, 0);
				}
				else
				{
					RESOLVE_32BIT(16SZ, u16, u32, 16, 16, 8, (u16), Frame, 0, 0);
				}

				break;
		}
	}
	else   // float16
	{
		switch (psm)
		{

			case PSMCT32:

			case PSMCT24:

				if (AA.y)
				{
					RESOLVE_32BIT(32, u32, Vector_16F, 32A4, 8, 8, Float16ToARGB, Frame16, 1, 1);
				}
				else if (AA.x)
				{
					RESOLVE_32BIT(32, u32, Vector_16F, 32A2, 8, 8, Float16ToARGB, Frame16, 1, 0);
				}
				else
				{
					RESOLVE_32BIT(32, u32, Vector_16F, 32, 8, 8, Float16ToARGB, Frame16, 0, 0);
				}

				break;

			case PSMCT16:

				if (AA.y)
				{
					RESOLVE_32BIT(16, u16, Vector_16F, 16A4, 16, 8, Float16ToARGB16, Frame16, 1, 1);
				}
				else if (AA.x)
				{
					RESOLVE_32BIT(16, u16, Vector_16F, 16A2, 16, 8, Float16ToARGB16, Frame16, 1, 0);
				}
				else
				{
					RESOLVE_32BIT(16, u16, Vector_16F, 16, 16, 8, Float16ToARGB16, Frame16, 0, 0);
				}

				break;

			case PSMCT16S:

				if (AA.y)
				{
					RESOLVE_32BIT(16S, u16, Vector_16F, 16A4, 16, 8, Float16ToARGB16, Frame16, 1, 1);
				}
				else if (AA.x)
				{
					RESOLVE_32BIT(16S, u16, Vector_16F, 16A2, 16, 8, Float16ToARGB16, Frame16, 1, 0);
				}
				else
				{
					RESOLVE_32BIT(16S, u16, Vector_16F, 16, 16, 8, Float16ToARGB16, Frame16, 0, 0);
				}

				break;

			case PSMT32Z:

			case PSMT24Z:

				if (AA.y)
				{
					RESOLVE_32BIT(32Z, u32, Vector_16F, 32ZA4, 8, 8, Float16ToARGB_Z, Frame16, 1, 1);
				}
				else if (AA.x)
				{
					RESOLVE_32BIT(32Z, u32, Vector_16F, 32ZA2, 8, 8, Float16ToARGB_Z, Frame16, 1, 0);
				}
				else
				{
					RESOLVE_32BIT(32Z, u32, Vector_16F, 32Z, 8, 8, Float16ToARGB_Z, Frame16, 0, 0);
				}

				break;

			case PSMT16Z:

				if (AA.y)
				{
					RESOLVE_32BIT(16Z, u16, Vector_16F, 16ZA4, 16, 8, Float16ToARGB16_Z, Frame16, 1, 1);
				}
				else if (AA.x)
				{
					RESOLVE_32BIT(16Z, u16, Vector_16F, 16ZA2, 16, 8, Float16ToARGB16_Z, Frame16, 1, 0);
				}
				else
				{
					RESOLVE_32BIT(16Z, u16, Vector_16F, 16Z, 16, 8, Float16ToARGB16_Z, Frame16, 0, 0);
				}

				break;

			case PSMT16SZ:

				if (AA.y)
				{
					RESOLVE_32BIT(16SZ, u16, Vector_16F, 16ZA4, 16, 8, Float16ToARGB16_Z, Frame16, 1, 1);
				}
				else if (AA.x)
				{
					RESOLVE_32BIT(16SZ, u16, Vector_16F, 16ZA2, 16, 8, Float16ToARGB16_Z, Frame16, 1, 0);
				}
				else
				{
					RESOLVE_32BIT(16SZ, u16, Vector_16F, 16Z, 16, 8, Float16ToARGB16_Z, Frame16, 0, 0);
				}

				break;
		}
	}

	g_MemTargs.ClearRange(start, end);

	INC_RESOLVE();
}

#endif
