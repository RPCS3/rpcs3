# Copyright (C) 2005-2006 zerofrog(@gmail.com)
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
#u8 memcmp_mmx(const void* src1, const void* src2, int cmpsize)
.globl memcmp_mmx
	.type memcmp_mmx, @function
memcmp_mmx:
        push %esi
        mov %ecx, dword ptr [%esp+16]
		mov %edx, dword ptr [%esp+8]
		mov %esi, dword ptr [%esp+12]

		cmp %ecx, 32
		jl Done4

		// custom test first 8 to make sure things are ok
		movq %mm0, [%esi]
		movq %mm1, [%esi+8]
		pcmpeqd %mm0, [%edx]
		pcmpeqd %mm1, [%edx+8]
		pand %mm0, %mm1
		movq %mm2, [%esi+16]
		pmovmskb %eax, %mm0
		movq %mm3, [%esi+24]

		// check if eq
		cmp %eax, 0xff
		je NextComp
		mov %eax, 1
		jmp End

NextComp:
		pcmpeqd %mm2, [%edx+16]
		pcmpeqd %mm3, [%edx+24]
		pand %mm2, %mm3
		pmovmskb %eax, %mm2

		sub %ecx, 32
		add %esi, 32
		add %edx, 32

		// check if eq
		cmp %eax, 0xff
		je ContinueTest
		mov %eax, 1
		jmp End

		cmp %ecx, 64
		jl Done8

Cmp8:
		movq %mm0, [%esi]
		movq %mm1, [%esi+8]
		movq %mm2, [%esi+16]
		movq %mm3, [%esi+24]
		movq %mm4, [%esi+32]
		movq %mm5, [%esi+40]
		movq %mm6, [%esi+48]
		movq %mm7, [%esi+56]
		pcmpeqd %mm0, [%edx]
		pcmpeqd %mm1, [%edx+8]
		pcmpeqd %mm2, [%edx+16]
		pcmpeqd %mm3, [%edx+24]
		pand %mm0, %mm1
		pcmpeqd %mm4, [%edx+32]
		pand %mm0, %mm2
		pcmpeqd %mm5, [%edx+40]
		pand %mm0, %mm3
		pcmpeqd %mm6, [%edx+48]
		pand %mm0, %mm4
		pcmpeqd %mm7, [%edx+56]
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
		sub %ecx, 64
		add %esi, 64
		add %edx, 64
ContinueTest:
		cmp %ecx, 64
		jge Cmp8

Done8:
		test %ecx, 0x20
		jz Done4
		movq %mm0, [%esi]
		movq %mm1, [%esi+8]
		movq %mm2, [%esi+16]
		movq %mm3, [%esi+24]
		pcmpeqd %mm0, [%edx]
		pcmpeqd %mm1, [%edx+8]
		pcmpeqd %mm2, [%edx+16]
		pcmpeqd %mm3, [%edx+24]
		pand %mm0, %mm1
		pand %mm0, %mm2
		pand %mm0, %mm3
		pmovmskb %eax, %mm0
		sub %ecx, 32
		add %esi, 32
		add %edx, 32

		// check if eq
		cmp %eax, 0xff
		je Done4
		mov %eax, 1
		jmp End

Done4:
		cmp %ecx, 24
		jne Done2
		movq %mm0, [%esi]
		movq %mm1, [%esi+8]
		movq %mm2, [%esi+16]
		pcmpeqd %mm0, [%edx]
		pcmpeqd %mm1, [%edx+8]
		pcmpeqd %mm2, [%edx+16]
		pand %mm0, %mm1
		pand %mm0, %mm2
		pmovmskb %eax, %mm0

		// check if eq
		cmp %eax, 0xff
		setne %al
		jmp End

Done2:
		cmp %ecx, 16
		jne Done1

		movq %mm0, [%esi]
		movq %mm1, [%esi+8]
		pcmpeqd %mm0, [%edx]
		pcmpeqd %mm1, [%edx+8]
		pand %mm0, %mm1
		pmovmskb %eax, %mm0

		// check if eq
		cmp %eax, 0xff
		setne %al
		jmp End

