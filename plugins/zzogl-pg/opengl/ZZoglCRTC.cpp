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
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

// Realisation of RenderCRTC function ONLY.
// It draw picture direct on screen, so here we have interlacing and frame skipping.

//------------------ Includes
#include "ZZoglCRTC.h"

using namespace ZeroGS;

//------------------ Defines
#if !defined(ZEROGS_DEVBUILD)
#define g_bSaveFrame 0
#define g_bSaveFinalFrame 0
#else
bool g_bSaveFrame = 0;  // saves the current psurfTarget
bool g_bSaveFinalFrame = 0; // saves the input to the CRTC
#endif // !defined(ZEROGS_DEVBUILD)


bool g_bCRTCBilinear = true, g_bDisplayFPS = false;
int g_nFrameRender = 10, g_nFramesSkipped = 0, s_nResolved = 0; // s_nResolved == number of targets resolved this frame
// Helper for skip frames.
int TimeLastSkip = 0;

// Adjusts vertex shader BitBltPos vector v to preserve aspect ratio. It used to emulate 4:3 or 16:9.
void ZeroGS::AdjustTransToAspect(Vector& v)
{
	double temp;
	float f;

	if (conf.width * nBackbufferHeight > conf.height * nBackbufferWidth) // limited by width
	{
		// change in ratio
		f = ((float)nBackbufferWidth / (float)conf.width) / ((float)nBackbufferHeight / (float)conf.height);
		v.y *= f;
		v.w *= f;

		// scanlines mess up when not aligned right
		v.y += (1 - (float)modf(v.y * (float)nBackbufferHeight * 0.5f + 0.05f, &temp)) * 2.0f / (float)nBackbufferHeight;
		v.w += (1 - (float)modf(v.w * (float)nBackbufferHeight * 0.5f + 0.05f, &temp)) * 2.0f / (float)nBackbufferHeight;
	}
	else // limited by height
	{
		f = ((float)nBackbufferHeight / (float)conf.height) / ((float)nBackbufferWidth / (float)conf.width);
		f -= (float)modf(f * nBackbufferWidth, &temp) / (float)nBackbufferWidth;
		v.x *= f;
		v.z *= f;
	}

	v *= 1 / 32767.0f;
}

inline bool FrameSkippingHelper()
{
	bool ShouldSkip = false;

	if (g_nFrameRender > 0)
	{
		if (g_nFrameRender < 8)
		{
			g_nFrameRender++;

			if (g_nFrameRender <= 3)
			{
				g_nFramesSkipped++;
				ShouldSkip = true;
			}
		}
	}
	else
	{
		if (g_nFrameRender < -1)
		{
			g_nFramesSkipped++;
			ShouldSkip = true;
		}

		g_nFrameRender--;
	}

#if defined _DEBUG
	if (timeGetTime() - TimeLastSkip > 15000 && ShouldSkip)
	{
		ZZLog::Debug_Log("ZZogl Skipped frames.");
		TimeLastSkip = timeGetTime();
	}
#endif

	return ShouldSkip;
}

// helper function for save frame in picture.
inline void FrameSavingHelper()
{
	if (g_bSaveFrame)
	{
		if (vb[0].prndr != NULL)
		{
			SaveTexture("frame1.tga", GL_TEXTURE_RECTANGLE_NV, vb[0].prndr->ptex, RW(vb[0].prndr->fbw), RH(vb[0].prndr->fbh));
		}

		if (vb[1].prndr != NULL && vb[0].prndr != vb[1].prndr)
		{
			SaveTexture("frame2.tga", GL_TEXTURE_RECTANGLE_NV, vb[1].prndr->ptex, RW(vb[1].prndr->fbw), RH(vb[1].prndr->fbh));
		}

#ifdef _WIN32
		else 
		{
			DeleteFile("frame2.tga");
		}
#endif
	}

	g_SaveFrameNum = 0;
	g_bSaveFlushedFrame = 1;
}

