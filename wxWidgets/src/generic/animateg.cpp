///////////////////////////////////////////////////////////////////////////////
// Name:        animateg.cpp
// Purpose:     wxAnimation and wxAnimationCtrl
// Author:      Julian Smart and Guillermo Rodriguez Garcia
// Modified by: Francesco Montorsi
// Created:     13/8/99
// RCS-ID:      $Id: animateg.cpp 48085 2007-08-15 11:36:50Z VZ $
// Copyright:   (c) Julian Smart and Guillermo Rodriguez Garcia
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

#include "wx/wxprec.h"

#ifdef __BORLANDC__
  #pragma hdrstop
#endif  //__BORLANDC__

#if wxUSE_ANIMATIONCTRL && (!defined(__WXGTK20__) || defined(__WXUNIVERSAL__))

#include "wx/animate.h"

#ifndef WX_PRECOMP
    #include "wx/log.h"
    #include "wx/image.h"
    #include "wx/dcmemory.h"
    #include "wx/dcclient.h"
    #include "wx/module.h"
#endif

#include "wx/wfstream.h"
#include "wx/gifdecod.h"
#include "wx/anidecod.h"

#include "wx/listimpl.cpp"
WX_DEFINE_LIST(wxAnimationDecoderList)

wxAnimationDecoderList wxAnimation::sm_handlers;


// ----------------------------------------------------------------------------
// wxAnimation
// ----------------------------------------------------------------------------

IMPLEMENT_DYNAMIC_CLASS(wxAnimation, wxAnimationBase)
#define M_ANIMDATA      wx_static_cast(wxAnimationDecoder*, m_refData)

wxSize wxAnimation::GetSize() const
{
    wxCHECK_MSG( IsOk(), wxDefaultSize, wxT("invalid animation") );

    return M_ANIMDATA->GetAnimationSize();
}

unsigned int wxAnimation::GetFrameCount() const
{
    wxCHECK_MSG( IsOk(), 0, wxT("invalid animation") );

    return M_ANIMDATA->GetFrameCount();
}

wxImage wxAnimation::GetFrame(unsigned int i) const
{
    wxCHECK_MSG( IsOk(), wxNullImage, wxT("invalid animation") );

    wxImage ret;
    if (!M_ANIMDATA->ConvertToImage(i, &ret))
        return wxNullImage;
    return ret;
}

int wxAnimation::GetDelay(unsigned int i) const
{
    wxCHECK_MSG( IsOk(), 0, wxT("invalid animation") );

    return M_ANIMDATA->GetDelay(i);
}

wxPoint wxAnimation::GetFramePosition(unsigned int frame) const
{
    wxCHECK_MSG( IsOk(), wxDefaultPosition, wxT("invalid animation") );

    return M_ANIMDATA->GetFramePosition(frame);
}

wxSize wxAnimation::GetFrameSize(unsigned int frame) const
{
    wxCHECK_MSG( IsOk(), wxDefaultSize, wxT("invalid animation") );

    return M_ANIMDATA->GetFrameSize(frame);
}

wxAnimationDisposal wxAnimation::GetDisposalMethod(unsigned int frame) const
{
    wxCHECK_MSG( IsOk(), wxANIM_UNSPECIFIED, wxT("invalid animation") );

    return M_ANIMDATA->GetDisposalMethod(frame);
}

wxColour wxAnimation::GetTransparentColour(unsigned int frame) const
{
    wxCHECK_MSG( IsOk(), wxNullColour, wxT("invalid animation") );

    return M_ANIMDATA->GetTransparentColour(frame);
}

wxColour wxAnimation::GetBackgroundColour() const
{
    wxCHECK_MSG( IsOk(), wxNullColour, wxT("invalid animation") );

    return M_ANIMDATA->GetBackgroundColour();
}

bool wxAnimation::LoadFile(const wxString& filename, wxAnimationType type)
{
    wxFileInputStream stream(filename);
    if ( !stream.IsOk() )
        return false;

    return Load(stream, type);
}

