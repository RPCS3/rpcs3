/*  convert.c
 *  Copyright (C) 2002-2005  PCSX2 Team
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
 *
 *  PCSX2 members can be contacted through their website at www.pcsx2.net.
 */
#include <stddef.h>
#include <sys/types.h>
#include "convert.h"
off64_t ConvertEndianOffset(off64_t number)
{
#ifndef CONVERTLITTLEENDIAN
	union
	{
		off64_t n;
		char c[sizeof(off64_t)];
	} oldnumber, newnumber;
	int i;
	oldnumber.n = number;
	for (i = 0; i < sizeof(off64_t); i++)
		newnumber.c[i] = oldnumber.c[sizeof(off64_t) - 1 - i];
	return(newnumber.n);
#else
	return(number);
#endif /* CONVERTLITTLEENDIAN */
} // END ConvertEndianOffset()

unsigned int ConvertEndianUInt(unsigned int number)
{
#ifndef CONVERTLITTLEENDIAN
	union
	{
		unsigned int n;
		char c[sizeof(unsigned int)];
	} oldnumber, newnumber;
	int i;

	oldnumber.n = number;
	for (i = 0; i < sizeof(unsigned int); i++)
		newnumber.c[i] = oldnumber.c[sizeof(unsigned int) - 1 - i];
	return(newnumber.n);
#else
	return(number);
#endif /* CONVERTLITTLEENDIAN */
} // END ConvertEndianUInt()
unsigned short ConvertEndianUShort(unsigned short number)
{
#ifndef CONVERTLITTLEENDIAN
	union
	{
		unsigned short n;
		char c[sizeof(unsigned short)];
	} oldnumber, newnumber;
	int i;
	oldnumber.n = number;
	for (i = 0; i < sizeof(unsigned short); i++)
		newnumber.c[i] = oldnumber.c[sizeof(unsigned short) - 1 - i];
	return(newnumber.n);
#else
	return(number);
#endif /* CONVERTLITTLEENDIAN */
} // END ConvertEndianUShort()

// Note: deposits M/S/F data in buffer[0]/[1]/[2] respectively.
void LBAtoMSF(unsigned long lsn, char *buffer)
{
	unsigned long templsn;

	if (lsn >= 0xFFFFFFFF - 150)
	{
		*(buffer + 2) = 75 - 1;
		*(buffer + 1) = 60 - 1;
		*(buffer) = 100 - 1;
	} // ENDIF- Out of range?

	templsn = lsn;
	templsn += 150; // 2 second offset (75 Frames * 2 Seconds)
	*(buffer + 2) = templsn % 75; // Remainder in frames
	templsn -= *(buffer + 2);
	templsn /= 75;
	*(buffer + 1) = templsn % 60; // Remainder in seconds
	templsn -= *(buffer + 1);
	templsn /= 60;
	*(buffer) = templsn; // Leftover quotient in minutes
} // END LBAtoMSF()
unsigned long MSFtoLBA(char *buffer)
{
	unsigned long templsn;
	if (buffer == NULL)  return(0xFFFFFFFF);
	templsn = *(buffer); // Minutes
	templsn *= 60;
	templsn += *(buffer + 1); // Seconds
	templsn *= 75;
	templsn += *(buffer + 2); // Frames
	if (templsn < 150)  return(0xFFFFFFFF);
	templsn -= 150; // Offset
	return(templsn);
} // END MSFtoLBA()
