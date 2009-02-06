##  Copyright (C) 2005-2006 zerofrog(@gmail.com)
#
#  This Program is free software you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation either ve%rsion 2, or (at your option)
#  any later ve%rsion.
#   
#  This Program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
#  GNU General Public License for more details.
#   
#  You should have received a copy of the GNU General Public License
#  along with GNU Make see the file COPYING.  If not, write to
#  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA. 
#  http://www.gnu.org/copyleft/gpl.html
#
#
.intel_syntax

## mmx memcpy implementation, size has to be a multiple of 8
## returns 0 is equal, nonzero value if not equal
## ~10 times faster than standard memcmp
## (zerofrog)
## u8 memcmp_mmx(const void* src1, const void* src2, int cmpsize)
## %rdi - src1
## %rsi - src2
## edx - cmpsize
.globl memcmp_mmx
	.type memcmp_mmx, @function
memcmp_mmx:
        cmp %edx, 32
		jl Done4

		## custom test first 8 to make sure things are ok
		movq %mm0, [%rsi]
		movq %mm1, [%rsi+8]
		pcmpeqd %mm0, [%rdi]
		pcmpeqd %mm1, [%rdi+8]
		pand %mm0, %mm1
		movq %mm2, [%rsi+16]
		pmovmskb %eax, %mm0
		movq %mm3, [%rsi+24]

		// check if eq
		cmp %eax, 0xff
		je NextComp
		mov %eax, 1
		jmp End

NextComp:
		pcmpeqd %mm2, [%rdi+16]
		pcmpeqd %mm3, [%rdi+24]
		pand %mm2, %mm3
		pmovmskb %eax, %mm2

		sub %edx, 32
		add %rsi, 32
		add %rdi, 32

		// check if eq
		cmp %eax, 0xff
		je ContinueTest
		mov %eax, 1
		jmp End

		cmp %edx, 64
		jl Done8

Cmp8:
		movq %mm0, [%rsi]
		movq %mm1, [%rsi+8]
		movq %mm2, [%rsi+16]
		movq %mm3, [%rsi+24]
		movq %mm4, [%rsi+32]
		movq %mm5, [%rsi+40]
		movq %mm6, [%rsi+48]
		movq %mm7, [%rsi+56]
		pcmpeqd %mm0, [%rdi]
		pcmpeqd %mm1, [%rdi+8]
		pcmpeqd %mm2, [%rdi+16]
		pcmpeqd %mm3, [%rdi+24]
		pand %mm0, %mm1
		pcmpeqd %mm4, [%rdi+32]
		pand %mm0, %mm2
		pcmpeqd %mm5, [%rdi+40]
		pand %mm0, %mm3
		pcmpeqd %mm6, [%rdi+48]
		pand %mm0, %mm4
		pcmpeqd %mm7, [%rdi+56]
		pand %mm0, %mm5
		pand %mm0, %mm6
		pand %mm0, %mm7
		pmovmskb %eax, %mm0
		
		// check if eq
		cmp %eax, 0xff
		je Continue
		mov %eax, 1
		jmp End

Continue:
		sub %edx, 64
		add %rsi, 64
		add %rdi, 64
ContinueTest:
		cmp %edx, 64
		jge Cmp8

Done8:
		test %edx, 0x20
		jz Done4
		movq %mm0, [%rsi]
		movq %mm1, [%rsi+8]
		movq %mm2, [%rsi+16]
		movq %mm3, [%rsi+24]
		pcmpeqd %mm0, [%rdi]
		pcmpeqd %mm1, [%rdi+8]
		pcmpeqd %mm2, [%rdi+16]
		pcmpeqd %mm3, [%rdi+24]
		pand %mm0, %mm1
		pand %mm0, %mm2
		pand %mm0, %mm3
		pmovmskb %eax, %mm0
		sub %edx, 32
		add %rsi, 32
		add %rdi, 32

		// check if eq
		cmp %eax, 0xff
		je Done4
		mov %eax, 1
		jmp End

Done4:
		cmp %edx, 24
		jne Done2
		movq %mm0, [%rsi]
		movq %mm1, [%rsi+8]
		movq %mm2, [%rsi+16]
		pcmpeqd %mm0, [%rdi]
		pcmpeqd %mm1, [%rdi+8]
		pcmpeqd %mm2, [%rdi+16]
		pand %mm0, %mm1
		pand %mm0, %mm2
		pmovmskb %eax, %mm0

		// check if eq
		cmp %eax, 0xff
        je Done
		mov %eax, 1
		jmp End

