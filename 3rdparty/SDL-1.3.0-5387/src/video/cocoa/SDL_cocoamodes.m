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

#include "SDL_cocoavideo.h"

#if MAC_OS_X_VERSION_MAX_ALLOWED < MAC_OS_X_VERSION_10_5
/* 
    Add methods to get at private members of NSScreen. 
    Since there is a bug in Apple's screen switching code
    that does not update this variable when switching
    to fullscreen, we'll set it manually (but only for the
    main screen).
*/
@interface NSScreen (NSScreenAccess)
- (void) setFrame:(NSRect)frame;
@end

@implementation NSScreen (NSScreenAccess)
- (void) setFrame:(NSRect)frame;
{
    _frame = frame;
}
@end
#endif

static void
CG_SetError(const char *prefix, CGDisplayErr result)
{
    const char *error;

    switch (result) {
    case kCGErrorFailure:
        error = "kCGErrorFailure";
        break;
    case kCGErrorIllegalArgument:
        error = "kCGErrorIllegalArgument";
        break;
    case kCGErrorInvalidConnection:
        error = "kCGErrorInvalidConnection";
        break;
    case kCGErrorInvalidContext:
        error = "kCGErrorInvalidContext";
        break;
    case kCGErrorCannotComplete:
        error = "kCGErrorCannotComplete";
        break;
    case kCGErrorNameTooLong:
        error = "kCGErrorNameTooLong";
        break;
    case kCGErrorNotImplemented:
        error = "kCGErrorNotImplemented";
        break;
    case kCGErrorRangeCheck:
        error = "kCGErrorRangeCheck";
        break;
    case kCGErrorTypeCheck:
        error = "kCGErrorTypeCheck";
        break;
    case kCGErrorNoCurrentPoint:
        error = "kCGErrorNoCurrentPoint";
        break;
    case kCGErrorInvalidOperation:
        error = "kCGErrorInvalidOperation";
        break;
    case kCGErrorNoneAvailable:
        error = "kCGErrorNoneAvailable";
        break;
    default:
        error = "Unknown Error";
        break;
    }
    SDL_SetError("%s: %s", prefix, error);
}

static SDL_bool
GetDisplayMode(CFDictionaryRef moderef, SDL_DisplayMode *mode)
{
    SDL_DisplayModeData *data;
    CFNumberRef number;
    long width, height, bpp, refreshRate;

    data = (SDL_DisplayModeData *) SDL_malloc(sizeof(*data));
    if (!data) {
        return SDL_FALSE;
    }
    data->moderef = moderef;

    number = CFDictionaryGetValue(moderef, kCGDisplayWidth);
    CFNumberGetValue(number, kCFNumberLongType, &width);
    number = CFDictionaryGetValue(moderef, kCGDisplayHeight);
    CFNumberGetValue(number, kCFNumberLongType, &height);
    number = CFDictionaryGetValue(moderef, kCGDisplayBitsPerPixel);
    CFNumberGetValue(number, kCFNumberLongType, &bpp);
    number = CFDictionaryGetValue(moderef, kCGDisplayRefreshRate);
    CFNumberGetValue(number, kCFNumberLongType, &refreshRate);

    mode->format = SDL_PIXELFORMAT_UNKNOWN;
    switch (bpp) {
    case 8:
        /* We don't support palettized modes now */
        return SDL_FALSE;
    case 16:
        mode->format = SDL_PIXELFORMAT_ARGB1555;
        break;
    case 32:
        mode->format = SDL_PIXELFORMAT_ARGB8888;
        break;
    }
    mode->w = width;
    mode->h = height;
    mode->refresh_rate = refreshRate;
    mode->driverdata = data;
    return SDL_TRUE;
}

void
Cocoa_InitModes(_THIS)
{
    CGDisplayErr result;
    CGDirectDisplayID *displays;
    CGDisplayCount numDisplays;
    int pass, i;

    result = CGGetOnlineDisplayList(0, NULL, &numDisplays);
    if (result != kCGErrorSuccess) {
        CG_SetError("CGGetOnlineDisplayList()", result);
        return;
    }
    displays = SDL_stack_alloc(CGDirectDisplayID, numDisplays);
    result = CGGetOnlineDisplayList(numDisplays, displays, &numDisplays);
    if (result != kCGErrorSuccess) {
        CG_SetError("CGGetOnlineDisplayList()", result);
        SDL_stack_free(displays);
        return;
    }

    /* Pick up the primary display in the first pass, then get the rest */
    for (pass = 0; pass < 2; ++pass) {
        for (i = 0; i < numDisplays; ++i) {
            SDL_VideoDisplay display;
            SDL_DisplayData *displaydata;
            SDL_DisplayMode mode;
            CFDictionaryRef moderef;

            if (pass == 0) {
                if (!CGDisplayIsMain(displays[i])) {
                    continue;
                }
            } else {
                if (CGDisplayIsMain(displays[i])) {
                    continue;
                }
            }

            if (CGDisplayMirrorsDisplay(displays[i]) != kCGNullDirectDisplay) {
                continue;
            }
            moderef = CGDisplayCurrentMode(displays[i]);
            if (!moderef) {
                continue;
            }

            displaydata = (SDL_DisplayData *) SDL_malloc(sizeof(*displaydata));
            if (!displaydata) {
                continue;
            }
            displaydata->display = displays[i];

            SDL_zero(display);
            if (!GetDisplayMode (moderef, &mode)) {
                SDL_free(displaydata);
                continue;
            }
            display.desktop_mode = mode;
            display.current_mode = mode;
            display.driverdata = displaydata;
            SDL_AddVideoDisplay(&display);
        }
    }
    SDL_stack_free(displays);
}

