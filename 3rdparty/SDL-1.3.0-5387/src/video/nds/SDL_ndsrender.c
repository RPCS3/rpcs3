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

#include <stdio.h>
#include <stdlib.h>
#include <nds.h>
//#include <nds/arm9/video.h>
//#include <nds/arm9/sprite.h>
//#include <nds/arm9/trig_lut.h>

#include "SDL_config.h"

#include "SDL_video.h"
#include "../SDL_sysvideo.h"
#include "SDL_render.h"
#include "../../render/SDL_sysrender.h"

/* SDL NDS renderer implementation */

static SDL_Renderer *NDS_CreateRenderer(SDL_Window * window, Uint32 flags);
static int NDS_ActivateRenderer(SDL_Renderer * renderer);
static int NDS_DisplayModeChanged(SDL_Renderer * renderer);
static int NDS_CreateTexture(SDL_Renderer * renderer, SDL_Texture * texture);
#if 0
static int NDS_QueryTexturePixels(SDL_Renderer * renderer,
                                  SDL_Texture * texture, void **pixels,
                                  int *pitch);
#endif
static int NDS_UpdateTexture(SDL_Renderer * renderer, SDL_Texture * texture,
                             const SDL_Rect * rect, const void *pixels,
                             int pitch);
static int NDS_LockTexture(SDL_Renderer * renderer, SDL_Texture * texture,
                           const SDL_Rect * rect, int markDirty,
                           void **pixels, int *pitch);
static void NDS_UnlockTexture(SDL_Renderer * renderer, SDL_Texture * texture);
static int NDS_RenderFillRects(SDL_Renderer * renderer, const SDL_Rect ** rects,
							   int count);
static int NDS_RenderCopy(SDL_Renderer * renderer,
                          SDL_Texture * texture,
                          const SDL_Rect * srcrect, const SDL_Rect * dstrect);
static void NDS_RenderPresent(SDL_Renderer * renderer);
static void NDS_DestroyTexture(SDL_Renderer * renderer,
                               SDL_Texture * texture);
static void NDS_DestroyRenderer(SDL_Renderer * renderer);


SDL_RenderDriver NDS_RenderDriver = {
    NDS_CreateRenderer,
    {"nds",                     /* char* name */
     (SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC),  /* u32 flags */
     2,                         /* u32 num_texture_formats */
     {
      SDL_PIXELFORMAT_ABGR1555,
      SDL_PIXELFORMAT_BGR555,
      },                        /* u32 texture_formats[20] */
     (256),                     /* int max_texture_width */
     (256),                     /* int max_texture_height */
     }
};

typedef struct
{
    u8 bg_taken[4];
    OamState *oam;
    int sub;
} NDS_RenderData;

typedef struct
{
    enum
    { NDSTX_BG, NDSTX_SPR } type;       /* represented in a bg or sprite. */
    int hw_index;               /* index of sprite in OAM or bg from libnds */
    int pitch, bpp;             /* useful information about the texture */
    struct
    {
        int x, y;
    } scale;                    /* x/y stretch (24.8 fixed point) */
    struct
    {
        int x, y;
    } scroll;                   /* x/y offset */
    int rotate;                 /* -32768 to 32767, texture rotation */
    u16 *vram_pixels;           /* where the pixel data is stored (a pointer into VRAM) */
    u16 *vram_palette;          /* where the palette data is stored if it's indexed. */
    /*int size; */
} NDS_TextureData;



