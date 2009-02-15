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

#include "PsxCommon.h"
#include "iR3000A.h"
#include "VU.h"


extern int g_psxWriteOk;
extern u32 g_psxMaxRecMem;
static u32 writectrl;

#ifdef PCSX2_VIRTUAL_MEM

#ifdef _DEBUG

#define ASSERT_WRITEOK \
{ \
	__asm cmp g_psxWriteOk, 1 \
	__asm je WriteOk \
	__asm int 10 \
} \
WriteOk: \

#else
#define ASSERT_WRITEOK
#endif

__declspec(naked) void psxRecMemRead8()
{
	__asm {
		mov edx, ecx
		shr edx, 16
		cmp dx, 0x1f80
		je hwread
		cmp dx, 0x1f40
		je hw4read
		cmp dx, 0x1000
		je devread
		cmp dx, 0x1f00
		je spuread
	}

	ASSERT_WRITEOK

	__asm {
memread:
		// rom reads, has to be PS2MEM_BASE_
		mov eax, dword ptr [ecx+PS2MEM_BASE_]
		ret

hwread:
		cmp cx, 0x1000
		jb memread

		push ecx
		call psxHwRead8
		add esp, 4
		ret

hw4read:
		push ecx
		call psxHw4Read8
		add esp, 4
		ret

devread:
		push ecx
		call DEV9read8
		// stack already incremented
		ret

spuread:
		push ecx
		call SPU2read
		// stack already incremented
		ret
	}
}

int psxRecMemConstRead8(u32 x86reg, u32 mem, u32 sign)
{
	u32 t = (mem >> 16) & 0x1fff;
	
	switch(t) {
		case 0x1f80:
			return psxHwConstRead8(x86reg, mem&0x1fffffff, sign);

#ifdef _DEBUG
		case 0x1d00: assert(0);
#endif

		case 0x1f40:
			return psxHw4ConstRead8(x86reg, mem&0x1fffffff, sign);

		case 0x1000:
			PUSH32I(mem&0x1fffffff);
			CALLFunc((uptr)DEV9read8);
			if( sign ) MOVSX32R8toR(x86reg, EAX);
			else MOVZX32R8toR(x86reg, EAX);
			return 0;

		default:
			_eeReadConstMem8(x86reg, (u32)PSXM(mem), sign);
			return 0;
	}
}

__declspec(naked) void psxRecMemRead16()
{
	__asm {
		mov edx, ecx
		shr edx, 16
		cmp dx, 0x1f80
		je hwread
		cmp dx, 0x1f90
		je spuread
		cmp dx, 0x1d00
		je sifread
		cmp dx, 0x1000
		je devread
	}

	ASSERT_WRITEOK

	__asm {
memread:
		// rom reads, has to be PS2MEM_BASE_
		mov eax, dword ptr [ecx+PS2MEM_BASE_]
		ret

hwread:
		cmp cx, 0x1000
		jb memread

		push ecx
		call psxHwRead16
		add esp, 4
		ret

sifread:
		mov edx, ecx
		and edx, 0xf0
		cmp dl, 0x60
		je Sif60
		

		mov eax, dword ptr [edx+PS2MEM_BASE_+0x1000f200]

		cmp dl, 0x40
		jne End

		// 0x40
		or eax, 2
		jmp End
Sif60:
		xor eax, eax
		jmp End

spuread:
		push ecx
		call SPU2read
		// stack already incremented

End:
		ret

devread:
		push ecx
		call DEV9read16
		// stack already incremented
		ret
	}
}

int psxRecMemConstRead16(u32 x86reg, u32 mem, u32 sign)
{
	u32 t = (mem >> 16) & 0x1fff;

	switch(t) {
		case 0x1f80: return psxHwConstRead16(x86reg, mem&0x1fffffff, sign);

		case 0x1d00:

			switch(mem & 0xF0)
			{
				case 0x40:
					_eeReadConstMem16(x86reg, (u32)PS2MEM_HW+0xF240, sign);
					OR32ItoR(x86reg, 0x0002);
					break;
				case 0x60:
					XOR32RtoR(x86reg, x86reg);
					break;
				default:
					_eeReadConstMem16(x86reg, (u32)PS2MEM_HW+0xf200+(mem&0xf0), sign);
					break;
			}
			return 0;

		case 0x1f90:
			PUSH32I(mem&0x1fffffff);
			CALLFunc((uptr)SPU2read);
			if( sign ) MOVSX32R16toR(x86reg, EAX);
			else MOVZX32R16toR(x86reg, EAX);
			return 0;

		case 0x1000:
			PUSH32I(mem&0x1fffffff);
			CALLFunc((uptr)DEV9read16);
			if( sign ) MOVSX32R16toR(x86reg, EAX);
			else MOVZX32R16toR(x86reg, EAX);
			return 0;

		default:
			assert( g_psxWriteOk );
			_eeReadConstMem16(x86reg, (u32)PSXM(mem), sign);
			return 0;
	}

	return 0;
}

