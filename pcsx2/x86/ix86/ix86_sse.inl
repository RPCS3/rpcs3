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

#pragma once

//------------------------------------------------------------------
// SSE instructions
//------------------------------------------------------------------

// This tells the recompiler's emitter to always use movaps instead of movdqa.  Both instructions
// do the exact same thing, but movaps is 1 byte shorter, and thus results in a cleaner L1 cache
// and some marginal speed gains as a result.  (it's possible someday in the future the per-
// formance of the two instructions could change, so this constant is provided to restore MOVDQA
// use easily at a later time, if needed).

static const bool AlwaysUseMovaps = true;

#define SSEMtoR( code, overb ) \
	assert( to < XMMREGS ), \
	RexR(0, to),             \
	write16<I>( code ), \
	ModRM<I>( 0, to, DISP32 ), \
	write32<I>( MEMADDR(from, 4 + overb) )

#define SSERtoM( code, overb ) \
	assert( from < XMMREGS), \
    RexR(0, from),  \
	write16<I>( code ), \
	ModRM<I>( 0, from, DISP32 ), \
	write32<I>( MEMADDR(to, 4 + overb) )

#define SSE_SS_MtoR( code, overb ) \
	assert( to < XMMREGS ), \
	write8<I>( 0xf3 ), \
    RexR(0, to),                      \
	write16<I>( code ), \
	ModRM<I>( 0, to, DISP32 ), \
	write32<I>( MEMADDR(from, 4 + overb) )

#define SSE_SS_RtoM( code, overb ) \
	assert( from < XMMREGS), \
	write8<I>( 0xf3 ), \
	RexR(0, from), \
	write16<I>( code ), \
	ModRM<I>( 0, from, DISP32 ), \
	write32<I>( MEMADDR(to, 4 + overb) )

#define SSERtoR( code ) \
	assert( to < XMMREGS && from < XMMREGS), \
    RexRB(0, to, from),            \
	write16<I>( code ), \
	ModRM<I>( 3, to, from )

#define SSEMtoR66( code ) \
	write8<I>( 0x66 ), \
	SSEMtoR( code, 0 )

#define SSERtoM66( code ) \
	write8<I>( 0x66 ), \
	SSERtoM( code, 0 )

#define SSERtoR66( code ) \
	write8<I>( 0x66 ), \
	SSERtoR( code )

#define _SSERtoR66( code ) \
	assert( to < XMMREGS && from < XMMREGS), \
	write8<I>( 0x66 ), \
	RexRB(0, from, to), \
	write16<I>( code ), \
	ModRM<I>( 3, from, to )

#define SSE_SS_RtoR( code ) \
	assert( to < XMMREGS && from < XMMREGS), \
	write8<I>( 0xf3 ), \
    RexRB(0, to, from),              \
	write16<I>( code ), \
	ModRM<I>( 3, to, from )

#define CMPPSMtoR( op ) \
   SSEMtoR( 0xc20f, 1 ), \
   write8<I>( op )

#define CMPPSRtoR( op ) \
   SSERtoR( 0xc20f ), \
   write8<I>( op )

#define CMPSSMtoR( op ) \
   SSE_SS_MtoR( 0xc20f, 1 ), \
   write8<I>( op )

#define CMPSSRtoR( op ) \
   SSE_SS_RtoR( 0xc20f ), \
   write8<I>( op )

/* movups [r32][r32*scale] to xmm1 */
emitterT void eSSE_MOVUPSRmStoR( x86SSERegType to, x86IntRegType from, x86IntRegType from2, int scale )
{
    RexRXB(0, to, from2, from);
	write16<I>( 0x100f );
	ModRM<I>( 0, to, 0x4 );
	SibSB<I>( scale, from2, from );
}

/* movups xmm1 to [r32][r32*scale] */
emitterT void eSSE_MOVUPSRtoRmS( x86SSERegType to, x86IntRegType from, x86IntRegType from2, int scale )
{
    RexRXB(1, to, from2, from);
	write16<I>( 0x110f );
	ModRM<I>( 0, to, 0x4 );
	SibSB<I>( scale, from2, from );
}

/* movups [r32] to r32 */
emitterT void eSSE_MOVUPSRmtoR( x86IntRegType to, x86IntRegType from ) 
{
	RexRB(0, to, from);
	write16<I>( 0x100f );
	ModRM<I>( 0, to, from );
}

/* movups r32 to [r32] */
emitterT void eSSE_MOVUPSRtoRm( x86IntRegType to, x86IntRegType from ) 
{
    RexRB(0, from, to);
	write16<I>( 0x110f );
	ModRM<I>( 0, from, to );
}

/* movlps [r32] to r32 */
emitterT void eSSE_MOVLPSRmtoR( x86SSERegType to, x86IntRegType from ) 
{
	RexRB(1, to, from);
	write16<I>( 0x120f );
	ModRM<I>( 0, to, from );
}

emitterT void eSSE_MOVLPSRmtoROffset( x86SSERegType to, x86IntRegType from, int offset )
{
    RexRB(0, to, from);
	write16<I>( 0x120f );
    WriteRmOffsetFrom<I>(to, from, offset);
}

/* movaps r32 to [r32] */
emitterT void eSSE_MOVLPSRtoRm( x86IntRegType to, x86IntRegType from ) 
{
    RexRB(0, from, to);
	write16<I>( 0x130f );
	ModRM<I>( 0, from, to );
}

emitterT void eSSE_MOVLPSRtoRmOffset( x86SSERegType to, x86IntRegType from, int offset )
{
    RexRB(0, from, to);
	write16<I>( 0x130f );
    WriteRmOffsetFrom<I>(from, to, offset);
}

/* movaps [r32][r32*scale] to xmm1 */
emitterT void eSSE_MOVAPSRmStoR( x86SSERegType to, x86IntRegType from, x86IntRegType from2, int scale )
{
	assert( from != EBP );
    RexRXB(0, to, from2, from);
	write16<I>( 0x280f );
	ModRM<I>( 0, to, 0x4 );
	SibSB<I>( scale, from2, from );
}

/* movaps xmm1 to [r32][r32*scale] */
emitterT void eSSE_MOVAPSRtoRmS( x86SSERegType to, x86IntRegType from, x86IntRegType from2, int scale )
{
	assert( from != EBP );
    RexRXB(0, to, from2, from);
	write16<I>( 0x290f );
	ModRM<I>( 0, to, 0x4 );
	SibSB<I>( scale, from2, from );
}

// movaps [r32+offset] to r32
emitterT void eSSE_MOVAPSRmtoROffset( x86SSERegType to, x86IntRegType from, int offset )
{
	RexRB(0, to, from);
	write16<I>( 0x280f );
    WriteRmOffsetFrom<I>(to, from, offset);
}

// movaps r32 to [r32+offset]
emitterT void eSSE_MOVAPSRtoRmOffset( x86IntRegType to, x86SSERegType from, int offset ) 
{
	RexRB(0, from, to);
	write16<I>( 0x290f );
    WriteRmOffsetFrom<I>(from, to, offset);
}

// movdqa [r32+offset] to r32
emitterT void eSSE2_MOVDQARmtoROffset( x86SSERegType to, x86IntRegType from, int offset )
{
	if( AlwaysUseMovaps )
		eSSE_MOVAPSRmtoROffset<I>( to, from, offset );
	else
	{
		write8<I>(0x66);
		RexRB(0, to, from);
		write16<I>( 0x6f0f );
		WriteRmOffsetFrom<I>(to, from, offset);
	}
}

// movdqa r32 to [r32+offset]
emitterT void eSSE2_MOVDQARtoRmOffset( x86IntRegType to, x86SSERegType from, int offset ) 
{
	if( AlwaysUseMovaps )
		eSSE_MOVAPSRtoRmOffset<I>( to, from, offset );
	else
	{
		write8<I>(0x66);
		RexRB(0, from, to);
		write16<I>( 0x7f0f );
		WriteRmOffsetFrom<I>(from, to, offset);
	}
}

// movups [r32+offset] to r32
emitterT void eSSE_MOVUPSRmtoROffset( x86SSERegType to, x86IntRegType from, int offset )
{
	RexRB(0, to, from);
	write16<I>( 0x100f );
    WriteRmOffsetFrom<I>(to, from, offset);
}

// movups r32 to [r32+offset]
emitterT void eSSE_MOVUPSRtoRmOffset( x86SSERegType to, x86IntRegType from, int offset )
{
    RexRB(0, from, to);
	write16<I>( 0x110f );
    WriteRmOffsetFrom<I>(from, to, offset);
}

//**********************************************************************************/
//MOVAPS: Move aligned Packed Single Precision FP values                           *
//**********************************************************************************
emitterT void eSSE_MOVAPS_M128_to_XMM( x86SSERegType to, uptr from )          { SSEMtoR( 0x280f, 0 ); }
emitterT void eSSE_MOVAPS_XMM_to_M128( uptr to, x86SSERegType from )          { SSERtoM( 0x290f, 0 ); }
emitterT void eSSE_MOVAPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from )  { if (to != from) { SSERtoR( 0x280f ); } }

