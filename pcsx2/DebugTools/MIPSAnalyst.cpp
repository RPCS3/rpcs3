#include "PrecompiledHeader.h"
#include "MIPSAnalyst.h"
#include "Debug.h"
#include "DebugInterface.h"
#include "SymbolMap.h"
#include "DebugInterface.h"
#include "../R5900.h"

static std::vector<MIPSAnalyst::AnalyzedFunction> functions;

#define MIPS_MAKE_J(addr)   (0x08000000 | ((addr)>>2))
#define MIPS_MAKE_JAL(addr) (0x0C000000 | ((addr)>>2))
#define MIPS_MAKE_JR_RA()   (0x03e00008)
#define MIPS_MAKE_NOP()     (0)

namespace MIPSAnalyst
{
	
	static const char *DefaultFunctionName(char buffer[256], u32 startAddr) {
		sprintf(buffer, "z_un_%08x", startAddr);
		return buffer;
	}

	void ScanForFunctions(u32 startAddr, u32 endAddr, bool insertSymbols) {
		AnalyzedFunction currentFunction = {startAddr};

		u32 furthestBranch = 0;
		bool looking = false;
		bool end = false;
		bool isStraightLeaf = true;

		u32 addr;
		for (addr = startAddr; addr <= endAddr; addr += 4) {
			// Use pre-existing symbol map info if available. May be more reliable.
			SymbolInfo syminfo;
			if (symbolMap.GetSymbolInfo(&syminfo, addr, ST_FUNCTION)) {
				addr = syminfo.address + syminfo.size - 4;

				// We still need to insert the func for hashing purposes.
				currentFunction.start = syminfo.address;
				currentFunction.end = syminfo.address + syminfo.size - 4;
				functions.push_back(currentFunction);
				currentFunction.start = addr + 4;
				furthestBranch = 0;
				looking = false;
				end = false;
				continue;
			}

			u32 op = r5900Debug.read32(addr);
/*
			MIPSOpcode op = Memory::Read_Instruction(addr);
			u32 target = GetBranchTargetNoRA(addr);
			if (target != INVALIDTARGET) {
				isStraightLeaf = false;
				if (target > furthestBranch) {
					furthestBranch = target;
				}
			} else if ((op & 0xFC000000) == 0x08000000) {
				u32 sureTarget = GetJumpTarget(addr);
				// Check for a tail call.  Might not even have a jr ra.
				if (sureTarget != INVALIDTARGET && sureTarget < currentFunction.start) {
					if (furthestBranch > addr) {
						looking = true;
						addr += 4;
					} else {
						end = true;
					}
				} else if (sureTarget != INVALIDTARGET && sureTarget > addr && sureTarget > furthestBranch) {
					// A jump later.  Probably tail, but let's check if it jumps back.
					u32 knownEnd = furthestBranch == 0 ? addr : furthestBranch;
					u32 jumpback = ScanAheadForJumpback(sureTarget, currentFunction.start, knownEnd);
					if (jumpback != INVALIDTARGET && jumpback > addr && jumpback > knownEnd) {
						furthestBranch = jumpback;
					} else {
						if (furthestBranch > addr) {
							looking = true;
							addr += 4;
						} else {
							end = true;
						}
					}
				}
			}*/
			if (op == MIPS_MAKE_JR_RA()) {
				// If a branch goes to the jr ra, it's still ending here.
				if (furthestBranch > addr) {
					looking = true;
					addr += 4;
				} else {
					end = true;
				}
			}

	/*		if (looking) {
				if (addr >= furthestBranch) {
					u32 sureTarget = GetSureBranchTarget(addr);
					// Regular j only, jals are to new funcs.
					if (sureTarget == INVALIDTARGET && ((op & 0xFC000000) == 0x08000000)) {
						sureTarget = GetJumpTarget(addr);
					}

					if (sureTarget != INVALIDTARGET && sureTarget < addr) {
						end = true;
					} else if (sureTarget != INVALIDTARGET) {
						// Okay, we have a downward jump.  Might be an else or a tail call...
						// If there's a jump back upward in spitting distance of it, it's an else.
						u32 knownEnd = furthestBranch == 0 ? addr : furthestBranch;
						u32 jumpback = ScanAheadForJumpback(sureTarget, currentFunction.start, knownEnd);
						if (jumpback != INVALIDTARGET && jumpback > addr && jumpback > knownEnd) {
							furthestBranch = jumpback;
						}
					}
				}
			}
			*/
			if (end) {
				currentFunction.end = addr + 4;
				currentFunction.isStraightLeaf = isStraightLeaf;
				functions.push_back(currentFunction);
				furthestBranch = 0;
				addr += 4;
				looking = false;
				end = false;
				isStraightLeaf = true;
				currentFunction.start = addr+4;
			}
		}

		currentFunction.end = addr + 4;
		functions.push_back(currentFunction);

		for (auto iter = functions.begin(); iter != functions.end(); iter++) {
			iter->size = iter->end - iter->start + 4;
			if (insertSymbols) {
				char temp[256];
				symbolMap.AddFunction(DefaultFunctionName(temp, iter->start), iter->start, iter->end - iter->start + 4);
			}
		}
	}


