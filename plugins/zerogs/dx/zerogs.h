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

#ifndef __ZEROGS__H
#define __ZEROGS__H

#pragma warning(disable:4200) // nonstandard extension used : zero-sized array in struct/union

#include <tchar.h>
#include <assert.h>

#include <list>
#include <vector>
#include <map>
#include <string>
#include <list>
using namespace std;

#ifndef SAFE_DELETE
#define SAFE_DELETE(x)		if( (x) != NULL ) { delete (x); (x) = NULL; }
#endif
#ifndef SAFE_DELETE_ARRAY
#define SAFE_DELETE_ARRAY(x)	if( (x) != NULL ) { delete[] (x); (x) = NULL; }
#endif
#ifndef SAFE_RELEASE
#define SAFE_RELEASE(x)		if( (x) != NULL ) { (x)->Release(); (x) = NULL; }
#endif

#define FORIT(it, v) for(it = (v).begin(); it != (v).end(); ++(it))

typedef D3DXVECTOR2 DXVEC2;
typedef D3DXVECTOR3 DXVEC3;
typedef D3DXVECTOR4 DXVEC4;
typedef D3DXMATRIX DXMAT;

// sends a message to output window if assert fails
#define BMSG(x, str)			{ if( !(x) ) { GS_LOG(str); GS_LOG(str); } }
#define BMSG_RETURN(x, str)	{ if( !(x) ) { GS_LOG(str); GS_LOG(str); return; } }
#define BMSG_RETURNX(x, str, rtype)	{ if( !(x) ) { GS_LOG(str); GS_LOG(str); return (##rtype); } }
#define B(x)				{ if( !(x) ) { GS_LOG(_T(#x"\n")); GS_LOG(#x"\n"); } }
#define B_RETURN(x)			{ if( !(x) ) { GS_LOG(_T(#x"\n")); GS_LOG(#x"\n"); return; } }
#define B_RETURNX(x, rtype)			{ if( !(x) ) { GS_LOG(_T(#x"\n")); GS_LOG(#x"\n"); return (##rtype); } }
#define B_G(x, action)			{ if( !(x) ) { GS_LOG(#x"\n"); action; } }

#ifndef RELEASE_TO_PUBLIC
	#ifndef V
#define V(x)		{ hr = x; if( FAILED(hr) ) { ERROR_LOG("%s:%d: %s (%8.8x)\n", __FILE__, (DWORD)__LINE__, _T(#x), hr); } }
	#endif
	#ifndef V_RETURN
		#define V_RETURN(x)		{ hr = x; if( FAILED(hr) ) { ERROR_LOG("%s:%d: %s (%8.8x)\n", __FILE__, (DWORD)__LINE__, _T(#x), hr); return hr; } }
	#endif
#else
	#ifndef V
		#define V(x)		   { hr = x; }
	#endif
	#ifndef V_RETURN
		#define V_RETURN(x)		{ hr = x; if( FAILED(hr) ) { return hr; } }
	#endif
#endif

#define SETRS(state, val)	pd3dDevice->SetRenderState(state, val)
#ifndef ArraySize
#define ArraySize(x) (sizeof(x) / sizeof((x)[0]))
#endif

// all textures have this width
//#define GPU_TEXWIDTH		512
extern int GPU_TEXWIDTH;
extern float g_fiGPU_TEXWIDTH;
#define GPU_TEXMASKWIDTH	1024 // bitwise mask width for region repeat mode

// set these shader constants
#define GPU_TEXALPHA20		(2|0x8000)
#define GPU_TEXOFFSET0		(4|0x8000)
#define GPU_TEXDIMS0		(6|0x8000)
#define GPU_TEXBLOCK0		(8|0x8000)
#define GPU_CLAMPEXTS0		(10|0x8000)
#define GPU_TEXWRAPMODE0	(12|0x8000)
#define GPU_REALTEXDIMS0	(14|0x8000)
#define GPU_TESTBLACK0		(16|0x8000)
#define GPU_PAGEOFFSET0		(18|0x8000)
#define GPU_TEXALPHA0		(20|0x8000)

#define GPU_INVTEXDIMS	(22|0x8000)
#define GPU_FOGCOLOR	(23|0x8000)
#define GPU_BITBLTZ		(24|0x8000)
#define GPU_ONECOLOR	(25|0x8000)

#define GPU_CLIPPLANE0  (24|0x8000)
#define GPU_CLIPPLANE1  (26|0x8000)

#define GPU_POSXY0		2

#define GPU_BITBLTPOS	4
#define GPU_Z			5
#define GPU_ZNORM		6
#define GPU_BITBLTTEX	7

#define SETCONSTF(id, ptr) { if( id & 0x8000 ) pd3dDevice->SetPixelShaderConstantF(id&0x7fff, (FLOAT*)ptr, 1); \
							else pd3dDevice->SetVertexShaderConstantF(id&0x7fff, (FLOAT*)ptr, 1); }

