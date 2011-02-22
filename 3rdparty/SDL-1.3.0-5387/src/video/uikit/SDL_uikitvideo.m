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

#import <UIKit/UIKit.h>

#include "SDL_config.h"

#include "SDL_video.h"
#include "SDL_mouse.h"
#include "../SDL_sysvideo.h"
#include "../SDL_pixels_c.h"
#include "../../events/SDL_events_c.h"

#include "SDL_uikitvideo.h"
#include "SDL_uikitevents.h"
#include "SDL_uikitwindow.h"
#include "SDL_uikitopengles.h"

#include "SDL_assert.h"

#define UIKITVID_DRIVER_NAME "uikit"

/* Initialization/Query functions */
static int UIKit_VideoInit(_THIS);
static void UIKit_GetDisplayModes(_THIS, SDL_VideoDisplay * sdl_display);
static int UIKit_SetDisplayMode(_THIS, SDL_VideoDisplay * display,
                                SDL_DisplayMode * mode);
static void UIKit_VideoQuit(_THIS);

BOOL SDL_UIKit_supports_multiple_displays = NO;

/* DUMMY driver bootstrap functions */

static int
UIKit_Available(void)
{
    return (1);
}

static void UIKit_DeleteDevice(SDL_VideoDevice * device)
{
    SDL_free(device);
}

static SDL_VideoDevice *
UIKit_CreateDevice(int devindex)
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
    device->VideoInit = UIKit_VideoInit;
    device->VideoQuit = UIKit_VideoQuit;
    device->GetDisplayModes = UIKit_GetDisplayModes;
    device->SetDisplayMode = UIKit_SetDisplayMode;
    device->PumpEvents = UIKit_PumpEvents;
    device->CreateWindow = UIKit_CreateWindow;
    device->DestroyWindow = UIKit_DestroyWindow;
    device->GetWindowWMInfo = UIKit_GetWindowWMInfo;
    
    
    /* OpenGL (ES) functions */
    device->GL_MakeCurrent        = UIKit_GL_MakeCurrent;
    device->GL_SwapWindow        = UIKit_GL_SwapWindow;
    device->GL_CreateContext    = UIKit_GL_CreateContext;
    device->GL_DeleteContext    = UIKit_GL_DeleteContext;
    device->GL_GetProcAddress   = UIKit_GL_GetProcAddress;
    device->GL_LoadLibrary        = UIKit_GL_LoadLibrary;
    device->free = UIKit_DeleteDevice;

    device->gl_config.accelerated = 1;

    return device;
}

VideoBootStrap UIKIT_bootstrap = {
    UIKITVID_DRIVER_NAME, "SDL UIKit video driver",
    UIKit_Available, UIKit_CreateDevice
};


/*
!!! FIXME:

The main screen should list a AxB mode for portrait orientation, and then
 also list BxA for landscape mode. When setting a given resolution, we should
 rotate the view's transform appropriately (extra credit if you check the
 accelerometer and rotate the display so it's never upside down).

  http://iphonedevelopment.blogspot.com/2008/10/starting-in-landscape-mode-without.html

*/

static void
UIKit_GetDisplayModes(_THIS, SDL_VideoDisplay * display)
{
    UIScreen *uiscreen = (UIScreen *) display->driverdata;
    SDL_DisplayMode mode;
    SDL_zero(mode);

    // availableModes showed up in 3.2 (the iPad and later). We should only
    //  land here for at least that version of the OS.
    if (!SDL_UIKit_supports_multiple_displays) {
        const CGRect rect = [uiscreen bounds];
        mode.format = SDL_PIXELFORMAT_ABGR8888;
        mode.w = (int) rect.size.width;
        mode.h = (int) rect.size.height;
        mode.refresh_rate = 0;
        mode.driverdata = NULL;
        SDL_AddDisplayMode(display, &mode);
        return;
    }

    const NSArray *modes = [uiscreen availableModes];
    const NSUInteger mode_count = [modes count];
    NSUInteger i;
    for (i = 0; i < mode_count; i++) {
        UIScreenMode *uimode = (UIScreenMode *) [modes objectAtIndex:i];
        const CGSize size = [uimode size];
        mode.format = SDL_PIXELFORMAT_ABGR8888;
        mode.w = (int) size.width;
        mode.h = (int) size.height;
        mode.refresh_rate = 0;
        mode.driverdata = uimode;
        [uimode retain];
        SDL_AddDisplayMode(display, &mode);
    }
}


