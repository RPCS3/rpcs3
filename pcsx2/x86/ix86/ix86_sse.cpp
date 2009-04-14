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

//////////////////////////////////////////////////////////////////////////////////////////
// AlwaysUseMovaps [const]
//
// This tells the recompiler's emitter to always use movaps instead of movdqa.  Both instructions
// do the exact same thing, but movaps is 1 byte shorter, and thus results in a cleaner L1 cache
// and some marginal speed gains as a result.  (it's possible someday in the future the per-
// formance of the two instructions could change, so this constant is provided to restore MOVDQA
// use easily at a later time, if needed).
//
static const bool AlwaysUseMovaps = true;


//------------------------------------------------------------------
// SSE instructions
//------------------------------------------------------------------

#define SSEMtoR( code, overb ) \
	assert( to < XMMREGS ), \
	RexR(0, to),             \
	write16( code ), \
	ModRM( 0, to, DISP32 ), \
	write32( MEMADDR(from, 4 + overb) )

#define SSERtoM( code, overb ) \
	assert( from < XMMREGS), \
    RexR(0, from),  \
	write16( code ), \
	ModRM( 0, from, DISP32 ), \
	write32( MEMADDR(to, 4 + overb) )

#define SSE_SS_MtoR( code, overb ) \
	assert( to < XMMREGS ), \
	write8( 0xf3 ), \
    RexR(0, to),                      \
	write16( code ), \
	ModRM( 0, to, DISP32 ), \
	write32( MEMADDR(from, 4 + overb) )

#define SSE_SS_RtoM( code, overb ) \
	assert( from < XMMREGS), \
	write8( 0xf3 ), \
	RexR(0, from), \
	write16( code ), \
	ModRM( 0, from, DISP32 ), \
	write32( MEMADDR(to, 4 + overb) )

#define SSERtoR( code ) \
	assert( to < XMMREGS && from < XMMREGS), \
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
	assert( to < XMMREGS && from < XMMREGS), \
	write8( 0x66 ), \
	RexRB(0, from, to), \
	write16( code ), \
	ModRM( 3, from, to )

#define SSE_SS_RtoR( code ) \
	assert( to < XMMREGS && from < XMMREGS), \
	write8( 0xf3 ), \
    RexRB(0, to, from),              \
	write16( code ), \
	ModRM( 3, to, from )

#define SSE_SD_MtoR( code, overb ) \
	assert( to < XMMREGS ) , \
	write8( 0xf2 ), \
    RexR(0, to),                      \
	write16( code ), \
	ModRM( 0, to, DISP32 ), \
	write32( MEMADDR(from, 4 + overb) ) \

#define SSE_SD_RtoM( code, overb ) \
	assert( from < XMMREGS) , \
	write8( 0xf2 ), \
	RexR(0, from), \
	write16( code ), \
	ModRM( 0, from, DISP32 ), \
	write32( MEMADDR(to, 4 + overb) ) \

#define SSE_SD_RtoR( code ) \
	assert( to < XMMREGS && from < XMMREGS) , \
	write8( 0xf2 ), \
    RexRB(0, to, from),   \
	write16( code ), \
	ModRM( 3, to, from )

#define CMPPSMtoR( op ) \
   SSEMtoR( 0xc20f, 1 ), \
   write8( op )

#define CMPPSRtoR( op ) \
   SSERtoR( 0xc20f ), \
   write8( op )

#define CMPSSMtoR( op ) \
   SSE_SS_MtoR( 0xc20f, 1 ), \
   write8( op )

#define CMPSSRtoR( op ) \
   SSE_SS_RtoR( 0xc20f ), \
   write8( op )

#define CMPSDMtoR( op ) \
   SSE_SD_MtoR( 0xc20f, 1 ), \
   write8( op )

#define CMPSDRtoR( op ) \
   SSE_SD_RtoR( 0xc20f ), \
   write8( op )

/* movups [r32][r32*scale] to xmm1 */
emitterT void SSE_MOVUPSRmStoR( x86SSERegType to, x86IntRegType from, x86IntRegType from2, int scale )
{
    RexRXB(0, to, from2, from);
	write16( 0x100f );
	ModRM( 0, to, 0x4 );
	SibSB( scale, from2, from );
}

/* movups xmm1 to [r32][r32*scale] */
emitterT void SSE_MOVUPSRtoRmS( x86SSERegType to, x86IntRegType from, x86IntRegType from2, int scale )
{
    RexRXB(1, to, from2, from);
	write16( 0x110f );
	ModRM( 0, to, 0x4 );
	SibSB( scale, from2, from );
}

/* movups [r32] to r32 */
emitterT void SSE_MOVUPSRmtoR( x86IntRegType to, x86IntRegType from ) 
{
	RexRB(0, to, from);
	write16( 0x100f );
	ModRM( 0, to, from );
}

/* movups r32 to [r32] */
emitterT void SSE_MOVUPSRtoRm( x86IntRegType to, x86IntRegType from ) 
{
    RexRB(0, from, to);
	write16( 0x110f );
	ModRM( 0, from, to );
}

/* movlps [r32] to r32 */
emitterT void SSE_MOVLPSRmtoR( x86SSERegType to, x86IntRegType from ) 
{
	RexRB(1, to, from);
	write16( 0x120f );
	ModRM( 0, to, from );
}

emitterT void SSE_MOVLPSRmtoR( x86SSERegType to, x86IntRegType from, int offset )
{
    RexRB(0, to, from);
	write16( 0x120f );
    WriteRmOffsetFrom(to, from, offset);
}

/* movaps r32 to [r32] */
emitterT void SSE_MOVLPSRtoRm( x86IntRegType to, x86IntRegType from ) 
{
    RexRB(0, from, to);
	write16( 0x130f );
	ModRM( 0, from, to );
}

emitterT void SSE_MOVLPSRtoRm( x86SSERegType to, x86IntRegType from, int offset )
{
    RexRB(0, from, to);
	write16( 0x130f );
    WriteRmOffsetFrom(from, to, offset);
}

/* movaps [r32][r32*scale] to xmm1 */
emitterT void SSE_MOVAPSRmStoR( x86SSERegType to, x86IntRegType from, x86IntRegType from2, int scale )
{
	assert( from != EBP );
    RexRXB(0, to, from2, from);
	write16( 0x280f );
	ModRM( 0, to, 0x4 );
	SibSB( scale, from2, from );
}

/* movaps xmm1 to [r32][r32*scale] */
emitterT void SSE_MOVAPSRtoRmS( x86SSERegType to, x86IntRegType from, x86IntRegType from2, int scale )
{
	assert( from != EBP );
    RexRXB(0, to, from2, from);
	write16( 0x290f );
	ModRM( 0, to, 0x4 );
	SibSB( scale, from2, from );
}

// movaps [r32+offset] to r32
emitterT void SSE_MOVAPSRmtoR( x86SSERegType to, x86IntRegType from, int offset )
{
	RexRB(0, to, from);
	write16( 0x280f );
    WriteRmOffsetFrom(to, from, offset);
}

