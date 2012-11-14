/////////////////////////////////////////////////////////////////////////////
// Name:        src/msw/wince/tbarwce.cpp
// Purpose:     wxToolBar for Windows CE
// Author:      Julian Smart
// Modified by:
// Created:     2003-07-12
// RCS-ID:      $Id: tbarwce.cpp 61236 2009-06-28 17:01:27Z JS $
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

// Use the WinCE-specific toolbar only if we're either compiling
// with a WinCE earlier than 4, or we wish to emulate a PocketPC-style UI
#if wxUSE_TOOLBAR && wxUSE_TOOLBAR_NATIVE && (_WIN32_WCE < 400 || defined(__POCKETPC__) || defined(__SMARTPHONE__))

#include "wx/toolbar.h"

#ifndef WX_PRECOMP
    #include "wx/msw/wrapcctl.h" // include <commctrl.h> "properly"
    #include "wx/dynarray.h"
    #include "wx/frame.h"
    #include "wx/log.h"
    #include "wx/intl.h"
    #include "wx/settings.h"
    #include "wx/bitmap.h"
    #include "wx/dcmemory.h"
    #include "wx/control.h"
#endif

#if !defined(__GNUWIN32__) && !defined(__SALFORDC__)
    #include "malloc.h"
#endif

#include "wx/msw/private.h"
#include <windows.h>
#include <windowsx.h>
#include <tchar.h>
#include <ole2.h>
#include <shellapi.h>
#if defined(WINCE_WITHOUT_COMMANDBAR)
  #include <aygshell.h>
#endif
#include "wx/msw/wince/missing.h"

#include "wx/msw/winundef.h"

#if !defined(__SMARTPHONE__)

///////////// This implementation is for PocketPC.
///////////// See later for the Smartphone dummy toolbar class.

// ----------------------------------------------------------------------------
// Event table
// ----------------------------------------------------------------------------

IMPLEMENT_DYNAMIC_CLASS(wxToolMenuBar, wxToolBar)

BEGIN_EVENT_TABLE(wxToolMenuBar, wxToolBar)
END_EVENT_TABLE()

// ----------------------------------------------------------------------------
// private classes
// ----------------------------------------------------------------------------

class wxToolMenuBarTool : public wxToolBarToolBase
{
public:
    wxToolMenuBarTool(wxToolBar *tbar,
                  int id,
                  const wxString& label,
                  const wxBitmap& bmpNormal,
                  const wxBitmap& bmpDisabled,
                  wxItemKind kind,
                  wxObject *clientData,
                  const wxString& shortHelp,
                  const wxString& longHelp)
        : wxToolBarToolBase(tbar, id, label, bmpNormal, bmpDisabled, kind,
                            clientData, shortHelp, longHelp)
    {
        m_nSepCount = 0;
        m_bitmapIndex = -1;
    }

    wxToolMenuBarTool(wxToolBar *tbar, wxControl *control)
        : wxToolBarToolBase(tbar, control)
    {
        m_nSepCount = 1;
        m_bitmapIndex = -1;
    }

    virtual void SetLabel(const wxString& label)
    {
        if ( label == m_label )
            return;

        wxToolBarToolBase::SetLabel(label);

        // we need to update the label shown in the toolbar because it has a
        // pointer to the internal buffer of the old label
        //
        // TODO: use TB_SETBUTTONINFO
    }

    // set/get the number of separators which we use to cover the space used by
    // a control in the toolbar
    void SetSeparatorsCount(size_t count) { m_nSepCount = count; }
    size_t GetSeparatorsCount() const { return m_nSepCount; }

    void SetBitmapIndex(int idx) { m_bitmapIndex = idx; }
    int GetBitmapIndex() const { return m_bitmapIndex; }

private:
    size_t m_nSepCount;
    int m_bitmapIndex;
};


// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// wxToolBarTool
// ----------------------------------------------------------------------------

wxToolBarToolBase *wxToolMenuBar::CreateTool(int id,
                                         const wxString& label,
                                         const wxBitmap& bmpNormal,
                                         const wxBitmap& bmpDisabled,
                                         wxItemKind kind,
                                         wxObject *clientData,
                                         const wxString& shortHelp,
                                         const wxString& longHelp)
{
    return new wxToolMenuBarTool(this, id, label, bmpNormal, bmpDisabled, kind,
                             clientData, shortHelp, longHelp);
}

wxToolBarToolBase *wxToolMenuBar::CreateTool(wxControl *control)
{
    return new wxToolMenuBarTool(this, control);
}

// ----------------------------------------------------------------------------
// wxToolBar construction
// ----------------------------------------------------------------------------

void wxToolMenuBar::Init()
{
    wxToolBar::Init();

    m_nButtons = 0;
    m_menuBar = NULL;
}

