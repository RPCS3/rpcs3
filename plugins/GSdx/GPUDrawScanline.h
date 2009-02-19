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

#include "GPUState.h"
#include "GSRasterizer.h"
#include "GSAlignedClass.h"
#include "GPUScanlineEnvironment.h"
#include "GPUDrawScanlineCodeGenerator.h"

class GPUDrawScanline : public GSAlignedClass<16>, public IDrawScanline
{
	GPUScanlineEnvironment m_env;

	//

	class GPUSetupPrimMap : public GSFunctionMap<DWORD, SetupPrimPtr>
	{
		SetupPrimPtr m_default[2][2][2];

	public:
		GPUSetupPrimMap();

		SetupPrimPtr GetDefaultFunction(DWORD key);
	};
	
	GPUSetupPrimMap m_sp;

	template<DWORD sprite, DWORD tme, DWORD iip>
	void SetupPrim(const GSVertexSW* vertices, const GSVertexSW& dscan);

	//

	class GPUDrawScanlineMap : public GSCodeGeneratorFunctionMap<GPUDrawScanlineCodeGenerator, DWORD, DrawScanlineStaticPtr>
	{
		GPUScanlineEnvironment& m_env;

	public:
		GPUDrawScanlineMap(GPUScanlineEnvironment& env);
		GPUDrawScanlineCodeGenerator* Create(DWORD key, void* ptr, size_t maxsize);
	} m_ds;

	void DrawScanline(int top, int left, int right, const GSVertexSW& v);

protected:
	GPUState* m_state;
	int m_id;

public:
	GPUDrawScanline(GPUState* state, int id);
	virtual ~GPUDrawScanline();

	// IDrawScanline

	void BeginDraw(const GSRasterizerData* data, Functions* f);
	void EndDraw(const GSRasterizerStats& stats) {}
	void PrintStats() {}
};
