/////////////////////////////////////////////////////////////////////////////
// Name:        wx/unix/sound.h
// Purpose:     wxSound class
// Author:      Julian Smart, Vaclav Slavik
// Modified by:
// Created:     25/10/98
// RCS-ID:      $Id: sound.h 42115 2006-10-19 13:09:48Z VZ $
// Copyright:   (c) Julian Smart, Vaclav Slavik
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_SOUND_H_
#define _WX_SOUND_H_

#include "wx/defs.h"

#if wxUSE_SOUND

#include "wx/object.h"

// ----------------------------------------------------------------------------
// wxSound: simple audio playback class
// ----------------------------------------------------------------------------

class WXDLLIMPEXP_ADV wxSoundBackend;
class WXDLLIMPEXP_ADV wxSound;
class WXDLLIMPEXP_BASE wxDynamicLibrary;

/// Sound data, as loaded from .wav file:
class WXDLLIMPEXP_ADV wxSoundData
{
public:
    wxSoundData() : m_refCnt(1) {}
    void IncRef();
    void DecRef();

    // .wav header information:
    unsigned m_channels;       // num of channels (mono:1, stereo:2)
    unsigned m_samplingRate;
    unsigned m_bitsPerSample;  // if 8, then m_data contains unsigned 8bit
                               // samples (wxUint8), if 16 then signed 16bit
                               // (wxInt16)
    unsigned m_samples;        // length in samples:

    // wave data:
    size_t   m_dataBytes;
    wxUint8 *m_data;           // m_dataBytes bytes of data

private:
    ~wxSoundData();
    unsigned m_refCnt;
    wxUint8 *m_dataWithHeader; // ditto, but prefixed with .wav header
    friend class wxSound;
};


/// Simple sound class:
class WXDLLIMPEXP_ADV wxSound : public wxSoundBase
{
public:
    wxSound();
    wxSound(const wxString& fileName, bool isResource = false);
    wxSound(int size, const wxByte* data);
    virtual ~wxSound();

    // Create from resource or file
    bool Create(const wxString& fileName, bool isResource = false);
    // Create from data
    bool Create(int size, const wxByte* data);

    bool IsOk() const { return m_data != NULL; }

    // Stop playing any sound
    static void Stop();

    // Returns true if a sound is being played
    static bool IsPlaying();

    // for internal use
    static void UnloadBackend();

protected:
    bool DoPlay(unsigned flags) const;

    static void EnsureBackend();
    void Free();
    bool LoadWAV(const wxUint8 *data, size_t length, bool copyData);

    static wxSoundBackend *ms_backend;
#if wxUSE_LIBSDL && wxUSE_PLUGINS
    // FIXME - temporary, until we have plugins architecture
    static wxDynamicLibrary *ms_backendSDL;
#endif

private:
    wxSoundData *m_data;
};


// ----------------------------------------------------------------------------
// wxSoundBackend:
// ----------------------------------------------------------------------------

// This is interface to sound playing implementation. There are multiple
// sound architectures in use on Unix platforms and wxWidgets can use several
// of them for playback, depending on their availability at runtime; hence
// the need for backends. This class is for use by wxWidgets and people writing
// additional backends only, it is _not_ for use by applications!

// Structure that holds playback status information
struct wxSoundPlaybackStatus
{
    // playback is in progress
    bool m_playing;
    // main thread called wxSound::Stop()
    bool m_stopRequested;
};

// Audio backend interface
class WXDLLIMPEXP_ADV wxSoundBackend
{
public:
    virtual ~wxSoundBackend() {}

    // Returns the name of the backend (e.g. "Open Sound System")
    virtual wxString GetName() const = 0;

    // Returns priority (higher priority backends are tried first)
    virtual int GetPriority() const = 0;

    // Checks if the backend's audio system is available and the backend can
    // be used for playback
    virtual bool IsAvailable() const = 0;

    // Returns true if the backend is capable of playing sound asynchronously.
    // If false, then wxWidgets creates a playback thread and handles async
    // playback, otherwise it is left up to the backend (will usually be more
    // effective).
    virtual bool HasNativeAsyncPlayback() const = 0;

    // Plays the sound. flags are same flags as those passed to wxSound::Play.
    // The function should periodically check the value of
    // status->m_stopRequested and terminate if it is set to true (it may
    // be modified by another thread)
    virtual bool Play(wxSoundData *data, unsigned flags,
                      volatile wxSoundPlaybackStatus *status) = 0;

    // Stops playback (if something is played).
    virtual void Stop() = 0;

    // Returns true if the backend is playing anything at the moment.
    // (This method is never called for backends that don't support async
    // playback.)
    virtual bool IsPlaying() const = 0;
};


#endif // wxUSE_SOUND

#endif // _WX_SOUND_H_

