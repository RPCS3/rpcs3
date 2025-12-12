#include <gtest/gtest.h>

#include "Emu/RSX/Common/simple_array.hpp"
#include "Emu/RSX/Program/Assembler/FPASM.h"
#include "Emu/RSX/Program/Assembler/Passes/FP/RegisterAnnotationPass.h"
#include "Emu/RSX/Program/Assembler/Passes/FP/RegisterDependencyPass.h"
#include "Emu/RSX/Program/RSXFragmentProgram.h"

namespace rsx::assembler
{
#define DECLARE_REG32(num)\
	Register R##num{ .id = num, .f16 = false }

#define DECLARE_REG16(num)\
	Register H##num{ .id = num, .f16 = true }

	DECLARE_REG32(0);
	DECLARE_REG32(1);
	DECLARE_REG32(2);
	DECLARE_REG32(3);
	DECLARE_REG32(4);
	DECLARE_REG32(5);
	DECLARE_REG32(6);
	DECLARE_REG32(7);
	DECLARE_REG32(8);

	DECLARE_REG16(0);
	DECLARE_REG16(1);
	DECLARE_REG16(2);
	DECLARE_REG16(3);
	DECLARE_REG16(4);
	DECLARE_REG16(5);
	DECLARE_REG16(6);
	DECLARE_REG16(7);
	DECLARE_REG16(8);

#undef DECLARE_REG32
#undef DECLARE_REG16

	static const BasicBlock* get_graph_block(const FlowGraph& graph, u32 index)
	{
		ensure(index < graph.blocks.size());
		for (auto it = graph.blocks.begin(); it != graph.blocks.end(); ++it)
		{
			if (!index)
			{
				return &(*it);
			}
			index--;
		}
		return nullptr;
	};

	static FlowGraph CFG_from_source(const std::string& asm_)
	{
		auto ir = FPIR::from_source(asm_);

		FlowGraph graph{};
		graph.blocks.push_back({});

		auto& bb = graph.blocks.back();
		bb.instructions = ir.instructions();
		return graph;
	}

	TEST(TestFPIR, FromSource)
	{
		auto ir = FPIR::from_source(R"(
			MOV R0, #{ 0.125 };
			ADD R1, R0, R0;
		)");

		const auto instructions = ir.instructions();

		ASSERT_EQ(instructions.size(), 2);

		EXPECT_EQ(OPDEST{ .HEX = instructions[0].bytecode[0] }.end, 0);
		EXPECT_EQ(OPDEST{ .HEX = instructions[0].bytecode[0] }.opcode, RSX_FP_OPCODE_MOV);
		EXPECT_EQ(SRC0{ .HEX = instructions[0].bytecode[1] }.reg_type, RSX_FP_REGISTER_TYPE_CONSTANT);
		EXPECT_EQ(OPDEST{ .HEX = instructions[0].bytecode[0] }.opcode, RSX_FP_OPCODE_MOV);
		EXPECT_EQ(instructions[0].length, 8);

		EXPECT_EQ(OPDEST{ .HEX = instructions[1].bytecode[0] }.end, 1);
		EXPECT_EQ(OPDEST{ .HEX = instructions[1].bytecode[0] }.opcode, RSX_FP_OPCODE_ADD);
		EXPECT_EQ(OPDEST{ .HEX = instructions[1].bytecode[0] }.dest_reg, 1);
		EXPECT_EQ(OPDEST{ .HEX = instructions[1].bytecode[0] }.fp16, 0);
		EXPECT_EQ(SRC0{ .HEX = instructions[1].bytecode[1] }.reg_type, RSX_FP_REGISTER_TYPE_TEMP);
		EXPECT_EQ(instructions[1].length, 4);
	}

	TEST(TestFPIR, RegisterAnnotationPass)
	{
		// Code snippet reads from R0, R1 and H4, clobbers R1, H0
		auto graph = CFG_from_source(R"(
			ADD R1, R0, R1;
			MOV H0, H4;
		)");

		ASSERT_EQ(graph.blocks.size(), 1);
		ASSERT_EQ(graph.blocks.front().instructions.size(), 2);

		auto& block = graph.blocks.front();
		RSXFragmentProgram prog{};
		FP::RegisterAnnotationPass annotation_pass{ prog };

		annotation_pass.run(graph);

		ASSERT_EQ(block.clobber_list.size(), 2);
		ASSERT_EQ(block.input_list.size(), 3);

		EXPECT_EQ(block.clobber_list[0].reg, H0);
		EXPECT_EQ(block.clobber_list[1].reg, R1);

		EXPECT_EQ(block.input_list[0].reg, H4);
		EXPECT_EQ(block.input_list[1].reg, R0);
		EXPECT_EQ(block.input_list[2].reg, R1);
	}

