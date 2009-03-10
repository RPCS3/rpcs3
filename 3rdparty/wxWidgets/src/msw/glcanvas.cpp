/////////////////////////////////////////////////////////////////////////////
// Name:        src/msw/glcanvas.cpp
// Purpose:     wxGLCanvas, for using OpenGL with wxWidgets under MS Windows
// Author:      Julian Smart
// Modified by:
// Created:     04/01/98
// RCS-ID:      $Id: glcanvas.cpp 41031 2006-09-06 13:31:20Z RR $
// Copyright:   (c) Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#include "wx/wxprec.h"

#if defined(__BORLANDC__)
    #pragma hdrstop
#endif

#if wxUSE_GLCANVAS

#ifndef WX_PRECOMP
    #include "wx/frame.h"
    #include "wx/settings.h"
    #include "wx/intl.h"
    #include "wx/log.h"
    #include "wx/app.h"
    #include "wx/module.h"
#endif

#include "wx/msw/private.h"

// DLL options compatibility check:
#include "wx/build.h"
WX_CHECK_BUILD_OPTIONS("wxGL")

#include "wx/glcanvas.h"

#if GL_EXT_vertex_array
    #define WXUNUSED_WITHOUT_GL_EXT_vertex_array(name) name
#else
    #define WXUNUSED_WITHOUT_GL_EXT_vertex_array(name) WXUNUSED(name)
#endif

/*
  The following two compiler directives are specific to the Microsoft Visual
  C++ family of compilers

  Fundementally what they do is instruct the linker to use these two libraries
  for the resolution of symbols. In essence, this is the equivalent of adding
  these two libraries to either the Makefile or project file.

  This is NOT a recommended technique, and certainly is unlikely to be used
  anywhere else in wxWidgets given it is so specific to not only wxMSW, but
  also the VC compiler. However, in the case of opengl support, it's an
  applicable technique as opengl is optional in setup.h This code (wrapped by
  wxUSE_GLCANVAS), now allows opengl support to be added purely by modifying
  setup.h rather than by having to modify either the project or DSP fle.

  See MSDN for further information on the exact usage of these commands.
*/
#ifdef _MSC_VER
#  pragma comment( lib, "opengl32" )
#  pragma comment( lib, "glu32" )
#endif


static const wxChar *wxGLCanvasClassName = wxT("wxGLCanvasClass");
static const wxChar *wxGLCanvasClassNameNoRedraw = wxT("wxGLCanvasClassNR");

LRESULT WXDLLEXPORT APIENTRY _EXPORT wxWndProc(HWND hWnd, UINT message,
                                   WPARAM wParam, LPARAM lParam);

// ----------------------------------------------------------------------------
// wxGLModule is responsible for unregistering wxGLCanvasClass Windows class
// ----------------------------------------------------------------------------

class wxGLModule : public wxModule
{
public:
    bool OnInit() { return true; }
    void OnExit() { UnregisterClasses(); }

    // register the GL classes if not done yet, return true if ok, false if
    // registration failed
    static bool RegisterClasses();

    // unregister the classes, done automatically on program termination
    static void UnregisterClasses();

private:
    // wxGLCanvas is only used from the main thread so this is MT-ok
    static bool ms_registeredGLClasses;

    DECLARE_DYNAMIC_CLASS(wxGLModule)
};

IMPLEMENT_DYNAMIC_CLASS(wxGLModule, wxModule)

bool wxGLModule::ms_registeredGLClasses = false;

