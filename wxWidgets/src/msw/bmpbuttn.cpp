/////////////////////////////////////////////////////////////////////////////
// Name:        src/msw/bmpbuttn.cpp
// Purpose:     wxBitmapButton
// Author:      Julian Smart
// Modified by:
// Created:     04/01/98
// RCS-ID:      $Id: bmpbuttn.cpp 42816 2006-10-31 08:50:17Z RD $
// Copyright:   (c) Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#if wxUSE_BMPBUTTON

#include "wx/bmpbuttn.h"

#ifndef WX_PRECOMP
    #include "wx/log.h"
    #include "wx/dcmemory.h"
    #include "wx/image.h"
#endif

#include "wx/msw/private.h"

#include "wx/msw/uxtheme.h"

#if wxUSE_UXTHEME
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

#ifndef ODS_NOFOCUSRECT
    #define ODS_NOFOCUSRECT     0x0200
#endif

// ----------------------------------------------------------------------------
// macros
// ----------------------------------------------------------------------------

#if wxUSE_EXTENDED_RTTI

WX_DEFINE_FLAGS( wxBitmapButtonStyle )

wxBEGIN_FLAGS( wxBitmapButtonStyle )
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

    wxFLAGS_MEMBER(wxBU_AUTODRAW)
    wxFLAGS_MEMBER(wxBU_LEFT)
    wxFLAGS_MEMBER(wxBU_RIGHT)
    wxFLAGS_MEMBER(wxBU_TOP)
    wxFLAGS_MEMBER(wxBU_BOTTOM)
wxEND_FLAGS( wxBitmapButtonStyle )

IMPLEMENT_DYNAMIC_CLASS_XTI(wxBitmapButton, wxButton,"wx/bmpbuttn.h")

wxBEGIN_PROPERTIES_TABLE(wxBitmapButton)
    wxPROPERTY_FLAGS( WindowStyle , wxBitmapButtonStyle , long , SetWindowStyleFlag , GetWindowStyleFlag , EMPTY_MACROVALUE, 0 /*flags*/ , wxT("Helpstring") , wxT("group")) // style
wxEND_PROPERTIES_TABLE()

wxBEGIN_HANDLERS_TABLE(wxBitmapButton)
wxEND_HANDLERS_TABLE()

wxCONSTRUCTOR_5( wxBitmapButton , wxWindow* , Parent , wxWindowID , Id , wxBitmap , Bitmap , wxPoint , Position , wxSize , Size )

#else
IMPLEMENT_DYNAMIC_CLASS(wxBitmapButton, wxButton)
#endif

BEGIN_EVENT_TABLE(wxBitmapButton, wxBitmapButtonBase)
    EVT_SYS_COLOUR_CHANGED(wxBitmapButton::OnSysColourChanged)
    EVT_ENTER_WINDOW(wxBitmapButton::OnMouseEnterOrLeave)
    EVT_LEAVE_WINDOW(wxBitmapButton::OnMouseEnterOrLeave)
END_EVENT_TABLE()

/*
TODO PROPERTIES :

long "style" , wxBU_AUTODRAW
bool "default" , 0
bitmap "selected" ,
bitmap "focus" ,
bitmap "disabled" ,
*/

bool wxBitmapButton::Create(wxWindow *parent, wxWindowID id,
    const wxBitmap& bitmap,
    const wxPoint& pos,
    const wxSize& size, long style,
    const wxValidator& wxVALIDATOR_PARAM(validator),
    const wxString& name)
{
    m_bmpNormal = bitmap;
    SetName(name);

#if wxUSE_VALIDATORS
    SetValidator(validator);
#endif // wxUSE_VALIDATORS

    parent->AddChild(this);

    m_windowStyle = style;

    if ( style & wxBU_AUTODRAW )
    {
        m_marginX =
        m_marginY = 4;
    }

    if (id == wxID_ANY)
        m_windowId = NewControlId();
    else
        m_windowId = id;

    long msStyle = WS_VISIBLE | WS_TABSTOP | WS_CHILD | BS_OWNERDRAW ;

    if ( m_windowStyle & wxCLIP_SIBLINGS )
        msStyle |= WS_CLIPSIBLINGS;

#ifdef __WIN32__
    if(m_windowStyle & wxBU_LEFT)
        msStyle |= BS_LEFT;
    if(m_windowStyle & wxBU_RIGHT)
        msStyle |= BS_RIGHT;
    if(m_windowStyle & wxBU_TOP)
        msStyle |= BS_TOP;
    if(m_windowStyle & wxBU_BOTTOM)
        msStyle |= BS_BOTTOM;
#endif

    m_hWnd = (WXHWND) CreateWindowEx(
                    0,
                    wxT("BUTTON"),
                    wxEmptyString,
                    msStyle,
                    0, 0, 0, 0,
                    GetWinHwnd(parent),
                    (HMENU)m_windowId,
                    wxGetInstance(),
                    NULL
                   );

    // Subclass again for purposes of dialog editing mode
    SubclassWin(m_hWnd);

    SetPosition(pos);
    SetInitialSize(size);

    return true;
}

