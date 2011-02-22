/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 2010 Eli Gottlieb

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

    Eli Gottlieb
    eligottlieb@gmail.com
*/

#include "SDL_cocoavideo.h"
#include "SDL_shape.h"
#include "SDL_cocoashape.h"
#include "../SDL_sysvideo.h"

SDL_WindowShaper*
Cocoa_CreateShaper(SDL_Window* window) {
    SDL_WindowData* windata = (SDL_WindowData*)window->driverdata;
    [windata->nswindow setOpaque:NO];
#if MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_6
    [windata->nswindow setStyleMask:NSBorderlessWindowMask];
#endif
    SDL_WindowShaper* result = result = malloc(sizeof(SDL_WindowShaper));
    result->window = window;
    result->mode.mode = ShapeModeDefault;
    result->mode.parameters.binarizationCutoff = 1;
    result->userx = result->usery = 0;
    window->shaper = result;
    
    SDL_ShapeData* data = malloc(sizeof(SDL_ShapeData));
    result->driverdata = data;
    data->context = [windata->nswindow graphicsContext];
    data->saved = SDL_FALSE;
    data->shape = NULL;
    
    int resized_properly = Cocoa_ResizeWindowShape(window);
    assert(resized_properly == 0);
    return result;
}

typedef struct {
    NSView* view;
    NSBezierPath* path;
    SDL_Window* window;
} SDL_CocoaClosure;

void
ConvertRects(SDL_ShapeTree* tree,void* closure) {
    SDL_CocoaClosure* data = (SDL_CocoaClosure*)closure;
    if(tree->kind == OpaqueShape) {
        NSRect rect = NSMakeRect(tree->data.shape.x,data->window->h - tree->data.shape.y,tree->data.shape.w,tree->data.shape.h);
        [data->path appendBezierPathWithRect:[data->view convertRect:rect toView:nil]];
    }
}

int
Cocoa_SetWindowShape(SDL_WindowShaper *shaper,SDL_Surface *shape,SDL_WindowShapeMode *shape_mode) {
    SDL_ShapeData* data = (SDL_ShapeData*)shaper->driverdata;
	SDL_WindowData* windata = (SDL_WindowData*)shaper->window->driverdata;
	SDL_CocoaClosure closure;
	NSAutoreleasePool *pool = NULL;
    if(data->saved == SDL_TRUE) {
        [data->context restoreGraphicsState];
        data->saved = SDL_FALSE;
    }
        
    //[data->context saveGraphicsState];
    //data->saved = SDL_TRUE;
	[NSGraphicsContext setCurrentContext:data->context];
    
    [[NSColor clearColor] set];
    NSRectFill([[windata->nswindow contentView] frame]);
    data->shape = SDL_CalculateShapeTree(*shape_mode,shape);
	
	pool = [[NSAutoreleasePool alloc] init];
    closure.view = [windata->nswindow contentView];
    closure.path = [[NSBezierPath bezierPath] autorelease];
	closure.window = shaper->window;
    SDL_TraverseShapeTree(data->shape,&ConvertRects,&closure);
    [closure.path addClip];
}

int
Cocoa_ResizeWindowShape(SDL_Window *window) {
    SDL_ShapeData* data = window->shaper->driverdata;
    assert(data != NULL);
    return 0;
}
