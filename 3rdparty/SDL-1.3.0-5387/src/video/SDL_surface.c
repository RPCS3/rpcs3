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
#include "SDL_compat.h"
#include "SDL_sysvideo.h"
#include "SDL_blit.h"
#include "SDL_RLEaccel_c.h"
#include "SDL_pixels_c.h"


/* Public routines */
/*
 * Create an empty RGB surface of the appropriate depth
 */
SDL_Surface *
SDL_CreateRGBSurface(Uint32 flags,
                     int width, int height, int depth,
                     Uint32 Rmask, Uint32 Gmask, Uint32 Bmask, Uint32 Amask)
{
    SDL_Surface *surface;
    Uint32 format;

    /* The flags are no longer used, make the compiler happy */
    (void)flags;

    /* Get the pixel format */
    format = SDL_MasksToPixelFormatEnum(depth, Rmask, Gmask, Bmask, Amask);
    if (format == SDL_PIXELFORMAT_UNKNOWN) {
        SDL_SetError("Unknown pixel format");
        return NULL;
    }

    /* Allocate the surface */
    surface = (SDL_Surface *) SDL_calloc(1, sizeof(*surface));
    if (surface == NULL) {
        SDL_OutOfMemory();
        return NULL;
    }

    surface->format = SDL_AllocFormat(format);
    if (!surface->format) {
        SDL_FreeSurface(surface);
        return NULL;
    }
    surface->w = width;
    surface->h = height;
    surface->pitch = SDL_CalculatePitch(surface);
    SDL_SetClipRect(surface, NULL);

    if (SDL_ISPIXELFORMAT_INDEXED(surface->format->format)) {
        SDL_Palette *palette =
            SDL_AllocPalette((1 << surface->format->BitsPerPixel));
        if (!palette) {
            SDL_FreeSurface(surface);
            return NULL;
        }
        if (Rmask || Bmask || Gmask) {
            const SDL_PixelFormat *format = surface->format;

            /* create palette according to masks */
            int i;
            int Rm = 0, Gm = 0, Bm = 0;
            int Rw = 0, Gw = 0, Bw = 0;

            if (Rmask) {
                Rw = 8 - format->Rloss;
                for (i = format->Rloss; i > 0; i -= Rw)
                    Rm |= 1 << i;
            }
            if (Gmask) {
                Gw = 8 - format->Gloss;
                for (i = format->Gloss; i > 0; i -= Gw)
                    Gm |= 1 << i;
            }
            if (Bmask) {
                Bw = 8 - format->Bloss;
                for (i = format->Bloss; i > 0; i -= Bw)
                    Bm |= 1 << i;
            }
            for (i = 0; i < palette->ncolors; ++i) {
                int r, g, b;
                r = (i & Rmask) >> format->Rshift;
                r = (r << format->Rloss) | ((r * Rm) >> Rw);
                palette->colors[i].r = r;

                g = (i & Gmask) >> format->Gshift;
                g = (g << format->Gloss) | ((g * Gm) >> Gw);
                palette->colors[i].g = g;

                b = (i & Bmask) >> format->Bshift;
                b = (b << format->Bloss) | ((b * Bm) >> Bw);
                palette->colors[i].b = b;
            }
        } else if (palette->ncolors == 2) {
            /* Create a black and white bitmap palette */
            palette->colors[0].r = 0xFF;
            palette->colors[0].g = 0xFF;
            palette->colors[0].b = 0xFF;
            palette->colors[1].r = 0x00;
            palette->colors[1].g = 0x00;
            palette->colors[1].b = 0x00;
        }
        SDL_SetSurfacePalette(surface, palette);
        SDL_FreePalette(palette);
    }

    /* Get the pixels */
    if (surface->w && surface->h) {
        surface->pixels = SDL_malloc(surface->h * surface->pitch);
        if (!surface->pixels) {
            SDL_FreeSurface(surface);
            SDL_OutOfMemory();
            return NULL;
        }
        /* This is important for bitmaps */
        SDL_memset(surface->pixels, 0, surface->h * surface->pitch);
    }

    /* Allocate an empty mapping */
    surface->map = SDL_AllocBlitMap();
    if (!surface->map) {
        SDL_FreeSurface(surface);
        return NULL;
    }

    /* By default surface with an alpha mask are set up for blending */
    if (Amask) {
        SDL_SetSurfaceBlendMode(surface, SDL_BLENDMODE_BLEND);
    }

    /* The surface is ready to go */
    surface->refcount = 1;
    return surface;
}

