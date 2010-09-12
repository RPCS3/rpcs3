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

#ifndef __ZEROGS_SHADERS_H__
#define __ZEROGS_SHADERS_H__

// I'll need to figure out a way to get rid of this dependency... --arcum42
//#include "GS.h"
#include <Cg/cg.h>
#include <Cg/cgGL.h>

#define ZZshProgram 		CGprogram
#define ZZshShader 		CGprogram
#define ZZshShaderLink		CGprogram
#define ZZshParameter 		CGparameter
#define ZZshContext		CGcontext
#define ZZshProfile		CGprofile
#define ZZshError		CGerror
#define pZero			0			// Zero parameter
#define sZero			0			// Zero program

#define NUM_FILTERS 2 		// texture filtering
#define NUM_TYPES 5 		// types of texture read modes
#define NUM_TEXWRAPS 4 		// texture wrapping

#define SHADER_REDUCED 1	// equivalent to ps2.0
#define SHADER_ACCURATE 2   	// for older cards with less accurate math (ps2.x+)

#define NUM_SHADERS (NUM_FILTERS*NUM_TYPES*NUM_TEXWRAPS*32) // # shaders for a given ps

const static char* g_pShaders[] = { "full", "reduced", "accurate", "accurate-reduced" };
const static char* g_pPsTexWrap[] = { "-DREPEAT", "-DCLAMP", "-DREGION_REPEAT", NULL };
const static char* g_pTexTypes[] = { "32", "tex32", "clut32", "tex32to16", "tex16to8h" };

#define TEXWRAP_REPEAT 0
#define TEXWRAP_CLAMP 1
#define TEXWRAP_REGION_REPEAT 2
#define TEXWRAP_REPEAT_CLAMP 3

inline int GET_SHADER_INDEX(int type, int texfilter, int texwrap, int fog, int writedepth, int testaem, int exactcolor, int context, int ps)
{
	return type + texfilter*NUM_TYPES + NUM_FILTERS*NUM_TYPES*texwrap + NUM_TEXWRAPS*NUM_FILTERS*NUM_TYPES*(fog+2*writedepth+4*testaem+8*exactcolor+16*context+32*ps);
}
	
struct SHADERHEADER
{
	unsigned int index, offset, size; // if highest bit of index is set, pixel shader
};

#define SH_WRITEDEPTH 0x2000 // depth is written
#define SH_CONTEXT1 0x1000 // context1 is used

#define SH_REGULARVS 0x8000
#define SH_TEXTUREVS 0x8001
#define SH_REGULARFOGVS 0x8002
#define SH_TEXTUREFOGVS 0x8003
#define SH_REGULARPS 0x8004
#define SH_REGULARFOGPS 0x8005
#define SH_BITBLTVS 0x8006
#define SH_BITBLTPS 0x8007
#define SH_BITBLTDEPTHPS 0x8009
#define SH_CRTCTARGPS 0x800a
#define SH_CRTCPS 0x800b
#define SH_CRTC24PS 0x800c
#define SH_ZEROPS 0x800e
#define SH_BASETEXTUREPS 0x800f
#define SH_BITBLTAAPS 0x8010
#define SH_CRTCTARGINTERPS 0x8012
#define SH_CRTCINTERPS 0x8013
#define SH_CRTC24INTERPS 0x8014
#define SH_BITBLTDEPTHMRTPS 0x8016
#define SH_CONVERT16TO32PS 0x8020
#define SH_CONVERT32TO16PS 0x8021
#define SH_CRTC_NEARESTPS 0x8022
#define SH_CRTCINTER_NEARESTPS 0x8023


struct FRAGMENTSHADER
{
	FRAGMENTSHADER() : prog(0), sMemory(0), sFinal(0), sBitwiseANDX(0), sBitwiseANDY(0), sInterlace(0), sCLUT(0), sOneColor(0), sBitBltZ(0),
		fTexAlpha2(0), fTexOffset(0), fTexDims(0), fTexBlock(0), fClampExts(0), fTexWrapMode(0),
		fRealTexDims(0), fTestBlack(0), fPageOffset(0), fTexAlpha(0) {}
	
	ZZshProgram prog;
	ZZshParameter sMemory, sFinal, sBitwiseANDX, sBitwiseANDY, sInterlace, sCLUT;
	ZZshParameter sOneColor, sBitBltZ, sInvTexDims;
	ZZshParameter fTexAlpha2, fTexOffset, fTexDims, fTexBlock, fClampExts, fTexWrapMode, fRealTexDims, fTestBlack, fPageOffset, fTexAlpha;

#ifdef _DEBUG
	string filename;
#endif

