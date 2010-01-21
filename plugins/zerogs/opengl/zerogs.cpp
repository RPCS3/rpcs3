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

#if defined(_WIN32)
#include <windows.h>
#include <aviUtil.h>

#include "resource.h"
#endif

#include <stdio.h>

#include <malloc.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "GS.h"
#include "Mem.h"
#include "x86.h"
#include "zerogs.h"
#include "zpipe.h"

#include "ZeroGSShaders/zerogsshaders.h"
#include "targets.h"
#include "rasterfont.h" // simple font

#define VB_BUFFERSIZE			   0x400
#define VB_NUMBUFFERS			   512
#define SIZEOF_VB		   sizeof(ZeroGS::VB)//((u32)((u8*)&vb[0].buffers-(u8*)&vb[0]))

#define MINMAX_SHIFT	3
#define MAX_ACTIVECLUTS				 16

#define ZEROGS_SAVEVER 0xaa000005

#define STENCIL_ALPHABIT	1	   // if set, dest alpha >= 0x80
#define STENCIL_PIXELWRITE  2	   // if set, pixel just written (reset after every Flush)
#define STENCIL_FBA		 4	   // if set, just written pixel's alpha >= 0 (reset after every Flush)
#define STENCIL_SPECIAL	 8	   // if set, indicates that pixel passed its alpha test (reset after every Flush)
//#define STENCIL_PBE		   16
#define STENCIL_CLEAR	   (2|4|8|16)

#define VBSAVELIMIT ((u32)((u8*)&vb[0].nNextFrameHeight-(u8*)&vb[0]))

using namespace ZeroGS;

extern u32 g_nGenVars, g_nTexVars, g_nAlphaVars, g_nResolve;
extern char *libraryName;
extern int g_nFrame, g_nRealFrame;
extern float fFPS;
extern unsigned char zgsrevision, zgsbuild, zgsminor;

BOOL g_bDisplayMsg = 1;

#ifdef _WIN32
HDC		 hDC=NULL;	   // Private GDI Device Context
HGLRC	   hRC=NULL;	   // Permanent Rendering Context
#endif

BOOL g_bCRTCBilinear = TRUE;
BOOL g_bSaveFlushedFrame = 0;
BOOL g_bIsLost = 0;
int g_nFrameRender = 10;
int g_nFramesSkipped = 0;

#ifndef ZEROGS_DEVBUILD

#define INC_GENVARS()
#define INC_TEXVARS()
#define INC_ALPHAVARS()
#define INC_RESOLVE()

#define g_bUpdateEffect 0
#define g_bSaveTex 0
#define g_bSaveTrans 0
#define g_bSaveFrame 0
#define g_bSaveFinalFrame 0
#define g_bSaveResolved 0

#else

#define INC_GENVARS() ++g_nGenVars
#define INC_TEXVARS() ++g_nTexVars
#define INC_ALPHAVARS() ++g_nAlphaVars
#define INC_RESOLVE() ++g_nResolve

BOOL g_bSaveTrans = 0;
BOOL g_bUpdateEffect = 0;
BOOL g_bSaveTex = 0;	// saves the curent texture
BOOL g_bSaveFrame = 0;  // saves the current psurfTarget
BOOL g_bSaveFinalFrame = 0; // saves the input to the CRTC
BOOL g_bSaveResolved = 0;

#ifdef _WIN32
//#define EFFECT_NAME "f:\\ps2dev\\svn\\pcsx2\\ZeroGS\\opengl\\"
char* EFFECT_DIR = "C:\\programming\\ps2dev\\zerogs\\opengl\\";
char* EFFECT_NAME = "C:\\programming\\ps2dev\\zerogs\\opengl\\ps2hw.fx";
#else
char EFFECT_DIR[255] = "~/pcsx2/plugins/gs/zerogs/opengl/";
char EFFECT_NAME[255] = "~/pcsx2/plugins/gs/zerogs/opengl/ps2hw.fx";
#endif

#endif

BOOL g_bUpdateStencil = 1;	  // only needed for dest alpha test (unfortunately, it has to be on all the time)	

#define DRAW() glDrawArrays(primtype[curvb.curprim.prim], 0, curvb.nCount)

#define GL_BLEND_RGB(src, dst) { \
	s_srcrgb = src; \
	s_dstrgb = dst; \
	zgsBlendFuncSeparateEXT(s_srcrgb, s_dstrgb, s_srcalpha, s_dstalpha); \
}

#define GL_BLEND_ALPHA(src, dst) { \
	s_srcalpha = src; \
	s_dstalpha = dst; \
	zgsBlendFuncSeparateEXT(s_srcrgb, s_dstrgb, s_srcalpha, s_dstalpha); \
}

#define GL_BLEND_ALL(srcrgb, dstrgb, srcalpha, dstalpha) { \
	s_srcrgb = srcrgb; \
	s_dstrgb = dstrgb; \
	s_srcalpha = srcalpha; \
	s_dstalpha = dstalpha; \
	zgsBlendFuncSeparateEXT(s_srcrgb, s_dstrgb, s_srcalpha, s_dstalpha); \
}

#define GL_BLEND_SET() zgsBlendFuncSeparateEXT(s_srcrgb, s_dstrgb, s_srcalpha, s_dstalpha)

#define GL_ZTEST(enable) { \
	if( enable ) glEnable(GL_DEPTH_TEST); \
	else glDisable(GL_DEPTH_TEST); \
}

#define GL_ALPHATEST(enable) { \
	if( enable ) glEnable(GL_ALPHA_TEST); \
	else glDisable(GL_ALPHA_TEST); \
}

#define GL_BLENDEQ_RGB(eq) { \
	s_rgbeq = eq; \
	zgsBlendEquationSeparateEXT(s_rgbeq, s_alphaeq); \
}

#define GL_BLENDEQ_ALPHA(eq) { \
	s_alphaeq = eq; \
	zgsBlendEquationSeparateEXT(s_rgbeq, s_alphaeq); \
}

#define GL_STENCILFUNC(func, ref, mask) { \
	s_stencilfunc  = func; \
	s_stencilref = ref; \
	s_stencilmask = mask; \
	glStencilFunc(func, ref, mask); \
}

#define GL_STENCILFUNC_SET() glStencilFunc(s_stencilfunc, s_stencilref, s_stencilmask)

#define COLORMASK_RED	   1
#define COLORMASK_GREEN	 2
#define COLORMASK_BLUE	  4
#define COLORMASK_ALPHA	 8
#define GL_COLORMASK(mask) glColorMask(!!((mask)&COLORMASK_RED), !!((mask)&COLORMASK_GREEN), !!((mask)&COLORMASK_BLUE), !!((mask)&COLORMASK_ALPHA))

typedef void (APIENTRYP _PFNSWAPINTERVAL) (int);

extern int s_frameskipping;

static u32 g_SaveFrameNum = 0;
BOOL g_bMakeSnapshot = 0;
string strSnapshot;

int GPU_TEXWIDTH = 512;
float g_fiGPU_TEXWIDTH = 1/512.0f;

int g_MaxTexWidth = 4096, g_MaxTexHeight = 4096;
CGprogram g_vsprog = 0, g_psprog = 0;
// AVI Capture
static int s_aviinit = 0;
static int s_avicapturing = 0;

inline u32 FtoDW(float f) { return (*((u32*)&f)); }

float g_fBlockMult = 1;
static int s_nFullscreen = 0;
int g_nDepthUpdateCount = 0;
int g_nDepthBias = 0;

// local alpha blending settings
static GLenum s_srcrgb, s_dstrgb, s_srcalpha, s_dstalpha; // set by zgsBlendFuncSeparateEXT
static GLenum s_rgbeq, s_alphaeq; // set by zgsBlendEquationSeparateEXT
static u32 s_stencilfunc, s_stencilref, s_stencilmask;
static GLenum s_drawbuffers[] = { GL_COLOR_ATTACHMENT0_EXT, GL_COLOR_ATTACHMENT1_EXT };

#ifdef _WIN32
extern HINSTANCE hInst;

void (__stdcall *zgsBlendEquationSeparateEXT)(GLenum, GLenum) = NULL;
void (__stdcall *zgsBlendFuncSeparateEXT)(GLenum, GLenum, GLenum, GLenum) = NULL;
#else
void (APIENTRY *zgsBlendEquationSeparateEXT)(GLenum, GLenum) = NULL;
void (APIENTRY *zgsBlendFuncSeparateEXT)(GLenum, GLenum, GLenum, GLenum) = NULL;
#endif

GLenum g_internalFloatFmt = GL_ALPHA_FLOAT32_ATI;
GLenum g_internalRGBAFloatFmt = GL_RGBA_FLOAT32_ATI;
GLenum g_internalRGBAFloat16Fmt = GL_RGBA_FLOAT16_ATI;

// Consts
static const GLenum primtype[8] = { GL_POINTS, GL_LINES, GL_LINES, GL_TRIANGLES, GL_TRIANGLES, GL_TRIANGLES, GL_TRIANGLES, 0xffffffff };
static const u32 blendalpha[3] = { GL_SRC_ALPHA, GL_DST_ALPHA, GL_CONSTANT_COLOR_EXT };
static const u32 blendinvalpha[3] = { GL_ONE_MINUS_SRC_ALPHA, GL_ONE_MINUS_DST_ALPHA, GL_ONE_MINUS_CONSTANT_COLOR_EXT };

static const int PRIMMASK = 0x0e;   // for now ignore 0x10 (AA)

static const u32 g_dwAlphaCmp[] = { GL_NEVER, GL_ALWAYS, GL_LESS, GL_LEQUAL, GL_EQUAL, GL_GEQUAL, GL_GREATER, GL_NOTEQUAL };

// used for afail case
static const u32 g_dwReverseAlphaCmp[] = { GL_ALWAYS, GL_NEVER, GL_GEQUAL, GL_GREATER, GL_NOTEQUAL, GL_LESS, GL_LEQUAL, GL_EQUAL };

static const u32 g_dwZCmp[] = { GL_NEVER, GL_ALWAYS, GL_GEQUAL, GL_GREATER };

PFNGLISRENDERBUFFEREXTPROC glIsRenderbufferEXT = NULL;
PFNGLBINDRENDERBUFFEREXTPROC glBindRenderbufferEXT = NULL;
PFNGLDELETERENDERBUFFERSEXTPROC glDeleteRenderbuffersEXT = NULL;
PFNGLGENRENDERBUFFERSEXTPROC glGenRenderbuffersEXT = NULL;
PFNGLRENDERBUFFERSTORAGEEXTPROC glRenderbufferStorageEXT = NULL;
PFNGLGETRENDERBUFFERPARAMETERIVEXTPROC glGetRenderbufferParameterivEXT = NULL;
PFNGLISFRAMEBUFFEREXTPROC glIsFramebufferEXT = NULL;
PFNGLBINDFRAMEBUFFEREXTPROC glBindFramebufferEXT = NULL;
PFNGLDELETEFRAMEBUFFERSEXTPROC glDeleteFramebuffersEXT = NULL;
PFNGLGENFRAMEBUFFERSEXTPROC glGenFramebuffersEXT = NULL;
PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC glCheckFramebufferStatusEXT = NULL;
PFNGLFRAMEBUFFERTEXTURE1DEXTPROC glFramebufferTexture1DEXT = NULL;
PFNGLFRAMEBUFFERTEXTURE2DEXTPROC glFramebufferTexture2DEXT = NULL;
PFNGLFRAMEBUFFERTEXTURE3DEXTPROC glFramebufferTexture3DEXT = NULL;
PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC glFramebufferRenderbufferEXT = NULL;
PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVEXTPROC glGetFramebufferAttachmentParameterivEXT = NULL;
PFNGLGENERATEMIPMAPEXTPROC glGenerateMipmapEXT = NULL;
PFNGLDRAWBUFFERSPROC glDrawBuffers = NULL;

/////////////////////
// graphics resources
static map<string, GLbyte> mapGLExtensions;
RasterFont* font_p = NULL;
CGprofile cgvProf, cgfProf;
static CGprogram pvs[16] = {NULL};
static FRAGMENTSHADER ppsRegular[4], ppsTexture[NUM_SHADERS];
static FRAGMENTSHADER ppsCRTC[2], ppsCRTC24[2], ppsCRTCTarg[2];
CGparameter g_vparamPosXY[2] = {0}, g_fparamFogColor = 0;

int g_nPixelShaderVer = 0; // default

static u8* s_lpShaderResources = NULL;
static map<int, SHADERHEADER*> mapShaderResources;

u32 s_uFramebuffer = 0;
u32 s_ptexCurSet[2] = {0};

#define s_bForceTexFlush 1
static u32 s_ptexNextSet[2] = {0};

u32 ptexBlocks = 0, ptexConv16to32 = 0;	 // holds information on block tiling
u32 ptexBilinearBlocks = 0;
u32 ptexConv32to16 = 0;
static u32 s_ptexInterlace = 0;		 // holds interlace fields
static int s_nInterlaceTexWidth = 0;					// width of texture
static vector<u32> s_vecTempTextures;		   // temporary textures, released at the end of every frame

static BOOL s_bTexFlush = FALSE;
static u32 ptexLogo = 0;
static int nLogoWidth, nLogoHeight;
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

static Vector vAlphaBlendColor;	 // used for GPU_COLOR

static u8 bNeedBlendFactorInAlpha;	  // set if the output source alpha is different from the real source alpha (only when blend factor > 0x80)
static u32 s_dwColorWrite = 0xf;			// the color write mask of the current target

BOOL g_bDisplayFPS = FALSE;

union {
	struct {
		u8 _bNeedAlphaColor;		// set if vAlphaBlendColor needs to be set
		u8 _b2XAlphaTest;		   // Only valid when bNeedAlphaColor is set. if 1st bit set set, double all alpha testing values
									// otherwise alpha testing needs to be done separately.
		u8 _bDestAlphaColor;		// set to 1 if blending with dest color (process only one tri at a time). If 2, dest alpha is always 1.
		u8 _bAlphaClamping;	 // if first bit is set, do min; if second bit, do max
	};
	u32 _bAlphaState;
} g_vars;

//#define bNeedAlphaColor g_vars._bNeedAlphaColor
#define b2XAlphaTest g_vars._b2XAlphaTest
#define bDestAlphaColor g_vars._bDestAlphaColor
#define bAlphaClamping g_vars._bAlphaClamping

int g_PrevBitwiseTexX = -1, g_PrevBitwiseTexY = -1; // textures stored in SAMP_BITWISEANDX and SAMP_BITWISEANDY

// stores the buffers for the last RenderCRTC
const float g_filog32 = 0.999f / (32.0f * logf(2.0f));

static alphaInfo s_alphaInfo;

CGcontext g_cgcontext;
static int nBackbufferWidth, nBackbufferHeight;

u8* g_pbyGSMemory = NULL;   // 4Mb GS system mem
u8* g_pbyGSClut = NULL;

namespace ZeroGS
{
	VB vb[2];
	float fiTexWidth[2], fiTexHeight[2];	// current tex width and height

	GLuint vboRect = 0;
	vector<GLuint> g_vboBuffers; // VBOs for all drawing commands
	int g_nCurVBOIndex = 0;

	u8 s_AAx = 0, s_AAy = 0; // if AAy is set, then AAx has to be set
	RenderFormatType g_RenderFormatType = RFT_float16;
	int icurctx = -1;

	Vector g_vdepth = Vector(256.0f*65536.0f, 65536.0f, 256.0f, 65536.0f*65536.0f);

	VERTEXSHADER pvsBitBlt;
	FRAGMENTSHADER ppsBitBlt[2], ppsBitBltDepth, ppsOne;
	FRAGMENTSHADER ppsBaseTexture, ppsConvert16to32, ppsConvert32to16;

	extern CRangeManager s_RangeMngr; // manages overwritten memory
	void FlushTransferRanges(const tex0Info* ptex);

	RenderFormatType GetRenderFormat() { return g_RenderFormatType; }
	GLenum GetRenderTargetFormat() { return GetRenderFormat()==RFT_byte8?4:g_internalRGBAFloat16Fmt; }

	// returns the first and last addresses aligned to a page that cover 
	void GetRectMemAddress(int& start, int& end, int psm, int x, int y, int w, int h, int bp, int bw);

	bool LoadEffects();
	bool LoadExtraEffects();
	FRAGMENTSHADER* LoadShadeEffect(int type, int texfilter, int fog, int testaem, int exactcolor, const clampInfo& clamp, int context, bool* pbFailed);

	static int s_nNewWidth = -1, s_nNewHeight = -1;
	void ChangeDeviceSize(int nNewWidth, int nNewHeight);

	void ProcessMessages();
	void RenderCustom(float fAlpha); // intro anim

	struct MESSAGE
	{
		MESSAGE() {}
		MESSAGE(const char* p, u32 dw) { strcpy(str, p); dwTimeStamp = dw; }
		char str[255];
		u32 dwTimeStamp;
	};

	static list<MESSAGE> listMsgs;

	///////////////////////
	// Method Prototypes //
	///////////////////////

	void AdjustTransToAspect(Vector& v, int dispwidth, int dispheight);

	void KickPoint();
	void KickLine();
	void KickTriangle();
	void KickTriangleFan();
	void KickSprite();
	void KickDummy();

	__forceinline void SetContextTarget(int context);

	// use to update the state
	void SetTexVariables(int context, FRAGMENTSHADER* pfragment, int settexint);
	void SetAlphaVariables(const alphaInfo& ainfo);
	void ResetAlphaVariables();

	__forceinline void SetAlphaTestInt(pixTest curtest);

	__forceinline void RenderAlphaTest(const VB& curvb, CGparameter sOneColor);
	__forceinline void RenderStencil(const VB& curvb, u32 dwUsingSpecialTesting);
	__forceinline void ProcessStencil(const VB& curvb);
	__forceinline void RenderFBA(const VB& curvb, CGparameter sOneColor);
	__forceinline void ProcessFBA(const VB& curvb, CGparameter sOneColor);

	void ResolveInRange(int start, int end);

	void ExtWrite();

	__forceinline u32 CreateInterlaceTex(int width) {
		if( width == s_nInterlaceTexWidth && s_ptexInterlace != 0 ) return s_ptexInterlace;

		SAFE_RELEASE_TEX(s_ptexInterlace);
		s_nInterlaceTexWidth = width;

		vector<u32> data(width);
		for(int i = 0; i < width; ++i) data[i] = (i&1) ? 0xffffffff : 0;

		glGenTextures(1, &s_ptexInterlace);
		glBindTexture(GL_TEXTURE_RECTANGLE_NV, s_ptexInterlace);
		glTexImage2D(GL_TEXTURE_RECTANGLE_NV, 0, 4, width, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, &data[0]);
		glTexParameteri(GL_TEXTURE_RECTANGLE_NV, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_RECTANGLE_NV, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		GL_REPORT_ERRORD();
		return s_ptexInterlace;
	}

	void ResetRenderTarget(int index) {
		glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT+index, GL_TEXTURE_RECTANGLE_NV, 0, 0 );
	}

	DrawFn drawfn[8] = { KickDummy, KickDummy, KickDummy, KickDummy,
						KickDummy, KickDummy, KickDummy, KickDummy };

}; // end namespace

///////////////////
// Context State //
///////////////////
ZeroGS::VB::VB()
{
	memset(this, 0, SIZEOF_VB);
	tex0.tw = 1;
	tex0.th = 1;
}

ZeroGS::VB::~VB()
{
	Destroy();
}

void ZeroGS::VB::Destroy()
{
	_aligned_free(pBufferData); pBufferData = NULL; nNumVertices = 0;
	
	prndr = NULL;
	pdepth = NULL;
}

bool ZeroGS::VB::CheckPrim()
{
	if( (PRIMMASK & prim->_val) != (PRIMMASK & curprim._val) || primtype[prim->prim] != primtype[curprim.prim] )
		return nCount > 0;

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

	bChanged = 0;

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
			if( gsfb.psm & 2 ) maxpos *= 2;;

			maxpos = min(gsfb.fbh, maxpos);
			maxpos = min(maxmin, maxpos);
			//? atelier iris crashes without it
			if( maxpos > 256 ) maxpos &= ~0x1f;
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
//		if (frame.fbw > 1024) frame.fbw = 1024;

//	  if( fbh > 256 && (fbh % m_Blocks[gsfb.psm].height) <= 2 ) {
//		  // dragon ball z
//		  fbh -= fbh%m_Blocks[gsfb.psm].height;
//	  }
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
		/*if( g_MaxRenderedHeight >= 0xe0 && frame.fbp >= 0x2000 ) {
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
		}*/
		
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
				s_RTs.DestroyAllTargs(0, 0x100, pnewtarg->fbw);
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
		tempfb.fbh = prndr->fbh;
		if( zbuf.psm == 0x31 ) 
			tempfb.fbm = 0xff000000;
		else 
			tempfb.fbm = 0;

		// check if there is a target that exactly aligns with zbuf (zbuf can be cleared this way, gunbird 2)
		//u32 key = zbuf.zbp|(frame.fbw<<16);
		//CRenderTargetMngr::MAPTARGETS::iterator it = s_RTs.mapTargets.find(key);
//	  if( it != s_RTs.mapTargets.end() ) {
//#ifdef _DEBUG
//		  DEBUG_LOG("zbuf resolve\n");
//#endif
//		  if( it->second->status & CRenderTarget::TS_Resolved )
//			  it->second->Resolve();
//	  }

		GL_REPORT_ERRORD();
		CDepthTarget* pnewdepth = (CDepthTarget*)s_DepthRTs.GetTarg(tempfb, CRenderTargetMngr::TO_DepthBuffer |
			CRenderTargetMngr::TO_StrictHeight|(zbuf.zmsk?CRenderTargetMngr::TO_Virtual:0),
			GET_MAXHEIGHT(zbuf.zbp, gsfb.fbw, 0));
		assert( pnewdepth != NULL && prndr != NULL );
		assert( pnewdepth->fbh == prndr->fbh );

		if( (pprevdepth != pnewdepth) || (pdepth != NULL && (pdepth->status & CRenderTarget::TS_NeedUpdate)) )
			bChanged |= 2;

		pdepth = pnewdepth;

		if( prndr->status & CRenderTarget::TS_NeedConvert32) {
			if( pdepth->pdepth != 0 )
				pdepth->SetDepthStencilSurface();
			prndr->fbh *= 2;
			prndr->ConvertTo32();
			prndr->status &= ~CRenderTarget::TS_NeedConvert32;
		}
		else if( prndr->status & CRenderTarget::TS_NeedConvert16 ) {
			if( pdepth->pdepth != 0 )
				pdepth->SetDepthStencilSurface();
			prndr->fbh /= 2;
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
				(zbuf.zmsk?CRenderTargetMngr::TO_Virtual:0), GET_MAXHEIGHT(zbuf.zbp, gsfb.fbw, 0));

			assert( pnewdepth != NULL && prndr != NULL );
			assert( pnewdepth->fbh == prndr->fbh );

			if( (pprevdepth != pnewdepth) || (pdepth != NULL && (pdepth->status & CRenderTarget::TS_NeedUpdate)) )
				bChanged = 2;

			pdepth = pnewdepth;
		}
	}
	
	if( prndr != NULL ) SetContextTarget(ictx);
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

