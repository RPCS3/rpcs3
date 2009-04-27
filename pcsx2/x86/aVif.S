/*Pcsx2 - Pc Ps2 Emulator
  Copyright (C) 2002-2007  Pcsx2 Team

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

*/
.intel_syntax noprefix
        
.extern vifRegs
.extern vifMaskRegs
.extern vifRow
        
#define VIF_ESP esp
#define VIF_SRC	esi
#define VIF_INC	ecx
#define VIF_DST edi
#define VIF_SIZE edx
#define VIF_TMPADDR eax
#define VIF_SAVEEBX ebx
#define VIF_SAVEEBXd ebx

#define XMM_R0			xmm0
#define XMM_R1			xmm1
#define XMM_R2			xmm2
#define XMM_WRITEMASK	xmm3
#define XMM_ROWMASK		xmm4
#define XMM_ROWCOLMASK	xmm5
#define XMM_ROW			xmm6
#define XMM_COL			xmm7

#define XMM_R3			XMM_COL

// writing masks
#define UNPACK_Write0_Regular(r0, CL, DEST_OFFSET, MOVDQA) \
	MOVDQA xmmword ptr [VIF_DST+DEST_OFFSET], r0;

#define UNPACK_Write1_Regular(r0, CL, DEST_OFFSET, MOVDQA) \
	MOVDQA xmmword ptr [VIF_DST], r0; \
	add VIF_DST, VIF_INC; \

#define UNPACK_Write0_Mask UNPACK_Write0_Regular
#define UNPACK_Write1_Mask UNPACK_Write1_Regular

// masked write (dest needs to be in edi)
#define UNPACK_Write0_WriteMask(r0, CL, DEST_OFFSET, MOVDQA) \
	movdqa XMM_WRITEMASK, xmmword ptr [VIF_TMPADDR + 64*(CL) + 48]; \
	pand r0, XMM_WRITEMASK; \
	pandn XMM_WRITEMASK, xmmword ptr [VIF_DST]; \
	por r0, XMM_WRITEMASK; \
	MOVDQA xmmword ptr [VIF_DST], r0; \
	add VIF_DST, 16; \

// masked write (dest needs to be in edi)
#define UNPACK_Write1_WriteMask(r0, CL, DEST_OFFSET, MOVDQA) \
	movdqa XMM_WRITEMASK, xmmword ptr [VIF_TMPADDR + 64*(0) + 48]; \
	pand r0, XMM_WRITEMASK; \
	pandn XMM_WRITEMASK, xmmword ptr [VIF_DST]; \
	por r0, XMM_WRITEMASK; \
	MOVDQA xmmword ptr [VIF_DST], r0; \
	add VIF_DST, VIF_INC; \

#define UNPACK_Mask_SSE_0(r0) \
	pand r0, XMM_WRITEMASK; \
	por r0, XMM_ROWCOLMASK; \

// once a xmmword is uncomprssed, applies masks and saves
// note: modifying XMM_WRITEMASK
// dest = row + write (only when mask=0), otherwise write
#define UNPACK_Mask_SSE_1(r0) \
	pand r0, XMM_WRITEMASK; \
	por r0, XMM_ROWCOLMASK; \
	pand XMM_WRITEMASK, XMM_ROW; \
	paddd r0, XMM_WRITEMASK; \

// dest = row + write (only when mask=0), otherwise write
// row = row + write (only when mask = 0), otherwise row
#define UNPACK_Mask_SSE_2(r0) \
	pand r0, XMM_WRITEMASK; \
	pand XMM_WRITEMASK, XMM_ROW; \
	paddd XMM_ROW, r0; \
	por r0, XMM_ROWCOLMASK; \
	paddd r0, XMM_WRITEMASK; \

#define UNPACK_WriteMask_SSE_0 UNPACK_Mask_SSE_0
#define UNPACK_WriteMask_SSE_1 UNPACK_Mask_SSE_1
#define UNPACK_WriteMask_SSE_2 UNPACK_Mask_SSE_2

#define UNPACK_Regular_SSE_0(r0)

#define UNPACK_Regular_SSE_1(r0) \
	paddd r0, XMM_ROW; \

#define UNPACK_Regular_SSE_2(r0) \
	paddd r0, XMM_ROW; \
	movdqa XMM_ROW, r0; \

// setting up masks
#define UNPACK_Setup_Mask_SSE(CL) \
	mov VIF_TMPADDR, vifMaskRegs; \
	movdqa XMM_ROWMASK, xmmword ptr [VIF_TMPADDR + 64*(CL) + 16]; \
	movdqa XMM_ROWCOLMASK, xmmword ptr [VIF_TMPADDR + 64*(CL) + 32]; \
	movdqa XMM_WRITEMASK, xmmword ptr [VIF_TMPADDR + 64*(CL)]; \
	pand XMM_ROWMASK, XMM_ROW; \
	pand XMM_ROWCOLMASK, XMM_COL; \
	por XMM_ROWCOLMASK, XMM_ROWMASK; \

#define UNPACK_Start_Setup_Mask_SSE_0(CL) UNPACK_Setup_Mask_SSE(CL)
#define UNPACK_Start_Setup_Mask_SSE_1(CL) \
	mov VIF_TMPADDR, vifMaskRegs; \
	movdqa XMM_ROWMASK, xmmword ptr [VIF_TMPADDR + 64*(CL) + 16]; \
	movdqa XMM_ROWCOLMASK, xmmword ptr [VIF_TMPADDR + 64*(CL) + 32]; \
	pand XMM_ROWMASK, XMM_ROW; \
	pand XMM_ROWCOLMASK, XMM_COL; \
	por XMM_ROWCOLMASK, XMM_ROWMASK; \

#define UNPACK_Start_Setup_Mask_SSE_2(CL)

#define UNPACK_Setup_Mask_SSE_0_1(CL) 
#define UNPACK_Setup_Mask_SSE_1_1(CL) \
	mov VIF_TMPADDR, vifMaskRegs; \
	movdqa XMM_WRITEMASK, xmmword ptr [VIF_TMPADDR + 64*(0)]; \

// ignore CL, since vif.cycle.wl == 1
#define UNPACK_Setup_Mask_SSE_2_1(CL) \
	mov VIF_TMPADDR, vifMaskRegs; \
	movdqa XMM_ROWMASK, xmmword ptr [VIF_TMPADDR + 64*(0) + 16]; \
	movdqa XMM_ROWCOLMASK, xmmword ptr [VIF_TMPADDR + 64*(0) + 32]; \
	movdqa XMM_WRITEMASK, xmmword ptr [VIF_TMPADDR + 64*(0)]; \
	pand XMM_ROWMASK, XMM_ROW; \
	pand XMM_ROWCOLMASK, XMM_COL; \
	por XMM_ROWCOLMASK, XMM_ROWMASK; \

#define UNPACK_Setup_Mask_SSE_0_0(CL) UNPACK_Setup_Mask_SSE(CL)
#define UNPACK_Setup_Mask_SSE_1_0(CL) UNPACK_Setup_Mask_SSE(CL)
#define UNPACK_Setup_Mask_SSE_2_0(CL) UNPACK_Setup_Mask_SSE(CL)

// write mask always destroys XMM_WRITEMASK, so 0_0 = 1_0
#define UNPACK_Setup_WriteMask_SSE_0_0(CL) UNPACK_Setup_Mask_SSE(CL)
#define UNPACK_Setup_WriteMask_SSE_1_0(CL) UNPACK_Setup_Mask_SSE(CL)
#define UNPACK_Setup_WriteMask_SSE_2_0(CL) UNPACK_Setup_Mask_SSE(CL)
#define UNPACK_Setup_WriteMask_SSE_0_1(CL) UNPACK_Setup_Mask_SSE_1_1(CL)
#define UNPACK_Setup_WriteMask_SSE_1_1(CL) UNPACK_Setup_Mask_SSE_1_1(CL)
#define UNPACK_Setup_WriteMask_SSE_2_1(CL) UNPACK_Setup_Mask_SSE_2_1(CL)

#define UNPACK_Start_Setup_WriteMask_SSE_0(CL) UNPACK_Start_Setup_Mask_SSE_1(CL)
#define UNPACK_Start_Setup_WriteMask_SSE_1(CL) UNPACK_Start_Setup_Mask_SSE_1(CL)
#define UNPACK_Start_Setup_WriteMask_SSE_2(CL) UNPACK_Start_Setup_Mask_SSE_2(CL)

#define UNPACK_Start_Setup_Regular_SSE_0(CL)
#define UNPACK_Start_Setup_Regular_SSE_1(CL)
#define UNPACK_Start_Setup_Regular_SSE_2(CL)
#define UNPACK_Setup_Regular_SSE_0_0(CL)
#define UNPACK_Setup_Regular_SSE_1_0(CL)
#define UNPACK_Setup_Regular_SSE_2_0(CL)
#define UNPACK_Setup_Regular_SSE_0_1(CL)
#define UNPACK_Setup_Regular_SSE_1_1(CL)
#define UNPACK_Setup_Regular_SSE_2_1(CL)

#define UNPACK_INC_DST_0_Regular(qw) add VIF_DST, (16*qw)
#define UNPACK_INC_DST_1_Regular(qw)
#define UNPACK_INC_DST_0_Mask(qw) add VIF_DST, (16*qw)
#define UNPACK_INC_DST_1_Mask(qw)
#define UNPACK_INC_DST_0_WriteMask(qw)
#define UNPACK_INC_DST_1_WriteMask(qw)

// unpacks for 1,2,3,4 elements (V3 uses this directly)
#define UNPACK4_SSE(CL, TOTALCL, MaskType, ModeType) \
	UNPACK_Setup_##MaskType##_SSE_##ModeType##_##TOTALCL##(CL+0); \
	UNPACK_##MaskType##_SSE_##ModeType##(XMM_R0); \
	UNPACK_Write##TOTALCL##_##MaskType##(XMM_R0, CL, 0, movdqa); \
	\
	UNPACK_Setup_##MaskType##_SSE_##ModeType##_##TOTALCL##(CL+1); \
	UNPACK_##MaskType##_SSE_##ModeType##(XMM_R1); \
	UNPACK_Write##TOTALCL##_##MaskType##(XMM_R1, CL+1, 16, movdqa); \
	\
	UNPACK_Setup_##MaskType##_SSE_##ModeType##_##TOTALCL##(CL+2); \
	UNPACK_##MaskType##_SSE_##ModeType##(XMM_R2); \
	UNPACK_Write##TOTALCL##_##MaskType##(XMM_R2, CL+2, 32, movdqa); \
	\
	UNPACK_Setup_##MaskType##_SSE_##ModeType##_##TOTALCL##(CL+3); \
	UNPACK_##MaskType##_SSE_##ModeType##(XMM_R3); \
	UNPACK_Write##TOTALCL##_##MaskType##(XMM_R3, CL+3, 48, movdqa); \
	\
	UNPACK_INC_DST_##TOTALCL##_##MaskType##(4)

