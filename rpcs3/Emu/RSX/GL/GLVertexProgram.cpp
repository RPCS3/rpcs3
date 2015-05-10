#include "stdafx.h"
#include "Utilities/Log.h"
#include "OpenGL.h"
#include "GLVertexProgram.h"

namespace gl
{
	namespace vertex_program
	{
		static const char swizzle_mask[4] = { 'x', 'y', 'z', 'w' };

		decompiler::decompiler(u32* data)
			: m_data(data)
		{
			gpu_program_builder::context.header = "!!ARBvp1.0";
			gpu_program_builder::context.extensions.push_back("NV_vertex_program3");
		}

		gpu4_program_context::argument decompiler::arg(SRC src)
		{
			gpu4_program_context::argument result;
			switch (src.reg_type)
			{
			case 1: //template register
				result = variable("TEMP", "R" + std::to_string(src.tmp_src));
				break;

			case 2: //input register
				result = predeclared_variable("vertex.attrib[" + std::to_string(d1.input_src) + "]");
				break;

			case 3: //constant
				if (d1.const_src < m_min_constant_id || d1.const_src > m_max_constant_id)
				{
					result = constant(0);
				}
				else
				{
					std::string constant_offset;
					if (d3.index_const)
					{
						std::string address_register = "A" + std::to_string(d0.addr_reg_sel_1);
						variable("ADDRESS", address_register);

						constant_offset = " + " + address_register + "." + (std::string(1, swizzle_mask[d0.addr_swz]));
					}
					result = predeclared_variable("constants[" + std::to_string(d1.const_src - m_min_constant_id) + constant_offset + "]");
				}
				break;

			default:
				throw std::runtime_error(fmt::Format("Bad src reg type: %d", fmt::by_value(src.reg_type)));
			}

			std::string mask;
			mask += swizzle_mask[src.swz_x];
			mask += swizzle_mask[src.swz_y];
			mask += swizzle_mask[src.swz_z];
			mask += swizzle_mask[src.swz_w];

			if (mask[0] == mask[1] && mask[1] == mask[2] && mask[2] == mask[3])
			{
				mask = std::string(1, mask[0]);
			}

			result.abs(src.abs).neg(src.neg).mask(mask);

			if (is_sca)
				result.mask("x");

			return result;
		}

		gpu4_program_context::argument decompiler::arg_dst()
		{
			if (is_sca)
			{
				std::string mask;
				if (d3.sca_writemask_x) mask += "x";
				if (d3.sca_writemask_y) mask += "y";
				if (d3.sca_writemask_z) mask += "z";
				if (d3.sca_writemask_w) mask += "w";

				return variable("TEMP", "R" + std::to_string(d3.sca_dst_tmp)).mask(mask);
			}

			gpu_program_builder<>::variable_t result;
			if (d3.dst == 0x1f)
			{
				result = variable("TEMP", "R" + std::to_string(d0.dst_tmp));
			}
			else
			{
				if (d0.vec_result)
				{
					result = variable("TEMP", "oR" + std::to_string(d3.dst));
				}
				else
				{
					result = variable("ADDRESS", "A" + std::to_string(d3.dst));
				}
			}

			std::string mask;
			if (d3.vec_writemask_x) mask += "x";
			if (d3.vec_writemask_y) mask += "y";
			if (d3.vec_writemask_z) mask += "z";
			if (d3.vec_writemask_w) mask += "w";

			return result.mask(mask);
		}

		gpu4_program_context::argument decompiler::condition()
		{
			if (!d0.cond_test_enable)
				return{};

			enum
			{
				lt = 0x1,
				eq = 0x2,
				gt = 0x4,
			};

			if (d0.cond == (lt | gt | eq))
				return{};

			static const char* cond_string_table[(lt | gt | eq) + 1] =
			{
				"error",
				"LT",
				"EQ",
				"LE",
				"GT",
				"NE",
				"GE",
				"error"
			};

			std::string mask;
			mask += swizzle_mask[d0.mask_x];
			mask += swizzle_mask[d0.mask_y];
			mask += swizzle_mask[d0.mask_z];
			mask += swizzle_mask[d0.mask_w];

			if (mask[0] == mask[1] && mask[1] == mask[2] && mask[2] == mask[3])
			{
				mask = std::string(1, mask[0]);
			}

			return predeclared_variable(cond_string_table[d0.cond]).mask(mask);
		}

