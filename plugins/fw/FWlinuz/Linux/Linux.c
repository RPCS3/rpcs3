/*  FireWire
 *  Copyright (C) 2002-2004  FireWire Team
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

#include "FireWire.h"

int ExecCfg(char *arg) {
	char cfg[256];
	struct stat buf;

	strcpy(cfg, "./cfgFireWire");
	if (stat(cfg, &buf) != -1) {
		sprintf(cfg, "%s %s", cfg, arg);
		return system(cfg);
	}

	strcpy(cfg, "./cfg/cfgFireWire");
	if (stat(cfg, &buf) != -1) {
		sprintf(cfg, "%s %s", cfg, arg);
		return system(cfg);
	}

	sprintf(cfg, "%s/cfgFireWire", getenv("HOME"));
	if (stat(cfg, &buf) != -1) {
		sprintf(cfg, "%s %s", cfg, arg);
		return system(cfg);
	}

	printf("cfgFireWire file not found!\n");
	return -1;
}

void SysMessage(char *fmt, ...) {
	va_list list;
	char msg[512];
	char cmd[512];

	va_start(list, fmt);
	vsprintf(msg, fmt, list);
	va_end(list);

	sprintf(cmd, "message \"%s\"", msg);
	ExecCfg(cmd);
}

void FireWireconfigure() {
	ExecCfg("configure");
}

void FireWireabout() {
	ExecCfg("about");
}

