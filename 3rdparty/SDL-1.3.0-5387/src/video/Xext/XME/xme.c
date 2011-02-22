/*
 * Copyright 1993-2001 by Xi Graphics, Inc.
 * All Rights Reserved.
 *
 * Please see the LICENSE file accompanying this distribution for licensing 
 * information. 
 *
 * Please send any bug fixes and modifications to src@xig.com.
 *
 * $XiGId: xme.c,v 1.2 2001/11/30 21:56:59 jon Exp $
 *
 */

#define NEED_EVENTS
#define NEED_REPLIES

/* Apparently some X11 systems can't include this multiple times... */
#ifndef SDL_INCLUDED_XLIBINT_H
#define SDL_INCLUDED_XLIBINT_H 1
#include <X11/Xlibint.h>
#endif

#include <X11/Xthreads.h>
#include <X11/Xmd.h>
#include <X11/Xproto.h>
#include "../extensions/Xext.h"
#include "../extensions/extutil.h"

/*****************************************************************************/


#define	XIGMISC_PROTOCOL_NAME 		     "XiG-SUNDRY-NONSTANDARD"
#define XIGMISC_MAJOR_VERSION	             2
#define XIGMISC_MINOR_VERSION 	             0

#define XiGMiscNumberEvents	             0

#define	X_XiGMiscQueryVersion		     0
#define	X_XiGMiscQueryViews		     1
#define X_XiGMiscQueryResolutions            2
#define X_XiGMiscChangeResolution            3
#define X_XiGMiscFullScreen                  4

#define sz_xXiGMiscQueryVersionReq	     8
#define sz_xXiGMiscQueryViewsReq	     8
#define sz_xXiGMiscQueryResolutionsReq       8
#define sz_xXiGMiscChangeResolutionReq       16
#define sz_xXiGMiscFullScreenReq             16

#define sz_xXiGMiscQueryVersionReply	     32
#define sz_xXiGMiscQueryViewsReply	     32
#define sz_xXiGMiscQueryResolutionsReply     32
#define sz_xXiGMiscQueryFullScreenReply      32

/*******************************************************************/

typedef struct
{
    CARD8 reqType;              /* always codes->major_opcode        */
    CARD8 xigmiscReqType;       /* always X_XiGMiscQueryVersion      */
    CARD16 length;
    CARD16 major;
    CARD16 minor;
} xXiGMiscQueryVersionReq;

typedef struct
{
    CARD8 reqType;              /* always codes->major_opcode        */
    CARD8 xigmiscReqType;       /* always X_XiGMiscQueryViews        */
    CARD16 length;
    CARD8 screen;
    CARD8 pad0;
    CARD16 pad1;
} xXiGMiscQueryViewsReq;

typedef struct
{
    CARD8 reqType;              /* always codes->major_opcode        */
    CARD8 xigmiscReqType;       /* always X_XiGMiscQueryResolutions  */
    CARD16 length;
    CARD8 screen;
    CARD8 view;
    CARD16 pad0;
} xXiGMiscQueryResolutionsReq;

typedef struct
{
    CARD8 reqType;              /* always codes->major_opcode        */
    CARD8 xigmiscReqType;       /* always X_XiGMiscChangeResolution  */
    CARD16 length;
    CARD8 screen;
    CARD8 view;
    CARD16 pad0;
    CARD16 width;
    CARD16 height;
    INT32 refresh;
} xXiGMiscChangeResolutionReq;

typedef struct
{
    CARD8 reqType;              /* always codes->major_opcode        */
    CARD8 xigmiscReqType;       /* always X_XiGMiscFullScreen        */
    CARD16 length;
    CARD8 screen;
    CARD8 pad0;
    CARD16 pad1;
    CARD32 window;
    CARD32 cmap;
} xXiGMiscFullScreenReq;

/*******************************************************************/

typedef struct
{
    BYTE type;                  /* X_Reply                           */
    CARD8 pad0;
    CARD16 sequenceNumber;
    CARD32 length;
    CARD16 major;
    CARD16 minor;
    CARD32 pad1;
    CARD32 pad2;
    CARD32 pad3;
    CARD32 pad4;
    CARD32 pad5;
} xXiGMiscQueryVersionReply;

typedef struct
{
    BYTE type;                  /* X_Reply                           */
    CARD8 pad0;
    CARD16 sequenceNumber;
    CARD32 length;
    CARD32 nviews;
    CARD32 pad1;
    CARD32 pad2;
    CARD32 pad3;
    CARD32 pad4;
    CARD32 pad5;
} xXiGMiscQueryViewsReply;

