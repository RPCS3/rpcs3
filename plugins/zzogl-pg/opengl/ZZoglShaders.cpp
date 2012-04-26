/*  ZZ Open GL graphics plugin
 *  Copyright (c)2009 zeydlitz@gmail.com
 *  Based on Zerofrog's ZeroGS KOSMOS (c)2005-2006
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
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

// By default enable nvidia cg api
#if !defined(GLSL_API) && !defined(NVIDIA_CG_API)
#define NVIDIA_CG_API
#endif

#ifdef NVIDIA_CG_API 		// This code is only for NVIDIA cg-toolkit API
// ZZogl Shader manipulation functions.

//------------------- Includes
#include "Util.h"
#include "ZZoglShaders.h"
#include "zpipe.h"
#include <math.h>
#include <map>

#ifdef _WIN32
#	include "Win32.h"
extern HINSTANCE hInst;
#endif

// ----------------- Defines

#define TEXWRAP_REPEAT 0
#define TEXWRAP_CLAMP 1
#define TEXWRAP_REGION_REPEAT 2
#define TEXWRAP_REPEAT_CLAMP 3

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

//------------------ Constants

// Used in a logarithmic Z-test, as (1-o(1))/log(MAX_U32).
const float g_filog32 = 0.999f / (32.0f * logf(2.0f));

#ifdef _DEBUG
const static char* g_pTexTypes[] = { "32", "tex32", "clut32", "tex32to16", "tex16to8h" };
#endif
const char* g_pShaders[4] = { "full", "reduced", "accurate", "accurate-reduced" };

// ----------------- Global Variables

ZZshContext	g_cgcontext;
ZZshProfile 	cgvProf, cgfProf;
int 		g_nPixelShaderVer = 0; 		// default
u8* 		s_lpShaderResources = NULL;
ZZshProgram 	pvs[16] = {NULL};
ZZshProgram 	g_vsprog = 0, g_psprog = 0;							// 2 -- ZZ
ZZshParameter 	g_vparamPosXY[2] = {0}, g_fparamFogColor = 0;

//#ifdef DEVBUILD
extern char* EFFECT_NAME;		// All this variables used for testing and set manually
extern char* EFFECT_DIR;
//#endif

bool g_bCRTCBilinear = true;

float4 g_vdepth, vlogz;
FRAGMENTSHADER ppsBitBlt[2], ppsBitBltDepth, ppsOne;
FRAGMENTSHADER ppsBaseTexture, ppsConvert16to32, ppsConvert32to16;
FRAGMENTSHADER ppsRegular[4], ppsTexture[NUM_SHADERS];
FRAGMENTSHADER ppsCRTC[2], /*ppsCRTC24[2],*/ ppsCRTCTarg[2];
VERTEXSHADER pvsBitBlt;

inline bool LoadEffects();
extern bool s_bWriteDepth;

struct SHADERHEADER
{
	unsigned int index, offset, size; // if highest bit of index is set, pixel shader
};
map<int, SHADERHEADER*> mapShaderResources;

// Debug variable, store name of the function that call the shader.
const char* ShaderCallerName = "";
const char* ShaderHandleName = "";

//------------------ Code

inline int GET_SHADER_INDEX(int type, int texfilter, int texwrap, int fog, int writedepth, int testaem, int exactcolor, int context, int ps) {
	return type + texfilter*NUM_TYPES + NUM_FILTERS*NUM_TYPES*texwrap + NUM_TEXWRAPS*NUM_FILTERS*NUM_TYPES*(fog+2*writedepth+4*testaem+8*exactcolor+16*context+32*ps) ;
}

bool ZZshCheckProfilesSupport() {
	// load the effect, find the best profiles (if any)
	if (cgGLIsProfileSupported(CG_PROFILE_ARBVP1) != CG_TRUE) {
		ZZLog::Error_Log("arbvp1 not supported.");
		return false;
	}
	if (cgGLIsProfileSupported(CG_PROFILE_ARBFP1) != CG_TRUE) {
		ZZLog::Error_Log("arbfp1 not supported.");
		return false;
	}
	return true;
}

// Error handler. Setup in ZZogl_Create once.
void HandleCgError(ZZshContext ctx, ZZshError err, void* appdata)
{
	ZZLog::Error_Log("%s->%s: %s\n", ShaderCallerName, ShaderHandleName, cgGetErrorString(err));
	const char* listing = cgGetLastListing(g_cgcontext);
	if (listing != NULL)
		ZZLog::Debug_Log("	last listing: %s\n", listing);
}