// movaps r32 to [r32+offset]
emitterT void SSE_MOVAPSRtoRm( x86IntRegType to, x86SSERegType from, int offset ) 
{
	RexRB(0, from, to);
	write16( 0x290f );
    WriteRmOffsetFrom(from, to, offset);
}

// movdqa [r32+offset] to r32
emitterT void SSE2_MOVDQARmtoR( x86SSERegType to, x86IntRegType from, int offset )
{
	if( AlwaysUseMovaps )
		SSE_MOVAPSRmtoR( to, from, offset );
	else
	{
		write8(0x66);
		RexRB(0, to, from);
		write16( 0x6f0f );
		WriteRmOffsetFrom(to, from, offset);
	}
}

// movdqa r32 to [r32+offset]
emitterT void SSE2_MOVDQARtoRm( x86IntRegType to, x86SSERegType from, int offset ) 
{
	if( AlwaysUseMovaps )
		SSE_MOVAPSRtoRm( to, from, offset );
	else
	{
		write8(0x66);
		RexRB(0, from, to);
		write16( 0x7f0f );
		WriteRmOffsetFrom(from, to, offset);
	}
}

// movups [r32+offset] to r32
emitterT void SSE_MOVUPSRmtoR( x86SSERegType to, x86IntRegType from, int offset )
{
	RexRB(0, to, from);
	write16( 0x100f );
    WriteRmOffsetFrom(to, from, offset);
}

// movups r32 to [r32+offset]
emitterT void SSE_MOVUPSRtoRm( x86IntRegType to, x86SSERegType from, int offset )
{
    RexRB(0, from, to);
	write16( 0x110f );
    WriteRmOffsetFrom(from, to, offset);
}

//**********************************************************************************/
//MOVAPS: Move aligned Packed Single Precision FP values                           *
//**********************************************************************************
emitterT void SSE_MOVAPS_M128_to_XMM( x86SSERegType to, uptr from )          { SSEMtoR( 0x280f, 0 ); }
emitterT void SSE_MOVAPS_XMM_to_M128( uptr to, x86SSERegType from )          { SSERtoM( 0x290f, 0 ); }
emitterT void SSE_MOVAPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from )  { if (to != from) { SSERtoR( 0x280f ); } }

emitterT void SSE_MOVUPS_M128_to_XMM( x86SSERegType to, uptr from )          { SSEMtoR( 0x100f, 0 ); }
emitterT void SSE_MOVUPS_XMM_to_M128( uptr to, x86SSERegType from )          { SSERtoM( 0x110f, 0 ); }

emitterT void SSE2_MOVSD_XMM_to_XMM( x86SSERegType to, x86SSERegType from )  {	SSE_SD_RtoR( 0x100f); }
emitterT void SSE2_MOVSD_M64_to_XMM( x86SSERegType to, uptr from )  {	SSE_SD_MtoR( 0x100f, 0); }
emitterT void SSE2_MOVSD_XMM_to_M64( uptr to, x86SSERegType from )  {	SSE_SD_RtoM( 0x110f, 0); }

emitterT void SSE2_MOVQ_M64_to_XMM( x86SSERegType to, uptr from )
{
	write8(0xf3); SSEMtoR( 0x7e0f, 0);
}

emitterT void SSE2_MOVQ_XMM_to_XMM( x86SSERegType to, x86SSERegType from )
{
	write8(0xf3); SSERtoR( 0x7e0f);
}

emitterT void SSE2_MOVQ_XMM_to_M64( u32 to, x86SSERegType from )
{
	SSERtoM66(0xd60f);
}

emitterT void SSE2_MOVDQ2Q_XMM_to_MM( x86MMXRegType to, x86SSERegType from)
{
	write8(0xf2);
	SSERtoR( 0xd60f);
}
emitterT void SSE2_MOVQ2DQ_MM_to_XMM( x86SSERegType to, x86MMXRegType from)
{
	write8(0xf3);
	SSERtoR( 0xd60f);
}

//**********************************************************************************/
//MOVSS: Move Scalar Single-Precision FP  value                                    *
//**********************************************************************************
emitterT void SSE_MOVSS_M32_to_XMM( x86SSERegType to, uptr from )			{ SSE_SS_MtoR( 0x100f, 0 ); }
emitterT void SSE_MOVSS_XMM_to_M32( u32 to, x86SSERegType from )			{ SSE_SS_RtoM( 0x110f, 0 ); }

emitterT void SSE_MOVSS_XMM_to_XMM( x86SSERegType to, x86SSERegType from )	{ if (to != from) { SSE_SS_RtoR( 0x100f ); } }

emitterT void SSE_MOVSS_Rm_to_XMM( x86SSERegType to, x86IntRegType from, int offset )
{
	write8(0xf3);
    RexRB(0, to, from);
    write16( 0x100f );
    WriteRmOffsetFrom(to, from, offset);
}

emitterT void SSE_MOVSS_XMM_to_Rm( x86IntRegType to, x86SSERegType from, int offset )
{
	write8(0xf3);
    RexRB(0, from, to);
    write16(0x110f);
    WriteRmOffsetFrom(from, to, offset);
}

emitterT void SSE_MASKMOVDQU_XMM_to_XMM( x86SSERegType to, x86SSERegType from )	{ SSERtoR66( 0xf70f ); }
//**********************************************************************************/
//MOVLPS: Move low Packed Single-Precision FP                                     *
//**********************************************************************************
emitterT void SSE_MOVLPS_M64_to_XMM( x86SSERegType to, uptr from )	{ SSEMtoR( 0x120f, 0 ); }
emitterT void SSE_MOVLPS_XMM_to_M64( u32 to, x86SSERegType from )	{ SSERtoM( 0x130f, 0 ); }

emitterT void SSE_MOVLPS_Rm_to_XMM( x86SSERegType to, x86IntRegType from, int offset )
{
    RexRB(0, to, from);
	write16( 0x120f );
    WriteRmOffsetFrom(to, from, offset);
}

emitterT void SSE_MOVLPS_XMM_to_Rm( x86IntRegType to, x86SSERegType from, int offset )
{
    RexRB(0, from, to);
	write16(0x130f);
    WriteRmOffsetFrom(from, to, offset);
}

/////////////////////////////////////////////////////////////////////////////////////
//**********************************************************************************/
//MOVHPS: Move High Packed Single-Precision FP                                     *
//**********************************************************************************
emitterT void SSE_MOVHPS_M64_to_XMM( x86SSERegType to, uptr from )	{ SSEMtoR( 0x160f, 0 ); }
emitterT void SSE_MOVHPS_XMM_to_M64( u32 to, x86SSERegType from )	{ SSERtoM( 0x170f, 0 ); }

emitterT void SSE_MOVHPS_Rm_to_XMM( x86SSERegType to, x86IntRegType from, int offset )
{
    RexRB(0, to, from);
	write16( 0x160f );
    WriteRmOffsetFrom(to, from, offset);
}

emitterT void SSE_MOVHPS_XMM_to_Rm( x86IntRegType to, x86SSERegType from, int offset )
{
    RexRB(0, from, to);
	write16(0x170f);
    WriteRmOffsetFrom(from, to, offset);
}

