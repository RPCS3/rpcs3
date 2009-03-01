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

// recompiler reworked to add dynamic linking Jan06
// and added reg caching, const propagation, block analysis Jun06
// zerofrog(@gmail.com)


#include "PrecompiledHeader.h"

#include "iR3000A.h"
#include <time.h>

#ifndef _WIN32
#include <sys/types.h>
#endif

#include "PsxCommon.h"
#include "VU.h"

#include "ix86/ix86.h"
#include "iCore.h"

#include "SamplProf.h"

extern u32 g_psxNextBranchCycle;
extern void psxBREAK();
extern void zeroEx();

u32 g_psxMaxRecMem = 0;
u32 s_psxrecblocks[] = {0};

//Using assembly code from an external file.
#ifdef __LINUX__
extern "C" {
#endif
void psxRecRecompile(u32 startpc);
#ifdef __LINUX__
}
#endif

uptr *psxRecLUT;

#define PSX_NUMBLOCKS (1<<12)
#define MAPBASE			0x48000000
#define RECMEM_SIZE		(8*1024*1024)

#define PSX_MEMMASK     0x5fffffff // mask when comparing two pcs

// R3000A statics
int psxreclog = 0;

static u8 *recMem = NULL;	// the recompiled blocks will be here
static BASEBLOCK *recRAM = NULL;	// and the ptr to the blocks here
static BASEBLOCK *recROM = NULL;	// and here
static BASEBLOCK *recROM1 = NULL;	// also here
static BASEBLOCKEX *recBlocks = NULL;
static u8 *recPtr = NULL;
u32 psxpc;			// recompiler psxpc
int psxbranch;		// set for branch
u32 g_iopCyclePenalty;

static EEINST* s_pInstCache = NULL;
static u32 s_nInstCacheSize = 0;

static BASEBLOCK* s_pCurBlock = NULL;
static BASEBLOCKEX* s_pCurBlockEx = NULL;

#if defined(_MSC_VER) && !defined(__x86_64__)
static BASEBLOCK* s_pDispatchBlock = NULL;
#endif

static u32 s_nEndBlock = 0; // what psxpc the current block ends	

static u32 s_nNextBlock = 0; // next free block in recBlocks

static u32 s_ConstGPRreg;
static u32 s_saveConstGPRreg = 0, s_saveHasConstReg = 0, s_saveFlushedConstReg = 0;
static EEINST* s_psaveInstInfo = NULL;

u32 s_psxBlockCycles = 0; // cycles of current block recompiling
static u32 s_savenBlockCycles = 0;

static void iPsxBranchTest(u32 newpc, u32 cpuBranch);
void psxRecompileNextInstruction(int delayslot);

extern void (*rpsxBSC[64])();
void rpsxpropBSC(EEINST* prev, EEINST* pinst);

#ifdef _DEBUG
u32 psxdump = 0;
#else
#define psxdump 0
#endif

#define PSX_GETBLOCK(x) PC_GETBLOCK_(x, psxRecLUT)

#define PSXREC_CLEARM(mem) { \
	if ((mem) < g_psxMaxRecMem && psxRecLUT[(mem) >> 16]) { \
		BASEBLOCK* p = PSX_GETBLOCK(mem); \
		if( *(u32*)p ) psxRecClearMem(p); \
	} \
} \

BASEBLOCKEX* PSX_GETBLOCKEX(BASEBLOCK* p)
{
//	BASEBLOCKEX* pex = *(BASEBLOCKEX**)(p+1);
//	if( pex >= recBlocks && pex < recBlocks+PSX_NUMBLOCKS )
//		return pex;

	// otherwise, use the sorted list
	return GetBaseBlockEx(p->startpc, 1);
}

