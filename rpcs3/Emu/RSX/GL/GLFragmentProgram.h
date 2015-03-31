#pragma once
#include "GLShaderParam.h"
#include "Emu/RSX/RSXFragmentProgram.h"
#include "Utilities/Thread.h"
#include "gpu_program.h"

struct GLFragmentDecompilerThread : public ThreadBase
{
	std::string main;
	std::string& m_shader;
	GLParamArray& m_parr;
	u32 m_addr;
	u32& m_size;
	u32 m_const_index;
	u32 m_offset;
	u32 m_location;
	u32 m_ctrl;
	u32 m_loop_count;
	int m_code_level;
	std::vector<u32> m_end_offsets;
	std::vector<u32> m_else_offsets;

	GLFragmentDecompilerThread(std::string& shader, GLParamArray& parr, u32 addr, u32& size, u32 ctrl)
		: ThreadBase("Fragment Shader Decompiler Thread")
		, m_shader(shader)
		, m_parr(parr)
		, m_addr(addr)
		, m_size(size) 
		, m_const_index(0)
		, m_location(0)
		, m_ctrl(ctrl)
	{
		m_size = 0;
	}

	OPDEST dst;
	SRC0 src0;
	SRC1 src1;
	SRC2 src2;

	std::string GetMask();

	void SetDst(std::string code, bool append_mask = true);
	void AddCode(const std::string& code);
	std::string AddReg(u32 index, int fp16);
	bool HasReg(u32 index, int fp16);
	std::string AddCond();
	std::string AddConst();
	std::string AddTex();
	std::string Format(const std::string& code);

	void AddCodeCond(const std::string& dst, const std::string& src);
	std::string GetCond();
	template<typename T> std::string GetSRC(T src);
	std::string BuildCode();

	virtual void Task();

	u32 GetData(const u32 d) const { return d << 16 | d >> 16; }
};

/** Storage for an Fragment Program in the process of of recompilation.
 *  This class calls OpenGL functions and should only be used from the RSX/Graphics thread.
 */
class GLFragmentProgram
{
public:
	GLFragmentProgram();
	~GLFragmentProgram();

	GLParamArray parr;
	u32 id;
	std::string shader;

	/**
	 * Decompile a fragment shader located in the PS3's Memory.  This function operates synchronously.
	 * @param prog RSXShaderProgram specifying the location and size of the shader in memory
	 */
	void Decompile(RSXFragmentProgram& prog);

	/**
	* Asynchronously decompile a fragment shader located in the PS3's Memory.
	* When this function is called you must call Wait() before GetShaderText() will return valid data.
	* @param prog RSXShaderProgram specifying the location and size of the shader in memory
	*/
	void DecompileAsync(RSXFragmentProgram& prog);

	/** Wait for the decompiler task to complete decompilation. */
	void Wait();

	/** Compile the decompiled fragment shader into a format we can use with OpenGL. */
	void Compile();

private:
	/** Threaded fragment shader decompiler responsible for decompiling this program */
	GLFragmentDecompilerThread* m_decompiler_thread;

	/** Deletes the shader and any stored information */
	void Delete();
};


namespace gl
{
	namespace fragment_program
	{
		static const char swizzle_mask[4] = { 'x', 'y', 'z', 'w' };

		class decompiler : gpu_program_builder<>
		{
			vm::ptr<u32> m_data;

			OPDEST dst;
			SRC0 src0;
			SRC1 src1;
			SRC2 src2;

			bool next_is_constant = false;

			std::string m_shader;

		public:
			decompiler(vm::ptr<u32> data) : m_data(data)
			{
				gpu_program_builder::context.header = "!!ARBfp1.0";
				gpu_program_builder::context.extensions.push_back("NV_fragment_program2");
				gpu_program_builder::context.extensions.push_back("ARB_draw_buffers");
			}

		private:
			static u32 swap_bytes(u32 data)
			{
				return (data << 16) | (data >> 16);
			}

			template<typename T>
			gpu4_program_context::argument arg(T src)
			{
				gpu4_program_context::argument result;
				switch (src.reg_type)
				{
				case 0: //template register
					result = variable("TEMP", (src.fp16 ? "H" : "R") + std::to_string(src.tmp_reg_index));
					break;

				case 1: //input register
				{
					static const std::string reg_table[] =
					{
						"fragment.position",
						"fragment.color",
						"fragment.color.secondary",
						"fragment.fogcoord",
						"fragment.texcoord[0]",
						"fragment.texcoord[1]",
						"fragment.texcoord[2]",
						"fragment.texcoord[3]",
						"fragment.texcoord[4]",
						"fragment.texcoord[5]",
						"fragment.texcoord[6]",
						"fragment.texcoord[7]",
						"fragment.texcoord[8]",
						"fragment.texcoord[9]"
					};

					if (dst.src_attr_reg_num < std::distance(std::begin(reg_table), std::end(reg_table)))
					{
						result = predeclared_variable(reg_table[dst.src_attr_reg_num]);
					}
					else
					{
						throw std::runtime_error("Bad src reg num");
					}
				}
				break;

				case 2: //constant
				{
					static f32 c0;
					static f32 c1;
					static f32 c2;
					static f32 c3;

					if (!next_is_constant)
					{
						(u32&)c0 = swap_bytes(m_data[0]);
						(u32&)c1 = swap_bytes(m_data[1]);
						(u32&)c2 = swap_bytes(m_data[2]);
						(u32&)c3 = swap_bytes(m_data[3]);

						next_is_constant = true;
					}

					result = constant(c0, c1, c2, c3);
				}
				break;
				}

				std::string mask;
				mask += swizzle_mask[src.swizzle_x];
				mask += swizzle_mask[src.swizzle_y];
				mask += swizzle_mask[src.swizzle_z];
				mask += swizzle_mask[src.swizzle_w];

				return result.mask(mask).abs(src.abs).neg(src.neg);
			}

