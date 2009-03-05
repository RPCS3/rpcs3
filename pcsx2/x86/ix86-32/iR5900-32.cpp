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

#include "PrecompiledHeader.h"

#include "Common.h"
#include "Memory.h"
#include "R5900OpcodeTables.h"
#include "iR5900.h"
#include "iR5900AritImm.h"
#include "iR5900Arit.h"
#include "iR5900MultDiv.h"
#include "iR5900Shift.h"
#include "iR5900Branch.h"
#include "iR5900Jump.h"
#include "iR5900LoadStore.h"
#include "iR5900Move.h"
#include "iMMI.h"
#include "iFPU.h"
#include "iCOP0.h"
#include "iVUmicro.h"
#include "VU.h"
#include "VUmicro.h"

#include "iVUzerorec.h"
#include "vtlb.h"

#include "SamplProf.h"
#include "Paths.h"

#include "NakedAsm.h"

using namespace R5900;

// used to disable register freezing during cpuBranchTests (registers
// are safe then since they've been completely flushed)
bool g_EEFreezeRegs = false;

u32 maxrecmem = 0;
uptr *recLUT = NULL;

u32 s_nBlockCycles = 0; // cycles of current block recompiling
//u8* dyna_block_discard_recmem=0;

u32 pc;			         // recompiler pc
int branch;		         // set for branch

PCSX2_ALIGNED16(GPR_reg64 g_cpuConstRegs[32]) = {0};
u32 g_cpuHasConstReg = 0, g_cpuFlushedConstReg = 0;
u32 s_saveConstGPRreg = 0;
GPR_reg64 s_ConstGPRreg;

////////////////////////////////////////////////////////////////
// Static Private Variables - R5900 Dynarec

#define X86
static const int RECSTACK_SIZE = 0x00010000;
static const int EE_NUMBLOCKS = (1<<15);

static u8 *recMem = NULL;			// the recompiled blocks will be here
static u8* recStack = NULL;			// stack mem
static BASEBLOCK *recRAM = NULL;		// and the ptr to the blocks here
static BASEBLOCK *recROM = NULL;		// and here
static BASEBLOCK *recROM1 = NULL;		// also here
static BASEBLOCKEX *recBlocks = NULL;
static u8* recPtr = NULL, *recStackPtr = NULL;
static EEINST* s_pInstCache = NULL;
static u32 s_nInstCacheSize = 0;

static BASEBLOCK* s_pCurBlock = NULL;
static BASEBLOCKEX* s_pCurBlockEx = NULL;
const BASEBLOCK* s_pDispatchBlock = NULL;
static u32 s_nEndBlock = 0; // what pc the current block ends	
static u32 s_nHasDelay = 0;

static u32 s_nNextBlock = 0; // next free block in recBlocks

// save states for branches
static u16 s_savex86FpuState, s_saveiCWstate;
static u32 s_saveHasConstReg = 0, s_saveFlushedConstReg = 0, s_saveRegHasLive1 = 0, s_saveRegHasSignExt = 0;
static EEINST* s_psaveInstInfo = NULL;

static u32 s_savenBlockCycles = 0;

#ifdef _DEBUG
static u32 dumplog = 0;
#else
#define dumplog 0
#endif

#ifdef PCSX2_DEVBUILD
// and not sure what these might have once been used for... (air)
//static const char *txt0 = "EAX = %x : ECX = %x : EDX = %x\n";
//static const char *txt0RC = "EAX = %x : EBX = %x : ECX = %x : EDX = %x : ESI = %x : EDI = %x\n";
//static const char *txt1 = "REG[%d] = %x_%x\n";
//static const char *txt2 = "M32 = %x\n";
#endif

static void iBranchTest(u32 newpc, bool noDispatch=false);

BASEBLOCKEX* PC_GETBLOCKEX(BASEBLOCK* p)
{
//	BASEBLOCKEX* pex = *(BASEBLOCKEX**)(p+1);
//	if( pex >= recBlocks && pex < recBlocks+EE_NUMBLOCKS )
//		return pex;

	// otherwise, use the sorted list
	return GetBaseBlockEx(p->startpc, 0);
}

////////////////////////////////////////////////////
static void iDumpBlock( int startpc, u8 * ptr )
{
	FILE *f;
	string filename;
	u32 i, j;
	EEINST* pcur;
	u8 used[34];
	u8 fpuused[33];
	int numused, count, fpunumused;

	Console::Status( "dump1 %x:%x, %x", params startpc, pc, cpuRegs.cycle );
	Path::CreateDirectory( "dumps" );
	ssprintf( filename, "dumps\\R5900dump%.8X.txt", startpc );

	fflush( stdout );
//	f = fopen( "dump1", "wb" );
//	fwrite( ptr, 1, (u32)x86Ptr[0] - (u32)ptr, f );
//	fclose( f );
//
//	sprintf( command, "objdump -D --target=binary --architecture=i386 dump1 > %s", filename );
//	system( command );

	f = fopen( filename.c_str(), "w" );

	std::string output;

    if( disR5900GetSym(startpc) != NULL )
        fprintf(f, "%s\n", disR5900GetSym(startpc));
	for ( i = startpc; i < s_nEndBlock; i += 4 ) {
		disR5900Fasm( output, memRead32( i ), i );
		fprintf( f, output.c_str() );
	}

	// write the instruction info

	fprintf(f, "\n\nlive0 - %x, live1 - %x, live2 - %x, lastuse - %x\nmmx - %x, xmm - %x, used - %x\n",
		EEINST_LIVE0, EEINST_LIVE1, EEINST_LIVE2, EEINST_LASTUSE, EEINST_MMX, EEINST_XMM, EEINST_USED);

	memzero_obj(used);
	numused = 0;
	for(i = 0; i < ArraySize(s_pInstCache->regs); ++i) {
		if( s_pInstCache->regs[i] & EEINST_USED ) {
			used[i] = 1;
			numused++;
		}
	}

	memzero_obj(fpuused);
	fpunumused = 0;
	for(i = 0; i < ArraySize(s_pInstCache->fpuregs); ++i) {
		if( s_pInstCache->fpuregs[i] & EEINST_USED ) {
			fpuused[i] = 1;
			fpunumused++;
		}
	}

	fprintf(f, "       ");
	for(i = 0; i < ArraySize(s_pInstCache->regs); ++i) {
		if( used[i] ) fprintf(f, "%2d ", i);
	}
	for(i = 0; i < ArraySize(s_pInstCache->fpuregs); ++i) {
		if( fpuused[i] ) fprintf(f, "%2d ", i);
	}
	fprintf(f, "\n");

	fprintf(f, "       ");
	for(i = 0; i < ArraySize(s_pInstCache->regs); ++i) {
		if( used[i] ) fprintf(f, "%s ", disRNameGPR[i]);
	}
	for(i = 0; i < ArraySize(s_pInstCache->fpuregs); ++i) {
		if( fpuused[i] ) fprintf(f, "%s ", i<32?"FR":"FA");
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
		count = 1;
		for(j = 0; j < ArraySize(s_pInstCache->fpuregs); j++) {
			if( fpuused[j] ) {
				fprintf(f, "%2.2x%s", pcur->fpuregs[j], ((count%8)&&count<fpunumused)?"_":" ");
				++count;
			}
		}
		fprintf(f, "\n");
	}
	fclose( f );
}

#ifdef PCSX2_VM_COISSUE
static u8 _eeLoadWritesRs(u32 tempcode)
{
	switch(tempcode>>26) {
		case 26: // ldl
		case 27: // ldr
		case 32: case 33: case 34: case 35: case 36: case 37: case 38: case 39:
		case 55: // LD
		case 30: // lq
			return ((tempcode>>21)&0x1f)==((tempcode>>16)&0x1f); // rs==rt
	}
	return 0;
}

static u8 _eeIsLoadStoreCoIssue(u32 firstcode, u32 secondcode)
{
	switch(firstcode>>26) {
		case 34: // lwl
			return (secondcode>>26)==38;
		case 38: // lwr
			return (secondcode>>26)==34;
		case 42: // swl
			return (secondcode>>26)==46;
		case 46: // swr
			return (secondcode>>26)==42;
		case 26: // ldl
			return (secondcode>>26)==27;
		case 27: // ldr
			return (secondcode>>26)==26;
		case 44: // sdl
			return (secondcode>>26)==45;
		case 45: // sdr
			return (secondcode>>26)==44;

		case 32: case 33: case 35: case 36: case 37: case 39:
		case 55: // LD

		// stores
		case 40: case 41: case 43:
		case 63: // sd
			return (secondcode>>26)==(firstcode>>26);

		case 30: // lq
		case 31: // sq
		case 49: // lwc1
		case 57: // swc1
		case 54: // lqc2
		case 62: // sqc2
			return (secondcode>>26)==(firstcode>>26);
	}
	return 0;
}

