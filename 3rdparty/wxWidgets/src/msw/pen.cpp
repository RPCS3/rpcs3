/////////////////////////////////////////////////////////////////////////////
// Name:        src/msw/pen.cpp
// Purpose:     wxPen
// Author:      Julian Smart
// Modified by:
// Created:     04/01/98
// RCS-ID:      $Id: pen.cpp 44818 2007-03-15 09:40:24Z JS $
// Copyright:   (c) Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#include "wx/pen.h"

#ifndef WX_PRECOMP
    #include <stdio.h>
    #include "wx/list.h"
    #include "wx/utils.h"
    #include "wx/app.h"
#endif

#include "wx/msw/private.h"

static int wx2msPenStyle(int wx_style);

IMPLEMENT_DYNAMIC_CLASS(wxPen, wxGDIObject)

wxPenRefData::wxPenRefData()
{
  m_style = wxSOLID;
  m_width = 1;
  m_join = wxJOIN_ROUND ;
  m_cap = wxCAP_ROUND ;
  m_nbDash = 0 ;
  m_dash = (wxDash*)NULL;
  m_hPen = 0;
}

wxPenRefData::wxPenRefData(const wxPenRefData& data)
             :wxGDIRefData()
{
    m_style = data.m_style;
    m_width = data.m_width;
    m_join = data.m_join;
    m_cap = data.m_cap;
    m_nbDash = data.m_nbDash;
    m_dash = data.m_dash;
    m_colour = data.m_colour;
    m_hPen = 0;
}

wxPenRefData::~wxPenRefData()
{
    if ( m_hPen )
        ::DeleteObject((HPEN) m_hPen);
}

// Pens

wxPen::wxPen()
{
}

wxPen::~wxPen()
{
}

// Should implement Create
wxPen::wxPen(const wxColour& col, int Width, int Style)
{
  m_refData = new wxPenRefData;

  M_PENDATA->m_colour = col;
//  M_PENDATA->m_stipple = NULL;
  M_PENDATA->m_width = Width;
  M_PENDATA->m_style = Style;
  M_PENDATA->m_join = wxJOIN_ROUND ;
  M_PENDATA->m_cap = wxCAP_ROUND ;
  M_PENDATA->m_nbDash = 0 ;
  M_PENDATA->m_dash = (wxDash*)NULL;
  M_PENDATA->m_hPen = 0 ;

  RealizeResource();
}

wxPen::wxPen(const wxBitmap& stipple, int Width)
{
    m_refData = new wxPenRefData;

//  M_PENDATA->m_colour = col;
    M_PENDATA->m_stipple = stipple;
    M_PENDATA->m_width = Width;
    M_PENDATA->m_style = wxSTIPPLE;
    M_PENDATA->m_join = wxJOIN_ROUND ;
    M_PENDATA->m_cap = wxCAP_ROUND ;
    M_PENDATA->m_nbDash = 0 ;
    M_PENDATA->m_dash = (wxDash*)NULL;
    M_PENDATA->m_hPen = 0 ;

    RealizeResource();
}

