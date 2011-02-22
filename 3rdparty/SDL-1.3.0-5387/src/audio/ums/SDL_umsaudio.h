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

    Carsten Griwodz
    griff@kom.tu-darmstadt.de

    based on linux/SDL_dspaudio.h by Sam Lantinga
*/
#include "SDL_config.h"

#ifndef _SDL_UMSaudio_h
#define _SDL_UMSaudio_h

#include <UMS/UMSAudioDevice.h>

#include "../SDL_sysaudio.h"

/* Hidden "this" pointer for the audio functions */
#define _THIS	SDL_AudioDevice *this

struct SDL_PrivateAudioData
{
    /* Pointer to the (open) UMS audio device */
    Environment *ev;
    UMSAudioDevice umsdev;

    /* Raw mixing buffer */
    UMSAudioTypes_Buffer playbuf;
    UMSAudioTypes_Buffer fillbuf;

    long bytesPerSample;
};

#endif /* _SDL_UMSaudio_h */
/* vi: set ts=4 sw=4 expandtab: */