// V3 uses this directly
#define UNPACK3_SSE(CL, TOTALCL, MaskType, ModeType) \
	UNPACK_Setup_##MaskType##_SSE_##ModeType##_##TOTALCL##(CL); \
	UNPACK_##MaskType##_SSE_##ModeType##(XMM_R0); \
	UNPACK_Write##TOTALCL##_##MaskType##(XMM_R0, CL, 0, movdqa); \
	\
	UNPACK_Setup_##MaskType##_SSE_##ModeType##_##TOTALCL##(CL+1); \
	UNPACK_##MaskType##_SSE_##ModeType##(XMM_R1); \
	UNPACK_Write##TOTALCL##_##MaskType##(XMM_R1, CL+1, 16, movdqa); \
	\
	UNPACK_Setup_##MaskType##_SSE_##ModeType##_##TOTALCL##(CL+2); \
	UNPACK_##MaskType##_SSE_##ModeType##(XMM_R2); \
	UNPACK_Write##TOTALCL##_##MaskType##(XMM_R2, CL+2, 32, movdqa); \
	\
	UNPACK_INC_DST_##TOTALCL##_##MaskType##(3); \

#define UNPACK2_SSE(CL, TOTALCL, MaskType, ModeType) \
	UNPACK_Setup_##MaskType##_SSE_##ModeType##_##TOTALCL##(CL); \
	UNPACK_##MaskType##_SSE_##ModeType##(XMM_R0); \
	UNPACK_Write##TOTALCL##_##MaskType##(XMM_R0, CL, 0, movdqa); \
	\
	UNPACK_Setup_##MaskType##_SSE_##ModeType##_##TOTALCL##(CL+1); \
	UNPACK_##MaskType##_SSE_##ModeType##(XMM_R1); \
	UNPACK_Write##TOTALCL##_##MaskType##(XMM_R1, CL+1, 16, movdqa); \
	\
	UNPACK_INC_DST_##TOTALCL##_##MaskType##(2); \

#define UNPACK1_SSE(CL, TOTALCL, MaskType, ModeType) \
	UNPACK_Setup_##MaskType##_SSE_##ModeType##_##TOTALCL##(CL); \
	UNPACK_##MaskType##_SSE_##ModeType##(XMM_R0); \
	UNPACK_Write##TOTALCL##_##MaskType##(XMM_R0, CL, 0, movdqa); \
	\
	UNPACK_INC_DST_##TOTALCL##_##MaskType##(1); \

// S-32
// only when cl==1
#define UNPACK_S_32SSE_4x(CL, TOTALCL, MaskType, ModeType, MOVDQA) \
	MOVDQA XMM_R3, xmmword ptr [VIF_SRC]; \
	\
	pshufd XMM_R0, XMM_R3, 0; \
	pshufd XMM_R1, XMM_R3, 0x55; \
	pshufd XMM_R2, XMM_R3, 0xaa; \
	pshufd XMM_R3, XMM_R3, 0xff; \
	\
	UNPACK4_SSE(CL, TOTALCL, MaskType, ModeType); \
	\
	add VIF_SRC, 16; \

#define UNPACK_S_32SSE_4A(CL, TOTALCL, MaskType, ModeType) UNPACK_S_32SSE_4x(CL, TOTALCL, MaskType, ModeType, movdqa)
#define UNPACK_S_32SSE_4(CL, TOTALCL, MaskType, ModeType) UNPACK_S_32SSE_4x(CL, TOTALCL, MaskType, ModeType, movdqu)

#define UNPACK_S_32SSE_3x(CL, TOTALCL, MaskType, ModeType, MOVDQA) \
	MOVDQA XMM_R2, xmmword ptr [VIF_SRC]; \
	\
	pshufd XMM_R0, XMM_R2, 0; \
	pshufd XMM_R1, XMM_R2, 0x55; \
	pshufd XMM_R2, XMM_R2, 0xaa; \
	\
	UNPACK3_SSE(CL, TOTALCL, MaskType, ModeType); \
	\
	add VIF_SRC, 12; \

#define UNPACK_S_32SSE_3A(CL, TOTALCL, MaskType, ModeType) UNPACK_S_32SSE_3x(CL, TOTALCL, MaskType, ModeType, movdqa)
#define UNPACK_S_32SSE_3(CL, TOTALCL, MaskType, ModeType) UNPACK_S_32SSE_3x(CL, TOTALCL, MaskType, ModeType, movdqu)

#define UNPACK_S_32SSE_2(CL, TOTALCL, MaskType, ModeType) \
	movq XMM_R1, qword ptr [VIF_SRC]; \
	\
	pshufd XMM_R0, XMM_R1, 0; \
	pshufd XMM_R1, XMM_R1, 0x55; \
	\
	UNPACK2_SSE(CL, TOTALCL, MaskType, ModeType); \
	\
	add VIF_SRC, 8; \

#define UNPACK_S_32SSE_2A UNPACK_S_32SSE_2

#define UNPACK_S_32SSE_1(CL, TOTALCL, MaskType, ModeType) \
	movd XMM_R0, dword ptr [VIF_SRC]; \
	pshufd XMM_R0, XMM_R0, 0; \
	\
	UNPACK1_SSE(CL, TOTALCL, MaskType, ModeType); \
	\
	add VIF_SRC, 4; \

#define UNPACK_S_32SSE_1A UNPACK_S_32SSE_1

// S-16
#define UNPACK_S_16SSE_4(CL, TOTALCL, MaskType, ModeType) \
	movq XMM_R3, qword ptr [VIF_SRC]; \
	punpcklwd XMM_R3, XMM_R3; \
	UNPACK_RIGHTSHIFT XMM_R3, 16; \
	\
	pshufd XMM_R0, XMM_R3, 0; \
	pshufd XMM_R1, XMM_R3, 0x55; \
	pshufd XMM_R2, XMM_R3, 0xaa; \
	pshufd XMM_R3, XMM_R3, 0xff; \
	\
	UNPACK4_SSE(CL, TOTALCL, MaskType, ModeType); \
	\
	add VIF_SRC, 8; \

#define UNPACK_S_16SSE_4A UNPACK_S_16SSE_4

#define UNPACK_S_16SSE_3(CL, TOTALCL, MaskType, ModeType) \
	movq XMM_R2, qword ptr [VIF_SRC]; \
	punpcklwd XMM_R2, XMM_R2; \
	UNPACK_RIGHTSHIFT XMM_R2, 16; \
	\
	pshufd XMM_R0, XMM_R2, 0; \
	pshufd XMM_R1, XMM_R2, 0x55; \
	pshufd XMM_R2, XMM_R2, 0xaa; \
	\
	UNPACK3_SSE(CL, TOTALCL, MaskType, ModeType); \
	add VIF_SRC, 6; \

#define UNPACK_S_16SSE_3A UNPACK_S_16SSE_3

#define UNPACK_S_16SSE_2(CL, TOTALCL, MaskType, ModeType) \
	movd XMM_R1, dword ptr [VIF_SRC]; \
	punpcklwd XMM_R1, XMM_R1; \
	UNPACK_RIGHTSHIFT XMM_R1, 16; \
	\
	pshufd XMM_R0, XMM_R1, 0; \
	pshufd XMM_R1, XMM_R1, 0x55; \
	\
	UNPACK2_SSE(CL, TOTALCL, MaskType, ModeType); \
	\
	add VIF_SRC, 4; \

#define UNPACK_S_16SSE_2A UNPACK_S_16SSE_2

#define UNPACK_S_16SSE_1(CL, TOTALCL, MaskType, ModeType) \
	movd XMM_R0, dword ptr [VIF_SRC]; \
	punpcklwd XMM_R0, XMM_R0; \
	UNPACK_RIGHTSHIFT XMM_R0, 16; \
	pshufd XMM_R0, XMM_R0, 0; \
	\
	UNPACK1_SSE(CL, TOTALCL, MaskType, ModeType); \
	\
	add VIF_SRC, 2; \

#define UNPACK_S_16SSE_1A UNPACK_S_16SSE_1

// S-8
#define UNPACK_S_8SSE_4(CL, TOTALCL, MaskType, ModeType) \
	movd XMM_R3, dword ptr [VIF_SRC]; \
	punpcklbw XMM_R3, XMM_R3; \
	punpcklwd XMM_R3, XMM_R3; \
	UNPACK_RIGHTSHIFT XMM_R3, 24; \
	\
	pshufd XMM_R0, XMM_R3, 0; \
	pshufd XMM_R1, XMM_R3, 0x55; \
	pshufd XMM_R2, XMM_R3, 0xaa; \
	pshufd XMM_R3, XMM_R3, 0xff; \
	\
	UNPACK4_SSE(CL, TOTALCL, MaskType, ModeType); \
	\
	add VIF_SRC, 4; \
	
#define UNPACK_S_8SSE_4A UNPACK_S_8SSE_4

#define UNPACK_S_8SSE_3(CL, TOTALCL, MaskType, ModeType) \
	movd XMM_R2, dword ptr [VIF_SRC]; \
	punpcklbw XMM_R2, XMM_R2; \
	punpcklwd XMM_R2, XMM_R2; \
	UNPACK_RIGHTSHIFT XMM_R2, 24; \
	\
	pshufd XMM_R0, XMM_R2, 0; \
	pshufd XMM_R1, XMM_R2, 0x55; \
	pshufd XMM_R2, XMM_R2, 0xaa; \
	\
	UNPACK3_SSE(CL, TOTALCL, MaskType, ModeType); \
	\
	add VIF_SRC, 3; \
	
#define UNPACK_S_8SSE_3A UNPACK_S_8SSE_3

