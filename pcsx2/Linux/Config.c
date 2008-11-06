/*  Pcsx2 - Pc Ps2 Emulator
 *  Copyright (C) 2002-2008  Pcsx2 Team
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "Linux.h"

#define GetValue(name, var) \
	tmp = strstr(data, name); \
	if (tmp != NULL) { \
		tmp+=strlen(name); \
		while ((*tmp == ' ') || (*tmp == '=')) tmp++; \
		if (*tmp != '\n') sscanf(tmp, "%s", var); \
	}

#define GetValuel(name, var) \
	tmp = strstr(data, name); \
	if (tmp != NULL) { \
		tmp+=strlen(name); \
		while ((*tmp == ' ') || (*tmp == '=')) tmp++; \
		if (*tmp != '\n') sscanf(tmp, "%x", &var); \
	}

#define SetValue(name, var) \
	fprintf (f,"%s = %s\n", name, var);

#define SetValuel(name, var) \
	fprintf (f,"%s = %x\n", name, var);

int LoadConfig() {
	struct stat buf;
	FILE *f;
	int size;
	char *data,*tmp;
	char strtemp[255];

	if (stat(cfgfile, &buf) == -1) return -1;
	size = buf.st_size;

	f = fopen(cfgfile,"r");
	if (f == NULL) return -1;

	data = (char*)malloc(size);
	if (data == NULL) return -1;

	fread(data, 1, size, f);
	fclose(f);

	GetValue("Bios", Config.Bios);
	Config.Lang[0] = 0;
	GetValue("Lang", Config.Lang);
	GetValuel("Ps2Out",     Config.PsxOut);
	GetValuel("ThPriority", Config.ThPriority);
	GetValue("PluginsDir", Config.PluginsDir);
	GetValue("BiosDir",    Config.BiosDir);
	GetValue("Mcd1", Config.Mcd1);
	GetValue("Mcd2", Config.Mcd2);

	// plugins
	GetValue("GS",   Config.GS);
	GetValue("SPU2", Config.SPU2);
	GetValue("CDVD", Config.CDVD);
	GetValue("PAD1", Config.PAD1);
	GetValue("PAD2", Config.PAD2);
	GetValue("DEV9", Config.DEV9);
	GetValue("USB",  Config.USB);
	GetValue("FW",  Config.FW);
	
	
	// cpu
	GetValuel("Options", Config.Options);

	GetValuel("Patch",      Config.Patch);

#ifdef PCSX2_DEVBUILD
	GetValuel("varLog", varLog);
#endif
	
	free(data);

#ifdef ENABLE_NLS
	if (Config.Lang[0]) {
		extern int _nl_msg_cat_cntr;

		setenv("LANGUAGE", Config.Lang, 1);
		++_nl_msg_cat_cntr;
	}
#endif

	return 0;
}

/////////////////////////////////////////////////////////

void SaveConfig() {
	FILE *f;

	f = fopen(cfgfile,"w");
	if (f == NULL) return;

	// interface
	SetValue("Bios", Config.Bios);
	SetValue("Lang",    Config.Lang);
	SetValue("PluginsDir", Config.PluginsDir);
	SetValue("BiosDir",    Config.BiosDir);
	SetValuel("Ps2Out",     Config.PsxOut);
	SetValuel("ThPriority", Config.ThPriority);
	SetValue("Mcd1", Config.Mcd1);
	SetValue("Mcd2", Config.Mcd2);
	// plugins
	SetValue("GS",   Config.GS);
	SetValue("SPU2", Config.SPU2);
	SetValue("CDVD", Config.CDVD);
	SetValue("PAD1", Config.PAD1);
	SetValue("PAD2", Config.PAD2);
	SetValue("DEV9", Config.DEV9);
	SetValue("USB",  Config.USB);
	SetValue("FW",  Config.FW);
	//cpu
	SetValuel("Options",        Config.Options);
	// misc
	SetValuel("Patch",      Config.Patch);

#ifdef PCSX2_DEVBUILD
	SetValuel("varLog", varLog);
#endif


	fclose(f);

	return;
}