Done2:
		cmp %edx, 16
		jne Done1

		movq %mm0, [%rsi]
		movq %mm1, [%rsi+8]
		pcmpeqd %mm0, [%rdi]
		pcmpeqd %mm1, [%rdi+8]
		pand %mm0, %mm1
		pmovmskb %eax, %mm0

		// check if eq
		cmp %eax, 0xff
        je Done
		mov %eax, 1
		jmp End

Done1:
		cmp %edx, 8
		jne Done

		mov %eax, [%rsi]
		mov %rsi, [%rsi+4]
		cmp %eax, [%rdi]
		je Next
		mov %eax, 1
		jmp End

Next:
		cmp %rsi, [%rdi+4]
        je Done
		mov %eax, 1
		jmp End

Done:
		xor %eax, %eax

End:
		emms
        ret
        
#ifdef ZEROGS_SSE2
// SSE2 extensions

#define punpck(op, sd0, sd2, s1, s3, d1, d3) \
	movdqa %xmm##d1, %xmm##sd0; \
	pshufd %xmm##d3, %xmm##sd2, 0xe4; \
	punpckl##op %xmm##sd0, %xmm##s1; \
	punpckh##op %xmm##d1, %xmm##s1; \
	punpckl##op %xmm##sd2, %xmm##s3; \
	punpckh##op %xmm##d3, %xmm##s3; \

#define punpcknbl \
	movdqa	%xmm4, %xmm0; \
	pshufd	%xmm5, %xmm1, 0xe4; \
        \
	psllq	%xmm1, 4; \
	psrlq	%xmm4, 4; \
         \
	movdqa	%xmm6, %xmm7; \
	pand	%xmm0, %xmm7; \
	pandn	%xmm6, %xmm1; \
	por		%xmm0, %xmm6; \
        \
	movdqa	%xmm6, %xmm7; \
	pand	%xmm4, %xmm7; \
	pandn	%xmm6, %xmm5; \
	por		%xmm4, %xmm6; \
        \
	movdqa	%xmm1, %xmm4; \
        \
	movdqa	%xmm4, %xmm2; \
	pshufd	%xmm5, %xmm3, 0xe4; \
        \
	psllq	%xmm3, 4; \
	psrlq	%xmm4, 4; \
        \
	movdqa	%xmm6, %xmm7; \
	pand	%xmm2, %xmm7; \
	pandn	%xmm6, %xmm3; \
	por		%xmm2, %xmm6; \
        \
	movdqa	%xmm6, %xmm7; \
	pand	%xmm4, %xmm7; \
	pandn	%xmm6, %xmm5; \
	por		%xmm4, %xmm6; \
        \
	movdqa	%xmm3, %xmm4; \
        \
	punpck(bw, 0, 2, 1, 3, 4, 6); \

#define punpcknbh \
	movdqa	%xmm12, %xmm8; \
	pshufd	%xmm13, %xmm9, 0xe4; \
        \
	psllq	%xmm9, 4; \
	psrlq	%xmm12, 4; \
        \
	movdqa	%xmm14, %xmm15; \
	pand	%xmm8, %xmm15; \
	pandn	%xmm14, %xmm9; \
	por		%xmm8, %xmm14; \
        \
	movdqa	%xmm14, %xmm15; \
	pand	%xmm12, %xmm15; \
	pandn	%xmm14, %xmm13; \
	por		%xmm12, %xmm14; \
        \
	movdqa	%xmm9, %xmm12; \
        \
	movdqa	%xmm12, %xmm10; \
	pshufd	%xmm13, %xmm11, 0xe4; \
        \
	psllq	%xmm11, 4; \
	psrlq	%xmm12, 4; \
        \
	movdqa	%xmm14, %xmm15; \
	pand	%xmm10, %xmm15; \
	pandn	%xmm14, %xmm11; \
	por		%xmm10, %xmm14; \
        \
	movdqa	%xmm14, %xmm15; \
	pand	%xmm12, %xmm15; \
	pandn	%xmm14, %xmm13; \
	por		%xmm12, %xmm14; \
        \
	movdqa	%xmm11, %xmm12; \
        \
	punpck(bw, 8, 10, 9, 11, 12, 14); \

