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
#include "SDL_blendline.h"
#include "SDL_blendpoint.h"


static void
SDL_BlendLine_RGB2(SDL_Surface * dst, int x1, int y1, int x2, int y2,
                   SDL_BlendMode blendMode, Uint8 _r, Uint8 _g, Uint8 _b, Uint8 _a,
                   SDL_bool draw_end)
{
    const SDL_PixelFormat *fmt = dst->format;
    unsigned r, g, b, a, inva;

    if (blendMode == SDL_BLENDMODE_BLEND || blendMode == SDL_BLENDMODE_ADD) {
        r = DRAW_MUL(_r, _a);
        g = DRAW_MUL(_g, _a);
        b = DRAW_MUL(_b, _a);
        a = _a;
    } else {
        r = _r;
        g = _g;
        b = _b;
        a = _a;
    }
    inva = (a ^ 0xff);

    if (y1 == y2) {
        switch (blendMode) {
        case SDL_BLENDMODE_BLEND:
            HLINE(Uint16, DRAW_SETPIXEL_BLEND_RGB, draw_end);
            break;
        case SDL_BLENDMODE_ADD:
            HLINE(Uint16, DRAW_SETPIXEL_ADD_RGB, draw_end);
            break;
        case SDL_BLENDMODE_MOD:
            HLINE(Uint16, DRAW_SETPIXEL_MOD_RGB, draw_end);
            break;
        default:
            HLINE(Uint16, DRAW_SETPIXEL_RGB, draw_end);
            break;
        }
    } else if (x1 == x2) {
        switch (blendMode) {
        case SDL_BLENDMODE_BLEND:
            VLINE(Uint16, DRAW_SETPIXEL_BLEND_RGB, draw_end);
            break;
        case SDL_BLENDMODE_ADD:
            VLINE(Uint16, DRAW_SETPIXEL_ADD_RGB, draw_end);
            break;
        case SDL_BLENDMODE_MOD:
            VLINE(Uint16, DRAW_SETPIXEL_MOD_RGB, draw_end);
            break;
        default:
            VLINE(Uint16, DRAW_SETPIXEL_RGB, draw_end);
            break;
        }
    } else if (ABS(x1 - x2) == ABS(y1 - y2)) {
        switch (blendMode) {
        case SDL_BLENDMODE_BLEND:
            DLINE(Uint16, DRAW_SETPIXEL_BLEND_RGB, draw_end);
            break;
        case SDL_BLENDMODE_ADD:
            DLINE(Uint16, DRAW_SETPIXEL_ADD_RGB, draw_end);
            break;
        case SDL_BLENDMODE_MOD:
            DLINE(Uint16, DRAW_SETPIXEL_MOD_RGB, draw_end);
            break;
        default:
            DLINE(Uint16, DRAW_SETPIXEL_RGB, draw_end);
            break;
        }
    } else {
        switch (blendMode) {
        case SDL_BLENDMODE_BLEND:
            AALINE(x1, y1, x2, y2,
                   DRAW_SETPIXELXY2_BLEND_RGB, DRAW_SETPIXELXY2_BLEND_RGB,
                   draw_end);
            break;
        case SDL_BLENDMODE_ADD:
            AALINE(x1, y1, x2, y2,
                   DRAW_SETPIXELXY2_ADD_RGB, DRAW_SETPIXELXY2_ADD_RGB,
                   draw_end);
            break;
        case SDL_BLENDMODE_MOD:
            AALINE(x1, y1, x2, y2,
                   DRAW_SETPIXELXY2_MOD_RGB, DRAW_SETPIXELXY2_MOD_RGB,
                   draw_end);
            break;
        default:
            AALINE(x1, y1, x2, y2,
                   DRAW_SETPIXELXY2_RGB, DRAW_SETPIXELXY2_BLEND_RGB,
                   draw_end);
            break;
        }
    }
}

