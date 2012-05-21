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

#include <math.h>
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

#ifdef GLSL4_API
#include "GSUniformBufferOGL.h"
#include "GSVertexArrayOGL.h"
#endif

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

extern float4 g_vdepth;
extern float4 vlogz;


#ifdef GLSL4_API
enum {
	ZZSH_CTX_0   = 0,
	ZZSH_CTX_1   = 1,
	ZZSH_CTX_ALL = 2
};

// Note A nice template could be better
// Warning order is important for buffer (see GLSL)
// Note must be keep POD (so you can map it to GLSL)
struct GlobalUniform {
	union {
		struct {
			// VS
			float g_fPosXY[4]; // dual context
			// PS
			float g_fFogColor[4];
		};
		float linear[2*4];
	};
	void SettleFloat(uint indice, const float* v) {
		assert(indice + 3 < 2*4);
		linear[indice+0] = v[0];
		linear[indice+1] = v[1];
		linear[indice+2] = v[2];
		linear[indice+3] = v[3];
	}
};
struct ConstantUniform {
	union {
		struct {
			// Both VS/PS
			float g_fBilinear[4];
			float g_fZbias[4];
			float g_fc0[4];
			float g_fMult[4];
			// VS
			float g_fZ[4];
			float g_fZMin[4];
			float g_fZNorm[4];
			// PS
			float g_fExactColor[4];
		};
		float linear[8*4];
	};
	void SettleFloat(uint indice, const float* v) {
		assert(indice + 3 < 8*4);
		linear[indice+0] = v[0];
		linear[indice+1] = v[1];
		linear[indice+2] = v[2];
		linear[indice+3] = v[3];
	}
};
struct FragmentUniform {
	union {
		struct {
			float g_fTexAlpha2[4]; // dual context
			float g_fTexOffset[4]; // dual context
			float g_fTexDims[4]; // dual context
			float g_fTexBlock[4]; // dual context
			float g_fClampExts[4]; // dual context
			float g_fTexWrapMode[4]; // dual context
			float g_fRealTexDims[4]; // dual context
			float g_fTestBlack[4]; // dual context
			float g_fPageOffset[4]; // dual context
			float g_fTexAlpha[4]; // dual context
			float g_fInvTexDims[4];
			float g_fBitBltZ[4];
			float g_fOneColor[4];
		};
		float linear[13*4];
	};
	void SettleFloat(uint indice, const float* v) {
		assert(indice + 3 < 13*4);
		linear[indice+0] = v[0];
		linear[indice+1] = v[1];
		linear[indice+2] = v[2];
		linear[indice+3] = v[3];
	}
};
struct VertexUniform {
	union {
		struct {
			float g_fBitBltPos[4];
			float g_fBitBltTex[4];
			float g_fBitBltTrans[4];
		};
		float linear[3*4];
	};
	void SettleFloat(uint indice, const float* v) {
		assert(indice + 3 <  3*4);
		linear[indice+0] = v[0];
		linear[indice+1] = v[1];
		linear[indice+2] = v[2];
		linear[indice+3] = v[3];
	}
};
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

#ifndef GLSL4_API
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
#else
const GLenum g_texture_target[10] = {GL_TEXTURE_RECTANGLE, GL_TEXTURE_RECTANGLE, GL_TEXTURE_2D, GL_TEXTURE_2D, GL_TEXTURE_2D, GL_TEXTURE_3D, GL_TEXTURE_RECTANGLE, GL_TEXTURE_RECTANGLE, GL_TEXTURE_RECTANGLE, GL_TEXTURE_2D};

struct SamplerParam {
	int		unit;
	GLuint	texid;
	GLenum target;

	SamplerParam() : unit(-1), texid(0), target(0) {}

	void set_unit(int new_unit) {
		assert(new_unit < 10);
		unit = new_unit;
		target = g_texture_target[new_unit];
	}

	void enable_texture() {
		assert(unit >= 0);
		assert(unit < 10);
		if (texid) {
			glActiveTexture(GL_TEXTURE0 + unit);
			glBindTexture(target, texid);
		}
	}

	void set_texture(GLuint new_texid) {
		texid = new_texid;
	}

	void release_texture() {
		texid = 0;
	}
};

