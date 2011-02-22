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

    This driver was written by:
    Erik Inge Bolsø
    knan@mo.himolde.no
*/
#include "SDL_config.h"

/* Allow access to a raw mixing buffer */

#include <signal.h>
#include <unistd.h>

#include "SDL_timer.h"
#include "SDL_audio.h"
#include "SDL_loadso.h"
#include "../SDL_audiomem.h"
#include "../SDL_audio_c.h"
#include "SDL_nasaudio.h"

/* The tag name used by nas audio */
#define NAS_DRIVER_NAME         "nas"

static struct SDL_PrivateAudioData *this2 = NULL;


static void (*NAS_AuCloseServer) (AuServer *);
static void (*NAS_AuNextEvent) (AuServer *, AuBool, AuEvent *);
static AuBool(*NAS_AuDispatchEvent) (AuServer *, AuEvent *);
static AuFlowID(*NAS_AuCreateFlow) (AuServer *, AuStatus *);
static void (*NAS_AuStartFlow) (AuServer *, AuFlowID, AuStatus *);
static void (*NAS_AuSetElements)
  (AuServer *, AuFlowID, AuBool, int, AuElement *, AuStatus *);
static void (*NAS_AuWriteElement)
  (AuServer *, AuFlowID, int, AuUint32, AuPointer, AuBool, AuStatus *);
static AuServer *(*NAS_AuOpenServer)
  (_AuConst char *, int, _AuConst char *, int, _AuConst char *, char **);
static AuEventHandlerRec *(*NAS_AuRegisterEventHandler)
  (AuServer *, AuMask, int, AuID, AuEventHandlerCallback, AuPointer);


#ifdef SDL_AUDIO_DRIVER_NAS_DYNAMIC

static const char *nas_library = SDL_AUDIO_DRIVER_NAS_DYNAMIC;
static void *nas_handle = NULL;

static int
load_nas_sym(const char *fn, void **addr)
{
    *addr = SDL_LoadFunction(nas_handle, fn);
    if (*addr == NULL) {
        return 0;
    }
    return 1;
}