// Function populated tex0Info[2] array
inline void FrameObtainDispinfo(u32 bInterlace, tex0Info* dispinfo)
{
	for (int i = 0; i < 2; ++i)
	{

		if (!(*(u32*)(PMODE) & (1 << i)))
		{
			dispinfo[i].tw = 0;
			dispinfo[i].th = 0;
			continue;
		}

		GSRegDISPFB* pfb = i ? DISPFB2 : DISPFB1;

		GSRegDISPLAY* pd = i ? DISPLAY2 : DISPLAY1;
		int magh = pd->MAGH + 1;
		int magv = pd->MAGV + 1;

		dispinfo[i].tbp0 =  pfb->FBP << 5;
		dispinfo[i].tbw = pfb->FBW << 6;
		dispinfo[i].tw = (pd->DW + 1) / magh;
		dispinfo[i].th = (pd->DH + 1) / magv;
		dispinfo[i].psm = pfb->PSM;

		// hack!!
		// 2 * dispinfo[i].tw / dispinfo[i].th <= 1, metal slug 4

		if (bInterlace && 2 * dispinfo[i].tw / dispinfo[i].th <= 1 && !(conf.settings().interlace_2x))
		{
			dispinfo[i].th >>= 1;
		}
	}
}


// Something should be done before Renderer the picture.
inline void RenderStartHelper(u32 bInterlace)
{
	// Crashes Final Fantasy X at startup if uncommented. --arcum42
//#ifdef !defined(ZEROGS_DEVBUILD)
//	if(g_nRealFrame < 80 ) {
//		RenderCustom( min(1.0f, 2.0f - (float)g_nRealFrame / 40.0f) );
//
//	  if( g_nRealFrame == 79 )
//		SAFE_RELEASE_TEX(ptexLogo);
//	  return;
//	}
//#endif
	if (conf.mrtdepth && pvs[8] == NULL)
	{
		conf.mrtdepth = 0;
		s_bWriteDepth = false;

		ZZLog::Debug_Log("Disabling MRT depth writing\n");
	}

	FlushBoth();

	FrameSavingHelper();

	if (s_RangeMngr.ranges.size() > 0) FlushTransferRanges(NULL);

	SetShaderCaller("RenderStartHelper");

	// reset fba after every frame
	vb[0].fba.fba = 0;
	vb[1].fba.fba = 0;

	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);   // switch to the backbuffer

	glViewport(0, 0, nBackbufferWidth, nBackbufferHeight);

	// if interlace, only clear every other vsync
	if (!bInterlace)
	{
		//u32 color = COLOR_ARGB(0, BGCOLOR->R, BGCOLOR->G, BGCOLOR->B);
		glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	}

	SETVERTEXSHADER(pvsBitBlt.prog);

	glBindBuffer(GL_ARRAY_BUFFER, vboRect);
	SET_STREAM();
	GL_REPORT_ERRORD();

	if (conf.wireframe()) glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	DisableAllgl();

	GL_REPORT_ERRORD();

	if (bInterlace) g_PrevBitwiseTexX = -1;  // reset since will be using
}

// It is setting for intrelace texture multiplied vector;
// Idea is: (x, y) -- position on screen, than interlaced texture get F = 1 ot 0 depends
// on image y coord. So it we write valpha.z * F + valpha.w + 0.5 it would be swicthig odd
// and even strings at each frame
// valpha.x and y used for image blending.
inline Vector RenderGetForClip(u32 bInterlace, int interlace, int psm, FRAGMENTSHADER* prog)
{
	SetShaderCaller("RenderGetForClip");

	Vector valpha;
	// first render the current render targets, then from ptexMem

	if (psm == 1)
	{
		valpha.x = 1;
		valpha.y = 0;
	}
	else
	{
		valpha.x = 0;
		valpha.y = 1;
	}

	if (bInterlace)
	{
		if (interlace == (conf.interlace&1))
		{
			// pass if odd
			valpha.z = 1.0f;
			valpha.w = -0.4999f;
		}
		else
		{
			// pass if even
			valpha.z = -1.0f;
			valpha.w = 0.5001f;
		}
	}
	else
	{
		// always pass interlace test
		valpha.z = 0;
		valpha.w = 1;
	}

	ZZcgSetParameter4fv(prog->sOneColor, valpha, "g_fOneColor");

	return valpha;
}

