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


#include <stdlib.h>
#include <string.h>

#include "PsxCommon.h"
#include "Common.h"

static int branch = 0;
static int branch2 = 0;
static u32 branchPC;

// These macros are used to assemble the repassembler functions

#ifdef _DEBUG
#define execI() { \
	psxRegs.code = PSXMu32(psxRegs.pc); \
 \
 	if (Log) { \
		PSXCPU_LOG("%s\n", disR3000AF(psxRegs.code, psxRegs.pc)); \
	} \
	psxRegs.pc+= 4; \
	psxRegs.cycle++; \
 \
	psxBSC[psxRegs.code >> 26](); \
}
#else
#define execI() { \
	psxRegs.code = PSXMu32(psxRegs.pc); \
 \
 	psxRegs.pc+= 4; \
	psxRegs.cycle++; \
 \
	psxBSC[psxRegs.code >> 26](); \
}
#endif


#define doBranch(tar) { \
	branch2 = branch = 1; \
	branchPC = tar; \
	execI(); \
	branch = 0; \
	psxRegs.pc = branchPC; \
 \
	psxBranchTest(); \
}

// Subsets
void (*psxBSC[64])();
void (*psxSPC[64])();
void (*psxREG[32])();
void (*psxCP0[32])();
void (*psxCP2[64])();
void (*psxCP2BSC[32])();

extern void bios_write();
extern void bios_printf();

typedef struct {
    char name[8];
    char names[64][32];
    int maxn;
} irxlib;

