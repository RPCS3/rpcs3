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

// ------------------------------------------------------------------------
//                         MMX / SSE Mixed Bag
// ------------------------------------------------------------------------

emitterT void MOVQMtoR( x86MMXRegType to, uptr from )							{ xMOVQ( xRegisterMMX(to), (void*)from ); }
emitterT void MOVQRtoM( uptr to, x86MMXRegType from )							{ xMOVQ( (void*)to, xRegisterMMX(from) ); }
emitterT void MOVQRtoR( x86MMXRegType to, x86MMXRegType from )					{ xMOVQ( xRegisterMMX(to), xRegisterMMX(from) ); }
emitterT void MOVQRmtoR( x86MMXRegType to, x86IntRegType from, int offset )		{ xMOVQ( xRegisterMMX(to), ptr[xAddressReg(from)+offset] ); }
emitterT void MOVQRtoRm( x86IntRegType to, x86MMXRegType from, int offset )		{ xMOVQ( ptr[xAddressReg(to)+offset], xRegisterMMX(from) ); }

emitterT void MOVDMtoMMX( x86MMXRegType to, uptr from )							{ xMOVDZX( xRegisterMMX(to), (void*)from ); }
emitterT void MOVDMMXtoM( uptr to, x86MMXRegType from )							{ xMOVD( (void*)to, xRegisterMMX(from) ); }
emitterT void MOVD32RtoMMX( x86MMXRegType to, x86IntRegType from )				{ xMOVDZX( xRegisterMMX(to), xRegister32(from) ); }
emitterT void MOVD32RmtoMMX( x86MMXRegType to, x86IntRegType from, int offset )	{ xMOVDZX( xRegisterMMX(to), ptr[xAddressReg(from)+offset] ); }
emitterT void MOVD32MMXtoR( x86IntRegType to, x86MMXRegType from )				{ xMOVD( xRegister32(to), xRegisterMMX(from) ); }
emitterT void MOVD32MMXtoRm( x86IntRegType to, x86MMXRegType from, int offset )	{ xMOVD( ptr[xAddressReg(to)+offset], xRegisterMMX(from) ); }

emitterT void PMOVMSKBMMXtoR(x86IntRegType to, x86MMXRegType from)				{ xPMOVMSKB( xRegister32(to), xRegisterMMX(from) ); }
emitterT void MASKMOVQRtoR(x86MMXRegType to, x86MMXRegType from)				{ xMASKMOV( xRegisterMMX(to), xRegisterMMX(from) ); }

#define DEFINE_LEGACY_LOGIC_OPCODE( mod ) \
	emitterT void P##mod##RtoR( x86MMXRegType to, x86MMXRegType from )				{ xP##mod( xRegisterMMX(to), xRegisterMMX(from) ); } \
	emitterT void P##mod##MtoR( x86MMXRegType to, uptr from )						{ xP##mod( xRegisterMMX(to), (void*)from ); } \
	emitterT void SSE2_P##mod##_XMM_to_XMM( x86SSERegType to, x86SSERegType from )	{ xP##mod( xRegisterSSE(to), xRegisterSSE(from) ); } \
	emitterT void SSE2_P##mod##_M128_to_XMM( x86SSERegType to, uptr from )			{ xP##mod( xRegisterSSE(to), (void*)from ); }

#define DEFINE_LEGACY_ARITHMETIC( mod, sub ) \
	emitterT void P##mod##sub##RtoR( x86MMXRegType to, x86MMXRegType from )				{ xP##mod.sub( xRegisterMMX(to), xRegisterMMX(from) ); } \
	emitterT void P##mod##sub##MtoR( x86MMXRegType to, uptr from )						{ xP##mod.sub( xRegisterMMX(to), (void*)from ); } \
	emitterT void SSE2_P##mod##sub##_XMM_to_XMM( x86SSERegType to, x86SSERegType from )	{ xP##mod.sub( xRegisterSSE(to), xRegisterSSE(from) ); } \
	emitterT void SSE2_P##mod##sub##_M128_to_XMM( x86SSERegType to, uptr from )			{ xP##mod.sub( xRegisterSSE(to), (void*)from ); }

