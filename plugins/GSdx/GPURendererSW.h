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

#include "GPURenderer.h"
#include "GPUDrawScanline.h"

class GPURendererSW : public GPURendererT<GSVertexSW>
{
	class GPURasterizerData : public GSRasterizerData
	{
	public:
		GPURasterizerData()
		{
			GPUScanlineGlobalData* gd = (GPUScanlineGlobalData*)_aligned_malloc(sizeof(GPUScanlineGlobalData), 32);

			gd->clut = NULL;

			param = gd;
		}

		virtual ~GPURasterizerData()
		{
			GPUScanlineGlobalData* gd = (GPUScanlineGlobalData*)param;

			if(gd->clut) _aligned_free(gd->clut);

			_aligned_free(gd);
		}
	};

protected:
	IRasterizer* m_rl;
	GSTexture* m_texture;
	uint32* m_output;

	void ResetDevice();
	GSTexture* GetOutput();
	void VertexKick();
	void Draw();

public:
	GPURendererSW(GSDevice* dev, int threads);
	virtual ~GPURendererSW();
};