Done1:
		cmp %ecx, 8
		jne Done

		mov %eax, [%esi]
		mov %esi, [%esi+4]
		cmp %eax, [%edx]
		je Next
		mov %eax, 1
		jmp End

Next:
		cmp %esi, [%edx+4]
		setne %al
		jmp End

Done:
		xor %eax, %eax

End:
		pop %esi
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

	
#define punpcknb \
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


//
// swizzling
//

//
// SwizzleBlock32
//

.globl SwizzleBlock32_sse2
        .type SwizzleBlock32_sse2, @function
SwizzleBlock32_sse2:

	push		%esi
	push		%edi

	mov			%edi, %ecx
	mov			%esi, %edx
	mov			%edx, [%esp+4+8]
	mov			%ecx, 4

	mov			%eax, [%esp+8+8]
	cmp			%eax, 0xffffffff
	jne			SwizzleBlock32_sse2_2

	.align 16
SwizzleBlock32_sse2_1:
	movdqa		%xmm0, [%esi]
	movdqa		%xmm4, [%esi+16]
	movdqa		%xmm1, [%esi+%edx]
	movdqa		%xmm5, [%esi+%edx+16]

	punpck(qdq, 0, 4, 1, 5, 2, 6)

	movntps		[%edi+16*0], %xmm0
	movntps		[%edi+16*1], %xmm2
	movntps		[%edi+16*2], %xmm4
	movntps		[%edi+16*3], %xmm6

	lea			%esi, [%esi+%edx*2]
	add			%edi, 64

	dec			%ecx
	jnz			SwizzleBlock32_sse2_1

	pop			%edi
	pop			%esi

	ret			8

SwizzleBlock32_sse2_2:

	movd		%xmm7, %eax
	pshufd		%xmm7, %xmm7, 0
	
	.align 16
SwizzleBlock32_sse2_3:
	movdqa		%xmm0, [%esi]
	movdqa		%xmm4, [%esi+16]
	movdqa		%xmm1, [%esi+%edx]
	movdqa		%xmm5, [%esi+%edx+16]

	punpck(qdq, 0, 4, 1, 5, 2, 6)

	movdqa		%xmm3, %xmm7
	pshufd		%xmm5, %xmm7, 0xe4

	pandn		%xmm3, [%edi+16*0]
	pand		%xmm0, %xmm7
	por			%xmm0, %xmm3
	movntps		[%edi+16*0], %xmm0

	pandn		%xmm5, [%edi+16*1]
	pand		%xmm2, %xmm7
	por			%xmm2, %xmm5
	movntps		[%edi+16*1], %xmm2

	movdqa		%xmm3, %xmm7
	pshufd		%xmm5, %xmm7, 0xe4

	pandn		%xmm3, [%edi+16*2]
	pand		%xmm4, %xmm7
	por			%xmm4, %xmm3
	movntps		[%edi+16*2], %xmm4

	pandn		%xmm5, [%edi+16*3]
	pand		%xmm6, %xmm7
	por			%xmm6, %xmm5
	movntps		[%edi+16*3], %xmm6

	lea			%esi, [%esi+%edx*2]
	add			%edi, 64

	dec			%ecx
	jnz			SwizzleBlock32_sse2_3

	pop			%edi
	pop			%esi

	ret			8

//
// SwizzleBlock16
//

.globl SwizzleBlock16_sse2
        .type SwizzleBlock16_sse2, @function
SwizzleBlock16_sse2:

	push		%ebx

	mov			%ebx, [%esp+4+4]
	mov			%eax, 4

	.align 16