/////////////////////////////////////////////////////////////////////////////////////
//**********************************************************************************/
//MOVLHPS: Moved packed Single-Precision FP low to high                            *
//**********************************************************************************
emitterT void SSE_MOVLHPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from )	{ SSERtoR( 0x160f ); }

//////////////////////////////////////////////////////////////////////////////////////
//**********************************************************************************/
//MOVHLPS: Moved packed Single-Precision FP High to Low                            *
//**********************************************************************************
emitterT void SSE_MOVHLPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from )	{ SSERtoR( 0x120f ); }

///////////////////////////////////////////////////////////////////////////////////
//**********************************************************************************/
//ANDPS: Logical Bit-wise  AND for Single FP                                        *
//**********************************************************************************
emitterT void SSE_ANDPS_M128_to_XMM( x86SSERegType to, uptr from )			{ SSEMtoR( 0x540f, 0 ); }
emitterT void SSE_ANDPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from )	{ SSERtoR( 0x540f ); }

emitterT void SSE2_ANDPD_M128_to_XMM( x86SSERegType to, uptr from )           { SSEMtoR66( 0x540f ); }
emitterT void SSE2_ANDPD_XMM_to_XMM( x86SSERegType to, x86SSERegType from ) { SSERtoR66( 0x540f ); }

///////////////////////////////////////////////////////////////////////////////////////
//**********************************************************************************/
//ANDNPS : Logical Bit-wise  AND NOT of Single-precision FP values                 *
//**********************************************************************************
emitterT void SSE_ANDNPS_M128_to_XMM( x86SSERegType to, uptr from )		{ SSEMtoR( 0x550f, 0 ); }
emitterT void SSE_ANDNPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from ){ SSERtoR( 0x550f ); }

emitterT void SSE2_ANDNPD_M128_to_XMM( x86SSERegType to, uptr from )          { SSEMtoR66( 0x550f ); }
emitterT void SSE2_ANDNPD_XMM_to_XMM( x86SSERegType to, x86SSERegType from ){ SSERtoR66( 0x550f ); }

/////////////////////////////////////////////////////////////////////////////////////
//**********************************************************************************/
//RCPPS : Packed Single-Precision FP Reciprocal                                     *
//**********************************************************************************
emitterT void SSE_RCPPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from )	{ SSERtoR( 0x530f ); }
emitterT void SSE_RCPPS_M128_to_XMM( x86SSERegType to, uptr from )			{ SSEMtoR( 0x530f, 0 ); }

emitterT void SSE_RCPSS_XMM_to_XMM( x86SSERegType to, x86SSERegType from ) { SSE_SS_RtoR(0x530f); }
emitterT void SSE_RCPSS_M32_to_XMM( x86SSERegType to, uptr from )			{ SSE_SS_MtoR(0x530f, 0); }

//////////////////////////////////////////////////////////////////////////////////////
//**********************************************************************************/
//ORPS : Bit-wise Logical OR of Single-Precision FP Data                            *
//**********************************************************************************
emitterT void SSE_ORPS_M128_to_XMM( x86SSERegType to, uptr from )				{ SSEMtoR( 0x560f, 0 ); }
emitterT void SSE_ORPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from )		{ SSERtoR( 0x560f ); }

emitterT void SSE2_ORPD_M128_to_XMM( x86SSERegType to, uptr from )            { SSEMtoR66( 0x560f ); }
emitterT void SSE2_ORPD_XMM_to_XMM( x86SSERegType to, x86SSERegType from )  { SSERtoR66( 0x560f ); }

/////////////////////////////////////////////////////////////////////////////////////
//**********************************************************************************/
//XORPS : Bitwise Logical XOR of Single-Precision FP Values                        *
//**********************************************************************************
emitterT void SSE_XORPS_M128_to_XMM( x86SSERegType to, uptr from )				{ SSEMtoR( 0x570f, 0 ); }
emitterT void SSE_XORPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from )		{ SSERtoR( 0x570f ); }

emitterT void SSE2_XORPD_M128_to_XMM( x86SSERegType to, uptr from )           { SSEMtoR66( 0x570f ); }
emitterT void SSE2_XORPD_XMM_to_XMM( x86SSERegType to, x86SSERegType from ) { SSERtoR66( 0x570f ); }

///////////////////////////////////////////////////////////////////////////////////////
//**********************************************************************************/
//ADDPS : ADD Packed Single-Precision FP Values                                    *
//**********************************************************************************
emitterT void SSE_ADDPS_M128_to_XMM( x86SSERegType to, uptr from )				{ SSEMtoR( 0x580f, 0 ); }
emitterT void SSE_ADDPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from )		{ SSERtoR( 0x580f ); }

////////////////////////////////////////////////////////////////////////////////////
//**********************************************************************************/
//ADDSS : ADD Scalar Single-Precision FP Values                                    *
//**********************************************************************************
emitterT void SSE_ADDSS_M32_to_XMM( x86SSERegType to, uptr from )				{ SSE_SS_MtoR( 0x580f, 0 ); }
emitterT void SSE_ADDSS_XMM_to_XMM( x86SSERegType to, x86SSERegType from )		{ SSE_SS_RtoR( 0x580f ); }

emitterT void SSE2_ADDSD_M64_to_XMM( x86SSERegType to, uptr from )           { SSE_SD_MtoR( 0x580f, 0 ); }
emitterT void SSE2_ADDSD_XMM_to_XMM( x86SSERegType to, x86SSERegType from ) { SSE_SD_RtoR( 0x580f ); }

/////////////////////////////////////////////////////////////////////////////////////////
//**********************************************************************************/
//SUBPS: Packed Single-Precision FP Subtract                                       *
//**********************************************************************************
emitterT void SSE_SUBPS_M128_to_XMM( x86SSERegType to, uptr from )				{ SSEMtoR( 0x5c0f, 0 ); }
emitterT void SSE_SUBPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from )		{ SSERtoR( 0x5c0f ); }

///////////////////////////////////////////////////////////////////////////////////////
//**********************************************************************************/
//SUBSS : Scalar  Single-Precision FP Subtract                                       *
//**********************************************************************************
emitterT void SSE_SUBSS_M32_to_XMM( x86SSERegType to, uptr from )				{ SSE_SS_MtoR( 0x5c0f, 0 ); }
emitterT void SSE_SUBSS_XMM_to_XMM( x86SSERegType to, x86SSERegType from )		{ SSE_SS_RtoR( 0x5c0f ); }

emitterT void SSE2_SUBSD_M64_to_XMM( x86SSERegType to, uptr from )           { SSE_SD_MtoR( 0x5c0f, 0 ); }
emitterT void SSE2_SUBSD_XMM_to_XMM( x86SSERegType to, x86SSERegType from ) { SSE_SD_RtoR( 0x5c0f ); }

/////////////////////////////////////////////////////////////////////////////////////////
//**********************************************************************************/
//MULPS : Packed Single-Precision FP Multiply                                      *
//**********************************************************************************
emitterT void SSE_MULPS_M128_to_XMM( x86SSERegType to, uptr from )				{ SSEMtoR( 0x590f, 0 ); }
emitterT void SSE_MULPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from )		{ SSERtoR( 0x590f ); }

