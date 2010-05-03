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

/*
 * ix86 core v0.9.1
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

#pragma once

// This header module contains functions which, under most circumstances, inline
// nicely with constant propagation from the compiler, resulting in little or
// no actual codegen in the majority of emitter statements. (common forms include:
// RegToReg, PointerToReg, RegToPointer).  These cannot be included in the class
// definitions in the .h file because of inter-dependencies with other classes.
//   (score one for C++!!)
//
// In order for MSVC to work correctly with __forceinline on class members,
// however, we need to include these methods into all source files which might
// reference them.  Without this MSVC generates linker errors.  Or, in other words,
// global optimization fails to resolve the externals and junk.
//   (score one for MSVC!)

namespace x86Emitter
{
	// --------------------------------------------------------------------------------------
	//  x86Register Method Implementations (inlined!)
	// --------------------------------------------------------------------------------------

	__forceinline xAddressInfo xAddressReg::operator+( const xAddressReg& right ) const
	{
		pxAssertMsg( Id != -1, "Uninitialized x86 register." );
		return xAddressInfo( *this, right );
	}

	__forceinline xAddressInfo xAddressReg::operator+( const xAddressInfo& right ) const
	{
		pxAssertMsg( Id != -1, "Uninitialized x86 register." );
		return right + *this;
	}

	__forceinline xAddressInfo xAddressReg::operator+( s32 right ) const
	{
		pxAssertMsg( Id != -1, "Uninitialized x86 register." );
		return xAddressInfo( *this, right );
	}

	__forceinline xAddressInfo xAddressReg::operator+( const void* right ) const
	{
		pxAssertMsg( Id != -1, "Uninitialized x86 register." );
		return xAddressInfo( *this, (s32)right );
	}

	// ------------------------------------------------------------------------
	__forceinline xAddressInfo xAddressReg::operator-( s32 right ) const
	{
		pxAssertMsg( Id != -1, "Uninitialized x86 register." );
		return xAddressInfo( *this, -right );
	}

	__forceinline xAddressInfo xAddressReg::operator-( const void* right ) const
	{
		pxAssertMsg( Id != -1, "Uninitialized x86 register." );
		return xAddressInfo( *this, -(s32)right );
	}

	// ------------------------------------------------------------------------
	__forceinline xAddressInfo xAddressReg::operator*( u32 right ) const
	{
		pxAssertMsg( Id != -1, "Uninitialized x86 register." );
		return xAddressInfo( xEmptyReg, *this, right );
	}

	__forceinline xAddressInfo xAddressReg::operator<<( u32 shift ) const
	{
		pxAssertMsg( Id != -1, "Uninitialized x86 register." );
		return xAddressInfo( xEmptyReg, *this, 1<<shift );
	}
}
