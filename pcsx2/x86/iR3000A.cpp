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
 
// recompiler reworked to add dynamic linking Jan06
// and added reg caching, const propagation, block analysis Jun06
// zerofrog(@gmail.com)


#include "PrecompiledHeader.h"

#include "iR3000A.h"
#include "BaseblockEx.h"

#include <time.h>

#ifndef _WIN32
#include <sys/types.h>
#endif

#include "IopCommon.h"
#include "VU.h"
#include "iCore.h"

#include "SamplProf.h"
#include "NakedAsm.h"
#include "AppConfig.h"


using namespace x86Emitter;

extern u32 g_psxNextBranchCycle;
extern void psxBREAK();
extern void zeroEx();

u32 g_psxMaxRecMem = 0;
u32 s_psxrecblocks[] = {0};

uptr psxRecLUT[0x10000];
uptr psxhwLUT[0x10000];

#define HWADDR(mem) (psxhwLUT[mem >> 16] + (mem))

#define MAPBASE			0x48000000
#define RECMEM_SIZE		(8*1024*1024)

// R3000A statics
int psxreclog = 0;

#ifdef _MSC_VER

static u32 g_temp;

// The address for all cleared blocks.  It recompiles the current pc and then
// dispatches to the recompiled block address.
static __declspec(naked) void iopJITCompile()
{
	__asm {
		mov esi, dword ptr [psxRegs.pc]
		push esi
		call iopRecRecompile
		add esp, 4
		mov ebx, esi
		shr esi, 16
		mov ecx, dword ptr [psxRecLUT+esi*4]
		jmp dword ptr [ecx+ebx]
	}
}

static __declspec(naked) void iopJITCompileInBlock()
{
	__asm {
		jmp iopJITCompile
	}
}

// called when jumping to variable psxpc address
static __declspec(naked) void iopDispatcherReg()
{
	__asm {
		mov eax, dword ptr [psxRegs.pc]
		mov ebx, eax
		shr eax, 16
		mov ecx, dword ptr [psxRecLUT+eax*4]
		jmp dword ptr [ecx+ebx]
	}
}
#endif // _MSC_VER


static u8 *recMem = NULL;	// the recompiled blocks will be here
static BASEBLOCK *recRAM = NULL;	// and the ptr to the blocks here
static BASEBLOCK *recROM = NULL;	// and here
static BASEBLOCK *recROM1 = NULL;	// also here
static BaseBlocks recBlocks((uptr)iopJITCompile);
static u8 *recPtr = NULL;
u32 psxpc;			// recompiler psxpc
int psxbranch;		// set for branch
u32 g_iopCyclePenalty;

static EEINST* s_pInstCache = NULL;
static u32 s_nInstCacheSize = 0;

static BASEBLOCK* s_pCurBlock = NULL;
static BASEBLOCKEX* s_pCurBlockEx = NULL;

static u32 s_nEndBlock = 0; // what psxpc the current block ends

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

////////////////////////////////////////////////////
using namespace R3000A;
#include "Utilities/AsciiFile.h"

