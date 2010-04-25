/*  ZeroGS KOSMOS
 *  Copyright (C) 2005-2006 zerofrog@gmail.com
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
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "GS.h"
#include <Cg/cg.h>
#include <Cg/cgGL.h>

#include <stdio.h>

#include <malloc.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "Mem.h"
#include "x86.h"
#include "zerogs.h"
#include "targets.h"

#define RHA
//#define RW

extern int g_GameSettings;
using namespace ZeroGS;
extern int g_TransferredToGPU;
extern bool g_bIsLost;
extern bool g_bUpdateStencil;
extern u32 s_uFramebuffer;

#ifdef RELEASE_TO_PUBLIC
#	define INC_RESOLVE()
#else
#	define INC_RESOLVE() ++g_nResolve
#endif

#ifdef DEVBUILD
static int g_bSaveResolved = 0;
#endif

extern int s_nResolved;
extern u32 g_nResolve;
extern bool g_bSaveTrans;

namespace ZeroGS {
	CRenderTargetMngr s_RTs, s_DepthRTs;
	CBitwiseTextureMngr s_BitwiseTextures;
	CMemoryTargetMngr g_MemTargs;
}

extern u32 s_ptexCurSet[2];
extern u32 ptexBilinearBlocks;
extern u32 ptexConv32to16;
BOOL g_bSaveZUpdate = 0;

// ------------------------- Usefull inlines ------------------------------------

// memory size for one row of texture. It's depends of windth of texture and number of bytes
// per pixel
inline u32 Pitch( int fbw ) { return (RW(fbw) * (GetRenderFormat() == RFT_float16 ? 8 : 4)) ; }

// memory size of whole texture. It is number of rows multiplied by memory size of row
inline u32 Tex_Memory_Size ( int fbw, int fbh ) { return (RH(fbh) * Pitch(fbw)); }

// Oftenly called for several reasons
// Call flush if renderer or depther target is equal to ptr
inline void FlushIfNecesary ( void* ptr ) {
	if( vb[0].prndr == ptr || vb[0].pdepth == ptr )
		Flush(0);
	if( vb[1].prndr == ptr || vb[1].pdepth == ptr )
		Flush(1);
}

// This block was repreaded several times, so I inlined it.
inline void DestroyAllTargetsHelper( void* ptr ) {
	for(int i = 0; i < 2; ++i) {
		if( ptr == vb[i].prndr ) { vb[i].prndr = NULL; vb[i].bNeedFrameCheck = 1; }
		if( ptr == vb[i].pdepth ) { vb[i].pdepth = NULL; vb[i].bNeedZCheck = 1; }
	}
}

// Made an empty rexture and bind it to $ptr_p
// return false if creating texture was uncuccessfull
// fbh and fdb should be properly shifter before calling this!.
// We should ignore framebuffer trouble here, we put textures of dufferent sized to it.
inline bool ZeroGS::CRenderTarget::InitialiseDefaultTexture ( u32 *ptr_p, int fbw, int fbh ) {
	glGenTextures(1, ptr_p);
	glBindTexture(GL_TEXTURE_RECTANGLE_NV, *ptr_p);
	// initialize to default
	glTexImage2D(GL_TEXTURE_RECTANGLE_NV, 0, GetRenderTargetFormat(), fbw, fbh, 0, GL_RGBA, GetRenderFormat()==RFT_float16?GL_FLOAT:GL_UNSIGNED_BYTE, NULL);

	glTexParameteri(GL_TEXTURE_RECTANGLE_NV, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_RECTANGLE_NV, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_RECTANGLE_NV, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_RECTANGLE_NV, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	GLenum Error = glGetError();
	return ((Error == GL_NO_ERROR) || (Error == GL_INVALID_FRAMEBUFFER_OPERATION_EXT));
}

// Draw 4 triangles from binded array using only stenclil buffer
inline void FillOnlyStencilBuffer() {
	if( ZeroGS::IsWriteDestAlphaTest() && !(g_GameSettings&GAME_NOSTENCIL)) {
		glColorMask(0,0,0,0);
		glEnable(GL_ALPHA_TEST);
		glAlphaFunc(GL_GEQUAL, 1.0f);

		glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
		glStencilFunc(GL_ALWAYS, 1, 0xff);

		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		glColorMask(1,1,1,1);
	}
}

// used for transformation from vertex position in GS window.coords (I hope)
// to view coordinates (in range 0, 1).
inline Vector ZeroGS::CRenderTarget::DefaultBitBltPos() {
	Vector v = Vector (1, -1, 0.5f/(float)RW(fbw), 0.5f/(float)RH(fbh) );
	v *= 1.0f/32767.0f;
	ZZcgSetParameter4fv(pvsBitBlt.sBitBltPos, v, "g_sBitBltPos");
	return v ;
}

// Used to transform texture coordinates from GS (when 0,0 is upper left) to
// OpenGL (0,0 - lower left).
inline Vector ZeroGS::CRenderTarget::DefaultBitBltTex() {
	// I really sure that -0.5 is correct, because OpenGL have no half-offset
	// issue, DirectX known for.
	Vector v = Vector (1, -1, 0.5f/(float)RW(fbw), -0.5f/(float)RH(fbh) );
	ZZcgSetParameter4fv(pvsBitBlt.sBitBltTex, v, "g_sBitBltTex");
	return v ;
}

inline void BindToSample ( u32 *p_ptr ) {
	glBindTexture(GL_TEXTURE_RECTANGLE_NV, *p_ptr);
	glTexParameteri(GL_TEXTURE_RECTANGLE_NV, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_RECTANGLE_NV, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
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
	vposxy.z = -1-0.5f/(float)fbw;
	vposxy.w = -1+0.5f/(float)fbh;
	status = 0;

	if( fbw > 0 && fbh > 0 ) {
		GetRectMemAddress(start, end, psm, 0, 0, fbw, fbh, fbp, fbw);
		psys = _aligned_malloc(  Tex_Memory_Size ( fbw, fbh ), 16 );

		GL_REPORT_ERRORD();

		if ( !InitialiseDefaultTexture( &ptex, RW(fbw), RH(fbh) )) {
			Destroy();
			return false;
		}

		status = TS_NeedUpdate;
	}
	else {
		start = end = 0;
	}

	return true;
}

void ZeroGS::CRenderTarget::Destroy()
{
	FUNCLOG
	created = 1;
	_aligned_free(psys); psys = NULL;
	SAFE_RELEASE_TEX(ptex);
	SAFE_RELEASE_TEX(ptexFeedback);
}

void ZeroGS::CRenderTarget::SetTarget(int fbplocal, const Rect2& scissor, int context)
{
	FUNCLOG
	int dy = 0;

	if( fbplocal != fbp ) {
		Vector v;

		// will be rendering to a subregion
		u32 bpp = PSMT_ISHALF(psm) ? 2 : 4;
		assert( ((256/bpp)*(fbplocal-fbp)) % fbw == 0 );
		assert( fbplocal >= fbp );

		dy = ((256/bpp)*(fbplocal-fbp)) / fbw;

		v.x = vposxy.x;
		v.y = vposxy.y;
		v.z = vposxy.z;
		v.w = vposxy.w - dy*2.0f/(float)fbh;
		ZZcgSetParameter4fv(g_vparamPosXY[context], v, "g_fPosXY");
	}
	else
		ZZcgSetParameter4fv(g_vparamPosXY[context], vposxy, "g_fPosXY");

	// set render states
	scissorrect.x = scissor.x0>>3;
	scissorrect.y = (scissor.y0>>3) + dy;
	scissorrect.w = (scissor.x1>>3)+1;
	scissorrect.h = (scissor.y1>>3)+1+dy;
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

inline bool NotResolveHelper() {
	return ((s_nResolved > 8 && (2 * s_nResolved > fFPS - 10)) || (g_GameSettings&GAME_NOTARGETRESOLVE));
}

void ZeroGS::CRenderTarget::Resolve()
{
	FUNCLOG

	if( ptex != 0 && !(status&TS_Resolved) && !(status&TS_NeedUpdate) ) {

		// flush if necessary
		FlushIfNecesary ( this ) ;

		if ((IsDepth() && !ZeroGS::IsWriteDepth()) || NotResolveHelper()) {
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

#if defined(DEVBUILD)
		if( g_bSaveResolved ) {
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

	assert( startrange < end && endrange > start ); // make sure it at least intersects

	if( ptex != 0 && !(status&TS_Resolved) && !(status&TS_NeedUpdate) ) {

		// flush if necessary
		FlushIfNecesary ( this ) ;

#if defined(DEVBUILD)
		if( g_bSaveResolved ) {
			SaveTexture("resolved.tga", GL_TEXTURE_RECTANGLE_NV, ptex, RW(fbw), RH(fbh));
			g_bSaveResolved = 0;
		}
#endif

		if(g_GameSettings&GAME_NOTARGETRESOLVE) {
			status = TS_Resolved;
			return;
		}

		int blockheight = PSMT_ISHALF(psm) ? 64 : 32;
		int resolvefbp = fbp, resolveheight = fbh;

		int scanlinewidth = 0x2000*(fbw>>6);

		// in now way should data be overwritten!, instead resolve less
		if( endrange < end ) {
			// round down to nearest block and scanline
			resolveheight = ((endrange-start)/(0x2000*(fbw>>6))) * blockheight;
			if( resolveheight <= 32 ) {
				status = TS_Resolved;
				return;
			}
		}
		else if( startrange > start ) {
			// round up to nearest block and scanline
			resolvefbp = startrange + scanlinewidth - 1;
			resolvefbp -= resolvefbp % scanlinewidth;

			resolveheight = fbh-((resolvefbp-fbp)*blockheight/scanlinewidth);
			if( resolveheight <= 64 ) { // this is a total hack, but kh doesn't resolve now
				status = TS_Resolved;
				return;
			}

			resolvefbp >>= 8;
		}

		glBindTexture(GL_TEXTURE_RECTANGLE_NV, ptex);
		glGetTexImage(GL_TEXTURE_RECTANGLE_NV, 0, GL_RGBA, GL_UNSIGNED_BYTE, psys);
		GL_REPORT_ERRORD();

		u8* pbits = (u8*)psys;
		if( fbp != resolvefbp )
			pbits += ((resolvefbp-fbp)*256/scanlinewidth)*blockheight*Pitch( fbw );

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
	assert( pdepth != NULL );
	((CDepthTarget*)pdepth)->SetDepthStencilSurface();

	SetShaderCaller("CRenderTarget::Update");
	Vector v = DefaultBitBltPos();

	CRenderTargetMngr::MAPTARGETS::iterator ittarg;
	if( nUpdateTarg ) {
		ittarg = s_RTs.mapTargets.find(nUpdateTarg);
		if( ittarg == s_RTs.mapTargets.end() ) {
			ittarg = s_DepthRTs.mapTargets.find(nUpdateTarg);
			if( ittarg == s_DepthRTs.mapTargets.end() )
				nUpdateTarg = 0;
			else if( ittarg->second == this ) {
				ERROR_LOG("updating self");
				nUpdateTarg = 0;
			}
		}
		else if( ittarg->second == this ) {
			ERROR_LOG("updating self");
			nUpdateTarg = 0;
		}
	}

	SetViewport();

	if( nUpdateTarg ) {

		cgGLSetTextureParameter(ppsBaseTexture.sFinal, ittarg->second->ptex);
		cgGLEnableTextureParameter(ppsBaseTexture.sFinal);

		//assert( ittarg->second->fbw == fbw );
		int offset = (fbp-ittarg->second->fbp)*64/fbw;
		if (PSMT_ISHALF(psm)) // 16 bit
			offset *= 2;

		v.x = (float)RW(fbw);
		v.y = (float)RH(fbh);
		v.z = 0.25f;
		v.w = (float)RH(offset) + 0.25f;
		ZZcgSetParameter4fv(pvsBitBlt.sBitBltTex, v, "g_fBitBltTex");
//		v = DefaultBitBltTex(); Maybe?
		v = DefaultOneColor ( ppsBaseTexture ) ;

		SETPIXELSHADER(ppsBaseTexture.prog);
		nUpdateTarg = 0;
	}
	else {
		// align the rect to the nearest page
		// note that fbp is always aligned on page boundaries
		tex0Info texframe;
		texframe.tbp0 = fbp;
		texframe.tbw = fbw;
		texframe.tw = fbw;
		texframe.th = fbh;
		texframe.psm = psm;
		CMemoryTarget* pmemtarg = g_MemTargs.GetMemoryTarget(texframe, 1);

		// write color and zero out stencil buf, always 0 context!
		// force bilinear if using AA
		// Fix in r133 -- FFX movies and Gust backgrounds!
		SetTexVariablesInt(0, 0*(s_AAx || s_AAy)?2:0, texframe, pmemtarg, &ppsBitBlt[!!s_AAx], 1);
		cgGLSetTextureParameter(ppsBitBlt[!!s_AAx].sMemory, pmemtarg->ptex->tex);
		cgGLEnableTextureParameter(ppsBitBlt[!!s_AAx].sMemory);

		v = Vector(1,1,0.0f,0.0f);
		ZZcgSetParameter4fv(pvsBitBlt.sBitBltTex, v, "g_fBitBltTex");

		v.x = 1;
		v.y = 2;
		ZZcgSetParameter4fv(ppsBitBlt[!!s_AAx].sOneColor, v, "g_fOneColor");

		assert( ptex != 0 );

		if( conf.options & GSOPTION_WIREFRAME ) glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		if( ZeroGS::IsWriteDestAlphaTest() ) {
			glEnable(GL_STENCIL_TEST);
			glStencilFunc(GL_ALWAYS, 0, 0xff);
			glStencilMask(0xff);
			glStencilOp(GL_KEEP, GL_KEEP, GL_ZERO);
		}

		// render with an AA shader if possible (bilinearly interpolates data)
		//cgGLLoadProgram(ppsBitBlt[!!s_AAx].prog);
		SETPIXELSHADER(ppsBitBlt[!!s_AAx].prog);
	}

	SETVERTEXSHADER(pvsBitBlt.prog);

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	// fill stencil buf only
	FillOnlyStencilBuffer();
	glEnable(GL_SCISSOR_TEST);

	if( conf.options & GSOPTION_WIREFRAME ) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	if( conf.mrtdepth && pdepth != NULL && ZeroGS::IsWriteDepth() )
		pdepth->SetRenderTarget(1);

	status = TS_Resolved;

	// reset since settings changed
	vb[0].bVarsTexSync = 0;
	ZeroGS::ResetAlphaVariables();
}

void ZeroGS::CRenderTarget::ConvertTo32()
{
	FUNCLOG

	u32 ptexConv;
//	ERROR_LOG("ZZogl: Convert to 32, report if something missing\n");
	// create new target
	if ( ! InitialiseDefaultTexture ( &ptexConv, RW(fbw), RH(fbh)/2 ) ) {
		ERROR_LOG("Failed to create target for ConvertTo32 %dx%d\n", RW(fbw), RH(fbh)/2);
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
	ZZcgSetParameter4fv(ppsConvert16to32.fTexOffset, v, "g_fTexOffset");

	v.x = (float)RW(8);
	v.y = 0;
	v.z = 0;
	v.w = 0.25f;
	ZZcgSetParameter4fv(ppsConvert16to32.fPageOffset, v, "g_fPageOffset");

	v.x = (float)RW(2 * fbw);
	v.y = (float)RH(fbh);
	v.z = 0;
	v.w = 0.0001f * (float)RH(fbh);
	ZZcgSetParameter4fv(ppsConvert16to32.fTexDims, v, "g_fTexDims");

//	v.x = 0;
//	ZZcgSetParameter4fv(ppsConvert16to32.fTexBlock, v, "g_fTexBlock");

	glBindBuffer(GL_ARRAY_BUFFER, vboRect);
	SET_STREAM();

	// assume depth already set !?
	glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_RECTANGLE_NV, ptexConv, 0 );
	ZeroGS::ResetRenderTarget(1);
	assert( glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT) == GL_FRAMEBUFFER_COMPLETE_EXT );

	BindToSample( &ptex ) ;

	cgGLSetTextureParameter(ppsConvert16to32.sFinal, ptex);
	cgGLEnableTextureParameter(ppsBitBlt[!!s_AAx].sMemory);

	fbh /= 2; // have 16 bit surfaces are usually 2x higher
	SetViewport();

	if( conf.options & GSOPTION_WIREFRAME ) glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	// render with an AA shader if possible (bilinearly interpolates data)
	SETVERTEXSHADER(pvsBitBlt.prog);
	SETPIXELSHADER(ppsConvert16to32.prog);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

#ifdef _DEBUG
	if( g_bSaveZUpdate ) {
		// buggy
		SaveTexture("tex1.tga", GL_TEXTURE_RECTANGLE_NV, ptex, RW(fbw), RH(fbh)*2);
		SaveTexture("tex3.tga", GL_TEXTURE_RECTANGLE_NV, ptexConv, RW(fbw), RH(fbh));
	}
#endif

	vposxy.y = -2.0f * (32767.0f / 8.0f) / (float)fbh;
	vposxy.w = 1+0.5f/fbh;

	// restore
	SAFE_RELEASE_TEX(ptex);
	SAFE_RELEASE_TEX(ptexFeedback);
	ptex = ptexConv;

	// no need to free psys since the render target is getting shrunk
	if( conf.options & GSOPTION_WIREFRAME ) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	// reset textures
	BindToSample ( &ptex ) ;

	glEnable(GL_SCISSOR_TEST);
	status = TS_Resolved;

	// TODO, reset depth?
	if( ZeroGS::icurctx >= 0 ) {
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

//	ERROR_LOG("ZZogl: Convert to 16, report if something missing\n");
	// create new target
	if ( ! InitialiseDefaultTexture ( &ptexConv, RW(fbw), RH(fbh)*2 ) ) {
		ERROR_LOG("Failed to create target for ConvertTo16 %dx%d\n", RW(fbw), RH(fbh)*2);
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
	ZZcgSetParameter4fv(ppsConvert32to16.fTexOffset, v, "g_fTexOffset");

	v.x = 256.0f / 255.0f;
	v.y = 256.0f / 255.0f;
	v.z = 0.05f / 256.0f;
	v.w = -0.001f / 256.0f;
	ZZcgSetParameter4fv(ppsConvert32to16.fPageOffset, v, "g_fPageOffset");

	v.x = (float)RW(fbw);
	v.y = (float)RH(2 * fbh);
	v.z = 0;
	v.w = -0.1f/RH(fbh);
	ZZcgSetParameter4fv(ppsConvert32to16.fTexDims, v, "g_fTexDims");

	glBindBuffer(GL_ARRAY_BUFFER, vboRect);
	SET_STREAM();

	// assume depth already set !?
	// assume depth already set !?
	glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_RECTANGLE_NV, ptexConv, 0 );
	ZeroGS::ResetRenderTarget(1);
	assert( glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT) == GL_FRAMEBUFFER_COMPLETE_EXT );

	BindToSample ( &ptex ) ;

	cgGLSetTextureParameter(ppsConvert32to16.sFinal, ptex);
	cgGLEnableTextureParameter(ppsConvert32to16.sFinal);

//	fbh *= 2; // have 16 bit surfaces are usually 2x higher

	SetViewport();

	if( conf.options & GSOPTION_WIREFRAME ) glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	// render with an AA shader if possible (bilinearly interpolates data)
	SETVERTEXSHADER(pvsBitBlt.prog);
	SETPIXELSHADER(ppsConvert32to16.prog);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

#ifdef _DEBUG
	//g_bSaveZUpdate = 1;
	if( g_bSaveZUpdate ) {
		SaveTexture("tex1.tga", GL_TEXTURE_RECTANGLE_NV, ptexConv, RW(fbw), RH(fbh));
	}
#endif

	vposxy.y = -2.0f * (32767.0f / 8.0f) / (float)fbh;
	vposxy.w = 1+0.5f/fbh;

	// restore
	SAFE_RELEASE_TEX(ptex);
	SAFE_RELEASE_TEX(ptexFeedback);
	ptex = ptexConv;

	_aligned_free(psys);
	psys = _aligned_malloc( Tex_Memory_Size ( fbw, fbh ), 16 );

	if( conf.options & GSOPTION_WIREFRAME ) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	// reset textures
	BindToSample ( &ptex ) ;
	glEnable(GL_SCISSOR_TEST);

	status = TS_Resolved;

	// TODO, reset depth?
	if( ZeroGS::icurctx >= 0 ) {
		// reset since settings changed
		vb[icurctx].bVarsTexSync = 0;
		vb[icurctx].bVarsSetTarg = 0;
	}
	vb[0].bVarsTexSync = 0;
}

void ZeroGS::CRenderTarget::_CreateFeedback()
{
	FUNCLOG

	if( ptexFeedback == 0 ) {
		// create
		if ( ! InitialiseDefaultTexture( &ptexFeedback, RW(fbw), RH(fbh) ) ) {
			ERROR_LOG("Failed to create feedback %dx%d\n", RW(fbw), RH(fbh));
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
	ZZcgSetParameter4fv(pvsBitBlt.sBitBltTex, v, "BitBltTex");*/

	// tex coords, test ffx bikanel island when changing these

