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

#ifndef __ZEROGS__H
#define __ZEROGS__H

#pragma warning(disable:4200) // nonstandard extension used : zero-sized array in struct/union

#ifndef _WIN32
// adding glew support instead of glXGetProcAddress (thanks to scaught)
#include <GL/glew.h>
#endif

#include <GL/gl.h>
#include <GL/glext.h>

#ifndef _WIN32
#include <GL/glx.h>
inline void* wglGetProcAddress(const char* x) {
  return (void*)glXGetProcAddress((const GLubyte*)x);
}
#else
#include "glprocs.h"
#endif

#include <Cg/cg.h>
#include <Cg/cgGL.h>

#ifndef GL_DEPTH24_STENCIL8_EXT // allows FBOs to support stencils
#define GL_DEPTH_STENCIL_EXT 0x84F9
#define GL_UNSIGNED_INT_24_8_EXT 0x84FA
#define GL_DEPTH24_STENCIL8_EXT 0x88F0
#define GL_TEXTURE_STENCIL_SIZE_EXT 0x88F1
#endif

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

#define SAFE_RELEASE_PROG(x) { if( (x) != NULL ) { cgDestroyProgram(x); x = NULL; } }
#define SAFE_RELEASE_TEX(x) { if( (x) != 0 ) { glDeleteTextures(1, &(x)); x = 0; } }

#define FORIT(it, v) for(it = (v).begin(); it != (v).end(); ++(it))

// sends a message to output window if assert fails
#define BMSG(x, str)			{ if( !(x) ) { GS_LOG(str); GS_LOG(str); } }
#define BMSG_RETURN(x, str)	{ if( !(x) ) { GS_LOG(str); GS_LOG(str); return; } }
#define BMSG_RETURNX(x, str, rtype)	{ if( !(x) ) { GS_LOG(str); GS_LOG(str); return (##rtype); } }
#define B(x)				{ if( !(x) ) { GS_LOG(_#x"\n"); GS_LOG(#x"\n"); } }
#define B_RETURN(x)			{ if( !(x) ) { ERROR_LOG("%s:%d: %s\n", __FILE__, (u32)__LINE__, #x); return; } }
#define B_RETURNX(x, rtype)			{ if( !(x) ) { ERROR_LOG("%s:%d: %s\n", __FILE__, (u32)__LINE__, #x); return (##rtype); } }
#define B_G(x, action)			{ if( !(x) ) { ERROR_LOG("%s:%d: %s\n", __FILE__, (u32)__LINE__, #x); action; } }


static __forceinline const char *error_name(int err)
{
	switch (err)
	{
		case GL_NO_ERROR:
			return "GL_NO_ERROR";
		case GL_INVALID_ENUM:
			return "GL_INVALID_ENUM";
		case GL_INVALID_VALUE:
			return "GL_INVALID_VALUE";
		case GL_INVALID_OPERATION:
			return "GL_INVALID_OPERATION";
		case GL_STACK_OVERFLOW:
			return "GL_STACK_OVERFLOW";
		case GL_STACK_UNDERFLOW:
			return "GL_STACK_UNDERFLOW";
		case GL_OUT_OF_MEMORY:
			return "GL_OUT_OF_MEMORY";
		case GL_TABLE_TOO_LARGE:
			return "GL_TABLE_TOO_LARGE";
		default:
		{
			char *str;
			sprintf(str, "Unknown error(0x%x)", err);
			return str;
		}
	}
}

#define GL_ERROR_LOG() \
{ \
	GLenum myGLerror = glGetError(); \
	\
	if( myGLerror != GL_NO_ERROR ) \
	{ \
		ERROR_LOG("%s:%d: gl error %s\n", __FILE__, (int)__LINE__, error_name(myGLerror)); \
	} \
}\
	
#define GL_REPORT_ERROR() \
{ \
	err = glGetError(); \
	if( err != GL_NO_ERROR ) \
	{ \
		ERROR_LOG("%s:%d: gl error %s\n", __FILE__, (int)__LINE__, error_name(err)); \
		ZeroGS::HandleGLError(); \
	} \
}
 
 #ifdef _DEBUG
#define GL_REPORT_ERRORD() \
{ \
	GLenum err = glGetError(); \
	if( err != GL_NO_ERROR ) \
	{ \
		ERROR_LOG("%s:%d: gl error %s\n", __FILE__, (int)__LINE__, error_name(err)); \
		ZeroGS::HandleGLError(); \
	} \
}
#else
#define GL_REPORT_ERRORD()
#endif

