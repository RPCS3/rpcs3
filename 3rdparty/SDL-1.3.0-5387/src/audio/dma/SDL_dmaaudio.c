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

/* !!! FIXME: merge this driver with "dsp". */

/* Allow access to a raw mixing buffer */

#include <stdio.h>
#include <string.h>             /* For strerror() */
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/mman.h>

#if SDL_AUDIO_DRIVER_OSS_SOUNDCARD_H
/* This is installed on some systems */
#include <soundcard.h>
#else
/* This is recommended by OSS */
#include <sys/soundcard.h>
#endif

#ifndef MAP_FAILED
#define MAP_FAILED	((Uint8 *)-1)
#endif

#include "SDL_timer.h"
#include "SDL_audio.h"
#include "../SDL_audio_c.h"
#include "../SDL_audiodev_c.h"
#include "SDL_dmaaudio.h"

/* The tag name used by DMA audio */
#define DMA_DRIVER_NAME         "dma"

/* Open the audio device for playback, and don't block if busy */
#define OPEN_FLAGS_INPUT    (O_RDWR|O_NONBLOCK)
#define OPEN_FLAGS_OUTPUT   (O_RDWR|O_NONBLOCK)

static char **outputDevices = NULL;
static int outputDeviceCount = 0;
static char **inputDevices = NULL;
static int inputDeviceCount = 0;

static int
test_for_mmap(int fd)
{
    int caps = 0;
    struct audio_buf_info info;
    if ((ioctl(fd, SNDCTL_DSP_GETCAPS, &caps) == 0) &&
        (caps & DSP_CAP_TRIGGER) && (caps & DSP_CAP_MMAP) &&
        (ioctl(fd, SNDCTL_DSP_GETOSPACE, &info) == 0)) {
        size_t len = info.fragstotal * info.fragsize;
        Uint8 *buf = (Uint8 *) mmap(NULL, len, PROT_WRITE, MAP_SHARED, fd, 0);
        if (buf != MAP_FAILED) {
            munmap(buf, len);
            return 1;
        }
    }
    return 0;
}


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
    SDL_EnumUnixAudioDevices(flags, 0, test_for_mmap, devs, count);
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
DMA_Deinitialize(void)
{
    free_device_lists();
}

static int
DMA_DetectDevices(int iscapture)
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
DMA_GetDeviceName(int index, int iscapture)
{
    if ((iscapture) && (index < inputDeviceCount)) {
        return inputDevices[index];
    } else if ((!iscapture) && (index < outputDeviceCount)) {
        return outputDevices[index];
    }

    SDL_SetError("No such device");
    return NULL;
}


static int
DMA_ReopenAudio(_THIS, const char *audiodev, int format, int stereo)
{
    int frag_spec;
    int value;

    /* Close and then reopen the audio device */
    close(audio_fd);
    audio_fd = open(audiodev, O_RDWR, 0);
    if (audio_fd < 0) {
        SDL_SetError("Couldn't open %s: %s", audiodev, strerror(errno));
        return (-1);
    }

    /* Calculate the final parameters for this audio specification */
    SDL_CalculateAudioSpec(&this->spec);

    /* Determine the power of two of the fragment size */
    for (frag_spec = 0; (0x01 << frag_spec) < this->spec.size; ++frag_spec);
    if ((0x01 << frag_spec) != this->spec.size) {
        SDL_SetError("Fragment size must be a power of two");
        return (-1);
    }

    /* Set the audio buffering parameters */
    if (ioctl(audio_fd, SNDCTL_DSP_SETFRAGMENT, &frag_spec) < 0) {
        SDL_SetError("Couldn't set audio fragment spec");
        return (-1);
    }

    /* Set the audio format */
    value = format;
    if ((ioctl(audio_fd, SNDCTL_DSP_SETFMT, &value) < 0) || (value != format)) {
        SDL_SetError("Couldn't set audio format");
        return (-1);
    }

    /* Set mono or stereo audio */
    value = (this->spec.channels > 1);
    if ((ioctl(audio_fd, SNDCTL_DSP_STEREO, &stereo) < 0) ||
        (value != stereo)) {
        SDL_SetError("Couldn't set audio channels");
        return (-1);
    }

    /* Set the DSP frequency */
    value = this->spec.freq;
    if (ioctl(audio_fd, SNDCTL_DSP_SPEED, &value) < 0) {
        SDL_SetError("Couldn't set audio frequency");
        return (-1);
    }
    this->spec.freq = value;

    /* We successfully re-opened the audio */
    return (0);
}