SDL_Renderer *
NDS_CreateRenderer(SDL_Window * window, Uint32 flags)
{
    SDL_VideoDisplay *display = SDL_GetDisplayForWindow(window);
    SDL_DisplayMode *displayMode = &display->current_mode;
    SDL_Renderer *renderer;
    NDS_RenderData *data;
    int i, n;
    int bpp;
    Uint32 Rmask, Gmask, Bmask, Amask;

    if (!SDL_PixelFormatEnumToMasks(displayMode->format, &bpp,
                                    &Rmask, &Gmask, &Bmask, &Amask)) {
        SDL_SetError("Unknown display format");
        return NULL;
    }
    switch (displayMode->format) {
    case SDL_PIXELFORMAT_ABGR1555:
    case SDL_PIXELFORMAT_BGR555:
        /* okay */
        break;
    case SDL_PIXELFORMAT_RGB555:
    case SDL_PIXELFORMAT_RGB565:
    case SDL_PIXELFORMAT_ARGB1555:
        /* we'll take these too for now */
        break;
    default:
        SDL_SetError("Warning: wrong display format for NDS!\n");
        break;
    }

    renderer = (SDL_Renderer *) SDL_calloc(1, sizeof(*renderer));
    if (!renderer) {
        SDL_OutOfMemory();
        return NULL;
    }

    data = (NDS_RenderData *) SDL_malloc(sizeof(*data));
    if (!data) {
        NDS_DestroyRenderer(renderer);
        SDL_OutOfMemory();
        return NULL;
    }
    SDL_zerop(data);

    renderer->RenderFillRects = NDS_RenderFillRects;
    renderer->RenderCopy = NDS_RenderCopy;
    renderer->RenderPresent = NDS_RenderPresent;
    renderer->DestroyRenderer = NDS_DestroyRenderer;
    renderer->info.name = NDS_RenderDriver.info.name;
    renderer->info.flags = 0;
    renderer->window = window;
    renderer->driverdata = data;
    renderer->CreateTexture = NDS_CreateTexture;
//  renderer->QueryTexturePixels = NDS_QueryTexturePixels;
    renderer->UpdateTexture = NDS_UpdateTexture;
    renderer->LockTexture = NDS_LockTexture;
    renderer->UnlockTexture = NDS_UnlockTexture;
    renderer->DestroyTexture = NDS_DestroyTexture;

    renderer->info.num_texture_formats =
        NDS_RenderDriver.info.num_texture_formats;
    SDL_memcpy(renderer->info.texture_formats,
               NDS_RenderDriver.info.texture_formats,
               sizeof(renderer->info.texture_formats));
    renderer->info.max_texture_width =
        NDS_RenderDriver.info.max_texture_width;
    renderer->info.max_texture_height =
        NDS_RenderDriver.info.max_texture_height;

    data->sub = 0;              /* TODO: this is hard-coded to the "main" screen.
                                   figure out how to detect whether to set it to
                                   "sub" screen.  window->id, perhaps? */
    data->bg_taken[2] = data->bg_taken[3] = 0;

    return renderer;
}

static int
NDS_ActivateRenderer(SDL_Renderer * renderer)
{
    NDS_RenderData *data = (NDS_RenderData *) renderer->driverdata;

    return 0;
}

static int
NDS_DisplayModeChanged(SDL_Renderer * renderer)
{
    NDS_RenderData *data = (NDS_RenderData *) renderer->driverdata;

    return 0;
}

