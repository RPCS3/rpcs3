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
#include "rasterfont.h" // simple font
#include "targets.h"

using namespace std;

//------------------------ Constants ----------------------

// Used in a logarithmic Z-test, as (1-o(1))/log(MAX_U32).
const float g_filog32 = 0.999f / (32.0f * logf(2.0f));

//------------------------ Inlines -------------------------

// Calculate maximum height for target
inline int get_maxheight(int fbp, int fbw, int psm)
{
	int ret;

	if (fbw == 0) return 0;

	ret = (((0x00100000 - 64 * fbp) / fbw) & ~0x1f);
	if (PSMT_ISHALF(psm)) ret *= 2;

	return ret;
}

// ------------------------ Variables -------------------------

// all textures have this width
extern int GPU_TEXWIDTH;
extern float g_fiGPU_TEXWIDTH;
#define MASKDIVISOR		0							// Used for decrement bitwise mask texture size if 1024 is too big
#define GPU_TEXMASKWIDTH	(1024 >> MASKDIVISOR)	// bitwise mask width for region repeat mode

extern u32 ptexBlocks;		// holds information on block tiling. It's texture number in OpenGL -- if 0 than such texture
extern u32 ptexConv16to32;	// does not exists. This textures should be created on start and released on finish.  
extern u32 ptexBilinearBlocks;
extern u32 ptexConv32to16;

// this is currently *not* used as a bool, in spite of its moniker --air
// Actually, the only thing written to it is 1 or 0, which makes the (g_bSaveFlushedFrame & 0x80000000) check rather bizzare.
//extern u32 g_bSaveFlushedFrame;	

//////////////////////////
// State parameters


#ifdef ZEROGS_DEVBUILD
extern char* EFFECT_NAME;
extern char* EFFECT_DIR;
extern u32 g_nGenVars, g_nTexVars, g_nAlphaVars, g_nResolve;
extern bool g_bSaveTrans, g_bUpdateEffect, g_bSaveTex, g_bSaveResolved;
#endif

extern u32 s_uFramebuffer;
extern int g_nPixelShaderVer;

extern bool s_bWriteDepth;

extern u32 ptexLogo;
extern int nLogoWidth, nLogoHeight;
extern int nBackbufferWidth, nBackbufferHeight;

typedef void (*DrawFn)();

// visible members
extern DrawFn drawfn[8];

extern float fiTexWidth[2], fiTexHeight[2];	// current tex width and height
extern vector<GLuint> g_vboBuffers; // VBOs for all drawing commands
extern GLuint vboRect;
extern int g_nCurVBOIndex;

void ZZAddMessage(const char* pstr, u32 ms = 5000);
void DrawText(const char* pstr, int left, int top, u32 color);
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

int ZZSave(s8* pbydata);
bool ZZLoad(s8* pbydata);

void SaveSnapshot(const char* filename);
bool SaveRenderTarget(const char* filename, int width, int height, int jpeg);
bool SaveTexture(const char* filename, u32 textarget, u32 tex, int width, int height);
bool SaveJPEG(const char* filename, int width, int height, const void* pdata, int quality);
bool SaveTGA(const char* filename, int width, int height, void* pdata);
void Stop_Avi();
void Delete_Avi_Capture();

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

	texClutWrite(ictx);
 };
 
// The size in bytes of x strings (of texture).
inline int MemorySize(int x) 
{
	return 4 * GPU_TEXWIDTH * x;
}

// Return the address in memory of data block for string x. 
inline u8* MemoryAddress(int x) 
{
	return g_pbyGSMemory + MemorySize(x);
}

template <u32 mult>
inline u8* _MemoryAddress(int x) 
{
	return g_pbyGSMemory + mult * x;
}
#endif
