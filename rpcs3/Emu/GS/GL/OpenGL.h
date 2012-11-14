#pragma once
#include <GL/gl.h>
#include "GL/glext.h"

#define OPENGL_PROC(p, n) extern p n
#include "GLProcTable.tbl"
#undef OPENGL_PROC

void InitProcTable();