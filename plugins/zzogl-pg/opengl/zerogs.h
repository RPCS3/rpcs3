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

#ifndef __ZEROGS__H
#define __ZEROGS__H

#ifdef _MSC_VER
#pragma warning(disable:4200) // nonstandard extension used : zero-sized array in struct/union
#endif

// ----------------------------- Includes
#include <list>
#include <vector>
#include <map>
#include <string>
#include <math.h>

#include "ZZGl.h"
#include "GS.h"
#include "CRC.h"
#include "targets.h"

// ------------------------ Variables -------------------------

//////////////////////////
// State parameters

#ifdef ZEROGS_DEVBUILD
extern char* EFFECT_NAME;
extern char* EFFECT_DIR;
extern u32 g_nGenVars, g_nTexVars, g_nAlphaVars, g_nResolve;
extern bool g_bSaveTrans, g_bUpdateEffect, g_bSaveTex, g_bSaveResolved;
#endif

extern int g_nPixelShaderVer;

extern bool s_bWriteDepth;

extern int nBackbufferWidth, nBackbufferHeight;

extern float fiTexWidth[2], fiTexHeight[2];	// current tex width and height
extern vector<GLuint> g_vboBuffers; // VBOs for all drawing commands
extern GLuint vboRect;
extern int g_nCurVBOIndex;

void ChangeWindowSize(int nNewWidth, int nNewHeight);
void SetChangeDeviceSize(int nNewWidth, int nNewHeight);
void ChangeDeviceSize(int nNewWidth, int nNewHeight);
void SetAA(int mode);
void SetCRC(int crc);

void ReloadEffects();

// Methods //
bool IsGLExt(const char* szTargetExtension);   ///< returns true if the the opengl extension is supported
inline bool Create_Window(int _width, int _height);
bool ZZCreate(int width, int height);
void ZZDestroy(bool bD3D);

void ZZReset(); // call to destroy video resources
void ZZGSStateReset();
void ZZGSReset();
void ZZGSSoftReset(u32 mask);

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
void SetFogColor(GIFRegFOGCOL* fog);
void SaveTex(tex0Info* ptex, int usevid);
char* NamedSaveTex(tex0Info* ptex, int usevid);

// switches the render target to the real target, flushes the current render targets and renders the real image
void RenderCRTC(int interlace);
void ResetRenderTarget(int index);

#endif
