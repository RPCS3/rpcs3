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

#include <AudioUnit/AudioUnit.h>

#include "SDL_audio.h"
#include "../SDL_audio_c.h"
#include "../SDL_sysaudio.h"
#include "SDL_coreaudio_iphone.h"

#define DEBUG_COREAUDIO 0

static void
COREAUDIO_Deinitialize(void)
{
}

/* The CoreAudio callback */
static OSStatus
outputCallback(void *inRefCon,
               AudioUnitRenderActionFlags * ioActionFlags,
               const AudioTimeStamp * inTimeStamp,
               UInt32 inBusNumber, UInt32 inNumberFrames,
               AudioBufferList * ioDataList)
{
    SDL_AudioDevice *this = (SDL_AudioDevice *) inRefCon;
    AudioBuffer *ioData = &ioDataList->mBuffers[0];
    UInt32 remaining, len;
    void *ptr;

    /* Is there ever more than one buffer, and what do you do with it? */
    if (ioDataList->mNumberBuffers != 1) {
        return noErr;
    }

    /* Only do anything if audio is enabled and not paused */
    if (!this->enabled || this->paused) {
        SDL_memset(ioData->mData, this->spec.silence, ioData->mDataByteSize);
        return 0;
    }

    /* No SDL conversion should be needed here, ever, since we accept
       any input format in OpenAudio, and leave the conversion to CoreAudio.
     */
    /*
       assert(!this->convert.needed);
       assert(this->spec.channels == ioData->mNumberChannels);
     */

    remaining = ioData->mDataByteSize;
    ptr = ioData->mData;
    while (remaining > 0) {
        if (this->hidden->bufferOffset >= this->hidden->bufferSize) {
            /* Generate the data */
            SDL_memset(this->hidden->buffer, this->spec.silence,
                       this->hidden->bufferSize);
            SDL_mutexP(this->mixer_lock);
            (*this->spec.callback) (this->spec.userdata, this->hidden->buffer,
                                    this->hidden->bufferSize);
            SDL_mutexV(this->mixer_lock);
            this->hidden->bufferOffset = 0;
        }

        len = this->hidden->bufferSize - this->hidden->bufferOffset;
        if (len > remaining)
            len = remaining;
        SDL_memcpy(ptr,
                   (char *) this->hidden->buffer + this->hidden->bufferOffset,
                   len);
        ptr = (char *) ptr + len;
        remaining -= len;
        this->hidden->bufferOffset += len;
    }

    return 0;
}

static OSStatus
inputCallback(void *inRefCon,
              AudioUnitRenderActionFlags * ioActionFlags,
              const AudioTimeStamp * inTimeStamp,
              UInt32 inBusNumber, UInt32 inNumberFrames,
              AudioBufferList * ioData)
{
    //err = AudioUnitRender(afr->fAudioUnit, ioActionFlags, inTimeStamp, inBusNumber, inNumberFrames, afr->fAudioBuffer);
    // !!! FIXME: write me!
    return noErr;
}


static void
COREAUDIO_CloseDevice(_THIS)
{
    if (this->hidden != NULL) {
        if (this->hidden->audioUnitOpened) {
            OSStatus result = noErr;
            AURenderCallbackStruct callback;
            const AudioUnitElement output_bus = 0;
            const AudioUnitElement input_bus = 1;
            const int iscapture = this->iscapture;
            const AudioUnitElement bus =
                ((iscapture) ? input_bus : output_bus);
            const AudioUnitScope scope =
                ((iscapture) ? kAudioUnitScope_Output :
                 kAudioUnitScope_Input);

            /* stop processing the audio unit */
            result = AudioOutputUnitStop(this->hidden->audioUnit);

            /* Remove the input callback */
            SDL_memset(&callback, '\0', sizeof(AURenderCallbackStruct));
            result = AudioUnitSetProperty(this->hidden->audioUnit,
                                          kAudioUnitProperty_SetRenderCallback,
                                          scope, bus, &callback,
                                          sizeof(callback));

            //CloseComponent(this->hidden->audioUnit);
            this->hidden->audioUnitOpened = 0;
        }
        SDL_free(this->hidden->buffer);
        SDL_free(this->hidden);
        this->hidden = NULL;
    }
}


