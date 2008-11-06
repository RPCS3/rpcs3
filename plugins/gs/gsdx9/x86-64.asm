
	.const

	__uvmin DD 0d01502f9r ; -1e+010
	__uvmax DD 0501502f9r ; +1e+010

	.code
	
;
; memsetd
;

memsetd proc public

	push	rdi

	mov		rdi, rcx
	mov		eax, edx
	mov		rcx, r8
	cld
	rep		stosd

	pop		rdi

	ret

memsetd endp

;
; SaturateColor
;

SaturateColor_amd64 proc public

	pxor		xmm0, xmm0
	movdqa		xmm1, [rcx]
	packssdw	xmm1, xmm0
	packuswb	xmm1, xmm0
	punpcklbw	xmm1, xmm0
	punpcklwd	xmm1, xmm0
	movdqa		[rcx], xmm1

	ret

SaturateColor_amd64 endp

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

punpck2 macro op, sd0, sd2, sd4, sd6, s1, s3, s5, s7, d1, d3, d5, d7

	movdqa					@CatStr(xmm, %d1),	@CatStr(xmm, %sd0)
	pshufd					@CatStr(xmm, %d3),	@CatStr(xmm, %sd2), 0e4h
	movdqa					@CatStr(xmm, %d5),	@CatStr(xmm, %sd4)
	pshufd					@CatStr(xmm, %d7),	@CatStr(xmm, %sd6), 0e4h
	
	@CatStr(punpckl, op)	@CatStr(xmm, %sd0),	@CatStr(xmm, %s1)
	@CatStr(punpckh, op)	@CatStr(xmm, %d1),	@CatStr(xmm, %s1)
	@CatStr(punpckl, op)	@CatStr(xmm, %sd2),	@CatStr(xmm, %s3)
	@CatStr(punpckh, op)	@CatStr(xmm, %d3),	@CatStr(xmm, %s3)
	@CatStr(punpckl, op)	@CatStr(xmm, %sd4),	@CatStr(xmm, %s5)
	@CatStr(punpckh, op)	@CatStr(xmm, %d5),	@CatStr(xmm, %s5)
	@CatStr(punpckl, op)	@CatStr(xmm, %sd6),	@CatStr(xmm, %s7)
	@CatStr(punpckh, op)	@CatStr(xmm, %d7),	@CatStr(xmm, %s7)

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
; unSwizzleBlock32
;

unSwizzleBlock32_amd64 proc public

	push		rsi
	push		rdi

	mov			rsi, rcx
	mov			rdi, rdx
	mov			rcx, 4

	align 16
@@:
	movdqa		xmm0, [rsi+16*0]
	movdqa		xmm1, [rsi+16*1]
	movdqa		xmm2, [rsi+16*2]
	movdqa		xmm3, [rsi+16*3]

	punpck		qdq, 0, 2, 1, 3, 4, 6

	movdqa		[rdi], xmm0
	movdqa		[rdi+16], xmm2
	movdqa		[rdi+r8], xmm4
	movdqa		[rdi+r8+16], xmm6

	add			rsi, 64
	lea			rdi, [rdi+r8*2]

	dec			rcx
	jnz			@B

	pop			rdi
	pop			rsi

	ret

unSwizzleBlock32_amd64 endp

;
; unSwizzleBlock32_2 (TODO: test me)
;

unSwizzleBlock32_2_amd64 proc public

	push		rsi
	push		rdi

	mov			rsi, rcx
	mov			rdi, rdx
	mov			rcx, 2

	align 16
