///////////////////////////////////////////////////////////////////////////////
// Name:        src/msw/ownerdrw.cpp
// Purpose:     implementation of wxOwnerDrawn class
// Author:      Vadim Zeitlin
// Modified by:
// Created:     13.11.97
// RCS-ID:      $Id: ownerdrw.cpp 60531 2009-05-06 16:04:20Z PC $
// Copyright:   (c) 1998 Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#ifndef WX_PRECOMP
    #include "wx/window.h"
    #include "wx/msw/private.h"
    #include "wx/font.h"
    #include "wx/bitmap.h"
    #include "wx/image.h"
    #include "wx/dcmemory.h"
    #include "wx/menu.h"
    #include "wx/utils.h"
    #include "wx/settings.h"
    #include "wx/menuitem.h"
    #include "wx/module.h"
#endif

#include "wx/ownerdrw.h"
#include "wx/fontutil.h"

#if wxUSE_OWNER_DRAWN

#ifndef SPI_GETKEYBOARDCUES
#define SPI_GETKEYBOARDCUES 0x100A
#endif

class wxMSWSystemMenuFontModule : public wxModule
{
public:

    virtual bool OnInit()
    {
        ms_systemMenuFont = new wxFont;

#if defined(__WXMSW__) && defined(__WIN32__) && defined(SM_CXMENUCHECK)
        NONCLIENTMETRICS nm;
        nm.cbSize = sizeof(NONCLIENTMETRICS);
        if ( !::SystemParametersInfo(SPI_GETNONCLIENTMETRICS,0,&nm,0) )
        {
#if WINVER >= 0x0600
            // a new field has been added to NONCLIENTMETRICS under Vista, so
            // the call to SystemParametersInfo() fails if we use the struct
            // size incorporating this new value on an older system -- retry
            // without it
            nm.cbSize -= sizeof(int);
            if ( !::SystemParametersInfo(SPI_GETNONCLIENTMETRICS, 0, &nm, 0) )
#endif // WINVER >= 0x0600
            {
                // maybe we should initialize the struct with some defaults?
                wxLogLastError(_T("SystemParametersInfo(SPI_GETNONCLIENTMETRICS)"));
            }
        }

        ms_systemMenuButtonWidth = nm.iMenuHeight;
        ms_systemMenuHeight = nm.iMenuHeight;

        // create menu font
        wxNativeFontInfo info;
        memcpy(&info.lf, &nm.lfMenuFont, sizeof(LOGFONT));
        ms_systemMenuFont->Create(info);

        if (SystemParametersInfo(SPI_GETKEYBOARDCUES, 0, &ms_showCues, 0) == 0)
            ms_showCues = true;
#endif

        return true;
    }

    virtual void OnExit()
    {
        delete ms_systemMenuFont;
        ms_systemMenuFont = NULL;
    }

    static wxFont* ms_systemMenuFont;
    static int ms_systemMenuButtonWidth;   // windows clean install default
    static int ms_systemMenuHeight;        // windows clean install default
    static bool ms_showCues;
private:
    DECLARE_DYNAMIC_CLASS(wxMSWSystemMenuFontModule)
};

// these static variables are from the wxMSWSystemMenuFontModule object
// and reflect the system settings returned by the Win32 API's
// SystemParametersInfo() call.

wxFont* wxMSWSystemMenuFontModule::ms_systemMenuFont = NULL;
int wxMSWSystemMenuFontModule::ms_systemMenuButtonWidth = 18;   // windows clean install default
int wxMSWSystemMenuFontModule::ms_systemMenuHeight = 18;        // windows clean install default
bool wxMSWSystemMenuFontModule::ms_showCues = true;

IMPLEMENT_DYNAMIC_CLASS(wxMSWSystemMenuFontModule, wxModule)


// VC++ 6 gives a warning here:
//
//      return type for 'OwnerDrawnSet_wxImplementation_HashTable::iterator::
//      operator ->' is 'class wxOwnerDrawn ** ' (ie; not a UDT or reference to
//      a UDT.  Will produce errors if applied using infix notation.
//
// shut it down
#if defined __VISUALC__ && __VISUALC__ <= 1300
    #if __VISUALC__ >= 1200
        #pragma warning(push)
        #define POP_WARNINGS
    #endif
    #pragma warning(disable: 4284)
