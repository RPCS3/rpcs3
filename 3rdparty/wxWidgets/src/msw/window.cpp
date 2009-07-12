/////////////////////////////////////////////////////////////////////////////
// Name:        src/msw/window.cpp
// Purpose:     wxWindowMSW
// Author:      Julian Smart
// Modified by: VZ on 13.05.99: no more Default(), MSWOnXXX() reorganisation
// Created:     04/01/98
// RCS-ID:      $Id: window.cpp 58750 2009-02-08 10:01:03Z VZ $
// Copyright:   (c) Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

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

#include "wx/window.h"

#ifndef WX_PRECOMP
    #include "wx/msw/wrapwin.h"
    #include "wx/msw/wrapcctl.h" // include <commctrl.h> "properly"
    #include "wx/msw/missing.h"
    #include "wx/accel.h"
    #include "wx/menu.h"
    #include "wx/dc.h"
    #include "wx/dcclient.h"
    #include "wx/dcmemory.h"
    #include "wx/utils.h"
    #include "wx/app.h"
    #include "wx/layout.h"
    #include "wx/dialog.h"
    #include "wx/frame.h"
    #include "wx/listbox.h"
    #include "wx/button.h"
    #include "wx/msgdlg.h"
    #include "wx/settings.h"
    #include "wx/statbox.h"
    #include "wx/sizer.h"
    #include "wx/intl.h"
    #include "wx/log.h"
    #include "wx/textctrl.h"
    #include "wx/menuitem.h"
    #include "wx/module.h"
#endif

#if wxUSE_OWNER_DRAWN && !defined(__WXUNIVERSAL__)
    #include "wx/ownerdrw.h"
#endif

#include "wx/evtloop.h"
#include "wx/power.h"
#include "wx/sysopt.h"

#if wxUSE_DRAG_AND_DROP
    #include "wx/dnd.h"
#endif

#if wxUSE_ACCESSIBILITY
    #include "wx/access.h"
    #include <ole2.h>
    #include <oleacc.h>
    #ifndef WM_GETOBJECT
        #define WM_GETOBJECT 0x003D
    #endif
    #ifndef OBJID_CLIENT
        #define OBJID_CLIENT 0xFFFFFFFC
    #endif
#endif

#include "wx/msw/private.h"

#if wxUSE_TOOLTIPS
    #include "wx/tooltip.h"
#endif

#if wxUSE_CARET
    #include "wx/caret.h"
#endif // wxUSE_CARET

#if wxUSE_SPINCTRL
    #include "wx/spinctrl.h"
#endif // wxUSE_SPINCTRL

#include "wx/notebook.h"
#include "wx/listctrl.h"
#include "wx/dynlib.h"

#include <string.h>

#if (!defined(__GNUWIN32_OLD__) && !defined(__WXMICROWIN__) /* && !defined(__WXWINCE__) */ ) || defined(__CYGWIN10__)
    #include <shellapi.h>
    #include <mmsystem.h>
#endif

#ifdef __WIN32__
    #include <windowsx.h>
#endif

#if !defined __WXWINCE__ && !defined NEED_PBT_H
    #include <pbt.h>
#endif

#if defined(__WXWINCE__)
    #include "wx/msw/wince/missing.h"
#ifdef __POCKETPC__
    #include <windows.h>
    #include <shellapi.h>
    #include <ole2.h>
    #include <aygshell.h>
#endif
#endif

#if wxUSE_UXTHEME
    #include "wx/msw/uxtheme.h"
    #define EP_EDITTEXT         1
    #define ETS_NORMAL          1
    #define ETS_HOT             2
    #define ETS_SELECTED        3
    #define ETS_DISABLED        4
    #define ETS_FOCUSED         5
    #define ETS_READONLY        6
    #define ETS_ASSIST          7
#endif

#if defined(TME_LEAVE) && defined(WM_MOUSELEAVE) && wxUSE_DYNLIB_CLASS
    #define HAVE_TRACKMOUSEEVENT
#endif // everything needed for TrackMouseEvent()

// if this is set to 1, we use deferred window sizing to reduce flicker when
// resizing complicated window hierarchies, but this can in theory result in
// different behaviour than the old code so we keep the possibility to use it
// by setting this to 0 (in the future this should be removed completely)
#ifdef __WXWINCE__
#define USE_DEFERRED_SIZING 0
#else
#define USE_DEFERRED_SIZING 1
#endif

// set this to 1 to filter out duplicate mouse events, e.g. mouse move events
// when mouse position didnd't change
#ifdef __WXWINCE__
    #define wxUSE_MOUSEEVENT_HACK 0
#else
    #define wxUSE_MOUSEEVENT_HACK 1
#endif

// ---------------------------------------------------------------------------
// global variables
// ---------------------------------------------------------------------------

#if wxUSE_MENUS_NATIVE
wxMenu *wxCurrentPopupMenu = NULL;
#endif // wxUSE_MENUS_NATIVE

#ifdef __WXWINCE__
extern       wxChar *wxCanvasClassName;
#else
extern const wxChar *wxCanvasClassName;
#endif

// true if we had already created the std colour map, used by
// wxGetStdColourMap() and wxWindow::OnSysColourChanged()           (FIXME-MT)
static bool gs_hasStdCmap = false;

// last mouse event information we need to filter out the duplicates
#if wxUSE_MOUSEEVENT_HACK
static struct MouseEventInfoDummy
{
    // mouse position (in screen coordinates)
    wxPoint pos;

    // last mouse event type
    wxEventType type;
} gs_lastMouseEvent;
#endif // wxUSE_MOUSEEVENT_HACK

// ---------------------------------------------------------------------------
// private functions
// ---------------------------------------------------------------------------

// the window proc for all our windows
LRESULT WXDLLEXPORT APIENTRY _EXPORT wxWndProc(HWND hWnd, UINT message,
                                   WPARAM wParam, LPARAM lParam);


#ifdef  __WXDEBUG__
    const wxChar *wxGetMessageName(int message);
#endif  //__WXDEBUG__

void wxRemoveHandleAssociation(wxWindowMSW *win);
extern void wxAssociateWinWithHandle(HWND hWnd, wxWindowMSW *win);
wxWindow *wxFindWinFromHandle(WXHWND hWnd);

// get the text metrics for the current font
static TEXTMETRIC wxGetTextMetrics(const wxWindowMSW *win);

#ifdef __WXWINCE__
// find the window for the mouse event at the specified position
static wxWindowMSW *FindWindowForMouseEvent(wxWindowMSW *win, int *x, int *y);
#endif // __WXWINCE__

// wrapper around BringWindowToTop() API
static inline void wxBringWindowToTop(HWND hwnd)
{
#ifdef __WXMICROWIN__
    // It seems that MicroWindows brings the _parent_ of the window to the top,
    // which can be the wrong one.

    // activate (set focus to) specified window
    ::SetFocus(hwnd);
#endif

    // raise top level parent to top of z order
    if (!::SetWindowPos(hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE))
    {
        wxLogLastError(_T("SetWindowPos"));
    }
}

#ifndef __WXWINCE__

// ensure that all our parent windows have WS_EX_CONTROLPARENT style
static void EnsureParentHasControlParentStyle(wxWindow *parent)
{
    /*
       If we have WS_EX_CONTROLPARENT flag we absolutely *must* set it for our
       parent as well as otherwise several Win32 functions using
       GetNextDlgTabItem() to iterate over all controls such as
       IsDialogMessage() or DefDlgProc() would enter an infinite loop: indeed,
       all of them iterate over all the controls starting from the currently
       focused one and stop iterating when they get back to the focus but
       unless all parents have WS_EX_CONTROLPARENT bit set, they would never
       get back to the initial (focused) window: as we do have this style,
       GetNextDlgTabItem() will leave this window and continue in its parent,
       but if the parent doesn't have it, it wouldn't recurse inside it later
       on and so wouldn't have a chance of getting back to this window either.
     */
    while ( parent && !parent->IsTopLevel() )
    {
        LONG exStyle = ::GetWindowLong(GetHwndOf(parent), GWL_EXSTYLE);
        if ( !(exStyle & WS_EX_CONTROLPARENT) )
        {
            // force the parent to have this style
            ::SetWindowLong(GetHwndOf(parent), GWL_EXSTYLE,
                            exStyle | WS_EX_CONTROLPARENT);
        }

        parent = parent->GetParent();
    }
}

#endif // !__WXWINCE__

#ifdef __WXWINCE__
// On Windows CE, GetCursorPos can return an error, so use this function
// instead
bool GetCursorPosWinCE(POINT* pt)
{
    if (!GetCursorPos(pt))
    {
        DWORD pos = GetMessagePos();
        pt->x = LOWORD(pos);
        pt->y = HIWORD(pos);
    }
    return true;
}
#endif

static wxBorder TranslateBorder(wxBorder border)
{
    if ( border == wxBORDER_THEME )
    {
#if defined(__POCKETPC__) || defined(__SMARTPHONE__)
        return wxBORDER_SIMPLE;
#elif wxUSE_UXTHEME
        if (wxUxThemeEngine::GetIfActive())
            return wxBORDER_THEME;
#endif
        return wxBORDER_SUNKEN;
    }

    return border;
}

// ---------------------------------------------------------------------------
// event tables
// ---------------------------------------------------------------------------

// in wxUniv/MSW this class is abstract because it doesn't have DoPopupMenu()
// method
#ifdef __WXUNIVERSAL__
    IMPLEMENT_ABSTRACT_CLASS(wxWindowMSW, wxWindowBase)
#else // __WXMSW__
#if wxUSE_EXTENDED_RTTI

// windows that are created from a parent window during its Create method, eg. spin controls in a calendar controls
// must never been streamed out separately otherwise chaos occurs. Right now easiest is to test for negative ids, as
// windows with negative ids never can be recreated anyway

bool wxWindowStreamingCallback( const wxObject *object, wxWriter * , wxPersister * , wxxVariantArray & )
{
    const wxWindow * win = dynamic_cast<const wxWindow*>(object) ;
    if ( win && win->GetId() < 0 )
        return false ;
    return true ;
}

IMPLEMENT_DYNAMIC_CLASS_XTI_CALLBACK(wxWindow, wxWindowBase,"wx/window.h", wxWindowStreamingCallback)

// make wxWindowList known before the property is used

wxCOLLECTION_TYPE_INFO( wxWindow* , wxWindowList ) ;

template<> void wxCollectionToVariantArray( wxWindowList const &theList, wxxVariantArray &value)
{
    wxListCollectionToVariantArray<wxWindowList::compatibility_iterator>( theList , value ) ;
}

WX_DEFINE_FLAGS( wxWindowStyle )

wxBEGIN_FLAGS( wxWindowStyle )
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
    wxFLAGS_MEMBER(wxBORDER)

    // standard window styles
    wxFLAGS_MEMBER(wxTAB_TRAVERSAL)
    wxFLAGS_MEMBER(wxCLIP_CHILDREN)
    wxFLAGS_MEMBER(wxTRANSPARENT_WINDOW)
    wxFLAGS_MEMBER(wxWANTS_CHARS)
    wxFLAGS_MEMBER(wxFULL_REPAINT_ON_RESIZE)
    wxFLAGS_MEMBER(wxALWAYS_SHOW_SB )
    wxFLAGS_MEMBER(wxVSCROLL)
    wxFLAGS_MEMBER(wxHSCROLL)

wxEND_FLAGS( wxWindowStyle )

wxBEGIN_PROPERTIES_TABLE(wxWindow)
    wxEVENT_PROPERTY( Close , wxEVT_CLOSE_WINDOW , wxCloseEvent)
    wxEVENT_PROPERTY( Create , wxEVT_CREATE , wxWindowCreateEvent )
    wxEVENT_PROPERTY( Destroy , wxEVT_DESTROY , wxWindowDestroyEvent )
    // Always constructor Properties first

    wxREADONLY_PROPERTY( Parent,wxWindow*, GetParent, EMPTY_MACROVALUE , 0 /*flags*/ , wxT("Helpstring") , wxT("group"))
    wxPROPERTY( Id,wxWindowID, SetId, GetId, -1 /*wxID_ANY*/ , 0 /*flags*/ , wxT("Helpstring") , wxT("group") )
    wxPROPERTY( Position,wxPoint, SetPosition , GetPosition, wxDefaultPosition , 0 /*flags*/ , wxT("Helpstring") , wxT("group")) // pos
    wxPROPERTY( Size,wxSize, SetSize, GetSize, wxDefaultSize , 0 /*flags*/ , wxT("Helpstring") , wxT("group")) // size
    wxPROPERTY( WindowStyle , long , SetWindowStyleFlag , GetWindowStyleFlag , EMPTY_MACROVALUE , 0 /*flags*/ , wxT("Helpstring") , wxT("group")) // style

    // Then all relations of the object graph

    wxREADONLY_PROPERTY_COLLECTION( Children , wxWindowList , wxWindowBase* , GetWindowChildren , wxPROP_OBJECT_GRAPH /*flags*/ , wxT("Helpstring") , wxT("group"))

   // and finally all other properties

    wxPROPERTY( ExtraStyle , long , SetExtraStyle , GetExtraStyle , EMPTY_MACROVALUE , 0 /*flags*/ , wxT("Helpstring") , wxT("group")) // extstyle
    wxPROPERTY( BackgroundColour , wxColour , SetBackgroundColour , GetBackgroundColour , EMPTY_MACROVALUE , 0 /*flags*/ , wxT("Helpstring") , wxT("group")) // bg
    wxPROPERTY( ForegroundColour , wxColour , SetForegroundColour , GetForegroundColour , EMPTY_MACROVALUE , 0 /*flags*/ , wxT("Helpstring") , wxT("group")) // fg
    wxPROPERTY( Enabled , bool , Enable , IsEnabled , wxxVariant((bool)true) , 0 /*flags*/ , wxT("Helpstring") , wxT("group"))
    wxPROPERTY( Shown , bool , Show , IsShown , wxxVariant((bool)true) , 0 /*flags*/ , wxT("Helpstring") , wxT("group"))
#if 0
    // possible property candidates (not in xrc) or not valid in all subclasses
    wxPROPERTY( Title,wxString, SetTitle, GetTitle, wxEmptyString )
    wxPROPERTY( Font , wxFont , SetFont , GetWindowFont  , )
    wxPROPERTY( Label,wxString, SetLabel, GetLabel, wxEmptyString )
    // MaxHeight, Width , MinHeight , Width
    // TODO switch label to control and title to toplevels

    wxPROPERTY( ThemeEnabled , bool , SetThemeEnabled , GetThemeEnabled , )
    //wxPROPERTY( Cursor , wxCursor , SetCursor , GetCursor , )
    // wxPROPERTY( ToolTip , wxString , SetToolTip , GetToolTipText , )
    wxPROPERTY( AutoLayout , bool , SetAutoLayout , GetAutoLayout , )



#endif
wxEND_PROPERTIES_TABLE()

wxBEGIN_HANDLERS_TABLE(wxWindow)
wxEND_HANDLERS_TABLE()

wxCONSTRUCTOR_DUMMY(wxWindow)

#else
    IMPLEMENT_DYNAMIC_CLASS(wxWindow, wxWindowBase)
#endif
#endif // __WXUNIVERSAL__/__WXMSW__

BEGIN_EVENT_TABLE(wxWindowMSW, wxWindowBase)
    EVT_SYS_COLOUR_CHANGED(wxWindowMSW::OnSysColourChanged)
    EVT_ERASE_BACKGROUND(wxWindowMSW::OnEraseBackground)
#ifdef __WXWINCE__
    EVT_INIT_DIALOG(wxWindowMSW::OnInitDialog)
#endif
END_EVENT_TABLE()

// ===========================================================================
// implementation
// ===========================================================================

// ---------------------------------------------------------------------------
// wxWindow utility functions
// ---------------------------------------------------------------------------

// Find an item given the MS Windows id
wxWindow *wxWindowMSW::FindItem(long id) const
{
#if wxUSE_CONTROLS
    wxControl *item = wxDynamicCastThis(wxControl);
    if ( item )
    {
        // is it us or one of our "internal" children?
        if ( item->GetId() == id
#ifndef __WXUNIVERSAL__
                || (item->GetSubcontrols().Index(id) != wxNOT_FOUND)
#endif // __WXUNIVERSAL__
           )
        {
            return item;
        }
    }
#endif // wxUSE_CONTROLS

    wxWindowList::compatibility_iterator current = GetChildren().GetFirst();
    while (current)
    {
        wxWindow *childWin = current->GetData();

        wxWindow *wnd = childWin->FindItem(id);
        if ( wnd )
            return wnd;

        current = current->GetNext();
    }

    return NULL;
}

// Find an item given the MS Windows handle
wxWindow *wxWindowMSW::FindItemByHWND(WXHWND hWnd, bool controlOnly) const
{
    wxWindowList::compatibility_iterator current = GetChildren().GetFirst();
    while (current)
    {
        wxWindow *parent = current->GetData();

        // Do a recursive search.
        wxWindow *wnd = parent->FindItemByHWND(hWnd);
        if ( wnd )
            return wnd;

        if ( !controlOnly
#if wxUSE_CONTROLS
                || parent->IsKindOf(CLASSINFO(wxControl))
#endif // wxUSE_CONTROLS
           )
        {
            wxWindow *item = current->GetData();
            if ( item->GetHWND() == hWnd )
                return item;
            else
            {
                if ( item->ContainsHWND(hWnd) )
                    return item;
            }
        }

        current = current->GetNext();
    }
    return NULL;
}

// Default command handler
bool wxWindowMSW::MSWCommand(WXUINT WXUNUSED(param), WXWORD WXUNUSED(id))
{
    return false;
}

// ----------------------------------------------------------------------------
// constructors and such
// ----------------------------------------------------------------------------

void wxWindowMSW::Init()
{
    // MSW specific
    m_isBeingDeleted = false;
    m_oldWndProc = NULL;
    m_mouseInWindow = false;
    m_lastKeydownProcessed = false;

    m_childrenDisabled = NULL;
    m_frozenness = 0;

    m_hWnd = 0;
    m_hDWP = 0;

    m_xThumbSize = 0;
    m_yThumbSize = 0;

    m_pendingPosition = wxDefaultPosition;
    m_pendingSize = wxDefaultSize;

#ifdef __POCKETPC__
    m_contextMenuEnabled = false;
#endif
}

// Destructor
wxWindowMSW::~wxWindowMSW()
{
    m_isBeingDeleted = true;

#ifndef __WXUNIVERSAL__
    // VS: make sure there's no wxFrame with last focus set to us:
    for ( wxWindow *win = GetParent(); win; win = win->GetParent() )
    {
        wxTopLevelWindow *frame = wxDynamicCast(win, wxTopLevelWindow);
        if ( frame )
        {
            if ( frame->GetLastFocus() == this )
            {
                frame->SetLastFocus(NULL);
            }

            // apparently sometimes we can end up with our grand parent
            // pointing to us as well: this is surely a bug in focus handling
            // code but it's not clear where it happens so for now just try to
            // fix it here by not breaking out of the loop
            //break;
        }
    }
#endif // __WXUNIVERSAL__

    // VS: destroy children first and _then_ detach *this from its parent.
    //     If we did it the other way around, children wouldn't be able
    //     find their parent frame (see above).
    DestroyChildren();

    if ( m_hWnd )
    {
        // VZ: test temp removed to understand what really happens here
        //if (::IsWindow(GetHwnd()))
        {
            if ( !::DestroyWindow(GetHwnd()) )
                wxLogLastError(wxT("DestroyWindow"));
        }

        // remove hWnd <-> wxWindow association
        wxRemoveHandleAssociation(this);
    }

    delete m_childrenDisabled;

}

// real construction (Init() must have been called before!)
bool wxWindowMSW::Create(wxWindow *parent,
                         wxWindowID id,
                         const wxPoint& pos,
                         const wxSize& size,
                         long style,
                         const wxString& name)
{
    wxCHECK_MSG( parent, false, wxT("can't create wxWindow without parent") );

    if ( !CreateBase(parent, id, pos, size, style, wxDefaultValidator, name) )
        return false;

    parent->AddChild(this);

    WXDWORD exstyle;
    DWORD msflags = MSWGetCreateWindowFlags(&exstyle);

#ifdef __WXUNIVERSAL__
    // no borders, we draw them ourselves
    exstyle &= ~(WS_EX_DLGMODALFRAME |
                 WS_EX_STATICEDGE |
                 WS_EX_CLIENTEDGE |
                 WS_EX_WINDOWEDGE);
    msflags &= ~WS_BORDER;
#endif // wxUniversal

    if ( IsShown() )
    {
        msflags |= WS_VISIBLE;
    }

    if ( !MSWCreate(wxCanvasClassName, NULL, pos, size, msflags, exstyle) )
        return false;

    InheritAttributes();

    return true;
}

// ---------------------------------------------------------------------------
// basic operations
// ---------------------------------------------------------------------------

void wxWindowMSW::SetFocus()
{
    HWND hWnd = GetHwnd();
    wxCHECK_RET( hWnd, _T("can't set focus to invalid window") );

#if !defined(__WXMICROWIN__) && !defined(__WXWINCE__)
    ::SetLastError(0);
#endif

    if ( !::SetFocus(hWnd) )
    {
#if defined(__WXDEBUG__) && !defined(__WXMICROWIN__)
        // was there really an error?
        DWORD dwRes = ::GetLastError();
        if ( dwRes )
        {
            HWND hwndFocus = ::GetFocus();
            if ( hwndFocus != hWnd )
            {
                wxLogApiError(_T("SetFocus"), dwRes);
            }
        }
#endif // Debug
    }
}

void wxWindowMSW::SetFocusFromKbd()
{
    // when the focus is given to the control with DLGC_HASSETSEL style from
    // keyboard its contents should be entirely selected: this is what
    // ::IsDialogMessage() does and so we should do it as well to provide the
    // same LNF as the native programs
    if ( ::SendMessage(GetHwnd(), WM_GETDLGCODE, 0, 0) & DLGC_HASSETSEL )
    {
        ::SendMessage(GetHwnd(), EM_SETSEL, 0, -1);
    }

    // do this after (maybe) setting the selection as like this when
    // wxEVT_SET_FOCUS handler is called, the selection would have been already
    // set correctly -- this may be important
    wxWindowBase::SetFocusFromKbd();
}

// Get the window with the focus
wxWindow *wxWindowBase::DoFindFocus()
{
    HWND hWnd = ::GetFocus();
    if ( hWnd )
    {
        return wxGetWindowFromHWND((WXHWND)hWnd);
    }

    return NULL;
}

bool wxWindowMSW::Enable(bool enable)
{
    if ( !wxWindowBase::Enable(enable) )
        return false;

    HWND hWnd = GetHwnd();
    if ( hWnd )
        ::EnableWindow(hWnd, (BOOL)enable);

    // the logic below doesn't apply to the top level windows -- otherwise
    // showing a modal dialog would result in total greying out (and ungreying
    // out later) of everything which would be really ugly
    if ( IsTopLevel() )
        return true;

    // when the parent is disabled, all of its children should be disabled as
    // well but when it is enabled back, only those of the children which
    // hadn't been already disabled in the beginning should be enabled again,
    // so we have to keep the list of those children
    for ( wxWindowList::compatibility_iterator node = GetChildren().GetFirst();
          node;
          node = node->GetNext() )
    {
        wxWindow *child = node->GetData();
        if ( child->IsTopLevel() )
        {
            // the logic below doesn't apply to top level children
            continue;
        }

        if ( enable )
        {
            // re-enable the child unless it had been disabled before us
            if ( !m_childrenDisabled || !m_childrenDisabled->Find(child) )
                child->Enable();
        }
        else // we're being disabled
        {
            if ( child->IsEnabled() )
            {
                // disable it as children shouldn't stay enabled while the
                // parent is not
                child->Disable();
            }
            else // child already disabled, remember it
            {
                // have we created the list of disabled children already?
                if ( !m_childrenDisabled )
                    m_childrenDisabled = new wxWindowList;

                m_childrenDisabled->Append(child);
            }
        }
    }

    if ( enable && m_childrenDisabled )
    {
        // we don't need this list any more, don't keep unused memory
        delete m_childrenDisabled;
        m_childrenDisabled = NULL;
    }

    return true;
}

bool wxWindowMSW::Show(bool show)
{
    if ( !wxWindowBase::Show(show) )
        return false;

    HWND hWnd = GetHwnd();

    // we could be called before the underlying window is created (this is
    // actually useful to prevent it from being initially shown), e.g.
    //
    //      wxFoo *foo = new wxFoo;
    //      foo->Hide();
    //      foo->Create(parent, ...);
    //
    // should work without errors
    if ( hWnd )
    {
        ::ShowWindow(hWnd, show ? SW_SHOW : SW_HIDE);
    }

    return true;
}

// Raise the window to the top of the Z order
void wxWindowMSW::Raise()
{
    wxBringWindowToTop(GetHwnd());
}

// Lower the window to the bottom of the Z order
void wxWindowMSW::Lower()
{
    ::SetWindowPos(GetHwnd(), HWND_BOTTOM, 0, 0, 0, 0,
                   SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
}

void wxWindowMSW::DoCaptureMouse()
{
    HWND hWnd = GetHwnd();
    if ( hWnd )
    {
        ::SetCapture(hWnd);
    }
}

void wxWindowMSW::DoReleaseMouse()
{
    if ( !::ReleaseCapture() )
    {
        wxLogLastError(_T("ReleaseCapture"));
    }
}

/* static */ wxWindow *wxWindowBase::GetCapture()
{
    HWND hwnd = ::GetCapture();
    return hwnd ? wxFindWinFromHandle((WXHWND)hwnd) : (wxWindow *)NULL;
}

bool wxWindowMSW::SetFont(const wxFont& font)
{
    if ( !wxWindowBase::SetFont(font) )
    {
        // nothing to do
        return false;
    }

    HWND hWnd = GetHwnd();
    if ( hWnd != 0 )
    {
        // note the use of GetFont() instead of m_font: our own font could have
        // just been reset and in this case we need to change the font used by
        // the native window to the default for this class, i.e. exactly what
        // GetFont() returns
        WXHANDLE hFont = GetFont().GetResourceHandle();

        wxASSERT_MSG( hFont, wxT("should have valid font") );

        ::SendMessage(hWnd, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));
    }

    return true;
}
bool wxWindowMSW::SetCursor(const wxCursor& cursor)
{
    if ( !wxWindowBase::SetCursor(cursor) )
    {
        // no change
        return false;
    }

    // don't "overwrite" busy cursor
    if ( m_cursor.Ok() && !wxIsBusy() )
    {
        // normally we should change the cursor only if it's over this window
        // but we should do it always if we capture the mouse currently
        bool set = HasCapture();
        if ( !set )
        {
            HWND hWnd = GetHwnd();

            POINT point;
#ifdef __WXWINCE__
            ::GetCursorPosWinCE(&point);
#else
            ::GetCursorPos(&point);
#endif

            RECT rect = wxGetWindowRect(hWnd);

            set = ::PtInRect(&rect, point) != 0;
        }

        if ( set )
        {
            ::SetCursor(GetHcursorOf(m_cursor));
        }
        //else: will be set later when the mouse enters this window
    }

    return true;
}

void wxWindowMSW::WarpPointer(int x, int y)
{
    ClientToScreen(&x, &y);

    if ( !::SetCursorPos(x, y) )
    {
        wxLogLastError(_T("SetCursorPos"));
    }
}

void wxWindowMSW::MSWUpdateUIState(int action, int state)
{
    // WM_CHANGEUISTATE only appeared in Windows 2000 so it can do us no good
    // to use it on older systems -- and could possibly do some harm
    static int s_needToUpdate = -1;
    if ( s_needToUpdate == -1 )
    {
        int verMaj, verMin;
        s_needToUpdate = wxGetOsVersion(&verMaj, &verMin) == wxOS_WINDOWS_NT &&
                            verMaj >= 5;
    }

    if ( s_needToUpdate )
    {
        // we send WM_CHANGEUISTATE so if nothing needs changing then the system
        // won't send WM_UPDATEUISTATE
        ::SendMessage(GetHwnd(), WM_CHANGEUISTATE, MAKEWPARAM(action, state), 0);
    }
}

// ---------------------------------------------------------------------------
// scrolling stuff
// ---------------------------------------------------------------------------

inline int GetScrollPosition(HWND hWnd, int wOrient)
{
#ifdef __WXMICROWIN__
    return ::GetScrollPosWX(hWnd, wOrient);
#else
    WinStruct<SCROLLINFO> scrollInfo;
    scrollInfo.cbSize = sizeof(SCROLLINFO);
    scrollInfo.fMask = SIF_POS;
    ::GetScrollInfo(hWnd, wOrient, &scrollInfo );

    return scrollInfo.nPos;

#endif
}

int wxWindowMSW::GetScrollPos(int orient) const
{
    HWND hWnd = GetHwnd();
    wxCHECK_MSG( hWnd, 0, _T("no HWND in GetScrollPos") );

    return GetScrollPosition(hWnd, orient == wxHORIZONTAL ? SB_HORZ : SB_VERT);
}

