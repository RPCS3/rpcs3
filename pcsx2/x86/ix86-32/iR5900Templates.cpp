/*  Pcsx2 - Pc Ps2 Emulator
 *  Copyright (C) 2002-2009  Pcsx2 Team
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA
 */


#include "PrecompiledHeader.h"

#include "Common.h"
#include "Memory.h"
#include "R5900OpcodeTables.h"
#include "iR5900.h"
#include "iMMI.h"
#include "iFPU.h"
#include "iCOP0.h"
#include "iVUmicro.h"
#include "VU.h"
#include "VUmicro.h"

#include "iVUzerorec.h"

#include "vtlb.h"

////////////////////
// Code Templates //
////////////////////

void CHECK_SAVE_REG(int reg)
{
	if( s_saveConstGPRreg == 0xffffffff ) {
		if( GPR_IS_CONST1(reg) ) {
			s_saveConstGPRreg = reg;
			s_ConstGPRreg = g_cpuConstRegs[reg];
		}
	}
	else {
		assert( s_saveConstGPRreg == 0 || s_saveConstGPRreg == reg );
	}
}

void _eeProcessHasLive(int reg, int signext)
{
	g_cpuPrevRegHasLive1 = g_cpuRegHasLive1;
	g_cpuRegHasLive1 |= 1<<reg;

	g_cpuPrevRegHasSignExt = g_cpuRegHasSignExt;

	if( signext ) {
		EEINST_SETSIGNEXT(reg);
	}
	else {
		EEINST_RESETSIGNEXT(reg);
	}
}

void _eeOnWriteReg(int reg, int signext)
{
	CHECK_SAVE_REG(reg);
	GPR_DEL_CONST(reg);
	_eeProcessHasLive(reg, signext);
}

void _deleteEEreg(int reg, int flush)
{
	if( !reg ) return;
	if( flush && GPR_IS_CONST1(reg) ) {
		_flushConstReg(reg);
		return;
	}
	GPR_DEL_CONST(reg);
	_deleteGPRtoXMMreg(reg, flush ? 0 : 2);
	_deleteMMXreg(MMX_GPR+reg, flush ? 0 : 2);
}

// if not mmx, then xmm
int eeProcessHILO(int reg, int mode, int mmx)
{
    int usemmx = mmx && _hasFreeMMXreg();
	if( (usemmx || _hasFreeXMMreg()) || !(g_pCurInstInfo->regs[reg]&EEINST_LASTUSE) ) {
		if( usemmx ) return _allocMMXreg(-1, MMX_GPR+reg, mode);
		return _allocGPRtoXMMreg(-1, reg, mode);
	}

	return -1;
}

#define PROCESS_EE_SETMODES(mmreg) ((mmxregs[mmreg].mode&MODE_WRITE)?PROCESS_EE_MODEWRITES:0)
#define PROCESS_EE_SETMODET(mmreg) ((mmxregs[mmreg].mode&MODE_WRITE)?PROCESS_EE_MODEWRITET:0)

