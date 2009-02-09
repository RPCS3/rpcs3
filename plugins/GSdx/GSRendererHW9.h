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
#include "GSTextureCache9.h"
#include "GSTextureFX9.h"

class GSRendererHW9 : public GSRendererHW<GSDevice9, GSVertexHW9, GSTextureCache9>
{
	typedef GSDevice9 Device;
	typedef GSVertexHW9 Vertex;
	typedef GSTextureCache9 TextureCache;

	bool WrapZ(float maxz);

protected:
	GSTextureFX9 m_tfx;
	bool m_logz;

	void Draw(int prim, Texture& rt, Texture& ds, GSTextureCache<Device>::GSTexture* tex);

	struct
	{
		Direct3DDepthStencilState9 dss;
		Direct3DBlendState9 bs;
	} m_date;

	struct
	{
		bool enabled;
		Direct3DDepthStencilState9 dss;
		Direct3DBlendState9 bs;
	} m_fba;

	void SetupDATE(Texture& rt, Texture& ds);
	void UpdateFBA(Texture& rt);

public:
	GSRendererHW9(BYTE* base, bool mt, void (*irq)(), int nloophack, const GSRendererSettings& rs);

	bool Create(LPCTSTR title);

	template<DWORD prim, DWORD tme, DWORD fst> void VertexKick(bool skip);
};
