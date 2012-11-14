/////////////////////////////////////////////////////////////////////////////
// Name:        src/unix/utilsx11.cpp
// Purpose:     Miscellaneous X11 functions
// Author:      Mattia Barbon, Vaclav Slavik, Robert Roebling
// Modified by:
// Created:     25.03.02
// RCS-ID:      $Id: utilsx11.cpp 44895 2007-03-18 17:49:06Z VZ $
// Copyright:   (c) wxWidgets team
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#if defined(__WXX11__) || defined(__WXGTK__) || defined(__WXMOTIF__)

// for compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#include "wx/unix/utilsx11.h"

#ifndef WX_PRECOMP
    #include "wx/log.h"
    #include "wx/app.h"
    #include "wx/icon.h"
    #include "wx/image.h"
#endif

#include "wx/iconbndl.h"

#ifdef __VMS
#pragma message disable nosimpint
#endif
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#ifdef __VMS
#pragma message enable nosimpint
#endif

#ifdef __WXGTK__
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#endif

// Various X11 Atoms used in this file:
static Atom _NET_WM_ICON = 0;
static Atom _NET_WM_STATE = 0;
static Atom _NET_WM_STATE_FULLSCREEN = 0;
static Atom _NET_WM_STATE_STAYS_ON_TOP = 0;
static Atom _NET_WM_WINDOW_TYPE = 0;
static Atom _NET_WM_WINDOW_TYPE_NORMAL = 0;
static Atom _KDE_NET_WM_WINDOW_TYPE_OVERRIDE = 0;
static Atom _WIN_LAYER = 0;
static Atom KWIN_RUNNING = 0;
#ifndef __WXGTK20__
static Atom _NET_SUPPORTING_WM_CHECK = 0;
static Atom _NET_SUPPORTED = 0;
#endif

