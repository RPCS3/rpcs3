/* I'm gambling no one uses this audio backend...we'll see who emails.  :)  */
#error this code has not been updated for SDL 1.3.
#error if no one emails icculus at icculus.org and tells him that this
#error  code is needed, this audio backend will eventually be removed from SDL.

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

/* Allow access to a raw mixing buffer */

#include <fcntl.h>
#include <errno.h>
#ifdef __NETBSD__
#include <sys/ioctl.h>
#include <sys/audioio.h>
#endif
#ifdef __SVR4
#include <sys/audioio.h>
#else
#include <sys/time.h>
#include <sys/types.h>
#endif
#include <unistd.h>

#include "SDL_timer.h"
#include "SDL_audio.h"
#include "../SDL_audiomem.h"
#include "../SDL_audio_c.h"
#include "../SDL_audiodev_c.h"
#include "SDL_sunaudio.h"

/* Open the audio device for playback, and don't block if busy */
#define OPEN_FLAGS	(O_WRONLY|O_NONBLOCK)

/* Audio driver functions */
static int DSP_OpenAudio(_THIS, SDL_AudioSpec * spec);
static void DSP_WaitAudio(_THIS);
static void DSP_PlayAudio(_THIS);
static Uint8 *DSP_GetAudioBuf(_THIS);
static void DSP_CloseAudio(_THIS);

static Uint8 snd2au(int sample);

/* Audio driver bootstrap functions */

static int
Audio_Available(void)
{
    int fd;
    int available;

    available = 0;
    fd = SDL_OpenAudioPath(NULL, 0, OPEN_FLAGS, 1);
    if (fd >= 0) {
        available = 1;
        close(fd);
    }
    return (available);
}

static void
Audio_DeleteDevice(SDL_AudioDevice * device)
{
    SDL_free(device->hidden);
    SDL_free(device);
}

static SDL_AudioDevice *
Audio_CreateDevice(int devindex)
{
    SDL_AudioDevice *this;

    /* Initialize all variables that we clean on shutdown */
    this = (SDL_AudioDevice *) SDL_malloc(sizeof(SDL_AudioDevice));
    if (this) {
        SDL_memset(this, 0, (sizeof *this));
        this->hidden = (struct SDL_PrivateAudioData *)
            SDL_malloc((sizeof *this->hidden));
    }
    if ((this == NULL) || (this->hidden == NULL)) {
        SDL_OutOfMemory();
        if (this) {
            SDL_free(this);
        }
        return (0);
    }
    SDL_memset(this->hidden, 0, (sizeof *this->hidden));
    audio_fd = -1;

    /* Set the function pointers */
    this->OpenAudio = DSP_OpenAudio;
    this->WaitAudio = DSP_WaitAudio;
    this->PlayAudio = DSP_PlayAudio;
    this->GetAudioBuf = DSP_GetAudioBuf;
    this->CloseAudio = DSP_CloseAudio;

    this->free = Audio_DeleteDevice;

    return this;
}

AudioBootStrap SUNAUDIO_bootstrap = {
    "audio", "UNIX /dev/audio interface",
    Audio_Available, Audio_CreateDevice, 0
};

#ifdef DEBUG_AUDIO
void
CheckUnderflow(_THIS)
{
#ifdef AUDIO_GETINFO
    audio_info_t info;
    int left;

    ioctl(audio_fd, AUDIO_GETINFO, &info);
    left = (written - info.play.samples);
    if (written && (left == 0)) {
        fprintf(stderr, "audio underflow!\n");
    }
#endif
}
#endif

void
DSP_WaitAudio(_THIS)
{
#ifdef AUDIO_GETINFO
#define SLEEP_FUDGE	10      /* 10 ms scheduling fudge factor */
    audio_info_t info;
    Sint32 left;

    ioctl(audio_fd, AUDIO_GETINFO, &info);
    left = (written - info.play.samples);
    if (left > fragsize) {
        Sint32 sleepy;

        sleepy = ((left - fragsize) / frequency);
        sleepy -= SLEEP_FUDGE;
        if (sleepy > 0) {
            SDL_Delay(sleepy);
        }
    }
#else
    fd_set fdset;

    FD_ZERO(&fdset);
    FD_SET(audio_fd, &fdset);
    select(audio_fd + 1, NULL, &fdset, NULL, NULL);
#endif
}