// ignores XMMINFO_READS, XMMINFO_READT, and XMMINFO_READD_LO from xmminfo
// core of reg caching
void eeRecompileCode0(R5900FNPTR constcode, R5900FNPTR_INFO constscode, R5900FNPTR_INFO consttcode, R5900FNPTR_INFO noconstcode, int xmminfo)
{
	int mmreg1, mmreg2, mmreg3, mmtemp, moded;

	if ( ! _Rd_ && (xmminfo&XMMINFO_WRITED) ) return;

	if( xmminfo&XMMINFO_WRITED) {
		CHECK_SAVE_REG(_Rd_);
		_eeProcessHasLive(_Rd_, 0);
		EEINST_RESETSIGNEXT(_Rd_);
	}

	if( GPR_IS_CONST2(_Rs_, _Rt_) ) {
		if( xmminfo & XMMINFO_WRITED ) {
			_deleteMMXreg(MMX_GPR+_Rd_, 2);
			_deleteGPRtoXMMreg(_Rd_, 2);
		}
		if( xmminfo&XMMINFO_WRITED ) GPR_SET_CONST(_Rd_);
		constcode();
		return;
	}

	moded = MODE_WRITE|((xmminfo&XMMINFO_READD)?MODE_READ:0);

	// test if should write mmx
	if( g_pCurInstInfo->info & EEINST_MMX ) {

		if( xmminfo & (XMMINFO_READLO|XMMINFO_WRITELO) ) _addNeededMMXreg(MMX_GPR+MMX_LO);
		if( xmminfo & (XMMINFO_READHI|XMMINFO_WRITEHI) ) _addNeededMMXreg(MMX_GPR+MMX_HI);
		_addNeededMMXreg(MMX_GPR+_Rs_);
		_addNeededMMXreg(MMX_GPR+_Rt_);

		if( GPR_IS_CONST1(_Rs_) || GPR_IS_CONST1(_Rt_) ) {
			int creg = GPR_IS_CONST1(_Rs_) ? _Rs_ : _Rt_;
			int vreg = creg == _Rs_ ? _Rt_ : _Rs_;

//			if(g_pCurInstInfo->regs[vreg]&EEINST_MMX) {
//				mmreg1 = _allocMMXreg(-1, MMX_GPR+vreg, MODE_READ);
//				_addNeededMMXreg(MMX_GPR+vreg);
//			}
			mmreg1 = _allocCheckGPRtoMMX(g_pCurInstInfo, vreg, MODE_READ);

			if( mmreg1 >= 0 ) {
				int info = PROCESS_EE_MMX;
				
				if( GPR_IS_CONST1(_Rs_) ) info |= PROCESS_EE_SETMODET(mmreg1);
				else info |= PROCESS_EE_SETMODES(mmreg1);

				if( xmminfo & XMMINFO_WRITED ) {
					_addNeededMMXreg(MMX_GPR+_Rd_);
					mmreg3 = _checkMMXreg(MMX_GPR+_Rd_, moded);

					if( !(xmminfo&XMMINFO_READD) && mmreg3 < 0 && ((g_pCurInstInfo->regs[vreg] & EEINST_LASTUSE) || !EEINST_ISLIVE64(vreg)) ) {
						if( EEINST_ISLIVE64(vreg) ) {
							_freeMMXreg(mmreg1);
							if( GPR_IS_CONST1(_Rs_) ) info &= ~PROCESS_EE_MODEWRITET;
							else info &= ~PROCESS_EE_MODEWRITES;
						}
						_deleteGPRtoXMMreg(_Rd_, 2);
						mmxregs[mmreg1].inuse = 1;
						mmxregs[mmreg1].reg = _Rd_;
						mmxregs[mmreg1].mode = moded;
						mmreg3 = mmreg1;
					}
					else if( mmreg3 < 0 ) mmreg3 = _allocMMXreg(-1, MMX_GPR+_Rd_, moded);

					info |= PROCESS_EE_SET_D(mmreg3);
				}

				if( xmminfo & (XMMINFO_READLO|XMMINFO_WRITELO) ) {
					mmtemp = eeProcessHILO(MMX_LO, ((xmminfo&XMMINFO_READLO)?MODE_READ:0)|((xmminfo&XMMINFO_WRITELO)?MODE_WRITE:0), 1);
					if( mmtemp >= 0 ) info |= PROCESS_EE_SET_LO(mmtemp);
				}
				if( xmminfo & (XMMINFO_READHI|XMMINFO_WRITEHI) ) {
					mmtemp = eeProcessHILO(MMX_HI, ((xmminfo&XMMINFO_READLO)?MODE_READ:0)|((xmminfo&XMMINFO_WRITELO)?MODE_WRITE:0), 1);
					if( mmtemp >= 0 ) info |= PROCESS_EE_SET_HI(mmtemp);
				}

				SetMMXstate();
				if( creg == _Rs_ ) constscode(info|PROCESS_EE_SET_T(mmreg1));
				else consttcode(info|PROCESS_EE_SET_S(mmreg1));
				_clearNeededMMXregs();
				if( xmminfo & XMMINFO_WRITED ) GPR_DEL_CONST(_Rd_);
				return;
			}
		}
		else {
			// no const regs
			mmreg1 = _allocCheckGPRtoMMX(g_pCurInstInfo, _Rs_, MODE_READ);
			mmreg2 = _allocCheckGPRtoMMX(g_pCurInstInfo, _Rt_, MODE_READ);

			if( mmreg1 >= 0 || mmreg2 >= 0 ) {
				int info = PROCESS_EE_MMX;

				// do it all in mmx
				if( mmreg1 < 0 ) mmreg1 = _allocMMXreg(-1, MMX_GPR+_Rs_, MODE_READ);
				if( mmreg2 < 0 ) mmreg2 = _allocMMXreg(-1, MMX_GPR+_Rt_, MODE_READ);

				info |= PROCESS_EE_SETMODES(mmreg1)|PROCESS_EE_SETMODET(mmreg2);

				// check for last used, if so don't alloc a new MMX reg
				if( xmminfo & XMMINFO_WRITED ) {
					_addNeededMMXreg(MMX_GPR+_Rd_);
					mmreg3 = _checkMMXreg(MMX_GPR+_Rd_, moded);

					if( mmreg3 < 0 ) {
						if( !(xmminfo&XMMINFO_READD) && ((g_pCurInstInfo->regs[_Rt_] & EEINST_LASTUSE) || !EEINST_ISLIVE64(_Rt_)) ) {
							if( EEINST_ISLIVE64(_Rt_) ) {
								_freeMMXreg(mmreg2);
								info &= ~PROCESS_EE_MODEWRITET;
							}
							_deleteGPRtoXMMreg(_Rd_, 2);
							mmxregs[mmreg2].inuse = 1;
							mmxregs[mmreg2].reg = _Rd_;
							mmxregs[mmreg2].mode = moded;
							mmreg3 = mmreg2;
						}
						else if( !(xmminfo&XMMINFO_READD) && ((g_pCurInstInfo->regs[_Rs_] & EEINST_LASTUSE) || !EEINST_ISLIVE64(_Rs_)) ) {
							if( EEINST_ISLIVE64(_Rs_) ) {
								_freeMMXreg(mmreg1);
								info &= ~PROCESS_EE_MODEWRITES;
							}
							_deleteGPRtoXMMreg(_Rd_, 2);
							mmxregs[mmreg1].inuse = 1;
							mmxregs[mmreg1].reg = _Rd_;
							mmxregs[mmreg1].mode = moded;
							mmreg3 = mmreg1;
						}
						else mmreg3 = _allocMMXreg(-1, MMX_GPR+_Rd_, moded);
					}

					info |= PROCESS_EE_SET_D(mmreg3);
				}

				if( xmminfo & (XMMINFO_READLO|XMMINFO_WRITELO) ) {
					mmtemp = eeProcessHILO(MMX_LO, ((xmminfo&XMMINFO_READLO)?MODE_READ:0)|((xmminfo&XMMINFO_WRITELO)?MODE_WRITE:0), 1);
					if( mmtemp >= 0 ) info |= PROCESS_EE_SET_LO(mmtemp);
				}
				if( xmminfo & (XMMINFO_READHI|XMMINFO_WRITEHI) ) {
					mmtemp = eeProcessHILO(MMX_HI, ((xmminfo&XMMINFO_READLO)?MODE_READ:0)|((xmminfo&XMMINFO_WRITELO)?MODE_WRITE:0), 1);
					if( mmtemp >= 0 ) info |= PROCESS_EE_SET_HI(mmtemp);
				}

				SetMMXstate();
				noconstcode(info|PROCESS_EE_SET_S(mmreg1)|PROCESS_EE_SET_T(mmreg2));
				_clearNeededMMXregs();
				if( xmminfo & XMMINFO_WRITED ) GPR_DEL_CONST(_Rd_);
				return;
			}
		}

		_clearNeededMMXregs();
	}

	// test if should write xmm, mirror to mmx code
	if( g_pCurInstInfo->info & EEINST_XMM ) {

		if( xmminfo & (XMMINFO_READLO|XMMINFO_WRITELO) ) _addNeededGPRtoXMMreg(XMMGPR_LO);
		if( xmminfo & (XMMINFO_READHI|XMMINFO_WRITEHI) ) _addNeededGPRtoXMMreg(XMMGPR_HI);
		_addNeededGPRtoXMMreg(_Rs_);
		_addNeededGPRtoXMMreg(_Rt_);

		if( GPR_IS_CONST1(_Rs_) || GPR_IS_CONST1(_Rt_) ) {
			int creg = GPR_IS_CONST1(_Rs_) ? _Rs_ : _Rt_;
			int vreg = creg == _Rs_ ? _Rt_ : _Rs_;

//			if(g_pCurInstInfo->regs[vreg]&EEINST_XMM) {
//				mmreg1 = _allocGPRtoXMMreg(-1, vreg, MODE_READ);
//				_addNeededGPRtoXMMreg(vreg);
//			}
			mmreg1 = _allocCheckGPRtoXMM(g_pCurInstInfo, vreg, MODE_READ);

			if( mmreg1 >= 0 ) {
				int info = PROCESS_EE_XMM;

				if( GPR_IS_CONST1(_Rs_) ) info |= PROCESS_EE_SETMODET(mmreg1);
				else info |= PROCESS_EE_SETMODES(mmreg1);

				if( xmminfo & XMMINFO_WRITED ) {

					_addNeededGPRtoXMMreg(_Rd_);
					mmreg3 = _checkXMMreg(XMMTYPE_GPRREG, _Rd_, MODE_WRITE);

					if( !(xmminfo&XMMINFO_READD) && mmreg3 < 0 && ((g_pCurInstInfo->regs[vreg] & EEINST_LASTUSE) || !EEINST_ISLIVEXMM(vreg)) ) {
						_freeXMMreg(mmreg1);
						if( GPR_IS_CONST1(_Rs_) ) info &= ~PROCESS_EE_MODEWRITET;
						else info &= ~PROCESS_EE_MODEWRITES;
						_deleteMMXreg(MMX_GPR+_Rd_, 2);
						xmmregs[mmreg1].inuse = 1;
						xmmregs[mmreg1].reg = _Rd_;
						xmmregs[mmreg1].mode = moded;
						mmreg3 = mmreg1;
					}
					else if( mmreg3 < 0 ) mmreg3 = _allocGPRtoXMMreg(-1, _Rd_, moded);

					info |= PROCESS_EE_SET_D(mmreg3);
				}

				if( xmminfo & (XMMINFO_READLO|XMMINFO_WRITELO) ) {
					mmtemp = eeProcessHILO(XMMGPR_LO, ((xmminfo&XMMINFO_READLO)?MODE_READ:0)|((xmminfo&XMMINFO_WRITELO)?MODE_WRITE:0), 0);
					if( mmtemp >= 0 ) info |= PROCESS_EE_SET_LO(mmtemp);
				}
				if( xmminfo & (XMMINFO_READHI|XMMINFO_WRITEHI) ) {
					mmtemp = eeProcessHILO(XMMGPR_HI, ((xmminfo&XMMINFO_READLO)?MODE_READ:0)|((xmminfo&XMMINFO_WRITELO)?MODE_WRITE:0), 0);
					if( mmtemp >= 0 ) info |= PROCESS_EE_SET_HI(mmtemp);
				}

				if( creg == _Rs_ ) constscode(info|PROCESS_EE_SET_T(mmreg1));
				else consttcode(info|PROCESS_EE_SET_S(mmreg1));
				_clearNeededXMMregs();
				if( xmminfo & XMMINFO_WRITED ) GPR_DEL_CONST(_Rd_);
				return;
			}
		}
		else {
			// no const regs
			mmreg1 = _allocCheckGPRtoXMM(g_pCurInstInfo, _Rs_, MODE_READ);
			mmreg2 = _allocCheckGPRtoXMM(g_pCurInstInfo, _Rt_, MODE_READ);

			if( mmreg1 >= 0 || mmreg2 >= 0 ) {
				int info = PROCESS_EE_XMM;

				// do it all in xmm
				if( mmreg1 < 0 ) mmreg1 = _allocGPRtoXMMreg(-1, _Rs_, MODE_READ);
				if( mmreg2 < 0 ) mmreg2 = _allocGPRtoXMMreg(-1, _Rt_, MODE_READ);

				info |= PROCESS_EE_SETMODES(mmreg1)|PROCESS_EE_SETMODET(mmreg2);

				if( xmminfo & XMMINFO_WRITED ) {
					// check for last used, if so don't alloc a new XMM reg
					_addNeededGPRtoXMMreg(_Rd_);
					mmreg3 = _checkXMMreg(XMMTYPE_GPRREG, _Rd_, moded);

					if( mmreg3 < 0 ) {
						if( !(xmminfo&XMMINFO_READD) && ((g_pCurInstInfo->regs[_Rt_] & EEINST_LASTUSE) || !EEINST_ISLIVEXMM(_Rt_)) ) {
							_freeXMMreg(mmreg2);
							info &= ~PROCESS_EE_MODEWRITET;
							_deleteMMXreg(MMX_GPR+_Rd_, 2);
							xmmregs[mmreg2].inuse = 1;
							xmmregs[mmreg2].reg = _Rd_;
							xmmregs[mmreg2].mode = moded;
							mmreg3 = mmreg2;
						}
						else if( !(xmminfo&XMMINFO_READD) && ((g_pCurInstInfo->regs[_Rs_] & EEINST_LASTUSE) || !EEINST_ISLIVEXMM(_Rs_)) ) {
							_freeXMMreg(mmreg1);
							info &= ~PROCESS_EE_MODEWRITES;
							_deleteMMXreg(MMX_GPR+_Rd_, 2);
							xmmregs[mmreg1].inuse = 1;
							xmmregs[mmreg1].reg = _Rd_;
							xmmregs[mmreg1].mode = moded;
							mmreg3 = mmreg1;
						}
						else mmreg3 = _allocGPRtoXMMreg(-1, _Rd_, moded);
					}

					info |= PROCESS_EE_SET_D(mmreg3);
				}

				if( xmminfo & (XMMINFO_READLO|XMMINFO_WRITELO) ) {
					mmtemp = eeProcessHILO(XMMGPR_LO, ((xmminfo&XMMINFO_READLO)?MODE_READ:0)|((xmminfo&XMMINFO_WRITELO)?MODE_WRITE:0), 0);
					if( mmtemp >= 0 ) info |= PROCESS_EE_SET_LO(mmtemp);
				}
				if( xmminfo & (XMMINFO_READHI|XMMINFO_WRITEHI) ) {
					mmtemp = eeProcessHILO(XMMGPR_HI, ((xmminfo&XMMINFO_READLO)?MODE_READ:0)|((xmminfo&XMMINFO_WRITELO)?MODE_WRITE:0), 0);
					if( mmtemp >= 0 ) info |= PROCESS_EE_SET_HI(mmtemp);
				}

				noconstcode(info|PROCESS_EE_SET_S(mmreg1)|PROCESS_EE_SET_T(mmreg2));
				_clearNeededXMMregs();
				if( xmminfo & XMMINFO_WRITED ) GPR_DEL_CONST(_Rd_);
				return;
			}
		}

		_clearNeededXMMregs();
	}

	// regular x86
	_deleteGPRtoXMMreg(_Rs_, 1);
	_deleteGPRtoXMMreg(_Rt_, 1);
	if( xmminfo&XMMINFO_WRITED )
		_deleteGPRtoXMMreg(_Rd_, (xmminfo&XMMINFO_READD)?0:2);
	_deleteMMXreg(MMX_GPR+_Rs_, 1);
	_deleteMMXreg(MMX_GPR+_Rt_, 1);
	if( xmminfo&XMMINFO_WRITED )
		_deleteMMXreg(MMX_GPR+_Rd_, (xmminfo&XMMINFO_READD)?0:2);

	// don't delete, fn will take care of them
//	if( xmminfo & (XMMINFO_READLO|XMMINFO_WRITELO) ) {
//		_deleteGPRtoXMMreg(XMMGPR_LO, (xmminfo&XMMINFO_READLO)?1:0);
//		_deleteMMXreg(MMX_GPR+MMX_LO, (xmminfo&XMMINFO_READLO)?1:0);
//	}
//	if( xmminfo & (XMMINFO_READHI|XMMINFO_WRITEHI) ) {
//		_deleteGPRtoXMMreg(XMMGPR_HI, (xmminfo&XMMINFO_READHI)?1:0);
//		_deleteMMXreg(MMX_GPR+MMX_HI, (xmminfo&XMMINFO_READHI)?1:0);
//	}

	if( GPR_IS_CONST1(_Rs_) ) {
		constscode(0);
		if( xmminfo&XMMINFO_WRITED ) GPR_DEL_CONST(_Rd_);
		return;
	}

	if( GPR_IS_CONST1(_Rt_) ) {
		consttcode(0);
		if( xmminfo&XMMINFO_WRITED ) GPR_DEL_CONST(_Rd_);
		return;
	}

	noconstcode(0);
	if( xmminfo&XMMINFO_WRITED ) GPR_DEL_CONST(_Rd_);
}

