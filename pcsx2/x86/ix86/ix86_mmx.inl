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
// MMX instructions
//
// note: r64 = mm
//------------------------------------------------------------------

/* movq m64 to r64 */
emitterT void eMOVQMtoR( x86MMXRegType to, uptr from )
{
	write16<I>( 0x6F0F );
	ModRM<I>( 0, to, DISP32 );
	write32<I>( MEMADDR(from, 4) ); 
}

/* movq r64 to m64 */
emitterT void eMOVQRtoM( uptr to, x86MMXRegType from ) 
{
	write16<I>( 0x7F0F );
	ModRM<I>( 0, from, DISP32 );
	write32<I>(MEMADDR(to, 4)); 
}

/* pand r64 to r64 */
emitterT void ePANDRtoR( x86MMXRegType to, x86MMXRegType from ) 
{
	write16<I>( 0xDB0F );
	ModRM<I>( 3, to, from ); 
}

emitterT void ePANDNRtoR( x86MMXRegType to, x86MMXRegType from )
{
	write16<I>( 0xDF0F );
	ModRM<I>( 3, to, from ); 
}

/* por r64 to r64 */
emitterT void ePORRtoR( x86MMXRegType to, x86MMXRegType from ) 
{
	write16<I>( 0xEB0F );
	ModRM<I>( 3, to, from ); 
}

/* pxor r64 to r64 */
emitterT void ePXORRtoR( x86MMXRegType to, x86MMXRegType from ) 
{
	write16<I>( 0xEF0F );
	ModRM<I>( 3, to, from ); 
}

/* psllq r64 to r64 */
emitterT void ePSLLQRtoR( x86MMXRegType to, x86MMXRegType from ) 
{
	write16<I>( 0xF30F );
	ModRM<I>( 3, to, from ); 
}

/* psllq m64 to r64 */
emitterT void ePSLLQMtoR( x86MMXRegType to, uptr from ) 
{
	write16<I>( 0xF30F ); 
	ModRM<I>( 0, to, DISP32 ); 
	write32<I>( MEMADDR(from, 4) );
}

/* psllq imm8 to r64 */
emitterT void ePSLLQItoR( x86MMXRegType to, u8 from ) 
{
	write16<I>( 0x730F ); 
	ModRM<I>( 3, 6, to); 
	write8<I>( from ); 
}

/* psrlq r64 to r64 */
emitterT void ePSRLQRtoR( x86MMXRegType to, x86MMXRegType from ) 
{
	write16<I>( 0xD30F ); 
	ModRM<I>( 3, to, from ); 
}

/* psrlq m64 to r64 */
emitterT void ePSRLQMtoR( x86MMXRegType to, uptr from ) 
{
	write16<I>( 0xD30F ); 
	ModRM<I>( 0, to, DISP32 ); 
	write32<I>( MEMADDR(from, 4) ); 
}

/* psrlq imm8 to r64 */
emitterT void ePSRLQItoR( x86MMXRegType to, u8 from ) 
{
	write16<I>( 0x730F );
	ModRM<I>( 3, 2, to); 
	write8<I>( from ); 
}

/* paddusb r64 to r64 */
emitterT void ePADDUSBRtoR( x86MMXRegType to, x86MMXRegType from ) 
{
	write16<I>( 0xDC0F ); 
	ModRM<I>( 3, to, from ); 
}

/* paddusb m64 to r64 */
emitterT void ePADDUSBMtoR( x86MMXRegType to, uptr from ) 
{
	write16<I>( 0xDC0F ); 
	ModRM<I>( 0, to, DISP32 ); 
	write32<I>( MEMADDR(from, 4) ); 
}

/* paddusw r64 to r64 */
emitterT void ePADDUSWRtoR( x86MMXRegType to, x86MMXRegType from ) 
{
	write16<I>( 0xDD0F ); 
	ModRM<I>( 3, to, from ); 
}

/* paddusw m64 to r64 */
emitterT void ePADDUSWMtoR( x86MMXRegType to, uptr from ) 
{
	write16<I>( 0xDD0F ); 
	ModRM<I>( 0, to, DISP32 ); 
	write32<I>( MEMADDR(from, 4) ); 
}