	TEST(TestFPIR, RegisterAnnotationPass_MixedIO)
	{
		// Code snippet reads from R0, R1, clobbers R0, R1, H0.
		// The H2 read does not count because R1 is clobbered.
		auto graph = CFG_from_source(R"(
			ADD  R1, R0, R1;
			PK8U R0, R1;
			MOV  H0, H2;
		)");

		ASSERT_EQ(graph.blocks.size(), 1);
		ASSERT_EQ(graph.blocks.front().instructions.size(), 3);

		auto& block = graph.blocks.front();
		RSXFragmentProgram prog{};
		FP::RegisterAnnotationPass annotation_pass{ prog };

		annotation_pass.run(graph);

		ASSERT_EQ(block.clobber_list.size(), 3);
		ASSERT_EQ(block.input_list.size(), 2);

		EXPECT_EQ(block.clobber_list[0].reg, H0);
		EXPECT_EQ(block.clobber_list[1].reg, R0);
		EXPECT_EQ(block.clobber_list[2].reg, R1);

		EXPECT_EQ(block.input_list[0].reg, R0);
		EXPECT_EQ(block.input_list[1].reg, R1);
	}

	TEST(TestFPIR, RegisterDependencyPass_Simple16)
	{
		// Instruction 2 clobers R0 which in turn clobbers H0.
		// Instruction 3 reads from H0 so a barrier16 is needed between them.
		auto graph = CFG_from_source(R"(
			ADD  R1, R0, R1;
			PK8U R0, R1;
			MOV  H2, H0;
		)");

		ASSERT_EQ(graph.blocks.size(), 1);
		ASSERT_EQ(graph.blocks.front().instructions.size(), 3);

		auto& block = graph.blocks.front();
		RSXFragmentProgram prog{};

		FP::RegisterAnnotationPass annotation_pass{ prog };
		FP::RegisterDependencyPass deps_pass{};

		annotation_pass.run(graph);
		deps_pass.run(graph);

		ASSERT_EQ(block.instructions.size(), 5);

		// H0.xy = unpackHalf2(r0.x);
		EXPECT_EQ(OPDEST{ .HEX = block.instructions[2].bytecode[0] }.opcode, RSX_FP_OPCODE_UP2);
		EXPECT_EQ(OPDEST{ .HEX = block.instructions[2].bytecode[0] }.fp16, 1);
		EXPECT_EQ(OPDEST{ .HEX = block.instructions[2].bytecode[0] }.mask_x, true);
		EXPECT_EQ(OPDEST{ .HEX = block.instructions[2].bytecode[0] }.mask_y, true);
		EXPECT_EQ(OPDEST{ .HEX = block.instructions[2].bytecode[0] }.mask_z, false);
		EXPECT_EQ(OPDEST{ .HEX = block.instructions[2].bytecode[0] }.mask_w, false);
		EXPECT_EQ(SRC0{ .HEX = block.instructions[2].bytecode[1] }.reg_type, RSX_FP_REGISTER_TYPE_TEMP);
		EXPECT_EQ(SRC0{ .HEX = block.instructions[2].bytecode[1] }.tmp_reg_index, 0);
		EXPECT_EQ(SRC0{ .HEX = block.instructions[2].bytecode[1] }.fp16, 0);
		EXPECT_EQ(SRC0{ .HEX = block.instructions[2].bytecode[1] }.swizzle_x, 0);

		// H0.zw = unpackHalf2(r0.y);
		EXPECT_EQ(OPDEST{ .HEX = block.instructions[3].bytecode[0] }.opcode, RSX_FP_OPCODE_UP2);
		EXPECT_EQ(OPDEST{ .HEX = block.instructions[3].bytecode[0] }.mask_x, false);
		EXPECT_EQ(OPDEST{ .HEX = block.instructions[3].bytecode[0] }.mask_y, false);
		EXPECT_EQ(OPDEST{ .HEX = block.instructions[3].bytecode[0] }.mask_z, true);
		EXPECT_EQ(OPDEST{ .HEX = block.instructions[3].bytecode[0] }.mask_w, true);
		EXPECT_EQ(SRC0{ .HEX = block.instructions[3].bytecode[1] }.reg_type, RSX_FP_REGISTER_TYPE_TEMP);
		EXPECT_EQ(SRC0{ .HEX = block.instructions[3].bytecode[1] }.tmp_reg_index, 0);
		EXPECT_EQ(SRC0{ .HEX = block.instructions[3].bytecode[1] }.fp16, 0);
		EXPECT_EQ(SRC0{ .HEX = block.instructions[3].bytecode[1] }.swizzle_x, 1);
	}