SwizzleBlock16_sse2_1:
	movdqa		%xmm0, [%edx]
	movdqa		%xmm1, [%edx+16]
	movdqa		%xmm2, [%edx+%ebx]
	movdqa		%xmm3, [%edx+%ebx+16]

	punpck(wd, 0, 2, 1, 3, 4, 6)
	punpck(qdq, 0, 4, 2, 6, 1, 5)

	movntps		[%ecx+16*0], %xmm0
	movntps		[%ecx+16*1], %xmm1
	movntps		[%ecx+16*2], %xmm4
	movntps		[%ecx+16*3], %xmm5

	lea			%edx, [%edx+%ebx*2]
	add			%ecx, 64

	dec			%eax
	jnz			SwizzleBlock16_sse2_1

	pop			%ebx

	ret			4

//
// SwizzleBlock8
//

.globl SwizzleBlock8_sse2
        .type SwizzleBlock8_sse2, @function
SwizzleBlock8_sse2:

	push		%ebx

	mov			%ebx, [%esp+4+4]
	mov			%eax, 2

	.align 16
SwizzleBlock8_sse2_1:
	// col 0, 2

	movdqa		%xmm0, [%edx]
	movdqa		%xmm2, [%edx+%ebx]
	lea			%edx, [%edx+%ebx*2]

	pshufd		%xmm1, [%edx], 0xb1
	pshufd		%xmm3, [%edx+%ebx], 0xb1
	lea			%edx, [%edx+%ebx*2]

	punpck(bw, 0, 2, 1, 3, 4, 6)
	punpck(wd, 0, 2, 4, 6, 1, 3)
	punpck(qdq, 0, 1, 2, 3, 4, 5)

	movntps		[%ecx+16*0], %xmm0
	movntps		[%ecx+16*1], %xmm4
	movntps		[%ecx+16*2], %xmm1
	movntps		[%ecx+16*3], %xmm5

	// col 1, 3

	pshufd		%xmm0, [%edx], 0xb1
	pshufd		%xmm2, [%edx+%ebx], 0xb1
	lea			%edx, [%edx+%ebx*2]

	movdqa		%xmm1, [%edx]
	movdqa		%xmm3, [%edx+%ebx]
	lea			%edx, [%edx+%ebx*2]

	punpck(bw, 0, 2, 1, 3, 4, 6)
	punpck(wd, 0, 2, 4, 6, 1, 3)
	punpck(qdq, 0, 1, 2, 3, 4, 5)

	movntps		[%ecx+16*4], %xmm0
	movntps		[%ecx+16*5], %xmm4
	movntps		[%ecx+16*6], %xmm1
	movntps		[%ecx+16*7], %xmm5

	add			%ecx, 128

	dec			%eax
	jnz			SwizzleBlock8_sse2_1

	pop			%ebx

	ret			4

//
// SwizzleBlock4
//

.globl SwizzleBlock4_sse2
        .type SwizzleBlock4_sse2, @function
SwizzleBlock4_sse2:

	push		%ebx
	
	mov         %eax, 0xf0f0f0f
	movd        %xmm7, %eax 
	pshufd      %xmm7, %xmm7, 0

	mov			%ebx, [%esp+4+4]
	mov			%eax, 2

	.align 16