#endif

#include "wx/hashset.h"
WX_DECLARE_HASH_SET(wxOwnerDrawn*, wxPointerHash, wxPointerEqual, OwnerDrawnSet);

#ifdef POP_WARNINGS
    #pragma warning(pop)
#endif

// ============================================================================
// implementation of wxOwnerDrawn class
// ============================================================================

// ctor
// ----
wxOwnerDrawn::wxOwnerDrawn(const wxString& str,
                           bool bCheckable,
                           bool bMenuItem)
            : m_strName(str)
{
    if ( ms_nDefaultMarginWidth == 0 )
    {
       ms_nDefaultMarginWidth = ::GetSystemMetrics(SM_CXMENUCHECK) +
                                wxSystemSettings::GetMetric(wxSYS_EDGE_X);
       ms_nLastMarginWidth = ms_nDefaultMarginWidth;
    }

    m_bCheckable   = bCheckable;
    m_bOwnerDrawn  = false;
    m_isMenuItem   = bMenuItem;
    m_nHeight      = 0;
    m_nMarginWidth = ms_nLastMarginWidth;
    m_nMinHeight   = wxMSWSystemMenuFontModule::ms_systemMenuHeight;
}

wxOwnerDrawn::~wxOwnerDrawn()
{
}

bool wxOwnerDrawn::IsMenuItem() const
{
    return m_isMenuItem;
}


// these items will be set during the first invocation of the c'tor,
// because the values will be determined by checking the system settings,
// which is a chunk of code
size_t wxOwnerDrawn::ms_nDefaultMarginWidth = 0;
size_t wxOwnerDrawn::ms_nLastMarginWidth = 0;


// drawing
// -------

wxFont wxOwnerDrawn::GetFontToUse() const
{
    wxFont font = m_font;
    if ( !font.Ok() )
    {
        if ( IsMenuItem() )
            font = *wxMSWSystemMenuFontModule::ms_systemMenuFont;

        if ( !font.Ok() )
            font = *wxNORMAL_FONT;
    }

    return font;
}

// get size of the item
// The item size includes the menu string, the accel string,
// the bitmap and size for a submenu expansion arrow...
bool wxOwnerDrawn::OnMeasureItem(size_t *pwidth, size_t *pheight)
{
    if ( IsOwnerDrawn() )
    {
        wxMemoryDC dc;

        wxString str = wxStripMenuCodes(m_strName);

        // if we have a valid accel string, then pad out
        // the menu string so that the menu and accel string are not
        // placed on top of each other.
        if ( !m_strAccel.empty() )
        {
            str.Pad(str.length()%8);
            str += m_strAccel;
        }

        dc.SetFont(GetFontToUse());

        dc.GetTextExtent(str, (long *)pwidth, (long *)pheight);

        // add space at the end of the menu for the submenu expansion arrow
        // this will also allow offsetting the accel string from the right edge
        *pwidth += GetMarginWidth() + 16;
    }
    else // don't draw the text, just the bitmap (if any)
    {
        *pwidth =
        *pheight = 0;
    }

    // increase size to accommodate bigger bitmaps if necessary
    if (m_bmpChecked.Ok())
    {
        // Is BMP height larger then text height?
        size_t adjustedHeight = m_bmpChecked.GetHeight() +
                                  2*wxSystemSettings::GetMetric(wxSYS_EDGE_Y);
        if (*pheight < adjustedHeight)
            *pheight = adjustedHeight;

        const size_t widthBmp = m_bmpChecked.GetWidth();
        if ( IsOwnerDrawn() )
        {
            // widen the margin to fit the bitmap if necessary
            if ((size_t)GetMarginWidth() < widthBmp)
                SetMarginWidth(widthBmp);
        }
        else // we must allocate enough space for the bitmap
        {
            *pwidth += widthBmp;
        }
    }

    // add a 4-pixel separator, otherwise menus look cluttered
    *pwidth += 4;

    // make sure that this item is at least as tall as the system menu height
    if ( *pheight < m_nMinHeight )
      *pheight = m_nMinHeight;

    // remember height for use in OnDrawItem
    m_nHeight = *pheight;

    return true;
}

