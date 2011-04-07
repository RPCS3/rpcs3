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

#include "GSSetting.h"

class GSdxApp
{
	std::string m_ini;
	std::string m_section;
#ifdef _LINUX
	std::map< std::string, std::string > m_configuration_map;
#endif

public:
	GSdxApp();

    void* GetModuleHandlePtr();

#ifdef _WINDOWS
 	HMODULE GetModuleHandle() {return (HMODULE)GetModuleHandlePtr();}
#endif
#ifdef _LINUX
	void BuildConfigurationMap(const char* lpFileName);
	size_t GetPrivateProfileString(const char* lpAppName, const char* lpKeyName, const char* lpDefault, char* lpReturnedString, size_t nSize, const char* lpFileName);
	bool WritePrivateProfileString(const char* lpAppName, const char* lpKeyName, const char* pString, const char* lpFileName);
	int GetPrivateProfileInt(const char* lpAppName, const char* lpKeyName, int nDefault, const char* lpFileName);
#endif

	string GetConfig(const char* entry, const char* value);
	void SetConfig(const char* entry, const char* value);
	int GetConfig(const char* entry, int value);
	void SetConfig(const char* entry, int value);

	void SetConfigDir(const char* dir);

	vector<GSSetting> m_gs_renderers;
	vector<GSSetting> m_gs_interlace;
	vector<GSSetting> m_gs_aspectratio;
	vector<GSSetting> m_gs_upscale_multiplier;

	vector<GSSetting> m_gpu_renderers;
	vector<GSSetting> m_gpu_filter;
	vector<GSSetting> m_gpu_dithering;
	vector<GSSetting> m_gpu_aspectratio;
	vector<GSSetting> m_gpu_scale;
};

extern GSdxApp theApp;
