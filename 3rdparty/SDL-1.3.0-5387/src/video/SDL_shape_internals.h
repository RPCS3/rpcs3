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
#include "SDL_config.h"

#ifndef _SDL_shape_internals_h
#define _SDL_shape_internals_h

#include "SDL_rect.h"
#include "SDL_shape.h"
#include "SDL_surface.h"

#include "begin_code.h"
/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
/* *INDENT-OFF* */
extern "C" {
/* *INDENT-ON* */
#endif

typedef struct {
	struct SDL_ShapeTree *upleft,*upright,*downleft,*downright;
} SDL_QuadTreeChildren;

typedef union {
	SDL_QuadTreeChildren children;
	SDL_Rect shape;
} SDL_ShapeUnion;

typedef enum { QuadShape,TransparentShape,OpaqueShape } SDL_ShapeKind;

typedef struct {
	SDL_ShapeKind kind;
	SDL_ShapeUnion data;
} SDL_ShapeTree;
	
typedef void(*SDL_TraversalFunction)(SDL_ShapeTree*,void*);

extern void SDL_CalculateShapeBitmap(SDL_WindowShapeMode mode,SDL_Surface *shape,Uint8* bitmap,Uint8 ppb);
extern SDL_ShapeTree* SDL_CalculateShapeTree(SDL_WindowShapeMode mode,SDL_Surface* shape);
extern void SDL_TraverseShapeTree(SDL_ShapeTree *tree,SDL_TraversalFunction function,void* closure);
extern void SDL_FreeShapeTree(SDL_ShapeTree** shape_tree);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
/* *INDENT-OFF* */
}
/* *INDENT-ON* */
#endif
#include "close_code.h"

#endif
