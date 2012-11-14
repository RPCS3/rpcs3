/////////////////////////////////////////////////////////////////////////////
// Name:        src/generic/combog.cpp
// Purpose:     Generic wxComboCtrl
// Author:      Jaakko Salli
// Modified by:
// Created:     Apr-30-2006
// RCS-ID:      $Id: combog.cpp 67178 2011-03-13 09:32:19Z JMS $
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

#include "wx/combo.h"

#ifndef WX_PRECOMP
    #include "wx/log.h"
    #include "wx/combobox.h"
    #include "wx/dcclient.h"
    #include "wx/settings.h"
#endif

#include "wx/dcbuffer.h"

// ----------------------------------------------------------------------------
// Some constant adjustments to make the generic more bearable

#if defined(__WXUNIVERSAL__)

#define TEXTCTRLXADJUST                 0 // position adjustment for wxTextCtrl, with zero indent
#define TEXTCTRLYADJUST                 0
#define TEXTXADJUST                     0 // how much is read-only text's x adjusted
#define DEFAULT_DROPBUTTON_WIDTH        19

#elif defined(__WXMSW__)

#define TEXTCTRLXADJUST                 2 // position adjustment for wxTextCtrl, with zero indent
#define TEXTCTRLYADJUST                 3
#define TEXTXADJUST                     0 // how much is read-only text's x adjusted
#define DEFAULT_DROPBUTTON_WIDTH        17

#elif defined(__WXGTK__)

#define TEXTCTRLXADJUST                 -1 // position adjustment for wxTextCtrl, with zero indent
#define TEXTCTRLYADJUST                 0
#define TEXTXADJUST                     1 // how much is read-only text's x adjusted
#define DEFAULT_DROPBUTTON_WIDTH        23

#elif defined(__WXMAC__)

#define TEXTCTRLXADJUST                 0 // position adjustment for wxTextCtrl, with zero indent
#define TEXTCTRLYADJUST                 0
#define TEXTXADJUST                     0 // how much is read-only text's x adjusted
#define DEFAULT_DROPBUTTON_WIDTH        22

#else

#define TEXTCTRLXADJUST                 0 // position adjustment for wxTextCtrl, with zero indent
#define TEXTCTRLYADJUST                 0
#define TEXTXADJUST                     0 // how much is read-only text's x adjusted
#define DEFAULT_DROPBUTTON_WIDTH        19

#endif


// ============================================================================
// implementation
// ============================================================================

// Only implement if no native or it wasn't fully featured
#ifndef wxCOMBOCONTROL_FULLY_FEATURED


// ----------------------------------------------------------------------------
// wxGenericComboCtrl
// ----------------------------------------------------------------------------

BEGIN_EVENT_TABLE(wxGenericComboCtrl, wxComboCtrlBase)
    EVT_PAINT(wxGenericComboCtrl::OnPaintEvent)
    EVT_MOUSE_EVENTS(wxGenericComboCtrl::OnMouseEvent)
END_EVENT_TABLE()


IMPLEMENT_DYNAMIC_CLASS(wxGenericComboCtrl, wxComboCtrlBase)

void wxGenericComboCtrl::Init()
{
}

