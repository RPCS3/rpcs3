;	Copyright (C) 2003-2005 Gabest
;	http://www.gabest.org
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
;  http://www.gnu.org/copyleft/gpl.html
;
;
	.686
	.model flat
	.mmx
	.xmm

	.const

	__uvmin DD 0d01502f9r ; -1e+010
	__uvmax DD 0501502f9r ; +1e+010

	.code

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
	
punpcknb macro

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


;
; swizzling
;

;
; SwizzleBlock32
;

@SwizzleBlock32_sse2@16 proc public


	push		esi
	push		edi

	mov			edi, ecx
	mov			esi, edx
	mov			edx, [esp+4+8]
	mov			ecx, 4

	mov			eax, [esp+8+8]
	cmp			eax, 0ffffffffh
	jne			SwizzleBlock32_sse2@WM

	align 16
@@:
	movdqa		xmm0, [esi]
	movdqa		xmm4, [esi+16]
	movdqa		xmm1, [esi+edx]
	movdqa		xmm5, [esi+edx+16]

	punpck		qdq, 0, 4, 1, 5, 2, 6

	movntps		[edi+16*0], xmm0
	movntps		[edi+16*1], xmm2
	movntps		[edi+16*2], xmm4
	movntps		[edi+16*3], xmm6

	lea			esi, [esi+edx*2]
	add			edi, 64

	dec			ecx
	jnz			@B

	pop			edi
	pop			esi

	ret			8

SwizzleBlock32_sse2@WM:

	movd		xmm7, eax
	pshufd		xmm7, xmm7, 0
	
	align 16
@@:
	movdqa		xmm0, [esi]
	movdqa		xmm4, [esi+16]
	movdqa		xmm1, [esi+edx]
	movdqa		xmm5, [esi+edx+16]

	punpck		qdq, 0, 4, 1, 5, 2, 6

	movdqa		xmm3, xmm7
	pshufd		xmm5, xmm7, 0e4h

	pandn		xmm3, [edi+16*0]
	pand		xmm0, xmm7
	por			xmm0, xmm3
	movntps		[edi+16*0], xmm0

	pandn		xmm5, [edi+16*1]
	pand		xmm2, xmm7
	por			xmm2, xmm5
	movntps		[edi+16*1], xmm2

	movdqa		xmm3, xmm7
	pshufd		xmm5, xmm7, 0e4h

	pandn		xmm3, [edi+16*2]
	pand		xmm4, xmm7
	por			xmm4, xmm3
	movntps		[edi+16*2], xmm4

	pandn		xmm5, [edi+16*3]
	pand		xmm6, xmm7
	por			xmm6, xmm5
	movntps		[edi+16*3], xmm6

	lea			esi, [esi+edx*2]
	add			edi, 64

	dec			ecx
	jnz			@B

	pop			edi
	pop			esi

	ret			8
	
@SwizzleBlock32_sse2@16 endp

;
; SwizzleBlock16
;

@SwizzleBlock16_sse2@12 proc public

	push		ebx

	mov			ebx, [esp+4+4]
	mov			eax, 4

	align 16
@@:
	movdqa		xmm0, [edx]
	movdqa		xmm1, [edx+16]
	movdqa		xmm2, [edx+ebx]
	movdqa		xmm3, [edx+ebx+16]

	punpck		wd, 0, 2, 1, 3, 4, 6
	punpck		qdq, 0, 4, 2, 6, 1, 5

	movntps		[ecx+16*0], xmm0
	movntps		[ecx+16*1], xmm1
	movntps		[ecx+16*2], xmm4
	movntps		[ecx+16*3], xmm5

	lea			edx, [edx+ebx*2]
	add			ecx, 64

	dec			eax
	jnz			@B

	pop			ebx

	ret			4

@SwizzleBlock16_sse2@12 endp

;
; SwizzleBlock8
;