emitterT void eSSE_MOVUPS_M128_to_XMM( x86SSERegType to, uptr from )          { SSEMtoR( 0x100f, 0 ); }
emitterT void eSSE_MOVUPS_XMM_to_M128( uptr to, x86SSERegType from )          { SSERtoM( 0x110f, 0 ); }

emitterT void eSSE2_MOVSD_XMM_to_XMM( x86SSERegType to, x86SSERegType from )
{
	write8<I>(0xf2);
	SSERtoR( 0x100f);
}

emitterT void eSSE2_MOVQ_M64_to_XMM( x86SSERegType to, uptr from )
{
	write8<I>(0xf3); SSEMtoR( 0x7e0f, 0);
}

emitterT void eSSE2_MOVQ_XMM_to_XMM( x86SSERegType to, x86SSERegType from )
{
	write8<I>(0xf3); SSERtoR( 0x7e0f);
}

emitterT void eSSE2_MOVQ_XMM_to_M64( u32 to, x86SSERegType from )
{
	SSERtoM66(0xd60f);
}

emitterT void eSSE2_MOVDQ2Q_XMM_to_MM( x86MMXRegType to, x86SSERegType from)
{
	write8<I>(0xf2);
	SSERtoR( 0xd60f);
}
emitterT void eSSE2_MOVQ2DQ_MM_to_XMM( x86SSERegType to, x86MMXRegType from)
{
	write8<I>(0xf3);
	SSERtoR( 0xd60f);
}

//**********************************************************************************/
//MOVSS: Move Scalar Single-Precision FP  value                                    *
//**********************************************************************************
emitterT void eSSE_MOVSS_M32_to_XMM( x86SSERegType to, uptr from )			{ SSE_SS_MtoR( 0x100f, 0 ); }
emitterT void eSSE_MOVSS_XMM_to_M32( u32 to, x86SSERegType from )			{ SSE_SS_RtoM( 0x110f, 0 ); }
emitterT void eSSE_MOVSS_XMM_to_Rm( x86IntRegType to, x86SSERegType from )
{
	write8<I>(0xf3);
    RexRB(0, from, to);
    write16<I>(0x110f);
	ModRM<I>(0, from, to);
}

emitterT void eSSE_MOVSS_XMM_to_XMM( x86SSERegType to, x86SSERegType from )	{ if (to != from) { SSE_SS_RtoR( 0x100f ); } }

emitterT void eSSE_MOVSS_RmOffset_to_XMM( x86SSERegType to, x86IntRegType from, int offset )
{
	write8<I>(0xf3);
    RexRB(0, to, from);
    write16<I>( 0x100f );
    WriteRmOffsetFrom<I>(to, from, offset);
}

emitterT void eSSE_MOVSS_XMM_to_RmOffset( x86IntRegType to, x86SSERegType from, int offset )
{
	write8<I>(0xf3);
    RexRB(0, from, to);
    write16<I>(0x110f);
    WriteRmOffsetFrom<I>(from, to, offset);
}

emitterT void eSSE_MASKMOVDQU_XMM_to_XMM( x86SSERegType to, x86SSERegType from )	{ SSERtoR66( 0xf70f ); }
//**********************************************************************************/
//MOVLPS: Move low Packed Single-Precision FP                                     *
//**********************************************************************************
emitterT void eSSE_MOVLPS_M64_to_XMM( x86SSERegType to, uptr from )	{ SSEMtoR( 0x120f, 0 ); }
emitterT void eSSE_MOVLPS_XMM_to_M64( u32 to, x86SSERegType from )	{ SSERtoM( 0x130f, 0 ); }

emitterT void eSSE_MOVLPS_RmOffset_to_XMM( x86SSERegType to, x86IntRegType from, int offset )
{
    RexRB(0, to, from);
	write16<I>( 0x120f );
    WriteRmOffsetFrom<I>(to, from, offset);
}

emitterT void eSSE_MOVLPS_XMM_to_RmOffset( x86IntRegType to, x86SSERegType from, int offset )
{
    RexRB(0, from, to);
	write16<I>(0x130f);
    WriteRmOffsetFrom<I>(from, to, offset);
}

/////////////////////////////////////////////////////////////////////////////////////
//**********************************************************************************/
//MOVHPS: Move High Packed Single-Precision FP                                     *
//**********************************************************************************
emitterT void eSSE_MOVHPS_M64_to_XMM( x86SSERegType to, uptr from )	{ SSEMtoR( 0x160f, 0 ); }
emitterT void eSSE_MOVHPS_XMM_to_M64( u32 to, x86SSERegType from )	{ SSERtoM( 0x170f, 0 ); }

emitterT void eSSE_MOVHPS_RmOffset_to_XMM( x86SSERegType to, x86IntRegType from, int offset )
{
    RexRB(0, to, from);
	write16<I>( 0x160f );
    WriteRmOffsetFrom<I>(to, from, offset);
}

emitterT void eSSE_MOVHPS_XMM_to_RmOffset( x86IntRegType to, x86SSERegType from, int offset )
{
    RexRB(0, from, to);
	write16<I>(0x170f);
    WriteRmOffsetFrom<I>(from, to, offset);
}

/////////////////////////////////////////////////////////////////////////////////////
//**********************************************************************************/
//MOVLHPS: Moved packed Single-Precision FP low to high                            *
//**********************************************************************************
emitterT void eSSE_MOVLHPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from )	{ SSERtoR( 0x160f ); }

//////////////////////////////////////////////////////////////////////////////////////
//**********************************************************************************/
//MOVHLPS: Moved packed Single-Precision FP High to Low                            *
//**********************************************************************************
emitterT void eSSE_MOVHLPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from )	{ SSERtoR( 0x120f ); }

///////////////////////////////////////////////////////////////////////////////////
//**********************************************************************************/
//ANDPS: Logical Bit-wise  AND for Single FP                                        *
//**********************************************************************************
emitterT void eSSE_ANDPS_M128_to_XMM( x86SSERegType to, uptr from )			{ SSEMtoR( 0x540f, 0 ); }
emitterT void eSSE_ANDPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from )	{ SSERtoR( 0x540f ); }

///////////////////////////////////////////////////////////////////////////////////////
//**********************************************************************************/
//ANDNPS : Logical Bit-wise  AND NOT of Single-precision FP values                 *
//**********************************************************************************
emitterT void eSSE_ANDNPS_M128_to_XMM( x86SSERegType to, uptr from )		{ SSEMtoR( 0x550f, 0 ); }
emitterT void eSSE_ANDNPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from ){ SSERtoR( 0x550f ); }

/////////////////////////////////////////////////////////////////////////////////////
//**********************************************************************************/
//RCPPS : Packed Single-Precision FP Reciprocal                                     *
//**********************************************************************************
emitterT void eSSE_RCPPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from )	{ SSERtoR( 0x530f ); }
emitterT void eSSE_RCPPS_M128_to_XMM( x86SSERegType to, uptr from )			{ SSEMtoR( 0x530f, 0 ); }

emitterT void eSSE_RCPSS_XMM_to_XMM( x86SSERegType to, x86SSERegType from ) { SSE_SS_RtoR(0x530f); }
emitterT void eSSE_RCPSS_M32_to_XMM( x86SSERegType to, uptr from )			{ SSE_SS_MtoR(0x530f, 0); }

//////////////////////////////////////////////////////////////////////////////////////
//**********************************************************************************/
//ORPS : Bit-wise Logical OR of Single-Precision FP Data                            *
//**********************************************************************************
emitterT void eSSE_ORPS_M128_to_XMM( x86SSERegType to, uptr from )				{ SSEMtoR( 0x560f, 0 ); }
emitterT void eSSE_ORPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from )		{ SSERtoR( 0x560f ); }

/////////////////////////////////////////////////////////////////////////////////////
//**********************************************************************************/
//XORPS : Bitwise Logical XOR of Single-Precision FP Values                        *
//**********************************************************************************
emitterT void eSSE_XORPS_M128_to_XMM( x86SSERegType to, uptr from )				{ SSEMtoR( 0x570f, 0 ); }
emitterT void eSSE_XORPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from )		{ SSERtoR( 0x570f ); }

///////////////////////////////////////////////////////////////////////////////////////
//**********************************************************************************/
//ADDPS : ADD Packed Single-Precision FP Values                                    *
//**********************************************************************************
emitterT void eSSE_ADDPS_M128_to_XMM( x86SSERegType to, uptr from )				{ SSEMtoR( 0x580f, 0 ); }
emitterT void eSSE_ADDPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from )		{ SSERtoR( 0x580f ); }

////////////////////////////////////////////////////////////////////////////////////
//**********************************************************************************/
//ADDSS : ADD Scalar Single-Precision FP Values                                    *
//**********************************************************************************
emitterT void eSSE_ADDSS_M32_to_XMM( x86SSERegType to, uptr from )				{ SSE_SS_MtoR( 0x580f, 0 ); }
emitterT void eSSE_ADDSS_XMM_to_XMM( x86SSERegType to, x86SSERegType from )		{ SSE_SS_RtoR( 0x580f ); }

