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

// Realisation of RenderCRTC function ONLY.
// It draw picture direct on screen, so here we have interlacing and frame skipping.

//------------------ Includes
#include "Util.h"
#include "ZZoglCRTC.h"
#include "GLWin.h"
#include "ZZoglShaders.h"
#include "ZZoglShoots.h"
#include "ZZoglDrawing.h"
#include "rasterfont.h" // simple font
#include <math.h>
#include "ZZoglVB.h"

//------------------ Defines
#if !defined(ZEROGS_DEVBUILD)
#define g_bSaveFrame 0
#define g_bSaveFinalFrame 0
#else
bool g_bSaveFrame = 0;  // saves the current psurfTarget
bool g_bSaveFinalFrame = 0; // saves the input to the CRTC
#endif // !defined(ZEROGS_DEVBUILD)

extern int maxmin;
extern bool g_bCRTCBilinear;
bool g_bDisplayFPS = false;
int g_nFrameRender = 10, g_nFramesSkipped = 0, s_nResolved = 0; // s_nResolved == number of targets resolved this frame
// Helper for skip frames.
int TimeLastSkip = 0;

vector<u32> s_vecTempTextures;		   // temporary textures, released at the end of every frame

// Snapshot variables.
extern bool g_bMakeSnapshot;
extern string strSnapshot;

extern void ExtWrite();
extern void ZZDestroy();
extern void ChangeDeviceSize(int nNewWidth, int nNewHeight);

extern GLuint vboRect;

// I'm making this variable global for the moment in the course of fiddling with the interlace code 
// to try and make it more straightforward.
int interlace_mode = 0; // 0 - not interlacing, 1 - interlacing.
bool bUsingStencil = false;

bool INTERLACE_COUNT()
{
	return (interlace_mode && (gs.interlace == conf.interlace));
}

// Adjusts vertex shader BitBltPos vector v to preserve aspect ratio. It used to emulate 4:3 or 16:9.
void AdjustTransToAspect(float4& v)
{
	double temp;
	float f;
	const float mult = 1 / 32767.0f;

	if (conf.width * GLWin.backbuffer.h > conf.height * GLWin.backbuffer.w) // limited by width
	{
		// change in ratio
		f = ((float)GLWin.backbuffer.w / (float)conf.width) / ((float)GLWin.backbuffer.h / (float)conf.height);
		v.y *= f;
		v.w *= f;

		// scanlines mess up when not aligned right
		v.y += (1 - (float)modf(v.y * (float)GLWin.backbuffer.h * 0.5f + 0.05f, &temp)) * 2.0f / (float)GLWin.backbuffer.h;
		v.w += (1 - (float)modf(v.w * (float)GLWin.backbuffer.h * 0.5f + 0.05f, &temp)) * 2.0f / (float)GLWin.backbuffer.h;
	}
	else // limited by height
	{
		f = ((float)GLWin.backbuffer.h / (float)conf.height) / ((float)GLWin.backbuffer.w / (float)conf.width);
		f -= (float)modf(f * GLWin.backbuffer.w, &temp) / (float)GLWin.backbuffer.w;
		v.x *= f;
		v.z *= f;
	}

	v  *= mult;
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
			DeleteFile(L"frame2.tga");
		}
#endif
	}
}

// Function populated tex0Info[2] array
inline void FrameObtainDispinfo(tex0Info* dispinfo)
{
	for (int i = 0; i < 2; ++i)
	{
		if (!Circuit_Enabled(i))
		{
			dispinfo[i].tw = 0;
			dispinfo[i].th = 0;
			continue;
		}

		GSRegDISPFB* pfb = Dispfb_Reg(i);
		GSRegDISPLAY* pd = Display_Reg(i);
		
		int magh = pd->MAGH + 1;
		int magv = pd->MAGV + 1;

		dispinfo[i].tbp0 =  pfb->FBP << 5;
		dispinfo[i].tbw = pfb->FBW << 6;
		dispinfo[i].tw = (pd->DW + 1) / magh;
		dispinfo[i].th = (pd->DH + 1) / magv;
		dispinfo[i].psm = pfb->PSM;

		// hack!!
		// 2 * dispinfo[i].tw / dispinfo[i].th <= 1, metal slug 4

		// Note: This is what causes the double image if interlace is off on the Final Fantasy X-2 opening.
		if (interlace_mode && 2 * dispinfo[i].tw / dispinfo[i].th <= 1 && !(conf.settings().interlace_2x))
		{
			dispinfo[i].th >>= 1;
		}
	}
}

extern bool s_bWriteDepth;

