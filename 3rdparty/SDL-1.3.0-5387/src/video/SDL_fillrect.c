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
#include "SDL_blit.h"


#ifdef __SSE__
/* *INDENT-OFF* */

#ifdef _MSC_VER
#define SSE_BEGIN \
    __m128 c128; \
    c128.m128_u32[0] = color; \
    c128.m128_u32[1] = color; \
    c128.m128_u32[2] = color; \
    c128.m128_u32[3] = color;
#else
#define SSE_BEGIN \
    DECLARE_ALIGNED(Uint32, cccc[4], 16); \
    cccc[0] = color; \
    cccc[1] = color; \
    cccc[2] = color; \
    cccc[3] = color; \
    __m128 c128 = *(__m128 *)cccc;
#endif

#define SSE_WORK \
    for (i = n / 64; i--;) { \
        _mm_stream_ps((float *)(p+0), c128); \
        _mm_stream_ps((float *)(p+16), c128); \
        _mm_stream_ps((float *)(p+32), c128); \
        _mm_stream_ps((float *)(p+48), c128); \
        p += 64; \
    }

#define SSE_END

#define DEFINE_SSE_FILLRECT(bpp, type) \
static void \
SDL_FillRect##bpp##SSE(Uint8 *pixels, int pitch, Uint32 color, int w, int h) \
{ \
    SSE_BEGIN; \
 \
    while (h--) { \
        int i, n = w * bpp; \
        Uint8 *p = pixels; \
 \
        if (n > 63) { \
            int adjust = 16 - ((uintptr_t)p & 15); \
            if (adjust < 16) { \
                n -= adjust; \
                adjust /= bpp; \
                while (adjust--) { \
                    *((type *)p) = (type)color; \
                    p += bpp; \
                } \
            } \
            SSE_WORK; \
        } \
        if (n & 63) { \
            int remainder = (n & 63); \
            remainder /= bpp; \
            while (remainder--) { \
                *((type *)p) = (type)color; \
                p += bpp; \
            } \
        } \
        pixels += pitch; \
    } \
 \
    SSE_END; \
}

static void
SDL_FillRect1SSE(Uint8 *pixels, int pitch, Uint32 color, int w, int h)
{
    SSE_BEGIN;

    while (h--) {
        int i, n = w;
        Uint8 *p = pixels;

        if (n > 63) {
            int adjust = 16 - ((uintptr_t)p & 15);
            if (adjust) {
                n -= adjust;
                SDL_memset(p, color, adjust);
                p += adjust;
            }
            SSE_WORK;
        }
        if (n & 63) {
            int remainder = (n & 63);
            SDL_memset(p, color, remainder);
            p += remainder;
        }
        pixels += pitch;
    }

    SSE_END;
}
/*DEFINE_SSE_FILLRECT(1, Uint8)*/
DEFINE_SSE_FILLRECT(2, Uint16)
DEFINE_SSE_FILLRECT(4, Uint32)

/* *INDENT-ON* */
#endif /* __SSE__ */

#ifdef __MMX__
/* *INDENT-OFF* */

#define MMX_BEGIN \
    __m64 c64 = _mm_set_pi32(color, color)

#define MMX_WORK \
    for (i = n / 64; i--;) { \
        _mm_stream_pi((__m64 *)(p+0), c64); \
        _mm_stream_pi((__m64 *)(p+8), c64); \
        _mm_stream_pi((__m64 *)(p+16), c64); \
        _mm_stream_pi((__m64 *)(p+24), c64); \
        _mm_stream_pi((__m64 *)(p+32), c64); \
        _mm_stream_pi((__m64 *)(p+40), c64); \
        _mm_stream_pi((__m64 *)(p+48), c64); \
        _mm_stream_pi((__m64 *)(p+56), c64); \
        p += 64; \
    }

#define MMX_END \
    _mm_empty()

#define DEFINE_MMX_FILLRECT(bpp, type) \
static void \
SDL_FillRect##bpp##MMX(Uint8 *pixels, int pitch, Uint32 color, int w, int h) \
{ \
    MMX_BEGIN; \
 \
    while (h--) { \
        int i, n = w * bpp; \
        Uint8 *p = pixels; \
 \
        if (n > 63) { \
            int adjust = 8 - ((uintptr_t)p & 7); \
            if (adjust < 8) { \
                n -= adjust; \
                adjust /= bpp; \
                while (adjust--) { \
                    *((type *)p) = (type)color; \
                    p += bpp; \
                } \
            } \
            MMX_WORK; \
        } \
        if (n & 63) { \
            int remainder = (n & 63); \
            remainder /= bpp; \
            while (remainder--) { \
                *((type *)p) = (type)color; \
                p += bpp; \
            } \
        } \
        pixels += pitch; \
    } \
 \
    MMX_END; \
}

static void
SDL_FillRect1MMX(Uint8 *pixels, int pitch, Uint32 color, int w, int h)
{
    MMX_BEGIN;

    while (h--) {
        int i, n = w;
        Uint8 *p = pixels;

        if (n > 63) {
            int adjust = 8 - ((uintptr_t)p & 7);
            if (adjust) {
                n -= adjust;
                SDL_memset(p, color, adjust);
                p += adjust;
            }
            MMX_WORK;
        }
        if (n & 63) {
            int remainder = (n & 63);
            SDL_memset(p, color, remainder);
            p += remainder;
        }
        pixels += pitch;
    }

    MMX_END;
}
/*DEFINE_MMX_FILLRECT(1, Uint8)*/
DEFINE_MMX_FILLRECT(2, Uint16)
DEFINE_MMX_FILLRECT(4, Uint32)

