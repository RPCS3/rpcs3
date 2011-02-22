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

/* Useful functions and variables from SDL_RLEaccel.c */

extern int SDL_RLESurface(SDL_Surface * surface);
extern int SDL_RLEBlit(SDL_Surface * src, SDL_Rect * srcrect,
                       SDL_Surface * dst, SDL_Rect * dstrect);
extern int SDL_RLEAlphaBlit(SDL_Surface * src, SDL_Rect * srcrect,
                            SDL_Surface * dst, SDL_Rect * dstrect);
extern void SDL_UnRLESurface(SDL_Surface * surface, int recode);
/* vi: set ts=4 sw=4 expandtab: */
