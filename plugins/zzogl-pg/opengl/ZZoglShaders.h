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

#include "ZZoglMath.h"
#include "GS.h"

// By default enable nvidia cg api
#if !defined(GLSL_API) && !defined(NVIDIA_CG_API)
#define NVIDIA_CG_API
#endif
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

#ifdef GLSL_API

enum ZZshPARAMTYPE {
	ZZ_UNDEFINED,
	ZZ_TEXTURE_2D,
	ZZ_TEXTURE_RECT,
	ZZ_TEXTURE_3D,
	ZZ_FLOAT4,
};

typedef struct {
	const char* 	ShName;		// Name of uniform
	ZZshPARAMTYPE	type;		// Choose between parameter type

	float 		fvalue[4];
	GLuint		sampler;	// Number of texture unit in array 
	GLint		texid;		// Number of texture - texid. 

	bool		Constant;	// Uniform could be constants, does not change at program flow
	bool 		Settled;	// Check if Uniform value was set.
} ZZshParamInfo;

typedef struct {
	void*	 	link;
	bool		isFragment;
} ZZshShaderLink; 

#define ZZshProgram 		GLuint
#define ZZshShader 		GLuint
#define ZZshParameter 		GLint
#define ZZshContext		int
#define ZZshProfile		int
#define ZZshError		int
#define ZZshIndex		GLuint

const ZZshParamInfo  qZero = {ShName:"", type:ZZ_UNDEFINED, fvalue:{0}, sampler: -1, texid: 0, Constant: false, Settled: false};

#define pZero			0

const ZZshShaderLink sZero = {link: NULL, isFragment: false};

inline bool ZZshActiveParameter(ZZshParameter param) {return (param > -1); }
#define SAFE_RELEASE_PROG(x) 	{ /*don't know what to do*/ }

// ---------------------------

#endif




//const static char* g_pPsTexWrap[] = { "-DREPEAT", "-DCLAMP", "-DREGION_REPEAT", NULL };

enum ZZshShaderType {ZZ_SH_ZERO, ZZ_SH_REGULAR, ZZ_SH_REGULAR_FOG, ZZ_SH_TEXTURE, ZZ_SH_TEXTURE_FOG, ZZ_SH_CRTC, ZZ_SH_NONE};
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

#ifdef NVIDIA_CG_API
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
#endif
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

	extern float4 g_vdepth;	
	extern float4 vlogz;
	extern VERTEXSHADER pvsBitBlt;
	extern FRAGMENTSHADER ppsBitBlt[2], ppsBitBltDepth, ppsOne;					// ppsOne used to stop using shaders for draw
	extern FRAGMENTSHADER ppsBaseTexture, ppsConvert16to32, ppsConvert32to16;

	extern FRAGMENTSHADER ppsRegular[4], ppsTexture[NUM_SHADERS];
	extern FRAGMENTSHADER ppsCRTC[2], /*ppsCRTC24[2],*/ ppsCRTCTarg[2];

	extern int interlace_mode;

	enum CRTC_TYPE
	{
		CRTC_RENDER,
		//CRTC_RENDER_24,
		CRTC_RENDER_TARG
	};

	static __forceinline FRAGMENTSHADER* curr_ppsCRTC() { return &ppsCRTC[interlace_mode]; }
	//static __forceinline FRAGMENTSHADER* curr_ppsCRTC24() { return &ppsCRTC24[interlace_mode]; }
	static __forceinline FRAGMENTSHADER* curr_ppsCRTCTarg() { return &ppsCRTCTarg[interlace_mode]; }
	
	static __forceinline FRAGMENTSHADER* curr_pps(CRTC_TYPE render_type) 
	{
		switch (render_type)
		{
			case CRTC_RENDER: return curr_ppsCRTC();
			//case CRTC_RENDER_24: return curr_ppsCRTC24();
			case CRTC_RENDER_TARG: return curr_ppsCRTCTarg();
			default: return NULL;
		}
		
	}
// ------------------------- Functions -------------------------------

#ifdef NVIDIA_CG_API
inline bool ZZshExistProgram(FRAGMENTSHADER* pf) {return (pf->prog != NULL); };			// We don't check ps != NULL, so be warned,
inline bool ZZshExistProgram(VERTEXSHADER* pf) {return (pf->prog != NULL); };
inline bool ZZshExistProgram(ZZshShaderLink prog) {return (prog != NULL); };
#endif
#ifdef GLSL_API
inline bool ZZshExistProgram(FRAGMENTSHADER* pf) {return (pf->Shader != 0); };
inline bool ZZshExistProgram(VERTEXSHADER* pf) {return (pf->Shader != 0); };
inline bool ZZshExistProgram(ZZshShaderLink prog) {return (prog.link != NULL); }		// This is used for pvs mainly. No NULL means that we do LOAD_VS
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


/////////////////////////////////////////////////////////////////
// Improvement:
//  * store the location of uniform. Avoid to call glGetUniformLocation a lots of time
//  * Use separate shader build pipeline: current code emulate this behavior but with the recent opengl4
//	  it would be much more easier to code it.
/////////////////////////////////////////////////////////////////

// GLSL: Stub
extern bool ZZshCheckProfilesSupport();

// Try various Shader to choose supported configuration
// g_nPixelShaderVer -> SHADER_ACCURATE and SHADER_REDUCED
// Honestly we can probably stop supporting those cards.
extern bool ZZshStartUsingShaders();

// Open the shader file into an source array
extern bool ZZshCreateOpenShadersFile();

// Those 2 functions are used to stop/start shader. The idea is to draw the HUD text.
// Enable is not implemented and it is likely to stop everythings
extern void ZZshGLDisableProfile();
extern void ZZshGLEnableProfile();

// Set the Uniform parameter in host (NOT GL)
// Param seem to be an absolute index inside a table of uniform
extern void ZZshSetParameter4fv(ZZshShaderLink prog, ZZshParameter param, const float* v, const char* name);
extern void ZZshSetParameter4fv(ZZshParameter param, const float* v, const char* name);
extern void ZZshSetParameter4fvWithRetry(ZZshParameter* param, ZZshShaderLink prog, const float* v, const char* name);

// Set the Texture parameter in host (NOT GL)
extern void ZZshGLSetTextureParameter(ZZshShaderLink prog, ZZshParameter param, GLuint texobj, const char* name);
extern void ZZshGLSetTextureParameter(ZZshParameter param, GLuint texobj, const char* name);

// Set a default value for 1 uniform in host (NOT GL)
extern void ZZshDefaultOneColor( FRAGMENTSHADER ptr );

// Link then run with the new Vertex/Fragment Shader
extern void ZZshSetVertexShader(ZZshShaderLink prog);
extern void ZZshSetPixelShader(ZZshShaderLink prog);

// Compile standalone Fragment/Vertex shader program
// Note It also init all the Uniform parameter in host (NOT GL)
extern bool ZZshLoadExtraEffects();

// Clean some stuff on exit
extern void ZZshExitCleaning();

extern FRAGMENTSHADER* ZZshLoadShadeEffect(int type, int texfilter, int fog, int testaem, int exactcolor, const clampInfo& clamp, int context, bool* pbFailed);

// only sets a limited amount of state (for Update)
void SetTexVariablesInt(int context, int bilinear, const tex0Info& tex0, bool CheckVB, FRAGMENTSHADER* pfragment, int force);

extern u32 ptexBlocks;		// holds information on block tiling. It's texture number in OpenGL -- if 0 than such texture
extern u32 ptexConv16to32;	// does not exists. This textures should be created on start and released on finish.
extern u32 ptexBilinearBlocks;
extern u32 ptexConv32to16;


#endif
