/*  conf.h
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 *
 *  PCSX2 members can be contacted through their website at www.pcsx2.net.
 */
#ifndef CONF_H
#define CONF_H

#define CDVDdefs
#include "../PS2Edefs.h"

#define VERBOSE_FUNCTION_CONF

// Configuration Data

typedef struct
{
	u8 isoname[256];
	u8 devicename[256];
	unsigned int startconfigure;
	unsigned int restartconfigure;
} CDVDconf;
extern CDVDconf conf;

#define DEFAULT_DEVICE "K:\\"

// Configuration Functions
extern void InitConf();
extern void LoadConf();
extern void SaveConf();
extern void ExecCfg(char *arg);

#endif /* CONF_H */
