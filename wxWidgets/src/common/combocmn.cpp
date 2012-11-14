/////////////////////////////////////////////////////////////////////////////
// Name:        src/common/combocmn.cpp
// Purpose:     wxComboCtrlBase
// Author:      Jaakko Salli
// Modified by:
// Created:     Apr-30-2006
// RCS-ID:      $Id: combocmn.cpp 67178 2011-03-13 09:32:19Z JMS $
// Copyright:   (c) 2005 Jaakko Salli
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#if wxUSE_COMBOCTRL

#include "wx/combobox.h"

#ifndef WX_PRECOMP
    #include "wx/app.h"
    #include "wx/log.h"
    #include "wx/dcclient.h"
    #include "wx/settings.h"
    #include "wx/dialog.h"
    #include "wx/timer.h"
#endif

#include "wx/tooltip.h"

#include "wx/combo.h"



// constants
// ----------------------------------------------------------------------------

#define DEFAULT_DROPBUTTON_WIDTH                19

#define BMP_BUTTON_MARGIN                       4

#define DEFAULT_POPUP_HEIGHT                    400

#define DEFAULT_TEXT_INDENT                     3

#define COMBO_MARGIN                            2 // spacing right of wxTextCtrl


#if defined(__WXMSW__)

#define USE_TRANSIENT_POPUP           1 // Use wxPopupWindowTransient (preferred, if it works properly on platform)
#define TRANSIENT_POPUPWIN_IS_PERFECT 0 // wxPopupTransientWindow works, its child can have focus, and common
                                        // native controls work on it like normal.
#define POPUPWIN_IS_PERFECT           0 // Same, but for non-transient popup window.
#define TEXTCTRL_TEXT_CENTERED        0 // 1 if text in textctrl is vertically centered
#define FOCUS_RING                    0 // No focus ring on wxMSW

//#undef wxUSE_POPUPWIN
//#define wxUSE_POPUPWIN 0

#elif defined(__WXGTK__)

// NB: It is not recommended to use wxDialog as popup on wxGTK, because of
//     this bug: If wxDialog is hidden, its position becomes corrupt
//     between hide and next show, but without internal coordinates being
//     reflected (or something like that - atleast commenting out ->Hide()
//     seemed to eliminate the position change).

#define USE_TRANSIENT_POPUP           1 // Use wxPopupWindowTransient (preferred, if it works properly on platform)
#define TRANSIENT_POPUPWIN_IS_PERFECT 0 // wxPopupTransientWindow works, its child can have focus, and common
                                        // native controls work on it like normal.
#define POPUPWIN_IS_PERFECT           1 // Same, but for non-transient popup window.
#define TEXTCTRL_TEXT_CENTERED        1 // 1 if text in textctrl is vertically centered
#define FOCUS_RING                    0 // No focus ring on wxGTK

#elif defined(__WXMAC__)

#define USE_TRANSIENT_POPUP           0 // Use wxPopupWindowTransient (preferred, if it works properly on platform)
#define TRANSIENT_POPUPWIN_IS_PERFECT 0 // wxPopupTransientWindow works, its child can have focus, and common
                                        // native controls work on it like normal.
#define POPUPWIN_IS_PERFECT           0 // Same, but for non-transient popup window.
#define TEXTCTRL_TEXT_CENTERED        1 // 1 if text in textctrl is vertically centered
#define FOCUS_RING                    3 // Reserve room for the textctrl's focus ring to display

#undef DEFAULT_DROPBUTTON_WIDTH
#define DEFAULT_DROPBUTTON_WIDTH      22
#undef COMBO_MARGIN
#define COMBO_MARGIN                  FOCUS_RING

#else

#define USE_TRANSIENT_POPUP           0 // Use wxPopupWindowTransient (preferred, if it works properly on platform)
#define TRANSIENT_POPUPWIN_IS_PERFECT 0 // wxPopupTransientWindow works, its child can have focus, and common
                                        // native controls work on it like normal.
#define POPUPWIN_IS_PERFECT           0 // Same, but for non-transient popup window.
#define TEXTCTRL_TEXT_CENTERED        1 // 1 if text in textctrl is vertically centered
#define FOCUS_RING                    0

#endif


// Popupwin is really only supported on wxMSW (not WINCE) and wxGTK, regardless
// what the wxUSE_POPUPWIN says.
// FIXME: Why isn't wxUSE_POPUPWIN reliable any longer? (it was in wxW2.6.2)
#if (!defined(__WXMSW__) && !defined(__WXGTK__)) || defined(__WXWINCE__)
#undef wxUSE_POPUPWIN
#define wxUSE_POPUPWIN 0
#endif


#if wxUSE_POPUPWIN
    #include "wx/popupwin.h"
#else
    #undef USE_TRANSIENT_POPUP
    #define USE_TRANSIENT_POPUP 0
#endif


// Define different types of popup windows
enum
{
    POPUPWIN_NONE                   = 0,
    POPUPWIN_WXPOPUPTRANSIENTWINDOW = 1,
    POPUPWIN_WXPOPUPWINDOW          = 2,
    POPUPWIN_WXDIALOG               = 3
};


#if USE_TRANSIENT_POPUP
    // wxPopupTransientWindow is implemented

    #define wxComboPopupWindowBase  wxPopupTransientWindow
    #define PRIMARY_POPUP_TYPE      POPUPWIN_WXPOPUPTRANSIENTWINDOW
    #define USES_WXPOPUPTRANSIENTWINDOW 1

    #if TRANSIENT_POPUPWIN_IS_PERFECT
        //
    #elif POPUPWIN_IS_PERFECT
        #define wxComboPopupWindowBase2     wxPopupWindow
        #define SECONDARY_POPUP_TYPE        POPUPWIN_WXPOPUPWINDOW
        #define USES_WXPOPUPWINDOW          1
    #else
        #define wxComboPopupWindowBase2     wxDialog
        #define SECONDARY_POPUP_TYPE        POPUPWIN_WXDIALOG
        #define USES_WXDIALOG               1
    #endif

#elif wxUSE_POPUPWIN
    // wxPopupWindow (but not wxPopupTransientWindow) is properly implemented

    #define wxComboPopupWindowBase      wxPopupWindow
    #define PRIMARY_POPUP_TYPE          POPUPWIN_WXPOPUPWINDOW
    #define USES_WXPOPUPWINDOW          1

    #if !POPUPWIN_IS_PERFECT
        #define wxComboPopupWindowBase2     wxDialog
        #define SECONDARY_POPUP_TYPE        POPUPWIN_WXDIALOG
        #define USES_WXDIALOG               1
    #endif

#else
    // wxPopupWindow is not implemented

    #define wxComboPopupWindowBase      wxDialog
    #define PRIMARY_POPUP_TYPE          POPUPWIN_WXDIALOG
    #define USES_WXDIALOG               1

#endif


#ifndef USES_WXPOPUPTRANSIENTWINDOW
    #define USES_WXPOPUPTRANSIENTWINDOW 0
#endif

#ifndef USES_WXPOPUPWINDOW
    #define USES_WXPOPUPWINDOW          0
#endif

#ifndef USES_WXDIALOG
    #define USES_WXDIALOG               0
#endif


#if USES_WXPOPUPWINDOW
    #define INSTALL_TOPLEV_HANDLER      1
#else
    #define INSTALL_TOPLEV_HANDLER      0
#endif


//
// ** TODO **
// * wxComboPopupWindow for external use (ie. replace old wxUniv wxPopupComboWindow)
//


// ----------------------------------------------------------------------------
// wxComboFrameEventHandler takes care of hiding the popup when events happen
// in its top level parent.
// ----------------------------------------------------------------------------

#if INSTALL_TOPLEV_HANDLER

//
// This will no longer be necessary after wxTransientPopupWindow
// works well on all platforms.
//

class wxComboFrameEventHandler : public wxEvtHandler
{
public:
    wxComboFrameEventHandler( wxComboCtrlBase* pCb );
    virtual ~wxComboFrameEventHandler();

    void OnPopup();

