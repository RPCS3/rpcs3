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

#include "SDL_timer.h"
#include "SDL_loadso.h"
#include "SDL_audio.h"
#include "../SDL_audio_c.h"
#include "SDL_dx5audio.h"

/* !!! FIXME: move this somewhere that other drivers can use it... */
#if defined(_WIN32_WCE)
#define WINDOWS_OS_NAME "Windows CE/PocketPC"
#elif defined(WIN64)
#define WINDOWS_OS_NAME "Win64"
#else
#define WINDOWS_OS_NAME "Win32"
#endif

/* DirectX function pointers for audio */
static void* DSoundDLL = NULL;
static HRESULT(WINAPI * DSoundCreate) (LPGUID, LPDIRECTSOUND *, LPUNKNOWN) =
    NULL;

static void
DSOUND_Unload(void)
{
    DSoundCreate = NULL;

    if (DSoundDLL != NULL) {
        SDL_UnloadObject(DSoundDLL);
        DSoundDLL = NULL;
    }
}


static int
DSOUND_Load(void)
{
    int loaded = 0;

    DSOUND_Unload();

    DSoundDLL = SDL_LoadObject("DSOUND.DLL");
    if (DSoundDLL == NULL) {
        SDL_SetError("DirectSound: failed to load DSOUND.DLL");
    } else {
        /* Now make sure we have DirectX 5 or better... */
        /*  (DirectSoundCaptureCreate was added in DX5) */
        if (!SDL_LoadFunction(DSoundDLL, "DirectSoundCaptureCreate")) {
            SDL_SetError("DirectSound: System doesn't appear to have DX5.");
        } else {
            DSoundCreate = SDL_LoadFunction(DSoundDLL, "DirectSoundCreate");
        }

        if (!DSoundCreate) {
            SDL_SetError("DirectSound: Failed to find DirectSoundCreate");
        } else {
            loaded = 1;
        }
    }

    if (!loaded) {
        DSOUND_Unload();
    }

    return loaded;
}


static void
SetDSerror(const char *function, int code)
{
    static const char *error;
    static char errbuf[1024];

    errbuf[0] = 0;
    switch (code) {
    case E_NOINTERFACE:
        error = "Unsupported interface -- Is DirectX 5.0 or later installed?";
        break;
    case DSERR_ALLOCATED:
        error = "Audio device in use";
        break;
    case DSERR_BADFORMAT:
        error = "Unsupported audio format";
        break;
    case DSERR_BUFFERLOST:
        error = "Mixing buffer was lost";
        break;
    case DSERR_CONTROLUNAVAIL:
        error = "Control requested is not available";
        break;
    case DSERR_INVALIDCALL:
        error = "Invalid call for the current state";
        break;
    case DSERR_INVALIDPARAM:
        error = "Invalid parameter";
        break;
    case DSERR_NODRIVER:
        error = "No audio device found";
        break;
    case DSERR_OUTOFMEMORY:
        error = "Out of memory";
        break;
    case DSERR_PRIOLEVELNEEDED:
        error = "Caller doesn't have priority";
        break;
    case DSERR_UNSUPPORTED:
        error = "Function not supported";
        break;
    default:
        SDL_snprintf(errbuf, SDL_arraysize(errbuf),
                     "%s: Unknown DirectSound error: 0x%x", function, code);
        break;
    }
    if (!errbuf[0]) {
        SDL_snprintf(errbuf, SDL_arraysize(errbuf), "%s: %s", function,
                     error);
    }
    SDL_SetError("%s", errbuf);
    return;
}

/* DirectSound needs to be associated with a window */
static HWND mainwin = NULL;
/* */

void
DSOUND_SoundFocus(HWND hwnd)
{
    /* !!! FIXME: probably broken with multi-window support in SDL 1.3 ... */
    mainwin = hwnd;
}

static void
DSOUND_ThreadInit(_THIS)
{
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);
}

