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

// ZZogl Shader manipulation functions.

//------------------- Includes
#include "zerogs.h"
#include "ZeroGSShaders/zerogsshaders.h"
#include "zpipe.h"

// ----------------- Defines

using namespace ZeroGS;
//------------------ Constants

// ----------------- Global Variables

namespace ZeroGS
{
FRAGMENTSHADER ppsBitBlt[2], ppsBitBltDepth, ppsOne;
FRAGMENTSHADER ppsBaseTexture, ppsConvert16to32, ppsConvert32to16;
}

// Debug variable, store name of the function that call the shader.
const char* ShaderCallerName = "";
const char* ShaderHandleName = "";

//------------------ Code

// Error handler. Setup in ZZogl_Create once.
void HandleCgError(CGcontext ctx, CGerror err, void* appdata)
{
	ZZLog::Error_Log("%s->%s: %s", ShaderCallerName, ShaderHandleName, cgGetErrorString(err));
	const char* listing = cgGetLastListing(g_cgcontext);

	if (listing != NULL) ZZLog::Debug_Log("	Last listing: %s", listing);
}

// This is a helper of cgGLSetParameter4fv, made for debugging purposes.
// The name could be any string. We must use it on compilation time, because the erronious handler does not
// return it.
void ZZcgSetParameter4fv(CGparameter param, const float* v, const char* name)
{
	ShaderHandleName = name;
	cgGLSetParameter4fv(param, v);
}

void SetupFragmentProgramParameters(FRAGMENTSHADER* pf, int context, int type)
{
	// uniform parameters
	pf->connect(g_fparamFogColor, "g_fFogColor");

	pf->set_uniform_param(pf->sOneColor, "g_fOneColor");
	pf->set_uniform_param(pf->sBitBltZ, "g_fBitBltZ");
	pf->set_uniform_param(pf->sInvTexDims, "g_fInvTexDims");
	pf->set_uniform_param(pf->fTexAlpha2, "fTexAlpha2");
	pf->set_uniform_param(pf->fTexOffset, "g_fTexOffset");
	pf->set_uniform_param(pf->fTexDims, "g_fTexDims");
	pf->set_uniform_param(pf->fTexBlock, "g_fTexBlock");
	pf->set_uniform_param(pf->fClampExts, "g_fClampExts");
	pf->set_uniform_param(pf->fTexWrapMode, "TexWrapMode");
	pf->set_uniform_param(pf->fRealTexDims, "g_fRealTexDims");
	pf->set_uniform_param(pf->fTestBlack, "g_fTestBlack");
	pf->set_uniform_param(pf->fPageOffset, "g_fPageOffset");
	pf->set_uniform_param(pf->fTexAlpha, "fTexAlpha");

	// textures
	pf->set_texture(ptexBlocks, "g_sBlocks");

	// cg parameter usage is wrong, so do it manually

	switch (type)
	{
		case 3:
			pf->set_texture(ptexConv16to32, "g_sConv16to32");
			break;

		case 4:
			pf->set_texture(ptexConv32to16, "g_sConv32to16");
			break;

		default:
			pf->set_texture(ptexBilinearBlocks, "g_sBilinearBlocks");
			break;
	}

	pf->set_texture(pf->sMemory, "g_sMemory");

	pf->set_texture(pf->sFinal, "g_sSrcFinal");
	pf->set_texture(pf->sBitwiseANDX, "g_sBitwiseANDX");
	pf->set_texture(pf->sBitwiseANDY, "g_sBitwiseANDY");
	pf->set_texture(pf->sCLUT, "g_sCLUT");
	pf->set_texture(pf->sInterlace, "g_sInterlace");

	// set global shader constants
	pf->set_shader_const(Vector(0.5f, (conf.settings().exact_color) ? 0.9f / 256.0f : 0.5f / 256.0f, 0, 1 / 255.0f), "g_fExactColor");
	pf->set_shader_const(Vector(-0.2f, -0.65f, 0.9f, 1.0f / 32767.0f), "g_fBilinear");
	pf->set_shader_const(Vector(1.0f / 256.0f, 1.0004f, 1, 0.5f), "g_fZBias");
	pf->set_shader_const(Vector(0, 1, 0.001f, 0.5f), "g_fc0");
	pf->set_shader_const(Vector(1 / 1024.0f, 0.2f / 1024.0f, 1 / 128.0f, 1 / 512.0f), "g_fMult");
}

