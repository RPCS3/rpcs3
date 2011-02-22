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

/**
 *  \file SDL_rect.h
 *  
 *  Header file for SDL_rect definition and management functions.
 */

#ifndef _SDL_rect_h
#define _SDL_rect_h

#include "SDL_stdinc.h"
#include "SDL_error.h"
#include "SDL_pixels.h"
#include "SDL_rwops.h"

#include "begin_code.h"
/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
/* *INDENT-OFF* */
extern "C" {
/* *INDENT-ON* */
#endif

/**
 *  \brief  The structure that defines a point
 *
 *  \sa SDL_EnclosePoints
 */
typedef struct
{
    int x;
    int y;
} SDL_Point;

/**
 *  \brief A rectangle, with the origin at the upper left.
 *  
 *  \sa SDL_RectEmpty
 *  \sa SDL_RectEquals
 *  \sa SDL_HasIntersection
 *  \sa SDL_IntersectRect
 *  \sa SDL_UnionRect
 *  \sa SDL_EnclosePoints
 */
typedef struct SDL_Rect
{
    int x, y;
    int w, h;
} SDL_Rect;

/**
 *  \brief Returns true if the rectangle has no area.
 */
#define SDL_RectEmpty(X)    (((X)->w <= 0) || ((X)->h <= 0))

/**
 *  \brief Returns true if the two rectangles are equal.
 */
#define SDL_RectEquals(A, B)   (((A)->x == (B)->x) && ((A)->y == (B)->y) && \
                                ((A)->w == (B)->w) && ((A)->h == (B)->h))

/**
 *  \brief Determine whether two rectangles intersect.
 *  
 *  \return SDL_TRUE if there is an intersection, SDL_FALSE otherwise.
 */
extern DECLSPEC SDL_bool SDLCALL SDL_HasIntersection(const SDL_Rect * A,
                                                     const SDL_Rect * B);

/**
 *  \brief Calculate the intersection of two rectangles.
 *  
 *  \return SDL_TRUE if there is an intersection, SDL_FALSE otherwise.
 */
extern DECLSPEC SDL_bool SDLCALL SDL_IntersectRect(const SDL_Rect * A,
                                                   const SDL_Rect * B,
                                                   SDL_Rect * result);

/**
 *  \brief Calculate the union of two rectangles.
 */
extern DECLSPEC void SDLCALL SDL_UnionRect(const SDL_Rect * A,
                                           const SDL_Rect * B,
                                           SDL_Rect * result);

/**
 *  \brief Calculate a minimal rectangle enclosing a set of points
 *
 *  \return SDL_TRUE if any points were within the clipping rect
 */
extern DECLSPEC SDL_bool SDLCALL SDL_EnclosePoints(const SDL_Point * points,
                                                   int count,
                                                   const SDL_Rect * clip,
                                                   SDL_Rect * result);

/**
 *  \brief Calculate the intersection of a rectangle and line segment.
 *  
 *  \return SDL_TRUE if there is an intersection, SDL_FALSE otherwise.
 */
extern DECLSPEC SDL_bool SDLCALL SDL_IntersectRectAndLine(const SDL_Rect *
                                                          rect, int *X1,
                                                          int *Y1, int *X2,
                                                          int *Y2);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
/* *INDENT-OFF* */
}
/* *INDENT-ON* */
#endif
#include "close_code.h"

#endif /* _SDL_rect_h */

/* vi: set ts=4 sw=4 expandtab: */