SwizzleBlock4_sse2_1:
	// col 0, 2

	movdqa		%xmm0, [%edx]
	movdqa		%xmm2, [%edx+%ebx]
	lea			%edx, [%edx+%ebx*2]

	movdqa		%xmm1, [%edx]
	movdqa		%xmm3, [%edx+%ebx]
	lea			%edx, [%edx+%ebx*2]

	pshuflw		%xmm1, %xmm1, 0xb1
	pshuflw		%xmm3, %xmm3, 0xb1
	pshufhw		%xmm1, %xmm1, 0xb1
	pshufhw		%xmm3, %xmm3, 0xb1

	punpcknb
	punpck(bw, 0, 2, 4, 6, 1, 3)
	punpck(bw, 0, 2, 1, 3, 4, 6)
	punpck(qdq, 0, 4, 2, 6, 1, 3)

	movntps		[%ecx+16*0], %xmm0
	movntps		[%ecx+16*1], %xmm1
	movntps		[%ecx+16*2], %xmm4
	movntps		[%ecx+16*3], %xmm3

	// col 1, 3

	movdqa		%xmm0, [%edx]
	movdqa		%xmm2, [%edx+%ebx]
	lea			%edx, [%edx+%ebx*2]

	movdqa		%xmm1, [%edx]
	movdqa		%xmm3, [%edx+%ebx]
	lea			%edx, [%edx+%ebx*2]

	pshuflw		%xmm0, %xmm0, 0xb1
	pshuflw		%xmm2, %xmm2, 0xb1
	pshufhw		%xmm0, %xmm0, 0xb1
	pshufhw		%xmm2, %xmm2, 0xb1

	punpcknb
	punpck(bw, 0, 2, 4, 6, 1, 3)
	punpck(bw, 0, 2, 1, 3, 4, 6)
	punpck(qdq, 0, 4, 2, 6, 1, 3)

	movntps		[%ecx+16*4], %xmm0
	movntps		[%ecx+16*5], %xmm1
	movntps		[%ecx+16*6], %xmm4
	movntps		[%ecx+16*7], %xmm3

	add			%ecx, 128

	dec			%eax
	jnz			SwizzleBlock4_sse2_1

	pop			%ebx

	ret			4

//
// swizzling with unaligned reads
//

//
// SwizzleBlock32u
//

.globl SwizzleBlock32u_sse2
        .type SwizzleBlock32u_sse2, @function
SwizzleBlock32u_sse2:

	push		%esi
	push		%edi

	mov			%edi, %ecx
	mov			%esi, %edx
	mov			%edx, [%esp+4+8]
	mov			%ecx, 4

	mov			%eax, [%esp+8+8]
	cmp			%eax, 0xffffffff
	jne			SwizzleBlock32u_sse2_2

	.align 16
SwizzleBlock32u_sse2_1:
	movdqu		%xmm0, [%esi]
	movdqu		%xmm4, [%esi+16]
	movdqu		%xmm1, [%esi+%edx]
	movdqu		%xmm5, [%esi+%edx+16]

	punpck(qdq, 0, 4, 1, 5, 2, 6)

	movntps		[%edi+16*0], %xmm0
	movntps		[%edi+16*1], %xmm2
	movntps		[%edi+16*2], %xmm4
	movntps		[%edi+16*3], %xmm6

	lea			%esi, [%esi+%edx*2]
	add			%edi, 64

	dec			%ecx
	jnz			SwizzleBlock32u_sse2_1

	pop			%edi
	pop			%esi

	ret			8

SwizzleBlock32u_sse2_2:

	movd		%xmm7, %eax
	pshufd		%xmm7, %xmm7, 0
	
	.align 16
SwizzleBlock32u_sse2_3:
	movdqu		%xmm0, [%esi]
	movdqu		%xmm4, [%esi+16]
	movdqu		%xmm1, [%esi+%edx]
	movdqu		%xmm5, [%esi+%edx+16]

	punpck(qdq, 0, 4, 1, 5, 2, 6)

	movdqa		%xmm3, %xmm7
	pshufd		%xmm5, %xmm7, 0xe4

	pandn		%xmm3, [%edi+16*0]
	pand		%xmm0, %xmm7
	por			%xmm0, %xmm3
	movdqa		[%edi+16*0], %xmm0

	pandn		%xmm5, [%edi+16*1]
	pand		%xmm2, %xmm7
	por			%xmm2, %xmm5
	movdqa		[%edi+16*1], %xmm2

	movdqa		%xmm3, %xmm7
	pshufd		%xmm5, %xmm7, 0xe4

	pandn		%xmm3, [%edi+16*2]
	pand		%xmm4, %xmm7
	por			%xmm4, %xmm3
	movdqa		[%edi+16*2], %xmm4

	pandn		%xmm5, [%edi+16*3]
	pand		%xmm6, %xmm7
	por			%xmm6, %xmm5
	movdqa		[%edi+16*3], %xmm6

	lea			%esi, [%esi+%edx*2]
	add			%edi, 64

	dec			%ecx
	jnz			SwizzleBlock32u_sse2_3

	pop			%edi
	pop			%esi

	ret			8

