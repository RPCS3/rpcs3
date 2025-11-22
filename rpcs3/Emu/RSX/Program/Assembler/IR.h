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
		// Raw data. Every instruction is max 128 bits
		u32 bytecode[4];

		// Decoded
		u32 opcode = 0;
		std::vector<RegisterRef> srcs;
		std::vector<RegisterRef> dsts;
	};

	struct FlowEdge
	{
		BasicBlock* from = nullptr;
		BasicBlock* to = nullptr;
	};

	struct BasicBlock
	{
		u32 id = 0;
		std::vector<Instruction> instructions;
		std::vector<FlowEdge> succ;          // [0] = if/loop, [1] = else
		std::vector<FlowEdge> pred;          // Back edge.

		void insert_succ(BasicBlock* b)
		{
			FlowEdge e{ .from = this, .to = b };
			succ.push_back(e);
		}

		void insert_pred(BasicBlock* b)
		{
			FlowEdge e{ .from = this, .to = b };
			pred.push_back(e);
		}
	};
}