bool wxAnimation::Load(wxInputStream &stream, wxAnimationType type)
{
    UnRef();

    const wxAnimationDecoder *handler;
    if ( type == wxANIMATION_TYPE_ANY )
    {
        for ( wxAnimationDecoderList::compatibility_iterator node = sm_handlers.GetFirst();
              node; node = node->GetNext() )
        {
            handler=(const wxAnimationDecoder*)node->GetData();

            if ( handler->CanRead(stream) )
            {
                // do a copy of the handler from the static list which we will own
                // as our reference data
                m_refData = handler->Clone();
                return M_ANIMDATA->Load(stream);
            }

        }

        wxLogWarning( _("No handler found for animation type.") );
        return false;
    }

    handler = FindHandler(type);

    // do a copy of the handler from the static list which we will own
    // as our reference data
    m_refData = handler->Clone();

    if (handler == NULL)
    {
        wxLogWarning( _("No animation handler for type %ld defined."), type );

        return false;
    }

    if (stream.IsSeekable() && !M_ANIMDATA->CanRead(stream))
    {
        wxLogError(_("Animation file is not of type %ld."), type);
        return false;
    }
    else
        return M_ANIMDATA->Load(stream);
}


// ----------------------------------------------------------------------------
// animation decoders
// ----------------------------------------------------------------------------

void wxAnimation::AddHandler( wxAnimationDecoder *handler )
{
    // Check for an existing handler of the type being added.
    if (FindHandler( handler->GetType() ) == 0)
    {
        sm_handlers.Append( handler );
    }
    else
    {
        // This is not documented behaviour, merely the simplest 'fix'
        // for preventing duplicate additions.  If someone ever has
        // a good reason to add and remove duplicate handlers (and they
        // may) we should probably refcount the duplicates.

        wxLogDebug( _T("Adding duplicate animation handler for '%d' type"),
                    handler->GetType() );
        delete handler;
    }
}

void wxAnimation::InsertHandler( wxAnimationDecoder *handler )
{
    // Check for an existing handler of the type being added.
    if (FindHandler( handler->GetType() ) == 0)
    {
        sm_handlers.Insert( handler );
    }
    else
    {
        // see AddHandler for additional comments.
        wxLogDebug( _T("Inserting duplicate animation handler for '%d' type"),
                    handler->GetType() );
        delete handler;
    }
}

const wxAnimationDecoder *wxAnimation::FindHandler( wxAnimationType animType )
{
    wxAnimationDecoderList::compatibility_iterator node = sm_handlers.GetFirst();
    while (node)
    {
        const wxAnimationDecoder *handler = (const wxAnimationDecoder *)node->GetData();
        if (handler->GetType() == animType) return handler;
        node = node->GetNext();
    }
    return 0;
}

void wxAnimation::InitStandardHandlers()
{
#if wxUSE_GIF
    AddHandler(new wxGIFDecoder);
#endif // wxUSE_GIF
#if wxUSE_ICO_CUR
    AddHandler(new wxANIDecoder);
#endif // wxUSE_ICO_CUR
}

void wxAnimation::CleanUpHandlers()
{
    wxAnimationDecoderList::compatibility_iterator node = sm_handlers.GetFirst();
    while (node)
    {
        wxAnimationDecoder *handler = (wxAnimationDecoder *)node->GetData();
        wxAnimationDecoderList::compatibility_iterator next = node->GetNext();
        delete handler;
        node = next;
    }

    sm_handlers.Clear();
}


// A module to allow wxAnimation initialization/cleanup
// without calling these functions from app.cpp or from
// the user's application.

class wxAnimationModule: public wxModule
{
DECLARE_DYNAMIC_CLASS(wxAnimationModule)
public:
    wxAnimationModule() {}
    bool OnInit() { wxAnimation::InitStandardHandlers(); return true; }
    void OnExit() { wxAnimation::CleanUpHandlers(); }
};

IMPLEMENT_DYNAMIC_CLASS(wxAnimationModule, wxModule)


// ----------------------------------------------------------------------------
// wxAnimationCtrl
// ----------------------------------------------------------------------------

IMPLEMENT_CLASS(wxAnimationCtrl, wxAnimationCtrlBase)
BEGIN_EVENT_TABLE(wxAnimationCtrl, wxAnimationCtrlBase)
    EVT_PAINT(wxAnimationCtrl::OnPaint)
    EVT_SIZE(wxAnimationCtrl::OnSize)
    EVT_TIMER(wxID_ANY, wxAnimationCtrl::OnTimer)
