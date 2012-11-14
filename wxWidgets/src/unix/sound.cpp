/////////////////////////////////////////////////////////////////////////////
// Name:        src/unix/sound.cpp
// Purpose:     wxSound
// Author:      Marcel Rasche, Vaclav Slavik
// Modified by:
// Created:     25/10/98
// RCS-ID:      $Id: sound.cpp 41020 2006-09-05 20:47:48Z VZ $
// Copyright:   (c) Julian Smart, Open Source Applications Foundation
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// for compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#if defined(__BORLANDC__)
    #pragma hdrstop
#endif

#if wxUSE_SOUND

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#ifdef HAVE_SYS_SOUNDCARD_H
#include <sys/soundcard.h>
#endif

#ifndef WX_PRECOMP
    #include "wx/event.h"
    #include "wx/intl.h"
    #include "wx/log.h"
    #include "wx/module.h"
#endif

#include "wx/thread.h"
#include "wx/file.h"
#include "wx/sound.h"
#include "wx/dynlib.h"


#if wxUSE_THREADS
// mutex for all wxSound's synchronization
static wxMutex gs_soundMutex;
#endif

// ----------------------------------------------------------------------------
// wxSoundData
// ----------------------------------------------------------------------------

void wxSoundData::IncRef()
{
#if wxUSE_THREADS
    wxMutexLocker locker(gs_soundMutex);
#endif
    m_refCnt++;
}

void wxSoundData::DecRef()
{
#if wxUSE_THREADS
    wxMutexLocker locker(gs_soundMutex);
#endif
    if (--m_refCnt == 0)
        delete this;
}

wxSoundData::~wxSoundData()
{
    delete[] m_dataWithHeader;
}


// ----------------------------------------------------------------------------
// wxSoundBackendNull, used in absence of audio API or card
// ----------------------------------------------------------------------------

class wxSoundBackendNull : public wxSoundBackend
{
public:
    wxString GetName() const { return _("No sound"); }
    int GetPriority() const { return 0; }
    bool IsAvailable() const { return true; }
    bool HasNativeAsyncPlayback() const { return true; }
    bool Play(wxSoundData *WXUNUSED(data), unsigned WXUNUSED(flags),
              volatile wxSoundPlaybackStatus *WXUNUSED(status))
        { return true; }
    void Stop() {}
    bool IsPlaying() const { return false; }
};


// ----------------------------------------------------------------------------
// wxSoundBackendOSS, for Linux
// ----------------------------------------------------------------------------

#ifdef HAVE_SYS_SOUNDCARD_H

#ifndef AUDIODEV
#define AUDIODEV   "/dev/dsp"    // Default path for audio device
#endif

class wxSoundBackendOSS : public wxSoundBackend
{
public:
    wxString GetName() const { return _T("Open Sound System"); }
    int GetPriority() const { return 10; }
    bool IsAvailable() const;
    bool HasNativeAsyncPlayback() const { return false; }
    bool Play(wxSoundData *data, unsigned flags,
              volatile wxSoundPlaybackStatus *status);
    void Stop() {}
    bool IsPlaying() const { return false; }

private:
    int OpenDSP(const wxSoundData *data);
    bool InitDSP(int dev, const wxSoundData *data);

    int m_DSPblkSize;        // Size of the DSP buffer
    bool m_needConversion;
};

bool wxSoundBackendOSS::IsAvailable() const
{
    int fd;
    fd = open(AUDIODEV, O_WRONLY | O_NONBLOCK);
    if (fd < 0)
        return false;
    close(fd);
    return true;
}

bool wxSoundBackendOSS::Play(wxSoundData *data, unsigned flags,
                             volatile wxSoundPlaybackStatus *status)
{
    int dev = OpenDSP(data);

    if (dev < 0)
        return false;

    ioctl(dev, SNDCTL_DSP_SYNC, 0);

    do
    {
        bool play = true;
        int i;
        unsigned l = 0;
        size_t datasize = data->m_dataBytes;

        do
        {
            if (status->m_stopRequested)
            {
                wxLogTrace(_T("sound"), _T("playback stopped"));
                close(dev);
                return true;
            }

            i= (int)((l + m_DSPblkSize) < datasize ?
                     m_DSPblkSize : (datasize - l));
            if (write(dev, &data->m_data[l], i) != i)
            {
                play = false;
            }
            l += i;
        } while (play && l < datasize);
    } while (flags & wxSOUND_LOOP);

    close(dev);
    return true;
}

