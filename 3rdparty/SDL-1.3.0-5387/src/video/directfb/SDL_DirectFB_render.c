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

    SDL1.3 DirectFB driver by couriersud@arcor.de
	
*/
#include "SDL_DirectFB_video.h"
#include "SDL_DirectFB_window.h"
#include "SDL_DirectFB_modes.h"

#include "SDL_DirectFB_shape.h"

#include "../SDL_sysvideo.h"
#include "../../render/SDL_sysrender.h"
//#include "../SDL_rect_c.h"
//#include "../SDL_yuv_sw_c.h"

/* the following is not yet tested ... */
#define USE_DISPLAY_PALETTE			(0)


#define SDL_DFB_RENDERERDATA(rend) DirectFB_RenderData *renddata = ((rend) ? (DirectFB_RenderData *) (rend)->driverdata : NULL)


/* GDI renderer implementation */

static SDL_Renderer *DirectFB_CreateRenderer(SDL_Window * window,
                                             Uint32 flags);
static void DirectFB_ActivateRenderer(SDL_Renderer * renderer);
static int DirectFB_CreateTexture(SDL_Renderer * renderer,
                                  SDL_Texture * texture);
static int DirectFB_QueryTexturePixels(SDL_Renderer * renderer,
                                       SDL_Texture * texture,
                                       void **pixels, int *pitch);
static int DirectFB_SetTexturePalette(SDL_Renderer * renderer,
                                      SDL_Texture * texture,
                                      const SDL_Color * colors,
                                      int firstcolor, int ncolors);
static int DirectFB_GetTexturePalette(SDL_Renderer * renderer,
                                      SDL_Texture * texture,
                                      SDL_Color * colors,
                                      int firstcolor, int ncolors);
static int DirectFB_SetTextureAlphaMod(SDL_Renderer * renderer,
                                       SDL_Texture * texture);
static int DirectFB_SetTextureColorMod(SDL_Renderer * renderer,
                                       SDL_Texture * texture);
static int DirectFB_SetTextureBlendMode(SDL_Renderer * renderer,
                                        SDL_Texture * texture);
static int DirectFB_SetTextureScaleMode(SDL_Renderer * renderer,
                                        SDL_Texture * texture);
static int DirectFB_UpdateTexture(SDL_Renderer * renderer,
                                  SDL_Texture * texture,
                                  const SDL_Rect * rect,
                                  const void *pixels, int pitch);
static int DirectFB_LockTexture(SDL_Renderer * renderer,
                                SDL_Texture * texture,
                                const SDL_Rect * rect,
                                void **pixels, int *pitch);
static void DirectFB_UnlockTexture(SDL_Renderer * renderer,
                                   SDL_Texture * texture);
static void DirectFB_DirtyTexture(SDL_Renderer * renderer,
                                  SDL_Texture * texture, int numrects,
                                  const SDL_Rect * rects);
static int DirectFB_SetDrawBlendMode(SDL_Renderer * renderer);
static int DirectFB_RenderDrawPoints(SDL_Renderer * renderer,
                                const SDL_Point * points, int count);
static int DirectFB_RenderDrawLines(SDL_Renderer * renderer,
                               const SDL_Point * points, int count);
static int DirectFB_RenderDrawRects(SDL_Renderer * renderer,
		const SDL_Rect ** rects, int count);
static int DirectFB_RenderFillRects(SDL_Renderer * renderer,
		const SDL_Rect ** rects, int count);
static int DirectFB_RenderCopy(SDL_Renderer * renderer,
                               SDL_Texture * texture,
                               const SDL_Rect * srcrect,
                               const SDL_Rect * dstrect);
static void DirectFB_RenderPresent(SDL_Renderer * renderer);
static void DirectFB_DestroyTexture(SDL_Renderer * renderer,
                                    SDL_Texture * texture);
static void DirectFB_DestroyRenderer(SDL_Renderer * renderer);
static int DirectFB_RenderReadPixels(SDL_Renderer * renderer, const SDL_Rect * rect,
                     Uint32 format, void * pixels, int pitch);
static int DirectFB_RenderWritePixels(SDL_Renderer * renderer, const SDL_Rect * rect,
                      Uint32 format, const void * pixels, int pitch);


#define SDL_DFB_WINDOWSURFACE(win)  IDirectFBSurface *destsurf = ((DFB_WindowData *) ((win)->driverdata))->surface;

SDL_RenderDriver DirectFB_RenderDriver = {
    DirectFB_CreateRenderer,
    {
     "directfb",
     (SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED),
     /* (SDL_TEXTUREMODULATE_NONE | SDL_TEXTUREMODULATE_COLOR |
      SDL_TEXTUREMODULATE_ALPHA),
      (SDL_BLENDMODE_NONE | SDL_BLENDMODE_MASK | SDL_BLENDMODE_BLEND |
      SDL_BLENDMODE_ADD | SDL_BLENDMODE_MOD),
     (SDL_SCALEMODE_NONE | SDL_SCALEMODE_FAST |
      SDL_SCALEMODE_SLOW | SDL_SCALEMODE_BEST),*/
     0,
     {
    		 /* formats filled in later */
     },
     0,
     0}
};

typedef struct
{
    SDL_Window *window;
    DFBSurfaceFlipFlags flipflags;
    int size_changed;
    int lastBlendMode;
    DFBSurfaceBlittingFlags blitFlags;
    DFBSurfaceDrawingFlags drawFlags;
} DirectFB_RenderData;

typedef struct
{
    IDirectFBSurface *surface;
    Uint32 format;
    void *pixels;
    int pitch;
    IDirectFBPalette *palette;
    int isDirty;

    SDL_VideoDisplay *display;      /* only for yuv textures */

#if (DFB_VERSION_ATLEAST(1,2,0))
    DFBSurfaceRenderOptions render_options;
#endif
} DirectFB_TextureData;

