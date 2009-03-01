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
 

#define TINY_BLOCK_COPY 64  
#define IN_CACHE_COPY 2 * 1024 
#define UNCACHED_COPY 4 * 1024 /
#define BLOCK_PREFETCH_COPY  infinity // no limit for movq/movntq w/block prefetch 
#define CACHEBLOCK 80h 

// Fast assembly routines for x86-64
// zerofrog(@gmail.com)
// and added to by arcum42@gmail.com
.intel_syntax
.extern g_EEFreezeRegs
.extern FreezeMMXRegs_
.extern _mmx_backup

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
		pop %esi
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

// void __fastcall memcpy_amd_(void *dest, const void *src, size_t n)
.globl memcpy_amd_
memcpy_amd_:
	push    %edi  
	push    %esi  

	mov		%edi, %ecx		// destination
	mov		%esi, %edx		// source
	mov		%ecx, [%esp+12]	// number of bytes to copy
	mov		%eax, %ecx		// keep a copy of count

	cld
	cmp		%eax, TINY_BLOCK_COPY
	jb		$memcpy_ic_3	// tiny? skip mmx copy

	cmp		%eax, 32*1024		// don't align between 32k-64k because
	jbe		$memcpy_do_align //  it appears to be slower
	cmp		%eax, 64*1024
	jbe		$memcpy_align_done
$memcpy_do_align:
	mov		%eax, 8			// a trick that's faster than rep movsb...
	sub		%eax, %edi		// align destination to qword
	andb	%eax, 111		// get the low bits
	sub		%ecx, %eax		// update copy count
	neg		%eax				// set up to jump into the array
	add		%eax, offset $memcpy_align_done
	jmp		%eax				// jump to array of movsb's

.align 4
	movsb
	movsb
	movsb
	movsb
	movsb
	movsb
	movsb
	movsb

$memcpy_align_done:			// destination is dword aligned
	mov		%eax, %ecx		// number of bytes left to copy
	shr		%eax, 6			// get 64-byte block count
	jz		$memcpy_ic_2	// finish the last few bytes

	cmp		%eax, IN_CACHE_COPY/64	// too big 4 cache? use uncached copy
	jae		$memcpy_uc_test

	movq	[_mmx_backup+0x00],%mm0
	movq	[_mmx_backup+0x08],%mm1
	movq	[_mmx_backup+0x10],%mm2
	movq	[_mmx_backup+0x18],%mm3

// This is small block copy that uses the MMX registers to copy 8 bytes
// at a time.  It uses the "unrolled loop" optimization, and also uses
// the software prefetch instruction to get the data into the cache.
.align 16
$memcpy_ic_1:			// 64-byte block copies, in-cache copy

	prefetchnta [%esi + (200*64/34+192)]		// start reading ahead

	movq	%mm0, [%esi+0]	// read 64 bits
	movq	%mm1, [%esi+8]
	movq	[%edi+0], %mm0	//write 64 bits
	movq	[%edi+8], %mm1	//    note:  the normal movq writes the
	movq	%mm2, [%esi+16]	//    data to cache; a cache line will be
	movq	%mm3, [%esi+24]	//    allocated as needed, to store the data
	movq	[%edi+16], %mm2
	movq	[%edi+24], %mm3
	movq	%mm0, [%esi+32]
	movq	%mm1, [%esi+40]
	movq	[%edi+32], %mm0
	movq	[%edi+40], %mm1
	movq	%mm2, [%esi+48]
	movq	%mm3, [%esi+56]
	movq	[%edi+48], %mm2
	movq	[%edi+56], %mm3

	add		%esi, 64			// update source pointer
	add		%edi, 64			// update destination pointer
	dec		%eax				// count down
	jnz		$memcpy_ic_1	// last 64-byte block?

	movq	%mm0,[_mmx_backup+0x00]
	movq	%mm1,[_mmx_backup+0x08]
	movq	%mm2,[_mmx_backup+0x10]
	movq	%mm3,[_mmx_backup+0x18]

