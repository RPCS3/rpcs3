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

#include "GSRendererHW.h"
#include "GSVertexHW.h"
#include "GSTextureCache10.h"
#include "GSTextureFX10.h"

class GSRendererHW10 : public GSRendererHW<GSVertexHW10>
{
protected:
	GSTextureFX10 m_tfx;

	void Draw(GS_PRIM_CLASS primclass, GSTexture* rt, GSTexture* ds, GSTextureCache::GSCachedTexture* tex);

	struct
	{
		CComPtr<ID3D10DepthStencilState> dss;
		CComPtr<ID3D10BlendState> bs;
	} m_date;

	void SetupDATE(GSTexture* rt, GSTexture* ds);

public:
	GSRendererHW10(uint8* base, bool mt, void (*irq)());

	bool Create(const string& title);

	template<uint32 prim, uint32 tme, uint32 fst> void VertexKick(bool skip);
};