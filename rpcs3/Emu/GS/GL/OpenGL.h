#pragma once
#ifndef _WIN32
#include <GL/glew.h>
#endif

#ifdef _WIN32
typedef BOOL (WINAPI* PFNWGLSWAPINTERVALEXTPROC) (int interval);

#define OPENGL_PROC(p, n) extern p gl##n
#define OPENGL_PROC2(p, n, tn) OPENGL_PROC(p, n)
	#include "GLProcTable.h"
#undef OPENGL_PROC
#undef OPENGL_PROC2

#elif __APPLE__
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>

#else
#include <GL/gl.h>
#include <GL/glx.h>
#include <GL/glxext.h>
#endif

void InitProcTable();

struct OpenGL
{
#define OPENGL_PROC2(p, n, tn) OPENGL_PROC(p, n)
#define OPENGL_PROC(p, n) p n
	#include "GLProcTable.h"
#undef OPENGL_PROC
#undef OPENGL_PROC2

	OpenGL();
	~OpenGL();

	void Init();
	void Close();
};
