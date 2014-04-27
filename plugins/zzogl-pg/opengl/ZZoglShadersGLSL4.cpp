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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifdef GLSL4_API 		// This code is only for GLSL API
// ZZogl Shader manipulation functions.

/*
 * used cg calls:
 * cgGLIsProfileSupported		-- don't needed
 * cgGetErrorString 			-- later
 * cgGetLastListing			-- later
 * cgSetErrorHandler			-- later
 * cgCreateContext			-- think that don't need
 * cgGLEnableProfile			-- don't need
 * cgGLSetOptimalOptions		-- don't need?
 * cgGLSetManageTextureParameters	-- what's this?
 * cgCreateParameter			-- don't need
 * cgGLLoadProgram			void LinkProgram(uint program)
 * cgGetError				-- later
 * cgGLDisableProfile			-- don't need
 * cgGLSetParameter4fv
 * cgGetNamedParameter
 * cgGLEnableTextureParameter
 * cgIsParameterUsed
 * cgGLBindProgram			void UseProgram(uint program)
 * cgConnectParameter
 * cgIsProgram				bool IsProgram(uint program)
 * cgCreateProgramFromFile
 */

//------------------- Includes
#include "Util.h"
#include "ZZoglShaders.h"
#include "zpipe.h"
#include <map>
#include <fcntl.h>			// this for open(). Maybe linux-specific
#include "ps2hw_gl4.h"

// ----------------- Defines

#define TEXWRAP_REPEAT 0
#define TEXWRAP_CLAMP 1
#define TEXWRAP_REGION_REPEAT 2
#define TEXWRAP_REPEAT_CLAMP 3

#ifdef DEVBUILD
#	define UNIFORM_ERROR_LOG 	ZZLog::Error_Log
#else
#	define UNIFORM_ERROR_LOG
#endif

// Set it to 0 to diable context usage, 1 -- to enable. FFX-1 have a strange issue with ClampExt.
#define NOCONTEXT		0
#define NUMBER_OF_SAMPLERS 	11
#define MAX_SHADER_NAME_SIZE	25
#define DEFINE_STRING_SIZE	256

// #define ENABLE_MARKER // Fire some marker for opengl Debugger (apitrace, gdebugger)
//------------------ Constants

const static char* g_pTexTypes[] = { "32", "tex32", "clut32", "tex32to16", "tex16to8h" };
const static char* g_pPsTexWrap[] = { "#define REPEAT 1\n", "#define CLAMP 1\n", "#define REGION_REPEAT 1\n", "\n" };
const int GLSL_VERSION = 330;


// ----------------- Global Variables

ZZshContext	g_cgcontext;
ZZshProfile 	cgvProf, cgfProf;
int 		g_nPixelShaderVer = 0; 		// default
u8* 		s_lpShaderResources = NULL;
ZZshShaderLink 	pvs[16] = {sZero}, g_vsprog = sZero, g_psprog = sZero;							// 2 -- ZZ
ZZshParameter 	g_vparamPosXY[2] = {pZero}, g_fparamFogColor = pZero;

bool g_bCRTCBilinear = true;

float4 g_vdepth, vlogz;
FRAGMENTSHADER ppsBitBlt[2], ppsBitBltDepth, ppsOne;
FRAGMENTSHADER ppsBaseTexture, ppsConvert16to32, ppsConvert32to16;
FRAGMENTSHADER ppsRegular[4], ppsTexture[NUM_SHADERS];
FRAGMENTSHADER ppsCRTC[2], /*ppsCRTC24[2],*/ ppsCRTCTarg[2];
VERTEXSHADER pvsStore[16];
VERTEXSHADER pvsBitBlt;

inline bool LoadEffects();
extern bool s_bWriteDepth;

// Debug variable, store name of the function that call the shader.
const char* ShaderCallerName = "";
const char* ShaderHandleName = "";

// new for GLSL4
GSUniformBufferOGL *constant_buffer;
GSUniformBufferOGL *common_buffer;
GSUniformBufferOGL *vertex_buffer;
GSUniformBufferOGL *fragment_buffer;
static bool dirty_common_buffer = true;
static bool dirty_vertex_buffer = true;
static bool dirty_fragment_buffer = true;

