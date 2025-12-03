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

	static FlowGraph CFG_from_source(const std::string& asm_)
	{
		auto ir = FPIR::from_source(asm_);

		FlowGraph graph{};
		graph.blocks.push_back({});

		auto& bb = graph.blocks.back();
		bb.instructions = ir.build();
		return graph;
	}

	static BasicBlock* BB_from_source(FlowGraph* graph, const std::string& asm_)
	{
		auto ir = FPIR::from_source(asm_);
		graph->blocks.push_back({});
		BasicBlock& bb = graph->blocks.back();
		bb.instructions = ir.build();
		return &bb;
	}
	TEST(TestFPIR, FromSource)
	{
		auto ir = FPIR::from_source(R"(
			MOV R0, #{ 0.125 };
			ADD R1, R0, R0;
		)");

		const auto instructions = ir.build();

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
		FlowGraph graph;
		BasicBlock* bb0 = BB_from_source(&graph, R"(
			DP4   R2, R0, R1
			SFL   R3
			SGT   R3, R2, R0
			IF.GE
		)");

		BasicBlock* bb1 = BB_from_source(&graph, R"(
			ADD R0, R0, R2
			MOV H6, #{ 0.25 }
		)");

		BasicBlock* bb2 = BB_from_source(&graph, R"(
			ADD R0, R0, R3
			MOV R1, R0
		)");

		// Front edges
		bb0->insert_succ(bb1, EdgeType::IF);
		bb0->insert_succ(bb2, EdgeType::ENDIF);
		bb1->insert_succ(bb2, EdgeType::ENDIF);

		// Back edges
		bb2->insert_pred(bb1, EdgeType::ENDIF);
		bb2->insert_pred(bb0, EdgeType::ENDIF);
		bb1->insert_pred(bb0, EdgeType::IF);

		RSXFragmentProgram prog{};

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
}
