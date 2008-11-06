;	Copyright (C) 2003-2005 Gabest/zerofrog
;	http:;;www.gabest.org
;
;  This Program is free software; you can redistribute it and/or modify
;  it under the terms of the GNU General Public License as published by
;  the Free Software Foundation; either version 2, or (at your option)
;  any later version.
;   
;  This Program is distributed in the hope that it will be useful,
;  but WITHOUT ANY WARRANTY; without even the implied warranty of
;  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
;  GNU General Public License for more details.
;   
;  You should have received a copy of the GNU General Public License
;  along with GNU Make; see the file COPYING.  If not, write to
;  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA. 
;  http:;;www.gnu.org/copyleft/gpl.html
;
;
extern s_clut16mask:ptr

	.code

; mmx memcpy implementation, size has to be a multiple of 8
; returns 0 is equal, nonzero value if not equal
; ~10 times faster than standard memcmp
; (zerofrog)
; u8 memcmp_mmx(const void* src1, const void* src2, int cmpsize)
; rcx - src1
; rdx - src2
; r8d - cmpsize
memcmp_mmx proc public
        cmp r8d, 32
		jl Done4

		; custom test first 8 to make sure things are ok
		movq mm0, [rdx]
		movq mm1, [rdx+8]
		pcmpeqd mm0, [rcx]
		pcmpeqd mm1, [rcx+8]
		pand mm0, mm1
		movq mm2, [rdx+16]
		pmovmskb eax, mm0
		movq mm3, [rdx+24]

		; check if eq
		cmp eax, 0ffh
		je NextComp
		mov eax, 1
		jmp Finish

NextComp:
		pcmpeqd mm2, [rcx+16]
		pcmpeqd mm3, [rcx+24]
		pand mm2, mm3
		pmovmskb eax, mm2

		sub r8d, 32
		add rdx, 32
		add rcx, 32

		; check if eq
		cmp eax, 0ffh
		je ContinueTest
		mov eax, 1
		jmp Finish

		cmp r8d, 64
		jl Done8

Cmp8:
		movq mm0, [rdx]
		movq mm1, [rdx+8]
		movq mm2, [rdx+16]
		movq mm3, [rdx+24]
		movq mm4, [rdx+32]
		movq mm5, [rdx+40]
		movq mm6, [rdx+48]
		movq mm7, [rdx+56]
		pcmpeqd mm0, [rcx]
		pcmpeqd mm1, [rcx+8]
		pcmpeqd mm2, [rcx+16]
		pcmpeqd mm3, [rcx+24]
		pand mm0, mm1
		pcmpeqd mm4, [rcx+32]
		pand mm0, mm2
		pcmpeqd mm5, [rcx+40]
		pand mm0, mm3
		pcmpeqd mm6, [rcx+48]
		pand mm0, mm4
		pcmpeqd mm7, [rcx+56]
		pand mm0, mm5
		pand mm0, mm6
		pand mm0, mm7
		pmovmskb eax, mm0
		
		; check if eq
		cmp eax, 0ffh
		je Continue
		mov eax, 1
		jmp Finish

Continue:
		sub r8d, 64
		add rdx, 64
		add rcx, 64
ContinueTest:
		cmp r8d, 64
		jge Cmp8

Done8:
		test r8d, 020h
		jz Done4
		movq mm0, [rdx]
		movq mm1, [rdx+8]
		movq mm2, [rdx+16]
		movq mm3, [rdx+24]
		pcmpeqd mm0, [rcx]
		pcmpeqd mm1, [rcx+8]
		pcmpeqd mm2, [rcx+16]
		pcmpeqd mm3, [rcx+24]
		pand mm0, mm1
		pand mm0, mm2
		pand mm0, mm3
		pmovmskb eax, mm0
		sub r8d, 32
		add rdx, 32
		add rcx, 32

		; check if eq
		cmp eax, 0ffh
		je Done4
		mov eax, 1
		jmp Finish

Done4:
		cmp r8d, 24
		jne Done2
		movq mm0, [rdx]
		movq mm1, [rdx+8]
		movq mm2, [rdx+16]
		pcmpeqd mm0, [rcx]
		pcmpeqd mm1, [rcx+8]
		pcmpeqd mm2, [rcx+16]
		pand mm0, mm1
		pand mm0, mm2
		pmovmskb eax, mm0

		; check if eq
		cmp eax, 0ffh
        je Done
		mov eax, 1
		jmp Finish