////////////////////////////////////////////////////////////////////////////////////////
//**********************************************************************************/
//MULSS : Scalar  Single-Precision FP Multiply                                       *
//**********************************************************************************
emitterT void SSE_MULSS_M32_to_XMM( x86SSERegType to, uptr from )				{ SSE_SS_MtoR( 0x590f, 0 ); }
emitterT void SSE_MULSS_XMM_to_XMM( x86SSERegType to, x86SSERegType from )		{ SSE_SS_RtoR( 0x590f ); }

emitterT void SSE2_MULSD_M64_to_XMM( x86SSERegType to, uptr from )           { SSE_SD_MtoR( 0x590f, 0 ); }
emitterT void SSE2_MULSD_XMM_to_XMM( x86SSERegType to, x86SSERegType from ) { SSE_SD_RtoR( 0x590f ); }

////////////////////////////////////////////////////////////////////////////////////////////
//**********************************************************************************/
//Packed Single-Precission FP compare (CMPccPS)                                    *
//**********************************************************************************
//missing  SSE_CMPPS_I8_to_XMM
//         SSE_CMPPS_M32_to_XMM
//	       SSE_CMPPS_XMM_to_XMM
emitterT void SSE_CMPEQPS_M128_to_XMM( x86SSERegType to, uptr from )         { CMPPSMtoR( 0 ); }
emitterT void SSE_CMPEQPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from )    { CMPPSRtoR( 0 ); }
emitterT void SSE_CMPLTPS_M128_to_XMM( x86SSERegType to, uptr from )         { CMPPSMtoR( 1 ); }
emitterT void SSE_CMPLTPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from )    { CMPPSRtoR( 1 ); }
emitterT void SSE_CMPLEPS_M128_to_XMM( x86SSERegType to, uptr from )         { CMPPSMtoR( 2 ); }
emitterT void SSE_CMPLEPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from )    { CMPPSRtoR( 2 ); }
emitterT void SSE_CMPUNORDPS_M128_to_XMM( x86SSERegType to, uptr from )      { CMPPSMtoR( 3 ); }
emitterT void SSE_CMPUNORDPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from ) { CMPPSRtoR( 3 ); }
emitterT void SSE_CMPNEPS_M128_to_XMM( x86SSERegType to, uptr from )         { CMPPSMtoR( 4 ); }
emitterT void SSE_CMPNEPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from )    { CMPPSRtoR( 4 ); }
emitterT void SSE_CMPNLTPS_M128_to_XMM( x86SSERegType to, uptr from )        { CMPPSMtoR( 5 ); }
emitterT void SSE_CMPNLTPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from )   { CMPPSRtoR( 5 ); }
emitterT void SSE_CMPNLEPS_M128_to_XMM( x86SSERegType to, uptr from )        { CMPPSMtoR( 6 ); }
emitterT void SSE_CMPNLEPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from )   { CMPPSRtoR( 6 ); }
emitterT void SSE_CMPORDPS_M128_to_XMM( x86SSERegType to, uptr from )        { CMPPSMtoR( 7 ); }
emitterT void SSE_CMPORDPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from )   { CMPPSRtoR( 7 ); }

///////////////////////////////////////////////////////////////////////////////////////////
//**********************************************************************************/
//Scalar Single-Precission FP compare (CMPccSS)                                    *
//**********************************************************************************
//missing  SSE_CMPSS_I8_to_XMM
//         SSE_CMPSS_M32_to_XMM
//	       SSE_CMPSS_XMM_to_XMM
emitterT void SSE_CMPEQSS_M32_to_XMM( x86SSERegType to, uptr from )         { CMPSSMtoR( 0 ); }
emitterT void SSE_CMPEQSS_XMM_to_XMM( x86SSERegType to, x86SSERegType from )    { CMPSSRtoR( 0 ); }
emitterT void SSE_CMPLTSS_M32_to_XMM( x86SSERegType to, uptr from )         { CMPSSMtoR( 1 ); }
emitterT void SSE_CMPLTSS_XMM_to_XMM( x86SSERegType to, x86SSERegType from )    { CMPSSRtoR( 1 ); }
emitterT void SSE_CMPLESS_M32_to_XMM( x86SSERegType to, uptr from )         { CMPSSMtoR( 2 ); }
emitterT void SSE_CMPLESS_XMM_to_XMM( x86SSERegType to, x86SSERegType from )    { CMPSSRtoR( 2 ); }
emitterT void SSE_CMPUNORDSS_M32_to_XMM( x86SSERegType to, uptr from )      { CMPSSMtoR( 3 ); }
emitterT void SSE_CMPUNORDSS_XMM_to_XMM( x86SSERegType to, x86SSERegType from ) { CMPSSRtoR( 3 ); }
emitterT void SSE_CMPNESS_M32_to_XMM( x86SSERegType to, uptr from )         { CMPSSMtoR( 4 ); }
emitterT void SSE_CMPNESS_XMM_to_XMM( x86SSERegType to, x86SSERegType from )    { CMPSSRtoR( 4 ); }
emitterT void SSE_CMPNLTSS_M32_to_XMM( x86SSERegType to, uptr from )        { CMPSSMtoR( 5 ); }
emitterT void SSE_CMPNLTSS_XMM_to_XMM( x86SSERegType to, x86SSERegType from )   { CMPSSRtoR( 5 ); }
emitterT void SSE_CMPNLESS_M32_to_XMM( x86SSERegType to, uptr from )        { CMPSSMtoR( 6 ); }
emitterT void SSE_CMPNLESS_XMM_to_XMM( x86SSERegType to, x86SSERegType from )   { CMPSSRtoR( 6 ); }
emitterT void SSE_CMPORDSS_M32_to_XMM( x86SSERegType to, uptr from )        { CMPSSMtoR( 7 ); }
emitterT void SSE_CMPORDSS_XMM_to_XMM( x86SSERegType to, x86SSERegType from )   { CMPSSRtoR( 7 ); }

