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

#ifdef _MSC_VER
#pragma warning(disable:4200) // nonstandard extension used : zero-sized array in struct/union
#endif

// ----------------------------- Includes
#include "PS2Etypes.h"
#include "PS2Edefs.h"

// Need this before gl.h
#ifdef _WIN32

#include <windows.h>
#include <GL/gl.h>
#include <GL/glext.h>
#include "glprocs.h"

#else

// adding glew support instead of glXGetProcAddress (thanks to scaught)
#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glext.h>
#include <GL/glx.h>

inline void* wglGetProcAddress(const char* x)
{
	return (void*)glXGetProcAddress((const GLubyte*)x);
}

#endif

#include <Cg/cg.h>
#include <Cg/cgGL.h>

//#include <assert.h>

#include <list>
#include <vector>
#include <map>
#include <string>

#include "GS.h"
#include "CRC.h"
#include "rasterfont.h" // simple font
#include "ZeroGSShaders/zerogsshaders.h"

using namespace std;

// ----------------------- Defines
#ifndef GL_DEPTH24_STENCIL8_EXT // allows FBOs to support stencils
#	define GL_DEPTH_STENCIL_EXT 0x84F9
#	define GL_UNSIGNED_INT_24_8_EXT 0x84FA
#	define GL_DEPTH24_STENCIL8_EXT 0x88F0
#	define GL_TEXTURE_STENCIL_SIZE_EXT 0x88F1
#endif

#define GL_STENCILFUNC(func, ref, mask) { \
	s_stencilfunc  = func; \
	s_stencilref = ref; \
	s_stencilmask = mask; \
	glStencilFunc(func, ref, mask); \
}

#define GL_STENCILFUNC_SET() glStencilFunc(s_stencilfunc, s_stencilref, s_stencilmask)

#ifndef SAFE_DELETE
#	define SAFE_DELETE(x)		if( (x) != NULL ) { delete (x); (x) = NULL; }
#endif
#ifndef SAFE_DELETE_ARRAY
#	define SAFE_DELETE_ARRAY(x)	if( (x) != NULL ) { delete[] (x); (x) = NULL; }
#endif
#ifndef SAFE_RELEASE
#	define SAFE_RELEASE(x)		if( (x) != NULL ) { (x)->Release(); (x) = NULL; }
#endif

#define SAFE_RELEASE_PROG(x) { if( (x) != NULL ) { cgDestroyProgram(x); x = NULL; } }
#define SAFE_RELEASE_TEX(x) { if( (x) != 0 ) { glDeleteTextures(1, &(x)); x = 0; } }

#define FORIT(it, v) for(it = (v).begin(); it != (v).end(); ++(it))

// sets the data stream
#define SET_STREAM() { \
	glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(VertexGPU), (void*)8); \
	glSecondaryColorPointerEXT(4, GL_UNSIGNED_BYTE, sizeof(VertexGPU), (void*)12); \
	glTexCoordPointer(3, GL_FLOAT, sizeof(VertexGPU), (void*)16); \
	glVertexPointer(4, GL_SHORT, sizeof(VertexGPU), (void*)0); \
}

extern const char* ShaderCallerName;
extern const char* ShaderHandleName;

inline void SetShaderCaller(const char* Name)
{
	ShaderCallerName = Name;
}

inline void SetHandleName(const char* Name)
{
	ShaderHandleName = Name;
}

extern void HandleCgError(CGcontext ctx, CGerror err, void* appdata);
extern void ZZcgSetParameter4fv(CGparameter param, const float* v, const char* name);

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
#	define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#endif

//------------------------ Constants ----------------------
#define VB_BUFFERSIZE			   0x400

// Used in logaripmic Z-test, as (1-o(1))/log(MAX_U32).
const float g_filog32 = 0.999f / (32.0f * logf(2.0f));

//------------------------ Inlines -------------------------

// inline for an extremely often used sequence
// This is turning off all gl functions. Safe to do updates.
inline void DisableAllgl()
{
	glDisable(GL_SCISSOR_TEST);
	glDisable(GL_BLEND);
	glDisable(GL_ALPHA_TEST);
	glDisable(GL_DEPTH_TEST);
	glDepthMask(0);
	glDisable(GL_STENCIL_TEST);
	glColorMask(1, 1, 1, 1);
}

// Calculate maximum height for target
inline int get_maxheight(int fbp, int fbw, int psm)
{
	int ret;

	if (fbw == 0) return 0;

	ret = (((0x00100000 - 64 * fbp) / fbw) & ~0x1f);
	if (PSMT_ISHALF(psm)) ret *= 2;

	return ret;
}

