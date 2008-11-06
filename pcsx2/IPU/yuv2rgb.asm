;/********************************************************
; * Some code. Copyright (C) 2003 by Pascal Massimino.   *
; * All Rights Reserved.      (http://skal.planet-d.net) *
; * For Educational/Academic use ONLY. See 'LICENSE.TXT'.*
; ********************************************************/
;//////////////////////////////////////////////////////////
;// NASM macros
;//////////////////////////////////////////////////////////

%ifdef LINUX

;//////////////////////////////////////////////////////////
;  LINUX / egcs / macros 
;//////////////////////////////////////////////////////////

%macro extrn 1
  extern %1
  %define %1 %1
%endmacro
%macro globl 1
  global %1
  %define %1 %1
%endmacro

%macro DATA 0
[section data align=16 write alloc USE32]
%endmacro
%macro TEXT 0
[section text align=16 nowrite alloc exec USE32]
%endmacro

%endif    ; LINUX

;//////////////////////////////////////////////////////////

%ifdef WIN32

%macro extrn 1
  extern _%1
  %define %1 _%1
%endmacro

%macro globl 1
  global _%1
  %define %1 _%1
%endmacro

%macro DATA 0
[section .data align=16 write alloc USE32]
%endmacro
%macro TEXT 0
[section .text align=16 nowrite alloc exec USE32]
%endmacro

%endif    ; WIN32

;//////////////////////////////////////////////////////////
;
; MACRO for timing. NASM.
;     Total additional code size is 0xb0.
;     this keep code alignment right.

extrn Skl_Cur_Count_
extrn Skl_Print_Tics

%macro SKL_USE_RDSTC 0
extrn SKL_RDTSC_0_ASM
extrn SKL_RDTSC_1_ASM
extrn SKL_RDTSC_2_ASM
%endmacro
%define SKL_RDTSC_OFFSET 15 ; check value with skl_rdtsc.h...

%macro SKL_RDTSC_IN   0
   SKL_USE_RDSTC
   call SKL_RDTSC_0_ASM
.Skl_RDTSC_Loop_:
   call SKL_RDTSC_1_ASM
%endmacro

%macro SKL_RDTSC_OUT   0
   call SKL_RDTSC_2_ASM
   dec dword [Skl_Cur_Count_]
   jge near .Skl_RDTSC_Loop_
   push dword 53
   call Skl_Print_Tics
%endmacro

;//////////////////////////////////////////////////////////

globl Skl_YUV_To_RGB32_MMX

;//////////////////////////////////////////////////////////////////////

  ; eax: *U
  ; ebx: *V
  ; esi: *Y
  ; edx: Src_BpS
  ; edi: *Dst
  ; ebx: Dst_BpS
  ; ecx: Counter

%define RGBp  esp+20
%define Yp    esp+16
%define Up    esp+12
%define Vp    esp+8
%define xCnt  esp+4
%define yCnt  esp+0

Skl_YUV_To_RGB32_MMX:

  push ebx
  push esi
  push edi
  push ebp

  mov edi, [esp+4  +16] ; RGB
  mov ebp, [esp+12 +16] ; Y
  mov eax, [esp+16 +16] ; U
  mov ebx, [esp+20 +16] ; V
  mov edx, [esp+24 +16] ; Src_BpS
  mov ecx, [esp+28 +16] ; Width

  lea edi, [edi+4*ecx]  ; RGB += Width*sizeof(32b)
  lea ebp, [ebp+ecx]    ; ebp: Y1 = Y + Width
  add edx, ebp          ; edx: Y2 = Y1+ BpS
  push edi              ; [RGBp]
  push ebp              ; [Yp]
  shr ecx, 1            ; Width/=2
  lea eax, [eax+ecx]    ; U += W/2
  lea ebx, [ebx+ecx]    ; V += W/2
  push eax              ; [Up]
  push ebx              ; [Vp]

  neg ecx               ; ecx = -Width/2
  push ecx              ; save [xCnt]
  push eax              ; fake ([yCnt])
  
  mov ecx, [esp+32 +40] ; Height
  shr ecx, 1            ; /2

  mov esi, [Up]
  mov edi, [Vp]

  jmp .Go

