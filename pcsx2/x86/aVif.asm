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

;; Fast VIF assembly routines for UNPACK zerofrog(@gmail.com)
;; NOTE: This file is used to build aVif_proc-[32/64].asm because ml has a very
;; weak preprocessor. To generate the files, install nasm and run the following command:
;; aVif_proc-32.asm: nasmw -e aVif.asm > aVif_proc-32.asm
;; aVif_proc-64.asm: nasmw -e -D__x86_64__ aVif.asm > aVif_proc-64.asm
;; once the files are built, remove all lines starting with %line
;; and remove the brackets from the exports

%ifndef __x86_64__
.686
.model flat, c
.mmx
.xmm
%endif

extern _vifRegs:abs
extern _vifMaskRegs:abs
extern _vifRow:abs
extern _vifCol:abs
extern s_TempDecompress:abs


.code


%ifdef __x86_64__
%define VIF_ESP rsp
%define VIF_SRC	rdx
%define VIF_INC	rdi
%define VIF_DST rcx
%define VIF_SIZE r8d
%define VIF_TMPADDR rax
%define VIF_SAVEEBX r9
%define VIF_SAVEEBXd r9d
%else
%define VIF_ESP esp
%define VIF_SRC esi
%define VIF_INC	ecx
%define VIF_DST edi
%define VIF_SIZE edx
%define VIF_TMPADDR eax
%define VIF_SAVEEBX ebx
%define VIF_SAVEEBXd ebx
%endif

%define XMM_R0			xmm0
%define XMM_R1			xmm1
%define XMM_R2			xmm2
%define XMM_WRITEMASK	xmm3
%define XMM_ROWMASK		xmm4
%define XMM_ROWCOLMASK	xmm5
%define XMM_ROW			xmm6
%define XMM_COL			xmm7
%define XMM_R3			XMM_COL

;; writing masks
UNPACK_Write0_Regular macro r0, CL, DEST_OFFSET, MOVDQA
	MOVDQA [VIF_DST+DEST_OFFSET], r0
	endm

UNPACK_Write1_Regular macro r0, CL, DEST_OFFSET, MOVDQA
	MOVDQA [VIF_DST], r0
	add VIF_DST, VIF_INC
	endm

UNPACK_Write0_Mask macro r0, CL, DEST_OFFSET, MOVDQA
	UNPACK_Write0_Regular r0, CL, DEST_OFFSET, MOVDQA
	endm
	
UNPACK_Write1_Mask macro r0, CL, DEST_OFFSET, MOVDQA
	UNPACK_Write1_Regular r0, CL, DEST_OFFSET, MOVDQA
	endm
	
;; masked write (dest needs to be in edi)
UNPACK_Write0_WriteMask macro r0, CL, DEST_OFFSET, MOVDQA
	;; masked write (dest needs to be in edi)
	movdqa XMM_WRITEMASK, [VIF_TMPADDR + 64*(CL) + 48]
	pand r0, XMM_WRITEMASK
	pandn XMM_WRITEMASK, [VIF_DST]
	por r0, XMM_WRITEMASK
	MOVDQA [VIF_DST], r0
	add VIF_DST, 16
	endm

;; masked write (dest needs to be in edi)
UNPACK_Write1_WriteMask macro r0, CL, DEST_OFFSET, MOVDQA
	;; masked write (dest needs to be in edi)
	movdqa XMM_WRITEMASK, [VIF_TMPADDR + 64*(0) + 48]
	pand r0, XMM_WRITEMASK
	pandn XMM_WRITEMASK, [VIF_DST]
	por r0, XMM_WRITEMASK
	MOVDQA [VIF_DST], r0
	add VIF_DST, VIF_INC
	endm

UNPACK_Mask_SSE_0 macro r0
	pand r0, XMM_WRITEMASK
	por r0, XMM_ROWCOLMASK
	endm

;; once a qword is uncomprssed, applies masks and saves
;; note: modifying XMM_WRITEMASK
;; dest = row + write (only when mask=0), otherwise write
UNPACK_Mask_SSE_1 macro r0
	;; dest = row + write (only when mask=0), otherwise write
	pand r0, XMM_WRITEMASK
	por r0, XMM_ROWCOLMASK
	pand XMM_WRITEMASK, XMM_ROW
	paddd r0, XMM_WRITEMASK
	endm

;; dest = row + write (only when mask=0), otherwise write
;; row = row + write (only when mask = 0), otherwise row
UNPACK_Mask_SSE_2 macro r0
	;; dest = row + write (only when mask=0), otherwise write
	;; row = row + write (only when mask = 0), otherwise row
	pand r0, XMM_WRITEMASK
	pand XMM_WRITEMASK, XMM_ROW
	paddd XMM_ROW, r0
	por r0, XMM_ROWCOLMASK
	paddd r0, XMM_WRITEMASK
	endm

UNPACK_WriteMask_SSE_0 macro r0
	UNPACK_Mask_SSE_0 r0
	endm
UNPACK_WriteMask_SSE_1 macro r0
	UNPACK_Mask_SSE_1 r0
	endm
UNPACK_WriteMask_SSE_2 macro r0
	UNPACK_Mask_SSE_2 r0
	endm

UNPACK_Regular_SSE_0 macro r0
	endm

UNPACK_Regular_SSE_1 macro r0
	paddd r0, XMM_ROW
	endm

UNPACK_Regular_SSE_2 macro r0
	paddd r0, XMM_ROW
	movdqa XMM_ROW, r0
	endm

;; setting up masks
UNPACK_Setup_Mask_SSE macro CL
	mov VIF_TMPADDR, [_vifMaskRegs]
	movdqa XMM_ROWMASK, [VIF_TMPADDR + 64*(CL) + 16]
	movdqa XMM_ROWCOLMASK, [VIF_TMPADDR + 64*(CL) + 32]
	movdqa XMM_WRITEMASK, [VIF_TMPADDR + 64*(CL)]
	pand XMM_ROWMASK, XMM_ROW
	pand XMM_ROWCOLMASK, XMM_COL
	por XMM_ROWCOLMASK, XMM_ROWMASK
	endm

UNPACK_Start_Setup_Mask_SSE_0 macro CL
	UNPACK_Setup_Mask_SSE CL
	endm
	
UNPACK_Start_Setup_Mask_SSE_1 macro CL
	mov VIF_TMPADDR, [_vifMaskRegs]
	movdqa XMM_ROWMASK, [VIF_TMPADDR + 64*(CL) + 16]
	movdqa XMM_ROWCOLMASK, [VIF_TMPADDR + 64*(CL) + 32]
	pand XMM_ROWMASK, XMM_ROW
	pand XMM_ROWCOLMASK, XMM_COL
	por XMM_ROWCOLMASK, XMM_ROWMASK
	endm

UNPACK_Start_Setup_Mask_SSE_2 macro CL
	endm

UNPACK_Setup_Mask_SSE_0_1 macro CL
	endm
UNPACK_Setup_Mask_SSE_1_1 macro CL
	mov VIF_TMPADDR, [_vifMaskRegs]
	movdqa XMM_WRITEMASK, [VIF_TMPADDR + 64*(0)]
	endm

;; ignore CL, since vif.cycle.wl == 1
UNPACK_Setup_Mask_SSE_2_1 macro CL
	;; ignore CL, since vif.cycle.wl == 1
	mov VIF_TMPADDR, [_vifMaskRegs]
	movdqa XMM_ROWMASK, [VIF_TMPADDR + 64*(0) + 16]
	movdqa XMM_ROWCOLMASK, [VIF_TMPADDR + 64*(0) + 32]
	movdqa XMM_WRITEMASK, [VIF_TMPADDR + 64*(0)]
	pand XMM_ROWMASK, XMM_ROW
	pand XMM_ROWCOLMASK, XMM_COL
	por XMM_ROWCOLMASK, XMM_ROWMASK
	endm

UNPACK_Setup_Mask_SSE_0_0 macro CL
	UNPACK_Setup_Mask_SSE CL
	endm
UNPACK_Setup_Mask_SSE_1_0 macro CL
	UNPACK_Setup_Mask_SSE CL
	endm
UNPACK_Setup_Mask_SSE_2_0 macro CL
	UNPACK_Setup_Mask_SSE CL
	endm

;; write mask always destroys XMM_WRITEMASK, so 0_0 = 1_0
UNPACK_Setup_WriteMask_SSE_0_0 macro CL
    UNPACK_Setup_Mask_SSE CL
    endm
UNPACK_Setup_WriteMask_SSE_1_0 macro CL
    UNPACK_Setup_Mask_SSE CL
    endm
UNPACK_Setup_WriteMask_SSE_2_0 macro CL
    UNPACK_Setup_Mask_SSE CL
    endm
UNPACK_Setup_WriteMask_SSE_0_1 macro CL
	UNPACK_Setup_Mask_SSE_1_1 CL
    endm
    
UNPACK_Setup_WriteMask_SSE_1_1 macro CL
    UNPACK_Setup_Mask_SSE_1_1 CL
    endm
    
UNPACK_Setup_WriteMask_SSE_2_1 macro CL
    UNPACK_Setup_Mask_SSE_2_1 CL
    endm

UNPACK_Start_Setup_WriteMask_SSE_0 macro CL
    UNPACK_Start_Setup_Mask_SSE_1 CL
    endm
UNPACK_Start_Setup_WriteMask_SSE_1 macro CL
    UNPACK_Start_Setup_Mask_SSE_1 CL
    endm
UNPACK_Start_Setup_WriteMask_SSE_2 macro CL
    UNPACK_Start_Setup_Mask_SSE_2 CL
    endm

UNPACK_Start_Setup_Regular_SSE_0 macro CL
	endm
UNPACK_Start_Setup_Regular_SSE_1 macro CL
    endm
UNPACK_Start_Setup_Regular_SSE_2 macro CL
    endm
UNPACK_Setup_Regular_SSE_0_0 macro CL
	endm
UNPACK_Setup_Regular_SSE_1_0 macro CL
	endm 
UNPACK_Setup_Regular_SSE_2_0 macro CL
    endm
UNPACK_Setup_Regular_SSE_0_1 macro CL
    endm
UNPACK_Setup_Regular_SSE_1_1 macro CL
    endm
UNPACK_Setup_Regular_SSE_2_1 macro CL
    endm

UNPACK_INC_DST_0_Regular macro qw
	add VIF_DST, (16*qw)
	endm
UNPACK_INC_DST_1_Regular macro qw
	endm
UNPACK_INC_DST_0_Mask macro qw
	add VIF_DST, (16*qw)
	endm
UNPACK_INC_DST_1_Mask macro qw
	endm
