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
#include <dxerr9.h>
#include <aviUtil.h>
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

#include "zerogsshaders/zerogsshaders.h"
#include "targets.h"

#define DEBUG_PS2		0

#define POINT_BUFFERFLUSH				512
#define POINT_BUFFERSIZE				(1<<18)

#define MINMAX_SHIFT	3
#define MAX_ACTIVECLUTS					16

#define ZEROGS_SAVEVER 0xaa000005

#define STENCIL_ALPHABIT	1		// if set, dest alpha >= 0x80
#define STENCIL_PIXELWRITE	2		// if set, pixel just written (reset after every Flush)
#define STENCIL_FBA			4		// if set, just written pixel's alpha >= 0 (reset after every Flush)
#define STENCIL_SPECIAL		8		// if set, indicates that pixel passed its alpha test (reset after every Flush)
//#define STENCIL_PBE			16
#define STENCIL_CLEAR		(2|4|8|16)

#define VBSAVELIMIT ((u32)((u8*)&vb[0].nNextFrameHeight-(u8*)&vb[0]))

using namespace ZeroGS;

static LPDIRECT3D9 pD3D = NULL;		// Used to create the D3DDevice
LPDIRECT3DDEVICE9 pd3dDevice = NULL;
static DXVEC4 s_vznorm;
extern u32 g_nGenVars, g_nTexVars, g_nAlphaVars, g_nResolve;
extern char *libraryName;
extern int g_nFrame, g_nRealFrame;
extern float fFPS;
extern unsigned char revision, build, minor;

BOOL g_bDisplayMsg = 1;

extern HINSTANCE hInst;

BOOL g_bSaveFlushedFrame = 1;
BOOL g_bIsLost = 0;
int g_nFrameRender = 10;
int g_nFramesSkipped = 0;
int g_MaxRenderedHeight = 0;

#ifdef RELEASE_TO_PUBLIC

#define INC_GENVARS()
#define INC_TEXVARS()
#define INC_ALPHAVARS()
#define INC_RESOLVE()

#define g_bUpdateEffect 0
#define g_bWriteProfile 0
#define g_bSaveTex 0
#define g_bSaveTrans 0
#define g_bSaveFrame 0
#define g_bSaveFinalFrame 0
#define g_bUpdateStencil 1
#define g_bSaveResolved 0

#else

#define INC_GENVARS() ++g_nGenVars
#define INC_TEXVARS() ++g_nTexVars
#define INC_ALPHAVARS() ++g_nAlphaVars
#define INC_RESOLVE() ++g_nResolve

BOOL g_bSaveTrans = 0;
BOOL g_bUpdateEffect = 0;
BOOL g_bWriteProfile = 0;
BOOL g_bSaveTex = 0;	// saves the curent texture
BOOL g_bSaveFrame = 0;	// saves the current psurfTarget
BOOL g_bSaveFinalFrame = 0; // saves the input to the CRTC
BOOL g_bUpdateStencil = 1;		// only needed for dest alpha test (unfortunately, it has to be on all the time)	
BOOL g_bSaveResolved = 0;

#endif

#define DRAW() pd3dDevice->DrawPrimitive(primtype[curvb.curprim.prim], 0, curvb.dwCount)

extern int s_frameskipping;
			
//inline void SetRenderTarget_(int index, LPD3DSURF psurf, int counter, const char* pname)
//{
//	static LPD3DSURF ptargs[4] = {NULL};
//	static int counters[4] = {0};
//	static const char* pnames[4] = {NULL};
//
//	if( ptargs[index] == psurf && psurf != NULL )
//		DEBUG_LOG("duplicate targets\n");
//	pd3dDevice->SetRenderTarget(index, psurf);
//	ptargs[index] = psurf;
//	counters[index] = counter;
//	pnames[index] = pname;
//}
//
//#define SetRenderTarget(index, psurf) SetRenderTarget_(index, psurf, __COUNTER__, __FUNCTION__)

static u32 g_SaveFrameNum = 0;
BOOL g_bMakeSnapshot = 0;
string strSnapshot;

int GPU_TEXWIDTH = 512;
float g_fiGPU_TEXWIDTH = 1/512.0f;

int g_MaxTexWidth = 4096, g_MaxTexHeight = 4096;

// AVI Capture
static int s_aviinit = 0;
static int s_avicapturing = 0;
static LPD3DSURF s_ptexAVICapture = NULL; // system memory texture

const u32 g_primmult[8] = { 1, 2, 2, 3, 3, 3, 2, 0xff };
const u32 g_primsub[8] = { 1, 2, 1, 3, 1, 1, 2, 0 };

inline DWORD FtoDW(float f) { return (*((DWORD*)&f)); }

float g_fBlockMult = 1;
static int s_nFullscreen = 0;
int g_nDepthUpdateCount = 0;
int g_nDepthBias = 0;

// Consts
static const D3DPRIMITIVETYPE primtype[8] = { D3DPT_POINTLIST, D3DPT_LINELIST, D3DPT_LINELIST, D3DPT_TRIANGLELIST, D3DPT_TRIANGLELIST, D3DPT_TRIANGLELIST, D3DPT_TRIANGLELIST, D3DPT_FORCE_DWORD };
static const DWORD blendalpha[3] = { D3DBLEND_SRCALPHA, D3DBLEND_DESTALPHA, D3DBLEND_BLENDFACTOR };
static const DWORD blendinvalpha[3] = { D3DBLEND_INVSRCALPHA, D3DBLEND_INVDESTALPHA, D3DBLEND_INVBLENDFACTOR };

static const int PRIMMASK = 0x0e;	// for now ignore 0x10 (AA)

static const DWORD g_dwAlphaCmp[] = { D3DCMP_NEVER, D3DCMP_ALWAYS, D3DCMP_LESS, D3DCMP_LESSEQUAL, 
								D3DCMP_EQUAL, D3DCMP_GREATEREQUAL, D3DCMP_GREATER, D3DCMP_NOTEQUAL };

// used for afail case
static const DWORD g_dwReverseAlphaCmp[] = { D3DCMP_ALWAYS, D3DCMP_NEVER, D3DCMP_GREATEREQUAL, D3DCMP_GREATER, 
									D3DCMP_NOTEQUAL, D3DCMP_LESS, D3DCMP_LESSEQUAL, D3DCMP_EQUAL };

static const DWORD g_dwZCmp[] = { D3DCMP_NEVER, D3DCMP_ALWAYS, D3DCMP_GREATEREQUAL, D3DCMP_GREATER };

/////////////////////
// graphics resources
static LPD3DDECL pdecl = NULL;
static LPD3DVS pvs[16] = {NULL};
static LPD3DPS ppsRegular[4] = {NULL}, ppsTexture[NUM_SHADERS] = {NULL};
static LPD3DPS ppsCRTC[2] = {NULL}, ppsCRTC24[2] = {NULL}, ppsCRTCTarg[2] = {NULL};

int g_nPixelShaderVer = SHADER_30; // default

static BYTE* s_lpShaderResources = NULL;
static map<int, SHADERHEADER*> mapShaderResources;

LPD3DTEX s_ptexCurSet[2] = {NULL};

#define s_bForceTexFlush 1
static LPD3DTEX s_ptexNextSet[2] = {NULL};

static ID3DXFont* pFont = NULL;
static ID3DXSprite* pSprite = NULL;

static LPD3DSURF psurfOrgTarg = NULL, psurfOrgDepth = NULL;

LPD3DTEX ptexBlocks = NULL, ptexConv16to32 = NULL;		// holds information on block tiling
LPD3DTEX ptexBilinearBlocks = NULL;
IDirect3DVolumeTexture9* ptexConv32to16 = NULL;
static LPD3DTEX s_ptexInterlace = NULL;			// holds interlace fields
static int s_nInterlaceTexWidth = 0;					// width of texture
static list<LPD3DTEX> s_vecTempTextures;			// temporary textures, released at the end of every frame

static BOOL s_bTexFlush = FALSE;
static LPD3DTEX ptexLogo = NULL;
static BOOL s_bWriteDepth = FALSE;
static BOOL s_bDestAlphaTest = FALSE;
static int s_nLastResolveReset = 0;
static int s_nResolveCounts[30] = {0}; // resolve counts for last 30 frames
static int s_nCurResolveIndex = 0;
int s_nResolved = 0; // number of targets resolved this frame 
int g_nDepthUsed = 0; // ffx2 pal movies
static int s_nWriteDepthCount = 0;
static int s_nWireframeCount = 0;
static int s_nWriteDestAlphaTest = 0;

////////////////////
// State parameters
static float fiRendWidth, fiRendHeight;	

static DWORD dwStencilRef, dwStencilMask;

static DXVEC4 vAlphaBlendColor;		// used for GPU_COLOR

static BYTE bNeedBlendFactorInAlpha;		// set if the output source alpha is different from the real source alpha (only when blend factor > 0x80)
static DWORD s_dwColorWrite = 0xf;			// the color write mask of the current target

BOOL g_bDisplayFPS = FALSE;

union {
	struct {
		BYTE _bNeedAlphaColor;		// set if vAlphaBlendColor needs to be set
		BYTE _b2XAlphaTest;			// Only valid when bNeedAlphaColor is set. if 1st bit set set, double all alpha testing values
									// otherwise alpha testing needs to be done separately.
		BYTE _bDestAlphaColor;		// set to 1 if blending with dest color (process only one tri at a time). If 2, dest alpha is always 1.
		BYTE _bAlphaClamping;		// if first bit is set, do min; if second bit, do max
	};
	u32 _bAlphaState;
} g_vars;

#define bNeedAlphaColor g_vars._bNeedAlphaColor
#define b2XAlphaTest g_vars._b2XAlphaTest
#define bDestAlphaColor g_vars._bDestAlphaColor
#define bAlphaClamping g_vars._bAlphaClamping

int g_PrevBitwiseTexX = -1, g_PrevBitwiseTexY = -1; // textures stored in SAMP_BITWISEANDX and SAMP_BITWISEANDY

// stores the buffers for the last RenderCRTC
const float g_filog32 = 0.999f / (32.0f * logf(2.0f));

static BOOL s_bAlphaSet = FALSE;
static alphaInfo s_alphaInfo;

namespace ZeroGS
{
	VB vb[2];
	float fiTexWidth[2], fiTexHeight[2];	// current tex width and height
	LONG width, height;
	u8* g_pbyGSMemory = NULL;	// 4Mb GS system mem
	u8* g_pbyGSClut = NULL;

	D3DPRESENT_PARAMETERS d3dpp; 

	BYTE s_AAx = 0, s_AAy = 0; // if AAy is set, then AAx has to be set
	BYTE bIndepWriteMasks = 1;
	BOOL s_bBeginScene = FALSE;
	D3DFORMAT g_RenderFormat = D3DFMT_A16B16G16R16F;
	int icurctx = -1;

	LPD3DVB pvbRect = NULL;

	DXVEC4 g_vdepth = DXVEC4(65536.0f, 256.0f, 1.0f, 65536.0f*256.0f);

	LPD3DVS pvsBitBlt = NULL, pvsBitBlt30 = NULL;
	LPD3DPS ppsBitBlt[2] = {NULL}, ppsBitBltDepth[2] = {NULL}, ppsBitBltDepthTex[2] = {NULL}, ppsOne = NULL;
	LPD3DPS ppsBaseTexture = NULL, ppsConvert16to32 = NULL, ppsConvert32to16 = NULL;

	extern CRangeManager s_RangeMngr; // manages overwritten memory
	void FlushTransferRanges(const tex0Info* ptex);

	// returns the first and last addresses aligned to a page that cover 
	void GetRectMemAddress(int& start, int& end, int psm, int x, int y, int w, int h, int bp, int bw);

	HRESULT LoadEffects();
	HRESULT LoadExtraEffects();
	LPD3DPS LoadShadeEffect(int type, int texfilter, int fog, int testaem, int exactcolor, const clampInfo& clamp, int context);

	static int s_nNewWidth = -1, s_nNewHeight = -1;
	void ChangeDeviceSize(int nNewWidth, int nNewHeight);

	void ProcessMessages();
	void RenderCustom(float fAlpha); // intro anim

	struct MESSAGE
	{
		MESSAGE() {}
		MESSAGE(const char* p, DWORD dw) { strcpy(str, p); dwTimeStamp = dw; }
		char str[255];
		DWORD dwTimeStamp;
	};

	static list<MESSAGE> listMsgs;

	///////////////////////
	// Method Prototypes //
	///////////////////////

	void AdjustTransToAspect(DXVEC4& v, int dispwidth, int dispheight);

	void KickPoint();
	void KickLine();
	void KickTriangle();
	void KickTriangleFan();
	void KickSprite();
	void KickDummy();

	inline void SetContextTarget(int context);

	// use to update the d3d state
	void SetTexVariables(int context);
	void SetAlphaVariables(const alphaInfo& ainfo);
	void ResetAlphaVariables();

	__forceinline void SetAlphaTestInt(pixTest curtest);

	__forceinline void RenderAlphaTest(const VB& curvb);
	__forceinline void RenderStencil(const VB& curvb, DWORD dwUsingSpecialTesting);
	__forceinline void ProcessStencil(const VB& curvb);
	__forceinline void RenderFBA(const VB& curvb);
	__forceinline void ProcessFBA(const VB& curvb);

	void ResolveInRange(int start, int end);

	void ExtWrite();

	inline LPD3DTEX CreateInterlaceTex(int width) {
		if( width == s_nInterlaceTexWidth && s_ptexInterlace != NULL ) return s_ptexInterlace;

		SAFE_RELEASE(s_ptexInterlace);
		s_nInterlaceTexWidth = width;

		HRESULT hr;
		V(pd3dDevice->CreateTexture(width, 1, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &s_ptexInterlace, NULL));

		D3DLOCKED_RECT lock;
		s_ptexInterlace->LockRect(0, &lock, NULL, 0);
		for(int i = 0; i < width; ++i) ((u32*)lock.pBits)[i] = (i&1) ? 0xffffffff : 0;
		s_ptexInterlace->UnlockRect(0);

		return s_ptexInterlace;
	}

	DrawFn drawfn[8] = { KickDummy, KickDummy, KickDummy, KickDummy,
						KickDummy, KickDummy, KickDummy, KickDummy };

}; // end namespace

///////////////////
// Context State //
///////////////////
ZeroGS::VB::VB()
{
	memset(this, 0, sizeof(VB));
	tex0.tw = 1;
	tex0.th = 1;
}

ZeroGS::VB::~VB()
{
	Destroy();
}

void ZeroGS::VB::Destroy()
{
	Unlock();
	SAFE_RELEASE(pvb);
	
	prndr = NULL;
	pdepth = NULL;
}

void ZeroGS::VB::Lock()
{
	assert(pvb != NULL);
	if( pbuf == NULL )
	{
		if( dwCurOff+POINT_BUFFERFLUSH > POINT_BUFFERSIZE ) dwCurOff = 0;

		pvb->Lock(dwCurOff*sizeof(VertexGPU), sizeof(VertexGPU)*POINT_BUFFERFLUSH, (void**)&pbuf, dwCurOff ? D3DLOCK_NOOVERWRITE|D3DLOCK_NOSYSLOCK : D3DLOCK_DISCARD|D3DLOCK_NOSYSLOCK);
		dwCount = 0;
		assert( pbuf != NULL );
	}
}

bool ZeroGS::VB::CheckPrim()
{
	Lock();

	if( (PRIMMASK & prim->_val) != (PRIMMASK & curprim._val) || primtype[prim->prim] != primtype[curprim.prim] )
		return dwCount > 0;

	return false;
}

// upper bound on max possible height
#define GET_MAXHEIGHT(fbp, fbw, psm) ((((0x00100000-64*(fbp))/(fbw))&~0x1f)<<((psm&2)?1:0))

#include <set>
static int maxmin = 608;
//static set<int> s_setFBP[2]; // previous frame/zbuf pointers for the last 2 frames
//static int s_nCurFBPSet = 0;
//static map<int, int> s_mapFrameHeights[2];
//static int s_nCurFrameMap = 0;

// a lot of times, target is too big and overwrites the texture using, if tbp != 0, use it to bound
void ZeroGS::VB::CheckFrame(int tbp)
{
	static int bChanged;

	if( bNeedZCheck ) {
		PRIM_LOG("zbuf_%d: zbp=0x%x psm=0x%x, zmsk=%d\n", ictx, zbuf.zbp, zbuf.psm, zbuf.zmsk);
		//zbuf = *zb;
	}

	// invalid bpp
	if( m_Blocks[gsfb.psm].bpp == 0 ) {
		ERROR_LOG("CheckFrame invalid bpp %d\n", gsfb.psm);
		return;
	}

	bChanged = 0;

	if( gsfb.fbw <= 0 ) {
		return;
	}

	if( bNeedFrameCheck ) {

		int maxpos = 0x00100000;

		// important to set before calling GetTarg
		bNeedFrameCheck = 0;
		bNeedZCheck = 0;

		// add constraints of other targets
		if( gsfb.fbw > 0 ) {
			maxpos = 0x00100000-64*gsfb.fbp;

			// make sure texture is far away from tbp
			if( gsfb.fbp < tbp && gsfb.fbp + 0x2000 < tbp) {
				maxpos = min(64*(tbp-gsfb.fbp), maxpos);
			}
			if( prndr != NULL ) {
				// offroad uses 0x80 fbp which messes up targets
				if( gsfb.fbp + 0x80 < frame.fbp ) {
					// special case when double buffering (hamsterball)
					maxpos = min(64*(frame.fbp-gsfb.fbp), maxpos);
				}
			}
			if( zbuf.zbp < tbp && !zbuf.zmsk ) {
				maxpos = min((tbp-zbuf.zbp)*((zbuf.psm&2)?128:64), maxpos);
			}

			// old caching method
			if( gsfb.fbp < zbuf.zbp && !zbuf.zmsk ) { // zmsk necessary for KH movie
				int temp = 64*(zbuf.zbp-gsfb.fbp);//min( (0x00100000-64*zbuf.zbp) , 64*(zbuf.zbp-gsfb.fbp) );
				maxpos = min(temp, maxpos);
			}

			maxpos /= gsfb.fbw;
			if( gsfb.psm & 2 )
				maxpos *= 2;

			maxpos = min(gsfb.fbh, maxpos);
			maxpos = min(maxmin, maxpos);
			//? atelier iris crashes without it
			if( maxpos > 256 )
				maxpos &= ~0x1f;
		}
		else {
			ERROR_LOG("render target null, ignoring\n");
			//prndr = NULL;
			//pdepth = NULL;
			return;
		}

		gsfb.psm &= 0xf; // shadow tower

		if( prndr != NULL ) {

			// render target
			if( prndr->psm != gsfb.psm ) {
				// behavior for dest alpha varies
				ResetAlphaVariables();
			}
		}
	
		int fbh = (scissor.y1>>MINMAX_SHIFT)+1;
		if( fbh > 2 && (fbh&1) ) fbh -= 1;

		if( !(gsfb.psm&2) || !(g_GameSettings&GAME_FULL16BITRES) ) {
			fbh = min(fbh, maxpos);
		}

		frame = gsfb;
		if (frame.fbw > 1024) frame.fbw = 1024;

//		if( fbh > 256 && (fbh % m_Blocks[gsfb.psm].height) <= 2 ) {
//			// dragon ball z
//			fbh -= fbh%m_Blocks[gsfb.psm].height;
//		}
		if( !(frame.psm&2) || !(g_GameSettings&GAME_FULL16BITRES) )
			frame.fbh = fbh;

		if( !(frame.psm&2) ) {//|| !(g_GameSettings&GAME_FULL16BITRES) ) {
			if( frame.fbh >= 512 ) {
				// neopets hack
				maxmin = min(maxmin, frame.fbh);
				frame.fbh = maxmin;
			}
		}

		// mgs3 hack to get proper resolution, targets after 0x2000 are usually feedback
		if( g_MaxRenderedHeight >= 0xe0 && frame.fbp >= 0x2000 ) {
			int considerheight = (g_MaxRenderedHeight/8+31)&~31;
			if( frame.fbh > considerheight )
				frame.fbh = considerheight;
			else if( frame.fbh <= 32 )
				frame.fbh = considerheight;
			
			if( frame.fbh == considerheight ) {
				// stops bad resolves (mgs3)
				if( !curprim.abe && (!test.ate || test.atst == 0) )
					s_nResolved |= 0x100;
			}
		}

		// ffxii hack to stop resolving
		if( !(frame.psm&2) || !(g_GameSettings&GAME_FULL16BITRES) ) {
			if( frame.fbp >= 0x3000 && fbh >= 0x1a0 ) {
				int endfbp = frame.fbp + frame.fbw*fbh/((gsfb.psm&2)?128:64);

				// see if there is a previous render target in the way, reduce
				for(CRenderTargetMngr::MAPTARGETS::iterator itnew = s_RTs.mapTargets.begin(); itnew != s_RTs.mapTargets.end(); ++itnew) {
					if( itnew->second->fbp > frame.fbp && endfbp > itnew->second->fbp ) {
						endfbp = itnew->second->fbp;
					}
				}

				frame.fbh = (endfbp-frame.fbp)*((gsfb.psm&2)?128:64)/frame.fbw;
			}
		}

		CRenderTarget* pprevrndr = prndr;
		CDepthTarget* pprevdepth = pdepth;

		// reset so that Resolve doesn't call Flush
		prndr = NULL;
		pdepth = NULL;

		CRenderTarget* pnewtarg = s_RTs.GetTarg(frame, 0, maxmin);
		assert( pnewtarg != NULL );

		// pnewtarg->fbh >= 0x1c0 needed for ffx
		if( pnewtarg->fbh >= 0x1c0 && pnewtarg->fbh > frame.fbh && zbuf.zbp < tbp && !zbuf.zmsk ) {
			// check if zbuf is in the way of the texture (suikoden5)
			int maxallowedfbh = (tbp-zbuf.zbp)*((zbuf.psm&2)?128:64) / gsfb.fbw;
			if( gsfb.psm & 2 )
				maxallowedfbh *= 2;

			if( pnewtarg->fbh > maxallowedfbh+32 ) { // +32 needed for ffx2
				// destroy and recreate
				s_RTs.DestroyAll(0, 0x100, pnewtarg->fbw);
				pnewtarg = s_RTs.GetTarg(frame, 0, maxmin);
				assert( pnewtarg != NULL );
			}
		}

		PRIM_LOG("frame_%d: fbp=0x%x fbw=%d fbh=%d(%d) psm=0x%x fbm=0x%x\n", ictx, gsfb.fbp, gsfb.fbw, gsfb.fbh, pnewtarg->fbh, gsfb.psm, gsfb.fbm);

		if( (pprevrndr != pnewtarg) || (prndr != NULL && (prndr->status & CRenderTarget::TS_NeedUpdate)) )
			bChanged = 1;

		prndr = pnewtarg;

		// update z
		frameInfo tempfb;
		tempfb.fbw = prndr->fbw;
		tempfb.fbp = zbuf.zbp;
		tempfb.psm = zbuf.psm;
		tempfb.fbh = prndr->targheight;
		if( zbuf.psm == 0x31 ) tempfb.fbm = 0xff000000;
		else tempfb.fbm = 0;

		// check if there is a target that exactly aligns with zbuf (zbuf can be cleared this way, gunbird 2)
		//u32 key = zbuf.zbp|(frame.fbw<<16);
		//CRenderTargetMngr::MAPTARGETS::iterator it = s_RTs.mapTargets.find(key);
//		if( it != s_RTs.mapTargets.end() ) {
//#ifdef _DEBUG
//			DEBUG_LOG("zbuf resolve\n");
//#endif
//			if( it->second->status & CRenderTarget::TS_Resolved )
//				it->second->Resolve();
//		}

		CDepthTarget* pnewdepth = (CDepthTarget*)s_DepthRTs.GetTarg(tempfb, CRenderTargetMngr::TO_DepthBuffer |
			CRenderTargetMngr::TO_StrictHeight|(zbuf.zmsk?CRenderTargetMngr::TO_Virtual:0),
			prndr->targheight);//GET_MAXHEIGHT(zbuf.zbp, gsfb.fbw, 0));
		assert( pnewdepth != NULL && prndr != NULL );
		assert( pnewdepth->fbh == prndr->targheight );

		if( (pprevdepth != pnewdepth) || (pdepth != NULL && (pdepth->status & CRenderTarget::TS_NeedUpdate)) )
			bChanged |= 2;

		pdepth = pnewdepth;

		if( prndr->status & CRenderTarget::TS_NeedConvert32) {
			if( pdepth->pdepth != NULL )
				pd3dDevice->SetDepthStencilSurface(pdepth->pdepth);
			prndr->fbh *= 2;
			prndr->targheight *= 2;
			prndr->ConvertTo32();
			prndr->status &= ~CRenderTarget::TS_NeedConvert32;
		}
		else if( prndr->status & CRenderTarget::TS_NeedConvert16 ) {
			if( pdepth->pdepth != NULL )
				pd3dDevice->SetDepthStencilSurface(pdepth->pdepth);
			prndr->fbh /= 2;
			prndr->targheight /= 2;
			prndr->ConvertTo16();
			prndr->status &= ~CRenderTarget::TS_NeedConvert16;
		}
	}
	else if( bNeedZCheck  ) {

		bNeedZCheck = 0;
		CDepthTarget* pprevdepth = pdepth;
		pdepth = NULL;

		if( prndr != NULL && gsfb.fbw > 0 ) {
			// just z changed
			frameInfo f;
			f.fbp = zbuf.zbp;
			f.fbw = prndr->fbw;
			f.fbh = prndr->fbh;
			f.psm = zbuf.psm;
			
			if( zbuf.psm == 0x31 ) f.fbm = 0xff000000;
			else f.fbm = 0;
			CDepthTarget* pnewdepth = (CDepthTarget*)s_DepthRTs.GetTarg(f, CRenderTargetMngr::TO_DepthBuffer|CRenderTargetMngr::TO_StrictHeight|
				(zbuf.zmsk?CRenderTargetMngr::TO_Virtual:0), prndr->fbh);//GET_MAXHEIGHT(zbuf.zbp, gsfb.fbw, 0));

			assert( pnewdepth != NULL && prndr != NULL );
			assert( pnewdepth->fbh == prndr->fbh );

			if( (pprevdepth != pnewdepth) || (pdepth != NULL && (pdepth->status & CRenderTarget::TS_NeedUpdate)) )
				bChanged = 2;

			pdepth = pnewdepth;
		}
	}
	
	s_nResolved &= 0xff; // restore

	if( prndr != NULL )
		SetContextTarget(ictx);

	//if( prndr != NULL && ictx == icurctx) 
	//else bVarsSetTarg = 0;

//	if( prndr != NULL && bChanged ) {
//		if( ictx == icurctx ) SetContextTarget(icurctx);
//		else
//			bVarsSetTarg = 0;
//	}
}