#ifndef _WIN32
		memset(&GLWin, 0, sizeof(GLWin));
#endif
	}
	~ZeroGSInit() {
		_aligned_free(g_pbyGSMemory); g_pbyGSMemory = NULL;
		_aligned_free(g_pbyGSClut); g_pbyGSClut = NULL;
	}
};

static ZeroGSInit s_ZeroGSInit;

#ifdef _WIN32
void __stdcall glBlendFuncSeparateDummy(GLenum e1, GLenum e2, GLenum e3, GLenum e4)
#else
void APIENTRY glBlendFuncSeparateDummy(GLenum e1, GLenum e2, GLenum e3, GLenum e4)
#endif
{
	glBlendFunc(e1, e2);
}

#ifdef _WIN32
void __stdcall glBlendEquationSeparateDummy(GLenum e1, GLenum e2)
#else
void APIENTRY glBlendEquationSeparateDummy(GLenum e1, GLenum e2)
#endif
{
	glBlendEquation(e1);
}

void HandleCgError(CGcontext ctx, CGerror err, void* appdata)
{
	ERROR_LOG("Cg error: %s\n", cgGetErrorString(err));
	const char* listing = cgGetLastListing(g_cgcontext);
	if( listing != NULL ) DEBUG_LOG("	last listing: %s\n", listing);
//	int loc;
//	const GLubyte* pstr = glGetString(GL_PROGRAM_ERROR_STRING_ARB);
//	if( pstr != NULL ) printf("error at: %s\n");
//	glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB, &loc);
//	DEBUG_LOG("pos: %d\n", loc);
}

#ifndef GL_FRAMEBUFFER_INCOMPLETE_DUPLICATE_ATTACHMENT_EXT
#define GL_FRAMEBUFFER_INCOMPLETE_DUPLICATE_ATTACHMENT_EXT 0x8CD8
#endif

void ZeroGS::HandleGLError()
{
	// check the error status of this framebuffer */
	GLenum error = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);

	// if error != GL_FRAMEBUFFER_COMPLETE_EXT, there's an error of some sort 
	if( error != 0 ) {
		int w, h;
		GLint fmt;
		glGetRenderbufferParameterivEXT(GL_COLOR_ATTACHMENT0_EXT, GL_RENDERBUFFER_INTERNAL_FORMAT_EXT, &fmt);
		glGetRenderbufferParameterivEXT(GL_COLOR_ATTACHMENT0_EXT, GL_RENDERBUFFER_WIDTH_EXT, &w);
		glGetRenderbufferParameterivEXT(GL_COLOR_ATTACHMENT0_EXT, GL_RENDERBUFFER_HEIGHT_EXT, &h);

		switch(error)
		{
			case GL_FRAMEBUFFER_COMPLETE_EXT:
				break;
			case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT:
				ERROR_LOG("Error! missing a required image/buffer attachment!\n");
				break;
			case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT:
				ERROR_LOG("Error! has no images/buffers attached!\n");
				break;
//			case GL_FRAMEBUFFER_INCOMPLETE_DUPLICATE_ATTACHMENT_EXT:
//				ERROR_LOG("Error! has an image/buffer attached in multiple locations!\n");
//				break;
			case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT:
				ERROR_LOG("Error! has mismatched image/buffer dimensions!\n");
				break;
			case GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT:
				ERROR_LOG("Error! colorbuffer attachments have different types!\n");
				break;
			case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT:
				ERROR_LOG("Error! trying to draw to non-attached color buffer!\n");
				break;
			case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER_EXT:
				ERROR_LOG("Error! trying to read from a non-attached color buffer!\n");
				break;
			case GL_FRAMEBUFFER_UNSUPPORTED_EXT:
				ERROR_LOG("Error! format is not supported by current graphics card/driver!\n");
				break;
			default:
				ERROR_LOG("*UNKNOWN ERROR* reported from glCheckFramebufferStatusEXT() for %s!\n");
				break;
		}
	}
}

