#include <gtest/gtest.h>

#include "Emu/RSX/Common/simple_array.hpp"
#include "Emu/RSX/Program/Assembler/CFG.h"
#include "Emu/RSX/Program/Assembler/FPASM.h"
#include "Emu/RSX/Program/RSXFragmentProgram.h"

#include <util/v128.hpp>

namespace rsx::assembler
{
	static const BasicBlock* get_graph_block_by_id(const FlowGraph& graph, u32 id)
	{
		auto found = std::find_if(graph.blocks.begin(), graph.blocks.end(), FN(x.id == id));
		return &(*found);
	}
	TEST(CFG, FpToCFG_IF)
	{
		auto ir = FPIR::from_source(R"(
			ADD R0, R0, R0;
			MOV R1, R0;
			IF.LT;
				ADD R1, R1, R0;
			ENDIF;
			MOV R0, R1;
		)");

		const std::pair<int, size_t> expected_block_data[3] = {
			{ 0, 3 }, // Head
			{ 3, 1 }, // Branch
			{ 4, 1 }, // Merge
		};

		RSXFragmentProgram program{};
		auto bytecode = ir.compile();
		program.data = bytecode.data();

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
		EXPECT_EQ(get_graph_block_by_id(graph, 3)->pred[0].type, EdgeType::IF);
		EXPECT_EQ(get_graph_block_by_id(graph, 0)->succ[0].type, EdgeType::IF);
		EXPECT_EQ(get_graph_block_by_id(graph, 4)->pred[0].type, EdgeType::ENDIF);
	}

	TEST(CFG, FpToCFG_NestedIF)
	{
		auto ir = FPIR::from_source(
			"ADD R0, R0, R0;"       // 0
			"MOV R1, R0;"           // 1
			"IF.LT;"                // 2 (BR, 8)
			"	ADD R1, R1, R0;"    // 3
			"	IF.GT;"             // 4 (BR, 6)
			"		MOV R3, R0;"    // 5
			"	ENDIF;"
			"	MOV R2, R3;"        // 6 (merge block 1)
			"	ADD R1, R2, R1;"    // 7
			"ENDIF;"
			"MOV R0, R1;"           // 8 (merge block 2
		);

		const std::pair<int, size_t> expected_block_data[5] = {
			{ 0, 3 }, // Head
			{ 3, 2 }, // Branch 1
			{ 5, 1 }, // Branch 2
			{ 6, 2 }, // Merge 1
			{ 8, 1 }, // Merge 2
		};

		RSXFragmentProgram program{};
		auto bytecode = ir.compile();
		program.data = bytecode.data();

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
		auto ir = FPIR::from_source(
			"ADD R0, R0, R0;"       // 0
			"MOV R1, R0;"           // 1
			"IF.LT;"                // 2 (BR, 6)
			"	ADD R1, R1, R0;"    // 3
			"	IF.GT;"             // 4 (BR, 6)
			"		MOV R3, R0;"    // 5
			"	ENDIF;"             // ENDIF (4)
			"ENDIF;"                // ENDIF (2)
			"MOV R2, R3;"           // 6 (merge block, unified)
			"ADD R1, R2, R1;"       // 7
			"MOV R0, R1;"           // 8
		);

		const std::pair<int, size_t> expected_block_data[4] = {
			{ 0, 3 }, // Head
			{ 3, 2 }, // Branch 1
			{ 5, 1 }, // Branch 2
			{ 6, 3 }, // Merge
		};

		RSXFragmentProgram program{};
		auto bytecode = ir.compile();
		program.data = bytecode.data();

		FlowGraph graph = deconstruct_fragment_program(program);

		ASSERT_EQ(graph.blocks.size(), 4);

		int i = 0;
		for (auto it = graph.blocks.begin(); it != graph.blocks.end(); ++it)
		{
			const auto& expected = expected_block_data[i++];
			EXPECT_EQ(it->id, expected.first);
			EXPECT_EQ(it->instructions.size(), expected.second);
		}

		const BasicBlock
			*bb0 = get_graph_block_by_id(graph, 0),
			*bb6 = get_graph_block_by_id(graph, 6);

		// Predecessors must be ordered, closest first
		ASSERT_EQ(bb6->pred.size(), 3);
		EXPECT_EQ(bb6->pred[0].type, EdgeType::ENDIF);
		EXPECT_EQ(bb6->pred[0].from->id, 5);
		EXPECT_EQ(bb6->pred[1].type, EdgeType::ENDIF);
		EXPECT_EQ(bb6->pred[1].from->id, 3);
		EXPECT_EQ(bb6->pred[2].type, EdgeType::ENDIF);
		EXPECT_EQ(bb6->pred[2].from->id, 0);

		// Successors must also be ordered, closest first
		ASSERT_EQ(bb0->succ.size(), 2);
		EXPECT_EQ(bb0->succ[0].type, EdgeType::IF);
		EXPECT_EQ(bb0->succ[0].to->id, 3);
		EXPECT_EQ(bb0->succ[1].type, EdgeType::ENDIF);
		EXPECT_EQ(bb0->succ[1].to->id, 6);
	}

