#include "stdafx.h"
#include "OpenGL.h"

void gl::init()
{
#ifdef _WIN32
#define OPENGL_PROC(p, n) OPENGL_PROC2(p, n, gl##n)
#define OPENGL_PROC2(p, n, tn) /*if(!gl##n)*/ if(!(gl##n = (p)wglGetProcAddress(#tn))) LOG_ERROR(RSX, "OpenGL: initialization of " #tn " failed.")
	#include "GLProcTable.h"
#undef OPENGL_PROC
#undef OPENGL_PROC2
#endif
#ifdef __unix__
	glewExperimental = true;
	glewInit();
#endif
}

#ifdef _WIN32

extern "C"
{
	// NVIDIA Optimus: Default dGPU instead of iGPU (Driver: 302+)
	__declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
	// AMD: Request dGPU High Performance (Driver: 13.35+)
	__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}

#define OPENGL_PROC(p, n) p gl##n = nullptr
#define OPENGL_PROC2(p, n, tn) OPENGL_PROC(p, n)
	#include "GLProcTable.h"
#undef OPENGL_PROC
#undef OPENGL_PROC2
#endif

OpenGL::OpenGL()
{
	Close();
	Init();
}

OpenGL::~OpenGL()
{
	Close();
}

void OpenGL::Init()
{
#ifdef _WIN32
#define OPENGL_PROC(p, n) OPENGL_PROC2(p, n, gl##n)
#define OPENGL_PROC2(p, n, tn) if(!(n = (p)wglGetProcAddress(#tn))) LOG_ERROR(RSX, "OpenGL: initialization of " #tn " failed.")
	#include "GLProcTable.h"
#undef OPENGL_PROC
#undef OPENGL_PROC2
#endif
}

void OpenGL::Close()
{
#ifdef _WIN32
#define OPENGL_PROC(p, n) n = nullptr
#define OPENGL_PROC2(p, n, tn) OPENGL_PROC(p, n)
	#include "GLProcTable.h"
#undef OPENGL_PROC
#undef OPENGL_PROC2
#endif
}
