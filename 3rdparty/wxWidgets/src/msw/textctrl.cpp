/////////////////////////////////////////////////////////////////////////////
// Name:        src/msw/textctrl.cpp
// Purpose:     wxTextCtrl
// Author:      Julian Smart
// Modified by:
// Created:     04/01/98
// RCS-ID:      $Id: textctrl.cpp 56394 2008-10-17 11:31:22Z VZ $
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

#if wxUSE_TEXTCTRL && !(defined(__SMARTPHONE__) && defined(__WXWINCE__))

#ifndef WX_PRECOMP
    #include "wx/textctrl.h"
    #include "wx/settings.h"
    #include "wx/brush.h"
    #include "wx/utils.h"
    #include "wx/intl.h"
    #include "wx/log.h"
    #include "wx/app.h"
    #include "wx/menu.h"
    #include "wx/math.h"
    #include "wx/module.h"
#endif

#include "wx/sysopt.h"

#if wxUSE_CLIPBOARD
    #include "wx/clipbrd.h"
#endif

#include "wx/textfile.h"

#include <windowsx.h>

#include "wx/msw/private.h"
#include "wx/msw/winundef.h"
#include "wx/msw/mslu.h"

#include <string.h>
#include <stdlib.h>

#ifndef __WXWINCE__
#include <sys/types.h>
#endif

#if wxUSE_RICHEDIT

#if wxUSE_INKEDIT
#include "wx/dynlib.h"
#endif

// old mingw32 has richedit stuff directly in windows.h and doesn't have
// richedit.h at all
#if !defined(__GNUWIN32_OLD__) || defined(__CYGWIN10__)
    #include <richedit.h>
#endif

#endif // wxUSE_RICHEDIT

#include "wx/msw/missing.h"

// ----------------------------------------------------------------------------
// private classes
// ----------------------------------------------------------------------------

#if wxUSE_RICHEDIT

// this module initializes RichEdit DLL(s) if needed
class wxRichEditModule : public wxModule
{
public:
    enum Version
    {
        Version_1,          // riched32.dll
        Version_2or3,       // both use riched20.dll
        Version_41,         // msftedit.dll (XP SP1 and Windows 2003)
        Version_Max
    };

    virtual bool OnInit();
    virtual void OnExit();

    // load the richedit DLL for the specified version of rich edit
    static bool Load(Version version);

#if wxUSE_INKEDIT
    // load the InkEdit library
    static bool LoadInkEdit();
#endif

private:
    // the handles to richedit 1.0 and 2.0 (or 3.0) DLLs
    static HINSTANCE ms_hRichEdit[Version_Max];

#if wxUSE_INKEDIT
    static wxDynamicLibrary ms_inkEditLib;
    static bool             ms_inkEditLibLoadAttemped;
#endif

    DECLARE_DYNAMIC_CLASS(wxRichEditModule)
};

HINSTANCE wxRichEditModule::ms_hRichEdit[Version_Max] = { NULL, NULL, NULL };

#if wxUSE_INKEDIT
wxDynamicLibrary wxRichEditModule::ms_inkEditLib;
bool             wxRichEditModule::ms_inkEditLibLoadAttemped = false;
#endif

IMPLEMENT_DYNAMIC_CLASS(wxRichEditModule, wxModule)

#endif // wxUSE_RICHEDIT

// a small class used to set m_updatesCount to 0 (to filter duplicate events if
// necessary) and to reset it back to -1 afterwards
class UpdatesCountFilter
{
public:
    UpdatesCountFilter(int& count)
        : m_count(count)
    {
        wxASSERT_MSG( m_count == -1 || m_count == -2,
                      _T("wrong initial m_updatesCount value") );

        if (m_count != -2)
            m_count = 0;
        //else: we don't want to count how many update events we get as we're going
        //      to ignore all of them
    }

    ~UpdatesCountFilter()
    {
        m_count = -1;
    }

    // return true if an event has been received
    bool GotUpdate() const
    {
        return m_count == 1;
    }

private:
    int& m_count;

    DECLARE_NO_COPY_CLASS(UpdatesCountFilter)
};

// ----------------------------------------------------------------------------
// event tables and other macros
// ----------------------------------------------------------------------------

#if wxUSE_EXTENDED_RTTI
WX_DEFINE_FLAGS( wxTextCtrlStyle )

wxBEGIN_FLAGS( wxTextCtrlStyle )
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

    wxFLAGS_MEMBER(wxTE_PROCESS_ENTER)
    wxFLAGS_MEMBER(wxTE_PROCESS_TAB)
    wxFLAGS_MEMBER(wxTE_MULTILINE)
    wxFLAGS_MEMBER(wxTE_PASSWORD)
    wxFLAGS_MEMBER(wxTE_READONLY)
    wxFLAGS_MEMBER(wxHSCROLL)
    wxFLAGS_MEMBER(wxTE_RICH)
    wxFLAGS_MEMBER(wxTE_RICH2)
    wxFLAGS_MEMBER(wxTE_AUTO_URL)
    wxFLAGS_MEMBER(wxTE_NOHIDESEL)
    wxFLAGS_MEMBER(wxTE_LEFT)
    wxFLAGS_MEMBER(wxTE_CENTRE)
    wxFLAGS_MEMBER(wxTE_RIGHT)
    wxFLAGS_MEMBER(wxTE_DONTWRAP)
    wxFLAGS_MEMBER(wxTE_CHARWRAP)
    wxFLAGS_MEMBER(wxTE_WORDWRAP)

wxEND_FLAGS( wxTextCtrlStyle )

IMPLEMENT_DYNAMIC_CLASS_XTI(wxTextCtrl, wxControl,"wx/textctrl.h")

wxBEGIN_PROPERTIES_TABLE(wxTextCtrl)
    wxEVENT_PROPERTY( TextUpdated , wxEVT_COMMAND_TEXT_UPDATED , wxCommandEvent )
    wxEVENT_PROPERTY( TextEnter , wxEVT_COMMAND_TEXT_ENTER , wxCommandEvent )

    wxPROPERTY( Font , wxFont , SetFont , GetFont  , EMPTY_MACROVALUE, 0 /*flags*/ , wxT("Helpstring") , wxT("group") )
    wxPROPERTY( Value , wxString , SetValue, GetValue, wxString() , 0 /*flags*/ , wxT("Helpstring") , wxT("group"))
    wxPROPERTY_FLAGS( WindowStyle , wxTextCtrlStyle , long , SetWindowStyleFlag , GetWindowStyleFlag , EMPTY_MACROVALUE , 0 /*flags*/ , wxT("Helpstring") , wxT("group")) // style
wxEND_PROPERTIES_TABLE()

wxBEGIN_HANDLERS_TABLE(wxTextCtrl)
wxEND_HANDLERS_TABLE()

wxCONSTRUCTOR_6( wxTextCtrl , wxWindow* , Parent , wxWindowID , Id , wxString , Value , wxPoint , Position , wxSize , Size , long , WindowStyle)
#else
IMPLEMENT_DYNAMIC_CLASS(wxTextCtrl, wxTextCtrlBase)
#endif


BEGIN_EVENT_TABLE(wxTextCtrl, wxTextCtrlBase)
    EVT_CHAR(wxTextCtrl::OnChar)
    EVT_KEY_DOWN(wxTextCtrl::OnKeyDown)
    EVT_DROP_FILES(wxTextCtrl::OnDropFiles)

#if wxUSE_RICHEDIT
    EVT_CONTEXT_MENU(wxTextCtrl::OnContextMenu)
#endif

    EVT_MENU(wxID_CUT, wxTextCtrl::OnCut)
    EVT_MENU(wxID_COPY, wxTextCtrl::OnCopy)
    EVT_MENU(wxID_PASTE, wxTextCtrl::OnPaste)
    EVT_MENU(wxID_UNDO, wxTextCtrl::OnUndo)
    EVT_MENU(wxID_REDO, wxTextCtrl::OnRedo)
    EVT_MENU(wxID_CLEAR, wxTextCtrl::OnDelete)
    EVT_MENU(wxID_SELECTALL, wxTextCtrl::OnSelectAll)

    EVT_UPDATE_UI(wxID_CUT, wxTextCtrl::OnUpdateCut)
    EVT_UPDATE_UI(wxID_COPY, wxTextCtrl::OnUpdateCopy)
    EVT_UPDATE_UI(wxID_PASTE, wxTextCtrl::OnUpdatePaste)
    EVT_UPDATE_UI(wxID_UNDO, wxTextCtrl::OnUpdateUndo)
    EVT_UPDATE_UI(wxID_REDO, wxTextCtrl::OnUpdateRedo)
    EVT_UPDATE_UI(wxID_CLEAR, wxTextCtrl::OnUpdateDelete)
    EVT_UPDATE_UI(wxID_SELECTALL, wxTextCtrl::OnUpdateSelectAll)

    EVT_SET_FOCUS(wxTextCtrl::OnSetFocus)
END_EVENT_TABLE()

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// creation
// ----------------------------------------------------------------------------

void wxTextCtrl::Init()
{
#if wxUSE_RICHEDIT
    m_verRichEdit = 0;
#endif // wxUSE_RICHEDIT

#if wxUSE_INKEDIT && wxUSE_RICHEDIT
    m_isInkEdit = 0;
#endif

    m_privateContextMenu = NULL;
    m_updatesCount = -1;
    m_isNativeCaretShown = true;
}

wxTextCtrl::~wxTextCtrl()
{
    delete m_privateContextMenu;
}

bool wxTextCtrl::Create(wxWindow *parent,
                        wxWindowID id,
                        const wxString& value,
                        const wxPoint& pos,
                        const wxSize& size,
                        long style,
                        const wxValidator& validator,
                        const wxString& name)
{
#ifdef __WXWINCE__
    if ((style & wxBORDER_MASK) == 0)
        style |= wxBORDER_SIMPLE;
#else
    // Standard text control already handles theming
    if ((style & (wxTE_RICH|wxTE_RICH2)) && ((style & wxBORDER_MASK) == wxBORDER_DEFAULT))
        style |= wxBORDER_THEME;
#endif

    // base initialization
    if ( !CreateControl(parent, id, pos, size, style, validator, name) )
        return false;

    if ( !MSWCreateText(value, pos, size) )
        return false;

    return true;
}

