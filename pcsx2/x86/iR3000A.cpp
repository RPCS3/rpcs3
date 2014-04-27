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

// recompiler reworked to add dynamic linking Jan06
// and added reg caching, const propagation, block analysis Jun06
// zerofrog(@gmail.com)


#include "PrecompiledHeader.h"

#include "iR3000A.h"
#include "BaseblockEx.h"
#include "System/RecTypes.h"

#include <time.h>

#ifndef _WIN32
#include <sys/types.h>
#endif

#include "IopCommon.h"
#include "iCore.h"

#include "NakedAsm.h"
#include "AppConfig.h"


using namespace x86Emitter;

extern u32 g_iopNextEventCycle;
extern void psxBREAK();

u32 g_psxMaxRecMem = 0;
u32 s_psxrecblocks[] = {0};

uptr psxRecLUT[0x10000];
uptr psxhwLUT[0x10000];

#define HWADDR(mem) (psxhwLUT[mem >> 16] + (mem))

static RecompiledCodeReserve* recMem = NULL;

static BASEBLOCK *recRAM = NULL;	// and the ptr to the blocks here
static BASEBLOCK *recROM = NULL;	// and here
static BASEBLOCK *recROM1 = NULL;	// also here
static BaseBlocks recBlocks;
static u8 *recPtr = NULL;
u32 psxpc;			// recompiler psxpc
int psxbranch;		// set for branch
u32 g_iopCyclePenalty;

static EEINST* s_pInstCache = NULL;
static u32 s_nInstCacheSize = 0;

static BASEBLOCK* s_pCurBlock = NULL;
static BASEBLOCKEX* s_pCurBlockEx = NULL;

static u32 s_nEndBlock = 0; // what psxpc the current block ends
static u32 s_branchTo;
static bool s_nBlockFF;

static u32 s_saveConstRegs[32];
static u32 s_saveHasConstReg = 0, s_saveFlushedConstReg = 0;
static EEINST* s_psaveInstInfo = NULL;

u32 s_psxBlockCycles = 0; // cycles of current block recompiling
static u32 s_savenBlockCycles = 0;

static void iPsxBranchTest(u32 newpc, u32 cpuBranch);
void psxRecompileNextInstruction(int delayslot);

extern void (*rpsxBSC[64])();
void rpsxpropBSC(EEINST* prev, EEINST* pinst);

static void iopClearRecLUT(BASEBLOCK* base, int count);

static u32 psxdump = 0;

#define PSX_GETBLOCK(x) PC_GETBLOCK_(x, psxRecLUT)

#define PSXREC_CLEARM(mem) \
	(((mem) < g_psxMaxRecMem && (psxRecLUT[(mem) >> 16] + (mem))) ? \
		psxRecClearMem(mem) : 4)

// =====================================================================================================
//  Dynamically Compiled Dispatchers - R3000A style
// =====================================================================================================

static void __fastcall iopRecRecompile( const u32 startpc );

static u32 s_store_ebp, s_store_esp;

// Recompiled code buffer for EE recompiler dispatchers!
static u8 __pagealigned iopRecDispatchers[__pagesize];

typedef void DynGenFunc();

static DynGenFunc* iopDispatcherEvent		= NULL;
static DynGenFunc* iopDispatcherReg			= NULL;
static DynGenFunc* iopJITCompile			= NULL;
static DynGenFunc* iopJITCompileInBlock		= NULL;
static DynGenFunc* iopEnterRecompiledCode	= NULL;
static DynGenFunc* iopExitRecompiledCode	= NULL;

static void recEventTest()
{
	_cpuEventTest_Shared();
}

// parameters:
//   espORebp - 0 for ESP, or 1 for EBP.
//   regval   - current value of the register at the time the fault was detected (predates the
//      stackframe setup code in this function)
static void __fastcall StackFrameCheckFailed( int espORebp, int regval )
{
	pxFailDev( pxsFmt( L"(R3000A Recompiler Stackframe) Sanity check failed on %s\n\tCurrent=%d; Saved=%d",
		(espORebp==0) ? L"ESP" : L"EBP", regval, (espORebp==0) ? s_store_esp : s_store_ebp )
	);

	// Note: The recompiler will attempt to recover ESP and EBP after returning from this function,
	// so typically selecting Continue/Ignore/Cancel for this assertion should allow PCSX2 to con-
	// tinue to run with some degree of stability.
}

static void _DynGen_StackFrameCheck()
{
	if( !IsDevBuild ) return;

	// --------- EBP Here -----------

	xCMP( ebp, ptr[&s_store_ebp] );
	xForwardJE8 skipassert_ebp;

	xMOV( ecx, 1 );					// 1 specifies EBP
	xMOV( edx, ebp );
	xCALL( StackFrameCheckFailed );
	xMOV( ebp, ptr[&s_store_ebp] );		// half-hearted frame recovery attempt!

	skipassert_ebp.SetTarget();

	// --------- ESP There -----------

	xCMP( esp, ptr[&s_store_esp] );
	xForwardJE8 skipassert_esp;

	xXOR( ecx, ecx );				// 0 specifies ESP
	xMOV( edx, esp );
	xCALL( StackFrameCheckFailed );
	xMOV( esp, ptr[&s_store_esp] );		// half-hearted frame recovery attempt!

	skipassert_esp.SetTarget();
}

// The address for all cleared blocks.  It recompiles the current pc and then
// dispatches to the recompiled block address.
static DynGenFunc* _DynGen_JITCompile()
{
	pxAssertMsg( iopDispatcherReg != NULL, "Please compile the DispatcherReg subroutine *before* JITComple.  Thanks." );

	u8* retval = xGetPtr();
	_DynGen_StackFrameCheck();

	xMOV( ecx, ptr[&psxRegs.pc] );
	xCALL( iopRecRecompile );

	xMOV( eax, ptr[&psxRegs.pc] );
	xMOV( ebx, eax );
	xSHR( eax, 16 );
	xMOV( ecx, ptr[psxRecLUT + (eax*4)] );
	xJMP( ptr32[ecx+ebx] );

	return (DynGenFunc*)retval;
}