Done2:
		cmp r8d, 16
		jne Done1

		movq mm0, [rdx]
		movq mm1, [rdx+8]
		pcmpeqd mm0, [rcx]
		pcmpeqd mm1, [rcx+8]
		pand mm0, mm1
		pmovmskb eax, mm0

		; check if eq
		cmp eax, 0ffh
        je Done
		mov eax, 1
		jmp Finish

Done1:
		cmp r8d, 8
		jne Done

		mov eax, [rdx]
		mov rdx, [rdx+4]
		cmp eax, [rcx]
		je Next
		mov eax, 1
		jmp Finish

Next:
		cmp rdx, [rcx+4]
        je Done
		mov eax, 1
		jmp Finish

Done:
		xor eax, eax

Finish:
		emms
        ret
           
memcmp_mmx endp

; TestClutChangeMMX
; mov rdx, dst
; mov rcx, src
; mov r8d, entries
TestClutChangeMMX proc public

Start:
		movq mm0, [rdx]
		movq mm1, [rdx+8]
		pcmpeqd mm0, [rcx]
		pcmpeqd mm1, [rcx+16]

		movq mm2, [rdx+16]
		movq mm3, [rdx+24]
		pcmpeqd mm2, [rcx+32]
		pcmpeqd mm3, [rcx+48]

		pand mm0, mm1
		pand mm2, mm3
		movq mm4, [rdx+32]
		movq mm5, [rdx+40]
		pcmpeqd mm4, [rcx+8]
		pcmpeqd mm5, [rcx+24]

		pand mm0, mm2
		pand mm4, mm5
		movq mm6, [rdx+48]
		movq mm7, [rdx+56]
		pcmpeqd mm6, [rcx+40]
		pcmpeqd mm7, [rcx+56]
		
		pand mm0, mm4
		pand mm6, mm7
		pand mm0, mm6

		pmovmskb eax, mm0
		cmp eax, 0ffh
		je Continue
		mov byte ptr [r9], 1
		jmp Return

Continue:
		cmp r8d, 16
		jle Return

		test r8d, 010h
		jz AddRcx
		sub rcx, 448 ; go back and down one column, 
AddRcx:
		add rcx, 256 ; go to the right block

	
		jne Continue1
		add rcx, 256 ; skip whole block
Continue1:
		add rdx, 64
		sub r8d, 16
		jmp Start

Return:
		emms
		ret
		
TestClutChangeMMX endp

UnswizzleZ16Target proc public
		pxor xmm7, xmm7

Z16Loop:
		;; unpack 64 bytes at a time
		movdqa xmm0, [rdx]
		movdqa xmm2, [rdx+16]
		movdqa xmm4, [rdx+32]
		movdqa xmm6, [rdx+48]

		movdqa xmm1, xmm0
		movdqa xmm3, xmm2
		movdqa xmm5, xmm4

		punpcklwd xmm0, xmm7
		punpckhwd xmm1, xmm7
		punpcklwd xmm2, xmm7
		punpckhwd xmm3, xmm7

		;; start saving
		movdqa [rcx], xmm0
		movdqa [rcx+16], xmm1

		punpcklwd xmm4, xmm7
		punpckhwd xmm5, xmm7

		movdqa [rcx+32], xmm2
		movdqa [rcx+48], xmm3

		movdqa xmm0, xmm6
		punpcklwd xmm6, xmm7

		movdqa [rcx+64], xmm4
		movdqa [rcx+80], xmm5

		punpckhwd xmm0, xmm7

		movdqa [rcx+96], xmm6
		movdqa [rcx+112], xmm0

		add rdx, 64
		add rcx, 128
		sub r9d, 1
		jne Z16Loop
		
		ret
UnswizzleZ16Target endp
                     	
;
; swizzling
;

punpck macro op, sd0, sd2, s1, s3, d1, d3

	movdqa					@CatStr(xmm, %d1),	@CatStr(xmm, %sd0)
	pshufd					@CatStr(xmm, %d3),	@CatStr(xmm, %sd2), 0e4h
	
	@CatStr(punpckl, op)	@CatStr(xmm, %sd0),	@CatStr(xmm, %s1)
	@CatStr(punpckh, op)	@CatStr(xmm, %d1),	@CatStr(xmm, %s1)
	@CatStr(punpckl, op)	@CatStr(xmm, %sd2),	@CatStr(xmm, %s3)
	@CatStr(punpckh, op)	@CatStr(xmm, %d3),	@CatStr(xmm, %s3)

	endm

