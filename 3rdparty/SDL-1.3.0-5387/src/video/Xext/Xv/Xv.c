/***********************************************************
Copyright 1991 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its 
documentation for any purpose and without fee is hereby granted, 
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in 
supporting documentation, and that the names of Digital or MIT not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.  

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

******************************************************************/
/* $XFree86: xc/lib/Xv/Xv.c,v 1.15 2001/05/11 08:23:22 alanh Exp $ */
/*
** File: 
**
**   Xv.c --- Xv library extension module.
**
** Author: 
**
**   David Carver (Digital Workstation Engineering/Project Athena)
**
** Revisions:
**
**   26.06.91 Carver
**     - changed XvFreeAdaptors to XvFreeAdaptorInfo
**     - changed XvFreeEncodings to XvFreeEncodingInfo
**
**   11.06.91 Carver
**     - changed SetPortControl to SetPortAttribute
**     - changed GetPortControl to GetPortAttribute
**     - changed QueryBestSize
**
**   15.05.91 Carver
**     - version 2.0 upgrade
**
**   240.01.91 Carver
**     - version 1.4 upgrade
**
*/

#include <stdio.h>
#include "Xvlibint.h"
#include "../extensions/Xext.h"
#include <X11/extensions/XShm.h>
#include "../extensions/extutil.h"

static XExtensionInfo _xv_info_data;
static XExtensionInfo *xv_info = &_xv_info_data;
static char *xv_extension_name = XvName;

#define XvCheckExtension(dpy, i, val) \
  XextCheckExtension(dpy, i, xv_extension_name, val)

static char *xv_error_string();
static int xv_close_display();
static Bool xv_wire_to_event();

static XExtensionHooks xv_extension_hooks = {
    NULL,                       /* create_gc */
    NULL,                       /* copy_gc */
    NULL,                       /* flush_gc */
    NULL,                       /* free_gc */
    NULL,                       /* create_font */
    NULL,                       /* free_font */
    xv_close_display,           /* close_display */
    xv_wire_to_event,           /* wire_to_event */
    NULL,                       /* event_to_wire */
    NULL,                       /* error */
    xv_error_string             /* error_string */
};


static char *xv_error_list[] = {
    "BadPort",                  /* XvBadPort     */
    "BadEncoding",              /* XvBadEncoding */
    "BadControl"                /* XvBadControl  */
};

static
XEXT_GENERATE_CLOSE_DISPLAY(xv_close_display, xv_info)
     static XEXT_GENERATE_FIND_DISPLAY(xv_find_display, xv_info,
                                       xv_extension_name,
                                       &xv_extension_hooks, XvNumEvents, NULL)
     static XEXT_GENERATE_ERROR_STRING(xv_error_string, xv_extension_name,
                                       XvNumErrors, xv_error_list)
     int SDL_NAME(XvQueryExtension) (Display * dpy,
                                     unsigned int *p_version,
                                     unsigned int *p_revision,
                                     unsigned int *p_requestBase,
                                     unsigned int *p_eventBase,
                                     unsigned int *p_errorBase)
{
    XExtDisplayInfo *info = xv_find_display(dpy);
    xvQueryExtensionReq *req;
    xvQueryExtensionReply rep;

    XvCheckExtension(dpy, info, XvBadExtension);

    LockDisplay(dpy);

    XvGetReq(QueryExtension, req);

    if (!_XReply(dpy, (xReply *) & rep, 0, xFalse)) {
        UnlockDisplay(dpy);
        SyncHandle();
        return XvBadExtension;
    }

    *p_version = rep.version;
    *p_revision = rep.revision;
    *p_requestBase = info->codes->major_opcode;
    *p_eventBase = info->codes->first_event;
    *p_errorBase = info->codes->first_error;

    UnlockDisplay(dpy);
    SyncHandle();

    return Success;
}