@@:
	movdqa		xmm0, [rsi+16*0]
	movdqa		xmm1, [rsi+16*1]
	movdqa		xmm2, [rsi+16*2]
	movdqa		xmm3, [rsi+16*3]
	movdqa		xmm4, [rsi+16*4]
	movdqa		xmm5, [rsi+16*5]
	movdqa		xmm6, [rsi+16*6]
	movdqa		xmm7, [rsi+16*7]

	punpck2		qdq, 0, 2, 4, 6, 1, 3, 5, 7, 8, 10, 12, 14

	movdqa		[rdi], xmm0
	movdqa		[rdi+16], xmm2
	movdqa		[rdi+r8], xmm4
	movdqa		[rdi+r8+16], xmm6
	lea			rdi, [rdi+r8*2]

	movdqa		[rdi], xmm8
	movdqa		[rdi+16], xmm10
	movdqa		[rdi+r8], xmm12
	movdqa		[rdi+r8+16], xmm14
	lea			rdi, [rdi+r8*2]

	add			rsi, 128

	dec			rcx
	jnz			@B

	pop			rdi
	pop			rsi

	ret

unSwizzleBlock32_2_amd64 endp

;
; unSwizzleBlock16
;

unSwizzleBlock16_amd64 proc public

	push		rsi
	push		rdi

	mov			rsi, rcx
	mov			rdi, rdx
	mov			rcx, 4

	align 16
@@:
	movdqa		xmm0, [rsi+16*0]
	movdqa		xmm1, [rsi+16*1]
	movdqa		xmm2, [rsi+16*2]
	movdqa		xmm3, [rsi+16*3]

	punpck		wd, 0, 2, 1, 3, 4, 6
	punpck		dq, 0, 4, 2, 6, 1, 3
	punpck		wd, 0, 4, 1, 3, 2, 6

	movdqa		[rdi], xmm0
	movdqa		[rdi+16], xmm2
	movdqa		[rdi+r8], xmm4
	movdqa		[rdi+r8+16], xmm6

	add			rsi, 64
	lea			rdi, [rdi+r8*2]

	dec			rcx
	jnz			@B

	pop			rdi
	pop			rsi

	ret
	
unSwizzleBlock16_amd64 endp

;
; unSwizzleBlock8
;

unSwizzleBlock8_amd64 proc public

	push		rsi
	push		rdi

	mov			rsi, rcx
	mov			rdi, rdx
	mov			rcx, 2
	
	; r9 = r8*3
	lea			r9, [r8*2]
	add			r9, r8

	align 16
@@:
	; col 0, 2
	
	movdqa		xmm0, [rsi+16*0]
	movdqa		xmm1, [rsi+16*1]
	movdqa		xmm4, [rsi+16*2]
	movdqa		xmm5, [rsi+16*3]
	
	; col 1, 3

	movdqa		xmm8, [rsi+16*4]
	movdqa		xmm9, [rsi+16*5]
	movdqa		xmm12, [rsi+16*6]
	movdqa		xmm13, [rsi+16*7]

	; col 0, 2
	
	punpck		bw,  0, 4, 1, 5, 2, 6
	punpck		wd,  0, 2, 4, 6, 1, 3
	punpck		bw,  0, 2, 1, 3, 4, 6
	punpck		qdq, 0, 2, 4, 6, 1, 3

	pshufd		xmm1, xmm1, 0b1h
	pshufd		xmm3, xmm3, 0b1h
	
	; col 1, 3

	punpck		bw,  8, 12, 9, 13, 10, 14
	punpck		wd,  8, 10, 12, 14, 9, 11
	punpck		bw,  8, 10, 9, 11, 12, 14
	punpck		qdq, 8, 10, 12, 14, 9, 11
	
	pshufd		xmm8, xmm8, 0b1h
	pshufd		xmm10, xmm10, 0b1h

	; col 0, 2
	
	movdqa		[rdi], xmm0
	movdqa		[rdi+r8], xmm2
	movdqa		[rdi+r8*2], xmm1
	movdqa		[rdi+r9], xmm3
	lea			rdi, [rdi+r8*4]

	; col 1, 3

	movdqa		[rdi], xmm8
	movdqa		[rdi+r8], xmm10
	movdqa		[rdi+r8*2], xmm9
	movdqa		[rdi+r9], xmm11
	lea			rdi, [rdi+r8*4]

	add			rsi, 128

	dec			rcx
	jnz			@B

	pop			rdi
	pop			rsi
	
	ret