static __inline__ void
SDLtoDFBRect(const SDL_Rect * sr, DFBRectangle * dr)
{
    dr->x = sr->x;
    dr->y = sr->y;
    dr->h = sr->h;
    dr->w = sr->w;
}


static int
TextureHasAlpha(DirectFB_TextureData * data)
{
    /* Drawing primitive ? */
    if (!data)
        return 0;
        
    return (DFB_PIXELFORMAT_HAS_ALPHA(DirectFB_SDLToDFBPixelFormat(data->format)) ? 1 : 0);
#if 0
    switch (data->format) {
    case SDL_PIXELFORMAT_INDEX4LSB:
    case SDL_PIXELFORMAT_INDEX4MSB:
    case SDL_PIXELFORMAT_ARGB4444:
    case SDL_PIXELFORMAT_ARGB1555:
    case SDL_PIXELFORMAT_ARGB8888:
    case SDL_PIXELFORMAT_RGBA8888:
    case SDL_PIXELFORMAT_ABGR8888:
    case SDL_PIXELFORMAT_BGRA8888:
    case SDL_PIXELFORMAT_ARGB2101010:
       return 1;
    default:
        return 0;
    }
#endif
}

static void
SetBlendMode(DirectFB_RenderData * data, int blendMode,
             DirectFB_TextureData * source)
{
    SDL_DFB_WINDOWSURFACE(data->window);

    //FIXME: check for format change
    if (1 || data->lastBlendMode != blendMode) {
        switch (blendMode) {
        case SDL_BLENDMODE_NONE:
                                           /**< No blending */
            data->blitFlags = DSBLIT_NOFX;
            data->drawFlags = DSDRAW_NOFX;
            SDL_DFB_CHECK(destsurf->SetSrcBlendFunction(destsurf, DSBF_ONE));
            SDL_DFB_CHECK(destsurf->SetDstBlendFunction(destsurf, DSBF_ZERO));
            break;
#if 0
        case SDL_BLENDMODE_MASK:
            data->blitFlags =  DSBLIT_BLEND_ALPHACHANNEL;
            data->drawFlags = DSDRAW_BLEND;
            SDL_DFB_CHECK(destsurf->SetSrcBlendFunction(destsurf, DSBF_SRCALPHA));
            SDL_DFB_CHECK(destsurf->SetDstBlendFunction(destsurf, DSBF_INVSRCALPHA));
            break;
#endif
        case SDL_BLENDMODE_BLEND:
            data->blitFlags = DSBLIT_BLEND_ALPHACHANNEL;
            data->drawFlags = DSDRAW_BLEND;
            SDL_DFB_CHECK(destsurf->SetSrcBlendFunction(destsurf, DSBF_SRCALPHA));
            SDL_DFB_CHECK(destsurf->SetDstBlendFunction(destsurf, DSBF_INVSRCALPHA));
            break;
        case SDL_BLENDMODE_ADD:
            data->blitFlags = DSBLIT_BLEND_ALPHACHANNEL;
            data->drawFlags = DSDRAW_BLEND;
            // FIXME: SRCALPHA kills performance on radeon ...
            // It will be cheaper to copy the surface to
            // a temporay surface and premultiply 
            if (source && TextureHasAlpha(source))
                SDL_DFB_CHECK(destsurf->SetSrcBlendFunction(destsurf, DSBF_SRCALPHA));
            else
                SDL_DFB_CHECK(destsurf->SetSrcBlendFunction(destsurf, DSBF_ONE));
            SDL_DFB_CHECK(destsurf->SetDstBlendFunction(destsurf, DSBF_ONE));
            break;
        case SDL_BLENDMODE_MOD:
            data->blitFlags = DSBLIT_BLEND_ALPHACHANNEL;
            data->drawFlags = DSDRAW_BLEND;
            //SDL_DFB_CHECK(destsurf->SetSrcBlendFunction(destsurf, DSBF_DESTCOLOR));
            //SDL_DFB_CHECK(destsurf->SetDstBlendFunction(destsurf, DSBF_ZERO));
            //data->glBlendFunc(GL_ZERO, GL_SRC_COLOR);
            SDL_DFB_CHECK(destsurf->SetSrcBlendFunction(destsurf, DSBF_ZERO));
            SDL_DFB_CHECK(destsurf->SetDstBlendFunction(destsurf, DSBF_SRCCOLOR));

            break;
        }
        data->lastBlendMode = blendMode;
    }
}

static int
DisplayPaletteChanged(void *userdata, SDL_Palette * palette)
{
#if USE_DISPLAY_PALETTE
    DirectFB_RenderData *data = (DirectFB_RenderData *) userdata;
    SDL_DFB_WINDOWSURFACE(data->window);
    IDirectFBPalette *surfpal;

    int i;
    int ncolors;
    DFBColor entries[256];

    SDL_DFB_CHECKERR(destsurf->GetPalette(destsurf, &surfpal));

    /* FIXME: number of colors */
    ncolors = (palette->ncolors < 256 ? palette->ncolors : 256);

    for (i = 0; i < ncolors; ++i) {
        entries[i].r = palette->colors[i].r;
        entries[i].g = palette->colors[i].g;
        entries[i].b = palette->colors[i].b;
        entries[i].a = palette->colors[i].unused;
    }
    SDL_DFB_CHECKERR(surfpal->SetEntries(surfpal, entries, ncolors, 0));
    return 0;
  error:
#else
    SDL_Unsupported();
#endif
    return -1;
}

static void
DirectFB_WindowEvent(SDL_Renderer * renderer, const SDL_WindowEvent *event)
{
    SDL_DFB_RENDERERDATA(renderer);

    if (event->event == SDL_WINDOWEVENT_SIZE_CHANGED) {
        /* Rebind the context to the window area and update matrices */
        //SDL_CurrentContext = NULL;
        //data->updateSize = SDL_TRUE;
        renddata->size_changed = SDL_TRUE;
   }
}