static u8 _eeIsLoadStoreCoX(u32 tempcode)
{
	switch( tempcode>>26 ) {
		case 30: case 31: case 49: case 57: case 55: case 63:
			return 1;
	}
	return 0;
}
#endif

void _eeFlushAllUnused()
{
	int i;
	for(i = 0; i < 34; ++i) {
		if( pc < s_nEndBlock ) {
			if( (g_pCurInstInfo[1].regs[i]&EEINST_USED) )
				continue;
		}
		else if( (g_pCurInstInfo[0].regs[i]&EEINST_USED) )
			continue;

		if( i < 32 && GPR_IS_CONST1(i) ) _flushConstReg(i);
		else {	
			_deleteMMXreg(MMX_GPR+i, 1);
			_deleteGPRtoXMMreg(i, 1);
		}
	}

	//TODO when used info is done for FPU and VU0
	for(i = 0; i < XMMREGS; ++i) {
		if( xmmregs[i].inuse && xmmregs[i].type != XMMTYPE_GPRREG )
			_freeXMMreg(i);
	}
}

u32* _eeGetConstReg(int reg)
{
	assert( GPR_IS_CONST1( reg ) );

	if( g_cpuFlushedConstReg & (1<<reg) )
		return &cpuRegs.GPR.r[ reg ].UL[0];

	// if written in the future, don't flush
	if( _recIsRegWritten(g_pCurInstInfo+1, (s_nEndBlock-pc)/4, XMMTYPE_GPRREG, reg) ) {
		u32* ptempmem;
		ptempmem = recAllocStackMem(8, 4);
		ptempmem[0] = g_cpuConstRegs[ reg ].UL[0];
		ptempmem[1] = g_cpuConstRegs[ reg ].UL[1];
		return ptempmem;
	}
	
	_flushConstReg(reg);
	return &cpuRegs.GPR.r[ reg ].UL[0];
}

void _eeMoveGPRtoR(x86IntRegType to, int fromgpr)
{
	if( GPR_IS_CONST1(fromgpr) )
		MOV32ItoR( to, g_cpuConstRegs[fromgpr].UL[0] );
	else {
		int mmreg;
		
		if( (mmreg = _checkXMMreg(XMMTYPE_GPRREG, fromgpr, MODE_READ)) >= 0 && (xmmregs[mmreg].mode&MODE_WRITE)) {
			SSE2_MOVD_XMM_to_R(to, mmreg);
		}
		else if( (mmreg = _checkMMXreg(MMX_GPR+fromgpr, MODE_READ)) >= 0 && (mmxregs[mmreg].mode&MODE_WRITE) ) {
			MOVD32MMXtoR(to, mmreg);
			SetMMXstate();
		}
		else {
			MOV32MtoR(to, (int)&cpuRegs.GPR.r[ fromgpr ].UL[ 0 ] );
		}
	}
}

void _eeMoveGPRtoM(u32 to, int fromgpr)
{
	if( GPR_IS_CONST1(fromgpr) )
		MOV32ItoM( to, g_cpuConstRegs[fromgpr].UL[0] );
	else {
		int mmreg;
		
		if( (mmreg = _checkXMMreg(XMMTYPE_GPRREG, fromgpr, MODE_READ)) >= 0 ) {
			SSEX_MOVD_XMM_to_M32(to, mmreg);
		}
		else if( (mmreg = _checkMMXreg(MMX_GPR+fromgpr, MODE_READ)) >= 0 ) {
			MOVDMMXtoM(to, mmreg);
			SetMMXstate();
		}
		else {
			MOV32MtoR(EAX, (int)&cpuRegs.GPR.r[ fromgpr ].UL[ 0 ] );
			MOV32RtoM(to, EAX );
		}
	}
}

void _eeMoveGPRtoRm(x86IntRegType to, int fromgpr)
{
	if( GPR_IS_CONST1(fromgpr) )
		MOV32ItoRmOffset( to, g_cpuConstRegs[fromgpr].UL[0], 0 );
	else {
		int mmreg;
		
		if( (mmreg = _checkXMMreg(XMMTYPE_GPRREG, fromgpr, MODE_READ)) >= 0 ) {
			SSEX_MOVD_XMM_to_Rm(to, mmreg);
		}
		else if( (mmreg = _checkMMXreg(MMX_GPR+fromgpr, MODE_READ)) >= 0 ) {
			MOVD32MMXtoRm(to, mmreg);
			SetMMXstate();
		}
		else {
			MOV32MtoR(EAX, (int)&cpuRegs.GPR.r[ fromgpr ].UL[ 0 ] );
			MOV32RtoRm(to, EAX );
		}
	}
}

int _flushXMMunused()
{
	int i;
	for (i=0; i<XMMREGS; i++) {
		if (!xmmregs[i].inuse || xmmregs[i].needed || !(xmmregs[i].mode&MODE_WRITE) ) continue;
		
		if (xmmregs[i].type == XMMTYPE_GPRREG ) {
			//if( !(g_pCurInstInfo->regs[xmmregs[i].reg]&EEINST_USED) ) {
			if( !_recIsRegWritten(g_pCurInstInfo+1, (s_nEndBlock-pc)/4, XMMTYPE_GPRREG, xmmregs[i].reg) ) {
				_freeXMMreg(i);
				xmmregs[i].inuse = 1;
				return 1;
			}
		}
	}

	return 0;
}

int _flushMMXunused()
{
	int i;
	for (i=0; i<MMXREGS; i++) {
		if (!mmxregs[i].inuse || mmxregs[i].needed || !(mmxregs[i].mode&MODE_WRITE) ) continue;
		
		if( MMX_ISGPR(mmxregs[i].reg) ) {
			//if( !(g_pCurInstInfo->regs[mmxregs[i].reg-MMX_GPR]&EEINST_USED) ) {
			if( !_recIsRegWritten(g_pCurInstInfo+1, (s_nEndBlock-pc)/4, XMMTYPE_GPRREG, mmxregs[i].reg-MMX_GPR) ) {
				_freeMMXreg(i);
				mmxregs[i].inuse = 1;
				return 1;
			}
		}
	}

	return 0;
}

int _flushUnusedConstReg()
{
	int i;
	for(i = 1; i < 32; ++i) {
		if( (g_cpuHasConstReg & (1<<i)) && !(g_cpuFlushedConstReg&(1<<i)) &&
			!_recIsRegWritten(g_pCurInstInfo+1, (s_nEndBlock-pc)/4, XMMTYPE_GPRREG, i) ) {

			// check if will be written in the future
			MOV32ItoM((uptr)&cpuRegs.GPR.r[i].UL[0], g_cpuConstRegs[i].UL[0]);
			MOV32ItoM((uptr)&cpuRegs.GPR.r[i].UL[1], g_cpuConstRegs[i].UL[1]);
			g_cpuFlushedConstReg |= 1<<i;
			return 1;
		}
	}

	return 0;
}

u32* recAllocStackMem(int size, int align)
{
	// write to a temp loc, trick
	if( (u32)recStackPtr % align ) recStackPtr += align - ((u32)recStackPtr%align);
	recStackPtr += size;
	return (u32*)(recStackPtr-size);
}


static const int REC_CACHEMEM = 0x01000000;
static void __fastcall dyna_block_discard(u32 start,u32 sz);

// memory allocation handle for the entire BASEBLOCK and stack allocations.
static u8* m_recBlockAlloc = NULL;

static const uint m_recBlockAllocSize = 
	(((Ps2MemSize::Base + Ps2MemSize::Rom + Ps2MemSize::Rom1) / 4) * sizeof(BASEBLOCK))
+	(EE_NUMBLOCKS*sizeof(BASEBLOCKEX))		// recBlocks
+	RECSTACK_SIZE;		// recStack

