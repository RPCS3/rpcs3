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

#ifndef DIALOG_H_INCLUDED
#define DIALOG_H_INCLUDED

#include "../Global.h"
#include "../Config.h"

namespace DebugConfig
{
	extern void ReadSettings();
	extern void WriteSettings();
	extern void DisplayDialog();
}

extern void CfgSetSettingsDir(const char* dir);

extern void		CfgWriteBool(const wchar_t* Section, const wchar_t* Name, bool Value);
extern void		CfgWriteInt(const wchar_t* Section, const wchar_t* Name, int Value);
extern void		CfgWriteStr(const wchar_t* Section, const wchar_t* Name, const wstring& Data);

extern bool		CfgReadBool(const wchar_t *Section,const wchar_t* Name, bool Default);
extern void		CfgReadStr(const wchar_t* Section, const wchar_t* Name, wstring& Data, int DataSize, const wchar_t* Default);
//extern void		CfgReadStr(const wchar_t* Section, const wchar_t* Name, wchar_t* Data, int DataSize, const wchar_t* Default);
extern int		CfgReadInt(const wchar_t* Section, const wchar_t* Name,int Default);

#endif
