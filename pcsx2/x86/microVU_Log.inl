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

microVUt(void) __mVUsetupLog() {
	if (!vuIndex) { if (!mVUlogFile[0]) mVUlogFile[0] = fopen(LOGS_DIR "\\microVU0.txt", "w"); }
	else		  { if (!mVUlogFile[1]) mVUlogFile[1] = fopen(LOGS_DIR "\\microVU1.txt", "w"); }
}

// writes text directly to the microVU.txt, no newlines appended.
microVUx(void) __mVULog(const char* fmt, ...) {
	char tmp[2024];
	va_list list;

	va_start(list, fmt);

	// concatenate the log message after the prefix:
	int length = vsprintf(tmp, fmt, list);
	va_end(list);

	if (mVUlogFile[vuIndex]) {
		fputs(tmp, mVUlogFile[vuIndex]);
		//fputs("\n", mVUlogFile[vuIndex]);
		fflush(mVUlogFile[vuIndex]);
	}
}

#define commaIf() { if (bitX[6]) { mVUlog(","); bitX[6] = 0; } }

microVUt(void) __mVUdumpProgram(int progIndex) {
	microVU* mVU = mVUx;
	bool bitX[7];

	mVUlog("*********************\n",   progIndex);
	mVUlog("* Micro-Program #%02d *\n", progIndex);
	mVUlog("*********************\n\n", progIndex);
	for (u32 i = 0; i < mVU->progSize; i+=2) {

		mVU->code = mVU->prog.prog[progIndex].data[i+1];
		mVUlog("[%04x] (%08x) ", i*4, mVU->code);

		bitX[0] = 0;
		bitX[1] = 0;
		bitX[2] = 0;
		bitX[3] = 0;
		bitX[4] = 0;
		bitX[5] = 0;
		bitX[6] = 0;

		if (mVU->code & _Ibit_) {bitX[0] = 1; bitX[5] = 1;}
		if (mVU->code & _Ebit_) {bitX[1] = 1; bitX[5] = 1;}
		if (mVU->code & _Mbit_) {bitX[2] = 1; bitX[5] = 1;}
		if (mVU->code & _Dbit_) {bitX[3] = 1; bitX[5] = 1;}
		if (mVU->code & _Tbit_) {bitX[4] = 1; bitX[5] = 1;}

		iPC = (i+1)/4;
		mVUopU<vuIndex, 2>();

		if (bitX[5]) { 
			mVUlog(" ("); 
			if (bitX[0]) { mVUlog("I"); bitX[6] = 1; }
			if (bitX[1]) { commaIf(); mVUlog("E"); bitX[6] = 1; }
			if (bitX[2]) { commaIf(); mVUlog("M"); bitX[6] = 1; }
			if (bitX[3]) { commaIf(); mVUlog("D"); bitX[6] = 1; }
			if (bitX[4]) { commaIf(); mVUlog("T"); }
			mVUlog(")");
		}

		iPC = i/4;
		mVU->code = mVU->prog.prog[progIndex].data[i];
		mVUlog("\n[%04x] (%08x) ", i*4, mVU->code);
		mVUopL<vuIndex, 2>();
		mVUlog("\n\n");
	}
}

#endif //PCSX2_MICROVU
