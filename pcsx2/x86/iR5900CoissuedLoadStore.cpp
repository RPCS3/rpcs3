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

#ifdef PCSX2_VM_COISSUE

#include "Common.h"
#include "R5900OpcodeTables.h"
#include "iR5900LoadStore.h"
#include "iR5900.h"

namespace R5900 { 
namespace Dynarec { 
namespace OpcodeImpl {

#define _Imm_co_ (*(s16*)PSM(pc))

int _eePrepareReg_coX(int gprreg, int num)
{
	int mmreg = _eePrepareReg(gprreg);

	if( (mmreg&MEM_MMXTAG) && num == 7 ) {
		if( mmxregs[mmreg&0xf].mode & MODE_WRITE ) {
			MOVQRtoM((u32)&cpuRegs.GPR.r[gprreg], mmreg&0xf);
			mmxregs[mmreg&0xf].mode &= ~MODE_WRITE;
			mmxregs[mmreg&0xf].needed = 0;
		}
	}

	return mmreg;
}

void recLoad32_co(u32 bit, u32 sign)
{
	int nextrt = ((*(u32*)(PSM(pc)) >> 16) & 0x1F);
	int mmreg1 = -1, mmreg2 = -1;

#ifdef REC_SLOWREAD
	_flushConstReg(_Rs_);
#else
	if( GPR_IS_CONST1( _Rs_ ) ) {
		int ineax = 0;
		u32 written = 0;

		_eeOnLoadWrite(_Rt_);
		_eeOnLoadWrite(nextrt);

		if( bit == 32 ) {
			mmreg1 = _allocCheckGPRtoMMX(g_pCurInstInfo, _Rt_, MODE_WRITE);
			if( mmreg1 >= 0 ) mmreg1 |= MEM_MMXTAG;
			else mmreg1 = EBX;

			mmreg2 = _allocCheckGPRtoMMX(g_pCurInstInfo+1, nextrt, MODE_WRITE);
			if( mmreg2 >= 0 ) mmreg2 |= MEM_MMXTAG;
			else mmreg2 = EAX;
		}
		else {
			_deleteEEreg(_Rt_, 0);
			_deleteEEreg(nextrt, 0);
			mmreg1 = EBX;
			mmreg2 = EAX;
		}

		// do const processing
		switch(bit) {
			case 8:
				if( recMemConstRead8(mmreg1, g_cpuConstRegs[_Rs_].UL[0]+_Imm_, sign) ) {
					if( mmreg1&MEM_MMXTAG ) mmxregs[mmreg1&0xf].inuse = 0;
					if ( _Rt_ ) recTransferX86ToReg(EAX, _Rt_, sign);
					written = 1;
				}
				ineax = recMemConstRead8(mmreg2, g_cpuConstRegs[_Rs_].UL[0]+_Imm_co_, sign);
				break;
			case 16:
				if( recMemConstRead16(mmreg1, g_cpuConstRegs[_Rs_].UL[0]+_Imm_, sign) ) {
					if( mmreg1&MEM_MMXTAG ) mmxregs[mmreg1&0xf].inuse = 0;
					if ( _Rt_ ) recTransferX86ToReg(EAX, _Rt_, sign);
					written = 1;
				}
				ineax = recMemConstRead16(mmreg2, g_cpuConstRegs[_Rs_].UL[0]+_Imm_co_, sign);
				break;
			case 32:
				assert( (g_cpuConstRegs[_Rs_].UL[0]+_Imm_) % 4 == 0 );
				if( recMemConstRead32(mmreg1, g_cpuConstRegs[_Rs_].UL[0]+_Imm_) ) {
					if( mmreg1&MEM_MMXTAG ) mmxregs[mmreg1&0xf].inuse = 0;
					if ( _Rt_ ) recTransferX86ToReg(EAX, _Rt_, sign);
					written = 1;
				}
				ineax = recMemConstRead32(mmreg2, g_cpuConstRegs[_Rs_].UL[0]+_Imm_co_);
				break;
		}

		if( !written && _Rt_ ) {
			if( mmreg1&MEM_MMXTAG ) {
				assert( mmxregs[mmreg1&0xf].mode & MODE_WRITE );
				if( sign ) _signExtendGPRtoMMX(mmreg1&0xf, _Rt_, 32-bit);
				else if( bit < 32 ) PSRLDItoR(mmreg1&0xf, 32-bit);
			}
			else recTransferX86ToReg(mmreg1, _Rt_, sign);
		}
		if( nextrt ) {
			g_pCurInstInfo++;
			if( !ineax && (mmreg2 & MEM_MMXTAG) ) {
				assert( mmxregs[mmreg2&0xf].mode & MODE_WRITE );
				if( sign ) _signExtendGPRtoMMX(mmreg2&0xf, nextrt, 32-bit);
				else if( bit < 32 ) PSRLDItoR(mmreg2&0xf, 32-bit);
			}
			else {
				if( mmreg2&MEM_MMXTAG ) mmxregs[mmreg2&0xf].inuse = 0;
				recTransferX86ToReg(mmreg2, nextrt, sign);
			}
			g_pCurInstInfo--;
		}
	}
	else
#endif
	{
		int dohw;
		int mmregs = _eePrepareReg(_Rs_);
		assert( !REC_FORCEMMX );

		_eeOnLoadWrite(_Rt_);
		_eeOnLoadWrite(nextrt);
		_deleteEEreg(_Rt_, 0);
		_deleteEEreg(nextrt, 0);

		dohw = recSetMemLocation(_Rs_, _Imm_, mmregs, 0, 0);

		switch(bit) {
			case 8:
				if( sign ) {
					MOVSX32Rm8toROffset(EBX, ECX, PS2MEM_BASE_+s_nAddMemOffset);
					MOVSX32Rm8toROffset(EAX, ECX, PS2MEM_BASE_+s_nAddMemOffset+_Imm_co_-_Imm_);
				}
				else {
					MOVZX32Rm8toROffset(EBX, ECX, PS2MEM_BASE_+s_nAddMemOffset);
					MOVZX32Rm8toROffset(EAX, ECX, PS2MEM_BASE_+s_nAddMemOffset+_Imm_co_-_Imm_);
				}
				break;
			case 16:
				if( sign ) {
					MOVSX32Rm16toROffset(EBX, ECX, PS2MEM_BASE_+s_nAddMemOffset);
					MOVSX32Rm16toROffset(EAX, ECX, PS2MEM_BASE_+s_nAddMemOffset+_Imm_co_-_Imm_);
				}
				else {
					MOVZX32Rm16toROffset(EBX, ECX, PS2MEM_BASE_+s_nAddMemOffset);
					MOVZX32Rm16toROffset(EAX, ECX, PS2MEM_BASE_+s_nAddMemOffset+_Imm_co_-_Imm_);
				}
				break;
			case 32:
				MOV32RmtoROffset(EBX, ECX, PS2MEM_BASE_+s_nAddMemOffset);
				MOV32RmtoROffset(EAX, ECX, PS2MEM_BASE_+s_nAddMemOffset+_Imm_co_-_Imm_);
				break;
		}

		if ( _Rt_ ) recTransferX86ToReg(EBX, _Rt_, sign);

		if( dohw ) {
			if( s_bCachingMem & 2 ) j32Ptr[4] = JMP32(0);
			else j8Ptr[2] = JMP8(0);

			SET_HWLOC_R5900();

			switch(bit) {
				case 8:
					MOV32RtoM((u32)&s_tempaddr, ECX);
					CALLFunc( (int)recMemRead8 );
					if( sign ) MOVSX32R8toR(EAX, EAX);
					MOV32MtoR(ECX, (u32)&s_tempaddr);
					if ( _Rt_ ) recTransferX86ToReg(EAX, _Rt_, sign);

					ADD32ItoR(ECX, _Imm_co_-_Imm_);
					CALLFunc( (int)recMemRead8 );
					if( sign ) MOVSX32R8toR(EAX, EAX);
					break;
				case 16:
					MOV32RtoM((u32)&s_tempaddr, ECX);
					CALLFunc( (int)recMemRead16 );
					if( sign ) MOVSX32R16toR(EAX, EAX);
					MOV32MtoR(ECX, (u32)&s_tempaddr);
					if ( _Rt_ ) recTransferX86ToReg(EAX, _Rt_, sign);

					ADD32ItoR(ECX, _Imm_co_-_Imm_);
					CALLFunc( (int)recMemRead16 );
					if( sign ) MOVSX32R16toR(EAX, EAX);
					break;
				case 32:
					MOV32RtoM((u32)&s_tempaddr, ECX);
					iMemRead32Check();
					CALLFunc( (int)recMemRead32 );
					MOV32MtoR(ECX, (u32)&s_tempaddr);
					if ( _Rt_ ) recTransferX86ToReg(EAX, _Rt_, sign);

					ADD32ItoR(ECX, _Imm_co_-_Imm_);
					iMemRead32Check();
					CALLFunc( (int)recMemRead32 );
					break;
			}

			if( s_bCachingMem & 2 ) x86SetJ32(j32Ptr[4]);
			else x86SetJ8(j8Ptr[2]);
		}

		if( nextrt ) {
			g_pCurInstInfo++;
			recTransferX86ToReg(EAX, nextrt, sign);
			g_pCurInstInfo--;
		}
	}
}

void recLB_co( void ) { recLoad32_co(8, 1); }
void recLBU_co( void ) { recLoad32_co(8, 0); }
void recLH_co( void ) { recLoad32_co(16, 1); }
void recLHU_co( void ) { recLoad32_co(16, 0); }
void recLW_co( void ) { recLoad32_co(32, 1); }
void recLWU_co( void ) { recLoad32_co(32, 0); }
void recLDL_co(void) { recLoad64(_Imm_-7, 0); }
void recLDR_co(void) { recLoad64(_Imm_, 0); }
void recLWL_co(void) { recLoad32(32, _Imm_-3, 1); }	// paired with LWR
void recLWR_co(void) { recLoad32(32, _Imm_, 1); }	// paired with LWL

void recLD_co( void )
{
#ifdef REC_SLOWREAD
	_flushConstReg(_Rs_);
#else
	if( GPR_IS_CONST1( _Rs_ ) ) {
		recLD();
		g_pCurInstInfo++;
		cpuRegs.code = *(u32*)PSM(pc);
		recLD();
		g_pCurInstInfo--; // incremented later
	}
	else
#endif
	{
		int dohw;
		int mmregs = _eePrepareReg(_Rs_);
		int mmreg1 = -1, mmreg2 = -1, t0reg = -1, t1reg = -1;
		int nextrt = ((*(u32*)(PSM(pc)) >> 16) & 0x1F);

		if( _Rt_ ) _eeOnWriteReg(_Rt_, 0);
		if( nextrt ) _eeOnWriteReg(nextrt, 0);

		if( _Rt_ ) {
			mmreg1 = _allocCheckGPRtoMMX(g_pCurInstInfo, _Rt_, MODE_WRITE);
			if( mmreg1 < 0 ) {
				mmreg1 = _allocCheckGPRtoXMM(g_pCurInstInfo, _Rt_, MODE_WRITE|MODE_READ);
				if( mmreg1 >= 0 ) mmreg1 |= 0x8000;
			}

			if( mmreg1 < 0 && _hasFreeMMXreg() ) mmreg1 = _allocMMXreg(-1, MMX_GPR+_Rt_, MODE_WRITE);
		}

		if( nextrt ) {
			mmreg2 = _allocCheckGPRtoMMX(g_pCurInstInfo+1, nextrt, MODE_WRITE);
			if( mmreg2 < 0 ) {
				mmreg2 = _allocCheckGPRtoXMM(g_pCurInstInfo+1, nextrt, MODE_WRITE|MODE_READ);
				if( mmreg2 >= 0 ) mmreg2 |= 0x8000;
			}

			if( mmreg2 < 0 && _hasFreeMMXreg() ) mmreg2 = _allocMMXreg(-1, MMX_GPR+nextrt, MODE_WRITE);
		}

		if( mmreg1 < 0 || mmreg2 < 0 ) {
			t0reg = _allocMMXreg(-1, MMX_TEMP, 0);

			if( mmreg1 < 0 && mmreg2 < 0 && _hasFreeMMXreg() ) {
				t1reg = _allocMMXreg(-1, MMX_TEMP, 0);
			}
			else t1reg = t0reg;
		}

		dohw = recSetMemLocation(_Rs_, _Imm_, mmregs, 1, 0);

		if( mmreg1 >= 0 ) {
			if( mmreg1 & 0x8000 ) SSE_MOVLPSRmtoROffset(mmreg1&0xf, ECX, PS2MEM_BASE_+s_nAddMemOffset);
			else MOVQRmtoROffset(mmreg1, ECX, PS2MEM_BASE_+s_nAddMemOffset);
		}
		else {
			if( _Rt_ ) {
				MOVQRmtoROffset(t0reg, ECX, PS2MEM_BASE_+s_nAddMemOffset);
				MOVQRtoM((int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ], t0reg);
			}
		}

		if( mmreg2 >= 0 ) {
			if( mmreg2 & 0x8000 ) SSE_MOVLPSRmtoROffset(mmreg2&0xf, ECX, PS2MEM_BASE_+s_nAddMemOffset+_Imm_co_-_Imm_);
			else MOVQRmtoROffset(mmreg2, ECX, PS2MEM_BASE_+s_nAddMemOffset+_Imm_co_-_Imm_);
		}
		else {
			if( nextrt ) {
				MOVQRmtoROffset(t1reg, ECX, PS2MEM_BASE_+s_nAddMemOffset+_Imm_co_-_Imm_);
				MOVQRtoM((int)&cpuRegs.GPR.r[ nextrt ].UL[ 0 ], t1reg);
			}
		}

		if( dohw ) {
			if( s_bCachingMem & 2 ) j32Ptr[4] = JMP32(0);
			else j8Ptr[2] = JMP8(0);

			SET_HWLOC_R5900();

			MOV32RtoM((u32)&s_tempaddr, ECX);

			if( mmreg1 >= 0 ) {
				PUSH32I( (int)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ] );
				CALLFunc( (int)recMemRead64 );

				if( mmreg1 & 0x8000 ) SSE_MOVLPS_M64_to_XMM(mmreg1&0xf, (int)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ]);
				else MOVQMtoR(mmreg1, (int)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ]);
			}
			else {
				if( _Rt_ ) {
					_deleteEEreg(_Rt_, 0);
					PUSH32I( (int)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ] );
				}
				else PUSH32I( (int)&retValues[0] );

