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

// Create and Destroy function. They would be called once per session.

//------------------ Includes
#include "GS.h"
#include "Mem.h"
#include "GLWin.h"
#include "ZZoglShaders.h"

#include "targets.h"
#include "rasterfont.h" // simple font
#include "ZZKick.h"
#include "ZZoglVB.h"

// This include for windows resource file with Shaders
#ifdef _WIN32
#	include "Win32.h"
#endif

//------------------ Defines

#ifdef _WIN32
#define GL_LOADFN(name) { \
		if( (*(void**)&name = (void*)wglGetProcAddress(#name)) == NULL ) { \
		ZZLog::Error_Log("Failed to find %s, exiting.", #name); \
	} \
}
#else
// let GLEW take care of it
#define GL_LOADFN(name)
#endif

#define GL_BLEND_RGB(src, dst) { \
	s_srcrgb = src; \
	s_dstrgb = dst; \
	zgsBlendFuncSeparateEXT(s_srcrgb, s_dstrgb, s_srcalpha, s_dstalpha); \
}

#define GL_BLEND_ALPHA(src, dst) { \
	s_srcalpha = src; \
	s_dstalpha = dst; \
	zgsBlendFuncSeparateEXT(s_srcrgb, s_dstrgb, s_srcalpha, s_dstalpha); \
}

#define GL_BLEND_ALL(srcrgb, dstrgb, srcalpha, dstalpha) { \
	s_srcrgb = srcrgb; \
	s_dstrgb = dstrgb; \
	s_srcalpha = srcalpha; \
	s_dstalpha = dstalpha; \
	zgsBlendFuncSeparateEXT(s_srcrgb, s_dstrgb, s_srcalpha, s_dstalpha); \
}

#define GL_BLEND_SET() zgsBlendFuncSeparateEXT(s_srcrgb, s_dstrgb, s_srcalpha, s_dstalpha)
#define VB_NUMBUFFERS			   512

// ----------------- Types
typedef void (APIENTRYP _PFNSWAPINTERVAL)(int);

map<string, GLbyte> mapGLExtensions;

extern bool LoadEffects();
extern bool ZZshLoadExtraEffects();
extern FRAGMENTSHADER* ZZshLoadShadeEffect(int type, int texfilter, int fog, int testaem, int exactcolor, const clampInfo& clamp, int context, bool* pbFailed);

GLuint vboRect = 0;
vector<GLuint> g_vboBuffers; // VBOs for all drawing commands
int g_nCurVBOIndex = 0;

inline bool CreateImportantCheck();
inline void CreateOtherCheck();
inline bool CreateOpenShadersFile();

void ZZGSStateReset();

//------------------ Dummies
#ifdef _WIN32
void __stdcall glBlendFuncSeparateDummy(GLenum e1, GLenum e2, GLenum e3, GLenum e4)
#else
void APIENTRY glBlendFuncSeparateDummy(GLenum e1, GLenum e2, GLenum e3, GLenum e4)
#endif
{
	glBlendFunc(e1, e2);
}

#ifdef _WIN32
void __stdcall glBlendEquationSeparateDummy(GLenum e1, GLenum e2)
#else
void APIENTRY glBlendEquationSeparateDummy(GLenum e1, GLenum e2)
#endif
{
	glBlendEquation(e1);
}

#ifdef _WIN32
extern HINSTANCE hInst;
void (__stdcall *zgsBlendEquationSeparateEXT)(GLenum, GLenum) = NULL;
void (__stdcall *zgsBlendFuncSeparateEXT)(GLenum, GLenum, GLenum, GLenum) = NULL;
#else
void (APIENTRY *zgsBlendEquationSeparateEXT)(GLenum, GLenum) = NULL;
void (APIENTRY *zgsBlendFuncSeparateEXT)(GLenum, GLenum, GLenum, GLenum) = NULL;
#endif

//------------------ variables
////////////////////////////
// State parameters

extern u8* s_lpShaderResources;

// String's for shader file in developer mode
#ifdef ZEROGS_DEVBUILD
char* EFFECT_NAME = "";
char* EFFECT_DIR = "";
#endif

/////////////////////
// graphics resources
GLenum s_srcrgb, s_dstrgb, s_srcalpha, s_dstalpha; // set by zgsBlendFuncSeparateEXT
u32 s_stencilfunc, s_stencilref, s_stencilmask;
GLenum s_drawbuffers[] = { GL_COLOR_ATTACHMENT0_EXT, GL_COLOR_ATTACHMENT1_EXT };