#define UNPACK_S_8SSE_2(CL, TOTALCL, MaskType, ModeType) \
	movd XMM_R1, dword ptr [VIF_SRC]; \
	punpcklbw XMM_R1, XMM_R1; \
	punpcklwd XMM_R1, XMM_R1; \
	UNPACK_RIGHTSHIFT XMM_R1, 24; \
	\
	pshufd XMM_R0, XMM_R1, 0; \
	pshufd XMM_R1, XMM_R1, 0x55; \
	\
	UNPACK2_SSE(CL, TOTALCL, MaskType, ModeType); \
	\
	add VIF_SRC, 2; \

#define UNPACK_S_8SSE_2A UNPACK_S_8SSE_2

#define UNPACK_S_8SSE_1(CL, TOTALCL, MaskType, ModeType) \
	movd XMM_R0, dword ptr [VIF_SRC]; \
	punpcklbw XMM_R0, XMM_R0; \
	punpcklwd XMM_R0, XMM_R0; \
	UNPACK_RIGHTSHIFT XMM_R0, 24; \
	pshufd XMM_R0, XMM_R0, 0; \
	\
	UNPACK1_SSE(CL, TOTALCL, MaskType, ModeType); \
	\
	inc VIF_SRC; \

#define UNPACK_S_8SSE_1A UNPACK_S_8SSE_1

// V2-32
#define UNPACK_V2_32SSE_4A(CL, TOTALCL, MaskType, ModeType) \
	MOVDQA XMM_R0, xmmword ptr [VIF_SRC]; \
	MOVDQA XMM_R2, xmmword ptr [VIF_SRC+16]; \
	\
	pshufd XMM_R1, XMM_R0, 0xee; \
	pshufd XMM_R3, XMM_R2, 0xee; \
	\
	UNPACK4_SSE(CL, TOTALCL, MaskType, ModeType); \
	\
	add VIF_SRC, 32; \

#define UNPACK_V2_32SSE_4(CL, TOTALCL, MaskType, ModeType) \
	movq XMM_R0, qword ptr [VIF_SRC]; \
	movq XMM_R1, qword ptr [VIF_SRC+8]; \
	movq XMM_R2, qword ptr [VIF_SRC+16]; \
	movq XMM_R3, qword ptr [VIF_SRC+24]; \
	\
	UNPACK4_SSE(CL, TOTALCL, MaskType, ModeType); \
	\
	add VIF_SRC, 32; \

#define UNPACK_V2_32SSE_3A(CL, TOTALCL, MaskType, ModeType) \
	MOVDQA XMM_R0, xmmword ptr [VIF_SRC]; \
	movq XMM_R2, qword ptr [VIF_SRC+16]; \
	pshufd XMM_R1, XMM_R0, 0xee; \
	\
	UNPACK3_SSE(CL, TOTALCL, MaskType, ModeType); \
	\
	add VIF_SRC, 24; \

#define UNPACK_V2_32SSE_3(CL, TOTALCL, MaskType, ModeType) \
	movq XMM_R0, qword ptr [VIF_SRC]; \
	movq XMM_R1, qword ptr [VIF_SRC+8]; \
	movq XMM_R2, qword ptr [VIF_SRC+16]; \
	\
	UNPACK3_SSE(CL, TOTALCL, MaskType, ModeType); \
	\
	add VIF_SRC, 24; \
	
#define UNPACK_V2_32SSE_2(CL, TOTALCL, MaskType, ModeType) \
	movq XMM_R0, qword ptr [VIF_SRC]; \
	movq XMM_R1, qword ptr [VIF_SRC+8]; \
	\
	UNPACK2_SSE(CL, TOTALCL, MaskType, ModeType); \
	\
	add VIF_SRC, 16; \

#define UNPACK_V2_32SSE_2A UNPACK_V2_32SSE_2

#define UNPACK_V2_32SSE_1(CL, TOTALCL, MaskType, ModeType) \
	movq XMM_R0, qword ptr [VIF_SRC]; \
	\
	UNPACK1_SSE(CL, TOTALCL, MaskType, ModeType); \
	\
	add VIF_SRC, 8; \
	
#define UNPACK_V2_32SSE_1A UNPACK_V2_32SSE_1

// V2-16
// due to lemmings, have to copy lower xmmword to the upper xmmword of every reg
#define UNPACK_V2_16SSE_4A(CL, TOTALCL, MaskType, ModeType) \
	punpcklwd XMM_R0, xmmword ptr [VIF_SRC]; \
	punpckhwd XMM_R2, xmmword ptr [VIF_SRC]; \
	\
	UNPACK_RIGHTSHIFT XMM_R0, 16; \
	UNPACK_RIGHTSHIFT XMM_R2, 16; \
	\
	punpckhqdq XMM_R1, XMM_R0; \
	punpckhqdq XMM_R3, XMM_R2; \
	\
	punpcklqdq XMM_R0, XMM_R0; \
	punpcklqdq XMM_R2, XMM_R2; \
	punpckhqdq XMM_R1, XMM_R1; \
	punpckhqdq XMM_R3, XMM_R3; \
	\
	UNPACK4_SSE(CL, TOTALCL, MaskType, ModeType); \
	add VIF_SRC, 16; \

#define UNPACK_V2_16SSE_4(CL, TOTALCL, MaskType, ModeType) \
	movdqu XMM_R0, xmmword ptr [VIF_SRC]; \
	\
	punpckhwd XMM_R2, XMM_R0; \
	punpcklwd XMM_R0, XMM_R0; \
	\
	UNPACK_RIGHTSHIFT XMM_R0, 16; \
	UNPACK_RIGHTSHIFT XMM_R2, 16; \
	\
	punpckhqdq XMM_R1, XMM_R0; \
	punpckhqdq XMM_R3, XMM_R2; \
	\
	punpcklqdq XMM_R0, XMM_R0; \
	punpcklqdq XMM_R2, XMM_R2; \
	punpckhqdq XMM_R1, XMM_R1; \
	punpckhqdq XMM_R3, XMM_R3; \
	\
	UNPACK4_SSE(CL, TOTALCL, MaskType, ModeType); \
	\
	add VIF_SRC, 16; \

#define UNPACK_V2_16SSE_3A(CL, TOTALCL, MaskType, ModeType) \
	punpcklwd XMM_R0, xmmword ptr [VIF_SRC]; \
	punpckhwd XMM_R2, xmmword ptr [VIF_SRC]; \
	\
	UNPACK_RIGHTSHIFT XMM_R0, 16; \
	UNPACK_RIGHTSHIFT XMM_R2, 16; \
	\
	punpckhqdq XMM_R1, XMM_R0; \
	\
	punpcklqdq XMM_R0, XMM_R0; \
	punpcklqdq XMM_R2, XMM_R2; \
	punpckhqdq XMM_R1, XMM_R1; \
	\
	UNPACK3_SSE(CL, TOTALCL, MaskType, ModeType); \
	\
	add VIF_SRC, 12; \

#define UNPACK_V2_16SSE_3(CL, TOTALCL, MaskType, ModeType) \
	movdqu XMM_R0, xmmword ptr [VIF_SRC]; \
	\
	punpckhwd XMM_R2, XMM_R0; \
	punpcklwd XMM_R0, XMM_R0; \
	\
	UNPACK_RIGHTSHIFT XMM_R0, 16; \
	UNPACK_RIGHTSHIFT XMM_R2, 16; \
	\
	punpckhqdq XMM_R1, XMM_R0; \
	\
	punpcklqdq XMM_R0, XMM_R0; \
	punpcklqdq XMM_R2, XMM_R2; \
	punpckhqdq XMM_R1, XMM_R1; \
	\
	UNPACK3_SSE(CL, TOTALCL, MaskType, ModeType); \
	\
	add VIF_SRC, 12; \

#define UNPACK_V2_16SSE_2A(CL, TOTALCL, MaskType, ModeType) \
	punpcklwd XMM_R0, xmmword ptr [VIF_SRC]; \
	UNPACK_RIGHTSHIFT XMM_R0, 16; \
	\
	punpckhqdq XMM_R1, XMM_R0; \
	\
	punpcklqdq XMM_R0, XMM_R0; \
	punpckhqdq XMM_R1, XMM_R1; \
	\
	UNPACK2_SSE(CL, TOTALCL, MaskType, ModeType); \
	\
	add VIF_SRC, 8; \

#define UNPACK_V2_16SSE_2(CL, TOTALCL, MaskType, ModeType) \
	movq XMM_R0, qword ptr [VIF_SRC]; \
	punpcklwd XMM_R0, XMM_R0; \
	UNPACK_RIGHTSHIFT XMM_R0, 16; \
	\
	punpckhqdq XMM_R1, XMM_R0; \
	\
	punpcklqdq XMM_R0, XMM_R0; \
	punpckhqdq XMM_R1, XMM_R1; \
	\
	UNPACK2_SSE(CL, TOTALCL, MaskType, ModeType); \
	\
	add VIF_SRC, 8; \

#define UNPACK_V2_16SSE_1A(CL, TOTALCL, MaskType, ModeType) \
	punpcklwd XMM_R0, xmmword ptr [VIF_SRC]; \
	UNPACK_RIGHTSHIFT XMM_R0, 16; \
	punpcklqdq XMM_R0, XMM_R0; \
	\
	UNPACK1_SSE(CL, TOTALCL, MaskType, ModeType); \
	\
	add VIF_SRC, 4; \

#define UNPACK_V2_16SSE_1(CL, TOTALCL, MaskType, ModeType) \
	movd XMM_R0, dword ptr [VIF_SRC]; \
	punpcklwd XMM_R0, XMM_R0; \
	UNPACK_RIGHTSHIFT XMM_R0, 16; \
	punpcklqdq XMM_R0, XMM_R0; \
	\
	UNPACK1_SSE(CL, TOTALCL, MaskType, ModeType); \
	\
	add VIF_SRC, 4; \