int
SDL_NAME(XvQueryAdaptors) (Display * dpy,
                           Window window,
                           unsigned int *p_nAdaptors,
                           SDL_NAME(XvAdaptorInfo) ** p_pAdaptors)
{
    XExtDisplayInfo *info = xv_find_display(dpy);
    xvQueryAdaptorsReq *req;
    xvQueryAdaptorsReply rep;
    int size, ii, jj;
    char *name;
    SDL_NAME(XvAdaptorInfo) * pas, *pa;
    SDL_NAME(XvFormat) * pfs, *pf;
    char *buffer;
    union
    {
        char *buffer;
        char *string;
        xvAdaptorInfo *pa;
        xvFormat *pf;
    } u;

    XvCheckExtension(dpy, info, XvBadExtension);

    LockDisplay(dpy);

    XvGetReq(QueryAdaptors, req);
    req->window = window;

    /* READ THE REPLY */

    if (_XReply(dpy, (xReply *) & rep, 0, xFalse) == 0) {
        UnlockDisplay(dpy);
        SyncHandle();
        return (XvBadReply);
    }

    size = rep.length << 2;
    if ((buffer = (char *) Xmalloc((unsigned) size)) == NULL) {
        UnlockDisplay(dpy);
        SyncHandle();
        return (XvBadAlloc);
    }
    _XRead(dpy, buffer, size);

    u.buffer = buffer;

    /* GET INPUT ADAPTORS */

    if (rep.num_adaptors == 0) {
        pas = NULL;
    } else {
        size = rep.num_adaptors * sizeof(SDL_NAME(XvAdaptorInfo));
        if ((pas = (SDL_NAME(XvAdaptorInfo) *) Xmalloc(size)) == NULL) {
            Xfree(buffer);
            UnlockDisplay(dpy);
            SyncHandle();
            return (XvBadAlloc);
        }
    }

    /* INIT ADAPTOR FIELDS */

    pa = pas;
    for (ii = 0; ii < rep.num_adaptors; ii++) {
        pa->num_adaptors = 0;
        pa->name = (char *) NULL;
        pa->formats = (SDL_NAME(XvFormat) *) NULL;
        pa++;
    }

    pa = pas;
    for (ii = 0; ii < rep.num_adaptors; ii++) {
        pa->type = u.pa->type;
        pa->base_id = u.pa->base_id;
        pa->num_ports = u.pa->num_ports;
        pa->num_formats = u.pa->num_formats;
        pa->num_adaptors = rep.num_adaptors - ii;

        /* GET ADAPTOR NAME */

        size = u.pa->name_size;
        u.buffer += (sz_xvAdaptorInfo + 3) & ~3;

        if ((name = (char *) Xmalloc(size + 1)) == NULL) {
            SDL_NAME(XvFreeAdaptorInfo) (pas);
            Xfree(buffer);
            UnlockDisplay(dpy);
            SyncHandle();
            return (XvBadAlloc);
        }
        SDL_strlcpy(name, u.string, size);
        pa->name = name;

        u.buffer += (size + 3) & ~3;

        /* GET FORMATS */

        size = pa->num_formats * sizeof(SDL_NAME(XvFormat));
        if ((pfs = (SDL_NAME(XvFormat) *) Xmalloc(size)) == NULL) {
            SDL_NAME(XvFreeAdaptorInfo) (pas);
            Xfree(buffer);
            UnlockDisplay(dpy);
            SyncHandle();
            return (XvBadAlloc);
        }

        pf = pfs;
        for (jj = 0; jj < pa->num_formats; jj++) {
            pf->depth = u.pf->depth;
            pf->visual_id = u.pf->visual;
            pf++;

            u.buffer += (sz_xvFormat + 3) & ~3;
        }

        pa->formats = pfs;

        pa++;

    }

    *p_nAdaptors = rep.num_adaptors;
    *p_pAdaptors = pas;

    Xfree(buffer);
    UnlockDisplay(dpy);
    SyncHandle();

    return (Success);
}


