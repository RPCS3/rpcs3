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

#ifdef GLSL_API 		// This code is only for GLSL API
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
#define MAX_UNIFORM_NAME_SIZE	20
#define DEFINE_STRING_SIZE	256
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

ZZshProgram	ZZshMainProgram;
char*		ZZshSource;			// Shader's source data.	
off_t		ZZshSourceSize;

extern char* EFFECT_NAME;				// All this variables used for testing and set manually
extern char* EFFECT_DIR;

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

struct SHADERHEADER
{
	unsigned int index, offset, size; // if highest bit of index is set, pixel shader
};
map<int, SHADERHEADER*> mapShaderResources;

// Debug variable, store name of the function that call the shader.
const char* ShaderCallerName = "";
const char* ShaderHandleName = "";

int NumActiveUniforms, NumGlobalUniforms;
ZZshParamInfo UniformsIndex[MAX_ACTIVE_UNIFORMS] = {qZero};
const char* ShaderNames[MAX_ACTIVE_SHADERS] = {""};
ZZshShaderType ShaderTypes[MAX_ACTIVE_SHADERS] = {ZZ_SH_NONE};

ZZshProgram CompiledPrograms[MAX_ACTIVE_SHADERS][MAX_ACTIVE_SHADERS] = {{0}};
const char* TextureUnits[NUMBER_OF_SAMPLERS] = 
	{"g_sMemory[0]", 	"g_sMemory[1]", 	"g_sSrcFinal", 		"g_sBitwiseANDX", 	"g_sBitwiseANDY",  "g_sInterlace", \
		"g_sCLUT", 		"g_sBlocks", 		"g_sBilinearBlocks", 		"g_sConv16to32", 	"g_sConv32to16"};
ZZshPARAMTYPE TextureTypes[NUMBER_OF_SAMPLERS] = 
	{ZZ_TEXTURE_RECT, 	ZZ_TEXTURE_RECT, 	ZZ_TEXTURE_RECT, 	ZZ_TEXTURE_RECT, 	ZZ_TEXTURE_RECT, 	ZZ_TEXTURE_RECT, \
		ZZ_TEXTURE_2D, 		ZZ_TEXTURE_2D, 		ZZ_TEXTURE_2D,			ZZ_TEXTURE_2D,		 ZZ_TEXTURE_3D} ;

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

float ZeroFloat4[4] = {0};

inline void SettleFloat(float* f, const float* v) {
	f[0] = v[0];
	f[1] = v[1];
	f[2] = v[2];
	f[3] = v[3];
}

inline ZZshParamInfo ParamInfo(const char* ShName, ZZshPARAMTYPE type, const float fvalue[], GLuint sampler, GLint texid, bool Constant, bool Settled) {
	ZZshParamInfo x;
	x.ShName = new char[MAX_UNIFORM_NAME_SIZE];
	x.ShName = ShName;
	x.type = type;
	SettleFloat(x.fvalue, fvalue);
	x.sampler = sampler;
	x.texid = texid;
	x.Constant = Constant;
	x.Settled = Settled;
	return x;
}

inline void SetGlobalUniform(ZZshParameter* param, const char* name) {
	*param = NumActiveUniforms;
	UniformsIndex[NumActiveUniforms] = ParamInfo(name, ZZ_FLOAT4, ZeroFloat4, -1, 0, false, false);
	NumActiveUniforms++;  
}

bool ZZshStartUsingShaders() {
	
	ZZLog::Error_Log("Creating effects.");
	B_G(LoadEffects(), return false);
	if (!glCreateShader) 
	{
		ZZLog::Error_Log("GLSL shaders is not supported, stop.");
		return false;
	}

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
			glLinkProgram(pfrag->Shader);
		if( bFailed || pfrag == NULL || glGetError() != GL_NO_ERROR) {
			g_nPixelShaderVer = SHADER_REDUCED;
			ZZLog::Error_Log("Basic shader test failed.");
		}
	}
	ZZshMainProgram = glCreateProgram();
	NumActiveUniforms = 0;
	SetGlobalUniform(&g_fparamFogColor, "g_fFogColor"); 
	SetGlobalUniform(&g_vparamPosXY[0], "g_fPosXY[0]");
	SetGlobalUniform(&g_vparamPosXY[1], NOCONTEXT?"g_fPosXY[1]":"g_fPosXY[0]");
	NumGlobalUniforms = NumActiveUniforms;

	if (g_nPixelShaderVer & SHADER_REDUCED)
		conf.bilinear = 0;

	ZZLog::Error_Log("Creating extra effects.");
	B_G(ZZshLoadExtraEffects(), return false);

	ZZLog::Error_Log("Using %s shaders.", g_pShaders[g_nPixelShaderVer]);	

	return true;
}