// V2-8
// and1 streetball needs to copy lower xmmword to the upper xmmword of every reg
#define UNPACK_V2_8SSE_4(CL, TOTALCL, MaskType, ModeType) \
	movq XMM_R0, qword ptr [VIF_SRC]; \
	\
	punpcklbw XMM_R0, XMM_R0; \
	punpckhwd XMM_R2, XMM_R0; \
	punpcklwd XMM_R0, XMM_R0; \
	\
	UNPACK_RIGHTSHIFT XMM_R0, 24; \
	UNPACK_RIGHTSHIFT XMM_R2, 24; \
	\
	punpckhqdq XMM_R1, XMM_R0; \
	punpckhqdq XMM_R3, XMM_R2; \
	\
	punpcklqdq XMM_R0, XMM_R0; \
	punpcklqdq XMM_R2, XMM_R2; \
	punpckhqdq XMM_R1, XMM_R1; \
	punpckhqdq XMM_R3, XMM_R3; \
	\
	UNPACK4_SSE(CL, TOTALCL, MaskType, ModeType); \
	\
	add VIF_SRC, 8; \

#define UNPACK_V2_8SSE_4A UNPACK_V2_8SSE_4

#define UNPACK_V2_8SSE_3(CL, TOTALCL, MaskType, ModeType) \
	movq XMM_R0, qword ptr [VIF_SRC]; \
	\
	punpcklbw XMM_R0, XMM_R0; \
	punpckhwd XMM_R2, XMM_R0; \
	punpcklwd XMM_R0, XMM_R0; \
	\
	UNPACK_RIGHTSHIFT XMM_R0, 24; \
	UNPACK_RIGHTSHIFT XMM_R2, 24; \
	\
	punpckhqdq XMM_R1, XMM_R0; \
	\
	punpcklqdq XMM_R0, XMM_R0; \
	punpcklqdq XMM_R2, XMM_R2; \
	punpckhqdq XMM_R1, XMM_R1; \
	\
	UNPACK3_SSE(CL, TOTALCL, MaskType, ModeType); \
	\
	add VIF_SRC, 6; \
	
#define UNPACK_V2_8SSE_3A UNPACK_V2_8SSE_3

#define UNPACK_V2_8SSE_2(CL, TOTALCL, MaskType, ModeType) \
	movd XMM_R0, dword ptr [VIF_SRC]; \
	punpcklbw XMM_R0, XMM_R0; \
	punpcklwd XMM_R0, XMM_R0; \
	UNPACK_RIGHTSHIFT XMM_R0, 24; \
	\
	punpckhqdq XMM_R1, XMM_R0; \
	\
	punpcklqdq XMM_R0, XMM_R0; \
	punpckhqdq XMM_R1, XMM_R1; \
	\
	UNPACK2_SSE(CL, TOTALCL, MaskType, ModeType); \
	\
	add VIF_SRC, 4; \

#define UNPACK_V2_8SSE_2A UNPACK_V2_8SSE_2

#define UNPACK_V2_8SSE_1(CL, TOTALCL, MaskType, ModeType) \
	movd XMM_R0, dword ptr [VIF_SRC]; \
	punpcklbw XMM_R0, XMM_R0; \
	punpcklwd XMM_R0, XMM_R0; \
	UNPACK_RIGHTSHIFT XMM_R0, 24; \
	punpcklqdq XMM_R0, XMM_R0; \
	\
	UNPACK1_SSE(CL, TOTALCL, MaskType, ModeType); \
	\
	add VIF_SRC, 2; \

#define UNPACK_V2_8SSE_1A UNPACK_V2_8SSE_1

// V3-32
// midnight club 2 crashes because reading a qw at +36 is out of bounds
#define UNPACK_V3_32SSE_4x(CL, TOTALCL, MaskType, ModeType, MOVDQA) \
	MOVDQA XMM_R0, xmmword ptr [VIF_SRC]; \
	movdqu XMM_R1, xmmword ptr [VIF_SRC+12]; \
	\
	UNPACK_Setup_##MaskType##_SSE_##ModeType##_##TOTALCL##(CL+0); \
	UNPACK_##MaskType##_SSE_##ModeType##(XMM_R0); \
	UNPACK_Write##TOTALCL##_##MaskType##(XMM_R0, CL, 0, movdqa); \
	\
	UNPACK_Setup_##MaskType##_SSE_##ModeType##_##TOTALCL##(CL+1); \
	UNPACK_##MaskType##_SSE_##ModeType##(XMM_R1); \
	UNPACK_Write##TOTALCL##_##MaskType##(XMM_R1, CL+1, 16, movdqa); \
	\
    MOVDQA XMM_R3, xmmword ptr [VIF_SRC+32]; \
	movdqu XMM_R2, xmmword ptr [VIF_SRC+24]; \
	psrldq XMM_R3, 4; \
	\
	UNPACK_Setup_##MaskType##_SSE_##ModeType##_##TOTALCL##(CL+2); \
	UNPACK_##MaskType##_SSE_##ModeType##(XMM_R2); \
	UNPACK_Write##TOTALCL##_##MaskType##(XMM_R2, CL+2, 32, movdqa); \
	\
	UNPACK_Setup_##MaskType##_SSE_##ModeType##_##TOTALCL##(CL+3); \
	UNPACK_##MaskType##_SSE_##ModeType##(XMM_R3); \
	UNPACK_Write##TOTALCL##_##MaskType##(XMM_R3, CL+3, 48, movdqa); \
	\
	UNPACK_INC_DST_##TOTALCL##_##MaskType##(4); \
	\
	add VIF_SRC, 48; \

#define UNPACK_V3_32SSE_4A(CL, TOTALCL, MaskType, ModeType) UNPACK_V3_32SSE_4x(CL, TOTALCL, MaskType, ModeType, movdqa)
#define UNPACK_V3_32SSE_4(CL, TOTALCL, MaskType, ModeType) UNPACK_V3_32SSE_4x(CL, TOTALCL, MaskType, ModeType, movdqu)

#define UNPACK_V3_32SSE_3x(CL, TOTALCL, MaskType, ModeType, MOVDQA) \
	MOVDQA XMM_R0, xmmword ptr [VIF_SRC]; \
	movdqu XMM_R1, xmmword ptr [VIF_SRC+12]; \
	\
	UNPACK_Setup_##MaskType##_SSE_##ModeType##_##TOTALCL##(CL); \
	UNPACK_##MaskType##_SSE_##ModeType##(XMM_R0); \
	UNPACK_Write##TOTALCL##_##MaskType##(XMM_R0, CL, 0, movdqa); \
	\
	UNPACK_Setup_##MaskType##_SSE_##ModeType##_##TOTALCL##(CL+1); \
	UNPACK_##MaskType##_SSE_##ModeType##(XMM_R1); \
	UNPACK_Write##TOTALCL##_##MaskType##(XMM_R1, CL+1, 16, movdqa); \
	\
	movdqu XMM_R2, xmmword ptr [VIF_SRC+24]; \
	\
	UNPACK_Setup_##MaskType##_SSE_##ModeType##_##TOTALCL##(CL+2); \
	UNPACK_##MaskType##_SSE_##ModeType##(XMM_R2); \
	UNPACK_Write##TOTALCL##_##MaskType##(XMM_R2, CL+2, 32, movdqa); \
	\
	UNPACK_INC_DST_##TOTALCL##_##MaskType##(3); \
	\
	add VIF_SRC, 36; \

#define UNPACK_V3_32SSE_3A(CL, TOTALCL, MaskType, ModeType) UNPACK_V3_32SSE_3x(CL, TOTALCL, MaskType, ModeType, movdqa)
#define UNPACK_V3_32SSE_3(CL, TOTALCL, MaskType, ModeType) UNPACK_V3_32SSE_3x(CL, TOTALCL, MaskType, ModeType, movdqu)

#define UNPACK_V3_32SSE_2x(CL, TOTALCL, MaskType, ModeType, MOVDQA) \
	MOVDQA XMM_R0, xmmword ptr [VIF_SRC]; \
	movdqu XMM_R1, xmmword ptr [VIF_SRC+12]; \
	\
	UNPACK2_SSE(CL, TOTALCL, MaskType, ModeType); \
	\
	add VIF_SRC, 24; \

#define UNPACK_V3_32SSE_2A(CL, TOTALCL, MaskType, ModeType) UNPACK_V3_32SSE_2x(CL, TOTALCL, MaskType, ModeType, movdqa)
#define UNPACK_V3_32SSE_2(CL, TOTALCL, MaskType, ModeType) UNPACK_V3_32SSE_2x(CL, TOTALCL, MaskType, ModeType, movdqu)

#define UNPACK_V3_32SSE_1x(CL, TOTALCL, MaskType, ModeType, MOVDQA) \
	MOVDQA XMM_R0, xmmword ptr [VIF_SRC]; \
	\
	UNPACK1_SSE(CL, TOTALCL, MaskType, ModeType); \
	\
	add VIF_SRC, 12; \

#define UNPACK_V3_32SSE_1A(CL, TOTALCL, MaskType, ModeType) UNPACK_V3_32SSE_1x(CL, TOTALCL, MaskType, ModeType, movdqa)
#define UNPACK_V3_32SSE_1(CL, TOTALCL, MaskType, ModeType) UNPACK_V3_32SSE_1x(CL, TOTALCL, MaskType, ModeType, movdqu)

// V3-16
#define UNPACK_V3_16SSE_4(CL, TOTALCL, MaskType, ModeType) \
	movq XMM_R0, qword ptr [VIF_SRC]; \
	movq XMM_R1, qword ptr [VIF_SRC+6]; \
	\
	punpcklwd XMM_R0, XMM_R0; \
	movq XMM_R2, qword ptr [VIF_SRC+12]; \
	punpcklwd XMM_R1, XMM_R1; \
	UNPACK_RIGHTSHIFT XMM_R0, 16; \
	movq XMM_R3, qword ptr [VIF_SRC+18]; \
	UNPACK_RIGHTSHIFT XMM_R1, 16; \
	punpcklwd XMM_R2, XMM_R2; \
	punpcklwd XMM_R3, XMM_R3; \
	\
	UNPACK_RIGHTSHIFT XMM_R2, 16; \
	UNPACK_RIGHTSHIFT XMM_R3, 16; \
	\
	UNPACK4_SSE(CL, TOTALCL, MaskType, ModeType); \
	\
	add VIF_SRC, 24; \

#define UNPACK_V3_16SSE_4A UNPACK_V3_16SSE_4