static void
SDL_BlendLine_RGB555(SDL_Surface * dst, int x1, int y1, int x2, int y2,
                     SDL_BlendMode blendMode, Uint8 _r, Uint8 _g, Uint8 _b, Uint8 _a,
                     SDL_bool draw_end)
{
    const SDL_PixelFormat *fmt = dst->format;
    unsigned r, g, b, a, inva;

    if (blendMode == SDL_BLENDMODE_BLEND || blendMode == SDL_BLENDMODE_ADD) {
        r = DRAW_MUL(_r, _a);
        g = DRAW_MUL(_g, _a);
        b = DRAW_MUL(_b, _a);
        a = _a;
    } else {
        r = _r;
        g = _g;
        b = _b;
        a = _a;
    }
    inva = (a ^ 0xff);

    if (y1 == y2) {
        switch (blendMode) {
        case SDL_BLENDMODE_BLEND:
            HLINE(Uint16, DRAW_SETPIXEL_BLEND_RGB555, draw_end);
            break;
        case SDL_BLENDMODE_ADD:
            HLINE(Uint16, DRAW_SETPIXEL_ADD_RGB555, draw_end);
            break;
        case SDL_BLENDMODE_MOD:
            HLINE(Uint16, DRAW_SETPIXEL_MOD_RGB555, draw_end);
            break;
        default:
            HLINE(Uint16, DRAW_SETPIXEL_RGB555, draw_end);
            break;
        }
    } else if (x1 == x2) {
        switch (blendMode) {
        case SDL_BLENDMODE_BLEND:
            VLINE(Uint16, DRAW_SETPIXEL_BLEND_RGB555, draw_end);
            break;
        case SDL_BLENDMODE_ADD:
            VLINE(Uint16, DRAW_SETPIXEL_ADD_RGB555, draw_end);
            break;
        case SDL_BLENDMODE_MOD:
            VLINE(Uint16, DRAW_SETPIXEL_MOD_RGB555, draw_end);
            break;
        default:
            VLINE(Uint16, DRAW_SETPIXEL_RGB555, draw_end);
            break;
        }
    } else if (ABS(x1 - x2) == ABS(y1 - y2)) {
        switch (blendMode) {
        case SDL_BLENDMODE_BLEND:
            DLINE(Uint16, DRAW_SETPIXEL_BLEND_RGB555, draw_end);
            break;
        case SDL_BLENDMODE_ADD:
            DLINE(Uint16, DRAW_SETPIXEL_ADD_RGB555, draw_end);
            break;
        case SDL_BLENDMODE_MOD:
            DLINE(Uint16, DRAW_SETPIXEL_MOD_RGB555, draw_end);
            break;
        default:
            DLINE(Uint16, DRAW_SETPIXEL_RGB555, draw_end);
            break;
        }
    } else {
        switch (blendMode) {
        case SDL_BLENDMODE_BLEND:
            AALINE(x1, y1, x2, y2,
                   DRAW_SETPIXELXY_BLEND_RGB555, DRAW_SETPIXELXY_BLEND_RGB555,
                   draw_end);
            break;
        case SDL_BLENDMODE_ADD:
            AALINE(x1, y1, x2, y2,
                   DRAW_SETPIXELXY_ADD_RGB555, DRAW_SETPIXELXY_ADD_RGB555,
                   draw_end);
            break;
        case SDL_BLENDMODE_MOD:
            AALINE(x1, y1, x2, y2,
                   DRAW_SETPIXELXY_MOD_RGB555, DRAW_SETPIXELXY_MOD_RGB555,
                   draw_end);
            break;
        default:
            AALINE(x1, y1, x2, y2,
                   DRAW_SETPIXELXY_RGB555, DRAW_SETPIXELXY_BLEND_RGB555,
                   draw_end);
            break;
        }
    }
}