// sets the data stream
#define SET_STREAM() { \
	glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(VertexGPU), (void*)8); \
	glSecondaryColorPointerEXT(4, GL_UNSIGNED_BYTE, sizeof(VertexGPU), (void*)12); \
	glTexCoordPointer(3, GL_FLOAT, sizeof(VertexGPU), (void*)16); \
	glVertexPointer(4, GL_SHORT, sizeof(VertexGPU), (void*)0); \
}

#define SETVERTEXSHADER(prog) { \
	if( (prog) != g_vsprog ) { \
		cgGLBindProgram(prog); \
		g_vsprog = prog; \
	} \
} \

#define SETPIXELSHADER(prog) { \
	if( (prog) != g_psprog ) { \
		cgGLBindProgram(prog); \
		g_psprog = prog; \
	} \
} \

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#endif

// all textures have this width
//#define GPU_TEXWIDTH		512
extern int GPU_TEXWIDTH;
extern float g_fiGPU_TEXWIDTH;
#define GPU_TEXMASKWIDTH	1024 // bitwise mask width for region repeat mode

extern CGprogram g_vsprog, g_psprog;

struct FRAGMENTSHADER
{
	FRAGMENTSHADER() : prog(0), sMemory(0), sFinal(0), sBitwiseANDX(0), sBitwiseANDY(0), sInterlace(0), sCLUT(0), sOneColor(0), sBitBltZ(0),
		fTexAlpha2(0), fTexOffset(0), fTexDims(0), fTexBlock(0), fClampExts(0), fTexWrapMode(0),
		fRealTexDims(0), fTestBlack(0), fPageOffset(0), fTexAlpha(0) {}
	
	CGprogram prog;
	CGparameter sMemory, sFinal, sBitwiseANDX, sBitwiseANDY, sCLUT, sInterlace;
	CGparameter sOneColor, sBitBltZ, sInvTexDims;
	CGparameter fTexAlpha2, fTexOffset, fTexDims, fTexBlock, fClampExts, fTexWrapMode, fRealTexDims, fTestBlack, fPageOffset, fTexAlpha;

#ifdef _DEBUG
	string filename;
#endif
};

struct VERTEXSHADER
{
	VERTEXSHADER() : prog(0), sBitBltPos(0), sBitBltTex(0) {}
	CGprogram prog;
	CGparameter sBitBltPos, sBitBltTex, fBitBltTrans;		 // vertex shader constants
};

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

#define USEALPHATESTING (!(g_GameSettings&GAME_NOALPHATEST))

extern u8* g_pbyGSMemory;
extern u8* g_pbyGSClut; // the temporary clut buffer
extern CGparameter g_vparamPosXY[2], g_fparamFogColor;

namespace ZeroGS {
	
	typedef void (*DrawFn)();
	
	enum RenderFormatType
	{
		RFT_byte8 = 0,	  // A8R8G8B8
		RFT_float16 = 1,	// A32R32B32G32
	};
	
	// managers render-to-texture targets
	class CRenderTarget
	{
	public:
		CRenderTarget();
		virtual ~CRenderTarget();
		
		virtual bool Create(const frameInfo& frame);
		virtual void Destroy();
		
		// set the GPU_POSXY variable, scissor rect, and current render target
		void SetTarget(int fbplocal, const Rect2& scissor, int context);
		void SetViewport();
		
		// copies/creates the feedback contents
		inline void CreateFeedback() {
			if( ptexFeedback == 0 || !(status&TS_FeedbackReady) )
				_CreateFeedback();
		}
		
		virtual void Resolve();
		virtual void Resolve(int startrange, int endrange); // resolves only in the allowed range
		virtual void Update(int context, CRenderTarget* pdepth);
		virtual void ConvertTo32(); // converts a psm==2 target, to a psm==0
		virtual void ConvertTo16(); // converts a psm==0 target, to a psm==2
		
		virtual bool IsDepth() { return false; }
		void SetRenderTarget(int targ);
		
		void* psys;   // system data used for comparison
		u32 ptex;
		
		int fbp, fbw, fbh; // if fbp is negative, virtual target (not mapped to any real addr)
		int start, end; // in bytes
		u32 lastused;	// time stamp since last used
		Vector vposxy;
		
