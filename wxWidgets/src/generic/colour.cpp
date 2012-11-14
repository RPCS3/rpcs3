/////////////////////////////////////////////////////////////////////////////
// Name:        src/generic/colour.cpp
// Purpose:     wxColour class
// Author:      Julian Smart
// Modified by:
// Created:     01/02/97
// RCS-ID:      $Id: colour.cpp 41123 2006-09-10 02:00:24Z VZ $
// Copyright:   (c) Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#include "wx/colour.h"

#ifndef WX_PRECOMP
    #include "wx/gdicmn.h"
#endif

IMPLEMENT_DYNAMIC_CLASS(wxColour, wxObject)

// Colour

void wxColour::Init()
{
    m_red =
    m_blue =
    m_green = 0;
    m_alpha = wxALPHA_OPAQUE;
    m_isInit = false;
}

wxColour::wxColour()
{
    Init();
}

wxColour::wxColour(const wxColour& col)
{
    *this = col;
}

wxColour& wxColour::operator=(const wxColour& col)
{
    m_red = col.m_red;
    m_green = col.m_green;
    m_blue = col.m_blue;
    m_alpha = col.m_alpha;
    m_isInit = col.m_isInit;
    return *this;
}

wxColour::~wxColour()
{
}

void wxColour::InitRGBA(unsigned char r,
                        unsigned char g,
                        unsigned char b,
                        unsigned char a)
{
    m_red = r;
    m_green = g;
    m_blue = b;
    m_alpha = a;
    m_isInit = true;
}
