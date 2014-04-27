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

namespace x86Emitter {

// helpermess is currently broken >_<

#if 0

template< typename xImpl, typename T >
void _DoI_helpermess( const xImpl& helpme, const xDirectOrIndirect& to, const xImmReg<T>& immOrReg )
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
		helpme( (ModSibStrict)to, immOrReg.GetImm() );
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
#endif

}	// End namespace x86Emitter