	enum BranchType { NONE, JUMP, BRANCH };
	struct BranchInfo
	{
		BranchType type;
		bool link;
		bool likely;
		bool toRegister;
	};

	bool getBranchInfo(MipsOpcodeInfo& info)
	{
		BranchType type = NONE;
		bool link = false;
		bool likely = false;
		bool toRegister = false;
		bool met = false;

		u32 op = info.encodedOpcode;

		u32 opNum = MIPS_GET_OP(op);
		u32 rsNum = MIPS_GET_RS(op);
		u32 rtNum = MIPS_GET_RT(op);
		u32 rs = info.cpu->getRegister(0,rsNum);
		u32 rt = info.cpu->getRegister(0,rtNum);

		switch (MIPS_GET_OP(op))
		{
		case 0x00:		// special
			switch (MIPS_GET_FUNC(op))
			{
			case 0x08:	// jr
				type = JUMP;
				toRegister = true;
				break;
			case 0x09:	// jalr
				type = JUMP;
				toRegister = true;
				link = true;
				break;
			}
			break;
		case 0x01:		// regimm
			switch (MIPS_GET_RT(op))
			{
			case 0x00:		// bltz
			case 0x02:		// bltzl
			case 0x10:		// bltzal
			case 0x12:		// bltzall
				type = BRANCH;
				met = (((s32)rs) < 0);
				likely = (rt & 2) != 0;
				link = rt >= 0x10;
				break;
				
			case 0x01:		// bgez
			case 0x03:		// bgezl
			case 0x11:		// bgezal
			case 0x13:		// bgezall
				type = BRANCH;
				met = (((s32)rs) >= 0);
				likely = (rt & 2) != 0;
				link = rt >= 0x10;
				break;
			}
			break;
		case 0x02:		// j
			type = JUMP;
			break;
		case 0x03:		// jal
			type = JUMP;
			link = true;
			break;
			
		case 0x04:		// beq
		case 0x14:		// beql
			type = BRANCH;
			met = (rt == rs);
			if (MIPS_GET_RT(op) == MIPS_GET_RS(op))	// always true
				info.isConditional = false;
			likely = opNum >= 0x10;
			break;
			
		case 0x05:		// bne
		case 0x15:		// bnel
			type = BRANCH;
			met = (rt != rs);
			if (MIPS_GET_RT(op) == MIPS_GET_RS(op))	// always false
				info.isConditional = false;
			likely = opNum >= 0x10;
			break;
			
		case 0x06:		// blez
		case 0x16:		// blezl
			type = BRANCH;
			met = (((s32)rs) <= 0);
			likely = opNum >= 0x10;
			break;

		case 0x07:		// bgtz
		case 0x17:		// bgtzl
			type = BRANCH;
			met = (((s32)rs) > 0);
			likely = opNum >= 0x10;
			break;
		}

		if (type == NONE)
			return false;

		info.isBranch = true;
		info.isLinkedBranch = link;
		info.isLikelyBranch = true;
		info.isBranchToRegister = toRegister;
		info.isConditional = type == BRANCH;
		info.conditionMet = met;

		switch (type)
		{
		case JUMP:
			if (toRegister)
			{
				info.branchRegisterNum = (int)MIPS_GET_RS(op);
				info.branchTarget = info.cpu->getRegister(0,info.branchRegisterNum)._u32[0];
			} else {
				info.branchTarget =  (info.opcodeAddress & 0xF0000000) | ((op&0x03FFFFFF) << 2);
			}
			break;
		case BRANCH:
			info.branchTarget = info.opcodeAddress + 4 + ((signed short)(op&0xFFFF)<<2);
			break;
		}

		return true;
	}

