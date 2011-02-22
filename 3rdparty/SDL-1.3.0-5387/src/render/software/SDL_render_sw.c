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

#include "../SDL_sysrender.h"

#include "SDL_draw.h"
#include "SDL_blendfillrect.h"
#include "SDL_blendline.h"
#include "SDL_blendpoint.h"
#include "SDL_drawline.h"
#include "SDL_drawpoint.h"


/* SDL surface based renderer implementation */

static SDL_Renderer *SW_CreateRenderer(SDL_Window * window, Uint32 flags);
static void SW_WindowEvent(SDL_Renderer * renderer,
                           const SDL_WindowEvent *event);
static int SW_CreateTexture(SDL_Renderer * renderer, SDL_Texture * texture);
static int SW_SetTextureColorMod(SDL_Renderer * renderer,
                                 SDL_Texture * texture);
static int SW_SetTextureAlphaMod(SDL_Renderer * renderer,
                                 SDL_Texture * texture);
static int SW_SetTextureBlendMode(SDL_Renderer * renderer,
                                  SDL_Texture * texture);
static int SW_UpdateTexture(SDL_Renderer * renderer, SDL_Texture * texture,
                            const SDL_Rect * rect, const void *pixels,
                            int pitch);
static int SW_LockTexture(SDL_Renderer * renderer, SDL_Texture * texture,
                          const SDL_Rect * rect, void **pixels, int *pitch);
static void SW_UnlockTexture(SDL_Renderer * renderer, SDL_Texture * texture);
static int SW_UpdateViewport(SDL_Renderer * renderer);
static int SW_RenderClear(SDL_Renderer * renderer);
static int SW_RenderDrawPoints(SDL_Renderer * renderer,
                               const SDL_Point * points, int count);
static int SW_RenderDrawLines(SDL_Renderer * renderer,
                              const SDL_Point * points, int count);
static int SW_RenderFillRects(SDL_Renderer * renderer,
                              const SDL_Rect * rects, int count);
static int SW_RenderCopy(SDL_Renderer * renderer, SDL_Texture * texture,
                         const SDL_Rect * srcrect, const SDL_Rect * dstrect);
static int SW_RenderReadPixels(SDL_Renderer * renderer, const SDL_Rect * rect,
                               Uint32 format, void * pixels, int pitch);
static void SW_RenderPresent(SDL_Renderer * renderer);
static void SW_DestroyTexture(SDL_Renderer * renderer, SDL_Texture * texture);
static void SW_DestroyRenderer(SDL_Renderer * renderer);


SDL_RenderDriver SW_RenderDriver = {
    SW_CreateRenderer,
    {
     "software",
     SDL_RENDERER_SOFTWARE,
     8,
     {
      SDL_PIXELFORMAT_RGB555,
      SDL_PIXELFORMAT_RGB565,
      SDL_PIXELFORMAT_RGB888,
      SDL_PIXELFORMAT_BGR888,
      SDL_PIXELFORMAT_ARGB8888,
      SDL_PIXELFORMAT_RGBA8888,
      SDL_PIXELFORMAT_ABGR8888,
      SDL_PIXELFORMAT_BGRA8888
     },
     0,
     0}
};

typedef struct
{
    SDL_Surface *surface;
} SW_RenderData;


static SDL_Surface *
SW_ActivateRenderer(SDL_Renderer * renderer)
{
    SW_RenderData *data = (SW_RenderData *) renderer->driverdata;

    if (!data->surface) {
        data->surface = SDL_GetWindowSurface(renderer->window);

        SW_UpdateViewport(renderer);
    }
    return data->surface;
}

