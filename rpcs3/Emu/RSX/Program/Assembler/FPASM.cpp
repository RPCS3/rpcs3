#include "stdafx.h"
#include "FPASM.h"
#include "Emu/RSX/Program/RSXFragmentProgram.h"

#include <stack>

#ifndef _WIN32
#define sscanf_s sscanf
#endif

namespace rsx::assembler
{
	struct FP_opcode_encoding_t
	{
		FP_opcode op;
		bool exec_if_lt;
		bool exec_if_eq;
		bool exec_if_gt;
		bool set_cond;
	};

	static std::unordered_map<std::string_view, FP_opcode_encoding_t> s_opcode_lookup
	{
		// Arithmetic
		{ "NOP", { .op = RSX_FP_OPCODE_NOP, .exec_if_lt = true, .exec_if_eq = true, .exec_if_gt = true, .set_cond = false } },
		{ "MOV", { .op = RSX_FP_OPCODE_MOV, .exec_if_lt = true, .exec_if_eq = true, .exec_if_gt = true, .set_cond = false } },
		{ "MUL", { .op = RSX_FP_OPCODE_MUL, .exec_if_lt = true, .exec_if_eq = true, .exec_if_gt = true, .set_cond = false } },
		{ "ADD", { .op = RSX_FP_OPCODE_ADD, .exec_if_lt = true, .exec_if_eq = true, .exec_if_gt = true, .set_cond = false } },
		{ "MAD", { .op = RSX_FP_OPCODE_MAD, .exec_if_lt = true, .exec_if_eq = true, .exec_if_gt = true, .set_cond = false } },
		{ "FMA", { .op = RSX_FP_OPCODE_MAD, .exec_if_lt = true, .exec_if_eq = true, .exec_if_gt = true, .set_cond = false } },
		{ "DP3", { .op = RSX_FP_OPCODE_DP3, .exec_if_lt = true, .exec_if_eq = true, .exec_if_gt = true, .set_cond = false } },
		{ "DP4", { .op = RSX_FP_OPCODE_DP4, .exec_if_lt = true, .exec_if_eq = true, .exec_if_gt = true, .set_cond = false } },

		// Constant load
		{ "SFL", {.op = RSX_FP_OPCODE_SFL, .exec_if_lt = true, .exec_if_eq = true, .exec_if_gt = true, .set_cond = false } },
		{ "STR", {.op = RSX_FP_OPCODE_STR, .exec_if_lt = true, .exec_if_eq = true, .exec_if_gt = true, .set_cond = false } },

		// Pack-unpack operations are great for testing dependencies
		{ "PKH", { .op = RSX_FP_OPCODE_PK2, .exec_if_lt = true, .exec_if_eq = true, .exec_if_gt = true, .set_cond = false } },
		{ "UPH", { .op = RSX_FP_OPCODE_UP2, .exec_if_lt = true, .exec_if_eq = true, .exec_if_gt = true, .set_cond = false } },
		{ "PK16U", { .op = RSX_FP_OPCODE_PK16, .exec_if_lt = true, .exec_if_eq = true, .exec_if_gt = true, .set_cond = false } },
		{ "UP16U", { .op = RSX_FP_OPCODE_UP16, .exec_if_lt = true, .exec_if_eq = true, .exec_if_gt = true, .set_cond = false } },
		{ "PK8U", { .op = RSX_FP_OPCODE_PKB, .exec_if_lt = true, .exec_if_eq = true, .exec_if_gt = true, .set_cond = false } },
		{ "UP8U", { .op = RSX_FP_OPCODE_UPB, .exec_if_lt = true, .exec_if_eq = true, .exec_if_gt = true, .set_cond = false } },
		{ "PK8G", { .op = RSX_FP_OPCODE_PKG, .exec_if_lt = true, .exec_if_eq = true, .exec_if_gt = true, .set_cond = false } },
		{ "UP8G", { .op = RSX_FP_OPCODE_UPG, .exec_if_lt = true, .exec_if_eq = true, .exec_if_gt = true, .set_cond = false } },
		{ "PK8S", { .op = RSX_FP_OPCODE_PK4, .exec_if_lt = true, .exec_if_eq = true, .exec_if_gt = true, .set_cond = false } },
		{ "UP8S", { .op = RSX_FP_OPCODE_UP4, .exec_if_lt = true, .exec_if_eq = true, .exec_if_gt = true, .set_cond = false } },

		// Basic conditionals
		{ "IF.LT", { .op = RSX_FP_OPCODE_IFE, .exec_if_lt = true,  .exec_if_eq = false, .exec_if_gt = false, .set_cond = false } },
		{ "IF.LE", { .op = RSX_FP_OPCODE_IFE, .exec_if_lt = true,  .exec_if_eq = true,  .exec_if_gt = false, .set_cond = false } },
		{ "IF.EQ", { .op = RSX_FP_OPCODE_IFE, .exec_if_lt = false, .exec_if_eq = true,  .exec_if_gt = false, .set_cond = false } },
		{ "IF.GE", { .op = RSX_FP_OPCODE_IFE, .exec_if_lt = false, .exec_if_eq = true,  .exec_if_gt = true, .set_cond = false } },
		{ "IF.GT", { .op = RSX_FP_OPCODE_IFE, .exec_if_lt = false, .exec_if_eq = false, .exec_if_gt = true, .set_cond = false } },

		{ "SLT", { .op = RSX_FP_OPCODE_SLT, .exec_if_lt = false, .exec_if_eq = false, .exec_if_gt = false, .set_cond = true } },
		{ "SEQ", { .op = RSX_FP_OPCODE_SEQ, .exec_if_lt = false, .exec_if_eq = false, .exec_if_gt = false, .set_cond = true } },
		{ "SGT", { .op = RSX_FP_OPCODE_SGT, .exec_if_lt = false, .exec_if_eq = false, .exec_if_gt = false, .set_cond = true } },

		// TODO: Add more

	};