void ZeroGS::VB::FlushTexData()
{
	assert( bNeedTexCheck );

	bNeedTexCheck = 0;

	u32 psm = (uNextTex0Data[0] >> 20) & 0x3f;
	if( psm == 9 ) psm = 1; // hmm..., ffx intro menu

	// don't update unless necessary
	if( uCurTex0Data[0] == uNextTex0Data[0] && (uCurTex0Data[1]&0x1f) == (uNextTex0Data[1]&0x1f) ) {

		if( PSMT_ISCLUT(psm) ) {
			
			// have to write the CLUT again if changed
			if( (uCurTex0Data[1]&0x1fffffe0) == (uNextTex0Data[1]&0x1fffffe0) ) {

				if( uNextTex0Data[1]&0xe0000000 ) {
					//ZeroGS::Flush(ictx);
					ZeroGS::texClutWrite(ictx);
					// invalidate to make sure target didn't change!
					bVarsTexSync = FALSE;
				}

				return;
			}

			if( (uNextTex0Data[1]&0xe0000000) == 0 ) {

				if( (uCurTex0Data[1]&0x1ff10000) != (uNextTex0Data[1]&0x1ff10000) )
					ZeroGS::Flush(ictx);

				// clut isn't going to be loaded so can ignore, but at least update CSA and CPSM!
				uCurTex0Data[1] = (uCurTex0Data[1]&0xe087ffff)|(uNextTex0Data[1]&0x1f780000);

				if( tex0.cpsm <= 1 ) tex0.csa  =  (uNextTex0Data[1] >> 24) & 0xf;
				else tex0.csa  =  (uNextTex0Data[1] >> 24) & 0x1f;

				tex0.cpsm = (uNextTex0Data[1] >> 19) & 0xe;
				ZeroGS::texClutWrite(ictx);

				bVarsTexSync = FALSE;
				return;
			}

			// fall through
		}
		else {
			//bVarsTexSync = FALSE;
			return;
		}
	}

	ZeroGS::Flush(ictx);
	bVarsTexSync = FALSE;
	bTexConstsSync = FALSE;

	uCurTex0Data[0] = uNextTex0Data[0];
	uCurTex0Data[1] = uNextTex0Data[1];

	tex0.tbp0 =  (uNextTex0Data[0]		& 0x3fff);
	tex0.tbw  = ((uNextTex0Data[0] >> 14) & 0x3f) * 64;
	tex0.psm  =  psm;
	tex0.tw   =  (uNextTex0Data[0] >> 26) & 0xf;
	if (tex0.tw > 10) tex0.tw = 10;
	tex0.tw   = 1<<tex0.tw;
	tex0.th   = ((uNextTex0Data[0] >> 30) & 0x3) | ((uNextTex0Data[1] & 0x3) << 2);
	if (tex0.th > 10) tex0.th = 10;
	tex0.th   = 1<<tex0.th;
	tex0.tcc  =  (uNextTex0Data[1] >>  2) & 0x1;
	tex0.tfx  =  (uNextTex0Data[1] >>  3) & 0x3;

	ZeroGS::fiTexWidth[ictx] = (1/16.0f)/ tex0.tw;
	ZeroGS::fiTexHeight[ictx] = (1/16.0f) / tex0.th;

	if (tex0.tbw == 0) tex0.tbw = 64;

	if( PSMT_ISCLUT(psm) ) {
		tex0.cbp  = ((uNextTex0Data[1] >>  5) & 0x3fff);
		tex0.cpsm =  (uNextTex0Data[1] >> 19) & 0xe;
		tex0.csm  =  (uNextTex0Data[1] >> 23) & 0x1;

		if( tex0.cpsm <= 1 ) tex0.csa  =  (uNextTex0Data[1] >> 24) & 0xf;
		else tex0.csa  =  (uNextTex0Data[1] >> 24) & 0x1f;

		tex0.cld  =  (uNextTex0Data[1] >> 29) & 0x7;

		ZeroGS::texClutWrite(ictx);
	}
}

// does one time only initializing/destruction
class ZeroGSInit
{
public:
	ZeroGSInit() {
		// clear
		g_pbyGSMemory = (u8*)_aligned_malloc(0x00410000, 1024); // leave some room for out of range accesses (saves on the checks)
		memset(g_pbyGSMemory, 0, 0x00410000);

		g_pbyGSClut = (u8*)_aligned_malloc(256*8, 1024); // need 512 alignment!
		memset(g_pbyGSClut, 0, 256*8);
	}
	~ZeroGSInit() {
		_aligned_free(g_pbyGSMemory); g_pbyGSMemory = NULL;
		_aligned_free(g_pbyGSClut); g_pbyGSClut = NULL;
	}
};

static ZeroGSInit s_ZeroGSInit;

HRESULT ZeroGS::Create(LONG _width, LONG _height)
{
	Destroy(1);
	GSStateReset();	

	width = _width;
	height = _height;
	fiRendWidth = 1.0f / width;
	fiRendHeight = 1.0f / height;

	HRESULT hr;

	if( NULL == (pD3D = Direct3DCreate9(D3D_SDK_VERSION)) ) {
		ERROR_LOG(_T("Failed to create the direct3d interface."));
		return E_FAIL;
	}

	D3DDISPLAYMODE d3ddm;
	if( FAILED( hr = pD3D->GetAdapterDisplayMode( D3DADAPTER_DEFAULT, &d3ddm ) ) ) {
		ERROR_LOG(_T("Error geting default adapter."));
		return hr;
	}

	if( conf.options & GSOPTION_FULLSCREEN ) {
		// choose best mode
//		RECT rcdesktop;
//		GetWindowRect(GetDesktopWindow(), &rcdesktop);
//		width = rcdesktop.right - rcdesktop.left;
//		height = rcdesktop.bottom - rcdesktop.top;

//		width = height = 0;
//		D3DDISPLAYMODE d3ddmtemp;
//
//		int modes = pD3D->GetAdapterModeCount(D3DADAPTER_DEFAULT, d3ddm.Format);
//		for(int i= 0; i < modes; ++i) {
//			pD3D->EnumAdapterModes(D3DADAPTER_DEFAULT, d3ddm.Format, i, &d3ddmtemp);
//
//			if( abs(1024-(int)d3ddmtemp.Width) <= abs(1280-width) && abs(768-(int)d3ddmtemp.Height) <= abs(1024-height) ) {
//				width = d3ddmtemp.Width;
//				height = d3ddmtemp.Height;
//			}
//		}
	}
	else {
		// change to default resolution
		ChangeDisplaySettings(NULL, 0);
	}

	// Set up the structure used to create the D3DDevice. Since we are now
	// using more complex geometry, we will create a device with a zbuffer.
	ZeroMemory( &d3dpp, sizeof(d3dpp) );
	d3dpp.Windowed = !(conf.options & GSOPTION_FULLSCREEN);
	d3dpp.hDeviceWindow = GShwnd;
	d3dpp.SwapEffect = (conf.options & GSOPTION_FULLSCREEN) ? D3DSWAPEFFECT_FLIP : D3DSWAPEFFECT_DISCARD;
	d3dpp.BackBufferFormat = D3DFMT_A8R8G8B8;
	d3dpp.EnableAutoDepthStencil = TRUE;
	d3dpp.AutoDepthStencilFormat = D3DFMT_D24S8;
	d3dpp.BackBufferWidth = width;
	d3dpp.BackBufferHeight = height;
	d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;//(conf.options & GSOPTION_FULLSCREEN) ? D3DPRESENT_INTERVAL_DEFAULT : D3DPRESENT_INTERVAL_IMMEDIATE;
	d3dpp.Flags = DEBUG_PS2 ? D3DPRESENTFLAG_LOCKABLE_BACKBUFFER : 0;

	s_nFullscreen = (conf.options & GSOPTION_FULLSCREEN) ? 1 : 0;

	// Create the D3DDevice
	UINT adapter = D3DADAPTER_DEFAULT;
	D3DDEVTYPE devtype = !DEBUG_PS2 ? D3DDEVTYPE_HAL : D3DDEVTYPE_REF;

#ifndef _DEBUG
	DWORD hwoptions = D3DCREATE_HARDWARE_VERTEXPROCESSING|D3DCREATE_PUREDEVICE;
#else
	DWORD hwoptions = D3DCREATE_HARDWARE_VERTEXPROCESSING;
#endif

#ifndef RELEASE_TO_PUBLIC
	for(UINT i = 0; i < pD3D->GetAdapterCount(); ++i) {
		D3DADAPTER_IDENTIFIER9 id;
		HRESULT hr = pD3D->GetAdapterIdentifier(i, 0, &id);
		if( strcmp(id.Description, "NVIDIA NVPerfHUD") == 0 ) {
			DEBUG_LOG("Using %s adapter\n", id.Description);
			adapter = i;
			devtype = D3DDEVTYPE_REF;
			break;
		}
	}
#endif

	if( FAILED( hr = pD3D->CreateDevice( adapter, devtype, GShwnd,
		!DEBUG_PS2 ? hwoptions : D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &pd3dDevice ) ) )
	{	
		ERROR_LOG(_T("Failed to create hardware device, creating software.\n"));

		if( FAILED( hr = pD3D->CreateDevice( D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, GShwnd,
									  D3DCREATE_SOFTWARE_VERTEXPROCESSING,
									  &d3dpp, &pd3dDevice ) ) )
		{
			ERROR_LOG(_T("Failed to create software device, switching to reference rasterizer.\n"));

			if( FAILED( hr = pD3D->CreateDevice( D3DADAPTER_DEFAULT, D3DDEVTYPE_REF, GShwnd,
									  D3DCREATE_SOFTWARE_VERTEXPROCESSING,
									  &d3dpp, &pd3dDevice ) ) )
			  return hr;
		}
	}

	// get caps and check if gfx card is ok
	D3DCAPS9 caps;
	pd3dDevice->GetDeviceCaps(&caps);

	if( caps.VertexShaderVersion < D3DVS_VERSION(2,0)  ) {
		ERROR_LOG("*********\nGS ERROR: Need at least vs2.0\n*********\n");
		Destroy(1);
		return E_FAIL;
	}

	conf.mrtdepth = 1;

	if( caps.NumSimultaneousRTs == 1 ) {
		ERROR_LOG("*********\nGS WARNING: Need at least 2 simultaneous render targets. Some zbuffer effects will look wrong\n*********\n");
		conf.mrtdepth = 0;
	}
	if( !(caps.PrimitiveMiscCaps & D3DPMISCCAPS_SEPARATEALPHABLEND) ) {
		ERROR_LOG("*********\nGS ERROR: Need separate alpha blending! Some effects will look bad\n*********\n");
	}
	if( !(caps.PrimitiveMiscCaps & D3DPMISCCAPS_INDEPENDENTWRITEMASKS) ) {
		ERROR_LOG("******\nGS WARNING: Need independent write masks! Some z buffer effects might look bad\n*********\n");
		bIndepWriteMasks = 0;
	}

	if( !(caps.PrimitiveMiscCaps & D3DPMISCCAPS_MRTPOSTPIXELSHADERBLENDING) ) {
		ERROR_LOG("******\nGS WARNING: Need MRT Post Pixel Shader Blending for some effects\n*********\n");
	}

	hr = pD3D->CheckDeviceFormat( D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, d3ddm.Format,
		D3DUSAGE_RENDERTARGET|D3DUSAGE_QUERY_POSTPIXELSHADER_BLENDING, D3DRTYPE_TEXTURE, g_RenderFormat);

	if( g_GameSettings & GAME_32BITTARGS ) {
		g_RenderFormat = D3DFMT_A8R8G8B8;
		ERROR_LOG("Setting 32 bit render target\n");
	}
	else if( FAILED(hr) ) {
		ERROR_LOG("******\nGS ERROR: Device doesn't support alpha blending for 16bit floating point targets.\nQuality will reduce.\n*********\n");
		g_RenderFormat = D3DFMT_A8R8G8B8;
	}	

//	hr = pD3D->CheckDeviceFormat( D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, d3ddm.Format, 0, D3DRTYPE_TEXTURE, D3DFMT_G32R32F);
//
//	if( FAILED(hr) ) {
//		ERROR_LOG("******\nGS ERROR: Device doesn't support G32R32F textures.\nTextures will look bad.\n*********\n");
//	}

	g_MaxTexWidth = caps.MaxTextureWidth;
	g_MaxTexHeight = caps.MaxTextureHeight;
	GPU_TEXWIDTH = caps.MaxTextureWidth/8;
	g_fiGPU_TEXWIDTH = 1.0f / GPU_TEXWIDTH;

	//g_RenderFormat = D3DFMT_A8R8G8B8;

	pd3dDevice->GetRenderTarget(0, &psurfOrgTarg);
	pd3dDevice->GetDepthStencilSurface(&psurfOrgDepth);

	SETRS(D3DRS_ZENABLE, TRUE);
	SETRS(D3DRS_LIGHTING, FALSE);
	SETRS(D3DRS_SPECULARENABLE, FALSE);

	V_RETURN( D3DXCreateFont( pd3dDevice, 15, 0, FW_BOLD, 1, FALSE, DEFAULT_CHARSET,
							  OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
							  "Arial", &pFont ) );

	// create the vertex decl
	const D3DVERTEXELEMENT9 Decl[] = {
		{ 0, 0,  D3DDECLTYPE_SHORT4N, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
		{ 0, 8, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 0 },
		{ 0, 12, D3DDECLTYPE_UBYTE4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
		{ 0, 16, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 1 },
		D3DDECL_END() };
	V_RETURN(pd3dDevice->CreateVertexDeclaration(Decl, &pdecl));

#ifdef RELEASE_TO_PUBLIC
	HRSRC hShaderSrc = FindResource(hInst, MAKEINTRESOURCE(IDR_SHADERS), RT_RCDATA);
	assert( hShaderSrc != NULL );
	HGLOBAL hShaderGlob = LoadResource(hInst, hShaderSrc);
	assert( hShaderGlob != NULL );
	s_lpShaderResources = (BYTE*)LockResource(hShaderGlob);
#endif

	// load the effect
	ERROR_LOG("Creating effects\n");
	V_RETURN(LoadEffects());

	g_bDisplayMsg = 0;
	if( caps.VertexShaderVersion >= D3DVS_VERSION(3,0) && caps.PixelShaderVersion >= D3DPS_VERSION(2,0) )
		g_nPixelShaderVer = SHADER_30;
	else if( caps.PixelShaderVersion == D3DPS_VERSION(2,0) )
		g_nPixelShaderVer = SHADER_20;
	else
		g_nPixelShaderVer = SHADER_20a;

#ifdef RELEASE_TO_PUBLIC
	// create a sample shader
	clampInfo temp;
	memset(&temp, 0, sizeof(temp));
	temp.wms = 3; temp.wmt = 3;

	if( g_nPixelShaderVer != SHADER_30 ) {
		// test more
		if( LoadShadeEffect(0, 1, 1, 1, 1, temp, 0) == NULL ) {
			g_nPixelShaderVer = SHADER_20b;
			if( LoadShadeEffect(0, 1, 1, 1, 1, temp, 0) == NULL ) {

				g_nPixelShaderVer = SHADER_20;
				if( LoadShadeEffect(0, 0, 1, 1, 0, temp, 0) == NULL ) {

					ERROR_LOG("*********\nGS ERROR: Need at least ps2.0 (ps2.0a+ recommended)\n*********\n");
					Destroy(1);
					return E_FAIL;
				}
			}
		}
	}
#endif

	// set global shader constants
	pd3dDevice->SetPixelShaderConstantF(27, DXVEC4(0.5f, (g_GameSettings&GAME_EXACTCOLOR)?0.9f/256.0f:0.5f/256.0f, 0,1/255.0f), 1); // g_fExactColor
	pd3dDevice->SetPixelShaderConstantF(28, DXVEC4(-0.7f, -0.65f, 0.9f,0), 1); // g_fBilinear
	pd3dDevice->SetPixelShaderConstantF(29, DXVEC4(1.0f/256.0f, 1.0004f, 1, 0.5f), 1); // g_fZBias
	pd3dDevice->SetPixelShaderConstantF(30, DXVEC4(0,1, 0.001f, 0.5f), 1); // g_fc0
	pd3dDevice->SetPixelShaderConstantF(31, DXVEC4(1/1024.0f, 0.2f/1024.0f, 1/128.0f, 1/512.0f), 1); // g_fMult

	pd3dDevice->SetVertexShaderConstantF(29, DXVEC4(1.0f/256.0f, 1.0004f, 1, 0.5f), 1); // g_fZBias
	pd3dDevice->SetVertexShaderConstantF(30, DXVEC4(0,1, 0.001f, 0.5f), 1); // g_fc0
	pd3dDevice->SetVertexShaderConstantF(31, DXVEC4(0.5f, -0.5f, 0.5f, 0.5f + 0.4f/416.0f), 1); // g_fBitBltTrans

	g_bDisplayMsg = 1;
	if( g_nPixelShaderVer == SHADER_20 )
		conf.bilinear = 0;

	ERROR_LOG("Creating extra effects\n");
	V_RETURN(LoadExtraEffects());

	ERROR_LOG("GS Using pixel shaders %s\n", g_pShaders[g_nPixelShaderVer]);

	pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET|D3DCLEAR_STENCIL|D3DCLEAR_ZBUFFER, 0, 1, 0);

	// init draw fns
	drawfn[0] = KickPoint;
	drawfn[1] = KickLine;
	drawfn[2] = KickLine;
	drawfn[3] = KickTriangle;
	drawfn[4] = KickTriangle;
	drawfn[5] = KickTriangleFan;
	drawfn[6] = KickSprite;
	drawfn[7] = KickDummy;

	SetAA(conf.aa);
	GSsetGameCRC(g_LastCRC, g_GameSettings);

	return S_OK;
}

void ZeroGS::Destroy(BOOL bD3D)
{
	DeleteDeviceObjects();

	vb[0].Destroy();
	vb[1].Destroy();

	for(int i = 0; i < ArraySize(pvs); ++i) {
		SAFE_RELEASE(pvs[i]);
	}
	for(int i = 0; i < ArraySize(ppsRegular); ++i) {
		SAFE_RELEASE(ppsRegular[i]);
	}
	for(int i = 0; i < ArraySize(ppsTexture); ++i) {
		SAFE_RELEASE(ppsTexture[i]);
	}

	SAFE_RELEASE(pvsBitBlt);
	SAFE_RELEASE(pvsBitBlt30);
	SAFE_RELEASE(ppsBitBlt[0]); SAFE_RELEASE(ppsBitBlt[1]);
	SAFE_RELEASE(ppsBitBltDepth[0]); SAFE_RELEASE(ppsBitBltDepth[1]);
	SAFE_RELEASE(ppsBitBltDepthTex[0]); SAFE_RELEASE(ppsBitBltDepthTex[1]);
	SAFE_RELEASE(ppsCRTCTarg[0]); SAFE_RELEASE(ppsCRTCTarg[1]);
	SAFE_RELEASE(ppsCRTC[0]); SAFE_RELEASE(ppsCRTC[1]);
	SAFE_RELEASE(ppsCRTC24[0]); SAFE_RELEASE(ppsCRTC24[1]);
	SAFE_RELEASE(ppsOne);

	SAFE_RELEASE(pdecl);
	SAFE_RELEASE(pFont);
	SAFE_RELEASE(psurfOrgTarg);
	SAFE_RELEASE(psurfOrgDepth);

	if( bD3D ) {
		SAFE_RELEASE(pd3dDevice);
		SAFE_RELEASE(pD3D);
	}
}

void ZeroGS::GSStateReset()
{
	icurctx = -1;

	for(int i = 0; i < 2; ++i) {
		LPD3DVB pvb = vb[i].pvb;
		if( pvb != NULL ) pvb->AddRef();
		vb[i].Destroy();
		memset(&vb[i], 0, sizeof(VB));
		
		vb[i].tex0.tw = 1;
		vb[i].tex0.th = 1;
		vb[i].pvb = pvb;
		vb[i].scissor.x1 = 639;
		vb[i].scissor.y1 = 479;
		vb[i].tex0.tbw = 64;
	}

	s_RangeMngr.Clear();
	g_MemTargs.Destroy();
	s_RTs.Destroy();
	s_DepthRTs.Destroy();
	s_BitwiseTextures.Destroy();

	vb[0].ictx = 0;
	vb[1].ictx = 1;
	s_bAlphaSet = FALSE;
}

void ZeroGS::AddMessage(const char* pstr, DWORD ms)
{
	listMsgs.push_back(MESSAGE(pstr, timeGetTime()+ms));
}

void ZeroGS::ChangeWindowSize(int nNewWidth, int nNewHeight)
{
	width = nNewWidth > 16 ? nNewWidth : 16;
	height = nNewHeight > 16 ? nNewHeight : 16;

	if( !(conf.options & GSOPTION_FULLSCREEN) ) {
		conf.width = nNewWidth;
		conf.height = nNewHeight;
		//SaveConfig();
	}
}

void ZeroGS::SetChangeDeviceSize(int nNewWidth, int nNewHeight)
{
	s_nNewWidth = nNewWidth;
	s_nNewHeight = nNewHeight;

	if( !(conf.options & GSOPTION_FULLSCREEN) ) {
		conf.width = nNewWidth;
		conf.height = nNewHeight;
		//SaveConfig();
	}
}

void ZeroGS::Reset()
{
	s_RTs.ResolveAll();
	s_DepthRTs.ResolveAll();
	
	vb[0].Unlock();
	vb[1].Unlock();

	memset(s_nResolveCounts, 0, sizeof(s_nResolveCounts));
	s_nLastResolveReset = 0;

	icurctx = -1;

	GSStateReset();
	Destroy(0);

	drawfn[0] = KickDummy;
	drawfn[1] = KickDummy;
	drawfn[2] = KickDummy;
	drawfn[3] = KickDummy;
	drawfn[4] = KickDummy;
	drawfn[5] = KickDummy;
	drawfn[6] = KickDummy;
	drawfn[7] = KickDummy;
}

void ZeroGS::ChangeDeviceSize(int nNewWidth, int nNewHeight)
{
	int oldscreen = s_nFullscreen;

	int oldwidth = width, oldheight = height;
	if( FAILED(Create(nNewWidth&~7, nNewHeight&~7)) ) {
		DEBUG_LOG("Failed to recreate, changing to old\n");
		if( FAILED(Create(oldwidth, oldheight)) ) {
			MessageBox(NULL, "failed to create dev, exiting...\n", "Error", MB_OK);
			exit(0);
		}
	}

	if( FAILED(InitDeviceObjects()) ) {
		MessageBox(NULL, "failed to init dev objs, exiting...\n", "Error", MB_OK);
		exit(0);
	}

	for(int i = 0; i < 2; ++i) {
		vb[i].bNeedFrameCheck = vb[i].bNeedZCheck = 1;
		vb[i].CheckFrame(0);
	}

	if( oldscreen && !(conf.options & GSOPTION_FULLSCREEN) ) { // if transitioning from full screen
		RECT rc;
		rc.left = 0; rc.top = 0;
		rc.right = conf.width; rc.bottom = conf.height;
		AdjustWindowRect(&rc, conf.winstyle, FALSE);

		RECT rcdesktop;
		GetWindowRect(GetDesktopWindow(), &rcdesktop);

		SetWindowLong( GShwnd, GWL_STYLE, conf.winstyle );
		SetWindowPos(GShwnd, HWND_TOP, ((rcdesktop.right-rcdesktop.left)-(rc.right-rc.left))/2,
			((rcdesktop.bottom-rcdesktop.top)-(rc.bottom-rc.top))/2,
			rc.right-rc.left, rc.bottom-rc.top, SWP_SHOWWINDOW);
		UpdateWindow(GShwnd);
	}

	vb[0].Lock();
	vb[1].Lock();

	assert( vb[0].pbuf != NULL && vb[1].pbuf != NULL );
}

void ZeroGS::SetAA(int mode)
{
	float f;
	
	// need to flush all targets
	s_RTs.ResolveAll();
	s_RTs.Destroy();
	s_DepthRTs.ResolveAll();
	s_DepthRTs.Destroy();

	s_AAx = s_AAy = 0;
	if( mode > 0 ) 
	{
		s_AAx = (mode+1) / 2;
		s_AAy = mode / 2;
	}
	memset(s_nResolveCounts, 0, sizeof(s_nResolveCounts));
	s_nLastResolveReset = 0;

	vb[0].prndr = NULL; vb[0].pdepth = NULL; vb[0].bNeedFrameCheck = 1; vb[0].bNeedZCheck = 1;
	vb[1].prndr = NULL; vb[1].pdepth = NULL; vb[1].bNeedFrameCheck = 1; vb[1].bNeedZCheck = 1;
	
	f = mode > 0 ? 2.0f : 1.0f;
	SETRS(D3DRS_POINTSIZE, FtoDW(f));
}

#ifdef RELEASE_TO_PUBLIC

#define LOAD_VS(Index, ptr) { \
	assert( mapShaderResources.find(Index) != mapShaderResources.end() ); \
	header = mapShaderResources[Index]; \
	assert( (header) != NULL && (header)->index == (Index) ); \
	hr = pd3dDevice->CreateVertexShader((DWORD*)(s_lpShaderResources + (header)->offset), &(ptr)); \
	if( FAILED(hr) || ptr == NULL ) { \
		DEBUG_LOG("errors 0x%x for %d, failed.. try updating your drivers or dx\n", hr, Index); \
		return E_FAIL; \
	} \
} \

#define LOAD_PS(index, ptr) { \
	assert( mapShaderResources.find(index) != mapShaderResources.end() ); \
	header = mapShaderResources[index]; \
	hr = pd3dDevice->CreatePixelShader((DWORD*)(s_lpShaderResources + (header)->offset), &(ptr)); \
	if( FAILED(hr) || ptr == NULL ) { \
		DEBUG_LOG("errors 0x%x for %s, failed.. try updating your drivers or dx\n", hr, index); \
		return E_FAIL; \
	} \
} \

HRESULT ZeroGS::LoadEffects()
{
	assert( s_lpShaderResources != NULL );

	// process the header
	DWORD num = *(DWORD*)s_lpShaderResources;
	SHADERHEADER* header = (SHADERHEADER*)((BYTE*)s_lpShaderResources + 4);

	mapShaderResources.clear();
	while(num-- > 0 ) {
		mapShaderResources[header->index] = header;
		++header;
	}

	// clear the textures
	for(int i = 0; i < ArraySize(ppsTexture); ++i) {
		SAFE_RELEASE(ppsTexture[i]);
	}
	memset(ppsTexture, 0, sizeof(ppsTexture));

	return S_OK;
}

