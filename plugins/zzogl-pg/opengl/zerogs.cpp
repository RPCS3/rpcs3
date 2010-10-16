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
#ifdef ZEROGS_SSE2
#include <emmintrin.h>
#endif

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

// returns the first and last addresses aligned to a page that cover
void GetRectMemAddress(int& start, int& end, int psm, int x, int y, int w, int h, int bp, int bw);

int s_nNewWidth = -1, s_nNewHeight = -1;
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

void KickPoint();
void KickLine();
void KickTriangle();
void KickTriangleFan();
void KickSprite();
void KickDummy();

void ResolveInRange(int start, int end);

void ExtWrite();

void ResetRenderTarget(int index)
{
	FBTexture(index);
}

DrawFn drawfn[8] = { KickDummy, KickDummy, KickDummy, KickDummy,
					 KickDummy, KickDummy, KickDummy, KickDummy
				   };

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

void ZZReset()
{
	FUNCLOG
	s_RTs.ResolveAll();
	s_DepthRTs.ResolveAll();

	vb[0].nCount = 0;
	vb[1].nCount = 0;

	memset(s_nResolveCounts, 0, sizeof(s_nResolveCounts));
	s_nLastResolveReset = 0;

	icurctx = -1;
	g_vsprog = g_psprog = 0;

	ZZGSStateReset();
	ZZDestroy(0);

	drawfn[0] = KickDummy;
	drawfn[1] = KickDummy;
	drawfn[2] = KickDummy;
	drawfn[3] = KickDummy;
	drawfn[4] = KickDummy;
	drawfn[5] = KickDummy;
	drawfn[6] = KickDummy;
	drawfn[7] = KickDummy;
}

void ZZGSReset()
{
	FUNCLOG

	memset(&gs, 0, sizeof(gs));

	ZZGSStateReset();

	gs.prac = 1;
	prim = &gs._prim[0];
	gs.nTriFanVert = -1;
	gs.imageTransfer = -1;
	gs.q = 1;
}

void ZZGSSoftReset(u32 mask)
{
	FUNCLOG

	if (mask & 1) memset(&gs.path[0], 0, sizeof(gs.path[0]));
	if (mask & 2) memset(&gs.path[1], 0, sizeof(gs.path[1]));
	if (mask & 4) memset(&gs.path[2], 0, sizeof(gs.path[2]));

	gs.imageTransfer = -1;
	gs.q = 1;
	gs.nTriFanVert = -1;
}

void ZZAddMessage(const char* pstr, u32 ms)
{
	FUNCLOG
	listMsgs.push_back(MESSAGE(pstr, timeGetTime() + ms));
	ZZLog::Log("%s\n", pstr);
}

