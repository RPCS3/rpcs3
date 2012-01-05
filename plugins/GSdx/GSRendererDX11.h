/*
 *	Copyright (C) 2007-2009 Gabest
 *	http://www.gabest.org
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

#include "GSRendererDX.h"
#include "GSVertexHW.h"
#include "GSTextureCache11.h"

class GSRendererDX11 : public GSRendererDX
{
protected:
	template<uint32 prim, uint32 tme, uint32 fst>
	void ConvertVertex(GSVertexHW11* RESTRICT vertex, size_t index);
	void Draw();
	void DrawPrims(GSTexture* rt, GSTexture* ds, GSTextureCache::Source* tex);

	int GetPosX(const void* vertex) const {return (int)((const GSVertexHW11*)vertex)->p.x;}
	int GetPosY(const void* vertex) const {return (int)((const GSVertexHW11*)vertex)->p.y;}
	uint32 GetColor(const void* vertex) const {return ((const GSVertexHW11*)vertex)->c0;}
	void SetColor(void* vertex, uint32 c) const {((GSVertexHW11*)vertex)->c0 = c;}

public:
	GSRendererDX11();
	virtual ~GSRendererDX11() {}

	bool CreateDevice(GSDevice* dev);
};
