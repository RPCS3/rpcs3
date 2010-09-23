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

#pragma once

#include "IPU_Fifo.h"

#define ipumsk( src ) ( (src) & 0xff )
#define ipucase( src ) case ipumsk(src)

#define IPU_INT_TO( cycles )  if(!(cpuRegs.interrupt & (1<<4))) CPU_INT( DMAC_TO_IPU, cycles )
#define IPU_INT_FROM( cycles )  CPU_INT( DMAC_FROM_IPU, cycles )

#define IPU_FORCEINLINE __fi

//
// Bitfield Structures
//

struct tIPU_CMD
{
	u32 DATA;
	u32 BUSY;
};

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

	tIPU_CTRL( u32 val ) { _u32 = val; }

    // CTRL = the first 16 bits of ctrl [0x8000ffff], + value for the next 16 bits,
    // minus the reserved bits. (18-19; 27-29) [0x47f30000]
	void write(u32 value) { _u32 = (value & 0x47f30000) | (_u32 & 0x8000ffff); }

	bool test(u32 flags) const { return !!(_u32 & flags); }
	void set_flags(u32 flags) { _u32 |= flags; }
	void clear_flags(u32 flags) { _u32 &= ~flags; }
	void reset() { _u32 = 0; }
};

struct __aligned16 tIPU_BP {
	__aligned16 u128 internal_qwc[2];

	u32 BP;		// Bit stream point (0 to 128*2)
	u32 IFC;	// Input FIFO counter (8QWC) (0 to 8)
	u32 FP;		// internal FIFO (2QWC) fill status (0 to 2)

	__fi void Align()
	{
		BP = (BP + 7) & ~7;
		Advance(0);
	}

	__fi void Advance(uint bits)
	{
		BP += bits;
		pxAssume( BP <= 256 );

		if (BP >= 128)
		{
			BP -= 128;

			if (FP == 2)
			{
				// when BP is over 128 it means we're reading data from the second quadword.  Shift that one
				// to the front and load the new quadword into the second QWC (its a manualized ringbuffer!)

				CopyQWC(&internal_qwc[0], &internal_qwc[1]);
				FP = 1;
			}
			else
			{
				// if FP == 1 then the buffer has been completely drained.
				// if FP == 0 then an already-drained buffer is being advanced, and we need to drop a
				// quadword from the IPU FIFO.

				if (!FP)
					ipu_fifo.in.read(&internal_qwc[0]);

				FP = 0;
			}
		}
	}

	__fi bool FillBuffer(u32 bits)
	{
		while (FP < 2)
		{
			if (ipu_fifo.in.read(&internal_qwc[FP]) == 0)
			{
				// Here we *try* to fill the entire internal QWC buffer; however that may not necessarily
				// be possible -- so if the fill fails we'll only return 0 if we don't have enough
				// remaining bits in the FIFO to fill the request.

				return ((FP!=0) && (BP + bits) <= 128);
			}

			++FP;
		}

		return true;
	}

	wxString desc() const
	{
		return wxsFormat(L"Ipu BP: bp = 0x%x, IFC = 0x%x, FP = 0x%x.", BP, IFC, FP);
	}
};

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

	u32 _u32;

	tIPU_CMD_IDEC( u32 val ) { _u32 = val; }

	bool test(u32 flags) const { return !!(_u32 & flags); }
	void set_flags(u32 flags) { _u32 |= flags; }
	void clear_flags(u32 flags) { _u32 &= ~flags; }
	void reset() { _u32 = 0; }
	void log() const;
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
	u32 _u32;

	tIPU_CMD_BDEC( u32 val ) { _u32 = val; }

	bool test(u32 flags) const { return !!(_u32 & flags); }
	void set_flags(u32 flags) { _u32 |= flags; }
	void clear_flags(u32 flags) { _u32 &= ~flags; }
	void reset() { _u32 = 0; }
	void log(int s_bdec) const;
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
	u32 _u32;

	tIPU_CMD_CSC( u32 val ){ _u32 = val; }

	bool test(u32 flags) const { return !!(_u32 & flags); }
	void set_flags(u32 flags) { _u32 |= flags; }
	void clear_flags(u32 flags) { _u32 &= ~flags; }
	void reset() { _u32 = 0; }
	void log_from_YCbCr() const;
	void log_from_RGB32() const;
};

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

struct tIPU_cmd
{
	int index;
	int pos[6];
	union {
		struct {
			u32 OPTION : 28;
			u32 CMD : 4;
		};
		u32 current;
	};
	void clear()
	{
		memzero(pos);
		index = 0;
		current = 0xffffffff;
	}
	wxString desc() const
	{
		return wxsFormat(L"Ipu cmd: index = 0x%x, current = 0x%x, pos[0] = 0x%x, pos[1] = 0x%x",
			index, current, pos[0], pos[1]);
	}
};

static IPUregisters& ipuRegs = (IPUregisters&)eeHw[0x2000];

extern tIPU_cmd ipu_cmd;
extern int coded_block_pattern;

extern int ipuInit();
extern void ipuReset();

extern u32 ipuRead32(u32 mem);
extern u64 ipuRead64(u32 mem);
extern bool ipuWrite32(u32 mem,u32 value);
extern bool ipuWrite64(u32 mem,u64 value);

extern void IPUCMD_WRITE(u32 val);
extern void ipuSoftReset();
extern void IPUProcessInterrupt();

extern u8 getBits128(u8 *address, bool advance);
extern u8 getBits64(u8 *address, bool advance);
extern u8 getBits32(u8 *address, bool advance);
extern u8 getBits16(u8 *address, bool advance);
extern u8 getBits8(u8 *address, bool advance);