static void recAlloc() 
{
	// Hardware Requirements Check...

	if ( !( cpucaps.hasMultimediaExtensions  ) )
		throw Exception::HardwareDeficiency( _( "Processor doesn't support MMX" ) );

	if ( !( cpucaps.hasStreamingSIMDExtensions ) )
		throw Exception::HardwareDeficiency( _( "Processor doesn't support SSE" ) );

	if ( !( cpucaps.hasStreamingSIMD2Extensions ) )
		throw Exception::HardwareDeficiency( _( "Processor doesn't support SSE2" ) );

	if( recLUT == NULL )
		recLUT = (uptr*) _aligned_malloc( 0x010000 * sizeof(uptr), 16 );

	if( recMem == NULL )
	{
		// Note: the VUrec depends on being able to grab an allocatione below the 0x10000000 line,
		// so we give the EErec an address above that to try first as it's basemem address, hence
		// the 0x20000000 pick.

		const uint cachememsize = REC_CACHEMEM+0x1000;
		recMem = (u8*)SysMmapEx( 0x20000000, cachememsize, 0, "recAlloc(R5900)" );
	}

	if( recMem == NULL )
		throw Exception::OutOfMemory( "R5900-32 > failed to allocate recompiler memory." );

	// Goal: Allocate BASEBLOCKs for every possible branch target in PS2 memory.
	// Any 4-byte aligned address makes a valid branch target as per MIPS design (all instructions are
	// always 4 bytes long).

    if( m_recBlockAlloc == NULL )
		m_recBlockAlloc = (u8*) _aligned_malloc( m_recBlockAllocSize, 4096 );

	if( m_recBlockAlloc == NULL )
		throw Exception::OutOfMemory( "R5900-32 Init > Failed to allocate memory for BASEBLOCK tables." );

	u8* curpos = m_recBlockAlloc;
	recRAM = (BASEBLOCK*)curpos; curpos += (Ps2MemSize::Base / 4) * sizeof(BASEBLOCK);
	recROM = (BASEBLOCK*)curpos; curpos += (Ps2MemSize::Rom / 4) * sizeof(BASEBLOCK);
	recROM1 = (BASEBLOCK*)curpos; curpos += (Ps2MemSize::Rom1 / 4) * sizeof(BASEBLOCK);
	recBlocks = (BASEBLOCKEX*)curpos; curpos += sizeof(BASEBLOCKEX)*EE_NUMBLOCKS;
	recStack = (u8*)curpos;

	if( s_pInstCache == NULL )
	{
		s_nInstCacheSize = 128;
		s_pInstCache = (EEINST*)malloc( sizeof(EEINST) * s_nInstCacheSize );
	}

	if( s_pInstCache == NULL )
		throw Exception::OutOfMemory( "R5900-32 Init > failed to allocate memory for pInstCache." );

	// No errors.. Proceed with initialization:

	ProfilerRegisterSource( "EERec", recMem, REC_CACHEMEM+0x1000 );

	x86FpuState = FPU_STATE;
}

////////////////////////////////////////////////////
void recResetEE( void )
{
	DbgCon::Status( "iR5900-32 > Resetting recompiler memory and structures." );

	s_nNextBlock = 0;
	maxrecmem = 0;

	memset_8<0xcd, REC_CACHEMEM>(recMem);
	memzero_ptr<m_recBlockAllocSize>( m_recBlockAlloc );

	if( s_pInstCache )
		memset( s_pInstCache, 0, sizeof(EEINST)*s_nInstCacheSize );

	ResetBaseBlockEx(0);
	mmap_ResetBlockTracking();

#ifdef _MSC_VER
	__asm emms;
#else
    __asm__("emms");
#endif

	memzero_ptr<0x010000 * sizeof(uptr)>( recLUT );

	for ( int i = 0x0000; i < 0x0200; i++ )
	{
		recLUT[ i + 0x0000 ] = (uptr)&recRAM[ i << 14 ];
		recLUT[ i + 0x2000 ] = (uptr)&recRAM[ i << 14 ];
		recLUT[ i + 0x3000 ] = (uptr)&recRAM[ i << 14 ];
	}

	for ( int i = 0x0000; i < 0x0040; i++ )
	{
		recLUT[ i + 0x1fc0 ] = (uptr)&recROM[ i << 14 ];
		recLUT[ i + 0x9fc0 ] = (uptr)&recROM[ i << 14 ];
		recLUT[ i + 0xbfc0 ] = (uptr)&recROM[ i << 14 ];
	}

	for ( int i = 0x0000; i < 0x0004; i++ )
	{
		recLUT[ i + 0x1e00 ] = (uptr)&recROM1[ i << 14 ];
		recLUT[ i + 0x9e00 ] = (uptr)&recROM1[ i << 14 ];
		recLUT[ i + 0xbe00 ] = (uptr)&recROM1[ i << 14 ];
	}

	memcpy_fast( recLUT + 0x8000, recLUT, 0x2000 * sizeof(uptr) );
	memcpy_fast( recLUT + 0xa000, recLUT, 0x2000 * sizeof(uptr) );
	
	// drk||Raziel says this is useful but I'm not sure why.  Something to do with forward jumps.
	// Anyways, it causes random crashing for some reasom, possibly because of memory
	// corrupition elsewhere in the recs.  I can't reproduce the problem here though,
	// so a fix will have to wait until later. -_- (air)

	//x86SetPtr(recMem+REC_CACHEMEM);
	//dyna_block_discard_recmem=(u8*)x86Ptr[0];
	//JMP32( (uptr)&dyna_block_discard - ( (u32)x86Ptr[0] + 5 ));

	x86SetPtr(recMem);

	recPtr = recMem;
	recStackPtr = recStack;
	x86FpuState = FPU_STATE;
	iCWstate = 0;

	branch = 0;
	SetCPUState(Config.sseMXCSR, Config.sseVUMXCSR);
}

static void recShutdown( void )
{
	ProfilerTerminateSource( "EERec" );
	ResetBaseBlockEx(0);

	SafeSysMunmap( recMem, REC_CACHEMEM );
	safe_aligned_free( recLUT );
	safe_aligned_free( m_recBlockAlloc );
	recRAM = recROM = recROM1 = NULL;
	recBlocks = NULL;
	recStack = NULL;

	safe_free( s_pInstCache );
	s_nInstCacheSize = 0;
}

// Ignored by Linux
#pragma warning(disable:4731) // frame pointer register 'ebp' modified by inline assembly code

void recStep( void ) {
}

static __forceinline bool recEventTest()
{
#ifdef PCSX2_DEVBUILD
    // dont' remove this check unless doing an official release
    if( g_globalXMMSaved || g_globalMMXSaved)
		DevCon::Error("Pcsx2 Foopah!  Frozen regs have not been restored!!!");
	assert( !g_globalXMMSaved && !g_globalMMXSaved);
#endif

	// Perform counters, ints, and IOP updates:
	bool retval = _cpuBranchTest_Shared();

#ifdef PCSX2_DEVBUILD
	assert( !g_globalXMMSaved && !g_globalMMXSaved);
#endif
	return retval;
}

////////////////////////////////////////////////////

static u32 g_lastpc = 0;
u32 g_EEDispatchTemp;

#ifdef _MSC_VER

// jumped to when invalid pc address
// EDX contains the jump addr to modify
static __naked void Dispatcher()
{
	// EDX contains the jump addr to modify
	__asm push edx

	// calc PC_GETBLOCK
	s_pDispatchBlock = PC_GETBLOCK(cpuRegs.pc);
	
	if( s_pDispatchBlock->startpc != cpuRegs.pc )
		recRecompile(cpuRegs.pc);

	__asm
	{
		mov eax, s_pDispatchBlock
		mov eax, dword ptr [eax]
	}

#ifdef _DEBUG
	__asm mov g_EEDispatchTemp, eax
	assert( g_EEDispatchTemp );
#endif

	// Modify the prev block's jump address, and jump to the new block:
	__asm {
		shl eax, 4
		pop ecx // x86Ptr[0] to mod
		mov edx, eax
		sub edx, ecx
		sub edx, 4
		mov dword ptr [ecx], edx

		jmp eax
	}
}

// edx -  baseblock->startpc
// stack - x86Ptr[0]
static __naked void DispatcherClear()
{
	// EDX contains the current pc
	__asm mov cpuRegs.pc, edx
	__asm push edx

	// calc PC_GETBLOCK
	s_pDispatchBlock = PC_GETBLOCK(cpuRegs.pc);

	if( s_pDispatchBlock != NULL && s_pDispatchBlock->startpc == cpuRegs.pc )
	{
		assert( s_pDispatchBlock->GetFnptr() != 0 );

		// already modded the code, jump to the new place
		__asm {
			pop edx
			mov eax, s_pDispatchBlock
			add esp, 4 // ignore stack
			mov eax, dword ptr [eax]
			shl eax, 4
			jmp eax
		}
	}

	__asm {
		call recRecompile
		add esp, 4 // pop old param
		mov eax, s_pDispatchBlock
		mov eax, dword ptr [eax]

		pop ecx // old fnptr

		shl eax, 4
		mov byte ptr [ecx], 0xe9 // jmp32
		mov edx, eax
		sub edx, ecx
		sub edx, 5
		mov dword ptr [ecx+1], edx

		jmp eax
	}
}