    void OnIdle( wxIdleEvent& event );
    void OnMouseEvent( wxMouseEvent& event );
    void OnActivate( wxActivateEvent& event );
    void OnResize( wxSizeEvent& event );
    void OnMove( wxMoveEvent& event );
    void OnMenuEvent( wxMenuEvent& event );
    void OnClose( wxCloseEvent& event );

protected:
    wxWindow*                       m_focusStart;
    wxComboCtrlBase*     m_combo;

private:
    DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(wxComboFrameEventHandler, wxEvtHandler)
    EVT_IDLE(wxComboFrameEventHandler::OnIdle)
    EVT_LEFT_DOWN(wxComboFrameEventHandler::OnMouseEvent)
    EVT_RIGHT_DOWN(wxComboFrameEventHandler::OnMouseEvent)
    EVT_SIZE(wxComboFrameEventHandler::OnResize)
    EVT_MOVE(wxComboFrameEventHandler::OnMove)
    EVT_MENU_HIGHLIGHT(wxID_ANY,wxComboFrameEventHandler::OnMenuEvent)
    EVT_MENU_OPEN(wxComboFrameEventHandler::OnMenuEvent)
    EVT_ACTIVATE(wxComboFrameEventHandler::OnActivate)
    EVT_CLOSE(wxComboFrameEventHandler::OnClose)
END_EVENT_TABLE()

wxComboFrameEventHandler::wxComboFrameEventHandler( wxComboCtrlBase* combo )
    : wxEvtHandler()
{
    m_combo = combo;
}

wxComboFrameEventHandler::~wxComboFrameEventHandler()
{
}

void wxComboFrameEventHandler::OnPopup()
{
    m_focusStart = ::wxWindow::FindFocus();
}

void wxComboFrameEventHandler::OnIdle( wxIdleEvent& event )
{
    wxWindow* winFocused = ::wxWindow::FindFocus();

    wxWindow* popup = m_combo->GetPopupControl()->GetControl();
    wxWindow* winpopup = m_combo->GetPopupWindow();

    if (
         winFocused != m_focusStart &&
         winFocused != popup &&
         winFocused->GetParent() != popup &&
         winFocused != winpopup &&
         winFocused->GetParent() != winpopup &&
         winFocused != m_combo &&
         winFocused != m_combo->GetButton() // GTK (atleast) requires this
        )
    {
        m_combo->HidePopup();
    }

    event.Skip();
}

void wxComboFrameEventHandler::OnMenuEvent( wxMenuEvent& event )
{
    m_combo->HidePopup();
    event.Skip();
}

void wxComboFrameEventHandler::OnMouseEvent( wxMouseEvent& event )
{
    m_combo->HidePopup();
    event.Skip();
}

void wxComboFrameEventHandler::OnClose( wxCloseEvent& event )
{
    m_combo->HidePopup();
    event.Skip();
}

void wxComboFrameEventHandler::OnActivate( wxActivateEvent& event )
{
    m_combo->HidePopup();
    event.Skip();
}

void wxComboFrameEventHandler::OnResize( wxSizeEvent& event )
{
    m_combo->HidePopup();
    event.Skip();
}

void wxComboFrameEventHandler::OnMove( wxMoveEvent& event )
{
    m_combo->HidePopup();
    event.Skip();
}

#endif // INSTALL_TOPLEV_HANDLER

// ----------------------------------------------------------------------------
// wxComboPopupWindow is, in essence, wxPopupWindow customized for
// wxComboCtrl.
// ----------------------------------------------------------------------------

class wxComboPopupWindow : public wxComboPopupWindowBase
{
public:

    wxComboPopupWindow( wxComboCtrlBase *parent,
                        int style )
    #if USES_WXPOPUPWINDOW || USES_WXPOPUPTRANSIENTWINDOW
                       : wxComboPopupWindowBase(parent,style)
    #else
                       : wxComboPopupWindowBase(parent,
                                                wxID_ANY,
                                                wxEmptyString,
                                                wxPoint(-21,-21),
                                                wxSize(20,20),
                                                style)
    #endif
    {
        m_inShow = 0;
    }

#if USES_WXPOPUPTRANSIENTWINDOW
    virtual bool Show( bool show );
    virtual bool ProcessLeftDown(wxMouseEvent& event);
protected:
    virtual void OnDismiss();
#endif

private:
    wxByte      m_inShow;
};


#if USES_WXPOPUPTRANSIENTWINDOW
bool wxComboPopupWindow::Show( bool show )
{
    // Guard against recursion
    if ( m_inShow )
        return wxComboPopupWindowBase::Show(show);

    m_inShow++;

    wxASSERT( IsKindOf(CLASSINFO(wxPopupTransientWindow)) );

    wxPopupTransientWindow* ptw = (wxPopupTransientWindow*) this;
    wxComboCtrlBase* combo = (wxComboCtrlBase*) GetParent();

    if ( show != ptw->IsShown() )
    {
        if ( show )
            ptw->Popup(combo->GetPopupControl()->GetControl());
        else
            ptw->Dismiss();
    }

    m_inShow--;

    return true;
}

bool wxComboPopupWindow::ProcessLeftDown(wxMouseEvent& event)
{
    return wxPopupTransientWindow::ProcessLeftDown(event);
}

// First thing that happens when a transient popup closes is that this method gets called.
void wxComboPopupWindow::OnDismiss()
{
    wxComboCtrlBase* combo = (wxComboCtrlBase*) GetParent();
    wxASSERT_MSG( combo->IsKindOf(CLASSINFO(wxComboCtrlBase)),
                  wxT("parent might not be wxComboCtrl, but check IMPLEMENT_DYNAMIC_CLASS(2) macro for correctness") );

    combo->OnPopupDismiss();
}
#endif // USES_WXPOPUPTRANSIENTWINDOW


// ----------------------------------------------------------------------------
// wxComboPopupWindowEvtHandler does bulk of the custom event handling
// of a popup window. It is separate so we can have different types
// of popup windows.
// ----------------------------------------------------------------------------

class wxComboPopupWindowEvtHandler : public wxEvtHandler
{
public:

    wxComboPopupWindowEvtHandler( wxComboCtrlBase *parent )
    {
        m_combo = parent;
    }

    void OnSizeEvent( wxSizeEvent& event );
    void OnKeyEvent(wxKeyEvent& event);
#if USES_WXDIALOG
    void OnActivate( wxActivateEvent& event );
#endif

private:
    wxComboCtrlBase*    m_combo;

    DECLARE_EVENT_TABLE()
};


BEGIN_EVENT_TABLE(wxComboPopupWindowEvtHandler, wxEvtHandler)
    EVT_KEY_DOWN(wxComboPopupWindowEvtHandler::OnKeyEvent)
    EVT_KEY_UP(wxComboPopupWindowEvtHandler::OnKeyEvent)
#if USES_WXDIALOG
    EVT_ACTIVATE(wxComboPopupWindowEvtHandler::OnActivate)
#endif
    EVT_SIZE(wxComboPopupWindowEvtHandler::OnSizeEvent)
END_EVENT_TABLE()


void wxComboPopupWindowEvtHandler::OnSizeEvent( wxSizeEvent& WXUNUSED(event) )
{
    // Block the event so that the popup control does not get auto-resized.
}

void wxComboPopupWindowEvtHandler::OnKeyEvent( wxKeyEvent& event )
{
    // Relay keyboard event to the main child controls
    wxWindowList children = m_combo->GetPopupWindow()->GetChildren();
    wxWindowList::iterator node = children.begin();
    wxWindow* child = (wxWindow*)*node;
    child->AddPendingEvent(event);
}

#if USES_WXDIALOG
void wxComboPopupWindowEvtHandler::OnActivate( wxActivateEvent& event )
{
    if ( !event.GetActive() )
    {
        // Tell combo control that we are dismissed.
        m_combo->HidePopup();

        event.Skip();
    }
}
#endif


// ----------------------------------------------------------------------------
// wxComboPopup
//
// ----------------------------------------------------------------------------

wxComboPopup::~wxComboPopup()
{
}

void wxComboPopup::OnPopup()
{
}

void wxComboPopup::OnDismiss()
{
}

wxSize wxComboPopup::GetAdjustedSize( int minWidth,
                                      int prefHeight,
                                      int WXUNUSED(maxHeight) )
{
    return wxSize(minWidth,prefHeight);
}

void wxComboPopup::DefaultPaintComboControl( wxComboCtrlBase* combo,
                                             wxDC& dc, const wxRect& rect )
{
    if ( combo->GetWindowStyle() & wxCB_READONLY ) // ie. no textctrl
    {
        combo->PrepareBackground(dc,rect,0);

        dc.DrawText( combo->GetValue(),
                     rect.x + combo->GetTextIndent(),
                     (rect.height-dc.GetCharHeight())/2 + rect.y );
    }
}

void wxComboPopup::PaintComboControl( wxDC& dc, const wxRect& rect )
{
    DefaultPaintComboControl(m_combo,dc,rect);
}

void wxComboPopup::OnComboKeyEvent( wxKeyEvent& event )
{
    event.Skip();
}

void wxComboPopup::OnComboDoubleClick()
{
}

void wxComboPopup::SetStringValue( const wxString& WXUNUSED(value) )
{
}

bool wxComboPopup::LazyCreate()
{
    return false;
}

void wxComboPopup::Dismiss()
{
    m_combo->HidePopup();
}

// ----------------------------------------------------------------------------
// input handling
// ----------------------------------------------------------------------------

//
// This is pushed to the event handler queue of the child textctrl.
//
class wxComboBoxExtraInputHandler : public wxEvtHandler
{
public:

    wxComboBoxExtraInputHandler( wxComboCtrlBase* combo )
        : wxEvtHandler()
    {
        m_combo = combo;
    }
    virtual ~wxComboBoxExtraInputHandler() { }
    void OnKey(wxKeyEvent& event);
    void OnFocus(wxFocusEvent& event);

protected:
    wxComboCtrlBase*   m_combo;

private:
    DECLARE_EVENT_TABLE()
};


BEGIN_EVENT_TABLE(wxComboBoxExtraInputHandler, wxEvtHandler)
    EVT_KEY_DOWN(wxComboBoxExtraInputHandler::OnKey)
    EVT_KEY_UP(wxComboBoxExtraInputHandler::OnKey)
    EVT_CHAR(wxComboBoxExtraInputHandler::OnKey)
    EVT_SET_FOCUS(wxComboBoxExtraInputHandler::OnFocus)
END_EVENT_TABLE()


void wxComboBoxExtraInputHandler::OnKey(wxKeyEvent& event)
{
    // Let the wxComboCtrl event handler have a go first.
    wxComboCtrlBase* combo = m_combo;

    wxKeyEvent redirectedEvent(event);
    redirectedEvent.SetId(combo->GetId());
    redirectedEvent.SetEventObject(combo);

    if ( !combo->GetEventHandler()->ProcessEvent(redirectedEvent) )
    {
        // Don't let TAB through to the text ctrl - looks ugly
        if ( event.GetKeyCode() != WXK_TAB )
            event.Skip();
    }
}

void wxComboBoxExtraInputHandler::OnFocus(wxFocusEvent& event)
{
    // FIXME: This code does run when control is clicked,
    //        yet on Windows it doesn't select all the text.
    if ( !(m_combo->GetInternalFlags() & wxCC_NO_TEXT_AUTO_SELECT) )
    {
        if ( m_combo->GetTextCtrl() )
            m_combo->GetTextCtrl()->SelectAll();
        else
            m_combo->SetSelection(-1,-1);
    }

    // Send focus indication to parent.
    // NB: This is needed for cases where the textctrl gets focus
    //     instead of its parent. While this may trigger multiple
    //     wxEVT_SET_FOCUSes (since m_text->SetFocus is called
    //     from combo's focus event handler), they should be quite
    //     harmless.
    wxFocusEvent evt2(wxEVT_SET_FOCUS,m_combo->GetId());
    evt2.SetEventObject(m_combo);
    m_combo->GetEventHandler()->ProcessEvent(evt2);

    event.Skip();
}


//
// This is pushed to the event handler queue of the control in popup.
//

class wxComboPopupExtraEventHandler : public wxEvtHandler
{
public:

    wxComboPopupExtraEventHandler( wxComboCtrlBase* combo )
        : wxEvtHandler()
    {
        m_combo = combo;
        m_beenInside = false;
    }
    virtual ~wxComboPopupExtraEventHandler() { }

    void OnMouseEvent( wxMouseEvent& event );

    // Called from wxComboCtrlBase::OnPopupDismiss
    void OnPopupDismiss()
    {
        m_beenInside = false;
    }

protected:
    wxComboCtrlBase*     m_combo;

    bool                    m_beenInside;

private:
    DECLARE_EVENT_TABLE()
};


BEGIN_EVENT_TABLE(wxComboPopupExtraEventHandler, wxEvtHandler)
    EVT_MOUSE_EVENTS(wxComboPopupExtraEventHandler::OnMouseEvent)
END_EVENT_TABLE()


void wxComboPopupExtraEventHandler::OnMouseEvent( wxMouseEvent& event )
{
    wxPoint pt = event.GetPosition();
    wxSize sz = m_combo->GetPopupControl()->GetControl()->GetClientSize();
    int evtType = event.GetEventType();
    bool isInside = pt.x >= 0 && pt.y >= 0 && pt.x < sz.x && pt.y < sz.y;

    if ( evtType == wxEVT_MOTION ||
         evtType == wxEVT_LEFT_DOWN ||
         evtType == wxEVT_RIGHT_DOWN )
    {
        // Block motion and click events outside the popup
        if ( !isInside || !m_combo->IsPopupShown() )
        {
            event.Skip(false);
            return;
        }
    }
    else if ( evtType == wxEVT_LEFT_UP )
    {
        if ( !m_combo->IsPopupShown() )
        {
            event.Skip(false);
            return;
        }

        if ( !m_beenInside )
        {
            if ( isInside )
            {
                m_beenInside = true;
            }
            else
            {
                //
                // Some mouse events to popup that happen outside it, before cursor
                // has been inside the popu, need to be ignored by it but relayed to
                // the dropbutton.
                //
                wxWindow* btn = m_combo->GetButton();
                if ( btn )
                    btn->GetEventHandler()->AddPendingEvent(event);
                else
                    m_combo->GetEventHandler()->AddPendingEvent(event);

                return;
            }

            event.Skip();
        }
    }

    event.Skip();
}

// ----------------------------------------------------------------------------
// wxComboCtrlBase
// ----------------------------------------------------------------------------


BEGIN_EVENT_TABLE(wxComboCtrlBase, wxControl)
    EVT_TEXT(wxID_ANY,wxComboCtrlBase::OnTextCtrlEvent)
    EVT_SIZE(wxComboCtrlBase::OnSizeEvent)
    EVT_SET_FOCUS(wxComboCtrlBase::OnFocusEvent)
    EVT_KILL_FOCUS(wxComboCtrlBase::OnFocusEvent)
    EVT_IDLE(wxComboCtrlBase::OnIdleEvent)
    //EVT_BUTTON(wxID_ANY,wxComboCtrlBase::OnButtonClickEvent)
    EVT_KEY_DOWN(wxComboCtrlBase::OnKeyEvent)
    EVT_TEXT_ENTER(wxID_ANY,wxComboCtrlBase::OnTextCtrlEvent)
    EVT_SYS_COLOUR_CHANGED(wxComboCtrlBase::OnSysColourChanged)
END_EVENT_TABLE()


IMPLEMENT_ABSTRACT_CLASS(wxComboCtrlBase, wxControl)

void wxComboCtrlBase::Init()
{
    m_winPopup = (wxWindow *)NULL;
    m_popup = (wxWindow *)NULL;
    m_popupWinState = Hidden;
    m_btn = (wxWindow*) NULL;
    m_text = (wxTextCtrl*) NULL;
    m_popupInterface = (wxComboPopup*) NULL;

    m_popupExtraHandler = (wxEvtHandler*) NULL;
    m_textEvtHandler = (wxEvtHandler*) NULL;

#if INSTALL_TOPLEV_HANDLER
    m_toplevEvtHandler = (wxEvtHandler*) NULL;
#endif

    m_mainCtrlWnd = this;

    m_heightPopup = -1;
    m_widthMinPopup = -1;
    m_anchorSide = 0;
    m_widthCustomPaint = 0;
    m_widthCustomBorder = 0;

    m_btnState = 0;
    m_btnWidDefault = 0;
    m_blankButtonBg = false;
    m_ignoreEvtText = 0;
    m_popupWinType = POPUPWIN_NONE;
    m_btnWid = m_btnHei = -1;
    m_btnSide = wxRIGHT;
    m_btnSpacingX = 0;

    m_extLeft = 0;
    m_extRight = 0;
    m_absIndent = -1;
    m_iFlags = 0;
    m_timeCanAcceptClick = 0;

    m_resetFocus = false;
}

bool wxComboCtrlBase::Create(wxWindow *parent,
                             wxWindowID id,
                             const wxString& value,
                             const wxPoint& pos,
                             const wxSize& size,
                             long style,
                             const wxValidator& validator,
                             const wxString& name)
{
    if ( !wxControl::Create(parent,
                            id,
                            pos,
                            size,
                            style | wxWANTS_CHARS,
                            validator,
                            name) )
        return false;

    m_valueString = value;

    // Get colours
    OnThemeChange();
    m_absIndent = GetNativeTextIndent();

    m_iFlags |= wxCC_IFLAG_CREATED;

    // If x and y indicate valid size, wxSizeEvent won't be
    // emitted automatically, so we need to add artifical one.
    if ( size.x > 0 && size.y > 0 )
    {
        wxSizeEvent evt(size,GetId());
        GetEventHandler()->AddPendingEvent(evt);
    }

    return true;
}

void wxComboCtrlBase::InstallInputHandlers()
{
    if ( m_text )
    {
        m_textEvtHandler = new wxComboBoxExtraInputHandler(this);
        m_text->PushEventHandler(m_textEvtHandler);
    }
}

void
wxComboCtrlBase::CreateTextCtrl(int style, const wxValidator& validator)
{
    if ( !(m_windowStyle & wxCB_READONLY) )
    {
        if ( m_text )
            m_text->Destroy();

        // wxTE_PROCESS_TAB is needed because on Windows, wxTAB_TRAVERSAL is
        // not used by the wxPropertyGrid and therefore the tab is processed by
        // looking at ancestors to see if they have wxTAB_TRAVERSAL. The
        // navigation event is then sent to the wrong window.
        style |= wxTE_PROCESS_TAB;

        if ( HasFlag(wxTE_PROCESS_ENTER) )
            style |= wxTE_PROCESS_ENTER;

        // Ignore EVT_TEXT generated by the constructor (but only
        // if the event redirector already exists)
        // NB: This must be " = 1" instead of "++";
        if ( m_textEvtHandler )
            m_ignoreEvtText = 1;
        else
            m_ignoreEvtText = 0;

        m_text = new wxTextCtrl(this, wxID_ANY, m_valueString,
                                wxDefaultPosition, wxSize(10,-1),
                                style, validator);
    }
}

void wxComboCtrlBase::OnThemeChange()
{
    // Leave the default bg on the Mac so the area used by the focus ring will
    // be the correct colour and themed brush.  Instead we'll use
    // wxSYS_COLOUR_WINDOW in the EVT_PAINT handler as needed.
#ifndef __WXMAC__
    if ( !m_hasBgCol )
    {
#ifdef __WXGTK__
        // Set background to gtk_rc_get_style(m_widget)->bg[GTK_STATE_NORMAL],
        // which can be different than the background of text entry.
        wxColour bgCol = GetDefaultAttributes().colBg;
#else
        wxColour bgCol = wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW);
#endif
        SetOwnBackgroundColour(bgCol);
        m_hasBgCol = false;
    }
#endif
}

