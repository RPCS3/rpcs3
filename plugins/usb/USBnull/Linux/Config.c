/*  USBnull
 *  Copyright (C) 2002-2004  USBnull Team
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

#include "USB.h"

void LoadConfig() {
/*	FILE *f;
	char cfg[256];

    sprintf(cfg, "%s/.PS2E/CDVDiso.cfg", getenv("HOME"));
	f = fopen(cfg, "r");
	if (f == NULL) {
		strcpy(IsoFile, DEV_DEF);
		strcpy(CdDev, CDDEV_DEF);
		return;
	}
	fscanf(f, "IsoFile = %[^\n]\n", IsoFile);
	fscanf(f, "CdDev   = %[^\n]\n", CdDev);
	if (!strncmp(IsoFile, "CdDev   =", 9)) *IsoFile = 0; // quick fix
	if (*CdDev == 0) strcpy(CdDev, CDDEV_DEF);
	fclose(f);*/
}

void SaveConfig() {
/*	FILE *f;
	char cfg[256];

    sprintf(cfg, "%s/.PS2E", getenv("HOME"));
	mkdir(cfg, 0755);
    sprintf(cfg, "%s/.PS2E/CDVDiso.cfg", getenv("HOME"));
	f = fopen(cfg, "w");
	if (f == NULL)
		return;
	fprintf(f, "IsoFile = %s\n", IsoFile);
	fprintf(f, "CdDev   = %s\n", CdDev);
	fclose(f);*/
}

