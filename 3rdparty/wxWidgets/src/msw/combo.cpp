/////////////////////////////////////////////////////////////////////////////
// Name:        src/msw/combo.cpp
// Purpose:     wxMSW wxComboCtrl
// Author:      Jaakko Salli
// Modified by:
// Created:     Apr-30-2006
// RCS-ID:      $Id: combo.cpp 48043 2007-08-13 11:13:03Z JS $
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

#ifndef WX_PRECOMP
    #include "wx/log.h"
    #include "wx/combobox.h"
    #include "wx/dcclient.h"
    #include "wx/settings.h"
    #include "wx/dialog.h"
    #include "wx/stopwatch.h"
#endif

#include "wx/dcbuffer.h"
#include "wx/combo.h"

#include "wx/msw/registry.h"

#if wxUSE_UXTHEME
#include "wx/msw/uxtheme.h"
#endif

// Change to #if 1 to include tmschema.h for easier testing of theme
// parameters.
#if 0
    #include <tmschema.h>
#else
    //----------------------------------
    #define EP_EDITTEXT         1
    #define ETS_NORMAL          1
    #define ETS_HOT             2
    #define ETS_SELECTED        3
    #define ETS_DISABLED        4
    #define ETS_FOCUSED         5
    #define ETS_READONLY        6
    #define ETS_ASSIST          7
    #define TMT_FILLCOLOR       3802
    #define TMT_TEXTCOLOR       3803
    #define TMT_BORDERCOLOR     3801
    #define TMT_EDGEFILLCOLOR   3808
    //----------------------------------
#endif


#define NATIVE_TEXT_INDENT_XP       4
#define NATIVE_TEXT_INDENT_CLASSIC  2

#define TEXTCTRLXADJUST_XP          1
#define TEXTCTRLYADJUST_XP          3
#define TEXTCTRLXADJUST_CLASSIC     1
#define TEXTCTRLYADJUST_CLASSIC     2

#define COMBOBOX_ANIMATION_RESOLUTION   10

#define COMBOBOX_ANIMATION_DURATION     200  // In milliseconds

#define wxMSW_DESKTOP_USERPREFERENCESMASK_COMBOBOXANIM    (1<<2)


// ============================================================================
// implementation
// ============================================================================


BEGIN_EVENT_TABLE(wxComboCtrl, wxComboCtrlBase)
    EVT_PAINT(wxComboCtrl::OnPaintEvent)
    EVT_MOUSE_EVENTS(wxComboCtrl::OnMouseEvent)
#if wxUSE_COMBOCTRL_POPUP_ANIMATION
    EVT_TIMER(wxID_ANY, wxComboCtrl::OnTimerEvent)
#endif
END_EVENT_TABLE()


IMPLEMENT_DYNAMIC_CLASS(wxComboCtrl, wxComboCtrlBase)

void wxComboCtrl::Init()
{
}

bool wxComboCtrl::Create(wxWindow *parent,
                            wxWindowID id,
                            const wxString& value,
                            const wxPoint& pos,
                            const wxSize& size,
                            long style,
                            const wxValidator& validator,
                            const wxString& name)
{

    // Set border
    long border = style & wxBORDER_MASK;

#if wxUSE_UXTHEME
    wxUxThemeEngine* theme = wxUxThemeEngine::GetIfActive();
#endif

    if ( !border )
    {
        // For XP, have 1-width custom border, for older version use sunken
#if wxUSE_UXTHEME
        if ( theme )
        {
            border = wxBORDER_NONE;
            m_widthCustomBorder = 1;
        }
        else
#endif
            border = wxBORDER_SUNKEN;

        style = (style & ~(wxBORDER_MASK)) | border;
    }

    // create main window
    if ( !wxComboCtrlBase::Create(parent,
                           id,
                           value,
                           pos,
                           size,
                           style | wxFULL_REPAINT_ON_RESIZE,
                           wxDefaultValidator,
                           name) )
        return false;

    if ( style & wxCC_STD_BUTTON )
        m_iFlags |= wxCC_POPUP_ON_MOUSE_UP;

    // Create textctrl, if necessary
    CreateTextCtrl( wxNO_BORDER, validator );

    // Add keyboard input handlers for main control and textctrl
    InstallInputHandlers();

    // Prepare background for double-buffering
    SetBackgroundStyle( wxBG_STYLE_CUSTOM );

    // SetInitialSize should be called last
    SetInitialSize(size);

    return true;
}