	TEST(TestFPIR, RegisterDependencyPass_Simple32)
	{
		// Instruction 2 clobers H1 which in turn clobbers R0.
		// Instruction 3 reads from R0 so a barrier32 is needed between them.
		auto graph = CFG_from_source(R"(
			ADD R1, R0, R1;
			MOV H1, R1
			MOV R2, R0;
		)");

		ASSERT_EQ(graph.blocks.size(), 1);
		ASSERT_EQ(graph.blocks.front().instructions.size(), 3);

		auto& block = graph.blocks.front();
		RSXFragmentProgram prog{};

		FP::RegisterAnnotationPass annotation_pass{ prog };
		FP::RegisterDependencyPass deps_pass{};

		annotation_pass.run(graph);
		deps_pass.run(graph);

		ASSERT_EQ(block.instructions.size(), 5);

		// R0.z = packHalf2(H1.xy);
		EXPECT_EQ(OPDEST{ .HEX = block.instructions[2].bytecode[0] }.opcode, RSX_FP_OPCODE_PK2);
		EXPECT_EQ(OPDEST{ .HEX = block.instructions[2].bytecode[0] }.fp16, 0);
		EXPECT_EQ(OPDEST{ .HEX = block.instructions[2].bytecode[0] }.dest_reg, 0);
		EXPECT_EQ(OPDEST{ .HEX = block.instructions[2].bytecode[0] }.mask_x, false);
		EXPECT_EQ(OPDEST{ .HEX = block.instructions[2].bytecode[0] }.mask_y, false);
		EXPECT_EQ(OPDEST{ .HEX = block.instructions[2].bytecode[0] }.mask_z, true);
		EXPECT_EQ(OPDEST{ .HEX = block.instructions[2].bytecode[0] }.mask_w, false);
		EXPECT_EQ(SRC0{ .HEX = block.instructions[2].bytecode[1] }.reg_type, RSX_FP_REGISTER_TYPE_TEMP);
		EXPECT_EQ(SRC0{ .HEX = block.instructions[2].bytecode[1] }.tmp_reg_index, 1);
		EXPECT_EQ(SRC0{ .HEX = block.instructions[2].bytecode[1] }.fp16, 1);
		EXPECT_EQ(SRC0{ .HEX = block.instructions[2].bytecode[1] }.swizzle_x, 0);
		EXPECT_EQ(SRC0{ .HEX = block.instructions[2].bytecode[1] }.swizzle_y, 1);

		// R0.w = packHalf2(H1.zw);
		EXPECT_EQ(OPDEST{ .HEX = block.instructions[3].bytecode[0] }.opcode, RSX_FP_OPCODE_PK2);
		EXPECT_EQ(OPDEST{ .HEX = block.instructions[3].bytecode[0] }.fp16, 0);
		EXPECT_EQ(OPDEST{ .HEX = block.instructions[3].bytecode[0] }.dest_reg, 0);
		EXPECT_EQ(OPDEST{ .HEX = block.instructions[3].bytecode[0] }.mask_x, false);
		EXPECT_EQ(OPDEST{ .HEX = block.instructions[3].bytecode[0] }.mask_y, false);
		EXPECT_EQ(OPDEST{ .HEX = block.instructions[3].bytecode[0] }.mask_z, false);
		EXPECT_EQ(OPDEST{ .HEX = block.instructions[3].bytecode[0] }.mask_w, true);
		EXPECT_EQ(SRC0{ .HEX = block.instructions[3].bytecode[1] }.reg_type, RSX_FP_REGISTER_TYPE_TEMP);
		EXPECT_EQ(SRC0{ .HEX = block.instructions[3].bytecode[1] }.tmp_reg_index, 1);
		EXPECT_EQ(SRC0{ .HEX = block.instructions[3].bytecode[1] }.fp16, 1);
		EXPECT_EQ(SRC0{ .HEX = block.instructions[3].bytecode[1] }.swizzle_x, 2);
		EXPECT_EQ(SRC0{ .HEX = block.instructions[3].bytecode[1] }.swizzle_y, 3);
	}