// open shader file according to build target 
bool ZZshCreateOpenShadersFile() {
	std::string ShaderFileName("plugins/ps2hw.glsl");
	int ShaderFD = open(ShaderFileName.c_str(), O_RDONLY);
	struct stat sb;
	if ((ShaderFD == -1) || (fstat(ShaderFD, &sb) == -1)) {	
		// Each linux distributions have his rules for path so we give them the possibility to
		// change it with compilation flags. -- Gregory
#ifdef PLUGIN_DIR_COMPILATION
#define xPLUGIN_DIR_str(s) PLUGIN_DIR_str(s)
#define PLUGIN_DIR_str(s) #s
		ShaderFileName = string(xPLUGIN_DIR_str(PLUGIN_DIR_COMPILATION)) + "/ps2hw.glsl";
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
}

// Disable CG
void ZZshGLDisableProfile() {			// This stop all other shader programs from running;
	glUseProgram(0);
}
//Enable CG
void ZZshGLEnableProfile() {
}
//-------------------------------------------------------------------------------------

// The same function for texture, also to cgGLEnable
void ZZshGLSetTextureParameter(ZZshParameter param, GLuint texobj, const char* name) {
	if (param > -1) {
//		ZZLog::Error_Log("Set texture parameter %s %d... Ok", name, texobj);
		UniformsIndex[param].texid = texobj;
		UniformsIndex[param].Settled = true;
	}
}

void ZZshGLSetTextureParameter(ZZshShaderLink prog, ZZshParameter param, GLuint texobj, const char* name) {
	if (param > -1) {
//		ZZLog::Error_Log("Set texture parameter %s %d... Ok", name, texobj);
		UniformsIndex[param].texid = texobj;
		UniformsIndex[param].Settled = true;
	}
}

// This is helper of cgGLSetParameter4fv, made for debug purpose.
// Name could be any string. We must use it on compilation time, because erroneus handler does not
// return name
void ZZshSetParameter4fv(ZZshShaderLink prog, ZZshParameter param, const float* v, const char* name) {	
	if (param > -1) {
//		ZZLog::Error_Log("Set float parameter %s %f, %f, %f, %f... Ok", name, v[0], v[1], v[2], v[3]);
		SettleFloat(UniformsIndex[param].fvalue, v);
		UniformsIndex[param].Settled = true;
	}
}
 
void ZZshSetParameter4fv(ZZshParameter param, const float* v, const char* name) {	
	if (param > -1) {
//		ZZLog::Error_Log("Set float parameter %s %f, %f, %f, %f... Ok", name, v[0], v[1], v[2], v[3]);
		SettleFloat(UniformsIndex[param].fvalue, v);	
		UniformsIndex[param].Settled = true;
	}
}

// The same stuff, but also with retry of param, name should be USED name of param for prog.
void ZZshSetParameter4fvWithRetry(ZZshParameter* param, ZZshShaderLink prog, const float* v, const char* name) {
	if (param != NULL)
		ZZshSetParameter4fv(prog, *param, v, name);
}

// Used sometimes for color 1.
void ZZshDefaultOneColor( FRAGMENTSHADER ptr ) {
//	return;	
	ShaderHandleName = "Set Default One colot";
	float4 v = float4 ( 1, 1, 1, 1 );
	ZZshSetParameter4fv(ptr.prog, ptr.sOneColor, v, "DegaultOne");
}
//-------------------------------------------------------------------------------------

const GLchar * EmptyVertex = "void main(void) {gl_Position = ftransform();}"; 
const GLchar * EmptyFragment = "void main(void) {gl_FragColor = gl_Color;}"; 

inline ZZshProgram UseEmptyProgram(const char* name, GLenum shaderType) {
	GLuint shader = glCreateShader(shaderType);
	if (shaderType == GL_VERTEX_SHADER)
		glShaderSource(shader, 1, &EmptyVertex, NULL); 
	else
		glShaderSource(shader, 1, &EmptyFragment, NULL); 

	glCompileShader(shader); 
	ZZshProgram prog = glCreateProgram(); 
	glAttachShader(prog, shader); 
	glLinkProgram(prog);
	if( !glIsProgram(prog) || glGetError() != GL_NO_ERROR ) { 
		ZZLog::Error_Log("Failed to load empty shader for %s:", name); 
		return -1; 
	} 
	ZZLog::Error_Log("Used Empty program for %s... Ok.",name);
	return prog;
}

ZZshShaderType ZZshGetShaderType(const char* name) {
	if (strncmp(name, "TextureFog", 10) == 0) return ZZ_SH_TEXTURE_FOG;
	if (strncmp(name, "Texture", 7) == 0) return ZZ_SH_TEXTURE;
	if (strncmp(name, "RegularFog", 10) == 0) return ZZ_SH_REGULAR_FOG;
	if (strncmp(name, "Regular", 7) == 0) return ZZ_SH_REGULAR;
	if (strncmp(name, "Zero", 4) == 0) return ZZ_SH_ZERO;
	return ZZ_SH_CRTC;
}

inline ZZshShader UseEmptyShader(const char* name, GLenum shaderType) {
	GLuint shader = glCreateShader(shaderType);
	if (shaderType == GL_VERTEX_SHADER)
		glShaderSource(shader, 1, &EmptyVertex, NULL); 
	else
		glShaderSource(shader, 1, &EmptyFragment, NULL); 

	glCompileShader(shader); 

	ShaderNames[shader] = name;
	ShaderTypes[shader] = ZZshGetShaderType(name);

	ZZLog::Error_Log("Used Empty shader for %s... Ok.",name);
	return shader;
}

inline bool GetCompilationLog(GLuint shader) {
	GLint CompileStatus;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &CompileStatus);
	if (CompileStatus == GL_TRUE)
		return true;

	int* lenght, infologlength;
	glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infologlength);
	char* InfoLog = new char[infologlength];
	glGetShaderInfoLog(shader, infologlength, lenght, InfoLog);
	ZZLog::Error_Log("Compiling... %d:\t %s", shader, InfoLog);

	return false;
}