/* paddb r64 to r64 */
emitterT void ePADDBRtoR( x86MMXRegType to, x86MMXRegType from ) 
{
	write16<I>( 0xFC0F ); 
	ModRM<I>( 3, to, from ); 
}

/* paddb m64 to r64 */
emitterT void ePADDBMtoR( x86MMXRegType to, uptr from ) 
{
	write16<I>( 0xFC0F ); 
	ModRM<I>( 0, to, DISP32 ); 
	write32<I>( MEMADDR(from, 4) ); 
}

/* paddw r64 to r64 */
emitterT void ePADDWRtoR( x86MMXRegType to, x86MMXRegType from ) 
{
	write16<I>( 0xFD0F ); 
	ModRM<I>( 3, to, from ); 
}

/* paddw m64 to r64 */
emitterT void ePADDWMtoR( x86MMXRegType to, uptr from ) 
{
	write16<I>( 0xFD0F ); 
	ModRM<I>( 0, to, DISP32 ); 
	write32<I>( MEMADDR(from, 4) ); 
}

/* paddd r64 to r64 */
emitterT void ePADDDRtoR( x86MMXRegType to, x86MMXRegType from ) 
{
	write16<I>( 0xFE0F ); 
	ModRM<I>( 3, to, from ); 
}

/* paddd m64 to r64 */
emitterT void ePADDDMtoR( x86MMXRegType to, uptr from ) 
{
	write16<I>( 0xFE0F ); 
	ModRM<I>( 0, to, DISP32 ); 
	write32<I>( MEMADDR(from, 4) ); 
}

/* emms */
emitterT void eEMMS() 
{
	write16<I>( 0x770F );
}

emitterT void ePADDSBRtoR( x86MMXRegType to, x86MMXRegType from ) 
{
	write16<I>( 0xEC0F ); 
	ModRM<I>( 3, to, from ); 
}

emitterT void ePADDSWRtoR( x86MMXRegType to, x86MMXRegType from ) 
{
	write16<I>( 0xED0F );
	ModRM<I>( 3, to, from ); 
}

// paddq m64 to r64 (sse2 only?)
emitterT void ePADDQMtoR( x86MMXRegType to, uptr from )
{
	write16<I>( 0xD40F ); 
	ModRM<I>( 0, to, DISP32 ); 
	write32<I>( MEMADDR(from, 4) ); 
}

// paddq r64 to r64 (sse2 only?)
emitterT void ePADDQRtoR( x86MMXRegType to, x86MMXRegType from )
{
	write16<I>( 0xD40F ); 
	ModRM<I>( 3, to, from ); 
}

emitterT void ePSUBSBRtoR( x86MMXRegType to, x86MMXRegType from ) 
{
	write16<I>( 0xE80F ); 
	ModRM<I>( 3, to, from ); 
}

emitterT void ePSUBSWRtoR( x86MMXRegType to, x86MMXRegType from ) 
{
	write16<I>( 0xE90F );
	ModRM<I>( 3, to, from ); 
}


emitterT void ePSUBBRtoR( x86MMXRegType to, x86MMXRegType from ) 
{
	write16<I>( 0xF80F ); 
	ModRM<I>( 3, to, from ); 
}

emitterT void ePSUBWRtoR( x86MMXRegType to, x86MMXRegType from ) 
{
	write16<I>( 0xF90F ); 
	ModRM<I>( 3, to, from ); 
}

emitterT void ePSUBDRtoR( x86MMXRegType to, x86MMXRegType from ) 
{
	write16<I>( 0xFA0F ); 
	ModRM<I>( 3, to, from ); 
}

emitterT void ePSUBDMtoR( x86MMXRegType to, uptr from )
{
	write16<I>( 0xFA0F ); 
	ModRM<I>( 0, to, DISP32 ); 
	write32<I>( MEMADDR(from, 4) ); 
}

emitterT void ePSUBUSBRtoR( x86MMXRegType to, x86MMXRegType from ) 
{
	write16<I>( 0xD80F ); 
	ModRM<I>( 3, to, from ); 
}

