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
// 3DNOW instructions
//------------------------------------------------------------------

/* femms */
emitterT void eFEMMS( void ) 
{
	write16<I>( 0x0E0F );
}

emitterT void ePFCMPEQMtoR( x86IntRegType to, uptr from )
{
	write16<I>( 0x0F0F );
	ModRM<I>( 0, to, DISP32 ); 
	write32<I>( from ); 
	write8<I>( 0xB0 );
}

emitterT void ePFCMPGTMtoR( x86IntRegType to, uptr from )
{
	write16<I>( 0x0F0F );
	ModRM<I>( 0, to, DISP32 ); 
	write32<I>( from ); 
	write8<I>( 0xA0 );
}

emitterT void ePFCMPGEMtoR( x86IntRegType to, uptr from )
{
	write16<I>( 0x0F0F );
	ModRM<I>( 0, to, DISP32 ); 
	write32<I>( from ); 
	write8<I>( 0x90 );
}

emitterT void ePFADDMtoR( x86IntRegType to, uptr from )
{
	write16<I>( 0x0F0F );
	ModRM<I>( 0, to, DISP32 ); 
	write32<I>( from ); 
	write8<I>( 0x9E );
}

emitterT void ePFADDRtoR( x86IntRegType to, x86IntRegType from )
{
	write16<I>( 0x0F0F );
	ModRM<I>( 3, to, from );
	write8<I>( 0x9E );
}

emitterT void ePFSUBMtoR( x86IntRegType to, uptr from )
{
	write16<I>( 0x0F0F );
	ModRM<I>( 0, to, DISP32 ); 
	write32<I>( from ); 
	write8<I>( 0x9A );
}

emitterT void ePFSUBRtoR( x86IntRegType to, x86IntRegType from )
{
	write16<I>( 0x0F0F );
	ModRM<I>( 3, to, from ); 
	write8<I>( 0x9A );
}

emitterT void ePFMULMtoR( x86IntRegType to, uptr from )
{
	write16<I>( 0x0F0F );
	ModRM<I>( 0, to, DISP32 ); 
	write32<I>( from ); 
	write8<I>( 0xB4 );
}

emitterT void ePFMULRtoR( x86IntRegType to, x86IntRegType from )
{
	write16<I>( 0x0F0F );
	ModRM<I>( 3, to, from ); 
	write8<I>( 0xB4 );
}

emitterT void ePFRCPMtoR( x86IntRegType to, uptr from )
{
	write16<I>( 0x0F0F );
	ModRM<I>( 0, to, DISP32 ); 
	write32<I>( from ); 
	write8<I>( 0x96 );
}

emitterT void ePFRCPRtoR( x86IntRegType to, x86IntRegType from )
{
	write16<I>( 0x0F0F );
	ModRM<I>( 3, to, from ); 
	write8<I>( 0x96 );
}

emitterT void ePFRCPIT1RtoR( x86IntRegType to, x86IntRegType from )
{
	write16<I>( 0x0F0F );
	ModRM<I>( 3, to, from ); 
	write8<I>( 0xA6 );
}

emitterT void ePFRCPIT2RtoR( x86IntRegType to, x86IntRegType from )
{
	write16<I>( 0x0F0F );
	ModRM<I>( 3, to, from ); 
	write8<I>( 0xB6 );
}

emitterT void ePFRSQRTRtoR( x86IntRegType to, x86IntRegType from )
{
	write16<I>( 0x0F0F );
	ModRM<I>( 3, to, from ); 
	write8<I>( 0x97 );
}

emitterT void ePFRSQIT1RtoR( x86IntRegType to, x86IntRegType from )
{
	write16<I>( 0x0F0F );
	ModRM<I>( 3, to, from ); 
	write8<I>( 0xA7 );
}

emitterT void ePF2IDMtoR( x86IntRegType to, uptr from )
{
	write16<I>( 0x0F0F );
	ModRM<I>( 0, to, DISP32 ); 
	write32<I>( from ); 
	write8<I>( 0x1D );
}

emitterT void ePF2IDRtoR( x86IntRegType to, x86IntRegType from )
{
	write16<I>( 0x0F0F );
	ModRM<I>( 3, to, from ); 
	write8<I>( 0x1D );
}

emitterT void ePI2FDMtoR( x86IntRegType to, uptr from )
{
	write16<I>( 0x0F0F );
	ModRM<I>( 0, to, DISP32 ); 
	write32<I>( from ); 
	write8<I>( 0x0D );
}

emitterT void ePI2FDRtoR( x86IntRegType to, x86IntRegType from )
{
	write16<I>( 0x0F0F );
	ModRM<I>( 3, to, from ); 
	write8<I>( 0x0D );
}

emitterT void ePFMAXMtoR( x86IntRegType to, uptr from )
{
	write16<I>( 0x0F0F );
	ModRM<I>( 0, to, DISP32 ); 
	write32<I>( from ); 
	write8<I>( 0xA4 );
}

emitterT void ePFMAXRtoR( x86IntRegType to, x86IntRegType from )
{
	write16<I>( 0x0F0F );
	ModRM<I>( 3, to, from ); 
	write8<I>( 0xA4 );
}

emitterT void ePFMINMtoR( x86IntRegType to, uptr from )
{
	write16<I>( 0x0F0F );
	ModRM<I>( 0, to, DISP32 ); 
	write32<I>( from ); 
	write8<I>( 0x94 );
}

emitterT void ePFMINRtoR( x86IntRegType to, x86IntRegType from )
{
	write16<I>( 0x0F0F );
	ModRM<I>( 3, to, from );
	write8<I>( 0x94 );
}