@SwizzleBlock8_sse2@12 proc public

	push		ebx

	mov			ebx, [esp+4+4]
	mov			eax, 2

	align 16
@@:
	; col 0, 2

	movdqa		xmm0, [edx]
	movdqa		xmm2, [edx+ebx]
	lea			edx, [edx+ebx*2]

	pshufd		xmm1, [edx], 0b1h
	pshufd		xmm3, [edx+ebx], 0b1h
	lea			edx, [edx+ebx*2]

	punpck		bw, 0, 2, 1, 3, 4, 6
	punpck		wd, 0, 2, 4, 6, 1, 3
	punpck		qdq, 0, 1, 2, 3, 4, 5

	movntps		[ecx+16*0], xmm0
	movntps		[ecx+16*1], xmm4
	movntps		[ecx+16*2], xmm1
	movntps		[ecx+16*3], xmm5

	; col 1, 3

	pshufd		xmm0, [edx], 0b1h
	pshufd		xmm2, [edx+ebx], 0b1h
	lea			edx, [edx+ebx*2]

	movdqa		xmm1, [edx]
	movdqa		xmm3, [edx+ebx]
	lea			edx, [edx+ebx*2]

	punpck		bw, 0, 2, 1, 3, 4, 6
	punpck		wd, 0, 2, 4, 6, 1, 3
	punpck		qdq, 0, 1, 2, 3, 4, 5

	movntps		[ecx+16*4], xmm0
	movntps		[ecx+16*5], xmm4
	movntps		[ecx+16*6], xmm1
	movntps		[ecx+16*7], xmm5

	add			ecx, 128

	dec			eax
	jnz			@B

	pop			ebx

	ret			4
	
@SwizzleBlock8_sse2@12 endp

;
; SwizzleBlock4
;

@SwizzleBlock4_sse2@12 proc public

	push		ebx
	
	mov         eax, 0f0f0f0fh
	movd        xmm7, eax 
	pshufd      xmm7, xmm7, 0

	mov			ebx, [esp+4+4]
	mov			eax, 2

	align 16
@@:
	; col 0, 2

	movdqa		xmm0, [edx]
	movdqa		xmm2, [edx+ebx]
	lea			edx, [edx+ebx*2]

	movdqa		xmm1, [edx]
	movdqa		xmm3, [edx+ebx]
	lea			edx, [edx+ebx*2]

	pshuflw		xmm1, xmm1, 0b1h
	pshuflw		xmm3, xmm3, 0b1h
	pshufhw		xmm1, xmm1, 0b1h
	pshufhw		xmm3, xmm3, 0b1h

	punpcknb
	punpck		bw, 0, 2, 4, 6, 1, 3
	punpck		bw, 0, 2, 1, 3, 4, 6
	punpck		qdq, 0, 4, 2, 6, 1, 3

	movntps		[ecx+16*0], xmm0
	movntps		[ecx+16*1], xmm1
	movntps		[ecx+16*2], xmm4
	movntps		[ecx+16*3], xmm3

	; col 1, 3

	movdqa		xmm0, [edx]
	movdqa		xmm2, [edx+ebx]
	lea			edx, [edx+ebx*2]

	movdqa		xmm1, [edx]
	movdqa		xmm3, [edx+ebx]
	lea			edx, [edx+ebx*2]

	pshuflw		xmm0, xmm0, 0b1h
	pshuflw		xmm2, xmm2, 0b1h
	pshufhw		xmm0, xmm0, 0b1h
	pshufhw		xmm2, xmm2, 0b1h

	punpcknb
	punpck		bw, 0, 2, 4, 6, 1, 3
	punpck		bw, 0, 2, 1, 3, 4, 6
	punpck		qdq, 0, 4, 2, 6, 1, 3

	movntps		[ecx+16*4], xmm0
	movntps		[ecx+16*5], xmm1
	movntps		[ecx+16*6], xmm4
	movntps		[ecx+16*7], xmm3

	add			ecx, 128

	dec			eax
	jnz			@B

	pop			ebx

	ret			4