//	Vector v = Vector(1, -1, 0.5f / (fbw<<s_AAx), 0.5f / (fbh << s_AAy));
//	v *= 1/32767.0f;
//	cgGLSetParameter4fv(pvsBitBlt.sBitBltPos, v);
	Vector v = DefaultBitBltPos();

	v.x = (float)(RW(fbw));
	v.y = (float)(RH(fbh));
	v.z = 0.0f;
	v.w = 0.0f;
	cgGLSetParameter4fv(pvsBitBlt.sBitBltTex, v);
	v = DefaultOneColor(ppsBaseTexture);

	glBindBuffer(GL_ARRAY_BUFFER, vboRect);
	SET_STREAM();

	glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_RECTANGLE_NV, ptexFeedback, 0 );
	glBindTexture(GL_TEXTURE_RECTANGLE_NV, ptex);
	assert( glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT) == GL_FRAMEBUFFER_COMPLETE_EXT );

	cgGLSetTextureParameter(ppsBaseTexture.sFinal, ptex);
	cgGLEnableTextureParameter(ppsBaseTexture.sFinal);

	SetViewport();

	if( conf.options & GSOPTION_WIREFRAME ) glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	// render with an AA shader if possible (bilinearly interpolates data)
	SETVERTEXSHADER(pvsBitBlt.prog);
	SETPIXELSHADER(ppsBaseTexture.prog);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	// restore
	swap(ptex, ptexFeedback);

	if( conf.options & GSOPTION_WIREFRAME ) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	glEnable(GL_SCISSOR_TEST);
	status |= TS_FeedbackReady;

	// TODO, reset depth?
	if( ZeroGS::icurctx >= 0 ) {
		// reset since settings changed
		vb[icurctx].bVarsTexSync = 0;
	}

	assert( glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT) == GL_FRAMEBUFFER_COMPLETE_EXT );
}

void ZeroGS::CRenderTarget::SetRenderTarget(int targ)
{
	FUNCLOG

	glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT+targ, GL_TEXTURE_RECTANGLE_NV, ptex, 0 );

//	if (glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT) == GL_FRAMEBUFFER_COMPLETE_EXT)
//		ERROR_LOG_SPAM("Too bad Framebuffer not compele, glitches could appear onscreen!\n");
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

	if( !CRenderTarget::Create(frame) )
		return false;

	GL_REPORT_ERROR();

	glGenRenderbuffersEXT( 1, &pdepth );
	glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, pdepth);
	glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_DEPTH24_STENCIL8_EXT, RW(fbw), RH(fbh));

	if (glGetError() != GL_NO_ERROR) {
		// try a separate depth and stencil buffer
		glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, pdepth);
		glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT24, RW(fbw), RH(fbh));

		if (g_bUpdateStencil) {
			glGenRenderbuffersEXT( 1, &pstencil );
			glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, pstencil);
			glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_STENCIL_INDEX8_EXT, RW(fbw), RH(fbh));

			if( glGetError() != GL_NO_ERROR ) {
				ERROR_LOG("failed to create depth buffer %dx%d\n", RW(fbw), RH(fbh));
				return false;
			}
		}
		else pstencil = 0;
	}
	else
		pstencil = pdepth;

	status = TS_NeedUpdate;

	return true;
}

void ZeroGS::CDepthTarget::Destroy()
{
	FUNCLOG

	if ( status ) { // In this case Framebuffer extension is off-use and lead to segfault
		ResetRenderTarget(1);
		glFramebufferRenderbufferEXT( GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, 0 );
		glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_STENCIL_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, 0 );
		GL_REPORT_ERRORD();

		if( pstencil != 0 ) {
				if( pstencil != pdepth )
				glDeleteRenderbuffersEXT( 1, &pstencil );
				pstencil = 0;
		}

		if( pdepth != 0 ) {
				glDeleteRenderbuffersEXT( 1, &pdepth );
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

	if( g_nDepthUsed > 0 && conf.mrtdepth && !(status&TS_Virtual) && ZeroGS::IsWriteDepth() && !(g_GameSettings&GAME_NODEPTHRESOLVE) )
		CRenderTarget::Resolve();
	else {
		// flush if necessary
		FlushIfNecesary ( this ) ;
		if( !(status & TS_Virtual) )
			status |= TS_Resolved;
	}

	if( !(status&TS_Virtual) ) {
		ZeroGS::SetWriteDepth();
	}
}

void ZeroGS::CDepthTarget::Resolve(int startrange, int endrange)
{
	FUNCLOG

	if( g_nDepthUsed > 0 && conf.mrtdepth && !(status&TS_Virtual) && ZeroGS::IsWriteDepth() )
		CRenderTarget::Resolve(startrange, endrange);
	else {
		// flush if necessary
		FlushIfNecesary ( this ) ;
		if( !(status & TS_Virtual) )
			status |= TS_Resolved;
	}

	if( !(status&TS_Virtual) ) {
		ZeroGS::SetWriteDepth();
	}
}

extern int g_nDepthUpdateCount;

void ZeroGS::CDepthTarget::Update(int context, ZeroGS::CRenderTarget* prndr)
{
	FUNCLOG

	assert( !(status & TS_Virtual) );

	// align the rect to the nearest page
	// note that fbp is always aligned on page boundaries
	tex0Info texframe;
	texframe.tbp0 = fbp;
	texframe.tbw = fbw;
	texframe.tw = fbw;
	texframe.th = fbh;
	texframe.psm = psm;
	CMemoryTarget* pmemtarg = g_MemTargs.GetMemoryTarget(texframe, 1);

	DisableAllgl () ;

	ZeroGS::VB& curvb = vb[context];
	if (curvb.test.zte == 0)
		return;

	SetShaderCaller("CDepthTarget::Update");
	glEnable(GL_DEPTH_TEST);
	glDepthMask(!curvb.zbuf.zmsk);

	static const u32 g_dwZCmp[] = { GL_NEVER, GL_ALWAYS, GL_GEQUAL, GL_GREATER };
	glDepthFunc(g_dwZCmp[curvb.test.ztst]);

	// write color and zero out stencil buf, always 0 context!
	SetTexVariablesInt(0, 0, texframe, pmemtarg, &ppsBitBltDepth, 1);
	cgGLSetTextureParameter(ppsBitBltDepth.sMemory, pmemtarg->ptex->tex);
	cgGLEnableTextureParameter(ppsBaseTexture.sFinal);

	Vector v = DefaultBitBltPos() ;
	v = DefaultBitBltTex() ;

	v.x = 1;
	v.y = 2;
	v.z = PSMT_IS16Z(psm)?1.0f:0.0f;
	v.w = g_filog32;
	ZZcgSetParameter4fv(ppsBitBltDepth.sOneColor, v, "g_fOneColor");

	Vector vdepth = g_vdepth;
	if (psm == PSMT24Z)
		vdepth.w = 0;
	else if (psm != PSMT32Z) {
		vdepth.z = vdepth.w = 0;
	}

	assert( ppsBitBltDepth.sBitBltZ != 0 );
	ZZcgSetParameter4fv(ppsBitBltDepth.sBitBltZ, ((255.0f/256.0f)*vdepth), "g_fBitBltZ");

	assert( pdepth != 0 );

	glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_RECTANGLE_NV, ptex, 0 );
	SetDepthStencilSurface();
	glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT1_EXT, GL_TEXTURE_RECTANGLE_NV, 0, 0 );
	GLenum buffer = GL_COLOR_ATTACHMENT0_EXT;
	if( glDrawBuffers != NULL )
		glDrawBuffers(1, &buffer);
	int stat = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
	assert( glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT) == GL_FRAMEBUFFER_COMPLETE_EXT );

	SetViewport();
	if( conf.options & GSOPTION_WIREFRAME ) glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	glBindBuffer(GL_ARRAY_BUFFER, vboRect);
	SET_STREAM();

	SETVERTEXSHADER(pvsBitBlt.prog);
	SETPIXELSHADER(ppsBitBltDepth.prog);

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	status = TS_Resolved;

	if( !ZeroGS::IsWriteDepth() ) {
		ResetRenderTarget(1);
	}

	if( conf.options & GSOPTION_WIREFRAME ) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glEnable(GL_SCISSOR_TEST);

#ifdef _DEBUG
	if( g_bSaveZUpdate ) {
		SaveTex(&texframe, 1);
		SaveTexture("frame1.tga", GL_TEXTURE_RECTANGLE_NV, ptex, RW(fbw), RH(fbh));
	}
#endif
}

