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

#pragma once

// sio_internal.h -- contains defines and structs used by sio and sio2, which
// are of little or no use to the rest of the world.

// Status Flags
static const int
	TX_RDY =		0x0001,
	RX_RDY =		0x0002,
	TX_EMPTY =		0x0004,
	PARITY_ERR =	0x0008,
	RX_OVERRUN =	0x0010,
	FRAMING_ERR =	0x0020,
	SYNC_DETECT =	0x0040,
	DSR =			0x0080,
	CTS =			0x0100,
	IRQ =			0x0200;

// Control Flags
static const int
	TX_PERM =		0x0001,
	DTR =			0x0002,
	RX_PERM =		0x0004,
	BREAK =			0x0008,
	RESET_ERR =		0x0010,
	RTS =			0x0020,
	SIO_RESET =		0x0040;

void inline SIO_STAT_READY()
{
	sio.StatReg &= ~TX_EMPTY;	// Now the Buffer is not empty
	sio.StatReg |= RX_RDY;		// Transfer is Ready
}

void inline SIO_STAT_EMPTY()
{
	sio.StatReg &= ~RX_RDY;		// Receive is not Ready now?
	sio.StatReg |= TX_EMPTY;	// Buffer is Empty
}

void inline DEVICE_PLUGGED()
{
	sio.ret = 0xFF;
	sio2.packet.recvVal1 = 0x01100;
	memset8<0xFF>(sio.buf);
}

void inline DEVICE_UNPLUGGED()
{
	sio.ret = 0x00;
	sio2.packet.recvVal1 = 0x1D100;
	memset8<0x00>(sio.buf);
}

enum SIO_MODE
{
	SIO_START = 0,
	SIO_CONTROLLER,
	SIO_MULTITAP,
	SIO_INFRARED,
	SIO_MEMCARD,
	SIO_MEMCARD_AUTH,
	SIO_MEMCARD_ERASE,
	SIO_MEMCARD_WRITE,
	SIO_MEMCARD_READ,
	SIO_MEMCARD_SECTOR,
	SIO_MEMCARD_PSX,
	SIO_DUMMY
};


#ifdef _MSC_VER
#pragma pack(1)
#endif
struct mc_command_0x26_tag{
	u8	field_151;			//+02 flags
	u16	sectorSize;			//+03 Size of each sector(page), in bytes.
	u16 eraseBlocks;		//+05 Number of sectors in the erase block
	u32	mcdSizeInSectors;	//+07 card size in sectors (pages).
	u8	mc_xor;				//+0b XOR Checksum of the superblock? (minus format ident and version?)
	u8	Z;					//+0c terminator?  Appears to be overwritten/unused.
#ifdef _MSC_VER
};
#pragma pack()
#else
} __attribute__((packed));
#endif