// This now returns the whole range, not just the number
// of positions that we can scroll.
int wxWindowMSW::GetScrollRange(int orient) const
{
    int maxPos;
    HWND hWnd = GetHwnd();
    if ( !hWnd )
        return 0;
#if 0
    ::GetScrollRange(hWnd, orient == wxHORIZONTAL ? SB_HORZ : SB_VERT,
                     &minPos, &maxPos);
#endif
    WinStruct<SCROLLINFO> scrollInfo;
    scrollInfo.fMask = SIF_RANGE;
    if ( !::GetScrollInfo(hWnd,
                          orient == wxHORIZONTAL ? SB_HORZ : SB_VERT,
                          &scrollInfo) )
    {
        // Most of the time this is not really an error, since the return
        // value can also be zero when there is no scrollbar yet.
        // wxLogLastError(_T("GetScrollInfo"));
    }
    maxPos = scrollInfo.nMax;

    // undo "range - 1" done in SetScrollbar()
    return maxPos + 1;
}

int wxWindowMSW::GetScrollThumb(int orient) const
{
    return orient == wxHORIZONTAL ? m_xThumbSize : m_yThumbSize;
}

void wxWindowMSW::SetScrollPos(int orient, int pos, bool refresh)
{
    HWND hWnd = GetHwnd();
    wxCHECK_RET( hWnd, _T("SetScrollPos: no HWND") );

    WinStruct<SCROLLINFO> info;
    info.nPage = 0;
    info.nMin = 0;
    info.nPos = pos;
    info.fMask = SIF_POS;
    if ( HasFlag(wxALWAYS_SHOW_SB) )
    {
        // disable scrollbar instead of removing it then
        info.fMask |= SIF_DISABLENOSCROLL;
    }

    ::SetScrollInfo(hWnd, orient == wxHORIZONTAL ? SB_HORZ : SB_VERT,
                    &info, refresh);
}

// New function that will replace some of the above.
void wxWindowMSW::SetScrollbar(int orient,
                               int pos,
                               int pageSize,
                               int range,
                               bool refresh)
{
    WinStruct<SCROLLINFO> info;
    info.nPage = pageSize;
    info.nMin = 0;              // range is nMax - nMin + 1
    info.nMax = range - 1;      //  as both nMax and nMax are inclusive
    info.nPos = pos;
    info.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
    if ( HasFlag(wxALWAYS_SHOW_SB) )
    {
        // disable scrollbar instead of removing it then
        info.fMask |= SIF_DISABLENOSCROLL;
    }

    HWND hWnd = GetHwnd();
    if ( hWnd )
    {
        // We have to set the variables here to make them valid in events
        // triggered by ::SetScrollInfo()
        *(orient == wxHORIZONTAL ? &m_xThumbSize : &m_yThumbSize) = pageSize;

        ::SetScrollInfo(hWnd, orient == wxHORIZONTAL ? SB_HORZ : SB_VERT,
                        &info, refresh);
    }
}

void wxWindowMSW::ScrollWindow(int dx, int dy, const wxRect *prect)
{
    RECT rect;
    RECT *pr;
    if ( prect )
    {
        rect.left = prect->x;
        rect.top = prect->y;
        rect.right = prect->x + prect->width;
        rect.bottom = prect->y + prect->height;
        pr = &rect;
    }
    else
    {
        pr = NULL;

    }

#ifdef __WXWINCE__
    // FIXME: is this the exact equivalent of the line below?
    ::ScrollWindowEx(GetHwnd(), dx, dy, pr, pr, 0, 0, SW_SCROLLCHILDREN|SW_ERASE|SW_INVALIDATE);
#else
    ::ScrollWindow(GetHwnd(), dx, dy, pr, pr);
#endif
}

static bool ScrollVertically(HWND hwnd, int kind, int count)
{
    int posStart = GetScrollPosition(hwnd, SB_VERT);

    int pos = posStart;
    for ( int n = 0; n < count; n++ )
    {
        ::SendMessage(hwnd, WM_VSCROLL, kind, 0);

        int posNew = GetScrollPosition(hwnd, SB_VERT);
        if ( posNew == pos )
        {
            // don't bother to continue, we're already at top/bottom
            break;
        }

        pos = posNew;
    }

    return pos != posStart;
}

bool wxWindowMSW::ScrollLines(int lines)
{
    bool down = lines > 0;

    return ScrollVertically(GetHwnd(),
                            down ? SB_LINEDOWN : SB_LINEUP,
                            down ? lines : -lines);
}

bool wxWindowMSW::ScrollPages(int pages)
{
    bool down = pages > 0;

    return ScrollVertically(GetHwnd(),
                            down ? SB_PAGEDOWN : SB_PAGEUP,
                            down ? pages : -pages);
}

// ----------------------------------------------------------------------------
// RTL support
// ----------------------------------------------------------------------------

void wxWindowMSW::SetLayoutDirection(wxLayoutDirection dir)
{
#ifdef __WXWINCE__
    wxUnusedVar(dir);
#else
    const HWND hwnd = GetHwnd();
    wxCHECK_RET( hwnd, _T("layout direction must be set after window creation") );

    LONG styleOld = ::GetWindowLong(hwnd, GWL_EXSTYLE);

    LONG styleNew = styleOld;
    switch ( dir )
    {
        case wxLayout_LeftToRight:
            styleNew &= ~WS_EX_LAYOUTRTL;
            break;

        case wxLayout_RightToLeft:
            styleNew |= WS_EX_LAYOUTRTL;
            break;

        default:
            wxFAIL_MSG(_T("unsupported layout direction"));
            break;
    }

    if ( styleNew != styleOld )
    {
        ::SetWindowLong(hwnd, GWL_EXSTYLE, styleNew);
    }
#endif
}

wxLayoutDirection wxWindowMSW::GetLayoutDirection() const
{
#ifdef __WXWINCE__
    return wxLayout_Default;
#else
    const HWND hwnd = GetHwnd();
    wxCHECK_MSG( hwnd, wxLayout_Default, _T("invalid window") );

    return ::GetWindowLong(hwnd, GWL_EXSTYLE) & WS_EX_LAYOUTRTL
                ? wxLayout_RightToLeft
                : wxLayout_LeftToRight;
#endif
}

wxCoord
wxWindowMSW::AdjustForLayoutDirection(wxCoord x,
                                      wxCoord WXUNUSED(width),
                                      wxCoord WXUNUSED(widthTotal)) const
{
    // Win32 mirrors the coordinates of RTL windows automatically, so don't
    // redo it ourselves
    return x;
}

// ---------------------------------------------------------------------------
// subclassing
// ---------------------------------------------------------------------------

void wxWindowMSW::SubclassWin(WXHWND hWnd)
{
    wxASSERT_MSG( !m_oldWndProc, wxT("subclassing window twice?") );

    HWND hwnd = (HWND)hWnd;
    wxCHECK_RET( ::IsWindow(hwnd), wxT("invalid HWND in SubclassWin") );

    wxAssociateWinWithHandle(hwnd, this);

    m_oldWndProc = (WXFARPROC)wxGetWindowProc((HWND)hWnd);

    // we don't need to subclass the window of our own class (in the Windows
    // sense of the word)
    if ( !wxCheckWindowWndProc(hWnd, (WXFARPROC)wxWndProc) )
    {
        wxSetWindowProc(hwnd, wxWndProc);
    }
    else
    {
        // don't bother restoring it either: this also makes it easy to
        // implement IsOfStandardClass() method which returns true for the
        // standard controls and false for the wxWidgets own windows as it can
        // simply check m_oldWndProc
        m_oldWndProc = NULL;
    }

    // we're officially created now, send the event
    wxWindowCreateEvent event((wxWindow *)this);
    (void)GetEventHandler()->ProcessEvent(event);
}

void wxWindowMSW::UnsubclassWin()
{
    wxRemoveHandleAssociation(this);

    // Restore old Window proc
    HWND hwnd = GetHwnd();
    if ( hwnd )
    {
        SetHWND(0);

        wxCHECK_RET( ::IsWindow(hwnd), wxT("invalid HWND in UnsubclassWin") );

        if ( m_oldWndProc )
        {
            if ( !wxCheckWindowWndProc((WXHWND)hwnd, m_oldWndProc) )
            {
                wxSetWindowProc(hwnd, (WNDPROC)m_oldWndProc);
            }

            m_oldWndProc = NULL;
        }
    }
}

void wxWindowMSW::AssociateHandle(WXWidget handle)
{
    if ( m_hWnd )
    {
      if ( !::DestroyWindow(GetHwnd()) )
        wxLogLastError(wxT("DestroyWindow"));
    }

    WXHWND wxhwnd = (WXHWND)handle;

    SetHWND(wxhwnd);
    SubclassWin(wxhwnd);
}

void wxWindowMSW::DissociateHandle()
{
    // this also calls SetHWND(0) for us
    UnsubclassWin();
}


bool wxCheckWindowWndProc(WXHWND hWnd,
                          WXFARPROC WXUNUSED(wndProc))
{
// TODO: This list of window class names should be factored out so they can be
// managed in one place and then accessed from here and other places, such as
// wxApp::RegisterWindowClasses() and wxApp::UnregisterWindowClasses()

#ifdef __WXWINCE__
    extern       wxChar *wxCanvasClassName;
    extern       wxChar *wxCanvasClassNameNR;
#else
    extern const wxChar *wxCanvasClassName;
    extern const wxChar *wxCanvasClassNameNR;
#endif
    extern const wxChar *wxMDIFrameClassName;
    extern const wxChar *wxMDIFrameClassNameNoRedraw;
    extern const wxChar *wxMDIChildFrameClassName;
    extern const wxChar *wxMDIChildFrameClassNameNoRedraw;
    wxString str(wxGetWindowClass(hWnd));
    if (str == wxCanvasClassName ||
        str == wxCanvasClassNameNR ||
#if wxUSE_GLCANVAS
        str == _T("wxGLCanvasClass") ||
        str == _T("wxGLCanvasClassNR") ||
#endif // wxUSE_GLCANVAS
        str == wxMDIFrameClassName ||
        str == wxMDIFrameClassNameNoRedraw ||
        str == wxMDIChildFrameClassName ||
        str == wxMDIChildFrameClassNameNoRedraw ||
        str == _T("wxTLWHiddenParent"))
        return true; // Effectively means don't subclass
    else
        return false;
}

// ----------------------------------------------------------------------------
// Style handling
// ----------------------------------------------------------------------------

void wxWindowMSW::SetWindowStyleFlag(long flags)
{
    long flagsOld = GetWindowStyleFlag();
    if ( flags == flagsOld )
        return;

    // update the internal variable
    wxWindowBase::SetWindowStyleFlag(flags);

    // and the real window flags
    MSWUpdateStyle(flagsOld, GetExtraStyle());
}

void wxWindowMSW::SetExtraStyle(long exflags)
{
    long exflagsOld = GetExtraStyle();
    if ( exflags == exflagsOld )
        return;

    // update the internal variable
    wxWindowBase::SetExtraStyle(exflags);

    // and the real window flags
    MSWUpdateStyle(GetWindowStyleFlag(), exflagsOld);
}

void wxWindowMSW::MSWUpdateStyle(long flagsOld, long exflagsOld)
{
    // now update the Windows style as well if needed - and if the window had
    // been already created
    if ( !GetHwnd() )
        return;

    // we may need to call SetWindowPos() when we change some styles
    bool callSWP = false;

    WXDWORD exstyle;
    long style = MSWGetStyle(GetWindowStyleFlag(), &exstyle);

    // this is quite a horrible hack but we need it because MSWGetStyle()
    // doesn't take exflags as parameter but uses GetExtraStyle() internally
    // and so we have to modify the window exflags temporarily to get the
    // correct exstyleOld
    long exflagsNew = GetExtraStyle();
    wxWindowBase::SetExtraStyle(exflagsOld);

    WXDWORD exstyleOld;
    long styleOld = MSWGetStyle(flagsOld, &exstyleOld);

    wxWindowBase::SetExtraStyle(exflagsNew);


    if ( style != styleOld )
    {
        // some flags (e.g. WS_VISIBLE or WS_DISABLED) should not be changed by
        // this function so instead of simply setting the style to the new
        // value we clear the bits which were set in styleOld but are set in
        // the new one and set the ones which were not set before
        long styleReal = ::GetWindowLong(GetHwnd(), GWL_STYLE);
        styleReal &= ~styleOld;
        styleReal |= style;

        ::SetWindowLong(GetHwnd(), GWL_STYLE, styleReal);

        // we need to call SetWindowPos() if any of the styles affecting the
        // frame appearance have changed
        callSWP = ((styleOld ^ style ) & (WS_BORDER |
                                      WS_THICKFRAME |
                                      WS_CAPTION |
                                      WS_DLGFRAME |
                                      WS_MAXIMIZEBOX |
                                      WS_MINIMIZEBOX |
                                      WS_SYSMENU) ) != 0;
    }

    // and the extended style
    long exstyleReal = ::GetWindowLong(GetHwnd(), GWL_EXSTYLE);

    if ( exstyle != exstyleOld )
    {
        exstyleReal &= ~exstyleOld;
        exstyleReal |= exstyle;

        ::SetWindowLong(GetHwnd(), GWL_EXSTYLE, exstyleReal);

        // ex style changes don't take effect without calling SetWindowPos
        callSWP = true;
    }

    if ( callSWP )
    {
        // we must call SetWindowPos() to flush the cached extended style and
        // also to make the change to wxSTAY_ON_TOP style take effect: just
        // setting the style simply doesn't work
        if ( !::SetWindowPos(GetHwnd(),
                             exstyleReal & WS_EX_TOPMOST ? HWND_TOPMOST
                                                         : HWND_NOTOPMOST,
                             0, 0, 0, 0,
                             SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED) )
        {
            wxLogLastError(_T("SetWindowPos"));
        }
    }
}

WXDWORD wxWindowMSW::MSWGetStyle(long flags, WXDWORD *exstyle) const
{
    // translate common wxWidgets styles to Windows ones

    // most of windows are child ones, those which are not (such as
    // wxTopLevelWindow) should remove WS_CHILD in their MSWGetStyle()
    WXDWORD style = WS_CHILD;

    // using this flag results in very significant reduction in flicker,
    // especially with controls inside the static boxes (as the interior of the
    // box is not redrawn twice), but sometimes results in redraw problems, so
    // optionally allow the old code to continue to use it provided a special
    // system option is turned on
    if ( !wxSystemOptions::GetOptionInt(wxT("msw.window.no-clip-children"))
            || (flags & wxCLIP_CHILDREN) )
        style |= WS_CLIPCHILDREN;

    // it doesn't seem useful to use WS_CLIPSIBLINGS here as we officially
    // don't support overlapping windows and it only makes sense for them and,
    // presumably, gives the system some extra work (to manage more clipping
    // regions), so avoid it alltogether


    if ( flags & wxVSCROLL )
        style |= WS_VSCROLL;

    if ( flags & wxHSCROLL )
        style |= WS_HSCROLL;

    const wxBorder border = TranslateBorder(GetBorder(flags));

    // WS_BORDER is only required for wxBORDER_SIMPLE
    if ( border == wxBORDER_SIMPLE )
        style |= WS_BORDER;

    // now deal with ext style if the caller wants it
    if ( exstyle )
    {
        *exstyle = 0;

#ifndef __WXWINCE__
        if ( flags & wxTRANSPARENT_WINDOW )
            *exstyle |= WS_EX_TRANSPARENT;
#endif

        switch ( border )
        {
            default:
            case wxBORDER_DEFAULT:
                wxFAIL_MSG( _T("unknown border style") );
                // fall through

            case wxBORDER_NONE:
            case wxBORDER_SIMPLE:
            case wxBORDER_THEME:
                break;

            case wxBORDER_STATIC:
                *exstyle |= WS_EX_STATICEDGE;
                break;

            case wxBORDER_RAISED:
                *exstyle |= WS_EX_DLGMODALFRAME;
                break;

            case wxBORDER_SUNKEN:
                *exstyle |= WS_EX_CLIENTEDGE;
                style &= ~WS_BORDER;
                break;

//            case wxBORDER_DOUBLE:
//                *exstyle |= WS_EX_DLGMODALFRAME;
//                break;
        }

        // wxUniv doesn't use Windows dialog navigation functions at all
#if !defined(__WXUNIVERSAL__) && !defined(__WXWINCE__)
        // to make the dialog navigation work with the nested panels we must
        // use this style (top level windows such as dialogs don't need it)
        if ( (flags & wxTAB_TRAVERSAL) && !IsTopLevel() )
        {
            *exstyle |= WS_EX_CONTROLPARENT;
        }
#endif // __WXUNIVERSAL__
    }

    return style;
}

// Helper for getting an appropriate theme style for the application. Unnecessary in
// 2.9 and above.
wxBorder wxWindowMSW::GetThemedBorderStyle() const
{
    return TranslateBorder(wxBORDER_THEME);
}

// Setup background and foreground colours correctly
void wxWindowMSW::SetupColours()
{
    if ( GetParent() )
        SetBackgroundColour(GetParent()->GetBackgroundColour());
}

bool wxWindowMSW::IsMouseInWindow() const
{
    // get the mouse position
    POINT pt;
#ifdef __WXWINCE__
    ::GetCursorPosWinCE(&pt);
#else
    ::GetCursorPos(&pt);
#endif

    // find the window which currently has the cursor and go up the window
    // chain until we find this window - or exhaust it
    HWND hwnd = ::WindowFromPoint(pt);
    while ( hwnd && (hwnd != GetHwnd()) )
        hwnd = ::GetParent(hwnd);

    return hwnd != NULL;
}

void wxWindowMSW::OnInternalIdle()
{
#ifndef HAVE_TRACKMOUSEEVENT
    // Check if we need to send a LEAVE event
    if ( m_mouseInWindow )
    {
        // note that we should generate the leave event whether the window has
        // or doesn't have mouse capture
        if ( !IsMouseInWindow() )
        {
            GenerateMouseLeave();
        }
    }
#endif // !HAVE_TRACKMOUSEEVENT

    if (wxUpdateUIEvent::CanUpdate(this) && IsShownOnScreen())
        UpdateWindowUI(wxUPDATE_UI_FROMIDLE);
}

// Set this window to be the child of 'parent'.
bool wxWindowMSW::Reparent(wxWindowBase *parent)
{
    if ( !wxWindowBase::Reparent(parent) )
        return false;

    HWND hWndChild = GetHwnd();
    HWND hWndParent = GetParent() ? GetWinHwnd(GetParent()) : (HWND)0;

    ::SetParent(hWndChild, hWndParent);

#ifndef __WXWINCE__
    if ( ::GetWindowLong(hWndChild, GWL_EXSTYLE) & WS_EX_CONTROLPARENT )
    {
        EnsureParentHasControlParentStyle(GetParent());
    }
#endif // !__WXWINCE__

    return true;
}

static inline void SendSetRedraw(HWND hwnd, bool on)
{
#ifndef __WXMICROWIN__
    ::SendMessage(hwnd, WM_SETREDRAW, (WPARAM)on, 0);
#endif
}

void wxWindowMSW::Freeze()
{
    if ( !m_frozenness++ )
    {
        if ( IsShown() )
        {
            if ( IsTopLevel() )
            {
                // If this is a TLW, then freeze it's non-TLW children
                // instead.  This is needed because on Windows a frozen TLW
                // lets window paint and mouse events pass through to other
                // Windows below this one in z-order.
                for ( wxWindowList::compatibility_iterator node = GetChildren().GetFirst();
                      node;
                      node = node->GetNext() )
                {
                    wxWindow *child = node->GetData();
                    if ( child->IsTopLevel() )
                        continue;
                    else
                        child->Freeze();
                }
            }
            else // This is not a TLW, so just freeze it.
            {
                SendSetRedraw(GetHwnd(), false);
            }
        }
    }
}

void wxWindowMSW::Thaw()
{
    wxASSERT_MSG( m_frozenness > 0, _T("Thaw() without matching Freeze()") );

    if ( --m_frozenness == 0 )
    {
        if ( IsShown() )
        {
            if ( IsTopLevel() )
            {
                // If this is a TLW, then Thaw it's non-TLW children
                // instead.  See Freeze.
                for ( wxWindowList::compatibility_iterator node = GetChildren().GetFirst();
                      node;
                      node = node->GetNext() )
                {
                    wxWindow *child = node->GetData();
                    if ( child->IsTopLevel() )
                        continue;
                    else
                    {
                        // in case the child was added while the TLW was
                        // frozen, it won't be frozen now so avoid the Thaw.
                        if ( child->IsFrozen() )
                            child->Thaw();
                    }
                }
            }
            else // This is not a TLW, so just thaw it.
            {
                SendSetRedraw(GetHwnd(), true);
            }

            // we need to refresh everything or otherwise the invalidated area
            // is not going to be repainted
            Refresh();
        }
    }
}

void wxWindowMSW::Refresh(bool eraseBack, const wxRect *rect)
{
    HWND hWnd = GetHwnd();
    if ( hWnd )
    {
        RECT mswRect;
        const RECT *pRect;
        if ( rect )
        {
            mswRect.left = rect->x;
            mswRect.top = rect->y;
            mswRect.right = rect->x + rect->width;
            mswRect.bottom = rect->y + rect->height;

            pRect = &mswRect;
        }
        else
        {
            pRect = NULL;
        }

        // RedrawWindow not available on SmartPhone or eVC++ 3
#if !defined(__SMARTPHONE__) && !(defined(_WIN32_WCE) && _WIN32_WCE < 400)
        UINT flags = RDW_INVALIDATE | RDW_ALLCHILDREN;
        if ( eraseBack )
            flags |= RDW_ERASE;

        ::RedrawWindow(hWnd, pRect, NULL, flags);
#else
        ::InvalidateRect(hWnd, pRect, eraseBack);
#endif
    }
}

void wxWindowMSW::Update()
{
    if ( !::UpdateWindow(GetHwnd()) )
    {
        wxLogLastError(_T("UpdateWindow"));
    }

#if !defined(__WXMICROWIN__) && !defined(__WXWINCE__)
    // just calling UpdateWindow() is not enough, what we did in our WM_PAINT
    // handler needs to be really drawn right now
    (void)::GdiFlush();
#endif // __WIN32__
}

// ---------------------------------------------------------------------------
// drag and drop
// ---------------------------------------------------------------------------

#if wxUSE_DRAG_AND_DROP || !defined(__WXWINCE__)

#if wxUSE_STATBOX