// Something should be done before Renderering the picture.
inline void RenderStartHelper()
{
	if (conf.mrtdepth && ZZshExistProgram(pvs[8]))
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

	FB::Unbind();   // switch to the backbuffer

	glViewport(0, 0, GLWin.backbuffer.w, GLWin.backbuffer.h);

	// if interlace, only clear every other vsync
	if (!interlace_mode)
	{
		glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	}

	ZZshSetVertexShader(pvsBitBlt.prog);

	glBindBuffer(GL_ARRAY_BUFFER, vboRect);
	SET_STREAM();
	GL_REPORT_ERRORD();

	if (conf.wireframe()) glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	DisableAllgl();

	GL_REPORT_ERRORD();

	if (interlace_mode) g_PrevBitwiseTexX = -1;  // reset since will be using
}

// Settings for interlace texture multiplied vector;
// The idea is: (x, y) -- position on screen, then interlaced texture get F = 1 or 0 depending
// on image y coords. So if we write valpha.z * F + valpha.w + 0.5, it would be switching odd
// and even strings at each frame.
// valpha.x and y are used for image blending.
inline float4 RenderGetForClip(int psm, CRTC_TYPE render_type)
{
	SetShaderCaller("RenderGetForClip");
	FRAGMENTSHADER* prog = curr_pps(render_type);
	float4 valpha;
	// first render the current render targets, then from ptexMem

	if (psm == PSMCT24)
	{
		valpha.x = 1;
		valpha.y = 0;
	}
	else
	{
		valpha.x = 0;
		valpha.y = 1;
	}

	if (interlace_mode)
	{
		if (gs.interlace == (conf.interlace & 1))
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

	ZZshSetParameter4fv(prog->prog, prog->sOneColor, valpha, "g_fOneColor");

	return valpha;
}

// Put interlaced texture in use for shader prog.
// Note: if the frame is interlaced, its th is halved, so we should multiply it by 2.
inline void RenderCreateInterlaceTex(int th, CRTC_TYPE render_type)
{
	FRAGMENTSHADER* prog;
	int interlacetex;
	
	if (!interlace_mode) return;
	
	prog = curr_pps(render_type);
	interlacetex = CreateInterlaceTex(2 * th);

	ZZshGLSetTextureParameter(prog->prog, prog->sInterlace, interlacetex, "Interlace");
}

// Do blending setup prior to second pass of half-frame drawing.
inline void RenderSetupBlending()
{
	// setup right blending
	glEnable(GL_BLEND);
	zgsBlendEquationSeparateEXT(GL_FUNC_ADD, GL_FUNC_ADD);

	if (PMODE->MMOD)
	{
		// Use the ALP register for alpha blending.
		glBlendColorEXT(PMODE->ALP*(1 / 255.0f), PMODE->ALP*(1 / 255.0f), PMODE->ALP*(1 / 255.0f), 0.5f);
		s_srcrgb = GL_CONSTANT_COLOR_EXT;
		s_dstrgb = GL_ONE_MINUS_CONSTANT_COLOR_EXT;
	}
	else
	{
		// Use the alpha value of circuit 1 for alpha blending.
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
	s_stencilmask = 1 << i;
	glStencilMask(s_stencilmask);
	GL_STENCILFUNC_SET();
}

// do stencil check for each found target i -- texturing stage
inline void RenderUpdateStencil(int i)
{
	if (!bUsingStencil) 
	{
		glClear(GL_STENCIL_BUFFER_BIT);
		bUsingStencil = true;
	}

	glEnable(GL_STENCIL_TEST);
	GL_STENCILFUNC(GL_NOTEQUAL, 3, 1 << i);
	glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
	glStencilMask(1 << i);
}

// CRTC24 could not be rendered
/*inline void RenderCRTC24helper(int psm)
{
	ZZLog::Debug_Log("ZZogl: CRTC24!!! I'm trying to show something.");
	SetShaderCaller("RenderCRTC24helper");
	// assume that data is already in ptexMem (do Resolve?)
	RenderGetForClip(psm, CRTC_RENDER_24);
	ZZshSetPixelShader(curr_ppsCRTC24()->prog);
	
	DrawTriangleArray();
}*/

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
inline float4 RenderSetTargetBitPos(int dh, int th, int movy)
{
	SetShaderCaller("RenderSetTargetBitPos");
	float4 v;
	// dest rect
	v.x = 1;
	v.y = dh / (float)th;
	v.z = 0;
	v.w = 1 - v.y;

	if (movy > 0) v.w -= movy / (float)th;

	AdjustTransToAspect(v);

	if (INTERLACE_COUNT())
	{
		// move down by 1 pixel
		v.w += 1.0f / (float)dh ;
	}

	ZZshSetParameter4fv(pvsBitBlt.prog, pvsBitBlt.sBitBltPos, v, "g_fBitBltPos");

	return v;
}

// Important stuff. We could use these coordinates to change viewport position on the frame.
// For example, use tw / X and tw / X magnify the viewport.
// Interlaced output is little out of VB, it could be seen as an evil blinking line on top
// and bottom, so we try to remove it.
inline float4 RenderSetTargetBitTex(float th, float tw, float dh, float dw)
{
	SetShaderCaller("RenderSetTargetBitTex");

	float4 v;
	v = float4(th, tw, dh, dw);

	// Incorrect Aspect ratio on interlaced frames

	if (INTERLACE_COUNT())
	{
		v.y -= 1.0f / conf.height;
		v.w += 1.0f / conf.height;
	}

	ZZshSetParameter4fv(pvsBitBlt.prog, pvsBitBlt.sBitBltTex, v, "g_fBitBltTex");

	return v;
}

// Translator for POSITION coordinates (-1.0:+1.0f at x axis, +1.0f:-1.0y at y) into target frame ones.
// We don't need x coordinate, because interlacing is y-axis only.
inline float4 RenderSetTargetBitTrans(int th)
{
	SetShaderCaller("RenderSetTargetBitTrans");
	float4 v = float4(float(th), -float(th), float(th), float(th));
	ZZshSetParameter4fv(pvsBitBlt.prog, pvsBitBlt.fBitBltTrans, v, "g_fBitBltTrans");
	return v;
}

// use g_fInvTexDims to store inverse texture dims
// Seems, that Targ shader does not use it
inline float4 RenderSetTargetInvTex(int tw, int th, CRTC_TYPE render_type)
{
	SetShaderCaller("RenderSetTargetInvTex");

	FRAGMENTSHADER* prog = curr_pps(render_type);
	float4 v = float4(0, 0, 0, 0);

	if (prog->sInvTexDims)
	{
		v.x = 1.0f / (float)tw;
		v.y = 1.0f / (float)th;
		v.z = (float)0.0;
		v.w = -0.5f / (float)th;
		ZZshSetParameter4fv(prog->prog, prog->sInvTexDims, v, "g_fInvTexDims");
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

inline void RenderCheckForMemory(tex0Info& texframe, list<CRenderTarget*>& listTargs, int circuit);

// First try to draw frame from targets. 
inline void RenderCheckForTargets(tex0Info& texframe, list<CRenderTarget*>& listTargs, int circuit)
{
	// get the start and end addresses of the buffer
	int bpp = RenderGetBpp(texframe.psm);
	GSRegDISPFB* pfb = Dispfb_Reg(circuit);

	int start, end;
	int tex_th = (interlace_mode) ? texframe.th * 2 : texframe.th;
	
	//ZZLog::WriteLn("Render checking for targets, circuit %d", circuit);
	GetRectMemAddressZero(start, end, texframe.psm, texframe.tw, tex_th, texframe.tbp0, texframe.tbw);

	// We need share list of targets between functions
	s_RTs.GetTargs(start, end, listTargs);

	for (list<CRenderTarget*>::iterator it = listTargs.begin(); it != listTargs.end();)
	{
		CRenderTarget* ptarg = *it;

		if (ptarg->fbw == texframe.tbw && !(ptarg->status&CRenderTarget::TS_NeedUpdate) && ((256 / bpp)*(texframe.tbp0 - ptarg->fbp)) % texframe.tbw == 0)
		{
			FRAGMENTSHADER* pps;
			int dby = pfb->DBY;
			int movy = 0;
			
			if (RenderLookForABetterTarget(ptarg->fbp, texframe.tbp0, listTargs, it))
			{
				continue;
			} 

			if (g_bSaveFinalFrame) SaveTexture("frame1.tga", GL_TEXTURE_RECTANGLE_NV, ptarg->ptex, RW(ptarg->fbw), RH(ptarg->fbh));

			// determine the rectangle to render
			int dh = RenderGetOffsets(&dby, &movy, texframe, ptarg, bpp);

			if (dh >= 64)
			{
				if (ptarg->fbh - dby < tex_th - movy && !bUsingStencil)
				{
					RenderUpdateStencil(circuit);
				}
				else if (ptarg->fbh - dby > 2 * ( tex_th - movy )) // I'm not sure this is needed any more.
				{
					// Sometimes calculated position onscreen is misaligned, ie in FFX-2 intro. In such case some part of image are out of
					// border's and we should move it manually.
					dby -= ((ptarg->fbh - dby) >> 2) -  ((tex_th + movy) >> 1);
				}

				SetShaderCaller("RenderCheckForTargets");

				// Texture
				float4 v = RenderSetTargetBitTex((float)RW(texframe.tw), (float)RH(dh), (float)RW(pfb->DBX), (float)RH(dby));
				
				// dest rect
				v = RenderSetTargetBitPos(dh, texframe.th, movy);
				v = RenderSetTargetBitTrans(ptarg->fbh);
				v = RenderSetTargetInvTex(texframe.tbw, ptarg->fbh, CRTC_RENDER_TARG); 	// FIXME. This is no use

				float4 valpha = RenderGetForClip(texframe.psm, CRTC_RENDER_TARG);
				pps = curr_ppsCRTCTarg();

				// inside vb[0]'s target area, so render that region only
				ZZshGLSetTextureParameter(pps->prog, pps->sFinal, ptarg->ptex, "CRTC target");
				RenderCreateInterlaceTex(texframe.th, CRTC_RENDER_TARG);

				ZZshSetPixelShader(pps->prog);

				DrawTriangleArray();

				if (abs(dh - (int)texframe.th) <= 1) 
				{
					return;
				}

				if (abs(dh - (int)ptarg->fbh) <= 1)
				{
					it = listTargs.erase(it);
					continue;
				}
			}
		}

		++it;
	}
	RenderCheckForMemory(texframe, listTargs, circuit);
}


// The same as the previous, but from memory.
// If you ever wondered why a picture from a minute ago suddenly flashes on the screen (say, in Mana Khemia),
// this is the function that does it.
inline void RenderCheckForMemory(tex0Info& texframe, list<CRenderTarget*>& listTargs, int circuit)
{
	float4 v;
	
	for (list<CRenderTarget*>::iterator it = listTargs.begin(); it != listTargs.end(); ++it)
	{
		(*it)->Resolve();
	}

	// context has to be 0
	if (interlace_mode >= 2) ZZLog::Error_Log("CRCR Check for memory shader fault.");

	//if (!bUsingStencil) RenderUpdateStencil(i);
		
	SetShaderCaller("RenderCheckForMemory");

	float w1, h1, w2, h2;
	if (g_bCRTCBilinear)
	{
		w1 = texframe.tw;
		h1 = texframe.th;
		w2 = -0.5f;
		h2 = -0.5f;
		SetTexVariablesInt(0, 2, texframe, false, curr_ppsCRTC(), 1);
	}
	else
	{
		w1 = 1;
		h1 = 1;
		w2 = -0.5f / (float)texframe.tw;
		h2 = -0.5f / (float)texframe.th;
		SetTexVariablesInt(0, 0, texframe, false, curr_ppsCRTC(), 1);
	}
	
	if (g_bSaveFinalFrame) SaveTex(&texframe, g_bSaveFinalFrame - 1 > 0);
	
	// Fixme: Why is this here?
	// We should probably call RenderSetTargetBitTex instead.
	v = RenderSetTargetBitTex(w1, h1, w2, h2);

	// finally render from the memory (note that the stencil buffer will keep previous regions)
	v = RenderSetTargetBitPos(1, 1, 0);
	v = RenderSetTargetBitTrans(texframe.th);
	v = RenderSetTargetInvTex(texframe.tw, texframe.th, CRTC_RENDER);
	float4 valpha = RenderGetForClip(texframe.psm, CRTC_RENDER);

#ifdef GLSL4_API
	// FIXME context
	int context = 0;
	ZZshGLSetTextureParameter(curr_ppsCRTC()->prog, curr_ppsCRTC()->sMemory[context], vb[0].pmemtarg->ptex->tex, "CRTC memory");
#else
	ZZshGLSetTextureParameter(curr_ppsCRTC()->prog, curr_ppsCRTC()->sMemory, vb[0].pmemtarg->ptex->tex, "CRTC memory");
#endif
	RenderCreateInterlaceTex(texframe.th, CRTC_RENDER_TARG);
	ZZshSetPixelShader(curr_ppsCRTC()->prog);
	
	DrawTriangleArray();
}

extern RasterFont* font_p;

void DrawText(const char* pstr, int left, int top, u32 color)
{
	FUNCLOG
	ZZshGLDisableProfile();

	float4 v;
	v.SetColor(color);
	glColor3f(v.z, v.y, v.x);

	font_p->printString(pstr, left * 2.0f / (float)GLWin.backbuffer.w - 1, 1 - top * 2.0f / (float)GLWin.backbuffer.h, 0);
	ZZshGLEnableProfile();
}

// Put FPS counter on screen (not in window title)
inline void DisplayFPS()
{
	char str[64];
	int left = 10, top = 15;
	sprintf(str, "%.1f fps", fFPS);

	DrawText(str, left + 1, top + 1, 0xff000000);
	DrawText(str, left, top, 0xffc0ffff);
}

// Snapshot helper
inline void MakeSnapshot()
{
	
	if (!g_bMakeSnapshot) return;
	
	char str[64];
	int left = 200, top = 15;
	sprintf(str, "ZeroGS %d.%d.%d - %.1f fps %s", zgsrevision, zgsbuild, zgsminor, fFPS, s_frameskipping ? " - frameskipping" : "");

	DrawText(str, left + 1, top + 1, 0xff000000);
	DrawText(str, left, top, 0xffc0ffff);

	if (SaveRenderTarget(strSnapshot != "" ? strSnapshot.c_str() : "temp.jpg", GLWin.backbuffer.w, -GLWin.backbuffer.h, 0))  //(conf.options.tga_snap)?0:1) ) {
	{
		char str[255];
		sprintf(str, "saved %s\n", strSnapshot.c_str());
		ZZAddMessage(str, 500);
	}
	
	g_bMakeSnapshot = false;
}

// call to destroy video resources
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
	g_vsprog = g_psprog = sZero;

	ZZGSStateReset();
	ZZDestroy();
	//clear_drawfn();
	if (ZZKick != NULL) delete ZZKick;
}

// Put new values on statistic variable
inline void CountStatistics()
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

	if (g_bDisplayFPS) DisplayFPS();

	// Swapping buffers, so we could use another window
	GLWin.SwapGLBuffers();

	// clear all targets
	if (conf.wireframe()) s_nWireframeCount = 1;

	if (g_bMakeSnapshot) MakeSnapshot();

	CaptureFrame();
	CountStatistics();

	if (s_nNewWidth >= 0 && s_nNewHeight >= 0) 
	{
		// If needed reset
		ZZReset();

		ChangeDeviceSize(s_nNewWidth, s_nNewHeight);
		s_nNewWidth = s_nNewHeight = -1;
	}

	maxmin = 608;
}

// Swich Framebuffers
inline void AfterRendererSwitchBackToTextures()
{
	FB::Bind();

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
		s_nCurResolveIndex = (s_nCurResolveIndex + 1) % ArraySize(s_nResolveCounts);

		int total = 0;

		for (int i = 0; i < ArraySize(s_nResolveCounts); ++i) total += s_nResolveCounts[i];

		if (total / ArraySize(s_nResolveCounts) > 3)
		{
			if (s_nLastResolveReset > (int)(fFPS * 8))
			{
				// reset
				ZZLog::Error_Log("Video memory reset.");
				s_nLastResolveReset = 0;
				memset(s_nResolveCounts, 0, sizeof(s_nResolveCounts));

				s_RTs.ResolveAll();
				return;
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
void RenderCRTC()
{
	tex0Info dispinfo[2];
	
	if (FrameSkippingHelper()) return;
	
	// If we are in frame mode and interlacing, and we haven't forced interlacing off, interlace_mode is 1.
	interlace_mode = SMODE2->INT && SMODE2->FFMD && (conf.interlace < 2);
	bUsingStencil = false;

	RenderStartHelper();

	FrameObtainDispinfo(dispinfo);
	
	// start from the last circuit
	for (int i = !PMODE->SLBG; i >= 0; --i)
	{
		if (!Circuit_Enabled(i)) continue;
		tex0Info& texframe = dispinfo[i];

		// I don't think this is neccessary, now that we make sure the ciruit we are working with is enabled.
		/*if (texframe.th <= 1) 
		{
			continue;
		}*/
		
		if (SMODE2->INT && SMODE2->FFMD) 
		{
			texframe.th >>= 1;
			
			// Final Fantasy X-2 issue here.
			/*if (conf.interlace == 2 && texframe.th >= 512) 
			{
				texframe.th >>= 1;
			}*/
		}
		
		if (i == 0) RenderSetupBlending();
		if (bUsingStencil) RenderSetupStencil(i);

		/*if (texframe.psm == 0x12) // Probably broken - 0x12 isn't a valid psm. 24 bit is 1.
		{
			RenderCRTC24helper(texframe.psm);
			continue;
		}*/

		// We shader targets between two functions, so declare it here;
		list<CRenderTarget*> listTargs;

		// if we could not draw image from target's, do it from memory
		RenderCheckForTargets(texframe, listTargs, i);
	}

	GL_REPORT_ERRORD();

	glDisable(GL_BLEND);

	AfterRendererUnimportantJob();
	AfterRendererSwitchBackToTextures();
	AfterRendererAutoresetTargets();
}