// draw the item
bool wxOwnerDrawn::OnDrawItem(wxDC& dc,
                              const wxRect& rc,
                              wxODAction,
                              wxODStatus st)
{
    // this flag determines whether or not an edge will
    // be drawn around the bitmap. In most "windows classic"
    // applications, a 1-pixel highlight edge is drawn around
    // the bitmap of an item when it is selected.  However,
    // with the new "luna" theme, no edge is drawn around
    // the bitmap because the background is white (this applies
    // only to "non-XP style" menus w/ bitmaps --
    // see IE 6 menus for an example)

    bool draw_bitmap_edge = true;

    // set the colors
    // --------------
    DWORD colBack, colText;
    if ( st & wxODSelected )
    {
        colBack = GetSysColor(COLOR_HIGHLIGHT);
        if (!(st & wxODDisabled))
        {
            colText = GetSysColor(COLOR_HIGHLIGHTTEXT);
        }
        else
        {
            colText = GetSysColor(COLOR_GRAYTEXT);
        }
    }
    else
    {
        // fall back to default colors if none explicitly specified
        colBack = m_colBack.Ok() ? wxColourToPalRGB(m_colBack)
                                 : GetSysColor(COLOR_MENU);
        colText = m_colText.Ok() ? wxColourToPalRGB(m_colText)
                                 : GetSysColor(COLOR_MENUTEXT);
    }

    if ( IsOwnerDrawn() )
    {
        // don't draw an edge around the bitmap, if background is white ...
        DWORD menu_bg_color = GetSysColor(COLOR_MENU);
        if (    ( GetRValue( menu_bg_color ) >= 0xf0 &&
                  GetGValue( menu_bg_color ) >= 0xf0 &&
                  GetBValue( menu_bg_color ) >= 0xf0 )
          )
        {
            draw_bitmap_edge = false;
        }
    }
    else // edge doesn't look well with default Windows drawing
    {
        draw_bitmap_edge = false;
    }


    HDC hdc = GetHdcOf(dc);
    COLORREF colOldText = ::SetTextColor(hdc, colText),
             colOldBack = ::SetBkColor(hdc, colBack);

    // *2, as in wxSYS_EDGE_Y
    int margin = GetMarginWidth() + 2 * wxSystemSettings::GetMetric(wxSYS_EDGE_X);

    // select the font and draw the text
    // ---------------------------------


    // determine where to draw and leave space for a check-mark.
    // + 1 pixel to separate the edge from the highlight rectangle
    int xText = rc.x + margin + 1;


    // using native API because it recognizes '&'
    if ( IsOwnerDrawn() )
    {
        int nPrevMode = SetBkMode(hdc, TRANSPARENT);
        AutoHBRUSH hbr(colBack);
        SelectInHDC selBrush(hdc, hbr);

        RECT rectFill = { rc.GetLeft(), rc.GetTop(),
                            rc.GetRight() + 1, rc.GetBottom() + 1 };

        if ( (st & wxODSelected) && m_bmpChecked.Ok() && draw_bitmap_edge )
        {
            // only draw the highlight under the text, not under
            // the bitmap or checkmark
            rectFill.left = xText;
        }

        FillRect(hdc, &rectFill, hbr);

        // use default font if no font set
        wxFont fontToUse = GetFontToUse();
        SelectInHDC selFont(hdc, GetHfontOf(fontToUse));

        wxString strMenuText = m_strName.BeforeFirst('\t');

        xText += 3; // separate text from the highlight rectangle

        SIZE sizeRect;
        ::GetTextExtentPoint32(hdc, strMenuText.c_str(), strMenuText.length(), &sizeRect);
        ::DrawState(hdc, NULL, NULL,
                    (LPARAM)strMenuText.c_str(), strMenuText.length(),
                    xText, rc.y + (int) ((rc.GetHeight()-sizeRect.cy)/2.0), // centre text vertically
                    rc.GetWidth()-margin, sizeRect.cy,
                    DST_PREFIXTEXT |
                    (((st & wxODDisabled) && !(st & wxODSelected)) ? DSS_DISABLED : 0) |
                    (((st & wxODHidePrefix) && !wxMSWSystemMenuFontModule::ms_showCues) ? 512 : 0)); // 512 == DSS_HIDEPREFIX

        // ::SetTextAlign(hdc, TA_RIGHT) doesn't work with DSS_DISABLED or DSS_MONO
        // as the last parameter in DrawState() (at least with Windows98). So we have
        // to take care of right alignment ourselves.
        if ( !m_strAccel.empty() )
        {
            int accel_width, accel_height;
            dc.GetTextExtent(m_strAccel, &accel_width, &accel_height);
            // right align accel string with right edge of menu ( offset by the
            // margin width )
            ::DrawState(hdc, NULL, NULL,
                    (LPARAM)m_strAccel.c_str(), m_strAccel.length(),
                    rc.GetWidth()-16-accel_width, rc.y+(int) ((rc.GetHeight()-sizeRect.cy)/2.0),
                    0, 0,
                    DST_TEXT |
                    (((st & wxODDisabled) && !(st & wxODSelected)) ? DSS_DISABLED : 0));
        }

        (void)SetBkMode(hdc, nPrevMode);
    }


    // draw the bitmap
    // ---------------
    if ( IsCheckable() && !m_bmpChecked.Ok() )
    {
        if ( st & wxODChecked )
        {
            // what goes on: DrawFrameControl creates a b/w mask,
            // then we copy it to screen to have right colors

            // first create a monochrome bitmap in a memory DC
            HDC hdcMem = CreateCompatibleDC(hdc);
            HBITMAP hbmpCheck = CreateBitmap(margin, m_nHeight, 1, 1, 0);
            SelectObject(hdcMem, hbmpCheck);

            // then draw a check mark into it
            RECT rect = { 0, 0, margin, m_nHeight };
            if ( m_nHeight > 0 )
            {
                ::DrawFrameControl(hdcMem, &rect, DFC_MENU, DFCS_MENUCHECK);
            }

            // finally copy it to screen DC and clean up
            BitBlt(hdc, rc.x, rc.y, margin, m_nHeight, hdcMem, 0, 0, SRCCOPY);

            DeleteDC(hdcMem);
            DeleteObject(hbmpCheck);
        }
    }
    else
    {
        wxBitmap bmp;

        if ( st & wxODDisabled )
        {
            bmp = GetDisabledBitmap();
        }

        if ( !bmp.Ok() )
        {
            // for not checkable bitmaps we should always use unchecked one
            // because their checked bitmap is not set
            bmp = GetBitmap(!IsCheckable() || (st & wxODChecked));

#if wxUSE_IMAGE
            if ( bmp.Ok() && st & wxODDisabled )
            {
                // we need to grey out the bitmap as we don't have any specific
                // disabled bitmap
                wxImage imgGrey = bmp.ConvertToImage().ConvertToGreyscale();
                if ( imgGrey.Ok() )
                    bmp = wxBitmap(imgGrey);
            }
#endif // wxUSE_IMAGE
        }

        if ( bmp.Ok() )
        {
            wxMemoryDC dcMem(&dc);
            dcMem.SelectObjectAsSource(bmp);

            // center bitmap
            int nBmpWidth = bmp.GetWidth(),
                nBmpHeight = bmp.GetHeight();

            // there should be enough space!
            wxASSERT((nBmpWidth <= rc.GetWidth()) && (nBmpHeight <= rc.GetHeight()));

            int heightDiff = m_nHeight - nBmpHeight;
            dc.Blit(rc.x + (margin - nBmpWidth) / 2,
                    rc.y + heightDiff / 2,
                    nBmpWidth, nBmpHeight,
                    &dcMem, 0, 0, wxCOPY, true /* use mask */);

            if ( ( st & wxODSelected ) && !( st & wxODDisabled ) && draw_bitmap_edge )
            {
                RECT rectBmp = { rc.GetLeft(), rc.GetTop(),
                                 rc.GetLeft() + margin,
                                 rc.GetTop() + m_nHeight };
                SetBkColor(hdc, colBack);

                DrawEdge(hdc, &rectBmp, BDR_RAISEDINNER, BF_RECT);
            }
        }
    }

    ::SetTextColor(hdc, colOldText);
    ::SetBkColor(hdc, colOldBack);

    return true;
}


#endif // wxUSE_OWNER_DRAWN
