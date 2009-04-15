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

/*
 * ix86 public header v0.9.0
 *
 * Original Authors (v0.6.2 and prior):
 *		linuzappz <linuzappz@pcsx.net>
 *		alexey silinov
 *		goldfinger
 *		zerofrog(@gmail.com)
 *
 * Authors of v0.9.0:
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

#include "ix86_types.h"
#include "ix86_instructions.h"

// Including legacy items for now, but these should be removed eventually,
// once most code is no longer dependent on them.
#include "ix86_legacy_types.h"
#include "ix86_legacy_instructions.h"