int
Cocoa_GetDisplayBounds(_THIS, SDL_VideoDisplay * display, SDL_Rect * rect)
{
    SDL_DisplayData *displaydata = (SDL_DisplayData *) display->driverdata;
    CGRect cgrect;

    cgrect = CGDisplayBounds(displaydata->display);
    rect->x = (int)cgrect.origin.x;
    rect->y = (int)cgrect.origin.y;
    rect->w = (int)cgrect.size.width;
    rect->h = (int)cgrect.size.height;
    return 0;
}

static void
AddDisplayMode(const void *moderef, void *context)
{
    SDL_VideoDisplay *display = (SDL_VideoDisplay *) context;
    SDL_DisplayMode mode;

    if (GetDisplayMode(moderef, &mode)) {
        SDL_AddDisplayMode(display, &mode);
    }
}

void
Cocoa_GetDisplayModes(_THIS, SDL_VideoDisplay * display)
{
    SDL_DisplayData *data = (SDL_DisplayData *) display->driverdata;
    CFArrayRef modes;
    CFRange range;

    modes = CGDisplayAvailableModes(data->display);
    if (!modes) {
        return;
    }
    range.location = 0;
    range.length = CFArrayGetCount(modes);
    CFArrayApplyFunction(modes, range, AddDisplayMode, display);
}

int
Cocoa_SetDisplayMode(_THIS, SDL_VideoDisplay * display, SDL_DisplayMode * mode)
{
    SDL_DisplayData *displaydata = (SDL_DisplayData *) display->driverdata;
    SDL_DisplayModeData *data = (SDL_DisplayModeData *) mode->driverdata;
    CGDisplayFadeReservationToken fade_token = kCGDisplayFadeReservationInvalidToken;
    CGError result;

    /* Fade to black to hide resolution-switching flicker */
    if (CGAcquireDisplayFadeReservation(5, &fade_token) == kCGErrorSuccess) {
        CGDisplayFade(fade_token, 0.3, kCGDisplayBlendNormal, kCGDisplayBlendSolidColor, 0.0, 0.0, 0.0, TRUE);
    }

    if (data == display->desktop_mode.driverdata) {
        /* Restoring desktop mode */
        CGDisplaySwitchToMode(displaydata->display, data->moderef);

        if (CGDisplayIsMain(displaydata->display)) {
            CGReleaseAllDisplays();
        } else {
            CGDisplayRelease(displaydata->display);
        }

        if (CGDisplayIsMain(displaydata->display)) {
            ShowMenuBar();
        }
    } else {
        /* Put up the blanking window (a window above all other windows) */
        if (CGDisplayIsMain(displaydata->display)) {
            /* If we don't capture all displays, Cocoa tries to rearrange windows... *sigh* */
            result = CGCaptureAllDisplays();
        } else {
            result = CGDisplayCapture(displaydata->display);
        }
        if (result != kCGErrorSuccess) {
            CG_SetError("CGDisplayCapture()", result);
            goto ERR_NO_CAPTURE;
        }

        /* Do the physical switch */
        result = CGDisplaySwitchToMode(displaydata->display, data->moderef);
        if (result != kCGErrorSuccess) {
            CG_SetError("CGDisplaySwitchToMode()", result);
            goto ERR_NO_SWITCH;
        }

        /* Hide the menu bar so it doesn't intercept events */
        if (CGDisplayIsMain(displaydata->display)) {
            HideMenuBar();
        }
    }

    /* Fade in again (asynchronously) */
    if (fade_token != kCGDisplayFadeReservationInvalidToken) {
        CGDisplayFade(fade_token, 0.5, kCGDisplayBlendSolidColor, kCGDisplayBlendNormal, 0.0, 0.0, 0.0, FALSE);
        CGReleaseDisplayFadeReservation(fade_token);
    }

    return 0;

    /* Since the blanking window covers *all* windows (even force quit) correct recovery is crucial */
ERR_NO_SWITCH:
    CGDisplayRelease(displaydata->display);
ERR_NO_CAPTURE:
    if (fade_token != kCGDisplayFadeReservationInvalidToken) {
        CGDisplayFade (fade_token, 0.5, kCGDisplayBlendSolidColor, kCGDisplayBlendNormal, 0.0, 0.0, 0.0, FALSE);
        CGReleaseDisplayFadeReservation(fade_token);
    }
    return -1;
}

void
Cocoa_QuitModes(_THIS)
{
    int i;

    for (i = 0; i < _this->num_displays; ++i) {
        SDL_VideoDisplay *display = &_this->displays[i];

        if (display->current_mode.driverdata != display->desktop_mode.driverdata) {
            Cocoa_SetDisplayMode(_this, display, &display->desktop_mode);
        }
    }
    ShowMenuBar();
}

/* vi: set ts=4 sw=4 expandtab: */
