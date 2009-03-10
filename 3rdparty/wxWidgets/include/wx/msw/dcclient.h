/////////////////////////////////////////////////////////////////////////////
// Name:        dcclient.h
// Purpose:     wxClientDC class
// Author:      Julian Smart
// Modified by:
// Created:     01/02/97
// RCS-ID:      $Id: dcclient.h 53135 2008-04-12 02:31:04Z VZ $
// Copyright:   (c) Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_DCCLIENT_H_
#define _WX_DCCLIENT_H_

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#include "wx/dc.h"
#include "wx/dynarray.h"

// ----------------------------------------------------------------------------
// array types
// ----------------------------------------------------------------------------

// this one if used by wxPaintDC only
struct WXDLLIMPEXP_FWD_CORE wxPaintDCInfo;

WX_DECLARE_EXPORTED_OBJARRAY(wxPaintDCInfo, wxArrayDCInfo);

// ----------------------------------------------------------------------------
// DC classes
// ----------------------------------------------------------------------------

class WXDLLEXPORT wxWindowDC : public wxDC
{
public:
    // default ctor
    wxWindowDC();

    // Create a DC corresponding to the whole window
    wxWindowDC(wxWindow *win);

protected:
    // initialize the newly created DC
    void InitDC();

    // override some base class virtuals
    virtual void DoGetSize(int *width, int *height) const;

private:
    DECLARE_DYNAMIC_CLASS_NO_COPY(wxWindowDC)
};

class WXDLLEXPORT wxClientDC : public wxWindowDC
{
public:
    // default ctor
    wxClientDC();

    // Create a DC corresponding to the client area of the window
    wxClientDC(wxWindow *win);

    virtual ~wxClientDC();

protected:
    void InitDC();

    // override some base class virtuals
    virtual void DoGetSize(int *width, int *height) const;

private:
    DECLARE_DYNAMIC_CLASS_NO_COPY(wxClientDC)
};

class WXDLLEXPORT wxPaintDC : public wxClientDC
{
public:
    wxPaintDC();

    // Create a DC corresponding for painting the window in OnPaint()
    wxPaintDC(wxWindow *win);

    virtual ~wxPaintDC();

    // find the entry for this DC in the cache (keyed by the window)
    static WXHDC FindDCInCache(wxWindow* win);

protected:
    static wxArrayDCInfo ms_cache;

    // find the entry for this DC in the cache (keyed by the window)
    wxPaintDCInfo *FindInCache(size_t *index = NULL) const;

private:
    DECLARE_DYNAMIC_CLASS_NO_COPY(wxPaintDC)
};

/*
 * wxPaintDCEx
 * This class is used when an application sends an HDC with the WM_PAINT
 * message. It is used in HandlePaint and need not be used by an application.
 */

class WXDLLEXPORT wxPaintDCEx : public wxPaintDC
{
public:
    wxPaintDCEx(wxWindow *canvas, WXHDC dc);
    virtual ~wxPaintDCEx();
private:
    int saveState;

    DECLARE_CLASS(wxPaintDCEx)
    DECLARE_NO_COPY_CLASS(wxPaintDCEx)
};

#endif
    // _WX_DCCLIENT_H_