align 16
.Loop_y
  dec ecx
  jg .Add

  add esp, 24   ; rid of all tmp
  pop ebp
  pop edi
  pop esi
  pop ebx

  ret

align 16
.Add
  mov edi, [esp+8 +40]  ; Dst_BpS
  mov esi, [esp+24 +40] ; Src_BpS
  mov edx, [RGBp]
  mov ebp, [Yp]
  lea edx, [edx+2*edi]  ; RGB += 2*Dst_BpS
  lea ebp, [ebp+2*esi]  ; Y   += 2*Src_BpS
  mov [RGBp], edx
  mov edi, [Vp]
  mov [Yp], ebp         ; Y1
  lea edx, [ebp+esi]    ; Y2

  lea edi, [edi+esi]    ; V += Src_BpS
  add esi, [Up]         ; U += Src_BpS
  mov [Vp], edi
  mov [Up], esi

.Go
  mov [yCnt], ecx
  mov ecx, [xCnt]

        ; 5210c@640x480

.Loop_x   ; edi,esi: U,V;  ebp,edx: Y1, Y2;  ecx: xCnt

    ; R = Y +       a.U
    ; G = Y + c.V + b.U
    ; B = Y + d.V

  movzx eax, byte [edi+ecx+0]
  movzx ebx, byte [esi+ecx+0]
  movq  mm0, [Skl_YUV_Tab32_MMX+0*2048 + eax*8]
  movzx eax, byte [edi+ecx+1]
  paddw mm0, [Skl_YUV_Tab32_MMX+1*2048 + ebx*8]
  movzx ebx, byte [esi+ecx+1]
  movq  mm4, [Skl_YUV_Tab32_MMX+0*2048 + eax*8]
  movzx eax, byte [ebp + 2*ecx+0]
  paddw mm4, [Skl_YUV_Tab32_MMX+1*2048 + ebx*8]
  movzx ebx, byte [ebp + 2*ecx+1]

  movq mm1, mm0
  movq mm2, mm0
  movq mm3, mm0
  movq mm5, mm4
  movq mm6, mm4
  movq mm7, mm4

  paddw mm0, [Skl_YUV_Tab32_MMX+2*2048 + eax*8]  
  movzx eax, byte [ebp + 2*ecx+2]
  paddw mm1, [Skl_YUV_Tab32_MMX+2*2048 + ebx*8]
  movzx ebx, byte [ebp + 2*ecx+3]
  packuswb mm0, mm1  
  paddw mm4, [Skl_YUV_Tab32_MMX+2*2048 + eax*8]
  movzx eax, byte [edx + 2*ecx+0]
  paddw mm5, [Skl_YUV_Tab32_MMX+2*2048 + ebx*8]

  packuswb mm4, mm5
  mov esi, [RGBp]  
  movzx ebx, byte [edx + 2*ecx+1]  
  movq [esi+8*ecx+0], mm0   ; 2x32b  
  movq [esi+8*ecx+8], mm4   ; 2x32b

  paddw mm2, [Skl_YUV_Tab32_MMX+2*2048 + eax*8]
  movzx eax, byte [edx + 2*ecx+2]  
  paddw mm3, [Skl_YUV_Tab32_MMX+2*2048 + ebx*8]
  movzx ebx, byte [edx + 2*ecx+3]
  packuswb mm2, mm3
  paddw mm6, [Skl_YUV_Tab32_MMX+2*2048 + eax*8]
  add esi, [esp+8  +40]
  paddw mm7, [Skl_YUV_Tab32_MMX+2*2048 + ebx*8]

  mov edi, [Vp]
  packuswb mm6, mm7
  movq [esi+8*ecx+0], mm2   ; 2x32b  
  movq [esi+8*ecx+8], mm6   ; 2x32b

  add ecx, 2
  mov esi, [Up]

  jl near .Loop_x

  mov ecx, [yCnt]
  jmp .Loop_y