				CALLFunc( (int)recMemRead64 );
			}

			MOV32MtoR(ECX, (u32)&s_tempaddr);

			if( mmreg2 >= 0 ) {
				MOV32ItoRmOffset(ESP, (int)&cpuRegs.GPR.r[ nextrt ].UD[ 0 ], 0 );
				ADD32ItoR(ECX, _Imm_co_-_Imm_);
				CALLFunc( (int)recMemRead64 );

				if( mmreg2 & 0x8000 ) SSE_MOVLPS_M64_to_XMM(mmreg2&0xf, (int)&cpuRegs.GPR.r[ nextrt ].UD[ 0 ]);
				else MOVQMtoR(mmreg2, (int)&cpuRegs.GPR.r[ nextrt ].UD[ 0 ]);
			}
			else {
				if( nextrt ) {
					_deleteEEreg(nextrt, 0);
					MOV32ItoRmOffset(ESP, (int)&cpuRegs.GPR.r[ nextrt ].UD[ 0 ], 0 );
				}
				else MOV32ItoRmOffset(ESP, (int)&retValues[0], 0 );

				ADD32ItoR(ECX, _Imm_co_-_Imm_);
				CALLFunc( (int)recMemRead64 );
			}

			ADD32ItoR(ESP, 4);

			if( s_bCachingMem & 2 ) x86SetJ32A(j32Ptr[4]);
			else x86SetJ8A(j8Ptr[2]);
		}

		if( mmreg1 < 0 || mmreg2 < 0 || !(mmreg1&0x8000) || !(mmreg2&0x8000) ) SetMMXstate();

		if( t0reg >= 0 ) _freeMMXreg(t0reg);
		if( t0reg != t1reg && t1reg >= 0 ) _freeMMXreg(t1reg);

		_clearNeededMMXregs();
		_clearNeededXMMregs();
	}
}

void recLD_coX( int num )
{
	int i;
	int mmreg = -1;
	int mmregs[XMMREGS];
	int nextrts[XMMREGS];

	assert( num > 1 && num < XMMREGS );

#ifdef REC_SLOWREAD
	_flushConstReg(_Rs_);
#else
	if( GPR_IS_CONST1( _Rs_ ) ) {
		recLD();

		for(i = 0; i < num; ++i) {
			g_pCurInstInfo++;
			cpuRegs.code = *(u32*)PSM(pc+i*4);
			recLD();
		}

		g_pCurInstInfo -= num; // incremented later
	}
	else
#endif
	{
		int dohw;
		int mmregS = _eePrepareReg_coX(_Rs_, num);

		if( _Rt_ ) _eeOnWriteReg(_Rt_, 0);
		for(i = 0; i < num; ++i) {
			nextrts[i] = ((*(u32*)(PSM(pc+i*4)) >> 16) & 0x1F);
			_eeOnWriteReg(nextrts[i], 0);
		}

		if( _Rt_ ) {
			mmreg = _allocCheckGPRtoMMX(g_pCurInstInfo, _Rt_, MODE_WRITE);
			if( mmreg < 0 ) {
				mmreg = _allocCheckGPRtoXMM(g_pCurInstInfo, _Rt_, MODE_WRITE|MODE_READ);
				if( mmreg >= 0 ) mmreg |= MEM_XMMTAG;
				else mmreg = _allocMMXreg(-1, MMX_GPR+_Rt_, MODE_WRITE)|MEM_MMXTAG;
			}
			else mmreg |= MEM_MMXTAG;
		}

		for(i = 0; i < num; ++i) {
			mmregs[i] = _allocCheckGPRtoMMX(g_pCurInstInfo+i+1, nextrts[i], MODE_WRITE);
			if( mmregs[i] < 0 ) {
				mmregs[i] = _allocCheckGPRtoXMM(g_pCurInstInfo+i+1, nextrts[i], MODE_WRITE|MODE_READ);
				if( mmregs[i] >= 0 ) mmregs[i] |= MEM_XMMTAG;
				else mmregs[i] = _allocMMXreg(-1, MMX_GPR+nextrts[i], MODE_WRITE)|MEM_MMXTAG;
			}
			else mmregs[i] |= MEM_MMXTAG;
		}

		dohw = recSetMemLocation(_Rs_, _Imm_, mmregS, 1, 0);

		if( mmreg >= 0 ) {
			if( mmreg & MEM_XMMTAG ) SSE_MOVLPSRmtoROffset(mmreg&0xf, ECX, PS2MEM_BASE_+s_nAddMemOffset);
			else MOVQRmtoROffset(mmreg&0xf, ECX, PS2MEM_BASE_+s_nAddMemOffset);
		}

		for(i = 0; i < num; ++i) {
			u32 off = PS2MEM_BASE_+s_nAddMemOffset+(*(s16*)PSM(pc+i*4))-_Imm_;

			if( mmregs[i] >= 0 ) {
				if( mmregs[i] & MEM_XMMTAG ) SSE_MOVLPSRmtoROffset(mmregs[i]&0xf, ECX, off);
				else MOVQRmtoROffset(mmregs[i]&0xf, ECX, off);
			}
		}

		if( dohw ) {
			if( (s_bCachingMem & 2) || num > 2 ) j32Ptr[4] = JMP32(0);
			else j8Ptr[2] = JMP8(0);

			SET_HWLOC_R5900();

			MOV32RtoM((u32)&s_tempaddr, ECX);

			if( mmreg >= 0 ) {
				PUSH32I( (int)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ] );
				CALLFunc( (int)recMemRead64 );

				if( mmreg & MEM_XMMTAG ) SSE_MOVLPS_M64_to_XMM(mmreg&0xf, (int)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ]);
				else MOVQMtoR(mmreg&0xf, (int)&cpuRegs.GPR.r[ _Rt_ ].UD[ 0 ]);
			}
			else {
				PUSH32I( (int)&retValues[0] );
				CALLFunc( (int)recMemRead64 );
			}

			for(i = 0; i < num; ++i ) {
				MOV32MtoR(ECX, (u32)&s_tempaddr);
				ADD32ItoR(ECX, (*(s16*)PSM(pc+i*4))-_Imm_);

				if( mmregs[i] >= 0 ) {
					MOV32ItoRmOffset(ESP, (int)&cpuRegs.GPR.r[nextrts[i]].UL[0], 0);
					CALLFunc( (int)recMemRead64 );

					if( mmregs[i] & MEM_XMMTAG ) SSE_MOVLPS_M64_to_XMM(mmregs[i]&0xf, (int)&cpuRegs.GPR.r[ nextrts[i] ].UD[ 0 ]);
					else MOVQMtoR(mmregs[i]&0xf, (int)&cpuRegs.GPR.r[ nextrts[i] ].UD[ 0 ]);
				}
				else {
					MOV32ItoRmOffset(ESP, (int)&retValues[0], 0);
					CALLFunc( (int)recMemRead64 );
				}
			}

			ADD32ItoR(ESP, 4);

			if( (s_bCachingMem & 2) || num > 2 ) x86SetJ32A(j32Ptr[4]);
			else x86SetJ8A(j8Ptr[2]);
		}

		_clearNeededMMXregs();
		_clearNeededXMMregs();
	}
}

