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

#ifndef __GIF_H__
#define __GIF_H__

#define gifsplit 0x10000
enum gifstate_t
{
	GIF_STATE_READY = 0,
	GIF_STATE_STALL = 1,
	GIF_STATE_DONE = 2,
	GIF_STATE_EMPTY = 0x10
};


extern void gsInterrupt();
int _GIFchain();
void GIFdma();
void dmaGIF();
void mfifoGIFtransfer(int qwc);
void gifMFIFOInterrupt();

// Under construction; use with caution.
union tGIF_CTRL
{
	struct
	{
		u32 RST : 1;
		u32 reserved1 : 2;
		u32 PSE : 1;
		u32 reserved2 : 28;
	};
	u32 _u32;

	tGIF_CTRL( u32 val ) : _u32( val )
	{
	}
};

union tGIF_MODE
{
	struct
	{
		u32 M3R : 1;
		u32 reserved1 : 1;
		u32 IMT : 1;
		u32 reserved2 : 29;
	};
	u32 _u32;

	tGIF_MODE( u32 val ) : _u32( val )
	{
	}
};

union tGIF_STAT
{
	struct
	{
		u32 M3R : 1;
		u32 M3P : 1;
		u32 IMT : 1;
		u32 PSE : 1;
		u32 reserved1 : 1;
		u32 IP3 : 1;
		u32 P3Q : 1;
		u32 P2Q : 1;
		u32 P1Q : 1;
		u32 OPH : 1;
		u32 APATH : 2;
		u32 DIR : 1;
		u32 reserved2 : 11;
		u32 FQC : 5;
		u32 reserved3 : 3;
	};
	u32 _u32;

	tGIF_STAT( u32 val ) : _u32( val )
	{
	}
};

union tGIF_TAG0
{
	struct
	{
		u32 NLOOP : 15;
		u32 EOP : 1;
		u32 TAG : 16;
	};
	u32 _u32;

	tGIF_TAG0( u32 val ) : _u32( val )
	{
	}
};

union tGIF_TAG1
{
	struct
	{
		u32 TAG : 14;
		u32 PRE : 1;
		u32 PRIM : 11;
		u32 FLG : 2;
		u32 NREG : 4;
	};
	u32 _u32;

	tGIF_TAG1( u32 val ) : _u32( val )
	{
	}
};

union tGIF_CNT
{
	struct
	{
		u32 LOOPCNT : 15;
		u32 reserved1 : 1;
		u32 REGCNT : 4;
		u32 VUADDR : 2;
		u32 reserved2 : 10;
		
	};
	u32 _u32;

	tGIF_CNT( u32 val ) : _u32( val )
	{
	}
};

union tGIF_P3CNT
{
	struct
	{
		u32 P3CNT : 15;
		u32 reserved1 : 17;
	};
	u32 _u32;

	tGIF_P3CNT( u32 val ) : _u32( val )
	{
	}
};

union tGIF_P3TAG
{
	struct
	{
		u32 LOOPCNT : 15;
		u32 EOP : 1;
		u32 reserved1 : 16;
	};
	u32 _u32;

	tGIF_P3TAG( u32 val ) : _u32( val )
	{
	}
};

struct GIFregisters
{
	// To do: Pad to the correct positions and hook up.
	tGIF_CTRL 	ctrl;
	u32 padding[3];
	tGIF_MODE 	mode;
	u32 padding1[3];
	tGIF_STAT		stat;
	u32 padding2[7];
	
	tGIF_TAG0	tag0;
	u32 padding3[3];
	tGIF_TAG1	tag1;
	u32 padding4[3];
	u32			tag2;
	u32 padding5[3];
	u32			tag3;
	u32 padding6[3];
	
	tGIF_CNT		cnt;
	u32 padding7[3];
	tGIF_P3CNT	p3cnt;
	u32 padding8[3];
	tGIF_P3TAG	p3tag;
	u32 padding9[3];
};

#define gifRegs ((GIFregisters*)(PS2MEM_HW+0x3000))

// Quick function to see if everythings in the write position.
/*static void checkGifRegs()
{
	Console::WriteLn("psHu32(GIF_CTRL) == 0x%x; gifRegs->ctrl == 0x%x", params &psHu32(GIF_CTRL),&gifRegs->ctrl);
	Console::WriteLn("psHu32(GIF_MODE) == 0x%x; gifRegs->mode == 0x%x", params &psHu32(GIF_MODE),&gifRegs->mode);
	Console::WriteLn("psHu32(GIF_STAT) == 0x%x; gifRegs->stat == 0x%x", params &psHu32(GIF_STAT),&gifRegs->stat);
	Console::WriteLn("psHu32(GIF_TAG0) == 0x%x; gifRegs->tag0 == 0x%x", params &psHu32(GIF_TAG0),&gifRegs->tag0);
	Console::WriteLn("psHu32(GIF_TAG1) == 0x%x; gifRegs->tag1 == 0x%x", params &psHu32(GIF_TAG1),&gifRegs->tag1);
	Console::WriteLn("psHu32(GIF_TAG2) == 0x%x; gifRegs->tag2 == 0x%x", params &psHu32(GIF_TAG2),&gifRegs->tag2);
	Console::WriteLn("psHu32(GIF_TAG3) == 0x%x; gifRegs->tag3 == 0x%x", params &psHu32(GIF_TAG3),&gifRegs->tag3);
	Console::WriteLn("psHu32(GIF_CNT) == 0x%x; gifRegs->cnt == 0x%x", params &psHu32(GIF_CNT),&gifRegs->cnt);
	Console::WriteLn("psHu32(GIF_P3CNT) == 0x%x; gifRegs->p3cnt == 0x%x", params &psHu32(GIF_P3CNT),&gifRegs->p3cnt);
	Console::WriteLn("psHu32(GIF_P3TAG) == 0x%x; gifRegs->p3tag == 0x%x", params &psHu32(GIF_P3TAG),&gifRegs->p3tag);

}*/

#endif