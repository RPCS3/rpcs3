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

//-------------------------- Includes
#if defined(_WIN32)
#	include <windows.h>
#	include "resource.h"
#endif

#include <stdlib.h>

#include "GS.h"
#include "Mem.h"
#include "x86.h"
#include "zerogs.h"
#include "targets.h"
#include "GLWin.h"
#include "ZZoglShaders.h"
#include "ZZKick.h"
#include "Clut.h"

//----------------------- Defines

//-------------------------- Typedefs
typedef void (APIENTRYP _PFNSWAPINTERVAL)(int);

//-------------------------- Extern variables

extern u32 g_nGenVars, g_nTexVars, g_nAlphaVars, g_nResolve;
extern char *libraryName;
extern int g_nFrame, g_nRealFrame;

//extern int s_nFullscreen;
//-------------------------- Variables

primInfo *prim;

inline u32 FtoDW(float f) { return (*((u32*)&f)); }

int g_nDepthUpdateCount = 0;

// Consts
const GLenum primtype[8] = { GL_POINTS, GL_LINES, GL_LINES, GL_TRIANGLES, GL_TRIANGLES, GL_TRIANGLES, GL_TRIANGLES, 0xffffffff };
static const int PRIMMASK = 0x0e;   // for now ignore 0x10 (AA)

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

bool s_bTexFlush = false;
int s_nLastResolveReset = 0;
int s_nResolveCounts[30] = {0}; // resolve counts for last 30 frames

////////////////////
// State parameters
int nBackbufferWidth, nBackbufferHeight;									// ZZ

//       	= float4( 255.0 /256.0f,  255.0/65536.0f, 255.0f/(65535.0f*256.0f), 1.0f/(65536.0f*65536.0f));
//	float4 g_vdepth = float4( 65536.0f*65536.0f, 256.0f*65536.0f, 65536.0f, 256.0f);

extern CRangeManager s_RangeMngr; // manages overwritten memory

int s_nNewWidth = -1, s_nNewHeight = -1;

void ProcessMessages();
void RenderCustom(float fAlpha); // intro anim

bool ZZCreate(int width, int height);

///////////////////////
// Method Prototypes //
///////////////////////

void ResolveInRange(int start, int end);

void ExtWrite();
extern GLuint vboRect;

void ResetRenderTarget(int index)
{
	FBTexture(index);
}

// does one time only initializing/destruction

class ZeroGSInit
{

	public:
		ZeroGSInit()
		{
			const u32 mem_size = MEMORY_END + 0x10000; // leave some room for out of range accesses (saves on the checks)
			// clear
			g_pbyGSMemory = (u8*)_aligned_malloc(mem_size, 1024);
			memset(g_pbyGSMemory, 0, mem_size);

			g_pbyGSClut = (u8*)_aligned_malloc(256 * 8, 1024); // need 512 alignment!
			memset(g_pbyGSClut, 0, 256*8);
			memset(&GLWin, 0, sizeof(GLWin));
		}

		~ZeroGSInit()
		{
			_aligned_free(g_pbyGSMemory);
			g_pbyGSMemory = NULL;
			
			_aligned_free(g_pbyGSClut);
			g_pbyGSClut = NULL;
		}
};

static ZeroGSInit s_ZeroGSInit;

#ifndef GL_FRAMEBUFFER_INCOMPLETE_DUPLICATE_ATTACHMENT_EXT
#define GL_FRAMEBUFFER_INCOMPLETE_DUPLICATE_ATTACHMENT_EXT 0x8CD8
#endif