// rt = rs op imm16
void eeRecompileCode1(R5900FNPTR constcode, R5900FNPTR_INFO noconstcode)
{
	int mmreg1, mmreg2;
	if ( ! _Rt_ ) return;

	CHECK_SAVE_REG(_Rt_);
	_eeProcessHasLive(_Rt_, 0);
	EEINST_RESETSIGNEXT(_Rt_);

	if( GPR_IS_CONST1(_Rs_) ) {
		_deleteMMXreg(MMX_GPR+_Rt_, 2);
		_deleteGPRtoXMMreg(_Rt_, 2);
		GPR_SET_CONST(_Rt_);
		constcode();
		return;
	}

	// test if should write mmx
	if( g_pCurInstInfo->info & EEINST_MMX ) {

		// no const regs
		mmreg1 = _allocCheckGPRtoMMX(g_pCurInstInfo, _Rs_, MODE_READ);

		if( mmreg1 >= 0 ) {
			int info = PROCESS_EE_MMX|PROCESS_EE_SETMODES(mmreg1);

			// check for last used, if so don't alloc a new MMX reg
			_addNeededMMXreg(MMX_GPR+_Rt_);
			mmreg2 = _checkMMXreg(MMX_GPR+_Rt_, MODE_WRITE);

			if( mmreg2 < 0 ) {
				if( (g_pCurInstInfo->regs[_Rs_] & EEINST_LASTUSE) || !EEINST_ISLIVE64(_Rs_) ) {
					if( EEINST_ISLIVE64(_Rs_) ) {
						_freeMMXreg(mmreg1);
						info &= ~PROCESS_EE_MODEWRITES;
					}
					_deleteGPRtoXMMreg(_Rt_, 2);
					mmxregs[mmreg1].inuse = 1;
					mmxregs[mmreg1].reg = _Rt_;
					mmxregs[mmreg1].mode = MODE_WRITE|MODE_READ;
					mmreg2 = mmreg1;
				}
				else mmreg2 = _allocMMXreg(-1, MMX_GPR+_Rt_, MODE_WRITE);
			}

			SetMMXstate();
			noconstcode(info|PROCESS_EE_SET_S(mmreg1)|PROCESS_EE_SET_T(mmreg2));
			_clearNeededMMXregs();
			GPR_DEL_CONST(_Rt_);
			return;
		}

		_clearNeededMMXregs();
	}

	// test if should write xmm, mirror to mmx code
	if( g_pCurInstInfo->info & EEINST_XMM ) {

		// no const regs
		mmreg1 = _allocCheckGPRtoXMM(g_pCurInstInfo, _Rs_, MODE_READ);

		if( mmreg1 >= 0 ) {
			int info = PROCESS_EE_XMM|PROCESS_EE_SETMODES(mmreg1);

			// check for last used, if so don't alloc a new XMM reg
			_addNeededGPRtoXMMreg(_Rt_);
			mmreg2 = _checkXMMreg(XMMTYPE_GPRREG, _Rt_, MODE_WRITE);

			if( mmreg2 < 0 ) {
				if( (g_pCurInstInfo->regs[_Rs_] & EEINST_LASTUSE) || !EEINST_ISLIVEXMM(_Rs_) ) {
					_freeXMMreg(mmreg1);
					info &= ~PROCESS_EE_MODEWRITES;
					_deleteMMXreg(MMX_GPR+_Rt_, 2);
					xmmregs[mmreg1].inuse = 1;
					xmmregs[mmreg1].reg = _Rt_;
					xmmregs[mmreg1].mode = MODE_WRITE|MODE_READ;
					mmreg2 = mmreg1;
				}
				else mmreg2 = _allocGPRtoXMMreg(-1, _Rt_, MODE_WRITE);
			}

			noconstcode(info|PROCESS_EE_SET_S(mmreg1)|PROCESS_EE_SET_T(mmreg2));
			_clearNeededXMMregs();
			GPR_DEL_CONST(_Rt_);
			return;
		}

		_clearNeededXMMregs();
	}

	// regular x86
	_deleteGPRtoXMMreg(_Rs_, 1);
	_deleteGPRtoXMMreg(_Rt_, 2);
	_deleteMMXreg(MMX_GPR+_Rs_, 1);
	_deleteMMXreg(MMX_GPR+_Rt_, 2);

	noconstcode(0);
	GPR_DEL_CONST(_Rt_);
}