void recLQ_co( void )
{
#ifdef REC_SLOWREAD
	_flushConstReg(_Rs_);
#else
	if( GPR_IS_CONST1( _Rs_ ) ) {
		recLQ();
		g_pCurInstInfo++;
		cpuRegs.code = *(u32*)PSM(pc);
		recLQ();
		g_pCurInstInfo--; // incremented later
	}
	else
#endif
	{
		int dohw;
		int t0reg = -1;
		int mmregs = _eePrepareReg(_Rs_);

		int nextrt = ((*(u32*)(PSM(pc)) >> 16) & 0x1F);
		int mmreg1 = -1, mmreg2 = -1;

		if( _Rt_ ) {
			_eeOnWriteReg(_Rt_, 0);

			if( _hasFreeMMXreg() ) {
				mmreg1 = _allocCheckGPRtoMMX(g_pCurInstInfo, _Rt_, MODE_WRITE);
				if( mmreg1 >= 0 ) {
					mmreg1 |= MEM_MMXTAG;
					if( t0reg < 0 ) t0reg = _allocMMXreg(-1, MMX_TEMP, 0);
				}
			}
			else _deleteMMXreg(MMX_GPR+_Rt_, 2);

			if( mmreg1 < 0 ) {
				mmreg1 = _allocGPRtoXMMreg(-1, _Rt_, MODE_WRITE);
				if( mmreg1 >= 0 ) mmreg1 |= MEM_XMMTAG;
			}
		}

		if( nextrt ) {
			_eeOnWriteReg(nextrt, 0);

			if( _hasFreeMMXreg() ) {
				mmreg2 = _allocCheckGPRtoMMX(g_pCurInstInfo+1, nextrt, MODE_WRITE);
				if( mmreg2 >= 0 ) {
					mmreg2 |= MEM_MMXTAG;
					if( t0reg < 0 ) t0reg = _allocMMXreg(-1, MMX_TEMP, 0);
				}
			}
			else _deleteMMXreg(MMX_GPR+nextrt, 2);

			if( mmreg2 < 0 ) {
				mmreg2 = _allocGPRtoXMMreg(-1, nextrt, MODE_WRITE);
				if( mmreg2 >= 0 ) mmreg2 |= MEM_XMMTAG;
			}
		}

		dohw = recSetMemLocation(_Rs_, _Imm_, mmregs, 2, 0);

		if( _Rt_ ) {
			if( mmreg1 >= 0 && (mmreg1 & MEM_MMXTAG) ) {
				MOVQRmtoROffset(t0reg, ECX, PS2MEM_BASE_+s_nAddMemOffset+8);
				MOVQRmtoROffset(mmreg1&0xf, ECX, PS2MEM_BASE_+s_nAddMemOffset);
				MOVQRtoM((u32)&cpuRegs.GPR.r[_Rt_].UL[2], t0reg);
			}
			else if( mmreg1 >= 0 && (mmreg1 & MEM_XMMTAG) ) {
				SSEX_MOVDQARmtoROffset(mmreg1&0xf, ECX, PS2MEM_BASE_+s_nAddMemOffset);
			}
			else {
				_recMove128RmOffsettoM((u32)&cpuRegs.GPR.r[_Rt_].UL[0], PS2MEM_BASE_+s_nAddMemOffset);
			}
		}

		if( nextrt ) {
			if( mmreg2 >= 0 && (mmreg2 & MEM_MMXTAG) ) {
				MOVQRmtoROffset(t0reg, ECX, PS2MEM_BASE_+s_nAddMemOffset+_Imm_co_-_Imm_+8);
				MOVQRmtoROffset(mmreg2&0xf, ECX, PS2MEM_BASE_+s_nAddMemOffset+_Imm_co_-_Imm_);
				MOVQRtoM((u32)&cpuRegs.GPR.r[nextrt].UL[2], t0reg);
			}
			else if( mmreg2 >= 0 && (mmreg2 & MEM_XMMTAG) ) {
				SSEX_MOVDQARmtoROffset(mmreg2&0xf, ECX, PS2MEM_BASE_+s_nAddMemOffset+_Imm_co_-_Imm_);
			}
			else {
				_recMove128RmOffsettoM((u32)&cpuRegs.GPR.r[nextrt].UL[0], PS2MEM_BASE_+s_nAddMemOffset);
			}
		}

		if( dohw ) {
			if( s_bCachingMem & 2 ) j32Ptr[4] = JMP32(0);
			else j8Ptr[2] = JMP8(0);

			SET_HWLOC_R5900();

			MOV32RtoM((u32)&s_tempaddr, ECX);
			if( _Rt_ ) PUSH32I( (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ] );
			else PUSH32I( (int)&retValues[0] );
			CALLFunc( (int)recMemRead128 );

			MOV32MtoR(ECX, (u32)&s_tempaddr);
			if( nextrt ) MOV32ItoRmOffset(ESP, (int)&cpuRegs.GPR.r[nextrt].UL[0], 0);
			else MOV32ItoRmOffset(ESP, (int)&retValues[0], 0);
			ADD32ItoR(ECX, _Imm_co_-_Imm_);
			CALLFunc( (int)recMemRead128 );

			if( _Rt_) {
				if( mmreg1 >= 0 && (mmreg1 & MEM_MMXTAG) ) MOVQMtoR(mmreg1&0xf, (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ]);
				else if( mmreg1 >= 0 && (mmreg1 & MEM_XMMTAG) ) SSEX_MOVDQA_M128_to_XMM(mmreg1&0xf, (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ] );
			}
			if( nextrt ) {
				if( mmreg2 >= 0 && (mmreg2 & MEM_MMXTAG) ) MOVQMtoR(mmreg2&0xf, (int)&cpuRegs.GPR.r[ nextrt ].UL[ 0 ]);
				else if( mmreg2 >= 0 && (mmreg2 & MEM_XMMTAG) ) SSEX_MOVDQA_M128_to_XMM(mmreg2&0xf, (int)&cpuRegs.GPR.r[ nextrt ].UL[ 0 ] );
			}
			ADD32ItoR(ESP, 4);

			if( s_bCachingMem & 2 ) x86SetJ32(j32Ptr[4]);
			else x86SetJ8(j8Ptr[2]);
		}

		if( t0reg >= 0 ) _freeMMXreg(t0reg);
	}
}

// coissues more than 2 LQs
void recLQ_coX(int num)
{
	int i;
	int mmreg = -1;
	int mmregs[XMMREGS];
	int nextrts[XMMREGS];

	assert( num > 1 && num < XMMREGS );

#ifdef REC_SLOWREAD
	_flushConstReg(_Rs_);
#else
	if( GPR_IS_CONST1( _Rs_ ) ) {
		recLQ();

		for(i = 0; i < num; ++i) {
			g_pCurInstInfo++;
			cpuRegs.code = *(u32*)PSM(pc+i*4);
			recLQ();
		}

		g_pCurInstInfo -= num; // incremented later
	}
	else
#endif
	{
		int dohw;
		int mmregS = _eePrepareReg_coX(_Rs_, num);


		if( _Rt_ ) _deleteMMXreg(MMX_GPR+_Rt_, 2);
		for(i = 0; i < num; ++i) {
			nextrts[i] = ((*(u32*)(PSM(pc+i*4)) >> 16) & 0x1F);
			if( nextrts[i] ) _deleteMMXreg(MMX_GPR+nextrts[i], 2);
		}

		if( _Rt_ ) {
			_eeOnWriteReg(_Rt_, 0);
			mmreg = _allocGPRtoXMMreg(-1, _Rt_, MODE_WRITE);
		}

		for(i = 0; i < num; ++i) {
			if( nextrts[i] ) {
				_eeOnWriteReg(nextrts[i], 0);
				mmregs[i] = _allocGPRtoXMMreg(-1, nextrts[i], MODE_WRITE);
			}
			else mmregs[i] = -1;
		}

		dohw = recSetMemLocation(_Rs_, _Imm_, mmregS, 2, 1);

		if( _Rt_ ) SSEX_MOVDQARmtoROffset(mmreg, ECX, PS2MEM_BASE_+s_nAddMemOffset);

		for(i = 0; i < num; ++i) {
			u32 off = s_nAddMemOffset+(*(s16*)PSM(pc+i*4))-_Imm_;
			if( nextrts[i] ) SSEX_MOVDQARmtoROffset(mmregs[i], ECX, PS2MEM_BASE_+off&~0xf);
		}

		if( dohw ) {
			if( s_bCachingMem & 2 ) j32Ptr[4] = JMP32(0);
			else j8Ptr[2] = JMP8(0);

			SET_HWLOC_R5900();

			MOV32RtoM((u32)&s_tempaddr, ECX);

			if( _Rt_ ) PUSH32I( (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ] );
			else PUSH32I( (int)&retValues[0] );
			CALLFunc( (int)recMemRead128 );
			if( _Rt_) SSEX_MOVDQA_M128_to_XMM(mmreg, (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ] );

			for(i = 0; i < num; ++i) {
				MOV32MtoR(ECX, (u32)&s_tempaddr);
				ADD32ItoR(ECX, (*(s16*)PSM(pc+i*4))-_Imm_);

				if( nextrts[i] ) MOV32ItoRmOffset(ESP, (int)&cpuRegs.GPR.r[nextrts[i]].UL[0], 0);
				else MOV32ItoRmOffset(ESP, (int)&retValues[0], 0);
				CALLFunc( (int)recMemRead128 );		
				if( nextrts[i] ) SSEX_MOVDQA_M128_to_XMM(mmregs[i], (int)&cpuRegs.GPR.r[ nextrts[i] ].UL[ 0 ] );
			}

			ADD32ItoR(ESP, 4);

			if( s_bCachingMem & 2 ) x86SetJ32A(j32Ptr[4]);
			else x86SetJ8A(j8Ptr[2]);
		}
	}
}