bool ZZshStartUsingShaders() {
	cgSetErrorHandler(HandleCgError, NULL);
	g_cgcontext = cgCreateContext();

	cgvProf = CG_PROFILE_ARBVP1;
	cgfProf = CG_PROFILE_ARBFP1;
	cgGLEnableProfile(cgvProf);
	cgGLEnableProfile(cgfProf);
	cgGLSetOptimalOptions(cgvProf);
	cgGLSetOptimalOptions(cgfProf);

	cgGLSetManageTextureParameters(g_cgcontext, CG_FALSE);
	//cgSetAutoCompile(g_cgcontext, CG_COMPILE_IMMEDIATE);

	g_fparamFogColor = cgCreateParameter(g_cgcontext, CG_FLOAT4);
	g_vparamPosXY[0] = cgCreateParameter(g_cgcontext, CG_FLOAT4);
	g_vparamPosXY[1] = cgCreateParameter(g_cgcontext, CG_FLOAT4);


	ZZLog::GS_Log("Creating effects.");
	B_G(LoadEffects(), return false);

	// create a sample shader
	clampInfo temp;
	memset(&temp, 0, sizeof(temp));
	temp.wms = 3; temp.wmt = 3;

	g_nPixelShaderVer = 0;//SHADER_ACCURATE;
	// test
	bool bFailed;
	FRAGMENTSHADER* pfrag = ZZshLoadShadeEffect(0, 1, 1, 1, 1, temp, 0, &bFailed);
	if( bFailed || pfrag == NULL ) {
		g_nPixelShaderVer = SHADER_ACCURATE|SHADER_REDUCED;

		pfrag = ZZshLoadShadeEffect(0, 0, 1, 1, 0, temp, 0, &bFailed);
		if( pfrag != NULL )
			cgGLLoadProgram(pfrag->prog);
		if( bFailed || pfrag == NULL || cgGetError() != CG_NO_ERROR ) {
			g_nPixelShaderVer = SHADER_REDUCED;
			ZZLog::Error_Log("Basic shader test failed.");
		}
	}

	if (g_nPixelShaderVer & SHADER_REDUCED)
		conf.bilinear = 0;

	ZZLog::GS_Log("Creating extra effects.");
	B_G(ZZshLoadExtraEffects(), return false);

	ZZLog::GS_Log("using %s shaders\n", g_pShaders[g_nPixelShaderVer]);
	return true;
}

void ZZshExitCleaning() {
	// nothing to do with cg
}

// open shader file according to build target
bool ZZshCreateOpenShadersFile() {
#ifndef DEVBUILD
#	ifdef _WIN32
	HRSRC hShaderSrc = FindResource(hInst, MAKEINTRESOURCE(IDR_SHADERS), RT_RCDATA);
	assert( hShaderSrc != NULL );
	HGLOBAL hShaderGlob = LoadResource(hInst, hShaderSrc);
	assert( hShaderGlob != NULL );
	s_lpShaderResources = (u8*)LockResource(hShaderGlob);
#	else // not _WIN32
	FILE* fres = fopen("ps2hw.dat", "rb");
	if( fres == NULL ) {
		fres = fopen("plugins/ps2hw.dat", "rb");
		if( fres == NULL ) {
			ZZLog::Error_Log("Cannot find ps2hw.dat in working directory. Exiting.");
			return false;
		}
	}
	fseek(fres, 0, SEEK_END);
	size_t s = ftell(fres);
	s_lpShaderResources = new u8[s+1];
	fseek(fres, 0, SEEK_SET);
	fread(s_lpShaderResources, s, 1, fres);
	s_lpShaderResources[s] = 0;
#	endif // _WIN32
#else // NOT RELEASE_TO_PUBLIC
#	ifndef _WIN32 // NOT WINDOWS
	// test if ps2hw.fx exists
	char tempstr[255];
	char curwd[255];
	getcwd(curwd, ArraySize(curwd));

	strcpy(tempstr, "/plugins/");
	sprintf(EFFECT_NAME, "%sps2hw.fx", tempstr);
	FILE* f = fopen(EFFECT_NAME, "r");
	if( f == NULL ) {

		strcpy(tempstr, "../../plugins/zzogl-pg/opengl/");
		sprintf(EFFECT_NAME, "%sps2hw.fx", tempstr);
		f = fopen(EFFECT_NAME, "r");

		if( f == NULL ) {
			ZZLog::Error_Log("Failed to find %s, try compiling a non-devbuild\n", EFFECT_NAME);
			return false;
		}
	}
	fclose(f);

	sprintf(EFFECT_DIR, "%s/%s", curwd, tempstr);
	sprintf(EFFECT_NAME, "%sps2hw.fx", EFFECT_DIR);
	#endif
#endif // RELEASE_TO_PUBLIC
	return true;
}

// Disable CG
void ZZshGLDisableProfile() {
	cgGLDisableProfile(cgvProf);
	cgGLDisableProfile(cgfProf);
}
//Enable CG
void ZZshGLEnableProfile() {
	cgGLEnableProfile(cgvProf);
	cgGLEnableProfile(cgfProf);
}

// This is helper of cgGLSetParameter4fv, made for debug purpose.
// Name could be any string. We must use it on compilation time, because erroneus handler does not
// return name
void ZZshSetParameter4fv(ZZshParameter param, const float* v, const char* name) {
	ShaderHandleName = name;
	cgGLSetParameter4fv(param, v);
}

void ZZshSetParameter4fv(ZZshProgram prog, ZZshParameter param, const float* v, const char* name) {
	ShaderHandleName = name;
	cgGLSetParameter4fv(param, v);
}