#define wxMAKE_ATOM(name, display) \
    if (name == 0) name = XInternAtom((display), #name, False)


// X11 Window is an int type, so use the macro to suppress warnings when
// converting to it
#define WindowCast(w) (Window)(wxPtrToUInt(w))

// Is the window mapped?
static bool IsMapped(Display *display, Window window)
{
    XWindowAttributes attr;
    XGetWindowAttributes(display, window, &attr);
    return (attr.map_state != IsUnmapped);
}



// Suspends X11 errors. Used when we expect errors but they are not fatal
// for us.
extern "C"
{
    typedef int (*wxX11ErrorHandler)(Display *, XErrorEvent *);

    static int wxX11ErrorsSuspender_handler(Display*, XErrorEvent*) { return 0; }
}

class wxX11ErrorsSuspender
{
public:
    wxX11ErrorsSuspender(Display *d) : m_display(d)
    {
        m_old = XSetErrorHandler(wxX11ErrorsSuspender_handler);
    }
    ~wxX11ErrorsSuspender()
    {
        XFlush(m_display);
        XSetErrorHandler(m_old);
    }

private:
    Display *m_display;
    wxX11ErrorHandler m_old;
};



// ----------------------------------------------------------------------------
// Setting icons for window manager:
// ----------------------------------------------------------------------------

void wxSetIconsX11( WXDisplay* display, WXWindow window,
                    const wxIconBundle& ib )
{
#if !wxUSE_NANOX
    size_t size = 0;
    size_t i, max = ib.m_icons.GetCount();

    for( i = 0; i < max; ++i )
        if( ib.m_icons[i].Ok() )
            size += 2 + ib.m_icons[i].GetWidth() * ib.m_icons[i].GetHeight();

    wxMAKE_ATOM(_NET_WM_ICON, (Display*)display);

    if( size > 0 )
    {
//       The code below is correct for 64-bit machines also.
//       wxUint32* data = new wxUint32[size];
//       wxUint32* ptr = data;
        unsigned long* data = new unsigned long[size];
        unsigned long* ptr = data;

        for( i = 0; i < max; ++i )
        {
            const wxImage image = ib.m_icons[i].ConvertToImage();
            int width = image.GetWidth(), height = image.GetHeight();
            unsigned char* imageData = image.GetData();
            unsigned char* imageDataEnd = imageData + ( width * height * 3 );
            bool hasMask = image.HasMask();
            unsigned char rMask, gMask, bMask;
            unsigned char r, g, b, a;

            if( hasMask )
            {
                rMask = image.GetMaskRed();
                gMask = image.GetMaskGreen();
                bMask = image.GetMaskBlue();
            }
            else // no mask, but still init the variables to avoid warnings
            {
                rMask =
                gMask =
                bMask = 0;
            }

            *ptr++ = width;
            *ptr++ = height;

            while( imageData < imageDataEnd ) {
                r = imageData[0];
                g = imageData[1];
                b = imageData[2];
                if( hasMask && r == rMask && g == gMask && b == bMask )
                    a = 0;
                else
                    a = 255;

                *ptr++ = ( a << 24 ) | ( r << 16 ) | ( g << 8 ) | b;

                imageData += 3;
            }
        }

        XChangeProperty( (Display*)display,
                         WindowCast(window),
                         _NET_WM_ICON,
                         XA_CARDINAL, 32,
                         PropModeReplace,
                         (unsigned char*)data, size );
        delete[] data;
    }
    else
    {
        XDeleteProperty( (Display*)display,
                         WindowCast(window),
                         _NET_WM_ICON );
    }
#endif // !wxUSE_NANOX
}


// ----------------------------------------------------------------------------
// Fullscreen mode:
// ----------------------------------------------------------------------------

// NB: Setting fullscreen mode under X11 is a complicated matter. There was
//     no standard way of doing it until recently. ICCCM doesn't know the
//     concept of fullscreen windows and the only way to make a window
//     fullscreen is to remove decorations, resize it to cover entire screen
//     and set WIN_LAYER_ABOVE_DOCK.
//
//     This doesn't always work, though. Specifically, at least kwin from
//     KDE 3 ignores the hint. The only way to make kwin accept our request
//     is to emulate the way Qt does it. That is, unmap the window, set
//     _NET_WM_WINDOW_TYPE to _KDE_NET_WM_WINDOW_TYPE_OVERRIDE (KDE extension),
//     add _NET_WM_STATE_STAYS_ON_TOP (ditto) to _NET_WM_STATE and map
//     the window again.
//
//     Version 1.2 of Window Manager Specification (aka wm-spec aka
//     Extended Window Manager Hints) introduced _NET_WM_STATE_FULLSCREEN
//     window state which provides cleanest and simplest possible way of
//     making a window fullscreen. WM-spec is a de-facto standard adopted
//     by GNOME and KDE folks, but _NET_WM_STATE_FULLSCREEN isn't yet widely
//     supported. As of January 2003, only GNOME 2's default WM Metacity
//     implements, KDE will support it from version 3.2. At toolkits level,
//     GTK+ >= 2.1.2 uses it as the only method of making windows fullscreen
//     (that's why wxGTK will *not* switch to using gtk_window_fullscreen
//     unless it has better compatibility with older WMs).
//
//
//     This is what wxWidgets does in wxSetFullScreenStateX11:
//       1) if _NET_WM_STATE_FULLSCREEN is supported, use it
//       2) otherwise try WM-specific hacks (KDE, IceWM)
//       3) use _WIN_LAYER and hope that the WM will recognize it
//     The code was tested with:
//     twm, IceWM, WindowMaker, Metacity, kwin, sawfish, lesstif-mwm


#define  WIN_LAYER_NORMAL       4
#define  WIN_LAYER_ABOVE_DOCK  10

static void wxWinHintsSetLayer(Display *display, Window rootWnd,
                               Window window, int layer)
{
    wxX11ErrorsSuspender noerrors(display);

    XEvent xev;

    wxMAKE_ATOM( _WIN_LAYER, display );

    if (IsMapped(display, window))
    {
        xev.type = ClientMessage;
        xev.xclient.type = ClientMessage;
        xev.xclient.window = window;
        xev.xclient.message_type = _WIN_LAYER;
        xev.xclient.format = 32;
        xev.xclient.data.l[0] = (long)layer;
        xev.xclient.data.l[1] = CurrentTime;

        XSendEvent(display, rootWnd, False,
                   SubstructureNotifyMask, (XEvent*) &xev);
    }
    else
    {
        long data[1];

        data[0] = layer;
        XChangeProperty(display, window,
                        _WIN_LAYER, XA_CARDINAL, 32,
                        PropModeReplace, (unsigned char *)data, 1);
    }
}



#ifdef __WXGTK20__
static bool wxQueryWMspecSupport(Display* WXUNUSED(display),
                                 Window WXUNUSED(rootWnd),
                                 Atom (feature))
{
    GdkAtom gatom = gdk_x11_xatom_to_atom(feature);
    return gdk_net_wm_supports(gatom);
}
#else
static bool wxQueryWMspecSupport(Display *display, Window rootWnd, Atom feature)
{
    wxMAKE_ATOM(_NET_SUPPORTING_WM_CHECK, display);
    wxMAKE_ATOM(_NET_SUPPORTED, display);

    // FIXME: We may want to cache these checks. Note that we can't simply
    //        remember the results in global variable because the WM may go
    //        away and be replaced by another one! One possible approach
    //        would be invalidate the case every 15 seconds or so. Since this
    //        code is currently only used by wxTopLevelWindow::ShowFullScreen,
    //        it is not important that it is not optimized.
    //
    //        If the WM supports ICCCM (i.e. the root window has
    //        _NET_SUPPORTING_WM_CHECK property that points to a WM-owned
    //        window), we could watch for DestroyNotify event on the window
    //        and invalidate our cache when the windows goes away (= WM
    //        is replaced by another one). This is what GTK+ 2 does.
    //        Let's do it only if it is needed, it requires changes to
    //        the event loop.

    Atom type;
    Window *wins;
    Atom *atoms;
    int format;
    unsigned long after;
    unsigned long nwins, natoms;

    // Is the WM ICCCM supporting?
    XGetWindowProperty(display, rootWnd,
                       _NET_SUPPORTING_WM_CHECK, 0, LONG_MAX,
                       False, XA_WINDOW, &type, &format, &nwins,
                       &after, (unsigned char **)&wins);
    if ( type != XA_WINDOW || nwins <= 0 || wins[0] == None )
       return false;
    XFree(wins);

    // Query for supported features:
    XGetWindowProperty(display, rootWnd,
                       _NET_SUPPORTED, 0, LONG_MAX,
                       False, XA_ATOM, &type, &format, &natoms,
                       &after, (unsigned char **)&atoms);
    if ( type != XA_ATOM || atoms == NULL )
        return false;

    // Lookup the feature we want:
    for (unsigned i = 0; i < natoms; i++)
    {
        if ( atoms[i] == feature )
        {
            XFree(atoms);
            return true;
        }
    }
    XFree(atoms);
    return false;
}
#endif


#define _NET_WM_STATE_REMOVE        0
#define _NET_WM_STATE_ADD           1

static void wxWMspecSetState(Display *display, Window rootWnd,
                             Window window, int operation, Atom state)
{
    wxMAKE_ATOM(_NET_WM_STATE, display);

    if ( IsMapped(display, window) )
    {
        XEvent xev;
        xev.type = ClientMessage;
        xev.xclient.type = ClientMessage;
        xev.xclient.serial = 0;
        xev.xclient.send_event = True;
        xev.xclient.display = display;
        xev.xclient.window = window;
        xev.xclient.message_type = _NET_WM_STATE;
        xev.xclient.format = 32;
        xev.xclient.data.l[0] = operation;
        xev.xclient.data.l[1] = state;
        xev.xclient.data.l[2] = None;

        XSendEvent(display, rootWnd,
                   False,
                   SubstructureRedirectMask | SubstructureNotifyMask,
                   &xev);
    }
    // FIXME - must modify _NET_WM_STATE property list if the window
    //         wasn't mapped!
}

static void wxWMspecSetFullscreen(Display *display, Window rootWnd,
                                  Window window, bool fullscreen)
{
    wxMAKE_ATOM(_NET_WM_STATE_FULLSCREEN, display);
    wxWMspecSetState(display, rootWnd,
                     window,
                     fullscreen ? _NET_WM_STATE_ADD : _NET_WM_STATE_REMOVE,
                      _NET_WM_STATE_FULLSCREEN);
}


// Is the user running KDE's kwin window manager? At least kwin from KDE 3
// sets KWIN_RUNNING property on the root window.
static bool wxKwinRunning(Display *display, Window rootWnd)
{
    wxMAKE_ATOM(KWIN_RUNNING, display);

    long *data;
    Atom type;
    int format;
    unsigned long nitems, after;
    if (XGetWindowProperty(display, rootWnd,
                           KWIN_RUNNING, 0, 1, False, KWIN_RUNNING,
                           &type, &format, &nitems, &after,
                           (unsigned char**)&data) != Success)
    {
        return false;
    }

    bool retval = (type == KWIN_RUNNING &&
                   nitems == 1 && data && data[0] == 1);
    XFree(data);
    return retval;
}

// KDE's kwin is Qt-centric so much than no normal method of fullscreen
// mode will work with it. We have to carefully emulate the Qt way.
static void wxSetKDEFullscreen(Display *display, Window rootWnd,
                               Window w, bool fullscreen, wxRect *origRect)
{
    long data[2];
    unsigned lng;

    wxMAKE_ATOM(_NET_WM_WINDOW_TYPE, display);
    wxMAKE_ATOM(_NET_WM_WINDOW_TYPE_NORMAL, display);
    wxMAKE_ATOM(_KDE_NET_WM_WINDOW_TYPE_OVERRIDE, display);
    wxMAKE_ATOM(_NET_WM_STATE_STAYS_ON_TOP, display);

    if (fullscreen)
    {
        data[0] = _KDE_NET_WM_WINDOW_TYPE_OVERRIDE;
        data[1] = _NET_WM_WINDOW_TYPE_NORMAL;
        lng = 2;
    }
    else
    {
        data[0] = _NET_WM_WINDOW_TYPE_NORMAL;
        data[1] = None;
        lng = 1;
    }

    // it is necessary to unmap the window, otherwise kwin will ignore us:
    XSync(display, False);

    bool wasMapped = IsMapped(display, w);
    if (wasMapped)
    {
        XUnmapWindow(display, w);
        XSync(display, False);
    }

    XChangeProperty(display, w, _NET_WM_WINDOW_TYPE, XA_ATOM, 32,
                    PropModeReplace, (unsigned char *) &data[0], lng);
    XSync(display, False);

    if (wasMapped)
    {
        XMapRaised(display, w);
        XSync(display, False);
    }

    wxWMspecSetState(display, rootWnd, w,
                     fullscreen ? _NET_WM_STATE_ADD : _NET_WM_STATE_REMOVE,
                     _NET_WM_STATE_STAYS_ON_TOP);
    XSync(display, False);

    if (!fullscreen)
    {
        // NB: like many other WMs, kwin ignores the first request for a window
        //     position change after the window was mapped. This additional
        //     move+resize event will ensure that the window is restored in
        //     exactly the same position as before it was made fullscreen
        //     (because wxTopLevelWindow::ShowFullScreen will call SetSize, thus
        //     setting the position for the second time).
        XMoveResizeWindow(display, w,
                          origRect->x, origRect->y,
                          origRect->width, origRect->height);
        XSync(display, False);
    }
}


wxX11FullScreenMethod wxGetFullScreenMethodX11(WXDisplay* display,
                                               WXWindow rootWindow)
{
    Window root = WindowCast(rootWindow);
    Display *disp = (Display*)display;

    // if WM supports _NET_WM_STATE_FULLSCREEN from wm-spec 1.2, use it:
    wxMAKE_ATOM(_NET_WM_STATE_FULLSCREEN, disp);
    if (wxQueryWMspecSupport(disp, root, _NET_WM_STATE_FULLSCREEN))
    {
        wxLogTrace(_T("fullscreen"),
                   _T("detected _NET_WM_STATE_FULLSCREEN support"));
        return wxX11_FS_WMSPEC;
    }

    // if the user is running KDE's kwin WM, use a legacy hack because
    // kwin doesn't understand any other method:
    if (wxKwinRunning(disp, root))
    {
        wxLogTrace(_T("fullscreen"), _T("detected kwin"));
        return wxX11_FS_KDE;
    }

    // finally, fall back to ICCCM heuristic method:
    wxLogTrace(_T("fullscreen"), _T("unknown WM, using _WIN_LAYER"));
    return wxX11_FS_GENERIC;
}


void wxSetFullScreenStateX11(WXDisplay* display, WXWindow rootWindow,
                             WXWindow window, bool show,
                             wxRect *origRect,
                             wxX11FullScreenMethod method)
{
    // NB: please see the comment under "Fullscreen mode:" title above
    //     for implications of changing this code.

    Window wnd = WindowCast(window);
    Window root = WindowCast(rootWindow);
    Display *disp = (Display*)display;

    if (method == wxX11_FS_AUTODETECT)
        method = wxGetFullScreenMethodX11(display, rootWindow);

    switch (method)
    {
        case wxX11_FS_WMSPEC:
            wxWMspecSetFullscreen(disp, root, wnd, show);
            break;
        case wxX11_FS_KDE:
            wxSetKDEFullscreen(disp, root, wnd, show, origRect);
            break;
        default:
            wxWinHintsSetLayer(disp, root, wnd,
                               show ? WIN_LAYER_ABOVE_DOCK : WIN_LAYER_NORMAL);
            break;
    }
}



// ----------------------------------------------------------------------------
// keycode translations
// ----------------------------------------------------------------------------

#include <X11/keysym.h>

// FIXME what about tables??

int wxCharCodeXToWX(KeySym keySym)
{
    int id;
    switch (keySym)
    {
        case XK_Shift_L:
        case XK_Shift_R:
            id = WXK_SHIFT; break;
        case XK_Control_L:
        case XK_Control_R:
            id = WXK_CONTROL; break;
        case XK_Meta_L:
        case XK_Meta_R:
            id = WXK_ALT; break;
        case XK_Caps_Lock:
            id = WXK_CAPITAL; break;
        case XK_BackSpace:
            id = WXK_BACK; break;
        case XK_Delete:
            id = WXK_DELETE; break;
        case XK_Clear:
            id = WXK_CLEAR; break;
        case XK_Tab:
            id = WXK_TAB; break;
        case XK_numbersign:
            id = '#'; break;
        case XK_Return:
            id = WXK_RETURN; break;
        case XK_Escape:
            id = WXK_ESCAPE; break;
        case XK_Pause:
        case XK_Break:
            id = WXK_PAUSE; break;
        case XK_Num_Lock:
            id = WXK_NUMLOCK; break;
        case XK_Scroll_Lock:
            id = WXK_SCROLL; break;

        case XK_Home:
            id = WXK_HOME; break;
        case XK_End:
            id = WXK_END; break;
        case XK_Left:
            id = WXK_LEFT; break;
        case XK_Right:
            id = WXK_RIGHT; break;
        case XK_Up:
            id = WXK_UP; break;
        case XK_Down:
            id = WXK_DOWN; break;
        case XK_Next:
            id = WXK_PAGEDOWN; break;
        case XK_Prior:
            id = WXK_PAGEUP; break;
        case XK_Menu:
            id = WXK_MENU; break;
        case XK_Select:
            id = WXK_SELECT; break;
        case XK_Cancel:
            id = WXK_CANCEL; break;
        case XK_Print:
            id = WXK_PRINT; break;
        case XK_Execute:
            id = WXK_EXECUTE; break;
        case XK_Insert:
            id = WXK_INSERT; break;
        case XK_Help:
            id = WXK_HELP; break;

        case XK_KP_Multiply:
            id = WXK_NUMPAD_MULTIPLY; break;
        case XK_KP_Add:
            id = WXK_NUMPAD_ADD; break;
        case XK_KP_Subtract:
            id = WXK_NUMPAD_SUBTRACT; break;
        case XK_KP_Divide:
            id = WXK_NUMPAD_DIVIDE; break;
        case XK_KP_Decimal:
            id = WXK_NUMPAD_DECIMAL; break;
        case XK_KP_Equal:
            id = WXK_NUMPAD_EQUAL; break;
        case XK_KP_Space:
            id = WXK_NUMPAD_SPACE; break;
        case XK_KP_Tab:
            id = WXK_NUMPAD_TAB; break;
        case XK_KP_Enter:
            id = WXK_NUMPAD_ENTER; break;
        case XK_KP_0:
            id = WXK_NUMPAD0; break;
        case XK_KP_1:
            id = WXK_NUMPAD1; break;
        case XK_KP_2:
            id = WXK_NUMPAD2; break;
        case XK_KP_3:
            id = WXK_NUMPAD3; break;
        case XK_KP_4:
            id = WXK_NUMPAD4; break;
        case XK_KP_5:
            id = WXK_NUMPAD5; break;
        case XK_KP_6:
            id = WXK_NUMPAD6; break;
        case XK_KP_7:
            id = WXK_NUMPAD7; break;
        case XK_KP_8:
            id = WXK_NUMPAD8; break;
        case XK_KP_9:
            id = WXK_NUMPAD9; break;
        case XK_KP_Insert:
            id = WXK_NUMPAD_INSERT; break;
        case XK_KP_End:
            id = WXK_NUMPAD_END; break;
        case XK_KP_Down:
            id = WXK_NUMPAD_DOWN; break;
        case XK_KP_Page_Down:
            id = WXK_NUMPAD_PAGEDOWN; break;
        case XK_KP_Left:
            id = WXK_NUMPAD_LEFT; break;
        case XK_KP_Right:
            id = WXK_NUMPAD_RIGHT; break;
        case XK_KP_Home:
            id = WXK_NUMPAD_HOME; break;
        case XK_KP_Up:
            id = WXK_NUMPAD_UP; break;
        case XK_KP_Page_Up:
            id = WXK_NUMPAD_PAGEUP; break;
        case XK_F1:
            id = WXK_F1; break;
        case XK_F2:
            id = WXK_F2; break;
        case XK_F3:
            id = WXK_F3; break;
        case XK_F4:
            id = WXK_F4; break;
        case XK_F5:
            id = WXK_F5; break;
        case XK_F6:
            id = WXK_F6; break;
        case XK_F7:
            id = WXK_F7; break;
        case XK_F8:
            id = WXK_F8; break;
        case XK_F9:
            id = WXK_F9; break;
        case XK_F10:
            id = WXK_F10; break;
        case XK_F11:
            id = WXK_F11; break;
        case XK_F12:
            id = WXK_F12; break;
        case XK_F13:
            id = WXK_F13; break;
        case XK_F14:
            id = WXK_F14; break;
        case XK_F15:
            id = WXK_F15; break;
        case XK_F16:
            id = WXK_F16; break;
        case XK_F17:
            id = WXK_F17; break;
        case XK_F18:
            id = WXK_F18; break;
        case XK_F19:
            id = WXK_F19; break;
        case XK_F20:
            id = WXK_F20; break;
        case XK_F21:
            id = WXK_F21; break;
        case XK_F22:
            id = WXK_F22; break;
        case XK_F23:
            id = WXK_F23; break;
        case XK_F24:
            id = WXK_F24; break;
        default:
            id = (keySym <= 255) ? (int)keySym : -1;
    }

    return id;
}

KeySym wxCharCodeWXToX(int id)
{
    KeySym keySym;

    switch (id)
    {
        case WXK_CANCEL:        keySym = XK_Cancel; break;
        case WXK_BACK:          keySym = XK_BackSpace; break;
        case WXK_TAB:           keySym = XK_Tab; break;
        case WXK_CLEAR:         keySym = XK_Clear; break;
        case WXK_RETURN:        keySym = XK_Return; break;
        case WXK_SHIFT:         keySym = XK_Shift_L; break;
        case WXK_CONTROL:       keySym = XK_Control_L; break;
        case WXK_ALT:           keySym = XK_Meta_L; break;
        case WXK_CAPITAL:       keySym = XK_Caps_Lock; break;
        case WXK_MENU :         keySym = XK_Menu; break;
        case WXK_PAUSE:         keySym = XK_Pause; break;
        case WXK_ESCAPE:        keySym = XK_Escape; break;
        case WXK_SPACE:         keySym = ' '; break;
        case WXK_PAGEUP:        keySym = XK_Prior; break;
        case WXK_PAGEDOWN:      keySym = XK_Next; break;
        case WXK_END:           keySym = XK_End; break;
        case WXK_HOME :         keySym = XK_Home; break;
        case WXK_LEFT :         keySym = XK_Left; break;
        case WXK_UP:            keySym = XK_Up; break;
        case WXK_RIGHT:         keySym = XK_Right; break;
        case WXK_DOWN :         keySym = XK_Down; break;
        case WXK_SELECT:        keySym = XK_Select; break;
        case WXK_PRINT:         keySym = XK_Print; break;
        case WXK_EXECUTE:       keySym = XK_Execute; break;
        case WXK_INSERT:        keySym = XK_Insert; break;
        case WXK_DELETE:        keySym = XK_Delete; break;
        case WXK_HELP :         keySym = XK_Help; break;
        case WXK_NUMPAD0:       keySym = XK_KP_0; break; case WXK_NUMPAD_INSERT:     keySym = XK_KP_Insert; break;
        case WXK_NUMPAD1:       keySym = XK_KP_1; break; case WXK_NUMPAD_END:           keySym = XK_KP_End; break;
        case WXK_NUMPAD2:       keySym = XK_KP_2; break; case WXK_NUMPAD_DOWN:      keySym = XK_KP_Down; break;
        case WXK_NUMPAD3:       keySym = XK_KP_3; break; case WXK_NUMPAD_PAGEDOWN:  keySym = XK_KP_Page_Down; break;
        case WXK_NUMPAD4:       keySym = XK_KP_4; break; case WXK_NUMPAD_LEFT:         keySym = XK_KP_Left; break;
        case WXK_NUMPAD5:       keySym = XK_KP_5; break;
        case WXK_NUMPAD6:       keySym = XK_KP_6; break; case WXK_NUMPAD_RIGHT:       keySym = XK_KP_Right; break;
        case WXK_NUMPAD7:       keySym = XK_KP_7; break; case WXK_NUMPAD_HOME:       keySym = XK_KP_Home; break;
        case WXK_NUMPAD8:       keySym = XK_KP_8; break; case WXK_NUMPAD_UP:             keySym = XK_KP_Up; break;
        case WXK_NUMPAD9:       keySym = XK_KP_9; break; case WXK_NUMPAD_PAGEUP:   keySym = XK_KP_Page_Up; break;
        case WXK_NUMPAD_DECIMAL:    keySym = XK_KP_Decimal; break; case WXK_NUMPAD_DELETE:   keySym = XK_KP_Delete; break;
        case WXK_NUMPAD_MULTIPLY:   keySym = XK_KP_Multiply; break;
        case WXK_NUMPAD_ADD:             keySym = XK_KP_Add; break;
        case WXK_NUMPAD_SUBTRACT: keySym = XK_KP_Subtract; break;
        case WXK_NUMPAD_DIVIDE:        keySym = XK_KP_Divide; break;
        case WXK_NUMPAD_ENTER:        keySym = XK_KP_Enter; break;
        case WXK_NUMPAD_SEPARATOR:  keySym = XK_KP_Separator; break;
        case WXK_F1:            keySym = XK_F1; break;
        case WXK_F2:            keySym = XK_F2; break;
        case WXK_F3:            keySym = XK_F3; break;
        case WXK_F4:            keySym = XK_F4; break;
        case WXK_F5:            keySym = XK_F5; break;
        case WXK_F6:            keySym = XK_F6; break;
        case WXK_F7:            keySym = XK_F7; break;
        case WXK_F8:            keySym = XK_F8; break;
        case WXK_F9:            keySym = XK_F9; break;
        case WXK_F10:           keySym = XK_F10; break;
        case WXK_F11:           keySym = XK_F11; break;
        case WXK_F12:           keySym = XK_F12; break;
        case WXK_F13:           keySym = XK_F13; break;
        case WXK_F14:           keySym = XK_F14; break;
        case WXK_F15:           keySym = XK_F15; break;
        case WXK_F16:           keySym = XK_F16; break;
        case WXK_F17:           keySym = XK_F17; break;
        case WXK_F18:           keySym = XK_F18; break;
        case WXK_F19:           keySym = XK_F19; break;
        case WXK_F20:           keySym = XK_F20; break;
        case WXK_F21:           keySym = XK_F21; break;
        case WXK_F22:           keySym = XK_F22; break;
        case WXK_F23:           keySym = XK_F23; break;
        case WXK_F24:           keySym = XK_F24; break;
        case WXK_NUMLOCK:       keySym = XK_Num_Lock; break;
        case WXK_SCROLL:        keySym = XK_Scroll_Lock; break;
        default:                keySym = id <= 255 ? (KeySym)id : 0;
    }

    return keySym;
}


// ----------------------------------------------------------------------------
// check current state of a key
// ----------------------------------------------------------------------------

bool wxGetKeyState(wxKeyCode key)
{
    wxASSERT_MSG(key != WXK_LBUTTON && key != WXK_RBUTTON && key !=
        WXK_MBUTTON, wxT("can't use wxGetKeyState() for mouse buttons"));

    Display *pDisplay = (Display*) wxGetDisplay();

    int iKey = wxCharCodeWXToX(key);
    int          iKeyMask = 0;
    Window       wDummy1, wDummy2;
    int          iDummy3, iDummy4, iDummy5, iDummy6;
    unsigned int iMask;
    KeyCode keyCode = XKeysymToKeycode(pDisplay,iKey);
    if (keyCode == NoSymbol)
        return false;

    if ( IsModifierKey(iKey) )  // If iKey is a modifier key, use a different method
    {
        XModifierKeymap *map = XGetModifierMapping(pDisplay);
        wxCHECK_MSG( map, false, _T("failed to get X11 modifiers map") );

        for (int i = 0; i < 8; ++i)
        {
            if ( map->modifiermap[map->max_keypermod * i] == keyCode)
            {
                iKeyMask = 1 << i;
            }
        }

        XQueryPointer(pDisplay, DefaultRootWindow(pDisplay), &wDummy1, &wDummy2,
                        &iDummy3, &iDummy4, &iDummy5, &iDummy6, &iMask );
        XFreeModifiermap(map);
        return (iMask & iKeyMask) != 0;
    }

    // From the XLib manual:
    // The XQueryKeymap() function returns a bit vector for the logical state of the keyboard,
    // where each bit set to 1 indicates that the corresponding key is currently pressed down.
    // The vector is represented as 32 bytes. Byte N (from 0) contains the bits for keys 8N to 8N + 7
    // with the least-significant bit in the byte representing key 8N.
    char key_vector[32];
    XQueryKeymap(pDisplay, key_vector);
    return key_vector[keyCode >> 3] & (1 << (keyCode & 7));
}

#endif // __WXX11__ || __WXGTK__ || __WXMOTIF__