// we need to lower the sibling static boxes so controls contained within can be
// a drop target
static void AdjustStaticBoxZOrder(wxWindow *parent)
{
    // no sibling static boxes if we have no parent (ie TLW)
    if ( !parent )
        return;

    for ( wxWindowList::compatibility_iterator node = parent->GetChildren().GetFirst();
          node;
          node = node->GetNext() )
    {
        wxStaticBox *statbox = wxDynamicCast(node->GetData(), wxStaticBox);
        if ( statbox )
        {
            ::SetWindowPos(GetHwndOf(statbox), HWND_BOTTOM, 0, 0, 0, 0,
                           SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
        }
    }
}

#else // !wxUSE_STATBOX

static inline void AdjustStaticBoxZOrder(wxWindow * WXUNUSED(parent))
{
}

#endif // wxUSE_STATBOX/!wxUSE_STATBOX

#endif // drag and drop is used

#if wxUSE_DRAG_AND_DROP
void wxWindowMSW::SetDropTarget(wxDropTarget *pDropTarget)
{
    if ( m_dropTarget != 0 ) {
        m_dropTarget->Revoke(m_hWnd);
        delete m_dropTarget;
    }

    m_dropTarget = pDropTarget;
    if ( m_dropTarget != 0 )
    {
        AdjustStaticBoxZOrder(GetParent());
        m_dropTarget->Register(m_hWnd);
    }
}
#endif // wxUSE_DRAG_AND_DROP

// old-style file manager drag&drop support: we retain the old-style
// DragAcceptFiles in parallel with SetDropTarget.
void wxWindowMSW::DragAcceptFiles(bool WXUNUSED_IN_WINCE(accept))
{
#ifndef __WXWINCE__
    HWND hWnd = GetHwnd();
    if ( hWnd )
    {
        AdjustStaticBoxZOrder(GetParent());
        ::DragAcceptFiles(hWnd, (BOOL)accept);
    }
#endif
}

// ----------------------------------------------------------------------------
// tooltips
// ----------------------------------------------------------------------------

#if wxUSE_TOOLTIPS

void wxWindowMSW::DoSetToolTip(wxToolTip *tooltip)
{
    wxWindowBase::DoSetToolTip(tooltip);

    if ( m_tooltip )
        m_tooltip->SetWindow((wxWindow *)this);
}

#endif // wxUSE_TOOLTIPS

// ---------------------------------------------------------------------------
// moving and resizing
// ---------------------------------------------------------------------------

bool wxWindowMSW::IsSizeDeferred() const
{
#if USE_DEFERRED_SIZING
    if ( m_pendingPosition != wxDefaultPosition ||
         m_pendingSize     != wxDefaultSize )
        return true;
#endif // USE_DEFERRED_SIZING

    return false;
}

// Get total size
void wxWindowMSW::DoGetSize(int *x, int *y) const
{
#if USE_DEFERRED_SIZING
    // if SetSize() had been called at wx level but not realized at Windows
    // level yet (i.e. EndDeferWindowPos() not called), we still should return
    // the new and not the old position to the other wx code
    if ( m_pendingSize != wxDefaultSize )
    {
        if ( x )
            *x = m_pendingSize.x;
        if ( y )
            *y = m_pendingSize.y;
    }
    else // use current size
#endif // USE_DEFERRED_SIZING
    {
        RECT rect = wxGetWindowRect(GetHwnd());

        if ( x )
            *x = rect.right - rect.left;
        if ( y )
            *y = rect.bottom - rect.top;
    }
}

// Get size *available for subwindows* i.e. excluding menu bar etc.
void wxWindowMSW::DoGetClientSize(int *x, int *y) const
{
#if USE_DEFERRED_SIZING
    if ( m_pendingSize != wxDefaultSize )
    {
        // we need to calculate the client size corresponding to pending size
        RECT rect;
        rect.left = m_pendingPosition.x;
        rect.top = m_pendingPosition.y;
        rect.right = rect.left + m_pendingSize.x;
        rect.bottom = rect.top + m_pendingSize.y;

        ::SendMessage(GetHwnd(), WM_NCCALCSIZE, FALSE, (LPARAM)&rect);

        if ( x )
            *x = rect.right - rect.left;
        if ( y )
            *y = rect.bottom - rect.top;
    }
    else
#endif // USE_DEFERRED_SIZING
    {
        RECT rect = wxGetClientRect(GetHwnd());

        if ( x )
            *x = rect.right;
        if ( y )
            *y = rect.bottom;
    }
}

void wxWindowMSW::DoGetPosition(int *x, int *y) const
{
    wxWindow * const parent = GetParent();

    wxPoint pos;
    if ( m_pendingPosition != wxDefaultPosition )
    {
        pos = m_pendingPosition;
    }
    else // use current position
    {
        RECT rect = wxGetWindowRect(GetHwnd());

        POINT point;
        point.x = rect.left;
        point.y = rect.top;

        // we do the adjustments with respect to the parent only for the "real"
        // children, not for the dialogs/frames
        if ( !IsTopLevel() )
        {
            if ( wxTheApp->GetLayoutDirection() == wxLayout_RightToLeft )
            {
                // In RTL mode, we want the logical left x-coordinate,
                // which would be the physical right x-coordinate.
                point.x = rect.right;
            }

            // Since we now have the absolute screen coords, if there's a
            // parent we must subtract its top left corner
            if ( parent )
            {
                ::ScreenToClient(GetHwndOf(parent), &point);
            }
        }

        pos.x = point.x;
        pos.y = point.y;
    }

    // we also must adjust by the client area offset: a control which is just
    // under a toolbar could be at (0, 30) in Windows but at (0, 0) in wx
    if ( parent && !IsTopLevel() )
    {
        const wxPoint pt(parent->GetClientAreaOrigin());
        pos.x -= pt.x;
        pos.y -= pt.y;
    }

    if ( x )
        *x = pos.x;
    if ( y )
        *y = pos.y;
}

void wxWindowMSW::DoScreenToClient(int *x, int *y) const
{
    POINT pt;
    if ( x )
        pt.x = *x;
    if ( y )
        pt.y = *y;

    ::ScreenToClient(GetHwnd(), &pt);

    if ( x )
        *x = pt.x;
    if ( y )
        *y = pt.y;
}

void wxWindowMSW::DoClientToScreen(int *x, int *y) const
{
    POINT pt;
    if ( x )
        pt.x = *x;
    if ( y )
        pt.y = *y;

    ::ClientToScreen(GetHwnd(), &pt);

    if ( x )
        *x = pt.x;
    if ( y )
        *y = pt.y;
}

bool
wxWindowMSW::DoMoveSibling(WXHWND hwnd, int x, int y, int width, int height)
{
#if USE_DEFERRED_SIZING
    // if our parent had prepared a defer window handle for us, use it (unless
    // we are a top level window)
    wxWindowMSW * const parent = IsTopLevel() ? NULL : GetParent();

    HDWP hdwp = parent ? (HDWP)parent->m_hDWP : NULL;
    if ( hdwp )
    {
        hdwp = ::DeferWindowPos(hdwp, (HWND)hwnd, NULL, x, y, width, height,
                                SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_NOACTIVATE);
        if ( !hdwp )
        {
            wxLogLastError(_T("DeferWindowPos"));
        }
    }

    if ( parent )
    {
        // hdwp must be updated as it may have been changed
        parent->m_hDWP = (WXHANDLE)hdwp;
    }

    if ( hdwp )
    {
        // did deferred move, remember new coordinates of the window as they're
        // different from what Windows would return for it
        return true;
    }

    // otherwise (or if deferring failed) move the window in place immediately
#endif // USE_DEFERRED_SIZING
    if ( !::MoveWindow((HWND)hwnd, x, y, width, height, IsShown()) )
    {
        wxLogLastError(wxT("MoveWindow"));
    }

    // if USE_DEFERRED_SIZING, indicates that we didn't use deferred move,
    // ignored otherwise
    return false;
}

void wxWindowMSW::DoMoveWindow(int x, int y, int width, int height)
{
    // TODO: is this consistent with other platforms?
    // Still, negative width or height shouldn't be allowed
    if (width < 0)
        width = 0;
    if (height < 0)
        height = 0;

    if ( DoMoveSibling(m_hWnd, x, y, width, height) )
    {
#if USE_DEFERRED_SIZING
        m_pendingPosition = wxPoint(x, y);
        m_pendingSize = wxSize(width, height);
    }
    else // window was moved immediately, without deferring it
    {
        m_pendingPosition = wxDefaultPosition;
        m_pendingSize = wxDefaultSize;
#endif // USE_DEFERRED_SIZING
    }
}

// set the size of the window: if the dimensions are positive, just use them,
// but if any of them is equal to -1, it means that we must find the value for
// it ourselves (unless sizeFlags contains wxSIZE_ALLOW_MINUS_ONE flag, in
// which case -1 is a valid value for x and y)
//
// If sizeFlags contains wxSIZE_AUTO_WIDTH/HEIGHT flags (default), we calculate
// the width/height to best suit our contents, otherwise we reuse the current
// width/height
void wxWindowMSW::DoSetSize(int x, int y, int width, int height, int sizeFlags)
{
    // get the current size and position...
    int currentX, currentY;
    int currentW, currentH;

    GetPosition(&currentX, &currentY);
    GetSize(&currentW, &currentH);

    // ... and don't do anything (avoiding flicker) if it's already ok unless
    // we're forced to resize the window
    if ( x == currentX && y == currentY &&
         width == currentW && height == currentH &&
            !(sizeFlags & wxSIZE_FORCE) )
    {
        return;
    }

    if ( x == wxDefaultCoord && !(sizeFlags & wxSIZE_ALLOW_MINUS_ONE) )
        x = currentX;
    if ( y == wxDefaultCoord && !(sizeFlags & wxSIZE_ALLOW_MINUS_ONE) )
        y = currentY;

    AdjustForParentClientOrigin(x, y, sizeFlags);

    wxSize size = wxDefaultSize;
    if ( width == wxDefaultCoord )
    {
        if ( sizeFlags & wxSIZE_AUTO_WIDTH )
        {
            size = DoGetBestSize();
            width = size.x;
        }
        else
        {
            // just take the current one
            width = currentW;
        }
    }

    if ( height == wxDefaultCoord )
    {
        if ( sizeFlags & wxSIZE_AUTO_HEIGHT )
        {
            if ( size.x == wxDefaultCoord )
            {
                size = DoGetBestSize();
            }
            //else: already called DoGetBestSize() above

            height = size.y;
        }
        else
        {
            // just take the current one
            height = currentH;
        }
    }

    DoMoveWindow(x, y, width, height);
}

void wxWindowMSW::DoSetClientSize(int width, int height)
{
    // setting the client size is less obvious than it could have been
    // because in the result of changing the total size the window scrollbar
    // may [dis]appear and/or its menubar may [un]wrap (and AdjustWindowRect()
    // doesn't take neither into account) and so the client size will not be
    // correct as the difference between the total and client size changes --
    // so we keep changing it until we get it right
    //
    // normally this loop shouldn't take more than 3 iterations (usually 1 but
    // if scrollbars [dis]appear as the result of the first call, then 2 and it
    // may become 3 if the window had 0 size originally and so we didn't
    // calculate the scrollbar correction correctly during the first iteration)
    // but just to be on the safe side we check for it instead of making it an
    // "infinite" loop (i.e. leaving break inside as the only way to get out)
    for ( int i = 0; i < 4; i++ )
    {
        RECT rectClient;
        ::GetClientRect(GetHwnd(), &rectClient);

        // if the size is already ok, stop here (NB: rectClient.left = top = 0)
        if ( (rectClient.right == width || width == wxDefaultCoord) &&
             (rectClient.bottom == height || height == wxDefaultCoord) )
        {
            break;
        }

        // Find the difference between the entire window (title bar and all)
        // and the client area; add this to the new client size to move the
        // window
        RECT rectWin;
        ::GetWindowRect(GetHwnd(), &rectWin);

        const int widthWin = rectWin.right - rectWin.left,
                  heightWin = rectWin.bottom - rectWin.top;

        // MoveWindow positions the child windows relative to the parent, so
        // adjust if necessary
        if ( !IsTopLevel() )
        {
            wxWindow *parent = GetParent();
            if ( parent )
            {
                ::ScreenToClient(GetHwndOf(parent), (POINT *)&rectWin);
            }
        }

        // don't call DoMoveWindow() because we want to move window immediately
        // and not defer it here as otherwise the value returned by
        // GetClient/WindowRect() wouldn't change as the window wouldn't be
        // really resized
        if ( !::MoveWindow(GetHwnd(),
                           rectWin.left,
                           rectWin.top,
                           width + widthWin - rectClient.right,
                           height + heightWin - rectClient.bottom,
                           TRUE) )
        {
            wxLogLastError(_T("MoveWindow"));
        }
    }
}

// ---------------------------------------------------------------------------
// text metrics
// ---------------------------------------------------------------------------

int wxWindowMSW::GetCharHeight() const
{
    return wxGetTextMetrics(this).tmHeight;
}

int wxWindowMSW::GetCharWidth() const
{
    // +1 is needed because Windows apparently adds it when calculating the
    // dialog units size in pixels
#if wxDIALOG_UNIT_COMPATIBILITY
    return wxGetTextMetrics(this).tmAveCharWidth;
#else
    return wxGetTextMetrics(this).tmAveCharWidth + 1;
#endif
}

void wxWindowMSW::GetTextExtent(const wxString& string,
                             int *x, int *y,
                             int *descent, int *externalLeading,
                             const wxFont *theFont) const
{
    wxASSERT_MSG( !theFont || theFont->Ok(),
                    _T("invalid font in GetTextExtent()") );

    wxFont fontToUse;
    if (theFont)
        fontToUse = *theFont;
    else
        fontToUse = GetFont();

    WindowHDC hdc(GetHwnd());
    SelectInHDC selectFont(hdc, GetHfontOf(fontToUse));

    SIZE sizeRect;
    TEXTMETRIC tm;
    ::GetTextExtentPoint32(hdc, string, string.length(), &sizeRect);
    GetTextMetrics(hdc, &tm);

    if ( x )
        *x = sizeRect.cx;
    if ( y )
        *y = sizeRect.cy;
    if ( descent )
        *descent = tm.tmDescent;
    if ( externalLeading )
        *externalLeading = tm.tmExternalLeading;
}

// ---------------------------------------------------------------------------
// popup menu
// ---------------------------------------------------------------------------

#if wxUSE_MENUS_NATIVE

// yield for WM_COMMAND events only, i.e. process all WM_COMMANDs in the queue
// immediately, without waiting for the next event loop iteration
//
// NB: this function should probably be made public later as it can almost
//     surely replace wxYield() elsewhere as well
static void wxYieldForCommandsOnly()
{
    // peek all WM_COMMANDs (it will always return WM_QUIT too but we don't
    // want to process it here)
    MSG msg;
    while ( ::PeekMessage(&msg, (HWND)0, WM_COMMAND, WM_COMMAND, PM_REMOVE) )
    {
        if ( msg.message == WM_QUIT )
        {
            // if we retrieved a WM_QUIT, insert back into the message queue.
            ::PostQuitMessage(0);
            break;
        }

        // luckily (as we don't have access to wxEventLoopImpl method from here
        // anyhow...) we don't need to pre process WM_COMMANDs so dispatch it
        // immediately
        ::TranslateMessage(&msg);
        ::DispatchMessage(&msg);
    }
}

bool wxWindowMSW::DoPopupMenu(wxMenu *menu, int x, int y)
{
    menu->SetInvokingWindow(this);
    menu->UpdateUI();

    if ( x == wxDefaultCoord && y == wxDefaultCoord )
    {
        wxPoint mouse = ScreenToClient(wxGetMousePosition());
        x = mouse.x; y = mouse.y;
    }

    HWND hWnd = GetHwnd();
    HMENU hMenu = GetHmenuOf(menu);
    POINT point;
    point.x = x;
    point.y = y;
    ::ClientToScreen(hWnd, &point);
    wxCurrentPopupMenu = menu;
#if defined(__WXWINCE__)
    static const UINT flags = 0;
#else // !__WXWINCE__
    UINT flags = TPM_RIGHTBUTTON;
    // NT4 doesn't support TPM_RECURSE and simply doesn't show the menu at all
    // when it's use, I'm not sure about Win95/98 but prefer to err on the safe
    // side and not to use it there neither -- modify the test if it does work
    // on these systems
    if ( wxGetWinVersion() >= wxWinVersion_5 )
    {
        // using TPM_RECURSE allows us to show a popup menu while another menu
        // is opened which can be useful and is supported by the other
        // platforms, so allow it under Windows too
        flags |= TPM_RECURSE;
    }
#endif // __WXWINCE__/!__WXWINCE__

    ::TrackPopupMenu(hMenu, flags, point.x, point.y, 0, hWnd, NULL);

    // we need to do it right now as otherwise the events are never going to be
    // sent to wxCurrentPopupMenu from HandleCommand()
    //
    // note that even eliminating (ugly) wxCurrentPopupMenu global wouldn't
    // help and we'd still need wxYieldForCommandsOnly() as the menu may be
    // destroyed as soon as we return (it can be a local variable in the caller
    // for example) and so we do need to process the event immediately
    wxYieldForCommandsOnly();

    wxCurrentPopupMenu = NULL;

    menu->SetInvokingWindow(NULL);

    return true;
}

#endif // wxUSE_MENUS_NATIVE

// ===========================================================================
// pre/post message processing
// ===========================================================================

WXLRESULT wxWindowMSW::MSWDefWindowProc(WXUINT nMsg, WXWPARAM wParam, WXLPARAM lParam)
{
    if ( m_oldWndProc )
        return ::CallWindowProc(CASTWNDPROC m_oldWndProc, GetHwnd(), (UINT) nMsg, (WPARAM) wParam, (LPARAM) lParam);
    else
        return ::DefWindowProc(GetHwnd(), nMsg, wParam, lParam);
}

bool wxWindowMSW::MSWProcessMessage(WXMSG* pMsg)
{
    // wxUniversal implements tab traversal itself
#ifndef __WXUNIVERSAL__
    if ( m_hWnd != 0 && (GetWindowStyleFlag() & wxTAB_TRAVERSAL) )
    {
        // intercept dialog navigation keys
        MSG *msg = (MSG *)pMsg;

        // here we try to do all the job which ::IsDialogMessage() usually does
        // internally
        if ( msg->message == WM_KEYDOWN )
        {
            bool bCtrlDown = wxIsCtrlDown();
            bool bShiftDown = wxIsShiftDown();

            // WM_GETDLGCODE: ask the control if it wants the key for itself,
            // don't process it if it's the case (except for Ctrl-Tab/Enter
            // combinations which are always processed)
            LONG lDlgCode = ::SendMessage(msg->hwnd, WM_GETDLGCODE, 0, 0);

            // surprizingly, DLGC_WANTALLKEYS bit mask doesn't contain the
            // DLGC_WANTTAB nor DLGC_WANTARROWS bits although, logically,
            // it, of course, implies them
            if ( lDlgCode & DLGC_WANTALLKEYS )
            {
                lDlgCode |= DLGC_WANTTAB | DLGC_WANTARROWS;
            }

            bool bForward = true,
                 bWindowChange = false,
                 bFromTab = false;

            // should we process this message specially?
            bool bProcess = true;
            switch ( msg->wParam )
            {
                case VK_TAB:
                    if ( (lDlgCode & DLGC_WANTTAB) && !bCtrlDown )
                    {
                        // let the control have the TAB
                        bProcess = false;
                    }
                    else // use it for navigation
                    {
                        // Ctrl-Tab cycles thru notebook pages
                        bWindowChange = bCtrlDown;
                        bForward = !bShiftDown;
                        bFromTab = true;
                    }
                    break;

                case VK_UP:
                case VK_LEFT:
                    if ( (lDlgCode & DLGC_WANTARROWS) || bCtrlDown )
                        bProcess = false;
                    else
                        bForward = false;
                    break;

                case VK_DOWN:
                case VK_RIGHT:
                    if ( (lDlgCode & DLGC_WANTARROWS) || bCtrlDown )
                        bProcess = false;
                    break;

                case VK_PRIOR:
                    bForward = false;
                    // fall through

                case VK_NEXT:
                    // we treat PageUp/Dn as arrows because chances are that
                    // a control which needs arrows also needs them for
                    // navigation (e.g. wxTextCtrl, wxListCtrl, ...)
                    if ( (lDlgCode & DLGC_WANTARROWS) && !bCtrlDown )
                        bProcess = false;
                    else // OTOH Ctrl-PageUp/Dn works as [Shift-]Ctrl-Tab
                        bWindowChange = true;
                    break;

                case VK_RETURN:
                    {
                        if ( (lDlgCode & DLGC_WANTMESSAGE) && !bCtrlDown )
                        {
                            // control wants to process Enter itself, don't
                            // call IsDialogMessage() which would consume it
                            return false;
                        }

#if wxUSE_BUTTON
                        // currently active button should get enter press even
                        // if there is a default button elsewhere so check if
                        // this window is a button first
                        wxWindow *btn = NULL;
                        if ( lDlgCode & DLGC_DEFPUSHBUTTON )
                        {
                            // let IsDialogMessage() handle this for all
                            // buttons except the owner-drawn ones which it
                            // just seems to ignore
                            long style = ::GetWindowLong(msg->hwnd, GWL_STYLE);
                            if ( (style & BS_OWNERDRAW) == BS_OWNERDRAW )
                            {
                                // emulate the button click
                                btn = wxFindWinFromHandle((WXHWND)msg->hwnd);
                            }

                            bProcess = false;
                        }
                        else // not a button itself, do we have default button?
                        {
                            wxTopLevelWindow *
                                tlw = wxDynamicCast(wxGetTopLevelParent(this),
                                                    wxTopLevelWindow);
                            if ( tlw )
                            {
                                btn = wxDynamicCast(tlw->GetDefaultItem(),
                                                    wxButton);
                            }
                        }

                        if ( btn && btn->IsEnabled() )
                        {
                            btn->MSWCommand(BN_CLICKED, 0 /* unused */);
                            return true;
                        }

#endif // wxUSE_BUTTON

#ifdef __WXWINCE__
                        // map Enter presses into button presses on PDAs
                        wxJoystickEvent event(wxEVT_JOY_BUTTON_DOWN);
                        event.SetEventObject(this);
                        if ( GetEventHandler()->ProcessEvent(event) )
                            return true;
#endif // __WXWINCE__
                    }
                    break;

                default:
                    bProcess = false;
            }

            if ( bProcess )
            {
                wxNavigationKeyEvent event;
                event.SetDirection(bForward);
                event.SetWindowChange(bWindowChange);
                event.SetFromTab(bFromTab);
                event.SetEventObject(this);

                if ( GetEventHandler()->ProcessEvent(event) )
                {
                    // as we don't call IsDialogMessage(), which would take of
                    // this by default, we need to manually send this message
                    // so that controls can change their UI state if needed
                    MSWUpdateUIState(UIS_CLEAR, UISF_HIDEFOCUS);

                    return true;
                }
            }
        }

        if ( ::IsDialogMessage(GetHwnd(), msg) )
        {
            // IsDialogMessage() did something...
            return true;
        }
    }
#endif // __WXUNIVERSAL__

#if wxUSE_TOOLTIPS
    if ( m_tooltip )
    {
        // relay mouse move events to the tooltip control
        MSG *msg = (MSG *)pMsg;
        if ( msg->message == WM_MOUSEMOVE )
            wxToolTip::RelayEvent(pMsg);
    }
#endif // wxUSE_TOOLTIPS

    return false;
}

bool wxWindowMSW::MSWTranslateMessage(WXMSG* pMsg)
{
#if wxUSE_ACCEL && !defined(__WXUNIVERSAL__)
    return m_acceleratorTable.Translate(this, pMsg);
#else
    (void) pMsg;
    return false;
#endif // wxUSE_ACCEL
}

bool wxWindowMSW::MSWShouldPreProcessMessage(WXMSG* msg)
{
    // all tests below have to deal with various bugs/misfeatures of
    // IsDialogMessage(): we have to prevent it from being called from our
    // MSWProcessMessage() in some situations

    // don't let IsDialogMessage() get VK_ESCAPE as it _always_ eats the
    // message even when there is no cancel button and when the message is
    // needed by the control itself: in particular, it prevents the tree in
    // place edit control from being closed with Escape in a dialog
    if ( msg->message == WM_KEYDOWN && msg->wParam == VK_ESCAPE )
    {
        return false;
    }

    // ::IsDialogMessage() is broken and may sometimes hang the application by
    // going into an infinite loop when it tries to find the control to give
    // focus to when Alt-<key> is pressed, so we try to detect [some of] the
    // situations when this may happen and not call it then
    if ( msg->message != WM_SYSCHAR )
        return true;

    // assume we can call it by default
    bool canSafelyCallIsDlgMsg = true;

    HWND hwndFocus = ::GetFocus();

    // if the currently focused window itself has WS_EX_CONTROLPARENT style,
    // ::IsDialogMessage() will also enter an infinite loop, because it will
    // recursively check the child windows but not the window itself and so if
    // none of the children accepts focus it loops forever (as it only stops
    // when it gets back to the window it started from)
    //
    // while it is very unusual that a window with WS_EX_CONTROLPARENT
    // style has the focus, it can happen. One such possibility is if
    // all windows are either toplevel, wxDialog, wxPanel or static
    // controls and no window can actually accept keyboard input.
#if !defined(__WXWINCE__)
    if ( ::GetWindowLong(hwndFocus, GWL_EXSTYLE) & WS_EX_CONTROLPARENT )
    {
        // pessimistic by default
        canSafelyCallIsDlgMsg = false;
        for ( wxWindowList::compatibility_iterator node = GetChildren().GetFirst();
              node;
              node = node->GetNext() )
        {
            wxWindow * const win = node->GetData();
            if ( win->AcceptsFocus() &&
                    !(::GetWindowLong(GetHwndOf(win), GWL_EXSTYLE) &
                        WS_EX_CONTROLPARENT) )
            {
                // it shouldn't hang...
                canSafelyCallIsDlgMsg = true;

                break;
            }
        }
    }
#endif // !__WXWINCE__

    if ( canSafelyCallIsDlgMsg )
    {
        // ::IsDialogMessage() can enter in an infinite loop when the
        // currently focused window is disabled or hidden and its
        // parent has WS_EX_CONTROLPARENT style, so don't call it in
        // this case
        while ( hwndFocus )
        {
            if ( !::IsWindowEnabled(hwndFocus) ||
                    !::IsWindowVisible(hwndFocus) )
            {
                // it would enter an infinite loop if we do this!
                canSafelyCallIsDlgMsg = false;

                break;
            }

            if ( !(::GetWindowLong(hwndFocus, GWL_STYLE) & WS_CHILD) )
            {
                // it's a top level window, don't go further -- e.g. even
                // if the parent of a dialog is disabled, this doesn't
                // break navigation inside the dialog
                break;
            }

            hwndFocus = ::GetParent(hwndFocus);
        }
    }

    return canSafelyCallIsDlgMsg;
}

// ---------------------------------------------------------------------------
// message params unpackers
// ---------------------------------------------------------------------------

void wxWindowMSW::UnpackCommand(WXWPARAM wParam, WXLPARAM lParam,
                             WORD *id, WXHWND *hwnd, WORD *cmd)
{
    *id = LOWORD(wParam);
    *hwnd = (WXHWND)lParam;
    *cmd = HIWORD(wParam);
}

void wxWindowMSW::UnpackActivate(WXWPARAM wParam, WXLPARAM lParam,
                              WXWORD *state, WXWORD *minimized, WXHWND *hwnd)
{
    *state = LOWORD(wParam);
    *minimized = HIWORD(wParam);
    *hwnd = (WXHWND)lParam;
}

void wxWindowMSW::UnpackScroll(WXWPARAM wParam, WXLPARAM lParam,
                            WXWORD *code, WXWORD *pos, WXHWND *hwnd)
{
    *code = LOWORD(wParam);
    *pos = HIWORD(wParam);
    *hwnd = (WXHWND)lParam;
}

void wxWindowMSW::UnpackCtlColor(WXWPARAM wParam, WXLPARAM lParam,
                                 WXHDC *hdc, WXHWND *hwnd)
{
    *hwnd = (WXHWND)lParam;
    *hdc = (WXHDC)wParam;
}

void wxWindowMSW::UnpackMenuSelect(WXWPARAM wParam, WXLPARAM lParam,
                                WXWORD *item, WXWORD *flags, WXHMENU *hmenu)
{
    *item = (WXWORD)wParam;
    *flags = HIWORD(wParam);
    *hmenu = (WXHMENU)lParam;
}

// ---------------------------------------------------------------------------
// Main wxWidgets window proc and the window proc for wxWindow
// ---------------------------------------------------------------------------

// Hook for new window just as it's being created, when the window isn't yet
// associated with the handle
static wxWindowMSW *gs_winBeingCreated = NULL;

// implementation of wxWindowCreationHook class: it just sets gs_winBeingCreated to the
// window being created and insures that it's always unset back later
wxWindowCreationHook::wxWindowCreationHook(wxWindowMSW *winBeingCreated)
{
    gs_winBeingCreated = winBeingCreated;
}

wxWindowCreationHook::~wxWindowCreationHook()
{
    gs_winBeingCreated = NULL;
}

// Main window proc
LRESULT WXDLLEXPORT APIENTRY _EXPORT wxWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    // trace all messages - useful for the debugging
#ifdef __WXDEBUG__
    wxLogTrace(wxTraceMessages,
               wxT("Processing %s(hWnd=%08lx, wParam=%8lx, lParam=%8lx)"),
               wxGetMessageName(message), (long)hWnd, (long)wParam, lParam);
#endif // __WXDEBUG__

    wxWindowMSW *wnd = wxFindWinFromHandle((WXHWND) hWnd);

    // when we get the first message for the HWND we just created, we associate
    // it with wxWindow stored in gs_winBeingCreated
    if ( !wnd && gs_winBeingCreated )
    {
        wxAssociateWinWithHandle(hWnd, gs_winBeingCreated);
        wnd = gs_winBeingCreated;
        gs_winBeingCreated = NULL;
        wnd->SetHWND((WXHWND)hWnd);
    }

    LRESULT rc;

    if ( wnd && wxEventLoop::AllowProcessing(wnd) )
        rc = wnd->MSWWindowProc(message, wParam, lParam);
    else
        rc = ::DefWindowProc(hWnd, message, wParam, lParam);

    return rc;
}