END_EVENT_TABLE()

void wxAnimationCtrl::Init()
{
    m_currentFrame = 0;
    m_looped = false;
    m_isPlaying = false;

    // use the window background colour by default to be consistent
    // with the GTK+ native version
    m_useWinBackgroundColour = true;
}

bool wxAnimationCtrl::Create(wxWindow *parent, wxWindowID id,
            const wxAnimation& animation, const wxPoint& pos,
            const wxSize& size, long style, const wxString& name)
{
    m_timer.SetOwner(this);

    if (!base_type::Create(parent, id, pos, size, style, wxDefaultValidator, name))
        return false;

    // by default we get the same background colour of our parent
    SetBackgroundColour(parent->GetBackgroundColour());

    SetAnimation(animation);

    return true;
}

wxAnimationCtrl::~wxAnimationCtrl()
{
    Stop();
}

bool wxAnimationCtrl::LoadFile(const wxString& filename, wxAnimationType type)
{
    wxAnimation anim;
    if (!anim.LoadFile(filename, type) ||
        !anim.IsOk())
        return false;

    SetAnimation(anim);
    return true;
}

wxSize wxAnimationCtrl::DoGetBestSize() const
{
    if (m_animation.IsOk() && !this->HasFlag(wxAC_NO_AUTORESIZE))
        return m_animation.GetSize();

    return wxSize(100, 100);
}

void wxAnimationCtrl::SetAnimation(const wxAnimation& animation)
{
    if (IsPlaying())
        Stop();

    // set new animation even if it's wxNullAnimation
    m_animation = animation;
    if (!m_animation.IsOk())
    {
        DisplayStaticImage();
        return;
    }

    if (m_animation.GetBackgroundColour() == wxNullColour)
        SetUseWindowBackgroundColour();
    if (!this->HasFlag(wxAC_NO_AUTORESIZE))
        FitToAnimation();

    DisplayStaticImage();
}

void wxAnimationCtrl::SetInactiveBitmap(const wxBitmap &bmp)
{
    // if the bitmap has an associated mask, we need to set our background to
    // the colour of our parent otherwise when calling DrawCurrentFrame()
    // (which uses the bitmap's mask), our background colour would be used for
    // transparent areas - and that's not what we want (at least for
    // consistency with the GTK version)
    if ( bmp.GetMask() != NULL && GetParent() != NULL )
        SetBackgroundColour(GetParent()->GetBackgroundColour());

    wxAnimationCtrlBase::SetInactiveBitmap(bmp);
}

void wxAnimationCtrl::FitToAnimation()
{
    SetSize(m_animation.GetSize());
}

bool wxAnimationCtrl::SetBackgroundColour(const wxColour& colour)
{
    if ( !wxWindow::SetBackgroundColour(colour) )
        return false;

    // if not playing, then this change must be seen immediately (unless
    // there's an inactive bitmap set which has higher priority than bg colour)
    if ( !IsPlaying() )
        DisplayStaticImage();

    return true;
}


// ----------------------------------------------------------------------------
// wxAnimationCtrl - stop/play methods
// ----------------------------------------------------------------------------

void wxAnimationCtrl::Stop()
{
    m_timer.Stop();
    m_isPlaying = false;

    // reset frame counter
    m_currentFrame = 0;

    DisplayStaticImage();
}

bool wxAnimationCtrl::Play(bool looped)
{
    if (!m_animation.IsOk())
        return false;

    m_looped = looped;
    m_currentFrame = 0;

    if (!RebuildBackingStoreUpToFrame(0))
        return false;

    m_isPlaying = true;

    // do a ClearBackground() to avoid that e.g. the custom static bitmap which
    // was eventually shown previously remains partially drawn
    ClearBackground();

    // DrawCurrentFrame() will use our updated backing store
    wxClientDC clientDC(this);
    DrawCurrentFrame(clientDC);

    // start the timer
    int delay = m_animation.GetDelay(0);
    if (delay == 0)
        delay = 1;      // 0 is invalid timeout for wxTimer.
    m_timer.Start(delay, true);

    return true;
}



// ----------------------------------------------------------------------------
// wxAnimationCtrl - rendering methods
// ----------------------------------------------------------------------------

