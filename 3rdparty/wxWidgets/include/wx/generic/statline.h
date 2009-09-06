/////////////////////////////////////////////////////////////////////////////
// Name:        generic/statline.h
// Purpose:     a generic wxStaticLine class
// Author:      Vadim Zeitlin
// Created:     28.06.99
// Version:     $Id: statline.h 43874 2006-12-09 14:52:59Z VZ $
// Copyright:   (c) 1998 Vadim Zeitlin
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_GENERIC_STATLINE_H_
#define _WX_GENERIC_STATLINE_H_

class wxStaticBox;

// ----------------------------------------------------------------------------
// wxStaticLine
// ----------------------------------------------------------------------------

class WXDLLEXPORT wxStaticLine : public wxStaticLineBase
{
    DECLARE_DYNAMIC_CLASS(wxStaticLine)

public:
    // constructors and pseudo-constructors
    wxStaticLine() { m_statbox = NULL; }

    wxStaticLine( wxWindow *parent,
                  wxWindowID id = wxID_ANY,
                  const wxPoint &pos = wxDefaultPosition,
                  const wxSize &size = wxDefaultSize,
                  long style = wxLI_HORIZONTAL,
                  const wxString &name = wxStaticLineNameStr )
    {
        Create(parent, id, pos, size, style, name);
    }

    virtual ~wxStaticLine();

    bool Create( wxWindow *parent,
                 wxWindowID id = wxID_ANY,
                 const wxPoint &pos = wxDefaultPosition,
                 const wxSize &size = wxDefaultSize,
                 long style = wxLI_HORIZONTAL,
                 const wxString &name = wxStaticLineNameStr );

    // it's necessary to override this wxWindow function because we
    // will want to return the main widget for m_statbox
    //
    WXWidget GetMainWidget() const;

    // override wxWindow methods to make things work
    virtual void DoSetSize(int x, int y, int width, int height,
                           int sizeFlags = wxSIZE_AUTO);
    virtual void DoMoveWindow(int x, int y, int width, int height);
protected:
    // we implement the static line using a static box
    wxStaticBox *m_statbox;
};

#endif // _WX_GENERIC_STATLINE_H_