inline bool CompileShader(ZZshProgram& shader, const char* DefineString, const char* name, GLenum shaderType) {
	const GLchar* ShaderSource[2];
	ShaderSource[0] = (const GLchar*)DefineString;
	ShaderSource[1] = (const GLchar*)ZZshSource;

	shader = glCreateShader(shaderType);
	glShaderSource(shader, 2, &ShaderSource[0], NULL); 
	glCompileShader(shader); 
	ZZLog::Debug_Log("Creating shader %d for %s", shader, name);

	if (!GetCompilationLog(shader)) {
		ZZLog::Error_Log("Failed to compile shader for %s:", name); 
		return false;
	}

	ShaderTypes[shader] = ZZshGetShaderType(name);
	ShaderNames[shader] = name;

	GL_REPORT_ERRORD();
	return true;
}

inline bool LoadShaderFromFile(ZZshShader& shader, const char* DefineString, const char* name, GLenum ShaderType) {			// Linux specific, as I presume
	if (!CompileShader(shader, DefineString, name, ShaderType)) {
		ZZLog::Error_Log("Failed to compile shader for %s: ", name); 
	       	return false; 
	}

	ZZLog::Error_Log("Used shader for %s... Ok",name);
	return true;
}

inline bool GetLinkLog(ZZshProgram prog) {
	GLint LinkStatus;
	glGetProgramiv(prog, GL_LINK_STATUS, &LinkStatus);

	int unif, atrib;
	glGetProgramiv(prog, GL_ACTIVE_UNIFORMS, &unif);
	glGetProgramiv(prog, GL_ACTIVE_ATTRIBUTES, &atrib);
	UNIFORM_ERROR_LOG("Uniforms %d, attributes %d", unif, atrib);

	if (LinkStatus == GL_TRUE && glIsProgram(prog)) return true;

#ifdef DEVBUILD
	int* lenght, infologlength;
	glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &infologlength);
	char* InfoLog = new char[infologlength];
	glGetProgramInfoLog(prog, infologlength, lenght, InfoLog);
	if (!infologlength == 0)
		ZZLog::Error_Log("Linking %d... %d:\t %s", prog, infologlength, InfoLog);
