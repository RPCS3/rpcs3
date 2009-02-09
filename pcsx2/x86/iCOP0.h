/*  Pcsx2 - Pc Ps2 Emulator
 *  Copyright (C) 2002-2008  Pcsx2 Team
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