static void
SDL_BlendLine_RGB565(SDL_Surface * dst, int x1, int y1, int x2, int y2,
                     SDL_BlendMode blendMode, Uint8 _r, Uint8 _g, Uint8 _b, Uint8 _a,
                     SDL_bool draw_end)
{
    const SDL_PixelFormat *fmt = dst->format;
    unsigned r, g, b, a, inva;

    if (blendMode == SDL_BLENDMODE_BLEND || blendMode == SDL_BLENDMODE_ADD) {
        r = DRAW_MUL(_r, _a);
        g = DRAW_MUL(_g, _a);
        b = DRAW_MUL(_b, _a);
        a = _a;
    } else {
        r = _r;
        g = _g;
        b = _b;
        a = _a;
    }
    inva = (a ^ 0xff);

    if (y1 == y2) {
        switch (blendMode) {
        case SDL_BLENDMODE_BLEND:
            HLINE(Uint16, DRAW_SETPIXEL_BLEND_RGB565, draw_end);
            break;
        case SDL_BLENDMODE_ADD:
            HLINE(Uint16, DRAW_SETPIXEL_ADD_RGB565, draw_end);
            break;
        case SDL_BLENDMODE_MOD:
            HLINE(Uint16, DRAW_SETPIXEL_MOD_RGB565, draw_end);
            break;
        default:
            HLINE(Uint16, DRAW_SETPIXEL_RGB565, draw_end);
            break;
        }
    } else if (x1 == x2) {
        switch (blendMode) {
        case SDL_BLENDMODE_BLEND:
            VLINE(Uint16, DRAW_SETPIXEL_BLEND_RGB565, draw_end);
            break;
        case SDL_BLENDMODE_ADD:
            VLINE(Uint16, DRAW_SETPIXEL_ADD_RGB565, draw_end);
            break;
        case SDL_BLENDMODE_MOD:
            VLINE(Uint16, DRAW_SETPIXEL_MOD_RGB565, draw_end);
            break;
        default:
            VLINE(Uint16, DRAW_SETPIXEL_RGB565, draw_end);
            break;
        }
    } else if (ABS(x1 - x2) == ABS(y1 - y2)) {
        switch (blendMode) {
        case SDL_BLENDMODE_BLEND:
            DLINE(Uint16, DRAW_SETPIXEL_BLEND_RGB565, draw_end);
            break;
        case SDL_BLENDMODE_ADD:
            DLINE(Uint16, DRAW_SETPIXEL_ADD_RGB565, draw_end);
            break;
        case SDL_BLENDMODE_MOD:
            DLINE(Uint16, DRAW_SETPIXEL_MOD_RGB565, draw_end);
            break;
        default:
            DLINE(Uint16, DRAW_SETPIXEL_RGB565, draw_end);
            break;
        }
    } else {
        switch (blendMode) {
        case SDL_BLENDMODE_BLEND:
            AALINE(x1, y1, x2, y2,
                   DRAW_SETPIXELXY_BLEND_RGB565, DRAW_SETPIXELXY_BLEND_RGB565,
                   draw_end);
            break;
        case SDL_BLENDMODE_ADD:
            AALINE(x1, y1, x2, y2,
                   DRAW_SETPIXELXY_ADD_RGB565, DRAW_SETPIXELXY_ADD_RGB565,
                   draw_end);
            break;
        case SDL_BLENDMODE_MOD:
            AALINE(x1, y1, x2, y2,
                   DRAW_SETPIXELXY_MOD_RGB565, DRAW_SETPIXELXY_MOD_RGB565,
                   draw_end);
            break;
        default:
            AALINE(x1, y1, x2, y2,
                   DRAW_SETPIXELXY_RGB565, DRAW_SETPIXELXY_BLEND_RGB565,
                   draw_end);
            break;
        }
    }
}