UNPACK_INC_DST_0_WriteMask macro qw
	endm
UNPACK_INC_DST_1_WriteMask macro qw
	endm
	
;; unpacks for 1,2,3,4 elements (V3 uses this directly)
UNPACK4_SSE macro CL, TOTALCL, MaskType, ModeType
	@CatStr(UNPACK_Setup_, MaskType, _SSE_, ModeType, _, TOTALCL) CL+0
	@CatStr(UNPACK_, MaskType, _SSE_, ModeType) XMM_R0
	@CatStr(UNPACK_Write, TOTALCL, _, MaskType) XMM_R0, CL, 0, movdqa
	
	@CatStr(UNPACK_Setup_, MaskType, _SSE_, ModeType, _, TOTALCL) CL+1
	@CatStr(UNPACK_, MaskType, _SSE_, ModeType) XMM_R1
	@CatStr(UNPACK_Write, TOTALCL, _, MaskType) XMM_R1, CL+1, 16, movdqa
	
	@CatStr(UNPACK_Setup_, MaskType, _SSE_, ModeType, _, TOTALCL) CL+2
	@CatStr(UNPACK_, MaskType, _SSE_, ModeType) XMM_R2
	@CatStr(UNPACK_Write, TOTALCL, _, MaskType) XMM_R2, CL+2, 32, movdqa
	
	@CatStr(UNPACK_Setup_, MaskType, _SSE_, ModeType, _, TOTALCL) CL+3
	@CatStr(UNPACK_, MaskType, _SSE_, ModeType) XMM_R3
	@CatStr(UNPACK_Write, TOTALCL, _, MaskType) XMM_R3, CL+3, 48, movdqa
	
	@CatStr(UNPACK_INC_DST_, TOTALCL, _, MaskType) 4
	endm
	
;; V3 uses this directly
UNPACK3_SSE macro CL, TOTALCL, MaskType, ModeType
	@CatStr(UNPACK_Setup_, MaskType, _SSE_, ModeType, _, TOTALCL) CL
	@CatStr(UNPACK_, MaskType, _SSE_, ModeType) XMM_R0
	@CatStr(UNPACK_Write, TOTALCL, _, MaskType) XMM_R0, CL, 0, movdqa
	
	@CatStr(UNPACK_Setup_, MaskType, _SSE_, ModeType, _, TOTALCL) CL+1
	@CatStr(UNPACK_, MaskType, _SSE_, ModeType) XMM_R1
	@CatStr(UNPACK_Write, TOTALCL, _, MaskType) XMM_R1, CL+1, 16, movdqa
	
	@CatStr(UNPACK_Setup_, MaskType, _SSE_, ModeType, _, TOTALCL) CL+2
	@CatStr(UNPACK_, MaskType, _SSE_, ModeType) XMM_R2
	@CatStr(UNPACK_Write, TOTALCL, _, MaskType) XMM_R2, CL+2, 32, movdqa
	
	@CatStr(UNPACK_INC_DST_, TOTALCL, _, MaskType) 3
	endm

UNPACK2_SSE macro CL, TOTALCL, MaskType, ModeType
	@CatStr(UNPACK_Setup_, MaskType, _SSE_, ModeType, _, TOTALCL) CL
	@CatStr(UNPACK_, MaskType, _SSE_, ModeType) XMM_R0
	@CatStr(UNPACK_Write, TOTALCL, _, MaskType) XMM_R0, CL, 0, movdqa
	
	@CatStr(UNPACK_Setup_, MaskType, _SSE_, ModeType, _, TOTALCL) CL+1
	@CatStr(UNPACK_, MaskType, _SSE_, ModeType) XMM_R1
	@CatStr(UNPACK_Write, TOTALCL, _, MaskType) XMM_R1, CL+1, 16, movdqa
	
	@CatStr(UNPACK_INC_DST_, TOTALCL, _, MaskType) 2
	endm

UNPACK1_SSE macro CL, TOTALCL, MaskType, ModeType
	@CatStr(UNPACK_Setup_, MaskType, _SSE_, ModeType, _, TOTALCL) CL
	@CatStr(UNPACK_, MaskType, _SSE_, ModeType) XMM_R0
	@CatStr(UNPACK_Write, TOTALCL, _, MaskType) XMM_R0, CL, 0, movdqa
	
	@CatStr(UNPACK_INC_DST_, TOTALCL, _, MaskType) 1
	endm

;; S-32
;; only when cl==1
UNPACK_S_32SSE_4x macro CL, TOTALCL, MaskType, ModeType, MOVDQA
	MOVDQA XMM_R3, [VIF_SRC]
	
	pshufd XMM_R0, XMM_R3, 0
	pshufd XMM_R1, XMM_R3, 055h
	pshufd XMM_R2, XMM_R3, 0aah
	pshufd XMM_R3, XMM_R3, 0ffh
	
	UNPACK4_SSE CL, TOTALCL, MaskType, ModeType
	
	add VIF_SRC, 16
	endm

UNPACK_S_32SSE_4A macro CL, TOTALCL, MaskType, ModeType
	UNPACK_S_32SSE_4x CL, TOTALCL, MaskType, ModeType, movdqa
	endm
UNPACK_S_32SSE_4 macro CL, TOTALCL, MaskType, ModeType
	UNPACK_S_32SSE_4x CL, TOTALCL, MaskType, ModeType, movdqu
	endm

UNPACK_S_32SSE_3x macro CL, TOTALCL, MaskType, ModeType, MOVDQA 
	MOVDQA XMM_R2, [VIF_SRC]  
	
	pshufd XMM_R0, XMM_R2, 0  
	pshufd XMM_R1, XMM_R2, 055h
	pshufd XMM_R2, XMM_R2, 0aah 
	 
	UNPACK3_SSE CL, TOTALCL, MaskType, ModeType 
	 
	add VIF_SRC, 12  
	endm
	
UNPACK_S_32SSE_3A macro CL, TOTALCL, MaskType, ModeType
	UNPACK_S_32SSE_3x CL, TOTALCL, MaskType, ModeType, movdqa
	endm
UNPACK_S_32SSE_3 macro CL, TOTALCL, MaskType, ModeType
	UNPACK_S_32SSE_3x CL, TOTALCL, MaskType, ModeType, movdqu
	endm

UNPACK_S_32SSE_2 macro CL, TOTALCL, MaskType, ModeType 
	movq XMM_R1, QWORD PTR [VIF_SRC]
	
	pshufd XMM_R0, XMM_R1, 0
	pshufd XMM_R1, XMM_R1, 055h
	
	UNPACK2_SSE CL, TOTALCL, MaskType, ModeType
	
	add VIF_SRC, 8
	endm

UNPACK_S_32SSE_2A macro CL, TOTALCL, MaskType, ModeType 
	UNPACK_S_32SSE_2 CL, TOTALCL, MaskType, ModeType 
	endm

UNPACK_S_32SSE_1 macro CL, TOTALCL, MaskType, ModeType 
	movd XMM_R0, dword ptr [VIF_SRC]
	pshufd XMM_R0, XMM_R0, 0
	
	UNPACK1_SSE CL, TOTALCL, MaskType, ModeType
	
	add VIF_SRC, 4
	endm

UNPACK_S_32SSE_1A macro CL, TOTALCL, MaskType, ModeType 
	UNPACK_S_32SSE_1 CL, TOTALCL, MaskType, ModeType 
	endm

;; S-16
UNPACK_S_16SSE_4 macro CL, TOTALCL, MaskType, ModeType 
	movq XMM_R3, QWORD PTR [VIF_SRC]
	punpcklwd XMM_R3, XMM_R3
	UNPACK_RIGHTSHIFT XMM_R3, 16
	
	pshufd XMM_R0, XMM_R3, 0
	pshufd XMM_R1, XMM_R3, 055h
	pshufd XMM_R2, XMM_R3, 0aah
	pshufd XMM_R3, XMM_R3, 0ffh
	
	UNPACK4_SSE CL, TOTALCL, MaskType, ModeType
	
	add VIF_SRC, 8
	endm

UNPACK_S_16SSE_4A macro CL, TOTALCL, MaskType, ModeType 
	UNPACK_S_16SSE_4 CL, TOTALCL, MaskType, ModeType
	endm

UNPACK_S_16SSE_3 macro CL, TOTALCL, MaskType, ModeType 
	movq XMM_R2, QWORD PTR [VIF_SRC]
	punpcklwd XMM_R2, XMM_R2
	UNPACK_RIGHTSHIFT XMM_R2, 16
	
	pshufd XMM_R0, XMM_R2, 0
	pshufd XMM_R1, XMM_R2, 055h
	pshufd XMM_R2, XMM_R2, 0aah
	
	UNPACK3_SSE CL, TOTALCL, MaskType, ModeType
	add VIF_SRC, 6
	endm

UNPACK_S_16SSE_3A macro CL, TOTALCL, MaskType, ModeType 
	UNPACK_S_16SSE_3 CL, TOTALCL, MaskType, ModeType 
	endm

UNPACK_S_16SSE_2 macro CL, TOTALCL, MaskType, ModeType 
	movd XMM_R1, dword ptr [VIF_SRC]
	punpcklwd XMM_R1, XMM_R1
	UNPACK_RIGHTSHIFT XMM_R1, 16
	
	pshufd XMM_R0, XMM_R1, 0
	pshufd XMM_R1, XMM_R1, 055h
	
	UNPACK2_SSE CL, TOTALCL, MaskType, ModeType
	
	add VIF_SRC, 4
	endm

UNPACK_S_16SSE_2A macro CL, TOTALCL, MaskType, ModeType
	UNPACK_S_16SSE_2 CL, TOTALCL, MaskType, ModeType 
	endm

UNPACK_S_16SSE_1 macro CL, TOTALCL, MaskType, ModeType 
	movd XMM_R0, dword ptr [VIF_SRC]
	punpcklwd XMM_R0, XMM_R0
	UNPACK_RIGHTSHIFT XMM_R0, 16
	pshufd XMM_R0, XMM_R0, 0
	
	UNPACK1_SSE CL, TOTALCL, MaskType, ModeType
	
	add VIF_SRC, 2
	endm

UNPACK_S_16SSE_1A macro CL, TOTALCL, MaskType, ModeType 
	UNPACK_S_16SSE_1 CL, TOTALCL, MaskType, ModeType 
	endm

