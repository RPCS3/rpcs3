/*  Pcsx2 - Pc Ps2 Emulator
 *  Copyright (C) 2009  Pcsx2 Team
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

// writes text directly to the microVU.txt, no newlines appended.
microVUx(void) __mVULog(const char* fmt, ...) {
	microVU* mVU = mVUx;
	char tmp[2024];
	va_list list;

	va_start(list, fmt);

	// concatenate the log message after the prefix:
	int length = vsprintf(tmp, fmt, list);
	va_end(list);

	if (mVU->logFile) {
		fputs(tmp, mVU->logFile);
		fflush(mVU->logFile);
	}
}

#define commaIf() { if (bitX[6]) { mVUlog(","); bitX[6] = 0; } }

microVUt(void) __mVUdumpProgram(int progIndex) {
	microVU* mVU = mVUx;
	bool bitX[9];
	char str[30];
	int	delay = 0;
	mVUbranch = 0;

	sprintf(str, "%s\\microVU%d prog - %02d.html", LOGS_DIR, vuIndex, progIndex);
	mVU->logFile = fopen(str, "w");
	
	mVUlog("<html>\n");
	mVUlog("<title>microVU%d MicroProgram Log</title>\n", vuIndex);
	mVUlog("<body bgcolor=\"#000000\" LINK=\"#1111ff\" VLINK=\"#1111ff\">\n");
	mVUlog("<font face=\"Courier New\" color=\"#ffffff\">\n");

	mVUlog("<font size=\"5\" color=\"#7099ff\">");
	mVUlog("*********************\n<br>",		progIndex);
	mVUlog("* Micro-Program #%02d *\n<br>",		progIndex);
	mVUlog("*********************\n\n<br><br>",	progIndex);
	mVUlog("</font>");

	for (u32 i = 0; i < mVU->progSize; i+=2) {

		if (delay)		{ delay--; mVUlog("</font>"); if (!delay) mVUlog("<hr/>"); }
		if (mVUbranch)	{ delay = 1; mVUbranch = 0; }
		mVU->code = mVU->prog.prog[progIndex].data[i+1];

		bitX[0] = 0;
		bitX[1] = 0;
		bitX[2] = 0;
		bitX[3] = 0;
		bitX[4] = 0;
		bitX[5] = 0;
		bitX[6] = 0;
		bitX[7] = 0;
		bitX[8] = 0;

		if (mVU->code & _Ibit_) { bitX[0] = 1; bitX[5] = 1; bitX[7] = 1; }
		if (mVU->code & _Ebit_) { bitX[1] = 1; bitX[5] = 1; delay = 2; }
		if (mVU->code & _Mbit_) { bitX[2] = 1; bitX[5] = 1; }
		if (mVU->code & _Dbit_) { bitX[3] = 1; bitX[5] = 1; }
		if (mVU->code & _Tbit_) { bitX[4] = 1; bitX[5] = 1; }

		if (delay == 2) { mVUlog("<font color=\"#FFFF00\">"); }
		if (delay == 1) { mVUlog("<font color=\"#999999\">"); }

		iPC = (i+1);
		mVUlog("<a name=\"addr%04x\">", i*4);
		mVUlog("[%04x] (%08x)</a> ", i*4, mVU->code);
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

		iPC = i;
		if (bitX[7]) { mVUlog("<font color=\"#0070ff\">"); }
		mVU->code = mVU->prog.prog[progIndex].data[i];
		mVUlog("<br>\n[%04x] (%08x) ", i*4, mVU->code);
		mVUopL<vuIndex, 2>();
		mVUlog("\n\n<br><br>");
		if (bitX[7]) { mVUlog("</font>"); }
	}
	mVUlog("</font>\n");
	mVUlog("</body>\n");
	mVUlog("</html>\n");
	fclose(mVU->logFile);
}

#endif //PCSX2_MICROVU
