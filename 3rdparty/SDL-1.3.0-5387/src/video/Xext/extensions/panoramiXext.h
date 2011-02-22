/* $Xorg: panoramiXext.h,v 1.4 2000/08/18 04:05:45 coskrey Exp $ */
/*****************************************************************
Copyright (c) 1991, 1997 Digital Equipment Corporation, Maynard, Massachusetts.
Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software.

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
DIGITAL EQUIPMENT CORPORATION BE LIABLE FOR ANY CLAIM, DAMAGES, INCLUDING,
BUT NOT LIMITED TO CONSEQUENTIAL OR INCIDENTAL DAMAGES, OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of Digital Equipment Corporation
shall not be used in advertising or otherwise to promote the sale, use or other
dealings in this Software without prior written authorization from Digital
Equipment Corporation.
******************************************************************/
/*  
 *	PanoramiX definitions
 */
/* $XFree86: xc/include/extensions/panoramiXext.h,v 3.6 2001/01/17 17:53:22 dawes Exp $ */

#include "SDL_name.h"

/* THIS IS NOT AN X PROJECT TEAM SPECIFICATION */

#define PANORAMIX_MAJOR_VERSION         1       /* current version number */
#define PANORAMIX_MINOR_VERSION         1

typedef struct
{
    Window window;              /* PanoramiX window - may not exist */
    int screen;
    int State;                  /* PanroamiXOff, PanoramiXOn */
    int width;                  /* width of this screen */
    int height;                 /* height of this screen */
    int ScreenCount;            /* real physical number of screens */
    XID eventMask;              /* selected events for this client */
} SDL_NAME(XPanoramiXInfo);

extern
SDL_NAME(XPanoramiXInfo) *
SDL_NAME(XPanoramiXAllocInfo) (
#if NeedFunctionPrototypes
                                  void
#endif
    );
/* vi: set ts=4 sw=4 expandtab: */
