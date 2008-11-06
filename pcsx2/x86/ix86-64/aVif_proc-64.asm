extern _vifRegs:abs
extern _vifMaskRegs:abs
extern _vifRow:abs
extern _vifCol:abs
extern s_TempDecompress:abs


.code

UNPACK_Write0_Regular macro r0, CL, DEST_OFFSET, MOVDQA
 MOVDQA [rcx+DEST_OFFSET], r0
 endm

UNPACK_Write1_Regular macro r0, CL, DEST_OFFSET, MOVDQA
 MOVDQA [rcx], r0
 add rcx, rdi
 endm

UNPACK_Write0_Mask macro r0, CL, DEST_OFFSET, MOVDQA
 UNPACK_Write0_Regular r0, CL, DEST_OFFSET, MOVDQA
 endm

UNPACK_Write1_Mask macro r0, CL, DEST_OFFSET, MOVDQA
 UNPACK_Write1_Regular r0, CL, DEST_OFFSET, MOVDQA
 endm


UNPACK_Write0_WriteMask macro r0, CL, DEST_OFFSET, MOVDQA

 movdqa xmm3, [rax + 64*(CL) + 48]
 pand r0, xmm3
 pandn xmm3, [rcx]
 por r0, xmm3
 MOVDQA [rcx], r0
 add rcx, 16
 endm


UNPACK_Write1_WriteMask macro r0, CL, DEST_OFFSET, MOVDQA

 movdqa xmm3, [rax + 64*(0) + 48]
 pand r0, xmm3
 pandn xmm3, [rcx]
 por r0, xmm3
 MOVDQA [rcx], r0
 add rcx, rdi
 endm

UNPACK_Mask_SSE_0 macro r0
 pand r0, xmm3
 por r0, xmm5
 endm




UNPACK_Mask_SSE_1 macro r0

 pand r0, xmm3
 por r0, xmm5
 pand xmm3, xmm6
 paddd r0, xmm3
 endm



UNPACK_Mask_SSE_2 macro r0


 pand r0, xmm3
 pand xmm3, xmm6
 paddd xmm6, r0
 por r0, xmm5
 paddd r0, xmm3
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
 paddd r0, xmm6
 endm

UNPACK_Regular_SSE_2 macro r0
 paddd r0, xmm6
 movdqa xmm6, r0
 endm


UNPACK_Setup_Mask_SSE macro CL
 mov rax, [_vifMaskRegs]
 movdqa xmm4, [rax + 64*(CL) + 16]
 movdqa xmm5, [rax + 64*(CL) + 32]
 movdqa xmm3, [rax + 64*(CL)]
 pand xmm4, xmm6
 pand xmm5, xmm7
 por xmm5, xmm4
 endm

UNPACK_Start_Setup_Mask_SSE_0 macro CL
 UNPACK_Setup_Mask_SSE CL
 endm

UNPACK_Start_Setup_Mask_SSE_1 macro CL
 mov rax, [_vifMaskRegs]
 movdqa xmm4, [rax + 64*(CL) + 16]
 movdqa xmm5, [rax + 64*(CL) + 32]
 pand xmm4, xmm6
 pand xmm5, xmm7
 por xmm5, xmm4
 endm

UNPACK_Start_Setup_Mask_SSE_2 macro CL
 endm

UNPACK_Setup_Mask_SSE_0_1 macro CL
 endm
UNPACK_Setup_Mask_SSE_1_1 macro CL
 mov rax, [_vifMaskRegs]
 movdqa xmm3, [rax + 64*(0)]
 endm


UNPACK_Setup_Mask_SSE_2_1 macro CL

 mov rax, [_vifMaskRegs]
 movdqa xmm4, [rax + 64*(0) + 16]
 movdqa xmm5, [rax + 64*(0) + 32]
 movdqa xmm3, [rax + 64*(0)]
 pand xmm4, xmm6
 pand xmm5, xmm7
 por xmm5, xmm4
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
 add rcx, (16*qw)
 endm
UNPACK_INC_DST_1_Regular macro qw
 endm
UNPACK_INC_DST_0_Mask macro qw
 add rcx, (16*qw)
 endm
UNPACK_INC_DST_1_Mask macro qw
 endm
UNPACK_INC_DST_0_WriteMask macro qw
 endm
UNPACK_INC_DST_1_WriteMask macro qw
 endm


UNPACK4_SSE macro CL, TOTALCL, MaskType, ModeType
 @CatStr(UNPACK_Setup_, MaskType, _SSE_, ModeType, _, TOTALCL) CL+0
 @CatStr(UNPACK_, MaskType, _SSE_, ModeType) xmm0
 @CatStr(UNPACK_Write, TOTALCL, _, MaskType) xmm0, CL, 0, movdqa

 @CatStr(UNPACK_Setup_, MaskType, _SSE_, ModeType, _, TOTALCL) CL+1
 @CatStr(UNPACK_, MaskType, _SSE_, ModeType) xmm1
 @CatStr(UNPACK_Write, TOTALCL, _, MaskType) xmm1, CL+1, 16, movdqa

 @CatStr(UNPACK_Setup_, MaskType, _SSE_, ModeType, _, TOTALCL) CL+2
 @CatStr(UNPACK_, MaskType, _SSE_, ModeType) xmm2
 @CatStr(UNPACK_Write, TOTALCL, _, MaskType) xmm2, CL+2, 32, movdqa

 @CatStr(UNPACK_Setup_, MaskType, _SSE_, ModeType, _, TOTALCL) CL+3
 @CatStr(UNPACK_, MaskType, _SSE_, ModeType) xmm7
 @CatStr(UNPACK_Write, TOTALCL, _, MaskType) xmm7, CL+3, 48, movdqa

 @CatStr(UNPACK_INC_DST_, TOTALCL, _, MaskType) 4
 endm


UNPACK3_SSE macro CL, TOTALCL, MaskType, ModeType
 @CatStr(UNPACK_Setup_, MaskType, _SSE_, ModeType, _, TOTALCL) CL
 @CatStr(UNPACK_, MaskType, _SSE_, ModeType) xmm0
 @CatStr(UNPACK_Write, TOTALCL, _, MaskType) xmm0, CL, 0, movdqa

 @CatStr(UNPACK_Setup_, MaskType, _SSE_, ModeType, _, TOTALCL) CL+1
 @CatStr(UNPACK_, MaskType, _SSE_, ModeType) xmm1
 @CatStr(UNPACK_Write, TOTALCL, _, MaskType) xmm1, CL+1, 16, movdqa

 @CatStr(UNPACK_Setup_, MaskType, _SSE_, ModeType, _, TOTALCL) CL+2
 @CatStr(UNPACK_, MaskType, _SSE_, ModeType) xmm2
 @CatStr(UNPACK_Write, TOTALCL, _, MaskType) xmm2, CL+2, 32, movdqa

 @CatStr(UNPACK_INC_DST_, TOTALCL, _, MaskType) 3
 endm

UNPACK2_SSE macro CL, TOTALCL, MaskType, ModeType
 @CatStr(UNPACK_Setup_, MaskType, _SSE_, ModeType, _, TOTALCL) CL
 @CatStr(UNPACK_, MaskType, _SSE_, ModeType) xmm0
 @CatStr(UNPACK_Write, TOTALCL, _, MaskType) xmm0, CL, 0, movdqa

 @CatStr(UNPACK_Setup_, MaskType, _SSE_, ModeType, _, TOTALCL) CL+1
 @CatStr(UNPACK_, MaskType, _SSE_, ModeType) xmm1
 @CatStr(UNPACK_Write, TOTALCL, _, MaskType) xmm1, CL+1, 16, movdqa

 @CatStr(UNPACK_INC_DST_, TOTALCL, _, MaskType) 2
 endm

UNPACK1_SSE macro CL, TOTALCL, MaskType, ModeType
 @CatStr(UNPACK_Setup_, MaskType, _SSE_, ModeType, _, TOTALCL) CL
 @CatStr(UNPACK_, MaskType, _SSE_, ModeType) xmm0
 @CatStr(UNPACK_Write, TOTALCL, _, MaskType) xmm0, CL, 0, movdqa

 @CatStr(UNPACK_INC_DST_, TOTALCL, _, MaskType) 1
 endm