static void
SDL_BlendLine_RGB4(SDL_Surface * dst, int x1, int y1, int x2, int y2,
                   SDL_BlendMode blendMode, Uint8 _r, Uint8 _g, Uint8 _b, Uint8 _a,
                   SDL_bool draw_end)
{
    const SDL_PixelFormat *fmt = dst->format;
    unsigned r, g, b, a, inva;

    if (blendMode == SDL_BLENDMODE_BLEND || blendMode == SDL_BLENDMODE_ADD) {
        r = DRAW_MUL(_r, _a);
        g = DRAW_MUL(_g, _a);
        b = DRAW_MUL(_b, _a);
        a = _a;
    } else {
        r = _r;
        g = _g;
        b = _b;
        a = _a;
    }
    inva = (a ^ 0xff);

    if (y1 == y2) {
        switch (blendMode) {
        case SDL_BLENDMODE_BLEND:
            HLINE(Uint32, DRAW_SETPIXEL_BLEND_RGB, draw_end);
            break;
        case SDL_BLENDMODE_ADD:
            HLINE(Uint32, DRAW_SETPIXEL_ADD_RGB, draw_end);
            break;
        case SDL_BLENDMODE_MOD:
            HLINE(Uint32, DRAW_SETPIXEL_MOD_RGB, draw_end);
            break;
        default:
            HLINE(Uint32, DRAW_SETPIXEL_RGB, draw_end);
            break;
        }
    } else if (x1 == x2) {
        switch (blendMode) {
        case SDL_BLENDMODE_BLEND:
            VLINE(Uint32, DRAW_SETPIXEL_BLEND_RGB, draw_end);
            break;
        case SDL_BLENDMODE_ADD:
            VLINE(Uint32, DRAW_SETPIXEL_ADD_RGB, draw_end);
            break;
        case SDL_BLENDMODE_MOD:
            VLINE(Uint32, DRAW_SETPIXEL_MOD_RGB, draw_end);
            break;
        default:
            VLINE(Uint32, DRAW_SETPIXEL_RGB, draw_end);
            break;
        }
    } else if (ABS(x1 - x2) == ABS(y1 - y2)) {
        switch (blendMode) {
        case SDL_BLENDMODE_BLEND:
            DLINE(Uint32, DRAW_SETPIXEL_BLEND_RGB, draw_end);
            break;
        case SDL_BLENDMODE_ADD:
            DLINE(Uint32, DRAW_SETPIXEL_ADD_RGB, draw_end);
            break;
        case SDL_BLENDMODE_MOD:
            DLINE(Uint32, DRAW_SETPIXEL_MOD_RGB, draw_end);
            break;
        default:
            DLINE(Uint32, DRAW_SETPIXEL_RGB, draw_end);
            break;
        }
    } else {
        switch (blendMode) {
        case SDL_BLENDMODE_BLEND:
            AALINE(x1, y1, x2, y2,
                   DRAW_SETPIXELXY4_BLEND_RGB, DRAW_SETPIXELXY4_BLEND_RGB,
                   draw_end);
            break;
        case SDL_BLENDMODE_ADD:
            AALINE(x1, y1, x2, y2,
                   DRAW_SETPIXELXY4_ADD_RGB, DRAW_SETPIXELXY4_ADD_RGB,
                   draw_end);
            break;
        case SDL_BLENDMODE_MOD:
            AALINE(x1, y1, x2, y2,
                   DRAW_SETPIXELXY4_MOD_RGB, DRAW_SETPIXELXY4_MOD_RGB,
                   draw_end);
            break;
        default:
            AALINE(x1, y1, x2, y2,
                   DRAW_SETPIXELXY4_RGB, DRAW_SETPIXELXY4_BLEND_RGB,
                   draw_end);
            break;
        }
    }
}

static void
SDL_BlendLine_RGBA4(SDL_Surface * dst, int x1, int y1, int x2, int y2,
                    SDL_BlendMode blendMode, Uint8 _r, Uint8 _g, Uint8 _b, Uint8 _a,
                    SDL_bool draw_end)
{
    const SDL_PixelFormat *fmt = dst->format;
    unsigned r, g, b, a, inva;

    if (blendMode == SDL_BLENDMODE_BLEND || blendMode == SDL_BLENDMODE_ADD) {
        r = DRAW_MUL(_r, _a);
        g = DRAW_MUL(_g, _a);
        b = DRAW_MUL(_b, _a);
        a = _a;
    } else {
        r = _r;
        g = _g;
        b = _b;
        a = _a;
    }
    inva = (a ^ 0xff);

    if (y1 == y2) {
        switch (blendMode) {
        case SDL_BLENDMODE_BLEND:
            HLINE(Uint32, DRAW_SETPIXEL_BLEND_RGBA, draw_end);
            break;
        case SDL_BLENDMODE_ADD:
            HLINE(Uint32, DRAW_SETPIXEL_ADD_RGBA, draw_end);
            break;
        case SDL_BLENDMODE_MOD:
            HLINE(Uint32, DRAW_SETPIXEL_MOD_RGBA, draw_end);
            break;
        default:
            HLINE(Uint32, DRAW_SETPIXEL_RGBA, draw_end);
            break;
        }
    } else if (x1 == x2) {
        switch (blendMode) {
        case SDL_BLENDMODE_BLEND:
            VLINE(Uint32, DRAW_SETPIXEL_BLEND_RGBA, draw_end);
            break;
        case SDL_BLENDMODE_ADD:
            VLINE(Uint32, DRAW_SETPIXEL_ADD_RGBA, draw_end);
            break;
        case SDL_BLENDMODE_MOD:
            VLINE(Uint32, DRAW_SETPIXEL_MOD_RGBA, draw_end);
            break;
        default:
            VLINE(Uint32, DRAW_SETPIXEL_RGBA, draw_end);
            break;
        }
    } else if (ABS(x1 - x2) == ABS(y1 - y2)) {
        switch (blendMode) {
        case SDL_BLENDMODE_BLEND:
            DLINE(Uint32, DRAW_SETPIXEL_BLEND_RGBA, draw_end);
            break;
        case SDL_BLENDMODE_ADD:
            DLINE(Uint32, DRAW_SETPIXEL_ADD_RGBA, draw_end);
            break;
        case SDL_BLENDMODE_MOD:
            DLINE(Uint32, DRAW_SETPIXEL_MOD_RGBA, draw_end);
            break;
        default:
            DLINE(Uint32, DRAW_SETPIXEL_RGBA, draw_end);
            break;
        }
    } else {
        switch (blendMode) {
        case SDL_BLENDMODE_BLEND:
            AALINE(x1, y1, x2, y2,
                   DRAW_SETPIXELXY4_BLEND_RGBA, DRAW_SETPIXELXY4_BLEND_RGBA,
                   draw_end);
            break;
        case SDL_BLENDMODE_ADD:
            AALINE(x1, y1, x2, y2,
                   DRAW_SETPIXELXY4_ADD_RGBA, DRAW_SETPIXELXY4_ADD_RGBA,
                   draw_end);
            break;
        case SDL_BLENDMODE_MOD:
            AALINE(x1, y1, x2, y2,
                   DRAW_SETPIXELXY4_MOD_RGBA, DRAW_SETPIXELXY4_MOD_RGBA,
                   draw_end);
            break;
        default:
            AALINE(x1, y1, x2, y2,
                   DRAW_SETPIXELXY4_RGBA, DRAW_SETPIXELXY4_BLEND_RGBA,
                   draw_end);
            break;
        }
    }
}

