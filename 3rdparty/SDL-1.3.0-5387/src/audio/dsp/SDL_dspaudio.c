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

    Modified in Oct 2004 by Hannu Savolainen 
    hannu@opensound.com
*/
#include "SDL_config.h"

/* Allow access to a raw mixing buffer */

#include <stdio.h>              /* For perror() */
#include <string.h>             /* For strerror() */
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/stat.h>

#if SDL_AUDIO_DRIVER_OSS_SOUNDCARD_H
/* This is installed on some systems */
#include <soundcard.h>
#else
/* This is recommended by OSS */
#include <sys/soundcard.h>
#endif

#include "SDL_timer.h"
#include "SDL_audio.h"
#include "../SDL_audiomem.h"
#include "../SDL_audio_c.h"
#include "../SDL_audiodev_c.h"
#include "SDL_dspaudio.h"

/* The tag name used by DSP audio */
#define DSP_DRIVER_NAME         "dsp"

/* Open the audio device for playback, and don't block if busy */
#define OPEN_FLAGS_OUTPUT    (O_WRONLY|O_NONBLOCK)
#define OPEN_FLAGS_INPUT    (O_RDONLY|O_NONBLOCK)

static char **outputDevices = NULL;
static int outputDeviceCount = 0;
static char **inputDevices = NULL;
static int inputDeviceCount = 0;

static inline void
free_device_list(char ***devs, int *count)
{
    SDL_FreeUnixAudioDevices(devs, count);
}

static inline void
build_device_list(int iscapture, char ***devs, int *count)
{
    const int flags = ((iscapture) ? OPEN_FLAGS_INPUT : OPEN_FLAGS_OUTPUT);
    free_device_list(devs, count);
    SDL_EnumUnixAudioDevices(flags, 0, NULL, devs, count);
}

static inline void
build_device_lists(void)
{
    build_device_list(0, &outputDevices, &outputDeviceCount);
    build_device_list(1, &inputDevices, &inputDeviceCount);
}


static inline void
free_device_lists(void)
{
    free_device_list(&outputDevices, &outputDeviceCount);
    free_device_list(&inputDevices, &inputDeviceCount);
}


static void
DSP_Deinitialize(void)
{
    free_device_lists();
}


static int
DSP_DetectDevices(int iscapture)
{
    if (iscapture) {
        build_device_list(1, &inputDevices, &inputDeviceCount);
        return inputDeviceCount;
    } else {
        build_device_list(0, &outputDevices, &outputDeviceCount);
        return outputDeviceCount;
    }

    return 0;                   /* shouldn't ever hit this. */
}

static const char *
DSP_GetDeviceName(int index, int iscapture)
{
    if ((iscapture) && (index < inputDeviceCount)) {
        return inputDevices[index];
    } else if ((!iscapture) && (index < outputDeviceCount)) {
        return outputDevices[index];
    }

    SDL_SetError("No such device");
    return NULL;
}


static void
DSP_CloseDevice(_THIS)
{
    if (this->hidden != NULL) {
        if (this->hidden->mixbuf != NULL) {
            SDL_FreeAudioMem(this->hidden->mixbuf);
            this->hidden->mixbuf = NULL;
        }
        if (this->hidden->audio_fd >= 0) {
            close(this->hidden->audio_fd);
            this->hidden->audio_fd = -1;
        }
        SDL_free(this->hidden);
        this->hidden = NULL;
    }
}