unSwizzleBlock8_amd64 endp

;
; unSwizzleBlock4
;

unSwizzleBlock4_amd64 proc public

	push		rsi
	push		rdi

	mov			rsi, rcx
	mov			rdi, rdx
	mov			rcx, 2

	; r9 = r8*3
	lea			r9, [r8*2]
	add			r9, r8

	mov         eax, 0f0f0f0fh
	movd        xmm7, rax 
	pshufd      xmm7, xmm7, 0
	movdqa      xmm15, xmm7

	align 16
@@:
	; col 0, 2

	movdqa		xmm0, [rsi+16*0]
	movdqa		xmm1, [rsi+16*1]
	movdqa		xmm4, [rsi+16*2]
	movdqa		xmm3, [rsi+16*3]

	; col 1, 3

	movdqa		xmm8, [rsi+16*4]
	movdqa		xmm9, [rsi+16*5]
	movdqa		xmm12, [rsi+16*6]
	movdqa		xmm11, [rsi+16*7]

	; col 0, 2

	punpck		dq, 0, 4, 1, 3, 2, 6
	punpck		dq, 0, 2, 4, 6, 1, 3
	punpcknbl
	punpck		bw, 0, 2, 4, 6, 1, 3
	punpck		wd, 0, 2, 1, 3, 4, 6

	; col 1, 3

	punpck		dq, 8, 12, 9, 11, 10, 14
	punpck		dq, 8, 10, 12, 14, 9, 11
	punpcknbh
	punpck		bw, 8, 10, 12, 14, 9, 11
	punpck		wd, 8, 10, 9, 11, 12, 14

	; col 0, 2

	pshufd		xmm0, xmm0, 0d8h
	pshufd		xmm2, xmm2, 0d8h
	pshufd		xmm4, xmm4, 0d8h
	pshufd		xmm6, xmm6, 0d8h

	; col 1, 3

	pshufd		xmm8, xmm8, 0d8h
	pshufd		xmm10, xmm10, 0d8h
	pshufd		xmm12, xmm12, 0d8h
	pshufd		xmm14, xmm14, 0d8h

	; col 0, 2

	punpck		qdq, 0, 2, 4, 6, 1, 3

	; col 1, 3

	punpck		qdq, 8, 10, 12, 14, 9, 11

	; col 0, 2

	pshuflw		xmm1, xmm1, 0b1h
	pshuflw		xmm3, xmm3, 0b1h
	pshufhw		xmm1, xmm1, 0b1h
	pshufhw		xmm3, xmm3, 0b1h

	; col 1, 3

	pshuflw		xmm8, xmm8, 0b1h
	pshuflw		xmm10, xmm10, 0b1h
	pshufhw		xmm8, xmm8, 0b1h
	pshufhw		xmm10, xmm10, 0b1h

	; col 0, 2

	movdqa		[rdi], xmm0
	movdqa		[rdi+r8], xmm2
	movdqa		[rdi+r8*2], xmm1
	movdqa		[rdi+r9], xmm3
	lea			rdi, [rdi+r8*4]

	; col 1, 3

	movdqa		[rdi], xmm8
	movdqa		[rdi+r8], xmm10
	movdqa		[rdi+r8*2], xmm9
	movdqa		[rdi+r9], xmm11
	lea			rdi, [rdi+r8*4]

	add			rsi, 128

	dec			rcx
	jnz			@B

	pop			rdi
	pop			rsi

	ret

unSwizzleBlock4_amd64 endp

;
; unSwizzleBlock8HP
;

unSwizzleBlock8HP_amd64 proc public

	push		rsi
	push		rdi

	mov			rsi, rcx
	mov			rdi, rdx
	mov			rcx, 4

	align 16
