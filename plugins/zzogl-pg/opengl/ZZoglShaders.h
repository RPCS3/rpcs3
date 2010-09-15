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

// -- Not very important things, but we keep it to enumerate shader
#define NUM_FILTERS 2 		// texture filtering
#define NUM_TYPES 5 		// types of texture read modes
#define NUM_TEXWRAPS 4 		// texture wrapping
#define NUM_SHADERS (NUM_FILTERS*NUM_TYPES*NUM_TEXWRAPS*32) // # shaders for a given ps

// Just bitmask for different type of shaders
#define SHADER_REDUCED 1	// equivalent to ps2.0
#define SHADER_ACCURATE 2   	// for older cards with less accurate math (ps2.x+)
// For output
const static char* g_pShaders[] = { "full", "reduced", "accurate", "accurate-reduced" };

#define NVIDIA_CG_API
// --------------------------- API abstraction level --------------------------------

#ifdef NVIDIA_CG_API				// Code for NVIDIA cg-toolkit API

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

#define SAFE_RELEASE_PROG(x) 	{ if( (x) != NULL ) { cgDestroyProgram(x); x = NULL; } }
inline bool ZZshActiveParameter(ZZshParameter param) {return (param !=NULL); }

#endif					// end NVIDIA cg-toolkit API

const static char* g_pPsTexWrap[] = { "-DREPEAT", "-DCLAMP", "-DREGION_REPEAT", NULL };
const static char* g_pTexTypes[] = { "32", "tex32", "clut32", "tex32to16", "tex16to8h" };

enum ZZshShaderType {ZZ_SH_ZERO, ZZ_SH_REGULAR, ZZ_SH_REGULAR_FOG, ZZ_SH_TEXTURE, ZZ_SH_TEXTURE_FOG, ZZ_SH_CRTC};
// We have "compatible" shaders, as RegularFogVS and RegularFogPS, if we don't need to worry about incompatible shaders.
// It's used only in GLSL mode. 

// ------------------------- Variables -------------------------------
extern int g_nPixelShaderVer;
extern ZZshShaderLink pvs[16], g_vsprog, g_psprog;
extern ZZshParameter g_vparamPosXY[2], g_fparamFogColor;

#define MAX_ACTIVE_UNIFORMS 600
#define MAX_ACTIVE_SHADERS 400

struct FRAGMENTSHADER
{
	FRAGMENTSHADER() : prog(sZero), Shader(0), sMemory(pZero), sFinal(pZero), sBitwiseANDX(pZero), sBitwiseANDY(pZero), sInterlace(pZero), sCLUT(pZero), sOneColor(pZero), sBitBltZ(pZero),
		fTexAlpha2(pZero), fTexOffset(pZero), fTexDims(pZero), fTexBlock(pZero), fClampExts(pZero), fTexWrapMode(pZero),
		fRealTexDims(pZero), fTestBlack(pZero), fPageOffset(pZero), fTexAlpha(pZero) {}
		
	ZZshShaderLink prog;						// it links to the FRAGMENTSHADER structure, for compatibility between GLSL and CG.
	ZZshShader Shader;							// GLSL store shaders not as ready programs, but as shader compiled objects. VS and PS should be linked together to
												// make a program.
	ZZshShaderType ShaderType;					// Not every PS and VS are used together, only compatible ones.

	ZZshParameter sMemory, sFinal, sBitwiseANDX, sBitwiseANDY, sInterlace, sCLUT;
	ZZshParameter sOneColor, sBitBltZ, sInvTexDims;
	ZZshParameter fTexAlpha2, fTexOffset, fTexDims, fTexBlock, fClampExts, fTexWrapMode, fRealTexDims, fTestBlack, fPageOffset, fTexAlpha;

	int ParametersStart, ParametersFinish;				// this is part of UniformsIndex array in which parameters of this shader asre stored. The last one is ParametersFinish-1

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
	VERTEXSHADER() : prog(sZero), Shader(0), sBitBltPos(pZero), sBitBltTex(pZero) {}
	
	ZZshShaderLink prog;
	ZZshShader Shader;
	ZZshShaderType ShaderType;

	ZZshParameter sBitBltPos, sBitBltTex, fBitBltTrans;		 // vertex shader constants

	int ParametersStart, ParametersFinish;
};

namespace ZeroGS {
	// Shaders variables
	extern Vector g_vdepth;	
	extern Vector vlogz;
	extern VERTEXSHADER pvsBitBlt;
	extern FRAGMENTSHADER ppsBitBlt[2], ppsBitBltDepth, ppsOne;					// ppsOne used to stop using shaders for draw
	extern FRAGMENTSHADER ppsBaseTexture, ppsConvert16to32, ppsConvert32to16;
	bool LoadEffects();
	bool LoadExtraEffects();
	FRAGMENTSHADER* LoadShadeEffect(int type, int texfilter, int fog, int testaem, int exactcolor, const clampInfo& clamp, int context, bool* pbFailed);

	// only sets a limited amount of state (for Update)
	void SetTexClamping(int context, FRAGMENTSHADER* pfragment);
	void SetTexVariablesInt(int context, int bilinear, const tex0Info& tex0, bool CheckVB, FRAGMENTSHADER* pfragment, int force);
}

// ------------------------- Variables -------------------------------

extern u8* s_lpShaderResources;
extern ZZshProfile cgvProf, cgfProf;
extern FRAGMENTSHADER ppsRegular[4], ppsTexture[NUM_SHADERS];
extern FRAGMENTSHADER ppsCRTC[2], ppsCRTC24[2], ppsCRTCTarg[2];

// ------------------------- Functions -------------------------------

#ifdef NVIDIA_CG_API
inline bool ZZshExistProgram(FRAGMENTSHADER* pf) {return (pf->prog != NULL); };			// We don't check ps != NULL, so be warned,
inline bool ZZshExistProgram(VERTEXSHADER* pf) {return (pf->prog != NULL); };
inline bool ZZshExistProgram(ZZshShaderLink prog) {return (prog != NULL); };
#endif

extern const char* ShaderCallerName;
extern const char* ShaderHandleName;

inline void SetShaderCaller(const char* Name) {
	ShaderCallerName = Name;
}

inline void SetHandleName(const char* Name) {
	ShaderHandleName = Name;
}
		
inline void ResetShaderCounters() {
//	g_vsprog = g_psprog = sZero;
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

inline int GET_SHADER_INDEX(int type, int texfilter, int texwrap, int fog, int writedepth, int testaem, int exactcolor, int context, int ps)
{
	return type + texfilter*NUM_TYPES + NUM_FILTERS*NUM_TYPES*texwrap + NUM_TEXWRAPS*NUM_FILTERS*NUM_TYPES*(fog+2*writedepth+4*testaem+8*exactcolor+16*context+32*ps);
}
	
struct SHADERHEADER
{
	unsigned int index, offset, size; // if highest bit of index is set, pixel shader
};

#endif