emitterT void ePSUBUSWRtoR( x86MMXRegType to, x86MMXRegType from ) 
{
	write16<I>( 0xD90F ); 
	ModRM<I>( 3, to, from ); 
}

// psubq m64 to r64 (sse2 only?)
emitterT void ePSUBQMtoR( x86MMXRegType to, uptr from )
{
	write16<I>( 0xFB0F ); 
	ModRM<I>( 0, to, DISP32 ); 
	write32<I>( MEMADDR(from, 4) ); 
}

// psubq r64 to r64 (sse2 only?)
emitterT void ePSUBQRtoR( x86MMXRegType to, x86MMXRegType from )
{
	write16<I>( 0xFB0F ); 
	ModRM<I>( 3, to, from ); 
}

// pmuludq m64 to r64 (sse2 only?)
emitterT void ePMULUDQMtoR( x86MMXRegType to, uptr from )
{
	write16<I>( 0xF40F ); 
	ModRM<I>( 0, to, DISP32 ); 
	write32<I>( MEMADDR(from, 4) ); 
}

// pmuludq r64 to r64 (sse2 only?)
emitterT void ePMULUDQRtoR( x86MMXRegType to, x86MMXRegType from )
{
	write16<I>( 0xF40F ); 
	ModRM<I>( 3, to, from ); 
}

emitterT void ePCMPEQBRtoR( x86MMXRegType to, x86MMXRegType from )
{
	write16<I>( 0x740F ); 
	ModRM<I>( 3, to, from ); 
}

emitterT void ePCMPEQWRtoR( x86MMXRegType to, x86MMXRegType from )
{
	write16<I>( 0x750F ); 
	ModRM<I>( 3, to, from ); 
}

emitterT void ePCMPEQDRtoR( x86MMXRegType to, x86MMXRegType from )
{
	write16<I>( 0x760F ); 
	ModRM<I>( 3, to, from ); 
}

emitterT void ePCMPEQDMtoR( x86MMXRegType to, uptr from )
{
	write16<I>( 0x760F );
	ModRM<I>( 0, to, DISP32 ); 
	write32<I>( MEMADDR(from, 4) );
}

emitterT void ePCMPGTBRtoR( x86MMXRegType to, x86MMXRegType from )
{
	write16<I>( 0x640F ); 
	ModRM<I>( 3, to, from ); 
}

emitterT void ePCMPGTWRtoR( x86MMXRegType to, x86MMXRegType from )
{
	write16<I>( 0x650F ); 
	ModRM<I>( 3, to, from ); 
}

emitterT void ePCMPGTDRtoR( x86MMXRegType to, x86MMXRegType from )
{
	write16<I>( 0x660F ); 
	ModRM<I>( 3, to, from ); 
}

emitterT void ePCMPGTDMtoR( x86MMXRegType to, uptr from )
{
	write16<I>( 0x660F );
	ModRM<I>( 0, to, DISP32 ); 
	write32<I>( MEMADDR(from, 4) );
}

emitterT void ePSRLWItoR( x86MMXRegType to, u8 from )
{
	write16<I>( 0x710F );
	ModRM<I>( 3, 2 , to ); 
	write8<I>( from );
}

emitterT void ePSRLDItoR( x86MMXRegType to, u8 from )
{
	write16<I>( 0x720F );
	ModRM<I>( 3, 2 , to ); 
	write8<I>( from );
}

emitterT void ePSRLDRtoR( x86MMXRegType to, x86MMXRegType from )
{
	write16<I>( 0xD20F );
	ModRM<I>( 3, to, from ); 
}

emitterT void ePSLLWItoR( x86MMXRegType to, u8 from )
{
	write16<I>( 0x710F );
	ModRM<I>( 3, 6 , to ); 
	write8<I>( from );
}

emitterT void ePSLLDItoR( x86MMXRegType to, u8 from )
{
	write16<I>( 0x720F );
	ModRM<I>( 3, 6 , to ); 
	write8<I>( from );
}

emitterT void ePSLLDRtoR( x86MMXRegType to, x86MMXRegType from )
{
	write16<I>( 0xF20F );
	ModRM<I>( 3, to, from ); 
}