#define DEFINE_LEGACY_SHIFT_STUFF( mod, sub ) \
	emitterT void P##mod##sub##RtoR( x86MMXRegType to, x86MMXRegType from )				{ xP##mod.sub( xRegisterMMX(to), xRegisterMMX(from) ); } \
	emitterT void P##mod##sub##MtoR( x86MMXRegType to, uptr from )						{ xP##mod.sub( xRegisterMMX(to), (void*)from ); } \
	emitterT void P##mod##sub##ItoR( x86MMXRegType to, u8 imm )							{ xP##mod.sub( xRegisterMMX(to), imm ); } \
	emitterT void SSE2_P##mod##sub##_XMM_to_XMM( x86SSERegType to, x86SSERegType from )	{ xP##mod.sub( xRegisterSSE(to), xRegisterSSE(from) ); } \
	emitterT void SSE2_P##mod##sub##_M128_to_XMM( x86SSERegType to, uptr from )			{ xP##mod.sub( xRegisterSSE(to), (void*)from ); } \
	emitterT void SSE2_P##mod##sub##_I8_to_XMM( x86SSERegType to, u8 imm )				{ xP##mod.sub( xRegisterSSE(to), imm ); }

#define DEFINE_LEGACY_SHIFT_OPCODE( mod ) \
	DEFINE_LEGACY_SHIFT_STUFF( mod, Q ) \
	DEFINE_LEGACY_SHIFT_STUFF( mod, D ) \
	DEFINE_LEGACY_SHIFT_STUFF( mod, W ) \
	emitterT void SSE2_P##mod##DQ_I8_to_XMM( x86MMXRegType to, u8 imm )					{ xP##mod.DQ( xRegisterSSE(to), imm ); }

DEFINE_LEGACY_LOGIC_OPCODE( AND )
DEFINE_LEGACY_LOGIC_OPCODE( ANDN )
DEFINE_LEGACY_LOGIC_OPCODE( OR )
DEFINE_LEGACY_LOGIC_OPCODE( XOR )

DEFINE_LEGACY_SHIFT_OPCODE( SLL )
DEFINE_LEGACY_SHIFT_OPCODE( SRL )
DEFINE_LEGACY_SHIFT_STUFF( SRA, D )
DEFINE_LEGACY_SHIFT_STUFF( SRA, W )

DEFINE_LEGACY_ARITHMETIC( ADD, B )
DEFINE_LEGACY_ARITHMETIC( ADD, W )
DEFINE_LEGACY_ARITHMETIC( ADD, D )
DEFINE_LEGACY_ARITHMETIC( ADD, Q )
DEFINE_LEGACY_ARITHMETIC( ADD, SB )
DEFINE_LEGACY_ARITHMETIC( ADD, SW )
DEFINE_LEGACY_ARITHMETIC( ADD, USB )
DEFINE_LEGACY_ARITHMETIC( ADD, USW )

DEFINE_LEGACY_ARITHMETIC( SUB, B )
DEFINE_LEGACY_ARITHMETIC( SUB, W )
DEFINE_LEGACY_ARITHMETIC( SUB, D )
DEFINE_LEGACY_ARITHMETIC( SUB, Q )
DEFINE_LEGACY_ARITHMETIC( SUB, SB )
DEFINE_LEGACY_ARITHMETIC( SUB, SW )
DEFINE_LEGACY_ARITHMETIC( SUB, USB )
DEFINE_LEGACY_ARITHMETIC( SUB, USW )

DEFINE_LEGACY_ARITHMETIC( CMP, EQB );
DEFINE_LEGACY_ARITHMETIC( CMP, EQW );
DEFINE_LEGACY_ARITHMETIC( CMP, EQD );
DEFINE_LEGACY_ARITHMETIC( CMP, GTB );
DEFINE_LEGACY_ARITHMETIC( CMP, GTW );
DEFINE_LEGACY_ARITHMETIC( CMP, GTD );