	TEST(TestFPIR, RegisterDependencyPass_Complex_IF_BothPredecessorsClobber)
	{
		// Multi-level but only single IF
		// Mockup of a simple lighting function, R0 = Light vector, R1 = Decompressed normal. DP4 used for simplicity.
		// Data hazards sprinkled in for testing. R3 is clobbered in the ancestor and the IF branch.
		// Barrier should go in the IF branch here.
		auto ir = FPIR::from_source(R"(
			DP4   R2, R0, R1
			SFL   R3
			SGT   R3, R2, R0
			IF.GE
				ADD R0, R0, R2
				MOV H6, #{ 0.25 }
			ENDIF
			ADD R0, R0, R3
			MOV R1, R0
		)");

		auto bytecode = ir.compile();

		RSXFragmentProgram prog{};
		prog.data = bytecode.data();

		auto graph = deconstruct_fragment_program(prog);
		auto bb0 = get_graph_block(graph, 0);
		auto bb1 = get_graph_block(graph, 1);
		auto bb2 = get_graph_block(graph, 2);

		FP::RegisterAnnotationPass annotation_pass{ prog };
		FP::RegisterDependencyPass deps_pass{};

		annotation_pass.run(graph);
		deps_pass.run(graph);

		ASSERT_EQ(bb0->instructions.size(), 4);
		ASSERT_EQ(bb1->instructions.size(), 2);
		ASSERT_EQ(bb2->instructions.size(), 2);

		// bb1 has a epilogue
		ASSERT_EQ(bb1->epilogue.size(), 2);

		// bb1 epilogue updates R3.xy

		// R3.x = packHalf2(H6.xy)
		EXPECT_EQ(OPDEST{ .HEX = bb1->epilogue[0].bytecode[0] }.opcode, RSX_FP_OPCODE_PK2);
		EXPECT_EQ(OPDEST{ .HEX = bb1->epilogue[0].bytecode[0] }.fp16, 0);
		EXPECT_EQ(OPDEST{ .HEX = bb1->epilogue[0].bytecode[0] }.dest_reg, 3);
		EXPECT_EQ(OPDEST{ .HEX = bb1->epilogue[0].bytecode[0] }.mask_x, true);
		EXPECT_EQ(OPDEST{ .HEX = bb1->epilogue[0].bytecode[0] }.mask_y, false);
		EXPECT_EQ(OPDEST{ .HEX = bb1->epilogue[0].bytecode[0] }.mask_z, false);
		EXPECT_EQ(OPDEST{ .HEX = bb1->epilogue[0].bytecode[0] }.mask_w, false);
		EXPECT_EQ(SRC0{ .HEX = bb1->epilogue[0].bytecode[1] }.reg_type, RSX_FP_REGISTER_TYPE_TEMP);
		EXPECT_EQ(SRC0{ .HEX = bb1->epilogue[0].bytecode[1] }.tmp_reg_index, 6);
		EXPECT_EQ(SRC0{ .HEX = bb1->epilogue[0].bytecode[1] }.fp16, 1);
		EXPECT_EQ(SRC0{ .HEX = bb1->epilogue[0].bytecode[1] }.swizzle_x, 0);
		EXPECT_EQ(SRC0{ .HEX = bb1->epilogue[0].bytecode[1] }.swizzle_y, 1);

		// R3.y = packHalf2(H6.zw)
		EXPECT_EQ(OPDEST{ .HEX = bb1->epilogue[1].bytecode[0] }.opcode, RSX_FP_OPCODE_PK2);
		EXPECT_EQ(OPDEST{ .HEX = bb1->epilogue[1].bytecode[0] }.fp16, 0);
		EXPECT_EQ(OPDEST{ .HEX = bb1->epilogue[1].bytecode[0] }.dest_reg, 3);
		EXPECT_EQ(OPDEST{ .HEX = bb1->epilogue[1].bytecode[0] }.mask_x, false);
		EXPECT_EQ(OPDEST{ .HEX = bb1->epilogue[1].bytecode[0] }.mask_y, true);
		EXPECT_EQ(OPDEST{ .HEX = bb1->epilogue[1].bytecode[0] }.mask_z, false);
		EXPECT_EQ(OPDEST{ .HEX = bb1->epilogue[1].bytecode[0] }.mask_w, false);
		EXPECT_EQ(SRC0{ .HEX = bb1->epilogue[1].bytecode[1] }.reg_type, RSX_FP_REGISTER_TYPE_TEMP);
		EXPECT_EQ(SRC0{ .HEX = bb1->epilogue[1].bytecode[1] }.tmp_reg_index, 6);
		EXPECT_EQ(SRC0{ .HEX = bb1->epilogue[1].bytecode[1] }.fp16, 1);
		EXPECT_EQ(SRC0{ .HEX = bb1->epilogue[1].bytecode[1] }.swizzle_x, 2);
		EXPECT_EQ(SRC0{ .HEX = bb1->epilogue[1].bytecode[1] }.swizzle_y, 3);
	}