bool wxToolMenuBar::Create(wxWindow *parent,
                       wxWindowID id,
                       const wxPoint& pos,
                       const wxSize& size,
                       long style,
                       const wxString& name,
                       wxMenuBar* menuBar)
{
    // common initialisation
    if ( !CreateControl(parent, id, pos, size, style, wxDefaultValidator, name) )
        return false;

    // MSW-specific initialisation
    if ( !MSWCreateToolbar(pos, size, menuBar) )
        return false;

    // set up the colors and fonts
    SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_MENUBAR));
    SetFont(wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT));

    return true;
}

bool wxToolMenuBar::MSWCreateToolbar(const wxPoint& WXUNUSED(pos), const wxSize& WXUNUSED(size), wxMenuBar* menuBar)
{
    SetMenuBar(menuBar);
    if (m_menuBar)
        m_menuBar->SetToolBar(this);

#if defined(WINCE_WITHOUT_COMMANDBAR)
    // Create the menubar.
    SHMENUBARINFO mbi;

    memset (&mbi, 0, sizeof (SHMENUBARINFO));
    mbi.cbSize     = sizeof (SHMENUBARINFO);
    mbi.hwndParent = (HWND) GetParent()->GetHWND();
#ifdef __SMARTPHONE__
    mbi.nToolBarId = 5002;
#else
    mbi.nToolBarId = 5000;
#endif
    mbi.nBmpId     = 0;
    mbi.cBmpImages = 0;
    mbi.dwFlags = 0 ; // SHCMBF_EMPTYBAR;
    mbi.hInstRes = wxGetInstance();

    if (!SHCreateMenuBar(&mbi))
    {
        wxFAIL_MSG( _T("SHCreateMenuBar failed") );
        return false;
    }

    SetHWND((WXHWND) mbi.hwndMB);
#else
    HWND hWnd = CommandBar_Create(wxGetInstance(), (HWND) GetParent()->GetHWND(), GetId());
    SetHWND((WXHWND) hWnd);
#endif

    // install wxWidgets window proc for this window
    SubclassWin(m_hWnd);

    if (menuBar)
        menuBar->Create();

    return true;
}

void wxToolMenuBar::Recreate()
{
    // TODO
}

wxToolMenuBar::~wxToolMenuBar()
{
    if (GetMenuBar())
        GetMenuBar()->SetToolBar(NULL);
}

// Return HMENU for the menu associated with the commandbar
WXHMENU wxToolMenuBar::GetHMenu()
{
#if defined(__HANDHELDPC__)
    return 0;
#else
    if (GetHWND())
    {
        return (WXHMENU) (HMENU)::SendMessage((HWND) GetHWND(), SHCMBM_GETMENU, (WPARAM)0, (LPARAM)0);
    }
    else
        return 0;
#endif
}

// ----------------------------------------------------------------------------
// adding/removing tools
// ----------------------------------------------------------------------------

bool wxToolMenuBar::DoInsertTool(size_t WXUNUSED(pos), wxToolBarToolBase *tool)
{
    // nothing special to do here - we really create the toolbar buttons in
    // Realize() later
    tool->Attach(this);

    return true;
}

bool wxToolMenuBar::DoDeleteTool(size_t pos, wxToolBarToolBase *tool)
{
    // Skip over the menus
    if (GetMenuBar())
        pos += GetMenuBar()->GetMenuCount();
        
    // the main difficulty we have here is with the controls in the toolbars:
    // as we (sometimes) use several separators to cover up the space used by
    // them, the indices are not the same for us and the toolbar

    // first determine the position of the first button to delete: it may be
    // different from pos if we use several separators to cover the space used
    // by a control
    wxToolBarToolsList::compatibility_iterator node;
    for ( node = m_tools.GetFirst(); node; node = node->GetNext() )
    {
        wxToolBarToolBase *tool2 = node->GetData();
        if ( tool2 == tool )
        {
            // let node point to the next node in the list
            node = node->GetNext();

            break;
        }

        if ( tool2->IsControl() )
        {
            pos += ((wxToolMenuBarTool *)tool2)->GetSeparatorsCount() - 1;
        }
    }

    // now determine the number of buttons to delete and the area taken by them
    size_t nButtonsToDelete = 1;

    // get the size of the button we're going to delete
    RECT r;
    if ( !::SendMessage(GetHwnd(), TB_GETITEMRECT, pos, (LPARAM)&r) )
    {
        wxLogLastError(_T("TB_GETITEMRECT"));
    }

    int width = r.right - r.left;

    if ( tool->IsControl() )
    {
        nButtonsToDelete = ((wxToolMenuBarTool *)tool)->GetSeparatorsCount();

        width *= nButtonsToDelete;
    }

    // do delete all buttons
    m_nButtons -= nButtonsToDelete;
    while ( nButtonsToDelete-- > 0 )
    {
        if ( !::SendMessage(GetHwnd(), TB_DELETEBUTTON, pos, 0) )
        {
            wxLogLastError(wxT("TB_DELETEBUTTON"));

            return false;
        }
    }

    tool->Detach();

    // and finally reposition all the controls after this button (the toolbar
    // takes care of all normal items)
    for ( /* node -> first after deleted */ ; node; node = node->GetNext() )
    {
        wxToolBarToolBase *tool2 = node->GetData();
        if ( tool2->IsControl() )
        {
            int x;
            wxControl *control = tool2->GetControl();
            control->GetPosition(&x, NULL);
            control->Move(x - width, wxDefaultCoord);
        }
    }

    return true;
}

