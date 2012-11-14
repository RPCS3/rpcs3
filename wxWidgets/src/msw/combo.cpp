/////////////////////////////////////////////////////////////////////////////
// Name:        src/msw/combo.cpp
// Purpose:     wxMSW wxComboCtrl
// Author:      Jaakko Salli
// Modified by:
// Created:     Apr-30-2006
// RCS-ID:      $Id: combo.cpp 64477 2010-06-03 15:25:41Z JMS $
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

//
// Define wxUSE_COMBOCTRL_VISTA_RENDERING as 1 to have read-only wxComboCtrl
// and wxOwnerDrawnComboBox appear much nicer on Win Vista/7. Disabled by
// default as this may cause some subtle rendering bugs with custom
// wxComboCtrls.
//
#ifndef wxUSE_COMBOCTRL_VISTA_RENDERING
    #define wxUSE_COMBOCTRL_VISTA_RENDERING 0
#endif

// Change to #if 1 to include tmschema.h for easier testing of theme
// parameters.
#if 0
    #include <tmschema.h>
    #include <VSStyle.h>
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
    #define TMT_BGTYPE          4001

    #define BT_IMAGEFILE        0
    #define BT_BORDERFILL       1

    #define CP_DROPDOWNBUTTON           1
    #define CP_BACKGROUND               2 // This and above are Vista and later only
    #define CP_TRANSPARENTBACKGROUND    3
    #define CP_BORDER                   4
    #define CP_READONLY                 5
    #define CP_DROPDOWNBUTTONRIGHT      6
    #define CP_DROPDOWNBUTTONLEFT       7
    #define CP_CUEBANNER                8

    #define CBXS_NORMAL                 1
    #define CBXS_HOT                    2
    #define CBXS_PRESSED                3
    #define CBXS_DISABLED               4

    #define CBXSR_NORMAL                1
    #define CBXSR_HOT                   2
    #define CBXSR_PRESSED               3
    #define CBXSR_DISABLED              4

    #define CBXSL_NORMAL                1
    #define CBXSL_HOT                   2
    #define CBXSL_PRESSED               3
    #define CBXSL_DISABLED              4

    #define CBTBS_NORMAL                1
    #define CBTBS_HOT                   2
    #define CBTBS_DISABLED              3
    #define CBTBS_FOCUSED               4

    #define CBB_NORMAL                  1
    #define CBB_HOT                     2
    #define CBB_FOCUSED                 3
    #define CBB_DISABLED                4

    #define CBRO_NORMAL                 1
    #define CBRO_HOT                    2
    #define CBRO_PRESSED                3
    #define CBRO_DISABLED               4

    #define CBCB_NORMAL                 1
    #define CBCB_HOT                    2
    #define CBCB_PRESSED                3
    #define CBCB_DISABLED               4

#endif

// In wx2.9, this is defined in msw\private.h
#define wxWinVersion_Vista 0x0600

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
    if ( !m_hasFgCol )
    {
        wxColour fgCol = wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT);
        SetForegroundColour(fgCol);
        m_hasFgCol = false;
    }

    wxColour bgCol = wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW);

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
            bgCol = wxRGBToColour(col);
        }
        else
        {
            wxLogApiError(_T("GetThemeColor(EDIT, ETS_NORMAL, TMT_FILLCOLOR)"),
                          hr);
        }
    }
