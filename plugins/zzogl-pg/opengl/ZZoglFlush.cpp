/*  ZZ Open GL graphics plugin
 *  Copyright (c)2009 zeydlitz@gmail.com
 *  Based on Zerofrog's ZeroGS KOSMOS (c)2005-2006
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

// Realization of Flush -- drawing function of GS

#include <stdlib.h>

#include "GS.h"
#include "Mem.h"
#include "zerogs.h"
#include "targets.h"

using namespace ZeroGS;

//------------------ Defines
#ifndef DEVBUILD

#define INC_GENVARS()
#define INC_TEXVARS()
#define INC_ALPHAVARS()
#define INC_RESOLVE()

#define g_bUpdateEffect 0
#define g_bSaveTex 0
bool g_bSaveTrans = 0;
#define g_bSaveResolved 0

#else // defined(ZEROGS_DEVBUILD)

#define INC_GENVARS() ++g_nGenVars
#define INC_TEXVARS() ++g_nTexVars
#define INC_ALPHAVARS() ++g_nAlphaVars
#define INC_RESOLVE() ++g_nResolve

bool g_bSaveTrans = 0;
bool g_bUpdateEffect = 0;
bool g_bSaveTex = 0;	// saves the curent texture
bool g_bSaveResolved = 0;
#endif // !defined(ZEROGS_DEVBUILD)

#define STENCIL_ALPHABIT	1	   // if set, dest alpha >= 0x80
#define STENCIL_PIXELWRITE  2	   // if set, pixel just written (reset after every Flush)
#define STENCIL_FBA		 4	   // if set, just written pixel's alpha >= 0 (reset after every Flush)
#define STENCIL_SPECIAL	 8	   // if set, indicates that pixel passed its alpha test (reset after every Flush)
//#define STENCIL_PBE		   16
#define STENCIL_CLEAR	   (2|4|8|16)

void Draw(const VB& curvb)
{
	glDrawArrays(primtype[curvb.curprim.prim], 0, curvb.nCount);
}

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

#define GL_ZTEST(enable) { \
	if (enable) glEnable(GL_DEPTH_TEST); \
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

#define COLORMASK_RED	  	1
#define COLORMASK_GREEN	 	2
#define COLORMASK_BLUE	  	4
#define COLORMASK_ALPHA	 	8
#define GL_COLORMASK(mask) glColorMask(!!((mask)&COLORMASK_RED), !!((mask)&COLORMASK_GREEN), !!((mask)&COLORMASK_BLUE), !!((mask)&COLORMASK_ALPHA))

// ----------------- Types
//------------------ Dummies

//------------------ variables

//extern bool g_bIsLost;
extern int g_nDepthBias;
extern float g_fBlockMult;
bool g_bUpdateStencil = 1;
//u32 g_SaveFrameNum = 0;									// ZZ

int GPU_TEXWIDTH = 512;
float g_fiGPU_TEXWIDTH = 1 / 512.0f;

extern CGprogram g_psprog;							// 2 -- ZZ

// local alpha blending settings
static GLenum s_rgbeq, s_alphaeq; // set by zgsBlendEquationSeparateEXT			// ZZ


static const u32 blendalpha[3] = { GL_SRC_ALPHA, GL_DST_ALPHA, GL_CONSTANT_COLOR_EXT };	// ZZ
static const u32 blendinvalpha[3] = { GL_ONE_MINUS_SRC_ALPHA, GL_ONE_MINUS_DST_ALPHA, GL_ONE_MINUS_CONSTANT_COLOR_EXT }; //ZZ
static const u32 g_dwAlphaCmp[] = { GL_NEVER, GL_ALWAYS, GL_LESS, GL_LEQUAL, GL_EQUAL, GL_GEQUAL, GL_GREATER, GL_NOTEQUAL };    // ZZ

// used for afail case
static const u32 g_dwReverseAlphaCmp[] = { GL_ALWAYS, GL_NEVER, GL_GEQUAL, GL_GREATER, GL_NOTEQUAL, GL_LESS, GL_LEQUAL, GL_EQUAL };
static const u32 g_dwZCmp[] = { GL_NEVER, GL_ALWAYS, GL_GEQUAL, GL_GREATER };

/////////////////////
// graphics resources
#define s_bForceTexFlush 1					// ZZ
static u32 s_ptexCurSet[2] = {0};
static u32 s_ptexNextSet[2] = {0};				// ZZ


extern vector<u32> s_vecTempTextures;		   // temporary textures, released at the end of every frame
extern bool s_bTexFlush;
bool s_bWriteDepth = false;
bool s_bDestAlphaTest = false;
int s_ClutResolve = 0;						// ZZ
int g_nDepthUsed = 0; 						// ffx2 pal movies
int s_nWriteDepthCount = 0;					// ZZ
int s_nWriteDestAlphaTest = 0;					// ZZ

////////////////////
// State parameters
static Vector vAlphaBlendColor;	 // used for GPU_COLOR

static bool bNeedBlendFactorInAlpha;	  // set if the output source alpha is different from the real source alpha (only when blend factor > 0x80)
static u32 s_dwColorWrite = 0xf;			// the color write mask of the current target

typedef union
{
	struct
	{
		u8 _bNeedAlphaColor;		// set if vAlphaBlendColor needs to be set
		u8 _b2XAlphaTest;		   // Only valid when bNeedAlphaColor is set. if 1st bit set set, double all alpha testing values
		// otherwise alpha testing needs to be done separately.
		u8 _bDestAlphaColor;		// set to 1 if blending with dest color (process only one tri at a time). If 2, dest alpha is always 1.
		u8 _bAlphaClamping;	 // if first bit is set, do min; if second bit, do max
	};

	u32 _bAlphaState;
} g_flag_vars;

g_flag_vars g_vars;

//#define bNeedAlphaColor g_vars._bNeedAlphaColor
#define b2XAlphaTest g_vars._b2XAlphaTest
#define bDestAlphaColor g_vars._bDestAlphaColor
#define bAlphaClamping g_vars._bAlphaClamping

int g_PrevBitwiseTexX = -1, g_PrevBitwiseTexY = -1; // textures stored in SAMP_BITWISEANDX and SAMP_BITWISEANDY		// ZZ

//static alphaInfo s_alphaInfo;												// ZZ

extern u8* g_pbyGSClut;
extern int ppf;

int s_nWireframeCount = 0;

//------------------ Namespace

namespace ZeroGS
{

VB vb[2];
float fiTexWidth[2], fiTexHeight[2];	// current tex width and height

u8 s_AAx = 0, s_AAy = 0; // if AAy is set, then AAx has to be set
u8 s_AAz = 0, s_AAw = 0; // if AAy is set, then AAx has to be set

int icurctx = -1;

extern CRangeManager s_RangeMngr; // manages overwritten memory				// zz
void FlushTransferRanges(const tex0Info* ptex);						//zz

RenderFormatType GetRenderFormat() { return g_RenderFormatType; }					//zz

// use to update the state
void SetTexVariables(int context, FRAGMENTSHADER* pfragment);			// zz
void SetTexInt(int context, FRAGMENTSHADER* pfragment, int settexint);		// zz
void SetAlphaVariables(const alphaInfo& ainfo);					// zzz
void ResetAlphaVariables();

inline void SetAlphaTestInt(pixTest curtest);

inline void RenderAlphaTest(const VB& curvb, CGparameter sOneColor);
inline void RenderStencil(const VB& curvb, u32 dwUsingSpecialTesting);
inline void ProcessStencil(const VB& curvb);
inline void RenderFBA(const VB& curvb, CGparameter sOneColor);
inline void ProcessFBA(const VB& curvb, CGparameter sOneColor);			// zz


}

//------------------ Code

inline float AlphaReferedValue(int aref)
{
	return b2XAlphaTest ? min(1.0f, (float)aref / 127.5f) : (float)aref / 255.0f ;
}

inline void SetAlphaTest(const pixTest& curtest)
{
	// if s_dwColorWrite is nontrivial, than we should not off alphatest.
	// This fix GOW and Okami.
	if (!curtest.ate && USEALPHATESTING && (s_dwColorWrite != 2 && s_dwColorWrite != 14))
	{
		glDisable(GL_ALPHA_TEST);
	}
	else
	{
		glEnable(GL_ALPHA_TEST);
		glAlphaFunc(g_dwAlphaCmp[curtest.atst], AlphaReferedValue(curtest.aref));
	}
}

// Switch wireframe rendering off for first flush, so it's draw few solid primitives
inline void SwitchWireframeOff()
{
	if (conf.wireframe())
	{
		if (s_nWireframeCount > 0)
		{
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		}
	}
}

// Switch wireframe rendering on, look at previous function
inline void SwitchWireframeOn()
{
	if (conf.wireframe())
	{
		if (s_nWireframeCount > 0)
		{
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			--s_nWireframeCount;
		}
	}
}

int GetTexFilter(const tex1Info& tex1)
{
	// always force
	if (conf.bilinear == 2) return 1;

	int texfilter = 0;

	if (conf.bilinear && ptexBilinearBlocks != 0)
	{
		if (tex1.mmin <= 1)
			texfilter = tex1.mmin | tex1.mmag;
		else
			texfilter = tex1.mmag ? ((tex1.mmin + 2) & 5) : tex1.mmin;

		texfilter = texfilter == 1 || texfilter == 4 || texfilter == 5;
	}

	return texfilter;
}

void ZeroGS::ReloadEffects()
{
#ifdef ZEROGS_DEVBUILD

	for (int i = 0; i < ARRAY_SIZE(ppsTexture); ++i)
	{
		SAFE_RELEASE_PROG(ppsTexture[i].prog);
	}

	memset(ppsTexture, 0, sizeof(ppsTexture));

	LoadExtraEffects();
#endif
}

long BufferNumber = 0;

// This is a debug function. It prints all buffer info and save current texture into the file, then prints the file name.
inline void VisualBufferMessage(int context)
{
#if defined(WRITE_PRIM_LOGS) && defined(_DEBUG)
	BufferNumber++;
	ZeroGS::VB& curvb = vb[context];
	static const char* patst[8] = { "NEVER", "ALWAYS", "LESS", "LEQUAL", "EQUAL", "GEQUAL", "GREATER", "NOTEQUAL"};
	static const char* pztst[4] = { "NEVER", "ALWAYS", "GEQUAL", "GREATER" };
	static const char* pafail[4] = { "KEEP", "FB_ONLY", "ZB_ONLY", "RGB_ONLY" };
	ZZLog::Debug_Log("**Drawing ctx %d, num %d, fbp: 0x%x, zbp: 0x%x, fpsm: %d, zpsm: %d, fbw: %d", context, vb[context].nCount, curvb.prndr->fbp, curvb.zbuf.zbp, curvb.prndr->psm, curvb.zbuf.psm, curvb.prndr->fbw);
	ZZLog::Debug_Log("prim: prim=%x iip=%x tme=%x fge=%x abe=%x aa1=%x fst=%x ctxt=%x fix=%x",
					 curvb.curprim.prim, curvb.curprim.iip, curvb.curprim.tme, curvb.curprim.fge, curvb.curprim.abe, curvb.curprim.aa1, curvb.curprim.fst, curvb.curprim.ctxt, curvb.curprim.fix);
	ZZLog::Debug_Log("test: ate:%d, atst: %s, aref: %d, afail: %s, date: %d, datm: %d, zte: %d, ztst: %s, fba: %d",
					 curvb.test.ate, patst[curvb.test.atst], curvb.test.aref, pafail[curvb.test.afail], curvb.test.date, curvb.test.datm, curvb.test.zte, pztst[curvb.test.ztst], curvb.fba.fba);
	ZZLog::Debug_Log("alpha: A%d B%d C%d D%d FIX:%d pabe: %d; aem: %d, ta0: %d, ta1: %d\n", curvb.alpha.a, curvb.alpha.b, curvb.alpha.c, curvb.alpha.d, curvb.alpha.fix, gs.pabe, gs.texa.aem, gs.texa.ta[0], gs.texa.ta[1]);
	ZZLog::Debug_Log("tex0: tbp0=0x%x, tbw=%d, psm=0x%x, tw=%d, th=%d, tcc=%d, tfx=%d, cbp=0x%x, cpsm=0x%x, csm=%d, csa=%d, cld=%d",
					 curvb.tex0.tbp0, curvb.tex0.tbw, curvb.tex0.psm, curvb.tex0.tw,
					 curvb.tex0.th, curvb.tex0.tcc, curvb.tex0.tfx, curvb.tex0.cbp,
					 curvb.tex0.cpsm, curvb.tex0.csm, curvb.tex0.csa, curvb.tex0.cld);
	char* Name;
//	if (g_bSaveTex) {
//		if (g_bSaveTex == 1)
	Name = NamedSaveTex(&curvb.tex0, 1);
//		else
//			Name = NamedSaveTex(&curvb.tex0, 0);
	ZZLog::Error_Log("TGA name '%s'.", Name);
	free(Name);
//	}
//	ZZLog::Debug_Log("frame: %d, buffer %ld.\n", g_SaveFrameNum, BufferNumber);
	ZZLog::Debug_Log("buffer %ld.\n", BufferNumber);
#endif
}

inline void SaveRendererTarget(VB& curvb)
{
#ifdef _DEBUG

//	if (g_bSaveFlushedFrame & 0x80000000)
//	{
//		char str[255];
//		sprintf(str, "rndr%d.tga", g_SaveFrameNum);
//		SaveRenderTarget(str, curvb.prndr->fbw, curvb.prndr->fbh, 0);
//	}

#endif
}

// Stop effects in Developers mode
inline void FlushUpdateEffect()
{
#if defined(DEVBUILD)

	if (g_bUpdateEffect)
	{
		ReloadEffects();
		g_bUpdateEffect = 0;
	}

#endif
}

// Check, maybe we cold skip flush
inline bool IsFlushNoNeed(VB& curvb, const pixTest& curtest)
{
	if (curvb.nCount == 0 || (curtest.zte && curtest.ztst == 0) /*|| g_bIsLost*/)
	{
		curvb.nCount = 0;
		return true;
	}

	return false;
}

