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

#include "../SDL_sysvideo.h"


#define DUMMY_SURFACE   "_SDL_DummySurface"

int SDL_DUMMY_CreateWindowFramebuffer(_THIS, SDL_Window * window, Uint32 * format, void ** pixels, int *pitch)
{
    SDL_Surface *surface;
    const Uint32 surface_format = SDL_PIXELFORMAT_RGB888;
    int w, h;
    int bpp;
    Uint32 Rmask, Gmask, Bmask, Amask;

    /* Free the old framebuffer surface */
    surface = (SDL_Surface *) SDL_GetWindowData(window, DUMMY_SURFACE);
    if (surface) {
        SDL_FreeSurface(surface);
    }

    /* Create a new one */
    SDL_PixelFormatEnumToMasks(surface_format, &bpp, &Rmask, &Gmask, &Bmask, &Amask);
    SDL_GetWindowSize(window, &w, &h);
    surface = SDL_CreateRGBSurface(0, w, h, bpp, Rmask, Gmask, Bmask, Amask);
    if (!surface) {
        return -1;
    }

    /* Save the info and return! */
    SDL_SetWindowData(window, DUMMY_SURFACE, surface);
    *format = surface_format;
    *pixels = surface->pixels;
    *pitch = surface->pitch;
    return 0;
}

int SDL_DUMMY_UpdateWindowFramebuffer(_THIS, SDL_Window * window, SDL_Rect * rects, int numrects)
{
    static int frame_number;
    SDL_Surface *surface;

    surface = (SDL_Surface *) SDL_GetWindowData(window, DUMMY_SURFACE);
    if (!surface) {
        SDL_SetError("Couldn't find dummy surface for window");
        return -1;
    }

    /* Send the data to the display */
    if (SDL_getenv("SDL_VIDEO_DUMMY_SAVE_FRAMES")) {
        char file[128];
        SDL_snprintf(file, sizeof(file), "SDL_window%d-%8.8d.bmp",
                     SDL_GetWindowID(window), ++frame_number);
        SDL_SaveBMP(surface, file);
    }
    return 0;
}

void SDL_DUMMY_DestroyWindowFramebuffer(_THIS, SDL_Window * window)
{
    SDL_Surface *surface;

    surface = (SDL_Surface *) SDL_SetWindowData(window, DUMMY_SURFACE, NULL);
    if (surface) {
        SDL_FreeSurface(surface);
    }
}

/* vi: set ts=4 sw=4 expandtab: */