static DynGenFunc* _DynGen_JITCompileInBlock()
{
	u8* retval = xGetPtr();
	xJMP( iopJITCompile );
	return (DynGenFunc*)retval;
}

// called when jumping to variable pc address
static DynGenFunc* _DynGen_DispatcherReg()
{
	u8* retval = xGetPtr();
	_DynGen_StackFrameCheck();

	xMOV( eax, ptr[&psxRegs.pc] );
	xMOV( ebx, eax );
	xSHR( eax, 16 );
	xMOV( ecx, ptr[psxRecLUT + (eax*4)] );
	xJMP( ptr32[ecx+ebx] );

	return (DynGenFunc*)retval;
}

// --------------------------------------------------------------------------------------
//  EnterRecompiledCode  - dynamic compilation stub!
// --------------------------------------------------------------------------------------

// In Release Builds this literally generates the following code:
//   push edi
//   push esi
//   push ebx
//   jmp DispatcherReg
//   pop ebx
//   pop esi
//   pop edi
//
// See notes on why this works in both GCC (aligned stack!) and other compilers (not-so-
// aligned stack!).  In debug/dev builds the code gen is more complicated, as it constructs
// ebp stackframe mess, which allows for a complete backtrace from debug breakpoints (yay).
//
// Also, if you set PCSX2_IOP_FORCED_ALIGN_STACK to 1, the codegen for MSVC becomes slightly
// more complicated since it has to perform a full stack alignment on entry.
//

#if defined(__GNUG__) || defined(__DARWIN__)
#	define PCSX2_ASSUME_ALIGNED_STACK		1
#else
#	define PCSX2_ASSUME_ALIGNED_STACK		0
#endif

// Set to 0 for a speedup in release builds.
// [doesn't apply to GCC/Mac, which must always align]
#define PCSX2_IOP_FORCED_ALIGN_STACK		0 //1


// For overriding stackframe generation options in Debug builds (possibly useful for troubleshooting)
// Typically this value should be the same as IsDevBuild.
static const bool GenerateStackFrame = IsDevBuild;

static DynGenFunc* _DynGen_EnterRecompiledCode()
{
	u8* retval = xGetPtr();

	bool allocatedStack = GenerateStackFrame || PCSX2_IOP_FORCED_ALIGN_STACK;

	// Optimization: The IOP never uses stack-based parameter invocation, so we can avoid
	// allocating any room on the stack for it (which is important since the IOP's entry
	// code gets invoked quite a lot).

	if( allocatedStack )
	{
		xPUSH( ebp );
		xMOV( ebp, esp );
		xAND( esp, -0x10 );

		xSUB( esp, 0x20 );

		xMOV( ptr[ebp-12], edi );
		xMOV( ptr[ebp-8], esi );
		xMOV( ptr[ebp-4], ebx );
	}
	else
	{
		// GCC Compiler:
		//   The frame pointer coming in from the EE's event test can be safely assumed to be
		//   aligned, since GCC always aligns stackframes.  While handy in x86-64, where CALL + PUSH EBP
		//   results in a neatly realigned stack on entry to every function, unfortunately in x86-32
		//   this is usually worthless because CALL+PUSH leaves us 8 byte aligned instead (fail).  So
		//   we have to do the usual set of stackframe alignments and simulated callstack mess
		//   *regardless*.

		// MSVC/Intel compilers:
		//   The PCSX2_IOP_FORCED_ALIGN_STACK setting is 0, so we don't care.  Just push regs like
		//   the good old days!  (stack alignment will be indeterminate)

		xPUSH( edi );
		xPUSH( esi );
		xPUSH( ebx );

		allocatedStack = false;
	}

	uptr* imm = NULL;
	if( allocatedStack )
	{
		if( GenerateStackFrame )
		{
			// Simulate a CALL function by pushing the call address and EBP onto the stack.
			// This retains proper stacktrace and stack unwinding (handy in devbuilds!)

			xMOV( ptr32[esp+0x0c], 0xffeeff );
			imm = (uptr*)(xGetPtr()-4);

			// This part simulates the "normal" stackframe prep of "push ebp, mov ebp, esp"
			xMOV( ptr32[esp+0x08], ebp );
			xLEA( ebp, ptr32[esp+0x08] );
		}
	}

	if( IsDevBuild )
	{
		xMOV( ptr[&s_store_esp], esp );
		xMOV( ptr[&s_store_ebp], ebp );
	}

	xJMP( iopDispatcherReg );
	if( imm != NULL )
		*imm = (uptr)xGetPtr();

	// ----------------------
	// ---->  Cleanup!  ---->

	iopExitRecompiledCode = (DynGenFunc*)xGetPtr();

	if( allocatedStack )
	{
		// pop the nested "simulated call" stackframe, if needed:
		if( GenerateStackFrame ) xLEAVE();
		xMOV( edi, ptr[ebp-12] );
		xMOV( esi, ptr[ebp-8] );
		xMOV( ebx, ptr[ebp-4] );
		xLEAVE();
	}
	else
	{
		xPOP( ebx );
		xPOP( esi );
		xPOP( edi );
	}

	xRET();

	return (DynGenFunc*)retval;
}

static void _DynGen_Dispatchers()
{
	// In case init gets called multiple times:
	HostSys::MemProtectStatic( iopRecDispatchers, PageAccess_ReadWrite() );

	// clear the buffer to 0xcc (easier debugging).
	memset_8<0xcc,__pagesize>( iopRecDispatchers );

	xSetPtr( iopRecDispatchers );

	// Place the EventTest and DispatcherReg stuff at the top, because they get called the
	// most and stand to benefit from strong alignment and direct referencing.
	iopDispatcherEvent = (DynGenFunc*)xGetPtr();
	xCALL( recEventTest );
	iopDispatcherReg	= _DynGen_DispatcherReg();

	iopJITCompile			= _DynGen_JITCompile();
	iopJITCompileInBlock	= _DynGen_JITCompileInBlock();
	iopEnterRecompiledCode	= _DynGen_EnterRecompiledCode();

	HostSys::MemProtectStatic( iopRecDispatchers, PageAccess_ExecOnly() );

	recBlocks.SetJITCompile( iopJITCompile );
}

