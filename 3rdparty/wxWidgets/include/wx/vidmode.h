///////////////////////////////////////////////////////////////////////////////
// Name:        wx/vidmode.h
// Purpose:     declares wxVideoMode class used by both wxDisplay and wxApp
// Author:      Vadim Zeitlin
// Modified by:
// Created:     27.09.2003 (extracted from wx/display.h)
// RCS-ID:      $Id: vidmode.h 27408 2004-05-23 20:53:33Z JS $
// Copyright:   (c) 2003 Vadim Zeitlin <vadim@wxwidgets.org>
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

#ifndef _WX_VMODE_H_
#define _WX_VMODE_H_

// ----------------------------------------------------------------------------
// wxVideoMode: a simple struct containing video mode parameters for a display
// ----------------------------------------------------------------------------

struct WXDLLEXPORT wxVideoMode
{
    wxVideoMode(int width = 0, int height = 0, int depth = 0, int freq = 0)
    {
        w = width;
        h = height;

        bpp = depth;

        refresh = freq;
    }

    // default copy ctor and assignment operator are ok

    bool operator==(const wxVideoMode& m) const
    {
        return w == m.w && h == m.h && bpp == m.bpp && refresh == m.refresh;
    }
    bool operator!=(const wxVideoMode& mode) const
    {
        return !operator==(mode);
    }

    // returns true if this mode matches the other one in the sense that all
    // non zero fields of the other mode have the same value in this one
    // (except for refresh which is allowed to have a greater value)
    bool Matches(const wxVideoMode& other) const
    {
        return (!other.w || w == other.w) &&
                    (!other.h || h == other.h) &&
                        (!other.bpp || bpp == other.bpp) &&
                            (!other.refresh || refresh >= other.refresh);
    }

    // trivial accessors
    int GetWidth() const { return w; }
    int GetHeight() const { return h; }
    int GetDepth() const { return bpp; }

    // returns true if the object has been initialized
    bool IsOk() const { return w && h; }


    // the screen size in pixels (e.g. 640*480), 0 means unspecified
    int w, h;

    // bits per pixel (e.g. 32), 1 is monochrome and 0 means unspecified/known
    int bpp;

    // refresh frequency in Hz, 0 means unspecified/unknown
    int refresh;
};

#endif // _WX_VMODE_H_