// Put interlaced texture in use for shader prog.
// Note: if frame interlaced it's th is halved, so we should x2 it.
inline void RenderCreateInterlaceTex(u32 bInterlace, int th, FRAGMENTSHADER* prog)
{
	if (!bInterlace) return;

	int interlacetex = CreateInterlaceTex(2 * th);

	cgGLSetTextureParameter(prog->sInterlace, interlacetex);
	cgGLEnableTextureParameter(prog->sInterlace);
}

// Well, do blending setup prior to second pass of half-frame drawing
inline void RenderSetupBlending()
{
	// setup right blending
	glEnable(GL_BLEND);
	zgsBlendEquationSeparateEXT(GL_FUNC_ADD, GL_FUNC_ADD);

	if (PMODE->MMOD)
	{
		glBlendColorEXT(PMODE->ALP*(1 / 255.0f), PMODE->ALP*(1 / 255.0f), PMODE->ALP*(1 / 255.0f), 0.5f);
		s_srcrgb = GL_CONSTANT_COLOR_EXT;
		s_dstrgb = GL_ONE_MINUS_CONSTANT_COLOR_EXT;
	}
	else
	{
		s_srcrgb = GL_SRC_ALPHA;
		s_dstrgb = GL_ONE_MINUS_SRC_ALPHA;
	}

	if (PMODE->AMOD)
	{
		s_srcalpha = GL_ZERO;
		s_dstalpha = GL_ONE;
	}
	else
	{
		s_srcalpha = GL_ONE;
		s_dstalpha = GL_ZERO;
	}
	
	zgsBlendFuncSeparateEXT(s_srcrgb, s_dstrgb, s_srcalpha, s_dstalpha);
}

// each frame could be drawn in two stages, so blending should be different for them
inline void RenderSetupStencil(int i)
{
	glStencilMask(1 << i);
	s_stencilmask = 1 << i;
	GL_STENCILFUNC_SET();
}

// do stencil check for each found target i -- texturing stage
inline void RenderUpdateStencil(int i, bool* bUsingStencil)
{
	if (!(*bUsingStencil)) glClear(GL_STENCIL_BUFFER_BIT);

	*bUsingStencil = 1;

	glEnable(GL_STENCIL_TEST);
	GL_STENCILFUNC(GL_NOTEQUAL, 3, 1 << i);
	glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
	glStencilMask(1 << i);
}

// CRTC24 could not be rendered
inline void RenderCRTC24helper(u32 bInterlace, int interlace, int psm)
{
	ZZLog::Debug_Log("ZZogl: CRTC24!!! I'm trying to show something.");
	SetShaderCaller("RenderCRTC24helper");
	// assume that data is already in ptexMem (do Resolve?)
	RenderGetForClip(bInterlace, interlace, psm, &ppsCRTC24[bInterlace]);
	SETPIXELSHADER(ppsCRTC24[bInterlace].prog);
	
	DrawTriangleArray();
}

// Maybe I do this function global-defined. Calculate bits per pixel for
// each psm. It's the only place with PSMCT16 which have a different bpp.
// FIXME: check PSMCT16S
inline int RenderGetBpp(int psm)
{
	if (psm == PSMCT16S)
	{
		//ZZLog::Debug_Log("ZZogl: 16S target.");
		return 3;
	}

	if (PSMT_ISHALF(psm)) return 2;

	return 4;
}

// We want to draw ptarg on screen, that could be disaligned to viewport.
// So we do aligning it by height.
inline int RenderGetOffsets(int* dby, int* movy, tex0Info& texframe, CRenderTarget* ptarg, int bpp)
{
	*dby += (256 / bpp) * (texframe.tbp0 - ptarg->fbp) / texframe.tbw;

	if (*dby < 0)
	{
		*movy = -*dby;
		*dby = 0;
	}

	return min(ptarg->fbh - *dby, texframe.th - *movy);
}

