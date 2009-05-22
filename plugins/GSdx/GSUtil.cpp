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

#include "stdafx.h"
#include "GS.h"
#include "GSUtil.h"
#include "svnrev.h"
#include "xbyak/xbyak_util.h"

static struct GSUtilMaps
{
	uint8 PrimClassField[8];
	bool CompatibleBitsField[64][64];
	bool SharedBitsField[64][64];

	struct GSUtilMaps()
	{
		PrimClassField[GS_POINTLIST] = GS_POINT_CLASS;
		PrimClassField[GS_LINELIST] = GS_LINE_CLASS;
		PrimClassField[GS_LINESTRIP] = GS_LINE_CLASS;
		PrimClassField[GS_TRIANGLELIST] = GS_TRIANGLE_CLASS;
		PrimClassField[GS_TRIANGLESTRIP] = GS_TRIANGLE_CLASS;
		PrimClassField[GS_TRIANGLEFAN] = GS_TRIANGLE_CLASS;
		PrimClassField[GS_SPRITE] = GS_SPRITE_CLASS;
		PrimClassField[GS_INVALID] = GS_INVALID_CLASS;

		memset(CompatibleBitsField, 0, sizeof(CompatibleBitsField));

		CompatibleBitsField[PSM_PSMCT32][PSM_PSMCT24] = true;
		CompatibleBitsField[PSM_PSMCT24][PSM_PSMCT32] = true;
		CompatibleBitsField[PSM_PSMCT16][PSM_PSMCT16S] = true;
		CompatibleBitsField[PSM_PSMCT16S][PSM_PSMCT16] = true;
		CompatibleBitsField[PSM_PSMZ32][PSM_PSMZ24] = true;
		CompatibleBitsField[PSM_PSMZ24][PSM_PSMZ32] = true;
		CompatibleBitsField[PSM_PSMZ16][PSM_PSMZ16S] = true;
		CompatibleBitsField[PSM_PSMZ16S][PSM_PSMZ16] = true;

		memset(SharedBitsField, 1, sizeof(SharedBitsField));

		SharedBitsField[PSM_PSMCT24][PSM_PSMT8H] = false;
		SharedBitsField[PSM_PSMCT24][PSM_PSMT4HL] = false;
		SharedBitsField[PSM_PSMCT24][PSM_PSMT4HH] = false;
		SharedBitsField[PSM_PSMZ24][PSM_PSMT8H] = false;
		SharedBitsField[PSM_PSMZ24][PSM_PSMT4HL] = false;
		SharedBitsField[PSM_PSMZ24][PSM_PSMT4HH] = false;
		SharedBitsField[PSM_PSMT8H][PSM_PSMCT24] = false;
		SharedBitsField[PSM_PSMT8H][PSM_PSMZ24] = false;
		SharedBitsField[PSM_PSMT4HL][PSM_PSMCT24] = false;
		SharedBitsField[PSM_PSMT4HL][PSM_PSMZ24] = false;
		SharedBitsField[PSM_PSMT4HL][PSM_PSMT4HH] = false;
		SharedBitsField[PSM_PSMT4HH][PSM_PSMCT24] = false;
		SharedBitsField[PSM_PSMT4HH][PSM_PSMZ24] = false;
		SharedBitsField[PSM_PSMT4HH][PSM_PSMT4HL] = false;
	}

} s_maps;

GS_PRIM_CLASS GSUtil::GetPrimClass(uint32 prim)
{
	return (GS_PRIM_CLASS)s_maps.PrimClassField[prim];
}

bool GSUtil::HasSharedBits(uint32 spsm, uint32 dpsm)
{
	return s_maps.SharedBitsField[spsm][dpsm];
}

bool GSUtil::HasSharedBits(uint32 sbp, uint32 spsm, uint32 dbp, uint32 dpsm)
{
	if(sbp != dbp) return false;

	return HasSharedBits(spsm, dpsm);
}

bool GSUtil::HasCompatibleBits(uint32 spsm, uint32 dpsm)
{
	if(spsm == dpsm) return true;

	return s_maps.CompatibleBitsField[spsm][dpsm];
}

bool GSUtil::IsRectInRect(const GSVector4i& inner, const GSVector4i& outer)
{
	return outer.left <= inner.left && inner.right <= outer.right && outer.top <= inner.top && inner.bottom <= outer.bottom;
}

bool GSUtil::IsRectInRectH(const GSVector4i& inner, const GSVector4i& outer)
{
	return outer.top <= inner.top && inner.bottom <= outer.bottom;
}

bool GSUtil::IsRectInRectV(const GSVector4i& inner, const GSVector4i& outer)
{
	return outer.left <= inner.left && inner.right <= outer.right;
}

bool GSUtil::CheckDirectX()
{
	if(HINSTANCE hDll = LoadLibrary(format("d3dx9_%d.dll", D3DX_SDK_VERSION).c_str()))
	{
		FreeLibrary(hDll);
	}
	else
	{
		int res = AfxMessageBox(_T("You need to update directx, would you like to do it now?"), MB_YESNO);

		if(res == IDYES)
		{
			ShellExecute(NULL, _T("open"), _T("http://www.microsoft.com/downloads/details.aspx?FamilyId=2DA43D38-DB71-4C1B-BC6A-9B6652CD92A3"), NULL, NULL, SW_SHOWNORMAL);
		}

		return false;
	}

	return true;
}

bool GSUtil::CheckSSE()
{
	Xbyak::util::Cpu cpu;
	Xbyak::util::Cpu::Type type;

	#if _M_SSE >= 0x402
	type = Xbyak::util::Cpu::tSSE42;
	#elif _M_SSE >= 0x401
	type = Xbyak::util::Cpu::tSSE41;
	#elif _M_SSE >= 0x301
	type = Xbyak::util::Cpu::tSSSE3;
	#elif _M_SSE >= 0x200
	type = Xbyak::util::Cpu::tSSE2;
	#endif

	if(!cpu.has(type))
	{
		string s = format("This CPU does not support SSE %d.%02d", _M_SSE >> 8, _M_SSE & 0xff);
		
		AfxMessageBox(s.c_str(), MB_OK);

		return false;
	}

	return true;
}

bool GSUtil::IsDirect3D10Available()
{
	if(HMODULE hModule = LoadLibrary(_T("d3d10.dll")))
	{
		FreeLibrary(hModule);

		return true;
	}

	return false;
}

char* GSUtil::GetLibName()
{
	static string str = format("GSdx %d", SVN_REV);

	if(SVN_MODS) str += "m";

	#if _M_AMD64
	str += " 64-bit";
	#endif

	list<string> sl;

	// TODO: gcc

	#ifdef __INTEL_COMPILER
	sl.push_back(format("Intel C++ %d.%02d", __INTEL_COMPILER / 100, __INTEL_COMPILER % 100));
	#elif _MSC_VER
	sl.push_back(format("MSVC %d.%02d", _MSC_VER / 100, _MSC_VER % 100));
	#endif
	
	#if _M_SSE >= 0x402
	sl.push_back("SSE42");
	#elif _M_SSE >= 0x401
	sl.push_back("SSE41");
	#elif _M_SSE >= 0x301
	sl.push_back("SSSE3");
	#elif _M_SSE >= 0x200
	sl.push_back("SSE2");
	#elif _M_SSE >= 0x100
	sl.push_back("SSE");
	#endif

	for(list<string>::iterator i = sl.begin(); i != sl.end(); )
	{
		if(i == sl.begin()) str += " (";
		str += *i;
		str += ++i != sl.end() ? ", " : ")";
	}

	return (char*)str.c_str();
}