bool wxAnimationCtrl::RebuildBackingStoreUpToFrame(unsigned int frame)
{
    // if we've not created the backing store yet or it's too
    // small, then recreate it
    wxSize sz = m_animation.GetSize(),
           winsz = GetClientSize();
    int w = wxMin(sz.GetWidth(), winsz.GetWidth());
    int h = wxMin(sz.GetHeight(), winsz.GetHeight());

    if ( !m_backingStore.IsOk() ||
            m_backingStore.GetWidth() < w || m_backingStore.GetHeight() < h )
    {
        if (!m_backingStore.Create(w, h))
            return false;
    }

    wxMemoryDC dc;
    dc.SelectObject(m_backingStore);

    // Draw the background
    DisposeToBackground(dc);

    // Draw all intermediate frames that haven't been removed from the animation
    for (unsigned int i = 0; i < frame; i++)
    {
        if (m_animation.GetDisposalMethod(i) == wxANIM_DONOTREMOVE ||
            m_animation.GetDisposalMethod(i) == wxANIM_UNSPECIFIED)
        {
            DrawFrame(dc, i);
        }
        else if (m_animation.GetDisposalMethod(i) == wxANIM_TOBACKGROUND)
            DisposeToBackground(dc, m_animation.GetFramePosition(i),
                                    m_animation.GetFrameSize(i));
    }

    // finally draw this frame
    DrawFrame(dc, frame);
    dc.SelectObject(wxNullBitmap);

    return true;
}

void wxAnimationCtrl::IncrementalUpdateBackingStore()
{
    wxMemoryDC dc;
    dc.SelectObject(m_backingStore);

    // OPTIMIZATION:
    // since wxAnimationCtrl can only play animations forward, without skipping
    // frames, we can be sure that m_backingStore contains the m_currentFrame-1
    // frame and thus we just need to dispose the m_currentFrame-1 frame and
    // render the m_currentFrame-th one.

    if (m_currentFrame == 0)
    {
        // before drawing the first frame always dispose to bg colour
        DisposeToBackground(dc);
    }
    else
    {
        switch (m_animation.GetDisposalMethod(m_currentFrame-1))
        {
        case wxANIM_TOBACKGROUND:
            DisposeToBackground(dc, m_animation.GetFramePosition(m_currentFrame-1),
                                    m_animation.GetFrameSize(m_currentFrame-1));
            break;

        case wxANIM_TOPREVIOUS:
            // this disposal should never be used too often.
            // E.g. GIF specification explicitely say to keep the usage of this
            //      disposal limited to the minimum.
            // In fact it may require a lot of time to restore
            if (m_currentFrame == 1)
            {
                // if 0-th frame disposal is to restore to previous frame,
                // the best we can do is to restore to background
                DisposeToBackground(dc);
            }
            else
                if (!RebuildBackingStoreUpToFrame(m_currentFrame-2))
                    Stop();
            break;

        case wxANIM_DONOTREMOVE:
        case wxANIM_UNSPECIFIED:
            break;
        }
    }

    // now just draw the current frame on the top of the backing store
    DrawFrame(dc, m_currentFrame);
    dc.SelectObject(wxNullBitmap);
}

void wxAnimationCtrl::DisplayStaticImage()
{
    wxASSERT(!IsPlaying());

    // m_bmpStaticReal will be updated only if necessary...
    UpdateStaticImage();

    if (m_bmpStaticReal.IsOk())
    {
        // copy the inactive bitmap in the backing store
        // eventually using the mask if the static bitmap has one
        if ( m_bmpStaticReal.GetMask() )
        {
            wxMemoryDC temp;
            temp.SelectObject(m_backingStore);
            DisposeToBackground(temp);
            temp.DrawBitmap(m_bmpStaticReal, 0, 0, true /* use mask */);
        }
        else
            m_backingStore = m_bmpStaticReal;
    }
    else
    {
        // put in the backing store the first frame of the animation
        if (!m_animation.IsOk() ||
            !RebuildBackingStoreUpToFrame(0))
        {
            m_animation = wxNullAnimation;
            DisposeToBackground();
        }
    }

    Refresh();
}