	Instruction* FPIR::load(const RegisterRef& ref, int operand, Instruction* prev)
	{
		Instruction* target = prev;
		if (!target)
		{
			m_instructions.push_back({});
			target = &m_instructions.back();
		}

		SRC_Common src{ .HEX = target->bytecode[operand + 1] };
		src.reg_type = RSX_FP_REGISTER_TYPE_TEMP;
		src.fp16 = ref.reg.f16 ? 1 : 0;
		src.tmp_reg_index = static_cast<u32>(ref.reg.id);

		src.swizzle_x = 0;
		src.swizzle_y = 1;
		src.swizzle_z = 2;
		src.swizzle_w = 3;

		target->bytecode[operand + 1] = src.HEX;
		return target;
	}

	Instruction* FPIR::load(const std::array<f32, 4>& constants, int operand, Instruction* prev)
	{
		Instruction* target = prev;
		if (!target)
		{
			m_instructions.push_back({});
			target = &m_instructions.back();
		}

		// Unsupported for now
		ensure(target->length == 4, "FPIR cannot encode more than one constant load per instruction");

		SRC_Common src{ .HEX = target->bytecode[operand + 1] };
		src.reg_type = RSX_FP_REGISTER_TYPE_CONSTANT;
		target->bytecode[operand + 1] = src.HEX;

		src.swizzle_x = 0;
		src.swizzle_y = 1;
		src.swizzle_z = 2;
		src.swizzle_w = 3;

		// Embed literal constant
		std::memcpy(&target->bytecode[4], constants.data(), 4 * sizeof(u32));
		target->length = 8;
		return target;
	}

	Instruction* FPIR::store(const RegisterRef& ref, Instruction* prev)
	{
		Instruction* target = prev;
		if (!target)
		{
			m_instructions.push_back({});
			target = &m_instructions.back();
		}

		OPDEST dst{ .HEX = target->bytecode[0] };
		dst.dest_reg = static_cast<u32>(ref.reg.id);
		dst.fp16 = ref.reg.f16 ? 1 : 0;
		dst.write_mask = ref.mask;
		dst.prec = ref.reg.f16 ? RSX_FP_PRECISION_HALF : RSX_FP_PRECISION_REAL;

		target->bytecode[0] = dst.HEX;
		return target;
	}

	void FPIR::mov(const RegisterRef& dst, f32 constant)
	{
		Instruction* inst = store(dst);
		inst = load(std::array<f32, 4>{ constant, constant, constant, constant }, 0);
		inst->opcode = RSX_FP_OPCODE_MOV;
	}

