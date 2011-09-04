/* SPU2-X, A plugin for Emulating the Sound Processing Unit of the Playstation 2
 * Developed and maintained by the Pcsx2 Development Team.
 *
 * Original portions from SPU2ghz are (c) 2008 by David Quintana [gigaherz]
 *
 * SPU2-X is free software: you can redistribute it and/or modify it under the terms
 * of the GNU Lesser General Public License as published by the Free Software Found-
 * ation, either version 3 of the License, or (at your option) any later version.
 *
 * SPU2-X is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with SPU2-X.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "../USB.h"

void SysMessage(const char *fmt, ...)
{
	va_list list;
	char tmp[512];
	wchar_t wtmp[512];

	va_start(list,fmt);
	vsprintf_s(tmp,fmt,list);
	va_end(list);
	swprintf_s(wtmp, L"%S", tmp);
	MessageBox( (!!gsWindowHandle) ? (HWND)gsWindowHandle : GetActiveWindow(), wtmp,
		L"USBqemu System Message", MB_OK | MB_SETFOREGROUND);
}

void SysMessage(const wchar_t *fmt, ...)
{
	va_list list;
	wchar_t tmp[512];

	va_start(list,fmt);
	vswprintf_s(tmp,fmt,list);
	va_end(list);

	MessageBox( (!!gsWindowHandle) ? (HWND)gsWindowHandle : GetActiveWindow(), tmp,
		L"USBqemu System Message", MB_OK | MB_SETFOREGROUND);
}

//////

static TCHAR CfgFile[260] = L"inis/USBqemu.ini";

void CfgSetSettingsDir( const char* dir )
{
	if(dir == NULL)
	{
		wcscpy(CfgFile, L"inis/USBqemu.ini");
	}
	else
	{
		swprintf_s(CfgFile, L"%S", dir);
		TCHAR term = CfgFile[wcslen(CfgFile)-1];
		if(term != L'\\' && term != L'/')
			wcscat(CfgFile, L"\\");
		wcscat(CfgFile, L"USBQemu.ini" );
	}
}


/*¯| Config File Format: |¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
+--+---------------------+------------------------+
|												  |
| Option=Value									  |
|												  |
|												  |
| Boolean Values: TRUE,YES,1,T,Y mean 'true',	  |
|                 everything else means 'false'.  |
|												  |
| All Values are limited to 255 chars.			  |
|												  |
+-------------------------------------------------+
\*______________________________________________*/


void CfgWriteBool(const TCHAR* Section, const TCHAR* Name, bool Value)
{
	const TCHAR *Data = Value ? L"TRUE" : L"FALSE";
	WritePrivateProfileString( Section, Name, Data, CfgFile );
}

void CfgWriteInt(const TCHAR* Section, const TCHAR* Name, int Value)
{
	TCHAR Data[32];
	_itow( Value, Data, 10 );
	WritePrivateProfileString(Section,Name,Data,CfgFile);
}

void CfgWriteStr(const TCHAR* Section, const TCHAR* Name, const TCHAR *Data)
{
	WritePrivateProfileString( Section, Name, Data, CfgFile );
}


/*****************************************************************************/

bool CfgReadBool(const TCHAR *Section,const TCHAR* Name, bool Default)
{
	TCHAR Data[255] = {0};

	GetPrivateProfileString( Section, Name, L"", Data, 255, CfgFile );
	Data[254]=0;
	if(wcslen(Data)==0) {
		CfgWriteBool(Section,Name,Default);
		return Default;
	}

	if(wcscmp(Data,L"1")==0) return true;
	if(wcscmp(Data,L"Y")==0) return true;
	if(wcscmp(Data,L"T")==0) return true;
	if(wcscmp(Data,L"YES")==0) return true;
	if(wcscmp(Data,L"TRUE")==0) return true;
	return false;
}


int CfgReadInt(const TCHAR* Section, const TCHAR* Name,int Default)
{
	TCHAR Data[255]={0};
	GetPrivateProfileString(Section,Name,L"",Data,255,CfgFile);
	Data[254]=0;

	if(wcslen(Data)==0) {
		CfgWriteInt(Section,Name,Default);
		return Default;
	}

	return _wtoi(Data);
}

void CfgReadStr(const TCHAR* Section, const TCHAR* Name, TCHAR* Data, int DataSize, const TCHAR* Default)
{
	int chars = GetPrivateProfileString(Section,Name,L"",Data,DataSize,CfgFile);
	
	if(!chars)
	{
		wcscpy(Data, Default);
		CfgWriteStr( Section, Name, Default );
	}
}

// Tries to read the requested value.
// Returns FALSE if the value isn't found.
bool CfgFindName( const TCHAR *Section, const TCHAR* Name)
{
	// Only load 24 characters.  No need to load more.
	TCHAR Data[24]={0};
	GetPrivateProfileString(Section,Name,L"",Data,24,CfgFile);
	Data[23]=0;

	if(wcslen(Data)==0) return false;
	return true;
}
