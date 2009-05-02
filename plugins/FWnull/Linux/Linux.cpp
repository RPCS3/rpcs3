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

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <pthread.h>
#include <unistd.h>

#include "FW.h"
#include "Config.h"

int ExecCfg(char *arg) 
{
	/*char cfg[256];
	struct stat buf;

	strcpy(cfg, "./cfgFWnull");
	if (stat(cfg, &buf) != -1) 
	{
		sprintf(cfg, "%s %s", cfg, arg);
		return system(cfg);
	}

	strcpy(cfg, "./plugins/cfgFWnull");
	if (stat(cfg, &buf) != -1) 
	{
		sprintf(cfg, "%s %s", cfg, arg);
		return system(cfg);
	}
	
	strcpy(cfg, "./cfg/cfgFWnull");
	if (stat(cfg, &buf) != -1) 
	{
		sprintf(cfg, "%s %s", cfg, arg);
		return system(cfg);
	}

	sprintf(cfg, "%s/cfgFWnull", getenv("HOME"));
	if (stat(cfg, &buf) != -1) 
	{
		sprintf(cfg, "%s %s", cfg, arg);
		return system(cfg);
	}

	printf("cfgFWnull file not found!\n");
	return -1;*/
}

void SysMessage(char *fmt, ...) 
{
	va_list list;
	char msg[512];
	char cmd[512];

	va_start(list, fmt);
	vsprintf(msg, fmt, list);
	va_end(list);

	//sprintf(cmd, "message \"%s\"", msg);
	cfgSysMessage(msg);
	//ExecCfg(cmd);
}

void FWconfigure() 
{
	//char *file;
	//getcwd(file, ArraySize(file));
	//chdir("plugins");
	//ExecCfg("configure");
	//chdir(file);
	CFGconfigure();
}

void FWabout() 
{
	//char *file;
	//getcwd(file, ArraySize(file));
	//chdir("plugins");
	//ExecCfg("about");
	//chdir(file);
	CFGabout();
}

