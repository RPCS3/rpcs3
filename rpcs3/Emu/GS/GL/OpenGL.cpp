#include "stdafx.h"
#include "OpenGL.h"

void InitProcTable()
{
#if 0
#define OPENGL_PROC(p, n) OPENGL_PROC2(p, n, gl##n)
#define OPENGL_PROC2(p, n, tn) /*if(!gl##n)*/ if(!(gl##n = (p)wglGetProcAddress(#tn))) ConLog.Error("OpenGL: initialization of " #tn " failed.")
	#include "GLProcTable.tbl"
#undef OPENGL_PROC
#undef OPENGL_PROC2
#endif
}

#define OPENGL_PROC(p, n) p gl##n = nullptr
#define OPENGL_PROC2(p, n, tn) OPENGL_PROC(p, n)
	#include "GLProcTable.tbl"
#undef OPENGL_PROC
#undef OPENGL_PROC2

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
#if 0
#define OPENGL_PROC(p, n) OPENGL_PROC2(p, n, gl##n)
#define OPENGL_PROC2(p, n, tn) if(!(n = (p)wglGetProcAddress(#tn))) ConLog.Error("OpenGL: initialization of " #tn " failed.")
	#include "GLProcTable.tbl"
#undef OPENGL_PROC
#undef OPENGL_PROC2
#endif
}

void OpenGL::Close()
{
#define OPENGL_PROC(p, n) n = nullptr
#define OPENGL_PROC2(p, n, tn) OPENGL_PROC(p, n)
	#include "GLProcTable.tbl"
#undef OPENGL_PROC
#undef OPENGL_PROC2
}