#endif

	return false;	

}

//-------------------------------------------------------------------------------------
inline ZZshProgram madeProgram(ZZshShader shader, ZZshShader shader2, char* name) {
	ZZshProgram prog = glCreateProgram();
	glAttachShader(prog, shader); 
	if (shader2 != 0)
		glAttachShader(prog, shader2); 
	glLinkProgram(prog);
	if (!GetLinkLog(prog)) { 
		ZZLog::Error_Log("Failed to link shader for %s: ", name); 
		prog = UseEmptyProgram(name, GL_FRAGMENT_SHADER);
	}
	glDetachShader(prog, shader); 

	ZZLog::Error_Log("Made shader program for %s... Ok",name);
	return prog;
}

void PutParametersInProgam(int start, int finish) {
	for (int i = start; i < finish; i++) {
		ZZshParamInfo param = UniformsIndex[i];
		GLint location = glGetUniformLocation(ZZshMainProgram, param.ShName);

		if (location != -1 && param.type != ZZ_UNDEFINED) {
			UNIFORM_ERROR_LOG("\tTry uniform %d %d %d %s...\t\t", i, location, param.type, param.ShName);

			if (!param.Settled && !param.Constant) {
				UNIFORM_ERROR_LOG("\tUnsettled, non-constant uniform, could be bug: %d %s", param.type, param.ShName);
				continue;
			}

			if (param.type == ZZ_FLOAT4) {
				glUniform4fv(location, 1, param.fvalue);
			}
			else
			{
				glActiveTexture(GL_TEXTURE0 + param.sampler);
				if (param.type == ZZ_TEXTURE_2D)
					glBindTexture(GL_TEXTURE_2D, param.texid);
				else if (param.type == ZZ_TEXTURE_3D)
					glBindTexture(GL_TEXTURE_3D, param.texid);
				else
					glBindTexture(GL_TEXTURE_RECTANGLE, param.texid);
				GL_REPORT_ERRORD();
			}

			if (glGetError() == GL_NO_ERROR)
				UNIFORM_ERROR_LOG("Ok. Param name %s, location %d, type %d", param.ShName, location, param.type);
			else
				ZZLog::Error_Log("error in PutParametersInProgam param name %s, location %d, type %d", param.ShName, location, param.type);

			if (!param.Constant)								// Unset used parameters 
				UniformsIndex[i].Settled == false;
		}
		else if (start != 0 && location == -1 && param.Settled)					// No global variable
			ZZLog::Error_Log("Warning! Unused, but set uniform %d, %s", location, param.ShName);
	}
	GL_REPORT_ERRORD();
}

void PutSInProgam(int start, int finish) {
	for (int i = start; i < finish; i++) {
		ZZshParamInfo param = UniformsIndex[i];
		GLint location = glGetUniformLocation(ZZshMainProgram, param.ShName);

		if (location != -1 && param.type != ZZ_UNDEFINED) {
			if (param.type != ZZ_FLOAT4) {
				UNIFORM_ERROR_LOG("\tTry sampler %d %d %d %s %d...\t\t", i, location, param.type, param.ShName, param.sampler);
				if (glGetError() == GL_NO_ERROR)
					UNIFORM_ERROR_LOG("Ok");
				else
					UNIFORM_ERROR_LOG("error!");
				glUniform1i(location, param.sampler);
			}
		}
	}
	GL_REPORT_ERRORD();
}