static int
NDS_CreateTexture(SDL_Renderer * renderer, SDL_Texture * texture)
{
    NDS_RenderData *data = (NDS_RenderData *) renderer->driverdata;
    NDS_TextureData *txdat = NULL;
    int i;
    int bpp;
    Uint32 Rmask, Gmask, Bmask, Amask;

    if (!SDL_PixelFormatEnumToMasks
        (texture->format, &bpp, &Rmask, &Gmask, &Bmask, &Amask)) {
        SDL_SetError("Unknown texture format");
        return -1;
    }

    /* conditional statements on w/h to place it as bg/sprite
       depending on which one it fits. */
    if (texture->w <= 64 && texture->h <= 64) {
        int whichspr = -1;
        printf("NDS_CreateTexture: Tried to make a sprite.\n");
        txdat->type = NDSTX_SPR;
#if 0
        for (i = 0; i < SPRITE_COUNT; ++i) {
            if (data->oam_copy.spriteBuffer[i].attribute[0] & ATTR0_DISABLED) {
                whichspr = i;
                break;
            }
        }
        if (whichspr >= 0) {
            SpriteEntry *sprent = &(data->oam_copy.spriteBuffer[whichspr]);
            int maxside = texture->w > texture->h ? texture->w : texture->h;
            int pitch;

            texture->driverdata = SDL_calloc(1, sizeof(NDS_TextureData));
            txdat = (NDS_TextureData *) texture->driverdata;
            if (!txdat) {
                SDL_OutOfMemory();
                return -1;
            }

            sprent->objMode = OBJMODE_BITMAP;
            sprent->posX = 0;
            sprent->posY = 0;
            sprent->colMode = OBJCOLOR_16;      /* OBJCOLOR_256 for INDEX8 */

            /* the first 32 sprites get transformation matrices.
               first come, first served */
            if (whichspr < MATRIX_COUNT) {
                sprent->isRotoscale = 1;
                sprent->rsMatrixIdx = whichspr;
            }

            /* containing shape (square or 2:1 rectangles) */
            sprent->objShape = OBJSHAPE_SQUARE;
            if (texture->w / 2 >= texture->h) {
                sprent->objShape = OBJSHAPE_WIDE;
            } else if (texture->h / 2 >= texture->w) {
                sprent->objShape = OBJSHAPE_TALL;
            }

            /* size in pixels */
            /* FIXME: "pitch" is hardcoded for 2bytes per pixel. */
            sprent->objSize = OBJSIZE_64;
            pitch = 128;
            if (maxside <= 8) {
                sprent->objSize = OBJSIZE_8;
                pitch = 16;
            } else if (maxside <= 16) {
                sprent->objSize = OBJSIZE_16;
                pitch = 32;
            } else if (maxside <= 32) {
                sprent->objSize = OBJSIZE_32;
                pitch = 64;
            }

            /* FIXME: this is hard-coded and will obviously only work for one
               sprite-texture.  tells it to look at the beginning of SPRITE_GFX
               for its pixels. */
            sprent->tileIdx = 0;

            /* now for the texture data */
            txdat->type = NDSTX_SPR;
            txdat->hw_index = whichspr;
            txdat->dim.hdx = 0x100;
            txdat->dim.hdy = 0;
            txdat->dim.vdx = 0;
            txdat->dim.vdy = 0x100;
            txdat->dim.pitch = pitch;
            txdat->dim.bpp = bpp;
            txdat->vram_pixels =
                (u16 *) (data->sub ? SPRITE_GFX_SUB : SPRITE_GFX);
            /* FIXME: use tileIdx*boundary
               to point to proper location */
        } else {
            SDL_SetError("Out of NDS sprites.");
        }
#endif
    } else if (texture->w <= 256 && texture->h <= 256) {
        int whichbg = -1, base = 0;
        if (!data->bg_taken[2]) {
            whichbg = 2;
        } else if (!data->bg_taken[3]) {
            whichbg = 3;
            base = 4;
        }
        if (whichbg >= 0) {
            texture->driverdata = SDL_calloc(1, sizeof(NDS_TextureData));
            txdat = (NDS_TextureData *) texture->driverdata;
            if (!txdat) {
                SDL_OutOfMemory();
                return -1;
            }
// hard-coded for 256x256 for now...
// TODO: a series of if-elseif-else's to find the closest but larger size.
            if (!data->sub) {
                if (bpp == 8) {
                    txdat->hw_index =
                        bgInit(whichbg, BgType_Bmp8, BgSize_B8_256x256, 0, 0);
                } else {
                    txdat->hw_index =
                        bgInit(whichbg, BgType_Bmp16, BgSize_B16_256x256, 0,
                               0);
                }
            } else {
                if (bpp == 8) {
                    txdat->hw_index =
                        bgInitSub(whichbg, BgType_Bmp8, BgSize_B8_256x256, 0,
                                  0);
                } else {
                    txdat->hw_index =
                        bgInitSub(whichbg, BgType_Bmp16, BgSize_B16_256x256,
                                  0, 0);
                }
            }

/*   useful functions
        bgGetGfxPtr(bg3);            
		bgSetCenter(bg3, rcX, rcY);
		bgSetRotateScale(bg3, angle, scaleX, scaleY);
		bgSetScroll(bg3, scrollX, scrollY);
		bgUpdate(bg3);
*/
            txdat->type = NDSTX_BG;
            txdat->pitch = (texture->w) * ((bpp+1) / 8);
            txdat->bpp = bpp;
            txdat->rotate = 0;
            txdat->scale.x = 0x100;
            txdat->scale.y = 0x100;
            txdat->scroll.x = 0;
            txdat->scroll.y = 0;
            txdat->vram_pixels = (u16 *) bgGetGfxPtr(txdat->hw_index);

            bgSetCenter(txdat->hw_index, 0, 0);
            bgSetRotateScale(txdat->hw_index, txdat->rotate, txdat->scale.x,
                             txdat->scale.y);
            bgSetScroll(txdat->hw_index, txdat->scroll.x, txdat->scroll.y);
            bgUpdate();

            data->bg_taken[whichbg] = 1;
            /*txdat->size = txdat->dim.pitch * texture->h; */
        } else {
            SDL_SetError("Out of NDS backgrounds.");
        }
    } else {
        SDL_SetError("Texture too big for NDS hardware.");
    }

    if (!texture->driverdata) {
        return -1;
    }

    return 0;
}

#if 0
static int
NDS_QueryTexturePixels(SDL_Renderer * renderer, SDL_Texture * texture,
                       void **pixels, int *pitch)
{
    NDS_TextureData *txdat = (NDS_TextureData *) texture->driverdata;
    *pixels = txdat->vram_pixels;
    *pitch = txdat->pitch;
    return 0;
}
#endif

