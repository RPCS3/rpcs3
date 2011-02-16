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

__aligned32 class GSRasterizerData
{
public:
	GSVector4i scissor;
	GS_PRIM_CLASS primclass;
	const GSVertexSW* vertices;
	int count;
	const void* param;
};
	
class IDrawScanline : public GSAlignedClass<32>
{
public:
	typedef void (__fastcall *SetupPrimPtr)(const GSVertexSW* vertices, const GSVertexSW& dscan);
	typedef void (__fastcall *DrawScanlinePtr)(int right, int left, int top, const GSVertexSW& scan);
	typedef void (IDrawScanline::*DrawRectPtr)(const GSVector4i& r, const GSVertexSW& v); // TODO: jit

protected:
	SetupPrimPtr m_sp;
	DrawScanlinePtr m_ds;
	DrawScanlinePtr m_de;
	DrawRectPtr m_dr;

public:
	IDrawScanline() : m_sp(NULL), m_ds(NULL), m_de(NULL), m_dr(NULL) {}
	virtual ~IDrawScanline() {}

	virtual void BeginDraw(const GSRasterizerData* data) = 0;
	virtual void EndDraw(const GSRasterizerStats& stats) = 0;
	virtual void PrintStats() = 0;

	__forceinline void SetupPrim(const GSVertexSW* v, const GSVertexSW& dscan) {m_sp(v, dscan);}
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
};

class GSRasterizer : public IRasterizer
{
protected:
	IDrawScanline* m_ds;
	int m_id;
	int m_threads;
	GSRasterizerStats m_stats;

	void DrawPoint(const GSVertexSW* v, const GSVector4i& scissor);
	void DrawLine(const GSVertexSW* v, const GSVector4i& scissor);
	void DrawTriangle(const GSVertexSW* v, const GSVector4i& scissor);
	void DrawEdge(const GSVertexSW* v, const GSVector4i& scissor);
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
	volatile long& m_sync;
	HANDLE m_exit;
	HANDLE m_draw;
	HANDLE m_ready;
	const GSRasterizerData* m_data;

	void ThreadProc();

public:
	GSRasterizerMT(IDrawScanline* ds, int id, int threads, HANDLE ready, volatile long& sync);
	virtual ~GSRasterizerMT();

	// IRasterizer

	void Draw(const GSRasterizerData* data);
};

class GSRasterizerList : protected vector<IRasterizer*>, public IRasterizer
{
protected:
	std::vector<HANDLE> m_ready;
	volatile long m_sync;
	long m_syncstart;
	GSRasterizerStats m_stats;
	int64 m_start;

	void FreeRasterizers();

public:
	GSRasterizerList();
	virtual ~GSRasterizerList();

	template<class DS, class T> void Create(T* parent, int threads)
	{
		FreeRasterizers();

		threads = std::max<int>(threads, 1); // TODO: min(threads, number of cpu cores)

		m_syncstart = 0;

		push_back(new GSRasterizer(new DS(parent, 0), 0, threads));

		for(int i = 1; i < threads; i++)
		{
			HANDLE ready = CreateEvent(NULL, FALSE, FALSE, NULL);

			push_back(new GSRasterizerMT(new DS(parent, i), i, threads, ready, m_sync));

			m_ready.push_back(ready);

			_interlockedbittestandset(&m_syncstart, i);
		}
	}

	void Sync();

	// IRasterizer

	void Draw(const GSRasterizerData* data);
	void GetStats(GSRasterizerStats& stats);
	void PrintStats();
};
