/*  Pcsx2 - Pc Ps2 Emulator
 *  Copyright (C) 2002-2009  Pcsx2 Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *  
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *  
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include "PrecompiledHeader.h"
#include "ix86_legacy_internal.h"
#include "ix86_sse_helpers.h"

using namespace x86Emitter;


//------------------------------------------------------------------
// SSE instructions
//------------------------------------------------------------------

#define SSEMtoR( code, overb ) \
	assert( to < iREGCNT_XMM ), \
	RexR(0, to),             \
	write16( code ), \
	ModRM( 0, to, DISP32 ), \
	write32( MEMADDR(from, 4 + overb) )

#define SSERtoR( code ) \
	assert( to < iREGCNT_XMM && from < iREGCNT_XMM), \
    RexRB(0, to, from),            \
	write16( code ), \
	ModRM( 3, to, from )

#define SSEMtoR66( code ) \
	write8( 0x66 ), \
	SSEMtoR( code, 0 )

#define SSERtoM66( code ) \
	write8( 0x66 ), \
	SSERtoM( code, 0 )

#define SSERtoR66( code ) \
	write8( 0x66 ), \
	SSERtoR( code )

#define _SSERtoR66( code ) \
	assert( to < iREGCNT_XMM && from < iREGCNT_XMM), \
	write8( 0x66 ), \
	RexRB(0, from, to), \
	write16( code ), \
	ModRM( 3, from, to )

#define SSE_SS_RtoR( code ) \
	assert( to < iREGCNT_XMM && from < iREGCNT_XMM), \
	write8( 0xf3 ), \
    RexRB(0, to, from),              \
	write16( code ), \
	ModRM( 3, to, from )

#define SSE_SD_MtoR( code, overb ) \
	assert( to < iREGCNT_XMM ) , \
	write8( 0xf2 ), \
    RexR(0, to),                      \
	write16( code ), \
	ModRM( 0, to, DISP32 ), \
	write32( MEMADDR(from, 4 + overb) )

#define DEFINE_LEGACY_MOV_OPCODE( mod, sse ) \
	emitterT void sse##_MOV##mod##_M128_to_XMM( x86SSERegType to, uptr from )	{ xMOV##mod( xRegisterSSE(to), (void*)from ); } \
	emitterT void sse##_MOV##mod##_XMM_to_M128( uptr to, x86SSERegType from )	{ xMOV##mod( (void*)to, xRegisterSSE(from) ); } \
	emitterT void sse##_MOV##mod##RmtoR( x86SSERegType to, x86IntRegType from, int offset )	{ xMOV##mod( xRegisterSSE(to), ptr[xAddressReg(from)+offset] ); } \
	emitterT void sse##_MOV##mod##RtoRm( x86IntRegType to, x86SSERegType from, int offset )	{ xMOV##mod( ptr[xAddressReg(to)+offset], xRegisterSSE(from) ); } \
	emitterT void sse##_MOV##mod##RmStoR( x86SSERegType to, x86IntRegType from, x86IntRegType from2, int scale ) \
	{ xMOV##mod( xRegisterSSE(to), ptr[xAddressReg(from)+xAddressReg(from2)] ); } \
	emitterT void sse##_MOV##mod##RtoRmS( x86IntRegType to, x86SSERegType from, x86IntRegType from2, int scale ) \
	{ xMOV##mod( ptr[xAddressReg(to)+xAddressReg(from2)], xRegisterSSE(from) ); }

#define DEFINE_LEGACY_PSD_OPCODE( mod ) \
	emitterT void SSE_##mod##PS_M128_to_XMM( x86SSERegType to, uptr from )			{ x##mod.PS( xRegisterSSE(to), (void*)from ); } \
	emitterT void SSE_##mod##PS_XMM_to_XMM( x86SSERegType to, x86SSERegType from )	{ x##mod.PS( xRegisterSSE(to), xRegisterSSE(from) ); } \
	emitterT void SSE2_##mod##PD_M128_to_XMM( x86SSERegType to, uptr from )			{ x##mod.PD( xRegisterSSE(to), (void*)from ); } \
	emitterT void SSE2_##mod##PD_XMM_to_XMM( x86SSERegType to, x86SSERegType from )	{ x##mod.PD( xRegisterSSE(to), xRegisterSSE(from) ); }

#define DEFINE_LEGACY_SSSD_OPCODE( mod ) \
	emitterT void SSE_##mod##SS_M32_to_XMM( x86SSERegType to, uptr from )			{ x##mod.SS( xRegisterSSE(to), (void*)from ); } \
	emitterT void SSE_##mod##SS_XMM_to_XMM( x86SSERegType to, x86SSERegType from )	{ x##mod.SS( xRegisterSSE(to), xRegisterSSE(from) ); } \
	emitterT void SSE2_##mod##SD_M64_to_XMM( x86SSERegType to, uptr from )			{ x##mod.SD( xRegisterSSE(to), (void*)from ); } \
	emitterT void SSE2_##mod##SD_XMM_to_XMM( x86SSERegType to, x86SSERegType from )	{ x##mod.SD( xRegisterSSE(to), xRegisterSSE(from) ); }

#define DEFINE_LEGACY_CMP_OPCODE( comp ) \
	emitterT void SSE_CMP##comp##PS_M128_to_XMM( x86SSERegType to, uptr from )         { xCMP##comp.PS( xRegisterSSE(to), (void*)from ); } \
	emitterT void SSE_CMP##comp##PS_XMM_to_XMM( x86SSERegType to, x86SSERegType from ) { xCMP##comp.PS( xRegisterSSE(to), xRegisterSSE(from) ); } \
	emitterT void SSE2_CMP##comp##PD_M128_to_XMM( x86SSERegType to, uptr from )         { xCMP##comp.PD( xRegisterSSE(to), (void*)from ); } \
	emitterT void SSE2_CMP##comp##PD_XMM_to_XMM( x86SSERegType to, x86SSERegType from ) { xCMP##comp.PD( xRegisterSSE(to), xRegisterSSE(from) ); } \
	emitterT void SSE_CMP##comp##SS_M32_to_XMM( x86SSERegType to, uptr from )         { xCMP##comp.SS( xRegisterSSE(to), (void*)from ); } \
	emitterT void SSE_CMP##comp##SS_XMM_to_XMM( x86SSERegType to, x86SSERegType from ) { xCMP##comp.SS( xRegisterSSE(to), xRegisterSSE(from) ); } \
	emitterT void SSE2_CMP##comp##SD_M64_to_XMM( x86SSERegType to, uptr from )         { xCMP##comp.SD( xRegisterSSE(to), (void*)from ); } \
	emitterT void SSE2_CMP##comp##SD_XMM_to_XMM( x86SSERegType to, x86SSERegType from ) { xCMP##comp.SD( xRegisterSSE(to), xRegisterSSE(from) ); }

#define DEFINE_LEGACY_RSQRT_OPCODE(mod) \
	emitterT void SSE_##mod##PS_M128_to_XMM( x86SSERegType to, uptr from )			{ x##mod.PS( xRegisterSSE(to), (void*)from ); } \
	emitterT void SSE_##mod##PS_XMM_to_XMM( x86SSERegType to, x86SSERegType from )	{ x##mod.PS( xRegisterSSE(to), xRegisterSSE(from) ); } \
	emitterT void SSE_##mod##SS_M32_to_XMM( x86SSERegType to, uptr from )			{ x##mod.SS( xRegisterSSE(to), (void*)from ); } \
	emitterT void SSE_##mod##SS_XMM_to_XMM( x86SSERegType to, x86SSERegType from )	{ x##mod.SS( xRegisterSSE(to), xRegisterSSE(from) ); }

#define DEFINE_LEGACY_SQRT_OPCODE(mod) \
	DEFINE_LEGACY_RSQRT_OPCODE(mod) \
	emitterT void SSE2_##mod##SD_M64_to_XMM( x86SSERegType to, uptr from )			{ x##mod.SD( xRegisterSSE(to), (void*)from ); } \
	emitterT void SSE2_##mod##SD_XMM_to_XMM( x86SSERegType to, x86SSERegType from )	{ x##mod.SD( xRegisterSSE(to), xRegisterSSE(from) ); }

#define DEFINE_LEGACY_PSSD_OPCODE( mod ) \
	DEFINE_LEGACY_PSD_OPCODE( mod ) \
	DEFINE_LEGACY_SSSD_OPCODE( mod )

DEFINE_LEGACY_MOV_OPCODE( UPS, SSE )
DEFINE_LEGACY_MOV_OPCODE( APS, SSE )
DEFINE_LEGACY_MOV_OPCODE( DQA, SSE2 )
DEFINE_LEGACY_MOV_OPCODE( DQU, SSE2 )

DEFINE_LEGACY_PSD_OPCODE( AND )
DEFINE_LEGACY_PSD_OPCODE( ANDN )
DEFINE_LEGACY_PSD_OPCODE( OR )
DEFINE_LEGACY_PSD_OPCODE( XOR )

DEFINE_LEGACY_PSSD_OPCODE( SUB )
DEFINE_LEGACY_PSSD_OPCODE( ADD )
DEFINE_LEGACY_PSSD_OPCODE( MUL )
DEFINE_LEGACY_PSSD_OPCODE( DIV )

DEFINE_LEGACY_PSSD_OPCODE( MIN )
DEFINE_LEGACY_PSSD_OPCODE( MAX )

DEFINE_LEGACY_CMP_OPCODE( EQ )
DEFINE_LEGACY_CMP_OPCODE( LT )
DEFINE_LEGACY_CMP_OPCODE( LE )
DEFINE_LEGACY_CMP_OPCODE( UNORD )
DEFINE_LEGACY_CMP_OPCODE( NE )
DEFINE_LEGACY_CMP_OPCODE( NLT )
DEFINE_LEGACY_CMP_OPCODE( NLE )
DEFINE_LEGACY_CMP_OPCODE( ORD )

DEFINE_LEGACY_SSSD_OPCODE( UCOMI )
DEFINE_LEGACY_RSQRT_OPCODE( RCP )
DEFINE_LEGACY_RSQRT_OPCODE( RSQRT )
DEFINE_LEGACY_SQRT_OPCODE( SQRT )


emitterT void SSE_MOVAPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from )	{ xMOVAPS( xRegisterSSE(to), xRegisterSSE(from) ); }
emitterT void SSE2_MOVDQA_XMM_to_XMM( x86SSERegType to, x86SSERegType from)	{ xMOVDQA( xRegisterSSE(to), xRegisterSSE(from) ); }

emitterT void SSE2_MOVD_M32_to_XMM( x86SSERegType to, uptr from )			{ xMOVDZX( xRegisterSSE(to), (void*)from ); }
emitterT void SSE2_MOVD_R_to_XMM( x86SSERegType to, x86IntRegType from )	{ xMOVDZX( xRegisterSSE(to), xRegister32(from) ); }
emitterT void SSE2_MOVD_Rm_to_XMM( x86SSERegType to, x86IntRegType from, int offset )
{
	xMOVDZX( xRegisterSSE(to), ptr[xAddressReg(from)+offset] );
}

emitterT void SSE2_MOVD_XMM_to_M32( u32 to, x86SSERegType from )			{ xMOVD( (void*)to, xRegisterSSE(from) ); }
emitterT void SSE2_MOVD_XMM_to_R( x86IntRegType to, x86SSERegType from )	{ xMOVD( xRegister32(to), xRegisterSSE(from) ); }
emitterT void SSE2_MOVD_XMM_to_Rm( x86IntRegType to, x86SSERegType from, int offset )
{
	xMOVD( ptr[xAddressReg(from)+offset], xRegisterSSE(from) );
}

emitterT void SSE2_MOVQ_M64_to_XMM( x86SSERegType to, uptr from )			{ xMOVQZX( xRegisterSSE(to), (void*)from ); }
emitterT void SSE2_MOVQ_XMM_to_XMM( x86SSERegType to, x86SSERegType from )	{ xMOVQZX( xRegisterSSE(to), xRegisterSSE(from) ); }
emitterT void SSE2_MOVQ_XMM_to_M64( u32 to, x86SSERegType from )			{ xMOVQ( (void*)to, xRegisterSSE(from) ); }
emitterT void SSE2_MOVDQ2Q_XMM_to_MM( x86MMXRegType to, x86SSERegType from)	{ xMOVQ( xRegisterMMX(to), xRegisterSSE(from) ); }
emitterT void SSE2_MOVQ2DQ_MM_to_XMM( x86SSERegType to, x86MMXRegType from)	{ xMOVQ( xRegisterSSE(to), xRegisterMMX(from) ); }


emitterT void SSE_MOVSS_M32_to_XMM( x86SSERegType to, uptr from )						{ xMOVSSZX( xRegisterSSE(to), (void*)from ); }
emitterT void SSE_MOVSS_XMM_to_M32( u32 to, x86SSERegType from )						{ xMOVSS( (void*)to, xRegisterSSE(from) ); }
emitterT void SSE_MOVSS_XMM_to_XMM( x86SSERegType to, x86SSERegType from )				{ xMOVSS( xRegisterSSE(to), xRegisterSSE(from) ); }
emitterT void SSE_MOVSS_Rm_to_XMM( x86SSERegType to, x86IntRegType from, int offset )	{ xMOVSSZX( xRegisterSSE(to), ptr[xAddressReg(from)+offset] ); }
emitterT void SSE_MOVSS_XMM_to_Rm( x86IntRegType to, x86SSERegType from, int offset )	{ xMOVSS( ptr[xAddressReg(to)+offset], xRegisterSSE(from) ); }

emitterT void SSE2_MOVSD_M32_to_XMM( x86SSERegType to, uptr from )						{ xMOVSDZX( xRegisterSSE(to), (void*)from ); }
emitterT void SSE2_MOVSD_XMM_to_M32( u32 to, x86SSERegType from )						{ xMOVSD( (void*)to, xRegisterSSE(from) ); }
emitterT void SSE2_MOVSD_XMM_to_XMM( x86SSERegType to, x86SSERegType from )				{ xMOVSD( xRegisterSSE(to), xRegisterSSE(from) ); }
emitterT void SSE2_MOVSD_Rm_to_XMM( x86SSERegType to, x86IntRegType from, int offset )	{ xMOVSDZX( xRegisterSSE(to), ptr[xAddressReg(from)+offset] ); }
emitterT void SSE2_MOVSD_XMM_to_Rm( x86IntRegType to, x86SSERegType from, int offset )	{ xMOVSD( ptr[xAddressReg(to)+offset], xRegisterSSE(from) ); }

emitterT void SSE_MOVLPS_M64_to_XMM( x86SSERegType to, uptr from )						{ xMOVL.PS( xRegisterSSE(to), (void*)from ); }
emitterT void SSE_MOVLPS_XMM_to_M64( u32 to, x86SSERegType from )						{ xMOVL.PS( (void*)to, xRegisterSSE(from) ); }
emitterT void SSE_MOVLPS_Rm_to_XMM( x86SSERegType to, x86IntRegType from, int offset )	{ xMOVL.PS( xRegisterSSE(to), ptr[xAddressReg(from)+offset] ); }
emitterT void SSE_MOVLPS_XMM_to_Rm( x86IntRegType to, x86SSERegType from, int offset )	{ xMOVL.PS( ptr[xAddressReg(to)+offset], xRegisterSSE(from) ); }

emitterT void SSE_MOVHPS_M64_to_XMM( x86SSERegType to, uptr from )						{ xMOVH.PS( xRegisterSSE(to), (void*)from ); }
emitterT void SSE_MOVHPS_XMM_to_M64( u32 to, x86SSERegType from )						{ xMOVH.PS( (void*)to, xRegisterSSE(from) ); }
emitterT void SSE_MOVHPS_Rm_to_XMM( x86SSERegType to, x86IntRegType from, int offset )	{ xMOVH.PS( xRegisterSSE(to), ptr[xAddressReg(from)+offset] ); }
emitterT void SSE_MOVHPS_XMM_to_Rm( x86IntRegType to, x86SSERegType from, int offset )	{ xMOVH.PS( ptr[xAddressReg(to)+offset], xRegisterSSE(from) ); }

emitterT void SSE_MOVLHPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from )	{ xMOVLH.PS( xRegisterSSE(to), xRegisterSSE(from) ); }
emitterT void SSE_MOVHLPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from )	{ xMOVHL.PS( xRegisterSSE(to), xRegisterSSE(from) ); }

emitterT void SSE_MASKMOVDQU_XMM_to_XMM( x86SSERegType to, x86SSERegType from )	{ xMASKMOV( xRegisterSSE(to), xRegisterSSE(from) ); }
emitterT void SSE2_PMOVMSKB_XMM_to_R32(x86IntRegType to, x86SSERegType from)	{ xPMOVMSKB( xRegister32(to), xRegisterSSE(from) ); }

emitterT void SSE_SHUFPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from, u8 imm8 )	{ xSHUF.PS( xRegisterSSE(to), xRegisterSSE(from), imm8 ); }
emitterT void SSE_SHUFPS_M128_to_XMM( x86SSERegType to, uptr from, u8 imm8 )			{ xSHUF.PS( xRegisterSSE(to), (void*)from, imm8 ); }
emitterT void SSE_SHUFPS_Rm_to_XMM( x86SSERegType to, x86IntRegType from, int offset, u8 imm8 )
{
	xSHUF.PS( xRegisterSSE(to), ptr[xAddressReg(from)+offset], imm8 );
}

emitterT void SSE_SHUFPD_XMM_to_XMM( x86SSERegType to, x86SSERegType from, u8 imm8 )	{ xSHUF.PD( xRegisterSSE(to), xRegisterSSE(from), imm8 ); }
emitterT void SSE_SHUFPD_M128_to_XMM( x86SSERegType to, uptr from, u8 imm8 )			{ xSHUF.PD( xRegisterSSE(to), (void*)from, imm8 ); }


emitterT void SSE_CVTPI2PS_M64_to_XMM( x86SSERegType to, uptr from )			{ xCVTPI2PS( xRegisterSSE(to), (u64*)from ); }
emitterT void SSE_CVTPI2PS_MM_to_XMM( x86SSERegType to, x86MMXRegType from )	{ xCVTPI2PS( xRegisterSSE(to), xRegisterMMX(from) ); }

emitterT void SSE_CVTPS2PI_M64_to_MM( x86MMXRegType to, uptr from )				{ xCVTPS2PI( xRegisterMMX(to), (u64*)from ); }
emitterT void SSE_CVTPS2PI_XMM_to_MM( x86MMXRegType to, x86SSERegType from )	{ xCVTPS2PI( xRegisterMMX(to), xRegisterSSE(from) ); }

emitterT void SSE_CVTTSS2SI_M32_to_R32(x86IntRegType to, uptr from)				{ xCVTTSS2SI( xRegister32(to), (u32*)from ); }
emitterT void SSE_CVTTSS2SI_XMM_to_R32(x86IntRegType to, x86SSERegType from)	{ xCVTTSS2SI( xRegister32(to), xRegisterSSE(from) ); }

emitterT void SSE_CVTSI2SS_M32_to_XMM(x86SSERegType to, uptr from)				{ xCVTSI2SS( xRegisterSSE(to), (u32*)from ); }
emitterT void SSE_CVTSI2SS_R_to_XMM(x86SSERegType to, x86IntRegType from)		{ xCVTSI2SS( xRegisterSSE(to), xRegister32(from) ); }

emitterT void SSE2_CVTSS2SD_M32_to_XMM( x86SSERegType to, uptr from)			{ xCVTSS2SD( xRegisterSSE(to), (u32*)from ); }
emitterT void SSE2_CVTSS2SD_XMM_to_XMM( x86SSERegType to, x86SSERegType from)	{ xCVTSS2SD( xRegisterSSE(to), xRegisterSSE(from) ); }

emitterT void SSE2_CVTSD2SS_M64_to_XMM( x86SSERegType to, uptr from)			{ xCVTSD2SS( xRegisterSSE(to), (u64*)from ); }
emitterT void SSE2_CVTSD2SS_XMM_to_XMM( x86SSERegType to, x86SSERegType from)	{ xCVTSD2SS( xRegisterSSE(to), xRegisterSSE(from) ); }

emitterT void SSE2_CVTDQ2PS_M128_to_XMM( x86SSERegType to, uptr from )			{ xCVTDQ2PS( xRegisterSSE(to), (u128*)from ); }
emitterT void SSE2_CVTDQ2PS_XMM_to_XMM( x86SSERegType to, x86SSERegType from )	{ xCVTDQ2PS( xRegisterSSE(to), xRegisterSSE(from) ); }

emitterT void SSE2_CVTPS2DQ_M128_to_XMM( x86SSERegType to, uptr from )			{ xCVTPS2DQ( xRegisterSSE(to), (u128*)from ); }
emitterT void SSE2_CVTPS2DQ_XMM_to_XMM( x86SSERegType to, x86SSERegType from )	{ xCVTPS2DQ( xRegisterSSE(to), xRegisterSSE(from) ); }

emitterT void SSE2_CVTTPS2DQ_XMM_to_XMM( x86SSERegType to, x86SSERegType from )	{ xCVTTPS2DQ( xRegisterSSE(to), xRegisterSSE(from) ); }

//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////////////////
//**********************************************************************************/
//PMAXSW: Packed Signed Integer Word Maximum                                        *
//**********************************************************************************
//missing
//     SSE_PMAXSW_M64_to_MM
//		SSE2_PMAXSW_M128_to_XMM
//		SSE2_PMAXSW_XMM_to_XMM
emitterT void SSE_PMAXSW_MM_to_MM( x86MMXRegType to, x86MMXRegType from ){ SSERtoR( 0xEE0F ); }