GLenum g_internalFloatFmt = GL_ALPHA_FLOAT32_ATI;
GLenum g_internalRGBAFloatFmt = GL_RGBA_FLOAT32_ATI;
GLenum g_internalRGBAFloat16Fmt = GL_RGBA_FLOAT16_ATI;

u32 ptexLogo = 0;
int nLogoWidth, nLogoHeight;
u32 s_ptexInterlace = 0;		 // holds interlace fields

//------------------ Global Variables
int GPU_TEXWIDTH = 512;
float g_fiGPU_TEXWIDTH = 1/512.0f;
int g_MaxTexWidth = 4096, g_MaxTexHeight = 4096;
u32 s_uFramebuffer = 0;

RasterFont* font_p = NULL;
float g_fBlockMult = 1;
//int s_nFullscreen = 0;

u32 ptexBlocks = 0, ptexConv16to32 = 0;	 // holds information on block tiling
u32 ptexBilinearBlocks = 0;
u32 ptexConv32to16 = 0;
int g_nDepthBias = 0;

extern void Delete_Avi_Capture();
extern void ZZDestroy();
extern void SetAA(int mode);

//------------------ Code

///< returns true if the the opengl extension is supported
bool IsGLExt(const char* szTargetExtension)
{
	return mapGLExtensions.find(string(szTargetExtension)) != mapGLExtensions.end();
}

// Function asks about different OGL extensions, that are required to setup accordingly. Return false if checks failed
inline bool CreateImportantCheck()
{
	bool bSuccess = true;
#ifndef _WIN32
	int const glew_ok = glewInit();

	if (glew_ok != GLEW_OK)
	{
		ZZLog::Error_Log("glewInit() is not ok!");
		bSuccess = false;
	}

#endif

	if (!IsGLExt("GL_EXT_framebuffer_object"))
	{
		ZZLog::Error_Log("*********\nZZogl: ERROR: Need GL_EXT_framebufer_object for multiple render targets\nZZogl: *********");
		bSuccess = false;
	}

	if (!IsGLExt("GL_EXT_secondary_color"))
	{
		ZZLog::Error_Log("*********\nZZogl: OGL WARNING: Need GL_EXT_secondary_color\nZZogl: *********");
		bSuccess = false;
	}
	
	bSuccess &= ZZshCheckProfilesSupport();
	
	return bSuccess;
}

// This is a check for less important open gl extensions.
inline void CreateOtherCheck()
{
	if (!IsGLExt("GL_EXT_blend_equation_separate") || glBlendEquationSeparateEXT == NULL)
	{
		ZZLog::Error_Log("*********\nZZogl: OGL WARNING: Need GL_EXT_blend_equation_separate\nZZogl: *********");
		zgsBlendEquationSeparateEXT = glBlendEquationSeparateDummy;
	}
	else
		zgsBlendEquationSeparateEXT = glBlendEquationSeparateEXT;

	if (!IsGLExt("GL_EXT_blend_func_separate") || glBlendFuncSeparateEXT == NULL)
	{
		ZZLog::Error_Log("*********\nZZogl: OGL WARNING: Need GL_EXT_blend_func_separate\nZZogl: *********");
		zgsBlendFuncSeparateEXT = glBlendFuncSeparateDummy;
	}
	else
		zgsBlendFuncSeparateEXT = glBlendFuncSeparateEXT;

	if (!IsGLExt("GL_ARB_draw_buffers") && !IsGLExt("GL_ATI_draw_buffers"))
	{
		ZZLog::Error_Log("*********\nZZogl: OGL WARNING: multiple render targets not supported, some effects might look bad\nZZogl: *********");
		conf.mrtdepth = 0;
	}

	if (IsGLExt("GL_ARB_draw_buffers"))
		glDrawBuffers = (PFNGLDRAWBUFFERSPROC)wglGetProcAddress("glDrawBuffers");
	else if (IsGLExt("GL_ATI_draw_buffers"))
		glDrawBuffers = (PFNGLDRAWBUFFERSPROC)wglGetProcAddress("glDrawBuffersATI");


	if (!IsGLExt("GL_ARB_multitexture"))
		ZZLog::Error_Log("No multitexturing.");
	else
		ZZLog::Error_Log("Using multitexturing.");

	GLint Max_Texture_Size_NV = 0;

	GLint Max_Texture_Size_2d = 0;

	glGetIntegerv(GL_MAX_RECTANGLE_TEXTURE_SIZE_NV, &Max_Texture_Size_NV);

	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &Max_Texture_Size_2d);

	ZZLog::Error_Log("Maximum texture size is %d for Tex_2d and %d for Tex_NV.", Max_Texture_Size_2d, Max_Texture_Size_NV);

	if (Max_Texture_Size_NV < 1024)
		ZZLog::Error_Log("Could not properly make bitmasks, so some textures will be missed.");