int wxSoundBackendOSS::OpenDSP(const wxSoundData *data)
{
    int dev = -1;

    if ((dev = open(AUDIODEV, O_WRONLY, 0)) <0)
        return -1;

    if (!InitDSP(dev, data) || m_needConversion)
    {
        close(dev);
        return -1;
    }

    return dev;
}


bool wxSoundBackendOSS::InitDSP(int dev, const wxSoundData *data)
{
    unsigned tmp;

    // Reset the dsp
    if (ioctl(dev, SNDCTL_DSP_RESET, 0) < 0)
    {
        wxLogTrace(_T("sound"), _T("unable to reset dsp"));
        return false;
    }

    m_needConversion = false;

    tmp = data->m_bitsPerSample;
    if (ioctl(dev, SNDCTL_DSP_SAMPLESIZE, &tmp) < 0)
    {
        wxLogTrace(_T("sound"), _T("IOCTL failure (SNDCTL_DSP_SAMPLESIZE)"));
        return false;
    }
    if (tmp != data->m_bitsPerSample)
    {
        wxLogTrace(_T("sound"),
                   _T("Unable to set DSP sample size to %d (wants %d)"),
                   data->m_bitsPerSample, tmp);
        m_needConversion = true;
    }

    unsigned stereo = data->m_channels == 1 ? 0 : 1;
    tmp = stereo;
    if (ioctl(dev, SNDCTL_DSP_STEREO, &tmp) < 0)
    {
        wxLogTrace(_T("sound"), _T("IOCTL failure (SNDCTL_DSP_STEREO)"));
        return false;
    }
    if (tmp != stereo)
    {
        wxLogTrace(_T("sound"), _T("Unable to set DSP to %s."), stereo?  _T("stereo"):_T("mono"));
        m_needConversion = true;
    }

    tmp = data->m_samplingRate;
    if (ioctl(dev, SNDCTL_DSP_SPEED, &tmp) < 0)
    {
        wxLogTrace(_T("sound"), _T("IOCTL failure (SNDCTL_DSP_SPEED)"));
       return false;
    }
    if (tmp != data->m_samplingRate)
    {
        // If the rate the sound card is using is not within 1% of what the
        // data specified then override the data setting.  The only reason not
        // to always override this is because of clock-rounding
        // problems. Sound cards will sometimes use things like 44101 when you
        // ask for 44100.  No need overriding this and having strange output
        // file rates for something that we can't hear anyways.
        if (data->m_samplingRate - tmp > (tmp * .01) ||
            tmp - data->m_samplingRate > (tmp * .01)) {
            wxLogTrace(_T("sound"),
                       _T("Unable to set DSP sampling rate to %d (wants %d)"),
                       data->m_samplingRate, tmp);
            m_needConversion = true;
        }
    }

    // Do this last because some drivers can adjust the buffer sized based on
    // the sampling rate, etc.
    if (ioctl(dev, SNDCTL_DSP_GETBLKSIZE, &m_DSPblkSize) < 0)
    {
        wxLogTrace(_T("sound"), _T("IOCTL failure (SNDCTL_DSP_GETBLKSIZE)"));
        return false;
    }
    return true;
}

#endif // HAVE_SYS_SOUNDCARD_H

// ----------------------------------------------------------------------------
// wxSoundSyncOnlyAdaptor
// ----------------------------------------------------------------------------

#if wxUSE_THREADS

class wxSoundSyncOnlyAdaptor;

// this class manages asynchronous playback of audio if the backend doesn't
// support it natively (e.g. OSS backend)
class wxSoundAsyncPlaybackThread : public wxThread
{
public:
    wxSoundAsyncPlaybackThread(wxSoundSyncOnlyAdaptor *adaptor,
                              wxSoundData *data, unsigned flags)
        : wxThread(), m_adapt(adaptor), m_data(data), m_flags(flags) {}
    virtual ExitCode Entry();

protected:
    wxSoundSyncOnlyAdaptor *m_adapt;
    wxSoundData *m_data;
    unsigned m_flags;
};

#endif // wxUSE_THREADS

