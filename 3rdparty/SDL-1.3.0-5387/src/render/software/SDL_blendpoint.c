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

#if !SDL_RENDER_DISABLED

#include "SDL_draw.h"
#include "SDL_blendpoint.h"


static int
SDL_BlendPoint_RGB555(SDL_Surface * dst, int x, int y, SDL_BlendMode blendMode, Uint8 r,
                      Uint8 g, Uint8 b, Uint8 a)
{
    unsigned inva = 0xff - a;

    switch (blendMode) {
    case SDL_BLENDMODE_BLEND:
        DRAW_SETPIXELXY_BLEND_RGB555(x, y);
        break;
    case SDL_BLENDMODE_ADD:
        DRAW_SETPIXELXY_ADD_RGB555(x, y);
        break;
    case SDL_BLENDMODE_MOD:
        DRAW_SETPIXELXY_MOD_RGB555(x, y);
        break;
    default:
        DRAW_SETPIXELXY_RGB555(x, y);
        break;
    }
    return 0;
}

static int
SDL_BlendPoint_RGB565(SDL_Surface * dst, int x, int y, SDL_BlendMode blendMode, Uint8 r,
                      Uint8 g, Uint8 b, Uint8 a)
{
    unsigned inva = 0xff - a;

    switch (blendMode) {
    case SDL_BLENDMODE_BLEND:
        DRAW_SETPIXELXY_BLEND_RGB565(x, y);
        break;
    case SDL_BLENDMODE_ADD:
        DRAW_SETPIXELXY_ADD_RGB565(x, y);
        break;
    case SDL_BLENDMODE_MOD:
        DRAW_SETPIXELXY_MOD_RGB565(x, y);
        break;
    default:
        DRAW_SETPIXELXY_RGB565(x, y);
        break;
    }
    return 0;
}

static int
SDL_BlendPoint_RGB888(SDL_Surface * dst, int x, int y, SDL_BlendMode blendMode, Uint8 r,
                      Uint8 g, Uint8 b, Uint8 a)
{
    unsigned inva = 0xff - a;

    switch (blendMode) {
    case SDL_BLENDMODE_BLEND:
        DRAW_SETPIXELXY_BLEND_RGB888(x, y);
        break;
    case SDL_BLENDMODE_ADD:
        DRAW_SETPIXELXY_ADD_RGB888(x, y);
        break;
    case SDL_BLENDMODE_MOD:
        DRAW_SETPIXELXY_MOD_RGB888(x, y);
        break;
    default:
        DRAW_SETPIXELXY_RGB888(x, y);
        break;
    }
    return 0;
}

static int
SDL_BlendPoint_ARGB8888(SDL_Surface * dst, int x, int y, SDL_BlendMode blendMode,
                        Uint8 r, Uint8 g, Uint8 b, Uint8 a)
{
    unsigned inva = 0xff - a;

    switch (blendMode) {
    case SDL_BLENDMODE_BLEND:
        DRAW_SETPIXELXY_BLEND_ARGB8888(x, y);
        break;
    case SDL_BLENDMODE_ADD:
        DRAW_SETPIXELXY_ADD_ARGB8888(x, y);
        break;
    case SDL_BLENDMODE_MOD:
        DRAW_SETPIXELXY_MOD_ARGB8888(x, y);
        break;
    default:
        DRAW_SETPIXELXY_ARGB8888(x, y);
        break;
    }
    return 0;
}

static int
SDL_BlendPoint_RGB(SDL_Surface * dst, int x, int y, SDL_BlendMode blendMode, Uint8 r,
                   Uint8 g, Uint8 b, Uint8 a)
{
    SDL_PixelFormat *fmt = dst->format;
    unsigned inva = 0xff - a;

    switch (fmt->BytesPerPixel) {
    case 2:
        switch (blendMode) {
        case SDL_BLENDMODE_BLEND:
            DRAW_SETPIXELXY2_BLEND_RGB(x, y);
            break;
        case SDL_BLENDMODE_ADD:
            DRAW_SETPIXELXY2_ADD_RGB(x, y);
            break;
        case SDL_BLENDMODE_MOD:
            DRAW_SETPIXELXY2_MOD_RGB(x, y);
            break;
        default:
            DRAW_SETPIXELXY2_RGB(x, y);
            break;
        }
        return 0;
    case 4:
        switch (blendMode) {
        case SDL_BLENDMODE_BLEND:
            DRAW_SETPIXELXY4_BLEND_RGB(x, y);
            break;
        case SDL_BLENDMODE_ADD:
            DRAW_SETPIXELXY4_ADD_RGB(x, y);
            break;
        case SDL_BLENDMODE_MOD:
            DRAW_SETPIXELXY4_MOD_RGB(x, y);
            break;
        default:
            DRAW_SETPIXELXY4_RGB(x, y);
            break;
        }
        return 0;
    default:
        SDL_Unsupported();
        return -1;
    }
}