@@:
	movdqa		xmm0, [rsi+16*0]
	movdqa		xmm1, [rsi+16*1]
	movdqa		xmm2, [rsi+16*2]
	movdqa		xmm3, [rsi+16*3]

	punpck		qdq, 0, 2, 1, 3, 4, 6

	psrld		xmm0, 24
	psrld		xmm2, 24
	psrld		xmm4, 24
	psrld		xmm6, 24
	
	packssdw	xmm0, xmm2
	packssdw	xmm4, xmm6
	packuswb	xmm0, xmm4

	movlps		qword ptr [rdi], xmm0
	movhps		qword ptr [rdi+r8], xmm0

	add			rsi, 64
	lea			rdi, [rdi+r8*2]

	dec			rcx
	jnz			@B

	pop			rdi
	pop			rsi
	
	ret

unSwizzleBlock8HP_amd64 endp

;
; unSwizzleBlock4HLP
;

unSwizzleBlock4HLP_amd64 proc public

	push		rsi
	push		rdi

	mov			rsi, rcx
	mov			rdi, rdx
	mov			rcx, 4

	mov         eax, 0f0f0f0fh
	movd        xmm7, eax 
	pshufd      xmm7, xmm7, 0

	align 16
@@:
	movdqa		xmm0, [rsi+16*0]
	movdqa		xmm1, [rsi+16*1]
	movdqa		xmm2, [rsi+16*2]
	movdqa		xmm3, [rsi+16*3]

	punpck		qdq, 0, 2, 1, 3, 4, 6

	psrld		xmm0, 24
	psrld		xmm2, 24
	psrld		xmm4, 24
	psrld		xmm6, 24
	
	packssdw	xmm0, xmm2
	packssdw	xmm4, xmm6
	packuswb	xmm0, xmm4

	pand		xmm0, xmm7
	
	movlps		qword ptr [rdi], xmm0
	movhps		qword ptr [rdi+r8], xmm0

	add			rsi, 64
	lea			rdi, [rdi+r8*2]

	dec			rcx
	jnz			@B

	pop			rdi
	pop			rsi
	
	ret
	
unSwizzleBlock4HLP_amd64 endp

;
; unSwizzleBlock4HHP
;

unSwizzleBlock4HHP_amd64 proc public

	push		rsi
	push		rdi

	mov			rsi, rcx
	mov			rdi, rdx
	mov			rcx, 4

	align 16
@@:
	movdqa		xmm0, [rsi+16*0]
	movdqa		xmm1, [rsi+16*1]
	movdqa		xmm2, [rsi+16*2]
	movdqa		xmm3, [rsi+16*3]

	punpck		qdq, 0, 2, 1, 3, 4, 6

	psrld		xmm0, 28
	psrld		xmm2, 28
	psrld		xmm4, 28
	psrld		xmm6, 28
	
	packssdw	xmm0, xmm2
	packssdw	xmm4, xmm6
	packuswb	xmm0, xmm4

	movlps		qword ptr [rdi], xmm0
	movhps		qword ptr [rdi+r8], xmm0

	add			rsi, 64
	lea			rdi, [rdi+r8*2]

	dec			rcx
	jnz			@B

	pop			rdi
	pop			rsi
	
	ret
	
unSwizzleBlock4HHP_amd64 endp

;
; unSwizzleBlock4P
;

