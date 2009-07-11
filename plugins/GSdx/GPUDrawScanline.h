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
#include "GPUScanlineEnvironment.h"
#include "GPUSetupPrimCodeGenerator.h"
#include "GPUDrawScanlineCodeGenerator.h"

class GPUDrawScanline : public IDrawScanline
{
	GPUScanlineEnvironment m_env;

	//

	class GPUSetupPrimMap : public GSCodeGeneratorFunctionMap<GPUSetupPrimCodeGenerator, uint32, SetupPrimStaticPtr>
	{
		GPUScanlineEnvironment& m_env;

	public:
		GPUSetupPrimMap(GPUScanlineEnvironment& env);
		GPUSetupPrimCodeGenerator* Create(uint32 key, void* ptr, size_t maxsize);
	} m_sp;

	//

	class GPUDrawScanlineMap : public GSCodeGeneratorFunctionMap<GPUDrawScanlineCodeGenerator, uint32, DrawScanlineStaticPtr>
	{
		GPUScanlineEnvironment& m_env;

	public:
		GPUDrawScanlineMap(GPUScanlineEnvironment& env);
		GPUDrawScanlineCodeGenerator* Create(uint32 key, void* ptr, size_t maxsize);
	} m_ds;

protected:
	GPUState* m_state;
	int m_id;

public:
	GPUDrawScanline(GPUState* state, int id);
	virtual ~GPUDrawScanline();

	// IDrawScanline

	void BeginDraw(const GSRasterizerData* data, Functions* f);
	void EndDraw(const GSRasterizerStats& stats);
	void PrintStats() {m_ds.PrintStats();}
};
