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

#include "GS.h"
#include "GSVertexSW.h"
#include "GSFunctionMap.h"
#include "GSThread.h"
#include "GSAlignedClass.h"

__aligned(class, 32) GSRasterizerData
{
public:
	GSVector4i scissor;
	GS_PRIM_CLASS primclass;
	const GSVertexSW* vertices;
	int count;
	uint64 frame;
	const void* param;
};

class IDrawScanline : public GSAlignedClass<32>
{
public:
	typedef void (__fastcall *SetupPrimPtr)(const GSVertexSW* vertices, const GSVertexSW& dscan);
	typedef void (__fastcall *DrawScanlinePtr)(int pixels, int left, int top, const GSVertexSW& scan);
	typedef void (IDrawScanline::*DrawRectPtr)(const GSVector4i& r, const GSVertexSW& v); // TODO: jit

protected:
	SetupPrimPtr m_sp;
	DrawScanlinePtr m_ds;
	DrawScanlinePtr m_de;
	DrawRectPtr m_dr;

public:
	IDrawScanline() : m_sp(NULL), m_ds(NULL), m_de(NULL), m_dr(NULL) {}
	virtual ~IDrawScanline() {}

	virtual void BeginDraw(const void* param) = 0;
	virtual void EndDraw(const GSRasterizerStats& stats, uint64 frame) = 0;
	virtual void PrintStats() = 0;

	__forceinline void SetupPrim(const GSVertexSW* vertices, const GSVertexSW& dscan) {m_sp(vertices, dscan);}
	__forceinline void DrawScanline(int right, int left, int top, const GSVertexSW& scan) {m_ds(right, left, top, scan);}
	__forceinline void DrawEdge(int right, int left, int top, const GSVertexSW& scan) {m_de(right, left, top, scan);}
	__forceinline void DrawRect(const GSVector4i& r, const GSVertexSW& v) {(this->*m_dr)(r, v);}

	__forceinline bool IsEdge() const {return m_de != NULL;}
	__forceinline bool IsRect() const {return m_dr != NULL;}
};

class IRasterizer
{
public:
	virtual ~IRasterizer() {}

	virtual void Draw(const GSRasterizerData* data) = 0;
	virtual void GetStats(GSRasterizerStats& stats) = 0;
	virtual void PrintStats() = 0;
	virtual void SetThreadId(int id, int threads) = 0;
};

__aligned(class, 32) GSRasterizer : public GSAlignedClass<32>, public IRasterizer
{
protected:
	IDrawScanline* m_ds;
	int m_id;
	int m_threads;
	GSRasterizerStats m_stats;
	GSVector4i m_scissor;
	GSVector4 m_fscissor;
	struct {GSVertexSW* buff; int count;} m_edge;

	void DrawPoint(const GSVertexSW* v);
	void DrawLine(const GSVertexSW* v);
	void DrawTriangle(const GSVertexSW* v);
	void DrawSprite(const GSVertexSW* v);
	void DrawEdge(const GSVertexSW* v);

	__forceinline void DrawTriangleSection(int top, int bottom, GSVertexSW& l, const GSVertexSW& dl, const GSVertexSW& dscan);

	void DrawEdge(const GSVertexSW& v0, const GSVertexSW& v1, const GSVertexSW& dv, int orientation, int side);

	__forceinline bool IsOneOfMyScanlines(int scanline) const;

	__forceinline void Flush(const GSVertexSW* vertices, const GSVertexSW& dscan, bool edge = false);

public:
	GSRasterizer(IDrawScanline* ds);
	virtual ~GSRasterizer();

	// IRasterizer

	void Draw(const GSRasterizerData* data);
	void GetStats(GSRasterizerStats& stats);
	void PrintStats() {m_ds->PrintStats();}
	void SetThreadId(int id, int threads) {m_id = id; m_threads = threads;}
};

class GSRasterizerMT : public GSRasterizer, private GSThread
{
protected:
	volatile long& m_sync;
	GSAutoResetEvent m_draw;
	const GSRasterizerData* m_data;

	void ThreadProc();

public:
	GSRasterizerMT(IDrawScanline* ds, volatile long& sync);
	virtual ~GSRasterizerMT();

	// IRasterizer

	void Draw(const GSRasterizerData* data);
};

class GSRasterizerList : protected vector<IRasterizer*>
{
protected:
	volatile long m_sync;
	GSRasterizerStats m_stats;
	int64 m_start;
	int m_threads;

public:
	GSRasterizerList();
	virtual ~GSRasterizerList();

	template<class DS> void Create(int threads)
	{
		threads = std::max<int>(threads, 1); // TODO: min(threads, number of cpu cores)

		push_back(new GSRasterizer(new DS()));

		for(int i = 1; i < threads; i++)
		{
			push_back(new GSRasterizerMT(new DS(), m_sync));
		}
	}

	void Sync();

	void Draw(const GSRasterizerData* data, int width, int height);
	void GetStats(GSRasterizerStats& stats);
	void PrintStats();
};