////////////////////////////////////////////////////
using namespace R3000A;
#include "Utilities/AsciiFile.h"

static void iIopDumpBlock( int startpc, u8 * ptr )
{
	u32 i, j;
	EEINST* pcur;
	u8 used[34];
	int numused, count;

	Console.WriteLn( "dump1 %x:%x, %x", startpc, psxpc, psxRegs.cycle );
	g_Conf->Folders.Logs.Mkdir();

	wxString filename( Path::Combine( g_Conf->Folders.Logs, wxsFormat( L"psxdump%.8X.txt", startpc ) ) );
	AsciiFile f( filename, L"w" );

	/*for ( i = startpc; i < s_nEndBlock; i += 4 ) {
		f.Printf("%s\n", disR3000Fasm( iopMemRead32( i ), i ) );
	}*/

	// write the instruction info
	f.Printf("\n\nlive0 - %x, lastuse - %x used - %x\n", EEINST_LIVE0, EEINST_LASTUSE, EEINST_USED);

	memzero(used);
	numused = 0;
	for(i = 0; i < ArraySize(s_pInstCache->regs); ++i) {
		if( s_pInstCache->regs[i] & EEINST_USED ) {
			used[i] = 1;
			numused++;
		}
	}

	f.Printf("       ");
	for(i = 0; i < ArraySize(s_pInstCache->regs); ++i) {
		if( used[i] ) f.Printf("%2d ", i);
	}
	f.Printf("\n");

	f.Printf("       ");
	for(i = 0; i < ArraySize(s_pInstCache->regs); ++i) {
		if( used[i] ) f.Printf("%s ", disRNameGPR[i]);
	}
	f.Printf("\n");

	pcur = s_pInstCache+1;
	for( i = 0; i < (s_nEndBlock-startpc)/4; ++i, ++pcur) {
		f.Printf("%2d: %2.2x ", i+1, pcur->info);

		count = 1;
		for(j = 0; j < ArraySize(s_pInstCache->regs); j++) {
			if( used[j] ) {
				f.Printf("%2.2x%s", pcur->regs[j], ((count%8)&&count<numused)?"_":" ");
				++count;
			}
		}
		f.Printf("\n");
	}

#ifdef __LINUX__
	char command[256];
	// dump the asm
	{
		AsciiFile f2( L"mydump1", L"w" );
		f2.Write( ptr, (uptr)x86Ptr - (uptr)ptr );
	}
	wxCharBuffer buf( filename.ToUTF8() );
	const char* filenamea = buf.data();
	sprintf( command, "objdump -D --target=binary --architecture=i386 -M intel mydump1 | cat %s - > tempdump", filenamea );
	system( command );
    sprintf( command, "mv tempdump %s", filenamea );
    system( command );
    //f = fopen( filename.c_str(), "a+" );
#endif
}

u8 _psxLoadWritesRs(u32 tempcode)
{
	switch(tempcode>>26) {
		case 32: case 33: case 34: case 35: case 36: case 37: case 38:
			return ((tempcode>>21)&0x1f)==((tempcode>>16)&0x1f); // rs==rt
	}
	return 0;
}

u8 _psxIsLoadStore(u32 tempcode)
{
	switch(tempcode>>26) {
		case 32: case 33: case 34: case 35: case 36: case 37: case 38:
		// 4 byte stores
		case 40: case 41: case 42: case 43: case 46:
			return 1;
	}
	return 0;
}

void _psxFlushAllUnused()
{
	int i;
	for(i = 0; i < 34; ++i) {
		if( psxpc < s_nEndBlock ) {
			if( (g_pCurInstInfo[1].regs[i]&EEINST_USED) )
				continue;
		}
		else if( (g_pCurInstInfo[0].regs[i]&EEINST_USED) )
			continue;

		if( i < 32 && PSX_IS_CONST1(i) ) _psxFlushConstReg(i);
		else {
			_deleteX86reg(X86TYPE_PSX, i, 1);
		}
	}
}

int _psxFlushUnusedConstReg()
{
	int i;
	for(i = 1; i < 32; ++i) {
		if( (g_psxHasConstReg & (1<<i)) && !(g_psxFlushedConstReg&(1<<i)) &&
			!_recIsRegWritten(g_pCurInstInfo+1, (s_nEndBlock-psxpc)/4, XMMTYPE_GPRREG, i) ) {

			// check if will be written in the future
			MOV32ItoM((uptr)&psxRegs.GPR.r[i], g_psxConstRegs[i]);
			g_psxFlushedConstReg |= 1<<i;
			return 1;
		}
	}

	return 0;
}

void _psxFlushCachedRegs()
{
	_psxFlushConstRegs();
}

void _psxFlushConstReg(int reg)
{
	if( PSX_IS_CONST1( reg ) && !(g_psxFlushedConstReg&(1<<reg)) ) {
		MOV32ItoM((uptr)&psxRegs.GPR.r[reg], g_psxConstRegs[reg]);
		g_psxFlushedConstReg |= (1<<reg);
	}
}

void _psxFlushConstRegs()
{
	int i;

	// flush constants

	// ignore r0
	for(i = 1; i < 32; ++i) {
		if( g_psxHasConstReg & (1<<i) ) {

			if( !(g_psxFlushedConstReg&(1<<i)) ) {
				MOV32ItoM((uptr)&psxRegs.GPR.r[i], g_psxConstRegs[i]);
				g_psxFlushedConstReg |= 1<<i;
			}

			if( g_psxHasConstReg == g_psxFlushedConstReg )
				break;
		}
	}
}

void _psxDeleteReg(int reg, int flush)
{
	if( !reg ) return;
	if( flush && PSX_IS_CONST1(reg) ) {
		_psxFlushConstReg(reg);
		return;
	}
	PSX_DEL_CONST(reg);
	_deleteX86reg(X86TYPE_PSX, reg, flush ? 0 : 2);
}