SDL_Renderer *
SW_CreateRendererForSurface(SDL_Surface * surface)
{
    SDL_Renderer *renderer;
    SW_RenderData *data;

    if (!surface) {
        SDL_SetError("Can't create renderer for NULL surface");
        return NULL;
    }

    renderer = (SDL_Renderer *) SDL_calloc(1, sizeof(*renderer));
    if (!renderer) {
        SDL_OutOfMemory();
        return NULL;
    }

    data = (SW_RenderData *) SDL_calloc(1, sizeof(*data));
    if (!data) {
        SW_DestroyRenderer(renderer);
        SDL_OutOfMemory();
        return NULL;
    }
    data->surface = surface;

    renderer->WindowEvent = SW_WindowEvent;
    renderer->CreateTexture = SW_CreateTexture;
    renderer->SetTextureColorMod = SW_SetTextureColorMod;
    renderer->SetTextureAlphaMod = SW_SetTextureAlphaMod;
    renderer->SetTextureBlendMode = SW_SetTextureBlendMode;
    renderer->UpdateTexture = SW_UpdateTexture;
    renderer->LockTexture = SW_LockTexture;
    renderer->UnlockTexture = SW_UnlockTexture;
    renderer->UpdateViewport = SW_UpdateViewport;
    renderer->DestroyTexture = SW_DestroyTexture;
    renderer->RenderClear = SW_RenderClear;
    renderer->RenderDrawPoints = SW_RenderDrawPoints;
    renderer->RenderDrawLines = SW_RenderDrawLines;
    renderer->RenderFillRects = SW_RenderFillRects;
    renderer->RenderCopy = SW_RenderCopy;
    renderer->RenderReadPixels = SW_RenderReadPixels;
    renderer->RenderPresent = SW_RenderPresent;
    renderer->DestroyRenderer = SW_DestroyRenderer;
    renderer->info = SW_RenderDriver.info;
    renderer->driverdata = data;

    SW_ActivateRenderer(renderer);

    return renderer;
}

SDL_Renderer *
SW_CreateRenderer(SDL_Window * window, Uint32 flags)
{
    SDL_Surface *surface;

    surface = SDL_GetWindowSurface(window);
    if (!surface) {
        return NULL;
    }
    return SW_CreateRendererForSurface(surface);
}

static void
SW_WindowEvent(SDL_Renderer * renderer, const SDL_WindowEvent *event)
{
    SW_RenderData *data = (SW_RenderData *) renderer->driverdata;

    if (event->event == SDL_WINDOWEVENT_SIZE_CHANGED) {
        data->surface = NULL;
    }
}

static int
SW_CreateTexture(SDL_Renderer * renderer, SDL_Texture * texture)
{
    int bpp;
    Uint32 Rmask, Gmask, Bmask, Amask;

    if (!SDL_PixelFormatEnumToMasks
        (texture->format, &bpp, &Rmask, &Gmask, &Bmask, &Amask)) {
        SDL_SetError("Unknown texture format");
        return -1;
    }

    texture->driverdata =
        SDL_CreateRGBSurface(0, texture->w, texture->h, bpp, Rmask, Gmask,
                             Bmask, Amask);
    SDL_SetSurfaceColorMod(texture->driverdata, texture->r, texture->g,
                           texture->b);
    SDL_SetSurfaceAlphaMod(texture->driverdata, texture->a);
    SDL_SetSurfaceBlendMode(texture->driverdata, texture->blendMode);

    if (texture->access == SDL_TEXTUREACCESS_STATIC) {
        SDL_SetSurfaceRLE(texture->driverdata, 1);
    }

    if (!texture->driverdata) {
        return -1;
    }
    return 0;
}

static int
SW_SetTextureColorMod(SDL_Renderer * renderer, SDL_Texture * texture)
{
    SDL_Surface *surface = (SDL_Surface *) texture->driverdata;
    return SDL_SetSurfaceColorMod(surface, texture->r, texture->g,
                                  texture->b);
}

static int
SW_SetTextureAlphaMod(SDL_Renderer * renderer, SDL_Texture * texture)
{
    SDL_Surface *surface = (SDL_Surface *) texture->driverdata;
    return SDL_SetSurfaceAlphaMod(surface, texture->a);
}

static int
SW_SetTextureBlendMode(SDL_Renderer * renderer, SDL_Texture * texture)
{
    SDL_Surface *surface = (SDL_Surface *) texture->driverdata;
    return SDL_SetSurfaceBlendMode(surface, texture->blendMode);
}

static int
SW_UpdateTexture(SDL_Renderer * renderer, SDL_Texture * texture,
                 const SDL_Rect * rect, const void *pixels, int pitch)
{
    SDL_Surface *surface = (SDL_Surface *) texture->driverdata;
    Uint8 *src, *dst;
    int row;
    size_t length;

    src = (Uint8 *) pixels;
    dst = (Uint8 *) surface->pixels +
                        rect->y * surface->pitch +
                        rect->x * surface->format->BytesPerPixel;
    length = rect->w * surface->format->BytesPerPixel;
    for (row = 0; row < rect->h; ++row) {
        SDL_memcpy(dst, src, length);
        src += pitch;
        dst += surface->pitch;
    }
    return 0;
}