	TEST(TestFPIR, RegisterDependencyPass_Complex_IF_ELSE_OneBranchClobbers)
	{
		// Single IF-ELSE, if clobbers, ELSE does not
		auto ir = FPIR::from_source(R"(
			DP4 R2, R0, R1
			SFL R3
			SGT R3, R2, R0
			IF.GE
				ADD R0, R0, R2
				MOV H6, #{ 0.25 }
			ELSE
				ADD R0, R0, R1
			ENDIF
			ADD R0, R0, R3
			MOV R1, R0
		)");

		auto bytecode = ir.compile();

		RSXFragmentProgram prog{};
		prog.data = bytecode.data();
		auto graph = deconstruct_fragment_program(prog);

		ASSERT_EQ(graph.blocks.size(), 4);

		FP::RegisterAnnotationPass annotation_pass{ prog };
		FP::RegisterDependencyPass deps_pass{};

		annotation_pass.run(graph);
		deps_pass.run(graph);

		const BasicBlock
			*bb0 = get_graph_block(graph, 0),
			*bb1 = get_graph_block(graph, 1),
			*bb2 = get_graph_block(graph, 2),
			*bb3 = get_graph_block(graph, 3);

		ASSERT_EQ(bb0->instructions.size(), 4);
		ASSERT_EQ(bb1->instructions.size(), 2);
		ASSERT_EQ(bb2->instructions.size(), 1);
		ASSERT_EQ(bb3->instructions.size(), 2);

		// bb1 has a epilogue
		ASSERT_EQ(bb0->epilogue.size(), 0);
		ASSERT_EQ(bb1->epilogue.size(), 2);
		ASSERT_EQ(bb2->epilogue.size(), 0);

		// bb1 epilogue updates R3.xy

		// R3.x = packHalf2(H6.xy)
		EXPECT_EQ(OPDEST{ .HEX = bb1->epilogue[0].bytecode[0] }.opcode, RSX_FP_OPCODE_PK2);
		EXPECT_EQ(OPDEST{ .HEX = bb1->epilogue[0].bytecode[0] }.fp16, 0);
		EXPECT_EQ(OPDEST{ .HEX = bb1->epilogue[0].bytecode[0] }.dest_reg, 3);
		EXPECT_EQ(OPDEST{ .HEX = bb1->epilogue[0].bytecode[0] }.mask_x, true);
		EXPECT_EQ(OPDEST{ .HEX = bb1->epilogue[0].bytecode[0] }.mask_y, false);
		EXPECT_EQ(OPDEST{ .HEX = bb1->epilogue[0].bytecode[0] }.mask_z, false);
		EXPECT_EQ(OPDEST{ .HEX = bb1->epilogue[0].bytecode[0] }.mask_w, false);
		EXPECT_EQ(SRC0{ .HEX = bb1->epilogue[0].bytecode[1] }.reg_type, RSX_FP_REGISTER_TYPE_TEMP);
		EXPECT_EQ(SRC0{ .HEX = bb1->epilogue[0].bytecode[1] }.tmp_reg_index, 6);
		EXPECT_EQ(SRC0{ .HEX = bb1->epilogue[0].bytecode[1] }.fp16, 1);
		EXPECT_EQ(SRC0{ .HEX = bb1->epilogue[0].bytecode[1] }.swizzle_x, 0);
		EXPECT_EQ(SRC0{ .HEX = bb1->epilogue[0].bytecode[1] }.swizzle_y, 1);

		// R3.y = packHalf2(H6.zw)
		EXPECT_EQ(OPDEST{ .HEX = bb1->epilogue[1].bytecode[0] }.opcode, RSX_FP_OPCODE_PK2);
		EXPECT_EQ(OPDEST{ .HEX = bb1->epilogue[1].bytecode[0] }.fp16, 0);
		EXPECT_EQ(OPDEST{ .HEX = bb1->epilogue[1].bytecode[0] }.dest_reg, 3);
		EXPECT_EQ(OPDEST{ .HEX = bb1->epilogue[1].bytecode[0] }.mask_x, false);
		EXPECT_EQ(OPDEST{ .HEX = bb1->epilogue[1].bytecode[0] }.mask_y, true);
		EXPECT_EQ(OPDEST{ .HEX = bb1->epilogue[1].bytecode[0] }.mask_z, false);
		EXPECT_EQ(OPDEST{ .HEX = bb1->epilogue[1].bytecode[0] }.mask_w, false);
		EXPECT_EQ(SRC0{ .HEX = bb1->epilogue[1].bytecode[1] }.reg_type, RSX_FP_REGISTER_TYPE_TEMP);
		EXPECT_EQ(SRC0{ .HEX = bb1->epilogue[1].bytecode[1] }.tmp_reg_index, 6);
		EXPECT_EQ(SRC0{ .HEX = bb1->epilogue[1].bytecode[1] }.fp16, 1);
		EXPECT_EQ(SRC0{ .HEX = bb1->epilogue[1].bytecode[1] }.swizzle_x, 2);
		EXPECT_EQ(SRC0{ .HEX = bb1->epilogue[1].bytecode[1] }.swizzle_y, 3);
	}