#define SAMP_MEMORY0		0
#define SAMP_MEMORY1		1
#define SAMP_FINAL			2
#define SAMP_BLOCKS			3
#define SAMP_BILINEARBLOCKS	4
#define SAMP_INTERLACE		5
#define SAMP_BITWISEANDX	5
#define SAMP_BITWISEANDY	6

// don't change these values!
#define GAME_TEXTURETARGS	0x01
#define GAME_AUTORESET		0x02
#define GAME_INTERLACE2X	0x04
#define GAME_TEXAHACK		0x08 // apply texa to non textured polys
#define GAME_NOTARGETRESOLVE 0x10
#define GAME_EXACTCOLOR		0x20
#define GAME_NOCOLORCLAMP	0x40
#define GAME_FFXHACK		0x80
#define GAME_NODEPTHUPDATE  0x0200
#define GAME_QUICKRESOLVE1  0x0400
#define GAME_NOQUICKRESOLVE 0x0800
#define GAME_NOTARGETCLUT   0x1000 // full 16 bit resolution
#define GAME_NOSTENCIL	  0x2000
#define GAME_VSSHACKOFF		0x4000 // vertical stripe syndrome
#define GAME_NODEPTHRESOLVE 0x8000
#define GAME_FULL16BITRES   0x00010000
#define GAME_RESOLVEPROMOTED 0x00020000
#define GAME_FASTUPDATE 0x00040000
#define GAME_NOALPHATEST 0x00080000
#define GAME_DISABLEMRTDEPTH 0x00100000
#define GAME_32BITTARGS 0x00200000
#define GAME_PATH3HACK 0x00400000
#define GAME_DOPARALLELCTX 0x00800000 // tries to parallelize both contexts so that render calls are reduced (xenosaga)
									  // makes the game faster, but can be buggy
#define GAME_XENOSPECHACK 0x01000000 // xenosaga specularity hack (ignore any zmask=1 draws)
#define GAME_PARTIALPOINTERS 0x02000000 // whenver the texture or render target are small, tries to look for bigger ones to read from
#define GAME_PARTIALDEPTH 0x04000000 // tries to save depth targets as much as possible across height changes
#define GAME_RELAXEDDEPTH 0x08000000 // tries to save depth targets as much as possible across height changes
									
#define USEALPHABLENDING 1//(!(g_GameSettings&GAME_NOALPHABLEND))
#define USEALPHATESTING (!(g_GameSettings&GAME_NOALPHATEST))

typedef IDirect3DDevice9* LPD3DDEV;
typedef IDirect3DVertexBuffer9* LPD3DVB;
typedef IDirect3DIndexBuffer9* LPD3DIB;
typedef IDirect3DBaseTexture9* LPD3DBASETEX;
typedef IDirect3DTexture9* LPD3DTEX;
typedef IDirect3DCubeTexture9* LPD3DCUBETEX;
typedef IDirect3DVolumeTexture9* LPD3DVOLUMETEX;
typedef IDirect3DSurface9* LPD3DSURF;
typedef IDirect3DVertexDeclaration9* LPD3DDECL;
typedef IDirect3DVertexShader9* LPD3DVS;
typedef IDirect3DPixelShader9* LPD3DPS;
typedef ID3DXAnimationController* LPD3DXAC;

namespace ZeroGS
{
	typedef void (*DrawFn)();

	// managers render-to-texture targets
	class CRenderTarget
	{
	public:
		CRenderTarget();
		CRenderTarget(const frameInfo& frame, CRenderTarget& r); // virtualized a target
		virtual ~CRenderTarget();

		virtual BOOL Create(const frameInfo& frame);
		virtual void Destroy();

		// set the GPU_POSXY variable, scissor rect, and current render target
		void SetTarget(int fbplocal, const Rect2& scissor, int context);
		void SetViewport();

		// copies/creates the feedback contents
		inline void CreateFeedback() {
			if( ptexFeedback == NULL || !(status&TS_FeedbackReady) )
				_CreateFeedback();
		}

		virtual void Resolve();
		virtual void Resolve(int startrange, int endrange); // resolves only in the allowed range
		virtual void Update(int context, CRenderTarget* pdepth);
		virtual void ConvertTo32(); // converts a psm==2 target, to a psm==0
		virtual void ConvertTo16(); // converts a psm==0 target, to a psm==2

		virtual bool IsDepth() { return false; }

		LPD3DSURF psurf, psys;
		LPD3DTEX ptex;

		int targoffx, targoffy; // the offset from the target that the real fbp starts at (in pixels)
		int targheight;		 // height of ptex
		CRenderTarget* pmimicparent; // if not NULL, this target is a subtarget of pmimicparent

