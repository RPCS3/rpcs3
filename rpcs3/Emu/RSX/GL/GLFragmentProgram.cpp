#include "stdafx.h"
#include "Utilities/Log.h"
#include "Emu/Memory/Memory.h"
#include "OpenGL.h"
#include "GLFragmentProgram.h"
#include "Emu/RSX/RSXFragmentProgram.h"

namespace gl
{
	namespace fragment_program
	{
		static const char swizzle_mask[4] = { 'x', 'y', 'z', 'w' };

		decompiler::decompiler(vm::ptr<u32> data) : m_data(data)
		{
			gpu_program_builder::context.header = "!!ARBfp1.0";
			gpu_program_builder::context.extensions.push_back("NV_fragment_program2");
			gpu_program_builder::context.extensions.push_back("ARB_draw_buffers");
		}

			
		gpu4_program_context::argument decompiler::condition()
		{
			if (src0.exec_if_gr && src0.exec_if_lt && src0.exec_if_eq)
			{
				return{};
			}

			static const char swizzle_selector[4] = { 'x', 'y', 'z', 'w' };

			std::string mask, cond;
			mask += swizzle_selector[src0.cond_swizzle_x];
			mask += swizzle_selector[src0.cond_swizzle_y];
			mask += swizzle_selector[src0.cond_swizzle_z];
			mask += swizzle_selector[src0.cond_swizzle_w];

			if (mask[0] == mask[1] && mask[1] == mask[2] && mask[2] == mask[3])
			{
				mask = std::string(1, mask[0]);
			}

			if (src0.exec_if_gr && src0.exec_if_eq)
			{
				cond = "GE";
			}
			else if (src0.exec_if_lt && src0.exec_if_eq)
			{
				cond = "LE";
			}
			else if (src0.exec_if_gr && src0.exec_if_lt)
			{
				cond = "NE";
			}
			else if (src0.exec_if_gr)
			{
				cond = "GT";
			}
			else if (src0.exec_if_lt)
			{
				cond = "LT";
			}
			else //if(src0.exec_if_eq)
			{
				cond = "EQ";
			}

			return predeclared_variable(cond/* + std::to_string(src0.cond_reg_index)*/).mask(mask);
		}

		gpu4_program_context::argument decompiler::texture()
		{
			return predeclared_variable("texture[" + std::to_string(dst.tex_num) + "]");
		}

		std::string decompiler::instr(std::string instruction, int flags)
		{
			if (flags & instr_flag::H)
			{
				if (dst.fp16)
					instruction += "H";
				else
					instruction += "R";
			}

			if (flags & instr_flag::C)
			{
				if (dst.set_cond)
				{
					instruction += "C";// + std::to_string(src0.cond_mod_reg_index);
				}
			}

			if (flags & instr_flag::S)
			{
				if (!src1.scale && dst.saturate)
				{
					instruction += "_SAT";
				}
			}

			return instruction;
		}

