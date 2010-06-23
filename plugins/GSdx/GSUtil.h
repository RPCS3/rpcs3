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
#include "GSLocalMemory.h"

class GSUtil
{
public:
	static GS_PRIM_CLASS GetPrimClass(uint32 prim);

	static bool HasSharedBits(uint32 spsm, uint32 dpsm);
	static bool HasSharedBits(uint32 sbp, uint32 spsm, uint32 dbp, uint32 dpsm);
	static bool HasCompatibleBits(uint32 spsm, uint32 dpsm);

	static bool CheckDirectX();
	static bool CheckSSE();

	static void UnloadDynamicLibraries();
	static char* GetLibName();

	// These should probably be located more closely to their respective DX9/DX11 driver files,
	// and not here in GSUtil (which should be DirectX independent, generally speaking) --air

	static void* GetDX9Proc( const char* methodname );
	static bool IsDirect3D11Available();
	static bool HasD3D11Features();
};