punpcknbl macro

	movdqa	xmm4, xmm0
	pshufd	xmm5, xmm1, 0e4h

	psllq	xmm1, 4
	psrlq	xmm4, 4

	movdqa	xmm6, xmm7
	pand	xmm0, xmm7
	pandn	xmm6, xmm1
	por		xmm0, xmm6

	movdqa	xmm6, xmm7
	pand	xmm4, xmm7
	pandn	xmm6, xmm5
	por		xmm4, xmm6

	movdqa	xmm1, xmm4

	movdqa	xmm4, xmm2
	pshufd	xmm5, xmm3, 0e4h

	psllq	xmm3, 4
	psrlq	xmm4, 4

	movdqa	xmm6, xmm7
	pand	xmm2, xmm7
	pandn	xmm6, xmm3
	por		xmm2, xmm6

	movdqa	xmm6, xmm7
	pand	xmm4, xmm7
	pandn	xmm6, xmm5
	por		xmm4, xmm6

	movdqa	xmm3, xmm4

	punpck	bw, 0, 2, 1, 3, 4, 6

	endm

punpcknbh macro

	movdqa	xmm12, xmm8
	pshufd	xmm13, xmm9, 0e4h

	psllq	xmm9, 4
	psrlq	xmm12, 4

	movdqa	xmm14, xmm15
	pand	xmm8, xmm15
	pandn	xmm14, xmm9
	por		xmm8, xmm14

	movdqa	xmm14, xmm15
	pand	xmm12, xmm15
	pandn	xmm14, xmm13
	por		xmm12, xmm14

	movdqa	xmm9, xmm12

	movdqa	xmm12, xmm10
	pshufd	xmm13, xmm11, 0e4h

	psllq	xmm11, 4
	psrlq	xmm12, 4

	movdqa	xmm14, xmm15
	pand	xmm10, xmm15
	pandn	xmm14, xmm11
	por		xmm10, xmm14

	movdqa	xmm14, xmm15
	pand	xmm12, xmm15
	pandn	xmm14, xmm13
	por		xmm12, xmm14

	movdqa	xmm11, xmm12

	punpck	bw, 8, 10, 9, 11, 12, 14

	endm

;
; SwizzleBlock32_sse2
;

SwizzleBlock32_sse2 proc public

	push		rsi
	push		rdi

	mov			rdi, rcx
	mov			rsi, rdx
	mov			rcx, 4

	cmp			r9d, 0ffffffffh
	jne			SwizzleBlock32_sse2@WM

	align 16
@@:
	movdqa		xmm0, [rsi]
	movdqa		xmm4, [rsi+16]
	movdqa		xmm1, [rsi+r8]
	movdqa		xmm5, [rsi+r8+16]

	punpck		qdq, 0, 4, 1, 5, 2, 6

	movdqa		[rdi+16*0], xmm0
	movdqa		[rdi+16*1], xmm2
	movdqa		[rdi+16*2], xmm4
	movdqa		[rdi+16*3], xmm6

	lea			rsi, [rsi+r8*2]
	add			rdi, 64

	dec			rcx
	jnz			@B

	pop			rdi
	pop			rsi

	ret

SwizzleBlock32_sse2@WM:

	movd		xmm7, r9d
	pshufd		xmm7, xmm7, 0
	
	align 16
@@:
	movdqa		xmm0, [rsi]
	movdqa		xmm4, [rsi+16]
	movdqa		xmm1, [rsi+r8]
	movdqa		xmm5, [rsi+r8+16]

	punpck		qdq, 0, 4, 1, 5, 2, 6

	movdqa		xmm3, xmm7
	pshufd		xmm5, xmm7, 0e4h
	movdqa		xmm9, xmm7
	pshufd		xmm11, xmm7, 0e4h

	pandn		xmm3, [rdi+16*0]
	pand		xmm0, xmm7
	por			xmm0, xmm3
	movdqa		[rdi+16*0], xmm0

	pandn		xmm5, [rdi+16*1]
	pand		xmm2, xmm7
	por			xmm2, xmm5
	movdqa		[rdi+16*1], xmm2

	pandn		xmm9, [rdi+16*2]
	pand		xmm4, xmm7
	por			xmm4, xmm9
	movdqa		[rdi+16*2], xmm4

	pandn		xmm11, [rdi+16*3]
	pand		xmm6, xmm7
	por			xmm6, xmm11
	movdqa		[edi+16*3], xmm6

	lea			rsi, [rsi+r8*2]
	add			rdi, 64

	dec			rcx
	jnz			@B

	pop			rdi
	pop			rsi

	ret
	