/////////////////////////////////////////////////////////////////////////////////////////
//**********************************************************************************/
//SUBPS: Packed Single-Precision FP Subtract                                       *
//**********************************************************************************
emitterT void eSSE_SUBPS_M128_to_XMM( x86SSERegType to, uptr from )				{ SSEMtoR( 0x5c0f, 0 ); }
emitterT void eSSE_SUBPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from )		{ SSERtoR( 0x5c0f ); }

///////////////////////////////////////////////////////////////////////////////////////
//**********************************************************************************/
//SUBSS : Scalar  Single-Precision FP Subtract                                       *
//**********************************************************************************
emitterT void eSSE_SUBSS_M32_to_XMM( x86SSERegType to, uptr from )				{ SSE_SS_MtoR( 0x5c0f, 0 ); }
emitterT void eSSE_SUBSS_XMM_to_XMM( x86SSERegType to, x86SSERegType from )		{ SSE_SS_RtoR( 0x5c0f ); }

/////////////////////////////////////////////////////////////////////////////////////////
//**********************************************************************************/
//MULPS : Packed Single-Precision FP Multiply                                      *
//**********************************************************************************
emitterT void eSSE_MULPS_M128_to_XMM( x86SSERegType to, uptr from )				{ SSEMtoR( 0x590f, 0 ); }
emitterT void eSSE_MULPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from )		{ SSERtoR( 0x590f ); }

////////////////////////////////////////////////////////////////////////////////////////
//**********************************************************************************/
//MULSS : Scalar  Single-Precision FP Multiply                                       *
//**********************************************************************************
emitterT void eSSE_MULSS_M32_to_XMM( x86SSERegType to, uptr from )				{ SSE_SS_MtoR( 0x590f, 0 ); }
emitterT void eSSE_MULSS_XMM_to_XMM( x86SSERegType to, x86SSERegType from )		{ SSE_SS_RtoR( 0x590f ); }

////////////////////////////////////////////////////////////////////////////////////////////
//**********************************************************************************/
//Packed Single-Precission FP compare (CMPccPS)                                    *
//**********************************************************************************
//missing  SSE_CMPPS_I8_to_XMM
//         SSE_CMPPS_M32_to_XMM
//	       SSE_CMPPS_XMM_to_XMM
emitterT void eSSE_CMPEQPS_M128_to_XMM( x86SSERegType to, uptr from )         { CMPPSMtoR( 0 ); }
emitterT void eSSE_CMPEQPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from )    { CMPPSRtoR( 0 ); }
emitterT void eSSE_CMPLTPS_M128_to_XMM( x86SSERegType to, uptr from )         { CMPPSMtoR( 1 ); }
emitterT void eSSE_CMPLTPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from )    { CMPPSRtoR( 1 ); }
emitterT void eSSE_CMPLEPS_M128_to_XMM( x86SSERegType to, uptr from )         { CMPPSMtoR( 2 ); }
emitterT void eSSE_CMPLEPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from )    { CMPPSRtoR( 2 ); }
emitterT void eSSE_CMPUNORDPS_M128_to_XMM( x86SSERegType to, uptr from )      { CMPPSMtoR( 3 ); }
emitterT void eSSE_CMPUNORDPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from ) { CMPPSRtoR( 3 ); }
emitterT void eSSE_CMPNEPS_M128_to_XMM( x86SSERegType to, uptr from )         { CMPPSMtoR( 4 ); }
emitterT void eSSE_CMPNEPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from )    { CMPPSRtoR( 4 ); }
emitterT void eSSE_CMPNLTPS_M128_to_XMM( x86SSERegType to, uptr from )        { CMPPSMtoR( 5 ); }
emitterT void eSSE_CMPNLTPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from )   { CMPPSRtoR( 5 ); }
emitterT void eSSE_CMPNLEPS_M128_to_XMM( x86SSERegType to, uptr from )        { CMPPSMtoR( 6 ); }
emitterT void eSSE_CMPNLEPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from )   { CMPPSRtoR( 6 ); }
emitterT void eSSE_CMPORDPS_M128_to_XMM( x86SSERegType to, uptr from )        { CMPPSMtoR( 7 ); }
emitterT void eSSE_CMPORDPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from )   { CMPPSRtoR( 7 ); }

///////////////////////////////////////////////////////////////////////////////////////////
//**********************************************************************************/
//Scalar Single-Precission FP compare (CMPccSS)                                    *
//**********************************************************************************
//missing  SSE_CMPSS_I8_to_XMM
//         SSE_CMPSS_M32_to_XMM
//	       SSE_CMPSS_XMM_to_XMM
emitterT void eSSE_CMPEQSS_M32_to_XMM( x86SSERegType to, uptr from )         { CMPSSMtoR( 0 ); }
emitterT void eSSE_CMPEQSS_XMM_to_XMM( x86SSERegType to, x86SSERegType from )    { CMPSSRtoR( 0 ); }
emitterT void eSSE_CMPLTSS_M32_to_XMM( x86SSERegType to, uptr from )         { CMPSSMtoR( 1 ); }
emitterT void eSSE_CMPLTSS_XMM_to_XMM( x86SSERegType to, x86SSERegType from )    { CMPSSRtoR( 1 ); }
emitterT void eSSE_CMPLESS_M32_to_XMM( x86SSERegType to, uptr from )         { CMPSSMtoR( 2 ); }
emitterT void eSSE_CMPLESS_XMM_to_XMM( x86SSERegType to, x86SSERegType from )    { CMPSSRtoR( 2 ); }
emitterT void eSSE_CMPUNORDSS_M32_to_XMM( x86SSERegType to, uptr from )      { CMPSSMtoR( 3 ); }
emitterT void eSSE_CMPUNORDSS_XMM_to_XMM( x86SSERegType to, x86SSERegType from ) { CMPSSRtoR( 3 ); }
emitterT void eSSE_CMPNESS_M32_to_XMM( x86SSERegType to, uptr from )         { CMPSSMtoR( 4 ); }
emitterT void eSSE_CMPNESS_XMM_to_XMM( x86SSERegType to, x86SSERegType from )    { CMPSSRtoR( 4 ); }
emitterT void eSSE_CMPNLTSS_M32_to_XMM( x86SSERegType to, uptr from )        { CMPSSMtoR( 5 ); }
emitterT void eSSE_CMPNLTSS_XMM_to_XMM( x86SSERegType to, x86SSERegType from )   { CMPSSRtoR( 5 ); }
emitterT void eSSE_CMPNLESS_M32_to_XMM( x86SSERegType to, uptr from )        { CMPSSMtoR( 6 ); }
emitterT void eSSE_CMPNLESS_XMM_to_XMM( x86SSERegType to, x86SSERegType from )   { CMPSSRtoR( 6 ); }
emitterT void eSSE_CMPORDSS_M32_to_XMM( x86SSERegType to, uptr from )        { CMPSSMtoR( 7 ); }
emitterT void eSSE_CMPORDSS_XMM_to_XMM( x86SSERegType to, x86SSERegType from )   { CMPSSRtoR( 7 ); }

emitterT void eSSE_UCOMISS_M32_to_XMM( x86SSERegType to, uptr from )
{
    RexR(0, to);
	write16<I>( 0x2e0f );
	ModRM<I>( 0, to, DISP32 );
	write32<I>( MEMADDR(from, 4) );
}

emitterT void eSSE_UCOMISS_XMM_to_XMM( x86SSERegType to, x86SSERegType from )
{
    RexRB(0, to, from);
	write16<I>( 0x2e0f );
	ModRM<I>( 3, to, from );
}

//////////////////////////////////////////////////////////////////////////////////////////
//**********************************************************************************/
//RSQRTPS : Packed Single-Precision FP Square Root Reciprocal                      *
//**********************************************************************************
emitterT void eSSE_RSQRTPS_M128_to_XMM( x86SSERegType to, uptr from )			{ SSEMtoR( 0x520f, 0 ); }
emitterT void eSSE_RSQRTPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from )	{ SSERtoR( 0x520f ); }

/////////////////////////////////////////////////////////////////////////////////////
//**********************************************************************************/
//RSQRTSS : Scalar Single-Precision FP Square Root Reciprocal                      *
//**********************************************************************************
emitterT void eSSE_RSQRTSS_M32_to_XMM( x86SSERegType to, uptr from )			{ SSE_SS_MtoR( 0x520f, 0 ); }
emitterT void eSSE_RSQRTSS_XMM_to_XMM( x86SSERegType to, x86SSERegType from )	{ SSE_SS_RtoR( 0x520f ); }

////////////////////////////////////////////////////////////////////////////////////
//**********************************************************************************/
//SQRTPS : Packed Single-Precision FP Square Root                                  *
//**********************************************************************************
emitterT void eSSE_SQRTPS_M128_to_XMM( x86SSERegType to, uptr from )			{ SSEMtoR( 0x510f, 0 ); }
emitterT void eSSE_SQRTPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from )	{ SSERtoR( 0x510f ); }

//////////////////////////////////////////////////////////////////////////////////////
//**********************************************************************************/
//SQRTSS : Scalar Single-Precision FP Square Root                                  *
//**********************************************************************************
emitterT void eSSE_SQRTSS_M32_to_XMM( x86SSERegType to, uptr from )				{ SSE_SS_MtoR( 0x510f, 0 ); }
emitterT void eSSE_SQRTSS_XMM_to_XMM( x86SSERegType to, x86SSERegType from )	{ SSE_SS_RtoR( 0x510f ); }