struct FRAGMENTSHADER
{
	FRAGMENTSHADER() : prog(sZero) 
					  , program(0)
					  , context(0)
					  , sMemory(0) // dual context need 2 slots
					  , sFinal(2)
					  , sBitwiseANDX(3)
					  , sBitwiseANDY(4)
					  , sInterlace(5)
					  , sCLUT(6)
	{
		// Uniform
		sOneColor    = (ZZshParameter)offsetof(struct FragmentUniform, g_fOneColor) /4;
		sBitBltZ     = (ZZshParameter)offsetof(struct FragmentUniform, g_fBitBltZ) /4;
		sInvTexDims  = (ZZshParameter)offsetof(struct FragmentUniform, g_fInvTexDims) /4;
		fTexAlpha    = (ZZshParameter)offsetof(struct FragmentUniform, g_fTexAlpha) /4;
		fTexAlpha2   = (ZZshParameter)offsetof(struct FragmentUniform, g_fTexAlpha2) /4;
		fTexOffset   = (ZZshParameter)offsetof(struct FragmentUniform, g_fTexOffset) /4;
		fTexDims     = (ZZshParameter)offsetof(struct FragmentUniform, g_fTexDims) /4;
		fTexBlock    = (ZZshParameter)offsetof(struct FragmentUniform, g_fTexBlock) /4;
		fClampExts   = (ZZshParameter)offsetof(struct FragmentUniform, g_fClampExts) /4; 	// FIXME: There is a bug, that lead FFX-1 to incorrect CLAMP if this uniform have context.
		fTexWrapMode = (ZZshParameter)offsetof(struct FragmentUniform, g_fTexWrapMode) /4;
		fRealTexDims = (ZZshParameter)offsetof(struct FragmentUniform, g_fRealTexDims) /4;
		fTestBlack   = (ZZshParameter)offsetof(struct FragmentUniform, g_fTestBlack) /4;
		fPageOffset  = (ZZshParameter)offsetof(struct FragmentUniform, g_fPageOffset) /4;

		//sFinal               = 2;
		//sBitwiseANDX         = 3;
		//sBitwiseANDY         = 4;
		//sInterlace           = 5;
		//sCLUT                = 6;
		samplers[sMemory].set_unit(0);
		samplers[sMemory+1].set_unit(0); // Dual context. Use same unit
		samplers[sFinal].set_unit(1);
		samplers[sBitwiseANDX].set_unit(6);
		samplers[sBitwiseANDY].set_unit(7);
		samplers[sInterlace].set_unit(8);
		samplers[sCLUT].set_unit(9);
	}

	ZZshShaderLink prog;						// it link to FRAGMENTSHADER structure, for compability between GLSL and CG
	ZZshProgram program;
	uint context;

	FragmentUniform uniform_buffer[ZZSH_CTX_ALL];

	// sampler
	const ZZshParameter sMemory;
	const ZZshParameter sFinal, sBitwiseANDX, sBitwiseANDY, sInterlace, sCLUT;
	SamplerParam samplers[7];

	// uniform
	ZZshParameter sOneColor, sBitBltZ, sInvTexDims;
	ZZshParameter fTexAlpha2, fTexOffset, fTexDims, fTexBlock, fClampExts, fTexWrapMode, fRealTexDims, fTestBlack, fPageOffset, fTexAlpha;

#ifdef _DEBUG
	string filename;
#endif

	void ZZshSetParameter4fv(ZZshParameter param, const float* v) {
		if (IsDualContext(param))
			uniform_buffer[context].SettleFloat((int) param, v);
		else
			for ( int i = 0; i < ZZSH_CTX_ALL ; i++)
				uniform_buffer[i].SettleFloat((int) param, v);
	}

	bool IsDualContext(ZZshParameter param) {
		if (param == sInvTexDims || param == sBitBltZ || param == sOneColor)
			return false;
		else
			return true;
	}

	void enable_texture() {
		samplers[sMemory+context].enable_texture(); // sMemory is dual context
		for (int i = 2; i < 7; i++)
			samplers[i].enable_texture();
	}

	void set_texture(ZZshParameter param, GLuint texid) {
		if (param == sMemory) // sMemory is dual context
			samplers[sMemory+context].set_texture(texid);
		else
			samplers[param].set_texture(texid);
	}

	void release_prog() {
		if(program) {
			glDeleteProgram(program);
			program = 0;
		}
		for (uint i = 0; i < 7 ; i++)
			samplers[i].release_texture();
	}
};
#endif

