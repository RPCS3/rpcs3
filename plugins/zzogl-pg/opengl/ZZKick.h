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

#include "Util.h"
#include "GS.h"

extern bool __forceinline NoHighlights(int i);

enum PRIM_TYPE {
    PRIM_POINT = 0,
    PRIM_LINE,
    PRIM_LINE_STRIP,
    PRIM_TRIANGLE,
    PRIM_TRIANGLE_STRIP,
    PRIM_TRIANGLE_FAN,
    PRIM_SPRITE,
    PRIM_DUMMY
};

class Kick
{
	private:
        // template<bool DO_Z_FOG> void Set_Vertex(VertexGPU *p, int i);
        template<bool DO_Z_FOG> void Set_Vertex(VertexGPU *p, Vertex &gsvertex);
		void Output_Vertex(VertexGPU vert, u32 id);
        bool ValidPrevPrim;
	public:
		Kick() { }
		~Kick() { }
		
		void KickVertex(bool adc);
		void DrawPrim(u32 i);

        inline void DirtyValidPrevPrim() {
            ValidPrevPrim = 0;
        }
};
extern Kick* ZZKick;

#endif // ZZKICK_H_INCLUDED