bool ValidateProgram(ZZshProgram Prog) {
	GLint isValid;
	glGetProgramiv(Prog, GL_VALIDATE_STATUS, &isValid);

	if (!isValid) {
		glValidateProgram(Prog);
		int* lenght, infologlength;
		glGetProgramiv(Prog, GL_INFO_LOG_LENGTH, &infologlength);
		char* InfoLog = new char[infologlength];
		glGetProgramInfoLog(Prog, infologlength, lenght, InfoLog);
		ZZLog::Error_Log("Validation %d... %d:\t %s", Prog, infologlength, InfoLog);
	}
	return (isValid != 0); 
}

void PutParametersAndRun(VERTEXSHADER* vs, FRAGMENTSHADER* ps) {
	UNIFORM_ERROR_LOG("Run program %s(%d) \t+\t%s(%d)", ShaderNames[vs->Shader], vs->Shader, ShaderNames[ps->Shader], ps->Shader);

	glUseProgram(ZZshMainProgram);	
	if (glGetError() != GL_NO_ERROR) {
		ZZLog::Error_Log("Something weird happened on Linking stage.");

		glUseProgram(0);
		return;
	}

	PutSInProgam(vs->ParametersStart, vs->ParametersFinish);
	PutSInProgam(ps->ParametersStart, ps->ParametersFinish);

	PutParametersInProgam(0, NumGlobalUniforms);
	PutParametersInProgam(vs->ParametersStart, vs->ParametersFinish);
	PutParametersInProgam(ps->ParametersStart, ps->ParametersFinish);	

	ValidateProgram(ZZshMainProgram);
	GL_REPORT_ERRORD();
}

void CreateAndRunMain(VERTEXSHADER* vs, FRAGMENTSHADER* ps) {
	ZZLog::Error_Log("\n--->  New shader program %d, %s(%d) \t+\t%s(%d).", ZZshMainProgram, ShaderNames[vs->Shader], vs->Shader, ShaderNames[ps->Shader], ps->Shader);

	if (vs->Shader != 0)
		glAttachShader(ZZshMainProgram, vs->Shader);
	if (ps->Shader != 0)
		glAttachShader(ZZshMainProgram, ps->Shader);

	glLinkProgram(ZZshMainProgram);
	if (!GetLinkLog(ZZshMainProgram)) {
		ZZLog::Error_Log("Main program linkage error, don't use any shader for this stage.");
		return;
	}

	GL_REPORT_ERRORD();

       	PutParametersAndRun(vs, ps);	
	GL_REPORT_ERRORD();
}

inline bool ZZshCheckShaderCompatibility(VERTEXSHADER* vs, FRAGMENTSHADER* ps) { 
	if (vs == NULL) return false;
	if (vs->ShaderType == ZZ_SH_ZERO) return true;			// ZeroPS is compatible with everything
	if (ps == NULL) return false;

	return (vs->ShaderType == ps->ShaderType);
}

void ZZshSetShader(VERTEXSHADER* vs, FRAGMENTSHADER* ps) {
	if (!ZZshCheckShaderCompatibility(vs, ps)) 				// We don't need to link uncompatible shaders
		return;	

	int vss = (vs!=NULL)?vs->Shader:0;
	int pss = (ps!=NULL)?ps->Shader:0;

	if (vss !=0 && pss != 0) {
		if (CompiledPrograms[vss][pss] != 0 && glIsProgram(CompiledPrograms[vss][pss])) {
			ZZshMainProgram = CompiledPrograms[vs->Shader][ps->Shader];
			PutParametersAndRun(vs, ps);
		}
		else {
			ZZshProgram NewProgram = glCreateProgram();
			ZZshMainProgram = NewProgram;
			CompiledPrograms[vss][pss] = NewProgram;
			CreateAndRunMain(vs, ps) ;
		}
	}
}

void ZZshSetVertexShader(ZZshShaderLink prog) {
	g_vsprog = prog;
	ZZshSetShader((VERTEXSHADER*)(g_vsprog.link), (FRAGMENTSHADER*)(g_psprog.link)) ;
}

void ZZshSetPixelShader(ZZshShaderLink prog) {
	g_psprog = prog;
	ZZshSetShader((VERTEXSHADER*)(g_vsprog.link), (FRAGMENTSHADER*)(g_psprog.link)) ;
}

