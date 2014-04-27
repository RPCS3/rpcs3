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

#include "stdafx.h"
#include "GS.h"
#include "GSUtil.h"
#include "xbyak/xbyak_util.h"

#ifdef _WINDOWS
#include "svnrev.h"
#else
#define SVN_REV 0
#define SVN_MODS 0
#endif

const char* GSUtil::GetLibName()
{
	// TODO: critsec

	static string str;

	if(str.empty())
	{
		str = "GSdx";

		#ifdef _WINDOWS
		str += format(" %lld", SVN_REV);
		if(SVN_MODS) str += "m";
		#endif

		#if _M_AMD64
		str += " 64-bit";
		#endif

		list<string> sl;

		// TODO: linux (gcc)

		#ifdef __INTEL_COMPILER
		sl.push_back(format("Intel C++ %d.%02d", __INTEL_COMPILER / 100, __INTEL_COMPILER % 100));
		#elif _MSC_VER
		sl.push_back(format("MSVC %d.%02d", _MSC_VER / 100, _MSC_VER % 100));
		#elif __GNUC__
		sl.push_back(format("GCC %d.%d.%d", __GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__));
		#endif

		#if _M_SSE >= 0x501
		sl.push_back("AVX2");
		#elif _M_SSE >= 0x500
		sl.push_back("AVX");
		#elif _M_SSE >= 0x402
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
	}

	return str.c_str();
}

static class GSUtilMaps
{
public:
	uint8 PrimClassField[8];
	uint8 VertexCountField[8];
	uint32 CompatibleBitsField[64][2];
	uint32 SharedBitsField[64][2];