// Transfer targets, that are located in current texture.
inline void FlushTransferRangesHelper(VB& curvb)
{
	if (s_RangeMngr.ranges.size() > 0)
	{
		// don't want infinite loop, so set nCount to 0.
		u32 prevcount = curvb.nCount;
		curvb.nCount = 0;

		FlushTransferRanges(curvb.curprim.tme ? &curvb.tex0 : NULL);

		curvb.nCount += prevcount;
	}
}

// If set bit for texture checking, do it. Maybe it's all.
inline bool FushTexDataHelper(VB& curvb)
{
	if (curvb.bNeedFrameCheck || curvb.bNeedZCheck)
	{
		curvb.CheckFrame(curvb.curprim.tme ? curvb.tex0.tbp0 : 0);
	}

	if (curvb.bNeedTexCheck)    // Zeydlitz want to try this
	{
		curvb.FlushTexData();

		if (curvb.nCount == 0) return true;
	}

	return false;
}

// Null target mean that we do something really bad.
inline bool FlushCheckForNULLTarget(VB& curvb, int context)
{
	if ((curvb.prndr == NULL) || (curvb.pdepth == NULL))
	{
		ERROR_LOG_SPAMA("Current render target NULL (ctx: %d)", context);
		curvb.nCount = 0;
		return true;
	}

	return false;
}

// O.k. A set of resolutions, we do before real flush. We do RangeManager, FrameCheck and
// ZCheck before this.
inline bool FlushInitialTest(VB& curvb, const pixTest& curtest, int context)
{
	GL_REPORT_ERRORD();
	assert(context >= 0 && context <= 1);

	FlushUpdateEffect();

	if (IsFlushNoNeed(curvb, curtest)) return true;

	FlushTransferRangesHelper(curvb);

	if (FushTexDataHelper(curvb)) return true;

	GL_REPORT_ERRORD();

	if (FlushCheckForNULLTarget(curvb, context)) return true;

	return false;
}

// Try to different approach if texture target was not found
inline CRenderTarget* FlushReGetTarget(int& tbw, int& tbp0, int& tpsm, VB& curvb)
{
	// This was incorrect code
	CRenderTarget* ptextarg = NULL;

	if ((ptextarg == NULL) && (tpsm == PSMT8) && (conf.settings().reget))
	{
		// check for targets with half the width. Break Valkyrie Chronicles
		ptextarg = s_RTs.GetTarg(tbp0, tbw / 2, curvb);

		if (ptextarg == NULL)
		{
			tbp0 &= ~0x7ff;
			ptextarg = s_RTs.GetTarg(tbp0, tbw / 2, curvb); // mgs3 hack

			if (ptextarg == NULL)
			{
				// check the next level (mgs3)
				tbp0 &= ~0xfff;
				ptextarg = s_RTs.GetTarg(tbp0, tbw / 2, curvb); // mgs3 hack
			}

			if (ptextarg != NULL && ptextarg->start > tbp0*256)
			{
				// target beyond range, so ignore
				ptextarg = NULL;
			}
		}
	}


	if (PSMT_ISZTEX(tpsm) && (ptextarg == NULL))
	{
		// try depth
		ptextarg = s_DepthRTs.GetTarg(tbp0, tbw, curvb);
	}

	if ((ptextarg == NULL) && (conf.settings().texture_targs))
	{
		// check if any part of the texture intersects the current target
		if (!PSMT_ISCLUT(tpsm) && (curvb.tex0.tbp0 >= curvb.frame.fbp) && ((curvb.tex0.tbp0) < curvb.prndr->end))
		{
			ptextarg = curvb.prndr;
		}
	}

#ifdef _DEBUG
	if (tbp0 == 0x3600 && tbw == 0x100)
	{
		if (ptextarg == NULL)
		{
			printf("Miss %x 0x%x %d\n", tbw, tbp0, tpsm);

			typedef map<u32, CRenderTarget*> MAPTARGETS;

			for (MAPTARGETS::iterator itnew = s_RTs.mapTargets.begin(); itnew != s_RTs.mapTargets.end(); ++itnew)
			{
				printf("\tRender %x 0x%x %x\n", itnew->second->fbw, itnew->second->fbp, itnew->second->psm);
			}

			for (MAPTARGETS::iterator itnew = s_DepthRTs.mapTargets.begin(); itnew != s_DepthRTs.mapTargets.end(); ++itnew)
			{
				printf("\tDepth %x 0x%x %x\n", itnew->second->fbw, itnew->second->fbp, itnew->second->psm);
			}

			printf("\tCurvb 0x%x 0x%x 0x%x %x\n", curvb.frame.fbp, curvb.prndr->end, curvb.prndr->fbp, curvb.prndr->fbw);
		}
		else
			printf("Hit  %x 0x%x %x\n", tbw, tbp0, tpsm);
	}

#endif

	return ptextarg;
}

// Find target to draw a texture, it's highly p
inline CRenderTarget* FlushGetTarget(VB& curvb)
{
	int tbw, tbp0, tpsm;

	CRenderTarget* ptextarg = NULL;

	if (!curvb.curprim.tme) return ptextarg;

	if (curvb.bNeedTexCheck)
	{
		printf("How it is possible?\n");
		// not yet initied, but still need to get correct target! (xeno3 ingame)
		tbp0 = ZZOglGet_tbp0_TexBits(curvb.uNextTex0Data[0]);
		tbw  = ZZOglGet_tbw_TexBitsMult(curvb.uNextTex0Data[0]);
		tpsm = ZZOglGet_psm_TexBitsFix(curvb.uNextTex0Data[0]);
	}
	else
	{
		tbw = curvb.tex0.tbw;
		tbp0 = curvb.tex0.tbp0;
		tpsm = curvb.tex0.psm;
	}

	ptextarg = s_RTs.GetTarg(tbp0, tbw, curvb);

	if (ptextarg == NULL)
		ptextarg = FlushReGetTarget(tbw, tbp0, tpsm, curvb);

	if ((ptextarg != NULL) && !(ptextarg->status & CRenderTarget::TS_NeedUpdate))
	{
		if (PSMT_BITMODE(tpsm) == 4)   // handle 8h cluts
		{
			// don't support clut targets, read from mem
			// 4hl - kh2 check - from dx version -- arcum42

			if (tpsm == PSMT4 && s_ClutResolve <= 1)
			{
				// xenosaga requires 2 resolves
				u32 prevcount = curvb.nCount;
				curvb.nCount = 0;
				ptextarg->Resolve();
				s_ClutResolve++;
				curvb.nCount += prevcount;
			}

			ptextarg = NULL;
		}
		else
		{
			if (ptextarg == curvb.prndr)
			{
				// need feedback
				curvb.prndr->CreateFeedback();

				if (s_bWriteDepth && (curvb.pdepth != NULL))
					curvb.pdepth->SetRenderTarget(1);
				else
					ResetRenderTarget(1);
			}
		}
	}
	else 
	{
		ptextarg = NULL;
	}

	return ptextarg;
}

// Set target for current context
inline void FlushSetContextTarget(VB& curvb, int context)
{
	if (!curvb.bVarsSetTarg)
	{
		SetContextTarget(context);
	}
	else
	{
		assert(curvb.pdepth != NULL);

		if (curvb.pdepth->status & CRenderTarget::TS_Virtual)
		{
			if (!curvb.zbuf.zmsk)
			{
				CRenderTarget* ptemp = s_DepthRTs.Promote(GetFrameKey(curvb.pdepth));
				assert(ptemp == curvb.pdepth);
			}
			else
			{
				curvb.pdepth->status &= ~CRenderTarget::TS_NeedUpdate;
			}
		}

		if ((curvb.pdepth->status & CRenderTarget::TS_NeedUpdate) || (curvb.prndr->status & CRenderTarget::TS_NeedUpdate))
			SetContextTarget(context);
	}

	assert(!(curvb.prndr->status&CRenderTarget::TS_NeedUpdate));

	curvb.prndr->status = 0;

	if (curvb.pdepth != NULL)
	{
#ifdef _DEBUG
		// Reduce an assert to a warning.
		if (curvb.pdepth->status & CRenderTarget::TS_NeedUpdate)
		{
			ZZLog::Debug_Log("In FlushSetContextTarget, pdepth has TS_NeedUpdate set.");
		}
#endif
		if (!curvb.zbuf.zmsk)
		{
			assert(!(curvb.pdepth->status & CRenderTarget::TS_Virtual));
			curvb.pdepth->status = 0;
		}
	}
}

inline void FlushSetStream(VB& curvb)
{
	glBindBuffer(GL_ARRAY_BUFFER, g_vboBuffers[g_nCurVBOIndex]);
	g_nCurVBOIndex = (g_nCurVBOIndex + 1) % g_vboBuffers.size();
	glBufferData(GL_ARRAY_BUFFER, curvb.nCount * sizeof(VertexGPU), curvb.pBufferData, GL_STREAM_DRAW);
//	void* pdata = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
//	memcpy_amd(pdata, curvb.pBufferData, curvb.nCount * sizeof(VertexGPU));
//	glUnmapBuffer(GL_ARRAY_BUFFER);
	SET_STREAM();
	
	GL_REPORT_ERRORD();
}

int SetMaskR = 0x0;
int SetMaskG = 0x0;
int SetMaskB = 0x0;
// Set color mask. Really, it's not as good as PS2 one.
inline void FlushSetColorMask(VB& curvb)
{
	s_dwColorWrite = (PSMT_BITMODE(curvb.prndr->psm) == 1) ? (COLORMASK_BLUE | COLORMASK_GREEN | COLORMASK_RED) : 0xf;

	int maskR = ZZOglGet_fbmRed_FrameBits(curvb.frame.fbm);
	int maskG = ZZOglGet_fbmGreen_FrameBits(curvb.frame.fbm);
	int maskB = ZZOglGet_fbmBlue_FrameBits(curvb.frame.fbm);
	int maskA = ZZOglGet_fbmAlpha_FrameBits(curvb.frame.fbm);

	if (maskR == 0xff) s_dwColorWrite &= ~COLORMASK_RED;
	if (maskG == 0xff) s_dwColorWrite &= ~COLORMASK_GREEN;
	if (maskB == 0xff) s_dwColorWrite &= ~COLORMASK_BLUE;

	if ((maskA == 0xff) || (curvb.curprim.abe && (curvb.test.atst == 2 && curvb.test.aref == 128)))
		s_dwColorWrite &= ~COLORMASK_ALPHA;

	GL_COLORMASK(s_dwColorWrite);
}

// Set Scissors for scissor test.
inline void FlushSetScissorRect(VB& curvb)
{
	Rect& scissor = curvb.prndr->scissorrect;
	glScissor(scissor.x, scissor.y, scissor.w, scissor.h);
}

// Prior really doing something check context
inline void FlushDoContextJob(VB& curvb, int context)
{
	SaveRendererTarget(curvb);

	FlushSetContextTarget(curvb, context);
	icurctx = context;

	FlushSetStream(curvb);
	FlushSetColorMask(curvb);
	FlushSetScissorRect(curvb);
}

// Set 1 is Alpha test is EQUAL and alpha should be proceed with care.
inline int FlushGetExactcolor(const pixTest curtest)
{
	if (!(g_nPixelShaderVer&SHADER_REDUCED))
		// ffx2 breaks when ==7
		return ((curtest.ate && curtest.aref <= 128) && (curtest.atst == 4));//||curtest.atst==7);

	return 0;
}