int
DirectFB_RenderClear(SDL_Renderer * renderer)
{
    SDL_DFB_RENDERERDATA(renderer);

    DirectFB_ActivateRenderer(renderer);

    return 0;
}

SDL_Renderer *
DirectFB_CreateRenderer(SDL_Window * window, Uint32 flags)
{
    SDL_DFB_WINDOWDATA(window);
    SDL_VideoDisplay *display = SDL_GetDisplayForWindow(window);
    SDL_Renderer *renderer = NULL;
    DirectFB_RenderData *data = NULL;
    DFBSurfaceCapabilities scaps;
    //char *p;

    SDL_DFB_ALLOC_CLEAR(renderer, sizeof(*renderer));
    SDL_DFB_ALLOC_CLEAR(data, sizeof(*data));

    renderer->WindowEvent = DirectFB_WindowEvent;
    renderer->CreateTexture = DirectFB_CreateTexture;
    renderer->SetTextureAlphaMod = DirectFB_SetTextureAlphaMod;
    renderer->SetTextureColorMod = DirectFB_SetTextureColorMod;
    renderer->SetTextureBlendMode = DirectFB_SetTextureBlendMode;
    renderer->UpdateTexture = DirectFB_UpdateTexture;
    renderer->LockTexture = DirectFB_LockTexture;
    renderer->RenderClear = DirectFB_RenderClear;
    renderer->UnlockTexture = DirectFB_UnlockTexture;
    renderer->RenderDrawPoints = DirectFB_RenderDrawPoints;
    renderer->RenderDrawLines = DirectFB_RenderDrawLines;
    /* SetDrawColor - no needed */
    renderer->RenderFillRects = DirectFB_RenderFillRects;
    /* RenderDrawEllipse - no reference implementation yet */
    /* RenderFillEllipse - no reference implementation yet */
    renderer->RenderCopy = DirectFB_RenderCopy;
    renderer->RenderPresent = DirectFB_RenderPresent;
    
    /* FIXME: Yet to be tested */
    renderer->RenderReadPixels = DirectFB_RenderReadPixels;
    //renderer->RenderWritePixels = DirectFB_RenderWritePixels;
    
    renderer->DestroyTexture = DirectFB_DestroyTexture;
    renderer->DestroyRenderer = DirectFB_DestroyRenderer;

#if 0
    renderer->QueryTexturePixels = DirectFB_QueryTexturePixels;
    renderer->SetTexturePalette = DirectFB_SetTexturePalette;
    renderer->GetTexturePalette = DirectFB_GetTexturePalette;
    renderer->SetTextureScaleMode = DirectFB_SetTextureScaleMode;
    renderer->DirtyTexture = DirectFB_DirtyTexture;
    renderer->SetDrawBlendMode = DirectFB_SetDrawBlendMode;
    renderer->RenderDrawRects = DirectFB_RenderDrawRects;
#endif

    renderer->info = DirectFB_RenderDriver.info;
    renderer->window = window;      /* SDL window */
    renderer->driverdata = data;

    renderer->info.flags =
        SDL_RENDERER_ACCELERATED;

    data->window = window;

    data->flipflags = DSFLIP_PIPELINE | DSFLIP_BLIT;

    if (flags & SDL_RENDERER_PRESENTVSYNC) {
        data->flipflags |= DSFLIP_WAITFORSYNC | DSFLIP_ONSYNC;
        renderer->info.flags |= SDL_RENDERER_PRESENTVSYNC;
    } else
        data->flipflags |= DSFLIP_ONSYNC;

    SDL_DFB_CHECKERR(windata->surface->
                     GetCapabilities(windata->surface, &scaps));

#if 0
    if (scaps & DSCAPS_DOUBLE)
        renderer->info.flags |= SDL_RENDERER_PRESENTFLIP2;
    else if (scaps & DSCAPS_TRIPLE)
        renderer->info.flags |= SDL_RENDERER_PRESENTFLIP3;
    else
        renderer->info.flags |= SDL_RENDERER_SINGLEBUFFER;
#endif

    DirectFB_SetSupportedPixelFormats(&renderer->info);

#if 0
    /* Set up a palette watch on the display palette */
    if (display->palette) {
        SDL_AddPaletteWatch(display->palette, DisplayPaletteChanged, data);
    }
#endif

    return renderer;

  error:
    SDL_DFB_FREE(renderer);
    SDL_DFB_FREE(data);
    return NULL;
}

static void
DirectFB_ActivateRenderer(SDL_Renderer * renderer)
{
    SDL_DFB_RENDERERDATA(renderer);
    SDL_Window *window = renderer->window;
    SDL_DFB_WINDOWDATA(window);

    if (renddata->size_changed || windata->wm_needs_redraw) {
        DirectFB_AdjustWindowSurface(window);
    }
}