typedef struct
{
    BYTE type;                  /* X_Reply                           */
    CARD8 pad0;
    CARD16 sequenceNumber;
    CARD32 length;
    CARD16 active;
    CARD16 nresolutions;
    CARD32 pad1;
    CARD32 pad2;
    CARD32 pad3;
    CARD32 pad4;
    CARD32 pad5;
} xXiGMiscQueryResolutionsReply;

typedef struct
{
    BYTE type;                  /* X_Reply                           */
    BOOL success;
    CARD16 sequenceNumber;
    CARD32 length;
    CARD32 pad1;
    CARD32 pad2;
    CARD32 pad3;
    CARD32 pad4;
    CARD32 pad5;
    CARD32 pad6;
} xXiGMiscFullScreenReply;

/*******************************************************************/

typedef struct
{
    INT16 x;
    INT16 y;
    CARD16 w;
    CARD16 h;
} XiGMiscViewInfo;

typedef struct
{
    CARD16 width;
    CARD16 height;
    INT32 refresh;
} XiGMiscResolutionInfo;

/*****************************************************************************/

static XExtensionInfo *xigmisc_info = NULL;
static char *xigmisc_extension_name = XIGMISC_PROTOCOL_NAME;

#define XiGMiscCheckExtension(dpy,i,val) \
  XextCheckExtension (dpy, i, xigmisc_extension_name, val)
#define XiGMiscSimpleCheckExtension(dpy,i) \
  XextSimpleCheckExtension (dpy, i, xigmisc_extension_name)

#if defined(__STDC__) && !defined(UNIXCPP)
#define XiGMiscGetReq(name,req,info) GetReq (name, req); \
        req->reqType = info->codes->major_opcode; \
        req->xigmiscReqType = X_##name;

#define XiGMiscGetReqExtra(name,n,req,info) GetReqExtra (name, n, req); \
        req->reqType = info->codes->major_opcode; \
        req->xigmicReqType = X_##name;
#else
#define XiGMiscGetReq(name,req,info) GetReq (name, req); \
        req->reqType = info->codes->major_opcode; \
        req->xigmiscReqType = X_/**/name;
#define XiGMiscGetReqExtra(name,n,req,info) GetReqExtra (name, n, req); \
        req->reqType = info->codes->major_opcode; \
        req->xigmiscReqType = X_/**/name;
#endif



/*
 * find_display - locate the display info block
 */
static int XiGMiscCloseDisplay();

static XExtensionHooks xigmisc_extension_hooks = {
    NULL,                       /* create_gc */
    NULL,                       /* copy_gc */
    NULL,                       /* flush_gc */
    NULL,                       /* free_gc */
    NULL,                       /* create_font */
    NULL,                       /* free_font */
    XiGMiscCloseDisplay,        /* close_display */
    NULL,                       /* wire_to_event */
    NULL,                       /* event_to_wire */
    NULL,                       /* error */
    NULL,                       /* error_string */
};


static
XEXT_GENERATE_CLOSE_DISPLAY(XiGMiscCloseDisplay, xigmisc_info)
     static XEXT_GENERATE_FIND_DISPLAY(XiGMiscFindDisplay, xigmisc_info,
                                       xigmisc_extension_name,
                                       &xigmisc_extension_hooks,
                                       XiGMiscNumberEvents, NULL)
/*****************************************************************************/
     Bool XiGMiscQueryVersion(Display * dpy, int *major, int *minor)
{
    int opcode, event, error;
    xXiGMiscQueryVersionReq *req;
    xXiGMiscQueryVersionReply rep;
    XExtDisplayInfo *info = XiGMiscFindDisplay(dpy);

    if (!XQueryExtension(dpy, XIGMISC_PROTOCOL_NAME, &opcode, &event, &error))
        return xFalse;

    XiGMiscCheckExtension(dpy, info, xFalse);

    LockDisplay(dpy);
    XiGMiscGetReq(XiGMiscQueryVersion, req, info);

    req->major = XIGMISC_MAJOR_VERSION;
    req->minor = XIGMISC_MINOR_VERSION;

    if (!_XReply(dpy, (xReply *) & rep, 0, xTrue)) {
        UnlockDisplay(dpy);
        SyncHandle();
        return xFalse;
    }

    *major = rep.major;
    *minor = rep.minor;
    UnlockDisplay(dpy);
    SyncHandle();

    return xTrue;
}

