/*  ZeroGS
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

#if defined(_WIN32) || defined(__WIN32__)
#include <d3dx9.h>
#include <dxerr.h>
#endif

#include <stdio.h>

#include <malloc.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <tchar.h>

#include "GS.h"
#include "Mem.h"
#include "x86.h"
#include "Regs.h"
#include "zerogs.h"
#include "resource.h"

#include "targets.h"

const float g_filog32 = 0.999f / (32.0f * logf(2.0f));

extern int g_GameSettings;
using namespace ZeroGS;
extern int g_TransferredToGPU;
extern BOOL g_bIsLost;
extern LPD3DTEX ptexConv16to32;

#ifdef RELEASE_TO_PUBLIC
#define INC_RESOLVE()
#else
#define INC_RESOLVE() ++g_nResolve
extern u32 g_nResolve;
extern BOOL g_bSaveTrans;
#endif

namespace ZeroGS {
	CRenderTargetMngr s_RTs, s_DepthRTs;
	CBitwiseTextureMngr s_BitwiseTextures;
	CMemoryTargetMngr g_MemTargs;

	extern BYTE s_AAx, s_AAy;
	extern BYTE bIndepWriteMasks;
	extern BOOL s_bBeginScene;
	extern D3DFORMAT g_RenderFormat;
	extern DXVEC4 g_vdepth;
	extern int icurctx;

	extern LPD3DVS pvsBitBlt;
	extern LPD3DPS ppsBitBlt[2], ppsBitBltDepth[2], ppsBitBltDepthTex[2], ppsOne;
	extern LPD3DPS ppsBaseTexture, ppsConvert16to32, ppsConvert32to16;
	extern LPD3DVB pvbRect;
}

extern LPD3DTEX s_ptexCurSet[2];
extern LPD3DTEX ptexBilinearBlocks;
extern IDirect3DVolumeTexture9* ptexConv32to16;
BOOL g_bSaveZUpdate = 0;

////////////////////
// Render Targets //
////////////////////
ZeroGS::CRenderTarget::CRenderTarget() : psurf(NULL), psys(NULL), ptex(NULL), ptexFeedback(NULL), psurfFeedback(NULL)
{
	nUpdateTarg = 0;
	targoffx = targoffy = 0;
}

ZeroGS::CRenderTarget::~CRenderTarget()
{
	Destroy();
}

CRenderTarget::CRenderTarget(const frameInfo& frame, CRenderTarget& r)
{
	lastused = timeGetTime();
	fbp = frame.fbp;
	fbw = frame.fbw;
	fbh = frame.fbh;
	psm = (u8)frame.psm;
	fbm = frame.fbm;

	// find the xy offset
	int blockheight = (frame.psm&2) ? 64 : 32;
	int scanlinewidth = 0x2000*(fbw>>6);

	// round down to nearest block and scanline
	int startheight = ((256*(fbp-r.fbp))/scanlinewidth) * blockheight;
	int offset = ((256*(fbp-r.fbp))%scanlinewidth) / 0x2000;
	
	targoffx = offset*64;
	targoffy = startheight;
	targheight = r.targheight;

	pmimicparent = &r;

	vposxy.x = 2.0f * (32767.0f / 8.0f) / (float)fbw;
	vposxy.y = -2.0f * (32767.0f / 8.0f) / (float)targheight;
	vposxy.z = -1+(2.0f*(float)targoffx-0.5f)/fbw;
	vposxy.w = 1+(-2.0f*(float)targoffy+0.5f)/(float)targheight;
	status = 0;

	GetRectMemAddress(start, end, psm, 0, 0, fbw, fbh, fbp, fbw);

	ptex = r.ptex;
	if( ptex != NULL ) ptex->AddRef();
	psurf = r.psurf;
	if( psurf != NULL ) psurf->AddRef();
	psys = r.psys;
	if( psys != NULL ) psys->AddRef();

	nUpdateTarg = 0;
	ptexFeedback = NULL;
	psurfFeedback = NULL;
}

BOOL ZeroGS::CRenderTarget::Create(const frameInfo& frame)
{
	Resolve();
	Destroy();

	lastused = timeGetTime();
	fbp = frame.fbp;
	fbw = frame.fbw;
	fbh = frame.fbh;
	psm = (u8)frame.psm;
	fbm = frame.fbm;
	pmimicparent = NULL;

	vposxy.x = 2.0f * (32767.0f / 8.0f) / (float)fbw;
	vposxy.y = -2.0f * (32767.0f / 8.0f) / (float)fbh;
	vposxy.z = -1+((float)targoffx-0.5f)/fbw;
	vposxy.w = 1+((float)targoffy+0.5f)/fbh;
	status = 0;

	if( fbw > 0 && fbh > 0 ) {
		GetRectMemAddress(start, end, psm, 0, 0, fbw, fbh, fbp, fbw);

		HRESULT hr;
		V(pd3dDevice->CreateTexture(fbw<<s_AAx, fbh<<s_AAy, 1, D3DUSAGE_RENDERTARGET, g_RenderFormat, D3DPOOL_DEFAULT, &ptex, NULL));

		if( SUCCEEDED(hr) ) {
			V(pd3dDevice->CreateOffscreenPlainSurface(fbw<<s_AAx, fbh<<s_AAy, g_RenderFormat, D3DPOOL_SYSTEMMEM, &psys, NULL));
		}

		if( SUCCEEDED(hr) ) {
			V(ptex->GetSurfaceLevel(0, &psurf));
		}

		if( FAILED(hr) ) {
			Destroy();
			return FALSE;
		}

		targheight = fbh;
		status = TS_NeedUpdate;
	}
	else {
		start = end = 0;
	}

	return TRUE;
}

void ZeroGS::CRenderTarget::Destroy()
{
	SAFE_RELEASE(psurf);
	SAFE_RELEASE(psys);
	SAFE_RELEASE(ptex);
	SAFE_RELEASE(ptexFeedback);
	SAFE_RELEASE(psurfFeedback);
}

void ZeroGS::CRenderTarget::SetTarget(int fbplocal, const Rect2& scissor, int context)
{
	int dy = 0;

	if( fbplocal != fbp ) {
		DXVEC4 v;

		// will be rendering to a subregion
		u32 bpp = (psm&2) ? 2 : 4;
		assert( ((256/bpp)*(fbplocal-fbp)) % fbw == 0 );
		assert( fbplocal >= fbp );

		dy = ((256/bpp)*(fbplocal-fbp)) / fbw;

		v.x = vposxy.x;
		v.y = vposxy.y;
		v.z = vposxy.z;
		v.w = vposxy.w - dy*2.0f/(float)fbh;
		SETCONSTF(GPU_POSXY0+context, v);
	}
	else
		SETCONSTF(GPU_POSXY0+context, vposxy);

	RECT rc;

	// set render states
	rc.left = scissor.x0>>3;
	rc.top = (scissor.y0>>3) + dy;
	rc.right = (scissor.x1>>3)+1;
	rc.bottom = (scissor.y1>>3)+1+dy;
	rc.right = min(rc.right, fbw);
	rc.bottom = min(rc.bottom, fbh);

	rc.left += targoffx; rc.right += targoffx;
	rc.top += targoffy; rc.bottom += targoffy;
	rc.left <<= s_AAx;
	rc.top <<= s_AAy;
	rc.right <<= s_AAx;
	rc.bottom <<= s_AAy;
//	rc.right--;
//	rc.bottom--;

	scissorrect = rc;
}

void ZeroGS::CRenderTarget::SetViewport()
{
	D3DVIEWPORT9 view;
	view.Width = fbw<<s_AAx;
	view.Height = targheight<<s_AAy;
	view.X = 0;
	view.Y = 0;
	view.MinZ = 0;
	view.MaxZ = 1.0f;
	pd3dDevice->SetViewport(&view);
}

static int g_bSaveResolved = 0;
extern int s_nResolved;

void ZeroGS::CRenderTarget::Resolve()
{
	if( psurf != NULL && !(status&TS_Resolved) && !(status&TS_NeedUpdate) ) {

		// flush if necessary
		if( vb[0].prndr == this || vb[0].pdepth == this ) Flush(0);
		if( vb[1].prndr == this || vb[1].pdepth == this ) Flush(1);

		if( (IsDepth() && !ZeroGS::IsWriteDepth()) || s_nResolved > 10 || (g_GameSettings&GAME_NOTARGETRESOLVE) || targoffx != 0 || targoffy != 0) {
			// don't resolve if depths aren't used
			status = TS_Resolved;
			return;
		}

		D3DLOCKED_RECT locksrc;
		pd3dDevice->GetRenderTargetData(psurf, psys);

#if !defined(RELEASE_TO_PUBLIC) && defined(PCSX2_DEBUG)
		if( g_bSaveResolved ) {
			D3DXSaveSurfaceToFile("resolved.tga", D3DXIFF_TGA, psys, NULL, NULL);
			g_bSaveResolved = 0;
		}
#endif

		psys->LockRect(&locksrc, NULL, D3DLOCK_READONLY);
		_Resolve(locksrc, fbp, fbw, fbh, psm, fbm);
		psys->UnlockRect();

		status = TS_Resolved;
	}
}

void ZeroGS::CRenderTarget::Resolve(int startrange, int endrange)
{
	assert( startrange < end && endrange > start ); // make sure it at least intersects

	if( psurf != NULL && !(status&TS_Resolved) && !(status&TS_NeedUpdate) ) {

		// flush if necessary
		if( vb[0].prndr == this || vb[0].pdepth == this ) Flush(0);
		if( vb[1].prndr == this || vb[1].pdepth == this ) Flush(1);

#if !defined(RELEASE_TO_PUBLIC) && defined(PCSX2_DEBUG)
		if( g_bSaveResolved ) {
			D3DXSaveSurfaceToFile("resolved.tga", D3DXIFF_TGA, psys, NULL, NULL);
			g_bSaveResolved = 0;
		}
#endif

		if((g_GameSettings&GAME_NOTARGETRESOLVE) || targoffx != 0 || targoffy != 0) {
			status = TS_Resolved;
			return;
		}

		int blockheight = (psm&2) ? 64 : 32;
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

		D3DLOCKED_RECT locksrc;
		pd3dDevice->GetRenderTargetData(psurf, psys);

		psys->LockRect(&locksrc, NULL, D3DLOCK_READONLY);

		if( fbp != resolvefbp )
			locksrc.pBits = (u8*)locksrc.pBits + ((resolvefbp-fbp)*256/scanlinewidth)*blockheight*locksrc.Pitch;
				
		_Resolve(locksrc, resolvefbp, fbw, resolveheight, psm, fbm);
		psys->UnlockRect();

		status = TS_Resolved;
	}
}

void ZeroGS::CRenderTarget::Update(int context, ZeroGS::CRenderTarget* pdepth)
{
	if( !s_bBeginScene ) {
		pd3dDevice->BeginScene();
		s_bBeginScene = TRUE;
	}

	assert( targoffx == 0 && targoffy == 0 );

	SETRS(D3DRS_SCISSORTESTENABLE, FALSE);
	SETRS(D3DRS_ALPHABLENDENABLE, FALSE);
	SETRS(D3DRS_ALPHATESTENABLE, FALSE);
	SETRS(D3DRS_ZENABLE, FALSE);
	SETRS(D3DRS_ZWRITEENABLE, FALSE);
	SETRS(D3DRS_COLORWRITEENABLE, 0xf);
	
	pd3dDevice->SetVertexShader(pvsBitBlt);
	pd3dDevice->SetStreamSource(0, pvbRect, 0, sizeof(VertexGPU));

	// assume depth already set
	//pd3dDevice->SetDepthStencilSurface(psurfDepth);
//	if( conf.mrtdepth && bIndepWriteMasks ) SETRS(D3DRS_COLORWRITEENABLE1, 0);
//	else
	pd3dDevice->SetRenderTarget(1, NULL);

	pd3dDevice->SetRenderTarget(0, psurf);

	DXVEC4 v = DXVEC4(1,1,-0.5f/(float)(fbw<<s_AAx),0.5f/(float)(targheight<<s_AAy));
		SETCONSTF(GPU_BITBLTPOS, v);

	CRenderTargetMngr::MAPTARGETS::iterator ittarg;
	if( nUpdateTarg ) {
		ittarg = s_RTs.mapTargets.find(nUpdateTarg);
		if( ittarg == s_RTs.mapTargets.end() ) {
			ittarg = s_DepthRTs.mapTargets.find(nUpdateTarg);
			if( ittarg == s_DepthRTs.mapTargets.end() )
				nUpdateTarg = 0;
			else if( ittarg->second == this ) {
				DEBUG_LOG("zerogs: updating self");
				nUpdateTarg = 0;
			}
		}
		else if( ittarg->second == this ) {
			DEBUG_LOG("zerogs: updating self");
			nUpdateTarg = 0;
		}
	}
	
	if( nUpdateTarg ) {

		pd3dDevice->SetTexture(SAMP_FINAL, ittarg->second->ptex);

		//assert( ittarg->second->fbw == fbw );
		int offset = (fbp-ittarg->second->fbp)*64/fbw;
		if( psm & 2 ) // 16 bit
			offset *= 2;

		v.x = 1;
		v.y = (float)targheight / ittarg->second->fbh;
		v.z = 0.25f / (fbw << s_AAx);
		v.w = (float)offset / ittarg->second->fbh + 0.25f / (ittarg->second->fbh << s_AAy);
		SETCONSTF(GPU_BITBLTTEX, v);

		v.x = v.y = v.z = v.w = 1;
		SETCONSTF(GPU_ONECOLOR, v);

		pd3dDevice->SetPixelShader(ppsBaseTexture);
		nUpdateTarg = 0;
	}
	else {
		// align the rect to the nearest page
		// note that fbp is always aligned on page boundaries
		tex0Info texframe;
		texframe.tbp0 =	fbp;
		texframe.tbw = fbw;
		texframe.tw = fbw;
		texframe.th = fbh;
		texframe.psm = psm;
		CMemoryTarget* pmemtarg = g_MemTargs.GetMemoryTarget(texframe, 1);

		// write color and zero out stencil buf, always 0 context!
		// force bilinear if using AA
		SetTexVariablesInt(0, (s_AAx || s_AAy)?2:0, texframe, pmemtarg, 1);

		v = DXVEC4(1,1,0,0);
		SETCONSTF(GPU_BITBLTTEX, v);

		v.x = 1;
		v.y = 2;
		SETCONSTF(GPU_ONECOLOR, v);

		assert( psurf != NULL );

		SetViewport();
		
		if( conf.options & GSOPTION_WIREFRAME ) SETRS(D3DRS_FILLMODE, D3DFILL_SOLID);

		if( ZeroGS::IsWriteDestAlphaTest() ) {
			SETRS(D3DRS_STENCILENABLE, TRUE);
			SETRS(D3DRS_STENCILWRITEMASK, 0xff); 
			SETRS(D3DRS_STENCILFUNC, D3DCMP_ALWAYS);
			SETRS(D3DRS_STENCILPASS, D3DSTENCILOP_ZERO);
		}

		// render with an AA shader if possible (bilinearly interpolates data)
		pd3dDevice->SetPixelShader(ppsBitBlt[s_AAx]);
	}

	pd3dDevice->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);

	// fill stencil buf only
	if( ZeroGS::IsWriteDestAlphaTest() && !(g_GameSettings&GAME_NOSTENCIL) ) {
		SETRS(D3DRS_COLORWRITEENABLE, 0);
		SETRS(D3DRS_ALPHATESTENABLE, TRUE);
		SETRS(D3DRS_ALPHAFUNC, D3DCMP_GREATEREQUAL);
		SETRS(D3DRS_ALPHAREF, 0xff);

		SETRS(D3DRS_STENCILPASS, D3DSTENCILOP_REPLACE);
		SETRS(D3DRS_STENCILREF, 1);

		pd3dDevice->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);
		SETRS(D3DRS_COLORWRITEENABLE, 0xf);
	}

	SETRS(D3DRS_SCISSORTESTENABLE, TRUE);

	if( conf.options & GSOPTION_WIREFRAME ) SETRS(D3DRS_FILLMODE, D3DFILL_WIREFRAME);

//	if( conf.mrtdepth && bIndepWriteMasks ) SETRS(D3DRS_COLORWRITEENABLE1, 0xf);
//	else
	if( conf.mrtdepth && pdepth != NULL && ZeroGS::IsWriteDepth() )
		pd3dDevice->SetRenderTarget(1, pdepth->psurf);

	status = TS_Resolved;

	// reset since settings changed
	vb[0].bVarsTexSync = 0; 
	ZeroGS::ResetAlphaVariables();
}

void ZeroGS::CRenderTarget::ConvertTo32()
{
	LPD3DTEX ptexConv;
	LPD3DSURF psurfConv;

	s_RTs.DestroyChildren(this);

	// create
	HRESULT hr;
	V(pd3dDevice->CreateTexture(fbw<<s_AAx, (fbh<<s_AAy)/2, 1, D3DUSAGE_RENDERTARGET, g_RenderFormat, D3DPOOL_DEFAULT, &ptexConv, NULL));

	if( FAILED(hr) ) {
		DEBUG_LOG("Failed to create feedback\n");
		return;
	}

	V(ptexConv->GetSurfaceLevel(0, &psurfConv));
	
	if( !s_bBeginScene ) {
		pd3dDevice->BeginScene();
		s_bBeginScene = TRUE;
	}

	SETRS(D3DRS_SCISSORTESTENABLE, FALSE);
	SETRS(D3DRS_ALPHABLENDENABLE, FALSE);
	SETRS(D3DRS_ALPHATESTENABLE, FALSE);
	SETRS(D3DRS_ZENABLE, FALSE);
	SETRS(D3DRS_ZWRITEENABLE, FALSE);
	SETRS(D3DRS_STENCILENABLE, FALSE);
	SETRS(D3DRS_COLORWRITEENABLE, 0xf);

	// tex coords, test ffx bikanel island when changing these
	float dx = 0.5f / (fbw << s_AAx);
	float dy = 0.5f / (fbh << s_AAy);

	DXVEC4 v = DXVEC4(1, 1, -dx, dy);
	SETCONSTF(GPU_BITBLTPOS, v);

	v.z = 0.5f*dx;
	v.w = 0.5f*dy;
	SETCONSTF(GPU_BITBLTTEX, v);

	v.x = v.y = v.z = 1;
	v.w = 1; // since all alpha is mult by 2
	SETCONSTF(GPU_ONECOLOR, v);

	v.x = 16.0f / (float)fbw;
	v.y = 64.0f / (float)fbh;
	v.z = 0.5f * v.x;
	v.w = 0.5f * v.y;
	SETCONSTF(GPU_TEXOFFSET0, v);

	v.x = 8.0f / (float)fbw;
	v.y = 0;
	v.z = 0;
	v.w = 0.25f;
	SETCONSTF(GPU_PAGEOFFSET0, v);

	v.x = 1;
	v.y = -0.5f;
	v.z = 0;
	v.w = 0.0001f;
	SETCONSTF(GPU_TEXDIMS0, v);

	v.x = 0;
	SETCONSTF(GPU_TEXBLOCK0, v);

	pd3dDevice->SetVertexShader(pvsBitBlt);
	pd3dDevice->SetStreamSource(0, pvbRect, 0, sizeof(VertexGPU));

	// assume depth already set !?
	pd3dDevice->SetRenderTarget(1, NULL);
	pd3dDevice->SetRenderTarget(0, psurfConv);
	pd3dDevice->SetTexture(SAMP_BILINEARBLOCKS, ptexConv32to16);
	pd3dDevice->SetTexture(SAMP_FINAL, ptex);

	pd3dDevice->SetSamplerState(SAMP_BILINEARBLOCKS, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
	pd3dDevice->SetSamplerState(SAMP_BILINEARBLOCKS, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
	pd3dDevice->SetSamplerState(SAMP_FINAL, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
	pd3dDevice->SetSamplerState(SAMP_FINAL, D3DSAMP_MINFILTER, D3DTEXF_POINT);

	if( s_ptexCurSet[0] == ptex ) {
		s_ptexCurSet[0] = NULL;
		pd3dDevice->SetTexture(SAMP_MEMORY0, NULL);
	}
	if( s_ptexCurSet[1] == ptex ) {
		s_ptexCurSet[1] = NULL;
		pd3dDevice->SetTexture(SAMP_MEMORY1, NULL);
	}

	fbh /= 2; // have 16 bit surfaces are usually 2x higher
	targheight /= 2;

	SetViewport();

	if( conf.options & GSOPTION_WIREFRAME ) SETRS(D3DRS_FILLMODE, D3DFILL_SOLID);

	// render with an AA shader if possible (bilinearly interpolates data)
	pd3dDevice->SetPixelShader(ppsConvert16to32);
	pd3dDevice->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);

#ifdef PCSX2_DEBUG
	//g_bSaveZUpdate = 1;
	//D3DXSaveSurfaceToFile("tex3.tga", D3DXIFF_TGA, psurfConv, NULL, NULL);
	if( g_bSaveZUpdate ) {
		// buggy
		D3DXSaveSurfaceToFile("tex3.tga", D3DXIFF_TGA, psurfConv, NULL, NULL);
		D3DXSaveSurfaceToFile("tex1.tga", D3DXIFF_TGA, psurf, NULL, NULL);

		//LPD3DTEX ptemp;
		//pd3dDevice->CreateTexture(fbw, fbh, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &ptemp, NULL);

		LPD3DSURF ptempsys;
		V(pd3dDevice->CreateOffscreenPlainSurface(fbw<<s_AAx, fbh<<s_AAy, g_RenderFormat, D3DPOOL_SYSTEMMEM, &ptempsys, NULL));

		D3DLOCKED_RECT srclock, dstlock;

		pd3dDevice->GetRenderTargetData(psurf, psys);

		psys->LockRect(&srclock, NULL, D3DLOCK_READONLY);
		ptempsys->LockRect(&dstlock, NULL, 0);
		DWORD cols[2];
		float tempf[4];
		for(int i = 0; i < fbh; ++i) {
			for(int j = 0; j < fbw; ++j) {
				D3DXVECTOR4_16F* pdst = (D3DXVECTOR4_16F*)dstlock.pBits + fbw*i + j;
				int jj = 2*j - (j%16);
				if( (j%16) >=8 ) jj += 8;
				int ii = 2*i - (i%8);
				if( j >= fbw/2 ) { ii += 8; jj -= fbw; }
				D3DXVECTOR4_16F* psrc = (D3DXVECTOR4_16F*)srclock.pBits + fbw*ii + jj;
				//D3DXFloat16To32Array(out, (D3DXFLOAT16*)psrc, 4);
				cols[0] = Float16ToARGB(psrc[0]);
				cols[1] = Float16ToARGB(psrc[8]);
				//cols[0] = (cols[0]&0xff00ff00)|((cols[0]&0xff)<<16)|((cols[0]&0xff0000)>>16);
				//cols[1] = (cols[1]&0xff00ff00)|((cols[1]&0xff)<<16)|((cols[1]&0xff0000)>>16);
				DWORD col = RGBA32to16(cols[0])|(RGBA32to16(cols[1])<<16);
				tempf[2] = (col&0xff)/255.0f;
				tempf[1] = ((col>>8)&0xff)/255.0f;
				tempf[0] = ((col>>16)&0xff)/255.0f;
				tempf[3] = 2*(((col>>24)&0xff)/255.0f);
				D3DXFloat32To16Array((D3DXFLOAT16*)pdst, tempf, 4);
			}
		}
		psys->UnlockRect();
		ptempsys->UnlockRect();

		D3DXSaveSurfaceToFile("tex2.tga", D3DXIFF_TGA, ptempsys, NULL, NULL);

//		SAFE_RELEASE(psys);
//		psys = ptempsys;
//
//		pd3dDevice->UpdateSurface(ptempsys, NULL, psurfConv, NULL);
	}
#endif

	vposxy.y = -2.0f * (32767.0f / 8.0f) / (float)fbh;
	vposxy.w = 1+0.5f/fbh;

	// restore
	SAFE_RELEASE(ptex);
	SAFE_RELEASE(psurf);
	SAFE_RELEASE(psys);
	SAFE_RELEASE(ptexFeedback);
	SAFE_RELEASE(psurfFeedback);
	ptex = ptexConv;
	psurf = psurfConv;

	V(pd3dDevice->CreateOffscreenPlainSurface(fbw<<s_AAx, fbh<<s_AAy, g_RenderFormat, D3DPOOL_SYSTEMMEM, &psys, NULL));

	if( conf.options & GSOPTION_WIREFRAME ) SETRS(D3DRS_FILLMODE, D3DFILL_WIREFRAME);

	pd3dDevice->SetTexture(SAMP_FINAL, NULL);
	pd3dDevice->SetTexture(SAMP_BILINEARBLOCKS, ptexBilinearBlocks); // restore
	pd3dDevice->SetSamplerState(SAMP_BILINEARBLOCKS, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP);
	pd3dDevice->SetSamplerState(SAMP_BILINEARBLOCKS, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP);
	pd3dDevice->SetSamplerState(SAMP_FINAL, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
	pd3dDevice->SetSamplerState(SAMP_FINAL, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
	SETRS(D3DRS_SCISSORTESTENABLE, TRUE);

	status = TS_Resolved;

	// TODO, reset depth?
	if( ZeroGS::icurctx >= 0 ) {
		// reset since settings changed
		vb[icurctx].bVarsTexSync = 0; 
		vb[icurctx].bVarsSetTarg = 0;
	}
	vb[0].bVarsTexSync = 0; 
}

// use when texture is not tiled and converting from 32bit to 16bit
// one condition is that the converted texture has to keep the same block configuration
// every 16 32bit horz pixels gets converted to 16x2 16bit horz pixels.
// the first row is the first 8 pixels, the second row is the last 8 pixels
// the last 8 columns are the upper bits
void ZeroGS::CRenderTarget::ConvertTo16()
{
	LPD3DTEX ptexConv;
	LPD3DSURF psurfConv;
	
	s_RTs.DestroyChildren(this);

	// create
	HRESULT hr;
	V(pd3dDevice->CreateTexture(fbw<<s_AAx, (fbh<<s_AAy)*2, 1, D3DUSAGE_RENDERTARGET, g_RenderFormat, D3DPOOL_DEFAULT, &ptexConv, NULL));

	if( FAILED(hr) ) {
		DEBUG_LOG("Failed to create feedback\n");
		return;
	}

	V(ptexConv->GetSurfaceLevel(0, &psurfConv));
	
	if( !s_bBeginScene ) {
		pd3dDevice->BeginScene();
		s_bBeginScene = TRUE;
	}

	SETRS(D3DRS_SCISSORTESTENABLE, FALSE);
	SETRS(D3DRS_ALPHABLENDENABLE, FALSE);
	SETRS(D3DRS_ALPHATESTENABLE, FALSE);
	SETRS(D3DRS_ZENABLE, FALSE);
	SETRS(D3DRS_ZWRITEENABLE, FALSE);
	SETRS(D3DRS_STENCILENABLE, FALSE);
	SETRS(D3DRS_COLORWRITEENABLE, 0xf);

	// tex coords, test ffx bikanel island when changing these
	float dx = 0.5f / (fbw << s_AAx);
	float dy = 0.5f / (fbh << s_AAy);

	DXVEC4 v = DXVEC4(1, 1, -dx, dy);
	SETCONSTF(GPU_BITBLTPOS, v);

	v.z = 0.5f*dx;
	v.w = 0.5f*dy;
	SETCONSTF(GPU_BITBLTTEX, v);

	v.x = v.y = v.z = 1;
	v.w = 2; // since all alpha is mult by 2
	SETCONSTF(GPU_ONECOLOR, v);

	v.x = 16.0f / (float)fbw;
	v.y = 32.0f / (float)fbh;
	v.z = 0.5f * v.x;
	v.w = 0.5f * v.y;
	SETCONSTF(GPU_TEXOFFSET0, v);

	v.x = 256.0f / 255.0f;
	v.y = 256.0f / 255.0f;
	v.z = 0.05f / 256.0f;
	v.w = -0.001f / 256.0f;
	SETCONSTF(GPU_PAGEOFFSET0, v);

	v.x = -0.5f;
	v.y = 1;
	v.z = 0;
	v.w = -0.1f/fbh;
	SETCONSTF(GPU_TEXDIMS0, v);

	pd3dDevice->SetVertexShader(pvsBitBlt);
	pd3dDevice->SetStreamSource(0, pvbRect, 0, sizeof(VertexGPU));

	// assume depth already set !?
	pd3dDevice->SetRenderTarget(1, NULL);
	pd3dDevice->SetRenderTarget(0, psurfConv);
	pd3dDevice->SetTexture(SAMP_BILINEARBLOCKS, ptexConv16to32);
	pd3dDevice->SetTexture(SAMP_FINAL, ptex);

	pd3dDevice->SetSamplerState(SAMP_BILINEARBLOCKS, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
	pd3dDevice->SetSamplerState(SAMP_BILINEARBLOCKS, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
	pd3dDevice->SetSamplerState(SAMP_FINAL, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
	pd3dDevice->SetSamplerState(SAMP_FINAL, D3DSAMP_MINFILTER, D3DTEXF_POINT);

	if( s_ptexCurSet[0] == ptex ) {
		s_ptexCurSet[0] = NULL;
		pd3dDevice->SetTexture(SAMP_MEMORY0, NULL);
	}
	if( s_ptexCurSet[1] == ptex ) {
		s_ptexCurSet[1] = NULL;
		pd3dDevice->SetTexture(SAMP_MEMORY1, NULL);
	}

	fbh *= 2; // have 16 bit surfaces are usually 2x higher
	targheight *= 2;

	// need to set a dummy target!
//	CRenderTargetMngr::MAPTARGETS::iterator itdepth = s_DepthRTs.mapDummyTargs.find( (fbw<<16)|fbh );
//	CDepthTarget* pnewdepth = NULL;
//	if( itdepth == s_DepthRTs.mapDummyTargs.end() ) {
//		frameInfo frame;
//		frame.fbh = fbh;
//		frame.fbw = fbw;
//		frame.psm = 0x30; //?
//		frame.fbw = fbw;
//		frame.fbm = 0;
//		pnewdepth = new CDepthTarget();
//		pnewdepth->Create(frame);
//		s_DepthRTs.mapDummyTargs[(fbw<<16)|fbh] = pnewdepth;
//	}
//	else pnewdepth = (CDepthTarget*)itdepth->second;
//
//	assert( pnewdepth != NULL );
//	pd3dDevice->SetDepthStencilSurface(pnewdepth->pdepth);

	SetViewport();

	if( conf.options & GSOPTION_WIREFRAME ) SETRS(D3DRS_FILLMODE, D3DFILL_SOLID);

	// render with an AA shader if possible (bilinearly interpolates data)
	pd3dDevice->SetPixelShader(ppsConvert32to16);
	pd3dDevice->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);

#ifdef PCSX2_DEBUG
	//g_bSaveZUpdate = 1;
	if( g_bSaveZUpdate ) {
		pd3dDevice->GetRenderTargetData(psurf, psys);

		D3DXSaveSurfaceToFile("tex1.tga", D3DXIFF_TGA, psurfConv, NULL, NULL);

		LPD3DSURF ptempsys;
		V(pd3dDevice->CreateOffscreenPlainSurface(fbw<<s_AAx, fbh<<s_AAy, g_RenderFormat, D3DPOOL_SYSTEMMEM, &ptempsys, NULL));

		D3DLOCKED_RECT srclock, dstlock;
		psys->LockRect(&srclock, NULL, D3DLOCK_READONLY);
		ptempsys->LockRect(&dstlock, NULL, 0);
		DWORD col;
		float temp[4];
		for(int i = 0; i < fbh; ++i) {
			for(int j = 0; j < fbw; ++j) {
				D3DXVECTOR4_16F* pdst = (D3DXVECTOR4_16F*)dstlock.pBits + fbw*i + j;
				int jj = j;
				int upper = 0;
				if( (j%16) >=8 ) {
					jj -= 8;
					upper = 1;
				}
				int ii = (i - (i%64))/2 + (i%64);
				if( (i%64) >= 32 ) {
					ii -= 32;
					jj += 8;
				}
				D3DXVECTOR4_16F* psrc = (D3DXVECTOR4_16F*)srclock.pBits + fbw*ii + jj;
				//D3DXFloat16To32Array(out, (D3DXFLOAT16*)psrc, 4);
				col = Float16ToARGB(psrc[0]);
				if( upper ) col >>= 16;
				else col &= 0xffff;
				col = RGBA16to32(col);
				temp[2] = (col&0xff)/255.0f;
				temp[1] = ((col>>8)&0xff)/255.0f;
				temp[0] = ((col>>16)&0xff)/255.0f;
				temp[3] = 2*(((col>>24)&0xff)/255.0f);
				D3DXFloat32To16Array((D3DXFLOAT16*)pdst, temp, 4);
				
			}
		}
		psys->UnlockRect();
		ptempsys->UnlockRect();

		D3DXSaveSurfaceToFile("tex2.tga", D3DXIFF_TGA, ptempsys, NULL, NULL);

		SAFE_RELEASE(psys);
		psys = ptempsys;

		pd3dDevice->UpdateSurface(ptempsys, NULL, psurfConv, NULL);
	}
#endif

	vposxy.y = -2.0f * (32767.0f / 8.0f) / (float)fbh;
	vposxy.w = 1+0.5f/fbh;

	// restore
	SAFE_RELEASE(ptex);
	SAFE_RELEASE(psurf);
	SAFE_RELEASE(psys);
	SAFE_RELEASE(ptexFeedback);
	SAFE_RELEASE(psurfFeedback);
	ptex = ptexConv;
	psurf = psurfConv;

	V(pd3dDevice->CreateOffscreenPlainSurface(fbw<<s_AAx, fbh<<s_AAy, g_RenderFormat, D3DPOOL_SYSTEMMEM, &psys, NULL));

	if( conf.options & GSOPTION_WIREFRAME ) SETRS(D3DRS_FILLMODE, D3DFILL_WIREFRAME);

	pd3dDevice->SetTexture(SAMP_FINAL, NULL);
	pd3dDevice->SetTexture(SAMP_BILINEARBLOCKS, ptexBilinearBlocks); // restore
	pd3dDevice->SetSamplerState(SAMP_BILINEARBLOCKS, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP);
	pd3dDevice->SetSamplerState(SAMP_BILINEARBLOCKS, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP);
	pd3dDevice->SetSamplerState(SAMP_FINAL, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
	pd3dDevice->SetSamplerState(SAMP_FINAL, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
	SETRS(D3DRS_SCISSORTESTENABLE, TRUE);

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
	if( ptexFeedback == NULL ) {
		// create
		assert( pmimicparent == NULL );

		HRESULT hr;
		V(pd3dDevice->CreateTexture(fbw<<s_AAx, fbh<<s_AAy, 1, D3DUSAGE_RENDERTARGET, g_RenderFormat, D3DPOOL_DEFAULT, &ptexFeedback, NULL));

		if( FAILED(hr) ) {
			DEBUG_LOG("Failed to create feedback\n");
			return;
		}

		V(ptexFeedback->GetSurfaceLevel(0, &psurfFeedback));
	}

	if( !s_bBeginScene ) {
		pd3dDevice->BeginScene();
		s_bBeginScene = TRUE;
	}

	SETRS(D3DRS_SCISSORTESTENABLE, FALSE);
	SETRS(D3DRS_ALPHABLENDENABLE, FALSE);
	SETRS(D3DRS_ALPHATESTENABLE, FALSE);
	SETRS(D3DRS_ZENABLE, FALSE);
	SETRS(D3DRS_ZWRITEENABLE, FALSE);
	SETRS(D3DRS_STENCILENABLE, FALSE);
	SETRS(D3DRS_COLORWRITEENABLE, 0xf);
	
	// assume depth already set
//	if( conf.mrtdepth && bIndepWriteMasks ) SETRS(D3DRS_COLORWRITEENABLE1, 0);
//	else
		pd3dDevice->SetRenderTarget(1, NULL);

	// tex coords, test ffx bikanel island when changing these
	float dx = 0.5f / (fbw << s_AAx);
	float dy = 0.5f / (fbh << s_AAy);

	DXVEC4 v = DXVEC4(1, 1, -dx, dy);
	SETCONSTF(GPU_BITBLTPOS, v);

	v.z = 0.5f*dx;
	v.w = 0.5f*dy;
	SETCONSTF(GPU_BITBLTTEX, v);

	v.x = v.y = v.z = 1;
	v.w = 1; // since all alpha is mult by 2
	SETCONSTF(GPU_ONECOLOR, v);

	pd3dDevice->SetVertexShader(pvsBitBlt);
	pd3dDevice->SetStreamSource(0, pvbRect, 0, sizeof(VertexGPU));

	pd3dDevice->SetRenderTarget(0, psurfFeedback);
	pd3dDevice->SetTexture(SAMP_FINAL, ptex);

	if( s_ptexCurSet[0] == ptexFeedback ) {
		s_ptexCurSet[0] = NULL;
		pd3dDevice->SetTexture(SAMP_MEMORY0, NULL);
	}
	if( s_ptexCurSet[1] == ptexFeedback ) {
		s_ptexCurSet[1] = NULL;
		pd3dDevice->SetTexture(SAMP_MEMORY1, NULL);
	}

	SetViewport();

	if( conf.options & GSOPTION_WIREFRAME ) SETRS(D3DRS_FILLMODE, D3DFILL_SOLID);

	// render with an AA shader if possible (bilinearly interpolates data)
	pd3dDevice->SetPixelShader(ppsBaseTexture);
	pd3dDevice->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);

	// restore
	swap(ptex, ptexFeedback);
	swap(psurf, psurfFeedback);

	if( conf.options & GSOPTION_WIREFRAME ) SETRS(D3DRS_FILLMODE, D3DFILL_WIREFRAME);

	pd3dDevice->SetTexture(SAMP_FINAL, NULL);
	SETRS(D3DRS_SCISSORTESTENABLE, TRUE);
	status |= TS_FeedbackReady;

	// TODO, reset depth?
	if( ZeroGS::icurctx >= 0 ) {
		// reset since settings changed
		vb[icurctx].bVarsTexSync = 0; 
	}
}

ZeroGS::CDepthTarget::CDepthTarget() : CRenderTarget(), pdepth(NULL) {}

ZeroGS::CDepthTarget::~CDepthTarget()
{
	Destroy();
}

BOOL ZeroGS::CDepthTarget::Create(const frameInfo& frame)
{
	if( !CRenderTarget::Create(frame) ) return FALSE;

	if( psm == 0x31 ) fbm = 0xff000000;
	else fbm = 0;

	HRESULT hr;
	V(pd3dDevice->CreateDepthStencilSurface(fbw<<s_AAx, fbh<<s_AAy, D3DFMT_D24S8, D3DMULTISAMPLE_NONE, 0, FALSE, &pdepth, NULL));
	if( FAILED(hr) ) {
		V(pd3dDevice->CreateDepthStencilSurface(fbw, fbh, D3DFMT_D24S8, D3DMULTISAMPLE_NONE, 0, FALSE, &pdepth, NULL));
	}

	if( FAILED(hr) )
		return FALSE;

	status = TS_NeedUpdate;

	return TRUE;
}

void ZeroGS::CDepthTarget::Destroy()
{
	if( pd3dDevice != NULL ) {
		pd3dDevice->SetRenderTarget(1, NULL);
		pd3dDevice->SetDepthStencilSurface(NULL);
		vb[0].bVarsSetTarg = 0;
		vb[1].bVarsSetTarg = 0;
	}

	CRenderTarget::Destroy();
	SAFE_RELEASE(pdepth);
}

extern int g_nDepthUsed; // > 0 if depth is used

void ZeroGS::CDepthTarget::Resolve()
{
	if( g_nDepthUsed > 0 && conf.mrtdepth && !(status&TS_Virtual) && ZeroGS::IsWriteDepth() && !(g_GameSettings&GAME_NODEPTHRESOLVE) )
		CRenderTarget::Resolve();
	else {
		// flush if necessary
		if( vb[0].prndr == this || vb[0].pdepth == this ) Flush(0);
		if( vb[1].prndr == this || vb[1].pdepth == this ) Flush(1);

		if( !(status & TS_Virtual) || targoffx != 0 || targoffy != 0 ) 
			status |= TS_Resolved;
	}

	if( !(status&TS_Virtual) ) {
		ZeroGS::SetWriteDepth();
	}
}

void ZeroGS::CDepthTarget::Resolve(int startrange, int endrange)
{
	if( g_nDepthUsed > 0 && conf.mrtdepth && !(status&TS_Virtual) && ZeroGS::IsWriteDepth() ) CRenderTarget::Resolve(startrange, endrange);
	else {
		// flush if necessary
		if( vb[0].prndr == this || vb[0].pdepth == this ) Flush(0);
		if( vb[1].prndr == this || vb[1].pdepth == this ) Flush(1);

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
	assert( !(status & TS_Virtual) );

	CRenderTarget* pusetarg = NULL;
	if( nUpdateTarg ) {
		CRenderTargetMngr::MAPTARGETS::iterator ittarg = s_DepthRTs.mapTargets.find(nUpdateTarg);
		if( ittarg == s_DepthRTs.mapTargets.end() || ittarg->second == this) {
			DEBUG_LOG("zerogs: updating self");
			nUpdateTarg = 0;
		}
		else pusetarg = ittarg->second;
	}

	tex0Info texframe;
	CMemoryTarget* pmemtarg = NULL;
	if( pusetarg == NULL ) {
		// align the rect to the nearest page
		// note that fbp is always aligned on page boundaries
		texframe.tbp0 =	fbp;
		texframe.tbw = fbw;
		texframe.tw = fbw;
		texframe.th = fbh;
		texframe.psm = psm;
		pmemtarg = g_MemTargs.GetMemoryTarget(texframe, 1);
	}

	if( !s_bBeginScene ) {
		pd3dDevice->BeginScene();
		s_bBeginScene = TRUE;
	}

	SETRS(D3DRS_SCISSORTESTENABLE, FALSE);
	SETRS(D3DRS_ALPHABLENDENABLE, FALSE);
	SETRS(D3DRS_ALPHATESTENABLE, FALSE);
	SETRS(D3DRS_ZENABLE, TRUE);
	SETRS(D3DRS_ZWRITEENABLE, TRUE);
	SETRS(D3DRS_STENCILENABLE, FALSE);
	SETRS(D3DRS_ZFUNC, D3DCMP_ALWAYS);

	DXVEC4 v = DXVEC4(1,1,-0.5f/(float)(fbw<<s_AAx),0.5f/(float)(fbh<<s_AAy));
		SETCONSTF(GPU_BITBLTPOS, v);

	if( pusetarg != NULL ) {
		pd3dDevice->SetTexture(SAMP_FINAL, pusetarg->ptex);

		//assert( pusetarg->fbw == fbw );
		int offset = (fbp-pusetarg->fbp)*64/fbw;
		if( psm & 2 ) // 16 bit
			offset *= 2;

		v.x = 1;
		v.y = (float)fbh / pusetarg->fbh;
		v.z = 0.25f / (fbw << s_AAx);
		v.w = (float)offset / pusetarg->fbh + 0.25f / (pusetarg->fbh << s_AAy);
		SETCONSTF(GPU_BITBLTTEX, v);
	}
	else {
		// write color and zero out stencil buf, always 0 context!
		SetTexVariablesInt(0, 0, texframe, pmemtarg, 1);

		DXVEC4 v = DXVEC4(1,1,0.5f/(float)fbw,0.5f/(float)fbh);
		SETCONSTF(GPU_BITBLTTEX, v);
	}

	pd3dDevice->SetVertexShader(pvsBitBlt);
	pd3dDevice->SetStreamSource(0, pvbRect, 0, sizeof(VertexGPU));

	v.x = 1;
	v.y = 2;
	v.z = (psm&3)==2?1.0f:0.0f;
	v.w = g_filog32;
	SETCONSTF(GPU_ONECOLOR, v);

	assert( psurf != NULL );

	DXVEC4 vdepth = ((255.0f/256.0f)*g_vdepth);
	if( psm == PSMT24Z ) vdepth.w = 0;
	else if( psm != PSMT32Z ) { vdepth.z = vdepth.w = 0; }
	SETCONSTF(GPU_BITBLTZ, ((255.0f/256.0f)*vdepth));

	assert( pdepth != NULL );

	if( ZeroGS::IsWriteDepth() && conf.mrtdepth && bIndepWriteMasks) {
		// write in MRT1
		pd3dDevice->SetRenderTarget(1, psurf);
		pd3dDevice->SetRenderTarget(0, prndr->psurf);
		SETRS(D3DRS_COLORWRITEENABLE, 0);
		SETRS(D3DRS_COLORWRITEENABLE1, 0xf);
	}
	else {
		// turn off writes?
		if( bIndepWriteMasks ) {
			pd3dDevice->SetRenderTarget(0, prndr->psurf);
			SETRS(D3DRS_COLORWRITEENABLE, 0);
		}
		else pd3dDevice->SetRenderTarget(0, psurf);
	}

	SetViewport();
	if( conf.options & GSOPTION_WIREFRAME ) SETRS(D3DRS_FILLMODE, D3DFILL_SOLID);

	pd3dDevice->SetDepthStencilSurface(pdepth);
	
	if( pusetarg != NULL ) {
		pd3dDevice->SetPixelShader(ppsBitBltDepthTex[conf.mrtdepth]);
	}
	else {
		pd3dDevice->SetPixelShader(ppsBitBltDepth[conf.mrtdepth]);
	}
	pd3dDevice->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);

	status = TS_Resolved;

	if( conf.mrtdepth && bIndepWriteMasks ) {
		SETRS(D3DRS_COLORWRITEENABLE, 0xf);
	}

	if( !ZeroGS::IsWriteDepth() ) {
		pd3dDevice->SetRenderTarget(1, NULL);
	}

	if( conf.options & GSOPTION_WIREFRAME ) SETRS(D3DRS_FILLMODE, D3DFILL_WIREFRAME);
	SETRS(D3DRS_SCISSORTESTENABLE, TRUE);

#ifdef PCSX2_DEBUG
	if( g_bSaveZUpdate ) {
		if( pusetarg != NULL )
			D3DXSaveTextureToFile("tex.tga", D3DXIFF_TGA, pusetarg->ptex, NULL);
		else
			SaveTex(&texframe, 1);
		D3DXSaveSurfaceToFile("frame1.tga", D3DXIFF_TGA, psurf, NULL, NULL);
	}
	//pd3dDevice->Clear(0, NULL, D3DCLEAR_ZBUFFER, 0, 0, 0);
#endif
}

void ZeroGS::CDepthTarget::SetDepthTarget()
{
	pd3dDevice->SetDepthStencilSurface(pdepth);

	if( conf.mrtdepth && ZeroGS::IsWriteDepth() )
		pd3dDevice->SetRenderTarget(1, psurf);
}

void ZeroGS::CRenderTargetMngr::Destroy()
{
	for(MAPTARGETS::iterator it = mapTargets.begin(); it != mapTargets.end(); ++it)
		delete it->second;
	mapTargets.clear();
	for(MAPTARGETS::iterator it = mapDummyTargs.begin(); it != mapDummyTargs.end(); ++it)
		delete it->second;
	mapDummyTargs.clear();
}

CRenderTarget* ZeroGS::CRenderTargetMngr::GetTarg(const frameInfo& frame, DWORD opts, int maxposheight)
{
	if( frame.fbw <= 0 || frame.fbh <= 0 )
		return NULL;

	u32 key = frame.fbp|(frame.fbw<<16);
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
			if( (frame.psm&2)==(it->second->psm&2) && !(g_GameSettings & GAME_FULL16BITRES) )
				bfound = (frame.fbh > 0x1c0 || it->second->fbh >= frame.fbh) && it->second->fbh <= maxposheight;
		}
	}

	if( !bfound ) {
		// might be a virtual target
		it = mapTargets.find(key|TARGET_VIRTUAL_KEY);
		bfound = it != mapTargets.end() && ((opts&TO_StrictHeight) ? it->second->fbh == frame.fbh : it->second->fbh >= frame.fbh) && it->second->fbh <= maxposheight;
	}

	if( bfound && (frame.psm&2) && !(it->second->psm&2) && (g_GameSettings&GAME_FULL16BITRES) ) {
		// mgs3
		if( frame.fbh > it->second->fbh ) {
			bfound = false;
		}
	}

	if( bfound ) {

		// can be both 16bit and 32bit
		if( (frame.psm&2) != (it->second->psm&2) ) {
			// a lot of games do this actually...
#ifdef PCSX2_DEBUG
			WARN_LOG("Really bad formats! %d %d\n", frame.psm, it->second->psm);
#endif
			if( !(opts&TO_StrictHeight) ) {
				if( !(g_GameSettings & GAME_VSSHACKOFF) ) {
					if( it->second->psm & 2 ) {
						it->second->status |= CRenderTarget::TS_NeedConvert32;
						it->second->fbh /= 2;
						it->second->targheight /= 2;
					}
					else {
						it->second->status |= CRenderTarget::TS_NeedConvert16;
						it->second->fbh *= 2;
						it->second->targheight *= 2;
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
		bool bCheckHalfCovering = (g_GameSettings&GAME_FULL16BITRES) && (it->second->psm&2) && it->second->fbh +32 < frame.fbh;

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

		if( !(opts & CRenderTargetMngr::TO_StrictHeight) ) {

			if( g_GameSettings&GAME_PARTIALPOINTERS ) {
				// if there is a render target that wholly encompasses this one, then use the pointer directly
				// this render target also has to be small (mgs3)
				if( frame.fbh <= 0x40 ) {
					for(MAPTARGETS::iterator it = mapTargets.begin(); it != mapTargets.end(); ++it) {
						if( it->second->start < end && start < it->second->end && (it->second->psm&~1) == (frame.psm&~1) && (it->second->fbp&0xff) == 0 ) {
							ptarg = new CRenderTarget(frame, *it->second);
							besttarg = it->first;
							break;
						}
					}
				}
			}
		}

		CRenderTarget* pbesttarg = NULL;

		if( besttarg == 0 ) {
			// if there is only one intersecting target and it encompasses the current one, update the new render target with
			// its data instead of resolving then updating (ffx2). Do not change the original target.
			for(MAPTARGETS::iterator it = mapTargets.begin(); it != mapTargets.end(); ++it) {
				if( it->second->start < end && start < it->second->end ) {
//						if( g_GameSettings&GAME_FASTUPDATE ) {
//							besttarg = it->first;
//							//break;
//						}
//						else {
						if( (g_GameSettings&GAME_FASTUPDATE) || (it->second->fbw == frame.fbw && 
							// check depth targets only if partialdepth option
							(it->second->fbp != frame.fbp|| ((g_GameSettings&GAME_PARTIALDEPTH)&&(opts&CRenderTargetMngr::TO_DepthBuffer))) ) ) {

							if( besttarg != 0 ) {
								besttarg = 0;
								break;
							}

							if( start >= it->second->start && end <= it->second->end ) {
								besttarg = it->first;
								pbesttarg = it->second;
							}
						}
//						}
				}
			}
		}

		if( besttarg == 0 ) {
			// if none found, resolve all
			DestroyAll(start, end, frame.fbw);
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
			DestroyChildren(it->second);
			delete it->second;
			mapTargets.erase(it);
		}
	}

	if( ptarg == NULL ) {
		// not found yet, so create

		if( mapDummyTargs.size() > 8 ) {
			it = GetOldestTarg(mapDummyTargs);

			DestroyChildren(it->second);
			delete it->second;
			mapDummyTargs.erase(it);
		}

		it = mapDummyTargs.find( (frame.fbw<<16)|frame.fbh );
		if( it != mapDummyTargs.end() ) {
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
			// create anew
			ptarg = (opts&TO_DepthBuffer) ? new CDepthTarget() : new CRenderTarget();
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

void CRenderTargetMngr::DestroyChildren(CRenderTarget* ptarg)
{
	if(ptarg == NULL)
		return;

	MAPTARGETS::iterator ittarg = mapTargets.begin();
	while(ittarg != mapTargets.end()) {
		if( ittarg->second->pmimicparent == ptarg ) {
			assert( ittarg->second != ptarg );
			delete ittarg->second;
			ittarg = mapTargets.erase(ittarg);
		}
		else ++ittarg;
	}
}

ZeroGS::CRenderTargetMngr::MAPTARGETS::iterator ZeroGS::CRenderTargetMngr::GetOldestTarg(MAPTARGETS& m)
{
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
	for(MAPTARGETS::const_iterator it = mapTargets.begin(); it != mapTargets.end(); ++it) {
		if( it->second->start < end && start < it->second->end ) listTargets.push_back(it->second);
	}
}

void ZeroGS::CRenderTargetMngr::Resolve(int start, int end)
{
	for(MAPTARGETS::const_iterator it = mapTargets.begin(); it != mapTargets.end(); ++it) {
		if( it->second->start < end && start < it->second->end )
			it->second->Resolve();
	}
}

void ZeroGS::CMemoryTargetMngr::Destroy()
{
	listTargets.clear();
	listClearedTargets.clear();
}

int memcmp_clut16(u16* pSavedBuffer, u16* pClutBuffer, int clutsize)
{
	assert( (clutsize&31) == 0 );

	// left > 0 only when csa < 16
	int left = ((u32)pClutBuffer & 2) ? 0 : (((u32)pClutBuffer & 0x3ff)/2) + clutsize - 512;
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
		pClutBuffer = (u16*)(ZeroGS::g_pbyGSClut + 2);

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
	assert( tex0.psm == psm && PSMT_ISCLUT(psm) && cpsm == tex0.cpsm );

	int nClutOffset = 0;
	int clutsize = 0;

	int entries = (tex0.psm&3)==3 ? 256 : 16;
	if( tex0.cpsm <= 1 ) { // 32 bit
		nClutOffset = 64 * tex0.csa;
		clutsize = min(entries, 256-tex0.csa*16)*4;
	}
	else {
		nClutOffset = 32 * (tex0.csa&15) + (tex0.csa>=16?2:0);
		clutsize = min(entries, 512-tex0.csa*16)*2;
	}

	assert( clutsize == clut.size() );

	if( cpsm <= 1 ) {
		// memcmp_mmx doesn't work on x86-64
#ifdef __x86_64__
		if( memcmp(&clut[0], ZeroGS::g_pbyGSClut+nClutOffset, clutsize) )
#else
		if( memcmp_mmx(&clut[0], ZeroGS::g_pbyGSClut+nClutOffset, clutsize) )
#endif
			return false;
	}
	else {
		if( memcmp_clut16((u16*)&clut[0], (u16*)(ZeroGS::g_pbyGSClut+nClutOffset), clutsize) )
			return false;
	}

	return true;
}

int VALIDATE_THRESH = 8;
u32 TEXDESTROY_THRESH = 16;

bool ZeroGS::CMemoryTarget::ValidateTex(const tex0Info& tex0, int starttex, int endtex, bool bDeleteBadTex)
{
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
	D3DLOCKED_RECT lock;
	assert( ptex != NULL );
	
	if( memory != NULL ) {
		assert( memory->ptr != NULL );
		lock.pBits = memory->ptr;
	}
	else {
		assert( ptexsys != NULL );
		ptexsys->LockRect(0, &lock, NULL, D3DLOCK_READONLY);
	}

	// memcmp_mmx doesn't work on x86_64
#ifdef __x86_64__
	int result = memcmp((u8*)lock.pBits + (checkstarty-realy)*4*GPU_TEXWIDTH, ZeroGS::g_pbyGSMemory+checkstarty*4*GPU_TEXWIDTH, (checkendy-checkstarty)*4*GPU_TEXWIDTH);
#else
	int result = memcmp_mmx((u8*)lock.pBits + (checkstarty-realy)*4*GPU_TEXWIDTH, ZeroGS::g_pbyGSMemory+checkstarty*4*GPU_TEXWIDTH, (checkendy-checkstarty)*4*GPU_TEXWIDTH);
#endif

	if( memory == NULL )
		ptexsys->UnlockRect(0);

	if( result == 0 || !bDeleteBadTex ) {
		if( result == 0 ) clearmaxy = 0;
		return result == 0;
	}

	// delete clearminy, clearmaxy range (not the checkstarty, checkendy range)
	int newstarty = 0;
	if( clearminy <= starty ) {
		if( clearmaxy < starty + height) {
			// preserve end
			height = starty+height-clearmaxy;
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
#define BUILDCLUT() { \
	switch(tex0.psm) { \
		case PSMT8: \
			for(int i = 0; i < targ->height; ++i) { \
				for(int j = 0; j < GPU_TEXWIDTH/2; ++j) { \
					pdst[0] = pclut[psrc[0]]; \
					pdst[1] = pclut[psrc[1]]; \
					pdst[2] = pclut[psrc[2]]; \
					pdst[3] = pclut[psrc[3]]; \
					pdst[4] = pclut[psrc[4]]; \
					pdst[5] = pclut[psrc[5]]; \
					pdst[6] = pclut[psrc[6]]; \
					pdst[7] = pclut[psrc[7]]; \
					pdst += 8; \
					psrc += 8; \
				} \
			} \
			break; \
		case PSMT4: \
			for(int i = 0; i < targ->height; ++i) { \
				for(int j = 0; j < GPU_TEXWIDTH; ++j) { \
					pdst[0] = pclut[psrc[0]&15]; \
					pdst[1] = pclut[psrc[0]>>4]; \
					pdst[2] = pclut[psrc[1]&15]; \
					pdst[3] = pclut[psrc[1]>>4]; \
					pdst[4] = pclut[psrc[2]&15]; \
					pdst[5] = pclut[psrc[2]>>4]; \
					pdst[6] = pclut[psrc[3]&15]; \
					pdst[7] = pclut[psrc[3]>>4]; \
					 \
					pdst += 8; \
					psrc += 4; \
				} \
			} \
			break; \
		case PSMT8H: \
			for(int i = 0; i < targ->height; ++i) { \
				for(int j = 0; j < GPU_TEXWIDTH/8; ++j) { \
					pdst[0] = pclut[psrc[3]]; \
					pdst[1] = pclut[psrc[7]]; \
					pdst[2] = pclut[psrc[11]]; \
					pdst[3] = pclut[psrc[15]]; \
					pdst[4] = pclut[psrc[19]]; \
					pdst[5] = pclut[psrc[23]]; \
					pdst[6] = pclut[psrc[27]]; \
					pdst[7] = pclut[psrc[31]]; \
					pdst += 8; \
					psrc += 32; \
				} \
			} \
			break; \
		case PSMT4HH: \
			for(int i = 0; i < targ->height; ++i) { \
				for(int j = 0; j < GPU_TEXWIDTH/8; ++j) { \
					pdst[0] = pclut[psrc[3]>>4]; \
					pdst[1] = pclut[psrc[7]>>4]; \
					pdst[2] = pclut[psrc[11]>>4]; \
					pdst[3] = pclut[psrc[15]>>4]; \
					pdst[4] = pclut[psrc[19]>>4]; \
					pdst[5] = pclut[psrc[23]>>4]; \
					pdst[6] = pclut[psrc[27]>>4]; \
					pdst[7] = pclut[psrc[31]>>4]; \
					pdst += 8; \
					psrc += 32; \
				} \
			} \
			break; \
		case PSMT4HL: \
			for(int i = 0; i < targ->height; ++i) { \
				for(int j = 0; j < GPU_TEXWIDTH/8; ++j) { \
					pdst[0] = pclut[psrc[3]&15]; \
					pdst[1] = pclut[psrc[7]&15]; \
					pdst[2] = pclut[psrc[11]&15]; \
					pdst[3] = pclut[psrc[15]&15]; \
					pdst[4] = pclut[psrc[19]&15]; \
					pdst[5] = pclut[psrc[23]&15]; \
					pdst[6] = pclut[psrc[27]&15]; \
					pdst[7] = pclut[psrc[31]&15]; \
					pdst += 8; \
					psrc += 32; \
				} \
			} \
			break; \
		default: \
			assert(0); \
	} \
} \

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

#if defined(_MSC_VER) && defined(__x86_64__)
extern "C" void UnswizzleZ16Target(void* dst, void* src, int iters);
#endif

ZeroGS::CMemoryTarget* ZeroGS::CMemoryTargetMngr::GetMemoryTarget(const tex0Info& tex0, int forcevalidate)
{
	int nbStart, nbEnd;
	GetRectMemAddress(nbStart, nbEnd, tex0.psm, 0, 0, tex0.tw, tex0.th, tex0.tbp0, tex0.tbw);
	assert( nbStart < nbEnd );
	nbEnd = min(nbEnd, 0x00400000);

	int nClutOffset = 0;
	int clutsize = 0;

	if( PSMT_ISCLUT(tex0.psm) ) {
		int entries = (tex0.psm&3)==3 ? 256 : 16;
		if( tex0.cpsm <= 1 ) { // 32 bit
			nClutOffset = 64 * tex0.csa;
			clutsize = min(entries, 256-tex0.csa*16)*4;
		}
		else {
			nClutOffset = 64 * (tex0.csa&15) + (tex0.csa>=16?2:0);
			clutsize = min(entries, 512-tex0.csa*16)*2;
		}
	}

	int start = nbStart / (4*GPU_TEXWIDTH);
	int end = (nbEnd + GPU_TEXWIDTH*4 - 1) / (4*GPU_TEXWIDTH);
	assert( start < end );

	for(list<CMemoryTarget>::iterator it = listTargets.begin(); it != listTargets.end();) {

		if( it->starty <= start && it->starty+it->height >= end ) {

			assert( it->psm != 0xd );

			// using clut, validate that same data
			if( PSMT_ISCLUT(it->psm) != PSMT_ISCLUT(tex0.psm) ) {
				if( it->validatecount++ > VALIDATE_THRESH ) {
					it = DestroyTargetIter(it);
					if( listTargets.size() == 0 )
						break;
				}
				else
					++it;
				continue;
			}

			if( PSMT_ISCLUT(tex0.psm) ) {
				assert( it->clut.size() > 0 );
				
				if( it->psm != tex0.psm || it->cpsm != tex0.cpsm || it->clut.size() != clutsize ) {
					// wrong clut
					list<CMemoryTarget>::iterator itprev = it;
					if( it->validatecount++ > VALIDATE_THRESH ) {
						it = DestroyTargetIter(it);
						if( listTargets.size() == 0 )
							break;
					}
					else
						++it;
					continue;
				}

				if( tex0.cpsm <= 1 ) {
#ifdef __x86_64__
					if( memcmp(&it->clut[0], ZeroGS::g_pbyGSClut+nClutOffset, clutsize) ) {
#else
					if( memcmp_mmx(&it->clut[0], ZeroGS::g_pbyGSClut+nClutOffset, clutsize) ) {
#endif
						++it;
						continue;
					}
				}
				else {
					if( memcmp_clut16((u16*)&it->clut[0], (u16*)(ZeroGS::g_pbyGSClut+nClutOffset), clutsize) ) {
						++it;
						continue;
					}
				}
			}
			else if( PSMT_IS16BIT(tex0.psm) != PSMT_IS16BIT(it->psm) ) {

				if( it->validatecount++ > VALIDATE_THRESH ) {
					it = DestroyTargetIter(it);
					if( listTargets.size() == 0 )
						break;
				}
				else ++it;

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
					else ++it;
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

#ifdef PCSX2_DEBUG
	PRIM_LOG("memtarget: tbp: %x tbw: %x th: %x psm: %x\n", tex0.tbp0, tex0.tbw, tex0.th, tex0.psm);
#endif

	// couldn't find so create
	HRESULT hr;
	CMemoryTarget* targ;

	D3DFORMAT fmt = D3DFMT_A8R8G8B8;
	if( (PSMT_ISCLUT(tex0.psm) && tex0.cpsm > 1) || tex0.psm == PSMCT16 || tex0.psm == PSMCT16S) {
		fmt = D3DFMT_A1R5G5B5;
	}
	
	int widthmult = 1;
	if( g_MaxTexHeight < 4096 ) {
		if( end-start > g_MaxTexHeight )
			widthmult = 2;
	}

	int channels = 1;
	if( PSMT_ISCLUT(tex0.psm) ) {
		if( tex0.psm == PSMT8 ) channels = 4;
		else if( tex0.psm == PSMT4 ) channels = 8;
	}
	else {
		if( PSMT_IS16BIT(tex0.psm) ) {
			// 16z needs to be a8r8g8b8
			channels = 2;
		}
	}

	if( listClearedTargets.size() > 0 ) {

		list<CMemoryTarget>::iterator itbest = listClearedTargets.begin();
		while(itbest != listClearedTargets.end()) {

			if( end-start <= itbest->realheight && itbest->fmt == fmt && itbest->widthmult == widthmult ) {

				// check channels
				int targchannels = 1;
				if( PSMT_ISCLUT(itbest->psm) ) {
					if( itbest->psm == PSMT8 ) targchannels = 4;
					else if( itbest->psm == PSMT4 ) targchannels = 8;
				}
				else if( PSMT_IS16BIT(itbest->psm) ) {
					targchannels = 2;
				}
				if( targchannels == channels )
					break;
			}
			++itbest;
		}

		if( itbest != listClearedTargets.end()) {
			listTargets.splice(listTargets.end(), listClearedTargets, itbest);
			targ = &listTargets.back();
			targ->validatecount = 0;
		}
		else {
			// create a new
			listTargets.push_back(CMemoryTarget());
			targ = &listTargets.back();
		}
	}
	else {
		listTargets.push_back(CMemoryTarget());
		targ = &listTargets.back();
	}

	// fill local clut
	if( PSMT_ISCLUT(tex0.psm) ) {
		assert( clutsize > 0 );
		targ->cpsm = tex0.cpsm;
		targ->clut.reserve(256*4); // no matter what
		targ->clut.resize(clutsize);
		
		if( tex0.cpsm <= 1 ) { // 32 bit
			memcpy_amd(&targ->clut[0], ZeroGS::g_pbyGSClut+nClutOffset, clutsize);
		}
		else {
			u16* pClutBuffer = (u16*)(ZeroGS::g_pbyGSClut + nClutOffset);
 			u16* pclut = (u16*)&targ->clut[0];
			int left = ((u32)nClutOffset & 2) ? 0 : ((nClutOffset&0x3ff)/2)+clutsize-512;
			if( left > 0 ) clutsize -= left;

			while(clutsize > 0) {
				pclut[0] = pClutBuffer[0];
				pclut++;
				pClutBuffer+=2;
				clutsize -= 2;
			}

			if( left > 0) {
				pClutBuffer = (u16*)(ZeroGS::g_pbyGSClut + 2);
				while(left > 0) {
					pclut[0] = pClutBuffer[0];
					left -= 2;
					pClutBuffer += 2;
					pclut++;
				}
			}
		}
	}

	if( targ->ptex != NULL ) {

		assert( end-start <= targ->realheight && targ->fmt == fmt && targ->widthmult == widthmult );
		// good enough, so init
		targ->realy = targ->starty = start;
		targ->usedstamp = curstamp;
		targ->psm = tex0.psm;
		targ->cpsm = tex0.cpsm;
		targ->height = end-start;
	}

	if( targ->ptex == NULL ) {

		// not initialized yet
		targ->fmt = fmt;
		targ->realy = targ->starty = start;
		targ->realheight = targ->height = end-start;
		targ->usedstamp = curstamp;
		targ->psm = tex0.psm;
		targ->cpsm = tex0.cpsm;
		targ->widthmult = widthmult;
	
		// alloc the mem
		while( FAILED(pd3dDevice->CreateTexture(GPU_TEXWIDTH*channels*widthmult, (targ->realheight+widthmult-1)/widthmult, 1, 0, fmt, D3DPOOL_DEFAULT, &targ->ptex, NULL)) ) {

			if( listClearedTargets.size() > 0 )
				listClearedTargets.pop_front();
			else {
				if( listTargets.size() == 0 ) {
					DEBUG_LOG("Failed to create %dx%x texture\n", GPU_TEXWIDTH*channels*widthmult, (targ->realheight+widthmult-1)/widthmult);
					channels = 1;
				}
				DestroyOldest();
			}
		}
		
		// lock
		V(pd3dDevice->CreateTexture(GPU_TEXWIDTH*channels*widthmult, (targ->realheight+widthmult-1)/widthmult, 1, 0, fmt, D3DPOOL_SYSTEMMEM, &targ->ptexsys, NULL));
		assert( targ->ptexsys != NULL );
	}

#ifndef RELEASE_TO_PUBLIC
	g_TransferredToGPU += GPU_TEXWIDTH * channels * 4 * targ->height;
#endif

	D3DLOCKED_RECT lock;
	targ->ptexsys->LockRect(0, &lock, NULL, 0);
	
	// fill with data
	if( PSMT_ISCLUT(tex0.psm) ) {

		if( targ->memory == NULL ) {
			targ->memory = new CMemoryTarget::MEMORY();
			targ->memory->ptr = (BYTE*)_aligned_malloc(4 * GPU_TEXWIDTH * targ->realheight, 16);
			targ->memory->ref = 1;
		}

		memcpy_amd(targ->memory->ptr, ZeroGS::g_pbyGSMemory + 4 * GPU_TEXWIDTH * targ->realy, 4 * GPU_TEXWIDTH * targ->height);

		u8* psrc = (u8*)(ZeroGS::g_pbyGSMemory + 4 * GPU_TEXWIDTH * targ->realy);

		if( tex0.cpsm <= 1 ) { // 32bit
			u32* pclut = (u32*)&targ->clut[0];
			u32* pdst = (u32*)lock.pBits;

			BUILDCLUT();
		}
		else {
			u16* pclut = (u16*)&targ->clut[0];
			u16* pdst = (u16*)lock.pBits;
			
			BUILDCLUT();
		}
	}
	else {
		if( tex0.psm == PSMT16Z || tex0.psm == PSMT16SZ ) {

			if( targ->memory == NULL ) {
				targ->memory = new CMemoryTarget::MEMORY();
				targ->memory->ptr = (BYTE*)_aligned_malloc(4 * GPU_TEXWIDTH * targ->realheight, 16);
				targ->memory->ref = 1;
			}

			memcpy_amd(targ->memory->ptr, ZeroGS::g_pbyGSMemory + 4 * GPU_TEXWIDTH * targ->realy, 4 * GPU_TEXWIDTH * targ->height);

			// needs to be 8 bit, use xmm for unpacking
			u16* dst = (u16*)lock.pBits;
			u16* src = (u16*)(ZeroGS::g_pbyGSMemory + 4 * GPU_TEXWIDTH * targ->realy);

			assert( ((u32)dst)%16 == 0 );

#if defined(ZEROGS_SSE2)
			int iters = targ->height*GPU_TEXWIDTH/16;

#if defined(__x86_64__)
			UnswizzleZ16Target(dst, src, iters);
#else
			__asm {
				mov edx, iters
				pxor xmm7, xmm7
				mov eax, dst
				mov ecx, src

Z16Loop:
				// unpack 64 bytes at a time
				movdqa xmm0, [ecx]
				movdqa xmm2, [ecx+16]
				movdqa xmm4, [ecx+32]
				movdqa xmm6, [ecx+48]

				movdqa xmm1, xmm0
				movdqa xmm3, xmm2
				movdqa xmm5, xmm4

				punpcklwd xmm0, xmm7
				punpckhwd xmm1, xmm7
				punpcklwd xmm2, xmm7
				punpckhwd xmm3, xmm7

				// start saving
				movdqa [eax], xmm0
				movdqa [eax+16], xmm1

				punpcklwd xmm4, xmm7
				punpckhwd xmm5, xmm7

				movdqa [eax+32], xmm2
				movdqa [eax+48], xmm3

				movdqa xmm0, xmm6
				punpcklwd xmm6, xmm7

				movdqa [eax+64], xmm4
				movdqa [eax+80], xmm5

				punpckhwd xmm0, xmm7

				movdqa [eax+96], xmm6
				movdqa [eax+112], xmm0

				add ecx, 64
				add eax, 128
				sub edx, 1
				jne Z16Loop
			}
#endif // __x86_64__
#else
				for(int i = 0; i < targ->height; ++i) {
					for(int j = 0; j < GPU_TEXWIDTH; ++j) {
						dst[0] = src[0]; dst[1] = 0;
						dst[2] = src[1]; dst[3] = 0;
						dst += 4;
						src += 2;
					}
				}
#endif
		}
		else {
			if( targ->memory != NULL ) {
				// release
				if( targ->memory->ref > 0 && --targ->memory->ref <= 0 ) {
					SAFE_DELETE(targ->memory);
				}
				targ->memory = NULL;
			}

			memcpy_amd(lock.pBits, ZeroGS::g_pbyGSMemory + 4 * GPU_TEXWIDTH * targ->realy, 4 * GPU_TEXWIDTH * targ->height );
		}
	}

	targ->ptexsys->UnlockRect(0);

	V(pd3dDevice->UpdateTexture(targ->ptexsys, targ->ptex));

	assert( tex0.psm != 0xd );
	if( PSMT_ISCLUT(tex0.psm) )
		assert( targ->clut.size() > 0 );

	return targ;
}

void ZeroGS::CMemoryTargetMngr::ClearRange(int nbStartY, int nbEndY)
{
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
//	}
//	else {
//		for(list<CMemoryTarget>::iterator it = listTargets.begin(); it != listTargets.end(); ) {
//
//			if( it->starty < endy && (it->starty+it->height) > starty ) {
//				int newstarty = 0;
//				if( starty <= it->starty ) {
//					if( endy < it->starty + it->height) {
//						// preserve end
//						it->height = it->starty+it->height-endy;
//						it->starty = endy;
//						assert(it->height > 0);
//					}
//					else {
//						// destroy
//						it->height = 0;
//					}
//				}
//				else {
//					// beginning can be preserved
//					it->height = starty-it->starty;
//				}
//
//				assert( it->starty >= it->realy && it->starty+it->height<=it->realy+it->realheight );
//				if( it->height <= 0 ) {
//					list<CMemoryTarget>::iterator itprev = it; ++it;
//					listClearedTargets.splice(listClearedTargets.end(), listTargets, itprev);
//					continue;
//				}
//			}
//			
//			++it;
//		}
//	}
}

void ZeroGS::CMemoryTargetMngr::DestroyCleared()
{
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
	for(map<u32, LPD3DTEX>::iterator it = mapTextures.begin(); it != mapTextures.end(); ++it)
		it->second->Release();
	mapTextures.clear();
}

LPD3DTEX ZeroGS::CBitwiseTextureMngr::GetTexInt(u32 bitvalue, LPD3DTEX ptexDoNotDelete)
{
	if( mapTextures.size() > 32 ) {
		// randomly delete 8
		for(map<u32, LPD3DTEX>::iterator it = mapTextures.begin(); it != mapTextures.end();) {
			if( !(rand()&3) && it->second != ptexDoNotDelete) {
				it->second->Release();
				it = mapTextures.erase(it);
			}
			else ++it;
		}
	}

	// create a new tex
	LPD3DTEX ptex;
	HRESULT hr = S_OK;
	V(pd3dDevice->CreateTexture(GPU_TEXMASKWIDTH, 1, 1, 0, D3DFMT_L16, D3DPOOL_MANAGED, &ptex, NULL));

	D3DLOCKED_RECT lock;
	ptex->LockRect(0, &lock, NULL, 0);
	for(u32 i = 0; i < GPU_TEXMASKWIDTH; ++i) ((u16*)lock.pBits)[i] = ((i&bitvalue)<<6)|0x1f; // add the 1/2 offset so that
	ptex->UnlockRect(0);

	mapTextures[bitvalue] = ptex;
	return ptex;
}

void ZeroGS::CRangeManager::Insert(int start, int end)
{
	int imin = 0, imax = (int)ranges.size(), imid;
	
#ifdef PCSX2_DEBUG
	// sanity check
	for(int i = 0; i < (int)ranges.size()-1; ++i) assert( ranges[i].end < ranges[i+1].start );
#endif

	switch( ranges.size() ) {
		case 0:
			ranges.push_back(RANGE(start, end));
			return;

		case 1:
			if( end < ranges.front().start ) {
				ranges.insert(ranges.begin(), RANGE(start, end));
			}
			else if( start > ranges.front().end ) {
				ranges.push_back(RANGE(start, end));
			}
			else {
				if( start < ranges.front().start ) ranges.front().start = start;
				if( end > ranges.front().end ) ranges.front().end = end;
			}

			return;
	}

	// find where start is
	while(imin < imax) {
		imid = (imin+imax)>>1;

		assert( imid < (int)ranges.size() );

		if( ranges[imid].end >= start && (imid == 0 || ranges[imid-1].end < start) ) {
			imin = imid;
			break;
		}
		else if( ranges[imid].start > start ) imax = imid;
		else imin = imid+1;
	}

	int startindex = imin;

	if( startindex >= (int)ranges.size() ) {
		// non intersecting
		assert( start > ranges.back().end );
		ranges.push_back(RANGE(start, end));
		return;
	}
	if( startindex == 0 && end < ranges.front().start ) {
		ranges.insert(ranges.begin(), RANGE(start, end));

#ifdef PCSX2_DEBUG
		// sanity check
		for(int i = 0; i < (int)ranges.size()-1; ++i) assert( ranges[i].end < ranges[i+1].start );
#endif
		return;
	}

	imin = 0; imax = (int)ranges.size();

	// find where end is
	while(imin < imax) {
		imid = (imin+imax)>>1;

		assert( imid < (int)ranges.size() );

		if( ranges[imid].end <= end && (imid == ranges.size()-1 || ranges[imid+1].start > end ) ) {
			imin = imid;
			break;
		}
		else if( ranges[imid].start >= end ) imax = imid;
		else imin = imid+1;
	}

	int endindex = imin;

	if( startindex > endindex ) {
		// create a new range
		ranges.insert(ranges.begin()+startindex, RANGE(start, end));

#ifdef PCSX2_DEBUG
		// sanity check
		for(int i = 0; i < (int)ranges.size()-1; ++i) assert( ranges[i].end < ranges[i+1].start );
#endif
		return;
	}

	if( endindex >= (int)ranges.size()-1 ) {
		// pop until startindex is reached
		int lastend = ranges.back().end;
		int numpop = (int)ranges.size() - startindex - 1;
		while(numpop-- > 0 ) ranges.pop_back();

		assert( start <= ranges.back().end );
		if( start < ranges.back().start ) ranges.back().start = start;
		if( lastend > ranges.back().end ) ranges.back().end = lastend;
		if( end > ranges.back().end ) ranges.back().end = end;

#ifdef PCSX2_DEBUG
		// sanity check
		for(int i = 0; i < (int)ranges.size()-1; ++i) assert( ranges[i].end < ranges[i+1].start );
#endif

		return;
	}

	if( endindex == 0 ) {
		assert( end >= ranges.front().start );
		if( start < ranges.front().start ) ranges.front().start = start;
		if( end > ranges.front().end ) ranges.front().end = end;
		
#ifdef PCSX2_DEBUG
		// sanity check
		for(int i = 0; i < (int)ranges.size()-1; ++i) assert( ranges[i].end < ranges[i+1].start );
#endif
	}

	// somewhere in the middle
	if( ranges[startindex].start < start ) start = ranges[startindex].start;

	if( startindex < endindex ) {
		ranges.erase(ranges.begin() + startindex, ranges.begin() + endindex );
	}

	if( start < ranges[startindex].start ) ranges[startindex].start = start;
	if( end > ranges[startindex].end ) ranges[startindex].end = end;

#ifdef PCSX2_DEBUG
	// sanity check
	for(int i = 0; i < (int)ranges.size()-1; ++i) assert( ranges[i].end < ranges[i+1].start );
#endif
}

namespace ZeroGS {

CRangeManager s_RangeMngr; // manages overwritten memory
static int gs_imageEnd = 0;

void ResolveInRange(int start, int end)
{
	list<CRenderTarget*> listTargs;
	s_DepthRTs.GetTargs(start, end, listTargs);
	s_RTs.GetTargs(start, end, listTargs);

	if( listTargs.size() > 0 ) {
		// flushes can delete targets!
		Flush(0);
		Flush(1);

		listTargs.clear(); 
		s_DepthRTs.GetTargs(start, end, listTargs);
		s_RTs.GetTargs(start, end, listTargs);

		for(list<CRenderTarget*>::iterator it = listTargs.begin(); it != listTargs.end(); ++it) {
			// only resolve if not completely covered
			(*it)->Resolve();
		}
	}
}

//////////////////
// Transferring //
//////////////////
void FlushTransferRanges(const tex0Info* ptex)
{
	assert( s_RangeMngr.ranges.size() > 0 );
	bool bHasFlushed = false;
	list<CRenderTarget*> listTransmissionUpdateTargs;

	int texstart = -1, texend = -1;
	if( ptex != NULL ) {
		GetRectMemAddress(texstart, texend, ptex->psm, 0, 0, ptex->tw, ptex->th, ptex->tbp0, ptex->tbw);
	}

	for(vector<CRangeManager::RANGE>::iterator itrange = s_RangeMngr.ranges.begin(); itrange != s_RangeMngr.ranges.end(); ++itrange ) {

		int start = itrange->start;
		int end = itrange->end;

		listTransmissionUpdateTargs.clear();
		s_DepthRTs.GetTargs(start, end, listTransmissionUpdateTargs);
		s_RTs.GetTargs(start, end, listTransmissionUpdateTargs);

//		if( !bHasFlushed && listTransmissionUpdateTargs.size() > 0 ) {
//			Flush(0);
//			Flush(1);
//
//#ifdef PCSX2_DEBUG
//			// make sure targets are still the same
//			list<CRenderTarget*>::iterator it;
//			FORIT(it, listTransmissionUpdateTargs) {
//				CRenderTargetMngr::MAPTARGETS::iterator itmap;
//				for(itmap = s_RTs.mapTargets.begin(); itmap != s_RTs.mapTargets.end(); ++itmap) {
//					if( itmap->second == *it )
//						break;
//				}
//
//				if( itmap == s_RTs.mapTargets.end() ) {
//
//					for(itmap = s_DepthRTs.mapTargets.begin(); itmap != s_DepthRTs.mapTargets.end(); ++itmap) {
//						if( itmap->second == *it )
//							break;
//					}
//
//					assert( itmap != s_DepthRTs.mapTargets.end() );
//				}
//			}
//#endif
//		}

		for(list<CRenderTarget*>::iterator it = listTransmissionUpdateTargs.begin(); it != listTransmissionUpdateTargs.end(); ++it) {

			CRenderTarget* ptarg = *it;

			if( (ptarg->status & CRenderTarget::TS_Virtual) || ptarg->targoffx != 0 || ptarg->targoffy != 0)
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
//				if( start <= texstart && end >= texend ) {
//					// texture taken care of so can skip!?
//					continue;
//				}
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
					if( end - start > 0x8000 ) {
						// intersects, do only one sided resolves
						if( end-start > 4*ptarg->fbw ) { // at least it be greater than one scanline (spiro is faster)
							if( start > ptarg->start ) {
								ptarg->Resolve(ptarg->start, start);
							}
							else if( end < ptarg->end ) {
								ptarg->Resolve(end, ptarg->end);
							}
						}
					}
					
					ptarg->status |= CRenderTarget::TS_Resolved;
					if( !ptarg->IsDepth() || (!(g_GameSettings & GAME_NODEPTHUPDATE) || end-start > 0x1000) )
						ptarg->status |= CRenderTarget::TS_NeedUpdate;
				}
			}
		}

		ZeroGS::g_MemTargs.ClearRange(start, end);
	}

	s_RangeMngr.Clear();
}

// need to take into account left over data of 24bit transfers (always <= 5)
static vector<u8> s_vTempBuffer, s_vTransferCache;

void InitTransferHostLocal()
{
	if( g_bIsLost )
		return;

#ifndef RELEASE_TO_PUBLIC
	if( gs.trxpos.dx+gs.imageWnew > gs.dstbuf.bw )
		WARN_LOG("Transfer error, width exceeds\n");
#endif

	bool bHasFlushed = false;

	gs.imageX = gs.trxpos.dx;
	gs.imageY = gs.trxpos.dy;
	gs.imageEndX = gs.imageX + gs.imageWnew;
	gs.imageEndY = gs.imageY + gs.imageHnew;
	s_vTransferCache.resize(0);

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

	if( vb[0].dwCount > 0 )
		Flush(0);
	if( vb[1].dwCount > 0 )
		Flush(1);

	//PRIM_LOG("trans: bp:%x x:%x y:%x w:%x h:%x\n", gs.dstbuf.bp, gs.trxpos.dx, gs.trxpos.dy, gs.imageWnew, gs.imageHnew);

//	if( !bHasFlushed && (vb[0].bNeedFrameCheck || vb[0].bNeedZCheck || vb[1].bNeedFrameCheck || vb[1].bNeedZCheck)) {
//		Flush(0);
//		Flush(1);
//		bHasFlushed = 1;
//	}
//
//	// for all ranges, flush the targets
//	// check if new rect intersects with current rendering texture, if so, flush
//	if( vb[0].dwCount > 0 && vb[0].curprim.tme ) {
//		int tstart, tend;
//		GetRectMemAddress(tstart, tend, vb[0].tex0.psm, 0, 0, vb[0].tex0.tw, vb[0].tex0.th, vb[0].tex0.tbp0, vb[0].tex0.tbw);
//
//		if( start < tend && end > tstart ) {
//			Flush(0);
//			Flush(1);
//			bHasFlushed = 1;
//		}
//	}
//
//	if( !bHasFlushed && vb[1].dwCount > 0 && vb[1].curprim.tme ) {
//		int tstart, tend;
//		GetRectMemAddress(tstart, tend, vb[1].tex0.psm, 0, 0, vb[1].tex0.tw, vb[1].tex0.th, vb[1].tex0.tbp0, vb[1].tex0.tbw);
//
//		if( start < tend && end > tstart ) {
//			Flush(0);
//			Flush(1);
//			bHasFlushed = 1;
//		}
//	}

	//ZeroGS::g_MemTargs.ClearRange(start, end);
	//s_RangeMngr.Insert(start, end);
}

void TransferHostLocal(const void* pbyMem, u32 nQWordSize)
{
	if( g_bIsLost )
		return;

	int start, end;
	GetRectMemAddress(start, end, gs.dstbuf.psm, gs.imageX, gs.imageY, gs.imageWnew, gs.imageHnew, gs.dstbuf.bp, gs.dstbuf.bw);
	assert( start < gs_imageEnd );

	end = gs_imageEnd;

	// sometimes games can decompress to alpha channel of render target only, in this case
	// do a resolve right away. wolverine x2
	if( gs.dstbuf.psm == PSMT8H || gs.dstbuf.psm == PSMT4HL || gs.dstbuf.psm == PSMT4HH ) {
		list<CRenderTarget*> listTransmissionUpdateTargs;
		s_RTs.GetTargs(start, end, listTransmissionUpdateTargs);

		for(list<CRenderTarget*>::iterator it = listTransmissionUpdateTargs.begin(); it != listTransmissionUpdateTargs.end(); ++it) {

			CRenderTarget* ptarg = *it;

			if( (ptarg->status & CRenderTarget::TS_Virtual) )
				continue;

			//DEBUG_LOG("zerogs: resolving to alpha channel\n");
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

#if !defined(RELEASE_TO_PUBLIC) && defined(PCSX2_DEBUG)
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

// left/right, top/down
//void TransferHostLocal(const void* pbyMem, u32 nQWordSize)
//{
//	assert( gs.imageTransfer == 0 );
//	u8* pstart = g_pbyGSMemory + gs.dstbuf.bp*256;
//	
//	const u8* pendbuf = (const u8*)pbyMem + nQWordSize*4;
//	int i = gs.imageY, j = gs.imageX;
//
//#define DSTPSM gs.dstbuf.psm
//
//#define TRANSFERHOSTLOCAL(psm, T, widthlimit) { \
//	const T* pbuf = (const T*)pbyMem; \
//	u32 nSize = nQWordSize*(4/sizeof(T)); \
//	assert( (nSize%widthlimit) == 0 && widthlimit <= 4 ); \
//	if( ((gs.imageEndX-gs.trxpos.dx)%widthlimit) ) DEBUG_LOG("Bad Transmission! %d %d, psm: %d\n", gs.trxpos.dx, gs.imageEndX, DSTPSM); \
//	for(; i < gs.imageEndY; ++i) { \
//		for(; j < gs.imageEndX && nSize > 0; j += widthlimit, nSize -= widthlimit, pbuf += widthlimit) { \
//			/* write as many pixel at one time as possible */ \
//			writePixel##psm##_0(pstart, j%2048, i%2048, pbuf[0], gs.dstbuf.bw); \
//			\
//			if( widthlimit > 1 ) { \
//				writePixel##psm##_0(pstart, (j+1)%2048, i%2048, pbuf[1], gs.dstbuf.bw); \
//				\
//				if( widthlimit > 2 ) { \
//					writePixel##psm##_0(pstart, (j+2)%2048, i%2048, pbuf[2], gs.dstbuf.bw); \
//					\
//					if( widthlimit > 3 ) { \
//						writePixel##psm##_0(pstart, (j+3)%2048, i%2048, pbuf[3], gs.dstbuf.bw); \
//					} \
//				} \
//			} \
//		} \
//		\
//		if( j >= gs.imageEndX ) { assert(j == gs.imageEndX); j = gs.trxpos.dx; } \
//		else { assert( nSize == 0 ); goto End; } \
//	} \
//} \
//
//#define TRANSFERHOSTLOCAL_4(psm) { \
//	const u8* pbuf = (const u8*)pbyMem; \
//	u32 nSize = nQWordSize*8; \
//	for(; i < gs.imageEndY; ++i) { \
//		for(; j < gs.imageEndX && nSize > 0; j += 8, nSize -= 8) { \
//			/* write as many pixel at one time as possible */ \
//			writePixel##psm##_0(pstart, j%2048, i%2048, *pbuf&0x0f, gs.dstbuf.bw); \
//			writePixel##psm##_0(pstart, (j+1)%2048, i%2048, *pbuf>>4, gs.dstbuf.bw); \
//			pbuf++; \
//			writePixel##psm##_0(pstart, (j+2)%2048, i%2048, *pbuf&0x0f, gs.dstbuf.bw); \
//			writePixel##psm##_0(pstart, (j+3)%2048, i%2048, *pbuf>>4, gs.dstbuf.bw); \
//			pbuf++; \
//			writePixel##psm##_0(pstart, (j+4)%2048, i%2048, *pbuf&0x0f, gs.dstbuf.bw); \
//			writePixel##psm##_0(pstart, (j+5)%2048, i%2048, *pbuf>>4, gs.dstbuf.bw); \
//			pbuf++; \
//			writePixel##psm##_0(pstart, (j+6)%2048, i%2048, *pbuf&0x0f, gs.dstbuf.bw); \
//			writePixel##psm##_0(pstart, (j+7)%2048, i%2048, *pbuf>>4, gs.dstbuf.bw); \
//			pbuf++; \
//		} \
//		\
//		if( j >= gs.imageEndX ) { /*assert(j == gs.imageEndX);*/ j = gs.trxpos.dx; } \
//		else { assert( nSize == 0 ); goto End; } \
//	} \
//} \
//
//	switch (gs.dstbuf.psm) {
//		case 0x0:  TRANSFERHOSTLOCAL(32, u32, 2); break;
//		case 0x1:  TRANSFERHOSTLOCAL(24, u32, 4); break;
//		case 0x2:  TRANSFERHOSTLOCAL(16, u16, 4); break;
//		case 0xA:  TRANSFERHOSTLOCAL(16S, u16, 4); break;
//		case 0x13:
//			if( ((gs.imageEndX-gs.trxpos.dx)%4) ) {
//				TRANSFERHOSTLOCAL(8, u8, 1);
//			}
//			else {
//				TRANSFERHOSTLOCAL(8, u8, 4);
//			}
//			break;
//
//		case 0x14:
////			if( (gs.imageEndX-gs.trxpos.dx)%8 ) {
////				// hack
////				if( abs((int)nQWordSize*8 - (gs.imageEndY-i)*(gs.imageEndX-gs.trxpos.dx)+(j-gs.trxpos.dx)) <= 8 ) {
////					// don't transfer
////					DEBUG_LOG("bad texture 4: %d %d %d\n", gs.trxpos.dx, gs.imageEndX, nQWordSize);
////					gs.imageEndX = gs.trxpos.dx + (gs.imageEndX-gs.trxpos.dx)&~7;
////					//i = gs.imageEndY;
////					//goto End;
////					gs.imageTransfer = -1;
////				}
////			}
//			TRANSFERHOSTLOCAL_4(4);
//			break;
//		case 0x1B: TRANSFERHOSTLOCAL(8H, u8, 4); break;
//		case 0x24: TRANSFERHOSTLOCAL_4(4HL); break;
//		case 0x2C: TRANSFERHOSTLOCAL_4(4HH); break;
//		case 0x30: TRANSFERHOSTLOCAL(32Z, u32, 2); break;
//		case 0x31: TRANSFERHOSTLOCAL(24Z, u32, 4); break;
//		case 0x32: TRANSFERHOSTLOCAL(16Z, u16, 4); break;
//		case 0x3A: TRANSFERHOSTLOCAL(16SZ, u16, 4); break;
//	}
//
//End:
//	if( i >= gs.imageEndY ) {
//		assert( i == gs.imageEndY );
//		gs.imageTransfer = -1;
//
//		if( g_bSaveTrans ) {
//			tex0Info t;
//			t.tbp0 = gs.dstbuf.bp;
//			t.tw = gs.imageWnew;
//			t.th = gs.imageHnew;
//			t.tbw = gs.dstbuf.bw;
//			t.psm = gs.dstbuf.psm;
//			SaveTex(&t, 0);
//		}
//	}
//	else {
//		/* update new params */
//		gs.imageY = i;
//		gs.imageX = j;
//	}
//}