static void
UIKit_AddDisplay(UIScreen *uiscreen, int w, int h)
{
    SDL_VideoDisplay display;
    SDL_DisplayMode mode;
    SDL_zero(mode);
    mode.format = SDL_PIXELFORMAT_ABGR8888;
    mode.w = w;
    mode.h = h;
    mode.refresh_rate = 0;

    SDL_zero(display);
    display.desktop_mode = mode;
    display.current_mode = mode;

    [uiscreen retain];
    display.driverdata = uiscreen;
    SDL_AddVideoDisplay(&display);
}


int
UIKit_VideoInit(_THIS)
{
    _this->gl_config.driver_loaded = 1;

    NSString *reqSysVer = @"3.2";
    NSString *currSysVer = [[UIDevice currentDevice] systemVersion];
    if ([currSysVer compare:reqSysVer options:NSNumericSearch] != NSOrderedAscending)
        SDL_UIKit_supports_multiple_displays = YES;

    // If this is iPhoneOS < 3.2, all devices are one screen, 320x480 pixels.
    //  The iPad added both a larger main screen and the ability to use
    //  external displays.
    if (!SDL_UIKit_supports_multiple_displays) {
        // Just give 'em the whole main screen.
        UIScreen *uiscreen = [UIScreen mainScreen];
        const CGRect rect = [uiscreen bounds];
        UIKit_AddDisplay(uiscreen, (int)rect.size.width, (int)rect.size.height);
    } else {
        const NSArray *screens = [UIScreen screens];
        const NSUInteger screen_count = [screens count];
        NSUInteger i;
        for (i = 0; i < screen_count; i++) {
            // the main screen is the first element in the array.
            UIScreen *uiscreen = (UIScreen *) [screens objectAtIndex:i];
            const CGSize size = [[uiscreen currentMode] size];
            UIKit_AddDisplay(uiscreen, (int) size.width, (int) size.height);
        }
    }

    /* We're done! */
    return 0;
}

static int
UIKit_SetDisplayMode(_THIS, SDL_VideoDisplay * display, SDL_DisplayMode * mode)
{
    UIScreen *uiscreen = (UIScreen *) display->driverdata;
    if (!SDL_UIKit_supports_multiple_displays) {
        // Not on at least iPhoneOS 3.2 (versions prior to iPad).
        SDL_assert(mode->driverdata == NULL);
    } else {
        UIScreenMode *uimode = (UIScreenMode *) mode->driverdata;
        [uiscreen setCurrentMode:uimode];
    }

    return 0;
}

void
UIKit_VideoQuit(_THIS)
{
    // Release Objective-C objects, so higher level doesn't free() them.
    int i, j;
    for (i = 0; i < _this->num_displays; i++) {
        SDL_VideoDisplay *display = &_this->displays[i];
        UIScreen *uiscreen = (UIScreen *) display->driverdata;
        [uiscreen release];
        display->driverdata = NULL;
        for (j = 0; j < display->num_display_modes; j++) {
            SDL_DisplayMode *mode = &display->display_modes[j];
            UIScreenMode *uimode = (UIScreenMode *) mode->driverdata;
            if (uimode) {
                [uimode release];
                mode->driverdata = NULL;
            }
        }
    }
}

/* vi: set ts=4 sw=4 expandtab: */
