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

#include "SDL.h"
#include "SDL_audio.h"
#include "SDL_audio_c.h"
#include "SDL_audiomem.h"
#include "SDL_sysaudio.h"

#define _THIS SDL_AudioDevice *_this

static SDL_AudioDriver current_audio;
static SDL_AudioDevice *open_devices[16];

/* !!! FIXME: These are wordy and unlocalized... */
#define DEFAULT_OUTPUT_DEVNAME "System audio output device"
#define DEFAULT_INPUT_DEVNAME "System audio capture device"


/*
 * Not all of these will be compiled and linked in, but it's convenient
 *  to have a complete list here and saves yet-another block of #ifdefs...
 *  Please see bootstrap[], below, for the actual #ifdef mess.
 */
extern AudioBootStrap BSD_AUDIO_bootstrap;
extern AudioBootStrap DSP_bootstrap;
extern AudioBootStrap DMA_bootstrap;
extern AudioBootStrap ALSA_bootstrap;
extern AudioBootStrap PULSEAUDIO_bootstrap;
extern AudioBootStrap QSAAUDIO_bootstrap;
extern AudioBootStrap SUNAUDIO_bootstrap;
extern AudioBootStrap DMEDIA_bootstrap;
extern AudioBootStrap ARTS_bootstrap;
extern AudioBootStrap ESD_bootstrap;
extern AudioBootStrap NAS_bootstrap;
extern AudioBootStrap DSOUND_bootstrap;
extern AudioBootStrap WINWAVEOUT_bootstrap;
extern AudioBootStrap PAUDIO_bootstrap;
extern AudioBootStrap BEOSAUDIO_bootstrap;
extern AudioBootStrap COREAUDIO_bootstrap;
extern AudioBootStrap COREAUDIOIPHONE_bootstrap;
extern AudioBootStrap SNDMGR_bootstrap;
extern AudioBootStrap DISKAUD_bootstrap;
extern AudioBootStrap DUMMYAUD_bootstrap;
extern AudioBootStrap DCAUD_bootstrap;
extern AudioBootStrap MMEAUDIO_bootstrap;
extern AudioBootStrap DART_bootstrap;
extern AudioBootStrap NDSAUD_bootstrap;
extern AudioBootStrap FUSIONSOUND_bootstrap;
extern AudioBootStrap ANDROIDAUD_bootstrap;


/* Available audio drivers */
static const AudioBootStrap *const bootstrap[] = {
#if SDL_AUDIO_DRIVER_PULSEAUDIO
    &PULSEAUDIO_bootstrap,
#endif
#if SDL_AUDIO_DRIVER_ALSA
    &ALSA_bootstrap,
#endif
#if SDL_AUDIO_DRIVER_BSD
    &BSD_AUDIO_bootstrap,
#endif
#if SDL_AUDIO_DRIVER_OSS
    &DSP_bootstrap,
    &DMA_bootstrap,
#endif
#if SDL_AUDIO_DRIVER_QSA
    &QSAAUDIO_bootstrap,
#endif
#if SDL_AUDIO_DRIVER_SUNAUDIO
    &SUNAUDIO_bootstrap,
#endif
#if SDL_AUDIO_DRIVER_DMEDIA
    &DMEDIA_bootstrap,
#endif
#if SDL_AUDIO_DRIVER_ARTS
    &ARTS_bootstrap,
#endif
#if SDL_AUDIO_DRIVER_ESD
    &ESD_bootstrap,
#endif
#if SDL_AUDIO_DRIVER_NAS
    &NAS_bootstrap,
#endif
#if SDL_AUDIO_DRIVER_DSOUND
    &DSOUND_bootstrap,
#endif
#if SDL_AUDIO_DRIVER_WINWAVEOUT
    &WINWAVEOUT_bootstrap,
#endif
#if SDL_AUDIO_DRIVER_PAUDIO
    &PAUDIO_bootstrap,
#endif
#if SDL_AUDIO_DRIVER_BEOSAUDIO
    &BEOSAUDIO_bootstrap,
#endif
#if SDL_AUDIO_DRIVER_COREAUDIO
    &COREAUDIO_bootstrap,
#endif
#if SDL_AUDIO_DRIVER_COREAUDIOIPHONE
    &COREAUDIOIPHONE_bootstrap,
#endif
#if SDL_AUDIO_DRIVER_DISK
    &DISKAUD_bootstrap,
#endif
#if SDL_AUDIO_DRIVER_DUMMY
    &DUMMYAUD_bootstrap,
#endif
#if SDL_AUDIO_DRIVER_MMEAUDIO
    &MMEAUDIO_bootstrap,
#endif
#if SDL_AUDIO_DRIVER_NDS
    &NDSAUD_bootstrap,
#endif
#if SDL_AUDIO_DRIVER_FUSIONSOUND
    &FUSIONSOUND_bootstrap,
#endif
#if SDL_AUDIO_DRIVER_ANDROID
    &ANDROIDAUD_bootstrap,
#endif
    NULL
};