;; S-8
UNPACK_S_8SSE_4 macro CL, TOTALCL, MaskType, ModeType 
	movd XMM_R3, dword ptr [VIF_SRC]
	punpcklbw XMM_R3, XMM_R3
	punpcklwd XMM_R3, XMM_R3
	UNPACK_RIGHTSHIFT XMM_R3, 24
	
	pshufd XMM_R0, XMM_R3, 0
	pshufd XMM_R1, XMM_R3, 055h
	pshufd XMM_R2, XMM_R3, 0aah
	pshufd XMM_R3, XMM_R3, 0ffh
	
	UNPACK4_SSE CL, TOTALCL, MaskType, ModeType
	
	add VIF_SRC, 4
	endm
	
UNPACK_S_8SSE_4A macro CL, TOTALCL, MaskType, ModeType 
	UNPACK_S_8SSE_4 CL, TOTALCL, MaskType, ModeType 
	endm

UNPACK_S_8SSE_3 macro CL, TOTALCL, MaskType, ModeType 
	movd XMM_R2, dword ptr [VIF_SRC]
	punpcklbw XMM_R2, XMM_R2
	punpcklwd XMM_R2, XMM_R2
	UNPACK_RIGHTSHIFT XMM_R2, 24
	
	pshufd XMM_R0, XMM_R2, 0
	pshufd XMM_R1, XMM_R2, 055h
	pshufd XMM_R2, XMM_R2, 0aah
	
	UNPACK3_SSE CL, TOTALCL, MaskType, ModeType
	
	add VIF_SRC, 3
	endm
	
UNPACK_S_8SSE_3A macro CL, TOTALCL, MaskType, ModeType 
	UNPACK_S_8SSE_3 CL, TOTALCL, MaskType, ModeType 
	endm

UNPACK_S_8SSE_2 macro CL, TOTALCL, MaskType, ModeType 
	movd XMM_R1, dword ptr [VIF_SRC]
	punpcklbw XMM_R1, XMM_R1
	punpcklwd XMM_R1, XMM_R1
	UNPACK_RIGHTSHIFT XMM_R1, 24
	
	pshufd XMM_R0, XMM_R1, 0
	pshufd XMM_R1, XMM_R1, 055h
	
	UNPACK2_SSE CL, TOTALCL, MaskType, ModeType
	
	add VIF_SRC, 2
	endm

UNPACK_S_8SSE_2A macro CL, TOTALCL, MaskType, ModeType 
	UNPACK_S_8SSE_2 CL, TOTALCL, MaskType, ModeType 
	endm

UNPACK_S_8SSE_1 macro CL, TOTALCL, MaskType, ModeType 
	movd XMM_R0, dword ptr [VIF_SRC]
	punpcklbw XMM_R0, XMM_R0
	punpcklwd XMM_R0, XMM_R0
	UNPACK_RIGHTSHIFT XMM_R0, 24
	pshufd XMM_R0, XMM_R0, 0
	
	UNPACK1_SSE CL, TOTALCL, MaskType, ModeType
	
	inc VIF_SRC
	endm

UNPACK_S_8SSE_1A macro CL, TOTALCL, MaskType, ModeType 
	UNPACK_S_8SSE_1 CL, TOTALCL, MaskType, ModeType 
	endm

;; V2-32
UNPACK_V2_32SSE_4A macro CL, TOTALCL, MaskType, ModeType 
	MOVDQA XMM_R0, [VIF_SRC]
	MOVDQA XMM_R2, [VIF_SRC+16]
	
	pshufd XMM_R1, XMM_R0, 0eeh
	pshufd XMM_R3, XMM_R2, 0eeh
	
	UNPACK4_SSE CL, TOTALCL, MaskType, ModeType
	
	add VIF_SRC, 32
	endm

UNPACK_V2_32SSE_4 macro CL, TOTALCL, MaskType, ModeType 
	movq XMM_R0, QWORD PTR [VIF_SRC]
	movq XMM_R1, QWORD PTR [VIF_SRC+8]
	movq XMM_R2, QWORD PTR [VIF_SRC+16]
	movq XMM_R3, QWORD PTR [VIF_SRC+24]
	
	UNPACK4_SSE CL, TOTALCL, MaskType, ModeType
	
	add VIF_SRC, 32
	endm

UNPACK_V2_32SSE_3A macro CL, TOTALCL, MaskType, ModeType 
	MOVDQA XMM_R0, [VIF_SRC]
	movq XMM_R2, QWORD PTR [VIF_SRC+16]
	pshufd XMM_R1, XMM_R0, 0eeh
	
	UNPACK3_SSE CL, TOTALCL, MaskType, ModeType
	
	add VIF_SRC, 24
	endm

UNPACK_V2_32SSE_3 macro CL, TOTALCL, MaskType, ModeType 
	movq XMM_R0, QWORD PTR [VIF_SRC]
	movq XMM_R1, QWORD PTR [VIF_SRC+8]
	movq XMM_R2, QWORD PTR [VIF_SRC+16]
	
	UNPACK3_SSE CL, TOTALCL, MaskType, ModeType
	
	add VIF_SRC, 24
	endm
	
UNPACK_V2_32SSE_2 macro CL, TOTALCL, MaskType, ModeType 
	movq XMM_R0, QWORD PTR [VIF_SRC]
	movq XMM_R1, QWORD PTR [VIF_SRC+8]
	
	UNPACK2_SSE CL, TOTALCL, MaskType, ModeType
	
	add VIF_SRC, 16
	endm

UNPACK_V2_32SSE_2A macro CL, TOTALCL, MaskType, ModeType 
	UNPACK_V2_32SSE_2 CL, TOTALCL, MaskType, ModeType 
	endm

UNPACK_V2_32SSE_1 macro CL, TOTALCL, MaskType, ModeType 
	movq XMM_R0, QWORD PTR [VIF_SRC]
	
	UNPACK1_SSE CL, TOTALCL, MaskType, ModeType
	
	add VIF_SRC, 8
	endm
	
UNPACK_V2_32SSE_1A macro CL, TOTALCL, MaskType, ModeType 
	UNPACK_V2_32SSE_1 CL, TOTALCL, MaskType, ModeType 
	endm

;; V2-16
;; due to lemmings, have to copy lower qword to the upper qword of every reg
UNPACK_V2_16SSE_4A macro CL, TOTALCL, MaskType, ModeType 
	punpcklwd XMM_R0, [VIF_SRC]
	punpckhwd XMM_R2, [VIF_SRC]
	
	UNPACK_RIGHTSHIFT XMM_R0, 16
	UNPACK_RIGHTSHIFT XMM_R2, 16
	;; move the lower 64 bits down
	punpckhqdq XMM_R1, XMM_R0
	punpckhqdq XMM_R3, XMM_R2
	
	punpcklqdq XMM_R0, XMM_R0
	punpcklqdq XMM_R2, XMM_R2
	punpckhqdq XMM_R1, XMM_R1
	punpckhqdq XMM_R3, XMM_R3
	
	UNPACK4_SSE CL, TOTALCL, MaskType, ModeType
	add VIF_SRC, 16
	endm

UNPACK_V2_16SSE_4 macro CL, TOTALCL, MaskType, ModeType 
	movdqu XMM_R0, [VIF_SRC]
	
	punpckhwd XMM_R2, XMM_R0
	punpcklwd XMM_R0, XMM_R0
	
	UNPACK_RIGHTSHIFT XMM_R0, 16
	UNPACK_RIGHTSHIFT XMM_R2, 16
	
	;; move the lower 64 bits down
	punpckhqdq XMM_R1, XMM_R0
	punpckhqdq XMM_R3, XMM_R2
	
	punpcklqdq XMM_R0, XMM_R0
	punpcklqdq XMM_R2, XMM_R2
	punpckhqdq XMM_R1, XMM_R1
	punpckhqdq XMM_R3, XMM_R3
	
	UNPACK4_SSE CL, TOTALCL, MaskType, ModeType
	
	add VIF_SRC, 16
	endm
	
UNPACK_V2_16SSE_3A macro CL, TOTALCL, MaskType, ModeType 
	punpcklwd XMM_R0, [VIF_SRC]
	punpckhwd XMM_R2, [VIF_SRC]
	
	UNPACK_RIGHTSHIFT XMM_R0, 16
	UNPACK_RIGHTSHIFT XMM_R2, 16
	
	;; move the lower 64 bits down
	punpckhqdq XMM_R1, XMM_R0
	
	punpcklqdq XMM_R0, XMM_R0
	punpcklqdq XMM_R2, XMM_R2
	punpckhqdq XMM_R1, XMM_R1
	
	UNPACK3_SSE CL, TOTALCL, MaskType, ModeType
	
	add VIF_SRC, 12
	endm

UNPACK_V2_16SSE_3 macro CL, TOTALCL, MaskType, ModeType 
	movdqu XMM_R0, [VIF_SRC]
	
	punpckhwd XMM_R2, XMM_R0
	punpcklwd XMM_R0, XMM_R0
	
	UNPACK_RIGHTSHIFT XMM_R0, 16
	UNPACK_RIGHTSHIFT XMM_R2, 16
	
	;; move the lower 64 bits down
	punpckhqdq XMM_R1, XMM_R0
	
	punpcklqdq XMM_R0, XMM_R0
	punpcklqdq XMM_R2, XMM_R2
	punpckhqdq XMM_R1, XMM_R1
	
	UNPACK3_SSE CL, TOTALCL, MaskType, ModeType
	
	add VIF_SRC, 12
	endm

UNPACK_V2_16SSE_2A macro CL, TOTALCL, MaskType, ModeType 
	punpcklwd XMM_R0, [VIF_SRC]
	UNPACK_RIGHTSHIFT XMM_R0, 16
	
	;; move the lower 64 bits down
	punpckhqdq XMM_R1, XMM_R0
	
	punpcklqdq XMM_R0, XMM_R0
	punpckhqdq XMM_R1, XMM_R1
	
	UNPACK2_SSE CL, TOTALCL, MaskType, ModeType
	
	add VIF_SRC, 8
	endm

UNPACK_V2_16SSE_2 macro CL, TOTALCL, MaskType, ModeType 
	movq XMM_R0, QWORD PTR [VIF_SRC]
	punpcklwd XMM_R0, XMM_R0
	UNPACK_RIGHTSHIFT XMM_R0, 16
	
	;; move the lower 64 bits down
	punpckhqdq XMM_R1, XMM_R0
	
	punpcklqdq XMM_R0, XMM_R0
	punpckhqdq XMM_R1, XMM_R1
	
	UNPACK2_SSE CL, TOTALCL, MaskType, ModeType
	
	add VIF_SRC, 8
	endm

UNPACK_V2_16SSE_1A macro CL, TOTALCL, MaskType, ModeType 
	punpcklwd XMM_R0, [VIF_SRC]
	UNPACK_RIGHTSHIFT XMM_R0, 16
	punpcklqdq XMM_R0, XMM_R0
	
	UNPACK1_SSE CL, TOTALCL, MaskType, ModeType
	
	add VIF_SRC, 4
	endm
	