// called when jumping to variable pc address
static __naked void DispatcherReg()
{
	s_pDispatchBlock = PC_GETBLOCK(cpuRegs.pc);

	if( s_pDispatchBlock->startpc != cpuRegs.pc )
		recRecompile(cpuRegs.pc);

	__asm
	{
		mov eax, s_pDispatchBlock
		mov eax, dword ptr [eax]
	}

#ifdef _DEBUG
	__asm mov g_EEDispatchTemp, eax
	assert( g_EEDispatchTemp );
#endif

	__asm {
		shl eax, 4
		jmp eax
	}
}

__forceinline void recExecute()
{
	// Optimization note : Compared pushad against manually pushing the regs one-by-one.
	// Manually pushing is faster, especially on Core2's and such. :)
	do
	{
		g_EEFreezeRegs = true;
		__asm
		{
			push ebx
			push esi
			push edi
			push ebp

			call DispatcherReg
			
			pop ebp
			pop edi
			pop esi
			pop ebx
		}
		g_EEFreezeRegs = false;
	}
	while( !recEventTest() );
}

static void recExecuteBlock()
{
	g_EEFreezeRegs = true;
	__asm
	{
		push ebx
		push esi
		push edi
		push ebp

		call DispatcherReg

		pop ebp
		pop edi
		pop esi
		pop ebx
	}
	g_EEFreezeRegs = false;
	recEventTest();
}

#else // _MSC_VER

__forceinline void recExecute()
{
	// Optimization note : Compared pushad against manually pushing the regs one-by-one.
	// Manually pushing is faster, especially on Core2's and such. :)
	do {
		g_EEFreezeRegs = true;
		__asm__
		(
			".intel_syntax\n"
			"push %ebx\n"
			"push %esi\n"
			"push %edi\n"
			"push %ebp\n"

			"call DispatcherReg\n"
			
			"pop %ebp\n"
			"pop %edi\n"
			"pop %esi\n"
			"pop %ebx\n"
			".att_syntax\n"
		);
		g_EEFreezeRegs = false;
	}
	while( !recEventTest() );
}

static void recExecuteBlock()
{
	g_EEFreezeRegs = true;
	__asm__
	(
		".intel_syntax\n"
		"push %ebx\n"
		"push %esi\n"
		"push %edi\n"
		"push %ebp\n"

		"call DispatcherReg\n"

		"pop %ebp\n"
		"pop %edi\n"
		"pop %esi\n"
		"pop %ebx\n"
		".att_syntax\n"
	);
	g_EEFreezeRegs = false;
	recEventTest();
}
#endif

namespace R5900 {
namespace Dynarec {
namespace OpcodeImpl {

////////////////////////////////////////////////////
void recSYSCALL( void ) {
	MOV32ItoM( (uptr)&cpuRegs.code, cpuRegs.code );
	MOV32ItoM( (uptr)&cpuRegs.pc, pc );
	iFlushCall(FLUSH_NODESTROY);
	CALLFunc( (uptr)R5900::Interpreter::OpcodeImpl::SYSCALL );

	CMP32ItoM((uptr)&cpuRegs.pc, pc);
	j8Ptr[0] = JE8(0);
	ADD32ItoM((uptr)&cpuRegs.cycle, eeScaleBlockCycles());
	JMP32((uptr)DispatcherReg - ( (uptr)x86Ptr[0] + 5 ));
	x86SetJ8(j8Ptr[0]);
	//branch = 2;
}

////////////////////////////////////////////////////
void recBREAK( void ) {
	MOV32ItoM( (uptr)&cpuRegs.code, cpuRegs.code );
	MOV32ItoM( (uptr)&cpuRegs.pc, pc );
	iFlushCall(FLUSH_EVERYTHING);
	CALLFunc( (uptr)R5900::Interpreter::OpcodeImpl::BREAK );

	CMP32ItoM((uptr)&cpuRegs.pc, pc);
	j8Ptr[0] = JE8(0);
	ADD32ItoM((uptr)&cpuRegs.cycle, eeScaleBlockCycles());
	RET();
	x86SetJ8(j8Ptr[0]);
	//branch = 2;
}

} } }		// end namespace R5900::Dynarec::OpcodeImpl

////////////////////////////////////////////////////
void recClear( u32 Addr, u32 Size )
{
	u32 i;
	for(i = 0; i < Size; ++i, Addr+=4) {
		REC_CLEARM(Addr);
	}
}

static const int EE_MIN_BLOCK_BYTES = 15;

void recClearMem(BASEBLOCK* p)
{
	BASEBLOCKEX* pexblock;
	BASEBLOCK* pstart;
	int lastdelay;

	// necessary since recompiler doesn't call femms/emms
#ifdef __INTEL_COMPILER
		__asm__("emms");
#else
	#ifdef _MSC_VER
		if (cpucaps.has3DNOWInstructionExtensions) __asm femms;
		else __asm emms;
	#else
		if( cpucaps.has3DNOWInstructionExtensions )__asm__("femms");
		else 
			__asm__("emms");
	#endif
#endif
		
	assert( p != NULL );

	if( p->uType & BLOCKTYPE_DELAYSLOT ) {
		recClearMem(p-1);
		if( p->GetFnptr() == 0 )
			return;
	}

	assert( p->GetFnptr() != 0 );
	assert( p->startpc );

	x86Ptr[0] = (u8*)p->GetFnptr();

	// there is a small problem: mem can be ored with 0xa<<28 or 0x8<<28, and don't know which
	MOV32ItoR(EDX, p->startpc);
	PUSH32I((u32)x86Ptr[0]); // will be replaced by JMP32
	JMP32((u32)DispatcherClear - ( (u32)x86Ptr[0] + 5 ));
	assert( x86Ptr[0] == (u8*)p->GetFnptr() + EE_MIN_BLOCK_BYTES );

	pstart = PC_GETBLOCK(p->startpc);
	pexblock = PC_GETBLOCKEX(pstart);
	assert( pexblock->startpc == pstart->startpc );

    if( pexblock->startpc != pstart->startpc ) {
        // some bug with ffx after beating a big snake in sewers
        RemoveBaseBlockEx(pexblock, 0);
	    pexblock->size = 0;
	    pexblock->startpc = 0;
        return;
    }

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

	RemoveBaseBlockEx(pexblock, 0);
	pexblock->size = 0;
	pexblock->startpc = 0;
}

void REC_CLEARM( u32 mem )
{
	if ((mem) < maxrecmem && recLUT[(mem) >> 16]) {
		BASEBLOCK* p = PC_GETBLOCK(mem);
		if( *(u32*)p ) recClearMem(p);
	}
}

// check for end of bios
void CheckForBIOSEnd()
{
	MOV32MtoR(EAX, (int)&cpuRegs.pc);

	CMP32ItoR(EAX, 0x00200008);
	j8Ptr[0] = JE8(0);

	CMP32ItoR(EAX, 0x00100008);
	j8Ptr[1] = JE8(0);

	// return
	j8Ptr[2] = JMP8(0);

	x86SetJ8( j8Ptr[0] );
	x86SetJ8( j8Ptr[1] );

	// bios end
	RET();

	x86SetJ8( j8Ptr[2] );
}

static int *s_pCode;

void SetBranchReg( u32 reg )
{
	branch = 1;

	if( reg != 0xffffffff ) {
//		if( GPR_IS_CONST1(reg) )
//			MOV32ItoM( (uptr)&cpuRegs.pc, g_cpuConstRegs[reg].UL[0] );
//		else {
//			int mmreg;
//			
//			if( (mmreg = _checkXMMreg(XMMTYPE_GPRREG, reg, MODE_READ)) >= 0 ) {
//				SSE_MOVSS_XMM_to_M32((u32)&cpuRegs.pc, mmreg);
//			}
//			else if( (mmreg = _checkMMXreg(MMX_GPR+reg, MODE_READ)) >= 0 ) {
//				MOVDMMXtoM((u32)&cpuRegs.pc, mmreg);
//				SetMMXstate();
//			}
//			else {
//				MOV32MtoR(EAX, (int)&cpuRegs.GPR.r[ reg ].UL[ 0 ] );
//				MOV32RtoM((u32)&cpuRegs.pc, EAX);
//			}
//		}
		_allocX86reg(ESI, X86TYPE_PCWRITEBACK, 0, MODE_WRITE);
		_eeMoveGPRtoR(ESI, reg);

		recompileNextInstruction(1);

		if( x86regs[ESI].inuse ) {
			assert( x86regs[ESI].type == X86TYPE_PCWRITEBACK );
			MOV32RtoM((int)&cpuRegs.pc, ESI);
			x86regs[ESI].inuse = 0;
		}
		else {
			MOV32MtoR(EAX, (u32)&g_recWriteback);
			MOV32RtoM((int)&cpuRegs.pc, EAX);
		}
	}

//	CMP32ItoM((u32)&cpuRegs.pc, 0);
//	j8Ptr[5] = JNE8(0);
//	CALLFunc((uptr)tempfn);
//	x86SetJ8( j8Ptr[5] );

	iFlushCall(FLUSH_EVERYTHING);

	iBranchTest(0xffffffff);
}