static SDL_AudioDevice *
get_audio_device(SDL_AudioDeviceID id)
{
    id--;
    if ((id >= SDL_arraysize(open_devices)) || (open_devices[id] == NULL)) {
        SDL_SetError("Invalid audio device ID");
        return NULL;
    }

    return open_devices[id];
}


/* stubs for audio drivers that don't need a specific entry point... */
static int
SDL_AudioDetectDevices_Default(int iscapture)
{
    return -1;
}

static void
SDL_AudioThreadInit_Default(_THIS)
{                               /* no-op. */
}

static void
SDL_AudioWaitDevice_Default(_THIS)
{                               /* no-op. */
}

static void
SDL_AudioPlayDevice_Default(_THIS)
{                               /* no-op. */
}

static Uint8 *
SDL_AudioGetDeviceBuf_Default(_THIS)
{
    return NULL;
}

static void
SDL_AudioWaitDone_Default(_THIS)
{                               /* no-op. */
}

static void
SDL_AudioCloseDevice_Default(_THIS)
{                               /* no-op. */
}

static void
SDL_AudioDeinitialize_Default(void)
{                               /* no-op. */
}

static int
SDL_AudioOpenDevice_Default(_THIS, const char *devname, int iscapture)
{
    return 0;
}

static const char *
SDL_AudioGetDeviceName_Default(int index, int iscapture)
{
    SDL_SetError("No such device");
    return NULL;
}

static void
SDL_AudioLockDevice_Default(SDL_AudioDevice * device)
{
    if (device->thread && (SDL_ThreadID() == device->threadid)) {
        return;
    }
    SDL_mutexP(device->mixer_lock);
}

static void
SDL_AudioUnlockDevice_Default(SDL_AudioDevice * device)
{
    if (device->thread && (SDL_ThreadID() == device->threadid)) {
        return;
    }
    SDL_mutexV(device->mixer_lock);
}


static void
finalize_audio_entry_points(void)
{
    /*
     * Fill in stub functions for unused driver entry points. This lets us
     *  blindly call them without having to check for validity first.
     */

#define FILL_STUB(x) \
        if (current_audio.impl.x == NULL) { \
            current_audio.impl.x = SDL_Audio##x##_Default; \
        }
    FILL_STUB(DetectDevices);
    FILL_STUB(GetDeviceName);
    FILL_STUB(OpenDevice);
    FILL_STUB(ThreadInit);
    FILL_STUB(WaitDevice);
    FILL_STUB(PlayDevice);
    FILL_STUB(GetDeviceBuf);
    FILL_STUB(WaitDone);
    FILL_STUB(CloseDevice);
    FILL_STUB(LockDevice);
    FILL_STUB(UnlockDevice);
    FILL_STUB(Deinitialize);
#undef FILL_STUB
}

/* Streaming functions (for when the input and output buffer sizes are different) */
/* Write [length] bytes from buf into the streamer */
static void
SDL_StreamWrite(SDL_AudioStreamer * stream, Uint8 * buf, int length)
{
    int i;

    for (i = 0; i < length; ++i) {
        stream->buffer[stream->write_pos] = buf[i];
        ++stream->write_pos;
    }
}

/* Read [length] bytes out of the streamer into buf */
static void
SDL_StreamRead(SDL_AudioStreamer * stream, Uint8 * buf, int length)
{
    int i;

    for (i = 0; i < length; ++i) {
        buf[i] = stream->buffer[stream->read_pos];
        ++stream->read_pos;
    }
}

static int
SDL_StreamLength(SDL_AudioStreamer * stream)
{
    return (stream->write_pos - stream->read_pos) % stream->max_len;
}

/* Initialize the stream by allocating the buffer and setting the read/write heads to the beginning */
#if 0
static int
SDL_StreamInit(SDL_AudioStreamer * stream, int max_len, Uint8 silence)
{
    /* First try to allocate the buffer */
    stream->buffer = (Uint8 *) SDL_malloc(max_len);
    if (stream->buffer == NULL) {
        return -1;
    }

    stream->max_len = max_len;
    stream->read_pos = 0;
    stream->write_pos = 0;

    /* Zero out the buffer */
    SDL_memset(stream->buffer, silence, max_len);

    return 0;
}
#endif

/* Deinitialize the stream simply by freeing the buffer */
static void
SDL_StreamDeinit(SDL_AudioStreamer * stream)
{
    if (stream->buffer != NULL) {
        SDL_free(stream->buffer);
    }
}

#if defined(ANDROID)
#include <android/log.h>
#endif