void recStore_co(int bit, int align)
{
	int nextrt = ((*(u32*)(PSM(pc)) >> 16) & 0x1F);

#ifdef REC_SLOWWRITE
	_flushConstReg(_Rs_);
#else
	if( GPR_IS_CONST1( _Rs_ ) ) {
		u32 addr = g_cpuConstRegs[_Rs_].UL[0]+_Imm_;
		u32 coaddr = g_cpuConstRegs[_Rs_].UL[0]+_Imm_co_;
		int mmreg, t0reg = -1, mmreg2;
		int doclear = 0;

		switch(bit) {
			case 8:
				if( GPR_IS_CONST1(_Rt_) ) doclear |= recMemConstWrite8(addr, MEM_EECONSTTAG|(_Rt_<<16));
				else {
					_eeMoveGPRtoR(EAX, _Rt_);
					doclear |= recMemConstWrite8(addr, EAX);
				}
				if( GPR_IS_CONST1(nextrt) ) doclear |= recMemConstWrite8(coaddr, MEM_EECONSTTAG|(nextrt<<16));
				else {
					_eeMoveGPRtoR(EAX, nextrt);
					doclear |= recMemConstWrite8(coaddr, EAX);
				}
				break;
			case 16:
				assert( (addr)%2 == 0 );
				assert( (coaddr)%2 == 0 );

				if( GPR_IS_CONST1(_Rt_) ) doclear |= recMemConstWrite16(addr, MEM_EECONSTTAG|(_Rt_<<16));
				else {
					_eeMoveGPRtoR(EAX, _Rt_);
					doclear |= recMemConstWrite16(addr, EAX);
				}

				if( GPR_IS_CONST1(nextrt) ) doclear |= recMemConstWrite16(coaddr, MEM_EECONSTTAG|(nextrt<<16));
				else {
					_eeMoveGPRtoR(EAX, nextrt);
					doclear |= recMemConstWrite16(coaddr, EAX);
				}
				break;
			case 32:
				assert( (addr)%4 == 0 );
				if( GPR_IS_CONST1(_Rt_) ) doclear = recMemConstWrite32(addr, MEM_EECONSTTAG|(_Rt_<<16));
				else if( (mmreg = _checkXMMreg(XMMTYPE_GPRREG,  _Rt_, MODE_READ)) >= 0 ) {
					doclear = recMemConstWrite32(addr, mmreg|MEM_XMMTAG|(_Rt_<<16));
				}
				else if( (mmreg = _checkMMXreg(MMX_GPR+_Rt_, MODE_READ)) >= 0 ) {
					doclear = recMemConstWrite32(addr, mmreg|MEM_MMXTAG|(_Rt_<<16));
				}
				else {
					_eeMoveGPRtoR(EAX, _Rt_);
					doclear = recMemConstWrite32(addr, EAX);
				}

				if( GPR_IS_CONST1(nextrt) ) doclear |= recMemConstWrite32(coaddr, MEM_EECONSTTAG|(nextrt<<16));
				else if( (mmreg = _checkXMMreg(XMMTYPE_GPRREG, nextrt, MODE_READ)) >= 0 ) {
					doclear |= recMemConstWrite32(coaddr, mmreg|MEM_XMMTAG|(nextrt<<16));
				}
				else if( (mmreg = _checkMMXreg(MMX_GPR+nextrt, MODE_READ)) >= 0 ) {
					doclear |= recMemConstWrite32(coaddr, mmreg|MEM_MMXTAG|(nextrt<<16));
				}
				else {
					_eeMoveGPRtoR(EAX, nextrt);
					doclear |= recMemConstWrite32(coaddr, EAX);
				}

				break;
			case 64:
				{
					int mask = align ? ~7 : ~0;
					//assert( (addr)%8 == 0 );

					if( GPR_IS_CONST1(_Rt_) ) mmreg = MEM_EECONSTTAG|(_Rt_<<16);
					else if( (mmreg = _checkXMMreg(XMMTYPE_GPRREG,  _Rt_, MODE_READ)) >= 0 ) {
						mmreg |= MEM_XMMTAG|(_Rt_<<16);
					}
					else mmreg = _allocMMXreg(-1, MMX_GPR+_Rt_, MODE_READ)|MEM_MMXTAG|(_Rt_<<16);

					if( GPR_IS_CONST1(nextrt) ) mmreg2 = MEM_EECONSTTAG|(nextrt<<16);
					else if( (mmreg2 = _checkXMMreg(XMMTYPE_GPRREG,  nextrt, MODE_READ)) >= 0 ) {
						mmreg2 |= MEM_XMMTAG|(nextrt<<16);
					}
					else mmreg2 = _allocMMXreg(-1, MMX_GPR+nextrt, MODE_READ)|MEM_MMXTAG|(nextrt<<16);

					doclear = recMemConstWrite64((addr)&mask, mmreg);
					doclear |= recMemConstWrite64((coaddr)&mask, mmreg2);
					doclear <<= 1;
					break;
				}
			case 128:
				assert( (addr)%16 == 0 );

				mmreg = _eePrepConstWrite128(_Rt_);
				mmreg2 = _eePrepConstWrite128(nextrt);
				doclear = recMemConstWrite128((addr)&~15, mmreg);
				doclear |= recMemConstWrite128((coaddr)&~15, mmreg2);
				doclear <<= 2;
				break;
		}

		if( doclear ) {
			u8* ptr;
			CMP32ItoM((u32)&maxrecmem, g_cpuConstRegs[_Rs_].UL[0]+(_Imm_ < _Imm_co_ ? _Imm_ : _Imm_co_));
			ptr = JB8(0);
			recMemConstClear((addr)&~(doclear*4-1), doclear);
			recMemConstClear((coaddr)&~(doclear*4-1), doclear);
			x86SetJ8A(ptr);
		}
	}
	else
#endif
	{
		int dohw;
		int mmregs = _eePrepareReg(_Rs_);
		int off = _Imm_co_-_Imm_;
		dohw = recSetMemLocation(_Rs_, _Imm_, mmregs, align ? bit/64 : 0, bit==128);

		recStore_raw(g_pCurInstInfo, bit, EAX, _Rt_, s_nAddMemOffset);
		recStore_raw(g_pCurInstInfo+1, bit, EBX, nextrt, s_nAddMemOffset+off);

		// clear the writes, do only one camera (with the lowest addr)
		if( off < 0 ) ADD32ItoR(ECX, s_nAddMemOffset+off);
		else if( s_nAddMemOffset ) ADD32ItoR(ECX, s_nAddMemOffset);
		CMP32MtoR(ECX, (u32)&maxrecmem);

		if( s_bCachingMem & 2 ) j32Ptr[5] = JAE32(0);
		else j8Ptr[1] = JAE8(0);

		MOV32RtoM((u32)&s_tempaddr, ECX);

		if( bit < 32 ) AND8ItoR(ECX, 0xfc);
		if( bit <= 32 ) CALLFunc((uptr)recWriteMemClear32);
		else if( bit == 64 ) CALLFunc((uptr)recWriteMemClear64);
		else CALLFunc((uptr)recWriteMemClear128);

		MOV32MtoR(ECX, (u32)&s_tempaddr);
		if( off < 0 ) ADD32ItoR(ECX, -off);
		else ADD32ItoR(ECX, off);

		if( bit < 32 ) AND8ItoR(ECX, 0xfc);
		if( bit <= 32 ) CALLFunc((uptr)recWriteMemClear32);
		else if( bit == 64 ) CALLFunc((uptr)recWriteMemClear64);
		else CALLFunc((uptr)recWriteMemClear128);

		if( dohw ) {
			if( s_bCachingMem & 2 ) j32Ptr[4] = JMP32(0);
			else j8Ptr[2] = JMP8(0);

			SET_HWLOC_R5900();

			MOV32RtoM((u32)&s_tempaddr, ECX);
			recStore_call(bit, _Rt_, s_nAddMemOffset);
			MOV32MtoR(ECX, (u32)&s_tempaddr);
			recStore_call(bit, nextrt, s_nAddMemOffset+_Imm_co_-_Imm_);

			if( s_bCachingMem & 2 ) x86SetJ32(j32Ptr[4]);
			else x86SetJ8(j8Ptr[2]);
		}

		if( s_bCachingMem & 2 ) x86SetJ32(j32Ptr[5]);
		else x86SetJ8(j8Ptr[1]);
	}

	_clearNeededMMXregs(); // needed since allocing
	_clearNeededXMMregs(); // needed since allocing
}

void recSB_co( void ) { recStore_co(8, 1); }
void recSH_co( void ) { recStore_co(16, 1); }
void recSW_co( void ) { recStore_co(32, 1); }
void recSD_co( void ) { recStore_co(64, 1); }
void recSQ_co( void ) {	recStore_co(128, 1); }

void recSWL_co(void) { recStore(32, _Imm_-3, 0); }
void recSWR_co(void) { recStore(32, _Imm_, 0); }
void recSDL_co(void) { recStore(64, _Imm_-7, 0); }
void recSDR_co(void) { recStore(64, _Imm_, 0); }