#endif

    if ( !m_hasBgCol )
    {
        SetBackgroundColour(bgCol);
        m_hasBgCol = false;
    }
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

    wxSize sz = GetClientSize();
    bool isEnabled;
    bool doDrawFocusRect; // also selected

    // For smaller size control (and for disabled background) use less spacing
    int focusSpacingX;
    int focusSpacingY;

    if ( !(flags & wxCONTROL_ISSUBMENU) )
    {
        // Drawing control
        isEnabled = IsEnabled();
        doDrawFocusRect = ShouldDrawFocus();

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
        doDrawFocusRect = flags & wxCONTROL_SELECTED ? true : false;

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

    //wxUxThemeEngine* theme = NULL;
    //if ( hTheme )
    //    theme = wxUxThemeEngine::GetIfActive();

    wxColour fgCol;
    wxColour bgCol;
    bool doDrawDottedEdge = false;
    bool doDrawSelRect = true;

    // TODO: doDrawDottedEdge = true when focus has arrived to control via tab.
    //       (and other cases which are not that apparent).

    if ( isEnabled )
    {
        // If popup is hidden and this control is focused,
        // then draw the focus-indicator (selbgcolor background etc.).
        if ( doDrawFocusRect )
        {
            // NB: We can't really use XP visual styles to get TMT_TEXTCOLOR since
            //     it is not properly defined for combo boxes. Instead, they expect
            //     you to use DrawThemeText.
            //
            //    Here is, however, sample code how to get theme colours:
            //
            //    COLORREF cref;
            //    theme->GetThemeColor(hTheme,EP_EDITTEXT,ETS_NORMAL,TMT_TEXTCOLOR,&cref);
            //    dc.SetTextForeground( wxRGBToColour(cref) );
            if ( (m_iFlags & wxCC_FULL_BUTTON) && !(flags & wxCONTROL_ISSUBMENU) )
            {
                // Vista style read-only combo
                fgCol = GetForegroundColour();
                bgCol = GetBackgroundColour();
                doDrawSelRect = false;
                doDrawDottedEdge = true;
            }
            else
            {
                fgCol = wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHTTEXT);
                bgCol = wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHT);
            }
        }
        else
        {
            fgCol = GetForegroundColour();
            bgCol = GetBackgroundColour();
            doDrawSelRect = false;
        }
    }
    else
    {
        fgCol = wxSystemSettings::GetColour(wxSYS_COLOUR_GRAYTEXT);
        bgCol = wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE);
    }

    dc.SetTextForeground(fgCol);
    dc.SetBrush(bgCol);
    if ( doDrawSelRect )
    {
        dc.SetPen(bgCol);
        dc.DrawRectangle(selRect);
    }

    if ( doDrawDottedEdge )
        wxMSWDrawFocusRect(dc, selRect);

    // Don't clip exactly to the selection rectangle so we can draw
    // to the non-selected area in front of it.
    wxRect clipRect(rect.x,rect.y,
                    (selRect.x+selRect.width)-rect.x-1,rect.height);
    dc.SetClippingRegion(clipRect);
}

void wxComboCtrl::OnPaintEvent( wxPaintEvent& WXUNUSED(event) )
{
    // TODO: Convert drawing in this function to Windows API Code

    // TODO: Convert drawing in this function to Windows API Code

    wxSize sz = GetClientSize();
    wxAutoBufferedPaintDC dc(this);

    const wxRect& rectButton = m_btnArea;
    wxRect rectTextField = m_tcArea;
    wxColour bgCol = GetBackgroundColour();

#if wxUSE_UXTHEME
    const bool isEnabled = IsEnabled();

    HDC hDc = GetHdcOf(dc);
    HWND hWnd = GetHwndOf(this);

    wxUxThemeEngine* theme = NULL;
    wxUxThemeHandle hTheme(this, L"COMBOBOX");

    if ( hTheme )
        theme = wxUxThemeEngine::GetIfActive();
#endif // wxUSE_UXTHEME

    wxRect borderRect(0,0,sz.x,sz.y);

    if ( m_iFlags & wxCC_IFLAG_BUTTON_OUTSIDE )
    {
        borderRect = m_tcArea;
        borderRect.Inflate(1);
    }

    int drawButFlags = 0;

#if wxUSE_UXTHEME
    if ( hTheme )
    {
  #if wxUSE_COMBOCTRL_VISTA_RENDERING
        const bool useVistaComboBox = ::wxGetWinVersion() >= wxWinVersion_Vista;
  #else
        const bool useVistaComboBox = false;
  #endif

        RECT rFull;
        wxCopyRectToRECT(borderRect, rFull);

        RECT rButton;
        wxCopyRectToRECT(rectButton, rButton);

        RECT rBorder;
        wxCopyRectToRECT(borderRect, rBorder);

        bool isNonStdButton = (m_iFlags & wxCC_IFLAG_BUTTON_OUTSIDE) ||
                              (m_iFlags & wxCC_IFLAG_HAS_NONSTANDARD_BUTTON);

        //
        // Get some states for themed drawing
        int butState;

        if ( !isEnabled )
        {
            butState = CBXS_DISABLED;
        }
        // Vista will display the drop-button as depressed always
        // when the popup window is visilbe
        else if ( (m_btnState & wxCONTROL_PRESSED) ||
                  (useVistaComboBox && !IsPopupWindowState(Hidden)) )
        {
            butState = CBXS_PRESSED;
        }
        else if ( m_btnState & wxCONTROL_CURRENT )
        {
            butState = CBXS_HOT;
        }
        else
        {
            butState = CBXS_NORMAL;
        }

        int comboBoxPart = 0;  // For XP, use the 'default' part
        RECT* rUseForBg = &rBorder;

        int bgState = butState;

  #if wxUSE_COMBOCTRL_VISTA_RENDERING
        bool drawFullButton = false;

        if ( useVistaComboBox )
        {
            const bool isFocused =
                (FindFocus() == GetMainWindowOfCompositeControl()) ?
                    true : false;

            // FIXME: Either SetBackgroundColour or GetBackgroundColour
            //        doesn't work under Vista, so here's a temporary
            //        workaround.
            bgCol = wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW);

            // Draw the entire control as a single button?
            if ( !isNonStdButton )
            {
                if ( HasFlag(wxCB_READONLY) )
                    drawFullButton = true;
            }

            if ( drawFullButton )
            {
                comboBoxPart = CP_READONLY;
                rUseForBg = &rFull;

                // It should be safe enough to update this flag here.
                m_iFlags |= wxCC_FULL_BUTTON;
            }
            else
            {
                comboBoxPart = CP_BORDER;
                m_iFlags &= ~wxCC_FULL_BUTTON;

                if ( isFocused )
                    bgState = CBB_FOCUSED;
                else
                    bgState = CBB_NORMAL;
            }
        }
  #endif

        //
        // Draw parent's background, if necessary
        RECT* rUseForTb = NULL;

        if ( theme->IsThemeBackgroundPartiallyTransparent( hTheme, comboBoxPart, bgState ) )
            rUseForTb = &rFull;
        else if ( m_iFlags & wxCC_IFLAG_BUTTON_OUTSIDE )
            rUseForTb = &rButton;

        if ( rUseForTb )
            theme->DrawThemeParentBackground( hWnd, hDc, rUseForTb );

        //
        // Draw the control background (including the border)
        if ( m_widthCustomBorder > 0 )
        {
            theme->DrawThemeBackground( hTheme, hDc, comboBoxPart, bgState, rUseForBg, NULL );
        }
        else
        {
            // No border. We can't use theme, since it cannot be relied on
            // to deliver borderless drawing, even with DrawThemeBackgroundEx.
            dc.SetBrush(bgCol);
            dc.SetPen(bgCol);
            dc.DrawRectangle(borderRect);
        }

        //
        // Draw the drop-button
        if ( !isNonStdButton )
        {
            drawButFlags = Draw_BitmapOnly;

            int butPart = CP_DROPDOWNBUTTON;

  #if wxUSE_COMBOCTRL_VISTA_RENDERING
            if ( useVistaComboBox )
            {
                if ( drawFullButton )
                {
                    // We need to alter the button style slightly before
                    // drawing the actual button (but it was good above
                    // when background etc was done).
                    if ( butState == CBXS_HOT || butState == CBXS_PRESSED )
                        butState = CBXS_NORMAL;
                }

                if ( m_btnSide == wxRIGHT )
                    butPart = CP_DROPDOWNBUTTONRIGHT;
                else
                    butPart = CP_DROPDOWNBUTTONLEFT;

            }
  #endif
            theme->DrawThemeBackground( hTheme, hDc, butPart, butState, &rButton, NULL );
        }
        else if ( useVistaComboBox &&
                  (m_iFlags & wxCC_IFLAG_BUTTON_OUTSIDE) )
        {
            // We'll do this, because DrawThemeParentBackground
            // doesn't seem to be reliable on Vista.
            drawButFlags |= Draw_PaintBg;
        }
    }
    else