UNPACK_S_32SSE_4x macro CL, TOTALCL, MaskType, ModeType, MOVDQA
 MOVDQA xmm7, [rdx]

 pshufd xmm0, xmm7, 0
 pshufd xmm1, xmm7, 055h
 pshufd xmm2, xmm7, 0aah
 pshufd xmm7, xmm7, 0ffh

 UNPACK4_SSE CL, TOTALCL, MaskType, ModeType

 add rdx, 16
 endm

UNPACK_S_32SSE_4A macro CL, TOTALCL, MaskType, ModeType
 UNPACK_S_32SSE_4x CL, TOTALCL, MaskType, ModeType, movdqa
 endm
UNPACK_S_32SSE_4 macro CL, TOTALCL, MaskType, ModeType
 UNPACK_S_32SSE_4x CL, TOTALCL, MaskType, ModeType, movdqu
 endm

UNPACK_S_32SSE_3x macro CL, TOTALCL, MaskType, ModeType, MOVDQA
 MOVDQA xmm2, [rdx]

 pshufd xmm0, xmm2, 0
 pshufd xmm1, xmm2, 055h
 pshufd xmm2, xmm2, 0aah

 UNPACK3_SSE CL, TOTALCL, MaskType, ModeType

 add rdx, 12
 endm

UNPACK_S_32SSE_3A macro CL, TOTALCL, MaskType, ModeType
 UNPACK_S_32SSE_3x CL, TOTALCL, MaskType, ModeType, movdqa
 endm
UNPACK_S_32SSE_3 macro CL, TOTALCL, MaskType, ModeType
 UNPACK_S_32SSE_3x CL, TOTALCL, MaskType, ModeType, movdqu
 endm

UNPACK_S_32SSE_2 macro CL, TOTALCL, MaskType, ModeType
 movq xmm1, QWORD PTR [rdx]

 pshufd xmm0, xmm1, 0
 pshufd xmm1, xmm1, 055h

 UNPACK2_SSE CL, TOTALCL, MaskType, ModeType

 add rdx, 8
 endm

UNPACK_S_32SSE_2A macro CL, TOTALCL, MaskType, ModeType
 UNPACK_S_32SSE_2 CL, TOTALCL, MaskType, ModeType
 endm

UNPACK_S_32SSE_1 macro CL, TOTALCL, MaskType, ModeType
 movd xmm0, dword ptr [rdx]
 pshufd xmm0, xmm0, 0

 UNPACK1_SSE CL, TOTALCL, MaskType, ModeType

 add rdx, 4
 endm

UNPACK_S_32SSE_1A macro CL, TOTALCL, MaskType, ModeType
 UNPACK_S_32SSE_1 CL, TOTALCL, MaskType, ModeType
 endm


UNPACK_S_16SSE_4 macro CL, TOTALCL, MaskType, ModeType
 movq xmm7, QWORD PTR [rdx]
 punpcklwd xmm7, xmm7
 UNPACK_RIGHTSHIFT xmm7, 16

 pshufd xmm0, xmm7, 0
 pshufd xmm1, xmm7, 055h
 pshufd xmm2, xmm7, 0aah
 pshufd xmm7, xmm7, 0ffh

 UNPACK4_SSE CL, TOTALCL, MaskType, ModeType

 add rdx, 8
 endm

UNPACK_S_16SSE_4A macro CL, TOTALCL, MaskType, ModeType
 UNPACK_S_16SSE_4 CL, TOTALCL, MaskType, ModeType
 endm

UNPACK_S_16SSE_3 macro CL, TOTALCL, MaskType, ModeType
 movq xmm2, QWORD PTR [rdx]
 punpcklwd xmm2, xmm2
 UNPACK_RIGHTSHIFT xmm2, 16

 pshufd xmm0, xmm2, 0
 pshufd xmm1, xmm2, 055h
 pshufd xmm2, xmm2, 0aah

 UNPACK3_SSE CL, TOTALCL, MaskType, ModeType
 add rdx, 6
 endm

UNPACK_S_16SSE_3A macro CL, TOTALCL, MaskType, ModeType
 UNPACK_S_16SSE_3 CL, TOTALCL, MaskType, ModeType
 endm

UNPACK_S_16SSE_2 macro CL, TOTALCL, MaskType, ModeType
 movd xmm1, dword ptr [rdx]
 punpcklwd xmm1, xmm1
 UNPACK_RIGHTSHIFT xmm1, 16

 pshufd xmm0, xmm1, 0
 pshufd xmm1, xmm1, 055h

 UNPACK2_SSE CL, TOTALCL, MaskType, ModeType

 add rdx, 4
 endm

UNPACK_S_16SSE_2A macro CL, TOTALCL, MaskType, ModeType
 UNPACK_S_16SSE_2 CL, TOTALCL, MaskType, ModeType
 endm

UNPACK_S_16SSE_1 macro CL, TOTALCL, MaskType, ModeType
 movd xmm0, dword ptr [rdx]
 punpcklwd xmm0, xmm0
 UNPACK_RIGHTSHIFT xmm0, 16
 pshufd xmm0, xmm0, 0

 UNPACK1_SSE CL, TOTALCL, MaskType, ModeType

 add rdx, 2
 endm

UNPACK_S_16SSE_1A macro CL, TOTALCL, MaskType, ModeType
 UNPACK_S_16SSE_1 CL, TOTALCL, MaskType, ModeType
 endm


UNPACK_S_8SSE_4 macro CL, TOTALCL, MaskType, ModeType
 movd xmm7, dword ptr [rdx]
 punpcklbw xmm7, xmm7
 punpcklwd xmm7, xmm7
 UNPACK_RIGHTSHIFT xmm7, 24

 pshufd xmm0, xmm7, 0
 pshufd xmm1, xmm7, 055h
 pshufd xmm2, xmm7, 0aah
 pshufd xmm7, xmm7, 0ffh

 UNPACK4_SSE CL, TOTALCL, MaskType, ModeType

 add rdx, 4
 endm

UNPACK_S_8SSE_4A macro CL, TOTALCL, MaskType, ModeType
 UNPACK_S_8SSE_4 CL, TOTALCL, MaskType, ModeType
 endm

UNPACK_S_8SSE_3 macro CL, TOTALCL, MaskType, ModeType
 movd xmm2, dword ptr [rdx]
 punpcklbw xmm2, xmm2
 punpcklwd xmm2, xmm2
 UNPACK_RIGHTSHIFT xmm2, 24

 pshufd xmm0, xmm2, 0
 pshufd xmm1, xmm2, 055h
 pshufd xmm2, xmm2, 0aah

 UNPACK3_SSE CL, TOTALCL, MaskType, ModeType

 add rdx, 3
 endm

UNPACK_S_8SSE_3A macro CL, TOTALCL, MaskType, ModeType
 UNPACK_S_8SSE_3 CL, TOTALCL, MaskType, ModeType
 endm

UNPACK_S_8SSE_2 macro CL, TOTALCL, MaskType, ModeType
 movd xmm1, dword ptr [rdx]
 punpcklbw xmm1, xmm1
 punpcklwd xmm1, xmm1
 UNPACK_RIGHTSHIFT xmm1, 24

 pshufd xmm0, xmm1, 0
 pshufd xmm1, xmm1, 055h

 UNPACK2_SSE CL, TOTALCL, MaskType, ModeType

 add rdx, 2
 endm

UNPACK_S_8SSE_2A macro CL, TOTALCL, MaskType, ModeType
 UNPACK_S_8SSE_2 CL, TOTALCL, MaskType, ModeType
 endm

UNPACK_S_8SSE_1 macro CL, TOTALCL, MaskType, ModeType
 movd xmm0, dword ptr [rdx]
 punpcklbw xmm0, xmm0
 punpcklwd xmm0, xmm0
 UNPACK_RIGHTSHIFT xmm0, 24
 pshufd xmm0, xmm0, 0

 UNPACK1_SSE CL, TOTALCL, MaskType, ModeType

 inc rdx
 endm