static void
SDL_BlendLine_RGB888(SDL_Surface * dst, int x1, int y1, int x2, int y2,
                     SDL_BlendMode blendMode, Uint8 _r, Uint8 _g, Uint8 _b, Uint8 _a,
                     SDL_bool draw_end)
{
    const SDL_PixelFormat *fmt = dst->format;
    unsigned r, g, b, a, inva;

    if (blendMode == SDL_BLENDMODE_BLEND || blendMode == SDL_BLENDMODE_ADD) {
        r = DRAW_MUL(_r, _a);
        g = DRAW_MUL(_g, _a);
        b = DRAW_MUL(_b, _a);
        a = _a;
    } else {
        r = _r;
        g = _g;
        b = _b;
        a = _a;
    }
    inva = (a ^ 0xff);

    if (y1 == y2) {
        switch (blendMode) {
        case SDL_BLENDMODE_BLEND:
            HLINE(Uint32, DRAW_SETPIXEL_BLEND_RGB888, draw_end);
            break;
        case SDL_BLENDMODE_ADD:
            HLINE(Uint32, DRAW_SETPIXEL_ADD_RGB888, draw_end);
            break;
        case SDL_BLENDMODE_MOD:
            HLINE(Uint32, DRAW_SETPIXEL_MOD_RGB888, draw_end);
            break;
        default:
            HLINE(Uint32, DRAW_SETPIXEL_RGB888, draw_end);
            break;
        }
    } else if (x1 == x2) {
        switch (blendMode) {
        case SDL_BLENDMODE_BLEND:
            VLINE(Uint32, DRAW_SETPIXEL_BLEND_RGB888, draw_end);
            break;
        case SDL_BLENDMODE_ADD:
            VLINE(Uint32, DRAW_SETPIXEL_ADD_RGB888, draw_end);
            break;
        case SDL_BLENDMODE_MOD:
            VLINE(Uint32, DRAW_SETPIXEL_MOD_RGB888, draw_end);
            break;
        default:
            VLINE(Uint32, DRAW_SETPIXEL_RGB888, draw_end);
            break;
        }
    } else if (ABS(x1 - x2) == ABS(y1 - y2)) {
        switch (blendMode) {
        case SDL_BLENDMODE_BLEND:
            DLINE(Uint32, DRAW_SETPIXEL_BLEND_RGB888, draw_end);
            break;
        case SDL_BLENDMODE_ADD:
            DLINE(Uint32, DRAW_SETPIXEL_ADD_RGB888, draw_end);
            break;
        case SDL_BLENDMODE_MOD:
            DLINE(Uint32, DRAW_SETPIXEL_MOD_RGB888, draw_end);
            break;
        default:
            DLINE(Uint32, DRAW_SETPIXEL_RGB888, draw_end);
            break;
        }
    } else {
        switch (blendMode) {
        case SDL_BLENDMODE_BLEND:
            AALINE(x1, y1, x2, y2,
                   DRAW_SETPIXELXY_BLEND_RGB888, DRAW_SETPIXELXY_BLEND_RGB888,
                   draw_end);
            break;
        case SDL_BLENDMODE_ADD:
            AALINE(x1, y1, x2, y2,
                   DRAW_SETPIXELXY_ADD_RGB888, DRAW_SETPIXELXY_ADD_RGB888,
                   draw_end);
            break;
        case SDL_BLENDMODE_MOD:
            AALINE(x1, y1, x2, y2,
                   DRAW_SETPIXELXY_MOD_RGB888, DRAW_SETPIXELXY_MOD_RGB888,
                   draw_end);
            break;
        default:
            AALINE(x1, y1, x2, y2,
                   DRAW_SETPIXELXY_RGB888, DRAW_SETPIXELXY_BLEND_RGB888,
                   draw_end);
            break;
        }
    }
}