static int
DirectFB_AcquireVidLayer(SDL_Renderer * renderer, SDL_Texture * texture)
{
    //SDL_DFB_RENDERERDATA(renderer);
    SDL_Window *window = renderer->window;
    SDL_VideoDisplay *display = SDL_GetDisplayForWindow(window);
    SDL_DFB_DEVICEDATA(display->device);
    DFB_DisplayData *dispdata = (DFB_DisplayData *) display->driverdata;
    DirectFB_TextureData *data = texture->driverdata;
    DFBDisplayLayerConfig layconf;
    DFBResult ret;

    if (devdata->use_yuv_direct && (dispdata->vidID >= 0)
        && (!dispdata->vidIDinuse)
        && SDL_ISPIXELFORMAT_FOURCC(data->format)) {
        layconf.flags =
            DLCONF_WIDTH | DLCONF_HEIGHT | DLCONF_PIXELFORMAT |
            DLCONF_SURFACE_CAPS;
        layconf.width = texture->w;
        layconf.height = texture->h;
        layconf.pixelformat = DirectFB_SDLToDFBPixelFormat(data->format);
        layconf.surface_caps = DSCAPS_VIDEOONLY | DSCAPS_DOUBLE;

        SDL_DFB_CHECKERR(devdata->dfb->GetDisplayLayer(devdata->dfb,
                                                       dispdata->vidID,
                                                       &dispdata->vidlayer));
        SDL_DFB_CHECKERR(dispdata->
                         vidlayer->SetCooperativeLevel(dispdata->vidlayer,
                                                       DLSCL_EXCLUSIVE));

        if (devdata->use_yuv_underlays) {
            ret = dispdata->vidlayer->SetLevel(dispdata->vidlayer, -1);
            if (ret != DFB_OK)
                SDL_DFB_DEBUG("Underlay Setlevel not supported\n");
        }
        SDL_DFB_CHECKERR(dispdata->
                         vidlayer->SetConfiguration(dispdata->vidlayer,
                                                    &layconf));
        SDL_DFB_CHECKERR(dispdata->
                         vidlayer->GetSurface(dispdata->vidlayer,
                                              &data->surface));
        dispdata->vidIDinuse = 1;
        data->display = display;
        return 0;
    }
    return 1;
  error:
    if (dispdata->vidlayer) {
        SDL_DFB_RELEASE(data->surface);
        SDL_DFB_CHECKERR(dispdata->
                         vidlayer->SetCooperativeLevel(dispdata->vidlayer,
                                                       DLSCL_ADMINISTRATIVE));
        SDL_DFB_RELEASE(dispdata->vidlayer);
    }
    return 1;
}

static int
DirectFB_CreateTexture(SDL_Renderer * renderer, SDL_Texture * texture)
{
    SDL_Window *window = renderer->window;
    SDL_VideoDisplay *display = SDL_GetDisplayForWindow(window);
    SDL_DFB_DEVICEDATA(display->device);
    DirectFB_TextureData *data;
    DFBSurfaceDescription dsc;
    DFBSurfacePixelFormat pixelformat;

    DirectFB_ActivateRenderer(renderer);

    SDL_DFB_ALLOC_CLEAR(data, sizeof(*data));
    texture->driverdata = data;

    /* find the right pixelformat */
    pixelformat = DirectFB_SDLToDFBPixelFormat(texture->format);
    if (pixelformat == DSPF_UNKNOWN) {
        SDL_SetError("Unknown pixel format %d\n", data->format);
        goto error;
    }

    data->format = texture->format;
    data->pitch = texture->w * DFB_BYTES_PER_PIXEL(pixelformat);

    if (DirectFB_AcquireVidLayer(renderer, texture) != 0) {
        /* fill surface description */
        dsc.flags =
            DSDESC_WIDTH | DSDESC_HEIGHT | DSDESC_PIXELFORMAT | DSDESC_CAPS;
        dsc.width = texture->w;
        dsc.height = texture->h;
        if(texture->format == SDL_PIXELFORMAT_YV12 ||
           texture->format == SDL_PIXELFORMAT_IYUV) {
           /* dfb has problems with odd sizes -make them even internally */
           dsc.width += (dsc.width % 2);
           dsc.height += (dsc.height % 2);
        }
        /* <1.2 Never use DSCAPS_VIDEOONLY here. It kills performance
         * No DSCAPS_SYSTEMONLY either - let dfb decide
         * 1.2: DSCAPS_SYSTEMONLY boosts performance by factor ~8
         * Depends on other settings as well. Let dfb decide.
         */
        dsc.caps = DSCAPS_PREMULTIPLIED;
#if 0
        if (texture->access == SDL_TEXTUREACCESS_STREAMING)
            dsc.caps |= DSCAPS_SYSTEMONLY;
        else
            dsc.caps |= DSCAPS_VIDEOONLY;
#endif

        dsc.pixelformat = pixelformat;
        data->pixels = NULL;

        /* Create the surface */
        SDL_DFB_CHECKERR(devdata->dfb->CreateSurface(devdata->dfb, &dsc,
                                                     &data->surface));
        if (SDL_ISPIXELFORMAT_INDEXED(data->format)
            && !SDL_ISPIXELFORMAT_FOURCC(data->format)) {
#if 1
            SDL_DFB_CHECKERR(data->surface->GetPalette(data->surface, &data->palette));
#else
            /* DFB has issues with blitting LUT8 surfaces.
			 * Creating a new palette does not help.
			 */
        	DFBPaletteDescription pal_desc;
        	pal_desc.flags = DPDESC_SIZE; // | DPDESC_ENTRIES
        	pal_desc.size = 256;
        	SDL_DFB_CHECKERR(devdata->dfb->CreatePalette(devdata->dfb, &pal_desc,&data->palette));
        	SDL_DFB_CHECKERR(data->surface->SetPalette(data->surface, data->palette));
#endif
        }

    }
#if (DFB_VERSION_ATLEAST(1,2,0))
    data->render_options = DSRO_NONE;
#endif
    if (texture->access == SDL_TEXTUREACCESS_STREAMING) {
        /* 3 plane YUVs return 1 bpp, but we need more space for other planes */
        if(texture->format == SDL_PIXELFORMAT_YV12 ||
           texture->format == SDL_PIXELFORMAT_IYUV) {
            SDL_DFB_ALLOC_CLEAR(data->pixels, (texture->h * data->pitch  + ((texture->h + texture->h % 2) * (data->pitch + data->pitch % 2) * 2) / 4));
        } else {
            SDL_DFB_ALLOC_CLEAR(data->pixels, texture->h * data->pitch);
        }
    }

    return 0;

  error:
    SDL_DFB_RELEASE(data->palette);
    SDL_DFB_RELEASE(data->surface);
    SDL_DFB_FREE(texture->driverdata);
    return -1;
}

