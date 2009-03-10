/////////////////////////////////////////////////////////////////////////////
// Name:        wx/msw/glcanvas.h
// Purpose:     wxGLCanvas, for using OpenGL with wxWidgets under Windows
// Author:      Julian Smart
// Modified by:
// Created:     04/01/98
// RCS-ID:      $Id: glcanvas.h 53135 2008-04-12 02:31:04Z VZ $
// Copyright:   (c) Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_GLCANVAS_H_
#define _WX_GLCANVAS_H_

#include "wx/palette.h"
#include "wx/scrolwin.h"

#include "wx/msw/wrapwin.h"

#include <GL/gl.h>

class WXDLLIMPEXP_FWD_GL wxGLCanvas;     /* forward reference */

class WXDLLIMPEXP_GL wxGLContext: public wxObject
{
public:
    wxGLContext(wxGLCanvas *win, const wxGLContext* other=NULL /* for sharing display lists */ );
    virtual ~wxGLContext();

    void SetCurrent(const wxGLCanvas& win) const;
    inline HGLRC GetGLRC() const { return m_glContext; }

protected:
    HGLRC m_glContext;

private:
    DECLARE_CLASS(wxGLContext)
};

class WXDLLIMPEXP_GL wxGLCanvas: public wxWindow
{
public:
    // This ctor is identical to the next, except for the fact that it
    // doesn't create an implicit wxGLContext.
    // The attribList parameter has been moved to avoid overload clashes.
    wxGLCanvas(wxWindow *parent, wxWindowID id = wxID_ANY,
        int* attribList = 0,
        const wxPoint& pos = wxDefaultPosition,
        const wxSize& size = wxDefaultSize, long style = 0,
        const wxString& name = wxGLCanvasName,
        const wxPalette& palette = wxNullPalette);

    wxGLCanvas(wxWindow *parent, wxWindowID id = wxID_ANY,
        const wxPoint& pos = wxDefaultPosition,
        const wxSize& size = wxDefaultSize, long style = 0,
        const wxString& name = wxGLCanvasName, int *attribList = 0,
        const wxPalette& palette = wxNullPalette);

    wxGLCanvas(wxWindow *parent,
        const wxGLContext *shared,
        wxWindowID id = wxID_ANY,
        const wxPoint& pos = wxDefaultPosition,
        const wxSize& size = wxDefaultSize,
        long style = 0,
        const wxString& name = wxGLCanvasName,
        int *attribList = (int *) NULL,
        const wxPalette& palette = wxNullPalette);

    wxGLCanvas(wxWindow *parent,
        const wxGLCanvas *shared,
        wxWindowID id = wxID_ANY,
        const wxPoint& pos = wxDefaultPosition,
        const wxSize& size = wxDefaultSize,
        long style = 0,
        const wxString& name = wxGLCanvasName,
        int *attribList = 0,
        const wxPalette& palette = wxNullPalette);

    virtual ~wxGLCanvas();

    // Replaces wxWindow::Create functionality, since
    // we need to use a different window class
    bool Create(wxWindow *parent, wxWindowID id,
        const wxPoint& pos, const wxSize& size,
        long style, const wxString& name);

    void SetCurrent(const wxGLContext& RC) const;
    void SetCurrent();

#ifdef __WXUNIVERSAL__
    virtual bool SetCurrent(bool doit) { return wxWindow::SetCurrent(doit); };
#endif

    void SetColour(const wxChar *colour);

    void SwapBuffers();

    void OnSize(wxSizeEvent& event);

    void OnQueryNewPalette(wxQueryNewPaletteEvent& event);

    void OnPaletteChanged(wxPaletteChangedEvent& event);

    inline wxGLContext* GetContext() const { return m_glContext; }

    inline WXHDC GetHDC() const { return m_hDC; }

    void SetupPixelFormat(int *attribList = (int *) NULL);

    void SetupPalette(const wxPalette& palette);

    wxPalette CreateDefaultPalette();

    inline wxPalette* GetPalette() const { return (wxPalette *) &m_palette; }

protected:
    wxGLContext*   m_glContext;  // this is typedef-ed ptr, in fact
    wxPalette      m_palette;
    WXHDC          m_hDC;

private:
    DECLARE_EVENT_TABLE()
    DECLARE_CLASS(wxGLCanvas)
};

#endif
    // _WX_GLCANVAS_H_

