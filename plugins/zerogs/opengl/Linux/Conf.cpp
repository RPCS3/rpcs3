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

void SaveConfig() 
{
	const std::string iniFile( s_strIniPath + "zerogs.ini" );

	FILE* f = fopen(iniFile.c_str(),"w");
	if (f == NULL) 
	{
		printf("failed to open %s\n", iniFile.c_str());
		return;
	}
	
	fprintf(f, "interlace = %hhx\n", conf.interlace);
	fprintf(f, "mrtdepth = %hhx\n", conf.mrtdepth);
	fprintf(f, "options = %x\n", conf.options); //u32
	fprintf(f, "bilinear  = %hhx\n", conf.bilinear);
	fprintf(f, "aliasing = %hhx\n", conf.aa);
	fprintf(f, "gamesettings = %x\n", conf.gamesettings); //u32
	fclose(f);
}

void LoadConfig() 
{
	char cfg[255];

	memset(&conf, 0, sizeof(conf));
	conf.interlace = 0; // on, mode 1
	conf.mrtdepth = 1;
	conf.options = 0;
	conf.bilinear = 1;
	conf.width = 640;
	conf.height = 480;
	conf.aa = 0;

	const std::string iniFile( s_strIniPath + "zerogs.ini" );
	FILE* f = fopen(iniFile.c_str(), "r");
	if (f == NULL) 
	{
		printf("failed to open %s\n", iniFile.c_str());
		SaveConfig();//save and return
		return;
	}
	fscanf(f, "interlace = %hhx\n", &conf.interlace);
	fscanf(f, "mrtdepth = %hhx\n", &conf.mrtdepth);
	fscanf(f, "options = %x\n", &conf.options);//u32
	fscanf(f, "bilinear = %hhx\n", &conf.bilinear);
	fscanf(f, "aliasing = %hhx\n", &conf.aa);
	fscanf(f, "gamesettings = %x\n", &conf.gamesettings);//u32
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