__declspec(naked) void psxRecMemRead32()
{
	__asm {
		mov edx, ecx
		shr edx, 16
		cmp dx, 0x1f80
		je hwread
		cmp dx, 0x1d00
		je sifread
		cmp dx, 0x1000
		je devread
		cmp ecx, 0x1ffe0130
		je WriteCtrlRead
	}

	ASSERT_WRITEOK

	__asm {
memread:
		// rom reads, has to be PS2MEM_BASE_
		mov eax, dword ptr [ecx+PS2MEM_BASE_]
		ret

hwread:
		cmp cx, 0x1000
		jb memread

		push ecx
		call psxHwRead32
		add esp, 4
		ret

sifread:
		mov edx, ecx
		and edx, 0xf0
		cmp dl, 0x60
		je Sif60
		
		// do the read from ps2 mem
		mov eax, dword ptr [edx+PS2MEM_BASE_+0x1000f200]

		cmp dl, 0x40
		jne End

		// 0x40
		or eax, 0xf0000002
		jmp End
Sif60:
		xor eax, eax
End:
		ret

devread:
		push ecx
		call DEV9read32
		// stack already incremented
		ret

WriteCtrlRead:
		mov eax, writectrl
		ret
	}
}

int psxRecMemConstRead32(u32 x86reg, u32 mem)
{
	u32 t = (mem >> 16) & 0x1fff;

	switch(t) {
		case 0x1f80: return psxHwConstRead32(x86reg, mem&0x1fffffff);

		case 0x1d00:
			switch(mem & 0xF0)
			{
				case 0x40:
					_eeReadConstMem32(x86reg, (u32)PS2MEM_HW+0xF240);
					OR32ItoR(x86reg, 0xf0000002);
					break;
				case 0x60:
					XOR32RtoR(x86reg, x86reg);
					break;
				default:
					_eeReadConstMem32(x86reg, (u32)PS2MEM_HW+0xf200+(mem&0xf0));
					break;
			}
			return 0;

		case 0x1000:
			PUSH32I(mem&0x1fffffff);
			CALLFunc((uptr)DEV9read32);
			return 1;

		default:
			if( mem == 0xfffe0130 )
				MOV32MtoR(x86reg, (uptr)&writectrl);
			else {
				XOR32RtoR(x86reg, x86reg);
				CMP32ItoM((uptr)&g_psxWriteOk, 0);
				CMOVNE32MtoR(x86reg, (u32)PSXM(mem));
			}

			return 0;
	}
}

__declspec(naked) void psxRecMemWrite8()
{
	__asm {
		mov edx, ecx
		shr edx, 16
		cmp dx, 0x1f80
		je hwwrite
		cmp dx, 0x1f40
		je hw4write
		cmp dx, 0x1000
		je devwrite
	}

	ASSERT_WRITEOK

	__asm {
memwrite:
		// rom writes, has to be PS2MEM_BASE_
		mov byte ptr [ecx+PS2MEM_BASE_], al
		ret

hwwrite:
		cmp cx, 0x1000
		jb memwrite

		push eax
		push ecx
		call psxHwWrite8
		add esp, 8
		ret

hw4write:
		push eax
		push ecx
		call psxHw4Write8
		add esp, 8
		ret

devwrite:
		push eax
		push ecx
		call DEV9write8
		// stack alwritey incremented
		ret
	}
}

