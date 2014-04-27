/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2010  PCSX2 Dev Team
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

#pragma once

//////////////////////////////////////////////////////////////////////////////////////////
// SSE-X Helpers (generates either INT or FLOAT versions of certain SSE instructions)
// This header should always be included *after* ix86.h.

// Added AlwaysUseMovaps check to the relevant functions here, which helps reduce the
// overhead of dynarec instructions that use these.

extern void SSEX_MOVDQA_M128_to_XMM( x86SSERegType to, uptr from );
extern void SSEX_MOVDQA_XMM_to_M128( uptr to, x86SSERegType from );
extern void SSEX_MOVDQA_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
extern void SSEX_MOVDQARmtoR( x86SSERegType to, x86IntRegType from, int offset=0 );
extern void SSEX_MOVDQARtoRm( x86IntRegType to, x86SSERegType from, int offset=0 );
extern void SSEX_MOVDQU_M128_to_XMM( x86SSERegType to, uptr from );
extern void SSEX_MOVDQU_XMM_to_M128( uptr to, x86SSERegType from );
extern void SSEX_MOVD_M32_to_XMM( x86SSERegType to, uptr from );
extern void SSEX_MOVD_XMM_to_M32( u32 to, x86SSERegType from );
extern void SSEX_MOVD_Rm_to_XMM( x86SSERegType to, x86IntRegType from, int offset=0 );
extern void SSEX_MOVD_XMM_to_Rm( x86IntRegType to, x86SSERegType from, int offset=0 );
extern void SSEX_POR_M128_to_XMM( x86SSERegType to, uptr from );
extern void SSEX_POR_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
extern void SSEX_PXOR_M128_to_XMM( x86SSERegType to, uptr from );
extern void SSEX_PXOR_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
extern void SSEX_PAND_M128_to_XMM( x86SSERegType to, uptr from );
extern void SSEX_PAND_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
extern void SSEX_PANDN_M128_to_XMM( x86SSERegType to, uptr from );
extern void SSEX_PANDN_XMM_to_XMM( x86SSERegType to, x86SSERegType from );
extern void SSEX_PUNPCKLDQ_M128_to_XMM(x86SSERegType to, uptr from );
extern void SSEX_PUNPCKLDQ_XMM_to_XMM(x86SSERegType to, x86SSERegType from );
extern void SSEX_PUNPCKHDQ_M128_to_XMM(x86SSERegType to, uptr from );
extern void SSEX_PUNPCKHDQ_XMM_to_XMM(x86SSERegType to, x86SSERegType from );