UNPACK_S_8SSE_1A macro CL, TOTALCL, MaskType, ModeType
 UNPACK_S_8SSE_1 CL, TOTALCL, MaskType, ModeType
 endm


UNPACK_V2_32SSE_4A macro CL, TOTALCL, MaskType, ModeType
 MOVDQA xmm0, [rdx]
 MOVDQA xmm2, [rdx+16]

 pshufd xmm1, xmm0, 0eeh
 pshufd xmm7, xmm2, 0eeh

 UNPACK4_SSE CL, TOTALCL, MaskType, ModeType

 add rdx, 32
 endm

UNPACK_V2_32SSE_4 macro CL, TOTALCL, MaskType, ModeType
 movq xmm0, QWORD PTR [rdx]
 movq xmm1, QWORD PTR [rdx+8]
 movq xmm2, QWORD PTR [rdx+16]
 movq xmm7, QWORD PTR [rdx+24]

 UNPACK4_SSE CL, TOTALCL, MaskType, ModeType

 add rdx, 32
 endm

UNPACK_V2_32SSE_3A macro CL, TOTALCL, MaskType, ModeType
 MOVDQA xmm0, [rdx]
 movq xmm2, QWORD PTR [rdx+16]
 pshufd xmm1, xmm0, 0eeh

 UNPACK3_SSE CL, TOTALCL, MaskType, ModeType

 add rdx, 24
 endm

UNPACK_V2_32SSE_3 macro CL, TOTALCL, MaskType, ModeType
 movq xmm0, QWORD PTR [rdx]
 movq xmm1, QWORD PTR [rdx+8]
 movq xmm2, QWORD PTR [rdx+16]

 UNPACK3_SSE CL, TOTALCL, MaskType, ModeType

 add rdx, 24
 endm

UNPACK_V2_32SSE_2 macro CL, TOTALCL, MaskType, ModeType
 movq xmm0, QWORD PTR [rdx]
 movq xmm1, QWORD PTR [rdx+8]

 UNPACK2_SSE CL, TOTALCL, MaskType, ModeType

 add rdx, 16
 endm

UNPACK_V2_32SSE_2A macro CL, TOTALCL, MaskType, ModeType
 UNPACK_V2_32SSE_2 CL, TOTALCL, MaskType, ModeType
 endm

UNPACK_V2_32SSE_1 macro CL, TOTALCL, MaskType, ModeType
 movq xmm0, QWORD PTR [rdx]

 UNPACK1_SSE CL, TOTALCL, MaskType, ModeType

 add rdx, 8
 endm

UNPACK_V2_32SSE_1A macro CL, TOTALCL, MaskType, ModeType
 UNPACK_V2_32SSE_1 CL, TOTALCL, MaskType, ModeType
 endm


UNPACK_V2_16SSE_4A macro CL, TOTALCL, MaskType, ModeType
 punpcklwd xmm0, [rdx]
 punpckhwd xmm2, [rdx]

 UNPACK_RIGHTSHIFT xmm0, 16
 UNPACK_RIGHTSHIFT xmm2, 16

 punpckhqdq xmm1, xmm0
 punpckhqdq xmm7, xmm2

 punpcklqdq xmm0, xmm0
 punpcklqdq xmm2, xmm2
 punpckhqdq xmm1, xmm1
 punpckhqdq xmm7, xmm7

 UNPACK4_SSE CL, TOTALCL, MaskType, ModeType
 add rdx, 16
 endm

UNPACK_V2_16SSE_4 macro CL, TOTALCL, MaskType, ModeType
 movdqu xmm0, [rdx]

 punpckhwd xmm2, xmm0
 punpcklwd xmm0, xmm0

 UNPACK_RIGHTSHIFT xmm0, 16
 UNPACK_RIGHTSHIFT xmm2, 16


 punpckhqdq xmm1, xmm0
 punpckhqdq xmm7, xmm2

 punpcklqdq xmm0, xmm0
 punpcklqdq xmm2, xmm2
 punpckhqdq xmm1, xmm1
 punpckhqdq xmm7, xmm7

 UNPACK4_SSE CL, TOTALCL, MaskType, ModeType

 add rdx, 16
 endm

UNPACK_V2_16SSE_3A macro CL, TOTALCL, MaskType, ModeType
 punpcklwd xmm0, [rdx]
 punpckhwd xmm2, [rdx]

 UNPACK_RIGHTSHIFT xmm0, 16
 UNPACK_RIGHTSHIFT xmm2, 16


 punpckhqdq xmm1, xmm0

 punpcklqdq xmm0, xmm0
 punpcklqdq xmm2, xmm2
 punpckhqdq xmm1, xmm1

 UNPACK3_SSE CL, TOTALCL, MaskType, ModeType

 add rdx, 12
 endm

UNPACK_V2_16SSE_3 macro CL, TOTALCL, MaskType, ModeType
 movdqu xmm0, [rdx]

 punpckhwd xmm2, xmm0
 punpcklwd xmm0, xmm0

 UNPACK_RIGHTSHIFT xmm0, 16
 UNPACK_RIGHTSHIFT xmm2, 16


 punpckhqdq xmm1, xmm0

 punpcklqdq xmm0, xmm0
 punpcklqdq xmm2, xmm2
 punpckhqdq xmm1, xmm1

 UNPACK3_SSE CL, TOTALCL, MaskType, ModeType

 add rdx, 12
 endm

UNPACK_V2_16SSE_2A macro CL, TOTALCL, MaskType, ModeType
 punpcklwd xmm0, [rdx]
 UNPACK_RIGHTSHIFT xmm0, 16


 punpckhqdq xmm1, xmm0

 punpcklqdq xmm0, xmm0
 punpckhqdq xmm1, xmm1

 UNPACK2_SSE CL, TOTALCL, MaskType, ModeType

 add rdx, 8
 endm

UNPACK_V2_16SSE_2 macro CL, TOTALCL, MaskType, ModeType
 movq xmm0, QWORD PTR [rdx]
 punpcklwd xmm0, xmm0
 UNPACK_RIGHTSHIFT xmm0, 16


 punpckhqdq xmm1, xmm0

 punpcklqdq xmm0, xmm0
 punpckhqdq xmm1, xmm1

 UNPACK2_SSE CL, TOTALCL, MaskType, ModeType

 add rdx, 8
 endm

UNPACK_V2_16SSE_1A macro CL, TOTALCL, MaskType, ModeType
 punpcklwd xmm0, [rdx]
 UNPACK_RIGHTSHIFT xmm0, 16
 punpcklqdq xmm0, xmm0

 UNPACK1_SSE CL, TOTALCL, MaskType, ModeType

 add rdx, 4
 endm

UNPACK_V2_16SSE_1 macro CL, TOTALCL, MaskType, ModeType
 movd xmm0, dword ptr [rdx]
 punpcklwd xmm0, xmm0
 UNPACK_RIGHTSHIFT xmm0, 16
 punpcklqdq xmm0, xmm0

 UNPACK1_SSE CL, TOTALCL, MaskType, ModeType

 add rdx, 4
 endm



UNPACK_V2_8SSE_4 macro CL, TOTALCL, MaskType, ModeType
 movq xmm0, QWORD PTR [rdx]

 punpcklbw xmm0, xmm0
 punpckhwd xmm2, xmm0
 punpcklwd xmm0, xmm0

 UNPACK_RIGHTSHIFT xmm0, 24
 UNPACK_RIGHTSHIFT xmm2, 24


 punpckhqdq xmm1, xmm0
 punpckhqdq xmm7, xmm2

 punpcklqdq xmm0, xmm0
 punpcklqdq xmm2, xmm2
 punpckhqdq xmm1, xmm1
 punpckhqdq xmm7, xmm7

 UNPACK4_SSE CL, TOTALCL, MaskType, ModeType

 add rdx, 8
 endm