#define UNPACK_V3_16SSE_3(CL, TOTALCL, MaskType, ModeType) \
	movq XMM_R0, qword ptr [VIF_SRC]; \
	movq XMM_R1, qword ptr [VIF_SRC+6]; \
	\
	punpcklwd XMM_R0, XMM_R0; \
	movq XMM_R2, qword ptr [VIF_SRC+12]; \
	punpcklwd XMM_R1, XMM_R1; \
	UNPACK_RIGHTSHIFT XMM_R0, 16; \
	punpcklwd XMM_R2, XMM_R2; \
	\
	UNPACK_RIGHTSHIFT XMM_R1, 16; \
	UNPACK_RIGHTSHIFT XMM_R2, 16; \
	\
	UNPACK3_SSE(CL, TOTALCL, MaskType, ModeType); \
	\
	add VIF_SRC, 18; \

#define UNPACK_V3_16SSE_3A UNPACK_V3_16SSE_3

#define UNPACK_V3_16SSE_2(CL, TOTALCL, MaskType, ModeType) \
	movq XMM_R0, qword ptr [VIF_SRC]; \
	movq XMM_R1, qword ptr [VIF_SRC+6]; \
	\
	punpcklwd XMM_R0, XMM_R0; \
	punpcklwd XMM_R1, XMM_R1; \
	\
	UNPACK_RIGHTSHIFT XMM_R0, 16; \
	UNPACK_RIGHTSHIFT XMM_R1, 16; \
	\
	UNPACK2_SSE(CL, TOTALCL, MaskType, ModeType); \
	\
	add VIF_SRC, 12; \

#define UNPACK_V3_16SSE_2A UNPACK_V3_16SSE_2

#define UNPACK_V3_16SSE_1(CL, TOTALCL, MaskType, ModeType) \
	movq XMM_R0, qword ptr [VIF_SRC]; \
	punpcklwd XMM_R0, XMM_R0; \
	UNPACK_RIGHTSHIFT XMM_R0, 16; \
	\
	UNPACK1_SSE(CL, TOTALCL, MaskType, ModeType); \
	\
	add VIF_SRC, 6; \

#define UNPACK_V3_16SSE_1A UNPACK_V3_16SSE_1

// V3-8
#define UNPACK_V3_8SSE_4(CL, TOTALCL, MaskType, ModeType) \
	movq XMM_R1, qword ptr [VIF_SRC]; \
	movq XMM_R3, qword ptr [VIF_SRC+6]; \
	\
	punpcklbw XMM_R1, XMM_R1; \
	punpcklbw XMM_R3, XMM_R3; \
	punpcklwd XMM_R0, XMM_R1; \
	psrldq XMM_R1, 6; \
	punpcklwd XMM_R2, XMM_R3; \
	psrldq XMM_R3, 6; \
	punpcklwd XMM_R1, XMM_R1; \
	UNPACK_RIGHTSHIFT XMM_R0, 24; \
	punpcklwd XMM_R3, XMM_R3; \
	\
	UNPACK_RIGHTSHIFT XMM_R2, 24; \
	UNPACK_RIGHTSHIFT XMM_R1, 24; \
	UNPACK_RIGHTSHIFT XMM_R3, 24; \
	\
	UNPACK4_SSE(CL, TOTALCL, MaskType, ModeType); \
	\
	add VIF_SRC, 12; \

#define UNPACK_V3_8SSE_4A UNPACK_V3_8SSE_4

#define UNPACK_V3_8SSE_3(CL, TOTALCL, MaskType, ModeType) \
	movd XMM_R0, dword ptr [VIF_SRC]; \
	movd XMM_R1, dword ptr [VIF_SRC+3]; \
	\
	punpcklbw XMM_R0, XMM_R0; \
	movd XMM_R2, dword ptr [VIF_SRC+6]; \
	punpcklbw XMM_R1, XMM_R1; \
	punpcklwd XMM_R0, XMM_R0; \
	punpcklbw XMM_R2, XMM_R2; \
	\
	punpcklwd XMM_R1, XMM_R1; \
	punpcklwd XMM_R2, XMM_R2; \
	\
	UNPACK_RIGHTSHIFT XMM_R0, 24; \
	UNPACK_RIGHTSHIFT XMM_R1, 24; \
	UNPACK_RIGHTSHIFT XMM_R2, 24; \
	\
	UNPACK3_SSE(CL, TOTALCL, MaskType, ModeType); \
	\
	add VIF_SRC, 9 \

#define UNPACK_V3_8SSE_3A UNPACK_V3_8SSE_3

#define UNPACK_V3_8SSE_2(CL, TOTALCL, MaskType, ModeType) \
	movd XMM_R0, dword ptr [VIF_SRC]; \
	movd XMM_R1, dword ptr [VIF_SRC+3]; \
	\
	punpcklbw XMM_R0, XMM_R0; \
	punpcklbw XMM_R1, XMM_R1; \
	\
	punpcklwd XMM_R0, XMM_R0; \
	punpcklwd XMM_R1, XMM_R1; \
	\
	UNPACK_RIGHTSHIFT XMM_R0, 24; \
	UNPACK_RIGHTSHIFT XMM_R1, 24; \
	\
	UNPACK2_SSE(CL, TOTALCL, MaskType, ModeType); \
	\
	add VIF_SRC, 6; \

#define UNPACK_V3_8SSE_2A UNPACK_V3_8SSE_2

#define UNPACK_V3_8SSE_1(CL, TOTALCL, MaskType, ModeType) \
	movd XMM_R0, dword ptr [VIF_SRC]; \
	punpcklbw XMM_R0, XMM_R0; \
	punpcklwd XMM_R0, XMM_R0; \
	UNPACK_RIGHTSHIFT XMM_R0, 24; \
	\
	UNPACK1_SSE(CL, TOTALCL, MaskType, ModeType); \
	\
	add VIF_SRC, 3; \
	
#define UNPACK_V3_8SSE_1A UNPACK_V3_8SSE_1

// V4-32
#define UNPACK_V4_32SSE_4A(CL, TOTALCL, MaskType, ModeType) \
	movdqa XMM_R0, xmmword ptr [VIF_SRC]; \
	movdqa XMM_R1, xmmword ptr [VIF_SRC+16]; \
	movdqa XMM_R2, xmmword ptr [VIF_SRC+32]; \
	movdqa XMM_R3, xmmword ptr [VIF_SRC+48]; \
	\
	UNPACK4_SSE(CL, TOTALCL, MaskType, ModeType); \
	\
	add VIF_SRC, 64; \

#define UNPACK_V4_32SSE_4(CL, TOTALCL, MaskType, ModeType) \
	movdqu XMM_R0, xmmword ptr [VIF_SRC]; \
	movdqu XMM_R1, xmmword ptr [VIF_SRC+16]; \
	movdqu XMM_R2, xmmword ptr [VIF_SRC+32]; \
	movdqu XMM_R3, xmmword ptr [VIF_SRC+48]; \
	\
	UNPACK4_SSE(CL, TOTALCL, MaskType, ModeType); \
	\
	add VIF_SRC, 64; \

#define UNPACK_V4_32SSE_3A(CL, TOTALCL, MaskType, ModeType) \
	movdqa XMM_R0, xmmword ptr [VIF_SRC]; \
	movdqa XMM_R1, xmmword ptr [VIF_SRC+16]; \
	movdqa XMM_R2, xmmword ptr [VIF_SRC+32]; \
	\
	UNPACK3_SSE(CL, TOTALCL, MaskType, ModeType); \
	\
	add VIF_SRC, 48; \

#define UNPACK_V4_32SSE_3(CL, TOTALCL, MaskType, ModeType) \
	movdqu XMM_R0, xmmword ptr [VIF_SRC]; \
	movdqu XMM_R1, xmmword ptr [VIF_SRC+16]; \
	movdqu XMM_R2, xmmword ptr [VIF_SRC+32]; \
	\
	UNPACK3_SSE(CL, TOTALCL, MaskType, ModeType); \
	\
	add VIF_SRC, 48; \

#define UNPACK_V4_32SSE_2A(CL, TOTALCL, MaskType, ModeType) \
	movdqa XMM_R0, xmmword ptr [VIF_SRC]; \
	movdqa XMM_R1, xmmword ptr [VIF_SRC+16]; \
	\
	UNPACK2_SSE(CL, TOTALCL, MaskType, ModeType); \
	\
	add VIF_SRC, 32; \

#define UNPACK_V4_32SSE_2(CL, TOTALCL, MaskType, ModeType) \
	movdqu XMM_R0, xmmword ptr [VIF_SRC]; \
	movdqu XMM_R1, xmmword ptr [VIF_SRC+16]; \
	\
	UNPACK2_SSE(CL, TOTALCL, MaskType, ModeType); \
	\
	add VIF_SRC, 32; \

#define UNPACK_V4_32SSE_1A(CL, TOTALCL, MaskType, ModeType) \
	movdqa XMM_R0, xmmword ptr [VIF_SRC]; \
	\
	UNPACK1_SSE(CL, TOTALCL, MaskType, ModeType); \
	\
	add VIF_SRC, 16; \

#define UNPACK_V4_32SSE_1(CL, TOTALCL, MaskType, ModeType) \
	movdqu XMM_R0, xmmword ptr [VIF_SRC]; \
	\
	UNPACK1_SSE(CL, TOTALCL, MaskType, ModeType); \
	\
	add VIF_SRC, 16; \

// V4-16
#define UNPACK_V4_16SSE_4A(CL, TOTALCL, MaskType, ModeType) \
	\
	punpcklwd XMM_R0, xmmword ptr [VIF_SRC]; \
	punpckhwd XMM_R1, xmmword ptr [VIF_SRC]; \
	punpcklwd XMM_R2, xmmword ptr [VIF_SRC+16]; \
	punpckhwd XMM_R3, xmmword ptr [VIF_SRC+16]; \
	\
	UNPACK_RIGHTSHIFT XMM_R1, 16; \
	UNPACK_RIGHTSHIFT XMM_R3, 16; \
	UNPACK_RIGHTSHIFT XMM_R0, 16; \
	UNPACK_RIGHTSHIFT XMM_R2, 16; \
	\
	UNPACK4_SSE(CL, TOTALCL, MaskType, ModeType); \
	\
	add VIF_SRC, 32; \