////////////////////////////////////////////////////////////////////////////////////////
//**********************************************************************************/
//MAXPS: Return Packed Single-Precision FP Maximum                                 *
//**********************************************************************************
emitterT void eSSE_MAXPS_M128_to_XMM( x86SSERegType to, uptr from )				{ SSEMtoR( 0x5f0f, 0 ); }
emitterT void eSSE_MAXPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from )		{ SSERtoR( 0x5f0f ); }

emitterT void eSSE2_MAXPD_M128_to_XMM( x86SSERegType to, uptr from )			{ SSEMtoR66( 0x5f0f ); }
emitterT void eSSE2_MAXPD_XMM_to_XMM( x86SSERegType to, x86SSERegType from )	{ SSERtoR66( 0x5f0f ); }

/////////////////////////////////////////////////////////////////////////////////////////
//**********************************************************************************/
//MAXSS: Return Scalar Single-Precision FP Maximum                                 *
//**********************************************************************************
emitterT void eSSE_MAXSS_M32_to_XMM( x86SSERegType to, uptr from )				{ SSE_SS_MtoR( 0x5f0f, 0 ); }
emitterT void eSSE_MAXSS_XMM_to_XMM( x86SSERegType to, x86SSERegType from )		{ SSE_SS_RtoR( 0x5f0f ); }

/////////////////////////////////////////////////////////////////////////////////////////
//**********************************************************************************/
//CVTPI2PS: Packed Signed INT32 to Packed Single  FP Conversion                    *
//**********************************************************************************
emitterT void eSSE_CVTPI2PS_M64_to_XMM( x86SSERegType to, uptr from )			{ SSEMtoR( 0x2a0f, 0 ); }
emitterT void eSSE_CVTPI2PS_MM_to_XMM( x86SSERegType to, x86MMXRegType from )   { SSERtoR( 0x2a0f ); }

///////////////////////////////////////////////////////////////////////////////////////////
//**********************************************************************************/
//CVTPS2PI: Packed Single FP to Packed Signed INT32 Conversion                      *
//**********************************************************************************
emitterT void eSSE_CVTPS2PI_M64_to_MM( x86MMXRegType to, uptr from )			{ SSEMtoR( 0x2d0f, 0 ); }
emitterT void eSSE_CVTPS2PI_XMM_to_MM( x86MMXRegType to, x86SSERegType from )   { SSERtoR( 0x2d0f ); }

emitterT void eSSE_CVTTSS2SI_M32_to_R32(x86IntRegType to, uptr from) { write8<I>(0xf3); SSEMtoR(0x2c0f, 0); }
emitterT void eSSE_CVTTSS2SI_XMM_to_R32(x86IntRegType to, x86SSERegType from)
{
	write8<I>(0xf3);
    RexRB(0, to, from);
	write16<I>(0x2c0f);
	ModRM<I>(3, to, from);
}

emitterT void eSSE_CVTSI2SS_M32_to_XMM(x86SSERegType to, uptr from) { write8<I>(0xf3); SSEMtoR(0x2a0f, 0); }
emitterT void eSSE_CVTSI2SS_R_to_XMM(x86SSERegType to, x86IntRegType from)
{
	write8<I>(0xf3);
    RexRB(0, to, from);
	write16<I>(0x2a0f);
	ModRM<I>(3, to, from);
}

///////////////////////////////////////////////////////////////////////////////////////////
//**********************************************************************************/
//CVTDQ2PS: Packed Signed INT32  to Packed Single Precision FP  Conversion         *
//**********************************************************************************
emitterT void eSSE2_CVTDQ2PS_M128_to_XMM( x86SSERegType to, uptr from )			{ SSEMtoR( 0x5b0f, 0 ); }
emitterT void eSSE2_CVTDQ2PS_XMM_to_XMM( x86SSERegType to, x86SSERegType from )	{ SSERtoR( 0x5b0f ); }

//**********************************************************************************/
//CVTPS2DQ: Packed Single Precision FP to Packed Signed INT32 Conversion           *
//**********************************************************************************
emitterT void eSSE2_CVTPS2DQ_M128_to_XMM( x86SSERegType to, uptr from )			{ SSEMtoR66( 0x5b0f ); }
emitterT void eSSE2_CVTPS2DQ_XMM_to_XMM( x86SSERegType to, x86SSERegType from )	{ SSERtoR66( 0x5b0f ); }

emitterT void eSSE2_CVTTPS2DQ_XMM_to_XMM( x86SSERegType to, x86SSERegType from ){ write8<I>(0xf3); SSERtoR(0x5b0f); }
/////////////////////////////////////////////////////////////////////////////////////
//**********************************************************************************/
//MINPS: Return Packed Single-Precision FP Minimum                                 *
//**********************************************************************************
emitterT void eSSE_MINPS_M128_to_XMM( x86SSERegType to, uptr from )				{ SSEMtoR( 0x5d0f, 0 ); }
emitterT void eSSE_MINPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from )		{ SSERtoR( 0x5d0f ); }

emitterT void eSSE2_MINPD_M128_to_XMM( x86SSERegType to, uptr from )			{ SSEMtoR66( 0x5d0f ); }
emitterT void eSSE2_MINPD_XMM_to_XMM( x86SSERegType to, x86SSERegType from )	{ SSERtoR66( 0x5d0f ); }

//////////////////////////////////////////////////////////////////////////////////////////
//**********************************************************************************/
//MINSS: Return Scalar Single-Precision FP Minimum                                 *
//**********************************************************************************
emitterT void eSSE_MINSS_M32_to_XMM( x86SSERegType to, uptr from )				{ SSE_SS_MtoR( 0x5d0f, 0 ); }
emitterT void eSSE_MINSS_XMM_to_XMM( x86SSERegType to, x86SSERegType from )		{ SSE_SS_RtoR( 0x5d0f ); }

///////////////////////////////////////////////////////////////////////////////////////////
//**********************************************************************************/
//PMAXSW: Packed Signed Integer Word Maximum                                        *
//**********************************************************************************
//missing
 //     SSE_PMAXSW_M64_to_MM
//		SSE2_PMAXSW_M128_to_XMM
//		SSE2_PMAXSW_XMM_to_XMM
emitterT void eSSE_PMAXSW_MM_to_MM( x86MMXRegType to, x86MMXRegType from ){ SSERtoR( 0xEE0F ); }

///////////////////////////////////////////////////////////////////////////////////////
//**********************************************************************************/
//PMINSW: Packed Signed Integer Word Minimum                                        *
//**********************************************************************************
//missing
 //     SSE_PMINSW_M64_to_MM
//		SSE2_PMINSW_M128_to_XMM
//		SSE2_PMINSW_XMM_to_XMM
emitterT void eSSE_PMINSW_MM_to_MM( x86MMXRegType to, x86MMXRegType from ){ SSERtoR( 0xEA0F ); }

//////////////////////////////////////////////////////////////////////////////////////
//**********************************************************************************/
//SHUFPS: Shuffle Packed Single-Precision FP Values                                *
//**********************************************************************************
emitterT void eSSE_SHUFPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from, u8 imm8 )	{ SSERtoR( 0xC60F ); write8<I>( imm8 ); }
emitterT void eSSE_SHUFPS_M128_to_XMM( x86SSERegType to, uptr from, u8 imm8 )			{ SSEMtoR( 0xC60F, 1 ); write8<I>( imm8 ); }

emitterT void eSSE_SHUFPS_RmOffset_to_XMM( x86SSERegType to, x86IntRegType from, int offset, u8 imm8 )
{
    RexRB(0, to, from);
	write16<I>(0xc60f);
    WriteRmOffsetFrom<I>(to, from, offset);
	write8<I>(imm8);
}

//////////////////////////////////////////////////////////////////////////////////////
//**********************************************************************************/
//SHUFPD: Shuffle Packed Double-Precision FP Values                                *
//**********************************************************************************
emitterT void eSSE2_SHUFPD_XMM_to_XMM( x86SSERegType to, x86SSERegType from, u8 imm8 )	{ SSERtoR66( 0xC60F ); write8<I>( imm8 ); }
emitterT void eSSE2_SHUFPD_M128_to_XMM( x86SSERegType to, uptr from, u8 imm8 )			{ SSEMtoR66( 0xC60F ); write8<I>( imm8 ); }

////////////////////////////////////////////////////////////////////////////////////
//**********************************************************************************/
//PSHUFD: Shuffle Packed DoubleWords                                               *
//**********************************************************************************
emitterT void eSSE2_PSHUFD_XMM_to_XMM( x86SSERegType to, x86SSERegType from, u8 imm8 )
{
	SSERtoR66( 0x700F );
	write8<I>( imm8 );
}
emitterT void eSSE2_PSHUFD_M128_to_XMM( x86SSERegType to, uptr from, u8 imm8 )	{ SSEMtoR66( 0x700F ); write8<I>( imm8 ); }