/*
 * Create an RGB surface from an existing memory buffer
 */
SDL_Surface *
SDL_CreateRGBSurfaceFrom(void *pixels,
                         int width, int height, int depth, int pitch,
                         Uint32 Rmask, Uint32 Gmask, Uint32 Bmask,
                         Uint32 Amask)
{
    SDL_Surface *surface;

    surface =
        SDL_CreateRGBSurface(0, 0, 0, depth, Rmask, Gmask, Bmask, Amask);
    if (surface != NULL) {
        surface->flags |= SDL_PREALLOC;
        surface->pixels = pixels;
        surface->w = width;
        surface->h = height;
        surface->pitch = pitch;
        SDL_SetClipRect(surface, NULL);
    }
    return surface;
}

int
SDL_SetSurfacePalette(SDL_Surface * surface, SDL_Palette * palette)
{
    if (!surface) {
        SDL_SetError("SDL_SetSurfacePalette() passed a NULL surface");
        return -1;
    }
    return SDL_SetPixelFormatPalette(surface->format, palette);
}

int
SDL_SetSurfaceRLE(SDL_Surface * surface, int flag)
{
    int flags;

    if (!surface) {
        return -1;
    }

    flags = surface->map->info.flags;
    if (flag) {
        surface->map->info.flags |= SDL_COPY_RLE_DESIRED;
    } else {
        surface->map->info.flags &= ~SDL_COPY_RLE_DESIRED;
    }
    if (surface->map->info.flags != flags) {
        SDL_InvalidateMap(surface->map);
    }
    return 0;
}

int
SDL_SetColorKey(SDL_Surface * surface, int flag, Uint32 key)
{
    int flags;

    if (!surface) {
        return -1;
    }

    if (flag & SDL_RLEACCEL) {
        SDL_SetSurfaceRLE(surface, 1);
    }

    flags = surface->map->info.flags;
    if (flag) {
        surface->map->info.flags |= SDL_COPY_COLORKEY;
        surface->map->info.colorkey = key;
    } else {
        surface->map->info.flags &= ~SDL_COPY_COLORKEY;
    }
    if (surface->map->info.flags != flags) {
        SDL_InvalidateMap(surface->map);
    }

    /* Compatibility mode */
    if (surface->map->info.flags & SDL_COPY_COLORKEY) {
        surface->flags |= SDL_SRCCOLORKEY;
    } else {
        surface->flags &= ~SDL_SRCCOLORKEY;
    }

    return 0;
}

int
SDL_GetColorKey(SDL_Surface * surface, Uint32 * key)
{
    if (!surface) {
        return -1;
    }

    if (!(surface->map->info.flags & SDL_COPY_COLORKEY)) {
        return -1;
    }

    if (key) {
        *key = surface->map->info.colorkey;
    }
    return 0;
}

/* This is a fairly slow function to switch from colorkey to alpha */
static void
SDL_ConvertColorkeyToAlpha(SDL_Surface * surface)
{
    int x, y;

    if (!surface) {
        return;
    }

    if (!(surface->map->info.flags & SDL_COPY_COLORKEY) ||
        !surface->format->Amask) {
        return;
    }

    SDL_LockSurface(surface);

    switch (surface->format->BytesPerPixel) {
    case 2:
        {
            Uint16 *row, *spot;
            Uint16 ckey = (Uint16) surface->map->info.colorkey;
            Uint16 mask = (Uint16) (~surface->format->Amask);

            row = (Uint16 *) surface->pixels;
            for (y = surface->h; y--;) {
                spot = row;
                for (x = surface->w; x--;) {
                    if (*spot == ckey) {
                        *spot &= mask;
                    }
                    ++spot;
                }
                row += surface->pitch / 2;
            }
        }
        break;
    case 3:
        /* FIXME */
        break;
    case 4:
        {
            Uint32 *row, *spot;
            Uint32 ckey = surface->map->info.colorkey;
            Uint32 mask = ~surface->format->Amask;

            row = (Uint32 *) surface->pixels;
            for (y = surface->h; y--;) {
                spot = row;
                for (x = surface->w; x--;) {
                    if (*spot == ckey) {
                        *spot &= mask;
                    }
                    ++spot;
                }
                row += surface->pitch / 4;
            }
        }
        break;
    }

    SDL_UnlockSurface(surface);

    SDL_SetColorKey(surface, 0, 0);
    SDL_SetSurfaceBlendMode(surface, SDL_BLENDMODE_BLEND);
}