//------------------------------------------------------------------------------------------------------------------

// For several reason texobj could not be put in sampler directly, only though GL_TEXTUREi interface. So we need to check correct sampler for each one.
inline void SettleTextureUnit(ZZshParamInfo* param, const char* name) {
	for (int i = 0; i < NUMBER_OF_SAMPLERS; i++) {
		if (strcmp(TextureUnits[i], name) == 0) {
			param->sampler = i;
			param->type = TextureTypes[i];
			return;
		}
	}
}

inline int SetUniformParam(ZZshProgram prog, ZZshParameter* param, const char* name) {
	GLint p = glGetUniformLocation(prog, name); 
	if (p > -1) { 
		*param = NumActiveUniforms;
		UniformsIndex[NumActiveUniforms] = ParamInfo(name, ZZ_FLOAT4, ZeroFloat4, -1, 0, false, false);		// By define Uniform is FLOAT4

		SettleTextureUnit(&(UniformsIndex[NumActiveUniforms]), name);
		UNIFORM_ERROR_LOG("uniform %s \t\t%d %d", name, p, UniformsIndex[NumActiveUniforms].type); 

		NumActiveUniforms++;  		
	}
	else 
		*param = -1; 
	return p;
}

#define SET_UNIFORMPARAM(var, name) { \
	p = SetUniformParam(prog, &(pf->var), name); \
} 

#define INIT_SAMPLERPARAM(tex, name) { \
	ZZshParameter x; \
	p = SetUniformParam(prog, &x, name); \
	(UniformsIndex[x]).Constant = true; \
	ZZshGLSetTextureParameter(pf->prog, x, tex, name); \
}

#define INIT_UNIFORMPARAM(var, name) { \
	ZZshParameter x; \
	p = SetUniformParam(prog, &x, name); \
	(UniformsIndex[x]).Constant = true; \
	ZZshSetParameter4fv(pf->prog, x, var, name); \
} 

char* AddContextToName(const char* name, int context) {
	char* newname = new char[MAX_UNIFORM_NAME_SIZE];
	sprintf(newname, "%s[%d]", name, context * NOCONTEXT);
	return newname;
}

