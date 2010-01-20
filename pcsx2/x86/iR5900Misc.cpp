#include "PrecompiledHeader.h"

#include "Common.h"
#include "iR5900.h"
#include "R5900OpcodeTables.h"


namespace R5900 {
namespace Dynarec {

// R5900 branch helper!
// Recompiles code for a branch test and/or skip, complete with delay slot
// handling.  Note, for "likely" branches use iDoBranchImm_Likely instead, which
// handles delay slots differently.
// Parameters:
//   jmpSkip - This parameter is the result of the appropriate J32 instruction
//   (usually JZ32 or JNZ32).
void recDoBranchImm( u32* jmpSkip, bool isLikely )
{
	// All R5900 branches use this format:
	const u32 branchTo = ((s32)_Imm_ * 4) + pc;

	// First up is the Branch Taken Path : Save the recompiler's state, compile the
	// DelaySlot, and issue a BranchTest insertion.  The state is reloaded below for
	// the "did not branch" path (maintains consts, register allocations, and other optimizations).

	SaveBranchState();
	recompileNextInstruction(1);
	SetBranchImm(branchTo);

	// Jump target when the branch is *not* taken, skips the branchtest code
	// insertion above.
	x86SetJ32(jmpSkip);

	// if it's a likely branch then we'll need to skip the delay slot here, since
	// MIPS cancels the delay slot instruction when branches aren't taken.
	LoadBranchState();
	if( !isLikely )
	{
		pc -= 4;		// instruction rewinder for delay slot, if non-likely.
		recompileNextInstruction(1);
	}
	SetBranchImm(pc);	// start a new recompiled block.
}

void recDoBranchImm_Likely( u32* jmpSkip )
{
	recDoBranchImm( jmpSkip, true );
}

namespace OpcodeImpl {

////////////////////////////////////////////////////
//static void recCACHE( void ) {
//	MOV32ItoM( (uptr)&cpuRegs.code, cpuRegs.code );
//	MOV32ItoM( (uptr)&cpuRegs.pc, pc );
//	iFlushCall(FLUSH_EVERYTHING);
//	CALLFunc( (uptr)CACHE );
//	//branch = 2;
//
//	CMP32ItoM((int)&cpuRegs.pc, pc);
//	j8Ptr[0] = JE8(0);
//	RET();
//	x86SetJ8(j8Ptr[0]);
//}


void recPREF( void ) 
{
}

void recSYNC( void )
{
}

void recMFSA( void ) 
{
	int mmreg;
	if (!_Rd_) return;

	mmreg = _checkXMMreg(XMMTYPE_GPRREG, _Rd_, MODE_WRITE);
	if( mmreg >= 0 ) {
		SSE_MOVLPS_M64_to_XMM(mmreg, (uptr)&cpuRegs.sa);
	}
	else if( (mmreg = _checkMMXreg(MMX_GPR+_Rd_, MODE_WRITE)) >= 0 ) {
		MOVDMtoMMX(mmreg, (uptr)&cpuRegs.sa);
		SetMMXstate();
	}
	else {
		MOV32MtoR(EAX, (u32)&cpuRegs.sa);
		_deleteEEreg(_Rd_, 0);
		MOV32RtoM((uptr)&cpuRegs.GPR.r[_Rd_].UL[0], EAX);
		MOV32ItoM((uptr)&cpuRegs.GPR.r[_Rd_].UL[1], 0);
	}
}

// SA is 4-bit and contains the amount of bytes to shift
void recMTSA( void )
{
	if( GPR_IS_CONST1(_Rs_) ) {
		MOV32ItoM((uptr)&cpuRegs.sa, g_cpuConstRegs[_Rs_].UL[0] & 0xf );
	}
	else {
		int mmreg;
		
		if( (mmreg = _checkXMMreg(XMMTYPE_GPRREG, _Rs_, MODE_READ)) >= 0 ) {
			SSE_MOVSS_XMM_to_M32((uptr)&cpuRegs.sa, mmreg);
		}
		else if( (mmreg = _checkMMXreg(MMX_GPR+_Rs_, MODE_READ)) >= 0 ) {
			MOVDMMXtoM((uptr)&cpuRegs.sa, mmreg);
			SetMMXstate();
		}
		else {
			MOV32MtoR(EAX, (uptr)&cpuRegs.GPR.r[_Rs_].UL[0]);
			MOV32RtoM((uptr)&cpuRegs.sa, EAX);
		}
		AND32ItoM((uptr)&cpuRegs.sa, 0xf);
	}
}

void recMTSAB( void ) 
{
	if( GPR_IS_CONST1(_Rs_) ) {
		MOV32ItoM((uptr)&cpuRegs.sa, ((g_cpuConstRegs[_Rs_].UL[0] & 0xF) ^ (_Imm_ & 0xF)) );
	}
	else {
		_eeMoveGPRtoR(EAX, _Rs_);
		AND32ItoR(EAX, 0xF);
		XOR32ItoR(EAX, _Imm_&0xf);
		MOV32RtoM((uptr)&cpuRegs.sa, EAX);
	}
}

void recMTSAH( void ) 
{
	if( GPR_IS_CONST1(_Rs_) ) {
		MOV32ItoM((uptr)&cpuRegs.sa, ((g_cpuConstRegs[_Rs_].UL[0] & 0x7) ^ (_Imm_ & 0x7)) << 1);
	}
	else {
		_eeMoveGPRtoR(EAX, _Rs_);
		AND32ItoR(EAX, 0x7);
		XOR32ItoR(EAX, _Imm_&0x7);
		SHL32ItoR(EAX, 1);
		MOV32RtoM((uptr)&cpuRegs.sa, EAX);
	}
}