static int
SW_LockTexture(SDL_Renderer * renderer, SDL_Texture * texture,
               const SDL_Rect * rect, void **pixels, int *pitch)
{
    SDL_Surface *surface = (SDL_Surface *) texture->driverdata;

    *pixels =
        (void *) ((Uint8 *) surface->pixels + rect->y * surface->pitch +
                  rect->x * surface->format->BytesPerPixel);
    *pitch = surface->pitch;
    return 0;
}

static void
SW_UnlockTexture(SDL_Renderer * renderer, SDL_Texture * texture)
{
}

static int
SW_UpdateViewport(SDL_Renderer * renderer)
{
    SW_RenderData *data = (SW_RenderData *) renderer->driverdata;
    SDL_Surface *surface = data->surface;

    if (!surface) {
        /* We'll update the viewport after we recreate the surface */
        return 0;
    }

    if (!renderer->viewport.w && !renderer->viewport.h) {
        /* There may be no window, so update the viewport directly */
        renderer->viewport.w = surface->w;
        renderer->viewport.h = surface->h;
    }
    SDL_SetClipRect(data->surface, &renderer->viewport);
    return 0;
}

static int
SW_RenderClear(SDL_Renderer * renderer)
{
    SDL_Surface *surface = SW_ActivateRenderer(renderer);
    Uint32 color;
    SDL_Rect clip_rect;

    if (!surface) {
        return -1;
    }

    color = SDL_MapRGBA(surface->format,
                        renderer->r, renderer->g, renderer->b, renderer->a);

    /* By definition the clear ignores the clip rect */
    clip_rect = surface->clip_rect;
    SDL_SetClipRect(surface, NULL);
    SDL_FillRect(surface, NULL, color);
    SDL_SetClipRect(surface, &clip_rect);
    return 0;
}

static int
SW_RenderDrawPoints(SDL_Renderer * renderer, const SDL_Point * points,
                    int count)
{
    SDL_Surface *surface = SW_ActivateRenderer(renderer);
    SDL_Point *temp = NULL;
    int status;

    if (!surface) {
        return -1;
    }

    if (renderer->viewport.x || renderer->viewport.y) {
        int i;
        int x = renderer->viewport.x;
        int y = renderer->viewport.y;

        temp = SDL_stack_alloc(SDL_Point, count);
        for (i = 0; i < count; ++i) {
            temp[i].x = x + points[i].x;
            temp[i].y = y + points[i].x;
        }
        points = temp;
    }

    /* Draw the points! */
    if (renderer->blendMode == SDL_BLENDMODE_NONE) {
        Uint32 color = SDL_MapRGBA(surface->format,
                                   renderer->r, renderer->g, renderer->b,
                                   renderer->a);

        status = SDL_DrawPoints(surface, points, count, color);
    } else {
        status = SDL_BlendPoints(surface, points, count,
                                renderer->blendMode,
                                renderer->r, renderer->g, renderer->b,
                                renderer->a);
    }

    if (temp) {
        SDL_stack_free(temp);
    }
    return status;
}

static int
SW_RenderDrawLines(SDL_Renderer * renderer, const SDL_Point * points,
                   int count)
{
    SDL_Surface *surface = SW_ActivateRenderer(renderer);
    SDL_Point *temp = NULL;
    int status;

    if (!surface) {
        return -1;
    }

    if (renderer->viewport.x || renderer->viewport.y) {
        int i;
        int x = renderer->viewport.x;
        int y = renderer->viewport.y;

        temp = SDL_stack_alloc(SDL_Point, count);
        for (i = 0; i < count; ++i) {
            temp[i].x = x + points[i].x;
            temp[i].y = y + points[i].y;
        }
        points = temp;
    }

    /* Draw the lines! */
    if (renderer->blendMode == SDL_BLENDMODE_NONE) {
        Uint32 color = SDL_MapRGBA(surface->format,
                                   renderer->r, renderer->g, renderer->b,
                                   renderer->a);

        status = SDL_DrawLines(surface, points, count, color);
    } else {
        status = SDL_BlendLines(surface, points, count,
                                renderer->blendMode,
                                renderer->r, renderer->g, renderer->b,
                                renderer->a);
    }

    if (temp) {
        SDL_stack_free(temp);
    }
    return status;
}