bool wxTextCtrl::MSWCreateText(const wxString& value,
                               const wxPoint& pos,
                               const wxSize& size)
{
    // translate wxWin style flags to MSW ones
    WXDWORD msStyle = MSWGetCreateWindowFlags();

    // do create the control - either an EDIT or RICHEDIT
    wxString windowClass = wxT("EDIT");

#if defined(__POCKETPC__) || defined(__SMARTPHONE__)
    // A control that capitalizes the first letter
    if ( HasFlag(wxTE_CAPITALIZE) )
        windowClass = wxT("CAPEDIT");
#endif

#if wxUSE_RICHEDIT
    if ( m_windowStyle & wxTE_AUTO_URL )
    {
        // automatic URL detection only works in RichEdit 2.0+
        m_windowStyle |= wxTE_RICH2;
    }

    if ( m_windowStyle & wxTE_RICH2 )
    {
        // using richedit 2.0 implies using wxTE_RICH
        m_windowStyle |= wxTE_RICH;
    }

    // we need to load the richedit DLL before creating the rich edit control
    if ( m_windowStyle & wxTE_RICH )
    {
        // versions 2.0, 3.0 and 4.1 of rich edit are mostly compatible with
        // each other but not with version 1.0, so we have separate flags for
        // the version 1.0 and the others (and so m_verRichEdit may be 0 (plain
        // EDIT control), 1 for version 1.0 or 2 for any higher version)
        //
        // notice that 1.0 has no Unicode support at all so in Unicode build we
        // must use another version

#if wxUSE_UNICODE
        m_verRichEdit = 2;
#else // !wxUSE_UNICODE
        m_verRichEdit = m_windowStyle & wxTE_RICH2 ? 2 : 1;
#endif // wxUSE_UNICODE/!wxUSE_UNICODE

#if wxUSE_INKEDIT
        // First test if we can load an ink edit control. Normally, all edit
        // controls will be made ink edit controls if a tablet environment is
        // found (or if msw.inkedit != 0 and the InkEd.dll is present).
        // However, an application can veto ink edit controls by either specifying
        // msw.inkedit = 0 or by removing wxTE_RICH[2] from the style.
        //
        if ((wxSystemSettings::HasFeature(wxSYS_TABLET_PRESENT) || wxSystemOptions::GetOptionInt(wxT("msw.inkedit")) != 0) &&
            !(wxSystemOptions::HasOption(wxT("msw.inkedit")) && wxSystemOptions::GetOptionInt(wxT("msw.inkedit")) == 0))
        {
            if (wxRichEditModule::LoadInkEdit())
            {
                windowClass = INKEDIT_CLASS;

#if wxUSE_INKEDIT && wxUSE_RICHEDIT
                m_isInkEdit = 1;
#endif

                // Fake rich text version for other calls
                m_verRichEdit = 2;
            }
        }
#endif

        if (!IsInkEdit())
        {
            if ( m_verRichEdit == 2 )
            {
                if ( wxRichEditModule::Load(wxRichEditModule::Version_41) )
                {
                    // yes, class name for version 4.1 really is 5.0
                    windowClass = _T("RICHEDIT50W");
                }
                else if ( wxRichEditModule::Load(wxRichEditModule::Version_2or3) )
                {
                    windowClass = _T("RichEdit20")
#if wxUSE_UNICODE
                                _T("W");
#else // ANSI
                                _T("A");
#endif // Unicode/ANSI
                }
                else // failed to load msftedit.dll and riched20.dll
                {
                    m_verRichEdit = 1;
                }
            }

            if ( m_verRichEdit == 1 )
            {
                if ( wxRichEditModule::Load(wxRichEditModule::Version_1) )
                {
                    windowClass = _T("RICHEDIT");
                }
                else // failed to load any richedit control DLL
                {
                    // only give the error msg once if the DLL can't be loaded
                    static bool s_errorGiven = false; // MT ok as only used by GUI

                    if ( !s_errorGiven )
                    {
                        wxLogError(_("Impossible to create a rich edit control, using simple text control instead. Please reinstall riched32.dll"));

                        s_errorGiven = true;
                    }

                    m_verRichEdit = 0;
                }
            }
        } // !useInkEdit
    }
#endif // wxUSE_RICHEDIT

    // we need to turn '\n's into "\r\n"s for the multiline controls
    wxString valueWin;
    if ( m_windowStyle & wxTE_MULTILINE )
    {
        valueWin = wxTextFile::Translate(value, wxTextFileType_Dos);
    }
    else // single line
    {
        valueWin = value;
    }

    if ( !MSWCreateControl(windowClass, msStyle, pos, size, valueWin) )
        return false;

#if wxUSE_RICHEDIT
    if (IsRich())
    {
#if wxUSE_INKEDIT
        if (IsInkEdit())
        {
            // Pass IEM_InsertText (0) as wParam, in order to have the ink always
            // converted to text.
            ::SendMessage(GetHwnd(), EM_SETINKINSERTMODE, 0, 0);

            // Make sure the mouse can be used for input
            ::SendMessage(GetHwnd(), EM_SETUSEMOUSEFORINPUT, 1, 0);
        }
#endif

        // enable the events we're interested in: we want to get EN_CHANGE as
        // for the normal controls
        LPARAM mask = ENM_CHANGE;

        if (GetRichVersion() == 1 && !IsInkEdit())
        {
            // we also need EN_MSGFILTER for richedit 1.0 for the reasons
            // explained in its handler
           mask |= ENM_MOUSEEVENTS;

           // we also need to force the appearance of the vertical scrollbar
           // initially as otherwise the control doesn't refresh correctly
           // after resize: but once the vertical scrollbar had been shown
           // (even if it's subsequently hidden) it does
           //
           // this is clearly a bug and for now it has been only noticed under
           // Windows XP, so if we're sure it works correctly under other
           // systems we could do this only for XP
           SetSize(-1, 1); // 1 is small enough to force vert scrollbar
           SetInitialSize(size);
        }
        else if ( m_windowStyle & wxTE_AUTO_URL )
        {
            mask |= ENM_LINK;

            ::SendMessage(GetHwnd(), EM_AUTOURLDETECT, TRUE, 0);
        }

        ::SendMessage(GetHwnd(), EM_SETEVENTMASK, 0, mask);
    }
#endif // wxUSE_RICHEDIT

#ifndef __WXWINCE__
    // Without this, if we pass the size in the constructor and then don't change it,
    // the themed borders will be drawn incorrectly.
    SetWindowPos(GetHwnd(), NULL, 0, 0, 0, 0,
                SWP_NOZORDER|SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE|
                SWP_FRAMECHANGED);
#endif

    return true;
}

// Make sure the window style (etc.) reflects the HWND style (roughly)
void wxTextCtrl::AdoptAttributesFromHWND()
{
    wxWindow::AdoptAttributesFromHWND();

    HWND hWnd = GetHwnd();
    long style = ::GetWindowLong(hWnd, GWL_STYLE);

    // retrieve the style to see whether this is an edit or richedit ctrl
#if wxUSE_RICHEDIT
    wxString classname = wxGetWindowClass(GetHWND());

    if ( classname.IsSameAs(_T("EDIT"), false /* no case */) )
    {
        m_verRichEdit = 0;
    }
    else // rich edit?
    {
        wxChar c;
        if ( wxSscanf(classname, _T("RichEdit%d0%c"), &m_verRichEdit, &c) != 2 )
        {
            wxLogDebug(_T("Unknown edit control '%s'."), classname.c_str());

            m_verRichEdit = 0;
        }
    }
#endif // wxUSE_RICHEDIT

    if (style & ES_MULTILINE)
        m_windowStyle |= wxTE_MULTILINE;
    if (style & ES_PASSWORD)
        m_windowStyle |= wxTE_PASSWORD;
    if (style & ES_READONLY)
        m_windowStyle |= wxTE_READONLY;
    if (style & ES_WANTRETURN)
        m_windowStyle |= wxTE_PROCESS_ENTER;
    if (style & ES_CENTER)
        m_windowStyle |= wxTE_CENTRE;
    if (style & ES_RIGHT)
        m_windowStyle |= wxTE_RIGHT;
}

WXDWORD wxTextCtrl::MSWGetStyle(long style, WXDWORD *exstyle) const
{
    long msStyle = wxControl::MSWGetStyle(style, exstyle);

    // styles which we alaways add by default
    if ( style & wxTE_MULTILINE )
    {
        msStyle |= ES_MULTILINE | ES_WANTRETURN;
        if ( !(style & wxTE_NO_VSCROLL) )
        {
            // always adjust the vertical scrollbar automatically if we have it
            msStyle |= WS_VSCROLL | ES_AUTOVSCROLL;

#if wxUSE_RICHEDIT
            // we have to use this style for the rich edit controls because
            // without it the vertical scrollbar never appears at all in
            // richedit 3.0 because of our ECO_NOHIDESEL hack (search for it)
            if ( style & wxTE_RICH2 )
            {
                msStyle |= ES_DISABLENOSCROLL;
            }
#endif // wxUSE_RICHEDIT
        }

        style |= wxTE_PROCESS_ENTER;
    }
    else // !multiline
    {
        // there is really no reason to not have this style for single line
        // text controls
        msStyle |= ES_AUTOHSCROLL;
    }

    // note that wxTE_DONTWRAP is the same as wxHSCROLL so if we have a horz
    // scrollbar, there is no wrapping -- which makes sense
    if ( style & wxTE_DONTWRAP )
    {
        // automatically scroll the control horizontally as necessary
        //
        // NB: ES_AUTOHSCROLL is needed for richedit controls or they don't
        //     show horz scrollbar at all, even in spite of WS_HSCROLL, and as
        //     it doesn't seem to do any harm for plain edit controls, add it
        //     always
        msStyle |= WS_HSCROLL | ES_AUTOHSCROLL;
    }

    if ( style & wxTE_READONLY )
        msStyle |= ES_READONLY;

    if ( style & wxTE_PASSWORD )
        msStyle |= ES_PASSWORD;

    if ( style & wxTE_NOHIDESEL )
        msStyle |= ES_NOHIDESEL;

    // note that we can't do do "& wxTE_LEFT" as wxTE_LEFT == 0
    if ( style & wxTE_CENTRE )
        msStyle |= ES_CENTER;
    else if ( style & wxTE_RIGHT )
        msStyle |= ES_RIGHT;
    else
        msStyle |= ES_LEFT; // ES_LEFT is 0 as well but for consistency...

    return msStyle;
}

void wxTextCtrl::SetWindowStyleFlag(long style)
{
    // changing the alignment of the control dynamically works under Win2003
    // (but not older Windows version: it seems to work under some versions of
    // XP but not other ones, and we have no way to determine it so be
    // conservative here) and only for plain EDIT controls (not RICH ones) and
    // we have to recreate the control to make it always work
    if ( IsRich() || wxGetWinVersion() < wxWinVersion_2003 )
    {
        const long alignMask = wxTE_LEFT | wxTE_CENTRE | wxTE_RIGHT;
        if ( (style & alignMask) != (GetWindowStyle() & alignMask) )
        {
            const wxString value = GetValue();
            const wxPoint pos = GetPosition();
            const wxSize size = GetSize();

            // delete the old window
            HWND hwnd = GetHwnd();
            DissociateHandle();
            ::DestroyWindow(hwnd);

            // create the new one with the updated flags
            m_windowStyle = style;
            MSWCreateText(value, pos, size);

            // and make sure it has the same attributes as before
            if ( m_hasFont )
            {
                // calling SetFont(m_font) would do nothing as the code would
                // notice that the font didn't change, so force it to believe
                // that it did
                wxFont font = m_font;
                m_font = wxNullFont;
                SetFont(font);
            }

            if ( m_hasFgCol )
            {
                wxColour colFg = m_foregroundColour;
                m_foregroundColour = wxNullColour;
                SetForegroundColour(colFg);
            }

            if ( m_hasBgCol )
            {
                wxColour colBg = m_backgroundColour;
                m_backgroundColour = wxNullColour;
                SetBackgroundColour(colBg);
            }

            // note that text styles are lost but this is probably not a big
            // problem: if you use styles, you probably don't use nor change
            // alignment flags anyhow

            return;
        }
    }

#if wxUSE_RICHEDIT
    // we have to deal with some styles separately because they can't be
    // changed by simply calling SetWindowLong(GWL_STYLE) but can be changed
    // using richedit-specific EM_SETOPTIONS
    if ( IsRich() &&
            ((style & wxTE_NOHIDESEL) != (GetWindowStyle() & wxTE_NOHIDESEL)) )
    {
        bool set = (style & wxTE_NOHIDESEL) != 0;

        ::SendMessage(GetHwnd(), EM_SETOPTIONS, set ? ECOOP_OR : ECOOP_AND,
                      set ? ECO_NOHIDESEL : ~ECO_NOHIDESEL);
    }
#endif // wxUSE_RICHEDIT

    wxControl::SetWindowStyleFlag(style);
}

