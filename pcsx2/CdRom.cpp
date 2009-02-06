/*  Pcsx2 - Pc Ps2 Emulator
 *  Copyright (C) 2002-2008  Pcsx2 Team
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

#include "PrecompiledHeader.h"

#include "PsxCommon.h"

//THIS ALL IS FOR THE CDROM REGISTERS HANDLING

#define CdlSync         0
#define CdlNop	        1
#define CdlSetloc		2
#define CdlPlay	        3
#define CdlForward		4
#define CdlBackward		5
#define CdlReadN		6
#define CdlStandby		7
#define CdlStop	        8
#define CdlPause        9
#define CdlInit 		10
#define CdlMute	        11
#define CdlDemute		12
#define CdlSetfilter	13
#define CdlSetmode		14
#define CdlGetmode      15
#define CdlGetlocL		16
#define CdlGetlocP		17
#define Cdl18     		18
#define CdlGetTN		19
#define CdlGetTD		20
#define CdlSeekL		21
#define CdlSeekP		22
#define CdlTest 		25
#define CdlID   		26
#define CdlReadS		27
#define CdlReset		28
#define CdlReadToc      30

#define AUTOPAUSE		249
#define READ_ACK		250
#define READ			251
#define REPPLAY_ACK		252
#define REPPLAY			253
#define ASYNC			254
/* don't set 255, it's reserved */

const char *CmdName[0x100]= {
	"CdlSync",    "CdlNop",       "CdlSetloc",  "CdlPlay",
	"CdlForward", "CdlBackward",  "CdlReadN",   "CdlStandby",
	"CdlStop",    "CdlPause",     "CdlInit",    "CdlMute",
	"CdlDemute",  "CdlSetfilter", "CdlSetmode", "CdlGetmode",
	"CdlGetlocL", "CdlGetlocP",   "Cdl18",      "CdlGetTN",
	"CdlGetTD",   "CdlSeekL",     "CdlSeekP",   NULL,
	NULL,         "CdlTest",      "CdlID",      "CdlReadS",
	"CdlReset",   NULL,           "CDlReadToc", NULL
};

cdrStruct cdr;
long LoadCdBios;
int cdOpenCase;

u8 Test04[] = { 0 };
u8 Test05[] = { 0 };
u8 Test20[] = { 0x98, 0x06, 0x10, 0xC3 };
u8 Test22[] = { 0x66, 0x6F, 0x72, 0x20, 0x45, 0x75, 0x72, 0x6F };
u8 Test23[] = { 0x43, 0x58, 0x44, 0x32, 0x39 ,0x34, 0x30, 0x51 };

// 1x = 75 sectors per second
// PSXCLK = 1 sec in the ps
// so (PSXCLK / 75) / BIAS = cdr read time (linuzappz)
//#define cdReadTime ((PSXCLK / 75) / BIAS)
unsigned long cdReadTime;// = ((PSXCLK / 75) / BIAS);

#define btoi(b)		((b)/16*10 + (b)%16)		/* BCD to u_char */
#define itob(i)		((i)/10*16 + (i)%10)		/* u_char to BCD */

#define CDR_INT(eCycle)    PSX_INT(IopEvt_Cdrom, eCycle)
#define CDREAD_INT(eCycle) PSX_INT(IopEvt_CdromRead, eCycle)

static __forceinline void StartReading(unsigned long type) {
   	cdr.Reading = type;
  	cdr.FirstSector = 1;
  	cdr.Readed = 0xff;
	AddIrqQueue(READ_ACK, 0x800);
}	

static __forceinline void StopReading() {
	if (cdr.Reading) {
		cdr.Reading = 0;
		psxRegs.interrupt &= ~(1<<IopEvt_CdromRead);
	}
}

static __forceinline void StopCdda() { 
	if (cdr.Play) { 
		cdr.StatP&=~0x80; 
		cdr.Play = 0; 
	} 
}

__forceinline void SetResultSize(u8 size) {
    cdr.ResultP = 0;
	cdr.ResultC = size;
	cdr.ResultReady = 1;
}

__forceinline s32 MSFtoLSN(u8 *Time) {
	u32 lsn;

	lsn = Time[2];
	lsn+=(Time[1] - 2) * 75;
	lsn+= Time[0] * 75 * 60;
	return lsn;
}

