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
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#pragma once

#include "GSLocalMemory.h"

class GSDirtyRect
{
	int left;
	int top;
	int right;
	int bottom;

	uint32 psm;

public:
	GSDirtyRect();
	GSDirtyRect(const GSVector4i& r, uint32 psm);
	GSVector4i GetDirtyRect(const GIFRegTEX0& TEX0);
};

class GSDirtyRectList : public list<GSDirtyRect>
{
public:
	GSDirtyRectList() {}
	GSVector4i GetDirtyRectAndClear(const GIFRegTEX0& TEX0, const GSVector2i& size);
};
