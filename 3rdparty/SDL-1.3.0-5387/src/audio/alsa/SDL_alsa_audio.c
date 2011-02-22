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

#include <sys/types.h>
#include <signal.h>             /* For kill() */
#include <errno.h>
#include <string.h>

#include "SDL_timer.h"
#include "SDL_audio.h"
#include "../SDL_audiomem.h"
#include "../SDL_audio_c.h"
#include "SDL_alsa_audio.h"

#ifdef SDL_AUDIO_DRIVER_ALSA_DYNAMIC
#include "SDL_loadso.h"
#endif

/* The tag name used by ALSA audio */
#define DRIVER_NAME         "alsa"

static int (*ALSA_snd_pcm_open)
  (snd_pcm_t **, const char *, snd_pcm_stream_t, int);
static int (*ALSA_snd_pcm_close) (snd_pcm_t * pcm);
static snd_pcm_sframes_t(*ALSA_snd_pcm_writei)
  (snd_pcm_t *, const void *, snd_pcm_uframes_t);
static int (*ALSA_snd_pcm_recover) (snd_pcm_t *, int, int);
static int (*ALSA_snd_pcm_prepare) (snd_pcm_t *);
static int (*ALSA_snd_pcm_drain) (snd_pcm_t *);
static const char *(*ALSA_snd_strerror) (int);
static size_t(*ALSA_snd_pcm_hw_params_sizeof) (void);
static size_t(*ALSA_snd_pcm_sw_params_sizeof) (void);
static void (*ALSA_snd_pcm_hw_params_copy)
  (snd_pcm_hw_params_t *, const snd_pcm_hw_params_t *);
static int (*ALSA_snd_pcm_hw_params_any) (snd_pcm_t *, snd_pcm_hw_params_t *);
static int (*ALSA_snd_pcm_hw_params_set_access)
  (snd_pcm_t *, snd_pcm_hw_params_t *, snd_pcm_access_t);
static int (*ALSA_snd_pcm_hw_params_set_format)
  (snd_pcm_t *, snd_pcm_hw_params_t *, snd_pcm_format_t);
static int (*ALSA_snd_pcm_hw_params_set_channels)
  (snd_pcm_t *, snd_pcm_hw_params_t *, unsigned int);
static int (*ALSA_snd_pcm_hw_params_get_channels)
  (const snd_pcm_hw_params_t *, unsigned int *);
static int (*ALSA_snd_pcm_hw_params_set_rate_near)
  (snd_pcm_t *, snd_pcm_hw_params_t *, unsigned int *, int *);
static int (*ALSA_snd_pcm_hw_params_set_period_size_near)
  (snd_pcm_t *, snd_pcm_hw_params_t *, snd_pcm_uframes_t *, int *);
static int (*ALSA_snd_pcm_hw_params_get_period_size)
  (const snd_pcm_hw_params_t *, snd_pcm_uframes_t *, int *);
static int (*ALSA_snd_pcm_hw_params_set_periods_near)
  (snd_pcm_t *, snd_pcm_hw_params_t *, unsigned int *, int *);
static int (*ALSA_snd_pcm_hw_params_get_periods)
  (const snd_pcm_hw_params_t *, unsigned int *, int *);
static int (*ALSA_snd_pcm_hw_params_set_buffer_size_near)
  (snd_pcm_t *pcm, snd_pcm_hw_params_t *, snd_pcm_uframes_t *);
static int (*ALSA_snd_pcm_hw_params_get_buffer_size)
  (const snd_pcm_hw_params_t *, snd_pcm_uframes_t *);
static int (*ALSA_snd_pcm_hw_params) (snd_pcm_t *, snd_pcm_hw_params_t *);
static int (*ALSA_snd_pcm_sw_params_current) (snd_pcm_t *,
                                              snd_pcm_sw_params_t *);
static int (*ALSA_snd_pcm_sw_params_set_start_threshold)
  (snd_pcm_t *, snd_pcm_sw_params_t *, snd_pcm_uframes_t);