int psxRecMemConstWrite8(u32 mem, int mmreg)
{
	u32 t = (mem >> 16) & 0x1fff;

	switch(t) {
		case 0x1f80:
			psxHwConstWrite8(mem&0x1fffffff, mmreg);
			return 0;
		case 0x1f40:
			psxHw4ConstWrite8(mem&0x1fffffff, mmreg);
			return 0;

		case 0x1d00:
			assert(0);
			_eeWriteConstMem8((u32)(PS2MEM_HW+0xf200+(mem&0xff)), mmreg);
			return 0;

		case 0x1000:
			_recPushReg(mmreg);
			PUSH32I(mem&0x1fffffff);
			CALLFunc((uptr)DEV9write8);
			return 0;

		default:
			_eeWriteConstMem8((u32)PSXM(mem), mmreg);
			return 1;
	}
}

__declspec(naked) void psxRecMemWrite16()
{
	__asm {
		mov edx, ecx
		shr edx, 16
		cmp dx, 0x1f80
		je hwwrite
		cmp dx, 0x1f90
		je spuwrite
		cmp dx, 0x1d00
		je sifwrite
		cmp dx, 0x1000
		je devwrite
        cmp dx, 0x1600
        je ignorewrite
	}

	ASSERT_WRITEOK

	__asm {
memwrite:
		// rom writes, has to be PS2MEM_BASE_
		mov word ptr [ecx+PS2MEM_BASE_], ax
		ret

hwwrite:
		cmp cx, 0x1000
		jb memwrite

		push eax
		push ecx
		call psxHwWrite16
		add esp, 8
		ret

sifwrite:
		mov edx, ecx
		and edx, 0xf0
		cmp dl, 0x60
		je Sif60
		cmp dl, 0x40
		je Sif40
		
		mov word ptr [edx+PS2MEM_BASE_+0x1000f200], ax
		ret

Sif40:
		mov bx, word ptr [edx+PS2MEM_BASE_+0x1000f200]
		test ax, 0xa0
		jz Sif40_2
		// psHu16(0x1000F240) &= ~0xF000;
		// psHu16(0x1000F240) |= 0x2000;
		and bx, 0x0fff
		or bx, 0x2000
		
Sif40_2:
		// if(psHu16(0x1000F240) & temp) psHu16(0x1000F240) &= ~temp;
		// else psHu16(0x1000F240) |= temp;
		and ax, 0xf0
		test bx, ax
		jz Sif40_3

		not ax
		and bx, ax
		jmp Sif40_4
Sif40_3:
		or bx, ax
Sif40_4:
		mov word ptr [edx+PS2MEM_BASE_+0x1000f200], bx
		ret

Sif60:
		mov word ptr [edx+PS2MEM_BASE_+0x1000f200], 0
		ret

spuwrite:
		push eax
		push ecx
		call SPU2write
		// stack alwritey incremented
		ret

devwrite:
		push eax
		push ecx
		call DEV9write16
		// stack alwritey incremented
		ret

ignorewrite:
        ret
	}
}

int psxRecMemConstWrite16(u32 mem, int mmreg)
{
	u32 t = (mem >> 16) & 0x1fff;
	switch(t) {
		case 0x1600:
			//HACK: DEV9 VM crash fix
			return 0;
		case 0x1f80:
			psxHwConstWrite16(mem&0x1fffffff, mmreg);
			return 0;

		case 0x1d00:
			switch (mem & 0xf0) {
				case 0x10:
					// write to ps2 mem
					_eeWriteConstMem16((u32)(PS2MEM_HW+0xf210), mmreg);
					return 0;
				case 0x40:
				{
					// delete x86reg
					_eeMoveMMREGtoR(EAX, mmreg);

					assert( mmreg != EBX );
					MOV16MtoR(EBX, (u32)PS2MEM_HW+0xf240);
					TEST16ItoR(EAX, 0xa0);
					j8Ptr[0] = JZ8(0);

					AND16ItoR(EBX, 0x0fff);
					OR16ItoR(EBX, 0x2000);

					x86SetJ8(j8Ptr[0]);

					AND16ItoR(EAX, 0xf0);
					TEST16RtoR(EAX, 0xf0);
					j8Ptr[0] = JZ8(0);

					NOT32R(EAX);
					AND16RtoR(EBX, EAX);
					j8Ptr[1] = JMP8(0);

					x86SetJ8(j8Ptr[0]);
					OR16RtoR(EBX, EAX);

					x86SetJ8(j8Ptr[1]);

					MOV16RtoM((u32)PS2MEM_HW+0xf240, EBX);

					return 0;
				}
				case 0x60:
					MOV32ItoM((u32)(PS2MEM_HW+0xf260), 0);
					return 0;
				default:
					assert(0);
			}
			return 0;

		case 0x1f90:
			_recPushReg(mmreg);
			PUSH32I(mem&0x1fffffff);
			CALLFunc((uptr)SPU2write);
			return 0;
			
		case 0x1000:
			_recPushReg(mmreg);
			PUSH32I(mem&0x1fffffff);
			CALLFunc((uptr)DEV9write16);
			return 0;

		default:
			_eeWriteConstMem16((u32)PSXM(mem), mmreg);
			return 1;
	}
}