wxComboCtrl::~wxComboCtrl()
{
}

void wxComboCtrl::OnThemeChange()
{
    // there doesn't seem to be any way to get the text colour using themes
    // API: TMT_TEXTCOLOR doesn't work neither for EDIT nor COMBOBOX
    SetForegroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT));

#if wxUSE_UXTHEME
    wxUxThemeEngine * const theme = wxUxThemeEngine::GetIfActive();
    if ( theme )
    {
        // NB: use EDIT, not COMBOBOX (the latter works in XP but not Vista)
        wxUxThemeHandle hTheme(this, L"EDIT");
        COLORREF col;
        HRESULT hr = theme->GetThemeColor
                            (
                                hTheme,
                                EP_EDITTEXT,
                                ETS_NORMAL,
                                TMT_FILLCOLOR,
                                &col
                            );
        if ( SUCCEEDED(hr) )
        {
            SetBackgroundColour(wxRGBToColour(col));

            // skip the call below
            return;
        }

        wxLogApiError(_T("GetThemeColor(EDIT, ETS_NORMAL, TMT_FILLCOLOR)"), hr);
    }
#endif

    SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW));
}

void wxComboCtrl::OnResize()
{
    //
    // Recalculates button and textctrl areas

    int textCtrlXAdjust;
    int textCtrlYAdjust;

#if wxUSE_UXTHEME
    if ( wxUxThemeEngine::GetIfActive() )
    {
        textCtrlXAdjust = TEXTCTRLXADJUST_XP;
        textCtrlYAdjust = TEXTCTRLYADJUST_XP;
    }
    else
#endif
    {
        textCtrlXAdjust = TEXTCTRLXADJUST_CLASSIC;
        textCtrlYAdjust = TEXTCTRLYADJUST_CLASSIC;
    }

    // Technically Classic Windows style combo has more narrow button,
    // but the native renderer doesn't paint it well like that.
    int btnWidth = 17;
    CalculateAreas(btnWidth);

    // Position textctrl using standard routine
    PositionTextCtrl(textCtrlXAdjust,textCtrlYAdjust);
}

// Draws non-XP GUI dotted line around the focus area
static void wxMSWDrawFocusRect( wxDC& dc, const wxRect& rect )
{
#if !defined(__WXWINCE__)
    /*
    RECT mswRect;
    mswRect.left = rect.x;
    mswRect.top = rect.y;
    mswRect.right = rect.x + rect.width;
    mswRect.bottom = rect.y + rect.height;
    HDC hdc = (HDC) dc.GetHDC();
    SetMapMode(hdc,MM_TEXT); // Just in case...
    DrawFocusRect(hdc,&mswRect);
    */
    // FIXME: Use DrawFocusRect code above (currently it draws solid line
    //   for caption focus but works ok for other stuff).
    //   Also, this code below may not work in future wx versions, since
    //   it employs wxCAP_BUTT hack to have line of width 1.
    dc.SetLogicalFunction(wxINVERT);

    wxPen pen(*wxBLACK,1,wxDOT);
    pen.SetCap(wxCAP_BUTT);
    dc.SetPen(pen);
    dc.SetBrush(*wxTRANSPARENT_BRUSH);

    dc.DrawRectangle(rect);

    dc.SetLogicalFunction(wxCOPY);
#else
    dc.SetLogicalFunction(wxINVERT);

    dc.SetPen(wxPen(*wxBLACK,1,wxDOT));
    dc.SetBrush(*wxTRANSPARENT_BRUSH);

    dc.DrawRectangle(rect);

    dc.SetLogicalFunction(wxCOPY);
#endif
}

