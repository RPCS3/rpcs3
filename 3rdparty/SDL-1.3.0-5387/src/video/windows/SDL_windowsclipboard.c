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
#include "SDL_windowswindow.h"
#include "../../events/SDL_clipboardevents_c.h"


#ifdef UNICODE
#define TEXT_FORMAT  CF_UNICODETEXT
#else
#define TEXT_FORMAT  CF_TEXT
#endif


/* Get any application owned window handle for clipboard association */
static HWND
GetWindowHandle(_THIS)
{
    SDL_Window *window;

    window = _this->windows;
    if (window) {
        return ((SDL_WindowData *) window->driverdata)->hwnd;
    }
    return NULL;
}

int
WIN_SetClipboardText(_THIS, const char *text)
{
    SDL_VideoData *data = (SDL_VideoData *) _this->driverdata;
    int result = 0;

    if (OpenClipboard(GetWindowHandle(_this))) {
        HANDLE hMem;
        LPTSTR tstr;
        SIZE_T i, size;

        /* Convert the text from UTF-8 to Windows Unicode */
        tstr = WIN_UTF8ToString(text);
        if (!tstr) {
            return -1;
        }

        /* Find out the size of the data */
        for (size = 0, i = 0; tstr[i]; ++i, ++size) {
            if (tstr[i] == '\n' && (i == 0 || tstr[i-1] != '\r')) {
                /* We're going to insert a carriage return */
                ++size;
            }
        }
        size = (size+1)*sizeof(*tstr);

        /* Save the data to the clipboard */
        hMem = GlobalAlloc(GMEM_MOVEABLE, size);
        if (hMem) {
            LPTSTR dst = (LPTSTR)GlobalLock(hMem);
            /* Copy the text over, adding carriage returns as necessary */
            for (i = 0; tstr[i]; ++i) {
                if (tstr[i] == '\n' && (i == 0 || tstr[i-1] != '\r')) {
                    *dst++ = '\r';
                }
                *dst++ = tstr[i];
            }
            *dst = 0;
            GlobalUnlock(hMem);

            EmptyClipboard();
            if (!SetClipboardData(TEXT_FORMAT, hMem)) {
                WIN_SetError("Couldn't set clipboard data");
                result = -1;
            }
#ifdef _WIN32_WCE
            data->clipboard_count = 0;
#else
            data->clipboard_count = GetClipboardSequenceNumber();
#endif
        }
        SDL_free(tstr);

        CloseClipboard();
    } else {
        WIN_SetError("Couldn't open clipboard");
        result = -1;
    }
    return result;
}

char *
WIN_GetClipboardText(_THIS)
{
    char *text;

    text = NULL;
    if (IsClipboardFormatAvailable(TEXT_FORMAT) &&
        OpenClipboard(GetWindowHandle(_this))) {
        HANDLE hMem;
        LPTSTR tstr;

        hMem = GetClipboardData(TEXT_FORMAT);
        if (hMem) {
            tstr = (LPTSTR)GlobalLock(hMem);
            text = WIN_StringToUTF8(tstr);
            GlobalUnlock(hMem);
        } else {
            WIN_SetError("Couldn't get clipboard data");
        }
        CloseClipboard();
    }
    if (!text) {
        text = SDL_strdup("");
    }
    return text;
}

SDL_bool
WIN_HasClipboardText(_THIS)
{
    if (IsClipboardFormatAvailable(TEXT_FORMAT)) {
        return SDL_TRUE;
    } else {
        return SDL_FALSE;
    }
}

void
WIN_CheckClipboardUpdate(struct SDL_VideoData * data)
{
    DWORD count;

#ifdef _WIN32_WCE
    count = 0;
#else
    count = GetClipboardSequenceNumber();
#endif
    if (count != data->clipboard_count) {
        if (data->clipboard_count) {
            SDL_SendClipboardUpdate();
        }
        data->clipboard_count = count;
    }
}

/* vi: set ts=4 sw=4 expandtab: */
