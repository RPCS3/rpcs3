/*  PADnull
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

#include "Pad.h"
#include "Config.h"

extern string s_strIniPath;
PluginConf Ini;

EXPORT_C_(void) PADabout()
{
	SysMessage("PADnull: A simple null plugin.");
}

EXPORT_C_(void) PADconfigure()
{
	LoadConfig();
	PluginNullConfigure("Since this is a null plugin, all that is really configurable is logging.", conf.Log);
	SaveConfig();
}

void LoadConfig()
{
    const std::string iniFile(s_strIniPath + "/Padnull.ini");

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
    const std::string iniFile(s_strIniPath + "/Padnull.ini");

	if (!Ini.Open(iniFile, WRITE_FILE))
	{
		printf("failed to open %s\n", iniFile.c_str());
		return;
	}

	Ini.WriteInt("logging", conf.Log);
	Ini.Close();
}
