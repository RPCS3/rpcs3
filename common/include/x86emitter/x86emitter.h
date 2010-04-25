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

/*
 * ix86 public header v0.9.1
 *
 * Original Authors (v0.6.2 and prior):
 *		linuzappz <linuzappz@pcsx.net>
 *		alexey silinov
 *		goldfinger
 *		zerofrog(@gmail.com)
 *
 * Authors of v0.9.1:
 *		Jake.Stine(@gmail.com)
 *		cottonvibes(@gmail.com)
 *		sudonim(1@gmail.com)
 */

//////////////////////////////////////////////////////////////////////////////////////////
// New C++ Emitter!
//
// To use it just include the x86Emitter namespace into your file/class/function off choice.
//
// This header file is intended for use by public code.  It includes the appropriate
// inlines and class definitions for efficient codegen.  (code internal to the emitter
// should usually use ix86_internal.h instead, and manually include the
// ix86_inlines.inl file when it is known that inlining of ModSib functions are
// wanted).
//

#pragma once

#include "x86types.h"
#include "tools.h"
#include "instructions.h"

// Including legacy items for now, but these should be removed eventually,
// once most code is no longer dependent on them.
#include "legacy_types.h"
#include "legacy_instructions.h"

// --------------------------------------------------------------------------------------
//  CallAddress Macros -- An Optimization work-around hack!
// --------------------------------------------------------------------------------------
// MSVC 2008 fails to optimize direct invocation of static recompiled code buffers, instead
// insisting on "mov eax, immaddr; call eax".  Likewise, GCC fails to optimize it also, unless
// the typecast is explicitly inlined.  These macros account for these problems.
//

#ifdef _MSC_VER

#	define CallAddress( ptr ) \
		__asm{ call offset ptr }

#	define FastCallAddress( ptr, param1 ) \
		__asm{ __asm mov ecx, param1 __asm call offset ptr }

#	define FastCallAddress2( ptr, param1, param2 ) \
		__asm{ __asm mov ecx, param1 __asm mov edx, param2 __asm call offset ptr }

#else

#	define CallAddress( ptr ) \
		( (void (*)()) &(ptr)[0] )()

#	define FastCallAddress( ptr, param1 ) \
		( (void (*)( int )) &(ptr)[0] )( param1 )

#	define FastCallAddress2( ptr, param1, param2 ) \
		( (void (*)( int, int )) &(ptr)[0] )( param1, param2 )

#endif
