/*  USBnull
 *  Copyright (C) 2002-2010  PCSX2 Dev Team
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

#include <string>
using namespace std;

#include "USB.h"
#include "Config.h"

extern string s_strIniPath;
extern PluginLog USBLog;
PluginConf Ini;

void setLoggingState()
{
	if (conf.Log)
	{
		USBLog.WriteToConsole = true;
		USBLog.WriteToFile = true;
	}
	else
	{
		USBLog.WriteToConsole = false;
		USBLog.WriteToFile = false;
	}
}

EXPORT_C_(void) USBabout()
{
	SysMessage("USBnull: A simple null plugin.");
}

EXPORT_C_(void) USBconfigure()
{
	LoadConfig();
	PluginNullConfigure("Since this is a null plugin, all that is really configurable is logging.", conf.Log);
	SaveConfig();
}

void LoadConfig()
{
	string IniPath = s_strIniPath + "/USBnull.ini";
	if (!Ini.Open(IniPath, READ_FILE))
	{
		USBLog.WriteLn("Failed to open %s", IniPath.c_str());
		SaveConfig();
		return;
	}

	conf.Log = Ini.ReadInt("logging", 0);
	setLoggingState();
	Ini.Close();
}

void SaveConfig()
{
	string IniPath = s_strIniPath + "/USBnull.ini";
	if (!Ini.Open(IniPath, WRITE_FILE))
	{
		USBLog.WriteLn("Failed to open %s", IniPath.c_str());
		return;
	}

	Ini.WriteInt("logging", conf.Log);
	Ini.Close();
}