/* The general mixing thread function */
int SDLCALL
SDL_RunAudio(void *devicep)
{
    SDL_AudioDevice *device = (SDL_AudioDevice *) devicep;
    Uint8 *stream;
    int stream_len;
    void *udata;
    void (SDLCALL * fill) (void *userdata, Uint8 * stream, int len);
    int silence;
    Uint32 delay;

    /* For streaming when the buffer sizes don't match up */
    Uint8 *istream;
    int istream_len = 0;

    /* Perform any thread setup */
    device->threadid = SDL_ThreadID();
    current_audio.impl.ThreadInit(device);

    /* Set up the mixing function */
    fill = device->spec.callback;
    udata = device->spec.userdata;

    /* By default do not stream */
    device->use_streamer = 0;

    if (device->convert.needed) {
        if (device->convert.src_format == AUDIO_U8) {
            silence = 0x80;
        } else {
            silence = 0;
        }

#if 0                           /* !!! FIXME: I took len_div out of the structure. Use rate_incr instead? */
        /* If the result of the conversion alters the length, i.e. resampling is being used, use the streamer */
        if (device->convert.len_mult != 1 || device->convert.len_div != 1) {
            /* The streamer's maximum length should be twice whichever is larger: spec.size or len_cvt */
            stream_max_len = 2 * device->spec.size;
            if (device->convert.len_mult > device->convert.len_div) {
                stream_max_len *= device->convert.len_mult;
                stream_max_len /= device->convert.len_div;
            }
            if (SDL_StreamInit(&device->streamer, stream_max_len, silence) <
                0)
                return -1;
            device->use_streamer = 1;

            /* istream_len should be the length of what we grab from the callback and feed to conversion,
               so that we get close to spec_size. I.e. we want device.spec_size = istream_len * u / d
             */
            istream_len =
                device->spec.size * device->convert.len_div /
                device->convert.len_mult;
        }
#endif

        /* stream_len = device->convert.len; */
        stream_len = device->spec.size;
    } else {
        silence = device->spec.silence;
        stream_len = device->spec.size;
    }

    /* Calculate the delay while paused */
    delay = ((device->spec.samples * 1000) / device->spec.freq);

    /* Determine if the streamer is necessary here */
    if (device->use_streamer == 1) {
        /* This code is almost the same as the old code. The difference is, instead of reading
           directly from the callback into "stream", then converting and sending the audio off,
           we go: callback -> "istream" -> (conversion) -> streamer -> stream -> device.
           However, reading and writing with streamer are done separately:
           - We only call the callback and write to the streamer when the streamer does not
           contain enough samples to output to the device.
           - We only read from the streamer and tell the device to play when the streamer
           does have enough samples to output.
           This allows us to perform resampling in the conversion step, where the output of the
           resampling process can be any number. We will have to see what a good size for the
           stream's maximum length is, but I suspect 2*max(len_cvt, stream_len) is a good figure.
         */
        while (device->enabled) {

            if (device->paused) {
                SDL_Delay(delay);
                continue;
            }

            /* Only read in audio if the streamer doesn't have enough already (if it does not have enough samples to output) */
            if (SDL_StreamLength(&device->streamer) < stream_len) {
                /* Set up istream */
                if (device->convert.needed) {
                    if (device->convert.buf) {
                        istream = device->convert.buf;
                    } else {
                        continue;
                    }
                } else {
/* FIXME: Ryan, this is probably wrong.  I imagine we don't want to get
 * a device buffer both here and below in the stream output.
 */
                    istream = current_audio.impl.GetDeviceBuf(device);
                    if (istream == NULL) {
                        istream = device->fake_stream;
                    }
                }

                /* Read from the callback into the _input_ stream */
                SDL_mutexP(device->mixer_lock);
                (*fill) (udata, istream, istream_len);
                SDL_mutexV(device->mixer_lock);

                /* Convert the audio if necessary and write to the streamer */
                if (device->convert.needed) {
                    SDL_ConvertAudio(&device->convert);
                    if (istream == NULL) {
                        istream = device->fake_stream;
                    }
                    /*SDL_memcpy(istream, device->convert.buf, device->convert.len_cvt); */
                    SDL_StreamWrite(&device->streamer, device->convert.buf,
                                    device->convert.len_cvt);
                } else {
                    SDL_StreamWrite(&device->streamer, istream, istream_len);
                }
            }

            /* Only output audio if the streamer has enough to output */
            if (SDL_StreamLength(&device->streamer) >= stream_len) {
                /* Set up the output stream */
                if (device->convert.needed) {
                    if (device->convert.buf) {
                        stream = device->convert.buf;
                    } else {
                        continue;
                    }
                } else {
                    stream = current_audio.impl.GetDeviceBuf(device);
                    if (stream == NULL) {
                        stream = device->fake_stream;
                    }
                }

                /* Now read from the streamer */
                SDL_StreamRead(&device->streamer, stream, stream_len);

                /* Ready current buffer for play and change current buffer */
                if (stream != device->fake_stream) {
                    current_audio.impl.PlayDevice(device);
                    /* Wait for an audio buffer to become available */
                    current_audio.impl.WaitDevice(device);
                } else {
                    SDL_Delay(delay);
                }
            }

        }
    } else {
        /* Otherwise, do not use the streamer. This is the old code. */

        /* Loop, filling the audio buffers */
        while (device->enabled) {

            if (device->paused) {
                SDL_Delay(delay);
                continue;
            }

            /* Fill the current buffer with sound */
            if (device->convert.needed) {
                if (device->convert.buf) {
                    stream = device->convert.buf;
                } else {
                    continue;
                }
            } else {
                stream = current_audio.impl.GetDeviceBuf(device);
                if (stream == NULL) {
                    stream = device->fake_stream;
                }
            }

            SDL_mutexP(device->mixer_lock);
            (*fill) (udata, stream, stream_len);
            SDL_mutexV(device->mixer_lock);

            /* Convert the audio if necessary */
            if (device->convert.needed) {
                SDL_ConvertAudio(&device->convert);
                stream = current_audio.impl.GetDeviceBuf(device);
                if (stream == NULL) {
                    stream = device->fake_stream;
                }
                SDL_memcpy(stream, device->convert.buf,
                           device->convert.len_cvt);
            }

            /* Ready current buffer for play and change current buffer */
            if (stream != device->fake_stream) {
                current_audio.impl.PlayDevice(device);
                /* Wait for an audio buffer to become available */
                current_audio.impl.WaitDevice(device);
            } else {
                SDL_Delay(delay);
            }
        }
    }

    /* Wait for the audio to drain.. */
    current_audio.impl.WaitDone(device);

    /* If necessary, deinit the streamer */
    if (device->use_streamer == 1)
        SDL_StreamDeinit(&device->streamer);

    return (0);
}