#ifdef GLSL4_API
struct COMMONSHADER
{
	COMMONSHADER() : context(0)
					 , sBlocks(0)
					 , sBilinearBlocks(1)
					 , sConv16to32(2)
				     , sConv32to16(3)
	{
		// sBlocks          = 0;
		// sBilinearBlocks  = 1;
		// sConv16to32      = 2;
		// sConv32to16      = 3;
		samplers[sBlocks].set_unit(2);
		samplers[sBilinearBlocks].set_unit(3);
		samplers[sConv16to32].set_unit(4);
		samplers[sConv32to16].set_unit(5);


		g_fparamFogColor = (ZZshParameter)offsetof(struct GlobalUniform, g_fFogColor) /4;
		g_vparamPosXY    = (ZZshParameter)offsetof(struct GlobalUniform, g_fPosXY) /4;

		g_fBilinear      = (ZZshParameter)offsetof(struct ConstantUniform, g_fBilinear) /4;
		g_fZBias         = (ZZshParameter)offsetof(struct ConstantUniform, g_fZbias) /4;
		g_fc0            = (ZZshParameter)offsetof(struct ConstantUniform, g_fc0) /4;
		g_fMult          = (ZZshParameter)offsetof(struct ConstantUniform, g_fMult) /4;
		g_fZ             = (ZZshParameter)offsetof(struct ConstantUniform, g_fZ) /4;
		g_fZMin          = (ZZshParameter)offsetof(struct ConstantUniform, g_fZMin) /4;
		g_fZNorm         = (ZZshParameter)offsetof(struct ConstantUniform, g_fZNorm) /4;
		g_fExactColor    = (ZZshParameter)offsetof(struct ConstantUniform, g_fExactColor) /4;

		// Setup the constant buffer

		// Set Z-test, log or no log;
		if (conf.settings().no_logz) {
			g_vdepth = float4( 255.0 /256.0f,  255.0/65536.0f, 255.0f/(65535.0f*256.0f), 1.0f/(65536.0f*65536.0f));
			vlogz = float4( 1.0f, 0.0f, 0.0f, 0.0f);
		}
		else {
			g_vdepth = float4( 256.0f*65536.0f, 65536.0f, 256.0f, 65536.0f*65536.0f);
			vlogz = float4( 0.0f, 1.0f, 0.0f, 0.0f);
		}
		uniform_buffer_constant.SettleFloat(g_fZ, g_vdepth );
		uniform_buffer_constant.SettleFloat(g_fZMin, vlogz );

		const float g_filog32 = 0.999f / (32.0f * logf(2.0f));
		float4 vnorm = float4(g_filog32, 0, 0,0);
		uniform_buffer_constant.SettleFloat(g_fZNorm, vnorm);

		uniform_buffer_constant.SettleFloat(g_fBilinear, float4(-0.2f, -0.65f, 0.9f, 1.0f / 32767.0f ) );
		uniform_buffer_constant.SettleFloat(g_fZBias, float4(1.0f/256.0f, 1.0004f, 1, 0.5f) );
		uniform_buffer_constant.SettleFloat(g_fc0, float4(0,1, 0.001f, 0.5f) );

		uniform_buffer_constant.SettleFloat(g_fExactColor, float4(0.5f, (conf.settings().exact_color)?0.9f/256.0f:0.5f/256.0f, 0,1/255.0f) );
		uniform_buffer_constant.SettleFloat(g_fMult, float4(1/1024.0f, 0.2f/1024.0f, 1/128.0f, 1/512.0f));
	}

	ZZshParameter g_fparamFogColor, g_vparamPosXY;
	ZZshParameter g_fBilinear, g_fZBias, g_fc0, g_fMult, g_fZ, g_fZMin, g_fZNorm, g_fExactColor;
	uint context;

	GlobalUniform	uniform_buffer[ZZSH_CTX_ALL];
	ConstantUniform uniform_buffer_constant;

	// Sampler
	const ZZshParameter sBlocks, sBilinearBlocks, sConv16to32, sConv32to16;
	SamplerParam samplers[4];

	void ZZshSetParameter4fv(ZZshParameter param, const float* v) {
		if (IsDualContext(param))
			uniform_buffer[context].SettleFloat((int) param, v);
		else
			for ( int i = 0; i < ZZSH_CTX_ALL ; i++)
				uniform_buffer[i].SettleFloat((int) param, v);
	}

	bool IsDualContext(ZZshParameter param) {
		if (param == g_vparamPosXY) return true;
		else return false;
	}

	void set_texture(ZZshParameter param, GLuint texid) {
		samplers[param].set_texture(texid);
	}

	void enable_texture() {
		for (int i = 0; i < 4; i++)
			samplers[i].enable_texture();
	}
};
#endif

#ifndef GLSL4_API
struct VERTEXSHADER
{
	VERTEXSHADER() : prog(sZero), Shader(0), sBitBltPos(pZero), sBitBltTex(pZero) {}
	