UNPACK_V2_16SSE_1 macro CL, TOTALCL, MaskType, ModeType 
	movd XMM_R0, dword ptr [VIF_SRC]
	punpcklwd XMM_R0, XMM_R0
	UNPACK_RIGHTSHIFT XMM_R0, 16
	punpcklqdq XMM_R0, XMM_R0
	
	UNPACK1_SSE CL, TOTALCL, MaskType, ModeType
	
	add VIF_SRC, 4
	endm
	
;; V2-8
;; and1 streetball needs to copy lower qword to the upper qword of every reg
UNPACK_V2_8SSE_4 macro CL, TOTALCL, MaskType, ModeType 
	movq XMM_R0, QWORD PTR [VIF_SRC]
	
	punpcklbw XMM_R0, XMM_R0
	punpckhwd XMM_R2, XMM_R0
	punpcklwd XMM_R0, XMM_R0
	
	UNPACK_RIGHTSHIFT XMM_R0, 24
	UNPACK_RIGHTSHIFT XMM_R2, 24
	
	;; move the lower 64 bits down
	punpckhqdq XMM_R1, XMM_R0
	punpckhqdq XMM_R3, XMM_R2
	
	punpcklqdq XMM_R0, XMM_R0
	punpcklqdq XMM_R2, XMM_R2
	punpckhqdq XMM_R1, XMM_R1
	punpckhqdq XMM_R3, XMM_R3
	
	UNPACK4_SSE CL, TOTALCL, MaskType, ModeType
	
	add VIF_SRC, 8
	endm
	
UNPACK_V2_8SSE_4A macro CL, TOTALCL, MaskType, ModeType 
	UNPACK_V2_8SSE_4 CL, TOTALCL, MaskType, ModeType 
	endm

UNPACK_V2_8SSE_3 macro CL, TOTALCL, MaskType, ModeType 
	movq XMM_R0, QWORD PTR [VIF_SRC]
	
	punpcklbw XMM_R0, XMM_R0
	punpckhwd XMM_R2, XMM_R0
	punpcklwd XMM_R0, XMM_R0
	
	UNPACK_RIGHTSHIFT XMM_R0, 24
	UNPACK_RIGHTSHIFT XMM_R2, 24
	
	;; move the lower 64 bits down
	punpckhqdq XMM_R1, XMM_R0
	
	punpcklqdq XMM_R0, XMM_R0
	punpcklqdq XMM_R2, XMM_R2
	punpckhqdq XMM_R1, XMM_R1
	
	UNPACK3_SSE CL, TOTALCL, MaskType, ModeType
	
	add VIF_SRC, 6
	endm
	
UNPACK_V2_8SSE_3A macro CL, TOTALCL, MaskType, ModeType 
	UNPACK_V2_8SSE_3 CL, TOTALCL, MaskType, ModeType 
	endm

UNPACK_V2_8SSE_2 macro CL, TOTALCL, MaskType, ModeType 
	movd XMM_R0, dword ptr [VIF_SRC]
	punpcklbw XMM_R0, XMM_R0
	punpcklwd XMM_R0, XMM_R0
	UNPACK_RIGHTSHIFT XMM_R0, 24
	
	;; move the lower 64 bits down
	punpckhqdq XMM_R1, XMM_R0
	
	punpcklqdq XMM_R0, XMM_R0
	punpckhqdq XMM_R1, XMM_R1
	
	UNPACK2_SSE CL, TOTALCL, MaskType, ModeType
	
	add VIF_SRC, 4
	endm
	
UNPACK_V2_8SSE_2A macro CL, TOTALCL, MaskType, ModeType 
	UNPACK_V2_8SSE_2 CL, TOTALCL, MaskType, ModeType 
	endm

UNPACK_V2_8SSE_1 macro CL, TOTALCL, MaskType, ModeType 
	movd XMM_R0, dword ptr [VIF_SRC]
	punpcklbw XMM_R0, XMM_R0
	punpcklwd XMM_R0, XMM_R0
	UNPACK_RIGHTSHIFT XMM_R0, 24
	punpcklqdq XMM_R0, XMM_R0
	
	UNPACK1_SSE CL, TOTALCL, MaskType, ModeType
	
	add VIF_SRC, 2
	endm
	
UNPACK_V2_8SSE_1A macro CL, TOTALCL, MaskType, ModeType 
	UNPACK_V2_8SSE_1 CL, TOTALCL, MaskType, ModeType 
	endm

;; V3-32
UNPACK_V3_32SSE_4x macro CL, TOTALCL, MaskType, ModeType, MOVDQA
	MOVDQA XMM_R0, [VIF_SRC]
	movdqu XMM_R1, [VIF_SRC+12]
	
	@CatStr(UNPACK_Setup_, MaskType, _SSE_, ModeType, _, TOTALCL) CL+0
	@CatStr(UNPACK_, MaskType, _SSE_, ModeType) XMM_R0
	@CatStr(UNPACK_Write, TOTALCL, _, MaskType) XMM_R0, CL, 0, movdqa
	
	@CatStr(UNPACK_Setup_, MaskType, _SSE_, ModeType, _, TOTALCL) CL+1
	@CatStr(UNPACK_, MaskType, _SSE_, ModeType) XMM_R1
	@CatStr(UNPACK_Write, TOTALCL, _, MaskType) XMM_R1, CL+1, 16, movdqa
	
	;; midnight club 2 crashes because reading a qw at +36 is out of bounds
    MOVDQA XMM_R3, [VIF_SRC+32]
	movdqu XMM_R2, [VIF_SRC+24]
	psrldq XMM_R3, 4
	
	@CatStr(UNPACK_Setup_, MaskType, _SSE_, ModeType, _, TOTALCL) CL+2
	@CatStr(UNPACK_, MaskType, _SSE_, ModeType) XMM_R2
	@CatStr(UNPACK_Write, TOTALCL, _, MaskType) XMM_R2, CL+2, 32, movdqa
	
	@CatStr(UNPACK_Setup_, MaskType, _SSE_, ModeType, _, TOTALCL) CL+3
	@CatStr(UNPACK_, MaskType, _SSE_, ModeType) XMM_R3
	@CatStr(UNPACK_Write, TOTALCL, _, MaskType) XMM_R3, CL+3, 48, movdqa
	
	@CatStr(UNPACK_INC_DST_, TOTALCL, _, MaskType) 4
	
	add VIF_SRC, 48
	endm

UNPACK_V3_32SSE_4A macro CL, TOTALCL, MaskType, ModeType
	UNPACK_V3_32SSE_4x CL, TOTALCL, MaskType, ModeType, movdqa
	endm
UNPACK_V3_32SSE_4 macro CL, TOTALCL, MaskType, ModeType
	UNPACK_V3_32SSE_4x CL, TOTALCL, MaskType, ModeType, movdqu
	endm

UNPACK_V3_32SSE_3x macro CL, TOTALCL, MaskType, ModeType, MOVDQA
	MOVDQA XMM_R0, [VIF_SRC]
	movdqu XMM_R1, [VIF_SRC+12]
	
	@CatStr(UNPACK_Setup_, MaskType, _SSE_, ModeType, _, TOTALCL) CL
	@CatStr(UNPACK_, MaskType, _SSE_, ModeType) XMM_R0
	@CatStr(UNPACK_Write, TOTALCL, _, MaskType) XMM_R0, CL, 0, movdqa
	
	@CatStr(UNPACK_Setup_, MaskType, _SSE_, ModeType, _, TOTALCL) CL+1
	@CatStr(UNPACK_, MaskType, _SSE_, ModeType) XMM_R1
	@CatStr(UNPACK_Write, TOTALCL, _, MaskType) XMM_R1, CL+1, 16, movdqa
	
	movdqu XMM_R2, [VIF_SRC+24]
	
	@CatStr(UNPACK_Setup_, MaskType, _SSE_, ModeType, _, TOTALCL) CL+2
	@CatStr(UNPACK_, MaskType, _SSE_, ModeType) XMM_R2
	@CatStr(UNPACK_Write, TOTALCL, _, MaskType) XMM_R2, CL+2, 32, movdqa
	
	@CatStr(UNPACK_INC_DST_, TOTALCL, _, MaskType) 3
	
	add VIF_SRC, 36
	endm
	
UNPACK_V3_32SSE_3A macro CL, TOTALCL, MaskType, ModeType
	UNPACK_V3_32SSE_3x CL, TOTALCL, MaskType, ModeType, movdqa
	endm
UNPACK_V3_32SSE_3 macro CL, TOTALCL, MaskType, ModeType
	UNPACK_V3_32SSE_3x CL, TOTALCL, MaskType, ModeType, movdqu
	endm

UNPACK_V3_32SSE_2x macro CL, TOTALCL, MaskType, ModeType, MOVDQA 
	MOVDQA XMM_R0, [VIF_SRC]
	movdqu XMM_R1, [VIF_SRC+12]
	
	UNPACK2_SSE CL, TOTALCL, MaskType, ModeType
	
	add VIF_SRC, 24
	endm

UNPACK_V3_32SSE_2A macro CL, TOTALCL, MaskType, ModeType
	UNPACK_V3_32SSE_2x CL, TOTALCL, MaskType, ModeType, movdqa
	endm
UNPACK_V3_32SSE_2 macro CL, TOTALCL, MaskType, ModeType
	UNPACK_V3_32SSE_2x CL, TOTALCL, MaskType, ModeType, movdqu
	endm

UNPACK_V3_32SSE_1x macro CL, TOTALCL, MaskType, ModeType, MOVDQA 
	MOVDQA XMM_R0, [VIF_SRC]
	
	UNPACK1_SSE CL, TOTALCL, MaskType, ModeType
	
	add VIF_SRC, 12
	endm

UNPACK_V3_32SSE_1A macro CL, TOTALCL, MaskType, ModeType
	UNPACK_V3_32SSE_1x CL, TOTALCL, MaskType, ModeType, movdqa
	endm
UNPACK_V3_32SSE_1 macro CL, TOTALCL, MaskType, ModeType
	UNPACK_V3_32SSE_1x CL, TOTALCL, MaskType, ModeType, movdqu
	endm

