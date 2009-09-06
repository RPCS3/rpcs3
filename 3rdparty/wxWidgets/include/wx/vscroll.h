/////////////////////////////////////////////////////////////////////////////
// Name:        include/wx/vscroll.h
// Purpose:     wxVScrolledWindow: generalization of wxScrolledWindow
// Author:      Vadim Zeitlin
// Modified by:
// Created:     30.05.03
// RCS-ID:      $Id: vscroll.h 38917 2006-04-26 00:17:52Z VZ $
// Copyright:   (c) 2003 Vadim Zeitlin <vadim@wxwidgets.org>
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_VSCROLL_H_
#define _WX_VSCROLL_H_

#include "wx/panel.h"           // base class

// ----------------------------------------------------------------------------
// wxVScrolledWindow
// ----------------------------------------------------------------------------

/*
   In the name of this class, "V" may stand for "variable" because it can be
   used for scrolling lines of variable heights; "virtual" because it is not
   necessary to know the heights of all lines in advance -- only those which
   are shown on the screen need to be measured; or, even, "vertical" because
   this class only supports scrolling in one direction currently (this could
   and probably will change in the future however).

   In any case, this is a generalization of the wxScrolledWindow class which
   can be only used when all lines have the same height. It lacks some other
   wxScrolledWindow features however, notably it currently lacks support for
   horizontal scrolling; it can't scroll another window nor only a rectangle
   of the window and not its entire client area.
 */
class WXDLLEXPORT wxVScrolledWindow : public wxPanel
{
public:
    // constructors and such
    // ---------------------

    // default ctor, you must call Create() later
    wxVScrolledWindow() { Init(); }

    // normal ctor, no need to call Create() after this one
    //
    // note that wxVSCROLL is always automatically added to our style, there is
    // no need to specify it explicitly
    wxVScrolledWindow(wxWindow *parent,
                      wxWindowID id = wxID_ANY,
                      const wxPoint& pos = wxDefaultPosition,
                      const wxSize& size = wxDefaultSize,
                      long style = 0,
                      const wxString& name = wxPanelNameStr)
    {
        Init();

        (void)Create(parent, id, pos, size, style, name);
    }

    // same as the previous ctor but returns status code: true if ok
    //
    // just as with the ctor above, wxVSCROLL style is always used, there is no
    // need to specify it
    bool Create(wxWindow *parent,
                wxWindowID id = wxID_ANY,
                const wxPoint& pos = wxDefaultPosition,
                const wxSize& size = wxDefaultSize,
                long style = 0,
                const wxString& name = wxPanelNameStr)
    {
        return wxPanel::Create(parent, id, pos, size, style | wxVSCROLL, name);
    }


    // operations
    // ----------

    // set the number of lines the window contains: the derived class must
    // provide the heights for all lines with indices up to the one given here
    // in its OnGetLineHeight()
    void SetLineCount(size_t count);

    // scroll to the specified line: it will become the first visible line in
    // the window
    //
    // return true if we scrolled the window, false if nothing was done
    bool ScrollToLine(size_t line);

    // scroll by the specified number of lines/pages
    virtual bool ScrollLines(int lines);
    virtual bool ScrollPages(int pages);

    // redraw the specified line
    virtual void RefreshLine(size_t line);

    // redraw all lines in the specified range (inclusive)
    virtual void RefreshLines(size_t from, size_t to);

    // return the item at the specified (in physical coordinates) position or.

    // wxNOT_FOUND if none, i.e. if it is below the last item
    int HitTest(wxCoord x, wxCoord y) const;
    int HitTest(const wxPoint& pt) const { return HitTest(pt.x, pt.y); }

    // recalculate all our parameters and redisplay all lines
    virtual void RefreshAll();


    // accessors
    // ---------

    // get the number of lines this window contains (previously set by
    // SetLineCount())
    size_t GetLineCount() const { return m_lineMax; }

    // get the first currently visible line
    size_t GetVisibleBegin() const { return m_lineFirst; }

    // get the first currently visible line
    size_t GetVisibleEnd() const { return m_lineFirst + m_nVisible; }

    // is this line currently visible?
    bool IsVisible(size_t line) const
        { return line >= GetVisibleBegin() && line < GetVisibleEnd(); }


    // this is the same as GetVisibleBegin(), exists to match
    // GetLastVisibleLine() and for backwards compatibility only
    size_t GetFirstVisibleLine() const { return m_lineFirst; }

    // get the last currently visible line
    //
    // this function is unsafe as it returns (size_t)-1 (i.e. a huge positive
    // number) if the control is empty, use GetVisibleEnd() instead, this one
    // is kept for backwards compatibility
    size_t GetLastVisibleLine() const { return GetVisibleEnd() - 1; }

    // layout the children (including the sizer if needed)
    virtual bool Layout();

protected:
    // this function must be overridden in the derived class and it should
    // return the height of the given line in pixels
    virtual wxCoord OnGetLineHeight(size_t n) const = 0;

    // this function doesn't have to be overridden but it may be useful to do
    // it if calculating the lines heights is a relatively expensive operation
    // as it gives the user code a possibility to calculate several of them at
    // once
    //
    // OnGetLinesHint() is normally called just before OnGetLineHeight() but you
    // shouldn't rely on the latter being called for all lines in the interval
    // specified here. It is also possible that OnGetLineHeight() will be
    // called for the lines outside of this interval, so this is really just a
    // hint, not a promise.
    //
    // finally note that lineMin is inclusive, while lineMax is exclusive, as
    // usual
    virtual void OnGetLinesHint(size_t WXUNUSED(lineMin),
                                size_t WXUNUSED(lineMax)) const { }

    // when the number of lines changes, we try to estimate the total height
    // of all lines which is a rather expensive operation in terms of lines
    // access, so if the user code may estimate the average height
    // better/faster than we do, it should override this function to implement
    // its own logic
    //
    // this function should return the best guess for the total height it may
    // make
    virtual wxCoord EstimateTotalHeight() const;


    // the event handlers
    void OnSize(wxSizeEvent& event);
    void OnScroll(wxScrollWinEvent& event);
#if wxUSE_MOUSEWHEEL
    void OnMouseWheel(wxMouseEvent& event);
#endif

    // find the index of the line we need to show at the top of the window such
    // that the last (fully or partially) visible line is the given one
    size_t FindFirstFromBottom(size_t lineLast, bool fullyVisible = false);

    // get the total height of the lines between lineMin (inclusive) and
    // lineMax (exclusive)
    wxCoord GetLinesHeight(size_t lineMin, size_t lineMax) const;

    // update the thumb size shown by the scrollbar
    void UpdateScrollbar();

    // remove the scrollbar completely because we don't need it
    void RemoveScrollbar();

private:
    // common part of all ctors
    void Init();


    // the total number of (logical) lines
    size_t m_lineMax;

    // the total (estimated) height
    wxCoord m_heightTotal;

    // the first currently visible line
    size_t m_lineFirst;

    // the number of currently visible lines (including the last, possibly only
    // partly, visible one)
    size_t m_nVisible;

    // accumulated mouse wheel rotation
#if wxUSE_MOUSEWHEEL
    int m_sumWheelRotation;
#endif

    DECLARE_EVENT_TABLE()
    DECLARE_NO_COPY_CLASS(wxVScrolledWindow)
    DECLARE_ABSTRACT_CLASS(wxVScrolledWindow)
};

#endif // _WX_VSCROLL_H_