int
SDL_SetSurfaceColorMod(SDL_Surface * surface, Uint8 r, Uint8 g, Uint8 b)
{
    int flags;

    if (!surface) {
        return -1;
    }

    surface->map->info.r = r;
    surface->map->info.g = g;
    surface->map->info.b = b;

    flags = surface->map->info.flags;
    if (r != 0xFF || g != 0xFF || b != 0xFF) {
        surface->map->info.flags |= SDL_COPY_MODULATE_COLOR;
    } else {
        surface->map->info.flags &= ~SDL_COPY_MODULATE_COLOR;
    }
    if (surface->map->info.flags != flags) {
        SDL_InvalidateMap(surface->map);
    }
    return 0;
}


int
SDL_GetSurfaceColorMod(SDL_Surface * surface, Uint8 * r, Uint8 * g, Uint8 * b)
{
    if (!surface) {
        return -1;
    }

    if (r) {
        *r = surface->map->info.r;
    }
    if (g) {
        *g = surface->map->info.g;
    }
    if (b) {
        *b = surface->map->info.b;
    }
    return 0;
}

int
SDL_SetSurfaceAlphaMod(SDL_Surface * surface, Uint8 alpha)
{
    int flags;

    if (!surface) {
        return -1;
    }

    surface->map->info.a = alpha;

    flags = surface->map->info.flags;
    if (alpha != 0xFF) {
        surface->map->info.flags |= SDL_COPY_MODULATE_ALPHA;
    } else {
        surface->map->info.flags &= ~SDL_COPY_MODULATE_ALPHA;
    }
    if (surface->map->info.flags != flags) {
        SDL_InvalidateMap(surface->map);
    }
    return 0;
}

int
SDL_GetSurfaceAlphaMod(SDL_Surface * surface, Uint8 * alpha)
{
    if (!surface) {
        return -1;
    }

    if (alpha) {
        *alpha = surface->map->info.a;
    }
    return 0;
}

int
SDL_SetSurfaceBlendMode(SDL_Surface * surface, SDL_BlendMode blendMode)
{
    int flags, status;

    if (!surface) {
        return -1;
    }

    status = 0;
    flags = surface->map->info.flags;
    surface->map->info.flags &=
        ~(SDL_COPY_BLEND | SDL_COPY_ADD | SDL_COPY_MOD);
    switch (blendMode) {
    case SDL_BLENDMODE_NONE:
        break;
    case SDL_BLENDMODE_BLEND:
        surface->map->info.flags |= SDL_COPY_BLEND;
        break;
    case SDL_BLENDMODE_ADD:
        surface->map->info.flags |= SDL_COPY_ADD;
        break;
    case SDL_BLENDMODE_MOD:
        surface->map->info.flags |= SDL_COPY_MOD;
        break;
    default:
        SDL_Unsupported();
        status = -1;
        break;
    }

    if (surface->map->info.flags != flags) {
        SDL_InvalidateMap(surface->map);
    }

    /* Compatibility mode */
    if (surface->map->info.flags & SDL_COPY_BLEND) {
        surface->flags |= SDL_SRCALPHA;
    } else {
        surface->flags &= ~SDL_SRCALPHA;
    }

    return status;
}

