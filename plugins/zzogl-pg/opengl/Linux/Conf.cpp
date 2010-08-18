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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
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
		ZZLog::Error_Log("Failed to open '%s'", iniFile.c_str());
		return;
	}

	fprintf(f, "interlace = %hhx\n", conf.interlace);

	fprintf(f, "mrtdepth = %hhx\n", conf.mrtdepth);
	fprintf(f, "zzoptions = %x\n", conf.zz_options._u32);
	fprintf(f, "options = %x\n", tempHacks);
	fprintf(f, "bilinear  = %hhx\n", conf.bilinear);
	fprintf(f, "aliasing = %hhx\n", conf.aa);
	fprintf(f, "width = %x\n", conf.width);
	fprintf(f, "height = %x\n", conf.height);
	fprintf(f, "x = %x\n", conf.x);
	fprintf(f, "y = %x\n", conf.y);
	fprintf(f, "log = %x\n", conf.log);
	fprintf(f, "skipdraw = %x\n", conf.SkipDraw);
	fclose(f);
}

void LoadConfig()
{
	int err = 0;
	memset(&conf, 0, sizeof(conf));
	conf.interlace = 0; // on, mode 1
	conf.mrtdepth = 1;
	conf.bilinear = 1;
	conf.log = 1;
	conf.SkipDraw = 0;

	const std::string iniFile(s_strIniPath + "zzogl-pg.ini");
	FILE* f = fopen(iniFile.c_str(), "r");

	if (f == NULL)
	{
		ZZLog::Error_Log("Failed to open '%s'", iniFile.c_str());
		SaveConfig();//save and return
		return;
	}

	err = fscanf(f, "interlace = %hhx\n", &conf.interlace);

	err = fscanf(f, "mrtdepth = %hhx\n", &conf.mrtdepth);
	err = fscanf(f, "zzoptions = %x\n", &conf.zz_options._u32);
	err = fscanf(f, "options = %x\n", &conf.hacks._u32);
	err = fscanf(f, "bilinear = %hhx\n", &conf.bilinear);
	err = fscanf(f, "aliasing = %hhx\n", &conf.aa);
	err = fscanf(f, "width = %x\n", &conf.width);
	err = fscanf(f, "height = %x\n", &conf.height);
	err = fscanf(f, "x = %x\n", &conf.x);
	err = fscanf(f, "y = %x\n", &conf.y);
	err = fscanf(f, "log = %x\n", &conf.log);
	err = fscanf(f, "skipdraw = %x\n", &conf.SkipDraw);
	fclose(f);

	// turn off all hacks by default
	conf.setWireframe(false);
	conf.setCaptureAvi(false);
	conf.setLoaded(true);
	
	conf.isWideScreen = conf.widescreen();
	
	// filter bad files
	if (conf.interlace > 2) conf.interlace = 0;
	if (conf.aa > 4) conf.aa = 0;
	if (conf.width <= 0 || conf.height <= 0)
	{
		conf.set_dimensions(conf.zz_options.dimensions);
	}

	if (conf.x < 0 || conf.y < 0)
	{
		conf.x = 0;
		conf.y = 0;
	}
}

