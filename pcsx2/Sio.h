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


#ifndef _SIO_H_
#define _SIO_H_

// SIO IRQ Timings...
// Scheduling ints into the future is a purist approach to emulation, and
// is mostly cosmetic since the emulator itself performs all actions instantly
// (as far as the emulated CPU is concerned).  In some cases this can actually
// cause more sync problems than it supposedly solves, due to accumulated
// delays incurred by the recompiler's low cycle update rate and also Pcsx2
// failing to properly handle pre-emptive DMA/IRQs or cpu exceptions.

// The SIO is one of these cases, where-by many games seem to be a lot happier
// if the SIO handles its IRQs instantly instead of scheduling them.
// Uncomment the line below for SIO instant-IRQ mode.  It improves responsiveness 
// considerably, fixes PAD latency problems in some games, and may even reduce the
// chance of saves getting corrupted (untested).  But it lacks the purist touch,
// so it's not enabled by default.

//#define SIO_INLINE_IRQS


struct _sio {
	u16 StatReg;
	u16 ModeReg;
	u16 CtrlReg;
	u16 BaudReg;

	u8  buf[256];
	u32 bufcount;
	u32 parp;
	u32 mcdst,rdwr;
	u8  adrH,adrL;
	u32 padst;
	u32 mtapst;
	u32 packetsize;

	u8  terminator;
	u8  mode;
	u8  mc_command;
	u32 lastsector;
	u32 sector;
	u32 k;
	u32 count;

	int GetMemcardIndex() const
	{
		return (CtrlReg&0x2000) >> 13;
	}
};

extern _sio sio;

// Status Flags
#define TX_RDY		0x0001
#define RX_RDY		0x0002
#define TX_EMPTY	0x0004
#define PARITY_ERR	0x0008
#define RX_OVERRUN	0x0010
#define FRAMING_ERR	0x0020
#define SYNC_DETECT	0x0040
#define DSR			0x0080
#define CTS			0x0100
#define IRQ			0x0200

// Control Flags
#define TX_PERM		0x0001
#define DTR			0x0002
#define RX_PERM		0x0004
#define BREAK		0x0008
#define RESET_ERR	0x0010
#define RTS			0x0020
#define SIO_RESET	0x0040

extern void sioInit();
extern void sioShutdown();
extern void psxSIOShutdown();
extern u8 sioRead8();
extern void sioWrite8(u8 value);
extern void sioWriteCtrl16(u16 value);
extern void sioInterrupt();
extern void InitializeSIO(u8 value);

#ifdef _MSC_VER
#pragma pack(1)
#endif
struct mc_command_0x26_tag{
	u8	field_151;	//+02 flags
	u16	sectorSize;	//+03 divide to it
	u16 field_2C;	//+05 divide to it
	u32	mc_size;	//+07
	u8	mc_xor;		//+0b don't forget to recalculate it!!!
	u8	Z;			//+0c
#ifdef _MSC_VER
};
#pragma pack()
#else
} __attribute__((packed));
#endif

#endif