int
SDL_GetSurfaceBlendMode(SDL_Surface * surface, SDL_BlendMode *blendMode)
{
    if (!surface) {
        return -1;
    }

    if (!blendMode) {
        return 0;
    }

    switch (surface->map->
            info.flags & (SDL_COPY_BLEND | SDL_COPY_ADD | SDL_COPY_MOD)) {
    case SDL_COPY_BLEND:
        *blendMode = SDL_BLENDMODE_BLEND;
        break;
    case SDL_COPY_ADD:
        *blendMode = SDL_BLENDMODE_ADD;
        break;
    case SDL_COPY_MOD:
        *blendMode = SDL_BLENDMODE_MOD;
        break;
    default:
        *blendMode = SDL_BLENDMODE_NONE;
        break;
    }
    return 0;
}

SDL_bool
SDL_SetClipRect(SDL_Surface * surface, const SDL_Rect * rect)
{
    SDL_Rect full_rect;

    /* Don't do anything if there's no surface to act on */
    if (!surface) {
        return SDL_FALSE;
    }

    /* Set up the full surface rectangle */
    full_rect.x = 0;
    full_rect.y = 0;
    full_rect.w = surface->w;
    full_rect.h = surface->h;

    /* Set the clipping rectangle */
    if (!rect) {
        surface->clip_rect = full_rect;
        return SDL_TRUE;
    }
    return SDL_IntersectRect(rect, &full_rect, &surface->clip_rect);
}

void
SDL_GetClipRect(SDL_Surface * surface, SDL_Rect * rect)
{
    if (surface && rect) {
        *rect = surface->clip_rect;
    }
}

/* 
 * Set up a blit between two surfaces -- split into three parts:
 * The upper part, SDL_UpperBlit(), performs clipping and rectangle 
 * verification.  The lower part is a pointer to a low level
 * accelerated blitting function.
 *
 * These parts are separated out and each used internally by this 
 * library in the optimimum places.  They are exported so that if
 * you know exactly what you are doing, you can optimize your code
 * by calling the one(s) you need.
 */
int
SDL_LowerBlit(SDL_Surface * src, SDL_Rect * srcrect,
              SDL_Surface * dst, SDL_Rect * dstrect)
{
    /* Check to make sure the blit mapping is valid */
    if ((src->map->dst != dst) ||
        (dst->format->palette &&
         src->map->palette_version != dst->format->palette->version)) {
        if (SDL_MapSurface(src, dst) < 0) {
            return (-1);
        }
        /* just here for debugging */
/*         printf */
/*             ("src = 0x%08X src->flags = %08X src->map->info.flags = %08x\ndst = 0x%08X dst->flags = %08X dst->map->info.flags = %08X\nsrc->map->blit = 0x%08x\n", */
/*              src, dst->flags, src->map->info.flags, dst, dst->flags, */
/*              dst->map->info.flags, src->map->blit); */
    }
    return (src->map->blit(src, srcrect, dst, dstrect));
}


