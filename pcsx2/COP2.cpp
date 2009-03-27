/*  Pcsx2 - Pc Ps2 Emulator
 *  Copyright (C) 2002-2009  Pcsx2 Team
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

#include "Common.h"
#include "DebugTools/Debug.h"
#include "R5900.h"
#include "R5900OpcodeTables.h"
#include "VUops.h"
#include "VUmicro.h"

//namespace R5900 {
//namespace Interpreter {
//namespace OpcodeImpl{

using namespace R5900;
using namespace R5900::Interpreter;

#define CP2COND (((VU0.VI[REG_VPU_STAT].US[0] >> 8) & 1))

void VCALLMS() {
	vu0Finish();
	vu0ExecMicro(((cpuRegs.code >> 6) & 0x7FFF) * 8);
}

void VCALLMSR() {
	vu0Finish();
	vu0ExecMicro(VU0.VI[REG_CMSAR0].US[0] * 8);
}

void BC2F()
{ 
	if (CP2COND == 0) 
	{ 
		Console::WriteLn("VU0 Macro Branch"); 
		intDoBranch(_BranchTarget_); 
	}
}
void BC2T() 
{ 
	if (CP2COND == 1) 
	{ 
		Console::WriteLn("VU0 Macro Branch"); 
		intDoBranch(_BranchTarget_); 
	}
}

void BC2FL()
{ 
	if (CP2COND == 0) 
	{ 
		Console::WriteLn("VU0 Macro Branch"); 
		intDoBranch(_BranchTarget_); 
	}
	else 
	{
		cpuRegs.pc+= 4;
	}
}
void BC2TL() 
{ 
	if (CP2COND == 1) 
	{ 
		Console::WriteLn("VU0 Macro Branch"); 
		intDoBranch(_BranchTarget_); 
	}
	else 
	{
		cpuRegs.pc+= 4;
	}
}

//}}}
