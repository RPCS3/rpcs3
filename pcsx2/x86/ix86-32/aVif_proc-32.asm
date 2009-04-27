
.686
.model flat, c
.mmx
.xmm


extern vifRegs:ptr
extern vifMaskRegs:ptr
extern vifRow:ptr
extern s_TempDecompress:ptr


.code

UNPACK_Write0_Regular macro r0, CL, DEST_OFFSET, MOVDQA
	MOVDQA [edi+DEST_OFFSET], r0
	endm

UNPACK_Write1_Regular macro r0, CL, DEST_OFFSET, MOVDQA
	MOVDQA [edi], r0
	add edi, ecx
	endm

UNPACK_Write0_Mask macro r0, CL, DEST_OFFSET, MOVDQA
	UNPACK_Write0_Regular r0, CL, DEST_OFFSET, MOVDQA
	endm

UNPACK_Write1_Mask macro r0, CL, DEST_OFFSET, MOVDQA
	UNPACK_Write1_Regular r0, CL, DEST_OFFSET, MOVDQA
	endm


UNPACK_Write0_WriteMask macro r0, CL, DEST_OFFSET, MOVDQA

	movdqa xmm3, [eax + 64*(CL) + 48]
	pand r0, xmm3
	pandn xmm3, [edi]
	por r0, xmm3
	MOVDQA [edi], r0
	add edi, 16
	endm


UNPACK_Write1_WriteMask macro r0, CL, DEST_OFFSET, MOVDQA

	movdqa xmm3, [eax + 64*(0) + 48]
	pand r0, xmm3
	pandn xmm3, [edi]
	por r0, xmm3
	MOVDQA [edi], r0
	add edi, ecx
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
	mov eax, [vifMaskRegs]
	movdqa xmm4, [eax + 64*(CL) + 16]
	movdqa xmm5, [eax + 64*(CL) + 32]
	movdqa xmm3, [eax + 64*(CL)]
	pand xmm4, xmm6
	pand xmm5, xmm7
	por xmm5, xmm4
	endm

UNPACK_Start_Setup_Mask_SSE_0 macro CL
	UNPACK_Setup_Mask_SSE CL
	endm

UNPACK_Start_Setup_Mask_SSE_1 macro CL
	mov eax, [vifMaskRegs]
	movdqa xmm4, [eax + 64*(CL) + 16]
	movdqa xmm5, [eax + 64*(CL) + 32]
	pand xmm4, xmm6
	pand xmm5, xmm7
	por xmm5, xmm4
	endm

UNPACK_Start_Setup_Mask_SSE_2 macro CL
	endm

UNPACK_Setup_Mask_SSE_0_1 macro CL
	endm
UNPACK_Setup_Mask_SSE_1_1 macro CL
	mov eax, [vifMaskRegs]
	movdqa xmm3, [eax + 64*(0)]
	endm


UNPACK_Setup_Mask_SSE_2_1 macro CL

	mov eax, [vifMaskRegs]
	movdqa xmm4, [eax + 64*(0) + 16]
	movdqa xmm5, [eax + 64*(0) + 32]
	movdqa xmm3, [eax + 64*(0)]
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
	add edi, (16*qw)
	endm
UNPACK_INC_DST_1_Regular macro qw
	endm
UNPACK_INC_DST_0_Mask macro qw
	add edi, (16*qw)
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
	MOVDQA xmm7, [esi]

	pshufd xmm0, xmm7, 0
	pshufd xmm1, xmm7, 055h
	pshufd xmm2, xmm7, 0aah
	pshufd xmm7, xmm7, 0ffh

	UNPACK4_SSE CL, TOTALCL, MaskType, ModeType

	add esi, 16
	endm

UNPACK_S_32SSE_4A macro CL, TOTALCL, MaskType, ModeType
	UNPACK_S_32SSE_4x CL, TOTALCL, MaskType, ModeType, movdqa
	endm
UNPACK_S_32SSE_4 macro CL, TOTALCL, MaskType, ModeType
	UNPACK_S_32SSE_4x CL, TOTALCL, MaskType, ModeType, movdqu
	endm

UNPACK_S_32SSE_3x macro CL, TOTALCL, MaskType, ModeType, MOVDQA
	MOVDQA xmm2, [esi]

	pshufd xmm0, xmm2, 0
	pshufd xmm1, xmm2, 055h
	pshufd xmm2, xmm2, 0aah

	UNPACK3_SSE CL, TOTALCL, MaskType, ModeType

	add esi, 12
	endm

UNPACK_S_32SSE_3A macro CL, TOTALCL, MaskType, ModeType
	UNPACK_S_32SSE_3x CL, TOTALCL, MaskType, ModeType, movdqa
	endm
UNPACK_S_32SSE_3 macro CL, TOTALCL, MaskType, ModeType
	UNPACK_S_32SSE_3x CL, TOTALCL, MaskType, ModeType, movdqu
	endm

UNPACK_S_32SSE_2 macro CL, TOTALCL, MaskType, ModeType
	movq xmm1, QWORD PTR [esi]

	pshufd xmm0, xmm1, 0
	pshufd xmm1, xmm1, 055h

	UNPACK2_SSE CL, TOTALCL, MaskType, ModeType

	add esi, 8
	endm

UNPACK_S_32SSE_2A macro CL, TOTALCL, MaskType, ModeType
	UNPACK_S_32SSE_2 CL, TOTALCL, MaskType, ModeType
	endm

UNPACK_S_32SSE_1 macro CL, TOTALCL, MaskType, ModeType
	movd xmm0, dword ptr [esi]
	pshufd xmm0, xmm0, 0

	UNPACK1_SSE CL, TOTALCL, MaskType, ModeType

	add esi, 4
	endm

UNPACK_S_32SSE_1A macro CL, TOTALCL, MaskType, ModeType
	UNPACK_S_32SSE_1 CL, TOTALCL, MaskType, ModeType
	endm


UNPACK_S_16SSE_4 macro CL, TOTALCL, MaskType, ModeType
	movq xmm7, QWORD PTR [esi]
	punpcklwd xmm7, xmm7
	UNPACK_RIGHTSHIFT xmm7, 16

	pshufd xmm0, xmm7, 0
	pshufd xmm1, xmm7, 055h
	pshufd xmm2, xmm7, 0aah
	pshufd xmm7, xmm7, 0ffh

	UNPACK4_SSE CL, TOTALCL, MaskType, ModeType

	add esi, 8
	endm

UNPACK_S_16SSE_4A macro CL, TOTALCL, MaskType, ModeType
	UNPACK_S_16SSE_4 CL, TOTALCL, MaskType, ModeType
	endm

UNPACK_S_16SSE_3 macro CL, TOTALCL, MaskType, ModeType
	movq xmm2, QWORD PTR [esi]
	punpcklwd xmm2, xmm2
	UNPACK_RIGHTSHIFT xmm2, 16

	pshufd xmm0, xmm2, 0
	pshufd xmm1, xmm2, 055h
	pshufd xmm2, xmm2, 0aah

	UNPACK3_SSE CL, TOTALCL, MaskType, ModeType
	add esi, 6
	endm