#ifdef _WIN32
#define GL_LOADFN(name) { \
		if( (*(void**)&name = (void*)wglGetProcAddress(#name)) == NULL ) { \
		ERROR_LOG("Failed to find %s, exiting\n", #name); \
	} \
}
#else
// let GLEW take care of it
#define GL_LOADFN(name)
#endif

bool ZeroGS::IsGLExt( const char* szTargetExtension )
{
	return mapGLExtensions.find(string(szTargetExtension)) != mapGLExtensions.end();
}

bool ZeroGS::Create(int _width, int _height)
{
	GLenum err = GL_NO_ERROR;
	bool bSuccess = true;
	int i;

	Destroy(1);
	GSStateReset(); 

	cgSetErrorHandler(HandleCgError, NULL);
	g_RenderFormatType = RFT_float16;

	nBackbufferWidth = _width;
	nBackbufferHeight = _height;
	fiRendWidth = 1.0f / nBackbufferWidth;
	fiRendHeight = 1.0f / nBackbufferHeight;

#ifdef _WIN32
	GLuint	  PixelFormat;			// Holds The Results After Searching For A Match
	DWORD	   dwExStyle;			  // Window Extended Style
	DWORD	   dwStyle;				// Window Style

	RECT rcdesktop;
	GetWindowRect(GetDesktopWindow(), &rcdesktop);
		
	if (conf.options & GSOPTION_FULLSCREEN) {
		nBackbufferWidth = rcdesktop.right - rcdesktop.left;
		nBackbufferHeight = rcdesktop.bottom - rcdesktop.top;

		DEVMODE dmScreenSettings;
		memset(&dmScreenSettings,0,sizeof(dmScreenSettings));
		dmScreenSettings.dmSize=sizeof(dmScreenSettings);
		dmScreenSettings.dmPelsWidth	= nBackbufferWidth;
		dmScreenSettings.dmPelsHeight   = nBackbufferHeight;
		dmScreenSettings.dmBitsPerPel   = 32;
		dmScreenSettings.dmFields=DM_BITSPERPEL|DM_PELSWIDTH|DM_PELSHEIGHT;

		// Try To Set Selected Mode And Get Results.  NOTE: CDS_FULLSCREEN Gets Rid Of Start Bar.
		if (ChangeDisplaySettings(&dmScreenSettings,CDS_FULLSCREEN)!=DISP_CHANGE_SUCCESSFUL)
		{
			if (MessageBox(NULL,"The Requested Fullscreen Mode Is Not Supported By\nYour Video Card. Use Windowed Mode Instead?","NeHe GL",MB_YESNO|MB_ICONEXCLAMATION)==IDYES)
				conf.options &= ~GSOPTION_FULLSCREEN;
			else
				return false;
		}
	}
	else {
		// change to default resolution
		ChangeDisplaySettings(NULL, 0);
	}

	if( conf.options & GSOPTION_FULLSCREEN) {
		dwExStyle=WS_EX_APPWINDOW;
		dwStyle=WS_POPUP;
		ShowCursor(FALSE);
	}
	else {
		dwExStyle=WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
		dwStyle=WS_OVERLAPPEDWINDOW;
	}

	RECT rc;
	rc.left = 0; rc.top = 0;
	rc.right = nBackbufferWidth; rc.bottom = nBackbufferHeight;
	AdjustWindowRectEx(&rc, dwStyle, FALSE, dwExStyle);
	int X = (rcdesktop.right-rcdesktop.left)/2 - (rc.right-rc.left)/2;
	int Y = (rcdesktop.bottom-rcdesktop.top)/2 - (rc.bottom-rc.top)/2;

	SetWindowPos(GShwnd, NULL, X, Y, rc.right-rc.left, rc.bottom-rc.top, SWP_NOREPOSITION|SWP_NOZORDER);

	PIXELFORMATDESCRIPTOR pfd=			  // pfd Tells Windows How We Want Things To Be
	{
		sizeof(PIXELFORMATDESCRIPTOR),			  // Size Of This Pixel Format Descriptor
		1,										  // Version Number
		PFD_DRAW_TO_WINDOW |						// Format Must Support Window
		PFD_SUPPORT_OPENGL |						// Format Must Support OpenGL
		PFD_DOUBLEBUFFER,						   // Must Support Double Buffering
		PFD_TYPE_RGBA,							  // Request An RGBA Format
		32,										 // Select Our Color Depth
		0, 0, 0, 0, 0, 0,						   // Color Bits Ignored
		0,										  // 8bit Alpha Buffer
		0,										  // Shift Bit Ignored
		0,										  // No Accumulation Buffer
		0, 0, 0, 0,								 // Accumulation Bits Ignored
		24,										 // 24Bit Z-Buffer (Depth Buffer)  
		8,										  // 8bit Stencil Buffer
		0,										  // No Auxiliary Buffer
		PFD_MAIN_PLANE,							 // Main Drawing Layer
		0,										  // Reserved
		0, 0, 0									 // Layer Masks Ignored
	};
	
	if (!(hDC=GetDC(GShwnd))) {
		MessageBox(NULL,"(1) Can't Create A GL Device Context.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return false;
	}
	
	if (!(PixelFormat=ChoosePixelFormat(hDC,&pfd))) {
		MessageBox(NULL,"(2) Can't Find A Suitable PixelFormat.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return false;
	}

	if(!SetPixelFormat(hDC,PixelFormat,&pfd)) {
		MessageBox(NULL,"(3) Can't Set The PixelFormat.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return false;
	}

	if (!(hRC=wglCreateContext(hDC))) {
		MessageBox(NULL,"(4) Can't Create A GL Rendering Context.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return false;
	}

	if(!wglMakeCurrent(hDC,hRC)) {
		MessageBox(NULL,"(5) Can't Activate The GL Rendering Context.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return false;
	}

#else
	GLWin.DisplayWindow(_width, _height);
#endif

	// fill the opengl extension map
	const char* ptoken = (const char*)glGetString( GL_EXTENSIONS );
	if( ptoken == NULL ) return false;

	int prevlog = conf.log;
	conf.log = 1;
	GS_LOG("Supported OpenGL Extensions:\n%s\n", ptoken);	 // write to the log file
	conf.log = prevlog;

	// insert all exts into mapGLExtensions

	const char* pend = NULL;

	while(ptoken != NULL ) {
		pend = strchr(ptoken, ' ');

		if( pend != NULL ) {
			mapGLExtensions[string(ptoken, pend-ptoken)];
		}
		else {
			mapGLExtensions[string(ptoken)];
			break;
		}

		ptoken = pend;
		while(*ptoken == ' ') ++ptoken;
	}

	s_nFullscreen = (conf.options & GSOPTION_FULLSCREEN) ? 1 : 0;
	conf.mrtdepth = 0; // for now

#ifndef _WIN32
	int const glew_ok = glewInit();
	if( glew_ok != GLEW_OK ) {
		ERROR_LOG("glewInit() is not ok!\n");
		return false;
	}
#endif

	if( !IsGLExt("GL_EXT_framebuffer_object") ) {
		ERROR_LOG("*********\nZeroGS: ERROR: Need GL_EXT_framebufer_object for multiple render targets\nZeroGS: *********\n");
		bSuccess = false;
	}
	if( !IsGLExt("GL_EXT_blend_equation_separate") || glBlendEquationSeparateEXT == NULL ) {
		ERROR_LOG("*********\nZeroGS: OGL WARNING: Need GL_EXT_blend_equation_separate\nZeroGS: *********\n");
		zgsBlendEquationSeparateEXT = glBlendEquationSeparateDummy;
	}
	else {
		zgsBlendEquationSeparateEXT = glBlendEquationSeparateEXT;
	}

	if( !IsGLExt("GL_EXT_blend_func_separate") || glBlendFuncSeparateEXT == NULL ) {
		ERROR_LOG("*********\nZeroGS: OGL WARNING: Need GL_EXT_blend_func_separate\nZeroGS: *********\n");
		zgsBlendFuncSeparateEXT = glBlendFuncSeparateDummy;
	}
	else {
		zgsBlendFuncSeparateEXT = glBlendFuncSeparateEXT;
	}

	if( !IsGLExt("GL_EXT_secondary_color") ) {
		ERROR_LOG("*********\nZeroGS: OGL WARNING: Need GL_EXT_secondary_color\nZeroGS: *********\n");
		bSuccess = false;
	}

	if( !IsGLExt("GL_ARB_draw_buffers") && !IsGLExt("GL_ATI_draw_buffers") ) {
		ERROR_LOG("*********\nZeroGS: OGL WARNING: multiple render targets not supported, some effects might look bad\nZeroGS: *********\n");
		conf.mrtdepth = 0;
	}

	if( !bSuccess )
		return false;
	
	if( g_GameSettings & GAME_32BITTARGS ) {
		g_RenderFormatType = RFT_byte8;
		ERROR_LOG("Setting 32 bit render target\n");
	}
	else {
		if( !IsGLExt("GL_NV_float_buffer") && !IsGLExt("GL_ARB_color_buffer_float") && !IsGLExt("ATI_pixel_format_float") ) {
			ERROR_LOG("******\nZeroGS: GS WARNING: Floating point render targets not supported, switching to 32bit\nZeroGS: *********\n");
			g_RenderFormatType = RFT_byte8;
		}
	}
	g_RenderFormatType = RFT_byte8;

#ifdef _WIN32
	if( IsGLExt("WGL_EXT_swap_control") || IsGLExt("EXT_swap_control") )
		wglSwapIntervalEXT(0);
#else
	if( IsGLExt("GLX_SGI_swap_control") ) {
		_PFNSWAPINTERVAL swapinterval = (_PFNSWAPINTERVAL)wglGetProcAddress("glXSwapInterval");
		if( !swapinterval )
			swapinterval = (_PFNSWAPINTERVAL)wglGetProcAddress("glXSwapIntervalSGI");
		if( !swapinterval )
			swapinterval = (_PFNSWAPINTERVAL)wglGetProcAddress("glXSwapIntervalEXT");

		if( swapinterval )
			swapinterval(0);
		else
			ERROR_LOG("no support for SwapInterval (framerate clamped to monitor refresh rate)\n");
	}
#endif

	// check the max texture width and height
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &g_MaxTexWidth);
	g_MaxTexHeight = g_MaxTexWidth;
	GPU_TEXWIDTH = g_MaxTexWidth/8;
	g_fiGPU_TEXWIDTH = 1.0f / GPU_TEXWIDTH;

#ifndef ZEROGS_DEVBUILD
#ifdef _WIN32
	HRSRC hShaderSrc = FindResource(hInst, MAKEINTRESOURCE(IDR_SHADERS), RT_RCDATA);
	assert( hShaderSrc != NULL );
	HGLOBAL hShaderGlob = LoadResource(hInst, hShaderSrc);
	assert( hShaderGlob != NULL );
	s_lpShaderResources = (u8*)LockResource(hShaderGlob);
#else
	FILE* fres = fopen("ps2hw.dat", "rb");
	if( fres == NULL ) {
		fres = fopen("plugins/ps2hw.dat", "rb");
		if( fres == NULL ) {
			ERROR_LOG("Cannot find ps2hw.dat in working directory. Exiting\n");
			return false;
		}
	}
	fseek(fres, 0, SEEK_END);
	size_t s = ftell(fres);
	s_lpShaderResources = new u8[s+1];
	fseek(fres, 0, SEEK_SET);
	fread(s_lpShaderResources, s, 1, fres);
	s_lpShaderResources[s] = 0;
#endif

#else

#ifndef _WIN32
	// test if ps2hw.fx exists
	char tempstr[255];
	char curwd[255];
	getcwd(curwd, ARRAY_SIZE(curwd));
	
	strcpy(tempstr, "../plugins/zerogs/opengl/");
	sprintf(EFFECT_NAME, "%sps2hw.fx", tempstr);
	FILE* f = fopen(EFFECT_NAME, "r");
	if( f == NULL ) {

		strcpy(tempstr, "../../plugins/zerogs/opengl/");
		sprintf(EFFECT_NAME, "%sps2hw.fx", tempstr);
		f = fopen(EFFECT_NAME, "r");

		if( f == NULL ) {
			ERROR_LOG("Failed to find %s, try compiling a non-devbuild\n", EFFECT_NAME);
			return false;
		}
	}
	fclose(f);

	sprintf(EFFECT_DIR, "%s/%s", curwd, tempstr);
	sprintf(EFFECT_NAME, "%sps2hw.fx", EFFECT_DIR);
#endif

#endif // ZEROGS_DEVBUILD

	// load the effect, find the best profiles (if any)
	if( cgGLIsProfileSupported(CG_PROFILE_ARBVP1) != CG_TRUE ) {
		ERROR_LOG("arbvp1 not supported\n");
		return false;
	}
	if( cgGLIsProfileSupported(CG_PROFILE_ARBFP1) != CG_TRUE ) {
		ERROR_LOG("arbfp1 not supported\n");
		return false;
	}

	GL_REPORT_ERROR();
	if( err != GL_NO_ERROR ) bSuccess = false;

	s_srcrgb = s_dstrgb = s_srcalpha = s_dstalpha = GL_ONE;

	GL_LOADFN(glIsRenderbufferEXT);
	GL_LOADFN(glBindRenderbufferEXT);
	GL_LOADFN(glDeleteRenderbuffersEXT);
	GL_LOADFN(glGenRenderbuffersEXT);
	GL_LOADFN(glRenderbufferStorageEXT);
	GL_LOADFN(glGetRenderbufferParameterivEXT);
	GL_LOADFN(glIsFramebufferEXT);
	GL_LOADFN(glBindFramebufferEXT);
	GL_LOADFN(glDeleteFramebuffersEXT);
	GL_LOADFN(glGenFramebuffersEXT);
	GL_LOADFN(glCheckFramebufferStatusEXT);
	GL_LOADFN(glFramebufferTexture1DEXT);
	GL_LOADFN(glFramebufferTexture2DEXT);
	GL_LOADFN(glFramebufferTexture3DEXT);
	GL_LOADFN(glFramebufferRenderbufferEXT);
	GL_LOADFN(glGetFramebufferAttachmentParameterivEXT);
	GL_LOADFN(glGenerateMipmapEXT);

	if( IsGLExt("GL_ARB_draw_buffers") )
		glDrawBuffers = (PFNGLDRAWBUFFERSPROC)wglGetProcAddress("glDrawBuffers");
	else if( IsGLExt("GL_ATI_draw_buffers") )
		glDrawBuffers = (PFNGLDRAWBUFFERSPROC)wglGetProcAddress("glDrawBuffersATI");

	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);
	
	GL_REPORT_ERROR();
	if( err != GL_NO_ERROR ) bSuccess = false;

	glGenFramebuffersEXT( 1, &s_uFramebuffer);
	if( s_uFramebuffer == 0 ) {
		ERROR_LOG("failed to create the renderbuffer\n");
	}

	assert( glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT) == GL_FRAMEBUFFER_COMPLETE_EXT );
	glBindFramebufferEXT( GL_FRAMEBUFFER_EXT, s_uFramebuffer );

	if( glDrawBuffers != NULL )
		glDrawBuffers(1, s_drawbuffers);
	GL_REPORT_ERROR();
	if( err != GL_NO_ERROR ) bSuccess = false;

	font_p = new RasterFont();

	GL_REPORT_ERROR();
	if( err != GL_NO_ERROR ) bSuccess = false;

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
	GL_STENCILFUNC(GL_ALWAYS, 0, 0);

	//g_GameSettings |= 0;//GAME_VSSHACK|GAME_FULL16BITRES|GAME_NODEPTHRESOLVE|GAME_FASTUPDATE;
	//s_bWriteDepth = TRUE;

	GL_BLEND_ALL(GL_ONE, GL_ONE, GL_ONE, GL_ONE);

	glViewport(0,0,nBackbufferWidth,nBackbufferHeight);					 // Reset The Current Viewport

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glShadeModel(GL_SMOOTH);
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClearDepth(1.0f);
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_LIGHTING);
	glDepthFunc(GL_LEQUAL);
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);  // Really Nice Perspective Calculations
	
	glGenTextures(1, &ptexLogo);
	glBindTexture(GL_TEXTURE_RECTANGLE_NV, ptexLogo);

#ifdef _WIN32
	HRSRC hBitmapSrc = FindResource(hInst, MAKEINTRESOURCE(IDB_ZEROGSLOGO), RT_BITMAP);
	assert( hBitmapSrc != NULL );
	HGLOBAL hBitmapGlob = LoadResource(hInst, hBitmapSrc);
	assert( hBitmapGlob != NULL );
	PBITMAPINFO pinfo = (PBITMAPINFO)LockResource(hBitmapGlob);

	glTexImage2D(GL_TEXTURE_RECTANGLE_NV, 0, 4, pinfo->bmiHeader.biWidth, pinfo->bmiHeader.biHeight, 0, pinfo->bmiHeader.biBitCount==32?GL_RGBA:GL_RGB, GL_UNSIGNED_BYTE, (u8*)pinfo+pinfo->bmiHeader.biSize);
	nLogoWidth = pinfo->bmiHeader.biWidth;
	nLogoHeight = pinfo->bmiHeader.biHeight;
	glTexParameteri(GL_TEXTURE_RECTANGLE_NV, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_RECTANGLE_NV, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
#else
#endif

	GL_REPORT_ERROR();

	g_nCurVBOIndex = 0;
	g_vboBuffers.resize(VB_NUMBUFFERS);
	glGenBuffers((GLsizei)g_vboBuffers.size(), &g_vboBuffers[0]);
	for(i = 0; i < (int)g_vboBuffers.size(); ++i) {
		glBindBuffer(GL_ARRAY_BUFFER, g_vboBuffers[i]);
		glBufferData(GL_ARRAY_BUFFER, 0x100*sizeof(VertexGPU), NULL, GL_STREAM_DRAW);
	}

	GL_REPORT_ERROR();
	if( err != GL_NO_ERROR ) bSuccess = false;

	// create the blocks texture
	g_fBlockMult = 1;

	vector<char> vBlockData, vBilinearData;
	BLOCK::FillBlocks(vBlockData, vBilinearData, 1);

	glGenTextures(1, &ptexBlocks);
	glBindTexture(GL_TEXTURE_2D, ptexBlocks);

	g_internalFloatFmt = GL_ALPHA_FLOAT32_ATI;
	g_internalRGBAFloatFmt = GL_RGBA_FLOAT32_ATI;
	g_internalRGBAFloat16Fmt = GL_RGBA_FLOAT16_ATI;

	glTexImage2D(GL_TEXTURE_2D, 0, g_internalFloatFmt, BLOCK_TEXWIDTH, BLOCK_TEXHEIGHT, 0, GL_ALPHA, GL_FLOAT, &vBlockData[0]);

	if( glGetError() != GL_NO_ERROR ) {
		// try different internal format
		g_internalFloatFmt = GL_FLOAT_R32_NV;
		glTexImage2D(GL_TEXTURE_2D, 0, g_internalFloatFmt, BLOCK_TEXWIDTH, BLOCK_TEXHEIGHT, 0, GL_RED, GL_FLOAT, &vBlockData[0]);
	}

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	if( glGetError() != GL_NO_ERROR ) {

		// error, resort to 16bit
		g_fBlockMult = 65535.0f*(float)g_fiGPU_TEXWIDTH;

		BLOCK::FillBlocks(vBlockData, vBilinearData, 0);
		glTexImage2D(GL_TEXTURE_2D, 0, 2, BLOCK_TEXWIDTH, BLOCK_TEXHEIGHT, 0, GL_R, GL_UNSIGNED_SHORT, &vBlockData[0]);
		if( glGetError() != GL_NO_ERROR )
			return false;
	}
	else {
		// fill in the bilinear blocks
		glGenTextures(1, &ptexBilinearBlocks);
		glBindTexture(GL_TEXTURE_2D, ptexBilinearBlocks);
		glTexImage2D(GL_TEXTURE_2D, 0, g_internalRGBAFloatFmt, BLOCK_TEXWIDTH, BLOCK_TEXHEIGHT, 0, GL_RGBA, GL_FLOAT, &vBilinearData[0]);

		if( glGetError() != GL_NO_ERROR ) {
			g_internalRGBAFloatFmt = GL_FLOAT_RGBA32_NV;
			g_internalRGBAFloat16Fmt = GL_FLOAT_RGBA16_NV;
			glTexImage2D(GL_TEXTURE_2D, 0, g_internalRGBAFloatFmt, BLOCK_TEXWIDTH, BLOCK_TEXHEIGHT, 0, GL_RGBA, GL_FLOAT, &vBilinearData[0]);
			B_G(glGetError() == GL_NO_ERROR, return false);
		}

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	}

	float fpri = 1;
	glPrioritizeTextures(1, &ptexBlocks, &fpri);
	if( ptexBilinearBlocks != 0 )
		glPrioritizeTextures(1, &ptexBilinearBlocks, &fpri);

	GL_REPORT_ERROR();

	// fill a simple rect
	glGenBuffers(1, &vboRect);
	glBindBuffer(GL_ARRAY_BUFFER, vboRect);
	
	vector<VertexGPU> verts(4);
	VertexGPU* pvert = &verts[0];
	pvert->x = -0x7fff; pvert->y = 0x7fff; pvert->z = 0; pvert->s = 0; pvert->t = 0; pvert++;
	pvert->x = 0x7fff; pvert->y = 0x7fff; pvert->z = 0; pvert->s = 1; pvert->t = 0; pvert++;
	pvert->x = -0x7fff; pvert->y = -0x7fff; pvert->z = 0; pvert->s = 0; pvert->t = 1; pvert++;
	pvert->x = 0x7fff; pvert->y = -0x7fff; pvert->z = 0; pvert->s = 1; pvert->t = 1; pvert++;
	glBufferDataARB(GL_ARRAY_BUFFER, 4*sizeof(VertexGPU), &verts[0], GL_STATIC_DRAW);

	// setup the default vertex declaration
	glEnableClientState(GL_VERTEX_ARRAY);
	glClientActiveTexture(GL_TEXTURE0);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnableClientState(GL_SECONDARY_COLOR_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);
	GL_REPORT_ERROR();

	// some cards don't support this
//	glClientActiveTexture(GL_TEXTURE0);
//	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
//	glTexCoordPointer(4, GL_UNSIGNED_BYTE, sizeof(VertexGPU), (void*)12);

	// create the conversion textures
	glGenTextures(1, &ptexConv16to32);
	glBindTexture(GL_TEXTURE_2D, ptexConv16to32);

	vector<u32> conv16to32data(256*256);
	for(i = 0; i < 256*256; ++i) {
		u32 tempcol = RGBA16to32(i);
		// have to flip r and b
		conv16to32data[i] = (tempcol&0xff00ff00)|((tempcol&0xff)<<16)|((tempcol&0xff0000)>>16);
	}
	glTexImage2D(GL_TEXTURE_2D, 0, 4, 256, 256, 0, GL_RGBA, GL_UNSIGNED_BYTE, &conv16to32data[0]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

	GL_REPORT_ERROR();
	if( err != GL_NO_ERROR ) bSuccess = false;

	vector<u32> conv32to16data(32*32*32);

	glGenTextures(1, &ptexConv32to16);
	glBindTexture(GL_TEXTURE_3D, ptexConv32to16);
	u32* dst = &conv32to16data[0];
	for(i = 0; i < 32; ++i) {
		for(int j = 0; j < 32; ++j) {
			for(int k = 0; k < 32; ++k) {
				u32 col = (i<<10)|(j<<5)|k;
				*dst++ = ((col&0xff)<<16)|(col&0xff00);
			}
		}
	}
	glTexImage3D(GL_TEXTURE_3D, 0, 4, 32, 32, 32, 0, GL_RGBA, GL_UNSIGNED_BYTE, &conv32to16data[0]);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP);
	GL_REPORT_ERROR();
	if( err != GL_NO_ERROR ) bSuccess = false;

	g_cgcontext = cgCreateContext();
	
	cgvProf = CG_PROFILE_ARBVP1;
	cgfProf = CG_PROFILE_ARBFP1;
	cgGLEnableProfile(cgvProf);
	cgGLEnableProfile(cgfProf);
	cgGLSetOptimalOptions(cgvProf);
	cgGLSetOptimalOptions(cgfProf);

	cgGLSetManageTextureParameters(g_cgcontext, CG_FALSE);
	//cgSetAutoCompile(g_cgcontext, CG_COMPILE_IMMEDIATE);

	g_fparamFogColor = cgCreateParameter(g_cgcontext, CG_FLOAT4);
	g_vparamPosXY[0] = cgCreateParameter(g_cgcontext, CG_FLOAT4);
	g_vparamPosXY[1] = cgCreateParameter(g_cgcontext, CG_FLOAT4);

	ERROR_LOG("Creating effects\n");
	B_G(LoadEffects(), return false);

	g_bDisplayMsg = 0;


	// create a sample shader
	clampInfo temp;
	memset(&temp, 0, sizeof(temp));
	temp.wms = 3; temp.wmt = 3;

	g_nPixelShaderVer = SHADER_ACCURATE;
	// test
	bool bFailed;
	FRAGMENTSHADER* pfrag = LoadShadeEffect(0, 1, 1, 1, 1, temp, 0, &bFailed);
	if( bFailed || pfrag == NULL ) {
		g_nPixelShaderVer = SHADER_ACCURATE|SHADER_REDUCED;

		pfrag = LoadShadeEffect(0, 0, 1, 1, 0, temp, 0, &bFailed);
		if( pfrag != NULL )
			cgGLLoadProgram(pfrag->prog);
		if( bFailed || pfrag == NULL || cgGetError() != CG_NO_ERROR ) {
			g_nPixelShaderVer = SHADER_REDUCED;
			ERROR_LOG("Basic shader test failed\n");
		}
	}

	g_bDisplayMsg = 1;
	if( g_nPixelShaderVer & SHADER_REDUCED )
		conf.bilinear = 0;

	ERROR_LOG("Creating extra effects\n");
	B_G(LoadExtraEffects(), return false);

	ERROR_LOG("using %s shaders\n", g_pShaders[g_nPixelShaderVer]);

	GL_REPORT_ERROR();
	if( err != GL_NO_ERROR ) bSuccess = false;

	glDisable(GL_STENCIL_TEST);
	glEnable(GL_SCISSOR_TEST);

	GL_BLEND_ALPHA(GL_ONE, GL_ZERO);
	glBlendColorEXT(0, 0, 0, 0.5f);
	
	glDisable(GL_CULL_FACE);

	// points
	// This was changed in SetAA - should we be changing it back?
	glPointSize(1.0f);
	g_nDepthBias = 0;
	glEnable(GL_POLYGON_OFFSET_FILL);
	glEnable(GL_POLYGON_OFFSET_LINE);
	glPolygonOffset(0, 1);

	vb[0].Init(VB_BUFFERSIZE);
	vb[1].Init(VB_BUFFERSIZE);
	g_bSaveFlushedFrame = 1;

	g_vsprog = g_psprog = 0;

	return glGetError() == GL_NO_ERROR && bSuccess;
}

void ZeroGS::Destroy(BOOL bD3D)
{
	if( s_aviinit ) {
		StopCapture();

#ifdef _WIN32
		STOP_AVI();
#else // linux
		//TODO
#endif
		ERROR_LOG("zerogs.avi stopped");
		s_aviinit = 0;
	}

	g_MemTargs.Destroy();
	s_RTs.Destroy();
	s_DepthRTs.Destroy();
	s_BitwiseTextures.Destroy();

	SAFE_RELEASE_TEX(s_ptexInterlace);
	SAFE_RELEASE_TEX(ptexBlocks);
	SAFE_RELEASE_TEX(ptexBilinearBlocks);
	SAFE_RELEASE_TEX(ptexConv16to32);
	SAFE_RELEASE_TEX(ptexConv32to16);

	vb[0].Destroy();
	vb[1].Destroy();

	if( g_vboBuffers.size() > 0 ) {
		glDeleteBuffers((GLsizei)g_vboBuffers.size(), &g_vboBuffers[0]);
		g_vboBuffers.clear();
	}
	g_nCurVBOIndex = 0;

	for(int i = 0; i < ARRAY_SIZE(pvs); ++i) {
		SAFE_RELEASE_PROG(pvs[i]);
	}
	for(int i = 0; i < ARRAY_SIZE(ppsRegular); ++i) {
		SAFE_RELEASE_PROG(ppsRegular[i].prog);
	}
	for(int i = 0; i < ARRAY_SIZE(ppsTexture); ++i) {
		SAFE_RELEASE_PROG(ppsTexture[i].prog);
	}

	SAFE_RELEASE_PROG(pvsBitBlt.prog);
	SAFE_RELEASE_PROG(ppsBitBlt[0].prog); SAFE_RELEASE_PROG(ppsBitBlt[1].prog);
	SAFE_RELEASE_PROG(ppsBitBltDepth.prog);
	SAFE_RELEASE_PROG(ppsCRTCTarg[0].prog); SAFE_RELEASE_PROG(ppsCRTCTarg[1].prog);
	SAFE_RELEASE_PROG(ppsCRTC[0].prog); SAFE_RELEASE_PROG(ppsCRTC[1].prog);
	SAFE_RELEASE_PROG(ppsCRTC24[0].prog); SAFE_RELEASE_PROG(ppsCRTC24[1].prog);
	SAFE_RELEASE_PROG(ppsOne.prog);

	SAFE_DELETE(font_p);

#ifdef _WIN32
	if (hRC)											// Do We Have A Rendering Context?
	{
		if (!wglMakeCurrent(NULL,NULL))				 // Are We Able To Release The DC And RC Contexts?
		{
			MessageBox(NULL,"Release Of DC And RC Failed.","SHUTDOWN ERROR",MB_OK | MB_ICONINFORMATION);
		}

		if (!wglDeleteContext(hRC))					 // Are We Able To Delete The RC?
		{
			MessageBox(NULL,"Release Rendering Context Failed.","SHUTDOWN ERROR",MB_OK | MB_ICONINFORMATION);
		}
		hRC=NULL;									   // Set RC To NULL
	}

	if (hDC && !ReleaseDC(GShwnd,hDC))				  // Are We Able To Release The DC
	{
		MessageBox(NULL,"Release Device Context Failed.","SHUTDOWN ERROR",MB_OK | MB_ICONINFORMATION);
		hDC=NULL;									   // Set DC To NULL
	}
#else // linux
    GLWin.DestroyWindow();
#endif

	mapGLExtensions.clear();
}

void ZeroGS::GSStateReset()
{
	icurctx = -1;

	for(int i = 0; i < 2; ++i) {
		vb[i].Destroy();
		memset(&vb[i], 0, SIZEOF_VB);

		vb[i].tex0.tw = 1;
		vb[i].tex0.th = 1;
		vb[i].scissor.x1 = 639;
		vb[i].scissor.y1 = 479;
		vb[i].tex0.tbw = 64;
		vb[i].Init(VB_BUFFERSIZE);
	}

	s_RangeMngr.Clear();
	g_MemTargs.Destroy();
	s_RTs.Destroy();
	s_DepthRTs.Destroy();
	s_BitwiseTextures.Destroy();

	vb[0].ictx = 0;
	vb[1].ictx = 1;
}

void ZeroGS::AddMessage(const char* pstr, u32 ms)
{
	listMsgs.push_back(MESSAGE(pstr, timeGetTime()+ms));
}

void ZeroGS::DrawText(const char* pstr, int left, int top, u32 color)
{
	cgGLDisableProfile(cgvProf);
	cgGLDisableProfile(cgfProf);
	
	glColor3f(((color>>16)&0xff)/255.0f, ((color>>8)&0xff)/255.0f, (color&0xff)/255.0f);
	
	font_p->printString(pstr, left * 2.0f / (float)nBackbufferWidth - 1, 1 - top * 2.0f / (float)nBackbufferHeight,0);
	cgGLEnableProfile(cgvProf);
	cgGLEnableProfile(cgfProf);
}

void ZeroGS::ChangeWindowSize(int nNewWidth, int nNewHeight)
{
	nBackbufferWidth = nNewWidth > 16 ? nNewWidth : 16;
	nBackbufferHeight = nNewHeight > 16 ? nNewHeight : 16;

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
	
	vb[0].nCount = 0;
	vb[1].nCount = 0;

	memset(s_nResolveCounts, 0, sizeof(s_nResolveCounts));
	s_nLastResolveReset = 0;

	icurctx = -1;
	g_vsprog = g_psprog = 0;

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

	int oldwidth = nBackbufferWidth, oldheight = nBackbufferHeight;
	if( !Create(nNewWidth&~7, nNewHeight&~7) ) {
		ERROR_LOG("Failed to recreate, changing to old\n");
		if( !Create(oldwidth, oldheight) ) {
			SysMessage("failed to create dev, exiting...\n");
			exit(0);
		}
	}

	for(int i = 0; i < 2; ++i) {
		vb[i].bNeedFrameCheck = vb[i].bNeedZCheck = 1;
		vb[i].CheckFrame(0);
	}

	if( oldscreen && !(conf.options & GSOPTION_FULLSCREEN) ) { // if transitioning from full screen
		RECT rc;
		rc.left = 0; rc.top = 0;
		rc.right = conf.width; rc.bottom = conf.height;

#ifdef _WIN32
		AdjustWindowRect(&rc, conf.winstyle, FALSE);

		RECT rcdesktop;
		GetWindowRect(GetDesktopWindow(), &rcdesktop);

		SetWindowLong( GShwnd, GWL_STYLE, conf.winstyle );
		SetWindowPos(GShwnd, HWND_TOP, ((rcdesktop.right-rcdesktop.left)-(rc.right-rc.left))/2,
			((rcdesktop.bottom-rcdesktop.top)-(rc.bottom-rc.top))/2,
			rc.right-rc.left, rc.bottom-rc.top, SWP_SHOWWINDOW);
		UpdateWindow(GShwnd);
#else // linux
#endif
	}

	assert( vb[0].pBufferData != NULL && vb[1].pBufferData != NULL );
}

void ZeroGS::SetAA(int mode)
{
	float f;
	
	// need to flush all targets
	s_RTs.ResolveAll();
	s_RTs.Destroy();
	s_DepthRTs.ResolveAll();
	s_DepthRTs.Destroy();
	
	s_AAx = s_AAy = 0;			// This is code for x0, x2, x4, x8 and x16 anti-aliasing.
	if (mode > 0) 
	{
		s_AAx = (mode+1) / 2;		// ( 1, 0 ) ; (  1, 1 ) ; ( 2, 1 ) ; ( 2, 2 ) -- it's used as binary shift, so x >> s_AAx, y >> s_AAy
		s_AAy = mode / 2;
	}

	memset(s_nResolveCounts, 0, sizeof(s_nResolveCounts));
	s_nLastResolveReset = 0;

	vb[0].prndr = NULL; vb[0].pdepth = NULL; vb[0].bNeedFrameCheck = 1; vb[0].bNeedZCheck = 1;
	vb[1].prndr = NULL; vb[1].pdepth = NULL; vb[1].bNeedFrameCheck = 1; vb[1].bNeedZCheck = 1;
	
	f = mode > 0 ? 2.0f : 1.0f;
	glPointSize(f);
}

#define SET_UNIFORMPARAM(var, name) { \
	p = cgGetNamedParameter(pf->prog, name); \
	if( p != NULL && cgIsParameterUsed(p, pf->prog) == CG_TRUE ) \
		pf->var = p; \
} \

void SetupFragmentProgramParameters(FRAGMENTSHADER* pf, int context, int type)
{
	// uniform parameters
	CGparameter p;

	p = cgGetNamedParameter(pf->prog, "g_fFogColor");
	if( p != NULL && cgIsParameterUsed(p, pf->prog) == CG_TRUE ) {
		cgConnectParameter(g_fparamFogColor, p);
	}

	SET_UNIFORMPARAM(sOneColor, "g_fOneColor");
	SET_UNIFORMPARAM(sBitBltZ, "g_fBitBltZ");
	SET_UNIFORMPARAM(sInvTexDims, "g_fInvTexDims");
	SET_UNIFORMPARAM(fTexAlpha2, "fTexAlpha2");
	SET_UNIFORMPARAM(fTexOffset, "g_fTexOffset");
	SET_UNIFORMPARAM(fTexDims, "g_fTexDims");
	SET_UNIFORMPARAM(fTexBlock, "g_fTexBlock");
	SET_UNIFORMPARAM(fClampExts, "g_fClampExts");
	SET_UNIFORMPARAM(fTexWrapMode, "TexWrapMode");
	SET_UNIFORMPARAM(fRealTexDims, "g_fRealTexDims");
	SET_UNIFORMPARAM(fTestBlack, "g_fTestBlack");
	SET_UNIFORMPARAM(fPageOffset, "g_fPageOffset");
	SET_UNIFORMPARAM(fTexAlpha, "fTexAlpha");

	// textures
	p = cgGetNamedParameter(pf->prog, "g_sBlocks");
	if( p != NULL && cgIsParameterUsed(p, pf->prog) == CG_TRUE ) {
		cgGLSetTextureParameter(p, ptexBlocks);
		cgGLEnableTextureParameter(p);
	}

	// cg parameter usage is wrong, so do it manually
	if( type == 3 ) {
		p = cgGetNamedParameter(pf->prog, "g_sConv16to32");
		if( p != NULL && cgIsParameterUsed(p, pf->prog) == CG_TRUE ) {
			cgGLSetTextureParameter(p, ptexConv16to32);
			cgGLEnableTextureParameter(p);
		}
	}
	else if( type == 4 ) {
		p = cgGetNamedParameter(pf->prog, "g_sConv32to16");
		if( p != NULL && cgIsParameterUsed(p, pf->prog) == CG_TRUE ) {
			cgGLSetTextureParameter(p, ptexConv32to16);
			cgGLEnableTextureParameter(p);
		}
	}
	else {
		p = cgGetNamedParameter(pf->prog, "g_sBilinearBlocks");
		if( p != NULL && cgIsParameterUsed(p, pf->prog) == CG_TRUE ) {
			cgGLSetTextureParameter(p, ptexBilinearBlocks);
			cgGLEnableTextureParameter(p);
		}
	}

	p = cgGetNamedParameter(pf->prog, "g_sMemory");
	if( p != NULL && cgIsParameterUsed(p, pf->prog) == CG_TRUE ) {
		//cgGLEnableTextureParameter(p);
		pf->sMemory = p;
	}
	p = cgGetNamedParameter(pf->prog, "g_sSrcFinal");
	if( p != NULL && cgIsParameterUsed(p, pf->prog) == CG_TRUE ) {
		//cgGLEnableTextureParameter(p);
		pf->sFinal = p;
	}
	p = cgGetNamedParameter(pf->prog, "g_sBitwiseANDX");
	if( p != NULL && cgIsParameterUsed(p, pf->prog) == CG_TRUE ) {
		//cgGLEnableTextureParameter(p);
		pf->sBitwiseANDX = p;
	}
	p = cgGetNamedParameter(pf->prog, "g_sBitwiseANDY");
	if( p != NULL && cgIsParameterUsed(p, pf->prog) == CG_TRUE ) {
		//cgGLEnableTextureParameter(p);
		pf->sBitwiseANDY = p;
	}
	p = cgGetNamedParameter(pf->prog, "g_sCLUT");
	if( p != NULL && cgIsParameterUsed(p, pf->prog) == CG_TRUE ) {
		//cgGLEnableTextureParameter(p);
		pf->sCLUT = p;
	}
	p = cgGetNamedParameter(pf->prog, "g_sInterlace");
	if( p != NULL && cgIsParameterUsed(p, pf->prog) == CG_TRUE ) {
		//cgGLEnableTextureParameter(p);
		pf->sInterlace = p;
	}

	// set global shader constants
	p = cgGetNamedParameter(pf->prog, "g_fExactColor");
	if( p != NULL && cgIsParameterUsed(p, pf->prog) == CG_TRUE ) {
		cgGLSetParameter4fv(p, Vector(0.5f, (g_GameSettings&GAME_EXACTCOLOR)?0.9f/256.0f:0.5f/256.0f, 0,1/255.0f));
	}

	p = cgGetNamedParameter(pf->prog, "g_fBilinear");
	if( p != NULL && cgIsParameterUsed(p, pf->prog) == CG_TRUE )
		cgGLSetParameter4fv(p, Vector(-0.2f, -0.65f, 0.9f, 1.0f / 32767.0f ));

	p = cgGetNamedParameter(pf->prog, "g_fZBias");
	if( p != NULL && cgIsParameterUsed(p, pf->prog) == CG_TRUE )
		cgGLSetParameter4fv(p, Vector(1.0f/256.0f, 1.0004f, 1, 0.5f));

	p = cgGetNamedParameter(pf->prog, "g_fc0");
	if( p != NULL && cgIsParameterUsed(p, pf->prog) == CG_TRUE )
		cgGLSetParameter4fv(p, Vector(0,1, 0.001f, 0.5f));

	p = cgGetNamedParameter(pf->prog, "g_fMult");
	if( p != NULL && cgIsParameterUsed(p, pf->prog) == CG_TRUE )
		cgGLSetParameter4fv(p, Vector(1/1024.0f, 0.2f/1024.0f, 1/128.0f, 1/512.0f));
}

void SetupVertexProgramParameters(CGprogram prog, int context)
{
	CGparameter p;

	p = cgGetNamedParameter(prog, "g_fPosXY");
	if( p != NULL && cgIsParameterUsed(p, prog) == CG_TRUE )
		cgConnectParameter(g_vparamPosXY[context], p);

	p = cgGetNamedParameter(prog, "g_fZ");
	if( p != NULL && cgIsParameterUsed(p, prog) == CG_TRUE )
		cgGLSetParameter4fv(p, g_vdepth);

	Vector vnorm = Vector(g_filog32, 0, 0,0);
	p = cgGetNamedParameter(prog, "g_fZNorm");
	if( p != NULL && cgIsParameterUsed(p, prog) == CG_TRUE )
		cgGLSetParameter4fv(p, vnorm);

	p = cgGetNamedParameter(prog, "g_fBilinear");
	if( p != NULL && cgIsParameterUsed(p, prog) == CG_TRUE )
		cgGLSetParameter4fv(p, Vector(-0.2f, -0.65f, 0.9f, 1.0f / 32767.0f ));

	p = cgGetNamedParameter(prog, "g_fZBias");
	if( p != NULL && cgIsParameterUsed(p, prog) == CG_TRUE )
		cgGLSetParameter4fv(p, Vector(1.0f/256.0f, 1.0004f, 1, 0.5f));

	p = cgGetNamedParameter(prog, "g_fc0");
	if( p != NULL && cgIsParameterUsed(p, prog) == CG_TRUE )
		cgGLSetParameter4fv(p, Vector(0,1, 0.001f, 0.5f));
}

#ifndef ZEROGS_DEVBUILD

#define LOAD_VS(Index, prog) {						  \
	assert( mapShaderResources.find(Index) != mapShaderResources.end() ); \
	header = mapShaderResources[Index]; \
	assert( (header) != NULL && (header)->index == (Index) ); \
	prog = cgCreateProgram(g_cgcontext, CG_OBJECT, (char*)(s_lpShaderResources + (header)->offset), cgvProf, NULL, NULL); \
	if( !cgIsProgram(prog) ) { \
		ERROR_LOG("Failed to load vs %d: \n%s\n", Index, cgGetLastListing(g_cgcontext)); \
		return false; \
	} \
	cgGLLoadProgram(prog); \
	if( cgGetError() != CG_NO_ERROR ) ERROR_LOG("failed to load program %d\n", Index); \
	SetupVertexProgramParameters(prog, !!(Index&SH_CONTEXT1));			\
} \

#define LOAD_PS(Index, fragment) {  \
	bLoadSuccess = true; \
	assert( mapShaderResources.find(Index) != mapShaderResources.end() ); \
	header = mapShaderResources[Index]; \
	fragment.prog = cgCreateProgram(g_cgcontext, CG_OBJECT, (char*)(s_lpShaderResources + (header)->offset), cgfProf, NULL, NULL); \
	if( !cgIsProgram(fragment.prog) ) { \
		ERROR_LOG("Failed to load ps %d: \n%s\n", Index, cgGetLastListing(g_cgcontext)); \
		return false; \
	} \
	cgGLLoadProgram(fragment.prog); \
	if( cgGetError() != CG_NO_ERROR ) { \
		ERROR_LOG("failed to load program %d\n", Index);		   \
		bLoadSuccess = false; \
	} \
	SetupFragmentProgramParameters(&fragment, !!(Index&SH_CONTEXT1), 0);  \
} \