emitterT void SSE2_CMPEQSD_M64_to_XMM( x86SSERegType to, uptr from )         { CMPSDMtoR( 0 ); }
emitterT void SSE2_CMPEQSD_XMM_to_XMM( x86SSERegType to, x86SSERegType from )    { CMPSDRtoR( 0 ); }
emitterT void SSE2_CMPLTSD_M64_to_XMM( x86SSERegType to, uptr from )         { CMPSDMtoR( 1 ); }
emitterT void SSE2_CMPLTSD_XMM_to_XMM( x86SSERegType to, x86SSERegType from )    { CMPSDRtoR( 1 ); }
emitterT void SSE2_CMPLESD_M64_to_XMM( x86SSERegType to, uptr from )         { CMPSDMtoR( 2 ); }
emitterT void SSE2_CMPLESD_XMM_to_XMM( x86SSERegType to, x86SSERegType from )    { CMPSDRtoR( 2 ); }
emitterT void SSE2_CMPUNORDSD_M64_to_XMM( x86SSERegType to, uptr from )      { CMPSDMtoR( 3 ); }
emitterT void SSE2_CMPUNORDSD_XMM_to_XMM( x86SSERegType to, x86SSERegType from ) { CMPSDRtoR( 3 ); }
emitterT void SSE2_CMPNESD_M64_to_XMM( x86SSERegType to, uptr from )         { CMPSDMtoR( 4 ); }
emitterT void SSE2_CMPNESD_XMM_to_XMM( x86SSERegType to, x86SSERegType from )    { CMPSDRtoR( 4 ); }
emitterT void SSE2_CMPNLTSD_M64_to_XMM( x86SSERegType to, uptr from )        { CMPSDMtoR( 5 ); }
emitterT void SSE2_CMPNLTSD_XMM_to_XMM( x86SSERegType to, x86SSERegType from )   { CMPSDRtoR( 5 ); }
emitterT void SSE2_CMPNLESD_M64_to_XMM( x86SSERegType to, uptr from )        { CMPSDMtoR( 6 ); }
emitterT void SSE2_CMPNLESD_XMM_to_XMM( x86SSERegType to, x86SSERegType from )   { CMPSDRtoR( 6 ); }
emitterT void SSE2_CMPORDSD_M64_to_XMM( x86SSERegType to, uptr from )        { CMPSDMtoR( 7 ); }
emitterT void SSE2_CMPORDSD_XMM_to_XMM( x86SSERegType to, x86SSERegType from )   { CMPSDRtoR( 7 ); }

emitterT void SSE_UCOMISS_M32_to_XMM( x86SSERegType to, uptr from )
{
    RexR(0, to);
	write16( 0x2e0f );
	ModRM( 0, to, DISP32 );
	write32( MEMADDR(from, 4) );
}

emitterT void SSE_UCOMISS_XMM_to_XMM( x86SSERegType to, x86SSERegType from )
{
    RexRB(0, to, from);
	write16( 0x2e0f );
	ModRM( 3, to, from );
}

emitterT void SSE2_UCOMISD_M64_to_XMM( x86SSERegType to, uptr from )
{
	write8(0x66);
    RexR(0, to);
	write16( 0x2e0f );
	ModRM( 0, to, DISP32 );
	write32( MEMADDR(from, 4) );
}

emitterT void SSE2_UCOMISD_XMM_to_XMM( x86SSERegType to, x86SSERegType from )
{
	write8(0x66);
    RexRB(0, to, from);
	write16( 0x2e0f );
	ModRM( 3, to, from );
}

//////////////////////////////////////////////////////////////////////////////////////////
//**********************************************************************************/
//RSQRTPS : Packed Single-Precision FP Square Root Reciprocal                      *
//**********************************************************************************
emitterT void SSE_RSQRTPS_M128_to_XMM( x86SSERegType to, uptr from )			{ SSEMtoR( 0x520f, 0 ); }
emitterT void SSE_RSQRTPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from )	{ SSERtoR( 0x520f ); }

/////////////////////////////////////////////////////////////////////////////////////
//**********************************************************************************/
//RSQRTSS : Scalar Single-Precision FP Square Root Reciprocal                      *
//**********************************************************************************
emitterT void SSE_RSQRTSS_M32_to_XMM( x86SSERegType to, uptr from )			{ SSE_SS_MtoR( 0x520f, 0 ); }
emitterT void SSE_RSQRTSS_XMM_to_XMM( x86SSERegType to, x86SSERegType from )	{ SSE_SS_RtoR( 0x520f ); }

////////////////////////////////////////////////////////////////////////////////////
//**********************************************************************************/
//SQRTPS : Packed Single-Precision FP Square Root                                  *
//**********************************************************************************
emitterT void SSE_SQRTPS_M128_to_XMM( x86SSERegType to, uptr from )			{ SSEMtoR( 0x510f, 0 ); }
emitterT void SSE_SQRTPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from )	{ SSERtoR( 0x510f ); }

//////////////////////////////////////////////////////////////////////////////////////
//**********************************************************************************/
//SQRTSS : Scalar Single-Precision FP Square Root                                  *
//**********************************************************************************
emitterT void SSE_SQRTSS_M32_to_XMM( x86SSERegType to, uptr from )				{ SSE_SS_MtoR( 0x510f, 0 ); }
emitterT void SSE_SQRTSS_XMM_to_XMM( x86SSERegType to, x86SSERegType from )	{ SSE_SS_RtoR( 0x510f ); }

emitterT void SSE2_SQRTSD_M64_to_XMM( x86SSERegType to, uptr from )          { SSE_SD_MtoR( 0x510f, 0 ); }
emitterT void SSE2_SQRTSD_XMM_to_XMM( x86SSERegType to, x86SSERegType from ){ SSE_SD_RtoR( 0x510f ); }

////////////////////////////////////////////////////////////////////////////////////////
//**********************************************************************************/
//MAXPS: Return Packed Single-Precision FP Maximum                                 *
//**********************************************************************************
emitterT void SSE_MAXPS_M128_to_XMM( x86SSERegType to, uptr from )				{ SSEMtoR( 0x5f0f, 0 ); }
emitterT void SSE_MAXPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from )		{ SSERtoR( 0x5f0f ); }

emitterT void SSE2_MAXPD_M128_to_XMM( x86SSERegType to, uptr from )			{ SSEMtoR66( 0x5f0f ); }
emitterT void SSE2_MAXPD_XMM_to_XMM( x86SSERegType to, x86SSERegType from )	{ SSERtoR66( 0x5f0f ); }

/////////////////////////////////////////////////////////////////////////////////////////
//**********************************************************************************/
//MAXSS: Return Scalar Single-Precision FP Maximum                                 *
//**********************************************************************************
emitterT void SSE_MAXSS_M32_to_XMM( x86SSERegType to, uptr from )				{ SSE_SS_MtoR( 0x5f0f, 0 ); }
emitterT void SSE_MAXSS_XMM_to_XMM( x86SSERegType to, x86SSERegType from )		{ SSE_SS_RtoR( 0x5f0f ); }

emitterT void SSE2_MAXSD_M64_to_XMM( x86SSERegType to, uptr from )           { SSE_SD_MtoR( 0x5f0f, 0 ); }
emitterT void SSE2_MAXSD_XMM_to_XMM( x86SSERegType to, x86SSERegType from ) { SSE_SD_RtoR( 0x5f0f ); }

/////////////////////////////////////////////////////////////////////////////////////////
//**********************************************************************************/
//CVTPI2PS: Packed Signed INT32 to Packed Single  FP Conversion                    *
//**********************************************************************************
emitterT void SSE_CVTPI2PS_M64_to_XMM( x86SSERegType to, uptr from )			{ SSEMtoR( 0x2a0f, 0 ); }
emitterT void SSE_CVTPI2PS_MM_to_XMM( x86SSERegType to, x86MMXRegType from )   { SSERtoR( 0x2a0f ); }

///////////////////////////////////////////////////////////////////////////////////////////
//**********************************************************************************/
//CVTPS2PI: Packed Single FP to Packed Signed INT32 Conversion                      *
//**********************************************************************************
emitterT void SSE_CVTPS2PI_M64_to_MM( x86MMXRegType to, uptr from )			{ SSEMtoR( 0x2d0f, 0 ); }
emitterT void SSE_CVTPS2PI_XMM_to_MM( x86MMXRegType to, x86SSERegType from )   { SSERtoR( 0x2d0f ); }