		int fbp, fbw, fbh; // if fbp is negative, virtual target (not mapped to any real addr)
		int start, end; // in bytes
		u32 lastused;	// time stamp since last used
		DXVEC4 vposxy;

		u32 fbm;
		u16 status;
		u8 psm;
		u8 resv0;
		RECT scissorrect;

		//int startresolve, endresolve;
		u32 nUpdateTarg; // use this target to update the texture if non 0 (one time only)

		// this is optionally used when feedback effects are used (render target is used as a texture when rendering to itself)
		LPD3DTEX ptexFeedback;
		LPD3DSURF psurfFeedback;

		enum TargetStatus {
			TS_Resolved = 1,
			TS_NeedUpdate = 2,
			TS_Virtual = 4, // currently not mapped to memory
			TS_FeedbackReady = 8, // feedback effect is ready and doesn't need to be updated
			TS_NeedConvert32 = 16,
			TS_NeedConvert16 = 32,
		};

	private:
		void _CreateFeedback();
	};

	// manages zbuffers
	class CDepthTarget : public CRenderTarget
	{
	public:
		CDepthTarget();
		virtual ~CDepthTarget();

		virtual BOOL Create(const frameInfo& frame);
		virtual void Destroy();

		virtual void Resolve();
		virtual void Resolve(int startrange, int endrange); // resolves only in the allowed range
		virtual void Update(int context, CRenderTarget* prndr);

		virtual bool IsDepth() { return true; }

		void SetDepthTarget();

		LPD3DSURF pdepth;
	};

	// manages contiguous chunks of memory (width is always 1024)
	class CMemoryTarget
	{
	public:
		struct MEMORY
		{
			inline MEMORY() : ptr(NULL), ref(0) {}
			inline ~MEMORY() { _aligned_free(ptr); }

			BYTE* ptr;
			int ref;
		};

		inline CMemoryTarget() : ptex(NULL), ptexsys(NULL), starty(0), height(0), realy(0), realheight(0), usedstamp(0), psm(0), 
			cpsm(0), memory(NULL), clearminy(0), clearmaxy(0), validatecount(0) {}
		inline CMemoryTarget(const CMemoryTarget& r) {
			ptex = r.ptex;
			ptexsys = r.ptexsys;
			if( ptex != NULL ) ptex->AddRef();
			if( ptexsys != NULL ) ptexsys->AddRef();
			starty = r.starty;
			height = r.height;
			realy = r.realy;
			realheight = r.realheight;
			usedstamp = r.usedstamp;
			psm = r.psm;
			cpsm = r.cpsm;
			clut = r.clut;
			memory = r.memory;
			clearminy = r.clearminy;
			clearmaxy = r.clearmaxy;
			widthmult = r.widthmult;
			validatecount = r.validatecount;
			fmt = r.fmt;
			if( memory != NULL ) memory->ref++;
		}

		~CMemoryTarget() { Destroy(); }

		inline void Destroy() {
			SAFE_RELEASE(ptex);
			SAFE_RELEASE(ptexsys);

			if( memory != NULL && memory->ref > 0 ) {
				if( --memory->ref <= 0 ) {
					SAFE_DELETE(memory);
				}
			}
			memory = NULL;
		}

		// returns true if clut data is synced
		bool ValidateClut(const tex0Info& tex0);
		// returns true if tex data is synced
		bool ValidateTex(const tex0Info& tex0, int starttex, int endtex, bool bDeleteBadTex);

		int clearminy, clearmaxy; // when maxy > 0, need to check for clearing

		// realy is offset in pixels from start of valid region
		// so texture in memory is [realy,starty+height]
		// valid texture is [starty,starty+height]
		// offset in mem [starty-realy, height]
		int starty, height; // assert(starty >= realy)
		int realy, realheight; // this is never touched once allocated
		u32 usedstamp;
		LPD3DTEX ptex; // can be 16bit if 
		LPD3DTEX ptexsys;
		D3DFORMAT fmt;

		int widthmult;
		
		int validatecount; // count how many times has been validated, if too many, destroy

		vector<BYTE> clut; // if nonzero, texture uses CLUT
		MEMORY* memory; // system memory used to compare for changes
		u8 psm, cpsm; // texture and clut format. For psm, only 16bit/32bit differentiation matters
	};


	struct VB
	{
		VB();
		~VB();

		void Destroy();

		inline bool IsLocked() { return pbuf != NULL; }
		inline void Lock();
		inline void Unlock() {
			if( pbuf != NULL ) {
				pvb->Unlock();
				pbuf = NULL;
			}
		}

		__forceinline bool CheckPrim();
		void CheckFrame(int tbp);

		// context specific state
		Point offset;
		Rect2 scissor;
		tex0Info tex0;
		tex1Info tex1;
		miptbpInfo miptbp0;
		miptbpInfo miptbp1;
		alphaInfo alpha;
		fbaInfo fba;
		clampInfo clamp;
		pixTest test;
		LPD3DTEX ptexClamp[2]; // textures for x and y dir region clamping