void SetupFragmentProgramParameters(FRAGMENTSHADER* pf, int context, int type)
{
	// uniform parameters
	GLint p;
	pf->prog.link = (void*)pf;			// Setting autolink
	pf->prog.isFragment = true;			// Setting autolink
	pf->ShaderType = ShaderTypes[pf->Shader];

	pf->ParametersStart = NumActiveUniforms;
	ZZshProgram prog = madeProgram(pf->Shader, 0, "");
	glUseProgram(prog);
	GL_REPORT_ERRORD();

	SET_UNIFORMPARAM(sOneColor, "g_fOneColor");
	SET_UNIFORMPARAM(sBitBltZ, "g_fBitBltZ");
	SET_UNIFORMPARAM(sInvTexDims, "g_fInvTexDims");
	SET_UNIFORMPARAM(fTexAlpha2, AddContextToName("fTexAlpha2", context));
	SET_UNIFORMPARAM(fTexOffset, AddContextToName("g_fTexOffset", context));
	SET_UNIFORMPARAM(fTexDims, AddContextToName("g_fTexDims", context));
	SET_UNIFORMPARAM(fTexBlock, AddContextToName("g_fTexBlock", context));
	SET_UNIFORMPARAM(fClampExts, AddContextToName("g_fClampExts",  context)); 		// FIXME: There is a bug, that lead FFX-1 to incorrect CLAMP if this uniform have context.
	SET_UNIFORMPARAM(fTexWrapMode, AddContextToName("TexWrapMode", context));
	SET_UNIFORMPARAM(fRealTexDims, AddContextToName("g_fRealTexDims", context));
	SET_UNIFORMPARAM(fTestBlack, AddContextToName("g_fTestBlack", context));
	SET_UNIFORMPARAM(fPageOffset, AddContextToName("g_fPageOffset", context));
	SET_UNIFORMPARAM(fTexAlpha, AddContextToName("fTexAlpha", context));
	GL_REPORT_ERRORD();

	// textures
	INIT_SAMPLERPARAM(ptexBlocks, "g_sBlocks");
	if (type == 3) 
		{INIT_SAMPLERPARAM(ptexConv16to32, "g_sConv16to32");}
	else if (type == 4) 
		{INIT_SAMPLERPARAM(ptexConv32to16, "g_sConv32to16");}
	else 
		{INIT_SAMPLERPARAM(ptexBilinearBlocks, "g_sBilinearBlocks");}
	GL_REPORT_ERRORD();

	SET_UNIFORMPARAM(sMemory, AddContextToName("g_sMemory", context));
	SET_UNIFORMPARAM(sFinal, "g_sSrcFinal");
	SET_UNIFORMPARAM(sBitwiseANDX, "g_sBitwiseANDX");
	SET_UNIFORMPARAM(sBitwiseANDY, "g_sBitwiseANDY");
	SET_UNIFORMPARAM(sCLUT, "g_sCLUT");
	SET_UNIFORMPARAM(sInterlace, "g_sInterlace");
	GL_REPORT_ERRORD();

	// set global shader constants
	INIT_UNIFORMPARAM(float4(0.5f, (conf.settings().exact_color)?0.9f/256.0f:0.5f/256.0f, 0,1/255.0f), "g_fExactColor");
	INIT_UNIFORMPARAM(float4(-0.2f, -0.65f, 0.9f, 1.0f / 32767.0f ), "g_fBilinear");
	INIT_UNIFORMPARAM(float4(1.0f/256.0f, 1.0004f, 1, 0.5f), "g_fZBias");
	INIT_UNIFORMPARAM(float4(0,1, 0.001f, 0.5f), "g_fc0");
	INIT_UNIFORMPARAM(float4(1/1024.0f, 0.2f/1024.0f, 1/128.0f, 1/512.0f), "g_fMult");
	pf->ParametersFinish = NumActiveUniforms;
	if (NumActiveUniforms > MAX_ACTIVE_UNIFORMS)
		ZZLog::Error_Log("Too many shader variables. You may increase the limit in source %d.", NumActiveUniforms);
		
	glUseProgram(0);
	GL_REPORT_ERRORD();
}

void SetupVertexProgramParameters(VERTEXSHADER* pf, int context)
{
	GLint p;
	pf->prog.link = (void*)pf;			// Setting autolink
	pf->prog.isFragment = false;			// Setting autolink
	pf->ShaderType = ShaderTypes[pf->Shader];

	pf->ParametersStart = NumActiveUniforms;

	ZZshProgram prog = madeProgram(pf->Shader, 0, "");
	glUseProgram(prog);

	GL_REPORT_ERRORD();

	// Set Z-test, log or no log;
	if (conf.settings().no_logz) {
       		g_vdepth = float4( 255.0 /256.0f,  255.0/65536.0f, 255.0f/(65535.0f*256.0f), 1.0f/(65536.0f*65536.0f));
		vlogz = float4( 1.0f, 0.0f, 0.0f, 0.0f);
	}
	else {
		g_vdepth = float4( 256.0f*65536.0f, 65536.0f, 256.0f, 65536.0f*65536.0f);	
		vlogz = float4( 0.0f, 1.0f, 0.0f, 0.0f);
	}

	INIT_UNIFORMPARAM(g_vdepth, "g_fZ");
	if (p > -1) {
		INIT_UNIFORMPARAM(vlogz, "g_fZMin");
		if (p == -1)  ZZLog::Error_Log ("Shader file version is outdated! Only log-Z is possible.");
	}
	GL_REPORT_ERRORD();

	float4 vnorm = float4(g_filog32, 0, 0,0);
	INIT_UNIFORMPARAM(vnorm, "g_fZNorm");
	INIT_UNIFORMPARAM(float4(-0.2f, -0.65f, 0.9f, 1.0f / 32767.0f ),  "g_fBilinear");
	INIT_UNIFORMPARAM(float4(1.0f/256.0f, 1.0004f, 1, 0.5f),  "g_fZBias") ;
	INIT_UNIFORMPARAM(float4(0,1, 0.001f, 0.5f), "g_fc0");

	SET_UNIFORMPARAM(sBitBltPos, "g_fBitBltPos");
	SET_UNIFORMPARAM(sBitBltTex, "g_fBitBltTex");
	SET_UNIFORMPARAM(fBitBltTrans, "g_fBitBltTrans");
	pf->ParametersFinish = NumActiveUniforms;
	if (NumActiveUniforms > MAX_ACTIVE_UNIFORMS)
		ZZLog::Error_Log("Too many shader variables. You may increase the limit in the source.");

	glUseProgram(0);
	GL_REPORT_ERRORD();
}

