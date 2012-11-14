/////////////////////////////////////////////////////////////////////////////
// Name:        src/msw/brush.cpp
// Purpose:     wxBrush
// Author:      Julian Smart
// Modified by:
// Created:     04/01/98
// RCS-ID:      $Id: brush.cpp 42776 2006-10-30 22:03:53Z VZ $
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

#include "wx/brush.h"

#ifndef WX_PRECOMP
    #include "wx/list.h"
    #include "wx/utils.h"
    #include "wx/app.h"
#endif // WX_PRECOMP

#include "wx/msw/private.h"

// ----------------------------------------------------------------------------
// private classes
// ----------------------------------------------------------------------------

class WXDLLEXPORT wxBrushRefData: public wxGDIRefData
{
public:
    wxBrushRefData(const wxColour& colour = wxNullColour, int style = wxSOLID);
    wxBrushRefData(const wxBitmap& stipple);
    wxBrushRefData(const wxBrushRefData& data);
    virtual ~wxBrushRefData();

    bool operator==(const wxBrushRefData& data) const;

    HBRUSH GetHBRUSH();
    void Free();

    const wxColour& GetColour() const { return m_colour; }
    int GetStyle() const { return m_style; }
    wxBitmap *GetStipple() { return &m_stipple; }

    void SetColour(const wxColour& colour) { Free(); m_colour = colour; }
    void SetStyle(int style) { Free(); m_style = style; }
    void SetStipple(const wxBitmap& stipple) { Free(); DoSetStipple(stipple); }

private:
    void DoSetStipple(const wxBitmap& stipple);

    int           m_style;
    wxBitmap      m_stipple;
    wxColour      m_colour;
    HBRUSH        m_hBrush;

    // no assignment operator, the objects of this class are shared and never
    // assigned after being created once
    wxBrushRefData& operator=(const wxBrushRefData&);
};

#define M_BRUSHDATA ((wxBrushRefData *)m_refData)

// ============================================================================
// wxBrushRefData implementation
// ============================================================================

IMPLEMENT_DYNAMIC_CLASS(wxBrush, wxGDIObject)

// ----------------------------------------------------------------------------
// wxBrushRefData ctors/dtor
// ----------------------------------------------------------------------------

wxBrushRefData::wxBrushRefData(const wxColour& colour, int style)
              : m_colour(colour)
{
    m_style = style;

    m_hBrush = NULL;
}

wxBrushRefData::wxBrushRefData(const wxBitmap& stipple)
{
    DoSetStipple(stipple);

    m_hBrush = NULL;
}

wxBrushRefData::wxBrushRefData(const wxBrushRefData& data)
              : wxGDIRefData(),
                m_stipple(data.m_stipple),
                m_colour(data.m_colour)
{
    m_style = data.m_style;

    // we can't share HBRUSH, we'd need to create another one ourselves
    m_hBrush = NULL;
}

wxBrushRefData::~wxBrushRefData()
{
    Free();
}

// ----------------------------------------------------------------------------
// wxBrushRefData accesors
// ----------------------------------------------------------------------------

bool wxBrushRefData::operator==(const wxBrushRefData& data) const
{
    // don't compare HBRUSHes
    return m_style == data.m_style &&
           m_colour == data.m_colour &&
           m_stipple.IsSameAs(data.m_stipple);
}

void wxBrushRefData::DoSetStipple(const wxBitmap& stipple)
{
    m_stipple = stipple;
    m_style = stipple.GetMask() ? wxSTIPPLE_MASK_OPAQUE : wxSTIPPLE;
}

// ----------------------------------------------------------------------------
// wxBrushRefData resource handling
// ----------------------------------------------------------------------------

void wxBrushRefData::Free()
{
    if ( m_hBrush )
    {
        ::DeleteObject(m_hBrush);

        m_hBrush = NULL;
    }
}

#if !defined(__WXMICROWIN__) && !defined(__WXWINCE__)

static int TranslateHatchStyle(int style)
{
    switch ( style )
    {
        case wxBDIAGONAL_HATCH: return HS_BDIAGONAL;
        case wxCROSSDIAG_HATCH: return HS_DIAGCROSS;
        case wxFDIAGONAL_HATCH: return HS_FDIAGONAL;
        case wxCROSS_HATCH:     return HS_CROSS;
        case wxHORIZONTAL_HATCH:return HS_HORIZONTAL;
        case wxVERTICAL_HATCH:  return HS_VERTICAL;
        default:                return -1;
    }
}