bool wxGenericComboCtrl::Create(wxWindow *parent,
                                wxWindowID id,
                                const wxString& value,
                                const wxPoint& pos,
                                const wxSize& size,
                                long style,
                                const wxValidator& validator,
                                const wxString& name)
{
    //
    // Note that technically we only support 'default' border and wxNO_BORDER.
    long border = style & wxBORDER_MASK;
    int tcBorder = wxNO_BORDER;

#if defined(__WXUNIVERSAL__)
    if ( !border )
        border = wxBORDER_SIMPLE;
#elif defined(__WXMSW__)
    if ( !border )
        // For XP, have 1-width custom border, for older version use sunken
        /*if ( wxUxThemeEngine::GetIfActive() )
        {
            border = wxBORDER_NONE;
            m_widthCustomBorder = 1;
        }
        else*/
            border = wxBORDER_SUNKEN;
#else

    //
    // Generic version is optimized for wxGTK
    //

    #define UNRELIABLE_TEXTCTRL_BORDER

    if ( !border )
    {
        if ( style & wxCB_READONLY )
        {
            m_widthCustomBorder = 1;
        }
        else
        {
            m_widthCustomBorder = 0;
            tcBorder = 0;
        }
    }
    else
    {
        // Have textctrl instead use the border given.
        tcBorder = border;
    }

    // Because we are going to have button outside the border,
    // let's use wxBORDER_NONE for the whole control.
    border = wxBORDER_NONE;

    Customize( wxCC_BUTTON_OUTSIDE_BORDER |
               wxCC_NO_TEXT_AUTO_SELECT );

#endif

    style = (style & ~(wxBORDER_MASK)) | border;
    if ( style & wxCC_STD_BUTTON )
        m_iFlags |= wxCC_POPUP_ON_MOUSE_UP;

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

    // Create textctrl, if necessary
    CreateTextCtrl( tcBorder, validator );

    // Add keyboard input handlers for main control and textctrl
    InstallInputHandlers();

    // Set background
    SetBackgroundStyle( wxBG_STYLE_CUSTOM ); // for double-buffering

    // SetInitialSize should be called last
    SetInitialSize(size);

    return true;
}

wxGenericComboCtrl::~wxGenericComboCtrl()
{
}

void wxGenericComboCtrl::OnResize()
{

    // Recalculates button and textctrl areas
    CalculateAreas(DEFAULT_DROPBUTTON_WIDTH);

#if 0
    // Move separate button control, if any, to correct position
    if ( m_btn )
    {
        wxSize sz = GetClientSize();
        m_btn->SetSize( m_btnArea.x + m_btnSpacingX,
                        (sz.y-m_btnSize.y)/2,
                        m_btnSize.x,
                        m_btnSize.y );
    }
#endif

    // Move textctrl, if any, accordingly
    PositionTextCtrl( TEXTCTRLXADJUST, TEXTCTRLYADJUST );
}

void wxGenericComboCtrl::OnPaintEvent( wxPaintEvent& WXUNUSED(event) )
{
    wxSize sz = GetClientSize();
    wxAutoBufferedPaintDC dc(this);

    const wxRect& rectb = m_btnArea;
    wxRect rect = m_tcArea;

    // artificial simple border
    if ( m_widthCustomBorder )
    {
        int customBorder = m_widthCustomBorder;

        // Set border colour
        wxPen pen1( wxSystemSettings::GetColour(wxSYS_COLOUR_GRAYTEXT),
                    customBorder,
                    wxSOLID );
        dc.SetPen( pen1 );

        // area around both controls
        wxRect rect2(0,0,sz.x,sz.y);
        if ( m_iFlags & wxCC_IFLAG_BUTTON_OUTSIDE )
        {
            rect2 = m_tcArea;
            if ( customBorder == 1 )
            {
                rect2.Inflate(1);
            }
            else
            {
            #ifdef __WXGTK__
                rect2.x -= 1;
                rect2.y -= 1;
            #else
                rect2.x -= customBorder;
                rect2.y -= customBorder;
            #endif
                rect2.width += 1 + customBorder;
                rect2.height += 1 + customBorder;
            }
        }

        dc.SetBrush( *wxTRANSPARENT_BRUSH );
        dc.DrawRectangle(rect2);
    }

#if defined(__WXMAC__) || defined(__WXGTK__)  // see note in OnThemeChange
    wxColour winCol = wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW);
#else
    wxColour winCol = GetBackgroundColour();
#endif
    dc.SetBrush(winCol);
    dc.SetPen(winCol);

    //wxLogDebug(wxT("hei: %i tcy: %i tchei: %i"),GetClientSize().y,m_tcArea.y,m_tcArea.height);
    //wxLogDebug(wxT("btnx: %i tcx: %i tcwid: %i"),m_btnArea.x,m_tcArea.x,m_tcArea.width);

    // clear main background
    dc.DrawRectangle(rect);
    
    if ( !m_btn )
    {
    #ifdef __WXGTK__
        // Under GTK+ this avoids drawing the button background with wrong
        // colour
        DrawButton(dc,rectb,0);
    #else
        // Standard button rendering
        DrawButton(dc,rectb);
    #endif
    }

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