extern RasterFont* font_p;
void DrawText(const char* pstr, int left, int top, u32 color)
{
	FUNCLOG
	ZZshGLDisableProfile();

	float4 v;
	v.SetColor(color);
	glColor3f(v.z, v.y, v.x);
	//glColor3f(((color >> 16) & 0xff) / 255.0f, ((color >> 8) & 0xff)/ 255.0f, (color & 0xff) / 255.0f);

	font_p->printString(pstr, left * 2.0f / (float)nBackbufferWidth - 1, 1 - top * 2.0f / (float)nBackbufferHeight, 0);
	ZZshGLEnableProfile();
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

void ProcessMessages()
{
	FUNCLOG

	if (listMsgs.size() > 0)
	{
		int left = 25, top = 15;
		list<MESSAGE>::iterator it = listMsgs.begin();

		while (it != listMsgs.end())
		{
			DrawText(it->str, left + 1, top + 1, 0xff000000);
			DrawText(it->str, left, top, 0xffffff30);
			top += 15;

			if ((int)(it->dwTimeStamp - timeGetTime()) < 0)
				it = listMsgs.erase(it);
			else ++it;
		}
	}
}

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


__forceinline void MOVZ(VertexGPU *p, u32 gsz, const VB& curvb)
{
	p->z = (curvb.zprimmask == 0xffff) ? min((u32)0xffff, gsz) : gsz;
}

__forceinline void MOVFOG(VertexGPU *p, Vertex gsf)
{
	p->f = ((s16)(gsf).f << 7) | 0x7f;
}


int Values[100] = {0, };

inline void SET_VERTEX(VertexGPU *p, int Index, const VB& curvb)
{
	int index = Index;
	p->x = ((((int)gs.gsvertex[index].x - curvb.offset.x) >> 1) & 0xffff);
	p->y = ((((int)gs.gsvertex[index].y - curvb.offset.y) >> 1) & 0xffff);
	p->f = ((s16)gs.gsvertex[index].f << 7) | 0x7f;

	MOVZ(p, gs.gsvertex[index].z, curvb);

	p->rgba = prim->iip ? gs.gsvertex[index].rgba : gs.rgba;

// 	This code is somehow incorrect
//	if ((gs.texa.aem) && ((p->rgba & 0xffffff ) == 0))
//		p->rgba = 0;

	if (conf.settings().texa)
	{
		u32 B = ((p->rgba & 0xfe000000) >> 1) +
				(0x01000000 * curvb.fba.fba) ;
		p->rgba = (p->rgba & 0xffffff) + B;
	}

	if (prim->tme)
	{
		if (prim->fst)
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

static __forceinline void OUTPUT_VERT(VertexGPU vert, u32 id)
{
#ifdef WRITE_PRIM_LOGS
	ZZLog::Prim_Log("%c%d(%d): xyzf=(%4d,%4d,0x%x,%3d), rgba=0x%8.8x, stq = (%2.5f,%2.5f,%2.5f)\n",
					id == 0 ? '*' : ' ', id, prim->prim, vert.x / 8, vert.y / 8, vert.z, vert.f / 128,
					vert.rgba, Clamp(vert.s, -10, 10), Clamp(vert.t, -10, 10), Clamp(vert.q, -10, 10));
#endif
}

void KickPoint()
{
	FUNCLOG
	assert(gs.primC >= 1);

	VB& curvb = vb[prim->ctxt];

	curvb.FlushTexData();

	if ((vb[!prim->ctxt].nCount > 0) && (vb[prim->ctxt].gsfb.fbp == vb[!prim->ctxt].gsfb.fbp))
	{
		assert(vb[prim->ctxt].nCount == 0);
		Flush(!prim->ctxt);
	}

	curvb.NotifyWrite(1);

	int last = gs.primNext(2);

	VertexGPU* p = curvb.pBufferData + curvb.nCount;
	SET_VERTEX(&p[0], last, curvb);
	curvb.nCount++;

	OUTPUT_VERT(p[0], 0);
}

void KickLine()
{
	FUNCLOG
	assert(gs.primC >= 2);
	VB& curvb = vb[prim->ctxt];

	curvb.FlushTexData();

	if ((vb[!prim->ctxt].nCount > 0) && (vb[prim->ctxt].gsfb.fbp == vb[!prim->ctxt].gsfb.fbp))
	{
		assert(vb[prim->ctxt].nCount == 0);
		Flush(!prim->ctxt);
	}

	curvb.NotifyWrite(2);

	int next = gs.primNext();
	int last = gs.primNext(2);

	VertexGPU* p = curvb.pBufferData + curvb.nCount;
	SET_VERTEX(&p[0], next, curvb);
	SET_VERTEX(&p[1], last, curvb);

	curvb.nCount += 2;

	OUTPUT_VERT(p[0], 0);
	OUTPUT_VERT(p[1], 1);
}

void KickTriangle()
{
	FUNCLOG
	assert(gs.primC >= 3);
	VB& curvb = vb[prim->ctxt];

	curvb.FlushTexData();

	if ((vb[!prim->ctxt].nCount > 0) && (vb[prim->ctxt].gsfb.fbp == vb[!prim->ctxt].gsfb.fbp))
	{
		assert(vb[prim->ctxt].nCount == 0);
		Flush(!prim->ctxt);
	}

	curvb.NotifyWrite(3);

	VertexGPU* p = curvb.pBufferData + curvb.nCount;
	SET_VERTEX(&p[0], 0, curvb);
	SET_VERTEX(&p[1], 1, curvb);
	SET_VERTEX(&p[2], 2, curvb);

	curvb.nCount += 3;

	OUTPUT_VERT(p[0], 0);
	OUTPUT_VERT(p[1], 1);
	OUTPUT_VERT(p[2], 2);
}

void KickTriangleFan()
{
	FUNCLOG
	assert(gs.primC >= 3);
	VB& curvb = vb[prim->ctxt];

	curvb.FlushTexData();

	if ((vb[!prim->ctxt].nCount > 0) && (vb[prim->ctxt].gsfb.fbp == vb[!prim->ctxt].gsfb.fbp))
	{
		assert(vb[prim->ctxt].nCount == 0);
		Flush(!prim->ctxt);
	}

	curvb.NotifyWrite(3);

	VertexGPU* p = curvb.pBufferData + curvb.nCount;
	SET_VERTEX(&p[0], 0, curvb);
	SET_VERTEX(&p[1], 1, curvb);
	SET_VERTEX(&p[2], 2, curvb);

	curvb.nCount += 3;

	// add 1 to skip the first vertex

	if (gs.primIndex == gs.nTriFanVert) gs.primIndex = gs.primNext();

	OUTPUT_VERT(p[0], 0);
	OUTPUT_VERT(p[1], 1);
	OUTPUT_VERT(p[2], 2);
}

void SetKickVertex(VertexGPU *p, Vertex v, int next, const VB& curvb)
{
	SET_VERTEX(p, next, curvb);
	MOVZ(p, v.z, curvb);
	MOVFOG(p, v);
}

void KickSprite()
{
	FUNCLOG
	assert(gs.primC >= 2);
	VB& curvb = vb[prim->ctxt];

	curvb.FlushTexData();

	if ((vb[!prim->ctxt].nCount > 0) && (vb[prim->ctxt].gsfb.fbp == vb[!prim->ctxt].gsfb.fbp))
	{
		assert(vb[prim->ctxt].nCount == 0);
		Flush(!prim->ctxt);
	}

	curvb.NotifyWrite(6);
	int next = gs.primNext();
	int last = gs.primNext(2);
	
	// sprite is too small and AA shows lines (tek4, Mana Khemia)
	gs.gsvertex[last].x += (4 * AA.x);
	gs.gsvertex[last].y += (4 * AA.y);

	// might be bad sprite (KH dialog text)
	//if( gs.gsvertex[next].x == gs.gsvertex[last].x || gs.gsvertex[next].y == gs.gsvertex[last].y )
	//return;

	VertexGPU* p = curvb.pBufferData + curvb.nCount;

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

	OUTPUT_VERT(p[0], 0);
	OUTPUT_VERT(p[1], 1);
}

void KickDummy()
{
	FUNCLOG
	//ZZLog::Greg_Log("Kicking bad primitive: %.8x\n", *(u32*)prim);
}

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

bool IsDirty(u32 highdword, u32 psm, int cld, int cbp)
{
	int cpsm = ZZOglGet_cpsm_TexBits(highdword);
	int csm = ZZOglGet_csm_TexBits(highdword);

	if (cpsm > 1 || csm)
	{
		// Mana Khemia triggers this.
        //ZZLog::Error_Log("16 bit clut not supported.");
		return true;
	}

	int csa = ZZOglGet_csa_TexBits(highdword);

	int entries = PSMT_IS8CLUT(psm) ? 256 : 16;

	u64* src = (u64*)(g_pbyGSMemory + cbp * 256);
	u64* dst = (u64*)(g_pbyGSClut + 64 * csa);

	bool bRet = false;

#define TEST_THIS
#ifdef TEST_THIS
    while(entries != 0) {
#ifdef ZEROGS_SSE2
        // Note: local memory datas are swizzles
        __m128i src_0 = _mm_load_si128((__m128i*)src);   // 9  8  1 0
        __m128i src_1 = _mm_load_si128((__m128i*)src+1); // 11 10 3 2
        __m128i src_2 = _mm_load_si128((__m128i*)src+2); // 13 12 5 4
        __m128i src_3 = _mm_load_si128((__m128i*)src+3); // 15 14 7 6

        __m128i dst_0 = _mm_load_si128((__m128i*)dst);
        __m128i dst_1 = _mm_load_si128((__m128i*)dst+1);
        __m128i dst_2 = _mm_load_si128((__m128i*)dst+2);
        __m128i dst_3 = _mm_load_si128((__m128i*)dst+3);

        __m128i result = _mm_cmpeq_epi32(_mm_unpacklo_epi64(src_0, src_1), dst_0);

        __m128i result_tmp = _mm_cmpeq_epi32(_mm_unpacklo_epi64(src_2, src_3), dst_1);
        result = _mm_and_si128(result, result_tmp);

        result_tmp = _mm_cmpeq_epi32(_mm_unpackhi_epi64(src_0, src_1), dst_2);
        result = _mm_and_si128(result, result_tmp);

        result_tmp = _mm_cmpeq_epi32(_mm_unpackhi_epi64(src_2, src_3), dst_3);
        result = _mm_and_si128(result, result_tmp);

        u32 result_int = _mm_movemask_epi8(result);
        if (result_int != 0xFFFF) {
            bRet = true;
            break;
        }
#else
        // I see no point to keep an mmx version. SSE2 versions is probably faster.
        // Keep a slow portable C version for reference/debug
        // Note: local memory datas are swizzles
        if (dst[0] != src[0] || dst[1] != src[2] || dst[2] != src[4] || dst[3] != src[6]
                || dst[4] != src[1] || dst[5] != src[3] || dst[6] != src[5] || dst[7] != src[7]) {
            bRet = true;
            break;
        }
#endif

        // go to the next memory block
        src += 32;

        // go back to the previous memory block then down one memory column
        if (entries & 0x10) {
            src -= (64-8);
        }
        // In case previous operation (down one column) cross the block boundary
        // Go to the next block
        if (entries == 0x90) {
            src += 32;
        }

        dst += 8;
        entries -= 16;
    }
#else

	// do a fast test with MMX
#ifdef _MSC_VER
	int storeebx;
	__asm
	{
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
		"cmp %%esi, 16\n"
		"jle Return\n"
		"test %%esi, 0x10\n"
		"jz AddEcx\n"
		"sub %%edx, 448\n" // go back and down one column
		"AddEcx:\n"
		"add %%edx, 256\n" // go to the right block
		"cmp %%esi, 0x90\n"
		"jne Continue1\n"
		"add %%edx, 256\n" // skip whole block
		"Continue1:\n"
		"add %%ecx, 64\n"
		"sub %%esi, 16\n"
		"jmp Start\n"
		"Return:\n"
		"emms\n"

	".att_syntax\n" : "=m"(bRet) : "c"(dst), "d"(src), "S"(entries) : "eax", "memory");

#endif // _WIN32
#endif
	return bRet;
}

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

			//case 4: return gs.cbp[0] != cbp;
			//case 5: return gs.cbp[1] != cbp;

			// default: load

		default:
			break;
	}

	return IsDirty(highdword, psm, cld, cbp);
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

	int entries = PSMT_IS8CLUT(tex0.psm) ? 256 : 16;

	if (tex0.csm)
	{
		switch (tex0.cpsm)
		{
				// 16bit psm
				// eggomania uses non16bit textures for csm2

			case PSMCT16:
			{
				u16* src = (u16*)g_pbyGSMemory + tex0.cbp * 128;
				u16 *dst = (u16*)(g_pbyGSClut + 64 * (tex0.csa & 15) + (tex0.csa >= 16 ? 2 : 0));

				for (int i = 0; i < entries; ++i)
				{
					*dst = src[getPixelAddress16_0(gs.clut.cou+i, gs.clut.cov, gs.clut.cbw)];
					dst += 2;

					// check for wrapping

					if (((u32)(uptr)dst & 0x3ff) == 0) dst = (u16*)(g_pbyGSClut + 2);
				}
				break;
			}

			case PSMCT16S:
			{
				u16* src = (u16*)g_pbyGSMemory + tex0.cbp * 128;
				u16 *dst = (u16*)(g_pbyGSClut + 64 * (tex0.csa & 15) + (tex0.csa >= 16 ? 2 : 0));

				for (int i = 0; i < entries; ++i)
				{
					*dst = src[getPixelAddress16S_0(gs.clut.cou+i, gs.clut.cov, gs.clut.cbw)];
					dst += 2;

					// check for wrapping

					if (((u32)(uptr)dst & 0x3ff) == 0) dst = (u16*)(g_pbyGSClut + 2);
				}
				break;
			}

			case PSMCT32:
			case PSMCT24:
			{
				u32* src = (u32*)g_pbyGSMemory + tex0.cbp * 64;
				u32 *dst = (u32*)(g_pbyGSClut + 64 * tex0.csa);

				// check if address exceeds src

				if (src + getPixelAddress32_0(gs.clut.cou + entries - 1, gs.clut.cov, gs.clut.cbw) >= (u32*)g_pbyGSMemory + 0x00100000)
					ZZLog::Error_Log("texClutWrite out of bounds.");
				else
					for (int i = 0; i < entries; ++i)
					{
						*dst = src[getPixelAddress32_0(gs.clut.cou+i, gs.clut.cov, gs.clut.cbw)];
						dst++;
					}
				break;
			}

			default:
			{
				//ZZLog::Debug_Log("Unknown cpsm: %x (%x).", tex0.cpsm, tex0.psm);
				break;
			}
		}
	}
	else
	{
		u32* src = (u32*)(g_pbyGSMemory + 256 * tex0.cbp);
		
		if (entries == 16)
		{
			switch (tex0.cpsm)
			{
				case PSMCT24:
				case PSMCT32:
					WriteCLUT_T32_I4_CSM1(src, (u32*)(g_pbyGSClut + 64 * tex0.csa));
					break;

				default:
#ifdef ZEROGS_SSE2
					WriteCLUT_T16_I4_CSM1_sse2(src, tex0.csa);
#else
					WriteCLUT_T16_I4_CSM1_c(src, (u32*)(g_pbyGSClut + 64*(tex0.csa & 15) + (tex0.csa >= 16 ? 2 : 0)));
#endif
					break;
			}
		}
		else
		{
			switch (tex0.cpsm)
			{
				case PSMCT24:
				case PSMCT32:
					WriteCLUT_T32_I8_CSM1(src, (u32*)(g_pbyGSClut + 64 * tex0.csa));
					break;

				default:
					// sse2 for 256 is more complicated, so use regular
#ifdef ZEROGS_SSE2
					WriteCLUT_T16_I8_CSM1_sse2(src, tex0.csa);
#else
					WriteCLUT_T16_I8_CSM1_c(src, (u32*)(g_pbyGSClut + 64*(tex0.csa & 15) + (tex0.csa >= 16 ? 2 : 0)));
#endif
					break;
			}

		}
	}
}