static void
DSOUND_WaitDevice(_THIS)
{
    DWORD status = 0;
    DWORD cursor = 0;
    DWORD junk = 0;
    HRESULT result = DS_OK;

    /* Semi-busy wait, since we have no way of getting play notification
       on a primary mixing buffer located in hardware (DirectX 5.0)
     */
    result = IDirectSoundBuffer_GetCurrentPosition(this->hidden->mixbuf,
                                                   &junk, &cursor);
    if (result != DS_OK) {
        if (result == DSERR_BUFFERLOST) {
            IDirectSoundBuffer_Restore(this->hidden->mixbuf);
        }
#ifdef DEBUG_SOUND
        SetDSerror("DirectSound GetCurrentPosition", result);
#endif
        return;
    }

    while ((cursor / this->hidden->mixlen) == this->hidden->lastchunk) {
        /* FIXME: find out how much time is left and sleep that long */
        SDL_Delay(1);

        /* Try to restore a lost sound buffer */
        IDirectSoundBuffer_GetStatus(this->hidden->mixbuf, &status);
        if ((status & DSBSTATUS_BUFFERLOST)) {
            IDirectSoundBuffer_Restore(this->hidden->mixbuf);
            IDirectSoundBuffer_GetStatus(this->hidden->mixbuf, &status);
            if ((status & DSBSTATUS_BUFFERLOST)) {
                break;
            }
        }
        if (!(status & DSBSTATUS_PLAYING)) {
            result = IDirectSoundBuffer_Play(this->hidden->mixbuf, 0, 0,
                                             DSBPLAY_LOOPING);
            if (result == DS_OK) {
                continue;
            }
#ifdef DEBUG_SOUND
            SetDSerror("DirectSound Play", result);
#endif
            return;
        }

        /* Find out where we are playing */
        result = IDirectSoundBuffer_GetCurrentPosition(this->hidden->mixbuf,
                                                       &junk, &cursor);
        if (result != DS_OK) {
            SetDSerror("DirectSound GetCurrentPosition", result);
            return;
        }
    }
}

static void
DSOUND_PlayDevice(_THIS)
{
    /* Unlock the buffer, allowing it to play */
    if (this->hidden->locked_buf) {
        IDirectSoundBuffer_Unlock(this->hidden->mixbuf,
                                  this->hidden->locked_buf,
                                  this->hidden->mixlen, NULL, 0);
    }

}

static Uint8 *
DSOUND_GetDeviceBuf(_THIS)
{
    DWORD cursor = 0;
    DWORD junk = 0;
    HRESULT result = DS_OK;
    DWORD rawlen = 0;

    /* Figure out which blocks to fill next */
    this->hidden->locked_buf = NULL;
    result = IDirectSoundBuffer_GetCurrentPosition(this->hidden->mixbuf,
                                                   &junk, &cursor);
    if (result == DSERR_BUFFERLOST) {
        IDirectSoundBuffer_Restore(this->hidden->mixbuf);
        result = IDirectSoundBuffer_GetCurrentPosition(this->hidden->mixbuf,
                                                       &junk, &cursor);
    }
    if (result != DS_OK) {
        SetDSerror("DirectSound GetCurrentPosition", result);
        return (NULL);
    }
    cursor /= this->hidden->mixlen;
#ifdef DEBUG_SOUND
    /* Detect audio dropouts */
    {
        DWORD spot = cursor;
        if (spot < this->hidden->lastchunk) {
            spot += this->hidden->num_buffers;
        }
        if (spot > this->hidden->lastchunk + 1) {
            fprintf(stderr, "Audio dropout, missed %d fragments\n",
                    (spot - (this->hidden->lastchunk + 1)));
        }
    }
#endif
    this->hidden->lastchunk = cursor;
    cursor = (cursor + 1) % this->hidden->num_buffers;
    cursor *= this->hidden->mixlen;

    /* Lock the audio buffer */
    result = IDirectSoundBuffer_Lock(this->hidden->mixbuf, cursor,
                                     this->hidden->mixlen,
                                     (LPVOID *) & this->hidden->locked_buf,
                                     &rawlen, NULL, &junk, 0);
    if (result == DSERR_BUFFERLOST) {
        IDirectSoundBuffer_Restore(this->hidden->mixbuf);
        result = IDirectSoundBuffer_Lock(this->hidden->mixbuf, cursor,
                                         this->hidden->mixlen,
                                         (LPVOID *) & this->
                                         hidden->locked_buf, &rawlen, NULL,
                                         &junk, 0);
    }
    if (result != DS_OK) {
        SetDSerror("DirectSound Lock", result);
        return (NULL);
    }
    return (this->hidden->locked_buf);
}

