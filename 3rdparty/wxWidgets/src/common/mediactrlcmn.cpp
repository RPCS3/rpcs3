/////////////////////////////////////////////////////////////////////////////
// Name:        src/common/mediactrl.cpp
// Purpose:     wxMediaCtrl common code
// Author:      Ryan Norton <wxprojects@comcast.net>
// Modified by:
// Created:     11/07/04
// RCS-ID:      $Id: mediactrlcmn.cpp 42816 2006-10-31 08:50:17Z RD $
// Copyright:   (c) Ryan Norton
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// TODO: Platform specific backend defaults?

//===========================================================================
// Definitions
//===========================================================================

//---------------------------------------------------------------------------
// Pre-compiled header stuff
//---------------------------------------------------------------------------

#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#if wxUSE_MEDIACTRL

#ifndef WX_PRECOMP
    #include "wx/hash.h"
#endif

//---------------------------------------------------------------------------
// Includes
//---------------------------------------------------------------------------
#include "wx/mediactrl.h"

//===========================================================================
//
// Implementation
//
//===========================================================================

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// RTTI and Event implementations
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

IMPLEMENT_CLASS(wxMediaCtrl, wxControl)
DEFINE_EVENT_TYPE(wxEVT_MEDIA_STATECHANGED)
DEFINE_EVENT_TYPE(wxEVT_MEDIA_PLAY)
DEFINE_EVENT_TYPE(wxEVT_MEDIA_PAUSE)
IMPLEMENT_CLASS(wxMediaBackend, wxObject)
IMPLEMENT_DYNAMIC_CLASS(wxMediaEvent, wxEvent)
DEFINE_EVENT_TYPE(wxEVT_MEDIA_FINISHED)
DEFINE_EVENT_TYPE(wxEVT_MEDIA_LOADED)
DEFINE_EVENT_TYPE(wxEVT_MEDIA_STOP)

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//  wxMediaCtrl
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//---------------------------------------------------------------------------
// wxMediaBackend Destructor
//
// This is here because the DARWIN gcc compiler badly screwed up and
// needs the destructor implementation in the source
//---------------------------------------------------------------------------
wxMediaBackend::~wxMediaBackend()
{
}

//---------------------------------------------------------------------------
// wxMediaCtrl::Create (file version)
// wxMediaCtrl::Create (URL version)
//
// Searches for a backend that is installed on the system (backends
// starting with lower characters in the alphabet are given priority),
// and creates the control from it
//
// This searches by searching the global RTTI hashtable, class by class,
// attempting to call CreateControl on each one found that is a derivative
// of wxMediaBackend - if it succeeded Create returns true, otherwise
// it keeps iterating through the hashmap.
//---------------------------------------------------------------------------
bool wxMediaCtrl::Create(wxWindow* parent, wxWindowID id,
                const wxString& fileName,
                const wxPoint& pos,
                const wxSize& size,
                long style,
                const wxString& szBackend,
                const wxValidator& validator,
                const wxString& name)
{
    if(!szBackend.empty())
    {
        wxClassInfo* pClassInfo = wxClassInfo::FindClass(szBackend);

        if(!pClassInfo || !DoCreate(pClassInfo, parent, id,
                                    pos, size, style, validator, name))
        {
            m_imp = NULL;
            return false;
        }

        if (!fileName.empty())
        {
            if (!Load(fileName))
            {
                delete m_imp;
                m_imp = NULL;
                return false;
            }
        }

        SetInitialSize(size);
        return true;
    }
    else
    {
        wxClassInfo::sm_classTable->BeginFind();

        wxClassInfo* classInfo;

        while((classInfo = NextBackend()) != NULL)
        {
            if(!DoCreate(classInfo, parent, id,
                         pos, size, style, validator, name))
                continue;

            if (!fileName.empty())
            {
                if (Load(fileName))
                {
                    SetInitialSize(size);
                    return true;
                }
                else
                    delete m_imp;
            }
            else
            {
                SetInitialSize(size);
                return true;
            }
        }

        m_imp = NULL;
        return false;
    }
}

bool wxMediaCtrl::Create(wxWindow* parent, wxWindowID id,
                         const wxURI& location,
                         const wxPoint& pos,
                         const wxSize& size,
                         long style,
                         const wxString& szBackend,
                         const wxValidator& validator,
                         const wxString& name)
{
    if(!szBackend.empty())
    {
        wxClassInfo* pClassInfo = wxClassInfo::FindClass(szBackend);
        if(!pClassInfo || !DoCreate(pClassInfo, parent, id,
                                    pos, size, style, validator, name))
        {
            m_imp = NULL;
            return false;
        }

        if (!Load(location))
        {
            delete m_imp;
            m_imp = NULL;
            return false;
        }

        SetInitialSize(size);
        return true;
    }
    else
    {
        wxClassInfo::sm_classTable->BeginFind();

        wxClassInfo* classInfo;

        while((classInfo = NextBackend()) != NULL)
        {
            if(!DoCreate(classInfo, parent, id,
                         pos, size, style, validator, name))
                continue;

            if (Load(location))
            {
                SetInitialSize(size);
                return true;
            }
            else
                delete m_imp;
        }

        m_imp = NULL;
        return false;
    }
}