////////////////////////////////////////////////////
#ifdef _DEBUG
using namespace R3000A;
static void iIopDumpBlock( int startpc, u8 * ptr )
{
	FILE *f;
#ifdef __LINUX__
	char command[256];
#endif
	u32 i, j;
	EEINST* pcur;
	u8 used[34];
	int numused, count;

	SysPrintf( "dump1 %x:%x, %x\n", startpc, psxpc, psxRegs.cycle );
	Path::CreateDirectory( "dumps" );

	string filename( Path::Combine( "dumps", fmt_string( "psxdump%.8X.txt", startpc ) ) );

	fflush( stdout );

	f = fopen( filename.c_str(), "w" );
	assert( f != NULL );
	for ( i = startpc; i < s_nEndBlock; i += 4 ) {
		fprintf( f, "%s\n", disR3000Fasm( iopMemRead32( i ), i ) );
	}

	// write the instruction info
	fprintf(f, "\n\nlive0 - %x, lastuse - %x used - %x\n", EEINST_LIVE0, EEINST_LASTUSE, EEINST_USED);

	memzero_obj(used);
	numused = 0;
	for(i = 0; i < ArraySize(s_pInstCache->regs); ++i) {
		if( s_pInstCache->regs[i] & EEINST_USED ) {
			used[i] = 1;
			numused++;
		}
	}

	fprintf(f, "       ");
	for(i = 0; i < ArraySize(s_pInstCache->regs); ++i) {
		if( used[i] ) fprintf(f, "%2d ", i);
	}
	fprintf(f, "\n");

	fprintf(f, "       ");
	for(i = 0; i < ArraySize(s_pInstCache->regs); ++i) {
		if( used[i] ) fprintf(f, "%s ", disRNameGPR[i]);
	}
	fprintf(f, "\n");

	pcur = s_pInstCache+1;
	for( i = 0; i < (s_nEndBlock-startpc)/4; ++i, ++pcur) {
		fprintf(f, "%2d: %2.2x ", i+1, pcur->info);
		
		count = 1;
		for(j = 0; j < ArraySize(s_pInstCache->regs); j++) {
			if( used[j] ) {
				fprintf(f, "%2.2x%s", pcur->regs[j], ((count%8)&&count<numused)?"_":" ");
				++count;
			}
		}
		fprintf(f, "\n");
	}
	fclose( f );

#ifdef __LINUX__
    // dump the asm
    f = fopen( "mydump1", "wb" );
	fwrite( ptr, 1, (uptr)x86Ptr - (uptr)ptr, f );
	fclose( f );
	sprintf( command, "objdump -D --target=binary --architecture=i386 -M intel mydump1 | cat %s - > tempdump", filename );
	system( command );
    sprintf(command, "mv tempdump %s", filename);
    system(command);
    f = fopen( filename, "a+" );
#endif
}
#endif

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
#if defined(_DEBUG)&&0
			else {
				// make sure the const regs are the same
				u8* ptemp;
				CMP32ItoM((uptr)&psxRegs.GPR.r[i], g_psxConstRegs[i]);
				ptemp = JE8(0);
				CALLFunc((uptr)checkconstreg);
				x86SetJ8( ptemp );
			}
#else
			if( g_psxHasConstReg == g_psxFlushedConstReg )
				break;
#endif
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
		MOV32ItoRmOffset( to, g_psxConstRegs[fromgpr], 0 );
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
	s_saveConstGPRreg = 0xffffffff; // indicate searching
	s_saveHasConstReg = g_psxHasConstReg;
	s_saveFlushedConstReg = g_psxFlushedConstReg;
	s_psaveInstInfo = g_pCurInstInfo;

	// save all regs
	memcpy(s_saveX86regs, x86regs, sizeof(x86regs));
}

void psxLoadBranchState()
{
	s_psxBlockCycles = s_savenBlockCycles;

	if( s_saveConstGPRreg != 0xffffffff ) {
		assert( s_saveConstGPRreg > 0 );

		// make sure right GPR was saved
		assert( g_psxHasConstReg == s_saveHasConstReg || (g_psxHasConstReg ^ s_saveHasConstReg) == (1<<s_saveConstGPRreg) );

		// restore the GPR reg
		g_psxConstRegs[s_saveConstGPRreg] = s_ConstGPRreg;
		PSX_SET_CONST(s_saveConstGPRreg);
		//s_saveConstGPRreg = 0;
	}

	g_psxHasConstReg = s_saveHasConstReg;
	g_psxFlushedConstReg = s_saveFlushedConstReg;
	g_pCurInstInfo = s_psaveInstInfo;

	// restore all regs
	memcpy(x86regs, s_saveX86regs, sizeof(x86regs));
}

////////////////////
// Code Templates //
////////////////////

void PSX_CHECK_SAVE_REG(int reg)
{
	if( s_saveConstGPRreg == 0xffffffff ) {
		if( PSX_IS_CONST1(reg) ) {
			s_saveConstGPRreg = reg;
			s_ConstGPRreg = g_psxConstRegs[reg];
		}
	}
	else {
		// can be non zero when double loading
		//assert( s_saveConstGPRreg == 0 );
	}
}

void _psxOnWriteReg(int reg)
{
	PSX_CHECK_SAVE_REG(reg);
	PSX_DEL_CONST(reg);
}

