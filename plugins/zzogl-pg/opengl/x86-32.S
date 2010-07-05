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
                        
#endif

#if defined(__linux__) && defined(__ELF__)
.section .note.GNU-stack,"",%progbits
#endif
