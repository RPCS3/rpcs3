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

//-------------------------- Includes
#if defined(_WIN32)
#	include <windows.h>
//#	include <aviUtil.h>
#	include "resource.h"
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

//----------------------- Defines     

//-------------------------- Typedefs
typedef void (APIENTRYP _PFNSWAPINTERVAL) (int);

//-------------------------- Extern variables
using namespace ZeroGS;

extern u32 g_nGenVars, g_nTexVars, g_nAlphaVars, g_nResolve;
extern char *libraryName;
extern int g_nFrame, g_nRealFrame;

//-------------------------- Variables

#ifdef _WIN32
HDC		 hDC=NULL;	   // Private GDI Device Context		
HGLRC	   hRC=NULL;	   // Permanent Rendering Context		
#endif

bool g_bIsLost = 0;									// ZZ

BOOL g_bMakeSnapshot = 0;
string strSnapshot;

CGprogram g_vsprog = 0, g_psprog = 0;							// 2 -- ZZ
// AVI Capture
int s_avicapturing = 0;

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
CGparameter g_vparamPosXY[2] = {0}, g_fparamFogColor = 0;

map<int, SHADERHEADER*> mapShaderResources;

bool s_bTexFlush = FALSE;			
int s_nLastResolveReset = 0;
int s_nWireframeCount = 0;
int s_nResolveCounts[30] = {0}; // resolve counts for last 30 frames

////////////////////
// State parameters
CGcontext g_cgcontext;
int nBackbufferWidth, nBackbufferHeight;

u8* g_pbyGSMemory = NULL;   // 4Mb GS system mem
u8* g_pbyGSClut = NULL;													// ZZ

namespace ZeroGS
{
	Vector g_vdepth, vlogz;

//       	= Vector( 255.0 /256.0f,  255.0/65536.0f, 255.0f/(65535.0f*256.0f), 1.0f/(65536.0f*65536.0f));
//	Vector g_vdepth = Vector( 65536.0f*65536.0f, 256.0f*65536.0f, 65536.0f, 256.0f);	

	extern CRangeManager s_RangeMngr; // manages overwritten memory				
	GLenum GetRenderTargetFormat() { return GetRenderFormat()==RFT_byte8?4:g_internalRGBAFloat16Fmt; }

	// returns the first and last addresses aligned to a page that cover 
	void GetRectMemAddress(int& start, int& end, int psm, int x, int y, int w, int h, int bp, int bw);

//	bool LoadEffects();
//	bool LoadExtraEffects();
//	FRAGMENTSHADER* LoadShadeEffect(int type, int texfilter, int fog, int testaem, int exactcolor, const clampInfo& clamp, int context, bool* pbFailed);

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

	void ResetRenderTarget(int index) {
		glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT+index, GL_TEXTURE_RECTANGLE_NV, 0, 0 );
	}

	DrawFn drawfn[8] = { KickDummy, KickDummy, KickDummy, KickDummy,
						KickDummy, KickDummy, KickDummy, KickDummy };

}; // end namespace

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

#ifndef GL_FRAMEBUFFER_INCOMPLETE_DUPLICATE_ATTACHMENT_EXT
#define GL_FRAMEBUFFER_INCOMPLETE_DUPLICATE_ATTACHMENT_EXT 0x8CD8
#endif