void SDL_NAME(XvFreeAdaptorInfo) (SDL_NAME(XvAdaptorInfo) * pAdaptors)
{

    SDL_NAME(XvAdaptorInfo) * pa;
    int ii;

    if (!pAdaptors)
        return;

    pa = pAdaptors;

    for (ii = 0; ii < pAdaptors->num_adaptors; ii++, pa++) {
        if (pa->name) {
            Xfree(pa->name);
        }
        if (pa->formats) {
            Xfree(pa->formats);
        }
    }

    Xfree(pAdaptors);
}

int
SDL_NAME(XvQueryEncodings) (Display * dpy,
                            XvPortID port,
                            unsigned int *p_nEncodings,
                            SDL_NAME(XvEncodingInfo) ** p_pEncodings)
{
    XExtDisplayInfo *info = xv_find_display(dpy);
    xvQueryEncodingsReq *req;
    xvQueryEncodingsReply rep;
    int size, jj;
    char *name;
    SDL_NAME(XvEncodingInfo) * pes, *pe;
    char *buffer;
    union
    {
        char *buffer;
        char *string;
        xvEncodingInfo *pe;
    } u;

    XvCheckExtension(dpy, info, XvBadExtension);

    LockDisplay(dpy);

    XvGetReq(QueryEncodings, req);
    req->port = port;

    /* READ THE REPLY */

    if (_XReply(dpy, (xReply *) & rep, 0, xFalse) == 0) {
        UnlockDisplay(dpy);
        SyncHandle();
        return (XvBadReply);
    }

    size = rep.length << 2;
    if ((buffer = (char *) Xmalloc((unsigned) size)) == NULL) {
        UnlockDisplay(dpy);
        SyncHandle();
        return (XvBadAlloc);
    }
    _XRead(dpy, buffer, size);

    u.buffer = buffer;

    /* GET ENCODINGS */

    size = rep.num_encodings * sizeof(SDL_NAME(XvEncodingInfo));
    if ((pes = (SDL_NAME(XvEncodingInfo) *) Xmalloc(size)) == NULL) {
        Xfree(buffer);
        UnlockDisplay(dpy);
        SyncHandle();
        return (XvBadAlloc);
    }

    /* INITIALIZE THE ENCODING POINTER */

    pe = pes;
    for (jj = 0; jj < rep.num_encodings; jj++) {
        pe->name = (char *) NULL;
        pe->num_encodings = 0;
        pe++;
    }

    pe = pes;
    for (jj = 0; jj < rep.num_encodings; jj++) {
        pe->encoding_id = u.pe->encoding;
        pe->width = u.pe->width;
        pe->height = u.pe->height;
        pe->rate.numerator = u.pe->rate.numerator;
        pe->rate.denominator = u.pe->rate.denominator;
        pe->num_encodings = rep.num_encodings - jj;

        size = u.pe->name_size;
        u.buffer += (sz_xvEncodingInfo + 3) & ~3;

        if ((name = (char *) Xmalloc(size + 1)) == NULL) {
            Xfree(buffer);
            UnlockDisplay(dpy);
            SyncHandle();
            return (XvBadAlloc);
        }
        SDL_strlcpy(name, u.string, size);
        pe->name = name;
        pe++;

        u.buffer += (size + 3) & ~3;
    }

    *p_nEncodings = rep.num_encodings;
    *p_pEncodings = pes;

    Xfree(buffer);
    UnlockDisplay(dpy);
    SyncHandle();

    return (Success);
}

void SDL_NAME(XvFreeEncodingInfo) (SDL_NAME(XvEncodingInfo) * pEncodings)
{

    SDL_NAME(XvEncodingInfo) * pe;
    int ii;

    if (!pEncodings)
        return;

    pe = pEncodings;

    for (ii = 0; ii < pEncodings->num_encodings; ii++, pe++) {
        if (pe->name)
            Xfree(pe->name);
    }

    Xfree(pEncodings);
}