//---------------------------------------------------------------------------
// wxMediaCtrl::DoCreate
//
// Attempts to create the control from a backend
//---------------------------------------------------------------------------
bool wxMediaCtrl::DoCreate(wxClassInfo* classInfo,
                            wxWindow* parent, wxWindowID id,
                            const wxPoint& pos,
                            const wxSize& size,
                            long style,
                            const wxValidator& validator,
                            const wxString& name)
{
    m_imp = (wxMediaBackend*)classInfo->CreateObject();

    if( m_imp->CreateControl(this, parent, id, pos, size,
                             style, validator, name) )
    {
        return true;
    }

    delete m_imp;
    return false;
}

//---------------------------------------------------------------------------
// wxMediaCtrl::NextBackend (static)
//
//
// Search through the RTTI hashmap one at a
// time, attempting to create each derivative
// of wxMediaBackend
//
//
// STL isn't compatible with and will have a compilation error
// on a wxNode, however, wxHashTable::compatibility_iterator is
// incompatible with the old 2.4 stable version - but since
// we're in 2.5+ only we don't need to worry about the new version
//---------------------------------------------------------------------------
wxClassInfo* wxMediaCtrl::NextBackend()
{
    wxHashTable::compatibility_iterator
            node = wxClassInfo::sm_classTable->Next();
    while (node)
    {
        wxClassInfo* classInfo = (wxClassInfo *)node->GetData();
        if ( classInfo->IsKindOf(CLASSINFO(wxMediaBackend)) &&
             classInfo != CLASSINFO(wxMediaBackend) )
        {
            return classInfo;
        }
        node = wxClassInfo::sm_classTable->Next();
    }

    //
    // Nope - couldn't successfully find one... fail
    //
    return NULL;
}


//---------------------------------------------------------------------------
// wxMediaCtrl Destructor
//
// Free up the backend if it exists
//---------------------------------------------------------------------------
wxMediaCtrl::~wxMediaCtrl()
{
    if (m_imp)
        delete m_imp;
}

//---------------------------------------------------------------------------
// wxMediaCtrl::Load (file version)
// wxMediaCtrl::Load (URL version)
// wxMediaCtrl::Load (URL & Proxy version)
// wxMediaCtrl::Load (wxInputStream version)
//
// Here we call load of the backend - keeping
// track of whether it was successful or not - which
// will determine which later method calls work
//---------------------------------------------------------------------------
bool wxMediaCtrl::Load(const wxString& fileName)
{
    if(m_imp)
        return (m_bLoaded = m_imp->Load(fileName));
    return false;
}

bool wxMediaCtrl::Load(const wxURI& location)
{
    if(m_imp)
        return (m_bLoaded = m_imp->Load(location));
    return false;
}

bool wxMediaCtrl::Load(const wxURI& location, const wxURI& proxy)
{
    if(m_imp)
        return (m_bLoaded = m_imp->Load(location, proxy));
    return false;
}

//---------------------------------------------------------------------------
// wxMediaCtrl::Play
// wxMediaCtrl::Pause
// wxMediaCtrl::Stop
// wxMediaCtrl::GetPlaybackRate
// wxMediaCtrl::SetPlaybackRate
// wxMediaCtrl::Seek --> SetPosition
// wxMediaCtrl::Tell --> GetPosition
// wxMediaCtrl::Length --> GetDuration
// wxMediaCtrl::GetState
// wxMediaCtrl::DoGetBestSize
// wxMediaCtrl::SetVolume
// wxMediaCtrl::GetVolume
// wxMediaCtrl::ShowInterface
// wxMediaCtrl::GetDownloadProgress
// wxMediaCtrl::GetDownloadTotal
//
// 1) Check to see whether the backend exists and is loading
// 2) Call the backend's version of the method, returning success
//    if the backend's version succeeds
//---------------------------------------------------------------------------
bool wxMediaCtrl::Play()
{
    if(m_imp && m_bLoaded)
        return m_imp->Play();
    return 0;
}

bool wxMediaCtrl::Pause()
{
    if(m_imp && m_bLoaded)
        return m_imp->Pause();
    return 0;
}

bool wxMediaCtrl::Stop()
{
    if(m_imp && m_bLoaded)
        return m_imp->Stop();
    return 0;
}

double wxMediaCtrl::GetPlaybackRate()
{
    if(m_imp && m_bLoaded)
        return m_imp->GetPlaybackRate();
    return 0;
}

bool wxMediaCtrl::SetPlaybackRate(double dRate)
{
    if(m_imp && m_bLoaded)
        return m_imp->SetPlaybackRate(dRate);
    return false;
}

wxFileOffset wxMediaCtrl::Seek(wxFileOffset where, wxSeekMode mode)
{
    wxFileOffset offset;

    switch (mode)
    {
    case wxFromStart:
        offset = where;
        break;
    case wxFromEnd:
        offset = Length() - where;
        break;
//    case wxFromCurrent:
    default:
        offset = Tell() + where;
        break;
    }

    if(m_imp && m_bLoaded && m_imp->SetPosition(offset))
        return offset;
    return wxInvalidOffset;
}