#define IRXLIBS 14
irxlib irxlibs[32] = {
/*00*/	{ { "sysmem" } ,
    { "start", "init_memory", "retonly", "return_addr_of_memsize",
      "AllocSysMemory", "FreeSysMemory", "QueryMemSize", "QueryMaxFreeMemSize",
      "QueryTotalFreeMemSize", "QueryBlockTopAddress", "QueryBlockSize", "retonly",
      "retonly", "retonly", "Kprintf", "set_Kprintf" } ,
    16 },
/*01*/	{ { "loadcore" } ,
    { "start", "retonly", "retonly_", "return_LibraryEntryTable",
      "FlushIcache", "FlushDcache", "RegisterLibraryEntries", "ReleaseLibraryEntries",
      "findFixImports", "restoreImports", "RegisterNonAutoLinkEntries", "QueryLibraryEntryTable",
      "QueryBootMode", "RegisterBootMode", "setFlag", "resetFlag",
      "linkModule", "unlinkModule", "retonly_", "retonly_",
      "registerFunc", "jumpA0001B34", "read_header", "load_module",
      "findImageInfo" },
    25 },
/*02*/	{ { "excepman" } ,
    { "start", "reinit", "deinit", "getcommon",
      "RegisterExceptionHandler", "RegisterPriorityExceptionHandler",
      "RegisterDefaultExceptionHandler", "ReleaseExceptionHandler",
      "ReleaseDefaultExceptionHandler" } ,
    9 },
/*03_4*/{ { "intrman" } ,
    { "start", "return_0", "deinit", "call3",
      "RegisterIntrHandler", "ReleaseIntrHandler", "EnableIntr", "DisableIntr",
      "CpuDisableIntr", "CpuEnableIntr", "syscall04", "syscall08",
      "resetICTRL", "setICTRL", "syscall0C", "call15",
      "call16", "CpuSuspendIntr", "CpuResumeIntr", "CpuSuspendIntr",
      "CpuResumeIntr",  "syscall10", "syscall14", "QueryIntrContext",
      "QueryIntrStack", "iCatchMultiIntr", "retonly", "call27",
      "set_h1", "reset_h1", "set_h2", "reset_h2" } ,
    0x20 },
/*05*/	{ { "ssbusc" } ,
    { "start", "retonly", "return_0", "retonly",
      "setTable1", "getTable1", "setTable2", "getTable2",
      "setCOM_DELAY_1st", "getCOM_DELAY_1st", "setCOM_DELAY_2nd", "getCOM_DELAY_2nd",
      "setCOM_DELAY_3rd", "getCOM_DELAY_3rd", "setCOM_DELAY_4th", "getCOM_DELAY_4th",
      "setCOM_DELAY", "getCOM_DELAY" } ,
    18 },
/*06*/	{ { "dmacman" } ,
    { "start", "retonly", "deinit", "retonly",
      "SetD_MADR", "GetD_MADR", "SetD_BCR", "GetD_BCR",
      "SetD_CHCR", "GetD_CHCR", "SetD_TADR", "GetD_TADR",
      "Set_4_9_A", "Get_4_9_A", "SetDPCR", "GetDPCR",
      "SetDPCR2", "GetDPCR2", "SetDPCR3", "GetDPCR3",
      "SetDICR", "GetDICR", "SetDICR2", "GetDICR2",
      "SetBF80157C", "GetBF80157C", "SetBF801578", "GetBF801578",
      "SetDMA", "SetDMA_chainedSPU_SIF0", "SetDMA_SIF0", "SetDMA_SIF1",
      "StartTransfer", "SetVal", "EnableDMAch", "DisableDMAch" } ,
    36 },
/*07_8*/{ { "timrman" } ,
    { "start", "retonly", "retonly", "call3",
      "AllocHardTimer", "ReferHardTimer", "FreeHardTimer", "SetTimerMode",
      "GetTimerStatus", "SetTimerCounter", "GetTimerCounter", "SetTimerCompare",
      "GetTimerCompare", "SetHoldMode", "GetHoldMode", "GetHoldReg",
      "GetHardTimerIntrCode" } ,
    17 },
/*09*/	{ { "sysclib" } ,
    { "start", "reinit", "retonly", "retonly",
      "setjmp", "longjmp", "toupper", "tolower",
	  "look_ctype_table", "get_ctype_table", "memchr", "memcmp",
	  "memcpy", "memmove", "memset", "bcmp",
	  "bcopy", "bzero", "prnt", "sprintf",
	  "strcat", "strchr", "strcmp", "strcpy",
	  "strcspn", "index", "rindex", "strlen",
	  "strncat", "strncmp", "strncpy", "strpbrk",
	  "strrchr", "strspn", "strstr", "strtok",
	  "strtol", "atob", "strtoul", "wmemcopy",
	  "wmemset", "vsprintf" } ,
    0x2b },
/*0A*/	{ { "heaplib" } ,
    { "start", "retonly", "retonly", "retonly",
      "CreateHeap", "DestroyHeap", "HeapMalloc", "HeapFree",
      "HeapSize", "retonly", "retonly", "call11",
      "call12", "call13", "call14", "call15",
      "retonly", "retonly" } ,
    18 },
/*13*/	{ { "stdio" } ,
    { "start", "unknown", "unknown", "unknown",
      "printf" } ,
    5 },
/*14*/	{ { "sifman" } ,
    { "start", "retonly", "deinit", "retonly",
      "sceSif2Init", "sceSifInit", "sceSifSetDChain", "sceSifSetDma",
      "sceSifDmaStat", "sceSifSend", "sceSifSendSync", "sceSifIsSending",
      "sceSifSetSIF0DMA", "sceSifSendSync0", "sceSifIsSending0", "sceSifSetSIF1DMA",
      "sceSifSendSync1", "sceSifIsSending1", "sceSifSetSIF2DMA", "sceSifSendSync2",
      "sceSifIsSending2", "getEEIOPflags", "setEEIOPflags", "getIOPEEflags",
      "setIOPEEflags", "getEErcvaddr", "getIOPrcvaddr", "setIOPrcvaddr",
      "call28", "sceSifCheckInit", "setSif0CB", "resetSif0CB",
      "retonly", "retonly", "retonly", "retonly" } ,
    36 },
/*16*/	{ { "sifcmd" } ,
    { "start", "retonly", "deinit", "retonly",
      "sceSifInitCmd", "sceSifExitCmd", "sceSifGetSreg", "sceSifSetSreg",
      "sceSifSetCmdBuffer", "sceSifSetSysCmdBuffer",
      "sceSifAddCmdHandler", "sceSifRemoveCmdHandler",
      "sceSifSendCmd", "isceSifSendCmd", "sceSifInitRpc", "sceSifBindRpc",
      "sceSifCallRpc", "sceSifRegisterRpc",
      "sceSifCheckStatRpc", "sceSifSetRpcQueue",
      "sceSifGetNextRequest", "sceSifExecRequest",
      "sceSifRpcLoop", "sceSifGetOtherData",
      "sceSifRemoveRpc", "sceSifRemoveRpcQueue",
      "setSif1CB", "resetSif1CB",
      "retonly", "retonly", "retonly", "retonly" } ,
    32 },
/*19*/	{ { "cdvdman" } ,
    { "start", "retonly", "retonly", "retonly",
      "sceCdInit", "sceCdStandby", "sceCdRead", "sceCdSeek",
      "sceCdGetError", "sceCdGetToc", "sceCdSearchFile", "sceCdSync",
      "sceCdGetDiskType", "sceCdDiskReady", "sceCdTrayReq", "sceCdStop",
      "sceCdPosToInt", "sceCdIntToPos", "retonly", "call19",
      "sceDvdRead", "sceCdCheckCmd", "_sceCdRI", "sceCdWriteILinkID",
      "sceCdReadClock", "sceCdWriteRTC", "sceCdReadNVM", "sceCdWriteNVM",
      "sceCdStatus", "sceCdApplySCmd", "setHDmode", "sceCdOpenConfig",
      "sceCdCloseConfig", "sceCdReadConfig", "sceCdWriteConfig", "sceCdReadKey",
      "sceCdDecSet", "sceCdCallback", "sceCdPause", "sceCdBreak",
      "call40", "sceCdReadConsoleID", "sceCdWriteConsoleID", "sceCdGetMecaconVersion",
      "sceCdGetReadPos", "AudioDigitalOut", "sceCdNop", "_sceGetFsvRbuf",
      "_sceCdstm0Cb", "_sceCdstm1Cb", "_sceCdSC", "_sceCdRC",
      "sceCdForbidDVDP", "sceCdReadSubQ", "sceCdApplyNCmd", "AutoAdjustCtrl",
      "sceCdStInit", "sceCdStRead", "sceCdStSeek", "sceCdStStart",
      "sceCdStStat", "sceCdStStop" } ,
    62 },
/*??*/	{ { "sio2man" } ,
    { "start", "retonly", "deinit", "retonly",
      "set8268_ctrl", "get8268_ctrl", "get826C_recv1", "call7_send1",
      "call8_send1", "call9_send2", "call10_send2", "get8270_recv2",
      "call12_set_params", "call13_get_params", "get8274_recv3", "set8278",
      "get8278", "set827C", "get827C", "set8260_datain",
      "get8264_dataout", "set8280_intr", "get8280_intr", "signalExchange1",
      "signalExchange2", "packetExchange" } ,
    26 }
};