// ----------------------------------------------------------------------------
// set/get the controls text
// ----------------------------------------------------------------------------

bool wxTextCtrl::IsEmpty() const
{
    // this is an optimization for multiline controls containing a lot of text
    if ( IsMultiLine() && GetNumberOfLines() != 1 )
        return false;

    return wxTextCtrlBase::IsEmpty();
}

wxString wxTextCtrl::GetValue() const
{
    // range 0..-1 is special for GetRange() and means to retrieve all text
    return GetRange(0, -1);
}

wxString wxTextCtrl::GetRange(long from, long to) const
{
    wxString str;

    if ( from >= to && to != -1 )
    {
        // nothing to retrieve
        return str;
    }

#if wxUSE_RICHEDIT
    if ( IsRich() )
    {
        int len = GetWindowTextLength(GetHwnd());
        if ( len > from )
        {
            if ( to == -1 )
                to = len;

#if !wxUSE_UNICODE
            // we must use EM_STREAMOUT if we don't want to lose all characters
            // not representable in the current character set (EM_GETTEXTRANGE
            // simply replaces them with question marks...)
            if ( GetRichVersion() > 1 )
            {
                // we must have some encoding, otherwise any 8bit chars in the
                // control are simply *lost* (replaced by '?')
                wxFontEncoding encoding = wxFONTENCODING_SYSTEM;

                wxFont font = m_defaultStyle.GetFont();
                if ( !font.Ok() )
                    font = GetFont();

                if ( font.Ok() )
                {
                   encoding = font.GetEncoding();
                }

#if wxUSE_INTL
                if ( encoding == wxFONTENCODING_SYSTEM )
                {
                    encoding = wxLocale::GetSystemEncoding();
                }
#endif // wxUSE_INTL

                if ( encoding == wxFONTENCODING_SYSTEM )
                {
                    encoding = wxFONTENCODING_ISO8859_1;
                }

                str = StreamOut(encoding);

                if ( !str.empty() )
                {
                    // we have to manually extract the required part, luckily
                    // this is easy in this case as EOL characters in str are
                    // just LFs because we remove CRs in wxRichEditStreamOut
                    str = str.Mid(from, to - from);
                }
            }

            // StreamOut() wasn't used or failed, try to do it in normal way
            if ( str.empty() )
#endif // !wxUSE_UNICODE
            {
                // alloc one extra WORD as needed by the control
                wxStringBuffer tmp(str, ++len);
                wxChar *p = tmp;

                TEXTRANGE textRange;
                textRange.chrg.cpMin = from;
                textRange.chrg.cpMax = to;
                textRange.lpstrText = p;

                (void)::SendMessage(GetHwnd(), EM_GETTEXTRANGE,
                                    0, (LPARAM)&textRange);

                if ( m_verRichEdit > 1 )
                {
                    // RichEdit 2.0 uses just CR ('\r') for the
                    // newlines which is neither Unix nor Windows
                    // style - convert it to something reasonable
                    for ( ; *p; p++ )
                    {
                        if ( *p == _T('\r') )
                            *p = _T('\n');
                    }
                }
            }

            if ( m_verRichEdit == 1 )
            {
                // convert to the canonical form - see comment below
                str = wxTextFile::Translate(str, wxTextFileType_Unix);
            }
        }
        //else: no text at all, leave the string empty
    }
    else
#endif // wxUSE_RICHEDIT
    {
        // retrieve all text
        str = wxGetWindowText(GetHWND());

        // need only a range?
        if ( from < to )
        {
            str = str.Mid(from, to - from);
        }

        // WM_GETTEXT uses standard DOS CR+LF (\r\n) convention - convert to the
        // canonical one (same one as above) for consistency with the other kinds
        // of controls and, more importantly, with the other ports
        str = wxTextFile::Translate(str, wxTextFileType_Unix);
    }

    return str;
}

void wxTextCtrl::DoSetValue(const wxString& value, int flags)
{
    // if the text is long enough, it's faster to just set it instead of first
    // comparing it with the old one (chances are that it will be different
    // anyhow, this comparison is there to avoid flicker for small single-line
    // edit controls mostly)
    if ( (value.length() > 0x400) || (value != GetValue()) )
    {
        DoWriteText(value, flags /* doesn't include SelectionOnly here */);

        // mark the control as being not dirty - we changed its text, not the
        // user
        DiscardEdits();

        // for compatibility, don't move the cursor when doing SetValue()
        SetInsertionPoint(0);
    }
    else // same text
    {
        // still reset the modified flag even if the value didn't really change
        // because now it comes from the program and not the user (and do it
        // before generating the event so that the event handler could get the
        // expected value from IsModified())
        DiscardEdits();

        // still send an event for consistency
        if (flags & SetValue_SendEvent)
            SendUpdateEvent();
    }
}

#if wxUSE_RICHEDIT && (!wxUSE_UNICODE || wxUSE_UNICODE_MSLU)

// TODO: using memcpy() would improve performance a lot for big amounts of text

DWORD CALLBACK
wxRichEditStreamIn(DWORD dwCookie, BYTE *buf, LONG cb, LONG *pcb)
{
    *pcb = 0;

    const wchar_t ** const ppws = (const wchar_t **)dwCookie;

    wchar_t *wbuf = (wchar_t *)buf;
    const wchar_t *wpc = *ppws;
    while ( cb && *wpc )
    {
        *wbuf++ = *wpc++;

        cb -= sizeof(wchar_t);
        (*pcb) += sizeof(wchar_t);
    }

    *ppws = wpc;

    return 0;
}

// helper struct used to pass parameters from wxTextCtrl to wxRichEditStreamOut
struct wxStreamOutData
{
    wchar_t *wpc;
    size_t len;
};

DWORD CALLBACK
wxRichEditStreamOut(DWORD_PTR dwCookie, BYTE *buf, LONG cb, LONG *pcb)
{
    *pcb = 0;

    wxStreamOutData *data = (wxStreamOutData *)dwCookie;

    const wchar_t *wbuf = (const wchar_t *)buf;
    wchar_t *wpc = data->wpc;
    while ( cb )
    {
        wchar_t wch = *wbuf++;

        // turn "\r\n" into "\n" on the fly
        if ( wch != L'\r' )
            *wpc++ = wch;
        else
            data->len--;

        cb -= sizeof(wchar_t);
        (*pcb) += sizeof(wchar_t);
    }

    data->wpc = wpc;

    return 0;
}


#if wxUSE_UNICODE_MSLU
    #define UNUSED_IF_MSLU(param)
#else
    #define UNUSED_IF_MSLU(param) param
#endif

bool
wxTextCtrl::StreamIn(const wxString& value,
                     wxFontEncoding UNUSED_IF_MSLU(encoding),
                     bool selectionOnly)
{
#if wxUSE_UNICODE_MSLU
    const wchar_t *wpc = value.c_str();
#else // !wxUSE_UNICODE_MSLU
    wxCSConv conv(encoding);

    const size_t len = conv.MB2WC(NULL, value, value.length());

    if (len == wxCONV_FAILED)
        return false;

#if wxUSE_WCHAR_T
    wxWCharBuffer wchBuf(len); // allocates one extra character
    wchar_t *wpc = wchBuf.data();
#else
    wchar_t *wchBuf = (wchar_t *)malloc((len + 1)*sizeof(wchar_t));
    wchar_t *wpc = wchBuf;
#endif

    conv.MB2WC(wpc, value, len + 1);
#endif // wxUSE_UNICODE_MSLU

    // finally, stream it in the control
    EDITSTREAM eds;
    wxZeroMemory(eds);
    eds.dwCookie = (DWORD)&wpc;
    // the cast below is needed for broken (very) old mingw32 headers
    eds.pfnCallback = (EDITSTREAMCALLBACK)wxRichEditStreamIn;

    // same problem as in DoWriteText(): we can get multiple events here
    UpdatesCountFilter ucf(m_updatesCount);

    ::SendMessage(GetHwnd(), EM_STREAMIN,
                  SF_TEXT |
                  SF_UNICODE |
                  (selectionOnly ? SFF_SELECTION : 0),
                  (LPARAM)&eds);

    // It's okay for EN_UPDATE to not be sent if the selection is empty and
    // the text is empty, otherwise warn the programmer about it.
    wxASSERT_MSG( ucf.GotUpdate() || ( !HasSelection() && value.empty() ),
                  _T("EM_STREAMIN didn't send EN_UPDATE?") );

    if ( eds.dwError )
    {
        wxLogLastError(_T("EM_STREAMIN"));
    }

#if !wxUSE_WCHAR_T
    free(wchBuf);
#endif // !wxUSE_WCHAR_T

    return true;
}

#if !wxUSE_UNICODE_MSLU

wxString
wxTextCtrl::StreamOut(wxFontEncoding encoding, bool selectionOnly) const
{
    wxString out;

    const int len = GetWindowTextLength(GetHwnd());

#if wxUSE_WCHAR_T
    wxWCharBuffer wchBuf(len);
    wchar_t *wpc = wchBuf.data();
#else
    wchar_t *wchBuf = (wchar_t *)malloc((len + 1)*sizeof(wchar_t));
    wchar_t *wpc = wchBuf;
#endif

    wxStreamOutData data;
    data.wpc = wpc;
    data.len = len;

    EDITSTREAM eds;
    wxZeroMemory(eds);
    eds.dwCookie = (DWORD)&data;
    eds.pfnCallback = wxRichEditStreamOut;

    ::SendMessage
      (
        GetHwnd(),
        EM_STREAMOUT,
        SF_TEXT | SF_UNICODE | (selectionOnly ? SFF_SELECTION : 0),
        (LPARAM)&eds
      );

    if ( eds.dwError )
    {
        wxLogLastError(_T("EM_STREAMOUT"));
    }
    else // streamed out ok
    {
        // NUL-terminate the string because its length could have been
        // decreased by wxRichEditStreamOut
        *(wchBuf.data() + data.len) = L'\0';

        // now convert to the given encoding (this is a possibly lossful
        // conversion but what else can we do)
        wxCSConv conv(encoding);
        size_t lenNeeded = conv.WC2MB(NULL, wchBuf, 0);

        if ( lenNeeded != wxCONV_FAILED && lenNeeded++ )
        {
            conv.WC2MB(wxStringBuffer(out, lenNeeded), wchBuf, lenNeeded);
        }
    }

#if !wxUSE_WCHAR_T
    free(wchBuf);
#endif // !wxUSE_WCHAR_T

    return out;
}

#endif // !wxUSE_UNICODE_MSLU

#endif // wxUSE_RICHEDIT

void wxTextCtrl::WriteText(const wxString& value)
{
    DoWriteText(value);
}

