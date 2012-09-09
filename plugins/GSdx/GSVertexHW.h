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

#include "GS.h"
#include "GSVector.h"

#pragma pack(push, 1)

__aligned(struct, 32) GSVertexHW9
{
	GSVector4 t; 
	GSVector4 p;

	// t.z = union {struct {uint8 r, g, b, a;}; uint32 c0;};
	// t.w = union {struct {uint8 ta0, ta1, res, f;}; uint32 c1;}

	GSVertexHW9& operator = (GSVertexHW9& v) {t = v.t; p = v.p; return *this;}
};

#pragma pack(pop)
