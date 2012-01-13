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

class GSUtil
{
public:
	static const char* GetLibName();

	static GS_PRIM_CLASS GetPrimClass(uint32 prim);
	static int GetVertexCount(uint32 prim);

	static const uint32* HasSharedBitsPtr(uint32 dpsm);
	static bool HasSharedBits(uint32 spsm, const uint32* ptr);
	static bool HasSharedBits(uint32 spsm, uint32 dpsm);
	static bool HasSharedBits(uint32 sbp, uint32 spsm, uint32 dbp, uint32 dpsm);
	static bool HasCompatibleBits(uint32 spsm, uint32 dpsm);

	static bool CheckSSE();

#ifdef _WINDOWS

	static bool CheckDirectX();
	static bool CheckDirect3D11Level(D3D_FEATURE_LEVEL& level);
	static void UnloadDynamicLibraries();

#endif
};