__declspec(naked) void psxRecMemWrite32()
{
	__asm {
		mov edx, ecx
		shr edx, 16
		cmp dx, 0x1f80
		je hwwrite
		cmp dx, 0x1d00
		je sifwrite
		cmp dx, 0x1000
		je devwrite
		cmp dx, 0x1ffe
		je WriteCtrl
	}

	__asm {
		// rom writes, has to be PS2MEM_BASE_
		test g_psxWriteOk, 1
		jz endwrite

memwrite:
		mov dword ptr [ecx+PS2MEM_BASE_], eax
endwrite:
		ret

hwwrite:
		cmp cx, 0x1000
		jb memwrite

		push eax
		push ecx
		call psxHwWrite32
		add esp, 8
		ret

sifwrite:
		mov edx, ecx
		and edx, 0xf0
		cmp dl, 0x60
		je Sif60
		cmp dl, 0x40
		je Sif40
		cmp dl, 0x30
		je Sif30
		cmp dl, 0x20
		je Sif20
		
		mov dword ptr [edx+PS2MEM_BASE_+0x1000f200], eax
		ret

Sif40:
		mov bx, word ptr [edx+PS2MEM_BASE_+0x1000f200]
		test ax, 0xa0
		jz Sif40_2
		// psHu16(0x1000F240) &= ~0xF000;
		// psHu16(0x1000F240) |= 0x2000;
		and bx, 0x0fff
		or bx, 0x2000
		
Sif40_2:
		// if(psHu16(0x1000F240) & temp) psHu16(0x1000F240) &= ~temp;
		// else psHu16(0x1000F240) |= temp;
		and ax, 0xf0
		test bx, ax
		jz Sif40_3

		not ax
		and bx, ax
		jmp Sif40_4
Sif40_3:
		or bx, ax
Sif40_4:
		mov word ptr [edx+PS2MEM_BASE_+0x1000f200], bx
		ret

Sif30:
		or dword ptr [edx+PS2MEM_BASE_+0x1000f200], eax
		ret
Sif20:
		not eax
		and dword ptr [edx+PS2MEM_BASE_+0x1000f200], eax
		ret
Sif60:
		mov dword ptr [edx+PS2MEM_BASE_+0x1000f200], 0
		ret

devwrite:
		push eax
		push ecx
		call DEV9write32
		// stack alwritey incremented
		ret

WriteCtrl:
		cmp ecx, 0x1ffe0130
		jne End

		mov writectrl, eax

		cmp eax, 0x800
		je SetWriteNotOk
		cmp eax, 0x804
		je SetWriteNotOk
		cmp eax, 0xc00
		je SetWriteNotOk
		cmp eax, 0xc04
		je SetWriteNotOk
		cmp eax, 0xcc0
		je SetWriteNotOk
		cmp eax, 0xcc4
		je SetWriteNotOk
		cmp eax, 0x0c4
		je SetWriteNotOk

		// test ok
		cmp eax, 0x1e988
		je SetWriteOk
		cmp eax, 0x1edd8
		je SetWriteOk

End:
		ret

SetWriteNotOk:
		mov g_psxWriteOk, 0
		ret
SetWriteOk:
		mov g_psxWriteOk, 1
		ret
	}
}