void ZeroGS::CDepthTarget::SetDepthStencilSurface()
{
	FUNCLOG
	glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, pdepth );

	if( pstencil ) {
		// there's a bug with attaching stencil and depth buffers
		glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_STENCIL_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, pstencil );

		if( icount++ < 8 ) { // not going to fail if succeeded 4 times
			GLenum status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
			if( status != GL_FRAMEBUFFER_COMPLETE_EXT ) {
				glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_STENCIL_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, 0 );
				if( pstencil != pdepth )
					glDeleteRenderbuffersEXT(1, &pstencil);
				pstencil = 0;
				g_bUpdateStencil = 0;
			}
		}
	}
	else
		glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_STENCIL_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, 0 );
}

void ZeroGS::CRenderTargetMngr::Destroy()
{
	FUNCLOG
	for(MAPTARGETS::iterator it = mapTargets.begin(); it != mapTargets.end(); ++it)
		delete it->second;
	mapTargets.clear();
	for(MAPTARGETS::iterator it = mapDummyTargs.begin(); it != mapDummyTargs.end(); ++it)
		delete it->second;
	mapDummyTargs.clear();
}

void ZeroGS::CRenderTargetMngr::DestroyAllTargs(int start, int end, int fbw)
{
	FUNCLOG
	for(MAPTARGETS::iterator it = mapTargets.begin(); it != mapTargets.end();) {
		if( it->second->start < end && start < it->second->end ) {
			// if is depth, only resolve if fbw is the same
			if( !it->second->IsDepth() ) {
				// only resolve if the widths are the same or it->second has bit outside the range
				// shadow of colossus swaps between fbw=256,fbh=256 and fbw=512,fbh=448. This kills the game if doing || it->second->end > end

				// kh hack, sometimes kh movies do this to clear the target, so have a static count that periodically checks end
				static int count = 0;

				if( it->second->fbw == fbw || (it->second->fbw != fbw && (it->second->start < start || ((count++&0xf)?0:it->second->end > end) )) )
					it->second->Resolve();
				else {
					FlushIfNecesary ( it->second ) ;
					it->second->status |= CRenderTarget::TS_Resolved;
				}
			}
			else {
				if( it->second->fbw == fbw )
					it->second->Resolve();
				else {
					FlushIfNecesary ( it->second ) ;
					it->second->status |= CRenderTarget::TS_Resolved;
				}
			}

			DestroyAllTargetsHelper( it->second ) ;

			u32 dummykey = GetFrameKeyDummy(it->second);
			if( mapDummyTargs.find(dummykey) == mapDummyTargs.end() ) {
				mapDummyTargs[dummykey] = it->second;
			}
			else
				delete it->second;
			mapTargets.erase(it++);
		}
		else ++it;
	}
}

void ZeroGS::CRenderTargetMngr::DestroyTarg(CRenderTarget* ptarg)
{
	FUNCLOG
	DestroyAllTargetsHelper ( ptarg ) ;
	delete ptarg;
}

void ZeroGS::CRenderTargetMngr::DestroyIntersecting(CRenderTarget* prndr)
{
	FUNCLOG
	assert( prndr != NULL );

	int start, end;
	GetRectMemAddress(start, end, prndr->psm, 0, 0, prndr->fbw, prndr->fbh, prndr->fbp, prndr->fbw);

	for(MAPTARGETS::iterator it = mapTargets.begin(); it != mapTargets.end();) {
		if( it->second != prndr && it->second->start < end && start < it->second->end ) {
			it->second->Resolve();

			DestroyAllTargetsHelper( it->second ) ;

			u32 dummykey = GetFrameKeyDummy(it->second);
			if( mapDummyTargs.find(dummykey) == mapDummyTargs.end() ) {
				mapDummyTargs[dummykey] = it->second;
			}
			else
				delete it->second;
			mapTargets.erase(it++);
		}
		else ++it;
	}
}

//--------------------------------------------------


inline bool CheckWidthIsSame (const frameInfo& frame, CRenderTarget* ptarg) {
	if (PSMT_ISHALF(frame.psm) == PSMT_ISHALF(ptarg->psm))
		return (frame.fbw == ptarg->fbw);
	if (PSMT_ISHALF(frame.psm))
		return (frame.fbw == 2 * ptarg->fbw);
	else
		return (2 * frame.fbw == ptarg->fbw);
}