static void
DMA_CloseDevice(_THIS)
{
    if (this->hidden != NULL) {
        if (dma_buf != NULL) {
            munmap(dma_buf, dma_len);
            dma_buf = NULL;
        }
        if (audio_fd >= 0) {
            close(audio_fd);
            audio_fd = -1;
        }
        SDL_free(this->hidden);
        this->hidden = NULL;
    }
}


static int
DMA_OpenDevice(_THIS, const char *devname, int iscapture)
{
    const int flags = ((iscapture) ? OPEN_FLAGS_INPUT : OPEN_FLAGS_OUTPUT);
    int format;
    int stereo;
    int value;
    SDL_AudioFormat test_format;
    struct audio_buf_info info;

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

    /* Initialize all variables that we clean on shutdown */
    this->hidden = (struct SDL_PrivateAudioData *)
        SDL_malloc((sizeof *this->hidden));
    if (this->hidden == NULL) {
        SDL_OutOfMemory();
        return 0;
    }
    SDL_memset(this->hidden, 0, (sizeof *this->hidden));

    /* Open the audio device */
    audio_fd = open(devname, flags, 0);
    if (audio_fd < 0) {
        DMA_CloseDevice(this);
        SDL_SetError("Couldn't open %s: %s", devname, strerror(errno));
        return 0;
    }
    dma_buf = NULL;
    ioctl(audio_fd, SNDCTL_DSP_RESET, 0);

    /* Get a list of supported hardware formats */
    if (ioctl(audio_fd, SNDCTL_DSP_GETFMTS, &value) < 0) {
        DMA_CloseDevice(this);
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
        case AUDIO_S8:
            if (value & AFMT_S8) {
                format = AFMT_S8;
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
        default:
            format = 0;
            break;
        }
        if (!format) {
            test_format = SDL_NextAudioFormat();
        }
    }
    if (format == 0) {
        DMA_CloseDevice(this);
        SDL_SetError("Couldn't find any hardware audio formats");
        return 0;
    }
    this->spec.format = test_format;

    /* Set the audio format */
    value = format;
    if ((ioctl(audio_fd, SNDCTL_DSP_SETFMT, &value) < 0) || (value != format)) {
        DMA_CloseDevice(this);
        SDL_SetError("Couldn't set audio format");
        return 0;
    }

    /* Set mono or stereo audio (currently only two channels supported) */
    stereo = (this->spec.channels > 1);
    ioctl(audio_fd, SNDCTL_DSP_STEREO, &stereo);
    if (stereo) {
        this->spec.channels = 2;
    } else {
        this->spec.channels = 1;
    }

    /* Because some drivers don't allow setting the buffer size
       after setting the format, we must re-open the audio device
       once we know what format and channels are supported
     */
    if (DMA_ReopenAudio(this, devname, format, stereo) < 0) {
        DMA_CloseDevice(this);
        /* Error is set by DMA_ReopenAudio() */
        return 0;
    }

    /* Memory map the audio buffer */
    if (ioctl(audio_fd, SNDCTL_DSP_GETOSPACE, &info) < 0) {
        DMA_CloseDevice(this);
        SDL_SetError("Couldn't get OSPACE parameters");
        return 0;
    }
    this->spec.size = info.fragsize;
    this->spec.samples = this->spec.size / ((this->spec.format & 0xFF) / 8);
    this->spec.samples /= this->spec.channels;
    num_buffers = info.fragstotal;
    dma_len = num_buffers * this->spec.size;
    dma_buf = (Uint8 *) mmap(NULL, dma_len, PROT_WRITE, MAP_SHARED,
                             audio_fd, 0);
    if (dma_buf == MAP_FAILED) {
        DMA_CloseDevice(this);
        SDL_SetError("DMA memory map failed");
        dma_buf = NULL;
        return 0;
    }
    SDL_memset(dma_buf, this->spec.silence, dma_len);

    /* Check to see if we need to use select() workaround */
    {
        char *workaround;
        workaround = SDL_getenv("SDL_DSP_NOSELECT");
        if (workaround) {
            frame_ticks =
                (float) (this->spec.samples * 1000) / this->spec.freq;
            next_frame = SDL_GetTicks() + frame_ticks;
        }
    }

    /* Trigger audio playback */
    value = 0;
    ioctl(audio_fd, SNDCTL_DSP_SETTRIGGER, &value);
    value = PCM_ENABLE_OUTPUT;
    if (ioctl(audio_fd, SNDCTL_DSP_SETTRIGGER, &value) < 0) {
        DMA_CloseDevice(this);
        SDL_SetError("Couldn't trigger audio output");
        return 0;
    }

    /* Get the parent process id (we're the parent of the audio thread) */
    parent = getpid();

    /* We're ready to rock and roll. :-) */
    return 1;
}