void HandleGLError()
{
	FUNCLOG
	// check the error status of this framebuffer */
	GLenum error = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);

	// if error != GL_FRAMEBUFFER_COMPLETE_EXT, there's an error of some sort

	if (error != 0)
	{
		int w = 0;
		int h = 0;
		GLint fmt;
		glGetRenderbufferParameterivEXT(GL_COLOR_ATTACHMENT0_EXT, GL_RENDERBUFFER_INTERNAL_FORMAT_EXT, &fmt);
		glGetRenderbufferParameterivEXT(GL_COLOR_ATTACHMENT0_EXT, GL_RENDERBUFFER_WIDTH_EXT, &w);
		glGetRenderbufferParameterivEXT(GL_COLOR_ATTACHMENT0_EXT, GL_RENDERBUFFER_HEIGHT_EXT, &h);

		switch (error)
		{
			case GL_FRAMEBUFFER_COMPLETE_EXT:
				break;

			case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT:
				ZZLog::Error_Log("Error! missing a required image/buffer attachment!");
				break;

			case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT:
				ZZLog::Error_Log("Error! has no images/buffers attached!");
				break;
				
//			case GL_FRAMEBUFFER_INCOMPLETE_DUPLICATE_ATTACHMENT_EXT:
//				ZZLog::Error_Log("Error! has an image/buffer attached in multiple locations!");
//				break;

			case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT:
				ZZLog::Error_Log("Error! has mismatched image/buffer dimensions!");
				break;

			case GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT:
				ZZLog::Error_Log("Error! colorbuffer attachments have different types!");
				break;

			case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT:
				ZZLog::Error_Log("Error! trying to draw to non-attached color buffer!");
				break;

			case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER_EXT:
				ZZLog::Error_Log("Error! trying to read from a non-attached color buffer!");
				break;

			case GL_FRAMEBUFFER_UNSUPPORTED_EXT:
				ZZLog::Error_Log("Error! format is not supported by current graphics card/driver!");
				break;

			default:
				ZZLog::Error_Log("*UNKNOWN ERROR* reported from glCheckFramebufferStatusEXT(0x%x)!", error);
				break;
		}
	}
}