static SDL_AudioFormat
SDL_ParseAudioFormat(const char *string)
{
#define CHECK_FMT_STRING(x) if (SDL_strcmp(string, #x) == 0) return AUDIO_##x
    CHECK_FMT_STRING(U8);
    CHECK_FMT_STRING(S8);
    CHECK_FMT_STRING(U16LSB);
    CHECK_FMT_STRING(S16LSB);
    CHECK_FMT_STRING(U16MSB);
    CHECK_FMT_STRING(S16MSB);
    CHECK_FMT_STRING(U16SYS);
    CHECK_FMT_STRING(S16SYS);
    CHECK_FMT_STRING(U16);
    CHECK_FMT_STRING(S16);
    CHECK_FMT_STRING(S32LSB);
    CHECK_FMT_STRING(S32MSB);
    CHECK_FMT_STRING(S32SYS);
    CHECK_FMT_STRING(S32);
    CHECK_FMT_STRING(F32LSB);
    CHECK_FMT_STRING(F32MSB);
    CHECK_FMT_STRING(F32SYS);
    CHECK_FMT_STRING(F32);
#undef CHECK_FMT_STRING
    return 0;
}

int
SDL_GetNumAudioDrivers(void)
{
    return (SDL_arraysize(bootstrap) - 1);
}

const char *
SDL_GetAudioDriver(int index)
{
    if (index >= 0 && index < SDL_GetNumAudioDrivers()) {
        return (bootstrap[index]->name);
    }
    return (NULL);
}

int
SDL_AudioInit(const char *driver_name)
{
    int i = 0;
    int initialized = 0;
    int tried_to_init = 0;

    if (SDL_WasInit(SDL_INIT_AUDIO)) {
        SDL_AudioQuit();        /* shutdown driver if already running. */
    }

    SDL_memset(&current_audio, '\0', sizeof(current_audio));
    SDL_memset(open_devices, '\0', sizeof(open_devices));

    /* Select the proper audio driver */
    if (driver_name == NULL) {
        driver_name = SDL_getenv("SDL_AUDIODRIVER");
    }

    for (i = 0; (!initialized) && (bootstrap[i]); ++i) {
        /* make sure we should even try this driver before doing so... */
        const AudioBootStrap *backend = bootstrap[i];
        if (((driver_name) && (SDL_strcasecmp(backend->name, driver_name))) ||
            ((!driver_name) && (backend->demand_only))) {
            continue;
        }

        tried_to_init = 1;
        SDL_memset(&current_audio, 0, sizeof(current_audio));
        current_audio.name = backend->name;
        current_audio.desc = backend->desc;
        initialized = backend->init(&current_audio.impl);
    }

    if (!initialized) {
        /* specific drivers will set the error message if they fail... */
        if (!tried_to_init) {
            if (driver_name) {
                SDL_SetError("Audio target '%s' not available", driver_name);
            } else {
                SDL_SetError("No available audio device");
            }
        }

        SDL_memset(&current_audio, 0, sizeof(current_audio));
        return (-1);            /* No driver was available, so fail. */
    }

    finalize_audio_entry_points();

    return (0);
}

/*
 * Get the current audio driver name
 */
const char *
SDL_GetCurrentAudioDriver()
{
    return current_audio.name;
}