// rd = rt op sa
void eeRecompileCode2(R5900FNPTR constcode, R5900FNPTR_INFO noconstcode)
{
	int mmreg1, mmreg2;
	if ( ! _Rd_ ) return;

	CHECK_SAVE_REG(_Rd_);
	_eeProcessHasLive(_Rd_, 0);
	EEINST_RESETSIGNEXT(_Rd_);

	if( GPR_IS_CONST1(_Rt_) ) {
		_deleteMMXreg(MMX_GPR+_Rd_, 2);
		_deleteGPRtoXMMreg(_Rd_, 2);
		GPR_SET_CONST(_Rd_);
		constcode();
		return;
	}

	// test if should write mmx
	if( g_pCurInstInfo->info & EEINST_MMX ) {

		// no const regs
		mmreg1 = _allocCheckGPRtoMMX(g_pCurInstInfo, _Rt_, MODE_READ);

		if( mmreg1 >= 0 ) {
			int info = PROCESS_EE_MMX|PROCESS_EE_SETMODET(mmreg1);

			// check for last used, if so don't alloc a new MMX reg
			_addNeededMMXreg(MMX_GPR+_Rd_);
			mmreg2 = _checkMMXreg(MMX_GPR+_Rd_, MODE_WRITE);

			if( mmreg2 < 0 ) {
				if( (g_pCurInstInfo->regs[_Rt_] & EEINST_LASTUSE) || !EEINST_ISLIVE64(_Rt_) ) {
					if( EEINST_ISLIVE64(_Rt_) ) {
						_freeMMXreg(mmreg1);
						info &= ~PROCESS_EE_MODEWRITET;
					}
					_deleteGPRtoXMMreg(_Rd_, 2);
					mmxregs[mmreg1].inuse = 1;
					mmxregs[mmreg1].reg = _Rd_;
					mmxregs[mmreg1].mode = MODE_WRITE|MODE_READ;
					mmreg2 = mmreg1;
				}
				else mmreg2 = _allocMMXreg(-1, MMX_GPR+_Rd_, MODE_WRITE);
			}

			SetMMXstate();
			noconstcode(info|PROCESS_EE_SET_T(mmreg1)|PROCESS_EE_SET_D(mmreg2));
			_clearNeededMMXregs();
			GPR_DEL_CONST(_Rd_);
			return;
		}

		_clearNeededMMXregs();
	}

	// test if should write xmm, mirror to mmx code
	if( g_pCurInstInfo->info & EEINST_XMM ) {

		// no const regs
		mmreg1 = _allocCheckGPRtoXMM(g_pCurInstInfo, _Rt_, MODE_READ);

		if( mmreg1 >= 0 ) {
			int info = PROCESS_EE_XMM|PROCESS_EE_SETMODET(mmreg1);

			// check for last used, if so don't alloc a new XMM reg
			_addNeededGPRtoXMMreg(_Rd_);
			mmreg2 = _checkXMMreg(XMMTYPE_GPRREG, _Rd_, MODE_WRITE);

			if( mmreg2 < 0 ) {
				if( (g_pCurInstInfo->regs[_Rt_] & EEINST_LASTUSE) || !EEINST_ISLIVE64(_Rt_) ) {
					_freeXMMreg(mmreg1);
					info &= ~PROCESS_EE_MODEWRITET;
					_deleteMMXreg(MMX_GPR+_Rd_, 2);
					xmmregs[mmreg1].inuse = 1;
					xmmregs[mmreg1].reg = _Rd_;
					xmmregs[mmreg1].mode = MODE_WRITE|MODE_READ;
					mmreg2 = mmreg1;
				}
				else mmreg2 = _allocGPRtoXMMreg(-1, _Rd_, MODE_WRITE);
			}

			noconstcode(info|PROCESS_EE_SET_T(mmreg1)|PROCESS_EE_SET_D(mmreg2));
			_clearNeededXMMregs();
			GPR_DEL_CONST(_Rd_);
			return;
		}

		_clearNeededXMMregs();
	}

	// regular x86
	_deleteGPRtoXMMreg(_Rt_, 1);
	_deleteGPRtoXMMreg(_Rd_, 2);
	_deleteMMXreg(MMX_GPR+_Rt_, 1);
	_deleteMMXreg(MMX_GPR+_Rd_, 2);

	noconstcode(0);
	GPR_DEL_CONST(_Rd_);
}