/* static */
bool wxGLModule::RegisterClasses()
{
    if (ms_registeredGLClasses)
        return true;

    // We have to register a special window class because we need the CS_OWNDC
    // style for GLCanvas.

  /*
  From Angel Popov <jumpo@bitex.com>

  Here are two snips from a dicussion in the OpenGL Gamedev list that explains
  how this problem can be fixed:

  "There are 5 common DCs available in Win95. These are aquired when you call
  GetDC or GetDCEx from a window that does _not_ have the OWNDC flag.
  OWNDC flagged windows do not get their DC from the common DC pool, the issue
  is they require 800 bytes each from the limited 64Kb local heap for GDI."

  "The deal is, if you hold onto one of the 5 shared DC's too long (as GL apps
  do), Win95 will actually "steal" it from you.  MakeCurrent fails,
  apparently, because Windows re-assigns the HDC to a different window.  The
  only way to prevent this, the only reliable means, is to set CS_OWNDC."
  */

    WNDCLASS wndclass;

    // the fields which are common to all classes
    wndclass.lpfnWndProc   = (WNDPROC)wxWndProc;
    wndclass.cbClsExtra    = 0;
    wndclass.cbWndExtra    = sizeof( DWORD ); // VZ: what is this DWORD used for?
    wndclass.hInstance     = wxhInstance;
    wndclass.hIcon         = (HICON) NULL;
    wndclass.hCursor       = ::LoadCursor((HINSTANCE)NULL, IDC_ARROW);
    wndclass.lpszMenuName  = NULL;

    // Register the GLCanvas class name
    wndclass.hbrBackground = (HBRUSH)NULL;
    wndclass.lpszClassName = wxGLCanvasClassName;
    wndclass.style         = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS | CS_OWNDC;

    if ( !::RegisterClass(&wndclass) )
    {
        wxLogLastError(wxT("RegisterClass(wxGLCanvasClass)"));
        return false;
    }

    // Register the GLCanvas class name for windows which don't do full repaint
    // on resize
    wndclass.lpszClassName = wxGLCanvasClassNameNoRedraw;
    wndclass.style        &= ~(CS_HREDRAW | CS_VREDRAW);

    if ( !::RegisterClass(&wndclass) )
    {
        wxLogLastError(wxT("RegisterClass(wxGLCanvasClassNameNoRedraw)"));

        ::UnregisterClass(wxGLCanvasClassName, wxhInstance);

        return false;
    }

    ms_registeredGLClasses = true;

    return true;
}

/* static */
void wxGLModule::UnregisterClasses()
{
    // we need to unregister the classes in case we're in a DLL which is
    // unloaded and then loaded again because if we don't, the registration is
    // going to fail in wxGLCanvas::Create() the next time we're loaded
    if ( ms_registeredGLClasses )
    {
        ::UnregisterClass(wxGLCanvasClassName, wxhInstance);
        ::UnregisterClass(wxGLCanvasClassNameNoRedraw, wxhInstance);

        ms_registeredGLClasses = false;
    }
}

/*
 * GLContext implementation
 */

IMPLEMENT_CLASS(wxGLContext, wxObject)

wxGLContext::wxGLContext(wxGLCanvas* win, const wxGLContext* other /* for sharing display lists */)
{
  m_glContext = wglCreateContext((HDC) win->GetHDC());
  wxCHECK_RET( m_glContext, wxT("Couldn't create OpenGL context") );

  if( other != 0 )
    wglShareLists( other->m_glContext, m_glContext );
}

wxGLContext::~wxGLContext()
{
    // If this context happens to be the current context, wglDeleteContext() makes it un-current first.
    wglDeleteContext(m_glContext);
}

void wxGLContext::SetCurrent(const wxGLCanvas& win) const
{
    wglMakeCurrent((HDC) win.GetHDC(), m_glContext);
}


/*
 * wxGLCanvas implementation
 */

IMPLEMENT_CLASS(wxGLCanvas, wxWindow)

BEGIN_EVENT_TABLE(wxGLCanvas, wxWindow)
    EVT_SIZE(wxGLCanvas::OnSize)
    EVT_PALETTE_CHANGED(wxGLCanvas::OnPaletteChanged)
    EVT_QUERY_NEW_PALETTE(wxGLCanvas::OnQueryNewPalette)
END_EVENT_TABLE()

