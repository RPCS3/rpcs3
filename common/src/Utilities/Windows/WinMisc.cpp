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

#pragma comment(lib, "User32.lib")

#include "PrecompiledHeader.h"
#include "RedtapeWindows.h"

#include <ShTypes.h>

static __aligned16 LARGE_INTEGER lfreq;

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

// Windows SDK 7 provides this but previous ones do not, so roll our own in those cases:
#ifndef VER_SUITE_WH_SERVER
#	define VER_SUITE_WH_SERVER                 0x00008000
#endif

typedef void (WINAPI *PGNSI)(LPSYSTEM_INFO);
typedef BOOL (WINAPI *PGPI)(DWORD, DWORD, DWORD, DWORD, PDWORD);

// Calculates the Windows OS Version and install information, and returns it as a
// human-readable string. :)
// (Handy function borrowed from Microsoft's MSDN Online, and reformatted to use wxString.)
wxString GetOSVersionString()
{
	wxString retval;

	OSVERSIONINFOEX	osvi;
	SYSTEM_INFO		si;
	PGNSI			pGNSI;
	PGPI			pGPI;
	BOOL			bOsVersionInfoEx;
	DWORD			dwType;

	memzero( si );
	memzero( osvi );

	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);

	if( !(bOsVersionInfoEx = GetVersionEx ((OSVERSIONINFO *) &osvi)) )
		return L"GetVersionEx Error!";

	// Call GetNativeSystemInfo if supported or GetSystemInfo otherwise.

	pGNSI = (PGNSI) GetProcAddress( GetModuleHandle(L"kernel32.dll"), "GetNativeSystemInfo" );
	if(NULL != pGNSI)
		pGNSI( &si );
	else
		GetSystemInfo( &si );

	if ( VER_PLATFORM_WIN32_NT!=osvi.dwPlatformId || osvi.dwMajorVersion <= 4 )
		return L"Unsupported Operating System!";

	retval += L"Microsoft ";

	// Test for the specific product.

	if ( osvi.dwMajorVersion == 6 )
	{
		if( osvi.dwMinorVersion == 0 )
			retval += ( osvi.wProductType == VER_NT_WORKSTATION ) ? L"Windows Vista " : L"Windows Server 2008 ";

		if ( osvi.dwMinorVersion == 1 )
			retval += ( osvi.wProductType == VER_NT_WORKSTATION ) ? L"Windows 7 " : L"Windows Server 2008 R2 ";

		pGPI = (PGPI) GetProcAddress( GetModuleHandle(TEXT("kernel32.dll")), "GetProductInfo");

		pGPI( osvi.dwMajorVersion, osvi.dwMinorVersion, 0, 0, &dwType);

		switch( dwType )
		{
			case PRODUCT_ULTIMATE:						retval += L"Ultimate Edition";							break;
			case PRODUCT_HOME_PREMIUM:					retval += L"Home Premium Edition";						break;
			case PRODUCT_HOME_BASIC:					retval += L"Home Basic Edition";						break;
			case PRODUCT_ENTERPRISE:					retval += L"Enterprise Edition";						break;
			case PRODUCT_BUSINESS:						retval += L"Business Edition";							break;
			case PRODUCT_STARTER:						retval += L"Starter Edition";							break;
			case PRODUCT_CLUSTER_SERVER:				retval += L"Cluster Server Edition";					break;
			case PRODUCT_DATACENTER_SERVER:				retval += L"Datacenter Edition";						break;
			case PRODUCT_DATACENTER_SERVER_CORE:		retval += L"Datacenter Edition (core installation)";	break;
			case PRODUCT_ENTERPRISE_SERVER:				retval += L"Enterprise Edition";						break;
			case PRODUCT_ENTERPRISE_SERVER_CORE:		retval += L"Enterprise Edition (core installation)";	break;
			case PRODUCT_SMALLBUSINESS_SERVER:			retval += L"Small Business Server";						break;
			case PRODUCT_SMALLBUSINESS_SERVER_PREMIUM:	retval += L"Small Business Server Premium Edition";		break;
			case PRODUCT_STANDARD_SERVER:				retval += L"Standard Edition";							break;
			case PRODUCT_STANDARD_SERVER_CORE:			retval += L"Standard Edition (core installation)";		break;
			case PRODUCT_WEB_SERVER:					retval += L"Web Server Edition";						break;
		}
	}

	if ( osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 2 )
	{
		if( GetSystemMetrics(SM_SERVERR2) )
			retval += L"Windows Server 2003 R2, ";
		else if ( osvi.wSuiteMask==VER_SUITE_STORAGE_SERVER )
			retval += L"Windows Storage Server 2003";
		else if ( osvi.wSuiteMask==VER_SUITE_WH_SERVER )
			retval += L"Windows Home Server";
		else if( osvi.wProductType == VER_NT_WORKSTATION && si.wProcessorArchitecture==PROCESSOR_ARCHITECTURE_AMD64)
			retval += L"Windows XP Professional x64 Edition";
		else
			retval += L"Windows Server 2003, ";

		// Test for the server type.
		if ( osvi.wProductType != VER_NT_WORKSTATION )
		{
			if ( si.wProcessorArchitecture==PROCESSOR_ARCHITECTURE_AMD64 )
			{
				if( osvi.wSuiteMask & VER_SUITE_DATACENTER )
					retval += L"Datacenter x64 Edition";
				else if( osvi.wSuiteMask & VER_SUITE_ENTERPRISE )
					retval += L"Enterprise x64 Edition";
				else
					retval += L"Standard x64 Edition";
			}

			else
			{
				if ( osvi.wSuiteMask & VER_SUITE_COMPUTE_SERVER )
					retval += L"Compute Cluster Edition";
				else if( osvi.wSuiteMask & VER_SUITE_DATACENTER )
					retval += L"Datacenter Edition";
				else if( osvi.wSuiteMask & VER_SUITE_ENTERPRISE )
					retval += L"Enterprise Edition";
				else if ( osvi.wSuiteMask & VER_SUITE_BLADE )
					retval += L"Web Edition";
				else
					retval += L"Standard Edition";
			}
		}
	}

	if ( osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 1 )
	{
		retval += L"Windows XP ";
		retval += ( osvi.wSuiteMask & VER_SUITE_PERSONAL ) ? L"Professional" : L"Home Edition";
	}

	if ( osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 0 )
	{
		retval += L"Windows 2000 ";

		if ( osvi.wProductType == VER_NT_WORKSTATION )
		{
			retval += L"Professional";
		}
		else
		{
			if( osvi.wSuiteMask & VER_SUITE_DATACENTER )
				retval += L"Datacenter Server";
			else if( osvi.wSuiteMask & VER_SUITE_ENTERPRISE )
				retval += L"Advanced Server";
			else
				retval += L"Server";
		}
	}

	// Include service pack (if any) and build number.

	if( _tcslen(osvi.szCSDVersion) > 0 )
		retval += (wxString)L" " + osvi.szCSDVersion;

	retval += wxsFormat( L" (build %d)", osvi.dwBuildNumber );

	if ( osvi.dwMajorVersion >= 6 )
	{
		if ( si.wProcessorArchitecture==PROCESSOR_ARCHITECTURE_AMD64 )
			retval += L", 64-bit";
		else if (si.wProcessorArchitecture==PROCESSOR_ARCHITECTURE_INTEL )
			retval += L", 32-bit";
	}

	return retval;
}