void wxTextCtrl::DoWriteText(const wxString& value, int flags)
{
    bool selectionOnly = (flags & SetValue_SelectionOnly) != 0;
    wxString valueDos;
    if ( m_windowStyle & wxTE_MULTILINE )
        valueDos = wxTextFile::Translate(value, wxTextFileType_Dos);
    else
        valueDos = value;

#if wxUSE_RICHEDIT
    // there are several complications with the rich edit controls here
    bool done = false;
    if ( IsRich() )
    {
        // first, ensure that the new text will be in the default style
        if ( !m_defaultStyle.IsDefault() )
        {
            long start, end;
            GetSelection(&start, &end);
            SetStyle(start, end, m_defaultStyle);
        }

#if wxUSE_UNICODE_MSLU
        // RichEdit doesn't have Unicode version of EM_REPLACESEL on Win9x,
        // but EM_STREAMIN works
        if ( wxUsingUnicowsDll() && GetRichVersion() > 1 )
        {
           done = StreamIn(valueDos, wxFONTENCODING_SYSTEM, selectionOnly);
        }
#endif // wxUSE_UNICODE_MSLU

#if !wxUSE_UNICODE
        // next check if the text we're inserting must be shown in a non
        // default charset -- this only works for RichEdit > 1.0
        if ( GetRichVersion() > 1 )
        {
            wxFont font = m_defaultStyle.GetFont();
            if ( !font.Ok() )
                font = GetFont();

            if ( font.Ok() )
            {
               wxFontEncoding encoding = font.GetEncoding();
               if ( encoding != wxFONTENCODING_SYSTEM )
               {
                   // we have to use EM_STREAMIN to force richedit control 2.0+
                   // to show any text in the non default charset -- otherwise
                   // it thinks it knows better than we do and always shows it
                   // in the default one
                   done = StreamIn(valueDos, encoding, selectionOnly);
               }
            }
        }
#endif // !wxUSE_UNICODE
    }

    if ( !done )
#endif // wxUSE_RICHEDIT
    {
        // in some cases we get 2 EN_CHANGE notifications after the SendMessage
        // call (this happens for plain EDITs with EM_REPLACESEL and under some
        // -- undetermined -- conditions with rich edit) and sometimes we don't
        // get any events at all (plain EDIT with WM_SETTEXT), so ensure that
        // we generate exactly one of them by ignoring all but the first one in
        // SendUpdateEvent() and generating one ourselves if we hadn't got any
        // notifications from Windows
        if ( !(flags & SetValue_SendEvent) )
            m_updatesCount = -2;        // suppress any update event

        UpdatesCountFilter ucf(m_updatesCount);

        ::SendMessage(GetHwnd(), selectionOnly ? EM_REPLACESEL : WM_SETTEXT,
                      // EM_REPLACESEL takes 1 to indicate the operation should be redoable
                      selectionOnly ? 1 : 0, (LPARAM)valueDos.c_str());

        if ( !ucf.GotUpdate() && (flags & SetValue_SendEvent) )
        {
            SendUpdateEvent();
        }
    }
}

void wxTextCtrl::AppendText(const wxString& text)
{
    SetInsertionPointEnd();

    WriteText(text);

#if wxUSE_RICHEDIT
    // don't do this if we're frozen, saves some time
    if ( !IsFrozen() && IsMultiLine() && GetRichVersion() > 1 )
    {
        // setting the caret to the end and showing it simply doesn't work for
        // RichEdit 2.0 -- force it to still do what we want
        ::SendMessage(GetHwnd(), EM_LINESCROLL, 0, GetNumberOfLines());
    }
#endif // wxUSE_RICHEDIT
}

void wxTextCtrl::Clear()
{
    ::SetWindowText(GetHwnd(), wxEmptyString);

#if wxUSE_RICHEDIT
    if ( !IsRich() )
#endif // wxUSE_RICHEDIT
    {
        // rich edit controls send EN_UPDATE from WM_SETTEXT handler themselves
        // but the normal ones don't -- make Clear() behaviour consistent by
        // always sending this event

        // Windows already sends an update event for single-line
        // controls.
        if ( m_windowStyle & wxTE_MULTILINE )
            SendUpdateEvent();
    }
}

#ifdef __WIN32__

bool wxTextCtrl::EmulateKeyPress(const wxKeyEvent& event)
{
    SetFocus();

    size_t lenOld = GetValue().length();

    wxUint32 code = event.GetRawKeyCode();
    ::keybd_event((BYTE)code, 0, 0 /* key press */, 0);
    ::keybd_event((BYTE)code, 0, KEYEVENTF_KEYUP, 0);

    // assume that any alphanumeric key changes the total number of characters
    // in the control - this should work in 99% of cases
    return GetValue().length() != lenOld;
}

#endif // __WIN32__

// ----------------------------------------------------------------------------
// Clipboard operations
// ----------------------------------------------------------------------------

void wxTextCtrl::Copy()
{
    if (CanCopy())
    {
        ::SendMessage(GetHwnd(), WM_COPY, 0, 0L);
    }
}

void wxTextCtrl::Cut()
{
    if (CanCut())
    {
        ::SendMessage(GetHwnd(), WM_CUT, 0, 0L);
    }
}

void wxTextCtrl::Paste()
{
    if (CanPaste())
    {
        ::SendMessage(GetHwnd(), WM_PASTE, 0, 0L);
    }
}

bool wxTextCtrl::HasSelection() const
{
    long from, to;
    GetSelection(&from, &to);
    return from != to;
}

bool wxTextCtrl::CanCopy() const
{
    // Can copy if there's a selection
    return HasSelection();
}

bool wxTextCtrl::CanCut() const
{
    return CanCopy() && IsEditable();
}

bool wxTextCtrl::CanPaste() const
{
    if ( !IsEditable() )
        return false;

#if wxUSE_RICHEDIT
    if ( IsRich() )
    {
        UINT cf = 0; // 0 == any format

        return ::SendMessage(GetHwnd(), EM_CANPASTE, cf, 0) != 0;
    }
#endif // wxUSE_RICHEDIT

    // Standard edit control: check for straight text on clipboard
    if ( !::OpenClipboard(GetHwndOf(wxTheApp->GetTopWindow())) )
        return false;

    bool isTextAvailable = ::IsClipboardFormatAvailable(CF_TEXT) != 0;
    ::CloseClipboard();

    return isTextAvailable;
}

// ----------------------------------------------------------------------------
// Accessors
// ----------------------------------------------------------------------------

void wxTextCtrl::SetEditable(bool editable)
{
    HWND hWnd = GetHwnd();
    ::SendMessage(hWnd, EM_SETREADONLY, (WPARAM)!editable, (LPARAM)0L);
}

void wxTextCtrl::SetInsertionPoint(long pos)
{
    DoSetSelection(pos, pos);
}

void wxTextCtrl::SetInsertionPointEnd()
{
    // we must not do anything if the caret is already there because calling
    // SetInsertionPoint() thaws the controls if Freeze() had been called even
    // if it doesn't actually move the caret anywhere and so the simple fact of
    // doing it results in horrible flicker when appending big amounts of text
    // to the control in a few chunks (see DoAddText() test in the text sample)
    const wxTextPos lastPosition = GetLastPosition();
    if ( GetInsertionPoint() == lastPosition )
    {
        return;
    }

    long pos;

#if wxUSE_RICHEDIT
    if ( m_verRichEdit == 1 )
    {
        // we don't have to waste time calling GetLastPosition() in this case
        pos = -1;
    }
    else // !RichEdit 1.0
#endif // wxUSE_RICHEDIT
    {
        pos = lastPosition;
    }

    SetInsertionPoint(pos);
}

long wxTextCtrl::GetInsertionPoint() const
{
#if wxUSE_RICHEDIT
    if ( IsRich() )
    {
        CHARRANGE range;
        range.cpMin = 0;
        range.cpMax = 0;
        ::SendMessage(GetHwnd(), EM_EXGETSEL, 0, (LPARAM) &range);
        return range.cpMin;
    }
#endif // wxUSE_RICHEDIT

    DWORD Pos = (DWORD)::SendMessage(GetHwnd(), EM_GETSEL, 0, 0L);
    return Pos & 0xFFFF;
}

wxTextPos wxTextCtrl::GetLastPosition() const
{
    int numLines = GetNumberOfLines();
    long posStartLastLine = XYToPosition(0, numLines - 1);

    long lenLastLine = GetLengthOfLineContainingPos(posStartLastLine);

    return posStartLastLine + lenLastLine;
}

// If the return values from and to are the same, there is no
// selection.
void wxTextCtrl::GetSelection(long* from, long* to) const
{
#if wxUSE_RICHEDIT
    if ( IsRich() )
    {
        CHARRANGE charRange;
        ::SendMessage(GetHwnd(), EM_EXGETSEL, 0, (LPARAM) &charRange);

        *from = charRange.cpMin;
        *to = charRange.cpMax;
    }
    else
#endif // !wxUSE_RICHEDIT
    {
        DWORD dwStart, dwEnd;
        ::SendMessage(GetHwnd(), EM_GETSEL, (WPARAM)&dwStart, (LPARAM)&dwEnd);

        *from = dwStart;
        *to = dwEnd;
    }
}

bool wxTextCtrl::IsEditable() const
{
    // strangely enough, we may be called before the control is created: our
    // own Create() calls MSWGetStyle() which calls AcceptsFocus() which calls
    // us
    if ( !m_hWnd )
        return true;

    long style = ::GetWindowLong(GetHwnd(), GWL_STYLE);

    return (style & ES_READONLY) == 0;
}

// ----------------------------------------------------------------------------
// selection
// ----------------------------------------------------------------------------

void wxTextCtrl::SetSelection(long from, long to)
{
    // if from and to are both -1, it means (in wxWidgets) that all text should
    // be selected - translate into Windows convention
    if ( (from == -1) && (to == -1) )
    {
        from = 0;
        to = -1;
    }

    DoSetSelection(from, to);
}

void wxTextCtrl::DoSetSelection(long from, long to, bool scrollCaret)
{
    HWND hWnd = GetHwnd();

#if wxUSE_RICHEDIT
    if ( IsRich() )
    {
        CHARRANGE range;
        range.cpMin = from;
        range.cpMax = to;
        ::SendMessage(hWnd, EM_EXSETSEL, 0, (LPARAM) &range);
    }
    else
#endif // wxUSE_RICHEDIT
    {
        ::SendMessage(hWnd, EM_SETSEL, (WPARAM)from, (LPARAM)to);
    }

    if ( scrollCaret && !IsFrozen() )
    {
#if wxUSE_RICHEDIT
        // richedit 3.0 (i.e. the version living in riched20.dll distributed
        // with Windows 2000 and beyond) doesn't honour EM_SCROLLCARET when
        // emulating richedit 2.0 unless the control has focus or ECO_NOHIDESEL
        // option is set (but it does work ok in richedit 1.0 mode...)
        //
        // so to make it work we either need to give focus to it here which
        // will probably create many problems (dummy focus events; window
        // containing the text control being brought to foreground
        // unexpectedly; ...) or to temporarily set ECO_NOHIDESEL which may
        // create other problems too -- and in fact it does because if we turn
        // on/off this style while appending the text to the control, the
        // vertical scrollbar never appears in it even if we append tons of
        // text and to work around this the only solution I found was to use
        // ES_DISABLENOSCROLL
        //
        // this is very ugly but I don't see any other way to make this work
        long style = 0;
        if ( GetRichVersion() > 1 )
        {
            if ( !HasFlag(wxTE_NOHIDESEL) )
            {
                // setting ECO_NOHIDESEL also sets WS_VISIBLE and possibly
                // others, remember the style so we can reset it later if needed
                style = ::GetWindowLong(GetHwnd(), GWL_STYLE);
                ::SendMessage(GetHwnd(), EM_SETOPTIONS,
                              ECOOP_OR, ECO_NOHIDESEL);
            }
            //else: everything is already ok
        }
#endif // wxUSE_RICHEDIT

        ::SendMessage(hWnd, EM_SCROLLCARET, (WPARAM)0, (LPARAM)0);

#if wxUSE_RICHEDIT
        // restore ECO_NOHIDESEL if we changed it
        if ( GetRichVersion() > 1 && !HasFlag(wxTE_NOHIDESEL) )
        {
            ::SendMessage(GetHwnd(), EM_SETOPTIONS,
                          ECOOP_AND, ~ECO_NOHIDESEL);
            if ( style != ::GetWindowLong(GetHwnd(), GWL_STYLE) )
                ::SetWindowLong(GetHwnd(), GWL_STYLE, style);
        }
#endif // wxUSE_RICHEDIT
    }
}