void SetupVertexProgramParameters(CGprogram prog, int context)
{
	CGparameter p;

	p = cgGetNamedParameter(prog, "g_fPosXY");

	if (p != NULL && cgIsParameterUsed(p, prog) == CG_TRUE)
		cgConnectParameter(g_vparamPosXY[context], p);

	// Set Z-test, log or no log;
	if (conf.settings().no_logz)
	{
		g_vdepth = Vector(255.0 / 256.0f,  255.0 / 65536.0f, 255.0f / (65535.0f * 256.0f), 1.0f / (65536.0f * 65536.0f));
		vlogz = Vector(1.0f, 0.0f, 0.0f, 0.0f);
	}
	else
	{
		g_vdepth = Vector(256.0f * 65536.0f, 65536.0f, 256.0f, 65536.0f * 65536.0f);
		vlogz = Vector(0.0f, 1.0f, 0.0f, 0.0f);
	}

	p = cgGetNamedParameter(prog, "g_fZ");

	if (p != NULL && cgIsParameterUsed(p, prog) == CG_TRUE)
	{
		cgGLSetParameter4fv(p, g_vdepth);

		p = cgGetNamedParameter(prog, "g_fZMin"); // Switch to flat-z when needed

		if (p != NULL && cgIsParameterUsed(p, prog) == CG_TRUE)
		{
			//ZZLog::Error_Log("Use flat-z");
			cgGLSetParameter4fv(p, vlogz);
		}
		else
		{
			ZZLog::Error_Log("Shader file version is outdated! Only log-Z is possible.");
		}
	}

	Vector vnorm = Vector(g_filog32, 0, 0, 0);

	p = cgGetNamedParameter(prog, "g_fZNorm");

	if (p != NULL && cgIsParameterUsed(p, prog) == CG_TRUE)
		cgGLSetParameter4fv(p, vnorm);

	p = cgGetNamedParameter(prog, "g_fBilinear");

	if (p != NULL && cgIsParameterUsed(p, prog) == CG_TRUE)
		cgGLSetParameter4fv(p, Vector(-0.2f, -0.65f, 0.9f, 1.0f / 32767.0f));

	p = cgGetNamedParameter(prog, "g_fZBias");

	if (p != NULL && cgIsParameterUsed(p, prog) == CG_TRUE)
		cgGLSetParameter4fv(p, Vector(1.0f / 256.0f, 1.0004f, 1, 0.5f));

	p = cgGetNamedParameter(prog, "g_fc0");

	if (p != NULL && cgIsParameterUsed(p, prog) == CG_TRUE)
		cgGLSetParameter4fv(p, Vector(0, 1, 0.001f, 0.5f));
}

#ifndef DEVBUILD

#define LOAD_VS(Index, prog) {						  \
	assert( mapShaderResources.find(Index) != mapShaderResources.end() ); \
	header = mapShaderResources[Index]; \
	assert( (header) != NULL && (header)->index == (Index) ); \
	prog = cgCreateProgram(g_cgcontext, CG_OBJECT, (char*)(s_lpShaderResources + (header)->offset), cgvProf, NULL, NULL); \
	if( !cgIsProgram(prog) ) { \
		ZZLog::Error_Log("Failed to load vs %d: \n%s.", Index, cgGetLastListing(g_cgcontext)); \
		return false; \
	} \
	cgGLLoadProgram(prog); \
	if( cgGetError() != CG_NO_ERROR ) ZZLog::Error_Log("failed to load program %d.", Index); \
	SetupVertexProgramParameters(prog, !!(Index&SH_CONTEXT1));			\
} \
 
#define LOAD_PS(Index, fragment) {  \
	bLoadSuccess = true; \
	assert( mapShaderResources.find(Index) != mapShaderResources.end() ); \
	header = mapShaderResources[Index]; \
	fragment.prog = cgCreateProgram(g_cgcontext, CG_OBJECT, (char*)(s_lpShaderResources + (header)->offset), cgfProf, NULL, NULL); \
	if( !cgIsProgram(fragment.prog) ) { \
		ZZLog::Error_Log("Failed to load ps %d: \n%s.", Index, cgGetLastListing(g_cgcontext)); \
		return false; \
	} \
	cgGLLoadProgram(fragment.prog); \
	if( cgGetError() != CG_NO_ERROR ) { \
		ZZLog::Error_Log("failed to load program %d.", Index);		   \
		bLoadSuccess = false; \
	} \
	SetupFragmentProgramParameters(&fragment, !!(Index&SH_CONTEXT1), 0);  \
} \
 