// BltBit shader calculate vertex (4 coord's pixel) position at the viewport.
inline Vector RenderSetTargetBitPos(int dh, int th, int movy, bool isInterlace)
{
	SetShaderCaller("RenderSetTargetBitPos");
	Vector v;
	// dest rect
	v.x = 1;
	v.y = dh / (float)th;
	v.z = 0;
	v.w = 1 - v.y;

	if (movy > 0) v.w -= movy / (float)th;

	AdjustTransToAspect(v);

	if (isInterlace)
	{
		// move down by 1 pixel
		v.w += 1.0f / (float)dh ;
	}

	ZZcgSetParameter4fv(pvsBitBlt.sBitBltPos, v, "g_fBitBltPos");

	return v;
}

// Important stuff. We could use these coordinates to change viewport position on the frame.
// For example, use tw / X and tw / X magnify the viewport.
// Interlaced output is little out of VB, it could be seen as an evil blinking line on top
// and bottom, so we try to remove it.
inline Vector RenderSetTargetBitTex(float th, float tw, float dh, float dw, bool isInterlace)
{
	SetShaderCaller("RenderSetTargetBitTex");

	Vector v;
	v = Vector(th, tw, dh, dw);

	// Incorrect Aspect ratio on interlaced frames

	if (isInterlace)
	{
		v.y -= 1.0f / conf.height;
		v.w += 1.0f / conf.height;
	}

	ZZcgSetParameter4fv(pvsBitBlt.sBitBltTex, v, "g_fBitBltTex");

	return v;
}

// Translator for POSITION coordinats (-1.0:+1.0f at x axis, +1.0f:-1.0y at y) into target frame ones
// We don't need x coordinate, bvecause interlacing is y-axis only.
inline Vector RenderSetTargetBitTrans(int th)
{
	SetShaderCaller("RenderSetTargetBitTrans");
	Vector v = Vector(float(th), -float(th), float(th), float(th));
	ZZcgSetParameter4fv(pvsBitBlt.fBitBltTrans, v, "g_fBitBltTrans");
	return v;
}

// use g_fInvTexDims to store inverse texture dims
// Seems, that Targ shader does not use it
inline Vector RenderSetTargetInvTex(int bInterlace, int tw, int th, FRAGMENTSHADER* prog)
{
	SetShaderCaller("RenderSetTargetInvTex");

	Vector v = Vector(0, 0, 0, 0);

	if (prog->sInvTexDims)
	{
		v.x = 1.0f / (float)tw;
		v.y = 1.0f / (float)th;
		v.z = (float)0.0;
		v.w = -0.5f / (float)th;
		ZZcgSetParameter4fv(prog->sInvTexDims, v, "g_fInvTexDims");
	}

	return v;
}

// Metal Slug 5 hack (as was written). If target tbp not equal to framed fbp, than we look for a better possibility,
// Note, than after true result iterator it could not be used.
inline bool RenderLookForABetterTarget(int fbp, int tbp, list<CRenderTarget*>& listTargs, list<CRenderTarget*>::iterator& it)
{
	if (fbp == tbp) return false;

	// look for a better target (metal slug 5)
	list<CRenderTarget*>::iterator itbetter;

	for (itbetter = listTargs.begin(); itbetter != listTargs.end(); ++itbetter)
	{
		if ((*itbetter)->fbp == tbp) break;
	}

	if (itbetter != listTargs.end())
	{
		it = listTargs.erase(it);
		return true;
	}

	return false;
}

inline void RenderCheckForMemory(tex0Info& texframe, list<CRenderTarget*>& listTargs, int i, bool* bUsingStencil, int interlace, int bInterlace);