// coissues more than 2 SDs
void recSD_coX(int num, int align) 
{	
	int i;
	int mmreg = -1;
	int nextrts[XMMREGS];
	u32 mask = align ? ~7 : ~0;

	assert( num > 1 && num < XMMREGS );

	for(i = 0; i < num; ++i) {
		nextrts[i] = ((*(u32*)(PSM(pc+i*4)) >> 16) & 0x1F);
	}

#ifdef REC_SLOWWRITE
	_flushConstReg(_Rs_);
#else
	if( GPR_IS_CONST1( _Rs_ ) ) {
		int minimm = _Imm_;
		int t0reg = -1;
		int doclear = 0;

		if( GPR_IS_CONST1(_Rt_) ) mmreg = MEM_EECONSTTAG|(_Rt_<<16);
		else if( (mmreg = _checkXMMreg(XMMTYPE_GPRREG,  _Rt_, MODE_READ)) >= 0 ) {
			mmreg |= MEM_XMMTAG|(_Rt_<<16);
		}
		else mmreg = _allocMMXreg(-1, MMX_GPR+_Rt_, MODE_READ)|MEM_MMXTAG|(_Rt_<<16);
		doclear |= recMemConstWrite64((g_cpuConstRegs[_Rs_].UL[0]+_Imm_)&mask, mmreg);

		for(i = 0; i < num; ++i) {
			int imm = (*(s16*)PSM(pc+i*4));
			if( minimm > imm ) minimm = imm;

			if( GPR_IS_CONST1(nextrts[i]) ) mmreg = MEM_EECONSTTAG|(nextrts[i]<<16);
			else if( (mmreg = _checkXMMreg(XMMTYPE_GPRREG,  nextrts[i], MODE_READ)) >= 0 ) {
				mmreg |= MEM_XMMTAG|(nextrts[i]<<16);
			}
			else mmreg = _allocMMXreg(-1, MMX_GPR+nextrts[i], MODE_READ)|MEM_MMXTAG|(nextrts[i]<<16);
			doclear |= recMemConstWrite64((g_cpuConstRegs[_Rs_].UL[0]+(*(s16*)PSM(pc+i*4)))&mask, mmreg);
		}

		if( doclear ) {
			u32* ptr;
			CMP32ItoM((u32)&maxrecmem, g_cpuConstRegs[_Rs_].UL[0]+minimm);
			ptr = JB32(0);
			recMemConstClear((g_cpuConstRegs[_Rs_].UL[0]+_Imm_)&~7, 4);

			for(i = 0; i < num; ++i) {
				recMemConstClear((g_cpuConstRegs[_Rs_].UL[0]+(*(s16*)PSM(pc+i*4)))&~7, 2);
			}
			x86SetJ32A(ptr);
		}
	}
	else
#endif
	{
		int dohw;
		int mmregs = _eePrepareReg_coX(_Rs_, num);
		int minoff = 0;

		dohw = recSetMemLocation(_Rs_, _Imm_, mmregs, align, 1);

		recStore_raw(g_pCurInstInfo, 64, EAX, _Rt_, s_nAddMemOffset);

		for(i = 0; i < num; ++i) {
			recStore_raw(g_pCurInstInfo+i+1, 64, EAX, nextrts[i], s_nAddMemOffset+(*(s16*)PSM(pc+i*4))-_Imm_);
		}

		// clear the writes
		minoff = _Imm_;
		for(i = 0; i < num; ++i) {
			if( minoff > (*(s16*)PSM(pc+i*4)) ) minoff = (*(s16*)PSM(pc+i*4));
		}

		if( s_nAddMemOffset || minoff != _Imm_ ) ADD32ItoR(ECX, s_nAddMemOffset+minoff-_Imm_);
		CMP32MtoR(ECX, (u32)&maxrecmem);
		if( s_bCachingMem & 2 ) j32Ptr[5] = JAE32(0);
		else j8Ptr[1] = JAE8(0);

		MOV32RtoM((u32)&s_tempaddr, ECX);
		if( minoff != _Imm_ ) ADD32ItoR(ECX, _Imm_-minoff);
		CALLFunc((uptr)recWriteMemClear64);

		for(i = 0; i < num; ++i) {
			MOV32MtoR(ECX, (u32)&s_tempaddr);
			if( minoff != (*(s16*)PSM(pc+i*4)) ) ADD32ItoR(ECX, (*(s16*)PSM(pc+i*4))-minoff);
			CALLFunc((uptr)recWriteMemClear64);
		}

		if( dohw ) {
			if( s_bCachingMem & 2 ) j32Ptr[4] = JMP32(0);
			else j8Ptr[2] = JMP8(0);

			SET_HWLOC_R5900();

			MOV32RtoM((u32)&s_tempaddr, ECX);
			recStore_call(64, _Rt_, s_nAddMemOffset);
			for(i = 0; i < num; ++i) {
				MOV32MtoR(ECX, (u32)&s_tempaddr);
				recStore_call(64, nextrts[i], s_nAddMemOffset+(*(s16*)PSM(pc+i*4))-_Imm_);
			}

			if( s_bCachingMem & 2 ) x86SetJ32(j32Ptr[4]);
			else x86SetJ8(j8Ptr[2]);
		}

		if( s_bCachingMem & 2 ) x86SetJ32(j32Ptr[5]);
		else x86SetJ8(j8Ptr[1]);

		_clearNeededMMXregs(); // needed since allocing
		_clearNeededXMMregs(); // needed since allocing
	}
}

// coissues more than 2 SQs
void recSQ_coX(int num) 
{	
	int i;
	int mmreg = -1;
	int nextrts[XMMREGS];

	assert( num > 1 && num < XMMREGS );

	for(i = 0; i < num; ++i) {
		nextrts[i] = ((*(u32*)(PSM(pc+i*4)) >> 16) & 0x1F);
	}

#ifdef REC_SLOWWRITE
	_flushConstReg(_Rs_);
#else
	if( GPR_IS_CONST1( _Rs_ ) ) {
		int minimm = _Imm_;
		int t0reg = -1;
		int doclear = 0;

		mmreg = _eePrepConstWrite128(_Rt_);
		doclear |= recMemConstWrite128((g_cpuConstRegs[_Rs_].UL[0]+_Imm_)&~15, mmreg);

		for(i = 0; i < num; ++i) {
			int imm = (*(s16*)PSM(pc+i*4));
			if( minimm > imm ) minimm = imm;

			mmreg = _eePrepConstWrite128(nextrts[i]);
			doclear |= recMemConstWrite128((g_cpuConstRegs[_Rs_].UL[0]+imm)&~15, mmreg);
		}

		if( doclear ) {
			u32* ptr;
			CMP32ItoM((u32)&maxrecmem, g_cpuConstRegs[_Rs_].UL[0]+minimm);
			ptr = JB32(0);
			recMemConstClear((g_cpuConstRegs[_Rs_].UL[0]+_Imm_)&~15, 4);

			for(i = 0; i < num; ++i) {
				recMemConstClear((g_cpuConstRegs[_Rs_].UL[0]+(*(s16*)PSM(pc+i*4)))&~15, 4);
			}
			x86SetJ32A(ptr);
		}
	}
	else
#endif
	{
		int dohw;
		int mmregs = _eePrepareReg_coX(_Rs_, num);
		int minoff = 0;

		dohw = recSetMemLocation(_Rs_, _Imm_, mmregs, 2, 1);

		recStore_raw(g_pCurInstInfo, 128, EAX, _Rt_, s_nAddMemOffset);

		for(i = 0; i < num; ++i) {
			recStore_raw(g_pCurInstInfo+i+1, 128, EAX, nextrts[i], s_nAddMemOffset+(*(s16*)PSM(pc+i*4))-_Imm_);
		}

		// clear the writes
		minoff = _Imm_;
		for(i = 0; i < num; ++i) {
			if( minoff > (*(s16*)PSM(pc+i*4)) ) minoff = (*(s16*)PSM(pc+i*4));
		}

		if( s_nAddMemOffset || minoff != _Imm_ ) ADD32ItoR(ECX, s_nAddMemOffset+minoff-_Imm_);
		CMP32MtoR(ECX, (u32)&maxrecmem);
		if( s_bCachingMem & 2 ) j32Ptr[5] = JAE32(0);
		else j8Ptr[1] = JAE8(0);

		MOV32RtoM((u32)&s_tempaddr, ECX);
		if( minoff != _Imm_ ) ADD32ItoR(ECX, _Imm_-minoff);
		CALLFunc((uptr)recWriteMemClear128);

		for(i = 0; i < num; ++i) {
			MOV32MtoR(ECX, (u32)&s_tempaddr);
			if( minoff != (*(s16*)PSM(pc+i*4)) ) ADD32ItoR(ECX, (*(s16*)PSM(pc+i*4))-minoff);
			CALLFunc((uptr)recWriteMemClear128);
		}

		if( dohw ) {
			if( s_bCachingMem & 2 ) j32Ptr[4] = JMP32(0);
			else j8Ptr[2] = JMP8(0);

			SET_HWLOC_R5900();

			MOV32RtoM((u32)&s_tempaddr, ECX);
			recStore_call(128, _Rt_, s_nAddMemOffset);
			for(i = 0; i < num; ++i) {
				MOV32MtoR(ECX, (u32)&s_tempaddr);
				recStore_call(128, nextrts[i], s_nAddMemOffset+(*(s16*)PSM(pc+i*4))-_Imm_);
			}

			if( s_bCachingMem & 2 ) x86SetJ32(j32Ptr[4]);
			else x86SetJ8(j8Ptr[2]);
		}

		if( s_bCachingMem & 2 ) x86SetJ32(j32Ptr[5]);
		else x86SetJ8(j8Ptr[1]);

		_clearNeededMMXregs(); // needed since allocing
		_clearNeededXMMregs(); // needed since allocing
	}
}