// The same stuff, but also with retry of param, name should be USED name of param for prog.
void ZZshSetParameter4fvWithRetry(ZZshParameter* param, ZZshProgram prog, const float* v, const char* name) {
	if (param != NULL)
		ZZshSetParameter4fv(prog, param[0], v, name);
	else
		ZZshSetParameter4fv(prog, cgGetNamedParameter(prog, name), v, name);
}

void ZZshGLSetTextureParameter(ZZshParameter param, GLuint texobj, const char* name) {
	ShaderHandleName = name;
	cgGLSetTextureParameter(param, texobj);
	cgGLEnableTextureParameter(param);
}

// The same function for texture, also to cgGLEnable
void ZZshGLSetTextureParameter(ZZshProgram prog, ZZshParameter param, GLuint texobj, const char* name) {
	ShaderHandleName = name;
	cgGLSetTextureParameter(param, texobj);
	cgGLEnableTextureParameter(param);
}

// Used sometimes for color 1.
void ZZshDefaultOneColor( FRAGMENTSHADER ptr ) {
	ShaderHandleName = "Set Default One color";
	float4 v = float4 ( 1, 1, 1, 1 );
	ZZshSetParameter4fv( ptr.prog, ptr.sOneColor, v, "DefaultOne");
}

#define SET_UNIFORMPARAM(var, name) { \
	p = cgGetNamedParameter(pf->prog, name); \
	if( p != NULL && cgIsParameterUsed(p, pf->prog) == CG_TRUE ) \
		pf->var = p; \
} \

void ZZshSetVertexShader(ZZshProgram prog) {
	if ((prog) != g_vsprog) {
		cgGLBindProgram(prog);
		g_vsprog = prog;
	}
}

void ZZshSetPixelShader(ZZshProgram prog) {
	if ((prog) != g_psprog) {
		cgGLBindProgram(prog);
		g_psprog = prog;
	}
}

void SetupFragmentProgramParameters(FRAGMENTSHADER* pf, int context, int type)
{
	// uniform parameters
	ZZshParameter p;

	p = cgGetNamedParameter(pf->prog, "g_fFogColor");
	if( p != NULL && cgIsParameterUsed(p, pf->prog) == CG_TRUE ) {
		cgConnectParameter(g_fparamFogColor, p);
	}

	SET_UNIFORMPARAM(sOneColor, "g_fOneColor");
	SET_UNIFORMPARAM(sBitBltZ, "g_fBitBltZ");
	SET_UNIFORMPARAM(sInvTexDims, "g_fInvTexDims");
	SET_UNIFORMPARAM(fTexAlpha2, "fTexAlpha2");
	SET_UNIFORMPARAM(fTexOffset, "g_fTexOffset");
	SET_UNIFORMPARAM(fTexDims, "g_fTexDims");
	SET_UNIFORMPARAM(fTexBlock, "g_fTexBlock");
	SET_UNIFORMPARAM(fClampExts, "g_fClampExts");
	SET_UNIFORMPARAM(fTexWrapMode, "TexWrapMode");
	SET_UNIFORMPARAM(fRealTexDims, "g_fRealTexDims");
	SET_UNIFORMPARAM(fTestBlack, "g_fTestBlack");
	SET_UNIFORMPARAM(fPageOffset, "g_fPageOffset");
	SET_UNIFORMPARAM(fTexAlpha, "fTexAlpha");

	// textures
	p = cgGetNamedParameter(pf->prog, "g_sBlocks");
	if( p != NULL && cgIsParameterUsed(p, pf->prog) == CG_TRUE ) {
		cgGLSetTextureParameter(p, ptexBlocks);
		cgGLEnableTextureParameter(p);
	}

	// cg parameter usage is wrong, so do it manually
	if( type == 3 ) {
		p = cgGetNamedParameter(pf->prog, "g_sConv16to32");
		if( p != NULL && cgIsParameterUsed(p, pf->prog) == CG_TRUE ) {
			cgGLSetTextureParameter(p, ptexConv16to32);
			cgGLEnableTextureParameter(p);
		}
	}
	else if( type == 4 ) {
		p = cgGetNamedParameter(pf->prog, "g_sConv32to16");
		if( p != NULL && cgIsParameterUsed(p, pf->prog) == CG_TRUE ) {
			cgGLSetTextureParameter(p, ptexConv32to16);
			cgGLEnableTextureParameter(p);
		}
	}
	else {
		p = cgGetNamedParameter(pf->prog, "g_sBilinearBlocks");
		if( p != NULL && cgIsParameterUsed(p, pf->prog) == CG_TRUE ) {
			cgGLSetTextureParameter(p, ptexBilinearBlocks);
			cgGLEnableTextureParameter(p);
		}
	}

	p = cgGetNamedParameter(pf->prog, "g_sMemory");
	if( p != NULL && cgIsParameterUsed(p, pf->prog) == CG_TRUE ) {
		//cgGLEnableTextureParameter(p);
		pf->sMemory = p;
	}
	p = cgGetNamedParameter(pf->prog, "g_sSrcFinal");
	if( p != NULL && cgIsParameterUsed(p, pf->prog) == CG_TRUE ) {
		//cgGLEnableTextureParameter(p);
		pf->sFinal = p;
	}
	p = cgGetNamedParameter(pf->prog, "g_sBitwiseANDX");
	if( p != NULL && cgIsParameterUsed(p, pf->prog) == CG_TRUE ) {
		//cgGLEnableTextureParameter(p);
		pf->sBitwiseANDX = p;
	}
	p = cgGetNamedParameter(pf->prog, "g_sBitwiseANDY");
	if( p != NULL && cgIsParameterUsed(p, pf->prog) == CG_TRUE ) {
		//cgGLEnableTextureParameter(p);
		pf->sBitwiseANDY = p;
	}
	p = cgGetNamedParameter(pf->prog, "g_sCLUT");
	if( p != NULL && cgIsParameterUsed(p, pf->prog) == CG_TRUE ) {
		//cgGLEnableTextureParameter(p);
		pf->sCLUT = p;
	}
	p = cgGetNamedParameter(pf->prog, "g_sInterlace");
	if( p != NULL && cgIsParameterUsed(p, pf->prog) == CG_TRUE ) {
		//cgGLEnableTextureParameter(p);
		pf->sInterlace = p;
	}

	// set global shader constants
	p = cgGetNamedParameter(pf->prog, "g_fExactColor");
	if( p != NULL && cgIsParameterUsed(p, pf->prog) == CG_TRUE ) {
		cgGLSetParameter4fv(p, float4(0.5f, (conf.settings().exact_color)?0.9f/256.0f:0.5f/256.0f, 0,1/255.0f));
	}

	p = cgGetNamedParameter(pf->prog, "g_fBilinear");
	if( p != NULL && cgIsParameterUsed(p, pf->prog) == CG_TRUE )
		cgGLSetParameter4fv(p, float4(-0.2f, -0.65f, 0.9f, 1.0f / 32767.0f ));

	p = cgGetNamedParameter(pf->prog, "g_fZBias");
	if( p != NULL && cgIsParameterUsed(p, pf->prog) == CG_TRUE )
		cgGLSetParameter4fv(p, float4(1.0f/256.0f, 1.0004f, 1, 0.5f));

	p = cgGetNamedParameter(pf->prog, "g_fc0");
	if( p != NULL && cgIsParameterUsed(p, pf->prog) == CG_TRUE )
		cgGLSetParameter4fv(p, float4(0,1, 0.001f, 0.5f));

	p = cgGetNamedParameter(pf->prog, "g_fMult");
	if( p != NULL && cgIsParameterUsed(p, pf->prog) == CG_TRUE )
		cgGLSetParameter4fv(p, float4(1/1024.0f, 0.2f/1024.0f, 1/128.0f, 1/512.0f));
}