void _psxMoveGPRtoR(x86IntRegType to, int fromgpr)
{
	if( PSX_IS_CONST1(fromgpr) )
		MOV32ItoR( to, g_psxConstRegs[fromgpr] );
	else {
		// check x86
		MOV32MtoR(to, (uptr)&psxRegs.GPR.r[ fromgpr ] );
	}
}

void _psxMoveGPRtoM(u32 to, int fromgpr)
{
	if( PSX_IS_CONST1(fromgpr) )
		MOV32ItoM( to, g_psxConstRegs[fromgpr] );
	else {
		// check x86
		MOV32MtoR(EAX, (uptr)&psxRegs.GPR.r[ fromgpr ] );
		MOV32RtoM(to, EAX );
	}
}

void _psxMoveGPRtoRm(x86IntRegType to, int fromgpr)
{
	if( PSX_IS_CONST1(fromgpr) )
		MOV32ItoRm( to, g_psxConstRegs[fromgpr] );
	else {
		// check x86
		MOV32MtoR(EAX, (uptr)&psxRegs.GPR.r[ fromgpr ] );
		MOV32RtoRm(to, EAX );
	}
}

void _psxFlushCall(int flushtype)
{
	// x86-32 ABI : These registers are not preserved across calls:
	_freeX86reg( EAX );
	_freeX86reg( ECX );
	_freeX86reg( EDX );

	if( flushtype & FLUSH_CACHED_REGS )
		_psxFlushConstRegs();
}

void psxSaveBranchState()
{
	s_savenBlockCycles = s_psxBlockCycles;
	memcpy(s_saveConstRegs, g_psxConstRegs, sizeof(g_psxConstRegs));
	s_saveHasConstReg = g_psxHasConstReg;
	s_saveFlushedConstReg = g_psxFlushedConstReg;
	s_psaveInstInfo = g_pCurInstInfo;

	// save all regs
	memcpy(s_saveX86regs, x86regs, sizeof(x86regs));
}

void psxLoadBranchState()
{
	s_psxBlockCycles = s_savenBlockCycles;

	memcpy(g_psxConstRegs, s_saveConstRegs, sizeof(g_psxConstRegs));
	g_psxHasConstReg = s_saveHasConstReg;
	g_psxFlushedConstReg = s_saveFlushedConstReg;
	g_pCurInstInfo = s_psaveInstInfo;

	// restore all regs
	memcpy(x86regs, s_saveX86regs, sizeof(x86regs));
}

////////////////////
// Code Templates //
////////////////////

void _psxOnWriteReg(int reg)
{
	PSX_DEL_CONST(reg);
}

// rd = rs op rt
void psxRecompileCodeConst0(R3000AFNPTR constcode, R3000AFNPTR_INFO constscode, R3000AFNPTR_INFO consttcode, R3000AFNPTR_INFO noconstcode)
{
	if ( ! _Rd_ ) return;

	// for now, don't support xmm

	_deleteX86reg(X86TYPE_PSX, _Rs_, 1);
	_deleteX86reg(X86TYPE_PSX, _Rt_, 1);
	_deleteX86reg(X86TYPE_PSX, _Rd_, 0);

	if( PSX_IS_CONST2(_Rs_, _Rt_) ) {
		PSX_SET_CONST(_Rd_);
		constcode();
		return;
	}

	if( PSX_IS_CONST1(_Rs_) ) {
		constscode(0);
		PSX_DEL_CONST(_Rd_);
		return;
	}

	if( PSX_IS_CONST1(_Rt_) ) {
		consttcode(0);
		PSX_DEL_CONST(_Rd_);
		return;
	}

	noconstcode(0);
	PSX_DEL_CONST(_Rd_);
}

// rt = rs op imm16
void psxRecompileCodeConst1(R3000AFNPTR constcode, R3000AFNPTR_INFO noconstcode)
{
    if ( ! _Rt_ ) {
		// check for iop module import table magic
        if (psxRegs.code >> 16 == 0x2400) {
			MOV32ItoM( (uptr)&psxRegs.code, psxRegs.code );
			MOV32ItoM( (uptr)&psxRegs.pc, psxpc );
			_psxFlushCall(FLUSH_NODESTROY);

			const char *libname = irxImportLibname(psxpc);
			u16 index = psxRegs.code & 0xffff;
#ifdef PCSX2_DEVBUILD
			const char *funcname = irxImportFuncname(libname, index);
			irxDEBUG debug = irxImportDebug(libname, index);

			if (SysTraceActive(IOP.Bios)) {
				xMOV(ecx, (uptr)libname);
				xMOV(edx, index);
				xPUSH((uptr)funcname);
				xCALL(irxImportLog);
			}

			if (debug)
				xCALL(debug);
#endif
			irxHLE hle = irxImportHLE(libname, index);
			if (hle) {
				xCALL(hle);
				xCMP(eax, 0);
				xJNE(iopDispatcherReg);
			}
		}
        return;
    }

	// for now, don't support xmm

	_deleteX86reg(X86TYPE_PSX, _Rs_, 1);
	_deleteX86reg(X86TYPE_PSX, _Rt_, 0);

	if( PSX_IS_CONST1(_Rs_) ) {
		PSX_SET_CONST(_Rt_);
		constcode();
		return;
	}

	noconstcode(0);
	PSX_DEL_CONST(_Rt_);
}

// rd = rt op sa
void psxRecompileCodeConst2(R3000AFNPTR constcode, R3000AFNPTR_INFO noconstcode)
{
	if ( ! _Rd_ ) return;

	// for now, don't support xmm

	_deleteX86reg(X86TYPE_PSX, _Rt_, 1);
	_deleteX86reg(X86TYPE_PSX, _Rd_, 0);

	if( PSX_IS_CONST1(_Rt_) ) {
		PSX_SET_CONST(_Rd_);
		constcode();
		return;
	}

	noconstcode(0);
	PSX_DEL_CONST(_Rd_);
}

