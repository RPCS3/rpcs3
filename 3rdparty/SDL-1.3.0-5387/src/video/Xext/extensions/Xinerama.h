/* $XFree86: xc/include/extensions/Xinerama.h,v 3.2 2000/03/01 01:04:20 dawes Exp $ */

#ifndef _Xinerama_h
#define _Xinerama_h

#include "SDL_name.h"

typedef struct
{
    int screen_number;
    short x_org;
    short y_org;
    short width;
    short height;
} SDL_NAME(XineramaScreenInfo);

Bool SDL_NAME(XineramaQueryExtension) (Display * dpy,
                                       int *event_base, int *error_base);

Status SDL_NAME(XineramaQueryVersion) (Display * dpy, int *major, int *minor);

Bool SDL_NAME(XineramaIsActive) (Display * dpy);


/* 
   Returns the number of heads and a pointer to an array of
   structures describing the position and size of the individual
   heads.  Returns NULL and number = 0 if Xinerama is not active.
  
   Returned array should be freed with XFree().
*/

SDL_NAME(XineramaScreenInfo) *
SDL_NAME(XineramaQueryScreens) (Display * dpy, int *number);

#endif /* _Xinerama_h */
/* vi: set ts=4 sw=4 expandtab: */