// rd = rs op rt
void psxRecompileCodeConst0(R3000AFNPTR constcode, R3000AFNPTR_INFO constscode, R3000AFNPTR_INFO consttcode, R3000AFNPTR_INFO noconstcode)
{
	if ( ! _Rd_ ) return;

	// for now, don't support xmm
	PSX_CHECK_SAVE_REG(_Rd_);

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
#ifdef _DEBUG
            MOV32ItoM( (uptr)&psxRegs.code, psxRegs.code );
	        MOV32ItoM( (uptr)&psxRegs.pc, psxpc );
            _psxFlushCall(FLUSH_NODESTROY);
            CALLFunc((uptr)zeroEx);
#endif
			// Bios Call: Force the IOP to do a Branch Test ASAP.
			// Important! This helps prevent game freeze-ups during boot-up and stage loads.
			// Note: Fixes to cdvd have removed the need for this code.
			//MOV32MtoR( EAX, (uptr)&psxRegs.cycle );
			//MOV32RtoM( (uptr)&g_psxNextBranchCycle, EAX );
		}
        return;
    }

	// for now, don't support xmm
	PSX_CHECK_SAVE_REG(_Rt_);

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
	PSX_CHECK_SAVE_REG(_Rd_);

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
		(((Ps2MemSize::IopRam + Ps2MemSize::Rom + Ps2MemSize::Rom1) / 4) * sizeof(BASEBLOCK))
	+	(PSX_NUMBLOCKS*sizeof(BASEBLOCKEX));	// recBlocks

static void recAlloc()
{
	// Note: the VUrec depends on being able to grab an allocation below the 0x10000000 line,
	// so we give the EErec an address above that to try first as it's basemem address, hence
	// the 0x28000000 pick (0x20000000 is picked by the EE)

	if( recMem == NULL )
		recMem = (u8*)SysMmapEx( 0x28000000, RECMEM_SIZE, 0, "recAlloc(R3000a)" );
	
	if( recMem == NULL )
		throw Exception::OutOfMemory( "R3000a Init > failed to allocate memory for the recompiler." );

	if( psxRecLUT == NULL )
		psxRecLUT = (uptr*) malloc(0x010000 * sizeof(uptr));

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
	recBlocks = (BASEBLOCKEX*)curpos;	// curpos += sizeof(BASEBLOCKEX)*EE_NUMBLOCKS;

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
	jASSUME( psxRecLUT != NULL );
	jASSUME( recMem != NULL );
	jASSUME( m_recBlockAlloc != NULL );

	DbgCon::Status( "iR3000A > Resetting recompiler memory and structures!" );

	memzero_ptr<0x010000 * sizeof(uptr)>( psxRecLUT );
	memset_8<0xcd,RECMEM_SIZE>( recMem );
	memzero_ptr<m_recBlockAllocSize>( m_recBlockAlloc );

	// We're only mapping 20 pages here in 4 places.
	// 0x80 comes from : (Ps2MemSize::IopRam / 0x10000) * 4
	for (int i=0; i<0x80; i++)
	{
		psxRecLUT[i + 0x0000] = (uptr)&recRAM[(i & 0x1f) << 14];
		psxRecLUT[i + 0x8000] = (uptr)&recRAM[(i & 0x1f) << 14];
		psxRecLUT[i + 0xa000] = (uptr)&recRAM[(i & 0x1f) << 14];
	}

	for (int i=0; i<(Ps2MemSize::Rom / 0x10000); i++)
	{
		psxRecLUT[i + 0x1fc0] = (uptr)&recROM[i << 14];
		psxRecLUT[i + 0x9fc0] = (uptr)&recROM[i << 14];
		psxRecLUT[i + 0xbfc0] = (uptr)&recROM[i << 14];
	}

	for (int i=0; i<(Ps2MemSize::Rom1 / 0x10000); i++)
	{
		psxRecLUT[i + 0x1e00] = (uptr)&recROM1[i << 14];
		psxRecLUT[i + 0x9e00] = (uptr)&recROM1[i << 14];
		psxRecLUT[i + 0xbe00] = (uptr)&recROM1[i << 14];
	}

	if( s_pInstCache )
		memset( s_pInstCache, 0, sizeof(EEINST)*s_nInstCacheSize );

	ResetBaseBlockEx(1);
	g_psxMaxRecMem = 0;

	recPtr = recMem;
	psxbranch = 0;
}

static void recShutdown()
{
	ProfilerTerminateSource( "IOPRec" );

	SafeSysMunmap(recMem, RECMEM_SIZE);
	safe_aligned_free( m_recBlockAlloc );

	safe_free(psxRecLUT);
	safe_free( s_pInstCache );
	s_nInstCacheSize = 0;

	x86Shutdown();
}

#pragma warning(disable:4731) // frame pointer register 'ebp' modified by inline assembly code

