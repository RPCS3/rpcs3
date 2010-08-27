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


int microRegAlloc::findFreeRegRec(int startIdx) {
	for (int i = startIdx; i < xmmTotal; i++) {
		if (!xmmMap[i].isNeeded) {
			int x = findFreeRegRec(i+1);
			if (x == -1) return i;
			return ((xmmMap[i].count < xmmMap[x].count) ? i : x);
		}
	}
	return -1;
}
int microRegAlloc::findFreeReg() {
	for (int i = 0; i < xmmTotal; i++) {
		if (!xmmMap[i].isNeeded && (xmmMap[i].VFreg < 0)) {
			return i; // Reg is not needed and was a temp reg
		}
	}
	int x = findFreeRegRec(0);
	pxAssumeDev( x >= 0, "microVU register allocation failure!" );
	return x;
}

microRegAlloc::microRegAlloc(microVU* _mVU) {
	mVU = _mVU;
}

void microRegAlloc::clearReg(int regId) {
	microMapXMM& clear( xmmMap[regId] );
	clear.VFreg		= -1;
	clear.count		=  0;
	clear.xyzw		=  0;
	clear.isNeeded	=  0;
}
void microRegAlloc::writeBackReg(const xmm& reg, bool invalidateRegs) {
	microMapXMM& write( xmmMap[reg.Id] );

	if ((write.VFreg > 0) && write.xyzw) { // Reg was modified and not Temp or vf0
		if		(write.VFreg == 33) xMOVSS(ptr32[&mVU->getVI(REG_I)], reg);
		else if (write.VFreg == 32) mVUsaveReg(reg, ptr[&mVU->regs().ACC],			write.xyzw, 1);
		else						mVUsaveReg(reg, ptr[&mVU->getVF(write.VFreg)],	write.xyzw, 1);
		if (invalidateRegs) {
			for (int i = 0; i < xmmTotal; i++) {
				microMapXMM& imap (xmmMap[i]);
				if ((i == reg.Id) || imap.isNeeded) continue;
				if (imap.VFreg == write.VFreg) {
					if (imap.xyzw && imap.xyzw < 0xf) DevCon.Error("microVU Error: writeBackReg() [%d]", imap.VFreg);
					clearReg(i); // Invalidate any Cached Regs of same vf Reg
				}
			}
		}
		if (write.xyzw == 0xf) { // Make Cached Reg if All Vectors were Modified
			write.count	 = counter;
			write.xyzw	 = 0;
			write.isNeeded = 0;
			return;
		}
	}
	clearReg(reg); // Clear Reg
}
void microRegAlloc::clearNeeded(const xmm& reg)
{
	if ((reg.Id < 0) || (reg.Id >= xmmTotal)) return;

	microMapXMM& clear (xmmMap[reg.Id]);
	clear.isNeeded = 0;
	if (clear.xyzw) { // Reg was modified
		if (clear.VFreg > 0) {
			int mergeRegs = 0;
			if (clear.xyzw < 0xf) { mergeRegs = 1; } // Try to merge partial writes
			for (int i = 0; i < xmmTotal; i++) { // Invalidate any other read-only regs of same vfReg
				if (i == reg.Id) continue;
				microMapXMM& imap (xmmMap[i]);
				if (imap.VFreg == clear.VFreg) {
					if (imap.xyzw && imap.xyzw < 0xf) DevCon.Error("microVU Error: clearNeeded() [%d]", imap.VFreg);
					if (mergeRegs == 1) {
						mVUmergeRegs(xmm(i), reg, clear.xyzw, 1);
						imap.xyzw = 0xf;
						imap.count = counter;
						mergeRegs = 2;
					}
					else clearReg(i);
				}
			}
			if (mergeRegs == 2) clearReg(reg);	   // Clear Current Reg if Merged
			else if (mergeRegs) writeBackReg(reg); // Write Back Partial Writes if couldn't merge
		}
		else clearReg(reg); // If Reg was temp or vf0, then invalidate itself
	}
}
const xmm& microRegAlloc::allocReg(int vfLoadReg, int vfWriteReg, int xyzw, bool cloneWrite) {
	counter++;
	if (vfLoadReg >= 0) { // Search For Cached Regs
		for (int i = 0; i < xmmTotal; i++) {
			const xmm& xmmi(xmm::GetInstance(i));
			microMapXMM& imap (xmmMap[i]);
			if ((imap.VFreg == vfLoadReg) && (!imap.xyzw // Reg Was Not Modified
			||  (imap.VFreg && (imap.xyzw==0xf)))) {	 // Reg Had All Vectors Modified and != VF0
				int z = i;
				if (vfWriteReg >= 0) { // Reg will be modified
					if (cloneWrite) {  // Clone Reg so as not to use the same Cached Reg
						z = findFreeReg();
						const xmm& xmmz(xmm::GetInstance(z));
						writeBackReg(xmmz);
						if (z!=i && xyzw==8) xMOVAPS (xmmz, xmmi);
						else if (xyzw == 4)  xPSHUF.D(xmmz, xmmi, 1);
						else if (xyzw == 2)  xPSHUF.D(xmmz, xmmi, 2);
						else if (xyzw == 1)  xPSHUF.D(xmmz, xmmi, 3);
						else if (z != i)	 xMOVAPS (xmmz, xmmi);
						imap.count = counter; // Reg i was used, so update counter
					}
					else { // Don't clone reg, but shuffle to adjust for SS ops
						if ((vfLoadReg != vfWriteReg) || (xyzw != 0xf)) { writeBackReg(xmmi); }
						if		(xyzw == 4) xPSHUF.D(xmmi, xmmi, 1);
						else if (xyzw == 2) xPSHUF.D(xmmi, xmmi, 2);
						else if (xyzw == 1) xPSHUF.D(xmmi, xmmi, 3);
					}
					xmmMap[z].VFreg  = vfWriteReg;
					xmmMap[z].xyzw = xyzw;
				}
				xmmMap[z].count	   = counter;
				xmmMap[z].isNeeded = 1;
				return xmm::GetInstance(z);
			}
		}
	}
	int x = findFreeReg();
	const xmm& xmmx = xmm::GetInstance(x);
	writeBackReg(xmmx);

	if (vfWriteReg >= 0) { // Reg Will Be Modified (allow partial reg loading)
		if	   ((vfLoadReg ==  0) && !(xyzw & 1)) { xPXOR(xmmx, xmmx); }
		else if	(vfLoadReg == 33) mVU->loadIreg(xmmx, xyzw);
		else if	(vfLoadReg == 32) mVUloadReg (xmmx, ptr[&mVU->regs().ACC], xyzw);
		else if (vfLoadReg >=  0) mVUloadReg (xmmx, ptr[&mVU->getVF(vfLoadReg)], xyzw);
		xmmMap[x].VFreg  = vfWriteReg;
		xmmMap[x].xyzw = xyzw;
	}
	else { // Reg Will Not Be Modified (always load full reg for caching)
		if		(vfLoadReg == 33) mVU->loadIreg(xmmx, 0xf);
		else if	(vfLoadReg == 32) xMOVAPS(xmmx, ptr128[&mVU->regs().ACC]);
		else if (vfLoadReg >=  0) xMOVAPS(xmmx, ptr128[&mVU->getVF(vfLoadReg)]);
		xmmMap[x].VFreg  = vfLoadReg;
		xmmMap[x].xyzw = 0;
	}
	xmmMap[x].count	   = counter;
	xmmMap[x].isNeeded = 1;
	return xmmx;
}