const int GLSL_VERSION = 130;  			// Sampler2DRect appear in 1.3

// We use strictly compilation from source for GSLS
static __forceinline void GlslHeaderString(char* header_string, const char* name, const char* depth)
{
	sprintf(header_string, "#version %d\n#define %s main\n%s\n", GLSL_VERSION, name, depth);
}

static __forceinline bool LOAD_VS(char* DefineString, const char* name, VERTEXSHADER vertex, int shaderver, ZZshProfile context, const char* depth)
{
	bool flag;
	char temp[200];
	GlslHeaderString(temp, name, depth);
	sprintf(DefineString, "%s#define VERTEX_SHADER 1\n#define CTX %d\n", temp, context * NOCONTEXT);
	//ZZLog::WriteLn("Define for VS == '%s'", DefineString);
	flag = LoadShaderFromFile(vertex.Shader, DefineString, name, GL_VERTEX_SHADER);
	SetupVertexProgramParameters(&vertex, context);
	return flag;
}

static __forceinline bool LOAD_PS(char* DefineString, const char* name, FRAGMENTSHADER fragment, int shaderver, ZZshProfile context, const char* depth)
{
	bool flag;
	char temp[200];
	GlslHeaderString(temp, name, depth);
	sprintf(DefineString, "%s#define FRAGMENT_SHADER 1\n#define CTX %d\n", temp, context * NOCONTEXT);
	//ZZLog::WriteLn("Define for PS == '%s'", DefineString);
	flag = LoadShaderFromFile(fragment.Shader, DefineString, name, GL_FRAGMENT_SHADER);
	SetupFragmentProgramParameters(&fragment, context, 0); 
	return flag;
}

inline bool LoadEffects()
{
	// clear the textures
	for(u32 i = 0; i < ArraySize(ppsTexture); ++i) {
		SAFE_RELEASE_PROG(ppsTexture[i].prog);
	}

#ifndef _DEBUG
	memset(ppsTexture, 0, sizeof(ppsTexture));
#endif

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
	GLint p;
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

	GL_REPORT_ERRORD();
	return true;
}

const static char* g_pPsTexWrap[] = { "#define REPEAT 1\n", "#define CLAMP 1\n", "#define REGION_REPEAT 1\n", "" };

static ZZshShader LoadShaderFromType(const char* srcdir, const char* srcfile, int type, int texfilter, int texwrap, int fog, int writedepth, int testaem, int exactcolor, int ps, int context) {

	assert( texwrap < NUM_TEXWRAPS);
	assert( type < NUM_TYPES );
	//ZZLog::Error_Log("\n");

	ZZshProgram prog;

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

	ZZshShader shader;
	if (!CompileShader(shader, DefineString, name, GL_FRAGMENT_SHADER)) 
		return UseEmptyShader(name, GL_FRAGMENT_SHADER);

	ZZLog::Debug_Log("Used shader for type:%d filter:%d wrap:%d for:%d depth:%d aem:%d color:%d decompression:%d ctx:%d... Ok \n", type, texfilter, texwrap, fog, writedepth, testaem, exactcolor, ps, context);

	GL_REPORT_ERRORD();
	return shader;
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
	pf->Shader = LoadShaderFromType(EFFECT_DIR, EFFECT_NAME, type, texfilter, texwrap, fog, s_bWriteDepth, testaem, exactcolor, g_nPixelShaderVer, context);
	
	if (ZZshExistProgram(pf)) {
		SetupFragmentProgramParameters(pf, context, type);
		GL_REPORT_ERRORD();

		if( glGetError() != GL_NO_ERROR ) {
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

#endif // GLSL_API
