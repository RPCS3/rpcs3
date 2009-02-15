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
#pragma once
#include "PrecompiledHeader.h"
#include "microVU.h"
#ifdef PCSX2_MICROVU

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

//------------------------------------------------------------------
// Micro VU Micromode Upper instructions
//------------------------------------------------------------------

microVUf(void) mVU_ABS(){}
microVUf(void) mVU_ADD(){}
microVUf(void) mVU_ADDi(){}
microVUf(void) mVU_ADDq(){}
microVUf(void) mVU_ADDx(){}
microVUf(void) mVU_ADDy(){}
microVUf(void) mVU_ADDz(){}
microVUf(void) mVU_ADDw(){}
microVUf(void) mVU_ADDA(){}
microVUf(void) mVU_ADDAi(){}
microVUf(void) mVU_ADDAq(){}
microVUf(void) mVU_ADDAx(){}
microVUf(void) mVU_ADDAy(){}
microVUf(void) mVU_ADDAz(){}
microVUf(void) mVU_ADDAw(){}
microVUf(void) mVU_SUB(){}
microVUf(void) mVU_SUBi(){}
microVUf(void) mVU_SUBq(){}
microVUf(void) mVU_SUBx(){}
microVUf(void) mVU_SUBy(){}
microVUf(void) mVU_SUBz(){}
microVUf(void) mVU_SUBw(){}
microVUf(void) mVU_SUBA(){}
microVUf(void) mVU_SUBAi(){}
microVUf(void) mVU_SUBAq(){}
microVUf(void) mVU_SUBAx(){}
microVUf(void) mVU_SUBAy(){}
microVUf(void) mVU_SUBAz(){}
microVUf(void) mVU_SUBAw(){}
microVUf(void) mVU_MUL(){}
microVUf(void) mVU_MULi(){}
microVUf(void) mVU_MULq(){}
microVUf(void) mVU_MULx(){}
microVUf(void) mVU_MULy(){}
microVUf(void) mVU_MULz(){}
microVUf(void) mVU_MULw(){}
microVUf(void) mVU_MULA(){}
microVUf(void) mVU_MULAi(){}
microVUf(void) mVU_MULAq(){}
microVUf(void) mVU_MULAx(){}
microVUf(void) mVU_MULAy(){}
microVUf(void) mVU_MULAz(){}
microVUf(void) mVU_MULAw(){}
microVUf(void) mVU_MADD(){}
microVUf(void) mVU_MADDi(){}
microVUf(void) mVU_MADDq(){}
microVUf(void) mVU_MADDx(){}
microVUf(void) mVU_MADDy(){}
microVUf(void) mVU_MADDz(){}
microVUf(void) mVU_MADDw(){}
microVUf(void) mVU_MADDA(){}
microVUf(void) mVU_MADDAi(){}
microVUf(void) mVU_MADDAq(){}
microVUf(void) mVU_MADDAx(){}
microVUf(void) mVU_MADDAy(){}
microVUf(void) mVU_MADDAz(){}
microVUf(void) mVU_MADDAw(){}
microVUf(void) mVU_MSUB(){}
microVUf(void) mVU_MSUBi(){}
microVUf(void) mVU_MSUBq(){}
microVUf(void) mVU_MSUBx(){}
microVUf(void) mVU_MSUBy(){}
microVUf(void) mVU_MSUBz(){}
microVUf(void) mVU_MSUBw(){}
microVUf(void) mVU_MSUBA(){}
microVUf(void) mVU_MSUBAi(){}
microVUf(void) mVU_MSUBAq(){}
microVUf(void) mVU_MSUBAx(){}
microVUf(void) mVU_MSUBAy(){}
microVUf(void) mVU_MSUBAz(){}
microVUf(void) mVU_MSUBAw(){}
microVUf(void) mVU_MAX(){}
microVUf(void) mVU_MAXi(){}
microVUf(void) mVU_MAXx(){}
microVUf(void) mVU_MAXy(){}
microVUf(void) mVU_MAXz(){}
microVUf(void) mVU_MAXw(){}
microVUf(void) mVU_MINI(){}
microVUf(void) mVU_MINIi(){}
microVUf(void) mVU_MINIx(){}
microVUf(void) mVU_MINIy(){}
microVUf(void) mVU_MINIz(){}
microVUf(void) mVU_MINIw(){}
microVUf(void) mVU_OPMULA(){}
microVUf(void) mVU_OPMSUB(){}
microVUf(void) mVU_NOP(){}
microVUf(void) mVU_FTOI0(){}
microVUf(void) mVU_FTOI4(){}
microVUf(void) mVU_FTOI12(){}
microVUf(void) mVU_FTOI15(){}
microVUf(void) mVU_ITOF0(){}
microVUf(void) mVU_ITOF4(){}
microVUf(void) mVU_ITOF12(){}
microVUf(void) mVU_ITOF15(){}
microVUf(void) mVU_CLIP(){}
#endif //PCSX2_MICROVU
