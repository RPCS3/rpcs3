/*  Pcsx2 - Pc Ps2 Emulator
 *  Copyright (C) 2002-2008  Pcsx2 Team
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
#include "ix86.h"

PCSX2_ALIGNED16(unsigned int p[4]);
PCSX2_ALIGNED16(unsigned int p2[4]);
PCSX2_ALIGNED16(float f[4]);


XMMSSEType g_xmmtypes[XMMREGS] = { XMMT_INT };

/********************/
/* SSE instructions */
/********************/

#define SSEMtoR( code, overb ) \
	assert( to < XMMREGS ) ; \
	RexR(0, to);             \
	write16( code ); \
	ModRM( 0, to, DISP32 ); \
	write32( MEMADDR(from, 4 + overb) ); \

#define SSERtoM( code, overb ) \
	assert( from < XMMREGS) ; \
    RexR(0, from);  \
	write16( code ); \
	ModRM( 0, from, DISP32 ); \
	write32( MEMADDR(to, 4 + overb) ); \

#define SSE_SS_MtoR( code, overb ) \
	assert( to < XMMREGS ) ; \
	write8( 0xf3 ); \
    RexR(0, to);                      \
	write16( code ); \
	ModRM( 0, to, DISP32 ); \
	write32( MEMADDR(from, 4 + overb) ); \

#define SSE_SS_RtoM( code, overb ) \
	assert( from < XMMREGS) ; \
	write8( 0xf3 ); \
	RexR(0, from); \
	write16( code ); \
	ModRM( 0, from, DISP32 ); \
	write32( MEMADDR(to, 4 + overb) ); \

#define SSERtoR( code ) \
	assert( to < XMMREGS && from < XMMREGS) ; \
    RexRB(0, to, from);            \
	write16( code ); \
	ModRM( 3, to, from );

#define SSEMtoR66( code ) \
	write8( 0x66 ); \
	SSEMtoR( code, 0 );

#define SSERtoM66( code ) \
	write8( 0x66 ); \
	SSERtoM( code, 0 );

#define SSERtoR66( code ) \
	write8( 0x66 ); \
	SSERtoR( code );

#define _SSERtoR66( code ) \
	assert( to < XMMREGS && from < XMMREGS) ; \
	write8( 0x66 ); \
	RexRB(0, from, to); \
	write16( code ); \
	ModRM( 3, from, to );

#define SSE_SS_RtoR( code ) \
	assert( to < XMMREGS && from < XMMREGS) ; \
	write8( 0xf3 ); \
    RexRB(0, to, from);              \
	write16( code ); \
	ModRM( 3, to, from );

#define CMPPSMtoR( op ) \
   SSEMtoR( 0xc20f, 1 ); \
   write8( op );

#define CMPPSRtoR( op ) \
   SSERtoR( 0xc20f ); \
   write8( op );

#define CMPSSMtoR( op ) \
   SSE_SS_MtoR( 0xc20f, 1 ); \
   write8( op );

#define CMPSSRtoR( op ) \
   SSE_SS_RtoR( 0xc20f ); \
   write8( op );



void WriteRmOffset(x86IntRegType to, int offset);
void WriteRmOffsetFrom(x86IntRegType to, x86IntRegType from, int offset);

/* movups [r32][r32*scale] to xmm1 */
__forceinline void SSE_MOVUPSRmStoR( x86SSERegType to, x86IntRegType from, x86IntRegType from2, int scale )
{
    RexRXB(0, to, from2, from);
	write16( 0x100f );
	ModRM( 0, to, 0x4 );
	SibSB( scale, from2, from );
}

/* movups xmm1 to [r32][r32*scale] */
__forceinline void SSE_MOVUPSRtoRmS( x86SSERegType to, x86IntRegType from, x86IntRegType from2, int scale )
{
    RexRXB(1, to, from2, from);
	write16( 0x110f );
	ModRM( 0, to, 0x4 );
	SibSB( scale, from2, from );
}

/* movups [r32] to r32 */
__forceinline void SSE_MOVUPSRmtoR( x86IntRegType to, x86IntRegType from ) 
{
	RexRB(0, to, from);
	write16( 0x100f );
	ModRM( 0, to, from );
}

/* movups r32 to [r32] */
__forceinline void SSE_MOVUPSRtoRm( x86IntRegType to, x86IntRegType from ) 
{
    RexRB(0, from, to);
	write16( 0x110f );
	ModRM( 0, from, to );
}

/* movlps [r32] to r32 */
__forceinline void SSE_MOVLPSRmtoR( x86SSERegType to, x86IntRegType from ) 
{
	RexRB(1, to, from);
	write16( 0x120f );
	ModRM( 0, to, from );
}

__forceinline void SSE_MOVLPSRmtoROffset( x86SSERegType to, x86IntRegType from, int offset )
{
    RexRB(0, to, from);
	write16( 0x120f );
    WriteRmOffsetFrom(to, from, offset);
}

/* movaps r32 to [r32] */
__forceinline void SSE_MOVLPSRtoRm( x86IntRegType to, x86IntRegType from ) 
{
    RexRB(0, from, to);
	write16( 0x130f );
	ModRM( 0, from, to );
}

__forceinline void SSE_MOVLPSRtoRmOffset( x86SSERegType to, x86IntRegType from, int offset )
{
    RexRB(0, from, to);
	write16( 0x130f );
    WriteRmOffsetFrom(from, to, offset);
}

/* movaps [r32][r32*scale] to xmm1 */
__forceinline void SSE_MOVAPSRmStoR( x86SSERegType to, x86IntRegType from, x86IntRegType from2, int scale )
{
	assert( from != EBP );
    RexRXB(0, to, from2, from);
	write16( 0x280f );
	ModRM( 0, to, 0x4 );
	SibSB( scale, from2, from );
}

/* movaps xmm1 to [r32][r32*scale] */
__forceinline void SSE_MOVAPSRtoRmS( x86SSERegType to, x86IntRegType from, x86IntRegType from2, int scale )
{
	assert( from != EBP );
    RexRXB(0, to, from2, from);
	write16( 0x290f );
	ModRM( 0, to, 0x4 );
	SibSB( scale, from2, from );
}

// movaps [r32+offset] to r32
__forceinline void SSE_MOVAPSRmtoROffset( x86SSERegType to, x86IntRegType from, int offset )
{
	RexRB(0, to, from);
	write16( 0x280f );
    WriteRmOffsetFrom(to, from, offset);
}

// movaps r32 to [r32+offset]
__forceinline void SSE_MOVAPSRtoRmOffset( x86IntRegType to, x86SSERegType from, int offset ) 
{
	RexRB(0, from, to);
	write16( 0x290f );
    WriteRmOffsetFrom(from, to, offset);
}

// movdqa [r32+offset] to r32
__forceinline void SSE2_MOVDQARmtoROffset( x86SSERegType to, x86IntRegType from, int offset )
{
	write8(0x66);
    RexRB(0, to, from);
	write16( 0x6f0f );
    WriteRmOffsetFrom(to, from, offset);
}

// movdqa r32 to [r32+offset]
__forceinline void SSE2_MOVDQARtoRmOffset( x86IntRegType to, x86SSERegType from, int offset ) 
{
	write8(0x66);
    RexRB(0, from, to);
	write16( 0x7f0f );
    WriteRmOffsetFrom(from, to, offset);
}

// movups [r32+offset] to r32
__forceinline void SSE_MOVUPSRmtoROffset( x86SSERegType to, x86IntRegType from, int offset )
{
	RexRB(0, to, from);
	write16( 0x100f );
    WriteRmOffsetFrom(to, from, offset);
}

// movups r32 to [r32+offset]
__forceinline void SSE_MOVUPSRtoRmOffset( x86SSERegType to, x86IntRegType from, int offset )
{
    RexRB(0, from, to);
	write16( 0x110f );
    WriteRmOffsetFrom(from, to, offset);
}

//**********************************************************************************/
//MOVAPS: Move aligned Packed Single Precision FP values                           *
//**********************************************************************************
__forceinline void SSE_MOVAPS_M128_to_XMM( x86SSERegType to, uptr from )          { SSEMtoR( 0x280f, 0 ); }
__forceinline void SSE_MOVAPS_XMM_to_M128( uptr to, x86SSERegType from )          { SSERtoM( 0x290f, 0 ); }
__forceinline void SSE_MOVAPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from )  { if (to != from) { SSERtoR( 0x280f ); } }

__forceinline void SSE_MOVUPS_M128_to_XMM( x86SSERegType to, uptr from )          { SSEMtoR( 0x100f, 0 ); }
__forceinline void SSE_MOVUPS_XMM_to_M128( uptr to, x86SSERegType from )          { SSERtoM( 0x110f, 0 ); }

__forceinline void SSE2_MOVSD_XMM_to_XMM( x86SSERegType to, x86SSERegType from )
{
	write8(0xf2);
	SSERtoR( 0x100f);
}

__forceinline void SSE2_MOVQ_M64_to_XMM( x86SSERegType to, uptr from )
{
	write8(0xf3); SSEMtoR( 0x7e0f, 0);
}

__forceinline void SSE2_MOVQ_XMM_to_XMM( x86SSERegType to, x86SSERegType from )
{
	write8(0xf3); SSERtoR( 0x7e0f);
}

__forceinline void SSE2_MOVQ_XMM_to_M64( u32 to, x86SSERegType from )
{
	SSERtoM66(0xd60f);
}