//
// SwizzleBlock16u
//

.globl SwizzleBlock16u_sse2
        .type SwizzleBlock16u_sse2, @function
SwizzleBlock16u_sse2:

	push		%ebx

	mov			%ebx, [%esp+4+4]
	mov			%eax, 4

	.align 16
SwizzleBlock16u_sse2_1:
	movdqu		%xmm0, [%edx]
	movdqu		%xmm1, [%edx+16]
	movdqu		%xmm2, [%edx+%ebx]
	movdqu		%xmm3, [%edx+%ebx+16]

	punpck(wd, 0, 2, 1, 3, 4, 6)
	punpck(qdq, 0, 4, 2, 6, 1, 5)

	movntps		[%ecx+16*0], %xmm0
	movntps		[%ecx+16*1], %xmm1
	movntps		[%ecx+16*2], %xmm4
	movntps		[%ecx+16*3], %xmm5

	lea			%edx, [%edx+%ebx*2]
	add			%ecx, 64

	dec			%eax
	jnz			SwizzleBlock16u_sse2_1
        
	pop			%ebx

	ret			4

//
// SwizzleBlock8u
//

.globl SwizzleBlock8u_sse2
        .type SwizzleBlock8u_sse2, @function
SwizzleBlock8u_sse2:

	push		%ebx

	mov			%ebx, [%esp+4+4]
	mov			%eax, 2

	.align 16
SwizzleBlock8u_sse2_1:
	// col 0, 2

	movdqu		%xmm0, [%edx]
	movdqu		%xmm2, [%edx+%ebx]
	lea			%edx, [%edx+%ebx*2]

	movdqu		%xmm1, [%edx]
	movdqu		%xmm3, [%edx+%ebx]
	pshufd		%xmm1, %xmm1, 0xb1
	pshufd		%xmm3, %xmm3, 0xb1
	lea			%edx, [%edx+%ebx*2]

	punpck(bw, 0, 2, 1, 3, 4, 6)
	punpck(wd, 0, 2, 4, 6, 1, 3)
	punpck(qdq, 0, 1, 2, 3, 4, 5)

	movntps		[%ecx+16*0], %xmm0
	movntps		[%ecx+16*1], %xmm4
	movntps		[%ecx+16*2], %xmm1
	movntps		[%ecx+16*3], %xmm5

	// col 1, 3

	movdqu		%xmm0, [%edx]
	movdqu		%xmm2, [%edx+%ebx]
	pshufd		%xmm0, %xmm0, 0xb1
	pshufd		%xmm2, %xmm2, 0xb1
	lea			%edx, [%edx+%ebx*2]

	movdqu		%xmm1, [%edx]
	movdqu		%xmm3, [%edx+%ebx]
	lea			%edx, [%edx+%ebx*2]

	punpck(bw, 0, 2, 1, 3, 4, 6)
	punpck(wd, 0, 2, 4, 6, 1, 3)
	punpck(qdq, 0, 1, 2, 3, 4, 5)

	movntps		[%ecx+16*4], %xmm0
	movntps		[%ecx+16*5], %xmm4
	movntps		[%ecx+16*6], %xmm1
	movntps		[%ecx+16*7], %xmm5

	add			%ecx, 128

	dec			%eax
	jnz			SwizzleBlock8u_sse2_1

	pop			%ebx

	ret			4

//
// SwizzleBlock4u
//

.globl SwizzleBlock4u_sse2
        .type SwizzleBlock4u_sse2, @function