UNPACK_S_16SSE_3A macro CL, TOTALCL, MaskType, ModeType
	UNPACK_S_16SSE_3 CL, TOTALCL, MaskType, ModeType
	endm

UNPACK_S_16SSE_2 macro CL, TOTALCL, MaskType, ModeType
	movd xmm1, dword ptr [esi]
	punpcklwd xmm1, xmm1
	UNPACK_RIGHTSHIFT xmm1, 16

	pshufd xmm0, xmm1, 0
	pshufd xmm1, xmm1, 055h

	UNPACK2_SSE CL, TOTALCL, MaskType, ModeType

	add esi, 4
	endm

UNPACK_S_16SSE_2A macro CL, TOTALCL, MaskType, ModeType
	UNPACK_S_16SSE_2 CL, TOTALCL, MaskType, ModeType
	endm

UNPACK_S_16SSE_1 macro CL, TOTALCL, MaskType, ModeType
	movd xmm0, dword ptr [esi]
	punpcklwd xmm0, xmm0
	UNPACK_RIGHTSHIFT xmm0, 16
	pshufd xmm0, xmm0, 0

	UNPACK1_SSE CL, TOTALCL, MaskType, ModeType

	add esi, 2
	endm

UNPACK_S_16SSE_1A macro CL, TOTALCL, MaskType, ModeType
	UNPACK_S_16SSE_1 CL, TOTALCL, MaskType, ModeType
	endm


UNPACK_S_8SSE_4 macro CL, TOTALCL, MaskType, ModeType
	movd xmm7, dword ptr [esi]
	punpcklbw xmm7, xmm7
	punpcklwd xmm7, xmm7
	UNPACK_RIGHTSHIFT xmm7, 24

	pshufd xmm0, xmm7, 0
	pshufd xmm1, xmm7, 055h
	pshufd xmm2, xmm7, 0aah
	pshufd xmm7, xmm7, 0ffh

	UNPACK4_SSE CL, TOTALCL, MaskType, ModeType

	add esi, 4
	endm

UNPACK_S_8SSE_4A macro CL, TOTALCL, MaskType, ModeType
	UNPACK_S_8SSE_4 CL, TOTALCL, MaskType, ModeType
	endm

UNPACK_S_8SSE_3 macro CL, TOTALCL, MaskType, ModeType
	movd xmm2, dword ptr [esi]
	punpcklbw xmm2, xmm2
	punpcklwd xmm2, xmm2
	UNPACK_RIGHTSHIFT xmm2, 24

	pshufd xmm0, xmm2, 0
	pshufd xmm1, xmm2, 055h
	pshufd xmm2, xmm2, 0aah

	UNPACK3_SSE CL, TOTALCL, MaskType, ModeType

	add esi, 3
	endm

UNPACK_S_8SSE_3A macro CL, TOTALCL, MaskType, ModeType
	UNPACK_S_8SSE_3 CL, TOTALCL, MaskType, ModeType
	endm

UNPACK_S_8SSE_2 macro CL, TOTALCL, MaskType, ModeType
	movd xmm1, dword ptr [esi]
	punpcklbw xmm1, xmm1
	punpcklwd xmm1, xmm1
	UNPACK_RIGHTSHIFT xmm1, 24

	pshufd xmm0, xmm1, 0
	pshufd xmm1, xmm1, 055h

	UNPACK2_SSE CL, TOTALCL, MaskType, ModeType

	add esi, 2
	endm

UNPACK_S_8SSE_2A macro CL, TOTALCL, MaskType, ModeType
	UNPACK_S_8SSE_2 CL, TOTALCL, MaskType, ModeType
	endm

UNPACK_S_8SSE_1 macro CL, TOTALCL, MaskType, ModeType
	movd xmm0, dword ptr [esi]
	punpcklbw xmm0, xmm0
	punpcklwd xmm0, xmm0
	UNPACK_RIGHTSHIFT xmm0, 24
	pshufd xmm0, xmm0, 0

	UNPACK1_SSE CL, TOTALCL, MaskType, ModeType

	inc esi
	endm

UNPACK_S_8SSE_1A macro CL, TOTALCL, MaskType, ModeType
	UNPACK_S_8SSE_1 CL, TOTALCL, MaskType, ModeType
	endm


UNPACK_V2_32SSE_4A macro CL, TOTALCL, MaskType, ModeType
	MOVDQA xmm0, [esi]
	MOVDQA xmm2, [esi+16]

	pshufd xmm1, xmm0, 0eeh
	pshufd xmm7, xmm2, 0eeh

	UNPACK4_SSE CL, TOTALCL, MaskType, ModeType

	add esi, 32
	endm

UNPACK_V2_32SSE_4 macro CL, TOTALCL, MaskType, ModeType
	movq xmm0, QWORD PTR [esi]
	movq xmm1, QWORD PTR [esi+8]
	movq xmm2, QWORD PTR [esi+16]
	movq xmm7, QWORD PTR [esi+24]

	UNPACK4_SSE CL, TOTALCL, MaskType, ModeType

	add esi, 32
	endm

UNPACK_V2_32SSE_3A macro CL, TOTALCL, MaskType, ModeType
	MOVDQA xmm0, [esi]
	movq xmm2, QWORD PTR [esi+16]
	pshufd xmm1, xmm0, 0eeh

	UNPACK3_SSE CL, TOTALCL, MaskType, ModeType

	add esi, 24
	endm

UNPACK_V2_32SSE_3 macro CL, TOTALCL, MaskType, ModeType
	movq xmm0, QWORD PTR [esi]
	movq xmm1, QWORD PTR [esi+8]
	movq xmm2, QWORD PTR [esi+16]

	UNPACK3_SSE CL, TOTALCL, MaskType, ModeType

	add esi, 24
	endm

UNPACK_V2_32SSE_2 macro CL, TOTALCL, MaskType, ModeType
	movq xmm0, QWORD PTR [esi]
	movq xmm1, QWORD PTR [esi+8]

	UNPACK2_SSE CL, TOTALCL, MaskType, ModeType

	add esi, 16
	endm

UNPACK_V2_32SSE_2A macro CL, TOTALCL, MaskType, ModeType
	UNPACK_V2_32SSE_2 CL, TOTALCL, MaskType, ModeType
	endm

UNPACK_V2_32SSE_1 macro CL, TOTALCL, MaskType, ModeType
	movq xmm0, QWORD PTR [esi]

	UNPACK1_SSE CL, TOTALCL, MaskType, ModeType

	add esi, 8
	endm

UNPACK_V2_32SSE_1A macro CL, TOTALCL, MaskType, ModeType
	UNPACK_V2_32SSE_1 CL, TOTALCL, MaskType, ModeType
	endm