static int
SW_RenderFillRects(SDL_Renderer * renderer, const SDL_Rect * rects, int count)
{
    SDL_Surface *surface = SW_ActivateRenderer(renderer);
    SDL_Rect *temp = NULL;
    int status;

    if (!surface) {
        return -1;
    }

    if (renderer->viewport.x || renderer->viewport.y) {
        int i;
        int x = renderer->viewport.x;
        int y = renderer->viewport.y;

        temp = SDL_stack_alloc(SDL_Rect, count);
        for (i = 0; i < count; ++i) {
            temp[i].x = x + rects[i].x;
            temp[i].y = y + rects[i].y;
            temp[i].w = rects[i].w;
            temp[i].h = rects[i].h;
        }
        rects = temp;
    }

    if (renderer->blendMode == SDL_BLENDMODE_NONE) {
        Uint32 color = SDL_MapRGBA(surface->format,
                                   renderer->r, renderer->g, renderer->b,
                                   renderer->a);
        status = SDL_FillRects(surface, rects, count, color);
    } else {
        status = SDL_BlendFillRects(surface, rects, count,
                                    renderer->blendMode,
                                    renderer->r, renderer->g, renderer->b,
                                    renderer->a);
    }

    if (temp) {
        SDL_stack_free(temp);
    }
    return status;
}

static int
SW_RenderCopy(SDL_Renderer * renderer, SDL_Texture * texture,
              const SDL_Rect * srcrect, const SDL_Rect * dstrect)
{
    SDL_Surface *surface = SW_ActivateRenderer(renderer);
    SDL_Surface *src = (SDL_Surface *) texture->driverdata;
    SDL_Rect final_rect = *dstrect;

    if (!surface) {
        return -1;
    }

    if (renderer->viewport.x || renderer->viewport.y) {
        final_rect.x += renderer->viewport.x;
        final_rect.y += renderer->viewport.y;
    }
    if ( srcrect->w == final_rect.w && srcrect->h == final_rect.h ) {
        return SDL_BlitSurface(src, srcrect, surface, &final_rect);
    } else {
        return SDL_BlitScaled(src, srcrect, surface, &final_rect);
    }
}

static int
SW_RenderReadPixels(SDL_Renderer * renderer, const SDL_Rect * rect,
                    Uint32 format, void * pixels, int pitch)
{
    SDL_Surface *surface = SW_ActivateRenderer(renderer);
    Uint32 src_format;
    void *src_pixels;
    SDL_Rect final_rect;

    if (!surface) {
        return -1;
    }

    if (renderer->viewport.x || renderer->viewport.y) {
        final_rect.x = renderer->viewport.x + rect->x;
        final_rect.y = renderer->viewport.y + rect->y;
        final_rect.w = rect->w;
        final_rect.h = rect->h;
        rect = &final_rect;
    }

    if (rect->x < 0 || rect->x+rect->w > surface->w ||
        rect->y < 0 || rect->y+rect->h > surface->h) {
        SDL_SetError("Tried to read outside of surface bounds");
        return -1;
    }

    src_format = surface->format->format;
    src_pixels = (void*)((Uint8 *) surface->pixels +
                    rect->y * surface->pitch +
                    rect->x * surface->format->BytesPerPixel);

    return SDL_ConvertPixels(rect->w, rect->h,
                             src_format, src_pixels, surface->pitch,
                             format, pixels, pitch);
}

static void
SW_RenderPresent(SDL_Renderer * renderer)
{
    SDL_Window *window = renderer->window;

    if (window) {
        SDL_UpdateWindowSurface(window);
    }
}

static void
SW_DestroyTexture(SDL_Renderer * renderer, SDL_Texture * texture)
{
    SDL_Surface *surface = (SDL_Surface *) texture->driverdata;

    SDL_FreeSurface(surface);
}

static void
SW_DestroyRenderer(SDL_Renderer * renderer)
{
    SW_RenderData *data = (SW_RenderData *) renderer->driverdata;

    if (data) {
        SDL_free(data);
    }
    SDL_free(renderer);
}

#endif /* !SDL_RENDER_DISABLED */

/* vi: set ts=4 sw=4 expandtab: */
