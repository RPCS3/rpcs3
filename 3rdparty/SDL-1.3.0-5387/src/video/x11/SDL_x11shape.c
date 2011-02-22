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

#include "SDL_assert.h"
#include "SDL_x11video.h"
#include "SDL_x11shape.h"
#include "SDL_x11window.h"

SDL_Window*
X11_CreateShapedWindow(const char *title,unsigned int x,unsigned int y,unsigned int w,unsigned int h,Uint32 flags) {
    return SDL_CreateWindow(title,x,y,w,h,flags);
}

SDL_WindowShaper*
X11_CreateShaper(SDL_Window* window) {
    SDL_WindowShaper* result = NULL;

#if SDL_VIDEO_DRIVER_X11_XSHAPE
    if (SDL_X11_HAVE_XSHAPE) {  /* Make sure X server supports it. */
        result = malloc(sizeof(SDL_WindowShaper));
        result->window = window;
        result->mode.mode = ShapeModeDefault;
        result->mode.parameters.binarizationCutoff = 1;
        result->userx = result->usery = 0;
        SDL_ShapeData* data = SDL_malloc(sizeof(SDL_ShapeData));
        result->driverdata = data;
        data->bitmapsize = 0;
        data->bitmap = NULL;
        window->shaper = result;
        int resized_properly = X11_ResizeWindowShape(window);
        SDL_assert(resized_properly == 0);
    }
#endif

    return result;
}

int
X11_ResizeWindowShape(SDL_Window* window) {
    SDL_ShapeData* data = window->shaper->driverdata;
    SDL_assert(data != NULL);
    
    unsigned int bitmapsize = window->w / 8;
    if(window->w % 8 > 0)
        bitmapsize += 1;
    bitmapsize *= window->h;
    if(data->bitmapsize != bitmapsize || data->bitmap == NULL) {
        data->bitmapsize = bitmapsize;
        if(data->bitmap != NULL)
            free(data->bitmap);
        data->bitmap = malloc(data->bitmapsize);
        if(data->bitmap == NULL) {
            SDL_SetError("Could not allocate memory for shaped-window bitmap.");
            return -1;
        }
    }
    memset(data->bitmap,0,data->bitmapsize);
    
    window->shaper->userx = window->x;
    window->shaper->usery = window->y;
    SDL_SetWindowPosition(window,-1000,-1000);
    
    return 0;
}
    
int
X11_SetWindowShape(SDL_WindowShaper *shaper,SDL_Surface *shape,SDL_WindowShapeMode *shape_mode) {
    if(shaper == NULL || shape == NULL || shaper->driverdata == NULL)
        return -1;

#if SDL_VIDEO_DRIVER_X11_XSHAPE
    if(shape->format->Amask == 0 && SDL_SHAPEMODEALPHA(shape_mode->mode))
        return -2;
    if(shape->w != shaper->window->w || shape->h != shaper->window->h)
        return -3;
    SDL_ShapeData *data = shaper->driverdata;
    
    /* Assume that shaper->alphacutoff already has a value, because SDL_SetWindowShape() should have given it one. */
    SDL_CalculateShapeBitmap(shaper->mode,shape,data->bitmap,8);
        
    SDL_WindowData *windowdata = (SDL_WindowData*)(shaper->window->driverdata);
    Pixmap shapemask = XCreateBitmapFromData(windowdata->videodata->display,windowdata->xwindow,data->bitmap,shaper->window->w,shaper->window->h);
    
    XShapeCombineMask(windowdata->videodata->display,windowdata->xwindow, ShapeBounding, 0, 0,shapemask, ShapeSet);
    XSync(windowdata->videodata->display,False);

    XFreePixmap(windowdata->videodata->display,shapemask);
#endif

    return 0;
}