// fill the buffer by decoding the clut
inline void FlushDecodeClut(VB& curvb, GLuint& ptexclut)
{
	glGenTextures(1, &ptexclut);
	glBindTexture(GL_TEXTURE_2D, ptexclut);
	vector<char> data(PSMT_ISHALF_STORAGE(curvb.tex0) ? 512 : 1024);

	if (ptexclut != 0)
	{

		int nClutOffset = 0, clutsize;
		int entries = PSMT_IS8CLUT(curvb.tex0.psm) ? 256 : 16;

		if (curvb.tex0.csm && curvb.tex0.csa)
			printf("ERROR, csm1\n");

		if (PSMT_IS32BIT(curvb.tex0.cpsm))   // 32 bit
		{
			nClutOffset = 64 * curvb.tex0.csa;
			clutsize = min(entries, 256 - curvb.tex0.csa * 16) * 4;
		}
		else
		{
			nClutOffset = 64 * (curvb.tex0.csa & 15) + (curvb.tex0.csa >= 16 ? 2 : 0);
			clutsize = min(entries, 512 - curvb.tex0.csa * 16) * 2;
		}

		if (PSMT_IS32BIT(curvb.tex0.cpsm))   // 32 bit
		{
			memcpy_amd(&data[0], g_pbyGSClut + nClutOffset, clutsize);
		}
		else
		{
			u16* pClutBuffer = (u16*)(g_pbyGSClut + nClutOffset);
			u16* pclut = (u16*) & data[0];
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

		GLenum tempType = PSMT_ISHALF_STORAGE(curvb.tex0) ? GL_UNSIGNED_SHORT_5_5_5_1 : GL_UNSIGNED_BYTE;
		Texture2D(4, 256, 1, GL_RGBA, tempType, &data[0]);
		
		s_vecTempTextures.push_back(ptexclut);

		if (g_bSaveTex) SaveTexture("clut.tga", GL_TEXTURE_2D, ptexclut, 256, 1);

		setTex2DWrap(GL_REPEAT);
		setTex2DFilters(GL_LINEAR);
	}
}

inline int FlushGetShaderType(VB& curvb, CRenderTarget* ptextarg, GLuint& ptexclut)
{
	if (PSMT_ISCLUT(curvb.tex0.psm) && !(conf.settings().no_target_clut))
	{
		FlushDecodeClut(curvb, ptexclut);

		if (!(g_nPixelShaderVer&SHADER_REDUCED) && PSMT_ISHALF(ptextarg->psm))
		{
			return 4;
		}
		else
		{
			// Valkyrie
			return 2;
		}
	}

	if (PSMT_ISHALF_STORAGE(curvb.tex0) != PSMT_ISHALF(ptextarg->psm) && (!(g_nPixelShaderVer&SHADER_REDUCED) || !curvb.curprim.fge))
	{
		if (PSMT_ISHALF_STORAGE(curvb.tex0))
		{
			// converting from 32->16
			// Radiata Chronicles
			return 3;
		}
		else
		{
			// converting from 16->32
			// Star Ward: Force
			return 0;
		}
	}

	return 1;
}


//Set page offsets depends on shader type.
inline Vector FlushSetPageOffset(FRAGMENTSHADER* pfragment, int shadertype, CRenderTarget* ptextarg)
{
	SetShaderCaller("FlushSetPageOffset");

	Vector vpageoffset;
	vpageoffset.w = 0;

	switch (shadertype)
	{
		case 3:
			vpageoffset.x = -0.1f / 256.0f;
			vpageoffset.y = -0.001f / 256.0f;
			vpageoffset.z = -0.1f / (ptextarg->fbh);
			vpageoffset.w = 0.0f;
			break;

		case 4:
			vpageoffset.x = 2;
			vpageoffset.y = 1;
			vpageoffset.z = 0;
			vpageoffset.w = 0.0001f;
			break;
	}

	// zoe2
	if (PSMT_ISZTEX(ptextarg->psm)) vpageoffset.w = -1.0f;

	ZZcgSetParameter4fv(pfragment->fPageOffset, vpageoffset, "g_fPageOffset");

	return vpageoffset;
}

//Set texture offsets depends omn shader type.
inline Vector FlushSetTexOffset(FRAGMENTSHADER* pfragment, int shadertype, VB& curvb, CRenderTarget* ptextarg)
{
	SetShaderCaller("FlushSetTexOffset");
	Vector v;

	if (shadertype == 3)
	{
		Vector v;
		v.x = 16.0f / (float)curvb.tex0.tw;
		v.y = 16.0f / (float)curvb.tex0.th;
		v.z = 0.5f * v.x;
		v.w = 0.5f * v.y;
		ZZcgSetParameter4fv(pfragment->fTexOffset, v, "g_fTexOffset");
	}
	else if (shadertype == 4)
	{
		Vector v;
		v.x = 16.0f / (float)ptextarg->fbw;
		v.y = 16.0f / (float)ptextarg->fbh;
		v.z = -1;
		v.w = 8.0f / (float)ptextarg->fbh;
		ZZcgSetParameter4fv(pfragment->fTexOffset, v, "g_fTexOffset");
	}

	return v;
}

// Set dimension (Real!) of texture. z and w
inline Vector FlushTextureDims(FRAGMENTSHADER* pfragment, int shadertype, VB& curvb, CRenderTarget* ptextarg)
{
	SetShaderCaller("FlushTextureDims");
	Vector vTexDims;
	vTexDims.x = (float)RW(curvb.tex0.tw) ;
	vTexDims.y = (float)RH(curvb.tex0.th) ;

	// look at the offset of tbp0 from fbp

	if (curvb.tex0.tbp0 <= ptextarg->fbp)
	{
		vTexDims.z = 0;//-0.5f/(float)ptextarg->fbw;
		vTexDims.w = 0;//0.2f/(float)ptextarg->fbh;
	}
	else
	{
		//u32 tbp0 = curvb.tex0.tbp0 >> 5; // align to a page
		int blockheight =  PSMT_ISHALF(ptextarg->psm) ? 64 : 32;
		int ycoord = ((curvb.tex0.tbp0 - ptextarg->fbp) / (32 * (ptextarg->fbw >> 6))) * blockheight;
		int xcoord = (((curvb.tex0.tbp0 - ptextarg->fbp) % (32 * (ptextarg -> fbw >> 6)))) * 2;
		vTexDims.z = (float)xcoord;
		vTexDims.w = (float)ycoord;
	}

	if (shadertype == 4)
		vTexDims.z += 8.0f;

	ZZcgSetParameter4fv(pfragment->fTexDims, vTexDims, "g_fTexDims");

	return vTexDims;
}

// Apply TEX1 mmag and mmin -- filter for expanding/reducing texture
// We ignore all settings, only NEAREST (0) is used
inline void FlushApplyResizeFilter(VB& curvb, u32& dwFilterOpts, CRenderTarget* ptextarg, int context)
{
	u32 ptexset = (ptextarg == curvb.prndr) ? ptextarg->ptexFeedback : ptextarg->ptex;
	s_ptexCurSet[context] = ptexset;

	if ((!curvb.tex1.mmag) || (!curvb.tex1.mmin))
		glBindTexture(GL_TEXTURE_RECTANGLE_NV, ptexset);

	if (!curvb.tex1.mmag)
	{
		glTexParameteri(GL_TEXTURE_RECTANGLE_NV, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		dwFilterOpts |= 1;
	}

	if (!curvb.tex1.mmin)
	{
		glTexParameteri(GL_TEXTURE_RECTANGLE_NV, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		dwFilterOpts |= 2;
	}
}


// Usage existing targets depends on several tricks, 32-16 conversion and CLUTing, so we need to handle it.
inline FRAGMENTSHADER* FlushUseExistRenderTarget(VB& curvb, CRenderTarget* ptextarg, u32& dwFilterOpts, int exactcolor, int context)
{
	if (ptextarg->IsDepth())
		SetWriteDepth();

	GLuint ptexclut = 0;

	//int psm = GetTexCPSM(curvb.tex0);
	int shadertype = FlushGetShaderType(curvb, ptextarg, ptexclut);

	FRAGMENTSHADER* pfragment = LoadShadeEffect(shadertype, 0, curvb.curprim.fge,
								IsAlphaTestExpansion(curvb), exactcolor, curvb.clamp, context, NULL);

	Vector vpageoffset = FlushSetPageOffset(pfragment, shadertype, ptextarg);

	Vector v = FlushSetTexOffset(pfragment, shadertype, curvb, ptextarg);

	Vector vTexDims = FlushTextureDims(pfragment, shadertype, curvb, ptextarg);

	if (pfragment->sCLUT != NULL && ptexclut != 0)
	{
		cgGLSetTextureParameter(pfragment->sCLUT, ptexclut);
		cgGLEnableTextureParameter(pfragment->sCLUT);
	}

	FlushApplyResizeFilter(curvb, dwFilterOpts, ptextarg, context);

	if (g_bSaveTex)
		SaveTexture("tex.tga", GL_TEXTURE_RECTANGLE_NV,
					ptextarg == curvb.prndr ? ptextarg->ptexFeedback : ptextarg->ptex, RW(ptextarg->fbw), RH(ptextarg->fbh));

	return pfragment;
}

// Usage most major shader.
inline FRAGMENTSHADER* FlushMadeNewTarget(VB& curvb, int exactcolor, int context)
{
	// save the texture
	if (g_bSaveTex)
	{
		if (g_bSaveTex == 1)
		{
			SaveTex(&curvb.tex0, 1);
			/*CMemoryTarget* pmemtarg = */
			g_MemTargs.GetMemoryTarget(curvb.tex0, 0);
		}
		else 
		{
			SaveTex(&curvb.tex0, 0);
		}
	}

	FRAGMENTSHADER* pfragment = LoadShadeEffect(0, GetTexFilter(curvb.tex1), curvb.curprim.fge,
								IsAlphaTestExpansion(curvb), exactcolor, curvb.clamp, context, NULL);

	if (pfragment == NULL)
		ZZLog::Error_Log("Could not find memory target shader.");

	return pfragment;
}

// We made an shader, so now need to put all common variables.
inline void FlushSetTexture(VB& curvb, FRAGMENTSHADER* pfragment, CRenderTarget* ptextarg, int context)
{
	SetTexVariables(context, pfragment);
	SetTexInt(context, pfragment, ptextarg == NULL);

	// have to enable the texture parameters(curtest.atst=

	if (curvb.ptexClamp[0] != 0)
	{
		cgGLSetTextureParameter(pfragment->sBitwiseANDX, curvb.ptexClamp[0]);
		cgGLEnableTextureParameter(pfragment->sBitwiseANDX);
	}

	if (curvb.ptexClamp[1] != 0)
	{
		cgGLSetTextureParameter(pfragment->sBitwiseANDY, curvb.ptexClamp[1]);
		cgGLEnableTextureParameter(pfragment->sBitwiseANDY);
	}

	if (pfragment->sMemory != NULL && s_ptexCurSet[context] != 0)
	{
		cgGLSetTextureParameter(pfragment->sMemory, s_ptexCurSet[context]);
		cgGLEnableTextureParameter(pfragment->sMemory);
	}
}

// Reset programm and texture variables;
inline void FlushBindProgramm(FRAGMENTSHADER* pfragment, int context)
{
	vb[context].bTexConstsSync = 0;
	vb[context].bVarsTexSync = 0;

	cgGLBindProgram(pfragment->prog);
	g_psprog = pfragment->prog;
}

inline FRAGMENTSHADER* FlushRendererStage(VB& curvb, u32& dwFilterOpts, CRenderTarget* ptextarg, int exactcolor, int context)
{

	FRAGMENTSHADER* pfragment = NULL;

	// set the correct pixel shaders

	if (curvb.curprim.tme)
	{
		if (ptextarg != NULL)
			pfragment = FlushUseExistRenderTarget(curvb, ptextarg, dwFilterOpts, exactcolor, context);
		else
			pfragment = FlushMadeNewTarget(curvb, exactcolor, context);

		if (pfragment == NULL)
		{
			ZZLog::Error_Log("Shader is not found.");
//			return NULL;
		}

		FlushSetTexture(curvb, pfragment, ptextarg, context);
	}
	else
	{
		pfragment = &ppsRegular[curvb.curprim.fge + 2 * s_bWriteDepth];
	}

	GL_REPORT_ERRORD();

	// set the shaders
	SetShaderCaller("FlushRendererStage")	;
	SETVERTEXSHADER(pvs[2 * ((curvb.curprim._val >> 1) & 3) + 8 * s_bWriteDepth + context]);
	FlushBindProgramm(pfragment, context);

	GL_REPORT_ERRORD();
	return pfragment;
}

inline bool AlphaCanRenderStencil(VB& curvb)
{
	return g_bUpdateStencil && (PSMT_BITMODE(curvb.prndr->psm) != 1) &&
		   !ZZOglGet_fbmHighByte(curvb.frame.fbm) && !(conf.settings().no_stencil);
}

inline void AlphaSetStencil(bool DoIt)
{
	if (DoIt)
	{
		glEnable(GL_STENCIL_TEST);
		GL_STENCILFUNC(GL_ALWAYS, 0, 0);
		glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
	}
	else glDisable(GL_STENCIL_TEST);
}

inline void AlphaSetDepthTest(VB& curvb, const pixTest curtest, FRAGMENTSHADER* pfragment)
{
	glDepthMask(!curvb.zbuf.zmsk && curtest.zte);
	//	&& curtest.zte &&  (curtest.ztst > 1) );

	if (curtest.zte)
	{
		if (curtest.ztst > 1) g_nDepthUsed = 2;
		if ((curtest.ztst == 2) ^(g_nDepthBias != 0))
		{
			g_nDepthBias = (curtest.ztst == 2);
			//SETRS(D3DRS_DEPTHBIAS, g_nDepthBias?FtoDW(0.0003f):FtoDW(0.000015f));
		}

		glDepthFunc(g_dwZCmp[curtest.ztst]);
	}

	GL_ZTEST(curtest.zte);

//	glEnable (GL_POLYGON_OFFSET_FILL);
//        glPolygonOffset (-1., -1.);

	if (s_bWriteDepth)
	{
		if (!curvb.zbuf.zmsk)
			curvb.pdepth->SetRenderTarget(1);
		else
			ResetRenderTarget(1);
	}
}

inline u32 AlphaSetupBlendTest(VB& curvb)
{
	if (curvb.curprim.abe)
		SetAlphaVariables(curvb.alpha);
	else
		glDisable(GL_BLEND);

	u32 oldabe = curvb.curprim.abe;

	if (gs.pabe)
	{
		//ZZLog::Error_Log("PABE!");
		curvb.curprim.abe = 1;
		glEnable(GL_BLEND);
	}

	return oldabe;
}

inline void AlphaRenderFBA(VB& curvb, FRAGMENTSHADER* pfragment, bool s_bDestAlphaTest, bool bCanRenderStencil)
{
	// needs to be before RenderAlphaTest
	if ((gs.pabe) || (curvb.fba.fba && !ZZOglGet_fbmHighByte(curvb.frame.fbm)) || (s_bDestAlphaTest && bCanRenderStencil))
	{
		RenderFBA(curvb, pfragment->sOneColor);
	}

}

inline u32 AlphaRenderAlpha(VB& curvb, const pixTest curtest, FRAGMENTSHADER* pfragment, int exactcolor)
{
	SetShaderCaller("AlphaRenderAlpha");
	u32 dwUsingSpecialTesting = 0;

	if (curvb.curprim.abe)
	{
		if ((bNeedBlendFactorInAlpha || ((curtest.ate && curtest.atst > 1) && (curtest.aref > 0x80))))
		{
			// need special stencil processing for the alpha
			RenderAlphaTest(curvb, pfragment->sOneColor);
			dwUsingSpecialTesting = 1;
		}

		// harvest fishing
		Vector v = vAlphaBlendColor;

		if (exactcolor)
		{
			v.y *= 255;
			v.w *= 255;
		}

		ZZcgSetParameter4fv(pfragment->sOneColor, v, "g_fOneColor");
	}
	else
	{
		// not using blending so set to defaults
		Vector v = exactcolor ? Vector(1, 510 * 255.0f / 256.0f, 0, 0) : Vector(1, 2 * 255.0f / 256.0f, 0, 0);
		ZZcgSetParameter4fv(pfragment->sOneColor, v, "g_fOneColor");

	}

	return dwUsingSpecialTesting;
}

inline void AlphaRenderStencil(VB& curvb, bool s_bDestAlphaTest, bool bCanRenderStencil, u32 dwUsingSpecialTesting)
{
	if (s_bDestAlphaTest && bCanRenderStencil)
	{
		// if not 24bit and can write to high alpha bit
		RenderStencil(curvb, dwUsingSpecialTesting);
	}
	else
	{
		s_stencilref = STENCIL_SPECIAL;
		s_stencilmask = STENCIL_SPECIAL;

		// setup the stencil to only accept the test pixels

		if (dwUsingSpecialTesting)
		{
			glEnable(GL_STENCIL_TEST);
			glStencilMask(STENCIL_PIXELWRITE);
			GL_STENCILFUNC(GL_EQUAL, STENCIL_SPECIAL | STENCIL_PIXELWRITE, STENCIL_SPECIAL);
			glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
		}
	}

#ifdef _DEBUG
	if (bDestAlphaColor == 1)
	{
		ZZLog::Debug_Log("Dest alpha blending! Manipulate alpha here.");
	}

#endif

	if (bCanRenderStencil && gs.pabe)
	{
		// only render the pixels with alpha values >= 0x80
		GL_STENCILFUNC(GL_EQUAL, s_stencilref | STENCIL_FBA, s_stencilmask | STENCIL_FBA);
	}

	GL_REPORT_ERRORD();
}

inline void AlphaTest(VB& curvb)
{
//	printf ("%d %d %d %d %d\n", curvb.test.date, curvb.test.datm, gs.texa.aem, curvb.test.ate, curvb.test.atst );

//	return;
	// Zeydlitz changed this with a reason! It's an "Alpha more than 1 hack."
	if (curvb.test.ate == 1 && curvb.test.atst == 1 && curvb.test.date == 1)
	{
		if (curvb.test.datm == 1)
		{
			glAlphaFunc(GL_GREATER, 1.0f);
		}
		else
		{
			glAlphaFunc(GL_LESS, 1.0f);
			printf("%d %d %d\n", curvb.test.date, curvb.test.datm, gs.texa.aem);
		}
	}

	if (!curvb.test.ate || curvb.test.atst > 0)
	{
		Draw(curvb);
	}

	GL_REPORT_ERRORD();
}

inline void AlphaPabe(VB& curvb, FRAGMENTSHADER* pfragment, int exactcolor)
{
	if (gs.pabe)
	{
		SetShaderCaller("AlphaPabe");
		// only render the pixels with alpha values < 0x80
		glDisable(GL_BLEND);
		GL_STENCILFUNC_SET();

		Vector v;
		v.x = 1;
		v.y = 2;
		v.z = 0;
		v.w = 0;

		if (exactcolor) v.y *= 255;

		ZZcgSetParameter4fv(pfragment->sOneColor, v, "g_fOneColor");

		Draw(curvb);

		// reset
		if (!s_stencilmask) s_stencilfunc = GL_ALWAYS;

		GL_STENCILFUNC_SET();
	}

	GL_REPORT_ERRORD();
}

// Alpha Failure does not work properly on this cases. True means that no failure job should be done.
// First three cases are trivial manual.
inline bool AlphaFailureIgnore(const pixTest curtest)
{
	if ((!curtest.ate) || (curtest.atst == 1) || (curtest.afail == 0)) return true;

	if (conf.settings().no_alpha_fail && ((s_dwColorWrite < 8) || (s_dwColorWrite == 15 && curtest.atst == 5 && (curtest.aref == 64))))
		return true;

//	old and seemingly incorrect code.
//	if ((s_dwColorWrite < 8 && s_dwColorWrite !=8) && curtest.afail == 1)
//		return true;
//	if ((s_dwColorWrite == 0xf) && curtest.atst == 5 && curtest.afail == 1 && !(conf.settings() & GAME_REGETHACK))
//		return true;
	return false;
}

// more work on alpha failure case
inline void AlphaFailureTestJob(VB& curvb, const pixTest curtest,  FRAGMENTSHADER* pfragment, int exactcolor, bool bCanRenderStencil, int oldabe)
{
	// Note, case when ate == 1, atst == 0 and afail > 0 in documentation wrote as failure case. But it seems that
	// either doc's are incorrect or this case has some issues.
	if (AlphaFailureIgnore(curtest)) return;

#ifdef NOALFAFAIL
	ZZLog::Error_Log("Alpha job here %d %d %d %d %d %d", s_dwColorWrite, curtest.atst, curtest.afail, curtest.aref, gs.pabe, s_bWriteDepth);

//	return;
#endif

	SetShaderCaller("AlphaFailureTestJob");

	// need to reverse the test and disable some targets
	glAlphaFunc(g_dwReverseAlphaCmp[curtest.atst], AlphaReferedValue(curtest.aref));

	if (curtest.afail & 1)     // front buffer update only
	{
		if (curtest.afail == 3) glColorMask(1, 1, 1, 0);// disable alpha

		glDepthMask(0);

		if (s_bWriteDepth) ResetRenderTarget(1);
	}
	else
	{
		// zbuffer update only
		glColorMask(0, 0, 0, 0);
	}

	if (gs.pabe && bCanRenderStencil)
	{
		// only render the pixels with alpha values >= 0x80
		Vector v = vAlphaBlendColor;

		if (exactcolor) { v.y *= 255; v.w *= 255; }

		ZZcgSetParameter4fv(pfragment->sOneColor, v, "g_fOneColor");

		glEnable(GL_BLEND);
		GL_STENCILFUNC(GL_EQUAL, s_stencilref | STENCIL_FBA, s_stencilmask | STENCIL_FBA);
	}

	Draw(curvb);

	GL_REPORT_ERRORD();

	if (gs.pabe)
	{
		// only render the pixels with alpha values < 0x80
		glDisable(GL_BLEND);
		GL_STENCILFUNC_SET();

		Vector v;
		v.x = 1;
		v.y = 2;
		v.z = 0;
		v.w = 0;

		if (exactcolor) v.y *= 255;

		ZZcgSetParameter4fv(pfragment->sOneColor, v, "g_fOneColor");

		Draw(curvb);

		// reset
		if (oldabe) glEnable(GL_BLEND);

		if (!s_stencilmask) s_stencilfunc = GL_ALWAYS;

		GL_STENCILFUNC_SET();
	}

	// restore
	if ((curtest.afail & 1) && !curvb.zbuf.zmsk)
	{
		glDepthMask(1);

		if (s_bWriteDepth)
		{
			assert(curvb.pdepth != NULL);
			curvb.pdepth->SetRenderTarget(1);
		}
	}

	GL_COLORMASK(s_dwColorWrite);

	// not needed anymore since rest of ops concentrate on image processing

	GL_REPORT_ERRORD();
}

inline void AlphaSpecialTesting(VB& curvb, FRAGMENTSHADER* pfragment, u32 dwUsingSpecialTesting, int exactcolor)
{
	if (dwUsingSpecialTesting)
	{
		SetShaderCaller("AlphaSpecialTesting");

		// render the real alpha
		glDisable(GL_ALPHA_TEST);
		glColorMask(0, 0, 0, 1);

		if (s_bWriteDepth)
		{
			ResetRenderTarget(1);
		}

		glDepthMask(0);

		glStencilFunc(GL_EQUAL, STENCIL_SPECIAL | STENCIL_PIXELWRITE, STENCIL_SPECIAL | STENCIL_PIXELWRITE);
		glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

		Vector v = Vector(0, exactcolor ? 510.0f : 2.0f, 0, 0);
		ZZcgSetParameter4fv(pfragment->sOneColor, v, "g_fOneColor");
		Draw(curvb);

		// don't need to restore
	}

	GL_REPORT_ERRORD();
}

inline void AlphaDestinationTest(VB& curvb, FRAGMENTSHADER* pfragment, bool s_bDestAlphaTest, bool bCanRenderStencil)
{
	if (s_bDestAlphaTest)
	{
		if ((s_dwColorWrite & COLORMASK_ALPHA))
		{
			if (curvb.fba.fba)
				ProcessFBA(curvb, pfragment->sOneColor);
			else if (bCanRenderStencil)
				// finally make sure all entries are 1 when the dest alpha >= 0x80 (if fba is 1, this is already the case)
				ProcessStencil(curvb);
		}
	}
	else if ((s_dwColorWrite & COLORMASK_ALPHA) && curvb.fba.fba)
		ProcessFBA(curvb, pfragment->sOneColor);

	if (bDestAlphaColor == 1)
	{
		// need to reset the dest colors to their original counter parts
		//WARN_LOG("Need to reset dest alpha color\n");
	}
}

inline void AlphaSaveTarget(VB& curvb)
{
#ifdef _DEBUG
	return; // Do nothing

//	if( g_bSaveFlushedFrame & 0xf ) {
//#ifdef _WIN32
//		CreateDirectory("frames", NULL);
//#else
//		char* strdir="";
//		sprintf(strdir, "mkdir %s", "frames");
//		system(strdir);
//#endif
//		char str[255];
//		sprintf(str, "frames/frame%.4d.tga", g_SaveFrameNum++);
//		if( (g_bSaveFlushedFrame & 2) ) {
//			//glBindFramebufferEXT( GL_FRAMEBUFFER_EXT, 0 ); // switch to the backbuffer
//			//glFlush();
//			//SaveTexture("tex.jpg", GL_TEXTURE_RECTANGLE_NV, curvb.prndr->ptex, RW(curvb.prndr->fbw), RH(curvb.prndr->fbh));
//			SaveRenderTarget(str, RW(curvb.prndr->fbw), RH(curvb.prndr->fbh), 0);
//		}
//	}
#endif
}

inline void AlphaColorClamping(VB& curvb, const pixTest curtest)
{
	// clamp the final colors, when enabled ffx2 credits mess up
	if (curvb.curprim.abe && bAlphaClamping && GetRenderFormat() != RFT_byte8 && !(conf.settings().no_color_clamp))   // if !colclamp, skip
	{
		ResetAlphaVariables();

		// if processing the clamping case, make sure can write to the front buffer
		glDisable(GL_STENCIL_TEST);
		glEnable(GL_BLEND);
		glDisable(GL_ALPHA_TEST);
		glDisable(GL_DEPTH_TEST);
		glDepthMask(0);
		glColorMask(1, 1, 1, 0);

		if (s_bWriteDepth) ResetRenderTarget(1);

		SetShaderCaller("AlphaColorClamping");

		SETPIXELSHADER(ppsOne.prog);
		GL_BLEND_RGB(GL_ONE, GL_ONE);

		float f;

		if (bAlphaClamping & 1)    // min
		{
			f = 0;
			ZZcgSetParameter4fv(ppsOne.sOneColor, &f, "g_fOneColor");
			GL_BLENDEQ_RGB(GL_MAX_EXT);
			Draw(curvb);
		}

		// bios shows white screen
		if (bAlphaClamping & 2)    // max
		{
			f = 1;
			ZZcgSetParameter4fv(ppsOne.sOneColor, &f, "g_fOneColor");
			GL_BLENDEQ_RGB(GL_MIN_EXT);
			Draw(curvb);
		}

		if (!curvb.zbuf.zmsk)
		{
			glDepthMask(1);

			if (s_bWriteDepth)
			{
				assert(curvb.pdepth != NULL);
				curvb.pdepth->SetRenderTarget(1);
			}
		}

		if (curvb.test.ate && USEALPHATESTING) glEnable(GL_ALPHA_TEST);

		GL_ZTEST(curtest.zte);
	}
}

inline void FlushUndoFiter(u32 dwFilterOpts)
{
	if (dwFilterOpts)
	{
		// undo filter changes (binding didn't change)
		if (dwFilterOpts & 1) glTexParameteri(GL_TEXTURE_RECTANGLE_NV, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		if (dwFilterOpts & 2) glTexParameteri(GL_TEXTURE_RECTANGLE_NV, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	}
}

// This is the most important function! It draws all collected info onscreen.
void ZeroGS::Flush(int context)
{
	FUNCLOG
	VB& curvb = vb[context];
	const pixTest curtest = curvb.test;

	if (FlushInitialTest(curvb, curtest, context)) return;

	VisualBufferMessage(context);

	GL_REPORT_ERRORD();

	CRenderTarget* ptextarg = FlushGetTarget(curvb);

	SwitchWireframeOff();
	FlushDoContextJob(curvb, context);

	u32 dwUsingSpecialTesting = 0, dwFilterOpts = 0;
	int exactcolor = FlushGetExactcolor(curtest);

	FRAGMENTSHADER* pfragment = FlushRendererStage(curvb, dwFilterOpts, ptextarg, exactcolor, context);

	bool bCanRenderStencil = AlphaCanRenderStencil(curvb);

	if (curtest.date || gs.pabe) SetDestAlphaTest();

	AlphaSetStencil(s_bDestAlphaTest && bCanRenderStencil);
	AlphaSetDepthTest(curvb, curtest, pfragment);						// Error!
	SetAlphaTest(curtest);

	u32 oldabe = AlphaSetupBlendTest(curvb);					// Unavoidable

	// needs to be before RenderAlphaTest
	AlphaRenderFBA(curvb, pfragment, s_bDestAlphaTest, bCanRenderStencil);

	dwUsingSpecialTesting =	AlphaRenderAlpha(curvb, curtest, pfragment, exactcolor);			// Unavoidable
	AlphaRenderStencil(curvb, s_bDestAlphaTest, bCanRenderStencil, dwUsingSpecialTesting);
	AlphaTest(curvb);								// Unavoidable
	AlphaPabe(curvb, pfragment, exactcolor);
	AlphaFailureTestJob(curvb, curtest, pfragment, exactcolor, bCanRenderStencil, oldabe);
	AlphaSpecialTesting(curvb, pfragment, dwUsingSpecialTesting, exactcolor);
	AlphaDestinationTest(curvb, pfragment, s_bDestAlphaTest, bCanRenderStencil);
	AlphaSaveTarget(curvb);
	
	GL_REPORT_ERRORD();

	AlphaColorClamping(curvb, curtest);
	FlushUndoFiter(dwFilterOpts);

	ppf += curvb.nCount + 0x100000;

	curvb.nCount = 0;
	curvb.curprim.abe = oldabe;

	SwitchWireframeOn();

	GL_REPORT_ERRORD();
}

void ZeroGS::FlushBoth()
{
	Flush(0);
	Flush(1);
}

inline void ZeroGS::RenderFBA(const VB& curvb, CGparameter sOneColor)
{
	// add fba to all pixels
	GL_STENCILFUNC(GL_ALWAYS, STENCIL_FBA, 0xff);
	glStencilMask(STENCIL_CLEAR);
	glStencilOp(GL_ZERO, GL_KEEP, GL_REPLACE);

	glDisable(GL_DEPTH_TEST);
	glDepthMask(0);
	glColorMask(0, 0, 0, 0);

	if (s_bWriteDepth) ResetRenderTarget(1);

	SetShaderCaller("RenderFBA");

	glEnable(GL_ALPHA_TEST);

	glAlphaFunc(GL_GEQUAL, 1);

	Vector v(1,2,0,0);

	ZZcgSetParameter4fv(sOneColor, v, "g_fOneColor");

	Draw(curvb);

	SetAlphaTest(curvb.test);

	// reset (not necessary)
	GL_COLORMASK(s_dwColorWrite);

	glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

	if (!curvb.zbuf.zmsk)
	{
		glDepthMask(1);

		assert(curvb.pdepth != NULL);

		if (s_bWriteDepth) curvb.pdepth->SetRenderTarget(1);
	}

	GL_ZTEST(curvb.test.zte);
}

__forceinline void ZeroGS::RenderAlphaTest(const VB& curvb, CGparameter sOneColor)
{
	if (!g_bUpdateStencil) return;

	if ((curvb.test.ate) && (curvb.test.afail == 1)) glDisable(GL_ALPHA_TEST);

	glDepthMask(0);

	glColorMask(0, 0, 0, 0);

	if (s_bWriteDepth) ResetRenderTarget(1);

	SetShaderCaller("RenderAlphaTest");

	Vector v(1,2,0,0);

	ZZcgSetParameter4fv(sOneColor, v, "g_fOneColor");

	// or a 1 to the stencil buffer wherever alpha passes
	glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

	s_stencilfunc = GL_ALWAYS;

	glEnable(GL_STENCIL_TEST);

	if (!s_bDestAlphaTest)
	{
		// clear everything
		s_stencilref = 0;
		glStencilMask(STENCIL_CLEAR);
		glDisable(GL_ALPHA_TEST);
		GL_STENCILFUNC_SET();
		Draw(curvb);

		if (curvb.test.ate && curvb.test.afail != 1 && USEALPHATESTING) glEnable(GL_ALPHA_TEST);
	}

	if (curvb.test.ate && curvb.test.atst > 1 && curvb.test.aref > 0x80)
	{
		v = Vector(1,1,0,0);
		ZZcgSetParameter4fv(sOneColor, v, "g_fOneColor");
		glAlphaFunc(g_dwAlphaCmp[curvb.test.atst], AlphaReferedValue(curvb.test.aref));
	}

	s_stencilref = STENCIL_SPECIAL;

	glStencilMask(STENCIL_SPECIAL);
	GL_STENCILFUNC_SET();
	glDisable(GL_DEPTH_TEST);

	Draw(curvb);

	if (curvb.test.zte) glEnable(GL_DEPTH_TEST);

	GL_ALPHATEST(0);

	GL_COLORMASK(s_dwColorWrite);

	if (!curvb.zbuf.zmsk)
	{
		glDepthMask(1);

		// set rt next level

		if (s_bWriteDepth) curvb.pdepth->SetRenderTarget(1);
	}
}

inline void ZeroGS::RenderStencil(const VB& curvb, u32 dwUsingSpecialTesting)
{
	//NOTE: This stencil hack for dest alpha testing ONLY works when
	// the geometry in one DrawPrimitive call does not overlap

	// mark the stencil buffer for the new data's bits (mark 4 if alpha is >= 0xff)
	// mark 4 if a pixel was written (so that the stencil buf can be changed with new values)
	glStencilMask(STENCIL_PIXELWRITE);
	glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

	s_stencilmask = (curvb.test.date ? STENCIL_ALPHABIT : 0) | (dwUsingSpecialTesting ? STENCIL_SPECIAL : 0);
	s_stencilfunc = s_stencilmask ? GL_EQUAL : GL_ALWAYS;

	s_stencilref = curvb.test.date * curvb.test.datm | STENCIL_PIXELWRITE | (dwUsingSpecialTesting ? STENCIL_SPECIAL : 0);
	GL_STENCILFUNC_SET();
}

inline void ZeroGS::ProcessStencil(const VB& curvb)
{
	assert(!curvb.fba.fba);

	// set new alpha bit
	glStencilMask(STENCIL_ALPHABIT);
	GL_STENCILFUNC(GL_EQUAL, STENCIL_PIXELWRITE, STENCIL_PIXELWRITE | STENCIL_FBA);
	glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

	glDisable(GL_DEPTH_TEST);
	glDepthMask(0);
	glColorMask(0, 0, 0, 0);

	if (s_bWriteDepth) ResetRenderTarget(1);

	GL_ALPHATEST(0);

	SetShaderCaller("ProcessStencil");

	SETPIXELSHADER(ppsOne.prog);
	Draw(curvb);

	// process when alpha >= 0xff
	GL_STENCILFUNC(GL_EQUAL, STENCIL_PIXELWRITE | STENCIL_FBA | STENCIL_ALPHABIT, STENCIL_PIXELWRITE | STENCIL_FBA);
	Draw(curvb);

	// clear STENCIL_PIXELWRITE bit
	glStencilMask(STENCIL_CLEAR);

	GL_STENCILFUNC(GL_ALWAYS, 0, STENCIL_PIXELWRITE | STENCIL_FBA);
	Draw(curvb);

	// restore state
	GL_COLORMASK(s_dwColorWrite);

	if (curvb.test.ate && USEALPHATESTING) glEnable(GL_ALPHA_TEST);

	if (!curvb.zbuf.zmsk)
	{
		glDepthMask(1);

		if (s_bWriteDepth)
		{
			assert(curvb.pdepth != NULL);
			curvb.pdepth->SetRenderTarget(1);
		}
	}

	GL_ZTEST(curvb.test.zte);

	glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
}

__forceinline void ZeroGS::ProcessFBA(const VB& curvb, CGparameter sOneColor)
{
	if ((curvb.frame.fbm&0x80000000)) return;

	// add fba to all pixels that were written and alpha was less than 0xff
	glStencilMask(STENCIL_ALPHABIT);
	glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
	GL_STENCILFUNC(GL_EQUAL, STENCIL_FBA | STENCIL_PIXELWRITE | STENCIL_ALPHABIT, STENCIL_PIXELWRITE | STENCIL_FBA);
	glDisable(GL_DEPTH_TEST);

	glDepthMask(0);
	glColorMask(0, 0, 0, 1);

	if (s_bWriteDepth) ResetRenderTarget(1);

	SetShaderCaller("ProcessFBA");

	// processes the pixels with ALPHA < 0x80*2
	glEnable(GL_ALPHA_TEST);
	glAlphaFunc(GL_LEQUAL, 1);

	// add 1 to dest
	GL_BLEND_ALPHA(GL_ONE, GL_ONE);
	GL_BLENDEQ_ALPHA(GL_FUNC_ADD);

	float f = 1;
	ZZcgSetParameter4fv(sOneColor, &f, "g_fOneColor");
	SETPIXELSHADER(ppsOne.prog);
	Draw(curvb);
	glDisable(GL_ALPHA_TEST);

	// reset bits
	glStencilMask(STENCIL_CLEAR);
	GL_STENCILFUNC(GL_GREATER, 0, STENCIL_PIXELWRITE | STENCIL_FBA);
	glStencilOp(GL_KEEP, GL_KEEP, GL_ZERO);
	Draw(curvb);

	if (curvb.test.atst && USEALPHATESTING)
	{
		glEnable(GL_ALPHA_TEST);
		glAlphaFunc(g_dwAlphaCmp[curvb.test.atst], AlphaReferedValue(curvb.test.aref));
	}

	// restore (SetAlphaVariables)
	GL_BLEND_ALPHA(GL_ONE, GL_ZERO);

	if (vAlphaBlendColor.y < 0) GL_BLENDEQ_ALPHA(GL_FUNC_REVERSE_SUBTRACT);

	// reset (not necessary)
	GL_COLORMASK(s_dwColorWrite);

	if (!curvb.zbuf.zmsk)
	{
		glDepthMask(1);

		if (s_bWriteDepth) curvb.pdepth->SetRenderTarget(1);
	}

	GL_ZTEST(curvb.test.zte);
}

void ZeroGS::SetContextTarget(int context)
{
	FUNCLOG
	VB& curvb = vb[context];
	GL_REPORT_ERRORD();

	if (curvb.prndr == NULL)
		curvb.prndr = s_RTs.GetTarg(curvb.frame, 0, get_maxheight(curvb.gsfb.fbp, curvb.gsfb.fbw, curvb.gsfb.psm));

	// make sure targets are valid
	if (curvb.pdepth == NULL)
	{
		frameInfo f;
		f.fbp = curvb.zbuf.zbp;
		f.fbw = curvb.frame.fbw;
		f.fbh = curvb.prndr->fbh;
		f.psm = curvb.zbuf.psm;
		f.fbm = 0;
		curvb.pdepth = (CDepthTarget*)s_DepthRTs.GetTarg(f, CRenderTargetMngr::TO_DepthBuffer | CRenderTargetMngr::TO_StrictHeight |
					   (curvb.zbuf.zmsk ? CRenderTargetMngr::TO_Virtual : 0), get_maxheight(curvb.zbuf.zbp, curvb.gsfb.fbw, 0));
	}

	assert(curvb.prndr != NULL && curvb.pdepth != NULL);

	if (curvb.pdepth->fbh != curvb.prndr->fbh) ZZLog::Debug_Log("(curvb.pdepth->fbh(0x%x) != curvb.prndr->fbh(0x%x))", curvb.pdepth->fbh, curvb.prndr->fbh);
	//assert(curvb.pdepth->fbh == curvb.prndr->fbh);

	if (curvb.pdepth->status & CRenderTarget::TS_Virtual)
	{

		if (!curvb.zbuf.zmsk)
		{
			CRenderTarget* ptemp = s_DepthRTs.Promote(curvb.pdepth->fbp | (curvb.pdepth->fbw << 16));
			assert(ptemp == curvb.pdepth);
		}
		else
		{
			curvb.pdepth->status &= ~CRenderTarget::TS_NeedUpdate;
		}
	}

	bool bSetTarg = 1;

	if (curvb.pdepth->status & CRenderTarget::TS_NeedUpdate)
	{
		assert(!(curvb.pdepth->status & CRenderTarget::TS_Virtual));

		// don't update if virtual
		curvb.pdepth->Update(context, curvb.prndr);
		bSetTarg = 0;
	}

	GL_REPORT_ERRORD();

	if (curvb.prndr->status & CRenderTarget::TS_NeedUpdate)
	{
		/*		if(bSetTarg) {
		*			printf ( " Here\n ");
		*			if(s_bWriteDepth) {
		*				curvb.pdepth->SetRenderTarget(1);
		*				curvb.pdepth->SetDepthStencilSurface();
		*			}
		*			else
		*				curvb.pdepth->SetDepthStencilSurface();
		*		}*/
		curvb.prndr->Update(context, curvb.pdepth);
	}
	else
	{

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

	if ((curvb.zbuf.zbp - curvb.pdepth->fbp) != (curvb.frame.fbp - curvb.prndr->fbp) && curvb.test.zte)
		ZZLog::Warn_Log("Frame and zbuf not aligned.");

	curvb.bVarsSetTarg = true;

	if (vb[!context].prndr != curvb.prndr) vb[!context].bVarsSetTarg = false;

#ifdef _DEBUG
	// These conditions happen often enough that we'll just warn about it rather then abort in Debug mode.
	if (curvb.prndr->status & CRenderTarget::TS_NeedUpdate) 
	{
		ZZLog::Debug_Log("In SetContextTarget, prndr is ending with TS_NeedUpdate set.");
	}
	
	if (curvb.pdepth != NULL && (curvb.pdepth->status & CRenderTarget::TS_NeedUpdate))
	{
		ZZLog::Debug_Log("In SetContextTarget, pdepth is ending with TS_NeedUpdate set.");
	}
#endif

	GL_REPORT_ERRORD();
}


void ZeroGS::SetTexInt(int context, FRAGMENTSHADER* pfragment, int settexint)
{
	FUNCLOG

	if (settexint)
	{
		tex0Info& tex0 = vb[context].tex0;

		CMemoryTarget* pmemtarg = g_MemTargs.GetMemoryTarget(tex0, 1);

		if (vb[context].bVarsTexSync)
		{
			if (vb[context].pmemtarg != pmemtarg)
			{
				SetTexVariablesInt(context, GetTexFilter(vb[context].tex1), tex0, pmemtarg, pfragment, s_bForceTexFlush);
				vb[context].bVarsTexSync = true;
			}
		}
		else
		{
			SetTexVariablesInt(context, GetTexFilter(vb[context].tex1), tex0, pmemtarg, pfragment, s_bForceTexFlush);
			vb[context].bVarsTexSync = true;

			INC_TEXVARS();
		}
	}
	else
	{
		vb[context].bVarsTexSync = false;
	}
}

// clamp relies on texture width
void ZeroGS::SetTexClamping(int context, FRAGMENTSHADER* pfragment)
{
	FUNCLOG
	SetShaderCaller("SetTexClamping");
	clampInfo* pclamp = &ZeroGS::vb[context].clamp;
	Vector v, v2;
	v.x = v.y = 0;
	u32* ptex = ZeroGS::vb[context].ptexClamp;
	ptex[0] = ptex[1] = 0;

	float fw = ZeroGS::vb[context].tex0.tw ;
	float fh = ZeroGS::vb[context].tex0.th ;

	switch (pclamp->wms)
	{
		case 0:
			v2.x = -1e10;
			v2.z = 1e10;
			break;

		case 1: // pclamp
			// suikoden5 movie text
			v2.x = 0;
			v2.z = 1 - 0.5f / fw;
			break;

		case 2: // reg pclamp
			v2.x = (pclamp->minu + 0.5f) / fw;
			v2.z = (pclamp->maxu - 0.5f) / fw;
			break;

		case 3: // region rep x
			v.x = 0.9999f;
			v.z = fw;
			v2.x = (float)GPU_TEXMASKWIDTH / fw;
			v2.z = pclamp->maxu / fw;
			int correctMinu = pclamp->minu & (~pclamp->maxu);		// (A && B) || C == (A && (B && !C)) + C

			if (correctMinu != g_PrevBitwiseTexX)
			{
				g_PrevBitwiseTexX = correctMinu;
				ptex[0] = ZeroGS::s_BitwiseTextures.GetTex(correctMinu, 0);
			}

			break;
	}

	switch (pclamp->wmt)
	{

		case 0:
			v2.y = -1e10;
			v2.w = 1e10;
			break;

		case 1: // pclamp
			// suikoden5 movie text
			v2.y = 0;
			v2.w = 1 - 0.5f / fh;
			break;

		case 2: // reg pclamp
			v2.y = (pclamp->minv + 0.5f) / fh;
			v2.w = (pclamp->maxv - 0.5f) / fh;
			break;

		case 3: // region rep y
			v.y = 0.9999f;
			v.w = fh;
			v2.y = (float)GPU_TEXMASKWIDTH / fh;
			v2.w = pclamp->maxv / fh;
			int correctMinv = pclamp->minv & (~pclamp->maxv);		// (A && B) || C == (A && (B && !C)) + C

			if (correctMinv != g_PrevBitwiseTexY)
			{
				g_PrevBitwiseTexY = correctMinv;
				ptex[1] = ZeroGS::s_BitwiseTextures.GetTex(correctMinv, ptex[0]);
			}
			break;
	}

	if (pfragment->fTexWrapMode != 0)
		ZZcgSetParameter4fv(pfragment->fTexWrapMode, v, "g_fTexWrapMode");

	if (pfragment->fClampExts != 0)
		ZZcgSetParameter4fv(pfragment->fClampExts, v2, "g_fClampExts");


}

// Fixme should be in Vector lib
inline bool equal_vectors(Vector a, Vector b)
{
	if (abs(a.x - b.x) + abs(a.y - b.y) + abs(a.z - b.z) + abs(a.w - b.w) < 0.01)
		return true;
	else
		return false;
}

int CheckTexArray[4][2][2][2] = {{{{0, }}}};
void ZeroGS::SetTexVariables(int context, FRAGMENTSHADER* pfragment)
{
	FUNCLOG

	if (!vb[context].curprim.tme) return;

	assert(!vb[context].bNeedTexCheck);

	Vector v, v2;

	tex0Info& tex0 = vb[context].tex0;

	//float fw = (float)tex0.tw;
	//float fh = (float)tex0.th;

	if (!vb[context].bTexConstsSync)
	{
		SetShaderCaller("SetTexVariables");

		// alpha and texture highlighting
		Vector valpha, valpha2 ;

		// if clut, use the frame format
		int psm = GetTexCPSM(tex0);

//		printf ( "A %d psm, is-clut %d. cpsm %d | %d %d\n", psm,  PSMT_ISCLUT(psm), tex0.cpsm,  tex0.tfx, tex0.tcc );

		Vector vblack;
		vblack.x = vblack.y = vblack.z = vblack.w = 10;

		/* tcc -- Tecture Color Component 0=RGB, 1=RGBA + use Alpha from TEXA reg when not in PSM
		 * tfx -- Texture Function (0=modulate, 1=decal, 2=hilight, 3=hilight2)
		 *
		 * valpha2 =  0 0 2 1     0 0 2 1
		 *            1 0 0 0     1 1 0 0
		 *            0 0 2 0     0 1 2 0
		 *            0 0 2 0     0 1 2 0
		 *
		 *              0        1,!nNeed         1, psm=2, 10                1, psm=1
		 * valpha =   0 0 0 1     0 2 0 0        2ta0  2ta1-2ta0 0 0          2ta0 0 0 0
		 *            0 0 0 1     0 1 0 0         ta0   ta1-ta0  0 0           ta0 0 0 0
		 *	      0 0 1 1     0 1 1 1                        1 1           ta0 0 1 1
		 *	      0 0 1 1     0 1 1 0                        1 0           ta0 0 1 0
		*/

		valpha2.x = (tex0.tfx == 1) ;
		valpha2.y = (tex0.tcc == 1) && (tex0.tfx != 0) ;
		valpha2.z = (tex0.tfx != 1) * 2 ;
		valpha2.w = (tex0.tfx == 0) ;

		if (tex0.tcc == 0 || !nNeedAlpha(psm))
		{
			valpha.x = 0 ;
			valpha.y = (!!tex0.tcc) * (1 + (tex0.tfx == 0)) ;
		}
		else
		{
			valpha.x = (gs.texa.fta[0])  * (1 + (tex0.tfx == 0)) ;
			valpha.y = (gs.texa.fta[psm!=1] - gs.texa.fta[0]) * (1 + (tex0.tfx == 0))  ;
		}

		valpha.z = (tex0.tfx >= 3) ;

		valpha.w = (tex0.tcc == 0) || (tex0.tcc == 1 && tex0.tfx == 2) ;

		if (tex0.tcc && gs.texa.aem && psm == PSMCT24)
			vblack.w = 0;

		/*
		// Test, old code.
				Vector valpha3, valpha4;
		 		switch(tex0.tfx) {
					case 0:
						valpha3.z = 0; valpha3.w = 0;
						valpha4.x = 0; valpha4.y = 0;
						valpha4.z = 2; valpha4.w = 1;

						break;
					case 1:
						valpha3.z = 0; valpha3.w = 1;
						valpha4.x = 1; valpha4.y = 0;
						valpha4.z = 0; valpha4.w = 0;

						break;
					case 2:
						valpha3.z = 1; valpha3.w = 1.0f;
						valpha4.x = 0; valpha4.y = tex0.tcc ? 1.0f : 0.0f;
						valpha4.z = 2; valpha4.w = 0;

						break;

					case 3:
						valpha3.z = 1; valpha3.w = tex0.tcc ? 0.0f : 1.0f;
						valpha4.x = 0; valpha4.y = tex0.tcc ? 1.0f : 0.0f;
						valpha4.z = 2; valpha4.w = 0;

						break;
				}
				if( tex0.tcc ) {

					if( tex0.tfx == 1 ) {
						//mode.x = 10;
						valpha3.z = 0; valpha3.w = 0;
						valpha4.x = 1; valpha4.y = 1;
						valpha4.z = 0; valpha4.w = 0;
					}

					if( nNeedAlpha(psm) ) {

						if( tex0.tfx == 0 ) {
							// make sure alpha is mult by two when the output is Cv = Ct*Cf
							valpha3.x = 2*gs.texa.fta[0];
							// if 24bit, always choose ta[0]
							valpha3.y = 2*gs.texa.fta[psm != 1];
							valpha3.y -= valpha.x;
						}
						else {
							valpha3.x = gs.texa.fta[0];
							// if 24bit, always choose ta[0]
							valpha3.y = gs.texa.fta[psm != 1];
							valpha3.y -= valpha.x;
						}
					}
					else {
						if( tex0.tfx == 0 ) {
							valpha3.x = 0;
							valpha3.y = 2;
						}
						else {
							valpha3.x = 0;
							valpha3.y = 1;
						}
					}
				}
				else {

					// reset alpha to color
					valpha3.x = valpha3.y = 0;
					valpha3.w = 1;
				}

				if ( equal_vectors(valpha, valpha3) && equal_vectors(valpha2, valpha4) ) {
					if (CheckTexArray[tex0.tfx][tex0.tcc][psm!=1][nNeedAlpha(psm)] == 0) {
						printf ( "Good issue %d %d %d %d\n", tex0.tfx,  tex0.tcc, psm, nNeedAlpha(psm) );
						CheckTexArray[tex0.tfx][tex0.tcc][psm!=1][nNeedAlpha(psm) ] = 1;
					}
				}
				else if (CheckTexArray[tex0.tfx][tex0.tcc][psm!=1][nNeedAlpha(psm)] == -1) {
					printf ("Bad array, %d %d %d %d\n\tolf valpha %f, %f, %f, %f : valpha2 %f %f %f %f\n\tnew valpha %f, %f, %f, %f : valpha2 %f %f %f %f\n",
						 tex0.tfx,  tex0.tcc, psm, nNeedAlpha(psm),
					 	valpha3.x, valpha3.y, valpha3.z, valpha3.w, valpha4.x, valpha4.y, valpha4.z, valpha4.w,
					 	valpha.x, valpha.y, valpha.z, valpha.w,  valpha2.x, valpha2.y, valpha2.z, valpha2.w);
					CheckTexArray[tex0.tfx][tex0.tcc][psm!=1][nNeedAlpha(psm)] = -1 ;
				}

		// Test;*/

		ZZcgSetParameter4fv(pfragment->fTexAlpha, valpha, "g_fTexAlpha");
		ZZcgSetParameter4fv(pfragment->fTexAlpha2, valpha2, "g_fTexAlpha2");

		if (tex0.tcc && gs.texa.aem && nNeedAlpha(psm))
			ZZcgSetParameter4fv(pfragment->fTestBlack, vblack, "g_fTestBlack");

		SetTexClamping(context, pfragment);

		vb[context].bTexConstsSync = true;
	}

	if (s_bTexFlush)
	{
		if (PSMT_ISCLUT(tex0.psm))
			texClutWrite(context);
		else
			s_bTexFlush = false;
	}
}

void ZeroGS::SetTexVariablesInt(int context, int bilinear, const tex0Info& tex0, CMemoryTarget* pmemtarg, FRAGMENTSHADER* pfragment, int force)
{
	FUNCLOG
	Vector v;
	assert(pmemtarg != NULL && pfragment != NULL && pmemtarg->ptex != NULL);

	if (pmemtarg == NULL || pfragment == NULL || pmemtarg->ptex == NULL)
	{
		printf("SetTexVariablesInt error\n");
		return;
	}

	SetShaderCaller("SetTexVariablesInt");

	float fw = (float)tex0.tw;
	float fh = (float)tex0.th;

	bool bUseBilinear = bilinear > 1 || (bilinear && conf.bilinear);

	if (bUseBilinear)
	{
		v.x = (float)fw;
		v.y = (float)fh;
		v.z = 1.0f / (float)fw;
		v.w = 1.0f / (float)fh;

		if (pfragment->fRealTexDims)
			ZZcgSetParameter4fv(pfragment->fRealTexDims, v, "g_fRealTexDims");
		else
			ZZcgSetParameter4fv(cgGetNamedParameter(pfragment->prog, "g_fRealTexDims"), v, "g_fRealTexDims");
	}

	if (m_Blocks[tex0.psm].bpp == 0)
	{
		ZZLog::Error_Log("Undefined tex psm 0x%x!", tex0.psm);
		return;
	}

	const BLOCK& b = m_Blocks[tex0.psm];

	float fbw = (float)tex0.tbw;

	Vector vTexDims;

	vTexDims.x = b.vTexDims.x * (fw);
	vTexDims.y = b.vTexDims.y * (fh);
	vTexDims.z = (float)BLOCK_TEXWIDTH * (0.002f / 64.0f + 0.01f / 128.0f);
	vTexDims.w = (float)BLOCK_TEXHEIGHT * 0.1f / 512.0f;

	if (bUseBilinear)
	{
		vTexDims.x *= 1 / 128.0f;
		vTexDims.y *= 1 / 512.0f;
		vTexDims.z *= 1 / 128.0f;
		vTexDims.w *= 1 / 512.0f;
	}

	float g_fitexwidth = g_fiGPU_TEXWIDTH / (float)pmemtarg->widthmult;

	//float g_texwidth = GPU_TEXWIDTH*(float)pmemtarg->widthmult;

	float fpage = tex0.tbp0 * (64.0f * g_fitexwidth);// + 0.05f * g_fitexwidth;
	float fpageint = floorf(fpage);
	//int starttbp = (int)fpage;

	// 2048 is number of words to span one page
	//float fblockstride = (2048.0f /(float)(g_texwidth*BLOCK_TEXWIDTH)) * b.vTexDims.x * fbw;

	float fblockstride = (2048.0f / (float)(GPU_TEXWIDTH * (float)pmemtarg->widthmult * BLOCK_TEXWIDTH)) * b.vTexDims.x * fbw;

	assert(fblockstride >= 1.0f);

	v.x = (float)(2048 * g_fitexwidth);
	v.y = fblockstride;
	v.z = g_fBlockMult / (float)pmemtarg->widthmult;
	v.w = fpage - fpageint ;

	if (g_fBlockMult > 1)
	{
		// make sure to divide by mult (since the G16R16 texture loses info)
		v.z *= b.bpp * (1 / 32.0f);
	}

	ZZcgSetParameter4fv(pfragment->fTexDims, vTexDims, "g_fTexDims");

//	ZZcgSetParameter4fv(pfragment->fTexBlock, b.vTexBlock, "g_fTexBlock"); // I change it, and it's working. Seems casting from Vector to float[4] is ok.
	ZZcgSetParameter4fv(pfragment->fTexBlock, &b.vTexBlock.x, "g_fTexBlock");
	ZZcgSetParameter4fv(pfragment->fTexOffset, v, "g_fTexOffset");

	// get hardware texture dims
	//int texheight = (pmemtarg->realheight+pmemtarg->widthmult-1)/pmemtarg->widthmult;
	int texwidth = GPU_TEXWIDTH * pmemtarg->widthmult * pmemtarg->channels;

	v.y = 1.0f;
	v.x = (fpageint - (float)pmemtarg->realy / (float)pmemtarg->widthmult + 0.5f);//*v.y;
	v.z = (float)texwidth;

	/*	if( !(g_nPixelShaderVer & SHADER_ACCURATE) || bUseBilinear ) {
			if (tex0.psm == PSMT4 )
				v.w = 0.0f;
			else
				v.w = 0.25f;
			}
		else
			v.w = 0.5f;*/
	v.w = 0.5f;

	ZZcgSetParameter4fv(pfragment->fPageOffset, v, "g_fPageOffset");

	if (force)
		s_ptexCurSet[context] = pmemtarg->ptex->tex;
	else
		s_ptexNextSet[context] = pmemtarg->ptex->tex;

	vb[context].pmemtarg = pmemtarg;

	vb[context].bVarsTexSync = false;
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
			switch(PSMT_BITMODE(vb[icurctx].prndr->psm)) \
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
			bNeedBlendFactorInAlpha = true; /* should disable alpha channel writing */ \
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

void ZeroGS::ResetAlphaVariables() {
	FUNCLOG
}

inline void ZeroGS::NeedFactor(int w)
{
	if (bDestAlphaColor == 2)
	{
		bNeedBlendFactorInAlpha = (w + 1) ? true : false;
		vAlphaBlendColor.y = 0;
		vAlphaBlendColor.w = (float)w;
	}
}

//static int CheckArray[48][2] = {{0,}};

void ZeroGS::SetAlphaVariables(const alphaInfo& a)
{
	FUNCLOG
	bool alphaenable = true;

	// TODO: negative color when not clamping turns to positive???
	g_vars._bAlphaState = 0; // set all to zero
	bNeedBlendFactorInAlpha = false;
	b2XAlphaTest = 1;
	//u32 dwTemp = 0xffffffff;
	bDestAlphaColor = 0;

	// default
	s_srcalpha = GL_ONE;
	s_dstalpha = GL_ZERO;
	s_alphaeq = GL_FUNC_ADD;
	s_rgbeq = 1;

//	s_alphaInfo = a;
	vAlphaBlendColor = Vector(1, 2 * 255.0f / 256.0f, 0, 0);
	u32 usec = a.c;


	/*
	 * Alpha table
	 *           a   +  b   +  d
	 *           	    S          		   D
	 *     0     a     -a      1    |   0      0      0
	 *     1     0      0      0    |   a     -a      1
	 *     2     0      0      0    |   0      0      0
	 *
	 *							d = 0 Cs
	 *   a   b   	0 Cs	   				   1 Cd        		2 0
	 *   			   	              | 			 	       |
	 * 0 000:  a+-a+ 1 | 0+ 0+ 0 = 	    1         | 010: a+ 0+ 1 | 0+-a+ 0 = 1-(-a)(+)(-a) | 020: a+ 0+ 1 | 0+ 0+ 0 = 1-(-a) (+)  0
	 * 1 100:  0+-a+ 1 | a+ 0+ 0 = 1-a (+) a      | 110: 0+ 0+ 1 | a+-a+ 0 =        1      | 120: 0+ 0+ 1 | a+ 0+ 0 = 1      (+)  a
	 * 2 200:  0+-a+ 1 | 0+ 0+ 0 = 1-a (+) 0      | 210: 0+ 0+ 1 | 0+-a+ 0 = 1     (-)  a  | 220: 0+ 0+ 1 | 0+ 0+ 0 =         1
	 *
	 *							d = 1 Cd
	 *		0		              |	1				       | 		2
	 * 0 001:  a+-a+ 0 | 0+ 0+ 1 = 0   (+) 1      | 011: a+ 0+ 0 | 0+-a+ 1 = a     (+) 1-a | 021: a+ 0+ 0 | 0+ 0+ 1 = a      (+)  1
	 * 1 101:  0+-a+ 0 | a+ 0+ 1 = (-a)(+) 1-(-a) | 111: 0+ 0+ 0 | a+-a+ 1 = 0     (+) 1   | 121: 0+ 0+ 0 | a+ 0+ 1 = 0      (+)  1-(-a)
	 * 2 201:  0+-a+ 0 | 0+ 0+ 1 = a   (R-)1      | 211: 0+ 0+ 0 | 0+-a+ 1 = 0     (+) 1-a | 221: 0+ 0+ 0 | 0+ 0+ 1 = 0	 (+)  1
	 *
	 *							d = 2 0
	 *		0			      |			1		       |		2
	 * 0 002:  a+-a+ 0 | 0+ 0+ 0 =      0         | 012: a+ 0+ 0 | 0+-a+ 0 = a     (-) a   | 022: a+ 0+ 0 | 0+ 0+ 0 = a      (+)  0
	 * 1 102:  0+-a+ 0 | a+ 0+ 0 = a  (R-) a      | 112: 0+ 0+ 0 | a+-a+ 0 =        0      | 122: 0+ 0+ 0 | a+ 0+ 0 = 0      (+)  a
	 * 2 202:  0+-a+ 0 | 0+ 0+ 0 = a  (R-) 0      | 212: 0+ 0+ 0 | 0+-a+ 0 = 0     (-) a   | 222: 0+ 0+ 0 | 0+ 0+ 0 =         0
	 *
	 *  Formulae is: (a-b) * (c /32) + d
	 *  		0	1	2
	 *  a		Cs	Cd	0
	 *  b		Cs	Cd	0
	 *  c		As	Ad	ALPHA.FIX
	 *  d		Cs	Cd	0
	 *
	 *  We want to emulate Cs * F1(alpha)  + Cd * F2(alpha)  by  OpenGl blending: (Cs * Ss (+,-,R-) Cd * Sd)
	 *  SET_ALPHA_COLOR_FACTOR(sign) set Set A (as As>>7, Ad>>7 or FIX>>7) with sign.
	 *  So we could use 1+a as one_minus_alpha and -a as alpha.
	 *
	 */
	int code = (a.a * 16) + (a.b * 4) + a.d ;

#define one_minus_alpha  (bDestAlphaColor == 2) ? GL_ONE_MINUS_SRC_ALPHA : blendinvalpha[usec]
#define alpha   	 (bDestAlphaColor == 2) ? GL_SRC_ALPHA : blendalpha[usec]
#define one 		 (bDestAlphaColor == 2) ? GL_ONE : blendalpha[usec]
#define	zero 		 (bDestAlphaColor == 2) ? GL_ZERO : blendinvalpha[usec]

	switch (code)
	{

		case 0: // 000				// Cs -- nothing changed
		case 20: // 110 = 16+4=20		// Cs
		case 40:   // 220 = 32+8=40		// Cs
		{
			alphaenable = false;
			break;
		}

		case 2: //002				// 0  -- should be zero
		case 22: //112				// 0
		case 42:   //222 = 32+8+2 =42		// 0
		{
			s_rgbeq = GL_FUNC_ADD;
			s_srcrgb = GL_ZERO;
			s_dstrgb = GL_ZERO;
			break;
		}

		case 1: //001				 // Cd  -- Should be destination alpha
		case 21: //111, 			 // Cd  -- 0*Source + 1*Desrinarion
		case 41:   //221 = 32+8+1=41		 // Cd  --
		{
			s_rgbeq = GL_FUNC_ADD;
			s_srcrgb = GL_ZERO;
			s_dstrgb = GL_ONE;
			break;
		}

		case 4:    // 010			 // (Cs-Cd)*A+Cs = Cs * (A + 1) - Cd * A
		{
			bAlphaClamping = 3;
			SET_ALPHA_COLOR_FACTOR(0);	 // a = -A

			s_rgbeq = GL_FUNC_ADD;		 // Cs*(1-a)+Cd*a
			s_srcrgb = one_minus_alpha ;
			s_dstrgb = alpha;

			NeedFactor(-1);
			break;
		}

		case 5:   // 011			// (Cs-Cd)*A+Cs = Cs * A + Cd * (1-A)
		{
			bAlphaClamping = 3; // all testing
			SET_ALPHA_COLOR_FACTOR(1);

			s_rgbeq = GL_FUNC_ADD;
			s_srcrgb = alpha;
			s_dstrgb = one_minus_alpha;

			NeedFactor(1);
			break;
		}

		case 6:   //012				// (Cs-Cd)*FIX
		{
			bAlphaClamping = 3;
			SET_ALPHA_COLOR_FACTOR(1);

			s_rgbeq = GL_FUNC_SUBTRACT;
			s_srcrgb = alpha;
			s_dstrgb = alpha;

			break;
		}

		case 8:   //020				// Cs*A+Cs = Cs * (1+A)
		{
			bAlphaClamping = 2; // max testing
			SET_ALPHA_COLOR_FACTOR(0);	// Zeyflitz change this! a = -A
				
			s_rgbeq = GL_FUNC_ADD;
			s_srcrgb = one_minus_alpha;	// Cs*(1-a).
			s_dstrgb = GL_ZERO;

//			NeedFactor(1);
			break;
		}

		case 9:   //021				// Cs*A+Cd
		{
			bAlphaClamping = 2; // max testing
			SET_ALPHA_COLOR_FACTOR(1);
				
			s_rgbeq = GL_FUNC_ADD;
			s_srcrgb = alpha;			// ZZ change it to.
			s_dstrgb = GL_ONE;
			break;
		}

		case 10:   //022			// Cs*A
		{
			bAlphaClamping = 2; // max testing
			SET_ALPHA_COLOR_FACTOR(1);

			s_rgbeq = GL_FUNC_ADD;
			s_srcrgb = alpha;
			s_dstrgb = GL_ZERO;
			break;
		}

		case 16:   //100
		{
			bAlphaClamping = 3;
			SET_ALPHA_COLOR_FACTOR(1);

			s_rgbeq = GL_FUNC_ADD;
			s_srcrgb = one_minus_alpha;
			s_dstrgb = alpha;

			NeedFactor(1);
			break;
		}

		case 17:   //101
		{
			bAlphaClamping = 3; // all testing
			SET_ALPHA_COLOR_FACTOR(0);

			s_rgbeq = GL_FUNC_ADD;
			s_srcrgb = alpha;
			s_dstrgb = one_minus_alpha;

			NeedFactor(-1);
			break;
		}

		case 18:   //102
		{
			bAlphaClamping = 3;
			SET_ALPHA_COLOR_FACTOR(1);

			s_rgbeq = GL_FUNC_REVERSE_SUBTRACT;
			s_srcrgb = alpha;
			s_dstrgb = alpha;

			break;
		}

		case 24:   //120 = 16+8
		{
			bAlphaClamping = 2; // max testing
			SET_ALPHA_COLOR_FACTOR(1);
				
			s_rgbeq = GL_FUNC_ADD;
			s_srcrgb = GL_ONE;
			s_dstrgb = alpha;
			break;
		}

		case 25:   //121				// Cd*(1+A)
		{
			bAlphaClamping = 2; // max testing
			SET_ALPHA_COLOR_FACTOR(0);

			s_rgbeq = GL_FUNC_ADD;
			s_srcrgb = GL_ZERO;
			s_dstrgb = one_minus_alpha;

//			 NeedFactor(-1);
			break;
		}

		case 26:   //122
		{
			bAlphaClamping = 2;
			SET_ALPHA_COLOR_FACTOR(1);

			s_rgbeq = GL_FUNC_ADD;
			s_srcrgb = GL_ZERO;
			s_dstrgb = alpha;
			break;
		}

		case 32:  // 200 = 32
		{
			bAlphaClamping = 1; // min testing
			SET_ALPHA_COLOR_FACTOR(1);

			s_rgbeq = GL_FUNC_ADD;
			s_srcrgb = one_minus_alpha;
			s_dstrgb = GL_ZERO;
			break;
		}

		case 33:  //201					// -Cs*A + Cd
		{
			bAlphaClamping = 1; // min testing
			SET_ALPHA_COLOR_FACTOR(1);

			s_rgbeq = GL_FUNC_REVERSE_SUBTRACT;
			s_srcrgb = alpha;
			s_dstrgb = GL_ONE;
			break;
		}

		case 34:  //202
		case 38:  //212
		{
			bAlphaClamping = 1; // min testing -- negative values

			s_rgbeq = GL_FUNC_ADD;
			s_srcrgb = GL_ZERO;
			s_dstrgb = GL_ZERO;
			break;
		}

		case 36:  //210
		{
			bAlphaClamping = 1; // min testing
			SET_ALPHA_COLOR_FACTOR(1);

			s_rgbeq = GL_FUNC_SUBTRACT;
			s_srcrgb = GL_ONE;
			s_dstrgb = alpha;
			break;
		}

		case 37:  //211
		{
			bAlphaClamping = 1; // min testing
			SET_ALPHA_COLOR_FACTOR(1);

			s_rgbeq = GL_FUNC_ADD;
			s_srcrgb = GL_ZERO;
			s_dstrgb = one_minus_alpha;
			break;
		}

		default:
		{
			ZZLog::Error_Log("Bad alpha code %d | %d %d %d", code, a.a, a.b, a.d);
		}
	}

	/*
		int t_rgbeq = GL_FUNC_ADD;
		int t_srcrgb = GL_ONE;
		int t_dstrgb = GL_ZERO;
		int tAlphaClamping = 0;

		if( a.a == a.b )
		{ // just d remains
			if( a.d == 0 ) 	{}
			else
			{
				t_dstrgb = a.d == 1 ? GL_ONE : GL_ZERO;
				t_srcrgb = GL_ZERO;
				t_rgbeq = GL_FUNC_ADD; 			//a) (001) (111) (221) b) (002) (112) (222)
			}
			goto EndSetAlpha;
		}
		else if( a.d == 2 )
		{ // zero
			if( a.a == 2 )
			{
				// zero all color
				t_srcrgb = GL_ZERO;
				t_dstrgb = GL_ZERO;
				goto EndSetAlpha;			// (202) (212)
			}
			else if( a.b == 2 )
			{
				//b2XAlphaTest = 1;		// a) (022)			// b) (122)
				SET_ALPHA_COLOR_FACTOR(1);

				if( bDestAlphaColor == 2 )
				{
					t_rgbeq = GL_FUNC_ADD;
					t_srcrgb = a.a == 0 ? GL_ONE : GL_ZERO;
					t_dstrgb = a.a == 0 ? GL_ZERO : GL_ONE;
				}
				else
				{
					tAlphaClamping = 2;
					t_rgbeq = GL_FUNC_ADD;
					t_srcrgb = a.a == 0 ? blendalpha[usec] : GL_ZERO;
					t_dstrgb = a.a == 0 ? GL_ZERO : blendalpha[usec];
				}

				goto EndSetAlpha;
			}

			// nothing is zero, so must do some real blending	//b2XAlphaTest = 1;		//a) (012) 	//b) (102)
			tAlphaClamping = 3;

			SET_ALPHA_COLOR_FACTOR(1);

			t_rgbeq = a.a == 0 ? GL_FUNC_SUBTRACT : GL_FUNC_REVERSE_SUBTRACT;
			t_srcrgb = bDestAlphaColor == 2 ? GL_ONE : blendalpha[usec];
			t_dstrgb = bDestAlphaColor == 2 ? GL_ONE : blendalpha[usec];
		}
		else if( a.a == 2 )
		{ // zero

			//b2XAlphaTest = 1;
			tAlphaClamping = 1; // min testing

			SET_ALPHA_COLOR_FACTOR(1);

			if( a.b == a.d )
			{
				// can get away with 1-A
				// a.a == a.d == 2!! (200) (211)
				t_rgbeq = GL_FUNC_ADD;
				t_srcrgb = (a.b == 0 && bDestAlphaColor != 2) ? blendinvalpha[usec] : GL_ZERO;
				t_dstrgb = (a.b == 0 || bDestAlphaColor == 2) ? GL_ZERO : blendinvalpha[usec];
			}
			else
			{
				// a) (201) b)(210)
				t_rgbeq = a.b==0 ? GL_FUNC_REVERSE_SUBTRACT : GL_FUNC_SUBTRACT;
				t_srcrgb = (a.b == 0 && bDestAlphaColor != 2) ? blendalpha[usec] : GL_ONE;
				t_dstrgb = (a.b == 0 || bDestAlphaColor == 2 ) ? GL_ONE : blendalpha[usec];
			}
		}
		else if( a.b == 2 )
		{
			tAlphaClamping = 2; // max testing

			SET_ALPHA_COLOR_FACTOR(a.a!=a.d);

			if( a.a == a.d )
			{
				// can get away with 1+A, but need to set alpha to negative
				// a)(020)
				// b)(121)
				t_rgbeq = GL_FUNC_ADD;

				if( bDestAlphaColor == 2 )
				{
					t_srcrgb = (a.a == 0) ? GL_ONE_MINUS_SRC_ALPHA : GL_ZERO;
					t_dstrgb = (a.a == 0) ? GL_ZERO : GL_ONE_MINUS_SRC_ALPHA;
				}
				else
				{
					t_srcrgb = a.a == 0 ? blendinvalpha[usec] : GL_ZERO;
					t_dstrgb = a.a == 0 ? GL_ZERO : blendinvalpha[usec];
				}
			}
			else
			{
				//a)(021) 			//b)(120) 			//b2XAlphaTest = 1;
				t_rgbeq = GL_FUNC_ADD;
				t_srcrgb = (a.a == 0 && bDestAlphaColor != 2) ? blendalpha[usec] : GL_ONE;
				t_dstrgb = (a.a == 0 || bDestAlphaColor == 2) ? GL_ONE : blendalpha[usec];
			}
		}
		else
		{
			// all 3 components are valid!
			tAlphaClamping = 3; // all testing
			SET_ALPHA_COLOR_FACTOR(a.a!=a.d);

			if( a.a == a.d )
			{
				// can get away with 1+A, but need to set alpha to negative		// a) 010,	// b) 101
				t_rgbeq = GL_FUNC_ADD;

				if( bDestAlphaColor == 2 )
				{
					// all ones
					t_srcrgb = a.a == 0 ? GL_ONE_MINUS_SRC_ALPHA : GL_SRC_ALPHA;
					t_dstrgb = a.a == 0 ? GL_SRC_ALPHA : GL_ONE_MINUS_SRC_ALPHA;
				}
				else
				{
					t_srcrgb = a.a == 0 ? blendinvalpha[usec] : blendalpha[usec];
					t_dstrgb = a.a == 0 ? blendalpha[usec] : blendinvalpha[usec];
				}
			}
			else
			{
				t_rgbeq = GL_FUNC_ADD;		// a) 011 		// b) 100 			//
				if( bDestAlphaColor == 2 )
				{
					// all ones
					t_srcrgb = a.a != 0 ? GL_ONE_MINUS_SRC_ALPHA : GL_SRC_ALPHA;
					t_dstrgb = a.a != 0 ? GL_SRC_ALPHA : GL_ONE_MINUS_SRC_ALPHA;
				}
				else
				{
					//b2XAlphaTest = 1;
					t_srcrgb = a.a != 0 ? blendinvalpha[usec] : blendalpha[usec];
					t_dstrgb = a.a != 0 ? blendalpha[usec] : blendinvalpha[usec];
				}
			}
		}
		EndSetAlpha:


		if ( alphaenable && (t_rgbeq != s_rgbeq || s_srcrgb != t_srcrgb || t_dstrgb != s_dstrgb || tAlphaClamping != bAlphaClamping)) {
			if (CheckArray[code][(bDestAlphaColor==2)] != -1) {
			printf ( "A code %d, 0x%x, 0x%x, 0x%x, 0x%x %d\n", code, alpha, one_minus_alpha, one, zero, bDestAlphaColor );
			printf ( "       Difference %d %d %d %d | 0x%x  0x%x | 0x%x  0x%x | 0x%x  0x%x | %d %d\n",
				code, a.a, a.b, a.d,
				t_rgbeq, s_rgbeq, t_srcrgb, s_srcrgb, t_dstrgb, s_dstrgb, tAlphaClamping, bAlphaClamping);
			CheckArray[code][(bDestAlphaColor==2)] = -1;
			}
		}
		else
		if (CheckArray[code][(bDestAlphaColor==2)] == 0){
			printf ( "Add good code %d %d, psm %d  destA %d\n", code, a.c, vb[icurctx].prndr->psm, bDestAlphaColor);
			CheckArray[code][(bDestAlphaColor==2)] = 1;
		}*/


	if (alphaenable)
	{
		zgsBlendFuncSeparateEXT(s_srcrgb, s_dstrgb, s_srcalpha, s_dstalpha);
		zgsBlendEquationSeparateEXT(s_rgbeq, s_alphaeq);
		glEnable(GL_BLEND); // always set
	}
	else
	{
		glDisable(GL_BLEND);
	}

	INC_ALPHAVARS();
}

void ZeroGS::SetWriteDepth()
{
	FUNCLOG

	if (conf.mrtdepth)
	{
		s_bWriteDepth = true;
		s_nWriteDepthCount = 4;
	}
}

bool ZeroGS::IsWriteDepth()
{
	FUNCLOG
	return s_bWriteDepth;
}

bool ZeroGS::IsWriteDestAlphaTest()
{
	FUNCLOG
	return s_bDestAlphaTest;
}

void ZeroGS::SetDestAlphaTest()
{
	FUNCLOG
	s_bDestAlphaTest = true;
	s_nWriteDestAlphaTest = 4;
}

void ZeroGS::SetTexFlush()
{
	FUNCLOG
	s_bTexFlush = true;

//	if( PSMT_ISCLUT(vb[0].tex0.psm) )
//		texClutWrite(0);
//	if( PSMT_ISCLUT(vb[1].tex0.psm) )
//		texClutWrite(1);

	if (!s_bForceTexFlush)
	{
		if (s_ptexCurSet[0] != s_ptexNextSet[0]) s_ptexCurSet[0] = s_ptexNextSet[0];
		if (s_ptexCurSet[1] != s_ptexNextSet[1]) s_ptexCurSet[1] = s_ptexNextSet[1];
	}
}

