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

#include "GSVector.h"
#include "GPULocalMemory.h"

union GPUScanlineSelector
{
	struct
	{
		DWORD iip:1; // 0
		DWORD me:1; // 1
		DWORD abe:1; // 2
		DWORD abr:2; // 3
		DWORD tge:1; // 5
		DWORD tme:1; // 6
		DWORD twin:1; // 7
		DWORD tlu:1; // 8
		DWORD dtd:1; // 9
		DWORD ltf:1; // 10
		DWORD md:1; // 11
		DWORD sprite:1; // 12
	};

	struct
	{
		DWORD _pad1:1; // 0
		DWORD rfb:2; // 1
		DWORD _pad2:2; // 3
		DWORD tfx:2; // 5
	};

	DWORD key;

	operator DWORD() {return key;}
};

__declspec(align(16)) struct GPUScanlineParam
{
	GPUScanlineSelector sel;

	const void* tex;
	const WORD* clut;
};

__declspec(align(16)) struct GPUScanlineEnvironment
{
	GPUScanlineSelector sel;

	// GPULocalMemory* mem; // TODO: obsolite
	void* vm;
	const void* tex;
	const WORD* clut;
	DWORD fbw; // 10 + m_scale.cx

	// GSVector4i md; // similar to gs fba

	struct {GSVector4i u, v;} twin[3];
	struct {GSVector4i s, t, r, g, b, _pad[3];} d;
	struct {GSVector4i st, c;} d8;
	struct {GSVector4i s, t, r, b, g, uf, vf, dither, fd, test;} temp;
};