#ifdef _WIN32
	if (IsGLExt("WGL_EXT_swap_control") || IsGLExt("EXT_swap_control"))
		wglSwapIntervalEXT(0);

#else
	if (IsGLExt("GLX_SGI_swap_control"))
	{
		_PFNSWAPINTERVAL swapinterval = (_PFNSWAPINTERVAL)wglGetProcAddress("glXSwapInterval");

		if (!swapinterval)
			swapinterval = (_PFNSWAPINTERVAL)wglGetProcAddress("glXSwapIntervalSGI");

		if (!swapinterval)
			swapinterval = (_PFNSWAPINTERVAL)wglGetProcAddress("glXSwapIntervalEXT");

		if (swapinterval)
			swapinterval(0);
		else
			ZZLog::Error_Log("No support for SwapInterval (framerate clamped to monitor refresh rate),");
	}

#endif
}


#ifdef _WIN32
__forceinline bool LoadShadersFromRes()
{
	HRSRC hShaderSrc = FindResource(hInst, MAKEINTRESOURCE(IDR_SHADERS), RT_RCDATA);
	assert(hShaderSrc != NULL);
	HGLOBAL hShaderGlob = LoadResource(hInst, hShaderSrc);
	assert(hShaderGlob != NULL);
	s_lpShaderResources = (u8*)LockResource(hShaderGlob);
	return true;
}
#else