emitterT void SSE_CVTTSS2SI_M32_to_R32(x86IntRegType to, uptr from) { write8(0xf3); SSEMtoR(0x2c0f, 0); }
emitterT void SSE_CVTTSS2SI_XMM_to_R32(x86IntRegType to, x86SSERegType from)
{
	write8(0xf3);
    RexRB(0, to, from);
	write16(0x2c0f);
	ModRM(3, to, from);
}

emitterT void SSE_CVTSI2SS_M32_to_XMM(x86SSERegType to, uptr from) { write8(0xf3); SSEMtoR(0x2a0f, 0); }
emitterT void SSE_CVTSI2SS_R_to_XMM(x86SSERegType to, x86IntRegType from)
{
	write8(0xf3);
    RexRB(0, to, from);
	write16(0x2a0f);
	ModRM(3, to, from);
}

emitterT void SSE2_CVTSS2SD_M32_to_XMM( x86SSERegType to, uptr from) { SSE_SS_MtoR(0x5a0f, 0); }
emitterT void SSE2_CVTSS2SD_XMM_to_XMM( x86SSERegType to, x86SSERegType from) { SSE_SS_RtoR(0x5a0f); }

emitterT void SSE2_CVTSD2SS_M64_to_XMM( x86SSERegType to, uptr from) { SSE_SD_MtoR(0x5a0f, 0); }
emitterT void SSE2_CVTSD2SS_XMM_to_XMM( x86SSERegType to, x86SSERegType from) { SSE_SD_RtoR(0x5a0f); }

///////////////////////////////////////////////////////////////////////////////////////////
//**********************************************************************************/
//CVTDQ2PS: Packed Signed INT32  to Packed Single Precision FP  Conversion         *
//**********************************************************************************
emitterT void SSE2_CVTDQ2PS_M128_to_XMM( x86SSERegType to, uptr from )			{ SSEMtoR( 0x5b0f, 0 ); }
emitterT void SSE2_CVTDQ2PS_XMM_to_XMM( x86SSERegType to, x86SSERegType from )	{ SSERtoR( 0x5b0f ); }

//**********************************************************************************/
//CVTPS2DQ: Packed Single Precision FP to Packed Signed INT32 Conversion           *
//**********************************************************************************
emitterT void SSE2_CVTPS2DQ_M128_to_XMM( x86SSERegType to, uptr from )			{ SSEMtoR66( 0x5b0f ); }
emitterT void SSE2_CVTPS2DQ_XMM_to_XMM( x86SSERegType to, x86SSERegType from )	{ SSERtoR66( 0x5b0f ); }

emitterT void SSE2_CVTTPS2DQ_XMM_to_XMM( x86SSERegType to, x86SSERegType from ){ write8(0xf3); SSERtoR(0x5b0f); }
/////////////////////////////////////////////////////////////////////////////////////
//**********************************************************************************/
//MINPS: Return Packed Single-Precision FP Minimum                                 *
//**********************************************************************************
emitterT void SSE_MINPS_M128_to_XMM( x86SSERegType to, uptr from )				{ SSEMtoR( 0x5d0f, 0 ); }
emitterT void SSE_MINPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from )		{ SSERtoR( 0x5d0f ); }

emitterT void SSE2_MINPD_M128_to_XMM( x86SSERegType to, uptr from )			{ SSEMtoR66( 0x5d0f ); }
emitterT void SSE2_MINPD_XMM_to_XMM( x86SSERegType to, x86SSERegType from )	{ SSERtoR66( 0x5d0f ); }

//////////////////////////////////////////////////////////////////////////////////////////
//**********************************************************************************/
//MINSS: Return Scalar Single-Precision FP Minimum                                 *
//**********************************************************************************
emitterT void SSE_MINSS_M32_to_XMM( x86SSERegType to, uptr from )				{ SSE_SS_MtoR( 0x5d0f, 0 ); }
emitterT void SSE_MINSS_XMM_to_XMM( x86SSERegType to, x86SSERegType from )		{ SSE_SS_RtoR( 0x5d0f ); }

emitterT void SSE2_MINSD_M64_to_XMM( x86SSERegType to, uptr from )           { SSE_SD_MtoR( 0x5d0f, 0 ); }
emitterT void SSE2_MINSD_XMM_to_XMM( x86SSERegType to, x86SSERegType from ) { SSE_SD_RtoR( 0x5d0f ); }

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

//////////////////////////////////////////////////////////////////////////////////////
//**********************************************************************************/
//SHUFPS: Shuffle Packed Single-Precision FP Values                                *
//**********************************************************************************
emitterT void SSE_SHUFPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from, u8 imm8 )	{ SSERtoR( 0xC60F ); write8( imm8 ); }
emitterT void SSE_SHUFPS_M128_to_XMM( x86SSERegType to, uptr from, u8 imm8 )			{ SSEMtoR( 0xC60F, 1 ); write8( imm8 ); }

emitterT void SSE_SHUFPS_Rm_to_XMM( x86SSERegType to, x86IntRegType from, int offset, u8 imm8 )
{
    RexRB(0, to, from);
	write16(0xc60f);
    WriteRmOffsetFrom(to, from, offset);
	write8(imm8);
}

//////////////////////////////////////////////////////////////////////////////////////
//**********************************************************************************/
//SHUFPD: Shuffle Packed Double-Precision FP Values                                *
//**********************************************************************************
emitterT void SSE2_SHUFPD_XMM_to_XMM( x86SSERegType to, x86SSERegType from, u8 imm8 )	{ SSERtoR66( 0xC60F ); write8( imm8 ); }
emitterT void SSE2_SHUFPD_M128_to_XMM( x86SSERegType to, uptr from, u8 imm8 )			{ SSEMtoR66( 0xC60F ); write8( imm8 ); }

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

////////////////////////////////////////////////////////////////////////////////////////
//**********************************************************************************/
//DIVPS : Packed Single-Precision FP Divide                                       *
//**********************************************************************************
emitterT void SSE_DIVPS_M128_to_XMM( x86SSERegType to, uptr from )				{ SSEMtoR( 0x5e0F, 0 ); }
emitterT void SSE_DIVPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from )		{ SSERtoR( 0x5e0F ); }

//////////////////////////////////////////////////////////////////////////////////////
//**********************************************************************************/
//DIVSS : Scalar  Single-Precision FP Divide                                       *
//**********************************************************************************
emitterT void SSE_DIVSS_M32_to_XMM( x86SSERegType to, uptr from )				{ SSE_SS_MtoR( 0x5e0F, 0 ); }
emitterT void SSE_DIVSS_XMM_to_XMM( x86SSERegType to, x86SSERegType from )		{ SSE_SS_RtoR( 0x5e0F ); }

emitterT void SSE2_DIVSD_M64_to_XMM( x86SSERegType to, uptr from )           { SSE_SD_MtoR( 0x5e0F, 0 ); }
emitterT void SSE2_DIVSD_XMM_to_XMM( x86SSERegType to, x86SSERegType from ) { SSE_SD_RtoR( 0x5e0F ); }

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