// rd = rt MULT rs  (SPECIAL)
void psxRecompileCodeConst3(R3000AFNPTR constcode, R3000AFNPTR_INFO constscode, R3000AFNPTR_INFO consttcode, R3000AFNPTR_INFO noconstcode, int LOHI)
{
	_deleteX86reg(X86TYPE_PSX, _Rs_, 1);
	_deleteX86reg(X86TYPE_PSX, _Rt_, 1);

	if( LOHI ) {
		_deleteX86reg(X86TYPE_PSX, PSX_HI, 1);
		_deleteX86reg(X86TYPE_PSX, PSX_LO, 1);
	}

	if( PSX_IS_CONST2(_Rs_, _Rt_) ) {
		constcode();
		return;
	}

	if( PSX_IS_CONST1(_Rs_) ) {
		constscode(0);
		return;
	}

	if( PSX_IS_CONST1(_Rt_) ) {
		consttcode(0);
		return;
	}

	noconstcode(0);
}

static uptr m_ConfiguredCacheReserve = 32;
static u8* m_recBlockAlloc = NULL;

static const uint m_recBlockAllocSize =
	(((Ps2MemSize::IopRam + Ps2MemSize::Rom + Ps2MemSize::Rom1) / 4) * sizeof(BASEBLOCK));

static void recReserveCache()
{
	if (!recMem) recMem = new RecompiledCodeReserve(L"R3000A Recompiler Cache", _1mb * 2);
	recMem->SetProfilerName("IOPrec");

	while (!recMem->IsOk())
	{
		if (recMem->Reserve( m_ConfiguredCacheReserve * _1mb, HostMemoryMap::IOPrec ) != NULL) break;

		// If it failed, then try again (if possible):
		if (m_ConfiguredCacheReserve < 4) break;
		m_ConfiguredCacheReserve /= 2;
	}

	recMem->ThrowIfNotOk();
}

static void recReserve()
{
	// IOP has no hardware requirements!

	recReserveCache();
}

static void recAlloc()
{
	// Goal: Allocate BASEBLOCKs for every possible branch target in IOP memory.
	// Any 4-byte aligned address makes a valid branch target as per MIPS design (all instructions are
	// always 4 bytes long).

	if( m_recBlockAlloc == NULL )
		m_recBlockAlloc = (u8*)_aligned_malloc( m_recBlockAllocSize, 4096 );

	if( m_recBlockAlloc == NULL )
		throw Exception::OutOfMemory( L"R3000A BASEBLOCK lookup tables" );

	u8* curpos = m_recBlockAlloc;
	recRAM = (BASEBLOCK*)curpos; curpos += (Ps2MemSize::IopRam / 4) * sizeof(BASEBLOCK);
	recROM = (BASEBLOCK*)curpos; curpos += (Ps2MemSize::Rom / 4) * sizeof(BASEBLOCK);
	recROM1 = (BASEBLOCK*)curpos; curpos += (Ps2MemSize::Rom1 / 4) * sizeof(BASEBLOCK);

	if( s_pInstCache == NULL )
	{
		s_nInstCacheSize = 128;
		s_pInstCache = (EEINST*)malloc( sizeof(EEINST) * s_nInstCacheSize );
	}

	if( s_pInstCache == NULL )
		throw Exception::OutOfMemory( L"R3000 InstCache." );

	_DynGen_Dispatchers();
}

void recResetIOP()
{
	DevCon.WriteLn( "iR3000A Recompiler reset." );

	recAlloc();
	recMem->Reset();

	iopClearRecLUT((BASEBLOCK*)m_recBlockAlloc,
		(((Ps2MemSize::IopRam + Ps2MemSize::Rom + Ps2MemSize::Rom1) / 4)));

	for (int i = 0; i < 0x10000; i++)
		recLUT_SetPage(psxRecLUT, 0, 0, 0, i, 0);

	// IOP knows 64k pages, hence for the 0x10000's

	// The bottom 2 bits of PC are always zero, so we <<14 to "compress"
	// the pc indexer into it's lower common denominator.

	// We're only mapping 20 pages here in 4 places.
	// 0x80 comes from : (Ps2MemSize::IopRam / 0x10000) * 4

	for (int i=0; i<0x80; i++)
	{
		recLUT_SetPage(psxRecLUT, psxhwLUT, recRAM, 0x0000, i, i & 0x1f);
		recLUT_SetPage(psxRecLUT, psxhwLUT, recRAM, 0x8000, i, i & 0x1f);
		recLUT_SetPage(psxRecLUT, psxhwLUT, recRAM, 0xa000, i, i & 0x1f);
	}

	for (int i=0x1fc0; i<0x2000; i++)
	{
		recLUT_SetPage(psxRecLUT, psxhwLUT, recROM, 0x0000, i, i - 0x1fc0);
		recLUT_SetPage(psxRecLUT, psxhwLUT, recROM, 0x8000, i, i - 0x1fc0);
		recLUT_SetPage(psxRecLUT, psxhwLUT, recROM, 0xa000, i, i - 0x1fc0);
	}

	for (int i=0x1e00; i<0x1e04; i++)
	{
		recLUT_SetPage(psxRecLUT, psxhwLUT, recROM1, 0x0000, i, i - 0x1fc0);
		recLUT_SetPage(psxRecLUT, psxhwLUT, recROM1, 0x8000, i, i - 0x1fc0);
		recLUT_SetPage(psxRecLUT, psxhwLUT, recROM1, 0xa000, i, i - 0x1fc0);
	}

	if( s_pInstCache )
		memset( s_pInstCache, 0, sizeof(EEINST)*s_nInstCacheSize );

	recBlocks.Reset();
	g_psxMaxRecMem = 0;

	recPtr = *recMem;
	psxbranch = 0;
}

static void recShutdown()
{
	safe_delete( recMem );

	safe_aligned_free( m_recBlockAlloc );

	safe_free( s_pInstCache );
	s_nInstCacheSize = 0;
}

static void iopClearRecLUT(BASEBLOCK* base, int count)
{
	for (int i = 0; i < count; i++)
		base[i].SetFnptr((uptr)iopJITCompile);
}

static void recExecute()
{
	// note: this function is currently never used.
	//for (;;) R3000AExecute();
}