// First try to draw frame from targets. 
inline void RenderCheckForTargets(tex0Info& texframe, list<CRenderTarget*>& listTargs, int i, bool* bUsingStencil, int interlace, int bInterlace)
{
	// get the start and end addresses of the buffer
	int bpp = RenderGetBpp(texframe.psm);
	GSRegDISPFB* pfb = i ? DISPFB2 : DISPFB1;

	int start, end;
	GetRectMemAddress(start, end, texframe.psm, 0, 0, texframe.tw, texframe.th, texframe.tbp0, texframe.tbw);

	// We need share list of targets between functions
	s_RTs.GetTargs(start, end, listTargs);

	for (list<CRenderTarget*>::iterator it = listTargs.begin(); it != listTargs.end();)
	{
		CRenderTarget* ptarg = *it;

		if (ptarg->fbw == texframe.tbw && !(ptarg->status&CRenderTarget::TS_NeedUpdate) && ((256 / bpp)*(texframe.tbp0 - ptarg->fbp)) % texframe.tbw == 0)
		{
			int dby = pfb->DBY;
			int movy = 0;
			
			if (RenderLookForABetterTarget(ptarg->fbp, texframe.tbp0, listTargs, it)) continue;

			if (g_bSaveFinalFrame) SaveTexture("frame1.tga", GL_TEXTURE_RECTANGLE_NV, ptarg->ptex, RW(ptarg->fbw), RH(ptarg->fbh));

			// determine the rectangle to render
			int dh = RenderGetOffsets(&dby, &movy, texframe, ptarg, bpp);

			if (dh >= 64)
			{

				if (ptarg->fbh - dby < texframe.th - movy && !(*bUsingStencil))
					RenderUpdateStencil(i, bUsingStencil);
				else if (ptarg->fbh - dby > 2 * ( texframe.th - movy )) 
				{
					// Sometimes calculated position onscreen is misaligned, ie in FFX-2 intro. In such case some part of image are out of
					// border's and we should move it manually.
					dby -= ((ptarg->fbh - dby) >> 2) -  ((texframe.th + movy) >> 1) ;
				}

				SetShaderCaller("RenderCheckForTargets");

				// Texture
				Vector v = RenderSetTargetBitTex((float)RW(texframe.tw), (float)RH(dh), (float)RW(pfb->DBX), (float)RH(dby), INTERLACE_COUNT);

				// dest rect
				v = RenderSetTargetBitPos(dh, texframe.th, movy, INTERLACE_COUNT);
				v = RenderSetTargetBitTrans(ptarg->fbh);
				v = RenderSetTargetInvTex(bInterlace, texframe.tbw, ptarg->fbh, &ppsCRTCTarg[bInterlace]) ; 	// FIXME. This is no use

				Vector valpha = RenderGetForClip(bInterlace, interlace, texframe.psm, &ppsCRTCTarg[bInterlace]);

				// inside vb[0]'s target area, so render that region only
				cgGLSetTextureParameter(ppsCRTCTarg[bInterlace].sFinal, ptarg->ptex);
				cgGLEnableTextureParameter(ppsCRTCTarg[bInterlace].sFinal);
				RenderCreateInterlaceTex(bInterlace, texframe.th, &ppsCRTCTarg[bInterlace]);

				SETPIXELSHADER(ppsCRTCTarg[bInterlace].prog);

				DrawTriangleArray();

				if (abs(dh - (int)texframe.th) <= 1) return;

				if (abs(dh - (int)ptarg->fbh) <= 1)
				{
					it = listTargs.erase(it);
					continue;
				}
			}
		}

		++it;
	}
	RenderCheckForMemory(texframe, listTargs, i, bUsingStencil, interlace, bInterlace);
}


