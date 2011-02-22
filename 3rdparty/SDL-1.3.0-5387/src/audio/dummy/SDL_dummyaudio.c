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

    This file written by Ryan C. Gordon (icculus@icculus.org)
*/
#include "SDL_config.h"

/* Output audio to nowhere... */

#include "SDL_audio.h"
#include "../SDL_audio_c.h"
#include "SDL_dummyaudio.h"

static int
DUMMYAUD_OpenDevice(_THIS, const char *devname, int iscapture)
{
    return 1;                   /* always succeeds. */
}

static int
DUMMYAUD_Init(SDL_AudioDriverImpl * impl)
{
    /* Set the function pointers */
    impl->OpenDevice = DUMMYAUD_OpenDevice;
    impl->OnlyHasDefaultOutputDevice = 1;
    return 1;   /* this audio target is available. */
}

AudioBootStrap DUMMYAUD_bootstrap = {
    "dummy", "SDL dummy audio driver", DUMMYAUD_Init, 1
};

/* vi: set ts=4 sw=4 expandtab: */
