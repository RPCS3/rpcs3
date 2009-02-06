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

static struct GSUtilMaps
{
	BYTE PrimClassField[8];
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

GS_PRIM_CLASS GSUtil::GetPrimClass(DWORD prim)
{
	return (GS_PRIM_CLASS)s_maps.PrimClassField[prim];
}

bool GSUtil::HasSharedBits(DWORD spsm, DWORD dpsm)
{
	return s_maps.SharedBitsField[spsm][dpsm];
}

bool GSUtil::HasSharedBits(DWORD sbp, DWORD spsm, DWORD dbp, DWORD dpsm)
{
	if(sbp != dbp) return false;

	return HasSharedBits(spsm, dpsm);
}

bool GSUtil::HasCompatibleBits(DWORD spsm, DWORD dpsm)
{
	if(spsm == dpsm) return true;

	return s_maps.CompatibleBitsField[spsm][dpsm];
}

bool GSUtil::IsRectInRect(const CRect& inner, const CRect& outer)
{
	return outer.left <= inner.left && inner.right <= outer.right && outer.top <= inner.top && inner.bottom <= outer.bottom;
}

bool GSUtil::IsRectInRectH(const CRect& inner, const CRect& outer)
{
	return outer.top <= inner.top && inner.bottom <= outer.bottom;
}

bool GSUtil::IsRectInRectV(const CRect& inner, const CRect& outer)
{
	return outer.left <= inner.left && inner.right <= outer.right;
}

void GSUtil::FitRect(CRect& r, int aspectratio)
{
	static const int ar[][2] = {{0, 0}, {4, 3}, {16, 9}};

	if(aspectratio <= 0 || aspectratio >= countof(ar))
	{
		return;
	}

	int arx = ar[aspectratio][0];
	int ary = ar[aspectratio][1];

	CRect r2 = r;

	if(arx > 0 && ary > 0)
	{
		if(r.Width() * ary > r.Height() * arx)
		{
			int w = r.Height() * arx / ary;
			r.left = r.CenterPoint().x - w / 2;
			if(r.left & 1) r.left++;
			r.right = r.left + w;
		}
		else
		{
			int h = r.Width() * ary / arx;
			r.top = r.CenterPoint().y - h / 2;
			if(r.top & 1) r.top++;
			r.bottom = r.top + h;
		}
	}

	r &= r2;
}

bool GSUtil::CheckDirectX()
{
	CString str;

	str.Format(_T("d3dx9_%d.dll"), D3DX_SDK_VERSION);

	if(HINSTANCE hDll = LoadLibrary(str))
	{
		FreeLibrary(hDll);
	}
	else
	{
		int res = AfxMessageBox(_T("Please update DirectX!\n\nWould you like to open the download page in your browser?"), MB_YESNO);

		if(res == IDYES)
		{
			ShellExecute(NULL, _T("open"), _T("http://www.microsoft.com/downloads/details.aspx?FamilyId=2DA43D38-DB71-4C1B-BC6A-9B6652CD92A3"), NULL, NULL, SW_SHOWNORMAL);
		}

		return false;
	}

	return true;
}

static bool _CheckSSE()
{
	__try
	{
		static __m128i m;

		#if _M_SSE >= 0x402
		m.m128i_i32[0] = _mm_popcnt_u32(1234);
		#elif _M_SSE >= 0x401
		m = _mm_packus_epi32(m, m);
		#elif _M_SSE >= 0x301
		m = _mm_alignr_epi8(m, m, 1);
		#elif _M_SSE >= 0x200
		m = _mm_packs_epi32(m, m);
		#endif
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		return false;
	}

	return true;
}

bool GSUtil::CheckSSE()
{
	if(!_CheckSSE())
	{
		CString str;
		str.Format(_T("This CPU does not support SSE %d.%02d"), _M_SSE >> 8, _M_SSE & 0xff);
		AfxMessageBox(str, MB_OK);

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
	CString str;

	str.Format(_T("GSdx %d"), SVN_REV);

	if(SVN_MODS) str += _T("m");

#if _M_AMD64
	str += _T(" 64-bit");
#endif

	CAtlList<CString> sl;

#ifdef __INTEL_COMPILER
	CString s;
	s.Format(_T("Intel C++ %d.%02d"), __INTEL_COMPILER/100, __INTEL_COMPILER%100);
	sl.AddTail(s);
#elif _MSC_VER
	CString s;
	s.Format(_T("MSVC %d.%02d"), _MSC_VER/100, _MSC_VER%100);
	sl.AddTail(s);
#endif

#if _M_SSE >= 0x402
	sl.AddTail(_T("SSE42"));
#elif _M_SSE >= 0x401
	sl.AddTail(_T("SSE41"));
#elif _M_SSE >= 0x301
	sl.AddTail(_T("SSSE3"));
#elif _M_SSE >= 0x200
	sl.AddTail(_T("SSE2"));
#elif _M_SSE >= 0x100
	sl.AddTail(_T("SSE"));
#endif

	POSITION pos = sl.GetHeadPosition();

	while(pos)
	{
		if(pos == sl.GetHeadPosition()) str += _T(" (");
		str += sl.GetNext(pos);
		str += pos ? _T(", ") : _T(")");
	}

	static char buff[256];
	strncpy(buff, CStringA(str), min(countof(buff)-1, str.GetLength()));
	return buff;
}