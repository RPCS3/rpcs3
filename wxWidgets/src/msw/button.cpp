/////////////////////////////////////////////////////////////////////////////
// Name:        src/msw/button.cpp
// Purpose:     wxButton
// Author:      Julian Smart
// Modified by:
// Created:     04/01/98
// RCS-ID:      $Id: button.cpp 51575 2008-02-06 19:58:30Z VZ $
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

#if wxUSE_BUTTON

#include "wx/button.h"

#ifndef WX_PRECOMP
    #include "wx/app.h"
    #include "wx/brush.h"
    #include "wx/panel.h"
    #include "wx/bmpbuttn.h"
    #include "wx/settings.h"
    #include "wx/dcscreen.h"
    #include "wx/dcclient.h"
    #include "wx/toplevel.h"
#endif

#include "wx/stockitem.h"
#include "wx/tokenzr.h"
#include "wx/msw/private.h"

#if wxUSE_UXTHEME
    #include "wx/msw/uxtheme.h"

    // no need to include tmschema.h
    #ifndef BP_PUSHBUTTON
        #define BP_PUSHBUTTON 1

        #define PBS_NORMAL    1
        #define PBS_HOT       2
        #define PBS_PRESSED   3
        #define PBS_DISABLED  4
        #define PBS_DEFAULTED 5

        #define TMT_CONTENTMARGINS 3602
    #endif
#endif // wxUSE_UXTHEME

#ifndef WM_THEMECHANGED
    #define WM_THEMECHANGED     0x031A
#endif

#ifndef ODS_NOACCEL
    #define ODS_NOACCEL         0x0100
#endif

#ifndef ODS_NOFOCUSRECT
    #define ODS_NOFOCUSRECT     0x0200
#endif

// ----------------------------------------------------------------------------
// macros
// ----------------------------------------------------------------------------

#if wxUSE_EXTENDED_RTTI

WX_DEFINE_FLAGS( wxButtonStyle )

wxBEGIN_FLAGS( wxButtonStyle )
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

    wxFLAGS_MEMBER(wxBU_LEFT)
    wxFLAGS_MEMBER(wxBU_RIGHT)
    wxFLAGS_MEMBER(wxBU_TOP)
    wxFLAGS_MEMBER(wxBU_BOTTOM)
    wxFLAGS_MEMBER(wxBU_EXACTFIT)
wxEND_FLAGS( wxButtonStyle )

IMPLEMENT_DYNAMIC_CLASS_XTI(wxButton, wxControl,"wx/button.h")

wxBEGIN_PROPERTIES_TABLE(wxButton)
    wxEVENT_PROPERTY( Click , wxEVT_COMMAND_BUTTON_CLICKED , wxCommandEvent)

    wxPROPERTY( Font , wxFont , SetFont , GetFont  , EMPTY_MACROVALUE, 0 /*flags*/ , wxT("Helpstring") , wxT("group"))
    wxPROPERTY( Label, wxString , SetLabel, GetLabel, wxString(), 0 /*flags*/ , wxT("Helpstring") , wxT("group") )

    wxPROPERTY_FLAGS( WindowStyle , wxButtonStyle , long , SetWindowStyleFlag , GetWindowStyleFlag , EMPTY_MACROVALUE , 0 /*flags*/ , wxT("Helpstring") , wxT("group")) // style

wxEND_PROPERTIES_TABLE()

wxBEGIN_HANDLERS_TABLE(wxButton)
wxEND_HANDLERS_TABLE()

wxCONSTRUCTOR_6( wxButton , wxWindow* , Parent , wxWindowID , Id , wxString , Label , wxPoint , Position , wxSize , Size , long , WindowStyle  )


#else
IMPLEMENT_DYNAMIC_CLASS(wxButton, wxControl)
#endif

// this macro tries to adjust the default button height to a reasonable value
// using the char height as the base
#define BUTTON_HEIGHT_FROM_CHAR_HEIGHT(cy) (11*EDIT_HEIGHT_FROM_CHAR_HEIGHT(cy)/10)

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// creation/destruction
// ----------------------------------------------------------------------------