UNPACK_V2_8SSE_4A macro CL, TOTALCL, MaskType, ModeType
 UNPACK_V2_8SSE_4 CL, TOTALCL, MaskType, ModeType
 endm

UNPACK_V2_8SSE_3 macro CL, TOTALCL, MaskType, ModeType
 movq xmm0, QWORD PTR [rdx]

 punpcklbw xmm0, xmm0
 punpckhwd xmm2, xmm0
 punpcklwd xmm0, xmm0

 UNPACK_RIGHTSHIFT xmm0, 24
 UNPACK_RIGHTSHIFT xmm2, 24


 punpckhqdq xmm1, xmm0

 punpcklqdq xmm0, xmm0
 punpcklqdq xmm2, xmm2
 punpckhqdq xmm1, xmm1

 UNPACK3_SSE CL, TOTALCL, MaskType, ModeType

 add rdx, 6
 endm

UNPACK_V2_8SSE_3A macro CL, TOTALCL, MaskType, ModeType
 UNPACK_V2_8SSE_3 CL, TOTALCL, MaskType, ModeType
 endm

UNPACK_V2_8SSE_2 macro CL, TOTALCL, MaskType, ModeType
 movd xmm0, dword ptr [rdx]
 punpcklbw xmm0, xmm0
 punpcklwd xmm0, xmm0
 UNPACK_RIGHTSHIFT xmm0, 24


 punpckhqdq xmm1, xmm0

 punpcklqdq xmm0, xmm0
 punpckhqdq xmm1, xmm1

 UNPACK2_SSE CL, TOTALCL, MaskType, ModeType

 add rdx, 4
 endm

UNPACK_V2_8SSE_2A macro CL, TOTALCL, MaskType, ModeType
 UNPACK_V2_8SSE_2 CL, TOTALCL, MaskType, ModeType
 endm

UNPACK_V2_8SSE_1 macro CL, TOTALCL, MaskType, ModeType
 movd xmm0, dword ptr [rdx]
 punpcklbw xmm0, xmm0
 punpcklwd xmm0, xmm0
 UNPACK_RIGHTSHIFT xmm0, 24
 punpcklqdq xmm0, xmm0

 UNPACK1_SSE CL, TOTALCL, MaskType, ModeType

 add rdx, 2
 endm

UNPACK_V2_8SSE_1A macro CL, TOTALCL, MaskType, ModeType
 UNPACK_V2_8SSE_1 CL, TOTALCL, MaskType, ModeType
 endm


UNPACK_V3_32SSE_4x macro CL, TOTALCL, MaskType, ModeType, MOVDQA
 MOVDQA xmm0, [rdx]
 movdqu xmm1, [rdx+12]

 @CatStr(UNPACK_Setup_, MaskType, _SSE_, ModeType, _, TOTALCL) CL+0
 @CatStr(UNPACK_, MaskType, _SSE_, ModeType) xmm0
 @CatStr(UNPACK_Write, TOTALCL, _, MaskType) xmm0, CL, 0, movdqa

 @CatStr(UNPACK_Setup_, MaskType, _SSE_, ModeType, _, TOTALCL) CL+1
 @CatStr(UNPACK_, MaskType, _SSE_, ModeType) xmm1
 @CatStr(UNPACK_Write, TOTALCL, _, MaskType) xmm1, CL+1, 16, movdqa


 MOVDQA xmm7, [rdx+32]
 movdqu xmm2, [rdx+24]
 psrldq xmm7, 4

 @CatStr(UNPACK_Setup_, MaskType, _SSE_, ModeType, _, TOTALCL) CL+2
 @CatStr(UNPACK_, MaskType, _SSE_, ModeType) xmm2
 @CatStr(UNPACK_Write, TOTALCL, _, MaskType) xmm2, CL+2, 32, movdqa

 @CatStr(UNPACK_Setup_, MaskType, _SSE_, ModeType, _, TOTALCL) CL+3
 @CatStr(UNPACK_, MaskType, _SSE_, ModeType) xmm7
 @CatStr(UNPACK_Write, TOTALCL, _, MaskType) xmm7, CL+3, 48, movdqa

 @CatStr(UNPACK_INC_DST_, TOTALCL, _, MaskType) 4

 add rdx, 48
 endm

UNPACK_V3_32SSE_4A macro CL, TOTALCL, MaskType, ModeType
 UNPACK_V3_32SSE_4x CL, TOTALCL, MaskType, ModeType, movdqa
 endm
UNPACK_V3_32SSE_4 macro CL, TOTALCL, MaskType, ModeType
 UNPACK_V3_32SSE_4x CL, TOTALCL, MaskType, ModeType, movdqu
 endm

UNPACK_V3_32SSE_3x macro CL, TOTALCL, MaskType, ModeType, MOVDQA
 MOVDQA xmm0, [rdx]
 movdqu xmm1, [rdx+12]

 @CatStr(UNPACK_Setup_, MaskType, _SSE_, ModeType, _, TOTALCL) CL
 @CatStr(UNPACK_, MaskType, _SSE_, ModeType) xmm0
 @CatStr(UNPACK_Write, TOTALCL, _, MaskType) xmm0, CL, 0, movdqa

 @CatStr(UNPACK_Setup_, MaskType, _SSE_, ModeType, _, TOTALCL) CL+1
 @CatStr(UNPACK_, MaskType, _SSE_, ModeType) xmm1
 @CatStr(UNPACK_Write, TOTALCL, _, MaskType) xmm1, CL+1, 16, movdqa

 movdqu xmm2, [rdx+24]

 @CatStr(UNPACK_Setup_, MaskType, _SSE_, ModeType, _, TOTALCL) CL+2
 @CatStr(UNPACK_, MaskType, _SSE_, ModeType) xmm2
 @CatStr(UNPACK_Write, TOTALCL, _, MaskType) xmm2, CL+2, 32, movdqa

 @CatStr(UNPACK_INC_DST_, TOTALCL, _, MaskType) 3

 add rdx, 36
 endm

UNPACK_V3_32SSE_3A macro CL, TOTALCL, MaskType, ModeType
 UNPACK_V3_32SSE_3x CL, TOTALCL, MaskType, ModeType, movdqa
 endm
UNPACK_V3_32SSE_3 macro CL, TOTALCL, MaskType, ModeType
 UNPACK_V3_32SSE_3x CL, TOTALCL, MaskType, ModeType, movdqu
 endm

UNPACK_V3_32SSE_2x macro CL, TOTALCL, MaskType, ModeType, MOVDQA
 MOVDQA xmm0, [rdx]
 movdqu xmm1, [rdx+12]

 UNPACK2_SSE CL, TOTALCL, MaskType, ModeType

 add rdx, 24
 endm

UNPACK_V3_32SSE_2A macro CL, TOTALCL, MaskType, ModeType
 UNPACK_V3_32SSE_2x CL, TOTALCL, MaskType, ModeType, movdqa
 endm
UNPACK_V3_32SSE_2 macro CL, TOTALCL, MaskType, ModeType
 UNPACK_V3_32SSE_2x CL, TOTALCL, MaskType, ModeType, movdqu
 endm

UNPACK_V3_32SSE_1x macro CL, TOTALCL, MaskType, ModeType, MOVDQA
 MOVDQA xmm0, [rdx]

 UNPACK1_SSE CL, TOTALCL, MaskType, ModeType

 add rdx, 12
 endm

UNPACK_V3_32SSE_1A macro CL, TOTALCL, MaskType, ModeType
 UNPACK_V3_32SSE_1x CL, TOTALCL, MaskType, ModeType, movdqa
 endm
UNPACK_V3_32SSE_1 macro CL, TOTALCL, MaskType, ModeType
 UNPACK_V3_32SSE_1x CL, TOTALCL, MaskType, ModeType, movdqu
 endm