void SetupVertexProgramParameters(ZZshProgram prog, int context)
{
	ZZshParameter p;

	p = cgGetNamedParameter(prog, "g_fPosXY");
	if( p != NULL && cgIsParameterUsed(p, prog) == CG_TRUE )
		cgConnectParameter(g_vparamPosXY[context], p);

	// Set Z-test, log or no log;
	if (conf.settings().no_logz) {
       		g_vdepth = float4( 255.0 /256.0f,  255.0/65536.0f, 255.0f/(65535.0f*256.0f), 1.0f/(65536.0f*65536.0f));
		vlogz = float4( 1.0f, 0.0f, 0.0f, 0.0f);
	}
	else {
		g_vdepth = float4( 256.0f*65536.0f, 65536.0f, 256.0f, 65536.0f*65536.0f);
		vlogz = float4( 0.0f, 1.0f, 0.0f, 0.0f);
	}

	p = cgGetNamedParameter(prog, "g_fZ");
	if( p != NULL && cgIsParameterUsed(p, prog) == CG_TRUE ) {
		cgGLSetParameter4fv(p, g_vdepth);

		p = cgGetNamedParameter(prog, "g_fZMin"); // Switch to flat-z when needed
		if( p != NULL && cgIsParameterUsed(p, prog) == CG_TRUE )  {
			//ZZLog::Error_Log("Use flat-z\n");
			cgGLSetParameter4fv(p, vlogz);
		}
		else
			ZZLog::Error_Log("Shader file version is outdated! Only log-Z is possible.");
	}

	float4 vnorm = float4(g_filog32, 0, 0,0);
	p = cgGetNamedParameter(prog, "g_fZNorm");
	if( p != NULL && cgIsParameterUsed(p, prog) == CG_TRUE )
		cgGLSetParameter4fv(p, vnorm);

	p = cgGetNamedParameter(prog, "g_fBilinear");
	if( p != NULL && cgIsParameterUsed(p, prog) == CG_TRUE )
		cgGLSetParameter4fv(p, float4(-0.2f, -0.65f, 0.9f, 1.0f / 32767.0f ));

	p = cgGetNamedParameter(prog, "g_fZBias");
	if( p != NULL && cgIsParameterUsed(p, prog) == CG_TRUE )
		cgGLSetParameter4fv(p, float4(1.0f/256.0f, 1.0004f, 1, 0.5f));

	p = cgGetNamedParameter(prog, "g_fc0");
	if( p != NULL && cgIsParameterUsed(p, prog) == CG_TRUE )
		cgGLSetParameter4fv(p, float4(0,1, 0.001f, 0.5f));
}