#define Ra0 ((char*)PSXM(psxRegs.GPR.n.a0))
#define Ra1 ((char*)PSXM(psxRegs.GPR.n.a1))
#define Ra2 ((char*)PSXM(psxRegs.GPR.n.a2))
#define Ra3 ((char*)PSXM(psxRegs.GPR.n.a3))

char* intrname[]={
"INT_VBLANK",   "INT_GM",       "INT_CDROM",   "INT_DMA",	//00
"INT_RTC0",     "INT_RTC1",     "INT_RTC2",    "INT_SIO0",	//04
"INT_SIO1",     "INT_SPU",      "INT_PIO",     "INT_EVBLANK",	//08
"INT_DVD",      "INT_PCMCIA",   "INT_RTC3",    "INT_RTC4",	//0C
"INT_RTC5",     "INT_SIO2",     "INT_HTR0",    "INT_HTR1",	//10
"INT_HTR2",     "INT_HTR3",     "INT_USB",     "INT_EXTR",	//14
"INT_FWRE",     "INT_FDMA",     "INT_1A",      "INT_1B",	//18
"INT_1C",       "INT_1D",       "INT_1E",      "INT_1F",	//1C
"INT_dmaMDECi", "INT_dmaMDECo", "INT_dmaGPU",  "INT_dmaCD",	//20
"INT_dmaSPU",   "INT_dmaPIO",   "INT_dmaOTC",  "INT_dmaBERR",	//24
"INT_dmaSPU2",  "INT_dma8",     "INT_dmaSIF0", "INT_dmaSIF1",	//28
"INT_dmaSIO2i", "INT_dmaSIO2o", "INT_2E",      "INT_2F",	//2C
"INT_30",       "INT_31",       "INT_32",      "INT_33",	//30
"INT_34",       "INT_35",       "INT_36",      "INT_37",	//34
"INT_38",       "INT_39",       "INT_3A",      "INT_3B",	//38
"INT_3C",       "INT_3D",       "INT_3E",      "INT_3F",	//3C
"INT_MAX"							//40
};