UNPACK_V3_16SSE_4 macro CL, TOTALCL, MaskType, ModeType
 movq xmm0, QWORD PTR [rdx]
 movq xmm1, QWORD PTR [rdx+6]

 punpcklwd xmm0, xmm0
 movq xmm2, QWORD PTR [rdx+12]
 punpcklwd xmm1, xmm1
 UNPACK_RIGHTSHIFT xmm0, 16
 movq xmm7, QWORD PTR [rdx+18]
 UNPACK_RIGHTSHIFT xmm1, 16
 punpcklwd xmm2, xmm2
 punpcklwd xmm7, xmm7

 UNPACK_RIGHTSHIFT xmm2, 16
 UNPACK_RIGHTSHIFT xmm7, 16

 UNPACK4_SSE CL, TOTALCL, MaskType, ModeType

 add rdx, 24
 endm

UNPACK_V3_16SSE_4A macro CL, TOTALCL, MaskType, ModeType
 UNPACK_V3_16SSE_4 CL, TOTALCL, MaskType, ModeType
 endm

UNPACK_V3_16SSE_3 macro CL, TOTALCL, MaskType, ModeType
 movq xmm0, QWORD PTR [rdx]
 movq xmm1, QWORD PTR [rdx+6]

 punpcklwd xmm0, xmm0
 movq xmm2, QWORD PTR [rdx+12]
 punpcklwd xmm1, xmm1
 UNPACK_RIGHTSHIFT xmm0, 16
 punpcklwd xmm2, xmm2

 UNPACK_RIGHTSHIFT xmm1, 16
 UNPACK_RIGHTSHIFT xmm2, 16

 UNPACK3_SSE CL, TOTALCL, MaskType, ModeType

 add rdx, 18
 endm

UNPACK_V3_16SSE_3A macro CL, TOTALCL, MaskType, ModeType
 UNPACK_V3_16SSE_3 CL, TOTALCL, MaskType, ModeType
 endm

UNPACK_V3_16SSE_2 macro CL, TOTALCL, MaskType, ModeType
 movq xmm0, QWORD PTR [rdx]
 movq xmm1, QWORD PTR [rdx+6]

 punpcklwd xmm0, xmm0
 punpcklwd xmm1, xmm1

 UNPACK_RIGHTSHIFT xmm0, 16
 UNPACK_RIGHTSHIFT xmm1, 16

 UNPACK2_SSE CL, TOTALCL, MaskType, ModeType

 add rdx, 12
 endm

UNPACK_V3_16SSE_2A macro CL, TOTALCL, MaskType, ModeType
 UNPACK_V3_16SSE_2 CL, TOTALCL, MaskType, ModeType
 endm

UNPACK_V3_16SSE_1 macro CL, TOTALCL, MaskType, ModeType
 movq xmm0, QWORD PTR [rdx]
 punpcklwd xmm0, xmm0
 UNPACK_RIGHTSHIFT xmm0, 16

 UNPACK1_SSE CL, TOTALCL, MaskType, ModeType

 add rdx, 6
 endm

UNPACK_V3_16SSE_1A macro CL, TOTALCL, MaskType, ModeType
 UNPACK_V3_16SSE_1 CL, TOTALCL, MaskType, ModeType
 endm


UNPACK_V3_8SSE_4 macro CL, TOTALCL, MaskType, ModeType
 movq xmm1, QWORD PTR [rdx]
 movq xmm7, QWORD PTR [rdx+6]

 punpcklbw xmm1, xmm1
 punpcklbw xmm7, xmm7
 punpcklwd xmm0, xmm1
 psrldq xmm1, 6
 punpcklwd xmm2, xmm7
 psrldq xmm7, 6
 punpcklwd xmm1, xmm1
 UNPACK_RIGHTSHIFT xmm0, 24
 punpcklwd xmm7, xmm7

 UNPACK_RIGHTSHIFT xmm2, 24
 UNPACK_RIGHTSHIFT xmm1, 24
 UNPACK_RIGHTSHIFT xmm7, 24

 UNPACK4_SSE CL, TOTALCL, MaskType, ModeType

 add rdx, 12
 endm

UNPACK_V3_8SSE_4A macro CL, TOTALCL, MaskType, ModeType
 UNPACK_V3_8SSE_4 CL, TOTALCL, MaskType, ModeType
 endm

UNPACK_V3_8SSE_3 macro CL, TOTALCL, MaskType, ModeType
 movd xmm0, dword ptr [rdx]
 movd xmm1, dword ptr [rdx+3]

 punpcklbw xmm0, xmm0
 movd xmm2, dword ptr [rdx+6]
 punpcklbw xmm1, xmm1
 punpcklwd xmm0, xmm0
 punpcklbw xmm2, xmm2

 punpcklwd xmm1, xmm1
 punpcklwd xmm2, xmm2

 UNPACK_RIGHTSHIFT xmm0, 24
 UNPACK_RIGHTSHIFT xmm1, 24
 UNPACK_RIGHTSHIFT xmm2, 24

 UNPACK3_SSE CL, TOTALCL, MaskType, ModeType

 add rdx, 9
 endm

UNPACK_V3_8SSE_3A macro CL, TOTALCL, MaskType, ModeType
 UNPACK_V3_8SSE_3 CL, TOTALCL, MaskType, ModeType
 endm

UNPACK_V3_8SSE_2 macro CL, TOTALCL, MaskType, ModeType
 movd xmm0, dword ptr [rdx]
 movd xmm1, dword ptr [rdx+3]

 punpcklbw xmm0, xmm0
 punpcklbw xmm1, xmm1

 punpcklwd xmm0, xmm0
 punpcklwd xmm1, xmm1

 UNPACK_RIGHTSHIFT xmm0, 24
 UNPACK_RIGHTSHIFT xmm1, 24

 UNPACK2_SSE CL, TOTALCL, MaskType, ModeType

 add rdx, 6
 endm

UNPACK_V3_8SSE_2A macro CL, TOTALCL, MaskType, ModeType
 UNPACK_V3_8SSE_2 CL, TOTALCL, MaskType, ModeType
 endm

UNPACK_V3_8SSE_1 macro CL, TOTALCL, MaskType, ModeType
 movd xmm0, dword ptr [rdx]
 punpcklbw xmm0, xmm0
 punpcklwd xmm0, xmm0
 UNPACK_RIGHTSHIFT xmm0, 24

 UNPACK1_SSE CL, TOTALCL, MaskType, ModeType

 add rdx, 3
 endm

UNPACK_V3_8SSE_1A macro CL, TOTALCL, MaskType, ModeType
 UNPACK_V3_8SSE_1 CL, TOTALCL, MaskType, ModeType
 endm


UNPACK_V4_32SSE_4A macro CL, TOTALCL, MaskType, ModeType
 movdqa xmm0, [rdx]
 movdqa xmm1, [rdx+16]
 movdqa xmm2, [rdx+32]
 movdqa xmm7, [rdx+48]

 UNPACK4_SSE CL, TOTALCL, MaskType, ModeType

 add rdx, 64
 endm

UNPACK_V4_32SSE_4 macro CL, TOTALCL, MaskType, ModeType
 movdqu xmm0, [rdx]
 movdqu xmm1, [rdx+16]
 movdqu xmm2, [rdx+32]
 movdqu xmm7, [rdx+48]

 UNPACK4_SSE CL, TOTALCL, MaskType, ModeType

 add rdx, 64
 endm

UNPACK_V4_32SSE_3A macro CL, TOTALCL, MaskType, ModeType
 movdqa xmm0, [rdx]
 movdqa xmm1, [rdx+16]
 movdqa xmm2, [rdx+32]

 UNPACK3_SSE CL, TOTALCL, MaskType, ModeType

 add rdx, 48
 endm

UNPACK_V4_32SSE_3 macro CL, TOTALCL, MaskType, ModeType
 movdqu xmm0, [rdx]
 movdqu xmm1, [rdx+16]
 movdqu xmm2, [rdx+32]

 UNPACK3_SSE CL, TOTALCL, MaskType, ModeType

 add rdx, 48
 endm

UNPACK_V4_32SSE_2A macro CL, TOTALCL, MaskType, ModeType
 movdqa xmm0, [rdx]
 movdqa xmm1, [rdx+16]

 UNPACK2_SSE CL, TOTALCL, MaskType, ModeType

 add rdx, 32
 endm

