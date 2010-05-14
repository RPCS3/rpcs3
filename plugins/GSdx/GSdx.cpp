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

static HMODULE s_hModule;

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

GSdxApp theApp;

std::string GSdxApp::m_ini( "inis/GSdx.ini" );
const char* GSdxApp::m_section = "Settings";

GSdxApp::GSdxApp()
{
}

HMODULE GSdxApp::GetModuleHandle()
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

		if((m_ini[m_ini.length()-1] != '/') && (m_ini[m_ini.length()-1] != '\\'))
		{
			m_ini += '\\';
		}

		m_ini += "GSdx.ini";
	}
}

string GSdxApp::GetConfig(const char* entry, const char* value)
{
	char buff[4096] = {0};
	GetPrivateProfileString(m_section, entry, value, buff, countof(buff), m_ini.c_str());
	return string(buff);
}

void GSdxApp::SetConfig(const char* entry, const char* value)
{
	WritePrivateProfileString(m_section, entry, value, m_ini.c_str());
}

int GSdxApp::GetConfig(const char* entry, int value)
{
	return GetPrivateProfileInt(m_section, entry, value, m_ini.c_str());
}

void GSdxApp::SetConfig(const char* entry, int value)
{
	char buff[32] = {0};
	itoa(value, buff, 10);
	SetConfig(entry, buff);
}