emitterT void eSSE2_PSHUFLW_XMM_to_XMM( x86SSERegType to, x86SSERegType from, u8 imm8 ) { write8<I>(0xF2); SSERtoR(0x700F); write8<I>(imm8); }
emitterT void eSSE2_PSHUFLW_M128_to_XMM( x86SSERegType to, uptr from, u8 imm8 ) { write8<I>(0xF2); SSEMtoR(0x700F, 1); write8<I>(imm8); }
emitterT void eSSE2_PSHUFHW_XMM_to_XMM( x86SSERegType to, x86SSERegType from, u8 imm8 ) { write8<I>(0xF3); SSERtoR(0x700F); write8<I>(imm8); }
emitterT void eSSE2_PSHUFHW_M128_to_XMM( x86SSERegType to, uptr from, u8 imm8 ) { write8<I>(0xF3); SSEMtoR(0x700F, 1); write8<I>(imm8); }

///////////////////////////////////////////////////////////////////////////////////
//**********************************************************************************/
//UNPCKLPS: Unpack and Interleave low Packed Single-Precision FP Data              *
//**********************************************************************************
emitterT void eSSE_UNPCKLPS_M128_to_XMM( x86SSERegType to, uptr from )			{ SSEMtoR(0x140f, 0); }
emitterT void eSSE_UNPCKLPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from )	{ SSERtoR( 0x140F ); }

////////////////////////////////////////////////////////////////////////////////////////
//**********************************************************************************/
//UNPCKHPS: Unpack and Interleave High Packed Single-Precision FP Data              *
//**********************************************************************************
emitterT void eSSE_UNPCKHPS_M128_to_XMM( x86SSERegType to, uptr from )			{ SSEMtoR(0x150f, 0); }
emitterT void eSSE_UNPCKHPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from )	{ SSERtoR( 0x150F ); }

////////////////////////////////////////////////////////////////////////////////////////
//**********************************************************************************/
//DIVPS : Packed Single-Precision FP Divide                                       *
//**********************************************************************************
emitterT void eSSE_DIVPS_M128_to_XMM( x86SSERegType to, uptr from )				{ SSEMtoR( 0x5e0F, 0 ); }
emitterT void eSSE_DIVPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from )		{ SSERtoR( 0x5e0F ); }

//////////////////////////////////////////////////////////////////////////////////////
//**********************************************************************************/
//DIVSS : Scalar  Single-Precision FP Divide                                       *
//**********************************************************************************
emitterT void eSSE_DIVSS_M32_to_XMM( x86SSERegType to, uptr from )				{ SSE_SS_MtoR( 0x5e0F, 0 ); }
emitterT void eSSE_DIVSS_XMM_to_XMM( x86SSERegType to, x86SSERegType from )		{ SSE_SS_RtoR( 0x5e0F ); }

/////////////////////////////////////////////////////////////////////////////////////////
//**********************************************************************************/
//STMXCSR : Store Streaming SIMD Extension Control/Status                         *
//**********************************************************************************
emitterT void eSSE_STMXCSR( uptr from ) {
	write16<I>( 0xAE0F );
	ModRM<I>( 0, 0x3, DISP32 );
	write32<I>( MEMADDR(from, 4) );
}

/////////////////////////////////////////////////////////////////////////////////////
//**********************************************************************************/
//LDMXCSR : Load Streaming SIMD Extension Control/Status                         *
//**********************************************************************************
emitterT void eSSE_LDMXCSR( uptr from ) {
	write16<I>( 0xAE0F );
	ModRM<I>( 0, 0x2, DISP32 );
	write32<I>( MEMADDR(from, 4) );
}

/////////////////////////////////////////////////////////////////////////////////////
//**********************************************************************************/
//PADDB,PADDW,PADDD : Add Packed Integers                                          *
//**********************************************************************************
emitterT void eSSE2_PADDB_XMM_to_XMM(x86SSERegType to, x86SSERegType from ){ SSERtoR66( 0xFC0F ); }
emitterT void eSSE2_PADDB_M128_to_XMM(x86SSERegType to, uptr from ){ SSEMtoR66( 0xFC0F ); }
emitterT void eSSE2_PADDW_XMM_to_XMM(x86SSERegType to, x86SSERegType from ){ SSERtoR66( 0xFD0F ); }
emitterT void eSSE2_PADDW_M128_to_XMM(x86SSERegType to, uptr from ){ SSEMtoR66( 0xFD0F ); }
emitterT void eSSE2_PADDD_XMM_to_XMM(x86SSERegType to, x86SSERegType from ){ SSERtoR66( 0xFE0F ); }
emitterT void eSSE2_PADDD_M128_to_XMM(x86SSERegType to, uptr from ){ SSEMtoR66( 0xFE0F ); }
emitterT void eSSE2_PADDQ_XMM_to_XMM(x86SSERegType to, x86SSERegType from ) { SSERtoR66( 0xD40F ); }
emitterT void eSSE2_PADDQ_M128_to_XMM(x86SSERegType to, uptr from ) { SSEMtoR66( 0xD40F ); }

///////////////////////////////////////////////////////////////////////////////////
//**********************************************************************************/
//PCMPxx: Compare Packed Integers                                                  *
//**********************************************************************************
emitterT void eSSE2_PCMPGTB_XMM_to_XMM(x86SSERegType to, x86SSERegType from ){ SSERtoR66( 0x640F ); }
emitterT void eSSE2_PCMPGTB_M128_to_XMM(x86SSERegType to, uptr from ){ SSEMtoR66( 0x640F ); }
emitterT void eSSE2_PCMPGTW_XMM_to_XMM(x86SSERegType to, x86SSERegType from ){ SSERtoR66( 0x650F ); }
emitterT void eSSE2_PCMPGTW_M128_to_XMM(x86SSERegType to, uptr from ){ SSEMtoR66( 0x650F ); }
emitterT void eSSE2_PCMPGTD_XMM_to_XMM(x86SSERegType to, x86SSERegType from ){ SSERtoR66( 0x660F ); }
emitterT void eSSE2_PCMPGTD_M128_to_XMM(x86SSERegType to, uptr from ){ SSEMtoR66( 0x660F ); }
emitterT void eSSE2_PCMPEQB_XMM_to_XMM(x86SSERegType to, x86SSERegType from ){ SSERtoR66( 0x740F ); }
emitterT void eSSE2_PCMPEQB_M128_to_XMM(x86SSERegType to, uptr from ){ SSEMtoR66( 0x740F ); }
emitterT void eSSE2_PCMPEQW_XMM_to_XMM(x86SSERegType to, x86SSERegType from ){ SSERtoR66( 0x750F ); }
emitterT void eSSE2_PCMPEQW_M128_to_XMM(x86SSERegType to, uptr from ){ SSEMtoR66( 0x750F ); }
emitterT void eSSE2_PCMPEQD_XMM_to_XMM(x86SSERegType to, x86SSERegType from ){ SSERtoR66( 0x760F ); }
emitterT void eSSE2_PCMPEQD_M128_to_XMM(x86SSERegType to, uptr from ){ SSEMtoR66( 0x760F ); }

////////////////////////////////////////////////////////////////////////////////////////////
//**********************************************************************************/
//PEXTRW,PINSRW: Packed Extract/Insert Word                                        *
//**********************************************************************************
emitterT void eSSE_PEXTRW_XMM_to_R32(x86IntRegType to, x86SSERegType from, u8 imm8 ){ SSERtoR66(0xC50F); write8<I>( imm8 ); }
emitterT void eSSE_PINSRW_R32_to_XMM(x86SSERegType to, x86IntRegType from, u8 imm8 ){ SSERtoR66(0xC40F); write8<I>( imm8 ); }

////////////////////////////////////////////////////////////////////////////////////////////
//**********************************************************************************/
//PSUBx: Subtract Packed Integers                                                  *
//**********************************************************************************
emitterT void eSSE2_PSUBB_XMM_to_XMM(x86SSERegType to, x86SSERegType from ){ SSERtoR66( 0xF80F ); }
emitterT void eSSE2_PSUBB_M128_to_XMM(x86SSERegType to, uptr from ){ SSEMtoR66( 0xF80F ); }
emitterT void eSSE2_PSUBW_XMM_to_XMM(x86SSERegType to, x86SSERegType from ){ SSERtoR66( 0xF90F ); }
emitterT void eSSE2_PSUBW_M128_to_XMM(x86SSERegType to, uptr from ){ SSEMtoR66( 0xF90F ); }
emitterT void eSSE2_PSUBD_XMM_to_XMM(x86SSERegType to, x86SSERegType from ){ SSERtoR66( 0xFA0F ); }
emitterT void eSSE2_PSUBD_M128_to_XMM(x86SSERegType to, uptr from ){ SSEMtoR66( 0xFA0F ); }
emitterT void eSSE2_PSUBQ_XMM_to_XMM(x86SSERegType to, x86SSERegType from ){ SSERtoR66( 0xFB0F ); }
emitterT void eSSE2_PSUBQ_M128_to_XMM(x86SSERegType to, uptr from ){ SSEMtoR66( 0xFB0F ); }