$memcpy_ic_2:
	mov		%eax, %ecx		// has valid low 6 bits of the byte count
$memcpy_ic_3:
	shr		%eax, 2			// dword count
	andb	%eax, 1111		// only look at the "remainder" bits
	neg		%eax				// set up to jump into the array
	add		%eax, offset $memcpy_last_few
	jmp		%eax				// jump to array of movsd's

$memcpy_uc_test:
//	cmp		%ecx, UNCACHED_COPY/64	// big enough? use block prefetch copy
//	jae		$memcpy_bp_1
//$memcpy_64_test:
	or		%eax, %eax		// tail end of block prefetch will jump here
	jz		$memcpy_ic_2	// no more 64-byte blocks left

// For larger blocks, which will spill beyond the cache, it's faster to
// use the Streaming Store instruction MOVNTQ.   This write instruction
// bypasses the cache and writes straight to main memory.  This code also
// uses the software prefetch instruction to pre-read the data.

	movq	[_mmx_backup+0x00],%mm0
	movq	[_mmx_backup+0x08],%mm1
	movq	[_mmx_backup+0x10],%mm2

.align 16
$memcpy_uc_1:				// 64-byte blocks, uncached copy

	prefetchnta [%esi + (200*64/34+192)]		// start reading ahead

	movq	%mm0,[%esi+0]		// read 64 bits
	add		%edi,64			// update destination pointer
	movq	%mm1,[%esi+8]
	add		%esi,64			// update source pointer
	movq	%mm2,[%esi-48]
	movntq	[%edi-64], %mm0	// write 64 bits, bypassing the cache
	movq	%mm0,[%esi-40]	//    note: movntq also prevents the CPU
	movntq	[%edi-56], %mm1	//    from READING the destination address
	movq	%mm1,[%esi-32]	//    into the cache, only to be over-written
	movntq	[%edi-48], %mm2	//    so that also helps performance
	movq	%mm2,[%esi-24]
	movntq	[%edi-40], %mm0
	movq	%mm0,[%esi-16]
	movntq	[%edi-32], %mm1
	movq	%mm1,[%esi-8]
	movntq	[%edi-24], %mm2
	movntq	[%edi-16], %mm0
	dec		%eax
	movntq	[%edi-8], %mm1
	jnz		$memcpy_uc_1	// last 64-byte block?

	movq	%mm0,[_mmx_backup+0x00]
	movq	%mm1,[_mmx_backup+0x08]
	movq	%mm2,[_mmx_backup+0x10]

	jmp		$memcpy_ic_2		// almost done  (not needed because large copy below was removed)

// For the largest size blocks, a special technique called Block Prefetch
// can be used to accelerate the read operations.   Block Prefetch reads
// one address per cache line, for a series of cache lines, in a short loop.
// This is faster than using software prefetch.  The technique is great for
// getting maximum read bandwidth, especially in DDR memory systems.

// Note: Pcsx2 rarely invokes large copies, so this mode has been disabled to
// help keep the code cache footprint of memcpy_fast to a minimum.
// <Code removed here>

// The smallest copy uses the X86 "movsd" instruction, in an optimized
// form which is an "unrolled loop".   Then it handles the last few bytes.
.align 4
	movsd
	movsd			// perform last 1-15 dword copies
	movsd
	movsd
	movsd
	movsd
	movsd
	movsd
	movsd
	movsd			// perform last 1-7 dword copies
	movsd
	movsd
	movsd
	movsd
	movsd
	movsd

$memcpy_last_few:		// dword aligned from before movsd's
	mov		%eax, %ecx	// has valid low 2 bits of the byte count
	andb	%eax, 11	// the last few cows must come home
	jz		$memcpy_final	// no more, let's leave
	rep		movsb		// the last 1, 2, or 3 bytes

$memcpy_final: 
	emms				// clean up the MMX state
	sfence				// flush the write buffer
	//mov		%eax, [dest]	// ret value = destination pointer

	pop    %esi  
	pop    %edi

	ret 4