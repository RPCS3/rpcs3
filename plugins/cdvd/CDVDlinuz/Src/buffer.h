/*  buffer.h
 *  Copyright (C) 2002-2005  CDVDlinuz Team
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
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __BUFFER_H__
#define __BUFFER_H__


#define CDVDdefs
#include "PS2Edefs.h"


// #define VERBOSE_FUNCTION_BUFFER
// #define VERBOSE_WARNINGS_BUFFER

// Remember, each buffer set is about 5k (packed. might be 4x that in-memory)
// Minimum: 16   Maximum: 32760
#define BUFFERMAX 256


// Buffer Structures

struct BufferList {
  u16 upsort; // Don't alter this variable
  u16 uppos; // Don't alter this variable

  u32 lsn;
  int mode; // -1 means error
  u8 buffer[2368];
  u8 offset;
  cdvdSubQ subq;
};


// Exported Variables

extern struct BufferList bufferlist[];
extern u16 userbuffer;
extern u16 replacebuffer;


// Exported Functions

extern void InitBuffer();
extern u16 FindListBuffer(u32 lsn);
extern void RemoveListBuffer(u16 oldbuffer);
extern void AddListBuffer(u16 newbuffer);
#ifdef VERBOSE_WARNINGS_BUFFER
extern void PrintSortBuffers();
#endif /* VERBOSE_WARNINGS_BUFFER */


#endif /* __BUFFER_H__ */