/* cast funcs to char* first, to please GCC's strict aliasing rules. */
#define SDL_NAS_SYM(x) \
    if (!load_nas_sym(#x, (void **) (char *) &NAS_##x)) return -1
#else
#define SDL_NAS_SYM(x) NAS_##x = x
#endif

static int
load_nas_syms(void)
{
    SDL_NAS_SYM(AuCloseServer);
    SDL_NAS_SYM(AuNextEvent);
    SDL_NAS_SYM(AuDispatchEvent);
    SDL_NAS_SYM(AuCreateFlow);
    SDL_NAS_SYM(AuStartFlow);
    SDL_NAS_SYM(AuSetElements);
    SDL_NAS_SYM(AuWriteElement);
    SDL_NAS_SYM(AuOpenServer);
    SDL_NAS_SYM(AuRegisterEventHandler);
    return 0;
}

#undef SDL_NAS_SYM

#ifdef SDL_AUDIO_DRIVER_NAS_DYNAMIC

static void
UnloadNASLibrary(void)
{
    if (nas_handle != NULL) {
        SDL_UnloadObject(nas_handle);
        nas_handle = NULL;
    }
}

static int
LoadNASLibrary(void)
{
    int retval = 0;
    if (nas_handle == NULL) {
        nas_handle = SDL_LoadObject(nas_library);
        if (nas_handle == NULL) {
            /* Copy error string so we can use it in a new SDL_SetError(). */
            char *origerr = SDL_GetError();
            size_t len = SDL_strlen(origerr) + 1;
            char *err = (char *) alloca(len);
            SDL_strlcpy(err, origerr, len);
            retval = -1;
            SDL_SetError("NAS: SDL_LoadObject('%s') failed: %s\n",
                         nas_library, err);
        } else {
            retval = load_nas_syms();
            if (retval < 0) {
                UnloadNASLibrary();
            }
        }
    }
    return retval;
}

#else

static void
UnloadNASLibrary(void)
{
}

static int
LoadNASLibrary(void)
{
    load_nas_syms();
    return 0;
}

#endif /* SDL_AUDIO_DRIVER_NAS_DYNAMIC */

/* This function waits until it is possible to write a full sound buffer */
static void
NAS_WaitDevice(_THIS)
{
    while (this->hidden->buf_free < this->hidden->mixlen) {
        AuEvent ev;
        NAS_AuNextEvent(this->hidden->aud, AuTrue, &ev);
        NAS_AuDispatchEvent(this->hidden->aud, &ev);
    }
}

static void
NAS_PlayDevice(_THIS)
{
    while (this->hidden->mixlen > this->hidden->buf_free) {
        /*
         * We think the buffer is full? Yikes! Ask the server for events,
         *  in the hope that some of them is LowWater events telling us more
         *  of the buffer is free now than what we think.
         */
        AuEvent ev;
        NAS_AuNextEvent(this->hidden->aud, AuTrue, &ev);
        NAS_AuDispatchEvent(this->hidden->aud, &ev);
    }
    this->hidden->buf_free -= this->hidden->mixlen;

    /* Write the audio data */
    NAS_AuWriteElement(this->hidden->aud, this->hidden->flow, 0,
                       this->hidden->mixlen, this->hidden->mixbuf, AuFalse,
                       NULL);

    this->hidden->written += this->hidden->mixlen;

#ifdef DEBUG_AUDIO
    fprintf(stderr, "Wrote %d bytes of audio data\n", this->hidden->mixlen);
#endif
}

static Uint8 *
NAS_GetDeviceBuf(_THIS)
{
    return (this->hidden->mixbuf);
}

static void
NAS_CloseDevice(_THIS)
{
    if (this->hidden != NULL) {
        if (this->hidden->mixbuf != NULL) {
            SDL_FreeAudioMem(this->hidden->mixbuf);
            this->hidden->mixbuf = NULL;
        }
        if (this->hidden->aud) {
            NAS_AuCloseServer(this->hidden->aud);
            this->hidden->aud = 0;
        }
        SDL_free(this->hidden);
        this2 = this->hidden = NULL;
    }
}

static unsigned char
sdlformat_to_auformat(unsigned int fmt)
{
    switch (fmt) {
    case AUDIO_U8:
        return AuFormatLinearUnsigned8;
    case AUDIO_S8:
        return AuFormatLinearSigned8;
    case AUDIO_U16LSB:
        return AuFormatLinearUnsigned16LSB;
    case AUDIO_U16MSB:
        return AuFormatLinearUnsigned16MSB;
    case AUDIO_S16LSB:
        return AuFormatLinearSigned16LSB;
    case AUDIO_S16MSB:
        return AuFormatLinearSigned16MSB;
    }
    return AuNone;
}

static AuBool
event_handler(AuServer * aud, AuEvent * ev, AuEventHandlerRec * hnd)
{
    switch (ev->type) {
    case AuEventTypeElementNotify:
        {
            AuElementNotifyEvent *event = (AuElementNotifyEvent *) ev;

            switch (event->kind) {
            case AuElementNotifyKindLowWater:
                if (this2->buf_free >= 0) {
                    this2->really += event->num_bytes;
                    gettimeofday(&this2->last_tv, 0);
                    this2->buf_free += event->num_bytes;
                } else {
                    this2->buf_free = event->num_bytes;
                }
                break;
            case AuElementNotifyKindState:
                switch (event->cur_state) {
                case AuStatePause:
                    if (event->reason != AuReasonUser) {
                        if (this2->buf_free >= 0) {
                            this2->really += event->num_bytes;
                            gettimeofday(&this2->last_tv, 0);
                            this2->buf_free += event->num_bytes;
                        } else {
                            this2->buf_free = event->num_bytes;
                        }
                    }
                    break;
                }
            }
        }
    }
    return AuTrue;
}

static AuDeviceID
find_device(_THIS, int nch)
{
    /* These "Au" things are all macros, not functions... */
    int i;
    for (i = 0; i < AuServerNumDevices(this->hidden->aud); i++) {
        if ((AuDeviceKind(AuServerDevice(this->hidden->aud, i)) ==
             AuComponentKindPhysicalOutput) &&
            AuDeviceNumTracks(AuServerDevice(this->hidden->aud, i)) == nch) {
            return AuDeviceIdentifier(AuServerDevice(this->hidden->aud, i));
        }
    }
    return AuNone;
}

static int
NAS_OpenDevice(_THIS, const char *devname, int iscapture)
{
    AuElement elms[3];
    int buffer_size;
    SDL_AudioFormat test_format, format;

    /* Initialize all variables that we clean on shutdown */
    this->hidden = (struct SDL_PrivateAudioData *)
        SDL_malloc((sizeof *this->hidden));
    if (this->hidden == NULL) {
        SDL_OutOfMemory();
        return 0;
    }
    SDL_memset(this->hidden, 0, (sizeof *this->hidden));

    /* Try for a closest match on audio format */
    format = 0;
    for (test_format = SDL_FirstAudioFormat(this->spec.format);
         !format && test_format;) {
        format = sdlformat_to_auformat(test_format);
        if (format == AuNone) {
            test_format = SDL_NextAudioFormat();
        }
    }
    if (format == 0) {
        NAS_CloseDevice(this);
        SDL_SetError("NAS: Couldn't find any hardware audio formats");
        return 0;
    }
    this->spec.format = test_format;

    this->hidden->aud = NAS_AuOpenServer("", 0, NULL, 0, NULL, NULL);
    if (this->hidden->aud == 0) {
        NAS_CloseDevice(this);
        SDL_SetError("NAS: Couldn't open connection to NAS server");
        return 0;
    }

    this->hidden->dev = find_device(this, this->spec.channels);
    if ((this->hidden->dev == AuNone)
        || (!(this->hidden->flow = NAS_AuCreateFlow(this->hidden->aud, 0)))) {
        NAS_CloseDevice(this);
        SDL_SetError("NAS: Couldn't find a fitting device on NAS server");
        return 0;
    }

    buffer_size = this->spec.freq;
    if (buffer_size < 4096)
        buffer_size = 4096;

    if (buffer_size > 32768)
        buffer_size = 32768;    /* So that the buffer won't get unmanageably big. */

    /* Calculate the final parameters for this audio specification */
    SDL_CalculateAudioSpec(&this->spec);

    this2 = this->hidden;

    AuMakeElementImportClient(elms, this->spec.freq, format,
                              this->spec.channels, AuTrue, buffer_size,
                              buffer_size / 4, 0, NULL);
    AuMakeElementExportDevice(elms + 1, 0, this->hidden->dev, this->spec.freq,
                              AuUnlimitedSamples, 0, NULL);
    NAS_AuSetElements(this->hidden->aud, this->hidden->flow, AuTrue, 2, elms,
                      NULL);
    NAS_AuRegisterEventHandler(this->hidden->aud, AuEventHandlerIDMask, 0,
                               this->hidden->flow, event_handler,
                               (AuPointer) NULL);

    NAS_AuStartFlow(this->hidden->aud, this->hidden->flow, NULL);

    /* Allocate mixing buffer */
    this->hidden->mixlen = this->spec.size;
    this->hidden->mixbuf = (Uint8 *) SDL_AllocAudioMem(this->hidden->mixlen);
    if (this->hidden->mixbuf == NULL) {
        NAS_CloseDevice(this);
        SDL_OutOfMemory();
        return 0;
    }
    SDL_memset(this->hidden->mixbuf, this->spec.silence, this->spec.size);

    /* We're ready to rock and roll. :-) */
    return 1;
}

static void
NAS_Deinitialize(void)
{
    UnloadNASLibrary();
}

static int
NAS_Init(SDL_AudioDriverImpl * impl)
{
    if (LoadNASLibrary() < 0) {
        return 0;
    } else {
        AuServer *aud = NAS_AuOpenServer("", 0, NULL, 0, NULL, NULL);
        if (aud == NULL) {
            SDL_SetError("NAS: AuOpenServer() failed (no audio server?)");
            return 0;
        }
        NAS_AuCloseServer(aud);
    }

    /* Set the function pointers */
    impl->OpenDevice = NAS_OpenDevice;
    impl->PlayDevice = NAS_PlayDevice;
    impl->WaitDevice = NAS_WaitDevice;
    impl->GetDeviceBuf = NAS_GetDeviceBuf;
    impl->CloseDevice = NAS_CloseDevice;
    impl->Deinitialize = NAS_Deinitialize;
    impl->OnlyHasDefaultOutputDevice = 1;       /* !!! FIXME: is this true? */

    return 1;   /* this audio target is available. */
}

AudioBootStrap NAS_bootstrap = {
    NAS_DRIVER_NAME, "Network Audio System", NAS_Init, 0
};

/* vi: set ts=4 sw=4 expandtab: */