UNPACK_V2_16SSE_4A macro CL, TOTALCL, MaskType, ModeType
	punpcklwd xmm0, [esi]
	punpckhwd xmm2, [esi]

	UNPACK_RIGHTSHIFT xmm0, 16
	UNPACK_RIGHTSHIFT xmm2, 16

	punpckhqdq xmm1, xmm0
 punpckhqdq xmm7, xmm2

 punpcklqdq xmm0, xmm0
 punpcklqdq xmm2, xmm2
 punpckhqdq xmm1, xmm1
 punpckhqdq xmm7, xmm7
	
	UNPACK4_SSE CL, TOTALCL, MaskType, ModeType
	add esi, 16
	endm

UNPACK_V2_16SSE_4 macro CL, TOTALCL, MaskType, ModeType
	pxor xmm0, xmm0
	pxor xmm2, xmm2
	movdqu xmm0, [esi]

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

	add esi, 16
	endm

UNPACK_V2_16SSE_3A macro CL, TOTALCL, MaskType, ModeType
	punpcklwd xmm0, [esi]
	punpckhwd xmm2, [esi]

	UNPACK_RIGHTSHIFT xmm0, 16
	UNPACK_RIGHTSHIFT xmm2, 16


	punpckhqdq xmm1, xmm0

 punpcklqdq xmm0, xmm0
 punpcklqdq xmm2, xmm2
 punpckhqdq xmm1, xmm1

	UNPACK3_SSE CL, TOTALCL, MaskType, ModeType

	add esi, 12
	endm

UNPACK_V2_16SSE_3 macro CL, TOTALCL, MaskType, ModeType
	movdqu xmm0, [esi]

	punpckhwd xmm2, xmm0
	punpcklwd xmm0, xmm0

	UNPACK_RIGHTSHIFT xmm0, 16
	UNPACK_RIGHTSHIFT xmm2, 16


	punpckhqdq xmm1, xmm0

 punpcklqdq xmm0, xmm0
 punpcklqdq xmm2, xmm2
 punpckhqdq xmm1, xmm1

	UNPACK3_SSE CL, TOTALCL, MaskType, ModeType

	add esi, 12
	endm

UNPACK_V2_16SSE_2A macro CL, TOTALCL, MaskType, ModeType
	punpcklwd xmm0, [esi]
	UNPACK_RIGHTSHIFT xmm0, 16


	punpckhqdq xmm1, xmm0

 punpcklqdq xmm0, xmm0
 punpckhqdq xmm1, xmm1

	UNPACK2_SSE CL, TOTALCL, MaskType, ModeType

	add esi, 8
	endm

UNPACK_V2_16SSE_2 macro CL, TOTALCL, MaskType, ModeType
	movq xmm0, QWORD PTR [esi]
	punpcklwd xmm0, xmm0
	UNPACK_RIGHTSHIFT xmm0, 16


	punpckhqdq xmm1, xmm0

 punpcklqdq xmm0, xmm0
 punpckhqdq xmm1, xmm1

	UNPACK2_SSE CL, TOTALCL, MaskType, ModeType

	add esi, 8
	endm

UNPACK_V2_16SSE_1A macro CL, TOTALCL, MaskType, ModeType
	punpcklwd xmm0, [esi]
	UNPACK_RIGHTSHIFT xmm0, 16

	punpcklqdq xmm0, xmm0
	
	UNPACK1_SSE CL, TOTALCL, MaskType, ModeType

	add esi, 4
	endm

UNPACK_V2_16SSE_1 macro CL, TOTALCL, MaskType, ModeType
	movd xmm0, dword ptr [esi]
	punpcklwd xmm0, xmm0
	UNPACK_RIGHTSHIFT xmm0, 16

	punpcklqdq xmm0, xmm0
	
	UNPACK1_SSE CL, TOTALCL, MaskType, ModeType

	add esi, 4
	endm


UNPACK_V2_8SSE_4 macro CL, TOTALCL, MaskType, ModeType
	movq xmm0, QWORD PTR [esi]

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

	add esi, 8
	endm

UNPACK_V2_8SSE_4A macro CL, TOTALCL, MaskType, ModeType
	UNPACK_V2_8SSE_4 CL, TOTALCL, MaskType, ModeType
	endm

UNPACK_V2_8SSE_3 macro CL, TOTALCL, MaskType, ModeType
	movq xmm0, QWORD PTR [esi]

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

	add esi, 6
	endm

UNPACK_V2_8SSE_3A macro CL, TOTALCL, MaskType, ModeType
	UNPACK_V2_8SSE_3 CL, TOTALCL, MaskType, ModeType
	endm

UNPACK_V2_8SSE_2 macro CL, TOTALCL, MaskType, ModeType
	movd xmm0, dword ptr [esi]
	punpcklbw xmm0, xmm0
	punpcklwd xmm0, xmm0
	UNPACK_RIGHTSHIFT xmm0, 24

	punpckhqdq xmm1, xmm0

 punpcklqdq xmm0, xmm0
 punpckhqdq xmm1, xmm1
 
	UNPACK2_SSE CL, TOTALCL, MaskType, ModeType

	add esi, 4
	endm

UNPACK_V2_8SSE_2A macro CL, TOTALCL, MaskType, ModeType
	UNPACK_V2_8SSE_2 CL, TOTALCL, MaskType, ModeType
	endm

UNPACK_V2_8SSE_1 macro CL, TOTALCL, MaskType, ModeType
	movd xmm0, dword ptr [esi]
	punpcklbw xmm0, xmm0
	punpcklwd xmm0, xmm0
	UNPACK_RIGHTSHIFT xmm0, 24
	punpcklqdq xmm0, xmm0

	UNPACK1_SSE CL, TOTALCL, MaskType, ModeType

	add esi, 2
	endm

UNPACK_V2_8SSE_1A macro CL, TOTALCL, MaskType, ModeType
	UNPACK_V2_8SSE_1 CL, TOTALCL, MaskType, ModeType
	endm


UNPACK_V3_32SSE_4x macro CL, TOTALCL, MaskType, ModeType, MOVDQA
	MOVDQA xmm0, [esi]
	movdqu xmm1, [esi+12]

	@CatStr(UNPACK_Setup_, MaskType, _SSE_, ModeType, _, TOTALCL) CL+0
	@CatStr(UNPACK_, MaskType, _SSE_, ModeType) xmm0
	@CatStr(UNPACK_Write, TOTALCL, _, MaskType) xmm0, CL, 0, movdqa

	@CatStr(UNPACK_Setup_, MaskType, _SSE_, ModeType, _, TOTALCL) CL+1
	@CatStr(UNPACK_, MaskType, _SSE_, ModeType) xmm1
	@CatStr(UNPACK_Write, TOTALCL, _, MaskType) xmm1, CL+1, 16, movdqa


    MOVDQA xmm7, [esi+32]
	movdqu xmm2, [esi+24]
	psrldq xmm7, 4

	@CatStr(UNPACK_Setup_, MaskType, _SSE_, ModeType, _, TOTALCL) CL+2
	@CatStr(UNPACK_, MaskType, _SSE_, ModeType) xmm2
	@CatStr(UNPACK_Write, TOTALCL, _, MaskType) xmm2, CL+2, 32, movdqa

	@CatStr(UNPACK_Setup_, MaskType, _SSE_, ModeType, _, TOTALCL) CL+3
	@CatStr(UNPACK_, MaskType, _SSE_, ModeType) xmm7
	@CatStr(UNPACK_Write, TOTALCL, _, MaskType) xmm7, CL+3, 48, movdqa

	@CatStr(UNPACK_INC_DST_, TOTALCL, _, MaskType) 4

	add esi, 48
	endm

