/* $XFree86: xc/include/extensions/xf86dga1.h,v 1.2 1999/04/17 07:05:41 dawes Exp $ */
/*

Copyright (c) 1995  Jon Tombs
Copyright (c) 1995  XFree86 Inc

*/

/************************************************************************

   THIS IS THE OLD DGA API AND IS OBSOLETE.  PLEASE DO NOT USE IT ANYMORE

************************************************************************/

#ifndef _XF86DGA1_H_
#define _XF86DGA1_H_

#include <X11/Xfuncproto.h>
#include "SDL_name.h"

#define X_XF86DGAQueryVersion		0
#define X_XF86DGAGetVideoLL		1
#define X_XF86DGADirectVideo		2
#define X_XF86DGAGetViewPortSize	3
#define X_XF86DGASetViewPort		4
#define X_XF86DGAGetVidPage		5
#define X_XF86DGASetVidPage		6
#define X_XF86DGAInstallColormap	7
#define X_XF86DGAQueryDirectVideo	8
#define X_XF86DGAViewPortChanged	9

#define XF86DGADirectPresent		0x0001
#define XF86DGADirectGraphics		0x0002
#define XF86DGADirectMouse		0x0004
#define XF86DGADirectKeyb		0x0008
#define XF86DGAHasColormap		0x0100
#define XF86DGADirectColormap		0x0200




#ifndef _XF86DGA_SERVER_

_XFUNCPROTOBEGIN Bool SDL_NAME(XF86DGAQueryVersion) (
#if NeedFunctionPrototypes
                                                        Display * /* dpy */ ,
                                                        int *
                                                        /* majorVersion */ ,
                                                        int *   /* minorVersion */
#endif
    );

Bool SDL_NAME(XF86DGAQueryExtension) (
#if NeedFunctionPrototypes
                                         Display * /* dpy */ ,
                                         int * /* event_base */ ,
                                         int *  /* error_base */
#endif
    );

Status SDL_NAME(XF86DGAGetVideoLL) (
#if NeedFunctionPrototypes
                                       Display * /* dpy */ ,
                                       int /* screen */ ,
                                       int * /* base addr */ ,
                                       int * /* width */ ,
                                       int * /* bank_size */ ,
                                       int *    /* ram_size */
#endif
    );

Status SDL_NAME(XF86DGAGetVideo) (
#if NeedFunctionPrototypes
                                     Display * /* dpy */ ,
                                     int /* screen */ ,
                                     char ** /* base addr */ ,
                                     int * /* width */ ,
                                     int * /* bank_size */ ,
                                     int *      /* ram_size */
#endif
    );

Status SDL_NAME(XF86DGADirectVideo) (
#if NeedFunctionPrototypes
                                        Display * /* dpy */ ,
                                        int /* screen */ ,
                                        int     /* enable */
#endif
    );

Status SDL_NAME(XF86DGADirectVideoLL) (
#if NeedFunctionPrototypes
                                          Display * /* dpy */ ,
                                          int /* screen */ ,
                                          int   /* enable */
#endif
    );

Status SDL_NAME(XF86DGAGetViewPortSize) (
#if NeedFunctionPrototypes
                                            Display * /* dpy */ ,
                                            int /* screen */ ,
                                            int * /* width */ ,
                                            int *       /* height */
#endif
    );

Status SDL_NAME(XF86DGASetViewPort) (
#if NeedFunctionPrototypes
                                        Display * /* dpy */ ,
                                        int /* screen */ ,
                                        int x /* X */ ,
                                        int y   /* Y */
#endif
    );

Status SDL_NAME(XF86DGAGetVidPage) (
#if NeedFunctionPrototypes
                                       Display * /* dpy */ ,
                                       int /* screen */ ,
                                       int *    /* vid page */
#endif
    );

Status SDL_NAME(XF86DGASetVidPage) (
#if NeedFunctionPrototypes
                                       Display * /* dpy */ ,
                                       int /* screen */ ,
                                       int      /* vid page */
#endif
    );

Status SDL_NAME(XF86DGAInstallColormap) (
#if NeedFunctionPrototypes
                                            Display * /* dpy */ ,
                                            int /* screen */ ,
                                            Colormap    /*Colormap */
#endif
    );

int SDL_NAME(XF86DGAForkApp) (
#if NeedFunctionPrototypes
                                 int screen
#endif
    );

Status SDL_NAME(XF86DGAQueryDirectVideo) (
#if NeedFunctionPrototypes
                                             Display * /* dpy */ ,
                                             int /* screen */ ,
                                             int *      /* flags */
#endif
    );

Bool SDL_NAME(XF86DGAViewPortChanged) (
#if NeedFunctionPrototypes
                                          Display * /* dpy */ ,
                                          int /* screen */ ,
                                          int   /* n */
#endif
    );


_XFUNCPROTOEND
#endif /* _XF86DGA_SERVER_ */
#endif /* _XF86DGA1_H_ */
/* vi: set ts=4 sw=4 expandtab: */
