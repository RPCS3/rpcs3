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

#include "pthread.h"
#include "semaphore.h"

__aligned16 class GSRasterizerData
{
public:
	GSVector4i scissor;
	GS_PRIM_CLASS primclass;
	const GSVertexSW* vertices;
	int count;
	const void* param;
};

class IRasterizer
{
public:
	virtual ~IRasterizer() {}

	virtual void Draw(const GSRasterizerData* data) = 0;
	virtual void GetStats(GSRasterizerStats& stats) = 0;
	virtual void PrintStats() = 0;
};

class IDrawScanline : public GSAlignedClass<16>
{
public:
	typedef void (__fastcall *DrawScanlineStaticPtr)(int right, int left, int top, const GSVertexSW& v);
	typedef void (__fastcall *SetupPrimStaticPtr)(const GSVertexSW* vertices, const GSVertexSW& dscan);
	typedef void (IDrawScanline::*DrawSolidRectPtr)(const GSVector4i& r, const GSVertexSW& v);

	struct Functions
	{
		DrawScanlineStaticPtr ssl;
		DrawScanlineStaticPtr ssle;
		SetupPrimStaticPtr ssp;
		DrawSolidRectPtr sr; // TODO
	};

	virtual ~IDrawScanline() {}

	virtual void BeginDraw(const GSRasterizerData* data, Functions* dsf) = 0;
	virtual void EndDraw(const GSRasterizerStats& stats) = 0;
	virtual void PrintStats() = 0;
};

class GSRasterizer : public IRasterizer
{
protected:
	IDrawScanline* m_ds;
	IDrawScanline::Functions m_dsf;
	int m_id;
	int m_threads;
	GSRasterizerStats m_stats;

	void DrawPoint(const GSVertexSW* v, const GSVector4i& scissor);
	void DrawLine(const GSVertexSW* v, const GSVector4i& scissor);
	void DrawTriangle(const GSVertexSW* v, const GSVector4i& scissor);
	void DrawTriangleEdge(const GSVertexSW* v, const GSVector4i& scissor);
	void DrawSprite(const GSVertexSW* v, const GSVector4i& scissor);

	void DrawTriangleTop(GSVertexSW* v, const GSVector4i& scissor);
	void DrawTriangleBottom(GSVertexSW* v, const GSVector4i& scissor);
	void DrawTriangleTopBottom(GSVertexSW* v, const GSVector4i& scissor);

	__forceinline void DrawTriangleSection(int top, int bottom, GSVertexSW& l, const GSVertexSW& dl, GSVector4& r, const GSVector4& dr, const GSVertexSW& dscan, const GSVector4& scissor);
	__forceinline void DrawTriangleSection(int top, int bottom, GSVertexSW& l, const GSVertexSW& dl, const GSVertexSW& dscan, const GSVector4& scissor);

	void DrawEdge(const GSVertexSW& v0, const GSVertexSW& v1, const GSVertexSW& dv, const GSVector4i& scissor, int orientation, int side);

	inline bool IsOneOfMyScanlines(int scanline) const;

public:
	GSRasterizer(IDrawScanline* ds, int id = 0, int threads = 0);
	virtual ~GSRasterizer();

	// IRasterizer

	void Draw(const GSRasterizerData* data);
	void GetStats(GSRasterizerStats& stats);
	void PrintStats() {m_ds->PrintStats();}
};

class GSRasterizerMT : public GSRasterizer, private GSThread
{
protected:
	sem_t& m_finished;
	volatile long& m_sync;
	sem_t m_semaphore;
	sem_t m_stopped;
	bool m_exit;
	const GSRasterizerData* m_data;

	void ThreadProc();

public:
	GSRasterizerMT(IDrawScanline* ds, int id, int threads, sem_t& finished, volatile long& sync);
	virtual ~GSRasterizerMT();

	// IRasterizer

	void Draw(const GSRasterizerData* data);
};

class GSRasterizerList : protected vector<IRasterizer*>, public IRasterizer
{
protected:
	int m_threadcount;
	sem_t m_finished;
	volatile long m_sync;
	long m_syncstart;
	GSRasterizerStats m_stats;
	void FreeRasterizers();

public:
	GSRasterizerList();
	virtual ~GSRasterizerList();

	template<class DS, class T> void Create(T* parent, int threads)
	{
		FreeRasterizers();

		threads = max(threads, 1); // TODO: min(threads, number of cpu cores)

		push_back(new GSRasterizer(new DS(parent, 0), 0, threads));

		m_syncstart = 0;
		for(int i = 1; i < threads; i++)
		{
			push_back(new GSRasterizerMT(new DS(parent, i), i, threads, m_finished, m_sync));
			_interlockedbittestandset(&m_syncstart, i);
		}
	}

	// IRasterizer

	void Draw(const GSRasterizerData* data);
	void GetStats(GSRasterizerStats& stats);
	void PrintStats();
};
