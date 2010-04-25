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

#ifndef __iCOP0_H__
#define __iCOP0_H__

#include "COP0.h"

/*********************************************************
*   COP0 opcodes                                         *
*                                                        *
*********************************************************/

namespace R5900 {
namespace Dynarec {
namespace OpcodeImpl {
namespace COP0
{
	void recMFC0( void );
	void recMTC0( void );
	void recBC0F( void );
	void recBC0T( void );
	void recBC0FL( void );
	void recBC0TL( void );
	void recTLBR( void );
	void recTLBWI( void );
	void recTLBWR( void );
	void recTLBP( void );
	void recERET( void );
	void recDI( void );
	void recEI( void );

}}}}
#endif
