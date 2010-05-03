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

struct IsoFileDescriptor
{
	struct FileDate // not 1:1 with iso9660 date struct
	{
		s32	year;
		u8	month;
		u8	day;
		u8	hour;
		u8	minute;
		u8	second;
		u8	gmtOffset; // Offset from Greenwich Mean Time in number of 15 min intervals from -48 (West) to + 52 (East)

	} date;

	u32		lba;
	u32		size;
	int		flags;
	wxString name;

	IsoFileDescriptor();
	IsoFileDescriptor(const u8* data, int length);

	void Load( const u8* data, int length );

	bool IsFile() const { return !(flags & 2); }
	bool IsDir() const { return !IsFile(); }
};
