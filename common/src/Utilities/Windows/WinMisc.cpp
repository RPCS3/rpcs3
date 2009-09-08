/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2009  PCSX2 Dev Team
 * 
 *  PCSX2 is free software: you can redistribute it and/or modify it under the terms
 *  of the GNU Lesser General Public License as published by the Free Software Found-
 *  ation, either version 3 of the License, or (at your option) any later version.
 *
 *  PCSX2 is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 *  without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 *  PURPOSE.  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along with PCSX2.
 *  If not, see <http://www.gnu.org/licenses/>.
 */
 
#include "../PrecompiledHeader.h"
#include "RedtapeWindows.h"
#include "WinVersion.h"

#include <shlwapi.h>		// for IsOS()


static LARGE_INTEGER lfreq;

void InitCPUTicks()
{
	QueryPerformanceFrequency( &lfreq );
}

u64 GetTickFrequency()
{
	return lfreq.QuadPart;
}

u64 GetCPUTicks()
{
	LARGE_INTEGER count;
	QueryPerformanceCounter( &count );
	return count.QuadPart;
}

//////////////////////////////////////////////////////////////////////////////////////////
static const char* GetVersionName( DWORD minorVersion, DWORD majorVersion )
{
	switch( majorVersion )
	{
		case 6:
			return "Vista";
		break;

		case 5:
			switch( minorVersion )
			{
				case 0:
					return "2000";
				break;
				
				case 1:
					return "XP";
				break;
				
				case 2:
					return "Server 2003";
				break;

				default:
					return "2000/XP";
			}
		break;
		
		case 4:
			switch( minorVersion )
			{
				case 0:
					return "95";
				break;
				
				case 10:
					return "98";
				break;
				
				case 90:
					return "ME";
				break;
				
				default:
					return "95/98/ME";
			}
		break;
		
		case 3:
			return "32S";
		break;
		
		default:
			return "Unknown";
	}
}

void Win32::RealVersionInfo::InitVersionString()
{
	DWORD version = GetVersion();

    int majorVersion = (DWORD)(LOBYTE(LOWORD(version)));
    int minorVersion = (DWORD)(HIBYTE(LOWORD(version)));

	wxString verName( wxString::FromAscii( GetVersionName( minorVersion, majorVersion ) ) );

	bool IsCompatMode = false;

	if( IsVista() )
	{
		m_VersionString = L"Windows Vista";

		if( verName != L"Vista" )
		{
			if( verName == L"Unknown" )
				m_VersionString += L" or Newer";
			else
				IsCompatMode = true;
		}

		m_VersionString += wxsFormat( L" v%d.%d", majorVersion, minorVersion );

		if( IsOS( OS_WOW6432 ) )
			m_VersionString += L" (64-bit)";
		else
			m_VersionString += L" (32-bit)";
	}
	else if( IsXP() )
	{
		m_VersionString = wxsFormat( L"Windows XP v%d.%d", majorVersion, minorVersion );
				
		if( IsOS( OS_WOW6432 ) )
			m_VersionString += L" (64-bit)";
		else
			m_VersionString += L" (32-bit)";

		if( verName != L"XP" )
			IsCompatMode = true;
	}
	else
	{
		m_VersionString = L"Windows " + verName;
	}

	if( IsCompatMode )
		m_VersionString += wxsFormat( L" [compatibility mode, running as Windows %s]", verName );
}

Win32::RealVersionInfo::~RealVersionInfo()
{
	if( m_kernel32 != NULL )
		FreeLibrary( m_kernel32 );
}

Win32::RealVersionInfo::RealVersionInfo() :
	m_kernel32( LoadLibrary( L"kernel32" ) )
{
	InitVersionString();
}

bool Win32::RealVersionInfo::IsVista() const
{
	if( m_kernel32 == NULL ) return false;

	// CreateThreadpoolWait is a good pick -- it's not likely to exist on older OS's
	// GetLocaleInfoEx is also good -- two picks are better than one, just in case
	//   one of the APIs becomes available in a future XP service pack.
    
	return
		( GetProcAddress( m_kernel32, "CreateThreadpoolWait" ) != NULL ) &&
		( GetProcAddress( m_kernel32, "GetLocaleInfoEx" ) != NULL );
}

bool Win32::RealVersionInfo::IsXP() const
{
	return ( GetProcAddress( m_kernel32, "GetNativeSystemInfo" ) != NULL );
}

const wxString& Win32::RealVersionInfo::GetVersionString() const
{
	return m_VersionString;
}

