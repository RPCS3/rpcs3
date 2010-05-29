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

 #include "Dialogs.h"
 #include <wx/fileconf.h>

 wxFileConfig *spuConfig = NULL;
 wxString path(L"~/.pcsx2/inis/spu2-x.ini");
 bool pathSet = false;

void initIni()
{
	if (spuConfig == NULL) spuConfig = new wxFileConfig(L"", L"", path, L"", wxCONFIG_USE_LOCAL_FILE);
}

void setIni(const wchar_t* Section)
 {
	initIni();
	spuConfig->SetPath(wxsFormat(L"/%s", Section));
 }

void CfgSetSettingsDir(const char* dir)
{
	FileLog("CfgSetSettingsDir(%s)\n", dir);
	path = wxString::FromAscii(dir) + L"/spu2-x.ini";
	pathSet = true;
}

void CfgWriteBool(const wchar_t* Section, const wchar_t* Name, bool Value)
{
	setIni(Section);
	spuConfig->Write(Name, Value);
}

void CfgWriteInt(const wchar_t* Section, const wchar_t* Name, int Value)
{
	setIni(Section);
	spuConfig->Write(Name, Value);
}

void CfgWriteStr(const wchar_t* Section, const wchar_t* Name, const wxString& Data)
{
	setIni(Section);
	spuConfig->Write(Name, Data);
}

bool CfgReadBool(const wchar_t *Section,const wchar_t* Name, bool Default)
{
	bool ret;

	setIni(Section);
	spuConfig->Read(Name, &ret, Default);

	return ret;
}

int CfgReadInt(const wchar_t* Section, const wchar_t* Name,int Default)
{
	int ret;

	setIni(Section);
	spuConfig->Read(Name, &ret, Default);

	return ret;
}

void CfgReadStr(const wchar_t* Section, const wchar_t* Name, wchar_t* Data, int DataSize, const wchar_t* Default)
{
	setIni(Section);
	wcscpy(Data, spuConfig->Read(Name, Default));
}

void CfgReadStr(const wchar_t* Section, const wchar_t* Name, wxString& Data, int DataSize, const wchar_t* Default)
{
	setIni(Section);
	Data = spuConfig->Read(Name, Default);
}

void CfgReadStr(const wchar_t* Section, const wchar_t* Name, wxString& Data, int DataSize, const wchar_t* Default)
{
	wxString temp;
	CfgReadStr(Section, Name, temp, DataSize, Default);
	Data = temp.c_str();
}