static int (*ALSA_snd_pcm_sw_params) (snd_pcm_t *, snd_pcm_sw_params_t *);
static int (*ALSA_snd_pcm_nonblock) (snd_pcm_t *, int);
static int (*ALSA_snd_pcm_wait)(snd_pcm_t *, int);


#ifdef SDL_AUDIO_DRIVER_ALSA_DYNAMIC
#define snd_pcm_hw_params_sizeof ALSA_snd_pcm_hw_params_sizeof
#define snd_pcm_sw_params_sizeof ALSA_snd_pcm_sw_params_sizeof

static const char *alsa_library = SDL_AUDIO_DRIVER_ALSA_DYNAMIC;
static void *alsa_handle = NULL;

static int
load_alsa_sym(const char *fn, void **addr)
{
    *addr = SDL_LoadFunction(alsa_handle, fn);
    if (*addr == NULL) {
        /* Don't call SDL_SetError(): SDL_LoadFunction already did. */
        return 0;
    }

    return 1;
}

/* cast funcs to char* first, to please GCC's strict aliasing rules. */
#define SDL_ALSA_SYM(x) \
    if (!load_alsa_sym(#x, (void **) (char *) &ALSA_##x)) return -1
#else
#define SDL_ALSA_SYM(x) ALSA_##x = x
#endif

static int
load_alsa_syms(void)
{
    SDL_ALSA_SYM(snd_pcm_open);
    SDL_ALSA_SYM(snd_pcm_close);
    SDL_ALSA_SYM(snd_pcm_writei);
    SDL_ALSA_SYM(snd_pcm_recover);
    SDL_ALSA_SYM(snd_pcm_prepare);
    SDL_ALSA_SYM(snd_pcm_drain);
    SDL_ALSA_SYM(snd_strerror);
    SDL_ALSA_SYM(snd_pcm_hw_params_sizeof);
    SDL_ALSA_SYM(snd_pcm_sw_params_sizeof);
    SDL_ALSA_SYM(snd_pcm_hw_params_copy);
    SDL_ALSA_SYM(snd_pcm_hw_params_any);
    SDL_ALSA_SYM(snd_pcm_hw_params_set_access);
    SDL_ALSA_SYM(snd_pcm_hw_params_set_format);
    SDL_ALSA_SYM(snd_pcm_hw_params_set_channels);
    SDL_ALSA_SYM(snd_pcm_hw_params_get_channels);
    SDL_ALSA_SYM(snd_pcm_hw_params_set_rate_near);
    SDL_ALSA_SYM(snd_pcm_hw_params_set_period_size_near);
    SDL_ALSA_SYM(snd_pcm_hw_params_get_period_size);
    SDL_ALSA_SYM(snd_pcm_hw_params_set_periods_near);
    SDL_ALSA_SYM(snd_pcm_hw_params_get_periods);
    SDL_ALSA_SYM(snd_pcm_hw_params_set_buffer_size_near);
    SDL_ALSA_SYM(snd_pcm_hw_params_get_buffer_size);
    SDL_ALSA_SYM(snd_pcm_hw_params);
    SDL_ALSA_SYM(snd_pcm_sw_params_current);
    SDL_ALSA_SYM(snd_pcm_sw_params_set_start_threshold);
    SDL_ALSA_SYM(snd_pcm_sw_params);
    SDL_ALSA_SYM(snd_pcm_nonblock);
    SDL_ALSA_SYM(snd_pcm_wait);
    return 0;
}

#undef SDL_ALSA_SYM

#ifdef SDL_AUDIO_DRIVER_ALSA_DYNAMIC

static void
UnloadALSALibrary(void)
{
    if (alsa_handle != NULL) {
		SDL_UnloadObject(alsa_handle);
        alsa_handle = NULL;
    }
}

static int
LoadALSALibrary(void)
{
    int retval = 0;
    if (alsa_handle == NULL) {
        alsa_handle = SDL_LoadObject(alsa_library);
        if (alsa_handle == NULL) {
            retval = -1;
            /* Don't call SDL_SetError(): SDL_LoadObject already did. */
        } else {
            retval = load_alsa_syms();
            if (retval < 0) {
                UnloadALSALibrary();
            }
        }
    }
    return retval;
}

#else

static void
UnloadALSALibrary(void)
{
}

static int
LoadALSALibrary(void)
{
    load_alsa_syms();
    return 0;
}

#endif /* SDL_AUDIO_DRIVER_ALSA_DYNAMIC */

static const char *
get_audio_device(int channels)
{
    const char *device;

    device = SDL_getenv("AUDIODEV");    /* Is there a standard variable name? */
    if (device == NULL) {
        switch (channels) {
        case 6:
            device = "plug:surround51";
            break;
        case 4:
            device = "plug:surround40";
            break;
        default:
            device = "default";
            break;
        }
    }
    return device;
}


/* This function waits until it is possible to write a full sound buffer */
static void
ALSA_WaitDevice(_THIS)
{
    /* We're in blocking mode, so there's nothing to do here */
}


/* !!! FIXME: is there a channel swizzler in alsalib instead? */
/*
 * http://bugzilla.libsdl.org/show_bug.cgi?id=110
 * "For Linux ALSA, this is FL-FR-RL-RR-C-LFE
 *  and for Windows DirectX [and CoreAudio], this is FL-FR-C-LFE-RL-RR"
 */
#define SWIZ6(T) \
    T *ptr = (T *) this->hidden->mixbuf; \
    Uint32 i; \
    for (i = 0; i < this->spec.samples; i++, ptr += 6) { \
        T tmp; \
        tmp = ptr[2]; ptr[2] = ptr[4]; ptr[4] = tmp; \
        tmp = ptr[3]; ptr[3] = ptr[5]; ptr[5] = tmp; \
    }

static __inline__ void
swizzle_alsa_channels_6_64bit(_THIS)
{
    SWIZ6(Uint64);
}

static __inline__ void
swizzle_alsa_channels_6_32bit(_THIS)
{
    SWIZ6(Uint32);
}

static __inline__ void
swizzle_alsa_channels_6_16bit(_THIS)
{
    SWIZ6(Uint16);
}

static __inline__ void
swizzle_alsa_channels_6_8bit(_THIS)
{
    SWIZ6(Uint8);
}

#undef SWIZ6


/*
 * Called right before feeding this->hidden->mixbuf to the hardware. Swizzle
 *  channels from Windows/Mac order to the format alsalib will want.
 */
static __inline__ void
swizzle_alsa_channels(_THIS)
{
    if (this->spec.channels == 6) {
        const Uint16 fmtsize = (this->spec.format & 0xFF);      /* bits/channel. */
        if (fmtsize == 16)
            swizzle_alsa_channels_6_16bit(this);
        else if (fmtsize == 8)
            swizzle_alsa_channels_6_8bit(this);
        else if (fmtsize == 32)
            swizzle_alsa_channels_6_32bit(this);
        else if (fmtsize == 64)
            swizzle_alsa_channels_6_64bit(this);
    }

    /* !!! FIXME: update this for 7.1 if needed, later. */
}


static void
ALSA_PlayDevice(_THIS)
{
    int status;
    const Uint8 *sample_buf = (const Uint8 *) this->hidden->mixbuf;
    const int frame_size = (((int) (this->spec.format & 0xFF)) / 8) *
                                this->spec.channels;
    snd_pcm_uframes_t frames_left = ((snd_pcm_uframes_t) this->spec.samples);

    swizzle_alsa_channels(this);

    while ( frames_left > 0 && this->enabled ) {
        /* !!! FIXME: This works, but needs more testing before going live */
        /*ALSA_snd_pcm_wait(this->hidden->pcm_handle, -1);*/
        status = ALSA_snd_pcm_writei(this->hidden->pcm_handle,
                                     sample_buf, frames_left);

        if (status < 0) {
            if (status == -EAGAIN) {
                /* Apparently snd_pcm_recover() doesn't handle this case -
                   does it assume snd_pcm_wait() above? */
                SDL_Delay(1);
                continue;
            }
            status = ALSA_snd_pcm_recover(this->hidden->pcm_handle, status, 0);
            if (status < 0) {
                /* Hmm, not much we can do - abort */
                fprintf(stderr, "ALSA write failed (unrecoverable): %s\n",
                        ALSA_snd_strerror(status));
                this->enabled = 0;
                return;
            }
            continue;
        }
        sample_buf += status * frame_size;
        frames_left -= status;
    }
}

static Uint8 *
ALSA_GetDeviceBuf(_THIS)
{
    return (this->hidden->mixbuf);
}

static void
ALSA_CloseDevice(_THIS)
{
    if (this->hidden != NULL) {
        if (this->hidden->mixbuf != NULL) {
            SDL_FreeAudioMem(this->hidden->mixbuf);
            this->hidden->mixbuf = NULL;
        }
        if (this->hidden->pcm_handle) {
            ALSA_snd_pcm_drain(this->hidden->pcm_handle);
            ALSA_snd_pcm_close(this->hidden->pcm_handle);
            this->hidden->pcm_handle = NULL;
        }
        SDL_free(this->hidden);
        this->hidden = NULL;
    }
}

static int
ALSA_finalize_hardware(_THIS, snd_pcm_hw_params_t *hwparams, int override)
{
    int status;
    snd_pcm_uframes_t bufsize;

    /* "set" the hardware with the desired parameters */
    status = ALSA_snd_pcm_hw_params(this->hidden->pcm_handle, hwparams);
    if ( status < 0 ) {
        return(-1);
    }

    /* Get samples for the actual buffer size */
    status = ALSA_snd_pcm_hw_params_get_buffer_size(hwparams, &bufsize);
    if ( status < 0 ) {
        return(-1);
    }
    if ( !override && bufsize != this->spec.samples * 2 ) {
        return(-1);
    }

    /* !!! FIXME: Is this safe to do? */
    this->spec.samples = bufsize / 2;

    /* This is useful for debugging */
    if ( SDL_getenv("SDL_AUDIO_ALSA_DEBUG") ) {
        snd_pcm_uframes_t persize = 0;
        unsigned int periods = 0;

        ALSA_snd_pcm_hw_params_get_period_size(hwparams, &persize, NULL);
        ALSA_snd_pcm_hw_params_get_periods(hwparams, &periods, NULL);

        fprintf(stderr,
            "ALSA: period size = %ld, periods = %u, buffer size = %lu\n",
            persize, periods, bufsize);
    }

    return(0);
}

static int
ALSA_set_period_size(_THIS, snd_pcm_hw_params_t *params, int override)
{
    const char *env;
    int status;
    snd_pcm_hw_params_t *hwparams;
    snd_pcm_uframes_t frames;
    unsigned int periods;

    /* Copy the hardware parameters for this setup */
    snd_pcm_hw_params_alloca(&hwparams);
    ALSA_snd_pcm_hw_params_copy(hwparams, params);

    if ( !override ) {
        env = SDL_getenv("SDL_AUDIO_ALSA_SET_PERIOD_SIZE");
        if ( env ) {
            override = SDL_atoi(env);
            if ( override == 0 ) {
                return(-1);
            }
        }
    }

    frames = this->spec.samples;
    status = ALSA_snd_pcm_hw_params_set_period_size_near(
                this->hidden->pcm_handle, hwparams, &frames, NULL);
    if ( status < 0 ) {
        return(-1);
    }

    periods = 2;
    status = ALSA_snd_pcm_hw_params_set_periods_near(
                this->hidden->pcm_handle, hwparams, &periods, NULL);
    if ( status < 0 ) {
        return(-1);
    }

    return ALSA_finalize_hardware(this, hwparams, override);
}

static int
ALSA_set_buffer_size(_THIS, snd_pcm_hw_params_t *params, int override)
{
    const char *env;
    int status;
    snd_pcm_hw_params_t *hwparams;
    snd_pcm_uframes_t frames;

    /* Copy the hardware parameters for this setup */
    snd_pcm_hw_params_alloca(&hwparams);
    ALSA_snd_pcm_hw_params_copy(hwparams, params);

    if ( !override ) {
        env = SDL_getenv("SDL_AUDIO_ALSA_SET_BUFFER_SIZE");
        if ( env ) {
            override = SDL_atoi(env);
            if ( override == 0 ) {
                return(-1);
            }
        }
    }

    frames = this->spec.samples * 2;
    status = ALSA_snd_pcm_hw_params_set_buffer_size_near(
                    this->hidden->pcm_handle, hwparams, &frames);
    if ( status < 0 ) {
        return(-1);
    }

    return ALSA_finalize_hardware(this, hwparams, override);
}

static int
ALSA_OpenDevice(_THIS, const char *devname, int iscapture)
{
    int status = 0;
    snd_pcm_t *pcm_handle = NULL;
    snd_pcm_hw_params_t *hwparams = NULL;
    snd_pcm_sw_params_t *swparams = NULL;
    snd_pcm_format_t format = 0;
    SDL_AudioFormat test_format = 0;
    unsigned int rate = 0;
    unsigned int channels = 0;

    /* Initialize all variables that we clean on shutdown */
    this->hidden = (struct SDL_PrivateAudioData *)
        SDL_malloc((sizeof *this->hidden));
    if (this->hidden == NULL) {
        SDL_OutOfMemory();
        return 0;
    }
    SDL_memset(this->hidden, 0, (sizeof *this->hidden));

    /* Open the audio device */
    /* Name of device should depend on # channels in spec */
    status = ALSA_snd_pcm_open(&pcm_handle,
                               get_audio_device(this->spec.channels),
                               SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK);

    if (status < 0) {
        ALSA_CloseDevice(this);
        SDL_SetError("ALSA: Couldn't open audio device: %s",
                     ALSA_snd_strerror(status));
        return 0;
    }

    this->hidden->pcm_handle = pcm_handle;

    /* Figure out what the hardware is capable of */
    snd_pcm_hw_params_alloca(&hwparams);
    status = ALSA_snd_pcm_hw_params_any(pcm_handle, hwparams);
    if (status < 0) {
        ALSA_CloseDevice(this);
        SDL_SetError("ALSA: Couldn't get hardware config: %s",
                     ALSA_snd_strerror(status));
        return 0;
    }

    /* SDL only uses interleaved sample output */
    status = ALSA_snd_pcm_hw_params_set_access(pcm_handle, hwparams,
                                               SND_PCM_ACCESS_RW_INTERLEAVED);
    if (status < 0) {
        ALSA_CloseDevice(this);
        SDL_SetError("ALSA: Couldn't set interleaved access: %s",
                     ALSA_snd_strerror(status));
        return 0;
    }

    /* Try for a closest match on audio format */
    status = -1;
    for (test_format = SDL_FirstAudioFormat(this->spec.format);
         test_format && (status < 0);) {
        status = 0;             /* if we can't support a format, it'll become -1. */
        switch (test_format) {
        case AUDIO_U8:
            format = SND_PCM_FORMAT_U8;
            break;
        case AUDIO_S8:
            format = SND_PCM_FORMAT_S8;
            break;
        case AUDIO_S16LSB:
            format = SND_PCM_FORMAT_S16_LE;
            break;
        case AUDIO_S16MSB:
            format = SND_PCM_FORMAT_S16_BE;
            break;
        case AUDIO_U16LSB:
            format = SND_PCM_FORMAT_U16_LE;
            break;
        case AUDIO_U16MSB:
            format = SND_PCM_FORMAT_U16_BE;
            break;
        case AUDIO_S32LSB:
            format = SND_PCM_FORMAT_S32_LE;
            break;
        case AUDIO_S32MSB:
            format = SND_PCM_FORMAT_S32_BE;
            break;
        case AUDIO_F32LSB:
            format = SND_PCM_FORMAT_FLOAT_LE;
            break;
        case AUDIO_F32MSB:
            format = SND_PCM_FORMAT_FLOAT_BE;
            break;
        default:
            status = -1;
            break;
        }
        if (status >= 0) {
            status = ALSA_snd_pcm_hw_params_set_format(pcm_handle,
                                                       hwparams, format);
        }
        if (status < 0) {
            test_format = SDL_NextAudioFormat();
        }
    }
    if (status < 0) {
        ALSA_CloseDevice(this);
        SDL_SetError("ALSA: Couldn't find any hardware audio formats");
        return 0;
    }
    this->spec.format = test_format;

    /* Set the number of channels */
    status = ALSA_snd_pcm_hw_params_set_channels(pcm_handle, hwparams,
                                                 this->spec.channels);
    channels = this->spec.channels;
    if (status < 0) {
        status = ALSA_snd_pcm_hw_params_get_channels(hwparams, &channels);
        if (status < 0) {
            ALSA_CloseDevice(this);
            SDL_SetError("ALSA: Couldn't set audio channels");
            return 0;
        }
        this->spec.channels = channels;
    }

    /* Set the audio rate */
    rate = this->spec.freq;
    status = ALSA_snd_pcm_hw_params_set_rate_near(pcm_handle, hwparams,
                                                  &rate, NULL);
    if (status < 0) {
        ALSA_CloseDevice(this);
        SDL_SetError("ALSA: Couldn't set audio frequency: %s",
                     ALSA_snd_strerror(status));
        return 0;
    }
    this->spec.freq = rate;

    /* Set the buffer size, in samples */
    if ( ALSA_set_period_size(this, hwparams, 0) < 0 &&
         ALSA_set_buffer_size(this, hwparams, 0) < 0 ) {
        /* Failed to set desired buffer size, do the best you can... */
        if ( ALSA_set_period_size(this, hwparams, 1) < 0 ) {
            ALSA_CloseDevice(this);
            SDL_SetError("Couldn't set hardware audio parameters: %s", ALSA_snd_strerror(status));
            return(-1);
        }
    }
    /* Set the software parameters */
    snd_pcm_sw_params_alloca(&swparams);
    status = ALSA_snd_pcm_sw_params_current(pcm_handle, swparams);
    if (status < 0) {
        ALSA_CloseDevice(this);
        SDL_SetError("ALSA: Couldn't get software config: %s",
                     ALSA_snd_strerror(status));
        return 0;
    }
    status =
        ALSA_snd_pcm_sw_params_set_start_threshold(pcm_handle, swparams, 1);
    if (status < 0) {
        ALSA_CloseDevice(this);
        SDL_SetError("ALSA: Couldn't set start threshold: %s",
                     ALSA_snd_strerror(status));
        return 0;
    }
    status = ALSA_snd_pcm_sw_params(pcm_handle, swparams);
    if (status < 0) {
        ALSA_CloseDevice(this);
        SDL_SetError("Couldn't set software audio parameters: %s",
                     ALSA_snd_strerror(status));
        return 0;
    }

    /* Calculate the final parameters for this audio specification */
    SDL_CalculateAudioSpec(&this->spec);

    /* Allocate mixing buffer */
    this->hidden->mixlen = this->spec.size;
    this->hidden->mixbuf = (Uint8 *) SDL_AllocAudioMem(this->hidden->mixlen);
    if (this->hidden->mixbuf == NULL) {
        ALSA_CloseDevice(this);
        SDL_OutOfMemory();
        return 0;
    }
    SDL_memset(this->hidden->mixbuf, this->spec.silence, this->spec.size);

    /* Switch to blocking mode for playback */
    ALSA_snd_pcm_nonblock(pcm_handle, 0);

    /* We're ready to rock and roll. :-) */
    return 1;
}

static void
ALSA_Deinitialize(void)
{
    UnloadALSALibrary();
}

static int
ALSA_Init(SDL_AudioDriverImpl * impl)
{
    if (LoadALSALibrary() < 0) {
        return 0;
    }

    /* Set the function pointers */
    impl->OpenDevice = ALSA_OpenDevice;
    impl->WaitDevice = ALSA_WaitDevice;
    impl->GetDeviceBuf = ALSA_GetDeviceBuf;
    impl->PlayDevice = ALSA_PlayDevice;
    impl->CloseDevice = ALSA_CloseDevice;
    impl->Deinitialize = ALSA_Deinitialize;
    impl->OnlyHasDefaultOutputDevice = 1;       /* !!! FIXME: Add device enum! */

    return 1;   /* this audio target is available. */
}


AudioBootStrap ALSA_bootstrap = {
    DRIVER_NAME, "ALSA PCM audio", ALSA_Init, 0
};

/* vi: set ts=4 sw=4 expandtab: */