static int
DirectFB_QueryTexturePixels(SDL_Renderer * renderer,
                            SDL_Texture * texture, void **pixels, int *pitch)
{
    DirectFB_TextureData *texturedata =
        (DirectFB_TextureData *) texture->driverdata;

    if (texturedata->display) {
        return -1;
    } else {
        *pixels = texturedata->pixels;
        *pitch = texturedata->pitch;
    }
    return 0;
}

static int
DirectFB_SetTexturePalette(SDL_Renderer * renderer,
                           SDL_Texture * texture,
                           const SDL_Color * colors, int firstcolor,
                           int ncolors)
{
    DirectFB_TextureData *data = (DirectFB_TextureData *) texture->driverdata;
    if (SDL_ISPIXELFORMAT_INDEXED(data->format)
        && !SDL_ISPIXELFORMAT_FOURCC(data->format)) {
        DFBColor entries[256];
        int i;

        if (ncolors > 256)
        	ncolors = 256;

        for (i = 0; i < ncolors; ++i) {
            entries[i].r = colors[i].r;
            entries[i].g = colors[i].g;
            entries[i].b = colors[i].b;
            entries[i].a = 0xff;
        }
        SDL_DFB_CHECKERR(data->
                         palette->SetEntries(data->palette, entries, ncolors, firstcolor));
        return 0;
    } else {
        SDL_SetError("YUV textures don't have a palette");
        return -1;
    }
  error:
    return -1;
}

static int
DirectFB_GetTexturePalette(SDL_Renderer * renderer,
                           SDL_Texture * texture, SDL_Color * colors,
                           int firstcolor, int ncolors)
{
    DirectFB_TextureData *data = (DirectFB_TextureData *) texture->driverdata;

    if (SDL_ISPIXELFORMAT_INDEXED(data->format)
        && !SDL_ISPIXELFORMAT_FOURCC(data->format)) {
        DFBColor entries[256];
        int i;

        SDL_DFB_CHECKERR(data->
                         palette->GetEntries(data->palette, entries, ncolors,
                                             firstcolor));

        for (i = 0; i < ncolors; ++i) {
            colors[i].r = entries[i].r;
            colors[i].g = entries[i].g;
            colors[i].b = entries[i].b;
            colors->unused = SDL_ALPHA_OPAQUE;
        }
        return 0;
    } else {
        SDL_SetError("YUV textures don't have a palette");
        return -1;
    }
  error:
    return -1;
}

static int
DirectFB_SetTextureAlphaMod(SDL_Renderer * renderer, SDL_Texture * texture)
{
    return 0;
}

static int
DirectFB_SetTextureColorMod(SDL_Renderer * renderer, SDL_Texture * texture)
{
    return 0;
}

static int
DirectFB_SetTextureBlendMode(SDL_Renderer * renderer, SDL_Texture * texture)
{
    switch (texture->blendMode) {
    case SDL_BLENDMODE_NONE:
    //case SDL_BLENDMODE_MASK:
    case SDL_BLENDMODE_BLEND:
    case SDL_BLENDMODE_ADD:
    case SDL_BLENDMODE_MOD:
        return 0;
    default:
        SDL_Unsupported();
        texture->blendMode = SDL_BLENDMODE_NONE;
        return -1;
    }
}

static int
DirectFB_SetDrawBlendMode(SDL_Renderer * renderer)
{
    switch (renderer->blendMode) {
    case SDL_BLENDMODE_NONE:
    //case SDL_BLENDMODE_MASK:
    case SDL_BLENDMODE_BLEND:
    case SDL_BLENDMODE_ADD:
    case SDL_BLENDMODE_MOD:
        return 0;
    default:
        SDL_Unsupported();
        renderer->blendMode = SDL_BLENDMODE_NONE;
        return -1;
    }
}

#if 0
static int
DirectFB_SetTextureScaleMode(SDL_Renderer * renderer, SDL_Texture * texture)
{
#if (DFB_VERSION_ATLEAST(1,2,0))

    DirectFB_TextureData *data = (DirectFB_TextureData *) texture->driverdata;

    switch (texture->scaleMode) {
    case SDL_SCALEMODE_NONE:
    case SDL_SCALEMODE_FAST:
        data->render_options = DSRO_NONE;
        break;
    case SDL_SCALEMODE_SLOW:
        data->render_options = DSRO_SMOOTH_UPSCALE | DSRO_SMOOTH_DOWNSCALE;
        break;
    case SDL_SCALEMODE_BEST:
        data->render_options =
            DSRO_SMOOTH_UPSCALE | DSRO_SMOOTH_DOWNSCALE | DSRO_ANTIALIAS;
        break;
    default:
        SDL_Unsupported();
        data->render_options = DSRO_NONE;
        texture->scaleMode = SDL_SCALEMODE_NONE;
        return -1;
    }
#endif
    return 0;
}
#endif

