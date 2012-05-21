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
#include <math.h>
#include <map>
#include  <fcntl.h>			// this for open(). Maybe linux-specific
#include <sys/mman.h>			// and this for mmap

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

// Used in a logarithmic Z-test, as (1-o(1))/log(MAX_U32).
const float g_filog32 = 0.999f / (32.0f * logf(2.0f));

const static char* g_pTexTypes[] = { "32", "tex32", "clut32", "tex32to16", "tex16to8h" };
const static char* g_pShaders[4] = { "full", "reduced", "accurate", "accurate-reduced" };

// ----------------- Global Variables

ZZshContext	g_cgcontext;
ZZshProfile 	cgvProf, cgfProf;
int 		g_nPixelShaderVer = 0; 		// default
u8* 		s_lpShaderResources = NULL;
ZZshShaderLink 	pvs[16] = {sZero}, g_vsprog = sZero, g_psprog = sZero;							// 2 -- ZZ
ZZshParameter 	g_vparamPosXY[2] = {pZero}, g_fparamFogColor = pZero;

char*		ZZshSource;			// Shader's source data.
off_t		ZZshSourceSize;

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

const char* ShaderNames[MAX_ACTIVE_SHADERS] = {""};

ZZshProgram CompiledPrograms[MAX_ACTIVE_SHADERS][MAX_ACTIVE_SHADERS] = {{0}};

// new for GLSL4
GSUniformBufferOGL *constant_buffer;
GSUniformBufferOGL *common_buffer;
GSUniformBufferOGL *vertex_buffer;
GSUniformBufferOGL *fragment_buffer;
GSVertexBufferStateOGL *vertex_array;

COMMONSHADER g_cs;
static GLuint s_pipeline = 0;

//FRAGMENTSHADER ppsDebug;
//FRAGMENTSHADER ppsDebug2;

//------------------ Code

inline int GET_SHADER_INDEX(int type, int texfilter, int texwrap, int fog, int writedepth, int testaem, int exactcolor, int context, int ps) {
	return type + texfilter*NUM_TYPES + NUM_FILTERS*NUM_TYPES*texwrap + NUM_TEXWRAPS*NUM_FILTERS*NUM_TYPES*(fog+2*writedepth+4*testaem+8*exactcolor+16*context+32*ps) ;
}

// Nothing need to be done.
bool ZZshCheckProfilesSupport() {
	return true;
}

// Error handler. Setup in ZZogl_Create once.
void HandleCgError(ZZshContext ctx, ZZshError err, void* appdata)
{/*
	ZZLog::Error_Log("%s->%s: %s", ShaderCallerName, ShaderHandleName, cgGetErrorString(err));
	const char* listing = cgGetLastListing(g_cgcontext);
	if (listing != NULL)
		ZZLog::Debug_Log("	last listing: %s", listing);
*/
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

	g_nPixelShaderVer = 0;//SHADER_ACCURATE;
	// test
	bool bFailed;
	FRAGMENTSHADER* pfrag = ZZshLoadShadeEffect(0, 1, 1, 1, 1, temp, 0, &bFailed);
	if( bFailed || pfrag == NULL ) {
		g_nPixelShaderVer = SHADER_ACCURATE|SHADER_REDUCED;

		pfrag = ZZshLoadShadeEffect(0, 0, 1, 1, 0, temp, 0, &bFailed);
		if( bFailed || pfrag == NULL || glGetError() != GL_NO_ERROR) {
			g_nPixelShaderVer = SHADER_REDUCED;
			ZZLog::Error_Log("Basic shader test failed.");
		}
	}

	if (g_nPixelShaderVer & SHADER_REDUCED)
		conf.bilinear = 0;

	ZZLog::Error_Log("Creating extra effects.");
	B_G(ZZshLoadExtraEffects(), return false);

	ZZLog::Error_Log("Using %s shaders.", g_pShaders[g_nPixelShaderVer]);

	return true;
}

