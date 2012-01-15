/*
 *	Copyright (C) 2011-2011 Gregory hainaut
 *	Copyright (C) 2007-2009 Gabest
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#pragma once

#include "GSRendererHW.h"

#include "GSRenderer.h"
#include "GSTextureCacheOGL.h"
#include "GSVertexHW.h"

// FIXME does it need a GSVertexHWOGL ??? Data order can be easily programmed on opengl (the only potential
// issue is the unsupported praga push/pop
// Note it impact GSVertexTrace.cpp => void GSVertexTrace::Update(const GSVertexHWOGL* v, int count, GS_PRIM_CLASS primclass)
class GSRendererOGL : public GSRendererHW
//class GSRendererOGL : public GSRendererHW<GSVertexHWOGL>
{
	private:
		GSVector2 m_pixelcenter;
		bool m_logz;
		bool m_fba;
		bool UserHacks_AlphaHack;

	protected:
		GLenum m_topology;

		template<uint32 prim, uint32 tme, uint32 fst>
		void ConvertVertex(size_t dst_index, size_t src_index);

		int GetPosX(const void* vertex) const {return (int)((const GSVertexHW11*)vertex)->p.x;}
		int GetPosY(const void* vertex) const {return (int)((const GSVertexHW11*)vertex)->p.y;}
		uint32 GetColor(const void* vertex) const {return ((const GSVertexHW11*)vertex)->c0;}
		void SetColor(void* vertex, uint32 c) const {((GSVertexHW11*)vertex)->c0 = c;}

	public:
		GSRendererOGL();
		virtual ~GSRendererOGL() {};

		template<uint32 prim, uint32 tme, uint32 fst> void VertexKick(bool skip);

		bool CreateDevice(GSDevice* dev);

		void UpdateFBA(GSTexture* rt) {}

		void DrawPrims(GSTexture* rt, GSTexture* ds, GSTextureCache::Source* tex);
};