GSVertexBufferStateOGL *vertex_array = NULL;

COMMONSHADER g_cs;
static GLuint s_pipeline = 0;

uint g_current_texture_bind[11] = {0};
GLenum g_current_vs = 0;
GLenum g_current_ps = 0;

FRAGMENTSHADER ppsDebug;
FRAGMENTSHADER ppsDebug2;
FRAGMENTSHADER ppsDebug3;

//------------------ Code

inline int GET_SHADER_INDEX(int type, int texfilter, int texwrap, int fog, int writedepth, int testaem, int exactcolor, int context, int ps) {
	return type + texfilter*NUM_TYPES + NUM_FILTERS*NUM_TYPES*texwrap + NUM_TEXWRAPS*NUM_FILTERS*NUM_TYPES*(fog+2*writedepth+4*testaem+8*exactcolor+16*context+32*ps) ;
}

// Nothing need to be done.
bool ZZshCheckProfilesSupport() {
	return true;
}

bool ZZshStartUsingShaders() {

	ZZLog::Error_Log("Creating effects.");
	B_G(LoadEffects(), return false);
	if (!glCreateShader)
	{
		ZZLog::Error_Log("GLSL shaders is not supported, stop.");
		return false;
	}

	init_shader();

	// create a sample shader
	clampInfo temp;
	memset(&temp, 0, sizeof(temp));
	temp.wms = 3; temp.wmt = 3;

	// test
	bool bFailed;
	FRAGMENTSHADER* pfrag = ZZshLoadShadeEffect(0, 1, 1, 1, 1, temp, 0, &bFailed);
	if( bFailed || pfrag == NULL ) {
		return false;
		ZZLog::Error_Log("Shader test failed.");
	}

	ZZLog::Error_Log("Creating extra effects.");
	B_G(ZZshLoadExtraEffects(), return false);


	return true;
}

// open shader file according to build target
bool ZZshCreateOpenShadersFile() {
	return true;
}

void ZZshExitCleaning() {
	delete constant_buffer;
	delete common_buffer;
	delete vertex_buffer;
	delete fragment_buffer;

	dirty_fragment_buffer = true;
	dirty_vertex_buffer = true;
	dirty_common_buffer = true;
	g_current_ps = 0;
	g_current_vs = 0;
	for (uint i = 0; i < 11; i++)
		g_current_texture_bind[i] = 0;

	glDeleteProgramPipelines(1, &s_pipeline);
}

// Disable CG
void ZZshGLDisableProfile() {			// This stop all other shader programs from running;
	glBindProgramPipeline(0);
}
//Enable CG
void ZZshGLEnableProfile() {
	glBindProgramPipeline(s_pipeline);
}
//-------------------------------------------------------------------------------------

// The same function for texture, also to cgGLEnable
void ZZshGLSetTextureParameter(ZZshParameter param, GLuint texobj, const char* name) {
#ifdef ENABLE_MARKER
	if (GLEW_GREMEDY_string_marker) glStringMarkerGREMEDY(0, format("CS: texture %d, param %d", texobj, param).c_str() );
#endif

	g_cs.set_texture(param, texobj);
}

void ZZshGLSetTextureParameter(ZZshShaderLink prog, ZZshParameter param, GLuint texobj, const char* name) {
	FRAGMENTSHADER* shader = (FRAGMENTSHADER*)prog.link;
#ifdef ENABLE_MARKER
	if (GLEW_GREMEDY_string_marker) glStringMarkerGREMEDY(0, format("FS(%d):texture %d, param %d", shader->program, texobj, param).c_str() );
#endif

	shader->set_texture(param, texobj);
}

