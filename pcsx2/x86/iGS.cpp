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
#include "VU.h"

#include "ix86/ix86.h"
#include "iR5900.h"

#include "GS.h"
#include "DebugTools/Debug.h"
	
extern u8 g_RealGSMem[0x2000];
#define PS2GS_BASE(mem) (g_RealGSMem+(mem&0x13ff))

// __thiscall -- Calling Convention Notes.

// ** MSVC passes the pointer to the object as ECX.  Other parameters are passed normally
// (_cdecl style).  Stack is cleaned by the callee.

// ** GCC works just like a __cdecl, except the pointer to the object is pushed onto the
// stack last (passed as the first parameter).  Caller cleans up the stack.

// The GCC code below is untested.  Hope it works. :|  (air)


// Used to send 8, 16, and 32 bit values to the MTGS.
static void __fastcall _rec_mtgs_Send32orSmaller( GS_RINGTYPE ringtype, u32 mem, int mmreg )
{
	iFlushCall(0);

	PUSH32I( 0 );
	_callPushArg( mmreg, 0 );
    PUSH32I( mem&0x13ff );
    PUSH32I( ringtype );

#ifdef _MSC_VER
	MOV32ItoR( ECX, (uptr)mtgsThread );
	CALLFunc( mtgsThread->FnPtr_SimplePacket() );
#else	// GCC -->
	PUSH32I( (uptr)mtgsThread );
	CALLFunc( mtgsThread->FnPtr_SimplePacket() );
	ADD32ItoR( ESP, 20 );
#endif
}

// Used to send 64 and 128 bit values to the MTGS (called twice for 128's, which
// is why it doesn't call iFlushCall)
static void __fastcall _rec_mtgs_Send64( uptr gsbase, u32 mem, int mmreg )
{
    PUSH32M( gsbase+4 );
    PUSH32M( gsbase );
    PUSH32I( mem&0x13ff );
    PUSH32I( GS_RINGTYPE_MEMWRITE64 );

#ifdef _MSC_VER
	MOV32ItoR( ECX, (uptr)mtgsThread );
	CALLFunc( mtgsThread->FnPtr_SimplePacket() );
#else	// GCC -->
	PUSH32I( (uptr)mtgsThread );
	CALLFunc( mtgsThread->FnPtr_SimplePacket() );
	ADD32ItoR( ESP, 20 );
#endif

}

void gsConstWrite8(u32 mem, int mmreg)
{
	switch (mem&~3) {
		case 0x12001000: // GS_CSR
			_eeMoveMMREGtoR(EAX, mmreg);
			iFlushCall(0);
			MOV32MtoR(ECX, (uptr)&CSRw);
			AND32ItoR(EAX, 0xff<<(mem&3)*8);
			AND32ItoR(ECX, ~(0xff<<(mem&3)*8));
			OR32ItoR(EAX, ECX);
			_callFunctionArg1((uptr)gsCSRwrite, EAX|MEM_X86TAG, 0);
			break;
		default:
			_eeWriteConstMem8( (uptr)PS2GS_BASE(mem), mmreg );

			if( mtgsThread != NULL )
				_rec_mtgs_Send32orSmaller( GS_RINGTYPE_MEMWRITE8, mem, mmreg );

			break;
	}
}

void gsConstWrite16(u32 mem, int mmreg)
{	
	switch (mem&~3) {

		case 0x12000010: // GS_SMODE1
		case 0x12000020: // GS_SMODE2
			// SMODE1 and SMODE2 fall back on the gsWrite library.
			iFlushCall(0);
			_callFunctionArg2((uptr)gsWrite16, MEM_CONSTTAG, mmreg, mem, 0 );
			break;

		case 0x12001000: // GS_CSR

			assert( !(mem&2) );
			_eeMoveMMREGtoR(EAX, mmreg);
			iFlushCall(0);

			MOV32MtoR(ECX, (uptr)&CSRw);
			AND32ItoR(EAX, 0xffff<<(mem&2)*8);
			AND32ItoR(ECX, ~(0xffff<<(mem&2)*8));
			OR32ItoR(EAX, ECX);
			_callFunctionArg1((uptr)gsCSRwrite, EAX|MEM_X86TAG, 0);
			break;

		default:
			_eeWriteConstMem16( (uptr)PS2GS_BASE(mem), mmreg );

			if( mtgsThread != NULL )
				_rec_mtgs_Send32orSmaller( GS_RINGTYPE_MEMWRITE16, mem, mmreg );

			break;
	}
}

// (value&0x1f00)|0x6000
void gsConstWriteIMR(int mmreg)
{
	const u32 mem = 0x12001010;
	if( mmreg & MEM_XMMTAG ) {
		SSE2_MOVD_XMM_to_M32((uptr)PS2GS_BASE(mem), mmreg&0xf);
		AND32ItoM((uptr)PS2GS_BASE(mem), 0x1f00);
		OR32ItoM((uptr)PS2GS_BASE(mem), 0x6000);
	}
	else if( mmreg & MEM_MMXTAG ) {
		SetMMXstate();
		MOVDMMXtoM((uptr)PS2GS_BASE(mem), mmreg&0xf);
		AND32ItoM((uptr)PS2GS_BASE(mem), 0x1f00);
		OR32ItoM((uptr)PS2GS_BASE(mem), 0x6000);
	}
	else if( mmreg & MEM_EECONSTTAG ) {
		MOV32ItoM( (uptr)PS2GS_BASE(mem), (g_cpuConstRegs[(mmreg>>16)&0x1f].UL[0]&0x1f00)|0x6000);
	}
	else {
		AND32ItoR(mmreg, 0x1f00);
		OR32ItoR(mmreg, 0x6000);
		MOV32RtoM( (uptr)PS2GS_BASE(mem), mmreg );
	}

	// IMR doesn't need to be updated in MTGS mode
}

