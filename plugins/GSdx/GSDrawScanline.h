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
#include "GSAlignedClass.h"

union GSScanlineSelector
{
	struct
	{
		DWORD fpsm:2; // 0
		DWORD zpsm:2; // 2
		DWORD ztst:2; // 4 (0: off, 1: write, 2: test (ge), 3: test (g))
		DWORD atst:3; // 6
		DWORD afail:2; // 9
		DWORD iip:1; // 11
		DWORD tfx:3; // 12
		DWORD tcc:1; // 15
		DWORD fst:1; // 16
		DWORD ltf:1; // 17
		DWORD tlu:1; // 18
		DWORD fge:1; // 19
		DWORD date:1; // 20
		DWORD abea:2; // 21
		DWORD abeb:2; // 23
		DWORD abec:2; // 25
		DWORD abed:2; // 27
		DWORD pabe:1; // 29
		DWORD rfb:1; // 30
		DWORD sprite:1; // 31
	};

	struct
	{
		DWORD _pad1:21;
		DWORD abe:8;
		DWORD _pad2:3;
	};

	DWORD dw;

	operator DWORD() {return dw;}

	bool IsSolidRect()
	{
		return sprite
			&& iip == 0
			&& tfx == TFX_NONE
			&& abe == 255 
			&& ztst <= 1 
			&& atst <= 1
			&& date == 0
			&& fge == 0;
	}
};

__declspec(align(16)) struct GSScanlineEnvironment
{
	GSScanlineSelector sel;

	void* vm;
	const void* tex;
	const DWORD* clut;
	DWORD tw;

	GSVector4i* fbr;
	GSVector4i* zbr;
	int** fbc;
	int** zbc;
	GSVector2i* fzbr;
	GSVector2i* fzbc;

	GSVector4i fm, zm;
	struct {GSVector4i min, max, mask;} t; // [u] x 4 [v] x 4
	GSVector4i datm;
	GSVector4i colclamp;
	GSVector4i fba;
	GSVector4i aref;
	GSVector4i afix, afix2;
	GSVector4i frb, fga;

	struct {GSVector4 z, s, t, q; GSVector4i rb, ga, f, si, ti, _pad[3];} d[4];
	struct {GSVector4 z, stq; GSVector4i c, f, st;} d4;
	struct {GSVector4i rb, ga;} c;
	struct {GSVector4i z, f;} p;
	struct {GSVector4i rb, ga;} c2;
};

__declspec(align(16)) struct GSScanlineParam
{
	GSScanlineSelector sel;

	void* vm;
	const void* tex;
	const DWORD* clut;
	DWORD tw;

	GSLocalMemory::Offset* fbo;
	GSLocalMemory::Offset* zbo;
	GSLocalMemory::Offset4* fzbo;

	DWORD fm, zm;
};

class GSDrawScanline : public GSAlignedClass<16>, public IDrawScanline
{
	GSScanlineEnvironment m_env;

	static const GSVector4 m_shift[4];
	static const GSVector4i m_test[8];

	//

	class GSDrawScanlineMap : public GSFunctionMap<DrawScanlinePtr>
	{
		DrawScanlinePtr m_default[4][4][4][2];

	public:
		GSDrawScanlineMap();

		DrawScanlinePtr GetDefaultFunction(DWORD dw);

		void PrintStats();
	};
	
	GSDrawScanlineMap m_ds;

	//

	class GSSetupPrimMap : public GSFunctionMap<SetupPrimPtr>
	{
		SetupPrimPtr m_default[2][2][2][2][2];

	public:
		GSSetupPrimMap();

		SetupPrimPtr GetDefaultFunction(DWORD dw);
	};
	
	GSSetupPrimMap m_sp;

	//

	template<DWORD zbe, DWORD fge, DWORD tme, DWORD fst, DWORD iip>
	void SetupPrim(const GSVertexSW* vertices, const GSVertexSW& dscan);

	//

	__forceinline GSVector4i Wrap(const GSVector4i& t);

	__forceinline void SampleTexture(DWORD ltf, DWORD tlu, const GSVector4i& u, const GSVector4i& v, GSVector4i* c);
	__forceinline void ColorTFX(DWORD iip, DWORD tfx, const GSVector4i& rbf, const GSVector4i& gaf, GSVector4i& rbt, GSVector4i& gat);
	__forceinline void AlphaTFX(DWORD iip, DWORD tfx, DWORD tcc, const GSVector4i& gaf, GSVector4i& gat);
	__forceinline void Fog(DWORD fge, const GSVector4i& f, GSVector4i& rb, GSVector4i& ga);
	__forceinline bool TestZ(DWORD zpsm, DWORD ztst, const GSVector4i& zs, const GSVector4i& zd, GSVector4i& test);
	__forceinline bool TestAlpha(DWORD atst, DWORD afail, const GSVector4i& ga, GSVector4i& fm, GSVector4i& zm, GSVector4i& test);
	__forceinline bool TestDestAlpha(DWORD fpsm, DWORD date, const GSVector4i& fd, GSVector4i& test);

	__forceinline void ReadPixel(int psm, int addr, GSVector4i& c) const;
	__forceinline static void WritePixel(int psm, WORD* RESTRICT vm16, DWORD c);
	__forceinline void WriteFrame(int fpsm, int rfb, GSVector4i* c, const GSVector4i& fd, const GSVector4i& fm, int addr, int fzm);
	__forceinline void WriteZBuf(int zpsm, int ztst, const GSVector4i& z, const GSVector4i& zd, const GSVector4i& zm, int addr, int fzm);

	template<DWORD fpsm, DWORD zpsm, DWORD ztst, DWORD iip>
	void DrawScanline(int top, int left, int right, const GSVertexSW& v);

	template<DWORD sel>
	void DrawScanlineEx(int top, int left, int right, const GSVertexSW& v);

	//

	void DrawSolidRect(const GSVector4i& r, const GSVertexSW& v);

	template<class T, bool masked> 
	void DrawSolidRectT(const GSVector4i* row, int* col, const GSVector4i& r, DWORD c, DWORD m);

	template<class T, bool masked> 
	__forceinline void FillRect(const GSVector4i* row, int* col, const GSVector4i& r, DWORD c, DWORD m);

	template<class T, bool masked> 
	__forceinline void FillBlock(const GSVector4i* row, int* col, const GSVector4i& r, const GSVector4i& c, const GSVector4i& m);

protected:
	GSState* m_state;
	int m_id;

public:
	GSDrawScanline(GSState* state, int id);
	virtual ~GSDrawScanline();

	// IDrawScanline

	void BeginDraw(const GSRasterizerData* data, Functions* f);
	void EndDraw(const GSRasterizerStats& stats);
	void PrintStats() {m_ds.PrintStats();}
};