/* This function waits until it is possible to write a full sound buffer */
static void
DMA_WaitDevice(_THIS)
{
    fd_set fdset;

    /* Check to see if the thread-parent process is still alive */
    {
        static int cnt = 0;
        /* Note that this only works with thread implementations 
           that use a different process id for each thread.
         */
        if (parent && (((++cnt) % 10) == 0)) {  /* Check every 10 loops */
            if (kill(parent, 0) < 0 && errno == ESRCH) {
                this->enabled = 0;
            }
        }
    }

    /* See if we need to use timed audio synchronization */
    if (frame_ticks) {
        /* Use timer for general audio synchronization */
        Sint32 ticks;

        ticks = ((Sint32) (next_frame - SDL_GetTicks())) - FUDGE_TICKS;
        if (ticks > 0) {
            SDL_Delay(ticks);
        }
    } else {
        /* Use select() for audio synchronization */
        struct timeval timeout;
        FD_ZERO(&fdset);
        FD_SET(audio_fd, &fdset);
        timeout.tv_sec = 10;
        timeout.tv_usec = 0;
#ifdef DEBUG_AUDIO
        fprintf(stderr, "Waiting for audio to get ready\n");
#endif
        if (select(audio_fd + 1, NULL, &fdset, NULL, &timeout) <= 0) {
            const char *message =
#ifdef AUDIO_OSPACE_HACK
                "Audio timeout - buggy audio driver? (trying ospace)";
#else
                "Audio timeout - buggy audio driver? (disabled)";
#endif
            /* In general we should never print to the screen,
               but in this case we have no other way of letting
               the user know what happened.
             */
            fprintf(stderr, "SDL: %s\n", message);
#ifdef AUDIO_OSPACE_HACK
            /* We may be able to use GET_OSPACE trick */
            frame_ticks = (float) (this->spec.samples * 1000) /
                this->spec.freq;
            next_frame = SDL_GetTicks() + frame_ticks;
#else
            this->enabled = 0;
            /* Don't try to close - may hang */
            audio_fd = -1;
#ifdef DEBUG_AUDIO
            fprintf(stderr, "Done disabling audio\n");
#endif
#endif /* AUDIO_OSPACE_HACK */
        }
#ifdef DEBUG_AUDIO
        fprintf(stderr, "Ready!\n");
#endif
    }
}

static void
DMA_PlayDevice(_THIS)
{
    /* If timer synchronization is enabled, set the next write frame */
    if (frame_ticks) {
        next_frame += frame_ticks;
    }
    return;
}

static Uint8 *
DMA_GetDeviceBuf(_THIS)
{
    count_info info;
    int playing;
    int filling;

    /* Get number of blocks, looping if we're not using select() */
    do {
        if (ioctl(audio_fd, SNDCTL_DSP_GETOPTR, &info) < 0) {
            /* Uh oh... */
            this->enabled = 0;
            return (NULL);
        }
    } while (frame_ticks && (info.blocks < 1));
#ifdef DEBUG_AUDIO
    if (info.blocks > 1) {
        printf("Warning: audio underflow (%d frags)\n", info.blocks - 1);
    }
#endif
    playing = info.ptr / this->spec.size;
    filling = (playing + 1) % num_buffers;
    return (dma_buf + (filling * this->spec.size));
}


static int
DMA_Init(SDL_AudioDriverImpl * impl)
{
    /* Set the function pointers */
    impl->DetectDevices = DMA_DetectDevices;
    impl->GetDeviceName = DMA_GetDeviceName;
    impl->OpenDevice = DMA_OpenDevice;
    impl->WaitDevice = DMA_WaitDevice;
    impl->PlayDevice = DMA_PlayDevice;
    impl->GetDeviceBuf = DMA_GetDeviceBuf;
    impl->CloseDevice = DMA_CloseDevice;
    impl->Deinitialize = DMA_Deinitialize;

    build_device_lists();

    return 1;   /* this audio target is available. */
}

AudioBootStrap DMA_bootstrap = {
    DMA_DRIVER_NAME, "OSS /dev/dsp DMA audio", DMA_Init, 0
};

/* vi: set ts=4 sw=4 expandtab: */