#endif // !__WXMICROWIN__ && !__WXWINCE__

HBRUSH wxBrushRefData::GetHBRUSH()
{
    if ( !m_hBrush )
    {
#if !defined(__WXMICROWIN__) && !defined(__WXWINCE__)
        int hatchStyle = TranslateHatchStyle(m_style);
        if ( hatchStyle == -1 )
#endif // !__WXMICROWIN__ && !__WXWINCE__
        {
            switch ( m_style )
            {
                case wxTRANSPARENT:
                    m_hBrush = (HBRUSH)::GetStockObject(NULL_BRUSH);
                    break;

                case wxSTIPPLE:
                    m_hBrush = ::CreatePatternBrush(GetHbitmapOf(m_stipple));
                    break;

                case wxSTIPPLE_MASK_OPAQUE:
                    m_hBrush = ::CreatePatternBrush((HBITMAP)m_stipple.GetMask()
                                                        ->GetMaskBitmap());
                    break;

                default:
                    wxFAIL_MSG( _T("unknown brush style") );
                    // fall through

                case wxSOLID:
                    m_hBrush = ::CreateSolidBrush(m_colour.GetPixel());
                    break;
            }
        }
#ifndef __WXWINCE__
        else // create a hatched brush
        {
            m_hBrush = ::CreateHatchBrush(hatchStyle, m_colour.GetPixel());
        }
#endif

        if ( !m_hBrush )
        {
            wxLogLastError(_T("CreateXXXBrush()"));
        }
    }

    return m_hBrush;
}

// ============================================================================
// wxBrush implementation
// ============================================================================

// ----------------------------------------------------------------------------
// wxBrush ctors/dtor
// ----------------------------------------------------------------------------

wxBrush::wxBrush()
{
}

wxBrush::wxBrush(const wxColour& col, int style)
{
    m_refData = new wxBrushRefData(col, style);
}

wxBrush::wxBrush(const wxBitmap& stipple)
{
    m_refData = new wxBrushRefData(stipple);
}

wxBrush::~wxBrush()
{
}

// ----------------------------------------------------------------------------
// wxBrush house keeping stuff
// ----------------------------------------------------------------------------

bool wxBrush::operator==(const wxBrush& brush) const
{
    const wxBrushRefData *brushData = (wxBrushRefData *)brush.m_refData;

    // an invalid brush is considered to be only equal to another invalid brush
    return m_refData ? (brushData && *M_BRUSHDATA == *brushData) : !brushData;
}

wxObjectRefData *wxBrush::CreateRefData() const
{
    return new wxBrushRefData;
}

wxObjectRefData *wxBrush::CloneRefData(const wxObjectRefData *data) const
{
    return new wxBrushRefData(*(const wxBrushRefData *)data);
}

// ----------------------------------------------------------------------------
// wxBrush accessors
// ----------------------------------------------------------------------------

wxColour wxBrush::GetColour() const
{
    wxCHECK_MSG( Ok(), wxNullColour, _T("invalid brush") );

    return M_BRUSHDATA->GetColour();
}

int wxBrush::GetStyle() const
{
    wxCHECK_MSG( Ok(), 0, _T("invalid brush") );

    return M_BRUSHDATA->GetStyle();
}

wxBitmap *wxBrush::GetStipple() const
{
    wxCHECK_MSG( Ok(), NULL, _T("invalid brush") );

    return M_BRUSHDATA->GetStipple();
}

WXHANDLE wxBrush::GetResourceHandle() const
{
    wxCHECK_MSG( Ok(), FALSE, _T("invalid brush") );

    return (WXHANDLE)M_BRUSHDATA->GetHBRUSH();
}

// ----------------------------------------------------------------------------
// wxBrush setters
// ----------------------------------------------------------------------------

void wxBrush::SetColour(const wxColour& col)
{
    AllocExclusive();

    M_BRUSHDATA->SetColour(col);
}

void wxBrush::SetColour(unsigned char r, unsigned char g, unsigned char b)
{
    AllocExclusive();

    M_BRUSHDATA->SetColour(wxColour(r, g, b));
}

void wxBrush::SetStyle(int style)
{
    AllocExclusive();

    M_BRUSHDATA->SetStyle(style);
}

void wxBrush::SetStipple(const wxBitmap& stipple)
{
    AllocExclusive();

    M_BRUSHDATA->SetStipple(stipple);
}
