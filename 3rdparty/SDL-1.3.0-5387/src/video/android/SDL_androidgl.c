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

/* Android SDL video driver implementation */

#include "SDL_video.h"

#include "SDL_androidvideo.h"
#include "../../core/android/SDL_android.h"

#include <android/log.h>


/* GL functions */
int
Android_GL_LoadLibrary(_THIS, const char *path)
{
    __android_log_print(ANDROID_LOG_INFO, "SDL", "[STUB] GL_LoadLibrary\n");
    return 0;
}

void *
Android_GL_GetProcAddress(_THIS, const char *proc)
{
    __android_log_print(ANDROID_LOG_INFO, "SDL", "[STUB] GL_GetProcAddress\n");
    return 0;
}

void
Android_GL_UnloadLibrary(_THIS)
{
    __android_log_print(ANDROID_LOG_INFO, "SDL", "[STUB] GL_UnloadLibrary\n");
}

SDL_GLContext
Android_GL_CreateContext(_THIS, SDL_Window * window)
{
    if (!Android_JNI_CreateContext(_this->gl_config.major_version,
                                   _this->gl_config.minor_version)) {
        SDL_SetError("Couldn't create OpenGL context - see Android log for details");
        return NULL;
    }
    return (SDL_GLContext)1;
}

int
Android_GL_MakeCurrent(_THIS, SDL_Window * window, SDL_GLContext context)
{
    /* There's only one context, nothing to do... */
    return 0;
}

int
Android_GL_SetSwapInterval(_THIS, int interval)
{
    __android_log_print(ANDROID_LOG_INFO, "SDL", "[STUB] GL_SetSwapInterval\n");
    return 0;
}

int
Android_GL_GetSwapInterval(_THIS)
{
    __android_log_print(ANDROID_LOG_INFO, "SDL", "[STUB] GL_GetSwapInterval\n");
    return 0;
}

void
Android_GL_SwapWindow(_THIS, SDL_Window * window)
{
    Android_JNI_SwapWindow();
}

void
Android_GL_DeleteContext(_THIS, SDL_GLContext context)
{
    __android_log_print(ANDROID_LOG_INFO, "SDL", "[STUB] GL_DeleteContext\n");
}

/* vi: set ts=4 sw=4 expandtab: */
