#include "stdafx.h"
#include "FPASM.h"
#include "Emu/RSX/Program/RSXFragmentProgram.h"

#ifndef _WIN32
#define sscanf_s sscanf
#endif

namespace rsx::assembler
{
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

	std::vector<Instruction> FPIR::build()
	{
		return m_instructions;
	}

	FPIR FPIR::from_source(const std::string& asm_)
	{
		std::vector<std::string> instructions = fmt::split(asm_, { "\n", ";" });
		if (instructions.empty())
		{
			return {};
		}

		auto transform_inst = [](const std::string& s)
		{
			std::string result;
			result.reserve(s.size());

			bool literal = false;
			for (auto& c : s)
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

		auto decode_instruction = [&](const std::string& inst, std::string& op, std::string& dst, std::vector<std::string>& sources)
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

		auto get_ref = [](const std::string& reg)
		{
			ensure(reg.length() > 1, "Invalid register specifier");

			const auto index = std::stoi(reg.substr(1));
			RegisterRef ref
			{
				.reg { .id = index, .f16 = false },
				.mask = 0x0F
			};

			if (reg[0] == 'H' || reg[0] == 'h')
			{
				ref.reg.f16 = true;
			}

			return ref;
		};

		auto get_constants = [](const std::string& reg) -> std::array<f32, 4>
		{
			float x, y, z, w;
			if (sscanf_s(reg.c_str(), "#{%f|%f|%f|%f}", &x, &y, &z, &w) == 4)
			{
				return { x, y, z, w };
			}

			if (sscanf_s(reg.c_str(), "#{%f}", &x) == 1)
			{
				return { x, x, x, x };
			}

			fmt::throw_exception("Invalid constant literal");
		};

		auto encode_opcode = [](const std::string& op, Instruction* inst)
		{
			OPDEST d0 { .HEX = inst->bytecode[0] };
			SRC0 s0 { .HEX = inst->bytecode[1] };

#define SET_OPCODE(code) \
			do { \
				inst->opcode = d0.opcode = code; \
				s0.exec_if_eq = s0.exec_if_gr = s0.exec_if_lt = 1; \
				inst->bytecode[0] = d0.HEX; \
				inst->bytecode[1] = s0.HEX; \
			} while (0)

			if (op == "MOV")
			{
				SET_OPCODE(RSX_FP_OPCODE_MOV);
				return;
			}

			if (op == "ADD")
			{
				SET_OPCODE(RSX_FP_OPCODE_ADD);
				return;
			}

			if (op == "MAD" || op == "FMA")
			{
				SET_OPCODE(RSX_FP_OPCODE_MAD);
				return;
			}

			if (op == "UP4S")
			{
				SET_OPCODE(RSX_FP_OPCODE_UP4);
				return;
			}

			if (op == "PK4S")
			{
				SET_OPCODE(RSX_FP_OPCODE_PK4);
				return;
			}

#undef SET_OPCODE

			fmt::throw_exception("Unhandled instruction '%s'", op);
		};

		std::string op, dst;
		std::vector<std::string> sources;

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

			ir.m_instructions.push_back({});
			Instruction* target = &ir.m_instructions.back();

			encode_opcode(op, target);
			ensure(sources.size() == FP::get_operand_count(static_cast<FP_opcode>(target->opcode)), "Invalid operand count for opcode");

			if (dst.empty())
			{
				OPDEST dst{};
				dst.no_dest = 1;
				target->bytecode[0] = dst.HEX;
			}
			else
			{
				ir.store(get_ref(dst), target);
			}

			int operand = 0;
			for (const auto& source : sources)
			{
				if (source.front() == '#')
				{
					const auto literal = get_constants(source);
					ir.load(literal, operand++, target);
					continue;
				}

				ir.load(get_ref(source), operand++, target);
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
