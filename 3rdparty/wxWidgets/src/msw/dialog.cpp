/////////////////////////////////////////////////////////////////////////////
// Name:        src/msw/dialog.cpp
// Purpose:     wxDialog class
// Author:      Julian Smart
// Modified by:
// Created:     01/02/97
// RCS-ID:      $Id: dialog.cpp 41054 2006-09-07 19:01:45Z ABX $
// Copyright:   (c) Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#include "wx/dialog.h"

#ifndef WX_PRECOMP
    #include "wx/msw/wrapcdlg.h"
    #include "wx/utils.h"
    #include "wx/frame.h"
    #include "wx/app.h"
    #include "wx/button.h"
    #include "wx/settings.h"
    #include "wx/intl.h"
    #include "wx/log.h"
    #include "wx/toolbar.h"
#endif

#include "wx/msw/private.h"
#include "wx/evtloop.h"
#include "wx/ptr_scpd.h"

#if defined(__SMARTPHONE__) && defined(__WXWINCE__)
    #include "wx/msw/wince/resources.h"
#endif // __SMARTPHONE__ && __WXWINCE__

// ----------------------------------------------------------------------------
// wxWin macros
// ----------------------------------------------------------------------------

#if wxUSE_EXTENDED_RTTI
WX_DEFINE_FLAGS( wxDialogStyle )

wxBEGIN_FLAGS( wxDialogStyle )
    // new style border flags, we put them first to
    // use them for streaming out
    wxFLAGS_MEMBER(wxBORDER_SIMPLE)
    wxFLAGS_MEMBER(wxBORDER_SUNKEN)
    wxFLAGS_MEMBER(wxBORDER_DOUBLE)
    wxFLAGS_MEMBER(wxBORDER_RAISED)
    wxFLAGS_MEMBER(wxBORDER_STATIC)
    wxFLAGS_MEMBER(wxBORDER_NONE)

    // old style border flags
    wxFLAGS_MEMBER(wxSIMPLE_BORDER)
    wxFLAGS_MEMBER(wxSUNKEN_BORDER)
    wxFLAGS_MEMBER(wxDOUBLE_BORDER)
    wxFLAGS_MEMBER(wxRAISED_BORDER)
    wxFLAGS_MEMBER(wxSTATIC_BORDER)
    wxFLAGS_MEMBER(wxNO_BORDER)

    // standard window styles
    wxFLAGS_MEMBER(wxTAB_TRAVERSAL)
    wxFLAGS_MEMBER(wxCLIP_CHILDREN)

    // dialog styles
    wxFLAGS_MEMBER(wxWS_EX_VALIDATE_RECURSIVELY)
    wxFLAGS_MEMBER(wxSTAY_ON_TOP)
    wxFLAGS_MEMBER(wxCAPTION)
#if WXWIN_COMPATIBILITY_2_6
    wxFLAGS_MEMBER(wxTHICK_FRAME)
#endif // WXWIN_COMPATIBILITY_2_6
    wxFLAGS_MEMBER(wxSYSTEM_MENU)
    wxFLAGS_MEMBER(wxRESIZE_BORDER)
#if WXWIN_COMPATIBILITY_2_6
    wxFLAGS_MEMBER(wxRESIZE_BOX)
#endif // WXWIN_COMPATIBILITY_2_6
    wxFLAGS_MEMBER(wxCLOSE_BOX)
    wxFLAGS_MEMBER(wxMAXIMIZE_BOX)
    wxFLAGS_MEMBER(wxMINIMIZE_BOX)
wxEND_FLAGS( wxDialogStyle )

IMPLEMENT_DYNAMIC_CLASS_XTI(wxDialog, wxTopLevelWindow,"wx/dialog.h")

wxBEGIN_PROPERTIES_TABLE(wxDialog)
    wxPROPERTY( Title, wxString, SetTitle, GetTitle, wxString() , 0 /*flags*/ , wxT("Helpstring") , wxT("group"))
    wxPROPERTY_FLAGS( WindowStyle , wxDialogStyle , long , SetWindowStyleFlag , GetWindowStyleFlag , EMPTY_MACROVALUE , 0 /*flags*/ , wxT("Helpstring") , wxT("group")) // style
wxEND_PROPERTIES_TABLE()

wxBEGIN_HANDLERS_TABLE(wxDialog)
wxEND_HANDLERS_TABLE()

wxCONSTRUCTOR_6( wxDialog , wxWindow* , Parent , wxWindowID , Id , wxString , Title , wxPoint , Position , wxSize , Size , long , WindowStyle)

#else
IMPLEMENT_DYNAMIC_CLASS(wxDialog, wxTopLevelWindow)
#endif

// ----------------------------------------------------------------------------
// wxDialogModalData
// ----------------------------------------------------------------------------

// this is simply a container for any data we need to implement modality which
// allows us to avoid changing wxDialog each time the implementation changes
class wxDialogModalData
{
public:
    wxDialogModalData(wxDialog *dialog) : m_evtLoop(dialog) { }

    void RunLoop()
    {
        m_evtLoop.Run();
    }

