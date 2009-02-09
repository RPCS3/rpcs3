/*  Pcsx2 - Pc Ps2 Emulator
*  Copyright (C) 2002-2008  Pcsx2 Team
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
#include "iR5900.h"

#include "IPU.h"

///////////////////////////////////////////////////////////////////////
//  IPU Register Reads

int ipuConstRead32(u32 x86reg, u32 mem)
{
	int workingreg, tempreg, tempreg2;
	iFlushCall(0);
	CALLFunc((u32)IPUProcessInterrupt);

	//	if( !(x86reg&(MEM_XMMTAG|MEM_MMXTAG)) ) {
	//		if( x86reg == EAX ) {
	//			tempreg =  ECX;
	//			tempreg2 = EDX;
	//		}
	//		else if( x86reg == ECX ) {
	//			tempreg =  EAX;
	//			tempreg2 = EDX;
	//		}
	//		else if( x86reg == EDX ) {
	//			tempreg =  EAX;
	//			tempreg2 = ECX;
	//		}
	//
	//		workingreg = x86reg;
	//	}
	//	else {
	workingreg = EAX;
	tempreg =  ECX;
	tempreg2 = EDX;
	//	}

	switch (mem){

		case 0x10002010: // IPU_CTRL

			MOV32MtoR(workingreg, (u32)&ipuRegs->ctrl._u32);
			AND32ItoR(workingreg, ~0x3f0f); // save OFC
			OR8MtoR(workingreg, (u32)&g_BP.IFC);
			OR8MtoR(workingreg+4, (u32)&coded_block_pattern); // or ah, mem

			//			MOV32MtoR(workingreg, (u32)&ipuRegs->ctrl._u32);
			//			AND32ItoR(workingreg, ~0x3fff);
			//			MOV32MtoR(tempreg, (u32)&g_nIPU0Data);
			//			MOV8MtoR(workingreg, (u32)&g_BP.IFC);
			//
			//			CMP32ItoR(tempreg, 8);
			//			j8Ptr[5] = JLE8(0);
			//			MOV32ItoR(tempreg, 8);
			//			x86SetJ8( j8Ptr[5] );
			//			SHL32ItoR(tempreg, 4);
			//
			//			OR8MtoR(workingreg+4, (u32)&coded_block_pattern); // or ah, mem
			//			OR8RtoR(workingreg, tempreg);

#ifdef _DEBUG
			MOV32RtoM((u32)&ipuRegs->ctrl._u32, workingreg);
#endif
			// NOTE: not updating ipuRegs->ctrl
			//			if( x86reg & MEM_XMMTAG ) SSE2_MOVD_R_to_XMM(x86reg&0xf, workingreg);
			//			else if( x86reg & MEM_MMXTAG ) MOVD32RtoMMX(x86reg&0xf, workingreg);
			return 1;

		case 0x10002020: // IPU_BP

			assert( (u32)&g_BP.FP + 1 == (u32)&g_BP.bufferhasnew );

			MOVZX32M8toR(workingreg, (u32)&g_BP.BP);
			MOVZX32M8toR(tempreg, (u32)&g_BP.FP);
			AND8ItoR(workingreg, 0x7f);
			ADD8MtoR(tempreg, (u32)&g_BP.bufferhasnew);
			MOV8MtoR(workingreg+4, (u32)&g_BP.IFC);

			SHL32ItoR(tempreg, 16);
			OR32RtoR(workingreg, tempreg);

#ifdef _DEBUG
			MOV32RtoM((u32)&ipuRegs->ipubp, workingreg);
#endif
			// NOTE: not updating ipuRegs->ipubp
			//			if( x86reg & MEM_XMMTAG ) SSE2_MOVD_R_to_XMM(x86reg&0xf, workingreg);
			//			else if( x86reg & MEM_MMXTAG ) MOVD32RtoMMX(x86reg&0xf, workingreg);

			return 1;

		default:
			// ipu repeats every 0x100
			_eeReadConstMem32(x86reg, (u32)(((u8*)ipuRegs)+(mem&0xff)));
			return 0;
	}

	return 0;
}

void ipuConstRead64(u32 mem, int mmreg)
{
	iFlushCall(0);
	CALLFunc((u32)IPUProcessInterrupt);

	if( IS_XMMREG(mmreg) ) SSE_MOVLPS_M64_to_XMM(mmreg&0xff, (u32)(((u8*)ipuRegs)+(mem&0xff)));
	else {
		MOVQMtoR(mmreg, (u32)(((u8*)ipuRegs)+(mem&0xff)));
		SetMMXstate();
	}
}

///////////////////////////////////////////////////////////////////////
//  IPU Register Writes!

void ipuConstWrite32(u32 mem, int mmreg)
{
	iFlushCall(0);
	if( !(mmreg & (MEM_XMMTAG|MEM_MMXTAG|MEM_EECONSTTAG)) ) PUSH32R(mmreg);
	CALLFunc((u32)IPUProcessInterrupt);

	switch (mem){
		case 0x10002000: // IPU_CMD
			if( (mmreg & (MEM_XMMTAG|MEM_MMXTAG|MEM_EECONSTTAG)) ) _recPushReg(mmreg);
			CALLFunc((u32)IPUCMD_WRITE);
			ADD32ItoR(ESP, 4);
			break;
		case 0x10002010: // IPU_CTRL
			if( mmreg & MEM_EECONSTTAG ) {
				u32 c = g_cpuConstRegs[(mmreg>>16)&0x1f].UL[0]&0x47f30000;

				if( c & 0x40000000 ) {
					CALLFunc((u32)ipuSoftReset);
				}
				else {
					AND32ItoM((u32)&ipuRegs->ctrl._u32, 0x8000ffff);
					OR32ItoM((u32)&ipuRegs->ctrl._u32, c);
				}
			}
			else {
				if( mmreg & MEM_XMMTAG ) SSE2_MOVD_XMM_to_R(EAX, mmreg&0xf);
				else if( mmreg & MEM_MMXTAG ) MOVD32MMXtoR(EAX, mmreg&0xf);
				else POP32R(EAX);

				MOV32MtoR(ECX, (u32)&ipuRegs->ctrl._u32);
				AND32ItoR(EAX, 0x47f30000);
				AND32ItoR(ECX, 0x8000ffff);
				OR32RtoR(EAX, ECX);
				MOV32RtoM((u32)&ipuRegs->ctrl._u32, EAX);

				TEST32ItoR(EAX, 0x40000000);
				j8Ptr[5] = JZ8(0);

				// reset
				CALLFunc((u32)ipuSoftReset);

				x86SetJ8( j8Ptr[5] );
			}

			break;
		default:
			if( !(mmreg & (MEM_XMMTAG|MEM_MMXTAG|MEM_EECONSTTAG)) ) POP32R(mmreg);
			_eeWriteConstMem32((u32)((u8*)ipuRegs + (mem&0xfff)), mmreg);
			break;
	}
}

void ipuConstWrite64(u32 mem, int mmreg)
{
	iFlushCall(0);
	CALLFunc((u32)IPUProcessInterrupt);

	switch (mem){
		case 0x10002000:
			_recPushReg(mmreg);
			CALLFunc((u32)IPUCMD_WRITE);
			ADD32ItoR(ESP, 4);
			break;

		default:
			_eeWriteConstMem64( (u32)((u8*)ipuRegs + (mem&0xfff)), mmreg);
			break;
	}
}
