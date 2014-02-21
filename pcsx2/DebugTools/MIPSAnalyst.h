#pragma once

class DebugInterface;


#define MIPS_GET_OP(op)   ((op>>26) & 0x3F)
#define MIPS_GET_FUNC(op) (op & 0x3F)
#define MIPS_GET_SA(op)   ((op>>6) & 0x1F)

#define MIPS_GET_RS(op) ((op>>21) & 0x1F)
#define MIPS_GET_RT(op) ((op>>16) & 0x1F)
#define MIPS_GET_RD(op) ((op>>11) & 0x1F)

namespace MIPSAnalyst
{
	struct AnalyzedFunction {
		u32 start;
		u32 end;
		u64 hash;
		u32 size;
		bool isStraightLeaf;
		bool hasHash;
		bool usesVFPU;
		char name[64];
	};

	void ScanForFunctions(u32 startAddr, u32 endAddr, bool insertSymbols);

	typedef struct {
		DebugInterface* cpu;
		u32 opcodeAddress;
		u32 encodedOpcode;
		
		// shared between branches and conditional moves
		bool isConditional;
		bool conditionMet;

		// branches
		u32 branchTarget;
		bool isBranch;
		bool isLinkedBranch;
		bool isLikelyBranch;
		bool isBranchToRegister;
		int branchRegisterNum;

		// data access
		bool isDataAccess;
		int dataSize;
		u32 dataAddress;

		bool hasRelevantAddress;
		u32 releventAddress;
	} MipsOpcodeInfo;
	
	MipsOpcodeInfo GetOpcodeInfo(DebugInterface* cpu, u32 address);
};