static int
DirectFB_UpdateTexture(SDL_Renderer * renderer, SDL_Texture * texture,
                       const SDL_Rect * rect, const void *pixels, int pitch)
{
    DirectFB_TextureData *data = (DirectFB_TextureData *) texture->driverdata;
    Uint8 *dpixels;
    int dpitch;
    Uint8 *src, *dst;
    int row;
    size_t length;
    int bpp = DFB_BYTES_PER_PIXEL(DirectFB_SDLToDFBPixelFormat(texture->format));
    // FIXME: SDL_BYTESPERPIXEL(texture->format) broken for yuv yv12 3 planes

    DirectFB_ActivateRenderer(renderer);

    if ((texture->format == SDL_PIXELFORMAT_YV12) ||
        (texture->format == SDL_PIXELFORMAT_IYUV)) {
        bpp = 1;
    }
    
    SDL_DFB_CHECKERR(data->surface->Lock(data->surface,
                                         DSLF_WRITE | DSLF_READ,
                                         ((void **) &dpixels), &dpitch));
    src = (Uint8 *) pixels;
    dst = (Uint8 *) dpixels + rect->y * dpitch + rect->x * bpp;
    length = rect->w * bpp;
    for (row = 0; row < rect->h; ++row) {
        SDL_memcpy(dst, src, length);
        src += pitch;
        dst += dpitch;
    }
    /* copy other planes for 3 plane formats */
    if ((texture->format == SDL_PIXELFORMAT_YV12) ||
        (texture->format == SDL_PIXELFORMAT_IYUV)) {
        src = (Uint8 *) pixels + texture->h * pitch;
        dst = (Uint8 *) dpixels + texture->h * dpitch + rect->y * dpitch / 4 + rect->x * bpp / 2;
        for (row = 0; row < rect->h / 2 + (rect->h & 1); ++row) {
            SDL_memcpy(dst, src, length / 2);
            src += pitch / 2;
            dst += dpitch / 2;
        }
        src = (Uint8 *) pixels + texture->h * pitch + texture->h * pitch / 4;
        dst = (Uint8 *) dpixels + texture->h * dpitch + texture->h * dpitch / 4 + rect->y * dpitch / 4 + rect->x * bpp / 2;
        for (row = 0; row < rect->h / 2 + (rect->h & 1); ++row) {
            SDL_memcpy(dst, src, length / 2);
            src += pitch / 2;
            dst += dpitch / 2;
        }
    }
    SDL_DFB_CHECKERR(data->surface->Unlock(data->surface));
    data->isDirty = 0;
    return 0;
  error:
    return 1;

}

static int
DirectFB_LockTexture(SDL_Renderer * renderer, SDL_Texture * texture,
                     const SDL_Rect * rect, void **pixels, int *pitch)
{
    DirectFB_TextureData *texturedata =
        (DirectFB_TextureData *) texture->driverdata;

    DirectFB_ActivateRenderer(renderer);

#if 0
    if (markDirty) {
        SDL_AddDirtyRect(&texturedata->dirty, rect);
    }
#endif

    if (texturedata->display) {
        void *fdata;
        int fpitch;

        SDL_DFB_CHECKERR(texturedata->surface->Lock(texturedata->surface,
                                                    DSLF_WRITE | DSLF_READ,
                                                    &fdata, &fpitch));
        *pitch = fpitch;
        *pixels = fdata;
    } else {
        *pixels =
            (void *) ((Uint8 *) texturedata->pixels +
                      rect->y * texturedata->pitch +
                      rect->x * DFB_BYTES_PER_PIXEL(DirectFB_SDLToDFBPixelFormat(texture->format)));
        *pitch = texturedata->pitch;
        texturedata->isDirty = 1;
    }
    return 0;

  error:
    return -1;
}

static void
DirectFB_UnlockTexture(SDL_Renderer * renderer, SDL_Texture * texture)
{
    DirectFB_TextureData *texturedata =
        (DirectFB_TextureData *) texture->driverdata;

    DirectFB_ActivateRenderer(renderer);

    if (texturedata->display) {
        SDL_DFB_CHECK(texturedata->surface->Unlock(texturedata->surface));
        texturedata->pixels = NULL;
    }
}

#if 0
static void
DirectFB_DirtyTexture(SDL_Renderer * renderer, SDL_Texture * texture,
                      int numrects, const SDL_Rect * rects)
{
    DirectFB_TextureData *data = (DirectFB_TextureData *) texture->driverdata;
    int i;

    for (i = 0; i < numrects; ++i) {
        SDL_AddDirtyRect(&data->dirty, &rects[i]);
    }
}
#endif

static int
PrepareDraw(SDL_Renderer * renderer)
{
    DirectFB_RenderData *data = (DirectFB_RenderData *) renderer->driverdata;
    SDL_DFB_WINDOWSURFACE(data->window);

    Uint8 r, g, b, a;

    r = renderer->r;
    g = renderer->g;
    b = renderer->b;
    a = renderer->a;

    SetBlendMode(data, renderer->blendMode, NULL);
    SDL_DFB_CHECKERR(destsurf->SetDrawingFlags(destsurf, data->drawFlags));

    switch (renderer->blendMode) {
    case SDL_BLENDMODE_NONE:
    //case SDL_BLENDMODE_MASK:
    case SDL_BLENDMODE_BLEND:
        break;
    case SDL_BLENDMODE_ADD:
    case SDL_BLENDMODE_MOD:
        r = ((int) r * (int) a) / 255;
        g = ((int) g * (int) a) / 255;
        b = ((int) b * (int) a) / 255;
        a = 255;
        break;
    }

    SDL_DFB_CHECKERR(destsurf->SetColor(destsurf, r, g, b, a));
    return 0;
  error:
    return -1;
}

static int DirectFB_RenderDrawPoints(SDL_Renderer * renderer,
                                const SDL_Point * points, int count)
{
    DirectFB_RenderData *data = (DirectFB_RenderData *) renderer->driverdata;
    SDL_DFB_WINDOWSURFACE(data->window);
    int i;

    DirectFB_ActivateRenderer(renderer);

    PrepareDraw(renderer);
    for (i=0; i < count; i++)
    	SDL_DFB_CHECKERR(destsurf->DrawLine(destsurf, points[i].x, points[i].y, points[i].x, points[i].y));
    return 0;
  error:
    return -1;
}

static int DirectFB_RenderDrawLines(SDL_Renderer * renderer,
                               const SDL_Point * points, int count)
{
    DirectFB_RenderData *data = (DirectFB_RenderData *) renderer->driverdata;
    SDL_DFB_WINDOWSURFACE(data->window);
    int i;

    DirectFB_ActivateRenderer(renderer);

    PrepareDraw(renderer);
    /* Use antialiasing when available */
#if (DFB_VERSION_ATLEAST(1,2,0))
    SDL_DFB_CHECKERR(destsurf->SetRenderOptions(destsurf, DSRO_ANTIALIAS));
#endif

    for (i=0; i < count - 1; i++)
    	SDL_DFB_CHECKERR(destsurf->DrawLine(destsurf, points[i].x, points[i].y, points[i+1].x, points[i+1].y));

    return 0;
  error:
    return -1;
}