// open shader file according to build target
bool ZZshCreateOpenShadersFile() {
	std::string ShaderFileName("plugins/ps2hw_gl4.glsl");
	int ShaderFD = open(ShaderFileName.c_str(), O_RDONLY);
	struct stat sb;
	if ((ShaderFD == -1) || (fstat(ShaderFD, &sb) == -1)) {
		// Each linux distributions have his rules for path so we give them the possibility to
		// change it with compilation flags. -- Gregory
#ifdef GLSL_SHADER_DIR_COMPILATION
#define xGLSL_SHADER_DIR_str(s) GLSL_SHADER_DIR_str(s)
#define GLSL_SHADER_DIR_str(s) #s
		ShaderFileName = string(xGLSL_SHADER_DIR_str(GLSL_SHADER_DIR_COMPILATION)) + "/ps2hw_gl4.glsl";
		ShaderFD = open(ShaderFileName.c_str(), O_RDONLY);
#endif
		if ((ShaderFD == -1) || (fstat(ShaderFD, &sb) == -1)) {
			ZZLog::Error_Log("No source for %s: \n", ShaderFileName.c_str());
			return false;
		}
	}

	ZZshSourceSize = sb.st_size;
	ZZshSource = (char*)mmap(NULL, sb.st_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, ShaderFD, 0);		// This function directly maped file into memory.
	ZZshSource[ ZZshSourceSize - 1] = 0;										// Made source null-terminated.

	close(ShaderFD);
	return true;
}

void ZZshExitCleaning() {
	munmap(ZZshSource, ZZshSourceSize);
	delete constant_buffer;
	delete common_buffer;
	delete vertex_buffer;
	delete fragment_buffer;

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
	char* debug = new char[100];
	sprintf(debug, "CS: texture %d, param %d", texobj, param);
	if (GLEW_GREMEDY_string_marker) glStringMarkerGREMEDY(0, debug);
#endif

	g_cs.set_texture(param, texobj);
}

void ZZshGLSetTextureParameter(ZZshShaderLink prog, ZZshParameter param, GLuint texobj, const char* name) {
	FRAGMENTSHADER* shader = (FRAGMENTSHADER*)prog.link;
#ifdef ENABLE_MARKER
	char* debug = new char[100];
	sprintf(debug, "FS(%d):texture %d, param %d", shader->program, texobj, param);
	if (GLEW_GREMEDY_string_marker) glStringMarkerGREMEDY(0, debug);
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
	} else {
		VERTEXSHADER* shader = (VERTEXSHADER*)prog.link;
		shader->ZZshSetParameter4fv(param, v);
	}
#ifdef ENABLE_MARKER
	char* debug = new char[100];
	sprintf(debug, "prog: uniform (%s) (%f)", name, *v);
	if (GLEW_GREMEDY_string_marker) glStringMarkerGREMEDY(0, debug);
#endif
}

void ZZshSetParameter4fv(ZZshParameter param, const float* v, const char* name) {
	g_cs.ZZshSetParameter4fv(param, v);
#ifdef ENABLE_MARKER
	char* debug = new char[100];
	sprintf(debug, "CS: uniform (%s) (%f)", name, *v);
	if (GLEW_GREMEDY_string_marker) glStringMarkerGREMEDY(0, debug);
#endif
}

// The same stuff, but also with retry of param, name should be USED name of param for prog.
void ZZshSetParameter4fvWithRetry(ZZshParameter* param, ZZshShaderLink& prog, const float* v, const char* name) {
	if (prog.isFragment) {
		FRAGMENTSHADER* shader = (FRAGMENTSHADER*)prog.link;
		shader->ZZshSetParameter4fv(*param, v);
	} else {
		VERTEXSHADER* shader = (VERTEXSHADER*)prog.link;
		shader->ZZshSetParameter4fv(*param, v);
	}
}

// Used sometimes for color 1.
void ZZshDefaultOneColor( FRAGMENTSHADER& ptr ) {
//	return;
	ShaderHandleName = "Set Default One colot";
	float4 v = float4 ( 1, 1, 1, 1 );
	//ZZshSetParameter4fv(ptr.prog, ptr.sOneColor, v, "DegaultOne");
	ptr.ZZshSetParameter4fv(ptr.sOneColor, v);
}
//-------------------------------------------------------------------------------------

