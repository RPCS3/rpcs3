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

/* Output audio to NDS */

#include <nds.h>

#include "SDL_audio.h"
#include "../SDL_audio_c.h"
#include "SDL_ndsaudio.h"

static int
NDSAUD_OpenDevice(_THIS, const char *devname, int iscapture)
{
    SDL_AudioFormat test_format = SDL_FirstAudioFormat(this->spec.format);
    int valid_datatype = 0;

    this->hidden = SDL_malloc(sizeof(*(this->hidden)));
    if (!this->hidden) {
        SDL_OutOfMemory();
        return 0;
    }
    SDL_memset(this->hidden, 0, (sizeof *this->hidden));

    while ((!valid_datatype) && (test_format)) {
        this->spec.format = test_format;
        switch (test_format) {
        case AUDIO_S8:
            /*case AUDIO_S16LSB: */
            valid_datatype = 1;
            break;
        default:
            test_format = SDL_NextAudioFormat();
            break;
        }
    }

#if 0
    /* set the generic sound parameters */
    setGenericSound(22050,      /* sample rate */
                    127,        /* volume */
                    64,         /* panning/balance */
                    0);         /* sound format */
#endif

    return 1;
}

static void
NDSAUD_PlayDevice(_THIS)
{
#if 0
    playGenericSound(this->hidden->mixbuf, this->hidden->mixlen);

//    sound->data = this->hidden->mixbuf;/* pointer to raw audio data */
//    sound->len = this->hidden->mixlen; /* size of raw data pointed to above */
//    sound->rate = 22050; /* sample rate = 22050Hz */
//    sound->vol = 127;    /* volume [0..127] for [min..max] */
//    sound->pan = 64;     /* balance [0..127] for [left..right] */
//    sound->format = 0;   /* 0 for 16-bit, 1 for 8-bit */
//    playSound(sound);
#endif
}


static Uint8 *
NDSAUD_GetDeviceBuf(_THIS)
{
    return this->hidden->mixbuf;        /* is this right? */
}

static void
NDSAUD_WaitDevice(_THIS)
{
    /* stub */
}

static void
NDSAUD_CloseDevice(_THIS)
{
    /* stub */
}

static int
NDSAUD_Init(SDL_AudioDriverImpl * impl)
{
    /* Set the function pointers */
    impl->OpenDevice = NDSAUD_OpenDevice;
    impl->PlayDevice = NDSAUD_PlayDevice;
    impl->WaitDevice = NDSAUD_WaitDevice;
    impl->GetDeviceBuf = NDSAUD_GetDeviceBuf;
    impl->CloseDevice = NDSAUD_CloseDevice;

    /* and the capabilities */
    impl->HasCaptureSupport = 1;
    impl->OnlyHasDefaultOutputDevice = 1;
    impl->OnlyHasDefaultInputDevice = 1;

    return 1;   /* this audio target is available. */
}

AudioBootStrap NDSAUD_bootstrap = {
    "nds", "SDL NDS audio driver", NDSAUD_Init, 0       /*1? */
};

/* vi: set ts=4 sw=4 expandtab: */
