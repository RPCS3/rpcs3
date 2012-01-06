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

#include "GSRenderer.h"

class GSRendererNull : public GSRenderer
{
	class GSVertexTraceNull : public GSVertexTrace
	{
	public:
		GSVertexTraceNull(const GSState* state) : GSVertexTrace(state) {}
	};

protected:
	template<uint32 prim, uint32 tme, uint32 fst> 
	void ConvertVertex(size_t dst_index, size_t src_index)
	{
	}

	void Draw()
	{
	}

	GSTexture* GetOutput(int i) 
	{
		return NULL;
	}

public:
	GSRendererNull() 
		: GSRenderer(new GSVertexTraceNull(this), sizeof(GSVertex)) 
	{
		InitConvertVertex(GSRendererNull);
	}
};