			template<>
			gpu4_program_context::argument arg(OPDEST dst)
			{
				std::string mask;

				if (dst.mask_x) mask += "x";
				if (dst.mask_y) mask += "y";
				if (dst.mask_z) mask += "z";
				if (dst.mask_w) mask += "w";

				return variable("TEMP", (dst.fp16 ? "H" : "R") + std::to_string(dst.dest_reg)).mask(mask);
			}

			gpu4_program_context::argument condition()
			{
				if (src0.exec_if_gr && src0.exec_if_lt && src0.exec_if_eq)
				{
					return{};
				}

				static const char swizzle_selector[4] = { 'x', 'y', 'z', 'w' };

				std::string swizzle, cond;
				swizzle += swizzle_selector[src0.cond_swizzle_x];
				swizzle += swizzle_selector[src0.cond_swizzle_y];
				swizzle += swizzle_selector[src0.cond_swizzle_z];
				swizzle += swizzle_selector[src0.cond_swizzle_w];

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

				return predeclared_variable(cond + std::to_string(src0.cond_reg_index)).mask(swizzle);
			}

			enum instr_flag
			{
				F = 1 << 0, //floating - point data type modifiers
				I = 1 << 1, //signed and unsigned integer data type modifiers
				C = 1 << 2, //condition code update modifiers
				S = 1 << 3, //clamping(saturation) modifiers
				H = 1 << 4, //half - precision float data type suffix
			};

			std::string instr(std::string instruction, int flags = 0)
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
						instruction += ".CC" + std::to_string(src0.cond_mod_reg_index);
					}

					auto cond = condition();

					if (cond)
						instruction += "(" + context.get(cond) + ")";
				}

				return instruction;
			}

		public:
			decompiler& decompile(u32 control)
			{
				for (int i = 0; i < 512; ++i)
				{
					if (next_is_constant)
					{
						m_data += 4;
						next_is_constant = false;
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
					case RSX_FP_OPCODE_KIL: op(instr("KIL", F | I | H), arg(src0)); break;
					case RSX_FP_OPCODE_PK4: op(instr("PK4", F | I | H), arg(src0)); break;
					case RSX_FP_OPCODE_UP4: op(instr("UP4", F | I | H), arg(src0)); break;
					case RSX_FP_OPCODE_DDX: op(instr("DDX", F | I | H), arg(src0)); break;
					case RSX_FP_OPCODE_DDY: op(instr("DDY", F | I | H), arg(dst), arg(src0)); break;
					case RSX_FP_OPCODE_TEX:
					case RSX_FP_OPCODE_TXP:
					case RSX_FP_OPCODE_TXD:
					case RSX_FP_OPCODE_RCP:
					case RSX_FP_OPCODE_RSQ:
					case RSX_FP_OPCODE_EX2:
					case RSX_FP_OPCODE_LG2:
					case RSX_FP_OPCODE_LIT:
					case RSX_FP_OPCODE_LRP:
					case RSX_FP_OPCODE_STR:
					case RSX_FP_OPCODE_SFL:
					case RSX_FP_OPCODE_COS:
					case RSX_FP_OPCODE_SIN:
					case RSX_FP_OPCODE_PK2:
					case RSX_FP_OPCODE_UP2:
					case RSX_FP_OPCODE_POW:
					case RSX_FP_OPCODE_PKB:
					case RSX_FP_OPCODE_UPB:
					case RSX_FP_OPCODE_PK16:
					case RSX_FP_OPCODE_UP16:
					case RSX_FP_OPCODE_BEM:
					case RSX_FP_OPCODE_PKG:
					case RSX_FP_OPCODE_UPG:
					case RSX_FP_OPCODE_DP2A:
					case RSX_FP_OPCODE_TXL:
					case RSX_FP_OPCODE_TXB:
					case RSX_FP_OPCODE_TEXBEM:
					case RSX_FP_OPCODE_TXPBEM:
					case RSX_FP_OPCODE_BEMLUM:
					case RSX_FP_OPCODE_REFL:
					case RSX_FP_OPCODE_TIMESWTEX:
					case RSX_FP_OPCODE_DP2:
					case RSX_FP_OPCODE_NRM:
					case RSX_FP_OPCODE_DIV:
					case RSX_FP_OPCODE_DIVSQ:
					case RSX_FP_OPCODE_LIF:
					case RSX_FP_OPCODE_FENCT:
					case RSX_FP_OPCODE_FENCB:

					//case RSX_FP_OPCODE_BRK: op(instr("BRK"), arg(src0)); break;
					//case RSX_FP_OPCODE_CAL: op(instr("CAL"), arg(src0)); break;
					case RSX_FP_OPCODE_IFE:
					case RSX_FP_OPCODE_LOOP:
					case RSX_FP_OPCODE_REP:
					case RSX_FP_OPCODE_RET:
					default:
						throw std::runtime_error(fmt::format("Unknown/illegal fp instruction: 0x%x", opcode));
						//LOG_ERROR(RSX, "Unknown/illegal fp instruction: 0x%x", opcode);
						break;
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
						op((control & 0x40 ? "MOVR" : "MOVH"), predeclared_variable(entry.first), variable("TEMP", entry.second));
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