const GLchar * EmptyVertex = "void main(void) {gl_Position = ftransform();}";
const GLchar * EmptyFragment = "void main(void) {gl_FragColor = gl_Color;}";

inline bool GetCompilationLog(ZZshProgram program) {
#if	defined(DEVBUILD) || defined(_DEBUG)
	GLint log_length = 0;
	glGetProgramiv(program, GL_INFO_LOG_LENGTH, &log_length);

	char* log = new char[log_length];
	glGetProgramInfoLog(program, log_length, NULL, log);

	GLint CompileStatus;
	glGetProgramiv(program, GL_LINK_STATUS, &CompileStatus);
	if (CompileStatus == GL_FALSE) {
		ZZLog::Error_Log("Compiling... %d:\t %s", program, log);
		delete [] log;
		return false;
	} 

	delete [] log;
#endif
	return true;
}

static bool ValidateProgram(ZZshProgram Prog) {
#if	defined(DEVBUILD) || defined(_DEBUG)
	GLint isValid;

	glValidateProgram(Prog);
	glGetProgramiv(Prog, GL_VALIDATE_STATUS, &isValid);

	if (!isValid) {
		int lenght, infologlength;
		glGetProgramiv(Prog, GL_INFO_LOG_LENGTH, &infologlength);
		char* InfoLog = new char[infologlength];
		glGetProgramInfoLog(Prog, infologlength, &lenght, InfoLog);
		ZZLog::Error_Log("Validation %d... %d:\t %s", Prog, infologlength, InfoLog);
	}
	return (isValid != 0);
#else
	return true;
#endif
}

inline bool CompileShader(ZZshProgram& program, const char* DefineString, const char* name, GLenum shaderType, bool empty = false) {
	const GLchar* ShaderSource[2];
	ShaderSource[0] = (const GLchar*)DefineString;
	// FIXME, can maybe replaced with the program 0 it will used default fixed stage
	if (empty)
	{
		if(shaderType == GL_VERTEX_SHADER)
			ShaderSource[1] = EmptyVertex;
		else
			ShaderSource[1] = EmptyFragment;
		ZZLog::Error_Log("Used Empty shader for %s... Ok.",name);
	}
	else
		ShaderSource[1] = (const GLchar*)ZZshSource;

	program = glCreateShaderProgramv(shaderType, 2, &ShaderSource[0]);

	ZZLog::Debug_Log("Creating program %d for %s", program, name);

	if (!GetCompilationLog(program)) {
		ZZLog::Error_Log("Failed to compile shader for %s:", name);
		return false;
	}

	ShaderNames[program] = name;

	GL_REPORT_ERRORD();

	if (!ValidateProgram(program)) return false;

	return true;
}

inline bool LoadShaderFromFile(ZZshProgram& program, const char* DefineString, const char* name, GLenum ShaderType, bool empty = false) {			// Linux specific, as I presume

	if (!CompileShader(program, DefineString, name, ShaderType, empty)) {
		ZZLog::Error_Log("Failed to compile shader for %s: ", name);
		return false;
	}

	ZZLog::Error_Log("Compile shader for %s... Ok",name);
	return true;
}

//-------------------------------------------------------------------------------------

static void PutParametersAndRun(VERTEXSHADER* vs, FRAGMENTSHADER* ps) {
	UNIFORM_ERROR_LOG("Run program %s(%d) \t+\t%s(%d)", ShaderNames[vs->program], vs->program, ShaderNames[ps->program], ps->program);

	PutParametersInProgram(vs, ps);

#if	defined(DEVBUILD) || defined(_DEBUG)
	glValidateProgramPipeline(s_pipeline);
	GLint isValid;
	glGetProgramPipelineiv(s_pipeline, GL_VALIDATE_STATUS, &isValid);
	if (!isValid) ZZLog::Error_Log("Something weird happened on pipeline validation.");
#endif

	GL_REPORT_ERRORD();
}