void zeroEx() {
	u32 pc;
	u32 code;
	char *lib;
	char *fname = NULL;
	int i;

	if (!Config.PsxOut) return;

	pc = psxRegs.pc;
	while (PSXMu32(pc) != 0x41e00000) pc-=4;

	lib  = (char*)PSXM(pc+12);
	code = PSXMu32(psxRegs.pc - 4) & 0xffff;

	for (i=0; i<IRXLIBS; i++) {
		if (!strncmp(lib, irxlibs[i].name, 8)) {
			if (code >= (u32)irxlibs[i].maxn) break;

			fname = irxlibs[i].names[code];
            //if( strcmp(fname, "setIOPrcvaddr") == 0 ) {
//                SysPrintf("yo\n");
//                varLog |= 0x100000;
//                Log = 1;
//            }
            break;
		}
	}

#ifdef PSXBIOS_LOG
	{char libz[9]; memcpy(libz, lib, 8); libz[8]=0;
	PSXBIOS_LOG("%s: %s (%x)"
		" (%x, %x, %x, %x)"	//comment this line to disable param showing
		, libz, fname == NULL ? "unknown" : fname, code,
		psxRegs.GPR.n.a0, psxRegs.GPR.n.a1, psxRegs.GPR.n.a2, psxRegs.GPR.n.a3);
	}
#endif

//	Log=0;
//	if (!strcmp(lib, "intrman") && code == 0x11) Log=1;
//	if (!strcmp(lib, "sifman") && code == 0x5) Log=1;
//	if (!strcmp(lib, "sifcmd") && code == 0x4) Log=1;
//	if (!strcmp(lib, "thbase") && code == 0x6) Log=1;
/*
	if (!strcmp(lib, "sifcmd") && code == 0xe) {
		branchPC = psxRegs.GPR.n.ra;
		psxRegs.GPR.n.v0 = 0;
		return;
	}
*/
	if (!strncmp(lib, "ioman", 5) && code == 7) {
		if (psxRegs.GPR.n.a0 == 1) {
			pc = psxRegs.pc;
			bios_write();
			psxRegs.pc = pc;
		}
	}

	if (!strncmp(lib, "sysmem", 6) && code == 0xe) {
		bios_printf();
		psxRegs.pc = psxRegs.GPR.n.ra;
	}

	if (!strncmp(lib, "loadcore", 8) && code == 6) {
		SysPrintf("loadcore RegisterLibraryEntries (%x): %8.8s\n", psxRegs.pc, PSXM(psxRegs.GPR.n.a0+12));
	}

	if (!strncmp(lib, "intrman", 7) && code == 4) {
		SysPrintf("intrman RegisterIntrHandler (%x): intr %s, handler %x\n", psxRegs.pc, intrname[psxRegs.GPR.n.a0], psxRegs.GPR.n.a2);
	}

	if (!strncmp(lib, "sifcmd", 6) && code == 17) {
		SysPrintf("sifcmd sceSifRegisterRpc (%x): rpc_id %x\n", psxRegs.pc, psxRegs.GPR.n.a1);
	}

#ifdef PSXBIOS_LOG
	if (!strncmp(lib, "sysclib", 8)) {
		switch (code) {
			case 0x16: // strcmp
				if (varLog & 0x00800000) EMU_LOG(" \"%s\": \"%s\"", Ra0, Ra1);
				break;

			case 0x1e: // strncpy
				if (varLog & 0x00800000) EMU_LOG(" \"%s\"", Ra1);
				break;
		}
	}
#endif

#ifdef PSXBIOS_LOG
	if (varLog & 0x00800000) EMU_LOG("\n");
#endif

/*	psxRegs.pc = branchPC;
	pc = psxRegs.GPR.n.ra;
	while (psxRegs.pc != pc) psxCpu->ExecuteBlock();

#ifdef PSXBIOS_LOG
	PSXBIOS_LOG("%s: %s (%x) END\n", lib, fname == NULL ? "unknown" : fname, code);
#endif*/
}
/*/==========================================CALL LOG
char* getName(char *file, u32 addr){
	FILE *f; u32 a;
	static char name[100];

	f=fopen(file, "r");
	if (!f)
		name[0]=0;
	else{
		while (!feof(f)){
			fscanf(f, "%08X %s\r\n", &a, name);
			if (a==addr)break;
		}
		fclose(f);
	}
	return name;
}

void spyFunctions(){
	register irxImageInfo *iii;
	if (psxRegs.pc >= 0x200000)	return;
	for (iii=(irxImageInfo*)PSXM(0x800); iii && iii->text_size;
		iii=iii->next ? (irxImageInfo*)PSXM(iii->next) : NULL)
			if (iii->vaddr<=psxRegs.pc && psxRegs.pc<iii->vaddr+iii->text_size+iii->data_size+iii->bss_size){
				if (strcmp("secrman_for_cex", PSXM(iii->name))==0){
					char *name=getName("secrman.fun", psxRegs.pc-iii->vaddr);
					if (strncmp("__push_params", name, 13)==0){
						PAD_LOG(PSXM(psxRegs.GPR.n.a0), psxRegs.GPR.n.a1, psxRegs.GPR.n.a2, psxRegs.GPR.n.a3);
					}else{
						PAD_LOG("secrman: %s (ra=%06X cycle=%d)\n", name, psxRegs.GPR.n.ra-iii->vaddr, psxRegs.cycle);}}else
				if (strcmp("mcman", PSXM(iii->name))==0){
					PAD_LOG("mcman: %s (ra=%06X cycle=%d)\n",  getName("mcman.fun", psxRegs.pc-iii->vaddr), psxRegs.GPR.n.ra-iii->vaddr, psxRegs.cycle);}else
				if (strcmp("padman", PSXM(iii->name))==0){
					PAD_LOG("padman: %s (ra=%06X cycle=%d)\n",  getName("padman.fun", psxRegs.pc-iii->vaddr), psxRegs.GPR.n.ra-iii->vaddr, psxRegs.cycle);}else
				if (strcmp("sio2man", PSXM(iii->name))==0){
					PAD_LOG("sio2man: %s (ra=%06X cycle=%d)\n", getName("sio2man.fun", psxRegs.pc-iii->vaddr), psxRegs.GPR.n.ra-iii->vaddr, psxRegs.cycle);}
				break;
			}
}
*/
/*********************************************************
* Arithmetic with immediate operand                      *
* Format:  OP rt, rs, immediate                          *
*********************************************************/
void psxADDI() 	{ if (!_Rt_) return; _rRt_ = _u32(_rRs_) + _Imm_ ; }		// Rt = Rs + Im 	(Exception on Integer Overflow)
void psxADDIU() { if (!_Rt_) { zeroEx(); return; } _rRt_ = _u32(_rRs_) + _Imm_ ; }		// Rt = Rs + Im
void psxANDI() 	{ if (!_Rt_) return; _rRt_ = _u32(_rRs_) & _ImmU_; }		// Rt = Rs And Im
void psxORI() 	{ if (!_Rt_) return; _rRt_ = _u32(_rRs_) | _ImmU_; }		// Rt = Rs Or  Im
void psxXORI() 	{ if (!_Rt_) return; _rRt_ = _u32(_rRs_) ^ _ImmU_; }		// Rt = Rs Xor Im
void psxSLTI() 	{ if (!_Rt_) return; _rRt_ = _i32(_rRs_) < _Imm_ ; }		// Rt = Rs < Im		(Signed)
void psxSLTIU() { if (!_Rt_) return; _rRt_ = _u32(_rRs_) < _ImmU_; }		// Rt = Rs < Im		(Unsigned)