int
XiGMiscQueryViews(Display * dpy, int screen, XiGMiscViewInfo ** pviews)
{
    int n, size;
    XiGMiscViewInfo *views;
    xXiGMiscQueryViewsReq *req;
    xXiGMiscQueryViewsReply rep;
    XExtDisplayInfo *info = XiGMiscFindDisplay(dpy);
    XiGMiscCheckExtension(dpy, info, 0);

    LockDisplay(dpy);
    XiGMiscGetReq(XiGMiscQueryViews, req, info);
    req->screen = screen;

    if (!_XReply(dpy, (xReply *) & rep, 0, xFalse)) {
        UnlockDisplay(dpy);
        SyncHandle();
        return 0;
    }

    n = rep.nviews;

    if (n > 0) {
        size = sizeof(XiGMiscViewInfo) * n;
        views = (XiGMiscViewInfo *) Xmalloc(size);
        if (!views) {
            _XEatData(dpy, (unsigned long) size);
            UnlockDisplay(dpy);
            SyncHandle();
            return 0;
        }

        _XReadPad(dpy, (void *) views, size);

        *pviews = views;
    }

    UnlockDisplay(dpy);
    SyncHandle();

    return n;
}

int
XiGMiscQueryResolutions(Display * dpy, int screen, int view, int *pactive,
                        XiGMiscResolutionInfo ** presolutions)
{
    int n, size;
    XiGMiscResolutionInfo *resolutions;
    xXiGMiscQueryResolutionsReq *req;
    xXiGMiscQueryResolutionsReply rep;
    XExtDisplayInfo *info = XiGMiscFindDisplay(dpy);
    XiGMiscCheckExtension(dpy, info, 0);

    LockDisplay(dpy);
    XiGMiscGetReq(XiGMiscQueryResolutions, req, info);
    req->screen = screen;
    req->view = view;

    if (!_XReply(dpy, (xReply *) & rep, 0, xFalse)) {
        UnlockDisplay(dpy);
        SyncHandle();
        return 0;
    }

    n = rep.nresolutions;

    if (n > 0) {
        size = sizeof(XiGMiscResolutionInfo) * n;
        resolutions = (XiGMiscResolutionInfo *) Xmalloc(size);
        if (!resolutions) {
            _XEatData(dpy, (unsigned long) size);
            UnlockDisplay(dpy);
            SyncHandle();
            return 0;
        }

        _XReadPad(dpy, (void *) resolutions, size);

        *presolutions = resolutions;
        *pactive = rep.active;
    }

    UnlockDisplay(dpy);
    SyncHandle();

    return n;
}

void
XiGMiscChangeResolution(Display * dpy, int screen, int view, int width,
                        int height, int refresh)
{
    xXiGMiscChangeResolutionReq *req;
    XExtDisplayInfo *info = XiGMiscFindDisplay(dpy);

    XiGMiscSimpleCheckExtension(dpy, info);

    LockDisplay(dpy);
    XiGMiscGetReq(XiGMiscChangeResolution, req, info);
    req->screen = screen;
    req->view = view;
    req->width = width;
    req->height = height;
    req->refresh = refresh;

    UnlockDisplay(dpy);
    SyncHandle();
}


Bool
XiGMiscFullScreen(Display * dpy, int screen, XID window, XID cmap)
{
    xXiGMiscFullScreenReq *req;
    xXiGMiscFullScreenReply rep;
    XExtDisplayInfo *info = XiGMiscFindDisplay(dpy);

    XiGMiscCheckExtension(dpy, info, xFalse);

    LockDisplay(dpy);
    XiGMiscGetReq(XiGMiscFullScreen, req, info);
    req->screen = screen;
    req->pad0 = 0;
    req->pad1 = 0;
    req->window = window;
    req->cmap = cmap;

    if (!_XReply(dpy, (xReply *) & rep, 0, xTrue)) {
        UnlockDisplay(dpy);
        SyncHandle();
        return xFalse;
    }

    UnlockDisplay(dpy);
    SyncHandle();

    return (rep.success ? xTrue : xFalse);
}

/* SDL addition from Ryan: free memory used by xme. */
void
XiGMiscDestroy(void)
{
    if (xigmisc_info) {
        XextDestroyExtension(xigmisc_info);
        xigmisc_info = NULL;
    }
}

/* vi: set ts=4 sw=4 expandtab: */