bool wxButton::Create(wxWindow *parent,
                      wxWindowID id,
                      const wxString& lbl,
                      const wxPoint& pos,
                      const wxSize& size,
                      long style,
                      const wxValidator& validator,
                      const wxString& name)
{
    wxString label(lbl);
    if (label.empty() && wxIsStockID(id))
    {
        // On Windows, some buttons aren't supposed to have
        // mnemonics, so strip them out.

        label = wxGetStockLabel(id
#if defined(__WXMSW__) || defined(__WXWINCE__)
                                        , ( id != wxID_OK &&
                                            id != wxID_CANCEL &&
                                            id != wxID_CLOSE )
#endif
                                );
    }

    if ( !CreateControl(parent, id, pos, size, style, validator, name) )
        return false;

    WXDWORD exstyle;
    WXDWORD msStyle = MSWGetStyle(style, &exstyle);

#ifdef __WIN32__
    // if the label contains several lines we must explicitly tell the button
    // about it or it wouldn't draw it correctly ("\n"s would just appear as
    // black boxes)
    //
    // NB: we do it here and not in MSWGetStyle() because we need the label
    //     value and m_label is not set yet when MSWGetStyle() is called;
    //     besides changing BS_MULTILINE during run-time is pointless anyhow
    if ( label.find(_T('\n')) != wxString::npos )
    {
        msStyle |= BS_MULTILINE;
    }
#endif // __WIN32__

    return MSWCreateControl(_T("BUTTON"), msStyle, pos, size, label, exstyle);
}

wxButton::~wxButton()
{
    wxTopLevelWindow *tlw = wxDynamicCast(wxGetTopLevelParent(this), wxTopLevelWindow);
    if ( tlw && tlw->GetTmpDefaultItem() == this )
    {
        UnsetTmpDefault();
    }
}

// ----------------------------------------------------------------------------
// flags
// ----------------------------------------------------------------------------

WXDWORD wxButton::MSWGetStyle(long style, WXDWORD *exstyle) const
{
    // buttons never have an external border, they draw their own one
    WXDWORD msStyle = wxControl::MSWGetStyle
                      (
                        (style & ~wxBORDER_MASK) | wxBORDER_NONE, exstyle
                      );

    // we must use WS_CLIPSIBLINGS with the buttons or they would draw over
    // each other in any resizeable dialog which has more than one button in
    // the bottom
    msStyle |= WS_CLIPSIBLINGS;

#ifdef __WIN32__
    // don't use "else if" here: weird as it is, but you may combine wxBU_LEFT
    // and wxBU_RIGHT to get BS_CENTER!
    if ( style & wxBU_LEFT )
        msStyle |= BS_LEFT;
    if ( style & wxBU_RIGHT )
        msStyle |= BS_RIGHT;
    if ( style & wxBU_TOP )
        msStyle |= BS_TOP;
    if ( style & wxBU_BOTTOM )
        msStyle |= BS_BOTTOM;
#ifndef __WXWINCE__
    // flat 2d buttons
    if ( style & wxNO_BORDER )
        msStyle |= BS_FLAT;
#endif // __WXWINCE__
#endif // __WIN32__

    return msStyle;
}

// ----------------------------------------------------------------------------
// size management including autosizing
// ----------------------------------------------------------------------------

wxSize wxButton::DoGetBestSize() const
{
    wxClientDC dc(wx_const_cast(wxButton *, this));
    dc.SetFont(GetFont());

    wxCoord wBtn,
            hBtn;
    dc.GetMultiLineTextExtent(GetLabelText(), &wBtn, &hBtn);

    // add a margin -- the button is wider than just its label
    wBtn += 3*GetCharWidth();
    hBtn = BUTTON_HEIGHT_FROM_CHAR_HEIGHT(hBtn);

    // all buttons have at least the standard size unless the user explicitly
    // wants them to be of smaller size and used wxBU_EXACTFIT style when
    // creating the button
    if ( !HasFlag(wxBU_EXACTFIT) )
    {
        wxSize sz = GetDefaultSize();
        if (wBtn > sz.x)
            sz.x = wBtn;
        if (hBtn > sz.y)
            sz.y = hBtn;

        return sz;
    }

    wxSize best(wBtn, hBtn);
    CacheBestSize(best);
    return best;
}

