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
	const std::string iniFile(s_strIniPath + "zzogl-pg.ini");
	
	u32 tempHacks = conf.hacks._u32 & ~conf.def_hacks._u32;
	FILE* f = fopen(iniFile.c_str(), "w");

	if (f == NULL)
	{
		printf("failed to open %s\n", iniFile.c_str());
		return;
	}

	fprintf(f, "interlace = %hhx\n", conf.interlace);

	fprintf(f, "mrtdepth = %hhx\n", conf.mrtdepth);
	fprintf(f, "zzoptions = %x\n", conf.zz_options); //u32
	fprintf(f, "options = %x\n", tempHacks); //u32
	fprintf(f, "bilinear  = %hhx\n", conf.bilinear);
	fprintf(f, "aliasing = %hhx\n", conf.aa);
	//fprintf(f, "gamesettings = %x\n", conf.def_hacks); //u32
	fprintf(f, "width = %x\n", conf.width); //u32
	fprintf(f, "height = %x\n", conf.height); //u32
	fprintf(f, "x = %x\n", conf.x); //u32
	fprintf(f, "y = %x\n", conf.y); //u32
	fprintf(f, "log = %x\n", conf.log); //u32
	fclose(f);
}

void LoadConfig()
{
	int err = 0;
	memset(&conf, 0, sizeof(conf));
	conf.interlace = 0; // on, mode 1
	conf.mrtdepth = 1;
	conf.zz_options._u32 = 0;
	conf.hacks._u32 = 0;
	conf.bilinear = 1;
	conf.width = 640;
	conf.height = 480;
	conf.x = 0;
	conf.y = 0;
	conf.aa = 0;
	conf.log = 1;

	const std::string iniFile(s_strIniPath + "zzogl-pg.ini");
	FILE* f = fopen(iniFile.c_str(), "r");

	if (f == NULL)
	{
		printf("failed to open %s\n", iniFile.c_str());
		SaveConfig();//save and return
		return;
	}

	err = fscanf(f, "interlace = %hhx\n", &conf.interlace);

	err = fscanf(f, "mrtdepth = %hhx\n", &conf.mrtdepth);
	err = fscanf(f, "zzoptions = %x\n", &conf.zz_options);//u32
	err = fscanf(f, "options = %x\n", &conf.hacks);//u32
	err = fscanf(f, "bilinear = %hhx\n", &conf.bilinear);
	err = fscanf(f, "aliasing = %hhx\n", &conf.aa);
	//err = fscanf(f, "gamesettings = %x\n", &conf.gamesettings);//u32
	err = fscanf(f, "width = %x\n", &conf.width);//u32
	err = fscanf(f, "height = %x\n", &conf.height);//u32
	err = fscanf(f, "x = %x\n", &conf.x);//u32
	err = fscanf(f, "y = %x\n", &conf.y);//u32
	err = fscanf(f, "log = %x\n", &conf.log);//u32
	fclose(f);

	// filter bad files

	if (conf.aa > 4) conf.aa = 0;

	conf.isWideScreen = conf.widescreen();

	switch (conf.zz_options.dimensions)
	{

		case GSDim_640:
			conf.width = 640;
			conf.height = conf.isWideScreen ? 360 : 480;
			break;

		case GSDim_800:
			conf.width = 800;
			conf.height = conf.isWideScreen ? 450 : 600;
			break;

		case GSDim_1024:
			conf.width = 1024;
			conf.height = conf.isWideScreen ? 576 : 768;
			break;

		case GSDim_1280:
			conf.width = 1280;
			conf.height = conf.isWideScreen ? 720 : 960;
			break;
	}

	// turn off all hacks by default
	conf.setWireframe(false);
	conf.setCaptureAvi(false);
	conf.setLoaded(true);

	if (conf.width <= 0 || conf.height <= 0)
	{
		conf.width = 640;
		conf.height = 480;
	}

	if (conf.x <= 0 || conf.y <= 0)
	{
		conf.x = 0;
		conf.y = 0;
	}
}