static void iIopDumpBlock( int startpc, u8 * ptr )
{
	u32 i, j;
	EEINST* pcur;
	u8 used[34];
	int numused, count;

	Console::WriteLn( "dump1 %x:%x, %x", startpc, psxpc, psxRegs.cycle );
	g_Conf->Folders.Logs.Mkdir();

	wxString filename( Path::Combine( g_Conf->Folders.Logs, wxsFormat( L"psxdump%.8X.txt", startpc ) ) );
	AsciiFile f( filename, wxFile::write );

	/*for ( i = startpc; i < s_nEndBlock; i += 4 ) {
		f.Printf("%s\n", disR3000Fasm( iopMemRead32( i ), i ) );
	}*/

	// write the instruction info
	f.Printf("\n\nlive0 - %x, lastuse - %x used - %x\n", EEINST_LIVE0, EEINST_LASTUSE, EEINST_USED);

	memzero_obj(used);
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
		AsciiFile f2( L"mydump1", wxFile::write );
		f2.Write( ptr, (uptr)x86Ptr - (uptr)ptr );
	}
	wxCharBuffer buf( filename.ToAscii() );
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
	_freeX86regs();

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
        if( (psxRegs.code>>26) == 9 ) {
            //ADDIU, call bios
			if( IsDebugBuild )
			{
				MOV32ItoM( (uptr)&psxRegs.code, psxRegs.code );
				MOV32ItoM( (uptr)&psxRegs.pc, psxpc );
				_psxFlushCall(FLUSH_NODESTROY);
				CALLFunc((uptr)zeroEx);
			}
			// Bios Call: Force the IOP to do a Branch Test ASAP.
			// Important! This helps prevent game freeze-ups during boot-up and stage loads.
			// Note: Fixes to cdvd have removed the need for this code.
			//MOV32MtoR( EAX, (uptr)&psxRegs.cycle );
			//MOV32RtoM( (uptr)&g_psxNextBranchCycle, EAX );
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

static u8* m_recBlockAlloc = NULL;

static const uint m_recBlockAllocSize =
	(((Ps2MemSize::IopRam + Ps2MemSize::Rom + Ps2MemSize::Rom1) / 4) * sizeof(BASEBLOCK));

static void recAlloc()
{
	// Note: the VUrec depends on being able to grab an allocation below the 0x10000000 line,
	// so we give the EErec an address above that to try first as it's basemem address, hence
	// the 0x28000000 pick (0x20000000 is picked by the EE)

	if( recMem == NULL )
		recMem = (u8*)SysMmapEx( 0x28000000, RECMEM_SIZE, 0, "recAlloc(R3000a)" );

	if( recMem == NULL )
		throw Exception::OutOfMemory( "R3000a Init > failed to allocate memory for the recompiler." );

	// Goal: Allocate BASEBLOCKs for every possible branch target in IOP memory.
	// Any 4-byte aligned address makes a valid branch target as per MIPS design (all instructions are
	// always 4 bytes long).

	if( m_recBlockAlloc == NULL )
		m_recBlockAlloc = (u8*)_aligned_malloc( m_recBlockAllocSize, 4096 );

	if( m_recBlockAlloc == NULL )
		throw Exception::OutOfMemory( "R3000a Init > Failed to allocate memory for baseblock lookup tables." );

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
		throw Exception::OutOfMemory( "R3000a Init > Failed to allocate memory for pInstCache." );

	ProfilerRegisterSource( "IOPRec", recMem, RECMEM_SIZE );
}

void recResetIOP()
{
	// calling recResetIOP without first calling recInit is bad mojo.
	jASSUME( recMem != NULL );
	jASSUME( m_recBlockAlloc != NULL );

	DevCon::Status( "iR3000A Resetting recompiler memory and structures" );

	memset_8<0xcc,RECMEM_SIZE>( recMem );	// 0xcc is INT3
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

	recPtr = recMem;
	psxbranch = 0;
}

static void recShutdown()
{
	ProfilerTerminateSource( "IOPRec" );

	SafeSysMunmap(recMem, RECMEM_SIZE);
	safe_aligned_free( m_recBlockAlloc );

	safe_free( s_pInstCache );
	s_nInstCacheSize = 0;
}

#pragma warning(disable:4731) // frame pointer register 'ebp' modified by inline assembly code

u32 g_psxlastpc = 0;

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

static __forceinline s32 recExecuteBlock( s32 eeCycles )
{
	psxBreak = 0;
	psxCycleEE = eeCycles;

	// Register freezing note:
	//  The IOP does not use mmx/xmm registers, so we don't modify the status
	//  of the g_EEFreezeRegs here.

#ifdef _MSC_VER
	__asm
	{
		push ebx
		push esi
		push edi
		push ebp

		call iopDispatcherReg

		pop ebp
		pop edi
		pop esi
		pop ebx
	}
#else
	__asm__
	(
		".intel_syntax noprefix\n"
		"push ebx\n"
		"push esi\n"
		"push edi\n"
		"push ebp\n"

		"call iopDispatcherReg\n"

		"pop ebp\n"
		"pop edi\n"
		"pop esi\n"
		"pop ebx\n"
		".att_syntax\n"
	);
#endif

	return psxBreak + psxCycleEE;
}

// Returns the offset to the next instruction after any cleared memory
static __forceinline u32 psxRecClearMem(u32 pc)
{
	BASEBLOCKEX* pexblock;
	BASEBLOCK* pblock;

	pblock = PSX_GETBLOCK(pc);
	// if ((u8*)iopJITCompile == pblock->GetFnptr())
	if (pblock->GetFnptr() == (uptr)iopJITCompile)
		return 4;

	pc = HWADDR(pc);

	u32 lowerextent = pc, upperextent = pc + 4;
	int blockidx = recBlocks.Index(pc);

	jASSUME(blockidx != -1);

	// Variable assignment in the middle of the while statements condition?
	while (pexblock = recBlocks[blockidx - 1]) {
		if (pexblock->startpc + pexblock->size * 4 <= lowerextent)
			break;

		lowerextent = min(lowerextent, pexblock->startpc);
		blockidx--;
	}

	// Same here.
	while (pexblock = recBlocks[blockidx]) {
		if (pexblock->startpc >= upperextent)
			break;

		lowerextent = min(lowerextent, pexblock->startpc);
		upperextent = max(upperextent, pexblock->startpc + pexblock->size * 4);
		recBlocks.Remove(blockidx);
	}

#ifdef PCSX2_DEVBUILD
	for (int i = 0; pexblock = recBlocks[i]; i++)
		if (pc >= pexblock->startpc && pc < pexblock->startpc + pexblock->size * 4) {
			Console::Error("Impossible block clearing failure");
			jASSUME(0);
		}
#endif

	iopClearRecLUT(PSX_GETBLOCK(lowerextent), (upperextent - lowerextent) / 4);

	return upperextent - pc;
}

static __forceinline void recClearIOP(u32 Addr, u32 Size)
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
			assert( x86regs[ESI].type == X86TYPE_PCWRITEBACK );
			MOV32RtoM((uptr)&psxRegs.pc, ESI);
			x86regs[ESI].inuse = 0;
		}
		else {
			MOV32MtoR(EAX, (uptr)&g_recWriteback);
			MOV32RtoM((uptr)&psxRegs.pc, EAX);
		}
	}

	_psxFlushCall(FLUSH_EVERYTHING);
	iPsxBranchTest(0xffffffff, 1);

	JMP32((uptr)iopDispatcherReg - ( (uptr)x86Ptr + 5 ));
}