static int
DirectFB_RenderDrawRects(SDL_Renderer * renderer, const SDL_Rect ** rects, int count)
{
    DirectFB_RenderData *data = (DirectFB_RenderData *) renderer->driverdata;
    SDL_DFB_WINDOWSURFACE(data->window);
    int i;

    DirectFB_ActivateRenderer(renderer);

    PrepareDraw(renderer);

    for (i=0; i<count; i++)
    	SDL_DFB_CHECKERR(destsurf->DrawRectangle(destsurf, rects[i]->x, rects[i]->y,
    			rects[i]->w, rects[i]->h));

    return 0;
  error:
    return -1;
}

static int
DirectFB_RenderFillRects(SDL_Renderer * renderer, const SDL_Rect ** rects, int count)
{
    DirectFB_RenderData *data = (DirectFB_RenderData *) renderer->driverdata;
    SDL_DFB_WINDOWSURFACE(data->window);
    int i;

    DirectFB_ActivateRenderer(renderer);

    PrepareDraw(renderer);

    for (i=0; i<count; i++)
    	SDL_DFB_CHECKERR(destsurf->FillRectangle(destsurf, rects[i]->x, rects[i]->y,
    			rects[i]->w, rects[i]->h));

    return 0;
  error:
    return -1;
}

static int
DirectFB_RenderCopy(SDL_Renderer * renderer, SDL_Texture * texture,
                    const SDL_Rect * srcrect, const SDL_Rect * dstrect)
{
    DirectFB_RenderData *data = (DirectFB_RenderData *) renderer->driverdata;
    SDL_DFB_WINDOWSURFACE(data->window);
    DirectFB_TextureData *texturedata =
        (DirectFB_TextureData *) texture->driverdata;
    Uint8 alpha, r, g, b;

    DirectFB_ActivateRenderer(renderer);

    if (texturedata->display) {
        int px, py;
        SDL_Window *window = renderer->window;
        SDL_DFB_WINDOWDATA(window);
        SDL_VideoDisplay *display = texturedata->display;
        DFB_DisplayData *dispdata = (DFB_DisplayData *) display->driverdata;

        SDL_DFB_CHECKERR(dispdata->
                         vidlayer->SetSourceRectangle(dispdata->vidlayer,
                                                      srcrect->x, srcrect->y,
                                                      srcrect->w,
                                                      srcrect->h));
        SDL_DFB_CHECK(windata->dfbwin->GetPosition(windata->dfbwin, &px, &py));
        px += windata->client.x;
        py += windata->client.y;
        SDL_DFB_CHECKERR(dispdata->
                         vidlayer->SetScreenRectangle(dispdata->vidlayer,
                                                      px + dstrect->x,
                                                      py + dstrect->y,
                                                      dstrect->w,
                                                      dstrect->h));
    } else {
        DFBRectangle sr, dr;
        DFBSurfaceBlittingFlags flags = 0;

#if 0
        if (texturedata->dirty.list) {
            SDL_DirtyRect *dirty;
            void *pixels;
            int bpp = DFB_BYTES_PER_PIXEL(DirectFB_SDLToDFBPixelFormat(texture->format));
            int pitch = texturedata->pitch;

            for (dirty = texturedata->dirty.list; dirty; dirty = dirty->next) {
                SDL_Rect *rect = &dirty->rect;
                pixels =
                    (void *) ((Uint8 *) texturedata->pixels +
                              rect->y * pitch + rect->x * bpp);
                DirectFB_UpdateTexture(renderer, texture, rect,
                                       pixels,
                                       texturedata->pitch);
            }
            SDL_ClearDirtyRects(&texturedata->dirty);
        }
#endif
        if (texturedata->isDirty)
        {
            SDL_Rect rect;

            rect.x = 0;
            rect.y = 0;
            rect.w = texture->w;
            rect.h = texture->h;

            DirectFB_UpdateTexture(renderer, texture, &rect, texturedata->pixels, texturedata->pitch);
        }

        SDLtoDFBRect(srcrect, &sr);
        SDLtoDFBRect(dstrect, &dr);

        alpha = r = g = b = 0xff;
		if (texture->modMode & SDL_TEXTUREMODULATE_ALPHA){
			alpha = texture->a;
        	flags |= DSBLIT_BLEND_COLORALPHA;
		}

        if (texture->modMode & SDL_TEXTUREMODULATE_COLOR) {
            r = texture->r;
            g = texture->g;
            b = texture->b;
            flags |= DSBLIT_COLORIZE;
        }
        SDL_DFB_CHECKERR(destsurf->
                         SetColor(destsurf, r, g, b, alpha));

        // ???? flags |= DSBLIT_SRC_PREMULTCOLOR;

        SetBlendMode(data, texture->blendMode, texturedata);

        SDL_DFB_CHECKERR(destsurf->SetBlittingFlags(destsurf,
                                                    data->blitFlags | flags));

#if (DFB_VERSION_ATLEAST(1,2,0))
        SDL_DFB_CHECKERR(destsurf->SetRenderOptions(destsurf,
                                                    texturedata->
                                                    render_options));
#endif

        if (srcrect->w == dstrect->w && srcrect->h == dstrect->h) {
            SDL_DFB_CHECKERR(destsurf->Blit(destsurf,
                                            texturedata->surface,
                                            &sr, dr.x, dr.y));
        } else {
            SDL_DFB_CHECKERR(destsurf->StretchBlit(destsurf,
                                                   texturedata->surface,
                                                   &sr, &dr));
        }
    }
    return 0;
  error:
    return -1;
}