;; V3-16
UNPACK_V3_16SSE_4 macro CL, TOTALCL, MaskType, ModeType 
	movq XMM_R0, QWORD PTR [VIF_SRC]
	movq XMM_R1, QWORD PTR [VIF_SRC+6]
	
	punpcklwd XMM_R0, XMM_R0
	movq XMM_R2, QWORD PTR [VIF_SRC+12]
	punpcklwd XMM_R1, XMM_R1
	UNPACK_RIGHTSHIFT XMM_R0, 16
	movq XMM_R3, QWORD PTR [VIF_SRC+18]
	UNPACK_RIGHTSHIFT XMM_R1, 16
	punpcklwd XMM_R2, XMM_R2
	punpcklwd XMM_R3, XMM_R3
	
	UNPACK_RIGHTSHIFT XMM_R2, 16
	UNPACK_RIGHTSHIFT XMM_R3, 16
	
	UNPACK4_SSE CL, TOTALCL, MaskType, ModeType
	
	add VIF_SRC, 24
	endm
	
UNPACK_V3_16SSE_4A macro CL, TOTALCL, MaskType, ModeType 
	UNPACK_V3_16SSE_4 CL, TOTALCL, MaskType, ModeType 
	endm

UNPACK_V3_16SSE_3 macro CL, TOTALCL, MaskType, ModeType 
	movq XMM_R0, QWORD PTR [VIF_SRC]
	movq XMM_R1, QWORD PTR [VIF_SRC+6]
	
	punpcklwd XMM_R0, XMM_R0
	movq XMM_R2, QWORD PTR [VIF_SRC+12]
	punpcklwd XMM_R1, XMM_R1
	UNPACK_RIGHTSHIFT XMM_R0, 16
	punpcklwd XMM_R2, XMM_R2
	
	UNPACK_RIGHTSHIFT XMM_R1, 16
	UNPACK_RIGHTSHIFT XMM_R2, 16
	
	UNPACK3_SSE CL, TOTALCL, MaskType, ModeType
	
	add VIF_SRC, 18
	endm
	
UNPACK_V3_16SSE_3A macro CL, TOTALCL, MaskType, ModeType 
	UNPACK_V3_16SSE_3 CL, TOTALCL, MaskType, ModeType 
	endm

UNPACK_V3_16SSE_2 macro CL, TOTALCL, MaskType, ModeType 
	movq XMM_R0, QWORD PTR [VIF_SRC]
	movq XMM_R1, QWORD PTR [VIF_SRC+6]
	
	punpcklwd XMM_R0, XMM_R0
	punpcklwd XMM_R1, XMM_R1
	
	UNPACK_RIGHTSHIFT XMM_R0, 16
	UNPACK_RIGHTSHIFT XMM_R1, 16
	
	UNPACK2_SSE CL, TOTALCL, MaskType, ModeType
	
	add VIF_SRC, 12
	endm
	
UNPACK_V3_16SSE_2A macro CL, TOTALCL, MaskType, ModeType 
	UNPACK_V3_16SSE_2 CL, TOTALCL, MaskType, ModeType 
	endm

UNPACK_V3_16SSE_1 macro CL, TOTALCL, MaskType, ModeType 
	movq XMM_R0, QWORD PTR [VIF_SRC]
	punpcklwd XMM_R0, XMM_R0
	UNPACK_RIGHTSHIFT XMM_R0, 16
	
	UNPACK1_SSE CL, TOTALCL, MaskType, ModeType
	
	add VIF_SRC, 6
	endm
	
UNPACK_V3_16SSE_1A macro CL, TOTALCL, MaskType, ModeType 
	UNPACK_V3_16SSE_1 CL, TOTALCL, MaskType, ModeType 
	endm
	
;; V3-8
UNPACK_V3_8SSE_4 macro CL, TOTALCL, MaskType, ModeType 
	movq XMM_R1, QWORD PTR [VIF_SRC]
	movq XMM_R3, QWORD PTR [VIF_SRC+6]
	
	punpcklbw XMM_R1, XMM_R1
	punpcklbw XMM_R3, XMM_R3
	punpcklwd XMM_R0, XMM_R1
	psrldq XMM_R1, 6
	punpcklwd XMM_R2, XMM_R3
	psrldq XMM_R3, 6
	punpcklwd XMM_R1, XMM_R1
	UNPACK_RIGHTSHIFT XMM_R0, 24
	punpcklwd XMM_R3, XMM_R3
	
	UNPACK_RIGHTSHIFT XMM_R2, 24
	UNPACK_RIGHTSHIFT XMM_R1, 24
	UNPACK_RIGHTSHIFT XMM_R3, 24
	
	UNPACK4_SSE CL, TOTALCL, MaskType, ModeType
	
	add VIF_SRC, 12
	endm
	
UNPACK_V3_8SSE_4A macro CL, TOTALCL, MaskType, ModeType 
	UNPACK_V3_8SSE_4 CL, TOTALCL, MaskType, ModeType 
	endm

UNPACK_V3_8SSE_3 macro CL, TOTALCL, MaskType, ModeType 
	movd XMM_R0, dword ptr [VIF_SRC]
	movd XMM_R1, dword ptr [VIF_SRC+3]
	
	punpcklbw XMM_R0, XMM_R0
	movd XMM_R2, dword ptr [VIF_SRC+6]
	punpcklbw XMM_R1, XMM_R1
	punpcklwd XMM_R0, XMM_R0
	punpcklbw XMM_R2, XMM_R2
	
	punpcklwd XMM_R1, XMM_R1
	punpcklwd XMM_R2, XMM_R2
	
	UNPACK_RIGHTSHIFT XMM_R0, 24
	UNPACK_RIGHTSHIFT XMM_R1, 24
	UNPACK_RIGHTSHIFT XMM_R2, 24
	
	UNPACK3_SSE CL, TOTALCL, MaskType, ModeType
	
	add VIF_SRC, 9
	endm

UNPACK_V3_8SSE_3A macro CL, TOTALCL, MaskType, ModeType 
	UNPACK_V3_8SSE_3 CL, TOTALCL, MaskType, ModeType 
	endm

UNPACK_V3_8SSE_2 macro CL, TOTALCL, MaskType, ModeType 
	movd XMM_R0, dword ptr [VIF_SRC]
	movd XMM_R1, dword ptr [VIF_SRC+3]
	
	punpcklbw XMM_R0, XMM_R0
	punpcklbw XMM_R1, XMM_R1
	
	punpcklwd XMM_R0, XMM_R0
	punpcklwd XMM_R1, XMM_R1
	
	UNPACK_RIGHTSHIFT XMM_R0, 24
	UNPACK_RIGHTSHIFT XMM_R1, 24
	
	UNPACK2_SSE CL, TOTALCL, MaskType, ModeType
	
	add VIF_SRC, 6
	endm
	
UNPACK_V3_8SSE_2A macro CL, TOTALCL, MaskType, ModeType 
	UNPACK_V3_8SSE_2 CL, TOTALCL, MaskType, ModeType 
	endm

UNPACK_V3_8SSE_1 macro CL, TOTALCL, MaskType, ModeType 
	movd XMM_R0, dword ptr [VIF_SRC]
	punpcklbw XMM_R0, XMM_R0
	punpcklwd XMM_R0, XMM_R0
	UNPACK_RIGHTSHIFT XMM_R0, 24
	
	UNPACK1_SSE CL, TOTALCL, MaskType, ModeType
	
	add VIF_SRC, 3
	endm
	
UNPACK_V3_8SSE_1A macro CL, TOTALCL, MaskType, ModeType 
	UNPACK_V3_8SSE_1 CL, TOTALCL, MaskType, ModeType 
	endm

;; V4-32
UNPACK_V4_32SSE_4A macro CL, TOTALCL, MaskType, ModeType 
	movdqa XMM_R0, [VIF_SRC]
	movdqa XMM_R1, [VIF_SRC+16]
	movdqa XMM_R2, [VIF_SRC+32]
	movdqa XMM_R3, [VIF_SRC+48]
	
	UNPACK4_SSE CL, TOTALCL, MaskType, ModeType
	
	add VIF_SRC, 64
	endm
	
UNPACK_V4_32SSE_4 macro CL, TOTALCL, MaskType, ModeType 
	movdqu XMM_R0, [VIF_SRC]
	movdqu XMM_R1, [VIF_SRC+16]
	movdqu XMM_R2, [VIF_SRC+32]
	movdqu XMM_R3, [VIF_SRC+48]
	
	UNPACK4_SSE CL, TOTALCL, MaskType, ModeType
	
	add VIF_SRC, 64
	endm
	
UNPACK_V4_32SSE_3A macro CL, TOTALCL, MaskType, ModeType 
	movdqa XMM_R0, [VIF_SRC]
	movdqa XMM_R1, [VIF_SRC+16]
	movdqa XMM_R2, [VIF_SRC+32]
	
	UNPACK3_SSE CL, TOTALCL, MaskType, ModeType
	
	add VIF_SRC, 48
	endm
	
UNPACK_V4_32SSE_3 macro CL, TOTALCL, MaskType, ModeType 
	movdqu XMM_R0, [VIF_SRC]
	movdqu XMM_R1, [VIF_SRC+16]
	movdqu XMM_R2, [VIF_SRC+32]
	
	UNPACK3_SSE CL, TOTALCL, MaskType, ModeType
	
	add VIF_SRC, 48
	endm
	
UNPACK_V4_32SSE_2A macro CL, TOTALCL, MaskType, ModeType 
	movdqa XMM_R0, [VIF_SRC]
	movdqa XMM_R1, [VIF_SRC+16]
	
	UNPACK2_SSE CL, TOTALCL, MaskType, ModeType
	
	add VIF_SRC, 32
	endm
	
UNPACK_V4_32SSE_2 macro CL, TOTALCL, MaskType, ModeType 
	movdqu XMM_R0, [VIF_SRC]
	movdqu XMM_R1, [VIF_SRC+16]
	
	UNPACK2_SSE CL, TOTALCL, MaskType, ModeType
	
	add VIF_SRC, 32
	endm
	
UNPACK_V4_32SSE_1A macro CL, TOTALCL, MaskType, ModeType 
	movdqa XMM_R0, [VIF_SRC]
	
	UNPACK1_SSE CL, TOTALCL, MaskType, ModeType
	
	add VIF_SRC, 16
	endm
	
UNPACK_V4_32SSE_1 macro CL, TOTALCL, MaskType, ModeType 
	movdqu XMM_R0, [VIF_SRC]
	
	UNPACK1_SSE CL, TOTALCL, MaskType, ModeType
	
	add VIF_SRC, 16
	endm
	
;; V4-16
UNPACK_V4_16SSE_4A macro CL, TOTALCL, MaskType, ModeType 
	
	punpcklwd XMM_R0, [VIF_SRC]
	punpckhwd XMM_R1, [VIF_SRC]
	punpcklwd XMM_R2, [VIF_SRC+16]
	punpckhwd XMM_R3, [VIF_SRC+16]
	
	UNPACK_RIGHTSHIFT XMM_R1, 16
	UNPACK_RIGHTSHIFT XMM_R3, 16
	UNPACK_RIGHTSHIFT XMM_R0, 16
	UNPACK_RIGHTSHIFT XMM_R2, 16
	
	UNPACK4_SSE CL, TOTALCL, MaskType, ModeType
	
	add VIF_SRC, 32
	endm
	
