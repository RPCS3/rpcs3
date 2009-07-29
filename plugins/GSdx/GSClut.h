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
#include "GSVector.h"
#include "GSTables.h"
#include "GSAlignedClass.h"

class GSLocalMemory;

__declspec(align(16)) class GSClut : public GSAlignedClass<16>
{
	GSLocalMemory* m_mem;

	uint32 m_CBP[2];
	uint16* m_clut;
	uint32* m_buff32;
	uint64* m_buff64;

	__declspec(align(16)) struct WriteState
	{
		GIFRegTEX0 TEX0;
		GIFRegTEXCLUT TEXCLUT;
		bool dirty;
		bool IsDirty(const GIFRegTEX0& TEX0, const GIFRegTEXCLUT& TEXCLUT);
	} m_write;

	__declspec(align(16)) struct ReadState
	{
		GIFRegTEX0 TEX0;
		GIFRegTEXA TEXA;
		bool dirty;
		bool adirty;
		int amin, amax;
		bool IsDirty(const GIFRegTEX0& TEX0);
		bool IsDirty(const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA);
	} m_read;

	typedef void (GSClut::*writeCLUT)(const GIFRegTEX0& TEX0, const GIFRegTEXCLUT& TEXCLUT);

	writeCLUT m_wc[2][16][64];

	void WriteCLUT32_I8_CSM1(const GIFRegTEX0& TEX0, const GIFRegTEXCLUT& TEXCLUT);
	void WriteCLUT32_I4_CSM1(const GIFRegTEX0& TEX0, const GIFRegTEXCLUT& TEXCLUT);
	void WriteCLUT16_I8_CSM1(const GIFRegTEX0& TEX0, const GIFRegTEXCLUT& TEXCLUT);
	void WriteCLUT16_I4_CSM1(const GIFRegTEX0& TEX0, const GIFRegTEXCLUT& TEXCLUT);
	void WriteCLUT16S_I8_CSM1(const GIFRegTEX0& TEX0, const GIFRegTEXCLUT& TEXCLUT);
	void WriteCLUT16S_I4_CSM1(const GIFRegTEX0& TEX0, const GIFRegTEXCLUT& TEXCLUT);

	template<int n> void WriteCLUT32_CSM2(const GIFRegTEX0& TEX0, const GIFRegTEXCLUT& TEXCLUT);
	template<int n> void WriteCLUT16_CSM2(const GIFRegTEX0& TEX0, const GIFRegTEXCLUT& TEXCLUT);
	template<int n> void WriteCLUT16S_CSM2(const GIFRegTEX0& TEX0, const GIFRegTEXCLUT& TEXCLUT);

	void WriteCLUT_NULL(const GIFRegTEX0& TEX0, const GIFRegTEXCLUT& TEXCLUT) {ASSERT(0);} // xenosaga 3

	static void WriteCLUT_T32_I8_CSM1(const uint32* RESTRICT src, uint16* RESTRICT clut);
	static void WriteCLUT_T32_I4_CSM1(const uint32* RESTRICT src, uint16* RESTRICT clut);
	static void WriteCLUT_T16_I8_CSM1(const uint16* RESTRICT src, uint16* RESTRICT clut);
	static void WriteCLUT_T16_I4_CSM1(const uint16* RESTRICT src, uint16* RESTRICT clut);
	static void ReadCLUT_T32_I8(const uint16* RESTRICT clut, uint32* RESTRICT dst);
	static void ReadCLUT_T32_I4(const uint16* RESTRICT clut, uint32* RESTRICT dst);
	static void ReadCLUT_T32_I4(const uint16* RESTRICT clut, uint32* RESTRICT dst32, uint64* RESTRICT dst64);
	static void ReadCLUT_T16_I8(const uint16* RESTRICT clut, uint32* RESTRICT dst);
	static void ReadCLUT_T16_I4(const uint16* RESTRICT clut, uint32* RESTRICT dst);
	static void ReadCLUT_T16_I4(const uint16* RESTRICT clut, uint32* RESTRICT dst32, uint64* RESTRICT dst64);
	static void ExpandCLUT64_T32_I8(const uint32* RESTRICT src, uint64* RESTRICT dst);
	static void ExpandCLUT64_T32(const GSVector4i& hi, const GSVector4i& lo0, const GSVector4i& lo1, const GSVector4i& lo2, const GSVector4i& lo3, GSVector4i* dst);
	static void ExpandCLUT64_T32(const GSVector4i& hi, const GSVector4i& lo, GSVector4i* dst);
	static void ExpandCLUT64_T16_I8(const uint32* RESTRICT src, uint64* RESTRICT dst);
	static void ExpandCLUT64_T16(const GSVector4i& hi, const GSVector4i& lo0, const GSVector4i& lo1, const GSVector4i& lo2, const GSVector4i& lo3, GSVector4i* dst);
	static void ExpandCLUT64_T16(const GSVector4i& hi, const GSVector4i& lo, GSVector4i* dst);

	static void Expand16(const uint16* RESTRICT src, uint32* RESTRICT dst, int w, const GIFRegTEXA& TEXA);

public:
	GSClut(GSLocalMemory* mem);
	virtual ~GSClut();

	void Invalidate();
	bool WriteTest(const GIFRegTEX0& TEX0, const GIFRegTEXCLUT& TEXCLUT);
	void Write(const GIFRegTEX0& TEX0, const GIFRegTEXCLUT& TEXCLUT);
	void Read(const GIFRegTEX0& TEX0);
	void Read32(const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA);
	void GetAlphaMinMax32(int& amin, int& amax);

	uint32 operator [] (size_t i) const {return m_buff32[i];}

	operator const uint32*() const  {return m_buff32;}
	operator const uint64*() const {return m_buff64;}
};