void SetBranchImm( u32 imm )
{
	branch = 1;

	assert( imm );

	// end the current block
	MOV32ItoM( (uptr)&cpuRegs.pc, imm );
	iFlushCall(FLUSH_EVERYTHING);

	iBranchTest(imm);
}

void SaveBranchState()
{
	s_savex86FpuState = x86FpuState;
	s_saveiCWstate = iCWstate;
	s_savenBlockCycles = s_nBlockCycles;
	s_saveConstGPRreg = 0xffffffff; // indicate searching
	s_saveHasConstReg = g_cpuHasConstReg;
	s_saveFlushedConstReg = g_cpuFlushedConstReg;
	s_psaveInstInfo = g_pCurInstInfo;
	s_saveRegHasLive1 = g_cpuRegHasLive1;
	s_saveRegHasSignExt = g_cpuRegHasSignExt;

	// save all mmx regs
	memcpy_fast(s_saveMMXregs, mmxregs, sizeof(mmxregs));
	memcpy_fast(s_saveXMMregs, xmmregs, sizeof(xmmregs));
}

void LoadBranchState()
{
	x86FpuState = s_savex86FpuState;
	iCWstate = s_saveiCWstate;
	s_nBlockCycles = s_savenBlockCycles;

	if( s_saveConstGPRreg != 0xffffffff ) {
		assert( s_saveConstGPRreg > 0 );

		// make sure right GPR was saved
		assert( g_cpuHasConstReg == s_saveHasConstReg || (g_cpuHasConstReg ^ s_saveHasConstReg) == (1<<s_saveConstGPRreg) );

		// restore the GPR reg
		g_cpuConstRegs[s_saveConstGPRreg] = s_ConstGPRreg;
		GPR_SET_CONST(s_saveConstGPRreg);

		s_saveConstGPRreg = 0;
	}

	g_cpuHasConstReg = s_saveHasConstReg;
	g_cpuFlushedConstReg = s_saveFlushedConstReg;
	g_pCurInstInfo = s_psaveInstInfo;
	g_cpuRegHasLive1 = g_cpuPrevRegHasLive1 = s_saveRegHasLive1;
	g_cpuRegHasSignExt = g_cpuPrevRegHasSignExt = s_saveRegHasSignExt;

	// restore all mmx regs
	memcpy_fast(mmxregs, s_saveMMXregs, sizeof(mmxregs));
	memcpy_fast(xmmregs, s_saveXMMregs, sizeof(xmmregs));
}

void iFlushCall(int flushtype)
{
	_freeX86regs();

	if( flushtype & FLUSH_FREE_XMM )
		_freeXMMregs();
	else if( flushtype & FLUSH_FLUSH_XMM)
		_flushXMMregs();

	if( flushtype & FLUSH_FREE_MMX )
		_freeMMXregs();
	else if( flushtype & FLUSH_FLUSH_MMX)
		_flushMMXregs();

	if( flushtype & FLUSH_CACHED_REGS )
		_flushConstRegs();

	LoadCW();
	
	if (x86FpuState==MMX_STATE) {
		if (cpucaps.has3DNOWInstructionExtensions) FEMMS();
		else EMMS();
		x86FpuState=FPU_STATE;
	}
}

//void testfpu()
//{
//	int i;
//	for(i = 0; i < 32; ++i ) {
//		if( fpuRegs.fpr[i].UL== 0x7f800000 || fpuRegs.fpr[i].UL == 0xffc00000) {
//			SysPrintf("bad fpu: %x %x %x\n", i, cpuRegs.cycle, g_lastpc);
//		}
//
//		if( VU0.VF[i].UL[0] == 0xffc00000 || //(VU0.VF[i].UL[1]&0xffc00000) == 0xffc00000 ||
//			VU0.VF[i].UL[0] == 0x7f800000) {
//			SysPrintf("bad vu0: %x %x %x\n", i, cpuRegs.cycle, g_lastpc);
//		}
//	}
//}


u32 eeScaleBlockCycles()
{
	// Note: s_nBlockCycles is 3 bit fixed point.  Divide by 8 when done!

	// Let's not scale blocks under 5-ish cycles.  This fixes countless "problems"
	// caused by sync hacks and such, since games seem to care a lot more about
	// these small blocks having accurate cycle counts.

	if( s_nBlockCycles <= (5<<3) || (CHECK_EE_CYCLERATE == 0) )
		return s_nBlockCycles >> 3;

	uint scalarLow, scalarMid, scalarHigh;

	// Note: larger blocks get a smaller scalar, to help keep
	// them from becoming "too fat" and delaying branch tests.

	switch( CHECK_EE_CYCLERATE )
	{
		case 0:	return s_nBlockCycles >> 3;

		case 1:		// Sync hack x1.5!
			scalarLow = 5;
			scalarMid = 7;
			scalarHigh = 5;
		break;

		case 2:		// Sync hack x2
			scalarLow = 7;
			scalarMid = 9;
			scalarHigh = 7;
		break;

		case 3:		// Sync hack x3
			scalarLow = 10;
			scalarMid = 19;
			scalarHigh = 10;
		break;

		jNO_DEFAULT
	}

	s_nBlockCycles *= 
		(s_nBlockCycles <= (10<<3)) ? scalarLow :
		((s_nBlockCycles > (21<<3)) ? scalarHigh : scalarMid );

	return s_nBlockCycles >> (3+2);
}

// Generates dynarec code for Event tests followed by a block dispatch (branch).
// Parameters:
//   newpc - address to jump to at the end of the block.  If newpc == 0xffffffff then
//   the jump is assumed to be to a register (dynamic).  For any other value the
//   jump is assumed to be static, in which case the block will be "hardlinked" after
//   the first time it's dispatched.
// 
//   noDispatch - When set true, the jump to Dispatcher.  Used by the recs
//   for blocks which perform exception checks without branching (it's enabled by
//   setting "branch = 2";
static void iBranchTest(u32 newpc, bool noDispatch)
{
#ifdef _DEBUG
	//CALLFunc((uptr)testfpu);
#endif
	u32* ptr;

	if( bExecBIOS ) CheckForBIOSEnd();

	MOV32MtoR(EAX, (uptr)&cpuRegs.cycle);
	if( !noDispatch && newpc != 0xffffffff )
	{
		// Optimization note: Instructions order to pair EDX with EAX's load above.

		// Load EDX with the address of the JS32 jump below.
		// We do this because the the Dispatcher will use this info to modify
		// the JS instruction later on with the address of the block it's jumping
		// to; creating a static link of blocks that doesn't require the overhead
		// of a dispatcher.
		MOV32ItoR(EDX, 0);
		ptr = (u32*)(x86Ptr[0]-4);
	}

	// Check the Event scheduler if our "cycle target" has been reached.
	// Equiv code to:
	//    cpuRegs.cycle += blockcycles;
	//    if( cpuRegs.cycle > g_nextBranchCycle ) { DoEvents(); }
	ADD32ItoR(EAX, eeScaleBlockCycles());
	MOV32RtoM((uptr)&cpuRegs.cycle, EAX); // update cycles
	SUB32MtoR(EAX, (uptr)&g_nextBranchCycle);

	if( newpc != 0xffffffff )
	{
		// This is the jump instruction which gets modified by Dispatcher.
		*ptr = (u32)JS32((u32)Dispatcher - ( (u32)x86Ptr[0] + 6 ));
	}
	else if( !noDispatch )
	{
		// This instruction is a dynamic link, so it's never modified.
		JS32((uptr)DispatcherReg - ( (uptr)x86Ptr[0] + 6 ));
	}

	RET();
}

static void checkcodefn()
{
	int pctemp;

#ifdef _MSC_VER
	__asm mov pctemp, eax;
#else
    __asm__("movl %%eax, %0" : "=m"(pctemp) );
#endif

	Console::Error("code changed! %x", params pctemp);
	assert(0);
}