UNPACK_V4_16SSE_4 macro CL, TOTALCL, MaskType, ModeType 
	movdqu XMM_R0, [VIF_SRC]
	movdqu XMM_R2, [VIF_SRC+16]
	
	punpckhwd XMM_R1, XMM_R0
	punpckhwd XMM_R3, XMM_R2
	punpcklwd XMM_R0, XMM_R0
	punpcklwd XMM_R2, XMM_R2
	
	UNPACK_RIGHTSHIFT XMM_R1, 16
	UNPACK_RIGHTSHIFT XMM_R3, 16
	UNPACK_RIGHTSHIFT XMM_R0, 16
	UNPACK_RIGHTSHIFT XMM_R2, 16
	
	UNPACK4_SSE CL, TOTALCL, MaskType, ModeType
	
	add VIF_SRC, 32
	endm
	
UNPACK_V4_16SSE_3A macro CL, TOTALCL, MaskType, ModeType 
	punpcklwd XMM_R0, [VIF_SRC]
	punpckhwd XMM_R1, [VIF_SRC]
	punpcklwd XMM_R2, [VIF_SRC+16]
	
	UNPACK_RIGHTSHIFT XMM_R0, 16
	UNPACK_RIGHTSHIFT XMM_R1, 16
	UNPACK_RIGHTSHIFT XMM_R2, 16
	
	UNPACK3_SSE CL, TOTALCL, MaskType, ModeType
	
	add VIF_SRC, 24
	endm
	
UNPACK_V4_16SSE_3 macro CL, TOTALCL, MaskType, ModeType 
	movdqu XMM_R0, [VIF_SRC]
	movq XMM_R2, QWORD PTR [VIF_SRC+16]
	
	punpckhwd XMM_R1, XMM_R0
	punpcklwd XMM_R0, XMM_R0
	punpcklwd XMM_R2, XMM_R2
	
	UNPACK_RIGHTSHIFT XMM_R0, 16
	UNPACK_RIGHTSHIFT XMM_R1, 16
	UNPACK_RIGHTSHIFT XMM_R2, 16
	
	UNPACK3_SSE CL, TOTALCL, MaskType, ModeType
	
	add VIF_SRC, 24
	endm
	
UNPACK_V4_16SSE_2A macro CL, TOTALCL, MaskType, ModeType 
	punpcklwd XMM_R0, [VIF_SRC]
	punpckhwd XMM_R1, [VIF_SRC]
	
	UNPACK_RIGHTSHIFT XMM_R0, 16
	UNPACK_RIGHTSHIFT XMM_R1, 16
	
	UNPACK2_SSE CL, TOTALCL, MaskType, ModeType
	
	add VIF_SRC, 16
	endm
	
UNPACK_V4_16SSE_2 macro CL, TOTALCL, MaskType, ModeType 
	movq XMM_R0, QWORD PTR [VIF_SRC]
	movq XMM_R1, QWORD PTR [VIF_SRC+8]
	
	punpcklwd XMM_R0, XMM_R0
	punpcklwd XMM_R1, XMM_R1
	
	UNPACK_RIGHTSHIFT XMM_R0, 16
	UNPACK_RIGHTSHIFT XMM_R1, 16
	
	UNPACK2_SSE CL, TOTALCL, MaskType, ModeType
	
	add VIF_SRC, 16
	endm
	
UNPACK_V4_16SSE_1A macro CL, TOTALCL, MaskType, ModeType 
	punpcklwd XMM_R0, [VIF_SRC]
	UNPACK_RIGHTSHIFT XMM_R0, 16
	
	UNPACK1_SSE CL, TOTALCL, MaskType, ModeType
	
	add VIF_SRC, 8
	endm
	
UNPACK_V4_16SSE_1 macro CL, TOTALCL, MaskType, ModeType 
	movq XMM_R0, QWORD PTR [VIF_SRC]
	punpcklwd XMM_R0, XMM_R0
	UNPACK_RIGHTSHIFT XMM_R0, 16
	
	UNPACK1_SSE CL, TOTALCL, MaskType, ModeType
	
	add VIF_SRC, 8
	endm
	
;; V4-8
UNPACK_V4_8SSE_4A macro CL, TOTALCL, MaskType, ModeType 
	punpcklbw XMM_R0, [VIF_SRC]
	punpckhbw XMM_R2, [VIF_SRC]
	
	punpckhwd XMM_R1, XMM_R0
	punpckhwd XMM_R3, XMM_R2
	punpcklwd XMM_R0, XMM_R0
	punpcklwd XMM_R2, XMM_R2
	
	UNPACK_RIGHTSHIFT XMM_R1, 24
	UNPACK_RIGHTSHIFT XMM_R3, 24
	UNPACK_RIGHTSHIFT XMM_R0, 24
	UNPACK_RIGHTSHIFT XMM_R2, 24
	
	UNPACK4_SSE CL, TOTALCL, MaskType, ModeType
	
	add VIF_SRC, 16
	endm
	
UNPACK_V4_8SSE_4 macro CL, TOTALCL, MaskType, ModeType 
	movdqu XMM_R0, [VIF_SRC]
	
	punpckhbw XMM_R2, XMM_R0
	punpcklbw XMM_R0, XMM_R0
	
	punpckhwd XMM_R3, XMM_R2
	punpckhwd XMM_R1, XMM_R0
	punpcklwd XMM_R2, XMM_R2
	punpcklwd XMM_R0, XMM_R0
	
	UNPACK_RIGHTSHIFT XMM_R3, 24
	UNPACK_RIGHTSHIFT XMM_R2, 24
	
	UNPACK_RIGHTSHIFT XMM_R0, 24
	UNPACK_RIGHTSHIFT XMM_R1, 24
	
	UNPACK4_SSE CL, TOTALCL, MaskType, ModeType
	
	add VIF_SRC, 16
	endm
	
UNPACK_V4_8SSE_3A macro CL, TOTALCL, MaskType, ModeType 
	punpcklbw XMM_R0, [VIF_SRC]
	punpckhbw XMM_R2, [VIF_SRC]
	
	punpckhwd XMM_R1, XMM_R0
	punpcklwd XMM_R0, XMM_R0
	punpcklwd XMM_R2, XMM_R2
	
	UNPACK_RIGHTSHIFT XMM_R1, 24
	UNPACK_RIGHTSHIFT XMM_R0, 24
	UNPACK_RIGHTSHIFT XMM_R2, 24
	
	UNPACK3_SSE CL, TOTALCL, MaskType, ModeType
	
	add VIF_SRC, 12
	endm
	
UNPACK_V4_8SSE_3 macro CL, TOTALCL, MaskType, ModeType 
	movq XMM_R0, QWORD PTR [VIF_SRC]
	movd XMM_R2, dword ptr [VIF_SRC+8]
	
	punpcklbw XMM_R0, XMM_R0
	punpcklbw XMM_R2, XMM_R2
	
	punpckhwd XMM_R1, XMM_R0
	punpcklwd XMM_R2, XMM_R2
	punpcklwd XMM_R0, XMM_R0
	
	UNPACK_RIGHTSHIFT XMM_R1, 24
	UNPACK_RIGHTSHIFT XMM_R0, 24
	UNPACK_RIGHTSHIFT XMM_R2, 24
	
	UNPACK3_SSE CL, TOTALCL, MaskType, ModeType
	
	add VIF_SRC, 12
	endm
	
UNPACK_V4_8SSE_2A macro CL, TOTALCL, MaskType, ModeType 
	punpcklbw XMM_R0, [VIF_SRC]
	
	punpckhwd XMM_R1, XMM_R0
	punpcklwd XMM_R0, XMM_R0
	
	UNPACK_RIGHTSHIFT XMM_R1, 24
	UNPACK_RIGHTSHIFT XMM_R0, 24
	
	UNPACK2_SSE CL, TOTALCL, MaskType, ModeType
	
	add VIF_SRC, 8
	endm
	
UNPACK_V4_8SSE_2 macro CL, TOTALCL, MaskType, ModeType 
	movq XMM_R0, QWORD PTR [VIF_SRC]
	
	punpcklbw XMM_R0, XMM_R0
	
	punpckhwd XMM_R1, XMM_R0
	punpcklwd XMM_R0, XMM_R0
	
	UNPACK_RIGHTSHIFT XMM_R1, 24
	UNPACK_RIGHTSHIFT XMM_R0, 24
	
	UNPACK2_SSE CL, TOTALCL, MaskType, ModeType
	
	add VIF_SRC, 8
	endm
	
UNPACK_V4_8SSE_1A macro CL, TOTALCL, MaskType, ModeType 
	punpcklbw XMM_R0, [VIF_SRC]
	punpcklwd XMM_R0, XMM_R0
	UNPACK_RIGHTSHIFT XMM_R0, 24
	
	UNPACK1_SSE CL, TOTALCL, MaskType, ModeType
	
	add VIF_SRC, 4
	endm
	
UNPACK_V4_8SSE_1 macro CL, TOTALCL, MaskType, ModeType 
	movd XMM_R0, dword ptr [VIF_SRC]
	punpcklbw XMM_R0, XMM_R0
	punpcklwd XMM_R0, XMM_R0
	UNPACK_RIGHTSHIFT XMM_R0, 24
	
	UNPACK1_SSE CL, TOTALCL, MaskType, ModeType
	
	add VIF_SRC, 4
	endm
	
;; V4-5
DECOMPRESS_RGBA macro OFFSET 
	mov bl, al
	shl bl, 3
	mov byte ptr [s_TempDecompress+OFFSET], bl
	
	mov bx, ax
	shr bx, 2
	and bx, 0f8h
	mov byte ptr [s_TempDecompress+OFFSET+1], bl
	
	mov bx, ax
	shr bx, 7
	and bx, 0f8h
	mov byte ptr [s_TempDecompress+OFFSET+2], bl
	mov bx, ax
	shr bx, 8
	and bx, 080h
	mov byte ptr [s_TempDecompress+OFFSET+3], bl
	endm
	
UNPACK_V4_5SSE_4 macro CL, TOTALCL, MaskType, ModeType 
	mov eax, dword ptr [VIF_SRC]
	DECOMPRESS_RGBA 0
	
	shr eax, 16
	DECOMPRESS_RGBA 4
	
	mov eax, dword ptr [VIF_SRC+4]
	DECOMPRESS_RGBA 8
	
	shr eax, 16
	DECOMPRESS_RGBA 12

	;; have to use movaps instead of movdqa