void LSNtoMSF(u8 *Time, s32 lsn) {
	lsn += 150;
	Time[2] = lsn / 4500;			// minuten
	lsn = lsn - Time[2] * 4500;		// minuten rest
	Time[1] = lsn / 75;				// sekunden
	Time[0] = lsn - Time[1] * 75;		// sekunden rest
}

void ReadTrack() {
	cdr.Prev[0] = itob(cdr.SetSector[0]);
	cdr.Prev[1] = itob(cdr.SetSector[1]);
	cdr.Prev[2] = itob(cdr.SetSector[2]);

	CDR_LOG("KEY *** %x:%x:%x\n", cdr.Prev[0], cdr.Prev[1], cdr.Prev[2]);
	cdr.RErr = CDVDreadTrack(MSFtoLSN(cdr.SetSector), CDVD_MODE_2352);
}

// cdr.Stat:
#define NoIntr		0
#define DataReady	1
#define Complete	2
#define Acknowledge	3
#define DataEnd		4
#define DiskError	5

void AddIrqQueue(u8 irq, unsigned long ecycle) {
	cdr.Irq = irq;
	if (cdr.Stat) {
		cdr.eCycle = ecycle;
	} else {
		CDR_INT(ecycle);
	}
}

void  cdrInterrupt() {
	cdvdTD trackInfo;
	int i;
	u8 Irq = cdr.Irq;

	if (cdr.Stat) {
		CDR_INT(0x800);
		return;
	}

	cdr.Irq = 0xff;
	cdr.Ctrl&=~0x80;

	switch (Irq) {
		case CdlSync:
			SetResultSize(1);
			cdr.StatP|= 0x2;
			cdr.Result[0] = cdr.StatP;
			cdr.Stat = Acknowledge; 
			break;

		case CdlNop:
			SetResultSize(1);
			cdr.Result[0] = cdr.StatP;
			cdr.Stat = Acknowledge;
			break;
			
		case CdlSetloc:
			cdr.CmdProcess = 0;
			SetResultSize(1);
			cdr.StatP|= 0x2;
			cdr.Result[0] = cdr.StatP;
			cdr.Stat = Acknowledge;
			break;

		case CdlPlay:
			cdr.CmdProcess = 0;
			SetResultSize(1);
			cdr.Result[0] = cdr.StatP;
			cdr.Stat = Acknowledge;
			cdr.StatP|= 0x82;
			break;

		case CdlForward:
			cdr.CmdProcess = 0;
			SetResultSize(1);
			cdr.StatP|= 0x2;
			cdr.Result[0] = cdr.StatP;
			cdr.Stat = Complete;
			break;

		case CdlBackward:
			cdr.CmdProcess = 0;
			SetResultSize(1);
			cdr.StatP|= 0x2;
			cdr.Result[0] = cdr.StatP;
			cdr.Stat = Complete;
			break;

		case CdlStandby:
			cdr.CmdProcess = 0;
			SetResultSize(1);
			cdr.StatP|= 0x2;
			cdr.Result[0] = cdr.StatP;
			cdr.Stat = Complete;
			break;

		case CdlStop:
			cdr.CmdProcess = 0;
			SetResultSize(1);
			cdr.StatP&=~0x2;
			cdr.Result[0] = cdr.StatP;
			cdr.Stat = Complete;
        		// cdr.Stat = Acknowledge;
			break;

		case CdlPause:
			SetResultSize(1);
			cdr.Result[0] = cdr.StatP;
			cdr.Stat = Acknowledge;
			AddIrqQueue(CdlPause + 0x20, 0x800);
			break;

		case CdlPause + 0x20:
			SetResultSize(1);
			cdr.StatP&=~0x20;
			cdr.StatP|= 0x2;
			cdr.Result[0] = cdr.StatP;
			cdr.Stat = Complete;
			break;

		case CdlInit:
			SetResultSize(1);
			cdr.StatP = 0x2;
			cdr.Result[0] = cdr.StatP;
			cdr.Stat = Acknowledge;
			AddIrqQueue(CdlInit + 0x20, 0x800);
        	break;

		case CdlInit + 0x20:
			SetResultSize(1);
			cdr.Result[0] = cdr.StatP;
			cdr.Stat = Complete;
			cdr.Init = 1;
			break;

		case CdlMute:
			SetResultSize(1);
			cdr.StatP|= 0x2;
			cdr.Result[0] = cdr.StatP;
			cdr.Stat = Acknowledge;
			break;

		case CdlDemute:
			SetResultSize(1);
			cdr.StatP|= 0x2;
			cdr.Result[0] = cdr.StatP;
			cdr.Stat = Acknowledge;
			break;

		case CdlSetfilter:
			SetResultSize(1);
			cdr.StatP|= 0x2;
			cdr.Result[0] = cdr.StatP;
			cdr.Stat = Acknowledge; 
			break;

		case CdlSetmode:
			SetResultSize(1);
			cdr.StatP|= 0x2;
			cdr.Result[0] = cdr.StatP;
			cdr.Stat = Acknowledge;
			break;

		case CdlGetmode:
			SetResultSize(6);
			cdr.StatP|= 0x2;
			cdr.Result[0] = cdr.StatP;
			cdr.Result[1] = cdr.Mode;
			cdr.Result[2] = cdr.File;
			cdr.Result[3] = cdr.Channel;
			cdr.Result[4] = 0;
			cdr.Result[5] = 0;
			cdr.Stat = Acknowledge;
			break;

		case CdlGetlocL:
			SetResultSize(8);
			for (i=0; i<8; i++) 
				cdr.Result[i] = cdr.Transfer[i];
			cdr.Stat = Acknowledge;
			break;

		case CdlGetlocP:
			SetResultSize(8);
			cdr.Result[0] = 1;
			cdr.Result[1] = 1;
			cdr.Result[2] = cdr.Prev[0];
			cdr.Result[3] = itob((btoi(cdr.Prev[1])) - 2);
			cdr.Result[4] = cdr.Prev[2];
			cdr.Result[5] = cdr.Prev[0];
			cdr.Result[6] = cdr.Prev[1];
			cdr.Result[7] = cdr.Prev[2];
			cdr.Stat = Acknowledge;
			break;

		case CdlGetTN:
			cdr.CmdProcess = 0;
			SetResultSize(3);
			cdr.StatP|= 0x2;
			cdr.Result[0] = cdr.StatP;
			if (CDVDgetTN(&cdr.ResultTN) == -1) {
				cdr.Stat = DiskError;
				cdr.Result[0]|= 0x01;
			} else {
				cdr.Stat = Acknowledge;
				cdr.Result[1] = itob(cdr.ResultTN.strack);
				cdr.Result[2] = itob(cdr.ResultTN.etrack);
			}
			break;

		case CdlGetTD:
			cdr.CmdProcess = 0;
			cdr.Track = btoi(cdr.Param[0]);
			SetResultSize(4);
			cdr.StatP|= 0x2;
			if (CDVDgetTD(cdr.Track, &trackInfo) == -1) {
				cdr.Stat = DiskError;
				cdr.Result[0]|= 0x01;
			} else {
				LSNtoMSF(cdr.ResultTD, trackInfo.lsn);
				cdr.Stat = Acknowledge;
				cdr.Result[0] = cdr.StatP;
				cdr.Result[1] = itob(cdr.ResultTD[0]);
				cdr.Result[2] = itob(cdr.ResultTD[1]);
				cdr.Result[3] = itob(cdr.ResultTD[2]);
			}
			break;

		case CdlSeekL:
			SetResultSize(1);
			cdr.StatP|= 0x2;
			cdr.Result[0] = cdr.StatP;
			cdr.Stat = Acknowledge;
			AddIrqQueue(CdlSeekL + 0x20, 0x800);
			break;

		case CdlSeekL + 0x20:
			SetResultSize(1);
			cdr.StatP|= 0x2;
			cdr.Result[0] = cdr.StatP;
			cdr.Stat = Complete;
			break;

		case CdlSeekP:
			SetResultSize(1);
			cdr.StatP|= 0x2;
			cdr.Result[0] = cdr.StatP;
			cdr.Stat = Acknowledge;
			AddIrqQueue(CdlSeekP + 0x20, 0x800);
			break;

		case CdlSeekP + 0x20:
			SetResultSize(1);
			cdr.StatP|= 0x2;
			cdr.Result[0] = cdr.StatP;
			cdr.Stat = Complete;
			break;

		case CdlTest:
			cdr.Stat = Acknowledge;
			switch (cdr.Param[0]) {
				case 0x20: // System Controller ROM Version
					SetResultSize(4);
					*(int*)cdr.Result = *(int*)Test20;
					break;
				case 0x22:
					SetResultSize(8);
					*(int*)cdr.Result = *(int*)Test22;
					break;
				case 0x23: case 0x24:
					SetResultSize(8);
					*(int*)cdr.Result = *(int*)Test23;
					break;
			}
			break;

		case CdlID:
			SetResultSize(1);
			cdr.StatP|= 0x2;
			cdr.Result[0] = cdr.StatP;
			cdr.Stat = Acknowledge;
			AddIrqQueue(CdlID + 0x20, 0x800);
			break;

		case CdlID + 0x20:
			SetResultSize(8);
        		cdr.Result[0] = 0x00; // 0x08 and cdr.Result[1]|0x10 : audio cd, enters cd player
			cdr.Result[1] = 0x00; // 0x80 leads to the menu in the bios, else loads CD

			if (!LoadCdBios) cdr.Result[1] |= 0x80;
			cdr.Result[2] = 0x00;
			cdr.Result[3] = 0x00;
			strncpy((char *)&cdr.Result[4], "PCSX", 4);
			cdr.Stat = Complete;
			break;

		case CdlReset:
			SetResultSize(1);
			cdr.StatP = 0x2;
			cdr.Result[0] = cdr.StatP;
			cdr.Stat = Acknowledge;
			break;

		case CdlReadToc:
			SetResultSize(1);
			cdr.StatP|= 0x2;
			cdr.Result[0] = cdr.StatP;
			cdr.Stat = Acknowledge;
			AddIrqQueue(CdlReadToc + 0x20, 0x800);
			break;

		case CdlReadToc + 0x20:
			SetResultSize(1);
			cdr.StatP|= 0x2;
			cdr.Result[0] = cdr.StatP;
			cdr.Stat = Complete;
			break;

		case AUTOPAUSE:
			cdr.OCUP = 0;
			AddIrqQueue(CdlPause, 0x400);
			break;

		case READ_ACK:
			if (!cdr.Reading)
				return;

			SetResultSize(1);
			cdr.StatP|= 0x2;
			cdr.Result[0] = cdr.StatP;
			cdr.Stat = Acknowledge;

			ReadTrack();

			CDREAD_INT((cdr.Mode & 0x80) ? (cdReadTime / 2) : cdReadTime);

			break;

		case REPPLAY_ACK:
			cdr.Stat = Acknowledge;
			cdr.Result[0] = cdr.StatP;
			SetResultSize(1);
			AddIrqQueue(REPPLAY, cdReadTime);
			break;

		case REPPLAY: 
			//if ((cdr.Mode & 5) != 5) break;
			break;

		case 0xff:
			return;

		default:
			cdr.Stat = Complete;
			break;
	}

	if (cdr.Stat != NoIntr && cdr.Reg2 != 0x18)
		psxHu32(0x1070)|= 0x4;

	CDR_LOG("Cdr Interrupt %x\n", Irq);
}