__forceinline void SSE2_MOVDQ2Q_XMM_to_MM( x86MMXRegType to, x86SSERegType from)
{
	write8(0xf2);
	SSERtoR( 0xd60f);
}
__forceinline void SSE2_MOVQ2DQ_MM_to_XMM( x86SSERegType to, x86MMXRegType from)
{
	write8(0xf3);
	SSERtoR( 0xd60f);
}

//**********************************************************************************/
//MOVSS: Move Scalar Single-Precision FP  value                                    *
//**********************************************************************************
__forceinline void SSE_MOVSS_M32_to_XMM( x86SSERegType to, uptr from )           { SSE_SS_MtoR( 0x100f, 0 ); }
__forceinline void SSE_MOVSS_XMM_to_M32( u32 to, x86SSERegType from )           { SSE_SS_RtoM( 0x110f, 0 ); }
__forceinline void SSE_MOVSS_XMM_to_Rm( x86IntRegType to, x86SSERegType from )
{
	write8(0xf3);
    RexRB(0, from, to);
    write16(0x110f);
	ModRM(0, from, to);
}

__forceinline void SSE_MOVSS_XMM_to_XMM( x86SSERegType to, x86SSERegType from )      { if (to != from) { SSE_SS_RtoR( 0x100f ); } }

__forceinline void SSE_MOVSS_RmOffset_to_XMM( x86SSERegType to, x86IntRegType from, int offset )
{
	write8(0xf3);
    RexRB(0, to, from);
    write16( 0x100f );
    WriteRmOffsetFrom(to, from, offset);
}

__forceinline void SSE_MOVSS_XMM_to_RmOffset( x86IntRegType to, x86SSERegType from, int offset )
{
	write8(0xf3);
    RexRB(0, from, to);
    write16(0x110f);
    WriteRmOffsetFrom(from, to, offset);
}

__forceinline void SSE_MASKMOVDQU_XMM_to_XMM( x86SSERegType to, x86SSERegType from )      { SSERtoR66( 0xf70f ); }
//**********************************************************************************/
//MOVLPS: Move low Packed Single-Precision FP                                     *
//**********************************************************************************
__forceinline void SSE_MOVLPS_M64_to_XMM( x86SSERegType to, uptr from )          { SSEMtoR( 0x120f, 0 ); }
__forceinline void SSE_MOVLPS_XMM_to_M64( u32 to, x86SSERegType from )          { SSERtoM( 0x130f, 0 ); }

__forceinline void SSE_MOVLPS_RmOffset_to_XMM( x86SSERegType to, x86IntRegType from, int offset )
{
    RexRB(0, to, from);
	write16( 0x120f );
    WriteRmOffsetFrom(to, from, offset);
}

__forceinline void SSE_MOVLPS_XMM_to_RmOffset( x86IntRegType to, x86SSERegType from, int offset )
{
    RexRB(0, from, to);
	write16(0x130f);
    WriteRmOffsetFrom(from, to, offset);
}

/////////////////////////////////////////////////////////////////////////////////////
//**********************************************************************************/
//MOVHPS: Move High Packed Single-Precision FP                                     *
//**********************************************************************************
__forceinline void SSE_MOVHPS_M64_to_XMM( x86SSERegType to, uptr from )          { SSEMtoR( 0x160f, 0 ); }
__forceinline void SSE_MOVHPS_XMM_to_M64( u32 to, x86SSERegType from )          { SSERtoM( 0x170f, 0 ); }

__forceinline void SSE_MOVHPS_RmOffset_to_XMM( x86SSERegType to, x86IntRegType from, int offset )
{
    RexRB(0, to, from);
	write16( 0x160f );
    WriteRmOffsetFrom(to, from, offset);
}

__forceinline void SSE_MOVHPS_XMM_to_RmOffset( x86IntRegType to, x86SSERegType from, int offset )
{
    RexRB(0, from, to);
	write16(0x170f);
    WriteRmOffsetFrom(from, to, offset);
}

/////////////////////////////////////////////////////////////////////////////////////
//**********************************************************************************/
//MOVLHPS: Moved packed Single-Precision FP low to high                            *
//**********************************************************************************
__forceinline void SSE_MOVLHPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from )    { SSERtoR( 0x160f ); }

//////////////////////////////////////////////////////////////////////////////////////
//**********************************************************************************/
//MOVHLPS: Moved packed Single-Precision FP High to Low                            *
//**********************************************************************************
__forceinline void SSE_MOVHLPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from )    { SSERtoR( 0x120f ); }

///////////////////////////////////////////////////////////////////////////////////
//**********************************************************************************/
//ANDPS: Logical Bit-wise  AND for Single FP                                        *
//**********************************************************************************
__forceinline void SSE_ANDPS_M128_to_XMM( x86SSERegType to, uptr from )           { SSEMtoR( 0x540f, 0 ); }
__forceinline void SSE_ANDPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from ) { SSERtoR( 0x540f ); }

///////////////////////////////////////////////////////////////////////////////////////
//**********************************************************************************/
//ANDNPS : Logical Bit-wise  AND NOT of Single-precision FP values                 *
//**********************************************************************************
__forceinline void SSE_ANDNPS_M128_to_XMM( x86SSERegType to, uptr from )          { SSEMtoR( 0x550f, 0 ); }
__forceinline void SSE_ANDNPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from ){ SSERtoR( 0x550f ); }

/////////////////////////////////////////////////////////////////////////////////////
//**********************************************************************************/
//RCPPS : Packed Single-Precision FP Reciprocal                                     *
//**********************************************************************************
__forceinline void SSE_RCPPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from ) { SSERtoR( 0x530f ); }
__forceinline void SSE_RCPPS_M128_to_XMM( x86SSERegType to, uptr from )           { SSEMtoR( 0x530f, 0 ); }

__forceinline void SSE_RCPSS_XMM_to_XMM( x86SSERegType to, x86SSERegType from ) { SSE_SS_RtoR(0x530f); }
__forceinline void SSE_RCPSS_M32_to_XMM( x86SSERegType to, uptr from ) { SSE_SS_MtoR(0x530f, 0); }

//////////////////////////////////////////////////////////////////////////////////////
//**********************************************************************************/
//ORPS : Bit-wise Logical OR of Single-Precision FP Data                            *
//**********************************************************************************
__forceinline void SSE_ORPS_M128_to_XMM( x86SSERegType to, uptr from )            { SSEMtoR( 0x560f, 0 ); }
__forceinline void SSE_ORPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from )  { SSERtoR( 0x560f ); }

/////////////////////////////////////////////////////////////////////////////////////
//**********************************************************************************/
//XORPS : Bitwise Logical XOR of Single-Precision FP Values                        *
//**********************************************************************************

__forceinline void SSE_XORPS_M128_to_XMM( x86SSERegType to, uptr from )           { SSEMtoR( 0x570f, 0 ); }
__forceinline void SSE_XORPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from ) { SSERtoR( 0x570f ); }

///////////////////////////////////////////////////////////////////////////////////////
//**********************************************************************************/
//ADDPS : ADD Packed Single-Precision FP Values                                    *
//**********************************************************************************
__forceinline void SSE_ADDPS_M128_to_XMM( x86SSERegType to, uptr from )           { SSEMtoR( 0x580f, 0 ); }
__forceinline void SSE_ADDPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from ) { SSERtoR( 0x580f ); }

////////////////////////////////////////////////////////////////////////////////////
//**********************************************************************************/
//ADDSS : ADD Scalar Single-Precision FP Values                                    *
//**********************************************************************************
__forceinline void SSE_ADDSS_M32_to_XMM( x86SSERegType to, uptr from )           { SSE_SS_MtoR( 0x580f, 0 ); }
__forceinline void SSE_ADDSS_XMM_to_XMM( x86SSERegType to, x86SSERegType from ) { SSE_SS_RtoR( 0x580f ); }

/////////////////////////////////////////////////////////////////////////////////////////
//**********************************************************************************/
//SUBPS: Packed Single-Precision FP Subtract                                       *
//**********************************************************************************
__forceinline void SSE_SUBPS_M128_to_XMM( x86SSERegType to, uptr from )           { SSEMtoR( 0x5c0f, 0 ); }
__forceinline void SSE_SUBPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from ) { SSERtoR( 0x5c0f ); }

///////////////////////////////////////////////////////////////////////////////////////
//**********************************************************************************/
//SUBSS : Scalar  Single-Precision FP Subtract                                       *
//**********************************************************************************
__forceinline void SSE_SUBSS_M32_to_XMM( x86SSERegType to, uptr from )           { SSE_SS_MtoR( 0x5c0f, 0 ); }
__forceinline void SSE_SUBSS_XMM_to_XMM( x86SSERegType to, x86SSERegType from ) { SSE_SS_RtoR( 0x5c0f ); }

/////////////////////////////////////////////////////////////////////////////////////////
//**********************************************************************************/
//MULPS : Packed Single-Precision FP Multiply                                      *
//**********************************************************************************
__forceinline void SSE_MULPS_M128_to_XMM( x86SSERegType to, uptr from )           { SSEMtoR( 0x590f, 0 ); }
__forceinline void SSE_MULPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from ) { SSERtoR( 0x590f ); }

////////////////////////////////////////////////////////////////////////////////////////
//**********************************************************************************/
//MULSS : Scalar  Single-Precision FP Multiply                                       *
//**********************************************************************************
__forceinline void SSE_MULSS_M32_to_XMM( x86SSERegType to, uptr from )           { SSE_SS_MtoR( 0x590f, 0 ); }
__forceinline void SSE_MULSS_XMM_to_XMM( x86SSERegType to, x86SSERegType from ) { SSE_SS_RtoR( 0x590f ); }