%ifdef __x86_64__
    movdqa XMM_R0, XMMWORD PTR [s_TempDecompress]
%else
	movaps XMM_R0, [s_TempDecompress]
%endif

	punpckhbw XMM_R2, XMM_R0
	punpcklbw XMM_R0, XMM_R0

	punpckhwd XMM_R3, XMM_R2
	punpckhwd XMM_R1, XMM_R0
	punpcklwd XMM_R0, XMM_R0
	punpcklwd XMM_R2, XMM_R2

	psrld XMM_R0, 24
	psrld XMM_R1, 24
	psrld XMM_R2, 24
	psrld XMM_R3, 24

	UNPACK4_SSE CL, TOTALCL, MaskType, ModeType

	add VIF_SRC, 8
	endm

UNPACK_V4_5SSE_4A macro CL, TOTALCL, MaskType, ModeType
	UNPACK_V4_5SSE_4 CL, TOTALCL, MaskType, ModeType
	endm

UNPACK_V4_5SSE_3 macro CL, TOTALCL, MaskType, ModeType
	mov eax, dword ptr [VIF_SRC]
	DECOMPRESS_RGBA 0

	shr eax, 16
	DECOMPRESS_RGBA 4

    mov eax, dword ptr [VIF_SRC]
	DECOMPRESS_RGBA 8

	;; have to use movaps instead of movdqa
%ifdef __x86_64__
    movdqa XMM_R0, XMMWORD PTR [s_TempDecompress]
%else
	movaps XMM_R0, [s_TempDecompress]
%endif

	punpckhbw XMM_R2, XMM_R0
	punpcklbw XMM_R0, XMM_R0

	punpckhwd XMM_R1, XMM_R0
	punpcklwd XMM_R0, XMM_R0
	punpcklwd XMM_R2, XMM_R2

	psrld XMM_R0, 24
	psrld XMM_R1, 24
	psrld XMM_R2, 24

	UNPACK3_SSE CL, TOTALCL, MaskType, ModeType

	add VIF_SRC, 6
	endm

UNPACK_V4_5SSE_3A macro CL, TOTALCL, MaskType, ModeType
	UNPACK_V4_5SSE_3 CL, TOTALCL, MaskType, ModeType
	endm

UNPACK_V4_5SSE_2 macro CL, TOTALCL, MaskType, ModeType
	mov eax, dword ptr [VIF_SRC]
	DECOMPRESS_RGBA 0

	shr eax, 16
	DECOMPRESS_RGBA 4

	movq XMM_R0, QWORD PTR [s_TempDecompress]
	
	punpcklbw XMM_R0, XMM_R0
	
	punpckhwd XMM_R1, XMM_R0
	punpcklwd XMM_R0, XMM_R0
	
	psrld XMM_R0, 24
	psrld XMM_R1, 24
	
	UNPACK2_SSE CL, TOTALCL, MaskType, ModeType
	
	add VIF_SRC, 4
	endm
	
UNPACK_V4_5SSE_2A macro CL, TOTALCL, MaskType, ModeType 
	UNPACK_V4_5SSE_2 CL, TOTALCL, MaskType, ModeType
	endm

UNPACK_V4_5SSE_1 macro CL, TOTALCL, MaskType, ModeType 
	mov ax, word ptr [VIF_SRC]
	DECOMPRESS_RGBA 0 
	
	movd XMM_R0, DWORD PTR [s_TempDecompress]
	punpcklbw XMM_R0, XMM_R0
	punpcklwd XMM_R0, XMM_R0
	
	psrld XMM_R0, 24
	
	UNPACK1_SSE CL, TOTALCL, MaskType, ModeType
	
	add VIF_SRC, 2
	endm
	
UNPACK_V4_5SSE_1A macro CL, TOTALCL, MaskType, ModeType 
	UNPACK_V4_5SSE_1 CL, TOTALCL, MaskType, ModeType 
	endm

;; save the row reg
SAVE_ROW_REG_BASE macro
	mov VIF_TMPADDR, [_vifRow]
	movdqa [VIF_TMPADDR], XMM_ROW
	mov VIF_TMPADDR, [_vifRegs]
	movss dword ptr [VIF_TMPADDR+0100h], XMM_ROW
	psrldq XMM_ROW, 4
	movss dword ptr [VIF_TMPADDR+0110h], XMM_ROW
	psrldq XMM_ROW, 4
	movss dword ptr [VIF_TMPADDR+0120h], XMM_ROW
	psrldq XMM_ROW, 4
	movss dword ptr [VIF_TMPADDR+0130h], XMM_ROW
	endm
	
SAVE_NO_REG macro
	endm

%ifdef __x86_64__

INIT_ARGS macro
	mov rax, qword ptr [_vifRow]
	mov r9, qword ptr [_vifCol]
	movaps xmm6, XMMWORD PTR [rax]
	movaps xmm7, XMMWORD PTR [r9]
	endm

INC_STACK macro reg
	add rsp, 8
	endm

%else

%define STACKOFFSET 12

;; 32 bit versions have the args on the stack
INIT_ARGS macro
    mov VIF_DST, dword ptr [esp+4+STACKOFFSET]
    mov VIF_SRC, dword ptr [esp+8+STACKOFFSET]
    mov VIF_SIZE, dword ptr [esp+12+STACKOFFSET]
    endm

INC_STACK macro reg
	add esp, 4
	endm

%endif

;; qsize - bytes of compressed size of 1 decompressed qword
;; int UNPACK_SkippingWrite_##name##_##sign##_##MaskType##_##ModeType(u32* dest, u32* data, int dmasize)
defUNPACK_SkippingWrite macro name, MaskType, ModeType, qsize, sign, SAVE_ROW_REG
@CatStr(UNPACK_SkippingWrite_, name, _, sign, _, MaskType, _, ModeType) proc public
%ifdef __x86_64__
	push rdi
%else
	push edi
	push esi
	push ebx
%endif
	INIT_ARGS
    mov VIF_TMPADDR, [_vifRegs]
    movzx VIF_INC, byte ptr [VIF_TMPADDR + 040h]
    movzx VIF_SAVEEBX, byte ptr [VIF_TMPADDR + 041h]
    sub VIF_INC, VIF_SAVEEBX
    shl VIF_INC, 4

    cmp VIF_SAVEEBXd, 1
    je @CatStr(name, _, sign, _, MaskType, _, ModeType, _WL1)
    cmp VIF_SAVEEBXd, 2
    je @CatStr(name, _, sign, _, MaskType, _, ModeType, _WL2)
    cmp VIF_SAVEEBXd, 3
    je @CatStr(name, _, sign, _, MaskType, _, ModeType, _WL3)
    jmp @CatStr(name, _, sign, _, MaskType, _, ModeType, _WL4)

@CatStr(name, _, sign, _, MaskType, _, ModeType, _WL1):
    @CatStr(UNPACK_Start_Setup_, MaskType, _SSE_, ModeType) 0

	cmp VIF_SIZE, qsize
	jl @CatStr(name, _, sign, _, MaskType, _, ModeType, _C1_Done3)

	add VIF_INC, 16

	;; first align VIF_SRC to 16 bytes
@CatStr(name, _, sign, _, MaskType, _, ModeType, _C1_Align16):

	test VIF_SRC, 15
	jz @CatStr(name, _, sign, _, MaskType, _, ModeType, _C1_UnpackAligned)

	@CatStr(UNPACK_, name, SSE_1) 0, 1, MaskType, ModeType

	cmp VIF_SIZE, (2*qsize)
	jl @CatStr(name, _, sign, _, MaskType, _, ModeType, _C1_DoneWithDec)
	sub VIF_SIZE, qsize
	jmp @CatStr(name, _, sign, _, MaskType, _, ModeType, _C1_Align16)

@CatStr(name, _, sign, _, MaskType, _, ModeType, _C1_UnpackAligned):

	cmp VIF_SIZE, (2*qsize)
	jl @CatStr(name, _, sign, _, MaskType, _, ModeType, _C1_Unpack1)
	cmp VIF_SIZE, (3*qsize)
	jl @CatStr(name, _, sign, _, MaskType, _, ModeType, _C1_Unpack2)
	cmp VIF_SIZE, (4*qsize)
	jl @CatStr(name, _, sign, _, MaskType, _, ModeType, _C1_Unpack3)
	prefetchnta [VIF_SRC + 64]
	
@CatStr(name, _, sign, _, MaskType, _, ModeType, _C1_Unpack4): 
	@CatStr(UNPACK_, name, SSE_4A) 0, 1, MaskType, ModeType
	
	cmp VIF_SIZE, (8*qsize)
	jl @CatStr(name, _, sign, _, MaskType, _, ModeType, _C1_DoneUnpack4)
	sub VIF_SIZE, (4*qsize)
	jmp @CatStr(name, _, sign, _, MaskType, _, ModeType, _C1_Unpack4)
	
@CatStr(name, _, sign, _, MaskType, _, ModeType, _C1_DoneUnpack4):
	
	sub VIF_SIZE, (4*qsize)
	cmp VIF_SIZE, qsize
	jl @CatStr(name, _, sign, _, MaskType, _, ModeType, _C1_Done3)
	cmp VIF_SIZE, (2*qsize)
	jl @CatStr(name, _, sign, _, MaskType, _, ModeType, _C1_Unpack1)
	cmp VIF_SIZE, (3*qsize)
	jl @CatStr(name, _, sign, _, MaskType, _, ModeType, _C1_Unpack2)
	;; fall through
	
@CatStr(name, _, sign, _, MaskType, _, ModeType, _C1_Unpack3): 
	@CatStr(UNPACK_, name, SSE_3A) 0, 1, MaskType, ModeType
	
	sub VIF_SIZE, (3*qsize)
	jmp @CatStr(name, _, sign, _, MaskType, _, ModeType, _C1_Done3)
	
@CatStr(name, _, sign, _, MaskType, _, ModeType, _C1_Unpack2): 
	@CatStr(UNPACK_, name, SSE_2A) 0, 1, MaskType, ModeType
	
	sub VIF_SIZE, (2*qsize)
	jmp @CatStr(name, _, sign, _, MaskType, _, ModeType, _C1_Done3)
	
@CatStr(name, _, sign, _, MaskType, _, ModeType, _C1_Unpack1): 
	@CatStr(UNPACK_, name, SSE_1A) 0, 1, MaskType, ModeType
@CatStr(name, _, sign, _, MaskType, _, ModeType, _C1_DoneWithDec): 
	sub VIF_SIZE, qsize