__forceinline bool LoadShadersFromDat()
{
	FILE* fres = fopen("ps2hw.dat", "rb");

	if (fres == NULL)
	{
		fres = fopen("plugins/ps2hw.dat", "rb");

		if (fres == NULL)
		{
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
	
	return true;
}

__forceinline bool LoadShadersFromFX()
{
	// test if ps2hw.fx exists
	char tempstr[255];
	char curwd[255];
	getcwd(curwd, ArraySize(curwd));

	strcpy(tempstr, "/plugins/");
	sprintf(EFFECT_NAME, "%sps2hw.fx", tempstr);
	FILE* f = fopen(EFFECT_NAME, "r");

	if (f == NULL)
	{

		strcpy(tempstr, "../../plugins/zzogl-pg/opengl/");
		sprintf(EFFECT_NAME, "%sps2hw.fx", tempstr);
		f = fopen(EFFECT_NAME, "r");

		if (f == NULL)
		{
			ZZLog::Error_Log("Failed to find %s, try compiling a non-devbuild.", EFFECT_NAME);
			return false;
		}
	}

	fclose(f);

	sprintf(EFFECT_DIR, "%s/%s", curwd, tempstr);
	sprintf(EFFECT_NAME, "%sps2hw.fx", EFFECT_DIR);
	
	return true;
}
#endif


// open shader file according to build target

inline bool CreateOpenShadersFile()
{
#ifndef DEVBUILD
#	ifdef _WIN32
	return LoadShadersFromRes();
#	else // not _WIN32
	return LoadShadersFromDat();
#	endif // _WIN32
#else // defined(ZEROGS_DEVBUILD)
#	ifndef _WIN32 // NOT WINDOWS
	return LoadShadersFromFX();
	
	// No else clause?
#endif
#endif // !defined(ZEROGS_DEVBUILD)
}

// Read all extensions name and fill mapGLExtensions
inline bool CreateFillExtensionsMap()
{
	string temp("");
	int max_ext = 0;
	glGetIntegerv(GL_NUM_EXTENSIONS, &max_ext);
	
	PFNGLGETSTRINGIPROC glGetStringi = 0;
	glGetStringi = (PFNGLGETSTRINGIPROC)wglGetProcAddress("glGetStringi");

	for (GLint i = 0; i < max_ext; i++)
	{
		string extension((const char*)glGetStringi(GL_EXTENSIONS, i));
		mapGLExtensions[extension];
		
		temp = temp + extension;
		if (i != (max_ext - 1)) temp += ", ";
	}
	
	// Write the extension list to the log, but only write it to the screen on a debug build.
#ifndef _DEBUG
	ZZLog::Log("%d supported OpenGL Extensions: %s\n", max_ext, temp.c_str());
#endif
	ZZLog::Debug_Log("%d supported OpenGL Extensions: %s\n", max_ext, temp.c_str());

	return true;
}

void LoadglFunctions()
{
	GL_LOADFN(glIsRenderbufferEXT);
	GL_LOADFN(glBindRenderbufferEXT);
	GL_LOADFN(glDeleteRenderbuffersEXT);
	GL_LOADFN(glGenRenderbuffersEXT);
	GL_LOADFN(glRenderbufferStorageEXT);
	GL_LOADFN(glGetRenderbufferParameterivEXT);
	GL_LOADFN(glIsFramebufferEXT);
	GL_LOADFN(glBindFramebufferEXT);
	GL_LOADFN(glDeleteFramebuffersEXT);
	GL_LOADFN(glGenFramebuffersEXT);
	GL_LOADFN(glCheckFramebufferStatusEXT);
	GL_LOADFN(glFramebufferTexture1DEXT);
	GL_LOADFN(glFramebufferTexture2DEXT);
	GL_LOADFN(glFramebufferTexture3DEXT);
	GL_LOADFN(glFramebufferRenderbufferEXT);
	GL_LOADFN(glGetFramebufferAttachmentParameterivEXT);
	GL_LOADFN(glGenerateMipmapEXT);
}

inline bool TryBlockFormat(GLint fmt, const GLvoid* vBlockData) {
	g_internalFloatFmt = fmt; 
	glTexImage2D(GL_TEXTURE_2D, 0, g_internalFloatFmt, BLOCK_TEXWIDTH, BLOCK_TEXHEIGHT, 0, GL_ALPHA, GL_FLOAT, vBlockData);
	return (glGetError() == GL_NO_ERROR);
}

inline bool TryBlinearFormat(GLint fmt32, GLint fmt16, const GLvoid* vBilinearData) {
	g_internalRGBAFloatFmt = fmt32;
	g_internalRGBAFloat16Fmt = fmt16;
	glTexImage2D(GL_TEXTURE_2D, 0, g_internalRGBAFloatFmt, BLOCK_TEXWIDTH, BLOCK_TEXHEIGHT, 0, GL_RGBA, GL_FLOAT, vBilinearData);
	return (glGetError() == GL_NO_ERROR);
}


bool ZZCreate(int _width, int _height)
{
	GLenum err = GL_NO_ERROR;
	bool bSuccess = true;

	ZZDestroy();
	ZZGSStateReset();
	
	if (!GLWin.DisplayWindow(_width, _height)) return false;

	conf.mrtdepth = 0; // for now

	if (!CreateFillExtensionsMap()) return false;
	if (!CreateImportantCheck()) return false;

	CreateOtherCheck();

	// check the max texture width and height
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &g_MaxTexWidth);
    // Limit the texture size supported to 8192. We do not need bigger texture.
    // Besides the following assertion is false when texture are too big.
    // ZZoglFlush.cpp:2349:	assert(fblockstride >= 1.0f)
    //g_MaxTexWidth = min(8192, g_MaxTexWidth);

	g_MaxTexHeight = g_MaxTexWidth / 4;
	GPU_TEXWIDTH = min (g_MaxTexWidth/8, 1024);
	g_fiGPU_TEXWIDTH = 1.0f / GPU_TEXWIDTH;

	if (!CreateOpenShadersFile()) return false;

	GL_REPORT_ERROR();

	if (err != GL_NO_ERROR) bSuccess = false;

	s_srcrgb = s_dstrgb = s_srcalpha = s_dstalpha = GL_ONE;

	LoadglFunctions();

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	GL_REPORT_ERROR();

	if (err != GL_NO_ERROR) bSuccess = false;

	glGenFramebuffersEXT(1, &s_uFramebuffer);

	if (s_uFramebuffer == 0)
	{
		ZZLog::Error_Log("Failed to create the renderbuffer.");
	}

	GL_REPORT_ERRORD();

	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, s_uFramebuffer);

	DrawBuffers(s_drawbuffers);
		
	GL_REPORT_ERROR();

	if (err != GL_NO_ERROR) bSuccess = false;

	font_p = new RasterFont();
	GL_REPORT_ERROR();

	if (err != GL_NO_ERROR) bSuccess = false;

	// init draw fns
	//init_drawfn();
	if (ZZKick != NULL) delete ZZKick;
	ZZKick = new Kick;
	
	SetAA(conf.aa);

	GSsetGameCRC(g_LastCRC, conf.settings()._u32);

	GL_STENCILFUNC(GL_ALWAYS, 0, 0);

	//s_bWriteDepth = true;

	GL_BLEND_ALL(GL_ONE, GL_ONE, GL_ONE, GL_ONE);
	glViewport(0, 0, GLWin.backbuffer.w, GLWin.backbuffer.h);					 // Reset The Current Viewport
	
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	
	glShadeModel(GL_SMOOTH);
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClearDepth(1.0f);
	
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_LIGHTING);
	
	glDepthFunc(GL_LEQUAL);
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);  // Really Nice Perspective Calculations
	glGenTextures(1, &ptexLogo);
	glBindTexture(GL_TEXTURE_RECTANGLE_NV, ptexLogo);

