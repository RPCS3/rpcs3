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
#include "SDL_sysvideo.h"
#include "SDL_blit.h"
#include "SDL_blit_auto.h"
#include "SDL_blit_copy.h"
#include "SDL_blit_slow.h"
#include "SDL_RLEaccel_c.h"
#include "SDL_pixels_c.h"

/* The general purpose software blit routine */
static int
SDL_SoftBlit(SDL_Surface * src, SDL_Rect * srcrect,
             SDL_Surface * dst, SDL_Rect * dstrect)
{
    int okay;
    int src_locked;
    int dst_locked;

    /* Everything is okay at the beginning...  */
    okay = 1;

    /* Lock the destination if it's in hardware */
    dst_locked = 0;
    if (SDL_MUSTLOCK(dst)) {
        if (SDL_LockSurface(dst) < 0) {
            okay = 0;
        } else {
            dst_locked = 1;
        }
    }
    /* Lock the source if it's in hardware */
    src_locked = 0;
    if (SDL_MUSTLOCK(src)) {
        if (SDL_LockSurface(src) < 0) {
            okay = 0;
        } else {
            src_locked = 1;
        }
    }

    /* Set up source and destination buffer pointers, and BLIT! */
    if (okay && srcrect->w && srcrect->h) {
        SDL_BlitFunc RunBlit;
        SDL_BlitInfo *info = &src->map->info;

        /* Set up the blit information */
        info->src = (Uint8 *) src->pixels +
            (Uint16) srcrect->y * src->pitch +
            (Uint16) srcrect->x * info->src_fmt->BytesPerPixel;
        info->src_w = srcrect->w;
        info->src_h = srcrect->h;
        info->src_pitch = src->pitch;
        info->src_skip =
            info->src_pitch - info->src_w * info->src_fmt->BytesPerPixel;
        info->dst =
            (Uint8 *) dst->pixels + (Uint16) dstrect->y * dst->pitch +
            (Uint16) dstrect->x * info->dst_fmt->BytesPerPixel;
        info->dst_w = dstrect->w;
        info->dst_h = dstrect->h;
        info->dst_pitch = dst->pitch;
        info->dst_skip =
            info->dst_pitch - info->dst_w * info->dst_fmt->BytesPerPixel;
        RunBlit = (SDL_BlitFunc) src->map->data;

        /* Run the actual software blit */
        RunBlit(info);
    }

    /* We need to unlock the surfaces if they're locked */
    if (dst_locked) {
        SDL_UnlockSurface(dst);
    }
    if (src_locked) {
        SDL_UnlockSurface(src);
    }
    /* Blit is done! */
    return (okay ? 0 : -1);
}


static SDL_BlitFunc
SDL_ChooseBlitFunc(Uint32 src_format, Uint32 dst_format, int flags,
                   SDL_BlitFuncEntry * entries)
{
    int i, flagcheck;
    static Uint32 features = 0xffffffff;

    /* Get the available CPU features */
    if (features == 0xffffffff) {
        const char *override = SDL_getenv("SDL_BLIT_CPU_FEATURES");

        features = SDL_CPU_ANY;

        /* Allow an override for testing .. */
        if (override) {
            SDL_sscanf(override, "%u", &features);
        } else {
            if (SDL_HasMMX()) {
                features |= SDL_CPU_MMX;
            }
            if (SDL_HasSSE()) {
                features |= SDL_CPU_SSE;
            }
            if (SDL_HasSSE2()) {
                features |= SDL_CPU_SSE2;
            }
        }
    }

    for (i = 0; entries[i].func; ++i) {
        /* Check for matching pixel formats */
        if (src_format != entries[i].src_format) {
            continue;
        }
        if (dst_format != entries[i].dst_format) {
            continue;
        }

        /* Check modulation flags */
        flagcheck =
            (flags & (SDL_COPY_MODULATE_COLOR | SDL_COPY_MODULATE_ALPHA));
        if ((flagcheck & entries[i].flags) != flagcheck) {
            continue;
        }

        /* Check blend flags */
        flagcheck =
            (flags &
             (SDL_COPY_BLEND | SDL_COPY_ADD | SDL_COPY_MOD));
        if ((flagcheck & entries[i].flags) != flagcheck) {
            continue;
        }

        /* Check colorkey flag */
        flagcheck = (flags & SDL_COPY_COLORKEY);
        if ((flagcheck & entries[i].flags) != flagcheck) {
            continue;
        }

        /* Check scaling flags */
        flagcheck = (flags & SDL_COPY_NEAREST);
        if ((flagcheck & entries[i].flags) != flagcheck) {
            continue;
        }

        /* Check CPU features */
        flagcheck = entries[i].cpu;
        if ((flagcheck & features) != flagcheck) {
            continue;
        }

        /* We found the best one! */
        return entries[i].func;
    }
    return NULL;
}

/* Figure out which of many blit routines to set up on a surface */
int
SDL_CalculateBlit(SDL_Surface * surface)
{
    SDL_BlitFunc blit = NULL;
    SDL_BlitMap *map = surface->map;
    SDL_Surface *dst = map->dst;

    /* Clean everything out to start */
    if ((surface->flags & SDL_RLEACCEL) == SDL_RLEACCEL) {
        SDL_UnRLESurface(surface, 1);
    }
    map->blit = SDL_SoftBlit;
    map->info.src_fmt = surface->format;
    map->info.src_pitch = surface->pitch;
    map->info.dst_fmt = dst->format;
    map->info.dst_pitch = dst->pitch;

    /* See if we can do RLE acceleration */
    if (map->info.flags & SDL_COPY_RLE_DESIRED) {
        if (SDL_RLESurface(surface) == 0) {
            return 0;
        }
    }

    /* Choose a standard blit function */
    if (map->identity && !(map->info.flags & ~SDL_COPY_RLE_DESIRED)) {
        blit = SDL_BlitCopy;
    } else if (surface->format->BitsPerPixel < 8) {
        blit = SDL_CalculateBlit0(surface);
    } else if (surface->format->BytesPerPixel == 1) {
        blit = SDL_CalculateBlit1(surface);
    } else if (map->info.flags & SDL_COPY_BLEND) {
        blit = SDL_CalculateBlitA(surface);
    } else {
        blit = SDL_CalculateBlitN(surface);
    }
    if (blit == NULL) {
        Uint32 src_format = surface->format->format;
        Uint32 dst_format = dst->format->format;

        blit =
            SDL_ChooseBlitFunc(src_format, dst_format, map->info.flags,
                               SDL_GeneratedBlitFuncTable);
    }
#ifndef TEST_SLOW_BLIT
    if (blit == NULL)
#endif
    {
        if (surface->format->BytesPerPixel > 1
            && dst->format->BytesPerPixel > 1) {
            blit = SDL_Blit_Slow;
        }
    }
    map->data = blit;

    /* Make sure we have a blit function */
    if (blit == NULL) {
        SDL_InvalidateMap(map);
        SDL_SetError("Blit combination not supported");
        return (-1);
    }

    return (0);
}

/* vi: set ts=4 sw=4 expandtab: */