///////////////////////////////////////////////////////////////////////////////////////
//**********************************************************************************/
//MOVD: Move Dword(32bit) to /from XMM reg                                         *
//**********************************************************************************
emitterT void eSSE2_MOVD_M32_to_XMM( x86SSERegType to, uptr from )			{ SSEMtoR66(0x6E0F); }
emitterT void eSSE2_MOVD_R_to_XMM( x86SSERegType to, x86IntRegType from )	{ SSERtoR66(0x6E0F); }

emitterT void eSSE2_MOVD_Rm_to_XMM( x86SSERegType to, x86IntRegType from )
{
	write8<I>(0x66);
	RexRB(0, to, from);
	write16<I>( 0x6e0f );
	ModRM<I>( 0, to, from);
}

emitterT void eSSE2_MOVD_RmOffset_to_XMM( x86SSERegType to, x86IntRegType from, int offset )
{
	write8<I>(0x66);
    RexRB(0, to, from);
	write16<I>( 0x6e0f );
    WriteRmOffsetFrom<I>(to, from, offset);
}

emitterT void eSSE2_MOVD_XMM_to_M32( u32 to, x86SSERegType from )			{ SSERtoM66(0x7E0F); }
emitterT void eSSE2_MOVD_XMM_to_R( x86IntRegType to, x86SSERegType from )	{ _SSERtoR66(0x7E0F); }

emitterT void eSSE2_MOVD_XMM_to_Rm( x86IntRegType to, x86SSERegType from )
{
	write8<I>(0x66);
    RexRB(0, from, to);
	write16<I>( 0x7e0f );
	ModRM<I>( 0, from, to );
}

emitterT void eSSE2_MOVD_XMM_to_RmOffset( x86IntRegType to, x86SSERegType from, int offset )
{
	write8<I>(0x66);
    RexRB(0, from, to);
	write16<I>( 0x7e0f );
    WriteRmOffsetFrom<I>(from, to, offset);
}

////////////////////////////////////////////////////////////////////////////////////
//**********************************************************************************/
//POR : SSE Bitwise OR                                                             *
//**********************************************************************************
emitterT void eSSE2_POR_XMM_to_XMM( x86SSERegType to, x86SSERegType from )		{ SSERtoR66( 0xEB0F ); }
emitterT void eSSE2_POR_M128_to_XMM( x86SSERegType to, uptr from )				{ SSEMtoR66( 0xEB0F ); }

// logical and to &= from
emitterT void eSSE2_PAND_XMM_to_XMM( x86SSERegType to, x86SSERegType from )		{ SSERtoR66( 0xDB0F ); }
emitterT void eSSE2_PAND_M128_to_XMM( x86SSERegType to, uptr from )				{ SSEMtoR66( 0xDB0F ); }

// to = (~to) & from
emitterT void eSSE2_PANDN_XMM_to_XMM( x86SSERegType to, x86SSERegType from )	{ SSERtoR66( 0xDF0F ); }
emitterT void eSSE2_PANDN_M128_to_XMM( x86SSERegType to, uptr from )			{ SSEMtoR66( 0xDF0F ); }

/////////////////////////////////////////////////////////////////////////////////////
//**********************************************************************************/
//PXOR : SSE Bitwise XOR                                                             *
//**********************************************************************************
emitterT void eSSE2_PXOR_XMM_to_XMM( x86SSERegType to, x86SSERegType from )		{ SSERtoR66( 0xEF0F ); }
emitterT void eSSE2_PXOR_M128_to_XMM( x86SSERegType to, uptr from )				{ SSEMtoR66( 0xEF0F ); }
///////////////////////////////////////////////////////////////////////////////////////

emitterT void eSSE2_MOVDQA_M128_to_XMM(x86SSERegType to, uptr from)				{ if( AlwaysUseMovaps ) eSSE_MOVAPS_M128_to_XMM<I>( to, from ); else SSEMtoR66(0x6F0F); }
emitterT void eSSE2_MOVDQA_XMM_to_M128( uptr to, x86SSERegType from )			{ if( AlwaysUseMovaps ) eSSE_MOVAPS_XMM_to_M128<I>( to, from ); else SSERtoM66(0x7F0F); } 
emitterT void eSSE2_MOVDQA_XMM_to_XMM( x86SSERegType to, x86SSERegType from)	{ if (to != from) { if( AlwaysUseMovaps ) eSSE_MOVAPS_XMM_to_XMM<I>( to, from ); else SSERtoR66(0x6F0F); } }

emitterT void eSSE2_MOVDQU_M128_to_XMM(x86SSERegType to, uptr from)
{
	if( AlwaysUseMovaps )
		eSSE_MOVUPS_M128_to_XMM<I>( to, from );
	else
	{
		write8<I>(0xF3);
		SSEMtoR(0x6F0F, 0);
	}
}
emitterT void eSSE2_MOVDQU_XMM_to_M128( uptr to, x86SSERegType from)
{
	if( AlwaysUseMovaps )
		eSSE_MOVUPS_XMM_to_M128<I>( to, from );
	else
	{
		write8<I>(0xF3);
		SSERtoM(0x7F0F, 0);
	}
}

// shift right logical

emitterT void eSSE2_PSRLW_XMM_to_XMM(x86SSERegType to, x86SSERegType from)	{ SSERtoR66(0xD10F); }
emitterT void eSSE2_PSRLW_M128_to_XMM(x86SSERegType to, uptr from)			{ SSEMtoR66(0xD10F); }
emitterT void eSSE2_PSRLW_I8_to_XMM(x86SSERegType to, u8 imm8)
{
	write8<I>( 0x66 );
    RexB(0, to);
	write16<I>( 0x710F );
	ModRM<I>( 3, 2 , to );
	write8<I>( imm8 );
}

emitterT void eSSE2_PSRLD_XMM_to_XMM(x86SSERegType to, x86SSERegType from)	{ SSERtoR66(0xD20F); }
emitterT void eSSE2_PSRLD_M128_to_XMM(x86SSERegType to, uptr from)			{ SSEMtoR66(0xD20F); }
emitterT void eSSE2_PSRLD_I8_to_XMM(x86SSERegType to, u8 imm8)
{
	write8<I>( 0x66 );
    RexB(0, to);
	write16<I>( 0x720F );
	ModRM<I>( 3, 2 , to ); 
	write8<I>( imm8 );
}

emitterT void eSSE2_PSRLQ_XMM_to_XMM(x86SSERegType to, x86SSERegType from)	{ SSERtoR66(0xD30F); }
emitterT void eSSE2_PSRLQ_M128_to_XMM(x86SSERegType to, uptr from)			{ SSEMtoR66(0xD30F); }
emitterT void eSSE2_PSRLQ_I8_to_XMM(x86SSERegType to, u8 imm8)
{
	write8<I>( 0x66 );
    RexB(0, to);
	write16<I>( 0x730F );
	ModRM<I>( 3, 2 , to ); 
	write8<I>( imm8 );
}

emitterT void eSSE2_PSRLDQ_I8_to_XMM(x86SSERegType to, u8 imm8)
{
	write8<I>( 0x66 );
    RexB(0, to);
	write16<I>( 0x730F );
	ModRM<I>( 3, 3 , to ); 
	write8<I>( imm8 );
}

// shift right arithmetic

emitterT void eSSE2_PSRAW_XMM_to_XMM(x86SSERegType to, x86SSERegType from)	{ SSERtoR66(0xE10F); }
emitterT void eSSE2_PSRAW_M128_to_XMM(x86SSERegType to, uptr from)			{ SSEMtoR66(0xE10F); }
emitterT void eSSE2_PSRAW_I8_to_XMM(x86SSERegType to, u8 imm8)
{
	write8<I>( 0x66 );
    RexB(0, to);
	write16<I>( 0x710F );
	ModRM<I>( 3, 4 , to );
	write8<I>( imm8 );
}

emitterT void eSSE2_PSRAD_XMM_to_XMM(x86SSERegType to, x86SSERegType from)	{ SSERtoR66(0xE20F); }
emitterT void eSSE2_PSRAD_M128_to_XMM(x86SSERegType to, uptr from)			{ SSEMtoR66(0xE20F); }
emitterT void eSSE2_PSRAD_I8_to_XMM(x86SSERegType to, u8 imm8)
{
	write8<I>( 0x66 );
    RexB(0, to);
	write16<I>( 0x720F );
	ModRM<I>( 3, 4 , to );
	write8<I>( imm8 );
}

// shift left logical

emitterT void eSSE2_PSLLW_XMM_to_XMM(x86SSERegType to, x86SSERegType from)	{ SSERtoR66(0xF10F); }
emitterT void eSSE2_PSLLW_M128_to_XMM(x86SSERegType to, uptr from)			{ SSEMtoR66(0xF10F); }
emitterT void eSSE2_PSLLW_I8_to_XMM(x86SSERegType to, u8 imm8)
{
	write8<I>( 0x66 );
    RexB(0, to);
	write16<I>( 0x710F );
	ModRM<I>( 3, 6 , to );
	write8<I>( imm8 );
}