// ----------------------------------------------------------------------------
// Working with files
// ----------------------------------------------------------------------------

bool wxTextCtrl::DoLoadFile(const wxString& file, int fileType)
{
    if ( wxTextCtrlBase::DoLoadFile(file, fileType) )
    {
        // update the size limit if needed
        AdjustSpaceLimit();

        return true;
    }

    return false;
}

// ----------------------------------------------------------------------------
// Editing
// ----------------------------------------------------------------------------

void wxTextCtrl::Replace(long from, long to, const wxString& value)
{
    // Set selection and remove it
    DoSetSelection(from, to, false /* don't scroll caret into view */);

    DoWriteText(value);
}

void wxTextCtrl::Remove(long from, long to)
{
    Replace(from, to, wxEmptyString);
}

bool wxTextCtrl::IsModified() const
{
    return ::SendMessage(GetHwnd(), EM_GETMODIFY, 0, 0) != 0;
}

void wxTextCtrl::MarkDirty()
{
    ::SendMessage(GetHwnd(), EM_SETMODIFY, TRUE, 0L);
}

void wxTextCtrl::DiscardEdits()
{
    ::SendMessage(GetHwnd(), EM_SETMODIFY, FALSE, 0L);
}

int wxTextCtrl::GetNumberOfLines() const
{
    return (int)::SendMessage(GetHwnd(), EM_GETLINECOUNT, (WPARAM)0, (LPARAM)0);
}

// ----------------------------------------------------------------------------
// Positions <-> coords
// ----------------------------------------------------------------------------

long wxTextCtrl::XYToPosition(long x, long y) const
{
    // This gets the char index for the _beginning_ of this line
    long charIndex = ::SendMessage(GetHwnd(), EM_LINEINDEX, (WPARAM)y, (LPARAM)0);

    return charIndex + x;
}

bool wxTextCtrl::PositionToXY(long pos, long *x, long *y) const
{
    HWND hWnd = GetHwnd();

    // This gets the line number containing the character
    long lineNo;
#if wxUSE_RICHEDIT
    if ( IsRich() )
    {
        lineNo = ::SendMessage(hWnd, EM_EXLINEFROMCHAR, 0, (LPARAM)pos);
    }
    else
#endif // wxUSE_RICHEDIT
    {
        lineNo = ::SendMessage(hWnd, EM_LINEFROMCHAR, (WPARAM)pos, 0);
    }

    if ( lineNo == -1 )
    {
        // no such line
        return false;
    }

    // This gets the char index for the _beginning_ of this line
    long charIndex = ::SendMessage(hWnd, EM_LINEINDEX, (WPARAM)lineNo, (LPARAM)0);
    if ( charIndex == -1 )
    {
        return false;
    }

    // The X position must therefore be the different between pos and charIndex
    if ( x )
        *x = pos - charIndex;
    if ( y )
        *y = lineNo;

    return true;
}

wxTextCtrlHitTestResult
wxTextCtrl::HitTest(const wxPoint& pt, long *posOut) const
{
    // first get the position from Windows
    LPARAM lParam;

#if wxUSE_RICHEDIT
    POINTL ptl;
    if ( IsRich() )
    {
        // for rich edit controls the position is passed iva the struct fields
        ptl.x = pt.x;
        ptl.y = pt.y;
        lParam = (LPARAM)&ptl;
    }
    else
#endif // wxUSE_RICHEDIT
    {
        // for the plain ones, we are limited to 16 bit positions which are
        // combined in a single 32 bit value
        lParam = MAKELPARAM(pt.x, pt.y);
    }

    LRESULT pos = ::SendMessage(GetHwnd(), EM_CHARFROMPOS, 0, lParam);

    if ( pos == -1 )
    {
        // this seems to indicate an error...
        return wxTE_HT_UNKNOWN;
    }

#if wxUSE_RICHEDIT
    if ( !IsRich() )
#endif // wxUSE_RICHEDIT
    {
        // for plain EDIT controls the higher word contains something else
        pos = LOWORD(pos);
    }


    // next determine where it is relatively to our point: EM_CHARFROMPOS
    // always returns the closest character but we need to be more precise, so
    // double check that we really are where it pretends
    POINTL ptReal;

#if wxUSE_RICHEDIT
    // FIXME: we need to distinguish between richedit 2 and 3 here somehow but
    //        we don't know how to do it
    if ( IsRich() )
    {
        ::SendMessage(GetHwnd(), EM_POSFROMCHAR, (WPARAM)&ptReal, pos);
    }
    else
#endif // wxUSE_RICHEDIT
    {
        LRESULT lRc = ::SendMessage(GetHwnd(), EM_POSFROMCHAR, pos, 0);

        if ( lRc == -1 )
        {
            // this is apparently returned when pos corresponds to the last
            // position
            ptReal.x =
            ptReal.y = 0;
        }
        else
        {
            ptReal.x = LOWORD(lRc);
            ptReal.y = HIWORD(lRc);
        }
    }

    wxTextCtrlHitTestResult rc;

    if ( pt.y > ptReal.y + GetCharHeight() )
        rc = wxTE_HT_BELOW;
    else if ( pt.x > ptReal.x + GetCharWidth() )
        rc = wxTE_HT_BEYOND;
    else
        rc = wxTE_HT_ON_TEXT;

    if ( posOut )
        *posOut = pos;

    return rc;
}

// ----------------------------------------------------------------------------
//
// ----------------------------------------------------------------------------

void wxTextCtrl::ShowPosition(long pos)
{
    HWND hWnd = GetHwnd();

    // To scroll to a position, we pass the number of lines and characters
    // to scroll *by*. This means that we need to:
    // (1) Find the line position of the current line.
    // (2) Find the line position of pos.
    // (3) Scroll by (pos - current).
    // For now, ignore the horizontal scrolling.

    // Is this where scrolling is relative to - the line containing the caret?
    // Or is the first visible line??? Try first visible line.
//    int currentLineLineNo1 = (int)::SendMessage(hWnd, EM_LINEFROMCHAR, (WPARAM)-1, (LPARAM)0L);

    int currentLineLineNo = (int)::SendMessage(hWnd, EM_GETFIRSTVISIBLELINE, (WPARAM)0, (LPARAM)0L);

    int specifiedLineLineNo = (int)::SendMessage(hWnd, EM_LINEFROMCHAR, (WPARAM)pos, (LPARAM)0L);

    int linesToScroll = specifiedLineLineNo - currentLineLineNo;

    if (linesToScroll != 0)
      (void)::SendMessage(hWnd, EM_LINESCROLL, (WPARAM)0, (LPARAM)linesToScroll);
}

long wxTextCtrl::GetLengthOfLineContainingPos(long pos) const
{
    return ::SendMessage(GetHwnd(), EM_LINELENGTH, (WPARAM)pos, 0);
}

int wxTextCtrl::GetLineLength(long lineNo) const
{
    long pos = XYToPosition(0, lineNo);

    return GetLengthOfLineContainingPos(pos);
}

wxString wxTextCtrl::GetLineText(long lineNo) const
{
    size_t len = (size_t)GetLineLength(lineNo) + 1;

    // there must be at least enough place for the length WORD in the
    // buffer
    len += sizeof(WORD);

    wxString str;
    {
        wxStringBufferLength tmp(str, len);
        wxChar *buf = tmp;

        *(WORD *)buf = (WORD)len;
        len = (size_t)::SendMessage(GetHwnd(), EM_GETLINE, lineNo, (LPARAM)buf);

#if wxUSE_RICHEDIT
        if ( IsRich() )
        {
            // remove the '\r' returned by the rich edit control, the user code
            // should never see it
            if ( buf[len - 2] == _T('\r') && buf[len - 1] == _T('\n') )
            {
                // richedit 1.0 uses "\r\n" as line terminator, so remove "\r"
                // here and "\n" below
                buf[len - 2] = _T('\n');
                len--;
            }
            else if ( buf[len - 1] == _T('\r') )
            {
                // richedit 2.0+ uses only "\r", replace it with "\n"
                buf[len - 1] = _T('\n');
            }
        }
#endif // wxUSE_RICHEDIT

        // remove the '\n' at the end, if any (this is how this function is
        // supposed to work according to the docs)
        if ( buf[len - 1] == _T('\n') )
        {
            len--;
        }

        buf[len] = 0;
        tmp.SetLength(len);
    }

    return str;
}

void wxTextCtrl::SetMaxLength(unsigned long len)
{
#if wxUSE_RICHEDIT
    if ( IsRich() )
    {
        ::SendMessage(GetHwnd(), EM_EXLIMITTEXT, 0, len ? len : 0x7fffffff);
    }
    else
#endif // wxUSE_RICHEDIT
    {
        if ( len >= 0xffff )
        {
            // this will set it to a platform-dependent maximum (much more
            // than 64Kb under NT)
            len = 0;
        }

        ::SendMessage(GetHwnd(), EM_LIMITTEXT, len, 0);
    }
}

// ----------------------------------------------------------------------------
// Undo/redo
// ----------------------------------------------------------------------------

void wxTextCtrl::Undo()
{
    if (CanUndo())
    {
        ::SendMessage(GetHwnd(), EM_UNDO, 0, 0);
    }
}

void wxTextCtrl::Redo()
{
    if (CanRedo())
    {
#if wxUSE_RICHEDIT
        if (GetRichVersion() > 1)
            ::SendMessage(GetHwnd(), EM_REDO, 0, 0);
        else
#endif
        // Same as Undo, since Undo undoes the undo, i.e. a redo.
        ::SendMessage(GetHwnd(), EM_UNDO, 0, 0);
    }
}

bool wxTextCtrl::CanUndo() const
{
    return ::SendMessage(GetHwnd(), EM_CANUNDO, 0, 0) != 0;
}

bool wxTextCtrl::CanRedo() const
{
#if wxUSE_RICHEDIT
    if (GetRichVersion() > 1)
        return ::SendMessage(GetHwnd(), EM_CANREDO, 0, 0) != 0;
    else
#endif
    return ::SendMessage(GetHwnd(), EM_CANUNDO, 0, 0) != 0;
}

// ----------------------------------------------------------------------------
// caret handling (Windows only)
// ----------------------------------------------------------------------------

bool wxTextCtrl::ShowNativeCaret(bool show)
{
    if ( show != m_isNativeCaretShown )
    {
        if ( !(show ? ::ShowCaret(GetHwnd()) : ::HideCaret(GetHwnd())) )
        {
            // not an error, may simply indicate that it's not shown/hidden
            // yet (i.e. it had been hidden/showh 2 times before)
            return false;
        }

        m_isNativeCaretShown = show;
    }

    return true;
}

// ----------------------------------------------------------------------------
// implemenation details
// ----------------------------------------------------------------------------

void wxTextCtrl::Command(wxCommandEvent & event)
{
    SetValue(event.GetString());
    ProcessCommand (event);
}

void wxTextCtrl::OnDropFiles(wxDropFilesEvent& event)
{
    // By default, load the first file into the text window.
    if (event.GetNumberOfFiles() > 0)
    {
        LoadFile(event.GetFiles()[0]);
    }
}

