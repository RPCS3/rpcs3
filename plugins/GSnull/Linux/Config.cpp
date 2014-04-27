/*  GSnull
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <string>
using namespace std;

#include "GS.h"
#include "Config.h"

extern string s_strIniPath;
PluginConf Ini;

void CFGabout()
{
	SysMessage("GSnull: A simple null plugin.");
}

void CFGconfigure()
{
	LoadConfig();
	PluginNullConfigure("Since this is a null plugin, all that is really configurable is logging.", conf.Log);
	SaveConfig();
}

void LoadConfig()
{
    const std::string iniFile(s_strIniPath + "/GSNull.ini");

	if (!Ini.Open(iniFile, READ_FILE))
	{
		printf("failed to open %s\n", iniFile.c_str());
		SaveConfig();//save and return
		return;
	}

	conf.Log = Ini.ReadInt("logging", 0);
	Ini.Close();
}

void SaveConfig()
{
    const std::string iniFile(s_strIniPath + "/GSNull.ini");

	if (!Ini.Open(iniFile, WRITE_FILE))
	{
		printf("failed to open %s\n", iniFile.c_str());
		return;
	}

	Ini.WriteInt("logging", conf.Log);
	Ini.Close();
}
