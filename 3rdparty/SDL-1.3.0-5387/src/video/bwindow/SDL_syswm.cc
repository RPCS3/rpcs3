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

#include "SDL_BWin.h"

extern "C"
{

#include "SDL_syswm_c.h"
#include "SDL_error.h"

    void BE_SetWMCaption(_THIS, const char *title, const char *icon)
    {
        SDL_Win->SetTitle(title);
    }

    int BE_IconifyWindow(_THIS)
    {
        SDL_Win->Minimize(true);
    }

    int BE_GetWMInfo(_THIS, SDL_SysWMinfo * info)
    {
        if (info->version.major <= SDL_MAJOR_VERSION) {
            return 1;
        } else {
            SDL_SetError("Application not compiled with SDL %d.%d\n",
                         SDL_MAJOR_VERSION, SDL_MINOR_VERSION);
            return -1;
        }
    }

};                              /* Extern C */

/* vi: set ts=4 sw=4 expandtab: */
