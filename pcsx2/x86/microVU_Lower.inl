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
#ifdef PCSX2_MICROVU

//------------------------------------------------------------------
// Micro VU Micromode Lower instructions
//------------------------------------------------------------------

microVUf(void) mVU_DIV(){}
microVUf(void) mVU_SQRT(){}
microVUf(void) mVU_RSQRT(){}

microVUf(void) mVU_EATAN(){}
microVUf(void) mVU_EATANxy(){}
microVUf(void) mVU_EATANxz(){}
microVUf(void) mVU_EEXP(){}
microVUf(void) mVU_ELENG(){}
microVUf(void) mVU_ERCPR(){}
microVUf(void) mVU_ERLENG(){}
microVUf(void) mVU_ERSADD(){}
microVUf(void) mVU_ERSQRT(){}
microVUf(void) mVU_ESADD(){}
microVUf(void) mVU_ESIN(){} 
microVUf(void) mVU_ESQRT(){}
microVUf(void) mVU_ESUM(){}

microVUf(void) mVU_FCAND(){}
microVUf(void) mVU_FCEQ(){}
microVUf(void) mVU_FCOR(){}
microVUf(void) mVU_FCSET(){}
microVUf(void) mVU_FCGET(){}

microVUf(void) mVU_FMAND() {
	microVU* mVU = mVUx;
	if (recPass == 0) {}
	else { 
		mVUallocMFLAGa<vuIndex>(gprT1, fvmInstance);
		mVUallocVIa<vuIndex>(gprT2, _Fs_);
		AND16RtoR(gprT1, gprT2);
		mVUallocVIb<vuIndex>(gprT1, _Ft_);
	}
}
microVUf(void) mVU_FMEQ() {
	microVU* mVU = mVUx;
	if (recPass == 0) {}
	else { 
		mVUallocMFLAGa<vuIndex>(gprT1, fvmInstance);
		mVUallocVIa<vuIndex>(gprT2, _Fs_);
		CMP16RtoR(gprT1, gprT2);
		SETE8R(gprT1);
		AND16ItoR(gprT1, 0x1);
		mVUallocVIb<vuIndex>(gprT1, _Ft_);
	}
}
microVUf(void) mVU_FMOR() {
	microVU* mVU = mVUx;
	if (recPass == 0) {}
	else { 
		mVUallocMFLAGa<vuIndex>(gprT1, fvmInstance);
		mVUallocVIa<vuIndex>(gprT2, _Fs_);
		OR16RtoR(gprT1, gprT2);
		mVUallocVIb<vuIndex>(gprT1, _Ft_);
	}
}

microVUf(void) mVU_FSAND() {
	microVU* mVU = mVUx;
	if (recPass == 0) {}
	else { 
		mVUallocSFLAGa<vuIndex>(gprT1, fvsInstance);
		AND16ItoR(gprT1, _Imm12_);
		mVUallocVIb<vuIndex>(gprT1, _Ft_);
	}
}
microVUf(void) mVU_FSEQ() {
	microVU* mVU = mVUx;
	if (recPass == 0) {}
	else { 
		mVUallocSFLAGa<vuIndex>(gprT1, fvsInstance);
		CMP16ItoR(gprT1, _Imm12_);
		SETE8R(gprT1);
		AND16ItoR(gprT1, 0x1);
		mVUallocVIb<vuIndex>(gprT1, _Ft_);
	}
}
microVUf(void) mVU_FSOR() {
	microVU* mVU = mVUx;
	if (recPass == 0) {}
	else { 
		mVUallocSFLAGa<vuIndex>(gprT1, fvsInstance);
		OR16ItoR(gprT1, _Imm12_);
		mVUallocVIb<vuIndex>(gprT1, _Ft_);
	}
}
microVUf(void) mVU_FSSET() {
	microVU* mVU = mVUx;
	if (recPass == 0) {}
	else { 
		int flagReg;
		getFlagReg(flagReg, fsInstance);
		MOV16ItoR(gprT1, (_Imm12_ & 0xfc0));
	}
}

microVUf(void) mVU_IADD(){}
microVUf(void) mVU_IADDI(){}
microVUf(void) mVU_IADDIU(){}
microVUf(void) mVU_IAND(){}
microVUf(void) mVU_IOR(){}
microVUf(void) mVU_ISUB(){}
microVUf(void) mVU_ISUBIU(){}

