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

//------------------------------------------------------------------
// MMX instructions
//
// note: r64 = mm
//------------------------------------------------------------------

using namespace x86Emitter;

emitterT void MOVQMtoR( x86MMXRegType to, uptr from )						{ xMOVQ( xRegisterMMX(to), (void*)from ); }
emitterT void MOVQRtoM( uptr to, x86MMXRegType from )						{ xMOVQ( (void*)to, xRegisterMMX(from) ); }
emitterT void MOVQRtoR( x86MMXRegType to, x86MMXRegType from )				{ xMOVQ( xRegisterMMX(to), xRegisterMMX(from) ); }
emitterT void MOVQRmtoR( x86MMXRegType to, x86IntRegType from, int offset ) { xMOVQ( xRegisterMMX(to), ptr[xAddressReg(from)+offset] ); }
emitterT void MOVQRtoRm( x86IntRegType to, x86MMXRegType from, int offset ) { xMOVQ( ptr[xAddressReg(to)+offset], xRegisterMMX(from) ); }

emitterT void MOVDMtoMMX( x86MMXRegType to, uptr from )							{ xMOVDZX( xRegisterMMX(to), (void*)from ); }
emitterT void MOVDMMXtoM( uptr to, x86MMXRegType from )							{ xMOVD( (void*)to, xRegisterMMX(from) ); }
emitterT void MOVD32RtoMMX( x86MMXRegType to, x86IntRegType from )				{ xMOVDZX( xRegisterMMX(to), xRegister32(from) ); }
emitterT void MOVD32RmtoMMX( x86MMXRegType to, x86IntRegType from, int offset )	{ xMOVDZX( xRegisterMMX(to), ptr[xAddressReg(from)+offset] ); }
emitterT void MOVD32MMXtoR( x86IntRegType to, x86MMXRegType from )				{ xMOVD( xRegister32(to), xRegisterMMX(from) ); }
emitterT void MOVD32MMXtoRm( x86IntRegType to, x86MMXRegType from, int offset )	{ xMOVD( ptr[xAddressReg(to)+offset], xRegisterMMX(from) ); }

emitterT void PMOVMSKBMMXtoR(x86IntRegType to, x86MMXRegType from)			{ xPMOVMSKB( xRegister32(to), xRegisterMMX(from) ); }

#define DEFINE_LEGACY_LOGIC_OPCODE( mod ) \
	emitterT void P##mod##RtoR( x86MMXRegType to, x86MMXRegType from )				{ xP##mod( xRegisterMMX(to), xRegisterMMX(from) ); } \
	emitterT void P##mod##MtoR( x86MMXRegType to, uptr from )						{ xP##mod( xRegisterMMX(to), (void*)from ); } \
	emitterT void SSE2_P##mod##_XMM_to_XMM( x86SSERegType to, x86SSERegType from )	{ xP##mod( xRegisterSSE(to), xRegisterSSE(from) ); } \
	emitterT void SSE2_P##mod##_M128_to_XMM( x86SSERegType to, uptr from )			{ xP##mod( xRegisterSSE(to), (void*)from ); }

#define DEFINE_LEGACY_ARITHMETIC( mod, sub ) \
	emitterT void P##mod##sub##RtoR( x86MMXRegType to, x86MMXRegType from )			{ xP##mod.sub( xRegisterMMX(to), xRegisterMMX(from) ); } \
	emitterT void P##mod##sub##MtoR( x86MMXRegType to, uptr from )					{ xP##mod.sub( xRegisterMMX(to), (void*)from ); } \
	emitterT void SSE2_P##mod##sub##_XMM_to_XMM( x86SSERegType to, x86SSERegType from )	{ xP##mod.sub( xRegisterSSE(to), xRegisterSSE(from) ); } \
	emitterT void SSE2_P##mod##sub##_M128_to_XMM( x86SSERegType to, uptr from )			{ xP##mod.sub( xRegisterSSE(to), (void*)from ); }

#define DEFINE_LEGACY_SHIFT_STUFF( mod, sub ) \
	emitterT void P##mod##sub##RtoR( x86MMXRegType to, x86MMXRegType from )			{ xP##mod.sub( xRegisterMMX(to), xRegisterMMX(from) ); } \
	emitterT void P##mod##sub##MtoR( x86MMXRegType to, uptr from )					{ xP##mod.sub( xRegisterMMX(to), (void*)from ); } \
	emitterT void P##mod##sub##ItoR( x86MMXRegType to, u8 imm )						{ xP##mod.sub( xRegisterMMX(to), imm ); } \
	emitterT void SSE2_P##mod##sub##_XMM_to_XMM( x86SSERegType to, x86SSERegType from )	{ xP##mod.sub( xRegisterSSE(to), xRegisterSSE(from) ); } \
	emitterT void SSE2_P##mod##sub##_M128_to_XMM( x86SSERegType to, uptr from )			{ xP##mod.sub( xRegisterSSE(to), (void*)from ); } \
	emitterT void SSE2_P##mod##sub##_I8_to_XMM( x86SSERegType to, u8 imm )				{ xP##mod.sub( xRegisterSSE(to), imm ); }

#define DEFINE_LEGACY_SHIFT_OPCODE( mod ) \
	DEFINE_LEGACY_SHIFT_STUFF( mod, Q ) \
	DEFINE_LEGACY_SHIFT_STUFF( mod, D ) \
	DEFINE_LEGACY_SHIFT_STUFF( mod, W ) \
	emitterT void SSE2_P##mod##DQ_I8_to_XMM( x86MMXRegType to, u8 imm )						{ xP##mod.DQ( xRegisterSSE(to), imm ); }

