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

/* This is an include file for windows.h with the SDL build settings */

#ifndef _INCLUDED_WINDOWS_H
#define _INCLUDED_WINDOWS_H

#define WIN32_LEAN_AND_MEAN
#define STRICT
#ifndef UNICODE
#define UNICODE 1
#endif
#undef _WIN32_WINNT
#define _WIN32_WINNT  0x500   /* Need 0x410 for AlphaBlend() and 0x500 for EnumDisplayDevices() */

#include <windows.h>


/* Routines to convert from UTF8 to native Windows text */
#if UNICODE
#define WIN_StringToUTF8(S) SDL_iconv_string("UTF-8", "UCS-2", (char *)(S), (SDL_wcslen(S)+1)*sizeof(WCHAR))
#define WIN_UTF8ToString(S) (WCHAR *)SDL_iconv_string("UCS-2", "UTF-8", (char *)(S), SDL_strlen(S)+1)
#else
#define WIN_StringToUTF8(S) SDL_iconv_string("UTF-8", "ASCII", (char *)(S), (SDL_strlen(S)+1))
#define WIN_UTF8ToString(S) SDL_iconv_string("ASCII", "UTF-8", (char *)(S), SDL_strlen(S)+1)
#endif

/* Sets an error message based on GetLastError() */
extern void WIN_SetError(const char *prefix);

#endif /* _INCLUDED_WINDOWS_H */

/* vi: set ts=4 sw=4 expandtab: */