#ifndef DEVBUILD
#if 0
static __forceinline void LOAD_VS(int Index, ZZshProgram prog)
{
	assert(mapShaderResources.find(Index) != mapShaderResources.end());
	header = mapShaderResources[Index];
	assert((header) != NULL && (header)->index == (Index));
	prog = cgCreateProgram(g_cgcontext, CG_OBJECT, (char*)(s_lpShaderResources + (header)->offset), cgvProf, NULL, NULL);
	if (!cgIsProgram(prog)) 
	{
		ZZLog::Error_Log("Failed to load vs %d: \n%s", Index, cgGetLastListing(g_cgcontext));
		return false;
	}
	cgGLLoadProgram(prog);
	
	if (cgGetError() != CG_NO_ERROR) ZZLog::Error_Log("Failed to load program %d.", Index);
	SetupVertexProgramParameters(prog, !!(Index&SH_CONTEXT1));	
}


static __forceinline void LOAD_VS(int Index, FRAGMENTSHADER fragment)
{
	bLoadSuccess = true;
	assert(mapShaderResources.find(Index) != mapShaderResources.end());
	header = mapShaderResources[Index];
	fragment.prog = cgCreateProgram(g_cgcontext, CG_OBJECT, (char*)(s_lpShaderResources + (header)->offset), cgfProf, NULL, NULL);
	if (!cgIsProgram(fragment.prog)) 
	{
		ZZLog::Error_Log("Failed to load ps %d: \n%s", Index, cgGetLastListing(g_cgcontext));
		return false;
	}
	
	cgGLLoadProgram(fragment.prog);
	
	if (cgGetError() != CG_NO_ERROR) 
	{
		ZZLog::Error_Log("failed to load program %d.", Index);
		bLoadSuccess = false;
	}
	
	SetupFragmentProgramParameters(&fragment, !!(Index&SH_CONTEXT1), 0);
}
#endif

#define LOAD_VS(Index, prog) {						  \
	assert( mapShaderResources.find(Index) != mapShaderResources.end() ); \
	header = mapShaderResources[Index]; \
	assert( (header) != NULL && (header)->index == (Index) ); \
	prog = cgCreateProgram(g_cgcontext, CG_OBJECT, (char*)(s_lpShaderResources + (header)->offset), cgvProf, NULL, NULL); \
	if( !cgIsProgram(prog) ) { \
		ZZLog::Error_Log("Failed to load vs %d: \n%s", Index, cgGetLastListing(g_cgcontext)); \
		return false; \
	} \
	cgGLLoadProgram(prog); \
	if( cgGetError() != CG_NO_ERROR ) ZZLog::Error_Log("Failed to load program %d.", Index); \
	SetupVertexProgramParameters(prog, !!(Index&SH_CONTEXT1));			\
} \

#define LOAD_PS(Index, fragment) {  \
	bLoadSuccess = true; \
	assert( mapShaderResources.find(Index) != mapShaderResources.end() ); \
	header = mapShaderResources[Index]; \
	fragment.prog = cgCreateProgram(g_cgcontext, CG_OBJECT, (char*)(s_lpShaderResources + (header)->offset), cgfProf, NULL, NULL); \
	if( !cgIsProgram(fragment.prog) ) { \
		ZZLog::Error_Log("Failed to load ps %d: \n%s", Index, cgGetLastListing(g_cgcontext)); \
		return false; \
	} \
	cgGLLoadProgram(fragment.prog); \
	if( cgGetError() != CG_NO_ERROR ) { \
		ZZLog::Error_Log("failed to load program %d.", Index);		   \
		bLoadSuccess = false; \
	} \
	SetupFragmentProgramParameters(&fragment, !!(Index&SH_CONTEXT1), 0);  \
} \

inline bool LoadEffects()
{
	assert( s_lpShaderResources != NULL );

	// process the header
	u32 num = *(u32*)s_lpShaderResources;
	int compressed_size = *(int*)(s_lpShaderResources+4);
	int real_size = *(int*)(s_lpShaderResources+8);
	int out;

	char* pbuffer = (char*)malloc(real_size);
	inf((char*)s_lpShaderResources+12, &pbuffer[0], compressed_size, real_size, &out);
	assert(out == real_size);

	s_lpShaderResources = (u8*)pbuffer;
	SHADERHEADER* header = (SHADERHEADER*)s_lpShaderResources;

	mapShaderResources.clear();
	while(num-- > 0 ) {
		mapShaderResources[header->index] = header;
		++header;
	}

	// clear the textures
	for(u16 i = 0; i < ArraySize(ppsTexture); ++i) {
		SAFE_RELEASE_PROG(ppsTexture[i].prog);
		ppsTexture[i].prog = NULL;
	}
#ifndef _DEBUG
	memset(ppsTexture, 0, sizeof(ppsTexture));
#endif

	return true;
}

