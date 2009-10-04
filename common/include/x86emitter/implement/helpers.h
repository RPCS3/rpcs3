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

// ------------------------------------------------------------------------
// helpers.h -- Various universally helpful functions for emitter convenience!
//
// Note: Header file should be included from the x86Emitter::Internal namespace, such
// that all members contained within are in said namespace.
// ------------------------------------------------------------------------


#pragma once

extern void SimdPrefix( u8 prefix, u16 opcode );
extern void EmitSibMagic( uint regfield, const void* address );
extern void EmitSibMagic( uint regfield, const ModSibBase& info );
extern void xJccKnownTarget( JccComparisonType comparison, const void* target, bool slideForward );

template< typename T > bool Is8BitOp() { return sizeof(T) == 1; }
template< typename T > void prefix16() { if( sizeof(T) == 2 ) xWrite8( 0x66 ); }


// Writes a ModRM byte for "Direct" register access forms, which is used for all
// instructions taking a form of [reg,reg].
template< typename T > __emitinline
void EmitSibMagic( uint reg1, const xRegisterBase<T>& reg2 )
{
	xWrite8( (Mod_Direct << 6) | (reg1 << 3) | reg2.Id );
}

template< typename T1, typename T2 > __emitinline
void EmitSibMagic( const xRegisterBase<T1> reg1, const xRegisterBase<T2>& reg2 )
{
	xWrite8( (Mod_Direct << 6) | (reg1.Id << 3) | reg2.Id );
}

template< typename T1 > __emitinline
void EmitSibMagic( const xRegisterBase<T1> reg1, const void* src )			{ EmitSibMagic( reg1.Id, src ); }

template< typename T1 > __emitinline
void EmitSibMagic( const xRegisterBase<T1> reg1, const ModSibBase& sib )	{ EmitSibMagic( reg1.Id, sib ); }

// ------------------------------------------------------------------------
template< typename T1, typename T2 > __emitinline
void xOpWrite( u8 prefix, u8 opcode, const T1& param1, const T2& param2 )
{
	if( prefix != 0 )
		xWrite16( (opcode<<8) | prefix );
	else
		xWrite8( opcode );

	EmitSibMagic( param1, param2 );
}

// ------------------------------------------------------------------------
template< typename T1, typename T2 > __emitinline
void xOpWrite0F( u8 prefix, u16 opcode, const T1& param1, const T2& param2 )
{
	SimdPrefix( prefix, opcode );
	EmitSibMagic( param1, param2 );
}

template< typename T1, typename T2 > __emitinline
void xOpWrite0F( u8 prefix, u16 opcode, const T1& param1, const T2& param2, u8 imm8 )
{
	xOpWrite0F( prefix, opcode, param1, param2 );
	xWrite8( imm8 );
}

template< typename T1, typename T2 > __emitinline
void xOpWrite0F( u16 opcode, const T1& param1, const T2& param2 )			{ xOpWrite0F( 0, opcode, param1, param2 ); }

template< typename T1, typename T2 > __emitinline
void xOpWrite0F( u16 opcode, const T1& param1, const T2& param2, u8 imm8 )	{ xOpWrite0F( 0, opcode, param1, param2, imm8 ); }

// ------------------------------------------------------------------------

template< typename xImpl, typename T >
void _DoI_helpermess( const xImpl& helpme, const xDirectOrIndirect<T>& to, const xImmReg<T>& immOrReg )
{
	if( to.IsDirect() )
	{
		if( immOrReg.IsReg() )
			helpme( to.GetReg(), immOrReg.GetReg() );
		else
			helpme( to.GetReg(), immOrReg.GetImm() );
	}
	else
	{
		if( immOrReg.IsReg() )
			helpme( to.GetMem(), immOrReg.GetReg() );
		else
			helpme( to.GetMem(), immOrReg.GetImm() );
	}
}

template< typename xImpl, typename T >
void _DoI_helpermess( const xImpl& helpme, const ModSibBase& to, const xImmReg<T>& immOrReg )
{
	if( immOrReg.IsReg() )
		helpme( to, immOrReg.GetReg() );
	else
		helpme( ModSibStrict<T>(to), immOrReg.GetImm() );
}

template< typename xImpl, typename T >
void _DoI_helpermess( const xImpl& helpme, const xDirectOrIndirect<T>& to, int imm )
{
	if( to.IsDirect() )
		helpme( to.GetReg(), imm );
	else
		helpme( to.GetMem(), imm );
}

template< typename xImpl, typename T >
void _DoI_helpermess( const xImpl& helpme, const xDirectOrIndirect<T>& parm )
{
	if( parm.IsDirect() )
		helpme( parm.GetReg() );
	else 
		helpme( parm.GetMem() );
}

template< typename xImpl, typename T >
void _DoI_helpermess( const xImpl& helpme, const xDirectOrIndirect<T>& to, const xDirectOrIndirect<T>& from )
{
	if( to.IsDirect() && from.IsDirect() )
		helpme( to.GetReg(), from.GetReg() );

	else if( to.IsDirect() )
		helpme( to.GetReg(), from.GetMem() );

	else if( from.IsDirect() )
		helpme( to.GetMem(), from.GetReg() );

	else

		// One of the fields needs to be direct, or else we cannot complete the operation.
		// (intel doesn't support indirects in both fields)

		pxFailDev( "Invalid asm instruction: Both operands are indirect memory addresses." );
}
