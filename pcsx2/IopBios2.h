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

/***** sysmem imageInfo
00000800:  00 16 00 00 70 14 00 00 ¦ 01 01 00 00 01 00 00 00
0000 next:           .word ?	//00001600
0004 name:           .word ?	//00001470
0008 version:        .half ?	//0101
000A flags:          .half ?	//----
000C index:          .half ?	//0001
000E field_E:        .half ?	//----
00000810:  90 08 00 00 A0 94 00 00 ¦ 30 08 00 00 40 0C 00 00
0010 entry:          .word ?	//00000890
0014 gp_value:       .word ?	//000094A0
0018 p1_vaddr:       .word ?	//00000830
001C text_size:      .word ?	//00000C40
00000820:  40 00 00 00 10 00 00 00 ¦ 00 00 00 00 00 00 00 00
0020 data_size:      .word ?	//00000040
0024 bss_size:       .word ?	//00000010
0028 field_28:       .word ?	//--------
002C field_2C:       .word ?	//--------
*****/

#ifndef __PSX_BIOS_H__
#define __PSX_BIOS_H__

struct irxImageInfo {
	u32	next,		//+00
		name;		//+04
	u16	version,	//+08
		flags,		//+0A
		index,		//+0C
		_unkE;		//+0E
	u32	entry,		//+10
		_gp,		//+14
		vaddr,		//+18
		text_size,	//+1C
		data_size,	//+20
		bss_size,	//+24
		_pad28,		//+28
		_pad2C;		//+2C
};		//=30

struct _sifServer {
	int active;
	u32 server;
	u32 fhandler;
};

#define SIF_SERVERS 32

_sifServer sifServer[SIF_SERVERS];

// max modules/funcs

#define IRX_MODULES 64
#define IRX_FUNCS 256 

struct irxFunc {
	u32 num;
	u32 entry;
};

struct irxModule {
	int active;
	u32 name[2];
	irxFunc funcs[IRX_FUNCS];
};

irxModule irxMod[IRX_MODULES];


void iopModulesInit();
int  iopSetImportFunc(u32 *ptr);
int  iopSetExportFunc(u32 *ptr);
void sifServerCall(u32 server, u32 num, char *bin, int insize, char *bout, int outsize);
void sifAddServer(u32 server, u32 fhandler);

#endif