int
SDL_UpperBlit(SDL_Surface * src, const SDL_Rect * srcrect,
              SDL_Surface * dst, SDL_Rect * dstrect)
{
    SDL_Rect fulldst;
    int srcx, srcy, w, h;

    /* Make sure the surfaces aren't locked */
    if (!src || !dst) {
        SDL_SetError("SDL_UpperBlit: passed a NULL surface");
        return (-1);
    }
    if (src->locked || dst->locked) {
        SDL_SetError("Surfaces must not be locked during blit");
        return (-1);
    }

    /* If the destination rectangle is NULL, use the entire dest surface */
    if (dstrect == NULL) {
        fulldst.x = fulldst.y = 0;
        dstrect = &fulldst;
    }

    /* clip the source rectangle to the source surface */
    if (srcrect) {
        int maxw, maxh;

        srcx = srcrect->x;
        w = srcrect->w;
        if (srcx < 0) {
            w += srcx;
            dstrect->x -= srcx;
            srcx = 0;
        }
        maxw = src->w - srcx;
        if (maxw < w)
            w = maxw;

        srcy = srcrect->y;
        h = srcrect->h;
        if (srcy < 0) {
            h += srcy;
            dstrect->y -= srcy;
            srcy = 0;
        }
        maxh = src->h - srcy;
        if (maxh < h)
            h = maxh;

    } else {
        srcx = srcy = 0;
        w = src->w;
        h = src->h;
    }

    /* clip the destination rectangle against the clip rectangle */
    {
        SDL_Rect *clip = &dst->clip_rect;
        int dx, dy;

        dx = clip->x - dstrect->x;
        if (dx > 0) {
            w -= dx;
            dstrect->x += dx;
            srcx += dx;
        }
        dx = dstrect->x + w - clip->x - clip->w;
        if (dx > 0)
            w -= dx;

        dy = clip->y - dstrect->y;
        if (dy > 0) {
            h -= dy;
            dstrect->y += dy;
            srcy += dy;
        }
        dy = dstrect->y + h - clip->y - clip->h;
        if (dy > 0)
            h -= dy;
    }

    if (w > 0 && h > 0) {
        SDL_Rect sr;
        sr.x = srcx;
        sr.y = srcy;
        sr.w = dstrect->w = w;
        sr.h = dstrect->h = h;
        return SDL_LowerBlit(src, &sr, dst, dstrect);
    }
    dstrect->w = dstrect->h = 0;
    return 0;
}

/*
 * Scale and blit a surface 
*/
int
SDL_BlitScaled(SDL_Surface * src, const SDL_Rect * srcrect,
               SDL_Surface * dst, const SDL_Rect * dstrect)
{
    /* Save off the original dst width, height */
    int dstW = dstrect->w;
    int dstH = dstrect->h;
    SDL_Rect final_dst = *dstrect;
    SDL_Rect final_src = *srcrect;

    /* Clip the dst surface to the dstrect */
    SDL_SetClipRect( dst, &final_dst );

    /* If the dest was clipped to a zero sized rect then exit */
    if ( dst->clip_rect.w <= 0 || dst->clip_rect.h <= 0 ) {
        return -1;
    }

    /* Did the dst width change? */
    if ( dstW != dst->clip_rect.w ) {
        /* scale the src width appropriately */
        final_src.w = final_src.w * dst->clip_rect.w / dstW;
    }

    /* Did the dst height change? */
    if ( dstH != dst->clip_rect.h ) {
        /* scale the src width appropriately */
        final_src.h = final_src.h * dst->clip_rect.h / dstH;
    }

    /* Clip the src surface to the srcrect */
    SDL_SetClipRect( src, &final_src );

    src->map->info.flags |= SDL_COPY_NEAREST;

    if ( src->format->format == dst->format->format && !SDL_ISPIXELFORMAT_INDEXED(src->format->format) ) {
        return SDL_SoftStretch( src, &final_src, dst, &final_dst );
    } else {
        return SDL_LowerBlit( src, &final_src, dst, &final_dst );
    }
}

/*
 * Lock a surface to directly access the pixels
 */
int
SDL_LockSurface(SDL_Surface * surface)
{
    if (!surface->locked) {
        /* Perform the lock */
        if (surface->flags & SDL_RLEACCEL) {
            SDL_UnRLESurface(surface, 1);
            surface->flags |= SDL_RLEACCEL;     /* save accel'd state */
        }
    }

    /* Increment the surface lock count, for recursive locks */
    ++surface->locked;

    /* Ready to go.. */
    return (0);
}

/*
 * Unlock a previously locked surface
 */
void
SDL_UnlockSurface(SDL_Surface * surface)
{
    /* Only perform an unlock if we are locked */
    if (!surface->locked || (--surface->locked > 0)) {
        return;
    }

    /* Update RLE encoded surface with new data */
    if ((surface->flags & SDL_RLEACCEL) == SDL_RLEACCEL) {
        surface->flags &= ~SDL_RLEACCEL;        /* stop lying */
        SDL_RLESurface(surface);
    }
}

/* 
 * Convert a surface into the specified pixel format.
 */
