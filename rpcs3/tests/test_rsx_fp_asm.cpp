#include <gtest/gtest.h>

#include "Emu/RSX/Common/simple_array.hpp"
#include "Emu/RSX/Program/Assembler/FPASM.h"
#include "Emu/RSX/Program/RSXFragmentProgram.h"

namespace rsx::assembler
{
	TEST(TestFPIR, FromSource)
	{
		auto ir = FPIR::from_source(R"(
			MOV R0, #{ 0.125 };
			ADD R1, R0;
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
}
