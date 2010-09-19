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

enum ZZshShaderType {ZZ_SH_ZERO, ZZ_SH_REGULAR, ZZ_SH_REGULAR_FOG, ZZ_SH_TEXTURE, ZZ_SH_TEXTURE_FOG, ZZ_SH_CRTC};
// We have "compatible" shaders, as RegularFogVS and RegularFogPS. if don't need to wory about incompatible shaders
// It used only in GLSL mode. 

// ------------------------- Variables -------------------------------

extern int 		g_nPixelShaderVer;
extern ZZshShaderLink 	pvs[16], g_vsprog, g_psprog;
extern ZZshParameter 	g_vparamPosXY[2], g_fparamFogColor;

#define MAX_ACTIVE_UNIFORMS 600
#define MAX_ACTIVE_SHADERS 400

struct FRAGMENTSHADER
{
	FRAGMENTSHADER() : prog(sZero), Shader(0), sMemory(pZero), sFinal(pZero), sBitwiseANDX(pZero), sBitwiseANDY(pZero), sInterlace(pZero), sCLUT(pZero), sOneColor(pZero), sBitBltZ(pZero),
		fTexAlpha2(pZero), fTexOffset(pZero), fTexDims(pZero), fTexBlock(pZero), fClampExts(pZero), fTexWrapMode(pZero),
		fRealTexDims(pZero), fTestBlack(pZero), fPageOffset(pZero), fTexAlpha(pZero)  {}
	
	ZZshShaderLink prog;						// it link to FRAGMENTSHADER structure, for compability between GLSL and CG
	ZZshShader Shader;						// GLSL store shader's not as ready programs, but as shaders compilated object. VS and PS should be linked together to
									// made a program.
	ZZshShaderType ShaderType;					// Not every PS and VS are used together, only compatible ones.

	ZZshParameter sMemory, sFinal, sBitwiseANDX, sBitwiseANDY, sInterlace, sCLUT;
	ZZshParameter sOneColor, sBitBltZ, sInvTexDims;
	ZZshParameter fTexAlpha2, fTexOffset, fTexDims, fTexBlock, fClampExts, fTexWrapMode, fRealTexDims, fTestBlack, fPageOffset, fTexAlpha;

	int ParametersStart, ParametersFinish;				// this is part of UniformsIndex array in which parameters of this shader stored. Last one is ParametersFinish-1

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

	bool set_shader_const(float4 v, const char *name)
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
	extern float4 g_vdepth;	
	extern float4 vlogz;
	extern VERTEXSHADER pvsBitBlt;
	extern FRAGMENTSHADER ppsBitBlt[2], ppsBitBltDepth, ppsOne;					// ppsOne used to stop using shaders for draw
	extern FRAGMENTSHADER ppsBaseTexture, ppsConvert16to32, ppsConvert32to16;

	extern FRAGMENTSHADER ppsRegular[4], ppsTexture[NUM_SHADERS];
	extern FRAGMENTSHADER ppsCRTC[2], ppsCRTC24[2], ppsCRTCTarg[2];
}

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
extern bool ZZshCreateOpenShadersFile();
extern void ZZshGLDisableProfile();
extern void ZZshGLEnableProfile();
extern void ZZshSetParameter4fv(ZZshShaderLink prog, ZZshParameter param, const float* v, const char* name);
extern void ZZshSetParameter4fv(ZZshParameter param, const float* v, const char* name);
extern void ZZshSetParameter4fvWithRetry(ZZshParameter* param, ZZshShaderLink prog, const float* v, const char* name);
extern void ZZshGLSetTextureParameter(ZZshShaderLink prog, ZZshParameter param, GLuint texobj, const char* name);
extern void ZZshGLSetTextureParameter(ZZshParameter param, GLuint texobj, const char* name);
extern void ZZshDefaultOneColor( FRAGMENTSHADER ptr );
extern void ZZshSetVertexShader(ZZshShaderLink prog);
extern void ZZshSetPixelShader(ZZshShaderLink prog);
extern bool ZZshLoadExtraEffects();

extern FRAGMENTSHADER* ZZshLoadShadeEffect(int type, int texfilter, int fog, int testaem, int exactcolor, const clampInfo& clamp, int context, bool* pbFailed);

namespace ZeroGS {
	// only sets a limited amount of state (for Update)
	void SetTexVariablesInt(int context, int bilinear, const tex0Info& tex0, bool CheckVB, FRAGMENTSHADER* pfragment, int force);
}
#endif