void ZZshSetupShader() {
	VERTEXSHADER* vs = (VERTEXSHADER*)g_vsprog.link;
	FRAGMENTSHADER* ps = (FRAGMENTSHADER*)g_psprog.link;

	if (vs == NULL || ps == NULL) return;

	PutParametersAndRun(vs, ps);
}

void ZZshSetVertexShader(ZZshShaderLink prog) {
	g_vsprog = prog;

	VERTEXSHADER* vs = (VERTEXSHADER*)g_vsprog.link;
	if (!vs) return;

	glUseProgramStages(s_pipeline, GL_VERTEX_SHADER_BIT, vs->program);
}

void ZZshSetPixelShader(ZZshShaderLink prog) {
	g_psprog = prog;

	FRAGMENTSHADER* ps = (FRAGMENTSHADER*)g_psprog.link;
	if (!ps) return;

	glUseProgramStages(s_pipeline, GL_FRAGMENT_SHADER_BIT, ps->program);
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
}

void PutParametersInProgram(VERTEXSHADER* vs, FRAGMENTSHADER* ps) {

	common_buffer->bind();
	common_buffer->upload((void*)&g_cs.uniform_buffer[g_cs.context]);

	vertex_buffer->bind();
	vertex_buffer->upload((void*)&vs->uniform_buffer[vs->context]);

	fragment_buffer->bind();
	fragment_buffer->upload((void*)&ps->uniform_buffer[ps->context]);

#ifdef ENABLE_MARKER
	char* debug = new char[100];
	sprintf(debug, "FS(%d): enable texture", ps->program);
	if (GLEW_GREMEDY_string_marker) glStringMarkerGREMEDY(0, debug);
#endif

	g_cs.enable_texture();
	ps->enable_texture();
}

static void SetupFragmentProgramParameters(FRAGMENTSHADER* pf, int context, int type)
{
	// uniform parameters
	pf->prog.link = (void*)pf;			// Setting autolink
	pf->prog.isFragment = true;			// Setting autolink
	pf->context = context;


}

void SetupVertexProgramParameters(VERTEXSHADER* pf, int context)
{
	pf->prog.link = (void*)pf;			// Setting autolink
	pf->prog.isFragment = false;			// Setting autolink
	pf->context = context;
}

//const int GLSL_VERSION = 130;  			// Sampler2DRect appear in 1.3
const int GLSL_VERSION = 330;

// We use strictly compilation from source for GSLS
static __forceinline void GlslHeaderString(char* header_string, const char* name, const char* depth)
{
	sprintf(header_string, "#version %d compatibility\n#define %s main\n%s\n", GLSL_VERSION, name, depth);
}

static __forceinline bool LOAD_VS(char* DefineString, const char* name, VERTEXSHADER& vertex, int shaderver, ZZshProfile context, const char* depth)
{
	bool flag;
	char temp[200];
	GlslHeaderString(temp, name, depth);
	sprintf(DefineString, "%s#define VERTEX_SHADER 1\n", temp);
	//ZZLog::WriteLn("Define for VS == '%s'", DefineString);
	flag = LoadShaderFromFile(vertex.program, DefineString, name, GL_VERTEX_SHADER);
	SetupVertexProgramParameters(&vertex, context * NOCONTEXT);
	return flag;
}

