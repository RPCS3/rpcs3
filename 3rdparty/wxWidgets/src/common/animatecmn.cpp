/////////////////////////////////////////////////////////////////////////////
// Name:        src/common/animatecmn.cpp
// Purpose:     wxAnimation and wxAnimationCtrl
// Author:      Francesco Montorsi
// Modified By:
// Created:     24/09/2006
// Id:          $Id: animatecmn.cpp 43494 2006-11-18 17:46:29Z RR $
// Copyright:   (c) Francesco Montorsi
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////


// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#if wxUSE_ANIMATIONCTRL

#include "wx/animate.h"
#include "wx/bitmap.h"
#include "wx/log.h"
#include "wx/brush.h"
#include "wx/image.h"
#include "wx/dcmemory.h"

const wxChar wxAnimationCtrlNameStr[] = wxT("animationctrl");

// global object
wxAnimation wxNullAnimation;

IMPLEMENT_ABSTRACT_CLASS(wxAnimationBase, wxObject)
IMPLEMENT_ABSTRACT_CLASS(wxAnimationCtrlBase, wxControl)


// ----------------------------------------------------------------------------
// wxAnimationCtrlBase
// ----------------------------------------------------------------------------

void wxAnimationCtrlBase::UpdateStaticImage()
{
    if (!m_bmpStaticReal.IsOk() || !m_bmpStatic.IsOk())
        return;

    // if given bitmap is not of the right size, recreate m_bmpStaticReal accordingly
    const wxSize &sz = GetClientSize();
    if (sz.GetWidth() != m_bmpStaticReal.GetWidth() ||
        sz.GetHeight() != m_bmpStaticReal.GetHeight())
    {
        if (!m_bmpStaticReal.IsOk() ||
            m_bmpStaticReal.GetWidth() != sz.GetWidth() ||
            m_bmpStaticReal.GetHeight() != sz.GetHeight())
        {
            // need to (re)create m_bmpStaticReal
            if (!m_bmpStaticReal.Create(sz.GetWidth(), sz.GetHeight(),
                                        m_bmpStatic.GetDepth()))
            {
                wxLogDebug(wxT("Cannot create the static bitmap"));
                m_bmpStatic = wxNullBitmap;
                return;
            }
        }

        if (m_bmpStatic.GetWidth() <= sz.GetWidth() &&
            m_bmpStatic.GetHeight() <= sz.GetHeight())
        {
            // clear the background of m_bmpStaticReal
            wxBrush brush(GetBackgroundColour());
            wxMemoryDC dc;
            dc.SelectObject(m_bmpStaticReal);
            dc.SetBackground(brush);
            dc.Clear();

            // center the user-provided bitmap in m_bmpStaticReal
            dc.DrawBitmap(m_bmpStatic,
                        (sz.GetWidth()-m_bmpStatic.GetWidth())/2,
                        (sz.GetHeight()-m_bmpStatic.GetHeight())/2,
                        true /* use mask */ );
        }
        else
        {
            // the user-provided bitmap is bigger than our control, strech it
            wxImage temp(m_bmpStatic.ConvertToImage());
            temp.Rescale(sz.GetWidth(), sz.GetHeight(), wxIMAGE_QUALITY_HIGH);
            m_bmpStaticReal = wxBitmap(temp);
        }
    }
}

void wxAnimationCtrlBase::SetInactiveBitmap(const wxBitmap &bmp)
{
    m_bmpStatic = bmp;
    m_bmpStaticReal = bmp;

    // if not playing, update the control now
    // NOTE: DisplayStaticImage() will call UpdateStaticImage automatically
    if ( !IsPlaying() )
        DisplayStaticImage();
}

#endif      // wxUSE_ANIMATIONCTRL