DEFINE_LEGACY_ARITHMETIC( UNPCK, HDQ );
DEFINE_LEGACY_ARITHMETIC( UNPCK, LDQ );
DEFINE_LEGACY_ARITHMETIC( UNPCK, HBW );
DEFINE_LEGACY_ARITHMETIC( UNPCK, LBW );

DEFINE_LEGACY_ARITHMETIC( UNPCK, LWD );
DEFINE_LEGACY_ARITHMETIC( UNPCK, HWD );


emitterT void PMULUDQMtoR( x86MMXRegType to, uptr from )					{ xPMUL.UDQ( xRegisterMMX( to ), (void*)from ); }
emitterT void PMULUDQRtoR( x86MMXRegType to, x86MMXRegType from )			{ xPMUL.UDQ( xRegisterMMX( to ), xRegisterMMX( from ) ); }

emitterT void PSHUFWRtoR(x86MMXRegType to, x86MMXRegType from, u8 imm8)		{ xPSHUF.W( xRegisterMMX(to), xRegisterMMX(from), imm8 ); }
emitterT void PSHUFWMtoR(x86MMXRegType to, uptr from, u8 imm8)				{ xPSHUF.W( xRegisterMMX(to), (void*)from, imm8 ); }

emitterT void PINSRWRtoMMX( x86MMXRegType to, x86SSERegType from, u8 imm8 ) { xPINSR.W( xRegisterMMX(to), xRegister32(from), imm8 ); }

emitterT void EMMS() { xEMMS(); }

// ------------------------------------------------------------------------
//                         Begin SSE-Only Part!
// ------------------------------------------------------------------------

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

#define DEFINE_LEGACY_OP128( ssenum, mod, sub ) \
	emitterT void SSE##ssenum##_##mod##sub##_XMM_to_XMM( x86SSERegType to, x86SSERegType from )	{ x##mod.sub( xRegisterSSE(to), xRegisterSSE(from) ); } \
	emitterT void SSE##ssenum##_##mod##sub##_M128_to_XMM( x86SSERegType to, uptr from )			{ x##mod.sub( xRegisterSSE(to), (void*)from ); }

#define DEFINE_LEGACY_MOV128( ssenum, mod, sub ) \
	emitterT void SSE##ssenum##_##mod##sub##_XMM_to_XMM( x86SSERegType to, x86SSERegType from )	{ x##mod##sub( xRegisterSSE(to), xRegisterSSE(from) ); } \
	emitterT void SSE##ssenum##_##mod##sub##_M128_to_XMM( x86SSERegType to, uptr from )			{ x##mod##sub( xRegisterSSE(to), (void*)from ); }


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

DEFINE_LEGACY_OP128( 2, PMUL, LW )
DEFINE_LEGACY_OP128( 2, PMUL, HW )
DEFINE_LEGACY_OP128( 2, PMUL, UDQ )

DEFINE_LEGACY_OP128( 2, PMAX, SW )
DEFINE_LEGACY_OP128( 2, PMAX, UB )
DEFINE_LEGACY_OP128( 2, PMIN, SW )
DEFINE_LEGACY_OP128( 2, PMIN, UB )

DEFINE_LEGACY_OP128( 2, UNPCK, LPS )
DEFINE_LEGACY_OP128( 2, UNPCK, HPS )
DEFINE_LEGACY_OP128( 2, PUNPCK, LQDQ )
DEFINE_LEGACY_OP128( 2, PUNPCK, HQDQ )

DEFINE_LEGACY_OP128( 2, PACK, SSWB )
DEFINE_LEGACY_OP128( 2, PACK, SSDW )
DEFINE_LEGACY_OP128( 2, PACK, USWB )

DEFINE_LEGACY_MOV128( 3, MOV, SLDUP )
DEFINE_LEGACY_MOV128( 3, MOV, SHDUP )

DEFINE_LEGACY_OP128( 4, PMAX, SD )
DEFINE_LEGACY_OP128( 4, PMIN, SD )
DEFINE_LEGACY_OP128( 4, PMAX, UD )
DEFINE_LEGACY_OP128( 4, PMIN, UD )


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