/* static */
wxSize wxButtonBase::GetDefaultSize()
{
    static wxSize s_sizeBtn;

    if ( s_sizeBtn.x == 0 )
    {
        wxScreenDC dc;
        dc.SetFont(wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT));

        // the size of a standard button in the dialog units is 50x14,
        // translate this to pixels
        // NB1: the multipliers come from the Windows convention
        // NB2: the extra +1/+2 were needed to get the size be the same as the
        //      size of the buttons in the standard dialog - I don't know how
        //      this happens, but on my system this size is 75x23 in pixels and
        //      23*8 isn't even divisible by 14... Would be nice to understand
        //      why these constants are needed though!
        s_sizeBtn.x = (50 * (dc.GetCharWidth() + 1))/4;
        s_sizeBtn.y = ((14 * dc.GetCharHeight()) + 2)/8;
    }

    return s_sizeBtn;
}

// ----------------------------------------------------------------------------
// default button handling
// ----------------------------------------------------------------------------

/*
   "Everything you ever wanted to know about the default buttons" or "Why do we
   have to do all this?"

   In MSW the default button should be activated when the user presses Enter
   and the current control doesn't process Enter itself somehow. This is
   handled by ::DefWindowProc() (or maybe ::DefDialogProc()) using DM_SETDEFID
   Another aspect of "defaultness" is that the default button has different
   appearance: this is due to BS_DEFPUSHBUTTON style which is completely
   separate from DM_SETDEFID stuff (!). Also note that BS_DEFPUSHBUTTON should
   be unset if our parent window is not active so it should be unset whenever
   we lose activation and set back when we regain it.

   Final complication is that when a button is active, it should be the default
   one, i.e. pressing Enter on a button always activates it and not another
   one.

   We handle this by maintaining a permanent and a temporary default items in
   wxControlContainer (both may be NULL). When a button becomes the current
   control (i.e. gets focus) it sets itself as the temporary default which
   ensures that it has the right appearance and that Enter will be redirected
   to it. When the button loses focus, it unsets the temporary default and so
   the default item will be the permanent default -- that is the default button
   if any had been set or none otherwise, which is just what we want.

   NB: all this is quite complicated by now and the worst is that normally
       it shouldn't be necessary at all as for the normal Windows programs
       DefWindowProc() and IsDialogMessage() take care of all this
       automatically -- however in wxWidgets programs this doesn't work for
       nested hierarchies (i.e. a notebook inside a notebook) for unknown
       reason and so we have to reproduce all this code ourselves. It would be
       very nice if we could avoid doing it.
 */

// set this button as the (permanently) default one in its panel
void wxButton::SetDefault()
{
    wxTopLevelWindow *tlw = wxDynamicCast(wxGetTopLevelParent(this), wxTopLevelWindow);

    wxCHECK_RET( tlw, _T("button without top level window?") );

    // set this one as the default button both for wxWidgets ...
    wxWindow *winOldDefault = tlw->SetDefaultItem(this);

    // ... and Windows
    SetDefaultStyle(wxDynamicCast(winOldDefault, wxButton), false);
    SetDefaultStyle(this, true);
}

// return the top level parent window if it's not being deleted yet, otherwise
// return NULL
static wxTopLevelWindow *GetTLWParentIfNotBeingDeleted(wxWindow *win)
{
    for ( ;; )
    {
        // IsTopLevel() will return false for a wxTLW being deleted, so we also
        // need the parent test for this case
        wxWindow * const parent = win->GetParent();
        if ( !parent || win->IsTopLevel() )
        {
            if ( win->IsBeingDeleted() )
                return NULL;

            break;
        }

        win = parent;
    }

    wxASSERT_MSG( win, _T("button without top level parent?") );

    wxTopLevelWindow * const tlw = wxDynamicCast(win, wxTopLevelWindow);
    wxASSERT_MSG( tlw, _T("logic error in GetTLWParentIfNotBeingDeleted()") );

    return tlw;
}

// set this button as being currently default
void wxButton::SetTmpDefault()
{
    wxTopLevelWindow * const tlw = GetTLWParentIfNotBeingDeleted(GetParent());
    if ( !tlw )
        return;

    wxWindow *winOldDefault = tlw->GetDefaultItem();
    tlw->SetTmpDefaultItem(this);

    SetDefaultStyle(wxDynamicCast(winOldDefault, wxButton), false);
    SetDefaultStyle(this, true);
}