static int
SDL_BlendPoint_RGBA(SDL_Surface * dst, int x, int y, SDL_BlendMode blendMode, Uint8 r,
                    Uint8 g, Uint8 b, Uint8 a)
{
    SDL_PixelFormat *fmt = dst->format;
    unsigned inva = 0xff - a;

    switch (fmt->BytesPerPixel) {
    case 4:
        switch (blendMode) {
        case SDL_BLENDMODE_BLEND:
            DRAW_SETPIXELXY4_BLEND_RGBA(x, y);
            break;
        case SDL_BLENDMODE_ADD:
            DRAW_SETPIXELXY4_ADD_RGBA(x, y);
            break;
        case SDL_BLENDMODE_MOD:
            DRAW_SETPIXELXY4_MOD_RGBA(x, y);
            break;
        default:
            DRAW_SETPIXELXY4_RGBA(x, y);
            break;
        }
        return 0;
    default:
        SDL_Unsupported();
        return -1;
    }
}

int
SDL_BlendPoint(SDL_Surface * dst, int x, int y, SDL_BlendMode blendMode, Uint8 r,
               Uint8 g, Uint8 b, Uint8 a)
{
    if (!dst) {
        SDL_SetError("Passed NULL destination surface");
        return -1;
    }

    /* This function doesn't work on surfaces < 8 bpp */
    if (dst->format->BitsPerPixel < 8) {
        SDL_SetError("SDL_BlendPoint(): Unsupported surface format");
        return -1;
    }

    /* Perform clipping */
    if (x < dst->clip_rect.x || y < dst->clip_rect.y ||
        x >= (dst->clip_rect.x + dst->clip_rect.w) ||
        y >= (dst->clip_rect.y + dst->clip_rect.h)) {
        return 0;
    }

    if (blendMode == SDL_BLENDMODE_BLEND || blendMode == SDL_BLENDMODE_ADD) {
        r = DRAW_MUL(r, a);
        g = DRAW_MUL(g, a);
        b = DRAW_MUL(b, a);
    }

    switch (dst->format->BitsPerPixel) {
    case 15:
        switch (dst->format->Rmask) {
        case 0x7C00:
            return SDL_BlendPoint_RGB555(dst, x, y, blendMode, r, g, b, a);
        }
        break;
    case 16:
        switch (dst->format->Rmask) {
        case 0xF800:
            return SDL_BlendPoint_RGB565(dst, x, y, blendMode, r, g, b, a);
        }
        break;
    case 32:
        switch (dst->format->Rmask) {
        case 0x00FF0000:
            if (!dst->format->Amask) {
                return SDL_BlendPoint_RGB888(dst, x, y, blendMode, r, g, b,
                                             a);
            } else {
                return SDL_BlendPoint_ARGB8888(dst, x, y, blendMode, r, g, b,
                                               a);
            }
            break;
        }
        break;
    default:
        break;
    }

    if (!dst->format->Amask) {
        return SDL_BlendPoint_RGB(dst, x, y, blendMode, r, g, b, a);
    } else {
        return SDL_BlendPoint_RGBA(dst, x, y, blendMode, r, g, b, a);
    }
}

int
SDL_BlendPoints(SDL_Surface * dst, const SDL_Point * points, int count,
                SDL_BlendMode blendMode, Uint8 r, Uint8 g, Uint8 b, Uint8 a)
{
    int minx, miny;
    int maxx, maxy;
    int i;
    int x, y;
    int (*func)(SDL_Surface * dst, int x, int y,
                SDL_BlendMode blendMode, Uint8 r, Uint8 g, Uint8 b, Uint8 a) = NULL;
    int status = 0;

    if (!dst) {
        SDL_SetError("Passed NULL destination surface");
        return -1;
    }

    /* This function doesn't work on surfaces < 8 bpp */
    if (dst->format->BitsPerPixel < 8) {
        SDL_SetError("SDL_BlendPoints(): Unsupported surface format");
        return (-1);
    }

    if (blendMode == SDL_BLENDMODE_BLEND || blendMode == SDL_BLENDMODE_ADD) {
        r = DRAW_MUL(r, a);
        g = DRAW_MUL(g, a);
        b = DRAW_MUL(b, a);
    }

    /* FIXME: Does this function pointer slow things down significantly? */
    switch (dst->format->BitsPerPixel) {
    case 15:
        switch (dst->format->Rmask) {
        case 0x7C00:
            func = SDL_BlendPoint_RGB555;
            break;
        }
        break;
    case 16:
        switch (dst->format->Rmask) {
        case 0xF800:
            func = SDL_BlendPoint_RGB565;
            break;
        }
        break;
    case 32:
        switch (dst->format->Rmask) {
        case 0x00FF0000:
            if (!dst->format->Amask) {
                func = SDL_BlendPoint_RGB888;
            } else {
                func = SDL_BlendPoint_ARGB8888;
            }
            break;
        }
        break;
    default:
        break;
    }

    if (!func) {
        if (!dst->format->Amask) {
            func = SDL_BlendPoint_RGB;
        } else {
            func = SDL_BlendPoint_RGBA;
        }
    }

    minx = dst->clip_rect.x;
    maxx = dst->clip_rect.x + dst->clip_rect.w - 1;
    miny = dst->clip_rect.y;
    maxy = dst->clip_rect.y + dst->clip_rect.h - 1;

    for (i = 0; i < count; ++i) {
        x = points[i].x;
        y = points[i].y;

        if (x < minx || x > maxx || y < miny || y > maxy) {
            continue;
        }
        status = func(dst, x, y, blendMode, r, g, b, a);
    }
    return status;
}

#endif /* !SDL_RENDER_DISABLED */

/* vi: set ts=4 sw=4 expandtab: */