static __noinline s32 recExecuteBlock( s32 eeCycles )
{
	iopBreak = 0;
	iopCycleEE = eeCycles;

#ifdef PCSX2_DEVBUILD
	//if (SysTrace.SIF.IsActive())
	//	SysTrace.IOP.R3000A.Write("Switching to IOP CPU for %d cycles", eeCycles);
#endif

	// [TODO] recExecuteBlock could be replaced by a direct call to the iopEnterRecompiledCode()
	//   (by assigning its address to the psxRec structure).  But for that to happen, we need
	//   to move iopBreak/iopCycleEE update code to emitted assembly code. >_<  --air

	// Likely Disasm, as borrowed from MSVC:

// Entry:
// 	mov         eax,dword ptr [esp+4]
// 	mov         dword ptr [iopBreak (0E88DCCh)],0
// 	mov         dword ptr [iopCycleEE (832A84h)],eax

// Exit:
// 	mov         ecx,dword ptr [iopBreak (0E88DCCh)]
// 	mov         edx,dword ptr [iopCycleEE (832A84h)]
// 	lea         eax,[edx+ecx]

	iopEnterRecompiledCode();

	return iopBreak + iopCycleEE;
}

// Returns the offset to the next instruction after any cleared memory
static __fi u32 psxRecClearMem(u32 pc)
{
	BASEBLOCK* pblock;

	pblock = PSX_GETBLOCK(pc);
	// if ((u8*)iopJITCompile == pblock->GetFnptr())
	if (pblock->GetFnptr() == (uptr)iopJITCompile)
		return 4;

	pc = HWADDR(pc);

	u32 lowerextent = pc, upperextent = pc + 4;
	int blockidx = recBlocks.Index(pc);
	pxAssert(blockidx != -1);

	while (BASEBLOCKEX* pexblock = recBlocks[blockidx - 1]) {
		if (pexblock->startpc + pexblock->size * 4 <= lowerextent)
			break;

		lowerextent = min(lowerextent, pexblock->startpc);
		blockidx--;
	}

	int toRemoveFirst = blockidx;

	while (BASEBLOCKEX* pexblock = recBlocks[blockidx]) {
		if (pexblock->startpc >= upperextent)
			break;

		lowerextent = min(lowerextent, pexblock->startpc);
		upperextent = max(upperextent, pexblock->startpc + pexblock->size * 4);

		blockidx++;
	}

	if(toRemoveFirst != blockidx) {
		recBlocks.Remove(toRemoveFirst, (blockidx - 1));
	}

	blockidx=0;
	while(BASEBLOCKEX* pexblock = recBlocks[blockidx++])
	{
		if (pc >= pexblock->startpc && pc < pexblock->startpc + pexblock->size * 4) {
			DevCon.Error("Impossible block clearing failure");
			pxFailDev( "Impossible block clearing failure" );
		}
	}

	iopClearRecLUT(PSX_GETBLOCK(lowerextent), (upperextent - lowerextent) / 4);

	return upperextent - pc;
}

static __fi void recClearIOP(u32 Addr, u32 Size)
{
	u32 pc = Addr;
	while (pc < Addr + Size*4)
		pc += PSXREC_CLEARM(pc);
}

void psxSetBranchReg(u32 reg)
{
	psxbranch = 1;

	if( reg != 0xffffffff ) {
		_allocX86reg(ESI, X86TYPE_PCWRITEBACK, 0, MODE_WRITE);
		_psxMoveGPRtoR(ESI, reg);

		psxRecompileNextInstruction(1);

		if( x86regs[ESI].inuse ) {
			pxAssert( x86regs[ESI].type == X86TYPE_PCWRITEBACK );
			MOV32RtoM((uptr)&psxRegs.pc, ESI);
			x86regs[ESI].inuse = 0;
			#ifdef PCSX2_DEBUG
			xOR( esi, esi );
			#endif
		}
		else {
			MOV32MtoR(EAX, (uptr)&g_recWriteback);
			MOV32RtoM((uptr)&psxRegs.pc, EAX);

			#ifdef PCSX2_DEBUG
			xOR( eax, eax );
			#endif
		}

		#ifdef PCSX2_DEBUG
		xForwardJNZ8 skipAssert;
		xWrite8( 0xcc );
		skipAssert.SetTarget();
		#endif
	}

	_psxFlushCall(FLUSH_EVERYTHING);
	iPsxBranchTest(0xffffffff, 1);

	JMP32((uptr)iopDispatcherReg - ( (uptr)x86Ptr + 5 ));
}

void psxSetBranchImm( u32 imm )
{
	psxbranch = 1;
	pxAssert( imm );

	// end the current block
	MOV32ItoM( (uptr)&psxRegs.pc, imm );
	_psxFlushCall(FLUSH_EVERYTHING);
	iPsxBranchTest(imm, imm <= psxpc);

	recBlocks.Link(HWADDR(imm), xJcc32());
}

static __fi u32 psxScaleBlockCycles()
{
	return s_psxBlockCycles;
}

static void iPsxBranchTest(u32 newpc, u32 cpuBranch)
{
	u32 blockCycles = psxScaleBlockCycles();

	if (EmuConfig.Speedhacks.WaitLoop && s_nBlockFF && newpc == s_branchTo)
	{
		xMOV(eax, ptr32[&psxRegs.cycle]);
		xMOV(ecx, eax);
		xMOV(edx, ptr32[&iopCycleEE]);
		xADD(edx, 7);
		xSHR(edx, 3);
		xADD(eax, edx);
		xCMP(eax, ptr32[&g_iopNextEventCycle]);
		xCMOVNS(eax, ptr32[&g_iopNextEventCycle]);
		xMOV(ptr32[&psxRegs.cycle], eax);
		xSUB(eax, ecx);
		xSHL(eax, 3);
		xSUB(ptr32[&iopCycleEE], eax);
		xJLE(iopExitRecompiledCode);

		xCALL(iopEventTest);

		if( newpc != 0xffffffff )
		{
			xCMP(ptr32[&psxRegs.pc], newpc);
			xJNE(iopDispatcherReg);
		}
	}
	else
	{
		xMOV(eax, ptr32[&psxRegs.cycle]);
		xADD(eax, blockCycles);
		xMOV(ptr32[&psxRegs.cycle], eax); // update cycles

		// jump if iopCycleEE <= 0  (iop's timeslice timed out, so time to return control to the EE)
		xSUB(ptr32[&iopCycleEE], blockCycles*8);
		xJLE(iopExitRecompiledCode);

		// check if an event is pending
		xSUB(eax, ptr32[&g_iopNextEventCycle]);
		xForwardJS<u8> nointerruptpending;

		xCALL(iopEventTest);

		if( newpc != 0xffffffff ) {
			xCMP(ptr32[&psxRegs.pc], newpc);
			xJNE(iopDispatcherReg);
		}

		nointerruptpending.SetTarget();
	}
}