///////////////////////////////////////////////////////////////////////////////////////
//**********************************************************************************/
//PMINSW: Packed Signed Integer Word Minimum                                        *
//**********************************************************************************
//missing
//     SSE_PMINSW_M64_to_MM
//		SSE2_PMINSW_M128_to_XMM
//		SSE2_PMINSW_XMM_to_XMM
emitterT void SSE_PMINSW_MM_to_MM( x86MMXRegType to, x86MMXRegType from ){ SSERtoR( 0xEA0F ); }

////////////////////////////////////////////////////////////////////////////////////
//**********************************************************************************/
//PSHUFD: Shuffle Packed DoubleWords                                               *
//**********************************************************************************
emitterT void SSE2_PSHUFD_XMM_to_XMM( x86SSERegType to, x86SSERegType from, u8 imm8 )
{
	SSERtoR66( 0x700F );
	write8( imm8 );
}
emitterT void SSE2_PSHUFD_M128_to_XMM( x86SSERegType to, uptr from, u8 imm8 )	{ SSEMtoR66( 0x700F ); write8( imm8 ); }

emitterT void SSE2_PSHUFLW_XMM_to_XMM( x86SSERegType to, x86SSERegType from, u8 imm8 ) { write8(0xF2); SSERtoR(0x700F); write8(imm8); }
emitterT void SSE2_PSHUFLW_M128_to_XMM( x86SSERegType to, uptr from, u8 imm8 ) { write8(0xF2); SSEMtoR(0x700F, 1); write8(imm8); }
emitterT void SSE2_PSHUFHW_XMM_to_XMM( x86SSERegType to, x86SSERegType from, u8 imm8 ) { write8(0xF3); SSERtoR(0x700F); write8(imm8); }
emitterT void SSE2_PSHUFHW_M128_to_XMM( x86SSERegType to, uptr from, u8 imm8 ) { write8(0xF3); SSEMtoR(0x700F, 1); write8(imm8); }

