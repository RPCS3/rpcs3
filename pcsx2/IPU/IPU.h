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

#ifndef __IPU_H__
#define __IPU_H__

#include "mpeg2lib/Mpeg.h"

// IPU_INLINE_IRQS
// Scheduling ints into the future is a purist approach to emulation, and
// is mostly cosmetic since the emulator itself performs all actions instantly
// (as far as the emulated CPU is concerned).  In some cases this can actually
// cause more sync problems than it supposedly solves, due to accumulated
// delays incurred by the recompiler's low cycle update rate and also Pcsx2
// failing to properly handle pre-emptive DMA/IRQs or cpu exceptions.

// Uncomment the following line to enable inline IRQs for the IPU.  Tests show
// that it doesn't have any effect on compatibility or audio/video sync, and it
// speeds up movie playback by some 6-8%. But it lacks the purist touch, so it's
// not enabled by default.

//#define IPU_INLINE_IRQS


#ifdef _MSC_VER
#pragma pack(1)
#endif


#define ipumsk( src ) ( (src) & 0xff )
#define ipucase( src ) case ipumsk(src)

//
// Bitfield Structure
//
union tIPU_CMD {
	struct {
		u32 OPTION : 28;	// VDEC decoded value
		u32 CMD : 4;	// last command
		u32 _BUSY;
	};
	struct {
		u32 DATA;
		u32 BUSY;
	};
};

//
// Bitfield Structure
//
union tIPU_CTRL {
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
		u32 MP1 : 1;	// MPEG1 bit stream
		u32 PCT : 3;	// Picture Type
		u32 resv1 : 3;
		u32 RST : 1;	// Reset
		u32 BUSY : 1;	// Busy
	};
	u32 _u32;
};

//
// Bitfield Structure
//
struct tIPU_BP {
	u32 BP;		// Bit stream point
	u16 IFC;	// Input FIFO counter
	u8 FP;		// FIFO point
	u8 bufferhasnew;
};

#ifdef _WIN32
#pragma pack()
#endif

union tIPU_CMD_IDEC
{
	struct
	{
		u32 FB  : 6;
		u32 UN2 :10;
		u32 QSC : 5;
		u32 UN1 : 3;
		u32 DTD : 1;
		u32 SGN : 1;
		u32 DTE : 1;
		u32 OFM : 1;
		u32 cmd : 4;
	};

	u32 value;

	tIPU_CMD_IDEC( u32 val ) : value( val )
	{
	}
};

union tIPU_CMD_BDEC
{
	struct
	{
		u32 FB  : 6;
		u32 UN2 :10;
		u32 QSC : 5;
		u32 UN1 : 4;
		u32 DT  : 1;
		u32 DCR : 1;
		u32 MBI : 1;
		u32 cmd : 4;
	};
	u32 value;

	tIPU_CMD_BDEC( u32 val ) : value( val )
	{
	}
};

union tIPU_CMD_CSC
{
	struct
	{
		u32 MBC :11;
		u32 UN2 :15;
		u32 DTE : 1;
		u32 OFM : 1;
		u32 cmd : 4;
	};
	u32 value;

	tIPU_CMD_CSC( u32 val ) : value( val )
	{
	}
};

union tIPU_DMA
{
	struct
	{
		u32 GIFSTALL  : 1;
		u32 TIE0 :1;
		u32 TIE1 : 1;
		u32 ACTV1 : 1;
		u32 DOTIE1  : 1;
		u32 FIREINT0 : 1;
		u32 FIREINT1 : 1;
		u32 VIFSTALL : 1;
		u32 SIFSTALL : 1;
	};
	u32 _u32;
};

static tIPU_DMA g_nDMATransfer;

enum SCE_IPU
{
	SCE_IPU_BCLR = 0x0
,	SCE_IPU_IDEC
,	SCE_IPU_BDEC
,	SCE_IPU_VDEC
,	SCE_IPU_FDEC
,	SCE_IPU_SETIQ
,	SCE_IPU_SETVQ
,	SCE_IPU_CSC
,	SCE_IPU_PACK
,	SCE_IPU_SETTH
};

struct IPUregisters {
  tIPU_CMD  cmd;
  u32 dummy0[2];
  tIPU_CTRL ctrl;
  u32 dummy1[3];
  u32   ipubp;
  u32 dummy2[3];
  u32		top;
  u32		topbusy;
  u32 dummy3[2];
};

#define ipuRegs ((IPUregisters*)(PS2MEM_HW+0x2000))

extern tIPU_BP g_BP;
extern int coded_block_pattern;
extern int g_nIPU0Data; // or 0x80000000 whenever transferring
extern u8* g_pIPU0Pointer;

// The IPU can only do one task at once and never uses other buffers so these
// should be made available to functions in other modules to save registers.
PCSX2_ALIGNED16(extern macroblock_rgb32 rgb32);
PCSX2_ALIGNED16(extern macroblock_8 mb8);

extern int ipuInit();
extern void ipuReset();
extern void ipuShutdown();
extern int  ipuFreeze(gzFile f, int Mode);
extern bool ipuCanFreeze();


extern u32 ipuRead32(u32 mem);
extern u64 ipuRead64(u32 mem);
extern void ipuWrite32(u32 mem,u32 value);
extern void ipuWrite64(u32 mem,u64 value);

extern void IPUCMD_WRITE(u32 val);
extern void ipuSoftReset();
extern void IPUProcessInterrupt();
extern void ipu0Interrupt();
extern void ipu1Interrupt();

extern void dmaIPU0();
extern void dmaIPU1();
extern u16 __fastcall FillInternalBuffer(u32 * pointer, u32 advance, u32 size);
extern u8 __fastcall getBits32(u8 *address, u32 advance);
extern u8 __fastcall getBits16(u8 *address, u32 advance);
extern u8 __fastcall getBits8(u8 *address, u32 advance);
extern int __fastcall getBits(u8 *address, u32 size, u32 advance);

// returns number of qw read
int FIFOfrom_write(const u32 * value, int size);
void FIFOfrom_read(void *value,int size);
int FIFOto_read(void *value);
int FIFOto_write(u32* pMem, int size);
void FIFOto_clear();
void FIFOfrom_clear();

#endif