static void
SDL_BlendLine_ARGB8888(SDL_Surface * dst, int x1, int y1, int x2, int y2,
                       SDL_BlendMode blendMode, Uint8 _r, Uint8 _g, Uint8 _b, Uint8 _a,
                       SDL_bool draw_end)
{
    const SDL_PixelFormat *fmt = dst->format;
    unsigned r, g, b, a, inva;

    if (blendMode == SDL_BLENDMODE_BLEND || blendMode == SDL_BLENDMODE_ADD) {
        r = DRAW_MUL(_r, _a);
        g = DRAW_MUL(_g, _a);
        b = DRAW_MUL(_b, _a);
        a = _a;
    } else {
        r = _r;
        g = _g;
        b = _b;
        a = _a;
    }
    inva = (a ^ 0xff);

    if (y1 == y2) {
        switch (blendMode) {
        case SDL_BLENDMODE_BLEND:
            HLINE(Uint32, DRAW_SETPIXEL_BLEND_ARGB8888, draw_end);
            break;
        case SDL_BLENDMODE_ADD:
            HLINE(Uint32, DRAW_SETPIXEL_ADD_ARGB8888, draw_end);
            break;
        case SDL_BLENDMODE_MOD:
            HLINE(Uint32, DRAW_SETPIXEL_MOD_ARGB8888, draw_end);
            break;
        default:
            HLINE(Uint32, DRAW_SETPIXEL_ARGB8888, draw_end);
            break;
        }
    } else if (x1 == x2) {
        switch (blendMode) {
        case SDL_BLENDMODE_BLEND:
            VLINE(Uint32, DRAW_SETPIXEL_BLEND_ARGB8888, draw_end);
            break;
        case SDL_BLENDMODE_ADD:
            VLINE(Uint32, DRAW_SETPIXEL_ADD_ARGB8888, draw_end);
            break;
        case SDL_BLENDMODE_MOD:
            VLINE(Uint32, DRAW_SETPIXEL_MOD_ARGB8888, draw_end);
            break;
        default:
            VLINE(Uint32, DRAW_SETPIXEL_ARGB8888, draw_end);
            break;
        }
    } else if (ABS(x1 - x2) == ABS(y1 - y2)) {
        switch (blendMode) {
        case SDL_BLENDMODE_BLEND:
            DLINE(Uint32, DRAW_SETPIXEL_BLEND_ARGB8888, draw_end);
            break;
        case SDL_BLENDMODE_ADD:
            DLINE(Uint32, DRAW_SETPIXEL_ADD_ARGB8888, draw_end);
            break;
        case SDL_BLENDMODE_MOD:
            DLINE(Uint32, DRAW_SETPIXEL_MOD_ARGB8888, draw_end);
            break;
        default:
            DLINE(Uint32, DRAW_SETPIXEL_ARGB8888, draw_end);
            break;
        }
    } else {
        switch (blendMode) {
        case SDL_BLENDMODE_BLEND:
            AALINE(x1, y1, x2, y2,
                   DRAW_SETPIXELXY_BLEND_ARGB8888, DRAW_SETPIXELXY_BLEND_ARGB8888,
                   draw_end);
            break;
        case SDL_BLENDMODE_ADD:
            AALINE(x1, y1, x2, y2,
                   DRAW_SETPIXELXY_ADD_ARGB8888, DRAW_SETPIXELXY_ADD_ARGB8888,
                   draw_end);
            break;
        case SDL_BLENDMODE_MOD:
            AALINE(x1, y1, x2, y2,
                   DRAW_SETPIXELXY_MOD_ARGB8888, DRAW_SETPIXELXY_MOD_ARGB8888,
                   draw_end);
            break;
        default:
            AALINE(x1, y1, x2, y2,
                   DRAW_SETPIXELXY_ARGB8888, DRAW_SETPIXELXY_BLEND_ARGB8888,
                   draw_end);
            break;
        }
    }
}

