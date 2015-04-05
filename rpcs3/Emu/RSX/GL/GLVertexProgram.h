#pragma once
#include "GLShaderParam.h"
#include "Emu/RSX/RSXVertexProgram.h"
#include "Utilities/Thread.h"
#include <set>
#include "gpu_program.h"

struct GLVertexDecompilerThread : public ThreadBase
{
	D0 d0;
	D1 d1;
	D2 d2;
	D3 d3;
	SRC src[3];

	struct FuncInfo
	{
		u32 offset;
		std::string name;
	};

	struct Instruction
	{
		std::vector<std::string> body;
		int open_scopes;
		int close_scopes;
		int put_close_scopes;
		int do_count;

		void reset()
		{
			body.clear();
			put_close_scopes = open_scopes = close_scopes = do_count = 0;
		}
	};

	static const size_t m_max_instr_count = 512;
	Instruction m_instructions[m_max_instr_count];
	Instruction* m_cur_instr;
	size_t m_instr_count;

	std::set<int> m_jump_lvls;
	std::vector<std::string> m_body;
	std::vector<FuncInfo> m_funcs;

	//wxString main;
	std::string& m_shader;
	u32 *m_data;
	u32 m_start;
	GLParamArray& m_parr;

	GLVertexDecompilerThread(u32 start, u32* data, std::string& shader, GLParamArray& parr)
		: ThreadBase("Vertex Shader Decompiler Thread")
		, m_data(data)
		, m_start(start)
		, m_shader(shader)
		, m_parr(parr)
	{
		m_funcs.emplace_back();
		m_funcs[0].offset = 0;
		m_funcs[0].name = "main";
		m_funcs.emplace_back();
		m_funcs[1].offset = 0;
		m_funcs[1].name = "func0";
		//m_cur_func->body = "\tgl_Position = vec4(0.0f, 0.0f, 0.0f, 1.0f);\n";
	}

	std::string GetMask(bool is_sca);
	std::string GetVecMask();
	std::string GetScaMask();
	std::string GetDST(bool is_sca = false);
	std::string GetSRC(const u32 n);
	std::string GetFunc();
	std::string GetTex();
	std::string GetCond();
	std::string AddAddrMask();
	std::string AddAddrReg();
	u32 GetAddr();
	std::string Format(const std::string& code);

	void AddCodeCond(const std::string& dst, const std::string& src);
	void AddCode(const std::string& code);
	void SetDST(bool is_sca, std::string value);
	void SetDSTVec(const std::string& code);
	void SetDSTSca(const std::string& code);
	std::string BuildFuncBody(const FuncInfo& func);
	std::string BuildCode();

	virtual void Task();
};

class GLVertexProgram
{ 
public:
	GLVertexProgram();
	~GLVertexProgram();

	GLParamArray parr;
	u32 id;
	std::string shader;

	void Decompile(u32 start, u32 *prog);
	void DecompileAsync(u32 start, u32 *prog);
	void Wait();
	void Compile();

private:
	GLVertexDecompilerThread* m_decompiler_thread;
	void Delete();
};


namespace gl
{
	namespace vertex_program
	{
		static const char swizzle_mask[4] = { 'x', 'y', 'z', 'w' };

		class decompiler : gpu_program_builder<>
		{
			u32* m_data;

			D0 d0;
			D1 d1;
			D2 d2;
			D3 d3;
			SRC src[3];

			bool is_sca = false;

			std::string m_shader;
			uint m_min_constant_id;
			uint m_max_constant_id;

		public:
			decompiler(u32* data)
				: m_data(data)
			{
				gpu_program_builder::context.header = "!!ARBvp1.0";
				gpu_program_builder::context.extensions.push_back("NV_vertex_program3");
			}

		private:
			gpu4_program_context::argument arg(SRC src)
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
						result = predeclared_variable("constants[" + std::to_string(d1.const_src - m_min_constant_id) + "]");
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

			gpu4_program_context::argument arg_dst()
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

			gpu4_program_context::argument condition()
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
					"lessThan",
					"equal",
					"lessThanEqual",
					"greaterThan",
					"notEqual",
					"greaterThanEqual",
					"error"
				};

				std::string mask;
				mask += swizzle_mask[d0.mask_x];
				mask += swizzle_mask[d0.mask_y];
				mask += swizzle_mask[d0.mask_z];
				mask += swizzle_mask[d0.mask_w];

				return predeclared_variable(cond_string_table[d0.cond]).mask(mask);
			}

			gpu4_program_context::argument texture()
			{
				return predeclared_variable("texture[0]");
			}

			context_t::operation& op(const std::string& instruction)
			{
				return gpu_program_builder<>::op(instruction).condition(context.get(condition()));
			}

			template<typename ...T>
			context_t::operation& op(const std::string& instruction, T... args)
			{
				return gpu_program_builder<>::op(instruction, args...).condition(context.get(condition()));
			}

			std::string instr(std::string instruction, int flags = 0)
			{
				//TODO
				return instruction;
			}

		public:
			decompiler& decompile(u32 start, std::unordered_map<u32, color4>& constants)
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

