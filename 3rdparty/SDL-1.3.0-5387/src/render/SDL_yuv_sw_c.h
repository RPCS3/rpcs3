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

#include "SDL_video.h"

/* This is the software implementation of the YUV texture support */

struct SDL_SW_YUVTexture
{
    Uint32 format;
    Uint32 target_format;
    int w, h;
    Uint8 *pixels;
    int *colortab;
    Uint32 *rgb_2_pix;
    void (*Display1X) (int *colortab, Uint32 * rgb_2_pix,
                       unsigned char *lum, unsigned char *cr,
                       unsigned char *cb, unsigned char *out,
                       int rows, int cols, int mod);
    void (*Display2X) (int *colortab, Uint32 * rgb_2_pix,
                       unsigned char *lum, unsigned char *cr,
                       unsigned char *cb, unsigned char *out,
                       int rows, int cols, int mod);

    /* These are just so we don't have to allocate them separately */
    Uint16 pitches[3];
    Uint8 *planes[3];

    /* This is a temporary surface in case we have to stretch copy */
    SDL_Surface *stretch;
    SDL_Surface *display;
};

typedef struct SDL_SW_YUVTexture SDL_SW_YUVTexture;

SDL_SW_YUVTexture *SDL_SW_CreateYUVTexture(Uint32 format, int w, int h);
int SDL_SW_QueryYUVTexturePixels(SDL_SW_YUVTexture * swdata, void **pixels,
                                 int *pitch);
int SDL_SW_UpdateYUVTexture(SDL_SW_YUVTexture * swdata, const SDL_Rect * rect,
                            const void *pixels, int pitch);
int SDL_SW_LockYUVTexture(SDL_SW_YUVTexture * swdata, const SDL_Rect * rect,
                          void **pixels, int *pitch);
void SDL_SW_UnlockYUVTexture(SDL_SW_YUVTexture * swdata);
int SDL_SW_CopyYUVToRGB(SDL_SW_YUVTexture * swdata, const SDL_Rect * srcrect,
                        Uint32 target_format, int w, int h, void *pixels,
                        int pitch);
void SDL_SW_DestroyYUVTexture(SDL_SW_YUVTexture * swdata);

/* vi: set ts=4 sw=4 expandtab: */