bool wxToolMenuBar::Realize()
{
    const size_t nTools = GetToolsCount();
    if ( nTools == 0 )
    {
        // nothing to do
        return true;
    }

#if 0
    // delete all old buttons, if any
    for ( size_t pos = 0; pos < m_nButtons; pos++ )
    {
        if ( !::SendMessage(GetHwnd(), TB_DELETEBUTTON, 0, 0) )
        {
            wxLogDebug(wxT("TB_DELETEBUTTON failed"));
        }
    }
#endif // 0

    bool lastWasRadio = false;
    wxToolBarToolsList::Node* node;
    for ( node = m_tools.GetFirst(); node; node = node->GetNext() )
    {
        wxToolMenuBarTool *tool = (wxToolMenuBarTool*) node->GetData();

        TBBUTTON buttons[1] ;

        TBBUTTON& button = buttons[0];

        wxZeroMemory(button);

        bool isRadio = false;
        switch ( tool->GetStyle() )
        {
            case wxTOOL_STYLE_CONTROL:
                button.idCommand = tool->GetId();
                // fall through: create just a separator too
                // TODO: controls are not yet supported on wxToolMenuBar.

            case wxTOOL_STYLE_SEPARATOR:
                button.fsState = TBSTATE_ENABLED;
                button.fsStyle = TBSTYLE_SEP;
                break;

            case wxTOOL_STYLE_BUTTON:

                if ( HasFlag(wxTB_TEXT) )
                {
                    const wxString& label = tool->GetLabel();
                    if ( !label.empty() )
                    {
                        button.iString = (int)label.c_str();
                    }
                }

                const wxBitmap& bmp = tool->GetNormalBitmap();

                wxBitmap bmpToUse = bmp;

                if (bmp.GetWidth() < 16 || bmp.GetHeight() < 16 || bmp.GetMask() != NULL)
                {
                    wxMemoryDC memDC;
                    wxBitmap b(16,16);
                    memDC.SelectObject(b);
                    wxColour col = wxColour(192,192,192);
                    memDC.SetBackground(wxBrush(col));
                    memDC.Clear();
                    int x = (16 - bmp.GetWidth())/2;
                    int y = (16 - bmp.GetHeight())/2;
                    memDC.DrawBitmap(bmp, x, y, true);
                    memDC.SelectObject(wxNullBitmap);

                    bmpToUse = b;
                    tool->SetNormalBitmap(b);
                }

                int n = 0;
                if ( bmpToUse.Ok() )
                {
                    n = ::CommandBar_AddBitmap( (HWND) GetHWND(), NULL, (int) (HBITMAP) bmpToUse.GetHBITMAP(),
                                                    1, 16, 16 );
                }

                button.idCommand = tool->GetId();
                button.iBitmap = n;

                if ( tool->IsEnabled() )
                    button.fsState |= TBSTATE_ENABLED;
                if ( tool->IsToggled() )
                    button.fsState |= TBSTATE_CHECKED;

                switch ( tool->GetKind() )
                {
                    case wxITEM_RADIO:
                        button.fsStyle = TBSTYLE_CHECKGROUP;

                        if ( !lastWasRadio )
                        {
                            // the first item in the radio group is checked by
                            // default to be consistent with wxGTK and the menu
                            // radio items
                            button.fsState |= TBSTATE_CHECKED;

                            tool->Toggle(true);
                        }

                        isRadio = true;
                        break;

                    case wxITEM_CHECK:
                        button.fsStyle = TBSTYLE_CHECK;
                        break;

                    default:
                        wxFAIL_MSG( _T("unexpected toolbar button kind") );
                        // fall through

                    case wxITEM_NORMAL:
                        button.fsStyle = TBSTYLE_BUTTON;
                }
                break;
        }

        if ( !::CommandBar_AddButtons( (HWND) GetHWND(), 1, buttons ) )
        {
            wxFAIL_MSG( wxT("Could not add toolbar button."));
        }

        lastWasRadio = isRadio;
    }

    return true;
}