@SwizzleBlock4_sse2@12 endp

;
; swizzling with unaligned reads
;

;
; SwizzleBlock32u
;

@SwizzleBlock32u_sse2@16 proc public

	push		esi
	push		edi

	mov			edi, ecx
	mov			esi, edx
	mov			edx, [esp+4+8]
	mov			ecx, 4

	mov			eax, [esp+8+8]
	cmp			eax, 0ffffffffh
	jne			SwizzleBlock32u_sse2@WM

	align 16
@@:
	movdqu		xmm0, [esi]
	movdqu		xmm4, [esi+16]
	movdqu		xmm1, [esi+edx]
	movdqu		xmm5, [esi+edx+16]

	punpck		qdq, 0, 4, 1, 5, 2, 6

	movntps		[edi+16*0], xmm0
	movntps		[edi+16*1], xmm2
	movntps		[edi+16*2], xmm4
	movntps		[edi+16*3], xmm6

	lea			esi, [esi+edx*2]
	add			edi, 64

	dec			ecx
	jnz			@B

	pop			edi
	pop			esi

	ret			8

SwizzleBlock32u_sse2@WM:

	movd		xmm7, eax
	pshufd		xmm7, xmm7, 0
	
	align 16
@@:
	movdqu		xmm0, [esi]
	movdqu		xmm4, [esi+16]
	movdqu		xmm1, [esi+edx]
	movdqu		xmm5, [esi+edx+16]

	punpck		qdq, 0, 4, 1, 5, 2, 6

	movdqa		xmm3, xmm7
	pshufd		xmm5, xmm7, 0e4h

	pandn		xmm3, [edi+16*0]
	pand		xmm0, xmm7
	por			xmm0, xmm3
	movdqa		[edi+16*0], xmm0

	pandn		xmm5, [edi+16*1]
	pand		xmm2, xmm7
	por			xmm2, xmm5
	movdqa		[edi+16*1], xmm2

	movdqa		xmm3, xmm7
	pshufd		xmm5, xmm7, 0e4h

	pandn		xmm3, [edi+16*2]
	pand		xmm4, xmm7
	por			xmm4, xmm3
	movdqa		[edi+16*2], xmm4

	pandn		xmm5, [edi+16*3]
	pand		xmm6, xmm7
	por			xmm6, xmm5
	movdqa		[edi+16*3], xmm6

	lea			esi, [esi+edx*2]
	add			edi, 64

	dec			ecx
	jnz			@B

	pop			edi
	pop			esi

	ret			8
	
@SwizzleBlock32u_sse2@16 endp

;
; SwizzleBlock16u
;

@SwizzleBlock16u_sse2@12 proc public

	push		ebx

	mov			ebx, [esp+4+4]
	mov			eax, 4

	align 16
@@:
	movdqu		xmm0, [edx]
	movdqu		xmm1, [edx+16]
	movdqu		xmm2, [edx+ebx]
	movdqu		xmm3, [edx+ebx+16]

	punpck		wd, 0, 2, 1, 3, 4, 6
	punpck		qdq, 0, 4, 2, 6, 1, 5

	movntps		[ecx+16*0], xmm0
	movntps		[ecx+16*1], xmm1
	movntps		[ecx+16*2], xmm4
	movntps		[ecx+16*3], xmm5

	lea			edx, [edx+ebx*2]
	add			ecx, 64

	dec			eax
	jnz			@B

	pop			ebx

	ret			4

@SwizzleBlock16u_sse2@12 endp

;
; SwizzleBlock8u
;

@SwizzleBlock8u_sse2@12 proc public

	push		ebx

	mov			ebx, [esp+4+4]
	mov			eax, 2

	align 16
