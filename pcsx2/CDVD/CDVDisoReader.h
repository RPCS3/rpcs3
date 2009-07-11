/*  CDVDiso
 *  Copyright (C) 2002-2004  CDVDiso Team
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

#ifndef __CDVD_ISO_H__
#define __CDVD_ISO_H__

#ifdef _MSC_VER
#pragma warning(disable:4018)
#endif

#include <stdio.h>

#include "IopCommon.h"
#include "IsoFStools.h"
#include "CDVD_internal.h"

#define CD_FRAMESIZE_RAW	2352
#define DATA_SIZE	(CD_FRAMESIZE_RAW-12)

#define itob(i)		((i)/10*16 + (i)%10)	/* u_char to BCD */
#define btoi(b)		((b)/16*10 + (b)%16)	/* BCD to u_char */

#define MSF2SECT(m,s,f)	(((m)*60+(s)-2)*75+(f))

extern char IsoFile[256];

extern int BlockDump;
extern FILE* fdump;
extern FILE* isoFile;
extern int isoType;

extern u8 cdbuffer[];
extern u8 *pbuffer;

#endif