/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2009  PCSX2 Dev Team
 * 
 *  PCSX2 is free software: you can redistribute it and/or modify it under the terms
 *  of the GNU Lesser General Public License as published by the Free Software Found-
 *  ation, either version 3 of the License, or (at your option) any later version.
 *
 *  PCSX2 is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 *  without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 *  PURPOSE.  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along with PCSX2.
 *  If not, see <http://www.gnu.org/licenses/>.
 */


#include "PrecompiledHeader.h"

u32 g_sseMXCSR = DEFAULT_sseMXCSR; 
u32 g_sseVUMXCSR = DEFAULT_sseVUMXCSR;

//////////////////////////////////////////////////////////////////////////////////////////
// SetCPUState -- for assignment of SSE roundmodes and clampmodes.
//
void SetCPUState(u32 sseMXCSR, u32 sseVUMXCSR)
{
	//Msgbox::Alert("SetCPUState: Config.sseMXCSR = %x; Config.sseVUMXCSR = %x \n", Config.sseMXCSR, Config.sseVUMXCSR);
	// SSE STATE //
	// WARNING: do not touch unless you know what you are doing

	sseMXCSR &= 0xffff; // clear the upper 16 bits since they shouldn't be set
	sseVUMXCSR &= 0xffff;

	if( !x86caps.hasStreamingSIMD2Extensions )
	{
		// SSE1 cpus do not support Denormals Are Zero flag (throws an exception
		// if we don't mask them off)

		sseMXCSR &= ~0x0040;
		sseVUMXCSR &= ~0x0040;
	}

	g_sseMXCSR = sseMXCSR;
	g_sseVUMXCSR = sseVUMXCSR;

#ifdef _MSC_VER
	__asm ldmxcsr g_sseMXCSR; // set the new sse control
#else
	__asm__("ldmxcsr %[g_sseMXCSR]" : : [g_sseMXCSR]"m"(g_sseMXCSR) );
#endif
	//g_sseVUMXCSR = g_sseMXCSR|0x6000;
}