void InitTransferLocalHost()
{
	assert( gs.trxpos.sx+gs.imageWnew <= 2048 && gs.trxpos.sy+gs.imageHnew <= 2048 );

#ifndef RELEASE_TO_PUBLIC
	if( gs.trxpos.sx+gs.imageWnew > gs.srcbuf.bw )
		WARN_LOG("Transfer error, width exceeds\n");
#endif

	gs.imageX = gs.trxpos.sx;
	gs.imageY = gs.trxpos.sy;
	gs.imageEndX = gs.imageX + gs.imageWnew;
	gs.imageEndY = gs.imageY + gs.imageHnew;

	int start, end;
	GetRectMemAddress(start, end, gs.srcbuf.psm, gs.trxpos.sx, gs.trxpos.sy, gs.imageWnew, gs.imageHnew, gs.srcbuf.bp, gs.srcbuf.bw);
	ResolveInRange(start, end);
}

// left/right, top/down
void TransferLocalHost(void* pbyMem, u32 nQWordSize)
{
	assert( gs.imageTransfer == 1 );

	u8* pstart = g_pbyGSMemory + 256*gs.srcbuf.bp;
	int i = gs.imageY, j = gs.imageX;

#define TRANSFERLOCALHOST(psm, T) { \
	T* pbuf = (T*)pbyMem; \
	u32 nSize = nQWordSize*16/sizeof(T); \
	for(; i < gs.imageEndY; ++i) { \
		for(; j < gs.imageEndX && nSize > 0; ++j, --nSize) { \
			*pbuf++ = readPixel##psm##_0(pstart, j%2048, i%2048, gs.srcbuf.bw); \
		} \
		\
		if( j >= gs.imageEndX ) { assert( j == gs.imageEndX); j = gs.trxpos.sx; } \
		else { assert( nSize == 0 ); break; } \
	} \
} \

