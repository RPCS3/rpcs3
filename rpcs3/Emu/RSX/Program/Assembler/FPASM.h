#pragma once

#include "IR.h"

namespace rsx::assembler
{
	class FPIR
	{
	public:
		void mov(const RegisterRef& dst, f32 constant);
		void mov(const RegisterRef& dst, const RegisterRef& src);

		void add(const RegisterRef& dst, const std::array<f32, 4>& constants);
		void add(const RegisterRef& dst, const RegisterRef& src);

		const std::vector<Instruction>& instructions() const;
		std::vector<u32> compile() const;

		static FPIR from_source(std::string_view asm_);

	private:
		Instruction* load(const RegisterRef& reg, int operand, Instruction* target = nullptr);
		Instruction* load(const std::array<f32, 4>& constants, int operand, Instruction* target = nullptr);
		Instruction* store(const RegisterRef& reg, Instruction* target = nullptr);

		std::vector<Instruction> m_instructions;
	};
}

