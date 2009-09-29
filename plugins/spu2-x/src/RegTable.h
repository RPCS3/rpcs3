//GiGaHeRz's SPU2 Driver
//Copyright (c) 2003-2008, David Quintana <gigaherz@gmail.com>
//
//This library is free software; you can redistribute it and/or
//modify it under the terms of the GNU Lesser General Public
//License as published by the Free Software Foundation; either
//version 2.1 of the License, or (at your option) any later version.
//
//This library is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//Lesser General Public License for more details.
//
//You should have received a copy of the GNU Lesser General Public
//License along with this library; if not, write to the Free Software
//Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//

#ifndef _REGTABLE_H_
#define _REGTABLE_H_

#define U16P(x)		( (u16*)&(x) )

// Returns the hiword of a 32 bit integer.
#define U16P_HI(x)	( ((u16*)&(x))+1 )

// Yay!  Global namespace pollution 101!
extern const u16 zero;

extern u16* regtable[0x800];

#endif