static __forceinline bool LOAD_PS(char* DefineString, const char* name, FRAGMENTSHADER& fragment, int shaderver, ZZshProfile context, const char* depth)
{
	bool flag;
	char temp[200];
	GlslHeaderString(temp, name, depth);
	sprintf(DefineString, "%s#define FRAGMENT_SHADER 1\n", temp);
	//ZZLog::WriteLn("Define for PS == '%s'", DefineString);

	flag = LoadShaderFromFile(fragment.program, DefineString, name, GL_FRAGMENT_SHADER);
	SetupFragmentProgramParameters(&fragment, context * NOCONTEXT, 0);
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
	char DefineString[DEFINE_STRING_SIZE] = "";
	const char* writedepth = "#define WRITE_DEPTH 1\n";	// should we write depth field


	const char* pvsshaders[4] = { "RegularVS", "TextureVS", "RegularFogVS", "TextureFogVS" };

	for (int i = 0; i < 4; ++i) {
		if (!LOAD_VS(DefineString, pvsshaders[i], pvsStore[2 * i], cgvProf, 0, "")) bLoadSuccess = false;
		if (!LOAD_VS(DefineString, pvsshaders[i], pvsStore[2 *i + 1 ], cgvProf, 1, "")) bLoadSuccess = false;
		if (!LOAD_VS(DefineString, pvsshaders[i], pvsStore[2 *i + 8 ], cgvProf, 0, writedepth)) bLoadSuccess = false;
		if (!LOAD_VS(DefineString, pvsshaders[i], pvsStore[2 *i + 8 + 1], cgvProf, 1, writedepth)) bLoadSuccess = false;
	}
	for (int i = 0; i < 16; ++i)
		pvs[i] = pvsStore[i].prog;

	if (!LOAD_VS(DefineString, "BitBltVS", pvsBitBlt, cgvProf, 0, "")) bLoadSuccess = false;
	GL_REPORT_ERRORD();

	if (!LOAD_PS(DefineString, "RegularPS", ppsRegular[0], cgfProf, 0, "")) bLoadSuccess = false;
	if (!LOAD_PS(DefineString, "RegularFogPS", ppsRegular[1], cgfProf, 0, "")) bLoadSuccess = false;

	if( conf.mrtdepth ) {
		if (!LOAD_PS(DefineString, "RegularPS", ppsRegular[2], cgfProf, 0, writedepth)) bLoadSuccess = false;
		if (!bLoadSuccess) conf.mrtdepth = 0;

		if (!LOAD_PS(DefineString, "RegularFogPS", ppsRegular[3], cgfProf, 0, writedepth)) bLoadSuccess = false;
		if (!bLoadSuccess) conf.mrtdepth = 0;
	}

	if (!LOAD_PS(DefineString, "BitBltPS", ppsBitBlt[0], cgfProf, 0, "")) bLoadSuccess = false;
	if (!LOAD_PS(DefineString, "BitBltAAPS", ppsBitBlt[1], cgfProf, 0, "")) bLoadSuccess = false;
	if (!bLoadSuccess) {
		ZZLog::Error_Log("Failed to load BitBltAAPS, using BitBltPS.");
		if (!LOAD_PS(DefineString, "BitBltPS", ppsBitBlt[1], cgfProf, 0, "")) bLoadSuccess = false;
	}

	if (!LOAD_PS(DefineString, "BitBltDepthPS", ppsBitBltDepth, cgfProf, 0, "")) bLoadSuccess = false;
	if (!LOAD_PS(DefineString, "CRTCTargPS", ppsCRTCTarg[0], cgfProf, 0, "")) bLoadSuccess = false;
	if (!LOAD_PS(DefineString, "CRTCTargInterPS", ppsCRTCTarg[1], cgfProf, 0, "")) bLoadSuccess = false;

	g_bCRTCBilinear = true;
	if (!LOAD_PS(DefineString, "CRTCPS", ppsCRTC[0], cgfProf, 0, "")) bLoadSuccess = false;
	if( !bLoadSuccess ) {
		// switch to simpler
		g_bCRTCBilinear = false;
		if (!LOAD_PS(DefineString, "CRTCPS_Nearest", ppsCRTC[0], cgfProf, 0, "")) bLoadSuccess = false;
		if (!LOAD_PS(DefineString, "CRTCInterPS_Nearest", ppsCRTC[0], cgfProf, 0, "")) bLoadSuccess = false;
	}
	else {
		if (!LOAD_PS(DefineString, "CRTCInterPS", ppsCRTC[1], cgfProf, 0, "")) bLoadSuccess = false;
	}

	if( !bLoadSuccess )
		ZZLog::Error_Log("Failed to create CRTC shaders.");

	// if (!LOAD_PS(DefineString, "CRTC24PS", ppsCRTC24[0], cgfProf, 0, "")) bLoadSuccess = false;
	// if (!LOAD_PS(DefineString, "CRTC24InterPS", ppsCRTC24[1], cgfProf, 0, "")) bLoadSuccess = false;
	if (!LOAD_PS(DefineString, "ZeroPS", ppsOne, cgfProf, 0, "")) bLoadSuccess = false;
	if (!LOAD_PS(DefineString, "BaseTexturePS", ppsBaseTexture, cgfProf, 0, "")) bLoadSuccess = false;
	if (!LOAD_PS(DefineString, "Convert16to32PS", ppsConvert16to32, cgfProf, 0, "")) bLoadSuccess = false;
	if (!LOAD_PS(DefineString, "Convert32to16PS", ppsConvert32to16, cgfProf, 0, "")) bLoadSuccess = false;

	// DEBUG
	// if (!LOAD_PS(DefineString, "ZeroDebugPS", ppsDebug, cgfProf, 0, "")) bLoadSuccess = false;
	// if (!LOAD_PS(DefineString, "ZeroDebug2PS", ppsDebug2, cgfProf, 0, "")) bLoadSuccess = false;

	GL_REPORT_ERRORD();
	return true;
}