////////////////////////////////////////////////////////////////////////////////////////////
//**********************************************************************************/
//Packed Single-Precission FP compare (CMPccPS)                                    *
//**********************************************************************************
//missing  SSE_CMPPS_I8_to_XMM
//         SSE_CMPPS_M32_to_XMM
//	       SSE_CMPPS_XMM_to_XMM
__forceinline void SSE_CMPEQPS_M128_to_XMM( x86SSERegType to, uptr from )         { CMPPSMtoR( 0 ); }
__forceinline void SSE_CMPEQPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from )    { CMPPSRtoR( 0 ); }
__forceinline void SSE_CMPLTPS_M128_to_XMM( x86SSERegType to, uptr from )         { CMPPSMtoR( 1 ); }
__forceinline void SSE_CMPLTPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from )    { CMPPSRtoR( 1 ); }
__forceinline void SSE_CMPLEPS_M128_to_XMM( x86SSERegType to, uptr from )         { CMPPSMtoR( 2 ); }
__forceinline void SSE_CMPLEPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from )    { CMPPSRtoR( 2 ); }
__forceinline void SSE_CMPUNORDPS_M128_to_XMM( x86SSERegType to, uptr from )      { CMPPSMtoR( 3 ); }
__forceinline void SSE_CMPUNORDPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from ) { CMPPSRtoR( 3 ); }
__forceinline void SSE_CMPNEPS_M128_to_XMM( x86SSERegType to, uptr from )         { CMPPSMtoR( 4 ); }
__forceinline void SSE_CMPNEPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from )    { CMPPSRtoR( 4 ); }
__forceinline void SSE_CMPNLTPS_M128_to_XMM( x86SSERegType to, uptr from )        { CMPPSMtoR( 5 ); }
__forceinline void SSE_CMPNLTPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from )   { CMPPSRtoR( 5 ); }
__forceinline void SSE_CMPNLEPS_M128_to_XMM( x86SSERegType to, uptr from )        { CMPPSMtoR( 6 ); }
__forceinline void SSE_CMPNLEPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from )   { CMPPSRtoR( 6 ); }
__forceinline void SSE_CMPORDPS_M128_to_XMM( x86SSERegType to, uptr from )        { CMPPSMtoR( 7 ); }
__forceinline void SSE_CMPORDPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from )   { CMPPSRtoR( 7 ); }

///////////////////////////////////////////////////////////////////////////////////////////
//**********************************************************************************/
//Scalar Single-Precission FP compare (CMPccSS)                                    *
//**********************************************************************************
//missing  SSE_CMPSS_I8_to_XMM
//         SSE_CMPSS_M32_to_XMM
//	       SSE_CMPSS_XMM_to_XMM
__forceinline void SSE_CMPEQSS_M32_to_XMM( x86SSERegType to, uptr from )         { CMPSSMtoR( 0 ); }
__forceinline void SSE_CMPEQSS_XMM_to_XMM( x86SSERegType to, x86SSERegType from )    { CMPSSRtoR( 0 ); }
__forceinline void SSE_CMPLTSS_M32_to_XMM( x86SSERegType to, uptr from )         { CMPSSMtoR( 1 ); }
__forceinline void SSE_CMPLTSS_XMM_to_XMM( x86SSERegType to, x86SSERegType from )    { CMPSSRtoR( 1 ); }
__forceinline void SSE_CMPLESS_M32_to_XMM( x86SSERegType to, uptr from )         { CMPSSMtoR( 2 ); }
__forceinline void SSE_CMPLESS_XMM_to_XMM( x86SSERegType to, x86SSERegType from )    { CMPSSRtoR( 2 ); }
__forceinline void SSE_CMPUNORDSS_M32_to_XMM( x86SSERegType to, uptr from )      { CMPSSMtoR( 3 ); }
__forceinline void SSE_CMPUNORDSS_XMM_to_XMM( x86SSERegType to, x86SSERegType from ) { CMPSSRtoR( 3 ); }
__forceinline void SSE_CMPNESS_M32_to_XMM( x86SSERegType to, uptr from )         { CMPSSMtoR( 4 ); }
__forceinline void SSE_CMPNESS_XMM_to_XMM( x86SSERegType to, x86SSERegType from )    { CMPSSRtoR( 4 ); }
__forceinline void SSE_CMPNLTSS_M32_to_XMM( x86SSERegType to, uptr from )        { CMPSSMtoR( 5 ); }
__forceinline void SSE_CMPNLTSS_XMM_to_XMM( x86SSERegType to, x86SSERegType from )   { CMPSSRtoR( 5 ); }
__forceinline void SSE_CMPNLESS_M32_to_XMM( x86SSERegType to, uptr from )        { CMPSSMtoR( 6 ); }
__forceinline void SSE_CMPNLESS_XMM_to_XMM( x86SSERegType to, x86SSERegType from )   { CMPSSRtoR( 6 ); }
__forceinline void SSE_CMPORDSS_M32_to_XMM( x86SSERegType to, uptr from )        { CMPSSMtoR( 7 ); }
__forceinline void SSE_CMPORDSS_XMM_to_XMM( x86SSERegType to, x86SSERegType from )   { CMPSSRtoR( 7 ); }

__forceinline void SSE_UCOMISS_M32_to_XMM( x86SSERegType to, uptr from )
{
    RexR(0, to);
	write16( 0x2e0f );
	ModRM( 0, to, DISP32 );
	write32( MEMADDR(from, 4) );
}

__forceinline void SSE_UCOMISS_XMM_to_XMM( x86SSERegType to, x86SSERegType from )
{
    RexRB(0, to, from);
	write16( 0x2e0f );
	ModRM( 3, to, from );
}

//////////////////////////////////////////////////////////////////////////////////////////
//**********************************************************************************/
//RSQRTPS : Packed Single-Precision FP Square Root Reciprocal                      *
//**********************************************************************************
__forceinline void SSE_RSQRTPS_M128_to_XMM( x86SSERegType to, uptr from )          { SSEMtoR( 0x520f, 0 ); }
__forceinline void SSE_RSQRTPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from ){ SSERtoR( 0x520f ); }

/////////////////////////////////////////////////////////////////////////////////////
//**********************************************************************************/
//RSQRTSS : Scalar Single-Precision FP Square Root Reciprocal                      *
//**********************************************************************************
__forceinline void SSE_RSQRTSS_M32_to_XMM( x86SSERegType to, uptr from )          { SSE_SS_MtoR( 0x520f, 0 ); }
__forceinline void SSE_RSQRTSS_XMM_to_XMM( x86SSERegType to, x86SSERegType from ){ SSE_SS_RtoR( 0x520f ); }

////////////////////////////////////////////////////////////////////////////////////
//**********************************************************************************/
//SQRTPS : Packed Single-Precision FP Square Root                                  *
//**********************************************************************************
__forceinline void SSE_SQRTPS_M128_to_XMM( x86SSERegType to, uptr from )          { SSEMtoR( 0x510f, 0 ); }
__forceinline void SSE_SQRTPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from ){ SSERtoR( 0x510f ); }

//////////////////////////////////////////////////////////////////////////////////////
//**********************************************************************************/
//SQRTSS : Scalar Single-Precision FP Square Root                                  *
//**********************************************************************************
__forceinline void SSE_SQRTSS_M32_to_XMM( x86SSERegType to, uptr from )          { SSE_SS_MtoR( 0x510f, 0 ); }
__forceinline void SSE_SQRTSS_XMM_to_XMM( x86SSERegType to, x86SSERegType from ){ SSE_SS_RtoR( 0x510f ); }

////////////////////////////////////////////////////////////////////////////////////////
//**********************************************************************************/
//MAXPS: Return Packed Single-Precision FP Maximum                                 *
//**********************************************************************************
__forceinline void SSE_MAXPS_M128_to_XMM( x86SSERegType to, uptr from )           { SSEMtoR( 0x5f0f, 0 ); }
__forceinline void SSE_MAXPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from ) { SSERtoR( 0x5f0f ); }

/////////////////////////////////////////////////////////////////////////////////////////
//**********************************************************************************/
//MAXSS: Return Scalar Single-Precision FP Maximum                                 *
//**********************************************************************************
__forceinline void SSE_MAXSS_M32_to_XMM( x86SSERegType to, uptr from )           { SSE_SS_MtoR( 0x5f0f, 0 ); }
__forceinline void SSE_MAXSS_XMM_to_XMM( x86SSERegType to, x86SSERegType from ) { SSE_SS_RtoR( 0x5f0f ); }

/////////////////////////////////////////////////////////////////////////////////////////
//**********************************************************************************/
//CVTPI2PS: Packed Signed INT32 to Packed Single  FP Conversion                    *
//**********************************************************************************
__forceinline void SSE_CVTPI2PS_M64_to_XMM( x86SSERegType to, uptr from )        { SSEMtoR( 0x2a0f, 0 ); }
__forceinline void SSE_CVTPI2PS_MM_to_XMM( x86SSERegType to, x86MMXRegType from )   { SSERtoR( 0x2a0f ); }