int
SDL_GetNumAudioDevices(int iscapture)
{
    if (!SDL_WasInit(SDL_INIT_AUDIO)) {
        return -1;
    }
    if ((iscapture) && (!current_audio.impl.HasCaptureSupport)) {
        return 0;
    }

    if ((iscapture) && (current_audio.impl.OnlyHasDefaultInputDevice)) {
        return 1;
    }

    if ((!iscapture) && (current_audio.impl.OnlyHasDefaultOutputDevice)) {
        return 1;
    }

    return current_audio.impl.DetectDevices(iscapture);
}


const char *
SDL_GetAudioDeviceName(int index, int iscapture)
{
    if (!SDL_WasInit(SDL_INIT_AUDIO)) {
        SDL_SetError("Audio subsystem is not initialized");
        return NULL;
    }

    if ((iscapture) && (!current_audio.impl.HasCaptureSupport)) {
        SDL_SetError("No capture support");
        return NULL;
    }

    if (index < 0) {
        SDL_SetError("No such device");
        return NULL;
    }

    if ((iscapture) && (current_audio.impl.OnlyHasDefaultInputDevice)) {
        return DEFAULT_INPUT_DEVNAME;
    }

    if ((!iscapture) && (current_audio.impl.OnlyHasDefaultOutputDevice)) {
        return DEFAULT_OUTPUT_DEVNAME;
    }

    return current_audio.impl.GetDeviceName(index, iscapture);
}


static void
close_audio_device(SDL_AudioDevice * device)
{
    device->enabled = 0;
    if (device->thread != NULL) {
        SDL_WaitThread(device->thread, NULL);
    }
    if (device->mixer_lock != NULL) {
        SDL_DestroyMutex(device->mixer_lock);
    }
    if (device->fake_stream != NULL) {
        SDL_FreeAudioMem(device->fake_stream);
    }
    if (device->convert.needed) {
        SDL_FreeAudioMem(device->convert.buf);
    }
    if (device->opened) {
        current_audio.impl.CloseDevice(device);
        device->opened = 0;
    }
    SDL_FreeAudioMem(device);
}


/*
 * Sanity check desired AudioSpec for SDL_OpenAudio() in (orig).
 *  Fills in a sanitized copy in (prepared).
 *  Returns non-zero if okay, zero on fatal parameters in (orig).
 */
static int
prepare_audiospec(const SDL_AudioSpec * orig, SDL_AudioSpec * prepared)
{
    SDL_memcpy(prepared, orig, sizeof(SDL_AudioSpec));

    if (orig->callback == NULL) {
        SDL_SetError("SDL_OpenAudio() passed a NULL callback");
        return 0;
    }

    if (orig->freq == 0) {
        const char *env = SDL_getenv("SDL_AUDIO_FREQUENCY");
        if ((!env) || ((prepared->freq = SDL_atoi(env)) == 0)) {
            prepared->freq = 22050;     /* a reasonable default */
        }
    }

    if (orig->format == 0) {
        const char *env = SDL_getenv("SDL_AUDIO_FORMAT");
        if ((!env) || ((prepared->format = SDL_ParseAudioFormat(env)) == 0)) {
            prepared->format = AUDIO_S16;       /* a reasonable default */
        }
    }

    switch (orig->channels) {
    case 0:{
            const char *env = SDL_getenv("SDL_AUDIO_CHANNELS");
            if ((!env) || ((prepared->channels = (Uint8) SDL_atoi(env)) == 0)) {
                prepared->channels = 2; /* a reasonable default */
            }
            break;
        }
    case 1:                    /* Mono */
    case 2:                    /* Stereo */
    case 4:                    /* surround */
    case 6:                    /* surround with center and lfe */
        break;
    default:
        SDL_SetError("Unsupported number of audio channels.");
        return 0;
    }

    if (orig->samples == 0) {
        const char *env = SDL_getenv("SDL_AUDIO_SAMPLES");
        if ((!env) || ((prepared->samples = (Uint16) SDL_atoi(env)) == 0)) {
            /* Pick a default of ~46 ms at desired frequency */
            /* !!! FIXME: remove this when the non-Po2 resampling is in. */
            const int samples = (prepared->freq / 1000) * 46;
            int power2 = 1;
            while (power2 < samples) {
                power2 *= 2;
            }
            prepared->samples = power2;
        }
    }

    /* Calculate the silence and size of the audio specification */
    SDL_CalculateAudioSpec(prepared);

    return 1;
}