#if 0
//static const int *s_pCode;

#if !defined(_MSC_VER)
static void checkcodefn()
{
	int pctemp;

#ifdef _MSC_VER
	__asm mov pctemp, eax;
#else
    __asm__ __volatile__("movl %%eax, %[pctemp]" : [pctemp]"m="(pctemp) );
#endif
	Console.WriteLn("iop code changed! %x", pctemp);
}
#endif
#endif

void rpsxSYSCALL()
{
	MOV32ItoM( (uptr)&psxRegs.code, psxRegs.code );
	MOV32ItoM((uptr)&psxRegs.pc, psxpc - 4);
	_psxFlushCall(FLUSH_NODESTROY);

	xMOV( ecx, 0x20 );			// exception code
	xMOV( edx, psxbranch==1 );	// branch delay slot?
	xCALL( psxException );

	CMP32ItoM((uptr)&psxRegs.pc, psxpc-4);
	j8Ptr[0] = JE8(0);

	ADD32ItoM((uptr)&psxRegs.cycle, psxScaleBlockCycles() );
	SUB32ItoM((uptr)&iopCycleEE, psxScaleBlockCycles()*8 );
	JMP32((uptr)iopDispatcherReg - ( (uptr)x86Ptr + 5 ));

	// jump target for skipping blockCycle updates
	x86SetJ8(j8Ptr[0]);

	//if (!psxbranch) psxbranch = 2;
}

void rpsxBREAK()
{
	MOV32ItoM( (uptr)&psxRegs.code, psxRegs.code );
	MOV32ItoM((uptr)&psxRegs.pc, psxpc - 4);
	_psxFlushCall(FLUSH_NODESTROY);

	xMOV( ecx, 0x24 );			// exception code
	xMOV( edx, psxbranch==1 );	// branch delay slot?
	xCALL( psxException );

	CMP32ItoM((uptr)&psxRegs.pc, psxpc-4);
	j8Ptr[0] = JE8(0);
	ADD32ItoM((uptr)&psxRegs.cycle, psxScaleBlockCycles() );
	SUB32ItoM((uptr)&iopCycleEE, psxScaleBlockCycles()*8 );
	JMP32((uptr)iopDispatcherReg - ( (uptr)x86Ptr + 5 ));
	x86SetJ8(j8Ptr[0]);

	//if (!psxbranch) psxbranch = 2;
}

void psxRecompileNextInstruction(int delayslot)
{
	static u8 s_bFlushReg = 1;

	// pblock isn't used elsewhere in this function.
	//BASEBLOCK* pblock = PSX_GETBLOCK(psxpc);

	if( IsDebugBuild )
		MOV32ItoR(EAX, psxpc);

	psxRegs.code = iopMemRead32( psxpc );
	s_psxBlockCycles++;
	psxpc += 4;

	g_pCurInstInfo++;

	g_iopCyclePenalty = 0;
	rpsxBSC[ psxRegs.code >> 26 ]();
	s_psxBlockCycles += g_iopCyclePenalty;

	if( !delayslot ) {
		if( s_bFlushReg ) {
			//_psxFlushUnusedConstReg();
		}
		else s_bFlushReg = 1;
	}
	else s_bFlushReg = 1;

	_clearNeededX86regs();
}

static void __fastcall  PreBlockCheck( u32 blockpc )
{
#ifdef PCSX2_DEBUG
	extern void iDumpPsxRegisters(u32 startpc, u32 temp);

	static int lastrec = 0;
	static int curcount = 0;
	const int skip = 0;

    //*(int*)PSXM(0x27990) = 1; // enables cdvd bios output for scph10000

    if( (psxdump&2) && lastrec != blockpc )
    {
		curcount++;

		if( curcount > skip ) {
			iDumpPsxRegisters(blockpc, 1);
			curcount = 0;
		}

		lastrec = blockpc;
	}
#endif
}