void wxAnimationCtrl::DrawFrame(wxDC &dc, unsigned int frame)
{
    // PERFORMANCE NOTE:
    // this draw stuff is not as fast as possible: the wxAnimationDecoder
    // needs first to convert from its internal format to wxImage RGB24;
    // the wxImage is then converted as a wxBitmap and finally blitted.
    // If wxAnimationDecoder had a function to convert directly from its
    // internal format to a port-specific wxBitmap, it would be somewhat faster.
    wxBitmap bmp(m_animation.GetFrame(frame));
    dc.DrawBitmap(bmp, m_animation.GetFramePosition(frame),
                  true /* use mask */);
}

void wxAnimationCtrl::DrawCurrentFrame(wxDC& dc)
{
    wxASSERT( m_backingStore.IsOk() );

    // m_backingStore always contains the current frame
    dc.DrawBitmap(m_backingStore, 0, 0, true /* use mask in case it's present */);
}

void wxAnimationCtrl::DisposeToBackground()
{
    // clear the backing store
    wxMemoryDC dc;
    dc.SelectObject(m_backingStore);
    if ( dc.IsOk() )
        DisposeToBackground(dc);
}

void wxAnimationCtrl::DisposeToBackground(wxDC& dc)
{ 
    wxColour col = IsUsingWindowBackgroundColour()
                    ? GetBackgroundColour()
                    : m_animation.GetBackgroundColour();

    wxBrush brush(col);
    dc.SetBackground(brush);
    dc.Clear();
}

void wxAnimationCtrl::DisposeToBackground(wxDC& dc, const wxPoint &pos, const wxSize &sz)
{
    wxColour col = IsUsingWindowBackgroundColour()
                    ? GetBackgroundColour()
                    : m_animation.GetBackgroundColour();
    wxBrush brush(col);
    dc.SetBrush(brush);         // SetBrush and not SetBackground !!
    dc.SetPen(*wxTRANSPARENT_PEN);
    dc.DrawRectangle(pos, sz);
}

// ----------------------------------------------------------------------------
// wxAnimationCtrl - event handlers
// ----------------------------------------------------------------------------

void wxAnimationCtrl::OnPaint(wxPaintEvent& WXUNUSED(event))
{
    // VERY IMPORTANT: the wxPaintDC *must* be created in any case
    wxPaintDC dc(this);

    if ( m_backingStore.IsOk() )
    {
        // NOTE: we draw the bitmap explicitely ignoring the mask (if any);
        //       i.e. we don't want to combine the backing store with the 
        //       possibly wrong preexisting contents of the window!
        dc.DrawBitmap(m_backingStore, 0, 0, false /* no mask */);
    }
    else
    {
        // m_animation is not valid and thus we don't have a valid backing store...
        // clear then our area to the background colour
        DisposeToBackground(dc);
    }
}

void wxAnimationCtrl::OnTimer(wxTimerEvent &WXUNUSED(event))
{
    m_currentFrame++;
    if (m_currentFrame == m_animation.GetFrameCount())
    {
        // Should a non-looped animation display the last frame?
        if (!m_looped)
        {
            Stop();
            return;
        }
        else
            m_currentFrame = 0;     // let's restart
    }

    IncrementalUpdateBackingStore();

    wxClientDC dc(this);
    DrawCurrentFrame(dc);

#ifdef __WXMAC__
    // without this, the animation currently doesn't redraw under Mac
    Refresh();
#endif // __WXMAC__

    // Set the timer for the next frame
    int delay = m_animation.GetDelay(m_currentFrame);
    if (delay == 0)
        delay = 1;      // 0 is invalid timeout for wxTimer.
    m_timer.Start(delay, true);
}

void wxAnimationCtrl::OnSize(wxSizeEvent &WXUNUSED(event))
{
    // NB: resizing an animation control may take a lot of time
    //     for big animations as the backing store must be
    //     extended and rebuilt. Try to avoid it e.g. using
    //     a null proportion value for your wxAnimationCtrls
    //     when using them inside sizers.
    if (m_animation.IsOk())
    {
        // be careful to change the backing store *only* if we are 
        // playing the animation as otherwise we may be displaying 
        // the inactive bitmap and overwriting the backing store 
        // with the last played frame is wrong in this case
        if (IsPlaying())
        {
            if (!RebuildBackingStoreUpToFrame(m_currentFrame))
                Stop();     // in case we are playing
        }
    }
}

#endif      // wxUSE_ANIMATIONCTRL

