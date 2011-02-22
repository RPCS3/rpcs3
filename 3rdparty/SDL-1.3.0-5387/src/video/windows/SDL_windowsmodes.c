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

#include "SDL_windowsvideo.h"

/* Windows CE compatibility */
#ifndef CDS_FULLSCREEN
#define CDS_FULLSCREEN 0
#endif

static SDL_bool
WIN_GetDisplayMode(LPCTSTR deviceName, DWORD index, SDL_DisplayMode * mode)
{
    SDL_DisplayModeData *data;
    DEVMODE devmode;
#ifndef _WIN32_WCE
    HDC hdc;
#endif

    devmode.dmSize = sizeof(devmode);
    devmode.dmDriverExtra = 0;
    if (!EnumDisplaySettings(deviceName, index, &devmode)) {
        return SDL_FALSE;
    }

    data = (SDL_DisplayModeData *) SDL_malloc(sizeof(*data));
    if (!data) {
        return SDL_FALSE;
    }
    data->DeviceMode = devmode;
    data->DeviceMode.dmFields =
        (DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT | DM_DISPLAYFREQUENCY |
         DM_DISPLAYFLAGS);

    /* Fill in the mode information */
    mode->format = SDL_PIXELFORMAT_UNKNOWN;
    mode->w = devmode.dmPelsWidth;
    mode->h = devmode.dmPelsHeight;
    mode->refresh_rate = devmode.dmDisplayFrequency;
    mode->driverdata = data;
#ifdef _WIN32_WCE
    /* In WinCE EnumDisplaySettings(ENUM_CURRENT_SETTINGS) doesn't take the user defined orientation
       into account but GetSystemMetrics does. */
    if (index == ENUM_CURRENT_SETTINGS) {
        mode->w = GetSystemMetrics(SM_CXSCREEN);
        mode->h = GetSystemMetrics(SM_CYSCREEN);
    }
#endif

/* WinCE has no GetDIBits, therefore we can't use it to get the display format */
#ifndef _WIN32_WCE
    if (index == ENUM_CURRENT_SETTINGS
        && (hdc = CreateDC(deviceName, NULL, NULL, NULL)) != NULL) {
        char bmi_data[sizeof(BITMAPINFOHEADER) + 256 * sizeof(RGBQUAD)];
        LPBITMAPINFO bmi;
        HBITMAP hbm;

        SDL_zero(bmi_data);
        bmi = (LPBITMAPINFO) bmi_data;
        bmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);

        hbm = CreateCompatibleBitmap(hdc, 1, 1);
        GetDIBits(hdc, hbm, 0, 1, NULL, bmi, DIB_RGB_COLORS);
        GetDIBits(hdc, hbm, 0, 1, NULL, bmi, DIB_RGB_COLORS);
        DeleteObject(hbm);
        DeleteDC(hdc);
        if (bmi->bmiHeader.biCompression == BI_BITFIELDS) {
            switch (*(Uint32 *) bmi->bmiColors) {
            case 0x00FF0000:
                mode->format = SDL_PIXELFORMAT_RGB888;
                break;
            case 0x000000FF:
                mode->format = SDL_PIXELFORMAT_BGR888;
                break;
            case 0xF800:
                mode->format = SDL_PIXELFORMAT_RGB565;
                break;
            case 0x7C00:
                mode->format = SDL_PIXELFORMAT_RGB555;
                break;
            }
        } else if (bmi->bmiHeader.biBitCount == 8) {
            mode->format = SDL_PIXELFORMAT_INDEX8;
        } else if (bmi->bmiHeader.biBitCount == 4) {
            mode->format = SDL_PIXELFORMAT_INDEX4LSB;
        }
    } else
#endif /* _WIN32_WCE */
    {
        /* FIXME: Can we tell what this will be? */
        if ((devmode.dmFields & DM_BITSPERPEL) == DM_BITSPERPEL) {
            switch (devmode.dmBitsPerPel) {
            case 32:
                mode->format = SDL_PIXELFORMAT_RGB888;
                break;
            case 24:
                mode->format = SDL_PIXELFORMAT_RGB24;
                break;
            case 16:
                mode->format = SDL_PIXELFORMAT_RGB565;
                break;
            case 15:
                mode->format = SDL_PIXELFORMAT_RGB555;
                break;
            case 8:
                mode->format = SDL_PIXELFORMAT_INDEX8;
                break;
            case 4:
                mode->format = SDL_PIXELFORMAT_INDEX4LSB;
                break;
            }
        }
    }
    return SDL_TRUE;
}

static SDL_bool
WIN_AddDisplay(LPTSTR DeviceName)
{
    SDL_VideoDisplay display;
    SDL_DisplayData *displaydata;
    SDL_DisplayMode mode;

#ifdef DEBUG_MODES
    printf("Display: %s\n", WIN_StringToUTF8(DeviceName));
#endif
    if (!WIN_GetDisplayMode(DeviceName, ENUM_CURRENT_SETTINGS, &mode)) {
        return SDL_FALSE;
    }

    displaydata = (SDL_DisplayData *) SDL_malloc(sizeof(*displaydata));
    if (!displaydata) {
        return SDL_FALSE;
    }
    SDL_memcpy(displaydata->DeviceName, DeviceName,
               sizeof(displaydata->DeviceName));

    SDL_zero(display);
    display.desktop_mode = mode;
    display.current_mode = mode;
    display.driverdata = displaydata;
    SDL_AddVideoDisplay(&display);
    return SDL_TRUE;
}

