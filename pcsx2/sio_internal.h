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