unSwizzleBlock4P_amd64 proc public

	mov         eax, 0f0f0f0fh
	movd        xmm8, eax 
	pshufd      xmm8, xmm8, 0

	; r9 = r8*3
	lea			r9, [r8*2]
	add			r9, r8

	; col 0
	
	movdqa		xmm0, [rcx+16*0]
	movdqa		xmm1, [rcx+16*1]
	movdqa		xmm2, [rcx+16*2]
	movdqa		xmm3, [rcx+16*3]

	punpck		bw, 0, 2, 1, 3, 4, 6
	punpck		wd, 0, 4, 2, 6, 1, 3
	punpck		bw, 0, 4, 1, 3, 2, 6

	movdqa		xmm1, xmm8
	pandn		xmm1, xmm0
	pand		xmm0, xmm8
	pshufd		xmm1, xmm1, 0b1h
	psrlq		xmm1, 4

	movdqa		xmm3, xmm8
	pandn		xmm3, xmm2
	pand		xmm2, xmm8
	pshufd		xmm3, xmm3, 0b1h
	psrlq		xmm3, 4
	
	movdqa		xmm5, xmm8
	pandn		xmm5, xmm4
	pand		xmm4, xmm8
	pshufd		xmm5, xmm5, 0b1h
	psrlq		xmm5, 4

	movdqa		xmm7, xmm8
	pandn		xmm7, xmm6
	pand		xmm6, xmm8
	pshufd		xmm7, xmm7, 0b1h
	psrlq		xmm7, 4

	movdqa		[rdx], xmm0
	movdqa		[rdx+16], xmm2
	movdqa		[rdx+r8], xmm4
	movdqa		[rdx+r8+16], xmm6
		
	movdqa		[rdx+r8*2], xmm1
	movdqa		[rdx+r8*2+16], xmm3
	movdqa		[rdx+r9], xmm5
	movdqa		[rdx+r9+16], xmm7
	
	lea			rdx, [rdx+r8*4]

	; col 1
	
	movdqa		xmm0, [rcx+16*4]
	movdqa		xmm1, [rcx+16*5]
	movdqa		xmm2, [rcx+16*6]
	movdqa		xmm3, [rcx+16*7]

	punpck		bw, 0, 2, 1, 3, 4, 6
	punpck		wd, 0, 4, 2, 6, 1, 3
	punpck		bw, 0, 4, 1, 3, 2, 6

	movdqa		xmm1, xmm8
	pandn		xmm1, xmm0
	pand		xmm0, xmm8
	pshufd		xmm0, xmm0, 0b1h
	psrlq		xmm1, 4

	movdqa		xmm3, xmm8
	pandn		xmm3, xmm2
	pand		xmm2, xmm8
	pshufd		xmm2, xmm2, 0b1h
	psrlq		xmm3, 4
	
	movdqa		xmm5, xmm8
	pandn		xmm5, xmm4
	pand		xmm4, xmm8
	pshufd		xmm4, xmm4, 0b1h
	psrlq		xmm5, 4

	movdqa		xmm7, xmm8
	pandn		xmm7, xmm6
	pand		xmm6, xmm8
	pshufd		xmm6, xmm6, 0b1h
	psrlq		xmm7, 4

	movdqa		[rdx], xmm0
	movdqa		[rdx+16], xmm2
	movdqa		[rdx+r8], xmm4
	movdqa		[rdx+r8+16], xmm6
		
	movdqa		[rdx+r8*2], xmm1
	movdqa		[rdx+r8*2+16], xmm3
	movdqa		[rdx+r9], xmm5
	movdqa		[rdx+r9+16], xmm7
	
	lea			rdx, [rdx+r8*4]

	; col 2
	
	movdqa		xmm0, [rcx+16*8]
	movdqa		xmm1, [rcx+16*9]
	movdqa		xmm2, [rcx+16*10]
	movdqa		xmm3, [rcx+16*11]

	punpck		bw, 0, 2, 1, 3, 4, 6
	punpck		wd, 0, 4, 2, 6, 1, 3
	punpck		bw, 0, 4, 1, 3, 2, 6

	movdqa		xmm1, xmm8
	pandn		xmm1, xmm0
	pand		xmm0, xmm8
	pshufd		xmm1, xmm1, 0b1h
	psrlq		xmm1, 4

	movdqa		xmm3, xmm8
	pandn		xmm3, xmm2
	pand		xmm2, xmm8
	pshufd		xmm3, xmm3, 0b1h
	psrlq		xmm3, 4
	
	movdqa		xmm5, xmm8
	pandn		xmm5, xmm4
	pand		xmm4, xmm8
	pshufd		xmm5, xmm5, 0b1h
	psrlq		xmm5, 4

	movdqa		xmm7, xmm8
	pandn		xmm7, xmm6
	pand		xmm6, xmm8
	pshufd		xmm7, xmm7, 0b1h
	psrlq		xmm7, 4

	movdqa		[rdx], xmm0
	movdqa		[rdx+16], xmm2
	movdqa		[rdx+r8], xmm4
	movdqa		[rdx+r8+16], xmm6
		
	movdqa		[rdx+r8*2], xmm1
	movdqa		[rdx+r8*2+16], xmm3
	movdqa		[rdx+r9], xmm5
	movdqa		[rdx+r9+16], xmm7
	
	lea			rdx, [rdx+r8*4]

	; col 3
	
	movdqa		xmm0, [rcx+16*12]
	movdqa		xmm1, [rcx+16*13]
	movdqa		xmm2, [rcx+16*14]
	movdqa		xmm3, [rcx+16*15]

	punpck		bw, 0, 2, 1, 3, 4, 6
	punpck		wd, 0, 4, 2, 6, 1, 3
	punpck		bw, 0, 4, 1, 3, 2, 6

	movdqa		xmm1, xmm8
	pandn		xmm1, xmm0
	pand		xmm0, xmm8
	pshufd		xmm0, xmm0, 0b1h
	psrlq		xmm1, 4

	movdqa		xmm3, xmm8
	pandn		xmm3, xmm2
	pand		xmm2, xmm8
	pshufd		xmm2, xmm2, 0b1h
	psrlq		xmm3, 4
	
	movdqa		xmm5, xmm8
	pandn		xmm5, xmm4
	pand		xmm4, xmm8
	pshufd		xmm4, xmm4, 0b1h
	psrlq		xmm5, 4

	movdqa		xmm7, xmm8
	pandn		xmm7, xmm6
	pand		xmm6, xmm8
	pshufd		xmm6, xmm6, 0b1h
	psrlq		xmm7, 4

	movdqa		[rdx], xmm0
	movdqa		[rdx+16], xmm2
	movdqa		[rdx+r8], xmm4
	movdqa		[rdx+r8+16], xmm6
		
	movdqa		[rdx+r8*2], xmm1
	movdqa		[rdx+r8*2+16], xmm3
	movdqa		[rdx+r9], xmm5
	movdqa		[rdx+r9+16], xmm7
	
	; lea			rdx, [rdx+r8*4]

	ret