// called
bool ZZshLoadExtraEffects()
{
	SHADERHEADER* header;
	bool bLoadSuccess = true;

	const int vsshaders[4] = { SH_REGULARVS, SH_TEXTUREVS, SH_REGULARFOGVS, SH_TEXTUREFOGVS };

	for(int i = 0; i < 4; ++i) {
		LOAD_VS(vsshaders[i], pvs[2*i]);
		LOAD_VS((vsshaders[i] | SH_CONTEXT1), pvs[2*i+1]);
		//if( conf.mrtdepth ) {
			LOAD_VS((vsshaders[i] | SH_WRITEDEPTH), pvs[2*i+8]);
			LOAD_VS((vsshaders[i] | SH_WRITEDEPTH | SH_CONTEXT1), pvs[2*i+8+1]);
//		}
//		else {
//			pvs[2*i+8] = pvs[2*i+8+1] = NULL;
//		}
	}

	LOAD_VS(SH_BITBLTVS, pvsBitBlt.prog);
	pvsBitBlt.sBitBltPos = cgGetNamedParameter(pvsBitBlt.prog, "g_fBitBltPos");
	pvsBitBlt.sBitBltTex = cgGetNamedParameter(pvsBitBlt.prog, "g_fBitBltTex");
	pvsBitBlt.fBitBltTrans = cgGetNamedParameter(pvsBitBlt.prog, "g_fBitBltTrans");

	LOAD_PS(SH_REGULARPS, ppsRegular[0]);
	LOAD_PS(SH_REGULARFOGPS, ppsRegular[1]);

	if( conf.mrtdepth ) {
		LOAD_PS(SH_REGULARPS, ppsRegular[2]);
		if( !bLoadSuccess )
			conf.mrtdepth = 0;
		LOAD_PS(SH_REGULARFOGPS, ppsRegular[3]);
		if( !bLoadSuccess )
			conf.mrtdepth = 0;
	}

	LOAD_PS(SH_BITBLTPS, ppsBitBlt[0]);
	LOAD_PS(SH_BITBLTAAPS, ppsBitBlt[1]);
	if( !bLoadSuccess ) {
		ZZLog::Error_Log("Failed to load BitBltAAPS, using BitBltPS.");
		LOAD_PS(SH_BITBLTPS, ppsBitBlt[1]);
	}
	LOAD_PS(SH_BITBLTDEPTHPS, ppsBitBltDepth);
	LOAD_PS(SH_CRTCTARGPS, ppsCRTCTarg[0]);
	LOAD_PS(SH_CRTCTARGINTERPS, ppsCRTCTarg[1]);

	g_bCRTCBilinear = true;
	LOAD_PS(SH_CRTCPS, ppsCRTC[0]);
	if( !bLoadSuccess ) {
		// switch to simpler
		g_bCRTCBilinear = false;
		LOAD_PS(SH_CRTC_NEARESTPS, ppsCRTC[0]);
		LOAD_PS(SH_CRTCINTER_NEARESTPS, ppsCRTC[0]);
	}
	else {
		LOAD_PS(SH_CRTCINTERPS, ppsCRTC[1]);
	}

	if( !bLoadSuccess )
		ZZLog::Error_Log("Failed to create CRTC shaders.");

//	LOAD_PS(SH_CRTC24PS, ppsCRTC24[0]);
//	LOAD_PS(SH_CRTC24INTERPS, ppsCRTC24[1]);
	LOAD_PS(SH_ZEROPS, ppsOne);
	LOAD_PS(SH_BASETEXTUREPS, ppsBaseTexture);
	LOAD_PS(SH_CONVERT16TO32PS, ppsConvert16to32);
	LOAD_PS(SH_CONVERT32TO16PS, ppsConvert32to16);

	return true;
}

FRAGMENTSHADER* ZZshLoadShadeEffect(int type, int texfilter, int fog, int testaem, int exactcolor, const clampInfo& clamp, int context, bool* pbFailed)
{
	int texwrap;
	assert( texfilter < NUM_FILTERS );

	if(g_nPixelShaderVer&SHADER_REDUCED)
		texfilter = 0;
	assert(!(g_nPixelShaderVer&SHADER_REDUCED) || !exactcolor);

	if( clamp.wms == clamp.wmt ) {
		switch( clamp.wms ) {
			case 0: texwrap = TEXWRAP_REPEAT; break;
			case 1: texwrap = TEXWRAP_CLAMP; break;
			case 2: texwrap = TEXWRAP_CLAMP; break;
			default: texwrap = TEXWRAP_REGION_REPEAT; break;
		}
	}
	else if( clamp.wms==3||clamp.wmt==3)
		texwrap = TEXWRAP_REGION_REPEAT;
	else
		texwrap = TEXWRAP_REPEAT_CLAMP;

	int index = GET_SHADER_INDEX(type, texfilter, texwrap, fog, s_bWriteDepth, testaem, exactcolor, context, 0);

	assert( index < ArraySize(ppsTexture) );
	FRAGMENTSHADER* pf = ppsTexture+index;

	if( pbFailed != NULL ) *pbFailed = false;

	if( pf->prog != NULL )
		return pf;

	if( (g_nPixelShaderVer & SHADER_ACCURATE) && mapShaderResources.find(index+NUM_SHADERS*SHADER_ACCURATE) != mapShaderResources.end() )
		index += NUM_SHADERS*SHADER_ACCURATE;

	assert( mapShaderResources.find(index) != mapShaderResources.end() );
	SHADERHEADER* header = mapShaderResources[index];
	if( header == NULL )
		ZZLog::Error_Log("%d %d", index, g_nPixelShaderVer);
	assert( header != NULL );

	//DEBUG_LOG("shader:\n%s\n", (char*)(s_lpShaderResources + (header)->offset));
	pf->prog = cgCreateProgram(g_cgcontext, CG_OBJECT, (char*)(s_lpShaderResources + (header)->offset), cgfProf, NULL, NULL);
	if( pf->prog != NULL && cgIsProgram(pf->prog) && cgGetError() == CG_NO_ERROR ) {
		SetupFragmentProgramParameters(pf, context, type);
		cgGLLoadProgram(pf->prog);
		if( cgGetError() != CG_NO_ERROR ) {
//		  cgGLLoadProgram(pf->prog);
//		  if( cgGetError() != CG_NO_ERROR ) {
				ZZLog::Error_Log("Failed to load shader %d,%d,%d,%d.", type, fog, texfilter, 4*clamp.wms+clamp.wmt);
				if( pbFailed != NULL ) *pbFailed = true;
				return pf;
//		  }
		}
		return pf;
	}

	ZZLog::Error_Log("Failed to create shader %d,%d,%d,%d", type, fog, texfilter, 4*clamp.wms+clamp.wmt);
	if( pbFailed != NULL ) *pbFailed = true;

	return NULL;
}

