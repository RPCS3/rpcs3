/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997-2011 Sam Lantinga

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

    Sam Lantinga
    slouken@libsdl.org
*/
#include "SDL_config.h"

/* OpenGL shader implementation */

typedef enum {
    SHADER_NONE,
    SHADER_SOLID,
    SHADER_RGB,
    SHADER_YV12,
    NUM_SHADERS
} GL_Shader;

typedef struct GL_ShaderContext GL_ShaderContext;

extern GL_ShaderContext * GL_CreateShaderContext();
extern void GL_SelectShader(GL_ShaderContext *ctx, GL_Shader shader);
extern void GL_DestroyShaderContext(GL_ShaderContext *ctx);

/* vi: set ts=4 sw=4 expandtab: */