// The same as the previous, but from memory.
// If you ever wondered why a picture from a minute ago suddenly flashes on the screen (say, in Mana Khemia),
// this is the function that does it.
inline void RenderCheckForMemory(tex0Info& texframe, list<CRenderTarget*>& listTargs, int i, bool* bUsingStencil, int interlace, int bInterlace)
{
	Vector v;
	
	for (list<CRenderTarget*>::iterator it = listTargs.begin(); it != listTargs.end(); ++it)
	{
		(*it)->Resolve();
	}

	// context has to be 0
	CMemoryTarget* pmemtarg = g_MemTargs.GetMemoryTarget(texframe, 1);

	if ((pmemtarg == NULL) || (bInterlace >= 2))
		ZZLog::Error_Log("CRCR Check for memory shader fault.");

	//if (!(*bUsingStencil)) RenderUpdateStencil(i, bUsingStencil);
		
	SetShaderCaller("RenderCheckForMemory");

	float w1, h1, w2, h2;
	if (g_bCRTCBilinear)
	{
		w1 = texframe.tw;
		h1 = texframe.th;
		w2 = -0.5f;
		h2 = -0.5f;
		SetTexVariablesInt(0, 2, texframe, pmemtarg, &ppsCRTC[bInterlace], 1);
	}
	else
	{
		w1 = 1;
		h1 = 1;
		w2 = -0.5f / (float)texframe.tw;
		h2 = -0.5f / (float)texframe.th;
		SetTexVariablesInt(0, 0, texframe, pmemtarg, &ppsCRTC[bInterlace], 1);
	}
	
	if (g_bSaveFinalFrame) SaveTex(&texframe, g_bSaveFinalFrame - 1 > 0);
	
	// Fixme: Why is this here?
	// We should probably call RenderSetTargetBitTex instead.
	v = RenderSetTargetBitTex(w1, h1, w2, h2, INTERLACE_COUNT);

	// finally render from the memory (note that the stencil buffer will keep previous regions)
	v = RenderSetTargetBitPos(1, 1, 0, INTERLACE_COUNT);
	v = RenderSetTargetBitTrans(texframe.th);
	v = RenderSetTargetInvTex(bInterlace, texframe.tw, texframe.th, &ppsCRTC[bInterlace]);
	Vector valpha = RenderGetForClip(bInterlace, interlace, texframe.psm, &ppsCRTC[bInterlace]);

	cgGLSetTextureParameter(ppsCRTC[bInterlace].sMemory, pmemtarg->ptex->tex);
	cgGLEnableTextureParameter(ppsCRTC[bInterlace].sMemory);
	
	RenderCreateInterlaceTex(bInterlace, texframe.th, &ppsCRTC[bInterlace]);

	SETPIXELSHADER(ppsCRTC[bInterlace].prog);
	
	DrawTriangleArray();
}

// Put FPS counter on screen (not in window title)
inline void AfterRenderDisplayFPS()
{
	char str[64];
	int left = 10, top = 15;
	sprintf(str, "%.1f fps", fFPS);

	DrawText(str, left + 1, top + 1, 0xff000000);
	DrawText(str, left, top, 0xffc0ffff);
}

// Swapping buffers, so we could use another window
inline void AfterRenderSwapBuffers()
{
	if (glGetError() != GL_NO_ERROR) ZZLog::Debug_Log("glError before swap!");

	GLWin.SwapGLBuffers();
}

// SnapeShoot helper
inline void AfterRenderMadeSnapshoot()
{
	char str[64];
	int left = 200, top = 15;
	sprintf(str, "ZeroGS %d.%d.%d - %.1f fps %s", zgsrevision, zgsbuild, zgsminor, fFPS, s_frameskipping ? " - frameskipping" : "");

	DrawText(str, left + 1, top + 1, 0xff000000);
	DrawText(str, left, top, 0xffc0ffff);

	if (SaveRenderTarget(strSnapshot != "" ? strSnapshot.c_str() : "temp.jpg", nBackbufferWidth, -nBackbufferHeight, 0))  //(conf.options.tga_snap)?0:1) ) {
	{
		char str[255];
		sprintf(str, "saved %s\n", strSnapshot.c_str());
		AddMessage(str, 500);
	}
}

// If needed reset
inline void AfterRendererResizeWindow()
{
	Reset();
	ChangeDeviceSize(s_nNewWidth, s_nNewHeight);
	s_nNewWidth = s_nNewHeight = -1;
}

// Put new values on statistic variable
inline void AfterRenderCountStatistics()
{
	if (s_nWriteDepthCount > 0)
	{
		assert(conf.mrtdepth);

		if (--s_nWriteDepthCount <= 0)
		{
			s_bWriteDepth = false;
		}
	}

	if (s_nWriteDestAlphaTest > 0)
	{
		if (--s_nWriteDestAlphaTest <= 0)
		{
			s_bDestAlphaTest = false;
		}
	}

	if (g_nDepthUsed > 0) --g_nDepthUsed;

	s_ClutResolve = 0;

	g_nDepthUpdateCount = 0;
}

// This all could be easily forefeit
inline void AfterRendererUnimportantJob()
{
	ProcessMessages();

	if (g_bDisplayFPS) AfterRenderDisplayFPS();

	AfterRenderSwapBuffers();

	if (conf.wireframe())
	{
		// clear all targets
		s_nWireframeCount = 1;
	}

	if (g_bMakeSnapshot)
	{
		AfterRenderMadeSnapshoot();
		g_bMakeSnapshot = 0;
	}

	if (s_avicapturing)
		CaptureFrame();

	AfterRenderCountStatistics();

	if (s_nNewWidth >= 0 && s_nNewHeight >= 0 && !g_bIsLost)
		AfterRendererResizeWindow();

	maxmin = 608;
}