				if (m_max_constant_id >= 0)
				{
					for (int i = m_min_constant_id; i < m_max_constant_id; ++i)
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
					case RSX_SCA_OPCODE_MOV: op("MOV", arg_dst(), arg(src[0])); break;
					case RSX_SCA_OPCODE_RCP: op("RCP", arg_dst(), arg(src[0])); break;
					case RSX_SCA_OPCODE_RCC: op("RCC", arg_dst(), arg(src[0])); break;
					case RSX_SCA_OPCODE_RSQ: op("RSQ", arg_dst(), arg(src[0])); break;
					case RSX_SCA_OPCODE_EXP: op("EXP", arg_dst(), arg(src[0])); break;
					case RSX_SCA_OPCODE_LOG: op("LOG", arg_dst(), arg(src[0])); break;
					case RSX_SCA_OPCODE_LIT: op("LIT", arg_dst(), arg(src[0])); break;
					case RSX_SCA_OPCODE_BRA: op("BRA", arg(src[0])); break;
					case RSX_SCA_OPCODE_BRI: op("BRI", arg(src[0])); break;
					case RSX_SCA_OPCODE_CAL: op("CAL", arg(src[0])); break;
					case RSX_SCA_OPCODE_CLI: op("CLI", arg(src[0])); break;
					case RSX_SCA_OPCODE_RET: op("RET"); break;
					case RSX_SCA_OPCODE_LG2: op("LG2", arg_dst(), arg(src[0])); break;
					case RSX_SCA_OPCODE_EX2: op("EX2", arg_dst(), arg(src[0])); break;
					case RSX_SCA_OPCODE_SIN: op("SIN", arg_dst(), arg(src[0])); break;
					case RSX_SCA_OPCODE_COS: op("COS", arg_dst(), arg(src[0])); break;
					//case RSX_SCA_OPCODE_BRB: op("BRB", arg_dst(), arg(src[0])); break;
					//case RSX_SCA_OPCODE_CLB: op("CLB", arg_dst(), arg(src[0])); break;
					//case RSX_SCA_OPCODE_PSH: op("PSH", arg_dst(), arg(src[0])); break;
					//case RSX_SCA_OPCODE_POP: op("POP", arg_dst(), arg(src[0])); break;

					default:
						throw std::runtime_error(fmt::Format("#Unknown vp opcode 0x%x", fmt::by_value(d1.vec_opcode)));
						break;
					}

					is_sca = false;
					switch (d1.vec_opcode)
					{
					case RSX_VEC_OPCODE_NOP: break;
					case RSX_VEC_OPCODE_MOV: op("MOV", arg_dst(), arg(src[0])); break;
					case RSX_VEC_OPCODE_MUL: op("MUL", arg_dst(), arg(src[0]), arg(src[1])); break;
					case RSX_VEC_OPCODE_ADD: op("ADD", arg_dst(), arg(src[0]), arg(src[2])); break;
					case RSX_VEC_OPCODE_MAD: op("MAD", arg_dst(), arg(src[0]), arg(src[1]), arg(src[2])); break;
					case RSX_VEC_OPCODE_DP3: op("DP3", arg_dst(), arg(src[0]), arg(src[1])); break;
					case RSX_VEC_OPCODE_DPH: op("DPH", arg_dst(), arg(src[0]), arg(src[1])); break;
					case RSX_VEC_OPCODE_DP4: op("DP4", arg_dst(), arg(src[0]), arg(src[1])); break;
					case RSX_VEC_OPCODE_DST: op("DST", arg_dst(), arg(src[0]), arg(src[1])); break;
					case RSX_VEC_OPCODE_MIN: op("MIN", arg_dst(), arg(src[0]), arg(src[1])); break;
					case RSX_VEC_OPCODE_MAX: op("MAX", arg_dst(), arg(src[0]), arg(src[1])); break;
					case RSX_VEC_OPCODE_SLT: op("SLT", arg_dst(), arg(src[0]), arg(src[1])); break;
					case RSX_VEC_OPCODE_SGE: op("SGE", arg_dst(), arg(src[0]), arg(src[1])); break;
					case RSX_VEC_OPCODE_ARL: op("ARL", arg_dst(), arg(src[0])); break;
					case RSX_VEC_OPCODE_FRC: op("FRC", arg_dst(), arg(src[0])); break;
					case RSX_VEC_OPCODE_FLR: op("FLR", arg_dst(), arg(src[0])); break;
					case RSX_VEC_OPCODE_SEQ: op("SEQ", arg_dst(), arg(src[0])); break;
					case RSX_VEC_OPCODE_SFL: op("SFL", arg_dst(), arg(src[0])); break;
					case RSX_VEC_OPCODE_SGT: op("SGT", arg_dst(), arg(src[0])); break;
					case RSX_VEC_OPCODE_SLE: op("SLE", arg_dst(), arg(src[0]), arg(src[1])); break;
					case RSX_VEC_OPCODE_SNE: op("SNE", arg_dst(), arg(src[0]), arg(src[1])); break;
					case RSX_VEC_OPCODE_STR: op("STR", arg_dst(), arg(src[0]), arg(src[1])); break;
					case RSX_VEC_OPCODE_SSG: op("SSG", arg_dst(), arg(src[0]), arg(src[1])); break;
					case RSX_VEC_OPCODE_TXL: op("TXL", arg_dst(), texture(), arg(src[0]), predeclared_variable("2D")); break;

					default:
						throw std::runtime_error(fmt::Format("#Unknown vp opcode 0x%x", fmt::by_value(d1.vec_opcode)));
						break;
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

			std::string shader()
			{
				return m_shader;
			}
		};
	}
}