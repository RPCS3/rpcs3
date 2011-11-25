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

#include "GSState.h"
#include "GSRasterizer.h"
#include "GSScanlineEnvironment.h"
#include "GSSetupPrimCodeGenerator.h"
#include "GSDrawScanlineCodeGenerator.h"

class GSDrawScanline : public IDrawScanline
{
	GSScanlineGlobalData m_global;
	GSScanlineLocalData m_local;

	GSCodeGeneratorFunctionMap<GSSetupPrimCodeGenerator, uint64, SetupPrimPtr> m_sp_map;
	GSCodeGeneratorFunctionMap<GSDrawScanlineCodeGenerator, uint64, DrawScanlinePtr> m_ds_map;

	template<class T, bool masked>
	void DrawRectT(const int* RESTRICT row, const int* RESTRICT col, const GSVector4i& r, uint32 c, uint32 m);

	template<class T, bool masked>
	__forceinline void FillRect(const int* RESTRICT row, const int* RESTRICT col, const GSVector4i& r, uint32 c, uint32 m);

	template<class T, bool masked>
	__forceinline void FillBlock(const int* RESTRICT row, const int* RESTRICT col, const GSVector4i& r, const GSVector4i& c, const GSVector4i& m);

public:
	GSDrawScanline();
	virtual ~GSDrawScanline();

	// IDrawScanline

	void BeginDraw(const void* param);
	void EndDraw(const GSRasterizerStats& stats, uint64 frame);
	void PrintStats();

	void DrawRect(const GSVector4i& r, const GSVertexSW& v);

#ifndef JIT_DRAW
	
	void SetupPrim(const GSVertexSW* vertices, const GSVertexSW& dscan);
	void DrawScanline(int pixels, int left, int top, const GSVertexSW& scan);
	void DrawEdge(int pixels, int left, int top, const GSVertexSW& scan);

	bool IsEdge() const {return m_global.sel.aa1;}
	bool IsRect() const {return m_global.sel.IsSolidRect();}

	bool TestAlpha(GSVector4i& test, GSVector4i& fm, GSVector4i& zm, const GSVector4i& ga);
	void WritePixel(const GSVector4i& src, int addr, int i, uint32 psm);

#endif
};
