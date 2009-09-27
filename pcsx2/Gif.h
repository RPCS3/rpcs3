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

const int gifsplit = 0x10000;

enum gifstate_t
{
	GIF_STATE_READY = 0,
	GIF_STATE_STALL = 1,
	GIF_STATE_DONE = 2,
	GIF_STATE_EMPTY = 0x10
};

enum Path3Modes //0 = Image Mode (DirectHL), 1 = transferring, 2 = Stopped at End of Packet
{
	IMAGE_MODE = 0,
	TRANSFER_MODE = 1,
	STOPPED_MODE = 2
};

//GIF_STAT
enum gif_stat_flags
{
	GIF_STAT_M3R		= (1),		// GIF_MODE Mask
	GIF_STAT_M3P		= (1<<1),	// VIF PATH3 Mask
	GIF_STAT_IMT		= (1<<2),	// Intermittent Transfer Mode
	GIF_STAT_PSE		= (1<<3),	// Temporary Transfer Stop
	GIF_STAT_IP3		= (1<<5),	// Interrupted PATH3
	GIF_STAT_P3Q		= (1<<6),	// PATH3 request Queued
	GIF_STAT_P2Q		= (1<<7),	// PATH2 request Queued
	GIF_STAT_P1Q		= (1<<8),	// PATH1 request Queued
	GIF_STAT_OPH		= (1<<9),	// Output Path (Outputting Data)
	GIF_STAT_APATH1	= (1<<10),	// Data Transfer Path 1 (In progress)
	GIF_STAT_APATH2	= (2<<10),	// Data Transfer Path 2 (In progress)
	GIF_STAT_APATH3	= (3<<10),	// Data Transfer Path 3 (In progress) (Mask too)
	GIF_STAT_DIR		= (1<<12),	// Transfer Direction
	GIF_STAT_FQC		= (31<<24)	// QWC in GIF-FIFO
};

enum gif_mode_flags
{
	GIF_MODE_M3R	= (1),
	GIF_MODE_IMT		= (1<<2)
};

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

extern Path3Modes Path3progress;

extern void gsInterrupt();
int _GIFchain();
void GIFdma();
void dmaGIF();
void mfifoGIFtransfer(int qwc);
void gifMFIFOInterrupt();

#endif
