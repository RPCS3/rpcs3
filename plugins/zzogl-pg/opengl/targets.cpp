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
#include "zerogs.h"
#include "targets.h"
#include "ZZoglShaders.h"

#define RHA
//#define RW

using namespace ZeroGS;
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

namespace ZeroGS
{
CRenderTargetMngr s_RTs, s_DepthRTs;
CBitwiseTextureMngr s_BitwiseTextures;
CMemoryTargetMngr g_MemTargs;
}

//extern u32 s_ptexCurSet[2];
bool g_bSaveZUpdate = 0;

int VALIDATE_THRESH = 8;
u32 TEXDESTROY_THRESH = 16;

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
inline bool ZeroGS::CRenderTarget::InitialiseDefaultTexture(u32 *ptr_p, int fbw, int fbh)
{
	glGenTextures(1, ptr_p);
	glBindTexture(GL_TEXTURE_RECTANGLE_NV, *ptr_p);

	// initialize to default
	TextureRect(4, fbw, fbh, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

	setRectWrap(GL_CLAMP);
	setRectFilters(GL_LINEAR);

	GLenum Error = glGetError();
	return ((Error == GL_NO_ERROR) || (Error == GL_INVALID_FRAMEBUFFER_OPERATION_EXT));
}

// Draw 4 triangles from binded array using only stencil buffer
inline void FillOnlyStencilBuffer()
{
	if (ZeroGS::IsWriteDestAlphaTest() && !(conf.settings().no_stencil))
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
inline Vector ZeroGS::CRenderTarget::DefaultBitBltPos()
{
	Vector v = Vector(1, -1, 0.5f / (float)RW(fbw), 0.5f / (float)RH(fbh));
	v *= 1.0f / 32767.0f;
	ZZshSetParameter4fv(pvsBitBlt.sBitBltPos, v, "g_sBitBltPos");
	return v;
}

// Used to transform texture coordinates from GS (when 0,0 is upper left) to
// OpenGL (0,0 - lower left).
inline Vector ZeroGS::CRenderTarget::DefaultBitBltTex()
{
	// I really sure that -0.5 is correct, because OpenGL have no half-offset
	// issue, DirectX known for.
	Vector v = Vector(1, -1, 0.5f / (float)RW(fbw), -0.5f / (float)RH(fbh));
	ZZshSetParameter4fv(pvsBitBlt.sBitBltTex, v, "g_sBitBltTex");
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
ZeroGS::CRenderTarget::CRenderTarget() : ptex(0), ptexFeedback(0), psys(NULL)
{
	FUNCLOG
	nUpdateTarg = 0;
}

ZeroGS::CRenderTarget::~CRenderTarget()
{
	FUNCLOG
	Destroy();
}

bool ZeroGS::CRenderTarget::Create(const frameInfo& frame)
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

void ZeroGS::CRenderTarget::Destroy()
{
	FUNCLOG
	created = 1;
	_aligned_free(psys);
	psys = NULL;
	SAFE_RELEASE_TEX(ptex);
	SAFE_RELEASE_TEX(ptexFeedback);
}

void ZeroGS::CRenderTarget::SetTarget(int fbplocal, const Rect2& scissor, int context)
{
	FUNCLOG
	int dy = 0;

	if (fbplocal != fbp)
	{
		Vector v;

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

void ZeroGS::CRenderTarget::SetViewport()
{
	FUNCLOG
	glViewport(0, 0, RW(fbw), RH(fbh));
}

inline bool NotResolveHelper()
{
	return ((s_nResolved > 8 && (2 * s_nResolved > fFPS - 10)) || (conf.settings().no_target_resolve));
}

void ZeroGS::CRenderTarget::Resolve()
{
	FUNCLOG

	if (ptex != 0 && !(status&TS_Resolved) && !(status&TS_NeedUpdate))
	{
		// flush if necessary
		FlushIfNecesary(this) ;

		if ((IsDepth() && !ZeroGS::IsWriteDepth()) || NotResolveHelper())
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

void ZeroGS::CRenderTarget::Resolve(int startrange, int endrange)
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

void ZeroGS::CRenderTarget::Update(int context, ZeroGS::CRenderTarget* pdepth)
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
	Vector v = DefaultBitBltPos();

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
		ZZshGLSetTextureParameter(ppsBaseTexture.sFinal, ittarg->second->ptex, "BaseTexture.final");

		//assert( ittarg->second->fbw == fbw );
		int offset = (fbp - ittarg->second->fbp) * 64 / fbw;

		if (PSMT_ISHALF(psm)) // 16 bit
			offset *= 2;

		v.x = (float)RW(fbw);
		v.y = (float)RH(fbh);
		v.z = 0.25f;
		v.w = (float)RH(offset) + 0.25f;

		ZZshSetParameter4fv(pvsBitBlt.sBitBltTex, v, "g_fBitBltTex");

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
		ZZshGLSetTextureParameter(ppsBitBlt[bit_idx].sMemory, vb[0].pmemtarg->ptex->tex, "BitBlt.memory");

		v = Vector(1, 1, 0.0f, 0.0f);
		ZZshSetParameter4fv(pvsBitBlt.sBitBltTex, v, "g_fBitBltTex");

		v.x = 1;
		v.y = 2;
		ZZshSetParameter4fv(ppsBitBlt[bit_idx].sOneColor, v, "g_fOneColor");

		assert(ptex != 0);

		if (conf.wireframe()) glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		if (ZeroGS::IsWriteDestAlphaTest())
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
	if (conf.mrtdepth && pdepth != NULL && ZeroGS::IsWriteDepth()) pdepth->SetRenderTarget(1);

	status = TS_Resolved;

	// reset since settings changed
	vb[0].bVarsTexSync = 0;

	ZeroGS::ResetAlphaVariables();
}

void ZeroGS::CRenderTarget::ConvertTo32()
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
	Vector v = DefaultBitBltPos();
	v = DefaultBitBltTex();

	v.x = (float)RW(16);
	v.y = (float)RH(16);
	v.z = -(float)RW(fbw);
	v.w = (float)RH(8);
	ZZshSetParameter4fv(ppsConvert16to32.fTexOffset, v, "g_fTexOffset");

	v.x = (float)RW(8);
	v.y = 0;
	v.z = 0;
	v.w = 0.25f;
	ZZshSetParameter4fv(ppsConvert16to32.fPageOffset, v, "g_fPageOffset");

	v.x = (float)RW(2 * fbw);
	v.y = (float)RH(fbh);
	v.z = 0;
	v.w = 0.0001f * (float)RH(fbh);
	ZZshSetParameter4fv(ppsConvert16to32.fTexDims, v, "g_fTexDims");

//	v.x = 0;
//	ZZshSetParameter4fv(ppsConvert16to32.fTexBlock, v, "g_fTexBlock");

	glBindBuffer(GL_ARRAY_BUFFER, vboRect);
	SET_STREAM();

	// assume depth already set !?
	FBTexture(0, ptexConv);
	ZeroGS::ResetRenderTarget(1);

	BindToSample(&ptex);
	ZZshGLSetTextureParameter(ppsConvert16to32.sFinal, ptex, "Convert 16 to 32.Final");

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
	if (ZeroGS::icurctx >= 0)
	{
		// reset since settings changed
		vb[icurctx].bVarsTexSync = 0;
		vb[icurctx].bVarsSetTarg = 0;
	}

	vb[0].bVarsTexSync = 0;
}

void ZeroGS::CRenderTarget::ConvertTo16()
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
	Vector v = DefaultBitBltPos();
	v = DefaultBitBltTex();

	v.x = 16.0f / (float)fbw;
	v.y = 8.0f / (float)fbh;
	v.z = 0.5f * v.x;
	v.w = 0.5f * v.y;
	ZZshSetParameter4fv(ppsConvert32to16.fTexOffset, v, "g_fTexOffset");

	v.x = 256.0f / 255.0f;
	v.y = 256.0f / 255.0f;
	v.z = 0.05f / 256.0f;
	v.w = -0.001f / 256.0f;
	ZZshSetParameter4fv(ppsConvert32to16.fPageOffset, v, "g_fPageOffset");

	v.x = (float)RW(fbw);
	v.y = (float)RH(2 * fbh);
	v.z = 0;
	v.w = -0.1f / RH(fbh);
	ZZshSetParameter4fv(ppsConvert32to16.fTexDims, v, "g_fTexDims");

	glBindBuffer(GL_ARRAY_BUFFER, vboRect);
	SET_STREAM();

	// assume depth already set !?
	FBTexture(0, ptexConv);
	ZeroGS::ResetRenderTarget(1);
	GL_REPORT_ERRORD();

	BindToSample(&ptex);

	ZZshGLSetTextureParameter(ppsConvert32to16.sFinal, ptex, "Convert 32 to 16");

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
	if (ZeroGS::icurctx >= 0)
	{
		// reset since settings changed
		vb[icurctx].bVarsTexSync = 0;
		vb[icurctx].bVarsSetTarg = 0;
	}

	vb[0].bVarsTexSync = 0;
}

void ZeroGS::CRenderTarget::_CreateFeedback()
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
	/*	Vector v = DefaultBitBltPos();
		v = Vector ((float)(RW(fbw+4)), (float)(RH(fbh+4)), +0.25f, -0.25f);
		ZZshSetParameter4fv(pvsBitBlt.sBitBltTex, v, "BitBltTex");*/

	// tex coords, test ffx bikanel island when changing these

//	Vector v = Vector(1, -1, 0.5f / (fbw << AA.x), 0.5f / (fbh << AA.y));
//	v *= 1/32767.0f;
//	cgGLSetParameter4fv(pvsBitBlt.sBitBltPos, v);
	Vector v = DefaultBitBltPos();

	v.x = (float)(RW(fbw));
	v.y = (float)(RH(fbh));
	v.z = 0.0f;
	v.w = 0.0f;
	ZZshSetParameter4fv(pvsBitBlt.sBitBltTex, v, "BitBlt.Feedback");
	ZZshDefaultOneColor(ppsBaseTexture);

	glBindBuffer(GL_ARRAY_BUFFER, vboRect);
	SET_STREAM();

	FBTexture(0, ptexFeedback);
	glBindTexture(GL_TEXTURE_RECTANGLE_NV, ptex);
	GL_REPORT_ERRORD();

	ZZshGLSetTextureParameter(ppsBaseTexture.sFinal, ptex, "BaseTexture.Feedback");

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
	if (ZeroGS::icurctx >= 0)
	{
		// reset since settings changed
		vb[icurctx].bVarsTexSync = 0;
	}

	GL_REPORT_ERRORD();
}

void ZeroGS::CRenderTarget::SetRenderTarget(int targ)
{
	FUNCLOG

	FBTexture(targ, ptex);

	//GL_REPORT_ERRORD();
	//if (glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT) != GL_FRAMEBUFFER_COMPLETE_EXT)
		//ERROR_LOG_SPAM("The Framebuffer is not complete. Glitches could appear onscreen.\n");
}

ZeroGS::CDepthTarget::CDepthTarget() : CRenderTarget(), pdepth(0), pstencil(0), icount(0) {}

ZeroGS::CDepthTarget::~CDepthTarget()
{
	FUNCLOG

	Destroy();
}

bool ZeroGS::CDepthTarget::Create(const frameInfo& frame)
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

void ZeroGS::CDepthTarget::Destroy()
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

void ZeroGS::CDepthTarget::Resolve()
{
	FUNCLOG

	if (g_nDepthUsed > 0 && conf.mrtdepth && !(status&TS_Virtual) && ZeroGS::IsWriteDepth() && !(conf.settings().no_depth_resolve))
		CRenderTarget::Resolve();
	else
	{
		// flush if necessary
		FlushIfNecesary(this) ;

		if (!(status & TS_Virtual)) status |= TS_Resolved;
	}

	if (!(status&TS_Virtual))
	{
		ZeroGS::SetWriteDepth();
	}
}

void ZeroGS::CDepthTarget::Resolve(int startrange, int endrange)
{
	FUNCLOG

	if (g_nDepthUsed > 0 && conf.mrtdepth && !(status&TS_Virtual) && ZeroGS::IsWriteDepth())
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
		ZeroGS::SetWriteDepth();
	}
}

void ZeroGS::CDepthTarget::Update(int context, ZeroGS::CRenderTarget* prndr)
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

	ZeroGS::VB& curvb = vb[context];

	if (curvb.test.zte == 0) return;

	SetShaderCaller("CDepthTarget::Update");

	glEnable(GL_DEPTH_TEST);

	glDepthMask(!curvb.zbuf.zmsk);

	static const u32 g_dwZCmp[] = { GL_NEVER, GL_ALWAYS, GL_GEQUAL, GL_GREATER };

	glDepthFunc(g_dwZCmp[curvb.test.ztst]);

	// write color and zero out stencil buf, always 0 context!
	SetTexVariablesInt(0, 0, texframe, false, &ppsBitBltDepth, 1);
	ZZshGLSetTextureParameter(ppsBitBltDepth.sMemory, vb[0].pmemtarg->ptex->tex, "BitBltDepth");

	Vector v = DefaultBitBltPos();

	v = DefaultBitBltTex();

	v.x = 1;
	v.y = 2;
	v.z = PSMT_IS16Z(psm) ? 1.0f : 0.0f;
	v.w = g_filog32;
	ZZshSetParameter4fv(ppsBitBltDepth.sOneColor, v, "g_fOneColor");

	Vector vdepth = g_vdepth;

	if (psm == PSMT24Z)
	{
		vdepth.w = 0;
	}
	else if (psm != PSMT32Z)
	{
		vdepth.z = vdepth.w = 0;
	}

	assert(ppsBitBltDepth.sBitBltZ != 0);

	ZZshSetParameter4fv(ppsBitBltDepth.sBitBltZ, ((255.0f / 256.0f)*vdepth), "g_fBitBltZ");

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

	if (!ZeroGS::IsWriteDepth())
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

void ZeroGS::CDepthTarget::SetDepthStencilSurface()
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

void ZeroGS::CRenderTargetMngr::Destroy()
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

void ZeroGS::CRenderTargetMngr::DestroyAllTargs(int start, int end, int fbw)
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

void ZeroGS::CRenderTargetMngr::DestroyTarg(CRenderTarget* ptarg)
{
	FUNCLOG
	DestroyAllTargetsHelper(ptarg) ;
	delete ptarg;
}

void ZeroGS::CRenderTargetMngr::DestroyIntersecting(CRenderTarget* prndr)
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

void ZeroGS::CRenderTargetMngr::PrintTargets()
{
#ifdef _DEBUG
	for (MAPTARGETS::iterator it1 = mapDummyTargs.begin(); it1 != mapDummyTargs.end(); ++it1)
		ZZLog::Debug_Log("\t Dummy Targets(0x%x) fbw:0x%x fbh:0x%x psm:0x%x fbp:0x%x", GetFrameKey(it1->second), it1->second->fbw, it1->second->fbh, it1->second->psm, it1->second->fbp);

	for (MAPTARGETS::iterator it1 = mapTargets.begin(); it1 != mapTargets.end(); ++it1)
		ZZLog::Debug_Log("\t Targets(0x%x) fbw:0x%x fbh:0x%x psm:0x%x fbp:0x%x", GetFrameKey(it1->second), it1->second->fbw, it1->second->fbh, it1->second->psm, it1->second->fbp);
#endif
}

CRenderTarget* ZeroGS::CRenderTargetMngr::GetTarg(const frameInfo& frame, u32 opts, int maxposheight)
{
	FUNCLOG

	if (frame.fbw <= 0 || frame.fbh <= 0) return NULL;

	GL_REPORT_ERRORD();

	u32 key = GetFrameKey(frame);

	MAPTARGETS::iterator it = mapTargets.find(key);

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

	if (bfound)
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
#if defined(ZEROGS_DEVBUILD)
				ZZLog::Warn_Log("Bad formats 2: %d %d", frame.psm, it->second->psm);
#endif
				it->second->psm = frame.psm;

				// recalc extents
				GetRectMemAddress(it->second->start, it->second->end, frame.psm, 0, 0, frame.fbw, it->second->fbh, it->second->fbp, frame.fbw);
			}
		}

		if (it->second->fbm != frame.fbm)
		{
			//ZZLog::Warn_Log("Bad fbm: 0x%8.8x 0x%8.8x, psm: %d", frame.fbm, it->second->fbm, frame.psm);
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
//			printf ("A %d %d %d %d\n", frame.psm, frame.fbw, pbesttarg->psm, pbesttarg->fbw);

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

ZeroGS::CRenderTargetMngr::MAPTARGETS::iterator ZeroGS::CRenderTargetMngr::GetOldestTarg(MAPTARGETS& m)
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

void ZeroGS::CRenderTargetMngr::GetTargs(int start, int end, list<ZeroGS::CRenderTarget*>& listTargets) const
{
	FUNCLOG

	for (MAPTARGETS::const_iterator it = mapTargets.begin(); it != mapTargets.end(); ++it)
	{
		if ((it->second->start < end) && (start < it->second->end)) listTargets.push_back(it->second);
	}
}

void ZeroGS::CRenderTargetMngr::Resolve(int start, int end)
{
	FUNCLOG

	for (MAPTARGETS::const_iterator it = mapTargets.begin(); it != mapTargets.end(); ++it)
	{
		if ((it->second->start < end) && (start < it->second->end))
			it->second->Resolve();
	}
}

void ZeroGS::CMemoryTargetMngr::Destroy()
{
	FUNCLOG
	listTargets.clear();
	listClearedTargets.clear();
}

int memcmp_clut16(u16* pSavedBuffer, u16* pClutBuffer, int clutsize)
{
	FUNCLOG
	assert((clutsize&31) == 0);

	// left > 0 only when csa < 16
	int left = 0;
	if (((u32)(uptr)pClutBuffer & 2) == 0)
	{
		left = (((u32)(uptr)pClutBuffer & 0x3ff) / 2) + clutsize - 512;
		clutsize -= left;
	}

	while (clutsize > 0)
	{
		for (int i = 0; i < 16; ++i)
		{
			if (pSavedBuffer[i] != pClutBuffer[2*i]) return 1;
		}

		clutsize -= 32;
		pSavedBuffer += 16;
		pClutBuffer += 32;
	}

	if (left > 0)
	{
		pClutBuffer = (u16*)(g_pbyGSClut + 2);

		while (left > 0)
		{
			for (int i = 0; i < 16; ++i)
			{
				if (pSavedBuffer[i] != pClutBuffer[2*i]) return 1;
			}

			left -= 32;

			pSavedBuffer += 16;
			pClutBuffer += 32;
		}
	}

	return 0;
}

bool ZeroGS::CMemoryTarget::ValidateClut(const tex0Info& tex0)
{
	FUNCLOG
	assert(tex0.psm == psm && PSMT_ISCLUT(psm) && cpsm == tex0.cpsm);

	int nClutOffset = 0, clutsize = 0;
	int entries = PSMT_IS8CLUT(tex0.psm) ? 256 : 16;

	if (PSMT_IS32BIT(tex0.cpsm))   // 32 bit
	{
		nClutOffset = 64 * tex0.csa;
		clutsize = min(entries, 256 - tex0.csa * 16) * 4;
	}
	else
	{
		nClutOffset = 32 * (tex0.csa & 15) + (tex0.csa >= 16 ? 2 : 0);
		clutsize = min(entries, 512 - tex0.csa * 16) * 2;
	}

	assert(clutsize == clut.size());

	if (PSMT_IS32BIT(cpsm))
	{
		if (memcmp_mmx(&clut[0], g_pbyGSClut + nClutOffset, clutsize)) return false;
	}
	else
	{
		if (memcmp_clut16((u16*)&clut[0], (u16*)(g_pbyGSClut + nClutOffset), clutsize)) return false;
	}

	return true;
}

bool ZeroGS::CMemoryTarget::ValidateTex(const tex0Info& tex0, int starttex, int endtex, bool bDeleteBadTex)
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

	int result = memcmp_mmx(ptex->memptr + (checkstarty - realy) * 4 * GPU_TEXWIDTH, g_pbyGSMemory + checkstarty * 4 * GPU_TEXWIDTH, (checkendy - checkstarty) * 4 * GPU_TEXWIDTH);

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

// used to build clut textures (note that this is for both 16 and 32 bit cluts)
template <class T>
static __forceinline void BuildClut(u32 psm, u32 height, T* pclut, u8* psrc, T* pdst)
{
	switch (psm)
	{
		case PSMT8:
			for (u32 i = 0; i < height; ++i)
			{
				for (int j = 0; j < GPU_TEXWIDTH / 2; ++j)
				{
					pdst[0] = pclut[psrc[0]];
					pdst[1] = pclut[psrc[1]];
					pdst[2] = pclut[psrc[2]];
					pdst[3] = pclut[psrc[3]];
					pdst[4] = pclut[psrc[4]];
					pdst[5] = pclut[psrc[5]];
					pdst[6] = pclut[psrc[6]];
					pdst[7] = pclut[psrc[7]];
					pdst += 8;
					psrc += 8;
				}
			}
			break;

		case PSMT4:
			for (u32 i = 0; i < height; ++i)
			{
				for (int j = 0; j < GPU_TEXWIDTH; ++j)
				{
					pdst[0] = pclut[psrc[0] & 15];
					pdst[1] = pclut[psrc[0] >> 4];
					pdst[2] = pclut[psrc[1] & 15];
					pdst[3] = pclut[psrc[1] >> 4];
					pdst[4] = pclut[psrc[2] & 15];
					pdst[5] = pclut[psrc[2] >> 4];
					pdst[6] = pclut[psrc[3] & 15];
					pdst[7] = pclut[psrc[3] >> 4];

					pdst += 8;
					psrc += 4;
				}
			}
			break;

		case PSMT8H:
			for (u32 i = 0; i < height; ++i)
			{
				for (int j = 0; j < GPU_TEXWIDTH / 8; ++j)
				{
					pdst[0] = pclut[psrc[3]];
					pdst[1] = pclut[psrc[7]];
					pdst[2] = pclut[psrc[11]];
					pdst[3] = pclut[psrc[15]];
					pdst[4] = pclut[psrc[19]];
					pdst[5] = pclut[psrc[23]];
					pdst[6] = pclut[psrc[27]];
					pdst[7] = pclut[psrc[31]];
					pdst += 8;
					psrc += 32;
				}
			}
			break;

		case PSMT4HH:
			for (u32 i = 0; i < height; ++i)
			{
				for (int j = 0; j < GPU_TEXWIDTH / 8; ++j)
				{
					pdst[0] = pclut[psrc[3] >> 4];
					pdst[1] = pclut[psrc[7] >> 4];
					pdst[2] = pclut[psrc[11] >> 4];
					pdst[3] = pclut[psrc[15] >> 4];
					pdst[4] = pclut[psrc[19] >> 4];
					pdst[5] = pclut[psrc[23] >> 4];
					pdst[6] = pclut[psrc[27] >> 4];
					pdst[7] = pclut[psrc[31] >> 4];
					pdst += 8;
					psrc += 32;
				}
			}
			break;

		case PSMT4HL:
			for (u32 i = 0; i < height; ++i)
			{
				for (int j = 0; j < GPU_TEXWIDTH / 8; ++j)
				{
					pdst[0] = pclut[psrc[3] & 15];
					pdst[1] = pclut[psrc[7] & 15];
					pdst[2] = pclut[psrc[11] & 15];
					pdst[3] = pclut[psrc[15] & 15];
					pdst[4] = pclut[psrc[19] & 15];
					pdst[5] = pclut[psrc[23] & 15];
					pdst[6] = pclut[psrc[27] & 15];
					pdst[7] = pclut[psrc[31] & 15];
					pdst += 8;
					psrc += 32;
				}
			}
			break;

		default:
			assert(0);
	}
}

#define TARGET_THRESH 0x500

extern int g_MaxTexWidth, g_MaxTexHeight; // Maximum height & width of supported texture.

//#define SORT_TARGETS
inline list<CMemoryTarget>::iterator ZeroGS::CMemoryTargetMngr::DestroyTargetIter(list<CMemoryTarget>::iterator& it)
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

int MemoryTarget_CompareTarget(list<CMemoryTarget>::iterator& it, const tex0Info& tex0, int clutsize, int nClutOffset)
{
	if (PSMT_ISCLUT(it->psm) != PSMT_ISCLUT(tex0.psm))
	{
		return 1;
	}

	if (PSMT_ISCLUT(tex0.psm))
	{
		assert(it->clut.size() > 0);

		if (it->psm != tex0.psm || it->cpsm != tex0.cpsm || it->clut.size() != clutsize)
		{
			return 1;
		}

		if	(PSMT_IS32BIT(tex0.cpsm))
		{
			if (memcmp_mmx(&it->clut[0], g_pbyGSClut + nClutOffset, clutsize))
			{
				return 2;
			}
		}
		else
		{
			if (memcmp_clut16((u16*)&it->clut[0], (u16*)(g_pbyGSClut + nClutOffset), clutsize))
			{
				return 2;
			}
		}

	}
	else
		if (PSMT_IS16BIT(tex0.psm) != PSMT_IS16BIT(it->psm))
		{
			return 1;
		}

	return 0;
}

void MemoryTarget_GetClutVariables(int& nClutOffset, int& clutsize, const tex0Info& tex0)
{
	nClutOffset = 0;
	clutsize = 0;

	if (PSMT_ISCLUT(tex0.psm))
	{
		int entries = PSMT_IS8CLUT(tex0.psm) ? 256 : 16;

		if (PSMT_IS32BIT(tex0.cpsm))
		{
			nClutOffset = 64 * tex0.csa;
			clutsize = min(entries, 256 - tex0.csa * 16) * 4;
		}
		else
		{
			nClutOffset = 64 * (tex0.csa & 15) + (tex0.csa >= 16 ? 2 : 0);
			clutsize = min(entries, 512 - tex0.csa * 16) * 2;
		}
	}
}

void MemoryTarget_GetMemAddress(int& start, int& end,  const tex0Info& tex0)
{
	int nbStart, nbEnd;
	GetRectMemAddress(nbStart, nbEnd, tex0.psm, 0, 0, tex0.tw, tex0.th, tex0.tbp0, tex0.tbw);
	assert(nbStart < nbEnd);
	nbEnd = min(nbEnd, MEMORY_END);

	start = nbStart / (4 * GPU_TEXWIDTH);
	end = (nbEnd + GPU_TEXWIDTH * 4 - 1) / (4 * GPU_TEXWIDTH);
	assert(start < end);

}

ZeroGS::CMemoryTarget* ZeroGS::CMemoryTargetMngr::MemoryTarget_SearchExistTarget(int start, int end, int nClutOffset, int clutsize, const tex0Info& tex0, int forcevalidate)
{
	for (list<CMemoryTarget>::iterator it = listTargets.begin(); it != listTargets.end();)
	{

		if (it->starty <= start && it->starty + it->height >= end)
		{

			int res = MemoryTarget_CompareTarget(it, tex0, clutsize, nClutOffset);

			if (res == 1)
			{
				if (it->validatecount++ > VALIDATE_THRESH)
				{
					it = DestroyTargetIter(it);

					if (listTargets.size() == 0) break;
				}
				else
				{
					++it;
				}
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
					{
						++it;
					}

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

ZeroGS::CMemoryTarget* ZeroGS::CMemoryTargetMngr::MemoryTarget_ClearedTargetsSearch(int fmt, int widthmult, int channels, int height)
{
	CMemoryTarget* targ = NULL;

	if (listClearedTargets.size() > 0)
	{
		list<CMemoryTarget>::iterator itbest = listClearedTargets.begin();

		while (itbest != listClearedTargets.end())
		{
			if ((height <= itbest->realheight) && (itbest->fmt == fmt) && (itbest->widthmult == widthmult) && (itbest->channels == channels))
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

ZeroGS::CMemoryTarget* ZeroGS::CMemoryTargetMngr::GetMemoryTarget(const tex0Info& tex0, int forcevalidate)
{
	FUNCLOG
	int start, end, nClutOffset, clutsize;

	MemoryTarget_GetClutVariables(nClutOffset, clutsize, tex0);
	MemoryTarget_GetMemAddress(start, end, tex0);

	ZeroGS::CMemoryTarget* it = MemoryTarget_SearchExistTarget(start, end, nClutOffset, clutsize, tex0, forcevalidate);

	if (it != NULL) return it;

	// couldn't find so create
	CMemoryTarget* targ;

	u32 fmt = GL_UNSIGNED_BYTE;

	// RGBA16 storage format
	if (PSMT_ISHALF_STORAGE(tex0)) fmt = GL_UNSIGNED_SHORT_1_5_5_5_REV;

	int widthmult = 1, channels = 1;

	// If our texture is too big and could not be placed in 1 GPU texture. Pretty rare.
	if ((g_MaxTexHeight < 4096) && (end - start > g_MaxTexHeight)) widthmult = 2;
	channels = PIXELS_PER_WORD(tex0.psm);

	targ = MemoryTarget_ClearedTargetsSearch(fmt, widthmult, channels, end - start);

	// fill local clut
	if (PSMT_ISCLUT(tex0.psm))
	{
		assert(clutsize > 0);

		targ->cpsm = tex0.cpsm;
		targ->clut.reserve(256*4); // no matter what
		targ->clut.resize(clutsize);

		if (PSMT_IS32BIT(tex0.cpsm))
		{
			memcpy_amd(&targ->clut[0], g_pbyGSClut + nClutOffset, clutsize);
		}
		else
		{
			u16* pClutBuffer = (u16*)(g_pbyGSClut + nClutOffset);
			u16* pclut = (u16*) & targ->clut[0];
			int left = ((u32)nClutOffset & 2) ? 0 : ((nClutOffset & 0x3ff) / 2) + clutsize - 512;

			if (left > 0) clutsize -= left;

			while (clutsize > 0)
			{
				pclut[0] = pClutBuffer[0];
				pclut++;
				pClutBuffer += 2;
				clutsize -= 2;
			}

			if (left > 0)
			{
				pClutBuffer = (u16*)(g_pbyGSClut + 2);

				while (left > 0)
				{
					pclut[0] = pClutBuffer[0];
					left -= 2;
					pClutBuffer += 2;
					pclut++;
				}
			}
		}
	}

	if (targ->ptex != NULL)
	{
		assert(end - start <= targ->realheight && targ->fmt == fmt && targ->widthmult == widthmult);

		// good enough, so init
		targ->realy = targ->starty = start;
		targ->usedstamp = curstamp;
		targ->psm = tex0.psm;
		targ->cpsm = tex0.cpsm;
		targ->height = end - start;
	}

	if (targ->ptex == NULL)
	{
		// not initialized yet
		targ->fmt = fmt;
		targ->realy = targ->starty = start;
		targ->realheight = targ->height = end - start;
		targ->usedstamp = curstamp;
		targ->psm = tex0.psm;
		targ->cpsm = tex0.cpsm;
		targ->widthmult = widthmult;
		targ->channels = channels;

		// alloc the mem
		targ->ptex = new CMemoryTarget::TEXTURE();
		targ->ptex->ref = 1;
	}

#if defined(ZEROGS_DEVBUILD)
	g_TransferredToGPU += GPU_TEXWIDTH * channels * 4 * targ->height;
#endif

	const int texH = (targ->realheight + widthmult - 1) / widthmult;
	const int texW = GPU_TEXWIDTH * channels * widthmult;

	// fill with data
	if (targ->ptex->memptr == NULL)
	{
		targ->ptex->memptr = (u8*)_aligned_malloc(4 * GPU_TEXWIDTH * targ->realheight, 16);
		assert(targ->ptex->ref > 0);
	}

	memcpy_amd(targ->ptex->memptr, g_pbyGSMemory + 4 * GPU_TEXWIDTH * targ->realy, 4 * GPU_TEXWIDTH * targ->height);

	vector<u8> texdata;
	u8* ptexdata = NULL;

	if (PSMT_ISCLUT(tex0.psm))
	{
		texdata.resize(((tex0.cpsm <= 1) ? 4 : 2) * texW * texH);
		ptexdata = &texdata[0];

		u8* psrc = (u8*)(g_pbyGSMemory + 4 * GPU_TEXWIDTH * targ->realy);

		if (PSMT_IS32BIT(tex0.cpsm))
		{
			u32* pclut = (u32*) & targ->clut[0];
			u32* pdst = (u32*)ptexdata;

			BuildClut<u32>(tex0.psm, targ->height, pclut, psrc, pdst);
		}
		else
		{
			u16* pclut = (u16*) & targ->clut[0];
			u16* pdst = (u16*)ptexdata;

			BuildClut<u16>(tex0.psm, targ->height, pclut, psrc, pdst);
		}
	}
	else
	{
		if (tex0.psm == PSMT16Z || tex0.psm == PSMT16SZ)
		{
			texdata.resize(4 * texW * texH
#if defined(ZEROGS_SSE2)
			+ 15 						// reserve additional elements for alignment if SSE2 used.
										// better do it now, so less resizing would be needed
#endif
							);

			ptexdata = &texdata[0];

			// needs to be 8 bit, use xmm for unpacking
			u16* dst = (u16*)ptexdata;
			u16* src = (u16*)(g_pbyGSMemory + 4 * GPU_TEXWIDTH * targ->realy);

#if defined(ZEROGS_SSE2)

			if (((u32)(uptr)dst) % 16 != 0)
			{
				// This is not unusual situation, when vector<u8> does not 16bit alignment, that is destructive for SSE2
				// instruction movdqa [%eax], xmm0
				// The idea would be resize vector to 15 elements, that set ptxedata to aligned position.
				// Later we would move eax by 16, so only we should verify is first element align
				// FIXME. As I see, texdata used only once here, it does not have any impact on other code.
				// Probably, usage of _aligned_maloc() would be preferable.

				// Note: this often happens when changing AA.
				int disalignment = 16 - ((u32)(uptr)dst) % 16;		// This is value of shift. It could be 0 < disalignment <= 15
				ptexdata = &texdata[disalignment];			// Set pointer to aligned element
				dst = (u16*)ptexdata;
				ZZLog::GS_Log("Made alignment for texdata, 0x%x", dst);
				assert(((u32)(uptr)dst) % 16 == 0);			// Assert, because at future could be vectors with uncontigious spaces
			}

			int iters = targ->height * GPU_TEXWIDTH / 16;

			SSE2_UnswizzleZ16Target(dst, src, iters) ;
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
		}
	}

	// create the texture
	GL_REPORT_ERRORD();

	assert(ptexdata != NULL);

	if (targ->ptex->tex == 0) glGenTextures(1, &targ->ptex->tex);

	glBindTexture(GL_TEXTURE_RECTANGLE_NV, targ->ptex->tex);

	if (fmt == GL_UNSIGNED_BYTE)
		TextureRect(4, texW, texH, GL_RGBA, fmt, ptexdata);
	else
		TextureRect(GL_RGB5_A1, texW, texH, GL_RGBA, fmt, ptexdata);

	int realheight = targ->realheight;

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
				ZZLog::Error_Log("Failed to create %dx%x texture.", GPU_TEXWIDTH*channels*widthmult, (realheight + widthmult - 1) / widthmult);
				channels = 1;
				return NULL;
			}

			DestroyOldest();
		}

		TextureRect(4, texW, texH, GL_RGBA, fmt, ptexdata);
	}

	setRectWrap(GL_CLAMP);

	assert(tex0.psm != 0xd);

	if (PSMT_ISCLUT(tex0.psm)) assert(targ->clut.size() > 0);

	return targ;
}

void ZeroGS::CMemoryTargetMngr::ClearRange(int nbStartY, int nbEndY)
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

void ZeroGS::CMemoryTargetMngr::DestroyCleared()
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

void ZeroGS::CMemoryTargetMngr::DestroyOldest()
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
void ZeroGS::CBitwiseTextureMngr::Destroy()
{
	FUNCLOG

	for (map<u32, u32>::iterator it = mapTextures.begin(); it != mapTextures.end(); ++it)
	{
			glDeleteTextures(1, &it->second);
	}

	mapTextures.clear();
}

u32 ZeroGS::CBitwiseTextureMngr::GetTexInt(u32 bitvalue, u32 ptexDoNotDelete)
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

void ZeroGS::CRangeManager::RangeSanityCheck()
{
#ifdef _DEBUG
	// sanity check

	for (int i = 0; i < (int)ranges.size() - 1; ++i)
	{
		assert(ranges[i].end < ranges[i+1].start);
	}

#endif
}

void ZeroGS::CRangeManager::Insert(int start, int end)
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

namespace ZeroGS
{

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
				ZeroGS::GetRectMemAddress(targstart, targend, ptarg->psm, 0, 0, ptarg->fbw, ptarg->fbh & ~(m_Blocks[ptarg->psm].height - 1), ptarg->fbp, ptarg->fbw);

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

		ZeroGS::g_MemTargs.ClearRange(start, end);
	}

	s_RangeMngr.Clear();
}


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

template <typename T, typename Tret>
inline Tret dummy_return(T value) { return value; }

template <typename T, typename Tsrc, T (*convfn)(Tsrc)>
inline void Resolve_32_Bit(const void* psrc, int fbp, int fbw, int fbh, const int psm, u32 fbm)
{
    u32 mask, imask;

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

    Tsrc* src = (Tsrc*)(psrc);
    T* pPageOffset = (T*)g_pbyGSMemory + fbp*(256/sizeof(T)), *dst;

    int maxfbh = (MEMORY_END-fbp*256) / (sizeof(T) * fbw);
    if( maxfbh > fbh ) maxfbh = fbh;

    for(int i = 0; i < maxfbh; ++i) {
        for(int j = 0; j < fbw; ++j) {
            T dsrc = (T)convfn(src[RW(j)]);
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
        src += RH(Pitch(fbw))/sizeof(Tsrc);
    }
}

void _Resolve(const void* psrc, int fbp, int fbw, int fbh, int psm, u32 fbm, bool mode = true)
{
	FUNCLOG

	int start, end;

	s_nResolved += 2;

	// align the rect to the nearest page
	// note that fbp is always aligned on page boundaries
	GetRectMemAddress(start, end, psm, 0, 0, fbw, fbh, fbp, fbw);

    // start the conversion process A8R8G8B8 -> psm
    switch (psm)
    {

        // NOTE pass psm as a constant value otherwise gcc does not do its job. It keep
        // the psm switch in Resolve_32_Bit
        case PSMCT32:
        case PSMCT24:
            Resolve_32_Bit<u32, u32, dummy_return >(psrc, fbp, fbw, fbh, PSMCT32, fbm);
            break;

        case PSMCT16:
            Resolve_32_Bit<u16, u32, RGBA32to16 >(psrc, fbp, fbw, fbh, PSMCT16, fbm);
            break;

        case PSMCT16S:
            Resolve_32_Bit<u16, u32, RGBA32to16 >(psrc, fbp, fbw, fbh, PSMCT16S, fbm);
            break;

        case PSMT32Z:
        case PSMT24Z:
            Resolve_32_Bit<u32, u32, dummy_return >(psrc, fbp, fbw, fbh, PSMT32Z, fbm);
            break;

        case PSMT16Z:
            Resolve_32_Bit<u16, u32, dummy_return >(psrc, fbp, fbw, fbh, PSMT16Z, fbm);
            break;

        case PSMT16SZ:
            Resolve_32_Bit<u16, u32, dummy_return >(psrc, fbp, fbw, fbh, PSMT16SZ, fbm);
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

} // End of namespece ZeroGS