@@:
	; col 0, 2

	movdqu		xmm0, [edx]
	movdqu		xmm2, [edx+ebx]
	lea			edx, [edx+ebx*2]

	movdqu		xmm1, [edx]
	movdqu		xmm3, [edx+ebx]
	pshufd		xmm1, xmm1, 0b1h
	pshufd		xmm3, xmm3, 0b1h
	lea			edx, [edx+ebx*2]

	punpck		bw, 0, 2, 1, 3, 4, 6
	punpck		wd, 0, 2, 4, 6, 1, 3
	punpck		qdq, 0, 1, 2, 3, 4, 5

	movntps		[ecx+16*0], xmm0
	movntps		[ecx+16*1], xmm4
	movntps		[ecx+16*2], xmm1
	movntps		[ecx+16*3], xmm5

	; col 1, 3

	movdqu		xmm0, [edx]
	movdqu		xmm2, [edx+ebx]
	pshufd		xmm0, xmm0, 0b1h
	pshufd		xmm2, xmm2, 0b1h
	lea			edx, [edx+ebx*2]

	movdqu		xmm1, [edx]
	movdqu		xmm3, [edx+ebx]
	lea			edx, [edx+ebx*2]

	punpck		bw, 0, 2, 1, 3, 4, 6
	punpck		wd, 0, 2, 4, 6, 1, 3
	punpck		qdq, 0, 1, 2, 3, 4, 5

	movntps		[ecx+16*4], xmm0
	movntps		[ecx+16*5], xmm4
	movntps		[ecx+16*6], xmm1
	movntps		[ecx+16*7], xmm5

	add			ecx, 128

	dec			eax
	jnz			@B

	pop			ebx

	ret			4
	
@SwizzleBlock8u_sse2@12 endp

;
; SwizzleBlock4u
;

@SwizzleBlock4u_sse2@12 proc public

	push		ebx
	
	mov         eax, 0f0f0f0fh
	movd        xmm7, eax 
	pshufd      xmm7, xmm7, 0

	mov			ebx, [esp+4+4]
	mov			eax, 2

	align 16
@@:
	; col 0, 2

	movdqu		xmm0, [edx]
	movdqu		xmm2, [edx+ebx]
	lea			edx, [edx+ebx*2]

	movdqu		xmm1, [edx]
	movdqu		xmm3, [edx+ebx]
	lea			edx, [edx+ebx*2]

	pshuflw		xmm1, xmm1, 0b1h
	pshuflw		xmm3, xmm3, 0b1h
	pshufhw		xmm1, xmm1, 0b1h
	pshufhw		xmm3, xmm3, 0b1h

	punpcknb
	punpck		bw, 0, 2, 4, 6, 1, 3
	punpck		bw, 0, 2, 1, 3, 4, 6
	punpck		qdq, 0, 4, 2, 6, 1, 3

	movntps		[ecx+16*0], xmm0
	movntps		[ecx+16*1], xmm1
	movntps		[ecx+16*2], xmm4
	movntps		[ecx+16*3], xmm3

	; col 1, 3

	movdqu		xmm0, [edx]
	movdqu		xmm2, [edx+ebx]
	lea			edx, [edx+ebx*2]

	movdqu		xmm1, [edx]
	movdqu		xmm3, [edx+ebx]
	lea			edx, [edx+ebx*2]

	pshuflw		xmm0, xmm0, 0b1h
	pshuflw		xmm2, xmm2, 0b1h
	pshufhw		xmm0, xmm0, 0b1h
	pshufhw		xmm2, xmm2, 0b1h

	punpcknb
	punpck		bw, 0, 2, 4, 6, 1, 3
	punpck		bw, 0, 2, 1, 3, 4, 6
	punpck		qdq, 0, 4, 2, 6, 1, 3

	movntps		[ecx+16*4], xmm0
	movntps		[ecx+16*5], xmm1
	movntps		[ecx+16*6], xmm4
	movntps		[ecx+16*7], xmm3

	add			ecx, 128

	dec			eax
	jnz			@B

	pop			ebx

	ret			4

@SwizzleBlock4u_sse2@12 endp

	end