int
SDL_NAME(XvPutVideo) (Display * dpy,
                      XvPortID port,
                      Drawable d,
                      GC gc,
                      int vx, int vy,
                      unsigned int vw, unsigned int vh,
                      int dx, int dy, unsigned int dw, unsigned int dh)
{
    XExtDisplayInfo *info = xv_find_display(dpy);
    xvPutVideoReq *req;

    XvCheckExtension(dpy, info, XvBadExtension);

    LockDisplay(dpy);

    FlushGC(dpy, gc);

    XvGetReq(PutVideo, req);

    req->port = port;
    req->drawable = d;
    req->gc = gc->gid;
    req->vid_x = vx;
    req->vid_y = vy;
    req->vid_w = vw;
    req->vid_h = vh;
    req->drw_x = dx;
    req->drw_y = dy;
    req->drw_w = dw;
    req->drw_h = dh;

    UnlockDisplay(dpy);
    SyncHandle();

    return Success;
}

int
SDL_NAME(XvPutStill) (Display * dpy,
                      XvPortID port,
                      Drawable d,
                      GC gc,
                      int vx, int vy,
                      unsigned int vw, unsigned int vh,
                      int dx, int dy, unsigned int dw, unsigned int dh)
{
    XExtDisplayInfo *info = xv_find_display(dpy);
    xvPutStillReq *req;

    XvCheckExtension(dpy, info, XvBadExtension);

    LockDisplay(dpy);

    FlushGC(dpy, gc);

    XvGetReq(PutStill, req);
    req->port = port;
    req->drawable = d;
    req->gc = gc->gid;
    req->vid_x = vx;
    req->vid_y = vy;
    req->vid_w = vw;
    req->vid_h = vh;
    req->drw_x = dx;
    req->drw_y = dy;
    req->drw_w = dw;
    req->drw_h = dh;

    UnlockDisplay(dpy);
    SyncHandle();

    return Success;
}

int
SDL_NAME(XvGetVideo) (Display * dpy,
                      XvPortID port,
                      Drawable d,
                      GC gc,
                      int vx, int vy,
                      unsigned int vw, unsigned int vh,
                      int dx, int dy, unsigned int dw, unsigned int dh)
{
    XExtDisplayInfo *info = xv_find_display(dpy);
    xvGetVideoReq *req;

    XvCheckExtension(dpy, info, XvBadExtension);

    LockDisplay(dpy);

    FlushGC(dpy, gc);

    XvGetReq(GetVideo, req);
    req->port = port;
    req->drawable = d;
    req->gc = gc->gid;
    req->vid_x = vx;
    req->vid_y = vy;
    req->vid_w = vw;
    req->vid_h = vh;
    req->drw_x = dx;
    req->drw_y = dy;
    req->drw_w = dw;
    req->drw_h = dh;

    UnlockDisplay(dpy);
    SyncHandle();

    return Success;
}

int
SDL_NAME(XvGetStill) (Display * dpy,
                      XvPortID port,
                      Drawable d,
                      GC gc,
                      int vx, int vy,
                      unsigned int vw, unsigned int vh,
                      int dx, int dy, unsigned int dw, unsigned int dh)
{
    XExtDisplayInfo *info = xv_find_display(dpy);
    xvGetStillReq *req;

    XvCheckExtension(dpy, info, XvBadExtension);

    LockDisplay(dpy);

    FlushGC(dpy, gc);

    XvGetReq(GetStill, req);
    req->port = port;
    req->drawable = d;
    req->gc = gc->gid;
    req->vid_x = vx;
    req->vid_y = vy;
    req->vid_w = vw;
    req->vid_h = vh;
    req->drw_x = dx;
    req->drw_y = dy;
    req->drw_w = dw;
    req->drw_h = dh;

    UnlockDisplay(dpy);
    SyncHandle();

    return Success;
}

int SDL_NAME(XvStopVideo) (Display * dpy, XvPortID port, Drawable draw)
{
    XExtDisplayInfo *info = xv_find_display(dpy);
    xvStopVideoReq *req;

    XvCheckExtension(dpy, info, XvBadExtension);

    LockDisplay(dpy);

    XvGetReq(StopVideo, req);
    req->port = port;
    req->drawable = draw;

    UnlockDisplay(dpy);
    SyncHandle();

    return Success;
}