emitterT void SSE_MOVLHPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from )			{ xMOVLH.PS( xRegisterSSE(to), xRegisterSSE(from) ); }
emitterT void SSE_MOVHLPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from )			{ xMOVHL.PS( xRegisterSSE(to), xRegisterSSE(from) ); }

emitterT void SSE_MASKMOVDQU_XMM_to_XMM( x86SSERegType to, x86SSERegType from )			{ xMASKMOV( xRegisterSSE(to), xRegisterSSE(from) ); }
emitterT void SSE2_PMOVMSKB_XMM_to_R32(x86IntRegType to, x86SSERegType from)			{ xPMOVMSKB( xRegister32(to), xRegisterSSE(from) ); }

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

emitterT void SSE_PMAXSW_MM_to_MM( x86MMXRegType to, x86MMXRegType from )		{ xPMAX.SW( xRegisterMMX(to), xRegisterMMX(from) ); }
emitterT void SSE_PMINSW_MM_to_MM( x86MMXRegType to, x86MMXRegType from )		{ xPMAX.SW( xRegisterMMX(to), xRegisterMMX(from) ); }

emitterT void SSE2_PSHUFD_XMM_to_XMM( x86SSERegType to, x86SSERegType from, u8 imm8 )	{ xPSHUF.D( xRegisterSSE(to), xRegisterSSE(from), imm8 ); }
emitterT void SSE2_PSHUFD_M128_to_XMM( x86SSERegType to, uptr from, u8 imm8 )			{ xPSHUF.D( xRegisterSSE(to), (void*)from, imm8 ); }
emitterT void SSE2_PSHUFLW_XMM_to_XMM( x86SSERegType to, x86SSERegType from, u8 imm8 )	{ xPSHUF.LW( xRegisterSSE(to), xRegisterSSE(from), imm8 ); }
emitterT void SSE2_PSHUFLW_M128_to_XMM( x86SSERegType to, uptr from, u8 imm8 )			{ xPSHUF.LW( xRegisterSSE(to), (void*)from, imm8 ); }
emitterT void SSE2_PSHUFHW_XMM_to_XMM( x86SSERegType to, x86SSERegType from, u8 imm8 )	{ xPSHUF.HW( xRegisterSSE(to), xRegisterSSE(from), imm8 ); }
emitterT void SSE2_PSHUFHW_M128_to_XMM( x86SSERegType to, uptr from, u8 imm8 )			{ xPSHUF.HW( xRegisterSSE(to), (void*)from, imm8 ); }

emitterT void SSE4_PMULDQ_XMM_to_XMM(x86SSERegType to, x86SSERegType from)		{ xPMUL.DQ( xRegisterSSE(to), xRegisterSSE(from) ); }

emitterT void SSE_UNPCKLPS_M128_to_XMM( x86SSERegType to, uptr from )			{ xUNPCK.LPS( xRegisterSSE(to), (void*)from ); }
emitterT void SSE_UNPCKLPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from )	{ xUNPCK.LPS( xRegisterSSE(to), xRegisterSSE(from) ); }
emitterT void SSE_UNPCKHPS_M128_to_XMM( x86SSERegType to, uptr from )			{ xUNPCK.HPS( xRegisterSSE(to), (void*)from ); }
emitterT void SSE_UNPCKHPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from )	{ xUNPCK.HPS( xRegisterSSE(to), xRegisterSSE(from) ); }

emitterT void SSE_MOVMSKPS_XMM_to_R32(x86IntRegType to, x86SSERegType from)		{ xMOVMSKPS( xRegister32(to), xRegisterSSE(from) ); }
emitterT void SSE2_MOVMSKPD_XMM_to_R32(x86IntRegType to, x86SSERegType from)	{ xMOVMSKPD( xRegister32(to), xRegisterSSE(from) ); }