//
// SwizzleBlock32_sse2
//

.globl SwizzleBlock32_sse2
        .type SwizzleBlock32_sse2, @function
SwizzleBlock32_sse2:

	mov			%eax, 4

	cmp			%ecx, 0xffffffff
	jne			SwizzleBlock32_sse2_2

	.align 16
SwizzleBlock32_sse2_1:
	movdqa		%xmm0, [%rsi]
	movdqa		%xmm4, [%rsi+16]
	movdqa		%xmm1, [%rsi+%rdx]
	movdqa		%xmm5, [%rsi+%rdx+16]

	punpck(qdq, 0, 4, 1, 5, 2, 6)

	movdqa		[%rdi+16*0], %xmm0
	movdqa		[%rdi+16*1], %xmm2
	movdqa		[%rdi+16*2], %xmm4
	movdqa		[%rdi+16*3], %xmm6

	lea			%rsi, [%rsi+%rdx*2]
	add			%rdi, 64

	dec			%eax
	jnz			SwizzleBlock32_sse2_1

	ret

SwizzleBlock32_sse2_2:

	movd		%xmm7, %rcx
	pshufd		%xmm7, %xmm7, 0
	
	.align 16
SwizzleBlock32_sse2_3:
	movdqa		%xmm0, [%rsi]
	movdqa		%xmm4, [%rsi+16]
	movdqa		%xmm1, [%rsi+%rdx]
	movdqa		%xmm5, [%rsi+%rdx+16]

	punpck(qdq, 0, 4, 1, 5, 2, 6)

	movdqa		%xmm3, %xmm7
	pshufd		%xmm5, %xmm7, 0xe4
	movdqa		%xmm9, %xmm7
	pshufd		%xmm11, %xmm7, 0xe4

	pandn		%xmm3, [%rdi+16*0]
	pand		%xmm0, %xmm7
	por			%xmm0, %xmm3
	movdqa		[%rdi+16*0], %xmm0

	pandn		%xmm5, [%rdi+16*1]
	pand		%xmm2, %xmm7
	por			%xmm2, %xmm5
	movdqa		[%rdi+16*1], %xmm2

	pandn		%xmm9, [%rdi+16*2]
	pand		%xmm4, %xmm7
	por			%xmm4, %xmm9
	movdqa		[%rdi+16*2], %xmm4

	pandn		%xmm11, [%rdi+16*3]
	pand		%xmm6, %xmm7
	por			%xmm6, %xmm11
	movdqa		[%rdi+16*3], %xmm6

	lea			%rsi, [%rsi+%rdx*2]
	add			%rdi, 64

	dec			%eax
	jnz			SwizzleBlock32_sse2_3

	ret

//
// SwizzleBlock16_sse2
//

.globl SwizzleBlock16_sse2
        .type SwizzleBlock16_sse2, @function
SwizzleBlock16_sse2:

	mov			%eax, 4

	.align 16
SwizzleBlock16_sse2_1:
	movdqa		%xmm0, [%rsi]
	movdqa		%xmm1, [%rsi+16]
	movdqa		%xmm2, [%rsi+%rdx]
	movdqa		%xmm3, [%rsi+%rdx+16]

	punpck(wd, 0, 2, 1, 3, 4, 6)
	punpck(qdq, 0, 4, 2, 6, 1, 5)

	movdqa		[%rdi+16*0], %xmm0
	movdqa		[%rdi+16*1], %xmm1
	movdqa		[%rdi+16*2], %xmm4
	movdqa		[%rdi+16*3], %xmm5

	lea			%rsi, [%rsi+%rdx*2]
	add			%rdi, 64

	dec			%eax
	jnz			SwizzleBlock16_sse2_1

	ret

//
// SwizzleBlock8
//

.globl SwizzleBlock8_sse2
        .type SwizzleBlock8_sse2, @function
SwizzleBlock8_sse2:

	mov			%ecx, 2

	.align 16