emitterT void eSSE2_PSLLD_XMM_to_XMM(x86SSERegType to, x86SSERegType from)	{ SSERtoR66(0xF20F); }
emitterT void eSSE2_PSLLD_M128_to_XMM(x86SSERegType to, uptr from)			{ SSEMtoR66(0xF20F); }
emitterT void eSSE2_PSLLD_I8_to_XMM(x86SSERegType to, u8 imm8)
{
	write8<I>( 0x66 );
    RexB(0, to);
	write16<I>( 0x720F );
	ModRM<I>( 3, 6 , to );
	write8<I>( imm8 );
}

emitterT void eSSE2_PSLLQ_XMM_to_XMM(x86SSERegType to, x86SSERegType from)	{ SSERtoR66(0xF30F); }
emitterT void eSSE2_PSLLQ_M128_to_XMM(x86SSERegType to, uptr from)			{ SSEMtoR66(0xF30F); }
emitterT void eSSE2_PSLLQ_I8_to_XMM(x86SSERegType to, u8 imm8)
{
	write8<I>( 0x66 );
    RexB(0, to);
	write16<I>( 0x730F );
	ModRM<I>( 3, 6 , to );
	write8<I>( imm8 );
}

emitterT void eSSE2_PSLLDQ_I8_to_XMM(x86SSERegType to, u8 imm8)
{
	write8<I>( 0x66 );
    RexB(0, to);
	write16<I>( 0x730F );
	ModRM<I>( 3, 7 , to ); 
	write8<I>( imm8 );
}

emitterT void eSSE2_PMAXSW_XMM_to_XMM( x86SSERegType to, x86SSERegType from )	{ SSERtoR66( 0xEE0F ); }
emitterT void eSSE2_PMAXSW_M128_to_XMM( x86SSERegType to, uptr from )			{ SSEMtoR66( 0xEE0F ); }

emitterT void eSSE2_PMAXUB_XMM_to_XMM( x86SSERegType to, x86SSERegType from )	{ SSERtoR66( 0xDE0F ); }
emitterT void eSSE2_PMAXUB_M128_to_XMM( x86SSERegType to, uptr from )			{ SSEMtoR66( 0xDE0F ); }

emitterT void eSSE2_PMINSW_XMM_to_XMM( x86SSERegType to, x86SSERegType from )	{ SSERtoR66( 0xEA0F ); }
emitterT void eSSE2_PMINSW_M128_to_XMM( x86SSERegType to, uptr from )			{ SSEMtoR66( 0xEA0F ); }

emitterT void eSSE2_PMINUB_XMM_to_XMM( x86SSERegType to, x86SSERegType from )	{ SSERtoR66( 0xDA0F ); }
emitterT void eSSE2_PMINUB_M128_to_XMM( x86SSERegType to, uptr from )			{ SSEMtoR66( 0xDA0F ); }

emitterT void eSSE2_PADDSB_XMM_to_XMM( x86SSERegType to, x86SSERegType from )	{ SSERtoR66( 0xEC0F ); }
emitterT void eSSE2_PADDSB_M128_to_XMM( x86SSERegType to, uptr from )			{ SSEMtoR66( 0xEC0F ); }

emitterT void eSSE2_PADDSW_XMM_to_XMM( x86SSERegType to, x86SSERegType from )	{ SSERtoR66( 0xED0F ); }
emitterT void eSSE2_PADDSW_M128_to_XMM( x86SSERegType to, uptr from )			{ SSEMtoR66( 0xED0F ); }

emitterT void eSSE2_PSUBSB_XMM_to_XMM( x86SSERegType to, x86SSERegType from )	{ SSERtoR66( 0xE80F ); }
emitterT void eSSE2_PSUBSB_M128_to_XMM( x86SSERegType to, uptr from )			{ SSEMtoR66( 0xE80F ); }

emitterT void eSSE2_PSUBSW_XMM_to_XMM( x86SSERegType to, x86SSERegType from )	{ SSERtoR66( 0xE90F ); }
emitterT void eSSE2_PSUBSW_M128_to_XMM( x86SSERegType to, uptr from )			{ SSEMtoR66( 0xE90F ); }

emitterT void eSSE2_PSUBUSB_XMM_to_XMM( x86SSERegType to, x86SSERegType from )	{ SSERtoR66( 0xD80F ); }
emitterT void eSSE2_PSUBUSB_M128_to_XMM( x86SSERegType to, uptr from )			{ SSEMtoR66( 0xD80F ); }
emitterT void eSSE2_PSUBUSW_XMM_to_XMM( x86SSERegType to, x86SSERegType from )	{ SSERtoR66( 0xD90F ); }
emitterT void eSSE2_PSUBUSW_M128_to_XMM( x86SSERegType to, uptr from )			{ SSEMtoR66( 0xD90F ); }

emitterT void eSSE2_PADDUSB_XMM_to_XMM( x86SSERegType to, x86SSERegType from )	{ SSERtoR66( 0xDC0F ); }
emitterT void eSSE2_PADDUSB_M128_to_XMM( x86SSERegType to, uptr from )			{ SSEMtoR66( 0xDC0F ); }
emitterT void eSSE2_PADDUSW_XMM_to_XMM( x86SSERegType to, x86SSERegType from )	{ SSERtoR66( 0xDD0F ); }
emitterT void eSSE2_PADDUSW_M128_to_XMM( x86SSERegType to, uptr from )			{ SSEMtoR66( 0xDD0F ); }

//**********************************************************************************/
//PACKSSWB,PACKSSDW: Pack Saturate Signed Word
//**********************************************************************************
emitterT void eSSE2_PACKSSWB_XMM_to_XMM(x86SSERegType to, x86SSERegType from)	{ SSERtoR66( 0x630F ); }
emitterT void eSSE2_PACKSSWB_M128_to_XMM(x86SSERegType to, uptr from)			{ SSEMtoR66( 0x630F ); }
emitterT void eSSE2_PACKSSDW_XMM_to_XMM(x86SSERegType to, x86SSERegType from)	{ SSERtoR66( 0x6B0F ); }
emitterT void eSSE2_PACKSSDW_M128_to_XMM(x86SSERegType to, uptr from)			{ SSEMtoR66( 0x6B0F ); }

emitterT void eSSE2_PACKUSWB_XMM_to_XMM(x86SSERegType to, x86SSERegType from)	{ SSERtoR66( 0x670F ); }
emitterT void eSSE2_PACKUSWB_M128_to_XMM(x86SSERegType to, uptr from)			{ SSEMtoR66( 0x670F ); }

//**********************************************************************************/
//PUNPCKHWD: Unpack 16bit high
//**********************************************************************************
emitterT void eSSE2_PUNPCKLBW_XMM_to_XMM(x86SSERegType to, x86SSERegType from)	{ SSERtoR66( 0x600F ); }
emitterT void eSSE2_PUNPCKLBW_M128_to_XMM(x86SSERegType to, uptr from)			{ SSEMtoR66( 0x600F ); }

emitterT void eSSE2_PUNPCKHBW_XMM_to_XMM(x86SSERegType to, x86SSERegType from)	{ SSERtoR66( 0x680F ); }
emitterT void eSSE2_PUNPCKHBW_M128_to_XMM(x86SSERegType to, uptr from)			{ SSEMtoR66( 0x680F ); }

emitterT void eSSE2_PUNPCKLWD_XMM_to_XMM(x86SSERegType to, x86SSERegType from)	{ SSERtoR66( 0x610F ); }
emitterT void eSSE2_PUNPCKLWD_M128_to_XMM(x86SSERegType to, uptr from)			{ SSEMtoR66( 0x610F ); }
emitterT void eSSE2_PUNPCKHWD_XMM_to_XMM(x86SSERegType to, x86SSERegType from)	{ SSERtoR66( 0x690F ); }
emitterT void eSSE2_PUNPCKHWD_M128_to_XMM(x86SSERegType to, uptr from)			{ SSEMtoR66( 0x690F ); }

emitterT void eSSE2_PUNPCKLDQ_XMM_to_XMM(x86SSERegType to, x86SSERegType from)	{ SSERtoR66( 0x620F ); }
emitterT void eSSE2_PUNPCKLDQ_M128_to_XMM(x86SSERegType to, uptr from)			{ SSEMtoR66( 0x620F ); }
emitterT void eSSE2_PUNPCKHDQ_XMM_to_XMM(x86SSERegType to, x86SSERegType from)	{ SSERtoR66( 0x6A0F ); }
emitterT void eSSE2_PUNPCKHDQ_M128_to_XMM(x86SSERegType to, uptr from)			{ SSEMtoR66( 0x6A0F ); }

emitterT void eSSE2_PUNPCKLQDQ_XMM_to_XMM(x86SSERegType to, x86SSERegType from) { SSERtoR66( 0x6C0F ); }
emitterT void eSSE2_PUNPCKLQDQ_M128_to_XMM(x86SSERegType to, uptr from)			{ SSEMtoR66( 0x6C0F ); }

emitterT void eSSE2_PUNPCKHQDQ_XMM_to_XMM(x86SSERegType to, x86SSERegType from)	{ SSERtoR66( 0x6D0F ); }
emitterT void eSSE2_PUNPCKHQDQ_M128_to_XMM(x86SSERegType to, uptr from)			{ SSEMtoR66( 0x6D0F ); }