// This class turns wxSoundBackend that doesn't support asynchronous playback
// into one that does
class wxSoundSyncOnlyAdaptor : public wxSoundBackend
{
public:
    wxSoundSyncOnlyAdaptor(wxSoundBackend *backend)
        : m_backend(backend), m_playing(false) {}
    virtual ~wxSoundSyncOnlyAdaptor()
    {
        delete m_backend;
    }
    wxString GetName() const
    {
        return m_backend->GetName();
    }
    int GetPriority() const
    {
        return m_backend->GetPriority();
    }
    bool IsAvailable() const
    {
        return m_backend->IsAvailable();
    }
    bool HasNativeAsyncPlayback() const
    {
        return true;
    }
    bool Play(wxSoundData *data, unsigned flags,
              volatile wxSoundPlaybackStatus *status);
    void Stop();
    bool IsPlaying() const;

private:
    friend class wxSoundAsyncPlaybackThread;

    wxSoundBackend *m_backend;
    bool m_playing;
#if wxUSE_THREADS
    // player thread holds this mutex and releases it after it finishes
    // playing, so that the main thread knows when it can play sound
    wxMutex m_mutexRightToPlay;
    wxSoundPlaybackStatus m_status;
#endif
};


#if wxUSE_THREADS
wxThread::ExitCode wxSoundAsyncPlaybackThread::Entry()
{
    m_adapt->m_backend->Play(m_data, m_flags & ~wxSOUND_ASYNC,
                             &m_adapt->m_status);

    m_data->DecRef();
    m_adapt->m_playing = false;
    m_adapt->m_mutexRightToPlay.Unlock();
    wxLogTrace(_T("sound"), _T("terminated async playback thread"));
    return 0;
}
#endif

bool wxSoundSyncOnlyAdaptor::Play(wxSoundData *data, unsigned flags,
                                  volatile wxSoundPlaybackStatus *status)
{
    Stop();
    if (flags & wxSOUND_ASYNC)
    {
#if wxUSE_THREADS
        m_mutexRightToPlay.Lock();
        m_status.m_playing = true;
        m_status.m_stopRequested = false;
        data->IncRef();
        wxThread *th = new wxSoundAsyncPlaybackThread(this, data, flags);
        th->Create();
        th->Run();
        wxLogTrace(_T("sound"), _T("launched async playback thread"));
        return true;
#else
        wxLogError(_("Unable to play sound asynchronously."));
        return false;
#endif
    }
    else
    {
#if wxUSE_THREADS
        m_mutexRightToPlay.Lock();
#endif
        bool rv = m_backend->Play(data, flags, status);
#if wxUSE_THREADS
        m_mutexRightToPlay.Unlock();
#endif
        return rv;
    }
}

void wxSoundSyncOnlyAdaptor::Stop()
{
    wxLogTrace(_T("sound"), _T("asking audio to stop"));

#if wxUSE_THREADS
    // tell the player thread (if running) to stop playback ASAP:
    m_status.m_stopRequested = true;

    // acquire the mutex to be sure no sound is being played, then
    // release it because we don't need it for anything (the effect of this
    // is that calling thread will wait until playback thread reacts to
    // our request to interrupt playback):
    m_mutexRightToPlay.Lock();
    m_mutexRightToPlay.Unlock();
    wxLogTrace(_T("sound"), _T("audio was stopped"));
#endif
}

bool wxSoundSyncOnlyAdaptor::IsPlaying() const
{
#if wxUSE_THREADS
    return m_status.m_playing;
#else
    return false;
#endif
}


// ----------------------------------------------------------------------------
// wxSound
// ----------------------------------------------------------------------------

wxSoundBackend *wxSound::ms_backend = NULL;

// FIXME - temporary, until we have plugins architecture
#if wxUSE_LIBSDL
    #if wxUSE_PLUGINS
        wxDynamicLibrary *wxSound::ms_backendSDL = NULL;
    #else
        extern "C" wxSoundBackend *wxCreateSoundBackendSDL();
    #endif
#endif

wxSound::wxSound() : m_data(NULL)
{
}

wxSound::wxSound(const wxString& sFileName, bool isResource) : m_data(NULL)
{
    Create(sFileName, isResource);
}

wxSound::wxSound(int size, const wxByte* data) : m_data(NULL)
{
    Create(size, data);
}

wxSound::~wxSound()
{
    Free();
}