bool ZeroGS::LoadEffects()
{
	assert( s_lpShaderResources != NULL );

	// process the header
	u32 num = *(u32*)s_lpShaderResources;
	int compressed_size = *(int*)(s_lpShaderResources+4);
	int real_size = *(int*)(s_lpShaderResources+8);
	int out;

	char* pbuffer = (char*)malloc(real_size);
	inf((char*)s_lpShaderResources+12, &pbuffer[0], compressed_size, real_size, &out);
	assert(out == real_size);

	s_lpShaderResources = (u8*)pbuffer;
	SHADERHEADER* header = (SHADERHEADER*)s_lpShaderResources;

	mapShaderResources.clear();
	while(num-- > 0 ) {
		mapShaderResources[header->index] = header;
		++header;
	}

	// clear the textures
	for(int i = 0; i < ARRAY_SIZE(ppsTexture); ++i) {
		SAFE_RELEASE_PROG(ppsTexture[i].prog);
		ppsTexture[i].prog = NULL;
	}
#ifndef _DEBUG
	memset(ppsTexture, 0, sizeof(ppsTexture));
#endif

	return true;
}

// called
bool ZeroGS::LoadExtraEffects()
{
	SHADERHEADER* header;
	bool bLoadSuccess = true;

	const int vsshaders[4] = { SH_REGULARVS, SH_TEXTUREVS, SH_REGULARFOGVS, SH_TEXTUREFOGVS };

	for(int i = 0; i < 4; ++i) {
		LOAD_VS(vsshaders[i], pvs[2*i]);
		LOAD_VS(vsshaders[i]|SH_CONTEXT1, pvs[2*i+1]);
		//if( conf.mrtdepth ) {
			LOAD_VS(vsshaders[i]|SH_WRITEDEPTH, pvs[2*i+8]);
			LOAD_VS(vsshaders[i]|SH_WRITEDEPTH|SH_CONTEXT1, pvs[2*i+8+1]);
//		}
//		else {
//			pvs[2*i+8] = pvs[2*i+8+1] = NULL;
//		}
	}
	
	LOAD_VS(SH_BITBLTVS, pvsBitBlt.prog);
	pvsBitBlt.sBitBltPos = cgGetNamedParameter(pvsBitBlt.prog, "g_fBitBltPos");
	pvsBitBlt.sBitBltTex = cgGetNamedParameter(pvsBitBlt.prog, "g_fBitBltTex");
	pvsBitBlt.fBitBltTrans = cgGetNamedParameter(pvsBitBlt.prog, "g_fBitBltTrans");

	LOAD_PS(SH_REGULARPS, ppsRegular[0]);
	LOAD_PS(SH_REGULARFOGPS, ppsRegular[1]);

	if( conf.mrtdepth ) {
		LOAD_PS(SH_REGULARPS, ppsRegular[2]);
		if( !bLoadSuccess )
			conf.mrtdepth = 0;
		LOAD_PS(SH_REGULARFOGPS, ppsRegular[3]);
		if( !bLoadSuccess )
			conf.mrtdepth = 0;
	}

	LOAD_PS(SH_BITBLTPS, ppsBitBlt[0]);
	LOAD_PS(SH_BITBLTAAPS, ppsBitBlt[1]);
	if( !bLoadSuccess ) {
		ERROR_LOG("Failed to load BitBltAAPS, using BitBltPS\n");
		LOAD_PS(SH_BITBLTPS, ppsBitBlt[1]);
	}
	LOAD_PS(SH_BITBLTDEPTHPS, ppsBitBltDepth);
	LOAD_PS(SH_CRTCTARGPS, ppsCRTCTarg[0]);
	LOAD_PS(SH_CRTCTARGINTERPS, ppsCRTCTarg[1]);
	
	g_bCRTCBilinear = TRUE;
	LOAD_PS(SH_CRTCPS, ppsCRTC[0]);
	if( !bLoadSuccess ) {
		// switch to simpler
		g_bCRTCBilinear = FALSE;
		LOAD_PS(SH_CRTC_NEARESTPS, ppsCRTC[0]);
		LOAD_PS(SH_CRTCINTER_NEARESTPS, ppsCRTC[0]);
	}
	else {
		LOAD_PS(SH_CRTCINTERPS, ppsCRTC[1]);
	}

	if( !bLoadSuccess )
		ERROR_LOG("Failed to create CRTC shaders\n");
	
	LOAD_PS(SH_CRTC24PS, ppsCRTC24[0]);
	LOAD_PS(SH_CRTC24INTERPS, ppsCRTC24[1]);
	LOAD_PS(SH_ZEROPS, ppsOne);
	LOAD_PS(SH_BASETEXTUREPS, ppsBaseTexture);
	LOAD_PS(SH_CONVERT16TO32PS, ppsConvert16to32);
	LOAD_PS(SH_CONVERT32TO16PS, ppsConvert32to16);

	return true;
}

FRAGMENTSHADER* ZeroGS::LoadShadeEffect(int type, int texfilter, int fog, int testaem, int exactcolor, const clampInfo& clamp, int context, bool* pbFailed)
{
	int texwrap;
	assert( texfilter < NUM_FILTERS );

	if(g_nPixelShaderVer&SHADER_REDUCED)
		texfilter = 0;
	assert(!(g_nPixelShaderVer&SHADER_REDUCED) || !exactcolor);

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
	
	assert( index < ARRAY_SIZE(ppsTexture) );
	FRAGMENTSHADER* pf = ppsTexture+index;
	
	if( pbFailed != NULL ) *pbFailed = false;

	if( pf->prog != NULL ) 
		return pf;

	if( (g_nPixelShaderVer & SHADER_ACCURATE) && mapShaderResources.find(index+NUM_SHADERS*SHADER_ACCURATE) != mapShaderResources.end() )
		index += NUM_SHADERS*SHADER_ACCURATE;

	assert( mapShaderResources.find(index) != mapShaderResources.end() );
	SHADERHEADER* header = mapShaderResources[index];
	if( header == NULL )
		ERROR_LOG("%d %d\n", index, g_nPixelShaderVer);
	assert( header != NULL );

	//DEBUG_LOG("shader:\n%s\n", (char*)(s_lpShaderResources + (header)->offset));
	pf->prog = cgCreateProgram(g_cgcontext, CG_OBJECT, (char*)(s_lpShaderResources + (header)->offset), cgfProf, NULL, NULL);
	if( pf->prog != NULL && cgIsProgram(pf->prog) && cgGetError() == CG_NO_ERROR ) {
		SetupFragmentProgramParameters(pf, context, type);
		cgGLLoadProgram(pf->prog);
		if( cgGetError() != CG_NO_ERROR ) {
//		  cgGLLoadProgram(pf->prog);
//		  if( cgGetError() != CG_NO_ERROR ) {
				ERROR_LOG("Failed to load shader %d,%d,%d,%d\n", type, fog, texfilter, 4*clamp.wms+clamp.wmt);
				if( pbFailed != NULL ) *pbFailed = true;
				return pf;
//		  }
		}
		return pf;
	}

	ERROR_LOG("Failed to create shader %d,%d,%d,%d\n", type, fog, texfilter, 4*clamp.wms+clamp.wmt);
	if( pbFailed != NULL ) *pbFailed = true;

	return NULL;
}
	
#else // ZEROGS_DEVBUILD

#define LOAD_VS(name, prog, shaderver) { \
	prog = cgCreateProgramFromFile(g_cgcontext, CG_SOURCE, EFFECT_NAME, shaderver, name, args); \
	if( !cgIsProgram(prog) ) { \
		ERROR_LOG("Failed to load vs %s: \n%s\n", name, cgGetLastListing(g_cgcontext)); \
		return false; \
	} \
	cgGLLoadProgram(prog); \
	if( cgGetError() != CG_NO_ERROR ) ERROR_LOG("failed to load program %s\n", name); \
	SetupVertexProgramParameters(prog, args[0]==context1); \
} \

#ifdef _DEBUG
#define SET_PSFILENAME(frag, name) frag.filename = name
#else
#define SET_PSFILENAME(frag, name)
#endif

#define LOAD_PS(name, fragment, shaderver) { \
	bLoadSuccess = true; \
	fragment.prog = cgCreateProgramFromFile(g_cgcontext, CG_SOURCE, EFFECT_NAME, shaderver, name, args); \
	if( !cgIsProgram(fragment.prog) ) { \
		ERROR_LOG("Failed to load ps %s: \n%s\n", name, cgGetLastListing(g_cgcontext)); \
		return false; \
	} \
	cgGLLoadProgram(fragment.prog); \
	if( cgGetError() != CG_NO_ERROR ) { \
		ERROR_LOG("failed to load program %s\n", name);		   \
		bLoadSuccess = false; \
	} \
	SetupFragmentProgramParameters(&fragment, args[0]==context1, 0);  \
	SET_PSFILENAME(fragment, name); \
} \

bool ZeroGS::LoadEffects()
{
	// clear the textures
	for(int i = 0; i < ARRAY_SIZE(ppsTexture); ++i) {
		SAFE_RELEASE_PROG(ppsTexture[i].prog);
	}

#ifndef _DEBUG
	memset(ppsTexture, 0, sizeof(ppsTexture));
#endif

	return true;
}

bool ZeroGS::LoadExtraEffects()
{
	const char* args[] = { NULL , NULL, NULL, NULL };
	char context0[255], context1[255];
	sprintf(context0, "-I%sctx0", EFFECT_DIR);
	sprintf(context1, "-I%sctx1", EFFECT_DIR);
	char* write_depth = "-DWRITE_DEPTH";
	bool bLoadSuccess = true;

	const char* pvsshaders[4] = { "RegularVS", "TextureVS", "RegularFogVS", "TextureFogVS" };

	for(int i = 0; i < 4; ++i) {
		args[0] = context0;
		args[1] = NULL;
		LOAD_VS(pvsshaders[i], pvs[2*i], cgvProf);
		args[0] = context1;
		LOAD_VS(pvsshaders[i], pvs[2*i+1], cgvProf);

		//if( conf.mrtdepth ) {
			args[0] = context0;
			args[1] = write_depth;
			LOAD_VS(pvsshaders[i], pvs[2*i+8], cgvProf);
			args[0] = context1;
			LOAD_VS(pvsshaders[i], pvs[2*i+8+1], cgvProf);
//		}
//		else {
//			pvs[2*i+8] = pvs[2*i+8+1] = NULL;
//		}
	}

	args[0] = context0;
	args[1] = NULL;
	LOAD_VS("BitBltVS", pvsBitBlt.prog, cgvProf);
	pvsBitBlt.sBitBltPos = cgGetNamedParameter(pvsBitBlt.prog, "g_fBitBltPos");
	pvsBitBlt.sBitBltTex = cgGetNamedParameter(pvsBitBlt.prog, "g_fBitBltTex");
	pvsBitBlt.fBitBltTrans = cgGetNamedParameter(pvsBitBlt.prog, "g_fBitBltTrans");

	LOAD_PS("RegularPS", ppsRegular[0], cgfProf);
	LOAD_PS("RegularFogPS", ppsRegular[1], cgfProf);

	if( conf.mrtdepth ) {
		args[0] = context0;
		args[1] = write_depth;
		LOAD_PS("RegularPS", ppsRegular[2], cgfProf);
		if( !bLoadSuccess )
			conf.mrtdepth = 0;
		LOAD_PS("RegularFogPS", ppsRegular[3], cgfProf);
		if( !bLoadSuccess )
			conf.mrtdepth = 0;
	}

	LOAD_PS("BitBltPS", ppsBitBlt[0], cgfProf);
	LOAD_PS("BitBltAAPS", ppsBitBlt[1], cgfProf);
	if( !bLoadSuccess ) {
		ERROR_LOG("Failed to load BitBltAAPS, using BitBltPS\n");
		LOAD_PS("BitBltPS", ppsBitBlt[1], cgfProf);
	}

	LOAD_PS("BitBltDepthPS", ppsBitBltDepth, cgfProf);
	LOAD_PS("CRTCTargPS", ppsCRTCTarg[0], cgfProf); LOAD_PS("CRTCTargInterPS", ppsCRTCTarg[1], cgfProf);
	
	g_bCRTCBilinear = TRUE;
	LOAD_PS("CRTCPS", ppsCRTC[0], cgfProf);
	if( !bLoadSuccess ) {
		// switch to simpler
		g_bCRTCBilinear = FALSE;
		LOAD_PS("CRTCPS_Nearest", ppsCRTC[0], cgfProf);
		LOAD_PS("CRTCInterPS_Nearest", ppsCRTC[0], cgfProf);
	}
	else {
		LOAD_PS("CRTCInterPS", ppsCRTC[1], cgfProf);
	}

	if( !bLoadSuccess )
		ERROR_LOG("Failed to create CRTC shaders\n");
	
	LOAD_PS("CRTC24PS", ppsCRTC24[0], cgfProf); LOAD_PS("CRTC24InterPS", ppsCRTC24[1], cgfProf);
	LOAD_PS("ZeroPS", ppsOne, cgfProf);
	LOAD_PS("BaseTexturePS", ppsBaseTexture, cgfProf);
	LOAD_PS("Convert16to32PS", ppsConvert16to32, cgfProf);
	LOAD_PS("Convert32to16PS", ppsConvert32to16, cgfProf);

//	if( !conf.mrtdepth ) {
//		ERROR_LOG("Disabling MRT depth writing\n");
//		s_bWriteDepth = FALSE;
//	}

	return true;
}

FRAGMENTSHADER* ZeroGS::LoadShadeEffect(int type, int texfilter, int fog, int testaem, int exactcolor, const clampInfo& clamp, int context, bool* pbFailed)
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

	if( pbFailed != NULL ) *pbFailed = false;

	FRAGMENTSHADER* pf = ppsTexture+index;

	if( pf->prog != NULL ) 
		return pf;
	
	pf->prog = LoadShaderFromType(EFFECT_DIR, EFFECT_NAME, type, texfilter, texwrap, fog, s_bWriteDepth, testaem, exactcolor, g_nPixelShaderVer, context);

	if( pf->prog != NULL ) {
#ifdef _DEBUG
		char str[255];
		sprintf(str, "Texture%s%d_%sPS", fog?"Fog":"", texfilter, g_pTexTypes[type]);
		pf->filename = str;
#endif
		SetupFragmentProgramParameters(pf, context, type);
		cgGLLoadProgram(pf->prog);
		if( cgGetError() != CG_NO_ERROR ) {
			// try again
//			cgGLLoadProgram(pf->prog);
//			if( cgGetError() != CG_NO_ERROR ) {
				ERROR_LOG("Failed to load shader %d,%d,%d,%d\n", type, fog, texfilter, 4*clamp.wms+clamp.wmt);
				if( pbFailed != NULL ) *pbFailed = true;
				//assert(0);
				// NULL makes things crash
				return pf;
//			}
		}
		return pf;
	}

	ERROR_LOG("Failed to create shader %d,%d,%d,%d\n", type, fog, texfilter, 4*clamp.wms+clamp.wmt);
	if( pbFailed != NULL ) *pbFailed = true;

	return NULL;
}

#endif // ZEROGS_DEVBUILD

void ZeroGS::Prim()
{
	if( g_bIsLost )
		return;

	VB& curvb = vb[prim->ctxt];
	if( curvb.CheckPrim() )
		Flush(prim->ctxt);
	curvb.curprim._val = prim->_val;

	// flush the other pipe if sharing the same buffer
//  if( vb[prim->ctxt].gsfb.fbp == vb[!prim->ctxt].gsfb.fbp && vb[!prim->ctxt].nCount > 0 )
//  {
//	  assert( vb[prim->ctxt].nCount == 0 );
//	  Flush(!prim->ctxt);
//  }

	curvb.curprim.prim = prim->prim;
}