#endif
    {
        // Windows 2000 and earlier
        drawButFlags = Draw_PaintBg;

        dc.SetBrush(bgCol);
        dc.SetPen(bgCol);
        dc.DrawRectangle(borderRect);
    }

    // Button rendering (may only do the bitmap on button, depending on the flags)
    DrawButton( dc, rectButton, drawButFlags );

    // Paint required portion of the custom image on the control
    if ( (!m_text || m_widthCustomPaint) )
    {
        wxASSERT( m_widthCustomPaint >= 0 );

        // this is intentionally here to allow drawed rectangle's
        // right edge to be hidden
        if ( m_text )
            rectTextField.width = m_widthCustomPaint;

        dc.SetFont( GetFont() );

        dc.SetClippingRegion(rectTextField);
        if ( m_popupInterface )
            m_popupInterface->PaintComboControl(dc,rectTextField);
        else
            wxComboPopup::DefaultPaintComboControl(this,dc,rectTextField);
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

    wxWindow* win = GetPopupWindow();
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
                // Note that apparently Move() should be called after
                // SetSize() to reduce (or even eliminate) animation garbage
                win->SetSize( rect.x, rect.y, rect.width, h );
                popup->Move( 0, -y );
            }
        }
        else
        {
            stopTimer = true;
        }
    }

    if ( stopTimer )
    {
        m_animTimer.Stop();
        DoShowPopup( m_animRect, m_animFlags );
        popup->Move( 0, 0 );

        // Do a one final refresh to clean up the rare cases of animation
        // garbage
        win->Refresh();
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
        case WXK_NUMPAD_DOWN:
        case WXK_NUMPAD_UP:
            // Alt plus arrow key toggles the popup in the native combo box
            if ( event.AltDown() )
                return true;
            break;
    }

    return false;
}

#endif // wxUSE_COMBOCTRL