///////////////////////////////////////////////////////////////////////////////////
//**********************************************************************************/
//UNPCKLPS: Unpack and Interleave low Packed Single-Precision FP Data              *
//**********************************************************************************
emitterT void SSE_UNPCKLPS_M128_to_XMM( x86SSERegType to, uptr from )			{ SSEMtoR(0x140f, 0); }
emitterT void SSE_UNPCKLPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from )	{ SSERtoR( 0x140F ); }

////////////////////////////////////////////////////////////////////////////////////////
//**********************************************************************************/
//UNPCKHPS: Unpack and Interleave High Packed Single-Precision FP Data              *
//**********************************************************************************
emitterT void SSE_UNPCKHPS_M128_to_XMM( x86SSERegType to, uptr from )			{ SSEMtoR(0x150f, 0); }
emitterT void SSE_UNPCKHPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from )	{ SSERtoR( 0x150F ); }

/////////////////////////////////////////////////////////////////////////////////////////
//**********************************************************************************/
//STMXCSR : Store Streaming SIMD Extension Control/Status                         *
//**********************************************************************************
emitterT void SSE_STMXCSR( uptr from ) {
	write16( 0xAE0F );
	ModRM( 0, 0x3, DISP32 );
	write32( MEMADDR(from, 4) );
}

