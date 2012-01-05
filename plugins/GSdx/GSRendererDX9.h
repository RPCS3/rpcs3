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
#include "GSTextureCache9.h"

class GSRendererDX9 : public GSRendererDX
{
protected:
	struct
	{
		Direct3DDepthStencilState9 dss;
		Direct3DBlendState9 bs;
	} m_fba;

	template<uint32 prim, uint32 tme, uint32 fst>
	void ConvertVertex(GSVertexHW9* RESTRICT vertex, size_t index);
	void Draw();
	void DrawPrims(GSTexture* rt, GSTexture* ds, GSTextureCache::Source* tex);
	void UpdateFBA(GSTexture* rt);

	int GetPosX(const void* vertex) const {return (int)((const GSVertexHW9*)vertex)->p.x;}
	int GetPosY(const void* vertex) const {return (int)((const GSVertexHW9*)vertex)->p.y;}
	uint32 GetColor(const void* vertex) const {return ((const GSVertexHW9*)vertex)->t.u32[2];}
	void SetColor(void* vertex, uint32 c) const {((GSVertexHW9*)vertex)->t.u32[2] = c;}

public:
	GSRendererDX9();
	virtual ~GSRendererDX9() {}

	bool CreateDevice(GSDevice* dev);
};