void ZZGSStateReset()
{
	FUNCLOG
	icurctx = -1;

	for (int i = 0; i < 2; ++i)
	{
		vb[i].Destroy();
		memset(&vb[i], 0, sizeof(VB));

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

void ChangeWindowSize(int nNewWidth, int nNewHeight)
{
	FUNCLOG
	nBackbufferWidth = max(nNewWidth, 16);
	nBackbufferHeight = max(nNewHeight, 16);

	if (!(conf.fullscreen()))
	{
		conf.width = nNewWidth;
		conf.height = nNewHeight;
	}
}

void SetChangeDeviceSize(int nNewWidth, int nNewHeight)
{
	FUNCLOG
	s_nNewWidth = nNewWidth;
	s_nNewHeight = nNewHeight;

	if (!(conf.fullscreen()))
	{
		conf.width = nNewWidth;
		conf.height = nNewHeight;
	}
}

void ChangeDeviceSize(int nNewWidth, int nNewHeight)
{
	FUNCLOG
	//int oldscreen = s_nFullscreen;

	int oldwidth = nBackbufferWidth, oldheight = nBackbufferHeight;

	if (!ZZCreate(nNewWidth&~7, nNewHeight&~7))
	{
		ZZLog::Error_Log("Failed to recreate, changing to old device.");

		if (ZZCreate(oldwidth, oldheight))
		{
			SysMessage("Failed to create device, exiting...");
			exit(0);
		}
	}

	for (int i = 0; i < 2; ++i)
	{
		vb[i].bNeedFrameCheck = vb[i].bNeedZCheck = 1;
		vb[i].CheckFrame(0);
	}

	assert(vb[0].pBufferData != NULL && vb[1].pBufferData != NULL);
}

void SetAA(int mode)
{
	FUNCLOG
	float f = 1.0f;

	// need to flush all targets
	s_RTs.ResolveAll();
	s_RTs.Destroy();
	s_DepthRTs.ResolveAll();
	s_DepthRTs.Destroy();

	AA.x = AA.y = 0;			// This is code for x0, x2, x4, x8 and x16 anti-aliasing.
	
	if (mode > 0)
	{
		// ( 1, 0 ) ; (  1, 1 ) ; ( 2, 1 ) ; ( 2, 2 ) 
		// it's used as a binary shift, so x >> AA.x, y >> AA.y
		AA.x = (mode + 1) / 2;
		AA.y = mode / 2;
		f = 2.0f;
	}

	memset(s_nResolveCounts, 0, sizeof(s_nResolveCounts));
	s_nLastResolveReset = 0;

	vb[0].prndr = NULL;
	vb[0].pdepth = NULL;
	vb[1].prndr = NULL;
	vb[1].pdepth = NULL;
	
	vb[0].bNeedFrameCheck = vb[0].bNeedZCheck = 1;
	vb[1].bNeedFrameCheck = vb[1].bNeedZCheck = 1;

	glPointSize(f);
}

void Prim()
{
	FUNCLOG

	VB& curvb = vb[prim->ctxt];

	if (curvb.CheckPrim()) Flush(prim->ctxt);

	curvb.curprim._val = prim->_val;
	curvb.curprim.prim = prim->prim;
}

extern u32 ptexLogo;
extern int nLogoWidth, nLogoHeight;

void RenderCustom(float fAlpha)
{
	FUNCLOG
	GL_REPORT_ERROR();

	fAlpha = 1;
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);   // switch to the backbuffer

	DisableAllgl() ;
	SetShaderCaller("RenderCustom");

	glViewport(0, 0, nBackbufferWidth, nBackbufferHeight);

	// play custom animation
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	// tex coords
	float4 v = float4(1 / 32767.0f, 1 / 32767.0f, 0, 0);
	ZZshSetParameter4fv(pvsBitBlt.prog, pvsBitBlt.sBitBltPos, v, "g_fBitBltPos");
	v.x = (float)nLogoWidth;
	v.y = (float)nLogoHeight;
	ZZshSetParameter4fv(pvsBitBlt.prog, pvsBitBlt.sBitBltTex, v, "g_fBitBltTex");

	v.x = v.y = v.z = v.w = fAlpha;
	ZZshSetParameter4fv(ppsBaseTexture.prog, ppsBaseTexture.sOneColor, v, "g_fOneColor");

	if (conf.wireframe()) glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	// inside vhDCb[0]'s target area, so render that region only
	ZZshGLSetTextureParameter(ppsBaseTexture.prog, ppsBaseTexture.sFinal, ptexLogo, "Logo");
	glBindBuffer(GL_ARRAY_BUFFER, vboRect);

	SET_STREAM();

	ZZshSetVertexShader(pvsBitBlt.prog);
	ZZshSetPixelShader(ppsBaseTexture.prog);
	DrawTriangleArray();
	
	// restore
	if (conf.wireframe()) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	ProcessMessages();

	GLWin.SwapGLBuffers();

	glEnable(GL_SCISSOR_TEST);
	glEnable(GL_STENCIL_TEST);

	vb[0].bSyncVars = 0;
	vb[1].bSyncVars = 0;

	GL_REPORT_ERROR();
}

//////////////////////////
// Internal Definitions //
//////////////////////////

int Values[100] = {0, };

void SetFogColor(u32 fog)
{
	FUNCLOG

//	Always set the fog color, even if it was already set.
//	if (gs.fogcol != fog)
//	{
	gs.fogcol = fog;

	FlushBoth();

	SetShaderCaller("SetFogColor");
	float4 v;

	// set it immediately
	v.SetColor(gs.fogcol);
	ZZshSetParameter4fv(g_fparamFogColor, v, "g_fParamFogColor");

//	}
}

void SetFogColor(GIFRegFOGCOL* fog)
{
	FUNCLOG
	
	SetShaderCaller("SetFogColor");
	float4 v;
	
	v.x = fog->FCR / 255.0f;
	v.y = fog->FCG / 255.0f;
	v.z = fog->FCB / 255.0f;
	ZZshSetParameter4fv(g_fparamFogColor, v, "g_fParamFogColor");
}

void ExtWrite()
{
	FUNCLOG
	ZZLog::Warn_Log("A hollow voice says 'EXTWRITE'! Nothing happens.");

	// use local DISPFB, EXTDATA, EXTBUF, and PMODE
//  int bpp, start, end;
//  tex0Info texframe;

//  bpp = 4;
//  if( texframe.psm == PSMT16S ) bpp = 3;
//  else if (PSMT_ISHALF(texframe.psm)) bpp = 2;
//
//  // get the start and end addresses of the buffer
//  GetRectMemAddress(start, end, texframe.psm, 0, 0, texframe.tw, texframe.th, texframe.tbp0, texframe.tbw);
}

////////////
// Caches //
////////////


//	case 0: return false;
//	case 1: break;
//	case 2: m_CBP[0] = TEX0.CBP; break;
//	case 3: m_CBP[1] = TEX0.CBP; break;
//	case 4: if(m_CBP[0] == TEX0.CBP) return false; m_CBP[0] = TEX0.CBP; break;
//	case 5: if(m_CBP[1] == TEX0.CBP) return false; m_CBP[1] = TEX0.CBP; break;
//	case 6: ASSERT(0); return false; // ffx2 menu
//	case 7: ASSERT(0); return false;
//	default: __assume(0);

// cld state:
// 000 - clut data is not loaded; data in the temp buffer is stored
// 001 - clut data is always loaded.
// 010 - clut data is always loaded; cbp0 = cbp.
// 011 - clut data is always loadedl cbp1 = cbp.
// 100 - cbp0 is compared with cbp. if different, clut data is loaded.
// 101 - cbp1 is compared with cbp. if different, clut data is loaded.

// GSdx sets cbp0 & cbp1 when checking for clut changes. ZeroGS sets them in texClutWrite.
bool CheckChangeInClut(u32 highdword, u32 psm)
{
	FUNCLOG
	int cld = ZZOglGet_cld_TexBits(highdword);
	int cbp = ZZOglGet_cbp_TexBits(highdword);

	// processing the CLUT after tex0/2 are written
	//ZZLog::Error_Log("high == 0x%x; cld == %d", highdword, cld);

	switch (cld)
	{
		case 0:
			return false;

		case 1:
			break;

		case 2:
			break;

		case 3:
			break;

		case 4:
			if (gs.cbp[0] == cbp) return false;
			break;

		case 5:
			if (gs.cbp[1] == cbp) return false;
			break;

		default:
			break;
	}

    // Compare the cache with current memory

    // CSM2 is not supported
    if (ZZOglGet_csm_TexBits(highdword))
		return true;

	int cpsm = ZZOglGet_cpsm_TexBits(highdword);
	int csa = ZZOglGet_csa_TexBits(highdword);
	int entries = PSMT_IS8CLUT(psm) ? 256 : 16;

	u8* GSMem = g_pbyGSMemory + cbp * 256;

    if (PSMT_IS32BIT(cpsm))
        return Cmp_ClutBuffer_GSMem<u32>((u32*)GSMem, csa, entries*4);
    else {
		// Mana Khemia triggers this.
        //ZZLog::Error_Log("16 bit clut not supported.");
		return Cmp_ClutBuffer_GSMem<u16>((u16*)GSMem, csa, entries*2);
    }
}

void texClutWrite(int ctx)
{
	FUNCLOG
	s_bTexFlush = false;

	tex0Info& tex0 = vb[ctx].tex0;

	assert(PSMT_ISCLUT(tex0.psm));

	// processing the CLUT after tex0/2 are written
	switch (tex0.cld)
	{
		case 0:
			return;

		case 1:
			break; // tex0.cld is usually 1.

		case 2:
			gs.cbp[0] = tex0.cbp;
			break;

		case 3:
			gs.cbp[1] = tex0.cbp;
			break;

		case 4:
			if (gs.cbp[0] == tex0.cbp) return;
			gs.cbp[0] = tex0.cbp;
			break;

		case 5:
			if (gs.cbp[1] == tex0.cbp) return;
			gs.cbp[1] = tex0.cbp;
			break;

		default:  //ZZLog::Debug_Log("cld isn't 0-5!");
			break;
	}

	Flush(!ctx);

    // Write the memory to clut buffer
    GSMem_to_ClutBuffer(tex0);
}