#define CHECK_RESULT(msg) \
    if (result != noErr) { \
        COREAUDIO_CloseDevice(this); \
        SDL_SetError("CoreAudio error (%s): %d", msg, result); \
        return 0; \
    }

static int
prepare_audiounit(_THIS, const char *devname, int iscapture,
                  const AudioStreamBasicDescription * strdesc)
{
    OSStatus result = noErr;
    AURenderCallbackStruct callback;
    AudioComponentDescription desc;
    AudioComponent comp = NULL;

    UInt32 enableIO = 0;
    const AudioUnitElement output_bus = 0;
    const AudioUnitElement input_bus = 1;
    const AudioUnitElement bus = ((iscapture) ? input_bus : output_bus);
    const AudioUnitScope scope = ((iscapture) ? kAudioUnitScope_Output :
                                  kAudioUnitScope_Input);

    SDL_memset(&desc, '\0', sizeof(AudioComponentDescription));
    desc.componentType = kAudioUnitType_Output;
    desc.componentSubType = kAudioUnitSubType_RemoteIO;
    desc.componentManufacturer = kAudioUnitManufacturer_Apple;

    comp = AudioComponentFindNext(NULL, &desc);
    if (comp == NULL) {
        SDL_SetError("Couldn't find requested CoreAudio component");
        return 0;
    }

    /* Open & initialize the audio unit */
    /*
       AudioComponentInstanceNew only available on iPhone OS 2.0 and Mac OS X 10.6  
       We can't use OpenAComponent on iPhone because it is not present
     */
    result = AudioComponentInstanceNew(comp, &this->hidden->audioUnit);
    CHECK_RESULT("AudioComponentInstanceNew");

    this->hidden->audioUnitOpened = 1;

    // !!! FIXME: this is wrong?
    enableIO = ((iscapture) ? 1 : 0);
    result = AudioUnitSetProperty(this->hidden->audioUnit,
                                  kAudioOutputUnitProperty_EnableIO,
                                  kAudioUnitScope_Input, input_bus,
                                  &enableIO, sizeof(enableIO));
    CHECK_RESULT("AudioUnitSetProperty (kAudioUnitProperty_EnableIO input)");

    // !!! FIXME: this is wrong?
    enableIO = ((iscapture) ? 0 : 1);
    result = AudioUnitSetProperty(this->hidden->audioUnit,
                                  kAudioOutputUnitProperty_EnableIO,
                                  kAudioUnitScope_Output, output_bus,
                                  &enableIO, sizeof(enableIO));
    CHECK_RESULT("AudioUnitSetProperty (kAudioUnitProperty_EnableIO output)");

    /*result = AudioUnitSetProperty(this->hidden->audioUnit,
       kAudioOutputUnitProperty_CurrentDevice,
       kAudioUnitScope_Global, 0,
       &this->hidden->deviceID,
       sizeof(AudioDeviceID));

       CHECK_RESULT("AudioUnitSetProperty (kAudioOutputUnitProperty_CurrentDevice)"); */

    /* Set the data format of the audio unit. */
    result = AudioUnitSetProperty(this->hidden->audioUnit,
                                  kAudioUnitProperty_StreamFormat,
                                  scope, bus, strdesc, sizeof(*strdesc));
    CHECK_RESULT("AudioUnitSetProperty (kAudioUnitProperty_StreamFormat)");

    /* Set the audio callback */
    SDL_memset(&callback, '\0', sizeof(AURenderCallbackStruct));
    callback.inputProc = ((iscapture) ? inputCallback : outputCallback);
    callback.inputProcRefCon = this;
    result = AudioUnitSetProperty(this->hidden->audioUnit,
                                  kAudioUnitProperty_SetRenderCallback,
                                  scope, bus, &callback, sizeof(callback));
    CHECK_RESULT
        ("AudioUnitSetProperty (kAudioUnitProperty_SetInputCallback)");

    /* Calculate the final parameters for this audio specification */
    SDL_CalculateAudioSpec(&this->spec);

    /* Allocate a sample buffer */
    this->hidden->bufferOffset = this->hidden->bufferSize = this->spec.size;
    this->hidden->buffer = SDL_malloc(this->hidden->bufferSize);

    result = AudioUnitInitialize(this->hidden->audioUnit);
    CHECK_RESULT("AudioUnitInitialize");

    /* Finally, start processing of the audio unit */
    result = AudioOutputUnitStart(this->hidden->audioUnit);
    CHECK_RESULT("AudioOutputUnitStart");
    /* We're running! */
    return 1;
}