bool wxBitmapButton::SetBackgroundColour(const wxColour& colour)
{
    if ( !wxBitmapButtonBase::SetBackgroundColour(colour) )
    {
        // didn't change
        return false;
    }

    // invalidate the brush, it will be recreated the next time it's needed
    m_brushDisabled = wxNullBrush;

    return true;
}

void wxBitmapButton::OnSysColourChanged(wxSysColourChangedEvent& event)
{
    m_brushDisabled = wxNullBrush;

    if ( !IsEnabled() )
    {
        // this change affects our current state
        Refresh();
    }

    event.Skip();
}

void wxBitmapButton::OnMouseEnterOrLeave(wxMouseEvent& event)
{
    if ( IsEnabled() && m_bmpHover.Ok() )
        Refresh();

    event.Skip();
}

void wxBitmapButton::OnSetBitmap()
{
    // if the focus bitmap is specified but hover one isn't, use the focus
    // bitmap for hovering as well if this is consistent with the current
    // Windows version look and feel
    //
    // rationale: this is compatible with the old wxGTK behaviour and also
    // makes it much easier to do "the right thing" for all platforms (some of
    // them, such as Windows XP, have "hot" buttons while others don't)
    if ( !m_bmpHover.Ok() &&
            m_bmpFocus.Ok() &&
                wxUxThemeEngine::GetIfActive() )
    {
        m_bmpHover = m_bmpFocus;
    }

    // this will redraw us
    wxBitmapButtonBase::OnSetBitmap();
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

// VZ: should be at the very least less than wxDEFAULT_BUTTON_MARGIN
#define FOCUS_MARGIN 3

bool wxBitmapButton::MSWOnDraw(WXDRAWITEMSTRUCT *item)
{
#ifndef __WXWINCE__
    long style = GetWindowLong((HWND) GetHWND(), GWL_STYLE);
    if (style & BS_BITMAP)
    {
        // Let default procedure draw the bitmap, which is defined
        // in the Windows resource.
        return false;
    }
#endif

    LPDRAWITEMSTRUCT lpDIS = (LPDRAWITEMSTRUCT) item;
    HDC hDC                = lpDIS->hDC;
    UINT state             = lpDIS->itemState;
    bool isSelected        = (state & ODS_SELECTED) != 0;
    bool autoDraw          = (GetWindowStyleFlag() & wxBU_AUTODRAW) != 0;


    // choose the bitmap to use depending on the button state
    wxBitmap *bitmap;

    if ( isSelected && m_bmpSelected.Ok() )
        bitmap = &m_bmpSelected;
    else if ( m_bmpHover.Ok() && IsMouseInWindow() )
        bitmap = &m_bmpHover;
    else if ((state & ODS_FOCUS) && m_bmpFocus.Ok())
        bitmap = &m_bmpFocus;
    else if ((state & ODS_DISABLED) && m_bmpDisabled.Ok())
        bitmap = &m_bmpDisabled;
    else
        bitmap = &m_bmpNormal;

    if ( !bitmap->Ok() )
        return false;

    // centre the bitmap in the control area
    int x      = lpDIS->rcItem.left;
    int y      = lpDIS->rcItem.top;
    int width  = lpDIS->rcItem.right - x;
    int height = lpDIS->rcItem.bottom - y;
    int wBmp   = bitmap->GetWidth();
    int hBmp   = bitmap->GetHeight();

#if wxUSE_UXTHEME
    if ( autoDraw && wxUxThemeEngine::GetIfActive() )
    {
        MSWDrawXPBackground(this, item);
        wxUxThemeHandle theme(this, L"BUTTON");

        // calculate content area margins
        // assuming here that each state is the same size
        MARGINS margins;
        wxUxThemeEngine::Get()->GetThemeMargins(theme, NULL,
                                                BP_PUSHBUTTON, PBS_NORMAL,
                                                TMT_CONTENTMARGINS, NULL,
                                                &margins);
        int marginX = margins.cxLeftWidth + 1;
        int marginY = margins.cyTopHeight + 1;
        int x1,y1;

        if ( m_windowStyle & wxBU_LEFT )
        {
            x1 = x + marginX;
        }
        else if ( m_windowStyle & wxBU_RIGHT )
        {
            x1 = x + (width - wBmp) - marginX;
        }
        else
        {
            x1 = x + (width - wBmp) / 2;
        }

        if ( m_windowStyle & wxBU_TOP )
        {
            y1 = y + marginY;
        }
        else if ( m_windowStyle & wxBU_BOTTOM )
        {
            y1 = y + (height - hBmp) - marginY;
        }
        else
        {
            y1 = y + (height - hBmp) / 2;
        }

        // draw the bitmap
        wxDCTemp dst((WXHDC)hDC);
        dst.DrawBitmap(*bitmap, x1, y1, true);

        return true;
    }
#endif // wxUSE_UXTHEME

    int x1,y1;

    if(m_windowStyle & wxBU_LEFT)
        x1 = x + (FOCUS_MARGIN+1);
    else if(m_windowStyle & wxBU_RIGHT)
        x1 = x + (width - wBmp) - (FOCUS_MARGIN+1);
    else
        x1 = x + (width - wBmp) / 2;

    if(m_windowStyle & wxBU_TOP)
        y1 = y + (FOCUS_MARGIN+1);
    else if(m_windowStyle & wxBU_BOTTOM)
        y1 = y + (height - hBmp) - (FOCUS_MARGIN+1);
    else
        y1 = y + (height - hBmp) / 2;

    if ( isSelected && autoDraw )
    {
        x1++;
        y1++;
    }

    // draw the face, if auto-drawing
    if ( autoDraw )
    {
        DrawFace((WXHDC) hDC,
                 lpDIS->rcItem.left, lpDIS->rcItem.top,
                 lpDIS->rcItem.right, lpDIS->rcItem.bottom,
                 isSelected);
    }

    // draw the bitmap
    wxDCTemp dst((WXHDC)hDC);
    dst.DrawBitmap(*bitmap, x1, y1, true);

    // draw focus / disabled state, if auto-drawing
    if ( (state & ODS_DISABLED) && autoDraw )
    {
        DrawButtonDisable((WXHDC) hDC,
                          lpDIS->rcItem.left, lpDIS->rcItem.top,
                          lpDIS->rcItem.right, lpDIS->rcItem.bottom,
                          true);
    }
    else if ( (state & ODS_FOCUS) && autoDraw )
    {
        DrawButtonFocus((WXHDC) hDC,
                        lpDIS->rcItem.left,
                        lpDIS->rcItem.top,
                        lpDIS->rcItem.right,
                        lpDIS->rcItem.bottom,
                        isSelected);
    }

    return true;
}

// GRG Feb/2000, support for bmp buttons with Win95/98 standard LNF

void wxBitmapButton::DrawFace( WXHDC dc, int left, int top,
    int right, int bottom, bool sel )
{
    HPEN oldp;
    HPEN penHiLight;
    HPEN penLight;
    HPEN penShadow;
    HPEN penDkShadow;
    HBRUSH brushFace;

    // create needed pens and brush
    penHiLight  = CreatePen(PS_SOLID, 0, GetSysColor(COLOR_3DHILIGHT));
    penLight    = CreatePen(PS_SOLID, 0, GetSysColor(COLOR_3DLIGHT));
    penShadow   = CreatePen(PS_SOLID, 0, GetSysColor(COLOR_3DSHADOW));
    penDkShadow = CreatePen(PS_SOLID, 0, GetSysColor(COLOR_3DDKSHADOW));
    brushFace   = CreateSolidBrush(GetSysColor(COLOR_BTNFACE));

    // draw the rectangle
    RECT rect;
    rect.left   = left;
    rect.right  = right;
    rect.top    = top;
    rect.bottom = bottom;
    FillRect((HDC) dc, &rect, brushFace);

    // draw the border
    oldp = (HPEN) SelectObject( (HDC) dc, sel? penDkShadow : penHiLight);

    wxDrawLine((HDC) dc, left, top, right-1, top);
    wxDrawLine((HDC) dc, left, top+1, left, bottom-1);

    SelectObject( (HDC) dc, sel? penShadow : penLight);
    wxDrawLine((HDC) dc, left+1, top+1, right-2, top+1);
    wxDrawLine((HDC) dc, left+1, top+2, left+1, bottom-2);

    SelectObject( (HDC) dc, sel? penLight : penShadow);
    wxDrawLine((HDC) dc, left+1, bottom-2, right-1, bottom-2);
    wxDrawLine((HDC) dc, right-2, bottom-3, right-2, top);

    SelectObject( (HDC) dc, sel? penHiLight : penDkShadow);
    wxDrawLine((HDC) dc, left, bottom-1, right+2, bottom-1);
    wxDrawLine((HDC) dc, right-1, bottom-2, right-1, top-1);

    // delete allocated resources
    SelectObject((HDC) dc,oldp);
    DeleteObject(penHiLight);
    DeleteObject(penLight);
    DeleteObject(penShadow);
    DeleteObject(penDkShadow);
    DeleteObject(brushFace);
}

void wxBitmapButton::DrawButtonFocus( WXHDC dc, int left, int top, int right,
    int bottom, bool WXUNUSED(sel) )
{
    RECT rect;
    rect.left = left;
    rect.top = top;
    rect.right = right;
    rect.bottom = bottom;
    InflateRect( &rect, - FOCUS_MARGIN, - FOCUS_MARGIN );

    // GRG: the focus rectangle should not move when the button is pushed!
/*
    if ( sel )
        OffsetRect( &rect, 1, 1 );
*/

    DrawFocusRect( (HDC) dc, &rect );
}

void
wxBitmapButton::DrawButtonDisable( WXHDC dc,
                                   int left, int top, int right, int bottom,
                                   bool with_marg )
{
    if ( !m_brushDisabled.Ok() )
    {
        // draw a bitmap with two black and two background colour pixels
        wxBitmap bmp(2, 2);
        wxMemoryDC dc;
        dc.SelectObject(bmp);
        dc.SetPen(*wxBLACK_PEN);
        dc.DrawPoint(0, 0);
        dc.DrawPoint(1, 1);
        dc.SetPen(GetBackgroundColour());
        dc.DrawPoint(0, 1);
        dc.DrawPoint(1, 0);

        m_brushDisabled = wxBrush(bmp);
    }

    SelectInHDC selectBrush((HDC)dc, GetHbrushOf(m_brushDisabled));

    // ROP for "dest |= pattern" operation -- as it doesn't have a standard
    // name, give it our own
    static const DWORD PATTERNPAINT = 0xFA0089UL;

    if ( with_marg )
    {
        left += m_marginX;
        top += m_marginY;
        right -= 2 * m_marginX;
        bottom -= 2 * m_marginY;
    }

    ::PatBlt( (HDC) dc, left, top, right, bottom, PATTERNPAINT);
}

void wxBitmapButton::SetDefault()
{
    wxButton::SetDefault();
}

wxSize wxBitmapButton::DoGetBestSize() const
{
    if ( m_bmpNormal.Ok() )
    {
        int width = m_bmpNormal.GetWidth(),
            height = m_bmpNormal.GetHeight();
        int marginH = 0,
            marginV = 0;

#if wxUSE_UXTHEME
        if ( wxUxThemeEngine::GetIfActive() )
        {
            wxUxThemeHandle theme((wxBitmapButton *)this, L"BUTTON");

            MARGINS margins;
            wxUxThemeEngine::Get()->GetThemeMargins(theme, NULL,
                                                    BP_PUSHBUTTON, PBS_NORMAL,
                                                    TMT_CONTENTMARGINS, NULL,
                                                    &margins);

            // XP doesn't draw themed buttons correctly when the client area is
            // smaller than 8x8 - enforce this minimum size for small bitmaps
            if ( width < 8 )
                width = 8;
            if ( height < 8 )
                height = 8;

            // don't add margins for the borderless buttons, they don't need
            // them and it just makes them appear larger than needed
            if ( !HasFlag(wxBORDER_NONE) )
            {
                // we need 2 extra pixels for the focus rectangle, without them
                // it's overwritten by the bitmap itself
                marginH = margins.cxLeftWidth + margins.cxRightWidth + 2;
                marginV = margins.cyTopHeight + margins.cyBottomHeight + 2;
            }
        }
        else
#endif // wxUSE_UXTHEME
        {
            if ( !HasFlag(wxBORDER_NONE) )
            {
                marginH = 2*m_marginX;
                marginV = 2*m_marginY;
            }
        }

        wxSize best(width + marginH, height + marginV);
        CacheBestSize(best);
        return best;
    }

    // no idea what our best size should be, defer to the base class
    return wxBitmapButtonBase::DoGetBestSize();
}

#endif // wxUSE_BMPBUTTON