UNPACK_V3_32SSE_4A macro CL, TOTALCL, MaskType, ModeType
	UNPACK_V3_32SSE_4x CL, TOTALCL, MaskType, ModeType, movdqa
	endm
UNPACK_V3_32SSE_4 macro CL, TOTALCL, MaskType, ModeType
	UNPACK_V3_32SSE_4x CL, TOTALCL, MaskType, ModeType, movdqu
	endm

UNPACK_V3_32SSE_3x macro CL, TOTALCL, MaskType, ModeType, MOVDQA
	MOVDQA xmm0, [esi]
	movdqu xmm1, [esi+12]

	@CatStr(UNPACK_Setup_, MaskType, _SSE_, ModeType, _, TOTALCL) CL
	@CatStr(UNPACK_, MaskType, _SSE_, ModeType) xmm0
	@CatStr(UNPACK_Write, TOTALCL, _, MaskType) xmm0, CL, 0, movdqa

	@CatStr(UNPACK_Setup_, MaskType, _SSE_, ModeType, _, TOTALCL) CL+1
	@CatStr(UNPACK_, MaskType, _SSE_, ModeType) xmm1
	@CatStr(UNPACK_Write, TOTALCL, _, MaskType) xmm1, CL+1, 16, movdqa

	movdqu xmm2, [esi+24]

	@CatStr(UNPACK_Setup_, MaskType, _SSE_, ModeType, _, TOTALCL) CL+2
	@CatStr(UNPACK_, MaskType, _SSE_, ModeType) xmm2
	@CatStr(UNPACK_Write, TOTALCL, _, MaskType) xmm2, CL+2, 32, movdqa

	@CatStr(UNPACK_INC_DST_, TOTALCL, _, MaskType) 3

	add esi, 36
	endm

UNPACK_V3_32SSE_3A macro CL, TOTALCL, MaskType, ModeType
	UNPACK_V3_32SSE_3x CL, TOTALCL, MaskType, ModeType, movdqa
	endm
UNPACK_V3_32SSE_3 macro CL, TOTALCL, MaskType, ModeType
	UNPACK_V3_32SSE_3x CL, TOTALCL, MaskType, ModeType, movdqu
	endm

UNPACK_V3_32SSE_2x macro CL, TOTALCL, MaskType, ModeType, MOVDQA
	MOVDQA xmm0, [esi]
	movdqu xmm1, [esi+12]

	UNPACK2_SSE CL, TOTALCL, MaskType, ModeType

	add esi, 24
	endm

UNPACK_V3_32SSE_2A macro CL, TOTALCL, MaskType, ModeType
	UNPACK_V3_32SSE_2x CL, TOTALCL, MaskType, ModeType, movdqa
	endm
UNPACK_V3_32SSE_2 macro CL, TOTALCL, MaskType, ModeType
	UNPACK_V3_32SSE_2x CL, TOTALCL, MaskType, ModeType, movdqu
	endm

UNPACK_V3_32SSE_1x macro CL, TOTALCL, MaskType, ModeType, MOVDQA
	MOVDQA xmm0, [esi]

	UNPACK1_SSE CL, TOTALCL, MaskType, ModeType

	add esi, 12
	endm

UNPACK_V3_32SSE_1A macro CL, TOTALCL, MaskType, ModeType
	UNPACK_V3_32SSE_1x CL, TOTALCL, MaskType, ModeType, movdqa
	endm
UNPACK_V3_32SSE_1 macro CL, TOTALCL, MaskType, ModeType
	UNPACK_V3_32SSE_1x CL, TOTALCL, MaskType, ModeType, movdqu
	endm


UNPACK_V3_16SSE_4 macro CL, TOTALCL, MaskType, ModeType
	movq xmm0, QWORD PTR [esi]
	movq xmm1, QWORD PTR [esi+6]

	punpcklwd xmm0, xmm0
	movq xmm2, QWORD PTR [esi+12]
	punpcklwd xmm1, xmm1
	UNPACK_RIGHTSHIFT xmm0, 16
	movq xmm7, QWORD PTR [esi+18]
	UNPACK_RIGHTSHIFT xmm1, 16
	punpcklwd xmm2, xmm2
	punpcklwd xmm7, xmm7

	UNPACK_RIGHTSHIFT xmm2, 16
	UNPACK_RIGHTSHIFT xmm7, 16

	UNPACK4_SSE CL, TOTALCL, MaskType, ModeType

	add esi, 24
	endm

UNPACK_V3_16SSE_4A macro CL, TOTALCL, MaskType, ModeType
	UNPACK_V3_16SSE_4 CL, TOTALCL, MaskType, ModeType
	endm

UNPACK_V3_16SSE_3 macro CL, TOTALCL, MaskType, ModeType
	movq xmm0, QWORD PTR [esi]
	movq xmm1, QWORD PTR [esi+6]

	punpcklwd xmm0, xmm0
	movq xmm2, QWORD PTR [esi+12]
	punpcklwd xmm1, xmm1
	UNPACK_RIGHTSHIFT xmm0, 16
	punpcklwd xmm2, xmm2

	UNPACK_RIGHTSHIFT xmm1, 16
	UNPACK_RIGHTSHIFT xmm2, 16

	UNPACK3_SSE CL, TOTALCL, MaskType, ModeType

	add esi, 18
	endm

UNPACK_V3_16SSE_3A macro CL, TOTALCL, MaskType, ModeType
	UNPACK_V3_16SSE_3 CL, TOTALCL, MaskType, ModeType
	endm

UNPACK_V3_16SSE_2 macro CL, TOTALCL, MaskType, ModeType
	movq xmm0, QWORD PTR [esi]
	movq xmm1, QWORD PTR [esi+6]

	punpcklwd xmm0, xmm0
	punpcklwd xmm1, xmm1

	UNPACK_RIGHTSHIFT xmm0, 16
	UNPACK_RIGHTSHIFT xmm1, 16

	UNPACK2_SSE CL, TOTALCL, MaskType, ModeType

	add esi, 12
	endm