SwizzleBlock8_sse2_1:
	// col 0, 2

	movdqa		%xmm0, [%rsi]
	movdqa		%xmm2, [%rsi+%rdx]
	lea			%rsi, [%rsi+%rdx*2]

	pshufd		%xmm1, [%rsi], 0xb1
	pshufd		%xmm3, [%rsi+%rdx], 0xb1
	lea			%rsi, [%rsi+%rdx*2]

	punpck(bw, 0, 2, 1, 3, 4, 6)
	punpck(wd, 0, 2, 4, 6, 1, 3)
	punpck(qdq, 0, 1, 2, 3, 4, 5)

	movdqa		[%rdi+16*0], %xmm0
	movdqa		[%rdi+16*1], %xmm4
	movdqa		[%rdi+16*2], %xmm1
	movdqa		[%rdi+16*3], %xmm5

	// col 1, 3

	pshufd		%xmm0, [%rsi], 0xb1
	pshufd		%xmm2, [%rsi+%rdx], 0xb1
	lea			%rsi, [%rsi+%rdx*2]

	movdqa		%xmm1, [%rsi]
	movdqa		%xmm3, [%rsi+%rdx]
	lea			%rsi, [%rsi+%rdx*2]

	punpck(bw, 0, 2, 1, 3, 4, 6)
	punpck(wd, 0, 2, 4, 6, 1, 3)
	punpck(qdq, 0, 1, 2, 3, 4, 5)

	movdqa		[%rdi+16*4], %xmm0
	movdqa		[%rdi+16*5], %xmm4
	movdqa		[%rdi+16*6], %xmm1
	movdqa		[%rdi+16*7], %xmm5

	add			%rdi, 128

	dec			%ecx
	jnz			SwizzleBlock8_sse2_1

	ret

//
// SwizzleBlock4
//

.globl SwizzleBlock4_sse2
        .type SwizzleBlock4_sse2, @function
SwizzleBlock4_sse2:

	mov			%ecx, 2

	mov         %eax, 0x0f0f0f0f
	movd        %xmm7, %eax 
	pshufd      %xmm7, %xmm7, 0

	.align 16
SwizzleBlock4_sse2_1:
	// col 0, 2

	movdqa		%xmm0, [%rsi]
	movdqa		%xmm2, [%rsi+%rdx]
	lea			%rsi, [%rsi+%rdx*2]

	movdqa		%xmm1, [%rsi]
	movdqa		%xmm3, [%rsi+%rdx]
	lea			%rsi, [%rsi+%rdx*2]

	pshuflw		%xmm1, %xmm1, 0xb1
	pshuflw		%xmm3, %xmm3, 0xb1
	pshufhw		%xmm1, %xmm1, 0xb1
	pshufhw		%xmm3, %xmm3, 0xb1

	punpcknbl
	punpck(bw, 0, 2, 4, 6, 1, 3)
	punpck(bw, 0, 2, 1, 3, 4, 6)
	punpck(qdq, 0, 4, 2, 6, 1, 3)

	movdqa		[%rdi+16*0], %xmm0
	movdqa		[%rdi+16*1], %xmm1
	movdqa		[%rdi+16*2], %xmm4
	movdqa		[%rdi+16*3], %xmm3

	// col 1, 3

	movdqa		%xmm0, [%rsi]
	movdqa		%xmm2, [%rsi+%rdx]
	lea			%rsi, [%rsi+%rdx*2]

	movdqa		%xmm1, [%rsi]
	movdqa		%xmm3, [%rsi+%rdx]
	lea			%rsi, [%rsi+%rdx*2]

	pshuflw		%xmm0, %xmm0, 0xb1
	pshuflw		%xmm2, %xmm2, 0xb1
	pshufhw		%xmm0, %xmm0, 0xb1
	pshufhw		%xmm2, %xmm2, 0xb1

	punpcknbl
	punpck(bw, 0, 2, 4, 6, 1, 3)
	punpck(bw, 0, 2, 1, 3, 4, 6)
	punpck(qdq, 0, 4, 2, 6, 1, 3)

	movdqa		[%rdi+16*4], %xmm0
	movdqa		[%rdi+16*5], %xmm1
	movdqa		[%rdi+16*6], %xmm4
	movdqa		[%rdi+16*7], %xmm3

	add			%rdi, 128

	dec			%ecx
	jnz			SwizzleBlock4_sse2_1

	ret

//
// swizzling with unaligned reads
//