int SDL_NAME(XvGrabPort) (Display * dpy, XvPortID port, Time time)
{
    XExtDisplayInfo *info = xv_find_display(dpy);
    int result;
    xvGrabPortReply rep;
    xvGrabPortReq *req;

    XvCheckExtension(dpy, info, XvBadExtension);

    LockDisplay(dpy);

    XvGetReq(GrabPort, req);
    req->port = port;
    req->time = time;

    if (_XReply(dpy, (xReply *) & rep, 0, xTrue) == 0)
        rep.result = GrabSuccess;

    result = rep.result;

    UnlockDisplay(dpy);
    SyncHandle();

    return result;
}

int SDL_NAME(XvUngrabPort) (Display * dpy, XvPortID port, Time time)
{
    XExtDisplayInfo *info = xv_find_display(dpy);
    xvUngrabPortReq *req;

    XvCheckExtension(dpy, info, XvBadExtension);

    LockDisplay(dpy);

    XvGetReq(UngrabPort, req);
    req->port = port;
    req->time = time;

    UnlockDisplay(dpy);
    SyncHandle();

    return Success;
}

int
SDL_NAME(XvSelectVideoNotify) (Display * dpy, Drawable drawable, Bool onoff)
{
    XExtDisplayInfo *info = xv_find_display(dpy);
    xvSelectVideoNotifyReq *req;

    XvCheckExtension(dpy, info, XvBadExtension);

    LockDisplay(dpy);

    XvGetReq(SelectVideoNotify, req);
    req->drawable = drawable;
    req->onoff = onoff;

    UnlockDisplay(dpy);
    SyncHandle();

    return Success;
}

int SDL_NAME(XvSelectPortNotify) (Display * dpy, XvPortID port, Bool onoff)
{
    XExtDisplayInfo *info = xv_find_display(dpy);
    xvSelectPortNotifyReq *req;

    XvCheckExtension(dpy, info, XvBadExtension);

    LockDisplay(dpy);

    XvGetReq(SelectPortNotify, req);
    req->port = port;
    req->onoff = onoff;

    UnlockDisplay(dpy);
    SyncHandle();

    return Success;
}

int
SDL_NAME(XvSetPortAttribute) (Display * dpy,
                              XvPortID port, Atom attribute, int value)
{
    XExtDisplayInfo *info = xv_find_display(dpy);
    xvSetPortAttributeReq *req;

    XvCheckExtension(dpy, info, XvBadExtension);

    LockDisplay(dpy);

    XvGetReq(SetPortAttribute, req);
    req->port = port;
    req->attribute = attribute;
    req->value = value;

    UnlockDisplay(dpy);
    SyncHandle();

    return (Success);
}

int
SDL_NAME(XvGetPortAttribute) (Display * dpy,
                              XvPortID port, Atom attribute, int *p_value)
{
    XExtDisplayInfo *info = xv_find_display(dpy);
    xvGetPortAttributeReq *req;
    xvGetPortAttributeReply rep;

    XvCheckExtension(dpy, info, XvBadExtension);

    LockDisplay(dpy);

    XvGetReq(GetPortAttribute, req);
    req->port = port;
    req->attribute = attribute;

    /* READ THE REPLY */

    if (_XReply(dpy, (xReply *) & rep, 0, xFalse) == 0) {
        UnlockDisplay(dpy);
        SyncHandle();
        return (XvBadReply);
    }

    *p_value = rep.value;

    UnlockDisplay(dpy);
    SyncHandle();

    return (Success);
}