UNPACK_V4_32SSE_2 macro CL, TOTALCL, MaskType, ModeType
 movdqu xmm0, [rdx]
 movdqu xmm1, [rdx+16]

 UNPACK2_SSE CL, TOTALCL, MaskType, ModeType

 add rdx, 32
 endm

UNPACK_V4_32SSE_1A macro CL, TOTALCL, MaskType, ModeType
 movdqa xmm0, [rdx]

 UNPACK1_SSE CL, TOTALCL, MaskType, ModeType

 add rdx, 16
 endm

UNPACK_V4_32SSE_1 macro CL, TOTALCL, MaskType, ModeType
 movdqu xmm0, [rdx]

 UNPACK1_SSE CL, TOTALCL, MaskType, ModeType

 add rdx, 16
 endm


UNPACK_V4_16SSE_4A macro CL, TOTALCL, MaskType, ModeType

 punpcklwd xmm0, [rdx]
 punpckhwd xmm1, [rdx]
 punpcklwd xmm2, [rdx+16]
 punpckhwd xmm7, [rdx+16]

 UNPACK_RIGHTSHIFT xmm1, 16
 UNPACK_RIGHTSHIFT xmm7, 16
 UNPACK_RIGHTSHIFT xmm0, 16
 UNPACK_RIGHTSHIFT xmm2, 16

 UNPACK4_SSE CL, TOTALCL, MaskType, ModeType

 add rdx, 32
 endm

UNPACK_V4_16SSE_4 macro CL, TOTALCL, MaskType, ModeType
 movdqu xmm0, [rdx]
 movdqu xmm2, [rdx+16]

 punpckhwd xmm1, xmm0
 punpckhwd xmm7, xmm2
 punpcklwd xmm0, xmm0
 punpcklwd xmm2, xmm2

 UNPACK_RIGHTSHIFT xmm1, 16
 UNPACK_RIGHTSHIFT xmm7, 16
 UNPACK_RIGHTSHIFT xmm0, 16
 UNPACK_RIGHTSHIFT xmm2, 16

 UNPACK4_SSE CL, TOTALCL, MaskType, ModeType

 add rdx, 32
 endm

UNPACK_V4_16SSE_3A macro CL, TOTALCL, MaskType, ModeType
 punpcklwd xmm0, [rdx]
 punpckhwd xmm1, [rdx]
 punpcklwd xmm2, [rdx+16]

 UNPACK_RIGHTSHIFT xmm0, 16
 UNPACK_RIGHTSHIFT xmm1, 16
 UNPACK_RIGHTSHIFT xmm2, 16

 UNPACK3_SSE CL, TOTALCL, MaskType, ModeType

 add rdx, 24
 endm

UNPACK_V4_16SSE_3 macro CL, TOTALCL, MaskType, ModeType
 movdqu xmm0, [rdx]
 movq xmm2, QWORD PTR [rdx+16]

 punpckhwd xmm1, xmm0
 punpcklwd xmm0, xmm0
 punpcklwd xmm2, xmm2

 UNPACK_RIGHTSHIFT xmm0, 16
 UNPACK_RIGHTSHIFT xmm1, 16
 UNPACK_RIGHTSHIFT xmm2, 16

 UNPACK3_SSE CL, TOTALCL, MaskType, ModeType

 add rdx, 24
 endm

UNPACK_V4_16SSE_2A macro CL, TOTALCL, MaskType, ModeType
 punpcklwd xmm0, [rdx]
 punpckhwd xmm1, [rdx]

 UNPACK_RIGHTSHIFT xmm0, 16
 UNPACK_RIGHTSHIFT xmm1, 16

 UNPACK2_SSE CL, TOTALCL, MaskType, ModeType

 add rdx, 16
 endm

UNPACK_V4_16SSE_2 macro CL, TOTALCL, MaskType, ModeType
 movq xmm0, QWORD PTR [rdx]
 movq xmm1, QWORD PTR [rdx+8]

 punpcklwd xmm0, xmm0
 punpcklwd xmm1, xmm1

 UNPACK_RIGHTSHIFT xmm0, 16
 UNPACK_RIGHTSHIFT xmm1, 16

 UNPACK2_SSE CL, TOTALCL, MaskType, ModeType

 add rdx, 16
 endm

UNPACK_V4_16SSE_1A macro CL, TOTALCL, MaskType, ModeType
 punpcklwd xmm0, [rdx]
 UNPACK_RIGHTSHIFT xmm0, 16

 UNPACK1_SSE CL, TOTALCL, MaskType, ModeType

 add rdx, 8
 endm

UNPACK_V4_16SSE_1 macro CL, TOTALCL, MaskType, ModeType
 movq xmm0, QWORD PTR [rdx]
 punpcklwd xmm0, xmm0
 UNPACK_RIGHTSHIFT xmm0, 16

 UNPACK1_SSE CL, TOTALCL, MaskType, ModeType

 add rdx, 8
 endm


UNPACK_V4_8SSE_4A macro CL, TOTALCL, MaskType, ModeType
 punpcklbw xmm0, [rdx]
 punpckhbw xmm2, [rdx]

 punpckhwd xmm1, xmm0
 punpckhwd xmm7, xmm2
 punpcklwd xmm0, xmm0
 punpcklwd xmm2, xmm2

 UNPACK_RIGHTSHIFT xmm1, 24
 UNPACK_RIGHTSHIFT xmm7, 24
 UNPACK_RIGHTSHIFT xmm0, 24
 UNPACK_RIGHTSHIFT xmm2, 24

 UNPACK4_SSE CL, TOTALCL, MaskType, ModeType

 add rdx, 16
 endm

UNPACK_V4_8SSE_4 macro CL, TOTALCL, MaskType, ModeType
 movdqu xmm0, [rdx]

 punpckhbw xmm2, xmm0
 punpcklbw xmm0, xmm0

 punpckhwd xmm7, xmm2
 punpckhwd xmm1, xmm0
 punpcklwd xmm2, xmm2
 punpcklwd xmm0, xmm0

 UNPACK_RIGHTSHIFT xmm7, 24
 UNPACK_RIGHTSHIFT xmm2, 24

 UNPACK_RIGHTSHIFT xmm0, 24
 UNPACK_RIGHTSHIFT xmm1, 24

 UNPACK4_SSE CL, TOTALCL, MaskType, ModeType

 add rdx, 16
 endm

UNPACK_V4_8SSE_3A macro CL, TOTALCL, MaskType, ModeType
 punpcklbw xmm0, [rdx]
 punpckhbw xmm2, [rdx]

 punpckhwd xmm1, xmm0
 punpcklwd xmm0, xmm0
 punpcklwd xmm2, xmm2

 UNPACK_RIGHTSHIFT xmm1, 24
 UNPACK_RIGHTSHIFT xmm0, 24
 UNPACK_RIGHTSHIFT xmm2, 24

 UNPACK3_SSE CL, TOTALCL, MaskType, ModeType

 add rdx, 12
 endm

UNPACK_V4_8SSE_3 macro CL, TOTALCL, MaskType, ModeType
 movq xmm0, QWORD PTR [rdx]
 movd xmm2, dword ptr [rdx+8]

 punpcklbw xmm0, xmm0
 punpcklbw xmm2, xmm2

 punpckhwd xmm1, xmm0
 punpcklwd xmm2, xmm2
 punpcklwd xmm0, xmm0

 UNPACK_RIGHTSHIFT xmm1, 24
 UNPACK_RIGHTSHIFT xmm0, 24
 UNPACK_RIGHTSHIFT xmm2, 24

 UNPACK3_SSE CL, TOTALCL, MaskType, ModeType

 add rdx, 12
 endm

UNPACK_V4_8SSE_2A macro CL, TOTALCL, MaskType, ModeType
 punpcklbw xmm0, [rdx]

 punpckhwd xmm1, xmm0
 punpcklwd xmm0, xmm0

 UNPACK_RIGHTSHIFT xmm1, 24
 UNPACK_RIGHTSHIFT xmm0, 24

 UNPACK2_SSE CL, TOTALCL, MaskType, ModeType

 add rdx, 8
 endm