//
// SwizzleBlock32u_sse2
//

.globl SwizzleBlock32u_sse2
        .type SwizzleBlock32u_sse2, @function
SwizzleBlock32u_sse2:

	mov			%eax, 4

	cmp			%ecx, 0xffffffff
	jne			SwizzleBlock32u_sse2_2

	.align 16
SwizzleBlock32u_sse2_1:
	movdqu		%xmm0, [%rsi]
	movdqu		%xmm4, [%rsi+16]
	movdqu		%xmm1, [%rsi+%rdx]
	movdqu		%xmm5, [%rsi+%rdx+16]

	punpck(qdq, 0, 4, 1, 5, 2, 6)

	movdqa		[%rdi+16*0], %xmm0
	movdqa		[%rdi+16*1], %xmm2
	movdqa		[%rdi+16*2], %xmm4
	movdqa		[%rdi+16*3], %xmm6

	lea			%rsi, [%rsi+%rdx*2]
	add			%rdi, 64

	dec			%eax
	jnz			SwizzleBlock32u_sse2_1

	ret

SwizzleBlock32u_sse2_2:

	movd		%xmm7, %rcx
	pshufd		%xmm7, %xmm7, 0
	
	.align 16
SwizzleBlock32u_sse2_3:
	movdqu		%xmm0, [%rsi]
	movdqu		%xmm4, [%rsi+16]
	movdqu		%xmm1, [%rsi+%rdx]
	movdqu		%xmm5, [%rsi+%rdx+16]

	punpck(qdq, 0, 4, 1, 5, 2, 6)

	movdqa		%xmm3, %xmm7
	pshufd		%xmm5, %xmm7, 0xe4
	movdqa		%xmm9, %xmm7
	pshufd		%xmm11, %xmm7, 0xe4

	pandn		%xmm3, [%rdi+16*0]
	pand		%xmm0, %xmm7
	por			%xmm0, %xmm3
	movdqa		[%rdi+16*0], %xmm0

	pandn		%xmm5, [%rdi+16*1]
	pand		%xmm2, %xmm7
	por			%xmm2, %xmm5
	movdqa		[%rdi+16*1], %xmm2

	pandn		%xmm9, [%rdi+16*2]
	pand		%xmm4, %xmm7
	por			%xmm4, %xmm9
	movdqa		[%rdi+16*2], %xmm4

	pandn		%xmm11, [%rdi+16*3]
	pand		%xmm6, %xmm7
	por			%xmm6, %xmm11
	movdqa		[%rdi+16*3], %xmm6

	lea			%rsi, [%rsi+%rdx*2]
	add			%rdi, 64

	dec			%eax
    jnz			SwizzleBlock32u_sse2_3

	ret

//
// SwizzleBlock16u_sse2
//

.globl SwizzleBlock16u_sse2
        .type SwizzleBlock16u_sse2, @function
SwizzleBlock16u_sse2:
	mov			%eax, 4

	.align 16
SwizzleBlock16u_sse2_1:
	movdqu		%xmm0, [%rsi]
	movdqu		%xmm1, [%rsi+16]
	movdqu		%xmm2, [%rsi+%rdx]
	movdqu		%xmm3, [%rsi+%rdx+16]

	punpck(wd, 0, 2, 1, 3, 4, 6)
	punpck(qdq, 0, 4, 2, 6, 1, 5)

	movdqa		[%rdi+16*0], %xmm0
	movdqa		[%rdi+16*1], %xmm1
	movdqa		[%rdi+16*2], %xmm4
	movdqa		[%rdi+16*3], %xmm5

	lea			%rsi, [%rsi+%rdx*2]
	add			%rdi, 64

	dec			%eax
	jnz			SwizzleBlock16u_sse2_1

	ret

//
// SwizzleBlock8u
//

.globl SwizzleBlock8u_sse2
        .type SwizzleBlock8u_sse2, @function
SwizzleBlock8u_sse2:
	mov			%ecx, 2

	.align 16