/////////////////////////////////////////////////////////////////////////////////////
//**********************************************************************************/
//PADDB,PADDW,PADDD : Add Packed Integers                                          *
//**********************************************************************************
emitterT void SSE2_PADDB_XMM_to_XMM(x86SSERegType to, x86SSERegType from ){ SSERtoR66( 0xFC0F ); }
emitterT void SSE2_PADDB_M128_to_XMM(x86SSERegType to, uptr from ){ SSEMtoR66( 0xFC0F ); }
emitterT void SSE2_PADDW_XMM_to_XMM(x86SSERegType to, x86SSERegType from ){ SSERtoR66( 0xFD0F ); }
emitterT void SSE2_PADDW_M128_to_XMM(x86SSERegType to, uptr from ){ SSEMtoR66( 0xFD0F ); }
emitterT void SSE2_PADDD_XMM_to_XMM(x86SSERegType to, x86SSERegType from ){ SSERtoR66( 0xFE0F ); }
emitterT void SSE2_PADDD_M128_to_XMM(x86SSERegType to, uptr from ){ SSEMtoR66( 0xFE0F ); }
emitterT void SSE2_PADDQ_XMM_to_XMM(x86SSERegType to, x86SSERegType from ) { SSERtoR66( 0xD40F ); }
emitterT void SSE2_PADDQ_M128_to_XMM(x86SSERegType to, uptr from ) { SSEMtoR66( 0xD40F ); }

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

////////////////////////////////////////////////////////////////////////////////////////////
//**********************************************************************************/
//PSUBx: Subtract Packed Integers                                                  *
//**********************************************************************************
emitterT void SSE2_PSUBB_XMM_to_XMM(x86SSERegType to, x86SSERegType from ){ SSERtoR66( 0xF80F ); }
emitterT void SSE2_PSUBB_M128_to_XMM(x86SSERegType to, uptr from ){ SSEMtoR66( 0xF80F ); }
emitterT void SSE2_PSUBW_XMM_to_XMM(x86SSERegType to, x86SSERegType from ){ SSERtoR66( 0xF90F ); }
emitterT void SSE2_PSUBW_M128_to_XMM(x86SSERegType to, uptr from ){ SSEMtoR66( 0xF90F ); }
emitterT void SSE2_PSUBD_XMM_to_XMM(x86SSERegType to, x86SSERegType from ){ SSERtoR66( 0xFA0F ); }
emitterT void SSE2_PSUBD_M128_to_XMM(x86SSERegType to, uptr from ){ SSEMtoR66( 0xFA0F ); }
emitterT void SSE2_PSUBQ_XMM_to_XMM(x86SSERegType to, x86SSERegType from ){ SSERtoR66( 0xFB0F ); }
emitterT void SSE2_PSUBQ_M128_to_XMM(x86SSERegType to, uptr from ){ SSEMtoR66( 0xFB0F ); }

///////////////////////////////////////////////////////////////////////////////////////
//**********************************************************************************/
//MOVD: Move Dword(32bit) to /from XMM reg                                         *
//**********************************************************************************
emitterT void SSE2_MOVD_M32_to_XMM( x86SSERegType to, uptr from )			{ SSEMtoR66(0x6E0F); }
emitterT void SSE2_MOVD_R_to_XMM( x86SSERegType to, x86IntRegType from )	{ SSERtoR66(0x6E0F); }

emitterT void SSE2_MOVD_Rm_to_XMM( x86SSERegType to, x86IntRegType from )
{
	write8(0x66);
	RexRB(0, to, from);
	write16( 0x6e0f );
	ModRM( 0, to, from);
}

emitterT void SSE2_MOVD_Rm_to_XMM( x86SSERegType to, x86IntRegType from, int offset )
{
	write8(0x66);
    RexRB(0, to, from);
	write16( 0x6e0f );
    WriteRmOffsetFrom(to, from, offset);
}

emitterT void SSE2_MOVD_XMM_to_M32( u32 to, x86SSERegType from )			{ SSERtoM66(0x7E0F); }
emitterT void SSE2_MOVD_XMM_to_R( x86IntRegType to, x86SSERegType from )	{ _SSERtoR66(0x7E0F); }

emitterT void SSE2_MOVD_XMM_to_Rm( x86IntRegType to, x86SSERegType from, int offset )
{
	write8(0x66);
    RexRB(0, from, to);
	write16( 0x7e0f );
    WriteRmOffsetFrom(from, to, offset);
}

////////////////////////////////////////////////////////////////////////////////////
//**********************************************************************************/
//POR : SSE Bitwise OR                                                             *
//**********************************************************************************
emitterT void SSE2_POR_XMM_to_XMM( x86SSERegType to, x86SSERegType from )		{ SSERtoR66( 0xEB0F ); }
emitterT void SSE2_POR_M128_to_XMM( x86SSERegType to, uptr from )				{ SSEMtoR66( 0xEB0F ); }

// logical and to &= from
emitterT void SSE2_PAND_XMM_to_XMM( x86SSERegType to, x86SSERegType from )		{ SSERtoR66( 0xDB0F ); }
emitterT void SSE2_PAND_M128_to_XMM( x86SSERegType to, uptr from )				{ SSEMtoR66( 0xDB0F ); }

// to = (~to) & from
emitterT void SSE2_PANDN_XMM_to_XMM( x86SSERegType to, x86SSERegType from )	{ SSERtoR66( 0xDF0F ); }
emitterT void SSE2_PANDN_M128_to_XMM( x86SSERegType to, uptr from )			{ SSEMtoR66( 0xDF0F ); }

/////////////////////////////////////////////////////////////////////////////////////
//**********************************************************************************/
//PXOR : SSE Bitwise XOR                                                             *
//**********************************************************************************
emitterT void SSE2_PXOR_XMM_to_XMM( x86SSERegType to, x86SSERegType from )		{ SSERtoR66( 0xEF0F ); }
emitterT void SSE2_PXOR_M128_to_XMM( x86SSERegType to, uptr from )				{ SSEMtoR66( 0xEF0F ); }
///////////////////////////////////////////////////////////////////////////////////////

emitterT void SSE2_MOVDQA_M128_to_XMM(x86SSERegType to, uptr from)				{ if( AlwaysUseMovaps ) SSE_MOVAPS_M128_to_XMM( to, from ); else SSEMtoR66(0x6F0F); }
emitterT void SSE2_MOVDQA_XMM_to_M128( uptr to, x86SSERegType from )			{ if( AlwaysUseMovaps ) SSE_MOVAPS_XMM_to_M128( to, from ); else SSERtoM66(0x7F0F); } 
emitterT void SSE2_MOVDQA_XMM_to_XMM( x86SSERegType to, x86SSERegType from)	{ if( AlwaysUseMovaps ) SSE_MOVAPS_XMM_to_XMM( to, from ); else if( to != from ) SSERtoR66(0x6F0F); }