wxGLCanvas::wxGLCanvas(wxWindow *parent, wxWindowID id, int *attribList,
    const wxPoint& pos, const wxSize& size, long style,
    const wxString& name, const wxPalette& palette) : wxWindow()
{
    m_glContext = NULL;

    if (Create(parent, id, pos, size, style, name))
    {
        SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_3DFACE));
    }

    m_hDC = (WXHDC) ::GetDC((HWND) GetHWND());

    SetupPixelFormat(attribList);
    SetupPalette(palette);

    // This ctor does *not* create an instance of wxGLContext,
    // m_glContext intentionally remains NULL.
}

wxGLCanvas::wxGLCanvas(wxWindow *parent, wxWindowID id,
    const wxPoint& pos, const wxSize& size, long style, const wxString& name,
    int *attribList, const wxPalette& palette) : wxWindow()
{
  m_glContext = (wxGLContext*) NULL;

  bool ret = Create(parent, id, pos, size, style, name);

  if ( ret )
  {
    SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_3DFACE));
  }

  m_hDC = (WXHDC) ::GetDC((HWND) GetHWND());

  SetupPixelFormat(attribList);
  SetupPalette(palette);

  m_glContext = new wxGLContext(this);
}

wxGLCanvas::wxGLCanvas( wxWindow *parent,
              const wxGLContext *shared, wxWindowID id,
              const wxPoint& pos, const wxSize& size, long style, const wxString& name,
              int *attribList, const wxPalette& palette )
  : wxWindow()
{
  m_glContext = (wxGLContext*) NULL;

  bool ret = Create(parent, id, pos, size, style, name);

  if ( ret )
  {
    SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_3DFACE));
  }

  m_hDC = (WXHDC) ::GetDC((HWND) GetHWND());

  SetupPixelFormat(attribList);
  SetupPalette(palette);

  m_glContext = new wxGLContext(this, shared);
}

// Not very useful for wxMSW, but this is to be wxGTK compliant

wxGLCanvas::wxGLCanvas( wxWindow *parent, const wxGLCanvas *shared, wxWindowID id,
                        const wxPoint& pos, const wxSize& size, long style, const wxString& name,
                        int *attribList, const wxPalette& palette ):
  wxWindow()
{
  m_glContext = (wxGLContext*) NULL;

  bool ret = Create(parent, id, pos, size, style, name);

  if ( ret )
  {
    SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_3DFACE));
  }

  m_hDC = (WXHDC) ::GetDC((HWND) GetHWND());

  SetupPixelFormat(attribList);
  SetupPalette(palette);

  wxGLContext *sharedContext=0;
  if (shared) sharedContext=shared->GetContext();
  m_glContext = new wxGLContext(this, sharedContext);
}

wxGLCanvas::~wxGLCanvas()
{
  delete m_glContext;

  ::ReleaseDC((HWND) GetHWND(), (HDC) m_hDC);
}

// Replaces wxWindow::Create functionality, since we need to use a different
// window class
bool wxGLCanvas::Create(wxWindow *parent,
                        wxWindowID id,
                        const wxPoint& pos,
                        const wxSize& size,
                        long style,
                        const wxString& name)
{
    wxCHECK_MSG( parent, false, wxT("can't create wxWindow without parent") );

    if ( !wxGLModule::RegisterClasses() )
    {
        wxLogError(_("Failed to register OpenGL window class."));

        return false;
    }

    if ( !CreateBase(parent, id, pos, size, style, wxDefaultValidator, name) )
        return false;

    parent->AddChild(this);

    DWORD msflags = 0;

    /*
       A general rule with OpenGL and Win32 is that any window that will have a
       HGLRC built for it must have two flags:  WS_CLIPCHILDREN & WS_CLIPSIBLINGS.
       You can find references about this within the knowledge base and most OpenGL
       books that contain the wgl function descriptions.
     */

    WXDWORD exStyle = 0;
    msflags |= WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
    msflags |= MSWGetStyle(style, & exStyle) ;

    return MSWCreate(wxGLCanvasClassName, NULL, pos, size, msflags, exStyle);
}

