; Pcsx2 - Pc Ps2 Emulator
; Copyright (C) 2002-2008  Pcsx2 Team
;
; This program is free software; you can redistribute it and/or modify
; it under the terms of the GNU General Public License as published by
; the Free Software Foundation; either version 2 of the License, or
; (at your option) any later version.

; This program is distributed in the hope that it will be useful,
; but WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
; GNU General Public License for more details.
;
; You should have received a copy of the GNU General Public License
; along with this program; if not, write to the Free Software
; Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA

;; Fast assembly routines for x86-64 masm compiler
;; zerofrog(@gmail.com)
.code

;; mmx memcmp implementation, size has to be a multiple of 8
;; returns 0 is equal, nonzero value if not equal
;; ~10 times faster than standard memcmp
;; (zerofrog)
;; u8 memcmp_mmx(const void* src1, const void* src2, int cmpsize)
memcmp_mmx proc public

        cmp r8d, 32
		jl memcmp_Done4

		;; custom test first 8 to make sure things are ok
		movq mm0, [rdx]
		movq mm1, [rdx+8]
		pcmpeqd mm0, [rcx]
		pcmpeqd mm1, [rcx+8]
		pand mm0, mm1
		movq mm2, [rdx+16]
		pmovmskb eax, mm0
		movq mm3, [rdx+24]

		;; check if eq
		cmp eax, 0ffh
		je memcmp_NextComp
		mov eax, 1
		jmp memcmp_End

memcmp_NextComp:
		pcmpeqd mm2, [rcx+16]
		pcmpeqd mm3, [rcx+24]
		pand mm2, mm3
		pmovmskb eax, mm2

		sub r8d, 32
		add rdx, 32
		add rcx, 32

		;; check if eq
		cmp eax, 0ffh
		je memcmp_ContinueTest
		mov eax, 1
		jmp memcmp_End

		cmp r8d, 64
		jl memcmp_Done8

memcmp_Cmp8:
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
		
		;; check if eq
		cmp eax, 0ffh
		je memcmp_Continue
		mov eax, 1
		jmp memcmp_End

memcmp_Continue:
		sub r8d, 64
		add rdx, 64
		add rcx, 64
memcmp_ContinueTest:
		cmp r8d, 64
		jge memcmp_Cmp8

memcmp_Done8:
		test r8d, 020h
		jz memcmp_Done4
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

		;; check if eq
		cmp eax, 0ffh
		je memcmp_Done4
		mov eax, 1
		jmp memcmp_End

memcmp_Done4:
		cmp r8d, 24
		jne memcmp_Done2
		movq mm0, [rdx]
		movq mm1, [rdx+8]
		movq mm2, [rdx+16]
		pcmpeqd mm0, [rcx]
		pcmpeqd mm1, [rcx+8]
		pcmpeqd mm2, [rcx+16]
		pand mm0, mm1
		pand mm0, mm2
		pmovmskb eax, mm0

		;; check if eq
		cmp eax, 0ffh
        je memcmp_Done
		mov eax, 1
		jmp memcmp_End

memcmp_Done2:
		cmp r8d, 16
		jne memcmp_Done1

		movq mm0, [rdx]
		movq mm1, [rdx+8]
		pcmpeqd mm0, [rcx]
		pcmpeqd mm1, [rcx+8]
		pand mm0, mm1
		pmovmskb eax, mm0

		;; check if eq
		cmp eax, 0ffh
        je memcmp_Done
		mov eax, 1
		jmp memcmp_End

memcmp_Done1:
		cmp r8d, 8
		jne memcmp_Done

		mov eax, [rdx]
		mov rdx, [rdx+4]
		cmp eax, [rcx]
		je memcmp_Next
		mov eax, 1
		jmp memcmp_End

memcmp_Next:
		cmp rdx, [rcx+4]
        je memcmp_Done
		mov eax, 1
		jmp memcmp_End

memcmp_Done:
		xor eax, eax

memcmp_End:
		emms
        ret
memcmp_mmx endp
        
;; memxor_mmx
memxor_mmx proc public

	cmp r8d, 64
	jl memxor_Setup4

	movq mm0, [rdx]
	movq mm1, [rdx+8]
	movq mm2, [rdx+16]
	movq mm3, [rdx+24]
	movq mm4, [rdx+32]
	movq mm5, [rdx+40]
	movq mm6, [rdx+48]
	movq mm7, [rdx+56]
	sub r8d, 64
	add rdx, 64
	cmp r8d, 64
	jl memxor_End8

memxor_Cmp8:
	pxor mm0, [rdx]
	pxor mm1, [rdx+8]
	pxor mm2, [rdx+16]
	pxor mm3, [rdx+24]
	pxor mm4, [rdx+32]
	pxor mm5, [rdx+40]
	pxor mm6, [rdx+48]
	pxor mm7, [rdx+56]

	sub r8d, 64
	add rdx, 64
	cmp r8d, 64
	jge memxor_Cmp8

memxor_End8:
	pxor mm0, mm4
	pxor mm1, mm5
	pxor mm2, mm6
	pxor mm3, mm7

	cmp r8d, 32
	jl memxor_End4
	pxor mm0, [rdx]
	pxor mm1, [rdx+8]
	pxor mm2, [rdx+16]
	pxor mm3, [rdx+24]
	sub r8d, 32
	add rdx, 32
	jmp memxor_End4

memxor_Setup4:
	cmp r8d, 32
	jl memxor_Setup2

	movq mm0, [rdx]
	movq mm1, [rdx+8]
	movq mm2, [rdx+16]
	movq mm3, [rdx+24]
	sub r8d, 32
	add rdx, 32

memxor_End4:
	pxor mm0, mm2
	pxor mm1, mm3

	cmp r8d, 16
	jl memxor_End2
	pxor mm0, [rdx]
	pxor mm1, [rdx+8]
	sub r8d, 16
	add rdx, 16
	jmp memxor_End2

memxor_Setup2:
	cmp r8d, 16
	jl memxor_Setup1

	movq mm0, [rdx]
	movq mm1, [rdx+8]
	sub r8d, 16
	add rdx, 16

memxor_End2:
	pxor mm0, mm1

	cmp r8d, 8
	jl memxor_End1
	pxor mm0, [rdx]
memxor_End1:
	movq [rcx], mm0
	jmp memxor_End

memxor_Setup1:
	movq mm0, [rdx]
	movq [rcx], mm0
memxor_End:
	emms
	ret

memxor_mmx endp

end