// Does psm need Alpha test with alpha expansion?
inline int nNeedAlpha(u8 psm)
{
	return (psm == PSMCT24 || psm == PSMCT16 || psm == PSMCT16S);
}

// Get color storage model psm, that is important on flush stage.
inline u8 GetTexCPSM(const tex0Info& tex)
{
	if (PSMT_ISCLUT(tex.psm))
		return tex.cpsm;
	else
		return tex.psm;
}

//--------------------- Dummies

#ifdef _WIN32
extern void (__stdcall *zgsBlendEquationSeparateEXT)(GLenum, GLenum);
extern void (__stdcall *zgsBlendFuncSeparateEXT)(GLenum, GLenum, GLenum, GLenum);
#else
extern void (APIENTRY *zgsBlendEquationSeparateEXT)(GLenum, GLenum);
extern void (APIENTRY *zgsBlendFuncSeparateEXT)(GLenum, GLenum, GLenum, GLenum);
#endif

// ------------------------ Types -------------------------

struct FRAGMENTSHADER
{
	FRAGMENTSHADER() : prog(0), sMemory(0), sFinal(0), sBitwiseANDX(0), sBitwiseANDY(0), sInterlace(0), sCLUT(0), sOneColor(0), sBitBltZ(0),
			fTexAlpha2(0), fTexOffset(0), fTexDims(0), fTexBlock(0), fClampExts(0), fTexWrapMode(0),
			fRealTexDims(0), fTestBlack(0), fPageOffset(0), fTexAlpha(0) {}

	CGprogram prog;
	CGparameter sMemory, sFinal, sBitwiseANDX, sBitwiseANDY, sInterlace, sCLUT;
	CGparameter sOneColor, sBitBltZ, sInvTexDims;
	CGparameter fTexAlpha2, fTexOffset, fTexDims, fTexBlock, fClampExts, fTexWrapMode, fRealTexDims, fTestBlack, fPageOffset, fTexAlpha;

#ifdef _DEBUG
	string filename;
#endif
	void set_uniform_param(CGparameter &var, const char *name)
	{
		CGparameter p;
		p = cgGetNamedParameter(prog, name);

		if (p != NULL && cgIsParameterUsed(p, prog) == CG_TRUE) var = p;
	}

	bool set_texture(GLuint texobj, const char *name)
	{
		CGparameter p;

		p = cgGetNamedParameter(prog, name);

		if (p != NULL && cgIsParameterUsed(p, prog) == CG_TRUE)
		{
			cgGLSetTextureParameter(p, texobj);
			cgGLEnableTextureParameter(p);
			return true;
		}

		return false;
	}

	bool connect(CGparameter &tex, const char *name)
	{
		CGparameter p;

		p = cgGetNamedParameter(prog, name);

		if (p != NULL && cgIsParameterUsed(p, prog) == CG_TRUE)
		{
			cgConnectParameter(tex, p);
			return true;
		}

		return false;
	}

	bool set_texture(CGparameter &tex, const char *name)
	{
		CGparameter p;

		p = cgGetNamedParameter(prog, name);

		if (p != NULL && cgIsParameterUsed(p, prog) == CG_TRUE)
		{
			//cgGLEnableTextureParameter(p);
			tex = p;
			return true;
		}

		return false;
	}

	bool set_shader_const(Vector v, const char *name)
	{
		CGparameter p;

		p = cgGetNamedParameter(prog, name);

		if (p != NULL && cgIsParameterUsed(p, prog) == CG_TRUE)
		{
			cgGLSetParameter4fv(p, v);
			return true;
		}

		return false;
	}
};

struct VERTEXSHADER
{
	VERTEXSHADER() : prog(0), sBitBltPos(0), sBitBltTex(0) {}

	CGprogram prog;
	CGparameter sBitBltPos, sBitBltTex, fBitBltTrans;		 // vertex shader constants
};

// ------------------------ Variables -------------------------
// all textures have this width
//#define GPU_TEXWIDTH		512
extern int GPU_TEXWIDTH;
extern float g_fiGPU_TEXWIDTH;
#define MASKDIVISOR		0
#define GPU_TEXMASKWIDTH	(1024 >> MASKDIVISOR) // bitwise mask width for region repeat mode

#ifdef _WIN32
extern HDC		hDC;	   // Private GDI Device Context
extern HGLRC	hRC;	   // Permanent Rendering Context
#endif

extern bool g_bIsLost;     // Context is lost -- could not draw

