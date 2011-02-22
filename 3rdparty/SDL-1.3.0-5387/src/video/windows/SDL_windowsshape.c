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

#include <stdio.h>
#include "SDL_assert.h"
#include "SDL_windowsshape.h"
#include "SDL_windowsvideo.h"

SDL_WindowShaper*
Win32_CreateShaper(SDL_Window * window) {
    int resized_properly;
    SDL_WindowShaper* result = (SDL_WindowShaper *)SDL_malloc(sizeof(SDL_WindowShaper));
    result->window = window;
    result->mode.mode = ShapeModeDefault;
    result->mode.parameters.binarizationCutoff = 1;
    result->userx = result->usery = 0;
    result->driverdata = (SDL_ShapeData*)SDL_malloc(sizeof(SDL_ShapeData));
    ((SDL_ShapeData*)result->driverdata)->mask_tree = NULL;
    //Put some driver-data here.
    window->shaper = result;
    resized_properly = Win32_ResizeWindowShape(window);
    if (resized_properly != 0)
            return NULL;
    
    return result;
}

void
CombineRectRegions(SDL_ShapeTree* node,void* closure) {
    HRGN mask_region = *((HRGN*)closure),temp_region = NULL;
    if(node->kind == OpaqueShape) {
        //Win32 API regions exclude their outline, so we widen the region by one pixel in each direction to include the real outline.
        temp_region = CreateRectRgn(node->data.shape.x,node->data.shape.y,node->data.shape.x + node->data.shape.w + 1,node->data.shape.y + node->data.shape.h + 1);
        if(mask_region != NULL) {
            CombineRgn(mask_region,mask_region,temp_region,RGN_OR);
            DeleteObject(temp_region);
		}
		else
            *((HRGN*)closure) = temp_region;
	}
}

int
Win32_SetWindowShape(SDL_WindowShaper *shaper,SDL_Surface *shape,SDL_WindowShapeMode *shape_mode) {
    SDL_ShapeData *data;
    HRGN mask_region = NULL;

    if (shaper == NULL || shape == NULL)
        return SDL_INVALID_SHAPE_ARGUMENT;
    if(shape->format->Amask == 0 && shape_mode->mode != ShapeModeColorKey || shape->w != shaper->window->w || shape->h != shaper->window->h)
        return SDL_INVALID_SHAPE_ARGUMENT;
    
    data = (SDL_ShapeData*)shaper->driverdata;
    if(data->mask_tree != NULL)
        SDL_FreeShapeTree(&data->mask_tree);
    data->mask_tree = SDL_CalculateShapeTree(*shape_mode,shape);
    
    SDL_TraverseShapeTree(data->mask_tree,&CombineRectRegions,&mask_region);
	SDL_assert(mask_region != NULL);

    SetWindowRgn(((SDL_WindowData *)(shaper->window->driverdata))->hwnd, mask_region, TRUE);
    
    return 0;
}

int
Win32_ResizeWindowShape(SDL_Window *window) {
    SDL_ShapeData* data;

    if (window == NULL)
        return -1;
    data = (SDL_ShapeData *)window->shaper->driverdata;
    if (data == NULL)
        return -1;
    
    if(data->mask_tree != NULL)
        SDL_FreeShapeTree(&data->mask_tree);
    if(window->shaper->hasshape == SDL_TRUE) {
        window->shaper->userx = window->x;
        window->shaper->usery = window->y;
        SDL_SetWindowPosition(window,-1000,-1000);
	}
    
    return 0;
}