	TEST(TestFPIR, RegisterDependencyPass_Complex_IF_ELSE_Simpsons)
	{
		// Complex IF-ELSE nest observed in Simpson's game. Rewritten for simplicity.
		// There is no tail block. No epilogues should be injected in this scenario since H4 (the trigger) is defined on all branches.
		// R2 is indeed clobbered but the outer ELSE branch should not be able to see the inner IF-ELSE blocks as predecessors.
		auto ir = FPIR::from_source(R"(
			MOV R2, #{ 0.25 };
			IF.GT;
				SLT R4, H2, #{ 0.125 };
				IF.GT;
					ADD H2, H0, H3;
					FMA H4, R2, H2, H3;
				ELSE;
					MOV H2, #{ 0.125 };
					ADD H0, H0, H2;
					FMA H4, R2, H2, H3;
				ENDIF;
			ELSE;
				FMA H4, R2, H2, H3;
				MOV H0, H4;
			ENDIF;
		)");

		auto bytecode = ir.compile();

		RSXFragmentProgram prog{};
		prog.data = bytecode.data();
		auto graph = deconstruct_fragment_program(prog);

		ASSERT_EQ(graph.blocks.size(), 6);

		FP::RegisterAnnotationPass annotation_pass{ prog };
		FP::RegisterDependencyPass deps_pass{};

		annotation_pass.run(graph);
		deps_pass.run(graph);

		const BasicBlock
			*bb0 = get_graph_block(graph, 0),
			*bb1 = get_graph_block(graph, 1),
			*bb2 = get_graph_block(graph, 2),
			*bb3 = get_graph_block(graph, 3),
			*bb4 = get_graph_block(graph, 4),
			*bb5 = get_graph_block(graph, 5);

		// Sanity
		EXPECT_EQ(bb0->instructions.size(), 2);
		EXPECT_EQ(bb1->instructions.size(), 2);
		EXPECT_EQ(bb2->instructions.size(), 2);
		EXPECT_EQ(bb3->instructions.size(), 3);
		EXPECT_EQ(bb4->instructions.size(), 2);
		EXPECT_EQ(bb5->instructions.size(), 0);  // Phi/Merge only.

		// Nested children must recursively fall out to the closest ENDIF
		ASSERT_EQ(bb4->pred.size(), 1);
		EXPECT_EQ(bb4->pred.front().type, EdgeType::ELSE);
		EXPECT_EQ(bb5->pred.size(), 4); // 2 IF and 2 ELSE paths exist

		// Check that we get no epilogues
		EXPECT_EQ(bb0->epilogue.size(), 0);
		EXPECT_EQ(bb1->epilogue.size(), 0);
		EXPECT_EQ(bb2->epilogue.size(), 0);
		EXPECT_EQ(bb3->epilogue.size(), 0);
		EXPECT_EQ(bb4->epilogue.size(), 0);
		EXPECT_EQ(bb5->epilogue.size(), 0);
	}

	TEST(TestFPIR, RegisterDependencyPass_Partial32_0)
	{
		// Instruction 2 partially clobers H1 which in turn clobbers R0.
		// Instruction 3 reads from R0 so a partial barrier32 is needed between them.
		auto graph = CFG_from_source(R"(
			ADD R1,   R0, R1;
			MOV H1.x, R1.x;
			MOV R2,   R0;
		)");

		ASSERT_EQ(graph.blocks.size(), 1);
		ASSERT_EQ(graph.blocks.front().instructions.size(), 3);

		auto& block = graph.blocks.front();
		RSXFragmentProgram prog{};

		FP::RegisterAnnotationPass annotation_pass{ prog };
		FP::RegisterDependencyPass deps_pass{};

		annotation_pass.run(graph);
		deps_pass.run(graph);

		ASSERT_EQ(block.instructions.size(), 4);

		OPDEST dst{ .HEX = block.instructions[2].bytecode[0] };
		SRC0 src0{ .HEX = block.instructions[2].bytecode[1] };
		SRC1 src1{ .HEX = block.instructions[2].bytecode[2] };

		const u32 opcode = dst.opcode | (src1.opcode_hi << 6);

		// R0.z = packHalf2(H1.xy);
		EXPECT_EQ(opcode, RSX_FP_OPCODE_OR16_LO);
		EXPECT_EQ(dst.fp16, 0);
		EXPECT_EQ(dst.dest_reg, 0);
		EXPECT_EQ(dst.mask_x, false);
		EXPECT_EQ(dst.mask_y, false);
		EXPECT_EQ(dst.mask_z, true);
		EXPECT_EQ(dst.mask_w, false);
		EXPECT_EQ(src0.reg_type, RSX_FP_REGISTER_TYPE_TEMP);
		EXPECT_EQ(src0.tmp_reg_index, 0);
		EXPECT_EQ(src0.fp16, 0);
		EXPECT_EQ(src0.swizzle_x, 2);
		EXPECT_EQ(src1.reg_type, RSX_FP_REGISTER_TYPE_TEMP);
		EXPECT_EQ(src1.tmp_reg_index, 1);
		EXPECT_EQ(src1.fp16, 1);
		EXPECT_EQ(src1.swizzle_x, 0);
	}

	TEST(TestFPIR, RegisterDependencyPass_Partial32_1)
	{
		// Instruction 2 partially clobers H1 which in turn clobbers R0.
		// Instruction 3 reads from R0 so a partial barrier32 is needed between them.
		auto graph = CFG_from_source(R"(
			ADD R1,   R0, R1;
			MOV H1.y, R1.y;
			MOV R2,   R0;
		)");

		ASSERT_EQ(graph.blocks.size(), 1);
		ASSERT_EQ(graph.blocks.front().instructions.size(), 3);

		auto& block = graph.blocks.front();
		RSXFragmentProgram prog{};

		FP::RegisterAnnotationPass annotation_pass{ prog };
		FP::RegisterDependencyPass deps_pass{};

		annotation_pass.run(graph);
		deps_pass.run(graph);

		ASSERT_EQ(block.instructions.size(), 4);

		OPDEST dst{ .HEX = block.instructions[2].bytecode[0] };
		SRC0 src0{ .HEX = block.instructions[2].bytecode[1] };
		SRC1 src1{ .HEX = block.instructions[2].bytecode[2] };

		const u32 opcode = dst.opcode | (src1.opcode_hi << 6);

		// R0.z = packHalf2(H1.xy);
		EXPECT_EQ(opcode, RSX_FP_OPCODE_OR16_HI);
		EXPECT_EQ(dst.fp16, 0);
		EXPECT_EQ(dst.dest_reg, 0);
		EXPECT_EQ(dst.mask_x, false);
		EXPECT_EQ(dst.mask_y, false);
		EXPECT_EQ(dst.mask_z, true);
		EXPECT_EQ(dst.mask_w, false);
		EXPECT_EQ(src0.reg_type, RSX_FP_REGISTER_TYPE_TEMP);
		EXPECT_EQ(src0.tmp_reg_index, 0);
		EXPECT_EQ(src0.fp16, 0);
		EXPECT_EQ(src0.swizzle_x, 2);
		EXPECT_EQ(src1.reg_type, RSX_FP_REGISTER_TYPE_TEMP);
		EXPECT_EQ(src1.tmp_reg_index, 1);
		EXPECT_EQ(src1.fp16, 1);
		EXPECT_EQ(src1.swizzle_x, 1);
	}