extern u32 ptexBlocks;		// holds information on block tiling
extern u32 ptexConv16to32;
extern u32 ptexBilinearBlocks;
extern u32 ptexConv32to16;
extern int g_MaxTexWidth, g_MaxTexHeight;
extern float g_fBlockMult;
extern bool g_bDisplayMsg;
extern bool g_bDisplayFPS;

extern int g_nDepthBias;
extern bool g_bSaveFlushedFrame;
extern u8* s_lpShaderResources;
extern u32 g_SaveFrameNum;
extern int s_nWriteDepthCount;
extern int s_nWireframeCount;
extern int s_nWriteDestAlphaTest;

//////////////////////////
// State parameters
extern float fiRendWidth, fiRendHeight;
extern int g_PrevBitwiseTexX, g_PrevBitwiseTexY; // textures stored in SAMP_BITWISEANDX and SAMP_BITWISEANDY
extern bool s_bDestAlphaTest;
extern int s_ClutResolve;
extern int s_nLastResolveReset;
extern int g_nDepthUpdateCount;
extern int s_nResolveCounts[30]; // resolve counts for last 30 frames
extern int g_nDepthUsed; // ffx2 pal movies

/////////////////////
// graphics resources
extern map<string, GLbyte> mapGLExtensions;
extern map<int, SHADERHEADER*> mapShaderResources;

#ifdef DEVBUILD
extern char* EFFECT_NAME;
extern char* EFFECT_DIR;
extern u32 g_nGenVars, g_nTexVars, g_nAlphaVars, g_nResolve;
extern bool g_bSaveTrans, g_bUpdateEffect, g_bSaveTex, g_bSaveResolved;

#endif

extern RasterFont* font_p;
extern u32 s_uFramebuffer;
extern CGprofile cgvProf, cgfProf;
extern int g_nPixelShaderVer;
extern CGprogram pvs[16];
extern FRAGMENTSHADER ppsRegular[4], ppsTexture[NUM_SHADERS];
extern FRAGMENTSHADER ppsCRTC[2], ppsCRTC24[2], ppsCRTCTarg[2];
extern GLenum s_srcrgb, s_dstrgb, s_srcalpha, s_dstalpha; // set by zgsBlendFuncSeparateEXT
extern u32 s_stencilfunc, s_stencilref, s_stencilmask;
extern GLenum s_drawbuffers[];
extern bool s_bTexFlush;
extern bool s_bWriteDepth;

extern int maxmin;
extern const GLenum primtype[8];
extern u32 ptexLogo;
extern int nLogoWidth, nLogoHeight;
extern u32 s_ptexInterlace;		 // holds interlace fields
extern int s_nFullscreen;

extern vector<u32> s_vecTempTextures;		   // temporary textures, released at the end of every frame
// global alpha blending settings
extern GLenum g_internalFloatFmt;
extern GLenum g_internalRGBAFloatFmt;
extern GLenum g_internalRGBAFloat16Fmt;

extern CGprogram g_vsprog, g_psprog;
extern bool g_bMakeSnapshot;
extern string strSnapshot;
extern bool g_bCRTCBilinear;

// AVI Capture
extern int s_aviinit;
extern int s_avicapturing;

extern int nBackbufferWidth, nBackbufferHeight;
extern u8* g_pbyGSMemory;
extern u8* g_pbyGSClut; // the temporary clut buffer
extern CGparameter g_vparamPosXY[2], g_fparamFogColor;

namespace ZeroGS
{

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
		inline void CreateFeedback()
		{
			if (ptexFeedback == 0 || !(status&TS_FeedbackReady))
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

		int fbp, fbw, fbh, fbhCalc; // if fbp is negative, virtual target (not mapped to any real addr)
		int start, end; // in bytes
		u32 lastused;	// time stamp since last used
		Vector vposxy;

		u32 fbm;
		u16 status;
		u8 psm;
		u8 resv0;
		Rect scissorrect;

		u8 created;	// Check for object destruction/creating for r201.

		//int startresolve, endresolve;
		u32 nUpdateTarg; // use this target to update the texture if non 0 (one time only)

		// this is optionally used when feedback effects are used (render target is used as a texture when rendering to itself)
		u32 ptexFeedback;

		enum TargetStatus
		{
			TS_Resolved = 1,
			TS_NeedUpdate = 2,
			TS_Virtual = 4, // currently not mapped to memory
			TS_FeedbackReady = 8, // feedback effect is ready and doesn't need to be updated
			TS_NeedConvert32 = 16,
			TS_NeedConvert16 = 32,
		};
		inline Vector DefaultBitBltPos() ;
		inline Vector DefaultBitBltTex() ;

	private:
		void _CreateFeedback();
		inline bool InitialiseDefaultTexture(u32 *p_ptr, int fbw, int fbh) ;
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
			u8* memptr;  // GPU memory used for comparison
			int ref;
		};

