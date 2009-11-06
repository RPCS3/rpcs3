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

namespace x86Emitter {

enum G1Type
{
	G1Type_ADD=0,
	G1Type_OR,
	G1Type_ADC,
	G1Type_SBB,
	G1Type_AND,
	G1Type_SUB,
	G1Type_XOR,
	G1Type_CMP
};

// --------------------------------------------------------------------------------------
//  xImpl_Group1
// --------------------------------------------------------------------------------------
struct xImpl_Group1
{
	G1Type	InstType;

	void operator()( const xRegister8& to, const xRegister8& from ) const;
	void operator()( const xRegister16& to, const xRegister16& from ) const;
	void operator()( const xRegister32& to, const xRegister32& from ) const;

	void operator()( const ModSibBase& to, const xRegisterInt& from ) const;
	void operator()( const xRegisterInt& to, const ModSibBase& from ) const;
	void operator()( const xRegisterInt& to, int imm ) const;
	void operator()( const ModSib32orLess& to, int imm ) const;

#if 0
	// ------------------------------------------------------------------------
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
		_DoI_helpermess( *this, to, from );
	}

	// FIXME : Make this struct to 8, 16, and 32 bit registers
	template< typename T > __noinline void operator()( const xRegisterBase& to, const xDirectOrIndirect<T>& from ) const
	{
		_DoI_helpermess( *this, xDirectOrIndirect<T>( to ), from );
	}

	// FIXME : Make this struct to 8, 16, and 32 bit registers
	template< typename T > __noinline void operator()( const xDirectOrIndirect<T>& to, const xRegisterBase& from ) const
	{
		_DoI_helpermess( *this, to, xDirectOrIndirect<T>( from ) );
	}
#endif
};

// ------------------------------------------------------------------------
// This class combines x86 with SSE/SSE2 logic operations (ADD, OR, and NOT).
// Note: ANDN [AndNot] is handled below separately.
//
struct xImpl_G1Logic
{
	G1Type	InstType;

	void operator()( const xRegister8& to, const xRegister8& from ) const;
	void operator()( const xRegister16& to, const xRegister16& from ) const;
	void operator()( const xRegister32& to, const xRegister32& from ) const;

	void operator()( const ModSibBase& to, const xRegisterInt& from ) const;
	void operator()( const xRegisterInt& to, const ModSibBase& from ) const;
	void operator()( const xRegisterInt& to, int imm ) const;

	void operator()( const ModSib32orLess& to, int imm ) const;

	xImplSimd_DestRegSSE PS;			// packed single precision
	xImplSimd_DestRegSSE PD;			// packed double precision
};

// ------------------------------------------------------------------------
// This class combines x86 with SSE/SSE2 arithmetic operations (ADD/SUB).
//
struct xImpl_G1Arith
{
	G1Type	InstType;

	void operator()( const xRegister8& to, const xRegister8& from ) const;
	void operator()( const xRegister16& to, const xRegister16& from ) const;
	void operator()( const xRegister32& to, const xRegister32& from ) const;

	void operator()( const ModSibBase& to, const xRegisterInt& from ) const;
	void operator()( const xRegisterInt& to, const ModSibBase& from ) const;
	void operator()( const xRegisterInt& to, int imm ) const;

	void operator()( const ModSib32orLess& to, int imm ) const;

	xImplSimd_DestRegSSE PS;			// packed single precision
	xImplSimd_DestRegSSE PD;			// packed double precision
	xImplSimd_DestRegSSE SS;			// scalar single precision
	xImplSimd_DestRegSSE SD;			// scalar double precision
};

// ------------------------------------------------------------------------
struct xImpl_G1Compare
{
	void operator()( const xRegister8& to, const xRegister8& from ) const;
	void operator()( const xRegister16& to, const xRegister16& from ) const;
	void operator()( const xRegister32& to, const xRegister32& from ) const;

	void operator()( const ModSibBase& to, const xRegisterInt& from ) const;
	void operator()( const xRegisterInt& to, const ModSibBase& from ) const;
	void operator()( const xRegisterInt& to, int imm ) const;

	void operator()( const ModSib32orLess& to, int imm ) const;

	xImplSimd_DestSSE_CmpImm	PS;
	xImplSimd_DestSSE_CmpImm	PD;
	xImplSimd_DestSSE_CmpImm	SS;
	xImplSimd_DestSSE_CmpImm	SD;
};

}	// End namespace x86Emitter

