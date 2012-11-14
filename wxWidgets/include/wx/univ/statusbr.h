///////////////////////////////////////////////////////////////////////////////
// Name:        wx/univ/statusbr.h
// Purpose:     wxStatusBarUniv: wxStatusBar for wxUniversal declaration
// Author:      Vadim Zeitlin
// Modified by:
// Created:     14.10.01
// RCS-ID:      $Id: statusbr.h 37393 2006-02-08 21:47:09Z VZ $
// Copyright:   (c) 2001 SciTech Software, Inc. (www.scitechsoft.com)
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

#ifndef _WX_UNIV_STATUSBR_H_
#define _WX_UNIV_STATUSBR_H_

#include "wx/univ/inpcons.h"
#include "wx/arrstr.h"

// ----------------------------------------------------------------------------
// wxStatusBar: a window near the bottom of the frame used for status info
// ----------------------------------------------------------------------------

class WXDLLEXPORT wxStatusBarUniv : public wxStatusBarBase,
                                    public wxInputConsumer
{
public:
    wxStatusBarUniv() { Init(); }

    wxStatusBarUniv(wxWindow *parent,
                    wxWindowID id = wxID_ANY,
                    long style = 0,
                    const wxString& name = wxPanelNameStr)
    {
        Init();

        (void)Create(parent, id, style, name);
    }

    bool Create(wxWindow *parent,
                wxWindowID id = wxID_ANY,
                long style = 0,
                const wxString& name = wxPanelNameStr);

    // set field count/widths
    virtual void SetFieldsCount(int number = 1, const int *widths = NULL);
    virtual void SetStatusWidths(int n, const int widths[]);

    // get/set the text of the given field
    virtual void SetStatusText(const wxString& text, int number = 0);
    virtual wxString GetStatusText(int number = 0) const;

    // Get the position and size of the field's internal bounding rectangle
    virtual bool GetFieldRect(int i, wxRect& rect) const;

    // sets the minimal vertical size of the status bar
    virtual void SetMinHeight(int height);

    // get the dimensions of the horizontal and vertical borders
    virtual int GetBorderX() const;
    virtual int GetBorderY() const;

    // wxInputConsumer pure virtual
    virtual wxWindow *GetInputWindow() const
        { return wx_const_cast(wxStatusBar*, this); }

protected:
    // recalculate the field widths
    void OnSize(wxSizeEvent& event);

    // draw the statusbar
    virtual void DoDraw(wxControlRenderer *renderer);

    // tell them about our preferred height
    virtual wxSize DoGetBestSize() const;

    // override DoSetSize() to prevent the status bar height from changing
    virtual void DoSetSize(int x, int y,
                           int width, int height,
                           int sizeFlags = wxSIZE_AUTO);

    // get the (fixed) status bar height
    wxCoord GetHeight() const;

    // get the rectangle containing all the fields and the border between them
    //
    // also updates m_widthsAbs if necessary
    wxRect GetTotalFieldRect(wxCoord *borderBetweenFields);

    // get the rect for this field without ani side effects (see code)
    wxRect DoGetFieldRect(int n) const;

    // refresh the given field
    void RefreshField(int i);

    // common part of all ctors
    void Init();

private:
    // the status fields strings
    wxArrayString m_statusText;

    // the absolute status fields widths
    wxArrayInt m_widthsAbs;

    DECLARE_DYNAMIC_CLASS(wxStatusBarUniv)
    DECLARE_EVENT_TABLE()
    WX_DECLARE_INPUT_CONSUMER()
};

#endif // _WX_UNIV_STATUSBR_H_