	TEST(TestFPIR, RegisterDependencyPass_SkipDelaySlots)
	{
		// Instruction 2 clobers H1 which in turn clobbers R0.
		// Instruction 3 reads from R0 but is a delay slot that does nothing and can be NOPed.
		auto graph = CFG_from_source(R"(
			ADD R1, R0, R1;
			MOV H1, R1
			MOV R0, R0;
		)");

		ASSERT_EQ(graph.blocks.size(), 1);
		ASSERT_EQ(graph.blocks.front().instructions.size(), 3);

		auto& block = graph.blocks.front();
		RSXFragmentProgram prog{};

		FP::RegisterAnnotationPass annotation_pass{ prog, { .skip_delay_slots = true } };
		FP::RegisterDependencyPass deps_pass{};

		annotation_pass.run(graph);
		deps_pass.run(graph);

		// Delay slot detection will cause no dependency injection
		ASSERT_EQ(block.instructions.size(), 3);
	}

	TEST(TestFPIR, RegisterDependencyPass_Skip_IF_ELSE_Ancestors)
	{
		// R4/H8 is clobbered but an IF-ELSE chain follows it.
		// Merge block reads H8, but since both IF-ELSE legs resolve the dependency, we do not need a barrier for H8.
		// H6 is included as a control.
		auto ir = FPIR::from_source(R"(
			MOV R4, #{ 0.25 }
			MOV H6.x, #{ 0.125 }
			IF.LT
				MOV H8, #{ 0.0 }
			ELSE
				MOV H8, #{ 0.25 }
			ENDIF
			ADD R0, R3, H8
		)");

		auto bytecode = ir.compile();
		RSXFragmentProgram prog{};
		prog.data = bytecode.data();
		auto graph = deconstruct_fragment_program(prog);

		// Verify state before
		ASSERT_EQ(graph.blocks.size(), 4);
		EXPECT_EQ(get_graph_block(graph, 0)->instructions.size(), 3);
		EXPECT_EQ(get_graph_block(graph, 1)->instructions.size(), 1);
		EXPECT_EQ(get_graph_block(graph, 2)->instructions.size(), 1);
		EXPECT_EQ(get_graph_block(graph, 3)->instructions.size(), 1);

		FP::RegisterAnnotationPass annotation_pass{ prog, {.skip_delay_slots = true } };
		FP::RegisterDependencyPass deps_pass{};

		annotation_pass.run(graph);
		deps_pass.run(graph);

		// We get one barrier on R3 (H6) but nont for R4 (H8)
		EXPECT_EQ(get_graph_block(graph, 0)->epilogue.size(), 1);

		// No intra-block barriers
		EXPECT_EQ(get_graph_block(graph, 0)->instructions.size(), 3);
		EXPECT_EQ(get_graph_block(graph, 1)->instructions.size(), 1);
		EXPECT_EQ(get_graph_block(graph, 2)->instructions.size(), 1);
		EXPECT_EQ(get_graph_block(graph, 3)->instructions.size(), 1);
	}

	TEST(TestFPIR, RegisterDependencyPass_Process_IF_Ancestors)
	{
		// H8.x is clobbered but only an IF sequence follows with no ELSE.
		// Merge block reads r4.x, but since both IF-ELSE legs resolve the dependency, we do not need a barrier.
		auto ir = FPIR::from_source(R"(
			MOV H8.x, #{ 0.25 }
			IF.LT
				MOV R4.x, #{ 0.0 }
			ENDIF
			MOV R0, R4
		)");

		auto bytecode = ir.compile();
		RSXFragmentProgram prog{};
		prog.data = bytecode.data();
		auto graph = deconstruct_fragment_program(prog);

		// Verify state before
		ASSERT_EQ(graph.blocks.size(), 3);
		EXPECT_EQ(get_graph_block(graph, 0)->instructions.size(), 2);
		EXPECT_EQ(get_graph_block(graph, 1)->instructions.size(), 1);
		EXPECT_EQ(get_graph_block(graph, 2)->instructions.size(), 1);

		FP::RegisterAnnotationPass annotation_pass{ prog, {.skip_delay_slots = true } };
		FP::RegisterDependencyPass deps_pass{};

		annotation_pass.run(graph);
		deps_pass.run(graph);

		// A barrier will be inserted into block 0 epilogue
		EXPECT_EQ(get_graph_block(graph, 0)->instructions.size(), 2);
		EXPECT_EQ(get_graph_block(graph, 1)->instructions.size(), 1);
		EXPECT_EQ(get_graph_block(graph, 2)->instructions.size(), 1);

		EXPECT_EQ(get_graph_block(graph, 0)->epilogue.size(), 1);
		EXPECT_EQ(get_graph_block(graph, 1)->epilogue.size(), 0);
		EXPECT_EQ(get_graph_block(graph, 2)->epilogue.size(), 0);
	}

