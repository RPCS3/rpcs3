/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2010  PCSX2 Dev Team
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

#ifndef __CDROM_H__
#define __CDROM_H__

#include "IopCommon.h"
#include "PS2Edefs.h"

// Not used.
typedef struct {
	s32	y0, y1;
} ADPCM_Decode_t;

// Not used.
typedef struct {
	s32				freq;
	s32				nbits;
	s32				stereo;
	s32				nsamples;
	ADPCM_Decode_t	left, right;
	s16			pcm[16384];
} xa_decode_t;

struct cdrStruct
{
	u8 OCUP;
	u8 Reg1Mode;
	u8 Reg2;
	u8 CmdProcess;
	u8 Ctrl;
	u8 Stat;

	u8 StatP;

	u8 Transfer[2352];
	u8 *pTransfer;

	u8 Prev[4];
	u8 Param[8];
	u8 Result[8];

	u8 ParamC;
	u8 ParamP;
	u8 ResultC;
	u8 ResultP;
	u8 ResultReady;
	u8 Cmd;
	u8 Readed;
	u32 Reading;

	cdvdTN ResultTN;
	u8 ResultTD[4];
	u8 SetSector[4];
	u8 SetSectorSeek[4];
	u8 Track;
	int Play;
	int CurTrack;
	int Mode, File, Channel, Muted;
	int Reset;
	int RErr;
	int FirstSector;

	xa_decode_t Xa;

	int Init;

	u8 IrqMask; // psxdev: Added on initial psx work, not referenced since. Is it needed?
	u8 Irq;
	u32 eCycle;

	char Unused[4087];
};

extern cdrStruct cdr;

void cdrReset();
void  cdrInterrupt();
void  cdrReadInterrupt();
u8   cdrRead0(void);
u8   cdrRead1(void);
u8   cdrRead2(void);
u8   cdrRead3(void);
void cdrWrite0(u8 rt);
void cdrWrite1(u8 rt);
void cdrWrite2(u8 rt);
void cdrWrite3(u8 rt);

#endif /* __CDROM_H__ */