// ----------------------------------------------------------------------------
// kbd input processing
// ----------------------------------------------------------------------------

bool wxTextCtrl::MSWShouldPreProcessMessage(WXMSG* msg)
{
    // check for our special keys here: if we don't do it and the parent frame
    // uses them as accelerators, they wouldn't work at all, so we disable
    // usual preprocessing for them
    if ( msg->message == WM_KEYDOWN )
    {
        const WPARAM vkey = msg->wParam;
        if ( HIWORD(msg->lParam) & KF_ALTDOWN )
        {
            // Alt-Backspace is accelerator for "Undo"
            if ( vkey == VK_BACK )
                return false;
        }
        else // no Alt
        {
            // we want to process some Ctrl-foo and Shift-bar but no key
            // combinations without either Ctrl or Shift nor with both of them
            // pressed
            const int ctrl = wxIsCtrlDown(),
                      shift = wxIsShiftDown();
            switch ( ctrl + shift )
            {
                default:
                    wxFAIL_MSG( _T("how many modifiers have we got?") );
                    // fall through

                case 0:
                    if ( IsMultiLine() && vkey == VK_RETURN )
                        return false;
                    // fall through
                case 2:
                    break;

                case 1:
                    // either Ctrl or Shift pressed
                    if ( ctrl )
                    {
                        switch ( vkey )
                        {
                            case 'C':
                            case 'V':
                            case 'X':
                            case VK_INSERT:
                            case VK_DELETE:
                            case VK_HOME:
                            case VK_END:
                                return false;
                        }
                    }
                    else // Shift is pressed
                    {
                        if ( vkey == VK_INSERT || vkey == VK_DELETE )
                            return false;
                    }
            }
        }
    }

    return wxControl::MSWShouldPreProcessMessage(msg);
}

void wxTextCtrl::OnChar(wxKeyEvent& event)
{
    switch ( event.GetKeyCode() )
    {
        case WXK_RETURN:
            {
                wxCommandEvent event(wxEVT_COMMAND_TEXT_ENTER, m_windowId);
                InitCommandEvent(event);
                event.SetString(GetValue());
                if ( GetEventHandler()->ProcessEvent(event) )
                if ( !HasFlag(wxTE_MULTILINE) )
                    return;
                //else: multiline controls need Enter for themselves
            }
            break;

        case WXK_TAB:
            // ok, so this is getting absolutely ridiculous but I don't see
            // any other way to fix this bug: when a multiline text control is
            // inside a wxFrame, we need to generate the navigation event as
            // otherwise nothing happens at all, but when the same control is
            // created inside a dialog, IsDialogMessage() *does* switch focus
            // all by itself and so if we do it here as well, it is advanced
            // twice and goes to the next control... to prevent this from
            // happening we're doing this ugly check, the logic being that if
            // we don't have focus then it had been already changed to the next
            // control
            //
            // the right thing to do would, of course, be to understand what
            // the hell is IsDialogMessage() doing but this is beyond my feeble
            // forces at the moment unfortunately
            if ( !(m_windowStyle & wxTE_PROCESS_TAB))
            {
                if ( FindFocus() == this )
                {
                    int flags = 0;
                    if (!event.ShiftDown())
                        flags |= wxNavigationKeyEvent::IsForward ;
                    if (event.ControlDown())
                        flags |= wxNavigationKeyEvent::WinChange ;
                    if (Navigate(flags))
                        return;
                }
            }
            else
            {
                // Insert tab since calling the default Windows handler
                // doesn't seem to do it
                WriteText(wxT("\t"));
                return;
            }
            break;
    }

    // no, we didn't process it
    event.Skip();
}

void wxTextCtrl::OnKeyDown(wxKeyEvent& event)
{
    // richedit control doesn't send WM_PASTE, WM_CUT and WM_COPY messages
    // when Ctrl-V, X or C is pressed and this prevents wxClipboardTextEvent
    // from working. So we work around it by intercepting these shortcuts
    // ourselves and emitting clipboard events (which richedit will handle,
    // so everything works as before, including pasting of rich text):
    if ( event.GetModifiers() == wxMOD_CONTROL && IsRich() )
    {
        switch ( event.GetKeyCode() )
        {
            case 'C':
                Copy();
                return;
            case 'X':
                Cut();
                return;
            case 'V':
                Paste();
                return;
            default:
                break;
        }
    }

    // no, we didn't process it
    event.Skip();
}

WXLRESULT wxTextCtrl::MSWWindowProc(WXUINT nMsg, WXWPARAM wParam, WXLPARAM lParam)
{
    // we must handle clipboard events before calling MSWWindowProc, otherwise
    // the event would be handled twice if there's a handler for it in user
    // code:
    switch ( nMsg )
    {
        case WM_CUT:
        case WM_COPY:
        case WM_PASTE:
            if ( HandleClipboardEvent(nMsg) )
                return 0;
            break;
    }

    WXLRESULT lRc = wxTextCtrlBase::MSWWindowProc(nMsg, wParam, lParam);

    switch ( nMsg )
    {
        case WM_GETDLGCODE:
            {
                // we always want the chars and the arrows: the arrows for
                // navigation and the chars because we want Ctrl-C to work even
                // in a read only control
                long lDlgCode = DLGC_WANTCHARS | DLGC_WANTARROWS;

                if ( IsEditable() )
                {
                    // we may have several different cases:
                    // 1. normal: both TAB and ENTER are used for navigation
                    // 2. ctrl wants TAB for itself: ENTER is used to pass to
                    //    the next control in the dialog
                    // 3. ctrl wants ENTER for itself: TAB is used for dialog
                    //    navigation
                    // 4. ctrl wants both TAB and ENTER: Ctrl-ENTER is used to
                    //    go to the next control (we need some way to do it)

                    // multiline controls should always get ENTER for themselves
                    if ( HasFlag(wxTE_PROCESS_ENTER) || HasFlag(wxTE_MULTILINE) )
                        lDlgCode |= DLGC_WANTMESSAGE;

                    if ( HasFlag(wxTE_PROCESS_TAB) )
                        lDlgCode |= DLGC_WANTTAB;

                    lRc |= lDlgCode;
                }
                else // !editable
                {
                    // NB: use "=", not "|=" as the base class version returns
                    //     the same flags is this state as usual (i.e.
                    //     including DLGC_WANTMESSAGE). This is strange (how
                    //     does it work in the native Win32 apps?) but for now
                    //     live with it.
                    lRc = lDlgCode;
                }
            }
            break;
    }

    return lRc;
}

// ----------------------------------------------------------------------------
// text control event processing
// ----------------------------------------------------------------------------

bool wxTextCtrl::SendUpdateEvent()
{
    switch ( m_updatesCount )
    {
        case 0:
            // remember that we've got an update
            m_updatesCount++;
            break;

        case 1:
            // we had already sent one event since the last control modification
            return false;

        default:
            wxFAIL_MSG( _T("unexpected wxTextCtrl::m_updatesCount value") );
            // fall through

        case -1:
            // we hadn't updated the control ourselves, this event comes from
            // the user, don't need to ignore it nor update the count
            break;

        case -2:
            // the control was updated programmatically and we do NOT want to
            // send events
            return false;
    }

    wxCommandEvent event(wxEVT_COMMAND_TEXT_UPDATED, GetId());
    InitCommandEvent(event);

    return ProcessCommand(event);
}

bool wxTextCtrl::MSWCommand(WXUINT param, WXWORD WXUNUSED(id))
{
    switch ( param )
    {
        case EN_SETFOCUS:
        case EN_KILLFOCUS:
            {
                wxFocusEvent event(param == EN_KILLFOCUS ? wxEVT_KILL_FOCUS
                                                         : wxEVT_SET_FOCUS,
                                   m_windowId);
                event.SetEventObject(this);
                GetEventHandler()->ProcessEvent(event);
            }
            break;

        case EN_CHANGE:
            SendUpdateEvent();
            break;

        case EN_MAXTEXT:
            // the text size limit has been hit -- try to increase it
            if ( !AdjustSpaceLimit() )
            {
                wxCommandEvent event(wxEVT_COMMAND_TEXT_MAXLEN, m_windowId);
                InitCommandEvent(event);
                event.SetString(GetValue());
                ProcessCommand(event);
            }
            break;

            // the other edit notification messages are not processed
        default:
            return false;
    }

    // processed
    return true;
}

WXHBRUSH wxTextCtrl::MSWControlColor(WXHDC hDC, WXHWND hWnd)
{
    if ( !IsEnabled() && !HasFlag(wxTE_MULTILINE) )
        return MSWControlColorDisabled(hDC);

    return wxTextCtrlBase::MSWControlColor(hDC, hWnd);
}

bool wxTextCtrl::HasSpaceLimit(unsigned int *len) const
{
    // HACK: we try to automatically extend the limit for the amount of text
    //       to allow (interactively) entering more than 64Kb of text under
    //       Win9x but we shouldn't reset the text limit which was previously
    //       set explicitly with SetMaxLength()
    //
    //       Unfortunately there is no EM_GETLIMITTEXTSETBYUSER and so we don't
    //       know the limit we set (if any). We could solve this by storing the
    //       limit we set in wxTextCtrl but to save space we prefer to simply
    //       test here the actual limit value: we consider that SetMaxLength()
    //       can only be called for small values while EN_MAXTEXT is only sent
    //       for large values (in practice the default limit seems to be 30000
    //       but make it smaller just to be on the safe side)
    *len = ::SendMessage(GetHwnd(), EM_GETLIMITTEXT, 0, 0);
    return *len < 10001;

}

bool wxTextCtrl::AdjustSpaceLimit()
{
    unsigned int limit;
    if ( HasSpaceLimit(&limit) )
        return false;

    unsigned int len = ::GetWindowTextLength(GetHwnd());
    if ( len >= limit )
    {
        // increment in 32Kb chunks
        SetMaxLength(len + 0x8000);
    }

    // we changed the limit
    return true;
}

bool wxTextCtrl::AcceptsFocus() const
{
    // we don't want focus if we can't be edited unless we're a multiline
    // control because then it might be still nice to get focus from keyboard
    // to be able to scroll it without mouse
    return (IsEditable() || IsMultiLine()) && wxControl::AcceptsFocus();
}

wxSize wxTextCtrl::DoGetBestSize() const
{
    int cx, cy;
    wxGetCharSize(GetHWND(), &cx, &cy, GetFont());

    int wText = DEFAULT_ITEM_WIDTH;

    int hText = cy;
    if ( m_windowStyle & wxTE_MULTILINE )
    {
        hText *= wxMax(wxMin(GetNumberOfLines(), 10), 2);
    }
    //else: for single line control everything is ok

    // we have to add the adjustments for the control height only once, not
    // once per line, so do it after multiplication above
    hText += EDIT_HEIGHT_FROM_CHAR_HEIGHT(cy) - cy;

    return wxSize(wText, hText);
}

// ----------------------------------------------------------------------------
// standard handlers for standard edit menu events
// ----------------------------------------------------------------------------

void wxTextCtrl::OnCut(wxCommandEvent& WXUNUSED(event))
{
    Cut();
}

void wxTextCtrl::OnCopy(wxCommandEvent& WXUNUSED(event))
{
    Copy();
}

void wxTextCtrl::OnPaste(wxCommandEvent& WXUNUSED(event))
{
    Paste();
}

void wxTextCtrl::OnUndo(wxCommandEvent& WXUNUSED(event))
{
    Undo();
}

