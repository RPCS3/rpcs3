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

#error TODO

#include "stdafx.h"
#include "GSDrawScanlineCodeGenerator.h"

#if _M_SSE < 0x500 && (defined(_M_AMD64) || defined(_WIN64))

void GSDrawScanlineCodeGenerator::Generate()
{
}

void GSDrawScanlineCodeGenerator::Init()
{
}

void GSDrawScanlineCodeGenerator::Step()
{
}

void GSDrawScanlineCodeGenerator::TestZ(const Xmm& temp1, const Xmm& temp2)
{
}

void GSDrawScanlineCodeGenerator::SampleTexture()
{
}

void GSDrawScanlineCodeGenerator::Wrap(const Xmm& uv)
{
}

void GSDrawScanlineCodeGenerator::Wrap(const Xmm& uv0, const Xmm& uv1)
{
}

void GSDrawScanlineCodeGenerator::AlphaTFX()
{
}

void GSDrawScanlineCodeGenerator::ReadMask()
{
}

void GSDrawScanlineCodeGenerator::TestAlpha()
{
}

void GSDrawScanlineCodeGenerator::ColorTFX()
{
}

void GSDrawScanlineCodeGenerator::Fog()
{
}

void GSDrawScanlineCodeGenerator::ReadFrame()
{
}

void GSDrawScanlineCodeGenerator::TestDestAlpha()
{
}

void GSDrawScanlineCodeGenerator::WriteMask()
{
}

void GSDrawScanlineCodeGenerator::WriteZBuf()
{
}

void GSDrawScanlineCodeGenerator::AlphaBlend()
{
}

void GSDrawScanlineCodeGenerator::WriteFrame()
{
}

void GSDrawScanlineCodeGenerator::ReadPixel(const Xmm& dst, const Reg32& addr)
{
}

void GSDrawScanlineCodeGenerator::WritePixel(const Xmm& src, const Reg32& addr, const Reg8& mask, bool fast, int psm, int fz)
{
}

static const int s_offsets[4] = {0, 2, 8, 10};

void GSDrawScanlineCodeGenerator::WritePixel(const Xmm& src, const Reg32& addr, uint8 i, int psm)
{
}

void GSDrawScanlineCodeGenerator::ReadTexel(const Xmm& dst, const Xmm& addr, const Xmm& temp1, const Xmm& temp2)
{
}

void GSDrawScanlineCodeGenerator::ReadTexel(const Xmm& dst, const Xmm& addr, uint8 i)
{
}

#endif