void  cdrReadInterrupt() {
	u8 *buf;

	if (!cdr.Reading)
		return;

	if (cdr.Stat) {
		CDREAD_INT(0x800);
		return;
	}

	CDR_LOG("KEY END");

	cdr.OCUP = 1;
	SetResultSize(1);
	cdr.StatP|= 0x22;
	cdr.Result[0] = cdr.StatP;

	SysPrintf("Reading From CDR");
	buf = CDVDgetBuffer();
	if (buf == NULL) cdr.RErr = -1;

	if (cdr.RErr == -1) {
		CDR_LOG(" err\n");
		memzero_ptr<2340>(cdr.Transfer);
		cdr.Stat = DiskError;
		cdr.Result[0]|= 0x01;
		ReadTrack();
		CDREAD_INT((cdr.Mode & 0x80) ? (cdReadTime / 2) : cdReadTime);
		return;
	}
	memcpy_fast(cdr.Transfer, buf+12, 2340);
	cdr.Stat = DataReady;

	CDR_LOG(" %x:%x:%x\n", cdr.Transfer[0], cdr.Transfer[1], cdr.Transfer[2]);

	cdr.SetSector[2]++;
	
	if (cdr.SetSector[2] == 75) {
		cdr.SetSector[2] = 0;
		cdr.SetSector[1]++;
		if (cdr.SetSector[1] == 60) {
			cdr.SetSector[1] = 0;
			cdr.SetSector[0]++;
		}
	}

	cdr.Readed = 0;

	if ((cdr.Transfer[4+2] & 0x80) && (cdr.Mode & 0x2)) { // EOF
		CDR_LOG("AutoPausing Read\n");
		AddIrqQueue(CdlPause, 0x800);
	}
	else {
		ReadTrack();
		CDREAD_INT((cdr.Mode & 0x80) ? (cdReadTime / 2) : cdReadTime);
	}

	psxHu32(0x1070)|= 0x4;
	return;
}

