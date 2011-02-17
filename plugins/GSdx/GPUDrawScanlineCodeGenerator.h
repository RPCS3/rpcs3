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

#include "GPUScanlineEnvironment.h"
#include "GSFunctionMap.h"

using namespace Xbyak;

class GPUDrawScanlineCodeGenerator : public GSCodeGenerator
{
	void operator = (const GPUDrawScanlineCodeGenerator&);

	static const GSVector4i m_test[8];
	static const uint16 m_dither[4][16];

	GPUScanlineSelector m_sel;
	GPUScanlineLocalData& m_local;

	void Generate();

	void Init();
	void Step();
	void TestMask();
	void SampleTexture();
	void ColorTFX();
	void AlphaBlend();
	void Dither();
	void WriteFrame();

	void ReadTexel(const Xmm& dst, const Xmm& addr);

	template<int shift> void modulate16(const Xmm& a, const Operand& f);
	template<int shift> void lerp16(const Xmm& a, const Xmm& b, const Operand& f);
	void alltrue();
	void blend8(const Xmm& a, const Xmm& b);
	void blend(const Xmm& a, const Xmm& b, const Xmm& mask);

public:
	GPUDrawScanlineCodeGenerator(void* param, uint32 key, void* code, size_t maxsize);
};