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

//////////////////////////////////////////////////////////////////////////////////////////
// SSE-X Helpers (generates either INT or FLOAT versions of certain SSE instructions)
// This header should always be included *after* ix86.h.

#ifndef _ix86_included_
#error Dependency fail: Please define _EmitterId_ and include ix86.h first.
#endif

// Added AlwaysUseMovaps check to the relevant functions here, which helps reduce the
// overhead of dynarec instructions that use these.

static __forceinline void SSEX_MOVDQA_M128_to_XMM( x86SSERegType to, uptr from )
{
	if( !AlwaysUseMovaps && g_xmmtypes[to] == XMMT_INT ) SSE2_MOVDQA_M128_to_XMM(to, from);
	else SSE_MOVAPS_M128_to_XMM(to, from);
}

static __forceinline void SSEX_MOVDQA_XMM_to_M128( uptr to, x86SSERegType from )
{
	if( !AlwaysUseMovaps && g_xmmtypes[from] == XMMT_INT ) SSE2_MOVDQA_XMM_to_M128(to, from);
	else SSE_MOVAPS_XMM_to_M128(to, from);
}

static __forceinline void SSEX_MOVDQA_XMM_to_XMM( x86SSERegType to, x86SSERegType from )
{
	if( !AlwaysUseMovaps && g_xmmtypes[from] == XMMT_INT ) SSE2_MOVDQA_XMM_to_XMM(to, from);
	else SSE_MOVAPS_XMM_to_XMM(to, from);
}

static __forceinline void SSEX_MOVDQARmtoROffset( x86SSERegType to, x86IntRegType from, int offset )
{
	if( !AlwaysUseMovaps && g_xmmtypes[to] == XMMT_INT ) SSE2_MOVDQARmtoROffset(to, from, offset);
	else SSE_MOVAPSRmtoROffset(to, from, offset);
}

static __forceinline void SSEX_MOVDQARtoRmOffset( x86IntRegType to, x86SSERegType from, int offset )
{
	if( !AlwaysUseMovaps && g_xmmtypes[from] == XMMT_INT ) SSE2_MOVDQARtoRmOffset(to, from, offset);
	else SSE_MOVAPSRtoRmOffset(to, from, offset);
}

static __forceinline void SSEX_MOVDQU_M128_to_XMM( x86SSERegType to, uptr from )
{
	if( !AlwaysUseMovaps && g_xmmtypes[to] == XMMT_INT ) SSE2_MOVDQU_M128_to_XMM(to, from);
	else SSE_MOVAPS_M128_to_XMM(to, from);
}

static __forceinline void SSEX_MOVDQU_XMM_to_M128( uptr to, x86SSERegType from )
{
	if( !AlwaysUseMovaps && g_xmmtypes[from] == XMMT_INT ) SSE2_MOVDQU_XMM_to_M128(to, from);
	else SSE_MOVAPS_XMM_to_M128(to, from);
}

static __forceinline void SSEX_MOVD_M32_to_XMM( x86SSERegType to, uptr from )
{
	if( g_xmmtypes[to] == XMMT_INT ) SSE2_MOVD_M32_to_XMM(to, from);
	else SSE_MOVSS_M32_to_XMM(to, from);
}

static __forceinline void SSEX_MOVD_XMM_to_M32( u32 to, x86SSERegType from )
{
	if( g_xmmtypes[from] == XMMT_INT ) SSE2_MOVD_XMM_to_M32(to, from);
	else SSE_MOVSS_XMM_to_M32(to, from);
}

static __forceinline void SSEX_MOVD_XMM_to_Rm( x86IntRegType to, x86SSERegType from )
{
	if( g_xmmtypes[from] == XMMT_INT ) SSE2_MOVD_XMM_to_Rm(to, from);
	else SSE_MOVSS_XMM_to_Rm(to, from);
}

static __forceinline void SSEX_MOVD_RmOffset_to_XMM( x86SSERegType to, x86IntRegType from, int offset )
{
	if( g_xmmtypes[to] == XMMT_INT ) SSE2_MOVD_RmOffset_to_XMM(to, from, offset);
	else SSE_MOVSS_RmOffset_to_XMM(to, from, offset);
}

static __forceinline void SSEX_MOVD_XMM_to_RmOffset( x86IntRegType to, x86SSERegType from, int offset )
{
	if( g_xmmtypes[from] == XMMT_INT ) SSE2_MOVD_XMM_to_RmOffset(to, from, offset);
	else SSE_MOVSS_XMM_to_RmOffset(to, from, offset);
}