/*
cdrRead0:
	bit 0 - 0 REG1 command send / 1 REG1 data read
	bit 1 - 0 data transfer finish / 1 data transfer ready/in progress
	bit 2 - unknown
	bit 3 - unknown
	bit 4 - unknown
	bit 5 - 1 result ready
	bit 6 - 1 dma ready
	bit 7 - 1 command being processed
*/

u8 cdrRead0(void) {
	if (cdr.ResultReady) 
		cdr.Ctrl |= 0x20;
	else 
		cdr.Ctrl &= ~0x20;

	if (cdr.OCUP) 
		cdr.Ctrl |= 0x40;
	else 
		cdr.Ctrl &= ~0x40;

	// what means the 0x10 and the 0x08 bits? i only saw it used by the bios
	cdr.Ctrl|=0x18;

	CDR_LOG("CD0 Read: %x\n", cdr.Ctrl);
	return psxHu8(0x1800) = cdr.Ctrl;
}

/*
cdrWrite0:
	0 - to send a command / 1 - to get the result
*/

void cdrWrite0(u8 rt) {
	CDR_LOG("CD0 write: %x\n", rt);

	cdr.Ctrl = rt | (cdr.Ctrl & ~0x3);

	if (rt == 0) {
		cdr.ParamP = 0;
		cdr.ParamC = 0;
		cdr.ResultReady = 0;
	}
}