/////////////////////////////////////////////////////////////////////////////////////
//**********************************************************************************/
//LDMXCSR : Load Streaming SIMD Extension Control/Status                         *
//**********************************************************************************
emitterT void SSE_LDMXCSR( uptr from ) {
	write16( 0xAE0F );
	ModRM( 0, 0x2, DISP32 );
	write32( MEMADDR(from, 4) );
}

///////////////////////////////////////////////////////////////////////////////////
//**********************************************************************************/
//PCMPxx: Compare Packed Integers                                                  *
//**********************************************************************************
emitterT void SSE2_PCMPGTB_XMM_to_XMM(x86SSERegType to, x86SSERegType from ){ SSERtoR66( 0x640F ); }
emitterT void SSE2_PCMPGTB_M128_to_XMM(x86SSERegType to, uptr from ){ SSEMtoR66( 0x640F ); }
emitterT void SSE2_PCMPGTW_XMM_to_XMM(x86SSERegType to, x86SSERegType from ){ SSERtoR66( 0x650F ); }
emitterT void SSE2_PCMPGTW_M128_to_XMM(x86SSERegType to, uptr from ){ SSEMtoR66( 0x650F ); }
emitterT void SSE2_PCMPGTD_XMM_to_XMM(x86SSERegType to, x86SSERegType from ){ SSERtoR66( 0x660F ); }
emitterT void SSE2_PCMPGTD_M128_to_XMM(x86SSERegType to, uptr from ){ SSEMtoR66( 0x660F ); }
emitterT void SSE2_PCMPEQB_XMM_to_XMM(x86SSERegType to, x86SSERegType from ){ SSERtoR66( 0x740F ); }
emitterT void SSE2_PCMPEQB_M128_to_XMM(x86SSERegType to, uptr from ){ SSEMtoR66( 0x740F ); }
emitterT void SSE2_PCMPEQW_XMM_to_XMM(x86SSERegType to, x86SSERegType from ){ SSERtoR66( 0x750F ); }
emitterT void SSE2_PCMPEQW_M128_to_XMM(x86SSERegType to, uptr from ){ SSEMtoR66( 0x750F ); }
emitterT void SSE2_PCMPEQD_XMM_to_XMM(x86SSERegType to, x86SSERegType from ){ SSERtoR66( 0x760F ); }
emitterT void SSE2_PCMPEQD_M128_to_XMM(x86SSERegType to, uptr from ){ SSEMtoR66( 0x760F ); }

