/////////////////////////////////////////////////////////////////////////////
// Name:        wx/dcbuffer.h
// Purpose:     wxBufferedDC class
// Author:      Ron Lee <ron@debian.org>
// Modified by: Vadim Zeitlin (refactored, added bg preservation)
// Created:     16/03/02
// RCS-ID:      $Id: dcbuffer.h 61872 2009-09-09 22:37:05Z VZ $
// Copyright:   (c) Ron Lee
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_DCBUFFER_H_
#define _WX_DCBUFFER_H_

#include "wx/dcmemory.h"
#include "wx/dcclient.h"
#include "wx/window.h"

// Split platforms into two groups - those which have well-working
// double-buffering by default, and those which do not.
#if defined(__WXMAC__) || defined(__WXGTK20__) || defined(__WXDFB__)
    #define wxALWAYS_NATIVE_DOUBLE_BUFFER       1
#else
    #define wxALWAYS_NATIVE_DOUBLE_BUFFER       0
#endif


// ----------------------------------------------------------------------------
// Double buffering helper.
// ----------------------------------------------------------------------------

// Assumes the buffer bitmap covers the entire scrolled window,
// and prepares the window DC accordingly
#define wxBUFFER_VIRTUAL_AREA       0x01

// Assumes the buffer bitmap only covers the client area;
// does not prepare the window DC
#define wxBUFFER_CLIENT_AREA        0x02

class WXDLLEXPORT wxBufferedDC : public wxMemoryDC
{
public:
    // Default ctor, must subsequently call Init for two stage construction.
    wxBufferedDC()
        : m_dc(NULL),
          m_buffer(NULL),
          m_style(0)
    {
    }

    // Construct a wxBufferedDC using a user supplied buffer.
    wxBufferedDC(wxDC *dc,
                 wxBitmap& buffer = wxNullBitmap,
                 int style = wxBUFFER_CLIENT_AREA)
        : m_dc(NULL), m_buffer(NULL)
    {
        Init(dc, buffer, style);
    }

    // Construct a wxBufferedDC with an internal buffer of 'area'
    // (where area is usually something like the size of the window
    // being buffered)
    wxBufferedDC(wxDC *dc, const wxSize& area, int style = wxBUFFER_CLIENT_AREA)
        : m_dc(NULL), m_buffer(NULL)
    {
        Init(dc, area, style);
    }

    // The usually desired  action in the dtor is to blit the buffer.
    virtual ~wxBufferedDC()
    {
        if ( m_dc )
            UnMask();
    }

    // These reimplement the actions of the ctors for two stage creation
    void Init(wxDC *dc,
              wxBitmap& buffer = wxNullBitmap,
              int style = wxBUFFER_CLIENT_AREA)
    {
        InitCommon(dc, style);

        m_buffer = &buffer;

        UseBuffer();
    }

    void Init(wxDC *dc, const wxSize &area, int style = wxBUFFER_CLIENT_AREA)
    {
        InitCommon(dc, style);

        UseBuffer(area.x, area.y);
    }

    // Blits the buffer to the dc, and detaches the dc from the buffer (so it
    // can be effectively used once only).
    //
    // Usually called in the dtor or by the dtor of derived classes if the
    // BufferedDC must blit before the derived class (which may own the dc it's
    // blitting to) is destroyed.
    void UnMask()
    {
        wxCHECK_RET( m_dc, wxT("no underlying wxDC?") );
        wxASSERT_MSG( m_buffer && m_buffer->IsOk(), wxT("invalid backing store") );

        wxCoord x = 0,
                y = 0;

        if ( m_style & wxBUFFER_CLIENT_AREA )
            GetDeviceOrigin(&x, &y);

        m_dc->Blit(0, 0, m_buffer->GetWidth(), m_buffer->GetHeight(),
                   this, -x, -y );
        m_dc = NULL;
    }

    // Set and get the style
    void SetStyle(int style) { m_style = style; }
    int GetStyle() const { return m_style; }

private:
    // common part of Init()s
    void InitCommon(wxDC *dc, int style)
    {
        wxASSERT_MSG( !m_dc, wxT("wxBufferedDC already initialised") );

        m_dc = dc;
        m_style = style;

        // inherit the same layout direction as the original DC
        if (dc && dc->IsOk())
            SetLayoutDirection(dc->GetLayoutDirection());
    }

    // check that the bitmap is valid and use it
    void UseBuffer(wxCoord w = -1, wxCoord h = -1);

