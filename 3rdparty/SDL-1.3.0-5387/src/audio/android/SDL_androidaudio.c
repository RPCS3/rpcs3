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

/* Output audio to Android */

#include "SDL_audio.h"
#include "../SDL_audio_c.h"
#include "SDL_androidaudio.h"

#include "../../core/android/SDL_android.h"

#include <android/log.h>

static void * audioDevice;

static int
AndroidAUD_OpenDevice(_THIS, const char *devname, int iscapture)
{
    SDL_AudioFormat test_format;
    int valid_datatype = 0;
    
    if (iscapture) {
    	//TODO: implement capture
    	SDL_SetError("Capture not supported on Android");
    	return 0;
    }

    if (audioDevice != NULL) {
    	SDL_SetError("Only one audio device at a time please!");
    	return 0;
    }

    audioDevice = this;

    this->hidden = SDL_malloc(sizeof(*(this->hidden)));
    if (!this->hidden) {
        SDL_OutOfMemory();
        return 0;
    }
    SDL_memset(this->hidden, 0, (sizeof *this->hidden));

    test_format = SDL_FirstAudioFormat(this->spec.format);
    while (test_format != 0) { // no "UNKNOWN" constant
        if ((test_format == AUDIO_U8) || (test_format == AUDIO_S16LSB)) {
            this->spec.format = test_format;
            break;
        }
        test_format = SDL_NextAudioFormat();
    }
    
    if (test_format == 0) {
    	// Didn't find a compatible format :(
    	SDL_SetError("No compatible audio format!");
    	return 0;
    }

    if (this->spec.channels > 1) {
    	this->spec.channels = 2;
    } else {
    	this->spec.channels = 1;
    }

    if (this->spec.freq < 8000) {
    	this->spec.freq = 8000;
    }
    if (this->spec.freq > 48000) {
    	this->spec.freq = 48000;
    }

    // TODO: pass in/return a (Java) device ID, also whether we're opening for input or output
    this->spec.samples = Android_JNI_OpenAudioDevice(this->spec.freq, this->spec.format == AUDIO_U8 ? 0 : 1, this->spec.channels, this->spec.samples);
    SDL_CalculateAudioSpec(&this->spec);

    if (this->spec.samples == 0) {
    	// Init failed?
    	SDL_SetError("Java-side initialization failed!");
    	return 0;
    }

    return 1;
}

static void
AndroidAUD_PlayDevice(_THIS)
{
    Android_JNI_WriteAudioBuffer();
}

static Uint8 *
AndroidAUD_GetDeviceBuf(_THIS)
{
    return Android_JNI_GetAudioBuffer();
}

static void
AndroidAUD_CloseDevice(_THIS)
{
    if (this->hidden != NULL) {
    	SDL_free(this->hidden);
    	this->hidden = NULL;
    }
	Android_JNI_CloseAudioDevice();

    if (audioDevice == this) {
    	audioDevice = NULL;
    }
}

static int
AndroidAUD_Init(SDL_AudioDriverImpl * impl)
{
    /* Set the function pointers */
    impl->OpenDevice = AndroidAUD_OpenDevice;
    impl->PlayDevice = AndroidAUD_PlayDevice;
    impl->GetDeviceBuf = AndroidAUD_GetDeviceBuf;
    impl->CloseDevice = AndroidAUD_CloseDevice;

    /* and the capabilities */
    impl->ProvidesOwnCallbackThread = 1;
    impl->HasCaptureSupport = 0; //TODO
    impl->OnlyHasDefaultOutputDevice = 1;
    impl->OnlyHasDefaultInputDevice = 1;

    return 1;   /* this audio target is available. */
}

AudioBootStrap ANDROIDAUD_bootstrap = {
    "android", "SDL Android audio driver", AndroidAUD_Init, 0       /*1? */
};

/* Called by the Java code to start the audio processing on a thread */
void
Android_RunAudioThread()
{
	SDL_RunAudio(audioDevice);
}

/* vi: set ts=4 sw=4 expandtab: */