#else // not RELEASE_TO_PUBLIC

#define LOAD_VS(name, prog, shaderver) { \
	prog = cgCreateProgramFromFile(g_cgcontext, CG_SOURCE, EFFECT_NAME, shaderver, name, args); \
	if( !cgIsProgram(prog) ) { \
		ZZLog::Error_Log("Failed to load vs %s: \n%s", name, cgGetLastListing(g_cgcontext)); \
		return false; \
	} \
	cgGLLoadProgram(prog); \
	if( cgGetError() != CG_NO_ERROR ) ZZLog::Error_Log("failed to load program %s", name); \
	SetupVertexProgramParameters(prog, args[0]==context1); \
} \

#ifdef _DEBUG
#define SET_PSFILENAME(frag, name) frag.filename = name
#else
#define SET_PSFILENAME(frag, name)
#endif

#define LOAD_PS(name, fragment, shaderver) { \
	bLoadSuccess = true; \
	fragment.prog = cgCreateProgramFromFile(g_cgcontext, CG_SOURCE, EFFECT_NAME, shaderver, name, args); \
	if( !cgIsProgram(fragment.prog) ) { \
		ZZLog::Error_Log("Failed to load ps %s: \n%s", name, cgGetLastListing(g_cgcontext)); \
		return false; \
	} \
	cgGLLoadProgram(fragment.prog); \
	if( cgGetError() != CG_NO_ERROR ) { \
		ZZLog::Error_Log("failed to load program %s", name);		   \
		bLoadSuccess = false; \
	} \
	SetupFragmentProgramParameters(&fragment, args[0]==context1, 0);  \
	SET_PSFILENAME(fragment, name); \
} \

inline bool LoadEffects()
{
	// clear the textures
	for(int i = 0; i < ArraySize(ppsTexture); ++i) {
		SAFE_RELEASE_PROG(ppsTexture[i].prog);
	}

#ifndef _DEBUG
	memset(ppsTexture, 0, sizeof(ppsTexture));
#endif

	return true;
}