UNPACK_V3_16SSE_2A macro CL, TOTALCL, MaskType, ModeType
	UNPACK_V3_16SSE_2 CL, TOTALCL, MaskType, ModeType
	endm

UNPACK_V3_16SSE_1 macro CL, TOTALCL, MaskType, ModeType
	movq xmm0, QWORD PTR [esi]
	punpcklwd xmm0, xmm0
	UNPACK_RIGHTSHIFT xmm0, 16

	UNPACK1_SSE CL, TOTALCL, MaskType, ModeType

	add esi, 6
	endm

UNPACK_V3_16SSE_1A macro CL, TOTALCL, MaskType, ModeType
	UNPACK_V3_16SSE_1 CL, TOTALCL, MaskType, ModeType
	endm


UNPACK_V3_8SSE_4 macro CL, TOTALCL, MaskType, ModeType
	movq xmm1, QWORD PTR [esi]
	movq xmm7, QWORD PTR [esi+6]

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

	add esi, 12
	endm

UNPACK_V3_8SSE_4A macro CL, TOTALCL, MaskType, ModeType
	UNPACK_V3_8SSE_4 CL, TOTALCL, MaskType, ModeType
	endm

UNPACK_V3_8SSE_3 macro CL, TOTALCL, MaskType, ModeType
	movd xmm0, dword ptr [esi]
	movd xmm1, dword ptr [esi+3]

	punpcklbw xmm0, xmm0
	movd xmm2, dword ptr [esi+6]
	punpcklbw xmm1, xmm1
	punpcklwd xmm0, xmm0
	punpcklbw xmm2, xmm2

	punpcklwd xmm1, xmm1
	punpcklwd xmm2, xmm2

	UNPACK_RIGHTSHIFT xmm0, 24
	UNPACK_RIGHTSHIFT xmm1, 24
	UNPACK_RIGHTSHIFT xmm2, 24

	UNPACK3_SSE CL, TOTALCL, MaskType, ModeType

	add esi, 9
	endm

UNPACK_V3_8SSE_3A macro CL, TOTALCL, MaskType, ModeType
	UNPACK_V3_8SSE_3 CL, TOTALCL, MaskType, ModeType
	endm

UNPACK_V3_8SSE_2 macro CL, TOTALCL, MaskType, ModeType
	movd xmm0, dword ptr [esi]
	movd xmm1, dword ptr [esi+3]

	punpcklbw xmm0, xmm0
	punpcklbw xmm1, xmm1

	punpcklwd xmm0, xmm0
	punpcklwd xmm1, xmm1

	UNPACK_RIGHTSHIFT xmm0, 24
	UNPACK_RIGHTSHIFT xmm1, 24

	UNPACK2_SSE CL, TOTALCL, MaskType, ModeType

	add esi, 6
	endm

UNPACK_V3_8SSE_2A macro CL, TOTALCL, MaskType, ModeType
	UNPACK_V3_8SSE_2 CL, TOTALCL, MaskType, ModeType
	endm

UNPACK_V3_8SSE_1 macro CL, TOTALCL, MaskType, ModeType
	movd xmm0, dword ptr [esi]
	punpcklbw xmm0, xmm0
	punpcklwd xmm0, xmm0
	UNPACK_RIGHTSHIFT xmm0, 24

	UNPACK1_SSE CL, TOTALCL, MaskType, ModeType

	add esi, 3
	endm

UNPACK_V3_8SSE_1A macro CL, TOTALCL, MaskType, ModeType
	UNPACK_V3_8SSE_1 CL, TOTALCL, MaskType, ModeType
	endm


UNPACK_V4_32SSE_4A macro CL, TOTALCL, MaskType, ModeType
	movdqa xmm0, [esi]
	movdqa xmm1, [esi+16]
	movdqa xmm2, [esi+32]
	movdqa xmm7, [esi+48]

	UNPACK4_SSE CL, TOTALCL, MaskType, ModeType

	add esi, 64
	endm

UNPACK_V4_32SSE_4 macro CL, TOTALCL, MaskType, ModeType
	movdqu xmm0, [esi]
	movdqu xmm1, [esi+16]
	movdqu xmm2, [esi+32]
	movdqu xmm7, [esi+48]

	UNPACK4_SSE CL, TOTALCL, MaskType, ModeType

	add esi, 64
	endm

UNPACK_V4_32SSE_3A macro CL, TOTALCL, MaskType, ModeType
	movdqa xmm0, [esi]
	movdqa xmm1, [esi+16]
	movdqa xmm2, [esi+32]

	UNPACK3_SSE CL, TOTALCL, MaskType, ModeType

	add esi, 48
	endm

UNPACK_V4_32SSE_3 macro CL, TOTALCL, MaskType, ModeType
	movdqu xmm0, [esi]
	movdqu xmm1, [esi+16]
	movdqu xmm2, [esi+32]

	UNPACK3_SSE CL, TOTALCL, MaskType, ModeType

	add esi, 48
	endm

UNPACK_V4_32SSE_2A macro CL, TOTALCL, MaskType, ModeType
	movdqa xmm0, [esi]
	movdqa xmm1, [esi+16]

	UNPACK2_SSE CL, TOTALCL, MaskType, ModeType

	add esi, 32
	endm

UNPACK_V4_32SSE_2 macro CL, TOTALCL, MaskType, ModeType
	movdqu xmm0, [esi]
	movdqu xmm1, [esi+16]

	UNPACK2_SSE CL, TOTALCL, MaskType, ModeType

	add esi, 32
	endm

UNPACK_V4_32SSE_1A macro CL, TOTALCL, MaskType, ModeType
	movdqa xmm0, [esi]

	UNPACK1_SSE CL, TOTALCL, MaskType, ModeType

	add esi, 16
	endm

UNPACK_V4_32SSE_1 macro CL, TOTALCL, MaskType, ModeType
	movdqu xmm0, [esi]

	UNPACK1_SSE CL, TOTALCL, MaskType, ModeType

	add esi, 16
	endm


UNPACK_V4_16SSE_4A macro CL, TOTALCL, MaskType, ModeType

	punpcklwd xmm0, [esi]
	punpckhwd xmm1, [esi]
	punpcklwd xmm2, [esi+16]
	punpckhwd xmm7, [esi+16]

	UNPACK_RIGHTSHIFT xmm1, 16
	UNPACK_RIGHTSHIFT xmm7, 16
	UNPACK_RIGHTSHIFT xmm0, 16
	UNPACK_RIGHTSHIFT xmm2, 16

	UNPACK4_SSE CL, TOTALCL, MaskType, ModeType

	add esi, 32
	endm

UNPACK_V4_16SSE_4 macro CL, TOTALCL, MaskType, ModeType
	movdqu xmm0, [esi]
	movdqu xmm2, [esi+16]

	punpckhwd xmm1, xmm0
	punpckhwd xmm7, xmm2
	punpcklwd xmm0, xmm0
	punpcklwd xmm2, xmm2

	UNPACK_RIGHTSHIFT xmm1, 16
	UNPACK_RIGHTSHIFT xmm7, 16
	UNPACK_RIGHTSHIFT xmm0, 16
	UNPACK_RIGHTSHIFT xmm2, 16

	UNPACK4_SSE CL, TOTALCL, MaskType, ModeType

	add esi, 32
	endm