int GetTexFilter(const tex1Info& tex1)
{
	// always force
	if( conf.bilinear == 2 )
		return 1;

	int texfilter = 0;
	if( conf.bilinear && ptexBilinearBlocks != 0 ) {
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
#ifdef ZEROGS_DEVBUILD
	for(int i = 0; i < ARRAY_SIZE(ppsTexture); ++i) {
		SAFE_RELEASE_PROG(ppsTexture[i].prog);
	}
	memset(ppsTexture, 0, sizeof(ppsTexture));
	LoadExtraEffects();
#endif
}

static int s_ClutResolve = 0;
void ZeroGS::Flush(int context)
{
	GL_REPORT_ERRORD();
	assert( context >= 0 && context <= 1 );

#ifdef ZEROGS_DEVBUILD
	if( g_bUpdateEffect ) {
		ReloadEffects();
		g_bUpdateEffect = 0;
	}
#endif

	VB& curvb = vb[context];
	const pixTest curtest = curvb.test;
	if( curvb.nCount == 0 || (curtest.zte && curtest.ztst == 0) || g_bIsLost ) {
		curvb.nCount = 0;
		return;
	}

	if( s_RangeMngr.ranges.size() > 0 ) {

		// don't want infinite loop
		u32 prevcount = curvb.nCount;
		curvb.nCount = 0;
		FlushTransferRanges(curvb.curprim.tme ? &curvb.tex0 : NULL);

		curvb.nCount = prevcount;
		//if( curvb.nCount == 0 )
		//  return;
	}

	if( curvb.bNeedTexCheck ) {
		curvb.FlushTexData();

		if( curvb.nCount == 0 )
			return;
	}

	DVProfileFunc _pf("Flush");

	GL_REPORT_ERRORD();
	if( curvb.bNeedFrameCheck || curvb.bNeedZCheck ) {
		curvb.CheckFrame(curvb.curprim.tme ? curvb.tex0.tbp0 : 0);
	}

	if( curvb.prndr == NULL || curvb.pdepth == NULL ) {
		WARN_LOG("Current render target NULL (ctx: %d)", context);
		curvb.nCount = 0;
		return;
	}

#if defined(PRIM_LOG) && defined(_DEBUG)
	static const char* patst[8] = { "NEVER", "ALWAYS", "LESS", "LEQUAL", "EQUAL", "GEQUAL", "GREATER", "NOTEQUAL"};
	static const char* pztst[4] = { "NEVER", "ALWAYS", "GEQUAL", "GREATER" };
	static const char* pafail[4] = { "KEEP", "FB_ONLY", "ZB_ONLY", "RGB_ONLY" };
	PRIM_LOG("**Drawing ctx %d, num %d, fbp: 0x%x, zbp: 0x%x, fpsm: %d, zpsm: %d, fbw: %d\n", context, vb[context].nCount, curvb.prndr->fbp, curvb.zbuf.zbp, curvb.prndr->psm, curvb.zbuf.psm, curvb.prndr->fbw);
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
	
	GL_REPORT_ERRORD();
	CRenderTarget* ptextarg = NULL;

	if( curtest.date || gs.pabe )
		SetDestAlphaTest();

	// set the correct pixel shaders
	if( curvb.curprim.tme ) {

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
			if( PSMT_ISCLUT(tpsm) && tpsm != PSMT8H ) { // handle 8h cluts
				// don't support clut targets, read from mem
				// 4hl - kh2 check - from dx version -- arcum42
				if( tpsm != PSMT4HL && tpsm != PSMT4HH && s_ClutResolve <= 1 ) 
				{ // xenosaga requires 2 resolves
					u32 prevcount = curvb.nCount;
					curvb.nCount = 0;
					ptextarg->Resolve();
					s_ClutResolve++;
					curvb.nCount = prevcount;
				}
				ptextarg = NULL;
			}
			else {
				if( ptextarg == curvb.prndr ) {
					// need feedback
					curvb.prndr->CreateFeedback();

					if(s_bWriteDepth && curvb.pdepth != NULL)
						curvb.pdepth->SetRenderTarget(1);
					else
						ResetRenderTarget(1);
				}
			}
		}
		else ptextarg = NULL;
	}

#ifdef _DEBUG
	if( g_bSaveFlushedFrame & 0x80000000 ) {
		char str[255];
		sprintf(str, "rndr.tga", g_SaveFrameNum);
		SaveRenderTarget(str, curvb.prndr->fbw, curvb.prndr->fbh, 0);
	}
#endif

	if( conf.options & GSOPTION_WIREFRAME ) {
		// always render first few geometry as solid
		if( s_nWireframeCount > 0 ) {
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
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

	icurctx = context;

	DVProfileFunc _pf5("Flush:after texvars");

	glBindBuffer(GL_ARRAY_BUFFER, g_vboBuffers[g_nCurVBOIndex]);
	g_nCurVBOIndex = (g_nCurVBOIndex+1)%g_vboBuffers.size();
	glBufferData(GL_ARRAY_BUFFER, curvb.nCount * sizeof(VertexGPU), curvb.pBufferData, GL_STREAM_DRAW);
//	void* pdata = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
//	memcpy_amd(pdata, curvb.pBufferData, curvb.nCount * sizeof(VertexGPU));
//	glUnmapBuffer(GL_ARRAY_BUFFER);
	SET_STREAM();

	assert( !(curvb.prndr->status&CRenderTarget::TS_NeedUpdate) );
	curvb.prndr->status = 0;
	
	if( curvb.pdepth != NULL ) {
		assert( !(curvb.pdepth->status&CRenderTarget::TS_NeedUpdate) );
		if( !curvb.zbuf.zmsk ) {
			assert( !(curvb.pdepth->status & CRenderTarget::TS_Virtual) );
			curvb.pdepth->status = 0;
		}
	}

#ifdef _DEBUG
	//curvb.prndr->SetRenderTarget(0);
	//curvb.pdepth->SetDepthStencilSurface();
	//glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT1_EXT, GL_TEXTURE_RECTANGLE_NV, 0, 0 );
	GLenum status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
	assert( glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT) == GL_FRAMEBUFFER_COMPLETE_EXT );
#endif
	s_dwColorWrite = (curvb.prndr->psm&0xf) == 1 ? (COLORMASK_BLUE|COLORMASK_GREEN|COLORMASK_RED) : 0xf;

	if( ((curvb.frame.fbm)&0xff) == 0xff) s_dwColorWrite &= ~COLORMASK_RED;
	if( ((curvb.frame.fbm>>8)&0xff) == 0xff) s_dwColorWrite &= ~COLORMASK_GREEN;
	if( ((curvb.frame.fbm>>16)&0xff) == 0xff) s_dwColorWrite &= ~COLORMASK_BLUE;
	if( ((curvb.frame.fbm>>24)&0xff) == 0xff) s_dwColorWrite &= ~COLORMASK_ALPHA;

	GL_COLORMASK(s_dwColorWrite);

	Rect& scissor = curvb.prndr->scissorrect;
	glScissor(scissor.x, scissor.y, scissor.w, scissor.h);

	u32 dwUsingSpecialTesting = 0;
	u32 dwFilterOpts = 0;

	FRAGMENTSHADER* pfragment = NULL;

	// need exact if equal or notequal
	int exactcolor = 0;
	if( !(g_nPixelShaderVer&SHADER_REDUCED) )
		// ffx2 breaks when ==7
		exactcolor = (curtest.ate && curtest.aref <= 128) && (curtest.atst==4);//||curtest.atst==7);

	int shadertype = 0;

	// set the correct pixel shaders
	u32 ptexclut = 0;
	if( curvb.curprim.tme ) {

		if( ptextarg != NULL ) {

			if( ptextarg->IsDepth() )
				SetWriteDepth();

			Vector vpageoffset;
			vpageoffset.w = 0;

			int psm = curvb.tex0.psm;
			if( PSMT_ISCLUT(curvb.tex0.psm) ) psm = curvb.tex0.cpsm;

			shadertype = 1;
			if( curvb.tex0.psm == PSMT8H && !(g_GameSettings&GAME_NOTARGETCLUT) ) {
				// load the clut to memory
				glGenTextures(1, &ptexclut);
				glBindTexture(GL_TEXTURE_2D, ptexclut);
				vector<char> data((curvb.tex0.cpsm&2) ? 512 : 1024);
				
				if( ptexclut != 0 ) {

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
						memcpy_amd(&data[0], g_pbyGSClut+nClutOffset, clutsize);
					}
					else {
						u16* pClutBuffer = (u16*)(g_pbyGSClut + nClutOffset);
						u16* pclut = (u16*)&data[0];
						int left = ((u32)nClutOffset & 2) ? 0 : ((nClutOffset&0x3ff)/2)+clutsize-512;
						if( left > 0 ) clutsize -= left;

						while(clutsize > 0) {
							pclut[0] = pClutBuffer[0];
							pclut++;
							pClutBuffer+=2;
							clutsize -= 2;
						}

						if( left > 0) {
							pClutBuffer = (u16*)(g_pbyGSClut + 2);
							while(left > 0) {
								pclut[0] = pClutBuffer[0];
								left -= 2;
								pClutBuffer += 2;
								pclut++;
							}
						}
					}

					glTexImage2D(GL_TEXTURE_2D, 0, 4, 256, 1, 0, GL_RGBA, (curvb.tex0.cpsm&2)?GL_UNSIGNED_SHORT_5_5_5_1:GL_UNSIGNED_BYTE, &data[0]);
					s_vecTempTextures.push_back(ptexclut);
					
					if( g_bSaveTex )
						SaveTexture("clut.tga", GL_TEXTURE_2D, ptexclut, 256, 1);

					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				}

				if( !(g_nPixelShaderVer&SHADER_REDUCED) && (ptextarg->psm & 2) ) {
					shadertype = 4;
				}
				else
					shadertype = 2;
			}
			else {
				if( PSMT_ISCLUT(curvb.tex0.psm) )
					WARN_LOG("Using render target with CLUTs %d!\n", curvb.tex0.psm);
				else {
					if( (curvb.tex0.psm&2) != (ptextarg->psm&2) && (!(g_nPixelShaderVer&SHADER_REDUCED) || !curvb.curprim.fge) ) {
						if( curvb.tex0.psm & 2 ) {
							// converting from 32->16
							shadertype = 3;
						}
						else {
							// converting from 16->32
							WARN_LOG("ZeroGS: converting from 16 to 32bit RTs\n");
							//shadetype = 4;
						}
					}
				}
			}

			pfragment = LoadShadeEffect(shadertype, 0, curvb.curprim.fge, curvb.tex0.tcc && gs.texa.aem && (psm == PSMCT24 || psm == PSMCT16 || psm == PSMCT16S),
				exactcolor, curvb.clamp, context, NULL);

			if( shadertype == 3 ) {
				Vector v;
				v.x = 16.0f / (float)curvb.tex0.tw;
				v.y = 16.0f / (float)curvb.tex0.th;
				v.z = 0.5f * v.x;
				v.w = 0.5f * v.y;
				cgGLSetParameter4fv(pfragment->fTexOffset, v);

				vpageoffset.x = -0.1f / 256.0f;
				vpageoffset.y = -0.001f / 256.0f;
				vpageoffset.z = -0.1f / ptextarg->fbh;
				vpageoffset.w = ((ptextarg->psm&0x30)==0x30)?-1.0f:0.0f;
			}
			else if( shadertype == 4 ) {
				Vector v;
				v.x = 16.0f / (float)ptextarg->fbw;
				v.y = 16.0f / (float)ptextarg->fbh;
				v.z = -1;
				v.w = 8.0f / (float)ptextarg->fbh;
				cgGLSetParameter4fv(pfragment->fTexOffset, v);

				vpageoffset.x = 2;
				vpageoffset.y = 1;
				vpageoffset.z = 0;
				vpageoffset.w = 0.0001f;
			}

			u32 ptexset = ptextarg == curvb.prndr ? ptextarg->ptexFeedback : ptextarg->ptex;
			s_ptexCurSet[context] = ptexset;

			if( !curvb.tex1.mmag ) {
				glBindTexture(GL_TEXTURE_RECTANGLE_NV, ptexset);
				glTexParameteri(GL_TEXTURE_RECTANGLE_NV, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
				dwFilterOpts |= 1;
			}
			
			if( !curvb.tex1.mmin ) {
				if( curvb.tex1.mmag )
					glBindTexture(GL_TEXTURE_RECTANGLE_NV, ptexset);
				glTexParameteri(GL_TEXTURE_RECTANGLE_NV, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
				dwFilterOpts |= 2;
			}
			
			Vector vTexDims;
			vTexDims.x = curvb.tex0.tw << s_AAx ;
			vTexDims.y = curvb.tex0.th << s_AAy ;


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
				vTexDims.z = (float)xcoord;
				vTexDims.w = (float)ycoord;
			}

			if( shadertype == 4 ) vTexDims.z += 8.0f;
			

			cgGLSetParameter4fv(pfragment->fTexDims, vTexDims);

			// zoe2
			if( (ptextarg->psm&0x30) == 0x30 ) {//&& (psm&2) == (ptextarg->psm&2) ) {
				// target of zbuf has +1 added to it, don't do 16bit
				vpageoffset.w = -1;
//			  Vector valpha2;
//			  valpha2.x = 1; valpha2.y = 0;
//			  valpha2.z = -1; valpha2.w = 0;
//			  SETCONSTF(GPU_TEXALPHA20+context, &valpha2);
			}
			cgGLSetParameter4fv(pfragment->fPageOffset, vpageoffset);

			if( g_bSaveTex )
				SaveTexture("tex.tga", GL_TEXTURE_RECTANGLE_NV,
					ptextarg == curvb.prndr ? ptextarg->ptexFeedback : ptextarg->ptex, ptextarg->fbw<<s_AAx, ptextarg->fbh<<s_AAy);
		}
		else {
			// save the texture
#ifdef _DEBUG
//		  CMemoryTarget* pmemtarg = g_MemTargs.GetMemoryTarget(curvb.tex0, 0);
//		  assert( curvb.pmemtarg == pmemtarg );
//		  if( PSMT_ISCLUT(curvb.tex0.psm) )
//			  assert( curvb.pmemtarg->ValidateClut(curvb.tex0) );
#endif

//#ifdef ZEROGS_CACHEDCLEAR
//		  if( !curvb.pmemtarg->ValidateTex(curvb.tex0, true) ) {
//			  CMemoryTarget* pmemtarg = g_MemTargs.GetMemoryTarget(curvb.tex0, 1);
//			  SetTexVariablesInt(context, GetTexFilter(curvb.tex1), curvb.tex0, pmemtarg, s_bForceTexFlush);
//			  vb[context].bVarsTexSync = TRUE;
//		  }
//#endif

			if( g_bSaveTex ) {
				if( g_bSaveTex == 1 ) {
					SaveTex(&curvb.tex0, 1);
//					CMemoryTarget* pmemtarg = g_MemTargs.GetMemoryTarget(curvb.tex0, 0);
//					int texheight = (pmemtarg->realheight+pmemtarg->widthmult-1)/pmemtarg->widthmult;
//					int texwidth = GPU_TEXWIDTH*pmemtarg->widthmult*pmemtarg->channels;
//					SaveTexture("real.tga", GL_TEXTURE_RECTANGLE_NV, pmemtarg->ptex->tex, texwidth, texheight);
				}
				else SaveTex(&curvb.tex0, 0);
			}

			int psm = curvb.tex0.psm;
			if( PSMT_ISCLUT(curvb.tex0.psm) ) psm = curvb.tex0.cpsm;

			pfragment = LoadShadeEffect(0, GetTexFilter(curvb.tex1), curvb.curprim.fge,
				curvb.tex0.tcc && gs.texa.aem && (psm == PSMCT24 || psm == PSMCT16 || psm == PSMCT16S),
				exactcolor, curvb.clamp, context, NULL);
		}

		if( pfragment->prog != g_psprog ) {
			// programs changed, so reset the texture variables
			vb[context].bTexConstsSync = 0;
			vb[context].bVarsTexSync = 0;
		}

		SetTexVariables(context, pfragment, ptextarg == NULL);
	}
	else {
		pfragment = &ppsRegular[curvb.curprim.fge+2*s_bWriteDepth];
	}
	
	assert(pfragment != 0 );
	if( curvb.curprim.tme ) {
		// have to enable the texture parameters
		if( curvb.ptexClamp[0] != 0 ) {
			cgGLSetTextureParameter(pfragment->sBitwiseANDX, curvb.ptexClamp[0]);
			cgGLEnableTextureParameter(pfragment->sBitwiseANDX);
		}
		if( curvb.ptexClamp[1] != 0 ) {
			cgGLSetTextureParameter(pfragment->sBitwiseANDY, curvb.ptexClamp[1]);
			cgGLEnableTextureParameter(pfragment->sBitwiseANDY);
		}
		if( pfragment->sMemory != NULL && s_ptexCurSet[context] != 0) {
			cgGLSetTextureParameter(pfragment->sMemory, s_ptexCurSet[context]);
			cgGLEnableTextureParameter(pfragment->sMemory);
		}
	   if( pfragment->sCLUT != NULL && ptexclut != 0 ) {
			cgGLSetTextureParameter(pfragment->sCLUT, ptexclut);
			cgGLEnableTextureParameter(pfragment->sCLUT);
	   }
	}

	DVProfileFunc _pf4("Flush:before bind");
	GL_REPORT_ERRORD();

	// set the shaders
	SETVERTEXSHADER(pvs[2*((curvb.curprim._val>>1)&3)+8*s_bWriteDepth+context]);

	if( pfragment->prog != g_psprog ) {
		cgGLBindProgram(pfragment->prog);
		g_psprog = pfragment->prog;
	}

	GL_REPORT_ERRORD();

	DVProfileFunc _pf1("Flush:after bind");

	BOOL bCanRenderStencil = g_bUpdateStencil && (curvb.prndr->psm&0xf) != 1 && !(curvb.frame.fbm&0x80000000);
	if( g_GameSettings & GAME_NOSTENCIL )
		bCanRenderStencil = 0;

	if( s_bDestAlphaTest && bCanRenderStencil) {
		glEnable(GL_STENCIL_TEST);
		GL_STENCILFUNC(GL_ALWAYS, 0, 0);
		glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
	}
	else glDisable(GL_STENCIL_TEST);

	glDepthMask(!curvb.zbuf.zmsk);
	GL_ZTEST(curtest.zte);
	if( curtest.zte ) {
		if( curtest.ztst > 1 ) g_nDepthUsed = 2;
		if( (curtest.ztst == 2) ^ (g_nDepthBias != 0) ) {
			g_nDepthBias = curtest.ztst == 2;
			//SETRS(D3DRS_DEPTHBIAS, g_nDepthBias?FtoDW(0.0003f):FtoDW(0.000015f));
		}

		glDepthFunc(g_dwZCmp[curtest.ztst]);

//	  if( curtest.ztst == 3 ) {
//		  // gequal
//		  if( s_vznorm.y == 0 ) {
//			  s_vznorm.y = 0.00001f;
//			  SETCONSTF(GPU_ZNORM, s_vznorm);
//		  }
//	  }
//	  else {
//		  if( s_vznorm.y > 0 ) {
//			  s_vznorm.y = 0;
//			  SETCONSTF(GPU_ZNORM, s_vznorm);
//		  }
//	  }
	}

	GL_ALPHATEST(curtest.ate&&USEALPHATESTING);
	if( curtest.ate ) {
		glAlphaFunc(g_dwAlphaCmp[curtest.atst], b2XAlphaTest ? min(1.0f,(float)curtest.aref * (1/ 127.5f)) : curtest.aref*(1/255.0f));
	}

	if( s_bWriteDepth ) {
		if(!curvb.zbuf.zmsk)
			curvb.pdepth->SetRenderTarget(1);
		else
			ResetRenderTarget(1);
	}

	if( curvb.curprim.abe )
		SetAlphaVariables(curvb.alpha);
	else
		glDisable(GL_BLEND);

	// needs to be before RenderAlphaTest
	if( curvb.fba.fba || s_bDestAlphaTest ) {
		if( gs.pabe || (curvb.fba.fba || bCanRenderStencil) && !(curvb.frame.fbm&0x80000000) ) {
			RenderFBA(curvb, pfragment->sOneColor);
		}
	}

	u32 oldabe = curvb.curprim.abe;
	if( gs.pabe ) {
		//WARN_LOG("PBE!\n");
		curvb.curprim.abe = 1;
		glEnable(GL_BLEND);
	}

	if( curvb.curprim.abe ) {

		if( //bCanRenderStencil &&
			(bNeedBlendFactorInAlpha || ((curtest.ate && curtest.atst>1) && (curtest.aref > 0x80))) ) {
			// need special stencil processing for the alpha
			RenderAlphaTest(curvb, pfragment->sOneColor);
			dwUsingSpecialTesting = 1;
		}

		// harvest fishing
		Vector v = vAlphaBlendColor;// + Vector(0,0,0,(curvb.test.atst==4 && curvb.test.aref>=128)?-0.004f:0);
		if( exactcolor ) { v.y *= 255; v.w *= 255; }
		cgGLSetParameter4fv(pfragment->sOneColor, v);
	}
	else {
		// not using blending so set to defaults
		Vector v = exactcolor ? Vector(1, 510*255.0f/256.0f, 0, 0) : Vector(1,2*255.0f/256.0f,0,0);
		cgGLSetParameter4fv(pfragment->sOneColor, v);
	}

	if( s_bDestAlphaTest && bCanRenderStencil ) {
		// if not 24bit and can write to high alpha bit
		RenderStencil(curvb, dwUsingSpecialTesting);
	}
	else {
		s_stencilref = STENCIL_SPECIAL;
		s_stencilmask = STENCIL_SPECIAL;

		// setup the stencil to only accept the test pixels
		if( dwUsingSpecialTesting ) {
			glEnable(GL_STENCIL_TEST);
			glStencilMask(STENCIL_PIXELWRITE);
			GL_STENCILFUNC(GL_EQUAL, STENCIL_SPECIAL|STENCIL_PIXELWRITE, STENCIL_SPECIAL);
			glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
		}
	}

#ifdef _DEBUG
	if( bDestAlphaColor == 1 ) {
		WARN_LOG("dest alpha blending! manipulate alpha here\n");
	}
#endif

	if( bCanRenderStencil && gs.pabe ) {
		// only render the pixels with alpha values >= 0x80
		GL_STENCILFUNC(GL_EQUAL, s_stencilref|STENCIL_FBA, s_stencilmask|STENCIL_FBA);
	}

//	curvb.prndr->SetViewport();
//	pd3dDevice->SetScissorRect(&curvb.prndr->scissorrect);
//	glEnable(GL_SCISSOR_TEST);

	GL_REPORT_ERRORD();
	if( !curvb.test.ate || curvb.test.atst > 0 ) {
		DRAW();
	}
	GL_REPORT_ERRORD();

	if( gs.pabe ) {
		// only render the pixels with alpha values < 0x80
		glDisable(GL_BLEND);
		GL_STENCILFUNC_SET();

		Vector v;
		v.x = 1; v.y = 2; v.z = 0; v.w = 0;
		if( exactcolor ) v.y *= 255;
		cgGLSetParameter4fv(pfragment->sOneColor, v);

		DRAW();

		// reset
		if( !s_stencilmask ) s_stencilfunc = GL_ALWAYS;
		GL_STENCILFUNC_SET();
	}

	GL_REPORT_ERRORD();

	// more work on alpha failure case
	if( curtest.ate && curtest.atst != 1 && curtest.afail > 0 ) {

		// need to reverse the test and disable some targets
		glAlphaFunc(g_dwReverseAlphaCmp[curtest.atst], b2XAlphaTest ? min(1.0f,(float)curtest.aref * (1/ 127.5f)) : curtest.aref*(1/255.0f));

		if( curtest.afail & 1 ) {   // front buffer update only
			
			if( curtest.afail == 3 ) // disable alpha
				glColorMask(1,1,1,0);

			glDepthMask(0);

			if( s_bWriteDepth )
				ResetRenderTarget(1);
		}
		else {
			// zbuffer update only
			glColorMask(0,0,0,0);   
		}

		if( gs.pabe && bCanRenderStencil ) {
			// only render the pixels with alpha values >= 0x80
			Vector v = vAlphaBlendColor;
			if( exactcolor ) { v.y *= 255; v.w *= 255; }
			cgGLSetParameter4fv(pfragment->sOneColor, v);
			glEnable(GL_BLEND);
			GL_STENCILFUNC(GL_EQUAL, s_stencilref|STENCIL_FBA, s_stencilmask|STENCIL_FBA);
		}

//	  IDirect3DQuery9* pOcclusionQuery;
//	  u32 numberOfPixelsDrawn;
//
//	  pd3dDevice->CreateQuery(D3DQUERYTYPE_OCCLUSION, &pOcclusionQuery);
//
//	  // Add an end marker to the command buffer queue.
//	  pOcclusionQuery->Issue(D3DISSUE_BEGIN);

		DRAW();
		GL_REPORT_ERRORD();

//	  pOcclusionQuery->Issue(D3DISSUE_END);
		// Force the driver to execute the commands from the command buffer.
		// Empty the command buffer and wait until the GPU is idle.
//	  while(S_FALSE == pOcclusionQuery->GetData( &numberOfPixelsDrawn, sizeof(u32), D3DGETDATA_FLUSH ));
//	  SAFE_RELEASE(pOcclusionQuery);
	
		if( gs.pabe ) {
			// only render the pixels with alpha values < 0x80
			glDisable(GL_BLEND);
			GL_STENCILFUNC_SET();

			Vector v;
			v.x = 1; v.y = 2; v.z = 0; v.w = 0;
			if( exactcolor ) v.y *= 255;
			cgGLSetParameter4fv(pfragment->sOneColor, v);

			DRAW();

			// reset
			if( oldabe ) glEnable(GL_BLEND);

			if( !s_stencilmask ) s_stencilfunc = GL_ALWAYS;
			GL_STENCILFUNC_SET();
		}

		// restore
		
		if( (curtest.afail & 1) && !curvb.zbuf.zmsk ) {
			glDepthMask(1);

			if( s_bWriteDepth ) {
				assert( curvb.pdepth != NULL);
				curvb.pdepth->SetRenderTarget(1);
			}
		}

		GL_COLORMASK(s_dwColorWrite);

		// not needed anymore since rest of ops concentrate on image processing
	}

	GL_REPORT_ERRORD();

	if( dwUsingSpecialTesting ) {
		// render the real alpha
		glDisable(GL_ALPHA_TEST);
		glColorMask(0,0,0,1);

		if( s_bWriteDepth ) {
			ResetRenderTarget(1);
		}

		glDepthMask(0);

		glStencilFunc(GL_EQUAL, STENCIL_SPECIAL|STENCIL_PIXELWRITE, STENCIL_SPECIAL|STENCIL_PIXELWRITE);
		glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

		Vector v = Vector(0,exactcolor ? 510.0f : 2.0f,0,0);
		cgGLSetParameter4fv(pfragment->sOneColor, v);
		DRAW();

		// don't need to restore
	}

	GL_REPORT_ERRORD();

	if( s_bDestAlphaTest ) {
		if( (s_dwColorWrite&COLORMASK_ALPHA) ) {
			if( curvb.fba.fba )
				ProcessFBA(curvb, pfragment->sOneColor);
			else if( bCanRenderStencil )
				// finally make sure all entries are 1 when the dest alpha >= 0x80 (if fba is 1, this is already the case)
				ProcessStencil(curvb);
		}
	}
	else if( (s_dwColorWrite&COLORMASK_ALPHA) && curvb.fba.fba )
		ProcessFBA(curvb, pfragment->sOneColor);

	if( bDestAlphaColor == 1 ) {
		// need to reset the dest colors to their original counter parts
		//WARN_LOG("Need to reset dest alpha color\n");
	}

#ifdef _DEBUG
	if( g_bSaveFlushedFrame & 0xf ) {
#ifdef _WIN32
		CreateDirectory("frames", NULL);
#else
		mkdir("frames", 0755);
#endif
		char str[255];
		sprintf(str, "frames/frame%.4d.tga", g_SaveFrameNum++);
		if( (g_bSaveFlushedFrame & 2) ) {
			//glBindFramebufferEXT( GL_FRAMEBUFFER_EXT, 0 ); // switch to the backbuffer
			//glFlush();
			//SaveTexture("tex.jpg", GL_TEXTURE_RECTANGLE_NV, curvb.prndr->ptex, curvb.prndr->fbw<<s_AAx, curvb.prndr->fbh<<s_AAx);
			SaveRenderTarget(str, curvb.prndr->fbw<<s_AAx, curvb.prndr->fbh<<s_AAx, 0);
		}
	}
#endif

	GL_REPORT_ERRORD();

	// clamp the final colors, when enabled ffx2 credits mess up
	if( curvb.curprim.abe && bAlphaClamping && GetRenderFormat() != RFT_byte8 && !(g_GameSettings&GAME_NOCOLORCLAMP)) { // if !colclamp, skip

		ResetAlphaVariables();

		// if processing the clamping case, make sure can write to the front buffer
		glDisable(GL_STENCIL_TEST);
		glEnable(GL_BLEND);
		glDisable(GL_ALPHA_TEST);
		glDisable(GL_DEPTH_TEST);
		glDepthMask(0);
		glColorMask(1,1,1,0);
		
		if( s_bWriteDepth ) {
			ResetRenderTarget(1);
		}

		SETPIXELSHADER(ppsOne.prog);

		// (dest&0x7f)+0x80, blend factor for alpha is always 0x80
		GL_BLEND_RGB(GL_ONE, GL_ONE);

		float f;

		if( bAlphaClamping & 1 ) { // min
			f = 0;
			cgGLSetParameter4fv(ppsOne.sOneColor, &f);
			GL_BLENDEQ_RGB(GL_MAX_EXT);
			DRAW();
		}
		
		// bios shows white screen
		if( bAlphaClamping & 2 ) { // max
			f = 1;
			cgGLSetParameter4fv(ppsOne.sOneColor, &f);
			GL_BLENDEQ_RGB(GL_MIN_EXT);
			DRAW();
		}

		if( !curvb.zbuf.zmsk ) {
			glDepthMask(1);

			if( s_bWriteDepth ) {
				assert( curvb.pdepth != NULL );
				curvb.pdepth->SetRenderTarget(1);
			}
		}

		if( curvb.test.ate && USEALPHATESTING )
			glEnable(GL_ALPHA_TEST);

		GL_ZTEST(curtest.zte);
	}

	if( dwFilterOpts ) {
		// undo filter changes (binding didn't change)
		if( dwFilterOpts & 1 ) glTexParameteri(GL_TEXTURE_RECTANGLE_NV, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		if( dwFilterOpts & 2 ) glTexParameteri(GL_TEXTURE_RECTANGLE_NV, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	}

//#ifdef ZEROGS_DEVBUILD
	ppf += curvb.nCount+0x100000;
//#endif

	curvb.nCount = 0;
	curvb.curprim.abe = oldabe;
	//if( oldabe ) glEnable(GL_BLEND);

	if( conf.options & GSOPTION_WIREFRAME ) {
		// always render first few geometry as solid
		if( s_nWireframeCount > 0 ) {
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			--s_nWireframeCount;
		}
	}

	GL_REPORT_ERRORD();
}

void ZeroGS::ProcessMessages()
{
	if( listMsgs.size() > 0 ) {
		int left = 25, top = 15;
		list<MESSAGE>::iterator it = listMsgs.begin();
		
		while( it != listMsgs.end() ) {
			DrawText(it->str, left+1, top+1, 0xff000000);
			DrawText(it->str, left, top, 0xffffff30);
			top += 15;

			if( (int)(it->dwTimeStamp - timeGetTime()) < 0 )
				it = listMsgs.erase(it);
			else ++it;
		}
	}
}

void ZeroGS::RenderCustom(float fAlpha)
{
	GLenum err = GL_NO_ERROR;

	GL_REPORT_ERROR();

	fAlpha = 1;
	glBindFramebufferEXT( GL_FRAMEBUFFER_EXT, 0 ); // switch to the backbuffer

	glDisable(GL_STENCIL_TEST);
	glDisable(GL_DEPTH_TEST);
	glDepthMask(0);
	glColorMask(1,1,1,1);
	glDisable(GL_BLEND);
	glDisable(GL_ALPHA_TEST);
	glDisable(GL_SCISSOR_TEST);
	glViewport(0, 0, nBackbufferWidth, nBackbufferHeight);

	// play custom animation
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);

	// tex coords
	Vector v = Vector(1/32767.0f, 1/32767.0f, 0, 0);
	cgGLSetParameter4fv(pvsBitBlt.sBitBltPos, v);
	v.x = (float)nLogoWidth;
	v.y = (float)nLogoHeight;
	cgGLSetParameter4fv(pvsBitBlt.sBitBltPos, v);

	v.x = v.y = v.z = v.w = fAlpha;
	cgGLSetParameter4fv(ppsBaseTexture.sOneColor, v);

	if( conf.options & GSOPTION_WIREFRAME ) glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	// inside vb[0]'s target area, so render that region only
	cgGLSetTextureParameter(ppsBaseTexture.sFinal, ptexLogo);
	cgGLEnableTextureParameter(ppsBaseTexture.sFinal);

	glBindBuffer(GL_ARRAY_BUFFER, vboRect);
	SET_STREAM();

	SETVERTEXSHADER(pvsBitBlt.prog);
	SETPIXELSHADER(ppsBaseTexture.prog);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	// restore
	if( conf.options & GSOPTION_WIREFRAME ) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	ProcessMessages();

#ifdef _WIN32
		SwapBuffers(hDC);
#else
		GLWin.SwapBuffers();
#endif

	glEnable(GL_SCISSOR_TEST);
	glEnable(GL_STENCIL_TEST);

	vb[0].bSyncVars = 0;
	vb[1].bSyncVars = 0;

	GL_REPORT_ERROR();
	GLint status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
	assert( status == GL_FRAMEBUFFER_COMPLETE_EXT || status == GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT );
}

// adjusts trans to preserve aspect ratio
void ZeroGS::AdjustTransToAspect(Vector& v, int dispwidth, int dispheight)
{
	double temp;
	float f;
	if( dispwidth * nBackbufferHeight > dispheight * nBackbufferWidth) {
		// limited by width
		
		// change in ratio
		f = ((float)nBackbufferWidth / (float)dispwidth) / ((float)nBackbufferHeight / (float)dispheight);
		v.y *= f;
		v.w *= f;

		// scanlines mess up when not aligned right
		v.y += (1-(float)modf(v.y*(float)nBackbufferHeight*0.5f+0.05f, &temp))*2.0f/(float)nBackbufferHeight;
		v.w += (1-(float)modf(v.w*(float)nBackbufferHeight*0.5f+0.05f, &temp))*2.0f/(float)nBackbufferHeight;
	}
	else {
		// limited by height
		f = ((float)nBackbufferHeight / (float)dispheight) / ((float)nBackbufferWidth / (float)dispwidth);
		f -= (float)modf(f*nBackbufferWidth, &temp)/(float)nBackbufferWidth;
		v.x *= f;
		v.z *= f;
	}

//	v.y *= -1;
//	v.w *= -1;
	v *= 1/32767.0f;
}

void ZeroGS::Restore()
{
	if( !g_bIsLost )
		return;

	//if( SUCCEEDED(pd3dDevice->Reset(&d3dpp)) ) {
		g_bIsLost = 0;
		// handle lost states
		ZeroGS::ChangeDeviceSize(nBackbufferWidth, nBackbufferHeight);
	//}
}

void ZeroGS::RenderCRTC(int interlace)
{
	if( g_bIsLost ) return;

// Crashes Final Fantasy X at startup if uncommented. --arcum42
//#ifndef ZEROGS_DEVBUILD
//	if(g_nRealFrame < 80 ) {
//		RenderCustom( min(1.0f, 2.0f - (float)g_nRealFrame / 40.0f) );
//
//	  if( g_nRealFrame == 79 )
//		SAFE_RELEASE_TEX(ptexLogo);
//	  return;
//	}
//#endif

	if( conf.mrtdepth && pvs[8] == NULL ) {
		conf.mrtdepth = 0;
		s_bWriteDepth = FALSE;
		ERROR_LOG("Disabling MRT depth writing\n");
	}

	Flush(0);
	Flush(1);
	GL_REPORT_ERRORD();

	DVProfileFunc _pf("RenderCRTC");

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
		if( vb[0].prndr != NULL ) {
			SaveTexture("frame1.tga", GL_TEXTURE_RECTANGLE_NV, vb[0].prndr->ptex, vb[0].prndr->fbw<<s_AAx, vb[0].prndr->fbh<<s_AAy);
		}
		
		if( vb[1].prndr != NULL && vb[0].prndr != vb[1].prndr ) {
			SaveTexture("frame2.tga", GL_TEXTURE_RECTANGLE_NV, vb[1].prndr->ptex, vb[1].prndr->fbw<<s_AAx, vb[1].prndr->fbh<<s_AAy);
		}
#ifdef _WIN32
		else DeleteFile("frame2.tga");
#endif
	}

	if( s_RangeMngr.ranges.size() > 0 )
		FlushTransferRanges(NULL);

	//g_GameSettings |= GAME_VSSHACK|GAME_FULL16BITRES|GAME_NODEPTHRESOLVE;
	//s_bWriteDepth = TRUE;
	g_SaveFrameNum = 0;
	g_bSaveFlushedFrame = 1;

	// reset fba after every frame
	vb[0].fba.fba = 0;
	vb[1].fba.fba = 0;

//	static int counter = 0;
//	counter++;
	u32 bInterlace = SMODE2->INT && SMODE2->FFMD && (conf.interlace<2);
	glBindFramebufferEXT( GL_FRAMEBUFFER_EXT, 0 ); // switch to the backbuffer
	glViewport(0, 0, nBackbufferWidth, nBackbufferHeight);

	// if interlace, only clear every other vsync
	if(!bInterlace ) {
		u32 color = COLOR_ARGB(0, BGCOLOR->R, BGCOLOR->G, BGCOLOR->B);
		glClear(GL_COLOR_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);
	}

	SETVERTEXSHADER(pvsBitBlt.prog);
	glBindBuffer(GL_ARRAY_BUFFER, vboRect);
	SET_STREAM();
	GL_REPORT_ERRORD();

	if( conf.options & GSOPTION_WIREFRAME ) glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glDisable(GL_DEPTH_TEST);
	glDepthMask(0);
	glColorMask(1,1,1,1);
	glDisable(GL_BLEND);
	glDisable(GL_ALPHA_TEST);
	glDisable(GL_SCISSOR_TEST);
	glDisable(GL_STENCIL_TEST);

	GL_REPORT_ERRORD();

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
		
		dispinfo[i].tbp0 =  pfb->FBP << 5;
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
//  if( SMODE2->INT && SMODE2->FFMD && SMODE1->CMOD == 3 && dispwidth <= 320)
//	  dispwidth *= 2;

	// hack! makai
	//if( !bInterlace && dispheight * 2 < dispwidth ) dispheight *= 2;

	// start from the last circuit
	for(int i = !PMODE->SLBG; i >= 0; --i) {

		tex0Info& texframe = dispinfo[i];
		if( texframe.th <= 1 )
			continue;

		GSRegDISPFB* pfb = i ? DISPFB2 : DISPFB1;
		GSRegDISPLAY* pd = i ? DISPLAY2 : DISPLAY1;

		Vector v, valpha;
		u32 interlacetex = 0;

		if( bInterlace ) {

			texframe.th >>= 1;

			// interlace mode
			interlacetex = CreateInterlaceTex(2*texframe.th);
			
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
			glEnable(GL_BLEND);
			zgsBlendEquationSeparateEXT(GL_FUNC_ADD, GL_FUNC_ADD);

			if( PMODE->MMOD ) {
				glBlendColorEXT(PMODE->ALP*(1/255.0f), PMODE->ALP*(1/255.0f), PMODE->ALP*(1/255.0f), 0.5f);
				s_srcrgb = GL_CONSTANT_COLOR_EXT;
				s_dstrgb = GL_ONE_MINUS_CONSTANT_COLOR_EXT;
			}
			else {
				s_srcrgb = GL_SRC_ALPHA;
				s_dstrgb = GL_ONE_MINUS_SRC_ALPHA;
			}

			s_srcalpha = PMODE->AMOD ? GL_ZERO : GL_ONE;
			s_dstalpha = PMODE->AMOD? GL_ONE : GL_ZERO;
			zgsBlendFuncSeparateEXT(s_srcrgb, s_dstrgb, s_srcalpha, s_dstalpha);
		}

		if( bUsingStencil ) {
			glStencilMask(1<<i);
			s_stencilmask = 1<<i;
			GL_STENCILFUNC_SET();
		}

		if( texframe.psm == 0x12 ) {
			ERROR_LOG("CRTC24!!!\n");
			// assume that data is already in ptexMem (do Resolve?)
			SETPIXELSHADER(ppsCRTC24[bInterlace].prog);
			valpha.x = 0;
			valpha.y = 1;
			cgGLSetParameter4fv(ppsCRTC24[bInterlace].sOneColor, valpha);
			glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
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

				if( g_bSaveFinalFrame )
					SaveTexture("frame1.tga", GL_TEXTURE_RECTANGLE_NV, ptarg->ptex, ptarg->fbw<<s_AAx, ptarg->fbh<<s_AAy);

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
							glClear(GL_STENCIL_BUFFER_BIT);
						}
						bUsingStencil = 1;
						glEnable(GL_STENCIL_TEST);
						GL_STENCILFUNC(GL_NOTEQUAL, 3, 1<<i);
						glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
						glStencilMask(1<<i);
					}

					float fiw = 1.0f / texframe.tbw;
					float fih = 1.0f / ptarg->fbh;

					// tex coords
					v = Vector((float)(texframe.tw << s_AAx), (float)(dh << s_AAy), (float)(pfb->DBX << s_AAx), (float)(dby<<s_AAy)-0.5f);
					cgGLSetParameter4fv(pvsBitBlt.sBitBltTex, v);

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

					AdjustTransToAspect(v, 640, 480);
					cgGLSetParameter4fv(pvsBitBlt.sBitBltPos, v);

					cgGLSetParameter4fv(pvsBitBlt.fBitBltTrans, Vector(fih * 0.5f, fih * -0.5f, fih * 0.5f, fih * 0.5f));

					// use g_fInvTexDims to store inverse texture dims
					if( ppsCRTCTarg[bInterlace].sInvTexDims ) {
						v.x = fiw;
						v.y = fih;
						v.z = 0;
						cgGLSetParameter4fv(ppsCRTCTarg[bInterlace].sInvTexDims, v);
					}

					cgGLSetParameter4fv(ppsCRTCTarg[bInterlace].sOneColor, valpha);

					// inside vb[0]'s target area, so render that region only
					cgGLSetTextureParameter(ppsCRTCTarg[bInterlace].sFinal, ptarg->ptex);
					cgGLEnableTextureParameter(ppsCRTCTarg[bInterlace].sFinal);
					if( interlacetex != 0 ) {
						cgGLSetTextureParameter(ppsCRTCTarg[bInterlace].sInterlace, interlacetex);
						cgGLEnableTextureParameter(ppsCRTCTarg[bInterlace].sInterlace);
					}

					SETPIXELSHADER(ppsCRTCTarg[bInterlace].prog);
					GL_REPORT_ERRORD();

					glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

					//SaveRenderTarget("temp.tga", nBackbufferWidth, nBackbufferHeight, 0);

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
			CMemoryTarget* pmemtarg = g_MemTargs.GetMemoryTarget(texframe, 1);
			SetTexVariablesInt(0, g_bCRTCBilinear?2:0, texframe, pmemtarg, &ppsCRTC[bInterlace], 1);
			cgGLSetTextureParameter(ppsCRTC[bInterlace].sMemory, pmemtarg->ptex->tex);
			cgGLEnableTextureParameter(ppsCRTC[bInterlace].sMemory);

			if( g_bSaveFinalFrame )
				SaveTex(&texframe, g_bSaveFinalFrame-1>0);

			// finally render from the memory (note that the stencil buffer will keep previous regions)
			v = Vector(1,1,0,0);

			if (bInterlace && interlace == (conf.interlace)) {
				// move down by 1 pixel
				v.w += 1.0f / (float)texframe.th;
			}

			cgGLSetParameter4fv(ppsCRTC[bInterlace].sOneColor, valpha);

			AdjustTransToAspect(v, 640, 480);
			cgGLSetParameter4fv(pvsBitBlt.sBitBltPos, v);

			float fih = 1.0f / (float)texframe.th;
			if( g_bCRTCBilinear )
				cgGLSetParameter4fv(pvsBitBlt.sBitBltTex, Vector(texframe.tw,texframe.th,-0.5f,-0.5f));
			else
				cgGLSetParameter4fv(pvsBitBlt.sBitBltTex, Vector(1,1,-0.5f/(float)texframe.tw,-0.5f/(float)texframe.th));
			
			cgGLSetParameter4fv(pvsBitBlt.fBitBltTrans, Vector(fih * 0.5f, fih * -0.5f, fih * 0.5f, fih * 0.5f));
			
			// use g_fInvTexDims to store inverse texture dims
			if( ppsCRTC[bInterlace].sInvTexDims ) {
				v.x = 1.0f / (float)texframe.tw;
				v.y = fih;
				v.z = 0;//-0.5f * v.x;
				v.w = -0.5f * fih;
				cgGLSetParameter4fv(ppsCRTC[bInterlace].sInvTexDims, v);
			}
			
			if( interlacetex != 0 ) {
				cgGLSetTextureParameter(ppsCRTC[bInterlace].sInterlace, interlacetex);
				cgGLEnableTextureParameter(ppsCRTC[bInterlace].sInterlace);
			}

			SETPIXELSHADER(ppsCRTC[bInterlace].prog);
			glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		}
	}

	GL_REPORT_ERRORD();
	if(1) {// || !bInterlace) {
		glDisable(GL_BLEND);
		ProcessMessages();

		if( g_bMakeSnapshot ) {
			char str[64];
			int left = 200, top = 15;
			sprintf(str, "ZeroGS %d.%d.%d - %.1f fps %s", zgsrevision, zgsbuild, zgsminor, fFPS, s_frameskipping?" - frameskipping":"");

			DrawText(str, left+1, top+1, 0xff000000);
			DrawText(str, left, top, 0xffc0ffff);
		}

		if( g_bDisplayFPS ) {
			char str[64];
			int left = 10, top = 15;
			sprintf(str, "%.1f fps", fFPS);

			DrawText(str, left+1, top+1, 0xff000000);
			DrawText(str, left, top, 0xffc0ffff);
		}

		if (glGetError() != GL_NO_ERROR) DEBUG_LOG("glerror before swap!\n");
		

#ifdef _WIN32
		static u32 lastswaptime = 0;
		//if( timeGetTime() - lastswaptime > 14 ) {
			SwapBuffers(hDC);
			lastswaptime = timeGetTime();
		//}
#else
		GLWin.SwapBuffers();
#endif

//	  if( glGetError() != GL_NO_ERROR) {
//		  // device is lost, need to recreate
//		  ERROR_LOG("device lost\n");
//		  g_bIsLost = TRUE;
//		  Reset();
//		  return;
//	  }

		if( conf.options & GSOPTION_WIREFRAME ) {
			// clear all targets
			s_nWireframeCount = 1;
		}

		if( g_bMakeSnapshot ) {
			
			if( SaveRenderTarget(strSnapshot != ""?strSnapshot.c_str():"temp.jpg", nBackbufferWidth, -nBackbufferHeight, 0)) {//(conf.options&GSOPTION_TGASNAP)?0:1) ) {
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
//	  s_nCurFBPSet ^= 1;
//	  s_setFBP[s_nCurFBPSet].clear();
		//s_nCurFrameMap ^= 1;
		//s_mapFrameHeights[s_nCurFrameMap].clear();
	}

	// switch back to rendering to textures
	glBindFramebufferEXT( GL_FRAMEBUFFER_EXT, s_uFramebuffer );

	g_MemTargs.DestroyCleared();

	if( s_vecTempTextures.size() > 0 )
		glDeleteTextures((GLsizei)s_vecTempTextures.size(), &s_vecTempTextures[0]);
	s_vecTempTextures.clear();

	if( EXTWRITE->WRITE&1 ) {
		WARN_LOG("EXTWRITE\n");
		ExtWrite();
		EXTWRITE->WRITE = 0;
	}

	if( conf.options & GSOPTION_WIREFRAME ) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glEnable(GL_SCISSOR_TEST);

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
		s_nCurResolveIndex = (s_nCurResolveIndex+1)%ARRAY_SIZE(s_nResolveCounts);

		int total = 0;
		for(int i = 0; i < ARRAY_SIZE(s_nResolveCounts); ++i) total += s_nResolveCounts[i];

		if( total / ARRAY_SIZE(s_nResolveCounts) > 3 ) {
			if( s_nLastResolveReset > (int)(fFPS * 8) ) {
				// reset
				ERROR_LOG("video mem reset\n");
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

void SET_VERTEX(VertexGPU *p, int Index, const VB& curvb) 
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

	if ((vb[!prim->ctxt].nCount > 0) && (vb[prim->ctxt].gsfb.fbp == vb[!prim->ctxt].gsfb.fbp))
	{
		assert( vb[prim->ctxt].nCount == 0 );
		Flush(!prim->ctxt);
	}

	curvb.NotifyWrite(1);

	int last = (gs.primIndex+2)%ARRAY_SIZE(gs.gsvertex);

	VertexGPU* p = curvb.pBufferData+curvb.nCount;
	SET_VERTEX(&p[0], last, curvb);
	curvb.nCount++;

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

	if( vb[!prim->ctxt].nCount > 0 && vb[prim->ctxt].gsfb.fbp == vb[!prim->ctxt].gsfb.fbp )
	{
		assert( vb[prim->ctxt].nCount == 0 );
		Flush(!prim->ctxt);
	}

	curvb.NotifyWrite(2);

	int next = (gs.primIndex+1)%ARRAY_SIZE(gs.gsvertex);
	int last = (gs.primIndex+2)%ARRAY_SIZE(gs.gsvertex);

	VertexGPU* p = curvb.pBufferData+curvb.nCount;
	SET_VERTEX(&p[0], next, curvb);
	SET_VERTEX(&p[1], last, curvb);

	curvb.nCount += 2;

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

	if ((vb[!prim->ctxt].nCount > 0) && (vb[prim->ctxt].gsfb.fbp == vb[!prim->ctxt].gsfb.fbp))
	{
		assert( vb[prim->ctxt].nCount == 0 );
		Flush(!prim->ctxt);
	}

	curvb.NotifyWrite(3);

	VertexGPU* p = curvb.pBufferData+curvb.nCount;
	SET_VERTEX(&p[0], 0, curvb);
	SET_VERTEX(&p[1], 1, curvb);
	SET_VERTEX(&p[2], 2, curvb);

	curvb.nCount += 3;

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

	if ((vb[!prim->ctxt].nCount > 0) && (vb[prim->ctxt].gsfb.fbp == vb[!prim->ctxt].gsfb.fbp))
	{
		assert( vb[prim->ctxt].nCount == 0 );
		Flush(!prim->ctxt);
	}

	curvb.NotifyWrite(3);

	VertexGPU* p = curvb.pBufferData+curvb.nCount;
	SET_VERTEX(&p[0], 0, curvb);
	SET_VERTEX(&p[1], 1, curvb);
	SET_VERTEX(&p[2], 2, curvb);

	curvb.nCount += 3;

	// add 1 to skip the first vertex
	if (gs.primIndex == gs.nTriFanVert)
		gs.primIndex = (gs.primIndex+1)%ARRAY_SIZE(gs.gsvertex);

#ifdef PRIM_LOG
	OUTPUT_VERT(PRIM_LOG, p[0], 0);
	OUTPUT_VERT(PRIM_LOG, p[1], 1);
	OUTPUT_VERT(PRIM_LOG, p[2], 2);
#endif
}

void SetKickVertex(VertexGPU *p, Vertex v, int next, const VB& curvb) 
{
	SET_VERTEX(p, next, curvb); 
	MOVZ(p, v.z, curvb);	
	MOVFOG(p, v);
}

void ZeroGS::KickSprite()
{
	assert( gs.primC >= 2 );
	VB& curvb = vb[prim->ctxt];
	
	if (curvb.bNeedTexCheck) curvb.FlushTexData();

	if ((vb[!prim->ctxt].nCount > 0) && (vb[prim->ctxt].gsfb.fbp == vb[!prim->ctxt].gsfb.fbp))
	{
		assert( vb[prim->ctxt].nCount == 0 );
		Flush(!prim->ctxt);
	}

	curvb.NotifyWrite(6);

	int next = (gs.primIndex+1)%ARRAY_SIZE(gs.gsvertex);
	int last = (gs.primIndex+2)%ARRAY_SIZE(gs.gsvertex);

	// sprite is too small and AA shows lines (tek4)
	if ( s_AAx ) 
	{
		gs.gsvertex[last].x += 4;
		if( s_AAy ) gs.gsvertex[last].y += 4;
	}

	// might be bad sprite (KH dialog text)
	//if( gs.gsvertex[next].x == gs.gsvertex[last].x || gs.gsvertex[next].y == gs.gsvertex[last].y )
	  //return;

	VertexGPU* p = curvb.pBufferData+curvb.nCount;
	
	SetKickVertex(&p[0], gs.gsvertex[last], next, curvb);
	SetKickVertex(&p[3], gs.gsvertex[last], next, curvb);
	SetKickVertex(&p[1], gs.gsvertex[last], last, curvb);
	SetKickVertex(&p[4], gs.gsvertex[last], last, curvb);
	SetKickVertex(&p[2], gs.gsvertex[last], next, curvb);
	
	p[2].s = p[1].s;
	p[2].x = p[1].x;

	SetKickVertex(&p[5], gs.gsvertex[last], last, curvb);
	p[5].s = p[0].s;
	p[5].x = p[0].x;	

	curvb.nCount += 6;

#ifdef PRIM_LOG
	OUTPUT_VERT(PRIM_LOG, p[0], 0);
	OUTPUT_VERT(PRIM_LOG, p[1], 1);
#endif
}

void ZeroGS::KickDummy()
{
	//GREG_LOG("Kicking bad primitive: %.8x\n", *(u32*)prim);
}

__forceinline void ZeroGS::RenderFBA(const VB& curvb, CGparameter sOneColor)
{
	// add fba to all pixels
	GL_STENCILFUNC(GL_ALWAYS, STENCIL_FBA, 0xff);
	glStencilMask(STENCIL_CLEAR);
	glStencilOp(GL_ZERO, GL_KEEP, GL_REPLACE);

	glDisable(GL_DEPTH_TEST);
	glDepthMask(0);
	glColorMask(0,0,0,0);

	if( s_bWriteDepth ) ResetRenderTarget(1);

	glEnable(GL_ALPHA_TEST);
	glAlphaFunc(GL_GEQUAL, 1);

	Vector v;
	v.x = 1; v.y = 2; v.z = 0; v.w = 0;
	cgGLSetParameter4fv(sOneColor, v);

	DRAW();

	if( !curvb.test.ate )
		glDisable(GL_ALPHA_TEST);
	else 
		glAlphaFunc(g_dwAlphaCmp[curvb.test.atst], b2XAlphaTest ? min(1.0f,curvb.test.aref*(1/127.5f)) : curvb.test.aref*(1/255.0f));

	// reset (not necessary)
	GL_COLORMASK(s_dwColorWrite);
	glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

	if( !curvb.zbuf.zmsk ) 
	{
		glDepthMask(1);
		
		assert( curvb.pdepth != NULL );
		if( s_bWriteDepth ) curvb.pdepth->SetRenderTarget(1);
	}
	GL_ZTEST(curvb.test.zte);
}

__forceinline void ZeroGS::RenderAlphaTest(const VB& curvb, CGparameter sOneColor)
{
	if( !g_bUpdateStencil ) return;

	if( curvb.test.ate )
		if( curvb.test.afail == 1 ) glDisable(GL_ALPHA_TEST);

	glDepthMask(0);
	glColorMask(0,0,0,0);

	Vector v;
	v.x = 1; v.y = 2; v.z = 0; v.w = 0;
	cgGLSetParameter4fv(sOneColor, v);

	if (s_bWriteDepth) ResetRenderTarget(1);

	// or a 1 to the stencil buffer wherever alpha passes
	glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
	s_stencilfunc = GL_ALWAYS;

	glEnable(GL_STENCIL_TEST);
	
	if( !s_bDestAlphaTest ) 
	{
		// clear everything
		s_stencilref = 0;
		glStencilMask(STENCIL_CLEAR);
		glDisable(GL_ALPHA_TEST);
		GL_STENCILFUNC_SET();
		DRAW();

		if( curvb.test.ate && curvb.test.afail != 1 && USEALPHATESTING )
			glEnable(GL_ALPHA_TEST);
	}

	if( curvb.test.ate && curvb.test.atst>1 && curvb.test.aref > 0x80) 
	{
		v.x = 1; v.y = 1; v.z = 0; v.w = 0;
		cgGLSetParameter4fv(sOneColor, v);
		glAlphaFunc(g_dwAlphaCmp[curvb.test.atst], curvb.test.aref*(1/255.0f));
	}

	s_stencilref = STENCIL_SPECIAL;
	glStencilMask(STENCIL_SPECIAL);
	GL_STENCILFUNC_SET();
	glDisable(GL_DEPTH_TEST);
	
	DRAW();

	if( curvb.test.zte )
		glEnable(GL_DEPTH_TEST);
	GL_ALPHATEST(0);
	GL_COLORMASK(s_dwColorWrite);

	if( !curvb.zbuf.zmsk ) 
	{
		glDepthMask(1);

		// set rt next level
		if (s_bWriteDepth) curvb.pdepth->SetRenderTarget(1);
	}
}

__forceinline void ZeroGS::RenderStencil(const VB& curvb, u32 dwUsingSpecialTesting)
{
	//NOTE: This stencil hack for dest alpha testing ONLY works when 
	// the geometry in one DrawPrimitive call does not overlap

	// mark the stencil buffer for the new data's bits (mark 4 if alpha is >= 0xff)
	// mark 4 if a pixel was written (so that the stencil buf can be changed with new values)
	glStencilMask(STENCIL_PIXELWRITE);
	glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

	s_stencilmask = (curvb.test.date?STENCIL_ALPHABIT:0)|(dwUsingSpecialTesting?STENCIL_SPECIAL:0);
	s_stencilfunc = s_stencilmask ? GL_EQUAL : GL_ALWAYS;
	s_stencilref = curvb.test.date*curvb.test.datm|STENCIL_PIXELWRITE|(dwUsingSpecialTesting?STENCIL_SPECIAL:0);
	GL_STENCILFUNC_SET();
}

__forceinline void ZeroGS::ProcessStencil(const VB& curvb)
{
	assert( !curvb.fba.fba );

	// set new alpha bit
	glStencilMask(STENCIL_ALPHABIT);
	GL_STENCILFUNC(GL_EQUAL, STENCIL_PIXELWRITE, STENCIL_PIXELWRITE|STENCIL_FBA);
	glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

	glDisable(GL_DEPTH_TEST);
	glDepthMask(0);
	glColorMask(0,0,0,0);

	if (s_bWriteDepth) ResetRenderTarget(1);

	GL_ALPHATEST(0);

	SETPIXELSHADER(ppsOne.prog);
	DRAW();

	// process when alpha >= 0xff
	GL_STENCILFUNC(GL_EQUAL, STENCIL_PIXELWRITE|STENCIL_FBA|STENCIL_ALPHABIT, STENCIL_PIXELWRITE|STENCIL_FBA);
	DRAW();

	// clear STENCIL_PIXELWRITE bit
	glStencilMask(STENCIL_CLEAR);
	GL_STENCILFUNC(GL_ALWAYS, 0, STENCIL_PIXELWRITE|STENCIL_FBA);

	DRAW();

	// restore state
	GL_COLORMASK(s_dwColorWrite);

	if( curvb.test.ate && USEALPHATESTING)
		glEnable(GL_ALPHA_TEST);

	if( !curvb.zbuf.zmsk ) {
		glDepthMask(1);

		if( s_bWriteDepth ) {
			assert( curvb.pdepth != NULL );
			curvb.pdepth->SetRenderTarget(1);
		}
	}

	GL_ZTEST(curvb.test.zte);
	glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
}

__forceinline void ZeroGS::ProcessFBA(const VB& curvb, CGparameter sOneColor)
{
	if( (curvb.frame.fbm&0x80000000) ) return;

	// add fba to all pixels that were written and alpha was less than 0xff
	glStencilMask(STENCIL_ALPHABIT);
	glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

	GL_STENCILFUNC(GL_EQUAL, STENCIL_FBA|STENCIL_PIXELWRITE|STENCIL_ALPHABIT, STENCIL_PIXELWRITE|STENCIL_FBA);

	glDisable(GL_DEPTH_TEST);
	glDepthMask(0);
	glColorMask(0,0,0,1);
	
	if( s_bWriteDepth ) {
		ResetRenderTarget(1);
	}

	// processes the pixels with ALPHA < 0x80*2
	glEnable(GL_ALPHA_TEST);
	glAlphaFunc(GL_LEQUAL, 1);

	// add 1 to dest
	GL_BLEND_ALPHA(GL_ONE, GL_ONE);
	GL_BLENDEQ_ALPHA(GL_FUNC_ADD);

	float f = 1;
	cgGLSetParameter4fv(sOneColor, &f);
	SETPIXELSHADER(ppsOne.prog);

	DRAW();

	glDisable(GL_ALPHA_TEST);

	// reset bits
	glStencilMask(STENCIL_CLEAR);
	GL_STENCILFUNC(GL_GREATER, 0, STENCIL_PIXELWRITE|STENCIL_FBA);
	glStencilOp(GL_KEEP, GL_KEEP, GL_ZERO);

	DRAW();

	if( curvb.test.atst && USEALPHATESTING) {
		glEnable(GL_ALPHA_TEST);
		glAlphaFunc(g_dwAlphaCmp[curvb.test.atst], b2XAlphaTest ? min(1.0f,curvb.test.aref*(1/127.5f)) : curvb.test.aref*(1/255.0f));
	}

	// restore (SetAlphaVariables)
	GL_BLEND_ALPHA(GL_ONE, GL_ZERO);
	if(vAlphaBlendColor.y<0) GL_BLENDEQ_ALPHA(GL_FUNC_REVERSE_SUBTRACT);

	// reset (not necessary)
	GL_COLORMASK(s_dwColorWrite);

	if( !curvb.zbuf.zmsk ) {
		glDepthMask(1);

		if (s_bWriteDepth) curvb.pdepth->SetRenderTarget(1);
	}
	GL_ZTEST(curvb.test.zte);
}

inline void ZeroGS::SetContextTarget(int context)
{
	VB& curvb = vb[context];
	GL_REPORT_ERRORD();

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
	assert( curvb.pdepth->fbh == curvb.prndr->fbh );

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
	
	GL_REPORT_ERRORD();
	if( curvb.prndr->status & CRenderTarget::TS_NeedUpdate ) {

//	  if(bSetTarg s_bWriteDepth ) {
//		  if( s_bWriteDepth ) {
//				curvb.pdepth->SetRenderTarget(1);
//				curvb.pdepth->SetDepthStencilSurface();
//		  }
//		  else curvb.pdepth->SetDepthStencilSurface();
//	  }

		curvb.prndr->Update(context, curvb.pdepth);
	}
	else { 

		//if( (vb[0].prndr != vb[1].prndr && vb[!context].bVarsSetTarg) || !vb[context].bVarsSetTarg )
		curvb.prndr->SetRenderTarget(0);
		//if( bSetTarg && ((vb[0].pdepth != vb[1].pdepth && vb[!context].bVarsSetTarg) || !vb[context].bVarsSetTarg) ) 
		curvb.pdepth->SetDepthStencilSurface();
		
		if (conf.mrtdepth && ZeroGS::IsWriteDepth()) curvb.pdepth->SetRenderTarget(1);
		
		if (s_ptexCurSet[0] == curvb.prndr->ptex) s_ptexCurSet[0] = 0;
		if (s_ptexCurSet[1] == curvb.prndr->ptex) s_ptexCurSet[1] = 0;

		curvb.prndr->SetViewport();
	}

	curvb.prndr->SetTarget(curvb.frame.fbp, curvb.scissor, context);

	if( (curvb.zbuf.zbp-curvb.pdepth->fbp) != (curvb.frame.fbp - curvb.prndr->fbp) && curvb.test.zte )
		WARN_LOG("frame and zbuf not aligned\n");

	curvb.bVarsSetTarg = TRUE;
	if( vb[!context].prndr != curvb.prndr ) vb[!context].bVarsSetTarg = FALSE;

	assert( !(curvb.prndr->status&CRenderTarget::TS_NeedUpdate) );
	assert( curvb.pdepth == NULL || !(curvb.pdepth->status&CRenderTarget::TS_NeedUpdate) );
	GL_REPORT_ERRORD();
}

void ZeroGS::SetTexVariables(int context, FRAGMENTSHADER* pfragment, int settexint)
{
	if (!vb[context].curprim.tme) return;

	DVProfileFunc _pf("SetTexVariables");

	assert( !vb[context].bNeedTexCheck );

	Vector v, v2;
	tex0Info& tex0 = vb[context].tex0;
	
	float fw = (float)tex0.tw;
	float fh = (float)tex0.th;

	if( !vb[context].bTexConstsSync ) {
		// alpha and texture highlighting
		Vector valpha, valpha2;

		// if clut, use the frame format
		int psm = tex0.psm;
		if( PSMT_ISCLUT(tex0.psm) ) psm = tex0.cpsm;
		
		int nNeedAlpha = (psm == 1 || psm == 2 || psm == 10);

		Vector vblack;
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

		cgGLSetParameter4fv(pfragment->fTexAlpha, valpha);
		cgGLSetParameter4fv(pfragment->fTexAlpha2, valpha2);
		if( tex0.tcc && gs.texa.aem && (psm == PSMCT24 || psm == PSMCT16 || psm == PSMCT16S) )
			cgGLSetParameter4fv(pfragment->fTestBlack, vblack);

		// clamp relies on texture width
		{
			clampInfo* pclamp = &ZeroGS::vb[context].clamp;
			Vector v, v2;
			v.x = v.y = 0;
			u32* ptex = ZeroGS::vb[context].ptexClamp;
			ptex[0] = ptex[1] = 0;

			float fw = ZeroGS::vb[context].tex0.tw;
			float fh = ZeroGS::vb[context].tex0.th;

			switch(pclamp->wms) {
				case 0:
					v2.x = -1e10;   v2.z = 1e10;
					break;
				case 1: // pclamp
					// suikoden5 movie text
					v2.x = 0; v2.z = 1-0.5f/fw;
					break;
				case 2: // reg pclamp
					v2.x = (pclamp->minu+0.5f)/fw;  v2.z = (pclamp->maxu-0.5f)/fw;
					break;

				case 3: // region rep x
					v.x = 0.9999f;
					v.z = fw / (float)GPU_TEXMASKWIDTH;
					v2.x = (float)GPU_TEXMASKWIDTH / fw;
					v2.z = pclamp->maxu / fw;

					if( pclamp->minu != g_PrevBitwiseTexX ) {
						g_PrevBitwiseTexX = pclamp->minu;
						ptex[0] = ZeroGS::s_BitwiseTextures.GetTex(pclamp->minu, 0);
					}
					break;
			}

			switch(pclamp->wmt) {
				case 0:
					v2.y = -1e10;   v2.w = 1e10;
					break;
				case 1: // pclamp
					// suikoden5 movie text
					v2.y = 0;   v2.w = 1-0.5f/fh;
					break;
				case 2: // reg pclamp
					v2.y = (pclamp->minv+0.5f)/fh; v2.w = (pclamp->maxv-0.5f)/fh;
					break;

				case 3: // region rep y
					v.y = 0.9999f;
					v.w = fh / (float)GPU_TEXMASKWIDTH;
					v2.y = (float)GPU_TEXMASKWIDTH / fh;
					v2.w = pclamp->maxv / fh;

					if( pclamp->minv != g_PrevBitwiseTexY ) {
						g_PrevBitwiseTexY = pclamp->minv;
						ptex[1] = ZeroGS::s_BitwiseTextures.GetTex(pclamp->minv, ptex[0]);
					}
					break;
			}

			if( pfragment->fTexWrapMode != 0 )
				cgGLSetParameter4fv(pfragment->fTexWrapMode, v);
			if( pfragment->fClampExts != 0  )
				cgGLSetParameter4fv(pfragment->fClampExts, v2);
		}

		vb[context].bTexConstsSync = TRUE;
	}

	if(s_bTexFlush ) {
		if( PSMT_ISCLUT(tex0.psm) )
			texClutWrite(context);
		else
			s_bTexFlush = FALSE;
	}

	if( settexint ) {
		CMemoryTarget* pmemtarg = g_MemTargs.GetMemoryTarget(tex0, 1);

		if( vb[context].bVarsTexSync ) {
			if( vb[context].pmemtarg != pmemtarg ) {
				SetTexVariablesInt(context, GetTexFilter(vb[context].tex1), tex0, pmemtarg, pfragment, s_bForceTexFlush);
				vb[context].bVarsTexSync = TRUE;
			}
		}
		else {
			SetTexVariablesInt(context, GetTexFilter(vb[context].tex1), tex0, pmemtarg, pfragment, s_bForceTexFlush);   
			vb[context].bVarsTexSync = TRUE;

			INC_TEXVARS();
		}
	}
	else {
		vb[context].bVarsTexSync = FALSE;
	}
}

void ZeroGS::SetTexVariablesInt(int context, int bilinear, const tex0Info& tex0, CMemoryTarget* pmemtarg, FRAGMENTSHADER* pfragment, int force)
{
	DVProfileFunc _pf("SetTexVariablesInt");

	Vector v;
	assert( pmemtarg != NULL && pfragment != NULL);

	float fw = (float)tex0.tw;
	float fh = (float)tex0.th;

	bool bUseBilinear = bilinear > 1 || (bilinear && conf.bilinear);
	if( bUseBilinear ) {
		v.x = (float)fw;
		v.y = (float)fh;
		v.z = 1.0f / (float)fw;
		v.w = 1.0f / (float)fh;
	if (pfragment->fRealTexDims)
			cgGLSetParameter4fv(pfragment->fRealTexDims, v);
	else
		cgGLSetParameter4fv(cgGetNamedParameter(pfragment->prog,"g_fRealTexDims"),v);	
	}

	if( m_Blocks[tex0.psm].bpp == 0 ) {
		ERROR_LOG("Undefined tex psm 0x%x!\n", tex0.psm);
		return;
	}

	const BLOCK& b = m_Blocks[tex0.psm];

	float fbw = (float)tex0.tbw;

	Vector vTexDims;
	vTexDims.x = b.vTexDims.x * fw;
	vTexDims.y = b.vTexDims.y * fh;
	vTexDims.z = (float)BLOCK_TEXWIDTH*(0.002f / 64.0f + 0.01f/128.0f);
	vTexDims.w = (float)BLOCK_TEXHEIGHT*0.2f/512.0f;

	if( bUseBilinear ) {
		vTexDims.x *= 1/128.0f;
		vTexDims.y *= 1/512.0f;
		vTexDims.z *= 1/128.0f;
		vTexDims.w *= 1/512.0f;
	}

	float g_fitexwidth = g_fiGPU_TEXWIDTH/(float)pmemtarg->widthmult;
	float g_texwidth = GPU_TEXWIDTH*(float)pmemtarg->widthmult;

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

	cgGLSetParameter4fv(pfragment->fTexDims, vTexDims);
	cgGLSetParameter4fv(pfragment->fTexBlock, &b.vTexBlock.x);
	cgGLSetParameter4fv(pfragment->fTexOffset, v);

	// get hardware texture dims
	int texheight = (pmemtarg->realheight+pmemtarg->widthmult-1)/pmemtarg->widthmult;
	int texwidth = GPU_TEXWIDTH*pmemtarg->widthmult*pmemtarg->channels;

	v.y = 1;//(float)1.0f / (float)((pmemtarg->realheight+pmemtarg->widthmult-1)/pmemtarg->widthmult);
	v.x = (fpageint-(float)pmemtarg->realy/(float)pmemtarg->widthmult+0.5f);//*v.y;
	
//	v.x *= (float)texheight;
//	v.y *= (float)texheight;
	v.z = (float)texwidth;
	if( !(g_nPixelShaderVer & SHADER_ACCURATE) || bUseBilinear )
		v.w = 0.25f;
	else
		v.w = 0.5f;

	cgGLSetParameter4fv(pfragment->fPageOffset, v);

	if( force ) s_ptexCurSet[context] = pmemtarg->ptex->tex;
	else s_ptexNextSet[context] = pmemtarg->ptex->tex;
	vb[context].pmemtarg = pmemtarg;
	
	vb[context].bVarsTexSync = FALSE; 
}
#define SET_ALPHA_COLOR_FACTOR(sign) \
{ \
	switch(a.c) \
	{ \
		case 0: \
			vAlphaBlendColor.y = (sign) ? 2.0f*255.0f/256.0f : -2.0f*255.0f/256.0f; \
			s_srcalpha = GL_ONE; \
			s_alphaeq = (sign) ? GL_FUNC_ADD : GL_FUNC_REVERSE_SUBTRACT; \
			break; \
			\
		case 1: \
			/* if in 24 bit mode, dest alpha should be one */ \
			switch(vb[icurctx].prndr->psm&0xf) \
			{ \
				case 0: \
					bDestAlphaColor = (a.d!=2)&&((a.a==a.d)||(a.b==a.d)); \
					break; \
					\
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
			vAlphaBlendColor.y = 0; \
			vAlphaBlendColor.w = (sign) ? (float)a.fix * (2.0f/255.0f) : (float)a.fix * (-2.0f/255.0f); \
			usec = 0; /* change so that alpha comes from source*/ \
		break; \
	} \
} \

//if( a.fix <= 0x80 ) { \
// dwTemp = (a.fix*2)>255?255:(a.fix*2); \
// dwTemp = dwTemp|(dwTemp<<8)|(dwTemp<<16)|0x80000000; \
// printf("bfactor: %8.8x\n", dwTemp); \
// glBlendColorEXT(dwTemp); \
// } \
// else { \

void ZeroGS::ResetAlphaVariables()
{
}

void ZeroGS::SetAlphaVariables(const alphaInfo& a)
{
	bool alphaenable = true;

	// TODO: negative color when not clamping turns to positive???
	g_vars._bAlphaState = 0; // set all to zero
	bNeedBlendFactorInAlpha = 0;
	b2XAlphaTest = 1;
	u32 dwTemp = 0xffffffff;

	// default
	s_srcalpha = GL_ONE;
	s_dstalpha = GL_ZERO;
	s_alphaeq = GL_FUNC_ADD;
	s_rgbeq = GL_FUNC_ADD;

	s_alphaInfo = a;

	vAlphaBlendColor = Vector(1,2*255.0f/256.0f,0,0);
	u32 usec = a.c;

	if( a.a == a.b ) 
	{ // just d remains
		if( a.d == 0 ) 
		{
			alphaenable = false;
		}
		else 
		{
			s_dstrgb = a.d == 1 ? GL_ONE : GL_ZERO;
			s_srcrgb = GL_ZERO;
			s_rgbeq = GL_FUNC_ADD;
		}

		goto EndSetAlpha;
	}
	else if( a.d == 2 ) 
	{ // zero
		if( a.a == 2 ) 
		{
			// zero all color
			s_srcrgb = GL_ZERO;
			s_dstrgb = GL_ZERO;
			goto EndSetAlpha;
		}
		else if( a.b == 2 ) 
		{
			//b2XAlphaTest = 1;

			SET_ALPHA_COLOR_FACTOR(1);

			if( bDestAlphaColor == 2 ) 
			{
				s_rgbeq = GL_FUNC_ADD;
				s_srcrgb = a.a == 0 ? GL_ONE : GL_ZERO;
				s_dstrgb = a.a == 0 ? GL_ZERO : GL_ONE;
			}
			else 
			{
				bAlphaClamping = 2;

			s_rgbeq = GL_FUNC_ADD;
			s_srcrgb = a.a == 0 ? blendalpha[usec] : GL_ZERO;
			s_dstrgb = a.a == 0 ? GL_ZERO : blendalpha[usec];
			}

			goto EndSetAlpha;
		}

		// nothing is zero, so must do some real blending
		//b2XAlphaTest = 1;
		bAlphaClamping = 3;
	
		SET_ALPHA_COLOR_FACTOR(1);
	
		s_rgbeq = a.a == 0 ? GL_FUNC_SUBTRACT : GL_FUNC_REVERSE_SUBTRACT;
		s_srcrgb = bDestAlphaColor == 2 ? GL_ONE : blendalpha[usec];
		s_dstrgb = bDestAlphaColor == 2 ? GL_ONE : blendalpha[usec];
	}
	else if( a.a == 2 ) 
	{ // zero

		//b2XAlphaTest = 1;
		bAlphaClamping = 1; // min testing

		SET_ALPHA_COLOR_FACTOR(1);

		if( a.b == a.d ) 
		{
			// can get away with 1-A
			s_rgbeq = GL_FUNC_ADD;
			s_srcrgb = (a.b == 0 && bDestAlphaColor != 2) ? blendinvalpha[usec] : GL_ZERO;
			s_dstrgb = (a.b == 0 || bDestAlphaColor == 2) ? GL_ZERO : blendinvalpha[usec];
		}
		else 
		{
			s_rgbeq = a.b==0 ? GL_FUNC_REVERSE_SUBTRACT : GL_FUNC_SUBTRACT;
			s_srcrgb = (a.b == 0 && bDestAlphaColor != 2) ? blendalpha[usec] : GL_ONE;
			s_dstrgb = (a.b == 0 || bDestAlphaColor == 2 ) ? GL_ONE : blendalpha[usec];
		}
	}
	else if( a.b == 2 ) 
	{
		bAlphaClamping = 2; // max testing

		SET_ALPHA_COLOR_FACTOR(a.a!=a.d);

		if( a.a == a.d ) 
		{
			// can get away with 1+A, but need to set alpha to negative
			s_rgbeq = GL_FUNC_ADD;

			if( bDestAlphaColor == 2 ) 
			{
				assert(usec==1);

				// all ones
				bNeedBlendFactorInAlpha = 1;
				vAlphaBlendColor.y = 0;
				vAlphaBlendColor.w = -1;

				s_srcrgb = (a.a == 0) ? GL_ONE_MINUS_SRC_ALPHA : GL_ZERO;
				s_dstrgb = (a.a == 0) ? GL_ZERO : GL_ONE_MINUS_SRC_ALPHA;
			}
			else 
			{
				s_srcrgb = a.a == 0 ? blendinvalpha[usec] : GL_ZERO;
				s_dstrgb = a.a == 0 ? GL_ZERO : blendinvalpha[usec];
			}
		}
		else 
		{
			//b2XAlphaTest = 1;
			s_rgbeq = GL_FUNC_ADD;
			s_srcrgb = (a.a == 0 && bDestAlphaColor != 2) ? blendalpha[usec] : GL_ONE;
			s_dstrgb = (a.a == 0 || bDestAlphaColor == 2) ? GL_ONE : blendalpha[usec];
		}
	}
	else 
	{
		// all 3 components are valid!
		bAlphaClamping = 3; // all testing
		SET_ALPHA_COLOR_FACTOR(a.a!=a.d);

		if( a.a == a.d ) 
		{
			// can get away with 1+A, but need to set alpha to negative
			s_rgbeq = GL_FUNC_ADD;

			if( bDestAlphaColor == 2 ) 
			{
				assert(usec==1);

				// all ones
				bNeedBlendFactorInAlpha = 1;
				vAlphaBlendColor.y = 0;
				vAlphaBlendColor.w = -1;
				s_srcrgb = a.a == 0 ? GL_ONE_MINUS_SRC_ALPHA : GL_SRC_ALPHA;
				s_dstrgb = a.a == 0 ? GL_SRC_ALPHA : GL_ONE_MINUS_SRC_ALPHA;
			}
			else 
			{
				s_srcrgb = a.a == 0 ? blendinvalpha[usec] : blendalpha[usec];
				s_dstrgb = a.a == 0 ? blendalpha[usec] : blendinvalpha[usec];
			}
		}
		else
		{
			assert(a.b == a.d);
			s_rgbeq = GL_FUNC_ADD;

			if( bDestAlphaColor == 2 ) 
			{
				assert(usec==1);

				// all ones
				bNeedBlendFactorInAlpha = 1;
				vAlphaBlendColor.y = 0;
				vAlphaBlendColor.w = 1;
				s_srcrgb = a.a != 0 ? GL_ONE_MINUS_SRC_ALPHA : GL_SRC_ALPHA;
				s_dstrgb = a.a != 0 ? GL_SRC_ALPHA : GL_ONE_MINUS_SRC_ALPHA;
			}
			else 
			{
				//b2XAlphaTest = 1;
				s_srcrgb = a.a != 0 ? blendinvalpha[usec] : blendalpha[usec];
				s_dstrgb = a.a != 0 ? blendalpha[usec] : blendinvalpha[usec];
			}
		}
	}

	EndSetAlpha:

	GL_BLEND_SET();
	zgsBlendEquationSeparateEXT(s_rgbeq, s_alphaeq);

	if( alphaenable ) 
		glEnable(GL_BLEND); // always set
	else 
		glDisable(GL_BLEND);

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
			Vector v;

			// set it immediately
			v.x = (gs.fogcol&0xff)/255.0f;
			v.y = ((gs.fogcol>>8)&0xff)/255.0f;
			v.z = ((gs.fogcol>>16)&0xff)/255.0f;
			cgGLSetParameter4fv(g_fparamFogColor, v);
		}
	}
}

void ZeroGS::ExtWrite()
{
	WARN_LOG("ExtWrite\n");

	// use local DISPFB, EXTDATA, EXTBUF, and PMODE
//  int bpp, start, end;
//  tex0Info texframe;

//  bpp = 4;
//  if( texframe.psm == 0x12 ) bpp = 3;
//  else if( texframe.psm & 2 ) bpp = 2;
//
//  // get the start and end addresses of the buffer
//  GetRectMemAddress(start, end, texframe.psm, 0, 0, texframe.tw, texframe.th, texframe.tbp0, texframe.tbw);
}

////////////
// Caches //
////////////

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

	// do a fast test with MMX
#ifdef _MSC_VER

	int storeebx;
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

	
		jne Continue1
		add ecx, 256 // skip whole block
Continue1:
		add edx, 64
		sub ebx, 16
		jmp Start

Return:
		emms
		mov ebx, storeebx
	}

#else // linux

	// do a fast test with MMX
	__asm__(
		".intel_syntax\n"
"Start:\n"
		"movq %%mm0, [%%ecx]\n"
		"movq %%mm1, [%%ecx+8]\n"
		"pcmpeqd %%mm0, [%%edx]\n"
		"pcmpeqd %%mm1, [%%edx+16]\n"
		"movq %%mm2, [%%ecx+16]\n"
		"movq %%mm3, [%%ecx+24]\n"
		"pcmpeqd %%mm2, [%%edx+32]\n"
		"pcmpeqd %%mm3, [%%edx+48]\n"
		"pand %%mm0, %%mm1\n"
		"pand %%mm2, %%mm3\n"
		"movq %%mm4, [%%ecx+32]\n"
		"movq %%mm5, [%%ecx+40]\n"
		"pcmpeqd %%mm4, [%%edx+8]\n"
		"pcmpeqd %%mm5, [%%edx+24]\n"
		"pand %%mm0, %%mm2\n"
		"pand %%mm4, %%mm5\n"
		"movq %%mm6, [%%ecx+48]\n"
		"movq %%mm7, [%%ecx+56]\n"
		"pcmpeqd %%mm6, [%%edx+40]\n"
		"pcmpeqd %%mm7, [%%edx+56]\n"
		"pand %%mm0, %%mm4\n"
		"pand %%mm6, %%mm7\n"
		"pand %%mm0, %%mm6\n"
		"pmovmskb %%eax, %%mm0\n"
		"cmp %%eax, 0xff\n"
		"je Continue\n"
		".att_syntax\n"
		"movb $1, %0\n"
		".intel_syntax\n"
		"jmp Return\n"
"Continue:\n"
		"cmp %%ebx, 16\n"
		"jle Return\n"
		"test %%ebx, 0x10\n"
		"jz AddEcx\n"
		"sub %%edx, 448\n" // go back and down one column
"AddEcx:\n"
		"add %%edx, 256\n" // go to the right block
		"cmp %%ebx, 0x90\n"
		"jne Continue1\n"
		"add %%edx, 256\n" // skip whole block
"Continue1:\n"
		"add %%ecx, 64\n"
		"sub %%ebx, 16\n"
		"jmp Start\n"
"Return:\n"
		"emms\n"
		".att_syntax\n" : "=m"(bRet) : "c"(dst), "d"(src), "S"(entries) : "eax", "memory"); // Breaks -fPIC

#endif // _WIN32

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

	int entries = (tex0.psm & 3)==3 ? 256 : 16;

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
					if (((u32)(uptr)dst & 0x3ff) == 0) dst = (u16*)(g_pbyGSClut+2);
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
					if (((u32)(uptr)dst & 0x3ff) == 0) dst = (u16*)(g_pbyGSClut+2);
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
			break;
			}
			
			default:
			{
#ifdef ZEROGS_DEVBUILD
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
		if (s_ptexCurSet[0] != s_ptexNextSet[0]) s_ptexCurSet[0] = s_ptexNextSet[0];
		if (s_ptexCurSet[1] != s_ptexNextSet[1]) s_ptexCurSet[1] = s_ptexNextSet[1];
	}
}