wxComboCtrlBase::~wxComboCtrlBase()
{
    if ( HasCapture() )
        ReleaseMouse();

#if INSTALL_TOPLEV_HANDLER
    delete ((wxComboFrameEventHandler*)m_toplevEvtHandler);
    m_toplevEvtHandler = (wxEvtHandler*) NULL;
#endif

    DestroyPopup();

    if ( m_text )
        m_text->RemoveEventHandler(m_textEvtHandler);

    delete m_textEvtHandler;
}


// ----------------------------------------------------------------------------
// geometry stuff
// ----------------------------------------------------------------------------

// Recalculates button and textctrl areas
void wxComboCtrlBase::CalculateAreas( int btnWidth )
{
    wxSize sz = GetClientSize();
    int customBorder = m_widthCustomBorder;
    int btnBorder; // border for button only

    // check if button should really be outside the border: we'll do it it if
    // its platform default or bitmap+pushbutton background is used, but not if
    // there is vertical size adjustment or horizontal spacing.
    if ( ( (m_iFlags & wxCC_BUTTON_OUTSIDE_BORDER) ||
                (m_bmpNormal.Ok() && m_blankButtonBg) ) &&
         m_btnSpacingX == 0 &&
         m_btnHei <= 0 )
    {
        m_iFlags |= wxCC_IFLAG_BUTTON_OUTSIDE;
        btnBorder = 0;
    }
    else
    {
        m_iFlags &= ~(wxCC_IFLAG_BUTTON_OUTSIDE);
        btnBorder = customBorder;
    }

    // Defaul indentation
    if ( m_absIndent < 0 )
        m_absIndent = GetNativeTextIndent();

    int butWidth = btnWidth;

    if ( butWidth <= 0 )
        butWidth = m_btnWidDefault;
    else
        m_btnWidDefault = butWidth;

    if ( butWidth <= 0 )
        return;

    int butHeight = sz.y - btnBorder*2;

    // Adjust button width
    if ( m_btnWid > 0 )
        butWidth = m_btnWid;
    else
    {
        // Adjust button width to match aspect ratio
        // (but only if control is smaller than best size).
        int bestHeight = GetBestSize().y;
        int height = GetSize().y;

        if ( height < bestHeight )
        {
            // Make very small buttons square, as it makes
            // them accommodate arrow image better and still
            // looks decent.
            if ( height > 18 )
                butWidth = (height*butWidth)/bestHeight;
            else
                butWidth = butHeight;
        }
    }

    // Adjust button height
    if ( m_btnHei > 0 )
        butHeight = m_btnHei;

    // Use size of normal bitmap if...
    //   It is larger
    //   OR
    //   button width is set to default and blank button bg is not drawn
    if ( m_bmpNormal.Ok() )
    {
        int bmpReqWidth = m_bmpNormal.GetWidth();
        int bmpReqHeight = m_bmpNormal.GetHeight();

        // If drawing blank button background, we need to add some margin.
        if ( m_blankButtonBg )
        {
            bmpReqWidth += BMP_BUTTON_MARGIN*2;
            bmpReqHeight += BMP_BUTTON_MARGIN*2;
        }

        if ( butWidth < bmpReqWidth || ( m_btnWid == 0 && !m_blankButtonBg ) )
            butWidth = bmpReqWidth;
        if ( butHeight < bmpReqHeight || ( m_btnHei == 0 && !m_blankButtonBg ) )
            butHeight = bmpReqHeight;

        // Need to fix height?
        if ( (sz.y-(customBorder*2)) < butHeight && btnWidth == 0 )
        {
            int newY = butHeight+(customBorder*2);
            SetClientSize(wxDefaultCoord,newY);
            sz.y = newY;
        }
    }

    int butAreaWid = butWidth + (m_btnSpacingX*2);

    m_btnSize.x = butWidth;
    m_btnSize.y = butHeight;

    m_btnArea.x = ( m_btnSide==wxRIGHT ? sz.x - butAreaWid - btnBorder : btnBorder );
    m_btnArea.y = btnBorder + FOCUS_RING;
    m_btnArea.width = butAreaWid;
    m_btnArea.height = sz.y - ((btnBorder+FOCUS_RING)*2);

    m_tcArea.x = ( m_btnSide==wxRIGHT ? 0 : butAreaWid ) + customBorder + FOCUS_RING;
    m_tcArea.y = customBorder + FOCUS_RING;
    m_tcArea.width = sz.x - butAreaWid - (customBorder*2) - (FOCUS_RING*2);
    m_tcArea.height = sz.y - ((customBorder+FOCUS_RING)*2);

/*
    if ( m_text )
    {
        ::wxMessageBox(wxString::Format(wxT("ButtonArea (%i,%i,%i,%i)\n"),m_btnArea.x,m_btnArea.y,m_btnArea.width,m_btnArea.height) +
                       wxString::Format(wxT("TextCtrlArea (%i,%i,%i,%i)"),m_tcArea.x,m_tcArea.y,m_tcArea.width,m_tcArea.height));
    }
*/
}

void wxComboCtrlBase::PositionTextCtrl( int textCtrlXAdjust, int textCtrlYAdjust )
{
    if ( !m_text )
        return;

#if !TEXTCTRL_TEXT_CENTERED

    wxSize sz = GetClientSize();

    int customBorder = m_widthCustomBorder;
    if ( (m_text->GetWindowStyleFlag() & wxBORDER_MASK) == wxNO_BORDER )
    {
        // Centre textctrl
        int tcSizeY = m_text->GetBestSize().y;
        int diff = sz.y - tcSizeY;
        int y = textCtrlYAdjust + (diff/2);

        if ( y < customBorder )
            y = customBorder;

        m_text->SetSize( m_tcArea.x + m_widthCustomPaint + m_absIndent + textCtrlXAdjust,
                         y,
                         m_tcArea.width - COMBO_MARGIN -
                         (textCtrlXAdjust + m_widthCustomPaint + m_absIndent),
                         -1 );

        // Make sure textctrl doesn't exceed the bottom custom border
        wxSize tsz = m_text->GetSize();
        diff = (y + tsz.y) - (sz.y - customBorder);
        if ( diff >= 0 )
        {
            tsz.y = tsz.y - diff - 1;
            m_text->SetSize(tsz);
        }
    }
    else
#else // TEXTCTRL_TEXT_CENTERED
    wxUnusedVar(textCtrlXAdjust);
    wxUnusedVar(textCtrlYAdjust);
#endif // !TEXTCTRL_TEXT_CENTERED/TEXTCTRL_TEXT_CENTERED
    {
        // If it has border, have textctrl will the entire text field.
        m_text->SetSize( m_tcArea.x + m_widthCustomPaint,
                         m_tcArea.y,
                         m_tcArea.width - m_widthCustomPaint,
                         m_tcArea.height );
    }
}