SwizzleBlock32_sse2 endp

;
; SwizzleBlock16_sse2
;

SwizzleBlock16_sse2 proc public

	push		rsi
	push		rdi

	mov			rdi, rcx
	mov			rsi, rdx
	mov			rcx, 4

	align 16
@@:
	movdqa		xmm0, [rsi]
	movdqa		xmm1, [rsi+16]
	movdqa		xmm2, [rsi+r8]
	movdqa		xmm3, [rsi+r8+16]

	punpck		wd, 0, 2, 1, 3, 4, 6
	punpck		qdq, 0, 4, 2, 6, 1, 5

	movdqa		[rdi+16*0], xmm0
	movdqa		[rdi+16*1], xmm1
	movdqa		[rdi+16*2], xmm4
	movdqa		[rdi+16*3], xmm5

	lea			rsi, [rsi+r8*2]
	add			rdi, 64

	dec			rcx
	jnz			@B

	pop			rdi
	pop			rsi

	ret
	
SwizzleBlock16_sse2 endp

;
; SwizzleBlock8
;

SwizzleBlock8_sse2 proc public

	push		rsi
	push		rdi

	mov			rdi, rcx
	mov			rsi, rdx
	mov			ecx, 2

	align 16
@@:
	; col 0, 2

	movdqa		xmm0, [rsi]
	movdqa		xmm2, [rsi+r8]
	lea			rsi, [rsi+r8*2]

	pshufd		xmm1, [rsi], 0b1h
	pshufd		xmm3, [rsi+r8], 0b1h
	lea			rsi, [rsi+r8*2]

	punpck		bw, 0, 2, 1, 3, 4, 6
	punpck		wd, 0, 2, 4, 6, 1, 3
	punpck		qdq, 0, 1, 2, 3, 4, 5

	movdqa		[rdi+16*0], xmm0
	movdqa		[rdi+16*1], xmm4
	movdqa		[rdi+16*2], xmm1
	movdqa		[rdi+16*3], xmm5

	; col 1, 3

	pshufd		xmm0, [rsi], 0b1h
	pshufd		xmm2, [rsi+r8], 0b1h
	lea			rsi, [rsi+r8*2]

	movdqa		xmm1, [rsi]
	movdqa		xmm3, [rsi+r8]
	lea			rsi, [rsi+r8*2]

	punpck		bw, 0, 2, 1, 3, 4, 6
	punpck		wd, 0, 2, 4, 6, 1, 3
	punpck		qdq, 0, 1, 2, 3, 4, 5

	movdqa		[rdi+16*4], xmm0
	movdqa		[rdi+16*5], xmm4
	movdqa		[rdi+16*6], xmm1
	movdqa		[rdi+16*7], xmm5

	add			edi, 128

	dec			rcx
	jnz			@B

	pop			rdi
	pop			rsi

	ret

SwizzleBlock8_sse2 endp

;
; SwizzleBlock4
;

SwizzleBlock4_sse2 proc public

	push		rsi
	push		rdi
	
	mov			rdi, rcx
	mov			rsi, rdx
	mov			rcx, 2

	mov         eax, 0f0f0f0fh
	movd        xmm7, eax 
	pshufd      xmm7, xmm7, 0

	align 16