// draw focus background on area in a way typical on platform
void
wxComboCtrl::PrepareBackground( wxDC& dc, const wxRect& rect, int flags ) const
{
#if wxUSE_UXTHEME
    wxUxThemeHandle hTheme(this, L"COMBOBOX");
#endif
    //COLORREF cref;

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

#if wxUSE_UXTHEME
        // Windows-style: for smaller size control (and for disabled background) use less spacing
        if ( hTheme )
        {
            // WinXP  Theme
            focusSpacingX = isEnabled ? 2 : 1;
            focusSpacingY = sz.y > (GetCharHeight()+2) && isEnabled ? 2 : 1;
        }
        else
#endif
        {
            // Classic Theme
            if ( isEnabled )
            {
                focusSpacingX = 1;
                focusSpacingY = 1;
            }
            else
            {
                focusSpacingX = 0;
                focusSpacingY = 0;
            }
        }
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

    //wxUxThemeEngine* theme = (wxUxThemeEngine*) NULL;
    //if ( hTheme )
    //    theme = wxUxThemeEngine::GetIfActive();

    wxColour bgCol;
    bool drawDottedEdge = false;

    if ( isEnabled )
    {
        // If popup is hidden and this control is focused,
        // then draw the focus-indicator (selbgcolor background etc.).
        if ( isFocused )
        {
        #if 0
            // TODO: Proper theme color getting (JMS: I don't know which parts/colors to use,
            //       those below don't work)
            if ( hTheme )
            {
                theme->GetThemeColor(hTheme,EP_EDITTEXT,ETS_SELECTED,TMT_TEXTCOLOR,&cref);
                dc.SetTextForeground( wxRGBToColour(cref) );
                theme->GetThemeColor(hTheme,EP_EDITTEXT,ETS_SELECTED,TMT_FILLCOLOR,&cref);
                bgCol = wxRGBToColour(cref);
            }
            else
        #endif
            {
                dc.SetTextForeground( wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHTTEXT) );
                bgCol = wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHT);
                if ( m_windowStyle & wxCB_READONLY )
                    drawDottedEdge = true;
            }
        }
        else
        {
            /*if ( hTheme )
            {
                theme->GetThemeColor(hTheme,EP_EDITTEXT,ETS_NORMAL,TMT_TEXTCOLOR,&cref);
                dc.SetTextForeground( wxRGBToColour(cref) );
                theme->GetThemeColor(hTheme,EP_EDITTEXT,ETS_NORMAL,TMT_FILLCOLOR,&cref);
                bgCol = wxRGBToColour(cref);
            }
            else
            {*/
                dc.SetTextForeground( wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT) );
                bgCol = GetBackgroundColour();
            //}
        }
    }
    else
    {
        /*if ( hTheme )
        {
            theme->GetThemeColor(hTheme,EP_EDITTEXT,ETS_DISABLED,TMT_TEXTCOLOR,&cref);
            dc.SetTextForeground( wxRGBToColour(cref) );
            theme->GetThemeColor(hTheme,EP_EDITTEXT,ETS_DISABLED,TMT_EDGEFILLCOLOR,&cref);
            bgCol = wxRGBToColour(cref);
        }
        else
        {*/
            dc.SetTextForeground( wxSystemSettings::GetColour(wxSYS_COLOUR_GRAYTEXT) );
            bgCol = wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE);
        //}
    }

    dc.SetBrush(bgCol);
    dc.SetPen(bgCol);
    dc.DrawRectangle(selRect);
    if ( drawDottedEdge )
        wxMSWDrawFocusRect(dc,selRect);

    // Don't clip exactly to the selection rectangle so we can draw
    // to the non-selected area in front of it.
    wxRect clipRect(rect.x,rect.y,
                    (selRect.x+selRect.width)-rect.x-1,rect.height);
    dc.SetClippingRegion(clipRect);
}

