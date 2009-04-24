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
// 3DNOW instructions [Anyone caught dead using these will be re-killed]
//------------------------------------------------------------------

/* femms */
emitterT void FEMMS( void ) 
{
	xWrite16( 0x0E0F );
}

emitterT void PFCMPEQMtoR( x86IntRegType to, uptr from )
{
	xWrite16( 0x0F0F );
	ModRM( 0, to, DISP32 ); 
	xWrite32( from ); 
	xWrite8( 0xB0 );
}

emitterT void PFCMPGTMtoR( x86IntRegType to, uptr from )
{
	xWrite16( 0x0F0F );
	ModRM( 0, to, DISP32 ); 
	xWrite32( from ); 
	xWrite8( 0xA0 );
}

emitterT void PFCMPGEMtoR( x86IntRegType to, uptr from )
{
	xWrite16( 0x0F0F );
	ModRM( 0, to, DISP32 ); 
	xWrite32( from ); 
	xWrite8( 0x90 );
}

emitterT void PFADDMtoR( x86IntRegType to, uptr from )
{
	xWrite16( 0x0F0F );
	ModRM( 0, to, DISP32 ); 
	xWrite32( from ); 
	xWrite8( 0x9E );
}

emitterT void PFADDRtoR( x86IntRegType to, x86IntRegType from )
{
	xWrite16( 0x0F0F );
	ModRM( 3, to, from );
	xWrite8( 0x9E );
}

emitterT void PFSUBMtoR( x86IntRegType to, uptr from )
{
	xWrite16( 0x0F0F );
	ModRM( 0, to, DISP32 ); 
	xWrite32( from ); 
	xWrite8( 0x9A );
}

emitterT void PFSUBRtoR( x86IntRegType to, x86IntRegType from )
{
	xWrite16( 0x0F0F );
	ModRM( 3, to, from ); 
	xWrite8( 0x9A );
}

emitterT void PFMULMtoR( x86IntRegType to, uptr from )
{
	xWrite16( 0x0F0F );
	ModRM( 0, to, DISP32 ); 
	xWrite32( from ); 
	xWrite8( 0xB4 );
}

emitterT void PFMULRtoR( x86IntRegType to, x86IntRegType from )
{
	xWrite16( 0x0F0F );
	ModRM( 3, to, from ); 
	xWrite8( 0xB4 );
}

emitterT void PFRCPMtoR( x86IntRegType to, uptr from )
{
	xWrite16( 0x0F0F );
	ModRM( 0, to, DISP32 ); 
	xWrite32( from ); 
	xWrite8( 0x96 );
}

emitterT void PFRCPRtoR( x86IntRegType to, x86IntRegType from )
{
	xWrite16( 0x0F0F );
	ModRM( 3, to, from ); 
	xWrite8( 0x96 );
}

emitterT void PFRCPIT1RtoR( x86IntRegType to, x86IntRegType from )
{
	xWrite16( 0x0F0F );
	ModRM( 3, to, from ); 
	xWrite8( 0xA6 );
}

emitterT void PFRCPIT2RtoR( x86IntRegType to, x86IntRegType from )
{
	xWrite16( 0x0F0F );
	ModRM( 3, to, from ); 
	xWrite8( 0xB6 );
}

emitterT void PFRSQRTRtoR( x86IntRegType to, x86IntRegType from )
{
	xWrite16( 0x0F0F );
	ModRM( 3, to, from ); 
	xWrite8( 0x97 );
}

emitterT void PFRSQIT1RtoR( x86IntRegType to, x86IntRegType from )
{
	xWrite16( 0x0F0F );
	ModRM( 3, to, from ); 
	xWrite8( 0xA7 );
}

emitterT void PF2IDMtoR( x86IntRegType to, uptr from )
{
	xWrite16( 0x0F0F );
	ModRM( 0, to, DISP32 ); 
	xWrite32( from ); 
	xWrite8( 0x1D );
}

emitterT void PF2IDRtoR( x86IntRegType to, x86IntRegType from )
{
	xWrite16( 0x0F0F );
	ModRM( 3, to, from ); 
	xWrite8( 0x1D );
}

emitterT void PI2FDMtoR( x86IntRegType to, uptr from )
{
	xWrite16( 0x0F0F );
	ModRM( 0, to, DISP32 ); 
	xWrite32( from ); 
	xWrite8( 0x0D );
}

emitterT void PI2FDRtoR( x86IntRegType to, x86IntRegType from )
{
	xWrite16( 0x0F0F );
	ModRM( 3, to, from ); 
	xWrite8( 0x0D );
}

emitterT void PFMAXMtoR( x86IntRegType to, uptr from )
{
	xWrite16( 0x0F0F );
	ModRM( 0, to, DISP32 ); 
	xWrite32( from ); 
	xWrite8( 0xA4 );
}

emitterT void PFMAXRtoR( x86IntRegType to, x86IntRegType from )
{
	xWrite16( 0x0F0F );
	ModRM( 3, to, from ); 
	xWrite8( 0xA4 );
}

emitterT void PFMINMtoR( x86IntRegType to, uptr from )
{
	xWrite16( 0x0F0F );
	ModRM( 0, to, DISP32 ); 
	xWrite32( from ); 
	xWrite8( 0x94 );
}

emitterT void PFMINRtoR( x86IntRegType to, x86IntRegType from )
{
	xWrite16( 0x0F0F );
	ModRM( 3, to, from );
	xWrite8( 0x94 );
}
