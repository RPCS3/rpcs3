#include "stdafx.h"
#include "OpenGL.h"

void InitProcTable()
{
	#define OPENGL_PROC(p, n) n = (p)wglGetProcAddress(#n)
	#include "GLProcTable.tbl"
	#undef OPENGL_PROC
}

#define OPENGL_PROC(p, n) p n = NULL
#include "GLProcTable.tbl"
#undef OPENGL_PROC