static int
COREAUDIO_OpenDevice(_THIS, const char *devname, int iscapture)
{
    AudioStreamBasicDescription strdesc;
    SDL_AudioFormat test_format = SDL_FirstAudioFormat(this->spec.format);
    int valid_datatype = 0;

    /* Initialize all variables that we clean on shutdown */
    this->hidden = (struct SDL_PrivateAudioData *)
        SDL_malloc((sizeof *this->hidden));
    if (this->hidden == NULL) {
        SDL_OutOfMemory();
        return (0);
    }
    SDL_memset(this->hidden, 0, (sizeof *this->hidden));

    /* Setup a AudioStreamBasicDescription with the requested format */
    SDL_memset(&strdesc, '\0', sizeof(AudioStreamBasicDescription));
    strdesc.mFormatID = kAudioFormatLinearPCM;
    strdesc.mFormatFlags = kLinearPCMFormatFlagIsPacked;
    strdesc.mChannelsPerFrame = this->spec.channels;
    strdesc.mSampleRate = this->spec.freq;
    strdesc.mFramesPerPacket = 1;

    while ((!valid_datatype) && (test_format)) {
        this->spec.format = test_format;
        /* Just a list of valid SDL formats, so people don't pass junk here. */
        switch (test_format) {
        case AUDIO_U8:
        case AUDIO_S8:
        case AUDIO_U16LSB:
        case AUDIO_S16LSB:
        case AUDIO_U16MSB:
        case AUDIO_S16MSB:
        case AUDIO_S32LSB:
        case AUDIO_S32MSB:
        case AUDIO_F32LSB:
        case AUDIO_F32MSB:
            valid_datatype = 1;
            strdesc.mBitsPerChannel = SDL_AUDIO_BITSIZE(this->spec.format);
            if (SDL_AUDIO_ISBIGENDIAN(this->spec.format))
                strdesc.mFormatFlags |= kLinearPCMFormatFlagIsBigEndian;

            if (SDL_AUDIO_ISFLOAT(this->spec.format))
                strdesc.mFormatFlags |= kLinearPCMFormatFlagIsFloat;
            else if (SDL_AUDIO_ISSIGNED(this->spec.format))
                strdesc.mFormatFlags |= kLinearPCMFormatFlagIsSignedInteger;
            break;
        }
    }

    if (!valid_datatype) {      /* shouldn't happen, but just in case... */
        COREAUDIO_CloseDevice(this);
        SDL_SetError("Unsupported audio format");
        return 0;
    }

    strdesc.mBytesPerFrame =
        strdesc.mBitsPerChannel * strdesc.mChannelsPerFrame / 8;
    strdesc.mBytesPerPacket =
        strdesc.mBytesPerFrame * strdesc.mFramesPerPacket;

    if (!prepare_audiounit(this, devname, iscapture, &strdesc)) {
        COREAUDIO_CloseDevice(this);
        return 0;               /* prepare_audiounit() will call SDL_SetError()... */
    }

    return 1;                   /* good to go. */
}

static int
COREAUDIO_Init(SDL_AudioDriverImpl * impl)
{
    /* Set the function pointers */
    impl->OpenDevice = COREAUDIO_OpenDevice;
    impl->CloseDevice = COREAUDIO_CloseDevice;
    impl->Deinitialize = COREAUDIO_Deinitialize;
    impl->ProvidesOwnCallbackThread = 1;

    /* added for iPhone */
    impl->OnlyHasDefaultInputDevice = 1;
    impl->OnlyHasDefaultOutputDevice = 1;
    impl->HasCaptureSupport = 0;        /* still needs to be written */

    return 1;   /* this audio target is available. */
}

AudioBootStrap COREAUDIOIPHONE_bootstrap = {
    "coreaudio-iphoneos", "SDL CoreAudio (iPhone OS) audio driver",
    COREAUDIO_Init, 0
};

/* vi: set ts=4 sw=4 expandtab: */