	GSUtilMaps()
	{
		PrimClassField[GS_POINTLIST] = GS_POINT_CLASS;
		PrimClassField[GS_LINELIST] = GS_LINE_CLASS;
		PrimClassField[GS_LINESTRIP] = GS_LINE_CLASS;
		PrimClassField[GS_TRIANGLELIST] = GS_TRIANGLE_CLASS;
		PrimClassField[GS_TRIANGLESTRIP] = GS_TRIANGLE_CLASS;
		PrimClassField[GS_TRIANGLEFAN] = GS_TRIANGLE_CLASS;
		PrimClassField[GS_SPRITE] = GS_SPRITE_CLASS;
		PrimClassField[GS_INVALID] = GS_INVALID_CLASS;

		VertexCountField[GS_POINTLIST] = 1;
		VertexCountField[GS_LINELIST] = 2;
		VertexCountField[GS_LINESTRIP] = 2;
		VertexCountField[GS_TRIANGLELIST] = 3;
		VertexCountField[GS_TRIANGLESTRIP] = 3;
		VertexCountField[GS_TRIANGLEFAN] = 3;
		VertexCountField[GS_SPRITE] = 2;
		VertexCountField[GS_INVALID] = 1;

		memset(CompatibleBitsField, 0, sizeof(CompatibleBitsField));

		for(int i = 0; i < 64; i++)
		{
			CompatibleBitsField[i][i >> 5] |= 1 << (i & 0x1f);
		}

		CompatibleBitsField[PSM_PSMCT32][PSM_PSMCT24 >> 5] |= 1 << (PSM_PSMCT24 & 0x1f);
		CompatibleBitsField[PSM_PSMCT24][PSM_PSMCT32 >> 5] |= 1 << (PSM_PSMCT32 & 0x1f);
		CompatibleBitsField[PSM_PSMCT16][PSM_PSMCT16S >> 5] |= 1 << (PSM_PSMCT16S & 0x1f);
		CompatibleBitsField[PSM_PSMCT16S][PSM_PSMCT16 >> 5] |= 1 << (PSM_PSMCT16 & 0x1f);
		CompatibleBitsField[PSM_PSMZ32][PSM_PSMZ24 >> 5] |= 1 << (PSM_PSMZ24 & 0x1f);
		CompatibleBitsField[PSM_PSMZ24][PSM_PSMZ32 >> 5] |= 1 << (PSM_PSMZ32 & 0x1f);
		CompatibleBitsField[PSM_PSMZ16][PSM_PSMZ16S >> 5] |= 1 << (PSM_PSMZ16S & 0x1f);
		CompatibleBitsField[PSM_PSMZ16S][PSM_PSMZ16 >> 5] |= 1 << (PSM_PSMZ16 & 0x1f);

		memset(SharedBitsField, 0, sizeof(SharedBitsField));

		SharedBitsField[PSM_PSMCT24][PSM_PSMT8H >> 5] |= 1 << (PSM_PSMT8H & 0x1f);
		SharedBitsField[PSM_PSMCT24][PSM_PSMT4HL >> 5] |= 1 << (PSM_PSMT4HL & 0x1f);
		SharedBitsField[PSM_PSMCT24][PSM_PSMT4HH >> 5] |= 1 << (PSM_PSMT4HH & 0x1f);
		SharedBitsField[PSM_PSMZ24][PSM_PSMT8H >> 5] |= 1 << (PSM_PSMT8H & 0x1f);
		SharedBitsField[PSM_PSMZ24][PSM_PSMT4HL >> 5] |= 1 << (PSM_PSMT4HL & 0x1f);
		SharedBitsField[PSM_PSMZ24][PSM_PSMT4HH >> 5] |= 1 << (PSM_PSMT4HH & 0x1f);
		SharedBitsField[PSM_PSMT8H][PSM_PSMCT24 >> 5] |= 1 << (PSM_PSMCT24 & 0x1f);
		SharedBitsField[PSM_PSMT8H][PSM_PSMZ24 >> 5] |= 1 << (PSM_PSMZ24 & 0x1f);
		SharedBitsField[PSM_PSMT4HL][PSM_PSMCT24 >> 5] |= 1 << (PSM_PSMCT24 & 0x1f);
		SharedBitsField[PSM_PSMT4HL][PSM_PSMZ24 >> 5] |= 1 << (PSM_PSMZ24 & 0x1f);
		SharedBitsField[PSM_PSMT4HL][PSM_PSMT4HH >> 5] |= 1 << (PSM_PSMT4HH & 0x1f);
		SharedBitsField[PSM_PSMT4HH][PSM_PSMCT24 >> 5] |= 1 << (PSM_PSMCT24 & 0x1f);
		SharedBitsField[PSM_PSMT4HH][PSM_PSMZ24 >> 5] |= 1 << (PSM_PSMZ24 & 0x1f);
		SharedBitsField[PSM_PSMT4HH][PSM_PSMT4HL >> 5] |= 1 << (PSM_PSMT4HL & 0x1f);
	}

} s_maps;

GS_PRIM_CLASS GSUtil::GetPrimClass(uint32 prim)
{
	return (GS_PRIM_CLASS)s_maps.PrimClassField[prim];
}

int GSUtil::GetVertexCount(uint32 prim)
{
	return s_maps.VertexCountField[prim];
}

const uint32* GSUtil::HasSharedBitsPtr(uint32 dpsm)
{
	return s_maps.SharedBitsField[dpsm];
}

bool GSUtil::HasSharedBits(uint32 spsm, const uint32* RESTRICT ptr)
{
	return (ptr[spsm >> 5] & (1 << (spsm & 0x1f))) == 0;
}

bool GSUtil::HasSharedBits(uint32 spsm, uint32 dpsm)
{
	return (s_maps.SharedBitsField[dpsm][spsm >> 5] & (1 << (spsm & 0x1f))) == 0;
}

bool GSUtil::HasSharedBits(uint32 sbp, uint32 spsm, uint32 dbp, uint32 dpsm)
{
	return ((sbp ^ dbp) | (s_maps.SharedBitsField[dpsm][spsm >> 5] & (1 << (spsm & 0x1f)))) == 0;
}

