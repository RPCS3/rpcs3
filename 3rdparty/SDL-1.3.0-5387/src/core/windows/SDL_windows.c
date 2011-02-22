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

#include "SDL_error.h"
#include "SDL_windows.h"


/* Sets an error message based on GetLastError() */
void
WIN_SetError(const char *prefix)
{
    TCHAR buffer[1024];
    char *message;
    FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(), 0,
                  buffer, SDL_arraysize(buffer), NULL);
    message = WIN_StringToUTF8(buffer);
    SDL_SetError("%s%s%s", prefix ? prefix : "", prefix ? ": " : "", message);
    SDL_free(message);
}

/* vi: set ts=4 sw=4 expandtab: */
