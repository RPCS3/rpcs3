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

#include "GSVector.h"
#include "GPULocalMemory.h"

union GPUScanlineSelector
{
	struct
	{
		uint32 iip:1; // 0
		uint32 me:1; // 1
		uint32 abe:1; // 2
		uint32 abr:2; // 3
		uint32 tge:1; // 5
		uint32 tme:1; // 6
		uint32 twin:1; // 7
		uint32 tlu:1; // 8
		uint32 dtd:1; // 9
		uint32 ltf:1; // 10
		uint32 md:1; // 11
		uint32 sprite:1; // 12
		uint32 scalex:2; // 13
	};

	struct
	{
		uint32 _pad1:1; // 0
		uint32 rfb:2; // 1
		uint32 _pad2:2; // 3
		uint32 tfx:2; // 5
	};

	uint32 key;

	operator uint32() const {return key;}
};

__aligned(struct, 32) GPUScanlineGlobalData
{
	GPUScanlineSelector sel;

	void* vm;
	const void* tex;
	uint16* clut;
	GSVector4i twin; // TWW, TWH, TWX, TWY
};

__aligned(struct, 32) GPUScanlineLocalData
{
	const GPUScanlineGlobalData* gd;

	struct {GSVector4i u, v;} twin[3];
	struct {GSVector4i s, t, r, g, b, _pad[3];} d;
	struct {GSVector4i st, c;} d8;

	struct {GSVector4i s, t, r, b, g, uf, vf, dither, fd, test;} temp;
};