// rt op rs 
void eeRecompileCode3(R5900FNPTR constcode, R5900FNPTR_INFO multicode)
{
	assert(0);
	// for now, don't support xmm
	_deleteEEreg(_Rs_, 1);
	_deleteEEreg(_Rt_, 1);

	if( GPR_IS_CONST2(_Rs_, _Rt_) ) {
		constcode();
		return;
	}

	if( GPR_IS_CONST1(_Rs_) ) {
		//multicode(PROCESS_EE_CONSTT);
		return;
	}

	if( GPR_IS_CONST1(_Rt_) ) {
		//multicode(PROCESS_EE_CONSTT);
		return;
	}

	multicode(0);
}

// Simple Code Templates //

// rd = rs op rt
void eeRecompileCodeConst0(R5900FNPTR constcode, R5900FNPTR_INFO constscode, R5900FNPTR_INFO consttcode, R5900FNPTR_INFO noconstcode)
{
	if ( ! _Rd_ ) return;

	// for now, don't support xmm
	CHECK_SAVE_REG(_Rd_);

	_deleteGPRtoXMMreg(_Rs_, 1);
	_deleteGPRtoXMMreg(_Rt_, 1);
	_deleteGPRtoXMMreg(_Rd_, 0);
	_deleteMMXreg(MMX_GPR+_Rs_, 1);
	_deleteMMXreg(MMX_GPR+_Rt_, 1);
	_deleteMMXreg(MMX_GPR+_Rd_, 0);

	if( GPR_IS_CONST2(_Rs_, _Rt_) ) {
		GPR_SET_CONST(_Rd_);
		constcode();
		return;
	}

	if( GPR_IS_CONST1(_Rs_) ) {
		constscode(0);
		GPR_DEL_CONST(_Rd_);
		return;
	}

	if( GPR_IS_CONST1(_Rt_) ) {
		consttcode(0);
		GPR_DEL_CONST(_Rd_);
		return;
	}

	noconstcode(0);
	GPR_DEL_CONST(_Rd_);
}