//#ifdef _DEBUG
//#define CHECK_XMMCHANGED() CALLFunc((uptr)checkxmmchanged);
//#else
//#define CHECK_XMMCHANGED()
//#endif
//
//static void checkxmmchanged()
//{
//	assert( !g_globalMMXSaved );
//	assert( !g_globalXMMSaved );
//}

u32 recompileCodeSafe(u32 temppc)
{
	BASEBLOCK* pblock = PC_GETBLOCK(temppc);

	if( pblock->GetFnptr() != 0 && pblock->startpc != s_pCurBlock->startpc ) {
		if( pc == pblock->startpc )
			return 0;
	}

	return 1;
}

void recompileNextInstruction(int delayslot)
{
	static u8 s_bFlushReg = 1;
	int i, count;

	BASEBLOCK* pblock = PC_GETBLOCK(pc);

	// need *ppblock != s_pCurBlock because of branches
	if( pblock->GetFnptr() != 0 && pblock->startpc != s_pCurBlock->startpc ) {

		if( !delayslot && pc == pblock->startpc ) {
			// code already in place, so jump to it and exit recomp
			assert( PC_GETBLOCKEX(pblock)->startpc == pblock->startpc );
			
			iFlushCall(FLUSH_EVERYTHING);
			MOV32ItoM((uptr)&cpuRegs.pc, pc);
				
//			if( pexblock->pOldFnptr ) {
//				// code already in place, so jump to it and exit recomp
//				JMP32((u32)pexblock->pOldFnptr - ((u32)x86Ptr[0] + 5));
//				branch = 3;
//				return;
//			}
			
			JMP32((uptr)pblock->GetFnptr() - ((uptr)x86Ptr[0] + 5));
			branch = 3;
			return;
		}
		else {

			if( !(delayslot && pblock->startpc == pc) ) {
				u8* oldX86 = x86Ptr[0];
				//__Log("clear block %x\n", pblock->startpc);
				recClearMem(pblock);
				x86Ptr[0] = oldX86;
				if( delayslot )
					Console::Notice("delay slot %x", params pc);
			}
		}
	}

	if( delayslot )
		pblock->uType = BLOCKTYPE_DELAYSLOT;
		
	s_pCode = (int *)PSM( pc );
	assert(s_pCode);

#ifdef _DEBUG
	MOV32ItoR(EAX, pc);
#endif

	cpuRegs.code = *(int *)s_pCode;
	pc += 4;
	
//#ifdef _DEBUG
//	CMP32ItoM((u32)s_pCode, cpuRegs.code);
//	j8Ptr[0] = JE8(0);
//	MOV32ItoR(EAX, pc);
//	CALLFunc((uptr)checkcodefn);
//	x86SetJ8( j8Ptr[ 0 ] );
//
//	if( !delayslot ) {
//		CMP32ItoM((u32)&cpuRegs.pc, s_pCurBlockEx->startpc);
//		j8Ptr[0] = JB8(0);
//		CMP32ItoM((u32)&cpuRegs.pc, pc);
//		j8Ptr[1] = JA8(0);
//		j8Ptr[2] = JMP8(0);
//		x86SetJ8( j8Ptr[ 0 ] );
//		x86SetJ8( j8Ptr[ 1 ] );
//		PUSH32I(s_pCurBlockEx->startpc);
//		ADD32ItoR(ESP, 4);
//		x86SetJ8( j8Ptr[ 2 ] );	
//	}
//#endif

	g_pCurInstInfo++;

	// reorder register priorities
//	for(i = 0; i < X86REGS; ++i) {
//		if( x86regs[i].inuse ) {
//			if( count > 0 ) mmxregs[i].counter = 1000-count;
//			else mmxregs[i].counter = 0;
//		}
//	}

	for(i = 0; i < MMXREGS; ++i) {
		if( mmxregs[i].inuse ) {
			assert( MMX_ISGPR(mmxregs[i].reg) );
			count = _recIsRegWritten(g_pCurInstInfo, (s_nEndBlock-pc)/4 + 1, XMMTYPE_GPRREG, mmxregs[i].reg-MMX_GPR);
			if( count > 0 ) mmxregs[i].counter = 1000-count;
			else mmxregs[i].counter = 0;
		}
	}

	for(i = 0; i < XMMREGS; ++i) {
		if( xmmregs[i].inuse ) {
			count = _recIsRegWritten(g_pCurInstInfo, (s_nEndBlock-pc)/4 + 1, xmmregs[i].type, xmmregs[i].reg);
			if( count > 0 ) xmmregs[i].counter = 1000-count;
			else xmmregs[i].counter = 0;
		}
	}

	const OPCODE& opcode = GetCurrentInstruction();

	// peephole optimizations
#ifdef PCSX2_VM_COISSUE
	if( g_pCurInstInfo->info & EEINSTINFO_COREC ) {

		if( g_pCurInstInfo->numpeeps > 1 ) {
			switch(_Opcode_) {
				case 30: recLQ_coX(g_pCurInstInfo->numpeeps); break;
				case 31: recSQ_coX(g_pCurInstInfo->numpeeps); break;
				case 49: recLWC1_coX(g_pCurInstInfo->numpeeps); break;
				case 57: recSWC1_coX(g_pCurInstInfo->numpeeps); break;
				case 55: recLD_coX(g_pCurInstInfo->numpeeps); break;
				case 63: recSD_coX(g_pCurInstInfo->numpeeps, 1); break; //not sure if should be set to 1 or 0; looks like "1" handles alignment, so i'm going with that for now

				jNO_DEFAULT
			}
			pc += g_pCurInstInfo->numpeeps*4;
			s_nBlockCycles += (g_pCurInstInfo->numpeeps+1) * opcode.cycles;
			g_pCurInstInfo += g_pCurInstInfo->numpeeps;
		}
		else {
			recBSC_co[_Opcode_]();
			pc += 4;
			g_pCurInstInfo++;
			s_nBlockCycles += opcode.cycles*2;
		}
	}
	else
#endif
	{
	 	//assert( !(g_pCurInstInfo->info & EEINSTINFO_NOREC) );

		// if this instruction is a jump or a branch, exit right away
		if( delayslot ) {
			switch(_Opcode_) {
				case 1:
					switch(_Rt_) {
						case 0: case 1: case 2: case 3: case 0x10: case 0x11: case 0x12: case 0x13:
							Console::Notice("branch %x in delay slot!", params cpuRegs.code);
							_clearNeededX86regs();
							_clearNeededMMXregs();
							_clearNeededXMMregs();
							return;
					}
					break;

				case 2: case 3: case 4: case 5: case 6: case 7: case 0x14: case 0x15: case 0x16: case 0x17:
					Console::Notice("branch %x in delay slot!", params cpuRegs.code);
					_clearNeededX86regs();
					_clearNeededMMXregs();
					_clearNeededXMMregs();
					return;
			}
		}
		opcode.recompile();
		s_nBlockCycles += opcode.cycles;
	}

	if( !delayslot ) {
		if( s_bFlushReg ) {
			//if( !_flushUnusedConstReg() ) {
				int flushed = 0;
				if( _getNumMMXwrite() > 3 ) flushed = _flushMMXunused();
				if( !flushed && _getNumXMMwrite() > 2 ) _flushXMMunused();
				s_bFlushReg = !flushed;
//			}
//			else s_bFlushReg = 0;
		}
		else s_bFlushReg = 1;
	}
	else s_bFlushReg = 1;

	//CHECK_XMMCHANGED();
	_clearNeededX86regs();
	_clearNeededMMXregs();
	_clearNeededXMMregs();

//	_freeXMMregs();
//	_freeMMXregs();
//	_flushCachedRegs();
//	g_cpuHasConstReg = 1;
}

extern u32 psxdump;

static void printfn()
{
	static int lastrec = 0;
	static int curcount = 0;
	const int skip = 0;

	assert( !g_globalMMXSaved );
	assert( !g_globalXMMSaved );

    if( (dumplog&2) && g_lastpc != 0x81fc0 ) {//&& lastrec != g_lastpc ) {
		curcount++;

		if( curcount > skip ) {
			iDumpRegisters(g_lastpc, 1);
			curcount = 0;
		}

		lastrec = g_lastpc;
	}
}

u32 s_recblocks[] = {0};

void badespfn() {
	Console::Error("Bad esp!");
	assert(0);
}

void __fastcall dyna_block_discard(u32 start,u32 sz)
{
	DevCon::WriteLn("dyna_block_discard %08X , count %d", params start,sz);
	Cpu->Clear(start,sz);
}