	public:
		
		void FlushTexData();

		u8 bNeedFrameCheck;
		u8 bNeedZCheck;
		u8 bNeedTexCheck;
		u8 dummy0;

		union {
			struct {
				u8 bTexConstsSync; // only pixel shader constants that context owns
				u8 bVarsTexSync; // texture info
				u8 bVarsSetTarg;
				u8 dummy1;
			};
			u32 bSyncVars;
		};

		int ictx;
		VertexGPU* pbuf;
		DWORD dwCurOff;
		DWORD dwCount;
		primInfo curprim;	// the previous prim the current buffers are set to

		zbufInfo zbuf;
		frameInfo gsfb; // the real info set by FRAME cmd
		frameInfo frame;
		int zprimmask; // zmask for incoming points

		u32 uCurTex0Data[2]; // current tex0 data
		u32 uNextTex0Data[2]; // tex0 data that has to be applied if bNeedTexCheck is 1 

		//int nFrameHeights[8];	// frame heights for the past frame changes
		int nNextFrameHeight;

		CMemoryTarget* pmemtarg; // the current mem target set
		LPD3DVB pvb;
		CRenderTarget* prndr;
		CDepthTarget* pdepth;
	};

	// visible members
	extern DrawFn drawfn[8];	
	extern VB vb[2];
	extern float fiTexWidth[2], fiTexHeight[2];	// current tex width and height
	extern LONG width, height;
	extern u8* g_pbyGSMemory;
	extern u8* g_pbyGSClut; // the temporary clut buffer

	void AddMessage(const char* pstr, DWORD ms = 5000);
	void ChangeWindowSize(int nNewWidth, int nNewHeight);
	void SetChangeDeviceSize(int nNewWidth, int nNewHeight);
	void ChangeDeviceSize(int nNewWidth, int nNewHeight);
	void SetAA(int mode);
	void SetCRC(int crc);

	void ReloadEffects();

	// Methods //

	HRESULT Create(LONG width, LONG height);
	void Destroy(BOOL bD3D);

	void Restore(); // call to restore device
	void Reset(); // call to destroy video resources

	HRESULT InitDeviceObjects();
	void DeleteDeviceObjects();

	void GSStateReset();

	// called on a primitive switch
	void Prim();

	void SetTexFlush();
	// flush current vertices, call before setting new registers (the main render method)
	void Flush(int context);

	void ExtWrite();

	void SetWriteDepth();
	BOOL IsWriteDepth();

	void SetDestAlphaTest();
	BOOL IsWriteDestAlphaTest();

	void SetFogColor(u32 fog);
	void SaveTex(tex0Info* ptex, int usevid);

	// called when trxdir is accessed. If host is involved, transfers memory to temp buffer byTransferBuf.
	// Otherwise performs the transfer. TODO: Perhaps divide the transfers into chunks?
	void InitTransferHostLocal();
	void TransferHostLocal(const void* pbyMem, u32 nQWordSize);

	void InitTransferLocalHost();
	void TransferLocalHost(void* pbyMem, u32 nQWordSize);
	inline void TerminateLocalHost() {}

	void TransferLocalLocal();

	// switches the render target to the real target, flushes the current render targets and renders the real image
	void RenderCRTC(int interlace);

	bool CheckChangeInClut(u32 highdword, u32 psm); // returns true if clut will change after this tex0 op

	// call to load CLUT data (depending on CLD)
	void texClutWrite(int ctx);

	int Save(char* pbydata);
	bool Load(char* pbydata);

	void SaveSnapshot(const char* filename);

	// private methods
	void FlushSysMem(const RECT* prc);
	void _Resolve(const D3DLOCKED_RECT& locksrc, int fbp, int fbw, int fbh, int psm, u32 fbm);

	// returns the first and last addresses aligned to a page that cover 
	void GetRectMemAddress(int& start, int& end, int psm, int x, int y, int w, int h, int bp, int bw);

	// inits the smallest rectangle in ptexMem that covers this region in ptexMem
	// returns the offset that needs to be added to the locked rect to get the beginning of the buffer
	//void GetMemRect(RECT& rc, int psm, int x, int y, int w, int h, int bp, int bw);

	// only sets a limited amount of state (for Update)
	void SetTexVariablesInt(int context, int bilinear, const tex0Info& tex0, ZeroGS::CMemoryTarget* pmemtarg, int force);

	void ResetAlphaVariables();

	bool StartCapture();
	void StopCapture();
	void CaptureFrame();
};

extern LPDIRECT3DDEVICE9 pd3dDevice;
extern const u32 g_primmult[8];
extern const u32 g_primsub[8];

#endif