///////////////////////////////////////////////////////////////////////////////////////////
//**********************************************************************************/
//CVTPS2PI: Packed Single FP to Packed Signed INT32 Conversion                      *
//**********************************************************************************
__forceinline void SSE_CVTPS2PI_M64_to_MM( x86MMXRegType to, uptr from )        { SSEMtoR( 0x2d0f, 0 ); }
__forceinline void SSE_CVTPS2PI_XMM_to_MM( x86MMXRegType to, x86SSERegType from )   { SSERtoR( 0x2d0f ); }

__forceinline void SSE_CVTTSS2SI_M32_to_R32(x86IntRegType to, uptr from) { write8(0xf3); SSEMtoR(0x2c0f, 0); }
__forceinline void SSE_CVTTSS2SI_XMM_to_R32(x86IntRegType to, x86SSERegType from)
{
	write8(0xf3);
    RexRB(0, to, from);
	write16(0x2c0f);
	ModRM(3, to, from);
}

__forceinline void SSE_CVTSI2SS_M32_to_XMM(x86SSERegType to, uptr from) { write8(0xf3); SSEMtoR(0x2a0f, 0); }
__forceinline void SSE_CVTSI2SS_R_to_XMM(x86SSERegType to, x86IntRegType from)
{
	write8(0xf3);
    RexRB(0, to, from);
	write16(0x2a0f);
	ModRM(3, to, from);
}

///////////////////////////////////////////////////////////////////////////////////////////
//**********************************************************************************/
//CVTDQ2PS: Packed Signed INT32  to Packed Single Precision FP  Conversion         *
//**********************************************************************************
__forceinline void SSE2_CVTDQ2PS_M128_to_XMM( x86SSERegType to, uptr from )        { SSEMtoR( 0x5b0f, 0 ); }
__forceinline void SSE2_CVTDQ2PS_XMM_to_XMM( x86SSERegType to, x86SSERegType from )   { SSERtoR( 0x5b0f ); }

//**********************************************************************************/
//CVTPS2DQ: Packed Single Precision FP to Packed Signed INT32 Conversion           *
//**********************************************************************************
__forceinline void SSE2_CVTPS2DQ_M128_to_XMM( x86SSERegType to, uptr from )        { SSEMtoR66( 0x5b0f ); }
__forceinline void SSE2_CVTPS2DQ_XMM_to_XMM( x86SSERegType to, x86SSERegType from )   { SSERtoR66( 0x5b0f ); }

__forceinline void SSE2_CVTTPS2DQ_XMM_to_XMM( x86SSERegType to, x86SSERegType from ) { write8(0xf3); SSERtoR(0x5b0f); }
/////////////////////////////////////////////////////////////////////////////////////
//**********************************************************************************/
//MINPS: Return Packed Single-Precision FP Minimum                                 *
//**********************************************************************************
__forceinline void SSE_MINPS_M128_to_XMM( x86SSERegType to, uptr from )           { SSEMtoR( 0x5d0f, 0 ); }
__forceinline void SSE_MINPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from ) { SSERtoR( 0x5d0f ); }

//////////////////////////////////////////////////////////////////////////////////////////
//**********************************************************************************/
//MINSS: Return Scalar Single-Precision FP Minimum                                 *
//**********************************************************************************
__forceinline void SSE_MINSS_M32_to_XMM( x86SSERegType to, uptr from )           { SSE_SS_MtoR( 0x5d0f, 0 ); }
__forceinline void SSE_MINSS_XMM_to_XMM( x86SSERegType to, x86SSERegType from ) { SSE_SS_RtoR( 0x5d0f ); }

///////////////////////////////////////////////////////////////////////////////////////////
//**********************************************************************************/
//PMAXSW: Packed Signed Integer Word Maximum                                        *
//**********************************************************************************
//missing
 //     SSE_PMAXSW_M64_to_MM
//		SSE2_PMAXSW_M128_to_XMM
//		SSE2_PMAXSW_XMM_to_XMM
__forceinline void SSE_PMAXSW_MM_to_MM( x86MMXRegType to, x86MMXRegType from ){ SSERtoR( 0xEE0F ); }

///////////////////////////////////////////////////////////////////////////////////////
//**********************************************************************************/
//PMINSW: Packed Signed Integer Word Minimum                                        *
//**********************************************************************************
//missing
 //     SSE_PMINSW_M64_to_MM
//		SSE2_PMINSW_M128_to_XMM
//		SSE2_PMINSW_XMM_to_XMM
__forceinline void SSE_PMINSW_MM_to_MM( x86MMXRegType to, x86MMXRegType from ){ SSERtoR( 0xEA0F ); }

//////////////////////////////////////////////////////////////////////////////////////
//**********************************************************************************/
//SHUFPS: Shuffle Packed Single-Precision FP Values                                *
//**********************************************************************************
__forceinline void SSE_SHUFPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from, u8 imm8 )	{ SSERtoR( 0xC60F ); write8( imm8 ); }
__forceinline void SSE_SHUFPS_M128_to_XMM( x86SSERegType to, uptr from, u8 imm8 )		{ SSEMtoR( 0xC60F, 1 ); write8( imm8 ); }

__forceinline void SSE_SHUFPS_RmOffset_to_XMM( x86SSERegType to, x86IntRegType from, int offset, u8 imm8 )
{
    RexRB(0, to, from);
	write16(0xc60f);
    WriteRmOffsetFrom(to, from, offset);
	write8(imm8);
}

////////////////////////////////////////////////////////////////////////////////////
//**********************************************************************************/
//PSHUFD: Shuffle Packed DoubleWords                                               *
//**********************************************************************************
__forceinline void SSE2_PSHUFD_XMM_to_XMM( x86SSERegType to, x86SSERegType from, u8 imm8 )
{
	SSERtoR66( 0x700F );
	write8( imm8 );
}
__forceinline void SSE2_PSHUFD_M128_to_XMM( x86SSERegType to, uptr from, u8 imm8 )	{ SSEMtoR66( 0x700F ); write8( imm8 ); }

__forceinline void SSE2_PSHUFLW_XMM_to_XMM( x86SSERegType to, x86SSERegType from, u8 imm8 ) { write8(0xF2); SSERtoR(0x700F); write8(imm8); }
__forceinline void SSE2_PSHUFLW_M128_to_XMM( x86SSERegType to, uptr from, u8 imm8 ) { write8(0xF2); SSEMtoR(0x700F, 1); write8(imm8); }
__forceinline void SSE2_PSHUFHW_XMM_to_XMM( x86SSERegType to, x86SSERegType from, u8 imm8 ) { write8(0xF3); SSERtoR(0x700F); write8(imm8); }
__forceinline void SSE2_PSHUFHW_M128_to_XMM( x86SSERegType to, uptr from, u8 imm8 ) { write8(0xF3); SSEMtoR(0x700F, 1); write8(imm8); }

///////////////////////////////////////////////////////////////////////////////////
//**********************************************************************************/
//UNPCKLPS: Unpack and Interleave low Packed Single-Precision FP Data              *
//**********************************************************************************
__forceinline void SSE_UNPCKLPS_M128_to_XMM( x86SSERegType to, uptr from ) { SSEMtoR(0x140f, 0); }
__forceinline void SSE_UNPCKLPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from )	{ SSERtoR( 0x140F ); }

////////////////////////////////////////////////////////////////////////////////////////
//**********************************************************************************/
//UNPCKHPS: Unpack and Interleave High Packed Single-Precision FP Data              *
//**********************************************************************************
__forceinline void SSE_UNPCKHPS_M128_to_XMM( x86SSERegType to, uptr from ) { SSEMtoR(0x150f, 0); }
__forceinline void SSE_UNPCKHPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from )	{ SSERtoR( 0x150F ); }

////////////////////////////////////////////////////////////////////////////////////////
//**********************************************************************************/
//DIVPS : Packed Single-Precision FP Divide                                       *
//**********************************************************************************
__forceinline void SSE_DIVPS_M128_to_XMM( x86SSERegType to, uptr from )           { SSEMtoR( 0x5e0F, 0 ); }
__forceinline void SSE_DIVPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from ) { SSERtoR( 0x5e0F ); }

//////////////////////////////////////////////////////////////////////////////////////
//**********************************************************************************/
//DIVSS : Scalar  Single-Precision FP Divide                                       *
//**********************************************************************************
__forceinline void SSE_DIVSS_M32_to_XMM( x86SSERegType to, uptr from )           { SSE_SS_MtoR( 0x5e0F, 0 ); }
__forceinline void SSE_DIVSS_XMM_to_XMM( x86SSERegType to, x86SSERegType from ) { SSE_SS_RtoR( 0x5e0F ); }

/////////////////////////////////////////////////////////////////////////////////////////
//**********************************************************************************/
//STMXCSR : Store Streaming SIMD Extension Control/Status                         *
//**********************************************************************************
__forceinline void SSE_STMXCSR( uptr from ) {
	write16( 0xAE0F );
	ModRM( 0, 0x3, DISP32 );
	write32( MEMADDR(from, 4) );
}

/////////////////////////////////////////////////////////////////////////////////////
//**********************************************************************************/
//LDMXCSR : Load Streaming SIMD Extension Control/Status                         *
//**********************************************************************************
__forceinline void SSE_LDMXCSR( uptr from ) {
	write16( 0xAE0F );
	ModRM( 0, 0x2, DISP32 );
	write32( MEMADDR(from, 4) );
}

