/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2009  PCSX2 Dev Team
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


#include "PrecompiledHeader.h"
#include "IopCommon.h"

using namespace R3000A;

// Used to flag delay slot instructions when throwig exceptions.
bool iopIsDelaySlot = false;

static bool branch2 = 0;
static u32 branchPC;

static void doBranch(s32 tar);	// forward declared prototype

struct irxlib {
    char name[16];
    char names[64][64];
    int maxn;
};

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

#define Ra0 (iopVirtMemR<char>(psxRegs.GPR.n.a0))
#define Ra1 (iopVirtMemR<char>(psxRegs.GPR.n.a1))
#define Ra2 (iopVirtMemR<char>(psxRegs.GPR.n.a2))
#define Ra3 (iopVirtMemR<char>(psxRegs.GPR.n.a3))

const char* intrname[]={
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

void zeroEx()
{
#ifdef PCSX2_DEVBUILD
	u32 pc;
	u32 code;
	const char *lib;
	char *fname = NULL;
	int i;

	pc = psxRegs.pc;
	while (iopMemRead32(pc) != 0x41e00000) pc-=4;

	lib  = iopVirtMemR<char>(pc+12);
	code = iopMemRead32(psxRegs.pc - 4) & 0xffff;

	for (i=0; i<IRXLIBS; i++) {
		if (!strncmp(lib, irxlibs[i].name, 8)) {
			if (code >= (u32)irxlibs[i].maxn) break;

			fname = irxlibs[i].names[code];
            //if( strcmp(fname, "setIOPrcvaddr") == 0 ) {
//                Console.WriteLn("yo");
//                varLog |= 0x100000;
//                Log = 1;
//            }
            break;
		}
	}

	{char libz[9]; memcpy(libz, lib, 8); libz[8]=0;
	PSXBIOS_LOG("%s: %s (%x)"
		" (%x, %x, %x, %x)"	//comment this line to disable param showing
		, libz, fname == NULL ? "unknown" : fname, code,
		psxRegs.GPR.n.a0, psxRegs.GPR.n.a1, psxRegs.GPR.n.a2, psxRegs.GPR.n.a3);
	}

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
		DevCon.WriteLn( Color_Gray, "loadcore RegisterLibraryEntries (%x): %8.8s", psxRegs.pc, iopVirtMemR<char>(psxRegs.GPR.n.a0+12));
	}

	if (!strncmp(lib, "intrman", 7) && code == 4) {
		DevCon.WriteLn( Color_Gray, "intrman RegisterIntrHandler (%x): intr %s, handler %x", psxRegs.pc, intrname[psxRegs.GPR.n.a0], psxRegs.GPR.n.a2);
	}

	if (!strncmp(lib, "sifcmd", 6) && code == 17) {
		DevCon.WriteLn( Color_Gray, "sifcmd sceSifRegisterRpc (%x): rpc_id %x", psxRegs.pc, psxRegs.GPR.n.a1);
	}

	if (!strncmp(lib, "sysclib", 8))
	{
		switch (code)
		{
			case 0x16: // strcmp
				PSXBIOS_LOG(" \"%s\": \"%s\"", Ra0, Ra1);
				break;

			case 0x1e: // strncpy
				PSXBIOS_LOG(" \"%s\"", Ra1);
				break;
		}
	}

/*	psxRegs.pc = branchPC;
	pc = psxRegs.GPR.n.ra;
	while (psxRegs.pc != pc) psxCpu->ExecuteBlock();

	PSXBIOS_LOG("%s: %s (%x) END", lib, fname == NULL ? "unknown" : fname, code);*/
#endif

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
			fscanf(f, "%08X %s", &a, name);
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
						PAD_LOG("secrman: %s (ra=%06X cycle=%d)", name, psxRegs.GPR.n.ra-iii->vaddr, psxRegs.cycle);}}else
				if (strcmp("mcman", PSXM(iii->name))==0){
					PAD_LOG("mcman: %s (ra=%06X cycle=%d)",  getName("mcman.fun", psxRegs.pc-iii->vaddr), psxRegs.GPR.n.ra-iii->vaddr, psxRegs.cycle);}else
				if (strcmp("padman", PSXM(iii->name))==0){
					PAD_LOG("padman: %s (ra=%06X cycle=%d)",  getName("padman.fun", psxRegs.pc-iii->vaddr), psxRegs.GPR.n.ra-iii->vaddr, psxRegs.cycle);}else
				if (strcmp("sio2man", PSXM(iii->name))==0){
					PAD_LOG("sio2man: %s (ra=%06X cycle=%d)", getName("sio2man.fun", psxRegs.pc-iii->vaddr), psxRegs.GPR.n.ra-iii->vaddr, psxRegs.cycle);}
				break;
			}
}
*/
/*********************************************************
* Register branch logic                                  *
* Format:  OP rs, offset                                 *
*********************************************************/