bool GSUtil::HasCompatibleBits(uint32 spsm, uint32 dpsm)
{
	return (s_maps.CompatibleBitsField[spsm][dpsm >> 5] & (1 << (dpsm & 0x1f))) != 0;
}

bool GSUtil::CheckSSE()
{
	Xbyak::util::Cpu cpu;
	Xbyak::util::Cpu::Type type;

	#if _M_SSE >= 0x500
	type = Xbyak::util::Cpu::tAVX;
	#elif _M_SSE >= 0x402
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
		fprintf(stderr, "This CPU does not support SSE %d.%02d", _M_SSE >> 8, _M_SSE & 0xff);

		return false;
	}

	return true;
}

#ifdef _WINDOWS

bool GSUtil::CheckDirectX()
{
	OSVERSIONINFOEX version;
	memset(&version, 0, sizeof(version));
	version.dwOSVersionInfoSize = sizeof(version);

	if(GetVersionEx((OSVERSIONINFO*)&version))
	{
		printf("Windows %d.%d.%d", version.dwMajorVersion, version.dwMinorVersion, version.dwBuildNumber);

		if(version.wServicePackMajor > 0)
		{
			printf(" (%s %d.%d)", version.szCSDVersion, version.wServicePackMajor, version.wServicePackMinor);
		}

		printf("\n");
	}

	string d3dx9_dll = format("d3dx9_%d.dll", D3DX_SDK_VERSION);

	if(HINSTANCE hDll = LoadLibrary(d3dx9_dll.c_str()))
	{
		FreeLibrary(hDll);
	}
	else
	{
		printf("Cannot find %s\n", d3dx9_dll.c_str());

		if(MessageBox(NULL, "You need to update some directx libraries, would you like to do it now?", "GSdx", MB_YESNO) == IDYES)
		{
			const char* url = "http://www.microsoft.com/downloads/details.aspx?FamilyId=2DA43D38-DB71-4C1B-BC6A-9B6652CD92A3";

			ShellExecute(NULL, "open", url, NULL, NULL, SW_SHOWNORMAL);
		}

		return false;
	}

	return true;
}

// ---------------------------------------------------------------------------------
//  DX11 Detection (includes DXGI detection and dynamic library method bindings)
// ---------------------------------------------------------------------------------
//  Code 'Borrowed' from Microsoft's DXGI sources -- Modified to suit our needs. --air
//  Stripped down because of unnecessary complexity and false positives
//  e.g. (d3d11_beta.dll would fail at device creation time) --pseudonym

static int s_DXGI;
static int s_D3D11;

bool GSUtil::CheckDXGI()
{
	if (0 == s_DXGI)
	{
		HMODULE hmod = LoadLibrary("dxgi.dll");
		s_DXGI = hmod ? 1 : -1;
		if (hmod)
			FreeLibrary(hmod);
	}

	return s_DXGI > 0;
}

bool GSUtil::CheckD3D11()
{
	if (!CheckDXGI())
		return false;

	if (0 == s_D3D11)
	{
		HMODULE hmod = LoadLibrary("d3d11.dll");
		s_D3D11 = hmod ? 1 : -1;
		if (hmod)
			FreeLibrary(hmod);
	}

	return s_D3D11 > 0;
}

D3D_FEATURE_LEVEL GSUtil::CheckDirect3D11Level(IDXGIAdapter *adapter, D3D_DRIVER_TYPE type)
{
	HRESULT hr;
	D3D_FEATURE_LEVEL level;

	if(!CheckD3D11())
		return (D3D_FEATURE_LEVEL)0;

	hr = D3D11CreateDevice(adapter, type, NULL, 0, NULL, 0, D3D11_SDK_VERSION, NULL, &level, NULL);

	return SUCCEEDED(hr) ? level : (D3D_FEATURE_LEVEL)0;
}

#endif