	void FPIR::mov(const RegisterRef& dst, const RegisterRef& src)
	{
		Instruction* inst = store(dst);
		inst = load(src, 0);
		inst->opcode = RSX_FP_OPCODE_MOV;
	}

	void FPIR::add(const RegisterRef& dst, const std::array<f32, 4>& constants)
	{
		Instruction* inst = store(dst);
		inst = load(constants, 0);
		inst->opcode = RSX_FP_OPCODE_ADD;
	}

	void FPIR::add(const RegisterRef& dst, const RegisterRef& src)
	{
		Instruction* inst = store(dst);
		inst = load(src, 0);
		inst->opcode = RSX_FP_OPCODE_ADD;
	}

	const std::vector<Instruction>& FPIR::instructions() const
	{
		return m_instructions;
	}

	std::vector<u32> FPIR::compile() const
	{
		std::vector<u32> result;
		result.reserve(m_instructions.size() * 4);

		for (const auto& inst : m_instructions)
		{
			const auto src = reinterpret_cast<const be_t<u16>*>(inst.bytecode);
			for (u32 j = 0; j < inst.length; ++j)
			{
				const u16 low = src[j * 2];
				const u16 hi = src[j * 2 + 1];
				const u32 word = static_cast<u16>(low) | (static_cast<u32>(hi) << 16u);
				result.push_back(word);
			}
		}

		return result;
	}