	////////////////////////////////////////////////////
	void recNULL( void )
	{
		Console.Error("EE: Unimplemented op %x", cpuRegs.code);
	}

	////////////////////////////////////////////////////
	void recUnknown()
	{
		// TODO : Unknown ops should throw an exception.
		Console.Error("EE: Unrecognized op %x", cpuRegs.code);
	}

	void recMMI_Unknown()
	{
		// TODO : Unknown ops should throw an exception.
		Console.Error("EE: Unrecognized MMI op %x", cpuRegs.code);
	}

	void recCOP0_Unknown()
	{
		// TODO : Unknown ops should throw an exception.
		Console.Error("EE: Unrecognized COP0 op %x", cpuRegs.code);
	}

	void recCOP1_Unknown()
	{
		// TODO : Unknown ops should throw an exception.
		Console.Error("EE: Unrecognized FPU/COP1 op %x", cpuRegs.code);
	}

	/**********************************************************
	*    UNHANDLED YET OPCODES
	*
	**********************************************************/

	// Suikoden 3 uses it a lot
	void recCACHE()
	{
	   //MOV32ItoM( (uptr)&cpuRegs.code, (u32)cpuRegs.code );
	   //MOV32ItoM( (uptr)&cpuRegs.pc, (u32)pc );
	   //iFlushCall(FLUSH_EVERYTHING);
	   //CALLFunc( (uptr)R5900::Interpreter::OpcodeImpl::CACHE );
	   //branch = 2;
	}

	void recTGE( void ) 
	{
		recBranchCall( R5900::Interpreter::OpcodeImpl::TGE );
	}

	void recTGEU( void ) 
	{
		recBranchCall( R5900::Interpreter::OpcodeImpl::TGEU );
	}

	void recTLT( void ) 
	{
		recBranchCall( R5900::Interpreter::OpcodeImpl::TLT );
	}

	void recTLTU( void ) 
	{
		recBranchCall( R5900::Interpreter::OpcodeImpl::TLTU );
	}

	void recTEQ( void ) 
	{
		recBranchCall( R5900::Interpreter::OpcodeImpl::TEQ );
	}

	void recTNE( void ) 
	{
		recBranchCall( R5900::Interpreter::OpcodeImpl::TNE );
	}

	void recTGEI( void )
	{
		recBranchCall( R5900::Interpreter::OpcodeImpl::TGEI );
	}

	void recTGEIU( void ) 
	{
		recBranchCall( R5900::Interpreter::OpcodeImpl::TGEIU );
	}

	void recTLTI( void ) 
	{
		recBranchCall( R5900::Interpreter::OpcodeImpl::TLTI );
	}

	void recTLTIU( void ) 
	{
		recBranchCall( R5900::Interpreter::OpcodeImpl::TLTIU );
	}

	void recTEQI( void ) 
	{
		recBranchCall( R5900::Interpreter::OpcodeImpl::TEQI );
	}

	void recTNEI( void ) 
	{
		recBranchCall( R5900::Interpreter::OpcodeImpl::TNEI );
	}

} }}		// end Namespace R5900::Dynarec::OpcodeImpl