/*********************************************************
* Register arithmetic                                    *
* Format:  OP rd, rs, rt                                 *
*********************************************************/
void psxADD()	{ if (!_Rd_) return; _rRd_ = _u32(_rRs_) + _u32(_rRt_); }	// Rd = Rs + Rt		(Exception on Integer Overflow)
void psxADDU() 	{ if (!_Rd_) return; _rRd_ = _u32(_rRs_) + _u32(_rRt_); }	// Rd = Rs + Rt
void psxSUB() 	{ if (!_Rd_) return; _rRd_ = _u32(_rRs_) - _u32(_rRt_); }	// Rd = Rs - Rt		(Exception on Integer Overflow)
void psxSUBU() 	{ if (!_Rd_) return; _rRd_ = _u32(_rRs_) - _u32(_rRt_); }	// Rd = Rs - Rt
void psxAND() 	{ if (!_Rd_) return; _rRd_ = _u32(_rRs_) & _u32(_rRt_); }	// Rd = Rs And Rt
void psxOR() 	{ if (!_Rd_) return; _rRd_ = _u32(_rRs_) | _u32(_rRt_); }	// Rd = Rs Or  Rt
void psxXOR() 	{ if (!_Rd_) return; _rRd_ = _u32(_rRs_) ^ _u32(_rRt_); }	// Rd = Rs Xor Rt
void psxNOR() 	{ if (!_Rd_) return; _rRd_ =~(_u32(_rRs_) | _u32(_rRt_)); }// Rd = Rs Nor Rt
void psxSLT() 	{ if (!_Rd_) return; _rRd_ = _i32(_rRs_) < _i32(_rRt_); }	// Rd = Rs < Rt		(Signed)
void psxSLTU() 	{ if (!_Rd_) return; _rRd_ = _u32(_rRs_) < _u32(_rRt_); }	// Rd = Rs < Rt		(Unsigned)

/*********************************************************
* Register mult/div & Register trap logic                *
* Format:  OP rs, rt                                     *
*********************************************************/
void psxDIV() {
	if (_rRt_ != 0) {
		_rLo_ = _i32(_rRs_) / _i32(_rRt_);
		_rHi_ = _i32(_rRs_) % _i32(_rRt_);
	}
}

void psxDIVU() {
	if (_rRt_ != 0) {
		_rLo_ = _rRs_ / _rRt_;
		_rHi_ = _rRs_ % _rRt_;
	}
}

void psxMULT() {
	u64 res = (s64)((s64)_i32(_rRs_) * (s64)_i32(_rRt_));

	psxRegs.GPR.n.lo = (u32)(res & 0xffffffff);
	psxRegs.GPR.n.hi = (u32)((res >> 32) & 0xffffffff);
}

void psxMULTU() {
	u64 res = (u64)((u64)_u32(_rRs_) * (u64)_u32(_rRt_));

	psxRegs.GPR.n.lo = (u32)(res & 0xffffffff);
	psxRegs.GPR.n.hi = (u32)((res >> 32) & 0xffffffff);
}

/*********************************************************
* Register branch logic                                  *
* Format:  OP rs, offset                                 *
*********************************************************/
#define RepZBranchi32(op)      if(_i32(_rRs_) op 0) doBranch(_BranchTarget_);
#define RepZBranchLinki32(op)  if(_i32(_rRs_) op 0) { _SetLink(31); doBranch(_BranchTarget_); }

void psxBGEZ()   { RepZBranchi32(>=) }      // Branch if Rs >= 0
void psxBGEZAL() { RepZBranchLinki32(>=) }  // Branch if Rs >= 0 and link
void psxBGTZ()   { RepZBranchi32(>) }       // Branch if Rs >  0
void psxBLEZ()   { RepZBranchi32(<=) }      // Branch if Rs <= 0
void psxBLTZ()   { RepZBranchi32(<) }       // Branch if Rs <  0
void psxBLTZAL() { RepZBranchLinki32(<) }   // Branch if Rs <  0 and link