    // the underlying DC to which we copy everything drawn on this one in
    // UnMask()
    //
    // NB: Without the existence of a wxNullDC, this must be a pointer, else it
    //     could probably be a reference.
    wxDC *m_dc;

    // the buffer (selected in this DC), initially invalid
    wxBitmap *m_buffer;

    // the buffering style
    int m_style;

    DECLARE_DYNAMIC_CLASS(wxBufferedDC)
    DECLARE_NO_COPY_CLASS(wxBufferedDC)
};


// ----------------------------------------------------------------------------
// Double buffered PaintDC.
// ----------------------------------------------------------------------------

// Creates a double buffered wxPaintDC, optionally allowing the
// user to specify their own buffer to use.
class WXDLLEXPORT wxBufferedPaintDC : public wxBufferedDC
{
public:
    // If no bitmap is supplied by the user, a temporary one will be created.
    wxBufferedPaintDC(wxWindow *window, wxBitmap& buffer, int style = wxBUFFER_CLIENT_AREA)
        : m_paintdc(window)
    {
        // If we're buffering the virtual window, scale the paint DC as well
        if (style & wxBUFFER_VIRTUAL_AREA)
            window->PrepareDC( m_paintdc );

        if( buffer.IsOk() )
            Init(&m_paintdc, buffer, style);
        else
            Init(&m_paintdc, GetBufferedSize(window, style), style);
    }

    // If no bitmap is supplied by the user, a temporary one will be created.
    wxBufferedPaintDC(wxWindow *window, int style = wxBUFFER_CLIENT_AREA)
        : m_paintdc(window)
    {
        // If we're using the virtual window, scale the paint DC as well
        if (style & wxBUFFER_VIRTUAL_AREA)
            window->PrepareDC( m_paintdc );

        Init(&m_paintdc, GetBufferedSize(window, style), style);
    }

    // default copy ctor ok.

    virtual ~wxBufferedPaintDC()
    {
        // We must UnMask here, else by the time the base class
        // does it, the PaintDC will have already been destroyed.
        UnMask();
    }

protected:
    // return the size needed by the buffer: this depends on whether we're
    // buffering just the currently shown part or the total (scrolled) window
    static wxSize GetBufferedSize(wxWindow *window, int style)
    {
        return style & wxBUFFER_VIRTUAL_AREA ? window->GetVirtualSize()
                                             : window->GetClientSize();
    }

private:
    wxPaintDC m_paintdc;

    DECLARE_ABSTRACT_CLASS(wxBufferedPaintDC)
    DECLARE_NO_COPY_CLASS(wxBufferedPaintDC)
};



//
// wxAutoBufferedPaintDC is a wxPaintDC in toolkits which have double-
// buffering by default. Otherwise it is a wxBufferedPaintDC. Thus,
// you can only expect it work with a simple constructor that
// accepts single wxWindow* argument.
//
#if wxALWAYS_NATIVE_DOUBLE_BUFFER
    #define wxAutoBufferedPaintDCBase           wxPaintDC
#else
    #define wxAutoBufferedPaintDCBase           wxBufferedPaintDC
#endif


#ifdef __WXDEBUG__

class wxAutoBufferedPaintDC : public wxAutoBufferedPaintDCBase
{
public:

    wxAutoBufferedPaintDC(wxWindow* win)
        : wxAutoBufferedPaintDCBase(win)
    {
        TestWinStyle(win);
    }

    virtual ~wxAutoBufferedPaintDC() { }

private:

    void TestWinStyle(wxWindow* win)
    {
        // Help the user to get the double-buffering working properly.
        wxASSERT_MSG( win->GetBackgroundStyle() == wxBG_STYLE_CUSTOM,
                      wxT("In constructor, you need to call SetBackgroundStyle(wxBG_STYLE_CUSTOM), ")
                      wxT("and also, if needed, paint the background manually in the paint event handler."));
    }

    DECLARE_NO_COPY_CLASS(wxAutoBufferedPaintDC)
};

#else // !__WXDEBUG__

// In release builds, just use typedef
typedef wxAutoBufferedPaintDCBase wxAutoBufferedPaintDC;

#endif


// Check if the window is natively double buffered and will return a wxPaintDC
// if it is, a wxBufferedPaintDC otherwise.  It is the caller's responsibility
// to delete the wxDC pointer when finished with it.
inline wxDC* wxAutoBufferedPaintDCFactory(wxWindow* window)
{
    if ( window->IsDoubleBuffered() )
        return new wxPaintDC(window);
    else
        return new wxBufferedPaintDC(window);
}

#endif  // _WX_DCBUFFER_H_
