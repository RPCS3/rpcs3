/*  FWnull
 *  Copyright (C) 2004-2009 PCSX2 Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <string>
using namespace std;

#include "FW.h"
#include "Config.h"

extern string s_strIniPath;
extern PluginLog FWLog;
PluginConf Ini;

void setLoggingState()
{
	if (conf.Log)
	{
		FWLog.WriteToConsole = true;
		FWLog.WriteToFile = true;
	}
	else
	{
		FWLog.WriteToConsole = false;
		FWLog.WriteToFile = false;
	}
}

EXPORT_C_(void) FWabout() 
{
	SysMessage("FWnull: A simple null plugin.");
}

EXPORT_C_(void) FWconfigure() 
{
	LoadConfig();
	PluginNullConfigure("Since this is a null plugin, all that is really configurable is logging.", conf.Log);
	SaveConfig();
}

void LoadConfig() 
{
	string IniPath = s_strIniPath + "/FWnull.ini";
	if (!Ini.Open(IniPath, READ_FILE))
	{
		FWLog.WriteLn("Failed to open %s", IniPath.c_str());
		SaveConfig();
		return;
	}
	
	conf.Log = Ini.ReadInt("logging");
	setLoggingState();
	Ini.Close();
}

void SaveConfig() 
{
	string IniPath = s_strIniPath + "/FWnull.ini";
	if (!Ini.Open(IniPath, WRITE_FILE))
	{
		FWLog.WriteLn("Failed to open %s\n", IniPath.c_str());
		return;
	}
	
	Ini.WriteInt("logging", conf.Log);
	Ini.Close();
}

