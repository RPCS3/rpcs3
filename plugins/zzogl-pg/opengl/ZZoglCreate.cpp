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

// Create and Destroy function. They would be called once per session. 

//------------------ Includes
#include "GS.h"
#include "Mem.h"
#include "zerogs.h"

#include "ZeroGSShaders/zerogsshaders.h"
#include "targets.h"
// This include for windows resource file with Shaders
#ifdef _WIN32
#	include "Win32.h"
#endif

//------------------ Defines

#ifdef _WIN32
#define GL_LOADFN(name) { \
		if( (*(void**)&name = (void*)wglGetProcAddress(#name)) == NULL ) { \
		ERROR_LOG("Failed to find %s, exiting\n", #name); \
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

#define GL_STENCILFUNC(func, ref, mask) { \
	s_stencilfunc  = func; \
	s_stencilref = ref; \
	s_stencilmask = mask; \
	glStencilFunc(func, ref, mask); \
}

#define GL_STENCILFUNC_SET() glStencilFunc(s_stencilfunc, s_stencilref, s_stencilmask)

#define VB_BUFFERSIZE			   0x400
#define VB_NUMBUFFERS			   512

// ----------------- Types
typedef void (APIENTRYP _PFNSWAPINTERVAL) (int);

map<string, GLbyte> mapGLExtensions;

namespace ZeroGS{
	RenderFormatType g_RenderFormatType = RFT_float16;

	extern void KickPoint();
	extern void KickLine();
	extern void KickTriangle();
	extern void KickTriangleFan();
	extern void KickSprite();
	extern void KickDummy();
	extern bool LoadEffects();
	extern bool LoadExtraEffects();
	extern FRAGMENTSHADER* LoadShadeEffect(int type, int texfilter, int fog, int testaem, int exactcolor, const clampInfo& clamp, int context, bool* pbFailed);
	VERTEXSHADER pvsBitBlt;

	GLuint vboRect = 0;
	vector<GLuint> g_vboBuffers; // VBOs for all drawing commands
	int g_nCurVBOIndex = 0;

	inline bool CreateImportantCheck(); 
	inline void CreateOtherCheck();
	inline bool CreateOpenShadersFile();
}

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
float fiRendWidth, fiRendHeight; 

u8* s_lpShaderResources = NULL;
CGprogram pvs[16] = {NULL};

// String's for shader file in developer's mode
#ifdef DEVBUILD
char* EFFECT_NAME="";
char* EFFECT_DIR="";
#endif 

/////////////////////
// graphics resources
FRAGMENTSHADER ppsRegular[4], ppsTexture[NUM_SHADERS];
FRAGMENTSHADER ppsCRTC[2], ppsCRTC24[2], ppsCRTCTarg[2];
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
int g_MaxTexWidth = 4096, g_MaxTexHeight = 4096;
u32 s_uFramebuffer = 0;
CGprofile cgvProf, cgfProf;
int g_nPixelShaderVer = 0; // default

RasterFont* font_p = NULL;
float g_fBlockMult = 1;
int s_nFullscreen = 0;

u32 ptexBlocks = 0, ptexConv16to32 = 0;	 // holds information on block tiling
u32 ptexBilinearBlocks = 0;
u32 ptexConv32to16 = 0;
BOOL g_bDisplayMsg = 1;
int g_nDepthBias = 0;
BOOL g_bSaveFlushedFrame = 0;

//------------------ Code

bool ZeroGS::IsGLExt( const char* szTargetExtension )
{
	return mapGLExtensions.find(string(szTargetExtension)) != mapGLExtensions.end();
}

inline bool
ZeroGS::Create_Window(int _width, int _height) {
	nBackbufferWidth = _width;
	nBackbufferHeight = _height;
	fiRendWidth = 1.0f / nBackbufferWidth;
	fiRendHeight = 1.0f / nBackbufferHeight;
#ifdef _WIN32

	GLuint	  PixelFormat;			// Holds The Results After Searching For A Match
	DWORD	   dwExStyle;			  // Window Extended Style
	DWORD	   dwStyle;				// Window Style

	RECT rcdesktop;
	GetWindowRect(GetDesktopWindow(), &rcdesktop);

	if( conf.options & GSOPTION_FULLSCREEN) {
		nBackbufferWidth = rcdesktop.right - rcdesktop.left;
		nBackbufferHeight = rcdesktop.bottom - rcdesktop.top;

		dwExStyle=WS_EX_APPWINDOW;
		dwStyle=WS_POPUP;
		ShowCursor(FALSE);
	}
	else {
		dwExStyle=WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
		dwStyle=WS_OVERLAPPEDWINDOW;
	}

	RECT rc;
	rc.left = 0; rc.top = 0;
	rc.right = nBackbufferWidth; rc.bottom = nBackbufferHeight;
	AdjustWindowRectEx(&rc, dwStyle, FALSE, dwExStyle);
	int X = (rcdesktop.right-rcdesktop.left)/2 - (rc.right-rc.left)/2;
	int Y = (rcdesktop.bottom-rcdesktop.top)/2 - (rc.bottom-rc.top)/2;

	SetWindowLong( GShwnd, GWL_STYLE, dwStyle );
	SetWindowLong( GShwnd, GWL_EXSTYLE, dwExStyle );

	SetWindowPos(GShwnd, HWND_TOP, X, Y, rc.right-rc.left, rc.bottom-rc.top, SWP_SHOWWINDOW);
		
	if (conf.options & GSOPTION_FULLSCREEN) {
		DEVMODE dmScreenSettings;
		memset(&dmScreenSettings,0,sizeof(dmScreenSettings));
		dmScreenSettings.dmSize=sizeof(dmScreenSettings);
		dmScreenSettings.dmPelsWidth	= nBackbufferWidth;
		dmScreenSettings.dmPelsHeight   = nBackbufferHeight;
		dmScreenSettings.dmBitsPerPel   = 32;
		dmScreenSettings.dmFields=DM_BITSPERPEL|DM_PELSWIDTH|DM_PELSHEIGHT;

		// Try To Set Selected Mode And Get Results.  NOTE: CDS_FULLSCREEN Gets Rid Of Start Bar.
		if (ChangeDisplaySettings(&dmScreenSettings,CDS_FULLSCREEN)!=DISP_CHANGE_SUCCESSFUL)
		{
			if (MessageBox(NULL,"The Requested Fullscreen Mode Is Not Supported By\nYour Video Card. Use Windowed Mode Instead?","NeHe GL",MB_YESNO|MB_ICONEXCLAMATION)==IDYES)
				conf.options &= ~GSOPTION_FULLSCREEN;
			else
				return false;
		}
	}
	else {
		// change to default resolution
		ChangeDisplaySettings(NULL, 0);
	}

	PIXELFORMATDESCRIPTOR pfd=			  // pfd Tells Windows How We Want Things To Be
	{
		sizeof(PIXELFORMATDESCRIPTOR),			  // Size Of This Pixel Format Descriptor
		1,										  // Version Number
		PFD_DRAW_TO_WINDOW |						// Format Must Support Window
		PFD_SUPPORT_OPENGL |						// Format Must Support OpenGL
		PFD_DOUBLEBUFFER,						   // Must Support Double Buffering
		PFD_TYPE_RGBA,							  // Request An RGBA Format
		32,										 // Select Our Color Depth
		0, 0, 0, 0, 0, 0,						   // Color Bits Ignored
		0,										  // 8bit Alpha Buffer
		0,										  // Shift Bit Ignored
		0,										  // No Accumulation Buffer
		0, 0, 0, 0,								 // Accumulation Bits Ignored
		24,										 // 24Bit Z-Buffer (Depth Buffer)  
		8,										  // 8bit Stencil Buffer
		0,										  // No Auxiliary Buffer
		PFD_MAIN_PLANE,							 // Main Drawing Layer
		0,										  // Reserved
		0, 0, 0									 // Layer Masks Ignored
	};
	
	if (!(hDC=GetDC(GShwnd))) {
		MessageBox(NULL,"(1) Can't Create A GL Device Context.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return false;
	}
	
	if (!(PixelFormat=ChoosePixelFormat(hDC,&pfd))) {
		MessageBox(NULL,"(2) Can't Find A Suitable PixelFormat.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return false;
	}

	if(!SetPixelFormat(hDC,PixelFormat,&pfd)) {
		MessageBox(NULL,"(3) Can't Set The PixelFormat.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return false;
	}

	if (!(hRC=wglCreateContext(hDC))) {
		MessageBox(NULL,"(4) Can't Create A GL Rendering Context.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return false;
	}

	if(!wglMakeCurrent(hDC,hRC)) {
		MessageBox(NULL,"(5) Can't Activate The GL Rendering Context.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return false;
	}
	
	UpdateWindow(GShwnd);
#else //NOT _WIN32
	int i;
	XVisualInfo *vi;
	Colormap cmap;
	int dpyWidth, dpyHeight;
	int glxMajorVersion, glxMinorVersion;
	int vidModeMajorVersion, vidModeMinorVersion;
	Atom wmDelete;
	Window winDummy;
	unsigned int borderDummy;
	
	// attributes for a single buffered visual in RGBA format with at least
	// 8 bits per color and a 24 bit depth buffer
	int attrListSgl[] = {GLX_RGBA, GLX_RED_SIZE, 8, 
						 GLX_GREEN_SIZE, 8, 
						 GLX_BLUE_SIZE, 8, 
						 GLX_DEPTH_SIZE, 24,
						 None};

	// attributes for a double buffered visual in RGBA format with at least 
	// 8 bits per color and a 24 bit depth buffer
	int attrListDbl[] = { GLX_RGBA, GLX_DOUBLEBUFFER, 
						  GLX_RED_SIZE, 8, 
						  GLX_GREEN_SIZE, 8, 
						  GLX_BLUE_SIZE, 8, 
						  GLX_DEPTH_SIZE, 24,
						  None };

	GLWin.fs = !!(conf.options & GSOPTION_FULLSCREEN);
	
	/* get an appropriate visual */
	vi = glXChooseVisual(GLWin.dpy, GLWin.screen, attrListDbl);
	if (vi == NULL) {
		vi = glXChooseVisual(GLWin.dpy, GLWin.screen, attrListSgl);
		GLWin.doubleBuffered = False;
		ERROR_LOG("Only Singlebuffered Visual!\n");
	}
	else {
		GLWin.doubleBuffered = True;
		ERROR_LOG("Got Doublebuffered Visual!\n");
	}

	glXQueryVersion(GLWin.dpy, &glxMajorVersion, &glxMinorVersion);
	ERROR_LOG("glX-Version %d.%d\n", glxMajorVersion, glxMinorVersion);
	/* create a GLX context */
	GLWin.ctx = glXCreateContext(GLWin.dpy, vi, 0, GL_TRUE);
	/* create a color map */
	cmap = XCreateColormap(GLWin.dpy, RootWindow(GLWin.dpy, vi->screen),
						   vi->visual, AllocNone);
	GLWin.attr.colormap = cmap;
	GLWin.attr.border_pixel = 0;

	// get a connection
	XF86VidModeQueryVersion(GLWin.dpy, &vidModeMajorVersion, &vidModeMinorVersion);

	if (GLWin.fs) {
		
		XF86VidModeModeInfo **modes = NULL;
		int modeNum = 0;
		int bestMode = 0;

		// set best mode to current
		bestMode = 0;
		ERROR_LOG("XF86VidModeExtension-Version %d.%d\n", vidModeMajorVersion, vidModeMinorVersion);
		XF86VidModeGetAllModeLines(GLWin.dpy, GLWin.screen, &modeNum, &modes);
		
		if( modeNum > 0 && modes != NULL ) {
			/* save desktop-resolution before switching modes */
			GLWin.deskMode = *modes[0];
			/* look for mode with requested resolution */
			for (i = 0; i < modeNum; i++) {
				if ((modes[i]->hdisplay == _width) && (modes[i]->vdisplay == _height)) {
					bestMode = i;
				}
			}	

			XF86VidModeSwitchToMode(GLWin.dpy, GLWin.screen, modes[bestMode]);
			XF86VidModeSetViewPort(GLWin.dpy, GLWin.screen, 0, 0);
			dpyWidth = modes[bestMode]->hdisplay;
			dpyHeight = modes[bestMode]->vdisplay;
			ERROR_LOG("Resolution %dx%d\n", dpyWidth, dpyHeight);
			XFree(modes);
			
			/* create a fullscreen window */
			GLWin.attr.override_redirect = True;
			GLWin.attr.event_mask = ExposureMask | KeyPressMask | ButtonPressMask |
				StructureNotifyMask;
			GLWin.win = XCreateWindow(GLWin.dpy, RootWindow(GLWin.dpy, vi->screen),
									  0, 0, dpyWidth, dpyHeight, 0, vi->depth, InputOutput, vi->visual,
									  CWBorderPixel | CWColormap | CWEventMask | CWOverrideRedirect,
									  &GLWin.attr);
			XWarpPointer(GLWin.dpy, None, GLWin.win, 0, 0, 0, 0, 0, 0);
			XMapRaised(GLWin.dpy, GLWin.win);
			XGrabKeyboard(GLWin.dpy, GLWin.win, True, GrabModeAsync,
						  GrabModeAsync, CurrentTime);
			XGrabPointer(GLWin.dpy, GLWin.win, True, ButtonPressMask,
						 GrabModeAsync, GrabModeAsync, GLWin.win, None, CurrentTime);
		}
		else {
			ERROR_LOG("Failed to start fullscreen. If you received the \n"
					  "\"XFree86-VidModeExtension\" extension is missing, add\n"
					  "Load \"extmod\"\n"
					  "to your X configuration file (under the Module Section)\n");
			GLWin.fs = 0;
		}
	}
	
	
	if( !GLWin.fs ) {

		//XRootWindow(dpy,screen)
		//int X = (rcdesktop.right-rcdesktop.left)/2 - (rc.right-rc.left)/2;
		//int Y = (rcdesktop.bottom-rcdesktop.top)/2 - (rc.bottom-rc.top)/2;

		// create a window in window mode
		GLWin.attr.event_mask = ExposureMask | KeyPressMask | ButtonPressMask |
			StructureNotifyMask;
		GLWin.win = XCreateWindow(GLWin.dpy, RootWindow(GLWin.dpy, vi->screen),
								  0, 0, _width, _height, 0, vi->depth, InputOutput, vi->visual,
								  CWBorderPixel | CWColormap | CWEventMask, &GLWin.attr);
		// only set window title and handle wm_delete_events if in windowed mode
		wmDelete = XInternAtom(GLWin.dpy, "WM_DELETE_WINDOW", True);
		XSetWMProtocols(GLWin.dpy, GLWin.win, &wmDelete, 1);
		XSetStandardProperties(GLWin.dpy, GLWin.win, "ZeroGS",
								   "ZeroGS", None, NULL, 0, NULL);
		XMapRaised(GLWin.dpy, GLWin.win);
	}	   

	// connect the glx-context to the window
	glXMakeCurrent(GLWin.dpy, GLWin.win, GLWin.ctx);
	XGetGeometry(GLWin.dpy, GLWin.win, &winDummy, &GLWin.x, &GLWin.y,
				 &GLWin.width, &GLWin.height, &borderDummy, &GLWin.depth);
	ERROR_LOG("Depth %d\n", GLWin.depth);
	if (glXIsDirect(GLWin.dpy, GLWin.ctx)) 
		ERROR_LOG("you have Direct Rendering!\n");
	else
		ERROR_LOG("no Direct Rendering possible!\n");

	// better for pad plugin key input (thc)
	XSelectInput(GLWin.dpy, GLWin.win, ExposureMask | KeyPressMask | KeyReleaseMask | 
				 ButtonPressMask | StructureNotifyMask | EnterWindowMask | LeaveWindowMask |
				 FocusChangeMask );

#endif // WIN32
	s_nFullscreen = (conf.options & GSOPTION_FULLSCREEN) ? 1 : 0;
	conf.mrtdepth = 0; // for now
	
	return true;
}

// Function ask about differnt OGL extensions, that are required to setup accordingly. Return false if cheks failed
inline bool ZeroGS::CreateImportantCheck() {
	bool bSuccess = true;
#ifndef _WIN32
	int const glew_ok = glewInit();
	if( glew_ok != GLEW_OK ) {
		ERROR_LOG("glewInit() is not ok!\n");
		bSuccess = false;
	}
#endif

	if( !IsGLExt("GL_EXT_framebuffer_object") ) {
		ERROR_LOG("*********\nZZogl: ERROR: Need GL_EXT_framebufer_object for multiple render targets\nZZogl: *********\n");
		bSuccess = false;
	}

	if( !IsGLExt("GL_EXT_secondary_color") ) {
		ERROR_LOG("*********\nZZogl: OGL WARNING: Need GL_EXT_secondary_color\nZZogl: *********\n");
		bSuccess = false;
	}

	// load the effect, find the best profiles (if any)
	if( cgGLIsProfileSupported(CG_PROFILE_ARBVP1) != CG_TRUE ) {
		ERROR_LOG("arbvp1 not supported\n");
		bSuccess = false;
	}
	if( cgGLIsProfileSupported(CG_PROFILE_ARBFP1) != CG_TRUE ) {
		ERROR_LOG("arbfp1 not supported\n");
		bSuccess = false;
	}

	return bSuccess;
}
	
// This is check for less important open gl extensions.
inline void ZeroGS::CreateOtherCheck() {
	if( !IsGLExt("GL_EXT_blend_equation_separate") || glBlendEquationSeparateEXT == NULL ) {
		ERROR_LOG("*********\nZZogl: OGL WARNING: Need GL_EXT_blend_equation_separate\nZZogl: *********\n");
		zgsBlendEquationSeparateEXT = glBlendEquationSeparateDummy;
	}
	else 
		zgsBlendEquationSeparateEXT = glBlendEquationSeparateEXT;
	
	if( !IsGLExt("GL_EXT_blend_func_separate") || glBlendFuncSeparateEXT == NULL ) {
		ERROR_LOG("*********\nZZogl: OGL WARNING: Need GL_EXT_blend_func_separate\nZZogl: *********\n");
		zgsBlendFuncSeparateEXT = glBlendFuncSeparateDummy;
	}
	else 
		zgsBlendFuncSeparateEXT = glBlendFuncSeparateEXT;

	if( !IsGLExt("GL_ARB_draw_buffers") && !IsGLExt("GL_ATI_draw_buffers") ) {
		ERROR_LOG("*********\nZZogl: OGL WARNING: multiple render targets not supported, some effects might look bad\nZZogl: *********\n");
		conf.mrtdepth = 0;
	}

	if( IsGLExt("GL_ARB_draw_buffers") )
		glDrawBuffers = (PFNGLDRAWBUFFERSPROC)wglGetProcAddress("glDrawBuffers");
	else if( IsGLExt("GL_ATI_draw_buffers") )
		glDrawBuffers = (PFNGLDRAWBUFFERSPROC)wglGetProcAddress("glDrawBuffersATI");


	if  (!IsGLExt("GL_ARB_multitexture"))
		ERROR_LOG("No multitexturing\n");
	else
		ERROR_LOG("Using multitexturing\n");

	GLint Max_Texture_Size_NV = 0;
	GLint Max_Texture_Size_2d = 0;

	glGetIntegerv(GL_MAX_RECTANGLE_TEXTURE_SIZE_NV, &Max_Texture_Size_NV);
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &Max_Texture_Size_2d);
	ERROR_LOG("Maximun texture size is %d for Tex_2d and %d for Tex_NV\n", Max_Texture_Size_2d, Max_Texture_Size_NV);
	if (Max_Texture_Size_NV < 1024)
		ERROR_LOG("Could not properly made bitmasks, so some textures should be missed\n");

	/* Zeydlitz: we don't support 128-bit targets yet. they are slow and weirdo
	if( g_GameSettings & GAME_32BITTARGS ) {
		g_RenderFormatType = RFT_byte8;
		ERROR_LOG("Setting 32 bit render target\n");
	}
	else {
		if( !IsGLExt("GL_NV_float_buffer") && !IsGLExt("GL_ARB_color_buffer_float") && !IsGLExt("ATI_pixel_format_float") ) {
			ERROR_LOG("******\nZZogl: GS WARNING: Floating point render targets not supported, switching to 32bit\nZZogl: *********\n");
			g_RenderFormatType = RFT_byte8;
		}
	}*/
	g_RenderFormatType = RFT_byte8;

#ifdef _WIN32
	if( IsGLExt("WGL_EXT_swap_control") || IsGLExt("EXT_swap_control") )
		wglSwapIntervalEXT(0);
#else
	if( IsGLExt("GLX_SGI_swap_control") ) {
		_PFNSWAPINTERVAL swapinterval = (_PFNSWAPINTERVAL)wglGetProcAddress("glXSwapInterval");
		if( !swapinterval )
			swapinterval = (_PFNSWAPINTERVAL)wglGetProcAddress("glXSwapIntervalSGI");
		if( !swapinterval )
			swapinterval = (_PFNSWAPINTERVAL)wglGetProcAddress("glXSwapIntervalEXT");

		if( swapinterval )
			swapinterval(0);
		else
			ERROR_LOG("no support for SwapInterval (framerate clamped to monitor refresh rate)\n");
	}
#endif
}

// open shader file according to build target

inline bool ZeroGS::CreateOpenShadersFile() {
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
			ERROR_LOG("Cannot find ps2hw.dat in working directory. Exiting\n");
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
	getcwd(curwd, ARRAY_SIZE(curwd));

	strcpy(tempstr, "/plugins/");
	sprintf(EFFECT_NAME, "%sps2hw.fx", tempstr);
	FILE* f = fopen(EFFECT_NAME, "r");
	if( f == NULL ) {

		strcpy(tempstr, "../../plugins/zzogl-pg/opengl/");
		sprintf(EFFECT_NAME, "%sps2hw.fx", tempstr);
		f = fopen(EFFECT_NAME, "r");

		if( f == NULL ) {
			ERROR_LOG("Failed to find %s, try compiling a non-devbuild\n", EFFECT_NAME);
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

// Read all extensions name and fill mapGLExtensions
inline bool CreateFillExtensionsMap(){
	// fill the opengl extension map
	const char* ptoken = (const char*)glGetString( GL_EXTENSIONS );
	if( ptoken == NULL ) return false;

	int prevlog = conf.log;
	conf.log = 1;
	GS_LOG("Supported OpenGL Extensions:\n%s\n", ptoken);	 // write to the log file
	conf.log = prevlog;

	// insert all exts into mapGLExtensions

	const char* pend = NULL;

	while(ptoken != NULL ) {
		pend = strchr(ptoken, ' ');

		if( pend != NULL ) {
			mapGLExtensions[string(ptoken, pend-ptoken)];
		}
		else {
			mapGLExtensions[string(ptoken)];
			break;
		}

		ptoken = pend;
		while(*ptoken == ' ') ++ptoken;
	}
	return true;
}

bool ZeroGS::Create(int _width, int _height)
{
	GLenum err = GL_NO_ERROR;
	bool bSuccess = true;
	int i;

	Destroy(1);
	GSStateReset(); 

	cgSetErrorHandler(HandleCgError, NULL);
	g_RenderFormatType = RFT_float16;

	if (!Create_Window(_width, _height))
		return false;

	if (!CreateFillExtensionsMap())
		return false;

	if (!CreateImportantCheck())
		return false;
	ZeroGS::CreateOtherCheck();

	// check the max texture width and height
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &g_MaxTexWidth);
	g_MaxTexHeight = g_MaxTexWidth/4;
	GPU_TEXWIDTH = g_MaxTexWidth/8;
	g_fiGPU_TEXWIDTH = 1.0f / GPU_TEXWIDTH;

	if (!CreateOpenShadersFile())
		return false;

	GL_REPORT_ERROR();
	if( err != GL_NO_ERROR ) bSuccess = false;

	s_srcrgb = s_dstrgb = s_srcalpha = s_dstalpha = GL_ONE;

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

	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);
	
	GL_REPORT_ERROR();
	if( err != GL_NO_ERROR ) bSuccess = false;

	glGenFramebuffersEXT( 1, &s_uFramebuffer);
	if( s_uFramebuffer == 0 ) {
		ERROR_LOG("failed to create the renderbuffer\n");
	}

	assert( glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT) == GL_FRAMEBUFFER_COMPLETE_EXT );
	glBindFramebufferEXT( GL_FRAMEBUFFER_EXT, s_uFramebuffer );

	if( glDrawBuffers != NULL )
		glDrawBuffers(1, s_drawbuffers);
	GL_REPORT_ERROR();
	if( err != GL_NO_ERROR ) bSuccess = false;

	font_p = new RasterFont();

	GL_REPORT_ERROR();
	if( err != GL_NO_ERROR ) bSuccess = false;

	// init draw fns
	drawfn[0] = KickPoint;
	drawfn[1] = KickLine;
	drawfn[2] = KickLine;
	drawfn[3] = KickTriangle;
	drawfn[4] = KickTriangle;
	drawfn[5] = KickTriangleFan;
	drawfn[6] = KickSprite;
	drawfn[7] = KickDummy;

	SetAA(conf.aa);
	GSsetGameCRC(g_LastCRC, g_GameSettings);
	GL_STENCILFUNC(GL_ALWAYS, 0, 0);

	//g_GameSettings |= 0;//GAME_VSSHACK|GAME_FULL16BITRES|GAME_NODEPTHRESOLVE|GAME_FASTUPDATE;
	//s_bWriteDepth = TRUE;

	GL_BLEND_ALL(GL_ONE, GL_ONE, GL_ONE, GL_ONE);

	glViewport(0,0,nBackbufferWidth,nBackbufferHeight);					 // Reset The Current Viewport

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
	assert( hBitmapSrc != NULL );
	HGLOBAL hBitmapGlob = LoadResource(hInst, hBitmapSrc);
	assert( hBitmapGlob != NULL );
	PBITMAPINFO pinfo = (PBITMAPINFO)LockResource(hBitmapGlob);

	glTexImage2D(GL_TEXTURE_RECTANGLE_NV, 0, 4, pinfo->bmiHeader.biWidth, pinfo->bmiHeader.biHeight, 0, pinfo->bmiHeader.biBitCount==32?GL_RGBA:GL_RGB, GL_UNSIGNED_BYTE, (u8*)pinfo+pinfo->bmiHeader.biSize);
	nLogoWidth = pinfo->bmiHeader.biWidth;
	nLogoHeight = pinfo->bmiHeader.biHeight;
	glTexParameteri(GL_TEXTURE_RECTANGLE_NV, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_RECTANGLE_NV, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
#else
#endif

	GL_REPORT_ERROR();

	g_nCurVBOIndex = 0;
	g_vboBuffers.resize(VB_NUMBUFFERS);
	glGenBuffers((GLsizei)g_vboBuffers.size(), &g_vboBuffers[0]);
	for(i = 0; i < (int)g_vboBuffers.size(); ++i) {
		glBindBuffer(GL_ARRAY_BUFFER, g_vboBuffers[i]);
		glBufferData(GL_ARRAY_BUFFER, 0x100*sizeof(VertexGPU), NULL, GL_STREAM_DRAW);
	}

	GL_REPORT_ERROR();
	if( err != GL_NO_ERROR ) bSuccess = false;

	// create the blocks texture
	g_fBlockMult = 1;

	vector<char> vBlockData, vBilinearData;
	BLOCK::FillBlocks(vBlockData, vBilinearData, 1);

	glGenTextures(1, &ptexBlocks);
	glBindTexture(GL_TEXTURE_2D, ptexBlocks);

	g_internalFloatFmt = GL_ALPHA_FLOAT32_ATI;
	g_internalRGBAFloatFmt = GL_RGBA_FLOAT32_ATI;
	g_internalRGBAFloat16Fmt = GL_RGBA_FLOAT16_ATI;

	glTexImage2D(GL_TEXTURE_2D, 0, g_internalFloatFmt, BLOCK_TEXWIDTH, BLOCK_TEXHEIGHT, 0, GL_ALPHA, GL_FLOAT, &vBlockData[0]);

	if( glGetError() != GL_NO_ERROR ) {
		// try different internal format
		g_internalFloatFmt = GL_FLOAT_R32_NV;
		glTexImage2D(GL_TEXTURE_2D, 0, g_internalFloatFmt, BLOCK_TEXWIDTH, BLOCK_TEXHEIGHT, 0, GL_RED, GL_FLOAT, &vBlockData[0]);
	}

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	if( glGetError() != GL_NO_ERROR ) {

		// error, resort to 16bit
		g_fBlockMult = 65535.0f*(float)g_fiGPU_TEXWIDTH / 32.0f;

		BLOCK::FillBlocks(vBlockData, vBilinearData, 0);
		glTexImage2D(GL_TEXTURE_2D, 0, 2, BLOCK_TEXWIDTH, BLOCK_TEXHEIGHT, 0, GL_R, GL_UNSIGNED_SHORT, &vBlockData[0]);
		if( glGetError() != GL_NO_ERROR ) {
			ERROR_LOG("ZZogl ERROR: could not fill blocks\n");
			return false;
		}
		ERROR_LOG("Using non-bilinear fill\n");
	}
	else {
		// fill in the bilinear blocks
		glGenTextures(1, &ptexBilinearBlocks);
		glBindTexture(GL_TEXTURE_2D, ptexBilinearBlocks);
		glTexImage2D(GL_TEXTURE_2D, 0, g_internalRGBAFloatFmt, BLOCK_TEXWIDTH, BLOCK_TEXHEIGHT, 0, GL_RGBA, GL_FLOAT, &vBilinearData[0]);

		if( glGetError() != GL_NO_ERROR ) {
			g_internalRGBAFloatFmt = GL_FLOAT_RGBA32_NV;
			g_internalRGBAFloat16Fmt = GL_FLOAT_RGBA16_NV;
			glTexImage2D(GL_TEXTURE_2D, 0, g_internalRGBAFloatFmt, BLOCK_TEXWIDTH, BLOCK_TEXHEIGHT, 0, GL_RGBA, GL_FLOAT, &vBilinearData[0]);
			ERROR_LOG("ZZogl Fill bilinear blocks\n");		
			B_G(glGetError() == GL_NO_ERROR, return false);
		}
		else
			ERROR_LOG("Fill bilinear blocks failed!\n");

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	}

	float fpri = 1;
	glPrioritizeTextures(1, &ptexBlocks, &fpri);
	if( ptexBilinearBlocks != 0 )
		glPrioritizeTextures(1, &ptexBilinearBlocks, &fpri);

	GL_REPORT_ERROR();

	// fill a simple rect
	glGenBuffers(1, &vboRect);
	glBindBuffer(GL_ARRAY_BUFFER, vboRect);
	
	vector<VertexGPU> verts(4);
	VertexGPU* pvert = &verts[0];
	pvert->x = -0x7fff; pvert->y = 0x7fff; pvert->z = 0; pvert->s = 0; pvert->t = 0; pvert++;
	pvert->x = 0x7fff; pvert->y = 0x7fff; pvert->z = 0; pvert->s = 1; pvert->t = 0; pvert++;
	pvert->x = -0x7fff; pvert->y = -0x7fff; pvert->z = 0; pvert->s = 0; pvert->t = 1; pvert++;
	pvert->x = 0x7fff; pvert->y = -0x7fff; pvert->z = 0; pvert->s = 1; pvert->t = 1; pvert++;
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
	for(i = 0; i < 256*256; ++i) {
		u32 tempcol = RGBA16to32(i);
		// have to flip r and b
		conv16to32data[i] = (tempcol&0xff00ff00)|((tempcol&0xff)<<16)|((tempcol&0xff0000)>>16);
	}
	glTexImage2D(GL_TEXTURE_2D, 0, 4, 256, 256, 0, GL_RGBA, GL_UNSIGNED_BYTE, &conv16to32data[0]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

	GL_REPORT_ERROR();
	if( err != GL_NO_ERROR ) bSuccess = false;

	vector<u32> conv32to16data(32*32*32);

	glGenTextures(1, &ptexConv32to16);
	glBindTexture(GL_TEXTURE_3D, ptexConv32to16);
	u32* dst = &conv32to16data[0];
	for(i = 0; i < 32; ++i) {
		for(int j = 0; j < 32; ++j) {
			for(int k = 0; k < 32; ++k) {
				u32 col = (i<<10)|(j<<5)|k;
				*dst++ = ((col&0xff)<<16)|(col&0xff00);
			}
		}
	}
	glTexImage3D(GL_TEXTURE_3D, 0, 4, 32, 32, 32, 0, GL_RGBA, GL_UNSIGNED_BYTE, &conv32to16data[0]);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP);
	GL_REPORT_ERROR();
	if( err != GL_NO_ERROR ) bSuccess = false;

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

	ERROR_LOG("Creating effects\n");
	B_G(LoadEffects(), return false);

	g_bDisplayMsg = 0;


	// create a sample shader
	clampInfo temp;
	memset(&temp, 0, sizeof(temp));
	temp.wms = 3; temp.wmt = 3;

	g_nPixelShaderVer = 0;//SHADER_ACCURATE;
	// test
	bool bFailed;
	FRAGMENTSHADER* pfrag = LoadShadeEffect(0, 1, 1, 1, 1, temp, 0, &bFailed);
	if( bFailed || pfrag == NULL ) {
		g_nPixelShaderVer = SHADER_ACCURATE|SHADER_REDUCED;

		pfrag = LoadShadeEffect(0, 0, 1, 1, 0, temp, 0, &bFailed);
		if( pfrag != NULL )
			cgGLLoadProgram(pfrag->prog);
		if( bFailed || pfrag == NULL || cgGetError() != CG_NO_ERROR ) {
			g_nPixelShaderVer = SHADER_REDUCED;
			ERROR_LOG("Basic shader test failed\n");
		}
	}

	g_bDisplayMsg = 1;
	if( g_nPixelShaderVer & SHADER_REDUCED )
		conf.bilinear = 0;

	ERROR_LOG("Creating extra effects\n");
	B_G(LoadExtraEffects(), return false);

	ERROR_LOG("using %s shaders\n", g_pShaders[g_nPixelShaderVer]);

	GL_REPORT_ERROR();
	if( err != GL_NO_ERROR ) bSuccess = false;

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
	g_bSaveFlushedFrame = 1;

	g_vsprog = g_psprog = 0;

	if (glGetError() == GL_NO_ERROR)
		return bSuccess;
	else {
		ERROR_LOG("ZZogl ERROR: in final init");
		return false;
	}
}

void ZeroGS::Destroy(BOOL bD3D)
{
	if( s_aviinit ) {
		StopCapture();
		Stop_Avi();
		ERROR_LOG("zerogs.avi stopped");
		s_aviinit = 0;
	}

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

	if( g_vboBuffers.size() > 0 ) {
		glDeleteBuffers((GLsizei)g_vboBuffers.size(), &g_vboBuffers[0]);
		g_vboBuffers.clear();
	}
	g_nCurVBOIndex = 0;

	for(int i = 0; i < ARRAY_SIZE(pvs); ++i) {
		SAFE_RELEASE_PROG(pvs[i]);
	}
	for(int i = 0; i < ARRAY_SIZE(ppsRegular); ++i) {
		SAFE_RELEASE_PROG(ppsRegular[i].prog);
	}
	for(int i = 0; i < ARRAY_SIZE(ppsTexture); ++i) {
		SAFE_RELEASE_PROG(ppsTexture[i].prog);
	}

	SAFE_RELEASE_PROG(pvsBitBlt.prog);
	SAFE_RELEASE_PROG(ppsBitBlt[0].prog); SAFE_RELEASE_PROG(ppsBitBlt[1].prog);
	SAFE_RELEASE_PROG(ppsBitBltDepth.prog);
	SAFE_RELEASE_PROG(ppsCRTCTarg[0].prog); SAFE_RELEASE_PROG(ppsCRTCTarg[1].prog);
	SAFE_RELEASE_PROG(ppsCRTC[0].prog); SAFE_RELEASE_PROG(ppsCRTC[1].prog);
	SAFE_RELEASE_PROG(ppsCRTC24[0].prog); SAFE_RELEASE_PROG(ppsCRTC24[1].prog);
	SAFE_RELEASE_PROG(ppsOne.prog);

	SAFE_DELETE(font_p);

#ifdef _WIN32
	if (hRC)											// Do We Have A Rendering Context?
	{
		if (!wglMakeCurrent(NULL,NULL))				 // Are We Able To Release The DC And RC Contexts?
		{
			MessageBox(NULL,"Release Of DC And RC Failed.","SHUTDOWN ERROR",MB_OK | MB_ICONINFORMATION);
		}

		if (!wglDeleteContext(hRC))					 // Are We Able To Delete The RC?
		{
			MessageBox(NULL,"Release Rendering Context Failed.","SHUTDOWN ERROR",MB_OK | MB_ICONINFORMATION);
		}
		hRC=NULL;									   // Set RC To NULL
	}

	if (hDC && !ReleaseDC(GShwnd,hDC))				  // Are We Able To Release The DC
	{
		MessageBox(NULL,"Release Device Context Failed.","SHUTDOWN ERROR",MB_OK | MB_ICONINFORMATION);
		hDC=NULL;									   // Set DC To NULL
	}
#else // linux
	if (GLWin.ctx)
	{
		if (!glXMakeCurrent(GLWin.dpy, None, NULL))
		{
			ERROR_LOG("Could not release drawing context.\n");
		}
		glXDestroyContext(GLWin.dpy, GLWin.ctx);
		GLWin.ctx = NULL;
	}
	/* switch back to original desktop resolution if we were in fs */
	if( GLWin.dpy != NULL ) {
		if (GLWin.fs) {
				XF86VidModeSwitchToMode(GLWin.dpy, GLWin.screen, &GLWin.deskMode);
				XF86VidModeSetViewPort(GLWin.dpy, GLWin.screen, 0, 0);
		}
	}
#endif

	mapGLExtensions.clear();
}

