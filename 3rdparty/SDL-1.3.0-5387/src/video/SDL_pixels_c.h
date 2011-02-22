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

/* Useful functions and variables from SDL_pixel.c */

#include "SDL_blit.h"

/* Pixel format functions */
extern int SDL_InitFormat(SDL_PixelFormat * format, Uint32 pixel_format);

/* Blit mapping functions */
extern SDL_BlitMap *SDL_AllocBlitMap(void);
extern void SDL_InvalidateMap(SDL_BlitMap * map);
extern int SDL_MapSurface(SDL_Surface * src, SDL_Surface * dst);
extern void SDL_FreeBlitMap(SDL_BlitMap * map);

/* Miscellaneous functions */
extern int SDL_CalculatePitch(SDL_Surface * surface);
extern void SDL_DitherColors(SDL_Color * colors, int bpp);
extern Uint8 SDL_FindColor(SDL_Palette * pal, Uint8 r, Uint8 g, Uint8 b);

/* vi: set ts=4 sw=4 expandtab: */
