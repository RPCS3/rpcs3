/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2009  PCSX2 Dev Team
 *
 *  PCSX2 is free software: you can redistribute it and/or modify it under the terms
 *  of the GNU Lesser General Public License as published by the Free Software Found-
 *  ation, either version 3 of the License, or (at your option) any later version.
 *
 *  PCSX2 is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 *  without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 *  PURPOSE.  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along with PCSX2.
 *  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __DECI2_H__
#define __DECI2_H__

#include "Common.h"
#include "deci2_dcmp.h"
#include "deci2_iloadp.h"
#include "deci2_dbgp.h"
#include "deci2_netmp.h"
#include "deci2_ttyp.h"

#define PROTO_DCMP		0x0001
#define PROTO_ITTYP		0x0110
#define PROTO_IDBGP		0x0130
#define PROTO_ILOADP	0x0150
#define PROTO_ETTYP		0x0220
#define PROTO_EDBGP		0x0230
#define PROTO_NETMP		0x0400


#pragma pack(1)
struct DECI2_HEADER {
	u16		length,		//+00
			_pad,		//+02
			protocol;	//+04
	char	source,		//+06
			destination;//+07
};			//=08

struct DECI2_DBGP_BRK{
	u32	address,			//+00
		count;				//+04
};			//=08
#pragma pack()

#define STOP	0
#define RUN		1

extern DECI2_DBGP_BRK	ebrk[32], ibrk[32];
extern s32 ebrk_count, ibrk_count;
extern volatile long runStatus;
extern s32 runCode, runCount;

extern Threading::Semaphore* runEvent;

extern s32		connected;
													//when add linux code this might change

int	writeData(const u8 *result);
void	exchangeSD(DECI2_HEADER *h);

#endif//__DECI2_H__
