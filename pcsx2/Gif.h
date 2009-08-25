/*  Pcsx2 - Pc Ps2 Emulator
 *  Copyright (C) 2002-2009  Pcsx2 Team
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

// This code isn't ready to be used yet, but after typing out all those bitflags, 
// I wanted a copy saved. :)
/*
union tGIF_CTRL
{
	struct
	{
		u32 RST :1,
		u32 reserved :2,
		u32 PSE :1,
		u32 reserved2 :28
	};
	u32 value;

	tGIF_CTRL( u32 val ) : value( val )
};

union tGIF_MODE
{
	struct
	{
		u32 M3R :1,
		u32 reserved :1,
		u32 IMT :1,
		u32 reserved2 :29
	};
	u32 value;

	tGIF_MODE( u32 val ) : value( val )
	{
	}
};

union tGIF_STAT
{
	struct
	{
		u32 M3R :1,
		u32 M3P :1,
		u32 IMT :1, 
		u32 PSE :1,
		u32 reserved :1,
		u32 IP3 :1,
		u32 P3Q :1,
		u32 P2Q :1,
		u32 P1Q :1,
		u32 OPH :1,
		u32 APATH : 2,
		u32 DIR :1,
		u32 reserved2 :11,
		u32 FQC :5,
		u32 reserved3 :3
	};
	u32 value;

	tGIF_STAT( u32 val ) : value( val )
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
	u32 value;

	tGIF_TAG0( u32 val ) : value( val )
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
	u32 value;

	tGIF_TAG1( u32 val ) : value( val )
	{
	}
};

union tGIF_CNT
{
	struct
	{
		u32 LOOPCNT : 15;
		u32 reserved : 1;
		u32 REGCNT : 4;
		u32 VUADDR : 2;
		u32 reserved2 : 10;
		
	};
	u32 value;

	tGIF_CNT( u32 val ) : value( val )
	{
	}
};

union tGIF_P3CNT
{
	struct
	{
		u32 P3CNT : 15;
		reserved : 17;
	};
	u32 value;

	tGIF_P3CNT( u32 val ) : value( val )
	{
	}
};

union tGIF_P3TAG
{
	struct
	{
		u32 LOOPCNT : 15;
		u32 EOP : 1;
		u32 reserved : 16;
	};
	u32 value;

	tGIF_P3TAG( u32 val ) : value( val )
	{
	}
};

struct GIFregisters
{
	// To do: Pad to the correct positions and hook up.
	tGIF_CTRL 	ctrl;
	tGIF_MODE 	mode;
	tGIF_STAT	stat;
	
	tGIF_TAG0	tag0;
	tGIF_TAG1	tag1;
	u32			tag2;
	u32			tag3;
	
	tGIF_CNT		cnt;
	tGIF_P3CNT	p3cnt;
	tGIF_P3TAG	p3tag;
};

#define gifRegs ((GIFregisters*)(PS2MEM_HW+0x3000))*/

#endif