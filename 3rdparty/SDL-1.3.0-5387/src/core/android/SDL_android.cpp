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
#include "SDL_stdinc.h"

#include "SDL_android.h"

extern "C" {
#include "../../events/SDL_events_c.h"
#include "../../video/android/SDL_androidkeyboard.h"
#include "../../video/android/SDL_androidtouch.h"
#include "../../video/android/SDL_androidvideo.h"

/* Impelemented in audio/android/SDL_androidaudio.c */
extern void Android_RunAudioThread();
} // C

/*******************************************************************************
 This file links the Java side of Android with libsdl
*******************************************************************************/
#include <jni.h>
#include <android/log.h>


/*******************************************************************************
                               Globals
*******************************************************************************/
static JNIEnv* mEnv = NULL;
static JNIEnv* mAudioEnv = NULL;

// Main activity
static jclass mActivityClass;

// method signatures
static jmethodID midCreateGLContext;
static jmethodID midFlipBuffers;
static jmethodID midAudioInit;
static jmethodID midAudioWriteShortBuffer;
static jmethodID midAudioWriteByteBuffer;
static jmethodID midAudioQuit;

// Accelerometer data storage
static float fLastAccelerometer[3];


/*******************************************************************************
                 Functions called by JNI
*******************************************************************************/

// Library init
extern "C" jint JNI_OnLoad(JavaVM* vm, void* reserved)
{
    return JNI_VERSION_1_4;
}

// Called before SDL_main() to initialize JNI bindings
extern "C" void SDL_Android_Init(JNIEnv* env, jclass cls)
{
    __android_log_print(ANDROID_LOG_INFO, "SDL", "SDL_Android_Init()");

    mEnv = env;
    mActivityClass = cls;

    midCreateGLContext = mEnv->GetStaticMethodID(mActivityClass,
                                "createGLContext","(II)Z");
    midFlipBuffers = mEnv->GetStaticMethodID(mActivityClass,
                                "flipBuffers","()V");
    midAudioInit = mEnv->GetStaticMethodID(mActivityClass, 
                                "audioInit", "(IZZI)Ljava/lang/Object;");
    midAudioWriteShortBuffer = mEnv->GetStaticMethodID(mActivityClass,
                                "audioWriteShortBuffer", "([S)V");
    midAudioWriteByteBuffer = mEnv->GetStaticMethodID(mActivityClass,
                                "audioWriteByteBuffer", "([B)V");
    midAudioQuit = mEnv->GetStaticMethodID(mActivityClass,
                                "audioQuit", "()V");

    if(!midCreateGLContext || !midFlipBuffers || !midAudioInit ||
       !midAudioWriteShortBuffer || !midAudioWriteByteBuffer || !midAudioQuit) {
        __android_log_print(ANDROID_LOG_WARN, "SDL", "SDL: Couldn't locate Java callbacks, check that they're named and typed correctly");
    }
}

// Resize
extern "C" void Java_org_libsdl_app_SDLActivity_onNativeResize(
                                    JNIEnv* env, jclass jcls,
                                    jint width, jint height, jint format)
{
    Android_SetScreenResolution(width, height, format);
}

// Keydown
extern "C" void Java_org_libsdl_app_SDLActivity_onNativeKeyDown(
                                    JNIEnv* env, jclass jcls, jint keycode)
{
    Android_OnKeyDown(keycode);
}

// Keyup
extern "C" void Java_org_libsdl_app_SDLActivity_onNativeKeyUp(
                                    JNIEnv* env, jclass jcls, jint keycode)
{
    Android_OnKeyUp(keycode);
}

// Touch
extern "C" void Java_org_libsdl_app_SDLActivity_onNativeTouch(
                                    JNIEnv* env, jclass jcls,
                                    jint action, jfloat x, jfloat y, jfloat p)
{
    Android_OnTouch(action, x, y, p);
}

// Accelerometer
extern "C" void Java_org_libsdl_app_SDLActivity_onNativeAccel(
                                    JNIEnv* env, jclass jcls,
                                    jfloat x, jfloat y, jfloat z)
{
    fLastAccelerometer[0] = x;
    fLastAccelerometer[1] = y;
    fLastAccelerometer[2] = z;   
}

// Quit
extern "C" void Java_org_libsdl_app_SDLActivity_nativeQuit(
                                    JNIEnv* env, jclass cls)
{    
    // Inject a SDL_QUIT event
    SDL_SendQuit();
}

