/////////////////////////////////////////////////////////////////////////////
// Name:        wx/glcanvas.h
// Purpose:     wxGLCanvas base header
// Author:      Julian Smart
// Modified by:
// Created:
// Copyright:   (c) Julian Smart
// RCS-ID:      $Id: glcanvas.h 61872 2009-09-09 22:37:05Z VZ $
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_GLCANVAS_H_BASE_
#define _WX_GLCANVAS_H_BASE_

#include "wx/defs.h"

#if wxUSE_GLCANVAS

//---------------------------------------------------------------------------
// Constants for attriblist
//---------------------------------------------------------------------------

// The generic GL implementation doesn't support most of these options,
// such as stereo, auxiliary buffers, alpha channel, and accum buffer.
// Other implementations may actually support them.

enum
{
    WX_GL_RGBA=1,          /* use true color palette */
    WX_GL_BUFFER_SIZE,     /* bits for buffer if not WX_GL_RGBA */
    WX_GL_LEVEL,           /* 0 for main buffer, >0 for overlay, <0 for underlay */
    WX_GL_DOUBLEBUFFER,    /* use doublebuffer */
    WX_GL_STEREO,          /* use stereoscopic display */
    WX_GL_AUX_BUFFERS,     /* number of auxiliary buffers */
    WX_GL_MIN_RED,         /* use red buffer with most bits (> MIN_RED bits) */
    WX_GL_MIN_GREEN,       /* use green buffer with most bits (> MIN_GREEN bits) */
    WX_GL_MIN_BLUE,        /* use blue buffer with most bits (> MIN_BLUE bits) */
    WX_GL_MIN_ALPHA,       /* use alpha buffer with most bits (> MIN_ALPHA bits) */
    WX_GL_DEPTH_SIZE,      /* bits for Z-buffer (0,16,32) */
    WX_GL_STENCIL_SIZE,    /* bits for stencil buffer */
    WX_GL_MIN_ACCUM_RED,   /* use red accum buffer with most bits (> MIN_ACCUM_RED bits) */
    WX_GL_MIN_ACCUM_GREEN, /* use green buffer with most bits (> MIN_ACCUM_GREEN bits) */
    WX_GL_MIN_ACCUM_BLUE,  /* use blue buffer with most bits (> MIN_ACCUM_BLUE bits) */
    WX_GL_MIN_ACCUM_ALPHA  /* use alpha buffer with most bits (> MIN_ACCUM_ALPHA bits) */
};

#define wxGLCanvasName wxT("GLCanvas")

#if defined(__WXMSW__)
#include "wx/msw/glcanvas.h"
#elif defined(__WXMOTIF__)
#include "wx/x11/glcanvas.h"
#elif defined(__WXGTK20__)
#include "wx/gtk/glcanvas.h"
#elif defined(__WXGTK__)
#include "wx/gtk1/glcanvas.h"
#elif defined(__WXX11__)
#include "wx/x11/glcanvas.h"
#elif defined(__WXMAC__)
#include "wx/mac/glcanvas.h"
#elif defined(__WXCOCOA__)
#include "wx/cocoa/glcanvas.h"
#else
#error "wxGLCanvas not supported in this wxWidgets port"
#endif

#include "wx/app.h"
class WXDLLIMPEXP_GL wxGLApp : public wxApp
{
public:
    wxGLApp() : wxApp() { }
    virtual ~wxGLApp();

    // use this in the constructor of the user-derived wxGLApp class to
    // determine if an OpenGL rendering context with these attributes
    // is available - returns true if so, false if not.
    bool InitGLVisual(int *attribList);

private:
    DECLARE_DYNAMIC_CLASS(wxGLApp)
};

#endif
    // wxUSE_GLCANVAS
#endif
    // _WX_GLCANVAS_H_BASE_
