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
 
#ifndef ZZOGLFLUSH_H_INCLUDED
#define ZZOGLFLUSH_H_INCLUDED

#ifndef ZEROGS_DEVBUILD

#define INC_GENVARS()
#define INC_TEXVARS()
#define INC_ALPHAVARS()
#define INC_RESOLVE()

#define g_bUpdateEffect 0
#define g_bSaveTex 0
#define g_bSaveResolved 0

#else // defined(ZEROGS_DEVBUILD)

#define INC_GENVARS() ++g_nGenVars
#define INC_TEXVARS() ++g_nTexVars
#define INC_ALPHAVARS() ++g_nAlphaVars
#define INC_RESOLVE() ++g_nResolve

extern bool g_bUpdateEffect;
extern bool g_bSaveTex;	// saves the current texture
extern bool g_bSaveResolved;
#endif // !defined(ZEROGS_DEVBUILD)

enum StencilBits
{
	STENCIL_ALPHABIT = 1,		// if set, dest alpha >= 0x80
	STENCIL_PIXELWRITE = 2,		// if set, pixel just written (reset after every Flush)
	STENCIL_FBA = 4,			// if set, just written pixel's alpha >= 0 (reset after every Flush)
	STENCIL_SPECIAL = 8		// if set, indicates that pixel passed its alpha test (reset after every Flush)
	//STENCIL_PBE = 16	
};
#define STENCIL_CLEAR	   (2|4|8|16)

enum ColorMask 
{
	COLORMASK_RED = 1,
	COLORMASK_GREEN = 2,
	COLORMASK_BLUE = 4,
	COLORMASK_ALPHA = 8
	
};
#define GL_COLORMASK(mask) glColorMask(!!((mask)&COLORMASK_RED), !!((mask)&COLORMASK_GREEN), !!((mask)&COLORMASK_BLUE), !!((mask)&COLORMASK_ALPHA))

// extern int g_nDepthBias;
extern float g_fBlockMult; // used for old cards, that do not support Alpha-32float textures. We store block data in u16 and use it.
extern u32 g_nCurVBOIndex;
extern u8* g_pbyGSClut;
extern int ppf;

extern bool s_bTexFlush;

extern vector<u32> s_vecTempTextures;		   // temporary textures, released at the end of every frame
extern GLuint g_vboBuffers[VB_NUMBUFFERS]; // VBOs for all drawing commands
extern CRangeManager s_RangeMngr; // manages overwritten memory				// zz

#if 0
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

extern g_flag_vars g_vars;
#endif

//#define bNeedAlphaColor g_vars._bNeedAlphaColor
//#define b2XAlphaTest g_vars._b2XAlphaTest
//#define bDestAlphaColor g_vars._bDestAlphaColor
//#define bAlphaClamping g_vars._bAlphaClamping

void FlushTransferRanges(const tex0Info* ptex);						//zz

// use to update the state
void SetTexVariables(int context, FRAGMENTSHADER* pfragment);			// zz
void SetTexInt(int context, FRAGMENTSHADER* pfragment, int settexint);		// zz
void SetAlphaVariables(const alphaInfo& ainfo);					// zzz
//void ResetAlphaVariables();

inline void SetAlphaTestInt(pixTest curtest);

inline void RenderAlphaTest(const VB& curvb, FRAGMENTSHADER* pfragment);
inline void RenderStencil(const VB& curvb, u32 dwUsingSpecialTesting);
inline void ProcessStencil(const VB& curvb);
inline void RenderFBA(const VB& curvb, FRAGMENTSHADER* pfragment);
inline void ProcessFBA(const VB& curvb, FRAGMENTSHADER* pfragment);			// zz

void SetContextTarget(int context);

void SetWriteDepth();
bool IsWriteDepth();
void SetDestAlphaTest();

#endif // ZZOGLFLUSH_H_INCLUDED