    void ExitLoop()
    {
        m_evtLoop.Exit();
    }

private:
    wxModalEventLoop m_evtLoop;
};

wxDEFINE_TIED_SCOPED_PTR_TYPE(wxDialogModalData)

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// wxDialog construction
// ----------------------------------------------------------------------------

void wxDialog::Init()
{
    m_oldFocus = (wxWindow *)NULL;
    m_isShown = false;
    m_modalData = NULL;
    m_endModalCalled = false;
#if wxUSE_TOOLBAR && defined(__POCKETPC__)
    m_dialogToolBar = NULL;
#endif
}

bool wxDialog::Create(wxWindow *parent,
                      wxWindowID id,
                      const wxString& title,
                      const wxPoint& pos,
                      const wxSize& size,
                      long style,
                      const wxString& name)
{
    SetExtraStyle(GetExtraStyle() | wxTOPLEVEL_EX_DIALOG);

    // save focus before doing anything which can potentially change it
    m_oldFocus = FindFocus();

    // All dialogs should really have this style
    style |= wxTAB_TRAVERSAL;

    if ( !wxTopLevelWindow::Create(parent, id, title, pos, size, style, name) )
        return false;

    if ( !m_hasFont )
        SetFont(wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT));

#if defined(__SMARTPHONE__) && defined(__WXWINCE__)
    SetLeftMenu(wxID_OK, _("OK"));
#endif
#if wxUSE_TOOLBAR && defined(__POCKETPC__)
    CreateToolBar();
#endif

    return true;
}

#if WXWIN_COMPATIBILITY_2_6

// deprecated ctor
wxDialog::wxDialog(wxWindow *parent,
                   const wxString& title,
                   bool WXUNUSED(modal),
                   int x,
                   int y,
                   int w,
                   int h,
                   long style,
                   const wxString& name)
{
    Init();

    Create(parent, wxID_ANY, title, wxPoint(x, y), wxSize(w, h), style, name);
}

void wxDialog::SetModal(bool WXUNUSED(flag))
{
    // nothing to do, obsolete method
}

#endif // WXWIN_COMPATIBILITY_2_6

wxDialog::~wxDialog()
{
    m_isBeingDeleted = true;

    // this will also reenable all the other windows for a modal dialog
    Show(false);
}

// ----------------------------------------------------------------------------
// showing the dialogs
// ----------------------------------------------------------------------------

#if WXWIN_COMPATIBILITY_2_6

bool wxDialog::IsModalShowing() const
{
    return IsModal();
}

#endif // WXWIN_COMPATIBILITY_2_6

wxWindow *wxDialog::FindSuitableParent() const
{
    // first try to use the currently active window
    HWND hwndFg = ::GetForegroundWindow();
    wxWindow *parent = hwndFg ? wxFindWinFromHandle((WXHWND)hwndFg)
                              : NULL;
    if ( !parent )
    {
        // next try the main app window
        parent = wxTheApp->GetTopWindow();
    }

    // finally, check if the parent we found is really suitable
    if ( !parent || parent == (wxWindow *)this || !parent->IsShown() )
    {
        // don't use this one
        parent = NULL;
    }

    return parent;
}

bool wxDialog::Show(bool show)
{
    if ( show == IsShown() )
        return false;

    if ( !show && m_modalData )
    {
        // we need to do this before calling wxDialogBase version because if we
        // had disabled other app windows, they must be reenabled right now as
        // if they stay disabled Windows will activate another window (one
        // which is enabled, anyhow) when we're hidden in the base class Show()
        // and we will lose activation
        m_modalData->ExitLoop();
    }

    if ( show )
    {
        // this usually will result in TransferDataToWindow() being called
        // which will change the controls values so do it before showing as
        // otherwise we could have some flicker
        InitDialog();
    }

    wxDialogBase::Show(show);

    if ( show )
    {
        // dialogs don't get WM_SIZE message after creation unlike most (all?)
        // other windows and so could start their life non laid out correctly
        // if we didn't call Layout() from here
        //
        // NB: normally we should call it just the first time but doing it
        //     every time is simpler than keeping a flag
        Layout();
    }

    return true;
}

void wxDialog::Raise()
{
    ::SetForegroundWindow(GetHwnd());
}

// show dialog modally
int wxDialog::ShowModal()
{
    wxASSERT_MSG( !IsModal(), _T("wxDialog::ShowModal() reentered?") );

    m_endModalCalled = false;

    Show();

    // EndModal may have been called from InitDialog handler (called from
    // inside Show()), which would cause an infinite loop if we didn't take it
    // into account
    if ( !m_endModalCalled )
    {
        // modal dialog needs a parent window, so try to find one
        wxWindow *parent = GetParent();
        if ( !parent )
        {
            parent = FindSuitableParent();
        }

        // remember where the focus was
        wxWindow *oldFocus = m_oldFocus;
        if ( !oldFocus )
        {
            // VZ: do we really want to do this?
            oldFocus = parent;
        }

        // We have to remember the HWND because we need to check
        // the HWND still exists (oldFocus can be garbage when the dialog
        // exits, if it has been destroyed)
        HWND hwndOldFocus = oldFocus ? GetHwndOf(oldFocus) : NULL;


        // enter and run the modal loop
        {
            wxDialogModalDataTiedPtr modalData(&m_modalData,
                                               new wxDialogModalData(this));
            modalData->RunLoop();
        }


        // and restore focus
        // Note that this code MUST NOT access the dialog object's data
        // in case the object has been deleted (which will be the case
        // for a modal dialog that has been destroyed before calling EndModal).
        if ( oldFocus && (oldFocus != this) && ::IsWindow(hwndOldFocus))
        {
            // This is likely to prove that the object still exists
            if (wxFindWinFromHandle((WXHWND) hwndOldFocus) == oldFocus)
                oldFocus->SetFocus();
        }
    }

    return GetReturnCode();
}