bool ZZshLoadExtraEffects()
{
	const char* args[] = { NULL , NULL, NULL, NULL };
	char context0[255], context1[255];
	sprintf(context0, "-I%sctx0", EFFECT_DIR);
	sprintf(context1, "-I%sctx1", EFFECT_DIR);
	char* write_depth = "-DWRITE_DEPTH";
	bool bLoadSuccess = true;

	const char* pvsshaders[4] = { "RegularVS", "TextureVS", "RegularFogVS", "TextureFogVS" };

	for(int i = 0; i < 4; ++i) {
		args[0] = context0;
		args[1] = NULL;
		LOAD_VS(pvsshaders[i], pvs[2*i], cgvProf);
		args[0] = context1;
		LOAD_VS(pvsshaders[i], pvs[2*i+1], cgvProf);

		//if( conf.mrtdepth ) {
			args[0] = context0;
			args[1] = write_depth;
			LOAD_VS(pvsshaders[i], pvs[2*i+8], cgvProf);
			args[0] = context1;
			LOAD_VS(pvsshaders[i], pvs[2*i+8+1], cgvProf);
//		}
//		else {
//			pvs[2*i+8] = pvs[2*i+8+1] = NULL;
//		}
	}

	args[0] = context0;
	args[1] = NULL;
	LOAD_VS("BitBltVS", pvsBitBlt.prog, cgvProf);
	pvsBitBlt.sBitBltPos = cgGetNamedParameter(pvsBitBlt.prog, "g_fBitBltPos");
	pvsBitBlt.sBitBltTex = cgGetNamedParameter(pvsBitBlt.prog, "g_fBitBltTex");
	pvsBitBlt.fBitBltTrans = cgGetNamedParameter(pvsBitBlt.prog, "g_fBitBltTrans");

	LOAD_PS("RegularPS", ppsRegular[0], cgfProf);
	LOAD_PS("RegularFogPS", ppsRegular[1], cgfProf);

	if( conf.mrtdepth ) {
		args[0] = context0;
		args[1] = write_depth;
		LOAD_PS("RegularPS", ppsRegular[2], cgfProf);
		if( !bLoadSuccess )
			conf.mrtdepth = 0;
		LOAD_PS("RegularFogPS", ppsRegular[3], cgfProf);
		if( !bLoadSuccess )
			conf.mrtdepth = 0;
	}

	LOAD_PS("BitBltPS", ppsBitBlt[0], cgfProf);
	LOAD_PS("BitBltAAPS", ppsBitBlt[1], cgfProf);
	if( !bLoadSuccess ) {
		ZZLog::Error_Log("Failed to load BitBltAAPS, using BitBltPS.");
		LOAD_PS("BitBltPS", ppsBitBlt[1], cgfProf);
	}

	LOAD_PS("BitBltDepthPS", ppsBitBltDepth, cgfProf);
	LOAD_PS("CRTCTargPS", ppsCRTCTarg[0], cgfProf);
	LOAD_PS("CRTCTargInterPS", ppsCRTCTarg[1], cgfProf);

	g_bCRTCBilinear = true;
	LOAD_PS("CRTCPS", ppsCRTC[0], cgfProf);
	if( !bLoadSuccess ) {
		// switch to simpler
		g_bCRTCBilinear = false;
		LOAD_PS("CRTCPS_Nearest", ppsCRTC[0], cgfProf);
		LOAD_PS("CRTCInterPS_Nearest", ppsCRTC[0], cgfProf);
	}
	else {
		LOAD_PS("CRTCInterPS", ppsCRTC[1], cgfProf);
	}

	if( !bLoadSuccess )
		ZZLog::Error_Log("Failed to create CRTC shaders.");

//	LOAD_PS("CRTC24PS", ppsCRTC24[0], cgfProf); LOAD_PS("CRTC24InterPS", ppsCRTC24[1], cgfProf);
	LOAD_PS("ZeroPS", ppsOne, cgfProf);
	LOAD_PS("BaseTexturePS", ppsBaseTexture, cgfProf);
	LOAD_PS("Convert16to32PS", ppsConvert16to32, cgfProf);
	LOAD_PS("Convert32to16PS", ppsConvert32to16, cgfProf);

//	if( !conf.mrtdepth ) {
//		ZZLog::Error_Log("Disabling MRT depth writing,");
//		s_bWriteDepth = FALSE;
//	}

	return true;
}

FRAGMENTSHADER* ZZshLoadShadeEffect(int type, int texfilter, int fog, int testaem, int exactcolor, const clampInfo& clamp, int context, bool* pbFailed)
{
	int texwrap;

	assert( texfilter < NUM_FILTERS );
	//assert( g_nPixelShaderVer == SHADER_30 );
	if( clamp.wms == clamp.wmt ) {
		switch( clamp.wms ) {
			case 0: texwrap = TEXWRAP_REPEAT; break;
			case 1: texwrap = TEXWRAP_CLAMP; break;
			case 2: texwrap = TEXWRAP_CLAMP; break;
			default:
				texwrap = TEXWRAP_REGION_REPEAT; break;
		}
	}
	else if( clamp.wms==3||clamp.wmt==3)
		texwrap = TEXWRAP_REGION_REPEAT;
	else
		texwrap = TEXWRAP_REPEAT_CLAMP;

	int index = GET_SHADER_INDEX(type, texfilter, texwrap, fog, s_bWriteDepth, testaem, exactcolor, context, 0);

	if( pbFailed != NULL ) *pbFailed = false;

	FRAGMENTSHADER* pf = ppsTexture+index;

	if( pf->prog != NULL )
		return pf;

	pf->prog = LoadShaderFromType(EFFECT_DIR, EFFECT_NAME, type, texfilter, texwrap, fog, s_bWriteDepth, testaem, exactcolor, g_nPixelShaderVer, context);

	if( pf->prog != NULL ) {
#ifdef _DEBUG
		char str[255];
		sprintf(str, "Texture%s%d_%sPS", fog?"Fog":"", texfilter, g_pTexTypes[type]);
		pf->filename = str;
#endif
		SetupFragmentProgramParameters(pf, context, type);
		cgGLLoadProgram(pf->prog);
		if( cgGetError() != CG_NO_ERROR ) {
			// try again
//			cgGLLoadProgram(pf->prog);
//			if( cgGetError() != CG_NO_ERROR ) {
				ZZLog::Error_Log("Failed to load shader %d,%d,%d,%d", type, fog, texfilter, 4*clamp.wms+clamp.wmt);
				if( pbFailed != NULL ) *pbFailed = true;
				//assert(0);
				// NULL makes things crash
				return pf;
//			}
		}
		return pf;
	}

	ZZLog::Error_Log("Failed to create shader %d,%d,%d,%d", type, fog, texfilter, 4*clamp.wms+clamp.wmt);
	if( pbFailed != NULL ) *pbFailed = true;

	return NULL;
}

#endif // RELEASE_TO_PUBLIC

#endif // NVIDIA_CG_API