static void
DSOUND_WaitDone(_THIS)
{
    Uint8 *stream = DSOUND_GetDeviceBuf(this);

    /* Wait for the playing chunk to finish */
    if (stream != NULL) {
        SDL_memset(stream, this->spec.silence, this->hidden->mixlen);
        DSOUND_PlayDevice(this);
    }
    DSOUND_WaitDevice(this);

    /* Stop the looping sound buffer */
    IDirectSoundBuffer_Stop(this->hidden->mixbuf);
}

static void
DSOUND_CloseDevice(_THIS)
{
    if (this->hidden != NULL) {
        if (this->hidden->sound != NULL) {
            if (this->hidden->mixbuf != NULL) {
                /* Clean up the audio buffer */
                IDirectSoundBuffer_Release(this->hidden->mixbuf);
                this->hidden->mixbuf = NULL;
            }
            IDirectSound_Release(this->hidden->sound);
            this->hidden->sound = NULL;
        }

        SDL_free(this->hidden);
        this->hidden = NULL;
    }
}

/* This function tries to create a secondary audio buffer, and returns the
   number of audio chunks available in the created buffer.
*/
static int
CreateSecondary(_THIS, HWND focus, WAVEFORMATEX * wavefmt)
{
    LPDIRECTSOUND sndObj = this->hidden->sound;
    LPDIRECTSOUNDBUFFER *sndbuf = &this->hidden->mixbuf;
    Uint32 chunksize = this->spec.size;
    const int numchunks = 8;
    HRESULT result = DS_OK;
    DSBUFFERDESC format;
    LPVOID pvAudioPtr1, pvAudioPtr2;
    DWORD dwAudioBytes1, dwAudioBytes2;

    /* Try to set primary mixing privileges */
    if (focus) {
        result = IDirectSound_SetCooperativeLevel(sndObj,
                                                  focus, DSSCL_PRIORITY);
    } else {
        result = IDirectSound_SetCooperativeLevel(sndObj,
                                                  GetDesktopWindow(),
                                                  DSSCL_NORMAL);
    }
    if (result != DS_OK) {
        SetDSerror("DirectSound SetCooperativeLevel", result);
        return (-1);
    }

    /* Try to create the secondary buffer */
    SDL_memset(&format, 0, sizeof(format));
    format.dwSize = sizeof(format);
    format.dwFlags = DSBCAPS_GETCURRENTPOSITION2;
    if (!focus) {
        format.dwFlags |= DSBCAPS_GLOBALFOCUS;
    } else {
        format.dwFlags |= DSBCAPS_STICKYFOCUS;
    }
    format.dwBufferBytes = numchunks * chunksize;
    if ((format.dwBufferBytes < DSBSIZE_MIN) ||
        (format.dwBufferBytes > DSBSIZE_MAX)) {
        SDL_SetError("Sound buffer size must be between %d and %d",
                     DSBSIZE_MIN / numchunks, DSBSIZE_MAX / numchunks);
        return (-1);
    }
    format.dwReserved = 0;
    format.lpwfxFormat = wavefmt;
    result = IDirectSound_CreateSoundBuffer(sndObj, &format, sndbuf, NULL);
    if (result != DS_OK) {
        SetDSerror("DirectSound CreateSoundBuffer", result);
        return (-1);
    }
    IDirectSoundBuffer_SetFormat(*sndbuf, wavefmt);

    /* Silence the initial audio buffer */
    result = IDirectSoundBuffer_Lock(*sndbuf, 0, format.dwBufferBytes,
                                     (LPVOID *) & pvAudioPtr1, &dwAudioBytes1,
                                     (LPVOID *) & pvAudioPtr2, &dwAudioBytes2,
                                     DSBLOCK_ENTIREBUFFER);
    if (result == DS_OK) {
        SDL_memset(pvAudioPtr1, this->spec.silence, dwAudioBytes1);
        IDirectSoundBuffer_Unlock(*sndbuf,
                                  (LPVOID) pvAudioPtr1, dwAudioBytes1,
                                  (LPVOID) pvAudioPtr2, dwAudioBytes2);
    }

    /* We're ready to go */
    return (numchunks);
}