SwizzleBlock8u_sse2_1:
	// col 0, 2

	movdqu		%xmm0, [%rsi]
	movdqu		%xmm2, [%rsi+%rdx]
	lea			%rsi, [%rsi+%rdx*2]

	pshufd		%xmm1, %xmm0, 0xb1
	pshufd		%xmm3, %xmm2, 0xb1
	lea			%rsi, [%rsi+%rdx*2]

	punpck(bw, 0, 2, 1, 3, 4, 6)
	punpck(wd, 0, 2, 4, 6, 1, 3)
	punpck(qdq, 0, 1, 2, 3, 4, 5)

	movdqa		[%rdi+16*0], %xmm0
	movdqa		[%rdi+16*1], %xmm4
	movdqa		[%rdi+16*2], %xmm1
	movdqa		[%rdi+16*3], %xmm5

	// col 1, 3

	movdqu		%xmm0, [%rsi]
	movdqu		%xmm2, [%rsi+%rdx]
	pshufd		%xmm0, %xmm0, 0xb1
	pshufd		%xmm2, %xmm2, 0xb1
	lea			%rsi, [%rsi+%rdx*2]

	movdqu		%xmm1, [%rsi]
	movdqu		%xmm3, [%rsi+%rdx]
	lea			%rsi, [%rsi+%rdx*2]

	punpck(bw, 0, 2, 1, 3, 4, 6)
	punpck(wd, 0, 2, 4, 6, 1, 3)
	punpck(qdq, 0, 1, 2, 3, 4, 5)

	movdqa		[%rdi+16*4], %xmm0
	movdqa		[%rdi+16*5], %xmm4
	movdqa		[%rdi+16*6], %xmm1
	movdqa		[%rdi+16*7], %xmm5

	add			%rdi, 128

	dec			%ecx
	jnz			SwizzleBlock8u_sse2_1

	ret

//
// SwizzleBlock4u
//

.globl SwizzleBlock4u_sse2
        .type SwizzleBlock4u_sse2, @function
SwizzleBlock4u_sse2:

	mov			%ecx, 2

	mov         %eax, 0xf0f0f0f
	movd        %xmm7, %eax 
	pshufd      %xmm7, %xmm7, 0

	.align 16
SwizzleBlock4u_sse2_1:
	// col 0, 2

	movdqu		%xmm0, [%rsi]
	movdqu		%xmm2, [%rsi+%rdx]
	lea			%rsi, [%rsi+%rdx*2]

	movdqu		%xmm1, [%rsi]
	movdqu		%xmm3, [%rsi+%rdx]
	lea			%rsi, [%rsi+%rdx*2]

	pshuflw		%xmm1, %xmm1, 0xb1
	pshuflw		%xmm3, %xmm3, 0xb1
	pshufhw		%xmm1, %xmm1, 0xb1
	pshufhw		%xmm3, %xmm3, 0xb1

	punpcknbl
	punpck(bw, 0, 2, 4, 6, 1, 3)
	punpck(bw, 0, 2, 1, 3, 4, 6)
	punpck(qdq, 0, 4, 2, 6, 1, 3)

	movdqa		[%rdi+16*0], %xmm0
	movdqa		[%rdi+16*1], %xmm1
	movdqa		[%rdi+16*2], %xmm4
	movdqa		[%rdi+16*3], %xmm3

	// col 1, 3

	movdqu		%xmm0, [%rsi]
	movdqu		%xmm2, [%rsi+%rdx]
	lea			%rsi, [%rsi+%rdx*2]

	movdqu		%xmm1, [%rsi]
	movdqu		%xmm3, [%rsi+%rdx]
	lea			%rsi, [%rsi+%rdx*2]

	pshuflw		%xmm0, %xmm0, 0xb1
	pshuflw		%xmm2, %xmm2, 0xb1
	pshufhw		%xmm0, %xmm0, 0xb1
	pshufhw		%xmm2, %xmm2, 0xb1

	punpcknbl
	punpck(bw, 0, 2, 4, 6, 1, 3)
	punpck(bw, 0, 2, 1, 3, 4, 6)
	punpck(qdq, 0, 4, 2, 6, 1, 3)

	movdqa		[%rdi+16*4], %xmm0
	movdqa		[%rdi+16*5], %xmm1
	movdqa		[%rdi+16*6], %xmm4
	movdqa		[%rdi+16*7], %xmm3

	add			%rdi, 128

	dec			%ecx
	jnz			SwizzleBlock4u_sse2_1

	ret


        .align 16
s_clut16mask:
        .long 0xffff0000
        .long 0xffff0000
        .long 0xffff0000
        .long 0xffff0000

        .align 16