// rt = rs op imm16
void eeRecompileCodeConst1(R5900FNPTR constcode, R5900FNPTR_INFO noconstcode)
{
    if ( ! _Rt_ )
        return;

	// for now, don't support xmm
	CHECK_SAVE_REG(_Rt_);

	_deleteGPRtoXMMreg(_Rs_, 1);
	_deleteGPRtoXMMreg(_Rt_, 0);

	if( GPR_IS_CONST1(_Rs_) ) {
		GPR_SET_CONST(_Rt_);
		constcode();
		return;
	}

	noconstcode(0);
	GPR_DEL_CONST(_Rt_);
}

// rd = rt op sa
void eeRecompileCodeConst2(R5900FNPTR constcode, R5900FNPTR_INFO noconstcode)
{
	if ( ! _Rd_ ) return;

	// for now, don't support xmm
	CHECK_SAVE_REG(_Rd_);

	_deleteGPRtoXMMreg(_Rt_, 1);
	_deleteGPRtoXMMreg(_Rd_, 0);

	if( GPR_IS_CONST1(_Rt_) ) {
		GPR_SET_CONST(_Rd_);
		constcode();
		return;
	}

	noconstcode(0);
	GPR_DEL_CONST(_Rd_);
}

// rd = rt MULT rs  (SPECIAL)
void eeRecompileCodeConstSPECIAL(R5900FNPTR constcode, R5900FNPTR_INFO multicode, int MULT)
{
	assert(0);
	// for now, don't support xmm
	if( MULT ) {
		CHECK_SAVE_REG(_Rd_);
		_deleteGPRtoXMMreg(_Rd_, 0);
	}

	_deleteGPRtoXMMreg(_Rs_, 1);
	_deleteGPRtoXMMreg(_Rt_, 1);

	if( GPR_IS_CONST2(_Rs_, _Rt_) ) {
		if( MULT && _Rd_ ) GPR_SET_CONST(_Rd_);
		constcode();
		return;
	}

	if( GPR_IS_CONST1(_Rs_) ) {
		//multicode(PROCESS_EE_CONSTS);
		if( MULT && _Rd_ ) GPR_DEL_CONST(_Rd_);
		return;
	}

	if( GPR_IS_CONST1(_Rt_) ) {
		//multicode(PROCESS_EE_CONSTT);
		if( MULT && _Rd_ ) GPR_DEL_CONST(_Rd_);
		return;
	}

	multicode(0);
	if( MULT && _Rd_ ) GPR_DEL_CONST(_Rd_);
}