int
SDL_NAME(XvQueryBestSize) (Display * dpy,
                           XvPortID port,
                           Bool motion,
                           unsigned int vid_w,
                           unsigned int vid_h,
                           unsigned int drw_w,
                           unsigned int drw_h,
                           unsigned int *p_actual_width,
                           unsigned int *p_actual_height)
{
    XExtDisplayInfo *info = xv_find_display(dpy);
    xvQueryBestSizeReq *req;
    xvQueryBestSizeReply rep;

    XvCheckExtension(dpy, info, XvBadExtension);

    LockDisplay(dpy);

    XvGetReq(QueryBestSize, req);
    req->port = port;
    req->motion = motion;
    req->vid_w = vid_w;
    req->vid_h = vid_h;
    req->drw_w = drw_w;
    req->drw_h = drw_h;

    /* READ THE REPLY */

    if (_XReply(dpy, (xReply *) & rep, 0, xFalse) == 0) {
        UnlockDisplay(dpy);
        SyncHandle();
        return (XvBadReply);
    }

    *p_actual_width = rep.actual_width;
    *p_actual_height = rep.actual_height;

    UnlockDisplay(dpy);
    SyncHandle();

    return (Success);
}


SDL_NAME(XvAttribute) *
SDL_NAME(XvQueryPortAttributes) (Display * dpy, XvPortID port, int *num)
{
    XExtDisplayInfo *info = xv_find_display(dpy);
    xvQueryPortAttributesReq *req;
    xvQueryPortAttributesReply rep;
    SDL_NAME(XvAttribute) * ret = NULL;

    *num = 0;

    XvCheckExtension(dpy, info, NULL);

    LockDisplay(dpy);

    XvGetReq(QueryPortAttributes, req);
    req->port = port;

    /* READ THE REPLY */

    if (_XReply(dpy, (xReply *) & rep, 0, xFalse) == 0) {
        UnlockDisplay(dpy);
        SyncHandle();
        return ret;
    }

    if (rep.num_attributes) {
        int size =
            (rep.num_attributes * sizeof(SDL_NAME(XvAttribute))) +
            rep.text_size;

        if ((ret = Xmalloc(size))) {
            char *marker = (char *) (&ret[rep.num_attributes]);
            xvAttributeInfo Info;
            int i;

            for (i = 0; i < rep.num_attributes; i++) {
                _XRead(dpy, (char *) (&Info), sz_xvAttributeInfo);
                ret[i].flags = (int) Info.flags;
                ret[i].min_value = Info.min;
                ret[i].max_value = Info.max;
                ret[i].name = marker;
                _XRead(dpy, marker, Info.size);
                marker += Info.size;
                (*num)++;
            }
        } else
            _XEatData(dpy, rep.length << 2);
    }

    UnlockDisplay(dpy);
    SyncHandle();

    return ret;
}

SDL_NAME(XvImageFormatValues) * SDL_NAME(XvListImageFormats) (Display * dpy,
                                                              XvPortID port,
                                                              int *num)
{
    XExtDisplayInfo *info = xv_find_display(dpy);
    xvListImageFormatsReq *req;
    xvListImageFormatsReply rep;
    SDL_NAME(XvImageFormatValues) * ret = NULL;

    *num = 0;

    XvCheckExtension(dpy, info, NULL);

    LockDisplay(dpy);

    XvGetReq(ListImageFormats, req);
    req->port = port;

    /* READ THE REPLY */

    if (_XReply(dpy, (xReply *) & rep, 0, xFalse) == 0) {
        UnlockDisplay(dpy);
        SyncHandle();
        return NULL;
    }

    if (rep.num_formats) {
        int size = (rep.num_formats * sizeof(SDL_NAME(XvImageFormatValues)));

        if ((ret = Xmalloc(size))) {
            xvImageFormatInfo Info;
            int i;

            for (i = 0; i < rep.num_formats; i++) {
                _XRead(dpy, (char *) (&Info), sz_xvImageFormatInfo);
                ret[i].id = Info.id;
                ret[i].type = Info.type;
                ret[i].byte_order = Info.byte_order;
                memcpy(&(ret[i].guid[0]), &(Info.guid[0]), 16);
                ret[i].bits_per_pixel = Info.bpp;
                ret[i].format = Info.format;
                ret[i].num_planes = Info.num_planes;
                ret[i].depth = Info.depth;
                ret[i].red_mask = Info.red_mask;
                ret[i].green_mask = Info.green_mask;
                ret[i].blue_mask = Info.blue_mask;
                ret[i].y_sample_bits = Info.y_sample_bits;
                ret[i].u_sample_bits = Info.u_sample_bits;
                ret[i].v_sample_bits = Info.v_sample_bits;
                ret[i].horz_y_period = Info.horz_y_period;
                ret[i].horz_u_period = Info.horz_u_period;
                ret[i].horz_v_period = Info.horz_v_period;
                ret[i].vert_y_period = Info.vert_y_period;
                ret[i].vert_u_period = Info.vert_u_period;
                ret[i].vert_v_period = Info.vert_v_period;
                memcpy(&(ret[i].component_order[0]),
                       &(Info.comp_order[0]), 32);
                ret[i].scanline_order = Info.scanline_order;
                (*num)++;
            }
        } else
            _XEatData(dpy, rep.length << 2);
    }

    UnlockDisplay(dpy);
    SyncHandle();

    return ret;
}