		u32 fbm;
		u16 status;
		u8 psm;
		u8 resv0;
		Rect scissorrect;
		
		//int startresolve, endresolve;
		u32 nUpdateTarg; // use this target to update the texture if non 0 (one time only)
	
		// this is optionally used when feedback effects are used (render target is used as a texture when rendering to itself)
		u32 ptexFeedback;
		
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
		
		virtual bool Create(const frameInfo& frame);
		virtual void Destroy();
		
		virtual void Resolve();
		virtual void Resolve(int startrange, int endrange); // resolves only in the allowed range
		virtual void Update(int context, CRenderTarget* prndr);
		
		virtual bool IsDepth() { return true; }
		
		void SetDepthStencilSurface();
		
		u32 pdepth;		 // 24 bit, will contain the stencil buffer if possible
		u32 pstencil;	   // if not 0, contains the stencil buffer
		int icount;		 // internal counter
	};
	
	// manages contiguous chunks of memory (width is always 1024)
	class CMemoryTarget
	{
	public:
		struct TEXTURE
		{
			inline TEXTURE() : tex(0), memptr(NULL), ref(0) {}
			inline ~TEXTURE() { glDeleteTextures(1, &tex); _aligned_free(memptr); }
		u32 tex;
			int ref;
			u8* memptr;  // GPU memory used for comparison
		};
		
		inline CMemoryTarget() : ptex(NULL), starty(0), height(0), realy(0), realheight(0), usedstamp(0), psm(0), channels(0),cpsm(0), clearminy(0), clearmaxy(0), validatecount(0) {}
			
		inline CMemoryTarget(const CMemoryTarget& r) {
			ptex = r.ptex;
			if( ptex != NULL ) ptex->ref++;
			starty = r.starty;
			height = r.height;
			realy = r.realy;
			realheight = r.realheight;
			usedstamp = r.usedstamp;
			psm = r.psm;
			cpsm = r.cpsm;
			clut = r.clut;
			clearminy = r.clearminy;
			clearmaxy = r.clearmaxy;
			widthmult = r.widthmult;
			channels = r.channels;
			validatecount = r.validatecount;
			fmt = r.fmt;
		}
		
		~CMemoryTarget() { Destroy(); }
		
		inline void Destroy() {
			if( ptex != NULL && ptex->ref > 0 ) {
				if( --ptex->ref <= 0 )
					delete ptex;
			}
			
			ptex = NULL;
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
		TEXTURE* ptex; // can be 16bit
		u32 fmt;
		
		int widthmult;
		int channels;
	
		int validatecount; // count how many times has been validated, if too many, destroy
		
		vector<u8> clut; // if nonzero, texture uses CLUT
		u8 psm, cpsm; // texture and clut format. For psm, only 16bit/32bit differentiation matters
	};
	
	
	struct VB
	{
		VB();
		~VB();
		
		void Destroy();
		
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
		u32 ptexClamp[2]; // textures for x and y dir region clamping
		
	public:
		void FlushTexData();
	
		// notify VB that nVerts need to be written to pbuf
		inline void NotifyWrite(int nVerts) {
			assert( pBufferData != NULL && nCount <= nNumVertices && nVerts > 0 );

			if( nCount + nVerts > nNumVertices ) {
				// recreate except with a bigger count
				VertexGPU* ptemp = (VertexGPU*)_aligned_malloc(sizeof(VertexGPU)*nNumVertices*2, 256);
				memcpy_amd(ptemp, pBufferData, sizeof(VertexGPU) * nCount);
				nNumVertices *= 2;
				assert( nCount + nVerts <= nNumVertices );
				_aligned_free(pBufferData);
				pBufferData = ptemp;
			}
		}

		void Init(int nVerts) {				
			if( pBufferData == NULL && nVerts > 0 ) {
				pBufferData = (VertexGPU*)_aligned_malloc(sizeof(VertexGPU)*nVerts, 256);
				nNumVertices = nVerts;
			}

			nCount = 0;
		}

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
		VertexGPU* pBufferData; // current allocated data

		int nNumVertices;   // size of pBufferData in terms of VertexGPU objects
		int nCount;
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
		CRenderTarget* prndr;
		CDepthTarget* pdepth;
	};

	// visible members
	extern DrawFn drawfn[8];	
	extern VB vb[2];
	extern float fiTexWidth[2], fiTexHeight[2];	// current tex width and height