static SDL_AudioDeviceID
open_audio_device(const char *devname, int iscapture,
                  const SDL_AudioSpec * desired, SDL_AudioSpec * obtained,
                  int allowed_changes, int min_id)
{
    SDL_AudioDeviceID id = 0;
    SDL_AudioSpec _obtained;
    SDL_AudioDevice *device;
    SDL_bool build_cvt;
    int i = 0;

    if (!SDL_WasInit(SDL_INIT_AUDIO)) {
        SDL_SetError("Audio subsystem is not initialized");
        return 0;
    }

    if ((iscapture) && (!current_audio.impl.HasCaptureSupport)) {
        SDL_SetError("No capture support");
        return 0;
    }

    if (!obtained) {
        obtained = &_obtained;
    }
    if (!prepare_audiospec(desired, obtained)) {
        return 0;
    }

    /* If app doesn't care about a specific device, let the user override. */
    if (devname == NULL) {
        devname = SDL_getenv("SDL_AUDIO_DEVICE_NAME");
    }

    /*
     * Catch device names at the high level for the simple case...
     * This lets us have a basic "device enumeration" for systems that
     *  don't have multiple devices, but makes sure the device name is
     *  always NULL when it hits the low level.
     *
     * Also make sure that the simple case prevents multiple simultaneous
     *  opens of the default system device.
     */

    if ((iscapture) && (current_audio.impl.OnlyHasDefaultInputDevice)) {
        if ((devname) && (SDL_strcmp(devname, DEFAULT_INPUT_DEVNAME) != 0)) {
            SDL_SetError("No such device");
            return 0;
        }
        devname = NULL;

        for (i = 0; i < SDL_arraysize(open_devices); i++) {
            if ((open_devices[i]) && (open_devices[i]->iscapture)) {
                SDL_SetError("Audio device already open");
                return 0;
            }
        }
    }

    if ((!iscapture) && (current_audio.impl.OnlyHasDefaultOutputDevice)) {
        if ((devname) && (SDL_strcmp(devname, DEFAULT_OUTPUT_DEVNAME) != 0)) {
            SDL_SetError("No such device");
            return 0;
        }
        devname = NULL;

        for (i = 0; i < SDL_arraysize(open_devices); i++) {
            if ((open_devices[i]) && (!open_devices[i]->iscapture)) {
                SDL_SetError("Audio device already open");
                return 0;
            }
        }
    }

    device = (SDL_AudioDevice *) SDL_AllocAudioMem(sizeof(SDL_AudioDevice));
    if (device == NULL) {
        SDL_OutOfMemory();
        return 0;
    }
    SDL_memset(device, '\0', sizeof(SDL_AudioDevice));
    device->spec = *obtained;
    device->enabled = 1;
    device->paused = 1;
    device->iscapture = iscapture;

    /* Create a semaphore for locking the sound buffers */
    if (!current_audio.impl.SkipMixerLock) {
        device->mixer_lock = SDL_CreateMutex();
        if (device->mixer_lock == NULL) {
            close_audio_device(device);
            SDL_SetError("Couldn't create mixer lock");
            return 0;
        }
    }

    if (!current_audio.impl.OpenDevice(device, devname, iscapture)) {
        close_audio_device(device);
        return 0;
    }
    device->opened = 1;

    /* Allocate a fake audio memory buffer */
    device->fake_stream = (Uint8 *)SDL_AllocAudioMem(device->spec.size);
    if (device->fake_stream == NULL) {
        close_audio_device(device);
        SDL_OutOfMemory();
        return 0;
    }

    /* If the audio driver changes the buffer size, accept it */
    if (device->spec.samples != obtained->samples) {
        obtained->samples = device->spec.samples;
        SDL_CalculateAudioSpec(obtained);
    }

    /* See if we need to do any conversion */
    build_cvt = SDL_FALSE;
    if (obtained->freq != device->spec.freq) {
        if (allowed_changes & SDL_AUDIO_ALLOW_FREQUENCY_CHANGE) {
            obtained->freq = device->spec.freq;
        } else {
            build_cvt = SDL_TRUE;
        }
    }
    if (obtained->format != device->spec.format) {
        if (allowed_changes & SDL_AUDIO_ALLOW_FORMAT_CHANGE) {
            obtained->format = device->spec.format;
        } else {
            build_cvt = SDL_TRUE;
        }
    }
    if (obtained->channels != device->spec.channels) {
        if (allowed_changes & SDL_AUDIO_ALLOW_CHANNELS_CHANGE) {
            obtained->channels = device->spec.channels;
        } else {
            build_cvt = SDL_TRUE;
        }
    }
    if (build_cvt) {
        /* Build an audio conversion block */
        if (SDL_BuildAudioCVT(&device->convert,
                              obtained->format, obtained->channels,
                              obtained->freq,
                              device->spec.format, device->spec.channels,
                              device->spec.freq) < 0) {
            close_audio_device(device);
            return 0;
        }
        if (device->convert.needed) {
            device->convert.len = (int) (((double) obtained->size) /
                                         device->convert.len_ratio);

            device->convert.buf =
                (Uint8 *) SDL_AllocAudioMem(device->convert.len *
                                            device->convert.len_mult);
            if (device->convert.buf == NULL) {
                close_audio_device(device);
                SDL_OutOfMemory();
                return 0;
            }
        }
    }

    /* Find an available device ID and store the structure... */
    for (id = min_id - 1; id < SDL_arraysize(open_devices); id++) {
        if (open_devices[id] == NULL) {
            open_devices[id] = device;
            break;
        }
    }

    if (id == SDL_arraysize(open_devices)) {
        SDL_SetError("Too many open audio devices");
        close_audio_device(device);
        return 0;
    }

    /* Start the audio thread if necessary */
    if (!current_audio.impl.ProvidesOwnCallbackThread) {
        /* Start the audio thread */
/* !!! FIXME: this is nasty. */
#if (defined(__WIN32__) && !defined(_WIN32_WCE)) && !defined(HAVE_LIBC)
#undef SDL_CreateThread
        device->thread = SDL_CreateThread(SDL_RunAudio, device, NULL, NULL);
#else
        device->thread = SDL_CreateThread(SDL_RunAudio, device);
#endif
        if (device->thread == NULL) {
            SDL_CloseAudioDevice(id + 1);
            SDL_SetError("Couldn't create audio thread");
            return 0;
        }
    }

    return id + 1;
}