SDL_Surface *
SDL_ConvertSurface(SDL_Surface * surface, SDL_PixelFormat * format,
                   Uint32 flags)
{
    SDL_Surface *convert;
    Uint32 copy_flags;
    SDL_Rect bounds;

    /* Check for empty destination palette! (results in empty image) */
    if (format->palette != NULL) {
        int i;
        for (i = 0; i < format->palette->ncolors; ++i) {
            if ((format->palette->colors[i].r != 0xFF) ||
                (format->palette->colors[i].g != 0xFF) ||
                (format->palette->colors[i].b != 0xFF))
                break;
        }
        if (i == format->palette->ncolors) {
            SDL_SetError("Empty destination palette");
            return (NULL);
        }
    }

    /* Create a new surface with the desired format */
    convert = SDL_CreateRGBSurface(flags, surface->w, surface->h,
                                   format->BitsPerPixel, format->Rmask,
                                   format->Gmask, format->Bmask,
                                   format->Amask);
    if (convert == NULL) {
        return (NULL);
    }

    /* Copy the palette if any */
    if (format->palette && convert->format->palette) {
        SDL_memcpy(convert->format->palette->colors,
                   format->palette->colors,
                   format->palette->ncolors * sizeof(SDL_Color));
        convert->format->palette->ncolors = format->palette->ncolors;
    }

    /* Save the original copy flags */
    copy_flags = surface->map->info.flags;
    surface->map->info.flags = 0;

    /* Copy over the image data */
    bounds.x = 0;
    bounds.y = 0;
    bounds.w = surface->w;
    bounds.h = surface->h;
    SDL_LowerBlit(surface, &bounds, convert, &bounds);

    /* Clean up the original surface, and update converted surface */
    convert->map->info.r = surface->map->info.r;
    convert->map->info.g = surface->map->info.g;
    convert->map->info.b = surface->map->info.b;
    convert->map->info.a = surface->map->info.a;
    convert->map->info.flags =
        (copy_flags &
         ~(SDL_COPY_COLORKEY | SDL_COPY_BLEND
           | SDL_COPY_RLE_DESIRED | SDL_COPY_RLE_COLORKEY |
           SDL_COPY_RLE_ALPHAKEY));
    surface->map->info.flags = copy_flags;
    if (copy_flags & SDL_COPY_COLORKEY) {
        Uint8 keyR, keyG, keyB, keyA;

        SDL_GetRGBA(surface->map->info.colorkey, surface->format, &keyR,
                    &keyG, &keyB, &keyA);
        SDL_SetColorKey(convert, 1,
                        SDL_MapRGBA(convert->format, keyR, keyG, keyB, keyA));
        /* This is needed when converting for 3D texture upload */
        SDL_ConvertColorkeyToAlpha(convert);
    }
    SDL_SetClipRect(convert, &surface->clip_rect);

    /* Enable alpha blending by default if the new surface has an
     * alpha channel or alpha modulation */
    if ((surface->format->Amask && format->Amask) ||
        (copy_flags & (SDL_COPY_COLORKEY|SDL_COPY_MODULATE_ALPHA))) {
        SDL_SetSurfaceBlendMode(convert, SDL_BLENDMODE_BLEND);
    }
    if ((copy_flags & SDL_COPY_RLE_DESIRED) || (flags & SDL_RLEACCEL)) {
        SDL_SetSurfaceRLE(convert, SDL_RLEACCEL);
    }

    /* We're ready to go! */
    return (convert);
}

SDL_Surface *
SDL_ConvertSurfaceFormat(SDL_Surface * surface, Uint32 pixel_format,
                         Uint32 flags)
{
    SDL_PixelFormat *fmt;
    SDL_Surface *convert;

    fmt = SDL_AllocFormat(pixel_format);
    if (fmt) {
        convert = SDL_ConvertSurface(surface, fmt, flags);
        SDL_FreeFormat(fmt);
    }
    return convert;
}

/*
 * Create a surface on the stack for quick blit operations
 */