microVUf(void) mVU_B(){}
microVUf(void) mVU_BAL(){}
microVUf(void) mVU_IBEQ(){}
microVUf(void) mVU_IBGEZ(){}
microVUf(void) mVU_IBGTZ(){}
microVUf(void) mVU_IBLTZ(){}
microVUf(void) mVU_IBLEZ(){}
microVUf(void) mVU_IBNE(){}
microVUf(void) mVU_JR(){}
microVUf(void) mVU_JALR(){}

microVUf(void) mVU_ILW(){}
microVUf(void) mVU_ISW(){}
microVUf(void) mVU_ILWR(){}
microVUf(void) mVU_ISWR(){}

microVUf(void) mVU_MOVE() {
	microVU* mVU = mVUx;
	if (recPass == 0) {}
	else { 
		mVUloadReg<vuIndex>(xmmT1, (uptr)&mVU->regs->VF[_Fs_].UL[0], _X_Y_Z_W);
		mVUsaveReg<vuIndex>(xmmT1, (uptr)&mVU->regs->VF[_Ft_].UL[0], _X_Y_Z_W);
	}
}
microVUf(void) mVU_MFIR() {
	microVU* mVU = mVUx;
	if (recPass == 0) {}
	else { 
		mVUallocVIa<vuIndex>(gprT1, _Fs_);
		SHL32ItoR(gprT1, 16);
		SAR32ItoR(gprT1, 16);
		SSE2_MOVD_R_to_XMM(xmmT1, gprT1);
		if (!_XYZW_SS) { mVUunpack_xyzw<vuIndex>(xmmT1, xmmT1, 0); }
		mVUsaveReg<vuIndex>(xmmT1, (uptr)&mVU->regs->VF[_Ft_].UL[0], _X_Y_Z_W);
	}
}
microVUf(void) mVU_MFP() {
	microVU* mVU = mVUx;
	if (recPass == 0) {}
	else { 
		getPreg(xmmFt);
		mVUsaveReg<vuIndex>(xmmFt, (uptr)&mVU->regs->VF[_Ft_].UL[0], _X_Y_Z_W);
	}
}
microVUf(void) mVU_MTIR() {
	microVU* mVU = mVUx;
	if (recPass == 0) {}
	else { 
		MOVZX32M16toR(gprT1, (uptr)&mVU->regs->VF[_Fs_].UL[_Fsf_]);
		mVUallocVIb<vuIndex>(gprT1, _Ft_);
	}
}
microVUf(void) mVU_MR32() {
	microVU* mVU = mVUx;
	if (recPass == 0) {}
	else { 
		mVUloadReg<vuIndex>(xmmT1, (uptr)&mVU->regs->VF[_Fs_].UL[0], (_X_Y_Z_W == 8) ? 4 : 15);
		if (_X_Y_Z_W != 8) { SSE_SHUFPS_XMM_to_XMM(xmmT1, xmmT1, 0x39); }
		mVUsaveReg<vuIndex>(xmmT1, (uptr)&mVU->regs->VF[_Ft_].UL[0], _X_Y_Z_W);
	}
}

microVUf(void) mVU_LQ(){}
microVUf(void) mVU_LQD(){}
microVUf(void) mVU_LQI(){}
microVUf(void) mVU_SQ(){}
microVUf(void) mVU_SQD(){}
microVUf(void) mVU_SQI(){}
//microVUf(void) mVU_LOI(){}

microVUf(void) mVU_RINIT(){}
microVUf(void) mVU_RGET(){}
microVUf(void) mVU_RNEXT(){}
microVUf(void) mVU_RXOR(){}

microVUf(void) mVU_WAITP(){}
microVUf(void) mVU_WAITQ(){}

microVUf(void) mVU_XGKICK(){}
microVUf(void) mVU_XTOP() {
	microVU* mVU = mVUx;
	if (recPass == 0) {}
	else { 
		MOVZX32M16toR( gprT1, (uptr)&mVU->regs->vifRegs->top);
		mVUallocVIb<vuIndex>(gprT1, _Ft_);
	}
}
microVUf(void) mVU_XITOP() {
	microVU* mVU = mVUx;
	if (recPass == 0) {}
	else { 
		MOVZX32M16toR( gprT1, (uptr)&mVU->regs->vifRegs->itop );
		mVUallocVIb<vuIndex>(gprT1, _Ft_);
	}
}
#endif //PCSX2_MICROVU
