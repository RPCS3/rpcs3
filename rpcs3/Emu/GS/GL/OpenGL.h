#pragma once
#include <GL/gl.h>
#include "GL/glext.h"

#ifdef __GNUG__ // HACK: detect xorg
#define GLX_GLXEXT_PROTOTYPES
//#include <X11/Xlib.h>
//#include <GL/glxext.h>
#else
typedef BOOL (WINAPI* PFNWGLSWAPINTERVALEXTPROC) (int interval);
#endif

#define OPENGL_PROC(p, n) extern p gl##n
#define OPENGL_PROC2(p, n, tn) OPENGL_PROC(p, n)
	#include "GLProcTable.tbl"
#undef OPENGL_PROC
#undef OPENGL_PROC2

void InitProcTable();

struct OpenGL
{
#define OPENGL_PROC2(p, n, tn) OPENGL_PROC(p, n)
#define OPENGL_PROC(p, n) p n
	#include "GLProcTable.tbl"
#undef OPENGL_PROC
#undef OPENGL_PROC2

	OpenGL();
	~OpenGL();

	void Init();
	void Close();
};
