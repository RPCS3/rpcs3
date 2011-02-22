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
#include "SDL_blendfillrect.h"


static int
SDL_BlendFillRect_RGB555(SDL_Surface * dst, const SDL_Rect * rect,
                         SDL_BlendMode blendMode, Uint8 r, Uint8 g, Uint8 b, Uint8 a)
{
    unsigned inva = 0xff - a;

    switch (blendMode) {
    case SDL_BLENDMODE_BLEND:
        FILLRECT(Uint16, DRAW_SETPIXEL_BLEND_RGB555);
        break;
    case SDL_BLENDMODE_ADD:
        FILLRECT(Uint16, DRAW_SETPIXEL_ADD_RGB555);
        break;
    case SDL_BLENDMODE_MOD:
        FILLRECT(Uint16, DRAW_SETPIXEL_MOD_RGB555);
        break;
    default:
        FILLRECT(Uint16, DRAW_SETPIXEL_RGB555);
        break;
    }
    return 0;
}

static int
SDL_BlendFillRect_RGB565(SDL_Surface * dst, const SDL_Rect * rect,
                         SDL_BlendMode blendMode, Uint8 r, Uint8 g, Uint8 b, Uint8 a)
{
    unsigned inva = 0xff - a;

    switch (blendMode) {
    case SDL_BLENDMODE_BLEND:
        FILLRECT(Uint16, DRAW_SETPIXEL_BLEND_RGB565);
        break;
    case SDL_BLENDMODE_ADD:
        FILLRECT(Uint16, DRAW_SETPIXEL_ADD_RGB565);
        break;
    case SDL_BLENDMODE_MOD:
        FILLRECT(Uint16, DRAW_SETPIXEL_MOD_RGB565);
        break;
    default:
        FILLRECT(Uint16, DRAW_SETPIXEL_RGB565);
        break;
    }
    return 0;
}

static int
SDL_BlendFillRect_RGB888(SDL_Surface * dst, const SDL_Rect * rect,
                         SDL_BlendMode blendMode, Uint8 r, Uint8 g, Uint8 b, Uint8 a)
{
    unsigned inva = 0xff - a;

    switch (blendMode) {
    case SDL_BLENDMODE_BLEND:
        FILLRECT(Uint32, DRAW_SETPIXEL_BLEND_RGB888);
        break;
    case SDL_BLENDMODE_ADD:
        FILLRECT(Uint32, DRAW_SETPIXEL_ADD_RGB888);
        break;
    case SDL_BLENDMODE_MOD:
        FILLRECT(Uint32, DRAW_SETPIXEL_MOD_RGB888);
        break;
    default:
        FILLRECT(Uint32, DRAW_SETPIXEL_RGB888);
        break;
    }
    return 0;
}

static int
SDL_BlendFillRect_ARGB8888(SDL_Surface * dst, const SDL_Rect * rect,
                           SDL_BlendMode blendMode, Uint8 r, Uint8 g, Uint8 b, Uint8 a)
{
    unsigned inva = 0xff - a;

    switch (blendMode) {
    case SDL_BLENDMODE_BLEND:
        FILLRECT(Uint32, DRAW_SETPIXEL_BLEND_ARGB8888);
        break;
    case SDL_BLENDMODE_ADD:
        FILLRECT(Uint32, DRAW_SETPIXEL_ADD_ARGB8888);
        break;
    case SDL_BLENDMODE_MOD:
        FILLRECT(Uint32, DRAW_SETPIXEL_MOD_ARGB8888);
        break;
    default:
        FILLRECT(Uint32, DRAW_SETPIXEL_ARGB8888);
        break;
    }
    return 0;
}

static int
SDL_BlendFillRect_RGB(SDL_Surface * dst, const SDL_Rect * rect,
                      SDL_BlendMode blendMode, Uint8 r, Uint8 g, Uint8 b, Uint8 a)
{
    SDL_PixelFormat *fmt = dst->format;
    unsigned inva = 0xff - a;

    switch (fmt->BytesPerPixel) {
    case 2:
        switch (blendMode) {
        case SDL_BLENDMODE_BLEND:
            FILLRECT(Uint16, DRAW_SETPIXEL_BLEND_RGB);
            break;
        case SDL_BLENDMODE_ADD:
            FILLRECT(Uint16, DRAW_SETPIXEL_ADD_RGB);
            break;
        case SDL_BLENDMODE_MOD:
            FILLRECT(Uint16, DRAW_SETPIXEL_MOD_RGB);
            break;
        default:
            FILLRECT(Uint16, DRAW_SETPIXEL_RGB);
            break;
        }
        return 0;
    case 4:
        switch (blendMode) {
        case SDL_BLENDMODE_BLEND:
            FILLRECT(Uint32, DRAW_SETPIXEL_BLEND_RGB);
            break;
        case SDL_BLENDMODE_ADD:
            FILLRECT(Uint32, DRAW_SETPIXEL_ADD_RGB);
            break;
        case SDL_BLENDMODE_MOD:
            FILLRECT(Uint32, DRAW_SETPIXEL_MOD_RGB);
            break;
        default:
            FILLRECT(Uint32, DRAW_SETPIXEL_RGB);
            break;
        }
        return 0;
    default:
        SDL_Unsupported();
        return -1;
    }
}