int
SDL_OpenAudio(SDL_AudioSpec * desired, SDL_AudioSpec * obtained)
{
    SDL_AudioDeviceID id = 0;

    /* Start up the audio driver, if necessary. This is legacy behaviour! */
    if (!SDL_WasInit(SDL_INIT_AUDIO)) {
        if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0) {
            return (-1);
        }
    }

    /* SDL_OpenAudio() is legacy and can only act on Device ID #1. */
    if (open_devices[0] != NULL) {
        SDL_SetError("Audio device is already opened");
        return (-1);
    }

    if (obtained) {
        id = open_audio_device(NULL, 0, desired, obtained,
                               SDL_AUDIO_ALLOW_ANY_CHANGE, 1);
    } else {
        id = open_audio_device(NULL, 0, desired, desired, 0, 1);
    }
    if (id > 1) {               /* this should never happen in theory... */
        SDL_CloseAudioDevice(id);
        SDL_SetError("Internal error"); /* MUST be Device ID #1! */
        return (-1);
    }

    return ((id == 0) ? -1 : 0);
}

SDL_AudioDeviceID
SDL_OpenAudioDevice(const char *device, int iscapture,
                    const SDL_AudioSpec * desired, SDL_AudioSpec * obtained,
                    int allowed_changes)
{
    return open_audio_device(device, iscapture, desired, obtained,
                             allowed_changes, 2);
}

SDL_AudioStatus
SDL_GetAudioDeviceStatus(SDL_AudioDeviceID devid)
{
    SDL_AudioDevice *device = get_audio_device(devid);
    SDL_AudioStatus status = SDL_AUDIO_STOPPED;
    if (device && device->enabled) {
        if (device->paused) {
            status = SDL_AUDIO_PAUSED;
        } else {
            status = SDL_AUDIO_PLAYING;
        }
    }
    return (status);
}


SDL_AudioStatus
SDL_GetAudioStatus(void)
{
    return SDL_GetAudioDeviceStatus(1);
}

void
SDL_PauseAudioDevice(SDL_AudioDeviceID devid, int pause_on)
{
    SDL_AudioDevice *device = get_audio_device(devid);
    if (device) {
        device->paused = pause_on;
    }
}

void
SDL_PauseAudio(int pause_on)
{
    SDL_PauseAudioDevice(1, pause_on);
}


void
SDL_LockAudioDevice(SDL_AudioDeviceID devid)
{
    /* Obtain a lock on the mixing buffers */
    SDL_AudioDevice *device = get_audio_device(devid);
    if (device) {
        current_audio.impl.LockDevice(device);
    }
}

void
SDL_LockAudio(void)
{
    SDL_LockAudioDevice(1);
}

void
SDL_UnlockAudioDevice(SDL_AudioDeviceID devid)
{
    /* Obtain a lock on the mixing buffers */
    SDL_AudioDevice *device = get_audio_device(devid);
    if (device) {
        current_audio.impl.UnlockDevice(device);
    }
}

void
SDL_UnlockAudio(void)
{
    SDL_UnlockAudioDevice(1);
}

void
SDL_CloseAudioDevice(SDL_AudioDeviceID devid)
{
    SDL_AudioDevice *device = get_audio_device(devid);
    if (device) {
        close_audio_device(device);
        open_devices[devid - 1] = NULL;
    }
}

void
SDL_CloseAudio(void)
{
    SDL_CloseAudioDevice(1);
}

void
SDL_AudioQuit(void)
{
    SDL_AudioDeviceID i;
    for (i = 0; i < SDL_arraysize(open_devices); i++) {
        SDL_CloseAudioDevice(i);
    }

    /* Free the driver data */
    current_audio.impl.Deinitialize();
    SDL_memset(&current_audio, '\0', sizeof(current_audio));
    SDL_memset(open_devices, '\0', sizeof(open_devices));
}

