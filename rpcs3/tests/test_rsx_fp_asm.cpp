#include <gtest/gtest.h>

#include "Emu/RSX/Common/simple_array.hpp"
#include "Emu/RSX/Program/Assembler/FPASM.h"
#include "Emu/RSX/Program/Assembler/Passes/FP/RegisterAnnotationPass.h"
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
		// Code snippet reads from R0 and R2, clobbers R0, R1, H0
		auto graph = CFG_from_source(R"(
			ADD R1, R0, R1;
			MOV H0, H4;
		)");

		ASSERT_EQ(graph.blocks.size(), 1);
		ASSERT_EQ(graph.blocks.front().instructions.size(), 2);

		auto& block = graph.blocks.front();
		RSXFragmentProgram prog{};
		FP::RegisterAnnotationPass annotation_pass(prog);

		annotation_pass.run(graph);

		ASSERT_EQ(block.clobber_list.size(), 2);
		ASSERT_EQ(block.input_list.size(), 3);

		EXPECT_EQ(block.clobber_list[0].reg, H0);
		EXPECT_EQ(block.clobber_list[1].reg, R1);

		EXPECT_EQ(block.input_list[0].reg, H4);
		EXPECT_EQ(block.input_list[1].reg, R0);
		EXPECT_EQ(block.input_list[2].reg, R1);
	}
}