DEFINE_LEGACY_LOGIC_OPCODE( AND )
DEFINE_LEGACY_LOGIC_OPCODE( ANDN )
DEFINE_LEGACY_LOGIC_OPCODE( OR )
DEFINE_LEGACY_LOGIC_OPCODE( XOR )

DEFINE_LEGACY_SHIFT_OPCODE( SLL )
DEFINE_LEGACY_SHIFT_OPCODE( SRL )

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

/* emms */
emitterT void EMMS() 
{
	write16( 0x770F );
}

// pmuludq m64 to r64 (sse2 only?)
emitterT void PMULUDQMtoR( x86MMXRegType to, uptr from )
{
	write16( 0xF40F ); 
	ModRM( 0, to, DISP32 ); 
	write32( MEMADDR(from, 4) ); 
}

// pmuludq r64 to r64 (sse2 only?)
emitterT void PMULUDQRtoR( x86MMXRegType to, x86MMXRegType from )
{
	write16( 0xF40F ); 
	ModRM( 3, to, from ); 
}

emitterT void PCMPEQBRtoR( x86MMXRegType to, x86MMXRegType from )
{
	write16( 0x740F ); 
	ModRM( 3, to, from ); 
}

emitterT void PCMPEQWRtoR( x86MMXRegType to, x86MMXRegType from )
{
	write16( 0x750F ); 
	ModRM( 3, to, from ); 
}

emitterT void PCMPEQDRtoR( x86MMXRegType to, x86MMXRegType from )
{
	write16( 0x760F ); 
	ModRM( 3, to, from ); 
}

emitterT void PCMPEQDMtoR( x86MMXRegType to, uptr from )
{
	write16( 0x760F );
	ModRM( 0, to, DISP32 ); 
	write32( MEMADDR(from, 4) );
}

emitterT void PCMPGTBRtoR( x86MMXRegType to, x86MMXRegType from )
{
	write16( 0x640F ); 
	ModRM( 3, to, from ); 
}

emitterT void PCMPGTWRtoR( x86MMXRegType to, x86MMXRegType from )
{
	write16( 0x650F ); 
	ModRM( 3, to, from ); 
}

emitterT void PCMPGTDRtoR( x86MMXRegType to, x86MMXRegType from )
{
	write16( 0x660F ); 
	ModRM( 3, to, from ); 
}

emitterT void PCMPGTDMtoR( x86MMXRegType to, uptr from )
{
	write16( 0x660F );
	ModRM( 0, to, DISP32 ); 
	write32( MEMADDR(from, 4) );
}

emitterT void PSRAWItoR( x86MMXRegType to, u8 from )
{
	write16( 0x710F );
	ModRM( 3, 4 , to ); 
	write8( from );
}

emitterT void PSRADItoR( x86MMXRegType to, u8 from )
{
	write16( 0x720F );
	ModRM( 3, 4 , to ); 
	write8( from );
}

emitterT void PSRADRtoR( x86MMXRegType to, x86MMXRegType from )
{
	write16( 0xE20F );
	ModRM( 3, to, from ); 
}

emitterT void PUNPCKHDQRtoR( x86MMXRegType to, x86MMXRegType from )
{
	write16( 0x6A0F );
	ModRM( 3, to, from );
}

emitterT void PUNPCKHDQMtoR( x86MMXRegType to, uptr from )
{
	write16( 0x6A0F );
	ModRM( 0, to, DISP32 ); 
	write32( MEMADDR(from, 4) );
}

emitterT void PUNPCKLDQRtoR( x86MMXRegType to, x86MMXRegType from )
{
	write16( 0x620F );
	ModRM( 3, to, from );
}

emitterT void PUNPCKLDQMtoR( x86MMXRegType to, uptr from )
{
	write16( 0x620F );
	ModRM( 0, to, DISP32 ); 
	write32( MEMADDR(from, 4) );
}

// untested
emitterT void PACKSSWBMMXtoMMX(x86MMXRegType to, x86MMXRegType from)
{
	write16( 0x630F );
	ModRM( 3, to, from ); 
}

emitterT void PACKSSDWMMXtoMMX(x86MMXRegType to, x86MMXRegType from)
{
	write16( 0x6B0F );
	ModRM( 3, to, from ); 
}

emitterT void PINSRWRtoMMX( x86MMXRegType to, x86SSERegType from, u8 imm8 )
{
	if (to > 7 || from > 7) Rex(1, to >> 3, 0, from >> 3);
	write16( 0xc40f );
	ModRM( 3, to, from );
	write8( imm8 );
}

emitterT void PSHUFWRtoR(x86MMXRegType to, x86MMXRegType from, u8 imm8)
{
	write16(0x700f);
	ModRM( 3, to, from );
	write8(imm8);
}

emitterT void PSHUFWMtoR(x86MMXRegType to, uptr from, u8 imm8)
{
	write16( 0x700f );
	ModRM( 0, to, DISP32 );
	write32( MEMADDR(from, 4) );
	write8(imm8);
}

emitterT void MASKMOVQRtoR(x86MMXRegType to, x86MMXRegType from)	{ xMASKMOV( xRegisterMMX(to), xRegisterMMX(from) ); }