		inline CMemoryTarget() : ptex(NULL), starty(0), height(0), realy(0), realheight(0), usedstamp(0), psm(0), cpsm(0), channels(0), clearminy(0), clearmaxy(0), validatecount(0) {}

		inline CMemoryTarget(const CMemoryTarget& r)
		{
			ptex = r.ptex;

			if (ptex != NULL) ptex->ref++;

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

		inline void Destroy()
		{
			if (ptex != NULL && ptex->ref > 0)
			{
				if (--ptex->ref <= 0) delete ptex;
			}

			ptex = NULL;
		}

		// returns true if clut data is synced
		bool ValidateClut(const tex0Info& tex0);
		// returns true if tex data is synced
		bool ValidateTex(const tex0Info& tex0, int starttex, int endtex, bool bDeleteBadTex);

		// realy is offset in pixels from start of valid region
		// so texture in memory is [realy,starty+height]
		// valid texture is [starty,starty+height]
		// offset in mem [starty-realy, height]
		TEXTURE* ptex; // can be 16bit

		int starty, height; // assert(starty >= realy)
		int realy, realheight; // this is never touched once allocated
		u32 usedstamp;
		u8 psm, cpsm; // texture and clut format. For psm, only 16bit/32bit differentiation matters

		u32 fmt;

		int widthmult;
		int channels;
		int clearminy, clearmaxy; // when maxy > 0, need to check for clearing

		int validatecount; // count how many times has been validated, if too many, destroy

		vector<u8> clut; // if nonzero, texture uses CLUT
};


struct VB
{
	VB();
	~VB();

	void Destroy();

	inline bool CheckPrim()
	{
		static const int PRIMMASK = 0x0e;   // for now ignore 0x10 (AA)

		if ((PRIMMASK & prim->_val) != (PRIMMASK & curprim._val) || primtype[prim->prim] != primtype[curprim.prim])
			return nCount > 0;

		return false;
	}

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
	inline int CheckFrameAddConstraints(int tbp);
	inline void CheckScissors(int maxpos);
	inline void CheckFrame32bitRes(int maxpos);
	inline int FindMinimalMemoryConstrain(int tbp, int maxpos);
	inline int FindZbufferMemoryConstrain(int tbp, int maxpos);
	inline int FindMinimalHeightConstrain(int maxpos);

	inline int CheckFrameResolveRender(int tbp);
	inline void CheckFrame16vs32Conversion();
	inline int CheckFrameResolveDepth(int tbp);

	inline void FlushTexUnchangedClutDontUpdate() ;
	inline void FlushTexClutDontUpdate() ;
	inline void FlushTexClutting() ;
	inline void FlushTexSetNewVars(u32 psm) ;

	// notify VB that nVerts need to be written to pbuf
	inline void NotifyWrite(int nVerts)
	{
		assert(pBufferData != NULL && nCount <= nNumVertices && nVerts > 0);

		if (nCount + nVerts > nNumVertices)
		{
			// recreate except with a bigger count
			VertexGPU* ptemp = (VertexGPU*)_aligned_malloc(sizeof(VertexGPU) * nNumVertices * 2, 256);
			memcpy_amd(ptemp, pBufferData, sizeof(VertexGPU) * nCount);
			nNumVertices *= 2;
			assert(nCount + nVerts <= nNumVertices);
			_aligned_free(pBufferData);
			pBufferData = ptemp;
		}
	}

	void Init(int nVerts)
	{
		if (pBufferData == NULL && nVerts > 0)
		{
			pBufferData = (VertexGPU*)_aligned_malloc(sizeof(VertexGPU) * nVerts, 256);
			nNumVertices = nVerts;
		}

		nCount = 0;
	}

	u8 bNeedFrameCheck;
	u8 bNeedZCheck;
	u8 bNeedTexCheck;
	u8 dummy0;