void wxTextCtrl::OnRedo(wxCommandEvent& WXUNUSED(event))
{
    Redo();
}

void wxTextCtrl::OnDelete(wxCommandEvent& WXUNUSED(event))
{
    long from, to;
    GetSelection(& from, & to);
    if (from != -1 && to != -1)
        Remove(from, to);
}

void wxTextCtrl::OnSelectAll(wxCommandEvent& WXUNUSED(event))
{
    SetSelection(-1, -1);
}

void wxTextCtrl::OnUpdateCut(wxUpdateUIEvent& event)
{
    event.Enable( CanCut() );
}

void wxTextCtrl::OnUpdateCopy(wxUpdateUIEvent& event)
{
    event.Enable( CanCopy() );
}

void wxTextCtrl::OnUpdatePaste(wxUpdateUIEvent& event)
{
    event.Enable( CanPaste() );
}

void wxTextCtrl::OnUpdateUndo(wxUpdateUIEvent& event)
{
    event.Enable( CanUndo() );
}

void wxTextCtrl::OnUpdateRedo(wxUpdateUIEvent& event)
{
    event.Enable( CanRedo() );
}

void wxTextCtrl::OnUpdateDelete(wxUpdateUIEvent& event)
{
    long from, to;
    GetSelection(& from, & to);
    event.Enable(from != -1 && to != -1 && from != to && IsEditable()) ;
}

void wxTextCtrl::OnUpdateSelectAll(wxUpdateUIEvent& event)
{
    event.Enable(GetLastPosition() > 0);
}

void wxTextCtrl::OnContextMenu(wxContextMenuEvent& event)
{
#if wxUSE_RICHEDIT
    if (IsRich())
    {
        if (!m_privateContextMenu)
        {
            m_privateContextMenu = new wxMenu;
            m_privateContextMenu->Append(wxID_UNDO, _("&Undo"));
            m_privateContextMenu->Append(wxID_REDO, _("&Redo"));
            m_privateContextMenu->AppendSeparator();
            m_privateContextMenu->Append(wxID_CUT, _("Cu&t"));
            m_privateContextMenu->Append(wxID_COPY, _("&Copy"));
            m_privateContextMenu->Append(wxID_PASTE, _("&Paste"));
            m_privateContextMenu->Append(wxID_CLEAR, _("&Delete"));
            m_privateContextMenu->AppendSeparator();
            m_privateContextMenu->Append(wxID_SELECTALL, _("Select &All"));
        }
        PopupMenu(m_privateContextMenu);
        return;
    }
    else
#endif
    event.Skip();
}

void wxTextCtrl::OnSetFocus(wxFocusEvent& event)
{
    // be sure the caret remains invisible if the user had hidden it
    if ( !m_isNativeCaretShown )
    {
        ::HideCaret(GetHwnd());
    }

    event.Skip();
}

// ----------------------------------------------------------------------------
// Default colors for MSW text control
//
// Set default background color to the native white instead of
// the default wxSYS_COLOUR_BTNFACE (is triggered with wxNullColour).
// ----------------------------------------------------------------------------

wxVisualAttributes wxTextCtrl::GetDefaultAttributes() const
{
    wxVisualAttributes attrs;
    attrs.font = wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT);
    attrs.colFg = wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT);
    attrs.colBg = wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW); //white

    return attrs;
}

// the rest of the file only deals with the rich edit controls
#if wxUSE_RICHEDIT

// ----------------------------------------------------------------------------
// EN_LINK processing
// ----------------------------------------------------------------------------

bool wxTextCtrl::MSWOnNotify(int idCtrl, WXLPARAM lParam, WXLPARAM *result)
{
    NMHDR *hdr = (NMHDR* )lParam;
    switch ( hdr->code )
    {
       case EN_MSGFILTER:
            {
                const MSGFILTER *msgf = (MSGFILTER *)lParam;
                UINT msg = msgf->msg;

                // this is a bit crazy but richedit 1.0 sends us all mouse
                // events _except_ WM_LBUTTONUP (don't ask me why) so we have
                // generate the wxWin events for this message manually
                //
                // NB: in fact, this is still not totally correct as it does
                //     send us WM_LBUTTONUP if the selection was cleared by the
                //     last click -- so currently we get 2 events in this case,
                //     but as I don't see any obvious way to check for this I
                //     leave this code in place because it's still better than
                //     not getting left up events at all
                if ( msg == WM_LBUTTONUP )
                {
                    WXUINT flags = msgf->wParam;
                    int x = GET_X_LPARAM(msgf->lParam),
                        y = GET_Y_LPARAM(msgf->lParam);

                    HandleMouseEvent(msg, x, y, flags);
                }
            }

            // return true to process the event (and false to ignore it)
            return true;

        case EN_LINK:
            {
                const ENLINK *enlink = (ENLINK *)hdr;

                switch ( enlink->msg )
                {
                    case WM_SETCURSOR:
                        // ok, so it is hardcoded - do we really nee to
                        // customize it?
                        {
                            wxCursor cur(wxCURSOR_HAND);
                            ::SetCursor(GetHcursorOf(cur));
                            *result = TRUE;
                            break;
                        }

                    case WM_MOUSEMOVE:
                    case WM_LBUTTONDOWN:
                    case WM_LBUTTONUP:
                    case WM_LBUTTONDBLCLK:
                    case WM_RBUTTONDOWN:
                    case WM_RBUTTONUP:
                    case WM_RBUTTONDBLCLK:
                        // send a mouse event
                        {
                            static const wxEventType eventsMouse[] =
                            {
                                wxEVT_MOTION,
                                wxEVT_LEFT_DOWN,
                                wxEVT_LEFT_UP,
                                wxEVT_LEFT_DCLICK,
                                wxEVT_RIGHT_DOWN,
                                wxEVT_RIGHT_UP,
                                wxEVT_RIGHT_DCLICK,
                            };

                            // the event ids are consecutive
                            wxMouseEvent
                                evtMouse(eventsMouse[enlink->msg - WM_MOUSEMOVE]);

                            InitMouseEvent(evtMouse,
                                           GET_X_LPARAM(enlink->lParam),
                                           GET_Y_LPARAM(enlink->lParam),
                                           enlink->wParam);

                            wxTextUrlEvent event(m_windowId, evtMouse,
                                                 enlink->chrg.cpMin,
                                                 enlink->chrg.cpMax);

                            InitCommandEvent(event);

                            *result = ProcessCommand(event);
                        }
                        break;
                }
            }
            return true;
    }

    // not processed, leave it to the base class
    return wxTextCtrlBase::MSWOnNotify(idCtrl, lParam, result);
}

// ----------------------------------------------------------------------------
// colour setting for the rich edit controls
// ----------------------------------------------------------------------------

bool wxTextCtrl::SetBackgroundColour(const wxColour& colour)
{
    if ( !wxTextCtrlBase::SetBackgroundColour(colour) )
    {
        // colour didn't really change
        return false;
    }

    if ( IsRich() )
    {
        // rich edit doesn't use WM_CTLCOLOR, hence we need to send
        // EM_SETBKGNDCOLOR additionally
        ::SendMessage(GetHwnd(), EM_SETBKGNDCOLOR, 0, wxColourToRGB(colour));
    }

    return true;
}

bool wxTextCtrl::SetForegroundColour(const wxColour& colour)
{
    if ( !wxTextCtrlBase::SetForegroundColour(colour) )
    {
        // colour didn't really change
        return false;
    }

    if ( IsRich() )
    {
        // change the colour of everything
        CHARFORMAT cf;
        wxZeroMemory(cf);
        cf.cbSize = sizeof(cf);
        cf.dwMask = CFM_COLOR;
        cf.crTextColor = wxColourToRGB(colour);
        ::SendMessage(GetHwnd(), EM_SETCHARFORMAT, SCF_ALL, (LPARAM)&cf);
    }

    return true;
}

// ----------------------------------------------------------------------------
// styling support for rich edit controls
// ----------------------------------------------------------------------------

#if wxUSE_RICHEDIT