void gsConstWrite32(u32 mem, int mmreg) {

	switch (mem) {

		case 0x12000010: // GS_SMODE1
		case 0x12000020: // GS_SMODE2
			// SMODE1 and SMODE2 fall back on the gsWrite library.
			iFlushCall(0);
			_callFunctionArg2((uptr)gsWrite32, MEM_CONSTTAG, mmreg, mem, 0 );
			break;

		case 0x12001000: // GS_CSR
			iFlushCall(0);
            _callFunctionArg1((uptr)gsCSRwrite, mmreg, 0);
			break;

		case 0x12001010: // GS_IMR
			gsConstWriteIMR(mmreg);
			break;
		default:
			_eeWriteConstMem32( (uptr)PS2GS_BASE(mem), mmreg );

			if( mtgsThread != NULL )
				_rec_mtgs_Send32orSmaller( GS_RINGTYPE_MEMWRITE32, mem, mmreg );

			break;
	}
}

void gsConstWrite64(u32 mem, int mmreg)
{
	switch (mem) {
		case 0x12000010: // GS_SMODE1
		case 0x12000020: // GS_SMODE2
			// SMODE1 and SMODE2 fall back on the gsWrite library.
			// the low 32 bit dword is all the SMODE regs care about.
			iFlushCall(0);
			_callFunctionArg2((uptr)gsWrite32, MEM_CONSTTAG, mmreg, mem, 0 );
			break;

		case 0x12001000: // GS_CSR
			iFlushCall(0);
            _callFunctionArg1((uptr)gsCSRwrite, mmreg, 0);
			break;

		case 0x12001010: // GS_IMR
			gsConstWriteIMR(mmreg);
			break;

		default:
			_eeWriteConstMem64((uptr)PS2GS_BASE(mem), mmreg);

			if( mtgsThread != NULL )
			{
				iFlushCall( 0 );
				_rec_mtgs_Send64( (uptr)PS2GS_BASE(mem), mem, mmreg );
			}

			break;
	}
}

void gsConstWrite128(u32 mem, int mmreg)
{
	switch (mem) {
		case 0x12000010: // GS_SMODE1
		case 0x12000020: // GS_SMODE2
			// SMODE1 and SMODE2 fall back on the gsWrite library.
			// the low 32 bit dword is all the SMODE regs care about.
			iFlushCall(0);
			_callFunctionArg2((uptr)gsWrite32, MEM_CONSTTAG, mmreg, mem, 0 );
			break;

		case 0x12001000: // GS_CSR
			iFlushCall(0);
            _callFunctionArg1((uptr)gsCSRwrite, mmreg, 0);
			break;

		case 0x12001010: // GS_IMR
			// (value&0x1f00)|0x6000
			gsConstWriteIMR(mmreg);
			break;

		default:
			_eeWriteConstMem128( (uptr)PS2GS_BASE(mem), mmreg);

			if( mtgsThread != NULL )
			{
				iFlushCall(0);
				_rec_mtgs_Send64( (uptr)PS2GS_BASE(mem), mem, mmreg );
				_rec_mtgs_Send64( (uptr)PS2GS_BASE(mem)+8, mem+8, mmreg );
			}

			break;
	}
}

int gsConstRead8(u32 x86reg, u32 mem, u32 sign)
{
	GIF_LOG("GS read 8 %8.8lx (%8.8x), at %8.8lx\n", (uptr)PS2GS_BASE(mem), mem);
	_eeReadConstMem8(x86reg, (uptr)PS2GS_BASE(mem), sign);
	return 0;
}

int gsConstRead16(u32 x86reg, u32 mem, u32 sign)
{
	GIF_LOG("GS read 16 %8.8lx (%8.8x), at %8.8lx\n", (uptr)PS2GS_BASE(mem), mem);
	_eeReadConstMem16(x86reg, (uptr)PS2GS_BASE(mem), sign);
	return 0;
}

int gsConstRead32(u32 x86reg, u32 mem)
{
	GIF_LOG("GS read 32 %8.8lx (%8.8x), at %8.8lx\n", (uptr)PS2GS_BASE(mem), mem);
	_eeReadConstMem32(x86reg, (uptr)PS2GS_BASE(mem));
	return 0;
}

void gsConstRead64(u32 mem, int mmreg)
{
	GIF_LOG("GS read 64 %8.8lx (%8.8x), at %8.8lx\n", (uptr)PS2GS_BASE(mem), mem);
	if( IS_XMMREG(mmreg) ) SSE_MOVLPS_M64_to_XMM(mmreg&0xff, (uptr)PS2GS_BASE(mem));
	else {
		MOVQMtoR(mmreg, (uptr)PS2GS_BASE(mem));
		SetMMXstate();
	}
}

void gsConstRead128(u32 mem, int xmmreg)
{
	GIF_LOG("GS read 128 %8.8lx (%8.8x), at %8.8lx\n", (uptr)PS2GS_BASE(mem), mem);
	_eeReadConstMem128( xmmreg, (uptr)PS2GS_BASE(mem));
}