SwizzleBlock4u_sse2:

	push		%ebx
	
	mov         %eax, 0xf0f0f0f
	movd        %xmm7, %eax 
	pshufd      %xmm7, %xmm7, 0

	mov			%ebx, [%esp+4+4]
	mov			%eax, 2

	.align 16
SwizzleBlock4u_sse2_1:
	// col 0, 2

	movdqu		%xmm0, [%edx]
	movdqu		%xmm2, [%edx+%ebx]
	lea			%edx, [%edx+%ebx*2]

	movdqu		%xmm1, [%edx]
	movdqu		%xmm3, [%edx+%ebx]
	lea			%edx, [%edx+%ebx*2]

	pshuflw		%xmm1, %xmm1, 0xb1
	pshuflw		%xmm3, %xmm3, 0xb1
	pshufhw		%xmm1, %xmm1, 0xb1
	pshufhw		%xmm3, %xmm3, 0xb1

	punpcknb
	punpck(bw, 0, 2, 4, 6, 1, 3)
	punpck(bw, 0, 2, 1, 3, 4, 6)
	punpck(qdq, 0, 4, 2, 6, 1, 3)

	movntps		[%ecx+16*0], %xmm0
	movntps		[%ecx+16*1], %xmm1
	movntps		[%ecx+16*2], %xmm4
	movntps		[%ecx+16*3], %xmm3

	// col 1, 3

	movdqu		%xmm0, [%edx]
	movdqu		%xmm2, [%edx+%ebx]
	lea			%edx, [%edx+%ebx*2]

	movdqu		%xmm1, [%edx]
	movdqu		%xmm3, [%edx+%ebx]
	lea			%edx, [%edx+%ebx*2]

	pshuflw		%xmm0, %xmm0, 0xb1
	pshuflw		%xmm2, %xmm2, 0xb1
	pshufhw		%xmm0, %xmm0, 0xb1
	pshufhw		%xmm2, %xmm2, 0xb1

	punpcknb
	punpck(bw, 0, 2, 4, 6, 1, 3)
	punpck(bw, 0, 2, 1, 3, 4, 6)
	punpck(qdq, 0, 4, 2, 6, 1, 3)

	movntps		[%ecx+16*4], %xmm0
	movntps		[%ecx+16*5], %xmm1
	movntps		[%ecx+16*6], %xmm4
	movntps		[%ecx+16*7], %xmm3

	add			%ecx, 128

	dec			%eax
	jnz			SwizzleBlock4u_sse2_1

	pop			%ebx

	ret			4


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
		movdqa %xmm0, xmmword ptr [%ecx]
		movdqa %xmm1, xmmword ptr [%ecx+16]
		movdqa %xmm2, xmmword ptr [%ecx+32]
		movdqa %xmm3, xmmword ptr [%ecx+48]

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

		test %edx, 15
		jnz WriteUnaligned

		movdqa %xmm7, [s_clut16mask] // saves upper 16 bits

		// have to save interlaced with the old data
		movdqa %xmm4, [%edx]
		movdqa %xmm5, [%edx+32]
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

		movdqa [%edx], %xmm0
		movdqa [%edx+32], %xmm1

		movdqa %xmm5, %xmm7
		pand %xmm7, [%edx+16]
		pand %xmm5, [%edx+48]

		por %xmm2, %xmm7
		por %xmm3, %xmm5

		movdqa [%edx+16], %xmm2
		movdqa [%edx+48], %xmm3
		jmp WriteCLUT_T16_I4_CSM1_End

WriteUnaligned:
		// %edx is offset by 2
		sub %edx, 2

		movdqa %xmm7, [s_clut16mask2] // saves lower 16 bits

		// have to save interlaced with the old data
		movdqa %xmm4, [%edx]
		movdqa %xmm5, [%edx+32]
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

		movdqa [%edx], %xmm0
		movdqa [%edx+32], %xmm1

		movdqa %xmm5, %xmm7
		pand %xmm7, [%edx+16]
		pand %xmm5, [%edx+48]

		por %xmm2, %xmm7
		por %xmm3, %xmm5

		movdqa [%edx+16], %xmm2
		movdqa [%edx+48], %xmm3