u8 cdrRead1(void) {
	if (cdr.ResultReady && cdr.Ctrl & 0x1) {
		psxHu8(0x1801) = cdr.Result[cdr.ResultP++];
		if (cdr.ResultP == cdr.ResultC) cdr.ResultReady = 0;
	} 
	else 
		psxHu8(0x1801) = 0;
	
	CDR_LOG("CD1 Read: %x\n", psxHu8(0x1801));
	return psxHu8(0x1801);
}

void cdrWrite1(u8 rt) {
	int i;

	CDR_LOG("CD1 write: %x (%s)\n", rt, CmdName[rt]);
	cdr.Cmd = rt;
	cdr.OCUP = 0;

#ifdef CDRCMD_DEBUG
	SysPrintf("CD1 write: %x (%s)", rt, CmdName[rt]);
	if (cdr.ParamC) {
		SysPrintf(" Param[%d] = {", cdr.ParamC);
		for (i=0;i<cdr.ParamC;i++) SysPrintf(" %x,", cdr.Param[i]);
		SysPrintf("}\n");
	} else SysPrintf("\n");
#endif

	if (cdr.Ctrl & 0x1) return;

	switch(cdr.Cmd) {
		case CdlSync:
			cdr.Ctrl|= 0x80;
			cdr.Stat = NoIntr; 
			AddIrqQueue(cdr.Cmd, 0x800);
			break;

		case CdlNop:
			cdr.Ctrl|= 0x80;
			cdr.Stat = NoIntr; 
			AddIrqQueue(cdr.Cmd, 0x800);
			break;

		case CdlSetloc:
			StopReading();
			for (i=0; i<3; i++) cdr.SetSector[i] = btoi(cdr.Param[i]);
			cdr.SetSector[3] = 0;
			if ((cdr.SetSector[0] | cdr.SetSector[1] | cdr.SetSector[2]) == 0) {
				*(unsigned long *)cdr.SetSector = *(unsigned long *)cdr.SetSectorSeek;
			}
			cdr.Ctrl|= 0x80;
			cdr.Stat = NoIntr;
			AddIrqQueue(cdr.Cmd, 0x800);
			break;

		case CdlPlay:
			cdr.Play = 1;
			cdr.Ctrl|= 0x80;
			cdr.Stat = NoIntr; 
			AddIrqQueue(cdr.Cmd, 0x800);
			break;

		case CdlForward:
			if (cdr.CurTrack < 0xaa) cdr.CurTrack++;
			cdr.Ctrl|= 0x80;
			cdr.Stat = NoIntr; 
			AddIrqQueue(cdr.Cmd, 0x800);
			break;

		case CdlBackward:
			if (cdr.CurTrack > 1) cdr.CurTrack--;
			cdr.Ctrl|= 0x80;
			cdr.Stat = NoIntr; 
			AddIrqQueue(cdr.Cmd, 0x800);
			break;

		case CdlReadN:
			cdr.Irq = 0;
			StopReading();
			cdr.Ctrl|= 0x80;
			cdr.Stat = NoIntr; 
			StartReading(1);
			break;

		case CdlStandby:
			StopCdda();
			StopReading();
			cdr.Ctrl|= 0x80;
			cdr.Stat = NoIntr; 
			AddIrqQueue(cdr.Cmd, 0x800);
			break;

		case CdlStop:
			StopCdda();
			StopReading();
			cdr.Ctrl|= 0x80;
			cdr.Stat = NoIntr; 
			AddIrqQueue(cdr.Cmd, 0x800);
			break;

		case CdlPause:
			StopCdda();
			StopReading();
			cdr.Ctrl|= 0x80;
			cdr.Stat = NoIntr; 
			AddIrqQueue(cdr.Cmd, 0x40000);
			break;

		case CdlReset:
		case CdlInit:
			StopCdda();
			StopReading();
			cdr.Ctrl|= 0x80;
			cdr.Stat = NoIntr; 
			AddIrqQueue(cdr.Cmd, 0x800);
			break;

		case CdlMute:
			cdr.Muted = 0;
			cdr.Ctrl|= 0x80;
			cdr.Stat = NoIntr; 
			AddIrqQueue(cdr.Cmd, 0x800);
			break;

		case CdlDemute:
			cdr.Muted = 1;
			cdr.Ctrl|= 0x80;
			cdr.Stat = NoIntr; 
			AddIrqQueue(cdr.Cmd, 0x800);
			break;

		case CdlSetfilter:
			cdr.File = cdr.Param[0];
			cdr.Channel = cdr.Param[1];
			cdr.Ctrl|= 0x80;
			cdr.Stat = NoIntr; 
			AddIrqQueue(cdr.Cmd, 0x800);
			break;

		case CdlSetmode:
			CDR_LOG("Setmode %x\n", cdr.Param[0]);
		
			cdr.Mode = cdr.Param[0];
			cdr.Ctrl|= 0x80;
			cdr.Stat = NoIntr; 
			AddIrqQueue(cdr.Cmd, 0x800);
			break;

		case CdlGetmode:
			cdr.Ctrl|= 0x80;
			cdr.Stat = NoIntr; 
			AddIrqQueue(cdr.Cmd, 0x800);
			break;

		case CdlGetlocL:
			cdr.Ctrl|= 0x80;
			cdr.Stat = NoIntr; 
			AddIrqQueue(cdr.Cmd, 0x800);
			break;

		case CdlGetlocP:
			cdr.Ctrl|= 0x80;
			cdr.Stat = NoIntr; 
			AddIrqQueue(cdr.Cmd, 0x800);
			break;

		case CdlGetTN:
			cdr.Ctrl|= 0x80;
			cdr.Stat = NoIntr; 
			AddIrqQueue(cdr.Cmd, 0x800);
			break;

		case CdlGetTD:
			cdr.Ctrl|= 0x80;
			cdr.Stat = NoIntr; 
			AddIrqQueue(cdr.Cmd, 0x800);
			break;

		case CdlSeekL:
			((unsigned long *)cdr.SetSectorSeek)[0] = ((unsigned long *)cdr.SetSector)[0];
			cdr.Ctrl|= 0x80;
			cdr.Stat = NoIntr; 
			AddIrqQueue(cdr.Cmd, 0x800);
			break;

		case CdlSeekP:
			((unsigned long *)cdr.SetSectorSeek)[0] = ((unsigned long *)cdr.SetSector)[0];
			cdr.Ctrl|= 0x80;
			cdr.Stat = NoIntr; 
			AddIrqQueue(cdr.Cmd, 0x800);
			break;

		case CdlTest:
			cdr.Ctrl|= 0x80;
			cdr.Stat = NoIntr; 
			AddIrqQueue(cdr.Cmd, 0x800);
			break;

		case CdlID:
			cdr.Ctrl|= 0x80;
			cdr.Stat = NoIntr; 
			AddIrqQueue(cdr.Cmd, 0x800);
			break;

		case CdlReadS:
			cdr.Irq = 0;
			StopReading();
			cdr.Ctrl|= 0x80;
			cdr.Stat = NoIntr; 
			StartReading(2);
			break;

		case CdlReadToc:
			cdr.Ctrl|= 0x80;
			cdr.Stat = NoIntr; 
			AddIrqQueue(cdr.Cmd, 0x800);
			break;

		default:
			CDR_LOG("Unknown Cmd: %x\n", cdr.Cmd);
			return;
    }
	if (cdr.Stat != NoIntr)
		iopIntcIrq( 2 );
}