#define NUM_FORMATS 10
static int format_idx;
static int format_idx_sub;
static SDL_AudioFormat format_list[NUM_FORMATS][NUM_FORMATS] = {
    {AUDIO_U8, AUDIO_S8, AUDIO_S16LSB, AUDIO_S16MSB, AUDIO_U16LSB,
     AUDIO_U16MSB, AUDIO_S32LSB, AUDIO_S32MSB, AUDIO_F32LSB, AUDIO_F32MSB},
    {AUDIO_S8, AUDIO_U8, AUDIO_S16LSB, AUDIO_S16MSB, AUDIO_U16LSB,
     AUDIO_U16MSB, AUDIO_S32LSB, AUDIO_S32MSB, AUDIO_F32LSB, AUDIO_F32MSB},
    {AUDIO_S16LSB, AUDIO_S16MSB, AUDIO_U16LSB, AUDIO_U16MSB, AUDIO_S32LSB,
     AUDIO_S32MSB, AUDIO_F32LSB, AUDIO_F32MSB, AUDIO_U8, AUDIO_S8},
    {AUDIO_S16MSB, AUDIO_S16LSB, AUDIO_U16MSB, AUDIO_U16LSB, AUDIO_S32MSB,
     AUDIO_S32LSB, AUDIO_F32MSB, AUDIO_F32LSB, AUDIO_U8, AUDIO_S8},
    {AUDIO_U16LSB, AUDIO_U16MSB, AUDIO_S16LSB, AUDIO_S16MSB, AUDIO_S32LSB,
     AUDIO_S32MSB, AUDIO_F32LSB, AUDIO_F32MSB, AUDIO_U8, AUDIO_S8},
    {AUDIO_U16MSB, AUDIO_U16LSB, AUDIO_S16MSB, AUDIO_S16LSB, AUDIO_S32MSB,
     AUDIO_S32LSB, AUDIO_F32MSB, AUDIO_F32LSB, AUDIO_U8, AUDIO_S8},
    {AUDIO_S32LSB, AUDIO_S32MSB, AUDIO_F32LSB, AUDIO_F32MSB, AUDIO_S16LSB,
     AUDIO_S16MSB, AUDIO_U16LSB, AUDIO_U16MSB, AUDIO_U8, AUDIO_S8},
    {AUDIO_S32MSB, AUDIO_S32LSB, AUDIO_F32MSB, AUDIO_F32LSB, AUDIO_S16MSB,
     AUDIO_S16LSB, AUDIO_U16MSB, AUDIO_U16LSB, AUDIO_U8, AUDIO_S8},
    {AUDIO_F32LSB, AUDIO_F32MSB, AUDIO_S32LSB, AUDIO_S32MSB, AUDIO_S16LSB,
     AUDIO_S16MSB, AUDIO_U16LSB, AUDIO_U16MSB, AUDIO_U8, AUDIO_S8},
    {AUDIO_F32MSB, AUDIO_F32LSB, AUDIO_S32MSB, AUDIO_S32LSB, AUDIO_S16MSB,
     AUDIO_S16LSB, AUDIO_U16MSB, AUDIO_U16LSB, AUDIO_U8, AUDIO_S8},
};

SDL_AudioFormat
SDL_FirstAudioFormat(SDL_AudioFormat format)
{
    for (format_idx = 0; format_idx < NUM_FORMATS; ++format_idx) {
        if (format_list[format_idx][0] == format) {
            break;
        }
    }
    format_idx_sub = 0;
    return (SDL_NextAudioFormat());
}

SDL_AudioFormat
SDL_NextAudioFormat(void)
{
    if ((format_idx == NUM_FORMATS) || (format_idx_sub == NUM_FORMATS)) {
        return (0);
    }
    return (format_list[format_idx][format_idx_sub++]);
}

void
SDL_CalculateAudioSpec(SDL_AudioSpec * spec)
{
    switch (spec->format) {
    case AUDIO_U8:
        spec->silence = 0x80;
        break;
    default:
        spec->silence = 0x00;
        break;
    }
    spec->size = SDL_AUDIO_BITSIZE(spec->format) / 8;
    spec->size *= spec->channels;
    spec->size *= spec->samples;
}


/*
 * Moved here from SDL_mixer.c, since it relies on internals of an opened
 *  audio device (and is deprecated, by the way!).
 */
void
SDL_MixAudio(Uint8 * dst, const Uint8 * src, Uint32 len, int volume)
{
    /* Mix the user-level audio format */
    SDL_AudioDevice *device = get_audio_device(1);
    if (device != NULL) {
        SDL_AudioFormat format;
        if (device->convert.needed) {
            format = device->convert.src_format;
        } else {
            format = device->spec.format;
        }
        SDL_MixAudioFormat(dst, src, format, len, volume);
    }
}

/* vi: set ts=4 sw=4 expandtab: */