bool wxSound::Create(const wxString& fileName,
                     bool WXUNUSED_UNLESS_DEBUG(isResource))
{
    wxASSERT_MSG( !isResource,
             _T("Loading sound from resources is only supported on Windows") );

    Free();

    wxFile fileWave;
    if (!fileWave.Open(fileName, wxFile::read))
    {
        return false;
    }

    wxFileOffset lenOrig = fileWave.Length();
    if ( lenOrig == wxInvalidOffset )
        return false;

    size_t len = wx_truncate_cast(size_t, lenOrig);
    wxUint8 *data = new wxUint8[len];
    if ( fileWave.Read(data, len) != lenOrig )
    {
        delete [] data;
        wxLogError(_("Couldn't load sound data from '%s'."), fileName.c_str());
        return false;
    }

    if (!LoadWAV(data, len, false))
    {
        delete [] data;
        wxLogError(_("Sound file '%s' is in unsupported format."),
                   fileName.c_str());
        return false;
    }

    return true;
}

bool wxSound::Create(int size, const wxByte* data)
{
    wxASSERT( data != NULL );

    Free();
    if (!LoadWAV(data, size, true))
    {
        wxLogError(_("Sound data are in unsupported format."));
        return false;
    }
    return true;
}

/*static*/ void wxSound::EnsureBackend()
{
    if (!ms_backend)
    {
        // FIXME -- make this fully dynamic when plugins architecture is in
        // place
#if wxUSE_LIBSDL
        //if (!ms_backend)
        {
#if !wxUSE_PLUGINS
            ms_backend = wxCreateSoundBackendSDL();
#else
            wxString dllname;
            dllname.Printf(_T("%s/%s"),
                wxDynamicLibrary::GetPluginsDirectory().c_str(),
                wxDynamicLibrary::CanonicalizePluginName(
                    _T("sound_sdl"), wxDL_PLUGIN_BASE).c_str());
            wxLogTrace(_T("sound"),
                       _T("trying to load SDL plugin from '%s'..."),
                       dllname.c_str());
            wxLogNull null;
            ms_backendSDL = new wxDynamicLibrary(dllname, wxDL_NOW);
            if (!ms_backendSDL->IsLoaded())
            {
                wxDELETE(ms_backendSDL);
            }
            else
            {
                typedef wxSoundBackend *(*wxCreateSoundBackend_t)();
                wxDYNLIB_FUNCTION(wxCreateSoundBackend_t,
                                  wxCreateSoundBackendSDL, *ms_backendSDL);
                if (pfnwxCreateSoundBackendSDL)
                {
                    ms_backend = (*pfnwxCreateSoundBackendSDL)();
                }
            }
#endif
            if (ms_backend && !ms_backend->IsAvailable())
            {
                wxDELETE(ms_backend);
            }
        }
#endif

#ifdef HAVE_SYS_SOUNDCARD_H
        if (!ms_backend)
        {
            ms_backend = new wxSoundBackendOSS();
            if (!ms_backend->IsAvailable())
            {
                wxDELETE(ms_backend);
            }
        }
#endif

        if (!ms_backend)
            ms_backend = new wxSoundBackendNull();

        if (!ms_backend->HasNativeAsyncPlayback())
            ms_backend = new wxSoundSyncOnlyAdaptor(ms_backend);

        wxLogTrace(_T("sound"),
                   _T("using backend '%s'"), ms_backend->GetName().c_str());
    }
}

/*static*/ void wxSound::UnloadBackend()
{
    if (ms_backend)
    {
        wxLogTrace(_T("sound"), _T("unloading backend"));

        Stop();

        delete ms_backend;
        ms_backend = NULL;
#if wxUSE_LIBSDL && wxUSE_PLUGINS
        delete ms_backendSDL;
#endif
    }
}

bool wxSound::DoPlay(unsigned flags) const
{
    wxCHECK_MSG( IsOk(), false, _T("Attempt to play invalid wave data") );

    EnsureBackend();
    wxSoundPlaybackStatus status;
    status.m_playing = true;
    status.m_stopRequested = false;
    return ms_backend->Play(m_data, flags, &status);
}

/*static*/ void wxSound::Stop()
{
    if (ms_backend)
        ms_backend->Stop();
}

/*static*/ bool wxSound::IsPlaying()
{
    if (ms_backend)
        return ms_backend->IsPlaying();
    else
        return false;
}

void wxSound::Free()
{
    if (m_data)
        m_data->DecRef();
}

