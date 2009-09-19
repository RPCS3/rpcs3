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
#include "GSTextureCache10.h"

class GSRendererDX10 : public GSRendererDX<GSVertexHW10>
{
protected:
	struct
	{
		CComPtr<ID3D10DepthStencilState> dss;
		CComPtr<ID3D10BlendState> bs;
	} m_date;

	void Draw(GSTexture* rt, GSTexture* ds, GSTextureCache::Source* tex);
	void SetupDATE(GSTexture* rt, GSTexture* ds);

public:
	GSRendererDX10();

	bool CreateDevice(GSDevice* dev);

	template<uint32 prim, uint32 tme, uint32 fst> void VertexKick(bool skip);
};