emitterT void eSSE2_PMULLW_XMM_to_XMM(x86SSERegType to, x86SSERegType from)		{ SSERtoR66( 0xD50F ); }
emitterT void eSSE2_PMULLW_M128_to_XMM(x86SSERegType to, uptr from)				{ SSEMtoR66( 0xD50F ); }
emitterT void eSSE2_PMULHW_XMM_to_XMM(x86SSERegType to, x86SSERegType from)		{ SSERtoR66( 0xE50F ); }
emitterT void eSSE2_PMULHW_M128_to_XMM(x86SSERegType to, uptr from)				{ SSEMtoR66( 0xE50F ); }

emitterT void eSSE2_PMULUDQ_XMM_to_XMM(x86SSERegType to, x86SSERegType from)	{ SSERtoR66( 0xF40F ); }
emitterT void eSSE2_PMULUDQ_M128_to_XMM(x86SSERegType to, uptr from)			{ SSEMtoR66( 0xF40F ); }

emitterT void eSSE2_PMOVMSKB_XMM_to_R32(x86IntRegType to, x86SSERegType from)	{ SSERtoR66(0xD70F); }

emitterT void eSSE_MOVMSKPS_XMM_to_R32(x86IntRegType to, x86SSERegType from)	{ SSERtoR(0x500F); }
emitterT void eSSE2_MOVMSKPD_XMM_to_R32(x86IntRegType to, x86SSERegType from)	{ SSERtoR66(0x500F); }

emitterT void eSSE2_PMADDWD_XMM_to_XMM(x86SSERegType to, x86SSERegType from)	{ SSERtoR66(0xF50F); }

emitterT void eSSE3_HADDPS_XMM_to_XMM(x86SSERegType to, x86SSERegType from)		{ write8<I>(0xf2); SSERtoR( 0x7c0f ); }
emitterT void eSSE3_HADDPS_M128_to_XMM(x86SSERegType to, uptr from)				{ write8<I>(0xf2); SSEMtoR( 0x7c0f, 0 ); }

emitterT void eSSE3_MOVSLDUP_XMM_to_XMM(x86SSERegType to, x86SSERegType from) {
	write8<I>(0xf3);
    RexRB(0, to, from);
	write16<I>( 0x120f);
	ModRM<I>( 3, to, from );
}

emitterT void eSSE3_MOVSLDUP_M128_to_XMM(x86SSERegType to, uptr from)			{ write8<I>(0xf3); SSEMtoR(0x120f, 0); }
emitterT void eSSE3_MOVSHDUP_XMM_to_XMM(x86SSERegType to, x86SSERegType from)	{ write8<I>(0xf3); SSERtoR(0x160f); }
emitterT void eSSE3_MOVSHDUP_M128_to_XMM(x86SSERegType to, uptr from)			{ write8<I>(0xf3); SSEMtoR(0x160f, 0); }

// SSSE3

emitterT void eSSSE3_PABSB_XMM_to_XMM(x86SSERegType to, x86SSERegType from)
{
	write8<I>(0x66);
	RexRB(0, to, from);
	write24<I>(0x1C380F);
	ModRM<I>(3, to, from);
}

emitterT void eSSSE3_PABSW_XMM_to_XMM(x86SSERegType to, x86SSERegType from)
{
	write8<I>(0x66);
	RexRB(0, to, from);
	write24<I>(0x1D380F);
	ModRM<I>(3, to, from);
}

emitterT void eSSSE3_PABSD_XMM_to_XMM(x86SSERegType to, x86SSERegType from)
{
	write8<I>(0x66);
	RexRB(0, to, from);
	write24<I>(0x1E380F);
	ModRM<I>(3, to, from);
}

emitterT void eSSSE3_PALIGNR_XMM_to_XMM(x86SSERegType to, x86SSERegType from, u8 imm8)
{
	write8<I>(0x66);
	RexRB(0, to, from);
	write24<I>(0x0F3A0F);
	ModRM<I>(3, to, from);
	write8<I>(imm8);
}

// SSE4.1

emitterT void eSSE4_DPPS_XMM_to_XMM(x86SSERegType to, x86SSERegType from, u8 imm8) 
{
	write8<I>(0x66);
	write24<I>(0x403A0F);
	ModRM<I>(3, to, from);
	write8<I>(imm8);
}

emitterT void eSSE4_DPPS_M128_to_XMM(x86SSERegType to, uptr from, u8 imm8)
{
	write8<I>(0x66);
	write24<I>(0x403A0F);
	ModRM<I>(0, to, DISP32);
	write32<I>(MEMADDR(from, 4));
	write8<I>(imm8);
}

emitterT void eSSE4_INSERTPS_XMM_to_XMM(x86SSERegType to, x86SSERegType from, u8 imm8)
{
	write8<I>(0x66);
    RexRB(0, to, from);
	write24<I>(0x213A0F);
	ModRM<I>(3, to, from);
	write8<I>(imm8);
}

emitterT void eSSE4_EXTRACTPS_XMM_to_R32(x86IntRegType to, x86SSERegType from, u8 imm8)
{
	write8<I>(0x66);
    RexRB(0, to, from);
	write24<I>(0x173A0F);
	ModRM<I>(3, to, from);
	write8<I>(imm8);
}

emitterT void eSSE4_BLENDPS_XMM_to_XMM(x86IntRegType to, x86SSERegType from, u8 imm8)
{
	write8<I>(0x66);
    RexRB(0, to, from);
	write24<I>(0x0C3A0F);
	ModRM<I>(3, to, from);
	write8<I>(imm8);
}

emitterT void eSSE4_BLENDVPS_XMM_to_XMM(x86SSERegType to, x86SSERegType from)
{
	write8<I>(0x66);
    RexRB(0, to, from);
	write24<I>(0x14380F);
	ModRM<I>(3, to, from);
}

emitterT void eSSE4_BLENDVPS_M128_to_XMM(x86SSERegType to, uptr from)
{
	write8<I>(0x66);
    RexR(0, to);
	write24<I>(0x14380F);
	ModRM<I>(0, to, DISP32);
	write32<I>(MEMADDR(from, 4));
}

emitterT void eSSE4_PMOVSXDQ_XMM_to_XMM(x86SSERegType to, x86SSERegType from)
{
	write8<I>(0x66);
    RexRB(0, to, from);
	write24<I>(0x25380F);
	ModRM<I>(3, to, from);
}

emitterT void eSSE4_PINSRD_R32_to_XMM(x86SSERegType to, x86IntRegType from, u8 imm8)
{
	write8<I>(0x66);
    RexRB(0, to, from);
	write24<I>(0x223A0F);
	ModRM<I>(3, to, from);
	write8<I>(imm8);
}

emitterT void eSSE4_PMAXSD_XMM_to_XMM(x86SSERegType to, x86SSERegType from)
{
	write8<I>(0x66);
	RexRB(0, to, from);
	write24<I>(0x3D380F);
	ModRM<I>(3, to, from);
}

emitterT void eSSE4_PMINSD_XMM_to_XMM(x86SSERegType to, x86SSERegType from)
{
	write8<I>(0x66);
	RexRB(0, to, from);
	write24<I>(0x39380F);
	ModRM<I>(3, to, from);
}

emitterT void eSSE4_PMAXUD_XMM_to_XMM(x86SSERegType to, x86SSERegType from)
{
	write8<I>(0x66);
	RexRB(0, to, from);
	write24<I>(0x3F380F);
	ModRM<I>(3, to, from);
}

emitterT void eSSE4_PMINUD_XMM_to_XMM(x86SSERegType to, x86SSERegType from)
{
	write8<I>(0x66);
	RexRB(0, to, from);
	write24<I>(0x3B380F);
	ModRM<I>(3, to, from);
}

emitterT void eSSE4_PMAXSD_M128_to_XMM(x86SSERegType to, uptr from)
{
	write8<I>(0x66);
	RexR(0, to);
	write24<I>(0x3D380F);
	ModRM<I>( 0, to, DISP32 );
	write32<I>(MEMADDR(from, 4));
}

emitterT void eSSE4_PMINSD_M128_to_XMM(x86SSERegType to, uptr from)
{
	write8<I>(0x66);
	RexR(0, to);
	write24<I>(0x39380F);
	ModRM<I>( 0, to, DISP32 );
	write32<I>(MEMADDR(from, 4));
}

emitterT void eSSE4_PMAXUD_M128_to_XMM(x86SSERegType to, uptr from)
{
	write8<I>(0x66);
	RexR(0, to);
	write24<I>(0x3F380F);
	ModRM<I>( 0, to, DISP32 );
	write32<I>(MEMADDR(from, 4));
}

emitterT void eSSE4_PMINUD_M128_to_XMM(x86SSERegType to, uptr from)
{
	write8<I>(0x66);
	RexR(0, to);
	write24<I>(0x3B380F);
	ModRM<I>( 0, to, DISP32 );
	write32<I>(MEMADDR(from, 4));
}

emitterT void eSSE4_PMULDQ_XMM_to_XMM(x86SSERegType to, x86SSERegType from)
{
	write8<I>(0x66);
	RexRB(0, to, from);
	write24<I>(0x28380F);
	ModRM<I>(3, to, from);
}
