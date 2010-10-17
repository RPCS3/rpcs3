/*  ZZ Open GL graphics plugin
 *  Copyright (c)2009-2010 zeydlitz@gmail.com, arcum42@gmail.com
 *  Based on Zerofrog's ZeroGS KOSMOS (c)2005-2008
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef ZZKICK_H_INCLUDED
#define ZZKICK_H_INCLUDED

#include "GS.h"

// visible members
typedef void (*DrawFn)();
extern DrawFn drawfn[8];

// Older Kick Vertex functions.
void __forceinline KICK_VERTEX2();
void __forceinline KICK_VERTEX3();

// Newer version.
extern void __forceinline KickVertex(bool adc);

extern bool __forceinline NoHighlights(int i);

extern void init_drawfn();
extern void clear_drawfn();

extern void KickPoint();
extern void KickLine();
extern void KickTriangle();
extern void KickTriangleFan();
extern void KickSprite();
extern void KickDummy();


#endif // ZZKICK_H_INCLUDED