static int
SDL_BlendFillRect_RGBA(SDL_Surface * dst, const SDL_Rect * rect,
                       SDL_BlendMode blendMode, Uint8 r, Uint8 g, Uint8 b, Uint8 a)
{
    SDL_PixelFormat *fmt = dst->format;
    unsigned inva = 0xff - a;

    switch (fmt->BytesPerPixel) {
    case 4:
        switch (blendMode) {
        case SDL_BLENDMODE_BLEND:
            FILLRECT(Uint32, DRAW_SETPIXEL_BLEND_RGBA);
            break;
        case SDL_BLENDMODE_ADD:
            FILLRECT(Uint32, DRAW_SETPIXEL_ADD_RGBA);
            break;
        case SDL_BLENDMODE_MOD:
            FILLRECT(Uint32, DRAW_SETPIXEL_MOD_RGBA);
            break;
        default:
            FILLRECT(Uint32, DRAW_SETPIXEL_RGBA);
            break;
        }
        return 0;
    default:
        SDL_Unsupported();
        return -1;
    }
}

int
SDL_BlendFillRect(SDL_Surface * dst, const SDL_Rect * rect,
                  SDL_BlendMode blendMode, Uint8 r, Uint8 g, Uint8 b, Uint8 a)
{
    SDL_Rect clipped;

    if (!dst) {
        SDL_SetError("Passed NULL destination surface");
        return -1;
    }

    /* This function doesn't work on surfaces < 8 bpp */
    if (dst->format->BitsPerPixel < 8) {
        SDL_SetError("SDL_BlendFillRect(): Unsupported surface format");
        return -1;
    }

    /* If 'rect' == NULL, then fill the whole surface */
    if (rect) {
        /* Perform clipping */
        if (!SDL_IntersectRect(rect, &dst->clip_rect, &clipped)) {
            return 0;
        }
        rect = &clipped;
    } else {
        rect = &dst->clip_rect;
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
            return SDL_BlendFillRect_RGB555(dst, rect, blendMode, r, g, b, a);
        }
        break;
    case 16:
        switch (dst->format->Rmask) {
        case 0xF800:
            return SDL_BlendFillRect_RGB565(dst, rect, blendMode, r, g, b, a);
        }
        break;
    case 32:
        switch (dst->format->Rmask) {
        case 0x00FF0000:
            if (!dst->format->Amask) {
                return SDL_BlendFillRect_RGB888(dst, rect, blendMode, r, g, b, a);
            } else {
                return SDL_BlendFillRect_ARGB8888(dst, rect, blendMode, r, g, b, a);
            }
            break;
        }
        break;
    default:
        break;
    }

    if (!dst->format->Amask) {
        return SDL_BlendFillRect_RGB(dst, rect, blendMode, r, g, b, a);
    } else {
        return SDL_BlendFillRect_RGBA(dst, rect, blendMode, r, g, b, a);
    }
}

int
SDL_BlendFillRects(SDL_Surface * dst, const SDL_Rect * rects, int count,
                   SDL_BlendMode blendMode, Uint8 r, Uint8 g, Uint8 b, Uint8 a)
{
    SDL_Rect rect;
    int i;
    int (*func)(SDL_Surface * dst, const SDL_Rect * rect,
                SDL_BlendMode blendMode, Uint8 r, Uint8 g, Uint8 b, Uint8 a) = NULL;
    int status = 0;

    if (!dst) {
        SDL_SetError("Passed NULL destination surface");
        return -1;
    }

    /* This function doesn't work on surfaces < 8 bpp */
    if (dst->format->BitsPerPixel < 8) {
        SDL_SetError("SDL_BlendFillRects(): Unsupported surface format");
        return -1;
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
            func = SDL_BlendFillRect_RGB555;
        }
        break;
    case 16:
        switch (dst->format->Rmask) {
        case 0xF800:
            func = SDL_BlendFillRect_RGB565;
        }
        break;
    case 32:
        switch (dst->format->Rmask) {
        case 0x00FF0000:
            if (!dst->format->Amask) {
                func = SDL_BlendFillRect_RGB888;
            } else {
                func = SDL_BlendFillRect_ARGB8888;
            }
            break;
        }
        break;
    default:
        break;
    }

    if (!func) {
        if (!dst->format->Amask) {
            func = SDL_BlendFillRect_RGB;
        } else {
            func = SDL_BlendFillRect_RGBA;
        }
    }

    for (i = 0; i < count; ++i) {
        /* Perform clipping */
        if (!SDL_IntersectRect(&rects[i], &dst->clip_rect, &rect)) {
            continue;
        }
        status = func(dst, &rect, blendMode, r, g, b, a);
    }
    return status;
}

#endif /* !SDL_RENDER_DISABLED */

/* vi: set ts=4 sw=4 expandtab: */
