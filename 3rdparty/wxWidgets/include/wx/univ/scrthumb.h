///////////////////////////////////////////////////////////////////////////////
// Name:        wx/univ/scrthumb.h
// Purpose:     wxScrollThumb class
// Author:      Vadim Zeitlin
// Modified by:
// Created:     12.02.01
// RCS-ID:      $Id: scrthumb.h 37168 2006-01-26 19:35:32Z ABX $
// Copyright:   (c) 2001 SciTech Software, Inc. (www.scitechsoft.com)
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

#ifndef _WX_UNIV_SCRTHUMB_H_
#define _WX_UNIV_SCRTHUMB_H_

// ----------------------------------------------------------------------------
// wxScrollThumb is not a control but just a class containing the common
// functionality of scroll thumb such as used by scrollbars, sliders and maybe
// other (user) controls
//
// This class is similar to wxScrollThumb.
// ----------------------------------------------------------------------------

class WXDLLEXPORT wxControlWithThumb;
class WXDLLEXPORT wxMouseEvent;
class WXDLLEXPORT wxRect;
class WXDLLEXPORT wxScrollTimer;

#include "wx/timer.h"

// ----------------------------------------------------------------------------
// wxScrollThumb: an abstraction of scrollbar thumb
// ----------------------------------------------------------------------------

class WXDLLEXPORT wxScrollThumb
{
public:
    enum Shaft
    {
        Shaft_None = -1,
        Shaft_Above,    // or to the left of the thumb
        Shaft_Below,    // or to the right of the thumb
        Shaft_Thumb,    // on the thumb
        Shaft_Max
    };

    // ctor requires a back pointer to wxControlWithThumb
    wxScrollThumb(wxControlWithThumb *control);

    // process a mouse click: will capture the mouse if the button was pressed
    // on either the thumb (start dragging it then) or the shaft (start
    // scrolling)
    bool HandleMouse(const wxMouseEvent& event) const;

    // process a mouse move
    bool HandleMouseMove(const wxMouseEvent& event) const;

    // dtor
    ~wxScrollThumb();

private:
    // do we have the mouse capture?
    bool HasCapture() const { return m_captureData != NULL; }

    // get the coord of this event in the direction we're interested in (y for
    // vertical shaft or x for horizontal ones)
    wxCoord GetMouseCoord(const wxMouseEvent& event) const;

    // get the position of the thumb corresponding to the current mouse
    // position (can only be called while we're dragging the thumb!)
    int GetThumbPos(const wxMouseEvent& event) const;

    // the main control
    wxControlWithThumb *m_control;

    // the part of it where the mouse currently is
    Shaft m_shaftPart;

    // the data for the mouse capture
    struct WXDLLEXPORT wxScrollThumbCaptureData *m_captureData;
};

// ----------------------------------------------------------------------------
// wxControlWithThumb: interface implemented by controls using wxScrollThumb
// ----------------------------------------------------------------------------

class WXDLLEXPORT wxControlWithThumb
{
public:
    virtual ~wxControlWithThumb() {}

    // simple accessors
    // ----------------

    // get the controls window (used for mouse capturing)
    virtual wxWindow *GetWindow() = 0;

    // get the orientation of the shaft (vertical or horizontal)
    virtual bool IsVertical() const = 0;

    // geometry functions
    // ------------------

    // hit testing: return part of the shaft the point is in (or Shaft_None)
    virtual wxScrollThumb::Shaft HitTest(const wxPoint& pt) const = 0;

    // get the current position in pixels of the thumb
    virtual wxCoord ThumbPosToPixel() const = 0;

    // transform from pixel offset to the thumb logical position
    virtual int PixelToThumbPos(wxCoord x) const = 0;

    // callbacks
    // ---------

    // set or clear the specified flag in the arrow state: this function is
    // responsible for refreshing the control
    virtual void SetShaftPartState(wxScrollThumb::Shaft shaftPart,
                                   int flag,
                                   bool set = true) = 0;

    // called when the user starts dragging the thumb
    virtual void OnThumbDragStart(int pos) = 0;

    // called while the user drags the thumb
    virtual void OnThumbDrag(int pos) = 0;

    // called when the user stops dragging the thumb
    virtual void OnThumbDragEnd(int pos) = 0;

    // called before starting to call OnPageScroll() - gives the control the
    // possibility to remember its current state
    virtual void OnPageScrollStart() = 0;

    // called while the user keeps the mouse pressed above/below the thumb,
    // return true to continue scrollign and false to stop it (e.g. because the
    // scrollbar has reached the top/bottom)
    virtual bool OnPageScroll(int pageInc) = 0;
};

#endif // _WX_UNIV_SCRTHUMB_H_
