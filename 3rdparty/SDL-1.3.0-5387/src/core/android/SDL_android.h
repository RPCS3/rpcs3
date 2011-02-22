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

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
/* *INDENT-OFF* */
extern "C" {
/* *INDENT-ON* */
#endif

/* Interface from the SDL library into the Android Java activity */
extern SDL_bool Android_JNI_CreateContext(int majorVersion, int minorVersion);
extern void Android_JNI_SwapWindow();
extern void Android_JNI_SetActivityTitle(const char *title);
extern void Android_JNI_GetAccelerometerValues(float values[3]);

// Audio support
extern int Android_JNI_OpenAudioDevice(int sampleRate, int is16Bit, int channelCount, int desiredBufferFrames);
extern void* Android_JNI_GetAudioBuffer();
extern void Android_JNI_WriteAudioBuffer();
extern void Android_JNI_CloseAudioDevice();

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
/* *INDENT-OFF* */
}
/* *INDENT-ON* */
#endif

/* vi: set ts=4 sw=4 expandtab: */