bool ZeroGS::LoadEffects()
{
	assert(s_lpShaderResources != NULL);

	// process the header
	u32 num = *(u32*)s_lpShaderResources;
	int compressed_size = *(int*)(s_lpShaderResources + 4);
	int real_size = *(int*)(s_lpShaderResources + 8);
	int out;

	char* pbuffer = (char*)malloc(real_size);
	inf((char*)s_lpShaderResources + 12, &pbuffer[0], compressed_size, real_size, &out);
	assert(out == real_size);

	s_lpShaderResources = (u8*)pbuffer;
	SHADERHEADER* header = (SHADERHEADER*)s_lpShaderResources;

	mapShaderResources.clear();

	while (num-- > 0)
	{
		mapShaderResources[header->index] = header;
		++header;
	}

	// clear the textures
	for (int i = 0; i < ARRAY_SIZE(ppsTexture); ++i)
	{
		SAFE_RELEASE_PROG(ppsTexture[i].prog);
		ppsTexture[i].prog = NULL;
	}

#ifndef _DEBUG
	memset(ppsTexture, 0, sizeof(ppsTexture));

#endif

	return true;
}

// called
bool ZeroGS::LoadExtraEffects()
{
	SHADERHEADER* header;
	bool bLoadSuccess = true;

	const int vsshaders[4] = { SH_REGULARVS, SH_TEXTUREVS, SH_REGULARFOGVS, SH_TEXTUREFOGVS };

	for (int i = 0; i < 4; ++i)
	{
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

	if (conf.mrtdepth)
	{
		LOAD_PS(SH_REGULARPS, ppsRegular[2]);

		if (!bLoadSuccess)
			conf.mrtdepth = 0;

		LOAD_PS(SH_REGULARFOGPS, ppsRegular[3]);

		if (!bLoadSuccess)
			conf.mrtdepth = 0;
	}

	LOAD_PS(SH_BITBLTPS, ppsBitBlt[0]);

	LOAD_PS(SH_BITBLTAAPS, ppsBitBlt[1]);

	if (!bLoadSuccess)
	{
		ZZLog::Error_Log("Failed to load BitBltAAPS, using BitBltPS.");
		LOAD_PS(SH_BITBLTPS, ppsBitBlt[1]);
	}

	LOAD_PS(SH_BITBLTDEPTHPS, ppsBitBltDepth);

	LOAD_PS(SH_CRTCTARGPS, ppsCRTCTarg[0]);
	LOAD_PS(SH_CRTCTARGINTERPS, ppsCRTCTarg[1]);

	g_bCRTCBilinear = true;
	LOAD_PS(SH_CRTCPS, ppsCRTC[0]);

	if (!bLoadSuccess)
	{
		// switch to simpler
		g_bCRTCBilinear = false;
		LOAD_PS(SH_CRTC_NEARESTPS, ppsCRTC[0]);
		LOAD_PS(SH_CRTCINTER_NEARESTPS, ppsCRTC[0]);
	}
	else
	{
		LOAD_PS(SH_CRTCINTERPS, ppsCRTC[1]);
	}

	if (!bLoadSuccess)
		ZZLog::Error_Log("Failed to create CRTC shaders.");

	LOAD_PS(SH_CRTC24PS, ppsCRTC24[0]);
	LOAD_PS(SH_CRTC24INTERPS, ppsCRTC24[1]);
	LOAD_PS(SH_ZEROPS, ppsOne);
	LOAD_PS(SH_BASETEXTUREPS, ppsBaseTexture);
	LOAD_PS(SH_CONVERT16TO32PS, ppsConvert16to32);
	LOAD_PS(SH_CONVERT32TO16PS, ppsConvert32to16);

	return true;
}

FRAGMENTSHADER* ZeroGS::LoadShadeEffect(int type, int texfilter, int fog, int testaem, int exactcolor, const clampInfo& clamp, int context, bool* pbFailed)
{
	int texwrap;
	assert(texfilter < NUM_FILTERS);

	if (g_nPixelShaderVer & SHADER_REDUCED) texfilter = 0;

	assert(!(g_nPixelShaderVer & SHADER_REDUCED) || !exactcolor);

	if (clamp.wms == clamp.wmt)
	{
		switch (clamp.wms)
		{
			case 0:
				texwrap = TEXWRAP_REPEAT;
				break;

			case 1:
				texwrap = TEXWRAP_CLAMP;
				break;

			case 2:
				texwrap = TEXWRAP_CLAMP;
				break;

			default:
				texwrap = TEXWRAP_REGION_REPEAT;
				break;
		}
	}
	else if (clamp.wms == 3 || clamp.wmt == 3)
		texwrap = TEXWRAP_REGION_REPEAT;
	else
		texwrap = TEXWRAP_REPEAT_CLAMP;

	int index = GET_SHADER_INDEX(type, texfilter, texwrap, fog, s_bWriteDepth, testaem, exactcolor, context, 0);

	assert(index < ARRAY_SIZE(ppsTexture));

	FRAGMENTSHADER* pf = ppsTexture + index;

	if (pbFailed != NULL) *pbFailed = false;

	if (pf->prog != NULL) return pf;

	if ((g_nPixelShaderVer & SHADER_ACCURATE) && mapShaderResources.find(index + NUM_SHADERS*SHADER_ACCURATE) != mapShaderResources.end())
		index += NUM_SHADERS * SHADER_ACCURATE;

	assert(mapShaderResources.find(index) != mapShaderResources.end());

	SHADERHEADER* header = mapShaderResources[index];

	if (header == NULL) ZZLog::Error_Log("%d %d", index, g_nPixelShaderVer);

	assert(header != NULL);

	//ZZLog::Debug_Log("Shader:\n%s.", (char*)(s_lpShaderResources + (header)->offset));
	pf->prog = cgCreateProgram(g_cgcontext, CG_OBJECT, (char*)(s_lpShaderResources + (header)->offset), cgfProf, NULL, NULL);

	if (pf->prog != NULL && cgIsProgram(pf->prog) && cgGetError() == CG_NO_ERROR)
	{
		SetupFragmentProgramParameters(pf, context, type);
		cgGLLoadProgram(pf->prog);

		if (cgGetError() != CG_NO_ERROR)
		{
//		  cgGLLoadProgram(pf->prog);
//		  if( cgGetError() != CG_NO_ERROR ) {
			ZZLog::Error_Log("Failed to load shader %d,%d,%d,%d.", type, fog, texfilter, 4*clamp.wms + clamp.wmt);

			if (pbFailed != NULL) *pbFailed = true;

			return pf;

//		  }
		}
		return pf;
	}

	ZZLog::Error_Log("Failed to create shader %d,%d,%d,%d", type, fog, texfilter, 4*clamp.wms + clamp.wmt);

	if (pbFailed != NULL) *pbFailed = true;

	return NULL;
}

#else // defined(ZEROGS_DEVBUILD)

#define LOAD_VS(name, prog, shaderver) { \
	prog = cgCreateProgramFromFile(g_cgcontext, CG_SOURCE, EFFECT_NAME, shaderver, name, args); \
	if( !cgIsProgram(prog) ) { \
		ZZLog::Error_Log("Failed to load vs %s: \n%s.", name, cgGetLastListing(g_cgcontext)); \
		return false; \
	} \
	cgGLLoadProgram(prog); \
	if( cgGetError() != CG_NO_ERROR ) ZZLog::Error_Log("failed to load program %s.", name); \
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
		ZZLog::Error_Log("Failed to load ps %s: \n%s.", name, cgGetLastListing(g_cgcontext)); \
		return false; \
	} \
	cgGLLoadProgram(fragment.prog); \
	if( cgGetError() != CG_NO_ERROR ) { \
		ZZLog::Error_Log("failed to load program %s.", name);		   \
		bLoadSuccess = false; \
	} \
	SetupFragmentProgramParameters(&fragment, args[0]==context1, 0);  \
	SET_PSFILENAME(fragment, name); \
} \
 
