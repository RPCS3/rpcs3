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

/* Dummy SDL video driver implementation; this is just enough to make an
 *  SDL-based application THINK it's got a working video driver, for
 *  applications that call SDL_Init(SDL_INIT_VIDEO) when they don't need it,
 *  and also for use as a collection of stubs when porting SDL to a new
 *  platform for which you haven't yet written a valid video driver.
 *
 * This is also a great way to determine bottlenecks: if you think that SDL
 *  is a performance problem for a given platform, enable this driver, and
 *  then see if your application runs faster without video overhead.
 *
 * Initial work by Ryan C. Gordon (icculus@icculus.org). A good portion
 *  of this was cut-and-pasted from Stephane Peter's work in the AAlib
 *  SDL video driver.  Renamed to "DUMMY" by Sam Lantinga.
 */

#include "SDL_video.h"
#include "SDL_mouse.h"
#include "../SDL_sysvideo.h"
#include "../SDL_pixels_c.h"
#include "../../events/SDL_events_c.h"

#include "SDL_nullvideo.h"
#include "SDL_nullevents_c.h"
#include "SDL_nullframebuffer_c.h"

#define DUMMYVID_DRIVER_NAME "dummy"

/* Initialization/Query functions */
static int DUMMY_VideoInit(_THIS);
static int DUMMY_SetDisplayMode(_THIS, SDL_VideoDisplay * display, SDL_DisplayMode * mode);
static void DUMMY_VideoQuit(_THIS);

/* DUMMY driver bootstrap functions */

static int
DUMMY_Available(void)
{
    const char *envr = SDL_getenv("SDL_VIDEODRIVER");
    if ((envr) && (SDL_strcmp(envr, DUMMYVID_DRIVER_NAME) == 0)) {
        return (1);
    }

    return (0);
}

static void
DUMMY_DeleteDevice(SDL_VideoDevice * device)
{
    SDL_free(device);
}

static SDL_VideoDevice *
DUMMY_CreateDevice(int devindex)
{
    SDL_VideoDevice *device;

    /* Initialize all variables that we clean on shutdown */
    device = (SDL_VideoDevice *) SDL_calloc(1, sizeof(SDL_VideoDevice));
    if (!device) {
        SDL_OutOfMemory();
        if (device) {
            SDL_free(device);
        }
        return (0);
    }

    /* Set the function pointers */
    device->VideoInit = DUMMY_VideoInit;
    device->VideoQuit = DUMMY_VideoQuit;
    device->SetDisplayMode = DUMMY_SetDisplayMode;
    device->PumpEvents = DUMMY_PumpEvents;
    device->CreateWindowFramebuffer = SDL_DUMMY_CreateWindowFramebuffer;
    device->UpdateWindowFramebuffer = SDL_DUMMY_UpdateWindowFramebuffer;
    device->DestroyWindowFramebuffer = SDL_DUMMY_DestroyWindowFramebuffer;

    device->free = DUMMY_DeleteDevice;

    return device;
}

VideoBootStrap DUMMY_bootstrap = {
    DUMMYVID_DRIVER_NAME, "SDL dummy video driver",
    DUMMY_Available, DUMMY_CreateDevice
};


int
DUMMY_VideoInit(_THIS)
{
    SDL_DisplayMode mode;

    /* Use a fake 32-bpp desktop mode */
    mode.format = SDL_PIXELFORMAT_RGB888;
    mode.w = 1024;
    mode.h = 768;
    mode.refresh_rate = 0;
    mode.driverdata = NULL;
    if (SDL_AddBasicVideoDisplay(&mode) < 0) {
        return -1;
    }

    SDL_zero(mode);
    SDL_AddDisplayMode(&_this->displays[0], &mode);

    /* We're done! */
    return 0;
}

static int
DUMMY_SetDisplayMode(_THIS, SDL_VideoDisplay * display, SDL_DisplayMode * mode)
{
    return 0;
}

void
DUMMY_VideoQuit(_THIS)
{
}

/* vi: set ts=4 sw=4 expandtab: */
