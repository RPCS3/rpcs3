/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2014 David Quintana [gigaherz]
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

template<int from, int to>
int Convert(char *dest, const char* psrc, int lsn);

template<>
int Convert<2048,2048>(char *dest, const char* psrc, int lsn)
{
	memcpy(dest,psrc,2048);
	return 0;
}

template<>
int Convert<2352,2352>(char *dest, const char* psrc, int lsn)
{
	memcpy(dest,psrc,2352);
	return 0;
}

template<>
int Convert<2352,2048>(char *dest, const char* psrc, int lsn)
{
	memcpy(dest,psrc+16,2048);
	return 0;
}

template<>
int Convert<2048,2352>(char *dest, const char* psrc, int lsn)
{
	int m = lsn / (75*60);
	int s = (lsn/75) % 60;
	int f = lsn%75;
	unsigned char header[16] = {
		0x00,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x00,   m,   s,   f,0x02
	};

	memcpy(dest,header,16);
	memcpy(dest+16,psrc,2048);
	return 0;
}