	FPIR FPIR::from_source(std::string_view asm_)
	{
		std::vector<std::string> instructions = fmt::split(asm_, { "\n", ";" });
		if (instructions.empty())
		{
			return {};
		}

		auto transform_inst = [](std::string_view s)
		{
			std::string result;
			result.reserve(s.size());

			bool literal = false;
			for (const auto& c : s)
			{
				if (c == ' ')
				{
					if (!literal && !result.empty() && result.back() != ',')
					{
						result += ','; // Replace token separator space with comma
					}
					continue;
				}

				if (std::isspace(c))
				{
					continue;
				}

				if (!literal && c == '{')
				{
					literal = true;
				}

				if (literal && c == '}')
				{
					literal = false;
				}

				if (c == ',')
				{
					result += (literal ? '|' : ',');
					continue;
				}

				result += c;
			}
			return result;
		};

		auto decode_instruction = [&](std::string_view inst, std::string& op, std::string& dst, std::vector<std::string>& sources)
		{
			const auto i = transform_inst(inst);
			if (i.empty())
			{
				return;
			}

			const auto tokens = fmt::split(i, { "," });
			ensure(!tokens.empty(), "Invalid input");

			op = tokens.front();

			if (tokens.size() > 1)
			{
				dst = tokens[1];
			}

			for (size_t n = 2; n < tokens.size(); ++n)
			{
				sources.push_back(tokens[n]);
			}
		};

		auto get_ref = [](std::string_view reg)
		{
			ensure(reg.length() > 1, "Invalid register specifier");

			const auto parts = fmt::split(reg, { "." });
			ensure(parts.size() > 0 && parts.size() <= 2);

			const auto index = std::stoi(parts[0].substr(1));
			RegisterRef ref
			{
				.reg { .id = index, .f16 = false },
				.mask = 0x0F
			};

			if (parts.size() > 1 && parts[1].length() > 0)
			{
				// FIXME: No swizzles for now, just lane masking
				ref.mask = 0;
				if (parts[1].find("x") != std::string::npos) ref.mask |= (1u << 0);
				if (parts[1].find("y") != std::string::npos) ref.mask |= (1u << 1);
				if (parts[1].find("z") != std::string::npos) ref.mask |= (1u << 2);
				if (parts[1].find("w") != std::string::npos) ref.mask |= (1u << 3);
			}

			if (reg[0] == 'H' || reg[0] == 'h')
			{
				ref.reg.f16 = true;
			}

			return ref;
		};

		auto get_constants = [](std::string_view reg) -> std::array<f32, 4>
		{
			float x, y, z, w;
			if (sscanf_s(reg.data(), "#{%f|%f|%f|%f}", &x, &y, &z, &w) == 4)
			{
				return { x, y, z, w };
			}

			if (sscanf_s(reg.data(), "#{%f}", &x) == 1)
			{
				return { x, x, x, x };
			}

			fmt::throw_exception("Invalid constant literal");
		};

		auto encode_branch_else = [](Instruction* inst, u32 end)
		{
			SRC1 src1{ .HEX = inst->bytecode[2] };
			src1.else_offset = static_cast<u32>(end);
			inst->bytecode[2] = src1.HEX;
		};

		auto encode_branch_end = [](Instruction *inst, u32 end)
		{
			SRC2 src2 { .HEX = inst->bytecode[3] };
			src2.end_offset = static_cast<u32>(end);
			inst->bytecode[3] = src2.HEX;

			SRC1 src1{ .HEX = inst->bytecode[2] };
			if (!src1.else_offset)
			{
				src1.else_offset = static_cast<u32>(end);
				inst->bytecode[2] = src1.HEX;
			}
		};

		auto encode_opcode = [](std::string_view op, Instruction* inst)
		{
			OPDEST d0 { .HEX = inst->bytecode[0] };
			SRC0 s0 { .HEX = inst->bytecode[1] };
			SRC1 s1 { .HEX = inst->bytecode[2] };

			const auto found = s_opcode_lookup.find(op);
			if (found == s_opcode_lookup.end())
			{
				fmt::throw_exception("Unhandled instruction '%s'", op);
			}
			const auto& encoding = found->second;

			inst->opcode = encoding.op;
			d0.opcode = encoding.op & 0x3F;
			s1.opcode_hi = (encoding.op > 0x3F)? 1 : 0;
			s0.exec_if_eq = encoding.exec_if_eq ? 1 : 0;
			s0.exec_if_gr = encoding.exec_if_gt ? 1 : 0;
			s0.exec_if_lt = encoding.exec_if_lt ? 1 : 0;
			d0.set_cond = encoding.set_cond ? 1 : 0;
			inst->bytecode[0] = d0.HEX;
			inst->bytecode[1] = s0.HEX;
			inst->bytecode[2] = s1.HEX;
		};

		std::string op, dst;
		std::vector<std::string> sources;

		std::stack<size_t> if_ops;
		std::stack<size_t> loop_ops;
		u32 pc = 0;

		FPIR ir{};

		for (const auto& instruction : instructions)
		{
			op.clear();
			dst.clear();
			sources.clear();
			decode_instruction(instruction, op, dst, sources);

			if (op.empty())
			{
				continue;
			}

			if (op.starts_with("IF."))
			{
				if_ops.push(ir.m_instructions.size());
			}
			else if (op == "LOOP")
			{
				loop_ops.push(ir.m_instructions.size());
			}
			else if (op == "ELSE")
			{
				ensure(!if_ops.empty());
				encode_branch_else(&ir.m_instructions[if_ops.top()], pc);
				continue;
			}
			else if (op == "ENDIF")
			{
				ensure(!if_ops.empty());
				encode_branch_end(&ir.m_instructions[if_ops.top()], pc);
				if_ops.pop();
				continue;
			}
			else if (op == "ENDLOOP")
			{
				ensure(!loop_ops.empty());
				encode_branch_end(&ir.m_instructions[loop_ops.top()], pc);
				loop_ops.pop();
				continue;
			}

			ir.m_instructions.push_back({});
			Instruction* target = &ir.m_instructions.back();
			pc += 4;

			encode_opcode(op, target);
			ensure(sources.size() == FP::get_operand_count(static_cast<FP_opcode>(target->opcode)), "Invalid operand count for opcode");

			if (dst.empty())
			{
				OPDEST dst{ .HEX = target->bytecode[0] };
				dst.no_dest = 1;
				target->bytecode[0] = dst.HEX;
			}
			else
			{
				ir.store(get_ref(dst), target);
			}

			int operand = 0;
			bool has_literal = false;
			for (const auto& source : sources)
			{
				if (source.front() == '#')
				{
					const auto literal = get_constants(source);
					ir.load(literal, operand++, target);
					has_literal = true;
					continue;
				}

				ir.load(get_ref(source), operand++, target);
			}

			if (has_literal)
			{
				pc += 4;
			}
		}

		if (!ir.m_instructions.empty())
		{
			OPDEST d0{ .HEX = ir.m_instructions.back().bytecode[0] };
			d0.end = 1;

			ir.m_instructions.back().bytecode[0] = d0.HEX;
		}

		return ir;
	}
}