void wxDialog::EndModal(int retCode)
{
    wxASSERT_MSG( IsModal(), _T("EndModal() called for non modal dialog") );

    m_endModalCalled = true;
    SetReturnCode(retCode);

    Hide();
}

// ----------------------------------------------------------------------------
// wxWin event handlers
// ----------------------------------------------------------------------------

#ifdef __POCKETPC__
// Responds to the OK button in a PocketPC titlebar. This
// can be overridden, or you can change the id used for
// sending the event, by calling SetAffirmativeId.
bool wxDialog::DoOK()
{
    const int idOk = GetAffirmativeId();
    if ( EmulateButtonClickIfPresent(idOk) )
        return true;

    wxCommandEvent event(wxEVT_COMMAND_BUTTON_CLICKED, GetAffirmativeId());
    event.SetEventObject(this);

    return GetEventHandler()->ProcessEvent(event);
}
#endif // __POCKETPC__

#if wxUSE_TOOLBAR && defined(__POCKETPC__)
// create main toolbar by calling OnCreateToolBar()
wxToolBar* wxDialog::CreateToolBar(long style, wxWindowID winid, const wxString& name)
{
    m_dialogToolBar = OnCreateToolBar(style, winid, name);

    return m_dialogToolBar;
}

// return a new toolbar
wxToolBar *wxDialog::OnCreateToolBar(long style,
                                       wxWindowID winid,
                                       const wxString& name)
{
    return new wxToolMenuBar(this, winid,
                         wxDefaultPosition, wxDefaultSize,
                         style, name);
}
#endif

// ---------------------------------------------------------------------------
// dialog Windows messages processing
// ---------------------------------------------------------------------------

WXLRESULT wxDialog::MSWWindowProc(WXUINT message, WXWPARAM wParam, WXLPARAM lParam)
{
    WXLRESULT rc = 0;
    bool processed = false;

    switch ( message )
    {
#ifdef __WXWINCE__
        // react to pressing the OK button in the title
        case WM_COMMAND:
        {
            switch ( LOWORD(wParam) )
            {
#ifdef __POCKETPC__
                case IDOK:
                    processed = DoOK();
                    if (!processed)
                        processed = !Close();
#endif
#ifdef __SMARTPHONE__
                case IDM_LEFT:
                case IDM_RIGHT:
                    processed = HandleCommand( LOWORD(wParam) , 0 , NULL );
                    break;
#endif // __SMARTPHONE__
            }
            break;
        }
#endif
        case WM_CLOSE:
            // if we can't close, tell the system that we processed the
            // message - otherwise it would close us
            processed = !Close();
            break;

        case WM_SIZE:
            // the Windows dialogs unfortunately are not meant to be resizeable
            // at all and their standard class doesn't include CS_[VH]REDRAW
            // styles which means that the window is not refreshed properly
            // after the resize and no amount of WS_CLIPCHILDREN/SIBLINGS can
            // help with it - so we have to refresh it manually which certainly
            // creates flicker but at least doesn't show garbage on the screen
            rc = wxWindow::MSWWindowProc(message, wParam, lParam);
            processed = true;
            if ( HasFlag(wxFULL_REPAINT_ON_RESIZE) )
            {
                ::InvalidateRect(GetHwnd(), NULL, false /* erase bg */);
            }
            break;

#ifndef __WXMICROWIN__
        case WM_SETCURSOR:
            // we want to override the busy cursor for modal dialogs:
            // typically, wxBeginBusyCursor() is called and then a modal dialog
            // is shown, but the modal dialog shouldn't have hourglass cursor
            if ( IsModal() && wxIsBusy() )
            {
                // set our cursor for all windows (but see below)
                wxCursor cursor = m_cursor;
                if ( !cursor.Ok() )
                    cursor = wxCURSOR_ARROW;

                ::SetCursor(GetHcursorOf(cursor));

                // in any case, stop here and don't let wxWindow process this
                // message (it would set the busy cursor)
                processed = true;

                // but return false to tell the child window (if the event
                // comes from one of them and not from ourselves) that it can
                // set its own cursor if it has one: thus, standard controls
                // (e.g. text ctrl) still have correct cursors in a dialog
                // invoked while wxIsBusy()
                rc = false;
            }
            break;
#endif // __WXMICROWIN__
    }

    if ( !processed )
        rc = wxWindow::MSWWindowProc(message, wParam, lParam);

    return rc;
}