void psxBGEZ()         // Branch if Rs >= 0
{ 
	if (_i32(_rRs_) >= 0) doBranch(_BranchTarget_); 
}

void psxBGEZAL()   // Branch if Rs >= 0 and link
{ 
	if (_i32(_rRs_) >= 0) 
	{ 
		_SetLink(31); 
		doBranch(_BranchTarget_); 
	} 
}

void psxBGTZ()          // Branch if Rs >  0
{ 
	if (_i32(_rRs_) > 0) doBranch(_BranchTarget_); 
}

void psxBLEZ()         // Branch if Rs <= 0
{ 
	if (_i32(_rRs_) <= 0) doBranch(_BranchTarget_); 
}
void psxBLTZ()          // Branch if Rs <  0
{ 
	if (_i32(_rRs_) < 0) doBranch(_BranchTarget_); 
}

void psxBLTZAL()    // Branch if Rs <  0 and link
{ 
	if (_i32(_rRs_) < 0) 
		{
			_SetLink(31); 
			doBranch(_BranchTarget_); 
		} 
}

/*********************************************************
* Register branch logic                                  *
* Format:  OP rs, rt, offset                             *
*********************************************************/

void psxBEQ()   // Branch if Rs == Rt
{ 
	if (_i32(_rRs_) == _i32(_rRt_)) doBranch(_BranchTarget_);
}

void psxBNE()   // Branch if Rs != Rt
{ 
	if (_i32(_rRs_) != _i32(_rRt_)) doBranch(_BranchTarget_); 
}

/*********************************************************
* Jump to target                                         *
* Format:  OP target                                     *
*********************************************************/
void psxJ()   
{             
	doBranch(_JumpTarget_); 
}

void psxJAL() 
{	
	_SetLink(31);
	doBranch(_JumpTarget_); 
	/*spyFunctions();*/ 
}

/*********************************************************
* Register jump                                          *
* Format:  OP rs, rd                                     *
*********************************************************/
void psxJR()   
{              
	doBranch(_u32(_rRs_));
}

void psxJALR() 
{
	if (_Rd_) 
	{
		_SetLink(_Rd_);
	} 
	doBranch(_u32(_rRs_));
}

///////////////////////////////////////////
// These macros are used to assemble the repassembler functions

static __forceinline void execI()
{
	psxRegs.code = iopMemRead32(psxRegs.pc);
	
	//if( (psxRegs.pc >= 0x1200 && psxRegs.pc <= 0x1400) || (psxRegs.pc >= 0x0b40 && psxRegs.pc <= 0x1000))
		PSXCPU_LOG("%s", disR3000AF(psxRegs.code, psxRegs.pc));

	psxRegs.pc+= 4;
	psxRegs.cycle++;
	psxCycleEE-=8;

	psxBSC[psxRegs.code >> 26]();
}


static void doBranch(s32 tar) {
	branch2 = iopIsDelaySlot = true;
	branchPC = tar;
	execI();
	PSXCPU_LOG( "\n" );
	iopIsDelaySlot = false;
	psxRegs.pc = branchPC;

	psxBranchTest();
}

static void intAlloc() {
}

static void intReset() {
}

static void intExecute() {
	for (;;) execI();
}

static s32 intExecuteBlock( s32 eeCycles )
{
	psxBreak = 0;
	psxCycleEE = eeCycles;

	while (psxCycleEE > 0){
		branch2 = 0;
		while (!branch2) {
			execI();
        }
	}
	return psxBreak + psxCycleEE;
}

static void intClear(u32 Addr, u32 Size) {
}

static void intShutdown() {
}

R3000Acpu psxInt = {
	intAlloc,
	intReset,
	intExecute,
	intExecuteBlock,
	intClear,
	intShutdown
};