static void AdjustPFDForAttributes(PIXELFORMATDESCRIPTOR& pfd, int *attribList)
{
  if (attribList) {
    pfd.dwFlags &= ~PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_COLORINDEX;
    pfd.cColorBits = 0;
    int arg=0;

    while( (attribList[arg]!=0) )
    {
      switch( attribList[arg++] )
      {
        case WX_GL_RGBA:
          pfd.iPixelType = PFD_TYPE_RGBA;
          break;
        case WX_GL_BUFFER_SIZE:
          pfd.cColorBits = (BYTE)attribList[arg++];
          break;
        case WX_GL_LEVEL:
          // this member looks like it may be obsolete
          if (attribList[arg] > 0) {
            pfd.iLayerType = (BYTE)PFD_OVERLAY_PLANE;
          } else if (attribList[arg] < 0) {
            pfd.iLayerType = (BYTE)PFD_UNDERLAY_PLANE;
          } else {
            pfd.iLayerType = (BYTE)PFD_MAIN_PLANE;
          }
          arg++;
          break;
        case WX_GL_DOUBLEBUFFER:
          pfd.dwFlags |= PFD_DOUBLEBUFFER;
          break;
        case WX_GL_STEREO:
          pfd.dwFlags |= PFD_STEREO;
          break;
        case WX_GL_AUX_BUFFERS:
          pfd.cAuxBuffers = (BYTE)attribList[arg++];
          break;
        case WX_GL_MIN_RED:
          pfd.cColorBits = (BYTE)(pfd.cColorBits + (pfd.cRedBits = (BYTE)attribList[arg++]));
          break;
        case WX_GL_MIN_GREEN:
          pfd.cColorBits = (BYTE)(pfd.cColorBits + (pfd.cGreenBits = (BYTE)attribList[arg++]));
          break;
        case WX_GL_MIN_BLUE:
          pfd.cColorBits = (BYTE)(pfd.cColorBits + (pfd.cBlueBits = (BYTE)attribList[arg++]));
          break;
        case WX_GL_MIN_ALPHA:
          // doesn't count in cColorBits
          pfd.cAlphaBits = (BYTE)attribList[arg++];
          break;
        case WX_GL_DEPTH_SIZE:
          pfd.cDepthBits = (BYTE)attribList[arg++];
          break;
        case WX_GL_STENCIL_SIZE:
          pfd.cStencilBits = (BYTE)attribList[arg++];
          break;
        case WX_GL_MIN_ACCUM_RED:
          pfd.cAccumBits = (BYTE)(pfd.cAccumBits + (pfd.cAccumRedBits = (BYTE)attribList[arg++]));
          break;
        case WX_GL_MIN_ACCUM_GREEN:
          pfd.cAccumBits = (BYTE)(pfd.cAccumBits + (pfd.cAccumGreenBits = (BYTE)attribList[arg++]));
          break;
        case WX_GL_MIN_ACCUM_BLUE:
          pfd.cAccumBits = (BYTE)(pfd.cAccumBits + (pfd.cAccumBlueBits = (BYTE)attribList[arg++]));
          break;
        case WX_GL_MIN_ACCUM_ALPHA:
          pfd.cAccumBits = (BYTE)(pfd.cAccumBits + (pfd.cAccumAlphaBits = (BYTE)attribList[arg++]));
          break;
        default:
          break;
      }
    }
  }
}

