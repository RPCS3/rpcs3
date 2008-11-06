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

#ifndef __IPU_H__
#define __IPU_H__

#include "Common.h"
#ifdef _MSC_VER
#pragma pack(1)
#endif

//
// Bitfield Structure
//
typedef union {
	struct {
		u32 OPTION : 28;	// VDEC decoded value
		u32 CMD : 4;	// last command
		u32 _BUSY;
	};
	struct {
		u32 DATA;
		u32 BUSY;
	};
} tIPU_CMD;

#define IPU_CTRL_IFC_M		(0x0f<< 0)
#define IPU_CTRL_OFC_M		(0x0f<< 4)
#define IPU_CTRL_CBP_M		(0x3f<< 8)
#define IPU_CTRL_ECD_M		(0x01<<14)
#define IPU_CTRL_SCD_M		(0x01<<15)
#define IPU_CTRL_IDP_M		(0x03<<16)
#define IPU_CTRL_AS_M		(0x01<<20)
#define IPU_CTRL_IVF_M		(0x01<<21)
#define IPU_CTRL_QST_M		(0x01<<22)
#define IPU_CTRL_MP1_M		(0x01<<23)
#define IPU_CTRL_PCT_M		(0x07<<24)
#define IPU_CTRL_RST_M		(0x01<<30)
#define IPU_CTRL_BUSY_M		(0x01<<31)

#define IPU_CTRL_IFC_O		( 0)
#define IPU_CTRL_OFC_O		( 4)
#define IPU_CTRL_CBP_O		( 8)
#define IPU_CTRL_ECD_O		(14)
#define IPU_CTRL_SCD_O		(15)
#define IPU_CTRL_IDP_O		(16)
#define IPU_CTRL_AS_O		(20)
#define IPU_CTRL_IVF_O		(21)
#define IPU_CTRL_QST_O		(22)
#define IPU_CTRL_MP1_O		(23)
#define IPU_CTRL_PCT_O		(24)
#define IPU_CTRL_RST_O		(30)
#define IPU_CTRL_BUSY_O		(31)


//
// Bitfield Structure
//
typedef union {
	struct {
		u32 IFC : 4;	// Input FIFO counter
		u32 OFC : 4;	// Output FIFO counter
		u32 CBP : 6;	// Coded block pattern
		u32 ECD : 1;	// Error code pattern
		u32 SCD : 1;	// Start code detected
		u32 IDP : 2;	// Intra DC precision
		u32 resv0 : 2;
		u32 AS : 1;		// Alternate scan
		u32 IVF : 1;	// Intra VLC format
		u32 QST : 1;	// Q scale step
		u32 MP1 : 1;	// MPEG1 bit strea
		u32 PCT : 3;	// Picture Type
		u32 resv1 : 3;
		u32 RST : 1;	// Reset
		u32 BUSY : 1;	// Busy
	};
	u32 _u32;
} tIPU_CTRL;

#define IPU_BP_BP_M		(0x7f<< 0)
#define IPU_BP_IFC_M	(0x0f<< 8)
#define IPU_BP_FP_M		(0x03<<16)

#define IPU_BP_BP_O		( 0)
#define IPU_BP_IFC_O	( 8)
#define IPU_BP_FP_O		(16)


//
// Bitfield Structure
//
typedef struct {
	u32 BP;		// Bit stream point
	u16 IFC;	// Input FIFO counter
	u8 FP;		// FIFO point
	u8 bufferhasnew;
} tIPU_BP;

#ifdef _WIN32
#pragma pack()
#endif

typedef struct {
	u32 FB  : 6;	
	u32 UN2 :10;
	u32 QSC : 5;	
	u32 UN1 : 3;	
	u32 DTD : 1;	
	u32 SGN : 1;	
	u32 DTE : 1;	
	u32 OFM : 1;	
	u32 cmd : 4;	
} tIPU_CMD_IDEC;

typedef struct {
	u32 FB  : 6;	
	u32 UN2 :10;
	u32 QSC : 5;	
	u32 UN1 : 4;	
	u32 DT  : 1;	
	u32 DCR : 1;	
	u32 MBI : 1;	
	u32 cmd : 4;	
} tIPU_CMD_BDEC;

typedef struct {
	u32 MBC :11;
	u32 UN2 :15;
	u32 DTE : 1;	
	u32 OFM : 1;	
	u32 cmd : 4;
} tIPU_CMD_CSC;

#define SCE_IPU_BCLR	0x0
#define SCE_IPU_IDEC	0x1
#define SCE_IPU_BDEC	0x2
#define SCE_IPU_VDEC	0x3
#define SCE_IPU_FDEC	0x4
#define SCE_IPU_SETIQ	0x5
#define SCE_IPU_SETVQ	0x6
#define SCE_IPU_CSC		0x7
#define SCE_IPU_PACK	0x8
#define SCE_IPU_SETTH	0x9

typedef struct {
  tIPU_CMD  cmd;
  u32 dummy0[2];
  tIPU_CTRL ctrl;
  u32 dummy1[3];
  u32   ipubp;
  u32 dummy2[3];
  u32		top;
  u32		topbusy;
  u32 dummy3[2];
} IPUregisters, *PIPUregisters;

#define ipuRegs ((IPUregisters*)(PS2MEM_HW+0x2000))

void dmaIPU0();
void dmaIPU1();

int ipuInit();
void ipuReset();
void ipuShutdown();
int  ipuFreeze(gzFile f, int Mode);
BOOL ipuCanFreeze();

u32 ipuRead32(u32 mem);
int ipuConstRead32(u32 x86reg, u32 mem);

u64 ipuRead64(u32 mem);
void ipuConstRead64(u32 mem, int mmreg);

void ipuWrite32(u32 mem,u32 value);
void ipuConstWrite32(u32 mem, int mmreg);

void ipuWrite64(u32 mem,u64 value);
void ipuConstWrite64(u32 mem, int mmreg);

void ipu0Interrupt();
void ipu1Interrupt();

u8 getBits32(u8 *address, u32 advance);
u8 getBits16(u8 *address, u32 advance);
u8 getBits8(u8 *address, u32 advance);
int getBits(u8 *address, u32 size, u32 advance);

#endif
