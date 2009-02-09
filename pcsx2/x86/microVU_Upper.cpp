/*  Pcsx2 - Pc Ps2 Emulator
*  Copyright (C) 2009  Pcsx2-Playground Team
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
*/

#include "PrecompiledHeader.h"

/*
Cotton's Notes on how things will work (*experimental*, subject to change if I get different ideas):

Guide:
Fd, Fs, Ft = operands in the Micro Instructions
Acc = VU's Accumulator register
Fs/t = shorthand notation I made-up for "Fs or Ft"
xmmFd, xmmFs, xmmFt, xmmAcc = XMM regs that hold Fd, Fs, Ft, and Acc values respectively.
xmmZ = XMM reg that holds the zero Register; always {0, 0, 0, 1.0}
xmmT1, xmmT2, xmmT3 = temp regs.

General:
XMM0 is a volatile temp reg throughout the recs. You can always freely use it.
EAX is a volatile temp reg. You can always freely use it.

Mapping:
xmmT1	= xmm0
xmmFd	= xmm1
xmmFs	= xmm2
xmmFt	= xmm3
xmmAcc	= xmm4
xmmT2	= xmm5
xmmT3	= xmm6
xmmZ	= xmm7

Most of the time the above mapping will be true, unless I find a reason not to do it this way :)

Opcodes:
Fd's 4-vectors must be preserved (kept valid); Unless operation is single-scalar, then only 'x' XMM vector 
will contain valid data for X, Y, Z, or W, and the other XMM vectors will be garbage and freely modifiable.

Fs and Ft are temp regs that won't be used after the opcode, so their values can be freely modified.

If (Fd == 0), Then you don't need to explicitly handle this case in the opcode implementation, 
				   since its dealt-with in the analyzing microVU pipeline functions.
				   (So just do the normal operation and don't worry about it.)

If (_X_Y_Z_W == 0) Then same as above. (btw, I'm'm not sure if this case ever happens...)

If (Fd == Fs/t), Then xmmFd != xmmFs/t (unless its more optimized this way! it'll be commented on the opcode)

Clamping:
Fs/t can always be clamped by case 15 (all vectors modified) since they won't be written back.

Problems:
The biggest problem I think I'll have is xgkick opcode having variable timing/stalling.

Other Notes:
These notes are mostly to help me (cottonvibes) remember good ideas and to help confused devs to
have an idea of how things work. Right now its all theoretical and I'll change things once implemented ;p
*/