WXLRESULT wxWindowMSW::MSWWindowProc(WXUINT message, WXWPARAM wParam, WXLPARAM lParam)
{
    // did we process the message?
    bool processed = false;

    // the return value
    union
    {
        bool        allow;
        WXLRESULT   result;
        WXHBRUSH    hBrush;
    } rc;

    // for most messages we should return 0 when we do process the message
    rc.result = 0;

    switch ( message )
    {
        case WM_CREATE:
            {
                bool mayCreate;
                processed = HandleCreate((WXLPCREATESTRUCT)lParam, &mayCreate);
                if ( processed )
                {
                    // return 0 to allow window creation
                    rc.result = mayCreate ? 0 : -1;
                }
            }
            break;

        case WM_DESTROY:
            // never set processed to true and *always* pass WM_DESTROY to
            // DefWindowProc() as Windows may do some internal cleanup when
            // processing it and failing to pass the message along may cause
            // memory and resource leaks!
            (void)HandleDestroy();
            break;

        case WM_SIZE:
            processed = HandleSize(LOWORD(lParam), HIWORD(lParam), wParam);
            break;

        case WM_MOVE:
            processed = HandleMove(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            break;

#if !defined(__WXWINCE__)
        case WM_MOVING:
            {
                LPRECT pRect = (LPRECT)lParam;
                wxRect rc;
                rc.SetLeft(pRect->left);
                rc.SetTop(pRect->top);
                rc.SetRight(pRect->right);
                rc.SetBottom(pRect->bottom);
                processed = HandleMoving(rc);
                if (processed) {
                    pRect->left = rc.GetLeft();
                    pRect->top = rc.GetTop();
                    pRect->right = rc.GetRight();
                    pRect->bottom = rc.GetBottom();
                }
            }
            break;

        case WM_SIZING:
            {
                LPRECT pRect = (LPRECT)lParam;
                wxRect rc;
                rc.SetLeft(pRect->left);
                rc.SetTop(pRect->top);
                rc.SetRight(pRect->right);
                rc.SetBottom(pRect->bottom);
                processed = HandleSizing(rc);
                if (processed) {
                    pRect->left = rc.GetLeft();
                    pRect->top = rc.GetTop();
                    pRect->right = rc.GetRight();
                    pRect->bottom = rc.GetBottom();
                }
            }
            break;
#endif // !__WXWINCE__

#if !defined(__WXMICROWIN__) && !defined(__WXWINCE__)
        case WM_ACTIVATEAPP:
            // This implicitly sends a wxEVT_ACTIVATE_APP event
            wxTheApp->SetActive(wParam != 0, FindFocus());
            break;
#endif

        case WM_ACTIVATE:
            {
                WXWORD state, minimized;
                WXHWND hwnd;
                UnpackActivate(wParam, lParam, &state, &minimized, &hwnd);

                processed = HandleActivate(state, minimized != 0, (WXHWND)hwnd);
            }
            break;

        case WM_SETFOCUS:
            processed = HandleSetFocus((WXHWND)(HWND)wParam);
            break;

        case WM_KILLFOCUS:
            processed = HandleKillFocus((WXHWND)(HWND)wParam);
            break;

        case WM_PRINTCLIENT:
            processed = HandlePrintClient((WXHDC)wParam);
            break;

        case WM_PAINT:
            if ( wParam )
            {
                wxPaintDCEx dc((wxWindow *)this, (WXHDC)wParam);

                processed = HandlePaint();
            }
            else // no DC given
            {
                processed = HandlePaint();
            }
            break;

        case WM_CLOSE:
#ifdef __WXUNIVERSAL__
            // Universal uses its own wxFrame/wxDialog, so we don't receive
            // close events unless we have this.
            Close();
#endif // __WXUNIVERSAL__

            // don't let the DefWindowProc() destroy our window - we'll do it
            // ourselves in ~wxWindow
            processed = true;
            rc.result = TRUE;
            break;

        case WM_SHOWWINDOW:
            processed = HandleShow(wParam != 0, (int)lParam);
            break;

        case WM_MOUSEMOVE:
            processed = HandleMouseMove(GET_X_LPARAM(lParam),
                                        GET_Y_LPARAM(lParam),
                                        wParam);
            break;

#ifdef HAVE_TRACKMOUSEEVENT
        case WM_MOUSELEAVE:
            // filter out excess WM_MOUSELEAVE events sent after PopupMenu()
            // (on XP at least)
            if ( m_mouseInWindow )
            {
                GenerateMouseLeave();
            }

            // always pass processed back as false, this allows the window
            // manager to process the message too.  This is needed to
            // ensure windows XP themes work properly as the mouse moves
            // over widgets like buttons. So don't set processed to true here.
            break;
#endif // HAVE_TRACKMOUSEEVENT

#if wxUSE_MOUSEWHEEL
        case WM_MOUSEWHEEL:
            processed = HandleMouseWheel(wParam, lParam);
            break;
#endif

        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_LBUTTONDBLCLK:
        case WM_RBUTTONDOWN:
        case WM_RBUTTONUP:
        case WM_RBUTTONDBLCLK:
        case WM_MBUTTONDOWN:
        case WM_MBUTTONUP:
        case WM_MBUTTONDBLCLK:
            {
#ifdef __WXMICROWIN__
                // MicroWindows seems to ignore the fact that a window is
                // disabled. So catch mouse events and throw them away if
                // necessary.
                wxWindowMSW* win = this;
                for ( ;; )
                {
                    if (!win->IsEnabled())
                    {
                        processed = true;
                        break;
                    }

                    win = win->GetParent();
                    if ( !win || win->IsTopLevel() )
                        break;
                }

                if ( processed )
                    break;

#endif // __WXMICROWIN__
                int x = GET_X_LPARAM(lParam),
                    y = GET_Y_LPARAM(lParam);

#ifdef __WXWINCE__
                // redirect the event to a static control if necessary by
                // finding one under mouse because under CE the static controls
                // don't generate mouse events (even with SS_NOTIFY)
                wxWindowMSW *win;
                if ( GetCapture() == this )
                {
                    // but don't do it if the mouse is captured by this window
                    // because then it should really get this event itself
                    win = this;
                }
                else
                {
                    win = FindWindowForMouseEvent(this, &x, &y);

                    // this should never happen
                    wxCHECK_MSG( win, 0,
                                 _T("FindWindowForMouseEvent() returned NULL") );
                }
#ifdef __POCKETPC__
                if (IsContextMenuEnabled() && message == WM_LBUTTONDOWN)
                {
                    SHRGINFO shrgi = {0};

                    shrgi.cbSize = sizeof(SHRGINFO);
                    shrgi.hwndClient = (HWND) GetHWND();
                    shrgi.ptDown.x = x;
                    shrgi.ptDown.y = y;

                    shrgi.dwFlags = SHRG_RETURNCMD;
                    // shrgi.dwFlags = SHRG_NOTIFYPARENT;

                    if (GN_CONTEXTMENU == ::SHRecognizeGesture(&shrgi))
                    {
                        wxPoint pt(x, y);
                        pt = ClientToScreen(pt);

                        wxContextMenuEvent evtCtx(wxEVT_CONTEXT_MENU, GetId(), pt);

                        evtCtx.SetEventObject(this);
                        if (GetEventHandler()->ProcessEvent(evtCtx))
                        {
                            processed = true;
                            return true;
                        }
                    }
                }
#endif

#else // !__WXWINCE__
                wxWindowMSW *win = this;
#endif // __WXWINCE__/!__WXWINCE__

                processed = win->HandleMouseEvent(message, x, y, wParam);

                // if the app didn't eat the event, handle it in the default
                // way, that is by giving this window the focus
                if ( !processed )
                {
                    // for the standard classes their WndProc sets the focus to
                    // them anyhow and doing it from here results in some weird
                    // problems, so don't do it for them (unnecessary anyhow)
                    if ( !win->IsOfStandardClass() )
                    {
                        if ( message == WM_LBUTTONDOWN && win->AcceptsFocus() )
                            win->SetFocus();
                    }
                }
            }
            break;

#ifdef MM_JOY1MOVE
        case MM_JOY1MOVE:
        case MM_JOY2MOVE:
        case MM_JOY1ZMOVE:
        case MM_JOY2ZMOVE:
        case MM_JOY1BUTTONDOWN:
        case MM_JOY2BUTTONDOWN:
        case MM_JOY1BUTTONUP:
        case MM_JOY2BUTTONUP:
            processed = HandleJoystickEvent(message,
                                            GET_X_LPARAM(lParam),
                                            GET_Y_LPARAM(lParam),
                                            wParam);
            break;
#endif // __WXMICROWIN__

        case WM_COMMAND:
            {
                WORD id, cmd;
                WXHWND hwnd;
                UnpackCommand(wParam, lParam, &id, &hwnd, &cmd);

                processed = HandleCommand(id, cmd, hwnd);
            }
            break;

        case WM_NOTIFY:
            processed = HandleNotify((int)wParam, lParam, &rc.result);
            break;

        // we only need to reply to WM_NOTIFYFORMAT manually when using MSLU,
        // otherwise DefWindowProc() does it perfectly fine for us, but MSLU
        // apparently doesn't always behave properly and needs some help
#if wxUSE_UNICODE_MSLU && defined(NF_QUERY)
        case WM_NOTIFYFORMAT:
            if ( lParam == NF_QUERY )
            {
                processed = true;
                rc.result = NFR_UNICODE;
            }
            break;
#endif // wxUSE_UNICODE_MSLU

            // for these messages we must return true if process the message
#ifdef WM_DRAWITEM
        case WM_DRAWITEM:
        case WM_MEASUREITEM:
            {
                int idCtrl = (UINT)wParam;
                if ( message == WM_DRAWITEM )
                {
                    processed = MSWOnDrawItem(idCtrl,
                                              (WXDRAWITEMSTRUCT *)lParam);
                }
                else
                {
                    processed = MSWOnMeasureItem(idCtrl,
                                                 (WXMEASUREITEMSTRUCT *)lParam);
                }

                if ( processed )
                    rc.result = TRUE;
            }
            break;
#endif // defined(WM_DRAWITEM)

        case WM_GETDLGCODE:
            if ( !IsOfStandardClass() || HasFlag(wxWANTS_CHARS) )
            {
                // we always want to get the char events
                rc.result = DLGC_WANTCHARS;

                if ( HasFlag(wxWANTS_CHARS) )
                {
                    // in fact, we want everything
                    rc.result |= DLGC_WANTARROWS |
                                 DLGC_WANTTAB |
                                 DLGC_WANTALLKEYS;
                }

                processed = true;
            }
            //else: get the dlg code from the DefWindowProc()
            break;

        case WM_SYSKEYDOWN:
        case WM_KEYDOWN:
            // If this has been processed by an event handler, return 0 now
            // (we've handled it).
            m_lastKeydownProcessed = HandleKeyDown((WORD) wParam, lParam);
            if ( m_lastKeydownProcessed )
            {
                processed = true;
            }

            if ( !processed )
            {
                switch ( wParam )
                {
                    // we consider these messages "not interesting" to OnChar, so
                    // just don't do anything more with them
                    case VK_SHIFT:
                    case VK_CONTROL:
                    case VK_MENU:
                    case VK_CAPITAL:
                    case VK_NUMLOCK:
                    case VK_SCROLL:
                        processed = true;
                        break;

                    // avoid duplicate messages to OnChar for these ASCII keys:
                    // they will be translated by TranslateMessage() and received
                    // in WM_CHAR
                    case VK_ESCAPE:
                    case VK_SPACE:
                    case VK_RETURN:
                    case VK_BACK:
                    case VK_TAB:
                    case VK_ADD:
                    case VK_SUBTRACT:
                    case VK_MULTIPLY:
                    case VK_DIVIDE:
                    case VK_NUMPAD0:
                    case VK_NUMPAD1:
                    case VK_NUMPAD2:
                    case VK_NUMPAD3:
                    case VK_NUMPAD4:
                    case VK_NUMPAD5:
                    case VK_NUMPAD6:
                    case VK_NUMPAD7:
                    case VK_NUMPAD8:
                    case VK_NUMPAD9:
                    case VK_OEM_1:
                    case VK_OEM_2:
                    case VK_OEM_3:
                    case VK_OEM_4:
                    case VK_OEM_5:
                    case VK_OEM_6:
                    case VK_OEM_7:
                    case VK_OEM_PLUS:
                    case VK_OEM_COMMA:
                    case VK_OEM_MINUS:
                    case VK_OEM_PERIOD:
                        // but set processed to false, not true to still pass them
                        // to the control's default window proc - otherwise
                        // built-in keyboard handling won't work
                        processed = false;
                        break;

#ifdef VK_APPS
                    // special case of VK_APPS: treat it the same as right mouse
                    // click because both usually pop up a context menu
                    case VK_APPS:
                        processed = HandleMouseEvent(WM_RBUTTONDOWN, -1, -1, 0);
                        break;
#endif // VK_APPS

                    default:
                        // do generate a CHAR event
                        processed = HandleChar((WORD)wParam, lParam);
                }
            }
            if (message == WM_SYSKEYDOWN)  // Let Windows still handle the SYSKEYs
                processed = false;
            break;

        case WM_SYSKEYUP:
        case WM_KEYUP:
#ifdef VK_APPS
            // special case of VK_APPS: treat it the same as right mouse button
            if ( wParam == VK_APPS )
            {
                processed = HandleMouseEvent(WM_RBUTTONUP, -1, -1, 0);
            }
            else
#endif // VK_APPS
            {
                processed = HandleKeyUp((WORD) wParam, lParam);
            }
            break;

        case WM_SYSCHAR:
        case WM_CHAR: // Always an ASCII character
            if ( m_lastKeydownProcessed )
            {
                // The key was handled in the EVT_KEY_DOWN and handling
                // a key in an EVT_KEY_DOWN handler is meant, by
                // design, to prevent EVT_CHARs from happening
                m_lastKeydownProcessed = false;
                processed = true;
            }
            else
            {
                processed = HandleChar((WORD)wParam, lParam, true);
            }
            break;

#if wxUSE_HOTKEY
        case WM_HOTKEY:
            processed = HandleHotKey((WORD)wParam, lParam);
            break;
#endif // wxUSE_HOTKEY

        case WM_HSCROLL:
        case WM_VSCROLL:
            {
                WXWORD code, pos;
                WXHWND hwnd;
                UnpackScroll(wParam, lParam, &code, &pos, &hwnd);

                processed = MSWOnScroll(message == WM_HSCROLL ? wxHORIZONTAL
                                                              : wxVERTICAL,
                                        code, pos, hwnd);
            }
            break;

        // CTLCOLOR messages are sent by children to query the parent for their
        // colors
#ifndef __WXMICROWIN__
        case WM_CTLCOLORMSGBOX:
        case WM_CTLCOLOREDIT:
        case WM_CTLCOLORLISTBOX:
        case WM_CTLCOLORBTN:
        case WM_CTLCOLORDLG:
        case WM_CTLCOLORSCROLLBAR:
        case WM_CTLCOLORSTATIC:
            {
                WXHDC hdc;
                WXHWND hwnd;
                UnpackCtlColor(wParam, lParam, &hdc, &hwnd);

                processed = HandleCtlColor(&rc.hBrush, (WXHDC)hdc, (WXHWND)hwnd);
            }
            break;
#endif // !__WXMICROWIN__

        case WM_SYSCOLORCHANGE:
            // the return value for this message is ignored
            processed = HandleSysColorChange();
            break;

#if !defined(__WXWINCE__)
        case WM_DISPLAYCHANGE:
            processed = HandleDisplayChange();
            break;
#endif

        case WM_PALETTECHANGED:
            processed = HandlePaletteChanged((WXHWND) (HWND) wParam);
            break;

        case WM_CAPTURECHANGED:
            processed = HandleCaptureChanged((WXHWND) (HWND) lParam);
            break;

        case WM_SETTINGCHANGE:
            processed = HandleSettingChange(wParam, lParam);
            break;

        case WM_QUERYNEWPALETTE:
            processed = HandleQueryNewPalette();
            break;

        case WM_ERASEBKGND:
            processed = HandleEraseBkgnd((WXHDC)(HDC)wParam);
            if ( processed )
            {
                // we processed the message, i.e. erased the background
                rc.result = TRUE;
            }
            break;

#if !defined(__WXWINCE__)
        case WM_DROPFILES:
            processed = HandleDropFiles(wParam);
            break;
#endif

        case WM_INITDIALOG:
            processed = HandleInitDialog((WXHWND)(HWND)wParam);

            if ( processed )
            {
                // we never set focus from here
                rc.result = FALSE;
            }
            break;

#if !defined(__WXWINCE__)
        case WM_QUERYENDSESSION:
            processed = HandleQueryEndSession(lParam, &rc.allow);
            break;

        case WM_ENDSESSION:
            processed = HandleEndSession(wParam != 0, lParam);
            break;

        case WM_GETMINMAXINFO:
            processed = HandleGetMinMaxInfo((MINMAXINFO*)lParam);
            break;
#endif

        case WM_SETCURSOR:
            processed = HandleSetCursor((WXHWND)(HWND)wParam,
                                        LOWORD(lParam),     // hit test
                                        HIWORD(lParam));    // mouse msg

            if ( processed )
            {
                // returning TRUE stops the DefWindowProc() from further
                // processing this message - exactly what we need because we've
                // just set the cursor.
                rc.result = TRUE;
            }
            break;

#if wxUSE_ACCESSIBILITY
        case WM_GETOBJECT:
            {
                //WPARAM dwFlags = (WPARAM) (DWORD) wParam;
                LPARAM dwObjId = (LPARAM) (DWORD) lParam;

                if (dwObjId == (LPARAM)OBJID_CLIENT && GetOrCreateAccessible())
                {
                    return LresultFromObject(IID_IAccessible, wParam, (IUnknown*) GetAccessible()->GetIAccessible());
                }
                break;
            }
#endif

#if defined(WM_HELP)
        case WM_HELP:
            {
                // by default, WM_HELP is propagated by DefWindowProc() upwards
                // to the window parent but as we do it ourselves already
                // (wxHelpEvent is derived from wxCommandEvent), we don't want
                // to get the other events if we process this message at all
                processed = true;

                // WM_HELP doesn't use lParam under CE
#ifndef __WXWINCE__
                HELPINFO* info = (HELPINFO*) lParam;
                if ( info->iContextType == HELPINFO_WINDOW )
                {
#endif // !__WXWINCE__
                    wxHelpEvent helpEvent
                                (
                                    wxEVT_HELP,
                                    GetId(),
#ifdef __WXWINCE__
                                    wxGetMousePosition() // what else?
#else
                                    wxPoint(info->MousePos.x, info->MousePos.y)
#endif
                                );

                    helpEvent.SetEventObject(this);
                    GetEventHandler()->ProcessEvent(helpEvent);
#ifndef __WXWINCE__
                }
                else if ( info->iContextType == HELPINFO_MENUITEM )
                {
                    wxHelpEvent helpEvent(wxEVT_HELP, info->iCtrlId);
                    helpEvent.SetEventObject(this);
                    GetEventHandler()->ProcessEvent(helpEvent);

                }
                else // unknown help event?
                {
                    processed = false;
                }
#endif // !__WXWINCE__
            }
            break;
#endif // WM_HELP

#if !defined(__WXWINCE__)
        case WM_CONTEXTMENU:
            {
                // we don't convert from screen to client coordinates as
                // the event may be handled by a parent window
                wxPoint pt(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));

                wxContextMenuEvent evtCtx(wxEVT_CONTEXT_MENU, GetId(), pt);

                // we could have got an event from our child, reflect it back
                // to it if this is the case
                wxWindowMSW *win = NULL;
                if ( (WXHWND)wParam != m_hWnd )
                {
                    win = FindItemByHWND((WXHWND)wParam);
                }

                if ( !win )
                    win = this;

                evtCtx.SetEventObject(win);
                processed = win->GetEventHandler()->ProcessEvent(evtCtx);
            }
            break;
#endif

        case WM_MENUCHAR:
            // we're only interested in our own menus, not MF_SYSMENU
            if ( HIWORD(wParam) == MF_POPUP )
            {
                // handle menu chars for ownerdrawn menu items
                int i = HandleMenuChar(toupper(LOWORD(wParam)), lParam);
                if ( i != wxNOT_FOUND )
                {
                    rc.result = MAKELRESULT(i, MNC_EXECUTE);
                    processed = true;
                }
            }
            break;

#ifndef __WXWINCE__
        case WM_POWERBROADCAST:
            {
                bool vetoed;
                processed = HandlePower(wParam, lParam, &vetoed);
                rc.result = processed && vetoed ? BROADCAST_QUERY_DENY : TRUE;
            }
            break;
#endif // __WXWINCE__

#if wxUSE_UXTHEME
        // If we want the default themed border then we need to draw it ourselves
        case WM_NCCALCSIZE:
            {
                wxUxThemeEngine* theme = wxUxThemeEngine::GetIfActive();
                if (theme && TranslateBorder(GetBorder()) == wxBORDER_THEME)
                {
                    // first ask the widget to calculate the border size
                    rc.result = MSWDefWindowProc(message, wParam, lParam);
                    processed = true;

                    // now alter the client size making room for drawing a themed border
                    NCCALCSIZE_PARAMS *csparam = NULL;
                    RECT rect;
                    if (wParam)
                    {
                        csparam = (NCCALCSIZE_PARAMS*)lParam;
                        rect = csparam->rgrc[0];
                    }
                    else
                    {
                        rect = *((RECT*)lParam);
                    }
                    wxUxThemeHandle hTheme((const wxWindow*) this, L"EDIT");
                    RECT rcClient = { 0, 0, 0, 0 };
                    wxClientDC dc((wxWindow*) this);

                    if (theme->GetThemeBackgroundContentRect(
                            hTheme, GetHdcOf(dc), EP_EDITTEXT, ETS_NORMAL,
                            &rect, &rcClient) == S_OK)
                    {
                        InflateRect(&rcClient, -1, -1);
                        if (wParam)
                            csparam->rgrc[0] = rcClient;
                        else
                            *((RECT*)lParam) = rcClient;
                        rc.result = WVR_REDRAW;
                    }
                }
            }
            break;

        case WM_NCPAINT:
            {
                wxUxThemeEngine* theme = wxUxThemeEngine::GetIfActive();
                if (theme && TranslateBorder(GetBorder()) == wxBORDER_THEME)
                {
                    // first ask the widget to paint its non-client area, such as scrollbars, etc.
                    rc.result = MSWDefWindowProc(message, wParam, lParam);
                    processed = true;

                    wxUxThemeHandle hTheme((const wxWindow*) this, L"EDIT");
                    wxWindowDC dc((wxWindow*) this);

                    // Clip the DC so that you only draw on the non-client area
                    RECT rcBorder;
                    wxCopyRectToRECT(GetSize(), rcBorder);

                    RECT rcClient;
                    theme->GetThemeBackgroundContentRect(
                        hTheme, GetHdcOf(dc), EP_EDITTEXT, ETS_NORMAL, &rcBorder, &rcClient);
                    InflateRect(&rcClient, -1, -1);

                    ::ExcludeClipRect(GetHdcOf(dc), rcClient.left, rcClient.top,
                                      rcClient.right, rcClient.bottom);

                    // Make sure the background is in a proper state
                    if (theme->IsThemeBackgroundPartiallyTransparent(hTheme, EP_EDITTEXT, ETS_NORMAL))
                    {
                        theme->DrawThemeParentBackground(GetHwnd(), GetHdcOf(dc), &rcBorder);
                    }

                    // Draw the border
                    int nState;
                    if ( !IsEnabled() )
                        nState = ETS_DISABLED;
                    // should we check this?
                    //else if ( ::GetWindowLong(GetHwnd(), GWL_STYLE) & ES_READONLY)
                    //    nState = ETS_READONLY;
                    else
                        nState = ETS_NORMAL;
                    theme->DrawThemeBackground(hTheme, GetHdcOf(dc), EP_EDITTEXT, nState, &rcBorder, NULL);
                }
            }
            break;

#endif // wxUSE_UXTHEME

    }

    if ( !processed )
    {
#ifdef __WXDEBUG__
        wxLogTrace(wxTraceMessages, wxT("Forwarding %s to DefWindowProc."),
                   wxGetMessageName(message));
#endif // __WXDEBUG__
        rc.result = MSWDefWindowProc(message, wParam, lParam);
    }

    return rc.result;
}

// ----------------------------------------------------------------------------
// wxWindow <-> HWND map
// ----------------------------------------------------------------------------

wxWinHashTable *wxWinHandleHash = NULL;

wxWindow *wxFindWinFromHandle(WXHWND hWnd)
{
    return (wxWindow*)wxWinHandleHash->Get((long)hWnd);
}

void wxAssociateWinWithHandle(HWND hWnd, wxWindowMSW *win)
{
    // adding NULL hWnd is (first) surely a result of an error and
    // (secondly) breaks menu command processing
    wxCHECK_RET( hWnd != (HWND)NULL,
                 wxT("attempt to add a NULL hWnd to window list ignored") );

    wxWindow *oldWin = wxFindWinFromHandle((WXHWND) hWnd);
#ifdef __WXDEBUG__
    if ( oldWin && (oldWin != win) )
    {
        wxLogDebug(wxT("HWND %X already associated with another window (%s)"),
                   (int) hWnd, win->GetClassInfo()->GetClassName());
    }
    else
#endif // __WXDEBUG__
    if (!oldWin)
    {
        wxWinHandleHash->Put((long)hWnd, (wxWindow *)win);
    }
}

void wxRemoveHandleAssociation(wxWindowMSW *win)
{
    wxWinHandleHash->Delete((long)win->GetHWND());
}

// ----------------------------------------------------------------------------
// various MSW speciic class dependent functions
// ----------------------------------------------------------------------------

// Default destroyer - override if you destroy it in some other way
// (e.g. with MDI child windows)
void wxWindowMSW::MSWDestroyWindow()
{
}

bool wxWindowMSW::MSWGetCreateWindowCoords(const wxPoint& pos,
                                           const wxSize& size,
                                           int& x, int& y,
                                           int& w, int& h) const
{
    // yes, those are just some arbitrary hardcoded numbers
    static const int DEFAULT_Y = 200;

    bool nonDefault = false;

    if ( pos.x == wxDefaultCoord )
    {
        // if x is set to CW_USEDEFAULT, y parameter is ignored anyhow so we
        // can just as well set it to CW_USEDEFAULT as well
        x =
        y = CW_USEDEFAULT;
    }
    else
    {
        // OTOH, if x is not set to CW_USEDEFAULT, y shouldn't be set to it
        // neither because it is not handled as a special value by Windows then
        // and so we have to choose some default value for it
        x = pos.x;
        y = pos.y == wxDefaultCoord ? DEFAULT_Y : pos.y;

        nonDefault = true;
    }

    /*
      NB: there used to be some code here which set the initial size of the
          window to the client size of the parent if no explicit size was
          specified. This was wrong because wxWidgets programs often assume
          that they get a WM_SIZE (EVT_SIZE) upon creation, however this broke
          it. To see why, you should understand that Windows sends WM_SIZE from
          inside ::CreateWindow() anyhow. However, ::CreateWindow() is called
          from some base class ctor and so this WM_SIZE is not processed in the
          real class' OnSize() (because it's not fully constructed yet and the
          event goes to some base class OnSize() instead). So the WM_SIZE we
          rely on is the one sent when the parent frame resizes its children
          but here is the problem: if the child already has just the right
          size, nothing will happen as both wxWidgets and Windows check for
          this and ignore any attempts to change the window size to the size it
          already has - so no WM_SIZE would be sent.
     */


    // we don't use CW_USEDEFAULT here for several reasons:
    //
    //  1. it results in huge frames on modern screens (1000*800 is not
    //     uncommon on my 1280*1024 screen) which is way too big for a half
    //     empty frame of most of wxWidgets samples for example)
    //
    //  2. it is buggy for frames with wxFRAME_TOOL_WINDOW style for which
    //     the default is for whatever reason 8*8 which breaks client <->
    //     window size calculations (it would be nice if it didn't, but it
    //     does and the simplest way to fix it seemed to change the broken
    //     default size anyhow)
    //
    //  3. there is just no advantage in doing it: with x and y it is
    //     possible that [future versions of] Windows position the new top
    //     level window in some smart way which we can't do, but we can
    //     guess a reasonably good size for a new window just as well
    //     ourselves

    // However, on PocketPC devices, we must use the default
    // size if possible.
#ifdef _WIN32_WCE
    if (size.x == wxDefaultCoord)
        w = CW_USEDEFAULT;
    else
        w = size.x;
    if (size.y == wxDefaultCoord)
        h = CW_USEDEFAULT;
    else
        h = size.y;
#else
    if ( size.x == wxDefaultCoord || size.y == wxDefaultCoord)
    {
        nonDefault = true;
    }
    w = WidthDefault(size.x);
    h = HeightDefault(size.y);
#endif

    AdjustForParentClientOrigin(x, y);

    return nonDefault;
}

WXHWND wxWindowMSW::MSWGetParent() const
{
    return m_parent ? m_parent->GetHWND() : WXHWND(NULL);
}

bool wxWindowMSW::MSWCreate(const wxChar *wclass,
                            const wxChar *title,
                            const wxPoint& pos,
                            const wxSize& size,
                            WXDWORD style,
                            WXDWORD extendedStyle)
{
    // choose the position/size for the new window
    int x, y, w, h;
    (void)MSWGetCreateWindowCoords(pos, size, x, y, w, h);

    // controlId is menu handle for the top level windows, so set it to 0
    // unless we're creating a child window
    int controlId = style & WS_CHILD ? GetId() : 0;

    // for each class "Foo" we have we also have "FooNR" ("no repaint") class
    // which is the same but without CS_[HV]REDRAW class styles so using it
    // ensures that the window is not fully repainted on each resize
    wxString className(wclass);
    if ( !HasFlag(wxFULL_REPAINT_ON_RESIZE) )
    {
        className += wxT("NR");
    }

    // do create the window
    wxWindowCreationHook hook(this);

    m_hWnd = (WXHWND)::CreateWindowEx
                       (
                        extendedStyle,
                        className,
                        title ? title : m_windowName.c_str(),
                        style,
                        x, y, w, h,
                        (HWND)MSWGetParent(),
                        (HMENU)controlId,
                        wxGetInstance(),
                        NULL                        // no extra data
                       );

    if ( !m_hWnd )
    {
        wxLogSysError(_("Can't create window of class %s"), className.c_str());

        return false;
    }

    SubclassWin(m_hWnd);

    return true;
}

// ===========================================================================
// MSW message handlers
// ===========================================================================

// ---------------------------------------------------------------------------
// WM_NOTIFY
// ---------------------------------------------------------------------------

bool wxWindowMSW::HandleNotify(int idCtrl, WXLPARAM lParam, WXLPARAM *result)
{
#ifndef __WXMICROWIN__
    LPNMHDR hdr = (LPNMHDR)lParam;
    HWND hWnd = hdr->hwndFrom;
    wxWindow *win = wxFindWinFromHandle((WXHWND)hWnd);

    // if the control is one of our windows, let it handle the message itself
    if ( win )
    {
        return win->MSWOnNotify(idCtrl, lParam, result);
    }

    // VZ: why did we do it? normally this is unnecessary and, besides, it
    //     breaks the message processing for the toolbars because the tooltip
    //     notifications were being forwarded to the toolbar child controls
    //     (if it had any) before being passed to the toolbar itself, so in my
    //     example the tooltip for the combobox was always shown instead of the
    //     correct button tooltips
#if 0
    // try all our children
    wxWindowList::compatibility_iterator node = GetChildren().GetFirst();
    while ( node )
    {
        wxWindow *child = node->GetData();
        if ( child->MSWOnNotify(idCtrl, lParam, result) )
        {
            return true;
        }

        node = node->GetNext();
    }
#endif // 0

    // by default, handle it ourselves
    return MSWOnNotify(idCtrl, lParam, result);
#else // __WXMICROWIN__
    return false;
#endif
}

#if wxUSE_TOOLTIPS

bool wxWindowMSW::HandleTooltipNotify(WXUINT code,
                                      WXLPARAM lParam,
                                      const wxString& ttip)
{
    // I don't know why it happens, but the versions of comctl32.dll starting
    // from 4.70 sometimes send TTN_NEEDTEXTW even to ANSI programs (normally,
    // this message is supposed to be sent to Unicode programs only) -- hence
    // we need to handle it as well, otherwise no tooltips will be shown in
    // this case
#ifndef __WXWINCE__
    if ( !(code == (WXUINT) TTN_NEEDTEXTA || code == (WXUINT) TTN_NEEDTEXTW)
            || ttip.empty() )
    {
        // not a tooltip message or no tooltip to show anyhow
        return false;
    }
#endif

    LPTOOLTIPTEXT ttText = (LPTOOLTIPTEXT)lParam;

    // We don't want to use the szText buffer because it has a limit of 80
    // bytes and this is not enough, especially for Unicode build where it
    // limits the tooltip string length to only 40 characters
    //
    // The best would be, of course, to not impose any length limitations at
    // all but then the buffer would have to be dynamic and someone would have
    // to free it and we don't have the tooltip owner object here any more, so
    // for now use our own static buffer with a higher fixed max length.
    //
    // Note that using a static buffer should not be a problem as only a single
    // tooltip can be shown at the same time anyhow.
#if !wxUSE_UNICODE
    if ( code == (WXUINT) TTN_NEEDTEXTW )
    {
        // We need to convert tooltip from multi byte to Unicode on the fly.
        static wchar_t buf[513];

        // Truncate tooltip length if needed as otherwise we might not have
        // enough space for it in the buffer and MultiByteToWideChar() would
        // return an error
        size_t tipLength = wxMin(ttip.length(), WXSIZEOF(buf) - 1);

        // Convert to WideChar without adding the NULL character. The NULL
        // character is added afterwards (this is more efficient).
        int len = ::MultiByteToWideChar
                    (
                        CP_ACP,
                        0,                      // no flags
                        ttip,
                        tipLength,
                        buf,
                        WXSIZEOF(buf) - 1
                    );

        if ( !len )
        {
            wxLogLastError(_T("MultiByteToWideChar()"));
        }

        buf[len] = L'\0';
        ttText->lpszText = (LPSTR) buf;
    }
    else // TTN_NEEDTEXTA
#endif // !wxUSE_UNICODE
    {
        // we get here if we got TTN_NEEDTEXTA (only happens in ANSI build) or
        // if we got TTN_NEEDTEXTW in Unicode build: in this case we just have
        // to copy the string we have into the buffer
        static wxChar buf[513];
        wxStrncpy(buf, ttip.c_str(), WXSIZEOF(buf) - 1);
        buf[WXSIZEOF(buf) - 1] = _T('\0');
        ttText->lpszText = buf;
    }

    return true;
}