	union
	{
		struct
		{
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

// Return, if tcc, aem or psm mode told us, than Alpha test should be used
// if tcc == 0 than no alpha used, aem used for alpha expanding and I am not sure
// that it's correct, psm -- color mode,
inline bool
IsAlphaTestExpansion(VB& curvb)
{
	return (curvb.tex0.tcc && gs.texa.aem && nNeedAlpha(GetTexCPSM(curvb.tex0)));
}

// visible members
extern DrawFn drawfn[8];

// VB variables
extern VB vb[2];
extern float fiTexWidth[2], fiTexHeight[2];	// current tex width and height
extern vector<GLuint> g_vboBuffers; // VBOs for all drawing commands
extern GLuint vboRect;
extern int g_nCurVBOIndex;

// Shaders variables
extern Vector g_vdepth;
extern Vector vlogz;
extern VERTEXSHADER pvsBitBlt;
extern FRAGMENTSHADER ppsBitBlt[2], ppsBitBltDepth, ppsOne;
extern FRAGMENTSHADER ppsBaseTexture, ppsConvert16to32, ppsConvert32to16;
bool LoadEffects();
bool LoadExtraEffects();
FRAGMENTSHADER* LoadShadeEffect(int type, int texfilter, int fog, int testaem, int exactcolor, const clampInfo& clamp, int context, bool* pbFailed);

extern RenderFormatType g_RenderFormatType;

void AddMessage(const char* pstr, u32 ms = 5000);
void DrawText(const char* pstr, int left, int top, u32 color);
void ChangeWindowSize(int nNewWidth, int nNewHeight);
void SetChangeDeviceSize(int nNewWidth, int nNewHeight);
void ChangeDeviceSize(int nNewWidth, int nNewHeight);
void SetAA(int mode);
void SetNegAA(int mode);
void SetCRC(int crc);

void ReloadEffects();

// Methods //
bool IsGLExt(const char* szTargetExtension);   ///< returns true if the the opengl extension is supported
inline bool Create_Window(int _width, int _height);
bool Create(int width, int height);
void Destroy(bool bD3D);

void Restore(); // call to restore device
void Reset(); // call to destroy video resources

void GSStateReset();
void HandleGLError();

// called on a primitive switch
void Prim();

void SetTexFlush();
// flush current vertices, call before setting new registers (the main render method)
void Flush(int context);
void FlushBoth();
void ExtWrite();

void SetWriteDepth();
bool IsWriteDepth();

void SetDestAlphaTest();
bool IsWriteDestAlphaTest();

void SetFogColor(u32 fog);
void SaveTex(tex0Info* ptex, int usevid);
char* NamedSaveTex(tex0Info* ptex, int usevid);

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

int Save(s8* pbydata);
bool Load(s8* pbydata);

void SaveSnapshot(const char* filename);
bool SaveRenderTarget(const char* filename, int width, int height, int jpeg);
bool SaveTexture(const char* filename, u32 textarget, u32 tex, int width, int height);
bool SaveJPEG(const char* filename, int width, int height, const void* pdata, int quality);
bool SaveTGA(const char* filename, int width, int height, void* pdata);
void Stop_Avi();

// private methods
void FlushSysMem(const RECT* prc);
void _Resolve(const void* psrc, int fbp, int fbw, int fbh, int psm, u32 fbm, bool mode);

// returns the first and last addresses aligned to a page that cover
void GetRectMemAddress(int& start, int& end, int psm, int x, int y, int w, int h, int bp, int bw);

// inits the smallest rectangle in ptexMem that covers this region in ptexMem
// returns the offset that needs to be added to the locked rect to get the beginning of the buffer
//void GetMemRect(RECT& rc, int psm, int x, int y, int w, int h, int bp, int bw);

void SetContextTarget(int context) ;

void NeedFactor(int w);
// only sets a limited amount of state (for Update)
void SetTexClamping(int context, FRAGMENTSHADER* pfragment);
void SetTexVariablesInt(int context, int bilinear, const tex0Info& tex0, ZeroGS::CMemoryTarget* pmemtarg, FRAGMENTSHADER* pfragment, int force);

void ResetAlphaVariables();

void StartCapture();
void StopCapture();
void CaptureFrame();

// Perform clutting for flushed texture. Better check if it needs a prior call.
inline void CluttingForFlushedTex(tex0Info* tex0, u32 Data, int ictx)
{
	tex0->cbp  = ZZOglGet_cbp_TexBits(Data);
	tex0->cpsm = ZZOglGet_cpsm_TexBits(Data);
	tex0->csm  = ZZOglGet_csm_TexBits(Data);
	tex0->csa  = ZZOglGet_csa_TexBits(Data);
	tex0->cld  = ZZOglGet_cld_TexBits(Data);

	ZeroGS::texClutWrite(ictx);
}
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
