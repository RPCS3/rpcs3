#include <gtest/gtest.h>

#include "Emu/RSX/Common/simple_array.hpp"
#include "Emu/RSX/Program/Assembler/CFG.h"
#include "Emu/RSX/Program/RSXFragmentProgram.h"

#include <util/v128.hpp>

namespace rsx::assembler
{
	auto swap_bytes16 = [](u32 dword) -> u32
	{
		// Lazy encode, but good enough for what we need here.
		union v32
		{
			u32 HEX;
			u8 _v[4];
		};

		u8* src_bytes = reinterpret_cast<u8*>(&dword);
		v32 dst_bytes;

		dst_bytes._v[0] = src_bytes[1];
		dst_bytes._v[1] = src_bytes[0];
		dst_bytes._v[2] = src_bytes[3];
		dst_bytes._v[3] = src_bytes[2];

		return dst_bytes.HEX;
	};

	// Instruction mocks because we don't have a working assember (yet)
	auto encode_instruction = [](u32 opcode, bool end = false) -> v128
	{
		OPDEST dst{};
		dst.opcode = opcode;

		if (end)
		{
			dst.end = 1;
		}

		return v128::from32(swap_bytes16(dst.HEX), 0, 0, 0);
	};

	auto create_if(u32 end, u32 _else = 0)
	{
		OPDEST dst{};
		dst.opcode = RSX_FP_OPCODE_IFE & 0x3Fu;

		SRC1 src1{};
		src1.else_offset = (_else ? _else : end) << 2;
		src1.opcode_is_branch = 1;

		SRC2 src2{};
		src2.end_offset = end << 2;

		return v128::from32(swap_bytes16(dst.HEX), 0, swap_bytes16(src1.HEX), swap_bytes16(src2.HEX));
	};

	TEST(CFG, FpToCFG_Basic)
	{
		rsx::simple_array<v128> buffer = {
			encode_instruction(RSX_FP_OPCODE_ADD),
			encode_instruction(RSX_FP_OPCODE_MOV, true)
		};

		RSXFragmentProgram program{};
		program.data = buffer.data();

		FlowGraph graph = deconstruct_fragment_program(program);

		EXPECT_EQ(graph.blocks.size(), 1);
		EXPECT_EQ(graph.blocks.front().instructions.size(), 2);
		EXPECT_EQ(graph.blocks.front().instructions.front().length, 4);
		EXPECT_EQ(graph.blocks.front().instructions[0].addr, 0);
		EXPECT_EQ(graph.blocks.front().instructions[1].addr, 16);
	}

	TEST(CFG, FpToCFG_IF)
	{
		rsx::simple_array<v128> buffer = {
			encode_instruction(RSX_FP_OPCODE_ADD),       // 0
			encode_instruction(RSX_FP_OPCODE_MOV),       // 1
			create_if(4),                                // 2 (BR, 4)
			encode_instruction(RSX_FP_OPCODE_ADD),       // 3
			encode_instruction(RSX_FP_OPCODE_MOV, true), // 4 (Merge block)
		};

		const std::pair<int, size_t> expected_block_data[3] = {
			{ 0, 3 }, // Head
			{ 3, 1 }, // Branch
			{ 4, 1 }, // Merge
		};

		RSXFragmentProgram program{};
		program.data = buffer.data();

		FlowGraph graph = deconstruct_fragment_program(program);

		ASSERT_EQ(graph.blocks.size(), 3);

		int i = 0;
		for (auto it = graph.blocks.begin(); it != graph.blocks.end(); ++it)
		{
			const auto& expected = expected_block_data[i++];
			EXPECT_EQ(it->id, expected.first);
			EXPECT_EQ(it->instructions.size(), expected.second);
		}

		// Check edges
		EXPECT_EQ(std::find_if(graph.blocks.begin(), graph.blocks.end(), FN(x.id == 3))->pred[0].type, EdgeType::IF);
		EXPECT_EQ(std::find_if(graph.blocks.begin(), graph.blocks.end(), FN(x.id == 0))->succ[0].type, EdgeType::IF);
		EXPECT_EQ(std::find_if(graph.blocks.begin(), graph.blocks.end(), FN(x.id == 4))->pred[0].type, EdgeType::ENDIF);
	}

	TEST(CFG, FpToCFG_NestedIF)
	{
		rsx::simple_array<v128> buffer = {
			encode_instruction(RSX_FP_OPCODE_ADD),       // 0
			encode_instruction(RSX_FP_OPCODE_MOV),       // 1
			create_if(8),                                // 2 (BR, 8)
			encode_instruction(RSX_FP_OPCODE_ADD),       // 3
			create_if(6),                                // 4 (BR, 6)
			encode_instruction(RSX_FP_OPCODE_MOV),       // 5
			encode_instruction(RSX_FP_OPCODE_MOV),       // 6 (merge block 1)
			encode_instruction(RSX_FP_OPCODE_ADD),       // 7
			encode_instruction(RSX_FP_OPCODE_MOV, true)  // 8 (merge block 2
		};

		const std::pair<int, size_t> expected_block_data[5] = {
			{ 0, 3 }, // Head
			{ 3, 2 }, // Branch 1
			{ 5, 1 }, // Branch 2
			{ 6, 2 }, // Merge 1
			{ 8, 1 }, // Merge 2
		};

		RSXFragmentProgram program{};
		program.data = buffer.data();

		FlowGraph graph = deconstruct_fragment_program(program);

		ASSERT_EQ(graph.blocks.size(), 5);

		int i = 0;
		for (auto it = graph.blocks.begin(); it != graph.blocks.end(); ++it)
		{
			const auto& expected = expected_block_data[i++];
			EXPECT_EQ(it->id, expected.first);
			EXPECT_EQ(it->instructions.size(), expected.second);
		}
	}