/*********************************************************
* Shift arithmetic with constant shift                   *
* Format:  OP rd, rt, sa                                 *
*********************************************************/
void psxSLL() { if (!_Rd_) return; _rRd_ = _u32(_rRt_) << _Sa_; } // Rd = Rt << sa
void psxSRA() { if (!_Rd_) return; _rRd_ = _i32(_rRt_) >> _Sa_; } // Rd = Rt >> sa (arithmetic)
void psxSRL() { if (!_Rd_) return; _rRd_ = _u32(_rRt_) >> _Sa_; } // Rd = Rt >> sa (logical)

/*********************************************************
* Shift arithmetic with variant register shift           *
* Format:  OP rd, rt, rs                                 *
*********************************************************/
void psxSLLV() { if (!_Rd_) return; _rRd_ = _u32(_rRt_) << _u32(_rRs_); } // Rd = Rt << rs
void psxSRAV() { if (!_Rd_) return; _rRd_ = _i32(_rRt_) >> _u32(_rRs_); } // Rd = Rt >> rs (arithmetic)
void psxSRLV() { if (!_Rd_) return; _rRd_ = _u32(_rRt_) >> _u32(_rRs_); } // Rd = Rt >> rs (logical)

/*********************************************************
* Load higher 16 bits of the first word in GPR with imm  *
* Format:  OP rt, immediate                              *
*********************************************************/
void psxLUI() { if (!_Rt_) return; _rRt_ = psxRegs.code << 16; } // Upper halfword of Rt = Im

/*********************************************************
* Move from HI/LO to GPR                                 *
* Format:  OP rd                                         *
*********************************************************/
void psxMFHI() { if (!_Rd_) return; _rRd_ = _rHi_; } // Rd = Hi
void psxMFLO() { if (!_Rd_) return; _rRd_ = _rLo_; } // Rd = Lo

/*********************************************************
* Move to GPR to HI/LO & Register jump                   *
* Format:  OP rs                                         *
*********************************************************/
void psxMTHI() { _rHi_ = _rRs_; } // Hi = Rs
void psxMTLO() { _rLo_ = _rRs_; } // Lo = Rs

/*********************************************************
* Special purpose instructions                           *
* Format:  OP                                            *
*********************************************************/
void psxBREAK() {
	// Break exception - psx rom doens't handles this
	psxRegs.pc -= 4;
	psxException(0x24, branch);
}

void psxSYSCALL() {
	psxRegs.pc -= 4;
	psxException(0x20, branch);

}

void psxRFE() {
//	SysPrintf("RFE\n");
	psxRegs.CP0.n.Status = (psxRegs.CP0.n.Status & 0xfffffff0) |
						  ((psxRegs.CP0.n.Status & 0x3c) >> 2);
//	Log=0;
}

/*********************************************************
* Register branch logic                                  *
* Format:  OP rs, rt, offset                             *
*********************************************************/
#define RepBranchi32(op)      if(_i32(_rRs_) op _i32(_rRt_)) doBranch(_BranchTarget_);

void psxBEQ() {	RepBranchi32(==) }  // Branch if Rs == Rt
void psxBNE() {	RepBranchi32(!=) }  // Branch if Rs != Rt

/*********************************************************
* Jump to target                                         *
* Format:  OP target                                     *
*********************************************************/
void psxJ()   {               doBranch(_JumpTarget_); }
void psxJAL() {	_SetLink(31); doBranch(_JumpTarget_); /*spyFunctions();*/ }

/*********************************************************
* Register jump                                          *
* Format:  OP rs, rd                                     *
*********************************************************/
void psxJR()   {                 doBranch(_u32(_rRs_)); }
void psxJALR() { if (_Rd_) { _SetLink(_Rd_); } doBranch(_u32(_rRs_)); }

/*********************************************************
* Load and store for GPR                                 *
* Format:  OP rt, offset(base)                           *
*********************************************************/

#define _oB_ (_u32(_rRs_) + _Imm_)

void psxLB() {
	if (_Rt_) {
		_rRt_ = (s8 )psxMemRead8(_oB_); 
	} else {
		psxMemRead8(_oB_); 
	}
}

void psxLBU() {
	if (_Rt_) {
		_rRt_ = psxMemRead8(_oB_);
	} else {
		psxMemRead8(_oB_); 
	}
}

void psxLH() {
	if (_Rt_) {
		_rRt_ = (s16)psxMemRead16(_oB_);
	} else {
		psxMemRead16(_oB_);
	}
}

void psxLHU() {
	if (_Rt_) {
		_rRt_ = psxMemRead16(_oB_);
	} else {
		psxMemRead16(_oB_);
	}
}

void psxLW() {
	if (_Rt_) {
		_rRt_ = psxMemRead32(_oB_);
	} else {
		psxMemRead32(_oB_);
	}
}