// This is helper of cgGLSetParameter4fv, made for debug purpose.
// Name could be any string. We must use it on compilation time, because erroneus handler does not
// return name
void ZZshSetParameter4fv(ZZshShaderLink& prog, ZZshParameter param, const float* v, const char* name) {
	if (prog.isFragment) {
		FRAGMENTSHADER* shader = (FRAGMENTSHADER*)prog.link;
		shader->ZZshSetParameter4fv(param, v);
		dirty_fragment_buffer = true;
	} else {
		VERTEXSHADER* shader = (VERTEXSHADER*)prog.link;
		shader->ZZshSetParameter4fv(param, v);
		dirty_vertex_buffer = true;
	}
#ifdef ENABLE_MARKER
	if (GLEW_GREMEDY_string_marker) glStringMarkerGREMEDY(0, format("prog: uniform (%s) (%f)", name, *v).c_str() );
#endif
}

void ZZshSetParameter4fv(ZZshParameter param, const float* v, const char* name) {
	g_cs.ZZshSetParameter4fv(param, v);
	dirty_common_buffer = true;
#ifdef ENABLE_MARKER
	if (GLEW_GREMEDY_string_marker) glStringMarkerGREMEDY(0, format("CS: uniform (%s) (%f)", name, *v).c_str() );
#endif
}

// The same stuff, but also with retry of param, name should be USED name of param for prog.
void ZZshSetParameter4fvWithRetry(ZZshParameter* param, ZZshShaderLink& prog, const float* v, const char* name) {
	ZZshSetParameter4fv(prog, *param, v, name);
}

// Used sometimes for color 1.
void ZZshDefaultOneColor( FRAGMENTSHADER& ptr ) {
	ShaderHandleName = "Set Default One colot";
	float4 v = float4 ( 1, 1, 1, 1 );
	ptr.ZZshSetParameter4fv(ptr.sOneColor, v);
	dirty_fragment_buffer = true;
}
//-------------------------------------------------------------------------------------

static bool ValidateProgram(ZZshProgram Prog) {
	GLint isValid;

	glValidateProgram(Prog);
	glGetProgramiv(Prog, GL_VALIDATE_STATUS, &isValid);

	if (!isValid) {
		int lenght, infologlength;
		glGetProgramiv(Prog, GL_INFO_LOG_LENGTH, &infologlength);
		char* InfoLog = new char[infologlength];
		glGetProgramInfoLog(Prog, infologlength, &lenght, InfoLog);
		ZZLog::Error_Log("Validation %d... %d:\t %s", Prog, infologlength, InfoLog);
		delete[] InfoLog;
	}
	return (isValid != 0);
}

static void ValidatePipeline(GLuint pipeline) {
	glValidateProgramPipeline(pipeline);
	GLint isValid;
	glGetProgramPipelineiv(pipeline, GL_VALIDATE_STATUS, &isValid);
	if (!isValid) {
		int lenght, infologlength;
		glGetProgramPipelineiv(pipeline, GL_INFO_LOG_LENGTH, &infologlength);
		char* InfoLog = new char[infologlength];
		glGetProgramPipelineInfoLog(pipeline, infologlength, &lenght, InfoLog);
		ZZLog::Error_Log("Validation %d... %d:\t %s", pipeline, infologlength, InfoLog);
		delete[] InfoLog;
	}
}

inline bool CompileShaderFromFile(ZZshProgram& program, const std::string& DefineString, std::string main_entry, GLenum ShaderType)
{
	std::string header("");

	header += format("#version %d\n", GLSL_VERSION);
	header += format("#define %s main\n", main_entry.c_str());
	if (ShaderType == GL_VERTEX_SHADER) header += "#define VERTEX_SHADER 1\n";
	else if (ShaderType == GL_FRAGMENT_SHADER) header += "#define FRAGMENT_SHADER 1\n";
	header += DefineString;

	const GLchar* ShaderSource[2];

#if 0
	// It sucks because it doesn't report the good line for error/warnings!
	// But at least this stupid AMD drivers doesn't crash...
	ShaderSource[0] = header.append(ps2hw_gl4_glsl).c_str();
	program = glCreateShaderProgramv(ShaderType, 1, &ShaderSource[0]);

#else
	ShaderSource[0] = header.c_str();
	ShaderSource[1] = ps2hw_gl4_glsl;

	program = glCreateShaderProgramv(ShaderType, 2, &ShaderSource[0]);
#endif

	ZZLog::Debug_Log("Creating program %d for %s", program, main_entry.c_str());

#if	defined(DEVBUILD) || defined(_DEBUG)
	if (!ValidateProgram(program)) return false;
#endif

	return true;

}