void recLWC1_co( void )
{
	int nextrt = ((*(u32*)(PSM(pc)) >> 16) & 0x1F);

#ifdef REC_SLOWREAD
	_flushConstReg(_Rs_);
#else
	if( GPR_IS_CONST1( _Rs_ ) ) {
		u32 written = 0;
		int ineax, mmreg1, mmreg2;

		mmreg1 = _allocCheckFPUtoXMM(g_pCurInstInfo, _Rt_, MODE_WRITE);
		if( mmreg1 >= 0 ) mmreg1 |= MEM_XMMTAG;
		else mmreg1 = EBX;

		mmreg2 = _allocCheckFPUtoXMM(g_pCurInstInfo+1, nextrt, MODE_WRITE);
		if( mmreg2 >= 0 ) mmreg2 |= MEM_XMMTAG;
		else mmreg2 = EAX;

		assert( (g_cpuConstRegs[_Rs_].UL[0]+_Imm_) % 4 == 0 );
		if( recMemConstRead32(mmreg1, g_cpuConstRegs[_Rs_].UL[0]+_Imm_) ) {
			if( mmreg1&MEM_XMMTAG ) xmmregs[mmreg1&0xf].inuse = 0;
			MOV32RtoM( (int)&fpuRegs.fpr[ _Rt_ ].UL, EBX );
			written = 1;
		}
		ineax = recMemConstRead32(mmreg2, g_cpuConstRegs[_Rs_].UL[0]+_Imm_co_);

		if( !written ) {
			if( !(mmreg1&MEM_XMMTAG) ) MOV32RtoM( (int)&fpuRegs.fpr[ _Rt_ ].UL, EBX );
		}

		if( ineax || !(mmreg2 & MEM_XMMTAG) ) {
			if( mmreg2&MEM_XMMTAG ) xmmregs[mmreg2&0xf].inuse = 0;
			MOV32RtoM( (int)&fpuRegs.fpr[ nextrt ].UL, EAX );
		}
	}
	else
#endif
	{
		int dohw;
		int regt, regtnext;
		int mmregs = _eePrepareReg(_Rs_);

		_deleteMMXreg(MMX_FPU+_Rt_, 2);
		regt = _allocCheckFPUtoXMM(g_pCurInstInfo, _Rt_, MODE_WRITE);
		regtnext = _allocCheckFPUtoXMM(g_pCurInstInfo+1, nextrt, MODE_WRITE);

		dohw = recSetMemLocation(_Rs_, _Imm_, mmregs, 0, 0);

		if( regt >= 0 ) {
			SSEX_MOVD_RmOffset_to_XMM(regt, ECX, PS2MEM_BASE_+s_nAddMemOffset);
		}
		else {
			MOV32RmtoROffset(EAX, ECX, PS2MEM_BASE_+s_nAddMemOffset);
			MOV32RtoM( (int)&fpuRegs.fpr[ _Rt_ ].UL, EAX );
		}

		if( regtnext >= 0 ) {
			SSEX_MOVD_RmOffset_to_XMM(regtnext, ECX, PS2MEM_BASE_+s_nAddMemOffset+_Imm_co_-_Imm_);
		}
		else {
			MOV32RmtoROffset(EAX, ECX, PS2MEM_BASE_+s_nAddMemOffset+_Imm_co_-_Imm_);
			MOV32RtoM( (int)&fpuRegs.fpr[ nextrt ].UL, EAX );
		}

		if( dohw ) {
			if( s_bCachingMem & 2 ) j32Ptr[4] = JMP32(0);
			else j8Ptr[2] = JMP8(0);

			SET_HWLOC_R5900();

			PUSH32R(ECX);
			CALLFunc( (int)recMemRead32 );
			POP32R(ECX);

			if( regt >= 0 ) {
				SSE2_MOVD_R_to_XMM(regt, EAX);
			}
			else {
				MOV32RtoM( (int)&fpuRegs.fpr[ _Rt_ ].UL, EAX );
			}

			ADD32ItoR(ECX, _Imm_co_-_Imm_);
			CALLFunc( (int)recMemRead32 );

			if( regtnext >= 0 ) {
				SSE2_MOVD_R_to_XMM(regtnext, EAX);
			}
			else {
				MOV32RtoM( (int)&fpuRegs.fpr[ nextrt ].UL, EAX );
			}

			if( s_bCachingMem & 2 ) x86SetJ32A(j32Ptr[4]);
			else x86SetJ8A(j8Ptr[2]);
		}
	}
}

void recLWC1_coX(int num)
{
	int i;
	int mmreg = -1;
	int mmregs[XMMREGS];
	int nextrts[XMMREGS];

	assert( num > 1 && num < XMMREGS );

#ifdef REC_SLOWREAD
	_flushConstReg(_Rs_);
#else
	if( GPR_IS_CONST1( _Rs_ ) ) {
		int ineax;
		u32 written = 0;
		mmreg = _allocCheckFPUtoXMM(g_pCurInstInfo, _Rt_, MODE_WRITE);
		if( mmreg >= 0 ) mmreg |= MEM_XMMTAG;
		else mmreg = EAX;

		assert( (g_cpuConstRegs[_Rs_].UL[0]+_Imm_) % 4 == 0 );
		if( recMemConstRead32(mmreg, g_cpuConstRegs[_Rs_].UL[0]+_Imm_) ) {
			if( mmreg&MEM_XMMTAG ) xmmregs[mmreg&0xf].inuse = 0;
			MOV32RtoM( (int)&fpuRegs.fpr[ _Rt_ ].UL, EBX );
			written = 1;
		}
		else if( !IS_XMMREG(mmreg) ) MOV32RtoM( (int)&fpuRegs.fpr[ _Rt_ ].UL, EAX );

		// recompile two at a time
		for(i = 0; i < num-1; i += 2) {
			nextrts[0] = ((*(u32*)(PSM(pc+i*4)) >> 16) & 0x1F);
			nextrts[1] = ((*(u32*)(PSM(pc+i*4+4)) >> 16) & 0x1F);

			written = 0;
			mmregs[0] = _allocCheckFPUtoXMM(g_pCurInstInfo+i+1, nextrts[0], MODE_WRITE);
			if( mmregs[0] >= 0 ) mmregs[0] |= MEM_XMMTAG;
			else mmregs[0] = EBX;

			mmregs[1] = _allocCheckFPUtoXMM(g_pCurInstInfo+i+2, nextrts[1], MODE_WRITE);
			if( mmregs[1] >= 0 ) mmregs[1] |= MEM_XMMTAG;
			else mmregs[1] = EAX;

			assert( (g_cpuConstRegs[_Rs_].UL[0]+_Imm_) % 4 == 0 );
			if( recMemConstRead32(mmregs[0], g_cpuConstRegs[_Rs_].UL[0]+(*(s16*)PSM(pc+i*4))) ) {
				if( mmregs[0]&MEM_XMMTAG ) xmmregs[mmregs[0]&0xf].inuse = 0;
				MOV32RtoM( (int)&fpuRegs.fpr[ nextrts[0] ].UL, EBX );
				written = 1;
			}
			ineax = recMemConstRead32(mmregs[1], g_cpuConstRegs[_Rs_].UL[0]+(*(s16*)PSM(pc+i*4+4)));

			if( !written ) {
				if( !(mmregs[0]&MEM_XMMTAG) ) MOV32RtoM( (int)&fpuRegs.fpr[ nextrts[0] ].UL, EBX );
			}

			if( ineax || !(mmregs[1] & MEM_XMMTAG) ) {
				if( mmregs[1]&MEM_XMMTAG ) xmmregs[mmregs[1]&0xf].inuse = 0;
				MOV32RtoM( (int)&fpuRegs.fpr[ nextrts[1] ].UL, EAX );
			}
		}

		if( i < num ) {
			// one left
			int nextrt = ((*(u32*)(PSM(pc+i*4)) >> 16) & 0x1F);

			mmreg = _allocCheckFPUtoXMM(g_pCurInstInfo+i+1, nextrt, MODE_WRITE);
			if( mmreg >= 0 ) mmreg |= MEM_XMMTAG;
			else mmreg = EAX;

			assert( (g_cpuConstRegs[_Rs_].UL[0]+_Imm_) % 4 == 0 );
			if( recMemConstRead32(mmreg, g_cpuConstRegs[_Rs_].UL[0]+(*(s16*)PSM(pc+i*4))) ) {
				if( mmreg&MEM_XMMTAG ) xmmregs[mmreg&0xf].inuse = 0;
				MOV32RtoM( (int)&fpuRegs.fpr[ nextrt ].UL, EBX );
				written = 1;
			}
			else if( !IS_XMMREG(mmreg) ) MOV32RtoM( (int)&fpuRegs.fpr[ nextrt ].UL, EAX );
		}
	}
	else
#endif
	{
		int dohw;
		int mmregS = _eePrepareReg_coX(_Rs_, num);


		_deleteMMXreg(MMX_FPU+_Rt_, 2);
		for(i = 0; i < num; ++i) {
			nextrts[i] = ((*(u32*)(PSM(pc+i*4)) >> 16) & 0x1F);
			_deleteMMXreg(MMX_FPU+nextrts[i], 2);
		}

		mmreg = _allocCheckFPUtoXMM(g_pCurInstInfo, _Rt_, MODE_WRITE);

		for(i = 0; i < num; ++i) {
			mmregs[i] = _allocCheckFPUtoXMM(g_pCurInstInfo+i+1, nextrts[i], MODE_WRITE);
		}

		dohw = recSetMemLocation(_Rs_, _Imm_, mmregS, 0, 1);

		if( mmreg >= 0 ) {
			SSEX_MOVD_RmOffset_to_XMM(mmreg, ECX, PS2MEM_BASE_+s_nAddMemOffset);
		}
		else {
			MOV32RmtoROffset(EAX, ECX, PS2MEM_BASE_+s_nAddMemOffset);
			MOV32RtoM( (int)&fpuRegs.fpr[ _Rt_ ].UL, EAX );
		}

		for(i = 0; i < num; ++i) {
			if( mmregs[i] >= 0 ) {
				SSEX_MOVD_RmOffset_to_XMM(mmregs[i], ECX, PS2MEM_BASE_+s_nAddMemOffset+(*(s16*)PSM(pc+i*4))-_Imm_);
			}
			else {
				MOV32RmtoROffset(EAX, ECX, PS2MEM_BASE_+s_nAddMemOffset+(*(s16*)PSM(pc+i*4))-_Imm_);
				MOV32RtoM( (int)&fpuRegs.fpr[ nextrts[i] ].UL, EAX );
			}
		}

		if( dohw ) {
			if( s_bCachingMem & 2 ) j32Ptr[4] = JMP32(0);
			else j8Ptr[2] = JMP8(0);

			SET_HWLOC_R5900();

			MOV32RtoM((u32)&s_tempaddr, ECX);
			CALLFunc( (int)recMemRead32 );

			if( mmreg >= 0 ) SSE2_MOVD_R_to_XMM(mmreg, EAX);
			else MOV32RtoM( (int)&fpuRegs.fpr[ _Rt_ ].UL, EAX );

			for(i = 0; i < num; ++i) {
				MOV32MtoR(ECX, (u32)&s_tempaddr);
				ADD32ItoR(ECX, (*(s16*)PSM(pc+i*4))-_Imm_);
				CALLFunc( (int)recMemRead32 );

				if( mmregs[i] >= 0 ) SSE2_MOVD_R_to_XMM(mmregs[i], EAX);
				else MOV32RtoM( (int)&fpuRegs.fpr[ nextrts[i] ].UL, EAX );
			}

			if( s_bCachingMem & 2 ) x86SetJ32A(j32Ptr[4]);
			else x86SetJ8A(j8Ptr[2]);
		}
	}
}