WriteCLUT_T16_I4_CSM1_End:
        ret


.globl WriteCLUT_T32_I8_CSM1_sse2
	.type	WriteCLUT_T32_I8_CSM1_sse2, @function
WriteCLUT_T32_I8_CSM1_sse2:
	push	%ebx
	xor	%ebx, %ebx
.L231:
	xor	%eax, %eax
	.align 16
.L232:
	movdqa	%xmm3, XMMWORD PTR [%eax+16+%ecx]
	movdqa	%xmm4, XMMWORD PTR [%eax+48+%ecx]
	movdqa	%xmm1, XMMWORD PTR [%eax+%ecx]
	movdqa	%xmm2, XMMWORD PTR [%eax+32+%ecx]
	movdqa	%xmm0, %xmm1
	punpckhqdq	%xmm1, %xmm3
	punpcklqdq	%xmm0, %xmm3
	movdqa	XMMWORD PTR [%edx+32+%eax*2], %xmm1
	movdqa	XMMWORD PTR [%edx+%eax*2], %xmm0
	movdqa	%xmm0, %xmm2
	punpckhqdq	%xmm2, %xmm4
	punpcklqdq	%xmm0, %xmm4
	movdqa	XMMWORD PTR [%edx+48+%eax*2], %xmm2
	movdqa	XMMWORD PTR [%edx+16+%eax*2], %xmm0
	movdqa	%xmm1, XMMWORD PTR [%eax+256+%ecx]
	movdqa	%xmm3, XMMWORD PTR [%eax+272+%ecx]
	movdqa	%xmm2, XMMWORD PTR [%eax+288+%ecx]
	movdqa	%xmm4, XMMWORD PTR [%eax+304+%ecx]
	movdqa	%xmm0, %xmm1
	punpckhqdq	%xmm1, %xmm3
	punpcklqdq	%xmm0, %xmm3
	movdqa	XMMWORD PTR [%edx+96+%eax*2], %xmm1
	movdqa	XMMWORD PTR [%edx+64+%eax*2], %xmm0
	movdqa	%xmm0, %xmm2
	punpckhqdq	%xmm2, %xmm4
	punpcklqdq	%xmm0, %xmm4
	movdqa	XMMWORD PTR [%edx+112+%eax*2], %xmm2
	movdqa	XMMWORD PTR [%edx+80+%eax*2], %xmm0
	add	%eax, 64
	cmp	%eax, 256
	jne	.L232
    add %edx, 512
    add %ecx, 512
	add	%ebx, 512
	cmp	%ebx, 1024
	jne	.L231
	pop	%ebx
	ret

.globl WriteCLUT_T32_I4_CSM1_sse2
	.type	WriteCLUT_T32_I4_CSM1_sse2, @function
WriteCLUT_T32_I4_CSM1_sse2:
	movdqa	%xmm1, XMMWORD PTR [%ecx]
	movdqa	%xmm3, XMMWORD PTR [%ecx+16]
	movdqa	%xmm2, XMMWORD PTR [%ecx+32]
	movdqa	%xmm4, XMMWORD PTR [%ecx+48]
	movdqa	%xmm0, %xmm1
	punpckhqdq	%xmm1, %xmm3
	punpcklqdq	%xmm0, %xmm3
	movdqa	XMMWORD PTR [%edx+32], %xmm1
	movdqa	XMMWORD PTR [%edx], %xmm0
	movdqa	%xmm0, %xmm2
	punpckhqdq	%xmm2, %xmm4
	punpcklqdq	%xmm0, %xmm4
	movdqa	XMMWORD PTR [%edx+48], %xmm2
	movdqa	XMMWORD PTR [%edx+16], %xmm0
	ret
                        
#endif