/////////////////////////////////////////////////////////////////////////////////////
//**********************************************************************************/
//PADDB,PADDW,PADDD : Add Packed Integers                                          *
//**********************************************************************************
__forceinline void SSE2_PADDB_XMM_to_XMM(x86SSERegType to, x86SSERegType from ){ SSERtoR66( 0xFC0F ); }
__forceinline void SSE2_PADDB_M128_to_XMM(x86SSERegType to, uptr from ){ SSEMtoR66( 0xFC0F ); }
__forceinline void SSE2_PADDW_XMM_to_XMM(x86SSERegType to, x86SSERegType from ){ SSERtoR66( 0xFD0F ); }
__forceinline void SSE2_PADDW_M128_to_XMM(x86SSERegType to, uptr from ){ SSEMtoR66( 0xFD0F ); }
__forceinline void SSE2_PADDD_XMM_to_XMM(x86SSERegType to, x86SSERegType from ){ SSERtoR66( 0xFE0F ); }
__forceinline void SSE2_PADDD_M128_to_XMM(x86SSERegType to, uptr from ){ SSEMtoR66( 0xFE0F ); }

__forceinline void SSE2_PADDQ_XMM_to_XMM(x86SSERegType to, x86SSERegType from ) { SSERtoR66( 0xD40F ); }
__forceinline void SSE2_PADDQ_M128_to_XMM(x86SSERegType to, uptr from ) { SSEMtoR66( 0xD40F ); }

///////////////////////////////////////////////////////////////////////////////////
//**********************************************************************************/
//PCMPxx: Compare Packed Integers                                                  *
//**********************************************************************************
__forceinline void SSE2_PCMPGTB_XMM_to_XMM(x86SSERegType to, x86SSERegType from ){ SSERtoR66( 0x640F ); }
__forceinline void SSE2_PCMPGTB_M128_to_XMM(x86SSERegType to, uptr from ){ SSEMtoR66( 0x640F ); }
__forceinline void SSE2_PCMPGTW_XMM_to_XMM(x86SSERegType to, x86SSERegType from ){ SSERtoR66( 0x650F ); }
__forceinline void SSE2_PCMPGTW_M128_to_XMM(x86SSERegType to, uptr from ){ SSEMtoR66( 0x650F ); }
__forceinline void SSE2_PCMPGTD_XMM_to_XMM(x86SSERegType to, x86SSERegType from ){ SSERtoR66( 0x660F ); }
__forceinline void SSE2_PCMPGTD_M128_to_XMM(x86SSERegType to, uptr from ){ SSEMtoR66( 0x660F ); }
__forceinline void SSE2_PCMPEQB_XMM_to_XMM(x86SSERegType to, x86SSERegType from ){ SSERtoR66( 0x740F ); }
__forceinline void SSE2_PCMPEQB_M128_to_XMM(x86SSERegType to, uptr from ){ SSEMtoR66( 0x740F ); }
__forceinline void SSE2_PCMPEQW_XMM_to_XMM(x86SSERegType to, x86SSERegType from ){ SSERtoR66( 0x750F ); }
__forceinline void SSE2_PCMPEQW_M128_to_XMM(x86SSERegType to, uptr from ){ SSEMtoR66( 0x750F ); }
__forceinline void SSE2_PCMPEQD_XMM_to_XMM(x86SSERegType to, x86SSERegType from )
{
	SSERtoR66( 0x760F );
}

__forceinline void SSE2_PCMPEQD_M128_to_XMM(x86SSERegType to, uptr from )
{
	SSEMtoR66( 0x760F );
}

////////////////////////////////////////////////////////////////////////////////////////////
//**********************************************************************************/
//PEXTRW,PINSRW: Packed Extract/Insert Word                                        *
//**********************************************************************************
__forceinline void SSE_PEXTRW_XMM_to_R32(x86IntRegType to, x86SSERegType from, u8 imm8 ){ SSERtoR66(0xC50F); write8( imm8 ); }
__forceinline void SSE_PINSRW_R32_to_XMM(x86SSERegType to, x86IntRegType from, u8 imm8 ){ SSERtoR66(0xC40F); write8( imm8 ); }

////////////////////////////////////////////////////////////////////////////////////////////
//**********************************************************************************/
//PSUBx: Subtract Packed Integers                                                  *
//**********************************************************************************
__forceinline void SSE2_PSUBB_XMM_to_XMM(x86SSERegType to, x86SSERegType from ){ SSERtoR66( 0xF80F ); }
__forceinline void SSE2_PSUBB_M128_to_XMM(x86SSERegType to, uptr from ){ SSEMtoR66( 0xF80F ); }
__forceinline void SSE2_PSUBW_XMM_to_XMM(x86SSERegType to, x86SSERegType from ){ SSERtoR66( 0xF90F ); }
__forceinline void SSE2_PSUBW_M128_to_XMM(x86SSERegType to, uptr from ){ SSEMtoR66( 0xF90F ); }
__forceinline void SSE2_PSUBD_XMM_to_XMM(x86SSERegType to, x86SSERegType from ){ SSERtoR66( 0xFA0F ); }
__forceinline void SSE2_PSUBD_M128_to_XMM(x86SSERegType to, uptr from ){ SSEMtoR66( 0xFA0F ); }
__forceinline void SSE2_PSUBQ_XMM_to_XMM(x86SSERegType to, x86SSERegType from ){ SSERtoR66( 0xFB0F ); }
__forceinline void SSE2_PSUBQ_M128_to_XMM(x86SSERegType to, uptr from ){ SSEMtoR66( 0xFB0F ); }

///////////////////////////////////////////////////////////////////////////////////////
//**********************************************************************************/
//MOVD: Move Dword(32bit) to /from XMM reg                                         *
//**********************************************************************************
__forceinline void SSE2_MOVD_M32_to_XMM( x86SSERegType to, uptr from ) { SSEMtoR66(0x6E0F); }
__forceinline void SSE2_MOVD_R_to_XMM( x86SSERegType to, x86IntRegType from )
{
	SSERtoR66(0x6E0F);
}

__forceinline void SSE2_MOVD_Rm_to_XMM( x86SSERegType to, x86IntRegType from )
{
	write8(0x66);
	RexRB(0, to, from);
	write16( 0x6e0f );
	ModRM( 0, to, from);
}

__forceinline void SSE2_MOVD_RmOffset_to_XMM( x86SSERegType to, x86IntRegType from, int offset )
{
	write8(0x66);
    RexRB(0, to, from);
	write16( 0x6e0f );
    WriteRmOffsetFrom(to, from, offset);
}

__forceinline void SSE2_MOVD_XMM_to_M32( u32 to, x86SSERegType from ) { SSERtoM66(0x7E0F); }
__forceinline void SSE2_MOVD_XMM_to_R( x86IntRegType to, x86SSERegType from ) 
{
	_SSERtoR66(0x7E0F);
}

__forceinline void SSE2_MOVD_XMM_to_Rm( x86IntRegType to, x86SSERegType from )
{
	write8(0x66);
    RexRB(0, from, to);
	write16( 0x7e0f );
	ModRM( 0, from, to );
}

__forceinline void SSE2_MOVD_XMM_to_RmOffset( x86IntRegType to, x86SSERegType from, int offset )
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
__forceinline void SSE2_POR_XMM_to_XMM( x86SSERegType to, x86SSERegType from ) { SSERtoR66( 0xEB0F ); }
__forceinline void SSE2_POR_M128_to_XMM( x86SSERegType to, uptr from ) { SSEMtoR66( 0xEB0F ); }

// logical and to &= from
__forceinline void SSE2_PAND_XMM_to_XMM( x86SSERegType to, x86SSERegType from ) { SSERtoR66( 0xDB0F ); }
__forceinline void SSE2_PAND_M128_to_XMM( x86SSERegType to, uptr from ) { SSEMtoR66( 0xDB0F ); }

// to = (~to) & from
__forceinline void SSE2_PANDN_XMM_to_XMM( x86SSERegType to, x86SSERegType from ) { SSERtoR66( 0xDF0F ); }
__forceinline void SSE2_PANDN_M128_to_XMM( x86SSERegType to, uptr from ) { SSEMtoR66( 0xDF0F ); }

/////////////////////////////////////////////////////////////////////////////////////
//**********************************************************************************/
//PXOR : SSE Bitwise XOR                                                             *
//**********************************************************************************
__forceinline void SSE2_PXOR_XMM_to_XMM( x86SSERegType to, x86SSERegType from ){ SSERtoR66( 0xEF0F ); }
__forceinline void SSE2_PXOR_M128_to_XMM( x86SSERegType to, uptr from ){ SSEMtoR66( 0xEF0F ) }; 
///////////////////////////////////////////////////////////////////////////////////////

__forceinline void SSE2_MOVDQA_M128_to_XMM(x86SSERegType to, uptr from) {SSEMtoR66(0x6F0F); }
__forceinline void SSE2_MOVDQA_XMM_to_M128( uptr to, x86SSERegType from ){SSERtoM66(0x7F0F);} 
__forceinline void SSE2_MOVDQA_XMM_to_XMM( x86SSERegType to, x86SSERegType from) { if (to != from) { SSERtoR66(0x6F0F); } }

__forceinline void SSE2_MOVDQU_M128_to_XMM(x86SSERegType to, uptr from) { write8(0xF3); SSEMtoR(0x6F0F, 0); }
__forceinline void SSE2_MOVDQU_XMM_to_M128( uptr to, x86SSERegType from) { write8(0xF3); SSERtoM(0x7F0F, 0); }
__forceinline void SSE2_MOVDQU_XMM_to_XMM( x86SSERegType to, x86SSERegType from) { write8(0xF3); SSERtoR(0x6F0F); }