// EE XMM allocation code
int eeRecompileCodeXMM(int xmminfo)
{
	int info = PROCESS_EE_XMM;

	// save state
	if( xmminfo & XMMINFO_WRITED ) {
		CHECK_SAVE_REG(_Rd_);
		_eeProcessHasLive(_Rd_, 0);
		EEINST_RESETSIGNEXT(_Rd_);
	}

	// flush consts
	if( xmminfo & XMMINFO_READT ) {
		if( GPR_IS_CONST1( _Rt_ ) && !(g_cpuFlushedConstReg&(1<<_Rt_)) ) {
			MOV32ItoM((int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ], g_cpuConstRegs[_Rt_].UL[0]);
			MOV32ItoM((int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 1 ], g_cpuConstRegs[_Rt_].UL[1]);
			g_cpuFlushedConstReg |= (1<<_Rt_);
		}
	}
	if( xmminfo & XMMINFO_READS) {
		if( GPR_IS_CONST1( _Rs_ ) && !(g_cpuFlushedConstReg&(1<<_Rs_)) ) {
			MOV32ItoM((int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ], g_cpuConstRegs[_Rs_].UL[0]);
			MOV32ItoM((int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 1 ], g_cpuConstRegs[_Rs_].UL[1]);
			g_cpuFlushedConstReg |= (1<<_Rs_);
		}
	}

	if( xmminfo & XMMINFO_WRITED ) {
		GPR_DEL_CONST(_Rd_);
	}

	// add needed
	if( xmminfo & (XMMINFO_READLO|XMMINFO_WRITELO) ) {
		_addNeededGPRtoXMMreg(XMMGPR_LO);
	}
	if( xmminfo & (XMMINFO_READHI|XMMINFO_WRITEHI) ) {
		_addNeededGPRtoXMMreg(XMMGPR_HI);
	}
	if( xmminfo & XMMINFO_READS) _addNeededGPRtoXMMreg(_Rs_);
	if( xmminfo & XMMINFO_READT) _addNeededGPRtoXMMreg(_Rt_);
	if( xmminfo & XMMINFO_WRITED ) _addNeededGPRtoXMMreg(_Rd_);

	// allocate
	if( xmminfo & XMMINFO_READS) {
		int reg = _allocGPRtoXMMreg(-1, _Rs_, MODE_READ);
		info |= PROCESS_EE_SET_S(reg)|PROCESS_EE_SETMODES(reg);
	}
	if( xmminfo & XMMINFO_READT) {
		int reg = _allocGPRtoXMMreg(-1, _Rt_, MODE_READ);
		info |= PROCESS_EE_SET_T(reg)|PROCESS_EE_SETMODET(reg);
	}

	if( xmminfo & XMMINFO_WRITED ) {
		int readd = MODE_WRITE|((xmminfo&XMMINFO_READD)?((xmminfo&XMMINFO_READD_LO)?(MODE_READ|MODE_READHALF):MODE_READ):0);

		int regd = _checkXMMreg(XMMTYPE_GPRREG, _Rd_, readd);

		if( regd < 0 ) {
			if( !(xmminfo&XMMINFO_READD) && (xmminfo & XMMINFO_READT) && (_Rt_ == 0 || (g_pCurInstInfo->regs[_Rt_] & EEINST_LASTUSE) || !EEINST_ISLIVEXMM(_Rt_)) ) {
				_freeXMMreg(EEREC_T);
				_deleteMMXreg(MMX_GPR+_Rd_, 2);
				xmmregs[EEREC_T].inuse = 1;
				xmmregs[EEREC_T].reg = _Rd_;
				xmmregs[EEREC_T].mode = readd;
				regd = EEREC_T;
			}
			else if( !(xmminfo&XMMINFO_READD) && (xmminfo & XMMINFO_READS) && (_Rs_ == 0 || (g_pCurInstInfo->regs[_Rs_] & EEINST_LASTUSE) || !EEINST_ISLIVEXMM(_Rs_)) ) {
				_freeXMMreg(EEREC_S);
				_deleteMMXreg(MMX_GPR+_Rd_, 2);
				xmmregs[EEREC_S].inuse = 1;
				xmmregs[EEREC_S].reg = _Rd_;
				xmmregs[EEREC_S].mode = readd;
				regd = EEREC_S;
			}
			else regd = _allocGPRtoXMMreg(-1, _Rd_, readd);
		}

		info |= PROCESS_EE_SET_D(regd);
	}
	if( xmminfo & (XMMINFO_READLO|XMMINFO_WRITELO) ) {
		info |= PROCESS_EE_SET_LO(_allocGPRtoXMMreg(-1, XMMGPR_LO, ((xmminfo&XMMINFO_READLO)?MODE_READ:0)|((xmminfo&XMMINFO_WRITELO)?MODE_WRITE:0)));
		info |= PROCESS_EE_LO;
	}
	if( xmminfo & (XMMINFO_READHI|XMMINFO_WRITEHI) ) {
		info |= PROCESS_EE_SET_HI(_allocGPRtoXMMreg(-1, XMMGPR_HI, ((xmminfo&XMMINFO_READHI)?MODE_READ:0)|((xmminfo&XMMINFO_WRITEHI)?MODE_WRITE:0)));
		info |= PROCESS_EE_HI;
	}
	return info;
}

// EE COP1(FPU) XMM allocation code
#define _Ft_ _Rt_
#define _Fs_ _Rd_
#define _Fd_ _Sa_

#define PROCESS_EE_SETMODES_XMM(mmreg) ((xmmregs[mmreg].mode&MODE_WRITE)?PROCESS_EE_MODEWRITES:0)
#define PROCESS_EE_SETMODET_XMM(mmreg) ((xmmregs[mmreg].mode&MODE_WRITE)?PROCESS_EE_MODEWRITET:0)

