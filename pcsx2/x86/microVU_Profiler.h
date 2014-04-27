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

enum microOpcode {
	// Upper Instructions
	opABS, opCLIP, opOPMULA, opOPMSUB, opNOP, 
	opADD, opADDi, opADDq, opADDx, opADDy, opADDz, opADDw, 
	opADDA, opADDAi, opADDAq, opADDAx, opADDAy, opADDAz, opADDAw, 
	opSUB, opSUBi, opSUBq, opSUBx, opSUBy, opSUBz, opSUBw,
	opSUBA, opSUBAi, opSUBAq, opSUBAx, opSUBAy, opSUBAz, opSUBAw, 
	opMUL, opMULi, opMULq, opMULx, opMULy, opMULz, opMULw, 
	opMULA, opMULAi, opMULAq, opMULAx, opMULAy, opMULAz, opMULAw, 
	opMADD, opMADDi, opMADDq, opMADDx, opMADDy, opMADDz, opMADDw, 
	opMADDA, opMADDAi, opMADDAq, opMADDAx, opMADDAy, opMADDAz, opMADDAw, 
	opMSUB, opMSUBi, opMSUBq, opMSUBx, opMSUBy, opMSUBz, opMSUBw, 
	opMSUBA, opMSUBAi, opMSUBAq, opMSUBAx, opMSUBAy, opMSUBAz, opMSUBAw, 
	opMAX, opMAXi, opMAXx, opMAXy, opMAXz, opMAXw, 
	opMINI, opMINIi, opMINIx, opMINIy, opMINIz, opMINIw, 
	opFTOI0, opFTOI4, opFTOI12, opFTOI15,
	opITOF0, opITOF4, opITOF12, opITOF15,
	// Lower Instructions
	opDIV, opSQRT, opRSQRT, 
	opIADD, opIADDI, opIADDIU, 
	opIAND, opIOR, 
	opISUB, opISUBIU, 
	opMOVE, opMFIR, opMTIR, opMR32, opMFP,
	opLQ, opLQD, opLQI, 
	opSQ, opSQD, opSQI, 
	opILW, opISW, opILWR, opISWR, 
	opRINIT, opRGET, opRNEXT, opRXOR, 
	opWAITQ, opWAITP,
	opFSAND, opFSEQ, opFSOR, opFSSET,
	opFMAND, opFMEQ, opFMOR, 
	opFCAND, opFCEQ, opFCOR, opFCSET, opFCGET, 
	opIBEQ, opIBGEZ, opIBGTZ, opIBLTZ, opIBLEZ, opIBNE, 
	opB, opBAL, opJR, opJALR, 
	opESADD, opERSADD, opELENG, opERLENG, 
	opEATANxy, opEATANxz, opESUM, opERCPR, 
	opESQRT, opERSQRT, opESIN, opEATAN, 
	opEEXP, opXITOP, opXTOP, opXGKICK,
	opLastOpcode
};

static const char microOpcodeName[][16] = {
	// Upper Instructions
	"ABS", "CLIP", "OPMULA", "OPMSUB", "NOP", 
	"ADD", "ADDi", "ADDq", "ADDx", "ADDy", "ADDz", "ADDw", 
	"ADDA", "ADDAi", "ADDAq", "ADDAx", "ADDAy", "ADDAz", "ADDAw", 
	"SUB", "SUBi", "SUBq", "SUBx", "SUBy", "SUBz", "SUBw",
	"SUBA", "SUBAi", "SUBAq", "SUBAx", "SUBAy", "SUBAz", "SUBAw", 
	"MUL", "MULi", "MULq", "MULx", "MULy", "MULz", "MULw", 
	"MULA", "MULAi", "MULAq", "MULAx", "MULAy", "MULAz", "MULAw", 
	"MADD", "MADDi", "MADDq", "MADDx", "MADDy", "MADDz", "MADDw", 
	"MADDA", "MADDAi", "MADDAq", "MADDAx", "MADDAy", "MADDAz", "MADDAw", 
	"MSUB", "MSUBi", "MSUBq", "MSUBx", "MSUBy", "MSUBz", "MSUBw", 
	"MSUBA", "MSUBAi", "MSUBAq", "MSUBAx", "MSUBAy", "MSUBAz", "MSUBAw", 
	"MAX", "MAXi", "MAXx", "MAXy", "MAXz", "MAXw", 
	"MINI", "MINIi", "MINIx", "MINIy", "MINIz", "MINIw", 
	"FTOI0", "FTOI4", "FTOI12", "FTOI15", 
	"ITOF0", "ITOF4", "ITOF12", "ITOF15", 
	// Lower Instructions
	"DIV", "SQRT", "RSQRT", 
	"IADD", "IADDI", "IADDIU", 
	"IAND", "IOR", 
	"ISUB", "ISUBIU", 
	"MOVE", "MFIR", "MTIR", "MR32", "MFP",
	"LQ", "LQD", "LQI", 
	"SQ", "SQD", "SQI", 
	"ILW", "ISW", "ILWR", "ISWR", 
	"RINIT", "RGET", "RNEXT", "RXOR", 
	"WAITQ", "WAITP",
	"FSAND", "FSEQ", "FSOR", "FSSET",
	"FMAND", "FMEQ", "FMOR", 
	"FCAND", "FCEQ", "FCOR", "FCSET", "FCGET", 
	"IBEQ", "IBGEZ", "IBGTZ", "IBLTZ", "IBLEZ", "IBNE", 
	"B", "BAL", "JR", "JALR", 
	"ESADD", "ERSADD", "ELENG", "ERLENG", 
	"EATANxy", "EATANxz", "ESUM", "ERCPR", 
	"ESQRT", "ERSQRT", "ESIN", "EATAN", 
	"EEXP", "XITOP", "XTOP", "XGKICK"
};

#ifdef mVUprofileProg
#include <utility>
#include <string>
#include <algorithm>

struct microProfiler {
	static const u32 progLimit = 10000;
	u64 opStats[opLastOpcode];
	u32 progCount;
	int index;
	void Reset(int _index) { memzero(*this); index = _index; }
	void EmitOp(microOpcode op) {
		xADD(ptr32[&(((u32*)opStats)[op*2+0])], 1);
		xADC(ptr32[&(((u32*)opStats)[op*2+1])], 0);
	}
	void Print() {
		progCount++;
		if ((progCount % progLimit) == 0) {
			u64 total = 0;
			vector< pair<u32, u32> > v;
			for(int i = 0; i < opLastOpcode; i++) {
				total += opStats[i];
				v.push_back(make_pair(opStats[i], i));
			}
			std::sort   (v.begin(), v.end());
			std::reverse(v.begin(), v.end());
			double dTotal = (double)total;
			DevCon.WriteLn("microVU%d Profiler:", index);
			for(u32 i = 0; i < v.size(); i++) {
				u64    count = v[i].first;
				double stat  = (double)count / dTotal * 100.0;
				string str   = microOpcodeName[v[i].second];
				str.resize(8, ' ');
				DevCon.WriteLn("%s - [%3.4f%%][count=%u]",
					str.c_str(), stat, (u32)count);
			}
			DevCon.WriteLn("Total = 0x%x%x\n\n", (u32)(u64)(total>>32),(u32)total);
		}
	}
};
#else
struct microProfiler {
	__fi void Reset(int _index) {}
	__fi void EmitOp(microOpcode op) {}
	__fi void Print() {}
};
#endif
