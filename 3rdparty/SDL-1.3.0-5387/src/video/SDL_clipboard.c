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

#include "SDL_clipboard.h"
#include "SDL_sysvideo.h"


int
SDL_SetClipboardText(const char *text)
{
    SDL_VideoDevice *_this = SDL_GetVideoDevice();

    if (!text) {
        text = "";
    }
    if (_this->SetClipboardText) {
        return _this->SetClipboardText(_this, text);
    } else {
        _this->clipboard_text = SDL_strdup(text);
        return 0;
    }
}

char *
SDL_GetClipboardText(void)
{
    SDL_VideoDevice *_this = SDL_GetVideoDevice();

    if (_this->GetClipboardText) {
        return _this->GetClipboardText(_this);
    } else {
        const char *text = _this->clipboard_text;
        if (!text) {
            text = "";
        }
        return SDL_strdup(text);
    }
}

SDL_bool
SDL_HasClipboardText(void)
{
    SDL_VideoDevice *_this = SDL_GetVideoDevice();

    if (_this->HasClipboardText) {
        return _this->HasClipboardText(_this);
    } else {
        if (_this->clipboard_text) {
            return SDL_TRUE;
        } else {
            return SDL_FALSE;
        }
    }
}

/* vi: set ts=4 sw=4 expandtab: */