wxSize wxComboCtrlBase::DoGetBestSize() const
{
    wxSize sizeText(150,0);

    if ( m_text )
        sizeText = m_text->GetBestSize();

    // TODO: Better method to calculate close-to-native control height.

    int fhei;
    if ( m_font.Ok() )
        fhei = (m_font.GetPointSize()*2) + 5;
    else if ( wxNORMAL_FONT->Ok() )
        fhei = (wxNORMAL_FONT->GetPointSize()*2) + 5;
    else
        fhei = sizeText.y + 4;

    // Need to force height to accomodate bitmap?
    int btnSizeY = m_btnSize.y;
    if ( m_bmpNormal.Ok() && fhei < btnSizeY )
        fhei = btnSizeY;

    // Control height doesn't depend on border
/*
    // Add border
    int border = m_windowStyle & wxBORDER_MASK;
    if ( border == wxSIMPLE_BORDER )
        fhei += 2;
    else if ( border == wxNO_BORDER )
        fhei += (m_widthCustomBorder*2);
    else
        // Sunken etc.
        fhei += 4;
*/

    // Final adjustments
#ifdef __WXGTK__
    fhei += 1;
#endif

#ifdef __WXMAC__
    // these are the numbers from the HIG:
    switch ( m_windowVariant )
    {
        case wxWINDOW_VARIANT_NORMAL:
        default :
            fhei = 22;
            break;
        case wxWINDOW_VARIANT_SMALL:
            fhei = 19;
            break;
        case wxWINDOW_VARIANT_MINI:
            fhei = 15;
            break;
    }
#endif

    fhei += 2 * FOCUS_RING;
    int width = sizeText.x + FOCUS_RING + COMBO_MARGIN + DEFAULT_DROPBUTTON_WIDTH;

    wxSize ret(width, fhei);
    CacheBestSize(ret);
    return ret;
}

void wxComboCtrlBase::OnSizeEvent( wxSizeEvent& event )
{
    if ( !IsCreated() )
        return;

    // defined by actual wxComboCtrls
    OnResize();

    event.Skip();
}

// ----------------------------------------------------------------------------
// standard operations
// ----------------------------------------------------------------------------

bool wxComboCtrlBase::Enable(bool enable)
{
    if ( !wxControl::Enable(enable) )
        return false;

    if ( m_btn )
        m_btn->Enable(enable);
    if ( m_text )
        m_text->Enable(enable);
        
    Refresh();

    return true;
}

bool wxComboCtrlBase::Show(bool show)
{
    if ( !wxControl::Show(show) )
        return false;

    if (m_btn)
        m_btn->Show(show);

    if (m_text)
        m_text->Show(show);

    return true;
}

bool wxComboCtrlBase::SetFont ( const wxFont& font )
{
    if ( !wxControl::SetFont(font) )
        return false;

    if (m_text)
        m_text->SetFont(font);

    return true;
}

#if wxUSE_TOOLTIPS
void wxComboCtrlBase::DoSetToolTip(wxToolTip *tooltip)
{
    wxControl::DoSetToolTip(tooltip);

    // Set tool tip for button and text box
    if ( tooltip )
    {
        const wxString &tip = tooltip->GetTip();
        if ( m_text ) m_text->SetToolTip(tip);
        if ( m_btn ) m_btn->SetToolTip(tip);
    }
    else
    {
        if ( m_text ) m_text->SetToolTip( (wxToolTip*) NULL );
        if ( m_btn ) m_btn->SetToolTip( (wxToolTip*) NULL );
    }
}
#endif // wxUSE_TOOLTIPS

#if wxUSE_VALIDATORS
void wxComboCtrlBase::SetValidator(const wxValidator& validator)
{
    wxTextCtrl* textCtrl = GetTextCtrl();

    if ( textCtrl )
        textCtrl->SetValidator( validator );
    else
        wxControl::SetValidator( validator );
}

wxValidator* wxComboCtrlBase::GetValidator()
{
    wxTextCtrl* textCtrl = GetTextCtrl();

    return textCtrl ? textCtrl->GetValidator() : wxControl::GetValidator();
}
#endif // wxUSE_VALIDATORS

// ----------------------------------------------------------------------------
// painting
// ----------------------------------------------------------------------------

#if (!defined(__WXMSW__)) || defined(__WXUNIVERSAL__)
// prepare combo box background on area in a way typical on platform
void wxComboCtrlBase::PrepareBackground( wxDC& dc, const wxRect& rect, int flags ) const
{
    wxSize sz = GetClientSize();
    bool isEnabled;
    bool isFocused; // also selected

    // For smaller size control (and for disabled background) use less spacing
    int focusSpacingX;
    int focusSpacingY;

    if ( !(flags & wxCONTROL_ISSUBMENU) )
    {
        // Drawing control
        isEnabled = IsEnabled();
        isFocused = ShouldDrawFocus();

        // Windows-style: for smaller size control (and for disabled background) use less spacing
        focusSpacingX = isEnabled ? 2 : 1;
        focusSpacingY = sz.y > (GetCharHeight()+2) && isEnabled ? 2 : 1;
    }
    else
    {
        // Drawing a list item
        isEnabled = true; // they are never disabled
        isFocused = flags & wxCONTROL_SELECTED ? true : false;

        focusSpacingX = 0;
        focusSpacingY = 0;
    }

    // Set the background sub-rectangle for selection, disabled etc
    wxRect selRect(rect);
    selRect.y += focusSpacingY;
    selRect.height -= (focusSpacingY*2);

    int wcp = 0;

    if ( !(flags & wxCONTROL_ISSUBMENU) )
        wcp += m_widthCustomPaint;

    selRect.x += wcp + focusSpacingX;
    selRect.width -= wcp + (focusSpacingX*2);

    wxColour fgCol;
    wxColour bgCol;

    if ( isEnabled )
    {
        if ( isFocused )
            fgCol = wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHTTEXT);
        else if ( m_hasFgCol )
            // Honour the custom foreground colour
            fgCol = GetForegroundColour();
        else
            fgCol = wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT);
    }
    else
    {
        fgCol = wxSystemSettings::GetColour(wxSYS_COLOUR_GRAYTEXT);
    }

    if ( isEnabled )
    {
        if ( isFocused )
            bgCol = wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHT);
        else if ( m_hasBgCol )
            // Honour the custom background colour
            bgCol = GetBackgroundColour();
        else
#if defined(__WXMAC__) || defined(__WXGTK__)  // see note in OnThemeChange
            bgCol = wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW);
#else
            bgCol = GetBackgroundColour();
#endif
    }
    else
    {
#if defined(__WXMAC__) || defined(__WXGTK__)  // see note in OnThemeChange
        bgCol = wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW);
#else
        bgCol = GetBackgroundColour();
#endif
    }

    dc.SetTextForeground( fgCol );
    dc.SetBrush( bgCol );
    dc.SetPen( bgCol );
    dc.DrawRectangle( selRect );

    // Don't clip exactly to the selection rectangle so we can draw
    // to the non-selected area in front of it.
    wxRect clipRect(rect.x,rect.y,
                    (selRect.x+selRect.width)-rect.x,rect.height);
    dc.SetClippingRegion(clipRect);
}
#else
// Save the library size a bit for platforms that re-implement this.
void wxComboCtrlBase::PrepareBackground( wxDC&, const wxRect&, int ) const
{
}
#endif

void wxComboCtrlBase::DrawButton( wxDC& dc, const wxRect& rect, int flags )
{
    int drawState = m_btnState;

#ifdef __WXGTK__
    if ( GetPopupWindowState() >= Animating )
        drawState |= wxCONTROL_PRESSED;
#endif

    wxRect drawRect(rect.x+m_btnSpacingX,
                    rect.y+((rect.height-m_btnSize.y)/2),
                    m_btnSize.x,
                    m_btnSize.y);

    // Make sure area is not larger than the control
    if ( drawRect.y < rect.y )
        drawRect.y = rect.y;
    if ( drawRect.height > rect.height )
        drawRect.height = rect.height;

    bool enabled = IsEnabled();

    if ( !enabled )
        drawState |= wxCONTROL_DISABLED;

    if ( !m_bmpNormal.Ok() )
    {
        if ( flags & Draw_BitmapOnly )
            return;

        // Need to clear button background even if m_btn is present
        if ( flags & Draw_PaintBg )
        {
            wxColour bgCol;

            if ( m_iFlags & wxCC_IFLAG_BUTTON_OUTSIDE )
                bgCol = GetParent()->GetBackgroundColour();
            else
                bgCol = GetBackgroundColour();

            dc.SetBrush(bgCol);
            dc.SetPen(bgCol);
            dc.DrawRectangle(rect);
        }

        // Draw standard button
        wxRendererNative::Get().DrawComboBoxDropButton(this,
                                                       dc,
                                                       drawRect,
                                                       drawState);
    }
    else
    {
        // Draw bitmap

        wxBitmap* pBmp;

        if ( !enabled )
            pBmp = &m_bmpDisabled;
        else if ( m_btnState & wxCONTROL_PRESSED )
            pBmp = &m_bmpPressed;
        else if ( m_btnState & wxCONTROL_CURRENT )
            pBmp = &m_bmpHover;
        else
            pBmp = &m_bmpNormal;

        if ( m_blankButtonBg )
        {
            // If using blank button background, we need to clear its background
            // with button face colour instead of colour for rest of the control.
            if ( flags & Draw_PaintBg )
            {
                wxColour bgCol = GetParent()->GetBackgroundColour(); //wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE);
                //wxColour bgCol = wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW);
                dc.SetPen(bgCol);
                dc.SetBrush(bgCol);
                dc.DrawRectangle(rect);
            }

            wxRendererNative::Get().DrawPushButton(this,
                                                   dc,
                                                   drawRect,
                                                   drawState);

        }
        else

        {
            // Need to clear button background even if m_btn is present
            // (assume non-button background was cleared just before this call so brushes are good)
            if ( flags & Draw_PaintBg )
                dc.DrawRectangle(rect);
        }

        // Draw bitmap centered in drawRect
        dc.DrawBitmap(*pBmp,
                      drawRect.x + (drawRect.width-pBmp->GetWidth())/2,
                      drawRect.y + (drawRect.height-pBmp->GetHeight())/2,
                      true);
    }
}