void wxComboCtrl::OnPaintEvent( wxPaintEvent& WXUNUSED(event) )
{
    // TODO: Convert drawing in this function to Windows API Code

    wxSize sz = GetClientSize();
    wxAutoBufferedPaintDC dc(this);

    const wxRect& rectb = m_btnArea;
    wxRect rect = m_tcArea;
    bool isEnabled = IsEnabled();
    wxColour bgCol = GetBackgroundColour();
    wxColour fgCol;

#if wxUSE_UXTHEME
    wxUxThemeEngine* theme = NULL;
    wxUxThemeHandle hTheme(this, L"COMBOBOX");
#endif

    int etsState;

    // area around both controls
    wxRect rect2(0,0,sz.x,sz.y);
    if ( m_iFlags & wxCC_IFLAG_BUTTON_OUTSIDE )
    {
        rect2 = m_tcArea;
        rect2.Inflate(1);
    }

#if wxUSE_UXTHEME
    // Use theme to draw border on XP
    if ( hTheme )
    {
        theme = wxUxThemeEngine::GetIfActive();
        COLORREF cref;

        // Select correct border colour
        if ( !isEnabled )
            etsState = ETS_DISABLED;
        else
            etsState = ETS_NORMAL;

        if ( m_widthCustomBorder )
        {
            theme->GetThemeColor(hTheme,EP_EDITTEXT,etsState,TMT_BORDERCOLOR,&cref);

            // Set border colour
            dc.SetPen( wxRGBToColour(cref) );

            dc.SetBrush( *wxTRANSPARENT_BRUSH );
            dc.DrawRectangle(rect2);
        }

        theme->GetThemeColor(hTheme,EP_EDITTEXT,etsState,TMT_TEXTCOLOR,&cref);
        fgCol = wxRGBToColour(cref);
    }
    else
#endif
    {
        // draw regular background
        fgCol = GetForegroundColour();
    }

    rect2.Deflate(m_widthCustomBorder);

    dc.SetBrush(bgCol);
    dc.SetPen(bgCol);

    // clear main background
    dc.DrawRectangle(rect);

    // Button background with theme?
    int drawButFlags = Draw_PaintBg;
#if wxUSE_UXTHEME
    if ( hTheme && m_blankButtonBg )
    {
        RECT r;
        wxCopyRectToRECT(rectb, r);

        // Draw parent background if needed (since button looks like its out of
        // the combo, this is preferred).
        theme->DrawThemeParentBackground(GetHwndOf(this),
                                         GetHdcOf(dc),
                                         &r);

        drawButFlags = 0;
    }
#endif

    // Standard button rendering
    DrawButton(dc,rectb,drawButFlags);

    // paint required portion on the control
    if ( (!m_text || m_widthCustomPaint) )
    {
        wxASSERT( m_widthCustomPaint >= 0 );

        // this is intentionally here to allow drawed rectangle's
        // right edge to be hidden
        if ( m_text )
            rect.width = m_widthCustomPaint;

        dc.SetFont( GetFont() );

        dc.SetClippingRegion(rect);
        if ( m_popupInterface )
            m_popupInterface->PaintComboControl(dc,rect);
        else
            wxComboPopup::DefaultPaintComboControl(this,dc,rect);
    }
}

void wxComboCtrl::OnMouseEvent( wxMouseEvent& event )
{
    int mx = event.m_x;
    bool isOnButtonArea = m_btnArea.Contains(mx,event.m_y);
    int handlerFlags = isOnButtonArea ? wxCC_MF_ON_BUTTON : 0;

    if ( PreprocessMouseEvent(event,isOnButtonArea) )
        return;

    if ( (m_windowStyle & (wxCC_SPECIAL_DCLICK|wxCB_READONLY)) == wxCB_READONLY )
    {
        // if no textctrl and no special double-click, then the entire control acts
        // as a button
        handlerFlags |= wxCC_MF_ON_BUTTON;
        if ( HandleButtonMouseEvent(event,handlerFlags) )
            return;
    }
    else
    {
        if ( isOnButtonArea || HasCapture() ||
             (m_widthCustomPaint && mx < (m_tcArea.x+m_widthCustomPaint)) )
        {
            handlerFlags |= wxCC_MF_ON_CLICK_AREA;

            if ( HandleButtonMouseEvent(event,handlerFlags) )
                return;
        }
        else if ( m_btnState )
        {
            // otherwise need to clear the hover status
            m_btnState = 0;
            RefreshRect(m_btnArea);
        }
    }

    //
    // This will handle left_down and left_dclick events outside button in a Windows-like manner.
    // See header file for further information on this method.
    HandleNormalMouseEvent(event);

}

