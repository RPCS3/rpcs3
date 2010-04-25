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

#include "PrecompiledHeader.h"
#include "internal.h"
#include "implement/helpers.h"

namespace x86Emitter {

void _xMovRtoR( const xRegisterInt& to, const xRegisterInt& from )
{
	pxAssert( to.GetOperandSize() == from.GetOperandSize() );

	if( to == from ) return;	// ignore redundant MOVs.

	from.prefix16();
	xWrite8( from.Is8BitOp() ? 0x88 : 0x89 );
	EmitSibMagic( from, to );
}

void xImpl_Mov::operator()( const xRegister8& to, const xRegister8& from ) const
{
	if( to == from ) return;	// ignore redundant MOVs.
	xWrite8( 0x88 );
	EmitSibMagic( from, to );
}

void xImpl_Mov::operator()( const xRegister16& to, const xRegister16& from ) const
{
	if( to == from ) return;	// ignore redundant MOVs.
	from.prefix16();
	xWrite8( 0x89 );
	EmitSibMagic( from, to );
}

void xImpl_Mov::operator()( const xRegister32& to, const xRegister32& from ) const
{
	if( to == from ) return;	// ignore redundant MOVs.
	xWrite8( 0x89 );
	EmitSibMagic( from, to );
}

void xImpl_Mov::operator()( const ModSibBase& dest, const xRegisterInt& from ) const
{
	from.prefix16();

	// mov eax has a special from when writing directly to a DISP32 address
	// (sans any register index/base registers).

	if( from.IsAccumulator() && dest.Index.IsEmpty() && dest.Base.IsEmpty() )
	{
		xWrite8( from.Is8BitOp() ? 0xa2 : 0xa3 );
		xWrite32( dest.Displacement );
	}
	else
	{
		xWrite8( from.Is8BitOp() ? 0x88 : 0x89 );
		EmitSibMagic( from.Id, dest );
	}
}

void xImpl_Mov::operator()( const xRegisterInt& to, const ModSibBase& src ) const
{
	to.prefix16();

	// mov eax has a special from when reading directly from a DISP32 address
	// (sans any register index/base registers).

	if( to.IsAccumulator() && src.Index.IsEmpty() && src.Base.IsEmpty() )
	{
		xWrite8( to.Is8BitOp() ? 0xa0 : 0xa1 );
		xWrite32( src.Displacement );
	}
	else
	{
		xWrite8( to.Is8BitOp() ? 0x8a : 0x8b );
		EmitSibMagic( to, src );
	}
}

void xImpl_Mov::operator()( const ModSib32orLess& dest, int imm ) const
{
	dest.prefix16();
	xWrite8( dest.Is8BitOp() ? 0xc6 : 0xc7 );
	EmitSibMagic( 0, dest );
	dest.xWriteImm( imm );
}

// preserve_flags  - set to true to disable optimizations which could alter the state of
//   the flags (namely replacing mov reg,0 with xor).
void xImpl_Mov::operator()( const xRegisterInt& to, int imm, bool preserve_flags ) const
{
	if( !preserve_flags && (imm == 0) )
		_g1_EmitOp( G1Type_XOR, to, to );
	else
	{
		// Note: MOV does not have (reg16/32,imm8) forms.

		to.prefix16();
		xWrite8( (to.Is8BitOp() ? 0xb0 : 0xb8) | to.Id );
		to.xWriteImm( imm );
	}
}

const xImpl_Mov xMOV;

// --------------------------------------------------------------------------------------
//  CMOVcc
// --------------------------------------------------------------------------------------

#define ccSane()	pxAssertDev( ccType >= 0 && ccType <= 0x0f, "Invalid comparison type specifier." )

// Macro useful for trapping unwanted use of EBP.
//#define EbpAssert() pxAssert( to != ebp )
#define EbpAssert()


void xCMOV( JccComparisonType ccType, const xRegister32& to, const xRegister32& from )		{ ccSane(); xOpWrite0F( 0x40 | ccType, to, from ); }
void xCMOV( JccComparisonType ccType, const xRegister32& to, const ModSibBase& sibsrc )		{ ccSane(); xOpWrite0F( 0x40 | ccType, to, sibsrc ); }
//void xCMOV( JccComparisonType ccType, const xDirectOrIndirect32& to, const xDirectOrIndirect32& from ) const { ccSane(); _DoI_helpermess( *this, to, from ); }	// too.. lazy.. to fix.

void xCMOV( JccComparisonType ccType, const xRegister16& to, const xRegister16& from )		{ ccSane(); xOpWrite0F( 0x66, 0x40 | ccType, to, from ); }
void xCMOV( JccComparisonType ccType, const xRegister16& to, const ModSibBase& sibsrc )		{ ccSane(); xOpWrite0F( 0x66, 0x40 | ccType, to, sibsrc ); }
//void xCMOV( JccComparisonType ccType, const xDirectOrIndirect16& to, const xDirectOrIndirect16& from ) const { ccSane(); _DoI_helpermess( *this, to, from ); }

void xSET( JccComparisonType ccType, const xRegister8& to )		{ ccSane(); xOpWrite0F( 0x90 | ccType, 0, to ); }
void xSET( JccComparisonType ccType, const ModSib8& dest )		{ ccSane(); xOpWrite0F( 0x90 | ccType, 0, dest ); }

void xImpl_CMov::operator()( const xRegister32& to, const xRegister32& from ) const					{ ccSane(); xOpWrite0F( 0x40 | ccType, to, from ); }
void xImpl_CMov::operator()( const xRegister32& to, const ModSibBase& sibsrc ) const				{ ccSane(); xOpWrite0F( 0x40 | ccType, to, sibsrc ); }
void xImpl_CMov::operator()( const xRegister16& to, const xRegister16& from ) const					{ ccSane(); xOpWrite0F( 0x66, 0x40 | ccType, to, from ); }
void xImpl_CMov::operator()( const xRegister16& to, const ModSibBase& sibsrc ) const				{ ccSane(); xOpWrite0F( 0x66, 0x40 | ccType, to, sibsrc ); }

//void xImpl_CMov::operator()( const xDirectOrIndirect32& to, const xDirectOrIndirect32& from ) const { ccSane(); _DoI_helpermess( *this, to, from ); }
//void xImpl_CMov::operator()( const xDirectOrIndirect16& to, const xDirectOrIndirect16& from ) const { ccSane(); _DoI_helpermess( *this, to, from ); }

void xImpl_Set::operator()( const xRegister8& to ) const				{ ccSane(); xOpWrite0F( 0x90 | ccType, 0, to ); }
void xImpl_Set::operator()( const ModSib8& dest ) const					{ ccSane(); xOpWrite0F( 0x90 | ccType, 0, dest ); }
//void xImpl_Set::operator()( const xDirectOrIndirect8& dest ) const		{ ccSane(); _DoI_helpermess( *this, dest ); }

void xImpl_MovExtend::operator()( const xRegister16or32& to, const xRegister8& from ) const
{
	EbpAssert();
	xOpWrite0F(
		( to->GetOperandSize() == 2 ) ? 0x66 : 0,
		SignExtend ? 0xbe : 0xb6,
		to, from
	);
}

void xImpl_MovExtend::operator()( const xRegister16or32& to, const ModSib8& sibsrc ) const
{
	EbpAssert();
	xOpWrite0F(
		( to->GetOperandSize() == 2 ) ? 0x66 : 0,
		SignExtend ? 0xbe : 0xb6,
		to, sibsrc
	);
}

void xImpl_MovExtend::operator()( const xRegister32& to, const xRegister16& from ) const
{
	EbpAssert();
	xOpWrite0F( SignExtend ? 0xbf : 0xb7, to, from );
}

void xImpl_MovExtend::operator()( const xRegister32& to, const ModSib16& sibsrc ) const
{
	EbpAssert();
	xOpWrite0F( SignExtend ? 0xbf : 0xb7, to, sibsrc );
}

#if 0
void xImpl_MovExtend::operator()( const xRegister32& to, const xDirectOrIndirect16& src ) const
{
	EbpAssert();
	_DoI_helpermess( *this, to, src );
}

void xImpl_MovExtend::operator()( const xRegister16or32& to, const xDirectOrIndirect8& src ) const
{
	EbpAssert();
	_DoI_helpermess( *this, to, src );
}
#endif

const xImpl_MovExtend xMOVSX = { true };
const xImpl_MovExtend xMOVZX = { false };

const xImpl_CMov	xCMOVA	= { Jcc_Above };
const xImpl_CMov	xCMOVAE	= { Jcc_AboveOrEqual };
const xImpl_CMov	xCMOVB	= { Jcc_Below };
const xImpl_CMov	xCMOVBE	= { Jcc_BelowOrEqual };

const xImpl_CMov	xCMOVG	= { Jcc_Greater };
const xImpl_CMov	xCMOVGE	= { Jcc_GreaterOrEqual };
const xImpl_CMov	xCMOVL	= { Jcc_Less };
const xImpl_CMov	xCMOVLE	= { Jcc_LessOrEqual };

const xImpl_CMov	xCMOVZ	= { Jcc_Zero };
const xImpl_CMov	xCMOVE	= { Jcc_Equal };
const xImpl_CMov	xCMOVNZ	= { Jcc_NotZero };
const xImpl_CMov	xCMOVNE	= { Jcc_NotEqual };

const xImpl_CMov	xCMOVO	= { Jcc_Overflow };
const xImpl_CMov	xCMOVNO	= { Jcc_NotOverflow };
const xImpl_CMov	xCMOVC	= { Jcc_Carry };
const xImpl_CMov	xCMOVNC	= { Jcc_NotCarry };

const xImpl_CMov	xCMOVS	= { Jcc_Signed };
const xImpl_CMov	xCMOVNS	= { Jcc_Unsigned };
const xImpl_CMov	xCMOVPE	= { Jcc_ParityEven };
const xImpl_CMov	xCMOVPO	= { Jcc_ParityOdd };


const xImpl_Set		xSETA	= { Jcc_Above };
const xImpl_Set		xSETAE	= { Jcc_AboveOrEqual };
const xImpl_Set		xSETB	= { Jcc_Below };
const xImpl_Set		xSETBE	= { Jcc_BelowOrEqual };

const xImpl_Set		xSETG	= { Jcc_Greater };
const xImpl_Set		xSETGE	= { Jcc_GreaterOrEqual };
const xImpl_Set		xSETL	= { Jcc_Less };
const xImpl_Set		xSETLE	= { Jcc_LessOrEqual };

const xImpl_Set		xSETZ	= { Jcc_Zero };
const xImpl_Set		xSETE	= { Jcc_Equal };
const xImpl_Set		xSETNZ	= { Jcc_NotZero };
const xImpl_Set		xSETNE	= { Jcc_NotEqual };

const xImpl_Set		xSETO	= { Jcc_Overflow };
const xImpl_Set		xSETNO	= { Jcc_NotOverflow };
const xImpl_Set		xSETC	= { Jcc_Carry };
const xImpl_Set		xSETNC	= { Jcc_NotCarry };

const xImpl_Set		xSETS	= { Jcc_Signed };
const xImpl_Set		xSETNS	= { Jcc_Unsigned };
const xImpl_Set		xSETPE	= { Jcc_ParityEven };
const xImpl_Set		xSETPO	= { Jcc_ParityOdd };

}	// end namespace x86Emitter