// rd = rs op rt
void eeFPURecompileCode(R5900FNPTR_INFO xmmcode, R5900FNPTR fpucode, int xmminfo)
{
	int mmregs=-1, mmregt=-1, mmregd=-1, mmregacc=-1;
	int info = PROCESS_EE_XMM;

	if( xmminfo & XMMINFO_READS ) _addNeededFPtoXMMreg(_Fs_);
	if( xmminfo & XMMINFO_READT ) _addNeededFPtoXMMreg(_Ft_);
	if( xmminfo & (XMMINFO_WRITED|XMMINFO_READD) ) _addNeededFPtoXMMreg(_Fd_);
	if( xmminfo & (XMMINFO_WRITEACC|XMMINFO_READACC) ) _addNeededFPACCtoXMMreg();

	if( xmminfo & XMMINFO_READT ) {
		if( g_pCurInstInfo->fpuregs[_Ft_] & EEINST_LASTUSE ) mmregt = _checkXMMreg(XMMTYPE_FPREG, _Ft_, MODE_READ);
		else mmregt = _allocFPtoXMMreg(-1, _Ft_, MODE_READ);
	}

	if( xmminfo & XMMINFO_READS ) {
		if( ( !(xmminfo & XMMINFO_READT) || (mmregt >= 0) ) && (g_pCurInstInfo->fpuregs[_Fs_] & EEINST_LASTUSE) ) {
			mmregs = _checkXMMreg(XMMTYPE_FPREG, _Fs_, MODE_READ);
		}
		else mmregs = _allocFPtoXMMreg(-1, _Fs_, MODE_READ);
	}

	if( mmregs >= 0 ) info |= PROCESS_EE_SETMODES_XMM(mmregs);
	if( mmregt >= 0 ) info |= PROCESS_EE_SETMODET_XMM(mmregt);

	if( xmminfo & XMMINFO_READD ) {
		assert( xmminfo & XMMINFO_WRITED );
		mmregd = _allocFPtoXMMreg(-1, _Fd_, MODE_READ);
	}

	if( xmminfo & XMMINFO_READACC ) {
		if( !(xmminfo&XMMINFO_WRITEACC) && (g_pCurInstInfo->fpuregs[_Ft_] & EEINST_LASTUSE) )
			mmregacc = _checkXMMreg(XMMTYPE_FPACC, 0, MODE_READ);
		else mmregacc = _allocFPACCtoXMMreg(-1, MODE_READ);
	}

	if( xmminfo & XMMINFO_WRITEACC ) {
			
		// check for last used, if so don't alloc a new XMM reg
		int readacc = MODE_WRITE|((xmminfo&XMMINFO_READACC)?MODE_READ:0);

		mmregacc = _checkXMMreg(XMMTYPE_FPACC, 0, readacc);

		if( mmregacc < 0 ) {
			if( (xmminfo&XMMINFO_READT) && mmregt >= 0 && (FPUINST_LASTUSE(_Ft_) || !FPUINST_ISLIVE(_Ft_)) ) {
				if( FPUINST_ISLIVE(_Ft_) ) {
					_freeXMMreg(mmregt);
					info &= ~PROCESS_EE_MODEWRITET;
				}
				_deleteMMXreg(MMX_FPU+XMMFPU_ACC, 2);
				xmmregs[mmregt].inuse = 1;
				xmmregs[mmregt].reg = 0;
				xmmregs[mmregt].mode = readacc;
				xmmregs[mmregt].type = XMMTYPE_FPACC;
				mmregacc = mmregt;
			}
			else if( (xmminfo&XMMINFO_READS) && mmregs >= 0 && (FPUINST_LASTUSE(_Fs_) || !FPUINST_ISLIVE(_Fs_)) ) {
				if( FPUINST_ISLIVE(_Fs_) ) {
					_freeXMMreg(mmregs);
					info &= ~PROCESS_EE_MODEWRITES;
				}
				_deleteMMXreg(MMX_FPU+XMMFPU_ACC, 2);
				xmmregs[mmregs].inuse = 1;
				xmmregs[mmregs].reg = 0;
				xmmregs[mmregs].mode = readacc;
				xmmregs[mmregs].type = XMMTYPE_FPACC;
				mmregacc = mmregs;
			}
			else mmregacc = _allocFPACCtoXMMreg(-1, readacc);
		}

		xmmregs[mmregacc].mode |= MODE_WRITE;
	}
	else if( xmminfo & XMMINFO_WRITED ) {
		// check for last used, if so don't alloc a new XMM reg
		int readd = MODE_WRITE|((xmminfo&XMMINFO_READD)?MODE_READ:0);
		if( xmminfo&XMMINFO_READD ) mmregd = _allocFPtoXMMreg(-1, _Fd_, readd);
		else mmregd = _checkXMMreg(XMMTYPE_FPREG, _Fd_, readd);

		if( mmregd < 0 ) {
			if( (xmminfo&XMMINFO_READT) && mmregt >= 0 && (FPUINST_LASTUSE(_Ft_) || !FPUINST_ISLIVE(_Ft_)) ) {
				if( FPUINST_ISLIVE(_Ft_) ) {
					_freeXMMreg(mmregt);
					info &= ~PROCESS_EE_MODEWRITET;
				}
				_deleteMMXreg(MMX_FPU+_Fd_, 2);
				xmmregs[mmregt].inuse = 1;
				xmmregs[mmregt].reg = _Fd_;
				xmmregs[mmregt].mode = readd;
				mmregd = mmregt;
			}
			else if( (xmminfo&XMMINFO_READS) && mmregs >= 0 && (FPUINST_LASTUSE(_Fs_) || !FPUINST_ISLIVE(_Fs_)) ) {
				if( FPUINST_ISLIVE(_Fs_) ) {
					_freeXMMreg(mmregs);
					info &= ~PROCESS_EE_MODEWRITES;
				}
				_deleteMMXreg(MMX_FPU+_Fd_, 2);
				xmmregs[mmregs].inuse = 1;
				xmmregs[mmregs].reg = _Fd_;
				xmmregs[mmregs].mode = readd;
				mmregd = mmregs;
			}
			else if( (xmminfo&XMMINFO_READACC) && mmregacc >= 0 && (FPUINST_LASTUSE(XMMFPU_ACC) || !FPUINST_ISLIVE(XMMFPU_ACC)) ) {
				if( FPUINST_ISLIVE(XMMFPU_ACC) )
					_freeXMMreg(mmregacc);
				_deleteMMXreg(MMX_FPU+_Fd_, 2);
				xmmregs[mmregacc].inuse = 1;
				xmmregs[mmregacc].reg = _Fd_;
				xmmregs[mmregacc].mode = readd;
				xmmregs[mmregacc].type = XMMTYPE_FPREG;
				mmregd = mmregacc;
			}
			else mmregd = _allocFPtoXMMreg(-1, _Fd_, readd);
		}
	}

	assert( mmregs >= 0 || mmregt >= 0 || mmregd >= 0 || mmregacc >= 0 );

	if( xmminfo & XMMINFO_WRITED ) {
		assert( mmregd >= 0 );
		info |= PROCESS_EE_SET_D(mmregd);
	}
	if( xmminfo & (XMMINFO_WRITEACC|XMMINFO_READACC) ) {
		if( mmregacc >= 0 ) info |= PROCESS_EE_SET_ACC(mmregacc)|PROCESS_EE_ACC;
		else assert( !(xmminfo&XMMINFO_WRITEACC));		
	}

	if( xmminfo & XMMINFO_READS ) {
		if( mmregs >= 0 ) info |= PROCESS_EE_SET_S(mmregs)|PROCESS_EE_S;
	}
	if( xmminfo & XMMINFO_READT ) {
		if( mmregt >= 0 ) info |= PROCESS_EE_SET_T(mmregt)|PROCESS_EE_T;
	}
		
	// at least one must be in xmm
	if( (xmminfo & (XMMINFO_READS|XMMINFO_READT)) == (XMMINFO_READS|XMMINFO_READT) ) {
		assert( mmregs >= 0 || mmregt >= 0 );
	}

	xmmcode(info);
	_clearNeededXMMregs();
}