void recRecompile( const u32 startpc )
{
	u32 i = 0;
	u32 branchTo;
	u32 willbranch3 = 0;
	u32* ptr;
	u32 usecop2;

#ifdef _DEBUG
    //dumplog |= 4;
    if( dumplog & 4 )
		iDumpRegisters(startpc, 0);	
#endif

	assert( startpc );

	// if recPtr reached the mem limit reset whole mem
	if ( ( (uptr)recPtr - (uptr)recMem ) >= REC_CACHEMEM-0x40000 || dumplog == 0xffffffff) {
		DevCon::WriteLn( "EE Recompiler data reset" );
		recResetEE();
	}
	if ( ( (uptr)recStackPtr - (uptr)recStack ) >= RECSTACK_SIZE-0x100 ) {
		DevCon::WriteLn("EE recompiler stack reset");
		recResetEE();
	}

	s_pCurBlock = PC_GETBLOCK(startpc);
	
	if( s_pCurBlock->GetFnptr() ) {
		// clear if already taken
		assert( s_pCurBlock->startpc < startpc );
		recClearMem(s_pCurBlock);	
	}

	if( s_pCurBlock->startpc == startpc ) {
		s_pCurBlockEx = PC_GETBLOCKEX(s_pCurBlock);
		assert( s_pCurBlockEx->startpc == startpc );
	}
	else {
		s_pCurBlockEx = NULL;
		for(i = 0; i < EE_NUMBLOCKS; ++i) {
			if( recBlocks[(i+s_nNextBlock)%EE_NUMBLOCKS].size == 0 ) {
				s_pCurBlockEx = recBlocks+(i+s_nNextBlock)%EE_NUMBLOCKS;
				s_nNextBlock = (i+s_nNextBlock+1)%EE_NUMBLOCKS;
				break;
			}
		}

		if( s_pCurBlockEx == NULL ) {
			//SysPrintf("ee reset (blocks)\n");
			recResetEE();
			s_nNextBlock = 0;
			s_pCurBlockEx = recBlocks;
		}

		s_pCurBlockEx->startpc = startpc;
	}

	x86SetPtr( recPtr );
	x86Align(16);
	recPtr = x86Ptr[0];
	s_pCurBlock->SetFnptr( (uptr)x86Ptr[0] );
	s_pCurBlock->startpc = startpc;

	branch = 0;

	// reset recomp state variables
	s_nBlockCycles = 0;
	pc = startpc;
	x86FpuState = FPU_STATE;
	iCWstate = 0;
	s_saveConstGPRreg = 0;
	g_cpuHasConstReg = g_cpuFlushedConstReg = 1;
	g_cpuPrevRegHasLive1 = g_cpuRegHasLive1 = 0xffffffff;
	g_cpuPrevRegHasSignExt = g_cpuRegHasSignExt = 0;
	assert( g_cpuConstRegs[0].UD[0] == 0 );

	_initX86regs();
	_initXMMregs();
	_initMMXregs();

#ifdef _DEBUG
	// for debugging purposes
	MOV32ItoM((uptr)&g_lastpc, pc);
	CALLFunc((uptr)printfn);

//	CMP32MtoR(EBP, (uptr)&s_uSaveEBP);
//	j8Ptr[0] = JE8(0);
//	CALLFunc((uptr)badespfn);
//	x86SetJ8(j8Ptr[0]);
#endif

	// go until the next branch
	i = startpc;
	s_nEndBlock = 0xffffffff;
	s_nHasDelay = 0;
	
	while(1) {
		BASEBLOCK* pblock = PC_GETBLOCK(i);
		if( pblock->GetFnptr() != 0 && pblock->startpc != s_pCurBlock->startpc ) {

			if( i == pblock->startpc ) {
				// branch = 3
				willbranch3 = 1;
				s_nEndBlock = i;
				break;
			}
		}
		//HUH ? PSM ? whut ? THIS IS VIRTUAL ACCESS GOD DAMMIT
		cpuRegs.code = *(int *)PSM(i);

		switch(cpuRegs.code >> 26) {
			case 0: // special

				if( _Funct_ == 8 || _Funct_ == 9 ) { // JR, JALR
					s_nEndBlock = i + 8;
					s_nHasDelay = 1;
					goto StartRecomp;
				}

				break;
			case 1: // regimm
				
				if( _Rt_ < 4 || (_Rt_ >= 16 && _Rt_ < 20) ) {
					// branches
					if( _Rt_ == 2 || _Rt_ == 3 || _Rt_ == 18 || _Rt_ == 19 ) s_nHasDelay = 1;
					else s_nHasDelay = 2;

					branchTo = _Imm_ * 4 + i + 4;
					if( branchTo > startpc && branchTo < i ) s_nEndBlock = branchTo;
					else  s_nEndBlock = i+8;

					goto StartRecomp;
				}

				break;

			case 2: // J
			case 3: // JAL
				s_nHasDelay = 1;
				s_nEndBlock = i + 8;
				goto StartRecomp;

			// branches
			case 4: case 5: case 6: case 7: 
			case 20: case 21: case 22: case 23:

				if( (cpuRegs.code >> 26) >= 20 ) s_nHasDelay = 1;
				else s_nHasDelay = 2;

				branchTo = _Imm_ * 4 + i + 4;
				if( branchTo > startpc && branchTo < i ) s_nEndBlock = branchTo;
				else  s_nEndBlock = i+8;
				
				goto StartRecomp;

			case 16: // cp0
				if( _Rs_ == 16 ) {
					if( _Funct_ == 24 ) { // eret
						s_nEndBlock = i+4;
						goto StartRecomp;
					}
				}
				// Fall through!
				// COP0's branch opcodes line up with COP1 and COP2's

			case 17: // cp1
			case 18: // cp2
				if( _Rs_ == 8 ) {
					// BC1F, BC1T, BC1FL, BC1TL
					// BC2F, BC2T, BC2FL, BC2TL
					if( _Rt_ >= 2 ) s_nHasDelay = 1;
					else s_nHasDelay = 2;

					branchTo = _Imm_ * 4 + i + 4;
					if( branchTo > startpc && branchTo < i ) s_nEndBlock = branchTo;
					else  s_nEndBlock = i+8;
					
					goto StartRecomp;
				}
				break;
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
			cpuRegs.code = *(int *)PSM(i-4);
			pcur[-1] = pcur[0];

			BSCPropagate bsc( pcur[-1], pcur[0] );
			bsc.rprop();
			pcur--;
		}
	}

	// analyze instructions //
	{
		usecop2 = 0;
		g_pCurInstInfo = s_pInstCache;

		for(i = startpc; i < s_nEndBlock; i += 4) {
			g_pCurInstInfo++;
			cpuRegs.code = *(u32*)PSM(i);

			// cop2 //
			if( g_pCurInstInfo->info & EEINSTINFO_COP2 ) {

				if( !usecop2 ) {
					// init
					vucycle = 0;
					usecop2 = 1;
				}
				
				VU0.code = cpuRegs.code;
				_vuRegsCOP22( &VU0, &g_pCurInstInfo->vuregs );
				continue;
			}

			// fixme - This should be based on the cycle count of the current EE
			// instruction being analyzed.
			if( usecop2 ) vucycle++;

			// peephole optimizations //
#ifdef PCSX2_VM_COISSUE
			if( i < s_nEndBlock-4 && recompileCodeSafe(i) ) {
				u32 curcode = cpuRegs.code;
				u32 nextcode = *(u32*)PSM(i+4);
				if( _eeIsLoadStoreCoIssue(curcode, nextcode) && recBSC_co[curcode>>26] != NULL ) {

					// rs has to be the same, and cannot be just written
					if( ((curcode >> 21) & 0x1F) == ((nextcode >> 21) & 0x1F) && !_eeLoadWritesRs(curcode) ) {

						if( _eeIsLoadStoreCoX(curcode) && ((nextcode>>16)&0x1f) != ((curcode>>21)&0x1f) ) {
							// see how many stores there are
							u32 j;
							// use xmmregs since only supporting lwc1,lq,swc1,sq
							for(j = i+8; j < s_nEndBlock && j < i+4*XMMREGS; j += 4 ) {
								u32 nncode = *(u32*)PSM(j);
								if( (nncode>>26) != (curcode>>26) || ((curcode>>21)&0x1f) != ((nncode>>21)&0x1f) ||
									_eeLoadWritesRs(nncode))
									break;
							}

							if( j > i+8 ) {
								u32 num = (j-i)>>2; // number of stores that can coissue
								assert( num <= XMMREGS );

								g_pCurInstInfo[0].numpeeps = num-1;
								g_pCurInstInfo[0].info |= EEINSTINFO_COREC;

								while(i < j-4) {
									g_pCurInstInfo++;
									g_pCurInstInfo[0].info |= EEINSTINFO_NOREC;
									i += 4;	
								}

								continue;
							}

							// fall through
						}

						// unaligned loadstores

						// if LWL, check if LWR and that offsets are +3 away
						switch(curcode >> 26) {
							case 0x22: // LWL
								if( (nextcode>>26) != 0x26 || ((s16)nextcode)+3 != (s16)curcode )
									continue;
								break;
							case 0x26: // LWR
								if( (nextcode>>26) != 0x22 || ((s16)nextcode) != (s16)curcode+3 )
									continue;
								break;

							case 0x2a: // SWL
								if( (nextcode>>26) != 0x2e || ((s16)nextcode)+3 != (s16)curcode )
									continue;
								break;
							case 0x2e: // SWR
								if( (nextcode>>26) != 0x2a || ((s16)nextcode) != (s16)curcode+3 )
									continue;
								break;

							case 0x1a: // LDL
								if( (nextcode>>26) != 0x1b || ((s16)nextcode)+7 != (s16)curcode )
									continue;
								break;
							case 0x1b: // LWR
								if( (nextcode>>26) != 0x1aa || ((s16)nextcode) != (s16)curcode+7 )
									continue;
								break;

							case 0x2c: // SWL
								if( (nextcode>>26) != 0x2d || ((s16)nextcode)+7 != (s16)curcode )
									continue;
								break;
							case 0x2d: // SWR
								if( (nextcode>>26) != 0x2c || ((s16)nextcode) != (s16)curcode+7 )
									continue;
								break;
						}
						
						// good enough
						g_pCurInstInfo[0].info |= EEINSTINFO_COREC;
						g_pCurInstInfo[0].numpeeps = 1;
						g_pCurInstInfo[1].info |= EEINSTINFO_NOREC;
						g_pCurInstInfo++;
						i += 4;
						continue;
					}
				}
			}
#endif // end peephole
		}
		// This *is* important because g_pCurInstInfo is checked a bit later on and
		// if it's not equal to s_pInstCache it handles recompilation differently.
		// ... but the empty if() conditional inside the for loop is still amusing. >_<
		if( usecop2 ) {
			// add necessary mac writebacks
			g_pCurInstInfo = s_pInstCache;

			for(i = startpc; i < s_nEndBlock-4; i += 4) {
				g_pCurInstInfo++;

				if( g_pCurInstInfo->info & EEINSTINFO_COP2 ) {
				}
			}
		}
	}

#ifdef _DEBUG
	// dump code
	for(i = 0; i < ArraySize(s_recblocks); ++i) {
		if( startpc == s_recblocks[i] ) {
			iDumpBlock(startpc, recPtr);
		}
	}

	if( (dumplog & 1) ) //|| usecop2 )
		iDumpBlock(startpc, recPtr);
#endif

	u32 sz=(s_nEndBlock-startpc)>>2;
#ifdef lulz
	/*
		Block checking (ADDED BY RAZ-TEMP)
	*/
	
	MOV32ItoR(ECX,startpc);
	MOV32ItoR(EDX,sz);

#endif

	u32 inpage_offs=startpc&0xFFF;
	u32 inpage_ptr=startpc;
	u32 inpage_sz=sz*4;

	MOV32ItoR(ECX,startpc);
	MOV32ItoR(EDX,sz);

	while(inpage_sz)
	{
		int PageType=mmap_GetRamPageInfo((u32*)PSM(inpage_ptr));
		u32 pgsz=std::min(0x1000-inpage_offs,inpage_sz);

		if(PageType!=-1)
		{
			if (PageType==0)
			{
				//MOV32ItoR(EAX,*pageVer);
				//CMP32MtoR(EAX,(uptr)pageVer);
				//JNE32(((u32)dyna_block_discard_recmem)- ( (u32)x86Ptr[0] + 6 ));

				mmap_MarkCountedRamPage(PSM(inpage_ptr),inpage_ptr&~0xFFF);
			}
			else
			{
				u32 lpc=inpage_ptr;
				u32 stg=pgsz;
				while(stg>0)
				{
					// was dyna_block_discard_recmem.  See note in recResetEE for details.
					CMP32ItoM((uptr)PSM(lpc),*(u32*)PSM(lpc));
					JNE32(((u32)&dyna_block_discard)- ( (u32)x86Ptr[0] + 6 ));

					stg-=4;
					lpc+=4;
				}
				DbgCon::WriteLn("Manual block @ %08X : %08X %d %d %d %d", params
					startpc,inpage_ptr,pgsz,0x1000-inpage_offs,inpage_sz,sz*4);
			}
		}
		inpage_ptr+=pgsz;
		inpage_sz-=pgsz;
		inpage_offs=inpage_ptr&0xFFF;
	}

	// finally recompile //
	g_pCurInstInfo = s_pInstCache;
	while (!branch && pc < s_nEndBlock) {
		recompileNextInstruction(0);
	}

#ifdef _DEBUG
	if( (dumplog & 1) )
		iDumpBlock(startpc, recPtr);
#endif

	assert( (pc-startpc)>>2 <= 0xffff );
	s_pCurBlockEx->size = (pc-startpc)>>2;

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
	AddBaseBlockEx(s_pCurBlockEx, 0);
//	if( p[1].startpc == p[0].startpc + 4 ) {
//		assert( p[1].GetFnptr() != 0 );
//		// already fn in place, so add to list
//		AddBaseBlockEx(s_pCurBlockEx, 0);
//	}
//	else
//		*(BASEBLOCKEX**)(p+1) = pex;
//	}

	//PC_SETBLOCKEX(s_pCurBlock, s_pCurBlockEx);

	if( !(pc&0x10000000) )
		maxrecmem = std::max( (pc&~0xa0000000), maxrecmem );

	if( branch == 2 )
	{
		// Branch type 2 - This is how I "think" this works (air):
		// Performs a branch/event test but does not actually "break" the block.
		// This allows exceptions to be raised, and is thus sufficient for
		// certain types of things like SYSCALL, EI, etc.  but it is not sufficient
		// for actual branching instructions.

		iFlushCall(FLUSH_EVERYTHING);
		iBranchTest(0xffffffff, true);
	}
	else
	{
		assert( branch != 3 );
		if( branch )
			assert( !willbranch3 );
		else
			ADD32ItoM((int)&cpuRegs.cycle, eeScaleBlockCycles() );

		if( willbranch3 ) {
			BASEBLOCK* pblock = PC_GETBLOCK(s_nEndBlock);
			assert( pc == s_nEndBlock );
			iFlushCall(FLUSH_EVERYTHING);
			MOV32ItoM((uptr)&cpuRegs.pc, pc);
			JMP32((uptr)pblock->GetFnptr() - ((uptr)x86Ptr[0] + 5));
			branch = 3;
		}
		else if( !branch ) {
			// didn't branch, but had to stop
			MOV32ItoM( (uptr)&cpuRegs.pc, pc );

			iFlushCall(FLUSH_EVERYTHING);

			ptr = JMP32(0);
		}
	}

	assert( x86Ptr[0] >= (u8*)s_pCurBlock->GetFnptr() + EE_MIN_BLOCK_BYTES );
	assert( x86Ptr[0] < recMem+REC_CACHEMEM );
	assert( recStackPtr < recStack+RECSTACK_SIZE );
	assert( x86FpuState == 0 );

	recPtr = x86Ptr[0];

	assert( (g_cpuHasConstReg&g_cpuFlushedConstReg) == g_cpuHasConstReg );

	if( !branch ) {
		BASEBLOCK* pcurblock = s_pCurBlock;
		u32 nEndBlock = s_nEndBlock;
		s_pCurBlock = PC_GETBLOCK(pc);
		assert( ptr != NULL );
		
		if( s_pCurBlock->startpc != pc ) 
 			recRecompile(pc);

		if( pcurblock->startpc == startpc ) {
			assert( pcurblock->GetFnptr() );
			assert( s_pCurBlock->startpc == nEndBlock );
			*ptr = s_pCurBlock->GetFnptr() - ( (u32)ptr + 4 );
		}
		else {
			recRecompile(startpc);
			assert( pcurblock->GetFnptr() != 0 );
		}
	}
}

R5900cpu recCpu = {
	recAlloc,
	recResetEE,
	recStep,
	recExecute,
	recExecuteBlock,
	recClear,
	recShutdown
};
