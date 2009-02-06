/*  GSsoft
 *  Copyright (C) 2002-2004  GSsoft Team
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

#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include "GS.h"

extern string s_strIniPath;

void SaveConfig() 
{
	FILE *f;
	char cfg[255];

	strcpy(cfg, s_strIniPath.c_str());
	f = fopen(cfg,"w");
	if (f == NULL) 
	{
		printf("failed to open %s\n", s_strIniPath.c_str());
		return;
	}
	
	fprintf(f, "interlace = %x\n", conf.interlace);
	fprintf(f, "mrtdepth = %x\n", conf.mrtdepth);
	fprintf(f, "options = %x\n", conf.options);
	fprintf(f, "bilinear  = %x\n", conf.bilinear);
	fprintf(f, "aliasing = %x\n", conf.aa);
	fprintf(f, "gamesettings = %x\n", conf.gamesettings);
	fclose(f);
}

void LoadConfig() 
{
	FILE *f;
	char cfg[255];

	memset(&conf, 0, sizeof(conf));
	conf.interlace = 0; // on, mode 1
	conf.mrtdepth = 1;
	conf.options = 0;
	conf.bilinear = 1;
	conf.width = 640;
	conf.height = 480;
	conf.aa = 0;

	strcpy(cfg, s_strIniPath.c_str());
	f = fopen(cfg, "r");
	if (f == NULL) 
	{
		printf("failed to open %s\n", s_strIniPath.c_str());
		SaveConfig();//save and return
		return;
	}
	fscanf(f, "interlace = %x\n", &conf.interlace);
	fscanf(f, "mrtdepth = %x\n", &conf.mrtdepth);
	fscanf(f, "options = %x\n", &conf.options);
	fscanf(f, "bilinear = %x\n", &conf.bilinear);
	fscanf(f, "aliasing = %x\n", &conf.aa);
	fscanf(f, "gamesettings = %x\n", &conf.gamesettings);
	fclose(f);

	// filter bad files
	if ((conf.aa < 0) || (conf.aa > 4)) conf.aa = 0;

	switch(conf.options & GSOPTION_WINDIMS) 
	{
		case GSOPTION_WIN640:
			conf.width = 640;
			conf.height = 480;
			break;
		case GSOPTION_WIN800:
			conf.width = 800;
			conf.height = 600;
			break;
		case GSOPTION_WIN1024:
			conf.width = 1024;
			conf.height = 768;
			break;
		case GSOPTION_WIN1280:
			conf.width = 1280;
			conf.height = 960;
			break;
	}

	// turn off all hacks by default
	conf.options &= ~(GSOPTION_FULLSCREEN | GSOPTION_WIREFRAME | GSOPTION_CAPTUREAVI);
	conf.options |= GSOPTION_LOADED;

	if( conf.width <= 0 || conf.height <= 0 ) 
	{
		conf.width = 640;
		conf.height = 480;
	}
}