void wxComboCtrlBase::RecalcAndRefresh()
{
    if ( IsCreated() )
    {
        wxSizeEvent evt(GetSize(),GetId());
        GetEventHandler()->ProcessEvent(evt);
        Refresh();
    }
}

// ----------------------------------------------------------------------------
// miscellaneous event handlers
// ----------------------------------------------------------------------------

void wxComboCtrlBase::OnTextCtrlEvent(wxCommandEvent& event)
{
    if ( event.GetEventType() == wxEVT_COMMAND_TEXT_UPDATED )
    {
        if ( m_ignoreEvtText > 0 )
        {
            m_ignoreEvtText--;
            return;
        }
    }

    // Change event id, object and string before relaying it forward
    event.SetId(GetId());
    wxString s = event.GetString();
    event.SetEventObject(this);
    event.SetString(s);
    event.Skip();
}

// call if cursor is on button area or mouse is captured for the button
bool wxComboCtrlBase::HandleButtonMouseEvent( wxMouseEvent& event,
                                                 int flags )
{
    int type = event.GetEventType();

    if ( type == wxEVT_MOTION )
    {
        if ( flags & wxCC_MF_ON_BUTTON )
        {
            if ( !(m_btnState & wxCONTROL_CURRENT) )
            {
                // Mouse hover begins
                m_btnState |= wxCONTROL_CURRENT;
                if ( HasCapture() ) // Retain pressed state.
                    m_btnState |= wxCONTROL_PRESSED;
                Refresh();
            }
        }
        else if ( (m_btnState & wxCONTROL_CURRENT) )
        {
            // Mouse hover ends
            m_btnState &= ~(wxCONTROL_CURRENT|wxCONTROL_PRESSED);
            Refresh();
        }
    }
    else if ( type == wxEVT_LEFT_DOWN || type == wxEVT_LEFT_DCLICK )
    {
        if ( flags & (wxCC_MF_ON_CLICK_AREA|wxCC_MF_ON_BUTTON) )
        {
            m_btnState |= wxCONTROL_PRESSED;
            Refresh();

            if ( !(m_iFlags & wxCC_POPUP_ON_MOUSE_UP) )
                OnButtonClick();
            else
                // If showing popup now, do not capture mouse or there will be interference
                CaptureMouse();
        }
    }
    else if ( type == wxEVT_LEFT_UP )
    {

        // Only accept event if mouse was left-press was previously accepted
        if ( HasCapture() )
            ReleaseMouse();

        if ( m_btnState & wxCONTROL_PRESSED )
        {
            // If mouse was inside, fire the click event.
            if ( m_iFlags & wxCC_POPUP_ON_MOUSE_UP )
            {
                if ( flags & (wxCC_MF_ON_CLICK_AREA|wxCC_MF_ON_BUTTON) )
                    OnButtonClick();
            }

            m_btnState &= ~(wxCONTROL_PRESSED);
            Refresh();
        }
    }
    else if ( type == wxEVT_LEAVE_WINDOW )
    {
        if ( m_btnState & (wxCONTROL_CURRENT|wxCONTROL_PRESSED) )
        {
            m_btnState &= ~(wxCONTROL_CURRENT);

            // Mouse hover ends
            if ( IsPopupWindowState(Hidden) )
            {
                m_btnState &= ~(wxCONTROL_PRESSED);
                Refresh();
            }
        }
    }
    else
        return false;

    return true;
}

// returns true if event was consumed or filtered
bool wxComboCtrlBase::PreprocessMouseEvent( wxMouseEvent& event,
                                            int WXUNUSED(flags) )
{
    wxLongLong t = ::wxGetLocalTimeMillis();
    int evtType = event.GetEventType();

#if USES_WXPOPUPWINDOW || USES_WXDIALOG
    if ( m_popupWinType != POPUPWIN_WXPOPUPTRANSIENTWINDOW )
    {
        if ( IsPopupWindowState(Visible) &&
             ( evtType == wxEVT_LEFT_DOWN || evtType == wxEVT_RIGHT_DOWN ) )
        {
            HidePopup();
            return true;
        }
    }
#endif

    // Filter out clicks on button immediately after popup dismiss (Windows like behaviour)
    if ( evtType == wxEVT_LEFT_DOWN && t < m_timeCanAcceptClick )
    {
        event.SetEventType(0);
        return true;
    }

    return false;
}

void wxComboCtrlBase::HandleNormalMouseEvent( wxMouseEvent& event )
{
    int evtType = event.GetEventType();

    if ( (evtType == wxEVT_LEFT_DOWN || evtType == wxEVT_LEFT_DCLICK) &&
         (m_windowStyle & wxCB_READONLY) )
    {
        if ( GetPopupWindowState() >= Animating )
        {
    #if USES_WXPOPUPWINDOW
            // Click here always hides the popup.
            if ( m_popupWinType == POPUPWIN_WXPOPUPWINDOW )
                HidePopup();
    #endif
        }
        else
        {
            if ( !(m_windowStyle & wxCC_SPECIAL_DCLICK) )
            {
                // In read-only mode, clicking the text is the
                // same as clicking the button.
                OnButtonClick();
            }
            else if ( /*evtType == wxEVT_LEFT_UP || */evtType == wxEVT_LEFT_DCLICK )
            {
                //if ( m_popupInterface->CycleValue() )
                //    Refresh();
                if ( m_popupInterface )
                    m_popupInterface->OnComboDoubleClick();
            }
        }
    }
    else
    if ( IsPopupShown() )
    {
        // relay (some) mouse events to the popup
        if ( evtType == wxEVT_MOUSEWHEEL )
            m_popup->AddPendingEvent(event);
    }
    else if ( evtType )
        event.Skip();
}

void wxComboCtrlBase::OnKeyEvent(wxKeyEvent& event)
{
    if ( IsPopupShown() )
    {
        // pass it to the popped up control
        GetPopupControl()->GetControl()->AddPendingEvent(event);
    }
    else // no popup
    {
        int keycode = event.GetKeyCode();

        wxWindow* mainCtrl = GetMainWindowOfCompositeControl();

        if ( mainCtrl->GetParent()->HasFlag(wxTAB_TRAVERSAL) &&
             keycode == WXK_TAB )
        {
            wxNavigationKeyEvent evt;

            evt.SetFlags(wxNavigationKeyEvent::FromTab|
                         (!event.ShiftDown() ? wxNavigationKeyEvent::IsForward
                                             : wxNavigationKeyEvent::IsBackward));
            evt.SetEventObject(mainCtrl);
            evt.SetCurrentFocus(mainCtrl);
            mainCtrl->GetParent()->GetEventHandler()->AddPendingEvent(evt);
            return;
        }

        if ( IsKeyPopupToggle(event) )
        {
            OnButtonClick();
            return;
        }

        int comboStyle = GetWindowStyle();
        wxComboPopup* popupInterface = GetPopupControl();

        if ( !popupInterface )
        {
            event.Skip();
            return;
        }

        if ( (comboStyle & wxCB_READONLY) ||
             (keycode != WXK_RIGHT && keycode != WXK_LEFT) )
        {
            popupInterface->OnComboKeyEvent(event);
        }
        else
            event.Skip();
    }
}

void wxComboCtrlBase::OnFocusEvent( wxFocusEvent& event )
{
    if ( event.GetEventType() == wxEVT_SET_FOCUS )
    {
        wxWindow* tc = GetTextCtrl();
        if ( tc && tc != DoFindFocus() )
#ifdef __WXMAC__
            m_resetFocus = true;
#else
            tc->SetFocus();
#endif
    }

    Refresh();
}

void wxComboCtrlBase::OnIdleEvent( wxIdleEvent& WXUNUSED(event) )
{
    if ( m_resetFocus )
    {
        m_resetFocus = false;
        wxWindow* tc = GetTextCtrl();
        if ( tc )
            tc->SetFocus();
    }
}