static void
DirectFB_RenderPresent(SDL_Renderer * renderer)
{
    DirectFB_RenderData *data = (DirectFB_RenderData *) renderer->driverdata;
    SDL_Window *window = renderer->window;
    SDL_DFB_WINDOWDATA(window);
    SDL_ShapeData *shape_data = (window->shaper ? window->shaper->driverdata : NULL);

    DirectFB_ActivateRenderer(renderer);

    if (shape_data && shape_data->surface) {
        /* saturate the window surface alpha channel */
        SDL_DFB_CHECK(windata->window_surface->SetSrcBlendFunction(windata->window_surface, DSBF_ONE));
        SDL_DFB_CHECK(windata->window_surface->SetDstBlendFunction(windata->window_surface, DSBF_ONE));
        SDL_DFB_CHECK(windata->window_surface->SetDrawingFlags(windata->window_surface, DSDRAW_BLEND));
        SDL_DFB_CHECK(windata->window_surface->SetColor(windata->window_surface, 0, 0, 0, 0xff));
        SDL_DFB_CHECK(windata->window_surface->FillRectangle(windata->window_surface, 0,0, windata->size.w, windata->size.h));

        /* blit the mask */
        SDL_DFB_CHECK(windata->surface->SetSrcBlendFunction(windata->surface, DSBF_DESTCOLOR));
        SDL_DFB_CHECK(windata->surface->SetDstBlendFunction(windata->surface, DSBF_ZERO));
        SDL_DFB_CHECK(windata->surface->SetBlittingFlags(windata->surface, DSBLIT_BLEND_ALPHACHANNEL));
#if (DFB_VERSION_ATLEAST(1,2,0))
        SDL_DFB_CHECK(windata->surface->SetRenderOptions(windata->surface, DSRO_NONE));
#endif
        SDL_DFB_CHECK(windata->surface->Blit(windata->surface, shape_data->surface, NULL, 0, 0));
    }

    /* Send the data to the display */
    SDL_DFB_CHECK(windata->window_surface->Flip(windata->window_surface, NULL,
                                                data->flipflags));
}

static void
DirectFB_DestroyTexture(SDL_Renderer * renderer, SDL_Texture * texture)
{
    DirectFB_TextureData *data = (DirectFB_TextureData *) texture->driverdata;

    DirectFB_ActivateRenderer(renderer);

    if (!data) {
        return;
    }
    //SDL_FreeDirtyRects(&data->dirty);
    SDL_DFB_RELEASE(data->palette);
    SDL_DFB_RELEASE(data->surface);
    if (data->display) {
        DFB_DisplayData *dispdata =
            (DFB_DisplayData *) data->display->driverdata;
        dispdata->vidIDinuse = 0;
        /* FIXME: Shouldn't we reset the cooperative level */
        SDL_DFB_CHECK(dispdata->vidlayer->SetCooperativeLevel(dispdata->vidlayer,
                                                DLSCL_ADMINISTRATIVE));
        SDL_DFB_RELEASE(dispdata->vidlayer);
    }
    SDL_DFB_FREE(data->pixels);
    SDL_free(data);
    texture->driverdata = NULL;
}

static void
DirectFB_DestroyRenderer(SDL_Renderer * renderer)
{
    DirectFB_RenderData *data = (DirectFB_RenderData *) renderer->driverdata;
    SDL_VideoDisplay *display = renderer->SDL_GetDisplayForWindow(window);

#if 0
    if (display->palette) {
        SDL_DelPaletteWatch(display->palette, DisplayPaletteChanged, data);
    }
#endif

    if (data) {
        SDL_free(data);
    }
    SDL_free(renderer);
}

static int
DirectFB_RenderReadPixels(SDL_Renderer * renderer, const SDL_Rect * rect,
                     Uint32 format, void * pixels, int pitch)
{
    SDL_Window *window = renderer->window;
    SDL_DFB_WINDOWDATA(window);
    Uint32 sdl_format;
   	void * laypixels;
   	int laypitch;
  	DFBSurfacePixelFormat dfb_format;

    DirectFB_ActivateRenderer(renderer);

    SDL_DFB_CHECK(windata->surface->GetPixelFormat(windata->surface, &dfb_format));
    sdl_format = DirectFB_DFBToSDLPixelFormat(dfb_format);
    SDL_DFB_CHECK(windata->surface->Lock(windata->surface, DSLF_READ, (void **) &laypixels, &laypitch));
    
    laypixels += (rect->y * laypitch + rect->x * SDL_BYTESPERPIXEL(sdl_format) );
    SDL_ConvertPixels(rect->w, rect->h,
                      sdl_format, laypixels, laypitch,
                      format, pixels, pitch);

    SDL_DFB_CHECK(windata->surface->Unlock(windata->surface));
    
    return 0;
}

static int
DirectFB_RenderWritePixels(SDL_Renderer * renderer, const SDL_Rect * rect,
                      Uint32 format, const void * pixels, int pitch)
{
    SDL_Window *window = renderer->window;
    SDL_DFB_WINDOWDATA(window);
    Uint32 sdl_format;
   	void * laypixels;
   	int laypitch;
  	DFBSurfacePixelFormat dfb_format;

    SDL_DFB_CHECK(windata->surface->GetPixelFormat(windata->surface, &dfb_format));
    sdl_format = DirectFB_DFBToSDLPixelFormat(dfb_format);

    SDL_DFB_CHECK(windata->surface->Lock(windata->surface, DSLF_WRITE, (void **) &laypixels, &laypitch));
      
    laypixels += (rect->y * laypitch + rect->x * SDL_BYTESPERPIXEL(sdl_format) );
    SDL_ConvertPixels(rect->w, rect->h,
                      format, pixels, pitch,
                      sdl_format, laypixels, laypitch);

    SDL_DFB_CHECK(windata->surface->Unlock(windata->surface));

    return 0;
}


/* vi: set ts=4 sw=4 expandtab: */