	TEST(CFG, FpToCFG_NestedIF_MultiplePred)
	{
		rsx::simple_array<v128> buffer = {
			encode_instruction(RSX_FP_OPCODE_ADD),       // 0
			encode_instruction(RSX_FP_OPCODE_MOV),       // 1
			create_if(6),                                // 2 (BR, 6)
			encode_instruction(RSX_FP_OPCODE_ADD),       // 3
			create_if(6),                                // 4 (BR, 6)
			encode_instruction(RSX_FP_OPCODE_MOV),       // 5
			encode_instruction(RSX_FP_OPCODE_MOV),       // 6 (merge block)
			encode_instruction(RSX_FP_OPCODE_ADD),       // 7
			encode_instruction(RSX_FP_OPCODE_MOV, true)  // 8
		};

		const std::pair<int, size_t> expected_block_data[4] = {
			{ 0, 3 }, // Head
			{ 3, 2 }, // Branch 1
			{ 5, 1 }, // Branch 2
			{ 6, 3 }, // Merge
		};

		RSXFragmentProgram program{};
		program.data = buffer.data();

		FlowGraph graph = deconstruct_fragment_program(program);

		ASSERT_EQ(graph.blocks.size(), 4);

		int i = 0;
		for (auto it = graph.blocks.begin(); it != graph.blocks.end(); ++it)
		{
			const auto& expected = expected_block_data[i++];
			EXPECT_EQ(it->id, expected.first);
			EXPECT_EQ(it->instructions.size(), expected.second);
		}

		// Predecessors must be ordered, closest first
		ASSERT_EQ(std::find_if(graph.blocks.begin(), graph.blocks.end(), FN(x.id == 6))->pred.size(), 2);
		EXPECT_EQ(std::find_if(graph.blocks.begin(), graph.blocks.end(), FN(x.id == 6))->pred[0].type, EdgeType::ENDIF);
		EXPECT_EQ(std::find_if(graph.blocks.begin(), graph.blocks.end(), FN(x.id == 6))->pred[0].from->id, 3);
		EXPECT_EQ(std::find_if(graph.blocks.begin(), graph.blocks.end(), FN(x.id == 6))->pred[1].type, EdgeType::ENDIF);
		EXPECT_EQ(std::find_if(graph.blocks.begin(), graph.blocks.end(), FN(x.id == 6))->pred[1].from->id, 0);

		// Successors must also be ordered, closest first
		ASSERT_EQ(std::find_if(graph.blocks.begin(), graph.blocks.end(), FN(x.id == 0))->succ.size(), 2);
		EXPECT_EQ(std::find_if(graph.blocks.begin(), graph.blocks.end(), FN(x.id == 0))->succ[0].type, EdgeType::IF);
		EXPECT_EQ(std::find_if(graph.blocks.begin(), graph.blocks.end(), FN(x.id == 0))->succ[0].to->id, 3);
		EXPECT_EQ(std::find_if(graph.blocks.begin(), graph.blocks.end(), FN(x.id == 0))->succ[1].type, EdgeType::ENDIF);
		EXPECT_EQ(std::find_if(graph.blocks.begin(), graph.blocks.end(), FN(x.id == 0))->succ[1].to->id, 6);
	}

	TEST(CFG, FpToCFG_IF_ELSE)
	{
		rsx::simple_array<v128> buffer = {
			encode_instruction(RSX_FP_OPCODE_ADD),       // 0
			encode_instruction(RSX_FP_OPCODE_MOV),       // 1
			create_if(6, 4),                             // 2 (BR, 6)
			encode_instruction(RSX_FP_OPCODE_ADD),       // 3
			encode_instruction(RSX_FP_OPCODE_MOV),       // 4 (Else)
			encode_instruction(RSX_FP_OPCODE_ADD),       // 5
			encode_instruction(RSX_FP_OPCODE_MOV, true), // 6 (Merge)
		};

		const std::pair<int, size_t> expected_block_data[4] = {
			{ 0, 3 }, // Head
			{ 3, 1 }, // Branch positive
			{ 4, 2 }, // Branch negative
			{ 6, 1 }, // Merge
		};

		RSXFragmentProgram program{};
		program.data = buffer.data();

		FlowGraph graph = deconstruct_fragment_program(program);

		ASSERT_EQ(graph.blocks.size(), 4);

		int i = 0;
		for (auto it = graph.blocks.begin(); it != graph.blocks.end(); ++it)
		{
			const auto& expected = expected_block_data[i++];
			EXPECT_EQ(it->id, expected.first);
			EXPECT_EQ(it->instructions.size(), expected.second);
		}
	}
}
