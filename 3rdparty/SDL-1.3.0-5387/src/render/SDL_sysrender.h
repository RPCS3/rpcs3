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

#ifndef _SDL_sysrender_h
#define _SDL_sysrender_h

#include "SDL_render.h"
#include "SDL_events.h"
#include "SDL_yuv_sw_c.h"

/* The SDL 2D rendering system */

typedef struct SDL_RenderDriver SDL_RenderDriver;

/* Define the SDL texture structure */
struct SDL_Texture
{
    const void *magic;
    Uint32 format;              /**< The pixel format of the texture */
    int access;                 /**< SDL_TextureAccess */
    int w;                      /**< The width of the texture */
    int h;                      /**< The height of the texture */
    int modMode;                /**< The texture modulation mode */
    SDL_BlendMode blendMode;    /**< The texture blend mode */
    Uint8 r, g, b, a;           /**< Texture modulation values */

    SDL_Renderer *renderer;

    /* Support for formats not supported directly by the renderer */
    SDL_Texture *native;
    SDL_SW_YUVTexture *yuv;
    void *pixels;
    int pitch;
    SDL_Rect locked_rect;

    void *driverdata;           /**< Driver specific texture representation */

    SDL_Texture *prev;
    SDL_Texture *next;
};

/* Define the SDL renderer structure */
struct SDL_Renderer
{
    const void *magic;

    void (*WindowEvent) (SDL_Renderer * renderer, const SDL_WindowEvent *event);
    int (*CreateTexture) (SDL_Renderer * renderer, SDL_Texture * texture);
    int (*SetTextureColorMod) (SDL_Renderer * renderer,
                               SDL_Texture * texture);
    int (*SetTextureAlphaMod) (SDL_Renderer * renderer,
                               SDL_Texture * texture);
    int (*SetTextureBlendMode) (SDL_Renderer * renderer,
                                SDL_Texture * texture);
    int (*UpdateTexture) (SDL_Renderer * renderer, SDL_Texture * texture,
                          const SDL_Rect * rect, const void *pixels,
                          int pitch);
    int (*LockTexture) (SDL_Renderer * renderer, SDL_Texture * texture,
                        const SDL_Rect * rect, void **pixels, int *pitch);
    void (*UnlockTexture) (SDL_Renderer * renderer, SDL_Texture * texture);
    int (*UpdateViewport) (SDL_Renderer * renderer);
    int (*RenderClear) (SDL_Renderer * renderer);
    int (*RenderDrawPoints) (SDL_Renderer * renderer, const SDL_Point * points,
                             int count);
    int (*RenderDrawLines) (SDL_Renderer * renderer, const SDL_Point * points,
                            int count);
    int (*RenderFillRects) (SDL_Renderer * renderer, const SDL_Rect * rects,
                            int count);
    int (*RenderCopy) (SDL_Renderer * renderer, SDL_Texture * texture,
                       const SDL_Rect * srcrect, const SDL_Rect * dstrect);
    int (*RenderReadPixels) (SDL_Renderer * renderer, const SDL_Rect * rect,
                             Uint32 format, void * pixels, int pitch);
    void (*RenderPresent) (SDL_Renderer * renderer);
    void (*DestroyTexture) (SDL_Renderer * renderer, SDL_Texture * texture);

    void (*DestroyRenderer) (SDL_Renderer * renderer);

    /* The current renderer info */
    SDL_RendererInfo info;

    /* The window associated with the renderer */
    SDL_Window *window;

    /* The drawable area within the window */
    SDL_Rect viewport;

    /* The list of textures */
    SDL_Texture *textures;

    Uint8 r, g, b, a;                   /**< Color for drawing operations values */
    SDL_BlendMode blendMode;            /**< The drawing blend mode */

    void *driverdata;
};

/* Define the SDL render driver structure */
struct SDL_RenderDriver
{
    SDL_Renderer *(*CreateRenderer) (SDL_Window * window, Uint32 flags);

    /* Info about the renderer capabilities */
    SDL_RendererInfo info;
};

#if !SDL_RENDER_DISABLED

#if SDL_VIDEO_RENDER_D3D
extern SDL_RenderDriver D3D_RenderDriver;
#endif
#if SDL_VIDEO_RENDER_OGL
extern SDL_RenderDriver GL_RenderDriver;
#endif
#if SDL_VIDEO_RENDER_OGL_ES2
extern SDL_RenderDriver GLES2_RenderDriver;
#endif
#if SDL_VIDEO_RENDER_OGL_ES
extern SDL_RenderDriver GLES_RenderDriver;
#endif
#if SDL_VIDEO_RENDER_DIRECTFB
extern SDL_RenderDriver DirectFB_RenderDriver;
#endif
extern SDL_RenderDriver SW_RenderDriver;

#endif /* !SDL_RENDER_DISABLED */

#endif /* _SDL_sysrender_h */

/* vi: set ts=4 sw=4 expandtab: */
