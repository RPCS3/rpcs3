/* SPU2-X, A plugin for Emulating the Sound Processing Unit of the Playstation 2
 * Developed and maintained by the Pcsx2 Development Team.
 * 
 * Original portions from SPU2ghz are (c) 2008 by David Quintana [gigaherz]
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free 
 * Software Foundation; either version 2.1 of the the License, or (at your
 * option) any later version.
 * 
 * This library is distributed in the hope that it will be useful, but WITHOUT 
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License
 * for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public License along
 * with this library; if not, write to the Free Software Foundation, Inc., 59
 * Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * 
 */

#include "dialogs.h"

//////

const TCHAR CfgFile[] = _T("inis\\SPU2-X.ini");


/*| Config File Format: |¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
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
\*_____________________________________________*/


void CfgWriteBool(const TCHAR* Section, const TCHAR* Name, bool Value)
{
	const TCHAR *Data = Value ? _T("TRUE") : _T("FALSE");
	WritePrivateProfileString( Section, Name, Data, CfgFile );
}

void CfgWriteInt(const TCHAR* Section, const TCHAR* Name, int Value)
{
	TCHAR Data[32];
	_itow( Value, Data, 10 );
	WritePrivateProfileString(Section,Name,Data,CfgFile);
}

/*void CfgWriteStr(const TCHAR* Section, const TCHAR* Name, const TCHAR *Data)
{
WritePrivateProfileString( Section, Name, Data, CfgFile );
}*/

void CfgWriteStr(const TCHAR* Section, const TCHAR* Name, const wstring& Data)
{
	WritePrivateProfileString( Section, Name, Data.c_str(), CfgFile );
}

/*****************************************************************************/

bool CfgReadBool(const TCHAR *Section,const TCHAR* Name, bool Default)
{
	TCHAR Data[255] = {0};

	GetPrivateProfileString( Section, Name, _T(""), Data, 255, CfgFile );
	Data[254]=0;
	if(wcslen(Data)==0) {
		CfgWriteBool(Section,Name,Default);
		return Default;
	}

	if(wcscmp(Data,_T("1"))==0) return true;
	if(wcscmp(Data,_T("Y"))==0) return true;
	if(wcscmp(Data,_T("T"))==0) return true;
	if(wcscmp(Data,_T("YES"))==0) return true;
	if(wcscmp(Data,_T("TRUE"))==0) return true;
	return false;
}


int CfgReadInt(const TCHAR* Section, const TCHAR* Name,int Default)
{
	TCHAR Data[255]={0};
	GetPrivateProfileString(Section,Name,_T(""),Data,255,CfgFile);
	Data[254]=0;

	if(wcslen(Data)==0) {
		CfgWriteInt(Section,Name,Default);
		return Default;
	}

	return _wtoi(Data);
}

void CfgReadStr(const TCHAR* Section, const TCHAR* Name, TCHAR* Data, int DataSize, const TCHAR* Default)
{
	GetPrivateProfileString(Section,Name,_T(""),Data,DataSize,CfgFile);

	if(wcslen(Data)==0) { 
		swprintf_s( Data, DataSize, _T("%s"), Default );
		CfgWriteStr( Section, Name, Data );
	}
}

void CfgReadStr(const TCHAR* Section, const TCHAR* Name, wstring Data, int DataSize, const TCHAR* Default)
{
	wchar_t workspace[512];
	GetPrivateProfileString(Section,Name,_T(""),workspace,DataSize,CfgFile);

	Data = workspace;

	if(Data.empty())
	{ 
		Data = Default;
		CfgWriteStr( Section, Name, Default );
	}
}

// Tries to read the requested value.
// Returns FALSE if the value isn't found.
bool CfgFindName( const TCHAR *Section, const TCHAR* Name)
{
	// Only load 24 characters.  No need to load more.
	TCHAR Data[24]={0};
	GetPrivateProfileString(Section,Name,_T(""),Data,24,CfgFile);
	Data[23]=0;

	if(wcslen(Data)==0) return false;
	return true;
}