#endif // wxUSE_TOOLTIPS

bool wxWindowMSW::MSWOnNotify(int WXUNUSED(idCtrl),
                              WXLPARAM lParam,
                              WXLPARAM* WXUNUSED(result))
{
#if wxUSE_TOOLTIPS
    if ( m_tooltip )
    {
        NMHDR* hdr = (NMHDR *)lParam;
        if ( HandleTooltipNotify(hdr->code, lParam, m_tooltip->GetTip()))
        {
            // processed
            return true;
        }
    }
#else
    wxUnusedVar(lParam);
#endif // wxUSE_TOOLTIPS

    return false;
}

// ---------------------------------------------------------------------------
// end session messages
// ---------------------------------------------------------------------------

bool wxWindowMSW::HandleQueryEndSession(long logOff, bool *mayEnd)
{
#ifdef ENDSESSION_LOGOFF
    wxCloseEvent event(wxEVT_QUERY_END_SESSION, wxID_ANY);
    event.SetEventObject(wxTheApp);
    event.SetCanVeto(true);
    event.SetLoggingOff(logOff == (long)ENDSESSION_LOGOFF);

    bool rc = wxTheApp->ProcessEvent(event);

    if ( rc )
    {
        // we may end only if the app didn't veto session closing (double
        // negation...)
        *mayEnd = !event.GetVeto();
    }

    return rc;
#else
    wxUnusedVar(logOff);
    wxUnusedVar(mayEnd);
    return false;
#endif
}

bool wxWindowMSW::HandleEndSession(bool endSession, long logOff)
{
#ifdef ENDSESSION_LOGOFF
    // do nothing if the session isn't ending
    if ( !endSession )
        return false;

    // only send once
    if ( (this != wxTheApp->GetTopWindow()) )
        return false;

    wxCloseEvent event(wxEVT_END_SESSION, wxID_ANY);
    event.SetEventObject(wxTheApp);
    event.SetCanVeto(false);
    event.SetLoggingOff((logOff & ENDSESSION_LOGOFF) != 0);

    return wxTheApp->ProcessEvent(event);
#else
    wxUnusedVar(endSession);
    wxUnusedVar(logOff);
    return false;
#endif
}

// ---------------------------------------------------------------------------
// window creation/destruction
// ---------------------------------------------------------------------------

bool wxWindowMSW::HandleCreate(WXLPCREATESTRUCT WXUNUSED_IN_WINCE(cs),
                               bool *mayCreate)
{
    // VZ: why is this commented out for WinCE? If it doesn't support
    //     WS_EX_CONTROLPARENT at all it should be somehow handled globally,
    //     not with multiple #ifdef's!
#ifndef __WXWINCE__
    if ( ((CREATESTRUCT *)cs)->dwExStyle & WS_EX_CONTROLPARENT )
        EnsureParentHasControlParentStyle(GetParent());
#endif // !__WXWINCE__

    *mayCreate = true;

    return true;
}

bool wxWindowMSW::HandleDestroy()
{
    SendDestroyEvent();

    // delete our drop target if we've got one
#if wxUSE_DRAG_AND_DROP
    if ( m_dropTarget != NULL )
    {
        m_dropTarget->Revoke(m_hWnd);

        delete m_dropTarget;
        m_dropTarget = NULL;
    }
#endif // wxUSE_DRAG_AND_DROP

    // WM_DESTROY handled
    return true;
}

// ---------------------------------------------------------------------------
// activation/focus
// ---------------------------------------------------------------------------

bool wxWindowMSW::HandleActivate(int state,
                              bool WXUNUSED(minimized),
                              WXHWND WXUNUSED(activate))
{
    wxActivateEvent event(wxEVT_ACTIVATE,
                          (state == WA_ACTIVE) || (state == WA_CLICKACTIVE),
                          m_windowId);
    event.SetEventObject(this);

    return GetEventHandler()->ProcessEvent(event);
}

bool wxWindowMSW::HandleSetFocus(WXHWND hwnd)
{
    // Strangly enough, some controls get set focus events when they are being
    // deleted, even if they already had focus before.
    if ( m_isBeingDeleted )
    {
        return false;
    }

    // notify the parent keeping track of focus for the kbd navigation
    // purposes that we got it
    wxChildFocusEvent eventFocus((wxWindow *)this);
    (void)GetEventHandler()->ProcessEvent(eventFocus);

#if wxUSE_CARET
    // Deal with caret
    if ( m_caret )
    {
        m_caret->OnSetFocus();
    }
#endif // wxUSE_CARET

#if wxUSE_TEXTCTRL
    // If it's a wxTextCtrl don't send the event as it will be done
    // after the control gets to process it from EN_FOCUS handler
    if ( wxDynamicCastThis(wxTextCtrl) )
    {
        return false;
    }
#endif // wxUSE_TEXTCTRL

    wxFocusEvent event(wxEVT_SET_FOCUS, m_windowId);
    event.SetEventObject(this);

    // wxFindWinFromHandle() may return NULL, it is ok
    event.SetWindow(wxFindWinFromHandle(hwnd));

    return GetEventHandler()->ProcessEvent(event);
}

bool wxWindowMSW::HandleKillFocus(WXHWND hwnd)
{
#if wxUSE_CARET
    // Deal with caret
    if ( m_caret )
    {
        m_caret->OnKillFocus();
    }
#endif // wxUSE_CARET

#if wxUSE_TEXTCTRL
    // If it's a wxTextCtrl don't send the event as it will be done
    // after the control gets to process it.
    wxTextCtrl *ctrl = wxDynamicCastThis(wxTextCtrl);
    if ( ctrl )
    {
        return false;
    }
#endif

    // Don't send the event when in the process of being deleted.  This can
    // only cause problems if the event handler tries to access the object.
    if ( m_isBeingDeleted )
    {
        return false;
    }

    wxFocusEvent event(wxEVT_KILL_FOCUS, m_windowId);
    event.SetEventObject(this);

    // wxFindWinFromHandle() may return NULL, it is ok
    event.SetWindow(wxFindWinFromHandle(hwnd));

    return GetEventHandler()->ProcessEvent(event);
}

// ---------------------------------------------------------------------------
// labels
// ---------------------------------------------------------------------------

void wxWindowMSW::SetLabel( const wxString& label)
{
    SetWindowText(GetHwnd(), label.c_str());
}

wxString wxWindowMSW::GetLabel() const
{
    return wxGetWindowText(GetHWND());
}

// ---------------------------------------------------------------------------
// miscellaneous
// ---------------------------------------------------------------------------

bool wxWindowMSW::HandleShow(bool show, int WXUNUSED(status))
{
    wxShowEvent event(GetId(), show);
    event.SetEventObject(this);

    return GetEventHandler()->ProcessEvent(event);
}

bool wxWindowMSW::HandleInitDialog(WXHWND WXUNUSED(hWndFocus))
{
    wxInitDialogEvent event(GetId());
    event.SetEventObject(this);

    return GetEventHandler()->ProcessEvent(event);
}

bool wxWindowMSW::HandleDropFiles(WXWPARAM wParam)
{
#if defined (__WXMICROWIN__) || defined(__WXWINCE__)
    wxUnusedVar(wParam);
    return false;
#else // __WXMICROWIN__
    HDROP hFilesInfo = (HDROP) wParam;

    // Get the total number of files dropped
    UINT gwFilesDropped = ::DragQueryFile
                            (
                                (HDROP)hFilesInfo,
                                (UINT)-1,
                                (LPTSTR)0,
                                (UINT)0
                            );

    wxString *files = new wxString[gwFilesDropped];
    for ( UINT wIndex = 0; wIndex < gwFilesDropped; wIndex++ )
    {
        // first get the needed buffer length (+1 for terminating NUL)
        size_t len = ::DragQueryFile(hFilesInfo, wIndex, NULL, 0) + 1;

        // and now get the file name
        ::DragQueryFile(hFilesInfo, wIndex,
                        wxStringBuffer(files[wIndex], len), len);
    }

    wxDropFilesEvent event(wxEVT_DROP_FILES, gwFilesDropped, files);
    event.SetEventObject(this);

    POINT dropPoint;
    DragQueryPoint(hFilesInfo, (LPPOINT) &dropPoint);
    event.m_pos.x = dropPoint.x;
    event.m_pos.y = dropPoint.y;

    DragFinish(hFilesInfo);

    return GetEventHandler()->ProcessEvent(event);
#endif
}


bool wxWindowMSW::HandleSetCursor(WXHWND WXUNUSED(hWnd),
                                  short nHitTest,
                                  int WXUNUSED(mouseMsg))
{
#ifndef __WXMICROWIN__
    // the logic is as follows:
    // -1. don't set cursor for non client area, including but not limited to
    //     the title bar, scrollbars, &c
    //  0. allow the user to override default behaviour by using EVT_SET_CURSOR
    //  1. if we have the cursor set it unless wxIsBusy()
    //  2. if we're a top level window, set some cursor anyhow
    //  3. if wxIsBusy(), set the busy cursor, otherwise the global one

    if ( nHitTest != HTCLIENT )
    {
        return false;
    }

    HCURSOR hcursor = 0;

    // first ask the user code - it may wish to set the cursor in some very
    // specific way (for example, depending on the current position)
    POINT pt;
#ifdef __WXWINCE__
    if ( !::GetCursorPosWinCE(&pt))
#else
    if ( !::GetCursorPos(&pt) )
#endif
    {
        wxLogLastError(wxT("GetCursorPos"));
    }

    int x = pt.x,
        y = pt.y;
    ScreenToClient(&x, &y);
    wxSetCursorEvent event(x, y);

    bool processedEvtSetCursor = GetEventHandler()->ProcessEvent(event);
    if ( processedEvtSetCursor && event.HasCursor() )
    {
        hcursor = GetHcursorOf(event.GetCursor());
    }

    if ( !hcursor )
    {
        bool isBusy = wxIsBusy();

        // the test for processedEvtSetCursor is here to prevent using m_cursor
        // if the user code caught EVT_SET_CURSOR() and returned nothing from
        // it - this is a way to say that our cursor shouldn't be used for this
        // point
        if ( !processedEvtSetCursor && m_cursor.Ok() )
        {
            hcursor = GetHcursorOf(m_cursor);
        }

        if ( !GetParent() )
        {
            if ( isBusy )
            {
                hcursor = wxGetCurrentBusyCursor();
            }
            else if ( !hcursor )
            {
                const wxCursor *cursor = wxGetGlobalCursor();
                if ( cursor && cursor->Ok() )
                {
                    hcursor = GetHcursorOf(*cursor);
                }
            }
        }
    }

    if ( hcursor )
    {
//        wxLogDebug("HandleSetCursor: Setting cursor %ld", (long) hcursor);

        ::SetCursor(hcursor);

        // cursor set, stop here
        return true;
    }
#endif // __WXMICROWIN__

    // pass up the window chain
    return false;
}

bool wxWindowMSW::HandlePower(WXWPARAM WXUNUSED_IN_WINCE(wParam),
                              WXLPARAM WXUNUSED(lParam),
                              bool *WXUNUSED_IN_WINCE(vetoed))
{
#ifdef __WXWINCE__
    // FIXME
    return false;
#else
    wxEventType evtType;
    switch ( wParam )
    {
        case PBT_APMQUERYSUSPEND:
            evtType = wxEVT_POWER_SUSPENDING;
            break;

        case PBT_APMQUERYSUSPENDFAILED:
            evtType = wxEVT_POWER_SUSPEND_CANCEL;
            break;

        case PBT_APMSUSPEND:
            evtType = wxEVT_POWER_SUSPENDED;
            break;

        case PBT_APMRESUMESUSPEND:
#ifdef PBT_APMRESUMEAUTOMATIC
        case PBT_APMRESUMEAUTOMATIC:
#endif
            evtType = wxEVT_POWER_RESUME;
            break;

        default:
            wxLogDebug(_T("Unknown WM_POWERBROADCAST(%d) event"), wParam);
            // fall through

        // these messages are currently not mapped to wx events
        case PBT_APMQUERYSTANDBY:
        case PBT_APMQUERYSTANDBYFAILED:
        case PBT_APMSTANDBY:
        case PBT_APMRESUMESTANDBY:
        case PBT_APMBATTERYLOW:
        case PBT_APMPOWERSTATUSCHANGE:
        case PBT_APMOEMEVENT:
        case PBT_APMRESUMECRITICAL:
            evtType = wxEVT_NULL;
            break;
    }

    // don't handle unknown messages
    if ( evtType == wxEVT_NULL )
        return false;

    // TODO: notify about PBTF_APMRESUMEFROMFAILURE in case of resume events?

    wxPowerEvent event(evtType);
    if ( !GetEventHandler()->ProcessEvent(event) )
        return false;

    *vetoed = event.IsVetoed();

    return true;
#endif
}

bool wxWindowMSW::IsDoubleBuffered() const
{
    const wxWindowMSW *wnd = this;
    do {
        long style = ::GetWindowLong(GetHwndOf(wnd), GWL_EXSTYLE);
        if ( (style & WS_EX_COMPOSITED) != 0 )
            return true;
        wnd = wnd->GetParent();
    } while ( wnd && !wnd->IsTopLevel() );

    return false;
}

void wxWindowMSW::SetDoubleBuffered(bool on)
{
    // Get the current extended style bits
    long exstyle = ::GetWindowLong(GetHwnd(), GWL_EXSTYLE);

    // Twiddle the bit as needed
    if ( on )
        exstyle |= WS_EX_COMPOSITED;
    else
        exstyle &= ~WS_EX_COMPOSITED;

    // put it back
    ::SetWindowLong(GetHwnd(), GWL_EXSTYLE, exstyle);
}

// ---------------------------------------------------------------------------
// owner drawn stuff
// ---------------------------------------------------------------------------

#if (wxUSE_OWNER_DRAWN && wxUSE_MENUS_NATIVE) || \
        (wxUSE_CONTROLS && !defined(__WXUNIVERSAL__))
    #define WXUNUSED_UNLESS_ODRAWN(param) param
#else
    #define WXUNUSED_UNLESS_ODRAWN(param)
#endif

bool
wxWindowMSW::MSWOnDrawItem(int WXUNUSED_UNLESS_ODRAWN(id),
                           WXDRAWITEMSTRUCT * WXUNUSED_UNLESS_ODRAWN(itemStruct))
{
#if wxUSE_OWNER_DRAWN

#if wxUSE_MENUS_NATIVE
    // is it a menu item?
    DRAWITEMSTRUCT *pDrawStruct = (DRAWITEMSTRUCT *)itemStruct;
    if ( id == 0 && pDrawStruct->CtlType == ODT_MENU )
    {
        wxMenuItem *pMenuItem = (wxMenuItem *)(pDrawStruct->itemData);

        // see comment before the same test in MSWOnMeasureItem() below
        if ( !pMenuItem )
            return false;

        wxCHECK_MSG( wxDynamicCast(pMenuItem, wxMenuItem),
                         false, _T("MSWOnDrawItem: bad wxMenuItem pointer") );

        // prepare to call OnDrawItem(): notice using of wxDCTemp to prevent
        // the DC from being released
        wxDCTemp dc((WXHDC)pDrawStruct->hDC);
        wxRect rect(pDrawStruct->rcItem.left, pDrawStruct->rcItem.top,
                    pDrawStruct->rcItem.right - pDrawStruct->rcItem.left,
                    pDrawStruct->rcItem.bottom - pDrawStruct->rcItem.top);

        return pMenuItem->OnDrawItem
               (
                dc,
                rect,
                (wxOwnerDrawn::wxODAction)pDrawStruct->itemAction,
                (wxOwnerDrawn::wxODStatus)pDrawStruct->itemState
               );
    }
#endif // wxUSE_MENUS_NATIVE

#endif // USE_OWNER_DRAWN

#if wxUSE_CONTROLS && !defined(__WXUNIVERSAL__)

#if wxUSE_OWNER_DRAWN
    wxControl *item = wxDynamicCast(FindItem(id), wxControl);
#else // !wxUSE_OWNER_DRAWN
    // we may still have owner-drawn buttons internally because we have to make
    // them owner-drawn to support colour change
    wxControl *item =
#                     if wxUSE_BUTTON
                         wxDynamicCast(FindItem(id), wxButton)
#                     else
                         NULL
#                     endif
                    ;
#endif // USE_OWNER_DRAWN

    if ( item )
    {
        return item->MSWOnDraw(itemStruct);
    }

#endif // wxUSE_CONTROLS

    return false;
}

bool
wxWindowMSW::MSWOnMeasureItem(int id, WXMEASUREITEMSTRUCT *itemStruct)
{
#if wxUSE_OWNER_DRAWN && wxUSE_MENUS_NATIVE
    // is it a menu item?
    MEASUREITEMSTRUCT *pMeasureStruct = (MEASUREITEMSTRUCT *)itemStruct;
    if ( id == 0 && pMeasureStruct->CtlType == ODT_MENU )
    {
        wxMenuItem *pMenuItem = (wxMenuItem *)(pMeasureStruct->itemData);

        // according to Carsten Fuchs the pointer may be NULL under XP if an
        // MDI child frame is initially maximized, see this for more info:
        // http://article.gmane.org/gmane.comp.lib.wxwidgets.general/27745
        //
        // so silently ignore it instead of asserting
        if ( !pMenuItem )
            return false;

        wxCHECK_MSG( wxDynamicCast(pMenuItem, wxMenuItem),
                        false, _T("MSWOnMeasureItem: bad wxMenuItem pointer") );

        size_t w, h;
        bool rc = pMenuItem->OnMeasureItem(&w, &h);

        pMeasureStruct->itemWidth = w;
        pMeasureStruct->itemHeight = h;

        return rc;
    }

    wxControl *item = wxDynamicCast(FindItem(id), wxControl);
    if ( item )
    {
        return item->MSWOnMeasure(itemStruct);
    }
#else
    wxUnusedVar(id);
    wxUnusedVar(itemStruct);
#endif // wxUSE_OWNER_DRAWN && wxUSE_MENUS_NATIVE

    return false;
}

// ---------------------------------------------------------------------------
// colours and palettes
// ---------------------------------------------------------------------------

bool wxWindowMSW::HandleSysColorChange()
{
    wxSysColourChangedEvent event;
    event.SetEventObject(this);

    (void)GetEventHandler()->ProcessEvent(event);

    // always let the system carry on the default processing to allow the
    // native controls to react to the colours update
    return false;
}

bool wxWindowMSW::HandleDisplayChange()
{
    wxDisplayChangedEvent event;
    event.SetEventObject(this);

    return GetEventHandler()->ProcessEvent(event);
}

#ifndef __WXMICROWIN__

bool wxWindowMSW::HandleCtlColor(WXHBRUSH *brush, WXHDC hDC, WXHWND hWnd)
{
#if !wxUSE_CONTROLS || defined(__WXUNIVERSAL__)
    wxUnusedVar(hDC);
    wxUnusedVar(hWnd);
#else
    wxControl *item = wxDynamicCast(FindItemByHWND(hWnd, true), wxControl);

    if ( item )
        *brush = item->MSWControlColor(hDC, hWnd);
    else
#endif // wxUSE_CONTROLS
        *brush = NULL;

    return *brush != NULL;
}

#endif // __WXMICROWIN__

bool wxWindowMSW::HandlePaletteChanged(WXHWND hWndPalChange)
{
#if wxUSE_PALETTE
    // same as below except we don't respond to our own messages
    if ( hWndPalChange != GetHWND() )
    {
        // check to see if we our our parents have a custom palette
        wxWindowMSW *win = this;
        while ( win && !win->HasCustomPalette() )
        {
            win = win->GetParent();
        }

        if ( win && win->HasCustomPalette() )
        {
            // realize the palette to see whether redrawing is needed
            HDC hdc = ::GetDC((HWND) hWndPalChange);
            win->m_palette.SetHPALETTE((WXHPALETTE)
                    ::SelectPalette(hdc, GetHpaletteOf(win->m_palette), FALSE));

            int result = ::RealizePalette(hdc);

            // restore the palette (before releasing the DC)
            win->m_palette.SetHPALETTE((WXHPALETTE)
                    ::SelectPalette(hdc, GetHpaletteOf(win->m_palette), FALSE));
            ::RealizePalette(hdc);
            ::ReleaseDC((HWND) hWndPalChange, hdc);

            // now check for the need to redraw
            if (result > 0)
                ::InvalidateRect((HWND) hWndPalChange, NULL, TRUE);
        }

    }
#endif // wxUSE_PALETTE

    wxPaletteChangedEvent event(GetId());
    event.SetEventObject(this);
    event.SetChangedWindow(wxFindWinFromHandle(hWndPalChange));

    return GetEventHandler()->ProcessEvent(event);
}

bool wxWindowMSW::HandleCaptureChanged(WXHWND hWndGainedCapture)
{
    // notify windows on the capture stack about lost capture
    // (see http://sourceforge.net/tracker/index.php?func=detail&aid=1153662&group_id=9863&atid=109863):
    wxWindowBase::NotifyCaptureLost();

    wxWindow *win = wxFindWinFromHandle(hWndGainedCapture);
    wxMouseCaptureChangedEvent event(GetId(), win);
    event.SetEventObject(this);
    return GetEventHandler()->ProcessEvent(event);
}

bool wxWindowMSW::HandleSettingChange(WXWPARAM wParam, WXLPARAM lParam)
{
    // despite MSDN saying "(This message cannot be sent directly to a window.)"
    // we need to send this to child windows (it is only sent to top-level
    // windows) so {list,tree}ctrls can adjust their font size if necessary
    // this is exactly how explorer does it to enable the font size changes

    wxWindowList::compatibility_iterator node = GetChildren().GetFirst();
    while ( node )
    {
        // top-level windows already get this message from the system
        wxWindow *win = node->GetData();
        if ( !win->IsTopLevel() )
        {
            ::SendMessage(GetHwndOf(win), WM_SETTINGCHANGE, wParam, lParam);
        }

        node = node->GetNext();
    }

    // let the system handle it
    return false;
}

bool wxWindowMSW::HandleQueryNewPalette()
{

#if wxUSE_PALETTE
    // check to see if we our our parents have a custom palette
    wxWindowMSW *win = this;
    while (!win->HasCustomPalette() && win->GetParent()) win = win->GetParent();
    if (win->HasCustomPalette()) {
        /* realize the palette to see whether redrawing is needed */
        HDC hdc = ::GetDC((HWND) GetHWND());
        win->m_palette.SetHPALETTE( (WXHPALETTE)
             ::SelectPalette(hdc, (HPALETTE) win->m_palette.GetHPALETTE(), FALSE) );

        int result = ::RealizePalette(hdc);
        /* restore the palette (before releasing the DC) */
        win->m_palette.SetHPALETTE( (WXHPALETTE)
             ::SelectPalette(hdc, (HPALETTE) win->m_palette.GetHPALETTE(), TRUE) );
        ::RealizePalette(hdc);
        ::ReleaseDC((HWND) GetHWND(), hdc);
        /* now check for the need to redraw */
        if (result > 0)
            ::InvalidateRect((HWND) GetHWND(), NULL, TRUE);
        }
#endif // wxUSE_PALETTE

    wxQueryNewPaletteEvent event(GetId());
    event.SetEventObject(this);

    return GetEventHandler()->ProcessEvent(event) && event.GetPaletteRealized();
}

// Responds to colour changes: passes event on to children.
void wxWindowMSW::OnSysColourChanged(wxSysColourChangedEvent& WXUNUSED(event))
{
    // the top level window also reset the standard colour map as it might have
    // changed (there is no need to do it for the non top level windows as we
    // only have to do it once)
    if ( IsTopLevel() )
    {
        // FIXME-MT
        gs_hasStdCmap = false;
    }
    wxWindowList::compatibility_iterator node = GetChildren().GetFirst();
    while ( node )
    {
        // Only propagate to non-top-level windows because Windows already
        // sends this event to all top-level ones
        wxWindow *win = node->GetData();
        if ( !win->IsTopLevel() )
        {
            // we need to send the real WM_SYSCOLORCHANGE and not just trigger
            // EVT_SYS_COLOUR_CHANGED call because the latter wouldn't work for
            // the standard controls
            ::SendMessage(GetHwndOf(win), WM_SYSCOLORCHANGE, 0, 0);
        }

        node = node->GetNext();
    }
}