static void __fastcall iopRecRecompile( const u32 startpc )
{
	u32 i;
	u32 willbranch3 = 0;

	if( IsDebugBuild && (psxdump & 4) )
	{
		extern void iDumpPsxRegisters(u32 startpc, u32 temp);
		iDumpPsxRegisters(startpc, 0);
	}

	pxAssert( startpc );

	// if recPtr reached the mem limit reset whole mem
	if (recPtr >= (recMem->GetPtrEnd() - _64kb)) {
		recResetIOP();
	}

	x86SetPtr( recPtr );
	x86Align(16);
	recPtr = x86Ptr;

	s_pCurBlock = PSX_GETBLOCK(startpc);

	pxAssert(s_pCurBlock->GetFnptr() == (uptr)iopJITCompile
		|| s_pCurBlock->GetFnptr() == (uptr)iopJITCompileInBlock);

	s_pCurBlockEx = recBlocks.Get(HWADDR(startpc));

	if(!s_pCurBlockEx || s_pCurBlockEx->startpc != HWADDR(startpc))
		s_pCurBlockEx = recBlocks.New(HWADDR(startpc), (uptr)recPtr);

	psxbranch = 0;

	s_pCurBlock->SetFnptr( (uptr)x86Ptr );
	s_psxBlockCycles = 0;

	// reset recomp state variables
	psxpc = startpc;
	g_psxHasConstReg = g_psxFlushedConstReg = 1;

	_initX86regs();

	if( IsDebugBuild )
	{
		xMOV(ecx, psxpc);
		xCALL(PreBlockCheck);
	}

	// go until the next branch
	i = startpc;
	s_nEndBlock = 0xffffffff;
	s_branchTo = -1;

	while(1) {
		BASEBLOCK* pblock = PSX_GETBLOCK(i);
		if (i != startpc
		 && pblock->GetFnptr() != (uptr)iopJITCompile
		 && pblock->GetFnptr() != (uptr)iopJITCompileInBlock) {
			// branch = 3
			willbranch3 = 1;
			s_nEndBlock = i;
			break;
		}

		psxRegs.code = iopMemRead32(i);

		switch(psxRegs.code >> 26) {
			case 0: // special

				if( _Funct_ == 8 || _Funct_ == 9 ) { // JR, JALR
					s_nEndBlock = i + 8;
					goto StartRecomp;
				}

				break;
			case 1: // regimm

				if( _Rt_ == 0 || _Rt_ == 1 || _Rt_ == 16 || _Rt_ == 17 ) {

					s_branchTo = _Imm_ * 4 + i + 4;
					if( s_branchTo > startpc && s_branchTo < i ) s_nEndBlock = s_branchTo;
					else  s_nEndBlock = i+8;

					goto StartRecomp;
				}

				break;

			case 2: // J
			case 3: // JAL
				s_branchTo = _Target_ << 2 | (i + 4) & 0xf0000000;
				s_nEndBlock = i + 8;
				goto StartRecomp;

			// branches
			case 4: case 5: case 6: case 7:

				s_branchTo = _Imm_ * 4 + i + 4;
				if( s_branchTo > startpc && s_branchTo < i ) s_nEndBlock = s_branchTo;
				else  s_nEndBlock = i+8;

				goto StartRecomp;
		}

		i += 4;
	}

StartRecomp:

	s_nBlockFF = false;
	if (s_branchTo == startpc) {
		s_nBlockFF = true;
		for (i = startpc; i < s_nEndBlock; i += 4) {
			if (i != s_nEndBlock - 8) {
				switch (iopMemRead32(i)) {
					case 0: // nop
						break;
					default:
						s_nBlockFF = false;
				}
			}
		}
	}

	// rec info //
	{
		EEINST* pcur;

		if( s_nInstCacheSize < (s_nEndBlock-startpc)/4+1 ) {
			free(s_pInstCache);
			s_nInstCacheSize = (s_nEndBlock-startpc)/4+10;
			s_pInstCache = (EEINST*)malloc(sizeof(EEINST)*s_nInstCacheSize);
			pxAssert( s_pInstCache != NULL );
		}

		pcur = s_pInstCache + (s_nEndBlock-startpc)/4;
		_recClearInst(pcur);
		pcur->info = 0;

		for(i = s_nEndBlock; i > startpc; i -= 4 ) {
			psxRegs.code = iopMemRead32(i-4);
			pcur[-1] = pcur[0];
			rpsxpropBSC(pcur-1, pcur);
			pcur--;
		}
	}

	// dump code
	if( IsDebugBuild )
	{
		for(i = 0; i < ArraySize(s_psxrecblocks); ++i) {
		if( startpc == s_psxrecblocks[i] ) {
			iIopDumpBlock(startpc, recPtr);
			}
		}

		if( (psxdump & 1) )
			iIopDumpBlock(startpc, recPtr);
	}

	g_pCurInstInfo = s_pInstCache;
	while (!psxbranch && psxpc < s_nEndBlock) {
		psxRecompileNextInstruction(0);
	}

	if( IsDebugBuild && (psxdump & 1) )
		iIopDumpBlock(startpc, recPtr);

	pxAssert( (psxpc-startpc)>>2 <= 0xffff );
	s_pCurBlockEx->size = (psxpc-startpc)>>2;

	for(i = 1; i < (u32)s_pCurBlockEx->size; ++i) {
		if (s_pCurBlock[i].GetFnptr() == (uptr)iopJITCompile)
			s_pCurBlock[i].SetFnptr((uptr)iopJITCompileInBlock);
	}

	if( !(psxpc&0x10000000) )
		g_psxMaxRecMem = std::max( (psxpc&~0xa0000000), g_psxMaxRecMem );

	if( psxbranch == 2 ) {
		_psxFlushCall(FLUSH_EVERYTHING);

		iPsxBranchTest(0xffffffff, 1);

		JMP32((uptr)iopDispatcherReg - ( (uptr)x86Ptr + 5 ));
	}
	else {
		if( psxbranch ) pxAssert( !willbranch3 );
		else
		{
			ADD32ItoM((uptr)&psxRegs.cycle, psxScaleBlockCycles() );
			SUB32ItoM((uptr)&iopCycleEE, psxScaleBlockCycles()*8 );
		}

		if (willbranch3 || !psxbranch) {
			pxAssert( psxpc == s_nEndBlock );
			_psxFlushCall(FLUSH_EVERYTHING);
			MOV32ItoM((uptr)&psxRegs.pc, psxpc);
			recBlocks.Link(HWADDR(s_nEndBlock), xJcc32() );
			psxbranch = 3;
		}
	}

	pxAssert( xGetPtr() < recMem->GetPtrEnd() );

	pxAssert(xGetPtr() - recPtr < _64kb);
	s_pCurBlockEx->x86size = xGetPtr() - recPtr;

	recPtr = xGetPtr();

	pxAssert( (g_psxHasConstReg&g_psxFlushedConstReg) == g_psxHasConstReg );

	s_pCurBlock = NULL;
	s_pCurBlockEx = NULL;
}

static void recSetCacheReserve( uint reserveInMegs )
{
	m_ConfiguredCacheReserve = reserveInMegs;
}

static uint recGetCacheReserve()
{
	return m_ConfiguredCacheReserve;
}

R3000Acpu psxRec = {
	recReserve,
	recResetIOP,
	recExecute,
	recExecuteBlock,
	recClearIOP,
	recShutdown,
	
	recGetCacheReserve,
	recSetCacheReserve
};