//-------------------------------------------------------------------------------------

void ZZshSetupShader() {
	VERTEXSHADER* vs = (VERTEXSHADER*)g_vsprog.link;
	FRAGMENTSHADER* ps = (FRAGMENTSHADER*)g_psprog.link;

	if (vs == NULL || ps == NULL) return;

	// From the glValidateProgram docs: "The implementation may use this as an opportunity to perform any internal
	// shader modifications that may be required to ensure correct operation of the installed
	// shaders given the current GL state"
	// It might be a good idea to validate the pipeline also in release mode???
#if	defined(DEVBUILD) || defined(_DEBUG)
	ValidatePipeline(s_pipeline);
#endif

	PutParametersInProgram(vs, ps);
	GL_REPORT_ERRORD();
}

void ZZshSetVertexShader(ZZshShaderLink prog) {
	g_vsprog = prog;

	VERTEXSHADER* vs = (VERTEXSHADER*)g_vsprog.link;
	if (!vs) return;

	if (vs->program != g_current_vs) {
		glUseProgramStages(s_pipeline, GL_VERTEX_SHADER_BIT, vs->program);
		g_current_vs = vs->program;
	}
}

void ZZshSetPixelShader(ZZshShaderLink prog) {
	g_psprog = prog;

	FRAGMENTSHADER* ps = (FRAGMENTSHADER*)g_psprog.link;
	if (!ps) return;

	if (ps->program != g_current_ps) {
		glUseProgramStages(s_pipeline, GL_FRAGMENT_SHADER_BIT, ps->program);
		g_current_ps = ps->program;
	}
}

//------------------------------------------------------------------------------------------------------------------

void init_shader() {
	// TODO:
	// Note it would be more clever to allocate buffer inside SHADER class
	// Add a dirty flags to avoid to upload twice same data...
	// You need to attach() properly the uniform buffer;
	// Note: don't put GSUniformBuffer creation inside constructor of static object (context won't 
	// be set to call gl command)
	
	// Warning put same order than GLSL
	constant_buffer = new GSUniformBufferOGL(0, sizeof(ConstantUniform));
	common_buffer = new GSUniformBufferOGL(1, sizeof(GlobalUniform));
	vertex_buffer = new GSUniformBufferOGL(2, sizeof(VertexUniform));
	fragment_buffer = new GSUniformBufferOGL(3, sizeof(FragmentUniform));

	constant_buffer->bind();
	constant_buffer->upload((void*)&g_cs.uniform_buffer_constant);

	g_cs.set_texture(g_cs.sBlocks, ptexBlocks);
	g_cs.set_texture(g_cs.sConv16to32, ptexConv16to32);
	g_cs.set_texture(g_cs.sConv32to16, ptexConv32to16);
	g_cs.set_texture(g_cs.sBilinearBlocks, ptexBilinearBlocks);
	
	glGenProgramPipelines(1, &s_pipeline);
	glBindProgramPipeline(s_pipeline);


	// FIXME
	// In the future it would be better to use directly the common shader
	g_vparamPosXY[0] = g_cs.g_vparamPosXY;
	g_vparamPosXY[1] = g_cs.g_vparamPosXY;
	g_fparamFogColor = g_cs.g_fparamFogColor;
}

void PutParametersInProgram(VERTEXSHADER* vs, FRAGMENTSHADER* ps) {

	if (dirty_common_buffer) {
		common_buffer->bind();
		common_buffer->upload((void*)&g_cs.uniform_buffer[g_cs.context]);
		dirty_common_buffer = false;
	}

	if (dirty_vertex_buffer) {
		vertex_buffer->bind();
		vertex_buffer->upload((void*)&vs->uniform_buffer[vs->context]);
		dirty_vertex_buffer = false;
	}

	if (dirty_fragment_buffer) {
		fragment_buffer->bind();
		fragment_buffer->upload((void*)&ps->uniform_buffer[ps->context]);
		dirty_fragment_buffer = false;
	}

#ifdef ENABLE_MARKER
	if (GLEW_GREMEDY_string_marker) glStringMarkerGREMEDY(0, format("FS(%d): enable texture", ps->program).c_str() );
#endif

	g_cs.enable_texture();
	ps->enable_texture();
	// By default enable the unit 0, so I have the guarantee that any
	// texture command won't change current binding of others unit
	glActiveTexture(GL_TEXTURE0);
}