void psxLWL() {
	u32 shift = (_oB_ & 3) << 3;
	u32 mem = psxMemRead32(_oB_ & 0xfffffffc);

	if (!_Rt_) return;
	_rRt_ =	( _u32(_rRt_) & (0x00ffffff >> shift) ) | 
				( mem << (24 - shift) );

	/*
	Mem = 1234.  Reg = abcd

	0   4bcd   (mem << 24) | (reg & 0x00ffffff)
	1   34cd   (mem << 16) | (reg & 0x0000ffff)
	2   234d   (mem <<  8) | (reg & 0x000000ff)
	3   1234   (mem      ) | (reg & 0x00000000)

	*/
}

void psxLWR() {
	u32 shift = (_oB_ & 3) << 3;
	u32 mem = psxMemRead32(_oB_ & 0xfffffffc);

	if (!_Rt_) return;
	_rRt_ =	( _u32(_rRt_) & (0xffffff00 << (24 - shift)) ) |
				( mem  >> shift );

	/*
	Mem = 1234.  Reg = abcd

	0   1234   (mem      ) | (reg & 0x00000000)
	1   a123   (mem >>  8) | (reg & 0xff000000)
	2   ab12   (mem >> 16) | (reg & 0xffff0000)
	3   abc1   (mem >> 24) | (reg & 0xffffff00)

	*/
}

void psxSB() { psxMemWrite8 (_oB_, _u8 (_rRt_)); }
void psxSH() { psxMemWrite16(_oB_, _u16(_rRt_)); }
void psxSW() { psxMemWrite32(_oB_, _u32(_rRt_)); }

void psxSWL() {
	u32 shift = (_oB_ & 3) << 3;
	u32 mem = psxMemRead32(_oB_ & 0xfffffffc);

	psxMemWrite32((_oB_ & 0xfffffffc),  ( ( _u32(_rRt_) >>  (24 - shift) ) ) |
			     (  mem & (0xffffff00 << shift) ));
	/*
	Mem = 1234.  Reg = abcd

	0   123a   (reg >> 24) | (mem & 0xffffff00)
	1   12ab   (reg >> 16) | (mem & 0xffff0000)
	2   1abc   (reg >>  8) | (mem & 0xff000000)
	3   abcd   (reg      ) | (mem & 0x00000000)

	*/
}

void psxSWR() {
	u32 shift = (_oB_ & 3) << 3;
	u32 mem = psxMemRead32(_oB_ & 0xfffffffc);

	psxMemWrite32((_oB_ & 0xfffffffc), ( ( _u32(_rRt_) << shift ) |
			     (mem  & (0x00ffffff >> (24 - shift)) ) ) );
	/*
	Mem = 1234.  Reg = abcd

	0   abcd   (reg      ) | (mem & 0x00000000)
	1   bcd4   (reg <<  8) | (mem & 0x000000ff)
	2   cd34   (reg << 16) | (mem & 0x0000ffff)
	3   d234   (reg << 24) | (mem & 0x00ffffff)

	*/
}

/*********************************************************
* Moves between GPR and COPx                             *
* Format:  OP rt, fs                                     *
*********************************************************/
void psxMFC0() { if (!_Rt_) return; _rRt_ = (int)_rFs_; }
void psxCFC0() { if (!_Rt_) return; _rRt_ = (int)_rFs_; }

void psxMTC0() { _rFs_ = _u32(_rRt_); }
void psxCTC0() { _rFs_ = _u32(_rRt_); }

/*********************************************************
* Unknow instruction (would generate an exception)       *
* Format:  ?                                             *
*********************************************************/
void psxNULL() { 
SysPrintf("psx: Unimplemented op %x\n", psxRegs.code);
}

void psxSPECIAL() {
	psxSPC[_Funct_]();
}

void psxREGIMM() {
	psxREG[_Rt_]();
}

void psxCOP0() {
	psxCP0[_Rs_]();
}

void psxCOP2() {
	psxCP2[_Funct_]();
}

void psxBASIC() {
	psxCP2BSC[_Rs_]();
}


void (*psxBSC[64])() = {
	psxSPECIAL, psxREGIMM, psxJ   , psxJAL  , psxBEQ , psxBNE , psxBLEZ, psxBGTZ,
	psxADDI   , psxADDIU , psxSLTI, psxSLTIU, psxANDI, psxORI , psxXORI, psxLUI ,
	psxCOP0   , psxNULL  , psxCOP2, psxNULL , psxNULL, psxNULL, psxNULL, psxNULL,
	psxNULL   , psxNULL  , psxNULL, psxNULL , psxNULL, psxNULL, psxNULL, psxNULL,
	psxLB     , psxLH    , psxLWL , psxLW   , psxLBU , psxLHU , psxLWR , psxNULL,
	psxSB     , psxSH    , psxSWL , psxSW   , psxNULL, psxNULL, psxSWR , psxNULL, 
	psxNULL   , psxNULL  , psxNULL, psxNULL , psxNULL, psxNULL, psxNULL, psxNULL,
	psxNULL   , psxNULL  , psxNULL, psxNULL  , psxNULL, psxNULL, psxNULL, psxNULL 
};