// shift right logical

__forceinline void SSE2_PSRLW_XMM_to_XMM(x86SSERegType to, x86SSERegType from) { SSERtoR66(0xD10F); }
__forceinline void SSE2_PSRLW_M128_to_XMM(x86SSERegType to, uptr from) { SSEMtoR66(0xD10F); }
__forceinline void SSE2_PSRLW_I8_to_XMM(x86SSERegType to, u8 imm8)
{
	write8( 0x66 );
    RexB(0, to);
	write16( 0x710F );
	ModRM( 3, 2 , to );
	write8( imm8 );
}

__forceinline void SSE2_PSRLD_XMM_to_XMM(x86SSERegType to, x86SSERegType from) { SSERtoR66(0xD20F); }
__forceinline void SSE2_PSRLD_M128_to_XMM(x86SSERegType to, uptr from) { SSEMtoR66(0xD20F); }
__forceinline void SSE2_PSRLD_I8_to_XMM(x86SSERegType to, u8 imm8)
{
	write8( 0x66 );
    RexB(0, to);
	write16( 0x720F );
	ModRM( 3, 2 , to ); 
	write8( imm8 );
}

__forceinline void SSE2_PSRLQ_XMM_to_XMM(x86SSERegType to, x86SSERegType from) { SSERtoR66(0xD30F); }
__forceinline void SSE2_PSRLQ_M128_to_XMM(x86SSERegType to, uptr from) { SSEMtoR66(0xD30F); }
__forceinline void SSE2_PSRLQ_I8_to_XMM(x86SSERegType to, u8 imm8)
{
	write8( 0x66 );
    RexB(0, to);
	write16( 0x730F );
	ModRM( 3, 2 , to ); 
	write8( imm8 );
}

__forceinline void SSE2_PSRLDQ_I8_to_XMM(x86SSERegType to, u8 imm8)
{
	write8( 0x66 );
    RexB(0, to);
	write16( 0x730F );
	ModRM( 3, 3 , to ); 
	write8( imm8 );
}

// shift right arithmetic

__forceinline void SSE2_PSRAW_XMM_to_XMM(x86SSERegType to, x86SSERegType from) { SSERtoR66(0xE10F); }
__forceinline void SSE2_PSRAW_M128_to_XMM(x86SSERegType to, uptr from) { SSEMtoR66(0xE10F); }
__forceinline void SSE2_PSRAW_I8_to_XMM(x86SSERegType to, u8 imm8)
{
	write8( 0x66 );
    RexB(0, to);
	write16( 0x710F );
	ModRM( 3, 4 , to );
	write8( imm8 );
}

__forceinline void SSE2_PSRAD_XMM_to_XMM(x86SSERegType to, x86SSERegType from) { SSERtoR66(0xE20F); }
__forceinline void SSE2_PSRAD_M128_to_XMM(x86SSERegType to, uptr from) { SSEMtoR66(0xE20F); }
__forceinline void SSE2_PSRAD_I8_to_XMM(x86SSERegType to, u8 imm8)
{
	write8( 0x66 );
    RexB(0, to);
	write16( 0x720F );
	ModRM( 3, 4 , to );
	write8( imm8 );
}

// shift left logical

__forceinline void SSE2_PSLLW_XMM_to_XMM(x86SSERegType to, x86SSERegType from) { SSERtoR66(0xF10F); }
__forceinline void SSE2_PSLLW_M128_to_XMM(x86SSERegType to, uptr from) { SSEMtoR66(0xF10F); }
__forceinline void SSE2_PSLLW_I8_to_XMM(x86SSERegType to, u8 imm8)
{
	write8( 0x66 );
    RexB(0, to);
	write16( 0x710F );
	ModRM( 3, 6 , to );
	write8( imm8 );
}

__forceinline void SSE2_PSLLD_XMM_to_XMM(x86SSERegType to, x86SSERegType from) { SSERtoR66(0xF20F); }
__forceinline void SSE2_PSLLD_M128_to_XMM(x86SSERegType to, uptr from) { SSEMtoR66(0xF20F); }
__forceinline void SSE2_PSLLD_I8_to_XMM(x86SSERegType to, u8 imm8)
{
	write8( 0x66 );
    RexB(0, to);
	write16( 0x720F );
	ModRM( 3, 6 , to );
	write8( imm8 );
}

__forceinline void SSE2_PSLLQ_XMM_to_XMM(x86SSERegType to, x86SSERegType from) { SSERtoR66(0xF30F); }
__forceinline void SSE2_PSLLQ_M128_to_XMM(x86SSERegType to, uptr from) { SSEMtoR66(0xF30F); }
__forceinline void SSE2_PSLLQ_I8_to_XMM(x86SSERegType to, u8 imm8)
{
	write8( 0x66 );
    RexB(0, to);
	write16( 0x730F );
	ModRM( 3, 6 , to );
	write8( imm8 );
}

__forceinline void SSE2_PSLLDQ_I8_to_XMM(x86SSERegType to, u8 imm8)
{
	write8( 0x66 );
    RexB(0, to);
	write16( 0x730F );
	ModRM( 3, 7 , to ); 
	write8( imm8 );
}


__forceinline void SSE2_PMAXSW_XMM_to_XMM( x86SSERegType to, x86SSERegType from ){ SSERtoR66( 0xEE0F ); }
__forceinline void SSE2_PMAXSW_M128_to_XMM( x86SSERegType to, uptr from ){ SSEMtoR66( 0xEE0F ); }

__forceinline void SSE2_PMAXUB_XMM_to_XMM( x86SSERegType to, x86SSERegType from ){ SSERtoR66( 0xDE0F ); }
__forceinline void SSE2_PMAXUB_M128_to_XMM( x86SSERegType to, uptr from ){ SSEMtoR66( 0xDE0F ); }

__forceinline void SSE2_PMINSW_XMM_to_XMM( x86SSERegType to, x86SSERegType from ){ SSERtoR66( 0xEA0F ); }
__forceinline void SSE2_PMINSW_M128_to_XMM( x86SSERegType to, uptr from ){ SSEMtoR66( 0xEA0F ); }

__forceinline void SSE2_PMINUB_XMM_to_XMM( x86SSERegType to, x86SSERegType from ){ SSERtoR66( 0xDA0F ); }
__forceinline void SSE2_PMINUB_M128_to_XMM( x86SSERegType to, uptr from ){ SSEMtoR66( 0xDA0F ); }

//

__forceinline void SSE2_PADDSB_XMM_to_XMM( x86SSERegType to, x86SSERegType from ){ SSERtoR66( 0xEC0F ); }
__forceinline void SSE2_PADDSB_M128_to_XMM( x86SSERegType to, uptr from ){ SSEMtoR66( 0xEC0F ); }

__forceinline void SSE2_PADDSW_XMM_to_XMM( x86SSERegType to, x86SSERegType from ){ SSERtoR66( 0xED0F ); }
__forceinline void SSE2_PADDSW_M128_to_XMM( x86SSERegType to, uptr from ){ SSEMtoR66( 0xED0F ); }

__forceinline void SSE2_PSUBSB_XMM_to_XMM( x86SSERegType to, x86SSERegType from ){ SSERtoR66( 0xE80F ); }
__forceinline void SSE2_PSUBSB_M128_to_XMM( x86SSERegType to, uptr from ){ SSEMtoR66( 0xE80F ); }

__forceinline void SSE2_PSUBSW_XMM_to_XMM( x86SSERegType to, x86SSERegType from ){ SSERtoR66( 0xE90F ); }
__forceinline void SSE2_PSUBSW_M128_to_XMM( x86SSERegType to, uptr from ){ SSEMtoR66( 0xE90F ); }

__forceinline void SSE2_PSUBUSB_XMM_to_XMM( x86SSERegType to, x86SSERegType from ) { SSERtoR66( 0xD80F ); }
__forceinline void SSE2_PSUBUSB_M128_to_XMM( x86SSERegType to, uptr from ) { SSEMtoR66( 0xD80F ); }
__forceinline void SSE2_PSUBUSW_XMM_to_XMM( x86SSERegType to, x86SSERegType from ) { SSERtoR66( 0xD90F ); }
__forceinline void SSE2_PSUBUSW_M128_to_XMM( x86SSERegType to, uptr from ) { SSEMtoR66( 0xD90F ); }

__forceinline void SSE2_PADDUSB_XMM_to_XMM( x86SSERegType to, x86SSERegType from ) { SSERtoR66( 0xDC0F ); }
__forceinline void SSE2_PADDUSB_M128_to_XMM( x86SSERegType to, uptr from ) { SSEMtoR66( 0xDC0F ); }
__forceinline void SSE2_PADDUSW_XMM_to_XMM( x86SSERegType to, x86SSERegType from ) { SSERtoR66( 0xDD0F ); }
__forceinline void SSE2_PADDUSW_M128_to_XMM( x86SSERegType to, uptr from ) { SSEMtoR66( 0xDD0F ); }

//**********************************************************************************/
//PACKSSWB,PACKSSDW: Pack Saturate Signed Word
//**********************************************************************************
__forceinline void SSE2_PACKSSWB_XMM_to_XMM(x86SSERegType to, x86SSERegType from) { SSERtoR66( 0x630F ); }
__forceinline void SSE2_PACKSSWB_M128_to_XMM(x86SSERegType to, uptr from) { SSEMtoR66( 0x630F ); }
__forceinline void SSE2_PACKSSDW_XMM_to_XMM(x86SSERegType to, x86SSERegType from) { SSERtoR66( 0x6B0F ); }
__forceinline void SSE2_PACKSSDW_M128_to_XMM(x86SSERegType to, uptr from) { SSEMtoR66( 0x6B0F ); }