		decompiler& decompiler::decompile(u32 control)
		{
			u32 begin = m_data.addr();
			//for (int i = 0; i < 512; ++i)
			while (true)
			{
				if (next_is_constant)
				{
					m_data += 4;
					next_is_constant = false;
				}

				{
					auto finded = custom_instruction.find(m_data.addr() - begin);

					if (finded != custom_instruction.end())
					{
						for (auto &value : finded->second)
							gpu_program_builder<>::op(value);
					}
				}

				dst.HEX = swap_bytes(m_data[0]);
				src0.HEX = swap_bytes(m_data[1]);
				src1.HEX = swap_bytes(m_data[2]);
				src2.HEX = swap_bytes(m_data[3]);
				m_data += 4;

				u32 opcode = dst.opcode | (src1.opcode_is_branch << 6);

				switch (opcode)
				{
				case RSX_FP_OPCODE_NOP: break;
				case RSX_FP_OPCODE_MOV: op(instr("MOV", F | I | C | S | H), arg(dst), arg(src0)); break;
				case RSX_FP_OPCODE_MUL: op(instr("MUL", F | I | C | S | H), arg(dst), arg(src0), arg(src1)); break;
				case RSX_FP_OPCODE_ADD: op(instr("ADD", F | I | C | S | H), arg(dst), arg(src0), arg(src1)); break;
				case RSX_FP_OPCODE_MAD: op(instr("MAD", F | I | C | S | H), arg(dst), arg(src0), arg(src1), arg(src2)); break;
				case RSX_FP_OPCODE_DP3: op(instr("DP3", F | C | S | H), arg(dst), arg(src0), arg(src1)); break;
				case RSX_FP_OPCODE_DP4: op(instr("DP4", F | C | S | H), arg(dst), arg(src0), arg(src1)); break;
				case RSX_FP_OPCODE_DST: op(instr("DST", F | C | S | H), arg(dst), arg(src0), arg(src1)); break;
				case RSX_FP_OPCODE_MIN: op(instr("MIN", F | I | C | S | H), arg(dst), arg(src0), arg(src1)); break;
				case RSX_FP_OPCODE_MAX: op(instr("MAX", F | I | C | S | H), arg(dst), arg(src0), arg(src1)); break;
				case RSX_FP_OPCODE_SLT: op(instr("SLT", F | I | C | S | H), arg(dst), arg(src0), arg(src1)); break;
				case RSX_FP_OPCODE_SGE: op(instr("SGE", F | I | C | S | H), arg(dst), arg(src0), arg(src1)); break;
				case RSX_FP_OPCODE_SLE: op(instr("SLE", F | I | C | S | H), arg(dst), arg(src0), arg(src1)); break;
				case RSX_FP_OPCODE_SGT: op(instr("SGT", F | I | C | S | H), arg(dst), arg(src0), arg(src1)); break;
				case RSX_FP_OPCODE_SNE: op(instr("SNE", F | I | C | S | H), arg(dst), arg(src0), arg(src1)); break;
				case RSX_FP_OPCODE_SEQ: op(instr("SEQ", F | I | C | S | H), arg(dst), arg(src0), arg(src1)); break;
				case RSX_FP_OPCODE_FRC: op(instr("FRC", F | C | S | H), arg(dst), arg(src0)); break;
				case RSX_FP_OPCODE_FLR: op(instr("FLR", F | I | C | S | H), arg(dst), arg(src0)); break;
				case RSX_FP_OPCODE_KIL: gpu_program_builder<>::op(instr("KIL"), condition()); break;
				case RSX_FP_OPCODE_PK4: op(instr("PK4", F | I | H), arg(dst), arg(src0)); break;
				case RSX_FP_OPCODE_UP4: op(instr("UP4", F | I | H), arg(dst), arg(src0)); break;
				case RSX_FP_OPCODE_DDX: op(instr("DDX", F | I | H), arg(dst), arg(src0)); break;
				case RSX_FP_OPCODE_DDY: op(instr("DDY", F | I | H), arg(dst), arg(src0)); break;
				case RSX_FP_OPCODE_TEX: op(instr("TEX"), arg(dst), arg(src0), texture(), predeclared_variable("2D")); break;
				case RSX_FP_OPCODE_TXP: op(instr("TXP"), arg(dst), texture(), arg(src0), predeclared_variable("2D")); break;
				case RSX_FP_OPCODE_TXD: op(instr("TXD"), arg(dst), texture(), arg(src0), predeclared_variable("2D")); break;
				case RSX_FP_OPCODE_RCP: op(instr("RCP", F | I | C | S | H), arg(dst), arg(src0)); break;
				case RSX_FP_OPCODE_RSQ: op(instr("RSQ", F | I | C | S | H), arg(dst), arg(src0)); break;
				case RSX_FP_OPCODE_EX2: op(instr("EX2", F | I | C | S | H), arg(dst), arg(src0).mask("x")); break;
				case RSX_FP_OPCODE_LG2: op(instr("LG2", F | I | C | S | H), arg(dst), arg(src0).mask("x")); break;
				case RSX_FP_OPCODE_LIT: op(instr("LIT", F | I | C | S | H), arg(dst), arg(src0), arg(src1)); break;
				case RSX_FP_OPCODE_LRP: op(instr("LRP", F | I | C | S | H), arg(dst), arg(src0), arg(src1)); break;
				case RSX_FP_OPCODE_STR: op(instr("STR", F | I | C | S | H), arg(dst), arg(src0), arg(src1)); break;
				case RSX_FP_OPCODE_SFL: op(instr("SFL", F | I | C | S | H), arg(dst), arg(src0), arg(src1)); break;
				case RSX_FP_OPCODE_COS: op(instr("COS", F | C | S | H), arg(dst), arg(src0).mask("x")); break;
				case RSX_FP_OPCODE_SIN: op(instr("SIN", F | C | S | H), arg(dst), arg(src0).mask("x")); break;
				case RSX_FP_OPCODE_PK2: op(instr("PK2", F | I | C | S | H), arg(dst), arg(src0), arg(src1)); break;
				case RSX_FP_OPCODE_UP2: op(instr("UP2", F | I | C | S | H), arg(dst), arg(src0), arg(src1)); break;
				case RSX_FP_OPCODE_POW: op(instr("POW", F | I | C | S | H), arg(dst), arg(src0), arg(src1)); break;
				case RSX_FP_OPCODE_PKB: op(instr("PKB", F | I | C | S | H), arg(dst), arg(src0), arg(src1)); break;
				case RSX_FP_OPCODE_UPB: op(instr("UPB", F | I | C | S | H), arg(dst), arg(src0), arg(src1)); break;
				case RSX_FP_OPCODE_PK16: op(instr("PK16", F | I | C | S | H), arg(dst), arg(src0), arg(src1)); break;
				case RSX_FP_OPCODE_UP16: op(instr("UP16", F | I | C | S | H), arg(dst), arg(src0), arg(src1)); break;
				case RSX_FP_OPCODE_BEM: op(instr("BEM", F | I | C | S | H), arg(dst), arg(src0), arg(src1)); break;
				case RSX_FP_OPCODE_PKG: op(instr("PKG", F | I | C | S | H), arg(dst), arg(src0), arg(src1)); break;
				case RSX_FP_OPCODE_UPG: op(instr("UPG", F | I | C | S | H), arg(dst), arg(src0), arg(src1)); break;
				case RSX_FP_OPCODE_DP2A: op(instr("DP2A", F | I | C | S | H), arg(dst), arg(src0), arg(src1)); break;
				case RSX_FP_OPCODE_TXL: op(instr("TXL", F | I | C | S | H), arg(dst), texture(), arg(src0), predeclared_variable("2D")); break;
				case RSX_FP_OPCODE_TXB: op(instr("TXB", F | I | C | S | H), arg(dst), texture(), arg(src0), predeclared_variable("2D")); break;
				case RSX_FP_OPCODE_TEXBEM: op(instr("TEXBEM", F | I | C | S | H), arg(dst), arg(src0), arg(src1)); break;
				case RSX_FP_OPCODE_TXPBEM: op(instr("TXPBEM", F | I | C | S | H), arg(dst), arg(src0), arg(src1)); break;
				case RSX_FP_OPCODE_BEMLUM: op(instr("BEMLUM", F | I | C | S | H), arg(dst), arg(src0), arg(src1)); break;
				case RSX_FP_OPCODE_REFL: op(instr("REFL", F | I | C | S | H), arg(dst), arg(src0), arg(src1)); break;
				case RSX_FP_OPCODE_TIMESWTEX: op(instr("TIMESWTEX", F | I | C | S | H), arg(dst), arg(src0), arg(src1)); break;
				case RSX_FP_OPCODE_DP2: op(instr("DP2", F | I | C | S | H), arg(dst), arg(src0), arg(src1)); break;
				case RSX_FP_OPCODE_NRM: op(instr("NRM", F | I | C | S | H), arg(dst), arg(src0)); break;
				case RSX_FP_OPCODE_DIV: op(instr("DIV", F | I | C | S | H), arg(dst), arg(src0), arg(src1).mask("x")); break;
				case RSX_FP_OPCODE_DIVSQ:
				{
					auto arg_dst = arg(dst);
					auto arg_src0 = arg(src0);
					auto arg_src1 = arg(src1).mask("x");
					auto arg_temp = variable("TEMP", "local_temp").mask(arg_src1.get_mask());

					//DST = SRC0 * inversesqrt(SRC1)
					op(instr("RSQ", F | I | C | S | H), arg_temp, arg_src1);
					op(instr("MUL", F | I | C | S | H), arg_dst, arg_src0, arg_temp);
				}
				break;
				case RSX_FP_OPCODE_LIF: op(instr("LIF", F | I | C | S | H), arg(dst), arg(src0), arg(src1)); break;
				case RSX_FP_OPCODE_FENCT: /*op(instr("FENCT", F | I | C | S | H), arg(dst), arg(src0), arg(src1));*/ break;
				case RSX_FP_OPCODE_FENCB: /*op(instr("FENCB", F | I | C | S | H), arg(dst), arg(src0), arg(src1));*/ break;

				case RSX_FP_OPCODE_BRK: op("BRK"); break;
					//case RSX_FP_OPCODE_CAL: op(instr("CAL"), arg(src0)); break;
				case RSX_FP_OPCODE_IFE:
					if (src2.end_offset != src1.else_offset)
						custom_instruction[src1.else_offset << 2].emplace_back("ELSE");
					custom_instruction[src2.end_offset << 2].emplace_back("ENDIF");
					gpu_program_builder<>::op("IF", condition());
					break;
				case RSX_FP_OPCODE_LOOP:
					op(instr("LOOP"), predeclared_variable(fmt::format("{%d, %d, %d}", src1.end_counter, src1.init_counter, src1.increment)));
					custom_instruction[src2.end_offset << 2].emplace_back("ENDLOOP");
					break;
					//case RSX_FP_OPCODE_REP: op(instr("SEQ", F | I | C | S | H), arg(dst), arg(src0), arg(src1)); break;
					//case RSX_FP_OPCODE_RET: op(instr("SEQ", F | I | C | S | H), arg(dst), arg(src0), arg(src1)); break;
				default:
					throw std::runtime_error(fmt::format("Unknown/illegal fp instruction: 0x%x", opcode));
					//LOG_ERROR(RSX, "Unknown/illegal fp instruction: 0x%x", opcode);
					break;
				}

				if (src1.scale)
				{
					std::string instruction;
					int value = 0;
					switch (src1.scale)
					{
					case 0: break;
					case 1: value = 2; instruction = "MUL"; break;
					case 2: value = 4; instruction = "MUL"; break;
					case 3: value = 8; instruction = "MUL"; break;
					case 5: value = 2; instruction = "DIV"; break;
					case 6: value = 4; instruction = "DIV"; break;
					case 7: value = 8; instruction = "DIV"; break;

					default:
						throw std::runtime_error(fmt::format("Bad scale: %d", fmt::by_value(src1.scale)));
					}

					if (dst.saturate)
						instruction += "_SAT";

					auto arg_dst = arg(dst);

					std::string mask = ((gpu4_program_context::mask_t)arg_dst.get_mask()).symplify().get();

					for (size_t i = 0; i < mask.length() - 1/*skip dot*/; ++i)
					{
						gpu4_program_context::argument arg = arg_dst;
						auto arg_dst_swizzle = arg.mask(std::string(1, swizzle_mask[i]));

						op(instr(instruction), arg_dst_swizzle, arg_dst_swizzle,
							constant(value).mask(std::string(1, swizzle_mask[i])));
					}
				}

				if (dst.end)
					break;
			}

			const std::pair<std::string, std::string> table[] =
			{
				{ "result.color[0]", control & 0x40 ? "R0" : "H0" },
				{ "result.color[1]", control & 0x40 ? "R2" : "H4" },
				{ "result.color[2]", control & 0x40 ? "R3" : "H6" },
				{ "result.color[3]", control & 0x40 ? "R4" : "H8" },
			};

			for (auto &entry : table)
			{
				if (variables.find(entry.second) != variables.end())
				{
					gpu_program_builder<>::op((control & 0x40 ? "MOVR" : "MOVH"), predeclared_variable(entry.first),
						variable("TEMP", entry.second));
				}
			}

			if (control & 0xe)
			{
				gpu_program_builder<>::op((control & 0x40 ? "MOVR" : "MOVH"), predeclared_variable("result.depth"),
					variable("TEMP", control & 0x40 ? "R1" : "H2").mask("z"));
			}
			link();
			m_shader = context.make();

			return *this;
		}
	}
}