bool wxPen::RealizeResource()
{
   if ( !M_PENDATA || M_PENDATA->m_hPen )
       return false;

   if (M_PENDATA->m_style==wxTRANSPARENT)
   {
       M_PENDATA->m_hPen = (WXHPEN) ::GetStockObject(NULL_PEN);
       return true;
   }

   static const int os = wxGetOsVersion();
   COLORREF ms_colour = M_PENDATA->m_colour.GetPixel();

   // Join style, Cap style, Pen Stippling
#if !defined(__WXMICROWIN__) && !defined(__WXWINCE__)
   // Only NT can display dashed or dotted lines with width > 1
   if ( os != wxOS_WINDOWS_NT &&
           (M_PENDATA->m_style == wxDOT ||
            M_PENDATA->m_style == wxLONG_DASH ||
            M_PENDATA->m_style == wxSHORT_DASH ||
            M_PENDATA->m_style == wxDOT_DASH ||
            M_PENDATA->m_style == wxUSER_DASH) &&
            M_PENDATA->m_width > 1 )
   {
       M_PENDATA->m_width = 1;
   }

   if (M_PENDATA->m_join==wxJOIN_ROUND        &&
       M_PENDATA->m_cap==wxCAP_ROUND          &&
       M_PENDATA->m_style!=wxUSER_DASH        &&
       M_PENDATA->m_style!=wxSTIPPLE          &&
       M_PENDATA->m_width <= 1)
   {
       M_PENDATA->m_hPen =
         (WXHPEN) CreatePen( wx2msPenStyle(M_PENDATA->m_style),
                             M_PENDATA->m_width,
                             ms_colour );
   }
   else
   {
       DWORD ms_style = PS_GEOMETRIC | wx2msPenStyle(M_PENDATA->m_style);

       switch(M_PENDATA->m_join)
       {
           case wxJOIN_BEVEL: ms_style |= PS_JOIN_BEVEL; break;
           case wxJOIN_MITER: ms_style |= PS_JOIN_MITER; break;
           default:
           case wxJOIN_ROUND: ms_style |= PS_JOIN_ROUND; break;
       }

       switch(M_PENDATA->m_cap)
       {
           case wxCAP_PROJECTING: ms_style |= PS_ENDCAP_SQUARE;  break;
           case wxCAP_BUTT:       ms_style |= PS_ENDCAP_FLAT;    break;
           default:
           case wxCAP_ROUND:      ms_style |= PS_ENDCAP_ROUND;   break;
       }

       LOGBRUSH logb;

       switch(M_PENDATA->m_style)
       {
           case wxSTIPPLE:
               logb.lbStyle = BS_PATTERN ;
               if (M_PENDATA->m_stipple.Ok())
                   logb.lbHatch = (LONG)M_PENDATA->m_stipple.GetHBITMAP();
               else
                   logb.lbHatch = (LONG)0;
               break;
           case wxBDIAGONAL_HATCH:
               logb.lbStyle = BS_HATCHED;
               logb.lbHatch = HS_BDIAGONAL;
               break;
           case wxCROSSDIAG_HATCH:
               logb.lbStyle = BS_HATCHED;
               logb.lbHatch = HS_DIAGCROSS;
               break;
           case wxFDIAGONAL_HATCH:
               logb.lbStyle = BS_HATCHED;
               logb.lbHatch = HS_FDIAGONAL;
               break;
           case wxCROSS_HATCH:
               logb.lbStyle = BS_HATCHED;
               logb.lbHatch = HS_CROSS;
               break;
           case wxHORIZONTAL_HATCH:
               logb.lbStyle = BS_HATCHED;
               logb.lbHatch = HS_HORIZONTAL;
               break;
           case wxVERTICAL_HATCH:
               logb.lbStyle = BS_HATCHED;
               logb.lbHatch = HS_VERTICAL;
               break;
           default:
               logb.lbStyle = BS_SOLID;
#ifdef __WXDEBUG__
               // this should be unnecessary (it's unused) but suppresses the Purify
               // messages about uninitialized memory read
               logb.lbHatch = 0;
#endif
               break;
       }

       logb.lbColor = ms_colour;

       wxMSWDash *real_dash;
       if (M_PENDATA->m_style==wxUSER_DASH && M_PENDATA->m_nbDash && M_PENDATA->m_dash)
       {
           real_dash = new wxMSWDash[M_PENDATA->m_nbDash];
           int rw = M_PENDATA->m_width > 1 ? M_PENDATA->m_width : 1;
           for ( int i = 0; i < M_PENDATA->m_nbDash; i++ )
               real_dash[i] = M_PENDATA->m_dash[i] * rw;
       }
       else
       {
           real_dash = (wxMSWDash*)NULL;
       }

       M_PENDATA->m_hPen =
         (WXHPEN) ExtCreatePen( ms_style,
                                M_PENDATA->m_width,
                                &logb,
                                M_PENDATA->m_style == wxUSER_DASH
                                  ? M_PENDATA->m_nbDash
                                  : 0,
                                (LPDWORD)real_dash );

       delete [] real_dash;
   }
#else // WinCE
   M_PENDATA->m_hPen =
     (WXHPEN) CreatePen( wx2msPenStyle(M_PENDATA->m_style),
                         M_PENDATA->m_width,
                         ms_colour );
#endif // !WinCE/WinCE

   return true;
}

WXHANDLE wxPen::GetResourceHandle() const
{
    if ( !M_PENDATA )
        return 0;
    else
        return (WXHANDLE)M_PENDATA->m_hPen;
}

bool wxPen::FreeResource(bool WXUNUSED(force))
{
    if (M_PENDATA && (M_PENDATA->m_hPen != 0))
    {
        DeleteObject((HPEN) M_PENDATA->m_hPen);
        M_PENDATA->m_hPen = 0;
        return true;
    }
    else return false;
}

bool wxPen::IsFree() const
{
    return (M_PENDATA && M_PENDATA->m_hPen == 0);
}

void wxPen::Unshare()
{
    // Don't change shared data
    if (!m_refData)
    {
        m_refData = new wxPenRefData();
    }
    else
    {
        wxPenRefData* ref = new wxPenRefData(*(wxPenRefData*)m_refData);
        UnRef();
        m_refData = ref;
    }
}

void wxPen::SetColour(const wxColour& col)
{
    Unshare();

    M_PENDATA->m_colour = col;

    RealizeResource();
}

void wxPen::SetColour(unsigned char r, unsigned char g, unsigned char b)
{
    Unshare();

    M_PENDATA->m_colour.Set(r, g, b);

    RealizeResource();
}

void wxPen::SetWidth(int Width)
{
    Unshare();

    M_PENDATA->m_width = Width;

    RealizeResource();
}

void wxPen::SetStyle(int Style)
{
    Unshare();

    M_PENDATA->m_style = Style;

    RealizeResource();
}

void wxPen::SetStipple(const wxBitmap& Stipple)
{
    Unshare();

    M_PENDATA->m_stipple = Stipple;
    M_PENDATA->m_style = wxSTIPPLE;

    RealizeResource();
}

void wxPen::SetDashes(int nb_dashes, const wxDash *Dash)
{
    Unshare();

    M_PENDATA->m_nbDash = nb_dashes;
    M_PENDATA->m_dash = (wxDash *)Dash;

    RealizeResource();
}

void wxPen::SetJoin(int Join)
{
    Unshare();

    M_PENDATA->m_join = Join;

    RealizeResource();
}

void wxPen::SetCap(int Cap)
{
    Unshare();

    M_PENDATA->m_cap = Cap;

    RealizeResource();
}

int wx2msPenStyle(int wx_style)
{
#if !defined(__WXMICROWIN__) && !defined(__WXWINCE__)
    switch (wx_style)
    {
        case wxDOT:
            return PS_DOT;

        case wxDOT_DASH:
            return PS_DASHDOT;

        case wxSHORT_DASH:
        case wxLONG_DASH:
            return PS_DASH;

        case wxTRANSPARENT:
            return PS_NULL;

        case wxUSER_DASH:
            return PS_USERSTYLE;
    }
#else
    wxUnusedVar(wx_style);
#endif
    return PS_SOLID;
}