void wxComboCtrlBase::OnSysColourChanged(wxSysColourChangedEvent& WXUNUSED(event))
{
    OnThemeChange();
    // indentation may also have changed
    if ( !(m_iFlags & wxCC_IFLAG_INDENT_SET) )
        m_absIndent = GetNativeTextIndent();
    RecalcAndRefresh();
}

// ----------------------------------------------------------------------------
// popup handling
// ----------------------------------------------------------------------------

// Create popup window and the child control
void wxComboCtrlBase::CreatePopup()
{
    wxComboPopup* popupInterface = m_popupInterface;
    wxWindow* popup;

    if ( !m_winPopup )
    {
#ifdef wxComboPopupWindowBase2
        if ( m_iFlags & wxCC_IFLAG_USE_ALT_POPUP )
        {
        #if !USES_WXDIALOG
            m_winPopup = new wxComboPopupWindowBase2( this, wxNO_BORDER );
        #else
            m_winPopup = new wxComboPopupWindowBase2( this, wxID_ANY, wxEmptyString,
                                                      wxPoint(-21,-21), wxSize(20, 20),
                                                      wxNO_BORDER );
        #endif
            m_popupWinType = SECONDARY_POPUP_TYPE;
        }
        else
#endif
        {
            m_winPopup = new wxComboPopupWindow( this, wxNO_BORDER );
            m_popupWinType = PRIMARY_POPUP_TYPE;
        }
        m_popupWinEvtHandler = new wxComboPopupWindowEvtHandler(this);
        m_winPopup->PushEventHandler(m_popupWinEvtHandler);
    }

    popupInterface->Create(m_winPopup);
    m_popup = popup = popupInterface->GetControl();

    m_popupExtraHandler = new wxComboPopupExtraEventHandler(this);
    popup->PushEventHandler( m_popupExtraHandler );

    // This may be helpful on some platforms
    //   (eg. it bypasses a wxGTK popupwindow bug where
    //    window is not initially hidden when it should be)
    m_winPopup->Hide();

    popupInterface->m_iFlags |= wxCP_IFLAG_CREATED;
}

// Destroy popup window and the child control
void wxComboCtrlBase::DestroyPopup()
{
    HidePopup();

    if ( m_popup )
        m_popup->RemoveEventHandler(m_popupExtraHandler);

    delete m_popupExtraHandler;

    delete m_popupInterface;

    if ( m_winPopup )
    {
        m_winPopup->RemoveEventHandler(m_popupWinEvtHandler);
        delete m_popupWinEvtHandler;
        m_popupWinEvtHandler = NULL;
        m_winPopup->Destroy();
    }

    m_popupExtraHandler = (wxEvtHandler*) NULL;
    m_popupInterface = (wxComboPopup*) NULL;
    m_winPopup = (wxWindow*) NULL;
    m_popup = (wxWindow*) NULL;
}

void wxComboCtrlBase::DoSetPopupControl(wxComboPopup* iface)
{
    wxCHECK_RET( iface, wxT("no popup interface set for wxComboCtrl") );

    DestroyPopup();

    iface->InitBase(this);
    iface->Init();

    m_popupInterface = iface;

    if ( !iface->LazyCreate() )
    {
        CreatePopup();
    }
    else
    {
        m_popup = (wxWindow*) NULL;
    }

    // This must be done after creation
    if ( m_valueString.length() )
    {
        iface->SetStringValue(m_valueString);
        //Refresh();
    }
}

// Ensures there is atleast the default popup
void wxComboCtrlBase::EnsurePopupControl()
{
    if ( !m_popupInterface )
        SetPopupControl(NULL);
}

void wxComboCtrlBase::OnButtonClick()
{
    // Derived classes can override this method for totally custom
    // popup action
    if ( !IsPopupWindowState(Visible) )
        ShowPopup();
    else
        HidePopup();
}

void wxComboCtrlBase::ShowPopup()
{
    EnsurePopupControl();
    wxCHECK_RET( !IsPopupWindowState(Visible), wxT("popup window already shown") );

    if ( IsPopupWindowState(Animating) )
        return;

    SetFocus();

    // Space above and below
    int screenHeight;
    wxPoint scrPos;
    int spaceAbove;
    int spaceBelow;
    int maxHeightPopup;
    wxSize ctrlSz = GetSize();

    screenHeight = wxSystemSettings::GetMetric( wxSYS_SCREEN_Y );
    scrPos = GetParent()->ClientToScreen(GetPosition());

    spaceAbove = scrPos.y;
    spaceBelow = screenHeight - spaceAbove - ctrlSz.y;

    maxHeightPopup = spaceBelow;
    if ( spaceAbove > spaceBelow )
        maxHeightPopup = spaceAbove;

    // Width
    int widthPopup = ctrlSz.x + m_extLeft + m_extRight;

    if ( widthPopup < m_widthMinPopup )
        widthPopup = m_widthMinPopup;

    wxWindow* winPopup = m_winPopup;
    wxWindow* popup;

    // Need to disable tab traversal of parent
    //
    // NB: This is to fix a bug in wxMSW. In theory it could also be fixed
    //     by, for instance, adding check to window.cpp:wxWindowMSW::MSWProcessMessage
    //     that if transient popup is open, then tab traversal is to be ignored.
    //     However, I think this code would still be needed for cases where
    //     transient popup doesn't work yet (wxWinCE?).
    wxWindow* parent = GetParent();
    int parentFlags = parent->GetWindowStyle();
    if ( parentFlags & wxTAB_TRAVERSAL )
    {
        parent->SetWindowStyle( parentFlags & ~(wxTAB_TRAVERSAL) );
        m_iFlags |= wxCC_IFLAG_PARENT_TAB_TRAVERSAL;
    }

    if ( !winPopup )
    {
        CreatePopup();
        winPopup = m_winPopup;
        popup = m_popup;
    }
    else
    {
        popup = m_popup;
    }

    winPopup->Enable();

    wxASSERT( !m_popup || m_popup == popup ); // Consistency check.

    wxSize adjustedSize = m_popupInterface->GetAdjustedSize(widthPopup,
                                                            m_heightPopup<=0?DEFAULT_POPUP_HEIGHT:m_heightPopup,
                                                            maxHeightPopup);

    popup->SetSize(adjustedSize);
    popup->Move(0,0);
    m_popupInterface->OnPopup();

    //
    // Reposition and resize popup window
    //

    wxSize szp = popup->GetSize();

    int popupX;
    int popupY = scrPos.y + ctrlSz.y;

    // Default anchor is wxLEFT
    int anchorSide = m_anchorSide;
    if ( !anchorSide )
        anchorSide = wxLEFT;

    int rightX = scrPos.x + ctrlSz.x + m_extRight - szp.x;
    int leftX = scrPos.x - m_extLeft;

    if ( wxTheApp->GetLayoutDirection() == wxLayout_RightToLeft )
        leftX -= ctrlSz.x;

    int screenWidth = wxSystemSettings::GetMetric( wxSYS_SCREEN_X );

    // If there is not enough horizontal space, anchor on the other side.
    // If there is no space even then, place the popup at x 0.
    if ( anchorSide == wxRIGHT )
    {
        if ( rightX < 0 )
        {
            if ( (leftX+szp.x) < screenWidth )
                anchorSide = wxLEFT;
            else
                anchorSide = 0;
        }
    }
    else
    {
        if ( (leftX+szp.x) >= screenWidth )
        {
            if ( rightX >= 0 )
                anchorSide = wxRIGHT;
            else
                anchorSide = 0;
        }
    }

    // Select x coordinate according to the anchor side
    if ( anchorSide == wxRIGHT )
        popupX = rightX;
    else if ( anchorSide == wxLEFT )
        popupX = leftX;
    else
        popupX = 0;

    int showFlags = CanDeferShow;

    if ( spaceBelow < szp.y )
    {
        popupY = scrPos.y - szp.y;
        showFlags |= ShowAbove;
    }

#if INSTALL_TOPLEV_HANDLER
    // Put top level window event handler into place
    if ( m_popupWinType == POPUPWIN_WXPOPUPWINDOW )
    {
        if ( !m_toplevEvtHandler )
            m_toplevEvtHandler = new wxComboFrameEventHandler(this);

        wxWindow* toplev = ::wxGetTopLevelParent( this );
        wxASSERT( toplev );
        ((wxComboFrameEventHandler*)m_toplevEvtHandler)->OnPopup();
        toplev->PushEventHandler( m_toplevEvtHandler );
    }
#endif

    // Set string selection (must be this way instead of SetStringSelection)
    if ( m_text )
    {
        if ( !(m_iFlags & wxCC_NO_TEXT_AUTO_SELECT) )
            m_text->SelectAll();

        m_popupInterface->SetStringValue( m_text->GetValue() );
    }
    else
    {
        // This is neede since focus/selection indication may change when popup is shown
        Refresh();
    }

    // This must be after SetStringValue
    m_popupWinState = Animating;

    wxRect popupWinRect( popupX, popupY, szp.x, szp.y );

    m_popup = popup;
    if ( (m_iFlags & wxCC_IFLAG_DISABLE_POPUP_ANIM) ||
         AnimateShow( popupWinRect, showFlags ) )
    {
        DoShowPopup( popupWinRect, showFlags );
    }
}