emitterT void SSSE3_PABSB_XMM_to_XMM(x86SSERegType to, x86SSERegType from)		{ xPABS.B( xRegisterSSE(to), xRegisterSSE(from) ); }
emitterT void SSSE3_PABSW_XMM_to_XMM(x86SSERegType to, x86SSERegType from)		{ xPABS.W( xRegisterSSE(to), xRegisterSSE(from) ); }
emitterT void SSSE3_PABSD_XMM_to_XMM(x86SSERegType to, x86SSERegType from)		{ xPABS.D( xRegisterSSE(to), xRegisterSSE(from) ); }

emitterT void SSSE3_PSIGNB_XMM_to_XMM(x86SSERegType to, x86SSERegType from)		{ xPSIGN.B( xRegisterSSE(to), xRegisterSSE(from) ); }
emitterT void SSSE3_PSIGNW_XMM_to_XMM(x86SSERegType to, x86SSERegType from)		{ xPSIGN.W( xRegisterSSE(to), xRegisterSSE(from) ); }
emitterT void SSSE3_PSIGND_XMM_to_XMM(x86SSERegType to, x86SSERegType from)		{ xPSIGN.D( xRegisterSSE(to), xRegisterSSE(from) ); }

emitterT void SSE_PEXTRW_XMM_to_R32(x86IntRegType to, x86SSERegType from, u8 imm8 )	{ xPEXTR.W( xRegister32(to), xRegisterSSE(from), imm8 ); }
emitterT void SSE_PINSRW_R32_to_XMM(x86SSERegType to, x86IntRegType from, u8 imm8 )	{ xPINSR.W( xRegisterSSE(to), xRegister32(from), imm8 ); }

emitterT void SSE2_PMADDWD_XMM_to_XMM(x86SSERegType to, x86SSERegType from)			{ xPMADD.WD( xRegisterSSE(to), xRegisterSSE(from) ); }

emitterT void SSE3_HADDPS_XMM_to_XMM(x86SSERegType to, x86SSERegType from)			{ xHADD.PS( xRegisterSSE(to), xRegisterSSE(from) ); }
emitterT void SSE3_HADDPS_M128_to_XMM(x86SSERegType to, uptr from)					{ xHADD.PS( xRegisterSSE(to), (void*)from ); }

emitterT void SSE4_PINSRD_R32_to_XMM(x86SSERegType to, x86IntRegType from, u8 imm8)	{ xPINSR.D( xRegisterSSE(to), xRegister32(from), imm8 ); }

emitterT void SSE4_INSERTPS_XMM_to_XMM(x86SSERegType to, x86SSERegType from, u8 imm8)	{ xINSERTPS( xRegisterSSE(to), xRegisterSSE(from), imm8 ); }
emitterT void SSE4_EXTRACTPS_XMM_to_R32(x86IntRegType to, x86SSERegType from, u8 imm8)	{ xEXTRACTPS( xRegister32(to), xRegisterSSE(from), imm8 ); }

emitterT void SSE4_DPPS_XMM_to_XMM(x86SSERegType to, x86SSERegType from, u8 imm8)		{ xDP.PS( xRegisterSSE(to), xRegisterSSE(from), imm8 ); }
emitterT void SSE4_DPPS_M128_to_XMM(x86SSERegType to, uptr from, u8 imm8)				{ xDP.PS( xRegisterSSE(to), (void*)from, imm8 ); }

emitterT void SSE4_BLENDPS_XMM_to_XMM(x86IntRegType to, x86SSERegType from, u8 imm8)	{ xBLEND.PS( xRegisterSSE(to), xRegisterSSE(from), imm8 ); }
emitterT void SSE4_BLENDVPS_XMM_to_XMM(x86SSERegType to, x86SSERegType from)			{ xBLEND.VPS( xRegisterSSE(to), xRegisterSSE(from) ); }
emitterT void SSE4_BLENDVPS_M128_to_XMM(x86SSERegType to, uptr from)					{ xBLEND.VPS( xRegisterSSE(to), (void*)from ); }

emitterT void SSE4_PMOVSXDQ_XMM_to_XMM(x86SSERegType to, x86SSERegType from)			{ xPMOVSX.DQ( xRegisterSSE(to), xRegisterSSE(from) ); }

emitterT void SSE_LDMXCSR( uptr from ) { xLDMXCSR( (u32*)from ); }


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