UNPACK_V4_16SSE_3A macro CL, TOTALCL, MaskType, ModeType
	punpcklwd xmm0, [esi]
	punpckhwd xmm1, [esi]
	punpcklwd xmm2, [esi+16]

	UNPACK_RIGHTSHIFT xmm0, 16
	UNPACK_RIGHTSHIFT xmm1, 16
	UNPACK_RIGHTSHIFT xmm2, 16

	UNPACK3_SSE CL, TOTALCL, MaskType, ModeType

	add esi, 24
	endm

UNPACK_V4_16SSE_3 macro CL, TOTALCL, MaskType, ModeType
	movdqu xmm0, [esi]
	movq xmm2, QWORD PTR [esi+16]

	punpckhwd xmm1, xmm0
	punpcklwd xmm0, xmm0
	punpcklwd xmm2, xmm2

	UNPACK_RIGHTSHIFT xmm0, 16
	UNPACK_RIGHTSHIFT xmm1, 16
	UNPACK_RIGHTSHIFT xmm2, 16

	UNPACK3_SSE CL, TOTALCL, MaskType, ModeType

	add esi, 24
	endm

UNPACK_V4_16SSE_2A macro CL, TOTALCL, MaskType, ModeType
	punpcklwd xmm0, [esi]
	punpckhwd xmm1, [esi]

	UNPACK_RIGHTSHIFT xmm0, 16
	UNPACK_RIGHTSHIFT xmm1, 16

	UNPACK2_SSE CL, TOTALCL, MaskType, ModeType

	add esi, 16
	endm

UNPACK_V4_16SSE_2 macro CL, TOTALCL, MaskType, ModeType
	movq xmm0, QWORD PTR [esi]
	movq xmm1, QWORD PTR [esi+8]

	punpcklwd xmm0, xmm0
	punpcklwd xmm1, xmm1

	UNPACK_RIGHTSHIFT xmm0, 16
	UNPACK_RIGHTSHIFT xmm1, 16

	UNPACK2_SSE CL, TOTALCL, MaskType, ModeType

	add esi, 16
	endm

UNPACK_V4_16SSE_1A macro CL, TOTALCL, MaskType, ModeType
	punpcklwd xmm0, [esi]
	UNPACK_RIGHTSHIFT xmm0, 16

	UNPACK1_SSE CL, TOTALCL, MaskType, ModeType

	add esi, 8
	endm

UNPACK_V4_16SSE_1 macro CL, TOTALCL, MaskType, ModeType
	movq xmm0, QWORD PTR [esi]
	punpcklwd xmm0, xmm0
	UNPACK_RIGHTSHIFT xmm0, 16

	UNPACK1_SSE CL, TOTALCL, MaskType, ModeType

	add esi, 8
	endm


UNPACK_V4_8SSE_4A macro CL, TOTALCL, MaskType, ModeType
	punpcklbw xmm0, [esi]
	punpckhbw xmm2, [esi]

	punpckhwd xmm1, xmm0
	punpckhwd xmm7, xmm2
	punpcklwd xmm0, xmm0
	punpcklwd xmm2, xmm2

	UNPACK_RIGHTSHIFT xmm1, 24
	UNPACK_RIGHTSHIFT xmm7, 24
	UNPACK_RIGHTSHIFT xmm0, 24
	UNPACK_RIGHTSHIFT xmm2, 24

	UNPACK4_SSE CL, TOTALCL, MaskType, ModeType

	add esi, 16
	endm

UNPACK_V4_8SSE_4 macro CL, TOTALCL, MaskType, ModeType
	movdqu xmm0, [esi]

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

	add esi, 16
	endm

UNPACK_V4_8SSE_3A macro CL, TOTALCL, MaskType, ModeType
	punpcklbw xmm0, [esi]
	punpckhbw xmm2, [esi]

	punpckhwd xmm1, xmm0
	punpcklwd xmm0, xmm0
	punpcklwd xmm2, xmm2

	UNPACK_RIGHTSHIFT xmm1, 24
	UNPACK_RIGHTSHIFT xmm0, 24
	UNPACK_RIGHTSHIFT xmm2, 24

	UNPACK3_SSE CL, TOTALCL, MaskType, ModeType

	add esi, 12
	endm

UNPACK_V4_8SSE_3 macro CL, TOTALCL, MaskType, ModeType
	movq xmm0, QWORD PTR [esi]
	movd xmm2, dword ptr [esi+8]

	punpcklbw xmm0, xmm0
	punpcklbw xmm2, xmm2

	punpckhwd xmm1, xmm0
	punpcklwd xmm2, xmm2
	punpcklwd xmm0, xmm0

	UNPACK_RIGHTSHIFT xmm1, 24
	UNPACK_RIGHTSHIFT xmm0, 24
	UNPACK_RIGHTSHIFT xmm2, 24

	UNPACK3_SSE CL, TOTALCL, MaskType, ModeType

	add esi, 12
	endm

UNPACK_V4_8SSE_2A macro CL, TOTALCL, MaskType, ModeType
	punpcklbw xmm0, [esi]

	punpckhwd xmm1, xmm0
	punpcklwd xmm0, xmm0

	UNPACK_RIGHTSHIFT xmm1, 24
	UNPACK_RIGHTSHIFT xmm0, 24

	UNPACK2_SSE CL, TOTALCL, MaskType, ModeType

	add esi, 8
	endm

UNPACK_V4_8SSE_2 macro CL, TOTALCL, MaskType, ModeType
	movq xmm0, QWORD PTR [esi]

	punpcklbw xmm0, xmm0

	punpckhwd xmm1, xmm0
	punpcklwd xmm0, xmm0

	UNPACK_RIGHTSHIFT xmm1, 24
	UNPACK_RIGHTSHIFT xmm0, 24

	UNPACK2_SSE CL, TOTALCL, MaskType, ModeType

	add esi, 8
	endm

UNPACK_V4_8SSE_1A macro CL, TOTALCL, MaskType, ModeType
	punpcklbw xmm0, [esi]
	punpcklwd xmm0, xmm0
	UNPACK_RIGHTSHIFT xmm0, 24

	UNPACK1_SSE CL, TOTALCL, MaskType, ModeType

	add esi, 4
	endm

