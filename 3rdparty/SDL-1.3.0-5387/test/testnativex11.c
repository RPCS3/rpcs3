
#include "testnative.h"

#ifdef TEST_NATIVE_X11

static void *CreateWindowX11(int w, int h);
static void DestroyWindowX11(void *window);

NativeWindowFactory X11WindowFactory = {
    "x11",
    CreateWindowX11,
    DestroyWindowX11
};

static Display *dpy;

static void *
CreateWindowX11(int w, int h)
{
    Window window = 0;

    dpy = XOpenDisplay(NULL);
    if (dpy) {
        window =
            XCreateSimpleWindow(dpy, DefaultRootWindow(dpy), 0, 0, w, h, 0, 0,
                                0);
        XMapRaised(dpy, window);
        XSync(dpy, False);
    }
    return (void *) window;
}

static void
DestroyWindowX11(void *window)
{
    if (dpy) {
        XDestroyWindow(dpy, (Window) window);
        XCloseDisplay(dpy);
    }
}

#endif