// unset this button as currently default, it may still stay permanent default
void wxButton::UnsetTmpDefault()
{
    wxTopLevelWindow * const tlw = GetTLWParentIfNotBeingDeleted(GetParent());
    if ( !tlw )
        return;

    tlw->SetTmpDefaultItem(NULL);

    wxWindow *winOldDefault = tlw->GetDefaultItem();

    SetDefaultStyle(this, false);
    SetDefaultStyle(wxDynamicCast(winOldDefault, wxButton), true);
}

/* static */
void
wxButton::SetDefaultStyle(wxButton *btn, bool on)
{
    // we may be called with NULL pointer -- simpler to do the check here than
    // in the caller which does wxDynamicCast()
    if ( !btn )
        return;

    // first, let DefDlgProc() know about the new default button
    if ( on )
    {
        // we shouldn't set BS_DEFPUSHBUTTON for any button if we don't have
        // focus at all any more
        if ( !wxTheApp->IsActive() )
            return;

        wxWindow * const tlw = wxGetTopLevelParent(btn);
        wxCHECK_RET( tlw, _T("button without top level window?") );

        ::SendMessage(GetHwndOf(tlw), DM_SETDEFID, btn->GetId(), 0L);

        // sending DM_SETDEFID also changes the button style to
        // BS_DEFPUSHBUTTON so there is nothing more to do
    }

    // then also change the style as needed
    long style = ::GetWindowLong(GetHwndOf(btn), GWL_STYLE);
    if ( !(style & BS_DEFPUSHBUTTON) == on )
    {
        // don't do it with the owner drawn buttons because it will
        // reset BS_OWNERDRAW style bit too (as BS_OWNERDRAW &
        // BS_DEFPUSHBUTTON != 0)!
        if ( (style & BS_OWNERDRAW) != BS_OWNERDRAW )
        {
            ::SendMessage(GetHwndOf(btn), BM_SETSTYLE,
                          on ? style | BS_DEFPUSHBUTTON
                             : style & ~BS_DEFPUSHBUTTON,
                          1L /* redraw */);
        }
        else // owner drawn
        {
            // redraw the button - it will notice itself that it's
            // [not] the default one [any longer]
            btn->Refresh();
        }
    }
    //else: already has correct style
}

// ----------------------------------------------------------------------------
// helpers
// ----------------------------------------------------------------------------

bool wxButton::SendClickEvent()
{
    wxCommandEvent event(wxEVT_COMMAND_BUTTON_CLICKED, GetId());
    event.SetEventObject(this);

    return ProcessCommand(event);
}

void wxButton::Command(wxCommandEvent & event)
{
    ProcessCommand(event);
}

// ----------------------------------------------------------------------------
// event/message handlers
// ----------------------------------------------------------------------------

bool wxButton::MSWCommand(WXUINT param, WXWORD WXUNUSED(id))
{
    bool processed = false;
    switch ( param )
    {
        // NOTE: Apparently older versions (NT 4?) of the common controls send
        //       BN_DOUBLECLICKED but not a second BN_CLICKED for owner-drawn
        //       buttons, so in order to send two EVT_BUTTON events we should
        //       catch both types.  Currently (Feb 2003) up-to-date versions of
        //       win98, win2k and winXP all send two BN_CLICKED messages for
        //       all button types, so we don't catch BN_DOUBLECLICKED anymore
        //       in order to not get 3 EVT_BUTTON events.  If this is a problem
        //       then we need to figure out which version of the comctl32 changed
        //       this behaviour and test for it.

        case 1:                     // message came from an accelerator
        case BN_CLICKED:            // normal buttons send this
            processed = SendClickEvent();
            break;
    }

    return processed;
}