bool wxToolMenuBar::MSWCommand(WXUINT WXUNUSED(cmd), WXWORD id)
{
    wxToolBarToolBase *tool = FindById((int)id);
    if ( !tool )
    {
        bool checked = false;
        if ( m_menuBar )
        {
            wxMenuItem *item = m_menuBar->FindItem(id);
            if ( item && item->IsCheckable() )
            {
                item->Toggle();
                checked = item->IsChecked();
            }
        }

        wxCommandEvent event(wxEVT_COMMAND_MENU_SELECTED);
        event.SetEventObject(this);
        event.SetId(id);
        event.SetInt(checked);

        return GetEventHandler()->ProcessEvent(event);
    }

    if ( tool->CanBeToggled() )
    {
        LRESULT state = ::SendMessage(GetHwnd(), TB_GETSTATE, id, 0);
        tool->Toggle((state & TBSTATE_CHECKED) != 0);
    }

    bool toggled = tool->IsToggled();

    // avoid sending the event when a radio button is released, this is not
    // interesting
    if ( !tool->CanBeToggled() || tool->GetKind() != wxITEM_RADIO || toggled )
    {
        // OnLeftClick() can veto the button state change - for buttons which
        // may be toggled only, of couse
        if ( !OnLeftClick((int)id, toggled) && tool->CanBeToggled() )
        {
            // revert back
            toggled = !toggled;
            tool->SetToggle(toggled);

            ::SendMessage(GetHwnd(), TB_CHECKBUTTON, id, MAKELONG(toggled, 0));
        }
    }

    return true;
}

WXLRESULT wxToolMenuBar::MSWWindowProc(WXUINT nMsg, WXWPARAM wParam, WXLPARAM lParam)
{
    switch ( nMsg )
    {
        case WM_SIZE:
            break;

        case WM_MOUSEMOVE:
            // we don't handle mouse moves, so always pass the message to
            // wxControl::MSWWindowProc
            HandleMouseMove(wParam, lParam);
            break;

        case WM_PAINT:
            break;
    }

    return MSWDefWindowProc(nMsg, wParam, lParam);
}


#else

////////////// For Smartphone

// ----------------------------------------------------------------------------
// Event table
// ----------------------------------------------------------------------------

IMPLEMENT_DYNAMIC_CLASS(wxToolBar, wxToolBarBase)

BEGIN_EVENT_TABLE(wxToolBar, wxToolBarBase)
END_EVENT_TABLE()

wxToolBarToolBase *wxToolBar::CreateTool(int id,
                                         const wxString& label,
                                         const wxBitmap& bmpNormal,
                                         const wxBitmap& bmpDisabled,
                                         wxItemKind kind,
                                         wxObject *clientData,
                                         const wxString& shortHelp,
                                         const wxString& longHelp)
{
    return new wxToolBarToolBase(this, id, label, bmpNormal, bmpDisabled, kind,
                             clientData, shortHelp, longHelp);
}

wxToolBarToolBase *wxToolBar::CreateTool(wxControl *control)
{
    return new wxToolBarToolBase(this, control);
}

bool wxToolBar::Create(wxWindow *parent,
                       wxWindowID WXUNUSED(id),
                       const wxPoint& WXUNUSED(pos),
                       const wxSize& WXUNUSED(size),
                       long style,
                       const wxString& name)
{
    // TODO: we may need to make this a dummy hidden window to
    // satisfy other parts of wxWidgets.

    parent->AddChild(this);

    SetWindowStyle(style);
    SetName(name);

    return true;
}

// ----------------------------------------------------------------------------
// adding/removing tools
// ----------------------------------------------------------------------------

bool wxToolBar::DoInsertTool(size_t WXUNUSED(pos), wxToolBarToolBase *tool)
{
    tool->Attach(this);
    return true;
}

bool wxToolBar::DoDeleteTool(size_t WXUNUSED(pos), wxToolBarToolBase *tool)
{
    tool->Detach();
    return true;
}

wxToolBarToolBase *wxToolBar::FindToolForPosition(wxCoord WXUNUSED(x), wxCoord WXUNUSED(y)) const
{
    return NULL;
}

// ----------------------------------------------------------------------------
// tool state
// ----------------------------------------------------------------------------

void wxToolBar::DoEnableTool(wxToolBarToolBase *WXUNUSED(tool), bool WXUNUSED(enable))
{
}

void wxToolBar::DoToggleTool(wxToolBarToolBase *WXUNUSED(tool), bool WXUNUSED(toggle))
{
}

void wxToolBar::DoSetToggle(wxToolBarToolBase *WXUNUSED(tool), bool WXUNUSED(toggle))
{
    wxFAIL_MSG( _T("not implemented") );
}

#endif
    // !__SMARTPHONE__

#endif // wxUSE_TOOLBAR