u8 cdrRead2(void) {
	u8 ret;

	if (cdr.Readed == 0) {
		ret = 0;
	} else {
		ret = *cdr.pTransfer++;
	}

	CDR_LOG("CD2 Read: %x\n", ret);
	return ret;
}

void cdrWrite2(u8 rt) {
	CDR_LOG("CD2 write: %x\n", rt);
	
	if (cdr.Ctrl & 0x1) {
		switch (rt) {
			case 0x07:
	    		cdr.ParamP = 0;
				cdr.ParamC = 0;
				cdr.ResultReady = 0;
				cdr.Ctrl = 0;
				break;

			default:
				cdr.Reg2 = rt;
				break;
		}
	} 
	else 
	{
		if (!(cdr.Ctrl & 0x1) && cdr.ParamP < 8) {
			cdr.Param[cdr.ParamP++] = rt;
			cdr.ParamC++;
		}
	}
}

u8 cdrRead3(void) {
	if (cdr.Stat) {
		if (cdr.Ctrl & 0x1) 
			psxHu8(0x1803) = cdr.Stat | 0xE0;
		else 
			psxHu8(0x1803) = 0xff;
	} else psxHu8(0x1803) = 0;

	CDR_LOG("CD3 Read: %x\n", psxHu8(0x1803));
	return psxHu8(0x1803);
}