	bool getDataAccessInfo(MipsOpcodeInfo& info)
	{
		int size = 0;
		
		u32 op = info.encodedOpcode;
		int off = 0;
		switch (MIPS_GET_OP(op))
		{
		case 0x20:		// lb
		case 0x24:		// lbu
		case 0x28:		// sb
			size = 1;
			break;
		case 0x21:		// lh
		case 0x25:		// lhu
		case 0x29:		// sh
			size = 2;
			break;
		case 0x23:		// lw
		case 0x26:		// lwr
		case 0x2B:		// sw
		case 0x2E:		// swr
			size = 4;
			break;
		case 0x22:		// lwl
		case 0x2A:		// swl
			size = 4;
			off = -3;
			break;
		case 0x37:		// ld
		case 0x1B:		// ldr
		case 0x3F:		// sd
		case 0x2D:		// sdr
			size = 8;
			break;
		case 0x1A:		// ldl
		case 0x2C:		// sdl
			size = 8;
			off = -7;
			break;
		}

		if (size == 0)
			return false;

		info.isDataAccess = true;
		info.dataSize = size;

		u32 rs = info.cpu->getRegister(0, (int)MIPS_GET_RS(op));
		s16 imm16 = op & 0xFFFF;
		info.dataAddress = rs + imm16 + off;

		info.hasRelevantAddress = true;
		info.releventAddress = info.dataAddress;
		return true;
	}

	MipsOpcodeInfo GetOpcodeInfo(DebugInterface* cpu, u32 address) {
		MipsOpcodeInfo info;
		memset(&info, 0, sizeof(info));

		if (cpu->isValidAddress(address) == false) {
			return info;
		}

		info.cpu = cpu;
		info.opcodeAddress = address;
		info.encodedOpcode = cpu->read32(address);
		u32 op = info.encodedOpcode;

		if (getBranchInfo(info) == true)
			return info;

		if (getDataAccessInfo(info) == true)
			return info;

		// gather relevant address for alu operations
		// that's usually the value of the dest register
		switch (MIPS_GET_OP(op)) {
		case 0:		// special
			switch (MIPS_GET_FUNC(op)) {
			case 0x0C:	// syscall
				info.isSyscall = true;
				info.branchTarget = 0x80000000+0x180;
				break;
			case 0x20:	// add
			case 0x21:	// addu
				info.hasRelevantAddress = true;
				info.releventAddress = cpu->getRegister(0,MIPS_GET_RS(op))._u32[0]+cpu->getRegister(0,MIPS_GET_RT(op))._u32[0];
				break;
			case 0x22:	// sub
			case 0x23:	// subu
				info.hasRelevantAddress = true;
				info.releventAddress = cpu->getRegister(0,MIPS_GET_RS(op))._u32[0]-cpu->getRegister(0,MIPS_GET_RT(op))._u32[0];
				break;
			}
			break;
		case 0x08:	// addi
		case 0x09:	// adiu
			info.hasRelevantAddress = true;
			info.releventAddress = cpu->getRegister(0,MIPS_GET_RS(op))._u32[0]+((s16)(op & 0xFFFF));
			break;
		case 0x10:	// cop0
			switch (MIPS_GET_RS(op))
			{
			case 0x10:	// tlb
				switch (MIPS_GET_FUNC(op))
				{
				case 0x18:	// eret
					info.isBranch = true;
					info.isConditional = false;

					// probably shouldn't be hard coded like this...
					if (cpuRegs.CP0.n.Status.b.ERL) {
						info.branchTarget = cpuRegs.CP0.n.ErrorEPC;
					} else {
						info.branchTarget = cpuRegs.CP0.n.EPC;
					}
					break;
				}
				break;
			}
			break;
		}

		// TODO: rest
/*		// movn, movz
		if (opInfo & IS_CONDMOVE) {
			info.isConditional = true;

			u32 rt = cpu->GetRegValue(0, (int)MIPS_GET_RT(op));
			switch (opInfo & CONDTYPE_MASK) {
			case CONDTYPE_EQ:
				info.conditionMet = (rt == 0);
				break;
			case CONDTYPE_NE:
				info.conditionMet = (rt != 0);
				break;
			}
		}*/

		return info;
	}
}