@CatStr(name, _, sign, _, MaskType, _, ModeType, _C1_Done3): 
	SAVE_ROW_REG
    mov eax, VIF_SIZE
%ifdef __x86_64__
	pop rdi
%else
	pop ebx
	pop esi
	pop edi
%endif
    ret
    
@CatStr(name, _, sign, _, MaskType, _, ModeType, _WL2): 
	cmp VIF_SIZE, (2*qsize)
	
	jl @CatStr(name, _, sign, _, MaskType, _, ModeType, _C2_Done3)
@CatStr(name, _, sign, _, MaskType, _, ModeType, _C2_Unpack): 
	@CatStr(UNPACK_, name, SSE_2) 0, 0, MaskType, ModeType
	
	;; take into account wl
	add VIF_DST, VIF_INC
	cmp VIF_SIZE, (4*qsize)
	jl @CatStr(name, _, sign, _, MaskType, _, ModeType, _C2_Done2)
	sub VIF_SIZE, (2*qsize)
	;; unpack next
	jmp @CatStr(name, _, sign, _, MaskType, _, ModeType, _C2_Unpack)
	
@CatStr(name, _, sign, _, MaskType, _, ModeType, _C2_Done2): 
	sub VIF_SIZE, (2*qsize)
@CatStr(name, _, sign, _, MaskType, _, ModeType, _C2_Done3): 
	cmp VIF_SIZE, qsize
    
	jl @CatStr(name, _, sign, _, MaskType, _, ModeType, _C2_Done4)

	;; execute left over qw
	@CatStr(UNPACK_, name, SSE_1) 0, 0, MaskType, ModeType
	
	sub VIF_SIZE, qsize
@CatStr(name, _, sign, _, MaskType, _, ModeType, _C2_Done4): 
	
	SAVE_ROW_REG
    mov eax, VIF_SIZE
%ifdef __x86_64__
	pop rdi
%else
	pop ebx
	pop esi
	pop edi
%endif
	ret
	
@CatStr(name, _, sign, _, MaskType, _, ModeType, _WL3): 
	cmp VIF_SIZE, (3*qsize)
	
	jl @CatStr(name, _, sign, _, MaskType, _, ModeType, _C3_Done5)
@CatStr(name, _, sign, _, MaskType, _, ModeType, _C3_Unpack): 
	@CatStr(UNPACK_, name, SSE_3) 0, 0, MaskType, ModeType
	
	add VIF_DST, VIF_INC
	cmp VIF_SIZE, (6*qsize)
	jl @CatStr(name, _, sign, _, MaskType, _, ModeType, _C3_Done2)
	sub VIF_SIZE, (3*qsize)
	jmp @CatStr(name, _, sign, _, MaskType, _, ModeType, _C3_Unpack)
@CatStr(name, _, sign, _, MaskType, _, ModeType, _C3_Done2): 
	sub VIF_SIZE, (3*qsize)
@CatStr(name, _, sign, _, MaskType, _, ModeType, _C3_Done5): 
	cmp VIF_SIZE, qsize
	jl @CatStr(name, _, sign, _, MaskType, _, ModeType, _C3_Done4)
	
    cmp VIF_SIZE, (2*qsize)
	jl @CatStr(name, _, sign, _, MaskType, _, ModeType, _C3_Done3)
	
    @CatStr(UNPACK_, name, SSE_2) 0, 0, MaskType, ModeType
	
	sub VIF_SIZE, (2*qsize)
	jmp @CatStr(name, _, sign, _, MaskType, _, ModeType, _C3_Done4)
@CatStr(name, _, sign, _, MaskType, _, ModeType, _C3_Done3): 
	sub VIF_SIZE, qsize
	@CatStr(UNPACK_, name, SSE_1) 0, 0, MaskType, ModeType
@CatStr(name, _, sign, _, MaskType, _, ModeType, _C3_Done4): 
	SAVE_ROW_REG
    mov eax, VIF_SIZE
%ifdef __x86_64__
	pop rdi
%else
	pop ebx
	pop esi
	pop edi
%endif
    ret
	
@CatStr(name, _, sign, _, MaskType, _, ModeType, _WL4): 
	sub VIF_SAVEEBX, 3
	push VIF_INC
	cmp VIF_SIZE, qsize
	jl @CatStr(name, _, sign, _, MaskType, _, ModeType, _C4_Done)
	
@CatStr(name, _, sign, _, MaskType, _, ModeType, _C4_Unpack): 
	cmp VIF_SIZE, (3*qsize)
	jge @CatStr(name, _, sign, _, MaskType, _, ModeType, _C4_Unpack3)
	cmp VIF_SIZE, (2*qsize)
	jge @CatStr(name, _, sign, _, MaskType, _, ModeType, _C4_Unpack2)
	
	@CatStr(UNPACK_, name, SSE_1) 0, 0, MaskType, ModeType 
	
	;; not enough data left
    sub VIF_SIZE, qsize
	jmp @CatStr(name, _, sign, _, MaskType, _, ModeType, _C4_Done)
@CatStr(name, _, sign, _, MaskType, _, ModeType, _C4_Unpack2): 
	@CatStr(UNPACK_, name, SSE_2) 0, 0, MaskType, ModeType
	
	;; not enough data left
    sub VIF_SIZE, (2*qsize)
	jmp @CatStr(name, _, sign, _, MaskType, _, ModeType, _C4_Done)
@CatStr(name, _, sign, _, MaskType, _, ModeType, _C4_Unpack3): 
	@CatStr(UNPACK_, name, SSE_3) 0, 0, MaskType, ModeType
	
	;; more data left, process 1qw at a time
	sub VIF_SIZE, (3*qsize)
    mov VIF_INC, VIF_SAVEEBX
	
@CatStr(name, _, sign, _, MaskType, _, ModeType, _C4_UnpackX): 

	;; check if any data left
    cmp VIF_SIZE, qsize
	jl @CatStr(name, _, sign, _, MaskType, _, ModeType, _C4_Done)
	
	@CatStr(UNPACK_, name, SSE_1) 3, 0, MaskType, ModeType
	
	sub VIF_SIZE, qsize
	cmp VIF_INC, 1
	je @CatStr(name, _, sign, _, MaskType, _, ModeType, _C4_DoneLoop)
	sub VIF_INC, 1
	jmp @CatStr(name, _, sign, _, MaskType, _, ModeType, _C4_UnpackX)
@CatStr(name, _, sign, _, MaskType, _, ModeType, _C4_DoneLoop): 
	add VIF_DST, [VIF_ESP]
	cmp VIF_SIZE, qsize
	jl @CatStr(name, _, sign, _, MaskType, _, ModeType, _C4_Done)
	jmp @CatStr(name, _, sign, _, MaskType, _, ModeType, _C4_Unpack)
@CatStr(name, _, sign, _, MaskType, _, ModeType, _C4_Done): 
	
	SAVE_ROW_REG
	INC_STACK()
    mov eax, VIF_SIZE
    
%ifdef __x86_64__
	pop rdi
%else
	pop ebx
	pop esi
	pop edi
%endif
    ret
@CatStr(UNPACK_SkippingWrite_, name, _, sign, _, MaskType, _, ModeType endp)
endm

UNPACK_RIGHTSHIFT macro reg, shift
	psrld reg, shift
	endm

defUNPACK_SkippingWrite2 macro name, qsize
	defUNPACK_SkippingWrite name, Regular, 0, qsize, u, SAVE_NO_REG 
	defUNPACK_SkippingWrite name, Regular, 1, qsize, u, SAVE_NO_REG 
	defUNPACK_SkippingWrite name, Regular, 2, qsize, u, SAVE_ROW_REG_BASE 
	defUNPACK_SkippingWrite name, Mask, 0, qsize, u, SAVE_NO_REG 
	defUNPACK_SkippingWrite name, Mask, 1, qsize, u, SAVE_NO_REG 
	defUNPACK_SkippingWrite name, Mask, 2, qsize, u, SAVE_ROW_REG_BASE 
	defUNPACK_SkippingWrite name, WriteMask, 0, qsize, u, SAVE_NO_REG 
	defUNPACK_SkippingWrite name, WriteMask, 1, qsize, u, SAVE_NO_REG 
	defUNPACK_SkippingWrite name, WriteMask, 2, qsize, u, SAVE_ROW_REG_BASE 
	endm

defUNPACK_SkippingWrite2 S_32, 4
defUNPACK_SkippingWrite2 S_16, 2
defUNPACK_SkippingWrite2 S_8, 1
defUNPACK_SkippingWrite2 V2_32, 8
defUNPACK_SkippingWrite2 V2_16, 4
defUNPACK_SkippingWrite2 V2_8, 2
defUNPACK_SkippingWrite2 V3_32, 12
defUNPACK_SkippingWrite2 V3_16, 6
defUNPACK_SkippingWrite2 V3_8, 3
defUNPACK_SkippingWrite2 V4_32, 16
defUNPACK_SkippingWrite2 V4_16, 8
defUNPACK_SkippingWrite2 V4_8, 4
defUNPACK_SkippingWrite2 V4_5, 2

UNPACK_RIGHTSHIFT macro reg, shift
	psrad reg, shift
	endm
	
	
defUNPACK_SkippingWrite2a macro name, qsize
	defUNPACK_SkippingWrite name, Mask, 0, qsize, s, SAVE_NO_REG
	defUNPACK_SkippingWrite name, Regular, 0, qsize, s, SAVE_NO_REG
	defUNPACK_SkippingWrite name, Regular, 1, qsize, s, SAVE_NO_REG
	defUNPACK_SkippingWrite name, Regular, 2, qsize, s, SAVE_ROW_REG_BASE
	defUNPACK_SkippingWrite name, Mask, 1, qsize, s, SAVE_NO_REG
	defUNPACK_SkippingWrite name, Mask, 2, qsize, s, SAVE_ROW_REG_BASE
	defUNPACK_SkippingWrite name, WriteMask, 0, qsize, s, SAVE_NO_REG
	defUNPACK_SkippingWrite name, WriteMask, 1, qsize, s, SAVE_NO_REG
	defUNPACK_SkippingWrite name, WriteMask, 2, qsize, s, SAVE_ROW_REG_BASE
	endm
	
defUNPACK_SkippingWrite2a S_16, 2
defUNPACK_SkippingWrite2a S_8, 1
defUNPACK_SkippingWrite2a V2_16, 4
defUNPACK_SkippingWrite2a V2_8, 2
defUNPACK_SkippingWrite2a V3_16, 6
defUNPACK_SkippingWrite2a V3_8, 3
defUNPACK_SkippingWrite2a V4_16, 8
defUNPACK_SkippingWrite2a V4_8, 4

end
