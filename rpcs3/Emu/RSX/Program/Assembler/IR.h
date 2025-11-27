#pragma once

#include <util/asm.hpp>

namespace rsx::assembler
{
	struct BasicBlock;

	struct Register
	{
		int id = 0;
		bool f16 = false;
	};

	struct RegisterRef
	{
		Register reg{};

		// Vector information
		union
		{
			u32 mask;

			struct
			{
				bool x : 1;
				bool y : 1;
				bool z : 1;
				bool w : 1;
			};
		};
	};

	struct Instruction
	{
		// Raw data. Every instruction is max 128 bits.
		// Each instruction can also have 128 bits of literal/embedded data.
		u32 bytecode[8]{ {} };
		u32 addr = 0;

		// Decoded
		u32 opcode = 0;
		u8  length = 4;  // Length in dwords

		// Padding
		u8  reserved0 = 0;
		u16 reserved1 = 0;

		// References
		std::vector<RegisterRef> srcs;
		std::vector<RegisterRef> dsts;
	};

	enum class EdgeType
	{
		NONE,
		IF,
		ELSE,
		ENDIF,
		LOOP,
		ENDLOOP,
	};

	struct FlowEdge
	{
		EdgeType type = EdgeType::NONE;
		BasicBlock* from = nullptr;
		BasicBlock* to = nullptr;
	};

	struct BasicBlock
	{
		u32 id = 0;
		std::vector<Instruction> instructions; // Program instructions for the RSX processor
		std::vector<FlowEdge> succ;            // Forward edges. Sorted closest first.
		std::vector<FlowEdge> pred;            // Back edges. Sorted closest first.

		std::vector<Instruction> prologue;     // Prologue, created by passes
		std::vector<Instruction> epilogue;     // Epilogue, created by passes

		FlowEdge* insert_succ(BasicBlock* b, EdgeType type = EdgeType::NONE)
		{
			FlowEdge e{ .type = type, .from = this, .to = b };
			succ.push_back(e);
			return &succ.back();
		}

		FlowEdge* insert_pred(BasicBlock* b, EdgeType type = EdgeType::NONE)
		{
			FlowEdge e{ .type = type, .from = b, .to = this };
			pred.push_back(e);
			return &pred.back();
		}
	};
}