void ZeroGS::HandleGLError()
{
	FUNCLOG
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


void ZeroGS::GSStateReset()
{
	FUNCLOG
	icurctx = -1;

	for(int i = 0; i < 2; ++i) {
		vb[i].Destroy();
		memset(&vb[i], 0, sizeof(ZeroGS::VB));

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
	FUNCLOG
	listMsgs.push_back(MESSAGE(pstr, timeGetTime()+ms));
}

void ZeroGS::DrawText(const char* pstr, int left, int top, u32 color)
{
	FUNCLOG
	cgGLDisableProfile(cgvProf);
	cgGLDisableProfile(cgfProf);
	
	glColor3f(((color>>16)&0xff)/255.0f, ((color>>8)&0xff)/255.0f, (color&0xff)/255.0f);
	
	font_p->printString(pstr, left * 2.0f / (float)nBackbufferWidth - 1, 1 - top * 2.0f / (float)nBackbufferHeight,0);
	cgGLEnableProfile(cgvProf);
	cgGLEnableProfile(cgfProf);
}

void ZeroGS::ChangeWindowSize(int nNewWidth, int nNewHeight)
{
	FUNCLOG
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
	FUNCLOG
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
	FUNCLOG
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
	FUNCLOG
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

	assert( vb[0].pBufferData != NULL && vb[1].pBufferData != NULL );
}


void ZeroGS::SetNegAA(int mode) {	
	FUNCLOG
	// need to flush all targets
	s_RTs.ResolveAll();
	s_RTs.Destroy();
	s_DepthRTs.ResolveAll();
	s_DepthRTs.Destroy();
	
	s_AAz = s_AAw = 0;			// This is code for x0, x2, x4, x8 and x16 anti-aliasing.
	if (mode > 0) 
	{
		s_AAz = (mode+1) / 2;		// ( 1, 0 ) ; (  1, 1 ) -- it's used as binary shift, so x << s_AAz, y << s_AAw
		s_AAw = mode / 2;
	}

	memset(s_nResolveCounts, 0, sizeof(s_nResolveCounts));
	s_nLastResolveReset = 0;

	vb[0].prndr = NULL; vb[0].pdepth = NULL; vb[0].bNeedFrameCheck = 1; vb[0].bNeedZCheck = 1;
	vb[1].prndr = NULL; vb[1].pdepth = NULL; vb[1].bNeedFrameCheck = 1; vb[1].bNeedZCheck = 1;
}

void ZeroGS::SetAA(int mode)
{
	FUNCLOG
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

void ZeroGS::Prim(){
	FUNCLOG
	if( g_bIsLost )
		return;

	VB& curvb = vb[prim->ctxt];

	if( curvb.CheckPrim() ){
		Flush(prim->ctxt);
	}
	curvb.curprim._val = prim->_val;

	// flush the other pipe if sharing the same buffer
//  if( vb[prim->ctxt].gsfb.fbp == vb[!prim->ctxt].gsfb.fbp && vb[!prim->ctxt].nCount > 0 )
//  {
//	  assert( vb[prim->ctxt].nCount == 0 );
//	  Flush(!prim->ctxt);
//  }

	curvb.curprim.prim = prim->prim;
}

void ZeroGS::ProcessMessages()
{
	FUNCLOG
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
	FUNCLOG
	GL_REPORT_ERROR();

	fAlpha = 1;
	glBindFramebufferEXT( GL_FRAMEBUFFER_EXT, 0 ); // switch to the backbuffer

	DisableAllgl() ;
	SetShaderCaller("RenderCustom");

	glViewport(0, 0, nBackbufferWidth, nBackbufferHeight);

	// play custom animation
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);

	// tex coords
	Vector v = Vector(1/32767.0f, 1/32767.0f, 0, 0);
	ZZcgSetParameter4fv(pvsBitBlt.sBitBltPos, v, "g_fBitBltPos");
	v.x = (float)nLogoWidth;
	v.y = (float)nLogoHeight;
	ZZcgSetParameter4fv(pvsBitBlt.sBitBltTex, v, "g_fBitBltTex");

	v.x = v.y = v.z = v.w = fAlpha;
	ZZcgSetParameter4fv(ppsBaseTexture.sOneColor, v, "g_fOneColor");

	if( conf.options & GSOPTION_WIREFRAME ) glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	// inside vhDCb[0]'s target area, so render that region only
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
		glXSwapBuffers(GLWin.dpy, GLWin.win);
#endif

	glEnable(GL_SCISSOR_TEST);
	glEnable(GL_STENCIL_TEST);

	vb[0].bSyncVars = 0;
	vb[1].bSyncVars = 0;

	GL_REPORT_ERROR();
	GLint status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
	assert( status == GL_FRAMEBUFFER_COMPLETE_EXT || status == GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT );
}

void ZeroGS::Restore()
{
	FUNCLOG
	if( !g_bIsLost )
		return;

	//if( SUCCEEDED(pd3dDevice->Reset(&d3dpp)) ) {
		g_bIsLost = 0;
		// handle lost states
		ZeroGS::ChangeDeviceSize(nBackbufferWidth, nBackbufferHeight);
	//}
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


int Values[100]={0,};
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

// 	This code is somehow icorrect	
//	if ((gs.texa.aem) && ((p->rgba & 0xffffff ) == 0)) 
//		p->rgba = 0;

	if (g_GameSettings & GAME_TEXAHACK) {
		u32 B = (( p->rgba & 0xfe000000 ) >> 1) + 
			            (0x01000000 * curvb.fba.fba) ;
		p->rgba = ( p->rgba & 0xffffff ) + B;
	}

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
	FUNCLOG
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
	FUNCLOG
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
	FUNCLOG
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
	FUNCLOG
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

__forceinline void SetKickVertex(VertexGPU *p, Vertex v, int next, const VB& curvb) 
{
	SET_VERTEX(p, next, curvb); 
	MOVZ(p, v.z, curvb);	
	MOVFOG(p, v);
}

void ZeroGS::KickSprite()
{
	FUNCLOG
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
	FUNCLOG
	//GREG_LOG("Kicking bad primitive: %.8x\n", *(u32*)prim);
}

void ZeroGS::SetFogColor(u32 fog)
{
	FUNCLOG
	if( 1||gs.fogcol != fog ) {
		gs.fogcol = fog;

		ZeroGS::Flush(0);
		ZeroGS::Flush(1);

		if( !g_bIsLost ) {
			SetShaderCaller("SetFogColor");
			Vector v;

			// set it immediately
			v.x = (gs.fogcol&0xff)/255.0f;
			v.y = ((gs.fogcol>>8)&0xff)/255.0f;
			v.z = ((gs.fogcol>>16)&0xff)/255.0f;
			ZZcgSetParameter4fv(g_fparamFogColor, v, "g_fParamFogColor");
		}
	}
}

void ZeroGS::ExtWrite()
{
	FUNCLOG
	WARN_LOG("ExtWrite\n");

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
#ifdef __x86_64__
extern "C" void TestClutChangeMMX(void* src, void* dst, int entries, void* pret);
#endif

bool ZeroGS::CheckChangeInClut(u32 highdword, u32 psm)
{
	FUNCLOG
	int cld = ZZOglGet_cld_TexBits(highdword);
	int cbp = ZZOglGet_cbp_TexBits(highdword);
	
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

	int cpsm = ZZOglGet_cpsm_TexBits(highdword);
	int csm = ZZOglGet_csm_TexBits(highdword);
	
	if( cpsm > 1 || csm )
		// don't support 16bit for now
		return true;

	int csa = ZZOglGet_csa_TexBits(highdword);

	int entries = PSMT_IS8CLUT(psm) ? 256 : 16;

	u64* src = (u64*)(g_pbyGSMemory + cbp*256);
	u64* dst = (u64*)(g_pbyGSClut+64*csa);

	bool bRet = false;

	// do a fast test with MMX
#ifdef _MSC_VER

#ifdef __x86_64__
	TestClutChangeMMX(dst, src, entries, &bRet);
#else
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
#endif // __x86_64__

#else // linux

#ifdef __x86_64__
	__asm__(
		".intel_syntax\n"
"Start:\n"
		"movq %%mm0, [%%rcx]\n"
		"movq %%mm1, [%%rcx+8]\n"
		"pcmpeqd %%mm0, [%%rdx]\n"
		"pcmpeqd %%mm1, [%%rdx+16]\n"
		"movq %%mm2, [%%rcx+16]\n"
		"movq %%mm3, [%%rcx+24]\n"
		"pcmpeqd %%mm2, [%%rdx+32]\n"
		"pcmpeqd %%mm3, [%%rdx+48]\n"
		"pand %%mm0, %%mm1\n"
		"pand %%mm2, %%mm3\n"
		"movq %%mm4, [%%rcx+32]\n"
		"movq %%mm5, [%%rcx+40]\n"
		"pcmpeqd %%mm4, [%%rdx+8]\n"
		"pcmpeqd %%mm5, [%%rdx+24]\n"
		"pand %%mm0, %%mm2\n"
		"pand %%mm4, %%mm5\n"
		"movq %%mm6, [%%rcx+48]\n"
		"movq %%mm7, [%%rcx+56]\n"
		"pcmpeqd %%mm6, [%%rdx+40]\n"
		"pcmpeqd %%mm7, [%%rdx+56]\n"
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
		"cmp %%rbx, 16\n"
		"jle Return\n"
		"test %%rbx, 0x10\n"
		"jz AddRcx\n"
		"sub %%rdx, 448\n" // go back and down one column
"AddRcx:\n"
		"add %%rdx, 256\n" // go to the right block
		"cmp %%rbx, 0x90\n"
		"jne Continue1\n"
		"add %%rdx, 256\n" // skip whole block
"Continue1:\n"
		"add %%rcx, 64\n"
		"sub %%rbx, 16\n"
		"jmp Start\n"
"Return:\n"
		"emms\n"
		".att_syntax\n" : "=m"(bRet) : "c"(dst), "d"(src), "b"(entries) : "rax", "memory");
#else
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
		".att_syntax\n" : "=m"(bRet) : "c"(dst), "d"(src), "b"(entries) : "eax", "memory");
#endif // __x86_64__

#endif // _WIN32

	return bRet;
}

void ZeroGS::texClutWrite(int ctx)
{
	FUNCLOG
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

	int entries = PSMT_IS8CLUT(tex0.psm) ? 256 : 16;

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