#if wxUSE_COMBOCTRL_POPUP_ANIMATION
static wxUint32 GetUserPreferencesMask()
{
    static wxUint32 userPreferencesMask = 0;
    static bool valueSet = false;

    if ( valueSet )
        return userPreferencesMask;

    wxRegKey* pKey = NULL;
    wxRegKey key1(wxRegKey::HKCU, wxT("Software\\Policies\\Microsoft\\Control Panel"));
    wxRegKey key2(wxRegKey::HKCU, wxT("Software\\Policies\\Microsoft\\Windows\\Control Panel"));
    wxRegKey key3(wxRegKey::HKCU, wxT("Control Panel\\Desktop"));

    if ( key1.Exists() )
        pKey = &key1;
    else if ( key2.Exists() )
        pKey = &key2;
    else if ( key3.Exists() )
        pKey = &key3;

    if ( pKey && pKey->Open(wxRegKey::Read) )
    {
        wxMemoryBuffer buf;
        if ( pKey->HasValue(wxT("UserPreferencesMask")) &&
             pKey->QueryValue(wxT("UserPreferencesMask"), buf) )
        {
            if ( buf.GetDataLen() >= 4 )
            {
                wxUint32* p = (wxUint32*) buf.GetData();
                userPreferencesMask = *p;
            }
        }
    }

    valueSet = true;

    return userPreferencesMask;
}
#endif

#if wxUSE_COMBOCTRL_POPUP_ANIMATION
void wxComboCtrl::OnTimerEvent( wxTimerEvent& WXUNUSED(event) )
{
    bool stopTimer = false;

    wxWindow* popup = GetPopupControl()->GetControl();

    // Popup was hidden before it was fully shown?
    if ( IsPopupWindowState(Hidden) )
    {
        stopTimer = true;
    }
    else
    {
        wxLongLong t = ::wxGetLocalTimeMillis();
        const wxRect& rect = m_animRect;
        wxWindow* win = GetPopupWindow();

        int pos = (int) (t-m_animStart).GetLo();
        if ( pos < COMBOBOX_ANIMATION_DURATION )
        {
            int height = rect.height;
            //int h0 = rect.height;
            int h = (((pos*256)/COMBOBOX_ANIMATION_DURATION)*height)/256;
            int y = (height - h);
            if ( y < 0 )
                y = 0;

            if ( m_animFlags & ShowAbove )
            {
                win->SetSize( rect.x, rect.y + height - h, rect.width, h );
            }
            else
            {
                popup->Move( 0, -y );
                win->SetSize( rect.x, rect.y, rect.width, h );
            }
        }
        else
        {
            stopTimer = true;
        }
    }

    if ( stopTimer )
    {
        popup->Move( 0, 0 );
        m_animTimer.Stop();
        DoShowPopup( m_animRect, m_animFlags );
    }
}
#endif

#if wxUSE_COMBOCTRL_POPUP_ANIMATION
bool wxComboCtrl::AnimateShow( const wxRect& rect, int flags )
{
    if ( GetUserPreferencesMask() & wxMSW_DESKTOP_USERPREFERENCESMASK_COMBOBOXANIM )
    {
        m_animStart = ::wxGetLocalTimeMillis();
        m_animRect = rect;
        m_animFlags = flags;

        wxWindow* win = GetPopupWindow();
        win->SetSize( rect.x, rect.y, rect.width, 0 );
        win->Show();

        m_animTimer.SetOwner( this, wxID_ANY );
        m_animTimer.Start( COMBOBOX_ANIMATION_RESOLUTION, wxTIMER_CONTINUOUS );

        OnTimerEvent(*((wxTimerEvent*)NULL));  // Event is never used, so we can give NULL

        return false;
    }

    return true;
}
#endif

wxCoord wxComboCtrl::GetNativeTextIndent() const
{
#if wxUSE_UXTHEME
    if ( wxUxThemeEngine::GetIfActive() )
        return NATIVE_TEXT_INDENT_XP;
#endif
    return NATIVE_TEXT_INDENT_CLASSIC;
}

bool wxComboCtrl::IsKeyPopupToggle(const wxKeyEvent& event) const
{
    const bool isPopupShown = IsPopupShown();

    switch ( event.GetKeyCode() )
    {
        case WXK_F4:
            // F4 toggles the popup in the native comboboxes, so emulate them
            if ( !event.AltDown() )
                return true;
            break;

        case WXK_ESCAPE:
            if ( isPopupShown )
                return true;
            break;

        case WXK_DOWN:
        case WXK_UP:
            // On XP or with writable combo in Classic, arrows don't open the
            // popup but Alt-arrow does
            if ( event.AltDown() ||
                    ( !isPopupShown &&
                      HasFlag(wxCB_READONLY)
#if wxUSE_UXTHEME
					  &&
                      !wxUxThemeEngine::GetIfActive()
#endif
                    ) )
            {
                return true;
            }
            break;
    }

    return false;
}

#endif // wxUSE_COMBOCTRL