	TEST(TestFPIR, RegisterDependencyPass_Complex_IF_ELSE_Ancestor_Clobber)
	{
		// 2 clobbered registers up the chain.
		// 1 full barrier is needed for R4 (4 instructions)
		auto ir = FPIR::from_source(R"(
			MOV R4, #{ 0.0 }
			IF.LT
				MOV H9, #{ 0.25 }
			ENDIF
			MOV H8, #{ 0.25 }
			IF.LT
				IF.GT
					ADD R0, R0, R0
				ELSE
					ADD R0, R1, R0
				ENDIF
			ENDIF
			ADD R0, R0, R4
		)");

		auto bytecode = ir.compile();
		RSXFragmentProgram prog{};
		prog.data = bytecode.data();
		auto graph = deconstruct_fragment_program(prog);

		// Verify state before
		ASSERT_EQ(graph.blocks.size(), 7);
		EXPECT_EQ(get_graph_block(graph, 0)->instructions.size(), 2);
		EXPECT_EQ(get_graph_block(graph, 1)->instructions.size(), 1);
		EXPECT_EQ(get_graph_block(graph, 2)->instructions.size(), 2);
		EXPECT_EQ(get_graph_block(graph, 3)->instructions.size(), 1);
		EXPECT_EQ(get_graph_block(graph, 4)->instructions.size(), 1);
		EXPECT_EQ(get_graph_block(graph, 5)->instructions.size(), 1);
		EXPECT_EQ(get_graph_block(graph, 6)->instructions.size(), 1);

		FP::RegisterAnnotationPass annotation_pass{ prog, {.skip_delay_slots = true } };
		FP::RegisterDependencyPass deps_pass{};

		annotation_pass.run(graph);
		deps_pass.run(graph);

		// Full-lane barrier on writing blocks
		EXPECT_EQ(get_graph_block(graph, 1)->epilogue.size(), 2);
		EXPECT_EQ(get_graph_block(graph, 2)->epilogue.size(), 2);

		EXPECT_EQ(get_graph_block(graph, 0)->instructions.size(), 2);
		EXPECT_EQ(get_graph_block(graph, 1)->instructions.size(), 1);
		EXPECT_EQ(get_graph_block(graph, 2)->instructions.size(), 2);
		EXPECT_EQ(get_graph_block(graph, 3)->instructions.size(), 1);
		EXPECT_EQ(get_graph_block(graph, 4)->instructions.size(), 1);
		EXPECT_EQ(get_graph_block(graph, 5)->instructions.size(), 1);
		EXPECT_EQ(get_graph_block(graph, 6)->instructions.size(), 1);
	}

	TEST(TestFPIR, RegisterDependencyPass_SplinterCell_DelaySlot)
	{
		// Real shader pattern found in splinter cell blacklist.
		// TEX instructions replaced with MOV for simplicity.
		// There are no dependent reads here, no barriers are expected.
		// In the game, instruction 4 was misclassified as a delay slot, causing a skipped clobber.
		auto ir = FPIR::from_source(R"(
			MOV R0.w, #{ 0.25 }
			MOV H0, H8
			MUL R0.w, H0.w, R0.w
			MOV R0.xyz, H0.xyz
			MOV R1, #{ 0.25 }
			FMA H0, R0, #{ 0.125 }, R1
		)");

		auto bytecode = ir.compile();
		RSXFragmentProgram prog{};
		prog.data = bytecode.data();
		auto graph = deconstruct_fragment_program(prog);

		// Verify state before
		ASSERT_EQ(graph.blocks.size(), 1);
		EXPECT_EQ(get_graph_block(graph, 0)->instructions.size(), 6);

		FP::RegisterAnnotationPass annotation_pass{ prog, {.skip_delay_slots = true } };
		FP::RegisterDependencyPass deps_pass{};

		annotation_pass.run(graph);
		deps_pass.run(graph);

		// Verify state after
		EXPECT_EQ(get_graph_block(graph, 0)->instructions.size(), 6);
		EXPECT_EQ(get_graph_block(graph, 0)->epilogue.size(), 0);
	}
}
