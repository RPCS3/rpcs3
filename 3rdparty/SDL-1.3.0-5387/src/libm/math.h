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
#include "SDL_stdinc.h"

/* Math routines from uClibc: http://www.uclibc.org */

#ifdef HAVE_ATAN
#define atan            SDL_uclibc_atan
#else
#define atan            SDL_atan
#endif

#ifndef HAVE_ATAN2
#define __ieee754_atan2 SDL_atan2
#endif

#ifdef HAVE_COPYSIGN
#define copysign        SDL_uclibc_copysign
#else
#define copysign        SDL_copysign
#endif

#ifdef HAVE_COS
#define cos             SDL_uclibc_cos
#else
#define cos             SDL_cos
#endif

#ifdef HAVE_FABS
#define fabs            SDL_uclibc_fabs
#else
#define fabs            SDL_fabs
#endif

#ifdef HAVE_FLOOR
#define floor           SDL_uclibc_floor
#else
#define floor           SDL_floor
#endif

#ifndef HAVE_LOG
#define __ieee754_log   SDL_log
#endif

#ifndef HAVE_POW
#define __ieee754_pow   SDL_pow
#endif

#ifdef HAVE_SCALBN
#define scalbn          SDL_uclibc_scalbn
#else
#define scalbn          SDL_scalbn
#endif

#ifdef HAVE_SIN
#define sin             SDL_uclibc_sin
#else
#define sin             SDL_sin
#endif

#ifndef HAVE_SQRT
#define __ieee754_sqrt  SDL_sqrt
#endif

/* vi: set ts=4 sw=4 expandtab: */
