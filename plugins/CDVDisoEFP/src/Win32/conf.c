/*  conf.c
 *  Copyright (C) 2002-2005  PCSX2 Team
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
 *
 *  PCSX2 members can be contacted through their website at www.pcsx2.net.
 */

#include <errno.h> // errno
#include <stddef.h> // NULL
#include <stdio.h> // sprintf()
#include <stdlib.h> // getenv()
#include <string.h> // strerror()
#include <sys/stat.h> // mkdir(), stat()
#include <sys/types.h> // mkdir(), stat(), fork()
#include <unistd.h> // stat(), fork(), execlp()

#include <windows.h> // CreateProcess()

// #define CDVDdefs
// #include "../PS2Edefs.h"
#include "../PS2Etypes.h" // u8
#include "logfile.h"
#include "../ini.h"
#include "conf.h"


const char *confnames[] = { "IsoFile", "Device", "OpenOnStart", "OpenOnRestart", NULL };
const u8 defaultdevice[] = DEFAULT_DEVICE;
const char defaulthome[] = "inis";
const char defaultdirectory[] = "HideMe.PS2E";
const char defaultfile[] = "CDVDisoEFP.ini";

char confdirname[256];
char conffilename[256];

CDVDconf conf;


void InitConf()
{
	DWORD retval;
	int i;
	int pos;
	char *envptr;

#ifdef VERBOSE_FUNCTION_CONF
	PrintLog("CDVD config: InitConf()");
#endif /* VERBOSE_FUNCTION_CONF */

	conf.isoname[0] = 0; // Empty the iso name

	i = 0;
	while ((i < 255) && defaultdevice[i] != 0)
	{
		conf.devicename[i] = defaultdevice[i];
		i++;
	} // ENDWHILE- copying the default CD/DVD name in
	conf.devicename[i] = 0; // 0-terminate the device name

	// Locating directory and file positions
	pos = 0;
	envptr = NULL;
	// envptr = getenv("HOME");
	if (envptr == NULL)
	{
		// = <Default Home>
		retval = GetCurrentDirectory(253, confdirname);
		if (retval > 0)
		{
			pos = retval;
		}
		else
		{
			pos = 2;
			confdirname[0] = '.';
			confdirname[1] = '\\';
		} // ENDIF- Did we retrieve a directory reference?

		i = 0;
		while (i < pos)
		{
			conffilename[i] = confdirname[i];
			i++;
		} // ENDWHILE- Copying dir info (so far) into file info

		if (confdirname[pos-1] != '\\')
		{
			confdirname[pos] = '\\';
			conffilename[pos] = '\\';
			pos++;
		} // ENDIF- No directory separator here? Add one.

		i = 0;
		while ((pos < 253) && (defaulthome[i] != 0))
		{
			confdirname[pos] = defaulthome[i];
			conffilename[pos] = defaulthome[i];
			pos++;
			i++;
		} // ENDWHILE- putting an offset where to store ini data

	}
	else
	{
		// = <Env Home>/<Default Directory>
		i = 0;
		while ((pos < 253) && (*(envptr + i) != 0))
		{
			confdirname[pos] = *(envptr + i);
			conffilename[pos] = *(envptr + i);
			pos++;
			i++;
		} // ENDWHILE- copying home directory info in

		if (confdirname[pos-1] != '\\')
		{
			confdirname[pos] = '\\';
			conffilename[pos] = '\\';
			pos++;
		} // ENDIF- No directory separator here? Add one.

		i = 0;
		while ((pos < 253) && (defaultdirectory[i] != 0))
		{
			confdirname[pos] = defaultdirectory[i];
			conffilename[pos] = defaultdirectory[i];
			pos++;
			i++;
		} // NEXT- putting a default place to store configuration data
	} // ENDIF- No Home directory?

	confdirname[pos] = 0; // Directory reference finished

	// += /<Config File Name>
	if (conffilename[pos-1] != '\\')
	{
		conffilename[pos] = '\\';
		pos++;
	} // ENDIF- No directory separator here? Add one.

	i = 0;
	while ((pos < 253) && (defaultfile[i] != 0))
	{
		conffilename[pos] = defaultfile[i];
		pos++;
		i++;
	} // NEXT- putting a default place to store configuration data

	conffilename[pos] = 0; // File reference finished

#ifdef VERBOSE_FUNCTION_CONF
	PrintLog("CDVD config:   Directory: %s", confdirname);
	PrintLog("CDVD config:   File: %s", conffilename);
#endif /* VERBOSE_FUNCTION_CONF */
} // END InitConf()


void LoadConf()
{
	int retval;

#ifdef VERBOSE_FUNCTION_CONF
	PrintLog("CDVD config: LoadConf()");
#endif /* VERBOSE_FUNCTION_CONF */

	retval = INILoadString(conffilename, "Settings", "IsoFile", conf.isoname);
	if (retval < 0)
	{
		sprintf(conf.isoname, "[Put an Image File here]");
	} // ENDIF- Couldn't find keyword? Fill in a default

	retval = INILoadString(conffilename, "Settings", "Device", conf.devicename);
	if (retval < 0)
	{
		sprintf(conf.devicename, "D:");
	} // ENDIF- Couldn't find keyword? Fill in a default

	retval = INILoadUInt(conffilename, "Settings", "OpenOnStart", &conf.startconfigure);
	if (retval < 0)
	{
		conf.startconfigure = 0; // FALSE
	} // ENDIF- Couldn't find keyword? Fill in a default

	retval = INILoadUInt(conffilename, "Settings", "OpenOnRestart", &conf.restartconfigure);
	if (retval < 0)
	{
		conf.restartconfigure = 1; // TRUE
	} // ENDIF- Couldn't find keyword? Fill in a default
} // END LoadConf()


void SaveConf()
{
#ifdef VERBOSE_FUNCTION_CONF
	PrintLog("CDVD config: SaveConf()");
#endif /* VERBOSE_FUNCTION_CONF */

	mkdir(confdirname);

	INISaveString(conffilename, "Settings", "IsoFile", conf.isoname);
	INISaveString(conffilename, "Settings", "Device", conf.devicename);
	INISaveUInt(conffilename, "Settings", "OpenOnStart", conf.startconfigure);
	INISaveUInt(conffilename, "Settings", "OpenOnRestart", conf.restartconfigure);
} // END SaveConf()