int
WIN_InitModes(_THIS)
{
    int pass;
    DWORD i, j, count;
    DISPLAY_DEVICE device;

    device.cb = sizeof(device);

    /* Get the primary display in the first pass */
    for (pass = 0; pass < 2; ++pass) {
        for (i = 0; ; ++i) {
            TCHAR DeviceName[32];

            if (!EnumDisplayDevices(NULL, i, &device, 0)) {
                break;
            }
            if (!(device.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP)) {
                continue;
            }
            if (pass == 0) {
                if (!(device.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE)) {
                    continue;
                }
            } else {
                if (device.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE) {
                    continue;
                }
            }
            SDL_memcpy(DeviceName, device.DeviceName, sizeof(DeviceName));
#ifdef DEBUG_MODES
            printf("Device: %s\n", WIN_StringToUTF8(DeviceName));
#endif
            count = 0;
            for (j = 0; ; ++j) {
                if (!EnumDisplayDevices(DeviceName, j, &device, 0)) {
                    break;
                }
                if (!(device.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP)) {
                    continue;
                }
                if (pass == 0) {
                    if (!(device.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE)) {
                        continue;
                    }
                } else {
                    if (device.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE) {
                        continue;
                    }
                }
                count += WIN_AddDisplay(device.DeviceName);
            }
            if (count == 0) {
                WIN_AddDisplay(DeviceName);
            }
        }
    }
    if (_this->num_displays == 0) {
        SDL_SetError("No displays available");
        return -1;
    }
    return 0;
}

int
WIN_GetDisplayBounds(_THIS, SDL_VideoDisplay * display, SDL_Rect * rect)
{
    SDL_DisplayModeData *data = (SDL_DisplayModeData *) display->current_mode.driverdata;

#ifdef _WIN32_WCE
    // WINCE: DEVMODE.dmPosition not found, or may be mingw32ce bug
    rect->x = 0;
    rect->y = 0;
    rect->w = _this->windows->w;
    rect->h = _this->windows->h;
#else
    rect->x = (int)data->DeviceMode.dmPosition.x;
    rect->y = (int)data->DeviceMode.dmPosition.y;
    rect->w = data->DeviceMode.dmPelsWidth;
    rect->h = data->DeviceMode.dmPelsHeight;
#endif
    return 0;
}

void
WIN_GetDisplayModes(_THIS, SDL_VideoDisplay * display)
{
    SDL_DisplayData *data = (SDL_DisplayData *) display->driverdata;
    DWORD i;
    SDL_DisplayMode mode;

    for (i = 0;; ++i) {
        if (!WIN_GetDisplayMode(data->DeviceName, i, &mode)) {
            break;
        }
        if (SDL_ISPIXELFORMAT_INDEXED(mode.format)) {
            /* We don't support palettized modes now */
            continue;
        }
        if (mode.format != SDL_PIXELFORMAT_UNKNOWN) {
            if (!SDL_AddDisplayMode(display, &mode)) {
                SDL_free(mode.driverdata);
            }
        }
    }
}

int
WIN_SetDisplayMode(_THIS, SDL_VideoDisplay * display, SDL_DisplayMode * mode)
{
    SDL_DisplayData *displaydata = (SDL_DisplayData *) display->driverdata;
    SDL_DisplayModeData *data = (SDL_DisplayModeData *) mode->driverdata;
    LONG status;

#ifdef _WIN32_WCE
    /* TODO: implement correctly.
       On my Asus MyPAL, if I execute the code below
       I get DISP_CHANGE_BADFLAGS and the Titlebar of the fullscreen window stays
       visible ... (SDL_RaiseWindow() would fix that one) */
    return 0;
#endif

    status =
        ChangeDisplaySettingsEx(displaydata->DeviceName, &data->DeviceMode,
                                NULL, CDS_FULLSCREEN, NULL);
    if (status != DISP_CHANGE_SUCCESSFUL) {
        const char *reason = "Unknown reason";
        switch (status) {
        case DISP_CHANGE_BADFLAGS:
            reason = "DISP_CHANGE_BADFLAGS";
            break;
        case DISP_CHANGE_BADMODE:
            reason = "DISP_CHANGE_BADMODE";
            break;
        case DISP_CHANGE_BADPARAM:
            reason = "DISP_CHANGE_BADPARAM";
            break;
        case DISP_CHANGE_FAILED:
            reason = "DISP_CHANGE_FAILED";
            break;
        }
        SDL_SetError("ChangeDisplaySettingsEx() failed: %s", reason);
        return -1;
    }
    EnumDisplaySettings(displaydata->DeviceName, ENUM_CURRENT_SETTINGS, &data->DeviceMode);
    return 0;
}

void
WIN_QuitModes(_THIS)
{
    /* All fullscreen windows should have restored modes by now */
}

/* vi: set ts=4 sw=4 expandtab: */