void recSWC1_co( void )
{
	int nextrt = ((*(u32*)(PSM(pc)) >> 16) & 0x1F);

#ifdef REC_SLOWWRITE
	_flushConstReg(_Rs_);
#else
	if( GPR_IS_CONST1( _Rs_ ) ) {
		int mmreg;
		assert( (g_cpuConstRegs[_Rs_].UL[0]+_Imm_)%4 == 0 );

		mmreg = _checkXMMreg(XMMTYPE_FPREG, _Rt_, MODE_READ);
		if( mmreg >= 0 ) mmreg |= MEM_XMMTAG|(_Rt_<<16);
		else {
			MOV32MtoR(EAX, (int)&fpuRegs.fpr[ _Rt_ ].UL);
			mmreg = EAX;
		}
		recMemConstWrite32(g_cpuConstRegs[_Rs_].UL[0]+_Imm_, mmreg);

		mmreg = _checkXMMreg(XMMTYPE_FPREG, nextrt, MODE_READ);
		if( mmreg >= 0 ) mmreg |= MEM_XMMTAG|(nextrt<<16);
		else {
			MOV32MtoR(EAX, (int)&fpuRegs.fpr[ nextrt ].UL);
			mmreg = EAX;
		}
		recMemConstWrite32(g_cpuConstRegs[_Rs_].UL[0]+_Imm_co_, mmreg);
	}
	else
#endif
	{
		int dohw;
		int mmregs = _eePrepareReg(_Rs_);

		int mmreg1, mmreg2;
		assert( _checkMMXreg(MMX_FPU+_Rt_, MODE_READ) == -1 );
		assert( _checkMMXreg(MMX_FPU+nextrt, MODE_READ) == -1 );

		mmreg1 = _checkXMMreg(XMMTYPE_FPREG, _Rt_, MODE_READ);
		mmreg2 = _checkXMMreg(XMMTYPE_FPREG, nextrt, MODE_READ);

		dohw = recSetMemLocation(_Rs_, _Imm_, mmregs, 0, 0);

		if(mmreg1 >= 0 ) {
			if( mmreg2 >= 0 ) {
				SSEX_MOVD_XMM_to_RmOffset(ECX, mmreg1, PS2MEM_BASE_+s_nAddMemOffset);
				SSEX_MOVD_XMM_to_RmOffset(ECX, mmreg2, PS2MEM_BASE_+s_nAddMemOffset+_Imm_co_-_Imm_);
			}
			else {
				MOV32MtoR(EAX, (int)&fpuRegs.fpr[ nextrt ].UL);
				SSEX_MOVD_XMM_to_RmOffset(ECX, mmreg1, PS2MEM_BASE_+s_nAddMemOffset);
				MOV32RtoRmOffset(ECX, EAX, PS2MEM_BASE_+s_nAddMemOffset+_Imm_co_-_Imm_);
			}
		}
		else {
			if( mmreg2 >= 0 ) {
				MOV32MtoR(EAX, (int)&fpuRegs.fpr[ _Rt_ ].UL);
				SSEX_MOVD_XMM_to_RmOffset(ECX, mmreg2, PS2MEM_BASE_+s_nAddMemOffset+_Imm_co_-_Imm_);
				MOV32RtoRmOffset(ECX, EAX, PS2MEM_BASE_+s_nAddMemOffset);
			}
			else {
				MOV32MtoR(EAX, (int)&fpuRegs.fpr[ _Rt_ ].UL);
				MOV32MtoR(EDX, (int)&fpuRegs.fpr[ nextrt ].UL);
				MOV32RtoRmOffset(ECX, EAX, PS2MEM_BASE_+s_nAddMemOffset);
				MOV32RtoRmOffset(ECX, EDX, PS2MEM_BASE_+s_nAddMemOffset+_Imm_co_-_Imm_);
			}
		}

		if( dohw ) {
			if( s_bCachingMem & 2 ) j32Ptr[4] = JMP32(0);
			else j8Ptr[2] = JMP8(0);

			SET_HWLOC_R5900();

			MOV32RtoM((u32)&s_tempaddr, ECX);

			// some type of hardware write
			if( mmreg1 >= 0) SSE2_MOVD_XMM_to_R(EAX, mmreg1);
			else MOV32MtoR(EAX, (int)&fpuRegs.fpr[ _Rt_ ].UL);
			CALLFunc( (int)recMemWrite32 );

			MOV32MtoR(ECX, (u32)&s_tempaddr);
			ADD32ItoR(ECX, _Imm_co_-_Imm_);
			if( mmreg2 >= 0) SSE2_MOVD_XMM_to_R(EAX, mmreg2);
			else MOV32MtoR(EAX, (int)&fpuRegs.fpr[ nextrt ].UL);
			CALLFunc( (int)recMemWrite32 );

			if( s_bCachingMem & 2 ) x86SetJ32A(j32Ptr[4]);
			else x86SetJ8A(j8Ptr[2]);
		}
	}
}

void recSWC1_coX(int num)
{
	int i;
	int mmreg = -1;
	int mmregs[XMMREGS];
	int nextrts[XMMREGS];

	assert( num > 1 && num < XMMREGS );

	for(i = 0; i < num; ++i) {
		nextrts[i] = ((*(u32*)(PSM(pc+i*4)) >> 16) & 0x1F);
	}

#ifdef REC_SLOWWRITE
	_flushConstReg(_Rs_);
#else
	if( GPR_IS_CONST1( _Rs_ ) ) {
		assert( (g_cpuConstRegs[_Rs_].UL[0]+_Imm_)%4 == 0 );

		mmreg = _checkXMMreg(XMMTYPE_FPREG, _Rt_, MODE_READ);
		if( mmreg >= 0 ) mmreg |= MEM_XMMTAG|(_Rt_<<16);
		else {
			MOV32MtoR(EAX, (int)&fpuRegs.fpr[ _Rt_ ].UL);
			mmreg = EAX;
		}
		recMemConstWrite32(g_cpuConstRegs[_Rs_].UL[0]+_Imm_, mmreg);

		for(i = 0; i < num; ++i) {
			mmreg = _checkXMMreg(XMMTYPE_FPREG, nextrts[i], MODE_READ);
			if( mmreg >= 0 ) mmreg |= MEM_XMMTAG|(nextrts[i]<<16);
			else {
				MOV32MtoR(EAX, (int)&fpuRegs.fpr[ nextrts[i] ].UL);
				mmreg = EAX;
			}
			recMemConstWrite32(g_cpuConstRegs[_Rs_].UL[0]+(*(s16*)PSM(pc+i*4)), mmreg);
		}
	}
	else
#endif
	{
		int dohw;
		int mmregS = _eePrepareReg_coX(_Rs_, num);

		assert( _checkMMXreg(MMX_FPU+_Rt_, MODE_READ) == -1 );
		mmreg = _checkXMMreg(XMMTYPE_FPREG, _Rt_, MODE_READ);

		for(i = 0; i < num; ++i) {
			assert( _checkMMXreg(MMX_FPU+nextrts[i], MODE_READ) == -1 );
			mmregs[i] = _checkXMMreg(XMMTYPE_FPREG, nextrts[i], MODE_READ);
		}

		dohw = recSetMemLocation(_Rs_, _Imm_, mmregS, 0, 1);

		if( mmreg >= 0) {
			SSEX_MOVD_XMM_to_RmOffset(ECX, mmreg, PS2MEM_BASE_+s_nAddMemOffset);
		}
		else {
			MOV32MtoR(EAX, (int)&fpuRegs.fpr[ _Rt_ ].UL);
			MOV32RtoRmOffset(ECX, EAX, PS2MEM_BASE_+s_nAddMemOffset);
		}

		for(i = 0; i < num; ++i) {
			if( mmregs[i] >= 0) {
				SSEX_MOVD_XMM_to_RmOffset(ECX, mmregs[i], PS2MEM_BASE_+s_nAddMemOffset+(*(s16*)PSM(pc+i*4))-_Imm_);
			}
			else {
				MOV32MtoR(EAX, (int)&fpuRegs.fpr[ nextrts[i] ].UL);
				MOV32RtoRmOffset(ECX, EAX, PS2MEM_BASE_+s_nAddMemOffset+(*(s16*)PSM(pc+i*4))-_Imm_);
			}
		}

		if( dohw ) {
			if( s_bCachingMem & 2 ) j32Ptr[4] = JMP32(0);
			else j8Ptr[2] = JMP8(0);

			SET_HWLOC_R5900();

			MOV32RtoM((u32)&s_tempaddr, ECX);

			// some type of hardware write
			if( mmreg >= 0) SSE2_MOVD_XMM_to_R(EAX, mmreg);
			else MOV32MtoR(EAX, (int)&fpuRegs.fpr[ _Rt_ ].UL);
			CALLFunc( (int)recMemWrite32 );

			for(i = 0; i < num; ++i) {
				MOV32MtoR(ECX, (u32)&s_tempaddr);
				ADD32ItoR(ECX, (*(s16*)PSM(pc+i*4))-_Imm_);
				if( mmregs[i] >= 0 && (xmmregs[mmregs[i]].mode&MODE_WRITE) ) SSE2_MOVD_XMM_to_R(EAX, mmregs[i]);
				else MOV32MtoR(EAX, (int)&fpuRegs.fpr[ nextrts[i] ].UL);
				CALLFunc( (int)recMemWrite32 );
			}

			if( s_bCachingMem & 2 ) x86SetJ32A(j32Ptr[4]);
			else x86SetJ8A(j8Ptr[2]);
		}
	}
}