SDL_NAME(XvImage) * SDL_NAME(XvCreateImage) (Display * dpy,
                                             XvPortID port,
                                             int id,
                                             char *data,
                                             int width, int height)
{
    XExtDisplayInfo *info = xv_find_display(dpy);
    xvQueryImageAttributesReq *req;
    xvQueryImageAttributesReply rep;
    SDL_NAME(XvImage) * ret = NULL;

    XvCheckExtension(dpy, info, NULL);

    LockDisplay(dpy);

    XvGetReq(QueryImageAttributes, req);
    req->id = id;
    req->port = port;
    req->width = width;
    req->height = height;

    /* READ THE REPLY */

    if (!_XReply(dpy, (xReply *) & rep, 0, xFalse)) {
        UnlockDisplay(dpy);
        SyncHandle();
        return NULL;
    }

    if ((ret =
         (SDL_NAME(XvImage) *) Xmalloc(sizeof(SDL_NAME(XvImage)) +
                                       (rep.num_planes << 3)))) {
        ret->id = id;
        ret->width = rep.width;
        ret->height = rep.height;
        ret->data_size = rep.data_size;
        ret->num_planes = rep.num_planes;
        ret->pitches = (int *) (&ret[1]);
        ret->offsets = ret->pitches + rep.num_planes;
        ret->data = data;
        ret->obdata = NULL;
        _XRead(dpy, (char *) (ret->pitches), rep.num_planes << 2);
        _XRead(dpy, (char *) (ret->offsets), rep.num_planes << 2);
    } else
        _XEatData(dpy, rep.length << 2);

    UnlockDisplay(dpy);
    SyncHandle();
    return ret;
}

SDL_NAME(XvImage) * SDL_NAME(XvShmCreateImage) (Display * dpy,
                                                XvPortID port,
                                                int id,
                                                char *data,
                                                int width,
                                                int height,
                                                XShmSegmentInfo * shminfo)
{
    SDL_NAME(XvImage) * ret;

    ret = SDL_NAME(XvCreateImage) (dpy, port, id, data, width, height);

    if (ret)
        ret->obdata = (XPointer) shminfo;

    return ret;
}