////////////////////////////////////////////////////////////////////////////////////////////
//**********************************************************************************/
//PEXTRW,PINSRW: Packed Extract/Insert Word                                        *
//**********************************************************************************
emitterT void SSE_PEXTRW_XMM_to_R32(x86IntRegType to, x86SSERegType from, u8 imm8 ){ SSERtoR66(0xC50F); write8( imm8 ); }
emitterT void SSE_PINSRW_R32_to_XMM(x86SSERegType to, x86IntRegType from, u8 imm8 ){ SSERtoR66(0xC40F); write8( imm8 ); }

///////////////////////////////////////////////////////////////////////////////////////

// shift right arithmetic

emitterT void SSE2_PSRAW_XMM_to_XMM(x86SSERegType to, x86SSERegType from)	{ SSERtoR66(0xE10F); }
emitterT void SSE2_PSRAW_M128_to_XMM(x86SSERegType to, uptr from)			{ SSEMtoR66(0xE10F); }
emitterT void SSE2_PSRAW_I8_to_XMM(x86SSERegType to, u8 imm8)
{
	write8( 0x66 );
    RexB(0, to);
	write16( 0x710F );
	ModRM( 3, 4 , to );
	write8( imm8 );
}

emitterT void SSE2_PSRAD_XMM_to_XMM(x86SSERegType to, x86SSERegType from)	{ SSERtoR66(0xE20F); }
emitterT void SSE2_PSRAD_M128_to_XMM(x86SSERegType to, uptr from)			{ SSEMtoR66(0xE20F); }
emitterT void SSE2_PSRAD_I8_to_XMM(x86SSERegType to, u8 imm8)
{
	write8( 0x66 );
    RexB(0, to);
	write16( 0x720F );
	ModRM( 3, 4 , to );
	write8( imm8 );
}

emitterT void SSE2_PMAXSW_XMM_to_XMM( x86SSERegType to, x86SSERegType from )	{ SSERtoR66( 0xEE0F ); }
emitterT void SSE2_PMAXSW_M128_to_XMM( x86SSERegType to, uptr from )			{ SSEMtoR66( 0xEE0F ); }

emitterT void SSE2_PMAXUB_XMM_to_XMM( x86SSERegType to, x86SSERegType from )	{ SSERtoR66( 0xDE0F ); }
emitterT void SSE2_PMAXUB_M128_to_XMM( x86SSERegType to, uptr from )			{ SSEMtoR66( 0xDE0F ); }

emitterT void SSE2_PMINSW_XMM_to_XMM( x86SSERegType to, x86SSERegType from )	{ SSERtoR66( 0xEA0F ); }
emitterT void SSE2_PMINSW_M128_to_XMM( x86SSERegType to, uptr from )			{ SSEMtoR66( 0xEA0F ); }

emitterT void SSE2_PMINUB_XMM_to_XMM( x86SSERegType to, x86SSERegType from )	{ SSERtoR66( 0xDA0F ); }
emitterT void SSE2_PMINUB_M128_to_XMM( x86SSERegType to, uptr from )			{ SSEMtoR66( 0xDA0F ); }

//**********************************************************************************/
//PACKSSWB,PACKSSDW: Pack Saturate Signed Word
//**********************************************************************************
emitterT void SSE2_PACKSSWB_XMM_to_XMM(x86SSERegType to, x86SSERegType from)	{ SSERtoR66( 0x630F ); }
emitterT void SSE2_PACKSSWB_M128_to_XMM(x86SSERegType to, uptr from)			{ SSEMtoR66( 0x630F ); }
emitterT void SSE2_PACKSSDW_XMM_to_XMM(x86SSERegType to, x86SSERegType from)	{ SSERtoR66( 0x6B0F ); }
emitterT void SSE2_PACKSSDW_M128_to_XMM(x86SSERegType to, uptr from)			{ SSEMtoR66( 0x6B0F ); }

emitterT void SSE2_PACKUSWB_XMM_to_XMM(x86SSERegType to, x86SSERegType from)	{ SSERtoR66( 0x670F ); }
emitterT void SSE2_PACKUSWB_M128_to_XMM(x86SSERegType to, uptr from)			{ SSEMtoR66( 0x670F ); }

//**********************************************************************************/
//PUNPCKHWD: Unpack 16bit high
//**********************************************************************************
emitterT void SSE2_PUNPCKLBW_XMM_to_XMM(x86SSERegType to, x86SSERegType from)	{ SSERtoR66( 0x600F ); }
emitterT void SSE2_PUNPCKLBW_M128_to_XMM(x86SSERegType to, uptr from)			{ SSEMtoR66( 0x600F ); }

emitterT void SSE2_PUNPCKHBW_XMM_to_XMM(x86SSERegType to, x86SSERegType from)	{ SSERtoR66( 0x680F ); }
emitterT void SSE2_PUNPCKHBW_M128_to_XMM(x86SSERegType to, uptr from)			{ SSEMtoR66( 0x680F ); }

