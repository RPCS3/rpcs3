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
#include "SDL_blit_slow.h"

/* The ONE TRUE BLITTER
 * This puppy has to handle all the unoptimized cases - yes, it's slow.
 */
void
SDL_Blit_Slow(SDL_BlitInfo * info)
{
    const int flags = info->flags;
    const Uint32 modulateR = info->r;
    const Uint32 modulateG = info->g;
    const Uint32 modulateB = info->b;
    const Uint32 modulateA = info->a;
    Uint32 srcpixel;
    Uint32 srcR, srcG, srcB, srcA;
    Uint32 dstpixel;
    Uint32 dstR, dstG, dstB, dstA;
    int srcy, srcx;
    int posy, posx;
    int incy, incx;
    SDL_PixelFormat *src_fmt = info->src_fmt;
    SDL_PixelFormat *dst_fmt = info->dst_fmt;
    int srcbpp = src_fmt->BytesPerPixel;
    int dstbpp = dst_fmt->BytesPerPixel;

    srcy = 0;
    posy = 0;
    incy = (info->src_h << 16) / info->dst_h;
    incx = (info->src_w << 16) / info->dst_w;

    while (info->dst_h--) {
        Uint8 *src;
        Uint8 *dst = (Uint8 *) info->dst;
        int n = info->dst_w;
        srcx = -1;
        posx = 0x10000L;
        while (posy >= 0x10000L) {
            ++srcy;
            posy -= 0x10000L;
        }
        while (n--) {
            if (posx >= 0x10000L) {
                while (posx >= 0x10000L) {
                    ++srcx;
                    posx -= 0x10000L;
                }
                src =
                    (info->src + (srcy * info->src_pitch) + (srcx * srcbpp));
            }
            if (src_fmt->Amask) {
                DISEMBLE_RGBA(src, srcbpp, src_fmt, srcpixel, srcR, srcG,
                              srcB, srcA);
            } else {
                DISEMBLE_RGB(src, srcbpp, src_fmt, srcpixel, srcR, srcG,
                             srcB);
                srcA = 0xFF;
            }
            if (flags & SDL_COPY_COLORKEY) {
                /* srcpixel isn't set for 24 bpp */
                if (srcbpp == 3) {
                    srcpixel = (srcR << src_fmt->Rshift) |
                        (srcG << src_fmt->Gshift) | (srcB << src_fmt->Bshift);
                }
                if (srcpixel == info->colorkey) {
                    posx += incx;
                    dst += dstbpp;
                    continue;
                }
            }
            if (dst_fmt->Amask) {
                DISEMBLE_RGBA(dst, dstbpp, dst_fmt, dstpixel, dstR, dstG,
                              dstB, dstA);
            } else {
                DISEMBLE_RGB(dst, dstbpp, dst_fmt, dstpixel, dstR, dstG,
                             dstB);
                dstA = 0xFF;
            }

            if (flags & SDL_COPY_MODULATE_COLOR) {
                srcR = (srcR * modulateR) / 255;
                srcG = (srcG * modulateG) / 255;
                srcB = (srcB * modulateB) / 255;
            }
            if (flags & SDL_COPY_MODULATE_ALPHA) {
                srcA = (srcA * modulateA) / 255;
            }
            if (flags & (SDL_COPY_BLEND | SDL_COPY_ADD)) {
                /* This goes away if we ever use premultiplied alpha */
                if (srcA < 255) {
                    srcR = (srcR * srcA) / 255;
                    srcG = (srcG * srcA) / 255;
                    srcB = (srcB * srcA) / 255;
                }
            }
            switch (flags & (SDL_COPY_BLEND | SDL_COPY_ADD | SDL_COPY_MOD)) {
            case 0:
                dstR = srcR;
                dstG = srcG;
                dstB = srcB;
                dstA = srcA;
                break;
            case SDL_COPY_BLEND:
                dstR = srcR + ((255 - srcA) * dstR) / 255;
                dstG = srcG + ((255 - srcA) * dstG) / 255;
                dstB = srcB + ((255 - srcA) * dstB) / 255;
                break;
            case SDL_COPY_ADD:
                dstR = srcR + dstR;
                if (dstR > 255)
                    dstR = 255;
                dstG = srcG + dstG;
                if (dstG > 255)
                    dstG = 255;
                dstB = srcB + dstB;
                if (dstB > 255)
                    dstB = 255;
                break;
            case SDL_COPY_MOD:
                dstR = (srcR * dstR) / 255;
                dstG = (srcG * dstG) / 255;
                dstB = (srcB * dstB) / 255;
                break;
            }
            if (dst_fmt->Amask) {
                ASSEMBLE_RGBA(dst, dstbpp, dst_fmt, dstR, dstG, dstB, dstA);
            } else {
                ASSEMBLE_RGB(dst, dstbpp, dst_fmt, dstR, dstG, dstB);
            }
            posx += incx;
            dst += dstbpp;
        }
        posy += incy;
        info->dst += info->dst_pitch;
    }
}

/* vi: set ts=4 sw=4 expandtab: */