void
DSP_PlayAudio(_THIS)
{
    /* Write the audio data */
    if (ulaw_only) {
        /* Assuming that this->spec.freq >= 8000 Hz */
        int accum, incr, pos;
        Uint8 *aubuf;

        accum = 0;
        incr = this->spec.freq / 8;
        aubuf = ulaw_buf;
        switch (audio_fmt & 0xFF) {
        case 8:
            {
                Uint8 *sndbuf;

                sndbuf = mixbuf;
                for (pos = 0; pos < fragsize; ++pos) {
                    *aubuf = snd2au((0x80 - *sndbuf) * 64);
                    accum += incr;
                    while (accum > 0) {
                        accum -= 1000;
                        sndbuf += 1;
                    }
                    aubuf += 1;
                }
            }
            break;
        case 16:
            {
                Sint16 *sndbuf;

                sndbuf = (Sint16 *) mixbuf;
                for (pos = 0; pos < fragsize; ++pos) {
                    *aubuf = snd2au(*sndbuf / 4);
                    accum += incr;
                    while (accum > 0) {
                        accum -= 1000;
                        sndbuf += 1;
                    }
                    aubuf += 1;
                }
            }
            break;
        }
#ifdef DEBUG_AUDIO
        CheckUnderflow(this);
#endif
        if (write(audio_fd, ulaw_buf, fragsize) < 0) {
            /* Assume fatal error, for now */
            this->enabled = 0;
        }
        written += fragsize;
    } else {
#ifdef DEBUG_AUDIO
        CheckUnderflow(this);
#endif
        if (write(audio_fd, mixbuf, this->spec.size) < 0) {
            /* Assume fatal error, for now */
            this->enabled = 0;
        }
        written += fragsize;
    }
}

Uint8 *
DSP_GetAudioBuf(_THIS)
{
    return (mixbuf);
}

void
DSP_CloseAudio(_THIS)
{
    if (mixbuf != NULL) {
        SDL_FreeAudioMem(mixbuf);
        mixbuf = NULL;
    }
    if (ulaw_buf != NULL) {
        SDL_free(ulaw_buf);
        ulaw_buf = NULL;
    }
    close(audio_fd);
}

