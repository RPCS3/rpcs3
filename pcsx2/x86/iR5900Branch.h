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

#ifndef __IR5900BRANCH_H__
#define __IR5900BRANCH_H__

/*********************************************************
* Shift arithmetic with constant shift                   *
* Format:  OP rd, rt, sa                                 *
*********************************************************/

namespace R5900 { 
namespace Dynarec { 
namespace OpcodeImpl {

	void recBEQ( void );
	void recBEQL( void );
	void recBNE( void );
	void recBNEL( void );
	void recBLTZ( void );
	void recBLTZL( void );
	void recBLTZAL( void );
	void recBLTZALL( void );
	void recBGTZ( void );
	void recBGTZL( void );
	void recBLEZ( void );
	void recBLEZL( void );
	void recBGEZ( void );
	void recBGEZL( void );
	void recBGEZAL( void );
	void recBGEZALL( void );
} } }

#endif