@@:
	; col 0, 2

	movdqa		xmm0, [rsi]
	movdqa		xmm2, [rsi+r8]
	lea			rsi, [rsi+r8*2]

	movdqa		xmm1, [rsi]
	movdqa		xmm3, [rsi+r8]
	lea			rsi, [rsi+r8*2]

	pshuflw		xmm1, xmm1, 0b1h
	pshuflw		xmm3, xmm3, 0b1h
	pshufhw		xmm1, xmm1, 0b1h
	pshufhw		xmm3, xmm3, 0b1h

	punpcknbl
	punpck		bw, 0, 2, 4, 6, 1, 3
	punpck		bw, 0, 2, 1, 3, 4, 6
	punpck		qdq, 0, 4, 2, 6, 1, 3

	movdqa		[rdi+16*0], xmm0
	movdqa		[rdi+16*1], xmm1
	movdqa		[rdi+16*2], xmm4
	movdqa		[rdi+16*3], xmm3

	; col 1, 3

	movdqa		xmm0, [rsi]
	movdqa		xmm2, [rsi+r8]
	lea			esi, [rsi+r8*2]

	movdqa		xmm1, [rsi]
	movdqa		xmm3, [rsi+r8]
	lea			rsi, [rsi+r8*2]

	pshuflw		xmm0, xmm0, 0b1h
	pshuflw		xmm2, xmm2, 0b1h
	pshufhw		xmm0, xmm0, 0b1h
	pshufhw		xmm2, xmm2, 0b1h

	punpcknbl
	punpck		bw, 0, 2, 4, 6, 1, 3
	punpck		bw, 0, 2, 1, 3, 4, 6
	punpck		qdq, 0, 4, 2, 6, 1, 3

	movdqa		[rdi+16*4], xmm0
	movdqa		[rdi+16*5], xmm1
	movdqa		[rdi+16*6], xmm4
	movdqa		[rdi+16*7], xmm3

	add			rdi, 128

	dec			rcx
	jnz			@B

	pop			rdi
	pop			rsi

	ret

SwizzleBlock4_sse2 endp

;
; swizzling with unaligned reads
;

;
; SwizzleBlock32u_sse2
;

SwizzleBlock32u_sse2 proc public

	push		rsi
	push		rdi

	mov			rdi, rcx
	mov			rsi, rdx
	mov			rcx, 4

	cmp			r9d, 0ffffffffh
	jne			SwizzleBlock32u_sse2@WM

	align 16
@@:
	movdqu		xmm0, [rsi]
	movdqu		xmm4, [rsi+16]
	movdqu		xmm1, [rsi+r8]
	movdqu		xmm5, [rsi+r8+16]

	punpck		qdq, 0, 4, 1, 5, 2, 6

	movdqa		[rdi+16*0], xmm0
	movdqa		[rdi+16*1], xmm2
	movdqa		[rdi+16*2], xmm4
	movdqa		[rdi+16*3], xmm6

	lea			rsi, [rsi+r8*2]
	add			rdi, 64

	dec			rcx
	jnz			@B

	pop			rdi
	pop			rsi

	ret

SwizzleBlock32u_sse2@WM:

	movd		xmm7, r9d
	pshufd		xmm7, xmm7, 0
	
	align 16
@@:
	movdqu		xmm0, [rsi]
	movdqu		xmm4, [rsi+16]
	movdqu		xmm1, [rsi+r8]
	movdqu		xmm5, [rsi+r8+16]

	punpck		qdq, 0, 4, 1, 5, 2, 6

	movdqa		xmm3, xmm7
	pshufd		xmm5, xmm7, 0e4h
	movdqa		xmm9, xmm7
	pshufd		xmm11, xmm7, 0e4h

	pandn		xmm3, [rdi+16*0]
	pand		xmm0, xmm7
	por			xmm0, xmm3
	movdqa		[rdi+16*0], xmm0

	pandn		xmm5, [rdi+16*1]
	pand		xmm2, xmm7
	por			xmm2, xmm5
	movdqa		[rdi+16*1], xmm2

	pandn		xmm9, [rdi+16*2]
	pand		xmm4, xmm7
	por			xmm4, xmm9
	movdqa		[rdi+16*2], xmm4

	pandn		xmm11, [rdi+16*3]
	pand		xmm6, xmm7
	por			xmm6, xmm11
	movdqa		[edi+16*3], xmm6

	lea			rsi, [rsi+r8*2]
	add			rdi, 64

	dec			rcx
	jnz			@B

	pop			rdi
	pop			rsi

	ret
	
SwizzleBlock32u_sse2 endp

;
; SwizzleBlock16u_sse2
;

SwizzleBlock16u_sse2 proc public

	push		rsi
	push		rdi

	mov			rdi, rcx
	mov			rsi, rdx
	mov			rcx, 4

	align 16
