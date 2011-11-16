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

#include "GSRendererOGL.h"
#include "GSRenderer.h"


GSRendererOGL::GSRendererOGL()
	// FIXME
	//: GSRendererHW<GSVertexHWOGL>(new GSTextureCacheOGL(this))
	: GSRendererHW<GSVertexHW11>(new GSTextureCacheOGL(this))
{
	// TODO must be implementer with macro InitVertexKick(GSRendererOGL)
	// template<uint32 prim, uint32 tme, uint32 fst> void VertexKick(bool skip);
	InitVertexKick(GSRendererOGL);
}

GSRendererOGL::~GSRendererOGL() { /* TODO */ }

bool GSRendererOGL::CreateDevice(GSDevice* dev) { /* TODO */ }

template<uint32 prim, uint32 tme, uint32 fst>
void GSRendererOGL::VertexKick(bool skip) { /* TODO */ }

void GSRendererOGL::Draw(GSTexture* rt, GSTexture* ds, GSTextureCache::Source* tex) { /* TODO */ }