void psxSetBranchImm( u32 imm )
{
	psxbranch = 1;
	assert( imm );

	// end the current block
	MOV32ItoM( (uptr)&psxRegs.pc, imm );
	_psxFlushCall(FLUSH_EVERYTHING);
	iPsxBranchTest(imm, imm <= psxpc);

	recBlocks.Link(HWADDR(imm), xJcc32());
}

static __forceinline u32 psxScaleBlockCycles()
{
	return s_psxBlockCycles * (EmuConfig.Speedhacks.IopCycleRate_X2 ? 2 : 1);
}

static void iPsxBranchTest(u32 newpc, u32 cpuBranch)
{
	u32 blockCycles = psxScaleBlockCycles();

	MOV32MtoR(ECX, (uptr)&psxRegs.cycle);
	MOV32MtoR(EAX, (uptr)&psxCycleEE);
	ADD32ItoR(ECX, blockCycles);
	SUB32ItoR(EAX, blockCycles*8);
	MOV32RtoM((uptr)&psxRegs.cycle, ECX); // update cycles
	MOV32RtoM((uptr)&psxCycleEE, EAX);

	j8Ptr[2] = JG8( 0 );	// jump if psxCycleEE > 0

	RET();		// returns control to the EE

	// Continue onward with branching here:
	x86SetJ8( j8Ptr[2] );

	// check if an event is pending
	SUB32MtoR(ECX, (uptr)&g_psxNextBranchCycle);
	j8Ptr[0] = JS8( 0 );

	CALLFunc((uptr)psxBranchTest);

	if( newpc != 0xffffffff )
	{
		CMP32ItoM((uptr)&psxRegs.pc, newpc);
		JNE32((uptr)iopDispatcherReg - ( (uptr)x86Ptr + 6 ));
	}

	// Skip branch jump target here:
	x86SetJ8( j8Ptr[0] );
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
    __asm__("movl %%eax, %[pctemp]" : : [pctemp]"m"(pctemp) );
#endif
	Console::WriteLn("iop code changed! %x", pctemp);
}
#endif
#endif