@@:
	movdqu		xmm0, [rsi]
	movdqu		xmm1, [rsi+16]
	movdqu		xmm2, [rsi+r8]
	movdqu		xmm3, [rsi+r8+16]

	punpck		wd, 0, 2, 1, 3, 4, 6
	punpck		qdq, 0, 4, 2, 6, 1, 5

	movdqa		[rdi+16*0], xmm0
	movdqa		[rdi+16*1], xmm1
	movdqa		[rdi+16*2], xmm4
	movdqa		[rdi+16*3], xmm5

	lea			rsi, [rsi+r8*2]
	add			rdi, 64

	dec			rcx
	jnz			@B

	pop			rdi
	pop			rsi

	ret
	
SwizzleBlock16u_sse2 endp

;
; SwizzleBlock8u
;

SwizzleBlock8u_sse2 proc public

	push		rsi
	push		rdi

	mov			rdi, rcx
	mov			rsi, rdx
	mov			ecx, 2

	align 16
@@:
	; col 0, 2

	movdqu		xmm0, [rsi]
	movdqu		xmm2, [rsi+r8]
	lea			rsi, [rsi+r8*2]

	pshufd		xmm1, xmm0, 0b1h
	pshufd		xmm3, xmm2, 0b1h
	lea			rsi, [rsi+r8*2]

	punpck		bw, 0, 2, 1, 3, 4, 6
	punpck		wd, 0, 2, 4, 6, 1, 3
	punpck		qdq, 0, 1, 2, 3, 4, 5

	movdqa		[rdi+16*0], xmm0
	movdqa		[rdi+16*1], xmm4
	movdqa		[rdi+16*2], xmm1
	movdqa		[rdi+16*3], xmm5

	; col 1, 3

	movdqu		xmm0, [rsi]
	movdqu		xmm2, [rsi+r8]
	pshufd		xmm0, xmm0, 0b1h
	pshufd		xmm2, xmm2, 0b1h
	lea			rsi, [rsi+r8*2]

	movdqu		xmm1, [rsi]
	movdqu		xmm3, [rsi+r8]
	lea			rsi, [rsi+r8*2]

	punpck		bw, 0, 2, 1, 3, 4, 6
	punpck		wd, 0, 2, 4, 6, 1, 3
	punpck		qdq, 0, 1, 2, 3, 4, 5

	movdqa		[rdi+16*4], xmm0
	movdqa		[rdi+16*5], xmm4
	movdqa		[rdi+16*6], xmm1
	movdqa		[rdi+16*7], xmm5

	add			edi, 128

	dec			rcx
	jnz			@B

	pop			rdi
	pop			rsi

	ret

SwizzleBlock8u_sse2 endp

;
; SwizzleBlock4u
;

SwizzleBlock4u_sse2 proc public

	push		rsi
	push		rdi
	
	mov			rdi, rcx
	mov			rsi, rdx
	mov			rcx, 2

	mov         eax, 0f0f0f0fh
	movd        xmm7, eax 
	pshufd      xmm7, xmm7, 0

	align 16
@@:
	; col 0, 2

	movdqu		xmm0, [rsi]
	movdqu		xmm2, [rsi+r8]
	lea			rsi, [rsi+r8*2]

	movdqu		xmm1, [rsi]
	movdqu		xmm3, [rsi+r8]
	lea			rsi, [rsi+r8*2]

	pshuflw		xmm1, xmm1, 0b1h
	pshuflw		xmm3, xmm3, 0b1h
	pshufhw		xmm1, xmm1, 0b1h
	pshufhw		xmm3, xmm3, 0b1h

	punpcknbl
	punpck		bw, 0, 2, 4, 6, 1, 3
	punpck		bw, 0, 2, 1, 3, 4, 6
	punpck		qdq, 0, 4, 2, 6, 1, 3

	movdqa		[rdi+16*0], xmm0
	movdqa		[rdi+16*1], xmm1
	movdqa		[rdi+16*2], xmm4
	movdqa		[rdi+16*3], xmm3

	; col 1, 3

	movdqu		xmm0, [rsi]
	movdqu		xmm2, [rsi+r8]
	lea			esi, [rsi+r8*2]

	movdqu		xmm1, [rsi]
	movdqu		xmm3, [rsi+r8]
	lea			rsi, [rsi+r8*2]

	pshuflw		xmm0, xmm0, 0b1h
	pshuflw		xmm2, xmm2, 0b1h
	pshufhw		xmm0, xmm0, 0b1h
	pshufhw		xmm2, xmm2, 0b1h

	punpcknbl
	punpck		bw, 0, 2, 4, 6, 1, 3
	punpck		bw, 0, 2, 1, 3, 4, 6
	punpck		qdq, 0, 4, 2, 6, 1, 3

	movdqa		[rdi+16*4], xmm0
	movdqa		[rdi+16*5], xmm1
	movdqa		[rdi+16*6], xmm4
	movdqa		[rdi+16*7], xmm3

	add			rdi, 128

	dec			rcx
	jnz			@B

	pop			rdi
	pop			rsi

	ret