int psxRecMemConstWrite32(u32 mem, int mmreg)
{
	u32 t = (mem >> 16) & 0x1fff;
	switch(t) {
		case 0x1f80:
			psxHwConstWrite32(mem&0x1fffffff, mmreg);
			return 0;

		case 0x1d00:
			switch (mem & 0xf0) {
				case 0x10:
					// write to ps2 mem
					_eeWriteConstMem32((u32)PS2MEM_HW+0xf210, mmreg);
					return 0;
				case 0x20:
					// write to ps2 mem
					// delete x86reg
					if( IS_PSXCONSTREG(mmreg) ) {
						AND32ItoM((u32)PS2MEM_HW+0xf220, ~g_psxConstRegs[(mmreg>>16)&0x1f]);
					}
					else {
						NOT32R(mmreg);
						AND32RtoM((u32)PS2MEM_HW+0xf220, mmreg);
					}
					return 0;
				case 0x30:
					// write to ps2 mem
					_eeWriteConstMem32OP((u32)PS2MEM_HW+0xf230, mmreg, 1);
					return 0;
				case 0x40:
				{
					// delete x86reg
					assert( mmreg != EBX );

					_eeMoveMMREGtoR(EAX, mmreg);

					MOV16MtoR(EBX, (u32)PS2MEM_HW+0xf240);
					TEST16ItoR(EAX, 0xa0);
					j8Ptr[0] = JZ8(0);

					AND16ItoR(EBX, 0x0fff);
					OR16ItoR(EBX, 0x2000);

					x86SetJ8(j8Ptr[0]);

					AND16ItoR(EAX, 0xf0);
					TEST16RtoR(EAX, 0xf0);
					j8Ptr[0] = JZ8(0);

					NOT32R(EAX);
					AND16RtoR(EBX, EAX);
					j8Ptr[1] = JMP8(0);

					x86SetJ8(j8Ptr[0]);
					OR16RtoR(EBX, EAX);

					x86SetJ8(j8Ptr[1]);

					MOV16RtoM((u32)PS2MEM_HW+0xf240, EBX);

					return 0;
				}
				case 0x60:
					MOV32ItoM((u32)(PS2MEM_HW+0xf260), 0);
					return 0;
				default:
					assert(0);
			}
			return 0;

		case 0x1000:
			_recPushReg(mmreg);
			PUSH32I(mem&0x1fffffff);
			CALLFunc((uptr)DEV9write32);
			return 0;

		case 0x1ffe:
			if( mem == 0xfffe0130 ) {
				u8* ptrs[9];

				_eeWriteConstMem32((uptr)&writectrl, mmreg);
		
				if( IS_PSXCONSTREG(mmreg) ) {
					switch (g_psxConstRegs[(mmreg>>16)&0x1f]) {
						case 0x800: case 0x804:
						case 0xc00: case 0xc04:
						case 0xcc0: case 0xcc4:
						case 0x0c4:
							MOV32ItoM((uptr)&g_psxWriteOk, 0);
							break;
						case 0x1e988:
						case 0x1edd8:
							MOV32ItoM((uptr)&g_psxWriteOk, 1);
							break;
						default:
							assert(0);
					}
				}
				else {
					// not ok
					CMP32ItoR(mmreg, 0x800);
					ptrs[0] = JE8(0);
					CMP32ItoR(mmreg, 0x804);
					ptrs[1] = JE8(0);
					CMP32ItoR(mmreg, 0xc00);
					ptrs[2] = JE8(0);
					CMP32ItoR(mmreg, 0xc04);
					ptrs[3] = JE8(0);
					CMP32ItoR(mmreg, 0xcc0);
					ptrs[4] = JE8(0);
					CMP32ItoR(mmreg, 0xcc4);
					ptrs[5] = JE8(0);
					CMP32ItoR(mmreg, 0x0c4);
					ptrs[6] = JE8(0);

					// ok
					CMP32ItoR(mmreg, 0x1e988);
					ptrs[7] = JE8(0);
					CMP32ItoR(mmreg, 0x1edd8);
					ptrs[8] = JE8(0);

					x86SetJ8(ptrs[0]);
					x86SetJ8(ptrs[1]);
					x86SetJ8(ptrs[2]);
					x86SetJ8(ptrs[3]);
					x86SetJ8(ptrs[4]);
					x86SetJ8(ptrs[5]);
					x86SetJ8(ptrs[6]);
					MOV32ItoM((uptr)&g_psxWriteOk, 0);
					ptrs[0] = JMP8(0);

					x86SetJ8(ptrs[7]);
					x86SetJ8(ptrs[8]);
					MOV32ItoM((uptr)&g_psxWriteOk, 1);

					x86SetJ8(ptrs[0]);
				}
			}
			return 0;

		default:
			TEST8ItoM((uptr)&g_psxWriteOk, 1);
			j8Ptr[0] = JZ8(0);
			_eeWriteConstMem32((u32)PSXM(mem), mmreg);
			x86SetJ8(j8Ptr[0]);
			return 1;
	}
}

#endif