#define TRANSFERLOCALHOST_24(psm) { \
	u8* pbuf = (u8*)pbyMem; \
	u32 nSize = nQWordSize*16/3; \
	for(; i < gs.imageEndY; ++i) { \
		for(; j < gs.imageEndX && nSize > 0; ++j, --nSize) { \
			u32 p = readPixel##psm##_0(pstart, j%2048, i%2048, gs.srcbuf.bw); \
			pbuf[0] = (u8)p; \
			pbuf[1] = (u8)(p>>8); \
			pbuf[2] = (u8)(p>>16); \
			pbuf += 3; \
		} \
		\
		if( j >= gs.imageEndX ) { assert( j == gs.imageEndX); j = gs.trxpos.sx; } \
		else { assert( nSize == 0 ); break; } \
	} \
} \

	switch (gs.srcbuf.psm) {
		case 0x0:  TRANSFERLOCALHOST(32, u32); break;
		case 0x1:  TRANSFERLOCALHOST_24(24); break;
		case 0x2:  TRANSFERLOCALHOST(16, u16); break;
		case 0xA:  TRANSFERLOCALHOST(16S, u16); break;
		case 0x13: TRANSFERLOCALHOST(8, u8); break;
		case 0x1B: TRANSFERLOCALHOST(8H, u8); break;
		case 0x30: TRANSFERLOCALHOST(32Z, u32); break;
		case 0x31: TRANSFERLOCALHOST_24(24Z); break;
		case 0x32: TRANSFERLOCALHOST(16Z, u16); break;
		case 0x3A: TRANSFERLOCALHOST(16SZ, u16); break;
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
			(*it)->status |= CRenderTarget::TS_NeedUpdate;
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

#if !defined(RELEASE_TO_PUBLIC) && defined(PCSX2_DEBUG)
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
	if( m_Blocks[psm].bpp == 0 ) {
		ERROR_LOG("ZeroGS: Bad psm 0x%x\n", psm);
		start = 0;
		end = 0x00400000;
		return;
	}
		
	if( (psm&0x30) == 0x30 || psm == 0xa ) {

		const BLOCK& b = m_Blocks[psm];

		bw = (bw + b.width -1)/b.width;
		start = bp*256 + ((y/b.height) * bw + (x/b.width) )*0x2000;
		end = bp*256  + (((y+h-1)/b.height) * bw + (x + w + b.width - 1)/b.width)*0x2000;
	}
	else {
		// just take the addresses
		switch(psm) {
			case 0x00:
			case 0x01:
			case 0x1b:
			case 0x24:
			case 0x2c:
				start = 4*getPixelAddress32(x, y, bp, bw);
				end = 4*getPixelAddress32(x+w-1, y+h-1, bp, bw) + 4;
				break;
			case 0x02:
				start = 2*getPixelAddress16(x, y, bp, bw);
				end = 2*getPixelAddress16(x+w-1, y+h-1, bp, bw)+2;
				break;
			case 0x13:
				start = getPixelAddress8(x, y, bp, bw);
				end = getPixelAddress8(x+w-1, y+h-1, bp, bw)+1;
				break;
			case 0x14:
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

void _Resolve(const D3DLOCKED_RECT& locksrc, int fbp, int fbw, int fbh, int psm, u32 fbm)
{
	s_nResolved += 2;

	// align the rect to the nearest page
	// note that fbp is always aligned on page boundaries
	int start, end;
	GetRectMemAddress(start, end, psm, 0, 0, fbw, fbh, fbp, fbw);

	PRIM_LOG("resolve: %x %x %x (%x-%x)\n", fbp, fbw, fbh, start, end);

	int i, j;
	short smask1 = gs.smask&1;
	short smask2 = gs.smask&2;
	u32 mask, imask;

	if( psm&2 ) { // 16 bit
		// mask is shifted
		imask = RGBA32to16(fbm);
		mask = (~imask)&0xffff;
	}
	else {
		mask = ~fbm;
		imask = fbm;

		if( (psm&0xf)>0 ) {
			// preserve the alpha?
			mask &= 0x00ffffff;
			imask |= 0xff000000;
		}
	}
	
#define RESOLVE_32BIT(psm, T, Tsrc, blockbits, blockwidth, blockheight, convfn, frame, aax, aay) \
	{ \
		Tsrc* src = (Tsrc*)locksrc.pBits; \
		T* pPageOffset = (T*)g_pbyGSMemory + fbp*(256/sizeof(T)), *dst; \
		int srcpitch = locksrc.Pitch * blockheight/sizeof(Tsrc); \
		int maxfbh = (0x00400000-fbp*256) / (sizeof(T) * fbw); \
		if( maxfbh > fbh ) maxfbh = fbh; \
		for(i = 0; i < (maxfbh&~(blockheight-1)); i += blockheight) { \
			/*if( smask2 && (i&1) == smask1 ) continue; */ \
			for(j = 0; j < fbw; j += blockwidth) { \
				/* have to write in the tiled format*/ \
				##frame##SwizzleBlock##blockbits##(pPageOffset + getPixelAddress##psm##_0(j, i, fbw), \
					src+(j<<aax), locksrc.Pitch/sizeof(Tsrc), mask); \
			} \
			src += (srcpitch<<aay); \
		} \
		for(; i < maxfbh; ++i) { \
			for(j = 0; j < fbw; ++j) { \
				T dsrc = convfn(src[j<<aax]); \
				dst = pPageOffset + getPixelAddress##psm##_0(j, i, fbw); \
				*dst = (dsrc & mask) | (*dst & imask); \
			} \
			src += (locksrc.Pitch<<aay)/sizeof(Tsrc); \
		} \
	} \

	if( g_RenderFormat == D3DFMT_A8R8G8B8 ) {
		// start the conversion process A8R8G8B8 -> psm
		switch(psm) {
			case PSMCT32:
			case PSMCT24:
				if( s_AAy ) {
					RESOLVE_32BIT(32, u32, u32, 32A4, 8, 8, (u32), Frame, 1, 1);
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
					RESOLVE_32BIT(16, u16, u32, 16A4, 16, 8, RGBA32to16, Frame, 1, 1);
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
					RESOLVE_32BIT(16S, u16, u32, 16A4, 16, 8, RGBA32to16, Frame, 1, 1);
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
					RESOLVE_32BIT(32Z, u32, u32, 32A4, 8, 8, (u32), Frame, 1, 1);
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
					RESOLVE_32BIT(16Z, u16, u32, 16A4, 16, 8, (u16), Frame, 1, 1);
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
					RESOLVE_32BIT(16SZ, u16, u32, 16A4, 16, 8, (u16), Frame, 1, 1);
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
	else {
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

////////////
// Saving //
////////////
void SaveTex(tex0Info* ptex, int usevid)
{
	LPD3DTEX pTexture;
	pd3dDevice->CreateTexture(ptex->tw, ptex->th, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_SYSTEMMEM, &pTexture, NULL);
	
	D3DLOCKED_RECT lockdst;
	pTexture->LockRect(0, &lockdst, NULL, 0);

	DWORD* dst = (DWORD*)lockdst.pBits;
	u8* psrc = g_pbyGSMemory;

	CMemoryTarget* pmemtarg = NULL;

	if( usevid ) {
		pmemtarg = g_MemTargs.GetMemoryTarget(*ptex, 1);
		assert( pmemtarg != NULL );
		D3DLOCKED_RECT lock;
		HRESULT hr = pmemtarg->ptexsys->LockRect(0, &lock, NULL, D3DLOCK_READONLY);
		if( FAILED(hr) )
			return;
		u32 offset = pmemtarg->realy * 4 * GPU_TEXWIDTH;
		if( ptex->psm == PSMT8 ) offset *= ptex->cpsm <= 1 ? 4 : 2;
		else if( ptex->psm == PSMT4 ) offset *= ptex->cpsm <= 1 ? 8 : 4;

		psrc = (u8*)lock.pBits - offset;
	}

	for(int i = 0; i < ptex->th; ++i) {
		for(int j = 0; j < ptex->tw; ++j) {
			u32 u, addr;
			switch(ptex->psm) {
				case PSMCT32:
					addr = getPixelAddress32(j, i, ptex->tbp0, ptex->tbw);

					if( addr*4 < 0x00400000 )
						u = readPixel32(psrc, j, i, ptex->tbp0, ptex->tbw);
					else u = 0;

					break;
				case PSMCT24:
					addr = getPixelAddress24(j, i, ptex->tbp0, ptex->tbw);

					if( addr*4 < 0x00400000 )
						u = readPixel24(psrc, j, i, ptex->tbp0, ptex->tbw);
					else u = 0;

					break;
				case PSMCT16:
					addr = getPixelAddress16(j, i, ptex->tbp0, ptex->tbw);

					if( addr*2 < 0x00400000 ) {
						u = readPixel16(psrc, j, i, ptex->tbp0, ptex->tbw);
						u = RGBA16to32(u);
					}
					else u = 0;
					
					break;
				case PSMCT16S:
					addr = getPixelAddress16(j, i, ptex->tbp0, ptex->tbw);

					if( addr*2 < 0x00400000 ) {
						u = readPixel16S(psrc, j, i, ptex->tbp0, ptex->tbw);
						u = RGBA16to32(u);
					}
					else u = 0;
					break;

				case PSMT8:
					addr = getPixelAddress8(j, i, ptex->tbp0, ptex->tbw);

					if( addr < 0x00400000 ) {
						if( usevid ) {
							if( ptex->cpsm <= 1 ) u = *(u32*)(psrc+4*addr);
							else u = RGBA16to32(*(u16*)(psrc+2*addr));
						}
						else
							u = readPixel8(psrc, j, i, ptex->tbp0, ptex->tbw);
					}
					else u = 0;
					break;

				case PSMT4:
					addr = getPixelAddress4(j, i, ptex->tbp0, ptex->tbw);

					if( addr < 2*0x00400000 ) {
						if( usevid ) {
							if( ptex->cpsm <= 1 ) u = *(u32*)(psrc+4*addr);
							else u = RGBA16to32(*(u16*)(psrc+2*addr));
						}
						else
							u = readPixel4(psrc, j, i, ptex->tbp0, ptex->tbw);
					}
					else u = 0;
					break;

				case PSMT8H:
					addr = getPixelAddress8H(j, i, ptex->tbp0, ptex->tbw);

					if( 4*addr < 0x00400000 ) {
						if( usevid ) {
							if( ptex->cpsm <= 1 ) u = *(u32*)(psrc+4*addr);
							else u = RGBA16to32(*(u16*)(psrc+2*addr));
						}
						else
							u = readPixel8H(psrc, j, i, ptex->tbp0, ptex->tbw);
					}
					else u = 0;

					break;

				case PSMT4HL:
					addr = getPixelAddress4HL(j, i, ptex->tbp0, ptex->tbw);

					if( 4*addr < 0x00400000 ) {
						if( usevid ) {
							if( ptex->cpsm <= 1 ) u = *(u32*)(psrc+4*addr);
							else u = RGBA16to32(*(u16*)(psrc+2*addr));
						}
						else
							u = readPixel4HL(psrc, j, i, ptex->tbp0, ptex->tbw);
					}
					else u = 0;
					break;

				case PSMT4HH:
					addr = getPixelAddress4HH(j, i, ptex->tbp0, ptex->tbw);

					if( 4*addr < 0x00400000 ) {
						if( usevid ) {
							if( ptex->cpsm <= 1 ) u = *(u32*)(psrc+4*addr);
							else u = RGBA16to32(*(u16*)(psrc+2*addr));
						}
						else
							u = readPixel4HH(psrc, j, i, ptex->tbp0, ptex->tbw);
					}
					else u = 0;
					break;

				case PSMT32Z:
					addr = getPixelAddress32Z(j, i, ptex->tbp0, ptex->tbw);

					if( 4*addr < 0x00400000 )
						u = readPixel32Z(psrc, j, i, ptex->tbp0, ptex->tbw);
					else u = 0;
					break;

				case PSMT24Z:
					addr = getPixelAddress24Z(j, i, ptex->tbp0, ptex->tbw);

					if( 4*addr < 0x00400000 )
						u = readPixel24Z(psrc, j, i, ptex->tbp0, ptex->tbw);
					else u = 0;
					break;

				case PSMT16Z:
					addr = getPixelAddress16Z(j, i, ptex->tbp0, ptex->tbw);

					if( 2*addr < 0x00400000 )
						u = readPixel16Z(psrc, j, i, ptex->tbp0, ptex->tbw);
					else u = 0;
					break;

				case PSMT16SZ:
					addr = getPixelAddress16SZ(j, i, ptex->tbp0, ptex->tbw);

					if( 2*addr < 0x00400000 )
						u = readPixel16SZ(psrc, j, i, ptex->tbp0, ptex->tbw);
					else u = 0;
					break;

				default:
					assert(0);
			}

			*dst++ = u;
		}

		dst += lockdst.Pitch/4 - ptex->tw;
	}

	pTexture->UnlockRect(0);

	if( usevid ) {
		pmemtarg->ptexsys->UnlockRect(0);
	}

	D3DXSaveTextureToFile("tex.tga", D3DXIFF_TGA, pTexture, NULL);
	
	SAFE_RELEASE(pTexture);

}

}