extern wxCOLORMAP *wxGetStdColourMap()
{
    static COLORREF s_stdColours[wxSTD_COL_MAX];
    static wxCOLORMAP s_cmap[wxSTD_COL_MAX];

    if ( !gs_hasStdCmap )
    {
        static bool s_coloursInit = false;

        if ( !s_coloursInit )
        {
            // When a bitmap is loaded, the RGB values can change (apparently
            // because Windows adjusts them to care for the old programs always
            // using 0xc0c0c0 while the transparent colour for the new Windows
            // versions is different). But we do this adjustment ourselves so
            // we want to avoid Windows' "help" and for this we need to have a
            // reference bitmap which can tell us what the RGB values change
            // to.
            wxLogNull logNo; // suppress error if we couldn't load the bitmap
            wxBitmap stdColourBitmap(_T("wxBITMAP_STD_COLOURS"));
            if ( stdColourBitmap.Ok() )
            {
                // the pixels in the bitmap must correspond to wxSTD_COL_XXX!
                wxASSERT_MSG( stdColourBitmap.GetWidth() == wxSTD_COL_MAX,
                              _T("forgot to update wxBITMAP_STD_COLOURS!") );

                wxMemoryDC memDC;
                memDC.SelectObject(stdColourBitmap);

                wxColour colour;
                for ( size_t i = 0; i < WXSIZEOF(s_stdColours); i++ )
                {
                    memDC.GetPixel(i, 0, &colour);
                    s_stdColours[i] = wxColourToRGB(colour);
                }
            }
            else // wxBITMAP_STD_COLOURS couldn't be loaded
            {
                s_stdColours[0] = RGB(000,000,000);     // black
                s_stdColours[1] = RGB(128,128,128);     // dark grey
                s_stdColours[2] = RGB(192,192,192);     // light grey
                s_stdColours[3] = RGB(255,255,255);     // white
                //s_stdColours[4] = RGB(000,000,255);     // blue
                //s_stdColours[5] = RGB(255,000,255);     // magenta
            }

            s_coloursInit = true;
        }

        gs_hasStdCmap = true;

        // create the colour map
#define INIT_CMAP_ENTRY(col) \
            s_cmap[wxSTD_COL_##col].from = s_stdColours[wxSTD_COL_##col]; \
            s_cmap[wxSTD_COL_##col].to = ::GetSysColor(COLOR_##col)

        INIT_CMAP_ENTRY(BTNTEXT);
        INIT_CMAP_ENTRY(BTNSHADOW);
        INIT_CMAP_ENTRY(BTNFACE);
        INIT_CMAP_ENTRY(BTNHIGHLIGHT);

#undef INIT_CMAP_ENTRY
    }

    return s_cmap;
}

// ---------------------------------------------------------------------------
// painting
// ---------------------------------------------------------------------------

bool wxWindowMSW::HandlePaint()
{
    HRGN hRegion = ::CreateRectRgn(0, 0, 0, 0); // Dummy call to get a handle
    if ( !hRegion )
        wxLogLastError(wxT("CreateRectRgn"));
    if ( ::GetUpdateRgn(GetHwnd(), hRegion, FALSE) == ERROR )
        wxLogLastError(wxT("GetUpdateRgn"));

    m_updateRegion = wxRegion((WXHRGN) hRegion);

    wxPaintEvent event(m_windowId);
    event.SetEventObject(this);

    bool processed = GetEventHandler()->ProcessEvent(event);

    // note that we must generate NC event after the normal one as otherwise
    // BeginPaint() will happily overwrite our decorations with the background
    // colour
    wxNcPaintEvent eventNc(m_windowId);
    eventNc.SetEventObject(this);
    GetEventHandler()->ProcessEvent(eventNc);

    // don't keep an HRGN we don't need any longer (GetUpdateRegion() can only
    // be called from inside the event handlers called above)
    m_updateRegion.Clear();

    return processed;
}

// Can be called from an application's OnPaint handler
void wxWindowMSW::OnPaint(wxPaintEvent& event)
{
#ifdef __WXUNIVERSAL__
    event.Skip();
#else
    HDC hDC = (HDC) wxPaintDC::FindDCInCache((wxWindow*) event.GetEventObject());
    if (hDC != 0)
    {
        MSWDefWindowProc(WM_PAINT, (WPARAM) hDC, 0);
    }
#endif
}

bool wxWindowMSW::HandleEraseBkgnd(WXHDC hdc)
{
    wxDCTemp dc(hdc, GetClientSize());

    dc.SetHDC(hdc);
    dc.SetWindow((wxWindow *)this);

    wxEraseEvent event(m_windowId, &dc);
    event.SetEventObject(this);
    bool rc = GetEventHandler()->ProcessEvent(event);

    // must be called manually as ~wxDC doesn't do anything for wxDCTemp
    dc.SelectOldObjects(hdc);

    return rc;
}

void wxWindowMSW::OnEraseBackground(wxEraseEvent& event)
{
    // standard non top level controls (i.e. except the dialogs) always erase
    // their background themselves in HandleCtlColor() or have some control-
    // specific ways to set the colours (common controls)
    if ( IsOfStandardClass() && !IsTopLevel() )
    {
        event.Skip();
        return;
    }

    if ( GetBackgroundStyle() == wxBG_STYLE_CUSTOM )
    {
        // don't skip the event here, custom background means that the app
        // is drawing it itself in its OnPaint(), so don't draw it at all
        // now to avoid flicker
        return;
    }


    // do default background painting
    if ( !DoEraseBackground(GetHdcOf(*event.GetDC())) )
    {
        // let the system paint the background
        event.Skip();
    }
}

bool wxWindowMSW::DoEraseBackground(WXHDC hDC)
{
    HBRUSH hbr = (HBRUSH)MSWGetBgBrush(hDC);
    if ( !hbr )
        return false;

    wxFillRect(GetHwnd(), (HDC)hDC, hbr);

    return true;
}

WXHBRUSH
wxWindowMSW::MSWGetBgBrushForChild(WXHDC WXUNUSED(hDC), WXHWND hWnd)
{
    if ( m_hasBgCol )
    {
        // our background colour applies to:
        //  1. this window itself, always
        //  2. all children unless the colour is "not inheritable"
        //  3. even if it is not inheritable, our immediate transparent
        //     children should still inherit it -- but not any transparent
        //     children because it would look wrong if a child of non
        //     transparent child would show our bg colour when the child itself
        //     does not
        wxWindow *win = wxFindWinFromHandle(hWnd);
        if ( win == this ||
                m_inheritBgCol ||
                    (win && win->HasTransparentBackground() &&
                        win->GetParent() == this) )
        {
            // draw children with the same colour as the parent
            wxBrush *
                brush = wxTheBrushList->FindOrCreateBrush(GetBackgroundColour());

            return (WXHBRUSH)GetHbrushOf(*brush);
        }
    }

    return 0;
}

WXHBRUSH wxWindowMSW::MSWGetBgBrush(WXHDC hDC, WXHWND hWndToPaint)
{
    if ( !hWndToPaint )
        hWndToPaint = GetHWND();

    for ( wxWindowMSW *win = this; win; win = win->GetParent() )
    {
        WXHBRUSH hBrush = win->MSWGetBgBrushForChild(hDC, hWndToPaint);
        if ( hBrush )
            return hBrush;

        // background is not inherited beyond top level windows
        if ( win->IsTopLevel() )
            break;
    }

    return 0;
}

bool wxWindowMSW::HandlePrintClient(WXHDC hDC)
{
    // we receive this message when DrawThemeParentBackground() is
    // called from def window proc of several controls under XP and we
    // must draw properly themed background here
    //
    // note that naively I'd expect filling the client rect with the
    // brush returned by MSWGetBgBrush() work -- but for some reason it
    // doesn't and we have to call parents MSWPrintChild() which is
    // supposed to call DrawThemeBackground() with appropriate params
    //
    // also note that in this case lParam == PRF_CLIENT but we're
    // clearly expected to paint the background and nothing else!

    if ( IsTopLevel() || InheritsBackgroundColour() )
        return false;

    // sometimes we don't want the parent to handle it at all, instead
    // return whatever value this window wants
    if ( !MSWShouldPropagatePrintChild() )
        return MSWPrintChild(hDC, (wxWindow *)this);

    for ( wxWindow *win = GetParent(); win; win = win->GetParent() )
    {
        if ( win->MSWPrintChild(hDC, (wxWindow *)this) )
            return true;

        if ( win->IsTopLevel() || win->InheritsBackgroundColour() )
            break;
    }

    return false;
}

// ---------------------------------------------------------------------------
// moving and resizing
// ---------------------------------------------------------------------------

bool wxWindowMSW::HandleMinimize()
{
    wxIconizeEvent event(m_windowId);
    event.SetEventObject(this);

    return GetEventHandler()->ProcessEvent(event);
}

bool wxWindowMSW::HandleMaximize()
{
    wxMaximizeEvent event(m_windowId);
    event.SetEventObject(this);

    return GetEventHandler()->ProcessEvent(event);
}

bool wxWindowMSW::HandleMove(int x, int y)
{
    wxPoint point(x,y);
    wxMoveEvent event(point, m_windowId);
    event.SetEventObject(this);

    return GetEventHandler()->ProcessEvent(event);
}

bool wxWindowMSW::HandleMoving(wxRect& rect)
{
    wxMoveEvent event(rect, m_windowId);
    event.SetEventObject(this);

    bool rc = GetEventHandler()->ProcessEvent(event);
    if (rc)
        rect = event.GetRect();
    return rc;
}

bool wxWindowMSW::HandleSize(int WXUNUSED(w), int WXUNUSED(h), WXUINT wParam)
{
#if USE_DEFERRED_SIZING
    // when we resize this window, its children are probably going to be
    // repositioned as well, prepare to use DeferWindowPos() for them
    int numChildren = 0;
    for ( HWND child = ::GetWindow(GetHwndOf(this), GW_CHILD);
          child;
          child = ::GetWindow(child, GW_HWNDNEXT) )
    {
        numChildren ++;
    }

    // Protect against valid m_hDWP being overwritten
    bool useDefer = false;

    if ( numChildren > 1 )
    {
        if (!m_hDWP)
        {
            m_hDWP = (WXHANDLE)::BeginDeferWindowPos(numChildren);
            if ( !m_hDWP )
            {
                wxLogLastError(_T("BeginDeferWindowPos"));
            }
            if (m_hDWP)
                useDefer = true;
        }
    }
#endif // USE_DEFERRED_SIZING

    // update this window size
    bool processed = false;
    switch ( wParam )
    {
        default:
            wxFAIL_MSG( _T("unexpected WM_SIZE parameter") );
            // fall through nevertheless

        case SIZE_MAXHIDE:
        case SIZE_MAXSHOW:
            // we're not interested in these messages at all
            break;

        case SIZE_MINIMIZED:
            processed = HandleMinimize();
            break;

        case SIZE_MAXIMIZED:
            /* processed = */ HandleMaximize();
            // fall through to send a normal size event as well

        case SIZE_RESTORED:
            // don't use w and h parameters as they specify the client size
            // while according to the docs EVT_SIZE handler is supposed to
            // receive the total size
            wxSizeEvent event(GetSize(), m_windowId);
            event.SetEventObject(this);

            processed = GetEventHandler()->ProcessEvent(event);
    }

#if USE_DEFERRED_SIZING
    // and finally change the positions of all child windows at once
    if ( useDefer && m_hDWP )
    {
        // reset m_hDWP to NULL so that child windows don't try to use our
        // m_hDWP after we call EndDeferWindowPos() on it (this shouldn't
        // happen anyhow normally but who knows what weird flow of control we
        // may have depending on what the users EVT_SIZE handler does...)
        HDWP hDWP = (HDWP)m_hDWP;
        m_hDWP = NULL;

        // do put all child controls in place at once
        if ( !::EndDeferWindowPos(hDWP) )
        {
            wxLogLastError(_T("EndDeferWindowPos"));
        }

        // Reset our children's pending pos/size values.
        for ( wxWindowList::compatibility_iterator node = GetChildren().GetFirst();
              node;
              node = node->GetNext() )
        {
            wxWindowMSW *child = node->GetData();
            child->m_pendingPosition = wxDefaultPosition;
            child->m_pendingSize = wxDefaultSize;
        }
    }
#endif // USE_DEFERRED_SIZING

    return processed;
}

bool wxWindowMSW::HandleSizing(wxRect& rect)
{
    wxSizeEvent event(rect, m_windowId);
    event.SetEventObject(this);

    bool rc = GetEventHandler()->ProcessEvent(event);
    if (rc)
        rect = event.GetRect();
    return rc;
}

bool wxWindowMSW::HandleGetMinMaxInfo(void *WXUNUSED_IN_WINCE(mmInfo))
{
#ifdef __WXWINCE__
    return false;
#else
    MINMAXINFO *info = (MINMAXINFO *)mmInfo;

    bool rc = false;

    int minWidth = GetMinWidth(),
        minHeight = GetMinHeight(),
        maxWidth = GetMaxWidth(),
        maxHeight = GetMaxHeight();

    if ( minWidth != wxDefaultCoord )
    {
        info->ptMinTrackSize.x = minWidth;
        rc = true;
    }

    if ( minHeight != wxDefaultCoord )
    {
        info->ptMinTrackSize.y = minHeight;
        rc = true;
    }

    if ( maxWidth != wxDefaultCoord )
    {
        info->ptMaxTrackSize.x = maxWidth;
        rc = true;
    }

    if ( maxHeight != wxDefaultCoord )
    {
        info->ptMaxTrackSize.y = maxHeight;
        rc = true;
    }

    return rc;
#endif
}

// ---------------------------------------------------------------------------
// command messages
// ---------------------------------------------------------------------------

bool wxWindowMSW::HandleCommand(WXWORD id, WXWORD cmd, WXHWND control)
{
#if wxUSE_MENUS_NATIVE
    if ( !cmd && wxCurrentPopupMenu )
    {
        wxMenu *popupMenu = wxCurrentPopupMenu;
        wxCurrentPopupMenu = NULL;

        return popupMenu->MSWCommand(cmd, id);
    }
#endif // wxUSE_MENUS_NATIVE

    wxWindow *win = NULL;

    // first try to find it from HWND - this works even with the broken
    // programs using the same ids for different controls
    if ( control )
    {
        win = wxFindWinFromHandle(control);
    }

    // try the id
    if ( !win )
    {
        // must cast to a signed type before comparing with other ids!
        win = FindItem((signed short)id);
    }

    if ( win )
    {
        return win->MSWCommand(cmd, id);
    }

    // the messages sent from the in-place edit control used by the treectrl
    // for label editing have id == 0, but they should _not_ be treated as menu
    // messages (they are EN_XXX ones, in fact) so don't translate anything
    // coming from a control to wxEVT_COMMAND_MENU_SELECTED
    if ( !control )
    {
        // If no child window, it may be an accelerator, e.g. for a popup menu
        // command

        wxCommandEvent event(wxEVT_COMMAND_MENU_SELECTED);
        event.SetEventObject(this);
        event.SetId(id);
        event.SetInt(id);

        return GetEventHandler()->ProcessEvent(event);
    }
    else
    {
#if wxUSE_SPINCTRL && !defined(__WXUNIVERSAL__)
        // the text ctrl which is logically part of wxSpinCtrl sends WM_COMMAND
        // notifications to its parent which we want to reflect back to
        // wxSpinCtrl
        wxSpinCtrl *spin = wxSpinCtrl::GetSpinForTextCtrl(control);
        if ( spin && spin->ProcessTextCommand(cmd, id) )
            return true;
#endif // wxUSE_SPINCTRL

#if wxUSE_CHOICE && defined(__SMARTPHONE__)
        // the listbox ctrl which is logically part of wxChoice sends WM_COMMAND
        // notifications to its parent which we want to reflect back to
        // wxChoice
        wxChoice *choice = wxChoice::GetChoiceForListBox(control);
        if ( choice && choice->MSWCommand(cmd, id) )
            return true;
#endif
    }

    return false;
}

// ---------------------------------------------------------------------------
// mouse events
// ---------------------------------------------------------------------------

void wxWindowMSW::InitMouseEvent(wxMouseEvent& event,
                                 int x, int y,
                                 WXUINT flags)
{
    // our client coords are not quite the same as Windows ones
    wxPoint pt = GetClientAreaOrigin();
    event.m_x = x - pt.x;
    event.m_y = y - pt.y;

    event.m_shiftDown = (flags & MK_SHIFT) != 0;
    event.m_controlDown = (flags & MK_CONTROL) != 0;
    event.m_leftDown = (flags & MK_LBUTTON) != 0;
    event.m_middleDown = (flags & MK_MBUTTON) != 0;
    event.m_rightDown = (flags & MK_RBUTTON) != 0;
    event.m_altDown = ::GetKeyState(VK_MENU) < 0;

#ifndef __WXWINCE__
    event.SetTimestamp(::GetMessageTime());
#endif

    event.SetEventObject(this);
    event.SetId(GetId());

#if wxUSE_MOUSEEVENT_HACK
    gs_lastMouseEvent.pos = ClientToScreen(wxPoint(x, y));
    gs_lastMouseEvent.type = event.GetEventType();
#endif // wxUSE_MOUSEEVENT_HACK
}

#ifdef __WXWINCE__
// Windows doesn't send the mouse events to the static controls (which are
// transparent in the sense that their WM_NCHITTEST handler returns
// HTTRANSPARENT) at all but we want all controls to receive the mouse events
// and so we manually check if we don't have a child window under mouse and if
// we do, send the event to it instead of the window Windows had sent WM_XXX
// to.
//
// Notice that this is not done for the mouse move events because this could
// (would?) be too slow, but only for clicks which means that the static texts
// still don't get move, enter nor leave events.
static wxWindowMSW *FindWindowForMouseEvent(wxWindowMSW *win, int *x, int *y)
{
    wxCHECK_MSG( x && y, win, _T("NULL pointer in FindWindowForMouseEvent") );

    // first try to find a non transparent child: this allows us to send events
    // to a static text which is inside a static box, for example
    POINT pt = { *x, *y };
    HWND hwnd = GetHwndOf(win),
         hwndUnderMouse;

#ifdef __WXWINCE__
    hwndUnderMouse = ::ChildWindowFromPoint
                       (
                        hwnd,
                        pt
                       );
#else
    hwndUnderMouse = ::ChildWindowFromPointEx
                       (
                        hwnd,
                        pt,
                        CWP_SKIPINVISIBLE   |
                        CWP_SKIPDISABLED    |
                        CWP_SKIPTRANSPARENT
                       );
#endif

    if ( !hwndUnderMouse || hwndUnderMouse == hwnd )
    {
        // now try any child window at all
        hwndUnderMouse = ::ChildWindowFromPoint(hwnd, pt);
    }

    // check that we have a child window which is susceptible to receive mouse
    // events: for this it must be shown and enabled
    if ( hwndUnderMouse &&
            hwndUnderMouse != hwnd &&
                ::IsWindowVisible(hwndUnderMouse) &&
                    ::IsWindowEnabled(hwndUnderMouse) )
    {
        wxWindow *winUnderMouse = wxFindWinFromHandle((WXHWND)hwndUnderMouse);
        if ( winUnderMouse )
        {
            // translate the mouse coords to the other window coords
            win->ClientToScreen(x, y);
            winUnderMouse->ScreenToClient(x, y);

            win = winUnderMouse;
        }
    }

    return win;
}
#endif // __WXWINCE__

bool wxWindowMSW::HandleMouseEvent(WXUINT msg, int x, int y, WXUINT flags)
{
    // the mouse events take consecutive IDs from WM_MOUSEFIRST to
    // WM_MOUSELAST, so it's enough to subtract WM_MOUSEMOVE == WM_MOUSEFIRST
    // from the message id and take the value in the table to get wxWin event
    // id
    static const wxEventType eventsMouse[] =
    {
        wxEVT_MOTION,
        wxEVT_LEFT_DOWN,
        wxEVT_LEFT_UP,
        wxEVT_LEFT_DCLICK,
        wxEVT_RIGHT_DOWN,
        wxEVT_RIGHT_UP,
        wxEVT_RIGHT_DCLICK,
        wxEVT_MIDDLE_DOWN,
        wxEVT_MIDDLE_UP,
        wxEVT_MIDDLE_DCLICK
    };

    wxMouseEvent event(eventsMouse[msg - WM_MOUSEMOVE]);
    InitMouseEvent(event, x, y, flags);

    return GetEventHandler()->ProcessEvent(event);
}

bool wxWindowMSW::HandleMouseMove(int x, int y, WXUINT flags)
{
    if ( !m_mouseInWindow )
    {
        // it would be wrong to assume that just because we get a mouse move
        // event that the mouse is inside the window: although this is usually
        // true, it is not if we had captured the mouse, so we need to check
        // the mouse coordinates here
        if ( !HasCapture() || IsMouseInWindow() )
        {
            // Generate an ENTER event
            m_mouseInWindow = true;

#ifdef HAVE_TRACKMOUSEEVENT
            typedef BOOL (WINAPI *_TrackMouseEvent_t)(LPTRACKMOUSEEVENT);
#ifdef __WXWINCE__
            static const _TrackMouseEvent_t
                s_pfn_TrackMouseEvent = _TrackMouseEvent;
#else // !__WXWINCE__
            static _TrackMouseEvent_t s_pfn_TrackMouseEvent;
            static bool s_initDone = false;
            if ( !s_initDone )
            {
                // see comment in wxApp::GetComCtl32Version() explaining the
                // use of wxLoadedDLL
                wxLoadedDLL dllComCtl32(_T("comctl32.dll"));
                if ( dllComCtl32.IsLoaded() )
                {
                    s_pfn_TrackMouseEvent = (_TrackMouseEvent_t)
                        dllComCtl32.GetSymbol(_T("_TrackMouseEvent"));
                }

                s_initDone = true;
            }

            if ( s_pfn_TrackMouseEvent )
#endif // __WXWINCE__/!__WXWINCE__
            {
                WinStruct<TRACKMOUSEEVENT> trackinfo;

                trackinfo.dwFlags = TME_LEAVE;
                trackinfo.hwndTrack = GetHwnd();

                (*s_pfn_TrackMouseEvent)(&trackinfo);
            }
#endif // HAVE_TRACKMOUSEEVENT

            wxMouseEvent event(wxEVT_ENTER_WINDOW);
            InitMouseEvent(event, x, y, flags);

            (void)GetEventHandler()->ProcessEvent(event);
        }
    }
#ifdef HAVE_TRACKMOUSEEVENT
    else // mouse not in window
    {
        // Check if we need to send a LEAVE event
        // Windows doesn't send WM_MOUSELEAVE if the mouse has been captured so
        // send it here if we are using native mouse leave tracking
        if ( HasCapture() && !IsMouseInWindow() )
        {
            GenerateMouseLeave();
        }
    }
#endif // HAVE_TRACKMOUSEEVENT

#if wxUSE_MOUSEEVENT_HACK
    // Windows often generates mouse events even if mouse position hasn't
    // changed (http://article.gmane.org/gmane.comp.lib.wxwidgets.devel/66576)
    //
    // Filter this out as it can result in unexpected behaviour compared to
    // other platforms
    if ( gs_lastMouseEvent.type == wxEVT_RIGHT_DOWN ||
         gs_lastMouseEvent.type == wxEVT_LEFT_DOWN ||
         gs_lastMouseEvent.type == wxEVT_MIDDLE_DOWN ||
         gs_lastMouseEvent.type == wxEVT_MOTION )
    {
        if ( ClientToScreen(wxPoint(x, y)) == gs_lastMouseEvent.pos )
        {
            gs_lastMouseEvent.type = wxEVT_MOTION;

            return false;
        }
    }
#endif // wxUSE_MOUSEEVENT_HACK

    return HandleMouseEvent(WM_MOUSEMOVE, x, y, flags);
}


bool wxWindowMSW::HandleMouseWheel(WXWPARAM wParam, WXLPARAM lParam)
{
#if wxUSE_MOUSEWHEEL
    // notice that WM_MOUSEWHEEL position is in screen coords (as it's
    // forwarded up to the parent by DefWindowProc()) and not in the client
    // ones as all the other messages, translate them to the client coords for
    // consistency
    const wxPoint
        pt = ScreenToClient(wxPoint(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)));
    wxMouseEvent event(wxEVT_MOUSEWHEEL);
    InitMouseEvent(event, pt.x, pt.y, LOWORD(wParam));
    event.m_wheelRotation = (short)HIWORD(wParam);
    event.m_wheelDelta = WHEEL_DELTA;

    static int s_linesPerRotation = -1;
    if ( s_linesPerRotation == -1 )
    {
        if ( !::SystemParametersInfo(SPI_GETWHEELSCROLLLINES, 0,
                                     &s_linesPerRotation, 0))
        {
            // this is not supposed to happen
            wxLogLastError(_T("SystemParametersInfo(GETWHEELSCROLLLINES)"));

            // the default is 3, so use it if SystemParametersInfo() failed
            s_linesPerRotation = 3;
        }
    }

    event.m_linesPerAction = s_linesPerRotation;
    return GetEventHandler()->ProcessEvent(event);

#else // !wxUSE_MOUSEWHEEL
    wxUnusedVar(wParam);
    wxUnusedVar(lParam);

    return false;
#endif // wxUSE_MOUSEWHEEL/!wxUSE_MOUSEWHEEL
}

void wxWindowMSW::GenerateMouseLeave()
{
    m_mouseInWindow = false;

    int state = 0;
    if ( wxIsShiftDown() )
        state |= MK_SHIFT;
    if ( wxIsCtrlDown() )
        state |= MK_CONTROL;

    // Only the high-order bit should be tested
    if ( GetKeyState( VK_LBUTTON ) & (1<<15) )
        state |= MK_LBUTTON;
    if ( GetKeyState( VK_MBUTTON ) & (1<<15) )
        state |= MK_MBUTTON;
    if ( GetKeyState( VK_RBUTTON ) & (1<<15) )
        state |= MK_RBUTTON;

    POINT pt;
#ifdef __WXWINCE__
    if ( !::GetCursorPosWinCE(&pt) )
#else
    if ( !::GetCursorPos(&pt) )
#endif
    {
        wxLogLastError(_T("GetCursorPos"));
    }

    // we need to have client coordinates here for symmetry with
    // wxEVT_ENTER_WINDOW
    RECT rect = wxGetWindowRect(GetHwnd());
    pt.x -= rect.left;
    pt.y -= rect.top;

    wxMouseEvent event(wxEVT_LEAVE_WINDOW);
    InitMouseEvent(event, pt.x, pt.y, state);

    (void)GetEventHandler()->ProcessEvent(event);
}

// ---------------------------------------------------------------------------
// keyboard handling
// ---------------------------------------------------------------------------

// create the key event of the given type for the given key - used by
// HandleChar and HandleKeyDown/Up
wxKeyEvent wxWindowMSW::CreateKeyEvent(wxEventType evType,
                                       int id,
                                       WXLPARAM lParam,
                                       WXWPARAM wParam) const
{
    wxKeyEvent event(evType);
    event.SetId(GetId());
    event.m_shiftDown = wxIsShiftDown();
    event.m_controlDown = wxIsCtrlDown();
    event.m_altDown = (HIWORD(lParam) & KF_ALTDOWN) == KF_ALTDOWN;

    event.SetEventObject((wxWindow *)this); // const_cast
    event.m_keyCode = id;
#if wxUSE_UNICODE
    event.m_uniChar = (wxChar) wParam;
#endif
    event.m_rawCode = (wxUint32) wParam;
    event.m_rawFlags = (wxUint32) lParam;
#ifndef __WXWINCE__
    event.SetTimestamp(::GetMessageTime());
#endif

    // translate the position to client coords
    POINT pt;
#ifdef __WXWINCE__
    GetCursorPosWinCE(&pt);
#else
    GetCursorPos(&pt);
#endif
    RECT rect;
    GetWindowRect(GetHwnd(),&rect);
    pt.x -= rect.left;
    pt.y -= rect.top;

    event.m_x = pt.x;
    event.m_y = pt.y;

    return event;
}

// isASCII is true only when we're called from WM_CHAR handler and not from
// WM_KEYDOWN one
bool wxWindowMSW::HandleChar(WXWPARAM wParam, WXLPARAM lParam, bool isASCII)
{
    int id;
    if ( isASCII )
    {
        id = wParam;
    }
    else // we're called from WM_KEYDOWN
    {
        // don't pass lParam to wxCharCodeMSWToWX() here because we don't want
        // to get numpad key codes: CHAR events should use the logical keys
        // such as WXK_HOME instead of WXK_NUMPAD_HOME which is for KEY events
        id = wxCharCodeMSWToWX(wParam);
        if ( id == 0 )
        {
            // it's ASCII and will be processed here only when called from
            // WM_CHAR (i.e. when isASCII = true), don't process it now
            return false;
        }
    }

    wxKeyEvent event(CreateKeyEvent(wxEVT_CHAR, id, lParam, wParam));

    // the alphanumeric keys produced by pressing AltGr+something on European
    // keyboards have both Ctrl and Alt modifiers which may confuse the user
    // code as, normally, keys with Ctrl and/or Alt don't result in anything
    // alphanumeric, so pretend that there are no modifiers at all (the
    // KEY_DOWN event would still have the correct modifiers if they're really
    // needed)
    if ( event.m_controlDown && event.m_altDown &&
            (id >= 32 && id < 256) )
    {
        event.m_controlDown =
        event.m_altDown = false;
    }

    return GetEventHandler()->ProcessEvent(event);
}

bool wxWindowMSW::HandleKeyDown(WXWPARAM wParam, WXLPARAM lParam)
{
    int id = wxCharCodeMSWToWX(wParam, lParam);

    if ( !id )
    {
        // normal ASCII char
        id = wParam;
    }

    wxKeyEvent event(CreateKeyEvent(wxEVT_KEY_DOWN, id, lParam, wParam));
    return GetEventHandler()->ProcessEvent(event);
}

bool wxWindowMSW::HandleKeyUp(WXWPARAM wParam, WXLPARAM lParam)
{
    int id = wxCharCodeMSWToWX(wParam, lParam);

    if ( !id )
    {
        // normal ASCII char
        id = wParam;
    }

    wxKeyEvent event(CreateKeyEvent(wxEVT_KEY_UP, id, lParam, wParam));
    return GetEventHandler()->ProcessEvent(event);
}

int wxWindowMSW::HandleMenuChar(int WXUNUSED_IN_WINCE(chAccel),
                                WXLPARAM WXUNUSED_IN_WINCE(lParam))
{
    // FIXME: implement GetMenuItemCount for WinCE, possibly
    // in terms of GetMenuItemInfo
#ifndef __WXWINCE__
    const HMENU hmenu = (HMENU)lParam;

    MENUITEMINFO mii;
    wxZeroMemory(mii);
    mii.cbSize = sizeof(MENUITEMINFO);

    // we could use MIIM_FTYPE here as we only need to know if the item is
    // ownerdrawn or not and not dwTypeData which MIIM_TYPE also returns, but
    // MIIM_FTYPE is not supported under Win95
    mii.fMask = MIIM_TYPE | MIIM_DATA;

    // find if we have this letter in any owner drawn item
    const int count = ::GetMenuItemCount(hmenu);
    for ( int i = 0; i < count; i++ )
    {
        // previous loop iteration could modify it, reset it back before
        // calling GetMenuItemInfo() to prevent it from overflowing dwTypeData
        mii.cch = 0;

        if ( ::GetMenuItemInfo(hmenu, i, TRUE, &mii) )
        {
            if ( mii.fType == MFT_OWNERDRAW )
            {
                //  dwItemData member of the MENUITEMINFO is a
                //  pointer to the associated wxMenuItem -- see the
                //  menu creation code
                wxMenuItem *item = (wxMenuItem*)mii.dwItemData;

                const wxChar *p = wxStrchr(item->GetText(), _T('&'));
                while ( p++ )
                {
                    if ( *p == _T('&') )
                    {
                        // this is not the accel char, find the real one
                        p = wxStrchr(p + 1, _T('&'));
                    }
                    else // got the accel char
                    {
                        // FIXME-UNICODE: this comparison doesn't risk to work
                        // for non ASCII accelerator characters I'm afraid, but
                        // what can we do?
                        if ( (wchar_t)wxToupper(*p) == (wchar_t)chAccel )
                        {
                            return i;
                        }
                        else
                        {
                            // this one doesn't match
                            break;
                        }
                    }
                }
            }
        }
        else // failed to get the menu text?
        {
            // it's not fatal, so don't show error, but still log it
            wxLogLastError(_T("GetMenuItemInfo"));
        }
    }
#endif
    return wxNOT_FOUND;
}

bool wxWindowMSW::HandleClipboardEvent( WXUINT nMsg )
{
    const wxEventType type = ( nMsg == WM_CUT ) ? wxEVT_COMMAND_TEXT_CUT :
                             ( nMsg == WM_COPY ) ? wxEVT_COMMAND_TEXT_COPY :
                           /*( nMsg == WM_PASTE ) ? */ wxEVT_COMMAND_TEXT_PASTE;
    wxClipboardTextEvent evt(type, GetId());

    evt.SetEventObject(this);

    return GetEventHandler()->ProcessEvent(evt);
}

// ---------------------------------------------------------------------------
// joystick
// ---------------------------------------------------------------------------

bool wxWindowMSW::HandleJoystickEvent(WXUINT msg, int x, int y, WXUINT flags)
{
#ifdef JOY_BUTTON1
    int change = 0;
    if ( flags & JOY_BUTTON1CHG )
        change = wxJOY_BUTTON1;
    if ( flags & JOY_BUTTON2CHG )
        change = wxJOY_BUTTON2;
    if ( flags & JOY_BUTTON3CHG )
        change = wxJOY_BUTTON3;
    if ( flags & JOY_BUTTON4CHG )
        change = wxJOY_BUTTON4;

    int buttons = 0;
    if ( flags & JOY_BUTTON1 )
        buttons |= wxJOY_BUTTON1;
    if ( flags & JOY_BUTTON2 )
        buttons |= wxJOY_BUTTON2;
    if ( flags & JOY_BUTTON3 )
        buttons |= wxJOY_BUTTON3;
    if ( flags & JOY_BUTTON4 )
        buttons |= wxJOY_BUTTON4;

    // the event ids aren't consecutive so we can't use table based lookup
    int joystick;
    wxEventType eventType;
    switch ( msg )
    {
        case MM_JOY1MOVE:
            joystick = 1;
            eventType = wxEVT_JOY_MOVE;
            break;

        case MM_JOY2MOVE:
            joystick = 2;
            eventType = wxEVT_JOY_MOVE;
            break;

        case MM_JOY1ZMOVE:
            joystick = 1;
            eventType = wxEVT_JOY_ZMOVE;
            break;

        case MM_JOY2ZMOVE:
            joystick = 2;
            eventType = wxEVT_JOY_ZMOVE;
            break;

        case MM_JOY1BUTTONDOWN:
            joystick = 1;
            eventType = wxEVT_JOY_BUTTON_DOWN;
            break;

        case MM_JOY2BUTTONDOWN:
            joystick = 2;
            eventType = wxEVT_JOY_BUTTON_DOWN;
            break;

        case MM_JOY1BUTTONUP:
            joystick = 1;
            eventType = wxEVT_JOY_BUTTON_UP;
            break;

        case MM_JOY2BUTTONUP:
            joystick = 2;
            eventType = wxEVT_JOY_BUTTON_UP;
            break;

        default:
            wxFAIL_MSG(wxT("no such joystick event"));

            return false;
    }

    wxJoystickEvent event(eventType, buttons, joystick, change);
    event.SetPosition(wxPoint(x, y));
    event.SetEventObject(this);

    return GetEventHandler()->ProcessEvent(event);
#else
    wxUnusedVar(msg);
    wxUnusedVar(x);
    wxUnusedVar(y);
    wxUnusedVar(flags);
    return false;
#endif
}

// ---------------------------------------------------------------------------
// scrolling
// ---------------------------------------------------------------------------

bool wxWindowMSW::MSWOnScroll(int orientation, WXWORD wParam,
                              WXWORD pos, WXHWND control)
{
    if ( control && control != m_hWnd ) // Prevent infinite recursion
    {
        wxWindow *child = wxFindWinFromHandle(control);
        if ( child )
            return child->MSWOnScroll(orientation, wParam, pos, control);
    }

    wxScrollWinEvent event;
    event.SetPosition(pos);
    event.SetOrientation(orientation);
    event.SetEventObject(this);

    switch ( wParam )
    {
    case SB_TOP:
        event.SetEventType(wxEVT_SCROLLWIN_TOP);
        break;

    case SB_BOTTOM:
        event.SetEventType(wxEVT_SCROLLWIN_BOTTOM);
        break;

    case SB_LINEUP:
        event.SetEventType(wxEVT_SCROLLWIN_LINEUP);
        break;

    case SB_LINEDOWN:
        event.SetEventType(wxEVT_SCROLLWIN_LINEDOWN);
        break;

    case SB_PAGEUP:
        event.SetEventType(wxEVT_SCROLLWIN_PAGEUP);
        break;

    case SB_PAGEDOWN:
        event.SetEventType(wxEVT_SCROLLWIN_PAGEDOWN);
        break;

    case SB_THUMBPOSITION:
    case SB_THUMBTRACK:
        // under Win32, the scrollbar range and position are 32 bit integers,
        // but WM_[HV]SCROLL only carry the low 16 bits of them, so we must
        // explicitly query the scrollbar for the correct position (this must
        // be done only for these two SB_ events as they are the only one
        // carrying the scrollbar position)
        {
            WinStruct<SCROLLINFO> scrollInfo;
            scrollInfo.fMask = SIF_TRACKPOS;

            if ( !::GetScrollInfo(GetHwnd(),
                                  orientation == wxHORIZONTAL ? SB_HORZ
                                                              : SB_VERT,
                                  &scrollInfo) )
            {
                // Not necessarily an error, if there are no scrollbars yet.
                // wxLogLastError(_T("GetScrollInfo"));
            }

            event.SetPosition(scrollInfo.nTrackPos);
        }

        event.SetEventType( wParam == SB_THUMBPOSITION
                                ? wxEVT_SCROLLWIN_THUMBRELEASE
                                : wxEVT_SCROLLWIN_THUMBTRACK );
        break;

    default:
        return false;
    }

    return GetEventHandler()->ProcessEvent(event);
}

// ===========================================================================
// global functions
// ===========================================================================

void wxGetCharSize(WXHWND wnd, int *x, int *y, const wxFont& the_font)
{
    TEXTMETRIC tm;
    HDC dc = ::GetDC((HWND) wnd);
    HFONT was = 0;

    //    the_font.UseResource();
    //    the_font.RealizeResource();
    HFONT fnt = (HFONT)the_font.GetResourceHandle(); // const_cast
    if ( fnt )
        was = (HFONT) SelectObject(dc,fnt);

    GetTextMetrics(dc, &tm);
    if ( fnt && was )
    {
        SelectObject(dc,was);
    }
    ReleaseDC((HWND)wnd, dc);

    if ( x )
        *x = tm.tmAveCharWidth;
    if ( y )
        *y = tm.tmHeight + tm.tmExternalLeading;

    //   the_font.ReleaseResource();
}

// use the "extended" bit (24) of lParam to distinguish extended keys
// from normal keys as the same key is sent
static inline
int ChooseNormalOrExtended(int lParam, int keyNormal, int keyExtended)
{
    // except that if lParam is 0, it means we don't have real lParam from
    // WM_KEYDOWN but are just translating just a VK constant (e.g. done from
    // msw/treectrl.cpp when processing TVN_KEYDOWN) -- then assume this is a
    // non-numpad (hence extended) key as this is a more common case
    return !lParam || (lParam & (1 << 24)) ? keyExtended : keyNormal;
}

// this array contains the Windows virtual key codes which map one to one to
// WXK_xxx constants and is used in wxCharCodeMSWToWX/WXToMSW() below
//
// note that keys having a normal and numpad version (e.g. WXK_HOME and
// WXK_NUMPAD_HOME) are not included in this table as the mapping is not 1-to-1
static const struct wxKeyMapping
{
    int vk;
    wxKeyCode wxk;
} gs_specialKeys[] =
{
    { VK_CANCEL,        WXK_CANCEL },
    { VK_BACK,          WXK_BACK },
    { VK_TAB,           WXK_TAB },
    { VK_CLEAR,         WXK_CLEAR },
    { VK_SHIFT,         WXK_SHIFT },
    { VK_CONTROL,       WXK_CONTROL },
    { VK_MENU ,         WXK_ALT },
    { VK_PAUSE,         WXK_PAUSE },
    { VK_CAPITAL,       WXK_CAPITAL },
    { VK_SPACE,         WXK_SPACE },
    { VK_ESCAPE,        WXK_ESCAPE },
    { VK_SELECT,        WXK_SELECT },
    { VK_PRINT,         WXK_PRINT },
    { VK_EXECUTE,       WXK_EXECUTE },
    { VK_SNAPSHOT,      WXK_SNAPSHOT },
    { VK_HELP,          WXK_HELP },

    { VK_NUMPAD0,       WXK_NUMPAD0 },
    { VK_NUMPAD1,       WXK_NUMPAD1 },
    { VK_NUMPAD2,       WXK_NUMPAD2 },
    { VK_NUMPAD3,       WXK_NUMPAD3 },
    { VK_NUMPAD4,       WXK_NUMPAD4 },
    { VK_NUMPAD5,       WXK_NUMPAD5 },
    { VK_NUMPAD6,       WXK_NUMPAD6 },
    { VK_NUMPAD7,       WXK_NUMPAD7 },
    { VK_NUMPAD8,       WXK_NUMPAD8 },
    { VK_NUMPAD9,       WXK_NUMPAD9 },
    { VK_MULTIPLY,      WXK_NUMPAD_MULTIPLY },
    { VK_ADD,           WXK_NUMPAD_ADD },
    { VK_SUBTRACT,      WXK_NUMPAD_SUBTRACT },
    { VK_DECIMAL,       WXK_NUMPAD_DECIMAL },
    { VK_DIVIDE,        WXK_NUMPAD_DIVIDE },

    { VK_F1,            WXK_F1 },
    { VK_F2,            WXK_F2 },
    { VK_F3,            WXK_F3 },
    { VK_F4,            WXK_F4 },
    { VK_F5,            WXK_F5 },
    { VK_F6,            WXK_F6 },
    { VK_F7,            WXK_F7 },
    { VK_F8,            WXK_F8 },
    { VK_F9,            WXK_F9 },
    { VK_F10,           WXK_F10 },
    { VK_F11,           WXK_F11 },
    { VK_F12,           WXK_F12 },
    { VK_F13,           WXK_F13 },
    { VK_F14,           WXK_F14 },
    { VK_F15,           WXK_F15 },
    { VK_F16,           WXK_F16 },
    { VK_F17,           WXK_F17 },
    { VK_F18,           WXK_F18 },
    { VK_F19,           WXK_F19 },
    { VK_F20,           WXK_F20 },
    { VK_F21,           WXK_F21 },
    { VK_F22,           WXK_F22 },
    { VK_F23,           WXK_F23 },
    { VK_F24,           WXK_F24 },

    { VK_NUMLOCK,       WXK_NUMLOCK },
    { VK_SCROLL,        WXK_SCROLL },

#ifdef VK_APPS
    { VK_LWIN,          WXK_WINDOWS_LEFT },
    { VK_RWIN,          WXK_WINDOWS_RIGHT },
    { VK_APPS,          WXK_WINDOWS_MENU },
#endif // VK_APPS defined
};

// Returns 0 if was a normal ASCII value, not a special key. This indicates that
// the key should be ignored by WM_KEYDOWN and processed by WM_CHAR instead.
int wxCharCodeMSWToWX(int vk, WXLPARAM lParam)
{
    // check the table first
    for ( size_t n = 0; n < WXSIZEOF(gs_specialKeys); n++ )
    {
        if ( gs_specialKeys[n].vk == vk )
            return gs_specialKeys[n].wxk;
    }

    // keys requiring special handling
    int wxk;
    switch ( vk )
    {
        // the mapping for these keys may be incorrect on non-US keyboards so
        // maybe we shouldn't map them to ASCII values at all
        case VK_OEM_1:      wxk = ';'; break;
        case VK_OEM_PLUS:   wxk = '+'; break;
        case VK_OEM_COMMA:  wxk = ','; break;
        case VK_OEM_MINUS:  wxk = '-'; break;
        case VK_OEM_PERIOD: wxk = '.'; break;
        case VK_OEM_2:      wxk = '/'; break;
        case VK_OEM_3:      wxk = '~'; break;
        case VK_OEM_4:      wxk = '['; break;
        case VK_OEM_5:      wxk = '\\'; break;
        case VK_OEM_6:      wxk = ']'; break;
        case VK_OEM_7:      wxk = '\''; break;

        // handle extended keys
        case VK_PRIOR:
            wxk = ChooseNormalOrExtended(lParam, WXK_NUMPAD_PAGEUP, WXK_PAGEUP);
            break;

        case VK_NEXT:
            wxk = ChooseNormalOrExtended(lParam, WXK_NUMPAD_PAGEDOWN, WXK_PAGEDOWN);
            break;

        case VK_END:
            wxk = ChooseNormalOrExtended(lParam, WXK_NUMPAD_END, WXK_END);
            break;

        case VK_HOME:
            wxk = ChooseNormalOrExtended(lParam, WXK_NUMPAD_HOME, WXK_HOME);
            break;

        case VK_LEFT:
            wxk = ChooseNormalOrExtended(lParam, WXK_NUMPAD_LEFT, WXK_LEFT);
            break;

        case VK_UP:
            wxk = ChooseNormalOrExtended(lParam, WXK_NUMPAD_UP, WXK_UP);
            break;

        case VK_RIGHT:
            wxk = ChooseNormalOrExtended(lParam, WXK_NUMPAD_RIGHT, WXK_RIGHT);
            break;

        case VK_DOWN:
            wxk = ChooseNormalOrExtended(lParam, WXK_NUMPAD_DOWN, WXK_DOWN);
            break;

        case VK_INSERT:
            wxk = ChooseNormalOrExtended(lParam, WXK_NUMPAD_INSERT, WXK_INSERT);
            break;

        case VK_DELETE:
            wxk = ChooseNormalOrExtended(lParam, WXK_NUMPAD_DELETE, WXK_DELETE);
            break;

        case VK_RETURN:
            // don't use ChooseNormalOrExtended() here as the keys are reversed
            // here: numpad enter is the extended one
            wxk = lParam && (lParam & (1 << 24)) ? WXK_NUMPAD_ENTER : WXK_RETURN;
            break;

        default:
            wxk = 0;
    }

    return wxk;
}

WXWORD wxCharCodeWXToMSW(int wxk, bool *isVirtual)
{
    if ( isVirtual )
        *isVirtual = true;

    // check the table first
    for ( size_t n = 0; n < WXSIZEOF(gs_specialKeys); n++ )
    {
        if ( gs_specialKeys[n].wxk == wxk )
            return gs_specialKeys[n].vk;
    }

    // and then check for special keys not included in the table
    WXWORD vk;
    switch ( wxk )
    {
        case WXK_PAGEUP:
        case WXK_NUMPAD_PAGEUP:
            vk = VK_PRIOR;
            break;

        case WXK_PAGEDOWN:
        case WXK_NUMPAD_PAGEDOWN:
            vk = VK_NEXT;
            break;

        case WXK_END:
        case WXK_NUMPAD_END:
            vk = VK_END;
            break;

        case WXK_HOME:
        case WXK_NUMPAD_HOME:
            vk = VK_HOME;
            break;

        case WXK_LEFT:
        case WXK_NUMPAD_LEFT:
            vk = VK_LEFT;
            break;

        case WXK_UP:
        case WXK_NUMPAD_UP:
            vk = VK_UP;
            break;

        case WXK_RIGHT:
        case WXK_NUMPAD_RIGHT:
            vk = VK_RIGHT;
            break;

        case WXK_DOWN:
        case WXK_NUMPAD_DOWN:
            vk = VK_DOWN;
            break;

        case WXK_INSERT:
        case WXK_NUMPAD_INSERT:
            vk = VK_INSERT;
            break;

        case WXK_DELETE:
        case WXK_NUMPAD_DELETE:
            vk = VK_DELETE;
            break;

        default:
#ifndef __WXWINCE__
            // check to see if its one of the OEM key codes.
            BYTE vks = LOBYTE(VkKeyScan(wxk));
            if ( vks != 0xff )
            {
                vk = vks;
            }
            else
#endif
            {
                if ( isVirtual )
                    *isVirtual = false;
                vk = (WXWORD)wxk;
            }
    }

    return vk;
}

#ifndef SM_SWAPBUTTON
    #define SM_SWAPBUTTON 23
#endif

// small helper for wxGetKeyState() and wxGetMouseState()
static inline bool wxIsKeyDown(WXWORD vk)
{
    switch (vk)
    {
        case VK_LBUTTON:
            if (GetSystemMetrics(SM_SWAPBUTTON)) vk = VK_RBUTTON;
            break;
        case VK_RBUTTON:
            if (GetSystemMetrics(SM_SWAPBUTTON)) vk = VK_LBUTTON;
            break;
    }
    // the low order bit indicates whether the key was pressed since the last
    // call and the high order one indicates whether it is down right now and
    // we only want that one
    return (GetAsyncKeyState(vk) & (1<<15)) != 0;
}

bool wxGetKeyState(wxKeyCode key)
{
    // although this does work under Windows, it is not supported under other
    // platforms so don't allow it, you must use wxGetMouseState() instead
    wxASSERT_MSG( key != VK_LBUTTON &&
                    key != VK_RBUTTON &&
                        key != VK_MBUTTON,
                    wxT("can't use wxGetKeyState() for mouse buttons") );

    const WXWORD vk = wxCharCodeWXToMSW(key);

    // if the requested key is a LED key, return true if the led is pressed
    if ( key == WXK_NUMLOCK || key == WXK_CAPITAL || key == WXK_SCROLL )
    {
        // low order bit means LED is highlighted and high order one means the
        // key is down; for compatibility with the other ports return true if
        // either one is set
        return GetKeyState(vk) != 0;

    }
    else // normal key
    {
        return wxIsKeyDown(vk);
    }
}


wxMouseState wxGetMouseState()
{
    wxMouseState ms;
    POINT pt;
    GetCursorPos( &pt );

    ms.SetX(pt.x);
    ms.SetY(pt.y);
    ms.SetLeftDown(wxIsKeyDown(VK_LBUTTON));
    ms.SetMiddleDown(wxIsKeyDown(VK_MBUTTON));
    ms.SetRightDown(wxIsKeyDown(VK_RBUTTON));

    ms.SetControlDown(wxIsKeyDown(VK_CONTROL));
    ms.SetShiftDown(wxIsKeyDown(VK_SHIFT));
    ms.SetAltDown(wxIsKeyDown(VK_MENU));
//    ms.SetMetaDown();

    return ms;
}


wxWindow *wxGetActiveWindow()
{
    HWND hWnd = GetActiveWindow();
    if ( hWnd != 0 )
    {
        return wxFindWinFromHandle((WXHWND) hWnd);
    }
    return NULL;
}

extern wxWindow *wxGetWindowFromHWND(WXHWND hWnd)
{
    HWND hwnd = (HWND)hWnd;

    // For a radiobutton, we get the radiobox from GWL_USERDATA (which is set
    // by code in msw/radiobox.cpp), for all the others we just search up the
    // window hierarchy
    wxWindow *win = (wxWindow *)NULL;
    if ( hwnd )
    {
        win = wxFindWinFromHandle((WXHWND)hwnd);
        if ( !win )
        {
#if wxUSE_RADIOBOX
            // native radiobuttons return DLGC_RADIOBUTTON here and for any
            // wxWindow class which overrides WM_GETDLGCODE processing to
            // do it as well, win would be already non NULL
            if ( ::SendMessage(hwnd, WM_GETDLGCODE, 0, 0) & DLGC_RADIOBUTTON )
            {
                win = (wxWindow *)wxGetWindowUserData(hwnd);
            }
            //else: it's a wxRadioButton, not a radiobutton from wxRadioBox
#endif // wxUSE_RADIOBOX

            // spin control text buddy window should be mapped to spin ctrl
            // itself so try it too
#if wxUSE_SPINCTRL && !defined(__WXUNIVERSAL__)
            if ( !win )
            {
                win = wxSpinCtrl::GetSpinForTextCtrl((WXHWND)hwnd);
            }
#endif // wxUSE_SPINCTRL
        }
    }

    while ( hwnd && !win )
    {
        // this is a really ugly hack needed to avoid mistakenly returning the
        // parent frame wxWindow for the find/replace modeless dialog HWND -
        // this, in turn, is needed to call IsDialogMessage() from
        // wxApp::ProcessMessage() as for this we must return NULL from here
        //
        // FIXME: this is clearly not the best way to do it but I think we'll
        //        need to change HWND <-> wxWindow code more heavily than I can
        //        do it now to fix it
#ifndef __WXMICROWIN__
        if ( ::GetWindow(hwnd, GW_OWNER) )
        {
            // it's a dialog box, don't go upwards
            break;
        }
#endif

        hwnd = ::GetParent(hwnd);
        win = wxFindWinFromHandle((WXHWND)hwnd);
    }

    return win;
}

#if !defined(__WXMICROWIN__) && !defined(__WXWINCE__)

// Windows keyboard hook. Allows interception of e.g. F1, ESCAPE
// in active frames and dialogs, regardless of where the focus is.
static HHOOK wxTheKeyboardHook = 0;
static FARPROC wxTheKeyboardHookProc = 0;
int APIENTRY _EXPORT
wxKeyboardHook(int nCode, WORD wParam, DWORD lParam);

void wxSetKeyboardHook(bool doIt)
{
    if ( doIt )
    {
        wxTheKeyboardHookProc = MakeProcInstance((FARPROC) wxKeyboardHook, wxGetInstance());
        wxTheKeyboardHook = SetWindowsHookEx(WH_KEYBOARD, (HOOKPROC) wxTheKeyboardHookProc, wxGetInstance(),

            GetCurrentThreadId()
        //      (DWORD)GetCurrentProcess()); // This is another possibility. Which is right?
            );
    }
    else
    {
        UnhookWindowsHookEx(wxTheKeyboardHook);
    }
}

int APIENTRY _EXPORT
wxKeyboardHook(int nCode, WORD wParam, DWORD lParam)
{
    DWORD hiWord = HIWORD(lParam);
    if ( nCode != HC_NOREMOVE && ((hiWord & KF_UP) == 0) )
    {
        int id = wxCharCodeMSWToWX(wParam, lParam);
        if ( id != 0 )
        {
            wxKeyEvent event(wxEVT_CHAR_HOOK);
            if ( (HIWORD(lParam) & KF_ALTDOWN) == KF_ALTDOWN )
                event.m_altDown = true;

            event.SetEventObject(NULL);
            event.m_keyCode = id;
            event.m_shiftDown = wxIsShiftDown();
            event.m_controlDown = wxIsCtrlDown();
#ifndef __WXWINCE__
            event.SetTimestamp(::GetMessageTime());
#endif
            wxWindow *win = wxGetActiveWindow();
            wxEvtHandler *handler;
            if ( win )
            {
                handler = win->GetEventHandler();
                event.SetId(win->GetId());
            }
            else
            {
                handler = wxTheApp;
                event.SetId(wxID_ANY);
            }

            if ( handler && handler->ProcessEvent(event) )
            {
                // processed
                return 1;
            }
        }
    }

    return (int)CallNextHookEx(wxTheKeyboardHook, nCode, wParam, lParam);
}

#endif // !__WXMICROWIN__

#ifdef __WXDEBUG__
const wxChar *wxGetMessageName(int message)
{
    switch ( message )
    {
        case 0x0000: return wxT("WM_NULL");
        case 0x0001: return wxT("WM_CREATE");
        case 0x0002: return wxT("WM_DESTROY");
        case 0x0003: return wxT("WM_MOVE");
        case 0x0005: return wxT("WM_SIZE");
        case 0x0006: return wxT("WM_ACTIVATE");
        case 0x0007: return wxT("WM_SETFOCUS");
        case 0x0008: return wxT("WM_KILLFOCUS");
        case 0x000A: return wxT("WM_ENABLE");
        case 0x000B: return wxT("WM_SETREDRAW");
        case 0x000C: return wxT("WM_SETTEXT");
        case 0x000D: return wxT("WM_GETTEXT");
        case 0x000E: return wxT("WM_GETTEXTLENGTH");
        case 0x000F: return wxT("WM_PAINT");
        case 0x0010: return wxT("WM_CLOSE");
        case 0x0011: return wxT("WM_QUERYENDSESSION");
        case 0x0012: return wxT("WM_QUIT");
        case 0x0013: return wxT("WM_QUERYOPEN");
        case 0x0014: return wxT("WM_ERASEBKGND");
        case 0x0015: return wxT("WM_SYSCOLORCHANGE");
        case 0x0016: return wxT("WM_ENDSESSION");
        case 0x0017: return wxT("WM_SYSTEMERROR");
        case 0x0018: return wxT("WM_SHOWWINDOW");
        case 0x0019: return wxT("WM_CTLCOLOR");
        case 0x001A: return wxT("WM_WININICHANGE");
        case 0x001B: return wxT("WM_DEVMODECHANGE");
        case 0x001C: return wxT("WM_ACTIVATEAPP");
        case 0x001D: return wxT("WM_FONTCHANGE");
        case 0x001E: return wxT("WM_TIMECHANGE");
        case 0x001F: return wxT("WM_CANCELMODE");
        case 0x0020: return wxT("WM_SETCURSOR");
        case 0x0021: return wxT("WM_MOUSEACTIVATE");
        case 0x0022: return wxT("WM_CHILDACTIVATE");
        case 0x0023: return wxT("WM_QUEUESYNC");
        case 0x0024: return wxT("WM_GETMINMAXINFO");
        case 0x0026: return wxT("WM_PAINTICON");
        case 0x0027: return wxT("WM_ICONERASEBKGND");
        case 0x0028: return wxT("WM_NEXTDLGCTL");
        case 0x002A: return wxT("WM_SPOOLERSTATUS");
        case 0x002B: return wxT("WM_DRAWITEM");
        case 0x002C: return wxT("WM_MEASUREITEM");
        case 0x002D: return wxT("WM_DELETEITEM");
        case 0x002E: return wxT("WM_VKEYTOITEM");
        case 0x002F: return wxT("WM_CHARTOITEM");
        case 0x0030: return wxT("WM_SETFONT");
        case 0x0031: return wxT("WM_GETFONT");
        case 0x0037: return wxT("WM_QUERYDRAGICON");
        case 0x0039: return wxT("WM_COMPAREITEM");
        case 0x0041: return wxT("WM_COMPACTING");
        case 0x0044: return wxT("WM_COMMNOTIFY");
        case 0x0046: return wxT("WM_WINDOWPOSCHANGING");
        case 0x0047: return wxT("WM_WINDOWPOSCHANGED");
        case 0x0048: return wxT("WM_POWER");

        case 0x004A: return wxT("WM_COPYDATA");
        case 0x004B: return wxT("WM_CANCELJOURNAL");
        case 0x004E: return wxT("WM_NOTIFY");
        case 0x0050: return wxT("WM_INPUTLANGCHANGEREQUEST");
        case 0x0051: return wxT("WM_INPUTLANGCHANGE");
        case 0x0052: return wxT("WM_TCARD");
        case 0x0053: return wxT("WM_HELP");
        case 0x0054: return wxT("WM_USERCHANGED");
        case 0x0055: return wxT("WM_NOTIFYFORMAT");
        case 0x007B: return wxT("WM_CONTEXTMENU");
        case 0x007C: return wxT("WM_STYLECHANGING");
        case 0x007D: return wxT("WM_STYLECHANGED");
        case 0x007E: return wxT("WM_DISPLAYCHANGE");
        case 0x007F: return wxT("WM_GETICON");
        case 0x0080: return wxT("WM_SETICON");

        case 0x0081: return wxT("WM_NCCREATE");
        case 0x0082: return wxT("WM_NCDESTROY");
        case 0x0083: return wxT("WM_NCCALCSIZE");
        case 0x0084: return wxT("WM_NCHITTEST");
        case 0x0085: return wxT("WM_NCPAINT");
        case 0x0086: return wxT("WM_NCACTIVATE");
        case 0x0087: return wxT("WM_GETDLGCODE");
        case 0x00A0: return wxT("WM_NCMOUSEMOVE");
        case 0x00A1: return wxT("WM_NCLBUTTONDOWN");
        case 0x00A2: return wxT("WM_NCLBUTTONUP");
        case 0x00A3: return wxT("WM_NCLBUTTONDBLCLK");
        case 0x00A4: return wxT("WM_NCRBUTTONDOWN");
        case 0x00A5: return wxT("WM_NCRBUTTONUP");
        case 0x00A6: return wxT("WM_NCRBUTTONDBLCLK");
        case 0x00A7: return wxT("WM_NCMBUTTONDOWN");
        case 0x00A8: return wxT("WM_NCMBUTTONUP");
        case 0x00A9: return wxT("WM_NCMBUTTONDBLCLK");
        case 0x0100: return wxT("WM_KEYDOWN");
        case 0x0101: return wxT("WM_KEYUP");
        case 0x0102: return wxT("WM_CHAR");
        case 0x0103: return wxT("WM_DEADCHAR");
        case 0x0104: return wxT("WM_SYSKEYDOWN");
        case 0x0105: return wxT("WM_SYSKEYUP");
        case 0x0106: return wxT("WM_SYSCHAR");
        case 0x0107: return wxT("WM_SYSDEADCHAR");
        case 0x0108: return wxT("WM_KEYLAST");

        case 0x010D: return wxT("WM_IME_STARTCOMPOSITION");
        case 0x010E: return wxT("WM_IME_ENDCOMPOSITION");
        case 0x010F: return wxT("WM_IME_COMPOSITION");

        case 0x0110: return wxT("WM_INITDIALOG");
        case 0x0111: return wxT("WM_COMMAND");
        case 0x0112: return wxT("WM_SYSCOMMAND");
        case 0x0113: return wxT("WM_TIMER");
        case 0x0114: return wxT("WM_HSCROLL");
        case 0x0115: return wxT("WM_VSCROLL");
        case 0x0116: return wxT("WM_INITMENU");
        case 0x0117: return wxT("WM_INITMENUPOPUP");
        case 0x011F: return wxT("WM_MENUSELECT");
        case 0x0120: return wxT("WM_MENUCHAR");
        case 0x0121: return wxT("WM_ENTERIDLE");
        case 0x0200: return wxT("WM_MOUSEMOVE");
        case 0x0201: return wxT("WM_LBUTTONDOWN");
        case 0x0202: return wxT("WM_LBUTTONUP");
        case 0x0203: return wxT("WM_LBUTTONDBLCLK");
        case 0x0204: return wxT("WM_RBUTTONDOWN");
        case 0x0205: return wxT("WM_RBUTTONUP");
        case 0x0206: return wxT("WM_RBUTTONDBLCLK");
        case 0x0207: return wxT("WM_MBUTTONDOWN");
        case 0x0208: return wxT("WM_MBUTTONUP");
        case 0x0209: return wxT("WM_MBUTTONDBLCLK");
        case 0x020A: return wxT("WM_MOUSEWHEEL");
        case 0x0210: return wxT("WM_PARENTNOTIFY");
        case 0x0211: return wxT("WM_ENTERMENULOOP");
        case 0x0212: return wxT("WM_EXITMENULOOP");

        case 0x0213: return wxT("WM_NEXTMENU");
        case 0x0214: return wxT("WM_SIZING");
        case 0x0215: return wxT("WM_CAPTURECHANGED");
        case 0x0216: return wxT("WM_MOVING");
        case 0x0218: return wxT("WM_POWERBROADCAST");
        case 0x0219: return wxT("WM_DEVICECHANGE");

        case 0x0220: return wxT("WM_MDICREATE");
        case 0x0221: return wxT("WM_MDIDESTROY");
        case 0x0222: return wxT("WM_MDIACTIVATE");
        case 0x0223: return wxT("WM_MDIRESTORE");
        case 0x0224: return wxT("WM_MDINEXT");
        case 0x0225: return wxT("WM_MDIMAXIMIZE");
        case 0x0226: return wxT("WM_MDITILE");
        case 0x0227: return wxT("WM_MDICASCADE");
        case 0x0228: return wxT("WM_MDIICONARRANGE");
        case 0x0229: return wxT("WM_MDIGETACTIVE");
        case 0x0230: return wxT("WM_MDISETMENU");
        case 0x0233: return wxT("WM_DROPFILES");

        case 0x0281: return wxT("WM_IME_SETCONTEXT");
        case 0x0282: return wxT("WM_IME_NOTIFY");
        case 0x0283: return wxT("WM_IME_CONTROL");
        case 0x0284: return wxT("WM_IME_COMPOSITIONFULL");
        case 0x0285: return wxT("WM_IME_SELECT");
        case 0x0286: return wxT("WM_IME_CHAR");
        case 0x0290: return wxT("WM_IME_KEYDOWN");
        case 0x0291: return wxT("WM_IME_KEYUP");

        case 0x0300: return wxT("WM_CUT");
        case 0x0301: return wxT("WM_COPY");
        case 0x0302: return wxT("WM_PASTE");
        case 0x0303: return wxT("WM_CLEAR");
        case 0x0304: return wxT("WM_UNDO");
        case 0x0305: return wxT("WM_RENDERFORMAT");
        case 0x0306: return wxT("WM_RENDERALLFORMATS");
        case 0x0307: return wxT("WM_DESTROYCLIPBOARD");
        case 0x0308: return wxT("WM_DRAWCLIPBOARD");
        case 0x0309: return wxT("WM_PAINTCLIPBOARD");
        case 0x030A: return wxT("WM_VSCROLLCLIPBOARD");
        case 0x030B: return wxT("WM_SIZECLIPBOARD");
        case 0x030C: return wxT("WM_ASKCBFORMATNAME");
        case 0x030D: return wxT("WM_CHANGECBCHAIN");
        case 0x030E: return wxT("WM_HSCROLLCLIPBOARD");
        case 0x030F: return wxT("WM_QUERYNEWPALETTE");
        case 0x0310: return wxT("WM_PALETTEISCHANGING");
        case 0x0311: return wxT("WM_PALETTECHANGED");
#if wxUSE_HOTKEY
        case 0x0312: return wxT("WM_HOTKEY");
#endif

        // common controls messages - although they're not strictly speaking
        // standard, it's nice to decode them nevertheless

        // listview
        case 0x1000 + 0: return wxT("LVM_GETBKCOLOR");
        case 0x1000 + 1: return wxT("LVM_SETBKCOLOR");
        case 0x1000 + 2: return wxT("LVM_GETIMAGELIST");
        case 0x1000 + 3: return wxT("LVM_SETIMAGELIST");
        case 0x1000 + 4: return wxT("LVM_GETITEMCOUNT");
        case 0x1000 + 5: return wxT("LVM_GETITEMA");
        case 0x1000 + 75: return wxT("LVM_GETITEMW");
        case 0x1000 + 6: return wxT("LVM_SETITEMA");
        case 0x1000 + 76: return wxT("LVM_SETITEMW");
        case 0x1000 + 7: return wxT("LVM_INSERTITEMA");
        case 0x1000 + 77: return wxT("LVM_INSERTITEMW");
        case 0x1000 + 8: return wxT("LVM_DELETEITEM");
        case 0x1000 + 9: return wxT("LVM_DELETEALLITEMS");
        case 0x1000 + 10: return wxT("LVM_GETCALLBACKMASK");
        case 0x1000 + 11: return wxT("LVM_SETCALLBACKMASK");
        case 0x1000 + 12: return wxT("LVM_GETNEXTITEM");
        case 0x1000 + 13: return wxT("LVM_FINDITEMA");
        case 0x1000 + 83: return wxT("LVM_FINDITEMW");
        case 0x1000 + 14: return wxT("LVM_GETITEMRECT");
        case 0x1000 + 15: return wxT("LVM_SETITEMPOSITION");
        case 0x1000 + 16: return wxT("LVM_GETITEMPOSITION");
        case 0x1000 + 17: return wxT("LVM_GETSTRINGWIDTHA");
        case 0x1000 + 87: return wxT("LVM_GETSTRINGWIDTHW");
        case 0x1000 + 18: return wxT("LVM_HITTEST");
        case 0x1000 + 19: return wxT("LVM_ENSUREVISIBLE");
        case 0x1000 + 20: return wxT("LVM_SCROLL");
        case 0x1000 + 21: return wxT("LVM_REDRAWITEMS");
        case 0x1000 + 22: return wxT("LVM_ARRANGE");
        case 0x1000 + 23: return wxT("LVM_EDITLABELA");
        case 0x1000 + 118: return wxT("LVM_EDITLABELW");
        case 0x1000 + 24: return wxT("LVM_GETEDITCONTROL");
        case 0x1000 + 25: return wxT("LVM_GETCOLUMNA");
        case 0x1000 + 95: return wxT("LVM_GETCOLUMNW");
        case 0x1000 + 26: return wxT("LVM_SETCOLUMNA");
        case 0x1000 + 96: return wxT("LVM_SETCOLUMNW");
        case 0x1000 + 27: return wxT("LVM_INSERTCOLUMNA");
        case 0x1000 + 97: return wxT("LVM_INSERTCOLUMNW");
        case 0x1000 + 28: return wxT("LVM_DELETECOLUMN");
        case 0x1000 + 29: return wxT("LVM_GETCOLUMNWIDTH");
        case 0x1000 + 30: return wxT("LVM_SETCOLUMNWIDTH");
        case 0x1000 + 31: return wxT("LVM_GETHEADER");
        case 0x1000 + 33: return wxT("LVM_CREATEDRAGIMAGE");
        case 0x1000 + 34: return wxT("LVM_GETVIEWRECT");
        case 0x1000 + 35: return wxT("LVM_GETTEXTCOLOR");
        case 0x1000 + 36: return wxT("LVM_SETTEXTCOLOR");
        case 0x1000 + 37: return wxT("LVM_GETTEXTBKCOLOR");
        case 0x1000 + 38: return wxT("LVM_SETTEXTBKCOLOR");
        case 0x1000 + 39: return wxT("LVM_GETTOPINDEX");
        case 0x1000 + 40: return wxT("LVM_GETCOUNTPERPAGE");
        case 0x1000 + 41: return wxT("LVM_GETORIGIN");
        case 0x1000 + 42: return wxT("LVM_UPDATE");
        case 0x1000 + 43: return wxT("LVM_SETITEMSTATE");
        case 0x1000 + 44: return wxT("LVM_GETITEMSTATE");
        case 0x1000 + 45: return wxT("LVM_GETITEMTEXTA");
        case 0x1000 + 115: return wxT("LVM_GETITEMTEXTW");
        case 0x1000 + 46: return wxT("LVM_SETITEMTEXTA");
        case 0x1000 + 116: return wxT("LVM_SETITEMTEXTW");
        case 0x1000 + 47: return wxT("LVM_SETITEMCOUNT");
        case 0x1000 + 48: return wxT("LVM_SORTITEMS");
        case 0x1000 + 49: return wxT("LVM_SETITEMPOSITION32");
        case 0x1000 + 50: return wxT("LVM_GETSELECTEDCOUNT");
        case 0x1000 + 51: return wxT("LVM_GETITEMSPACING");
        case 0x1000 + 52: return wxT("LVM_GETISEARCHSTRINGA");
        case 0x1000 + 117: return wxT("LVM_GETISEARCHSTRINGW");
        case 0x1000 + 53: return wxT("LVM_SETICONSPACING");
        case 0x1000 + 54: return wxT("LVM_SETEXTENDEDLISTVIEWSTYLE");
        case 0x1000 + 55: return wxT("LVM_GETEXTENDEDLISTVIEWSTYLE");
        case 0x1000 + 56: return wxT("LVM_GETSUBITEMRECT");
        case 0x1000 + 57: return wxT("LVM_SUBITEMHITTEST");
        case 0x1000 + 58: return wxT("LVM_SETCOLUMNORDERARRAY");
        case 0x1000 + 59: return wxT("LVM_GETCOLUMNORDERARRAY");
        case 0x1000 + 60: return wxT("LVM_SETHOTITEM");
        case 0x1000 + 61: return wxT("LVM_GETHOTITEM");
        case 0x1000 + 62: return wxT("LVM_SETHOTCURSOR");
        case 0x1000 + 63: return wxT("LVM_GETHOTCURSOR");
        case 0x1000 + 64: return wxT("LVM_APPROXIMATEVIEWRECT");
        case 0x1000 + 65: return wxT("LVM_SETWORKAREA");

        // tree view
        case 0x1100 + 0: return wxT("TVM_INSERTITEMA");
        case 0x1100 + 50: return wxT("TVM_INSERTITEMW");
        case 0x1100 + 1: return wxT("TVM_DELETEITEM");
        case 0x1100 + 2: return wxT("TVM_EXPAND");
        case 0x1100 + 4: return wxT("TVM_GETITEMRECT");
        case 0x1100 + 5: return wxT("TVM_GETCOUNT");
        case 0x1100 + 6: return wxT("TVM_GETINDENT");
        case 0x1100 + 7: return wxT("TVM_SETINDENT");
        case 0x1100 + 8: return wxT("TVM_GETIMAGELIST");
        case 0x1100 + 9: return wxT("TVM_SETIMAGELIST");
        case 0x1100 + 10: return wxT("TVM_GETNEXTITEM");
        case 0x1100 + 11: return wxT("TVM_SELECTITEM");
        case 0x1100 + 12: return wxT("TVM_GETITEMA");
        case 0x1100 + 62: return wxT("TVM_GETITEMW");
        case 0x1100 + 13: return wxT("TVM_SETITEMA");
        case 0x1100 + 63: return wxT("TVM_SETITEMW");
        case 0x1100 + 14: return wxT("TVM_EDITLABELA");
        case 0x1100 + 65: return wxT("TVM_EDITLABELW");
        case 0x1100 + 15: return wxT("TVM_GETEDITCONTROL");
        case 0x1100 + 16: return wxT("TVM_GETVISIBLECOUNT");
        case 0x1100 + 17: return wxT("TVM_HITTEST");
        case 0x1100 + 18: return wxT("TVM_CREATEDRAGIMAGE");
        case 0x1100 + 19: return wxT("TVM_SORTCHILDREN");
        case 0x1100 + 20: return wxT("TVM_ENSUREVISIBLE");
        case 0x1100 + 21: return wxT("TVM_SORTCHILDRENCB");
        case 0x1100 + 22: return wxT("TVM_ENDEDITLABELNOW");
        case 0x1100 + 23: return wxT("TVM_GETISEARCHSTRINGA");
        case 0x1100 + 64: return wxT("TVM_GETISEARCHSTRINGW");
        case 0x1100 + 24: return wxT("TVM_SETTOOLTIPS");
        case 0x1100 + 25: return wxT("TVM_GETTOOLTIPS");

        // header
        case 0x1200 + 0: return wxT("HDM_GETITEMCOUNT");
        case 0x1200 + 1: return wxT("HDM_INSERTITEMA");
        case 0x1200 + 10: return wxT("HDM_INSERTITEMW");
        case 0x1200 + 2: return wxT("HDM_DELETEITEM");
        case 0x1200 + 3: return wxT("HDM_GETITEMA");
        case 0x1200 + 11: return wxT("HDM_GETITEMW");
        case 0x1200 + 4: return wxT("HDM_SETITEMA");
        case 0x1200 + 12: return wxT("HDM_SETITEMW");
        case 0x1200 + 5: return wxT("HDM_LAYOUT");
        case 0x1200 + 6: return wxT("HDM_HITTEST");
        case 0x1200 + 7: return wxT("HDM_GETITEMRECT");
        case 0x1200 + 8: return wxT("HDM_SETIMAGELIST");
        case 0x1200 + 9: return wxT("HDM_GETIMAGELIST");
        case 0x1200 + 15: return wxT("HDM_ORDERTOINDEX");
        case 0x1200 + 16: return wxT("HDM_CREATEDRAGIMAGE");
        case 0x1200 + 17: return wxT("HDM_GETORDERARRAY");
        case 0x1200 + 18: return wxT("HDM_SETORDERARRAY");
        case 0x1200 + 19: return wxT("HDM_SETHOTDIVIDER");

        // tab control
        case 0x1300 + 2: return wxT("TCM_GETIMAGELIST");
        case 0x1300 + 3: return wxT("TCM_SETIMAGELIST");
        case 0x1300 + 4: return wxT("TCM_GETITEMCOUNT");
        case 0x1300 + 5: return wxT("TCM_GETITEMA");
        case 0x1300 + 60: return wxT("TCM_GETITEMW");
        case 0x1300 + 6: return wxT("TCM_SETITEMA");
        case 0x1300 + 61: return wxT("TCM_SETITEMW");
        case 0x1300 + 7: return wxT("TCM_INSERTITEMA");
        case 0x1300 + 62: return wxT("TCM_INSERTITEMW");
        case 0x1300 + 8: return wxT("TCM_DELETEITEM");
        case 0x1300 + 9: return wxT("TCM_DELETEALLITEMS");
        case 0x1300 + 10: return wxT("TCM_GETITEMRECT");
        case 0x1300 + 11: return wxT("TCM_GETCURSEL");
        case 0x1300 + 12: return wxT("TCM_SETCURSEL");
        case 0x1300 + 13: return wxT("TCM_HITTEST");
        case 0x1300 + 14: return wxT("TCM_SETITEMEXTRA");
        case 0x1300 + 40: return wxT("TCM_ADJUSTRECT");
        case 0x1300 + 41: return wxT("TCM_SETITEMSIZE");
        case 0x1300 + 42: return wxT("TCM_REMOVEIMAGE");
        case 0x1300 + 43: return wxT("TCM_SETPADDING");
        case 0x1300 + 44: return wxT("TCM_GETROWCOUNT");
        case 0x1300 + 45: return wxT("TCM_GETTOOLTIPS");
        case 0x1300 + 46: return wxT("TCM_SETTOOLTIPS");
        case 0x1300 + 47: return wxT("TCM_GETCURFOCUS");
        case 0x1300 + 48: return wxT("TCM_SETCURFOCUS");
        case 0x1300 + 49: return wxT("TCM_SETMINTABWIDTH");
        case 0x1300 + 50: return wxT("TCM_DESELECTALL");

        // toolbar
        case WM_USER+1: return wxT("TB_ENABLEBUTTON");
        case WM_USER+2: return wxT("TB_CHECKBUTTON");
        case WM_USER+3: return wxT("TB_PRESSBUTTON");
        case WM_USER+4: return wxT("TB_HIDEBUTTON");
        case WM_USER+5: return wxT("TB_INDETERMINATE");
        case WM_USER+9: return wxT("TB_ISBUTTONENABLED");
        case WM_USER+10: return wxT("TB_ISBUTTONCHECKED");
        case WM_USER+11: return wxT("TB_ISBUTTONPRESSED");
        case WM_USER+12: return wxT("TB_ISBUTTONHIDDEN");
        case WM_USER+13: return wxT("TB_ISBUTTONINDETERMINATE");
        case WM_USER+17: return wxT("TB_SETSTATE");
        case WM_USER+18: return wxT("TB_GETSTATE");
        case WM_USER+19: return wxT("TB_ADDBITMAP");
        case WM_USER+20: return wxT("TB_ADDBUTTONS");
        case WM_USER+21: return wxT("TB_INSERTBUTTON");
        case WM_USER+22: return wxT("TB_DELETEBUTTON");
        case WM_USER+23: return wxT("TB_GETBUTTON");
        case WM_USER+24: return wxT("TB_BUTTONCOUNT");
        case WM_USER+25: return wxT("TB_COMMANDTOINDEX");
        case WM_USER+26: return wxT("TB_SAVERESTOREA");
        case WM_USER+76: return wxT("TB_SAVERESTOREW");
        case WM_USER+27: return wxT("TB_CUSTOMIZE");
        case WM_USER+28: return wxT("TB_ADDSTRINGA");
        case WM_USER+77: return wxT("TB_ADDSTRINGW");
        case WM_USER+29: return wxT("TB_GETITEMRECT");
        case WM_USER+30: return wxT("TB_BUTTONSTRUCTSIZE");
        case WM_USER+31: return wxT("TB_SETBUTTONSIZE");
        case WM_USER+32: return wxT("TB_SETBITMAPSIZE");
        case WM_USER+33: return wxT("TB_AUTOSIZE");
        case WM_USER+35: return wxT("TB_GETTOOLTIPS");
        case WM_USER+36: return wxT("TB_SETTOOLTIPS");
        case WM_USER+37: return wxT("TB_SETPARENT");
        case WM_USER+39: return wxT("TB_SETROWS");
        case WM_USER+40: return wxT("TB_GETROWS");
        case WM_USER+42: return wxT("TB_SETCMDID");
        case WM_USER+43: return wxT("TB_CHANGEBITMAP");
        case WM_USER+44: return wxT("TB_GETBITMAP");
        case WM_USER+45: return wxT("TB_GETBUTTONTEXTA");
        case WM_USER+75: return wxT("TB_GETBUTTONTEXTW");
        case WM_USER+46: return wxT("TB_REPLACEBITMAP");
        case WM_USER+47: return wxT("TB_SETINDENT");
        case WM_USER+48: return wxT("TB_SETIMAGELIST");
        case WM_USER+49: return wxT("TB_GETIMAGELIST");
        case WM_USER+50: return wxT("TB_LOADIMAGES");
        case WM_USER+51: return wxT("TB_GETRECT");
        case WM_USER+52: return wxT("TB_SETHOTIMAGELIST");
        case WM_USER+53: return wxT("TB_GETHOTIMAGELIST");
        case WM_USER+54: return wxT("TB_SETDISABLEDIMAGELIST");
        case WM_USER+55: return wxT("TB_GETDISABLEDIMAGELIST");
        case WM_USER+56: return wxT("TB_SETSTYLE");
        case WM_USER+57: return wxT("TB_GETSTYLE");
        case WM_USER+58: return wxT("TB_GETBUTTONSIZE");
        case WM_USER+59: return wxT("TB_SETBUTTONWIDTH");
        case WM_USER+60: return wxT("TB_SETMAXTEXTROWS");
        case WM_USER+61: return wxT("TB_GETTEXTROWS");
        case WM_USER+41: return wxT("TB_GETBITMAPFLAGS");

        default:
            static wxString s_szBuf;
            s_szBuf.Printf(wxT("<unknown message = %d>"), message);
            return s_szBuf.c_str();
    }
}
#endif //__WXDEBUG__

static TEXTMETRIC wxGetTextMetrics(const wxWindowMSW *win)
{
    // prepare the DC
    TEXTMETRIC tm;
    HWND hwnd = GetHwndOf(win);
    HDC hdc = ::GetDC(hwnd);

#if !wxDIALOG_UNIT_COMPATIBILITY
    // and select the current font into it
    HFONT hfont = GetHfontOf(win->GetFont());
    if ( hfont )
    {
        hfont = (HFONT)::SelectObject(hdc, hfont);
    }
#endif

    // finally retrieve the text metrics from it
    GetTextMetrics(hdc, &tm);

#if !wxDIALOG_UNIT_COMPATIBILITY
    // and clean up
    if ( hfont )
    {
        (void)::SelectObject(hdc, hfont);
    }
#endif

    ::ReleaseDC(hwnd, hdc);

    return tm;
}

// Find the wxWindow at the current mouse position, returning the mouse
// position.
wxWindow* wxFindWindowAtPointer(wxPoint& pt)
{
    pt = wxGetMousePosition();
    return wxFindWindowAtPoint(pt);
}

wxWindow* wxFindWindowAtPoint(const wxPoint& pt)
{
    POINT pt2;
    pt2.x = pt.x;
    pt2.y = pt.y;

    HWND hWnd = ::WindowFromPoint(pt2);

    return wxGetWindowFromHWND((WXHWND)hWnd);
}

// Get the current mouse position.
wxPoint wxGetMousePosition()
{
    POINT pt;
#ifdef __WXWINCE__
    GetCursorPosWinCE(&pt);
#else
    GetCursorPos( & pt );
#endif

    return wxPoint(pt.x, pt.y);
}

#if wxUSE_HOTKEY

#if defined(__SMARTPHONE__) || defined(__POCKETPC__)
static void WinCEUnregisterHotKey(int modifiers, int id)
{
    // Register hotkeys for the hardware buttons
    HINSTANCE hCoreDll;
    typedef BOOL (WINAPI *UnregisterFunc1Proc)(UINT, UINT);

    UnregisterFunc1Proc procUnregisterFunc;
    hCoreDll = LoadLibrary(_T("coredll.dll"));
    if (hCoreDll)
    {
        procUnregisterFunc = (UnregisterFunc1Proc)GetProcAddress(hCoreDll, _T("UnregisterFunc1"));
        if (procUnregisterFunc)
            procUnregisterFunc(modifiers, id);
        FreeLibrary(hCoreDll);
    }
}
#endif

bool wxWindowMSW::RegisterHotKey(int hotkeyId, int modifiers, int keycode)
{
    UINT win_modifiers=0;
    if ( modifiers & wxMOD_ALT )
        win_modifiers |= MOD_ALT;
    if ( modifiers & wxMOD_SHIFT )
        win_modifiers |= MOD_SHIFT;
    if ( modifiers & wxMOD_CONTROL )
        win_modifiers |= MOD_CONTROL;
    if ( modifiers & wxMOD_WIN )
        win_modifiers |= MOD_WIN;

#if defined(__SMARTPHONE__) || defined(__POCKETPC__)
    // Required for PPC and Smartphone hardware buttons
    if (keycode >= WXK_SPECIAL1 && keycode <= WXK_SPECIAL20)
        WinCEUnregisterHotKey(win_modifiers, hotkeyId);
#endif

    if ( !::RegisterHotKey(GetHwnd(), hotkeyId, win_modifiers, keycode) )
    {
        wxLogLastError(_T("RegisterHotKey"));

        return false;
    }

    return true;
}

bool wxWindowMSW::UnregisterHotKey(int hotkeyId)
{
#if defined(__SMARTPHONE__) || defined(__POCKETPC__)
    WinCEUnregisterHotKey(MOD_WIN, hotkeyId);
#endif

    if ( !::UnregisterHotKey(GetHwnd(), hotkeyId) )
    {
        wxLogLastError(_T("UnregisterHotKey"));

        return false;
    }

    return true;
}

#if wxUSE_ACCEL

bool wxWindowMSW::HandleHotKey(WXWPARAM wParam, WXLPARAM lParam)
{
    int hotkeyId = wParam;
    int virtualKey = HIWORD(lParam);
    int win_modifiers = LOWORD(lParam);

    wxKeyEvent event(CreateKeyEvent(wxEVT_HOTKEY, virtualKey, wParam, lParam));
    event.SetId(hotkeyId);
    event.m_shiftDown = (win_modifiers & MOD_SHIFT) != 0;
    event.m_controlDown = (win_modifiers & MOD_CONTROL) != 0;
    event.m_altDown = (win_modifiers & MOD_ALT) != 0;
    event.m_metaDown = (win_modifiers & MOD_WIN) != 0;

    return GetEventHandler()->ProcessEvent(event);
}

#endif // wxUSE_ACCEL

#endif // wxUSE_HOTKEY

// Not tested under WinCE
#ifndef __WXWINCE__

// this class installs a message hook which really wakes up our idle processing
// each time a WM_NULL is received (wxWakeUpIdle does this), even if we're
// sitting inside a local modal loop (e.g. a menu is opened or scrollbar is
// being dragged or even inside ::MessageBox()) and so don't control message
// dispatching otherwise
class wxIdleWakeUpModule : public wxModule
{
public:
    virtual bool OnInit()
    {
        ms_hMsgHookProc = ::SetWindowsHookEx
                            (
                             WH_GETMESSAGE,
                             &wxIdleWakeUpModule::MsgHookProc,
                             NULL,
                             GetCurrentThreadId()
                            );

        if ( !ms_hMsgHookProc )
        {
            wxLogLastError(_T("SetWindowsHookEx(WH_GETMESSAGE)"));

            return false;
        }

        return true;
    }

    virtual void OnExit()
    {
        ::UnhookWindowsHookEx(wxIdleWakeUpModule::ms_hMsgHookProc);
    }

    static LRESULT CALLBACK MsgHookProc(int nCode, WPARAM wParam, LPARAM lParam)
    {
        MSG *msg = (MSG*)lParam;

        // only process the message if it is actually going to be removed from
        // the message queue, this prevents that the same event from being
        // processed multiple times if now someone just called PeekMessage()
        if ( msg->message == WM_NULL && wParam == PM_REMOVE )
        {
            wxTheApp->ProcessPendingEvents();
        }

        return CallNextHookEx(ms_hMsgHookProc, nCode, wParam, lParam);
    }

private:
    static HHOOK ms_hMsgHookProc;

    DECLARE_DYNAMIC_CLASS(wxIdleWakeUpModule)
};

HHOOK wxIdleWakeUpModule::ms_hMsgHookProc = 0;

IMPLEMENT_DYNAMIC_CLASS(wxIdleWakeUpModule, wxModule)

#endif // __WXWINCE__

#ifdef __WXWINCE__

#if wxUSE_STATBOX
static void wxAdjustZOrder(wxWindow* parent)
{
    if (parent->IsKindOf(CLASSINFO(wxStaticBox)))
    {
        // Set the z-order correctly
        SetWindowPos((HWND) parent->GetHWND(), HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE);
    }

    wxWindowList::compatibility_iterator current = parent->GetChildren().GetFirst();
    while (current)
    {
        wxWindow *childWin = current->GetData();
        wxAdjustZOrder(childWin);
        current = current->GetNext();
    }
}
#endif

// We need to adjust the z-order of static boxes in WinCE, to
// make 'contained' controls visible
void wxWindowMSW::OnInitDialog( wxInitDialogEvent& event )
{
#if wxUSE_STATBOX
    wxAdjustZOrder(this);
#endif

    event.Skip();
}
#endif
