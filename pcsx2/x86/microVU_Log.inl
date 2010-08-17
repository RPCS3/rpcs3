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

#include "Utilities/AsciiFile.h"

// writes text directly to mVU->logFile, no newlines appended.
_mVUt void __mVULog(const char* fmt, ...) {

	microVU* mVU = mVUx;
	if (!mVU->logFile) return;

	char tmp[2024];
	va_list list;
	va_start(list, fmt);

	// concatenate the log message after the prefix:
	int length = vsprintf(tmp, fmt, list);
	va_end(list);

	mVU->logFile->Write( tmp );
	mVU->logFile->Flush();
}

#define commaIf() { if (bitX[6]) { mVUlog(","); bitX[6] = 0; } }

#include "AppConfig.h"

_mVUt void __mVUdumpProgram(microProgram& prog) {
	microVU* mVU = mVUx;
	bool bitX[7];
	int	delay = 0;
	int bBranch = mVUbranch;
	int bCode	= mVU->code;
	int bPC		= iPC;
	mVUbranch	= 0;

	const wxString logname(wxsFormat(L"microVU%d prog - %02d.html", vuIndex, prog.idx));
	mVU->logFile = new AsciiFile(Path::Combine(g_Conf->Folders.Logs, logname), L"w");

	mVUlog("<html>\n");
	mVUlog("<title>microVU%d MicroProgram Log</title>\n", vuIndex);
	mVUlog("<body bgcolor=\"#000000\" LINK=\"#1111ff\" VLINK=\"#1111ff\">\n");
	mVUlog("<font face=\"Courier New\" color=\"#ffffff\">\n");

	mVUlog("<font size=\"5\" color=\"#7099ff\">");
	mVUlog("*********************\n<br>",		prog.idx);
	mVUlog("* Micro-Program #%02d *\n<br>",		prog.idx);
	mVUlog("*********************\n\n<br><br>",	prog.idx);
	mVUlog("</font>");

	for (u32 i = 0; i < mVU->progSize; i+=2) {

		if (delay)		{ delay--; mVUlog("</font>"); if (!delay) mVUlog("<hr/>"); }
		if (mVUbranch)	{ delay = 1; mVUbranch = 0; }
		mVU->code = prog.data[i+1];

		bitX[0] = 0;
		bitX[1] = 0;
		bitX[2] = 0;
		bitX[3] = 0;
		bitX[4] = 0;
		bitX[5] = 0;
		bitX[6] = 0;

		if (mVU->code & _Ibit_) { bitX[0] = 1; bitX[5] = 1; }
		if (mVU->code & _Ebit_) { bitX[1] = 1; bitX[5] = 1; delay = 2; }
		if (mVU->code & _Mbit_) { bitX[2] = 1; bitX[5] = 1; }
		if (mVU->code & _Dbit_) { bitX[3] = 1; bitX[5] = 1; }
		if (mVU->code & _Tbit_) { bitX[4] = 1; bitX[5] = 1; }

		if (delay == 2) { mVUlog("<font color=\"#FFFF00\">"); }
		if (delay == 1) { mVUlog("<font color=\"#999999\">"); }

		iPC = (i+1);
		mVUlog("<a name=\"addr%04x\">", i*4);
		mVUlog("[%04x] (%08x)</a> ", i*4, mVU->code);
		mVUopU(mVU, 2);

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
		mVU->code = prog.data[i];

		if(bitX[0]) {
			mVUlog("<br>\n<font color=\"#FF7000\">");
			mVUlog("[%04x] (%08x) %f", i*4, mVU->code, *(float*)&mVU->code);
			mVUlog("</font>\n\n<br><br>");
		}
		else {
			mVUlog("<br>\n[%04x] (%08x) ", i*4, mVU->code);
			mVUopL(mVU, 2);
			mVUlog("\n\n<br><br>");
		}
	}
	mVUlog("</font>\n");
	mVUlog("</body>\n");
	mVUlog("</html>\n");

	mVUbranch = bBranch;
	mVU->code = bCode;
	iPC		  = bPC;
	setCode();

	mVU->logFile.Delete();
}