#ifdef _WIN32
	HRSRC hBitmapSrc = FindResource(hInst, MAKEINTRESOURCE(IDB_ZEROGSLOGO), RT_BITMAP);
	assert(hBitmapSrc != NULL);

	HGLOBAL hBitmapGlob = LoadResource(hInst, hBitmapSrc);
	assert(hBitmapGlob != NULL);

	PBITMAPINFO pinfo = (PBITMAPINFO)LockResource(hBitmapGlob);

	GLenum tempFmt = (pinfo->bmiHeader.biBitCount == 32) ? GL_RGBA : GL_RGB;
	TextureRect(GL_RGBA, pinfo->bmiHeader.biWidth, pinfo->bmiHeader.biHeight, tempFmt, GL_UNSIGNED_BYTE, (u8*)pinfo + pinfo->bmiHeader.biSize);

	nLogoWidth = pinfo->bmiHeader.biWidth;
	nLogoHeight = pinfo->bmiHeader.biHeight;

	setRectFilters(GL_LINEAR);

#else
#endif

	GL_REPORT_ERROR();

	g_nCurVBOIndex = 0;

	g_vboBuffers.resize(VB_NUMBUFFERS);
	glGenBuffers((GLsizei)g_vboBuffers.size(), &g_vboBuffers[0]);

	for (int i = 0; i < (int)g_vboBuffers.size(); ++i)
	{
		glBindBuffer(GL_ARRAY_BUFFER, g_vboBuffers[i]);
		glBufferData(GL_ARRAY_BUFFER, 0x100*sizeof(VertexGPU), NULL, GL_STREAM_DRAW);
	}

	GL_REPORT_ERROR();
	if (err != GL_NO_ERROR) bSuccess = false;

	// create the blocks texture
	g_fBlockMult = 1;
	bool do_not_use_billinear = false;

	vector<char> vBlockData, vBilinearData;
	BLOCK::FillBlocks(vBlockData, vBilinearData, 1);

	glGenTextures(1, &ptexBlocks);
	glBindTexture(GL_TEXTURE_2D, ptexBlocks);
	
	if 	(TryBlockFormat(GL_RGBA32F, &vBlockData[0]))
		ZZLog::Error_Log("Use GL_RGBA32F for blockdata.");
	else if (TryBlockFormat(GL_ALPHA_FLOAT32_ATI, &vBlockData[0]))
	 	ZZLog::Error_Log("Use ATI_texture_float for blockdata.");
	else if (TryBlockFormat(GL_ALPHA32F_ARB, &vBlockData[0]))
		ZZLog::Error_Log("Use ARB_texture_float for blockdata.");
	else 
		{	// This case is bad. But for really old cards it could be nice. 			
		g_fBlockMult = 65535.0f*(float)g_fiGPU_TEXWIDTH;
		BLOCK::FillBlocks(vBlockData, vBilinearData, 0);
		g_internalFloatFmt = GL_ALPHA16;
		
		// We store block data on u16 rather float numbers. It's not so preciese, but ALPHA16 is OpenGL 2.0 standart
		// and use only 16 bit. Old zerogs use red channel, but it does not work.
		
		glTexImage2D(GL_TEXTURE_2D, 0, g_internalFloatFmt, BLOCK_TEXWIDTH, BLOCK_TEXHEIGHT, 0, GL_ALPHA, GL_UNSIGNED_SHORT, &vBlockData[0]);
		if( glGetError() != GL_NO_ERROR ) {
			ZZLog::Error_Log("ZZogl ERROR: could not fill blocks");
			return false;
 		}
	
		do_not_use_billinear = true;
		conf.bilinear = 0;
		ZZLog::Error_Log("Using non-bilinear fill, quallity is outdated!");	
	}

	setTex2DFilters(GL_NEAREST);
	setTex2DWrap(GL_REPEAT);

	if (!do_not_use_billinear) 
	{
		// fill in the bilinear blocks (main variant).
		glGenTextures(1, &ptexBilinearBlocks);
		glBindTexture(GL_TEXTURE_2D, ptexBilinearBlocks);
		
		if 	(TryBlinearFormat(GL_RGBA32F, GL_RGBA16F, &vBilinearData[0]))
			ZZLog::Error_Log("Fill bilinear blocks OK.!");
		else if (TryBlinearFormat(GL_RGBA_FLOAT32_ATI, GL_RGBA_FLOAT16_ATI, &vBilinearData[0]))
			ZZLog::Error_Log("Fill bilinear blocks with ATI_texture_float.");
		else if (TryBlinearFormat(GL_FLOAT_RGBA32_NV, GL_FLOAT_RGBA16_NV, &vBilinearData[0]))
			ZZLog::Error_Log("ZZogl Fill bilinear blocks with NVidia_float.");
		else 
			ZZLog::Error_Log("Fill bilinear blocks failed.");

		setTex2DFilters(GL_NEAREST);
		setTex2DWrap(GL_REPEAT);
	}

	float fpri = 1;

	glPrioritizeTextures(1, &ptexBlocks, &fpri);

	if (ptexBilinearBlocks != 0) glPrioritizeTextures(1, &ptexBilinearBlocks, &fpri);

	GL_REPORT_ERROR();

	// fill a simple rect
	glGenBuffers(1, &vboRect);
	glBindBuffer(GL_ARRAY_BUFFER, vboRect);

	vector<VertexGPU> verts(4);

	VertexGPU* pvert = &verts[0];

	pvert->set_xyzst(-0x7fff, 0x7fff, 0, 0, 0);
	pvert++;

	pvert->set_xyzst(0x7fff, 0x7fff, 0, 1, 0);
	pvert++;

	pvert->set_xyzst(-0x7fff, -0x7fff, 0, 0, 1);
	pvert++;

	pvert->set_xyzst(0x7fff, -0x7fff, 0, 1, 1);
	pvert++;

	glBufferDataARB(GL_ARRAY_BUFFER, 4*sizeof(VertexGPU), &verts[0], GL_STATIC_DRAW);

	// setup the default vertex declaration
	glEnableClientState(GL_VERTEX_ARRAY);
	glClientActiveTexture(GL_TEXTURE0);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnableClientState(GL_SECONDARY_COLOR_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);
	GL_REPORT_ERROR();

	// some cards don't support this
