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

#pragma once

// Header: ix86_impl_movs.h -- covers mov, cmov, movsx/movzx, and SETcc (which shares
// with cmov many similarities).

namespace x86Emitter {

// --------------------------------------------------------------------------------------
//  MovImplAll
// --------------------------------------------------------------------------------------
// MOV instruction Implementation, plus many SIMD sub-mov variants.
//
struct xImpl_Mov
{
	xImpl_Mov() {} // Satisfy GCC's whims.

	void operator()( const xRegister8& to, const xRegister8& from ) const;
	void operator()( const xRegister16& to, const xRegister16& from ) const;
	void operator()( const xRegister32& to, const xRegister32& from ) const;

	void operator()( const ModSibBase& dest, const xRegisterInt& from ) const;
	void operator()( const xRegisterInt& to, const ModSibBase& src ) const;
	void operator()( const ModSib32orLess& dest, int imm ) const;
	void operator()( const xRegisterInt& to, int imm, bool preserve_flags=false ) const;

#if 0
	template< typename T > __noinline void operator()( const ModSibBase& to, const xImmReg<T>& immOrReg ) const
	{
		_DoI_helpermess( *this, to, immOrReg );
	}

	template< typename T > __noinline void operator()( const xDirectOrIndirect<T>& to, const xImmReg<T>& immOrReg ) const
	{
		_DoI_helpermess( *this, to, immOrReg );
	}

	template< typename T > __noinline void operator()( const xDirectOrIndirect<T>& to, int imm ) const
	{
		_DoI_helpermess( *this, to, imm );
	}

	template< typename T > __noinline void operator()( const xDirectOrIndirect<T>& to, const xDirectOrIndirect<T>& from ) const
	{
		if( to == from ) return;
		_DoI_helpermess( *this, to, from );
	}

	/*template< typename T > __noinline void operator()( const xRegister<T>& to, const xDirectOrIndirect<T>& from ) const
	{
		_DoI_helpermess( *this, xDirectOrIndirect<T>( to ), from );
	}
	
	template< typename T > __noinline void operator()( const xDirectOrIndirect<T>& to, const xRegister<T>& from ) const
	{
		_DoI_helpermess( *this, to, xDirectOrIndirect<T>( from ) );
	}*/
#endif
};

// --------------------------------------------------------------------------------------
//  xImpl_CMov
// --------------------------------------------------------------------------------------
// CMOVcc !!  [in all of it's disappointing lack-of glory]  .. and ..
// SETcc !!  [more glory, less lack!]
//
// CMOV Disclaimer: Caution!  This instruction can look exciting and cool, until you
// realize that it cannot load immediate values into registers. -_-
//
// I use explicit method declarations here instead of templates, in order to provide
// *only* 32 and 16 bit register operand forms (8 bit registers are not valid in CMOV).
//

struct xImpl_CMov
{
	JccComparisonType	ccType;

	void operator()( const xRegister32& to, const xRegister32& from ) const;
	void operator()( const xRegister32& to, const ModSibBase& sibsrc ) const;

	void operator()( const xRegister16& to, const xRegister16& from ) const;
	void operator()( const xRegister16& to, const ModSibBase& sibsrc ) const;

	//void operator()( const xDirectOrIndirect32& to, const xDirectOrIndirect32& from );
	//void operator()( const xDirectOrIndirect16& to, const xDirectOrIndirect16& from ) const;
};

struct xImpl_Set
{
	JccComparisonType ccType;

	void operator()( const xRegister8& to ) const;
	void operator()( const ModSib8& dest ) const;

	//void operator()( const xDirectOrIndirect8& dest ) const;
};


// --------------------------------------------------------------------------------------
//  xImpl_MovExtend
// --------------------------------------------------------------------------------------
// Mov with sign/zero extension implementations (movsx / movzx)
//
struct xImpl_MovExtend
{
	bool	SignExtend;

	void operator()( const xRegister16or32& to, const xRegister8& from ) const;
	void operator()( const xRegister16or32& to, const ModSib8& sibsrc ) const;
	void operator()( const xRegister32& to, const xRegister16& from ) const;
	void operator()( const xRegister32& to, const ModSib16& sibsrc ) const;

	//void operator()( const xRegister32& to, const xDirectOrIndirect16& src ) const;
	//void operator()( const xRegister16or32& to, const xDirectOrIndirect8& src ) const;
	//void operator()( const xRegister16& to, const xDirectOrIndirect8& src ) const;
};

}	// End namespace x86Emitter
