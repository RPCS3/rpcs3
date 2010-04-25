/*  GSnull
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

#include "GS.h"
#include "Config.h"

void SysMessage(char *fmt, ...)
{
	va_list list;
	char msg[512];
	char cmd[512];

	va_start(list, fmt);
	vsprintf(msg, fmt, list);
	va_end(list);

	cfgSysMessage(msg);
}

void GSconfigure()
{
	CFGconfigure();
}

void GSabout()
{
	CFGabout();
}

