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

/**
 *  \file SDL_blendmode.h
 *  
 *  Header file declaring the SDL_BlendMode enumeration
 */

#ifndef _SDL_blendmode_h
#define _SDL_blendmode_h

#include "begin_code.h"
/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
/* *INDENT-OFF* */
extern "C" {
/* *INDENT-ON* */
#endif

/**
 *  \brief The blend mode used in SDL_RenderCopy() and drawing operations.
 */
typedef enum
{
    SDL_BLENDMODE_NONE = 0x00000000,     /**< No blending */
    SDL_BLENDMODE_BLEND = 0x00000001,    /**< dst = (src * A) + (dst * (1-A)) */
    SDL_BLENDMODE_ADD = 0x00000002,      /**< dst = (src * A) + dst */
    SDL_BLENDMODE_MOD = 0x00000004       /**< dst = src * dst */
} SDL_BlendMode;

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
/* *INDENT-OFF* */
}
/* *INDENT-ON* */
#endif
#include "close_code.h"

#endif /* _SDL_video_h */

/* vi: set ts=4 sw=4 expandtab: */
