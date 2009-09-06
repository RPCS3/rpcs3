///////////////////////////////////////////////////////////////////////////////
// Name:        msw/caret.cpp
// Purpose:     MSW implementation of wxCaret
// Author:      Vadim Zeitlin
// Modified by:
// Created:     23.05.99
// RCS-ID:      $Id: caret.cpp 35650 2005-09-23 12:56:45Z MR $
// Copyright:   (c) wxWidgets team
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

// ===========================================================================
// declarations
// ===========================================================================

// ---------------------------------------------------------------------------
// headers
// ---------------------------------------------------------------------------

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#ifndef WX_PRECOMP
    #include "wx/window.h"
    #include "wx/log.h"
#endif // WX_PRECOMP

#include "wx/caret.h"

#if wxUSE_CARET

#include "wx/msw/private.h"

// ---------------------------------------------------------------------------
// macros
// ---------------------------------------------------------------------------

#define CALL_CARET_API(api, args)   \
        if ( !api args )                \
            wxLogLastError(_T(#api))

// ===========================================================================
// implementation
// ===========================================================================

// ---------------------------------------------------------------------------
// blink time
// ---------------------------------------------------------------------------

//static
int wxCaretBase::GetBlinkTime()
{
    int blinkTime = ::GetCaretBlinkTime();
    if ( !blinkTime )
    {
        wxLogLastError(wxT("GetCaretBlinkTime"));
    }

    return blinkTime;
}

//static
void wxCaretBase::SetBlinkTime(int milliseconds)
{
    CALL_CARET_API(SetCaretBlinkTime, (milliseconds));
}

// ---------------------------------------------------------------------------
// creating/destroying the caret
// ---------------------------------------------------------------------------

bool wxCaret::MSWCreateCaret()
{
    wxASSERT_MSG( GetWindow(), wxT("caret without window cannot be created") );
    wxASSERT_MSG( IsOk(),  wxT("caret of zero size cannot be created") );

    if ( !m_hasCaret )
    {
        CALL_CARET_API(CreateCaret, (GetWinHwnd(GetWindow()), 0,
                                     m_width, m_height));

        m_hasCaret = true;
    }

    return m_hasCaret;
}

void wxCaret::OnSetFocus()
{
    if ( m_countVisible > 0 )
    {
        if ( MSWCreateCaret() )
        {
            // the caret was recreated but it doesn't remember its position and
            // it's not shown

            DoMove();
            DoShow();
        }
    }
    //else: caret is invisible, don't waste time creating it
}

void wxCaret::OnKillFocus()
{
    if ( m_hasCaret )
    {
        m_hasCaret = false;

        CALL_CARET_API(DestroyCaret, ());
    }
}

// ---------------------------------------------------------------------------
// showing/hiding the caret
// ---------------------------------------------------------------------------

void wxCaret::DoShow()
{
    wxASSERT_MSG( GetWindow(), wxT("caret without window cannot be shown") );
    wxASSERT_MSG( IsOk(), wxT("caret of zero size cannot be shown") );

    // we might not have created the caret yet if we had got the focus first
    // and the caret was shown later - so do it now if we have the focus but
    // not the caret
    if ( !m_hasCaret && (wxWindow::FindFocus() == GetWindow()) )
    {
        if ( MSWCreateCaret() )
        {
            DoMove();
        }
    }

    if ( m_hasCaret )
    {
        CALL_CARET_API(ShowCaret, (GetWinHwnd(GetWindow())));
    }
    //else: will be shown when we get the focus
}

void wxCaret::DoHide()
{
    if ( m_hasCaret )
    {
        CALL_CARET_API(HideCaret, (GetWinHwnd(GetWindow())));
    }
}

// ---------------------------------------------------------------------------
// moving the caret
// ---------------------------------------------------------------------------

void wxCaret::DoMove()
{
    if ( m_hasCaret )
    {
        wxASSERT_MSG( wxWindow::FindFocus() == GetWindow(),
                      wxT("how did we lose focus?") );

        // for compatibility with the generic version, the coordinates are
        // client ones
        wxPoint pt = GetWindow()->GetClientAreaOrigin();
        CALL_CARET_API(SetCaretPos, (m_x + pt.x, m_y + pt.y));
    }
    //else: we don't have caret right now, nothing to do (this does happen)
}


// ---------------------------------------------------------------------------
// resizing the caret
// ---------------------------------------------------------------------------

void wxCaret::DoSize()
{
    if ( m_hasCaret )
    {
        m_hasCaret = false;
        CALL_CARET_API(DestroyCaret, ());
        MSWCreateCaret();
        OnSetFocus();
    }
}

#endif
