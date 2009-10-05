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

#ifndef __IR5900_H__
#define __IR5900_H__

#include "x86emitter/x86emitter.h"
#include "x86emitter/sse_helpers.h"
#include "R5900.h"
#include "VU.h"
#include "iCore.h"

#define PC_GETBLOCK(x) PC_GETBLOCK_(x, recLUT)

extern u32 pc;
extern int branch;
extern uptr recLUT[];

extern u32 maxrecmem;
extern u32 pc;			         // recompiler pc (also used by the SuperVU! .. why? (air))
extern int branch;		         // set for branch (also used by the SuperVU! .. why? (air))
extern u32 target;		         // branch target
extern u32 s_nBlockCycles;		// cycles of current block recompiling

//////////////////////////////////////////////////////////////////////////////////////////
//

#define REC_FUNC( f ) \
   void rec##f( void ) \
   { \
	   MOV32ItoM( (uptr)&cpuRegs.code, (u32)cpuRegs.code ); \
	   MOV32ItoM( (uptr)&cpuRegs.pc, (u32)pc ); \
	   iFlushCall(FLUSH_EVERYTHING); \
	   CALLFunc( (uptr)Interp::f ); \
   }

#define REC_FUNC_DEL( f, delreg ) \
	void rec##f( void ) \
{ \
	MOV32ItoM( (uptr)&cpuRegs.code, (u32)cpuRegs.code ); \
	MOV32ItoM( (uptr)&cpuRegs.pc, (u32)pc ); \
	iFlushCall(FLUSH_EVERYTHING); \
	if( (delreg) > 0 ) _deleteEEreg(delreg, 0); \
	CALLFunc( (uptr)Interp::f ); \
}

#define REC_SYS( f ) \
   void rec##f( void ) \
   { \
	   MOV32ItoM( (uptr)&cpuRegs.code, (u32)cpuRegs.code ); \
	   MOV32ItoM( (uptr)&cpuRegs.pc, (u32)pc ); \
	   iFlushCall(FLUSH_EVERYTHING); \
	   CALLFunc( (uptr)Interp::f ); \
	   branch = 2; \
   }

#define REC_SYS_DEL( f, delreg ) \
   void rec##f( void ) \
   { \
	   MOV32ItoM( (uptr)&cpuRegs.code, (u32)cpuRegs.code ); \
	   MOV32ItoM( (uptr)&cpuRegs.pc, (u32)pc ); \
	   iFlushCall(FLUSH_EVERYTHING); \
	   if( (delreg) > 0 ) _deleteEEreg(delreg, 0); \
	   CALLFunc( (uptr)Interp::f ); \
	   branch = 2; \
   }


// Used to clear recompiled code blocks during memory/dma write operations.
u32 recClearMem(u32 pc);
u32 REC_CLEARM( u32 mem );

// used when processing branches
void SaveBranchState();
void LoadBranchState();

void recompileNextInstruction(int delayslot);
void SetBranchReg( u32 reg );
void SetBranchImm( u32 imm );

void iFlushCall(int flushtype);
void recBranchCall( void (*func)() );
void recCall( void (*func)(), int delreg );

namespace R5900{
namespace Dynarec {
extern void recDoBranchImm( u32* jmpSkip, bool isLikely = false );
extern void recDoBranchImm_Likely( u32* jmpSkip );
} }

////////////////////////////////////////////////////////////////////
// Constant Propagation - From here to the end of the header!

#define GPR_IS_CONST1(reg) ((reg)<32 && (g_cpuHasConstReg&(1<<(reg))))
#define GPR_IS_CONST2(reg1, reg2) ((g_cpuHasConstReg&(1<<(reg1)))&&(g_cpuHasConstReg&(1<<(reg2))))
#define GPR_SET_CONST(reg) { \
	if( (reg) < 32 ) { \
		g_cpuHasConstReg |= (1<<(reg)); \
		g_cpuFlushedConstReg &= ~(1<<(reg)); \
	} \
}

#define GPR_DEL_CONST(reg) { \
	if( (reg) < 32 ) g_cpuHasConstReg &= ~(1<<(reg)); \
}

extern __aligned16 GPR_reg64 g_cpuConstRegs[32];
extern u32 g_cpuHasConstReg, g_cpuFlushedConstReg;

// gets a memory pointer to the constant reg
u32* _eeGetConstReg(int reg);

// finds where the GPR is stored and moves lower 32 bits to EAX
void _eeMoveGPRtoR(x86IntRegType to, int fromgpr);
void _eeMoveGPRtoM(u32 to, int fromgpr);
void _eeMoveGPRtoRm(x86IntRegType to, int fromgpr);

void _eeFlushAllUnused();
void _eeOnWriteReg(int reg, int signext);

// totally deletes from const, xmm, and mmx entries
// if flush is 1, also flushes to memory
// if 0, only flushes if not an xmm reg (used when overwriting lower 64bits of reg)
void _deleteEEreg(int reg, int flush);

// allocates memory on the instruction size and returns the pointer
u32* recGetImm64(u32 hi, u32 lo);

void _vuRegsCOP22(VURegs * VU, _VURegsNum *VUregsn);

//////////////////////////////////////
// Templates for code recompilation //
//////////////////////////////////////

typedef void (*R5900FNPTR)();
typedef void (*R5900FNPTR_INFO)(int info);