// called
HRESULT ZeroGS::LoadExtraEffects()
{
	HRESULT hr;

	SHADERHEADER* header;

	DWORD mask = g_nPixelShaderVer == SHADER_30 ? SH_30 : 0;

	const int vsshaders[4] = { SH_REGULARVS, SH_TEXTUREVS, SH_REGULARFOGVS, SH_TEXTUREFOGVS };
	for(int i = 0; i < 4; ++i) {
		LOAD_VS(vsshaders[i]|mask, pvs[2*i]);
		LOAD_VS(vsshaders[i]|mask|SH_CONTEXT1, pvs[2*i+1]);
		LOAD_VS(vsshaders[i]|mask|SH_WRITEDEPTH, pvs[2*i+8]);
		LOAD_VS(vsshaders[i]|mask|SH_WRITEDEPTH|SH_CONTEXT1, pvs[2*i+8+1]);
	}
	
	LOAD_VS(SH_BITBLTVS, pvsBitBlt);
	//LOAD_VS(SH_BITBLTVS|SH_30, pvsBitBlt30);

	LOAD_PS(SH_REGULARPS|mask, ppsRegular[0]);
	LOAD_PS(SH_REGULARFOGPS|mask, ppsRegular[1]);
	LOAD_PS(SH_REGULARPS|SH_WRITEDEPTH|mask, ppsRegular[2]);
	LOAD_PS(SH_REGULARFOGPS|SH_WRITEDEPTH|mask, ppsRegular[3]);

	LOAD_PS(SH_BITBLTPS, ppsBitBlt[0]); LOAD_PS(SH_BITBLTAAPS, ppsBitBlt[0]);
	LOAD_PS(SH_BITBLTDEPTHPS, ppsBitBltDepth[0]); LOAD_PS(SH_BITBLTDEPTHMRTPS, ppsBitBltDepth[1]);
	LOAD_PS(SH_BITBLTDEPTHTEXPS, ppsBitBltDepthTex[0]); LOAD_PS(SH_BITBLTDEPTHTEXMRTPS, ppsBitBltDepthTex[1]);
	LOAD_PS(SH_CRTCTARGPS, ppsCRTCTarg[0]); LOAD_PS(SH_CRTCTARGINTERPS, ppsCRTCTarg[1]);
	LOAD_PS(SH_CRTCPS, ppsCRTC[0]); LOAD_PS(SH_CRTCINTERPS, ppsCRTC[1]);
	LOAD_PS(SH_CRTC24PS, ppsCRTC24[0]); LOAD_PS(SH_CRTC24INTERPS, ppsCRTC24[1]);
	LOAD_PS(SH_ZEROPS|mask, ppsOne);
	LOAD_PS(SH_BASETEXTUREPS, ppsBaseTexture);
	LOAD_PS(SH_CONVERT16TO32PS, ppsConvert16to32);
	LOAD_PS(SH_CONVERT32TO16PS, ppsConvert32to16);

	return S_OK;
}

LPD3DPS ZeroGS::LoadShadeEffect(int type, int texfilter, int fog, int testaem, int exactcolor, const clampInfo& clamp, int context)
{
	int texwrap;
	assert( texfilter < NUM_FILTERS );

	if(g_nPixelShaderVer == SHADER_20 )
		texfilter = 0;
	if(g_nPixelShaderVer == SHADER_20 )
		exactcolor = 0;

	if( clamp.wms == clamp.wmt ) {
		switch( clamp.wms ) {
			case 0: texwrap = TEXWRAP_REPEAT; break;
			case 1: texwrap = TEXWRAP_CLAMP; break;
			case 2: texwrap = TEXWRAP_CLAMP; break;
			default: texwrap = TEXWRAP_REGION_REPEAT; break;
		}
	}
	else if( clamp.wms==3||clamp.wmt==3)
		texwrap = TEXWRAP_REGION_REPEAT;
	else
		texwrap = TEXWRAP_REPEAT_CLAMP;

	int index = GET_SHADER_INDEX(type, texfilter, texwrap, fog, s_bWriteDepth, testaem, exactcolor, context, 0);

	assert( index < ArraySize(ppsTexture) );
	LPD3DPS* pps = ppsTexture+index;

	if( *pps != NULL ) 
		return *pps;

	index += NUM_SHADERS*g_nPixelShaderVer;
	assert( mapShaderResources.find(index) != mapShaderResources.end() );
	SHADERHEADER* header = mapShaderResources[index];
	if( header == NULL ) DEBUG_LOG("%d %d\n", index%NUM_SHADERS, g_nPixelShaderVer);
	assert( header != NULL );
	HRESULT hr = pd3dDevice->CreatePixelShader((DWORD*)(s_lpShaderResources + header->offset), pps);

	if( SUCCEEDED(hr) )
		return *pps;

	if( g_bDisplayMsg )
		ERROR_LOG("Failed to create shader %d,%d,%d,%d\n", 3, fog, texfilter, 4*clamp.wms+clamp.wmt);

	return NULL;
}
	
#else // not RELEASE_TO_PUBLIC

//#define EFFECT_NAME "f:\\ps2dev\\pcsx2\\zerogs\\dx\\"
#define EFFECT_NAME ".\\"
#define COMPILE_SHADER(name, type, flags) 

class ZeroGSShaderInclude : public ID3DXInclude
{
public:
	int context;
	STDMETHOD(Open)(D3DXINCLUDE_TYPE IncludeType, LPCSTR pFileName, LPCVOID pParentData, LPCVOID *ppData, UINT *pBytes)
	{
		const char* pfilename = pFileName;
		char strfile[255];
		if( strstr(pFileName, "ps2hw_ctx") != NULL ) {

			_snprintf(strfile, 255, "%sps2hw_ctx%d.fx", EFFECT_NAME, context);
			pfilename = strfile;
		}
		else if( strstr(pFileName, "\\") == NULL ) {
			_snprintf(strfile, 255, "%s%s", EFFECT_NAME, pFileName);
			pfilename = strfile;
		}
		
		FILE* f = fopen(pfilename, "rb");

		if( f == NULL )
			return E_FAIL;

		fseek(f, 0, SEEK_END);
		DWORD size = ftell(f);
		fseek(f, 0, SEEK_SET);
		char* buffer = new char[size+1];
		fread(buffer, size, 1, f);
		buffer[size] = 0;

		*ppData = buffer;
		*pBytes = size;
		fclose(f);

		return S_OK;
	}

	STDMETHOD(Close)(LPCVOID pData)
	{
		delete[] (char*)pData;
		return S_OK;
	}
};

#define LOAD_VS(name, ptr, shaderver) { \
	LPD3DXBUFFER pShader, pError; \
	V(D3DXCompileShaderFromFile(EFFECT_NAME"ps2hw.fx", pmacros, pInclude, name, shaderver, ShaderFlagsVS, &pShader, &pError, NULL)); \
	if( FAILED(hr) ) \
	{ \
		DEBUG_LOG("Failed to load vs %s: \n%s\n", name, pError->GetBufferPointer()); \
		SAFE_RELEASE(pShader); \
		SAFE_RELEASE(pError); \
		return hr; \
	} \
	hr = pd3dDevice->CreateVertexShader((const DWORD*)pShader->GetBufferPointer(), &(ptr)); \
	SAFE_RELEASE(pShader); \
	SAFE_RELEASE(pError); \
} \

#define LOAD_PS(name, ptr, shmodel) { \
	LPD3DXBUFFER pShader, pError; \
	SAFE_RELEASE(ptr); \
	V(D3DXCompileShaderFromFile(EFFECT_NAME"ps2hw.fx", pmacros, pInclude, name, shmodel, ShaderFlagsPS, &pShader, &pError, NULL)); \
	if( FAILED(hr) ) \
	{ \
		DEBUG_LOG("Failed to load ps %s: \n%s\n", name, pError->GetBufferPointer()); \
		SAFE_RELEASE(pShader); \
		SAFE_RELEASE(pError); \
		return hr; \
	} \
	hr = pd3dDevice->CreatePixelShader((const DWORD*)pShader->GetBufferPointer(), &(ptr)); \
	SAFE_RELEASE(pShader); \
	SAFE_RELEASE(pError); \
	if( FAILED(hr) || ptr == NULL ) { \
		DEBUG_LOG("errors 0x%x for %s, failed.. try updating your drivers or dx\n", hr, name); \
		return E_FAIL; \
	} \
} \

HRESULT ZeroGS::LoadEffects()
{
	// clear the textures
	for(int i = 0; i < ArraySize(ppsTexture); ++i) {
		SAFE_RELEASE(ppsTexture[i]);
	}
	memset(ppsTexture, 0, sizeof(ppsTexture));

	return S_OK;
}

#define VS_VER (g_nPixelShaderVer == SHADER_20?"vs_2_0":"vs_3_0")
#define PS_VER (g_nPixelShaderVer == SHADER_20?"ps_2_0":"ps_3_0")

HRESULT ZeroGS::LoadExtraEffects()
{
	HRESULT hr;
	DWORD ShaderFlagsPS = !DEBUG_PS2 ? 0 : (D3DXSHADER_DEBUG|D3DXSHADER_SKIPOPTIMIZATION);
	DWORD ShaderFlagsVS = !DEBUG_PS2 ? 0 : (D3DXSHADER_DEBUG|D3DXSHADER_SKIPOPTIMIZATION);

	ZeroGSShaderInclude inc;
	inc.context = 0;
	ZeroGSShaderInclude* pInclude = &inc;

	//assert( g_nPixelShaderVer == SHADER_30) ;
	const char* pstrps = g_nPixelShaderVer == SHADER_20 ? "ps_2_0" : "ps_2_a";
	const char* pvsshaders[4] = { "RegularVS", "TextureVS", "RegularFogVS", "TextureFogVS" };

	D3DXMACRO macros[2] = {0};
	D3DXMACRO* pmacros = NULL;

	macros[0].Name = "WRITE_DEPTH";
	macros[0].Definition = "1";	

	for(int i = 0; i < 4; ++i) {
		pmacros = NULL;
		inc.context = 0;
		LOAD_VS(pvsshaders[i], pvs[2*i], VS_VER);
		inc.context = 1;
		LOAD_VS(pvsshaders[i], pvs[2*i+1], VS_VER);

		pmacros = macros;
		inc.context = 0;
		LOAD_VS(pvsshaders[i], pvs[2*i+8], VS_VER);
		inc.context = 1;
		LOAD_VS(pvsshaders[i], pvs[2*i+8+1], VS_VER);
	}

	inc.context = 0;
	pmacros = NULL;
	LOAD_PS("RegularPS", ppsRegular[0], PS_VER);
	LOAD_PS("RegularFogPS", ppsRegular[1], PS_VER);

	pmacros = macros;
	LOAD_PS("RegularPS", ppsRegular[2], PS_VER);
	LOAD_PS("RegularFogPS", ppsRegular[3], PS_VER);

	pmacros = NULL;
	LOAD_VS("BitBltVS", pvsBitBlt, "vs_2_0");
	LOAD_PS("BitBltPS", ppsBitBlt[0], pstrps);
	LOAD_PS("BitBltAAPS", ppsBitBlt[1], pstrps);
	LOAD_PS("BitBltDepthPS", ppsBitBltDepth[0], pstrps);
	LOAD_PS("BitBltDepthMRTPS", ppsBitBltDepth[1], pstrps);
	LOAD_PS("BitBltDepthTexPS", ppsBitBltDepthTex[0], pstrps);
	LOAD_PS("BitBltDepthTexMRTPS", ppsBitBltDepthTex[1], pstrps);
	LOAD_PS("CRTCTargPS", ppsCRTCTarg[0], pstrps); LOAD_PS("CRTCTargInterPS", ppsCRTCTarg[1], pstrps);
	LOAD_PS("CRTCPS", ppsCRTC[0], pstrps); LOAD_PS("CRTCInterPS", ppsCRTC[1], pstrps);
	LOAD_PS("CRTC24PS", ppsCRTC24[0], pstrps); LOAD_PS("CRTC24InterPS", ppsCRTC24[1], pstrps);
	LOAD_PS("ZeroPS", ppsOne, PS_VER);
	LOAD_PS("BaseTexturePS", ppsBaseTexture, pstrps);
	LOAD_PS("Convert16to32PS", ppsConvert16to32, pstrps);
	LOAD_PS("Convert32to16PS", ppsConvert32to16, pstrps);

	return S_OK;
}

LPD3DPS ZeroGS::LoadShadeEffect(int type, int texfilter, int fog, int testaem, int exactcolor, const clampInfo& clamp, int context)
{
	int texwrap;
	
	assert( texfilter < NUM_FILTERS );
	//assert( g_nPixelShaderVer == SHADER_30 );
	if( clamp.wms == clamp.wmt ) {
		switch( clamp.wms ) {
			case 0: texwrap = TEXWRAP_REPEAT; break;
			case 1: texwrap = TEXWRAP_CLAMP; break;
			case 2: texwrap = TEXWRAP_CLAMP; break;
			default:
				texwrap = TEXWRAP_REGION_REPEAT; break;
		}
	}
	else if( clamp.wms==3||clamp.wmt==3)
		texwrap = TEXWRAP_REGION_REPEAT;
	else
		texwrap = TEXWRAP_REPEAT_CLAMP;

	int index = GET_SHADER_INDEX(type, texfilter, texwrap, fog, s_bWriteDepth, testaem, exactcolor, context, 0);

	LPD3DPS* pps = ppsTexture+index;

	if( *pps != NULL ) 
		return *pps;
	
	ZeroGSShaderInclude inc;
	inc.context = context;

	HRESULT hr = LoadShaderFromType(EFFECT_NAME"ps2hw.fx", type, texfilter, texwrap, fog, s_bWriteDepth, testaem, exactcolor, g_nPixelShaderVer, 0, pd3dDevice, &inc, pps);

	if( SUCCEEDED(hr) )
		return *pps;

	DEBUG_LOG("Failed to create shader %d,%d,%d,%d\n", type, fog, texfilter, 4*clamp.wms+clamp.wmt);

	return NULL;
}

#endif // RELEASE_TO_PUBLIC

