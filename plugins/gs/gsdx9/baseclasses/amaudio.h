//------------------------------------------------------------------------------
// File: AMAudio.h
//
// Desc: Audio related definitions and interfaces for ActiveMovie.
//
// Copyright (c) 1992-2001, Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------------------------


#ifndef __AMAUDIO__
#define __AMAUDIO__

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include <mmsystem.h>
#include <dsound.h>

// This is the interface the audio renderer supports to give the application
// access to the direct sound object and the buffers it is using, to allow the
// application to use things like the 3D features of Direct Sound for the
// soundtrack of a movie being played with Active Movie.

// be nice to our friends in C
#undef INTERFACE
#define INTERFACE IAMDirectSound

DECLARE_INTERFACE_(IAMDirectSound,IUnknown)
{
    /* IUnknown methods */

    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID *ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;

    /* IAMDirectSound methods */

    STDMETHOD(GetDirectSoundInterface)(THIS_ LPDIRECTSOUND *lplpds) PURE;
    STDMETHOD(GetPrimaryBufferInterface)(THIS_ LPDIRECTSOUNDBUFFER *lplpdsb) PURE;
    STDMETHOD(GetSecondaryBufferInterface)(THIS_ LPDIRECTSOUNDBUFFER *lplpdsb) PURE;
    STDMETHOD(ReleaseDirectSoundInterface)(THIS_ LPDIRECTSOUND lpds) PURE;
    STDMETHOD(ReleasePrimaryBufferInterface)(THIS_ LPDIRECTSOUNDBUFFER lpdsb) PURE;
    STDMETHOD(ReleaseSecondaryBufferInterface)(THIS_ LPDIRECTSOUNDBUFFER lpdsb) PURE;
    STDMETHOD(SetFocusWindow)(THIS_ HWND, BOOL) PURE ;
    STDMETHOD(GetFocusWindow)(THIS_ HWND *, BOOL*) PURE ;
};


#ifdef __cplusplus
}
#endif // __cplusplus
#endif // __AMAUDIO__