#define UNPACK_V4_16SSE_4(CL, TOTALCL, MaskType, ModeType) \
	movdqu XMM_R0, xmmword ptr [VIF_SRC]; \
	movdqu XMM_R2, xmmword ptr [VIF_SRC+16]; \
	\
	punpckhwd XMM_R1, XMM_R0; \
	punpckhwd XMM_R3, XMM_R2; \
	punpcklwd XMM_R0, XMM_R0; \
	punpcklwd XMM_R2, XMM_R2; \
	\
	UNPACK_RIGHTSHIFT XMM_R1, 16; \
	UNPACK_RIGHTSHIFT XMM_R3, 16; \
	UNPACK_RIGHTSHIFT XMM_R0, 16; \
	UNPACK_RIGHTSHIFT XMM_R2, 16; \
	\
	UNPACK4_SSE(CL, TOTALCL, MaskType, ModeType); \
	\
	add VIF_SRC, 32; \

#define UNPACK_V4_16SSE_3A(CL, TOTALCL, MaskType, ModeType) \
	punpcklwd XMM_R0, xmmword ptr [VIF_SRC]; \
	punpckhwd XMM_R1, xmmword ptr [VIF_SRC]; \
	punpcklwd XMM_R2, xmmword ptr [VIF_SRC+16]; \
	\
	UNPACK_RIGHTSHIFT XMM_R0, 16; \
	UNPACK_RIGHTSHIFT XMM_R1, 16; \
	UNPACK_RIGHTSHIFT XMM_R2, 16; \
	\
	UNPACK3_SSE(CL, TOTALCL, MaskType, ModeType); \
	\
	add VIF_SRC, 24; \

#define UNPACK_V4_16SSE_3(CL, TOTALCL, MaskType, ModeType) \
	movdqu XMM_R0, xmmword ptr [VIF_SRC]; \
	movq XMM_R2, qword ptr [VIF_SRC+16]; \
	\
	punpckhwd XMM_R1, XMM_R0; \
	punpcklwd XMM_R0, XMM_R0; \
	punpcklwd XMM_R2, XMM_R2; \
	\
	UNPACK_RIGHTSHIFT XMM_R0, 16; \
	UNPACK_RIGHTSHIFT XMM_R1, 16; \
	UNPACK_RIGHTSHIFT XMM_R2, 16; \
	\
	UNPACK3_SSE(CL, TOTALCL, MaskType, ModeType); \
	\
	add VIF_SRC, 24; \

#define UNPACK_V4_16SSE_2A(CL, TOTALCL, MaskType, ModeType) \
	punpcklwd XMM_R0, xmmword ptr [VIF_SRC]; \
	punpckhwd XMM_R1, xmmword ptr [VIF_SRC]; \
	\
	UNPACK_RIGHTSHIFT XMM_R0, 16; \
	UNPACK_RIGHTSHIFT XMM_R1, 16; \
	\
	UNPACK2_SSE(CL, TOTALCL, MaskType, ModeType); \
	\
	add VIF_SRC, 16; \

#define UNPACK_V4_16SSE_2(CL, TOTALCL, MaskType, ModeType) \
	movq XMM_R0, qword ptr [VIF_SRC]; \
	movq XMM_R1, qword ptr [VIF_SRC+8]; \
	\
	punpcklwd XMM_R0, XMM_R0; \
	punpcklwd XMM_R1, XMM_R1; \
	\
	UNPACK_RIGHTSHIFT XMM_R0, 16; \
	UNPACK_RIGHTSHIFT XMM_R1, 16; \
	\
	UNPACK2_SSE(CL, TOTALCL, MaskType, ModeType); \
	\
	add VIF_SRC, 16; \

#define UNPACK_V4_16SSE_1A(CL, TOTALCL, MaskType, ModeType) \
	punpcklwd XMM_R0, xmmword ptr [VIF_SRC]; \
	UNPACK_RIGHTSHIFT XMM_R0, 16; \
	\
	UNPACK1_SSE(CL, TOTALCL, MaskType, ModeType); \
	\
	add VIF_SRC, 8; \

#define UNPACK_V4_16SSE_1(CL, TOTALCL, MaskType, ModeType) \
	movq XMM_R0, qword ptr [VIF_SRC]; \
	punpcklwd XMM_R0, XMM_R0; \
	UNPACK_RIGHTSHIFT XMM_R0, 16; \
	\
	UNPACK1_SSE(CL, TOTALCL, MaskType, ModeType); \
	\
	add VIF_SRC, 8; \
	
// V4-8
#define UNPACK_V4_8SSE_4A(CL, TOTALCL, MaskType, ModeType) \
	punpcklbw XMM_R0, xmmword ptr [VIF_SRC]; \
	punpckhbw XMM_R2, xmmword ptr [VIF_SRC]; \
	\
	punpckhwd XMM_R1, XMM_R0; \
	punpckhwd XMM_R3, XMM_R2; \
	punpcklwd XMM_R0, XMM_R0; \
	punpcklwd XMM_R2, XMM_R2; \
	\
	UNPACK_RIGHTSHIFT XMM_R1, 24; \
	UNPACK_RIGHTSHIFT XMM_R3, 24; \
	UNPACK_RIGHTSHIFT XMM_R0, 24; \
	UNPACK_RIGHTSHIFT XMM_R2, 24; \
	\
	UNPACK4_SSE(CL, TOTALCL, MaskType, ModeType); \
	\
	add VIF_SRC, 16; \

#define UNPACK_V4_8SSE_4(CL, TOTALCL, MaskType, ModeType) \
	movdqu XMM_R0, xmmword ptr [VIF_SRC]; \
	\
	punpckhbw XMM_R2, XMM_R0; \
	punpcklbw XMM_R0, XMM_R0; \
	\
	punpckhwd XMM_R3, XMM_R2; \
	punpckhwd XMM_R1, XMM_R0; \
	punpcklwd XMM_R2, XMM_R2; \
	punpcklwd XMM_R0, XMM_R0; \
	\
	UNPACK_RIGHTSHIFT XMM_R3, 24; \
	UNPACK_RIGHTSHIFT XMM_R2, 24; \
	\
	UNPACK_RIGHTSHIFT XMM_R0, 24; \
	UNPACK_RIGHTSHIFT XMM_R1, 24; \
	\
	UNPACK4_SSE(CL, TOTALCL, MaskType, ModeType); \
	\
	add VIF_SRC, 16; \

#define UNPACK_V4_8SSE_3A(CL, TOTALCL, MaskType, ModeType) \
	punpcklbw XMM_R0, xmmword ptr [VIF_SRC]; \
	punpckhbw XMM_R2, xmmword ptr [VIF_SRC]; \
	\
	punpckhwd XMM_R1, XMM_R0; \
	punpcklwd XMM_R0, XMM_R0; \
	punpcklwd XMM_R2, XMM_R2; \
	\
	UNPACK_RIGHTSHIFT XMM_R1, 24; \
	UNPACK_RIGHTSHIFT XMM_R0, 24; \
	UNPACK_RIGHTSHIFT XMM_R2, 24; \
	\
	UNPACK3_SSE(CL, TOTALCL, MaskType, ModeType); \
	\
	add VIF_SRC, 12; \

#define UNPACK_V4_8SSE_3(CL, TOTALCL, MaskType, ModeType) \
	movq XMM_R0, qword ptr [VIF_SRC]; \
	movd XMM_R2, dword ptr [VIF_SRC+8]; \
	\
	punpcklbw XMM_R0, XMM_R0; \
	punpcklbw XMM_R2, XMM_R2; \
	\
	punpckhwd XMM_R1, XMM_R0; \
	punpcklwd XMM_R2, XMM_R2; \
	punpcklwd XMM_R0, XMM_R0; \
	\
	UNPACK_RIGHTSHIFT XMM_R1, 24; \
	UNPACK_RIGHTSHIFT XMM_R0, 24; \
	UNPACK_RIGHTSHIFT XMM_R2, 24; \
	\
	UNPACK3_SSE(CL, TOTALCL, MaskType, ModeType); \
	\
	add VIF_SRC, 12; \

#define UNPACK_V4_8SSE_2A(CL, TOTALCL, MaskType, ModeType) \
	punpcklbw XMM_R0, xmmword ptr [VIF_SRC]; \
	\
	punpckhwd XMM_R1, XMM_R0; \
	punpcklwd XMM_R0, XMM_R0; \
	\
	UNPACK_RIGHTSHIFT XMM_R1, 24; \
	UNPACK_RIGHTSHIFT XMM_R0, 24; \
	\
	UNPACK2_SSE(CL, TOTALCL, MaskType, ModeType); \
	\
	add VIF_SRC, 8; \

#define UNPACK_V4_8SSE_2(CL, TOTALCL, MaskType, ModeType) \
	movq XMM_R0, qword ptr [VIF_SRC]; \
	\
	punpcklbw XMM_R0, XMM_R0; \
	\
	punpckhwd XMM_R1, XMM_R0; \
	punpcklwd XMM_R0, XMM_R0; \
	\
	UNPACK_RIGHTSHIFT XMM_R1, 24; \
	UNPACK_RIGHTSHIFT XMM_R0, 24; \
	\
	UNPACK2_SSE(CL, TOTALCL, MaskType, ModeType); \
	\
	add VIF_SRC, 8; \

#define UNPACK_V4_8SSE_1A(CL, TOTALCL, MaskType, ModeType) \
	punpcklbw XMM_R0, xmmword ptr [VIF_SRC]; \
	punpcklwd XMM_R0, XMM_R0; \
	UNPACK_RIGHTSHIFT XMM_R0, 24; \
	\
	UNPACK1_SSE(CL, TOTALCL, MaskType, ModeType); \
	\
	add VIF_SRC, 4; \

#define UNPACK_V4_8SSE_1(CL, TOTALCL, MaskType, ModeType) \
	movd XMM_R0, dword ptr [VIF_SRC]; \
	punpcklbw XMM_R0, XMM_R0; \
	punpcklwd XMM_R0, XMM_R0; \
	UNPACK_RIGHTSHIFT XMM_R0, 24; \
	\
	UNPACK1_SSE(CL, TOTALCL, MaskType, ModeType); \
	\
	add VIF_SRC, 4; \

// V4-5
.extern s_TempDecompress

#define DECOMPRESS_RGBA(OFFSET) \
	mov bl, al; \
	shl bl, 3; \
	mov byte ptr [s_TempDecompress+OFFSET], bl; \
	\
	mov bx, ax; \
	shr bx, 2; \
	and bx, 0xf8; \
	mov byte ptr [s_TempDecompress+OFFSET+1], bl; \
	\
	mov bx, ax; \
	shr bx, 7; \
	and bx, 0xf8; \
	mov byte ptr [s_TempDecompress+OFFSET+2], bl; \
	mov bx, ax; \
	shr bx, 8; \
	and bx, 0x80; \
	mov byte ptr [s_TempDecompress+OFFSET+3], bl; \