emitterT void SSE2_PUNPCKLWD_XMM_to_XMM(x86SSERegType to, x86SSERegType from)	{ SSERtoR66( 0x610F ); }
emitterT void SSE2_PUNPCKLWD_M128_to_XMM(x86SSERegType to, uptr from)			{ SSEMtoR66( 0x610F ); }
emitterT void SSE2_PUNPCKHWD_XMM_to_XMM(x86SSERegType to, x86SSERegType from)	{ SSERtoR66( 0x690F ); }
emitterT void SSE2_PUNPCKHWD_M128_to_XMM(x86SSERegType to, uptr from)			{ SSEMtoR66( 0x690F ); }

emitterT void SSE2_PUNPCKLDQ_XMM_to_XMM(x86SSERegType to, x86SSERegType from)	{ SSERtoR66( 0x620F ); }
emitterT void SSE2_PUNPCKLDQ_M128_to_XMM(x86SSERegType to, uptr from)			{ SSEMtoR66( 0x620F ); }
emitterT void SSE2_PUNPCKHDQ_XMM_to_XMM(x86SSERegType to, x86SSERegType from)	{ SSERtoR66( 0x6A0F ); }
emitterT void SSE2_PUNPCKHDQ_M128_to_XMM(x86SSERegType to, uptr from)			{ SSEMtoR66( 0x6A0F ); }

emitterT void SSE2_PUNPCKLQDQ_XMM_to_XMM(x86SSERegType to, x86SSERegType from) { SSERtoR66( 0x6C0F ); }
emitterT void SSE2_PUNPCKLQDQ_M128_to_XMM(x86SSERegType to, uptr from)			{ SSEMtoR66( 0x6C0F ); }

emitterT void SSE2_PUNPCKHQDQ_XMM_to_XMM(x86SSERegType to, x86SSERegType from)	{ SSERtoR66( 0x6D0F ); }
emitterT void SSE2_PUNPCKHQDQ_M128_to_XMM(x86SSERegType to, uptr from)			{ SSEMtoR66( 0x6D0F ); }

emitterT void SSE2_PMULLW_XMM_to_XMM(x86SSERegType to, x86SSERegType from)		{ SSERtoR66( 0xD50F ); }
emitterT void SSE2_PMULLW_M128_to_XMM(x86SSERegType to, uptr from)				{ SSEMtoR66( 0xD50F ); }
emitterT void SSE2_PMULHW_XMM_to_XMM(x86SSERegType to, x86SSERegType from)		{ SSERtoR66( 0xE50F ); }
emitterT void SSE2_PMULHW_M128_to_XMM(x86SSERegType to, uptr from)				{ SSEMtoR66( 0xE50F ); }

emitterT void SSE2_PMULUDQ_XMM_to_XMM(x86SSERegType to, x86SSERegType from)	{ SSERtoR66( 0xF40F ); }
emitterT void SSE2_PMULUDQ_M128_to_XMM(x86SSERegType to, uptr from)			{ SSEMtoR66( 0xF40F ); }

emitterT void SSE_MOVMSKPS_XMM_to_R32(x86IntRegType to, x86SSERegType from)	{ SSERtoR(0x500F); }
emitterT void SSE2_MOVMSKPD_XMM_to_R32(x86IntRegType to, x86SSERegType from)	{ SSERtoR66(0x500F); }

emitterT void SSE2_PMADDWD_XMM_to_XMM(x86SSERegType to, x86SSERegType from)	{ SSERtoR66(0xF50F); }

emitterT void SSE3_HADDPS_XMM_to_XMM(x86SSERegType to, x86SSERegType from)		{ write8(0xf2); SSERtoR( 0x7c0f ); }
emitterT void SSE3_HADDPS_M128_to_XMM(x86SSERegType to, uptr from)				{ write8(0xf2); SSEMtoR( 0x7c0f, 0 ); }

emitterT void SSE3_MOVSLDUP_XMM_to_XMM(x86SSERegType to, x86SSERegType from) {
	write8(0xf3);
    RexRB(0, to, from);
	write16( 0x120f);
	ModRM( 3, to, from );
}

emitterT void SSE3_MOVSLDUP_M128_to_XMM(x86SSERegType to, uptr from)			{ write8(0xf3); SSEMtoR(0x120f, 0); }
emitterT void SSE3_MOVSHDUP_XMM_to_XMM(x86SSERegType to, x86SSERegType from)	{ write8(0xf3); SSERtoR(0x160f); }
emitterT void SSE3_MOVSHDUP_M128_to_XMM(x86SSERegType to, uptr from)			{ write8(0xf3); SSEMtoR(0x160f, 0); }

// SSSE3

emitterT void SSSE3_PABSB_XMM_to_XMM(x86SSERegType to, x86SSERegType from)
{
	write8(0x66);
	RexRB(0, to, from);
	write24(0x1C380F);
	ModRM(3, to, from);
}

emitterT void SSSE3_PABSW_XMM_to_XMM(x86SSERegType to, x86SSERegType from)
{
	write8(0x66);
	RexRB(0, to, from);
	write24(0x1D380F);
	ModRM(3, to, from);
}

emitterT void SSSE3_PABSD_XMM_to_XMM(x86SSERegType to, x86SSERegType from)
{
	write8(0x66);
	RexRB(0, to, from);
	write24(0x1E380F);
	ModRM(3, to, from);
}

emitterT void SSSE3_PALIGNR_XMM_to_XMM(x86SSERegType to, x86SSERegType from, u8 imm8)
{
	write8(0x66);
	RexRB(0, to, from);
	write24(0x0F3A0F);
	ModRM(3, to, from);
	write8(imm8);
}

emitterT void SSSE3_PSIGNB_XMM_to_XMM(x86SSERegType to, x86SSERegType from)
{
	write8(0x66);
	RexRB(0, to, from);
	write24(0x08380F);
	ModRM(3, to, from);
}

emitterT void SSSE3_PSIGNW_XMM_to_XMM(x86SSERegType to, x86SSERegType from)
{
	write8(0x66);
	RexRB(0, to, from);
	write24(0x09380F);
	ModRM(3, to, from);
}

emitterT void SSSE3_PSIGND_XMM_to_XMM(x86SSERegType to, x86SSERegType from)
{
	write8(0x66);
	RexRB(0, to, from);
	write24(0x0A380F);
	ModRM(3, to, from);
}

// SSE4.1