void wxGLCanvas::SetupPixelFormat(int *attribList) // (HDC hDC)
{
  PIXELFORMATDESCRIPTOR pfd = {
        sizeof(PIXELFORMATDESCRIPTOR),    /* size */
        1,                /* version */
        PFD_SUPPORT_OPENGL |
        PFD_DRAW_TO_WINDOW |
        PFD_DOUBLEBUFFER,        /* support double-buffering */
        PFD_TYPE_RGBA,            /* color type */
        16,                /* preferred color depth */
        0, 0, 0, 0, 0, 0,        /* color bits (ignored) */
        0,                /* no alpha buffer */
        0,                /* alpha bits (ignored) */
        0,                /* no accumulation buffer */
        0, 0, 0, 0,            /* accum bits (ignored) */
        16,                /* depth buffer */
        0,                /* no stencil buffer */
        0,                /* no auxiliary buffers */
        PFD_MAIN_PLANE,            /* main layer */
        0,                /* reserved */
        0, 0, 0,            /* no layer, visible, damage masks */
    };

  AdjustPFDForAttributes(pfd, attribList);

  int pixelFormat = ChoosePixelFormat((HDC) m_hDC, &pfd);
  if (pixelFormat == 0) {
    wxLogLastError(_T("ChoosePixelFormat"));
  }
  else {
    if ( !::SetPixelFormat((HDC) m_hDC, pixelFormat, &pfd) ) {
      wxLogLastError(_T("SetPixelFormat"));
    }
  }
}

void wxGLCanvas::SetupPalette(const wxPalette& palette)
{
    int pixelFormat = GetPixelFormat((HDC) m_hDC);
    PIXELFORMATDESCRIPTOR pfd;

    DescribePixelFormat((HDC) m_hDC, pixelFormat, sizeof(PIXELFORMATDESCRIPTOR), &pfd);

    if (pfd.dwFlags & PFD_NEED_PALETTE)
    {
    }
    else
    {
      return;
    }

    m_palette = palette;

    if ( !m_palette.Ok() )
    {
        m_palette = CreateDefaultPalette();
    }

    if (m_palette.Ok())
    {
        ::SelectPalette((HDC) m_hDC, (HPALETTE) m_palette.GetHPALETTE(), FALSE);
        ::RealizePalette((HDC) m_hDC);
    }
}

wxPalette wxGLCanvas::CreateDefaultPalette()
{
    PIXELFORMATDESCRIPTOR pfd;
    int paletteSize;
    int pixelFormat = GetPixelFormat((HDC) m_hDC);

    DescribePixelFormat((HDC) m_hDC, pixelFormat, sizeof(PIXELFORMATDESCRIPTOR), &pfd);

    paletteSize = 1 << pfd.cColorBits;

    LOGPALETTE* pPal =
     (LOGPALETTE*) malloc(sizeof(LOGPALETTE) + paletteSize * sizeof(PALETTEENTRY));
    pPal->palVersion = 0x300;
    pPal->palNumEntries = (WORD)paletteSize;

    /* build a simple RGB color palette */
    {
    int redMask = (1 << pfd.cRedBits) - 1;
    int greenMask = (1 << pfd.cGreenBits) - 1;
    int blueMask = (1 << pfd.cBlueBits) - 1;
    int i;

    for (i=0; i<paletteSize; ++i) {
        pPal->palPalEntry[i].peRed =
            (BYTE)((((i >> pfd.cRedShift) & redMask) * 255) / redMask);
        pPal->palPalEntry[i].peGreen =
            (BYTE)((((i >> pfd.cGreenShift) & greenMask) * 255) / greenMask);
        pPal->palPalEntry[i].peBlue =
            (BYTE)((((i >> pfd.cBlueShift) & blueMask) * 255) / blueMask);
        pPal->palPalEntry[i].peFlags = 0;
    }
    }

    HPALETTE hPalette = CreatePalette(pPal);
    free(pPal);

    wxPalette palette;
    palette.SetHPALETTE((WXHPALETTE) hPalette);

    return palette;
}

void wxGLCanvas::SwapBuffers()
{
    ::SwapBuffers((HDC) m_hDC);
}

void wxGLCanvas::OnSize(wxSizeEvent& WXUNUSED(event))
{
}