	ZZshShaderLink prog;
	ZZshShader Shader;
	ZZshShaderType ShaderType;

	ZZshParameter sBitBltPos, sBitBltTex, fBitBltTrans;		 // vertex shader constants

	int ParametersStart, ParametersFinish;
};
#else
struct VERTEXSHADER
{

	VERTEXSHADER() : prog(sZero), program(0), context(0)
	{
		sBitBltPos = (ZZshParameter)offsetof(struct VertexUniform, g_fBitBltPos) /4;
		sBitBltTex = (ZZshParameter)offsetof(struct VertexUniform, g_fBitBltTex) /4;
		fBitBltTrans = (ZZshParameter)offsetof(struct VertexUniform, g_fBitBltTrans) /4;

		// Default value not sure it is needed
		uniform_buffer[0].SettleFloat(fBitBltTrans, float4(0.5f, -0.5f, 0.5, 0.5 + 0.4/416.0f ) );
		uniform_buffer[1].SettleFloat(fBitBltTrans, float4(0.5f, -0.5f, 0.5, 0.5 + 0.4/416.0f ) );
	}

	VertexUniform uniform_buffer[ZZSH_CTX_ALL];

	ZZshShaderLink prog;
	ZZshProgram program;
	uint	context;

	ZZshParameter sBitBltPos, sBitBltTex, fBitBltTrans;		 // vertex shader constants

	void ZZshSetParameter4fv(ZZshParameter param, const float* v) {
		if (IsDualContext(param))
			uniform_buffer[context].SettleFloat((int) param, v);
		else
			for ( int i = 0; i < ZZSH_CTX_ALL ; i++)
				uniform_buffer[i].SettleFloat((int) param, v);
	}

	bool IsDualContext(ZZshParameter param) { return false;}

};
#endif

	extern VERTEXSHADER pvsBitBlt;
	extern FRAGMENTSHADER ppsBitBlt[2], ppsBitBltDepth, ppsOne;					// ppsOne used to stop using shaders for draw
	extern FRAGMENTSHADER ppsBaseTexture, ppsConvert16to32, ppsConvert32to16;

	extern FRAGMENTSHADER ppsRegular[4], ppsTexture[NUM_SHADERS];
	extern FRAGMENTSHADER ppsCRTC[2], /*ppsCRTC24[2],*/ ppsCRTCTarg[2];
#ifdef GLSL4_API
	extern COMMONSHADER g_cs;
#endif

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
#if defined(GLSL_API) && !defined(GLSL4_API)
inline bool ZZshExistProgram(FRAGMENTSHADER* pf) {return (pf->Shader != 0); };
inline bool ZZshExistProgram(VERTEXSHADER* pf) {return (pf->Shader != 0); };
inline bool ZZshExistProgram(ZZshShaderLink prog) {return (prog.link != NULL); }		// This is used for pvs mainly. No NULL means that we do LOAD_VS
#endif
#if defined(GLSL4_API)
inline bool ZZshExistProgram(FRAGMENTSHADER* pf) {return (pf->program != 0); };
inline bool ZZshExistProgram(VERTEXSHADER* pf) {return (pf->program != 0); };
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
extern void ZZshSetParameter4fv(ZZshShaderLink& prog, ZZshParameter param, const float* v, const char* name);
extern void ZZshSetParameter4fv(ZZshParameter param, const float* v, const char* name);
extern void ZZshSetParameter4fvWithRetry(ZZshParameter* param, ZZshShaderLink& prog, const float* v, const char* name);

// Set the Texture parameter in host (NOT GL)
extern void ZZshGLSetTextureParameter(ZZshShaderLink prog, ZZshParameter param, GLuint texobj, const char* name);
extern void ZZshGLSetTextureParameter(ZZshParameter param, GLuint texobj, const char* name);

// Set a default value for 1 uniform in host (NOT GL)
extern void ZZshDefaultOneColor( FRAGMENTSHADER& ptr );

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

#ifdef GLSL4_API
extern GSUniformBufferOGL *constant_buffer;
extern GSUniformBufferOGL *common_buffer;
extern GSUniformBufferOGL *vertex_buffer;
extern GSUniformBufferOGL *fragment_buffer;
extern GSVertexBufferStateOGL *vertex_array;

extern void init_shader();
extern void PutParametersInProgram(VERTEXSHADER* vs, FRAGMENTSHADER* ps);
extern void init_shader();
extern void ZZshSetupShader();

#endif

#endif