s_clut16mask2:

        .long 0x0000ffff
        .long 0x0000ffff
        .long 0x0000ffff
        .long 0x0000ffff

.globl WriteCLUT_T16_I4_CSM1_sse2
        .type WriteCLUT_T16_I4_CSM1_sse2, @function
WriteCLUT_T16_I4_CSM1_sse2:
        movdqa %xmm0, xmmword ptr [%rdi]
		movdqa %xmm1, xmmword ptr [%rdi+16]
		movdqa %xmm2, xmmword ptr [%rdi+32]
		movdqa %xmm3, xmmword ptr [%rdi+48]

		// rearrange
		pshuflw %xmm0, %xmm0, 0x88
		pshufhw %xmm0, %xmm0, 0x88
		pshuflw %xmm1, %xmm1, 0x88
		pshufhw %xmm1, %xmm1, 0x88
		pshuflw %xmm2, %xmm2, 0x88
		pshufhw %xmm2, %xmm2, 0x88
		pshuflw %xmm3, %xmm3, 0x88
		pshufhw %xmm3, %xmm3, 0x88

		shufps %xmm0, %xmm1, 0x88
		shufps %xmm2, %xmm3, 0x88

		pshufd %xmm0, %xmm0, 0xd8
		pshufd %xmm2, %xmm2, 0xd8

		pxor %xmm6, %xmm6

		test %rsi, 15
		jnz WriteUnaligned

		movdqa %xmm7, [%rip+s_clut16mask] // saves upper 16 bits

		// have to save interlaced with the old data
		movdqa %xmm4, [%rsi]
		movdqa %xmm5, [%rsi+32]
		movhlps %xmm1, %xmm0
		movlhps %xmm0, %xmm2 // lower 8 colors

		pand %xmm4, %xmm7
		pand %xmm5, %xmm7

		shufps %xmm1, %xmm2, 0xe4 // upper 8 colors
		movdqa %xmm2, %xmm0
		movdqa %xmm3, %xmm1

		punpcklwd %xmm0, %xmm6
		punpcklwd %xmm1, %xmm6
		por %xmm0, %xmm4
		por %xmm1, %xmm5

		punpckhwd %xmm2, %xmm6
		punpckhwd %xmm3, %xmm6

		movdqa [%rsi], %xmm0
		movdqa [%rsi+32], %xmm1

		movdqa %xmm5, %xmm7
		pand %xmm7, [%rsi+16]
		pand %xmm5, [%rsi+48]

		por %xmm2, %xmm7
		por %xmm3, %xmm5

		movdqa [%rsi+16], %xmm2
		movdqa [%rsi+48], %xmm3
		jmp WriteCLUT_T16_I4_CSM1_End

WriteUnaligned:
		// %rsi is offset by 2
		sub %rsi, 2

		movdqa %xmm7, [%rip+s_clut16mask2] // saves lower 16 bits

		// have to save interlaced with the old data
		movdqa %xmm4, [%rsi]
		movdqa %xmm5, [%rsi+32]
		movhlps %xmm1, %xmm0
		movlhps %xmm0, %xmm2 // lower 8 colors

		pand %xmm4, %xmm7
		pand %xmm5, %xmm7

		shufps %xmm1, %xmm2, 0xe4 // upper 8 colors
		movdqa %xmm2, %xmm0
		movdqa %xmm3, %xmm1

		punpcklwd %xmm0, %xmm6
		punpcklwd %xmm1, %xmm6
		pslld %xmm0, 16
		pslld %xmm1, 16
		por %xmm0, %xmm4
		por %xmm1, %xmm5

		punpckhwd %xmm2, %xmm6
		punpckhwd %xmm3, %xmm6
		pslld %xmm2, 16
		pslld %xmm3, 16

		movdqa [%rsi], %xmm0
		movdqa [%rsi+32], %xmm1

		movdqa %xmm5, %xmm7
		pand %xmm7, [%rsi+16]
		pand %xmm5, [%rsi+48]

		por %xmm2, %xmm7
		por %xmm3, %xmm5

		movdqa [%rsi+16], %xmm2
		movdqa [%rsi+48], %xmm3
WriteCLUT_T16_I4_CSM1_End:
        ret
        	      
#endif
