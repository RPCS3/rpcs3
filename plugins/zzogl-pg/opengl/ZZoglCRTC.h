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

#ifndef ZZOGLCRTC_H_INCLUDED
#define ZZOGLCRTC_H_INCLUDED

#include <stdlib.h>

#include "zerogs.h"
#include "targets.h"

#define INTERLACE_COUNT (bInterlace && interlace == (conf.interlace))

#ifdef _WIN32
extern HDC		hDC;	   // Private GDI Device Context
extern HGLRC	hRC;	   // Permanent Rendering Context
#endif

extern int s_frameskipping;
extern float fFPS;
extern unsigned char zgsrevision, zgsbuild, zgsminor;

//extern u32 g_SaveFrameNum;
extern int s_nWriteDepthCount;
extern int s_nWireframeCount;
extern int s_nWriteDestAlphaTest;

extern int g_PrevBitwiseTexX, g_PrevBitwiseTexY; // textures stored in SAMP_BITWISEANDX and SAMP_BITWISEANDY


extern bool s_bDestAlphaTest;
extern int s_ClutResolve;
extern int s_nLastResolveReset;
extern int g_nDepthUpdateCount;
extern int s_nResolveCounts[30]; // resolve counts for last 30 frames
static int s_nCurResolveIndex = 0;
extern int g_nDepthUsed; // ffx2 pal movies

//------------------ Namespace

extern u32 s_ptexInterlace;		 // holds interlace fields

namespace ZeroGS
{
extern int s_nNewWidth, s_nNewHeight;

extern CRangeManager s_RangeMngr; // manages overwritten memory
extern void FlushTransferRanges(const tex0Info* ptex);
extern void ProcessMessages();
void AdjustTransToAspect(Vector& v);

// Interlace texture is lazy 1*(height) array of 1 and 0.
// If its height (named s_nInterlaceTexWidth here) is hanging we must redo
// the texture.
// FIXME: If this function were spammed too often, we could use
// width < s_nInterlaceTexWidth as correct for old texture
static int s_nInterlaceTexWidth = 0;				// width of texture

inline u32 CreateInterlaceTex(int width)
{
	if (width == s_nInterlaceTexWidth && s_ptexInterlace != 0) return s_ptexInterlace;

	SAFE_RELEASE_TEX(s_ptexInterlace);

	s_nInterlaceTexWidth = width;

	vector<u32> data(width);

	for (int i = 0; i < width; ++i)
	{
		data[i] = (i & 1) ? 0xffffffff : 0;
	}

	glGenTextures(1, &s_ptexInterlace);
	glBindTexture(GL_TEXTURE_RECTANGLE_NV, s_ptexInterlace);
	TextureRect(4, width, 1, GL_RGBA, GL_UNSIGNED_BYTE, &data[0]);
	setRectFilters(GL_NEAREST);
	GL_REPORT_ERRORD();

	return s_ptexInterlace;
}
}

#endif // ZZOGLCRTC_H_INCLUDED
