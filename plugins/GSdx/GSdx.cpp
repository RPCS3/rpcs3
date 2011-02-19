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
#include "GSdx.h"

static void* s_hModule;

#ifdef _WINDOWS

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	switch(ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		s_hModule = hModule;
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}

	return TRUE;
}

#else

size_t GetPrivateProfileString(const char* lpAppName, const char* lpKeyName, const char* lpDefault, char* lpReturnedString, size_t nSize, const char* lpFileName)
{
    // TODO: linux

    return 0;
}

bool WritePrivateProfileString(const char* lpAppName, const char* lpKeyName, const char* pString, const char* lpFileName)
{
    // TODO: linux

    return false;
}

int GetPrivateProfileInt(const char* lpAppName, const char* lpKeyName, int nDefault, const char* lpFileName)
{
    // TODO: linux

    return nDefault;
}
#endif

GSdxApp theApp;

GSdxApp::GSdxApp()
{
	m_ini = "inis/GSdx.ini";
	m_section = "Settings";

	m_gs_renderers.push_back(GSSetting(0, "Direct3D9 (Hardware)", ""));
	m_gs_renderers.push_back(GSSetting(1, "Direct3D9 (Software)", ""));
	m_gs_renderers.push_back(GSSetting(2, "Direct3D9 (Null)", ""));
	m_gs_renderers.push_back(GSSetting(3, "Direct3D%d    ", "Hardware"));
	m_gs_renderers.push_back(GSSetting(4, "Direct3D%d    ", "Software"));
	m_gs_renderers.push_back(GSSetting(5, "Direct3D%d    ", "Null"));
	m_gs_renderers.push_back(GSSetting(12, "Null (Software)", ""));
	m_gs_renderers.push_back(GSSetting(13, "Null (Null)", ""));

	m_gs_interlace.push_back(GSSetting(0, "None", ""));
	m_gs_interlace.push_back(GSSetting(1, "Weave tff", "saw-tooth"));
	m_gs_interlace.push_back(GSSetting(2, "Weave bff", "saw-tooth"));
	m_gs_interlace.push_back(GSSetting(3, "Bob tff", "use blend if shaking"));
	m_gs_interlace.push_back(GSSetting(4, "Bob bff", "use blend if shaking"));
	m_gs_interlace.push_back(GSSetting(5, "Blend tff", "slight blur, 1/2 fps"));
	m_gs_interlace.push_back(GSSetting(6, "Blend bff", "slight blur, 1/2 fps"));

	m_gs_aspectratio.push_back(GSSetting(0, "Stretch", ""));
	m_gs_aspectratio.push_back(GSSetting(1, "4:3", ""));
	m_gs_aspectratio.push_back(GSSetting(2, "16:9", ""));

	m_gs_upscale_multiplier.push_back(GSSetting(1, "Custom", ""));
	m_gs_upscale_multiplier.push_back(GSSetting(2, "2x Native", ""));
	m_gs_upscale_multiplier.push_back(GSSetting(3, "3x Native", ""));
	m_gs_upscale_multiplier.push_back(GSSetting(4, "4x Native", ""));
	m_gs_upscale_multiplier.push_back(GSSetting(5, "5x Native", ""));
	m_gs_upscale_multiplier.push_back(GSSetting(6, "6x Native", ""));

	m_gpu_renderers.push_back(GSSetting(0, "Direct3D9 (Software)", ""));
	m_gpu_renderers.push_back(GSSetting(1, "Direct3D11 (Software)", ""));
	m_gpu_renderers.push_back(GSSetting(2, "Null (Software)", ""));
	//m_gpu_renderers.push_back(GSSetting(3, "Null (Null)", ""));

	m_gpu_filter.push_back(GSSetting(0, "Nearest", ""));
	m_gpu_filter.push_back(GSSetting(1, "Bilinear (polygons only)", ""));
	m_gpu_filter.push_back(GSSetting(2, "Bilinear", ""));

	m_gpu_dithering.push_back(GSSetting(0, "Disabled", ""));
	m_gpu_dithering.push_back(GSSetting(1, "Auto", ""));

	m_gpu_aspectratio.push_back(GSSetting(0, "Stretch", ""));
	m_gpu_aspectratio.push_back(GSSetting(1, "4:3", ""));
	m_gpu_aspectratio.push_back(GSSetting(2, "16:9", ""));

	m_gpu_scale.push_back(GSSetting(0 | (0 << 2), "H x 1 - V x 1", ""));
	m_gpu_scale.push_back(GSSetting(1 | (0 << 2), "H x 2 - V x 1", ""));
	m_gpu_scale.push_back(GSSetting(0 | (1 << 2), "H x 1 - V x 2", ""));
	m_gpu_scale.push_back(GSSetting(1 | (1 << 2), "H x 2 - V x 2", ""));
	m_gpu_scale.push_back(GSSetting(2 | (1 << 2), "H x 4 - V x 2", ""));
	m_gpu_scale.push_back(GSSetting(1 | (2 << 2), "H x 2 - V x 4", ""));
	m_gpu_scale.push_back(GSSetting(2 | (2 << 2), "H x 4 - V x 4", ""));
}

void* GSdxApp::GetModuleHandlePtr()
{
	return s_hModule;
}

void GSdxApp::SetConfigDir(const char* dir)
{
	if( dir == NULL )
	{
		m_ini = "inis/GSdx.ini";
	}
	else
	{
		m_ini = dir;

		if(m_ini[m_ini.length() - 1] != DIRECTORY_SEPARATOR)
		{
			m_ini += DIRECTORY_SEPARATOR;
		}

		m_ini += "GSdx.ini";
	}
}

string GSdxApp::GetConfig(const char* entry, const char* value)
{
	char buff[4096] = {0};

	GetPrivateProfileString(m_section.c_str(), entry, value, buff, countof(buff), m_ini.c_str());

	return string(buff);
}

void GSdxApp::SetConfig(const char* entry, const char* value)
{
	WritePrivateProfileString(m_section.c_str(), entry, value, m_ini.c_str());
}

int GSdxApp::GetConfig(const char* entry, int value)
{
	return GetPrivateProfileInt(m_section.c_str(), entry, value, m_ini.c_str());
}

void GSdxApp::SetConfig(const char* entry, int value)
{
	char buff[32] = {0};

	sprintf(buff, "%d", value);

	SetConfig(entry, buff);
}