void rpsxSYSCALL()
{
	MOV32ItoM( (uptr)&psxRegs.code, psxRegs.code );
	MOV32ItoM((uptr)&psxRegs.pc, psxpc - 4);
	_psxFlushCall(FLUSH_NODESTROY);

	_callFunctionArg2((uptr)psxException, MEM_CONSTTAG, MEM_CONSTTAG, 0x20, psxbranch==1);

	CMP32ItoM((uptr)&psxRegs.pc, psxpc-4);
	j8Ptr[0] = JE8(0);

	ADD32ItoM((uptr)&psxRegs.cycle, psxScaleBlockCycles() );
	SUB32ItoM((uptr)&psxCycleEE, psxScaleBlockCycles()*8 );
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

	_callFunctionArg2((uptr)psxBREAK, MEM_CONSTTAG, MEM_CONSTTAG, 0x24, psxbranch==1);

	CMP32ItoM((uptr)&psxRegs.pc, psxpc-4);
	j8Ptr[0] = JE8(0);
	ADD32ItoM((uptr)&psxRegs.cycle, psxScaleBlockCycles() );
	SUB32ItoM((uptr)&psxCycleEE, psxScaleBlockCycles()*8 );
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

static void printfn()
{
#ifdef PCSX2_DEBUG
	extern void iDumpPsxRegisters(u32 startpc, u32 temp);

	static int lastrec = 0;
	static int curcount = 0;
	const int skip = 0;

    //*(int*)PSXM(0x27990) = 1; // enables cdvd bios output for scph10000

    if( (psxdump&2) && lastrec != g_psxlastpc )
    {
		curcount++;

		if( curcount > skip ) {
			iDumpPsxRegisters(g_psxlastpc, 1);
			curcount = 0;
		}

		lastrec = g_psxlastpc;
	}
#endif
}

void iopRecRecompile(u32 startpc)
{
	u32 i;
	u32 branchTo;
	u32 willbranch3 = 0;

	if( IsDebugBuild && (psxdump & 4) )
	{
		extern void iDumpPsxRegisters(u32 startpc, u32 temp);
		iDumpPsxRegisters(startpc, 0);
	}

	assert( startpc );

	// if recPtr reached the mem limit reset whole mem
	if (((uptr)recPtr - (uptr)recMem) >= (RECMEM_SIZE - 0x10000))
		recResetIOP();

	x86SetPtr( recPtr );
	x86Align(16);
	recPtr = x86Ptr;

	s_pCurBlock = PSX_GETBLOCK(startpc);

	assert(s_pCurBlock->GetFnptr() == (uptr)iopJITCompile
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
		MOV32ItoM((uptr)&g_psxlastpc, psxpc);
		CALLFunc((uptr)printfn);
	}

	// go until the next branch
	i = startpc;
	s_nEndBlock = 0xffffffff;

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

					branchTo = _Imm_ * 4 + i + 4;
					if( branchTo > startpc && branchTo < i ) s_nEndBlock = branchTo;
					else  s_nEndBlock = i+8;

					goto StartRecomp;
				}

				break;

			case 2: // J
			case 3: // JAL
				s_nEndBlock = i + 8;
				goto StartRecomp;

			// branches
			case 4: case 5: case 6: case 7:

				branchTo = _Imm_ * 4 + i + 4;
				if( branchTo > startpc && branchTo < i ) s_nEndBlock = branchTo;
				else  s_nEndBlock = i+8;

				goto StartRecomp;
		}

		i += 4;
	}

StartRecomp:

	// rec info //
	{
		EEINST* pcur;

		if( s_nInstCacheSize < (s_nEndBlock-startpc)/4+1 ) {
			free(s_pInstCache);
			s_nInstCacheSize = (s_nEndBlock-startpc)/4+10;
			s_pInstCache = (EEINST*)malloc(sizeof(EEINST)*s_nInstCacheSize);
			assert( s_pInstCache != NULL );
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

	assert( (psxpc-startpc)>>2 <= 0xffff );
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
		if( psxbranch ) assert( !willbranch3 );
		else
		{
			ADD32ItoM((uptr)&psxRegs.cycle, psxScaleBlockCycles() );
			SUB32ItoM((uptr)&psxCycleEE, psxScaleBlockCycles()*8 );
		}

		if (willbranch3 || !psxbranch) {
			assert( psxpc == s_nEndBlock );
			_psxFlushCall(FLUSH_EVERYTHING);
			MOV32ItoM((uptr)&psxRegs.pc, psxpc);
			recBlocks.Link(HWADDR(s_nEndBlock), xJcc32() );
			psxbranch = 3;
		}
	}

	assert( x86Ptr < recMem+RECMEM_SIZE );

	assert(x86Ptr - recPtr < 0x10000);
	s_pCurBlockEx->x86size = x86Ptr - recPtr;

	recPtr = x86Ptr;

	assert( (g_psxHasConstReg&g_psxFlushedConstReg) == g_psxHasConstReg );

	s_pCurBlock = NULL;
	s_pCurBlockEx = NULL;
}

R3000Acpu psxRec = {
	recAlloc,
	recResetIOP,
	recExecute,
	recExecuteBlock,
	recClearIOP,
	recShutdown
};