int ZeroGS::Save(char* pbydata)
{
	if( pbydata == NULL )
		return 40 + 0x00400000 + sizeof(gs) + 2*VBSAVELIMIT + 2*sizeof(frameInfo) + 4 + 256*4;

	s_RTs.ResolveAll();
	s_DepthRTs.ResolveAll();
	
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

	return 0;
}

extern u32 s_uTex1Data[2][2], s_uClampData[2];
extern char *libraryName;
bool ZeroGS::Load(char* pbydata)
{
	memset(s_uTex1Data, 0, sizeof(s_uTex1Data));
	memset(s_uClampData, 0, sizeof(s_uClampData));

	g_nCurVBOIndex = 0;

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

		vb[0].Destroy();
		memcpy(&vb[0], pbydata, min(savelimit, VBSAVELIMIT));
		pbydata += savelimit;
		vb[0].pBufferData = NULL;

		vb[1].Destroy();
		memcpy(&vb[1], pbydata, min(savelimit, VBSAVELIMIT));
		pbydata += savelimit;
		vb[1].pBufferData = NULL;

		for(int i = 0; i < 2; ++i) {
			vb[i].Init(VB_BUFFERSIZE);
			vb[i].bNeedZCheck = vb[i].bNeedFrameCheck = 1;
			
			vb[i].bSyncVars = 0; vb[i].bNeedTexCheck = 1;
			memset(vb[i].uCurTex0Data, 0, sizeof(vb[i].uCurTex0Data));
		}

		icurctx = -1;
		
		glBindFramebufferEXT( GL_FRAMEBUFFER_EXT, s_uFramebuffer ); // switch to the backbuffer
		SetFogColor(gs.fogcol);

		GL_REPORT_ERRORD();
		return true;
	}

	return false;
}