		gpu4_program_context::argument decompiler::texture()
		{
			return predeclared_variable("texture[0]");
		}

		void decompiler::sca_op(const std::string& instruction, gpu4_program_context::argument dst, gpu4_program_context::argument src)
		{
			std::string dst_mask = dst.get_mask().get();
			std::string src_mask = src.get_mask().get();
			if (dst_mask == src_mask || dst_mask.empty() || src_mask.empty())
			{
				op(instruction, dst, src);
			}
			else
			{
				auto tmp = variable("TEMP", "scalar");
				op(instruction, tmp, src);
				op("MOV", dst, tmp);
			}
		}

		std::string decompiler::instr(const std::string& instruction)
		{
			if (d0.cond_update_enable_0 && d0.cond_update_enable_1)
			{
				return instruction + (d0.cond_reg_sel_1 ? "1" : "");
			}

			return instruction;
		}

		decompiler& decompiler::decompile(u32 start, std::unordered_map<u32, color4>& constants)
		{
			if (constants.empty())
			{
				m_min_constant_id = 0;
				m_max_constant_id = -1;
			}
			else
			{
				auto find_fn = [](std::pair<u32, color4> a, std::pair<u32, color4> b)
				{
					return a.first < b.first;
				};

				m_min_constant_id = std::min_element(constants.begin(), constants.end(), find_fn)->first;
				m_max_constant_id = std::max_element(constants.begin(), constants.end(), find_fn)->first;
			}

			std::string constants_initialization = "{\n";

			if (m_max_constant_id != -1)
			{
				for (uint i = m_min_constant_id; i < m_max_constant_id; ++i)
				{
					auto result = constants.find(i);
					if (result == constants.end())
					{
						constants_initialization += "\t\t{0, 0, 0, 0},\n";
					}
					else
					{
						constants_initialization += "\t\t{" +
							fmt::format("%g", result->second.r) + ", " +
							fmt::format("%g", result->second.g) + ", " +
							fmt::format("%g", result->second.b) + ", " +
							fmt::format("%g", result->second.a) + "},\n";
					}
				}

				constants_initialization += "\t\t{" +
					fmt::format("%g", constants[m_max_constant_id].r) + ", " +
					fmt::format("%g", constants[m_max_constant_id].g) + ", " +
					fmt::format("%g", constants[m_max_constant_id].b) + ", " +
					fmt::format("%g", constants[m_max_constant_id].a) + "}\n}";

				variable("PARAM", "constants[" + std::to_string(m_max_constant_id - m_min_constant_id + 1) + "]", constants_initialization);
			}

			gpu_program_builder<>::op("MOV", variable("TEMP", "oR0"), constant(0.f, 0.f, 0.f, 1.f));

			for (int i = start; i < 512; ++i)
			{
				d0.HEX = m_data[i * 4 + 0];
				d1.HEX = m_data[i * 4 + 1];
				d2.HEX = m_data[i * 4 + 2];
				d3.HEX = m_data[i * 4 + 3];

				src[0].src0l = d2.src0l;
				src[0].src0h = d1.src0h;
				src[1].src1 = d2.src1;
				src[2].src2l = d3.src2l;
				src[2].src2h = d2.src2h;

				src[0].abs = d0.src0_abs;
				src[1].abs = d0.src1_abs;
				src[2].abs = d0.src2_abs;

				is_sca = true;
				switch (d1.sca_opcode)
				{
				case RSX_SCA_OPCODE_NOP: break;
				case RSX_SCA_OPCODE_MOV: op(instr("MOV"), arg_dst(), arg(src[2])); break;
				case RSX_SCA_OPCODE_RCP: sca_op(instr("RCP"), arg_dst(), arg(src[2])); break;
				case RSX_SCA_OPCODE_RCC: sca_op(instr("RCC"), arg_dst(), arg(src[2])); break;
				case RSX_SCA_OPCODE_RSQ: sca_op(instr("RSQ"), arg_dst(), arg(src[2])); break;
				case RSX_SCA_OPCODE_EXP: sca_op(instr("EXP"), arg_dst(), arg(src[2])); break;
				case RSX_SCA_OPCODE_LOG: sca_op(instr("LOG"), arg_dst(), arg(src[2])); break;
				case RSX_SCA_OPCODE_LIT: op(instr("LIT"), arg_dst(), arg(src[2])); break;
				case RSX_SCA_OPCODE_BRA: op(instr("BRA"), arg(src[2])); break;
				case RSX_SCA_OPCODE_BRI: op(instr("BRI"), arg(src[2])); break;
				case RSX_SCA_OPCODE_CAL: op(instr("CAL"), arg(src[2])); break;
				case RSX_SCA_OPCODE_CLI: op(instr("CLI"), arg(src[2])); break;
				case RSX_SCA_OPCODE_RET: op("RET"); break;
				case RSX_SCA_OPCODE_LG2: sca_op(instr("LG2"), arg_dst(), arg(src[2])); break;
				case RSX_SCA_OPCODE_EX2: sca_op(instr("EX2"), arg_dst(), arg(src[2])); break;
				case RSX_SCA_OPCODE_SIN: sca_op(instr("SIN"), arg_dst(), arg(src[2])); break;
				case RSX_SCA_OPCODE_COS: sca_op(instr("COS"), arg_dst(), arg(src[2])); break;
				//case RSX_SCA_OPCODE_BRB: op(instr("BRB"), arg_dst(), arg(src[2])); break;
				//case RSX_SCA_OPCODE_CLB: op(instr("CLB"), arg_dst(), arg(src[2])); break;
				//case RSX_SCA_OPCODE_PSH: op(instr("PSH"), arg_dst(), arg(src[2])); break;
				//case RSX_SCA_OPCODE_POP: op(instr("POP"), arg_dst(), arg(src[2])); break;

				default:
					throw std::runtime_error(fmt::format("#Unknown vp opcode 0x%x", fmt::by_value(d1.vec_opcode)));
				}

				is_sca = false;
				switch (d1.vec_opcode)
				{
				case RSX_VEC_OPCODE_NOP: break;
				case RSX_VEC_OPCODE_MOV: op(instr("MOV"), arg_dst(), arg(src[0])); break;
				case RSX_VEC_OPCODE_MUL: op(instr("MUL"), arg_dst(), arg(src[0]), arg(src[1])); break;
				case RSX_VEC_OPCODE_ADD: op(instr("ADD"), arg_dst(), arg(src[0]), arg(src[2])); break;
				case RSX_VEC_OPCODE_MAD: op(instr("MAD"), arg_dst(), arg(src[0]), arg(src[1]), arg(src[2])); break;
				case RSX_VEC_OPCODE_DP3: op(instr("DP3"), arg_dst(), arg(src[0]), arg(src[1])); break;
				case RSX_VEC_OPCODE_DPH: op(instr("DPH"), arg_dst(), arg(src[0]), arg(src[1])); break;
				case RSX_VEC_OPCODE_DP4: op(instr("DP4"), arg_dst(), arg(src[0]), arg(src[1])); break;
				case RSX_VEC_OPCODE_DST: op(instr("DST"), arg_dst(), arg(src[0]), arg(src[1])); break;
				case RSX_VEC_OPCODE_MIN: op(instr("MIN"), arg_dst(), arg(src[0]), arg(src[1])); break;
				case RSX_VEC_OPCODE_MAX: op(instr("MAX"), arg_dst(), arg(src[0]), arg(src[1])); break;
				case RSX_VEC_OPCODE_SLT: op(instr("SLT"), arg_dst(), arg(src[0]), arg(src[1])); break;
				case RSX_VEC_OPCODE_SGE: op(instr("SGE"), arg_dst(), arg(src[0]), arg(src[1])); break;
				case RSX_VEC_OPCODE_ARL: op(instr("ARL"), arg_dst(), arg(src[0])); break;
				case RSX_VEC_OPCODE_FRC: op(instr("FRC"), arg_dst(), arg(src[0])); break;
				case RSX_VEC_OPCODE_FLR: op(instr("FLR"), arg_dst(), arg(src[0])); break;
				case RSX_VEC_OPCODE_SEQ: op(instr("SEQ"), arg_dst(), arg(src[0]), arg(src[1])); break;
				case RSX_VEC_OPCODE_SFL: op(instr("SFL"), arg_dst(), arg(src[0])); break;
				case RSX_VEC_OPCODE_SGT: op(instr("SGT"), arg_dst(), arg(src[0]), arg(src[1])); break;
				case RSX_VEC_OPCODE_SLE: op(instr("SLE"), arg_dst(), arg(src[0]), arg(src[1])); break;
				case RSX_VEC_OPCODE_SNE: op(instr("SNE"), arg_dst(), arg(src[0]), arg(src[1])); break;
				case RSX_VEC_OPCODE_STR: op(instr("STR"), arg_dst(), arg(src[0])); break;
				case RSX_VEC_OPCODE_SSG: op(instr("SSG"), arg_dst(), arg(src[0])); break;
				case RSX_VEC_OPCODE_TXL: op(instr("TXL"), arg_dst(), texture(), arg(src[0]), predeclared_variable("2D")); break;

				default:
					throw std::runtime_error(fmt::format("#Unknown vp opcode 0x%x", fmt::by_value(d1.vec_opcode)));
				}

				if (d3.end)
					break;
			}

			struct table_entry
			{
				std::string destination;
				std::string source;
				std::string source_channel;
			};

			const table_entry table[] =
			{
				//{ "result.position", "oR0" },
				{ "result.color.front.primary", "oR1" },
				{ "result.color.front.secondary", "oR2" },
				{ "result.color.back.primary", "oR3" },
				{ "result.color.back.secondary", "oR4" },
				{ "result.fogcoord", "oR5", "x" },
				{ "result.clip[0]", "oR5", "y" },
				{ "result.clip[1]", "oR5", "z" },
				{ "result.clip[2]", "oR5", "w" },
				{ "result.texcoord[9]", "oR6" },
				{ "result.pointsize", "oR6", "x" },
				{ "result.clip[3]", "oR6", "y" },
				{ "result.clip[4]", "oR6", "z" },
				{ "result.clip[5]", "oR6", "w" },
				{ "result.texcoord[9]", "oR6" },
				{ "result.texcoord[0]", "oR7" },
				{ "result.texcoord[1]", "oR8" },
				{ "result.texcoord[2]", "oR9" },
				{ "result.texcoord[3]", "oR10" },
				{ "result.texcoord[4]", "oR11" },
				{ "result.texcoord[5]", "oR12" },
				{ "result.texcoord[6]", "oR13" },
				{ "result.texcoord[7]", "oR14" },
				{ "result.texcoord[8]", "oR15" }
			};

			if (variables.find("oR0") != variables.end())
			{
				gpu_program_builder<>::op("DP4", predeclared_variable("result.position.x"), variable("TEMP", "oR0"), predeclared_variable("state.matrix.program[0].row[0]"));
				gpu_program_builder<>::op("DP4", predeclared_variable("result.position.y"), variable("TEMP", "oR0"), predeclared_variable("state.matrix.program[0].row[1]"));
				gpu_program_builder<>::op("DP4", predeclared_variable("result.position.z"), variable("TEMP", "oR0"), predeclared_variable("state.matrix.program[0].row[2]"));
				gpu_program_builder<>::op("DP4", predeclared_variable("result.position.w"), variable("TEMP", "oR0"), predeclared_variable("state.matrix.program[0].row[3]"));
			}

			for (auto& entry : table)
			{
				if (variables.find(entry.source) != variables.end())
				{
					gpu_program_builder<>::op("MOV", predeclared_variable(entry.destination),
						variable("TEMP", entry.source).mask(entry.source_channel));
				}
			}

			link();
			m_shader = context.make();

			return *this;
		}
	}
}