WXLRESULT wxButton::MSWWindowProc(WXUINT nMsg, WXWPARAM wParam, WXLPARAM lParam)
{
    // when we receive focus, we want to temporarily become the default button in
    // our parent panel so that pressing "Enter" would activate us -- and when
    // losing it we should restore the previous default button as well
    if ( nMsg == WM_SETFOCUS )
    {
        SetTmpDefault();

        // let the default processing take place too
    }
    else if ( nMsg == WM_KILLFOCUS )
    {
        UnsetTmpDefault();
    }
    else if ( nMsg == WM_LBUTTONDBLCLK )
    {
        // emulate a click event to force an owner-drawn button to change its
        // appearance - without this, it won't do it
        (void)wxControl::MSWWindowProc(WM_LBUTTONDOWN, wParam, lParam);

        // and continue with processing the message normally as well
    }
#if wxUSE_UXTHEME
    else if ( nMsg == WM_THEMECHANGED )
    {
        // need to recalculate the best size here
        // as the theme size might have changed
        InvalidateBestSize();
    }
    else if ( wxUxThemeEngine::GetIfActive() )
    {
        // we need to Refresh() if mouse has entered or left window
        // so we can update the hot tracking state
        // must use m_mouseInWindow here instead of IsMouseInWindow()
        // since we need to know the first time the mouse enters the window
        // and IsMouseInWindow() would return true in this case
        if ( ( nMsg == WM_MOUSEMOVE && !m_mouseInWindow ) ||
             nMsg == WM_MOUSELEAVE )
        {
            Refresh();
        }
    }
#endif // wxUSE_UXTHEME

    // let the base class do all real processing
    return wxControl::MSWWindowProc(nMsg, wParam, lParam);
}

// ----------------------------------------------------------------------------
// owner-drawn buttons support
// ----------------------------------------------------------------------------

#ifdef __WIN32__

// drawing helpers

static void DrawButtonText(HDC hdc,
                           RECT *pRect,
                           const wxString& text,
                           COLORREF col)
{
    COLORREF colOld = SetTextColor(hdc, col);
    int modeOld = SetBkMode(hdc, TRANSPARENT);

    if ( text.find(_T('\n')) != wxString::npos )
    {
        // draw multiline label

        // first we need to compute its bounding rect
        RECT rc;
        ::CopyRect(&rc, pRect);
        ::DrawText(hdc, text, text.length(), &rc, DT_CENTER | DT_CALCRECT);

        // now center this rect inside the entire button area
        const LONG w = rc.right - rc.left;
        const LONG h = rc.bottom - rc.top;
        rc.left = (pRect->right - pRect->left)/2 - w/2;
        rc.right = rc.left+w;
        rc.top = (pRect->bottom - pRect->top)/2 - h/2;
        rc.bottom = rc.top+h;

        ::DrawText(hdc, text, text.length(), &rc, DT_CENTER);
    }
    else // single line label
    {
        // Note: we must have DT_SINGLELINE for DT_VCENTER to work.
        ::DrawText(hdc, text, text.length(), pRect,
                   DT_SINGLELINE | DT_CENTER | DT_VCENTER);
    }

    SetBkMode(hdc, modeOld);
    SetTextColor(hdc, colOld);
}

static void DrawRect(HDC hdc, const RECT& r)
{
    wxDrawLine(hdc, r.left, r.top, r.right, r.top);
    wxDrawLine(hdc, r.right, r.top, r.right, r.bottom);
    wxDrawLine(hdc, r.right, r.bottom, r.left, r.bottom);
    wxDrawLine(hdc, r.left, r.bottom, r.left, r.top);
}

void wxButton::MakeOwnerDrawn()
{
    long style = GetWindowLong(GetHwnd(), GWL_STYLE);
    if ( (style & BS_OWNERDRAW) != BS_OWNERDRAW )
    {
        // make it so
        style |= BS_OWNERDRAW;
        SetWindowLong(GetHwnd(), GWL_STYLE, style);
    }
}

bool wxButton::SetBackgroundColour(const wxColour &colour)
{
    if ( !wxControl::SetBackgroundColour(colour) )
    {
        // nothing to do
        return false;
    }

    MakeOwnerDrawn();

    Refresh();

    return true;
}

bool wxButton::SetForegroundColour(const wxColour &colour)
{
    if ( !wxControl::SetForegroundColour(colour) )
    {
        // nothing to do
        return false;
    }

    MakeOwnerDrawn();

    Refresh();

    return true;
}