void recLQC2_co( void ) 
{
	int mmreg1 = -1, mmreg2 = -1, t0reg = -1;
	int nextrt = ((*(u32*)(PSM(pc)) >> 16) & 0x1F);

#ifdef REC_SLOWREAD
	_flushConstReg(_Rs_);
#else
	if( GPR_IS_CONST1( _Rs_ ) ) {
		assert( (g_cpuConstRegs[_Rs_].UL[0]+_Imm_) % 16 == 0 );

		if( _Ft_ ) mmreg1 = _allocVFtoXMMreg(&VU0, -1, _Ft_, MODE_WRITE);
		else t0reg = _allocTempXMMreg(XMMT_FPS, -1);
		recMemConstRead128((g_cpuConstRegs[_Rs_].UL[0]+_Imm_)&~15, mmreg1 >= 0 ? mmreg1 : t0reg);

		if( nextrt ) mmreg2 = _allocVFtoXMMreg(&VU0, -1, nextrt, MODE_WRITE);
		else if( t0reg < 0 ) t0reg = _allocTempXMMreg(XMMT_FPS, -1);
		recMemConstRead128((g_cpuConstRegs[_Rs_].UL[0]+_Imm_co_)&~15, mmreg2 >= 0 ? mmreg2 : t0reg);

		if( t0reg >= 0 ) _freeXMMreg(t0reg);
	}
	else
#endif
	{
		u8* rawreadptr;
		int dohw;
		int mmregs = _eePrepareReg(_Rs_);

		if( _Ft_ ) mmreg1 = _allocVFtoXMMreg(&VU0, -1, _Ft_, MODE_WRITE);
		if( nextrt ) mmreg2 = _allocVFtoXMMreg(&VU0, -1, nextrt, MODE_WRITE);

		dohw = recSetMemLocation(_Rs_, _Imm_, mmregs, 2, 0);

		rawreadptr = x86Ptr[0];
		if( mmreg1 >= 0 ) SSEX_MOVDQARmtoROffset(mmreg1, ECX, PS2MEM_BASE_+s_nAddMemOffset);
		if( mmreg2 >= 0 ) SSEX_MOVDQARmtoROffset(mmreg2, ECX, PS2MEM_BASE_+s_nAddMemOffset+_Imm_co_-_Imm_);

		if( dohw ) {
			j8Ptr[1] = JMP8(0);
			SET_HWLOC_R5900();

			// check if writing to VUs
			CMP32ItoR(ECX, 0x11000000);
			JAE8(rawreadptr - (x86Ptr[0]+2));

			MOV32RtoM((u32)&s_tempaddr, ECX);

			if( _Ft_ ) PUSH32I( (int)&VU0.VF[_Ft_].UD[0] );
			else PUSH32I( (int)&retValues[0] );
			CALLFunc( (int)recMemRead128 );

			if( mmreg1 >= 0 ) SSEX_MOVDQA_M128_to_XMM(mmreg1, (int)&VU0.VF[_Ft_].UD[0] );

			MOV32MtoR(ECX, (u32)&s_tempaddr);
			ADD32ItoR(ECX, _Imm_co_-_Imm_);

			if( nextrt ) MOV32ItoRmOffset(ESP, (int)&VU0.VF[nextrt].UD[0], 0 );
			else MOV32ItoRmOffset(ESP, (int)&retValues[0], 0 );
			CALLFunc( (int)recMemRead128 );

			if( mmreg2 >= 0 ) SSEX_MOVDQA_M128_to_XMM(mmreg2, (int)&VU0.VF[nextrt].UD[0] );

			ADD32ItoR(ESP, 4);
			x86SetJ8(j8Ptr[1]);
		}
	}

	_clearNeededXMMregs(); // needed since allocing
}

void recSQC2_co( void ) 
{
	int nextrt = ((*(u32*)(PSM(pc)) >> 16) & 0x1F);
	int mmreg1, mmreg2;

#ifdef REC_SLOWWRITE
	_flushConstReg(_Rs_);
#else
	if( GPR_IS_CONST1( _Rs_ ) ) {
		assert( (g_cpuConstRegs[_Rs_].UL[0]+_Imm_)%16 == 0 );

		mmreg1 = _allocVFtoXMMreg(&VU0, -1, _Ft_, MODE_READ)|MEM_XMMTAG;
		mmreg2 = _allocVFtoXMMreg(&VU0, -1, nextrt, MODE_READ)|MEM_XMMTAG;
		recMemConstWrite128((g_cpuConstRegs[_Rs_].UL[0]+_Imm_)&~15, mmreg1);
		recMemConstWrite128((g_cpuConstRegs[_Rs_].UL[0]+_Imm_co_)&~15, mmreg2);
	}
	else
#endif
	{
		u8* rawreadptr;
		int dohw;
		int mmregs = _eePrepareReg(_Rs_);

		mmreg1 = _checkXMMreg(XMMTYPE_VFREG, _Ft_, MODE_READ);
		mmreg2 = _checkXMMreg(XMMTYPE_VFREG, nextrt, MODE_READ);

		dohw = recSetMemLocation(_Rs_, _Imm_, mmregs, 2, 0);

		rawreadptr = x86Ptr[0];

		if( mmreg1 >= 0 ) {
			SSEX_MOVDQARtoRmOffset(ECX, mmreg1, PS2MEM_BASE_+s_nAddMemOffset);
		}
		else {	
			if( _hasFreeXMMreg() ) {
				mmreg1 = _allocTempXMMreg(XMMT_FPS, -1);
				SSEX_MOVDQA_M128_to_XMM(mmreg1, (int)&VU0.VF[_Ft_].UD[0]);
				SSEX_MOVDQARtoRmOffset(ECX, mmreg1, PS2MEM_BASE_+s_nAddMemOffset);
				_freeXMMreg(mmreg1);
			}
			else if( _hasFreeMMXreg() ) {
				mmreg1 = _allocMMXreg(-1, MMX_TEMP, 0);
				MOVQMtoR(mmreg1, (int)&VU0.VF[_Ft_].UD[0]);
				MOVQRtoRmOffset(ECX, mmreg1, PS2MEM_BASE_+s_nAddMemOffset);
				MOVQMtoR(mmreg1, (int)&VU0.VF[_Ft_].UL[2]);
				MOVQRtoRmOffset(ECX, mmreg1, PS2MEM_BASE_+s_nAddMemOffset+8);
				SetMMXstate();
				_freeMMXreg(mmreg1);
			}
			else {
				MOV32MtoR(EAX, (int)&VU0.VF[_Ft_].UL[0]);
				MOV32MtoR(EDX, (int)&VU0.VF[_Ft_].UL[1]);
				MOV32RtoRmOffset(ECX, EAX, PS2MEM_BASE_+s_nAddMemOffset);
				MOV32RtoRmOffset(ECX, EDX, PS2MEM_BASE_+s_nAddMemOffset+4);
				MOV32MtoR(EAX, (int)&VU0.VF[_Ft_].UL[2]);
				MOV32MtoR(EDX, (int)&VU0.VF[_Ft_].UL[3]);
				MOV32RtoRmOffset(ECX, EAX, PS2MEM_BASE_+s_nAddMemOffset+8);
				MOV32RtoRmOffset(ECX, EDX, PS2MEM_BASE_+s_nAddMemOffset+12);
			}
		}

		if( mmreg2 >= 0 ) {
			SSEX_MOVDQARtoRmOffset(ECX, mmreg2, PS2MEM_BASE_+s_nAddMemOffset+_Imm_co_-_Imm_);
		}
		else {	
			if( _hasFreeXMMreg() ) {
				mmreg2 = _allocTempXMMreg(XMMT_FPS, -1);
				SSEX_MOVDQA_M128_to_XMM(mmreg2, (int)&VU0.VF[nextrt].UD[0]);
				SSEX_MOVDQARtoRmOffset(ECX, mmreg2, PS2MEM_BASE_+s_nAddMemOffset+_Imm_co_-_Imm_);
				_freeXMMreg(mmreg2);
			}
			else if( _hasFreeMMXreg() ) {
				mmreg2 = _allocMMXreg(-1, MMX_TEMP, 0);
				MOVQMtoR(mmreg2, (int)&VU0.VF[nextrt].UD[0]);
				MOVQRtoRmOffset(ECX, mmreg2, PS2MEM_BASE_+s_nAddMemOffset+_Imm_co_-_Imm_);
				MOVQMtoR(mmreg2, (int)&VU0.VF[nextrt].UL[2]);
				MOVQRtoRmOffset(ECX, mmreg2, PS2MEM_BASE_+s_nAddMemOffset+_Imm_co_-_Imm_+8);
				SetMMXstate();
				_freeMMXreg(mmreg2);
			}
			else {
				MOV32MtoR(EAX, (int)&VU0.VF[nextrt].UL[0]);
				MOV32MtoR(EDX, (int)&VU0.VF[nextrt].UL[1]);
				MOV32RtoRmOffset(ECX, EAX, PS2MEM_BASE_+s_nAddMemOffset+_Imm_co_-_Imm_);
				MOV32RtoRmOffset(ECX, EDX, PS2MEM_BASE_+s_nAddMemOffset+_Imm_co_-_Imm_+4);
				MOV32MtoR(EAX, (int)&VU0.VF[nextrt].UL[2]);
				MOV32MtoR(EDX, (int)&VU0.VF[nextrt].UL[3]);
				MOV32RtoRmOffset(ECX, EAX, PS2MEM_BASE_+s_nAddMemOffset+_Imm_co_-_Imm_+8);
				MOV32RtoRmOffset(ECX, EDX, PS2MEM_BASE_+s_nAddMemOffset+_Imm_co_-_Imm_+12);
			}
		}

		if( dohw ) {
			j8Ptr[1] = JMP8(0);

			SET_HWLOC_R5900();

			// check if writing to VUs
			CMP32ItoR(ECX, 0x11000000);
			JAE8(rawreadptr - (x86Ptr[0]+2));

			// some type of hardware write
			if( mmreg1 >= 0) {
				if( xmmregs[mmreg1].mode & MODE_WRITE ) {
					SSEX_MOVDQA_XMM_to_M128((int)&VU0.VF[_Ft_].UD[0], mmreg1);
				}
			}

			MOV32RtoM((u32)&s_tempaddr, ECX);

			MOV32ItoR(EAX, (int)&VU0.VF[_Ft_].UD[0]);
			CALLFunc( (int)recMemWrite128 );

			if( mmreg2 >= 0) {
				if( xmmregs[mmreg2].mode & MODE_WRITE ) {
					SSEX_MOVDQA_XMM_to_M128((int)&VU0.VF[nextrt].UD[0], mmreg2);
				}
			}

			MOV32MtoR(ECX, (u32)&s_tempaddr);
			ADD32ItoR(ECX, _Imm_co_-_Imm_);

			MOV32ItoR(EAX, (int)&VU0.VF[nextrt].UD[0]);
			CALLFunc( (int)recMemWrite128 );

			x86SetJ8A(j8Ptr[1]);
		}
	}
}

#endif	// PCSX2_VM_COISSUE
