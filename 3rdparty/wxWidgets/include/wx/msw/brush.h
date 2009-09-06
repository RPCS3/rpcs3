/////////////////////////////////////////////////////////////////////////////
// Name:        wx/msw/brush.h
// Purpose:     wxBrush class
// Author:      Julian Smart
// Modified by:
// Created:     01/02/97
// RCS-ID:      $Id: brush.h 53135 2008-04-12 02:31:04Z VZ $
// Copyright:   (c) Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_BRUSH_H_
#define _WX_BRUSH_H_

#include "wx/gdicmn.h"
#include "wx/gdiobj.h"
#include "wx/bitmap.h"

class WXDLLIMPEXP_FWD_CORE wxBrush;

// ----------------------------------------------------------------------------
// wxBrush
// ----------------------------------------------------------------------------

class WXDLLEXPORT wxBrush : public wxBrushBase
{
public:
    wxBrush();
    wxBrush(const wxColour& col, int style = wxSOLID);
    wxBrush(const wxBitmap& stipple);
    virtual ~wxBrush();

    virtual void SetColour(const wxColour& col);
    virtual void SetColour(unsigned char r, unsigned char g, unsigned char b);
    virtual void SetStyle(int style);
    virtual void SetStipple(const wxBitmap& stipple);

    bool operator==(const wxBrush& brush) const;
    bool operator!=(const wxBrush& brush) const { return !(*this == brush); }

    wxColour GetColour() const;
    virtual int GetStyle() const;
    wxBitmap *GetStipple() const;

    bool Ok() const { return IsOk(); }
    bool IsOk() const { return m_refData != NULL; }

    // return the HBRUSH for this brush
    virtual WXHANDLE GetResourceHandle() const;

protected:
    virtual wxObjectRefData *CreateRefData() const;
    virtual wxObjectRefData *CloneRefData(const wxObjectRefData *data) const;

private:
    DECLARE_DYNAMIC_CLASS(wxBrush)
};

#endif
    // _WX_BRUSH_H_
