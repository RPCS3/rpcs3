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

 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

 *

 *  PCSX2 members can be contacted through their website at www.pcsx2.net.

 */



#ifndef CONF_H

#define CONF_H





#ifndef __LINUX__

#ifdef __linux__

#define __LINUX__

#endif /* __linux__ */

#endif /* No __LINUX__ */



#define CDVDdefs

#include "../PS2Edefs.h"





#define VERBOSE_FUNCTION_CONF





// Configuration Data



typedef struct {

  u8 devicename[256];

} CDVDconf;

extern CDVDconf conf;



#define DEFAULT_DEVICE "/dev/cdrom"





// Configuration Functions



extern void InitConf();

extern void LoadConf();

extern void SaveConf();



extern void ExecCfg(char *arg);





#endif /* CONF_H */

