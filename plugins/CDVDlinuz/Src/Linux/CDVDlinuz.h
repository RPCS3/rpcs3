/*  CDVDlinuz.h

 *  Copyright (C) 2002-2005  CDVDlinuz Team

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



#ifndef __CDVDLINUZ_H__

#define __CDVDLINUZ_H__

#define CDVDdefs
#include "PS2Edefs.h"

extern char *libname;

extern const unsigned char version;
extern const unsigned char revision;
extern const unsigned char build;



// #define VERBOSE_WARNINGS

// #define VERBOSE_FUNCTION

#define VERBOSE_DISC_INFO

#define VERBOSE_DISC_TYPE

#define READ_AHEAD_BUFFERS 32

#endif /* __CDVDLINUZ_H__ */