void (*psxSPC[64])() = {
	psxSLL , psxNULL , psxSRL , psxSRA , psxSLLV   , psxNULL , psxSRLV, psxSRAV,
	psxJR  , psxJALR , psxNULL, psxNULL, psxSYSCALL, psxBREAK, psxNULL, psxNULL,
	psxMFHI, psxMTHI , psxMFLO, psxMTLO, psxNULL   , psxNULL , psxNULL, psxNULL,
	psxMULT, psxMULTU, psxDIV , psxDIVU, psxNULL   , psxNULL , psxNULL, psxNULL,
	psxADD , psxADDU , psxSUB , psxSUBU, psxAND    , psxOR   , psxXOR , psxNOR ,
	psxNULL, psxNULL , psxSLT , psxSLTU, psxNULL   , psxNULL , psxNULL, psxNULL,
	psxNULL, psxNULL , psxNULL, psxNULL, psxNULL   , psxNULL , psxNULL, psxNULL,
	psxNULL, psxNULL , psxNULL, psxNULL, psxNULL   , psxNULL , psxNULL, psxNULL
};

void (*psxREG[32])() = {
	psxBLTZ  , psxBGEZ  , psxNULL, psxNULL, psxNULL, psxNULL, psxNULL, psxNULL,
	psxNULL  , psxNULL  , psxNULL, psxNULL, psxNULL, psxNULL, psxNULL, psxNULL,
	psxBLTZAL, psxBGEZAL, psxNULL, psxNULL, psxNULL, psxNULL, psxNULL, psxNULL,
	psxNULL  , psxNULL  , psxNULL, psxNULL, psxNULL, psxNULL, psxNULL, psxNULL
};

void (*psxCP0[32])() = {
	psxMFC0, psxNULL, psxCFC0, psxNULL, psxMTC0, psxNULL, psxCTC0, psxNULL,
	psxNULL, psxNULL, psxNULL, psxNULL, psxNULL, psxNULL, psxNULL, psxNULL,
	psxRFE , psxNULL, psxNULL, psxNULL, psxNULL, psxNULL, psxNULL, psxNULL,
	psxNULL, psxNULL, psxNULL, psxNULL, psxNULL, psxNULL, psxNULL, psxNULL
};

void (*psxCP2[64])() = {
	psxBASIC, psxNULL , psxNULL , psxNULL, psxNULL, psxNULL , psxNULL, psxNULL, // 00
	psxNULL , psxNULL , psxNULL , psxNULL, psxNULL  , psxNULL , psxNULL , psxNULL, // 08
	psxNULL , psxNULL, psxNULL, psxNULL, psxNULL , psxNULL , psxNULL , psxNULL, // 10
	psxNULL , psxNULL , psxNULL , psxNULL, psxNULL  , psxNULL , psxNULL  , psxNULL, // 18
	psxNULL  , psxNULL , psxNULL , psxNULL, psxNULL, psxNULL , psxNULL , psxNULL, // 20
	psxNULL  , psxNULL , psxNULL , psxNULL, psxNULL, psxNULL, psxNULL, psxNULL, // 28 
	psxNULL , psxNULL , psxNULL , psxNULL, psxNULL, psxNULL , psxNULL , psxNULL, // 30
	psxNULL , psxNULL , psxNULL , psxNULL, psxNULL, psxNULL  , psxNULL  , psxNULL  // 38
};

void (*psxCP2BSC[32])() = {
	psxNULL, psxNULL, psxNULL, psxNULL, psxNULL, psxNULL, psxNULL, psxNULL,
	psxNULL, psxNULL, psxNULL, psxNULL, psxNULL, psxNULL, psxNULL, psxNULL,
	psxNULL, psxNULL, psxNULL, psxNULL, psxNULL, psxNULL, psxNULL, psxNULL,
	psxNULL, psxNULL, psxNULL, psxNULL, psxNULL, psxNULL, psxNULL, psxNULL
};


///////////////////////////////////////////

static int intInit() {
	return 0;
}

static void intReset() {
}

static void intExecute() {
	for (;;) execI();
}

#ifdef _DEBUG
extern u32 psxdump;
extern void iDumpPsxRegisters(u32,u32);
#endif

static void intExecuteBlock() {
	while (EEsCycle > 0){
		branch2 = 0;
		while (!branch2) {
			execI();

#ifdef _DEBUG
            if( psxdump & 16 ) {
                iDumpPsxRegisters(psxRegs.pc,1);
            }
#endif
        }
	}
}

static void intClear(u32 Addr, u32 Size) {
}

static void intShutdown() {
}

R3000Acpu psxInt = {
	intInit,
	intReset,
	intExecute,
	intExecuteBlock,
	intClear,
	intShutdown
};