static int
DSOUND_OpenDevice(_THIS, const char *devname, int iscapture)
{
    HRESULT result;
    WAVEFORMATEX waveformat;
    int valid_format = 0;
    SDL_AudioFormat test_format = SDL_FirstAudioFormat(this->spec.format);

    /* !!! FIXME: handle devname */
    /* !!! FIXME: handle iscapture */

    /* Initialize all variables that we clean on shutdown */
    this->hidden = (struct SDL_PrivateAudioData *)
        SDL_malloc((sizeof *this->hidden));
    if (this->hidden == NULL) {
        SDL_OutOfMemory();
        return 0;
    }
    SDL_memset(this->hidden, 0, (sizeof *this->hidden));

    while ((!valid_format) && (test_format)) {
        switch (test_format) {
        case AUDIO_U8:
        case AUDIO_S16:
        case AUDIO_S32:
            this->spec.format = test_format;
            valid_format = 1;
            break;
        }
        test_format = SDL_NextAudioFormat();
    }

    if (!valid_format) {
        DSOUND_CloseDevice(this);
        SDL_SetError("DirectSound: Unsupported audio format");
        return 0;
    }

    SDL_memset(&waveformat, 0, sizeof(waveformat));
    waveformat.wFormatTag = WAVE_FORMAT_PCM;
    waveformat.wBitsPerSample = SDL_AUDIO_BITSIZE(this->spec.format);
    waveformat.nChannels = this->spec.channels;
    waveformat.nSamplesPerSec = this->spec.freq;
    waveformat.nBlockAlign =
        waveformat.nChannels * (waveformat.wBitsPerSample / 8);
    waveformat.nAvgBytesPerSec =
        waveformat.nSamplesPerSec * waveformat.nBlockAlign;

    /* Update the fragment size as size in bytes */
    SDL_CalculateAudioSpec(&this->spec);

    /* Open the audio device */
    result = DSoundCreate(NULL, &this->hidden->sound, NULL);
    if (result != DS_OK) {
        DSOUND_CloseDevice(this);
        SetDSerror("DirectSoundCreate", result);
        return 0;
    }

    /* Create the audio buffer to which we write */
    this->hidden->num_buffers = CreateSecondary(this, mainwin, &waveformat);
    if (this->hidden->num_buffers < 0) {
        DSOUND_CloseDevice(this);
        return 0;
    }

    /* The buffer will auto-start playing in DSOUND_WaitDevice() */
    this->hidden->mixlen = this->spec.size;

    return 1;                   /* good to go. */
}


static void
DSOUND_Deinitialize(void)
{
    DSOUND_Unload();
}


static int
DSOUND_Init(SDL_AudioDriverImpl * impl)
{
    OSVERSIONINFO ver;

    /*
     * Unfortunately, the sound drivers on NT have higher latencies than the
     *  audio buffers used by many SDL applications, so there are gaps in the
     *  audio - it sounds terrible.  Punt for now.
     */
    SDL_memset(&ver, '\0', sizeof(OSVERSIONINFO));
    ver.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    GetVersionEx(&ver);
    if (ver.dwPlatformId == VER_PLATFORM_WIN32_NT) {
        if (ver.dwMajorVersion <= 4) {
            return 0;           /* NT4.0 or earlier. Disable dsound support. */
        }
    }

    if (!DSOUND_Load()) {
        return 0;
    }

    /* Set the function pointers */
    impl->OpenDevice = DSOUND_OpenDevice;
    impl->PlayDevice = DSOUND_PlayDevice;
    impl->WaitDevice = DSOUND_WaitDevice;
    impl->WaitDone = DSOUND_WaitDone;
    impl->ThreadInit = DSOUND_ThreadInit;
    impl->GetDeviceBuf = DSOUND_GetDeviceBuf;
    impl->CloseDevice = DSOUND_CloseDevice;
    impl->Deinitialize = DSOUND_Deinitialize;
    impl->OnlyHasDefaultOutputDevice = 1;       /* !!! FIXME */

    return 1;   /* this audio target is available. */
}

AudioBootStrap DSOUND_bootstrap = {
    "dsound", WINDOWS_OS_NAME "DirectSound", DSOUND_Init, 0
};

/* vi: set ts=4 sw=4 expandtab: */