typedef void (*BlendLineFunc) (SDL_Surface * dst,
                               int x1, int y1, int x2, int y2,
                               SDL_BlendMode blendMode,
                               Uint8 r, Uint8 g, Uint8 b, Uint8 a,
                               SDL_bool draw_end);

static BlendLineFunc
SDL_CalculateBlendLineFunc(const SDL_PixelFormat * fmt)
{
    switch (fmt->BytesPerPixel) {
    case 2:
        if (fmt->Rmask == 0x7C00) {
            return SDL_BlendLine_RGB555;
        } else if (fmt->Rmask == 0xF800) {
            return SDL_BlendLine_RGB565;
        } else {
            return SDL_BlendLine_RGB2;
        }
        break;
    case 4:
        if (fmt->Rmask == 0x00FF0000) {
            if (fmt->Amask) {
                return SDL_BlendLine_ARGB8888;
            } else {
                return SDL_BlendLine_RGB888;
            }
        } else {
            if (fmt->Amask) {
                return SDL_BlendLine_RGBA4;
            } else {
                return SDL_BlendLine_RGB4;
            }
        }
    }
    return NULL;
}

int
SDL_BlendLine(SDL_Surface * dst, int x1, int y1, int x2, int y2,
              SDL_BlendMode blendMode, Uint8 r, Uint8 g, Uint8 b, Uint8 a)
{
    BlendLineFunc func;

    if (!dst) {
        SDL_SetError("SDL_BlendLine(): Passed NULL destination surface");
        return -1;
    }

    func = SDL_CalculateBlendLineFunc(dst->format);
    if (!func) {
        SDL_SetError("SDL_BlendLine(): Unsupported surface format");
        return -1;
    }

    /* Perform clipping */
    /* FIXME: We don't actually want to clip, as it may change line slope */
    if (!SDL_IntersectRectAndLine(&dst->clip_rect, &x1, &y1, &x2, &y2)) {
        return 0;
    }

    func(dst, x1, y1, x2, y2, blendMode, r, g, b, a, SDL_TRUE);
    return 0;
}

int
SDL_BlendLines(SDL_Surface * dst, const SDL_Point * points, int count,
               SDL_BlendMode blendMode, Uint8 r, Uint8 g, Uint8 b, Uint8 a)
{
    int i;
    int x1, y1;
    int x2, y2;
    SDL_bool draw_end;
    BlendLineFunc func;

    if (!dst) {
        SDL_SetError("SDL_BlendLines(): Passed NULL destination surface");
        return -1;
    }

    func = SDL_CalculateBlendLineFunc(dst->format);
    if (!func) {
        SDL_SetError("SDL_BlendLines(): Unsupported surface format");
        return -1;
    }

    for (i = 1; i < count; ++i) {
        x1 = points[i-1].x;
        y1 = points[i-1].y;
        x2 = points[i].x;
        y2 = points[i].y;

        /* Perform clipping */
        /* FIXME: We don't actually want to clip, as it may change line slope */
        if (!SDL_IntersectRectAndLine(&dst->clip_rect, &x1, &y1, &x2, &y2)) {
            continue;
        }

        /* Draw the end if it was clipped */
        draw_end = (x2 != points[i].x || y2 != points[i].y);

        func(dst, x1, y1, x2, y2, blendMode, r, g, b, a, draw_end);
    }
    if (points[0].x != points[count-1].x || points[0].y != points[count-1].y) {
        SDL_BlendPoint(dst, points[count-1].x, points[count-1].y,
                       blendMode, r, g, b, a);
    }
    return 0;
}

#endif /* !SDL_RENDER_DISABLED */

/* vi: set ts=4 sw=4 expandtab: */