/*
static __forceinline void R3000AExecute()
{
	BASEBLOCK* pblock;

	pblock = PSX_GETBLOCK(psxRegs.pc);

	if ( !pblock->GetFnptr() || (pblock->startpc&PSX_MEMMASK) != (psxRegs.pc&PSX_MEMMASK) ) {
		psxRecRecompile(psxRegs.pc);
	}

	assert( pblock->GetFnptr() != 0 );

#ifdef _DEBUG

	fnptr = (u8*)pblock->GetFnptr();

#ifdef _MSC_VER

    __asm {
        // save data
        mov oldesi, esi;
        mov s_uSaveESP, esp;
        sub s_uSaveESP, 8;
        push ebp;
        
        call fnptr; // jump into function
        // restore data
        pop ebp;
        mov esi, oldesi;
    }
    
#else // linux
    
    __asm__("movl %%esi, %0\n"
            "movl %%esp, %1\n"
            "sub $8, %%esp\n"
            "push %%ebp\n"
            "call *%2\n"
            "pop %%ebp\n"
            "movl %0, %%esi\n" : "=m"(oldesi), "=m"(s_uSaveESP) : "c"(fnptr) : );
#endif // _MSC_VER
    
#else
    ((R3000AFNPTR)pblock->GetFnptr())();
#endif
}*/

u32 g_psxlastpc = 0;

#ifdef _MSC_VER

static u32 g_temp;

// jumped to when invalid psxpc address
__declspec(naked) void psxDispatcher()
{
	// EDX contains the current psxpc to jump to, stack contains the jump addr to modify
	__asm push edx

	//jASSUME( psxRegs.pc <= PSX_MEMMASK );
	s_pDispatchBlock = PSX_GETBLOCK( psxRegs.pc );

	if( s_pDispatchBlock->startpc != psxRegs.pc )
		psxRecRecompile(psxRegs.pc);

	__asm
	{
		mov eax, s_pDispatchBlock
		mov eax, dword ptr [eax]
	}

#ifdef _DEBUG
	__asm mov g_temp, eax
	assert( g_temp );
#endif

	// Modify the prev block's jump address, and jump to the new block:
	__asm
	{
		shl eax,4
		pop ecx // x86Ptr to mod
		mov edx, eax
		sub edx, ecx
		sub edx, 4
		mov dword ptr [ecx], edx

		jmp eax
	}
}

__declspec(naked) void psxDispatcherClear()
{
	// EDX contains the current psxpc
	__asm mov psxRegs.pc, edx
	__asm push edx

	//jASSUME( psxRegs.pc <= PSX_MEMMASK );

	// calc PSX_GETBLOCK
	s_pDispatchBlock = PSX_GETBLOCK(psxRegs.pc);

	if( s_pDispatchBlock->startpc == psxRegs.pc ) {
		assert( s_pDispatchBlock->GetFnptr() != 0 );

		// already modded the code, jump to the new place
		__asm
		{
			pop edx
			add esp, 4 // ignore stack
			mov eax, s_pDispatchBlock
			mov eax, dword ptr [eax]
			//and eax, 0x0fffffff
			shl eax,4
			jmp eax
		}
	}

	__asm
	{
		call psxRecRecompile
		add esp, 4 // pop old param
		mov eax, s_pDispatchBlock
		mov eax, dword ptr [eax]

		pop ecx // old fnptr

		//and eax, 0x0fffffff
		shl eax,4
		mov byte ptr [ecx], 0xe9 // jmp32
		mov edx, eax
		sub edx, ecx
		sub edx, 5
		mov dword ptr [ecx+1], edx

		jmp eax
	}
}

// called when jumping to variable psxpc address
__declspec(naked) void psxDispatcherReg()
{
	//jASSUME( psxRegs.pc <= PSX_MEMMASK );
	s_pDispatchBlock = PSX_GETBLOCK( psxRegs.pc );

	if( s_pDispatchBlock->startpc != psxRegs.pc )
		psxRecRecompile(psxRegs.pc);

	__asm
	{
		mov eax, s_pDispatchBlock
		mov eax, dword ptr [eax]
	}

#ifdef _DEBUG
	__asm mov g_temp, eax
	assert( g_temp );
#endif

	__asm
	{
		shl eax, 4
		jmp eax
	}
}

#else // _MSC_VER
// Linux uses an assembly version of these routines.
#ifdef __LINUX__
extern "C" {
#endif
void psxDispatcher();
void psxDispatcherClear();
void psxDispatcherReg();
#ifdef __LINUX__
}
#endif

#endif // _MSC_VER

static void recExecute()
{
	// note: this function is currently never used.
	//for (;;) R3000AExecute();
}

static s32 recExecuteBlock( s32 eeCycles )
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

		call psxDispatcherReg

		pop ebp
		pop edi
		pop esi
		pop ebx
	}
#else
	__asm__
	(
		".intel_syntax\n"
		"push %ebx\n"
		"push %esi\n"
		"push %edi\n"
		"push %ebp\n"

		"call psxDispatcherReg\n"
		
		"pop %ebp\n"
		"pop %edi\n"
		"pop %esi\n"
		"pop %ebx\n"
		".att_syntax\n"
	);
