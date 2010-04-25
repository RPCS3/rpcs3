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

 wxConfigBase *oldConfig;
 wxString path;

 wxFileConfig *setIni(const wchar_t* Section)
 {
 	wxConfig *tempConfig;

	oldConfig = wxConfigBase::Get(false);

	tempConfig = new wxFileConfig(L"", L"", path, L"", wxCONFIG_USE_LOCAL_FILE);
	wxConfigBase::Set(tempConfig);
	tempConfig->SetPath(L"/");
	tempConfig->SetPath(Section);
	return tempConfig;
 }

 void writeIni(wxFileConfig *tempConfig)
 {
 	tempConfig->Flush();

	if (oldConfig != NULL) wxConfigBase::Set(oldConfig);
	delete tempConfig;
 }

void CfgSetSettingsDir(const char* dir)
{
	path = wxString::FromAscii(dir) + L"spu2-x.ini";

}

void CfgWriteBool(const wchar_t* Section, const wchar_t* Name, bool Value)
{
	wxConfig *config = setIni(Section);
	config->Write(Name, Value);
	writeIni(config);
}

void CfgWriteInt(const wchar_t* Section, const wchar_t* Name, int Value)
{
	wxConfig *config = setIni(Section);
	config->Write(Name, Value);
	writeIni(config);
}

void CfgWriteStr(const wchar_t* Section, const wchar_t* Name, const wstring& Data)
{
	wxConfig *config = setIni(Section);
	config->Write(Name, Data);
	writeIni(config);
}

bool CfgReadBool(const wchar_t *Section,const wchar_t* Name, bool Default)
{
	bool ret;

	wxConfig *config = setIni(Section);
	config->Read(Name, &ret, Default);
	writeIni(config);

	return ret;
}

int CfgReadInt(const wchar_t* Section, const wchar_t* Name,int Default)
{
	int ret;

	wxConfig *config = setIni(Section);
	config->Read(Name, &ret, Default);
	writeIni(config);

	return ret;
}

void CfgReadStr(const wchar_t* Section, const wchar_t* Name, wchar_t* Data, int DataSize, const wchar_t* Default)
{
	wxConfig *config = setIni(Section);
	wcscpy(Data, config->Read(Name, Default));
	writeIni(config);
}

void CfgReadStr(const wchar_t* Section, const wchar_t* Name, wxString& Data, int DataSize, const wchar_t* Default)
{
	wxConfig *config = setIni(Section);
	Data = config->Read(Name, Default);
	writeIni(config);
}

void CfgReadStr(const wchar_t* Section, const wchar_t* Name, wstring& Data, int DataSize, const wchar_t* Default)
{
	wxString temp;
	CfgReadStr(Section, Name, temp, DataSize, Default);
	Data = temp.c_str();
}