//	glClientActiveTexture(GL_TEXTURE0);
//	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
//	glTexCoordPointer(4, GL_UNSIGNED_BYTE, sizeof(VertexGPU), (void*)12);

	// create the conversion textures
	glGenTextures(1, &ptexConv16to32);
	glBindTexture(GL_TEXTURE_2D, ptexConv16to32);

	vector<u32> conv16to32data(256*256);

	for (int i = 0; i < 256*256; ++i)
	{
		u32 tempcol = RGBA16to32(i);
		// have to flip r and b
		conv16to32data[i] = (tempcol & 0xff00ff00) | ((tempcol & 0xff) << 16) | ((tempcol & 0xff0000) >> 16);
	}

	Texture2D(4, 256, 256, GL_RGBA, GL_UNSIGNED_BYTE, &conv16to32data[0]);

	setTex2DFilters(GL_NEAREST);
	setTex2DWrap(GL_CLAMP);

	GL_REPORT_ERROR();

	if (err != GL_NO_ERROR) bSuccess = false;

	vector<u32> conv32to16data(32*32*32);

	glGenTextures(1, &ptexConv32to16);

	glBindTexture(GL_TEXTURE_3D, ptexConv32to16);

	u32* dst = &conv32to16data[0];

	for (int i = 0; i < 32; ++i)
	{
		for (int j = 0; j < 32; ++j)
		{
			for (int k = 0; k < 32; ++k)
			{
				u32 col = (i << 10) | (j << 5) | k;
				*dst++ = ((col & 0xff) << 16) | (col & 0xff00);
			}
		}
	}

	Texture3D(4, 32, 32, 32, GL_RGBA, GL_UNSIGNED_BYTE, &conv32to16data[0]);
	setTex3DFilters(GL_NEAREST);
	setTex3DWrap(GL_CLAMP);
	GL_REPORT_ERROR();

	if (err != GL_NO_ERROR) bSuccess = false;

	if (!ZZshStartUsingShaders())  bSuccess = false;

	GL_REPORT_ERROR();

	if (err != GL_NO_ERROR) bSuccess = false;

	glDisable(GL_STENCIL_TEST);
	glEnable(GL_SCISSOR_TEST);

	GL_BLEND_ALPHA(GL_ONE, GL_ZERO);

	glBlendColorEXT(0, 0, 0, 0.5f);

	glDisable(GL_CULL_FACE);

	// points
	// This was changed in SetAA - should we be changing it back?
	glPointSize(1.0f);

	g_nDepthBias = 0;

	glEnable(GL_POLYGON_OFFSET_FILL);
	glEnable(GL_POLYGON_OFFSET_LINE);

	glPolygonOffset(0, 1);

	vb[0].Init(VB_BUFFERSIZE);
	vb[1].Init(VB_BUFFERSIZE);

	g_vsprog = g_psprog = 0;

	if (glGetError() == GL_NO_ERROR)
	{
		return bSuccess;
	}
	else
	{
		ZZLog::Debug_Log("In final init!");
		return false;
	}
}

