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

#ifndef _SDL_config_android_h
#define _SDL_config_android_h

#include "SDL_platform.h"

/**
 *  \file SDL_config_android.h
 *  
 *  This is a configuration that can be used to build SDL for Android
 */

#include <stdarg.h>

/*
typedef signed char int8_t;
typedef unsigned char uint8_t;
typedef signed short int16_t;
typedef unsigned short uint16_t;
typedef signed int int32_t;
typedef unsigned int uint32_t;
*/


#define HAVE_ALLOCA_H		1
#define HAVE_SYS_TYPES_H	1
#define HAVE_STDIO_H	1
#define STDC_HEADERS	1
#define HAVE_STRING_H	1
#define HAVE_INTTYPES_H	1
#define HAVE_STDINT_H	1
#define HAVE_CTYPE_H	1
#define HAVE_MATH_H	1
#define HAVE_SIGNAL_H	1

/* C library functions */
#define HAVE_MALLOC	1
#define HAVE_CALLOC	1
#define HAVE_REALLOC	1
#define HAVE_FREE	1
#define HAVE_ALLOCA	1
#define HAVE_GETENV	1
#define HAVE_SETENV	1
#define HAVE_PUTENV	1
#define HAVE_SETENV	1
#define HAVE_UNSETENV	1
#define HAVE_QSORT	1
#define HAVE_ABS	1
#define HAVE_BCOPY	1
#define HAVE_MEMSET	1
#define HAVE_MEMCPY	1
#define HAVE_MEMMOVE	1
#define HAVE_MEMCMP	1
#define HAVE_STRLEN	1
#define HAVE_STRLCPY	1
#define HAVE_STRLCAT	1
#define HAVE_STRDUP	1
#define HAVE_STRCHR	1
#define HAVE_STRRCHR	1
#define HAVE_STRSTR	1
#define HAVE_STRTOL	1
#define HAVE_STRTOUL	1
#define HAVE_STRTOLL	1
#define HAVE_STRTOULL	1
#define HAVE_STRTOD	1
#define HAVE_ATOI	1
#define HAVE_ATOF	1
#define HAVE_STRCMP	1
#define HAVE_STRNCMP	1
#define HAVE_STRCASECMP	1
#define HAVE_STRNCASECMP 1
#define HAVE_SSCANF	1
#define HAVE_SNPRINTF	1
#define HAVE_VSNPRINTF	1
#define HAVE_M_PI	1
#define HAVE_ATAN	1
#define HAVE_ATAN2	1
#define HAVE_CEIL	1
#define HAVE_COPYSIGN	1
#define HAVE_COS	1
#define HAVE_COSF	1
#define HAVE_FABS	1
#define HAVE_FLOOR	1
#define HAVE_LOG	1
#define HAVE_POW	1
#define HAVE_SCALBN	1
#define HAVE_SIN	1
#define HAVE_SINF	1
#define HAVE_SQRT	1
#define HAVE_SIGACTION	1
#define HAVE_SETJMP	1
#define HAVE_NANOSLEEP	1
#define HAVE_SYSCONF	1

#define SIZEOF_VOIDP 4

typedef unsigned int size_t;
//typedef unsigned long uintptr_t;

/* Enable various audio drivers */
#define SDL_AUDIO_DRIVER_ANDROID	1
#define SDL_AUDIO_DRIVER_DUMMY	1

/* Enable various input drivers */
#define SDL_JOYSTICK_ANDROID	1
#define SDL_HAPTIC_DUMMY	1

/* Enable various shared object loading systems */
#define SDL_LOADSO_DLOPEN	1

/* Enable various threading systems */
#define SDL_THREAD_PTHREAD	1
#define SDL_THREAD_PTHREAD_RECURSIVE_MUTEX	1

/* Enable various timer systems */
#define SDL_TIMER_UNIX	1

/* Enable various video drivers */
#define SDL_VIDEO_DRIVER_ANDROID 1

/* Enable OpenGL ES */
#define SDL_VIDEO_OPENGL_ES	1
#define SDL_VIDEO_RENDER_OGL_ES	1
#define SDL_VIDEO_RENDER_OGL_ES2	1

#endif /* _SDL_config_minimal_h */