#define EERECOMPILE_CODE0(fn, xmminfo) \
void rec##fn(void) \
{ \
	eeRecompileCode0(rec##fn##_const, rec##fn##_consts, rec##fn##_constt, rec##fn##_, xmminfo); \
}

#define EERECOMPILE_CODEX(codename, fn) \
void rec##fn(void) \
{ \
	codename(rec##fn##_const, rec##fn##_); \
}

//
// MMX/XMM caching helpers
//

// rd = rs op rt
void eeRecompileCode0(R5900FNPTR constcode, R5900FNPTR_INFO constscode, R5900FNPTR_INFO consttcode, R5900FNPTR_INFO noconstcode, int xmminfo);
// rt = rs op imm16
void eeRecompileCode1(R5900FNPTR constcode, R5900FNPTR_INFO noconstcode);
// rd = rt op sa
void eeRecompileCode2(R5900FNPTR constcode, R5900FNPTR_INFO noconstcode);
// rt op rs  (SPECIAL)
void eeRecompileCode3(R5900FNPTR constcode, R5900FNPTR_INFO multicode);

//
// non mmx/xmm version, slower
//
// rd = rs op rt
#define EERECOMPILE_CONSTCODE0(fn) \
void rec##fn(void) \
{ \
	eeRecompileCodeConst0(rec##fn##_const, rec##fn##_consts, rec##fn##_constt, rec##fn##_); \
} \

// rt = rs op imm16
#define EERECOMPILE_CONSTCODE1(fn) \
void rec##fn(void) \
{ \
	eeRecompileCodeConst1(rec##fn##_const, rec##fn##_); \
} \

// rd = rt op sa
#define EERECOMPILE_CONSTCODE2(fn) \
void rec##fn(void) \
{ \
	eeRecompileCodeConst2(rec##fn##_const, rec##fn##_); \
} \

// rd = rt op rs
#define EERECOMPILE_CONSTCODESPECIAL(fn, mult) \
void rec##fn(void) \
{ \
	eeRecompileCodeConstSPECIAL(rec##fn##_const, rec##fn##_, mult); \
} \

// rd = rs op rt
void eeRecompileCodeConst0(R5900FNPTR constcode, R5900FNPTR_INFO constscode, R5900FNPTR_INFO consttcode, R5900FNPTR_INFO noconstcode);
// rt = rs op imm16
void eeRecompileCodeConst1(R5900FNPTR constcode, R5900FNPTR_INFO noconstcode);
// rd = rt op sa
void eeRecompileCodeConst2(R5900FNPTR constcode, R5900FNPTR_INFO noconstcode);
// rd = rt MULT rs  (SPECIAL)
void eeRecompileCodeConstSPECIAL(R5900FNPTR constcode, R5900FNPTR_INFO multicode, int MULT);

// XMM caching helpers
#define XMMINFO_READLO	0x01
#define XMMINFO_READHI	0x02
#define XMMINFO_WRITELO	0x04
#define XMMINFO_WRITEHI	0x08
#define XMMINFO_WRITED	0x10
#define XMMINFO_READD	0x20
#define XMMINFO_READS	0x40
#define XMMINFO_READT	0x80
#define XMMINFO_READD_LO	0x100 // if set and XMMINFO_READD is set, reads only low 64 bits of D
#define XMMINFO_READACC		0x200
#define XMMINFO_WRITEACC	0x400

#define FPURECOMPILE_CONSTCODE(fn, xmminfo) \
void rec##fn(void) \
{ \
	if (CHECK_FPU_FULL) \
		eeFPURecompileCode(DOUBLE::rec##fn##_xmm, R5900::Interpreter::OpcodeImpl::COP1::fn, xmminfo); \
	else \
		eeFPURecompileCode(rec##fn##_xmm, R5900::Interpreter::OpcodeImpl::COP1::fn, xmminfo); \
}

// rd = rs op rt (all regs need to be in xmm)
int eeRecompileCodeXMM(int xmminfo);
void eeFPURecompileCode(R5900FNPTR_INFO xmmcode, R5900FNPTR fpucode, int xmminfo);


// For propagation of BSC stuff.
// Code implementations in ir5900tables.c
class BSCPropagate
{
protected:
	EEINST& prev;
	EEINST& pinst;

public:
	BSCPropagate( EEINST& previous, EEINST& pinstance );

	void rprop();

protected:
	void rpropSPECIAL();
	void rpropREGIMM();
	void rpropCP0();
	void rpropCP1();
	void rpropCP2();
	void rpropMMI();
	void rpropMMI0();
	void rpropMMI1();
	void rpropMMI2();
	void rpropMMI3();

	void rpropSetRead( int reg, int mask );
	void rpropSetFPURead( int reg, int mask );
	void rpropSetWrite( int reg, int mask );
	void rpropSetFPUWrite( int reg, int mask );

	template< int mask >
	void rpropSetRead( int reg );

	template< int live >
	void rpropSetWrite0( int reg, int mask );

	void rpropSetFast( int write1, int read1, int read2, int mask );

	template< int low, int hi >
	void rpropSetLOHI( int write1, int read1, int read2, int mask );

	template< int live >
	void rpropSetFPUWrite0( int reg, int mask );

};

#endif // __IR5900_H__
