/////////////////////////////////////////////////////////////////////////////
// Name:        wx/statusbr.h
// Purpose:     wxStatusBar class interface
// Author:      Vadim Zeitlin
// Modified by:
// Created:     05.02.00
// RCS-ID:      $Id: statusbr.h 41035 2006-09-06 17:36:22Z PC $
// Copyright:   (c) Vadim Zeitlin
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_STATUSBR_H_BASE_
#define _WX_STATUSBR_H_BASE_

#include "wx/defs.h"

#if wxUSE_STATUSBAR

#include "wx/window.h"
#include "wx/list.h"
#include "wx/dynarray.h"

extern WXDLLIMPEXP_DATA_CORE(const wxChar) wxStatusBarNameStr[];

WX_DECLARE_LIST(wxString, wxListString);

// ----------------------------------------------------------------------------
// wxStatusBar constants
// ----------------------------------------------------------------------------

// style flags for fields
#define wxSB_NORMAL    0x0000
#define wxSB_FLAT      0x0001
#define wxSB_RAISED    0x0002

// ----------------------------------------------------------------------------
// wxStatusBar: a window near the bottom of the frame used for status info
// ----------------------------------------------------------------------------

class WXDLLEXPORT wxStatusBarBase : public wxWindow
{
public:
    wxStatusBarBase();

    virtual ~wxStatusBarBase();

    // field count
    // -----------

    // set the number of fields and call SetStatusWidths(widths) if widths are
    // given
    virtual void SetFieldsCount(int number = 1, const int *widths = NULL);
    int GetFieldsCount() const { return m_nFields; }

    // field text
    // ----------

    virtual void SetStatusText(const wxString& text, int number = 0) = 0;
    virtual wxString GetStatusText(int number = 0) const = 0;

    void PushStatusText(const wxString& text, int number = 0);
    void PopStatusText(int number = 0);

    // fields widths
    // -------------

    // set status field widths as absolute numbers: positive widths mean that
    // the field has the specified absolute width, negative widths are
    // interpreted as the sizer options, i.e. the extra space (total space
    // minus the sum of fixed width fields) is divided between the fields with
    // negative width according to the abs value of the width (field with width
    // -2 grows twice as much as one with width -1 &c)
    virtual void SetStatusWidths(int n, const int widths[]);

    // field styles
    // ------------

    // Set the field style. Use either wxSB_NORMAL (default) for a standard 3D
    // border around a field, wxSB_FLAT for no border around a field, so that it
    // appears flat or wxSB_POPOUT to make the field appear raised.
    // Setting field styles only works on wxMSW
    virtual void SetStatusStyles(int n, const int styles[]);

    // geometry
    // --------

    // Get the position and size of the field's internal bounding rectangle
    virtual bool GetFieldRect(int i, wxRect& rect) const = 0;

    // sets the minimal vertical size of the status bar
    virtual void SetMinHeight(int height) = 0;

    // get the dimensions of the horizontal and vertical borders
    virtual int GetBorderX() const = 0;
    virtual int GetBorderY() const = 0;

    // don't want status bars to accept the focus at all
    virtual bool AcceptsFocus() const { return false; }

protected:
    // set the widths array to NULL
    void InitWidths();

    // free the status widths arrays
    void FreeWidths();

    // reset the widths
    void ReinitWidths() { FreeWidths(); InitWidths(); }

    // same, for field styles
    void InitStyles();
    void FreeStyles();
    void ReinitStyles() { FreeStyles(); InitStyles(); }

    // same, for text stacks
    void InitStacks();
    void FreeStacks();
    void ReinitStacks() { FreeStacks(); InitStacks(); }

    // calculate the real field widths for the given total available size
    wxArrayInt CalculateAbsWidths(wxCoord widthTotal) const;

    // use these functions to access the stacks of field strings
    wxListString *GetStatusStack(int i) const;
    wxListString *GetOrCreateStatusStack(int i);

    // the current number of fields
    int        m_nFields;

    // the widths of the fields in pixels if !NULL, all fields have the same
    // width otherwise
    int       *m_statusWidths;

    // the styles of the fields
    int       *m_statusStyles;

    // stacks of previous values for PushStatusText/PopStatusText
    // this is created on demand, use GetStatusStack/GetOrCreateStatusStack
    wxListString **m_statusTextStacks;

    DECLARE_NO_COPY_CLASS(wxStatusBarBase)
};

// ----------------------------------------------------------------------------
// include the actual wxStatusBar class declaration
// ----------------------------------------------------------------------------

#if defined(__WXUNIVERSAL__)
    #define wxStatusBarUniv wxStatusBar

    #include "wx/univ/statusbr.h"
#elif defined(__WXPALMOS__)
    #define wxStatusBarPalm wxStatusBar

    #include "wx/palmos/statusbr.h"
#elif defined(__WIN32__) && wxUSE_NATIVE_STATUSBAR
    #define wxStatusBar95 wxStatusBar

    #include "wx/msw/statbr95.h"
#elif defined(__WXMAC__)
    #define wxStatusBarMac wxStatusBar

    #include "wx/generic/statusbr.h"
    #include "wx/mac/statusbr.h"
#else
    #define wxStatusBarGeneric wxStatusBar

    #include "wx/generic/statusbr.h"
#endif

#endif // wxUSE_STATUSBAR

#endif
    // _WX_STATUSBR_H_BASE_