UNPACK_V4_8SSE_2 macro CL, TOTALCL, MaskType, ModeType
 movq xmm0, QWORD PTR [rdx]

 punpcklbw xmm0, xmm0

 punpckhwd xmm1, xmm0
 punpcklwd xmm0, xmm0

 UNPACK_RIGHTSHIFT xmm1, 24
 UNPACK_RIGHTSHIFT xmm0, 24

 UNPACK2_SSE CL, TOTALCL, MaskType, ModeType

 add rdx, 8
 endm

UNPACK_V4_8SSE_1A macro CL, TOTALCL, MaskType, ModeType
 punpcklbw xmm0, [rdx]
 punpcklwd xmm0, xmm0
 UNPACK_RIGHTSHIFT xmm0, 24

 UNPACK1_SSE CL, TOTALCL, MaskType, ModeType

 add rdx, 4
 endm

UNPACK_V4_8SSE_1 macro CL, TOTALCL, MaskType, ModeType
 movd xmm0, dword ptr [rdx]
 punpcklbw xmm0, xmm0
 punpcklwd xmm0, xmm0
 UNPACK_RIGHTSHIFT xmm0, 24

 UNPACK1_SSE CL, TOTALCL, MaskType, ModeType

 add rdx, 4
 endm


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
 mov eax, dword ptr [rdx]
 DECOMPRESS_RGBA 0

 shr eax, 16
 DECOMPRESS_RGBA 4

 mov eax, dword ptr [rdx+4]
 DECOMPRESS_RGBA 8

 shr eax, 16
 DECOMPRESS_RGBA 12


 movdqa xmm0, XMMWORD PTR [s_TempDecompress]

 punpckhbw xmm2, xmm0
 punpcklbw xmm0, xmm0

 punpckhwd xmm7, xmm2
 punpckhwd xmm1, xmm0
 punpcklwd xmm0, xmm0
 punpcklwd xmm2, xmm2

 psrld xmm0, 24
 psrld xmm1, 24
 psrld xmm2, 24
 psrld xmm7, 24

 UNPACK4_SSE CL, TOTALCL, MaskType, ModeType

 add rdx, 8
 endm

UNPACK_V4_5SSE_4A macro CL, TOTALCL, MaskType, ModeType
 UNPACK_V4_5SSE_4 CL, TOTALCL, MaskType, ModeType
 endm

UNPACK_V4_5SSE_3 macro CL, TOTALCL, MaskType, ModeType
 mov eax, dword ptr [rdx]
 DECOMPRESS_RGBA 0

 shr eax, 16
 DECOMPRESS_RGBA 4

 mov eax, dword ptr [rdx]
 DECOMPRESS_RGBA 8


 movdqa xmm0, XMMWORD PTR [s_TempDecompress]

 punpckhbw xmm2, xmm0
 punpcklbw xmm0, xmm0

 punpckhwd xmm1, xmm0
 punpcklwd xmm0, xmm0
 punpcklwd xmm2, xmm2

 psrld xmm0, 24
 psrld xmm1, 24
 psrld xmm2, 24

 UNPACK3_SSE CL, TOTALCL, MaskType, ModeType

 add rdx, 6
 endm

UNPACK_V4_5SSE_3A macro CL, TOTALCL, MaskType, ModeType
 UNPACK_V4_5SSE_3 CL, TOTALCL, MaskType, ModeType
 endm

UNPACK_V4_5SSE_2 macro CL, TOTALCL, MaskType, ModeType
 mov eax, dword ptr [rdx]
 DECOMPRESS_RGBA 0

 shr eax, 16
 DECOMPRESS_RGBA 4

 movq xmm0, QWORD PTR [s_TempDecompress]

 punpcklbw xmm0, xmm0

 punpckhwd xmm1, xmm0
 punpcklwd xmm0, xmm0

 psrld xmm0, 24
 psrld xmm1, 24

 UNPACK2_SSE CL, TOTALCL, MaskType, ModeType

 add rdx, 4
 endm

UNPACK_V4_5SSE_2A macro CL, TOTALCL, MaskType, ModeType
 UNPACK_V4_5SSE_2 CL, TOTALCL, MaskType, ModeType
 endm

UNPACK_V4_5SSE_1 macro CL, TOTALCL, MaskType, ModeType
 mov ax, word ptr [rdx]
 DECOMPRESS_RGBA 0

 movd xmm0, DWORD PTR [s_TempDecompress]
 punpcklbw xmm0, xmm0
 punpcklwd xmm0, xmm0

 psrld xmm0, 24

 UNPACK1_SSE CL, TOTALCL, MaskType, ModeType

 add rdx, 2
 endm

UNPACK_V4_5SSE_1A macro CL, TOTALCL, MaskType, ModeType
 UNPACK_V4_5SSE_1 CL, TOTALCL, MaskType, ModeType
 endm


SAVE_ROW_REG_BASE macro
 mov rax, [_vifRow]
 movdqa [rax], xmm6
 mov rax, [_vifRegs]
 movss dword ptr [rax+0100h], xmm6
 psrldq xmm6, 4
 movss dword ptr [rax+0110h], xmm6
 psrldq xmm6, 4
 movss dword ptr [rax+0120h], xmm6
 psrldq xmm6, 4
 movss dword ptr [rax+0130h], xmm6
 endm

SAVE_NO_REG macro
 endm



INIT_ARGS macro
 mov rax, qword ptr [_vifRow]
 mov r9, qword ptr [_vifCol]
 movaps xmm6, XMMWORD PTR [rax]
 movaps xmm7, XMMWORD PTR [r9]
 endm

INC_STACK macro reg
 add rsp, 8
 endm



defUNPACK_SkippingWrite macro name, MaskType, ModeType, qsize, sign, SAVE_ROW_REG
@CatStr(UNPACK_SkippingWrite_, name, _, sign, _, MaskType, _, ModeType) proc public

 push rdi
 INIT_ARGS
 mov rax, [_vifRegs]
 movzx rdi, byte ptr [rax + 040h]
 movzx r9, byte ptr [rax + 041h]
 sub rdi, r9
 shl rdi, 4

 cmp r9d, 1
 je @CatStr(name, _, sign, _, MaskType, _, ModeType, _WL1)
 cmp r9d, 2
 je @CatStr(name, _, sign, _, MaskType, _, ModeType, _WL2)
 cmp r9d, 3
 je @CatStr(name, _, sign, _, MaskType, _, ModeType, _WL3)
 jmp @CatStr(name, _, sign, _, MaskType, _, ModeType, _WL4)

@CatStr(name, _, sign, _, MaskType, _, ModeType, _WL1):
 @CatStr(UNPACK_Start_Setup_, MaskType, _SSE_, ModeType) 0

 cmp r8d, qsize
 jl @CatStr(name, _, sign, _, MaskType, _, ModeType, _C1_Done3)

 add rdi, 16


@CatStr(name, _, sign, _, MaskType, _, ModeType, _C1_Align16):

 test rdx, 15
 jz @CatStr(name, _, sign, _, MaskType, _, ModeType, _C1_UnpackAligned)

 @CatStr(UNPACK_, name, SSE_1) 0, 1, MaskType, ModeType

 cmp r8d, (2*qsize)
 jl @CatStr(name, _, sign, _, MaskType, _, ModeType, _C1_DoneWithDec)
 sub r8d, qsize
 jmp @CatStr(name, _, sign, _, MaskType, _, ModeType, _C1_Align16)

@CatStr(name, _, sign, _, MaskType, _, ModeType, _C1_UnpackAligned):

 cmp r8d, (2*qsize)
 jl @CatStr(name, _, sign, _, MaskType, _, ModeType, _C1_Unpack1)
 cmp r8d, (3*qsize)
 jl @CatStr(name, _, sign, _, MaskType, _, ModeType, _C1_Unpack2)
 cmp r8d, (4*qsize)
 jl @CatStr(name, _, sign, _, MaskType, _, ModeType, _C1_Unpack3)
 prefetchnta [rdx + 64]

