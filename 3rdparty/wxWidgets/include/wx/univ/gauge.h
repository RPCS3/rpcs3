///////////////////////////////////////////////////////////////////////////////
// Name:        wx/univ/gauge.h
// Purpose:     wxUniversal wxGauge declaration
// Author:      Vadim Zeitlin
// Modified by:
// Created:     20.02.01
// RCS-ID:      $Id: gauge.h 35698 2005-09-25 20:49:40Z MW $
// Copyright:   (c) 2001 SciTech Software, Inc. (www.scitechsoft.com)
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

#ifndef _WX_UNIV_GAUGE_H_
#define _WX_UNIV_GAUGE_H_

// ----------------------------------------------------------------------------
// wxGauge: a progress bar
// ----------------------------------------------------------------------------

class WXDLLEXPORT wxGauge : public wxGaugeBase
{
public:
    wxGauge() { Init(); }

    wxGauge(wxWindow *parent,
            wxWindowID id,
            int range,
            const wxPoint& pos = wxDefaultPosition,
            const wxSize& size = wxDefaultSize,
            long style = wxGA_HORIZONTAL,
            const wxValidator& validator = wxDefaultValidator,
            const wxString& name = wxGaugeNameStr)
    {
        Init();

        (void)Create(parent, id, range, pos, size, style, validator, name);
    }

    bool Create(wxWindow *parent,
                wxWindowID id,
                int range,
                const wxPoint& pos = wxDefaultPosition,
                const wxSize& size = wxDefaultSize,
                long style = wxGA_HORIZONTAL,
                const wxValidator& validator = wxDefaultValidator,
                const wxString& name = wxGaugeNameStr);

    // implement base class virtuals
    virtual void SetRange(int range);
    virtual void SetValue(int pos);

    // wxUniv-specific methods

    // is it a smooth progress bar or a discrete one?
    bool IsSmooth() const { return (GetWindowStyle() & wxGA_SMOOTH) != 0; }

    // is it a vertica; progress bar or a horizontal one?
    bool IsVertical() const { return (GetWindowStyle() & wxGA_VERTICAL) != 0; }

protected:
    // common part of all ctors
    void Init();

    // return the def border for a progress bar
    virtual wxBorder GetDefaultBorder() const;

    // return the default size
    virtual wxSize DoGetBestClientSize() const;

    // draw the control
    virtual void DoDraw(wxControlRenderer *renderer);

    DECLARE_DYNAMIC_CLASS(wxGauge)
};

#endif // _WX_UNIV_GAUGE_H_