emitterT void SSE4_DPPS_XMM_to_XMM(x86SSERegType to, x86SSERegType from, u8 imm8) 
{
	write8(0x66);
	write24(0x403A0F);
	ModRM(3, to, from);
	write8(imm8);
}

emitterT void SSE4_DPPS_M128_to_XMM(x86SSERegType to, uptr from, u8 imm8)
{
	write8(0x66);
	write24(0x403A0F);
	ModRM(0, to, DISP32);
	write32(MEMADDR(from, 4));
	write8(imm8);
}

emitterT void SSE4_INSERTPS_XMM_to_XMM(x86SSERegType to, x86SSERegType from, u8 imm8)
{
	write8(0x66);
    RexRB(0, to, from);
	write24(0x213A0F);
	ModRM(3, to, from);
	write8(imm8);
}

emitterT void SSE4_EXTRACTPS_XMM_to_R32(x86IntRegType to, x86SSERegType from, u8 imm8)
{
	write8(0x66);
    RexRB(0, to, from);
	write24(0x173A0F);
	ModRM(3, to, from);
	write8(imm8);
}

emitterT void SSE4_BLENDPS_XMM_to_XMM(x86IntRegType to, x86SSERegType from, u8 imm8)
{
	write8(0x66);
    RexRB(0, to, from);
	write24(0x0C3A0F);
	ModRM(3, to, from);
	write8(imm8);
}

emitterT void SSE4_BLENDVPS_XMM_to_XMM(x86SSERegType to, x86SSERegType from)
{
	write8(0x66);
    RexRB(0, to, from);
	write24(0x14380F);
	ModRM(3, to, from);
}

emitterT void SSE4_BLENDVPS_M128_to_XMM(x86SSERegType to, uptr from)
{
	write8(0x66);
    RexR(0, to);
	write24(0x14380F);
	ModRM(0, to, DISP32);
	write32(MEMADDR(from, 4));
}

emitterT void SSE4_PMOVSXDQ_XMM_to_XMM(x86SSERegType to, x86SSERegType from)
{
	write8(0x66);
    RexRB(0, to, from);
	write24(0x25380F);
	ModRM(3, to, from);
}

emitterT void SSE4_PMOVZXDQ_XMM_to_XMM(x86SSERegType to, x86SSERegType from)
{
	write8(0x66);
    RexRB(0, to, from);
	write24(0x35380F);
	ModRM(3, to, from);
}

emitterT void SSE4_PINSRD_R32_to_XMM(x86SSERegType to, x86IntRegType from, u8 imm8)
{
	write8(0x66);
    RexRB(0, to, from);
	write24(0x223A0F);
	ModRM(3, to, from);
	write8(imm8);
}

emitterT void SSE4_PMAXSD_XMM_to_XMM(x86SSERegType to, x86SSERegType from)
{
	write8(0x66);
	RexRB(0, to, from);
	write24(0x3D380F);
	ModRM(3, to, from);
}

emitterT void SSE4_PMINSD_XMM_to_XMM(x86SSERegType to, x86SSERegType from)
{
	write8(0x66);
	RexRB(0, to, from);
	write24(0x39380F);
	ModRM(3, to, from);
}

emitterT void SSE4_PMAXUD_XMM_to_XMM(x86SSERegType to, x86SSERegType from)
{
	write8(0x66);
	RexRB(0, to, from);
	write24(0x3F380F);
	ModRM(3, to, from);
}

emitterT void SSE4_PMINUD_XMM_to_XMM(x86SSERegType to, x86SSERegType from)
{
	write8(0x66);
	RexRB(0, to, from);
	write24(0x3B380F);
	ModRM(3, to, from);
}

emitterT void SSE4_PMAXSD_M128_to_XMM(x86SSERegType to, uptr from)
{
	write8(0x66);
	RexR(0, to);
	write24(0x3D380F);
	ModRM( 0, to, DISP32 );
	write32(MEMADDR(from, 4));
}

emitterT void SSE4_PMINSD_M128_to_XMM(x86SSERegType to, uptr from)
{
	write8(0x66);
	RexR(0, to);
	write24(0x39380F);
	ModRM( 0, to, DISP32 );
	write32(MEMADDR(from, 4));
}

emitterT void SSE4_PMAXUD_M128_to_XMM(x86SSERegType to, uptr from)
{
	write8(0x66);
	RexR(0, to);
	write24(0x3F380F);
	ModRM( 0, to, DISP32 );
	write32(MEMADDR(from, 4));
}

emitterT void SSE4_PMINUD_M128_to_XMM(x86SSERegType to, uptr from)
{
	write8(0x66);
	RexR(0, to);
	write24(0x3B380F);
	ModRM( 0, to, DISP32 );
	write32(MEMADDR(from, 4));
}

emitterT void SSE4_PMULDQ_XMM_to_XMM(x86SSERegType to, x86SSERegType from)
{
	write8(0x66);
	RexRB(0, to, from);
	write24(0x28380F);
	ModRM(3, to, from);
}

//////////////////////////////////////////////////////////////////////////////////////////
// SSE-X Helpers (generates either INT or FLOAT versions of certain SSE instructions)
//
// Added AlwaysUseMovaps check to the relevant functions here, which helps reduce the
// overhead of dynarec instructions that use these, even thought the same check would
// have been done redundantly by the emitter function.

emitterT void SSEX_MOVDQA_M128_to_XMM( x86SSERegType to, uptr from )
{
	if( !AlwaysUseMovaps && g_xmmtypes[to] == XMMT_INT ) SSE2_MOVDQA_M128_to_XMM(to, from);
	else SSE_MOVAPS_M128_to_XMM(to, from);
}

emitterT void SSEX_MOVDQA_XMM_to_M128( uptr to, x86SSERegType from )
{
	if( !AlwaysUseMovaps && g_xmmtypes[from] == XMMT_INT ) SSE2_MOVDQA_XMM_to_M128(to, from);
	else SSE_MOVAPS_XMM_to_M128(to, from);
}

emitterT void SSEX_MOVDQA_XMM_to_XMM( x86SSERegType to, x86SSERegType from )
{
	if( !AlwaysUseMovaps && g_xmmtypes[from] == XMMT_INT ) SSE2_MOVDQA_XMM_to_XMM(to, from);
	else SSE_MOVAPS_XMM_to_XMM(to, from);
}

emitterT void SSEX_MOVDQARmtoR( x86SSERegType to, x86IntRegType from, int offset )
{
	if( !AlwaysUseMovaps && g_xmmtypes[to] == XMMT_INT ) SSE2_MOVDQARmtoR(to, from, offset);
	else SSE_MOVAPSRmtoR(to, from, offset);
}