int
DSP_OpenAudio(_THIS, SDL_AudioSpec * spec)
{
    char audiodev[1024];
#ifdef AUDIO_SETINFO
    int enc;
#endif
    int desired_freq = spec->freq;

    /* Initialize our freeable variables, in case we fail */
    audio_fd = -1;
    mixbuf = NULL;
    ulaw_buf = NULL;

    /* Determine the audio parameters from the AudioSpec */
    switch (SDL_AUDIO_BITSIZE(spec->format)) {

    case 8:
        {                       /* Unsigned 8 bit audio data */
            spec->format = AUDIO_U8;
#ifdef AUDIO_SETINFO
            enc = AUDIO_ENCODING_LINEAR8;
#endif
        }
        break;

    case 16:
        {                       /* Signed 16 bit audio data */
            spec->format = AUDIO_S16SYS;
#ifdef AUDIO_SETINFO
            enc = AUDIO_ENCODING_LINEAR;
#endif
        }
        break;

    default:
        {
            /* !!! FIXME: fallback to conversion on unsupported types! */
            SDL_SetError("Unsupported audio format");
            return (-1);
        }
    }
    audio_fmt = spec->format;

    /* Open the audio device */
    audio_fd = SDL_OpenAudioPath(audiodev, sizeof(audiodev), OPEN_FLAGS, 1);
    if (audio_fd < 0) {
        SDL_SetError("Couldn't open %s: %s", audiodev, strerror(errno));
        return (-1);
    }

    ulaw_only = 0;              /* modern Suns do support linear audio */
#ifdef AUDIO_SETINFO
    for (;;) {
        audio_info_t info;
        AUDIO_INITINFO(&info);  /* init all fields to "no change" */

        /* Try to set the requested settings */
        info.play.sample_rate = spec->freq;
        info.play.channels = spec->channels;
        info.play.precision = (enc == AUDIO_ENCODING_ULAW)
            ? 8 : spec->format & 0xff;
        info.play.encoding = enc;
        if (ioctl(audio_fd, AUDIO_SETINFO, &info) == 0) {

            /* Check to be sure we got what we wanted */
            if (ioctl(audio_fd, AUDIO_GETINFO, &info) < 0) {
                SDL_SetError("Error getting audio parameters: %s",
                             strerror(errno));
                return -1;
            }
            if (info.play.encoding == enc
                && info.play.precision == (spec->format & 0xff)
                && info.play.channels == spec->channels) {
                /* Yow! All seems to be well! */
                spec->freq = info.play.sample_rate;
                break;
            }
        }

        switch (enc) {
        case AUDIO_ENCODING_LINEAR8:
            /* unsigned 8bit apparently not supported here */
            enc = AUDIO_ENCODING_LINEAR;
            spec->format = AUDIO_S16SYS;
            break;              /* try again */

        case AUDIO_ENCODING_LINEAR:
            /* linear 16bit didn't work either, resort to µ-law */
            enc = AUDIO_ENCODING_ULAW;
            spec->channels = 1;
            spec->freq = 8000;
            spec->format = AUDIO_U8;
            ulaw_only = 1;
            break;

        default:
            /* oh well... */
            SDL_SetError("Error setting audio parameters: %s",
                         strerror(errno));
            return -1;
        }
    }
#endif /* AUDIO_SETINFO */
    written = 0;

    /* We can actually convert on-the-fly to U-Law */
    if (ulaw_only) {
        spec->freq = desired_freq;
        fragsize = (spec->samples * 1000) / (spec->freq / 8);
        frequency = 8;
        ulaw_buf = (Uint8 *) SDL_malloc(fragsize);
        if (ulaw_buf == NULL) {
            SDL_OutOfMemory();
            return (-1);
        }
        spec->channels = 1;
    } else {
        fragsize = spec->samples;
        frequency = spec->freq / 1000;
    }
#ifdef DEBUG_AUDIO
    fprintf(stderr, "Audio device %s U-Law only\n",
            ulaw_only ? "is" : "is not");
    fprintf(stderr, "format=0x%x chan=%d freq=%d\n",
            spec->format, spec->channels, spec->freq);
#endif

    /* Update the fragment size as size in bytes */
    SDL_CalculateAudioSpec(spec);

    /* Allocate mixing buffer */
    mixbuf = (Uint8 *) SDL_AllocAudioMem(spec->size);
    if (mixbuf == NULL) {
        SDL_OutOfMemory();
        return (-1);
    }
    SDL_memset(mixbuf, spec->silence, spec->size);

    /* We're ready to rock and roll. :-) */
    return (0);
}

/************************************************************************/
/* This function (snd2au()) copyrighted:                                */
/************************************************************************/
/*      Copyright 1989 by Rich Gopstein and Harris Corporation          */
/*                                                                      */
/*      Permission to use, copy, modify, and distribute this software   */
/*      and its documentation for any purpose and without fee is        */
/*      hereby granted, provided that the above copyright notice        */
/*      appears in all copies and that both that copyright notice and   */
/*      this permission notice appear in supporting documentation, and  */
/*      that the name of Rich Gopstein and Harris Corporation not be    */
/*      used in advertising or publicity pertaining to distribution     */
/*      of the software without specific, written prior permission.     */
/*      Rich Gopstein and Harris Corporation make no representations    */
/*      about the suitability of this software for any purpose.  It     */
/*      provided "as is" without express or implied warranty.           */
/************************************************************************/

static Uint8
snd2au(int sample)
{

    int mask;

    if (sample < 0) {
        sample = -sample;
        mask = 0x7f;
    } else {
        mask = 0xff;
    }

    if (sample < 32) {
        sample = 0xF0 | (15 - sample / 2);
    } else if (sample < 96) {
        sample = 0xE0 | (15 - (sample - 32) / 4);
    } else if (sample < 224) {
        sample = 0xD0 | (15 - (sample - 96) / 8);
    } else if (sample < 480) {
        sample = 0xC0 | (15 - (sample - 224) / 16);
    } else if (sample < 992) {
        sample = 0xB0 | (15 - (sample - 480) / 32);
    } else if (sample < 2016) {
        sample = 0xA0 | (15 - (sample - 992) / 64);
    } else if (sample < 4064) {
        sample = 0x90 | (15 - (sample - 2016) / 128);
    } else if (sample < 8160) {
        sample = 0x80 | (15 - (sample - 4064) / 256);
    } else {
        sample = 0x80;
    }
    return (mask & sample);
}

/* vi: set ts=4 sw=4 expandtab: */