static int
DSP_OpenDevice(_THIS, const char *devname, int iscapture)
{
    const int flags = ((iscapture) ? OPEN_FLAGS_INPUT : OPEN_FLAGS_OUTPUT);
    int format;
    int value;
    int frag_spec;
    SDL_AudioFormat test_format;

    /* We don't care what the devname is...we'll try to open anything. */
    /*  ...but default to first name in the list... */
    if (devname == NULL) {
        if (((iscapture) && (inputDeviceCount == 0)) ||
            ((!iscapture) && (outputDeviceCount == 0))) {
            SDL_SetError("No such audio device");
            return 0;
        }
        devname = ((iscapture) ? inputDevices[0] : outputDevices[0]);
    }

    /* Make sure fragment size stays a power of 2, or OSS fails. */
    /* I don't know which of these are actually legal values, though... */
    if (this->spec.channels > 8)
        this->spec.channels = 8;
    else if (this->spec.channels > 4)
        this->spec.channels = 4;
    else if (this->spec.channels > 2)
        this->spec.channels = 2;

    /* Initialize all variables that we clean on shutdown */
    this->hidden = (struct SDL_PrivateAudioData *)
        SDL_malloc((sizeof *this->hidden));
    if (this->hidden == NULL) {
        SDL_OutOfMemory();
        return 0;
    }
    SDL_memset(this->hidden, 0, (sizeof *this->hidden));

    /* Open the audio device */
    this->hidden->audio_fd = open(devname, flags, 0);
    if (this->hidden->audio_fd < 0) {
        DSP_CloseDevice(this);
        SDL_SetError("Couldn't open %s: %s", devname, strerror(errno));
        return 0;
    }
    this->hidden->mixbuf = NULL;

    /* Make the file descriptor use blocking writes with fcntl() */
    {
        long ctlflags;
        ctlflags = fcntl(this->hidden->audio_fd, F_GETFL);
        ctlflags &= ~O_NONBLOCK;
        if (fcntl(this->hidden->audio_fd, F_SETFL, ctlflags) < 0) {
            DSP_CloseDevice(this);
            SDL_SetError("Couldn't set audio blocking mode");
            return 0;
        }
    }

    /* Get a list of supported hardware formats */
    if (ioctl(this->hidden->audio_fd, SNDCTL_DSP_GETFMTS, &value) < 0) {
        perror("SNDCTL_DSP_GETFMTS");
        DSP_CloseDevice(this);
        SDL_SetError("Couldn't get audio format list");
        return 0;
    }

    /* Try for a closest match on audio format */
    format = 0;
    for (test_format = SDL_FirstAudioFormat(this->spec.format);
         !format && test_format;) {
#ifdef DEBUG_AUDIO
        fprintf(stderr, "Trying format 0x%4.4x\n", test_format);
#endif
        switch (test_format) {
        case AUDIO_U8:
            if (value & AFMT_U8) {
                format = AFMT_U8;
            }
            break;
        case AUDIO_S16LSB:
            if (value & AFMT_S16_LE) {
                format = AFMT_S16_LE;
            }
            break;
        case AUDIO_S16MSB:
            if (value & AFMT_S16_BE) {
                format = AFMT_S16_BE;
            }
            break;
#if 0
/*
 * These formats are not used by any real life systems so they are not 
 * needed here.
 */
        case AUDIO_S8:
            if (value & AFMT_S8) {
                format = AFMT_S8;
            }
            break;
        case AUDIO_U16LSB:
            if (value & AFMT_U16_LE) {
                format = AFMT_U16_LE;
            }
            break;
        case AUDIO_U16MSB:
            if (value & AFMT_U16_BE) {
                format = AFMT_U16_BE;
            }
            break;
#endif
        default:
            format = 0;
            break;
        }
        if (!format) {
            test_format = SDL_NextAudioFormat();
        }
    }
    if (format == 0) {
        DSP_CloseDevice(this);
        SDL_SetError("Couldn't find any hardware audio formats");
        return 0;
    }
    this->spec.format = test_format;

    /* Set the audio format */
    value = format;
    if ((ioctl(this->hidden->audio_fd, SNDCTL_DSP_SETFMT, &value) < 0) ||
        (value != format)) {
        perror("SNDCTL_DSP_SETFMT");
        DSP_CloseDevice(this);
        SDL_SetError("Couldn't set audio format");
        return 0;
    }

    /* Set the number of channels of output */
    value = this->spec.channels;
    if (ioctl(this->hidden->audio_fd, SNDCTL_DSP_CHANNELS, &value) < 0) {
        perror("SNDCTL_DSP_CHANNELS");
        DSP_CloseDevice(this);
        SDL_SetError("Cannot set the number of channels");
        return 0;
    }
    this->spec.channels = value;

    /* Set the DSP frequency */
    value = this->spec.freq;
    if (ioctl(this->hidden->audio_fd, SNDCTL_DSP_SPEED, &value) < 0) {
        perror("SNDCTL_DSP_SPEED");
        DSP_CloseDevice(this);
        SDL_SetError("Couldn't set audio frequency");
        return 0;
    }
    this->spec.freq = value;

    /* Calculate the final parameters for this audio specification */
    SDL_CalculateAudioSpec(&this->spec);

    /* Determine the power of two of the fragment size */
    for (frag_spec = 0; (0x01U << frag_spec) < this->spec.size; ++frag_spec);
    if ((0x01U << frag_spec) != this->spec.size) {
        DSP_CloseDevice(this);
        SDL_SetError("Fragment size must be a power of two");
        return 0;
    }
    frag_spec |= 0x00020000;    /* two fragments, for low latency */

    /* Set the audio buffering parameters */
#ifdef DEBUG_AUDIO
    fprintf(stderr, "Requesting %d fragments of size %d\n",
            (frag_spec >> 16), 1 << (frag_spec & 0xFFFF));
#endif
    if (ioctl(this->hidden->audio_fd, SNDCTL_DSP_SETFRAGMENT, &frag_spec) < 0) {
        perror("SNDCTL_DSP_SETFRAGMENT");
    }
#ifdef DEBUG_AUDIO
    {
        audio_buf_info info;
        ioctl(this->hidden->audio_fd, SNDCTL_DSP_GETOSPACE, &info);
        fprintf(stderr, "fragments = %d\n", info.fragments);
        fprintf(stderr, "fragstotal = %d\n", info.fragstotal);
        fprintf(stderr, "fragsize = %d\n", info.fragsize);
        fprintf(stderr, "bytes = %d\n", info.bytes);
    }
#endif

    /* Allocate mixing buffer */
    this->hidden->mixlen = this->spec.size;
    this->hidden->mixbuf = (Uint8 *) SDL_AllocAudioMem(this->hidden->mixlen);
    if (this->hidden->mixbuf == NULL) {
        DSP_CloseDevice(this);
        SDL_OutOfMemory();
        return 0;
    }
    SDL_memset(this->hidden->mixbuf, this->spec.silence, this->spec.size);

    /* We're ready to rock and roll. :-) */
    return 1;
}


