/*  Pcsx2 - Pc Ps2 Emulator
 *  Copyright (C) 2002-2005  Pcsx2 Team
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

// Fast assembly routines for x86-64
// zerofrog(@gmail.com)
.intel_syntax
.extern g_EEFreezeRegs
.extern FreezeMMXRegs_

// mmx memcmp implementation, size has to be a multiple of 8
// returns 0 is equal, nonzero value if not equal
// ~10 times faster than standard memcmp
// (zerofrog)
// u8 memcmp_mmx(const void* src1, const void* src2, int cmpsize)
#define MEMCMP_SRC1 %edx
#define MEMCMP_SRC2 %esi
#define MEMCMP_SIZE %ecx

.globl memcmp_mmx
memcmp_mmx:
		// make sure mmx regs are stored
		// FreezeMMXRegs(1);
		cmp dword ptr [g_EEFreezeRegs], 0
		je memcmp_mmx_begin
		push 1
		call FreezeMMXRegs_
		add %esp, 4

memcmp_mmx_begin:		
		push %esi
		mov MEMCMP_SRC1, dword ptr [%esp+8]
		mov MEMCMP_SRC2, dword ptr [%esp+12]
		mov MEMCMP_SIZE, dword ptr [%esp+16]

        cmp MEMCMP_SIZE, 32
		jl memcmp_Done4

		// custom test first 8 to make sure things are ok
		movq %mm0, [MEMCMP_SRC2]
		movq %mm1, [MEMCMP_SRC2+8]
		pcmpeqd %mm0, [MEMCMP_SRC1]
		pcmpeqd %mm1, [MEMCMP_SRC1+8]
		pand %mm0, %mm1
		movq %mm2, [MEMCMP_SRC2+16]
		pmovmskb %eax, %mm0
		movq %mm3, [MEMCMP_SRC2+24]

		// check if eq
		cmp %eax, 0xff
		je memcmp_NextComp
		mov %eax, 1
		jmp memcmp_End

memcmp_NextComp:
		pcmpeqd %mm2, [MEMCMP_SRC1+16]
		pcmpeqd %mm3, [MEMCMP_SRC1+24]
		pand %mm2, %mm3
		pmovmskb %eax, %mm2

		sub MEMCMP_SIZE, 32
		add MEMCMP_SRC2, 32
		add MEMCMP_SRC1, 32

		// check if eq
		cmp %eax, 0xff
		je memcmp_ContinueTest
		mov %eax, 1
		jmp memcmp_End

		cmp MEMCMP_SIZE, 64
		jl memcmp_Done8

memcmp_Cmp8:
		movq %mm0, [MEMCMP_SRC2]
		movq %mm1, [MEMCMP_SRC2+8]
		movq %mm2, [MEMCMP_SRC2+16]
		movq %mm3, [MEMCMP_SRC2+24]
		movq %mm4, [MEMCMP_SRC2+32]
		movq %mm5, [MEMCMP_SRC2+40]
		movq %mm6, [MEMCMP_SRC2+48]
		movq %mm7, [MEMCMP_SRC2+56]
		pcmpeqd %mm0, [MEMCMP_SRC1]
		pcmpeqd %mm1, [MEMCMP_SRC1+8]
		pcmpeqd %mm2, [MEMCMP_SRC1+16]
		pcmpeqd %mm3, [MEMCMP_SRC1+24]
		pand %mm0, %mm1
		pcmpeqd %mm4, [MEMCMP_SRC1+32]
		pand %mm0, %mm2
		pcmpeqd %mm5, [MEMCMP_SRC1+40]
		pand %mm0, %mm3
		pcmpeqd %mm6, [MEMCMP_SRC1+48]
		pand %mm0, %mm4
		pcmpeqd %mm7, [MEMCMP_SRC1+56]
		pand %mm0, %mm5
		pand %mm0, %mm6
		pand %mm0, %mm7
		pmovmskb %eax, %mm0
		
		// check if eq
		cmp %eax, 0xff
		je memcmp_Continue
		mov %eax, 1
		jmp memcmp_End

memcmp_Continue:
		sub MEMCMP_SIZE, 64
		add MEMCMP_SRC2, 64
		add MEMCMP_SRC1, 64
memcmp_ContinueTest:
		cmp MEMCMP_SIZE, 64
		jge memcmp_Cmp8

memcmp_Done8:
		test MEMCMP_SIZE, 0x20
		jz memcmp_Done4
		movq %mm0, [MEMCMP_SRC2]
		movq %mm1, [MEMCMP_SRC2+8]
		movq %mm2, [MEMCMP_SRC2+16]
		movq %mm3, [MEMCMP_SRC2+24]
		pcmpeqd %mm0, [MEMCMP_SRC1]
		pcmpeqd %mm1, [MEMCMP_SRC1+8]
		pcmpeqd %mm2, [MEMCMP_SRC1+16]
		pcmpeqd %mm3, [MEMCMP_SRC1+24]
		pand %mm0, %mm1
		pand %mm0, %mm2
		pand %mm0, %mm3
		pmovmskb %eax, %mm0
		sub MEMCMP_SIZE, 32
		add MEMCMP_SRC2, 32
		add MEMCMP_SRC1, 32

		// check if eq
		cmp %eax, 0xff
		je memcmp_Done4
		mov %eax, 1
		jmp memcmp_End

memcmp_Done4:
		cmp MEMCMP_SIZE, 24
		jne memcmp_Done2
		movq %mm0, [MEMCMP_SRC2]
		movq %mm1, [MEMCMP_SRC2+8]
		movq %mm2, [MEMCMP_SRC2+16]
		pcmpeqd %mm0, [MEMCMP_SRC1]
		pcmpeqd %mm1, [MEMCMP_SRC1+8]
		pcmpeqd %mm2, [MEMCMP_SRC1+16]
		pand %mm0, %mm1
		pand %mm0, %mm2
		pmovmskb %eax, %mm0

		// check if eq
		cmp %eax, 0xff
        je memcmp_Done
		mov %eax, 1
		jmp memcmp_End

memcmp_Done2:
		cmp MEMCMP_SIZE, 16
		jne memcmp_Done1

		movq %mm0, [MEMCMP_SRC2]
		movq %mm1, [MEMCMP_SRC2+8]
		pcmpeqd %mm0, [MEMCMP_SRC1]
		pcmpeqd %mm1, [MEMCMP_SRC1+8]
		pand %mm0, %mm1
		pmovmskb %eax, %mm0

		// check if eq
		cmp %eax, 0xff
        je memcmp_Done
		mov %eax, 1
		jmp memcmp_End

memcmp_Done1:
		cmp MEMCMP_SIZE, 8
		jne memcmp_Done

		mov %eax, [MEMCMP_SRC2]
		mov MEMCMP_SRC2, [MEMCMP_SRC2+4]
		cmp %eax, [MEMCMP_SRC1]
		je memcmp_Next
		mov %eax, 1
		jmp memcmp_End

memcmp_Next:
		cmp MEMCMP_SRC2, [MEMCMP_SRC1+4]
        je memcmp_Done
		mov %eax, 1
		jmp memcmp_End

memcmp_Done:
		xor %eax, %eax

memcmp_End:
		emms
#ifndef __x86_64__
		pop %esi
#endif
		ret
        
// memxor_mmx
#define MEMXOR_SRC1 %edx
#define MEMXOR_SRC2 %esi
#define MEMXOR_SIZE %ecx

.globl memxor_mmx
memxor_mmx:
		// make sure mmx regs are stored
		// FreezeMMXRegs(1);
		cmp dword ptr [g_EEFreezeRegs], 0
		je memxor_mmx_begin
		push 1
		call FreezeMMXRegs_
		add %esp, 4

memxor_mmx_begin:
		push %esi
		mov MEMXOR_SRC1, dword ptr [%esp+8]
		mov MEMXOR_SRC2, dword ptr [%esp+12]
		mov MEMXOR_SIZE, dword ptr [%esp+16]
		cmp MEMXOR_SIZE, 64
	jl memxor_Setup4

	movq %mm0, [MEMXOR_SRC2]
	movq %mm1, [MEMXOR_SRC2+8]
	movq %mm2, [MEMXOR_SRC2+16]
	movq %mm3, [MEMXOR_SRC2+24]
	movq %mm4, [MEMXOR_SRC2+32]
	movq %mm5, [MEMXOR_SRC2+40]
	movq %mm6, [MEMXOR_SRC2+48]
	movq %mm7, [MEMXOR_SRC2+56]
	sub MEMXOR_SIZE, 64
	add MEMXOR_SRC2, 64
	cmp MEMXOR_SIZE, 64
	jl memxor_End8

memxor_Cmp8:
	pxor %mm0, [MEMXOR_SRC2]
	pxor %mm1, [MEMXOR_SRC2+8]
	pxor %mm2, [MEMXOR_SRC2+16]
	pxor %mm3, [MEMXOR_SRC2+24]
	pxor %mm4, [MEMXOR_SRC2+32]
	pxor %mm5, [MEMXOR_SRC2+40]
	pxor %mm6, [MEMXOR_SRC2+48]
	pxor %mm7, [MEMXOR_SRC2+56]

	sub MEMXOR_SIZE, 64
	add MEMXOR_SRC2, 64
	cmp MEMXOR_SIZE, 64
	jge memxor_Cmp8

memxor_End8:
	pxor %mm0, %mm4
	pxor %mm1, %mm5
	pxor %mm2, %mm6
	pxor %mm3, %mm7

	cmp MEMXOR_SIZE, 32
	jl memxor_End4
	pxor %mm0, [MEMXOR_SRC2]
	pxor %mm1, [MEMXOR_SRC2+8]
	pxor %mm2, [MEMXOR_SRC2+16]
	pxor %mm3, [MEMXOR_SRC2+24]
	sub MEMXOR_SIZE, 32
	add MEMXOR_SRC2, 32
	jmp memxor_End4

memxor_Setup4:
	cmp MEMXOR_SIZE, 32
	jl memxor_Setup2

	movq %mm0, [MEMXOR_SRC2]
	movq %mm1, [MEMXOR_SRC2+8]
	movq %mm2, [MEMXOR_SRC2+16]
	movq %mm3, [MEMXOR_SRC2+24]
	sub MEMXOR_SIZE, 32
	add MEMXOR_SRC2, 32

memxor_End4:
	pxor %mm0, %mm2
	pxor %mm1, %mm3

	cmp MEMXOR_SIZE, 16
	jl memxor_End2
	pxor %mm0, [MEMXOR_SRC2]
	pxor %mm1, [MEMXOR_SRC2+8]
	sub MEMXOR_SIZE, 16
	add MEMXOR_SRC2, 16
	jmp memxor_End2

memxor_Setup2:
	cmp MEMXOR_SIZE, 16
	jl memxor_Setup1

	movq %mm0, [MEMXOR_SRC2]
	movq %mm1, [MEMXOR_SRC2+8]
	sub MEMXOR_SIZE, 16
	add MEMXOR_SRC2, 16

memxor_End2:
	pxor %mm0, %mm1

	cmp MEMXOR_SIZE, 8
	jl memxor_End1
	pxor %mm0, [MEMXOR_SRC2]
memxor_End1:
	movq [MEMXOR_SRC1], %mm0
	jmp memxor_End

memxor_Setup1:
	movq %mm0, [MEMXOR_SRC2]
	movq [MEMXOR_SRC1], %mm0
memxor_End:
	emms
	pop %esi
	ret