bool wxComboCtrlBase::AnimateShow( const wxRect& WXUNUSED(rect), int WXUNUSED(flags) )
{
    return true;
}

void wxComboCtrlBase::DoShowPopup( const wxRect& rect, int WXUNUSED(flags) )
{
    wxWindow* winPopup = m_winPopup;

    if ( IsPopupWindowState(Animating) )
    {
        // Make sure the popup window is shown in the right position.
        // Should not matter even if animation already did this.

        // Some platforms (GTK) may like SetSize and Move to be separate
        // (though the bug was probably fixed).
        winPopup->SetSize( rect );

        winPopup->Show();

        m_popupWinState = Visible;
    }
    else if ( IsPopupWindowState(Hidden) )
    {
        // Animation was aborted

        wxASSERT( !winPopup->IsShown() );

        m_popupWinState = Hidden;
    }
}

void wxComboCtrlBase::OnPopupDismiss()
{
    // Just in case, avoid double dismiss
    if ( IsPopupWindowState(Hidden) )
        return;

    // This must be set before focus - otherwise there will be recursive
    // OnPopupDismisses.
    m_popupWinState = Hidden;

    //SetFocus();
    m_winPopup->Disable();

    // Inform popup control itself
    m_popupInterface->OnDismiss();

    if ( m_popupExtraHandler )
        ((wxComboPopupExtraEventHandler*)m_popupExtraHandler)->OnPopupDismiss();

#if INSTALL_TOPLEV_HANDLER
    // Remove top level window event handler
    if ( m_toplevEvtHandler )
    {
        wxWindow* toplev = ::wxGetTopLevelParent( this );
        if ( toplev )
            toplev->RemoveEventHandler( m_toplevEvtHandler );
    }
#endif

    m_timeCanAcceptClick = ::wxGetLocalTimeMillis();

    if ( m_popupWinType == POPUPWIN_WXPOPUPTRANSIENTWINDOW )
        m_timeCanAcceptClick += 150;

    // If cursor not on dropdown button, then clear its state
    // (technically not required by all ports, but do it for all just in case)
    if ( !m_btnArea.Contains(ScreenToClient(::wxGetMousePosition())) )
        m_btnState = 0;

    // Return parent's tab traversal flag.
    // See ShowPopup for notes.
    if ( m_iFlags & wxCC_IFLAG_PARENT_TAB_TRAVERSAL )
    {
        wxWindow* parent = GetParent();
        parent->SetWindowStyle( parent->GetWindowStyle() | wxTAB_TRAVERSAL );
        m_iFlags &= ~(wxCC_IFLAG_PARENT_TAB_TRAVERSAL);
    }

    // refresh control (necessary even if m_text)
    Refresh();

    SetFocus();
}

void wxComboCtrlBase::HidePopup()
{
    // Should be able to call this without popup interface
    if ( IsPopupWindowState(Hidden) )
        return;

    // transfer value and show it in textctrl, if any
    if ( !IsPopupWindowState(Animating) )
        SetValue( m_popupInterface->GetStringValue() );

    m_winPopup->Hide();

    OnPopupDismiss();
}

// ----------------------------------------------------------------------------
// customization methods
// ----------------------------------------------------------------------------

void wxComboCtrlBase::SetButtonPosition( int width, int height,
                                         int side, int spacingX )
{
    m_btnWid = width;
    m_btnHei = height;
    m_btnSide = side;
    m_btnSpacingX = spacingX;

    if ( width > 0 || height > 0 || spacingX )
        m_iFlags |= wxCC_IFLAG_HAS_NONSTANDARD_BUTTON;

    RecalcAndRefresh();
}

wxSize wxComboCtrlBase::GetButtonSize()
{
    if ( m_btnSize.x > 0 )
        return m_btnSize;

    wxSize retSize(m_btnWid,m_btnHei);

    // Need to call CalculateAreas now if button size is
    // is not explicitly specified.
    if ( retSize.x <= 0 || retSize.y <= 0)
    {
        OnResize();

        retSize = m_btnSize;
    }

    return retSize;
}

void wxComboCtrlBase::SetButtonBitmaps( const wxBitmap& bmpNormal,
                                           bool blankButtonBg,
                                           const wxBitmap& bmpPressed,
                                           const wxBitmap& bmpHover,
                                           const wxBitmap& bmpDisabled )
{
    m_bmpNormal = bmpNormal;
    m_blankButtonBg = blankButtonBg;

    if ( bmpPressed.Ok() )
        m_bmpPressed = bmpPressed;
    else
        m_bmpPressed = bmpNormal;

    if ( bmpHover.Ok() )
        m_bmpHover = bmpHover;
    else
        m_bmpHover = bmpNormal;

    if ( bmpDisabled.Ok() )
        m_bmpDisabled = bmpDisabled;
    else
        m_bmpDisabled = bmpNormal;

    RecalcAndRefresh();
}

void wxComboCtrlBase::SetCustomPaintWidth( int width )
{
    if ( m_text )
    {
        // move textctrl accordingly
        wxRect r = m_text->GetRect();
        int inc = width - m_widthCustomPaint;
        r.x += inc;
        r.width -= inc;
        m_text->SetSize( r );
    }

    m_widthCustomPaint = width;

    RecalcAndRefresh();
}

void wxComboCtrlBase::SetTextIndent( int indent )
{
    if ( indent < 0 )
    {
        m_absIndent = GetNativeTextIndent();
        m_iFlags &= ~(wxCC_IFLAG_INDENT_SET);
    }
    else
    {
        m_absIndent = indent;
        m_iFlags |= wxCC_IFLAG_INDENT_SET;
    }

    RecalcAndRefresh();
}

wxCoord wxComboCtrlBase::GetNativeTextIndent() const
{
    return DEFAULT_TEXT_INDENT;
}

// ----------------------------------------------------------------------------
// methods forwarded to wxTextCtrl
// ----------------------------------------------------------------------------

wxString wxComboCtrlBase::GetValue() const
{
    if ( m_text )
        return m_text->GetValue();
    return m_valueString;
}

void wxComboCtrlBase::SetValueWithEvent(const wxString& value, bool withEvent)
{
    if ( m_text )
    {
        if ( !withEvent )
            m_ignoreEvtText++;

        m_text->SetValue(value);
        if ( !(m_iFlags & wxCC_NO_TEXT_AUTO_SELECT) )
            m_text->SelectAll();
    }

    // Since wxComboPopup may want to paint the combo as well, we need
    // to set the string value here (as well as sometimes in ShowPopup).
    if ( m_valueString != value )
    {
        m_valueString = value;

        EnsurePopupControl();

        if (m_popupInterface)
            m_popupInterface->SetStringValue(value);
    }

    Refresh();
}

void wxComboCtrlBase::SetValue(const wxString& value)
{
    SetValueWithEvent(value, false);
}

// In this SetValue variant wxComboPopup::SetStringValue is not called
void wxComboCtrlBase::SetText(const wxString& value)
{
    // Unlike in SetValue(), this must be called here or
    // the behaviour will no be consistent in readonlys.
    EnsurePopupControl();

    m_valueString = value;

    if ( m_text )
    {
        m_ignoreEvtText++;
        m_text->SetValue( value );
    }

    Refresh();
}

void wxComboCtrlBase::Copy()
{
    if ( m_text )
        m_text->Copy();
}

void wxComboCtrlBase::Cut()
{
    if ( m_text )
        m_text->Cut();
}

void wxComboCtrlBase::Paste()
{
    if ( m_text )
        m_text->Paste();
}

void wxComboCtrlBase::SetInsertionPoint(long pos)
{
    if ( m_text )
        m_text->SetInsertionPoint(pos);
}

void wxComboCtrlBase::SetInsertionPointEnd()
{
    if ( m_text )
        m_text->SetInsertionPointEnd();
}

long wxComboCtrlBase::GetInsertionPoint() const
{
    if ( m_text )
        return m_text->GetInsertionPoint();

    return 0;
}

long wxComboCtrlBase::GetLastPosition() const
{
    if ( m_text )
        return m_text->GetLastPosition();

    return 0;
}

void wxComboCtrlBase::Replace(long from, long to, const wxString& value)
{
    if ( m_text )
        m_text->Replace(from, to, value);
}

void wxComboCtrlBase::Remove(long from, long to)
{
    if ( m_text )
        m_text->Remove(from, to);
}

void wxComboCtrlBase::SetSelection(long from, long to)
{
    if ( m_text )
        m_text->SetSelection(from, to);
}

void wxComboCtrlBase::Undo()
{
    if ( m_text )
        m_text->Undo();
}

#endif // wxUSE_COMBOCTRL