void ZZDestroy()
{
	Delete_Avi_Capture();

	g_MemTargs.Destroy();

	s_RTs.Destroy();
	s_DepthRTs.Destroy();
	s_BitwiseTextures.Destroy();

	SAFE_RELEASE_TEX(s_ptexInterlace);
	SAFE_RELEASE_TEX(ptexBlocks);
	SAFE_RELEASE_TEX(ptexBilinearBlocks);
	SAFE_RELEASE_TEX(ptexConv16to32);
	SAFE_RELEASE_TEX(ptexConv32to16);

	vb[0].Destroy();
	vb[1].Destroy();

	if (g_vboBuffers.size() > 0)
	{
		glDeleteBuffers((GLsizei)g_vboBuffers.size(), &g_vboBuffers[0]);
		g_vboBuffers.clear();
	}

	g_nCurVBOIndex = 0;
	
	if (pvs != NULL)
	{
		for (int i = 0; i < ArraySize(pvs); ++i)
		{
			SAFE_RELEASE_PROG(pvs[i]);
		}
	}

	if (ppsRegular != NULL)
	{
		for (int i = 0; i < ArraySize(ppsRegular); ++i)
		{
			SAFE_RELEASE_PROG(ppsRegular[i].prog);
		}
	}

	if (ppsTexture != NULL)
	{
		for (int i = 0; i < ArraySize(ppsTexture); ++i)
		{
			SAFE_RELEASE_PROG(ppsTexture[i].prog);
		}
	}

	SAFE_RELEASE_PROG(pvsBitBlt.prog);

	SAFE_RELEASE_PROG(ppsBitBlt[0].prog);
	SAFE_RELEASE_PROG(ppsBitBlt[1].prog);
	SAFE_RELEASE_PROG(ppsBitBltDepth.prog);
	SAFE_RELEASE_PROG(ppsCRTCTarg[0].prog);
	SAFE_RELEASE_PROG(ppsCRTCTarg[1].prog);
	SAFE_RELEASE_PROG(ppsCRTC[0].prog);
	SAFE_RELEASE_PROG(ppsCRTC[1].prog);
	SAFE_RELEASE_PROG(ppsCRTC24[0].prog);
	SAFE_RELEASE_PROG(ppsCRTC24[1].prog);
	SAFE_RELEASE_PROG(ppsOne.prog);

	safe_delete(font_p);

	GLWin.ReleaseContext();

	mapGLExtensions.clear();
}