void ZeroGS::SaveSnapshot(const char* filename)
{
	g_bMakeSnapshot = 1;
	strSnapshot = filename;
}

bool ZeroGS::SaveRenderTarget(const char* filename, int width, int height, int jpeg)
{
	bool bflip = height < 0;
	height = abs(height);
	vector<u32> data(width*height);
	glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, &data[0]);
	if( glGetError() != GL_NO_ERROR )
		return false;

	if( bflip ) {
		// swap scanlines
		vector<u32> scanline(width);
		for(int i = 0; i < height/2; ++i) {
			memcpy(&scanline[0], &data[i*width], width*4);
			memcpy(&data[i*width], &data[(height-i-1)*width], width*4);
			memcpy(&data[(height-i-1)*width], &scanline[0], width*4);
		}
	}

	if( jpeg ) return SaveJPEG(filename, width, height, &data[0], 70);
	
	return SaveTGA(filename, width, height, &data[0]);
}

bool ZeroGS::SaveTexture(const char* filename, u32 textarget, u32 tex, int width, int height)
{
	vector<u32> data(width*height);
	glBindTexture(textarget, tex);
	glGetTexImage(textarget, 0, GL_RGBA, GL_UNSIGNED_BYTE, &data[0]);
	if( glGetError() != GL_NO_ERROR ) {
		return false;
	}

	return SaveTGA(filename, width, height, &data[0]);//SaveJPEG(filename, width, height, &data[0], 70);
}