	void AddMessage(const char* pstr, u32 ms = 5000);
	void DrawText(const char* pstr, int left, int top, u32 color);
	void ChangeWindowSize(int nNewWidth, int nNewHeight);
	void SetChangeDeviceSize(int nNewWidth, int nNewHeight);
	void ChangeDeviceSize(int nNewWidth, int nNewHeight);
	void SetAA(int mode);
	void SetCRC(int crc);

	void ReloadEffects();

	// Methods //
	bool IsGLExt( const char* szTargetExtension ); ///< returns true if the the opengl extension is supported
	bool Create(int width, int height);
	void Destroy(BOOL bD3D);

	void Restore(); // call to restore device
	void Reset(); // call to destroy video resources

	void GSStateReset();
	void HandleGLError();

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
	void ResetRenderTarget(int index);

	bool CheckChangeInClut(u32 highdword, u32 psm); // returns true if clut will change after this tex0 op

	// call to load CLUT data (depending on CLD)
	void texClutWrite(int ctx);
	RenderFormatType GetRenderFormat();
	GLenum GetRenderTargetFormat();

	int Save(char* pbydata);
	bool Load(char* pbydata);

	void SaveSnapshot(const char* filename);
	bool SaveRenderTarget(const char* filename, int width, int height, int jpeg);
	bool SaveTexture(const char* filename, u32 textarget, u32 tex, int width, int height);
	bool SaveJPEG(const char* filename, int width, int height, const void* pdata, int quality);
	bool SaveTGA(const char* filename, int width, int height, void* pdata);

	// private methods
	void FlushSysMem(const RECT* prc);
	void _Resolve(const void* psrc, int fbp, int fbw, int fbh, int psm, u32 fbm);

	// returns the first and last addresses aligned to a page that cover 
	void GetRectMemAddress(int& start, int& end, int psm, int x, int y, int w, int h, int bp, int bw);

	// inits the smallest rectangle in ptexMem that covers this region in ptexMem
	// returns the offset that needs to be added to the locked rect to get the beginning of the buffer
	//void GetMemRect(RECT& rc, int psm, int x, int y, int w, int h, int bp, int bw);

	// only sets a limited amount of state (for Update)
	void SetTexVariablesInt(int context, int bilinear, const tex0Info& tex0, ZeroGS::CMemoryTarget* pmemtarg, FRAGMENTSHADER* pfragment, int force);

	void ResetAlphaVariables();

	void StartCapture();
	void StopCapture();
	void CaptureFrame();
};

// GL prototypes
extern PFNGLISRENDERBUFFEREXTPROC glIsRenderbufferEXT;
extern PFNGLBINDRENDERBUFFEREXTPROC glBindRenderbufferEXT;
extern PFNGLDELETERENDERBUFFERSEXTPROC glDeleteRenderbuffersEXT;
extern PFNGLGENRENDERBUFFERSEXTPROC glGenRenderbuffersEXT;
extern PFNGLRENDERBUFFERSTORAGEEXTPROC glRenderbufferStorageEXT;
extern PFNGLGETRENDERBUFFERPARAMETERIVEXTPROC glGetRenderbufferParameterivEXT;
extern PFNGLISFRAMEBUFFEREXTPROC glIsFramebufferEXT;
extern PFNGLBINDFRAMEBUFFEREXTPROC glBindFramebufferEXT;
extern PFNGLDELETEFRAMEBUFFERSEXTPROC glDeleteFramebuffersEXT;
extern PFNGLGENFRAMEBUFFERSEXTPROC glGenFramebuffersEXT;
extern PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC glCheckFramebufferStatusEXT;
extern PFNGLFRAMEBUFFERTEXTURE1DEXTPROC glFramebufferTexture1DEXT;
extern PFNGLFRAMEBUFFERTEXTURE2DEXTPROC glFramebufferTexture2DEXT;
extern PFNGLFRAMEBUFFERTEXTURE3DEXTPROC glFramebufferTexture3DEXT;
extern PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC glFramebufferRenderbufferEXT;
extern PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVEXTPROC glGetFramebufferAttachmentParameterivEXT;
extern PFNGLGENERATEMIPMAPEXTPROC glGenerateMipmapEXT;
extern PFNGLDRAWBUFFERSPROC glDrawBuffers;

#endif