std::string BuildGlslMacro(bool writedepth, int texwrap = 3, bool testaem = false, bool exactcolor = false)
{
	std::string header("");

	if (writedepth) header += "#define WRITE_DEPTH 1\n";
	if (testaem)	header += "#define TEST_AEM 1\n";
	if (exactcolor) header += "#define EXACT_COLOR 1\n";
	header += format("%s", g_pPsTexWrap[texwrap]);
	//const char* AddAccurate  = (ps & SHADER_ACCURATE)?"#define ACCURATE_DECOMPRESSION 1\n":"";
	if (conf.settings().no_logz) {
		header += "#define NO_LOGZ 1\n";
	}

	return header;
}

static __forceinline bool LOAD_VS(const std::string& DefineString, const char* name, VERTEXSHADER& vertex, ZZshProfile context)
{
	bool flag = CompileShaderFromFile(vertex.program, DefineString, name, GL_VERTEX_SHADER);
	vertex.set_context(context);
	return flag;
}

static __forceinline bool LOAD_PS(const std::string& DefineString, const char* name, FRAGMENTSHADER& fragment, ZZshProfile context)
{
	bool flag = CompileShaderFromFile(fragment.program, DefineString, name, GL_FRAGMENT_SHADER);
	fragment.set_context(context);
	return flag;
}

inline bool LoadEffects()
{
	// clear the textures
	for(u32 i = 0; i < ArraySize(ppsTexture); ++i)
		ppsTexture[i].release_prog();

	return true;
}