extern "C" void Java_org_libsdl_app_SDLActivity_nativeRunAudioThread(
                                    JNIEnv* env, jclass cls)
{
    /* This is the audio thread, with a different environment */
    mAudioEnv = env;

    Android_RunAudioThread();
}


/*******************************************************************************
             Functions called by SDL into Java
*******************************************************************************/
extern "C" SDL_bool Android_JNI_CreateContext(int majorVersion, int minorVersion)
{
    if (mEnv->CallStaticBooleanMethod(mActivityClass, midCreateGLContext, majorVersion, minorVersion)) {
        return SDL_TRUE;
    } else {
        return SDL_FALSE;
    }
}

extern "C" void Android_JNI_SwapWindow()
{
    mEnv->CallStaticVoidMethod(mActivityClass, midFlipBuffers); 
}

extern "C" void Android_JNI_SetActivityTitle(const char *title)
{
    jmethodID mid;

    mid = mEnv->GetStaticMethodID(mActivityClass,"setActivityTitle","(Ljava/lang/String;)V");
    if (mid) {
        mEnv->CallStaticVoidMethod(mActivityClass, mid, mEnv->NewStringUTF(title));
    }
}

extern "C" void Android_JNI_GetAccelerometerValues(float values[3])
{
    int i;
    for (i = 0; i < 3; ++i) {
        values[i] = fLastAccelerometer[i];
    }
}

//
// Audio support
//
static jboolean audioBuffer16Bit = JNI_FALSE;
static jboolean audioBufferStereo = JNI_FALSE;
static jobject audioBuffer = NULL;
static void* audioBufferPinned = NULL;

extern "C" int Android_JNI_OpenAudioDevice(int sampleRate, int is16Bit, int channelCount, int desiredBufferFrames)
{
    int audioBufferFrames;

    __android_log_print(ANDROID_LOG_VERBOSE, "SDL", "SDL audio: opening device");
    audioBuffer16Bit = is16Bit;
    audioBufferStereo = channelCount > 1;

    audioBuffer = mEnv->CallStaticObjectMethod(mActivityClass, midAudioInit, sampleRate, audioBuffer16Bit, audioBufferStereo, desiredBufferFrames);

    if (audioBuffer == NULL) {
        __android_log_print(ANDROID_LOG_WARN, "SDL", "SDL audio: didn't get back a good audio buffer!");
        return 0;
    }
    audioBuffer = mEnv->NewGlobalRef(audioBuffer);

    jboolean isCopy = JNI_FALSE;
    if (audioBuffer16Bit) {
        audioBufferPinned = mEnv->GetShortArrayElements((jshortArray)audioBuffer, &isCopy);
        audioBufferFrames = mEnv->GetArrayLength((jshortArray)audioBuffer);
    } else {
        audioBufferPinned = mEnv->GetByteArrayElements((jbyteArray)audioBuffer, &isCopy);
        audioBufferFrames = mEnv->GetArrayLength((jbyteArray)audioBuffer);
    }
    if (audioBufferStereo) {
        audioBufferFrames /= 2;
    }

    return audioBufferFrames;
}

extern "C" void * Android_JNI_GetAudioBuffer()
{
    return audioBufferPinned;
}

extern "C" void Android_JNI_WriteAudioBuffer()
{
    if (audioBuffer16Bit) {
        mAudioEnv->ReleaseShortArrayElements((jshortArray)audioBuffer, (jshort *)audioBufferPinned, JNI_COMMIT);
        mAudioEnv->CallStaticVoidMethod(mActivityClass, midAudioWriteShortBuffer, (jshortArray)audioBuffer);
    } else {
        mAudioEnv->ReleaseByteArrayElements((jbyteArray)audioBuffer, (jbyte *)audioBufferPinned, JNI_COMMIT);
        mAudioEnv->CallStaticVoidMethod(mActivityClass, midAudioWriteByteBuffer, (jbyteArray)audioBuffer);
    }

    /* JNI_COMMIT means the changes are committed to the VM but the buffer remains pinned */
}

extern "C" void Android_JNI_CloseAudioDevice()
{
    mEnv->CallStaticVoidMethod(mActivityClass, midAudioQuit); 

    if (audioBuffer) {
        mEnv->DeleteGlobalRef(audioBuffer);
        audioBuffer = NULL;
        audioBufferPinned = NULL;
    }
}

/* vi: set ts=4 sw=4 expandtab: */