#define UNPACK_V4_5SSE_4(CL, TOTALCL, MaskType, ModeType) \
	mov eax, dword ptr [VIF_SRC]; \
	DECOMPRESS_RGBA(0); \
	\
	shr eax, 16; \
	DECOMPRESS_RGBA(4); \
	\
	mov eax, dword ptr [VIF_SRC+4]; \
	DECOMPRESS_RGBA(8); \
	\
	shr eax, 16; \
	DECOMPRESS_RGBA(12); \
	\
	movdqa XMM_R0, xmmword ptr [s_TempDecompress]; \
	\
	punpckhbw XMM_R2, XMM_R0; \
	punpcklbw XMM_R0, XMM_R0; \
	\
	punpckhwd XMM_R3, XMM_R2; \
	punpckhwd XMM_R1, XMM_R0; \
	punpcklwd XMM_R0, XMM_R0; \
	punpcklwd XMM_R2, XMM_R2; \
	\
	psrld XMM_R0, 24; \
	psrld XMM_R1, 24; \
	psrld XMM_R2, 24; \
	psrld XMM_R3, 24; \
	\
	UNPACK4_SSE(CL, TOTALCL, MaskType, ModeType); \
	\
	add VIF_SRC, 8; \

#define UNPACK_V4_5SSE_4A UNPACK_V4_5SSE_4

#define UNPACK_V4_5SSE_3(CL, TOTALCL, MaskType, ModeType) \
	mov eax, dword ptr [VIF_SRC]; \
	DECOMPRESS_RGBA(0); \
	\
	shr eax, 16; \
	DECOMPRESS_RGBA(4); \
	\
    mov eax, dword ptr [VIF_SRC]; \
	DECOMPRESS_RGBA(8); \
	\
	movdqa XMM_R0, xmmword ptr [s_TempDecompress]; \
	\
	punpckhbw XMM_R2, XMM_R0; \
	punpcklbw XMM_R0, XMM_R0; \
	\
	punpckhwd XMM_R1, XMM_R0; \
	punpcklwd XMM_R0, XMM_R0; \
	punpcklwd XMM_R2, XMM_R2; \
	\
	psrld XMM_R0, 24; \
	psrld XMM_R1, 24; \
	psrld XMM_R2, 24; \
	\
	UNPACK3_SSE(CL, TOTALCL, MaskType, ModeType); \
	\
	add VIF_SRC, 6; \

#define UNPACK_V4_5SSE_3A UNPACK_V4_5SSE_3

#define UNPACK_V4_5SSE_2(CL, TOTALCL, MaskType, ModeType) \
	mov eax, dword ptr [VIF_SRC]; \
	DECOMPRESS_RGBA(0); \
	\
	shr eax, 16; \
	DECOMPRESS_RGBA(4); \
	\
	movq XMM_R0, qword ptr [s_TempDecompress]; \
	\
	punpcklbw XMM_R0, XMM_R0; \
	\
	punpckhwd XMM_R1, XMM_R0; \
	punpcklwd XMM_R0, XMM_R0; \
	\
	psrld XMM_R0, 24; \
	psrld XMM_R1, 24; \
	\
	UNPACK2_SSE(CL, TOTALCL, MaskType, ModeType); \
	\
	add VIF_SRC, 4; \

#define UNPACK_V4_5SSE_2A UNPACK_V4_5SSE_2

#define UNPACK_V4_5SSE_1(CL, TOTALCL, MaskType, ModeType) \
	mov ax, word ptr [VIF_SRC]; \
	DECOMPRESS_RGBA(0) \
	\
	movd XMM_R0, dword ptr [s_TempDecompress]; \
	punpcklbw XMM_R0, XMM_R0; \
	punpcklwd XMM_R0, XMM_R0; \
	\
	psrld XMM_R0, 24; \
	\
	UNPACK1_SSE(CL, TOTALCL, MaskType, ModeType); \
	\
	add VIF_SRC, 2; \

#define UNPACK_V4_5SSE_1A UNPACK_V4_5SSE_1

#pragma warning(disable:4731)

#define SAVE_ROW_REG_BASE \
	mov VIF_TMPADDR, vifRow; \
	movdqa xmmword ptr [VIF_TMPADDR], XMM_ROW; \
	mov VIF_TMPADDR, vifRegs; \
	movss dword ptr [VIF_TMPADDR+0x100], XMM_ROW; \
	psrldq XMM_ROW, 4; \
	movss dword ptr [VIF_TMPADDR+0x110], XMM_ROW; \
	psrldq XMM_ROW, 4; \
	movss dword ptr [VIF_TMPADDR+0x120], XMM_ROW; \
	psrldq XMM_ROW, 4; \
	movss dword ptr [VIF_TMPADDR+0x130], XMM_ROW; \

#define SAVE_NO_REG

// 32 bit versions have the args on the stack
#define INIT_ARGS() \
    push edi; \
    push esi; \
    push ebx; \
    mov VIF_DST, dword ptr [esp+4+12]; \
    mov VIF_SRC, dword ptr [esp+8+12]; \
    mov VIF_SIZE, dword ptr [esp+12+12]; \


#define POP_REGS() \
    pop ebx; \
    pop esi; \
    pop edi; \

#define INC_STACK(reg) add esp, 4;
        
// qsize - bytes of compressed size of 1 decompressed xmmword
// int UNPACK_SkippingWrite_##name##_##sign##_##MaskType##_##ModeType(u32* dest, u32* data, int dmasize)

#define defUNPACK_SkippingWrite(name, MaskType, ModeType, qsize, sign, SAVE_ROW_REG) \
.globl UNPACK_SkippingWrite_##name##_##sign##_##MaskType##_##ModeType; \
UNPACK_SkippingWrite_##name##_##sign##_##MaskType##_##ModeType: \
    INIT_ARGS(); \
    mov VIF_TMPADDR, vifRegs; \
    movzx VIF_INC, byte ptr [VIF_TMPADDR + 0x40]; \
    movzx VIF_SAVEEBX, byte ptr [VIF_TMPADDR + 0x41]; \
    sub VIF_INC, VIF_SAVEEBX; \
    shl VIF_INC, 4; \
	\
    cmp VIF_SAVEEBXd, 1; \
    je name##_##sign##_##MaskType##_##ModeType##_WL1; \
    cmp VIF_SAVEEBXd, 2; \
    je name##_##sign##_##MaskType##_##ModeType##_WL2; \
    cmp VIF_SAVEEBXd, 3; \
    je name##_##sign##_##MaskType##_##ModeType##_WL3; \
    jmp name##_##sign##_##MaskType##_##ModeType##_WL4; \
    \
name##_##sign##_##MaskType##_##ModeType##_WL1: \
    UNPACK_Start_Setup_##MaskType##_SSE_##ModeType##(0); \
	\
	cmp VIF_SIZE, qsize; \
	jl name##_##sign##_##MaskType##_##ModeType##_C1_Done3; \
	\
	add VIF_INC, 16; \
	\
    /* first align VIF_SRC to 16 bytes */ \
name##_##sign##_##MaskType##_##ModeType##_C1_Align16: \
	\
	test VIF_SRC, 15; \
	jz name##_##sign##_##MaskType##_##ModeType##_C1_UnpackAligned; \
	\
	UNPACK_##name##SSE_1(0, 1, MaskType, ModeType); \
	\
	cmp VIF_SIZE, (2*qsize); \
	jl name##_##sign##_##MaskType##_##ModeType##_C1_DoneWithDec; \
	sub VIF_SIZE, qsize; \
	jmp name##_##sign##_##MaskType##_##ModeType##_C1_Align16; \
	\
name##_##sign##_##MaskType##_##ModeType##_C1_UnpackAligned: \
	\
	cmp VIF_SIZE, (2*qsize); \
	jl name##_##sign##_##MaskType##_##ModeType##_C1_Unpack1; \
	cmp VIF_SIZE, (3*qsize); \
	jl name##_##sign##_##MaskType##_##ModeType##_C1_Unpack2; \
	cmp VIF_SIZE, (4*qsize); \
	jl name##_##sign##_##MaskType##_##ModeType##_C1_Unpack3; \
	prefetchnta [VIF_SRC + 64]; \
	\
name##_##sign##_##MaskType##_##ModeType##_C1_Unpack4: \
	UNPACK_##name##SSE_4A(0, 1, MaskType, ModeType); \
	\
	cmp VIF_SIZE, (8*qsize); \
	jl name##_##sign##_##MaskType##_##ModeType##_C1_DoneUnpack4; \
	sub VIF_SIZE, (4*qsize); \
	jmp name##_##sign##_##MaskType##_##ModeType##_C1_Unpack4; \
	\
name##_##sign##_##MaskType##_##ModeType##_C1_DoneUnpack4: \
	\
	sub VIF_SIZE, (4*qsize); \
	cmp VIF_SIZE, qsize; \
	jl name##_##sign##_##MaskType##_##ModeType##_C1_Done3; \
	cmp VIF_SIZE, (2*qsize); \
	jl name##_##sign##_##MaskType##_##ModeType##_C1_Unpack1; \
	cmp VIF_SIZE, (3*qsize); \
	jl name##_##sign##_##MaskType##_##ModeType##_C1_Unpack2; \
    /* fall through */ \
	\
name##_##sign##_##MaskType##_##ModeType##_C1_Unpack3: \
	UNPACK_##name##SSE_3A(0, 1, MaskType, ModeType); \
	\
	sub VIF_SIZE, (3*qsize); \
	jmp name##_##sign##_##MaskType##_##ModeType##_C1_Done3; \
	\
name##_##sign##_##MaskType##_##ModeType##_C1_Unpack2: \
	UNPACK_##name##SSE_2A(0, 1, MaskType, ModeType); \
	\
	sub VIF_SIZE, (2*qsize); \
	jmp name##_##sign##_##MaskType##_##ModeType##_C1_Done3; \
	\
name##_##sign##_##MaskType##_##ModeType##_C1_Unpack1: \
	UNPACK_##name##SSE_1A(0, 1, MaskType, ModeType); \
name##_##sign##_##MaskType##_##ModeType##_C1_DoneWithDec: \
	sub VIF_SIZE, qsize; \