bool ZZshLoadExtraEffects() {
	bool bLoadSuccess = true;

	std::string depth_macro = BuildGlslMacro(true);
	std::string empty_macro = BuildGlslMacro(false);

	// DEBUG
	// Put them first so it is easier to get their program index in apitrace. Namely 3,4,5
	if (!LOAD_PS(empty_macro, "ZeroDebugPS", ppsDebug,  0)) bLoadSuccess = false;
	if (!LOAD_PS(empty_macro, "ZeroDebug2PS", ppsDebug2,  0)) bLoadSuccess = false;
	//if (!LOAD_PS(empty_macro, "ZeroDebug3PS", ppsDebug3,  0)) bLoadSuccess = false;

	const char* pvsshaders[4] = { "RegularVS", "TextureVS", "RegularFogVS", "TextureFogVS" };

	for (int i = 0; i < 4; ++i) {
		if (!LOAD_VS(empty_macro, pvsshaders[i], pvsStore[2 * i], 0)) bLoadSuccess = false;
		if (!LOAD_VS(empty_macro, pvsshaders[i], pvsStore[2 *i + 1 ], 1)) bLoadSuccess = false;
		if (!LOAD_VS(depth_macro, pvsshaders[i], pvsStore[2 *i + 8 ], 0)) bLoadSuccess = false;
		if (!LOAD_VS(depth_macro, pvsshaders[i], pvsStore[2 *i + 8 + 1], 1)) bLoadSuccess = false;
	}
	for (int i = 0; i < 16; ++i)
		pvs[i] = pvsStore[i].prog;

	if (!LOAD_VS(empty_macro, "BitBltVS", pvsBitBlt, 0)) bLoadSuccess = false;
	GL_REPORT_ERRORD();

	if (!LOAD_PS(empty_macro, "RegularPS", ppsRegular[0],  0)) bLoadSuccess = false;
	if (!LOAD_PS(empty_macro, "RegularFogPS", ppsRegular[1],  0)) bLoadSuccess = false;

	if( conf.mrtdepth ) {
		if (!LOAD_PS(depth_macro, "RegularPS", ppsRegular[2],  0)) bLoadSuccess = false;
		if (!bLoadSuccess) conf.mrtdepth = 0;

		if (!LOAD_PS(depth_macro, "RegularFogPS", ppsRegular[3],  0)) bLoadSuccess = false;
		if (!bLoadSuccess) conf.mrtdepth = 0;
	}

	if (!LOAD_PS(empty_macro, "BitBltPS", ppsBitBlt[0],  0)) bLoadSuccess = false;
	if (!LOAD_PS(empty_macro, "BitBltAAPS", ppsBitBlt[1],  0)) bLoadSuccess = false;
	if (!bLoadSuccess) {
		ZZLog::Error_Log("Failed to load BitBltAAPS, using BitBltPS.");
		if (!LOAD_PS(empty_macro, "BitBltPS", ppsBitBlt[1],  0)) bLoadSuccess = false;
	}

	if (!LOAD_PS(empty_macro, "BitBltDepthPS", ppsBitBltDepth,  0)) bLoadSuccess = false;
	if (!LOAD_PS(empty_macro, "CRTCTargPS", ppsCRTCTarg[0],  0)) bLoadSuccess = false;
	if (!LOAD_PS(empty_macro, "CRTCTargInterPS", ppsCRTCTarg[1],  0)) bLoadSuccess = false;

	g_bCRTCBilinear = true;
	if (!LOAD_PS(empty_macro, "CRTCPS", ppsCRTC[0],  0)) bLoadSuccess = false;
	if( !bLoadSuccess ) {
		// switch to simpler
		g_bCRTCBilinear = false;
		if (!LOAD_PS(empty_macro, "CRTCPS_Nearest", ppsCRTC[0],  0)) bLoadSuccess = false;
		if (!LOAD_PS(empty_macro, "CRTCInterPS_Nearest", ppsCRTC[0],  0)) bLoadSuccess = false;
	}
	else {
		if (!LOAD_PS(empty_macro, "CRTCInterPS", ppsCRTC[1],  0)) bLoadSuccess = false;
	}

	if( !bLoadSuccess )
		ZZLog::Error_Log("Failed to create CRTC shaders.");

	// if (!LOAD_PS(empty_macro, "CRTC24PS", ppsCRTC24[0],  0)) bLoadSuccess = false;
	// if (!LOAD_PS(empty_macro, "CRTC24InterPS", ppsCRTC24[1],  0)) bLoadSuccess = false;
	if (!LOAD_PS(empty_macro, "ZeroPS", ppsOne,  0)) bLoadSuccess = false;
	if (!LOAD_PS(empty_macro, "BaseTexturePS", ppsBaseTexture,  0)) bLoadSuccess = false;
	if (!LOAD_PS(empty_macro, "Convert16to32PS", ppsConvert16to32,  0)) bLoadSuccess = false;
	if (!LOAD_PS(empty_macro, "Convert32to16PS", ppsConvert32to16,  0)) bLoadSuccess = false;

	GL_REPORT_ERRORD();
	return true;
}

FRAGMENTSHADER* ZZshLoadShadeEffect(int type, int texfilter, int fog, int testaem, int exactcolor, const clampInfo& clamp, int context, bool* pbFailed)
{
	int texwrap;

	assert( texfilter < NUM_FILTERS );
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

	int index = GET_SHADER_INDEX(type, texfilter, texwrap, fog, s_bWriteDepth, testaem, exactcolor, 0, 0);

	if( pbFailed != NULL ) *pbFailed = false;

	FRAGMENTSHADER* pf = ppsTexture+index;

	if (ZZshExistProgram(pf))
	{
		return pf;
	}

	std::string macro = BuildGlslMacro(s_bWriteDepth, texwrap, testaem, exactcolor);
	std::string main_entry  = format("Texture%s%d_%sPS", fog?"Fog":"", texfilter, g_pTexTypes[type]);

	pf->set_context(context);
	if (!CompileShaderFromFile(pf->program, macro, main_entry, GL_FRAGMENT_SHADER)) {
		ZZLog::Error_Log("Failed to create shader %d,%d,%d,%d.", type, fog, texfilter, 4*clamp.wms+clamp.wmt);
		if( pbFailed != NULL ) *pbFailed = false;
		return NULL;
	}
	return pf;
}

#endif // GLSL4_API