HRESULT ZeroGS::InitDeviceObjects()
{
	//g_GameSettings |= 0;//GAME_VSSHACK|GAME_FULL16BITRES|GAME_NODEPTHRESOLVE|GAME_FASTUPDATE;
	//s_bWriteDepth = TRUE;
	DeleteDeviceObjects();

	int i;
	HRESULT hr;
	SETRS(D3DRS_SRCBLEND, D3DBLEND_ONE);
	SETRS(D3DRS_DESTBLEND, D3DBLEND_ONE);

	if( pFont ) V_RETURN( pFont->OnResetDevice() );
	V_RETURN( D3DXCreateSprite( pd3dDevice, &pSprite ) );

	V(D3DXCreateTextureFromResource(pd3dDevice, hInst, MAKEINTRESOURCE( IDB_ZEROGSLOGO ), &ptexLogo));

	for(i = 0; i < 2; ++i)
	{
		V_RETURN(pd3dDevice->CreateVertexBuffer( sizeof(VertexGPU) * POINT_BUFFERSIZE, D3DUSAGE_DYNAMIC|D3DUSAGE_WRITEONLY, 0, D3DPOOL_DEFAULT, &vb[i].pvb, NULL));
	}

	// create the blocks texture
	D3DFORMAT blockfmt = D3DFMT_R32F;
	g_fBlockMult = 1;

	if( FAILED(hr = pd3dDevice->CreateTexture(BLOCK_TEXWIDTH, BLOCK_TEXHEIGHT, 1, 0, blockfmt, D3DPOOL_MANAGED, &ptexBlocks, NULL)) ) {
		blockfmt = D3DFMT_G16R16;
		g_fBlockMult = 65535.0f*(float)g_fiGPU_TEXWIDTH;
		V_RETURN(pd3dDevice->CreateTexture(BLOCK_TEXWIDTH, BLOCK_TEXHEIGHT, 1, 0, blockfmt, D3DPOOL_MANAGED, &ptexBlocks, NULL));
	}

	if( blockfmt == D3DFMT_R32F ) {
		if( FAILED(hr = pd3dDevice->CreateTexture(BLOCK_TEXWIDTH, BLOCK_TEXHEIGHT, 1, 0, D3DFMT_A32B32G32R32F, D3DPOOL_MANAGED, &ptexBilinearBlocks, NULL)) ) {
			DEBUG_LOG("Failed to create bilinear block texture, fmt = D3DFMT_A32B32G32R32F\n");
		}
	}
	else ptexBilinearBlocks = NULL;

	// fill a simple rect
	V_RETURN(pd3dDevice->CreateVertexBuffer( 4 * sizeof(VertexGPU), D3DUSAGE_WRITEONLY, 0, D3DPOOL_DEFAULT, &pvbRect, NULL));
	VertexGPU* pvert;

	pvbRect->Lock(0, 0, (void**)&pvert, 0);
	pvert->x = -0x7fff; pvert->y = 0x7fff; pvert->z = 0; pvert->s = 0; pvert->t = 0; pvert++;
	pvert->x = 0x7fff; pvert->y = 0x7fff; pvert->z = 0; pvert->s = 1; pvert->t = 0; pvert++;
	pvert->x = -0x7fff; pvert->y = -0x7fff; pvert->z = 0; pvert->s = 0; pvert->t = 1; pvert++;
	pvert->x = 0x7fff; pvert->y = -0x7fff; pvert->z = 0; pvert->s = 1; pvert->t = 1; pvert++;
	pvbRect->Unlock();

	D3DLOCKED_RECT lock, lockbilinear;
	ptexBlocks->LockRect(0, &lock, NULL, 0);

	if( ptexBilinearBlocks != NULL )
		ptexBilinearBlocks->LockRect(0, &lockbilinear, NULL, 0);
	
	BLOCK::FillBlocks(&lock, ptexBilinearBlocks != NULL ? &lockbilinear : NULL, blockfmt);

	ptexBlocks->UnlockRect(0);
	if( ptexBilinearBlocks != NULL )
		ptexBilinearBlocks->UnlockRect(0);

	// create the conversion textures
	V_RETURN(pd3dDevice->CreateTexture(256, 256, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &ptexConv16to32, NULL));
	ptexConv16to32->LockRect(0, &lock, NULL, 0);
	assert(lock.Pitch == 256*4);
	u32* dst = (u32*)lock.pBits;
	for(i = 0; i < 256*256; ++i) {
		DWORD tempcol = RGBA16to32(i);
		// have to flip r and b
		*dst++ = (tempcol&0xff00ff00)|((tempcol&0xff)<<16)|((tempcol&0xff0000)>>16);
	}
	ptexConv16to32->UnlockRect(0);

	V_RETURN(pd3dDevice->CreateVolumeTexture(32, 32, 32, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &ptexConv32to16, NULL));
	D3DLOCKED_BOX lockbox;
	ptexConv32to16->LockBox(0, &lockbox, NULL, 0);
	dst = (u32*)lockbox.pBits;
	for(i = 0; i < 32; ++i) {
		for(int j = 0; j < 32; ++j) {
			for(int k = 0; k < 32; ++k) {
				u32 col = (i<<10)|(j<<5)|k;
				*dst++ = ((col&0xff)<<16)|(col&0xff00);
			}
		}
	}
	ptexConv32to16->UnlockBox(0);

	// set samplers
	for(i = 0; i < 8; ++i) {
		pd3dDevice->SetSamplerState(i, D3DSAMP_MINFILTER, D3DTEXF_POINT);
		pd3dDevice->SetSamplerState(i, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
		pd3dDevice->SetSamplerState(i, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
		pd3dDevice->SetSamplerState(i, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
	}

	//pd3dDevice->SetSamplerState(SAMP_SRC, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP);
	//pd3dDevice->SetSamplerState(SAMP_SRC, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP);
	pd3dDevice->SetSamplerState(SAMP_BLOCKS, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP);
	pd3dDevice->SetSamplerState(SAMP_BLOCKS, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP);
	pd3dDevice->SetSamplerState(SAMP_BILINEARBLOCKS, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP);
	pd3dDevice->SetSamplerState(SAMP_BILINEARBLOCKS, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP);
	pd3dDevice->SetSamplerState(SAMP_BILINEARBLOCKS, D3DSAMP_ADDRESSW, D3DTADDRESS_CLAMP); // can be used as a 3d texture
	pd3dDevice->SetSamplerState(SAMP_BITWISEANDX, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP);
	pd3dDevice->SetSamplerState(SAMP_BITWISEANDX, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP);
	pd3dDevice->SetSamplerState(SAMP_BITWISEANDY, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP);
	pd3dDevice->SetSamplerState(SAMP_BITWISEANDY, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP);

	pd3dDevice->SetSamplerState(SAMP_FINAL, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
	pd3dDevice->SetSamplerState(SAMP_FINAL, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);

	pd3dDevice->SetTexture(SAMP_BLOCKS, ptexBlocks);
	pd3dDevice->SetTexture(SAMP_BILINEARBLOCKS, ptexBilinearBlocks);
	pd3dDevice->SetVertexDeclaration(pdecl);
	
	SETRS(D3DRS_STENCILENABLE, FALSE);
	SETRS(D3DRS_SCISSORTESTENABLE, 1);
	SETRS(D3DRS_SEPARATEALPHABLENDENABLE, USEALPHABLENDING);
	SETRS(D3DRS_SRCBLENDALPHA, D3DBLEND_ONE);
	SETRS(D3DRS_DESTBLENDALPHA, D3DBLEND_ZERO);
	SETRS(D3DRS_CULLMODE, D3DCULL_NONE);
	SETRS(D3DRS_BLENDFACTOR, 0x80000000);
	SETRS(D3DRS_COLORWRITEENABLE1, 0);

	// points
	SETRS(D3DRS_POINTSCALEENABLE, FALSE);
	SETRS(D3DRS_POINTSIZE, FtoDW(1.0f));

	g_nDepthBias = 0;
	SETRS(D3DRS_DEPTHBIAS, FtoDW(0.000015f));

	SETCONSTF(GPU_Z, g_vdepth);//vb[icurctx].zbuf.psm&3]);

	s_vznorm = DXVEC4(g_filog32, 0, 0,0);
	SETCONSTF(GPU_ZNORM, s_vznorm);

	return S_OK;
}

void ZeroGS::DeleteDeviceObjects()
{
	if( s_aviinit ) {
		StopCapture();
		STOP_AVI();
		DEBUG_LOG("zerogs.avi stopped");
		s_aviinit = 0;
	}

	SAFE_RELEASE(s_ptexAVICapture);

	if( pFont ) pFont->OnLostDevice();
	SAFE_RELEASE(pSprite);

	g_MemTargs.Destroy();
	s_RTs.Destroy();
	s_DepthRTs.Destroy();
	s_BitwiseTextures.Destroy();

	SAFE_RELEASE(s_ptexInterlace);
	SAFE_RELEASE(pvbRect);
	SAFE_RELEASE(ptexBlocks);
	SAFE_RELEASE(ptexBilinearBlocks);
	SAFE_RELEASE(ptexConv16to32);
	SAFE_RELEASE(ptexConv32to16);
	s_bAlphaSet = FALSE;

	vb[0].Unlock();
	SAFE_RELEASE(vb[0].pvb);

	vb[1].Unlock();
	SAFE_RELEASE(vb[1].pvb);
}

void ZeroGS::Prim()
{
	if( g_bIsLost )
		return;

	VB& curvb = vb[prim->ctxt];
	if( curvb.CheckPrim() )
		Flush(prim->ctxt);
	curvb.curprim._val = prim->_val;

	// flush the other pipe if sharing the same buffer
//	if( vb[prim->ctxt].gsfb.fbp == vb[!prim->ctxt].gsfb.fbp && vb[!prim->ctxt].dwCount > 0 )
//	{
//		assert( vb[prim->ctxt].dwCount == 0 );
//		Flush(!prim->ctxt);
//	}

	curvb.curprim.prim = prim->prim;
	vb[prim->ctxt].Lock();
}

int GetTexFilter(const tex1Info& tex1)
{
	// always force
	if( conf.bilinear == 2 )
		return 1;

	int texfilter = 0;
	if( conf.bilinear && ptexBilinearBlocks != NULL ) {
		if( tex1.mmin <= 1 )
			texfilter = tex1.mmin|tex1.mmag;
		else
			texfilter = tex1.mmag ? ((tex1.mmin+2)&5) : tex1.mmin;

		texfilter = texfilter == 1 || texfilter == 4 || texfilter == 5;
	}
	return texfilter;
}

void ZeroGS::ReloadEffects()
{
#ifndef RELEASE_TO_PUBLIC
	for(int i = 0; i < ArraySize(ppsTexture); ++i) {
		SAFE_RELEASE(ppsTexture[i]);
	}
	memset(ppsTexture, 0, sizeof(ppsTexture));
	LoadExtraEffects();
#endif
}

static int s_ClutResolve = 0;
static int s_PSM8Resolve = 0;
void ZeroGS::Flush(int context)
{
	assert( context >= 0 && context <= 1 );

#ifndef RELEASE_TO_PUBLIC
	if( g_bUpdateEffect ) {
		ReloadEffects();
		g_bUpdateEffect = 0;
	}
#endif

	VB& curvb = vb[context];
	const pixTest curtest = curvb.test;
	if( curvb.dwCount == 0 || (curtest.zte && curtest.ztst == 0) || g_bIsLost ) {
		curvb.dwCount = 0;
		return;
	}

	if( s_RangeMngr.ranges.size() > 0 ) {

		// don't want infinite loop
		DWORD prevcount = curvb.dwCount;
		curvb.dwCount = 0;
		FlushTransferRanges(curvb.curprim.tme ? &curvb.tex0 : NULL);

		curvb.dwCount = prevcount;
		//if( curvb.dwCount == 0 )
		//	return;
	}

	if( curvb.bNeedTexCheck ) {
		curvb.FlushTexData();

		if( curvb.dwCount == 0 )
			return;
	}

	if( !s_bBeginScene ) {
		pd3dDevice->BeginScene();
		s_bBeginScene = TRUE;
	}

	curvb.Unlock();

	LPD3DTEX ptexRenderTargetCached = NULL;
	int cachedtbp0, cachedtbw, cachedtbh;

	//s_bWriteDepth = TRUE;

	//static int lasttime = 0;
	//fprintf(gsLog, "%d: %d\n", g_SaveFrameNum, timeGetTime()-lasttime);
	//lasttime = timeGetTime();

	if( curvb.bNeedFrameCheck || curvb.bNeedZCheck ) {

		int tpsm = curvb.tex0.psm;
		if( curvb.bNeedTexCheck )
			tpsm = (curvb.uNextTex0Data[0] >> 20) & 0x3f;

		if( tpsm == PSMT8H && (g_GameSettings&GAME_NOTARGETCLUT) ) {
			curvb.dwCount = 0;
			return;
		}

		// check for the texture before checking the frame (since things could get destroyed)
		if( (g_GameSettings&GAME_PARTIALPOINTERS) &&curvb.curprim.tme  ) {

//			if( (curvb.gsfb.fbp&0xff) != 0 ) {
//				curvb.dwCount = 0;
//				return;
//			}
		
			// if texture is part of a previous target, use that instead
			int tbw = curvb.tex0.tbw;
			int tbp0 = curvb.tex0.tbp0;

			if( curvb.bNeedTexCheck ) {
				// not yet initied, but still need to get correct target! (xeno3 ingame)
				tbp0 =  (curvb.uNextTex0Data[0]		& 0x3fff);
				tbw  = ((curvb.uNextTex0Data[0] >> 14) & 0x3f) * 64;
			}

			if( (tpsm&~1) == 0 ) {
				CRenderTarget* ptemptarg = s_RTs.GetTarg(tbp0, tbw);
				if( ptemptarg != NULL && (ptemptarg->psm&~1) == (tpsm&~1) ) {
					ptexRenderTargetCached = ptemptarg->ptex;
					ptexRenderTargetCached->AddRef();
					cachedtbp0 = ptemptarg->fbp;
					cachedtbw = ptemptarg->fbw;
					cachedtbh = ptemptarg->fbh;
				}
			}
		}
		curvb.CheckFrame(curvb.curprim.tme ? curvb.tex0.tbp0 : 0);
	}

//	if( g_SaveFrameNum == 976 ) {
//		curvb.prndr->ConvertTo32();
//	}
	if( curvb.prndr == NULL || curvb.pdepth == NULL ) {
		WARN_LOG("Current render target NULL (ctx: %d)", context);
		curvb.dwCount = 0;
		SAFE_RELEASE(ptexRenderTargetCached);
		return;
	}

#if defined(PRIM_LOG) && defined(_DEBUG)
	static const char* patst[8] = { "NEVER", "ALWAYS", "LESS", "LEQUAL", "EQUAL", "GEQUAL", "GREATER", "NOTEQUAL"};
	static const char* pztst[4] = { "NEVER", "ALWAYS", "GEQUAL", "GREATER" };
	static const char* pafail[4] = { "KEEP", "FB_ONLY", "ZB_ONLY", "RGB_ONLY" };
	PRIM_LOG("**Drawing ctx %d, num %d, fbp: 0x%x, zbp: 0x%x, fpsm: %d, zpsm: %d, fbw: %d\n", context, vb[context].dwCount, curvb.prndr->fbp, curvb.zbuf.zbp, curvb.prndr->psm, curvb.zbuf.psm, curvb.prndr->fbw);
	PRIM_LOG("prim: prim=%x iip=%x tme=%x fge=%x abe=%x aa1=%x fst=%x ctxt=%x fix=%x\n",
			 curvb.curprim.prim, curvb.curprim.iip, curvb.curprim.tme, curvb.curprim.fge, curvb.curprim.abe, curvb.curprim.aa1, curvb.curprim.fst, curvb.curprim.ctxt, curvb.curprim.fix);
	PRIM_LOG("test: ate:%d, atst: %s, aref: %d, afail: %s, date: %d, datm: %d, zte: %d, ztst: %s, fba: %d\n",
		curvb.test.ate, patst[curvb.test.atst], curvb.test.aref, pafail[curvb.test.afail], curvb.test.date, curvb.test.datm, curvb.test.zte, pztst[curvb.test.ztst], curvb.fba.fba);
	PRIM_LOG("alpha: A%d B%d C%d D%d FIX:%d pabe: %d; aem: %d, ta0: %d, ta1: %d\n", curvb.alpha.a, curvb.alpha.b, curvb.alpha.c, curvb.alpha.d, curvb.alpha.fix, gs.pabe, gs.texa.aem, gs.texa.ta[0], gs.texa.ta[1]);
	PRIM_LOG("tex0: tbp0=0x%x, tbw=%d, psm=0x%x, tw=%d, th=%d, tcc=%d, tfx=%d, cbp=0x%x, cpsm=0x%x, csm=%d, csa=%d, cld=%d\n", 
			 curvb.tex0.tbp0, curvb.tex0.tbw, curvb.tex0.psm, curvb.tex0.tw, 
			 curvb.tex0.th, curvb.tex0.tcc, curvb.tex0.tfx, curvb.tex0.cbp,
		 	 curvb.tex0.cpsm, curvb.tex0.csm, curvb.tex0.csa, curvb.tex0.cld);
	PRIM_LOG("frame: %d\n\n", g_SaveFrameNum);
#endif
	
	CMemoryTarget* pmemtarg = NULL;
	CRenderTarget* ptextarg = NULL;

	// kh2 hack
//	if( curvb.dwCount == 2 && curvb.curprim.tme == 0 && curvb.curprim.abe == 0 && (curvb.tex0.tbp0 == 0x2a00 || curvb.tex0.tbp0==0x1d00) ) {
//		// skip
//		DEBUG_LOG("skipping\n");
//		g_SaveFrameNum++;
//		curvb.dwCount = 0;
//		return;
//	}

	if( curtest.date || gs.pabe )
		SetDestAlphaTest();

	// set the correct pixel shaders
	if( curvb.curprim.tme && ptexRenderTargetCached == NULL ) {

		// if texture is part of a previous target, use that instead
		int tbw = curvb.tex0.tbw;
		int tbp0 = curvb.tex0.tbp0;
		int tpsm = curvb.tex0.psm;

		if( curvb.bNeedTexCheck ) {
			// not yet initied, but still need to get correct target! (xeno3 ingame)
			tbp0 =  (curvb.uNextTex0Data[0]		& 0x3fff);
			tbw  = ((curvb.uNextTex0Data[0] >> 14) & 0x3f) * 64;
			tpsm = (curvb.uNextTex0Data[0] >> 20) & 0x3f;
		}
		ptextarg = s_RTs.GetTarg(tbp0, tbw);

		if( ptextarg == NULL && tpsm == PSMT8 ) {
			// check for targets with half the width
			ptextarg = s_RTs.GetTarg(tbp0, tbw/2);
			if( ptextarg == NULL ) {
				tbp0 &= ~0x7ff;
				ptextarg = s_RTs.GetTarg(tbp0, tbw/2); // mgs3 hack

				if( ptextarg == NULL ) {
					// check the next level (mgs3)
					tbp0 &= ~0xfff;
					ptextarg = s_RTs.GetTarg(tbp0, tbw/2); // mgs3 hack
				}

				if( ptextarg != NULL && ptextarg->start > tbp0*256 ) {
					// target beyond range, so ignore
					ptextarg = NULL;
				}
			}

//			if( ptextarg != NULL ) {
//				// make sure target isn't invalidated by the ranges
//				for(vector<CRangeManager::RANGE>::iterator itrange = s_RangeMngr.ranges.begin(); itrange != s_RangeMngr.ranges.end(); ++itrange ) {
//
//					int start = itrange->start;
//					int end = itrange->end;
//
//					// if start and end are in the range or there's a range that is between tbp0 and start, then remove
//					if( (start <= tbp0*256 && end > tbp0*256) || (start >= ptextarg->fbp*256 && start <= tbp0*256) ) {
//						ptextarg = NULL;
//						break;
//					}
//				}
//			}

			if( ptextarg != NULL && !(ptextarg->status&CRenderTarget::TS_NeedUpdate) ) {
				// find the equivalent memtarg
				if( s_PSM8Resolve == 0 ) { //|| (s_PSM8Resolve > 0 && s_PSM8Resolve+128 < g_SaveFrameNum) ) {
					DWORD prevcount = curvb.dwCount;
					curvb.dwCount = 0;
					if( ptextarg->pmimicparent != NULL )
						ptextarg->pmimicparent->Resolve();
					else
						ptextarg->Resolve();
					curvb.dwCount = prevcount;
					s_PSM8Resolve = g_SaveFrameNum; // stop from resolving again (once per frame)
				}

				tex0Info mytex0 = curvb.tex0;
				mytex0.tbp0 = tbp0;
				if( ptextarg->pmimicparent != NULL ) {
					mytex0.tbp0 = ptextarg->pmimicparent->fbp;
				}
				pmemtarg = g_MemTargs.GetMemoryTarget(mytex0, 1);

				// have to add an offset to all texture reads
				mytex0.tbp0 = tbp0; // change so that SetTexVariablesInt can set the right offsets

				SetTexVariablesInt(context, GetTexFilter(curvb.tex1), mytex0, pmemtarg, s_bForceTexFlush);	
				curvb.bVarsTexSync = TRUE;

				ptextarg = NULL; // won't be needing this anymore
			}
		}

		if( (tpsm&0x30)==0x30 && ptextarg == NULL ) {
			// try depth
			ptextarg = s_DepthRTs.GetTarg(tbp0, tbw);
		}

		if( ptextarg == NULL && (g_GameSettings&GAME_TEXTURETARGS) ) {
			// check if any part of the texture intersects the current target
			if( !PSMT_ISCLUT(tpsm) && curvb.tex0.tbp0 >= curvb.frame.fbp && (curvb.tex0.tbp0 << 8) < curvb.prndr->end) {
				ptextarg = curvb.prndr;
			}
		}

		if( ptextarg != NULL && !(ptextarg->status&CRenderTarget::TS_NeedUpdate) ) {
			if( PSMT_ISCLUT(tpsm) && tpsm != PSMT8H && tpsm != PSMT8 ) { // handle 8h cluts
				// don't support clut targets, read from mem
				// 4hl - kh2 check
				if( tpsm != PSMT4HL && tpsm != PSMT4HH && s_ClutResolve <= 1 ) { // xenosaga requires 2 resolves
					DWORD prevcount = curvb.dwCount;
					curvb.dwCount = 0;
					ptextarg->Resolve();
					s_ClutResolve++;
					curvb.dwCount = prevcount;
				}
				ptextarg = NULL;
			}
			else {
				if( ptextarg == curvb.prndr ) {
					// need feedback
					if( ptextarg->pmimicparent != NULL ) {
						// if the target is mimic, create the feedback of the parent
						assert( ptextarg->pmimicparent->ptex == ptextarg->ptex || ptextarg->pmimicparent->ptexFeedback == ptextarg->ptex );
						SAFE_RELEASE(ptextarg->ptexFeedback);
						SAFE_RELEASE(ptextarg->psurfFeedback);
						ptextarg->pmimicparent->CreateFeedback();
						ptextarg->ptex = ptextarg->pmimicparent->ptex;
						ptextarg->ptexFeedback = ptextarg->pmimicparent->ptexFeedback; ptextarg->ptexFeedback->AddRef();
						ptextarg->psurf = ptextarg->pmimicparent->psurf;
						ptextarg->psurfFeedback = ptextarg->pmimicparent->psurfFeedback; ptextarg->psurfFeedback->AddRef();
					}
					else
						curvb.prndr->CreateFeedback();

					pd3dDevice->SetRenderTarget(1, (s_bWriteDepth && curvb.pdepth != NULL) ? curvb.pdepth->psurf : NULL);
				}
			}
		}
		else ptextarg = NULL;
	}

#ifdef _DEBUG
	if( g_bSaveFlushedFrame & 0x80000000 ) {
		char str[255];
		sprintf(str, "rndr.tga", g_SaveFrameNum);
		D3DXSaveSurfaceToFile(str, D3DXIFF_TGA, curvb.prndr->psurf, NULL, NULL);
	}
#endif

	if( conf.options & GSOPTION_WIREFRAME ) {
		// always render first few geometry as solid
		if( s_nWireframeCount > 0 ) {
			SETRS(D3DRS_FILLMODE, D3DFILL_SOLID);
		}
	}

	if( !curvb.bVarsSetTarg )
		SetContextTarget(context);
	else {
		assert( curvb.pdepth != NULL );
		
		if( curvb.pdepth->status & CRenderTarget::TS_Virtual) {
		
			if( !curvb.zbuf.zmsk ) {
				CRenderTarget* ptemp = s_DepthRTs.Promote(curvb.pdepth->fbp|(curvb.pdepth->fbw<<16));
				assert( ptemp == curvb.pdepth );
			}
			else
				curvb.pdepth->status &= ~CRenderTarget::TS_NeedUpdate;
		}

		if( (curvb.pdepth->status & CRenderTarget::TS_NeedUpdate) || (curvb.prndr->status & CRenderTarget::TS_NeedUpdate) )
			SetContextTarget(context);
	}

	SetTexVariables(context);
	if( ptextarg == NULL && pmemtarg == NULL ) {
		pmemtarg = g_MemTargs.GetMemoryTarget(curvb.tex0, 1);

		if( vb[context].bVarsTexSync ) {
			if( vb[context].pmemtarg != pmemtarg ) {
				SetTexVariablesInt(context, GetTexFilter(curvb.tex1), curvb.tex0, pmemtarg, s_bForceTexFlush);
				vb[context].bVarsTexSync = TRUE;
			}
		}
		else {
			SetTexVariablesInt(context, GetTexFilter(curvb.tex1), curvb.tex0, pmemtarg, s_bForceTexFlush);	
			vb[context].bVarsTexSync = TRUE;

			INC_TEXVARS();
		}
	}

	icurctx = context;

	assert( !(curvb.prndr->status&CRenderTarget::TS_NeedUpdate) );
	curvb.prndr->status = 0;
	
	if( curvb.pdepth != NULL ) {
		assert( !(curvb.pdepth->status&CRenderTarget::TS_NeedUpdate) );
		if( !curvb.zbuf.zmsk ) {
			assert( !(curvb.pdepth->status & CRenderTarget::TS_Virtual) );
			curvb.pdepth->status = 0;
		}
	}

	s_dwColorWrite = (curvb.prndr->psm&0xf) == 1 ? (D3DCOLORWRITEENABLE_BLUE|D3DCOLORWRITEENABLE_GREEN|D3DCOLORWRITEENABLE_RED) : 0xf;

	if( ((curvb.frame.fbm)&0xff) == 0xff) s_dwColorWrite &= ~D3DCOLORWRITEENABLE_RED;
	if( ((curvb.frame.fbm>>8)&0xff) == 0xff) s_dwColorWrite &= ~D3DCOLORWRITEENABLE_GREEN;
	if( ((curvb.frame.fbm>>16)&0xff) == 0xff) s_dwColorWrite &= ~D3DCOLORWRITEENABLE_BLUE;
	if( ((curvb.frame.fbm>>24)&0xff) == 0xff) s_dwColorWrite &= ~D3DCOLORWRITEENABLE_ALPHA;

	SETRS(D3DRS_COLORWRITEENABLE, s_dwColorWrite);

	pd3dDevice->SetScissorRect(&curvb.prndr->scissorrect); // need to always set it since something in this code resets it

	// set the shaders
	pd3dDevice->SetVertexShader(pvs[2*((curvb.curprim._val>>1)&3)+8*s_bWriteDepth+context]);
	pd3dDevice->SetStreamSource(0, curvb.pvb, curvb.dwCurOff*sizeof(VertexGPU), sizeof(VertexGPU));

	DWORD dwUsingSpecialTesting = 0;
	DWORD dwFilterOpts = 0;

	IDirect3DPixelShader9* pps;

	// need exact if equal or notequal
	int exactcolor = 0;
	if( g_nPixelShaderVer != SHADER_20 )
		// ffx2 breaks when ==7
		exactcolor = (curtest.ate && curtest.aref <= 128) && (curtest.atst==4);//||curtest.atst==7);

	int shadertype = 0;

	// set the correct pixel shaders
	if( curvb.curprim.tme ) {

		if( curvb.ptexClamp[0] != NULL ) pd3dDevice->SetTexture(SAMP_BITWISEANDX, curvb.ptexClamp[0]);
		if( curvb.ptexClamp[1] != NULL ) pd3dDevice->SetTexture(SAMP_BITWISEANDY, curvb.ptexClamp[1]);

		if( ptexRenderTargetCached != NULL ) {
			DXVEC4 vpageoffset;
			vpageoffset.w = 0;

			int psm = curvb.tex0.psm;
			assert( !PSMT_ISCLUT(curvb.tex0.psm));

			pps = LoadShadeEffect(1, 0, curvb.curprim.fge, curvb.tex0.tcc && gs.texa.aem && (psm == PSMCT24 || psm == PSMCT16 || psm == PSMCT16S),
				exactcolor, curvb.clamp, context);
			pd3dDevice->SetTexture(SAMP_MEMORY0+context, ptexRenderTargetCached);
			s_ptexCurSet[context] = ptexRenderTargetCached;

			if( curvb.tex1.mmag ) {
				pd3dDevice->SetSamplerState(SAMP_MEMORY0+context, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
				dwFilterOpts |= 1;
			}
			
			if( curvb.tex1.mmin ) {
				pd3dDevice->SetSamplerState(SAMP_MEMORY0+context, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
				dwFilterOpts |= 2;
			}

			DXVEC4 vTexDims;
			vTexDims.x = curvb.tex0.tw / (float)cachedtbw;
			vTexDims.y = curvb.tex0.th / (float)cachedtbh;

//			u32 tbp0 = curvb.tex0.tbp0 >> 5; // align to a page
//			int blockheight = 32;
//			int ycoord = ((curvb.tex0.tbp0-cachedtbp0)/(32*(cachedtbw>>6))) * blockheight;
//			int xcoord = (((curvb.tex0.tbp0-cachedtbp0)%(32*(cachedtbw>>6)))) * 2;
////				xcoord += ptextarg->targoffx;
////				ycoord += ptextarg->targoffy;
//			vTexDims.z = (float)xcoord / (float)cachedtbw;
//			vTexDims.w = (float)ycoord / (float)cachedtbh;
			vTexDims.z = vTexDims.w = 0;
			SETCONSTF(GPU_TEXDIMS0+context, vTexDims);

			SETCONSTF(GPU_PAGEOFFSET0+context, vpageoffset);

			if( g_bSaveTex )
				D3DXSaveTextureToFile("tex.tga", D3DXIFF_TGA, ptexRenderTargetCached, NULL);
		}
		else if( ptextarg != NULL ) {

			if( ptextarg->IsDepth() )
				SetWriteDepth();

			DXVEC4 vpageoffset;
			vpageoffset.w = 0;

			shadertype = 1;
			if( (curvb.tex0.psm == PSMT8 || curvb.tex0.psm == PSMT8H) && !(g_GameSettings&GAME_NOTARGETCLUT) ) {
				// load the clut to memory
				LPD3DTEX ptexclut = NULL;
				pd3dDevice->CreateTexture(256, 1, 1, 0, (curvb.tex0.cpsm&2) ? D3DFMT_A1R5G5B5 : D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &ptexclut, NULL);
				if( ptexclut != NULL ) {

					D3DLOCKED_RECT lock;
					ptexclut->LockRect(0, &lock, NULL, D3DLOCK_NOSYSLOCK);

					// fill the buffer by decoding the clut
					int nClutOffset = 0, clutsize;
					int entries = (curvb.tex0.psm&3)==3 ? 256 : 16;
					if( curvb.tex0.cpsm <= 1 ) { // 32 bit
						nClutOffset = 64 * curvb.tex0.csa;
						clutsize = min(entries, 256-curvb.tex0.csa*16)*4;
					}
					else {
						nClutOffset = 64 * (curvb.tex0.csa&15) + (curvb.tex0.csa>=16?2:0);
						clutsize = min(entries, 512-curvb.tex0.csa*16)*2;
					}

					if( curvb.tex0.cpsm <= 1 ) { // 32 bit
						memcpy_amd(lock.pBits, ZeroGS::g_pbyGSClut+nClutOffset, clutsize);
					}
					else {
						u16* pClutBuffer = (u16*)(ZeroGS::g_pbyGSClut + nClutOffset);
						u16* pclut = (u16*)lock.pBits;
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

					ptexclut->UnlockRect(0);

					s_vecTempTextures.push_back(ptexclut);
					pd3dDevice->SetTexture(SAMP_FINAL, ptexclut);

					if( g_bSaveTex )
						D3DXSaveTextureToFile("clut.tga", D3DXIFF_TGA, ptexclut, NULL);
				}

				if( g_nPixelShaderVer != SHADER_20 && (ptextarg->psm & 2) ) {
					// 16 bit texture
					shadertype = 4;

					DXVEC4 v;
					v.x = 16.0f / (float)ptextarg->fbw;
					v.y = 64.0f / (float)ptextarg->fbh;
					v.z = 0.5f * v.x;
					v.w = 0.5f * v.y;
					SETCONSTF(GPU_TEXOFFSET0, v);

					v.x = 1;
					v.y = -0.5f;
					v.z = 0;
					v.w = 0.0001f;
					SETCONSTF(GPU_PAGEOFFSET0, v);

					pd3dDevice->SetSamplerState(SAMP_BILINEARBLOCKS, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
					pd3dDevice->SetSamplerState(SAMP_BILINEARBLOCKS, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
					pd3dDevice->SetTexture(SAMP_BILINEARBLOCKS, ptexConv32to16);
				}
				else
					shadertype = 2;
			}
			else {
				if( PSMT_ISCLUT(curvb.tex0.psm) )
					WARN_LOG("Using render target with CLUTs %d!\n", curvb.tex0.psm);
				else {
					if( (curvb.tex0.psm&2) != (ptextarg->psm&2) && (g_nPixelShaderVer != SHADER_20 || !curvb.curprim.fge) ) {
						if( curvb.tex0.psm & 2 ) {
							// converting from 32->16
							shadertype = 3;

							DXVEC4 v;
							v.x = 16.0f / (float)curvb.tex0.tw;
							v.y = 64.0f / (float)curvb.tex0.th;
							v.z = 0.5f * v.x;
							v.w = 0.5f * v.y;
							SETCONSTF(GPU_TEXOFFSET0+context, v);

							vpageoffset.x = -0.1f / 256.0f;
							vpageoffset.y = -0.001f / 256.0f;
							vpageoffset.z = -0.1f / ptextarg->fbh;
							vpageoffset.w = ((ptextarg->psm&0x30)==0x30)?-1.0f:0.0f;

							pd3dDevice->SetSamplerState(SAMP_BILINEARBLOCKS, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
							pd3dDevice->SetSamplerState(SAMP_BILINEARBLOCKS, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
							pd3dDevice->SetTexture(SAMP_BILINEARBLOCKS, ptexConv16to32);
						}
						else {
							// converting from 16->32
							WARN_LOG("ZeroGS: converting from 16 to 32bit RTs\n");
							//shadetype = 4;
						}
					}
				}
			}

			int psm = curvb.tex0.psm;
			if( PSMT_ISCLUT(curvb.tex0.psm) ) psm = curvb.tex0.cpsm;

			pps = LoadShadeEffect(shadertype, 0, curvb.curprim.fge, curvb.tex0.tcc && gs.texa.aem && (psm == PSMCT24 || psm == PSMCT16 || psm == PSMCT16S),
				exactcolor, curvb.clamp, context);
			LPD3DTEX ptexset = ptextarg == curvb.prndr ? ptextarg->ptexFeedback : ptextarg->ptex;
			pd3dDevice->SetTexture(SAMP_MEMORY0+context, ptexset);
			s_ptexCurSet[context] = ptexset;

			if( curvb.tex1.mmag ) {
				pd3dDevice->SetSamplerState(SAMP_MEMORY0+context, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
				dwFilterOpts |= 1;
			}
			
			if( curvb.tex1.mmin ) {
				pd3dDevice->SetSamplerState(SAMP_MEMORY0+context, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
				dwFilterOpts |= 2;
			}

			DXVEC4 vTexDims;
			vTexDims.x = curvb.tex0.tw / (float)ptextarg->fbw;
			vTexDims.y = curvb.tex0.th / (float)ptextarg->targheight;

			// look at the offset of tbp0 from fbp
			if( curvb.tex0.tbp0 <= ptextarg->fbp ) {
				vTexDims.z = 0;//-0.5f/(float)ptextarg->fbw;
				vTexDims.w = 0;//0.2f/(float)ptextarg->fbh;
			}
			else {
				u32 tbp0 = curvb.tex0.tbp0 >> 5; // align to a page
				int blockheight = (ptextarg->psm&2) ? 64 : 32;
				int ycoord = ((curvb.tex0.tbp0-ptextarg->fbp)/(32*(ptextarg->fbw>>6))) * blockheight;
				int xcoord = (((curvb.tex0.tbp0-ptextarg->fbp)%(32*(ptextarg->fbw>>6)))) * 2;
				xcoord += ptextarg->targoffx;
				ycoord += ptextarg->targoffy;
				vTexDims.z = (float)xcoord / (float)ptextarg->fbw;
				vTexDims.w = (float)ycoord / (float)ptextarg->targheight;
			}

			if( shadertype == 4 ) {
				vTexDims.z += 8.0f / (float)ptextarg->fbw;
			}

			SETCONSTF(GPU_TEXDIMS0+context, vTexDims);

			// zoe2
			if( (ptextarg->psm&0x30) == 0x30 ) {//&& (psm&2) == (ptextarg->psm&2) ) {
				// target of zbuf has +1 added to it, don't do 16bit
				vpageoffset.w = -1;
//				DXVEC4 valpha2;
//				valpha2.x = 1; valpha2.y = 0;
//				valpha2.z = -1; valpha2.w = 0;
//				SETCONSTF(GPU_TEXALPHA20+context, &valpha2);
			}
			SETCONSTF(GPU_PAGEOFFSET0+context, vpageoffset);

			if( g_bSaveTex )
				D3DXSaveTextureToFile("tex.tga", D3DXIFF_TGA, ptextarg == curvb.prndr ? ptextarg->ptexFeedback : ptextarg->ptex, NULL);
		}
		else {
			// save the texture
#ifdef _DEBUG
//			CMemoryTarget* pmemtarg = g_MemTargs.GetMemoryTarget(curvb.tex0, 0);
//			assert( curvb.pmemtarg == pmemtarg );
//			if( PSMT_ISCLUT(curvb.tex0.psm) )
//				assert( curvb.pmemtarg->ValidateClut(curvb.tex0) );
#endif

//#ifdef ZEROGS_CACHEDCLEAR
//			if( !curvb.pmemtarg->ValidateTex(curvb.tex0, true) ) {
//				CMemoryTarget* pmemtarg = g_MemTargs.GetMemoryTarget(curvb.tex0, 1);
//				SetTexVariablesInt(context, GetTexFilter(curvb.tex1), curvb.tex0, pmemtarg, s_bForceTexFlush);
//				vb[context].bVarsTexSync = TRUE;
//			}
//#endif

			if( g_bSaveTex ) {
				if( g_bSaveTex == 1 ) SaveTex(&curvb.tex0, 1);
				else SaveTex(&curvb.tex0, 0);
			}

			int psm = curvb.tex0.psm;
			if( PSMT_ISCLUT(curvb.tex0.psm) ) psm = curvb.tex0.cpsm;

			pps = LoadShadeEffect(0, GetTexFilter(curvb.tex1), curvb.curprim.fge,
				curvb.tex0.tcc && gs.texa.aem && (psm == PSMCT24 || psm == PSMCT16 || psm == PSMCT16S),
				exactcolor, curvb.clamp, context);
		}
	}
	else pps = ppsRegular[curvb.curprim.fge+2*s_bWriteDepth];

	pd3dDevice->SetPixelShader(pps);
		
	BOOL bCanRenderStencil = g_bUpdateStencil && (curvb.prndr->psm&0xf) != 1 && !(curvb.frame.fbm&0x80000000);
	if( g_GameSettings & GAME_NOSTENCIL )
		bCanRenderStencil = 0;

	if( s_bDestAlphaTest) {
		SETRS(D3DRS_STENCILENABLE, bCanRenderStencil);		
		SETRS(D3DRS_STENCILPASS, D3DSTENCILOP_KEEP);
		SETRS(D3DRS_STENCILFUNC, D3DCMP_ALWAYS);
	}
	else SETRS(D3DRS_STENCILENABLE, 0);

	SETRS(D3DRS_ZWRITEENABLE, !curvb.zbuf.zmsk);
	SETRS(D3DRS_ZENABLE, curtest.zte);
	if( curtest.zte ) {
		if( curtest.ztst > 1 ) g_nDepthUsed = 2;
		if( (curtest.ztst == 2) ^ (g_nDepthBias != 0) ) {
			g_nDepthBias = curtest.ztst == 2;
			if( g_GameSettings & GAME_RELAXEDDEPTH )
				SETRS(D3DRS_DEPTHBIAS, g_nDepthBias?FtoDW(0.00003f):FtoDW(0.0001f));
			else
				SETRS(D3DRS_DEPTHBIAS, g_nDepthBias?FtoDW(0.0003f):FtoDW(0.000015f));
		}

		SETRS(D3DRS_ZFUNC, g_dwZCmp[curtest.ztst]);

//		if( curtest.ztst == 3 ) {
//			// gequal
//			if( s_vznorm.y == 0 ) {
//				s_vznorm.y = 0.00001f;
//				SETCONSTF(GPU_ZNORM, s_vznorm);
//			}
//		}
//		else {
//			if( s_vznorm.y > 0 ) {
//				s_vznorm.y = 0;
//				SETCONSTF(GPU_ZNORM, s_vznorm);
//			}
//		}
	}

	SETRS(D3DRS_ALPHATESTENABLE, curtest.ate&&USEALPHATESTING);
	if( curtest.ate ) {

		if( curtest.atst == 7 && curtest.aref == 255 ) {
			// when it is at the very top, do a less than rather than not equal (gekibo2)
			SETRS(D3DRS_ALPHAFUNC, D3DCMP_LESS);
			SETRS(D3DRS_ALPHAREF, 255);
		}
		else {
			SETRS(D3DRS_ALPHAFUNC, g_dwAlphaCmp[curtest.atst]);
			SETRS(D3DRS_ALPHAREF, b2XAlphaTest ? min(255,2 * curtest.aref) : curtest.aref);
		}
	}

	if( s_bWriteDepth ) {
		//pd3dDevice->SetRenderTarget(0, curvb.prndr->psurf);
		//pd3dDevice->SetRenderTarget(1, !curvb.zbuf.zmsk?curvb.pdepth->psurf:NULL);
		if( bIndepWriteMasks )
			SETRS(D3DRS_COLORWRITEENABLE1, !curvb.zbuf.zmsk?0xf:0);
		else
			pd3dDevice->SetRenderTarget(1, !curvb.zbuf.zmsk?curvb.pdepth->psurf:NULL);
	}

	if( curvb.curprim.abe )
		SetAlphaVariables(curvb.alpha);
	else
		SETRS(D3DRS_ALPHABLENDENABLE, 0);

	// needs to be before RenderAlphaTest
	if( curvb.fba.fba || s_bDestAlphaTest ) {
		if( gs.pabe || (curvb.fba.fba || bCanRenderStencil) && !(curvb.frame.fbm&0x80000000) ) {
			RenderFBA(curvb);
		}
	}

	u32 oldabe = curvb.curprim.abe;
	if( gs.pabe ) {
		//WARN_LOG("PBE!\n");
		curvb.curprim.abe = 1;
		SETRS(D3DRS_ALPHABLENDENABLE, USEALPHABLENDING);
	}

	if( curvb.curprim.abe && bNeedAlphaColor ) {

		if( //bCanRenderStencil &&
			(bNeedBlendFactorInAlpha || ((curtest.ate && curtest.atst>1) && (curtest.aref > 0x80))) ) {
			// need special stencil processing for the alpha
			RenderAlphaTest(curvb);
			dwUsingSpecialTesting = 1;
		}

		// harvest fishing
		DXVEC4 v = vAlphaBlendColor;// + DXVEC4(0,0,0,(curvb.test.atst==4 && curvb.test.aref>=128)?-0.004f:0);
		if( exactcolor ) { v.y *= 255; v.w *= 255; }
		SETCONSTF(GPU_ONECOLOR, v);
	}
	else {
		// not using blending so set to defaults
		DXVEC4 v = exactcolor ? DXVEC4(1, 510*255.0f/256.0f, 0, 0) : DXVEC4(1,2*255.0f/256.0f,0,0);
		SETCONSTF(GPU_ONECOLOR, v);
	}

	if( s_bDestAlphaTest && bCanRenderStencil ) {
		// if not 24bit and can write to high alpha bit
		RenderStencil(curvb, dwUsingSpecialTesting);
	}
	else {
		dwStencilRef = STENCIL_SPECIAL;
		dwStencilMask = STENCIL_SPECIAL;

		// setup the stencil to only accept the test pixels
		if( dwUsingSpecialTesting ) {
			SETRS(D3DRS_STENCILENABLE, TRUE);
			SETRS(D3DRS_STENCILWRITEMASK, STENCIL_PIXELWRITE);
			SETRS(D3DRS_STENCILMASK, STENCIL_SPECIAL);
			SETRS(D3DRS_STENCILPASS, D3DSTENCILOP_REPLACE);
			SETRS(D3DRS_STENCILFUNC, D3DCMP_EQUAL);
			SETRS(D3DRS_STENCILREF, STENCIL_SPECIAL|STENCIL_PIXELWRITE);
		}
	}

#ifdef _DEBUG
	if( bDestAlphaColor == 1 ) {
		WARN_LOG("dest alpha blending! manipulate alpha here\n");
	}
#endif

	if( bCanRenderStencil && gs.pabe ) {
		// only render the pixels with alpha values >= 0x80
		SETRS(D3DRS_STENCILREF, dwStencilRef|STENCIL_FBA);
		SETRS(D3DRS_STENCILMASK, dwStencilMask|STENCIL_FBA);
		if( !dwStencilMask ) SETRS(D3DRS_STENCILFUNC, D3DCMP_EQUAL);
	}

//	curvb.prndr->SetViewport();
//	pd3dDevice->SetScissorRect(&curvb.prndr->scissorrect);
//	SETRS(D3DRS_SCISSORTESTENABLE, TRUE);

	if( !curvb.test.ate || curvb.test.atst > 0 ) {
		DRAW();
	}

	if( gs.pabe ) {
		// only render the pixels with alpha values < 0x80
		SETRS(D3DRS_ALPHABLENDENABLE, 0);
		SETRS(D3DRS_STENCILREF, dwStencilRef);

		DXVEC4 v;
		v.x = 1; v.y = 2; v.z = 0; v.w = 0;
		if( exactcolor ) v.y *= 255;
		SETCONSTF(GPU_ONECOLOR, v);

		DRAW();

		// reset
		SETRS(D3DRS_STENCILMASK, dwStencilMask);
		if( !dwStencilMask ) SETRS(D3DRS_STENCILFUNC, D3DCMP_ALWAYS);
	}

	// more work on alpha failure case
	if( curtest.ate && curtest.atst != 1 && curtest.afail > 0 ) {

		// need to reverse the test and disable some targets
		SETRS(D3DRS_ALPHAFUNC, g_dwReverseAlphaCmp[curtest.atst]);

		if( curtest.afail & 1 ) {	// front buffer update only
			
			if( curtest.afail == 3 ) // disable alpha
				SETRS(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_BLUE|D3DCOLORWRITEENABLE_GREEN|D3DCOLORWRITEENABLE_RED);

			SETRS(D3DRS_ZWRITEENABLE, FALSE);

			if( s_bWriteDepth ) {
				if( bIndepWriteMasks ) SETRS(D3DRS_COLORWRITEENABLE1, 0);
				else pd3dDevice->SetRenderTarget(1,NULL);
			}
		}
		else {
			// zbuffer update only
			SETRS(D3DRS_COLORWRITEENABLE, 0);	
		}

		if( gs.pabe && bCanRenderStencil ) {
			// only render the pixels with alpha values >= 0x80
			DXVEC4 v = vAlphaBlendColor;
			if( exactcolor ) { v.y *= 255; v.w *= 255; }
			SETCONSTF(GPU_ONECOLOR, v);
			SETRS(D3DRS_ALPHABLENDENABLE, USEALPHABLENDING);
			SETRS(D3DRS_STENCILREF, dwStencilRef|STENCIL_FBA);
			SETRS(D3DRS_STENCILMASK, dwStencilMask|STENCIL_FBA);
			if( !dwStencilMask ) SETRS(D3DRS_STENCILFUNC, D3DCMP_EQUAL);
		}

		// setup the stencil to only accept the test pixels
		if( dwUsingSpecialTesting ) {

			if( !s_bDestAlphaTest || !bCanRenderStencil ) {
				SETRS(D3DRS_STENCILENABLE, FALSE);
			}
		}

//		IDirect3DQuery9* pOcclusionQuery;
//		DWORD numberOfPixelsDrawn;
//
//		pd3dDevice->CreateQuery(D3DQUERYTYPE_OCCLUSION, &pOcclusionQuery);
//
//		// Add an end marker to the command buffer queue.
//		pOcclusionQuery->Issue(D3DISSUE_BEGIN);

		DRAW();

//		pOcclusionQuery->Issue(D3DISSUE_END);
		// Force the driver to execute the commands from the command buffer.
		// Empty the command buffer and wait until the GPU is idle.
//		while(S_FALSE == pOcclusionQuery->GetData( &numberOfPixelsDrawn, sizeof(DWORD), D3DGETDATA_FLUSH ));
//		SAFE_RELEASE(pOcclusionQuery);
	
		if( gs.pabe ) {
			// only render the pixels with alpha values < 0x80
			SETRS(D3DRS_ALPHABLENDENABLE, 0);
			SETRS(D3DRS_STENCILREF, dwStencilRef);

			DXVEC4 v;
			v.x = 1; v.y = 2; v.z = 0; v.w = 0;
			if( exactcolor ) v.y *= 255;
			SETCONSTF(GPU_ONECOLOR, v);

			DRAW();

			// reset
			SETRS(D3DRS_STENCILMASK, dwStencilMask);
			SETRS(D3DRS_ALPHABLENDENABLE, oldabe);
			if( !dwStencilMask ) SETRS(D3DRS_STENCILFUNC, D3DCMP_ALWAYS);
		}

		// restore
		
		if( (curtest.afail & 1) && !curvb.zbuf.zmsk ) {
			SETRS(D3DRS_ZWRITEENABLE, TRUE);

			if( s_bWriteDepth ) {
				assert( curvb.pdepth != NULL);
				if( bIndepWriteMasks ) SETRS(D3DRS_COLORWRITEENABLE1, 0xf);
				else pd3dDevice->SetRenderTarget(1,curvb.pdepth->psurf);
			}
		}

		SETRS(D3DRS_COLORWRITEENABLE, s_dwColorWrite);

		// not needed anymore since rest of ops concentrate on image processing
		//SETRS(D3DRS_ALPHAFUNC, g_dwAlphaCmp[curtest.atst]);
	}

	if( dwUsingSpecialTesting ) {
		// render the real alpha
		SETRS(D3DRS_ALPHATESTENABLE, FALSE);
		SETRS(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_ALPHA);

		if( s_bWriteDepth ) {
			if( bIndepWriteMasks ) SETRS(D3DRS_COLORWRITEENABLE1, 0);
			else pd3dDevice->SetRenderTarget(1,NULL);
		}

		SETRS(D3DRS_ZWRITEENABLE, FALSE);

		SETRS(D3DRS_STENCILMASK, STENCIL_SPECIAL|STENCIL_PIXELWRITE);
		SETRS(D3DRS_STENCILPASS, D3DSTENCILOP_KEEP);
		SETRS(D3DRS_STENCILFUNC, D3DCMP_EQUAL);
		SETRS(D3DRS_STENCILREF, STENCIL_SPECIAL|STENCIL_PIXELWRITE);

		DXVEC4 v = DXVEC4(0,exactcolor ? 510.0f : 2.0f,0,0);
		SETCONSTF(GPU_ONECOLOR, v);
		DRAW();

		// don't need to restore
	}

	if( s_bDestAlphaTest ) {
		if( (s_dwColorWrite&D3DCOLORWRITEENABLE_ALPHA) ) {
			if( curvb.fba.fba )
				ProcessFBA(curvb);
			else if( bCanRenderStencil )
				// finally make sure all entries are 1 when the dest alpha >= 0x80 (if fba is 1, this is already the case)
				ProcessStencil(curvb);
		}
	}
	else if( (s_dwColorWrite&D3DCOLORWRITEENABLE_ALPHA) && curvb.fba.fba )
		ProcessFBA(curvb);

	if( bDestAlphaColor == 1 ) {
		// need to reset the dest colors to their original counter parts
		//WARN_LOG("Need to reset dest alpha color\n");
	}

#ifdef _DEBUG
	if( g_bSaveFlushedFrame & 0xf ) {
		char str[255];
		sprintf(str, "frames\\frame%.4d.jpg", g_SaveFrameNum++);
		if( (g_bSaveFlushedFrame & 2)  )
			D3DXSaveSurfaceToFile(str, D3DXIFF_JPG, curvb.prndr->psurf, NULL, NULL);
	}
#endif

	// clamp the final colors, when enabled ffx2 credits mess up
	if( curvb.curprim.abe && bAlphaClamping && g_RenderFormat != D3DFMT_A8R8G8B8 && !(g_GameSettings&GAME_NOCOLORCLAMP) ) { // if !colclamp, skip

		ResetAlphaVariables();

		// if processing the clamping case, make sure can write to the front buffer
		SETRS(D3DRS_STENCILENABLE, 0);
		SETRS(D3DRS_ALPHABLENDENABLE, TRUE);
		SETRS(D3DRS_ALPHATESTENABLE, FALSE);
		SETRS(D3DRS_ZENABLE, FALSE);
		SETRS(D3DRS_ZWRITEENABLE, FALSE);
		SETRS(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_RED|D3DCOLORWRITEENABLE_BLUE|D3DCOLORWRITEENABLE_GREEN);
		
		if( s_bWriteDepth ) {
			if( bIndepWriteMasks ) SETRS(D3DRS_COLORWRITEENABLE1, 0);
			else pd3dDevice->SetRenderTarget(1,NULL);
		}

		pd3dDevice->SetPixelShader(ppsOne);

		// (dest&0x7f)+0x80, blend factor for alpha is always 0x80
		SETRS(D3DRS_DESTBLEND, D3DBLEND_ONE);
		SETRS(D3DRS_SRCBLEND, D3DBLEND_ONE);

		float f;

		if( bAlphaClamping & 1 ) { // min
			f = 0;
			SETCONSTF(GPU_ONECOLOR, &f);
			SETRS(D3DRS_BLENDOP, D3DBLENDOP_MAX);
			DRAW();
		}
		
		// bios shows white screen
		if( bAlphaClamping & 2 ) { // max
			f = 1;
			SETCONSTF(GPU_ONECOLOR, &f);
			SETRS(D3DRS_BLENDOP, D3DBLENDOP_MIN);
			DRAW();
		}

		if( !curvb.zbuf.zmsk ) {
			SETRS(D3DRS_ZWRITEENABLE, TRUE);

			if( s_bWriteDepth ) {
				assert( curvb.pdepth != NULL );
				if( bIndepWriteMasks ) SETRS(D3DRS_COLORWRITEENABLE1, 0xf);
				else pd3dDevice->SetRenderTarget(1,curvb.pdepth->psurf);
			}
		}

		if( curvb.test.ate && USEALPHATESTING )
			SETRS(D3DRS_ALPHATESTENABLE, TRUE);

		SETRS(D3DRS_ZENABLE, curtest.zte);
	}

	if( dwFilterOpts ) {
		// undo filter changes
		if( dwFilterOpts & 1 ) pd3dDevice->SetSamplerState(SAMP_MEMORY0+context, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
		if( dwFilterOpts & 2 ) pd3dDevice->SetSamplerState(SAMP_MEMORY0+context, D3DSAMP_MINFILTER, D3DTEXF_POINT);
	}

	// reset used textures
	if( shadertype > 2 ) {
		pd3dDevice->SetSamplerState(SAMP_BILINEARBLOCKS, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP);
		pd3dDevice->SetSamplerState(SAMP_BILINEARBLOCKS, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP);
		pd3dDevice->SetTexture(SAMP_BILINEARBLOCKS, ptexBilinearBlocks);
	}

	SETRS(D3DRS_CLIPPLANEENABLE, 0);
//#ifndef RELEASE_TO_PUBLIC
	ppf += curvb.dwCount+0x100000;
//#endif
	curvb.dwCurOff += POINT_BUFFERFLUSH;
	SAFE_RELEASE(ptexRenderTargetCached);
	
	g_MaxRenderedHeight = 0;
	curvb.dwCount = 0;
	//curvb.Lock();
	curvb.curprim.abe = oldabe;
	//if( oldabe ) SETRS(D3DRS_ALPHABLENDENABLE, USEALPHABLENDING);

	if( conf.options & GSOPTION_WIREFRAME ) {
		// always render first few geometry as solid
		if( s_nWireframeCount > 0 ) {
			SETRS(D3DRS_FILLMODE, D3DFILL_WIREFRAME);
			--s_nWireframeCount;
		}
	}
}

void ZeroGS::ProcessMessages()
{
	if( listMsgs.size() > 0 ) {
		pSprite->Begin(D3DXSPRITE_ALPHABLEND|D3DXSPRITE_SORT_TEXTURE);

		RECT rctext;
		rctext.left = 25; rctext.top = 15;
		list<MESSAGE>::iterator it = listMsgs.begin();
		
		while( it != listMsgs.end() ) {
			rctext.left += 1;
			rctext.top += 1;
			pFont->DrawText(pSprite, it->str, -1, &rctext, DT_LEFT|DT_NOCLIP, 0xff000000);
			rctext.left -= 1;
			rctext.top -= 1;
			pFont->DrawText(pSprite, it->str, -1, &rctext, DT_LEFT|DT_NOCLIP, 0xffffff30);
			rctext.top += 15;

			if( (int)(it->dwTimeStamp - timeGetTime()) < 0 )
				it = listMsgs.erase(it);
			else ++it;
		}

		pSprite->End();
	}
}

void ZeroGS::RenderCustom(float fAlpha)
{
	if( !s_bBeginScene )
		pd3dDevice->BeginScene();

	pd3dDevice->SetDepthStencilSurface(psurfOrgDepth);

	pd3dDevice->SetRenderTarget(0, psurfOrgTarg);

	if( s_bWriteDepth )
		pd3dDevice->SetRenderTarget(1, NULL);

	SETRS(D3DRS_STENCILENABLE, 0);
	SETRS(D3DRS_ZENABLE, FALSE);
	SETRS(D3DRS_ZWRITEENABLE, FALSE);
	SETRS(D3DRS_COLORWRITEENABLE, 0xf);
	SETRS(D3DRS_ALPHABLENDENABLE, 0);
	SETRS(D3DRS_ALPHATESTENABLE, 0);
	SETRS(D3DRS_SCISSORTESTENABLE, 0);

	// play custom animation
	pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER|D3DCLEAR_STENCIL, 0, 1, 0);

	// tex coords
	DXVEC4 v = DXVEC4(1, 1, 0, 0);
	SETCONSTF(GPU_BITBLTTEX, v);
	SETCONSTF(GPU_BITBLTPOS, v);

	v.x = v.y = v.z = v.w = fAlpha;
	SETCONSTF(GPU_ONECOLOR, v);

	if( conf.options & GSOPTION_WIREFRAME ) SETRS(D3DRS_FILLMODE, D3DFILL_SOLID);

	pd3dDevice->SetVertexShader(pvsBitBlt);
	pd3dDevice->SetStreamSource(0, pvbRect, 0, sizeof(VertexGPU));
	pd3dDevice->SetPixelShader(ppsBaseTexture);

	// inside vb[0]'s target area, so render that region only
	pd3dDevice->SetTexture(SAMP_FINAL, ptexLogo);
	//pd3dDevice->SetSamplerState(SAMP_FINAL, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
	//pd3dDevice->SetSamplerState(SAMP_FINAL, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);

	pd3dDevice->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);

	// restore
	//pd3dDevice->SetSamplerState(SAMP_FINAL, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
	//pd3dDevice->SetSamplerState(SAMP_FINAL, D3DSAMP_MINFILTER, D3DTEXF_POINT);
	if( conf.options & GSOPTION_WIREFRAME ) SETRS(D3DRS_FILLMODE, D3DFILL_WIREFRAME);

	ProcessMessages();

	pd3dDevice->EndScene();
	s_bBeginScene = FALSE;

	pd3dDevice->Present(NULL, NULL, NULL, NULL);

	SETRS(D3DRS_SCISSORTESTENABLE, TRUE);
	SETRS(D3DRS_STENCILENABLE, 1);

	if( icurctx >= 0 ) vb[icurctx].bSyncVars = 0;
}

// adjusts trans to preserve aspect ratio
void ZeroGS::AdjustTransToAspect(DXVEC4& v, int dispwidth, int dispheight)
{
	float temp, f;
	if( dispwidth * height > dispheight * width ) {
		// limited by width
		
		// change in ratio
		f = ((float)width / (float)dispwidth) / ((float)height / (float)dispheight);
		v.y *= f;
		v.w *= f;

		// scanlines mess up when not aligned right
		v.y += (1-modf(v.y*height/2+0.05f, &temp))*2.0f/(float)height;
		v.w += (1-modf(v.w*height/2+0.05f, &temp))*2.0f/(float)height;
	}
	else {
		// limited by height
		f = ((float)height / (float)dispheight) / ((float)width / (float)dispwidth);
		f -= modf(f*width, &temp)/(float)width;
		v.x *= f;
		v.z *= f;
	}
}

void ZeroGS::Restore()
{
	if( !g_bIsLost )
		return;

	if( SUCCEEDED(pd3dDevice->Reset(&d3dpp)) ) {
		g_bIsLost = 0;
		// handle lost states
		ZeroGS::ChangeDeviceSize(width, height);
	}
}

void ZeroGS::RenderCRTC(int interlace)
{
	if( pd3dDevice == NULL ) {
		return;
	}

	if( g_bIsLost ) return;

#ifdef RELEASE_TO_PUBLIC
	if( g_nRealFrame < 80 ) {
		RenderCustom( min(1.0f, 2.0f - (float)g_nRealFrame / 40.0f) );

		if( g_nRealFrame == 79 )
			SAFE_RELEASE(ptexLogo);
		return;
	}
#endif

	Flush(0);
	Flush(1);

	// frame skipping
	if( g_nFrameRender > 0 ) {

		if( g_nFrameRender < 8 ) {
			g_nFrameRender++;
			if( g_nFrameRender <= 3 ) {
				g_nFramesSkipped++;
				return;
			}
		}
	}
	else {
		if( g_nFrameRender < -1 ) {
			g_nFramesSkipped++;
			return;
		}
		g_nFrameRender--;
	}

	if( g_bSaveFrame ) {
		if( vb[0].prndr != NULL ) D3DXSaveSurfaceToFile("frame1.tga", D3DXIFF_TGA, vb[0].prndr->psurf, NULL, NULL);
		
		if( vb[1].prndr != NULL && vb[0].prndr != vb[1].prndr ) D3DXSaveSurfaceToFile("frame2.tga", D3DXIFF_TGA, vb[1].prndr->psurf, NULL, NULL);
		else DeleteFile("frame2.tga");
	}

	if( s_RangeMngr.ranges.size() > 0 )
		FlushTransferRanges(NULL);

	if( icurctx >= 0 && vb[icurctx].bVarsSetTarg ) { // check if anything rendered
		pd3dDevice->SetRenderTarget(0, psurfOrgTarg);
		pd3dDevice->SetRenderTarget(1, NULL);

		pd3dDevice->SetDepthStencilSurface(psurfOrgDepth);
	}

	D3DVIEWPORT9 view;
	view.Width = width;
	view.Height = height;
	view.X = 0;
	view.Y = 0;
	view.MinZ = 0;
	view.MaxZ = 1.0f;
	pd3dDevice->SetViewport(&view);

	//g_GameSettings |= GAME_VSSHACK|GAME_FULL16BITRES|GAME_NODEPTHRESOLVE;
	//s_bWriteDepth = TRUE;
	g_SaveFrameNum = 0;
	g_bSaveFlushedFrame = 1;
//	static int counter = 0;
//	counter++;
	// reset fba after every frame
	//if( !(g_GameSettings&GAME_NOFBARESET) ) {
		vb[0].fba.fba = 0;
		vb[1].fba.fba = 0;
	//}
	u32 bInterlace = SMODE2->INT && SMODE2->FFMD && (conf.interlace<2);

	// if interlace, only clear every other vsync
	if(!bInterlace ) {
		u32 color = D3DCOLOR_ARGB(0, BGCOLOR->R, BGCOLOR->G, BGCOLOR->B);
		pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET|D3DCLEAR_STENCIL, color, 1, 0);
	}

	if( !s_bBeginScene ) {
		pd3dDevice->BeginScene();
		s_bBeginScene = TRUE;
	}

	pd3dDevice->SetVertexShader(pvsBitBlt);
	pd3dDevice->SetStreamSource(0, pvbRect, 0, sizeof(VertexGPU));

	if( conf.options & GSOPTION_WIREFRAME ) SETRS(D3DRS_FILLMODE, D3DFILL_SOLID);
	SETRS(D3DRS_ZENABLE, 0);
	SETRS(D3DRS_ZWRITEENABLE, 0);
	SETRS(D3DRS_COLORWRITEENABLE, 0xf);
	SETRS(D3DRS_ALPHABLENDENABLE, 0);
	SETRS(D3DRS_ALPHATESTENABLE, 0);
	SETRS(D3DRS_SCISSORTESTENABLE, 0);
	SETRS(D3DRS_STENCILENABLE, 0);

	BOOL bUsingStencil = 0;

	if( bInterlace ) g_PrevBitwiseTexX = -1; // reset since will be using

	tex0Info dispinfo[2];

	for(int i = 0; i < 2; ++i) {

		if( !(*(u32*)(PMODE) & (1<<i)) ) {
			dispinfo[i].tw = 0;
			dispinfo[i].th = 0;
			continue;
		}

		GSRegDISPFB* pfb = i ? DISPFB2 : DISPFB1;
		GSRegDISPLAY* pd = i ? DISPLAY2 : DISPLAY1;
		int magh = pd->MAGH+1;
		int magv = pd->MAGV+1;
		
		dispinfo[i].tbp0 =	pfb->FBP << 5;
		dispinfo[i].tbw = pfb->FBW << 6;
		dispinfo[i].tw = (pd->DW + 1) / magh;
		dispinfo[i].th = (pd->DH + 1) / magv;
		dispinfo[i].psm = pfb->PSM;

		// hack!!
		// 2 * dispinfo[i].tw / dispinfo[i].th <= 1, metal slug 4
		if( bInterlace && 2 * dispinfo[i].tw / dispinfo[i].th <= 1 && !(g_GameSettings&GAME_INTERLACE2X) ) {
			dispinfo[i].th >>= 1;
		}
	}

	//int dispwidth = max(dispinfo[0].tw, dispinfo[1].tw), dispheight = max(dispinfo[0].th, dispinfo[1].th);

	// hack!, CMOD != 3, gradius
//	if( SMODE2->INT && SMODE2->FFMD && SMODE1->CMOD == 3 && dispwidth <= 320)
//		dispwidth *= 2;

	// hack! makai
	//if( !bInterlace && dispheight * 2 < dispwidth ) dispheight *= 2;

	// start from the last circuit
	for(int i = !PMODE->SLBG; i >= 0; --i) {

		tex0Info& texframe = dispinfo[i];
		if( texframe.th <= 1 )
			continue;

		GSRegDISPFB* pfb = i ? DISPFB2 : DISPFB1;
		GSRegDISPLAY* pd = i ? DISPLAY2 : DISPLAY1;

		DXVEC4 v, valpha;

		if( bInterlace ) {

			texframe.th >>= 1;

			// interlace mode
			pd3dDevice->SetTexture(SAMP_INTERLACE, CreateInterlaceTex(2*texframe.th));
			
			if( interlace == (conf.interlace&1) ) {
				// pass if odd
				valpha.z = 1.0f;
				valpha.w = -0.4999f;
			}
			else {
				// pass if even
				valpha.z = -1.0f;
				valpha.w = 0.5001f;
			}
		}
		else {

			if( SMODE2->INT && SMODE2->FFMD ) {
				texframe.th >>= 1;
			}

			// always pass interlace test
			valpha.z = 0;
			valpha.w = 1;
		}

		int bpp = 4;
		if( texframe.psm == 0x12 ) bpp = 3;
		else if( texframe.psm & 2 ) bpp = 2;

		// get the start and end addresses of the buffer
		int start, end;
		GetRectMemAddress(start, end, texframe.psm, 0, 0, texframe.tw, texframe.th, texframe.tbp0, texframe.tbw);

		if( i == 0 ) {
			// setup right blending
			SETRS(D3DRS_ALPHABLENDENABLE, USEALPHABLENDING);
			SETRS(D3DRS_BLENDOP, D3DBLENDOP_ADD);
			SETRS(D3DRS_BLENDOPALPHA, D3DBLENDOP_ADD);

			if( PMODE->MMOD ) {
				SETRS(D3DRS_BLENDFACTOR, D3DCOLOR_ARGB(0x80, PMODE->ALP, PMODE->ALP, PMODE->ALP));
				SETRS(D3DRS_SRCBLEND, D3DBLEND_BLENDFACTOR);
				SETRS(D3DRS_DESTBLEND, D3DBLEND_INVBLENDFACTOR);
			}
			else {
				SETRS(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
				SETRS(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
			}

			SETRS(D3DRS_SRCBLENDALPHA, PMODE->AMOD ? D3DBLEND_ZERO : D3DBLEND_ONE);
			SETRS(D3DRS_DESTBLENDALPHA, PMODE->AMOD? D3DBLEND_ONE : D3DBLEND_ZERO);
		}

		if( bUsingStencil ) {
			SETRS(D3DRS_STENCILWRITEMASK, 1<<i);
			SETRS(D3DRS_STENCILMASK, 1<<i);
		}

		if( texframe.psm == 0x12 ) {
			WARN_LOG("CRTC24!!!\n");
			// assume that data is already in ptexMem (do Resolve?)
			pd3dDevice->SetPixelShader(ppsCRTC24[bInterlace]);
			valpha.x = 0;
			valpha.y = 1;
			SETCONSTF(GPU_ONECOLOR, valpha);
			pd3dDevice->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);
			continue;
		}
		
		// first render the current render targets, then from ptexMem
		if( texframe.psm == 1 ) {
			valpha.x = 0;
			valpha.y = 1;
		}
		else {
			valpha.x = 1;
			valpha.y = 0;
		}

		SETCONSTF(GPU_ONECOLOR, valpha);

		BOOL bSkip = 0;
		BOOL bResolveTargs = 1;

		//s_mapFrameHeights[s_nCurFrameMap][texframe.tbp0] = texframe.th;
		list<CRenderTarget*> listTargs;

		s_RTs.GetTargs(start, end, listTargs);

		for(list<CRenderTarget*>::iterator it = listTargs.begin(); it != listTargs.end(); ) {
	
			CRenderTarget* ptarg = *it;

			if( ptarg->fbw == texframe.tbw && !(ptarg->status&CRenderTarget::TS_NeedUpdate) && ((256/bpp)*(texframe.tbp0-ptarg->fbp))%texframe.tbw == 0 ) {

				if( ptarg->fbp != texframe.tbp0 ) {
					// look for a better target (metal slug 5)
					list<CRenderTarget*>::iterator itbetter;
					for(itbetter = listTargs.begin(); itbetter != listTargs.end(); ++itbetter ) {
						if( (*itbetter)->fbp == texframe.tbp0 )
							break;
					}

					if( itbetter != listTargs.end() ) {
						it = listTargs.erase(it);
						continue;
					}
				}
				
				static int sindex = 0;
				char strtemp[25];
				sprintf(strtemp, "frames/frame%d.jpg", sindex++);
//				D3DXSaveSurfaceToFile(strtemp, D3DXIFF_JPG, ptarg->psurf, NULL, NULL);
//				if( g_bSaveFinalFrame )
//					D3DXSaveSurfaceToFile("frame1.tga", D3DXIFF_TGA, ptarg->psurf, NULL, NULL);

				int dby = pfb->DBY;
				int movy = 0;

				// determine the rectangle to render
				if( ptarg->fbp < texframe.tbp0 ) {
					dby += (256/bpp)*(texframe.tbp0-ptarg->fbp)/texframe.tbw;
				}
				else if( ptarg->fbp > texframe.tbp0 ) {
					dby -= (256/bpp)*(ptarg->fbp-texframe.tbp0)/texframe.tbw;

					if( dby < 0 ) {
						movy = -dby;
						dby = 0;
					}
				}

				int dh = min(ptarg->fbh - dby, texframe.th-movy);

				if( dh >= 64 ) {

					if( ptarg->fbh - dby < texframe.th-movy && !bUsingStencil ) {

						if( !bUsingStencil ) {
							pd3dDevice->Clear(0, NULL, D3DCLEAR_STENCIL, 0, 1, 0);
						}
						bUsingStencil = 1;
						SETRS(D3DRS_STENCILENABLE, TRUE);
						SETRS(D3DRS_STENCILPASS, D3DSTENCILOP_REPLACE);
						SETRS(D3DRS_STENCILFUNC, D3DCMP_NOTEQUAL);
						SETRS(D3DRS_STENCILREF, 3);
						SETRS(D3DRS_STENCILWRITEMASK, 1<<i);
						SETRS(D3DRS_STENCILMASK, 1<<i);
					}

					float fiw = 1.0f / texframe.tbw;
					float fih = 1.0f / ptarg->fbh;

					// tex coords
					v = DXVEC4(fiw*(float)texframe.tw, fih*(float)(dh), fiw*(float)(pfb->DBX), fih*((float)dby-0.5f));
					SETCONSTF(GPU_BITBLTTEX, v);

					// dest rect
					v.x = 1;
					v.y = dh/(float)texframe.th;
					v.z = 0;
					v.w = 1-v.y;
					
					if( movy > 0 )
						v.w -= movy/(float)texframe.th;

					if (bInterlace && interlace == (conf.interlace&1) ) {
						// move down by 1 pixel
						v.w += 1.0f / (float)dh;
					}

					AdjustTransToAspect(v, (conf.options&GSOPTION_WIDESCREEN)?960:640, (conf.options&GSOPTION_WIDESCREEN)?540:480);
					SETCONSTF(GPU_BITBLTPOS, v);

					// use GPU_INVTEXDIMS to store inverse texture dims
					v.x = fiw;
					v.y = fih;
					v.z = 0;
					SETCONSTF(GPU_INVTEXDIMS, v);

					// inside vb[0]'s target area, so render that region only
					pd3dDevice->SetTexture(SAMP_FINAL, ptarg->ptex);
					pd3dDevice->SetPixelShader(ppsCRTCTarg[bInterlace]);

					pd3dDevice->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);

					if( abs(dh - (int)texframe.th) <= 1 ) {
						bSkip = 1;
						break;
					}
					if( abs(dh - (int)ptarg->fbh) <= 1 ) {
						it = listTargs.erase(it);
						continue;
					}
				}
			}
		
			++it;
		}

		if( !bSkip ) {
			for(list<CRenderTarget*>::iterator it = listTargs.begin(); it != listTargs.end(); ++it)
				(*it)->Resolve();

			// context has to be 0
			SetTexVariablesInt(0, 2, texframe, g_MemTargs.GetMemoryTarget(texframe, 1), 1);

			if( g_bSaveFinalFrame )
				SaveTex(&texframe, g_bSaveFinalFrame-1>0);

			// finally render from the memory (note that the stencil buffer will keep previous regions)
			v = DXVEC4(1,1,0,0);

			if (bInterlace && interlace == (conf.interlace)) {
				// move down by 1 pixel
				v.w += 1.0f / (float)texframe.th;
			}

			AdjustTransToAspect(v, (conf.options&GSOPTION_WIDESCREEN)?960:640, (conf.options&GSOPTION_WIDESCREEN)?540:480);
			SETCONSTF(GPU_BITBLTPOS, v);

			v = DXVEC4(texframe.tw,texframe.th,-0.5f,-0.5f);
			SETCONSTF(GPU_BITBLTTEX, v);

			// use GPU_INVTEXDIMS to store inverse texture dims
			v.x = 1.0f / (float)texframe.tw;
			v.y = 1.0f / (float)texframe.th;
			v.z = 0;//-0.5f * v.x;
			v.w = -0.5f * v.y;
			SETCONSTF(GPU_INVTEXDIMS, v);
			
			pd3dDevice->SetPixelShader(ppsCRTC[bInterlace]);
			pd3dDevice->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);
		}
	}

	if(1) {// || !bInterlace) {
		s_bBeginScene = FALSE;

		ProcessMessages();

		if( g_bMakeSnapshot ) {
			RECT rctext;
			char str[64];
			rctext.left = 200; rctext.top = 15;
			sprintf(str, "ZeroGS %d.%d.%d - %.1f fps %s", revision, build, minor, fFPS, s_frameskipping?" - frameskipping":"");

			pSprite->Begin(D3DXSPRITE_ALPHABLEND|D3DXSPRITE_SORT_TEXTURE);
			rctext.left += 1;
			rctext.top += 1;
			pFont->DrawText(pSprite, str, -1, &rctext, DT_LEFT|DT_NOCLIP, 0xff000000);
			rctext.left -= 1;
			rctext.top -= 1;
			pFont->DrawText(pSprite, str, -1, &rctext, DT_LEFT|DT_NOCLIP, 0xffc0ffff);
			pSprite->End();
		}

		if( g_bDisplayFPS ) {
			RECT rctext;
			char str[64];
			rctext.left = 10; rctext.top = 10;
			sprintf(str, "%.1f fps", fFPS);

			pSprite->Begin(D3DXSPRITE_ALPHABLEND|D3DXSPRITE_SORT_TEXTURE);
			rctext.left += 1;
			rctext.top += 1;
			pFont->DrawText(pSprite, str, -1, &rctext, DT_LEFT|DT_NOCLIP, 0xff000000);
			rctext.left -= 1;
			rctext.top -= 1;
			pFont->DrawText(pSprite, str, -1, &rctext, DT_LEFT|DT_NOCLIP, 0xffc0ffff);
			pSprite->End();
		}

		pd3dDevice->EndScene();
		if( pd3dDevice->Present(NULL, NULL, NULL, NULL) == D3DERR_DEVICELOST ) {
			// device is lost, need to recreate
			DEBUG_LOG("device lost\n");
			g_bIsLost = TRUE;
			Reset();
			return;
		}

		if( conf.options & GSOPTION_WIREFRAME ) {
			// clear all targets
			s_nWireframeCount = 1;
		}

		if( g_bMakeSnapshot ) {
			
			if( SUCCEEDED(D3DXSaveSurfaceToFile(strSnapshot != ""?strSnapshot.c_str():"temp.jpg", (conf.options&GSOPTION_BMPSNAP)?D3DXIFF_BMP:D3DXIFF_JPG, psurfOrgTarg, NULL, NULL)) ) {
				char str[255];
				sprintf(str, "saved %s\n", strSnapshot.c_str());
				AddMessage(str, 500);
			}
			g_bMakeSnapshot = 0;
		}

		if( s_avicapturing ) {
			CaptureFrame();
		}

		if( s_nNewWidth >= 0 && s_nNewHeight >= 0 && !g_bIsLost ) {
			Reset();
			ChangeDeviceSize(s_nNewWidth, s_nNewHeight);
			s_nNewWidth = s_nNewHeight = -1;
		}

		// switch the fbp lists
//		s_nCurFBPSet ^= 1;
//		s_setFBP[s_nCurFBPSet].clear();
		//s_nCurFrameMap ^= 1;
		//s_mapFrameHeights[s_nCurFrameMap].clear();
	}

	pd3dDevice->SetTexture(SAMP_FINAL, NULL); // d3d debug complains if not
	g_MemTargs.DestroyCleared();

	for(list<LPD3DTEX>::iterator it = s_vecTempTextures.begin(); it != s_vecTempTextures.end(); ++it)
		(*it)->Release();
	s_vecTempTextures.clear();

	if( EXTWRITE->WRITE&1 ) {
		WARN_LOG("EXTWRITE\n");
		ExtWrite();
		EXTWRITE->WRITE = 0;
	}

	if( conf.options & GSOPTION_WIREFRAME ) SETRS(D3DRS_FILLMODE, D3DFILL_WIREFRAME);
	SETRS(D3DRS_SCISSORTESTENABLE, TRUE);

	if( icurctx >= 0 ) {
		vb[icurctx].bVarsSetTarg = FALSE;
		vb[icurctx].bVarsTexSync = FALSE;
		vb[0].bVarsTexSync = FALSE;
	}

	// statistics
	if( s_nWriteDepthCount > 0 ) {
		assert( conf.mrtdepth );
		if( --s_nWriteDepthCount <= 0 ) {
			s_bWriteDepth = FALSE;
		}
	}

	if( s_nWriteDestAlphaTest > 0 ) {
		if( --s_nWriteDestAlphaTest <= 0 ) {
			s_bDestAlphaTest = FALSE;
		}
	}

	if( g_GameSettings & GAME_AUTORESET ) {
		s_nResolveCounts[s_nCurResolveIndex] = s_nResolved;
		s_nCurResolveIndex = (s_nCurResolveIndex+1)%ArraySize(s_nResolveCounts);

		int total = 0;
		for(int i = 0; i < ArraySize(s_nResolveCounts); ++i) total += s_nResolveCounts[i];

		if( total / ArraySize(s_nResolveCounts) > 3 ) {
			if( s_nLastResolveReset > (int)(fFPS * 8) ) {
				// reset
				DEBUG_LOG("ZeroGS: video mem reset\n");
				s_nLastResolveReset = 0;
				memset(s_nResolveCounts, 0, sizeof(s_nResolveCounts));

				s_RTs.ResolveAll();
				s_RTs.Destroy();
				s_DepthRTs.ResolveAll();
				s_DepthRTs.Destroy();

				vb[0].prndr = NULL; vb[0].pdepth = NULL; vb[0].bNeedFrameCheck = 1; vb[0].bNeedZCheck = 1;
				vb[1].prndr = NULL; vb[1].pdepth = NULL; vb[1].bNeedFrameCheck = 1; vb[1].bNeedZCheck = 1;
			}
		}

		s_nLastResolveReset++;
	}

	if( s_nResolved > 8 ) s_nResolved = 2;
	else if( s_nResolved > 0 ) --s_nResolved;

	if( g_nDepthUsed > 0 ) --g_nDepthUsed;

	s_ClutResolve = 0;
	s_PSM8Resolve = 0;
	g_nDepthUpdateCount = 0;

	maxmin = 608;
}

//////////////////////////
// Internal Definitions //
//////////////////////////

__forceinline void MOVZ(VertexGPU *p, u32 gsz, const VB& curvb) 
{
	p->z = curvb.zprimmask==0xffff?min((u32)0xffff, gsz):gsz;
}

__forceinline void MOVFOG(VertexGPU *p, Vertex gsf)
{
	p->f = ((s16)(gsf).f<<7)|0x7f;
}

__forceinline void SET_VERTEX(VertexGPU *p, int Index, const VB& curvb) 
{ 
	int index = Index; 
	
	p->x = (((int)gs.gsvertex[index].x - curvb.offset.x)>>1)&0xffff;
	p->y = (((int)gs.gsvertex[index].y - curvb.offset.y)>>1)&0xffff;
	
	/*x = ((int)gs.gsvertex[index].x - curvb.offset.x);
	y = ((int)gs.gsvertex[index].y - curvb.offset.y);
	p.x = (x&0x7fff) | (x < 0 ? 0x8000 : 0);
	p.y = (y&0x7fff) | (y < 0 ? 0x8000 : 0);*/
	
	p->f = ((s16)gs.gsvertex[index].f<<7)|0x7f;
	MOVZ(p, gs.gsvertex[index].z, curvb);
	p->rgba = prim->iip ? gs.gsvertex[index].rgba : gs.rgba;
	
	if ((g_GameSettings & GAME_TEXAHACK) && !(p->rgba&0xffffff))
		p->rgba = 0;
	if (prim->tme ) 
	{
		if( prim->fst ) 
		{
			p->s = (float)gs.gsvertex[index].u * fiTexWidth[prim->ctxt];
			p->t = (float)gs.gsvertex[index].v * fiTexHeight[prim->ctxt];
			p->q = 1;
		}
		else 
		{
			p->s = gs.gsvertex[index].s;
			p->t = gs.gsvertex[index].t;
			p->q = gs.gsvertex[index].q;
		}
	}
}

#define OUTPUT_VERT(fn, vert, id) { \
	fn("%c%d(%d): xyzf=(%4d,%4d,0x%x,%3d), rgba=0x%8.8x, stq = (%2.5f,%2.5f,%2.5f)\n", id==0?'*':' ', id, prim->prim, vert.x/8, vert.y/8, vert.z, vert.f/128, \
		vert.rgba, Clamp(vert.s, -10, 10), Clamp(vert.t, -10, 10), Clamp(vert.q, -10, 10)); \
} \


void ZeroGS::KickPoint()
{
	assert( gs.primC >= 1 );

	VB& curvb = vb[prim->ctxt];
	if (curvb.bNeedTexCheck) curvb.FlushTexData();

	if( !(g_GameSettings&GAME_DOPARALLELCTX) && vb[!prim->ctxt].dwCount > 0 && vb[prim->ctxt].gsfb.fbp == vb[!prim->ctxt].gsfb.fbp )
	{
		assert( vb[prim->ctxt].dwCount == 0 );
		Flush(!prim->ctxt);
	}

	if( curvb.dwCount >= POINT_BUFFERFLUSH)
		Flush(prim->ctxt);

	curvb.Lock();
	int last = (gs.primIndex+2)%ArraySize(gs.gsvertex);

	VertexGPU* p = curvb.pbuf+curvb.dwCount;
	SET_VERTEX(&p[0], last, curvb);
	curvb.dwCount++;

#ifdef PRIM_LOG
	OUTPUT_VERT(PRIM_LOG, p[0], 0);
#endif
}

void ZeroGS::KickLine()
{
	assert( gs.primC >= 2 );
	VB& curvb = vb[prim->ctxt];
	if( curvb.bNeedTexCheck )
		curvb.FlushTexData();

	if( !(g_GameSettings&GAME_DOPARALLELCTX) && vb[!prim->ctxt].dwCount > 0 && vb[prim->ctxt].gsfb.fbp == vb[!prim->ctxt].gsfb.fbp )
	{
		assert( vb[prim->ctxt].dwCount == 0 );
		Flush(!prim->ctxt);
	}

	if( curvb.dwCount >= POINT_BUFFERFLUSH/2 )
		Flush(prim->ctxt);

	curvb.Lock();
	int next = (gs.primIndex+1)%ArraySize(gs.gsvertex);
	int last = (gs.primIndex+2)%ArraySize(gs.gsvertex);

	VertexGPU* p = curvb.pbuf+curvb.dwCount*2;
	SET_VERTEX(&p[0], next, curvb);
	SET_VERTEX(&p[1], last, curvb);

	curvb.dwCount++;

#ifdef PRIM_LOG
	OUTPUT_VERT(PRIM_LOG, p[0], 0);
	OUTPUT_VERT(PRIM_LOG, p[1], 1);
#endif
}

void ZeroGS::KickTriangle()
{
	assert( gs.primC >= 3 );
	VB& curvb = vb[prim->ctxt];
	if (curvb.bNeedTexCheck) curvb.FlushTexData();

	if( !(g_GameSettings&GAME_DOPARALLELCTX) && vb[!prim->ctxt].dwCount > 0 && vb[prim->ctxt].gsfb.fbp == vb[!prim->ctxt].gsfb.fbp )
	{
		assert( vb[prim->ctxt].dwCount == 0 );
		Flush(!prim->ctxt);
	}

	if( curvb.dwCount >= POINT_BUFFERFLUSH/3 )
		Flush(prim->ctxt);

	curvb.Lock();
	VertexGPU* p = curvb.pbuf+curvb.dwCount*3;
	SET_VERTEX(&p[0], 0, curvb);
	SET_VERTEX(&p[1], 1, curvb);
	SET_VERTEX(&p[2], 2, curvb);

	curvb.dwCount++;

#ifdef PRIM_LOG
	OUTPUT_VERT(PRIM_LOG, p[0], 0);
	OUTPUT_VERT(PRIM_LOG, p[1], 1);
	OUTPUT_VERT(PRIM_LOG, p[2], 2);
#endif
}

void ZeroGS::KickTriangleFan()
{
	assert( gs.primC >= 3 );
	VB& curvb = vb[prim->ctxt];
	if (curvb.bNeedTexCheck) curvb.FlushTexData();

	if( !(g_GameSettings&GAME_DOPARALLELCTX) && vb[!prim->ctxt].dwCount > 0 && vb[prim->ctxt].gsfb.fbp == vb[!prim->ctxt].gsfb.fbp )
	{
		assert( vb[prim->ctxt].dwCount == 0 );
		Flush(!prim->ctxt);
	}

	if( curvb.dwCount >= POINT_BUFFERFLUSH/3 )
		Flush(prim->ctxt);

	curvb.Lock();
	VertexGPU* p = curvb.pbuf+curvb.dwCount*3;
	SET_VERTEX(&p[0], 0, curvb);
	SET_VERTEX(&p[1], 1, curvb);
	SET_VERTEX(&p[2], 2, curvb);

	curvb.dwCount++;

	// add 1 to skip the first vertex
	if( gs.primIndex == gs.nTriFanVert )
		gs.primIndex = (gs.primIndex+1)%ArraySize(gs.gsvertex);

#ifdef PRIM_LOG
	OUTPUT_VERT(PRIM_LOG, p[0], 0);
	OUTPUT_VERT(PRIM_LOG, p[1], 1);
	OUTPUT_VERT(PRIM_LOG, p[2], 2);
#endif
}

__forceinline void SetKickVertex(VertexGPU *p, Vertex v, int next, const VB& curvb) 
{
	SET_VERTEX(p, next, curvb); 
	MOVZ(p, v.z, curvb);	
	MOVFOG(p, v);
}

void ZeroGS::KickSprite()
{
	assert( gs.primC >= 2 );
	VB& curvb = vb[prim->ctxt];
	if( curvb.bNeedTexCheck )
		curvb.FlushTexData();

	if( !(g_GameSettings&GAME_DOPARALLELCTX) && vb[!prim->ctxt].dwCount > 0 && vb[prim->ctxt].gsfb.fbp == vb[!prim->ctxt].gsfb.fbp )
	{
		assert( vb[prim->ctxt].dwCount == 0 );
		Flush(!prim->ctxt);
	}

	if (curvb.dwCount >= POINT_BUFFERFLUSH/3) Flush(prim->ctxt);

	curvb.Lock();
	int next = (gs.primIndex+1)%ArraySize(gs.gsvertex);
	int last = (gs.primIndex+2)%ArraySize(gs.gsvertex);

	// sprite is too small and AA shows lines (tek4)
	if( s_AAx ) 
	{
		gs.gsvertex[last].x += 4;
		if (s_AAy) gs.gsvertex[last].y += 4;
	}

	// might be bad sprite (KH dialog text)
	//if( gs.gsvertex[next].x == gs.gsvertex[last].x || gs.gsvertex[next].y == gs.gsvertex[last].y )
	//	return;

	VertexGPU* p = curvb.pbuf+curvb.dwCount*3;
	
	SetKickVertex(&p[0], gs.gsvertex[last], next, curvb);
	SetKickVertex(&p[3], gs.gsvertex[last], next, curvb);
	SetKickVertex(&p[1], gs.gsvertex[last], last, curvb);
	SetKickVertex(&p[4], gs.gsvertex[last], last, curvb);

	if (g_MaxRenderedHeight < p[0].y) g_MaxRenderedHeight = p[0].y;
	if (g_MaxRenderedHeight < p[1].y) g_MaxRenderedHeight = p[1].y;

	SetKickVertex(&p[2], gs.gsvertex[last], next, curvb);
	p[2].s = p[1].s;
	p[2].x = p[1].x;

	SetKickVertex(&p[5], gs.gsvertex[last], last, curvb);
	p[5].s = p[0].s;
	p[5].x = p[0].x;	

	curvb.dwCount += 2;

#ifdef PRIM_LOG
	OUTPUT_VERT(PRIM_LOG, p[0], 0);
	OUTPUT_VERT(PRIM_LOG, p[1], 1);
#endif
}

void ZeroGS::KickDummy()
{
	//GREG_LOG("Kicking bad primitive: %.8x\n", *(u32*)prim);
}

__forceinline void ZeroGS::RenderFBA(const VB& curvb)
{
	// add fba to all pixels
	SETRS(D3DRS_STENCILWRITEMASK, STENCIL_CLEAR);
	SETRS(D3DRS_STENCILPASS, D3DSTENCILOP_REPLACE);
	SETRS(D3DRS_STENCILFAIL, D3DSTENCILOP_ZERO);
	SETRS(D3DRS_STENCILFUNC, D3DCMP_ALWAYS);
	SETRS(D3DRS_STENCILREF, STENCIL_FBA);

	SETRS(D3DRS_ZENABLE, FALSE);
	SETRS(D3DRS_ZWRITEENABLE, FALSE);
	SETRS(D3DRS_COLORWRITEENABLE, 0);

	if( s_bWriteDepth ) {
		if( bIndepWriteMasks ) SETRS(D3DRS_COLORWRITEENABLE1, 0);
		else pd3dDevice->SetRenderTarget(1, NULL);
	}

	SETRS(D3DRS_STENCILPASS, D3DSTENCILOP_REPLACE);
	SETRS(D3DRS_ALPHATESTENABLE, TRUE);
	SETRS(D3DRS_ALPHAFUNC, D3DCMP_GREATEREQUAL);
	SETRS(D3DRS_ALPHAREF, 0xff);

	DXVEC4 v;
	v.x = 1; v.y = 2; v.z = 0; v.w = 0;
	SETCONSTF(GPU_ONECOLOR, v);

	DRAW();

	if( !curvb.test.ate )
		SETRS(D3DRS_ALPHATESTENABLE, FALSE);
	else {
		SETRS(D3DRS_ALPHAFUNC, g_dwAlphaCmp[curvb.test.atst]);
		SETRS(D3DRS_ALPHAREF, b2XAlphaTest ? min(255,2 * curvb.test.aref) : curvb.test.aref);
	}

	// reset (not necessary)
	SETRS(D3DRS_COLORWRITEENABLE, s_dwColorWrite);
	SETRS(D3DRS_STENCILFAIL, D3DSTENCILOP_KEEP);

	if( !curvb.zbuf.zmsk ) {
		SETRS(D3DRS_ZWRITEENABLE, TRUE);
		
		assert( curvb.pdepth != NULL );
		if( s_bWriteDepth ) {
			if( bIndepWriteMasks ) SETRS(D3DRS_COLORWRITEENABLE1, 0xf);
			else pd3dDevice->SetRenderTarget(1, curvb.pdepth->psurf);
		}
	}
	SETRS(D3DRS_ZENABLE, curvb.test.zte);
}

__forceinline void ZeroGS::RenderAlphaTest(const VB& curvb)
{
	if( !g_bUpdateStencil ) return;

	if( curvb.test.ate ) {
		if( curvb.test.afail == 1 ) SETRS(D3DRS_ALPHATESTENABLE, FALSE);
	}

	SETRS(D3DRS_ZWRITEENABLE, FALSE);
	SETRS(D3DRS_COLORWRITEENABLE, 0);

	DXVEC4 v;
	v.x = 1; v.y = 2; v.z = 0; v.w = 0;
	SETCONSTF(GPU_ONECOLOR, v);

	if( s_bWriteDepth ) {
		if( bIndepWriteMasks ) SETRS(D3DRS_COLORWRITEENABLE1, 0);
		else pd3dDevice->SetRenderTarget(1,NULL);
	}

	// or a 1 to the stencil buffer wherever alpha passes
	SETRS(D3DRS_STENCILPASS, D3DSTENCILOP_REPLACE);
	SETRS(D3DRS_STENCILFUNC, D3DCMP_ALWAYS);

	SETRS(D3DRS_STENCILENABLE, TRUE);
	
	if( !s_bDestAlphaTest ) {
		// clear everything
		SETRS(D3DRS_STENCILREF, 0);
		SETRS(D3DRS_STENCILWRITEMASK, STENCIL_CLEAR);
		SETRS(D3DRS_ALPHATESTENABLE, FALSE);
		DRAW();

		if( curvb.test.ate && curvb.test.afail != 1 && USEALPHATESTING)
			SETRS(D3DRS_ALPHATESTENABLE, TRUE);
	}

	if( curvb.test.ate && curvb.test.atst>1 && curvb.test.aref > 0x80) {
		v.x = 1; v.y = 1; v.z = 0; v.w = 0;
		SETCONSTF(GPU_ONECOLOR, v);
		SETRS(D3DRS_ALPHAREF, curvb.test.aref);
	}

	SETRS(D3DRS_STENCILREF, STENCIL_SPECIAL);
	SETRS(D3DRS_STENCILWRITEMASK, STENCIL_SPECIAL);
	SETRS(D3DRS_ZENABLE, FALSE);
	
	DRAW();

	if( curvb.test.zte ) SETRS(D3DRS_ZENABLE, TRUE);
	SETRS(D3DRS_ALPHATESTENABLE, 0);
	SETRS(D3DRS_COLORWRITEENABLE, s_dwColorWrite);

	if( !curvb.zbuf.zmsk ) {
		SETRS(D3DRS_ZWRITEENABLE, TRUE);

		// set rt next level
		if( s_bWriteDepth ) {
			if( bIndepWriteMasks ) SETRS(D3DRS_COLORWRITEENABLE1, 0xf);
			else pd3dDevice->SetRenderTarget(1, curvb.pdepth->psurf);
		}
	}
}

__forceinline void ZeroGS::RenderStencil(const VB& curvb, DWORD dwUsingSpecialTesting)
{
	//NOTE: This stencil hack for dest alpha testing ONLY works when 
	// the geometry in one DrawPrimitive call does not overlap

	// mark the stencil buffer for the new data's bits (mark 4 if alpha is >= 0xff)
	// mark 4 if a pixel was written (so that the stencil buf can be changed with new values)
	SETRS(D3DRS_STENCILWRITEMASK, STENCIL_PIXELWRITE);

	dwStencilMask = (curvb.test.date?STENCIL_ALPHABIT:0)|(dwUsingSpecialTesting?STENCIL_SPECIAL:0);
	SETRS(D3DRS_STENCILMASK, dwStencilMask);
	SETRS(D3DRS_STENCILPASS, D3DSTENCILOP_REPLACE);
	SETRS(D3DRS_STENCILFUNC, dwStencilMask ? D3DCMP_EQUAL : D3DCMP_ALWAYS);

	dwStencilRef = curvb.test.date*curvb.test.datm|STENCIL_PIXELWRITE|(dwUsingSpecialTesting?STENCIL_SPECIAL:0);
	SETRS(D3DRS_STENCILREF, dwStencilRef);
}

__forceinline void ZeroGS::ProcessStencil(const VB& curvb)
{
	assert( !curvb.fba.fba );

	// set new alpha bit
	SETRS(D3DRS_STENCILWRITEMASK, STENCIL_ALPHABIT);
	SETRS(D3DRS_STENCILMASK, STENCIL_PIXELWRITE|STENCIL_FBA);
	SETRS(D3DRS_STENCILPASS, D3DSTENCILOP_REPLACE);
	SETRS(D3DRS_STENCILFUNC, D3DCMP_EQUAL);
	SETRS(D3DRS_STENCILREF, STENCIL_PIXELWRITE);

	SETRS(D3DRS_ZENABLE, FALSE);
	SETRS(D3DRS_ZWRITEENABLE, FALSE);
	SETRS(D3DRS_COLORWRITEENABLE, 0);

	if( s_bWriteDepth ) {
		if( bIndepWriteMasks ) SETRS(D3DRS_COLORWRITEENABLE1, 0);
		else pd3dDevice->SetRenderTarget(1, NULL);
	}

	SETRS(D3DRS_ALPHATESTENABLE, 0);

	pd3dDevice->SetPixelShader(ppsOne);
	DRAW();

	// process when alpha >= 0xff
	SETRS(D3DRS_STENCILREF, STENCIL_PIXELWRITE|STENCIL_FBA|STENCIL_ALPHABIT);
	SETRS(D3DRS_STENCILFUNC, D3DCMP_EQUAL);
	DRAW();

	// clear STENCIL_PIXELWRITE bit
	SETRS(D3DRS_STENCILWRITEMASK, STENCIL_CLEAR);
	SETRS(D3DRS_STENCILPASS, D3DSTENCILOP_REPLACE);
	SETRS(D3DRS_STENCILFUNC, D3DCMP_ALWAYS);
	SETRS(D3DRS_STENCILREF, 0);

	DRAW();

	// restore state
	SETRS(D3DRS_COLORWRITEENABLE, s_dwColorWrite);

	if( curvb.test.ate && USEALPHATESTING)
		SETRS(D3DRS_ALPHATESTENABLE, TRUE);

	if( !curvb.zbuf.zmsk ) {
		SETRS(D3DRS_ZWRITEENABLE, TRUE);

		if( s_bWriteDepth ) {
			assert( curvb.pdepth != NULL );
			if( bIndepWriteMasks ) SETRS(D3DRS_COLORWRITEENABLE1, 0xf);
			else pd3dDevice->SetRenderTarget(1, curvb.pdepth->psurf);
		}
	}

	SETRS(D3DRS_ZENABLE, curvb.test.zte);

	SETRS(D3DRS_STENCILPASS, D3DSTENCILOP_KEEP);
}

__forceinline void ZeroGS::ProcessFBA(const VB& curvb)
{
	if( (curvb.frame.fbm&0x80000000) ) return;

	// add fba to all pixels that were written and alpha was less than 0xff
	SETRS(D3DRS_STENCILWRITEMASK, STENCIL_ALPHABIT);
	SETRS(D3DRS_STENCILMASK, STENCIL_PIXELWRITE|STENCIL_FBA);
	SETRS(D3DRS_STENCILPASS, D3DSTENCILOP_REPLACE);
	SETRS(D3DRS_STENCILFUNC, D3DCMP_EQUAL);
	SETRS(D3DRS_STENCILREF, STENCIL_FBA|STENCIL_PIXELWRITE|STENCIL_ALPHABIT);

	SETRS(D3DRS_ZENABLE, FALSE);
	SETRS(D3DRS_ZWRITEENABLE, FALSE);
	SETRS(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_ALPHA);
	
	if( s_bWriteDepth ) {
		if( bIndepWriteMasks ) SETRS(D3DRS_COLORWRITEENABLE1, 0);
		else pd3dDevice->SetRenderTarget(1, NULL);
	}

	// processes the pixels with ALPHA < 0x80*2
	SETRS(D3DRS_ALPHATESTENABLE, TRUE);
	SETRS(D3DRS_ALPHAFUNC, D3DCMP_LESSEQUAL);
	SETRS(D3DRS_ALPHAREF, 0xff);

	// add 1 to dest
	SETRS(D3DRS_SRCBLENDALPHA, D3DBLEND_ONE);
	SETRS(D3DRS_DESTBLENDALPHA, D3DBLEND_ONE);
	SETRS(D3DRS_BLENDOPALPHA, D3DBLENDOP_ADD);

	float f = 1;
	SETCONSTF(GPU_ONECOLOR, &f);
	pd3dDevice->SetPixelShader(ppsOne);

	DRAW();

	SETRS(D3DRS_ALPHATESTENABLE, FALSE);

	// reset bits
	SETRS(D3DRS_STENCILWRITEMASK, STENCIL_CLEAR);
	SETRS(D3DRS_STENCILMASK, STENCIL_PIXELWRITE|STENCIL_FBA);
	SETRS(D3DRS_STENCILPASS, D3DSTENCILOP_ZERO);
	SETRS(D3DRS_STENCILFAIL, D3DSTENCILOP_KEEP);
	SETRS(D3DRS_STENCILFUNC, D3DCMP_GREATER);
	SETRS(D3DRS_STENCILREF, 0);

	DRAW();

	if( curvb.test.atst && USEALPHATESTING) {
		SETRS(D3DRS_ALPHATESTENABLE, TRUE);
		SETRS(D3DRS_ALPHAFUNC, g_dwAlphaCmp[curvb.test.atst]);
		SETRS(D3DRS_ALPHAREF, b2XAlphaTest ? min(255,2 * curvb.test.aref) : curvb.test.aref);
	}

	// restore (SetAlphaVariables)
	if( !bNeedAlphaColor ) SETRS(D3DRS_SRCBLENDALPHA, D3DBLEND_ONE);//(bNeedBlendFactorInAlpha ? D3DBLEND_ZERO : D3DBLEND_ONE));
	SETRS(D3DRS_DESTBLENDALPHA, D3DBLEND_ZERO);//bNeedBlendFactorInAlpha ? D3DBLEND_ONE : D3DBLEND_ZERO);
	if(bNeedAlphaColor && vAlphaBlendColor.y<0) SETRS(D3DRS_BLENDOPALPHA, D3DBLENDOP_REVSUBTRACT);

	// reset (not necessary)
	SETRS(D3DRS_COLORWRITEENABLE, s_dwColorWrite);

	if( !curvb.zbuf.zmsk ) {
		SETRS(D3DRS_ZWRITEENABLE, TRUE);

		if( s_bWriteDepth ) {
			if( bIndepWriteMasks ) SETRS(D3DRS_COLORWRITEENABLE1, 0xf);
			else pd3dDevice->SetRenderTarget(1, curvb.pdepth->psurf);
		}
	}
	SETRS(D3DRS_ZENABLE, curvb.test.zte);
}

inline void ZeroGS::SetContextTarget(int context)
{
	VB& curvb = vb[context];

	if( curvb.prndr == NULL )
		curvb.prndr = s_RTs.GetTarg(curvb.frame, 0, GET_MAXHEIGHT(curvb.gsfb.fbp, curvb.gsfb.fbw, curvb.gsfb.psm));

	// make sure targets are valid
	if( curvb.pdepth == NULL ) {
		frameInfo f;
		f.fbp = curvb.zbuf.zbp;
		f.fbw = curvb.frame.fbw;
		f.fbh = curvb.prndr->fbh;
		f.psm = curvb.zbuf.psm;
		f.fbm = 0;
		curvb.pdepth = (CDepthTarget*)s_DepthRTs.GetTarg(f, CRenderTargetMngr::TO_DepthBuffer|CRenderTargetMngr::TO_StrictHeight|
			(curvb.zbuf.zmsk?CRenderTargetMngr::TO_Virtual:0), GET_MAXHEIGHT(curvb.zbuf.zbp, curvb.gsfb.fbw, 0));
	}
	
	assert( curvb.prndr != NULL && curvb.pdepth != NULL );
	assert( curvb.pdepth->fbh == curvb.prndr->targheight );
	
//	if( curvb.pdepth->fbh != curvb.prndr->fbh ) {
//
//		s_DepthRTs.DestroyTarg(curvb.pdepth);
//		ERROR_LOG("ZeroGS: render and depth heights different: %x %x\n", curvb.prndr->fbh, curvb.pdepth->fbh);
//		frameInfo f;
//		f.fbp = curvb.zbuf.zbp;
//		f.fbw = curvb.frame.fbw;
//		f.fbh = curvb.prndr->fbh;
//		f.psm = curvb.zbuf.psm;
//		f.fbm = 0;
//		curvb.pdepth = (CDepthTarget*)s_DepthRTs.GetTarg(f, CRenderTargetMngr::TO_DepthBuffer|CRenderTargetMngr::TO_StrictHeight|
//			(curvb.zbuf.zmsk?CRenderTargetMngr::TO_Virtual:0), GET_MAXHEIGHT(curvb.zbuf.zbp, curvb.gsfb.fbw, 0));		
//	}

	if( curvb.pdepth->status & CRenderTarget::TS_Virtual) {
		
		if( !curvb.zbuf.zmsk ) {
			CRenderTarget* ptemp = s_DepthRTs.Promote(curvb.pdepth->fbp|(curvb.pdepth->fbw<<16));
			assert( ptemp == curvb.pdepth );
		}
		else
			curvb.pdepth->status &= ~CRenderTarget::TS_NeedUpdate;
	}

	BOOL bSetTarg = 1;
	if( curvb.pdepth->status & CRenderTarget::TS_NeedUpdate ) {

		assert( !(curvb.pdepth->status & CRenderTarget::TS_Virtual) );

		// don't update if virtual
		curvb.pdepth->Update(context, curvb.prndr);
		bSetTarg = 0;
	}
	
	if( curvb.prndr->status & CRenderTarget::TS_NeedUpdate ) {

		if( s_bWriteDepth ) {
			if( bSetTarg ) {
				pd3dDevice->SetRenderTarget(1, curvb.pdepth->psurf);
				pd3dDevice->SetDepthStencilSurface(curvb.pdepth->pdepth);
			}
		}
		else if( bSetTarg )
			pd3dDevice->SetDepthStencilSurface(curvb.pdepth->pdepth);

		curvb.prndr->Update(context, curvb.pdepth);
		// note, targ already set
	}
	else { 

		//if( (vb[0].prndr != vb[1].prndr && vb[!context].bVarsSetTarg) || !vb[context].bVarsSetTarg )
			pd3dDevice->SetRenderTarget(0, curvb.prndr->psurf);
		//if( bSetTarg && ((vb[0].pdepth != vb[1].pdepth && vb[!context].bVarsSetTarg) || !vb[context].bVarsSetTarg) ) 
			curvb.pdepth->SetDepthTarget();

		if( s_ptexCurSet[0] == curvb.prndr->ptex ) {
			s_ptexCurSet[0] = NULL;
			pd3dDevice->SetTexture(SAMP_MEMORY0, NULL);
		}
		if( s_ptexCurSet[1] == curvb.prndr->ptex ) {
			s_ptexCurSet[1] = NULL;
			pd3dDevice->SetTexture(SAMP_MEMORY1, NULL);
		}

		curvb.prndr->SetViewport();
	}

	curvb.prndr->SetTarget(curvb.frame.fbp, curvb.scissor, context);

	if( (curvb.zbuf.zbp-curvb.pdepth->fbp) != (curvb.frame.fbp - curvb.prndr->fbp) && curvb.test.zte ) {
		WARN_LOG("frame and zbuf not aligned\n");
	}

	curvb.bVarsSetTarg = TRUE;
	if( vb[!context].prndr != curvb.prndr )
		vb[!context].bVarsSetTarg = FALSE;

	assert( !(curvb.prndr->status&CRenderTarget::TS_NeedUpdate) );
	assert( curvb.pdepth == NULL || !(curvb.pdepth->status&CRenderTarget::TS_NeedUpdate) );
}

void ZeroGS::SetTexVariables(int context)
{
	if( !vb[context].curprim.tme ) {
		return;
	}

	assert( !vb[context].bNeedTexCheck );

	DXVEC4 v, v2;
	tex0Info& tex0 = vb[context].tex0;
	
	float fw = (float)tex0.tw;
	float fh = (float)tex0.th;

	if( !vb[context].bTexConstsSync ) {
		// alpha and texture highlighting
		DXVEC4 valpha, valpha2;

		// if clut, use the frame format
		int psm = tex0.psm;
		if( PSMT_ISCLUT(tex0.psm) ) psm = tex0.cpsm;
		
		int nNeedAlpha = (psm == 1 || psm == 2 || psm == 10);

		DXVEC4 vblack;
		vblack.x = vblack.y = vblack.z = vblack.w = 10;

		switch(tex0.tfx) {
			case 0:
				valpha.z = 0; valpha.w = 0;
				valpha2.x = 0; valpha2.y = 0;
				valpha2.z = 2; valpha2.w = 1;
				
				break;
			case 1:
				valpha.z = 0; valpha.w = 1;
				valpha2.x = 1; valpha2.y = 0;
				valpha2.z = 0; valpha2.w = 0;
				
				break;
			case 2:
				valpha.z = 1; valpha.w = 1.0f;
				valpha2.x = 0; valpha2.y = tex0.tcc ? 1.0f : 0.0f;
				valpha2.z = 2; valpha2.w = 0;				
				
				break;

			case 3:
				valpha.z = 1; valpha.w = tex0.tcc ? 0.0f : 1.0f;
				valpha2.x = 0; valpha2.y = tex0.tcc ? 1.0f : 0.0f;
				valpha2.z = 2; valpha2.w = 0;
		
				break;
			default: __assume(0);
		}

		if( tex0.tcc ) {

			if( tex0.tfx == 1 ) {
				//mode.x = 10;
				valpha.z = 0; valpha.w = 0;
				valpha2.x = 1; valpha2.y = 1;
				valpha2.z = 0; valpha2.w = 0;
			}

			if( nNeedAlpha ) {
				
				if( tex0.tfx == 0 ) {
					// make sure alpha is mult by two when the output is Cv = Ct*Cf
					valpha.x = 2*gs.texa.fta[0];
					// if 24bit, always choose ta[0]
					valpha.y = 2*gs.texa.fta[psm != 1];
					valpha.y -= valpha.x;
				}
				else {
					valpha.x = gs.texa.fta[0];
					// if 24bit, always choose ta[0]
					valpha.y = gs.texa.fta[psm != 1];
					valpha.y -= valpha.x;
				}

				// need black detection
				if( gs.texa.aem && psm == PSMCT24 )
					vblack.w = 0;
			}
			else {
				if( tex0.tfx == 0 ) {
					valpha.x = 0;
					valpha.y = 2;
				}
				else {
					valpha.x = 0;
					valpha.y = 1;
				}
			}
		}
		else {

			// reset alpha to color
			valpha.x = valpha.y = 0;
			valpha.w = 1;
		}

		SETCONSTF(GPU_TEXALPHA0+context, &valpha);
		SETCONSTF(GPU_TEXALPHA20+context, &valpha2);
		if( tex0.tcc && gs.texa.aem && (psm == PSMCT24 || psm == PSMCT16 || psm == PSMCT16S) )
			SETCONSTF(GPU_TESTBLACK0+context, &vblack);

		// clamp relies on texture width
		{
			clampInfo* pclamp = &ZeroGS::vb[context].clamp;
			DXVEC4 v, v2;
			v.x = v.y = 0;
			LPD3DTEX* ptex = ZeroGS::vb[context].ptexClamp;
			ptex[0] = ptex[1] = NULL;

			float fw = ZeroGS::vb[context].tex0.tw;
			float fh = ZeroGS::vb[context].tex0.th;

			switch(pclamp->wms) {
				case 0:
					v2.x = -1e10;	v2.z = 1e10;
					break;
				case 1: // pclamp
					// suikoden5 movie text
					v2.x = 0; v2.z = 1-0.5f/fw;
					break;
				case 2: // reg pclamp
					v2.x = (pclamp->minu+0.5f)/fw;	v2.z = (pclamp->maxu-0.5f)/fw;
					break;

				case 3:	// region rep x
					v.x = 0.9999f;
					v.z = fw / (float)GPU_TEXMASKWIDTH;
					v2.x = (float)GPU_TEXMASKWIDTH / fw;
					v2.z = pclamp->maxu / fw;

					if( pclamp->minu != g_PrevBitwiseTexX ) {
						g_PrevBitwiseTexX = pclamp->minu;
						ptex[0] = ZeroGS::s_BitwiseTextures.GetTex(pclamp->minu, NULL);
					}
					break;

				default: __assume(0);
			}

			switch(pclamp->wmt) {
				case 0:
					v2.y = -1e10;	v2.w = 1e10;
					break;
				case 1: // pclamp
					// suikoden5 movie text
					v2.y = 0;	v2.w = 1-0.5f/fh;
					break;
				case 2: // reg pclamp
					v2.y = (pclamp->minv+0.5f)/fh; v2.w = (pclamp->maxv-0.5f)/fh;
					break;

				case 3:	// region rep y
					v.y = 0.9999f;
					v.w = fh / (float)GPU_TEXMASKWIDTH;
					v2.y = (float)GPU_TEXMASKWIDTH / fh;
					v2.w = pclamp->maxv / fh;

					if( pclamp->minv != g_PrevBitwiseTexY ) {
						g_PrevBitwiseTexY = pclamp->minv;
						ptex[1] = ZeroGS::s_BitwiseTextures.GetTex(pclamp->minv, ptex[0]);
					}
					break;

				default: __assume(0);
			}

			SETCONSTF(GPU_TEXWRAPMODE0+context, v);
			SETCONSTF(GPU_CLAMPEXTS0+context, v2);
		}

		vb[context].bTexConstsSync = TRUE;
	}

	if(s_bTexFlush ) {
		if( PSMT_ISCLUT(tex0.psm) )
			texClutWrite(context);
		else
			s_bTexFlush = FALSE;
	}
}

void ZeroGS::SetTexVariablesInt(int context, int bilinear, const tex0Info& tex0, CMemoryTarget* pmemtarg, int force)
{
	DXVEC4 v;
	assert( pmemtarg != NULL );

	float fw = (float)tex0.tw;
	float fh = (float)tex0.th;

	if( bilinear > 1 || (bilinear && conf.bilinear)) {
		v.x = (float)fw;
		v.y = (float)fh;
		v.z = 1.0f / (float)fw;
		v.w = 1.0f / (float)fh;
		SETCONSTF(GPU_REALTEXDIMS0+context, v);
	}

	if( m_Blocks[tex0.psm].bpp == 0 ) {
		ERROR_LOG("ZeroGS: Undefined tex psm 0x%x!\n", tex0.psm);
		return;
	}

	const BLOCK& b = m_Blocks[tex0.psm];

	float fbw = (float)tex0.tbw;

	v.x = b.vTexDims.x * fw;
	v.y = b.vTexDims.y * fh;
	v.z = (float)BLOCK_TEXWIDTH*(0.002f / 64.0f + 0.01f/128.0f);
	v.w = (float)BLOCK_TEXHEIGHT*0.2f/512.0f;

	if( bilinear > 1 || (conf.bilinear && bilinear) ) {
		v.x *= 1/128.0f;
		v.y *= 1/512.0f;
		v.z *= 1/128.0f;
		v.w *= 1/512.0f;
	}
	SETCONSTF(GPU_TEXDIMS0+context, v);

	float g_fitexwidth = g_fiGPU_TEXWIDTH/(float)pmemtarg->widthmult;
	float g_texwidth = GPU_TEXWIDTH*(float)pmemtarg->widthmult;

	SETCONSTF(GPU_TEXBLOCK0+context, &b.vTexBlock.x);

	float fpage = tex0.tbp0*(64.0f*g_fitexwidth) + 0.05f * g_fitexwidth;
	float fpageint = floorf(fpage);
	int starttbp = (int)fpage;
	
	// 2048 is number of words to span one page
	float fblockstride = (2048.0f /(float)(g_texwidth*BLOCK_TEXWIDTH)) * b.vTexDims.x * fbw;
	assert( fblockstride >= 1.0f );

	v.x = (float)(2048 * g_fitexwidth);
	v.y = fblockstride;
	v.z = g_fBlockMult/(float)pmemtarg->widthmult;
	v.w = fpage-fpageint;

	if( g_fBlockMult > 1 ) {
		// make sure to divide by mult (since the G16R16 texture loses info)
		v.z *= b.bpp * (1/32.0f);
	}

	SETCONSTF(GPU_TEXOFFSET0+context, v);

	v.y = (float)1.0f / (float)((pmemtarg->realheight+pmemtarg->widthmult-1)/pmemtarg->widthmult);
	v.x = (fpageint-(float)pmemtarg->realy/(float)pmemtarg->widthmult+0.5f)*v.y;

	SETCONSTF(GPU_PAGEOFFSET0+context, v);

	if( force ) {
		pd3dDevice->SetTexture(SAMP_MEMORY0+context, pmemtarg->ptex);
		s_ptexCurSet[context] = pmemtarg->ptex;
	}
	else s_ptexNextSet[context] = pmemtarg->ptex;
	vb[context].pmemtarg = pmemtarg;

	vb[context].bVarsTexSync = FALSE;
}

// assumes texture factor is unused
#define SET_ALPHA_COLOR_FACTOR(sign) { \
	switch(a.c) { \
		case 0: \
			bNeedAlphaColor = 1; \
			vAlphaBlendColor.y = (sign) ? 2.0f*255.0f/256.0f : -2.0f*255.0f/256.0f; \
			SETRS(D3DRS_SRCBLENDALPHA, D3DBLEND_ONE); \
			SETRS(D3DRS_BLENDOPALPHA, (sign) ? D3DBLENDOP_ADD : D3DBLENDOP_REVSUBTRACT); \
			break; \
		case 1: \
			/* if in 24 bit mode, dest alpha should be one */ \
			switch(vb[icurctx].prndr->psm&0xf) { \
				case 0: \
					bDestAlphaColor = (a.d!=2)&&((a.a==a.d)||(a.b==a.d)); \
					break; \
				case 1: \
					/* dest alpha should be one */ \
					bDestAlphaColor = 2; \
					break; \
				/* default: 16bit surface, so returned alpha is ok */ \
			} \
			break; \
			\
		case 2: \
			bNeedBlendFactorInAlpha = 1; /* should disable alpha channel writing */ \
			bNeedAlphaColor = 1; \
			vAlphaBlendColor.y = 0; \
			vAlphaBlendColor.w = (sign) ? (float)a.fix * (2.0f/255.0f) : (float)a.fix * (-2.0f/255.0f); \
			usec = 0; /* change so that alpha comes from source*/ \
			break; \
	} \
} \

//if( a.fix <= 0x80 ) { \
//				dwTemp = (a.fix*2)>255?255:(a.fix*2); \
//				dwTemp = dwTemp|(dwTemp<<8)|(dwTemp<<16)|0x80000000; \
//				DEBUG_LOG("bfactor: %8.8x\n", dwTemp); \
//				SETRS(D3DRS_BLENDFACTOR, dwTemp); \
//			} \
//			else { \

void ZeroGS::ResetAlphaVariables()
{
	s_bAlphaSet = FALSE;
}

void ZeroGS::SetAlphaVariables(const alphaInfo& a)
{
	SETRS(D3DRS_ALPHABLENDENABLE, USEALPHABLENDING); // always set

	if( s_bAlphaSet && a.abcd == s_alphaInfo.abcd && a.fix == s_alphaInfo.fix ) {
		return;
	}

	// TODO: negative color when not clamping turns to positive???
	g_vars._bAlphaState = 0; // set all to zero
	bNeedBlendFactorInAlpha = 0;
	b2XAlphaTest = 1;
	DWORD dwTemp = 0xffffffff;

	// default
	SETRS(D3DRS_SRCBLENDALPHA, D3DBLEND_ONE);
	SETRS(D3DRS_DESTBLENDALPHA, D3DBLEND_ZERO);
	SETRS(D3DRS_BLENDOPALPHA, D3DBLENDOP_ADD);

	s_alphaInfo = a;

	vAlphaBlendColor = DXVEC4(1,2*255.0f/256.0f,0,0);
	DWORD usec = a.c;

	if( a.a == a.b ) { // just d remains
		SETRS(D3DRS_ALPHABLENDENABLE, USEALPHABLENDING);

		if( a.d == 0 ) {
			SETRS(D3DRS_ALPHABLENDENABLE, 0);
		}
		else {
			SETRS(D3DRS_DESTBLEND, a.d == 1 ? D3DBLEND_ONE : D3DBLEND_ZERO);
			SETRS(D3DRS_SRCBLEND, D3DBLEND_ZERO);
			SETRS(D3DRS_BLENDOP, D3DBLENDOP_ADD);
		}

		goto EndSetAlpha;
	}
	else if( a.d == 2 ) { // zero

		if( a.a == 2 ) {
			// zero all color
			SETRS(D3DRS_SRCBLEND, D3DBLEND_ZERO);
			SETRS(D3DRS_DESTBLEND, D3DBLEND_ZERO);	
			goto EndSetAlpha;
		}
		else if( a.b == 2 ) {				
			//b2XAlphaTest = 1;
			
			SET_ALPHA_COLOR_FACTOR(1);

			if( bDestAlphaColor == 2 ) {
				SETRS(D3DRS_BLENDOP, D3DBLENDOP_ADD);
				SETRS(D3DRS_SRCBLEND, a.a == 0 ? D3DBLEND_ONE : D3DBLEND_ZERO);
				SETRS(D3DRS_DESTBLEND, a.a == 0 ? D3DBLEND_ZERO : D3DBLEND_ONE);
			}
			else {
				if( bNeedAlphaColor ) bAlphaClamping = 2;

				SETRS(D3DRS_BLENDOP, D3DBLENDOP_ADD);					
				SETRS(D3DRS_SRCBLEND, a.a == 0 ? blendalpha[usec] : D3DBLEND_ZERO);
				SETRS(D3DRS_DESTBLEND, a.a == 0 ? D3DBLEND_ZERO : blendalpha[usec]);
			}

			goto EndSetAlpha;
		}

		// nothing is zero, so must do some real blending
		//b2XAlphaTest = 1;
		bAlphaClamping = 3;

		SET_ALPHA_COLOR_FACTOR(1);

		SETRS(D3DRS_BLENDOP, a.a == 0 ? D3DBLENDOP_SUBTRACT : D3DBLENDOP_REVSUBTRACT);
		SETRS(D3DRS_SRCBLEND, bDestAlphaColor == 2 ? D3DBLEND_ONE : blendalpha[usec]);
		SETRS(D3DRS_DESTBLEND, bDestAlphaColor == 2 ? D3DBLEND_ONE : blendalpha[usec]);
	}
	else if( a.a == 2 ) {	// zero

		//b2XAlphaTest = 1;
		bAlphaClamping = 1; // min testing

		SET_ALPHA_COLOR_FACTOR(1);

		if( a.b == a.d ) {
			// can get away with 1-A
			SETRS(D3DRS_BLENDOP, D3DBLENDOP_ADD);
			SETRS(D3DRS_SRCBLEND, (a.b == 0 && bDestAlphaColor != 2) ? blendinvalpha[usec] : D3DBLEND_ZERO);
			SETRS(D3DRS_DESTBLEND, (a.b == 0 || bDestAlphaColor == 2) ? D3DBLEND_ZERO : blendinvalpha[usec]);
		}
		else {
			SETRS(D3DRS_BLENDOP, a.b==0 ? D3DBLENDOP_REVSUBTRACT : D3DBLENDOP_SUBTRACT);
			SETRS(D3DRS_SRCBLEND, (a.b == 0 && bDestAlphaColor != 2) ? blendalpha[usec] : D3DBLEND_ONE);
			SETRS(D3DRS_DESTBLEND, (a.b == 0 || bDestAlphaColor == 2 ) ? D3DBLEND_ONE : blendalpha[usec]);
		}
	}
	else if( a.b == 2 ) {
		bAlphaClamping = 2; // max testing

		SET_ALPHA_COLOR_FACTOR(a.a!=a.d);

		if( a.a == a.d ) {
			// can get away with 1+A, but need to set alpha to negative
			SETRS(D3DRS_BLENDOP, D3DBLENDOP_ADD);

			if( bDestAlphaColor == 2 ) {
				assert(usec==1);

				// all ones
				bNeedAlphaColor = 1;
				bNeedBlendFactorInAlpha = 1;
				vAlphaBlendColor.y = 0;
				vAlphaBlendColor.w = -1;
				SETRS(D3DRS_SRCBLEND, a.a == 0 ? D3DBLEND_INVSRCALPHA : D3DBLEND_ZERO);
				SETRS(D3DRS_DESTBLEND, a.a == 0 ? D3DBLEND_ZERO : D3DBLEND_INVSRCALPHA);
			}
			else {
				SETRS(D3DRS_SRCBLEND, a.a == 0 ? blendinvalpha[usec] : D3DBLEND_ZERO);
				SETRS(D3DRS_DESTBLEND, a.a == 0 ? D3DBLEND_ZERO : blendinvalpha[usec]);
			}
		}
		else {
			//b2XAlphaTest = 1;
			SETRS(D3DRS_BLENDOP, D3DBLENDOP_ADD);
			SETRS(D3DRS_SRCBLEND, (a.a == 0 && bDestAlphaColor != 2) ? blendalpha[usec] : D3DBLEND_ONE);
			SETRS(D3DRS_DESTBLEND, (a.a == 0 || bDestAlphaColor == 2) ? D3DBLEND_ONE : blendalpha[usec]);
		}
	}
	else {
		// all 3 components are valid!
		bAlphaClamping = 3; // all testing
		SET_ALPHA_COLOR_FACTOR(a.a!=a.d);

		if( a.a == a.d ) {
			// can get away with 1+A, but need to set alpha to negative
			SETRS(D3DRS_BLENDOP, D3DBLENDOP_ADD);
			
			if( bDestAlphaColor == 2 ) {
				assert(usec==1);

				// all ones
				bNeedAlphaColor = 1;
				bNeedBlendFactorInAlpha = 1;
				vAlphaBlendColor.y = 0;
				vAlphaBlendColor.w = -1;
				SETRS(D3DRS_SRCBLEND, a.a == 0 ? D3DBLEND_INVSRCALPHA : D3DBLEND_SRCALPHA);
				SETRS(D3DRS_DESTBLEND, a.a == 0 ? D3DBLEND_SRCALPHA : D3DBLEND_INVSRCALPHA);
			}
			else {
				SETRS(D3DRS_SRCBLEND, a.a == 0 ? blendinvalpha[usec] : blendalpha[usec]);
				SETRS(D3DRS_DESTBLEND, a.a == 0 ? blendalpha[usec] : blendinvalpha[usec]);
			}
		}
		else {
			assert(a.b == a.d);
			SETRS(D3DRS_BLENDOP, D3DBLENDOP_ADD);

			if( bDestAlphaColor == 2 ) {
				assert(usec==1);

				// all ones
				bNeedAlphaColor = 1;
				bNeedBlendFactorInAlpha = 1;
				vAlphaBlendColor.y = 0;
				vAlphaBlendColor.w = 1;
				SETRS(D3DRS_SRCBLEND, a.a != 0 ? D3DBLEND_INVSRCALPHA : D3DBLEND_SRCALPHA);
				SETRS(D3DRS_DESTBLEND, a.a != 0 ? D3DBLEND_SRCALPHA : D3DBLEND_INVSRCALPHA);
			}
			else {
				//b2XAlphaTest = 1;
				SETRS(D3DRS_SRCBLEND, a.a != 0 ? blendinvalpha[usec] : blendalpha[usec]);
				SETRS(D3DRS_DESTBLEND, a.a != 0 ? blendalpha[usec] : blendinvalpha[usec]);
			}
		}
	}

EndSetAlpha:
	//b2XAlphaTest = b2XAlphaTest && bNeedAlphaColor && !bNeedBlendFactorInAlpha;

	INC_ALPHAVARS();
}

void ZeroGS::SetWriteDepth()
{
	if( conf.mrtdepth ) {
		s_bWriteDepth = TRUE;
		s_nWriteDepthCount = 4;
	}
}

BOOL ZeroGS::IsWriteDepth()
{
	return s_bWriteDepth;
}

BOOL ZeroGS::IsWriteDestAlphaTest()
{
	return s_bWriteDepth;
}

void ZeroGS::SetDestAlphaTest()
{
	s_bDestAlphaTest = TRUE;
	s_nWriteDestAlphaTest = 4;
}

void ZeroGS::SetFogColor(u32 fog)
{
	if( 1||gs.fogcol != fog ) {
		gs.fogcol = fog;

		ZeroGS::Flush(0);
		ZeroGS::Flush(1);

		if( !g_bIsLost ) {
			DXVEC4 v;

			// set it immediately
			v.x = (gs.fogcol&0xff)/255.0f;
			v.y = ((gs.fogcol>>8)&0xff)/255.0f;
			v.z = ((gs.fogcol>>16)&0xff)/255.0f;
			SETCONSTF(GPU_FOGCOLOR, v);
		}
	}
}

void ZeroGS::ExtWrite()
{
	WARN_LOG("ExtWrite\n");

	// use local DISPFB, EXTDATA, EXTBUF, and PMODE
//	int bpp, start, end;
//	tex0Info texframe;

//	bpp = 4;
//	if( texframe.psm == 0x12 ) bpp = 3;
//	else if( texframe.psm & 2 ) bpp = 2;
//
//	// get the start and end addresses of the buffer
//	GetRectMemAddress(start, end, texframe.psm, 0, 0, texframe.tw, texframe.th, texframe.tbp0, texframe.tbw);
}

////////////
// Caches //
////////////
#ifdef __x86_64__
extern "C" void TestClutChangeMMX(void* src, void* dst, int entries, void* pret);
#endif

bool ZeroGS::CheckChangeInClut(u32 highdword, u32 psm)
{
	int cld = (highdword >> 29) & 0x7;
	int cbp = ((highdword >>  5) & 0x3fff);

	// processing the CLUT after tex0/2 are written
	switch(cld) {
		case 0: return false;
		case 1: break; // Seems to rarely not be 1.
		// note sure about changing cbp[0,1]
		case 4: return gs.cbp[0] != cbp;
		case 5: return gs.cbp[1] != cbp;

		// default: load
		default: break;
	}

	int cpsm = (highdword >> 19) & 0xe;
	int csm = (highdword >> 23) & 0x1;
	
	if( cpsm > 1 || csm )
		// don't support 16bit for now
		return true;

	int csa = (highdword >> 24) & 0x1f;

	int entries = (psm&3)==3 ? 256 : 16;

	u64* src = (u64*)(g_pbyGSMemory + cbp*256);
	u64* dst = (u64*)(g_pbyGSClut+64*csa);

	bool bRet = false;

#ifdef __x86_64__
	TestClutChangeMMX(dst, src, entries, &bRet);
#else
	int storeebx;
	// do a fast test with MMX
	__asm {
		mov storeebx, ebx
		mov edx, dst
		mov ecx, src
		mov ebx, entries

Start:
		movq mm0, [edx]
		movq mm1, [edx+8]
		pcmpeqd mm0, [ecx]
		pcmpeqd mm1, [ecx+16]

		movq mm2, [edx+16]
		movq mm3, [edx+24]
		pcmpeqd mm2, [ecx+32]
		pcmpeqd mm3, [ecx+48]

		pand mm0, mm1
		pand mm2, mm3
		movq mm4, [edx+32]
		movq mm5, [edx+40]
		pcmpeqd mm4, [ecx+8]
		pcmpeqd mm5, [ecx+24]

		pand mm0, mm2
		pand mm4, mm5
		movq mm6, [edx+48]
		movq mm7, [edx+56]
		pcmpeqd mm6, [ecx+40]
		pcmpeqd mm7, [ecx+56]
		
		pand mm0, mm4
		pand mm6, mm7
		pand mm0, mm6

		pmovmskb eax, mm0
		cmp eax, 0xff
		je Continue
		mov bRet, 1
		jmp Return

Continue:
		cmp ebx, 16
		jle Return

		test ebx, 0x10
		jz AddEcx
		sub ecx, 448 // go back and down one column, 
AddEcx:
		add ecx, 256 // go to the right block

		cmp ebx, 0x90
		jne Continue1
		add ecx, 256 // skip whole block
Continue1:
		add edx, 64
		sub ebx, 16
		jmp Start
	}

Return:
	__asm {
		emms
		mov ebx, storeebx
	}
#endif

	return bRet;
}

void ZeroGS::texClutWrite(int ctx)
{
	s_bTexFlush = 0;
	if( g_bIsLost )
		return;

	tex0Info& tex0 = vb[ctx].tex0;
	assert( PSMT_ISCLUT(tex0.psm) );

	// processing the CLUT after tex0/2 are written
	switch(tex0.cld) {
		case 0: return;
		case 1: break; // tex0.cld is usually 1.
		case 2: gs.cbp[0] = tex0.cbp; break;
		case 3: gs.cbp[1] = tex0.cbp; break;
		// not sure about changing cbp[0,1]
		case 4:
			if( gs.cbp[0] == tex0.cbp )
				return;
			gs.cbp[0] = tex0.cbp;
			break;
		case 5:
			if( gs.cbp[1] == tex0.cbp )
				return;
			gs.cbp[1] = tex0.cbp;
			break;
		default:  //DEBUG_LOG("cld isn't 0-5!");
			break;
	}

	Flush(!ctx);

	int entries = (tex0.psm&3)==3 ? 256 : 16;
if (tex0.csm) 
	{
		switch (tex0.cpsm)
		{
			// 16bit psm
			// eggomania uses non16bit textures for csm2
			case PSMCT16:
			{
				u16* src = (u16*)g_pbyGSMemory + tex0.cbp*128;
				u16 *dst = (u16*)(g_pbyGSClut+32*(tex0.csa&15)+(tex0.csa>=16?2:0));

				for (int i = 0; i < entries; ++i) 
				{
					*dst = src[getPixelAddress16_0(gs.clut.cou+i, gs.clut.cov, gs.clut.cbw)];
					dst += 2;
					
					// check for wrapping
					if (((u32)dst & 0x3ff) == 0) dst = (u16*)(g_pbyGSClut+2);
				}
				break;
			}
			
			case PSMCT16S:
			{
				u16* src = (u16*)g_pbyGSMemory + tex0.cbp*128;
				u16 *dst = (u16*)(g_pbyGSClut+32*(tex0.csa&15)+(tex0.csa>=16?2:0));

				for (int i = 0; i < entries; ++i) 
				{
					*dst = src[getPixelAddress16S_0(gs.clut.cou+i, gs.clut.cov, gs.clut.cbw)];
					dst += 2;
					
					// check for wrapping
					if (((u32)dst & 0x3ff) == 0) dst = (u16*)(g_pbyGSClut+2);
				}
			break;
			}
		
			case PSMCT32:
			case PSMCT24:
			{
				u32* src = (u32*)g_pbyGSMemory + tex0.cbp*64;
				u32 *dst = (u32*)(g_pbyGSClut+64*tex0.csa);
				
				// check if address exceeds src
				if( src+getPixelAddress32_0(gs.clut.cou+entries-1, gs.clut.cov, gs.clut.cbw) >= (u32*)g_pbyGSMemory + 0x00100000 ) 
					ERROR_LOG("texClutWrite out of bounds\n");
				else 
					for(int i = 0; i < entries; ++i) 
					{
						*dst = src[getPixelAddress32_0(gs.clut.cou+i, gs.clut.cov, gs.clut.cbw)];
						dst++;
					}
			}
			break;
			
			default:
			{
#ifndef RELEASE_TO_PUBLIC
			//DEBUG_LOG("unknown cpsm: %x (%x)\n", tex0.cpsm, tex0.psm);
#endif
				break;
			}
		}
	}
	else 
	{
		switch (tex0.cpsm)
		{
			case PSMCT24:
			case PSMCT32:
				if( entries == 16 )
					WriteCLUT_T32_I4_CSM1((u32*)(g_pbyGSMemory + tex0.cbp*256), (u32*)(g_pbyGSClut+64*tex0.csa));
				else 
					WriteCLUT_T32_I8_CSM1((u32*)(g_pbyGSMemory + tex0.cbp*256), (u32*)(g_pbyGSClut+64*tex0.csa));
				break;
				
			default:
				if( entries == 16 )
					WriteCLUT_T16_I4_CSM1((u32*)(g_pbyGSMemory + 256 * tex0.cbp), (u32*)(g_pbyGSClut+32*(tex0.csa&15)+(tex0.csa>=16?2:0)));
				else // sse2 for 256 is more complicated, so use regular
					WriteCLUT_T16_I8_CSM1_c((u32*)(g_pbyGSMemory + 256 * tex0.cbp), (u32*)(g_pbyGSClut+32*(tex0.csa&15)+(tex0.csa>=16?2:0)));
				break;
			
		}
	}
}

void ZeroGS::SetTexFlush()
{
	s_bTexFlush = TRUE;

//	if( PSMT_ISCLUT(vb[0].tex0.psm) )
//		texClutWrite(0);
//	if( PSMT_ISCLUT(vb[1].tex0.psm) )
//		texClutWrite(1);

	if( !s_bForceTexFlush ) 
	{
		if( s_ptexCurSet[0] != s_ptexNextSet[0] ) 
		{
			s_ptexCurSet[0] = s_ptexNextSet[0];
			pd3dDevice->SetTexture(SAMP_MEMORY0, s_ptexNextSet[0]);
		}
		if( s_ptexCurSet[1] != s_ptexNextSet[1] ) 
		{
			s_ptexCurSet[1] = s_ptexNextSet[1];
			pd3dDevice->SetTexture(SAMP_MEMORY1, s_ptexNextSet[1]);
		}
	}
}

int ZeroGS::Save(char* pbydata)
{
	if( pbydata == NULL )
		return 40 + 0x00400000 + sizeof(gs) + 2*VBSAVELIMIT + 2*sizeof(frameInfo) + 4 + 256*4;

	s_RTs.ResolveAll();
	s_DepthRTs.ResolveAll();
	
	vb[0].Unlock();
	vb[1].Unlock();
	
	strcpy(pbydata, libraryName);
	*(u32*)(pbydata+16) = ZEROGS_SAVEVER;
	pbydata += 32;

	*(int*)pbydata = icurctx; pbydata += 4;
	*(int*)pbydata = VBSAVELIMIT; pbydata += 4;

	memcpy(pbydata, g_pbyGSMemory, 0x00400000);
	pbydata += 0x00400000;

	memcpy(pbydata, g_pbyGSClut, 256*4);
	pbydata += 256*4;

	*(int*)pbydata = sizeof(gs);
	pbydata += 4;
	memcpy(pbydata, &gs, sizeof(gs));
	pbydata += sizeof(gs);

	for(int i = 0; i < 2; ++i) {
		memcpy(pbydata, &vb[i], VBSAVELIMIT);
		pbydata += VBSAVELIMIT;
	}

	vb[0].Lock();
	vb[1].Lock();

	return 0;
}

extern u32 s_uTex1Data[2][2], s_uClampData[2];
extern char *libraryName;
bool ZeroGS::Load(char* pbydata)
{
	memset(s_uTex1Data, 0, sizeof(s_uTex1Data));
	memset(s_uClampData, 0, sizeof(s_uClampData));

	// first 32 bytes are the id
	u32 savever = *(u32*)(pbydata+16);
	
	if( strncmp(pbydata, libraryName, 6) == 0 && (savever == ZEROGS_SAVEVER || savever == 0xaa000004) ) {

		g_MemTargs.Destroy();

		GSStateReset();
		pbydata += 32;

		int context = *(int*)pbydata; pbydata += 4;
		u32 savelimit = VBSAVELIMIT;
		
		savelimit = *(u32*)pbydata; pbydata += 4;

		memcpy(g_pbyGSMemory, pbydata, 0x00400000);
		pbydata += 0x00400000;

		memcpy(g_pbyGSClut, pbydata, 256*4);
		pbydata += 256*4;

		memset(&gs, 0, sizeof(gs));

		int savedgssize;
		if( savever == 0xaa000004 )
			savedgssize = 0x1d0;
		else {
			savedgssize = *(int*)pbydata;
			pbydata += 4;
		}

		memcpy(&gs, pbydata, savedgssize);
		pbydata += savedgssize;
		prim = &gs._prim[gs.prac];

		LPD3DVB pvb = vb[0].pvb;
		memcpy(&vb[0], pbydata, min(savelimit, VBSAVELIMIT));
		pbydata += savelimit;
		vb[0].pvb = pvb;

		pvb = vb[1].pvb;
		memcpy(&vb[1], pbydata, min(savelimit, VBSAVELIMIT));
		pbydata += savelimit;
		vb[1].pvb = pvb;

		for(int i = 0; i < 2; ++i) {
			vb[i].bNeedZCheck = vb[i].bNeedFrameCheck = 1;
			
			vb[i].bSyncVars = 0; vb[i].bNeedTexCheck = 1;
			memset(vb[i].uCurTex0Data, 0, sizeof(vb[i].uCurTex0Data));
		}

		icurctx = -1;
		
		pd3dDevice->SetRenderTarget(0, psurfOrgTarg);
		pd3dDevice->SetRenderTarget(1, NULL);
		pd3dDevice->SetDepthStencilSurface(psurfOrgDepth);
		SetFogColor(gs.fogcol);

		vb[0].Lock();
		vb[1].Lock();

		return true;
	}

	return false;
}

void ZeroGS::SaveSnapshot(const char* filename)
{
	g_bMakeSnapshot = 1;
	strSnapshot = filename;
}

// AVI capture stuff
bool ZeroGS::StartCapture()
{
	if( !s_aviinit ) {
		START_AVI("zerogs.avi");

		assert( s_ptexAVICapture == NULL );
		if( FAILED(pd3dDevice->CreateOffscreenPlainSurface(width, height, D3DFMT_A8R8G8B8, D3DPOOL_SYSTEMMEM, &s_ptexAVICapture, NULL)) ) {
			STOP_AVI();
			ZeroGS::AddMessage("Failed to create avi");
			return false;
		}

		s_aviinit = 1;
	}
	else {
		DEBUG_LOG("ZeroGS: Continuing from previous capture");
	}

	s_avicapturing = 1;
	return true;
}

void ZeroGS::StopCapture()
{
	s_avicapturing = 0;
}

void ZeroGS::CaptureFrame()
{
	assert( s_avicapturing && s_aviinit && s_ptexAVICapture != NULL );

	vector<BYTE> mem;

	pd3dDevice->GetRenderTargetData(psurfOrgTarg, s_ptexAVICapture);
	
	D3DLOCKED_RECT lock;
	mem.resize(width * height * 4);

	s_ptexAVICapture->LockRect(&lock, NULL, D3DLOCK_READONLY);
	assert( lock.Pitch == width*4 );

	BYTE* pend = (BYTE*)lock.pBits + (conf.height-1)*width*4;
	for(int i = 0; i < conf.height; ++i) {
		memcpy_amd(&mem[width*4*i], pend - width*4*i, width * 4);
	}
	s_ptexAVICapture->UnlockRect();

	int fps = SMODE1->CMOD == 3 ? 50 : 60;
	bool bSuccess = ADD_FRAME_FROM_DIB_TO_AVI("AAAA", fps, width, height, 32, &mem[0]);

	if( !bSuccess ) {
		s_avicapturing = 0;
		STOP_AVI();
		s_aviinit = 0;
		ZeroGS::AddMessage("Failed to create avi");
		return;
	}
}