emitterT void SSEX_MOVDQARtoRm( x86IntRegType to, x86SSERegType from, int offset )
{
	if( !AlwaysUseMovaps && g_xmmtypes[from] == XMMT_INT ) SSE2_MOVDQARtoRm(to, from, offset);
	else SSE_MOVAPSRtoRm(to, from, offset);
}

emitterT void SSEX_MOVDQU_M128_to_XMM( x86SSERegType to, uptr from )
{
	if( !AlwaysUseMovaps && g_xmmtypes[to] == XMMT_INT ) SSE2_MOVDQU_M128_to_XMM(to, from);
	else SSE_MOVUPS_M128_to_XMM(to, from);
}

emitterT void SSEX_MOVDQU_XMM_to_M128( uptr to, x86SSERegType from )
{
	if( !AlwaysUseMovaps && g_xmmtypes[from] == XMMT_INT ) SSE2_MOVDQU_XMM_to_M128(to, from);
	else SSE_MOVUPS_XMM_to_M128(to, from);
}

emitterT void SSEX_MOVD_M32_to_XMM( x86SSERegType to, uptr from )
{
	if( g_xmmtypes[to] == XMMT_INT ) SSE2_MOVD_M32_to_XMM(to, from);
	else SSE_MOVSS_M32_to_XMM(to, from);
}

emitterT void SSEX_MOVD_XMM_to_M32( u32 to, x86SSERegType from )
{
	if( g_xmmtypes[from] == XMMT_INT ) SSE2_MOVD_XMM_to_M32(to, from);
	else SSE_MOVSS_XMM_to_M32(to, from);
}

emitterT void SSEX_MOVD_Rm_to_XMM( x86SSERegType to, x86IntRegType from, int offset )
{
	if( g_xmmtypes[to] == XMMT_INT ) SSE2_MOVD_Rm_to_XMM(to, from, offset);
	else SSE_MOVSS_Rm_to_XMM(to, from, offset);
}

emitterT void SSEX_MOVD_XMM_to_Rm( x86IntRegType to, x86SSERegType from, int offset )
{
	if( g_xmmtypes[from] == XMMT_INT ) SSE2_MOVD_XMM_to_Rm(to, from, offset);
	else SSE_MOVSS_XMM_to_Rm(to, from, offset);
}

emitterT void SSEX_POR_M128_to_XMM( x86SSERegType to, uptr from )
{
	if( g_xmmtypes[to] == XMMT_INT ) SSE2_POR_M128_to_XMM(to, from);
	else SSE_ORPS_M128_to_XMM(to, from);
}

emitterT void SSEX_POR_XMM_to_XMM( x86SSERegType to, x86SSERegType from )
{
	if( g_xmmtypes[from] == XMMT_INT ) SSE2_POR_XMM_to_XMM(to, from);
	else SSE_ORPS_XMM_to_XMM(to, from);
}

emitterT void SSEX_PXOR_M128_to_XMM( x86SSERegType to, uptr from )
{
	if( g_xmmtypes[to] == XMMT_INT ) SSE2_PXOR_M128_to_XMM(to, from);
	else SSE_XORPS_M128_to_XMM(to, from);
}

emitterT void SSEX_PXOR_XMM_to_XMM( x86SSERegType to, x86SSERegType from )
{
	if( g_xmmtypes[from] == XMMT_INT ) SSE2_PXOR_XMM_to_XMM(to, from);
	else SSE_XORPS_XMM_to_XMM(to, from);
}

emitterT void SSEX_PAND_M128_to_XMM( x86SSERegType to, uptr from )
{
	if( g_xmmtypes[to] == XMMT_INT ) SSE2_PAND_M128_to_XMM(to, from);
	else SSE_ANDPS_M128_to_XMM(to, from);
}

emitterT void SSEX_PAND_XMM_to_XMM( x86SSERegType to, x86SSERegType from )
{
	if( g_xmmtypes[from] == XMMT_INT ) SSE2_PAND_XMM_to_XMM(to, from);
	else SSE_ANDPS_XMM_to_XMM(to, from);
}

emitterT void SSEX_PANDN_M128_to_XMM( x86SSERegType to, uptr from )
{
	if( g_xmmtypes[to] == XMMT_INT ) SSE2_PANDN_M128_to_XMM(to, from);
	else SSE_ANDNPS_M128_to_XMM(to, from);
}

emitterT void SSEX_PANDN_XMM_to_XMM( x86SSERegType to, x86SSERegType from )
{
	if( g_xmmtypes[from] == XMMT_INT ) SSE2_PANDN_XMM_to_XMM(to, from);
	else SSE_ANDNPS_XMM_to_XMM(to, from);
}

emitterT void SSEX_PUNPCKLDQ_M128_to_XMM(x86SSERegType to, uptr from)
{
	if( g_xmmtypes[to] == XMMT_INT ) SSE2_PUNPCKLDQ_M128_to_XMM(to, from);
	else SSE_UNPCKLPS_M128_to_XMM(to, from);
}

emitterT void SSEX_PUNPCKLDQ_XMM_to_XMM(x86SSERegType to, x86SSERegType from)
{
	if( g_xmmtypes[from] == XMMT_INT ) SSE2_PUNPCKLDQ_XMM_to_XMM(to, from);
	else SSE_UNPCKLPS_XMM_to_XMM(to, from);
}

emitterT void SSEX_PUNPCKHDQ_M128_to_XMM(x86SSERegType to, uptr from)
{
	if( g_xmmtypes[to] == XMMT_INT ) SSE2_PUNPCKHDQ_M128_to_XMM(to, from);
	else SSE_UNPCKHPS_M128_to_XMM(to, from);
}

emitterT void SSEX_PUNPCKHDQ_XMM_to_XMM(x86SSERegType to, x86SSERegType from)
{
	if( g_xmmtypes[from] == XMMT_INT ) SSE2_PUNPCKHDQ_XMM_to_XMM(to, from);
	else SSE_UNPCKHPS_XMM_to_XMM(to, from);
}

emitterT void SSEX_MOVHLPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from )
{
	if( g_xmmtypes[from] == XMMT_INT ) {
		SSE2_PUNPCKHQDQ_XMM_to_XMM(to, from);
		if( to != from ) SSE2_PSHUFD_XMM_to_XMM(to, to, 0x4e);
	}
	else {
		SSE_MOVHLPS_XMM_to_XMM(to, from);
	}
}