void cdrWrite3(u8 rt) {
	CDR_LOG("CD3 write: %x\n", rt);
	
	if (rt == 0x07 && cdr.Ctrl & 0x1) {
		cdr.Stat = 0;

		if (cdr.Irq == 0xff) { cdr.Irq = 0; return; }
		if (cdr.Irq) {
			CDR_INT(cdr.eCycle);
		}
		return;
	}
	
	if (rt == 0x80 && !(cdr.Ctrl & 0x1) && cdr.Readed == 0) {
		cdr.Readed = 1;
		cdr.pTransfer = cdr.Transfer;

		switch (cdr.Mode&0x30) {
			case 0x10:
			case 0x00: cdr.pTransfer+=12; break;
			default: break;
		}
	}
}

void psxDma3(u32 madr, u32 bcr, u32 chcr) {
	u32 cdsize;

	CDR_LOG("*** DMA 3 *** %lx addr = %lx size = %lx\n", chcr, madr, bcr);

	switch (chcr) {
		case 0x11000000:
		case 0x11400100:
			if (cdr.Readed == 0) {
				CDR_LOG("*** DMA 3 *** NOT READY\n");
				return;
			}

			cdsize = (bcr & 0xffff) * 4;
			memcpy_fast((u8*)PSXM(madr), cdr.pTransfer, cdsize);
			psxCpu->Clear(madr, cdsize/4);
			cdr.pTransfer+=cdsize;

			break;
		case 0x41000200:
			//SysPrintf("unhandled cdrom dma3: madr: %x, bcr: %x, chcr %x\n", madr, bcr, chcr);
			return;

		default:
			CDR_LOG("Unknown cddma %lx\n", chcr);
			break;
	}
	HW_DMA3_CHCR &= ~0x01000000;
	psxDmaInterrupt(3);
}

void cdrReset() {
	memzero_obj(cdr);
	cdr.CurTrack=1;
	cdr.File=1; cdr.Channel=1;
	cdReadTime = (PSXCLK / 1757) * BIAS; 
}

void SaveState::cdrFreeze() {
	Freeze(cdr);

	// Alrighty!  This code used to, for some reason, recalculate the pTransfer value
	// even though it's being saved as part of the cdr struct.  Probably a backwards
	// compat fix with an earlier save version.

	int tmp; // = (int)(cdr.pTransfer - cdr.Transfer);
	Freeze(tmp);
	//if (Mode == 0) cdr.pTransfer = cdr.Transfer + tmp;
}