	TEST(CFG, FpToCFG_IF_ELSE)
	{
		auto ir = FPIR::from_source(
			"ADD R0, R0, R0;"       // 0
			"MOV R1, R0;"           // 1
			"IF.LT;"                // 2 (BR, 6)
			"	ADD R1, R1, R0;"    // 3
			"ELSE;"                 // ELSE (2)
			"	MOV R2, R3;"        // 4
			"	ADD R1, R2, R1;"    // 5
			"ENDIF;"                // ENDIF (2)
			"MOV R0, R1;"           // 6 (merge)
		);

		const std::pair<int, size_t> expected_block_data[4] = {
			{ 0, 3 }, // Head
			{ 3, 1 }, // Branch positive
			{ 4, 2 }, // Branch negative
			{ 6, 1 }, // Merge
		};

		RSXFragmentProgram program{};
		auto bytecode = ir.compile();
		program.data = bytecode.data();

		FlowGraph graph = deconstruct_fragment_program(program);

		ASSERT_EQ(graph.blocks.size(), 4);

		int i = 0;
		for (auto it = graph.blocks.begin(); it != graph.blocks.end(); ++it)
		{
			const auto& expected = expected_block_data[i++];
			EXPECT_EQ(it->id, expected.first);
			EXPECT_EQ(it->instructions.size(), expected.second);
		}

		// The IF and ELSE branches don't link to each other directly. Their predecessor should point to both and they both point to the merge.
		const BasicBlock
			*bb0 = get_graph_block_by_id(graph, 0),
			*bb3 = get_graph_block_by_id(graph, 3),
			*bb4 = get_graph_block_by_id(graph, 4),
			*bb6 = get_graph_block_by_id(graph, 6);

		EXPECT_EQ(bb0->succ.size(), 3);
		EXPECT_EQ(bb3->succ.size(), 1);
		EXPECT_EQ(bb4->succ.size(), 1);

		EXPECT_EQ(bb3->succ.front().to, bb6);
		EXPECT_EQ(bb4->succ.front().to, bb6);

		EXPECT_EQ(bb6->pred.size(), 3);
		EXPECT_EQ(bb6->pred[0].from, bb4);
		EXPECT_EQ(bb6->pred[1].from, bb3);
		EXPECT_EQ(bb6->pred[2].from, bb0);
	}

	TEST(CFG, FpToCFG_EmptyIF)
	{
		auto ir = FPIR::from_source(
			"IF.LT;"                // Empty branch
			"ENDIF;"
			"MOV R0, R1;"           // False merge block.
		);

		RSXFragmentProgram program{};
		auto bytecode = ir.compile();
		program.data = bytecode.data();

		FlowGraph graph = deconstruct_fragment_program(program);

		ASSERT_EQ(graph.blocks.size(), 1);
		EXPECT_EQ(graph.blocks.front().instructions.size(), 1);
	}
}
