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

#ifndef _SDL_config_h
#define _SDL_config_h

#include "SDL_platform.h"

/**
 *  \file SDL_config.h
 *
 *  SDL_config.h for any platform that doesn't build using the configure system.
 */

/* Add any platform that doesn't build using the configure system. */
#if defined(__WIN32__)
#include "SDL_config_windows.h"
#elif defined(__MACOSX__)
#include "SDL_config_macosx.h"
#elif defined(__IPHONEOS__)
#include "SDL_config_iphoneos.h"
#elif defined(__ANDROID__)
#include "SDL_config_android.h"
#elif defined(__NINTENDODS__)
#include "SDL_config_nintendods.h"
#elif defined(__LINUX__)
#include "SDL_config_linux.h"
#else
#include "SDL_config_minimal.h"
#endif /* platform config */

#endif /* _SDL_config_h */