static __forceinline void SSEX_POR_M128_to_XMM( x86SSERegType to, uptr from )
{
	if( g_xmmtypes[to] == XMMT_INT ) SSE2_POR_M128_to_XMM(to, from);
	else SSE_ORPS_M128_to_XMM(to, from);
}

static __forceinline void SSEX_POR_XMM_to_XMM( x86SSERegType to, x86SSERegType from )
{
	if( g_xmmtypes[from] == XMMT_INT ) SSE2_POR_XMM_to_XMM(to, from);
	else SSE_ORPS_XMM_to_XMM(to, from);
}

static __forceinline void SSEX_PXOR_M128_to_XMM( x86SSERegType to, uptr from )
{
	if( g_xmmtypes[to] == XMMT_INT ) SSE2_PXOR_M128_to_XMM(to, from);
	else SSE_XORPS_M128_to_XMM(to, from);
}

static __forceinline void SSEX_PXOR_XMM_to_XMM( x86SSERegType to, x86SSERegType from )
{
	if( g_xmmtypes[from] == XMMT_INT ) SSE2_PXOR_XMM_to_XMM(to, from);
	else SSE_XORPS_XMM_to_XMM(to, from);
}

static __forceinline void SSEX_PAND_M128_to_XMM( x86SSERegType to, uptr from )
{
	if( g_xmmtypes[to] == XMMT_INT ) SSE2_PAND_M128_to_XMM(to, from);
	else SSE_ANDPS_M128_to_XMM(to, from);
}

static __forceinline void SSEX_PAND_XMM_to_XMM( x86SSERegType to, x86SSERegType from )
{
	if( g_xmmtypes[from] == XMMT_INT ) SSE2_PAND_XMM_to_XMM(to, from);
	else SSE_ANDPS_XMM_to_XMM(to, from);
}

static __forceinline void SSEX_PANDN_M128_to_XMM( x86SSERegType to, uptr from )
{
	if( g_xmmtypes[to] == XMMT_INT ) SSE2_PANDN_M128_to_XMM(to, from);
	else SSE_ANDNPS_M128_to_XMM(to, from);
}

static __forceinline void SSEX_PANDN_XMM_to_XMM( x86SSERegType to, x86SSERegType from )
{
	if( g_xmmtypes[from] == XMMT_INT ) SSE2_PANDN_XMM_to_XMM(to, from);
	else SSE_ANDNPS_XMM_to_XMM(to, from);
}

static __forceinline void SSEX_PUNPCKLDQ_M128_to_XMM(x86SSERegType to, uptr from)
{
	if( g_xmmtypes[to] == XMMT_INT ) SSE2_PUNPCKLDQ_M128_to_XMM(to, from);
	else SSE_UNPCKLPS_M128_to_XMM(to, from);
}

static __forceinline void SSEX_PUNPCKLDQ_XMM_to_XMM(x86SSERegType to, x86SSERegType from)
{
	if( g_xmmtypes[from] == XMMT_INT ) SSE2_PUNPCKLDQ_XMM_to_XMM(to, from);
	else SSE_UNPCKLPS_XMM_to_XMM(to, from);
}

static __forceinline void SSEX_PUNPCKHDQ_M128_to_XMM(x86SSERegType to, uptr from)
{
	if( g_xmmtypes[to] == XMMT_INT ) SSE2_PUNPCKHDQ_M128_to_XMM(to, from);
	else SSE_UNPCKHPS_M128_to_XMM(to, from);
}

static __forceinline void SSEX_PUNPCKHDQ_XMM_to_XMM(x86SSERegType to, x86SSERegType from)
{
	if( g_xmmtypes[from] == XMMT_INT ) SSE2_PUNPCKHDQ_XMM_to_XMM(to, from);
	else SSE_UNPCKHPS_XMM_to_XMM(to, from);
}

static __forceinline void SSEX_MOVHLPS_XMM_to_XMM( x86SSERegType to, x86SSERegType from )
{
	if( g_xmmtypes[from] == XMMT_INT ) {
		SSE2_PUNPCKHQDQ_XMM_to_XMM(to, from);
		if( to != from ) SSE2_PSHUFD_XMM_to_XMM(to, to, 0x4e);
	}
	else {
		SSE_MOVHLPS_XMM_to_XMM(to, from);
	}
}