static void
DSP_PlayDevice(_THIS)
{
    const Uint8 *mixbuf = this->hidden->mixbuf;
    const int mixlen = this->hidden->mixlen;
    if (write(this->hidden->audio_fd, mixbuf, mixlen) == -1) {
        perror("Audio write");
        this->enabled = 0;
    }
#ifdef DEBUG_AUDIO
    fprintf(stderr, "Wrote %d bytes of audio data\n", mixlen);
#endif
}

static Uint8 *
DSP_GetDeviceBuf(_THIS)
{
    return (this->hidden->mixbuf);
}

static int
DSP_Init(SDL_AudioDriverImpl * impl)
{
    /* Set the function pointers */
    impl->DetectDevices = DSP_DetectDevices;
    impl->GetDeviceName = DSP_GetDeviceName;
    impl->OpenDevice = DSP_OpenDevice;
    impl->PlayDevice = DSP_PlayDevice;
    impl->GetDeviceBuf = DSP_GetDeviceBuf;
    impl->CloseDevice = DSP_CloseDevice;
    impl->Deinitialize = DSP_Deinitialize;

    build_device_lists();

    return 1;   /* this audio target is available. */
}


AudioBootStrap DSP_bootstrap = {
    DSP_DRIVER_NAME, "OSS /dev/dsp standard audio", DSP_Init, 0
};

/* vi: set ts=4 sw=4 expandtab: */