unSwizzleBlock4P_amd64 endp

;
; swizzling
;

;
; SwizzleBlock32_amd64
;

SwizzleBlock32_amd64 proc public

	push		rsi
	push		rdi

	mov			rdi, rcx
	mov			rsi, rdx
	mov			rcx, 4

	cmp			r9d, 0ffffffffh
	jnz			SwizzleBlock32_amd64@WM

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

SwizzleBlock32_amd64@WM:

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
	
SwizzleBlock32_amd64 endp

;
; SwizzleBlock16_amd64
;

SwizzleBlock16_amd64 proc public

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
	
SwizzleBlock16_amd64 endp

;
; SwizzleBlock8
;

SwizzleBlock8_amd64 proc public

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

SwizzleBlock8_amd64 endp

;
; SwizzleBlock4
;

SwizzleBlock4_amd64 proc public

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

SwizzleBlock4_amd64 endp

;
; swizzling with unaligned reads
;

;
; SwizzleBlock32u_amd64
;

SwizzleBlock32u_amd64 proc public

	push		rsi
	push		rdi

	mov			rdi, rcx
	mov			rsi, rdx
	mov			rcx, 4

	cmp			r9d, 0ffffffffh
	jnz			SwizzleBlock32u_amd64@WM

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

SwizzleBlock32u_amd64@WM:

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
	
SwizzleBlock32u_amd64 endp

;
; SwizzleBlock16u_amd64
;

SwizzleBlock16u_amd64 proc public

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
	
SwizzleBlock16u_amd64 endp

;
; SwizzleBlock8u
;

SwizzleBlock8u_amd64 proc public

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

SwizzleBlock8u_amd64 endp

;
; SwizzleBlock4u
;

SwizzleBlock4u_amd64 proc public

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

SwizzleBlock4u_amd64 endp

	end
	