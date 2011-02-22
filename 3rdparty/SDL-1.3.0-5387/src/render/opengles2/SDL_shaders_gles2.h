/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997-2011 Sam Lantinga
    Copyright (C) 2010 itsnotabigtruck.

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

#if SDL_VIDEO_RENDER_OGL_ES2

#ifndef SDL_shaderdata_h_
#define SDL_shaderdata_h_

typedef struct GLES2_ShaderInstance
{
    GLenum type;
    GLenum format;
    int length;
    const void *data;
} GLES2_ShaderInstance;

typedef struct GLES2_Shader
{
    int instance_count;
    const GLES2_ShaderInstance *instances[4];
} GLES2_Shader;

typedef enum
{
    GLES2_SHADER_VERTEX_DEFAULT,
    GLES2_SHADER_FRAGMENT_SOLID_SRC,
    GLES2_SHADER_FRAGMENT_TEXTURE_SRC
} GLES2_ShaderType;

#define GLES2_SOURCE_SHADER (GLenum)-1

const GLES2_Shader *GLES2_GetShader(GLES2_ShaderType type, SDL_BlendMode blendMode);

#endif /* SDL_shaderdata_h_ */

#endif /* SDL_VIDEO_RENDER_OGL_ES2 */

/* vi: set ts=4 sw=4 expandtab: */