// Swich Framebuffers
inline void AfterRendererSwitchBackToTextures()
{
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, s_uFramebuffer);

	g_MemTargs.DestroyCleared();

	if (s_vecTempTextures.size() > 0)
		glDeleteTextures((GLsizei)s_vecTempTextures.size(), &s_vecTempTextures[0]);

	s_vecTempTextures.clear();

	if (EXTWRITE->WRITE & 1)
	{
		ZZLog::Warn_Log("EXTWRITE!");
		ExtWrite();
		EXTWRITE->WRITE = 0;
	}

	if (conf.wireframe()) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	glEnable(GL_SCISSOR_TEST);

	if (icurctx >= 0)
	{
		vb[icurctx].bVarsSetTarg = false;
		vb[icurctx].bVarsTexSync = false;
		vb[0].bVarsTexSync = false;
	}
}

// Reset Targets Helper, for hack.
inline void AfterRendererAutoresetTargets()
{
	if (conf.settings().auto_reset)
	{
		s_nResolveCounts[s_nCurResolveIndex] = s_nResolved;
		s_nCurResolveIndex = (s_nCurResolveIndex + 1) % ARRAY_SIZE(s_nResolveCounts);

		int total = 0;

		for (int i = 0; i < ARRAY_SIZE(s_nResolveCounts); ++i) total += s_nResolveCounts[i];

		if (total / ARRAY_SIZE(s_nResolveCounts) > 3)
		{
			if (s_nLastResolveReset > (int)(fFPS * 8))
			{
				// reset
				ZZLog::Error_Log("Video memory reset.");
				s_nLastResolveReset = 0;
				memset(s_nResolveCounts, 0, sizeof(s_nResolveCounts));

				s_RTs.ResolveAll();
				return;
//				s_RTs.Destroy();
//				s_DepthRTs.ResolveAll();
//				s_DepthRTs.Destroy();
//
//				vb[0].prndr = NULL;
//				vb[0].pdepth = NULL;
//				vb[0].bNeedFrameCheck = 1;
//				vb[0].bNeedZCheck = 1;
//				vb[1].prndr = NULL;
//				vb[1].pdepth = NULL;
//				vb[1].bNeedFrameCheck = 1;
//				vb[1].bNeedZCheck = 1;
			}
		}

		s_nLastResolveReset++;
	}

	if (s_nResolved > 8) 
		s_nResolved = 2;
	else if (s_nResolved > 0)
		--s_nResolved;
}

int count = 0;
// The main renderer function
void ZeroGS::RenderCRTC(int interlace)
{
	if (g_bIsLost || FrameSkippingHelper()) return;

	u32 bInterlace = SMODE2->INT && SMODE2->FFMD && (conf.interlace < 2);

	RenderStartHelper(bInterlace);

	bool bUsingStencil = false;
	tex0Info dispinfo[2];

	FrameObtainDispinfo(bInterlace, dispinfo);

	// start from the last circuit
	for (int i = !PMODE->SLBG; i >= 0; --i)
	{
		tex0Info& texframe = dispinfo[i];

		if (texframe.th <= 1) continue;
		if (SMODE2->INT && SMODE2->FFMD) texframe.th >>= 1;
		if (i == 0) RenderSetupBlending();
		if (bUsingStencil) RenderSetupStencil(i);

		if (texframe.psm == 0x12)
		{
			RenderCRTC24helper(bInterlace, interlace, texframe.psm);
			continue;
		}

		// We shader targets between two functions, so declare it here;
		list<CRenderTarget*> listTargs;

		// if we could not draw image from target's do it from memory
		RenderCheckForTargets(texframe, listTargs, i, &bUsingStencil, interlace, bInterlace);
	}

	GL_REPORT_ERRORD();

	glDisable(GL_BLEND);

	AfterRendererUnimportantJob();
	AfterRendererSwitchBackToTextures();
	AfterRendererAutoresetTargets();
}