static __inline__ SDL_bool
SDL_CreateSurfaceOnStack(int width, int height, Uint32 pixel_format,
                         void * pixels, int pitch, SDL_Surface * surface, 
                         SDL_PixelFormat * format, SDL_BlitMap * blitmap)
{
    if (SDL_ISPIXELFORMAT_INDEXED(pixel_format)) {
        SDL_SetError("Indexed pixel formats not supported");
        return SDL_FALSE;
    }
    if (SDL_InitFormat(format, pixel_format) < 0) {
        return SDL_FALSE;
    }

    SDL_zerop(surface);
    surface->flags = SDL_PREALLOC;
    surface->format = format;
    surface->pixels = pixels;
    surface->w = width;
    surface->h = height;
    surface->pitch = pitch;
    /* We don't actually need to set up the clip rect for our purposes */
    /*SDL_SetClipRect(surface, NULL);*/

    /* Allocate an empty mapping */
    SDL_zerop(blitmap);
    blitmap->info.r = 0xFF;
    blitmap->info.g = 0xFF;
    blitmap->info.b = 0xFF;
    blitmap->info.a = 0xFF;
    surface->map = blitmap;

    /* The surface is ready to go */
    surface->refcount = 1;
    return SDL_TRUE;
}

/*
 * Copy a block of pixels of one format to another format
 */
int SDL_ConvertPixels(int width, int height,
                      Uint32 src_format, const void * src, int src_pitch,
                      Uint32 dst_format, void * dst, int dst_pitch)
{
    SDL_Surface src_surface, dst_surface;
    SDL_PixelFormat src_fmt, dst_fmt;
    SDL_BlitMap src_blitmap, dst_blitmap;
    SDL_Rect rect;

    /* Fast path for same format copy */
    if (src_format == dst_format) {
        int bpp;

        if (SDL_ISPIXELFORMAT_FOURCC(src_format)) {
            switch (src_format) {
            case SDL_PIXELFORMAT_YV12:
            case SDL_PIXELFORMAT_IYUV:
            case SDL_PIXELFORMAT_YUY2:
            case SDL_PIXELFORMAT_UYVY:
            case SDL_PIXELFORMAT_YVYU:
                bpp = 2;
            default:
                SDL_SetError("Unknown FOURCC pixel format");
                return -1;
            }
        } else {
            bpp = SDL_BYTESPERPIXEL(src_format);
        }
        width *= bpp;

        while (height-- > 0) {
            SDL_memcpy(dst, src, width);
            src = (Uint8*)src + src_pitch;
            dst = (Uint8*)dst + dst_pitch;
        }
        return SDL_TRUE;
    }

    if (!SDL_CreateSurfaceOnStack(width, height, src_format, (void*)src,
                                  src_pitch,
                                  &src_surface, &src_fmt, &src_blitmap)) {
        return -1;
    }
    if (!SDL_CreateSurfaceOnStack(width, height, dst_format, dst, dst_pitch,
                                  &dst_surface, &dst_fmt, &dst_blitmap)) {
        return -1;
    }

    /* Set up the rect and go! */
    rect.x = 0;
    rect.y = 0;
    rect.w = width;
    rect.h = height;
    return SDL_LowerBlit(&src_surface, &rect, &dst_surface, &rect);
}

/*
 * Free a surface created by the above function.
 */
void
SDL_FreeSurface(SDL_Surface * surface)
{
    if (surface == NULL) {
        return;
    }
    if (surface->flags & SDL_DONTFREE) {
        return;
    }
    if (--surface->refcount > 0) {
        return;
    }
    while (surface->locked > 0) {
        SDL_UnlockSurface(surface);
    }
    if (surface->flags & SDL_RLEACCEL) {
        SDL_UnRLESurface(surface, 0);
    }
    if (surface->format) {
        SDL_SetSurfacePalette(surface, NULL);
        SDL_FreeFormat(surface->format);
        surface->format = NULL;
    }
    if (surface->map != NULL) {
        SDL_FreeBlitMap(surface->map);
        surface->map = NULL;
    }
    if (surface->pixels && ((surface->flags & SDL_PREALLOC) != SDL_PREALLOC)) {
        SDL_free(surface->pixels);
    }
    SDL_free(surface);
}

/* vi: set ts=4 sw=4 expandtab: */