emitterT void ePSRAWItoR( x86MMXRegType to, u8 from )
{
	write16<I>( 0x710F );
	ModRM<I>( 3, 4 , to ); 
	write8<I>( from );
}

emitterT void ePSRADItoR( x86MMXRegType to, u8 from )
{
	write16<I>( 0x720F );
	ModRM<I>( 3, 4 , to ); 
	write8<I>( from );
}

emitterT void ePSRADRtoR( x86MMXRegType to, x86MMXRegType from )
{
	write16<I>( 0xE20F );
	ModRM<I>( 3, to, from ); 
}

/* por m64 to r64 */
emitterT void ePORMtoR( x86MMXRegType to, uptr from ) 
{
	write16<I>( 0xEB0F );
	ModRM<I>( 0, to, DISP32 ); 
	write32<I>( MEMADDR(from, 4) );
}

/* pxor m64 to r64 */
emitterT void ePXORMtoR( x86MMXRegType to, uptr from ) 
{
	write16<I>( 0xEF0F );
	ModRM<I>( 0, to, DISP32 ); 
	write32<I>( MEMADDR(from, 4) );
}

/* pand m64 to r64 */
emitterT void ePANDMtoR( x86MMXRegType to, uptr from ) 
{
	//u64 rip = (u64)x86Ptr[0] + 7;
	write16<I>( 0xDB0F );
	ModRM<I>( 0, to, DISP32 ); 
	write32<I>( MEMADDR(from, 4) );
}

emitterT void ePANDNMtoR( x86MMXRegType to, uptr from )
{
	write16<I>( 0xDF0F );
	ModRM<I>( 0, to, DISP32 ); 
	write32<I>( MEMADDR(from, 4) );
}

emitterT void ePUNPCKHDQRtoR( x86MMXRegType to, x86MMXRegType from )
{
	write16<I>( 0x6A0F );
	ModRM<I>( 3, to, from );
}

emitterT void ePUNPCKHDQMtoR( x86MMXRegType to, uptr from )
{
	write16<I>( 0x6A0F );
	ModRM<I>( 0, to, DISP32 ); 
	write32<I>( MEMADDR(from, 4) );
}

emitterT void ePUNPCKLDQRtoR( x86MMXRegType to, x86MMXRegType from )
{
	write16<I>( 0x620F );
	ModRM<I>( 3, to, from );
}

emitterT void ePUNPCKLDQMtoR( x86MMXRegType to, uptr from )
{
	write16<I>( 0x620F );
	ModRM<I>( 0, to, DISP32 ); 
	write32<I>( MEMADDR(from, 4) );
}

emitterT void eMOVQ64ItoR( x86MMXRegType reg, u64 i ) 
{
	eMOVQMtoR<I>( reg, ( uptr )(x86Ptr[0]) + 2 + 7 );
	eJMP8<I>( 8 );
	write64<I>( i );
}

emitterT void eMOVQRtoR( x86MMXRegType to, x86MMXRegType from )
{
	write16<I>( 0x6F0F );
	ModRM<I>( 3, to, from );
}

emitterT void eMOVQRmtoROffset( x86MMXRegType to, x86IntRegType from, u32 offset )
{
	write16<I>( 0x6F0F );

	if( offset < 128 ) {
		ModRM<I>( 1, to, from );
		write8<I>(offset);
	}
	else {
		ModRM<I>( 2, to, from );
		write32<I>(offset);
	}
}

emitterT void eMOVQRtoRmOffset( x86IntRegType to, x86MMXRegType from, u32 offset )
{
	write16<I>( 0x7F0F );

	if( offset < 128 ) {
		ModRM<I>( 1, from , to );
		write8<I>(offset);
	}
	else {
		ModRM<I>( 2, from, to );
		write32<I>(offset);
	}
}

/* movd m32 to r64 */
emitterT void eMOVDMtoMMX( x86MMXRegType to, uptr from ) 
{
	write16<I>( 0x6E0F );
	ModRM<I>( 0, to, DISP32 );
	write32<I>( MEMADDR(from, 4) ); 
}

