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
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA USA. 
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#pragma once

#include "GPUState.h"
#include "GSRasterizer.h"
#include "GPUScanlineEnvironment.h"
#include "GPUSetupPrimCodeGenerator.h"
#include "GPUDrawScanlineCodeGenerator.h"

class GPUDrawScanline : public IDrawScanline
{
public:
	class SharedData : public GSRasterizerData
	{
	public:
		GPUScanlineGlobalData global;

	public:
		SharedData()
		{
			global.clut = NULL;
		}

		virtual ~SharedData()
		{
			if(global.clut) _aligned_free(global.clut);
		}
	};

protected:
	GPUScanlineGlobalData m_global;
	GPUScanlineLocalData m_local;

	GSCodeGeneratorFunctionMap<GPUSetupPrimCodeGenerator, uint32, SetupPrimPtr> m_sp_map;
	GSCodeGeneratorFunctionMap<GPUDrawScanlineCodeGenerator, uint32, DrawScanlinePtr> m_ds_map;

public:
	GPUDrawScanline();
	virtual ~GPUDrawScanline();

	// IDrawScanline

	void BeginDraw(const GSRasterizerData* data);
	void EndDraw(uint64 frame, uint64 ticks, int actual, int total);

#ifndef ENABLE_JIT_RASTERIZER

	void SetupPrim(const GSVertexSW* vertex, const uint32* index, const GSVertexSW& dscan);
	void DrawScanline(int pixels, int left, int top, const GSVertexSW& scan);
	void DrawEdge(int pixels, int left, int top, const GSVertexSW& scan);
	void DrawRect(const GSVector4i& r, const GSVertexSW& v);

#endif

	void PrintStats() {m_ds_map.PrintStats();}
};