extern "C" {
#ifdef _WIN32
#define XMD_H
#undef FAR
#define HAVE_BOOLEAN
#endif

#include "jpeglib.h"
}

bool ZeroGS::SaveJPEG(const char* filename, int image_width, int image_height, const void* pdata, int quality)
{
	u8* image_buffer = new u8[image_width * image_height * 3];
	u8* psrc = (u8*)pdata;
	
	// input data is rgba format, so convert to rgb
	u8* p = image_buffer;
	for(int i = 0; i < image_height; ++i) {
		for(int j = 0; j < image_width; ++j) {
			p[0] = psrc[0];
			p[1] = psrc[1];
			p[2] = psrc[2];
			p += 3;
			psrc += 4;
		}
	}

	/* This struct contains the JPEG compression parameters and pointers to
	* working space (which is allocated as needed by the JPEG library).
	* It is possible to have several such structures, representing multiple
	* compression/decompression processes, in existence at once.  We refer
	* to any one struct (and its associated working data) as a "JPEG object".
	*/
	struct jpeg_compress_struct cinfo;
	/* This struct represents a JPEG error handler.  It is declared separately
	* because applications often want to supply a specialized error handler
	* (see the second half of this file for an example).  But here we just
	* take the easy way out and use the standard error handler, which will
	* print a message on stderr and call exit() if compression fails.
	* Note that this struct must live as long as the main JPEG parameter
	* struct, to avoid dangling-pointer problems.
	*/
	struct jpeg_error_mgr jerr;
	/* More stuff */
	FILE * outfile;	 /* target file */
	JSAMPROW row_pointer[1];	/* pointer to JSAMPLE row[s] */
	int row_stride;	 /* physical row width in image buffer */

	/* Step 1: allocate and initialize JPEG compression object */

	/* We have to set up the error handler first, in case the initialization
	* step fails.  (Unlikely, but it could happen if you are out of memory.)
	* This routine fills in the contents of struct jerr, and returns jerr's
	* address which we place into the link field in cinfo.
	*/
	cinfo.err = jpeg_std_error(&jerr);
	/* Now we can initialize the JPEG compression object. */
	jpeg_create_compress(&cinfo);

	/* Step 2: specify data destination (eg, a file) */
	/* Note: steps 2 and 3 can be done in either order. */

	/* Here we use the library-supplied code to send compressed data to a
	* stdio stream.  You can also write your own code to do something else.
	* VERY IMPORTANT: use "b" option to fopen() if you are on a machine that
	* requires it in order to write binary files.
	*/
	if ((outfile = fopen(filename, "wb")) == NULL) {
	fprintf(stderr, "can't open %s\n", filename);
	exit(1);
	}
	jpeg_stdio_dest(&cinfo, outfile);

	/* Step 3: set parameters for compression */

	/* First we supply a description of the input image.
	* Four fields of the cinfo struct must be filled in:
	*/
	cinfo.image_width = image_width;	/* image width and height, in pixels */
	cinfo.image_height = image_height;
	cinfo.input_components = 3;	 /* # of color components per pixel */
	cinfo.in_color_space = JCS_RGB;	 /* colorspace of input image */
	/* Now use the library's routine to set default compression parameters.
	* (You must set at least cinfo.in_color_space before calling this,
	* since the defaults depend on the source color space.)
	*/
	jpeg_set_defaults(&cinfo);
	/* Now you can set any non-default parameters you wish to.
	* Here we just illustrate the use of quality (quantization table) scaling:
	*/
	jpeg_set_quality(&cinfo, quality, TRUE /* limit to baseline-JPEG values */);

	/* Step 4: Start compressor */

	/* TRUE ensures that we will write a complete interchange-JPEG file.
	* Pass TRUE unless you are very sure of what you're doing.
	*/
	jpeg_start_compress(&cinfo, TRUE);

	/* Step 5: while (scan lines remain to be written) */
	/*		   jpeg_write_scanlines(...); */

	/* Here we use the library's state variable cinfo.next_scanline as the
	* loop counter, so that we don't have to keep track ourselves.
	* To keep things simple, we pass one scanline per call; you can pass
	* more if you wish, though.
	*/
	row_stride = image_width * 3;   /* JSAMPLEs per row in image_buffer */

	while (cinfo.next_scanline < cinfo.image_height) {
	/* jpeg_write_scanlines expects an array of pointers to scanlines.
		* Here the array is only one element long, but you could pass
		* more than one scanline at a time if that's more convenient.
		*/
	row_pointer[0] = & image_buffer[cinfo.next_scanline * row_stride];
	(void) jpeg_write_scanlines(&cinfo, row_pointer, 1);
	}

	/* Step 6: Finish compression */

	jpeg_finish_compress(&cinfo);
	/* After finish_compress, we can close the output file. */
	fclose(outfile);

	/* Step 7: release JPEG compression object */

	/* This is an important step since it will release a good deal of memory. */
	jpeg_destroy_compress(&cinfo);

	delete image_buffer;
	/* And we're done! */
	return true;
}

#if defined(_MSC_VER)
#pragma pack(push, 1)
#endif

struct TGA_HEADER
{
	u8  identsize;		  // size of ID field that follows 18 u8 header (0 usually)
	u8  colourmaptype;	  // type of colour map 0=none, 1=has palette
	u8  imagetype;		  // type of image 0=none,1=indexed,2=rgb,3=grey,+8=rle packed

	s16 colourmapstart;	 // first colour map entry in palette
	s16 colourmaplength;	// number of colours in palette
	u8  colourmapbits;	  // number of bits per palette entry 15,16,24,32

	s16 xstart;			 // image x origin
	s16 ystart;			 // image y origin
	s16 width;			  // image width in pixels
	s16 height;			 // image height in pixels
	u8  bits;			   // image bits per pixel 8,16,24,32
	u8  descriptor;		 // image descriptor bits (vh flip bits)
	
	// pixel data follows header
	
#if defined(_MSC_VER)
};
#pragma pack(pop)
#else
} __attribute__((packed));
#endif

bool ZeroGS::SaveTGA(const char* filename, int width, int height, void* pdata)
{
	TGA_HEADER hdr;
	FILE* f = fopen(filename, "wb");
	if( f == NULL )
		return false;

	assert( sizeof(TGA_HEADER) == 18 && sizeof(hdr) == 18 );
	
	memset(&hdr, 0, sizeof(hdr));
	hdr.imagetype = 2;
	hdr.bits = 32;
	hdr.width = width;
	hdr.height = height;
	hdr.descriptor |= 8|(1<<5); // 8bit alpha, flip vertical

	fwrite(&hdr, sizeof(hdr), 1, f);
	fwrite(pdata, width*height*4, 1, f);
	fclose(f);
	return true;
}

// AVI capture stuff
void ZeroGS::StartCapture()
{
	if( !s_aviinit ) {

#ifdef _WIN32
		START_AVI("zerogs.avi");
#else // linux
		//TODO
#endif
		s_aviinit = 1;
	}
	else {
		ERROR_LOG("Continuing from previous capture");
	}

	s_avicapturing = 1;
}

void ZeroGS::StopCapture()
{
	s_avicapturing = 0;
}

void ZeroGS::CaptureFrame()
{
	assert( s_avicapturing && s_aviinit );

	//vector<u8> mem(nBackbufferWidth*nBackbufferHeight);
	vector<u32> data(nBackbufferWidth*nBackbufferHeight);
	glReadPixels(0, 0, nBackbufferWidth, nBackbufferHeight, GL_RGBA, GL_UNSIGNED_BYTE, &data[0]);
	if( glGetError() != GL_NO_ERROR )
		return;

//  u8* pend = (u8*)&data[0] + (nBackbufferHeight-1)*nBackbufferWidth*4;
//  for(int i = 0; i < conf.height; ++i) {
//	  memcpy_amd(&mem[nBackbufferWidth*4*i], pend - nBackbufferWidth*4*i, nBackbufferWidth * 4);
//  }

	int fps = SMODE1->CMOD == 3 ? 50 : 60;

#ifdef _WIN32
	bool bSuccess = ADD_FRAME_FROM_DIB_TO_AVI("AAAA", fps, nBackbufferWidth, nBackbufferHeight, 32, &data[0]);

	if( !bSuccess ) {
		s_avicapturing = 0;
		STOP_AVI();
		ZeroGS::AddMessage("Failed to create avi");
		return;
	}
#else // linux
	//TODO
#endif
}