bool ZeroGS::LoadEffects()
{
	// clear the textures
	for (int i = 0; i < ARRAY_SIZE(ppsTexture); ++i)
	{
		SAFE_RELEASE_PROG(ppsTexture[i].prog);
	}

#ifndef _DEBUG
	memset(ppsTexture, 0, sizeof(ppsTexture));

#endif

	return true;
}

bool ZeroGS::LoadExtraEffects()
{
	const char* args[] = { NULL , NULL, NULL, NULL };
	char context0[255], context1[255];
	sprintf(context0, "-I%sctx0", EFFECT_DIR);
	sprintf(context1, "-I%sctx1", EFFECT_DIR);
	char* write_depth = "-DWRITE_DEPTH";
	bool bLoadSuccess = true;

	const char* pvsshaders[4] = { "RegularVS", "TextureVS", "RegularFogVS", "TextureFogVS" };

	for (int i = 0; i < 4; ++i)
	{
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

	if (conf.mrtdepth)
	{
		args[0] = context0;
		args[1] = write_depth;
		LOAD_PS("RegularPS", ppsRegular[2], cgfProf);

		if (!bLoadSuccess) conf.mrtdepth = 0;

		LOAD_PS("RegularFogPS", ppsRegular[3], cgfProf);

		if (!bLoadSuccess) conf.mrtdepth = 0;
	}

	LOAD_PS("BitBltPS", ppsBitBlt[0], cgfProf);
	LOAD_PS("BitBltAAPS", ppsBitBlt[1], cgfProf);

	if (!bLoadSuccess)
	{
		ZZLog::Error_Log("Failed to load BitBltAAPS, using BitBltPS.");
		LOAD_PS("BitBltPS", ppsBitBlt[1], cgfProf);
	}

	LOAD_PS("BitBltDepthPS", ppsBitBltDepth, cgfProf);
	LOAD_PS("CRTCTargPS", ppsCRTCTarg[0], cgfProf);
	LOAD_PS("CRTCTargInterPS", ppsCRTCTarg[1], cgfProf);

	g_bCRTCBilinear = true;
	LOAD_PS("CRTCPS", ppsCRTC[0], cgfProf);

	if (!bLoadSuccess)
	{
		// switch to simpler
		g_bCRTCBilinear = false;
		LOAD_PS("CRTCPS_Nearest", ppsCRTC[0], cgfProf);
		LOAD_PS("CRTCInterPS_Nearest", ppsCRTC[0], cgfProf);
	}
	else
	{
		LOAD_PS("CRTCInterPS", ppsCRTC[1], cgfProf);
	}

	if (!bLoadSuccess) ZZLog::Error_Log("Failed to create CRTC shaders.");

	LOAD_PS("CRTC24PS", ppsCRTC24[0], cgfProf);
	LOAD_PS("CRTC24InterPS", ppsCRTC24[1], cgfProf);
	LOAD_PS("ZeroPS", ppsOne, cgfProf);
	LOAD_PS("BaseTexturePS", ppsBaseTexture, cgfProf);
	LOAD_PS("Convert16to32PS", ppsConvert16to32, cgfProf);
	LOAD_PS("Convert32to16PS", ppsConvert32to16, cgfProf);

//	if( !conf.mrtdepth ) {
//		ZZLog::Error_Log("Disabling MRT depth writing.");
//		s_bWriteDepth = false;
//	}

	return true;
}

FRAGMENTSHADER* ZeroGS::LoadShadeEffect(int type, int texfilter, int fog, int testaem, int exactcolor, const clampInfo& clamp, int context, bool* pbFailed)
{
	int texwrap;

	assert(texfilter < NUM_FILTERS);
	//assert( g_nPixelShaderVer == SHADER_30 );

	if (clamp.wms == clamp.wmt)
	{
		switch (clamp.wms)
		{
			case 0:
				texwrap = TEXWRAP_REPEAT;
				break;

			case 1:
				texwrap = TEXWRAP_CLAMP;
				break;

			case 2:
				texwrap = TEXWRAP_CLAMP;
				break;

			default:
				texwrap = TEXWRAP_REGION_REPEAT;
				break;
		}
	}
	else if (clamp.wms == 3 || clamp.wmt == 3)
		texwrap = TEXWRAP_REGION_REPEAT;
	else
		texwrap = TEXWRAP_REPEAT_CLAMP;

	int index = GET_SHADER_INDEX(type, texfilter, texwrap, fog, s_bWriteDepth, testaem, exactcolor, context, 0);

	if (pbFailed != NULL) *pbFailed = false;

	FRAGMENTSHADER* pf = ppsTexture + index;

	if (pf->prog != NULL) return pf;

	pf->prog = LoadShaderFromType(EFFECT_DIR, EFFECT_NAME, type, texfilter, texwrap, fog, s_bWriteDepth, testaem, exactcolor, g_nPixelShaderVer, context);

	if (pf->prog != NULL)
	{
#ifdef _DEBUG
		char str[255];
		sprintf(str, "Texture%s%d_%sPS", fog ? "Fog" : "", texfilter, g_pTexTypes[type]);
		pf->filename = str;
#endif
		SetupFragmentProgramParameters(pf, context, type);
		cgGLLoadProgram(pf->prog);

		if (cgGetError() != CG_NO_ERROR)
		{
			// try again
//			cgGLLoadProgram(pf->prog);
//			if( cgGetError() != CG_NO_ERROR ) {
			ZZLog::Error_Log("Failed to load shader %d,%d,%d,%d.", type, fog, texfilter, 4*clamp.wms + clamp.wmt);

			if (pbFailed != NULL) *pbFailed = true;

			//assert(0);
			// NULL makes things crash
			return pf;

//			}
		}

		return pf;
	}

	ZZLog::Error_Log("Failed to create shader %d,%d,%d,%d.", type, fog, texfilter, 4*clamp.wms + clamp.wmt);

	if (pbFailed != NULL) *pbFailed = true;

	return NULL;
}

#endif // !defined(ZEROGS_DEVBUILD)