/*
   The button frame looks like this normally:

   WWWWWWWWWWWWWWWWWWB
   WHHHHHHHHHHHHHHHHGB  W = white       (HILIGHT)
   WH               GB  H = light grey  (LIGHT)
   WH               GB  G = dark grey   (SHADOW)
   WH               GB  B = black       (DKSHADOW)
   WH               GB
   WGGGGGGGGGGGGGGGGGB
   BBBBBBBBBBBBBBBBBBB

   When the button is selected, the button becomes like this (the total button
   size doesn't change):

   BBBBBBBBBBBBBBBBBBB
   BWWWWWWWWWWWWWWWWBB
   BWHHHHHHHHHHHHHHGBB
   BWH             GBB
   BWH             GBB
   BWGGGGGGGGGGGGGGGBB
   BBBBBBBBBBBBBBBBBBB
   BBBBBBBBBBBBBBBBBBB

   When the button is pushed (while selected) it is like:

   BBBBBBBBBBBBBBBBBBB
   BGGGGGGGGGGGGGGGGGB
   BG               GB
   BG               GB
   BG               GB
   BG               GB
   BGGGGGGGGGGGGGGGGGB
   BBBBBBBBBBBBBBBBBBB
*/

static void DrawButtonFrame(HDC hdc, const RECT& rectBtn,
                            bool selected, bool pushed)
{
    RECT r;
    CopyRect(&r, &rectBtn);

    HPEN hpenBlack   = CreatePen(PS_SOLID, 1, GetSysColor(COLOR_3DDKSHADOW)),
         hpenGrey    = CreatePen(PS_SOLID, 1, GetSysColor(COLOR_3DSHADOW)),
         hpenLightGr = CreatePen(PS_SOLID, 1, GetSysColor(COLOR_3DLIGHT)),
         hpenWhite   = CreatePen(PS_SOLID, 1, GetSysColor(COLOR_3DHILIGHT));

    HPEN hpenOld = (HPEN)SelectObject(hdc, hpenBlack);

    r.right--;
    r.bottom--;

    if ( pushed )
    {
        DrawRect(hdc, r);

        (void)SelectObject(hdc, hpenGrey);
        ::InflateRect(&r, -1, -1);

        DrawRect(hdc, r);
    }
    else // !pushed
    {
        if ( selected )
        {
            DrawRect(hdc, r);

            ::InflateRect(&r, -1, -1);
        }

        wxDrawLine(hdc, r.left, r.bottom, r.right, r.bottom);
        wxDrawLine(hdc, r.right, r.bottom, r.right, r.top - 1);

        (void)SelectObject(hdc, hpenWhite);
        wxDrawLine(hdc, r.left, r.bottom - 1, r.left, r.top);
        wxDrawLine(hdc, r.left, r.top, r.right, r.top);

        (void)SelectObject(hdc, hpenLightGr);
        wxDrawLine(hdc, r.left + 1, r.bottom - 2, r.left + 1, r.top + 1);
        wxDrawLine(hdc, r.left + 1, r.top + 1, r.right - 1, r.top + 1);

        (void)SelectObject(hdc, hpenGrey);
        wxDrawLine(hdc, r.left + 1, r.bottom - 1, r.right - 1, r.bottom - 1);
        wxDrawLine(hdc, r.right - 1, r.bottom - 1, r.right - 1, r.top);
    }

    (void)SelectObject(hdc, hpenOld);
    DeleteObject(hpenWhite);
    DeleteObject(hpenLightGr);
    DeleteObject(hpenGrey);
    DeleteObject(hpenBlack);
}