@CatStr(name, _, sign, _, MaskType, _, ModeType, _C1_Unpack4):
 @CatStr(UNPACK_, name, SSE_4A) 0, 1, MaskType, ModeType

 cmp r8d, (8*qsize)
 jl @CatStr(name, _, sign, _, MaskType, _, ModeType, _C1_DoneUnpack4)
 sub r8d, (4*qsize)
 jmp @CatStr(name, _, sign, _, MaskType, _, ModeType, _C1_Unpack4)

@CatStr(name, _, sign, _, MaskType, _, ModeType, _C1_DoneUnpack4):

 sub r8d, (4*qsize)
 cmp r8d, qsize
 jl @CatStr(name, _, sign, _, MaskType, _, ModeType, _C1_Done3)
 cmp r8d, (2*qsize)
 jl @CatStr(name, _, sign, _, MaskType, _, ModeType, _C1_Unpack1)
 cmp r8d, (3*qsize)
 jl @CatStr(name, _, sign, _, MaskType, _, ModeType, _C1_Unpack2)


@CatStr(name, _, sign, _, MaskType, _, ModeType, _C1_Unpack3):
 @CatStr(UNPACK_, name, SSE_3A) 0, 1, MaskType, ModeType

 sub r8d, (3*qsize)
 jmp @CatStr(name, _, sign, _, MaskType, _, ModeType, _C1_Done3)

@CatStr(name, _, sign, _, MaskType, _, ModeType, _C1_Unpack2):
 @CatStr(UNPACK_, name, SSE_2A) 0, 1, MaskType, ModeType

 sub r8d, (2*qsize)
 jmp @CatStr(name, _, sign, _, MaskType, _, ModeType, _C1_Done3)

@CatStr(name, _, sign, _, MaskType, _, ModeType, _C1_Unpack1):
 @CatStr(UNPACK_, name, SSE_1A) 0, 1, MaskType, ModeType
@CatStr(name, _, sign, _, MaskType, _, ModeType, _C1_DoneWithDec):
 sub r8d, qsize
@CatStr(name, _, sign, _, MaskType, _, ModeType, _C1_Done3):
 SAVE_ROW_REG
 mov eax, r8d

 pop rdi
 ret

@CatStr(name, _, sign, _, MaskType, _, ModeType, _WL2):
 cmp r8d, (2*qsize)

 jl @CatStr(name, _, sign, _, MaskType, _, ModeType, _C2_Done3)
@CatStr(name, _, sign, _, MaskType, _, ModeType, _C2_Unpack):
 @CatStr(UNPACK_, name, SSE_2) 0, 0, MaskType, ModeType


 add rcx, rdi
 cmp r8d, (4*qsize)
 jl @CatStr(name, _, sign, _, MaskType, _, ModeType, _C2_Done2)
 sub r8d, (2*qsize)

 jmp @CatStr(name, _, sign, _, MaskType, _, ModeType, _C2_Unpack)

@CatStr(name, _, sign, _, MaskType, _, ModeType, _C2_Done2):
 sub r8d, (2*qsize)
@CatStr(name, _, sign, _, MaskType, _, ModeType, _C2_Done3):
 cmp r8d, qsize

 jl @CatStr(name, _, sign, _, MaskType, _, ModeType, _C2_Done4)


 @CatStr(UNPACK_, name, SSE_1) 0, 0, MaskType, ModeType

 sub r8d, qsize
@CatStr(name, _, sign, _, MaskType, _, ModeType, _C2_Done4):

 SAVE_ROW_REG
 mov eax, r8d

 pop rdi
 ret

@CatStr(name, _, sign, _, MaskType, _, ModeType, _WL3):
 cmp r8d, (3*qsize)

 jl @CatStr(name, _, sign, _, MaskType, _, ModeType, _C3_Done5)
@CatStr(name, _, sign, _, MaskType, _, ModeType, _C3_Unpack):
 @CatStr(UNPACK_, name, SSE_3) 0, 0, MaskType, ModeType

 add rcx, rdi
 cmp r8d, (6*qsize)
 jl @CatStr(name, _, sign, _, MaskType, _, ModeType, _C3_Done2)
 sub r8d, (3*qsize)
 jmp @CatStr(name, _, sign, _, MaskType, _, ModeType, _C3_Unpack)
@CatStr(name, _, sign, _, MaskType, _, ModeType, _C3_Done2):
 sub r8d, (3*qsize)
@CatStr(name, _, sign, _, MaskType, _, ModeType, _C3_Done5):
 cmp r8d, qsize
 jl @CatStr(name, _, sign, _, MaskType, _, ModeType, _C3_Done4)

 cmp r8d, (2*qsize)
 jl @CatStr(name, _, sign, _, MaskType, _, ModeType, _C3_Done3)

 @CatStr(UNPACK_, name, SSE_2) 0, 0, MaskType, ModeType

 sub r8d, (2*qsize)
 jmp @CatStr(name, _, sign, _, MaskType, _, ModeType, _C3_Done4)
@CatStr(name, _, sign, _, MaskType, _, ModeType, _C3_Done3):
 sub r8d, qsize
 @CatStr(UNPACK_, name, SSE_1) 0, 0, MaskType, ModeType
@CatStr(name, _, sign, _, MaskType, _, ModeType, _C3_Done4):
 SAVE_ROW_REG
 mov eax, r8d

 pop rdi

 ret

@CatStr(name, _, sign, _, MaskType, _, ModeType, _WL4):
 sub r9, 3
 push rdi
 cmp r8d, qsize
 jl @CatStr(name, _, sign, _, MaskType, _, ModeType, _C4_Done)

@CatStr(name, _, sign, _, MaskType, _, ModeType, _C4_Unpack):
 cmp r8d, (3*qsize)
 jge @CatStr(name, _, sign, _, MaskType, _, ModeType, _C4_Unpack3)
 cmp r8d, (2*qsize)
 jge @CatStr(name, _, sign, _, MaskType, _, ModeType, _C4_Unpack2)

 @CatStr(UNPACK_, name, SSE_1) 0, 0, MaskType, ModeType


 sub r8d, qsize
 jmp @CatStr(name, _, sign, _, MaskType, _, ModeType, _C4_Done)
@CatStr(name, _, sign, _, MaskType, _, ModeType, _C4_Unpack2):
 @CatStr(UNPACK_, name, SSE_2) 0, 0, MaskType, ModeType


 sub r8d, (2*qsize)
 jmp @CatStr(name, _, sign, _, MaskType, _, ModeType, _C4_Done)
@CatStr(name, _, sign, _, MaskType, _, ModeType, _C4_Unpack3):
 @CatStr(UNPACK_, name, SSE_3) 0, 0, MaskType, ModeType


 sub r8d, (3*qsize)
 mov rdi, r9

@CatStr(name, _, sign, _, MaskType, _, ModeType, _C4_UnpackX):


 cmp r8d, qsize
 jl @CatStr(name, _, sign, _, MaskType, _, ModeType, _C4_Done)

 @CatStr(UNPACK_, name, SSE_1) 3, 0, MaskType, ModeType

 sub r8d, qsize
 cmp rdi, 1
 je @CatStr(name, _, sign, _, MaskType, _, ModeType, _C4_DoneLoop)
 sub rdi, 1
 jmp @CatStr(name, _, sign, _, MaskType, _, ModeType, _C4_UnpackX)
@CatStr(name, _, sign, _, MaskType, _, ModeType, _C4_DoneLoop):
 add rcx, [rsp]
 cmp r8d, qsize
 jl @CatStr(name, _, sign, _, MaskType, _, ModeType, _C4_Done)
 jmp @CatStr(name, _, sign, _, MaskType, _, ModeType, _C4_Unpack)
@CatStr(name, _, sign, _, MaskType, _, ModeType, _C4_Done):

 SAVE_ROW_REG
 INC_STACK()
 mov eax, r8d


 pop rdi
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
