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
#include "SDL_blit_copy.h"


#ifdef __SSE__
/* This assumes 16-byte aligned src and dst */
static __inline__ void
SDL_memcpySSE(Uint8 * dst, const Uint8 * src, int len)
{
    int i;

    __m128 values[4];
    for (i = len / 64; i--;) {
        _mm_prefetch(src, _MM_HINT_NTA);
        values[0] = *(__m128 *) (src + 0);
        values[1] = *(__m128 *) (src + 16);
        values[2] = *(__m128 *) (src + 32);
        values[3] = *(__m128 *) (src + 48);
        _mm_stream_ps((float *) (dst + 0), values[0]);
        _mm_stream_ps((float *) (dst + 16), values[1]);
        _mm_stream_ps((float *) (dst + 32), values[2]);
        _mm_stream_ps((float *) (dst + 48), values[3]);
        src += 64;
        dst += 64;
    }

    if (len & 63)
        SDL_memcpy(dst, src, len & 63);
}
#endif /* __SSE__ */

#ifdef __MMX__
#ifdef _MSC_VER
#pragma warning(disable:4799)
#endif
/* This assumes 8-byte aligned src and dst */
static __inline__ void
SDL_memcpyMMX(Uint8 * dst, const Uint8 * src, int len)
{
    int i;

    __m64 values[8];
    for (i = len / 64; i--;) {
#ifdef __SSE__
        _mm_prefetch(src, _MM_HINT_NTA);
#endif
        values[0] = *(__m64 *) (src + 0);
        values[1] = *(__m64 *) (src + 8);
        values[2] = *(__m64 *) (src + 16);
        values[3] = *(__m64 *) (src + 24);
        values[4] = *(__m64 *) (src + 32);
        values[5] = *(__m64 *) (src + 40);
        values[6] = *(__m64 *) (src + 48);
        values[7] = *(__m64 *) (src + 56);
        _mm_stream_pi((__m64 *) (dst + 0), values[0]);
        _mm_stream_pi((__m64 *) (dst + 8), values[1]);
        _mm_stream_pi((__m64 *) (dst + 16), values[2]);
        _mm_stream_pi((__m64 *) (dst + 24), values[3]);
        _mm_stream_pi((__m64 *) (dst + 32), values[4]);
        _mm_stream_pi((__m64 *) (dst + 40), values[5]);
        _mm_stream_pi((__m64 *) (dst + 48), values[6]);
        _mm_stream_pi((__m64 *) (dst + 56), values[7]);
        src += 64;
        dst += 64;
    }

    if (len & 63)
        SDL_memcpy(dst, src, len & 63);
}
#endif /* __MMX__ */

void
SDL_BlitCopy(SDL_BlitInfo * info)
{
    SDL_bool overlap;
    Uint8 *src, *dst;
    int w, h;
    int srcskip, dstskip;

    w = info->dst_w * info->dst_fmt->BytesPerPixel;
    h = info->dst_h;
    src = info->src;
    dst = info->dst;
    srcskip = info->src_pitch;
    dstskip = info->dst_pitch;

    /* Properly handle overlapping blits */
    if (src < dst) {
        overlap = (dst < (src + h*srcskip));
    } else {
        overlap = (src < (dst + h*dstskip));
    }
    if (overlap) {
        while (h--) {
            SDL_memmove(dst, src, w);
            src += srcskip;
            dst += dstskip;
        }
        return;
    }

#ifdef __SSE__
    if (SDL_HasSSE() &&
        !((uintptr_t) src & 15) && !(srcskip & 15) &&
        !((uintptr_t) dst & 15) && !(dstskip & 15)) {
        while (h--) {
            SDL_memcpySSE(dst, src, w);
            src += srcskip;
            dst += dstskip;
        }
        return;
    }
#endif

#ifdef __MMX__
    if (SDL_HasMMX() &&
        !((uintptr_t) src & 7) && !(srcskip & 7) &&
        !((uintptr_t) dst & 7) && !(dstskip & 7)) {
        while (h--) {
            SDL_memcpyMMX(dst, src, w);
            src += srcskip;
            dst += dstskip;
        }
        _mm_empty();
        return;
    }
#endif

    while (h--) {
        SDL_memcpy(dst, src, w);
        src += srcskip;
        dst += dstskip;
    }
}

/* vi: set ts=4 sw=4 expandtab: */