void wxGenericComboCtrl::OnMouseEvent( wxMouseEvent& event )
{
    int mx = event.m_x;
    bool isOnButtonArea = m_btnArea.Contains(mx,event.m_y);
    int handlerFlags = isOnButtonArea ? wxCC_MF_ON_BUTTON : 0;

    if ( PreprocessMouseEvent(event,handlerFlags) )
        return;

    const bool ctrlIsButton = wxPlatformIs(wxOS_WINDOWS);

    if ( ctrlIsButton &&
         (m_windowStyle & (wxCC_SPECIAL_DCLICK|wxCB_READONLY)) == wxCB_READONLY )
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
    // This will handle left_down and left_dclick events outside button in a Windows/GTK-like manner.
    // See header file for further information on this method.
    HandleNormalMouseEvent(event);

}

void wxGenericComboCtrl::SetCustomPaintWidth( int width )
{
#ifdef UNRELIABLE_TEXTCTRL_BORDER
    //
    // If starting/stopping to show an image in front
    // of a writable text-field, then re-create textctrl
    // with different kind of border (because we can't
    // assume that textctrl fully supports wxNO_BORDER).
    //
    wxTextCtrl* tc = GetTextCtrl();

    if ( tc && (m_iFlags & wxCC_BUTTON_OUTSIDE_BORDER) )
    {
        int borderType = tc->GetWindowStyle() & wxBORDER_MASK;
        int tcCreateStyle = -1;

        if ( width > 0 )
        {
            // Re-create textctrl with no border
            if ( borderType != wxNO_BORDER )
            {
                m_widthCustomBorder = 1;
                tcCreateStyle = wxNO_BORDER;
            }
        }
        else if ( width == 0 )
        {
            // Re-create textctrl with normal border
            if ( borderType == wxNO_BORDER )
            {
                m_widthCustomBorder = 0;
                tcCreateStyle = 0;
            }
        }

        // Common textctrl re-creation code
        if ( tcCreateStyle != -1 )
        {
            tc->RemoveEventHandler(m_textEvtHandler);
            delete m_textEvtHandler;

#if wxUSE_VALIDATORS
            wxValidator* pValidator = tc->GetValidator();
            if ( pValidator )
            {
                pValidator = (wxValidator*) pValidator->Clone();
                CreateTextCtrl( tcCreateStyle, *pValidator );
                delete pValidator;
            }
            else
#endif
            {
                CreateTextCtrl( tcCreateStyle, wxDefaultValidator );
            }

            InstallInputHandlers();
        }
    }
#endif // UNRELIABLE_TEXTCTRL_BORDER

    wxComboCtrlBase::SetCustomPaintWidth( width );
}

bool wxGenericComboCtrl::IsKeyPopupToggle(const wxKeyEvent& event) const
{
    int keycode = event.GetKeyCode();
    bool isPopupShown = IsPopupShown();

    // This code is AFAIK appropriate for wxGTK.

    if ( isPopupShown )
    {
        if ( keycode == WXK_ESCAPE ||
             ( keycode == WXK_UP && event.AltDown() ) )
            return true;
    }
    else
    {
        if ( keycode == WXK_DOWN && event.AltDown() )
            return true;
    }

    return false;
}

#ifdef __WXUNIVERSAL__

bool wxGenericComboCtrl::PerformAction(const wxControlAction& action,
                                       long numArg,
                                       const wxString& strArg)
{
    bool processed = false;
    if ( action == wxACTION_COMBOBOX_POPUP )
    {
        if ( !IsPopupShown() )
        {
            ShowPopup();

            processed = true;
        }
    }
    else if ( action == wxACTION_COMBOBOX_DISMISS )
    {
        if ( IsPopupShown() )
        {
            HidePopup();

            processed = true;
        }
    }

    if ( !processed )
    {
        // pass along
        return wxControl::PerformAction(action, numArg, strArg);
    }

    return true;
}

#endif // __WXUNIVERSAL__

// If native wxComboCtrl was not defined, then prepare a simple
// front-end so that wxRTTI works as expected.
#ifndef _WX_COMBOCONTROL_H_
IMPLEMENT_DYNAMIC_CLASS(wxComboCtrl, wxGenericComboCtrl)
#endif

#endif // !wxCOMBOCONTROL_FULLY_FEATURED

#endif // wxUSE_COMBOCTRL