SwizzleBlock4u_sse2 endp

WriteCLUT_T16_I4_CSM1_sse2 proc public
        movdqa xmm0, XMMWORD PTR [rcx]
		movdqa xmm1, XMMWORD PTR [rcx+16]
		movdqa xmm2, XMMWORD PTR [rcx+32]
		movdqa xmm3, XMMWORD PTR [rcx+48]

		;; rearrange
		pshuflw xmm0, xmm0, 088h
		pshufhw xmm0, xmm0, 088h
		pshuflw xmm1, xmm1, 088h
		pshufhw xmm1, xmm1, 088h
		pshuflw xmm2, xmm2, 088h
		pshufhw xmm2, xmm2, 088h
		pshuflw xmm3, xmm3, 088h
		pshufhw xmm3, xmm3, 088h

		shufps xmm0, xmm1, 088h
		shufps xmm2, xmm3, 088h

		pshufd xmm0, xmm0, 0d8h
		pshufd xmm2, xmm2, 0d8h

		pxor xmm6, xmm6
		mov rax, offset s_clut16mask

		test rdx, 15
		jnz WriteUnaligned

		movdqa xmm7, XMMWORD PTR [rax] ;; saves upper 16 bits

		;; have to save interlaced with the old data
		movdqa xmm4, [rdx]
		movdqa xmm5, [rdx+32]
		movhlps xmm1, xmm0
		movlhps xmm0, xmm2 ;; lower 8 colors

		pand xmm4, xmm7
		pand xmm5, xmm7

		shufps xmm1, xmm2, 0e4h ;; upper 8 colors
		movdqa xmm2, xmm0
		movdqa xmm3, xmm1

		punpcklwd xmm0, xmm6
		punpcklwd xmm1, xmm6
		por xmm0, xmm4
		por xmm1, xmm5

		punpckhwd xmm2, xmm6
		punpckhwd xmm3, xmm6

		movdqa [rdx], xmm0
		movdqa [rdx+32], xmm1

		movdqa xmm5, xmm7
		pand xmm7, [rdx+16]
		pand xmm5, [rdx+48]

		por xmm2, xmm7
		por xmm3, xmm5

		movdqa [rdx+16], xmm2
		movdqa [rdx+48], xmm3
		jmp WriteCLUT_T16_I4_CSM1_End

WriteUnaligned:
		;; rdx is offset by 2
		sub rdx, 2

		movdqa xmm7, XMMWORD PTR [rax+16] ;; saves lower 16 bits

		;; have to save interlaced with the old data
		movdqa xmm4, [rdx]
		movdqa xmm5, [rdx+32]
		movhlps xmm1, xmm0
		movlhps xmm0, xmm2 ;; lower 8 colors

		pand xmm4, xmm7
		pand xmm5, xmm7

		shufps xmm1, xmm2, 0e4h ;; upper 8 colors
		movdqa xmm2, xmm0
		movdqa xmm3, xmm1

		punpcklwd xmm0, xmm6
		punpcklwd xmm1, xmm6
		pslld xmm0, 16
		pslld xmm1, 16
		por xmm0, xmm4
		por xmm1, xmm5

		punpckhwd xmm2, xmm6
		punpckhwd xmm3, xmm6
		pslld xmm2, 16
		pslld xmm3, 16

		movdqa [rdx], xmm0
		movdqa [rdx+32], xmm1

		movdqa xmm5, xmm7
		pand xmm7, [rdx+16]
		pand xmm5, [rdx+48]

		por xmm2, xmm7
		por xmm3, xmm5

		movdqa [rdx+16], xmm2
		movdqa [rdx+48], xmm3
WriteCLUT_T16_I4_CSM1_End:
        ret
        
WriteCLUT_T16_I4_CSM1_sse2 endp

end