#endif

	return psxBreak + psxCycleEE;
}

static void recClear(u32 Addr, u32 Size)
{
	u32 i;
	for(i = 0; i < Size; ++i, Addr+=4) {
		PSXREC_CLEARM(Addr);
	}
}

#define IOP_MIN_BLOCK_BYTES 15

void rpsxMemConstClear(u32 mem)
{
	// NOTE! This assumes recLUT never changes its mapping
	if( !psxRecLUT[mem>>16] )
		return;

	CMP32ItoM((uptr)PSX_GETBLOCK(mem), 0);
	j8Ptr[6] = JE8(0);

    _callFunctionArg1((uptr)psxRecClearMem, MEM_CONSTTAG, (uptr)PSX_GETBLOCK(mem));
	x86SetJ8(j8Ptr[6]);
}

void psxRecClearMem(BASEBLOCK* p)
{
	BASEBLOCKEX* pexblock;
	BASEBLOCK* pstart;
	int lastdelay;
		
	assert( p != NULL );

	if( p->uType & BLOCKTYPE_DELAYSLOT ) {
		psxRecClearMem(p-1);
		if( p->GetFnptr() == 0 )
			return;
	}
 
	assert( p->GetFnptr() != 0 );
	assert( p->startpc );

	x86Ptr = (u8*)p->GetFnptr();

	// there is a small problem: mem can be ored with 0xa<<28 or 0x8<<28, and don't know which
	MOV32ItoR(EDX, p->startpc);
	assert( (uptr)x86Ptr <= 0xffffffff );
	PUSH32I((uptr)x86Ptr);
	JMP32((uptr)psxDispatcherClear - ( (uptr)x86Ptr + 5 ));
	assert( x86Ptr == (u8*)p->GetFnptr() + IOP_MIN_BLOCK_BYTES );

	pstart = PSX_GETBLOCK(p->startpc);
	pexblock = PSX_GETBLOCKEX(pstart);
	assert( pexblock->startpc == pstart->startpc );

	// don't delete if last is delay
	lastdelay = pexblock->size;
	if( pstart[pexblock->size-1].uType & BLOCKTYPE_DELAYSLOT ) {
		assert( pstart[pexblock->size-1].GetFnptr() != pstart->GetFnptr() );
		if( pstart[pexblock->size-1].GetFnptr() != 0 ) {
			pstart[pexblock->size-1].uType = 0;
			--lastdelay;
		}
	}

	memset(pstart, 0, lastdelay*sizeof(BASEBLOCK));

	RemoveBaseBlockEx(pexblock, 1);
	pexblock->size = 0;
	pexblock->startpc = 0;
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

	JMP32((uptr)psxDispatcherReg - ( (uptr)x86Ptr + 5 ));
}

void psxSetBranchImm( u32 imm )
{
	u32* ptr;
	psxbranch = 1;
	assert( imm );

	// end the current block
	MOV32ItoM( (uptr)&psxRegs.pc, imm );
	_psxFlushCall(FLUSH_EVERYTHING);
	iPsxBranchTest(imm, imm <= psxpc);

	MOV32ItoR(EDX, 0);
	ptr = (u32*)(x86Ptr-4);
	*ptr = (uptr)JMP32((uptr)psxDispatcher - ( (uptr)x86Ptr + 5 ));
}

//fixme : this is all a huge hack, we base the counter advancements on the average an opcode should take (wtf?)
//		  If that wasn't bad enough we have default values like 9/8 which will get cast to int later
//		  (yeah, that means all sync code couldn't have worked to beginn with)
//		  So for now these are new settings that work.
//		  (rama)

static u32 psxScaleBlockCycles()
{
	return s_psxBlockCycles * (CHECK_IOP_CYCLERATE ? 2 : 1);
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

	RET2();		// returns control to the EE

	// Continue onward with branching here:
	x86SetJ8( j8Ptr[2] );

	// check if should branch
	SUB32MtoR(ECX, (uptr)&g_psxNextBranchCycle);
	j8Ptr[0] = JS8( 0 );

	CALLFunc((uptr)psxBranchTest);

	if( newpc != 0xffffffff )
	{
		CMP32ItoM((uptr)&psxRegs.pc, newpc);
		JNE32((uptr)psxDispatcherReg - ( (uptr)x86Ptr + 6 ));
	}

	// Skip branch jump target here:
	x86SetJ8( j8Ptr[0] );
}

static const int *s_pCode;

#if !defined(_MSC_VER)
static void checkcodefn()
{
	int pctemp;

#ifdef _MSC_VER
	__asm mov pctemp, eax;
#else
    __asm__("movl %%eax, %0" : : "m"(pctemp) );
#endif
	SysPrintf("iop code changed! %x\n", pctemp);
}
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
	JMP32((uptr)psxDispatcherReg - ( (uptr)x86Ptr + 5 ));

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
	JMP32((uptr)psxDispatcherReg - ( (uptr)x86Ptr + 5 ));
	x86SetJ8(j8Ptr[0]);

	//if (!psxbranch) psxbranch = 2;
}