void wxGLCanvas::SetCurrent(const wxGLContext& RC) const
{
    // although on MSW it works even if the window is still hidden, it doesn't
  	// under wxGTK and documentation mentions that SetCurrent() can only be
  	// called for a shown window, so check it
  	wxASSERT_MSG( GetParent()->IsShown(), _T("can't make hidden GL canvas current") );

    RC.SetCurrent(*this);
}

void wxGLCanvas::SetCurrent()
{
  // although on MSW it works even if the window is still hidden, it doesn't
  // under wxGTK and documentation mentions that SetCurrent() can only be
  // called for a shown window, so check it
  wxASSERT_MSG( GetParent()->IsShown(),
                    _T("can't make hidden GL canvas current") );

  if (m_glContext)
  {
    m_glContext->SetCurrent(*this);
  }
}

void wxGLCanvas::SetColour(const wxChar *colour)
{
    wxColour col = wxTheColourDatabase->Find(colour);

    if (col.Ok())
    {
        float r = (float)(col.Red()/256.0);
        float g = (float)(col.Green()/256.0);
        float b = (float)(col.Blue()/256.0);
        glColor3f( r, g, b);
    }
}

// TODO: Have to have this called by parent frame (?)
// So we need wxFrame to call OnQueryNewPalette for all children...
void wxGLCanvas::OnQueryNewPalette(wxQueryNewPaletteEvent& event)
{
  /* realize palette if this is the current window */
  if ( GetPalette()->Ok() ) {
    ::UnrealizeObject((HPALETTE) GetPalette()->GetHPALETTE());
    ::SelectPalette((HDC) GetHDC(), (HPALETTE) GetPalette()->GetHPALETTE(), FALSE);
    ::RealizePalette((HDC) GetHDC());
    Refresh();
    event.SetPaletteRealized(true);
  }
  else
    event.SetPaletteRealized(false);
}

// I think this doesn't have to be propagated to child windows.
void wxGLCanvas::OnPaletteChanged(wxPaletteChangedEvent& event)
{
  /* realize palette if this is *not* the current window */
  if ( GetPalette() &&
       GetPalette()->Ok() && (this != event.GetChangedWindow()) )
  {
    ::UnrealizeObject((HPALETTE) GetPalette()->GetHPALETTE());
    ::SelectPalette((HDC) GetHDC(), (HPALETTE) GetPalette()->GetHPALETTE(), FALSE);
    ::RealizePalette((HDC) GetHDC());
    Refresh();
  }
}


//---------------------------------------------------------------------------
// wxGLApp
//---------------------------------------------------------------------------

IMPLEMENT_CLASS(wxGLApp, wxApp)

bool wxGLApp::InitGLVisual(int *attribList)
{
  int pixelFormat;
  PIXELFORMATDESCRIPTOR pfd = {
        sizeof(PIXELFORMATDESCRIPTOR),    /* size */
        1,                /* version */
        PFD_SUPPORT_OPENGL |
        PFD_DRAW_TO_WINDOW |
        PFD_DOUBLEBUFFER,        /* support double-buffering */
        PFD_TYPE_RGBA,            /* color type */
        16,                /* preferred color depth */
        0, 0, 0, 0, 0, 0,        /* color bits (ignored) */
        0,                /* no alpha buffer */
        0,                /* alpha bits (ignored) */
        0,                /* no accumulation buffer */
        0, 0, 0, 0,            /* accum bits (ignored) */
        16,                /* depth buffer */
        0,                /* no stencil buffer */
        0,                /* no auxiliary buffers */
        PFD_MAIN_PLANE,            /* main layer */
        0,                /* reserved */
        0, 0, 0,            /* no layer, visible, damage masks */
    };

  AdjustPFDForAttributes(pfd, attribList);

  // use DC for whole (root) screen, since no windows have yet been created
  pixelFormat = ChoosePixelFormat(ScreenHDC(), &pfd);

  if (pixelFormat == 0) {
    wxLogError(_("Failed to initialize OpenGL"));
    return false;
  }

  return true;
}

wxGLApp::~wxGLApp()
{
}

#endif
    // wxUSE_GLCANVAS
