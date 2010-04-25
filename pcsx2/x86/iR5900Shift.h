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

#ifndef __IR5900SHIFT_H__
#define __IR5900SHIFT_H__

/*********************************************************
* Shift arithmetic with constant shift                   *
* Format:  OP rd, rt, sa                                 *
*********************************************************/

namespace R5900 {
namespace Dynarec {
namespace OpcodeImpl {

	void recSLL( void );
	void recSRL( void );
	void recSRA( void );
	void recDSLL( void );
	void recDSRL( void );
	void recDSRA( void );
	void recDSLL32( void );
	void recDSRL32( void );
	void recDSRA32( void );

	void recSLLV( void );
	void recSRLV( void );
	void recSRAV( void );
	void recDSLLV( void );
	void recDSRLV( void );
	void recDSRAV( void );
} } }

#endif