CRenderTarget* ZeroGS::CRenderTargetMngr::GetTarg(const frameInfo& frame, u32 opts, int maxposheight)
{
	FUNCLOG
	if( frame.fbw <= 0 || frame.fbh <= 0 )
		return NULL;

	GL_REPORT_ERRORD();

	u32 key = GetFrameKey(frame);
	MAPTARGETS::iterator it = mapTargets.find(key);

	// only enforce height if frame.fbh <= 0x1c0
	bool bfound = it != mapTargets.end();
	if( bfound ) {
		if( opts&TO_StrictHeight ) {
			bfound = it->second->fbh == frame.fbh;

			if( (g_GameSettings&GAME_PARTIALDEPTH) && !bfound ) {
				MAPTARGETS::iterator itnew = mapTargets.find(key+1);
				if( itnew != mapTargets.end() && itnew->second->fbh == frame.fbh ) {
					// found! delete the previous and restore
					delete it->second;
					mapTargets.erase(it);

					it = mapTargets.insert(MAPTARGETS::value_type(key, itnew->second)).first; // readd
					mapTargets.erase(itnew); // delete old

					bfound = true;
				}
			}
		}
		else {
			if( PSMT_ISHALF(frame.psm)==PSMT_ISHALF(it->second->psm) && !(g_GameSettings & GAME_FULL16BITRES) )
				bfound = (frame.fbh > 0x1c0 || it->second->fbh >= frame.fbh) && it->second->fbh <= maxposheight;
		}
	}

	if( !bfound ) {
		// might be a virtual target
		it = mapTargets.find(key|TARGET_VIRTUAL_KEY);
		bfound = it != mapTargets.end() && ((opts&TO_StrictHeight) ? it->second->fbh == frame.fbh : it->second->fbh >= frame.fbh) && it->second->fbh <= maxposheight;
	}

	if( bfound && PSMT_ISHALF(frame.psm) && PSMT_ISHALF(it->second->psm) && (g_GameSettings&GAME_FULL16BITRES) ) {
		// mgs3
		if( frame.fbh > it->second->fbh ) {
			bfound = false;
		}
	}

	if( bfound ) {

		// can be both 16bit and 32bit
		if( PSMT_ISHALF(frame.psm) != PSMT_ISHALF(it->second->psm) ) {
			// a lot of games do this actually...
#ifdef _DEBUG
			WARN_LOG("Really bad formats! %d %d\n", frame.psm, it->second->psm);
#endif

//			This code SHOULD be commented, until I redone _Resolve function
			if( !(opts&TO_StrictHeight) ) {
				if( (g_GameSettings & GAME_VSSHACKOFF) ) {
					if (PSMT_ISHALF(it->second->psm)) {
						it->second->status |= CRenderTarget::TS_NeedConvert32;
						it->second->fbh /= 2;
					}
					else {
						it->second->status |= CRenderTarget::TS_NeedConvert16;
						it->second->fbh *= 2;
					}
				}
			}

			// recalc extents
			GetRectMemAddress(it->second->start, it->second->end, frame.psm, 0, 0, frame.fbw, it->second->fbh, it->second->fbp, frame.fbw);
		}
		else {
			// certain variables have to be reset every time
			if( (it->second->psm&~1) != (frame.psm&~1) ) {
#ifndef RELEASE_TO_PUBLIC
				WARN_LOG("bad formats 2: %d %d\n", frame.psm, it->second->psm);
#endif
				it->second->psm = frame.psm;

				// recalc extents
				GetRectMemAddress(it->second->start, it->second->end, frame.psm, 0, 0, frame.fbw, it->second->fbh, it->second->fbp, frame.fbw);
			}
		}

		if( it->second->fbm != frame.fbm ) {
			//WARN_LOG("bad fbm: 0x%8.8x 0x%8.8x, psm: %d\n", frame.fbm, it->second->fbm, frame.psm);
		}

		it->second->fbm &= frame.fbm;
		it->second->psm = frame.psm; // have to convert (ffx2)

		if( (it->first & TARGET_VIRTUAL_KEY) && !(opts&TO_Virtual) ) {
			// switch
			it->second->lastused = timeGetTime();
			return Promote(it->first&~TARGET_VIRTUAL_KEY);
		}

		// check if there exists a more recent target that this target could update from
		// only update if target isn't mirrored
		bool bCheckHalfCovering = (g_GameSettings&GAME_FULL16BITRES) && PSMT_ISHALF(it->second->psm) && it->second->fbh +32 < frame.fbh;

		for(MAPTARGETS::iterator itnew = mapTargets.begin(); itnew != mapTargets.end(); ++itnew) {
			if( itnew->second != it->second && itnew->second->ptex != it->second->ptex && itnew->second->ptexFeedback != it->second->ptex &&
				itnew->second->lastused > it->second->lastused && !(itnew->second->status & CRenderTarget::TS_NeedUpdate) ) {

				// if new target totally encompasses the current one
				if( itnew->second->start <= it->second->start && itnew->second->end >= it->second->end ) {
					it->second->status |= CRenderTarget::TS_NeedUpdate;
					it->second->nUpdateTarg = itnew->first;
					break;
				}

				// if 16bit, then check for half encompassing targets
				if( bCheckHalfCovering && itnew->second->start > it->second->start && itnew->second->start < it->second->end && itnew->second->end <= it->second->end+0x2000 ) {
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

	if( !(opts & CRenderTargetMngr::TO_Virtual) ) {

		int start, end;
		GetRectMemAddress(start, end, frame.psm, 0, 0, frame.fbw, frame.fbh, frame.fbp, frame.fbw);
		CRenderTarget* pbesttarg = NULL;


		if( besttarg == 0 ) {
			// if there is only one intersecting target and it encompasses the current one, update the new render target with
			// its data instead of resolving then updating (ffx2). Do not change the original target.
			for(MAPTARGETS::iterator it = mapTargets.begin(); it != mapTargets.end(); ++it) {
				if( it->second->start < end && start < it->second->end ) {
						if ((g_GameSettings&GAME_FASTUPDATE) ||
							((frame.fbw == it->second->fbw) &&
							// check depth targets only if partialdepth option
							((it->second->fbp != frame.fbp) || ((g_GameSettings & GAME_PARTIALDEPTH) && (opts & CRenderTargetMngr::TO_DepthBuffer)))))
						{
							if( besttarg != 0 ) {
								besttarg = 0;
								break;
							}

							if( start >= it->second->start && end <= it->second->end ) {
								besttarg = it->first;
								pbesttarg = it->second;
							}
						}
				}
			}
		}

		if (besttarg != 0 && pbesttarg->fbw != frame.fbw) {
//			printf ("A %d %d %d %d\n", frame.psm, frame.fbw, pbesttarg->psm, pbesttarg->fbw);

			vb[0].frame.fbw = pbesttarg->fbw;
			// Something should be here, but what?
		}

		if( besttarg == 0 ) {
			// if none found, resolve all
			DestroyAllTargs(start, end, frame.fbw);
		}
		else if( key == besttarg && pbesttarg != NULL ) {
			// add one and store in a different location until best targ is processed
			mapTargets.erase(besttarg);
			besttarg++;
			mapTargets[besttarg] = pbesttarg;
		}
	}

	if( mapTargets.size() > 8 ) {
		// release some resources
		it = GetOldestTarg(mapTargets);

		// if more than 5s passed since target used, destroy
		if( it->second != vb[0].prndr && it->second != vb[1].prndr && it->second != vb[0].pdepth && it->second != vb[1].pdepth &&
			timeGetTime()-it->second->lastused > 5000 ) {
			delete it->second;
			mapTargets.erase(it);
		}
	}

	if( ptarg == NULL ) {
		// not found yet, so create

		if( mapDummyTargs.size() > 8 ) {
			it = GetOldestTarg(mapDummyTargs);

			delete it->second;
			mapDummyTargs.erase(it);
		}

		it = mapDummyTargs.find(GetFrameKeyDummy(frame));

		if (it != mapDummyTargs.end()) {
#ifdef DEBUG
			printf ("A %x %x %x %x\n", frame.fbw, frame.fbh, frame.psm, frame.fbp);
			for(MAPTARGETS::iterator it1 = mapDummyTargs.begin(); it1 != mapDummyTargs.end(); ++it1)
				printf ("\t %x %x %x %x\n", it1->second->fbw, it1->second->fbh, it1->second->psm, it1->second->fbp);
			for(MAPTARGETS::iterator it1 = mapTargets.begin(); it1 != mapTargets.end(); ++it1)
				printf ("\t ! %x %x %x %x\n", it1->second->fbw, it1->second->fbh, it1->second->psm, it1->second->fbp);
			printf ("\t\t %x %x %x %x\n", it->second->fbw, it->second->fbh, it->second->psm, it->second->fbp);
#endif
			ptarg = it->second;
			mapDummyTargs.erase(it);

			// restore all setttings
			ptarg->psm = frame.psm;
			ptarg->fbm = frame.fbm;
			ptarg->fbp = frame.fbp;
			GetRectMemAddress(ptarg->start, ptarg->end, frame.psm, 0, 0, frame.fbw, frame.fbh, frame.fbp, frame.fbw);

			ptarg->status = CRenderTarget::TS_NeedUpdate;
		}
		else {
#ifdef DEBUG
			printf ("A %x %x %x %x\n", frame.fbw, frame.fbh, frame.psm, frame.fbp);
			for(MAPTARGETS::iterator it1 = mapDummyTargs.begin(); it1 != mapDummyTargs.end(); ++it1)
				printf ("\t %x %x %x %x\n", it1->second->fbw, it1->second->fbh, it1->second->psm, it1->second->fbp);
			for(MAPTARGETS::iterator it1 = mapTargets.begin(); it1 != mapTargets.end(); ++it1)
				printf ("\t ! %x %x %x %x\n", it1->second->fbw, it1->second->fbh, it1->second->psm, it1->second->fbp);
#endif
			// create anew
			ptarg = (opts&TO_DepthBuffer) ? new CDepthTarget : new CRenderTarget;
			CRenderTargetMngr* pmngrs[2] = { &s_DepthRTs, this == &s_RTs ? &s_RTs : NULL };
			int cur = 0;

			while( !ptarg->Create(frame) ) {

				// destroy unused targets
				if( mapDummyTargs.size() > 0 ) {
					it = mapDummyTargs.begin();
					delete it->second;
					mapDummyTargs.erase(it);
					continue;
				}

				if( g_MemTargs.listClearedTargets.size() > 0 ) {
					g_MemTargs.DestroyCleared();
					continue;
				}
				else
				if( g_MemTargs.listTargets.size() > 32 ) {
					g_MemTargs.DestroyOldest();
					continue;
				}

				if( pmngrs[cur] == NULL ) {
					cur = !cur;
					if( pmngrs[cur] == NULL ) {
						WARN_LOG("Out of memory!\n");
						delete ptarg;
						return NULL;
					}
				}

				if( pmngrs[cur]->mapTargets.size() == 0 )
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

	if( (opts & CRenderTargetMngr::TO_Virtual) ) {
		ptarg->status = CRenderTarget::TS_Virtual;
		key |= TARGET_VIRTUAL_KEY;

		if( (it = mapTargets.find(key)) != mapTargets.end() ) {

			DestroyTarg(it->second);
			it->second = ptarg;
			ptarg->nUpdateTarg = besttarg;
			return ptarg;
		}
	}
	else
		assert( mapTargets.find(key) == mapTargets.end());

	ptarg->nUpdateTarg = besttarg;
	mapTargets[key] = ptarg;
	return ptarg;
}

ZeroGS::CRenderTargetMngr::MAPTARGETS::iterator ZeroGS::CRenderTargetMngr::GetOldestTarg(MAPTARGETS& m)
{
	FUNCLOG
	if( m.size() == 0 ) {
		return m.end();
	}

	// release some resources
	u32 curtime = timeGetTime();
	MAPTARGETS::iterator itmaxtarg = m.begin();
	for(MAPTARGETS::iterator it = ++m.begin(); it != m.end(); ++it) {
		if( itmaxtarg->second->lastused-curtime < it->second->lastused-curtime ) itmaxtarg = it;
	}

	return itmaxtarg;
}

void ZeroGS::CRenderTargetMngr::GetTargs(int start, int end, list<ZeroGS::CRenderTarget*>& listTargets) const
{
	FUNCLOG
	for(MAPTARGETS::const_iterator it = mapTargets.begin(); it != mapTargets.end(); ++it) {
		if( it->second->start < end && start < it->second->end ) listTargets.push_back(it->second);
	}
}

void ZeroGS::CRenderTargetMngr::Resolve(int start, int end)
{
	FUNCLOG
	for(MAPTARGETS::const_iterator it = mapTargets.begin(); it != mapTargets.end(); ++it) {
		if( it->second->start < end && start < it->second->end )
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
	assert( (clutsize&31) == 0 );

	// left > 0 only when csa < 16
	int left = ((u32)(uptr)pClutBuffer & 2) ? 0 : (((u32)(uptr)pClutBuffer & 0x3ff)/2) + clutsize - 512;
	if( left > 0 ) clutsize -= left;

	while(clutsize > 0) {
		for(int i = 0; i < 16; ++i) {
			if( pSavedBuffer[i] != pClutBuffer[2*i] )
				return 1;
		}

		clutsize -= 32;
		pSavedBuffer += 16;
		pClutBuffer += 32;
	}

	if( left > 0 ) {
		pClutBuffer = (u16*)(g_pbyGSClut + 2);

		while(left > 0) {
			for(int i = 0; i < 16; ++i) {
				if( pSavedBuffer[i] != pClutBuffer[2*i] )
					return 1;
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
	assert( tex0.psm == psm && PSMT_ISCLUT(psm) && cpsm == tex0.cpsm );

	int nClutOffset = 0;
	int clutsize = 0;

	int entries = PSMT_IS8CLUT(tex0.psm) ? 256 : 16;
	if (PSMT_IS32BIT(tex0.cpsm)) { // 32 bit
		nClutOffset = 64 * tex0.csa;
		clutsize = min(entries, 256-tex0.csa*16)*4;
	}
	else {
		nClutOffset = 32 * (tex0.csa&15) + (tex0.csa>=16?2:0);
		clutsize = min(entries, 512-tex0.csa*16)*2;
	}

	assert( clutsize == clut.size() );

	if( PSMT_IS32BIT(cpsm)) {
		if( memcmp_mmx(&clut[0], g_pbyGSClut+nClutOffset, clutsize) )
			return false;
	}
	else {
		if( memcmp_clut16((u16*)&clut[0], (u16*)(g_pbyGSClut+nClutOffset), clutsize) )
			return false;
	}

	return true;
}

int VALIDATE_THRESH = 8;
u32 TEXDESTROY_THRESH = 16;

bool ZeroGS::CMemoryTarget::ValidateTex(const tex0Info& tex0, int starttex, int endtex, bool bDeleteBadTex)
{
	FUNCLOG
	if( clearmaxy == 0 )
		return true;

	int checkstarty = max(starttex, clearminy);
	int checkendy = min(endtex, clearmaxy);
	if( checkstarty >= checkendy )
		return true;

	if( validatecount++ > VALIDATE_THRESH ) {
		height = 0;
		return false;
	}

	// lock and compare
	assert( ptex != NULL && ptex->memptr != NULL);

	int result = memcmp_mmx(ptex->memptr + (checkstarty-realy)*4*GPU_TEXWIDTH, g_pbyGSMemory+checkstarty*4*GPU_TEXWIDTH, (checkendy-checkstarty)*4*GPU_TEXWIDTH);

	if( result == 0 || !bDeleteBadTex ) {
		if( result == 0 ) clearmaxy = 0;
		return result == 0;
	}

	// delete clearminy, clearmaxy range (not the checkstarty, checkendy range)
	//int newstarty = 0;
	if( clearminy <= starty ) {
		if( clearmaxy < starty + height) {
			// preserve end
			height = starty + height - clearmaxy;
			starty = clearmaxy;
			assert(height > 0);
		}
		else {
			// destroy
			height = 0;
		}
	}
	else {
		// beginning can be preserved
		height = clearminy-starty;
	}

	clearmaxy = 0;
	assert( starty >= realy && starty+height<=realy+realheight );

	return false;
}

// used to build clut textures (note that this is for both 16 and 32 bit cluts)
template <class T>
static __forceinline void BuildClut(u32 psm, u32 height, T* pclut, u8* psrc, T* pdst)
{
	switch(psm)
	{
		case PSMT8:
			for(u32 i = 0; i < height; ++i)
			{
				for(int j = 0; j < GPU_TEXWIDTH/2; ++j)
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
			for(u32 i = 0; i < height; ++i)
			{
				for(int j = 0; j < GPU_TEXWIDTH; ++j)
				{
					pdst[0] = pclut[psrc[0]&15];
					pdst[1] = pclut[psrc[0]>>4];
					pdst[2] = pclut[psrc[1]&15];
					pdst[3] = pclut[psrc[1]>>4];
					pdst[4] = pclut[psrc[2]&15];
					pdst[5] = pclut[psrc[2]>>4];
					pdst[6] = pclut[psrc[3]&15];
					pdst[7] = pclut[psrc[3]>>4];

					pdst += 8;
					psrc += 4;
				}
			}
			break;

		case PSMT8H:
			for(u32 i = 0; i < height; ++i)
			{
				for(int j = 0; j < GPU_TEXWIDTH/8; ++j)
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
			for(u32 i = 0; i < height; ++i)
			{
				for(int j = 0; j < GPU_TEXWIDTH/8; ++j)
				{
					pdst[0] = pclut[psrc[3]>>4];
					pdst[1] = pclut[psrc[7]>>4];
					pdst[2] = pclut[psrc[11]>>4];
					pdst[3] = pclut[psrc[15]>>4];
					pdst[4] = pclut[psrc[19]>>4];
					pdst[5] = pclut[psrc[23]>>4];
					pdst[6] = pclut[psrc[27]>>4];
					pdst[7] = pclut[psrc[31]>>4];
					pdst += 8;
					psrc += 32;
				}
			}
			break;

		case PSMT4HL:
			for(u32 i = 0; i < height; ++i)
			{
				for(int j = 0; j < GPU_TEXWIDTH/8; ++j)
				{
					pdst[0] = pclut[psrc[3]&15];
					pdst[1] = pclut[psrc[7]&15];
					pdst[2] = pclut[psrc[11]&15];
					pdst[3] = pclut[psrc[15]&15];
					pdst[4] = pclut[psrc[19]&15];
					pdst[5] = pclut[psrc[23]&15];
					pdst[6] = pclut[psrc[27]&15];
					pdst[7] = pclut[psrc[31]&15];
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

extern int g_MaxTexWidth, g_MaxTexHeight;

//#define SORT_TARGETS
inline list<CMemoryTarget>::iterator ZeroGS::CMemoryTargetMngr::DestroyTargetIter(list<CMemoryTarget>::iterator& it)
{
	// find the target and destroy
	list<CMemoryTarget>::iterator itprev = it; ++it;
	listClearedTargets.splice(listClearedTargets.end(), listTargets, itprev);

	if( listClearedTargets.size() > TEXDESTROY_THRESH ) {
		listClearedTargets.pop_front();
	}

	return it;
}

int MemoryTarget_CompareTarget (list<CMemoryTarget>::iterator& it, const tex0Info& tex0, int clutsize, int nClutOffset) {
	if( PSMT_ISCLUT(it->psm) != PSMT_ISCLUT(tex0.psm) ) {
		return 1;
	}

	if( PSMT_ISCLUT(tex0.psm) ) {
		assert( it->clut.size() > 0 );

		if( it->psm != tex0.psm || it->cpsm != tex0.cpsm || it->clut.size() != clutsize ) {
			return 1;
		}

		if	(PSMT_IS32BIT(tex0.cpsm)) {
			if (memcmp_mmx(&it->clut[0], g_pbyGSClut+nClutOffset, clutsize)) {
				return 2;
			}
		}
		else {
			if (memcmp_clut16((u16*)&it->clut[0], (u16*)(g_pbyGSClut+nClutOffset), clutsize)) {
				return 2;
			}
		}

	}
	else
		if ( PSMT_IS16BIT(tex0.psm) != PSMT_IS16BIT(it->psm) ) {
			return 1;
		}
	return 0;
}

void MemoryTarget_GetClutVariables (int& nClutOffset, int& clutsize, const tex0Info& tex0) {
	nClutOffset = 0;
	clutsize = 0;

	if( PSMT_ISCLUT(tex0.psm) ) {
		int entries = PSMT_IS8CLUT(tex0.psm) ? 256 : 16;
		if (PSMT_IS32BIT(tex0.cpsm)) {
			nClutOffset = 64 * tex0.csa;
			clutsize = min(entries, 256-tex0.csa*16)*4;
		}
		else {
			nClutOffset = 64 * (tex0.csa&15) + (tex0.csa>=16?2:0);
			clutsize = min(entries, 512-tex0.csa*16)*2;
		}
	}
}

void MemoryTarget_GetMemAddress(int& start, int& end,  const tex0Info& tex0) {
	int nbStart, nbEnd;
	GetRectMemAddress(nbStart, nbEnd, tex0.psm, 0, 0, tex0.tw, tex0.th, tex0.tbp0, tex0.tbw);
	assert( nbStart < nbEnd );
	nbEnd = min(nbEnd, 0x00400000);

	start = nbStart / (4*GPU_TEXWIDTH);
	end = (nbEnd + GPU_TEXWIDTH*4 - 1) / (4*GPU_TEXWIDTH);
	assert( start < end );

}

ZeroGS::CMemoryTarget* ZeroGS::CMemoryTargetMngr::MemoryTarget_SearchExistTarget (int start, int end, int nClutOffset, int clutsize, const tex0Info& tex0, int forcevalidate) {
	for(list<CMemoryTarget>::iterator it = listTargets.begin(); it != listTargets.end(); ) {

		if( it->starty <= start && it->starty+it->height >= end ) {

			int res = MemoryTarget_CompareTarget (it, tex0, clutsize, nClutOffset);

			if (res == 1) {
 				if( it->validatecount++ > VALIDATE_THRESH ) {
					it = DestroyTargetIter(it);
					if( listTargets.size() == 0 )
						break;
				}
				else
					++it;
				continue;
			}
			else if (res == 2) {
				++it;
				continue;
			}

			if( forcevalidate ) {//&& listTargets.size() < TARGET_THRESH ) {
				// do more validation checking. delete if not been used for a while
				if( !it->ValidateTex(tex0, start, end, curstamp > it->usedstamp + 3) ) {

					if( it->height <= 0 ) {
						it = DestroyTargetIter(it);
						if( listTargets.size() == 0 )
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
		else if( it->starty >= end )
			break;
#endif

		++it;
	}
	return NULL;
}

static __forceinline int NumberOfChannels(int psm)
{
	int channels = 1;

	if (PSMT_ISCLUT(psm))
	{
		if (psm == PSMT8)
			channels = 4;
		else if (psm == PSMT4)
			channels = 8;
	}
	else
	{
		if (PSMT_IS16BIT(psm))
		{
			// 16z needs to be a8r8g8b8
			channels = 2;
		}
	}

	return channels;
}

ZeroGS::CMemoryTarget* ZeroGS::CMemoryTargetMngr::MemoryTarget_ClearedTargetsSearch(int fmt, int widthmult, int channels, int height)
{
	CMemoryTarget* targ = NULL;

	if (listClearedTargets.size() > 0)
	{
		list<CMemoryTarget>::iterator itbest = listClearedTargets.begin();

		while(itbest != listClearedTargets.end())
		{
			if ((height <= itbest->realheight) && (itbest->fmt == fmt) && (itbest->widthmult == widthmult) && (itbest->channels == channels))
			{
				// check channels
				int targchannels = NumberOfChannels(itbest->psm);

				if (targchannels == channels) break;
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

	MemoryTarget_GetClutVariables (nClutOffset, clutsize, tex0);
	MemoryTarget_GetMemAddress(start, end, tex0);

	ZeroGS::CMemoryTarget* it = MemoryTarget_SearchExistTarget (start, end, nClutOffset, clutsize, tex0, forcevalidate);
	if (it != NULL) return it;

	// couldn't find so create
	CMemoryTarget* targ;

	u32 fmt = GL_UNSIGNED_BYTE;

	if (PSMT_ISHALF_STORAGE(tex0)) fmt = GL_UNSIGNED_SHORT_1_5_5_5_REV;

	int widthmult = 1;
	if ((g_MaxTexHeight < 4096) && (end-start > g_MaxTexHeight)) widthmult = 2;

	int channels = 1;
	channels = NumberOfChannels(tex0.psm);

	targ = MemoryTarget_ClearedTargetsSearch(fmt, widthmult, channels, end - start);

	// fill local clut
	if (PSMT_ISCLUT(tex0.psm))
	{
		assert( clutsize > 0 );

		targ->cpsm = tex0.cpsm;
		targ->clut.reserve(256*4); // no matter what
		targ->clut.resize(clutsize);

		if (PSMT_IS32BIT(tex0.cpsm))
		{
			memcpy_amd(&targ->clut[0], g_pbyGSClut+nClutOffset, clutsize);
		}
		else
		{
			u16* pClutBuffer = (u16*)(g_pbyGSClut + nClutOffset);
			u16* pclut = (u16*)&targ->clut[0];
			int left = ((u32)nClutOffset & 2) ? 0 : ((nClutOffset&0x3ff)/2)+clutsize-512;
			if( left > 0 ) clutsize -= left;

			while(clutsize > 0)
			{
				pclut[0] = pClutBuffer[0];
				pclut++;
				pClutBuffer+=2;
				clutsize -= 2;
			}

			if( left > 0)
			{
				pClutBuffer = (u16*)(g_pbyGSClut + 2);

				while(left > 0)
				{
					pclut[0] = pClutBuffer[0];
					left -= 2;
					pClutBuffer += 2;
					pclut++;
				}
			}
		}
	}

	if( targ->ptex != NULL )
	{
		assert( end-start <= targ->realheight && targ->fmt == fmt && targ->widthmult == widthmult );

		// good enough, so init
		targ->realy = targ->starty = start;
		targ->usedstamp = curstamp;
		targ->psm = tex0.psm;
		targ->cpsm = tex0.cpsm;
		targ->height = end-start;
	}

	if (targ->ptex == NULL)
	{
		// not initialized yet
		targ->fmt = fmt;
		targ->realy = targ->starty = start;
		targ->realheight = targ->height = end-start;
		targ->usedstamp = curstamp;
		targ->psm = tex0.psm;
		targ->cpsm = tex0.cpsm;
		targ->widthmult = widthmult;
		targ->channels = channels;

		// alloc the mem
		targ->ptex = new CMemoryTarget::TEXTURE();
		targ->ptex->ref = 1;
	}

#ifndef RELEASE_TO_PUBLIC
	g_TransferredToGPU += GPU_TEXWIDTH * channels * 4 * targ->height;
#endif

	// fill with data
	if( targ->ptex->memptr == NULL )
	{
		targ->ptex->memptr = (u8*)_aligned_malloc(4 * GPU_TEXWIDTH * targ->realheight, 16);
		assert(targ->ptex->ref > 0 );
	}

	memcpy_amd(targ->ptex->memptr, g_pbyGSMemory + 4 * GPU_TEXWIDTH * targ->realy, 4 * GPU_TEXWIDTH * targ->height);
	vector<u8> texdata;
	u8* ptexdata = NULL;

	if (PSMT_ISCLUT(tex0.psm))
	{
		texdata.resize( (tex0.cpsm <= 1?4:2) *GPU_TEXWIDTH*channels*widthmult*(targ->realheight+widthmult-1)/widthmult);
		ptexdata = &texdata[0];

		u8* psrc = (u8*)(g_pbyGSMemory + 4 * GPU_TEXWIDTH * targ->realy);

		if (PSMT_IS32BIT(tex0.cpsm))
		{
			u32* pclut = (u32*)&targ->clut[0];
			u32* pdst = (u32*)ptexdata;

			BuildClut<u32>(tex0.psm, targ->height, pclut, psrc, pdst);
		}
		else
		{
			u16* pclut = (u16*)&targ->clut[0];
			u16* pdst = (u16*)ptexdata;

			BuildClut<u16>(tex0.psm, targ->height, pclut, psrc, pdst);
		}
	}
	else
	{
		if (tex0.psm == PSMT16Z || tex0.psm == PSMT16SZ)
		{
			texdata.resize(4*GPU_TEXWIDTH*channels*widthmult*(targ->realheight+widthmult-1)/widthmult
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
			if (((u32)(uptr)dst)%16 != 0)
			{
			// This is not unusual situation, when vector<u8> does not 16bit alignment, that is destructive for SSE2
			// instruction movdqa [%eax], xmm0
			// The idea would be resise vector to 15 elements, that set ptxedata to aligned position.
			// Later we would move eax by 16, so only we should verify is first element align
			// FIXME. As I see, texdata used only once here, it does not have any impact on other code.
			// Probably, usage of _aligned_maloc() would be preferable.
				int disalignment = 16 - ((u32)(uptr)dst)%16 ;		// This is value of shift. It could be 0 < disalignment <= 15
				ptexdata = &texdata[disalignment];			// Set pointer to aligned element
				dst = (u16*)ptexdata;
				GS_LOG("Made alignment for texdata, 0x%x\n", dst );
				assert( ((u32)(uptr)dst)%16 == 0 );			// Assert, because at future could be vectors with uncontigious spaces
			}

			int iters = targ->height*GPU_TEXWIDTH/16;
			SSE2_UnswizzleZ16Target( dst, src, iters ) ;
#else // ZEROGS_SSE2
			for(int i = 0; i < targ->height; ++i)
			{
				for(int j = 0; j < GPU_TEXWIDTH; ++j)
				{
					dst[0] = src[0]; dst[1] = 0;
					dst[2] = src[1]; dst[3] = 0;
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
	glTexImage2D(GL_TEXTURE_RECTANGLE_NV, 0, fmt==GL_UNSIGNED_BYTE?4:GL_RGB5_A1, GPU_TEXWIDTH*channels*widthmult,
		(targ->realheight+widthmult-1)/widthmult, 0, GL_RGBA, fmt, ptexdata);

	int realheight = targ->realheight;
	while(glGetError() != GL_NO_ERROR)
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
				ERROR_LOG("Failed to create %dx%x texture\n", GPU_TEXWIDTH*channels*widthmult, (realheight+widthmult-1)/widthmult);
				channels = 1;
				return NULL;
			}
			DestroyOldest();
		}

		glTexImage2D(GL_TEXTURE_RECTANGLE_NV, 0, 4, GPU_TEXWIDTH*channels*widthmult, (targ->realheight+widthmult-1)/widthmult, 0, GL_RGBA, fmt, ptexdata);
	}

	glTexParameteri(GL_TEXTURE_RECTANGLE_NV, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_RECTANGLE_NV, GL_TEXTURE_WRAP_T, GL_CLAMP);

	assert( tex0.psm != 0xd );
	if (PSMT_ISCLUT(tex0.psm)) assert( targ->clut.size() > 0 );

	return targ;
}

void ZeroGS::CMemoryTargetMngr::ClearRange(int nbStartY, int nbEndY)
{
	FUNCLOG
	int starty = nbStartY / (4*GPU_TEXWIDTH);
	int endy = (nbEndY+4*GPU_TEXWIDTH-1) / (4*GPU_TEXWIDTH);
	//int endy = (nbEndY+4096-1) / 4096;

	//if( listTargets.size() < TARGET_THRESH ) {
		for(list<CMemoryTarget>::iterator it = listTargets.begin(); it != listTargets.end(); ) {

			if( it->starty < endy && (it->starty+it->height) > starty ) {

				// intersects, reduce valid texture mem (or totally delete texture)
				// there are 4 cases
				int miny = max(it->starty, starty);
				int maxy = min(it->starty+it->height, endy);
				assert(miny < maxy);

				if( it->clearmaxy == 0 ) {
					it->clearminy = miny;
					it->clearmaxy = maxy;
				}
				else {
					if( it->clearminy > miny ) it->clearminy = miny;
					if( it->clearmaxy < maxy ) it->clearmaxy = maxy;
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
	for(list<CMemoryTarget>::iterator it = listClearedTargets.begin(); it != listClearedTargets.end(); ) {
		if( it->usedstamp < curstamp - 2 ) {
			it = listClearedTargets.erase(it);
			continue;
		}

		++it;
	}

	if( (curstamp % 3) == 0 ) {
		// purge old targets every 3 frames
		for(list<CMemoryTarget>::iterator it = listTargets.begin(); it != listTargets.end(); ) {
			if( it->usedstamp < curstamp - 3 ) {
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
	if( listTargets.size() == 0 )
		return;

	list<CMemoryTarget>::iterator it, itbest;
	it = itbest = listTargets.begin();

	while(it != listTargets.end()) {
		if( it->usedstamp < itbest->usedstamp )
			itbest = it;
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
	for(map<u32, u32>::iterator it = mapTextures.begin(); it != mapTextures.end(); ++it)
		glDeleteTextures(1, &it->second);
	mapTextures.clear();
}

u32 ZeroGS::CBitwiseTextureMngr::GetTexInt(u32 bitvalue, u32 ptexDoNotDelete)
{
	FUNCLOG
	if( mapTextures.size() > 32 ) {
		// randomly delete 8
		for(map<u32, u32>::iterator it = mapTextures.begin(); it != mapTextures.end();) {
			if( !(rand()&3) && it->second != ptexDoNotDelete) {
				glDeleteTextures(1, &it->second);
				mapTextures.erase(it++);
			}
			else ++it;
		}
	}

	if (glGetError() != GL_NO_ERROR )
		ERROR_LOG ("Error before creation of bitmask texture\n");

	// create a new tex
	u32 ptex;
	glGenTextures(1, &ptex);

	if (glGetError() != GL_NO_ERROR )
		ERROR_LOG ("Error on generation of bitmask texture\n");

	vector<u16> data(GPU_TEXMASKWIDTH+1);
	for(u32 i = 0; i < GPU_TEXMASKWIDTH; ++i)
		data[i] = (((i << MASKDIVISOR) & bitvalue)<<6); // add the 1/2 offset so that
	data[GPU_TEXMASKWIDTH] = 0;

	glBindTexture(GL_TEXTURE_RECTANGLE_NV, ptex);

	if (glGetError() != GL_NO_ERROR )
		ERROR_LOG ("Error on binding bitmask texture\n");

	glTexImage2D(GL_TEXTURE_RECTANGLE_NV, 0, GL_LUMINANCE16, GPU_TEXMASKWIDTH+1, 1, 0, GL_LUMINANCE, GL_UNSIGNED_SHORT, &data[0]);

	if (glGetError() != GL_NO_ERROR )
		ERROR_LOG ("Error on puting bitmask texture\n");

//	Removing clamping, as it seems lead to numerous troubles at some drivers
//	Need to observe, may be clamping is not really needed.
	/*glTexParameteri(GL_TEXTURE_RECTANGLE_NV, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_RECTANGLE_NV, GL_TEXTURE_WRAP_T, GL_REPEAT);

	GLint Error = glGetError();
	if( Error != GL_NO_ERROR ) {
		ERROR_LOG_SPAM_TEST("Failed to create bitmask texture; \t");
		if (SPAM_PASS) {
			ERROR_LOG("bitmask cache %d; \t",  mapTextures.size());
			switch (Error) {
				case GL_INVALID_ENUM: ERROR_LOG("Invalid enumerator\n") ; break;
				case GL_INVALID_VALUE: ERROR_LOG("Invalid value\n"); break;
				case GL_INVALID_OPERATION: ERROR_LOG("Invalid operation\n"); break;
				default: ERROR_LOG("Error number: %d \n", Error);
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
	for(int i = 0; i < (int)ranges.size()-1; ++i)
	{
		assert( ranges[i].end < ranges[i+1].start );
	}
#endif
}

void ZeroGS::CRangeManager::Insert(int start, int end)
{
	FUNCLOG
	int imin = 0, imax = (int)ranges.size(), imid;

	RangeSanityCheck();

	switch(ranges.size())
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
	while(imin < imax)
	{
		imid = (imin+imax)>>1;

		assert( imid < (int)ranges.size() );

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
		assert( start > ranges.back().end );
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
	while(imin < imax)
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

		while(numpop-- > 0 )
		{
			ranges.pop_back();
		}

		assert( start <= ranges.back().end );
		if (start < ranges.back().start) ranges.back().start = start;
		if (lastend > ranges.back().end) ranges.back().end = lastend;
		if (end > ranges.back().end) ranges.back().end = end;
		RangeSanityCheck();
		return;
	}

	if( endindex == 0 )
	{
		assert( end >= ranges.front().start );
		if (start < ranges.front().start) ranges.front().start = start;
		if (end > ranges.front().end) ranges.front().end = end;
		RangeSanityCheck();
	}

	// somewhere in the middle
	if (ranges[startindex].start < start) start = ranges[startindex].start;

	if (startindex < endindex)
	{
		ranges.erase(ranges.begin() + startindex, ranges.begin() + endindex );
	}

	if (start < ranges[startindex].start) ranges[startindex].start = start;
	if (end > ranges[startindex].end) ranges[startindex].end = end;

	RangeSanityCheck();
}

namespace ZeroGS {

CRangeManager s_RangeMngr; // manages overwritten memory
static int gs_imageEnd = 0;

void ResolveInRange(int start, int end)
{
	FUNCLOG
	list<CRenderTarget*> listTargs = CreateTargetsList(start, end);
/*	s_DepthRTs.GetTargs(start, end, listTargs);
	s_RTs.GetTargs(start, end, listTargs);*/
	if( listTargs.size() > 0 ) {
		Flush(0);
		Flush(1);

		// We need another list, because old one could be brocken by Flush().
		listTargs.clear();
		listTargs = CreateTargetsList(start, end);
/*		s_DepthRTs.GetTargs(start, end, listTargs_1);
		s_RTs.GetTargs(start, end, listTargs_1);*/

		for(list<CRenderTarget*>::iterator it = listTargs.begin(); it != listTargs.end(); ++it) {
			// only resolve if not completely covered
			if ((*it)->created == 123 )
				(*it)->Resolve();
			else
				ERROR_LOG("Resolving non-existing object! Destroy code %d\n", (*it)->created);
		}
	}
}

//////////////////
// Transferring //
//////////////////
void FlushTransferRanges(const tex0Info* ptex)
{
	FUNCLOG
	assert( s_RangeMngr.ranges.size() > 0 );
	//bool bHasFlushed = false;
	list<CRenderTarget*> listTransmissionUpdateTargs;

	int texstart = -1, texend = -1;
	if( ptex != NULL ) {
		GetRectMemAddress(texstart, texend, ptex->psm, 0, 0, ptex->tw, ptex->th, ptex->tbp0, ptex->tbw);
	}

	for(vector<CRangeManager::RANGE>::iterator itrange = s_RangeMngr.ranges.begin(); itrange != s_RangeMngr.ranges.end(); ++itrange ) {

		int start = itrange->start;
		int end = itrange->end;

		listTransmissionUpdateTargs.clear();
		listTransmissionUpdateTargs = CreateTargetsList(start, end);

/*		s_DepthRTs.GetTargs(start, end, listTransmissionUpdateTargs);
		s_RTs.GetTargs(start, end, listTransmissionUpdateTargs);*/

//	  if( !bHasFlushed && listTransmissionUpdateTargs.size() > 0 ) {
//		  Flush(0);
//		  Flush(1);
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

		for(list<CRenderTarget*>::iterator it = listTransmissionUpdateTargs.begin(); it != listTransmissionUpdateTargs.end(); ++it) {

			CRenderTarget* ptarg = *it;

			if( (ptarg->status & CRenderTarget::TS_Virtual) )
				continue;

			if( !(ptarg->start < texend && ptarg->end > texstart) ) {
				// chekc if target is currently being used

				if( !(g_GameSettings & GAME_NOQUICKRESOLVE) ) {
					if( ptarg->fbp != vb[0].gsfb.fbp ) {//&& (vb[0].prndr == NULL || ptarg->fbp != vb[0].prndr->fbp) ) {

						if( ptarg->fbp != vb[1].gsfb.fbp ) { //&& (vb[1].prndr == NULL || ptarg->fbp != vb[1].prndr->fbp) ) {
							// this render target currently isn't used and is not in the texture's way, so can safely ignore
							// resolving it. Also the range has to be big enough compared to the target to really call it resolved
							// (ffx changing screens, shadowhearts)
							// start == ptarg->start, used for kh to transfer text
							if( ptarg->IsDepth() || end-start > 0x50000 || ((g_GameSettings&GAME_QUICKRESOLVE1)&&start == ptarg->start) )
								ptarg->status |= CRenderTarget::TS_NeedUpdate|CRenderTarget::TS_Resolved;

							continue;
						}
					}
				}
			}
			else {
//			  if( start <= texstart && end >= texend ) {
//				  // texture taken care of so can skip!?
//				  continue;
//			  }
			}

			// the first range check was very rough; some games (dragonball z) have the zbuf in the same page as textures (but not overlapping)
			// so detect that condition
			if( ptarg->fbh % m_Blocks[ptarg->psm].height ) {

				// get start of left-most boundry page
				int targstart, targend;
				ZeroGS::GetRectMemAddress(targstart, targend, ptarg->psm, 0, 0, ptarg->fbw, ptarg->fbh & ~(m_Blocks[ptarg->psm].height-1), ptarg->fbp, ptarg->fbw);

				if( start >= targend ) {

					// don't bother
					if( (ptarg->fbh % m_Blocks[ptarg->psm].height) <= 2 )
						continue;

					// calc how many bytes of the block that the page spans
				}
			}

			if( !(ptarg->status & CRenderTarget::TS_Virtual) ) {

				if( start < ptarg->end && end > ptarg->start ) {

					// suikoden5 is faster with check, but too big of a value and kh screens mess up
					/* Zeydlitz remove this check, it does not do anything good
					if ((end - start > 0x8000) && (!(g_GameSettings & GAME_GUSTHACK) || (end-start > 0x40000))) {
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
					if( (!ptarg->IsDepth() || (!(g_GameSettings & GAME_NODEPTHUPDATE) || end-start > 0x1000)) && ((end-start > 0x40000) || !(g_GameSettings & GAME_GUSTHACK)))
						ptarg->status |= CRenderTarget::TS_NeedUpdate;
				}
			}
		}

		ZeroGS::g_MemTargs.ClearRange(start, end);
	}

	s_RangeMngr.Clear();
}

static vector<u8> s_vTempBuffer, s_vTransferCache;

void InitTransferHostLocal()
{
	FUNCLOG
	if( g_bIsLost )
		return;

#ifndef RELEASE_TO_PUBLIC
	if( gs.trxpos.dx+gs.imageWnew > gs.dstbuf.bw )
		WARN_LOG("Transfer error, width exceeds\n");
#endif

	//bool bHasFlushed = false;

	gs.imageX = gs.trxpos.dx;
	gs.imageY = gs.trxpos.dy;
	gs.imageEndX = gs.imageX + gs.imageWnew;
	gs.imageEndY = gs.imageY + gs.imageHnew;

	assert( gs.imageEndX < 2048 && gs.imageEndY < 2048 );

	// hack! viewful joe
	if( gs.dstbuf.psm == 63 )
		gs.dstbuf.psm = 0;

	int start, end;
	GetRectMemAddress(start, end, gs.dstbuf.psm, gs.trxpos.dx, gs.trxpos.dy, gs.imageWnew, gs.imageHnew, gs.dstbuf.bp, gs.dstbuf.bw);

	if( end > 0x00400000 ) {
		WARN_LOG("host local out of bounds!\n");
		//gs.imageTransfer = -1;
		end = 0x00400000;
	}

	gs_imageEnd = end;

	if( vb[0].nCount > 0 )
		Flush(0);
	if( vb[1].nCount > 0 )
		Flush(1);

	//PRIM_LOG("trans: bp:%x x:%x y:%x w:%x h:%x\n", gs.dstbuf.bp, gs.trxpos.dx, gs.trxpos.dy, gs.imageWnew, gs.imageHnew);

//  if( !bHasFlushed && (vb[0].bNeedFrameCheck || vb[0].bNeedZCheck || vb[1].bNeedFrameCheck || vb[1].bNeedZCheck)) {
//	  Flush(0);
//	  Flush(1);
//	  bHasFlushed = 1;
//  }
//
//  // for all ranges, flush the targets
//  // check if new rect intersects with current rendering texture, if so, flush
//  if( vb[0].nCount > 0 && vb[0].curprim.tme ) {
//	  int tstart, tend;
//	  GetRectMemAddress(tstart, tend, vb[0].tex0.psm, 0, 0, vb[0].tex0.tw, vb[0].tex0.th, vb[0].tex0.tbp0, vb[0].tex0.tbw);
//
//	  if( start < tend && end > tstart ) {
//		  Flush(0);
//		  Flush(1);
//		  bHasFlushed = 1;
//	  }
//  }
//
//  if( !bHasFlushed && vb[1].nCount > 0 && vb[1].curprim.tme ) {
//	  int tstart, tend;
//	  GetRectMemAddress(tstart, tend, vb[1].tex0.psm, 0, 0, vb[1].tex0.tw, vb[1].tex0.th, vb[1].tex0.tbp0, vb[1].tex0.tbw);
//
//	  if( start < tend && end > tstart ) {
//		  Flush(0);
//		  Flush(1);
//		  bHasFlushed = 1;
//	  }
//  }

	//ZeroGS::g_MemTargs.ClearRange(start, end);
	//s_RangeMngr.Insert(start, end);
}

void TransferHostLocal(const void* pbyMem, u32 nQWordSize)
{
	FUNCLOG
	if( g_bIsLost )
		return;

	int start, end;
	GetRectMemAddress(start, end, gs.dstbuf.psm, gs.imageX, gs.imageY, gs.imageWnew, gs.imageHnew, gs.dstbuf.bp, gs.dstbuf.bw);
	assert( start < gs_imageEnd );

	end = gs_imageEnd;

	// sometimes games can decompress to alpha channel of render target only, in this case
	// do a resolve right away. wolverine x2
	if ((gs.dstbuf.psm == PSMT8H || gs.dstbuf.psm == PSMT4HL || gs.dstbuf.psm == PSMT4HH) && !(g_GameSettings & GAME_GUSTHACK)) {
		list<CRenderTarget*> listTransmissionUpdateTargs;
		s_RTs.GetTargs(start, end, listTransmissionUpdateTargs);

		for(list<CRenderTarget*>::iterator it = listTransmissionUpdateTargs.begin(); it != listTransmissionUpdateTargs.end(); ++it) {

			CRenderTarget* ptarg = *it;

			if( (ptarg->status & CRenderTarget::TS_Virtual) )
				continue;

			//ERROR_LOG("resolving to alpha channel\n");
			ptarg->Resolve();
		}
	}

	s_RangeMngr.Insert(start, min(end, start+(int)nQWordSize*16));

	const u8* porgend = (const u8*)pbyMem + 4 * nQWordSize;

	if( s_vTransferCache.size() > 0 ) {

		int imagecache = s_vTransferCache.size();
		s_vTempBuffer.resize(imagecache + nQWordSize*4);
		memcpy(&s_vTempBuffer[0], &s_vTransferCache[0], imagecache);
		memcpy(&s_vTempBuffer[imagecache], pbyMem, nQWordSize*4);

		pbyMem = (const void*)&s_vTempBuffer[0];
		porgend = &s_vTempBuffer[0]+s_vTempBuffer.size();

		int wordinc = imagecache / 4;
		if( (nQWordSize * 4 + imagecache)/3 == ((nQWordSize+wordinc) * 4) / 3 ) {
			// can use the data
			nQWordSize += wordinc;
		}
	}

	int leftover = m_Blocks[gs.dstbuf.psm].TransferHostLocal(pbyMem, nQWordSize);

	if( leftover > 0 ) {
		// copy the last gs.image24bitOffset to the cache
		s_vTransferCache.resize(leftover);
		memcpy(&s_vTransferCache[0], porgend - leftover, leftover);
	}
	else s_vTransferCache.resize(0);

#if !defined(RELEASE_TO_PUBLIC) && defined(_DEBUG)
	if( g_bSaveTrans ) {
		tex0Info t;
		t.tbp0 = gs.dstbuf.bp;
		t.tw = gs.imageWnew;
		t.th = gs.imageHnew;
		t.tbw = gs.dstbuf.bw;
		t.psm = gs.dstbuf.psm;
		SaveTex(&t, 0);
	}
#endif
}

#if 0
// left/right, top/down
//void TransferHostLocal(const void* pbyMem, u32 nQWordSize)
//{
//  assert( gs.imageTransfer == 0 );
//  u8* pstart = g_pbyGSMemory + gs.dstbuf.bp*256;
//
//  const u8* pendbuf = (const u8*)pbyMem + nQWordSize*4;
//  int i = gs.imageY, j = gs.imageX;
//
//#define DSTPSM gs.dstbuf.psm
//
//#define TRANSFERHOSTLOCAL(psm, T, widthlimit) { \
//  const T* pbuf = (const T*)pbyMem; \
//  u32 nSize = nQWordSize*(4/sizeof(T)); \
//  assert( (nSize%widthlimit) == 0 && widthlimit <= 4 ); \
//  if( ((gs.imageEndX-gs.trxpos.dx)%widthlimit) ) ERROR_LOG("Bad Transmission! %d %d, psm: %d\n", gs.trxpos.dx, gs.imageEndX, DSTPSM); \
//  for(; i < gs.imageEndY; ++i) { \
//	  for(; j < gs.imageEndX && nSize > 0; j += widthlimit, nSize -= widthlimit, pbuf += widthlimit) { \
//		  /* write as many pixel at one time as possible */ \
//		  writePixel##psm##_0(pstart, j%2048, i%2048, pbuf[0], gs.dstbuf.bw); \
//		  \
//		  if( widthlimit > 1 ) { \
//			  writePixel##psm##_0(pstart, (j+1)%2048, i%2048, pbuf[1], gs.dstbuf.bw); \
//			  \
//			  if( widthlimit > 2 ) { \
//				  writePixel##psm##_0(pstart, (j+2)%2048, i%2048, pbuf[2], gs.dstbuf.bw); \
//				  \
//				  if( widthlimit > 3 ) { \
//					  writePixel##psm##_0(pstart, (j+3)%2048, i%2048, pbuf[3], gs.dstbuf.bw); \
//				  } \
//			  } \
//		  } \
//	  } \
//	  \
//	  if( j >= gs.imageEndX ) { assert(j == gs.imageEndX); j = gs.trxpos.dx; } \
//	  else { assert( nSize == 0 ); goto End; } \
//  } \
//} \
//
//#define TRANSFERHOSTLOCAL_4(psm) { \
//  const u8* pbuf = (const u8*)pbyMem; \
//  u32 nSize = nQWordSize*8; \
//  for(; i < gs.imageEndY; ++i) { \
//	  for(; j < gs.imageEndX && nSize > 0; j += 8, nSize -= 8) { \
//		  /* write as many pixel at one time as possible */ \
//		  writePixel##psm##_0(pstart, j%2048, i%2048, *pbuf&0x0f, gs.dstbuf.bw); \
//		  writePixel##psm##_0(pstart, (j+1)%2048, i%2048, *pbuf>>4, gs.dstbuf.bw); \
//		  pbuf++; \
//		  writePixel##psm##_0(pstart, (j+2)%2048, i%2048, *pbuf&0x0f, gs.dstbuf.bw); \
//		  writePixel##psm##_0(pstart, (j+3)%2048, i%2048, *pbuf>>4, gs.dstbuf.bw); \
//		  pbuf++; \
//		  writePixel##psm##_0(pstart, (j+4)%2048, i%2048, *pbuf&0x0f, gs.dstbuf.bw); \
//		  writePixel##psm##_0(pstart, (j+5)%2048, i%2048, *pbuf>>4, gs.dstbuf.bw); \
//		  pbuf++; \
//		  writePixel##psm##_0(pstart, (j+6)%2048, i%2048, *pbuf&0x0f, gs.dstbuf.bw); \
//		  writePixel##psm##_0(pstart, (j+7)%2048, i%2048, *pbuf>>4, gs.dstbuf.bw); \
//		  pbuf++; \
//	  } \
//	  \
//	  if( j >= gs.imageEndX ) { /*assert(j == gs.imageEndX);*/ j = gs.trxpos.dx; } \
//	  else { assert( nSize == 0 ); goto End; } \
//  } \
//} \
//
//  switch (gs.dstbuf.psm) {
//	  case 0x0:  TRANSFERHOSTLOCAL(32, u32, 2); break;
//	  case 0x1:  TRANSFERHOSTLOCAL(24, u32, 4); break;
//	  case 0x2:  TRANSFERHOSTLOCAL(16, u16, 4); break;
//	  case 0xA:  TRANSFERHOSTLOCAL(16S, u16, 4); break;
//	  case 0x13:
//		  if( ((gs.imageEndX-gs.trxpos.dx)%4) ) {
//			  TRANSFERHOSTLOCAL(8, u8, 1);
//		  }
//		  else {
//			  TRANSFERHOSTLOCAL(8, u8, 4);
//		  }
//		  break;
//
//	  case 0x14:
////			if( (gs.imageEndX-gs.trxpos.dx)%8 ) {
////				// hack
////				if( abs((int)nQWordSize*8 - (gs.imageEndY-i)*(gs.imageEndX-gs.trxpos.dx)+(j-gs.trxpos.dx)) <= 8 ) {
////					// don't transfer
////					ERROR_LOG("bad texture 4: %d %d %d\n", gs.trxpos.dx, gs.imageEndX, nQWordSize);
////					gs.imageEndX = gs.trxpos.dx + (gs.imageEndX-gs.trxpos.dx)&~7;
////					//i = gs.imageEndY;
////					//goto End;
////					gs.imageTransfer = -1;
////				}
////			}
//		  TRANSFERHOSTLOCAL_4(4);
//		  break;
//	  case 0x1B: TRANSFERHOSTLOCAL(8H, u8, 4); break;
//	  case 0x24: TRANSFERHOSTLOCAL_4(4HL); break;
//	  case 0x2C: TRANSFERHOSTLOCAL_4(4HH); break;
//	  case 0x30: TRANSFERHOSTLOCAL(32Z, u32, 2); break;
//	  case 0x31: TRANSFERHOSTLOCAL(24Z, u32, 4); break;
//	  case 0x32: TRANSFERHOSTLOCAL(16Z, u16, 4); break;
//	  case 0x3A: TRANSFERHOSTLOCAL(16SZ, u16, 4); break;
//  }
//
//End:
//  if( i >= gs.imageEndY ) {
//	  assert( i == gs.imageEndY );
//	  gs.imageTransfer = -1;
//
//	  if( g_bSaveTrans ) {
//		  tex0Info t;
//		  t.tbp0 = gs.dstbuf.bp;
//		  t.tw = gs.imageWnew;
//		  t.th = gs.imageHnew;
//		  t.tbw = gs.dstbuf.bw;
//		  t.psm = gs.dstbuf.psm;
//		  SaveTex(&t, 0);
//	  }
//  }
//  else {
//	  /* update new params */
//	  gs.imageY = i;
//	  gs.imageX = j;
//  }
//}
#endif //if 0

void InitTransferLocalHost()
{
	FUNCLOG
	assert( gs.trxpos.sx+gs.imageWnew <= 2048 && gs.trxpos.sy+gs.imageHnew <= 2048 );

#ifndef RELEASE_TO_PUBLIC
	if( gs.trxpos.sx+gs.imageWnew > gs.srcbuf.bw )
		WARN_LOG("Transfer error, width exceeds\n");
#endif

	gs.imageX = gs.trxpos.sx;
	gs.imageY = gs.trxpos.sy;
	gs.imageEndX = gs.imageX + gs.imageWnew;
	gs.imageEndY = gs.imageY + gs.imageHnew;
	s_vTransferCache.resize(0);

	int start, end;
	GetRectMemAddress(start, end, gs.srcbuf.psm, gs.trxpos.sx, gs.trxpos.sy, gs.imageWnew, gs.imageHnew, gs.srcbuf.bp, gs.srcbuf.bw);
	ResolveInRange(start, end);
}

template <class T>
void TransferLocalHost(void* pbyMem, u32 nQWordSize, int& x, int& y, u8 *pstart, _readPixel_0 rp)
{
	int i = x, j = y;
	T* pbuf = (T*)pbyMem;
	u32 nSize = nQWordSize*16/sizeof(T);

	for(; i < gs.imageEndY; ++i)
	{
		for(; j < gs.imageEndX && nSize > 0; ++j, --nSize)
		{
			*pbuf++ = rp(pstart, j%2048, i%2048, gs.srcbuf.bw);
		}

		if( j >= gs.imageEndX )
		{
			assert( j == gs.imageEndX);
			j = gs.trxpos.sx;
		}
		else
		{
			assert( nSize == 0 );
			break;
		}
	}

}

void TransferLocalHost_24(void* pbyMem, u32 nQWordSize, int& x, int& y, u8 *pstart, _readPixel_0 rp)
{
	int i = x, j = y;
	u8* pbuf = (u8*)pbyMem;
	u32 nSize = nQWordSize*16/3;

	for(; i < gs.imageEndY; ++i)
	{
		for(; j < gs.imageEndX && nSize > 0; ++j, --nSize)
		{
			u32 p = rp(pstart, j%2048, i%2048, gs.srcbuf.bw);
			pbuf[0] = (u8)p;
			pbuf[1] = (u8)(p>>8);
			pbuf[2] = (u8)(p>>16);
			pbuf += 3;
		}

		if( j >= gs.imageEndX )
			{
				assert( j == gs.imageEndX);
				j = gs.trxpos.sx;
			}
		else
			{
				assert( nSize == 0 );
				break;
			}
	}
}

// left/right, top/down
void TransferLocalHost(void* pbyMem, u32 nQWordSize)
{
	FUNCLOG
	assert( gs.imageTransfer == 1 );

	u8* pstart = g_pbyGSMemory + 256*gs.srcbuf.bp;
	int i = gs.imageY, j = gs.imageX;

	switch (gs.srcbuf.psm) {
		case PSMCT32:  TransferLocalHost<u32>(pbyMem, nQWordSize, i, j, pstart, readPixel32_0); break;
		case PSMCT24:  TransferLocalHost_24(pbyMem, nQWordSize, i, j, pstart, readPixel24_0); break;
		case PSMCT16:  TransferLocalHost<u16>(pbyMem, nQWordSize, i, j, pstart, readPixel16_0); break;
		case PSMCT16S: TransferLocalHost<u16>(pbyMem, nQWordSize, i, j, pstart, readPixel16S_0); break;
		case PSMT8: TransferLocalHost<u8>(pbyMem, nQWordSize, i, j, pstart, readPixel8_0); break;
		case PSMT8H: TransferLocalHost<u8>(pbyMem, nQWordSize, i, j, pstart, readPixel8H_0); break;
		case PSMT32Z: TransferLocalHost<u32>(pbyMem, nQWordSize, i, j, pstart, readPixel32Z_0); break;
		case PSMT24Z: TransferLocalHost_24(pbyMem, nQWordSize, i, j, pstart, readPixel24Z_0); break;
		case PSMT16Z: TransferLocalHost<u16>(pbyMem, nQWordSize, i, j, pstart, readPixel16Z_0); break;
		case PSMT16SZ: TransferLocalHost<u16>(pbyMem, nQWordSize, i, j, pstart, readPixel16SZ_0); break;
		default: assert(0);
	}

	gs.imageY = i;
	gs.imageX = j;

	if( gs.imageY >= gs.imageEndY ) {
		assert( gs.imageY == gs.imageEndY );
		gs.imageTransfer = -1;
	}
}

// dir depends on trxpos.dir
void TransferLocalLocal()
{
	FUNCLOG
	assert( gs.imageTransfer == 2 );
	assert( gs.trxpos.sx+gs.imageWnew < 2048 && gs.trxpos.sy+gs.imageHnew < 2048 );
	assert( gs.trxpos.dx+gs.imageWnew < 2048 && gs.trxpos.dy+gs.imageHnew < 2048 );
	assert( (gs.srcbuf.psm&0x7) == (gs.dstbuf.psm&0x7) );
	if( gs.trxpos.sx+gs.imageWnew > gs.srcbuf.bw )
		WARN_LOG("Transfer error, src width exceeds\n");
	if( gs.trxpos.dx+gs.imageWnew > gs.dstbuf.bw )
		WARN_LOG("Transfer error, dst width exceeds\n");

	int srcstart, srcend, dststart, dstend;

	GetRectMemAddress(srcstart, srcend, gs.srcbuf.psm, gs.trxpos.sx, gs.trxpos.sy, gs.imageWnew, gs.imageHnew, gs.srcbuf.bp, gs.srcbuf.bw);
	GetRectMemAddress(dststart, dstend, gs.dstbuf.psm, gs.trxpos.dx, gs.trxpos.dy, gs.imageWnew, gs.imageHnew, gs.dstbuf.bp, gs.dstbuf.bw);

	// resolve the targs
	ResolveInRange(srcstart, srcend);

	list<CRenderTarget*> listTargs;
	s_RTs.GetTargs(dststart, dstend, listTargs);
	for(list<CRenderTarget*>::iterator it = listTargs.begin(); it != listTargs.end(); ++it) {
		if( !((*it)->status & CRenderTarget::TS_Virtual) ) {
			(*it)->Resolve();
			//(*it)->status |= CRenderTarget::TS_NeedUpdate;
		}
	}

	u8* pSrcBuf = g_pbyGSMemory + gs.srcbuf.bp*256;
	u8* pDstBuf = g_pbyGSMemory + gs.dstbuf.bp*256;

#define TRANSFERLOCALLOCAL(srcpsm, dstpsm, widthlimit) { \
	if( (gs.imageWnew&widthlimit)!=0 ) break; \
	assert( (gs.imageWnew&widthlimit)==0 && widthlimit <= 4); \
	for(int i = gs.trxpos.sy, i2 = gs.trxpos.dy; i < gs.trxpos.sy+gs.imageHnew; i++, i2++) { \
		for(int j = gs.trxpos.sx, j2 = gs.trxpos.dx; j < gs.trxpos.sx+gs.imageWnew; j+=widthlimit, j2+=widthlimit) { \
			\
			writePixel##dstpsm##_0(pDstBuf, j2%2048, i2%2048, \
				readPixel##srcpsm##_0(pSrcBuf, j%2048, i%2048, gs.srcbuf.bw), gs.dstbuf.bw); \
			\
			if( widthlimit > 1 ) { \
				writePixel##dstpsm##_0(pDstBuf, (j2+1)%2048, i2%2048, \
					readPixel##srcpsm##_0(pSrcBuf, (j+1)%2048, i%2048, gs.srcbuf.bw), gs.dstbuf.bw); \
				\
				if( widthlimit > 2 ) { \
					writePixel##dstpsm##_0(pDstBuf, (j2+2)%2048, i2%2048, \
						readPixel##srcpsm##_0(pSrcBuf, (j+2)%2048, i%2048, gs.srcbuf.bw), gs.dstbuf.bw); \
					\
					if( widthlimit > 3 ) { \
						writePixel##dstpsm##_0(pDstBuf, (j2+3)%2048, i2%2048, \
							readPixel##srcpsm##_0(pSrcBuf, (j+3)%2048, i%2048, gs.srcbuf.bw), gs.dstbuf.bw); \
					} \
				} \
			} \
		} \
	} \
} \

#define TRANSFERLOCALLOCAL_4(srcpsm, dstpsm) { \
	assert( (gs.imageWnew%8) == 0 ); \
	for(int i = gs.trxpos.sy, i2 = gs.trxpos.dy; i < gs.trxpos.sy+gs.imageHnew; ++i, ++i2) { \
		for(int j = gs.trxpos.sx, j2 = gs.trxpos.dx; j < gs.trxpos.sx+gs.imageWnew; j+=8, j2+=8) { \
			/* NOTE: the 2 conseq 4bit values are in NOT in the same byte */ \
			u32 read = getPixelAddress##srcpsm##_0(j%2048, i%2048, gs.srcbuf.bw); \
			u32 write = getPixelAddress##dstpsm##_0(j2%2048, i2%2048, gs.dstbuf.bw); \
			pDstBuf[write] = (pDstBuf[write]&0xf0)|(pSrcBuf[read]&0x0f); \
 \
			read = getPixelAddress##srcpsm##_0((j+1)%2048, i%2048, gs.srcbuf.bw); \
			write = getPixelAddress##dstpsm##_0((j2+1)%2048, i2%2048, gs.dstbuf.bw); \
			pDstBuf[write] = (pDstBuf[write]&0x0f)|(pSrcBuf[read]&0xf0); \
 \
			read = getPixelAddress##srcpsm##_0((j+2)%2048, i%2048, gs.srcbuf.bw); \
			write = getPixelAddress##dstpsm##_0((j2+2)%2048, i2%2048, gs.dstbuf.bw); \
			pDstBuf[write] = (pDstBuf[write]&0xf0)|(pSrcBuf[read]&0x0f); \
 \
			read = getPixelAddress##srcpsm##_0((j+3)%2048, i%2048, gs.srcbuf.bw); \
			write = getPixelAddress##dstpsm##_0((j2+3)%2048, i2%2048, gs.dstbuf.bw); \
			pDstBuf[write] = (pDstBuf[write]&0x0f)|(pSrcBuf[read]&0xf0); \
 \
			read = getPixelAddress##srcpsm##_0((j+2)%2048, i%2048, gs.srcbuf.bw); \
			write = getPixelAddress##dstpsm##_0((j2+2)%2048, i2%2048, gs.dstbuf.bw); \
			pDstBuf[write] = (pDstBuf[write]&0xf0)|(pSrcBuf[read]&0x0f); \
 \
			read = getPixelAddress##srcpsm##_0((j+3)%2048, i%2048, gs.srcbuf.bw); \
			write = getPixelAddress##dstpsm##_0((j2+3)%2048, i2%2048, gs.dstbuf.bw); \
			pDstBuf[write] = (pDstBuf[write]&0x0f)|(pSrcBuf[read]&0xf0); \
 \
			read = getPixelAddress##srcpsm##_0((j+2)%2048, i%2048, gs.srcbuf.bw); \
			write = getPixelAddress##dstpsm##_0((j2+2)%2048, i2%2048, gs.dstbuf.bw); \
			pDstBuf[write] = (pDstBuf[write]&0xf0)|(pSrcBuf[read]&0x0f); \
 \
			read = getPixelAddress##srcpsm##_0((j+3)%2048, i%2048, gs.srcbuf.bw); \
			write = getPixelAddress##dstpsm##_0((j2+3)%2048, i2%2048, gs.dstbuf.bw); \
			pDstBuf[write] = (pDstBuf[write]&0x0f)|(pSrcBuf[read]&0xf0); \
		} \
	} \
} \

	switch (gs.srcbuf.psm) {
		case PSMCT32:
			if( gs.dstbuf.psm == PSMCT32 ) {
				TRANSFERLOCALLOCAL(32, 32, 2);
			}
			else {
				TRANSFERLOCALLOCAL(32, 32Z, 2);
			}
			break;

		case PSMCT24:
			if( gs.dstbuf.psm == PSMCT24 ) {
				TRANSFERLOCALLOCAL(24, 24, 4);
			}
			else {
				TRANSFERLOCALLOCAL(24, 24Z, 4);
			}
			break;

		case PSMCT16:
			switch(gs.dstbuf.psm) {
				case PSMCT16: TRANSFERLOCALLOCAL(16, 16, 4); break;
				case PSMCT16S: TRANSFERLOCALLOCAL(16, 16S, 4); break;
				case PSMT16Z: TRANSFERLOCALLOCAL(16, 16Z, 4); break;
				case PSMT16SZ: TRANSFERLOCALLOCAL(16, 16SZ, 4); break;
			}
			break;

		case PSMCT16S:
			switch(gs.dstbuf.psm) {
				case PSMCT16: TRANSFERLOCALLOCAL(16S, 16, 4); break;
				case PSMCT16S: TRANSFERLOCALLOCAL(16S, 16S, 4); break;
				case PSMT16Z: TRANSFERLOCALLOCAL(16S, 16Z, 4); break;
				case PSMT16SZ: TRANSFERLOCALLOCAL(16S, 16SZ, 4); break;
			}
			break;

		case PSMT8:
			if( gs.dstbuf.psm == PSMT8 ) {
				TRANSFERLOCALLOCAL(8, 8, 4);
			}
			else {
				TRANSFERLOCALLOCAL(8, 8H, 4);
			}
			break;

		case PSMT4:

			switch(gs.dstbuf.psm ) {
				case PSMT4: TRANSFERLOCALLOCAL_4(4, 4); break;
				case PSMT4HL: TRANSFERLOCALLOCAL_4(4, 4HL); break;
				case PSMT4HH: TRANSFERLOCALLOCAL_4(4, 4HH); break;
			}
			break;

		case PSMT8H:
			if( gs.dstbuf.psm == PSMT8 ) {
				TRANSFERLOCALLOCAL(8H, 8, 4);
			}
			else {
				TRANSFERLOCALLOCAL(8H, 8H, 4);
			}
			break;

		case PSMT4HL:
			switch(gs.dstbuf.psm ) {
				case PSMT4: TRANSFERLOCALLOCAL_4(4HL, 4); break;
				case PSMT4HL: TRANSFERLOCALLOCAL_4(4HL, 4HL); break;
				case PSMT4HH: TRANSFERLOCALLOCAL_4(4HL, 4HH); break;
			}
			break;
		case PSMT4HH:
			switch(gs.dstbuf.psm ) {
				case PSMT4: TRANSFERLOCALLOCAL_4(4HH, 4); break;
				case PSMT4HL: TRANSFERLOCALLOCAL_4(4HH, 4HL); break;
				case PSMT4HH: TRANSFERLOCALLOCAL_4(4HH, 4HH); break;
			}
			break;

		case PSMT32Z:
			if( gs.dstbuf.psm == PSMCT32 ) {
				TRANSFERLOCALLOCAL(32Z, 32, 2);
			}
			else {
				TRANSFERLOCALLOCAL(32Z, 32Z, 2);
			}
			break;

		case PSMT24Z:
			if( gs.dstbuf.psm == PSMCT24 ) {
				TRANSFERLOCALLOCAL(24Z, 24, 4);
			}
			else {
				TRANSFERLOCALLOCAL(24Z, 24Z, 4);
			}
			break;

		case PSMT16Z:
			switch(gs.dstbuf.psm) {
				case PSMCT16: TRANSFERLOCALLOCAL(16Z, 16, 4); break;
				case PSMCT16S: TRANSFERLOCALLOCAL(16Z, 16S, 4); break;
				case PSMT16Z: TRANSFERLOCALLOCAL(16Z, 16Z, 4); break;
				case PSMT16SZ: TRANSFERLOCALLOCAL(16Z, 16SZ, 4); break;
			}
			break;

		case PSMT16SZ:
			switch(gs.dstbuf.psm) {
				case PSMCT16: TRANSFERLOCALLOCAL(16SZ, 16, 4); break;
				case PSMCT16S: TRANSFERLOCALLOCAL(16SZ, 16S, 4); break;
				case PSMT16Z: TRANSFERLOCALLOCAL(16SZ, 16Z, 4); break;
				case PSMT16SZ: TRANSFERLOCALLOCAL(16SZ, 16SZ, 4); break;
			}
			break;
	}

	g_MemTargs.ClearRange(dststart, dstend);

#ifdef DEVBUILD
	if( g_bSaveTrans ) {
		tex0Info t;
		t.tbp0 = gs.dstbuf.bp;
		t.tw = gs.imageWnew;
		t.th = gs.imageHnew;
		t.tbw = gs.dstbuf.bw;
		t.psm = gs.dstbuf.psm;
		SaveTex(&t, 0);

		t.tbp0 = gs.srcbuf.bp;
		t.tw = gs.imageWnew;
		t.th = gs.imageHnew;
		t.tbw = gs.srcbuf.bw;
		t.psm = gs.srcbuf.psm;
		SaveTex(&t, 0);
	}
#endif
}

void GetRectMemAddress(int& start, int& end, int psm, int x, int y, int w, int h, int bp, int bw)
{
	FUNCLOG
	if( m_Blocks[psm].bpp == 0 ) {
		ERROR_LOG("ZeroGS: Bad psm 0x%x\n", psm);
		start = 0;
		end = 0x00400000;
		return;
	}

	if( PSMT_ISZTEX(psm) || psm == PSMCT16S ) {

		const BLOCK& b = m_Blocks[psm];

		bw = (bw + b.width - 1)/b.width;
		start = bp*256 + ((y/b.height) * bw + (x/b.width) )*0x2000;
		end = bp*256  + (((y+h-1)/b.height) * bw + (x + w + b.width - 1)/b.width)*0x2000;
	}
	else {
		// just take the addresses
		switch(psm) {
			case PSMCT32:
			case PSMCT24:
			case PSMT8H:
			case PSMT4HL:
			case PSMT4HH:
				start = 4*getPixelAddress32(x, y, bp, bw);
				end = 4*getPixelAddress32(x+w-1, y+h-1, bp, bw) + 4;
				break;
			case PSMCT16:
				start = 2*getPixelAddress16(x, y, bp, bw);
				end = 2*getPixelAddress16(x+w-1, y+h-1, bp, bw)+2;
				break;
			case PSMT8:
				start = getPixelAddress8(x, y, bp, bw);
				end = getPixelAddress8(x+w-1, y+h-1, bp, bw)+1;
				break;
			case PSMT4:
			{
				start = getPixelAddress4(x, y, bp, bw)/2;
				int newx = ((x+w-1+31)&~31)-1;
				int newy = ((y+h-1+15)&~15)-1;
				end = (getPixelAddress4(max(newx,x), max(newy,y), bp, bw)+2)/2;
				break;
			}
		}
	}
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
		int maxfbh = (0x00400000-fbp*256) / (sizeof(T) * fbw); \
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

void _Resolve(const void* psrc, int fbp, int fbw, int fbh, int psm, u32 fbm, bool mode = true)
{
	FUNCLOG

	int start, end;

	s_nResolved += 2;

	// align the rect to the nearest page
	// note that fbp is always aligned on page boundaries
	GetRectMemAddress(start, end, psm, 0, 0, fbw, fbh, fbp, fbw);

	if (GetRenderFormat() == RFT_byte8)
	{
		// start the conversion process A8R8G8B8 -> psm
		switch(psm)
		{
			case PSMCT32:
			case PSMCT24:
				RESOLVE_32_BIT(32, u32, u32, (u32));
				break;

			case PSMCT16:
				RESOLVE_32_BIT(16, u16, u32, RGBA32to16);
				break;

			case PSMCT16S:
				RESOLVE_32_BIT(16S, u16, u32, RGBA32to16);
				break;

			case PSMT32Z:
			case PSMT24Z:
				RESOLVE_32_BIT(32Z, u32, u32, (u32));
				break;

			case PSMT16Z:
				RESOLVE_32_BIT(16Z, u16, u32, (u16));
				break;

			case PSMT16SZ:
				RESOLVE_32_BIT(16SZ, u16, u32, (u16));
				break;
		}
	}
	else  // float16
	{
		switch(psm)
		{
			case PSMCT32:
			case PSMCT24:
				RESOLVE_32_BIT(32, u32, Vector_16F, Float16ToARGB);
				break;

			case PSMCT16:
				RESOLVE_32_BIT(16, u16, Vector_16F, Float16ToARGB16);
				break;

			case PSMCT16S:
				RESOLVE_32_BIT(16S, u16, Vector_16F, Float16ToARGB16);
				break;

			case PSMT32Z:
			case PSMT24Z:
				RESOLVE_32_BIT(32Z, u32, Vector_16F,  Float16ToARGB_Z);
				break;

			case PSMT16Z:
				RESOLVE_32_BIT(16Z, u16, Vector_16F, Float16ToARGB16_Z);
				break;

			case PSMT16SZ:
				RESOLVE_32_BIT(16SZ, u16, Vector_16F, Float16ToARGB16_Z);
				break;
		}
	}

	g_MemTargs.ClearRange(start, end);
	INC_RESOLVE();
}

// Leaving this code in for reference for the moment.
#if 0
void _Resolve(const void* psrc, int fbp, int fbw, int fbh, int psm, u32 fbm, bool mode)
{
	FUNCLOG
	//assert( glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT) == GL_FRAMEBUFFER_COMPLETE_EXT );
	s_nResolved += 2;

	// align the rect to the nearest page
	// note that fbp is always aligned on page boundaries
	int start, end;
	GetRectMemAddress(start, end, psm, 0, 0, fbw, fbh, fbp, fbw);

	int i, j;
	//short smask1 = gs.smask&1;
	//short smask2 = gs.smask&2;
	u32 mask, imask;

	if (PSMT_ISHALF(psm)) { // 16 bit
		// mask is shifted
		imask = RGBA32to16(fbm);
		mask = (~imask)&0xffff;
	}
	else {
		mask = ~fbm;
		imask = fbm;

		if( (psm&0xf)>0  && 0) {
			// preserve the alpha?
			mask &= 0x00ffffff;
			imask |= 0xff000000;
		}
	}

	// Targets over 2000 should be shuffle. FFX and KH2 (0x2100)
	int X = (psm == 0) ? 0 : 0;
//if (X == 1)
//ERROR_LOG("resolve: %x %x %x %x (%x-%x)\n", psm, fbp, fbw, fbh, start, end);


#define RESOLVE_32BIT(psm, T, Tsrc, blockbits, blockwidth, blockheight, convfn, frame, aax, aay) \
	{ \
		Tsrc* src = (Tsrc*)(psrc); \
		T* pPageOffset = (T*)g_pbyGSMemory + fbp*(256/sizeof(T)), *dst; \
		int srcpitch = Pitch(fbw) * blockheight/sizeof(Tsrc); \
		int maxfbh = (0x00400000-fbp*256) / (sizeof(T) * fbw); \
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
		switch(psm) {
			case PSMCT32:
			case PSMCT24:
				if( s_AAy ) {
					RESOLVE_32BIT(32, u32, u32, 32A4, 8, 8, (u32), Frame, s_AAx, s_AAy);
				}
				else if( s_AAx ) {
					RESOLVE_32BIT(32, u32, u32, 32A2, 8, 8, (u32), Frame, 1, 0);
				}
				else {
					RESOLVE_32BIT(32, u32, u32, 32, 8, 8, (u32), Frame, 0, 0);
				}
				break;
			case PSMCT16:
				if( s_AAy ) {
					RESOLVE_32BIT(16, u16, u32, 16A4, 16, 8, RGBA32to16, Frame, s_AAx, s_AAy);
				}
				else if( s_AAx ) {
					RESOLVE_32BIT(16, u16, u32, 16A2, 16, 8, RGBA32to16, Frame, 1, 0);
				}
				else {
					RESOLVE_32BIT(16, u16, u32, 16, 16, 8, RGBA32to16, Frame, 0, 0);
				}

				break;
			case PSMCT16S:
				if( s_AAy ) {
					RESOLVE_32BIT(16S, u16, u32, 16A4, 16, 8, RGBA32to16, Frame, s_AAx, s_AAy);
				}
				else if( s_AAx ) {
					RESOLVE_32BIT(16S, u16, u32, 16A2, 16, 8, RGBA32to16, Frame, 1, 0);
				}
				else {
					RESOLVE_32BIT(16S, u16, u32, 16, 16, 8, RGBA32to16, Frame, 0, 0);
				}

				break;
			case PSMT32Z:
			case PSMT24Z:
				if( s_AAy ) {
					RESOLVE_32BIT(32Z, u32, u32, 32A4, 8, 8, (u32), Frame, s_AAx, s_AAy);
				}
				else if( s_AAx ) {
					RESOLVE_32BIT(32Z, u32, u32, 32A2, 8, 8, (u32), Frame, 1, 0);
				}
				else {
					RESOLVE_32BIT(32Z, u32, u32, 32, 8, 8, (u32), Frame, 0, 0);
				}

				break;
			case PSMT16Z:
				if( s_AAy ) {
					RESOLVE_32BIT(16Z, u16, u32, 16A4, 16, 8, (u16), Frame, s_AAx, s_AAy);
				}
				else if( s_AAx ) {
					RESOLVE_32BIT(16Z, u16, u32, 16A2, 16, 8, (u16), Frame, 1, 0);
				}
				else {
					RESOLVE_32BIT(16Z, u16, u32, 16, 16, 8, (u16), Frame, 0, 0);
				}

				break;
			case PSMT16SZ:
				if( s_AAy ) {
					RESOLVE_32BIT(16SZ, u16, u32, 16A4, 16, 8, (u16), Frame, s_AAx, s_AAy);
				}
				else if( s_AAx ) {
					RESOLVE_32BIT(16SZ, u16, u32, 16A2, 16, 8, (u16), Frame, 1, 0);
				}
				else {
					RESOLVE_32BIT(16SZ, u16, u32, 16, 16, 8, (u16), Frame, 0, 0);
				}

				break;
		}
	}
	else { // float16
		switch(psm) {
			case PSMCT32:
			case PSMCT24:
				if( s_AAy ) {
					RESOLVE_32BIT(32, u32, Vector_16F, 32A4, 8, 8, Float16ToARGB, Frame16, 1, 1);
				}
				else if( s_AAx ) {
					RESOLVE_32BIT(32, u32, Vector_16F, 32A2, 8, 8, Float16ToARGB, Frame16, 1, 0);
				}
				else {
					RESOLVE_32BIT(32, u32, Vector_16F, 32, 8, 8, Float16ToARGB, Frame16, 0, 0);
				}

				break;
			case PSMCT16:
				if( s_AAy ) {
					RESOLVE_32BIT(16, u16, Vector_16F, 16A4, 16, 8, Float16ToARGB16, Frame16, 1, 1);
				}
				else if( s_AAx ) {
					RESOLVE_32BIT(16, u16, Vector_16F, 16A2, 16, 8, Float16ToARGB16, Frame16, 1, 0);
				}
				else {
					RESOLVE_32BIT(16, u16, Vector_16F, 16, 16, 8, Float16ToARGB16, Frame16, 0, 0);
				}

				break;
			case PSMCT16S:
				if( s_AAy ) {
					RESOLVE_32BIT(16S, u16, Vector_16F, 16A4, 16, 8, Float16ToARGB16, Frame16, 1, 1);
				}
				else if( s_AAx ) {
					RESOLVE_32BIT(16S, u16, Vector_16F, 16A2, 16, 8, Float16ToARGB16, Frame16, 1, 0);
				}
				else {
					RESOLVE_32BIT(16S, u16, Vector_16F, 16, 16, 8, Float16ToARGB16, Frame16, 0, 0);
				}

				break;
			case PSMT32Z:
			case PSMT24Z:
				if( s_AAy ) {
					RESOLVE_32BIT(32Z, u32, Vector_16F, 32ZA4, 8, 8, Float16ToARGB_Z, Frame16, 1, 1);
				}
				else if( s_AAx ) {
					RESOLVE_32BIT(32Z, u32, Vector_16F, 32ZA2, 8, 8, Float16ToARGB_Z, Frame16, 1, 0);
				}
				else {
					RESOLVE_32BIT(32Z, u32, Vector_16F, 32Z, 8, 8, Float16ToARGB_Z, Frame16, 0, 0);
				}

				break;
			case PSMT16Z:
				if( s_AAy ) {
					RESOLVE_32BIT(16Z, u16, Vector_16F, 16ZA4, 16, 8, Float16ToARGB16_Z, Frame16, 1, 1);
				}
				else if( s_AAx ) {
					RESOLVE_32BIT(16Z, u16, Vector_16F, 16ZA2, 16, 8, Float16ToARGB16_Z, Frame16, 1, 0);
				}
				else {
					RESOLVE_32BIT(16Z, u16, Vector_16F, 16Z, 16, 8, Float16ToARGB16_Z, Frame16, 0, 0);
				}

				break;
			case PSMT16SZ:
				if( s_AAy ) {
					RESOLVE_32BIT(16SZ, u16, Vector_16F, 16ZA4, 16, 8, Float16ToARGB16_Z, Frame16, 1, 1);
				}
				else if( s_AAx ) {
					RESOLVE_32BIT(16SZ, u16, Vector_16F, 16ZA2, 16, 8, Float16ToARGB16_Z, Frame16, 1, 0);
				}
				else {
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