typedef struct
{
    wxUint32      uiSize;
    wxUint16      uiFormatTag;
    wxUint16      uiChannels;
    wxUint32      ulSamplesPerSec;
    wxUint32      ulAvgBytesPerSec;
    wxUint16      uiBlockAlign;
    wxUint16      uiBitsPerSample;
} WAVEFORMAT;

#define WAVE_FORMAT_PCM  1
#define WAVE_INDEX       8
#define FMT_INDEX       12

bool wxSound::LoadWAV(const wxUint8 *data, size_t length, bool copyData)
{
    // the simplest wave file header consists of 44 bytes:
    //
    //      0   "RIFF"
    //      4   file size - 8
    //      8   "WAVE"
    //
    //      12  "fmt "
    //      16  chunk size                  |
    //      20  format tag                  |
    //      22  number of channels          |
    //      24  sample rate                 | WAVEFORMAT
    //      28  average bytes per second    |
    //      32  bytes per frame             |
    //      34  bits per sample             |
    //
    //      36  "data"
    //      40  number of data bytes
    //      44  (wave signal) data
    //
    // so check that we have at least as much
    if ( length < 44 )
        return false;

    WAVEFORMAT waveformat;
    memcpy(&waveformat, &data[FMT_INDEX + 4], sizeof(WAVEFORMAT));
    waveformat.uiSize = wxUINT32_SWAP_ON_BE(waveformat.uiSize);
    waveformat.uiFormatTag = wxUINT16_SWAP_ON_BE(waveformat.uiFormatTag);
    waveformat.uiChannels = wxUINT16_SWAP_ON_BE(waveformat.uiChannels);
    waveformat.ulSamplesPerSec = wxUINT32_SWAP_ON_BE(waveformat.ulSamplesPerSec);
    waveformat.ulAvgBytesPerSec = wxUINT32_SWAP_ON_BE(waveformat.ulAvgBytesPerSec);
    waveformat.uiBlockAlign = wxUINT16_SWAP_ON_BE(waveformat.uiBlockAlign);
    waveformat.uiBitsPerSample = wxUINT16_SWAP_ON_BE(waveformat.uiBitsPerSample);

    // get the sound data size
    wxUint32 ul;
    memcpy(&ul, &data[FMT_INDEX + waveformat.uiSize + 12], 4);
    ul = wxUINT32_SWAP_ON_BE(ul);

    if ( length < ul + FMT_INDEX + waveformat.uiSize + 16 )
        return false;

    if (memcmp(data, "RIFF", 4) != 0)
        return false;
    if (memcmp(&data[WAVE_INDEX], "WAVE", 4) != 0)
        return false;
    if (memcmp(&data[FMT_INDEX], "fmt ", 4) != 0)
        return false;
    if (memcmp(&data[FMT_INDEX + waveformat.uiSize + 8], "data", 4) != 0)
        return false;

    if (waveformat.uiFormatTag != WAVE_FORMAT_PCM)
        return false;

    if (waveformat.ulSamplesPerSec !=
        waveformat.ulAvgBytesPerSec / waveformat.uiBlockAlign)
        return false;

    m_data = new wxSoundData;
    m_data->m_channels = waveformat.uiChannels;
    m_data->m_samplingRate = waveformat.ulSamplesPerSec;
    m_data->m_bitsPerSample = waveformat.uiBitsPerSample;
    m_data->m_samples = ul / (m_data->m_channels * m_data->m_bitsPerSample / 8);
    m_data->m_dataBytes = ul;

    if (copyData)
    {
        m_data->m_dataWithHeader = new wxUint8[length];
        memcpy(m_data->m_dataWithHeader, data, length);
    }
    else
        m_data->m_dataWithHeader = (wxUint8*)data;

    m_data->m_data =
        (&m_data->m_dataWithHeader[FMT_INDEX + waveformat.uiSize + 8]);

    return true;
}


// ----------------------------------------------------------------------------
// wxSoundCleanupModule
// ----------------------------------------------------------------------------

class wxSoundCleanupModule: public wxModule
{
public:
    bool OnInit() { return true; }
    void OnExit() { wxSound::UnloadBackend(); }
    DECLARE_DYNAMIC_CLASS(wxSoundCleanupModule)
};

IMPLEMENT_DYNAMIC_CLASS(wxSoundCleanupModule, wxModule)

#endif