bool wxTextCtrl::SetStyle(long start, long end, const wxTextAttr& style)
{
    if ( !IsRich() )
    {
        // can't do it with normal text control
        return false;
    }

    // the richedit 1.0 doesn't handle setting background colour, so don't
    // even try to do anything if it's the only thing we want to change
    if ( m_verRichEdit == 1 && !style.HasFont() && !style.HasTextColour() &&
        !style.HasLeftIndent() && !style.HasRightIndent() && !style.HasAlignment() &&
        !style.HasTabs() )
    {
        // nothing to do: return true if there was really nothing to do and
        // false if we failed to set bg colour
        return !style.HasBackgroundColour();
    }

    // order the range if needed
    if ( start > end )
    {
        long tmp = start;
        start = end;
        end = tmp;
    }

    // we can only change the format of the selection, so select the range we
    // want and restore the old selection later
    long startOld, endOld;
    GetSelection(&startOld, &endOld);

    // but do we really have to change the selection?
    bool changeSel = start != startOld || end != endOld;

    if ( changeSel )
    {
        DoSetSelection(start, end, false /* don't scroll caret into view */);
    }

    // initialize CHARFORMAT struct
#if wxUSE_RICHEDIT2
    CHARFORMAT2 cf;
#else
    CHARFORMAT cf;
#endif

    wxZeroMemory(cf);

    // we can't use CHARFORMAT2 with RichEdit 1.0, so pretend it is a simple
    // CHARFORMAT in that case
#if wxUSE_RICHEDIT2
    if ( m_verRichEdit == 1 )
    {
        // this is the only thing the control is going to grok
        cf.cbSize = sizeof(CHARFORMAT);
    }
    else
#endif
    {
        // CHARFORMAT or CHARFORMAT2
        cf.cbSize = sizeof(cf);
    }

    if ( style.HasFont() )
    {
        // VZ: CFM_CHARSET doesn't seem to do anything at all in RichEdit 2.0
        //     but using it doesn't seem to hurt neither so leaving it for now

        cf.dwMask |= CFM_FACE | CFM_SIZE | CFM_CHARSET |
                     CFM_ITALIC | CFM_BOLD | CFM_UNDERLINE;

        // fill in data from LOGFONT but recalculate lfHeight because we need
        // the real height in twips and not the negative number which
        // wxFillLogFont() returns (this is correct in general and works with
        // the Windows font mapper, but not here)
        LOGFONT lf;
        wxFillLogFont(&lf, &style.GetFont());
        cf.yHeight = 20*style.GetFont().GetPointSize(); // 1 pt = 20 twips
        cf.bCharSet = lf.lfCharSet;
        cf.bPitchAndFamily = lf.lfPitchAndFamily;
        wxStrncpy( cf.szFaceName, lf.lfFaceName, WXSIZEOF(cf.szFaceName) );

        // also deal with underline/italic/bold attributes: note that we must
        // always set CFM_ITALIC &c bits in dwMask, even if we don't set the
        // style to allow clearing it
        if ( lf.lfItalic )
        {
            cf.dwEffects |= CFE_ITALIC;
        }

        if ( lf.lfWeight == FW_BOLD )
        {
            cf.dwEffects |= CFE_BOLD;
        }

        if ( lf.lfUnderline )
        {
            cf.dwEffects |= CFE_UNDERLINE;
        }

        // strikeout fonts are not supported by wxWidgets
    }

    if ( style.HasTextColour() )
    {
        cf.dwMask |= CFM_COLOR;
        cf.crTextColor = wxColourToRGB(style.GetTextColour());
    }

#if wxUSE_RICHEDIT2
    if ( m_verRichEdit != 1 && style.HasBackgroundColour() )
    {
        cf.dwMask |= CFM_BACKCOLOR;
        cf.crBackColor = wxColourToRGB(style.GetBackgroundColour());
    }
#endif // wxUSE_RICHEDIT2

    // do format the selection
    bool ok = ::SendMessage(GetHwnd(), EM_SETCHARFORMAT,
                            SCF_SELECTION, (LPARAM)&cf) != 0;
    if ( !ok )
    {
        wxLogDebug(_T("SendMessage(EM_SETCHARFORMAT, SCF_SELECTION) failed"));
    }

    // now do the paragraph formatting
    PARAFORMAT2 pf;
    wxZeroMemory(pf);
    // we can't use PARAFORMAT2 with RichEdit 1.0, so pretend it is a simple
    // PARAFORMAT in that case
#if wxUSE_RICHEDIT2
    if ( m_verRichEdit == 1 )
    {
        // this is the only thing the control is going to grok
        pf.cbSize = sizeof(PARAFORMAT);
    }
    else
#endif
    {
        // PARAFORMAT or PARAFORMAT2
        pf.cbSize = sizeof(pf);
    }

    if (style.HasAlignment())
    {
        pf.dwMask |= PFM_ALIGNMENT;
        if (style.GetAlignment() == wxTEXT_ALIGNMENT_RIGHT)
            pf.wAlignment = PFA_RIGHT;
        else if (style.GetAlignment() == wxTEXT_ALIGNMENT_CENTRE)
            pf.wAlignment = PFA_CENTER;
        else if (style.GetAlignment() == wxTEXT_ALIGNMENT_JUSTIFIED)
            pf.wAlignment = PFA_JUSTIFY;
        else
            pf.wAlignment = PFA_LEFT;
    }

    if (style.HasLeftIndent())
    {
        pf.dwMask |= PFM_STARTINDENT | PFM_OFFSET;

        // Convert from 1/10 mm to TWIPS
        pf.dxStartIndent = (int) (((double) style.GetLeftIndent()) * mm2twips / 10.0) ;
        pf.dxOffset = (int) (((double) style.GetLeftSubIndent()) * mm2twips / 10.0) ;
    }

    if (style.HasRightIndent())
    {
        pf.dwMask |= PFM_RIGHTINDENT;

        // Convert from 1/10 mm to TWIPS
        pf.dxRightIndent = (int) (((double) style.GetRightIndent()) * mm2twips / 10.0) ;
    }

    if (style.HasTabs())
    {
        pf.dwMask |= PFM_TABSTOPS;

        const wxArrayInt& tabs = style.GetTabs();

        pf.cTabCount = (SHORT)wxMin(tabs.GetCount(), MAX_TAB_STOPS);
        size_t i;
        for (i = 0; i < (size_t) pf.cTabCount; i++)
        {
            // Convert from 1/10 mm to TWIPS
            pf.rgxTabs[i] = (int) (((double) tabs[i]) * mm2twips / 10.0) ;
        }
    }

#if wxUSE_RICHEDIT2
    if ( m_verRichEdit > 1 )
    {
        if ( wxTheApp->GetLayoutDirection() == wxLayout_RightToLeft )
        {
            // Use RTL paragraphs in RTL mode to get proper layout
            pf.dwMask |= PFM_RTLPARA;
            pf.wEffects |= PFE_RTLPARA;
        }
    }
#endif // wxUSE_RICHEDIT2

    if ( pf.dwMask )
    {
        // do format the selection
        bool ok = ::SendMessage(GetHwnd(), EM_SETPARAFORMAT,
                                0, (LPARAM) &pf) != 0;
        if ( !ok )
        {
            wxLogDebug(_T("SendMessage(EM_SETPARAFORMAT, 0) failed"));
        }
    }

    if ( changeSel )
    {
        // restore the original selection
        DoSetSelection(startOld, endOld, false);
    }

    return ok;
}

bool wxTextCtrl::SetDefaultStyle(const wxTextAttr& style)
{
    if ( !wxTextCtrlBase::SetDefaultStyle(style) )
        return false;

    if ( IsEditable() )
    {
        // we have to do this or the style wouldn't apply for the text typed by
        // the user
        wxTextPos posLast = GetLastPosition();
        SetStyle(posLast, posLast, m_defaultStyle);
    }

    return true;
}

bool wxTextCtrl::GetStyle(long position, wxTextAttr& style)
{
    if ( !IsRich() )
    {
        // can't do it with normal text control
        return false;
    }

    // initialize CHARFORMAT struct
#if wxUSE_RICHEDIT2
    CHARFORMAT2 cf;
#else
    CHARFORMAT cf;
#endif

    wxZeroMemory(cf);

    // we can't use CHARFORMAT2 with RichEdit 1.0, so pretend it is a simple
    // CHARFORMAT in that case
#if wxUSE_RICHEDIT2
    if ( m_verRichEdit == 1 )
    {
        // this is the only thing the control is going to grok
        cf.cbSize = sizeof(CHARFORMAT);
    }
    else
#endif
    {
        // CHARFORMAT or CHARFORMAT2
        cf.cbSize = sizeof(cf);
    }
    // we can only change the format of the selection, so select the range we
    // want and restore the old selection later
    long startOld, endOld;
    GetSelection(&startOld, &endOld);

    // but do we really have to change the selection?
    bool changeSel = position != startOld || position != endOld;

    if ( changeSel )
    {
        DoSetSelection(position, position+1, false /* don't scroll caret into view */);
    }

    // get the selection formatting
    (void) ::SendMessage(GetHwnd(), EM_GETCHARFORMAT,
                            SCF_SELECTION, (LPARAM)&cf) ;


    LOGFONT lf;
    lf.lfHeight = cf.yHeight;
    lf.lfWidth = 0;
    lf.lfCharSet = ANSI_CHARSET; // FIXME: how to get correct charset?
    lf.lfClipPrecision = 0;
    lf.lfEscapement = 0;
    wxStrcpy(lf.lfFaceName, cf.szFaceName);

    //NOTE:  we _MUST_ set each of these values to _something_ since we
    //do not call wxZeroMemory on the LOGFONT lf
    if (cf.dwEffects & CFE_ITALIC)
        lf.lfItalic = TRUE;
    else
        lf.lfItalic = FALSE;

    lf.lfOrientation = 0;
    lf.lfPitchAndFamily = cf.bPitchAndFamily;
    lf.lfQuality = 0;

    if (cf.dwEffects & CFE_STRIKEOUT)
        lf.lfStrikeOut = TRUE;
    else
        lf.lfStrikeOut = FALSE;

    if (cf.dwEffects & CFE_UNDERLINE)
        lf.lfUnderline = TRUE;
    else
        lf.lfUnderline = FALSE;

    if (cf.dwEffects & CFE_BOLD)
        lf.lfWeight = FW_BOLD;
    else
        lf.lfWeight = FW_NORMAL;

    wxFont font = wxCreateFontFromLogFont(& lf);
    if (font.Ok())
    {
        style.SetFont(font);
    }
    style.SetTextColour(wxColour(cf.crTextColor));

#if wxUSE_RICHEDIT2
    if ( m_verRichEdit != 1 )
    {
        // cf.dwMask |= CFM_BACKCOLOR;
        style.SetBackgroundColour(wxColour(cf.crBackColor));
    }
#endif // wxUSE_RICHEDIT2

    // now get the paragraph formatting
    PARAFORMAT2 pf;
    wxZeroMemory(pf);
    // we can't use PARAFORMAT2 with RichEdit 1.0, so pretend it is a simple
    // PARAFORMAT in that case
#if wxUSE_RICHEDIT2
    if ( m_verRichEdit == 1 )
    {
        // this is the only thing the control is going to grok
        pf.cbSize = sizeof(PARAFORMAT);
    }
    else
#endif
    {
        // PARAFORMAT or PARAFORMAT2
        pf.cbSize = sizeof(pf);
    }

    // do format the selection
    (void) ::SendMessage(GetHwnd(), EM_GETPARAFORMAT, 0, (LPARAM) &pf) ;

    style.SetLeftIndent( (int) ((double) pf.dxStartIndent * twips2mm * 10.0), (int) ((double) pf.dxOffset * twips2mm * 10.0) );
    style.SetRightIndent( (int) ((double) pf.dxRightIndent * twips2mm * 10.0) );

    if (pf.wAlignment == PFA_CENTER)
        style.SetAlignment(wxTEXT_ALIGNMENT_CENTRE);
    else if (pf.wAlignment == PFA_RIGHT)
        style.SetAlignment(wxTEXT_ALIGNMENT_RIGHT);
    else if (pf.wAlignment == PFA_JUSTIFY)
        style.SetAlignment(wxTEXT_ALIGNMENT_JUSTIFIED);
    else
        style.SetAlignment(wxTEXT_ALIGNMENT_LEFT);

    wxArrayInt tabStops;
    size_t i;
    for (i = 0; i < (size_t) pf.cTabCount; i++)
    {
        tabStops.Add( (int) ((double) (pf.rgxTabs[i] & 0xFFFF) * twips2mm * 10.0) );
    }

    if ( changeSel )
    {
        // restore the original selection
        DoSetSelection(startOld, endOld, false);
    }

    return true;
}

#endif

// ----------------------------------------------------------------------------
// wxRichEditModule
// ----------------------------------------------------------------------------

static const HINSTANCE INVALID_HINSTANCE = (HINSTANCE)-1;

bool wxRichEditModule::OnInit()
{
    // don't do anything - we will load it when needed
    return true;
}

void wxRichEditModule::OnExit()
{
    for ( size_t i = 0; i < WXSIZEOF(ms_hRichEdit); i++ )
    {
        if ( ms_hRichEdit[i] && ms_hRichEdit[i] != INVALID_HINSTANCE )
        {
            ::FreeLibrary(ms_hRichEdit[i]);
            ms_hRichEdit[i] = NULL;
        }
    }
#if wxUSE_INKEDIT
    if (ms_inkEditLib.IsLoaded())
        ms_inkEditLib.Unload();
#endif
}

/* static */
bool wxRichEditModule::Load(Version version)
{
    if ( ms_hRichEdit[version] == INVALID_HINSTANCE )
    {
        // we had already tried to load it and failed
        return false;
    }

    if ( ms_hRichEdit[version] )
    {
        // we've already got this one
        return true;
    }

    static const wxChar *dllnames[] =
    {
        _T("riched32"),
        _T("riched20"),
        _T("msftedit"),
    };

    wxCOMPILE_TIME_ASSERT( WXSIZEOF(dllnames) == Version_Max,
                            RichEditDllNamesVersionsMismatch );

    ms_hRichEdit[version] = ::LoadLibrary(dllnames[version]);

    if ( !ms_hRichEdit[version] )
    {
        ms_hRichEdit[version] = INVALID_HINSTANCE;

        return false;
    }

    return true;
}

#if wxUSE_INKEDIT
// load the InkEdit library
bool wxRichEditModule::LoadInkEdit()
{
    static wxDynamicLibrary ms_inkEditLib;
    static bool             ms_inkEditLibLoadAttemped;
    if (ms_inkEditLibLoadAttemped)
        ms_inkEditLib.IsLoaded();

    ms_inkEditLibLoadAttemped = true;

    wxLogNull logNull;
    return ms_inkEditLib.Load(wxT("inked"));
}
#endif


#endif // wxUSE_RICHEDIT

#endif // wxUSE_TEXTCTRL && !(__SMARTPHONE__ && __WXWINCE__)
