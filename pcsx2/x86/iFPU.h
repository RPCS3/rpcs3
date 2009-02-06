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

#ifndef __IFPU_H__
#define __IFPU_H__

namespace R5900 { 
namespace Dynarec { 

	void SaveCW();
	void LoadCW();

	namespace OpcodeImpl {
	namespace COP1
	{
		void recMFC1( void );
		void recCFC1( void );
		void recMTC1( void );
		void recCTC1( void );
		void recCOP1_BC1( void );
		void recCOP1_S( void );
		void recCOP1_W( void );
		void recC_EQ( void );
		void recC_F( void );
		void recC_LT( void );
		void recC_LE( void );
		void recADD_S( void );
		void recSUB_S( void );
		void recMUL_S( void );
		void recDIV_S( void );
		void recSQRT_S( void );
		void recABS_S( void );
		void recMOV_S( void );
		void recNEG_S( void );
		void recRSQRT_S( void );
		void recADDA_S( void );
		void recSUBA_S( void );
		void recMULA_S( void );
		void recMADD_S( void );
		void recMSUB_S( void );
		void recMADDA_S( void );
		void recMSUBA_S( void );
		void recCVT_S( void );
		void recCVT_W( void );
		void recMAX_S( void );
		void recMIN_S( void );
		void recBC1F( void );
		void recBC1T( void );
		void recBC1FL( void );
		void recBC1TL( void );
	} }
} }

#endif


