/*  Pcsx2 - Pc Ps2 Emulator
*  Copyright (C) 2009  Pcsx2-Playground Team
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
#pragma once
#include "PrecompiledHeader.h"
#include "microVU.h"
#ifdef PCSX2_MICROVU

//------------------------------------------------------------------
// Micro VU Micromode Lower instructions
//------------------------------------------------------------------

microVUf(void) mVU_DIV(){}
microVUf(void) mVU_SQRT(){}
microVUf(void) mVU_RSQRT(){}
microVUf(void) mVU_IADD(){}
microVUf(void) mVU_IADDI(){}
microVUf(void) mVU_IADDIU(){}
microVUf(void) mVU_IAND(){}
microVUf(void) mVU_IOR(){}
microVUf(void) mVU_ISUB(){}
microVUf(void) mVU_ISUBIU(){}
microVUf(void) mVU_MOVE(){}
microVUf(void) mVU_MFIR(){}
microVUf(void) mVU_MTIR(){}
microVUf(void) mVU_MR32(){}
microVUf(void) mVU_LQ(){}
microVUf(void) mVU_LQD(){}
microVUf(void) mVU_LQI(){}
microVUf(void) mVU_SQ(){}
microVUf(void) mVU_SQD(){}
microVUf(void) mVU_SQI(){}
microVUf(void) mVU_ILW(){}
microVUf(void) mVU_ISW(){}
microVUf(void) mVU_ILWR(){}
microVUf(void) mVU_ISWR(){}
microVUf(void) mVU_LOI(){}
microVUf(void) mVU_RINIT(){}
microVUf(void) mVU_RGET(){}
microVUf(void) mVU_RNEXT(){}
microVUf(void) mVU_RXOR(){}
microVUf(void) mVU_WAITQ(){}
microVUf(void) mVU_FSAND(){}
microVUf(void) mVU_FSEQ(){}
microVUf(void) mVU_FSOR(){}
microVUf(void) mVU_FSSET(){}
microVUf(void) mVU_FMAND(){}
microVUf(void) mVU_FMEQ(){}
microVUf(void) mVU_FMOR(){}
microVUf(void) mVU_FCAND(){}
microVUf(void) mVU_FCEQ(){}
microVUf(void) mVU_FCOR(){}
microVUf(void) mVU_FCSET(){}
microVUf(void) mVU_FCGET(){}
microVUf(void) mVU_IBEQ(){}
microVUf(void) mVU_IBGEZ(){}
microVUf(void) mVU_IBGTZ(){}
microVUf(void) mVU_IBLTZ(){}
microVUf(void) mVU_IBLEZ(){}
microVUf(void) mVU_IBNE(){}
microVUf(void) mVU_B(){}
microVUf(void) mVU_BAL(){}
microVUf(void) mVU_JR(){}
microVUf(void) mVU_JALR(){}
microVUf(void) mVU_MFP(){}
microVUf(void) mVU_WAITP(){}
microVUf(void) mVU_ESADD(){}
microVUf(void) mVU_ERSADD(){}
microVUf(void) mVU_ELENG(){}
microVUf(void) mVU_ERLENG(){}
microVUf(void) mVU_EATANxy(){}
microVUf(void) mVU_EATANxz(){}
microVUf(void) mVU_ESUM(){}
microVUf(void) mVU_ERCPR(){}
microVUf(void) mVU_ESQRT(){}
microVUf(void) mVU_ERSQRT(){}
microVUf(void) mVU_ESIN(){} 
microVUf(void) mVU_EATAN(){}
microVUf(void) mVU_EEXP(){}
microVUf(void) mVU_XGKICK(){}
microVUf(void) mVU_XTOP(){}
microVUf(void) mVU_XITOP(){}
#endif //PCSX2_MICROVU
