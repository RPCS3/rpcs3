/*  convert.h
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
#ifndef CONVERT_H
#define CONVERT_H

#include <sys/types.h> // off64_t
#include "PS2Etypes.h"
#ifdef __linux__
#include "endian.h"
#if __BYTE_ORDER == __LITTLE_ENDIAN
#define CONVERTLITTLEENDIAN
#endif /* __BYTE_ORDER */
#endif /* __linux__ */
#ifdef _WIN32
#define CONVERTLITTLEENDIAN
#endif /* _WIN32 */
#define HEXTOBCD(i)  (((i)/10*16) + ((i)%10))
#define BCDTOHEX(i)  (((i)/16*10) + ((i)%16))

extern off64_t ConvertEndianOffset(off64_t number);
extern unsigned int ConvertEndianUInt(unsigned int number);
extern unsigned short ConvertEndianUShort(unsigned short number);
extern void LBAtoMSF(unsigned long lsn, char *buffer);
extern unsigned long MSFtoLBA(char *buffer);

#endif /* CONVERT_H */