/* movd r64 to m32 */
emitterT void eMOVDMMXtoM( uptr to, x86MMXRegType from ) 
{
	write16<I>( 0x7E0F );
	ModRM<I>( 0, from, DISP32 );
	write32<I>( MEMADDR(to, 4) ); 
}

emitterT void eMOVD32RtoMMX( x86MMXRegType to, x86IntRegType from )
{
	write16<I>( 0x6E0F );
	ModRM<I>( 3, to, from );
}

emitterT void eMOVD32RmtoMMX( x86MMXRegType to, x86IntRegType from )
{
	write16<I>( 0x6E0F );
	ModRM<I>( 0, to, from );
}

emitterT void eMOVD32RmOffsettoMMX( x86MMXRegType to, x86IntRegType from, u32 offset )
{
	write16<I>( 0x6E0F );

	if( offset < 128 ) {
		ModRM<I>( 1, to, from );
		write8<I>(offset);
	}
	else {
		ModRM<I>( 2, to, from );
		write32<I>(offset);
	}
}

emitterT void eMOVD32MMXtoR( x86IntRegType to, x86MMXRegType from )
{
	write16<I>( 0x7E0F );
	ModRM<I>( 3, from, to );
}

emitterT void eMOVD32MMXtoRm( x86IntRegType to, x86MMXRegType from )
{
	write16<I>( 0x7E0F );
	ModRM<I>( 0, from, to );
	if( to >= 4 ) {
		// no idea why
		assert( to == ESP );
		write8<I>(0x24);
	}

}

emitterT void eMOVD32MMXtoRmOffset( x86IntRegType to, x86MMXRegType from, u32 offset )
{
	write16<I>( 0x7E0F );

	if( offset < 128 ) {
		ModRM<I>( 1, from, to );
		write8<I>(offset);
	}
	else {
		ModRM<I>( 2, from, to );
		write32<I>(offset);
	}
}

///* movd r32 to r64 */
//emitterT void eMOVD32MMXtoMMX( x86MMXRegType to, x86MMXRegType from ) 
//{
//	write16<I>( 0x6E0F );
//	ModRM<I>( 3, to, from );
//}
//
///* movq r64 to r32 */
//emitterT void eMOVD64MMXtoMMX( x86MMXRegType to, x86MMXRegType from ) 
//{
//	write16<I>( 0x7E0F );
//	ModRM<I>( 3, from, to );
//}

// untested
emitterT void ePACKSSWBMMXtoMMX(x86MMXRegType to, x86MMXRegType from)
{
	write16<I>( 0x630F );
	ModRM<I>( 3, to, from ); 
}

emitterT void ePACKSSDWMMXtoMMX(x86MMXRegType to, x86MMXRegType from)
{
	write16<I>( 0x6B0F );
	ModRM<I>( 3, to, from ); 
}

emitterT void ePMOVMSKBMMXtoR(x86IntRegType to, x86MMXRegType from)
{
	write16<I>( 0xD70F ); 
	ModRM<I>( 3, to, from );
}

emitterT void ePINSRWRtoMMX( x86MMXRegType to, x86SSERegType from, u8 imm8 )
{
	if (to > 7 || from > 7) Rex(1, to >> 3, 0, from >> 3);
	write16<I>( 0xc40f );
	ModRM<I>( 3, to, from );
	write8<I>( imm8 );
}

emitterT void ePSHUFWRtoR(x86MMXRegType to, x86MMXRegType from, u8 imm8)
{
	write16<I>(0x700f);
	ModRM<I>( 3, to, from );
	write8<I>(imm8);
}

emitterT void ePSHUFWMtoR(x86MMXRegType to, uptr from, u8 imm8)
{
	write16<I>( 0x700f );
	ModRM<I>( 0, to, DISP32 );
	write32<I>( MEMADDR(from, 4) );
	write8<I>(imm8);
}

emitterT void eMASKMOVQRtoR(x86MMXRegType to, x86MMXRegType from)
{
	write16<I>(0xf70f);
	ModRM<I>( 3, to, from );
}