#if wxUSE_UXTHEME
static
void MSWDrawXPBackground(wxButton *button, WXDRAWITEMSTRUCT *wxdis)
{
    LPDRAWITEMSTRUCT lpDIS = (LPDRAWITEMSTRUCT)wxdis;
    HDC hdc = lpDIS->hDC;
    UINT state = lpDIS->itemState;
    RECT rectBtn;
    CopyRect(&rectBtn, &lpDIS->rcItem);

    wxUxThemeHandle theme(button, L"BUTTON");
    int iState;

    if ( state & ODS_SELECTED )
    {
        iState = PBS_PRESSED;
    }
    else if ( button->HasCapture() || button->IsMouseInWindow() )
    {
        iState = PBS_HOT;
    }
    else if ( state & ODS_FOCUS )
    {
        iState = PBS_DEFAULTED;
    }
    else if ( state & ODS_DISABLED )
    {
        iState = PBS_DISABLED;
    }
    else
    {
        iState = PBS_NORMAL;
    }

    // draw parent background if needed
    if ( wxUxThemeEngine::Get()->IsThemeBackgroundPartiallyTransparent(theme,
                                                                       BP_PUSHBUTTON,
                                                                       iState) )
    {
        wxUxThemeEngine::Get()->DrawThemeParentBackground(GetHwndOf(button), hdc, &rectBtn);
    }

    // draw background
    wxUxThemeEngine::Get()->DrawThemeBackground(theme, hdc, BP_PUSHBUTTON, iState,
                                                &rectBtn, NULL);

    // calculate content area margins
    MARGINS margins;
    wxUxThemeEngine::Get()->GetThemeMargins(theme, hdc, BP_PUSHBUTTON, iState,
                                            TMT_CONTENTMARGINS, &rectBtn, &margins);
    RECT rectClient;
    ::CopyRect(&rectClient, &rectBtn);
    ::InflateRect(&rectClient, -margins.cxLeftWidth, -margins.cyTopHeight);

    // if focused and !nofocus rect
    if ( (state & ODS_FOCUS) && !(state & ODS_NOFOCUSRECT) )
    {
        DrawFocusRect(hdc, &rectClient);
    }

    if ( button->UseBgCol() )
    {
        COLORREF colBg = wxColourToRGB(button->GetBackgroundColour());
        HBRUSH hbrushBackground = ::CreateSolidBrush(colBg);

        // don't overwrite the focus rect
        ::InflateRect(&rectClient, -1, -1);
        FillRect(hdc, &rectClient, hbrushBackground);
        ::DeleteObject(hbrushBackground);
    }
}
#endif // wxUSE_UXTHEME

bool wxButton::MSWOnDraw(WXDRAWITEMSTRUCT *wxdis)
{
    LPDRAWITEMSTRUCT lpDIS = (LPDRAWITEMSTRUCT)wxdis;
    HDC hdc = lpDIS->hDC;
    UINT state = lpDIS->itemState;
    RECT rectBtn;
    CopyRect(&rectBtn, &lpDIS->rcItem);

#if wxUSE_UXTHEME
    if ( wxUxThemeEngine::GetIfActive() )
    {
        MSWDrawXPBackground(this, wxdis);
    }
    else
#endif // wxUSE_UXTHEME
    {
        COLORREF colBg = wxColourToRGB(GetBackgroundColour());

        // first, draw the background
        HBRUSH hbrushBackground = ::CreateSolidBrush(colBg);
        FillRect(hdc, &rectBtn, hbrushBackground);
        ::DeleteObject(hbrushBackground);

        // draw the border for the current state
        bool selected = (state & ODS_SELECTED) != 0;
        if ( !selected )
        {
            wxTopLevelWindow *tlw = wxDynamicCast(wxGetTopLevelParent(this), wxTopLevelWindow);
            if ( tlw )
            {
                selected = tlw->GetDefaultItem() == this;
            }
        }
        bool pushed = (SendMessage(GetHwnd(), BM_GETSTATE, 0, 0) & BST_PUSHED) != 0;

        DrawButtonFrame(hdc, rectBtn, selected, pushed);

        // if focused and !nofocus rect
        if ( (state & ODS_FOCUS) && !(state & ODS_NOFOCUSRECT) )
        {
            RECT rectFocus;
            CopyRect(&rectFocus, &rectBtn);

            // I don't know where does this constant come from, but this is how
            // Windows draws them
            InflateRect(&rectFocus, -4, -4);

            DrawFocusRect(hdc, &rectFocus);
        }

        if ( pushed )
        {
            // the label is shifted by 1 pixel to create "pushed" effect
            OffsetRect(&rectBtn, 1, 1);
        }
    }

    COLORREF colFg = wxColourToRGB(GetForegroundColour());
    if ( state & ODS_DISABLED ) colFg = GetSysColor(COLOR_GRAYTEXT) ;
    wxString label = GetLabel();
    if ( state & ODS_NOACCEL ) label = GetLabelText() ;
    DrawButtonText(hdc, &rectBtn, label, colFg);

    return true;
}

#endif // __WIN32__

#endif // wxUSE_BUTTON