emitterT void SSE2_MOVDQU_M128_to_XMM(x86SSERegType to, uptr from)
{
	if( AlwaysUseMovaps )
		SSE_MOVUPS_M128_to_XMM( to, from );
	else
	{
		write8(0xF3);
		SSEMtoR(0x6F0F, 0);
	}
}
emitterT void SSE2_MOVDQU_XMM_to_M128( uptr to, x86SSERegType from)
{
	if( AlwaysUseMovaps )
		SSE_MOVUPS_XMM_to_M128( to, from );
	else
	{
		write8(0xF3);
		SSERtoM(0x7F0F, 0);
	}
}

// shift right logical

emitterT void SSE2_PSRLW_XMM_to_XMM(x86SSERegType to, x86SSERegType from)	{ SSERtoR66(0xD10F); }
emitterT void SSE2_PSRLW_M128_to_XMM(x86SSERegType to, uptr from)			{ SSEMtoR66(0xD10F); }
emitterT void SSE2_PSRLW_I8_to_XMM(x86SSERegType to, u8 imm8)
{
	write8( 0x66 );
    RexB(0, to);
	write16( 0x710F );
	ModRM( 3, 2 , to );
	write8( imm8 );
}

emitterT void SSE2_PSRLD_XMM_to_XMM(x86SSERegType to, x86SSERegType from)	{ SSERtoR66(0xD20F); }
emitterT void SSE2_PSRLD_M128_to_XMM(x86SSERegType to, uptr from)			{ SSEMtoR66(0xD20F); }
emitterT void SSE2_PSRLD_I8_to_XMM(x86SSERegType to, u8 imm8)
{
	write8( 0x66 );
    RexB(0, to);
	write16( 0x720F );
	ModRM( 3, 2 , to ); 
	write8( imm8 );
}

emitterT void SSE2_PSRLQ_XMM_to_XMM(x86SSERegType to, x86SSERegType from)	{ SSERtoR66(0xD30F); }
emitterT void SSE2_PSRLQ_M128_to_XMM(x86SSERegType to, uptr from)			{ SSEMtoR66(0xD30F); }
emitterT void SSE2_PSRLQ_I8_to_XMM(x86SSERegType to, u8 imm8)
{
	write8( 0x66 );
    RexB(0, to);
	write16( 0x730F );
	ModRM( 3, 2 , to ); 
	write8( imm8 );
}

emitterT void SSE2_PSRLDQ_I8_to_XMM(x86SSERegType to, u8 imm8)
{
	write8( 0x66 );
    RexB(0, to);
	write16( 0x730F );
	ModRM( 3, 3 , to ); 
	write8( imm8 );
}

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

// shift left logical

emitterT void SSE2_PSLLW_XMM_to_XMM(x86SSERegType to, x86SSERegType from)	{ SSERtoR66(0xF10F); }
emitterT void SSE2_PSLLW_M128_to_XMM(x86SSERegType to, uptr from)			{ SSEMtoR66(0xF10F); }
emitterT void SSE2_PSLLW_I8_to_XMM(x86SSERegType to, u8 imm8)
{
	write8( 0x66 );
    RexB(0, to);
	write16( 0x710F );
	ModRM( 3, 6 , to );
	write8( imm8 );
}

emitterT void SSE2_PSLLD_XMM_to_XMM(x86SSERegType to, x86SSERegType from)	{ SSERtoR66(0xF20F); }
emitterT void SSE2_PSLLD_M128_to_XMM(x86SSERegType to, uptr from)			{ SSEMtoR66(0xF20F); }
emitterT void SSE2_PSLLD_I8_to_XMM(x86SSERegType to, u8 imm8)
{
	write8( 0x66 );
    RexB(0, to);
	write16( 0x720F );
	ModRM( 3, 6 , to );
	write8( imm8 );
}

emitterT void SSE2_PSLLQ_XMM_to_XMM(x86SSERegType to, x86SSERegType from)	{ SSERtoR66(0xF30F); }
emitterT void SSE2_PSLLQ_M128_to_XMM(x86SSERegType to, uptr from)			{ SSEMtoR66(0xF30F); }
emitterT void SSE2_PSLLQ_I8_to_XMM(x86SSERegType to, u8 imm8)
{
	write8( 0x66 );
    RexB(0, to);
	write16( 0x730F );
	ModRM( 3, 6 , to );
	write8( imm8 );
}

emitterT void SSE2_PSLLDQ_I8_to_XMM(x86SSERegType to, u8 imm8)
{
	write8( 0x66 );
    RexB(0, to);
	write16( 0x730F );
	ModRM( 3, 7 , to ); 
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

emitterT void SSE2_PADDSB_XMM_to_XMM( x86SSERegType to, x86SSERegType from )	{ SSERtoR66( 0xEC0F ); }
emitterT void SSE2_PADDSB_M128_to_XMM( x86SSERegType to, uptr from )			{ SSEMtoR66( 0xEC0F ); }

emitterT void SSE2_PADDSW_XMM_to_XMM( x86SSERegType to, x86SSERegType from )	{ SSERtoR66( 0xED0F ); }
emitterT void SSE2_PADDSW_M128_to_XMM( x86SSERegType to, uptr from )			{ SSEMtoR66( 0xED0F ); }

emitterT void SSE2_PSUBSB_XMM_to_XMM( x86SSERegType to, x86SSERegType from )	{ SSERtoR66( 0xE80F ); }
emitterT void SSE2_PSUBSB_M128_to_XMM( x86SSERegType to, uptr from )			{ SSEMtoR66( 0xE80F ); }

emitterT void SSE2_PSUBSW_XMM_to_XMM( x86SSERegType to, x86SSERegType from )	{ SSERtoR66( 0xE90F ); }
emitterT void SSE2_PSUBSW_M128_to_XMM( x86SSERegType to, uptr from )			{ SSEMtoR66( 0xE90F ); }

emitterT void SSE2_PSUBUSB_XMM_to_XMM( x86SSERegType to, x86SSERegType from )	{ SSERtoR66( 0xD80F ); }
emitterT void SSE2_PSUBUSB_M128_to_XMM( x86SSERegType to, uptr from )			{ SSEMtoR66( 0xD80F ); }
emitterT void SSE2_PSUBUSW_XMM_to_XMM( x86SSERegType to, x86SSERegType from )	{ SSERtoR66( 0xD90F ); }
emitterT void SSE2_PSUBUSW_M128_to_XMM( x86SSERegType to, uptr from )			{ SSEMtoR66( 0xD90F ); }

emitterT void SSE2_PADDUSB_XMM_to_XMM( x86SSERegType to, x86SSERegType from )	{ SSERtoR66( 0xDC0F ); }
emitterT void SSE2_PADDUSB_M128_to_XMM( x86SSERegType to, uptr from )			{ SSEMtoR66( 0xDC0F ); }
emitterT void SSE2_PADDUSW_XMM_to_XMM( x86SSERegType to, x86SSERegType from )	{ SSERtoR66( 0xDD0F ); }
emitterT void SSE2_PADDUSW_M128_to_XMM( x86SSERegType to, uptr from )			{ SSEMtoR66( 0xDD0F ); }

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

emitterT void SSE2_PMOVMSKB_XMM_to_R32(x86IntRegType to, x86SSERegType from)	{ SSERtoR66(0xD70F); }

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
// This header should always be included *after* ix86.h.

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