UNPACK_V4_8SSE_1 macro CL, TOTALCL, MaskType, ModeType
	movd xmm0, dword ptr [esi]
	punpcklbw xmm0, xmm0
	punpcklwd xmm0, xmm0
	UNPACK_RIGHTSHIFT xmm0, 24

	UNPACK1_SSE CL, TOTALCL, MaskType, ModeType

	add esi, 4
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
	mov eax, dword ptr [esi]
	DECOMPRESS_RGBA 0

	shr eax, 16
	DECOMPRESS_RGBA 4

	mov eax, dword ptr [esi+4]
	DECOMPRESS_RGBA 8

	shr eax, 16
	DECOMPRESS_RGBA 12

	;; have to use movaps instead of movdqa
	movaps xmm0, [s_TempDecompress]

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

	add esi, 8
	endm

UNPACK_V4_5SSE_4A macro CL, TOTALCL, MaskType, ModeType
	UNPACK_V4_5SSE_4 CL, TOTALCL, MaskType, ModeType
	endm

UNPACK_V4_5SSE_3 macro CL, TOTALCL, MaskType, ModeType
	mov eax, dword ptr [esi]
	DECOMPRESS_RGBA 0

	shr eax, 16
	DECOMPRESS_RGBA 4

	mov eax, dword ptr [esi]
	DECOMPRESS_RGBA 8

	;; have to use movaps instead of movdqa
	movaps xmm0, [s_TempDecompress]

	punpckhbw xmm2, xmm0
	punpcklbw xmm0, xmm0

	punpckhwd xmm1, xmm0
	punpcklwd xmm0, xmm0
	punpcklwd xmm2, xmm2

	psrld xmm0, 24
	psrld xmm1, 24
	psrld xmm2, 24

	UNPACK3_SSE CL, TOTALCL, MaskType, ModeType

	add esi, 6
	endm

UNPACK_V4_5SSE_3A macro CL, TOTALCL, MaskType, ModeType
	UNPACK_V4_5SSE_3 CL, TOTALCL, MaskType, ModeType
	endm

UNPACK_V4_5SSE_2 macro CL, TOTALCL, MaskType, ModeType
	mov eax, dword ptr [esi]
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

	add esi, 4
	endm

UNPACK_V4_5SSE_2A macro CL, TOTALCL, MaskType, ModeType
	UNPACK_V4_5SSE_2 CL, TOTALCL, MaskType, ModeType
	endm

UNPACK_V4_5SSE_1 macro CL, TOTALCL, MaskType, ModeType
	mov ax, word ptr [esi]
	DECOMPRESS_RGBA 0

	movd xmm0, DWORD PTR [s_TempDecompress]
	punpcklbw xmm0, xmm0
	punpcklwd xmm0, xmm0

	psrld xmm0, 24

	UNPACK1_SSE CL, TOTALCL, MaskType, ModeType

	add esi, 2
	endm

UNPACK_V4_5SSE_1A macro CL, TOTALCL, MaskType, ModeType
	UNPACK_V4_5SSE_1 CL, TOTALCL, MaskType, ModeType
	endm


SAVE_ROW_REG_BASE macro
	mov eax, [vifRow]
	movdqa [eax], xmm6
	mov eax, [vifRegs]
	movss dword ptr [eax+0100h], xmm6
	psrldq xmm6, 4
	movss dword ptr [eax+0110h], xmm6
	psrldq xmm6, 4
	movss dword ptr [eax+0120h], xmm6
	psrldq xmm6, 4
	movss dword ptr [eax+0130h], xmm6
	endm

SAVE_NO_REG macro
	endm

INIT_ARGS macro
    mov edi, dword ptr [esp+4+12]
    mov esi, dword ptr [esp+8+12]
    mov edx, dword ptr [esp+12+12]
    endm

INC_STACK macro reg
	add esp, 4
	endm





defUNPACK_SkippingWrite macro name, MaskType, ModeType, qsize, sign, SAVE_ROW_REG
@CatStr(UNPACK_SkippingWrite_, name, _, sign, _, MaskType, _, ModeType) proc public
	push edi
	push esi
	push ebx

	INIT_ARGS
    mov eax, [vifRegs]
    movzx ecx, byte ptr [eax + 040h]
    movzx ebx, byte ptr [eax + 041h]
    sub ecx, ebx
    shl ecx, 4

    cmp ebx, 1
    je @CatStr(name, _, sign, _, MaskType, _, ModeType, _WL1)
    cmp ebx, 2
    je @CatStr(name, _, sign, _, MaskType, _, ModeType, _WL2)
    cmp ebx, 3
    je @CatStr(name, _, sign, _, MaskType, _, ModeType, _WL3)
    jmp @CatStr(name, _, sign, _, MaskType, _, ModeType, _WL4)

@CatStr(name, _, sign, _, MaskType, _, ModeType, _WL1):
    @CatStr(UNPACK_Start_Setup_, MaskType, _SSE_, ModeType) 0

	cmp edx, qsize
	jl @CatStr(name, _, sign, _, MaskType, _, ModeType, _C1_Done3)

	add ecx, 16


@CatStr(name, _, sign, _, MaskType, _, ModeType, _C1_Align16):

	test esi, 15
	jz @CatStr(name, _, sign, _, MaskType, _, ModeType, _C1_UnpackAligned)

	@CatStr(UNPACK_, name, SSE_1) 0, 1, MaskType, ModeType

	cmp edx, (2*qsize)
	jl @CatStr(name, _, sign, _, MaskType, _, ModeType, _C1_DoneWithDec)
	sub edx, qsize
	jmp @CatStr(name, _, sign, _, MaskType, _, ModeType, _C1_Align16)

@CatStr(name, _, sign, _, MaskType, _, ModeType, _C1_UnpackAligned):

	cmp edx, (2*qsize)
	jl @CatStr(name, _, sign, _, MaskType, _, ModeType, _C1_Unpack1)
	cmp edx, (3*qsize)
	jl @CatStr(name, _, sign, _, MaskType, _, ModeType, _C1_Unpack2)
	cmp edx, (4*qsize)
	jl @CatStr(name, _, sign, _, MaskType, _, ModeType, _C1_Unpack3)
	prefetchnta [esi + 64]

@CatStr(name, _, sign, _, MaskType, _, ModeType, _C1_Unpack4):
	@CatStr(UNPACK_, name, SSE_4A) 0, 1, MaskType, ModeType

	cmp edx, (8*qsize)
	jl @CatStr(name, _, sign, _, MaskType, _, ModeType, _C1_DoneUnpack4)
	sub edx, (4*qsize)
	jmp @CatStr(name, _, sign, _, MaskType, _, ModeType, _C1_Unpack4)

@CatStr(name, _, sign, _, MaskType, _, ModeType, _C1_DoneUnpack4):

	sub edx, (4*qsize)
	cmp edx, qsize
	jl @CatStr(name, _, sign, _, MaskType, _, ModeType, _C1_Done3)
	cmp edx, (2*qsize)
	jl @CatStr(name, _, sign, _, MaskType, _, ModeType, _C1_Unpack1)
	cmp edx, (3*qsize)
	jl @CatStr(name, _, sign, _, MaskType, _, ModeType, _C1_Unpack2)


