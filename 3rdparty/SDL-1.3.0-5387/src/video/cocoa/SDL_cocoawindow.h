/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997-2011 Sam Lantinga

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

    Sam Lantinga
    slouken@libsdl.org
*/
#include "SDL_config.h"

#ifndef _SDL_cocoawindow_h
#define _SDL_cocoawindow_h

#import <Cocoa/Cocoa.h>

typedef struct SDL_WindowData SDL_WindowData;

/* *INDENT-OFF* */
#if MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_6
@interface Cocoa_WindowListener : NSResponder <NSWindowDelegate> {
#else
@interface Cocoa_WindowListener : NSResponder {
#endif
    SDL_WindowData *_data;
}

-(void) listen:(SDL_WindowData *) data;
-(void) close;

/* Window delegate functionality */
-(BOOL) windowShouldClose:(id) sender;
-(void) windowDidExpose:(NSNotification *) aNotification;
-(void) windowDidMove:(NSNotification *) aNotification;
-(void) windowDidResize:(NSNotification *) aNotification;
-(void) windowDidMiniaturize:(NSNotification *) aNotification;
-(void) windowDidDeminiaturize:(NSNotification *) aNotification;
-(void) windowDidBecomeKey:(NSNotification *) aNotification;
-(void) windowDidResignKey:(NSNotification *) aNotification;
-(void) windowDidHide:(NSNotification *) aNotification;
-(void) windowDidUnhide:(NSNotification *) aNotification;

/* Window event handling */
-(void) mouseDown:(NSEvent *) theEvent;
-(void) rightMouseDown:(NSEvent *) theEvent;
-(void) otherMouseDown:(NSEvent *) theEvent;
-(void) mouseUp:(NSEvent *) theEvent;
-(void) rightMouseUp:(NSEvent *) theEvent;
-(void) otherMouseUp:(NSEvent *) theEvent;
-(void) mouseEntered:(NSEvent *)theEvent;
-(void) mouseExited:(NSEvent *)theEvent;
-(void) mouseMoved:(NSEvent *) theEvent;
-(void) mouseDragged:(NSEvent *) theEvent;
-(void) rightMouseDragged:(NSEvent *) theEvent;
-(void) otherMouseDragged:(NSEvent *) theEvent;
-(void) scrollWheel:(NSEvent *) theEvent;
-(void) touchesBeganWithEvent:(NSEvent *) theEvent;
-(void) touchesMovedWithEvent:(NSEvent *) theEvent;
-(void) touchesEndedWithEvent:(NSEvent *) theEvent;
-(void) touchesCancelledWithEvent:(NSEvent *) theEvent;

/* Touch event handling */
typedef enum {
    COCOA_TOUCH_DOWN,
    COCOA_TOUCH_UP,
    COCOA_TOUCH_MOVE,
    COCOA_TOUCH_CANCELLED
} cocoaTouchType;
-(void) handleTouches:(cocoaTouchType)type withEvent:(NSEvent*) event;

@end
/* *INDENT-ON* */

struct SDL_WindowData
{
    SDL_Window *window;
    NSWindow *nswindow;
    SDL_bool created;
    Cocoa_WindowListener *listener;
    struct SDL_VideoData *videodata;
};

extern int Cocoa_CreateWindow(_THIS, SDL_Window * window);
extern int Cocoa_CreateWindowFrom(_THIS, SDL_Window * window,
                                  const void *data);
extern void Cocoa_SetWindowTitle(_THIS, SDL_Window * window);
extern void Cocoa_SetWindowIcon(_THIS, SDL_Window * window, SDL_Surface * icon);
extern void Cocoa_SetWindowPosition(_THIS, SDL_Window * window);
extern void Cocoa_SetWindowSize(_THIS, SDL_Window * window);
extern void Cocoa_ShowWindow(_THIS, SDL_Window * window);
extern void Cocoa_HideWindow(_THIS, SDL_Window * window);
extern void Cocoa_RaiseWindow(_THIS, SDL_Window * window);
extern void Cocoa_MaximizeWindow(_THIS, SDL_Window * window);
extern void Cocoa_MinimizeWindow(_THIS, SDL_Window * window);
extern void Cocoa_RestoreWindow(_THIS, SDL_Window * window);
extern void Cocoa_SetWindowFullscreen(_THIS, SDL_Window * window, SDL_VideoDisplay * display, SDL_bool fullscreen);
extern void Cocoa_SetWindowGrab(_THIS, SDL_Window * window);
extern void Cocoa_DestroyWindow(_THIS, SDL_Window * window);
extern SDL_bool Cocoa_GetWindowWMInfo(_THIS, SDL_Window * window,
                                      struct SDL_SysWMinfo *info);

#endif /* _SDL_cocoawindow_h */

/* vi: set ts=4 sw=4 expandtab: */
