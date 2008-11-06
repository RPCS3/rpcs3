; i386.asm  -  description
; -------------------
; begin                : Sun Nov 08 2001
; copyright            : (C) 2001 by Pete Bernert
; email                : BlackDove@addcom.de

; This program is free software; you can redistribute it and/or modify  *
; it under the terms of the GNU General Public License as published by  *
; the Free Software Foundation; either version 2 of the License, or     *
; (at your option) any later version. See also the license.txt file for *
; additional informations.                                              *


bits 32

section .text

%include "macros.inc"

;-----------------------------------------------------------------
NEWSYM i386_reOrder
	push ebp
	mov ebp, esp

    mov eax, [ebp+8]
    bswap eax

	mov esp, ebp
	pop ebp
	ret