__forceinline void SSE2_PACKUSWB_XMM_to_XMM(x86SSERegType to, x86SSERegType from) { SSERtoR66( 0x670F ); }
__forceinline void SSE2_PACKUSWB_M128_to_XMM(x86SSERegType to, uptr from) { SSEMtoR66( 0x670F ); }

//**********************************************************************************/
//PUNPCKHWD: Unpack 16bit high
//**********************************************************************************
__forceinline void SSE2_PUNPCKLBW_XMM_to_XMM(x86SSERegType to, x86SSERegType from) { SSERtoR66( 0x600F ); }
__forceinline void SSE2_PUNPCKLBW_M128_to_XMM(x86SSERegType to, uptr from) { SSEMtoR66( 0x600F ); }

__forceinline void SSE2_PUNPCKHBW_XMM_to_XMM(x86SSERegType to, x86SSERegType from) { SSERtoR66( 0x680F ); }
__forceinline void SSE2_PUNPCKHBW_M128_to_XMM(x86SSERegType to, uptr from) { SSEMtoR66( 0x680F ); }

__forceinline void SSE2_PUNPCKLWD_XMM_to_XMM(x86SSERegType to, x86SSERegType from) { SSERtoR66( 0x610F ); }
__forceinline void SSE2_PUNPCKLWD_M128_to_XMM(x86SSERegType to, uptr from) { SSEMtoR66( 0x610F ); }
__forceinline void SSE2_PUNPCKHWD_XMM_to_XMM(x86SSERegType to, x86SSERegType from) { SSERtoR66( 0x690F ); }
__forceinline void SSE2_PUNPCKHWD_M128_to_XMM(x86SSERegType to, uptr from) { SSEMtoR66( 0x690F ); }

__forceinline void SSE2_PUNPCKLDQ_XMM_to_XMM(x86SSERegType to, x86SSERegType from) { SSERtoR66( 0x620F ); }
__forceinline void SSE2_PUNPCKLDQ_M128_to_XMM(x86SSERegType to, uptr from) { SSEMtoR66( 0x620F ); }
__forceinline void SSE2_PUNPCKHDQ_XMM_to_XMM(x86SSERegType to, x86SSERegType from) { SSERtoR66( 0x6A0F ); }
__forceinline void SSE2_PUNPCKHDQ_M128_to_XMM(x86SSERegType to, uptr from) { SSEMtoR66( 0x6A0F ); }

__forceinline void SSE2_PUNPCKLQDQ_XMM_to_XMM(x86SSERegType to, x86SSERegType from) { SSERtoR66( 0x6C0F ); }
__forceinline void SSE2_PUNPCKLQDQ_M128_to_XMM(x86SSERegType to, uptr from) { SSEMtoR66( 0x6C0F ); }

__forceinline void SSE2_PUNPCKHQDQ_XMM_to_XMM(x86SSERegType to, x86SSERegType from) { SSERtoR66( 0x6D0F ); }
__forceinline void SSE2_PUNPCKHQDQ_M128_to_XMM(x86SSERegType to, uptr from) { SSEMtoR66( 0x6D0F ); }

__forceinline void SSE2_PMULLW_XMM_to_XMM(x86SSERegType to, x86SSERegType from) { SSERtoR66( 0xD50F ); }
__forceinline void SSE2_PMULLW_M128_to_XMM(x86SSERegType to, uptr from) { SSEMtoR66( 0xD50F ); }
__forceinline void SSE2_PMULHW_XMM_to_XMM(x86SSERegType to, x86SSERegType from) { SSERtoR66( 0xE50F ); }
__forceinline void SSE2_PMULHW_M128_to_XMM(x86SSERegType to, uptr from) { SSEMtoR66( 0xE50F ); }

__forceinline void SSE2_PMULUDQ_XMM_to_XMM(x86SSERegType to, x86SSERegType from) { SSERtoR66( 0xF40F ); }
__forceinline void SSE2_PMULUDQ_M128_to_XMM(x86SSERegType to, uptr from) { SSEMtoR66( 0xF40F ); }

__forceinline void SSE2_PMOVMSKB_XMM_to_R32(x86IntRegType to, x86SSERegType from) { SSERtoR66(0xD70F); }

__forceinline void SSE_MOVMSKPS_XMM_to_R32(x86IntRegType to, x86SSERegType from) { SSERtoR(0x500F); }
__forceinline void SSE2_MOVMSKPD_XMM_to_R32(x86IntRegType to, x86SSERegType from) { SSERtoR66(0x500F); }

__forceinline void SSE2_PMADDWD_XMM_to_XMM(x86SSERegType to, x86SSERegType from) { SSERtoR66(0xF50F); }

__forceinline void SSE3_HADDPS_XMM_to_XMM(x86SSERegType to, x86SSERegType from) { write8(0xf2); SSERtoR( 0x7c0f ); }
__forceinline void SSE3_HADDPS_M128_to_XMM(x86SSERegType to, uptr from){ write8(0xf2); SSEMtoR( 0x7c0f, 0 ); }

__forceinline void SSE3_MOVSLDUP_XMM_to_XMM(x86SSERegType to, x86SSERegType from) {
	write8(0xf3);
    RexRB(0, to, from);
	write16( 0x120f);
	ModRM( 3, to, from );
}

__forceinline void SSE3_MOVSLDUP_M128_to_XMM(x86SSERegType to, uptr from) { write8(0xf3); SSEMtoR(0x120f, 0); }
__forceinline void SSE3_MOVSHDUP_XMM_to_XMM(x86SSERegType to, x86SSERegType from) { write8(0xf3); SSERtoR(0x160f); }
__forceinline void SSE3_MOVSHDUP_M128_to_XMM(x86SSERegType to, uptr from) { write8(0xf3); SSEMtoR(0x160f, 0); }

// SSE4.1

__forceinline void SSE4_DPPS_XMM_to_XMM(x86SSERegType to, x86SSERegType from, u8 imm8) 
{
	write8(0x66);
	write24(0x403A0F);
	ModRM(3, to, from);
	write8(imm8);
}

__forceinline void SSE4_DPPS_M128_to_XMM(x86SSERegType to, uptr from, u8 imm8)
{
	write8(0x66);
	write24(0x403A0F);
	ModRM(0, to, DISP32);
	write32(MEMADDR(from, 4));
	write8(imm8);
}

__forceinline void SSE4_INSERTPS_XMM_to_XMM(x86SSERegType to, x86SSERegType from, u8 imm8)
{
	write8(0x66);
    RexRB(0, to, from);
	write24(0x213A0F);
	ModRM(3, to, from);
	write8(imm8);
}

__forceinline void SSE4_EXTRACTPS_XMM_to_R32(x86IntRegType to, x86SSERegType from, u8 imm8)
{
	write8(0x66);
    RexRB(0, to, from);
	write24(0x173A0F);
	ModRM(3, to, from);
	write8(imm8);
}

__forceinline void SSE4_BLENDPS_XMM_to_XMM(x86IntRegType to, x86SSERegType from, u8 imm8)
{
	write8(0x66);
    RexRB(0, to, from);
	write24(0x0C3A0F);
	ModRM(3, to, from);
	write8(imm8);
}

__forceinline void SSE4_BLENDVPS_XMM_to_XMM(x86SSERegType to, x86SSERegType from)
{
	write8(0x66);
    RexRB(0, to, from);
	write24(0x14380F);
	ModRM(3, to, from);
}

__forceinline void SSE4_BLENDVPS_M128_to_XMM(x86SSERegType to, uptr from)
{
	write8(0x66);
    RexR(0, to);
	write24(0x14380F);
	ModRM(0, to, DISP32);
	write32(MEMADDR(from, 4));
}

__forceinline void SSE4_PMOVSXDQ_XMM_to_XMM(x86SSERegType to, x86SSERegType from)
{
	write8(0x66);
    RexRB(0, to, from);
	write24(0x25380F);
	ModRM(3, to, from);
}

__forceinline void SSE4_PINSRD_R32_to_XMM(x86SSERegType to, x86IntRegType from, u8 imm8)
{
	write8(0x66);
    RexRB(0, to, from);
	write24(0x223A0F);
	ModRM(3, to, from);
	write8(imm8);
}

__forceinline void SSE4_PMAXSD_XMM_to_XMM(x86SSERegType to, x86SSERegType from)
{
	write8(0x66);
	RexRB(0, to, from);
	write24(0x3D380F);
	ModRM(3, to, from);
}

__forceinline void SSE4_PMINSD_XMM_to_XMM(x86SSERegType to, x86SSERegType from)
{
	write8(0x66);
	RexRB(0, to, from);
	write24(0x39380F);
	ModRM(3, to, from);
}

__forceinline void SSE4_PMAXUD_XMM_to_XMM(x86SSERegType to, x86SSERegType from)
{
	write8(0x66);
	RexRB(0, to, from);
	write24(0x3F380F);
	ModRM(3, to, from);
}

__forceinline void SSE4_PMINUD_XMM_to_XMM(x86SSERegType to, x86SSERegType from)
{
	write8(0x66);
	RexRB(0, to, from);
	write24(0x3B380F);
	ModRM(3, to, from);
}