const static char* g_pPsTexWrap[] = { "#define REPEAT 1\n", "#define CLAMP 1\n", "#define REGION_REPEAT 1\n", "" };

static ZZshProgram LoadShaderFromType(int type, int texfilter, int texwrap, int fog, int writedepth, int testaem, int exactcolor, int ps, int context) {

	assert( texwrap < NUM_TEXWRAPS);
	assert( type < NUM_TYPES );

	char* name = new char[MAX_SHADER_NAME_SIZE];
	sprintf(name, "Texture%s%d_%sPS", fog?"Fog":"", texfilter, g_pTexTypes[type]);

	ZZLog::Debug_Log("Starting shader for %s", name);

	const char* AddWrap 	= g_pPsTexWrap[texwrap];
	const char* AddDepth 	= writedepth?"#define WRITE_DEPTH 1\n":"";
	const char* AddAEM	= testaem?"#define TEST_AEM 1\n":"";
	const char* AddExcolor	= exactcolor?"#define EXACT_COLOR 1\n":"";
	const char* AddAccurate  = (ps & SHADER_ACCURATE)?"#define ACCURATE_DECOMPRESSION 1\n":"";
	char DefineString[DEFINE_STRING_SIZE] = "";
	char temp[200];
	GlslHeaderString(temp, name, AddWrap);
	sprintf(DefineString, "%s#define FRAGMENT_SHADER 1\n%s%s%s%s\n#define CTX %d\n", temp, AddDepth, AddAEM, AddExcolor, AddAccurate, context * NOCONTEXT);

	ZZshProgram program;
	if (!LoadShaderFromFile(program, DefineString, name, GL_FRAGMENT_SHADER))
		return LoadShaderFromFile(program, DefineString, name, GL_FRAGMENT_SHADER, true);

	ZZLog::Debug_Log("Create new shader for type:%d filter:%d wrap:%d for:%d depth:%d aem:%d color:%d decompression:%d ctx:%d... Ok \n", type, texfilter, texwrap, fog, writedepth, testaem, exactcolor, ps, context);

	GL_REPORT_ERRORD();
	return program;
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

	if (ZZshExistProgram(pf))
	{
		return pf;
	}
	pf->program = LoadShaderFromType(type, texfilter, texwrap, fog, s_bWriteDepth, testaem, exactcolor, g_nPixelShaderVer, context);

	if (ZZshExistProgram(pf)) {
		SetupFragmentProgramParameters(pf, context, type);
		GL_REPORT_ERRORD();

		if( glGetError() != GL_NO_ERROR ) {
				ZZLog::Check_GL_Error();
				ZZLog::Error_Log("Failed to load shader %d,%d,%d,%d.", type, fog, texfilter, 4*clamp.wms+clamp.wmt);
				if (pbFailed != NULL ) *pbFailed = true;
				return pf;
		}

		return pf;
	}

	ZZLog::Error_Log("Failed to create shader %d,%d,%d,%d.", type, fog, texfilter, 4*clamp.wms+clamp.wmt);
	if( pbFailed != NULL ) *pbFailed = true;

	GL_REPORT_ERRORD();
	return NULL;
}

#endif // GLSL4_API