@CatStr(name, _, sign, _, MaskType, _, ModeType, _C1_Unpack3):
	@CatStr(UNPACK_, name, SSE_3A) 0, 1, MaskType, ModeType

	sub edx, (3*qsize)
	jmp @CatStr(name, _, sign, _, MaskType, _, ModeType, _C1_Done3)

@CatStr(name, _, sign, _, MaskType, _, ModeType, _C1_Unpack2):
	@CatStr(UNPACK_, name, SSE_2A) 0, 1, MaskType, ModeType

	sub edx, (2*qsize)
	jmp @CatStr(name, _, sign, _, MaskType, _, ModeType, _C1_Done3)

@CatStr(name, _, sign, _, MaskType, _, ModeType, _C1_Unpack1):
	@CatStr(UNPACK_, name, SSE_1A) 0, 1, MaskType, ModeType
@CatStr(name, _, sign, _, MaskType, _, ModeType, _C1_DoneWithDec):
	sub edx, qsize
@CatStr(name, _, sign, _, MaskType, _, ModeType, _C1_Done3):
	SAVE_ROW_REG
    mov eax, edx
	pop ebx
	pop esi
	pop edi

    ret

@CatStr(name, _, sign, _, MaskType, _, ModeType, _WL2):
	cmp edx, (2*qsize)

	jl @CatStr(name, _, sign, _, MaskType, _, ModeType, _C2_Done3)
@CatStr(name, _, sign, _, MaskType, _, ModeType, _C2_Unpack):
	@CatStr(UNPACK_, name, SSE_2) 0, 0, MaskType, ModeType


	add edi, ecx
	cmp edx, (4*qsize)
	jl @CatStr(name, _, sign, _, MaskType, _, ModeType, _C2_Done2)
	sub edx, (2*qsize)

	jmp @CatStr(name, _, sign, _, MaskType, _, ModeType, _C2_Unpack)

@CatStr(name, _, sign, _, MaskType, _, ModeType, _C2_Done2):
	sub edx, (2*qsize)
@CatStr(name, _, sign, _, MaskType, _, ModeType, _C2_Done3):
	cmp edx, qsize

	jl @CatStr(name, _, sign, _, MaskType, _, ModeType, _C2_Done4)


	@CatStr(UNPACK_, name, SSE_1) 0, 0, MaskType, ModeType

	sub edx, qsize
@CatStr(name, _, sign, _, MaskType, _, ModeType, _C2_Done4):

	SAVE_ROW_REG
    mov eax, edx
	pop ebx
	pop esi
	pop edi

	ret

@CatStr(name, _, sign, _, MaskType, _, ModeType, _WL3):
	cmp edx, (3*qsize)

	jl @CatStr(name, _, sign, _, MaskType, _, ModeType, _C3_Done5)
@CatStr(name, _, sign, _, MaskType, _, ModeType, _C3_Unpack):
	@CatStr(UNPACK_, name, SSE_3) 0, 0, MaskType, ModeType

	add edi, ecx
	cmp edx, (6*qsize)
	jl @CatStr(name, _, sign, _, MaskType, _, ModeType, _C3_Done2)
	sub edx, (3*qsize)
	jmp @CatStr(name, _, sign, _, MaskType, _, ModeType, _C3_Unpack)
@CatStr(name, _, sign, _, MaskType, _, ModeType, _C3_Done2):
	sub edx, (3*qsize)
@CatStr(name, _, sign, _, MaskType, _, ModeType, _C3_Done5):
	cmp edx, qsize
	jl @CatStr(name, _, sign, _, MaskType, _, ModeType, _C3_Done4)

    cmp edx, (2*qsize)
	jl @CatStr(name, _, sign, _, MaskType, _, ModeType, _C3_Done3)

    @CatStr(UNPACK_, name, SSE_2) 0, 0, MaskType, ModeType

	sub edx, (2*qsize)
	jmp @CatStr(name, _, sign, _, MaskType, _, ModeType, _C3_Done4)
@CatStr(name, _, sign, _, MaskType, _, ModeType, _C3_Done3):
	sub edx, qsize
	@CatStr(UNPACK_, name, SSE_1) 0, 0, MaskType, ModeType
@CatStr(name, _, sign, _, MaskType, _, ModeType, _C3_Done4):
	SAVE_ROW_REG
    mov eax, edx
	pop ebx
	pop esi
	pop edi

    ret

@CatStr(name, _, sign, _, MaskType, _, ModeType, _WL4):
	sub ebx, 3
	push ecx
	cmp edx, qsize
	jl @CatStr(name, _, sign, _, MaskType, _, ModeType, _C4_Done)

@CatStr(name, _, sign, _, MaskType, _, ModeType, _C4_Unpack):
	cmp edx, (3*qsize)
	jge @CatStr(name, _, sign, _, MaskType, _, ModeType, _C4_Unpack3)
	cmp edx, (2*qsize)
	jge @CatStr(name, _, sign, _, MaskType, _, ModeType, _C4_Unpack2)

	@CatStr(UNPACK_, name, SSE_1) 0, 0, MaskType, ModeType


    sub edx, qsize
	jmp @CatStr(name, _, sign, _, MaskType, _, ModeType, _C4_Done)
@CatStr(name, _, sign, _, MaskType, _, ModeType, _C4_Unpack2):
	@CatStr(UNPACK_, name, SSE_2) 0, 0, MaskType, ModeType


    sub edx, (2*qsize)
	jmp @CatStr(name, _, sign, _, MaskType, _, ModeType, _C4_Done)
@CatStr(name, _, sign, _, MaskType, _, ModeType, _C4_Unpack3):
	@CatStr(UNPACK_, name, SSE_3) 0, 0, MaskType, ModeType


	sub edx, (3*qsize)
    mov ecx, ebx

@CatStr(name, _, sign, _, MaskType, _, ModeType, _C4_UnpackX):


    cmp edx, qsize
	jl @CatStr(name, _, sign, _, MaskType, _, ModeType, _C4_Done)

	@CatStr(UNPACK_, name, SSE_1) 3, 0, MaskType, ModeType

	sub edx, qsize
	cmp ecx, 1
	je @CatStr(name, _, sign, _, MaskType, _, ModeType, _C4_DoneLoop)
	sub ecx, 1
	jmp @CatStr(name, _, sign, _, MaskType, _, ModeType, _C4_UnpackX)
@CatStr(name, _, sign, _, MaskType, _, ModeType, _C4_DoneLoop):
	add edi, [esp]
	cmp edx, qsize
	jl @CatStr(name, _, sign, _, MaskType, _, ModeType, _C4_Done)
	jmp @CatStr(name, _, sign, _, MaskType, _, ModeType, _C4_Unpack)
@CatStr(name, _, sign, _, MaskType, _, ModeType, _C4_Done):

	SAVE_ROW_REG
	INC_STACK()
    mov eax, edx
	pop ebx
	pop esi
	pop edi

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