__forceinline void SSE4_PMAXSD_M128_to_XMM(x86SSERegType to, uptr from)
{
	write8(0x66);
	RexR(0, to);
	write24(0x3D380F);
	ModRM( 0, to, DISP32 );
	write32(MEMADDR(from, 4));
}

__forceinline void SSE4_PMINSD_M128_to_XMM(x86SSERegType to, uptr from)
{
	write8(0x66);
	RexR(0, to);
	write24(0x39380F);
	ModRM( 0, to, DISP32 );
	write32(MEMADDR(from, 4));
}

__forceinline void SSE4_PMAXUD_M128_to_XMM(x86SSERegType to, uptr from)
{
	write8(0x66);
	RexR(0, to);
	write24(0x3F380F);
	ModRM( 0, to, DISP32 );
	write32(MEMADDR(from, 4));
}

__forceinline void SSE4_PMINUD_M128_to_XMM(x86SSERegType to, uptr from)
{
	write8(0x66);
	RexR(0, to);
	write24(0x3B380F);
	ModRM( 0, to, DISP32 );
	write32(MEMADDR(from, 4));
}

// SSE-X
__forceinline void SSEX_MOVDQA_M128_to_XMM( x86SSERegType to, uptr from )
{
	if( g_xmmtypes[to] == XMMT_INT ) SSE2_MOVDQA_M128_to_XMM(to, from);
	else SSE_MOVAPS_M128_to_XMM(to, from);
}

__forceinline void SSEX_MOVDQA_XMM_to_M128( uptr to, x86SSERegType from )
{
	if( g_xmmtypes[from] == XMMT_INT ) SSE2_MOVDQA_XMM_to_M128(to, from);
	else SSE_MOVAPS_XMM_to_M128(to, from);
}

__forceinline void SSEX_MOVDQA_XMM_to_XMM( x86SSERegType to, x86SSERegType from )
{
	if( g_xmmtypes[from] == XMMT_INT ) SSE2_MOVDQA_XMM_to_XMM(to, from);
	else SSE_MOVAPS_XMM_to_XMM(to, from);
}

__forceinline void SSEX_MOVDQARmtoROffset( x86SSERegType to, x86IntRegType from, int offset )
{
	if( g_xmmtypes[to] == XMMT_INT ) SSE2_MOVDQARmtoROffset(to, from, offset);
	else SSE_MOVAPSRmtoROffset(to, from, offset);
}

__forceinline void SSEX_MOVDQARtoRmOffset( x86IntRegType to, x86SSERegType from, int offset )
{
	if( g_xmmtypes[from] == XMMT_INT ) SSE2_MOVDQARtoRmOffset(to, from, offset);
	else SSE_MOVAPSRtoRmOffset(to, from, offset);
}

__forceinline void SSEX_MOVDQU_M128_to_XMM( x86SSERegType to, uptr from )
{
	if( g_xmmtypes[to] == XMMT_INT ) SSE2_MOVDQU_M128_to_XMM(to, from);
	else SSE_MOVAPS_M128_to_XMM(to, from);
}

__forceinline void SSEX_MOVDQU_XMM_to_M128( uptr to, x86SSERegType from )
{
	if( g_xmmtypes[from] == XMMT_INT ) SSE2_MOVDQU_XMM_to_M128(to, from);
	else SSE_MOVAPS_XMM_to_M128(to, from);
}

__forceinline void SSEX_MOVDQU_XMM_to_XMM( x86SSERegType to, x86SSERegType from )
{
	if( g_xmmtypes[from] == XMMT_INT ) SSE2_MOVDQU_XMM_to_XMM(to, from);
	else SSE_MOVAPS_XMM_to_XMM(to, from);
}

__forceinline void SSEX_MOVD_M32_to_XMM( x86SSERegType to, uptr from )
{
	if( g_xmmtypes[to] == XMMT_INT ) SSE2_MOVD_M32_to_XMM(to, from);
	else SSE_MOVSS_M32_to_XMM(to, from);
}

__forceinline void SSEX_MOVD_XMM_to_M32( u32 to, x86SSERegType from )
{
	if( g_xmmtypes[from] == XMMT_INT ) SSE2_MOVD_XMM_to_M32(to, from);
	else SSE_MOVSS_XMM_to_M32(to, from);
}

__forceinline void SSEX_MOVD_XMM_to_Rm( x86IntRegType to, x86SSERegType from )
{
	if( g_xmmtypes[from] == XMMT_INT ) SSE2_MOVD_XMM_to_Rm(to, from);
	else SSE_MOVSS_XMM_to_Rm(to, from);
}

__forceinline void SSEX_MOVD_RmOffset_to_XMM( x86SSERegType to, x86IntRegType from, int offset )
{
	if( g_xmmtypes[to] == XMMT_INT ) SSE2_MOVD_RmOffset_to_XMM(to, from, offset);
	else SSE_MOVSS_RmOffset_to_XMM(to, from, offset);
}

__forceinline void SSEX_MOVD_XMM_to_RmOffset( x86IntRegType to, x86SSERegType from, int offset )
{
	if( g_xmmtypes[from] == XMMT_INT ) SSE2_MOVD_XMM_to_RmOffset(to, from, offset);
	else SSE_MOVSS_XMM_to_RmOffset(to, from, offset);
}

__forceinline void SSEX_POR_M128_to_XMM( x86SSERegType to, uptr from )
{
	if( g_xmmtypes[to] == XMMT_INT ) SSE2_POR_M128_to_XMM(to, from);
	else SSE_ORPS_M128_to_XMM(to, from);
}

__forceinline void SSEX_POR_XMM_to_XMM( x86SSERegType to, x86SSERegType from )
{
	if( g_xmmtypes[from] == XMMT_INT ) SSE2_POR_XMM_to_XMM(to, from);
	else SSE_ORPS_XMM_to_XMM(to, from);
}

__forceinline void SSEX_PXOR_M128_to_XMM( x86SSERegType to, uptr from )
{
	if( g_xmmtypes[to] == XMMT_INT ) SSE2_PXOR_M128_to_XMM(to, from);
	else SSE_XORPS_M128_to_XMM(to, from);
}

__forceinline void SSEX_PXOR_XMM_to_XMM( x86SSERegType to, x86SSERegType from )
{
	if( g_xmmtypes[from] == XMMT_INT ) SSE2_PXOR_XMM_to_XMM(to, from);
	else SSE_XORPS_XMM_to_XMM(to, from);
}

__forceinline void SSEX_PAND_M128_to_XMM( x86SSERegType to, uptr from )
{
	if( g_xmmtypes[to] == XMMT_INT ) SSE2_PAND_M128_to_XMM(to, from);
	else SSE_ANDPS_M128_to_XMM(to, from);
}

__forceinline void SSEX_PAND_XMM_to_XMM( x86SSERegType to, x86SSERegType from )
{
	if( g_xmmtypes[from] == XMMT_INT ) SSE2_PAND_XMM_to_XMM(to, from);
	else SSE_ANDPS_XMM_to_XMM(to, from);
}

__forceinline void SSEX_PANDN_M128_to_XMM( x86SSERegType to, uptr from )
{
	if( g_xmmtypes[to] == XMMT_INT ) SSE2_PANDN_M128_to_XMM(to, from);
	else SSE_ANDNPS_M128_to_XMM(to, from);
}

__forceinline void SSEX_PANDN_XMM_to_XMM( x86SSERegType to, x86SSERegType from )
{
	if( g_xmmtypes[from] == XMMT_INT ) SSE2_PANDN_XMM_to_XMM(to, from);
	else SSE_ANDNPS_XMM_to_XMM(to, from);
}

__forceinline void SSEX_PUNPCKLDQ_M128_to_XMM(x86SSERegType to, uptr from)
{
	if( g_xmmtypes[to] == XMMT_INT ) SSE2_PUNPCKLDQ_M128_to_XMM(to, from);
	else SSE_UNPCKLPS_M128_to_XMM(to, from);
}

__forceinline void SSEX_PUNPCKLDQ_XMM_to_XMM(x86SSERegType to, x86SSERegType from)
{
	if( g_xmmtypes[from] == XMMT_INT ) SSE2_PUNPCKLDQ_XMM_to_XMM(to, from);
	else SSE_UNPCKLPS_XMM_to_XMM(to, from);
}

__forceinline void SSEX_PUNPCKHDQ_M128_to_XMM(x86SSERegType to, uptr from)
{
	if( g_xmmtypes[to] == XMMT_INT ) SSE2_PUNPCKHDQ_M128_to_XMM(to, from);
	else SSE_UNPCKHPS_M128_to_XMM(to, from);
}

__forceinline void SSEX_PUNPCKHDQ_XMM_to_XMM(x86SSERegType to, x86SSERegType from)
{
	if( g_xmmtypes[from] == XMMT_INT ) SSE2_PUNPCKHDQ_XMM_to_XMM(to, from);
	else SSE_UNPCKHPS_XMM_to_XMM(to, from);
}

__forceinline void SSEX_MOVHLPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from )
{
	if( g_xmmtypes[from] == XMMT_INT ) {
		SSE2_PUNPCKHQDQ_XMM_to_XMM(to, from);
		if( to != from ) SSE2_PSHUFD_XMM_to_XMM(to, to, 0x4e);
	}
	else {
		SSE_MOVHLPS_XMM_to_XMM(to, from);
	}
}
