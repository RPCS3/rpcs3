///////////////////////////////////////////////////////////////////////////////
// Name:        wx/radiobox.h
// Purpose:     wxRadioBox declaration
// Author:      Vadim Zeitlin
// Modified by:
// Created:     10.09.00
// RCS-ID:      $Id: radiobox.h 54930 2008-08-02 19:45:23Z VZ $
// Copyright:   (c) Vadim Zeitlin
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

#ifndef _WX_RADIOBOX_H_BASE_
#define _WX_RADIOBOX_H_BASE_

#if wxUSE_RADIOBOX

#include "wx/ctrlsub.h"

#if wxUSE_TOOLTIPS

#include "wx/dynarray.h"

class WXDLLIMPEXP_FWD_CORE wxToolTip;

WX_DEFINE_EXPORTED_ARRAY_PTR(wxToolTip *, wxToolTipArray);

#endif // wxUSE_TOOLTIPS

extern WXDLLEXPORT_DATA(const wxChar) wxRadioBoxNameStr[];

// ----------------------------------------------------------------------------
// wxRadioBoxBase is not a normal base class, but rather a mix-in because the
// real wxRadioBox derives from different classes on different platforms: for
// example, it is a wxStaticBox in wxUniv and wxMSW but not in other ports
// ----------------------------------------------------------------------------

class WXDLLEXPORT wxRadioBoxBase : public wxItemContainerImmutable
{
public:
    virtual ~wxRadioBoxBase();

    // change/query the individual radio button state
    virtual bool Enable(unsigned int n, bool enable = true) = 0;
    virtual bool Show(unsigned int n, bool show = true) = 0;
    virtual bool IsItemEnabled(unsigned int n) const = 0;
    virtual bool IsItemShown(unsigned int n) const = 0;

    // return number of columns/rows in this radiobox
    unsigned int GetColumnCount() const { return m_numCols; }
    unsigned int GetRowCount() const { return m_numRows; }

    // return the next active (i.e. shown and not disabled) item above/below/to
    // the left/right of the given one
    int GetNextItem(int item, wxDirection dir, long style) const;

#if wxUSE_TOOLTIPS
    // set the tooltip text for a radio item, empty string unsets any tooltip
    void SetItemToolTip(unsigned int item, const wxString& text);

    // get the individual items tooltip; returns NULL if none
    wxToolTip *GetItemToolTip(unsigned int item) const
        { return m_itemsTooltips ? (*m_itemsTooltips)[item] : NULL; }
#endif // wxUSE_TOOLTIPS

#if wxUSE_HELP
    // set helptext for a particular item, pass an empty string to erase it
    void SetItemHelpText(unsigned int n, const wxString& helpText);

    // retrieve helptext for a particular item, empty string means no help text
    wxString GetItemHelpText(unsigned int n) const;
#else // wxUSE_HELP
    // just silently ignore the help text, it's better than requiring using
    // conditional compilation in all code using this function
    void SetItemHelpText(unsigned int WXUNUSED(n),
                         const wxString& WXUNUSED(helpText))
    {
    }
#endif // wxUSE_HELP

    // returns the radio item at the given position or wxNOT_FOUND if none
    // (currently implemented only under MSW and GTK)
    virtual int GetItemFromPoint(const wxPoint& WXUNUSED(pt)) const
    {
        return wxNOT_FOUND;
    }


    // deprecated functions
    // --------------------

#if WXWIN_COMPATIBILITY_2_4
    wxDEPRECATED( int GetNumberOfRowsOrCols() const );
    wxDEPRECATED( void SetNumberOfRowsOrCols(int n) );
#endif // WXWIN_COMPATIBILITY_2_4

protected:
    wxRadioBoxBase()
    {
        m_numCols =
        m_numRows =
        m_majorDim = 0;

#if wxUSE_TOOLTIPS
        m_itemsTooltips = NULL;
#endif // wxUSE_TOOLTIPS
    }

    // return the number of items in major direction (which depends on whether
    // we have wxRA_SPECIFY_COLS or wxRA_SPECIFY_ROWS style)
    unsigned int GetMajorDim() const { return m_majorDim; }

    // sets m_majorDim and also updates m_numCols/Rows
    //
    // the style parameter should be the style of the radiobox itself
    void SetMajorDim(unsigned int majorDim, long style);

#if wxUSE_TOOLTIPS
    // called from SetItemToolTip() to really set the tooltip for the specified
    // item in the box (or, if tooltip is NULL, to remove any existing one).
    //
    // NB: this function should really be pure virtual but to avoid breaking
    //     the build of the ports for which it's not implemented yet we provide
    //     an empty stub in the base class for now
    virtual void DoSetItemToolTip(unsigned int item, wxToolTip *tooltip);

    // returns true if we have any item tooltips
    bool HasItemToolTips() const { return m_itemsTooltips != NULL; }
#endif // wxUSE_TOOLTIPS

#if wxUSE_HELP
    // Retrieve help text for an item: this is a helper for the implementation
    // of wxWindow::GetHelpTextAtPoint() in the real radiobox class
    wxString DoGetHelpTextAtPoint(const wxWindow *derived,
                                  const wxPoint& pt,
                                  wxHelpEvent::Origin origin) const;
#endif // wxUSE_HELP

private:
    // the number of elements in major dimension (i.e. number of columns if
    // wxRA_SPECIFY_COLS or the number of rows if wxRA_SPECIFY_ROWS) and also
    // the number of rows/columns calculated from it
    unsigned int m_majorDim,
                 m_numCols,
                 m_numRows;

#if wxUSE_TOOLTIPS
    // array of tooltips for the individual items
    //
    // this array is initially NULL and initialized on first use
    wxToolTipArray *m_itemsTooltips;
#endif

#if wxUSE_HELP
    // help text associated with a particular item or empty string if none
    wxArrayString m_itemsHelpTexts;
#endif // wxUSE_HELP
};

#if defined(__WXUNIVERSAL__)
    #include "wx/univ/radiobox.h"
#elif defined(__WXMSW__)
    #include "wx/msw/radiobox.h"
#elif defined(__WXMOTIF__)
    #include "wx/motif/radiobox.h"
#elif defined(__WXGTK20__)
    #include "wx/gtk/radiobox.h"
#elif defined(__WXGTK__)
    #include "wx/gtk1/radiobox.h"
#elif defined(__WXMAC__)
    #include "wx/mac/radiobox.h"
#elif defined(__WXCOCOA__)
    #include "wx/cocoa/radiobox.h"
#elif defined(__WXPM__)
    #include "wx/os2/radiobox.h"
#elif defined(__WXPALMOS__)
    #include "wx/palmos/radiobox.h"
#endif

#endif // wxUSE_RADIOBOX

#endif // _WX_RADIOBOX_H_BASE_
