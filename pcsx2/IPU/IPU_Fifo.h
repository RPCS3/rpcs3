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

#ifndef IPU_FIFO_H_INCLUDED
#define IPU_FIFO_H_INCLUDED

// Important!  All FIFO containers in this header should be 'struct' type, not class type.
// They are saved into the savestate as-is, and keeping them as struct ensures that the
// layout of their contents is reliable.

struct IPU_Fifo_Input
{
	__aligned16 u32 data[32];
	int readpos, writepos;

	int write(u32* pMem, int size);
	int read(void *value);
	void clear();
	wxString desc() const;
};

struct IPU_Fifo_Output
{
	__aligned16 u32 data[32];
	int readpos, writepos;

	// returns number of qw read
	int write(const u32 * value, uint size);
	void read(void *value, uint size);
	void clear();
	wxString desc() const;
};

struct IPU_Fifo
{
	__aligned16 IPU_Fifo_Input in;
	__aligned16 IPU_Fifo_Output out;

	void init();
	void clear();
};

extern __aligned16 IPU_Fifo ipu_fifo;

#endif // IPU_FIFO_H_INCLUDED