u32 psxRecompileCodeSafe(u32 temppc)
{
	BASEBLOCK* pblock = PSX_GETBLOCK(temppc);

	if( pblock->GetFnptr() != 0 && pblock->startpc != s_pCurBlock->startpc ) {
		if( psxpc == pblock->startpc )
			return 0;
	}

	return 1;
}

void psxRecompileNextInstruction(int delayslot)
{
	static u8 s_bFlushReg = 1;

	BASEBLOCK* pblock = PSX_GETBLOCK(psxpc);

	// need *ppblock != s_pCurBlock because of branches
	if( pblock->GetFnptr() != 0 && pblock->startpc != s_pCurBlock->startpc ) {

		if( !delayslot && psxpc == pblock->startpc ) {
			// code already in place, so jump to it and exit recomp
			assert( PSX_GETBLOCKEX(pblock)->startpc == pblock->startpc );
			
			_psxFlushCall(FLUSH_EVERYTHING);
			MOV32ItoM((uptr)&psxRegs.pc, psxpc);
				
//			if( pexblock->pOldFnptr ) {
//				// code already in place, so jump to it and exit recomp
//				JMP32((uptr)pexblock->pOldFnptr - ((uptr)x86Ptr + 5));
//				branch = 3;
//				return;
//			}
			
			JMP32((uptr)pblock->GetFnptr() - ((uptr)x86Ptr + 5));
			psxbranch = 3;
			return;
		}
		else {

			if( !(delayslot && pblock->startpc == psxpc) ) {
				u8* oldX86 = x86Ptr;
				//__Log("clear block %x\n", pblock->startpc);
				psxRecClearMem(pblock);
				x86Ptr = oldX86;
				if( delayslot )
					SysPrintf("delay slot %x\n", psxpc);
			}
		}
	}

	if( delayslot )
		pblock->uType = BLOCKTYPE_DELAYSLOT;

#ifdef _DEBUG
	MOV32ItoR(EAX, psxpc);
#endif

	s_pCode = iopVirtMemR<int>( psxpc );
	assert(s_pCode);

	psxRegs.code = *(int *)s_pCode;
	s_psxBlockCycles++;
	psxpc += 4;

//#ifdef _DEBUG
//	CMP32ItoM((uptr)s_pCode, psxRegs.code);
//	j8Ptr[0] = JE8(0);
//	MOV32ItoR(EAX, psxpc);
//	CALLFunc((uptr)checkcodefn);
//	x86SetJ8( j8Ptr[ 0 ] );
//#endif


	g_pCurInstInfo++;

#ifdef PCSX2_VM_COISSUE
	assert( g_pCurInstInfo->info & EEINSTINFO_COREC );
#endif

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

#include "IopHw.h"

void iDumpPsxRegisters(u32 startpc, u32 temp)
{
// [TODO] fixme : thie code is broken and has no labels.  Needs a rewrite to be useful.

#if 0
	int i;
	const char* pstr = temp ? "t" : "";

    __Log("%spsxreg: %x %x ra:%x k0: %x %x\n", pstr, startpc, psxRegs.cycle, psxRegs.GPR.n.ra, psxRegs.GPR.n.k0, *(int*)PSXM(0x13c128));
    for(i = 0; i < 34; i+=2) __Log("%spsx%s: %x %x\n", pstr, disRNameGPR[i], psxRegs.GPR.r[i], psxRegs.GPR.r[i+1]);
	__Log("%scycle: %x %x %x; counters %x %x\n", pstr, psxRegs.cycle, g_psxNextBranchCycle, EEsCycle, 
		psxNextsCounter, psxNextCounter);

	__Log("psxdma%d c%x b%x m%x t%x\n", 2, HW_DMA2_CHCR, HW_DMA2_BCR, HW_DMA2_MADR, HW_DMA2_TADR);
	__Log("psxdma%d c%x b%x m%x\n", 3, HW_DMA3_CHCR, HW_DMA3_BCR, HW_DMA3_MADR);
	__Log("psxdma%d c%x b%x m%x t%x\n", 4, HW_DMA4_CHCR, HW_DMA4_BCR, HW_DMA4_MADR, HW_DMA4_TADR);
	__Log("psxdma%d c%x b%x m%x\n", 6, HW_DMA6_CHCR, HW_DMA6_BCR, HW_DMA6_MADR);
	__Log("psxdma%d c%x b%x m%x\n", 7, HW_DMA7_CHCR, HW_DMA7_BCR, HW_DMA7_MADR);
	__Log("psxdma%d c%x b%x m%x\n", 8, HW_DMA8_CHCR, HW_DMA8_BCR, HW_DMA8_MADR);
	__Log("psxdma%d c%x b%x m%x t%x\n", 9, HW_DMA9_CHCR, HW_DMA9_BCR, HW_DMA9_MADR, HW_DMA9_TADR);
	__Log("psxdma%d c%x b%x m%x\n", 10, HW_DMA10_CHCR, HW_DMA10_BCR, HW_DMA10_MADR);
    __Log("psxdma%d c%x b%x m%x\n", 11, HW_DMA11_CHCR, HW_DMA11_BCR, HW_DMA11_MADR);
	__Log("psxdma%d c%x b%x m%x\n", 12, HW_DMA12_CHCR, HW_DMA12_BCR, HW_DMA12_MADR);
	for(i = 0; i < 7; ++i)
		__Log("%scounter%d: mode %x count %I64x rate %x scycle %x target %I64x\n", pstr, i, psxCounters[i].mode, psxCounters[i].count, psxCounters[i].rate, psxCounters[i].sCycleT, psxCounters[i].target);
#endif
}

void iDumpPsxRegisters(u32 startpc);

#ifdef _DEBUG
static void printfn()
{
	static int lastrec = 0;
	static int curcount = 0;
	const int skip = 0;

    //*(int*)PSXM(0x27990) = 1; // enables cdvd bios output for scph10000

    if( psxRegs.cycle == 0x113a1be5 ) {
//        FILE* tempf = fopen("tempdmciop.txt", "wb");
//        fwrite(PSXM(0), 0x200000, 1, tempf);
//        fclose(tempf);
        //psxdump |= 2;
    }

//    if( psxRegs.cycle == 0x114152d8 ) {
//        psxRegs.GPR.n.s0 = 0x55000;
//    }

    if( (psxdump&2) && lastrec != g_psxlastpc ) {
		curcount++;

		if( curcount > skip ) {
			iDumpPsxRegisters(g_psxlastpc, 1);
			curcount = 0;
		}

		lastrec = g_psxlastpc;
	}
}
#endif

void psxRecRecompile(u32 startpc)
{
	u32 i;
	u32 branchTo;
	u32 willbranch3 = 0;
	u32* ptr;

#ifdef _DEBUG
	//psxdump |= 4;
	if( psxdump & 4 )
		iDumpPsxRegisters(startpc, 0);
#endif

	assert( startpc );

	// if recPtr reached the mem limit reset whole mem
	if (((uptr)recPtr - (uptr)recMem) >= (RECMEM_SIZE - 0x10000)) {
		DevCon::WriteLn("IOP Recompiler data reset");
		recResetIOP();
	}

	s_pCurBlock = PSX_GETBLOCK(startpc);
	
	if( s_pCurBlock->GetFnptr() ) {
		// clear if already taken
		assert( s_pCurBlock->startpc < startpc );
		psxRecClearMem(s_pCurBlock);	
	}

	if( s_pCurBlock->startpc == startpc ) {
		s_pCurBlockEx = PSX_GETBLOCKEX(s_pCurBlock);
		assert( s_pCurBlockEx->startpc == startpc );
	}
	else {
		s_pCurBlockEx = NULL;
		for(i = 0; i < PSX_NUMBLOCKS; ++i) {
			if( recBlocks[(i+s_nNextBlock)%PSX_NUMBLOCKS].size == 0 ) {
				s_pCurBlockEx = recBlocks+(i+s_nNextBlock)%PSX_NUMBLOCKS;
				s_nNextBlock = (i+s_nNextBlock+1)%PSX_NUMBLOCKS;
				break;
			}
		}

		if( s_pCurBlockEx == NULL ) {
			DevCon::WriteLn("IOP Recompiler data reset");
			recResetIOP();
			s_nNextBlock = 0;
			s_pCurBlockEx = recBlocks;
		}

		s_pCurBlockEx->startpc = startpc;
	}
	
	x86SetPtr( recPtr );
	x86Align(16);
	recPtr = x86Ptr;

    psxbranch = 0;

	s_pCurBlock->startpc = startpc;
	s_pCurBlock->SetFnptr( (uptr)x86Ptr );
	s_psxBlockCycles = 0;

	// reset recomp state variables
	psxpc = startpc;
	s_saveConstGPRreg = 0;
	g_psxHasConstReg = g_psxFlushedConstReg = 1;

	_initX86regs();

#ifdef _DEBUG
	// for debugging purposes
	MOV32ItoM((uptr)&g_psxlastpc, psxpc);
	CALLFunc((uptr)printfn);
#endif

	// go until the next branch
	i = startpc;
	s_nEndBlock = 0xffffffff;
	
	while(1) {
		BASEBLOCK* pblock = PSX_GETBLOCK(i);
		if( pblock->GetFnptr() != 0 && pblock->startpc != s_pCurBlock->startpc ) {

			if( i == pblock->startpc ) {
				// branch = 3
				willbranch3 = 1;
				s_nEndBlock = i;
				break;
			}
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

#ifdef _DEBUG
	// dump code
	for(i = 0; i < ArraySize(s_psxrecblocks); ++i) {
		if( startpc == s_psxrecblocks[i] ) {
			iIopDumpBlock(startpc, recPtr);
		}
	}

	if( (psxdump & 1) )
		iIopDumpBlock(startpc, recPtr);
#endif
	
	g_pCurInstInfo = s_pInstCache;
	while (!psxbranch && psxpc < s_nEndBlock) {
		psxRecompileNextInstruction(0);
	}

#ifdef _DEBUG
	if( (psxdump & 1) )
		iIopDumpBlock(startpc, recPtr);
#endif

	assert( (psxpc-startpc)>>2 <= 0xffff );
	s_pCurBlockEx->size = (psxpc-startpc)>>2;

	for(i = 1; i < (u32)s_pCurBlockEx->size-1; ++i) {
		s_pCurBlock[i].SetFnptr( s_pCurBlock->GetFnptr() );
		s_pCurBlock[i].startpc = s_pCurBlock->startpc;
	}

	// don't overwrite if delay slot
	if( i < (u32)s_pCurBlockEx->size && !(s_pCurBlock[i].uType & BLOCKTYPE_DELAYSLOT) ) {
		s_pCurBlock[i].SetFnptr( s_pCurBlock->GetFnptr() );
		s_pCurBlock[i].startpc = s_pCurBlock->startpc;
	}

	// set the block ptr
	AddBaseBlockEx(s_pCurBlockEx, 1);

	if( !(psxpc&0x10000000) )
		g_psxMaxRecMem = std::max( (psxpc&~0xa0000000), g_psxMaxRecMem );

	if( psxbranch == 2 ) {
		_psxFlushCall(FLUSH_EVERYTHING);

		iPsxBranchTest(0xffffffff, 1);	

		JMP32((uptr)psxDispatcherReg - ( (uptr)x86Ptr + 5 ));
	}
	else {
		assert( psxbranch != 3 );
		if( psxbranch ) assert( !willbranch3 );
		else
		{
			ADD32ItoM((uptr)&psxRegs.cycle, psxScaleBlockCycles() );
			SUB32ItoM((uptr)&psxCycleEE, psxScaleBlockCycles()*8 );
		}

		if( willbranch3 ) {
			BASEBLOCK* pblock = PSX_GETBLOCK(s_nEndBlock);
			assert( psxpc == s_nEndBlock );
			_psxFlushCall(FLUSH_EVERYTHING);
			MOV32ItoM((uptr)&psxRegs.pc, psxpc);			
			JMP32((uptr)pblock->GetFnptr() - ((uptr)x86Ptr + 5));
			psxbranch = 3;
		}
		else if( !psxbranch ) {
			// didn't branch, but had to stop
			MOV32ItoM( (uptr)&psxRegs.pc, psxpc );

			_psxFlushCall(FLUSH_EVERYTHING);

			ptr = JMP32(0);
			//JMP32((uptr)psxDispatcherReg - ( (uptr)x86Ptr + 5 ));
		}
	}

	assert( x86Ptr >= (u8*)s_pCurBlock->GetFnptr() + IOP_MIN_BLOCK_BYTES );
	assert( x86Ptr < recMem+RECMEM_SIZE );

	recPtr = x86Ptr;

	assert( (g_psxHasConstReg&g_psxFlushedConstReg) == g_psxHasConstReg );

	if( !psxbranch ) {
		BASEBLOCK* pcurblock = s_pCurBlock;
		u32 nEndBlock = s_nEndBlock;
		s_pCurBlock = PSX_GETBLOCK(psxpc);
		assert( ptr != NULL );
		
		if( s_pCurBlock->startpc != psxpc ){
 			psxRecRecompile(psxpc);
		}

		// could have reset
		if( pcurblock->startpc == startpc ) {
			assert( pcurblock->GetFnptr() );
			assert( s_pCurBlock->startpc == nEndBlock );
			*ptr = (u32)((uptr)s_pCurBlock->GetFnptr() - ( (uptr)ptr + 4 ));
		}
		else {
			psxRecRecompile(startpc);
			assert( pcurblock->GetFnptr() != 0 );
		}
	}
    else
        assert( s_pCurBlock->GetFnptr() != 0 );
}

R3000Acpu psxRec = {
	recAlloc,
	recResetIOP,
	recExecute,
	recExecuteBlock,
	recClear,
	recShutdown
};