name##_##sign##_##MaskType##_##ModeType##_C1_Done3: \
	SAVE_ROW_REG; \
    mov eax, VIF_SIZE; \
    POP_REGS(); \
    ret; \
    \
name##_##sign##_##MaskType##_##ModeType##_WL2: \
	cmp VIF_SIZE, (2*qsize); \
	\
	jl name##_##sign##_##MaskType##_##ModeType##_C2_Done3; \
name##_##sign##_##MaskType##_##ModeType##_C2_Unpack: \
	UNPACK_##name##SSE_2(0, 0, MaskType, ModeType); \
	\
	add VIF_DST, VIF_INC; /* take into account wl */ \
	cmp VIF_SIZE, (4*qsize); \
	jl name##_##sign##_##MaskType##_##ModeType##_C2_Done2; \
	sub VIF_SIZE, (2*qsize); \
	jmp name##_##sign##_##MaskType##_##ModeType##_C2_Unpack; /* unpack next */ \
	\
name##_##sign##_##MaskType##_##ModeType##_C2_Done2: \
	sub VIF_SIZE, (2*qsize); \
name##_##sign##_##MaskType##_##ModeType##_C2_Done3: \
	cmp VIF_SIZE, qsize; \
    /* execute left over qw */ \
	jl name##_##sign##_##MaskType##_##ModeType##_C2_Done4; \
	UNPACK_##name##SSE_1(0, 0, MaskType, ModeType); \
	\
	sub VIF_SIZE, qsize; \
name##_##sign##_##MaskType##_##ModeType##_C2_Done4: \
	\
	SAVE_ROW_REG; \
    mov eax, VIF_SIZE; \
    POP_REGS(); \
	ret; \
	\
name##_##sign##_##MaskType##_##ModeType##_WL3: \
	cmp VIF_SIZE, (3*qsize); \
	\
	jl name##_##sign##_##MaskType##_##ModeType##_C3_Done5; \
name##_##sign##_##MaskType##_##ModeType##_C3_Unpack: \
	UNPACK_##name##SSE_3(0, 0, MaskType, ModeType); \
	\
	add VIF_DST, VIF_INC; /* take into account wl */ \
	cmp VIF_SIZE, (6*qsize); \
	jl name##_##sign##_##MaskType##_##ModeType##_C3_Done2; \
	sub VIF_SIZE, (3*qsize); \
	jmp name##_##sign##_##MaskType##_##ModeType##_C3_Unpack; /* unpack next */ \
name##_##sign##_##MaskType##_##ModeType##_C3_Done2: \
	sub VIF_SIZE, (3*qsize); \
name##_##sign##_##MaskType##_##ModeType##_C3_Done5: \
	cmp VIF_SIZE, qsize; \
	jl name##_##sign##_##MaskType##_##ModeType##_C3_Done4; \
	\
    /* execute left over qw */ \
	cmp VIF_SIZE, (2*qsize); \
	jl name##_##sign##_##MaskType##_##ModeType##_C3_Done3; \
	\
    /* process 2 qws */ \
	UNPACK_##name##SSE_2(0, 0, MaskType, ModeType); \
	\
	sub VIF_SIZE, (2*qsize); \
	jmp name##_##sign##_##MaskType##_##ModeType##_C3_Done4; \
name##_##sign##_##MaskType##_##ModeType##_C3_Done3: \
    /* process 1 qw */ \
	sub VIF_SIZE, qsize; \
	UNPACK_##name##SSE_1(0, 0, MaskType, ModeType); \
name##_##sign##_##MaskType##_##ModeType##_C3_Done4: \
	SAVE_ROW_REG; \
    mov eax, VIF_SIZE; \
    POP_REGS(); \
    ret; \
	\
name##_##sign##_##MaskType##_##ModeType##_WL4: /* >= 4 */ \
	sub VIF_SAVEEBX, 3; \
	push VIF_INC; \
	cmp VIF_SIZE, qsize; \
	jl name##_##sign##_##MaskType##_##ModeType##_C4_Done; \
	\
name##_##sign##_##MaskType##_##ModeType##_C4_Unpack: \
	cmp VIF_SIZE, (3*qsize); \
	jge name##_##sign##_##MaskType##_##ModeType##_C4_Unpack3; \
	cmp VIF_SIZE, (2*qsize); \
	jge name##_##sign##_##MaskType##_##ModeType##_C4_Unpack2; \
	\
	UNPACK_##name##SSE_1(0, 0, MaskType, ModeType) \
	\
    /* not enough data left */ \
	sub VIF_SIZE, qsize; \
	jmp name##_##sign##_##MaskType##_##ModeType##_C4_Done; \
name##_##sign##_##MaskType##_##ModeType##_C4_Unpack2: \
	UNPACK_##name##SSE_2(0, 0, MaskType, ModeType); \
	\
    /* not enough data left */ \
	sub VIF_SIZE, (2*qsize); \
	jmp name##_##sign##_##MaskType##_##ModeType##_C4_Done; \
name##_##sign##_##MaskType##_##ModeType##_C4_Unpack3: \
	UNPACK_##name##SSE_3(0, 0, MaskType, ModeType); \
	\
	sub VIF_SIZE, (3*qsize); \
    /* more data left, process 1qw at a time */ \
	mov VIF_INC, VIF_SAVEEBX; \
	\
name##_##sign##_##MaskType##_##ModeType##_C4_UnpackX: \
    /* check if any data left */ \
	cmp VIF_SIZE, qsize; \
	jl name##_##sign##_##MaskType##_##ModeType##_C4_Done; \
	\
	UNPACK_##name##SSE_1(3, 0, MaskType, ModeType); \
	\
	sub VIF_SIZE, qsize; \
	cmp VIF_INC, 1; \
	je name##_##sign##_##MaskType##_##ModeType##_C4_DoneLoop; \
	sub VIF_INC, 1; \
	jmp name##_##sign##_##MaskType##_##ModeType##_C4_UnpackX; \
name##_##sign##_##MaskType##_##ModeType##_C4_DoneLoop: \
	add VIF_DST, [VIF_ESP]; /* take into account wl */ \
	cmp VIF_SIZE, qsize; \
	jl name##_##sign##_##MaskType##_##ModeType##_C4_Done; \
	jmp name##_##sign##_##MaskType##_##ModeType##_C4_Unpack; /* unpack next */ \
name##_##sign##_##MaskType##_##ModeType##_C4_Done: \
	\
	SAVE_ROW_REG; \
	INC_STACK(); \
    mov eax, VIF_SIZE; \
    POP_REGS(); \
    ret; \
        
#define UNPACK_RIGHTSHIFT psrld
#define defUNPACK_SkippingWrite2(name, qsize) \
	defUNPACK_SkippingWrite(name, Regular, 0, qsize, u, SAVE_NO_REG) \
	defUNPACK_SkippingWrite(name, Regular, 1, qsize, u, SAVE_NO_REG) \
	defUNPACK_SkippingWrite(name, Regular, 2, qsize, u, SAVE_ROW_REG_BASE) \
	defUNPACK_SkippingWrite(name, Mask, 0, qsize, u, SAVE_NO_REG) \
	defUNPACK_SkippingWrite(name, Mask, 1, qsize, u, SAVE_NO_REG) \
	defUNPACK_SkippingWrite(name, Mask, 2, qsize, u, SAVE_ROW_REG_BASE) \
	defUNPACK_SkippingWrite(name, WriteMask, 0, qsize, u, SAVE_NO_REG) \
	defUNPACK_SkippingWrite(name, WriteMask, 1, qsize, u, SAVE_NO_REG) \
	defUNPACK_SkippingWrite(name, WriteMask, 2, qsize, u, SAVE_ROW_REG_BASE) \

defUNPACK_SkippingWrite2(S_32, 4)
defUNPACK_SkippingWrite2(S_16, 2)
defUNPACK_SkippingWrite2(S_8, 1)
defUNPACK_SkippingWrite2(V2_32, 8)
defUNPACK_SkippingWrite2(V2_16, 4)
defUNPACK_SkippingWrite2(V2_8, 2)
defUNPACK_SkippingWrite2(V3_32, 12)
defUNPACK_SkippingWrite2(V3_16, 6)
defUNPACK_SkippingWrite2(V3_8, 3)
defUNPACK_SkippingWrite2(V4_32, 16)
defUNPACK_SkippingWrite2(V4_16, 8)
defUNPACK_SkippingWrite2(V4_8, 4)
defUNPACK_SkippingWrite2(V4_5, 2)

#undef UNPACK_RIGHTSHIFT
#undef defUNPACK_SkippingWrite2

#define UNPACK_RIGHTSHIFT psrad
#define defUNPACK_SkippingWrite2(name, qsize) \
	defUNPACK_SkippingWrite(name, Mask, 0, qsize, s, SAVE_NO_REG) \
	defUNPACK_SkippingWrite(name, Regular, 0, qsize, s, SAVE_NO_REG) \
	defUNPACK_SkippingWrite(name, Regular, 1, qsize, s, SAVE_NO_REG) \
	defUNPACK_SkippingWrite(name, Regular, 2, qsize, s, SAVE_ROW_REG_BASE) \
	defUNPACK_SkippingWrite(name, Mask, 1, qsize, s, SAVE_NO_REG) \
	defUNPACK_SkippingWrite(name, Mask, 2, qsize, s, SAVE_ROW_REG_BASE) \
	defUNPACK_SkippingWrite(name, WriteMask, 0, qsize, s, SAVE_NO_REG) \
	defUNPACK_SkippingWrite(name, WriteMask, 1, qsize, s, SAVE_NO_REG) \
	defUNPACK_SkippingWrite(name, WriteMask, 2, qsize, s, SAVE_ROW_REG_BASE) \

defUNPACK_SkippingWrite2(S_16, 2)
defUNPACK_SkippingWrite2(S_8, 1)
defUNPACK_SkippingWrite2(V2_16, 4)
defUNPACK_SkippingWrite2(V2_8, 2)
defUNPACK_SkippingWrite2(V3_16, 6)
defUNPACK_SkippingWrite2(V3_8, 3)
defUNPACK_SkippingWrite2(V4_16, 8)
defUNPACK_SkippingWrite2(V4_8, 4)

#undef UNPACK_RIGHTSHIFT
#undef defUNPACK_SkippingWrite2