wxFileOffset wxMediaCtrl::Tell()
{
    if(m_imp && m_bLoaded)
        return (wxFileOffset) m_imp->GetPosition().ToLong();
    return wxInvalidOffset;
}

wxFileOffset wxMediaCtrl::Length()
{
    if(m_imp && m_bLoaded)
        return (wxFileOffset) m_imp->GetDuration().ToLong();
    return wxInvalidOffset;
}

wxMediaState wxMediaCtrl::GetState()
{
    if(m_imp && m_bLoaded)
        return m_imp->GetState();
    return wxMEDIASTATE_STOPPED;
}

wxSize wxMediaCtrl::DoGetBestSize() const
{
    if(m_imp)
        return m_imp->GetVideoSize();
    return wxSize(0,0);
}

double wxMediaCtrl::GetVolume()
{
    if(m_imp && m_bLoaded)
        return m_imp->GetVolume();
    return 0.0;
}

bool wxMediaCtrl::SetVolume(double dVolume)
{
    if(m_imp && m_bLoaded)
        return m_imp->SetVolume(dVolume);
    return false;
}

bool wxMediaCtrl::ShowPlayerControls(wxMediaCtrlPlayerControls flags)
{
    if(m_imp)
        return m_imp->ShowPlayerControls(flags);
    return false;
}

wxFileOffset wxMediaCtrl::GetDownloadProgress()
{
    if(m_imp && m_bLoaded)
        return (wxFileOffset) m_imp->GetDownloadProgress().ToLong();
    return wxInvalidOffset;
}

wxFileOffset wxMediaCtrl::GetDownloadTotal()
{
    if(m_imp && m_bLoaded)
        return (wxFileOffset) m_imp->GetDownloadTotal().ToLong();
    return wxInvalidOffset;
}

//---------------------------------------------------------------------------
// wxMediaCtrl::DoMoveWindow
//
// 1) Call parent's version so that our control's window moves where
//    it's supposed to
// 2) If the backend exists and is loaded, move the video
//    of the media to where our control's window is now located
//---------------------------------------------------------------------------
void wxMediaCtrl::DoMoveWindow(int x, int y, int w, int h)
{
    wxControl::DoMoveWindow(x,y,w,h);

    if(m_imp)
        m_imp->Move(x, y, w, h);
}

//---------------------------------------------------------------------------
// wxMediaCtrl::MacVisibilityChanged
//---------------------------------------------------------------------------
#ifdef __WXMAC__
void wxMediaCtrl::MacVisibilityChanged()
{
    wxControl::MacVisibilityChanged();

    if(m_imp)
        m_imp->MacVisibilityChanged();
}
#endif

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//  wxMediaBackendCommonBase
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void wxMediaBackendCommonBase::NotifyMovieSizeChanged()
{
    // our best size changed after opening a new file
    m_ctrl->InvalidateBestSize();
    m_ctrl->SetSize(m_ctrl->GetSize());

    // if the parent of the control has a sizer ask it to refresh our size
    wxWindow * const parent = m_ctrl->GetParent();
    if ( parent->GetSizer() )
    {
        m_ctrl->GetParent()->Layout();
        m_ctrl->GetParent()->Refresh();
        m_ctrl->GetParent()->Update();
    }
}

void wxMediaBackendCommonBase::NotifyMovieLoaded()
{
    NotifyMovieSizeChanged();

    // notify about movie being fully loaded
    QueueEvent(wxEVT_MEDIA_LOADED);
}

bool wxMediaBackendCommonBase::SendStopEvent()
{
    wxMediaEvent theEvent(wxEVT_MEDIA_STOP, m_ctrl->GetId());

    return !m_ctrl->ProcessEvent(theEvent) || theEvent.IsAllowed();
}

void wxMediaBackendCommonBase::QueueEvent(wxEventType evtType)
{
    wxMediaEvent theEvent(evtType, m_ctrl->GetId());
    m_ctrl->AddPendingEvent(theEvent);
}

void wxMediaBackendCommonBase::QueuePlayEvent()
{
    QueueEvent(wxEVT_MEDIA_STATECHANGED);
    QueueEvent(wxEVT_MEDIA_PLAY);
}

void wxMediaBackendCommonBase::QueuePauseEvent()
{
    QueueEvent(wxEVT_MEDIA_STATECHANGED);
    QueueEvent(wxEVT_MEDIA_PAUSE);
}

void wxMediaBackendCommonBase::QueueStopEvent()
{
    QueueEvent(wxEVT_MEDIA_STATECHANGED);
    QueueEvent(wxEVT_MEDIA_STOP);
}


//
// Force link default backends in -
// see http://wiki.wxwidgets.org/wiki.pl?RTTI
//
#include "wx/html/forcelnk.h"

#ifdef __WXMSW__ // MSW has huge backends so we do it seperately
FORCE_LINK(wxmediabackend_am)
FORCE_LINK(wxmediabackend_wmp10)
#else
FORCE_LINK(basewxmediabackends)
#endif

#endif //wxUSE_MEDIACTRL
