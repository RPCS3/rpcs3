/////////////////////////////////////////////////////////////////////////////
// Name:        wx/display_impl.h
// Purpose:     wxDisplayImpl class declaration
// Author:      Vadim Zeitlin
// Created:     2006-03-15
// RCS-ID:      $Id: display_impl.h 41548 2006-10-02 05:38:05Z PC $
// Copyright:   (c) 2002-2006 Vadim Zeitlin <vadim@wxwindows.org>
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_DISPLAY_IMPL_H_BASE_
#define _WX_DISPLAY_IMPL_H_BASE_

#include "wx/gdicmn.h"      // for wxRect

// ----------------------------------------------------------------------------
// wxDisplayFactory: allows to create wxDisplay objects
// ----------------------------------------------------------------------------

class WXDLLEXPORT wxDisplayFactory
{
public:
    wxDisplayFactory() { }
    virtual ~wxDisplayFactory() { }

    // create a new display object
    //
    // it can return a NULL pointer if the display creation failed
    virtual wxDisplayImpl *CreateDisplay(unsigned n) = 0;

    // get the total number of displays
    virtual unsigned GetCount() = 0;

    // return the display for the given point or wxNOT_FOUND
    virtual int GetFromPoint(const wxPoint& pt) = 0;

    // return the display for the given window or wxNOT_FOUND
    //
    // the window pointer must not be NULL (i.e. caller should check it)
    virtual int GetFromWindow(wxWindow *window);
};

// ----------------------------------------------------------------------------
// wxDisplayImpl: base class for all wxDisplay implementations
// ----------------------------------------------------------------------------

class WXDLLEXPORT wxDisplayImpl
{
public:
    // virtual dtor for this base class
    virtual ~wxDisplayImpl() { }


    // return the full area of this display
    virtual wxRect GetGeometry() const = 0;

    // return the area of the display available for normal windows
    virtual wxRect GetClientArea() const { return GetGeometry(); }

    // return the name (may be empty)
    virtual wxString GetName() const = 0;

    // return the index of this display
    unsigned GetIndex() const { return m_index; }

    // return true if this is the primary monitor (usually one with index 0)
    virtual bool IsPrimary() const { return GetIndex() == 0; }


#if wxUSE_DISPLAY
    // implements wxDisplay::GetModes()
    virtual wxArrayVideoModes GetModes(const wxVideoMode& mode) const = 0;

    // get current video mode
    virtual wxVideoMode GetCurrentMode() const = 0;

    // change current mode, return true if succeeded, false otherwise
    virtual bool ChangeMode(const wxVideoMode& mode) = 0;
#endif // wxUSE_DISPLAY

protected:
    // create the object providing access to the display with the given index
    wxDisplayImpl(unsigned n) : m_index(n) { }


    // the index of this display (0 is always the primary one)
    const unsigned m_index;


    friend class wxDisplayFactory;

    DECLARE_NO_COPY_CLASS(wxDisplayImpl)
};

// ----------------------------------------------------------------------------
// wxDisplayFactorySingle
// ----------------------------------------------------------------------------

// this is a stub implementation using single/main display only, it is
// available even if wxUSE_DISPLAY == 0
class WXDLLEXPORT wxDisplayFactorySingle : public wxDisplayFactory
{
public:
    virtual wxDisplayImpl *CreateDisplay(unsigned n);
    virtual unsigned GetCount() { return 1; }
    virtual int GetFromPoint(const wxPoint& pt);
};

#endif // _WX_DISPLAY_IMPL_H_BASE_