static int
NDS_UpdateTexture(SDL_Renderer * renderer, SDL_Texture * texture,
                  const SDL_Rect * rect, const void *pixels, int pitch)
{
    NDS_TextureData *txdat;
    Uint8 *src, *dst;
    int row;
    size_t length;

    txdat = (NDS_TextureData *) texture->driverdata;

    src = (Uint8 *) pixels;
    dst =
        (Uint8 *) txdat->vram_pixels + rect->y * txdat->pitch + rect->x *
        ((txdat->bpp + 1) / 8);
    length = rect->w * ((txdat->bpp + 1) / 8);

    if (rect->w == texture->w) {
        dmaCopy(src, dst, length * rect->h);
    } else {
        for (row = 0; row < rect->h; ++row) {
            dmaCopy(src, dst, length);
            src += pitch;
            dst += txdat->pitch;
        }
    }

    return 0;
}

static int
NDS_LockTexture(SDL_Renderer * renderer, SDL_Texture * texture,
                const SDL_Rect * rect, int markDirty, void **pixels,
                int *pitch)
{
    NDS_TextureData *txdat = (NDS_TextureData *) texture->driverdata;

    *pixels = (void *) ((u8 *) txdat->vram_pixels + rect->y * txdat->pitch +
                        rect->x * ((txdat->bpp + 1) / 8));
    *pitch = txdat->pitch;

    return 0;
}

static void
NDS_UnlockTexture(SDL_Renderer * renderer, SDL_Texture * texture)
{
    /* stub! */
}

static int
NDS_RenderFillRects(SDL_Renderer * renderer, const SDL_Rect ** rects,
					int count)
{
    NDS_RenderData *data = (NDS_RenderData *) renderer->driverdata;

    printf("NDS_RenderFill: stub\n");

    /* TODO: make a single-color sprite and stretch it.
       calculate the "HDX" width modifier of the sprite by:
       let S be the actual sprite's width (like, 32 pixels for example)
       let R be the rectangle's width (maybe 50 pixels)
       HDX = (R<<8) / S;
       (it's fixed point, hence the bit shift.  same goes for vertical.
       be sure to use 32-bit int's for the bit shift before the division!)
     */

    return 0;
}

static int
NDS_RenderCopy(SDL_Renderer * renderer, SDL_Texture * texture,
               const SDL_Rect * srcrect, const SDL_Rect * dstrect)
{
    NDS_RenderData *data = (NDS_RenderData *) renderer->driverdata;
    NDS_TextureData *txdat = (NDS_TextureData *) texture->driverdata;
    SDL_Window *window = renderer->window;
    SDL_VideoDisplay *display = SDL_GetDisplayForWindow(window);
    int Bpp = SDL_BYTESPERPIXEL(texture->format);

    if (txdat->type == NDSTX_BG) {
        txdat->scroll.x = dstrect->x;
        txdat->scroll.y = dstrect->y;
    } else {
        /* sprites not fully implemented yet */
        printf("NDS_RenderCopy: used sprite!\n");
//        SpriteEntry *spr = &(data->oam_copy.spriteBuffer[txdat->hw_index]);
//        spr->posX = dstrect->x;
//        spr->posY = dstrect->y;
//        if (txdat->hw_index < MATRIX_COUNT && spr->isRotoscale) {          
//        }
    }

    return 0;
}


static void
NDS_RenderPresent(SDL_Renderer * renderer)
{
    NDS_RenderData *data = (NDS_RenderData *) renderer->driverdata;
    SDL_Window *window = renderer->window;
    SDL_VideoDisplay *display = SDL_GetDisplayForWindow(window);

    /* update sprites */
//    NDS_OAM_Update(&(data->oam_copy), data->sub);
    /* vsync for NDS */
    if (renderer->info.flags & SDL_RENDERER_PRESENTVSYNC) {
        swiWaitForVBlank();
    }
}

static void
NDS_DestroyTexture(SDL_Renderer * renderer, SDL_Texture * texture)
{
    NDS_TextureData *txdat = texture->driverdata;
    /* free anything else allocated for texture */
    SDL_free(txdat);
}

static void
NDS_DestroyRenderer(SDL_Renderer * renderer)
{
    NDS_RenderData *data = (NDS_RenderData *) renderer->driverdata;
    int i;

    if (data) {
        /* free anything else relevant if anything else is allocated. */
        SDL_free(data);
    }
    SDL_free(renderer);
}

/* vi: set ts=4 sw=4 expandtab: */