/* *INDENT-ON* */
#endif /* __MMX__ */

static void
SDL_FillRect1(Uint8 * pixels, int pitch, Uint32 color, int w, int h)
{
    while (h--) {
        int n = w;
        Uint8 *p = pixels;

        if (n > 3) {
            switch ((uintptr_t) p & 3) {
            case 1:
                *p++ = (Uint8) color;
                --n;
            case 2:
                *p++ = (Uint8) color;
                --n;
            case 3:
                *p++ = (Uint8) color;
                --n;
            }
            SDL_memset4(p, color, (n >> 2));
        }
        if (n & 3) {
            p += (n & ~3);
            switch (n & 3) {
            case 3:
                *p++ = (Uint8) color;
            case 2:
                *p++ = (Uint8) color;
            case 1:
                *p++ = (Uint8) color;
            }
        }
        pixels += pitch;
    }
}

static void
SDL_FillRect2(Uint8 * pixels, int pitch, Uint32 color, int w, int h)
{
    while (h--) {
        int n = w;
        Uint16 *p = (Uint16 *) pixels;

        if (n > 1) {
            if ((uintptr_t) p & 2) {
                *p++ = (Uint16) color;
                --n;
            }
            SDL_memset4(p, color, (n >> 1));
        }
        if (n & 1) {
            p[n - 1] = (Uint16) color;
        }
        pixels += pitch;
    }
}

static void
SDL_FillRect3(Uint8 * pixels, int pitch, Uint32 color, int w, int h)
{
    Uint8 r = (Uint8) ((color >> 16) & 0xFF);
    Uint8 g = (Uint8) ((color >> 8) & 0xFF);
    Uint8 b = (Uint8) (color & 0xFF);

    while (h--) {
        int n = w;
        Uint8 *p = pixels;

        while (n--) {
            *p++ = r;
            *p++ = g;
            *p++ = b;
        }
        pixels += pitch;
    }
}

static void
SDL_FillRect4(Uint8 * pixels, int pitch, Uint32 color, int w, int h)
{
    while (h--) {
        SDL_memset4(pixels, color, w);
        pixels += pitch;
    }
}

/* 
 * This function performs a fast fill of the given rectangle with 'color'
 */
int
SDL_FillRect(SDL_Surface * dst, const SDL_Rect * rect, Uint32 color)
{
    SDL_Rect clipped;
    Uint8 *pixels;

    if (!dst) {
        SDL_SetError("Passed NULL destination surface");
        return -1;
    }

    /* This function doesn't work on surfaces < 8 bpp */
    if (dst->format->BitsPerPixel < 8) {
        SDL_SetError("SDL_FillRect(): Unsupported surface format");
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

    /* Perform software fill */
    if (!dst->pixels) {
        SDL_SetError("SDL_FillRect(): You must lock the surface");
        return (-1);
    }

    pixels = (Uint8 *) dst->pixels + rect->y * dst->pitch +
                                     rect->x * dst->format->BytesPerPixel;

    switch (dst->format->BytesPerPixel) {
    case 1:
        {
            color |= (color << 8);
            color |= (color << 16);
#ifdef __SSE__
            if (SDL_HasSSE()) {
                SDL_FillRect1SSE(pixels, dst->pitch, color, rect->w, rect->h);
                break;
            }
#endif
#ifdef __MMX__
            if (SDL_HasMMX()) {
                SDL_FillRect1MMX(pixels, dst->pitch, color, rect->w, rect->h);
                break;
            }
#endif
            SDL_FillRect1(pixels, dst->pitch, color, rect->w, rect->h);
            break;
        }

    case 2:
        {
            color |= (color << 16);
#ifdef __SSE__
            if (SDL_HasSSE()) {
                SDL_FillRect2SSE(pixels, dst->pitch, color, rect->w, rect->h);
                break;
            }
#endif
#ifdef __MMX__
            if (SDL_HasMMX()) {
                SDL_FillRect2MMX(pixels, dst->pitch, color, rect->w, rect->h);
                break;
            }
#endif
            SDL_FillRect2(pixels, dst->pitch, color, rect->w, rect->h);
            break;
        }

    case 3:
        /* 24-bit RGB is a slow path, at least for now. */
        {
            SDL_FillRect3(pixels, dst->pitch, color, rect->w, rect->h);
            break;
        }

    case 4:
        {
#ifdef __SSE__
            if (SDL_HasSSE()) {
                SDL_FillRect4SSE(pixels, dst->pitch, color, rect->w, rect->h);
                break;
            }
#endif
#ifdef __MMX__
            if (SDL_HasMMX()) {
                SDL_FillRect4MMX(pixels, dst->pitch, color, rect->w, rect->h);
                break;
            }
#endif
            SDL_FillRect4(pixels, dst->pitch, color, rect->w, rect->h);
            break;
        }
    }

    /* We're done! */
    return 0;
}

int
SDL_FillRects(SDL_Surface * dst, const SDL_Rect * rects, int count,
              Uint32 color)
{
    int i;
    int status = 0;

    if (!rects) {
        SDL_SetError("SDL_FillRects() passed NULL rects");
        return -1;
    }

    for (i = 0; i < count; ++i) {
        status += SDL_FillRect(dst, &rects[i], color);
    }
    return status;
}

/* vi: set ts=4 sw=4 expandtab: */