int SDL_NAME(XvPutImage) (Display * dpy,
                          XvPortID port,
                          Drawable d,
                          GC gc,
                          SDL_NAME(XvImage) * image,
                          int src_x,
                          int src_y,
                          unsigned int src_w,
                          unsigned int src_h,
                          int dest_x,
                          int dest_y,
                          unsigned int dest_w, unsigned int dest_h)
{
    XExtDisplayInfo *info = xv_find_display(dpy);
    xvPutImageReq *req;
    int len;

    XvCheckExtension(dpy, info, XvBadExtension);

    LockDisplay(dpy);

    FlushGC(dpy, gc);

    XvGetReq(PutImage, req);

    req->port = port;
    req->drawable = d;
    req->gc = gc->gid;
    req->id = image->id;
    req->src_x = src_x;
    req->src_y = src_y;
    req->src_w = src_w;
    req->src_h = src_h;
    req->drw_x = dest_x;
    req->drw_y = dest_y;
    req->drw_w = dest_w;
    req->drw_h = dest_h;
    req->width = image->width;
    req->height = image->height;

    len = (image->data_size + 3) >> 2;
    SetReqLen(req, len, len);

    /* Yes it's kindof lame that we are sending the whole thing,
       but for video all of it may be needed even if displaying
       only a subsection, and I don't want to go through the 
       trouble of creating subregions to send */
    Data(dpy, (char *) image->data, image->data_size);

    UnlockDisplay(dpy);
    SyncHandle();

    return Success;
}

int SDL_NAME(XvShmPutImage) (Display * dpy,
                             XvPortID port,
                             Drawable d,
                             GC gc,
                             SDL_NAME(XvImage) * image,
                             int src_x,
                             int src_y,
                             unsigned int src_w,
                             unsigned int src_h,
                             int dest_x,
                             int dest_y,
                             unsigned int dest_w,
                             unsigned int dest_h, Bool send_event)
{
    XExtDisplayInfo *info = xv_find_display(dpy);
    XShmSegmentInfo *shminfo = (XShmSegmentInfo *) image->obdata;
    xvShmPutImageReq *req;

    XvCheckExtension(dpy, info, XvBadExtension);

    LockDisplay(dpy);

    FlushGC(dpy, gc);

    XvGetReq(ShmPutImage, req);

    req->port = port;
    req->drawable = d;
    req->gc = gc->gid;
    req->shmseg = shminfo->shmseg;
    req->id = image->id;
    req->src_x = src_x;
    req->src_y = src_y;
    req->src_w = src_w;
    req->src_h = src_h;
    req->drw_x = dest_x;
    req->drw_y = dest_y;
    req->drw_w = dest_w;
    req->drw_h = dest_h;
    req->offset = image->data - shminfo->shmaddr;
    req->width = image->width;
    req->height = image->height;
    req->send_event = send_event;

    UnlockDisplay(dpy);
    SyncHandle();

    return Success;
}


static Bool
xv_wire_to_event(Display * dpy, XEvent * host, xEvent * wire)
{
    XExtDisplayInfo *info = xv_find_display(dpy);
    SDL_NAME(XvEvent) * re = (SDL_NAME(XvEvent) *) host;
    xvEvent *event = (xvEvent *) wire;

    XvCheckExtension(dpy, info, False);

    switch ((event->u.u.type & 0x7F) - info->codes->first_event) {
    case XvVideoNotify:
        re->xvvideo.type = event->u.u.type & 0x7f;
        re->xvvideo.serial =
            _XSetLastRequestRead(dpy, (xGenericReply *) event);
        re->xvvideo.send_event = ((event->u.u.type & 0x80) != 0);
        re->xvvideo.display = dpy;
        re->xvvideo.time = event->u.videoNotify.time;
        re->xvvideo.reason = event->u.videoNotify.reason;
        re->xvvideo.drawable = event->u.videoNotify.drawable;
        re->xvvideo.port_id = event->u.videoNotify.port;
        break;
    case XvPortNotify:
        re->xvport.type = event->u.u.type & 0x7f;
        re->xvport.serial =
            _XSetLastRequestRead(dpy, (xGenericReply *) event);
        re->xvport.send_event = ((event->u.u.type & 0x80) != 0);
        re->xvport.display = dpy;
        re->xvport.time = event->u.portNotify.time;
        re->xvport.port_id = event->u.portNotify.port;
        re->xvport.attribute = event->u.portNotify.attribute;
        re->xvport.value = event->u.portNotify.value;
        break;
    default:
        return False;
    }

    return (True);
}

/* vi: set ts=4 sw=4 expandtab: */