	void set_uniform_param(ZZshParameter &var, const char *name)
	{
		ZZshParameter p;
		p = cgGetNamedParameter(prog, name);

		if (p != NULL && cgIsParameterUsed(p, prog) == CG_TRUE) var = p;
	}

	bool set_texture(GLuint texobj, const char *name)
	{
		ZZshParameter p;

		p = cgGetNamedParameter(prog, name);

		if (p != NULL && cgIsParameterUsed(p, prog) == CG_TRUE)
		{
			cgGLSetTextureParameter(p, texobj);
			cgGLEnableTextureParameter(p);
			return true;
		}

		return false;
	}

	bool connect(ZZshParameter &tex, const char *name)
	{
		ZZshParameter p;

		p = cgGetNamedParameter(prog, name);

		if (p != NULL && cgIsParameterUsed(p, prog) == CG_TRUE)
		{
			cgConnectParameter(tex, p);
			return true;
		}

		return false;
	}

	bool set_texture(ZZshParameter &tex, const char *name)
	{
		ZZshParameter p;

		p = cgGetNamedParameter(prog, name);

		if (p != NULL && cgIsParameterUsed(p, prog) == CG_TRUE)
		{
			//cgGLEnableTextureParameter(p);
			tex = p;
			return true;
		}

		return false;
	}

	bool set_shader_const(Vector v, const char *name)
	{
		ZZshParameter p;

		p = cgGetNamedParameter(prog, name);

		if (p != NULL && cgIsParameterUsed(p, prog) == CG_TRUE)
		{
			cgGLSetParameter4fv(p, v);
			return true;
		}

		return false;
	}
};

struct VERTEXSHADER
{
	VERTEXSHADER() : prog(0), sBitBltPos(0), sBitBltTex(0) {}
	ZZshProgram prog;
	ZZshParameter sBitBltPos, sBitBltTex, fBitBltTrans;		 // vertex shader constants
};

// ------------------------- Variables -------------------------------

extern u8* s_lpShaderResources;
extern ZZshProfile cgvProf, cgfProf;
extern int g_nPixelShaderVer;
extern ZZshProgram pvs[16];
extern FRAGMENTSHADER ppsRegular[4], ppsTexture[NUM_SHADERS];
extern FRAGMENTSHADER ppsCRTC[2], ppsCRTC24[2], ppsCRTCTarg[2];
extern ZZshProgram g_vsprog, g_psprog;
extern ZZshParameter g_vparamPosXY[2], g_fparamFogColor;

// ------------------------- Functions -------------------------------
extern const char* ShaderCallerName;
extern const char* ShaderHandleName;

inline void SetShaderCaller(const char* Name) {
	ShaderCallerName = Name;
}

inline void SetHandleName(const char* Name) {
	ShaderHandleName = Name;
}

extern bool ZZshCheckProfilesSupport();
extern bool ZZshStartUsingShaders();
extern void ZZshGLDisableProfile();
extern void ZZshGLEnableProfile();
extern void ZZshSetParameter4fv(ZZshParameter param, const float* v, const char* name);
extern void ZZshGLSetTextureParameter(ZZshParameter param, GLuint texobj, const char* name);
extern void ZZshDefaultOneColor( FRAGMENTSHADER ptr );
extern void ZZshSetVertexShader(ZZshShader prog);
extern void ZZshSetPixelShader(ZZshShader prog);

#define SAFE_RELEASE_PROG(x) { if( (x) != NULL ) { cgDestroyProgram(x); x = NULL; } }

namespace ZeroGS {
	// Shaders variables
	extern Vector g_vdepth;	
	extern Vector vlogz;
	extern VERTEXSHADER pvsBitBlt;
	extern FRAGMENTSHADER ppsBitBlt[2], ppsBitBltDepth, ppsOne;
	extern FRAGMENTSHADER ppsBaseTexture, ppsConvert16to32, ppsConvert32to16;
	bool LoadEffects();
	bool LoadExtraEffects();
	FRAGMENTSHADER* LoadShadeEffect(int type, int texfilter, int fog, int testaem, int exactcolor, const clampInfo& clamp, int context, bool* pbFailed);

	// only sets a limited amount of state (for Update)
	void SetTexClamping(int context, FRAGMENTSHADER* pfragment);
	void SetTexVariablesInt(int context, int bilinear, const tex0Info& tex0, bool CheckVB, FRAGMENTSHADER* pfragment, int force);
}

#endif
